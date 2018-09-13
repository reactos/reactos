/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990  Microsoft Corporation

Module Name:

    memprint.h

Abstract:

    Include file for in-memory DbgPrint function.  Including this file
    will change DbgPrints to a routine which puts the display text in a
    circular buffer in memory.  By default, the text is then sent to the
    console via DbgPrint.  By changing the value of the MemPrintFlags
    flag, however, the text may be routed to a file instead, thereby
    significantly speeding up the DbgPrint operation.

Author:

    David Treadwell (davidtr) 05-Oct-1990

Revision History:

--*/

#ifndef _MEMPRINT_
#define _MEMPRINT_

#define MEM_PRINT_FLAG_CONSOLE     0x01
#define MEM_PRINT_FLAG_FILE        0x02
#define MEM_PRINT_FLAG_HEADER      0x04

extern ULONG MemPrintFlags;

#ifdef MIPS
#define MEM_PRINT_DEF_BUFFER_SIZE 16384
#else
#define MEM_PRINT_DEF_BUFFER_SIZE 65536
#endif

//
// The subbuffer count is the number of subbuffers within the circular
// buffer.  A subbuffer is the method used to buffer data between
// MemPrint and writing to disk--when a subbuffer is filled, its
// contents are written to the log file.  This value should be a power
// of two between two and sixty-four (two is necessary to allow writing
// to disk and RAM simultaneously, sixty-four is the maximum number of
// things a thread can wait on at once).
//
//

#define MEM_PRINT_DEF_SUBBUFFER_COUNT 16
#define MEM_PRINT_MAX_SUBBUFFER_COUNT 64

#define MEM_PRINT_LOG_FILE_NAME "\\SystemRoot\\Logfile"

//
// Exported routines.  MemPrintInitialize sets up the circular buffer
// and other memory, MemPrint writes text to the console and/or a
// log file, and MemPrintFlush writes the current subbuffer to disk
// whether or not it is full.
//

VOID
MemPrintInitialize (
    VOID
    );

VOID
MemPrint (
    CHAR *Format, ...
    );

VOID
MemPrintFlush (
    VOID
    );

#define DbgPrint MemPrint

#endif // def _MEMPRINT_
