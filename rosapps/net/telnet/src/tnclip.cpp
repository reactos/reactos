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

// TnClip.cpp
// A simple class for handling clipboard functions
// Written by Paul Brannan <pbranna@clemson.edu>
// Last modified 7/12/98

#include <string.h>
#include "tnclip.h"

Tnclip::Tnclip(HWND W, TNetwork &RefNetwork): Network(RefNetwork) {
	Window = W;
}

Tnclip::~Tnclip() {
}

void Tnclip::Copy(HGLOBAL clipboard_data) {
	if(!OpenClipboard(Window)) return;
	if(!EmptyClipboard()) return;
						
	SetClipboardData(CF_TEXT, clipboard_data);
	CloseClipboard();
}

void Tnclip::Paste() {
	if(!OpenClipboard(Window)) return;

	HANDLE clipboard_data = GetClipboardData(CF_TEXT);
	LPVOID clipboard_ptr = GlobalLock(clipboard_data);
	DWORD size = strlen((const char *)clipboard_data);
	Network.WriteString((const char *)clipboard_ptr, size);
	GlobalUnlock(clipboard_data);
	
	CloseClipboard();
}