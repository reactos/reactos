/*
 *  ReactOS File Manager
 *
 *  run.h
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// run.h - definitions necessary to use Microsoft's "Run" dialog
// Undocumented Windows call
// use the type below to declare a function pointer

// Information taken from http://www.geocities.com/SiliconValley/4942/
// Copyright © 1998-1999 James Holderness. All Rights Reserved.
// jholderness@geocities.com

#ifndef __RUN_H__
#define __RUN_H__

#ifdef __cplusplus
extern "C" {
#endif


void OnFileRun(void);
BOOL OpenTarget(HWND hWnd, TCHAR* target);


#ifdef __cplusplus
};
#endif

#endif // __RUN_H__
