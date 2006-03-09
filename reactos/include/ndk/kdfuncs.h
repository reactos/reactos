/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/kdfuncs.h
 * PURPOSE:         Prototypes for Kernel Debugger Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _KDFUNCS_H
#define _KDFUNCS_H

/* DEPENDENCIES **************************************************************/
#include "kdtypes.h"

/* PROTOTYPES ****************************************************************/

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
