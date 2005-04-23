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
// Module:		ansiprsr.cpp
//
// Contents:	ANSI parser base class
//
// Product:		telnet
//
// Revisions: August 30, 1998 Paul Brannan <pbranna@clemson.edu>
//            July 29, 1998 pbranna@clemson.edu
//            June 15, 1998 pbranna@clemson.edu
//            May 19, 1998  pbranna@clemson.edu
//            24 Dec, 1997  Andrey.V.Smilianets
//            05. Sep.1997  roryt@hol.gr (I.Ioannou)
//            11.May.1997   roryt@hol.gr (I.Ioannou)
//            6.April.1997  roryt@hol.gr (I.Ioannou)
//            5.April.1997  jbj@nounname.com
//            30.M„rz.1997	Titus_Boxberg@public.uni-hamburg.de
//		      14.Sept.1996  jbj@nounname.com
//            Version 2.0
//
//            13.Jul.1995	igor.milavec@uni-lj.si
//					  Original code
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <string.h>
#include "ansiprsr.h"

// The constructor now takes different arguments and initializes different
// variables (Paul Brannan 6/15/98)
TANSIParser::TANSIParser(TConsole &RefConsole, KeyTranslator &RefKeyTrans,
						 TScroller &RefScroller, TNetwork &RefNetwork,
						 TCharmap &RefCharmap):
TParser(RefConsole, RefKeyTrans, RefScroller, RefNetwork, RefCharmap) {
	Init();
	iSavedAttributes = (unsigned char) 7;
	// must also check to make sure the string is non-NULL
	// (Paul Brannan 5/8/98)
	if ((ini.get_dumpfile() != NULL) && (*ini.get_dumpfile() != '\0')){
		dumpfile = fopen(ini.get_dumpfile(), "wb");
	}else {
		dumpfile = NULL;
	}
	InPrintMode = 0;
	printfile = NULL;

	fast_write = ini.get_fast_write(); // Paul Brannan 6/28/98
	Scroller.init(&StripBuffer);
}

TANSIParser::~TANSIParser(){
	if (dumpfile) fclose (dumpfile);
	// Added I.Ioannou 06 April, 1997
	if (printfile != NULL) fclose (printfile);
}

// Created Init() function to initialize the parser but not clear the screen
// (Paul Brannan 9/23/98)
void TANSIParser::Init() {
	// Paul Brannan 6/25/98
	map_G0 = 'B'; map_G1 = 'B';
	Charmap.setmap(map_G0);
	current_map = 'B';

	ignore_margins = 0;
	vt52_mode = 0;
	print_ctrl = 0;
	newline_mode = false;

	KeyTrans.clear_ext_mode();

	iSavedCurY = 0;							// Reset Variables
	iSavedCurX = 0;
	inGraphMode = 0;
	Console.SetScroll(-1, -1);
	Console.Normal();						// Reset Attributes

	// Set tabs stops
	resetTabStops();
}

void TANSIParser::ResetTerminal() {
	Init();
	Console.ClearScreen();					// Clear Screen
	Console.SetRawCursorPosition(0,0);		// Home Cursor
}
void TANSIParser::SaveCurY(int iY){
	iSavedCurY=iY;
}

void TANSIParser::SaveCurX(int iX){
	iSavedCurX=iX;
}

void TANSIParser::resetTabStops() {
	for(int j = 0; j < MAX_TAB_POSITIONS; j++) {
		tab_stops[j] = 8 + j - (j%8);
	}
}

void TANSIParser::ConSetAttribute(unsigned char TextAttrib){
	// Paul Brannan 5/8/98
	// Made this go a little bit faster by changing from switch{} to an array
	// for the colors
	if(TextAttrib >= 30) {
		if(TextAttrib <= 37) {
			Console.SetForeground(ANSIColors[TextAttrib-30]);
			return;
		} else if((TextAttrib >= 40) && (TextAttrib <= 47)) {
			Console.SetBackground(ANSIColors[TextAttrib-40]);
			return;
		}
	}
	
	switch (TextAttrib){
		// Text Attributes
	case 0: Console.Normal();           break;	// Normal video
	case 1: Console.HighVideo();        break;	// High video
	case 2: Console.LowVideo();         break;	// Low video
	case 4: Console.UnderlineOn();		break;	// Underline on (I.Ioannou)
	case 5: Console.BlinkOn();			break;	// Blink video
		// Corrected by I.Ioannou 11 May, 1997
	case 7: Console.ReverseOn();		break;	// Reverse video
	case 8:								break;	// hidden
		// All from 10 thru 27 are hacked from linux kernel
		// I.Ioannou 06 April, 1997
	case 10:
		//  I.Ioannou 04 Sep 1997 turn on/off high bit
		inGraphMode = 0;
		print_ctrl = 0;
		Charmap.setmap(current_map ? map_G1:map_G0); // Paul Brannan 6/25/98
		break; // ANSI X3.64-1979 (SCO-ish?)
		// Select primary font,
		// don't display control chars
		// if defined, don't set
		// bit 8 on output (normal)
	case 11:
		inGraphMode = 0;
		print_ctrl = 1;
		Charmap.setmap(0); // Paul Brannan 6/25/98
		break; // ANSI X3.64-1979 (SCO-ish?)
		// Select first alternate font,
		// let chars < 32 be displayed
		// as ROM chars
	case 12:
		inGraphMode = 1;
		print_ctrl = 1;
		Charmap.setmap(0); // Paul Brannan 6/25/98
		break; // ANSI X3.64-1979 (SCO-ish?)
		// Select second alternate font,
		// toggle high bit before
		// displaying as ROM char.
		
	case 21:									// not really Low video
	case 22: Console.LowVideo();		break;	// but this works good also
	case 24: Console.UnderlineOff();	break;	// Underline off
	case 25: Console.BlinkOff();		break;	// blink off
		// Corrected by I.Ioannou 11 May, 1997
	case 27: Console.ReverseOff();		break;	//Reverse video off

	// Mutt needs this (Paul Brannan, Peter Jordan 12/31/98)
	// This is from the Linux kernel source
    case 38: /* ANSI X3.64-1979 (SCO-ish?)
			  * Enables underscore, white foreground
			  * with white underscore (Linux - use
			  * default foreground).
			  */
			Console.UnderlineOn();
			Console.SetForeground(ini.get_normal_fg());
			break;
	case 39: /* ANSI X3.64-1979 (SCO-ish?)
			  * Disable underline option.
			  * Reset colour to default? It did this
			  * before...
			  */
			Console.UnderlineOff();
			Console.SetForeground(ini.get_normal_fg());
			break;
	case 49:
			Console.SetBackground(ini.get_normal_bg());
			break;

	}
}

void TANSIParser::ConSetCursorPos(int x, int y) {
	if(ignore_margins)
		Console.SetRawCursorPosition(x, y);
	else
		Console.SetCursorPosition(x, y);
}

char* TANSIParser::GetTerminalID()
{
	return "\033[?1;2c";
}

// All of the Telnet protocol stuff has been moved to TTelHndl.cpp
// This is more consistent with what OO should be
// (Paul Brannan 6/15/98)

#ifdef __BORLANDC__
// argsused doesn't work on MSVC++
#pragma argsused
#endif

// Use this for the VT100 flags (Paul Brannan 12/2/98)
#define FLAG_DOLLAR		0x0001
#define FLAG_QMARK		0x0002
#define FLAG_GREATER	0x0004
#define FLAG_LESS		0x0008
#define FLAG_EXCLAM		0x0010
#define FLAG_AMPERSAND	0x0020
#define FLAG_SLASH		0x0040
#define FLAG_EQUAL		0x0080
#define FLAG_QUOTE		0x0100
#define FLAG_OTHER		0x8000

char* TANSIParser::ParseEscapeANSI(char* pszBuffer, char* pszBufferEnd)
{
	
	//	The buffer contains something like <ESC>[pA
	//	where p is an optional decimal number specifying the count by which the
	//	appropriate action should take place.
	//	The pointer pszBuffer points us to the p, <ESC> and [ are
	//	already 'consumed'
	
	//	TITUS: Simplification of the code: Assume default count of 1 in case
	//	there are no parameters.
	char tmpc;
	const int nParam = 10;	// Maximum number of parameters
	int	iParam[nParam] = {1, 0, 0, 0, 0};	// Assume 1 Parameter, Default 1
	int iCurrentParam = 0;
	DWORD flag = 0;
	int missing_param = 0;

	// Get parameters from escape sequence.
	while ((tmpc = *pszBuffer) <= '?') {

		if(tmpc < '0' || tmpc > '9') {
			// Check for parameter delimiter.
			if(tmpc == ';') {
				// This is a hack (Paul Brannan 6/27/98)
				if(*(pszBuffer - 1) == '[') missing_param = iCurrentParam+1;
				pszBuffer++;
				continue;
			}

			// It is legal to have control characters inside ANSI sequences
			// (Paul Brannan 6/26/98)
			if(tmpc < ' ') {
				Console.WriteCtrlChar(tmpc);
				pszBuffer++;
				continue;
			}

			// A new way of handling flags (Paul Brannan 12/2/98)
			switch(tmpc) {
			case '$': flag |= FLAG_DOLLAR; break;
			case '?': flag |= FLAG_QMARK; break;
			case '>': flag |= FLAG_GREATER; break;
			case '<': flag |= FLAG_LESS; break;
			case '!': flag |= FLAG_EXCLAM; break;
			case '&': flag |= FLAG_AMPERSAND; break;
			case '/': flag |= FLAG_SLASH; break;
			case '=': flag |= FLAG_EQUAL; break;
			case '\"': flag |= FLAG_QUOTE; break;
			default: flag |= FLAG_OTHER; break;
			}

			pszBuffer++;
		}

		//  Got Numerical Parameter.
		iParam[iCurrentParam] = strtoul(pszBuffer, &pszBuffer, 10);
		if (iCurrentParam < nParam)
			iCurrentParam++;
	}
	
	//~~~ TITUS: Apparently the digit is optional (look at termcap or terminfo)
	// So: If there is no digit, assume a count of 1
	
	switch ((unsigned char)*pszBuffer++) {
		// Insert Character
		case '@':
			if(iParam[0] == 0) iParam[0] = 1; // Paul Brannan 9/1/98
			Console.InsertCharacter(iParam[0]); break;
		// Move cursor up.
		case 'A':
			if(iParam[0] == 0) iParam[0] = 1;
			Console.MoveCursorPosition(0, -iParam[0]); break;
		// Move cursor down.
		// Added by I.Ioannou 06 April, 1997
		case 'B':
		case 'e':
			if(iParam[0] == 0) iParam[0] = 1;
			Console.MoveCursorPosition(0, iParam[0]);
			break;
		// Move cursor right.
		// Added by I.Ioannou 06 April, 1997
		case 'C':
		case 'a':
			// Handle cursor size sequences (Jose Cesar Otero Rodriquez and
			// Paul Brannan, 3/27/1999)
			if(flag & FLAG_EQUAL) {
				switch(iParam[0]) {
				case 7: Console.SetCursorSize(50); break;
				case 11: Console.SetCursorSize(6); break;
				case 32: Console.SetCursorSize(0); break;
				default: Console.SetCursorSize(13);
				}
			} else {
				if(iParam[0] == 0) iParam[0] = 1;
				Console.MoveCursorPosition(iParam[0], 0);
				break;
			}
		// Move cursor left.
		case 'D':
			if(iParam[0] == 0) iParam[0] = 1;
			Console.MoveCursorPosition(-iParam[0], 0);
			break;
		// Move cursor to beginning of line, p lines down.
		// Added by I.Ioannou 06 April, 1997
		case 'E': 
			Console.MoveCursorPosition(-Console.GetCursorX(), iParam[0]);
			break;
		// Moves active position to beginning of line, p lines up
		// Added by I.Ioannou 06 April, 1997
		// With '=' this changes the default fg color (Paul Brannan 6/27/98)
		case 'F':
			if(flag & FLAG_EQUAL)
				Console.setDefaultFg(iParam[0]);
			else
				Console.MoveCursorPosition(-Console.GetCursorX(), -iParam[0]);
			break;
		// Go to column p
		// Added by I.Ioannou 06 April, 1997
		// With '=' this changes the default bg color (Paul Brannan 6/27/98)
		case '`': 
		case 'G': // 'G' is from Linux kernel sources
			if(flag & FLAG_EQUAL) {
				Console.setDefaultBg(iParam[0]);
			} else {
				if (iCurrentParam < 1)			// Alter Default
					iParam[0] = 0;
				// this was backward, and we should subtract 1 from x
				// (Paul Brannan 5/27/98)
				ConSetCursorPos(iParam[0] - 1, Console.GetCursorY());
			}
			break;
		// Set cursor position.
		case 'f': 
		case 'H':
			if (iCurrentParam < 2 || iParam[1] < 1)
				iParam[1] = 1;
			ConSetCursorPos(iParam[1] - 1, iParam[0] - 1);
			break;
		// Clear screen
		case 'J': 
			if ( iCurrentParam < 1 ) iParam[0] = 0;	// Alter Default
			switch (iParam[0]) {
				case 0: Console.ClearEOScreen(); break;
				case 1: Console.ClearBOScreen(); break;
				case 2:
					Console.ClearScreen();
					Console.SetRawCursorPosition(0, 0);
					break;
			}
			break;
		// Clear line
		case 'K': 
			if (iCurrentParam < 1)			// Alter Default
				iParam[0] = 0;
			switch (iParam[0]) {
				case 0: Console.ClearEOLine(); break;
				case 1: Console.ClearBOLine(); break;
				case 2: Console.ClearLine(); break;
			}
			break;
		//  Insert p new, blank lines.
		// Added by I.Ioannou 06 April, 1997
		case 'L': 
			{
				// for (int i = 1; i <= iParam[0]; i++)
				// This should speed things up a bit (Paul Brannan 9/2/98)
				Console.ScrollDown(Console.GetRawCursorY(), -1, iParam[0]);
				break;
			}
		//  Delete p lines.
		// Added by I.Ioannou 06 April, 1997
		case 'M': 
			{
				for (int i = 1; i <= iParam[0]; i++)
				// This should speed things up a bit (Paul Brannan 9/2/98)
				Console.ScrollDown(Console.GetRawCursorY(), -1, -1);
				break;
			}
		// DELETE CHAR
		case 'P': 
			Console.DeleteCharacter(iParam[0]);
			break;
		// Scrolls screen up (down? -- PB) p lines,
		// Added by I.Ioannou 06 April, 1997
		// ANSI X3.64-1979 references this but I didn't
		// found it in any telnet implementation
		// note 05 Oct 97  : but SCO terminfo uses them, so uncomment them !!
		case 'S': 
			{
				//for (int i = 1; i <= iParam[0]; i++)
				// This should speed things up a bit (Paul Brannan 9/2/98)
				Console.ScrollDown(-1, -1, -iParam[0]);
				break;
			}
		// Scrolls screen up p lines,
		// Added by I.Ioannou 06 April, 1997
		// ANSI X3.64-1979 references this but I didn't
		// found it in any telnet implementation
		// note 05 Oct 97  : but SCO terminfo uses them, so uncomment them !!
		case 'T': 
			{
				// for (int i = 1; i <= iParam[0]; i++)
				// This should speed things up a bit (Paul Brannan 9/2/98)
				Console.ScrollDown(-1, -1, iParam[0]);
				break;
			}
		//  Erases p characters up to the end of line
		// Added by I.Ioannou 06 April, 1997
		case 'X': 
			{
				int iKeepX = Console.GetRawCursorX();
				int iKeepY = Console.GetRawCursorY();
				if (iParam[0] > Console.GetWidth())
					iParam[0] = Console.GetWidth(); // up to the end of line
				for ( int i = 1; i <= iParam[0]; i++ )
					Console.WriteString(" ", 1);
				Console.SetRawCursorPosition(iKeepX , iKeepY);
				break;
			}
		// Go back p tab stops
		// Added by I.Ioannou 06 April, 1997
		// Implemented by Paul Brannan, 4/13/2000
		case 'Z':
			{
				int x = Console.GetCursorX();
				for(int j = 0; x > 0 && j < iParam[0]; j++)
					while(x > 0 && tab_stops[j] == tab_stops[x]) x--;
				Console.SetCursorPosition(x, Console.GetCursorY());
			}
			break;
		// Get Terminal ID
		case 'c': 
			{
				char* szTerminalId = GetTerminalID();
				Network.WriteString(szTerminalId, strlen(szTerminalId));
				break;
			}
		// TITUS++ 2. November 1998: Repeat Character.
		case 'b':
			// isprint may be causing problems (Paul Brannan 3/27/99)
			// if ( isprint(last_char) ) {
				char    buf[150];       // at most 1 line (max 132 chars)

				if ( iParam[0] > 149 ) iParam[0] = 149;
				memset(buf, last_char, iParam[0]);
				buf[iParam[0]] = 0;
				if ( fast_write )
					Console.WriteStringFast(buf, iParam[0]);
				else
					Console.WriteString(buf, iParam[0]);
			// } /* IF */
		break;
		// Go to line p
		// Added by I.Ioannou 06 April, 1997
		case 'd': 
			if (iCurrentParam < 1)			// Alter Default
				iParam[0] = 0;
			// this was backward, and we should subtract 1 from y
			// (Paul Brannan 5/27/98)
			ConSetCursorPos(Console.GetCursorX(), iParam[0] - 1);
			break;
		// iBCS2 tab erase
		// Added by I.Ioannou 06 April, 1997
		case 'g': 
			if (iCurrentParam < 1)			// Alter Default
				iParam[0] = 0;
			switch (iParam[0]) {
				case 0:
					{
						// Clear the horizontal tab stop at the current active position
						for(int j = 0; j < MAX_TAB_POSITIONS; j++) {
							int x = Console.GetCursorX();
							if(tab_stops[j] == x) tab_stops[j] = tab_stops[x + 1];
						}
					}
					break;
				case 2:
					// I think this might be "set as default?"
					break;
				case 3:
					{
						// Clear all tab stops
						for(int j = 0; j < MAX_TAB_POSITIONS; j++)
							tab_stops[j] = -1;
					}
					break;
			}
			break;
		// Set extended mode
		case 'h': 
			{
				for (int i = 0; i < iCurrentParam; i++) {
					// Changed to a switch statement (Paul Brannan 5/27/98)
					if(flag & FLAG_QMARK) {
						switch(iParam[i]) {
							case 1: // App cursor keys
								KeyTrans.set_ext_mode(APP_KEY);
								break;
							case 2: // VT102 mode
								vt52_mode = 0;
								KeyTrans.unset_ext_mode(APP2_KEY);
								break;
							case 3: // 132 columns
								if(ini.get_wide_enable()) {
									Console.SetWindowSize(132, -1);
								}
								break;
							case 4: // smooth scrolling
								break;
							case 5: // Light background
								Console.Lightbg();
								break;
							case 6: // Stay in margins
								ignore_margins = 0;
								break;
							case 7:
								Console.setLineWrap(true);
								break;
							case 8:	// Auto-repeat keys
								break;
							case 18: // Send FF to printer
								break;
							case 19: // Entire screen legal for printer
								break;
							case 25: // Visible cursor
								break;
							case 66: // Application numeric keypad
								break;
							default:
#ifdef DEBUG
								Console.Beep();
#endif
								break;
						}
					} else {
						switch(iParam[i]) {
							case 2: // Lock keyboard
								break;
							case 3: // Act upon control codes (PB 12/5/98)
								print_ctrl = 0;
								break;
							case 4: // Set insert mode
								Console.InsertMode(1);
								break;
							case 12: // Local echo off
								break;
							case 20: // Newline sends cr/lf
								KeyTrans.set_ext_mode(APP4_KEY);
								newline_mode = true;
								break;
							default:
#ifdef DEBUG
								Console.Beep();
#endif
								break;
						}
					}
				}
			}
			break;
		// Print Screen
		case 'i': 
			if (iCurrentParam < 1)
				iParam[0]=0;
			switch (iParam[0]){
				case 0: break; // Print Screen
				case 1: break; // Print Line
				// Added I.Ioannou 06 April, 1997
				case 4:
					// Stop Print Log
					InPrintMode = 0;
					if ( printfile != NULL )
						fclose(printfile);
					break;
				case 5:
					// Start Print Log
					printfile = fopen(ini.get_printer_name(), "ab");
					if (printfile != NULL) InPrintMode = 1;
					break;
			}
			break;
		// Unset extended mode
		case 'l': 
			{
				for (int i = 0; i < iCurrentParam; i++) {
					// Changed to a switch statement (Paul Brannan 5/27/98)
					if(flag & FLAG_QMARK) {
						switch(iParam[i]) {
							case 1: // Numeric cursor keys
								KeyTrans.unset_ext_mode(APP_KEY);
								break;
							case 2: // VT52 mode
								vt52_mode = 1;
								KeyTrans.set_ext_mode(APP2_KEY);
								break;
							case 3: // 80 columns
								if(ini.get_wide_enable()) {
									Console.SetWindowSize(80, -1);
								}
								break;
							case 4: // jump scrolling
								break;
							case 5: // Dark background
								Console.Darkbg();
								break;
							case 6: // Ignore margins
								ignore_margins = 1;
								break;
							case 7:
								Console.setLineWrap(false);
								break;
							case 8:	// Auto-repeat keys
								break;
							case 19: // Only send scrolling region to printer
								break;
							case 25: // Invisible cursor
								break;
							case 66: // Numeric keypad
								break;
							default:
#ifdef DEBUG
								Console.Beep();
#endif
								break;
						}
					} else {
						switch(iParam[i]) {
							case 2: // Unlock keyboard
								break;
							case 3: // Display control codes (PB 12/5/98)
								print_ctrl = 1;
								break;
							case 4: // Set overtype mode
								Console.InsertMode(0);
								break;
							case 12: // Local echo on
								break;
							case 20: // sends lf only
								KeyTrans.unset_ext_mode(APP4_KEY);
								newline_mode = false;
								break;
							default:
#ifdef DEBUG
								Console.Beep();
#endif
								break;
						}
					}
				}
			}
			break;
		// Set color
		case 'm':
			if(missing_param) Console.Normal();
			if(iCurrentParam == 0) {
				Console.Normal();
			} else {
				for(int i = 0; i < iCurrentParam; i++)
					ConSetAttribute(iParam[i]);
			}
			break;
		// report cursor position Row X Col
		case 'n': 
			if (iCurrentParam == 1 && iParam[0]==5) {
				// report the cursor position
				Network.WriteString("\x1B[0n", 6);
				break;
			}
			if (iCurrentParam == 1 && iParam[0]==6){
				// report the cursor position
				// The cursor position needs to be sent as a single string
				// (Paul Brannan 6/27/98)
				char szCursorReport[40] = "\x1B[";

				itoa(Console.GetCursorY() + 1,
					&szCursorReport[strlen(szCursorReport)], 10);
				strcat(szCursorReport, ";");
				itoa(Console.GetCursorX() + 1,
					&szCursorReport[strlen(szCursorReport)], 10);
				strcat(szCursorReport, "R");

				Network.WriteString(szCursorReport, strlen(szCursorReport));
		
			}
			break;
		// Miscellaneous weird sequences (Paul Brannan 6/27/98)
		case 'p':
			// Set conformance level
			if(flag & FLAG_QUOTE) {
				break;
			}
			// Soft terminal reset
			if(flag & FLAG_EXCLAM) {
				break;
			}
			// Report mode settings
			if(flag & FLAG_DOLLAR) {
				break;
			}
			break;
		// Scroll Screen
		case 'r': 
			if (iCurrentParam < 1) {
				// Enable scrolling for entire display
				Console.SetScroll(-1, -1);
				break;
			}
			if (iCurrentParam >1) {
				// Enable scrolling from row1 to row2
				Console.SetScroll(iParam[0] - 1, iParam[1] - 1);
				// If the cursor is outside the scrolling range, fix it
				// (Paul Brannan 6/26/98)
				// if(Console.GetRawCursorY() < iParam[0] - 1) {
				// 	Console.SetRawCursorPosition(Console.GetCursorX(),
				// 		iParam[0] - 1);
				// }
				// if(Console.GetRawCursorY() > iParam[1] - 1) {
				// 	Console.SetRawCursorPosition(Console.GetCursorX(),
				// 		iParam[1] - 1);
				// }
			}
			// Move the cursor to the home position (Paul Brannan 12/2/98)
			Console.SetCursorPosition(0, 0);
			break;
		// Save cursor position
		case 's': 
			SaveCurY(Console.GetRawCursorY());
			SaveCurX(Console.GetRawCursorX());
			break;
		// Restore cursor position
		case 'u': 
			Console.SetRawCursorPosition(iSavedCurX, iSavedCurY);
			break;
		// DEC terminal report (Paul Brannan 6/28/98)
		case 'x':
			if(iParam[0])
				Network.WriteString("\033[3;1;1;128;128;1;0x", 20);
			else
				Network.WriteString("\033[2;1;1;128;128;1;0x", 20);
			break;
		default:
#ifdef DEBUG
			Console.Beep();
#endif
			break;
	}

	return pszBuffer;
}

#ifdef MTE_SUPPORT
// Added by Frediano Ziglio, 5/31/2000
// MTE extension
// initially copied from ParseEscapeANSI
char* TANSIParser::ParseEscapeMTE(char* pszBuffer, char* pszBufferEnd)
{
	//      The buffer contains something like <ESC>~pA
	//      where p is an optional decimal number specifying the count by which the
	//      appropriate action should take place.
	//      The pointer pszBuffer points us to the p, <ESC> and ~ are
	//      already 'consumed'
	//      TITUS: Simplification of the code: Assume default count of 1 in case
	//      there are no parameters.
	char tmpc;
	const int nParam = 10;  // Maximum number of parameters
	int     iParam[nParam] = {1, 0, 0, 0, 0};       // Assume 1 parameter, Default 1
	int iCurrentParam = 0;
	char sRepeat[2];
	
	// Get parameters from escape sequence.
	while ((tmpc = *pszBuffer) <= '?') {
		if(tmpc < '0' || tmpc > '9') {
			// Check for parameter delimiter.
			if(tmpc == ';') {
				pszBuffer++;
				continue;
			}
			pszBuffer++;
		}
		
		//  Got Numerical Parameter.
		iParam[iCurrentParam] = strtoul(pszBuffer, &pszBuffer, 10);
		if (iCurrentParam < nParam)
			iCurrentParam++;
	}
	
	//~~~ TITUS: Apparently the digit is optional (look at termcap or terminfo)
	// So: If there is no digit, assume a count of 1
	
	switch ((unsigned char)*pszBuffer++) {
		case 'A':
			// set colors
			if (iCurrentParam < 2 )
				break;
			if (iParam[0] <= 15 && iParam[1] <= 15)
				Console.SetAttrib( (iParam[1] << 4) | iParam[0] );
			break;
			
		case 'R':
			// define region
			mteRegionXF = -1;
			if (iCurrentParam < 2 )
				break;
			mteRegionXF = iParam[1]-1;
			mteRegionYF = iParam[0]-1;
			break;
			
		case 'F':
			// fill with char
			{
				if (mteRegionXF == -1 || iCurrentParam < 1)
					break;
				sRepeat[0] = (char)iParam[0];
				sRepeat[1] = '\0';
				int xi = Console.GetCursorX(),yi = Console.GetCursorY();
				int xf = mteRegionXF;
				int yf = mteRegionYF;
				mteRegionXF = -1;
				for(int y=yi;y<=yf;++y)
				{
					Console.SetCursorPosition(xi,y);
					for(int x=xi;x<=xf;++x)
						
						Console.WriteStringFast(sRepeat,1);
				}
			}
			break;
			
		case 'S':
			// Scroll region
			{
				if (mteRegionXF == -1 || iCurrentParam < 2)
					break;
				int /*x = Console.GetCursorX(),*/y = Console.GetCursorY();
				// int xf = mteRegionXF;
				int yf = mteRegionYF;
				mteRegionXF = -1;
				// !!! don't use x during scroll
				int diff = (iParam[0]-1)-y;
				if (diff<0)
					Console.ScrollDown(y-1,yf,diff);
				else
					Console.ScrollDown(y,yf+1,diff);
			}
			break;
			// Meridian main version ??
		case 'x':
			// disable echo and line mode
			Network.set_local_echo(0);
			Network.set_line_mode(0);
			// Meridian Server handle cursor itself
			Console.SetCursorSize(0);
			break;
			// query ??
		case 'Q':
			if (iParam[0] == 1)
				Network.WriteString("\033vga.",5);
			break;
		default:
#ifdef DEBUG
			Console.Beep();
#endif
			break;
	}
	
	return pszBuffer;
 }
#endif

char* TANSIParser::ParseEscape(char* pszBuffer, char* pszBufferEnd) {
	char *pszChar;

	// Check if we have enough characters in buffer.
	if ((pszBufferEnd - pszBuffer) < 2)
		return pszBuffer;
	
	//  I.Ioannou 04 Sep 1997
	// there is no need for pszBuffer++; after each command
	
	// Decode the command.
	pszBuffer++;
	
	switch (*pszBuffer++) {
		case 'A': // Cursor up
			Console.MoveCursorPosition(0, -1);
			break;
		// Cursor down
		case 'B': 
  			Console.MoveCursorPosition(0, 1);
			break;
		// Cursor right
		case 'C':
  			Console.MoveCursorPosition(1, 0);
			break;
		// LF *or* cursor left (Paul Brannan 6/27/98)
		case 'D':
			if(vt52_mode)
				Console.MoveCursorPosition(-1, 0);
			else
				Console.index();
			break;
		// CR/LF (Paul Brannan 6/26/98)
		case 'E':
			Console.WriteCtrlString("\r\n", 2);
			break;
		// Special graphics char set (Paul Brannan 6/27/98)
		case 'F':
			Charmap.setmap('0');
			break;
		// ASCII char set (Paul Brannan 6/27/98)
		case 'G':
			Charmap.setmap('B');
			break;
		// Home cursor/tab set
		case 'H': 
			if(ini.get_vt100_mode()) {
				int x = Console.GetCursorX();
				if(x != 0) {
					int t = tab_stops[x - 1];
					for(int j = x - 1; j >= 0 && tab_stops[j] == t; j--)
						tab_stops[j] = x;
				}
			} else {
				//  I.Ioannou 04 Sep 1997 (0,0) not (1,1)
				ConSetCursorPos(0, 0);
			}
			break;
		// Reverse line feed (Paul Brannan 6/27/98)
		// FIX ME!!!  reverse_index is wrong to be calling here
		// (Paul Brannan 12/2/98)
		case 'I':
			Console.reverse_index();
			break;
		// Erase end of screen
		case 'J': 
			Console.ClearEOScreen();
			break;
		// Erase EOL
		case 'K':
			Console.ClearEOLine();
			break;
		// Scroll Up one line //Reverse index
		case 'M':
			Console.reverse_index();
			break;
		// Direct cursor addressing
		case 'Y':
			if ((pszBufferEnd - pszBuffer) >= 2){
				// if we subtract '\x1F', then we may end up with a negative
				// cursor position! (Paul Brannan 6/26/98)
				ConSetCursorPos(pszBuffer[1] - ' ',	pszBuffer[0] - ' ');
				pszBuffer+=2;
			} else {
				pszBuffer--; // Paul Brannan 6/26/98
			}
			break;
		// Terminal ID Request
		case 'Z':
			{
				char* szTerminalId = GetTerminalID();
				Network.WriteString(szTerminalId, strlen(szTerminalId));
				break;
			}
		// reset terminal to defaults
		case 'c':
			ResetTerminal();
			break;
		// Enter alternate keypad mode
		case '=':
			KeyTrans.set_ext_mode(APP3_KEY);
			break;
		// Exit alternate keypad mode
		case '>':
			KeyTrans.unset_ext_mode(APP3_KEY);
			break;
		// Enter ANSI mode
		case '<':
			KeyTrans.unset_ext_mode(APP2_KEY); // exit vt52 mode
			break;
		// Graphics processor on (See note 3)
		case '1':
			break;
		// Line size commands
		case '#':        //Line size commands
			// (Paul Brannan 6/26/98)
			if(pszBuffer < pszBufferEnd) {
				switch(*pszBuffer++) {
				case '3': break; // top half of a double-height line
				case '4': break; // bottom half of a double-height line
				case '6': break; // current line becomes double-width
				case '8': Console.ClearScreen('E'); break;
				}
			} else {
				pszBuffer--;
			}			
			break;
		// Graphics processor off (See note 3)
		case '2':
			break;
		// Save cursor and attribs
		case '7':
			SaveCurY(Console.GetRawCursorY());
			SaveCurX(Console.GetRawCursorX());
			iSavedAttributes = Console.GetAttrib();
			break;
			// Restore cursor position and attribs
		case '8':
			Console.SetRawCursorPosition(iSavedCurX, iSavedCurY);
			Console.SetAttrib(iSavedAttributes);
			break;
		// Set G0 map (Paul Brannan 6/25/98)
		case '(':
			if (pszBuffer < pszBufferEnd) {
				map_G0 = *pszBuffer;
				if(current_map == 0) Charmap.setmap(map_G0);
				pszBuffer++;
			} else {
				pszBuffer--;
			}
			break;
		// Set G1 map (Paul Brannan 6/25/98)
		case ')':
			if (pszBuffer < pszBufferEnd) {
				map_G1 = *pszBuffer;
				if(current_map == 1) Charmap.setmap(map_G1);
				pszBuffer++;
			} else {
				pszBuffer--;
			}
			break;
		// This doesn't do anything, as far as I can tell, but it does take
		// a parameter (Paul Brannan 6/27/98)
		case '%':
			if (pszBuffer < pszBufferEnd) {
				pszBuffer++;
			} else {
				pszBuffer--;
			}
			break;
		// ANSI escape sequence
		case '[':
			// Check if we have whole escape sequence in buffer.
			// This should not be isalpha anymore (Paul Brannan 9/1/98)
			pszChar = pszBuffer;
			while ((pszChar < pszBufferEnd) && (*pszChar <= '?'))
				pszChar++;
			if (pszChar == pszBufferEnd)
				pszBuffer -= 2;
			else
				pszBuffer = ParseEscapeANSI(pszBuffer, pszBufferEnd);
			break;
#ifdef MTE_SUPPORT
		case '~':
			// Frediano Ziglio, 5/31/2000
			// Meridian Terminal Emulator extension
			// !!! same as ANSI
			// !!! should put in MTE procedure
			pszChar = pszBuffer;
			while ((pszChar < pszBufferEnd) && (*pszChar <= '?'))
				pszChar++;
			if (pszChar == pszBufferEnd)
				pszBuffer -= 2;
			else
				pszBuffer = ParseEscapeMTE(pszBuffer, pszBufferEnd);
			break;
#endif
		default:
#ifdef DEBUG
			Console.Beep();
#endif
			break;
	}

	return pszBuffer;
}

// This function now only parses the ANSI buffer and does not do anything
// with IAC sequences.  That code has been moved to TTelHndl.cpp.
// The scroller update routines have been moved to TScroll.cpp.
// (Paul Brannan 6/15/98)
char* TANSIParser::ParseBuffer(char* pszHead, char* pszTail){
	// copy into ANSI buffer
	char * pszResult;
	
	// Parse the buffer for ANSI or display
	while (pszHead < pszTail) {
		if(!ini.get_output_redir()) {
			pszResult = ParseANSIBuffer(pszHead, pszTail);
		} else {
			// Output is being redirected
			if(ini.get_strip_redir()) {
				// Skip the WriteFile() altogether and pass the buffer to a filter
				// Mark Miesfield 09/24/2000
				pszResult = PrintGoodChars(pszHead, pszTail);
			} else {
				DWORD Result;
				// Paul Brannan 7/29/98
				// Note that this has the unforunate effect of printing out
				// NULL (ascii 0) characters onto the screen
				if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),	pszHead,
					pszTail - pszHead, &Result,	NULL)) pszResult = pszHead;
				pszResult = pszHead + Result;
			}
		}
		if (dumpfile)
			fwrite( pszHead, sizeof (char), pszResult-pszHead, dumpfile);
		if(ini.get_scroll_enable()) Scroller.update(pszHead, pszResult);
		if (pszResult == pszHead) break;
		pszHead = pszResult;
	}
	// return the new head to the buffer
	return pszHead;
}

// A simple routine to strip ANSI sequences
// This isn't perfect, but it does an okay job (Paul Brannan 7/5/98)
// Fixed a line counting bug (Paul Brannan 12/4/98)
int TANSIParser::StripBuffer(char* pszHead, char* pszTail, int width) {
	int lines = 0, c = 0;
	char *pszBuf = pszHead;

	while(pszHead < pszTail) {
		if(iscntrl(*pszHead)) {
			switch(*(pszHead++)) {
			case 8:
			case 127:
				if(c>0) {
					if(!(c%width)) lines--;
					c--;
					pszBuf--;
				}
				break;
			case 10: lines++;
			case 13:
				*(pszBuf++) = *(pszHead - 1);
				c = 0;
				break;
			case 27:
				switch(*(pszHead++)) {
				case 'Y': pszHead += 2; break;
				case '#':
				case '(':
				case ')':
				case '%': pszHead++; break;
				case '[':
					while((pszHead < pszTail) && (*pszHead < '?'))
						pszHead++;
					pszHead++;
					break;
				}
			}
		} else {
			*(pszBuf++) = *(pszHead++);
			c++;
		}
		if(c != 0 && !(c%width))
			lines++;
	}
	
	// Fill in the end of the buffer with blanks
	while(pszBuf <= pszTail) *pszBuf++ = ' ';

	return lines;
}

char* TANSIParser::ParseANSIBuffer(char* pszBuffer, char* pszBufferEnd)
{
	if(InPrintMode) {
		return PrintBuffer(pszBuffer, pszBufferEnd);
	}
	
	unsigned char tmpc = *(unsigned char *)pszBuffer;

	if(tmpc == 27) {
		return ParseEscape(pszBuffer, pszBufferEnd);
	}
	
//	if((fast_write && tmpc < 32) ||
//		!print_ctrl && (tmpc < 32 || (EightBit_Ansi &&
//		(tmpc > 128 && tmpc < 128 + ' ')))) {

	// We shouldn't print ctrl characters when fast write is enabled
	// and ctrl chars are disabled (Paul Brannan 9/1/98)
	if(tmpc < 32) {
		// From the Linux kernel (Paul Brannan 12/5/98):
		/* A bitmap for codes <32. A bit of 1 indicates that the code
		 * corresponding to that bit number invokes some special action
		 * (such as cursor movement) and should not be displayed as a
		 * glyph unless the disp_ctrl mode is explicitly enabled.
		 */
		const long CTRL_ACTION = 0x0d00ff81;
		const long CTRL_ALWAYS = 0x0800f501;
		if(!(((print_ctrl?CTRL_ALWAYS:CTRL_ACTION)>>tmpc)&1)) {
			
			Console.WriteString((char *)&tmpc, 1);
			pszBuffer++;
			return pszBuffer;
		}

		switch (tmpc) {
		case 0:
			pszBuffer++;
			break;
		
		// I.Ioannou 5/30/98
		case 7:
			Console.Beep();
			pszBuffer++;
			break;
		
		// destructive backspace
		case 8:
			// Added option for destructive backspace (Paul Brannan 5/13/98)
			// Changed to ConWriteCtrlString so that the cursor position can be
			// updated (Paul Brannan 5/25/98)
			if(ini.get_dstrbksp()) {
				Console.WriteCtrlChar('\b');
				Console.WriteString(" ", 1);
				Console.WriteCtrlChar('\b');
			}
			else Console.WriteCtrlChar('\b');
			pszBuffer++;
			break;
		
		// horizontal tab
		case 9:
			{
				pszBuffer++;
				int x = Console.GetCursorX();
				if(x != -1)
					Console.SetCursorPosition(tab_stops[x], Console.GetCursorY());
			}
			break;
		
		// Line Feed Char
		case 10:
			// Test for local echo (Paul Brannan 8/25/98)
			if(Network.get_local_echo() || newline_mode) // &&
				Console.WriteCtrlChar('\x0d');
			Console.WriteCtrlChar('\x0a');
			pszBuffer++;
			break;
		
		// form feed
		case 12:
			pszBuffer++;
			Console.ClearScreen();
			Console.SetRawCursorPosition(Console.GetCursorX(), 1); // changed fm 1
			break;
		
		case 13:
			Console.WriteCtrlChar('\x0d');
			pszBuffer++;

			break;

		case 14:  // shift out of alternate chararcter set
			pszBuffer++;
			Charmap.setmap(map_G1); // Paul Brannan 6/25/98
			current_map = 1;
			break;
	
		case 15:  // shift in
			pszBuffer++;
			Charmap.setmap(map_G0); // Paul Brannan 6/25/98
			current_map = 0;
			break;
		
		// Paul Brannan 9/1/98 - Is this okay?
		default:
			pszBuffer++;
		}

		return pszBuffer;
	}

	//  added by I.Ioannou 06 April, 1997
	//  In 8 bit systems the server may send 0x9b instead of ESC[
	//  Well, this will produce troubles in Greek 737 Code page
	//  which uses 0x9b as the small "delta" - and I thing that there
	//  is another European country with the same problem.
	//  If we have to stay 8-bit clean we may have to
	//  give the ability of ROM characters (ESC[11m),
	//  for striped 8'th bit (ESC[12m) as SCO does,
	//  or a parameter at compile (or run ?) time.
	// We now check for a flag in the ini file (Paul Brannan 5/13/98)
	// We also handle any 8-bit ESC sequence (Paul Brannan 6/28/98)
	if(ini.get_eightbit_ansi() && (tmpc > 128 && tmpc < 128 + ' ')) {
		// There's a chance the sequence might not parse.  If this happens
		// then pszBuffer will be one character too far back, since
		// ParseEscape is expecting two characters, not one.
		// In that case we must handle it.
		char *pszCurrent = pszBuffer;
		pszBuffer = ParseEscape(pszBuffer, pszBufferEnd);
		if(pszBuffer < pszCurrent) pszBuffer = pszCurrent;
	}

	char* pszCurrent = pszBuffer + 1;
	// I.Ioannou 04 Sep 1997 FIXME with ESC[11m must show chars < 32
	// Fixed (Paul Brannan 6/28/98)
	while ((pszCurrent < pszBufferEnd) && (!iscntrl(*pszCurrent))) {
		// I.Ioannou 04 Sep 1997 strip on high bit
		if ( (inGraphMode) && (*pszCurrent > (char)32) )
			*pszCurrent |= 0x80 ;
		pszCurrent++;
	}
	
	// Note that this may break dumpfiles slightly.
	// If 'B' is set to anything other than ASCII, this will cause problems
	// (Paul Brannan 6/28/98)
	if(current_map != 'B' && Charmap.enabled)
		Charmap.translate_buffer(pszBuffer, pszCurrent);    
	
	last_char = *(pszCurrent-1);    // TITUS++: Remember last char

	if(fast_write) {
		pszBuffer += Console.WriteStringFast(pszBuffer,
			pszCurrent - pszBuffer);
	} else {
		pszBuffer += Console.WriteString(pszBuffer,
			pszCurrent - pszBuffer);
	}

	return pszBuffer;
}

// Added by I.Ioannou 06 April, 1997
// Print the buffer until you reach ESC[4i
char* TANSIParser::PrintBuffer(char* pszBuffer, char* pszBufferEnd) {
	// Check if we have enough characters in buffer.
	if ((pszBufferEnd - pszBuffer) < 4)
		return pszBuffer;
	char *tmpChar;
	
	tmpChar = pszBuffer;
	if ( *tmpChar == 27 ) {
		tmpChar++;
		if ( *tmpChar == '[' ) {
			tmpChar++;
			if ( *tmpChar == '4' ) {
				tmpChar++;
				if ( *tmpChar == 'i' ) {
					InPrintMode = 0; // Stop Print Log
					if ( printfile != NULL )
						fclose(printfile);
					pszBuffer += 4;
					return pszBuffer;
				}
			}
		}
	}
	
	if (printfile != NULL) {
		fputc( *pszBuffer, printfile);
		pszBuffer++;
	} else
		InPrintMode = 0;
	
	return pszBuffer;
}

/* - PrintGoodChars( pszHead, pszTail ) - - - - - - - - - - - - - - - - - - -
-*

  Mark Miesfield 09/24/2000

  Prints the characters in a buffer, from the specified head to the specified
  tail, to standard out, skipping any control characters or ANSI escape
  sequences.

  Parameters on entry:
    pszHead  ->  Starting point in buffer.

    pszTail  ->  Ending point in buffer.

  Returns:
    Pointer to the first character in the buffer that was not output to
    standard out.  (Since no error checking is done, this is in effect
    pszTail.)

  Side Effects:
    None.
* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/
char * TANSIParser::PrintGoodChars( char * pszHead, char * pszTail )  {

  while ( pszHead < pszTail )  {
    if ( iscntrl( *pszHead ) )  {
      switch ( *(pszHead++) )  {
        case 10 :
          putc( 10, stdout );
          break;

        case 13 :
          putc( 13, stdout );
          break;

        case 27:
          switch ( *(pszHead++) )  {
            case 'Y':
              pszHead += 2;
              break;

            case '#':
            case '(':
            case ')':
            case '%': pszHead++; break;
            case '[':
              while ( (pszHead < pszTail) && (*pszHead < '?') )
                pszHead++;
              pszHead++;
              break;

            default :
              break;
          }
          break;

        default :
          break;
      }
    }
    else
      putc( *(pszHead++), stdout );
  }
  return ( pszTail );
}
// End of function:  PrintGoodChars( pszHead, pszTail )
