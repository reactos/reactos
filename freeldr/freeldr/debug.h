/*
 *  FreeLoader
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
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


#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef DEBUG

	#define DPRINT_WARNING		0x00000001		// OR this with DebugPrintMask to enable debugger messages and other misc stuff
	#define DPRINT_MEMORY		0x00000002		// OR this with DebugPrintMask to enable memory management messages
	#define DPRINT_FILESYSTEM	0x00000004		// OR this with DebugPrintMask to enable file system messages
	#define DPRINT_INIFILE		0x00000008		// OR this with DebugPrintMask to enable .ini file messages
	#define DPRINT_UI			0x00000010		// OR this with DebugPrintMask to enable user interface messages

	VOID	DebugInit(VOID);
	VOID	DebugPrint(ULONG Mask, char *format, ...);

	#define DbgPrint(_x_)	DebugPrint _x_
	#define BugCheck(_x_) { DebugPrint(DPRINT_WARNING, "Fatal Error: %s:%d\n", __FILE__, __LINE__); DebugPrint _x_ ; for (;;); }

#else

	#define DbgPrint(_x_)
	#define BugCheck(_x_)

#endif // defined DEBUG

#endif // defined __DEBUG_H