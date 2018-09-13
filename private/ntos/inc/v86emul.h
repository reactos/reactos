/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    v86emul.h

Abstract:

    This module contains the V86 instruction emulator interface definitions
    used by kernel device drivers.

Author:

    Andre Vachon (andreva) 8-Jan-1992

Revision History:


--*/

#ifndef _V86EMUL_
#define _V86EMUL_


// begin_ntminiport

//
// Structures used by the kernel drivers to describe which ports must be
// hooked out directly from the V86 emulator to the driver.
//

typedef enum _EMULATOR_PORT_ACCESS_TYPE {
    Uchar,
    Ushort,
    Ulong
} EMULATOR_PORT_ACCESS_TYPE, *PEMULATOR_PORT_ACCESS_TYPE;

//
// Access Modes
//

#define EMULATOR_READ_ACCESS    0x01
#define EMULATOR_WRITE_ACCESS   0x02

typedef struct _EMULATOR_ACCESS_ENTRY {
    ULONG BasePort;
    ULONG NumConsecutivePorts;
    EMULATOR_PORT_ACCESS_TYPE AccessType;
    UCHAR AccessMode;
    UCHAR StringSupport;
    PVOID Routine;
} EMULATOR_ACCESS_ENTRY, *PEMULATOR_ACCESS_ENTRY;

// end_ntminiport

//
// These are the various function prototypes of the routines that are
// provided by the kernel driver to hook out access to io ports.
//

typedef
NTSTATUS
(*PDRIVER_IO_PORT_UCHAR ) (
    IN ULONG_PTR Context,
    IN ULONG Port,
    IN UCHAR AccessMode,
    IN OUT PUCHAR Data
    );

typedef
NTSTATUS
(*PDRIVER_IO_PORT_UCHAR_STRING ) (
    IN ULONG_PTR Context,
    IN ULONG Port,
    IN UCHAR AccessMode,
    IN OUT PUCHAR Data,
    IN ULONG DataLength
    );

typedef
NTSTATUS
(*PDRIVER_IO_PORT_USHORT ) (
    IN ULONG_PTR Context,
    IN ULONG Port,
    IN UCHAR AccessMode,
    IN OUT PUSHORT Data
    );

typedef
NTSTATUS
(*PDRIVER_IO_PORT_USHORT_STRING ) (
    IN ULONG_PTR Context,
    IN ULONG Port,
    IN UCHAR AccessMode,
    IN OUT PUSHORT Data,
    IN ULONG DataLength // number of words
    );

typedef
NTSTATUS
(*PDRIVER_IO_PORT_ULONG ) (
    IN ULONG_PTR Context,
    IN ULONG Port,
    IN UCHAR AccessMode,
    IN OUT PULONG Data
    );

typedef
NTSTATUS
(*PDRIVER_IO_PORT_ULONG_STRING ) (
    IN ULONG_PTR Context,
    IN ULONG Port,
    IN UCHAR AccessMode,
    IN OUT PULONG Data,
    IN ULONG DataLength  // number of dwords
    );

#endif // _V86EMUL_
