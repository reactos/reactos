/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    machine.h

Abstract:

    This is the include file that defines detect of machine type.

Author:

    kazum    10-Aug-1994

Revision History:

--*/

#ifndef _MACHINE_ID_
#define _MACHINE_ID_

#if defined(i386)
//
// These definition is only for Intel platform.
//
//
// Hardware platform ID
//

#define PC_AT_COMPATIBLE      0x00000000
#define PC_9800_COMPATIBLE    0x00000001
#define FMR_COMPATIBLE        0x00000002

//
// NT Vendor ID
//

#define NT_MICROSOFT          0x00010000
#define NT_NEC                0x00020000
#define NT_FUJITSU            0x00040000

//
// Vendor/Machine IDs
//
// DWORD MachineID
//
// 31           15             0
// +-------------+-------------+
// |  Vendor ID  | Platform ID |
// +-------------+-------------+
//

#define MACHINEID_MS_PCAT     (NT_MICROSOFT|PC_AT_COMPATIBLE)
#define MACHINEID_MS_PC98     (NT_MICROSOFT|PC_9800_COMPATIBLE)
#define MACHINEID_NEC_PC98    (NT_NEC      |PC_9800_COMPATIBLE)
#define MACHINEID_FUJITSU_FMR (NT_FUJITSU  |FMR_COMPATIBLE)

//
// Build 683 compatibility.
//
// !!! should be removed.

#define MACHINEID_MICROSOFT   MACHINEID_MS_PCAT

//
// Macros
//

#define ISNECPC98(x)    (x == MACHINEID_NEC_PC98)
#define ISFUJITSUFMR(x) (x == MACHINEID_FUJITSU_FMR)
#define ISMICROSOFT(x)  (x == MACHINEID_MS_PCAT)

//
// Functions.
//

//
// User mode ( NT API )
//

LONG
NtGetMachineIdentifierValue(
    IN OUT PULONG Value
    );

//
// User mode ( Win32 API )
//

LONG
RegGetMachineIdentifierValue(
    IN OUT PULONG Value
    );

#endif // defined(i386)
#endif // _MACHINE_ID_
