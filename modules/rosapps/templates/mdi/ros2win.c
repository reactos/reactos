/*
 *  ReactOS to Win32 entry points for testing
 *
 *  ros2win.c
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

#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif

#define __ROS2WIN_H__MAIN__
#include "ros2win.h"

//#undef DefWndProc
//#undef DefFrameProc
//#undef DefMDIChildProc

extern HINSTANCE hInst;


LRESULT CALLBACK RosWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
//        return TRUE;
        break;
    case WM_COMMAND:
//        return TRUE;
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK RosDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
//        return TRUE;
        break;
    }
    return DefDlgProc(hDlg, message, wParam, lParam);
}

LRESULT CALLBACK RosFrameProc(HWND hWnd, HWND hMdi, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefFrameProc(hWnd, hMdi, message, wParam, lParam);
}

LRESULT CALLBACK RosMDIChildProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefMDIChildProc(hWnd, message, wParam, lParam);
}


/*
*/
