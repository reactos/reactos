/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    kdfuncs.h

Abstract:

    Function definitions for the Kernel Debugger.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _KDFUNCS_H
#define _KDFUNCS_H

//
// Dependencies
//
#include "kdtypes.h"

//
// Port Functions
//
UCHAR
NTAPI
KdPollBreakIn(VOID);

BOOLEAN
NTAPI
KdPortInitialize(
    PKD_PORT_INFORMATION PortInformation,
    ULONG Unknown1,
    ULONG Unknown2
);

BOOLEAN
NTAPI
KdPortInitializeEx(
    PKD_PORT_INFORMATION PortInformation,
    ULONG Unknown1,
    ULONG Unknown2
);

BOOLEAN
NTAPI
KdPortGetByte(
    PUCHAR ByteRecieved
);

BOOLEAN
NTAPI
KdPortGetByteEx(
    PKD_PORT_INFORMATION PortInformation,
    PUCHAR ByteRecieved
);

BOOLEAN
NTAPI
KdPortPollByte(
    PUCHAR ByteRecieved
);

BOOLEAN
NTAPI
KdPortPollByteEx(
    PKD_PORT_INFORMATION PortInformation,
    PUCHAR ByteRecieved
);

VOID
NTAPI
KdPortPutByte(
    UCHAR ByteToSend
);

VOID
NTAPI
KdPortPutByteEx(
    PKD_PORT_INFORMATION PortInformation,
    UCHAR ByteToSend
);

VOID
NTAPI
KdPortRestore(VOID);

VOID
NTAPI
KdPortSave (VOID);

BOOLEAN
NTAPI
KdPortDisableInterrupts(VOID);

BOOLEAN
NTAPI
KdPortEnableInterrupts(VOID);

#endif
