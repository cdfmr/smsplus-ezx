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

#include <qapplication.h>
#include <stdio.h>
#include <stdlib.h>

#include "qmainapp.h"
#include "qmainwnd.h"
#include "qspthread.h"
#include "qsmsplus.h"

QMainWnd *qmainwnd;

//-----------------------------------------------------------------------------
// parse_args()
//-----------------------------------------------------------------------------
static int parse_args(int argc, char **argv)
{
	int i;
	
	strcpy(option.game_name, argv[1]);
	option.country = TERRITORY_EXPORT;
	option.volume = 4;
	
	for(i = 2; i < argc; ++i) {
		if (strcasecmp(argv[i], "--japan") == 0)
			option.country = TERRITORY_DOMESTIC;
		else if (strcasecmp(argv[i], "--throttle") == 0)
			option.throttle = 1;
		else if (strcasecmp(argv[i], "--fullscreen") == 0)
			option.fullscreen = 1;
		else if (strcasecmp(argv[i], "--volume") == 0) {
			if (++i < argc) {
				option.volume = atoi(argv[i]);
				if (option.volume < 0)
					option.volume = 0;
				else if (option.volume > 10)
					option.volume = 10;
			}
		}
		else if (strcasecmp(argv[i], "--mute") == 0)
			option.mute = 1;
		else if (strcasecmp(argv[i], "--enablefm") == 0)
			option.enablefm = 1;
		else if (strcasecmp(argv[i], "--keymap") == 0) {
			if (++i < argc)
				option.keymap = atoi(argv[i]);
		}
		else 
			printf("WARNING: unknown option '%s'.\n", argv[i]);
	}
	
	return 1;
}

//-----------------------------------------------------------------------------
// main()
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
	printf("%s (Build on %s)\n", APP_TITLE, __DATE__);
	printf("Copyright (C) 1998-2004 Charles MacDonald\n");
	printf("Copyright (C) 2005-2006 Lin Fan (EZX Port)\n");
	printf("\n");

	if(argc < 2) {
		printf("Usage: %s <filename> [--options]\n", argv[0]);
		printf("Options:\n");
		printf("  --japan        \t set the machine type as DOMESTIC instead of OVERSEAS.\n");
		//printf("  --throttle     \t limit fps to 60. \n");
		printf("  --fullscreen   \t stretch video to fit the screen. \n");
		printf("  --volume <0-10>\t set volume. \n");
		printf("  --mute         \t disable sound.\n");
		printf("  --enablefm     \t enable YM2413 sound.\n");
		printf("  --keymap <0-3> \t set different key map. \n");
		return 1;
	}

	memset(&option, 0, sizeof(option));
	if (!parse_args(argc, argv))
		return 1;

	if (!smsplus_init())
		return 1;
	
	QApplication app(argc, argv);
	QMainWnd mainwnd (NULL, "", Qt::WStyle_Customize | Qt::WStyle_NoBorder | Qt::WStyle_StaysOnTop);
	qmainwnd = &mainwnd;
	mainwnd.setGeometry(0, 0, EZX_WIDTH, EZX_HEIGHT);
	app.setMainWidget(&mainwnd);
	mainwnd.show();
	QSPThread *threadEmulation = new QSPThread();
	threadEmulation->start();
	app.exec();
	exit_flag = 1;
	threadEmulation->wait(500);
	delete threadEmulation;

	smsplus_uninit();
	
	return 0;
}

extern "C" void update_video()
{
	qmainwnd->updateGraphic();
}

