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

#define DPRINT_NONE         0   // No debug print
#define DPRINT_WARNING      1   // debugger messages and other misc stuff
#define DPRINT_MEMORY       2   // memory management messages
#define DPRINT_FILESYSTEM   3   // file system messages
#define DPRINT_INIFILE      4   // .ini file messages
#define DPRINT_UI           5   // user interface messages
#define DPRINT_DISK         6   // disk messages
#define DPRINT_CACHE        7   // cache messages
#define DPRINT_REGISTRY     8   // registry messages
#define DPRINT_REACTOS      9   // ReactOS messages
#define DPRINT_LINUX        10  // Linux messages
#define DPRINT_HWDETECT     11  // hardware detection messages
#define DPRINT_WINDOWS      12  // messages from Windows loader
#define DPRINT_PELOADER     13  // messages from PE images loader
#define DPRINT_SCSIPORT     14  // messages from SCSI miniport
#define DPRINT_HEAP         15  // messages in a bottle
#define DBG_CHANNELS_COUNT  16

#if DBG && !defined(_M_ARM)

    VOID    DebugInit(IN ULONG_PTR FrLdrSectionId);
    ULONG   DbgPrint(const char *Format, ...);
    VOID    DbgPrint2(ULONG Mask, ULONG Level, const char *File, ULONG Line, char *Format, ...);
    VOID    DebugDumpBuffer(ULONG Mask, PVOID Buffer, ULONG Length);
    VOID    DebugDisableScreenPort(VOID);
    VOID    DbgParseDebugChannels(PCHAR Value);

    #define ERR_LEVEL      0x1
    #define FIXME_LEVEL    0x2
    #define WARN_LEVEL     0x4
    #define TRACE_LEVEL    0x8

    #define MAX_LEVEL ERR_LEVEL | FIXME_LEVEL | WARN_LEVEL | TRACE_LEVEL

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
    #define DbgDumpBuffer(mask, buf, len)    DebugDumpBuffer(mask, buf, len)

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
#define    BREAKPOINT()                __asm__ ("int $3");
void    INSTRUCTION_BREAKPOINT1(unsigned long addr);
void    MEMORY_READWRITE_BREAKPOINT1(unsigned long addr);
void    MEMORY_WRITE_BREAKPOINT1(unsigned long addr);
void    INSTRUCTION_BREAKPOINT2(unsigned long addr);
void    MEMORY_READWRITE_BREAKPOINT2(unsigned long addr);
void    MEMORY_WRITE_BREAKPOINT2(unsigned long addr);
void    INSTRUCTION_BREAKPOINT3(unsigned long addr);
void    MEMORY_READWRITE_BREAKPOINT3(unsigned long addr);
void    MEMORY_WRITE_BREAKPOINT3(unsigned long addr);
void    INSTRUCTION_BREAKPOINT4(unsigned long addr);
void    MEMORY_READWRITE_BREAKPOINT4(unsigned long addr);
void    MEMORY_WRITE_BREAKPOINT4(unsigned long addr);

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

    #define DebugInit(FrLdrSectionId)
    #define BugCheck(fmt, ...)
    #define DbgDumpBuffer(mask, buf, len)
    #define DebugDisableScreenPort()
    #define DbgParseDebugChannels(val)

#endif // DBG

void
NTAPI
FrLdrBugCheck(ULONG BugCode);

VOID
FrLdrBugCheckWithMessage(
    ULONG BugCode,
    PCHAR File,
    ULONG Line,
    PSTR Format,
    ...);

/* Bugcheck codes */
enum _FRLDR_BUGCHECK_CODES
{
    TEST_BUGCHECK,
    MISSING_HARDWARE_REQUIREMENTS,
    FREELDR_IMAGE_CORRUPTION,
    MEMORY_INIT_FAILURE,
};

extern char *BugCodeStrings[];
extern ULONG_PTR BugCheckInfo[5];

#endif // defined __DEBUG_H
