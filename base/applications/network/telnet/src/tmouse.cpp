///////////////////////////////////////////////////////////////////////////////
//Telnet Win32 : an ANSI telnet client.
//Copyright (C) 1998  Paul Brannan
//Copyright (C) 1998  I.Ioannou
//Copyright (C) 1997  Brad Johnson
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//I.Ioannou
//roryt@hol.gr
//
///////////////////////////////////////////////////////////////////////////

// TMouse.cpp
// A simple class for handling mouse events
// Written by Paul Brannan <pbranna@clemson.edu>
// Last modified August 30, 1998

#include "tmouse.h"
#include "tconsole.h"

TMouse::TMouse(Tnclip &RefClipboard): Clipboard(RefClipboard) {
	hConsole = GetStdHandle(STD_INPUT_HANDLE);
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
}

TMouse::~TMouse() {
}

void TMouse::get_coords(COORD *start_coords, COORD *end_coords,
				COORD *first_coords, COORD *last_coords) {
	if(end_coords->Y < start_coords->Y ||
		(end_coords->Y == start_coords->Y && end_coords->X < start_coords->X))
	{
		*first_coords = *end_coords;
		*last_coords = *start_coords;
	} else {
		*first_coords = *start_coords;
		*last_coords = *end_coords;
	}
	last_coords->X++;
}

void TMouse::doMouse_init() {
	GetConsoleScreenBufferInfo(hStdout, &ConsoleInfo);
	chiBuffer = newBuffer();
	saveScreen(chiBuffer);
}

void TMouse::doMouse_cleanup() {
	restoreScreen(chiBuffer);
	delete[] chiBuffer;
}

void TMouse::move_mouse(COORD start_coords, COORD end_coords) {
	COORD screen_start = {0, 0};
	COORD first_coords, last_coords;
	DWORD Result;

	FillConsoleOutputAttribute(hStdout, normal,
		ConsoleInfo.dwSize.X * ConsoleInfo.dwSize.Y, screen_start, &Result);
					
	get_coords(&start_coords, &end_coords, &first_coords, &last_coords);
	FillConsoleOutputAttribute(hStdout, inverse, ConsoleInfo.dwSize.X * 
		(last_coords.Y - first_coords.Y) + (last_coords.X - first_coords.X),
		first_coords, &Result);
}

void TMouse::doClip(COORD start_coords, COORD end_coords) {
	// COORD screen_start = {0, 0};
	COORD first_coords, last_coords;
	DWORD Result;

	get_coords(&start_coords, &end_coords, &first_coords, &last_coords);

	// Allocate the minimal size buffer
	int data_size = 3 + ConsoleInfo.dwSize.X *
		(last_coords.Y - first_coords.Y) + (last_coords.X - first_coords.X);
	HGLOBAL clipboard_data = GlobalAlloc(GMEM_MOVEABLE + GMEM_DDESHARE,
		data_size);
	LPVOID mem_ptr = GlobalLock(clipboard_data);

	// Reset data_size so we can count the actual data size
	data_size = 0;

	// Read the console, put carriage returns at the end of each line if
	// reading more than one line (Paul Brannan 9/17/98)
	for(int j = first_coords.Y; j <= last_coords.Y; j++) {

		// Read line at (0,j)
		COORD coords;
		coords.X = 0;
		coords.Y = j;
		int length = ConsoleInfo.dwSize.X;

		if(j == first_coords.Y) {
			coords.X = first_coords.X;
			length = ConsoleInfo.dwSize.X - first_coords.X;
		} else {
			// Add a carriage return to the end of the previous line
			*((char *)mem_ptr + data_size++) = '\r';
			*((char *)mem_ptr + data_size++) = '\n';
		}

		if(j == last_coords.Y) {
			length -= (ConsoleInfo.dwSize.X - last_coords.X);
		}

		// Read the next line
		ReadConsoleOutputCharacter(hStdout, (LPTSTR)((char *)mem_ptr +
			data_size), length, coords, &Result);
		data_size += Result;

		// Strip the spaces at the end of the line
		if((j != last_coords.Y) && (first_coords.Y != last_coords.Y))
			while(*((char *)mem_ptr + data_size - 1) == ' ') data_size--;
	}
	if(first_coords.Y != last_coords.Y) {
		// Add a carriage return to the end of the last line
		*((char *)mem_ptr + data_size++) = '\r';
		*((char *)mem_ptr + data_size++) = '\n';
	}

	*((char *)mem_ptr + data_size) = 0;
	GlobalUnlock(clipboard_data);

	Clipboard.Copy(clipboard_data);
}

void TMouse::doMouse() {
	INPUT_RECORD InputRecord;
	DWORD Result;
	InputRecord.EventType = KEY_EVENT; // just in case
	while(InputRecord.EventType != MOUSE_EVENT) {
		if (!ReadConsoleInput(hConsole, &InputRecord, 1, &Result))
			return; // uh oh!  we don't know the starting coordinates!
	}
	if(InputRecord.Event.MouseEvent.dwButtonState == 0) return;
	if(!(InputRecord.Event.MouseEvent.dwButtonState &
		FROM_LEFT_1ST_BUTTON_PRESSED)) {
		Clipboard.Paste();
		return;
	}

	COORD screen_start = {0, 0};
    COORD start_coords = InputRecord.Event.MouseEvent.dwMousePosition;
	COORD end_coords = start_coords;
	BOOL done = FALSE;
	
	// init vars
	doMouse_init();
	int normal_bg = ini.get_normal_bg();
	int normal_fg = ini.get_normal_fg();
	if(normal_bg == -1) normal_bg = 0;		// FIX ME!!  This is just a hack
	if(normal_fg == -1) normal_fg = 7;
	normal = (normal_bg << 4) | normal_fg;
	inverse = (normal_fg << 4) | normal_bg;

	// make screen all one attribute
	FillConsoleOutputAttribute(hStdout, normal, ConsoleInfo.dwSize.X *
		ConsoleInfo.dwSize.Y, screen_start, &Result);
	
	while(!done) {

		switch (InputRecord.EventType) {
		case MOUSE_EVENT:
			switch(InputRecord.Event.MouseEvent.dwEventFlags) {
			case 0: // only copy if the mouse button has been released
				if(!InputRecord.Event.MouseEvent.dwButtonState) {
					doClip(start_coords, end_coords);
					done = TRUE;
				}
				break;
				
			case MOUSE_MOVED:
				end_coords = InputRecord.Event.MouseEvent.dwMousePosition;
				move_mouse(start_coords, end_coords);					
				break;
			}
			break;
		// If we are changing focus, we don't want to highlight anything
		// (Paul Brannan 9/2/98)
		case FOCUS_EVENT:
			return;			
		}
		
		WaitForSingleObject(hConsole, INFINITE);
		if (!ReadConsoleInput(hConsole, &InputRecord, 1, &Result))
			done = TRUE;
		
	}

	doMouse_cleanup();
}

void TMouse::scrollMouse() {
	doMouse();
}
