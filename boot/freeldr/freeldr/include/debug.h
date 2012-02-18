/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __DEBUG_H
#define __DEBUG_H

// OR this with DebugPrintMask to enable ...
#define DPRINT_NONE         0x00000000  // No debug print
#define DPRINT_WARNING      0x00000001  // debugger messages and other misc stuff
#define DPRINT_MEMORY       0x00000002  // memory management messages
#define DPRINT_FILESYSTEM   0x00000004  // file system messages
#define DPRINT_INIFILE      0x00000008  // .ini file messages
#define DPRINT_UI           0x00000010  // user interface messages
#define DPRINT_DISK         0x00000020  // disk messages
#define DPRINT_CACHE        0x00000040  // cache messages
#define DPRINT_REGISTRY     0x00000080  // registry messages
#define DPRINT_REACTOS      0x00000100  // ReactOS messages
#define DPRINT_LINUX        0x00000200  // Linux messages
#define DPRINT_HWDETECT     0x00000400  // hardware detection messages
#define DPRINT_WINDOWS      0x00000800  // messages from Windows loader
#define DPRINT_PELOADER     0x00001000  // messages from PE images loader
#define DPRINT_SCSIPORT     0x00002000  // messages from SCSI miniport
#define DPRINT_HEAP         0x00004000  // messages in a bottle

#if DBG && !defined(_M_ARM)

	VOID	DebugInit(VOID);
    ULONG   DbgPrint(const char *Format, ...);
    VOID    DbgPrint2(ULONG Mask, ULONG Level, const char *File, ULONG Line, char *Format, ...);
	VOID	DebugDumpBuffer(ULONG Mask, PVOID Buffer, ULONG Length);

    #define ERR_LEVEL      0x1
    #define FIXME_LEVEL    0x2
    #define WARN_LEVEL     0x4
    #define TRACE_LEVEL    0x8

    #define DBG_DEFAULT_CHANNEL(ch) static int DbgDefaultChannel = DPRINT_##ch

    #define ERR_CH(ch, fmt, ...)    DbgPrint2(DPRINT_##ch, ERR_LEVEL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define FIXME_CH(ch, fmt, ...)  DbgPrint2(DPRINT_##ch, FIXME_LEVEL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define WARN_CH(ch, fmt, ...)   DbgPrint2(DPRINT_##ch, WARN_LEVEL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define TRACE_CH(ch, fmt, ...)  DbgPrint2(DPRINT_##ch, TRACE_LEVEL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

    #define ERR(fmt, ...)    DbgPrint2(DbgDefaultChannel, ERR_LEVEL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define FIXME(fmt, ...)  DbgPrint2(DbgDefaultChannel, FIXME_LEVEL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define WARN(fmt, ...)   DbgPrint2(DbgDefaultChannel, WARN_LEVEL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
    #define TRACE(fmt, ...)  DbgPrint2(DbgDefaultChannel, TRACE_LEVEL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

    #define UNIMPLEMENTED DbgPrint("(%s:%d) WARNING: %s is UNIMPLEMENTED!\n", __FILE__, __LINE__, __FUNCTION__);

	#define BugCheck(fmt, ...)              do { DbgPrint("(%s:%d) Fatal Error in %s: " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); for (;;); } while (0)
	#define DbgDumpBuffer(mask, buf, len)	DebugDumpBuffer(mask, buf, len)

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

    #define DBG_DEFAULT_CHANNEL(ch)

    #define ERR_CH(ch, fmt, ...)
    #define FIXME_CH(ch, fmt, ...)
    #define WARN_CH(ch, fmt, ...)
    #define TRACE_CH(ch, fmt, ...)

    #define ERR(fmt, ...)
    #define FIXME(fmt, ...)
    #define WARN(fmt, ...)
    #define TRACE(fmt, ...)

    #define UNIMPLEMENTED

	#define DebugInit()
	#define BugCheck(fmt, ...)
	#define DbgDumpBuffer(mask, buf, len)

#endif // DBG

#endif // defined __DEBUG_H
