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

#ifndef __QMAINWND__
#define __QMAINWND__

#include <qwidget.h>
#include "qsmsplus.h"

class QMainWnd: public QWidget
{
	Q_OBJECT
	public:
		QMainWnd(QWidget *parent = NULL, const char *name = NULL, WFlags f = 0);
		~QMainWnd();
		void updateGraphic();
	private:
		int fullscreenToggled;
		void saveSnapshot();
		virtual void keyPressEvent(QKeyEvent *event);
		virtual void keyReleaseEvent(QKeyEvent *event);
	protected:
		void focusOutEvent(QFocusEvent *);
};

#endif //__QMAINWND__
