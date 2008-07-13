/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/base/kdcom/arm/kdbg.c
 * PURPOSE:         Serial Port Kernel Debugging Transport Library
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#define NOEXTAPI
#include <ntddk.h>
#define NDEBUG
#include <halfuncs.h>
#include <stdio.h>
#include <debug.h>
#include "arc/arc.h"
#include "windbgkd.h"
#include <kddll.h>
#include <ioaccess.h>

/* GLOBALS ********************************************************************/

typedef struct _KD_PORT_INFORMATION
{
    ULONG ComPort;
    ULONG BaudRate;
    ULONG BaseAddress;
} KD_PORT_INFORMATION, *PKD_PORT_INFORMATION;

KD_PORT_INFORMATION DefaultPort = {0, 0, 0};

/* REACTOS FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
KdPortInitialize(IN PKD_PORT_INFORMATION PortInformation,
                 IN ULONG Unknown1,
                 IN ULONG Unknown2)
{
    UNIMPLEMENTED;
    while (TRUE);
    return TRUE;
}

BOOLEAN
NTAPI
KdPortInitializeEx(IN PKD_PORT_INFORMATION PortInformation,
                   IN ULONG Unknown1,
                   IN ULONG Unknown2)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
NTAPI
KdPortGetByteEx(IN PKD_PORT_INFORMATION PortInformation,
                OUT PUCHAR ByteReceived)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

BOOLEAN
NTAPI
KdPortGetByte(OUT PUCHAR ByteReceived)
{
    return KdPortGetByteEx(&DefaultPort, ByteReceived); 
}

BOOLEAN
NTAPI
KdPortPollByteEx(IN PKD_PORT_INFORMATION PortInformation,
                 OUT PUCHAR ByteReceived)
{
    UNIMPLEMENTED;
    while (TRUE);
    return TRUE;
}

BOOLEAN
NTAPI
KdPortPollByte(OUT PUCHAR ByteReceived)
{
    return KdPortPollByteEx(&DefaultPort, ByteReceived);
}

VOID
NTAPI
KdPortPutByteEx(IN PKD_PORT_INFORMATION PortInformation,
                IN UCHAR ByteToSend)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
KdPortPutByte(IN UCHAR ByteToSend)
{
    KdPortPutByteEx(&DefaultPort, ByteToSend);
}

VOID
NTAPI
KdPortRestore(VOID)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
KdPortSave(VOID)
{
    UNIMPLEMENTED;
    while (TRUE);
}

BOOLEAN
NTAPI
KdPortDisableInterrupts(VOID)
{
    UNIMPLEMENTED;
    while (TRUE);
    return TRUE;
}

BOOLEAN
NTAPI
KdPortEnableInterrupts(VOID)
{
    UNIMPLEMENTED;
    while (TRUE);
    return TRUE;
}

/* WINDOWS FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
KdDebuggerInitialize0(IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
KdDebuggerInitialize1(IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
KdSave(IN BOOLEAN SleepTransition)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdRestore(IN BOOLEAN SleepTransition)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

VOID
NTAPI
KdSendPacket(IN ULONG PacketType,
             IN PSTRING MessageHeader,
             IN PSTRING MessageData,
             IN OUT PKD_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
    return;
}

KDSTATUS
NTAPI
KdReceivePacket(IN ULONG PacketType,
                OUT PSTRING MessageHeader,
                OUT PSTRING MessageData,
                OUT PULONG DataLength,
                IN OUT PKD_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

/* EOF */
