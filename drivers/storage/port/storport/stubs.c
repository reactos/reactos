/*
 * PROJECT:     ReactOS Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Storport driver stub functions
 * COPYRIGHT:   Copyright 2017 Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

STORPORT_API
VOID
NTAPI
StorPortReadPortBufferUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Port,
    _In_ PUCHAR Buffer,
    _In_ ULONG Count)
{
    READ_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}


STORPORT_API
VOID
NTAPI
StorPortReadPortBufferUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Port,
    _In_ PULONG Buffer,
    _In_ ULONG Count)
{
    READ_PORT_BUFFER_ULONG(Port, Buffer, Count);
}


STORPORT_API
VOID
NTAPI
StorPortReadPortBufferUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Port,
    _In_ PUSHORT Buffer,
    _In_ ULONG Count)
{
    READ_PORT_BUFFER_USHORT(Port, Buffer, Count);
}


STORPORT_API
UCHAR
NTAPI
StorPortReadPortUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Port)
{
    return READ_PORT_UCHAR(Port);
}


STORPORT_API
ULONG
NTAPI
StorPortReadPortUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Port)
{
    return READ_PORT_ULONG(Port);
}


STORPORT_API
USHORT
NTAPI
StorPortReadPortUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Port)
{
    return READ_PORT_USHORT(Port);
}


STORPORT_API
VOID
NTAPI
StorPortReadRegisterBufferUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Register,
    _In_ PUCHAR Buffer,
    _In_ ULONG Count)
{
    READ_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}


STORPORT_API
VOID
NTAPI
StorPortReadRegisterBufferUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Register,
    _In_ PULONG Buffer,
    _In_ ULONG Count)
{
    READ_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}


STORPORT_API
VOID
NTAPI
StorPortReadRegisterBufferUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Register,
    _In_ PUSHORT Buffer,
    _In_ ULONG Count)
{
    READ_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}


STORPORT_API
UCHAR
NTAPI
StorPortReadRegisterUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Register)
{
    return READ_REGISTER_UCHAR(Register);
}


STORPORT_API
ULONG
NTAPI
StorPortReadRegisterUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Register)
{
    return READ_REGISTER_ULONG(Register);
}


STORPORT_API
USHORT
NTAPI
StorPortReadRegisterUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Register)
{
    return READ_REGISTER_USHORT(Register);
}


STORPORT_API
VOID
NTAPI
StorPortWritePortBufferUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Port,
    _In_ PUCHAR Buffer,
    _In_ ULONG Count)
{
    WRITE_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}


STORPORT_API
VOID
NTAPI
StorPortWritePortBufferUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Port,
    _In_ PULONG Buffer,
    _In_ ULONG Count)
{
    WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);
}


STORPORT_API
VOID
NTAPI
StorPortWritePortBufferUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Port,
    _In_ PUSHORT Buffer,
    _In_ ULONG Count)
{
    WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);
}


STORPORT_API
VOID
NTAPI
StorPortWritePortUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Port,
    _In_ UCHAR Value)
{
    WRITE_PORT_UCHAR(Port, Value);
}


STORPORT_API
VOID
NTAPI
StorPortWritePortUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Port,
    _In_ ULONG Value)
{
    WRITE_PORT_ULONG(Port, Value);
}


STORPORT_API
VOID
NTAPI
StorPortWritePortUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Port,
    _In_ USHORT Value)
{
    WRITE_PORT_USHORT(Port, Value);
}


STORPORT_API
VOID
NTAPI
StorPortWriteRegisterBufferUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Register,
    _In_ PUCHAR Buffer,
    _In_ ULONG Count)
{
    WRITE_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}


STORPORT_API
VOID
NTAPI
StorPortWriteRegisterBufferUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Register,
    _In_ PULONG Buffer,
    _In_ ULONG Count)
{
    WRITE_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}


STORPORT_API
VOID
NTAPI
StorPortWriteRegisterBufferUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Register,
    _In_ PUSHORT Buffer,
    _In_ ULONG Count)
{
    WRITE_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}


STORPORT_API
VOID
NTAPI
StorPortWriteRegisterUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Register,
    _In_ UCHAR Value)
{
    WRITE_REGISTER_UCHAR(Register, Value);
}


STORPORT_API
VOID
NTAPI
StorPortWriteRegisterUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Register,
    _In_ ULONG Value)
{
    WRITE_REGISTER_ULONG(Register, Value);
}


STORPORT_API
VOID
NTAPI
StorPortWriteRegisterUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Register,
    _In_ USHORT Value)
{
    WRITE_REGISTER_USHORT(Register, Value);
}

/* EOF */
