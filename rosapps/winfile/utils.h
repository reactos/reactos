/*
 *  ReactOS winfile
 *
 *  utils.h
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

#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



void display_error(HWND hwnd, DWORD error);
void frame_get_clientspace(HWND hwnd, PRECT prect);
BOOL toggle_fullscreen(HWND hwnd);
void fullscreen_move(HWND hwnd);

BOOL calc_widths(Pane* pane, BOOL anyway);
void calc_single_width(Pane* pane, int col);
void read_directory(Entry* parent, LPCTSTR path, int sortOrder);
int is_registered_type(LPCTSTR ext);
int is_exe_file(LPCTSTR ext);
void set_curdir(ChildWnd* child, Entry* entry);
void get_path(Entry* dir, PTSTR path);



#ifdef __cplusplus
};
#endif

#endif // __UTILS_H__
