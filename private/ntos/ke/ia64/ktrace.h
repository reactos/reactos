/*++

Copyright (c) 1996  Intel Corporation

Module Name:

    ktrace.h

Abstract:

    This module is an include file for use by all clients of the
    KTrace utility.

    By default, all kernel modules which have this module automatically
    included since an include for this file will exist in kx<arch>.h

Author:

    Roy D'Souza (rdsouza@gomez) 22-April-1996

Environment:

    User or Kernel mode.

Revision History:

--*/

#ifndef _KTRACE_H_
#define _KTRACE_H_

/***********************************************************************
Message Types:
***********************************************************************/

#define MESSAGE_INFORMATION 0x1
#define MESSAGE_WARNING     0x2
#define MESSAGE_ERROR       0x4

/***********************************************************************
Module IDs:
***********************************************************************/

#define MODULE_INIT  0x1
#define MODULE_KE    0x2
#define MODULE_EX    0x4
#define MODULE_MM    0x8
#define MODULE_LPC   0x10
#define MODULE_SE    0x20
#define MODULE_TDI   0x40
#define MODULE_RTL   0x80
#define MODULE_PO    0x100
#define MODULE_PNP   0x200

#define DRIVER_1    0x10000000
#define DRIVER_2    0x20000000
#define DRIVER_3    0x40000000
#define DRIVER_4    0x80000000

/***********************************************************************
Prototypes:
***********************************************************************/

//
// Add an entry. Routine will determine current processor.
//

#if DBG
NTSTATUS
KeAddKTrace (
    ULONG  ModuleID,
    USHORT MessageType,
    USHORT MessageIndex,
    ULONGLONG Arg1,
    ULONGLONG Arg2,
    ULONGLONG Arg3,
    ULONGLONG Arg4
    );
#else
#define KeAddKTrace
#endif


//
// Selectively dump trace.
//
#if DBG
ULONG
__stdcall
KeDumpKTrace (
    ULONG ProcessorNumber,
    ULONG StartEntry,
    ULONG NumberOfEntries,
    ULONGLONG ModuleFilter,
    ULONGLONG MessageFilter,
    BOOLEAN Sort);
#else
#define KeDumpKTrace
#endif

//
// Selectively permit kernel modules to write to trace.
//
#if DBG
VOID
__stdcall
KeEnableKTrace (
        ULONG IDMask
    );
#else
#define KeEnableKTrace
#endif

#if DBG
VOID
NTAPI // __stdcall
DumpRecord (IN ULONG ProcessorNumber,
            IN ULONG Index);
#else
#define DumpRecord
#endif

#endif // _KTRACE_H_

