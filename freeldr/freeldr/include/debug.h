/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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

	#define DPRINT_NONE		    0x00000000  // Simple debug print
	#define DPRINT_WARNING		0x00000001  // OR this with DebugPrintMask to enable debugger messages and other misc stuff
	#define DPRINT_MEMORY		  0x00000002  // OR this with DebugPrintMask to enable memory management messages
	#define DPRINT_FILESYSTEM	0x00000004  // OR this with DebugPrintMask to enable file system messages
	#define DPRINT_INIFILE		0x00000008  // OR this with DebugPrintMask to enable .ini file messages
	#define DPRINT_UI			    0x00000010  // OR this with DebugPrintMask to enable user interface messages
	#define DPRINT_DISK			  0x00000020  // OR this with DebugPrintMask to enable disk messages
	#define DPRINT_CACHE		  0x00000040  // OR this with DebugPrintMask to enable cache messages
	#define DPRINT_REGISTRY		0x00000080  // OR this with DebugPrintMask to enable registry messages
	#define DPRINT_REACTOS		0x00000100  // OR this with DebugPrintMask to enable ReactOS messages
	#define DPRINT_LINUX		  0x00000200  // OR this with DebugPrintMask to enable Linux messages

	VOID	DebugInit(VOID);
	VOID	DebugPrint(U32 Mask, char *format, ...);
	VOID	DebugDumpBuffer(U32 Mask, PVOID Buffer, U32 Length);

	#define DbgPrint(_x_)					{ DebugPrint(DPRINT_NONE, "%s:%d(%s)\n", __FILE__, __LINE__, __FUNCTION__);  DebugPrint _x_ ; }
	#define BugCheck(_x_)				  { DebugPrint(DPRINT_WARNING, "Fatal Error: %s:%d(%s)\n", __FILE__, __LINE__, __FUNCTION__); DebugPrint _x_ ; for (;;); }
	#define DbgDumpBuffer(_x_, _y_, _z_)	DebugDumpBuffer(_x_, _y_, _z_)

#ifdef __i386__

	// Debugging support functions:
	//
	// BREAKPOINT() - Inserts an "int 3" instruction
	// INSTRUCTION_BREAKPOINTX(x) - Enters exception handler right before instruction at address "x" is executed
	// MEMORY_READWRITE_BREAKPOINTX(x) - Enters exception handler when a read or write occurs at address "x"
	// MEMORY_WRITE_BREAKPOINTX(x) - Enters exception handler when a write occurs at address "x"
	//
	// You may have as many BREAKPOINT()'s as you like but you may only
	// have up to four of any of the others.
#define	BREAKPOINT()				__asm__ ("int $3");
void	INSTRUCTION_BREAKPOINT1(unsigned long addr);
void	MEMORY_READWRITE_BREAKPOINT1(unsigned long addr);
void	MEMORY_WRITE_BREAKPOINT1(unsigned long addr);
void	INSTRUCTION_BREAKPOINT2(unsigned long addr);
void	MEMORY_READWRITE_BREAKPOINT2(unsigned long addr);
void	MEMORY_WRITE_BREAKPOINT2(unsigned long addr);
void	INSTRUCTION_BREAKPOINT3(unsigned long addr);
void	MEMORY_READWRITE_BREAKPOINT3(unsigned long addr);
void	MEMORY_WRITE_BREAKPOINT3(unsigned long addr);
void	INSTRUCTION_BREAKPOINT4(unsigned long addr);
void	MEMORY_READWRITE_BREAKPOINT4(unsigned long addr);
void	MEMORY_WRITE_BREAKPOINT4(unsigned long addr);

#endif // defined __i386__

#else

	#define DbgPrint(_x_)
	#define BugCheck(_x_)
	#define DbgDumpBuffer(_x_, _y_, _z_)

#endif // defined DEBUG

#define UNIMPLEMENTED   BugCheck((DPRINT_WARNING, "Unimplemented\n"));

#endif // defined __DEBUG_H
