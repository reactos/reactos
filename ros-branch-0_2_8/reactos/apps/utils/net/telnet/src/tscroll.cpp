///////////////////////////////////////////////////////////////////////////////
//Telnet Win32 : an ANSI telnet client.
//Copyright (C) 1998-2000 Paul Brannan
//Copyright (C) 1998 I.Ioannou
//Copyright (C) 1997 Brad Johnson
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

///////////////////////////////////////////////////////////////////////////////
//
// Module:		tscroll.cpp
//
// Contents:	Telnet Handler
//
// Product:		telnet
//
// Revisions: Dec. 5, 1998	Paul Brannan <pbranna@clemson.edu>
//            June 15, 1998 Paul Brannan
//
//            This is code originally from tnclass.cpp and ansiprsr.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <string.h>
#include <ctype.h>
#include "tscroll.h"
#include "tncon.h"
#include "tconsole.h"
#include "tnconfig.h"

enum {
	HEX,
	DUMP,
	DUMPB,
	TEXTB,
};

int DummyStripBuffer(char *start, char *end, int width) {return 0;}

TScroller::TScroller(TMouse &M, int size) : Mouse(M) {
	iScrollSize = size;
	pcScrollData = new char[iScrollSize];
	iScrollEnd = 0;
	iPastEnd = 0;
	memset(pcScrollData, ' ', iScrollSize);

	if(stricmp(ini.get_scroll_mode(), "hex") == 0) iDisplay = HEX;
	else if(stricmp(ini.get_scroll_mode(), "dump") == 0) iDisplay = DUMP;
	else if(stricmp(ini.get_scroll_mode(), "dumpb") == 0) iDisplay = DUMPB;
	else if(stricmp(ini.get_scroll_mode(), "text") == 0) iDisplay = TEXTB;
	else iDisplay = DUMP;

	strip = &DummyStripBuffer;
}

TScroller::~TScroller() {
	delete[] pcScrollData;
}

void TScroller::init(stripfunc *s) {
	strip = s;
}

// Fixed update of circular buffer (Paul Brannan 12/4/98)
// Note: iScrollEnd is one character beyond the end
void TScroller::update(const char *pszHead, const char *pszTail) {
	if ((iScrollEnd)+(pszTail-pszHead) < iScrollSize) {
		memcpy(&pcScrollData[iScrollEnd], pszHead, pszTail-pszHead);
	} else if (pszTail-pszHead > iScrollSize) {
		memcpy(pcScrollData, pszTail-iScrollSize, iScrollSize);
		iScrollEnd = 0;
	} else {
		memcpy(&pcScrollData[iScrollEnd], pszHead, iScrollSize-iScrollEnd);
		memcpy(&pcScrollData[0], pszHead + (iScrollSize-iScrollEnd),
			pszTail-pszHead-(iScrollSize-iScrollEnd));
	}

	// This could probably be optimized better, but it's probably not worth it
	int temp = iScrollEnd;
	iScrollEnd = ((iScrollEnd)+(pszTail-pszHead))%iScrollSize;
	if(iScrollEnd < temp) iPastEnd = 1;
}

// Perhaps this should be moved to Tconsole.cpp? (Paul Brannan 6/12/98)
static BOOL WriteConsoleOutputCharAndAttribute(
										HANDLE  hConsoleOutput,	// handle of a console screen buffer
										CHAR * lpWriteBuffer,
										WORD wAttrib,
										SHORT sX,
										SHORT sY ){
	// we ought to allocate memory before writing to an address (PB 5/12/98)
	DWORD cWritten;
	const LPDWORD lpcWritten = &cWritten;
	
	DWORD  cWriteCells = strlen(lpWriteBuffer);
	COORD  coordWrite = {sX,sY};
	LPWORD lpwAttribute = new WORD[cWriteCells];
	for (unsigned int i = 0; i < cWriteCells; i++)
		lpwAttribute[i] = wAttrib;
	WriteConsoleOutputAttribute(
		hConsoleOutput,			// handle of a console screen buffer
		lpwAttribute,			// address of buffer to write attributes from
		cWriteCells,			// number of character cells to write to
		coordWrite,				// coordinates of first cell to write to
		lpcWritten				// address of number of cells written to
		);
	WriteConsoleOutputCharacter(
		hConsoleOutput,			// handle of a console screen buffer
		lpWriteBuffer,			// address of buffer to write characters from
		cWriteCells,			// number of character cells to write to
		coordWrite,				// coordinates of first cell to write to
		lpcWritten				// address of number of cells written to
		);
	delete [] lpwAttribute;
	return 1;
}

static void hexify(int x, char *str, int len) {
	for(int j = len - 1; j >= 0; j--) {
		str[j] = x % 16;
		if(str[j] > 9) str[j] += 'A' - 10;
		else str[j] += '0';
		x /= 16;
	}
}

static int setmaxlines(int iDisplay, int iScrollSize, int strippedlines,
				int con_width) {
	switch(iDisplay) {
		case HEX: return(iScrollSize / 16); break;
		case DUMP: 
		case DUMPB: return(iScrollSize / con_width); break;
		case TEXTB: return(strippedlines); break;
	}
	return 0;
}

static void setstatusline(char *szStatusLine, int len, int iDisplay) {
	memset(szStatusLine, ' ', len);
	memcpy(&szStatusLine[1], "Scrollback Mode", 15);
	switch(iDisplay) {
	case HEX: memcpy(&szStatusLine[len / 2 - 1], "HEX", 3); break;
	case DUMP: memcpy(&szStatusLine[len / 2 - 2], "DUMP", 4); break;
	case DUMPB: memcpy(&szStatusLine[len / 2 - 5], "BINARY DUMP", 11); break;
	case TEXTB: memcpy(&szStatusLine[len / 2 - 2], "TEXT", 4); break;
	}
	memcpy(&szStatusLine[len - 6], "READY", 5);
	szStatusLine[len] = 0;
}

void TScroller::ScrollBack(){
	char p;
	int r,c;

	// define colors (Paul Brannan 7/5/98)
	int normal = (ini.get_scroll_bg() << 4) | ini.get_scroll_fg();
	// int inverse = (ini.get_scroll_fg() << 4) | ini.get_scroll_bg();
	int status = (ini.get_status_bg() << 4) | ini.get_status_fg();

	CHAR_INFO* chiBuffer;
	chiBuffer = newBuffer();
	saveScreen(chiBuffer);

	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
	GetConsoleScreenBufferInfo(hStdout, &ConsoleInfo);

	// Update iScrollBegin -- necessary in case the buffer isn't full yet
	long iScrollBegin, iScrollLast;
	if(iPastEnd == 0) {
		iScrollBegin = 0;
		iScrollLast = iScrollEnd - 1;
	} else {
		iScrollBegin = iScrollEnd;
		iScrollLast = iScrollSize - 1;
	}

	// Create buffer with ANSI codes stripped
	// Fixed this to work properly with a circular buffer (PB 12/4/98)
	char *stripped = new char[iScrollSize];
	memcpy(stripped, pcScrollData + iScrollBegin, iScrollSize - 
		iScrollBegin);
	if(iScrollBegin != 0) memcpy(stripped + (iScrollSize - iScrollBegin),
		pcScrollData, iScrollBegin - 1);
	int strippedlines = (*strip)(stripped, stripped + iScrollLast,
		CON_COLS);

	// Calculate the last line of the scroll buffer (Paul Brannan 12/4/98)
	int maxlines = setmaxlines(iDisplay, iScrollLast + 1, strippedlines,
		CON_COLS);

	// init scroll position
	int current = maxlines - CON_HEIGHT + 1;
	if(current < 0) current = 0;
	
	// paint border and info
    // paint last two lines black on white
    char * szStatusLine;
    szStatusLine = new char[CON_WIDTH+2];
	setstatusline(szStatusLine, CON_COLS, iDisplay);
    WriteConsoleOutputCharAndAttribute(hStdout, szStatusLine, status,
		CON_LEFT, CON_BOTTOM);
	
	// loop while not done
	BOOL done = FALSE;
	while (!done){
		switch (iDisplay){
		case HEX:
			memset(szStatusLine, ' ', CON_COLS);
			szStatusLine[8] = ':';
			szStatusLine[34] = '-';
            for (r = 0; r < CON_HEIGHT; r++) {
				hexify((r + current) * 16, &szStatusLine[2], 6);
				for (c = 0; c < 16; c++){
					if (c+(16*(r+current)) >= iScrollLast)
						p = 0;
					else
						p = pcScrollData[(c+16*(r+current) + iScrollBegin) %
							iScrollSize];
					hexify((char)p, &szStatusLine[11 + 3*c], 2);
					if (!iscntrl(p)) {
						szStatusLine[60 + c] = (char)p;
					} else {
						szStatusLine[60 + c] = '.';
					}
				}
				for(int j = 0; j < 16; j++) {
				}
				szStatusLine[CON_COLS] = '\0';
				WriteConsoleOutputCharAndAttribute(hStdout, szStatusLine,
					normal, CON_LEFT, r+CON_TOP);
            }
            break;
		case DUMP:
            for (r = 0; r < CON_HEIGHT; r++) {
				for (c = 0; c <= CON_WIDTH; c++) {
					if (c+((CON_COLS)*(r+current)) >= iScrollLast) p = ' ';
					else p = pcScrollData[(c+((CON_COLS)*(r+current))
						+ iScrollBegin)	% iScrollSize];
					if (!iscntrl(p))
						szStatusLine[c] = p;
					else
						szStatusLine[c] = '.';
				}
				szStatusLine[c] = '\0';
				WriteConsoleOutputCharAndAttribute(hStdout, szStatusLine,
					normal, CON_LEFT, r+CON_TOP);
            }
            break;
		case DUMPB:
			for (r = 0; r < CON_HEIGHT; r++) {
				for (c = 0; c <= CON_WIDTH; c++) {
					if (c+((CON_COLS)*(r+current)) >= iScrollLast) p = ' ';
					else p = pcScrollData[  (c+((CON_COLS)*(r+current))
						+ iScrollBegin) % iScrollSize];
					if (p != 0)
						szStatusLine[c] = p;
					else
						szStatusLine[c] = ' ';
				}
				szStatusLine[c] = '\0';
				WriteConsoleOutputCharAndAttribute(hStdout, szStatusLine,
					normal, CON_LEFT, r+CON_TOP);
            }
            break;
		case TEXTB: {
			int ch, lines, x;
			// Find the starting position
			for(ch = 0, lines = 0, x = 1; ch < iScrollSize &&
				lines < current; ch++, x++) {
				
				if(stripped[ch] == '\n') lines++;
				if(stripped[ch] == '\r') x = 1;
			}

			for (r = 0; r < CON_HEIGHT; r++) {
				memset(szStatusLine, ' ', CON_COLS);
				for(c = 0; c <= CON_WIDTH; c++) {
					int done = FALSE;
					if (ch >= iScrollSize) p = ' ';
					else p = stripped[ch];
					switch(p) {
					case 10: done = TRUE; break;
					case 13: c = 0; break;
					default: szStatusLine[c] = p;
					}
					ch++;
					if(done) break;
				}
				szStatusLine[CON_COLS] = '\0';
				WriteConsoleOutputCharAndAttribute(hStdout, szStatusLine,
					normal, CON_LEFT, r+CON_TOP);
			}
					}
            break;
		}

		setstatusline(szStatusLine, CON_COLS, iDisplay);
		WriteConsoleOutputCharAndAttribute(hStdout, szStatusLine, status,
			CON_LEFT, CON_BOTTOM);

		// paint scroll back data
		// get key input
		switch(scrollkeys()){
		case VK_ESCAPE:
			done = TRUE;
			break;
		case VK_PRIOR:
			if ( current > CON_HEIGHT)
				current-= CON_HEIGHT;
			else
				current = 0;
			break;
		case VK_NEXT:
			if ( current < maxlines - 2*CON_HEIGHT + 2)
				current += CON_HEIGHT;
			else
				current = maxlines - CON_HEIGHT + 1;
			break;
		case VK_DOWN:
			if (current <= maxlines - CON_HEIGHT) current++;
			break;
		case VK_UP:
			if ( current > 0) current--;
			break;
		case VK_TAB:
			iDisplay = (iDisplay+1)%4;
			maxlines = setmaxlines(iDisplay, iScrollLast + 1, strippedlines,
				CON_COLS);
			if(current > maxlines) current = maxlines - 1;
			if(current < 0) current = 0;
			break;
		case VK_END:
			current = maxlines - CON_HEIGHT + 1;
			if(current < 0) current = 0;
			break;
		case VK_HOME:
			current = 0;
			break;
		case SC_MOUSE:
			Mouse.scrollMouse();
			break;
		}
	}
	
	// Clean up
	restoreScreen(chiBuffer);
	delete[] szStatusLine;
	delete[] chiBuffer;
	delete[] stripped;
}
