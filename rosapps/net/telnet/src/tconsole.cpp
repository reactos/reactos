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
// Module:		tconsole.cpp
//
// Contents:	screen functions
//
// Product:		telnet
//
//
// Revisions: Mar. 29, 2000 pbranna@clemson (Paul Brannan)
//            June 15, 1998 pbranna@clemson.edu
//            May 16, 1998	pbranna@clemson.edu
//            05. Sep.1997  roryt@hol.gr (I.Ioannou)
//            11.May,1997   roryt@hol.gr
//            06.April,1997 roryt@hol.gr
//            30.M„rz.1997  Titus_Boxberg@public.uni-hamburg.de
//		      5.Dec.1996    jbj@nounname.com
//            Version 2.0
//            02.Apr.1995	igor.milavec@uni-lj.si
//					  Original code
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "tconsole.h"

// argsused doesn't work on MSVC++
#ifdef __BORLANDC__
#pragma argsused
#endif

TConsole::TConsole(HANDLE h) {
	hConsole = h;

	GetConsoleScreenBufferInfo(hConsole, &ConsoleInfo);

	// Start with correct colors
	int fg = ini.get_normal_fg();
	int bg = ini.get_normal_bg();
	if(fg == -1)
		fg = defaultfg = origfg = ConsoleInfo.wAttributes & 0xF;
	else
		defaultfg = origfg = fg;
	if(bg == -1)
		bg = defaultbg = origbg = (ConsoleInfo.wAttributes >> 4) & 0xF;
	else
		defaultbg = origbg = bg;
	wAttributes = fg | (bg << 4);
	reverse = blink = underline = false;
	SetConsoleTextAttribute(hConsole, wAttributes);

	insert_mode = 0;
	
	// Set the screen size
	SetWindowSize(ini.get_term_width(), ini.get_term_height());

	iScrollStart = -1;
	iScrollEnd = -1;
}

TConsole::~TConsole() {
	wAttributes = origfg | (origbg << 4);
	SetCursorPosition(0, CON_HEIGHT);
	SetConsoleTextAttribute(hConsole, wAttributes);
	WriteCtrlChar('\x0a');
}

// Paul Brannan 8/2/98
void TConsole::SetWindowSize(int width, int height) {
	SMALL_RECT sr = {
		CON_LEFT,
		CON_TOP,
		(width == -1) ? CON_RIGHT : CON_LEFT + width - 1,
		(height == -1) ? CON_BOTTOM : CON_TOP + height - 1
	};
	ConsoleInfo.dwSize.X = width;
	if(ConsoleInfo.dwSize.Y < height) ConsoleInfo.dwSize.Y = height;
	SetConsoleScreenBufferSize(hConsole, ConsoleInfo.dwSize);
	SetConsoleWindowInfo(hConsole, TRUE, &sr);
	SetConsoleScreenBufferSize(hConsole, ConsoleInfo.dwSize);
	sync();
}

// Paul Brannan 5/15/98
void TConsole::sync() {
	GetConsoleScreenBufferInfo(hConsole, &ConsoleInfo);
}

void TConsole::HighVideo() {
	wAttributes = wAttributes | (unsigned char) 8;
}

void TConsole::LowVideo() {
	wAttributes = wAttributes & (unsigned char) (0xff-8);
}

void TConsole::Normal() {
	// I.Ioannou 11 May 1997
	// Color 7 is correct on some systems (for example Linux)
	// but not with others (for example SCO)
	// we must preserve the colors :
	// 06/04/98 thanks to Paul a .ini parameter from now on
	
	BlinkOff();
	UnderlineOff();
	if(ini.get_preserve_colors()) {
		ReverseOff();
		LowVideo();
	} else {
		fg = defaultfg;
		bg = defaultbg;
		wAttributes = (unsigned char)fg | (bg << 4);
		reverse = false;
	}
}

void TConsole::SetForeground(unsigned char wAttrib) {
	if(reverse) bg = wAttrib; else fg = wAttrib;
	wAttributes = (wAttributes & (unsigned char)0x88) | 
		(unsigned char)fg | (bg << 4);
}

void TConsole::SetBackground(unsigned char wAttrib) {
	if(reverse) fg = wAttrib; else bg = wAttrib;
	wAttributes = (wAttributes & (unsigned char)0x88) | 
		(unsigned char)fg | (bg << 4);
}

// As far as I can tell, there's no such thing as blink in Windows Console.
// I tried using some inline asm to turn off high-intensity backgrounds,
// but I got a BSOD.  Perhaps there is an undocumented function?
// (Paul Brannan 6/27/98)
void TConsole::BlinkOn() {
	blink = 1;
	if(underline) {
		UlBlinkOn();
	} else {
		if(ini.get_blink_bg() != -1) {
			wAttributes &= 0x8f;					// turn off bg
			wAttributes |= ini.get_blink_bg() << 4;
		}
		if(ini.get_blink_fg() != -1) {
			wAttributes &= 0xf8;					// turn off fg
			wAttributes |= ini.get_blink_fg();
		}
		if(ini.get_blink_bg() == -1 && ini.get_blink_fg() == -1)
			wAttributes |= 0x80;
	}
}

// Added by I.Ioannou 06 April, 1997
void TConsole::BlinkOff() {
	blink = 0;
	if(underline) {
		UlBlinkOff();
	} else {
		if(ini.get_blink_bg() != -1) {
			wAttributes &= 0x8f;					// turn off bg
			wAttributes |= defaultbg << 4;
		}
		if(ini.get_blink_fg() != -1) {
			wAttributes &= 0xf8;					// turn off fg
			wAttributes |= defaultfg;
		}
		if(ini.get_blink_bg() == -1 && ini.get_blink_fg() == -1)
			wAttributes &= 0x7f;
	}
}

// Paul Brannan 6/27/98
void TConsole::UnderlineOn() {
	underline = 1;
	if(blink) {
		UlBlinkOn();
	} else {
		if(ini.get_underline_bg() != -1) {
			wAttributes &= 0x8f;					// turn off bg
			wAttributes |= ini.get_underline_bg() << 4;
		}
		if(ini.get_underline_fg() != -1) {
			wAttributes &= 0xf8;					// turn off fg
			wAttributes |= ini.get_underline_fg();
		}
		if(ini.get_underline_bg() == -1 && ini.get_underline_fg() == -1)
			wAttributes |= 0x80;
	}
}

// Paul Brannan 6/27/98
void TConsole::UnderlineOff() {
	underline = 0;
	if(blink) {
		UlBlinkOff();
	} else {
		if(ini.get_blink_bg() != -1) {
			wAttributes &= 0x8f;					// turn off bg
			wAttributes |= defaultbg << 4;
		}
		if(ini.get_blink_fg() != -1) {
			wAttributes &= 0xf8;					// turn off fg
			wAttributes |= defaultfg;
		}
		if(ini.get_blink_bg() == -1 && ini.get_blink_fg() == -1)
			wAttributes &= 0x7f;
	}
}

// Paul Brannan 6/27/98
void TConsole::UlBlinkOn() {
	if(ini.get_ulblink_bg() != -1) {
		wAttributes &= 0x8f;					// turn off bg
		wAttributes |= ini.get_ulblink_bg() << 4;
	}
	if(ini.get_ulblink_fg() != -1) {
		wAttributes &= 0xf8;					// turn off fg
		wAttributes |= ini.get_ulblink_fg();
	}
	if(ini.get_ulblink_bg() == -1 && ini.get_ulblink_fg() == -1)
		wAttributes |= 0x80;
}

// Paul Brannan 6/27/98
void TConsole::UlBlinkOff() {
	if(blink) {
		BlinkOn();
	} else if(underline) {
		UnderlineOn();
	} else {
		Normal();
	}
}

// Paul Brannan 6/26/98
void TConsole::Lightbg() {
	WORD *pAttributes = new WORD[CON_COLS];
	DWORD Result;

	// Paul Brannan 8/5/98
	// Correction: processing more than one line at a time causes a segfault
	// if the screen width != 80
	for(int i = CON_TOP; i <= CON_BOTTOM; i++) {
		COORD Coord = {CON_LEFT, i};

		ReadConsoleOutputAttribute(hConsole, pAttributes, (DWORD)(CON_COLS),
			Coord, &Result);
	
		for(DWORD j = 0; j < Result; j++) pAttributes[j] |= 0x80;

		WriteConsoleOutputAttribute(hConsole, pAttributes, Result, Coord,
			&Result);
	}

	delete[] pAttributes; // clean up

	wAttributes |= (unsigned char)0x80;
	bg |= 8;
}

// Paul Brannan 6/26/98
void TConsole::Darkbg() {
	WORD *pAttributes = new WORD[CON_COLS];
	DWORD Result;

	// Paul Brannan 8/5/98
	// Correction: processing more than one line at a time causes a segfault
	// if the screen width != 80
	for(int i = CON_TOP; i <= CON_BOTTOM; i++) {
		COORD Coord = {CON_LEFT, i};

		ReadConsoleOutputAttribute(hConsole, pAttributes, (DWORD)(CON_COLS),
			Coord, &Result);
	
		for(DWORD j = 0; j < Result; j++) pAttributes[j] &= 0x7f;

		WriteConsoleOutputAttribute(hConsole, pAttributes, Result, Coord,
			&Result);
	}

	delete[] pAttributes; // clean up


	wAttributes &= (unsigned char)0x7f;
	bg &= 7;
}

// Added by I.Ioannou 11.May,1997
void TConsole::ReverseOn() {
	if (!reverse) {
		reverse = true;

		// atl  : forground attributes without the intensity
		// ath  : backgound attributes without the blink
		// bl   : the blink state
		// ints : the intensity
		unsigned char atl   = wAttributes & (unsigned char) 0x07;
		unsigned char ath   = wAttributes & (unsigned char) 0x70;
		unsigned char bl    = wAttributes & (unsigned char) 0x80;
		unsigned char ints  = wAttributes & (unsigned char) 0x08;
		wAttributes = bl | (atl << 4) | ints | (ath >> 4);
	}
}

// Added by I.Ioannou 11.May,1997
void TConsole::ReverseOff() {
	if (reverse) {
		reverse = false;
		wAttributes = fg | (bg << 4);
	}
}

unsigned long TConsole::WriteText(const char *pszString, unsigned long cbString) {
	DWORD Result;

	if(insert_mode) {
		InsertCharacter(cbString);
	}

	WriteConsoleOutputCharacter(hConsole, (char *)pszString, cbString,
		ConsoleInfo.dwCursorPosition, &Result);
	FillConsoleOutputAttribute(hConsole, wAttributes, cbString,
		ConsoleInfo.dwCursorPosition, &Result);
	return Result;
}

// Formerly ConWriteString (Paul Brannan 6/28/98)
unsigned long TConsole::WriteStringFast(const char* pszString, unsigned long cbString) {
	DWORD Result;

	SetConsoleTextAttribute(hConsole, wAttributes);

	//check to see if the line is longer than the display
	if (!getLineWrap() && ((unsigned)CON_CUR_X + cbString) >= (unsigned)CON_COLS) {
		// Take care of the last line last colum exception...
		// The display scrolls up if you use the normal char out
		//   function even if you only write to the last place
		//   on the line. :-(
		if ((unsigned)CON_CUR_Y >= (unsigned)CON_HEIGHT) {
			unsigned long iFakeResult = cbString;
			cbString = CON_COLS - CON_CUR_X - 1;

			// FIX ME !!! This will avoid the exception when cbString
			// is <= 0 but still doesn't work :-(
			if (cbString > 0)
				WriteConsole(hConsole, pszString, cbString, &Result, 0);

			COORD dwBufferCoord;
			dwBufferCoord.X = 0;
			dwBufferCoord.Y = 0;

			CHAR_INFO ciBuffer;
			ciBuffer.Char.AsciiChar = *(pszString+cbString);
			ciBuffer.Attributes = wAttributes;
			SMALL_RECT srWriteRegion;
			srWriteRegion.Top =		(SHORT) CON_BOTTOM;
			srWriteRegion.Bottom =	(SHORT) CON_BOTTOM;
			srWriteRegion.Left =	(SHORT) CON_RIGHT;
			srWriteRegion.Right =	(SHORT) CON_RIGHT;

			COORD bufSize = {1,1};

			WriteConsoleOutput(hConsole, &ciBuffer, bufSize,
				dwBufferCoord, &srWriteRegion);

			// We need to update the ConsoleInfo struct now (Paul Brannan 5/9/98)
			ConsoleInfo.dwCursorPosition.X = CON_RIGHT;

			return iFakeResult; // Skip the chars that did not fit
		}
		// just write the line up to the end
		else {
			int iFakeResult = cbString;
			cbString = CON_COLS - CON_CUR_X;

			if(cbString > 0) {
				WriteConsole(hConsole, pszString, cbString, &Result, 0);

				// We need to update the ConsoleInfo struct now (Paul Brannan 5/9/98)
				ConsoleInfo.dwCursorPosition.X += (unsigned short)Result;
			}

			return iFakeResult; // Skip the chars that did not fit
		}
	} else {
		// If custom scrolling is enabled we must take care of it
		if(iScrollStart != -1 || iScrollEnd != -1) {
			return WriteString(pszString, cbString);
		}

		// Apparently VT100 terminals have an invisible "81st" column that
		// can hold a cursor until another character is printed.  I'm not sure
		// exactly how to handle this, but here's a hack (Paul Brannan 5/28/98)
		if(ini.get_vt100_mode() && cbString + (unsigned)CON_CUR_X == (unsigned)CON_COLS) {

			cbString--;
			if(cbString >= 0) WriteConsole(hConsole, pszString, cbString, &Result, 0);

			COORD dwBufferCoord;
			dwBufferCoord.X = 0;
			dwBufferCoord.Y = 0;

			CHAR_INFO ciBuffer;
			ciBuffer.Char.AsciiChar = *(pszString+cbString);
			ciBuffer.Attributes = wAttributes;
			SMALL_RECT srWriteRegion;
			srWriteRegion.Top =    (SHORT) ConsoleInfo.dwCursorPosition.Y;
			srWriteRegion.Bottom = (SHORT) ConsoleInfo.dwCursorPosition.Y;
			srWriteRegion.Left =   (SHORT) CON_RIGHT;
			srWriteRegion.Right =  (SHORT) CON_RIGHT;

			COORD bufSize = {1,1};

			WriteConsoleOutput(hConsole, &ciBuffer, bufSize,
				dwBufferCoord, &srWriteRegion);

			// Update the ConsoleInfo struct
			ConsoleInfo.dwCursorPosition.X = CON_RIGHT + 1;

			return Result + 1;
		}

		// normal line will wrap normally or not to the end of buffer
		WriteConsole(hConsole, pszString, cbString, &Result, 0);

		// We need to update the ConsoleInfo struct now (Paul Brannan 5/9/98)
		// FIX ME!!! This is susceptible to the same problem as above.
		// (e.g. we write out 160 characters)
		ConsoleInfo.dwCursorPosition.X += (unsigned short)Result;
		while(CON_CUR_X > CON_WIDTH) {
			ConsoleInfo.dwCursorPosition.X -= ConsoleInfo.dwSize.X;
			if((unsigned)CON_CUR_Y < (unsigned)CON_HEIGHT) {
				ConsoleInfo.dwCursorPosition.Y++;
			} else {
				// If we aren't at the bottom of the window, then we need to
				// scroll down (Paul Brannan 4/14/2000)
				if(ConsoleInfo.srWindow.Bottom < ConsoleInfo.dwSize.Y - 1) {
					ConsoleInfo.srWindow.Top++;
					ConsoleInfo.srWindow.Bottom++;
					ConsoleInfo.dwCursorPosition.Y++;
					SetConsoleWindowInfo(hConsole, TRUE, &ConsoleInfo.srWindow);
				}
			}
		}
	}

	return Result;

}

unsigned long TConsole::WriteString(const char* pszString, unsigned long cbString) {
	DWORD Result = 0;
	
	SetConsoleTextAttribute(hConsole, wAttributes);

	//check to see if the line is longer than the display
	if (!getLineWrap()){
		unsigned long iFakeResult = cbString;
		if((CON_CUR_X + cbString) >= (unsigned int)CON_COLS)
			cbString = CON_COLS - CON_CUR_X;
		if(cbString > 0)
			Result = WriteText(pszString, cbString);

		// We need to update the ConsoleInfo struct now (Paul Brannan 5/9/98)
		ConsoleInfo.dwCursorPosition.X += (unsigned short)Result;
		SetConsoleCursorPosition(hConsole, ConsoleInfo.dwCursorPosition);

		return iFakeResult; // Skip the chars that did not fit
	} else {
		// Write up to the end of the line
		unsigned long temp = cbString;
		if((CON_CUR_X + temp) > (unsigned int)CON_COLS) {
			temp = CON_COLS - CON_CUR_X;
		} else {
			Result = WriteText(pszString, temp);
			ConsoleInfo.dwCursorPosition.X += (unsigned short)Result;
			SetConsoleCursorPosition(hConsole, ConsoleInfo.dwCursorPosition);
			return Result;
		}
		if(temp > 0) {
			Result = WriteText(pszString, temp);
			ConsoleInfo.dwCursorPosition.X += (unsigned short)Result;
			temp = (unsigned short)Result;
		}

		// keep writing lines until we get to less than 80 chars left
		while((temp + (unsigned int)CON_COLS) < cbString) {
			index(); // LF
			ConsoleInfo.dwCursorPosition.X = 0; // CR
			Result = WriteText(&pszString[temp], CON_COLS);
			temp += (unsigned short)Result;
		}

		// write out the last bit
		if(temp < cbString) {
			index();
			ConsoleInfo.dwCursorPosition.X = 0;
			Result = WriteText(&pszString[temp], cbString - temp);
			temp += (unsigned short)Result;
		}

		// Apparently VT100 terminals have an invisible "81st" column that
		// can hold a cursor until another character is printed.  I'm not sure
		// exactly how to handle this, but here's a hack (Paul Brannan 5/28/98)
		if(!ini.get_vt100_mode() && cbString + (unsigned)ConsoleInfo.dwCursorPosition.X
			== (unsigned int)CON_COLS) {
			index();
			ConsoleInfo.dwCursorPosition.X = 0;
		}

		SetConsoleCursorPosition(hConsole, ConsoleInfo.dwCursorPosition);

		return temp;
	}

	return 0;
}

// This is for multi-character control strings (Paul Brannan 6/26/98)
unsigned long TConsole::WriteCtrlString(const char *pszString, unsigned long cbString) {
	unsigned long total = 0;
	while(total < cbString) {
		unsigned long Result = WriteCtrlChar(*(pszString + total));
		if(Result == 0) {
			Result = WriteStringFast(pszString + total, 1);
			if(Result == 0) return total;
		}
		total += Result;
	}
	return total;
}

// This is for printing single control characters
// WriteCtrlString uses this (Paul Brannan 6/26/98)
unsigned long TConsole::WriteCtrlChar(char c) {
	// The console does not handel the CR/LF chars as we might expect
	// when using color. The attributes are not set correctly, so we
	// must interpret them manualy to preserve the colors on the screen.
	
	unsigned long Result = 0; // just in case (Paul Brannan 6/26/98)
	switch (c) {
    case '\x09': // horizontal tab
		SetCursorPosition((((CON_CUR_X/8)+1)*8), CON_CUR_Y);
		Result = 1;
		break;
		
    case '\x0a': // line feed
		index();
		Result = 1;
		break;
    case '\x0d': // carrage return
		SetCursorPosition(CON_LEFT, CON_CUR_Y); // move to beginning of line
		Result = 1;
		break;
	case '\b': // backspace
		// Added support for backspace so the cursor position can be changed
		// (Paul Brannan 5/25/98)
		MoveCursorPosition(-1, 0);
		Result = 1;
    default :    // else just write it like normal
		break;
	}
	
	return Result;
}

void TConsole::index() {
	// if on the last line scroll up
	// This must work with scrolling (Paul Brannan 5/13/98)
	if(iScrollEnd != -1 && (signed)CON_CUR_Y >= iScrollEnd) {
		ScrollDown(iScrollStart, iScrollEnd, -1);
	} else if ((iScrollEnd == -1 && (signed)CON_CUR_Y >= (signed)CON_HEIGHT)) {
		DWORD Result;
		WriteConsole(hConsole, "\n", 1, &Result, NULL);
		
		// If we aren't at the bottom of the buffer, then we need to
		// scroll down (Paul Brannan 4/14/2000)
		if(iScrollEnd == -1 && ConsoleInfo.srWindow.Bottom < ConsoleInfo.dwSize.Y - 1) {
			ConsoleInfo.srWindow.Top++;
			ConsoleInfo.srWindow.Bottom++;
			ConsoleInfo.dwCursorPosition.Y++;
			// SetConsoleWindowInfo(hConsole, TRUE, &ConsoleInfo.srWindow);
		} else {
			ClearLine();
		}
	} else {     // else move cursor down to the next line
		SetCursorPosition(CON_CUR_X, CON_CUR_Y + 1);
	}
}

void TConsole::reverse_index() {
	// if on the top line scroll down
	// This must work with scrolling (Paul Brannan 5/13/98)
	// We should be comparing against iScrollStart, not iScrollEnd (PB 12/2/98)
	if (iScrollStart == -1 && (signed)CON_CUR_Y <= 0) {
		ScrollDown(iScrollStart, -1, 1);
	} else if (iScrollStart != -1 && (signed)CON_CUR_Y <= iScrollStart) {
			ScrollDown(iScrollStart, iScrollEnd, 1);
	} else       // else move cursor up to the previous line
		SetCursorPosition(CON_CUR_X,CON_CUR_Y - 1);
}

void TConsole::ScrollDown( int iStartRow , int iEndRow, int bUp ){
	CHAR_INFO ciChar;
	SMALL_RECT srScrollWindow;
	
	// Correction from I.Ioannou 11 May 1997
	// check the scroll region
	if (iStartRow < iScrollStart) iStartRow = iScrollStart;
	
	// Correction from I.Ioannou 11 May 1997
	// this will make Top the CON_TOP
	if ( iStartRow == -1) iStartRow = 0;

	// Correction from I.Ioannou 18 Aug 97
	if ( iEndRow == -1)	{
		if ( iScrollEnd == -1 )
			iEndRow = CON_HEIGHT;
		else
			iEndRow = ((CON_HEIGHT <= iScrollEnd) ? CON_HEIGHT : iScrollEnd);
	}
	//

	if ( iStartRow > CON_HEIGHT) iStartRow = CON_HEIGHT;
	if ( iEndRow > CON_HEIGHT) iEndRow = CON_HEIGHT;
	
	srScrollWindow.Left =           (CON_LEFT);
	srScrollWindow.Right =  (SHORT) (CON_RIGHT);
	srScrollWindow.Top =    (SHORT) (CON_TOP + iStartRow );
	srScrollWindow.Bottom = (SHORT) (CON_TOP + iEndRow); // don't subtract 1 (PB 5/28)

	ciChar.Char.AsciiChar = ' ';           // fill with spaces
	ciChar.Attributes = wAttributes;       // fill with current attrib
	
	// This should speed things up (Paul Brannan 9/2/98)
	COORD dwDestOrg = {srScrollWindow.Left, srScrollWindow.Top + bUp};
	
	// Note that iEndRow and iStartRow had better not be equal to -1 at this
	// point.  There are four cases to consider for out of bounds.  Two of
	// these cause the scroll window to be cleared; the others cause the
	// scroll region to be modified.  (Paul Brannan 12/3/98)
	if(dwDestOrg.Y > CON_TOP + iEndRow) {
		// We are scrolling past the end of the scroll region, so just
		// clear the window instead (Paul Brannan 12/3/98)
		ClearWindow(CON_TOP + iStartRow, CON_TOP + iEndRow);
		return;
	} else if(dwDestOrg.Y + (iEndRow-iStartRow+1) < CON_TOP + iStartRow) {
		// We are scrolling past the end of the scroll region, so just
		// clear the window instead (Paul Brannan 12/3/98)
		ClearWindow(CON_TOP + iStartRow, CON_TOP + iEndRow);
		return;
	} else if(dwDestOrg.Y < CON_TOP + iStartRow) {
		// Modify the scroll region (Paul Brannan 12/3/98)
		dwDestOrg.Y = CON_TOP + iStartRow;
		srScrollWindow.Top -= bUp;
	} else 	if(dwDestOrg.Y + (iEndRow-iStartRow+1) > CON_TOP + iEndRow) {
		// Modify the scroll region (Paul Brannan 12/3/98)
		srScrollWindow.Bottom -= bUp;
	}
	
	ScrollConsoleScreenBuffer(hConsole, &srScrollWindow,
		0, dwDestOrg, &ciChar);
}

// This allows us to clear the screen with an arbitrary character
// (Paul Brannan 6/26/98)
void TConsole::ClearScreen(char c) {
	DWORD dwWritten;
	COORD Coord = {CON_LEFT, CON_TOP};
	FillConsoleOutputCharacter(hConsole, c, (DWORD)(CON_COLS)*
		(DWORD)(CON_LINES), Coord, &dwWritten);
	FillConsoleOutputAttribute(hConsole, wAttributes, (DWORD)(CON_COLS)*
		(DWORD)(CON_LINES), Coord, &dwWritten);
}

// Same as clear screen, but only affects the scroll region
void TConsole::ClearWindow(int iStartRow, int iEndRow, char c) {
	DWORD dwWritten;
	COORD Coord = {CON_LEFT, CON_TOP + iStartRow};
	FillConsoleOutputCharacter(hConsole, c, (DWORD)(CON_COLS)*
		(DWORD)(iEndRow-iStartRow+1), Coord, &dwWritten);
	FillConsoleOutputAttribute(hConsole, wAttributes, (DWORD)(CON_COLS)*
		(DWORD)(CON_LINES), Coord, &dwWritten);
}

// Clear from cursor to end of screen
void TConsole::ClearEOScreen(char c)
{
	DWORD dwWritten;
	COORD Coord = {CON_LEFT, CON_TOP + CON_CUR_Y + 1};
	FillConsoleOutputCharacter(hConsole, c, (DWORD)(CON_COLS)*
		(DWORD)(CON_HEIGHT - CON_CUR_Y), Coord, &dwWritten);
	FillConsoleOutputAttribute(hConsole, wAttributes, (DWORD)(CON_COLS)*
		(DWORD)(CON_LINES - CON_CUR_Y), Coord, &dwWritten);
	ClearEOLine();
}

// Clear from beginning of screen to cursor
void TConsole::ClearBOScreen(char c)
{
	DWORD dwWritten;
	COORD Coord = {CON_LEFT, CON_TOP};
	FillConsoleOutputCharacter(hConsole, c, (DWORD)(CON_COLS)*
		(DWORD)(CON_CUR_Y), Coord, &dwWritten);
	FillConsoleOutputAttribute(hConsole, wAttributes, (DWORD)(CON_COLS)*
		(DWORD)(CON_CUR_Y), Coord, &dwWritten);
	ClearBOLine();
}

void TConsole::ClearLine(char c)
{
	DWORD dwWritten;
	COORD Coord = {CON_LEFT, CON_TOP + CON_CUR_Y};
	FillConsoleOutputCharacter(hConsole, c, (DWORD)(CON_COLS),
		Coord, &dwWritten);
	FillConsoleOutputAttribute(hConsole, wAttributes, (DWORD)(CON_COLS),
		Coord, &dwWritten);
	GetConsoleScreenBufferInfo(hConsole, &ConsoleInfo);
}

void TConsole::ClearEOLine(char c)
{
	DWORD dwWritten;
	COORD Coord = {CON_LEFT + CON_CUR_X, CON_TOP + CON_CUR_Y};
	FillConsoleOutputAttribute(hConsole, wAttributes,
		(DWORD)(CON_RIGHT - CON_CUR_X) +1, Coord, &dwWritten);
	FillConsoleOutputCharacter(hConsole, c, (DWORD)(CON_RIGHT - CON_CUR_X) +1,
		Coord, &dwWritten);
	GetConsoleScreenBufferInfo(hConsole, &ConsoleInfo);
}

void TConsole::ClearBOLine(char c)
{
	DWORD dwWritten;
	COORD Coord = {CON_LEFT, CON_TOP + CON_CUR_Y};
	FillConsoleOutputCharacter(hConsole, c, (DWORD)(CON_CUR_X) + 1, Coord,
		&dwWritten);
	FillConsoleOutputAttribute(hConsole, wAttributes, (DWORD)(CON_CUR_X) + 1,
		Coord, &dwWritten);
	GetConsoleScreenBufferInfo(hConsole, &ConsoleInfo);
}


//	Inserts blank lines to the cursor-y-position
//	scrolls down the rest. CURSOR MOVEMENT (to Col#1) ???
void TConsole::InsertLine(int numlines)
{
	COORD		to;
	SMALL_RECT	from;
	SMALL_RECT	clip;
	CHAR_INFO		fill;
	int		acty;
	
	// Rest of screen would be deleted
	if ( (acty = GetCursorY()) >= CON_LINES - numlines ) {
		ClearEOScreen();    // delete rest of screen
		return;
	} /* IF */
	
	//	Else scroll down the part of the screen which is below the
	//	cursor.
	from.Left =		CON_LEFT;
	from.Top =		CON_TOP + (SHORT)acty;
	from.Right =	CON_LEFT + (SHORT)CON_COLS;
	from.Bottom =	CON_TOP + (SHORT)CON_LINES;
	
	clip = from;
	to.X = 0;
	to.Y = (SHORT)(from.Top + numlines);
	
	fill.Char.AsciiChar = ' ';
	fill.Attributes = 7; 		// WHICH ATTRIBUTES TO TAKE FOR BLANK LINE ??
	
	ScrollConsoleScreenBuffer(hConsole, &from, &clip, to, &fill);
} /* InsertLine */

//	Inserts blank characters under the cursor
void TConsole::InsertCharacter(int numchar)
{
	int		actx;
	SMALL_RECT	from;
	SMALL_RECT	clip;
	COORD			to;
	CHAR_INFO		fill;
	
	if ( (actx = GetCursorX()) >= CON_COLS - numchar ) {
		ClearEOLine();
		return;
	} /* IF */
	
	from.Left =		CON_LEFT + (SHORT)actx;
	from.Top =		CON_TOP + (SHORT)GetCursorY();
	from.Right =	CON_LEFT + (SHORT)CON_COLS;
	from.Bottom =	CON_TOP + (SHORT)from.Top;
	
	clip = from;
	to.X = (SHORT)(actx + numchar);
	to.Y = from.Top;
	
	fill.Char.AsciiChar = ' ';
	fill.Attributes = wAttributes; // WHICH ATTRIBUTES TO TAKE FOR BLANK CHAR ??
	
	ScrollConsoleScreenBuffer(hConsole, &from, &clip, to, &fill);
} /* InsertCharacter */

// Deletes characters under the cursor
// Note that there are cases in which all the following lines should shift by
// a character, but we don't handle these.  This could break some
// VT102-applications, but it shouldn't be too much of an issue.
void TConsole::DeleteCharacter(int numchar)
{
	int		actx;
	SMALL_RECT	from;
	SMALL_RECT	clip;
	COORD			to;
	CHAR_INFO		fill;
	
	if ( (actx = GetCursorX()) >= CON_COLS - numchar ) {
		ClearEOLine();
		return;
	} /* IF */
	
	from.Left =		CON_LEFT + (SHORT)actx;
	from.Top =		CON_TOP + (SHORT)GetCursorY();
	from.Right =	CON_LEFT + (SHORT)CON_COLS;
	from.Bottom =	CON_TOP + from.Top;
	
	clip = from;
	to.X = (SHORT)(actx - numchar);
	to.Y = from.Top;
	
	fill.Char.AsciiChar = ' ';
	fill.Attributes = wAttributes; // WHICH ATTRIBUTES TO TAKE FOR BLANK CHAR ??
	
	ScrollConsoleScreenBuffer(hConsole, &from, &clip, to, &fill);
} /* DeleteCharacter */

void TConsole::SetRawCursorPosition(int x, int y) {
	if (x > CON_WIDTH)  x = CON_WIDTH;
	if (x < 0)			x = 0;
	if (y > CON_HEIGHT)	y = CON_HEIGHT;
	if (y < 0)			y = 0;
	COORD Coord = {(short)(CON_LEFT + x), (short)(CON_TOP + y)};
	SetConsoleCursorPosition(hConsole, Coord);
	
	// Update the ConsoleInfo struct (Paul Brannan 5/9/98)
	ConsoleInfo.dwCursorPosition.Y = Coord.Y;
	ConsoleInfo.dwCursorPosition.X = Coord.X;
	
	// bug fix in case we went too far (Paul Brannan 5/25/98)
	if(ConsoleInfo.dwCursorPosition.X < CON_LEFT)
		ConsoleInfo.dwCursorPosition.X = CON_LEFT;
	if(ConsoleInfo.dwCursorPosition.X > CON_RIGHT)
		ConsoleInfo.dwCursorPosition.X = CON_RIGHT;
	if(ConsoleInfo.dwCursorPosition.Y < CON_TOP)
		ConsoleInfo.dwCursorPosition.Y = CON_TOP;
	if(ConsoleInfo.dwCursorPosition.Y > CON_BOTTOM)
		ConsoleInfo.dwCursorPosition.Y = CON_BOTTOM;
}

// The new SetCursorPosition takes scroll regions into consideration
// (Paul Brannan 6/27/98)
void TConsole::SetCursorPosition(int x, int y) {
	if (x > CON_WIDTH)  x = CON_WIDTH;
	if (x < 0)			x = 0;
	if(iScrollEnd != -1) {
		if(y > iScrollEnd)		y = iScrollEnd;
	} else {
		if(y > CON_HEIGHT)		y = CON_HEIGHT;
	}
	if(iScrollStart != -1) {
		if(y < iScrollStart)	y = iScrollStart;
	} else {
		if(y < 0)				y = 0;
	}

	COORD Coord = {(short)(CON_LEFT + x), (short)(CON_TOP + y)};
	SetConsoleCursorPosition(hConsole, Coord);
	
	// Update the ConsoleInfo struct
	ConsoleInfo.dwCursorPosition.Y = Coord.Y;
	ConsoleInfo.dwCursorPosition.X = Coord.X;
}

void TConsole::MoveCursorPosition(int x, int y) {
	SetCursorPosition(CON_CUR_X + x, CON_CUR_Y + y);
}

void TConsole::SetExtendedMode(int iFunction, BOOL bEnable)
{
	// Probably should do something here...
	// Should change the screen mode, but do we need this?
}

void TConsole::SetScroll(int start, int end) {
	iScrollStart = start;
	iScrollEnd = end;
}

void TConsole::Beep() {
	if(ini.get_do_beep()) {
		if(!ini.get_speaker_beep()) printit("\a");
		else ::Beep(400, 100);
	}
}

void TConsole::SetCursorSize(int pct) {
	CONSOLE_CURSOR_INFO ci = {(pct != 0)?pct:1, pct != 0};
	SetConsoleCursorInfo(hConsole, &ci);
}

void saveScreen(CHAR_INFO *chiBuffer) {
	HANDLE hStdout;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
	SMALL_RECT srctReadRect;
	COORD coordBufSize;
	COORD coordBufCoord;
	
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hStdout, &ConsoleInfo);
	
    srctReadRect.Top = CON_TOP;    /* top left: row 0, col 0  */
    srctReadRect.Left = CON_LEFT;
    srctReadRect.Bottom = CON_BOTTOM; /* bot. right: row 1, col 79 */
    srctReadRect.Right = CON_RIGHT;
	
    coordBufSize.Y = CON_BOTTOM-CON_TOP+1;
    coordBufSize.X = CON_RIGHT-CON_LEFT+1;
	
    coordBufCoord.X = CON_TOP;
    coordBufCoord.Y = CON_LEFT;
	
    ReadConsoleOutput(
		hStdout,        /* screen buffer to read from       */
		chiBuffer,      /* buffer to copy into              */
		coordBufSize,   /* col-row size of chiBuffer        */
		
		coordBufCoord,  /* top left dest. cell in chiBuffer */
		&srctReadRect); /* screen buffer source rectangle   */
}

void restoreScreen(CHAR_INFO *chiBuffer) {
	HANDLE hStdout;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
	SMALL_RECT srctReadRect;
	COORD coordBufSize;
	COORD coordBufCoord;
	
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hStdout, &ConsoleInfo);
	
	// restore screen
    srctReadRect.Top = CON_TOP;    /* top left: row 0, col 0  */
    srctReadRect.Left = CON_LEFT;
    srctReadRect.Bottom = CON_BOTTOM; /* bot. right: row 1, col 79 */
    srctReadRect.Right = CON_RIGHT;
	
    coordBufSize.Y = CON_BOTTOM-CON_TOP+1;
    coordBufSize.X = CON_RIGHT-CON_LEFT+1;
	
    coordBufCoord.X = CON_TOP;
    coordBufCoord.Y = CON_LEFT;
    WriteConsoleOutput(
        hStdout, /* screen buffer to write to    */
        chiBuffer,        /* buffer to copy from          */
        coordBufSize,     /* col-row size of chiBuffer    */
        coordBufCoord, /* top left src cell in chiBuffer  */
        &srctReadRect); /* dest. screen buffer rectangle */
	// end restore screen
    
}

CHAR_INFO* newBuffer() {
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hStdout, &ConsoleInfo);
    CHAR_INFO * chiBuffer;
    chiBuffer = new CHAR_INFO[(CON_BOTTOM-CON_TOP+1)*(CON_RIGHT-CON_LEFT+1)];
	return chiBuffer;
}

void deleteBuffer(CHAR_INFO* chiBuffer) {
	delete[] chiBuffer;
}

