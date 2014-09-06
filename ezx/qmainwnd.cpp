//---------------------------------------------------------------------------
// SMS Plus / EZX
// Sega Master System / Game Gear emulator for EZX platform
// Based on SMS Plus by Charles MacDonald
// EZX version ported by Lin Fan (cnlinfan@gmail.com)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version. See also the license.txt file for
//	additional informations.
//---------------------------------------------------------------------------

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include <qapplication.h>
#include <qwidget.h>
#include <qdirectpainter_qws.h>
#include <qpixmap.h>

#include "qmainwnd.h"

typedef struct {
	unsigned char *data;
	int width;
	int height;
	int linestep;
} sp_bitmap;

int	key_ezx_up, key_ezx_down, key_ezx_left, key_ezx_right,
	key_ezx_send, key_ezx_power, key_ezx_ok,
	key_ezx_player, key_ezx_radio, key_ezx_camera,
	key_ezx_volup, key_ezx_voldown;

int	key_vm_a, key_vm_b, key_vm_start;

int	kst_ezx_ok, kst_ezx_volup, kst_ezx_voldown;

sp_bitmap	rbitmap;	//rotated bitmap data
int	centerByteOffset;	//left-top position to draw images in center of screen
int	bitmapByteOffset;	//left-top position of gg data in the whole bitmap
int	bmpWidth, bmpHeight;	//original bitmap size
int	fullWidth, fullHeight;	//size of full screen images
int	fullByteOffset;		//left-top position to draw images in full screen mode

int	focusLost = 0;

#define SMS_OFFSET_X	((EZX_WIDTH - SMS_HEIGHT) >> 1)
#define SMS_OFFSET_Y	((EZX_HEIGHT - SMS_WIDTH) >> 1)
#define GG_OFFSET_X	((EZX_WIDTH - GG_HEIGHT) >> 1)
#define GG_OFFSET_Y	((EZX_HEIGHT - GG_WIDTH) >> 1)
#define SMS_GG_OFFSET_X	((SMS_WIDTH - GG_WIDTH) >> 1)
#define SMS_GG_OFFSET_Y	((SMS_HEIGHT - GG_HEIGHT) >> 1)
#define SMS_WIDTH_FS	240
#define SMS_HEIGHT_FS	320
#define GG_WIDTH_FS	240
#define GG_HEIGHT_FS	267
#define GG_OFFSET_Y_FS	((EZX_HEIGHT - GG_HEIGHT_FS) >> 1)
	
//-----------------------------------------------------------------------------
// file_exists()
//-----------------------------------------------------------------------------
int file_exists(char *filename)
{
	FILE	*f;

	f = fopen(filename, "rb");
	if (f != NULL) {
		fclose(f);
		return 1;
	}
	
	return 0;
}

static inline void bitBlt(unsigned char *src, int srcstep, unsigned char *dst, int dststep, int width, int height)
{
	for (int i = 0; i < height; i++) {
		memcpy(dst, src, srcstep);
		src += srcstep;
		dst += dststep;
	}
}

static inline void fastRotateBlt(unsigned char *src, int srcstep, int srcwidth, int srcheight, unsigned char *dst, int dststep)
{
	int w = srcwidth >> 1;
	int h = srcheight >> 1;
	UINT32 *sp1, *sp2, *dp1, *dp2;

	sp1 = (UINT32 *)src;
	sp2 = sp1 + (srcstep >> 2);
	dp1 = (UINT32 *)dst + h * (srcwidth - 1);
	dp2 = dp1 - h;

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			*dp1 = (*sp1 & 0xffff) | ((*sp2 & 0xffff) << 16);
			*dp2 = ((*sp1 & 0xffff0000) >> 16) | (*sp2 & 0xffff0000);
			sp1++;
			sp2++;
			dp1 -= srcheight;
			dp2 -= srcheight;
		}
		sp1 += (srcstep >> 1) - w;
		sp2 = sp1 + (srcstep >> 2);
		dp1 += h * srcwidth + 1;
		dp2 = dp1 - h;
	}
}

static inline void stretchBlt(unsigned char *src, int srcwidth, int srcheight,
                       unsigned char *dst, int dstwidth, int dstheight)
{
	int px, py, spx = 0, spy = 0, cx = 0, cy = 0;

	UINT16 *inputData	= (UINT16 *)src;
	UINT16 *outputData	= (UINT16 *)dst;

	int newline = 1;
	for (py = 0; py < dstheight; py++) {
		if (newline) {
			for (px = 0; px < dstwidth; px++) {
				outputData[py * dstwidth + px] = inputData[spy * srcwidth + spx];
				cx += srcwidth;
				if (cx >= dstwidth) {
					cx -= dstwidth;
					spx++;
				}
			}
			spx = 0;
		}
		else
			memcpy(outputData + py * dstwidth, outputData + (py - 1) * dstwidth, dstwidth * 2);

		cy += srcheight;
		newline = cy >= dstheight;
		if (newline) {
			cy -= dstheight;
			spy++;
		}
	}
}

// inline void stretchBlt(unsigned char *src, int srcwidth, int srcheight,
//                        unsigned char *dst, int dstwidth, int dstheight)
// {
// 	int px, py, spx = 0, spy = 0, cx = 0, cy = 0;
// 
// 	UINT16 *inputData	= (UINT16 *)src;
// 	UINT16 *outputData	= (UINT16 *)dst;
// 
// 	for (py = 0; py < dstheight; py++) {
// 		for (px = 0; px < dstwidth; px++) {
// 			outputData[py * dstwidth + px] = inputData[spy * srcwidth + spx];
// 			cx += srcwidth;
// 			while (cx >= dstwidth) {
// 				cx -= dstwidth;
// 				spx++;
// 			}
// 		}
// 		spx = 0;
// 		cy += srcheight;
// 		while (cy >= dstheight) {
// 			cy -= dstheight;
// 			spy++;
// 		}
// 	}
// }

QMainWnd::QMainWnd(QWidget *parent, const char *name, WFlags f): QWidget(parent, name, f)
{
	setBackgroundMode(NoBackground);
	setFocusPolicy(QWidget::StrongFocus);

	if (1) {
		key_ezx_up	= Qt::Key_Up;
		key_ezx_down	= Qt::Key_Down;
		key_ezx_left	= Qt::Key_Left;
		key_ezx_right	= Qt::Key_Right;
		key_ezx_send	= Qt::Key_F6;
		key_ezx_power	= Qt::Key_F7;
		key_ezx_ok	= Qt::Key_Enter;
		key_ezx_player	= Qt::Key_F9;
		key_ezx_radio	= Qt::Key_F10;
		key_ezx_camera	= Qt::Key_F2;
		key_ezx_volup	= Qt::Key_F11;
		key_ezx_voldown	= Qt::Key_F12;
	}
	switch (option.keymap) {
		case 1:
			key_vm_a = key_ezx_radio;
			key_vm_b = key_ezx_player;
			key_vm_start = key_ezx_volup;
			break;
		case 2:
			key_vm_a = key_ezx_player;
			key_vm_b = key_ezx_volup;
			key_vm_start = key_ezx_radio;
			break;
		case 3:
			key_vm_a = key_ezx_volup;
			key_vm_b = key_ezx_player;
			key_vm_start = key_ezx_radio;
			break;
		default: //0
			key_vm_a = key_ezx_player;
			key_vm_b = key_ezx_radio;
			key_vm_start = key_ezx_volup;
			break;
	}

	memset(QDirectPainter(this).frameBuffer(), 0, EZX_FBSIZE);

	fullscreenToggled = 0;

	rbitmap.width = IS_GG ? GG_HEIGHT : SMS_HEIGHT;
	rbitmap.height = IS_GG ? GG_WIDTH : SMS_WIDTH;
	rbitmap.linestep = rbitmap.width * 2;
	rbitmap.data = (unsigned char *)malloc(rbitmap.linestep * rbitmap.height);
	
	if (!IS_GG) {
		centerByteOffset = EZX_PITCH * SMS_OFFSET_Y + 2 * SMS_OFFSET_X;
		bitmapByteOffset = 0;
		bmpWidth = SMS_WIDTH;
		bmpHeight = SMS_HEIGHT;
		fullWidth = SMS_WIDTH_FS;
		fullHeight = SMS_HEIGHT_FS;
		fullByteOffset = 0;
	}
	else {
		centerByteOffset = EZX_PITCH * GG_OFFSET_Y + 2 * GG_OFFSET_X;
		bitmapByteOffset = SMS_PITCH * SMS_GG_OFFSET_Y + 2 * SMS_GG_OFFSET_X;
		bmpWidth = GG_WIDTH;
		bmpHeight = GG_HEIGHT;
		fullWidth = GG_WIDTH_FS;
		fullHeight = GG_HEIGHT_FS;
		fullByteOffset = EZX_PITCH * GG_OFFSET_Y_FS;
	}
}

QMainWnd::~QMainWnd()
{
	free(rbitmap.data);
}

void QMainWnd::updateGraphic()
{
	if (focusLost) return;

	if (fullscreenToggled) {
		memset(QDirectPainter(this).frameBuffer(), 0, EZX_FBSIZE);
		option.fullscreen = !option.fullscreen;
		fullscreenToggled = 0;
	}

	fastRotateBlt(
		bitmap.data + bitmapByteOffset, bitmap.pitch,
		bmpWidth, bmpHeight,
		rbitmap.data, rbitmap.linestep
	);

	if (option.fullscreen)
		stretchBlt(
			rbitmap.data, rbitmap.width, rbitmap.height,
			QDirectPainter(this).frameBuffer() + fullByteOffset, fullWidth, fullHeight
		);
	else
		bitBlt(
			rbitmap.data, rbitmap.linestep,
			QDirectPainter(this).frameBuffer() + centerByteOffset, EZX_PITCH,
			rbitmap.width, rbitmap.height
		);
}

void QMainWnd::keyPressEvent(QKeyEvent *event)
{
	int key = event->key();

	if (key == key_ezx_up)
		kst_right = 1;
	else if (key == key_ezx_down)
		kst_left = 1;
	else if (key == key_ezx_left)
		kst_up = 1;
	else if (key == key_ezx_right)
		kst_down = 1;
	else if (key == key_vm_a)
		kst_a = 1;
	else if (key == key_vm_b)
		kst_b = 1;
	else if (key == key_vm_start)
		kst_start = 1;

	if (key == key_ezx_ok)
		kst_ezx_ok = 1;
	else if (key == key_ezx_volup)
		kst_ezx_volup = 1;
	else if (key == key_ezx_voldown)
		kst_ezx_voldown = 1;
}

void QMainWnd::keyReleaseEvent(QKeyEvent *event)
{
	int key = event->key();

	if (key == key_ezx_up)
		kst_right = 0;
	else if (key == key_ezx_down)
		kst_left = 0;
	else if (key == key_ezx_left)
		kst_up = 0;
	else if (key == key_ezx_right)
		kst_down = 0;
	else if (key == key_vm_a)
		kst_a = 0;
	else if (key == key_vm_b)
		kst_b = 0;
	else if (key == key_vm_start)
		kst_start = 0;

	if (key == key_ezx_send)
		save_state();
	else if (key == key_ezx_power)
		load_state();
	else if (key == key_ezx_ok) {
		kst_ezx_ok = 0;
		if (kst_ezx_volup && kst_ezx_voldown)
			qApp->quit();
	}
	else if (key == key_ezx_camera) {
		if (kst_ezx_ok)
			system_reset();
		else
			saveSnapshot();
	}
	else if (key == key_ezx_volup) {
		kst_ezx_volup = 0;
		if (kst_ezx_ok) {
			if (option.volume < 10) {
				option.volume++;
				audio_setvolume(option.volume);
			}
		}
	}
	else if (key == key_ezx_voldown) {
		kst_ezx_voldown = 0;
		if (kst_ezx_ok) {
			if (option.volume > 0) {
				option.volume--;
				audio_setvolume(option.volume);
			}
		}
		else
			fullscreenToggled = 1;
	}
}

void QMainWnd::saveSnapshot()
{
	QPixmap *pm;
	char filename[PATH_MAX];

	int count = 1;
	strcpy(filename, option.game_name);
	sprintf(strrchr(filename, '.'), ".%03d.png", count);
	while (file_exists(filename) && (count < 999)) {
		strcpy(filename, option.game_name);
		sprintf(strrchr(filename, '.'), ".%03d.png", ++count);
	}

	pm = new QPixmap();

	if (IS_GG) {
		unsigned char *ggdata = (unsigned char *)malloc(GG_HEIGHT * GG_PITCH);
		pm->setRawData(ggdata, GG_WIDTH, GG_HEIGHT, 16, GG_PITCH);
		unsigned char *bmpdata = (unsigned char *)(bitmap.data + SMS_GG_OFFSET_Y * bitmap.pitch + SMS_GG_OFFSET_X * 2);
		unsigned char *rawdata = ggdata;
		for (int i = 0; i < GG_HEIGHT; i++) {
			memcpy(rawdata, bmpdata, GG_PITCH);
			rawdata += GG_PITCH;
			bmpdata += bitmap.pitch;
		}
		pm->save(QString::fromUtf8(filename), "PNG");
		free(ggdata);
	}
	else {
		pm->setRawData(bitmap.data, bitmap.width, bitmap.height, bitmap.depth, bitmap.pitch);
		pm->save(QString::fromUtf8(filename), "PNG");
	}

	delete pm;
}

void QMainWnd::focusOutEvent(QFocusEvent *)
{
	focusLost = 1;
	qApp->quit();
}
