/*
 *  ReactOS Task Manager
 *
 *  graph.h
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
	
#ifndef __GRAPH_H
#define __GRAPH_H


#define BRIGHT_GREEN	RGB(0, 255, 0)
#define DARK_GREEN		RGB(0, 130, 0)
#define RED				RGB(255, 0, 0)

extern	LONG				OldGraphWndProc;

LRESULT CALLBACK	Graph_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif // __GRAPH_H