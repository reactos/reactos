/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Storage Stack
 * FILE:            drivers/storage/scsiport/stubs.c
 * PURPOSE:         SCSI port driver
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "scsiport.h"

#include <srb.h>

#define NDEBUG
#include <debug.h>

#undef ScsiPortReadPortBufferUchar
#undef ScsiPortReadPortBufferUshort
#undef ScsiPortReadPortBufferUlong
#undef ScsiPortReadPortUchar
#undef ScsiPortReadPortUshort
#undef ScsiPortReadPortUlong
#undef ScsiPortReadRegisterBufferUchar
#undef ScsiPortReadRegisterBufferUshort
#undef ScsiPortReadRegisterBufferUlong
#undef ScsiPortReadRegisterUchar
#undef ScsiPortReadRegisterUshort
#undef ScsiPortReadRegisterUlong
#undef ScsiPortWritePortBufferUchar
#undef ScsiPortWritePortBufferUshort
#undef ScsiPortWritePortBufferUlong
#undef ScsiPortWritePortUchar
#undef ScsiPortWritePortUshort
#undef ScsiPortWritePortUlong
#undef ScsiPortWriteRegisterBufferUchar
#undef ScsiPortWriteRegisterBufferUshort
#undef ScsiPortWriteRegisterBufferUlong
#undef ScsiPortWriteRegisterUchar
#undef ScsiPortWriteRegisterUshort
#undef ScsiPortWriteRegisterUlong

SCSI_PHYSICAL_ADDRESS
NTAPI
ScsiPortConvertUlongToPhysicalAddress(
    IN ULONG_PTR UlongAddress)
{
    SCSI_PHYSICAL_ADDRESS Address;

    Address.QuadPart = UlongAddress;
    return Address;
}

VOID
NTAPI
ScsiPortReadPortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count)
{
    READ_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}

VOID
NTAPI
ScsiPortReadPortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG   Count)
{
    READ_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

VOID
NTAPI
ScsiPortReadPortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG  Count)
{
    READ_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

UCHAR
NTAPI
ScsiPortReadPortUchar(
    IN PUCHAR Port)
{
    return READ_PORT_UCHAR(Port);
}

USHORT
NTAPI
ScsiPortReadPortUshort(
    IN PUSHORT Port)
{
    return READ_PORT_USHORT(Port);
}

ULONG
NTAPI
ScsiPortReadPortUlong(
    IN PULONG Port)
{
    return READ_PORT_ULONG(Port);
}

VOID
NTAPI
ScsiPortReadRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Count)
{
    READ_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}

VOID
NTAPI
ScsiPortReadRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count)
{
    READ_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}

VOID
NTAPI
ScsiPortReadRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count)
{
    READ_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}

UCHAR
NTAPI
ScsiPortReadRegisterUchar(
    IN PUCHAR Register)
{
    return READ_REGISTER_UCHAR(Register);
}

USHORT
NTAPI
ScsiPortReadRegisterUshort(
    IN PUSHORT Register)
{
    return READ_REGISTER_USHORT(Register);
}

ULONG
NTAPI
ScsiPortReadRegisterUlong(
    IN PULONG Register)
{
    return READ_REGISTER_ULONG(Register);
}

VOID
NTAPI
ScsiPortWritePortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG Count)
{
    WRITE_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}

VOID
NTAPI
ScsiPortWritePortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count)
{
    WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

VOID
NTAPI
ScsiPortWritePortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count)
{
    WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

VOID
NTAPI
ScsiPortWritePortUchar(
    IN PUCHAR Port,
    IN UCHAR Value)
{
    WRITE_PORT_UCHAR(Port, Value);
}

VOID
NTAPI
ScsiPortWritePortUshort(
    IN PUSHORT Port,
    IN USHORT Value)
{
    WRITE_PORT_USHORT(Port, Value);
}

VOID
NTAPI
ScsiPortWritePortUlong(
    IN PULONG Port,
    IN ULONG Value)
{
    WRITE_PORT_ULONG(Port, Value);
}

VOID
NTAPI
ScsiPortWriteRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Count)
{
    WRITE_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}

VOID
NTAPI
ScsiPortWriteRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count)
{
    WRITE_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}

VOID
NTAPI
ScsiPortWriteRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count)
{
    WRITE_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}

VOID
NTAPI
ScsiPortWriteRegisterUchar(
    IN PUCHAR  Register,
    IN UCHAR  Value)
{
    WRITE_REGISTER_UCHAR(Register, Value);
}

VOID
NTAPI
ScsiPortWriteRegisterUshort(
    IN PUSHORT Register,
    IN USHORT Value)
{
    WRITE_REGISTER_USHORT(Register, Value);
}

VOID
NTAPI
ScsiPortWriteRegisterUlong(
    IN PULONG Register,
    IN ULONG Value)
{
    WRITE_REGISTER_ULONG(Register, Value);
}
