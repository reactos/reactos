/*
 *  ReactOS File Manager
 *
 *  draw.h
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
    
#ifndef __DRAW_H__
#define __DRAW_H__

#ifdef __cplusplus
extern "C" {
#endif



void draw_item(Pane* pane, LPDRAWITEMSTRUCT dis, Entry* entry, int calcWidthCol);
void draw_splitbar(HWND hwnd, int x);



#ifdef __cplusplus
};
#endif

#endif // __DRAW_H__