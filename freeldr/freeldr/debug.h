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

#define DPRINT_WARNING		0x00000001		// OR this with DebugPrintMask to enable debugger messages and other misc stuff
#define DPRINT_MEMORY		0x00000002		// OR this with DebugPrintMask to enable memory management messages

void	DebugPrint(ULONG Mask, char *format, ...);

#define BugCheck0(format) \
			{ \
				DebugPrint(DPRINT_WARNING, "Fatal Error: %s:%d\n", __FILE__, __LINE__); \
				DebugPrint(DPRINT_WARNING, format); \
				for (;;); \
			}

#define BugCheck1(format, arg1) \
			{ \
				DebugPrint(DPRINT_WARNING, "Fatal Error: %s:%d\n", __FILE__, __LINE__); \
				DebugPrint(DPRINT_WARNING, format, arg1); \
				for (;;); \
			}

#define BugCheck2(format, arg1, arg2) \
			{ \
				DebugPrint(DPRINT_WARNING, "Fatal Error: %s:%d\n", __FILE__, __LINE__); \
				DebugPrint(DPRINT_WARNING, format, arg1, arg2); \
				for (;;); \
			}

#define BugCheck3(format, arg1, arg2, arg3) \
			{ \
				DebugPrint(DPRINT_WARNING, "Fatal Error: %s:%d\n", __FILE__, __LINE__); \
				DebugPrint(DPRINT_WARNING, format, arg1, arg2, arg3); \
				for (;;); \
			}

#endif // defined __DEBUG_H