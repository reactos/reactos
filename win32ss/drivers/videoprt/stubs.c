/*
 * VideoPort driver
 *
 * Copyright (C) 2002-2004, 2007 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "videoprt.h"

#define NDEBUG
#include <debug.h>

VP_STATUS
NTAPI
VideoPortFlushRegistry(
    PVOID HwDeviceExtension)
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
NTAPI
VideoPortGetAssociatedDeviceID(
    IN PVOID DeviceObject)
{
    UNIMPLEMENTED;
    return 0;
}


ULONG
NTAPI
VideoPortGetBytesUsed(
    IN PVOID HwDeviceExtension,
    IN PDMA pDma)
{
    UNIMPLEMENTED;
    return 0;
}

PVOID
NTAPI
VideoPortGetMdl(
    IN PVOID HwDeviceExtension,
    IN PDMA pDma)
{
    UNIMPLEMENTED;
    return 0;
}

LONG
NTAPI
VideoPortReadStateEvent(
    IN PVOID HwDeviceExtension,
    IN PEVENT pEvent)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
NTAPI
VideoPortSetBytesUsed(
    IN PVOID HwDeviceExtension,
    IN OUT PDMA pDma,
    IN ULONG BytesUsed)
{
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
VideoPortUnlockPages(
    IN PVOID hwDeviceExtension,
    IN PDMA pDma)
{
    UNIMPLEMENTED;
    return 0;
}

_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
WdDdiWatchdogDpcCallback(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    UNIMPLEMENTED;
}

#ifdef _M_AMD64
UCHAR
NTAPI
VideoPortReadPortUchar(
    PUCHAR Port)
{
    return READ_PORT_UCHAR(Port);
}

USHORT
NTAPI
VideoPortReadPortUshort(
    PUSHORT Port)
{
    return READ_PORT_USHORT(Port);
}

ULONG
NTAPI
VideoPortReadPortUlong(
    PULONG Port)
{
    return READ_PORT_ULONG(Port);
}

VOID
NTAPI
VideoPortReadPortBufferUchar(
    PUCHAR Port,
    PUCHAR Buffer,
    ULONG Count)
{
    READ_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}

VOID
NTAPI
VideoPortReadPortBufferUshort(
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG Count)
{
    READ_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

VOID
NTAPI
VideoPortReadPortBufferUlong(
    PULONG Port,
    PULONG Buffer,
    ULONG Count)
{
    READ_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

UCHAR
NTAPI
VideoPortReadRegisterUchar(
    PUCHAR Register)
{
    return READ_REGISTER_UCHAR(Register);
}

USHORT
NTAPI
VideoPortReadRegisterUshort(
    PUSHORT Register)
{
    return READ_REGISTER_USHORT(Register);
}

ULONG
NTAPI
VideoPortReadRegisterUlong(
    PULONG Register)
{
    return READ_REGISTER_ULONG(Register);
}

VOID
NTAPI
VideoPortReadRegisterBufferUchar(
    PUCHAR Register,
    PUCHAR Buffer,
    ULONG Count)
{
    READ_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}

VOID
NTAPI
VideoPortReadRegisterBufferUshort(
    PUSHORT Register,
    PUSHORT Buffer,
    ULONG Count)
{
    READ_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}

VOID
NTAPI
VideoPortReadRegisterBufferUlong(
    PULONG Register,
    PULONG Buffer,
    ULONG Count)
{
    READ_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}

VOID
NTAPI
VideoPortWritePortUchar(
    PUCHAR Port,
    UCHAR Value)
{
    WRITE_PORT_UCHAR(Port, Value);
}

VOID
NTAPI
VideoPortWritePortUshort(
    PUSHORT Port,
    USHORT Value)
{
    WRITE_PORT_USHORT(Port, Value);
}

VOID
NTAPI
VideoPortWritePortUlong(
    PULONG Port,
    ULONG Value)
{
    WRITE_PORT_ULONG(Port, Value);
}

VOID
NTAPI
VideoPortWritePortBufferUchar(
    PUCHAR Port,
    PUCHAR Buffer,
    ULONG Count)
{
    WRITE_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}

VOID
NTAPI
VideoPortWritePortBufferUshort(
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG Count)
{
    WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

VOID
NTAPI
VideoPortWritePortBufferUlong(
    PULONG Port,
    PULONG Buffer,
    ULONG Count)
{
    WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

VOID
NTAPI
VideoPortWriteRegisterUchar(
    PUCHAR Register,
    UCHAR Value)
{
    WRITE_REGISTER_UCHAR(Register, Value);
}

VOID
NTAPI
VideoPortWriteRegisterUshort(
    PUSHORT Register,
    USHORT Value)
{
    WRITE_REGISTER_USHORT(Register, Value);
}

VOID
NTAPI
VideoPortWriteRegisterUlong(
    PULONG Register,
    ULONG Value)
{
    WRITE_REGISTER_ULONG(Register, Value);
}

VOID
NTAPI
VideoPortWriteRegisterBufferUchar(
    PUCHAR Register,
    PUCHAR Buffer,
    ULONG Count)
{
    WRITE_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}

VOID
NTAPI
VideoPortWriteRegisterBufferUshort(
    PUSHORT Register,
    PUSHORT Buffer,
    ULONG Count)
{
    WRITE_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}

VOID
NTAPI
VideoPortWriteRegisterBufferUlong(
    PULONG Register,
    PULONG Buffer,
    ULONG Count)
{
    WRITE_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}

LONG
FASTCALL
VideoPortInterlockedDecrement(
    IN PLONG Addend)
{
    return _InterlockedDecrement(Addend);
}

LONG
FASTCALL
VideoPortInterlockedIncrement(
    IN PLONG Addend)
{
    return _InterlockedIncrement(Addend);
}

LONG
FASTCALL
VideoPortInterlockedExchange(
    IN OUT PLONG Target,
    IN LONG Value)
{
    return InterlockedExchange(Target, Value);
}

VOID
NTAPI
VideoPortQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime)
{
    KeQuerySystemTime(CurrentTime);
}

#endif /* _M_AMD64 */
