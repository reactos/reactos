/*
 *  ReactOS winfile
 *
 *  framewnd.h
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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

#ifndef __FRAMEWND_H__
#define __FRAMEWND_H__

#ifdef __cplusplus
extern "C" {
#endif

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


LRESULT CALLBACK FrameWndProc(HWND, UINT, WPARAM, LPARAM);
HWND CreateChildWindow(int drv_id);

void resize_frame_client(HWND hWnd);
//BOOL activate_drive_window(LPCTSTR path);
//void toggle_child(HWND hWnd, UINT cmd, HWND hchild);


#ifdef __cplusplus
};
#endif

#endif // __FRAMEWND_H__
