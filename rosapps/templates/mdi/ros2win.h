/*
 *  ReactOS to Win32 entry points for testing
 *
 *  ros2win.h
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
    
#ifndef __ROS2WIN_H__
#define __ROS2WIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ROS2WIN_H__MAIN__
#define DefWindowProc RosWindowProc
#define DefFrameProc RosFrameProc
#define DefMDIChildProc RosMDIChildProc
#define DefDlgProc RosDlgProc
#endif

LRESULT CALLBACK RosWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK RosFrameProc(HWND hWnd, HWND hMdi, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK RosMDIChildProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK RosDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


#ifdef __cplusplus
};
#endif

#endif // __ROS2WIN_H__
