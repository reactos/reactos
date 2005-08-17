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

BYTE
STDCALL
KdPollBreakIn(VOID);

BOOLEAN
STDCALL
KdPortInitialize(
    PKD_PORT_INFORMATION PortInformation,
    DWORD Unknown1,
    DWORD Unknown2
);

BOOLEAN
STDCALL
KdPortInitializeEx(
    PKD_PORT_INFORMATION PortInformation,
    DWORD Unknown1,
    DWORD Unknown2
);

BOOLEAN
STDCALL
KdPortGetByte(
    PUCHAR ByteRecieved
);

BOOLEAN
STDCALL
KdPortGetByteEx(
    PKD_PORT_INFORMATION PortInformation,
    PUCHAR ByteRecieved
);

BOOLEAN
STDCALL
KdPortPollByte(
    PUCHAR ByteRecieved
);

BOOLEAN
STDCALL
KdPortPollByteEx(
    PKD_PORT_INFORMATION PortInformation,
    PUCHAR ByteRecieved
);

VOID
STDCALL
KdPortPutByte(
    UCHAR ByteToSend
);

VOID
STDCALL
KdPortPutByteEx(
    PKD_PORT_INFORMATION PortInformation,
    UCHAR ByteToSend
);

VOID
STDCALL
KdPortRestore(VOID);

VOID
STDCALL
KdPortSave (VOID);

BOOLEAN
STDCALL
KdPortDisableInterrupts(VOID);

BOOLEAN
STDCALL
KdPortEnableInterrupts(VOID);

#endif
