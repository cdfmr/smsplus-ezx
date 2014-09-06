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

#ifndef __QSMSPLUS__
#define __QSMSPLUS__

#ifdef __cplusplus
	extern "C" {
#endif

#include "types.h"
#include "sms.h"
#include "system.h"

#define APP_TITLE	"SMS Plus / EZX v1.1"

#define EZX_WIDTH	240
#define EZX_HEIGHT	320
#define EZX_PITCH	EZX_WIDTH * 2
#define EZX_FBSIZE	EZX_PITCH * EZX_HEIGHT
#define SMS_WIDTH	256
#define SMS_HEIGHT	192
#define GG_WIDTH	160
#define GG_HEIGHT	144
#define SMS_PITCH	SMS_WIDTH * 2
#define GG_PITCH	GG_WIDTH * 2

typedef struct {
	int country;
	int throttle;
	int fullscreen;
	int volume;
	int mute;
	int enablefm;
	int keymap;
	char game_name[PATH_MAX];
} sp_option;

extern sp_option option;
extern int exit_flag;
extern int kst_up, kst_down, kst_left, kst_right, kst_a, kst_b, kst_start;

int smsplus_init();
void smsplus_emulate();
void smsplus_uninit();
int load_state();
int save_state();
void audio_setvolume(int vol);

#ifdef __cplusplus
	} // extern "C"
#endif

#endif
