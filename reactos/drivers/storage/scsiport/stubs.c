/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Storage Stack
 * FILE:            drivers/storage/scsiport/stubs.c
 * PURPOSE:         SCSI port driver
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntddk.h>
#include <srb.h>

#define NDEBUG
#include <debug.h>

#ifdef _MSC_VER
  #define DDKAPI
#endif

SCSI_PHYSICAL_ADDRESS
DDKAPI
ScsiPortConvertUlongToPhysicalAddress(
    IN ULONG  UlongAddress)
{
    return RtlConvertUlongToLargeInteger(UlongAddress);
}

VOID
DDKAPI
ScsiPortReadPortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count)
{
    READ_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}

VOID
DDKAPI
ScsiPortReadPortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG   Count)
{
    READ_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

VOID
DDKAPI
ScsiPortReadPortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG  Count)
{
    READ_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

UCHAR
DDKAPI
ScsiPortReadPortUchar(
    IN PUCHAR Port)
{
    return READ_PORT_UCHAR(Port);
}

USHORT
DDKAPI
ScsiPortReadPortUshort(
    IN PUSHORT Port)
{
    return READ_PORT_USHORT(Port);
}

ULONG
DDKAPI
ScsiPortReadPortUlong(
    IN PULONG Port)
{
    return READ_PORT_ULONG(Port);
}

VOID
DDKAPI
ScsiPortReadRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Count)
{
    READ_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}

VOID
DDKAPI
ScsiPortReadRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count)
{
    READ_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}

VOID
DDKAPI
ScsiPortReadRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count)
{
    READ_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}

UCHAR
DDKAPI
ScsiPortReadRegisterUchar(
    IN PUCHAR Register)
{
    return READ_REGISTER_UCHAR(Register);
}

USHORT
DDKAPI
ScsiPortReadRegisterUshort(
    IN PUSHORT Register)
{
    return READ_REGISTER_USHORT(Register);
}

ULONG
DDKAPI
ScsiPortReadRegisterUlong(
    IN PULONG Register)
{
    return READ_REGISTER_ULONG(Register);
}

VOID
DDKAPI
ScsiPortWritePortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG Count)
{
    WRITE_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}

VOID
DDKAPI
ScsiPortWritePortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count)
{
    WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

VOID
DDKAPI
ScsiPortWritePortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count)
{
    WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

VOID
DDKAPI
ScsiPortWritePortUchar(
    IN PUCHAR Port,
    IN UCHAR Value)
{
    WRITE_PORT_UCHAR(Port, Value);
}

VOID
DDKAPI
ScsiPortWritePortUshort(
    IN PUSHORT Port,
    IN USHORT Value)
{
    WRITE_PORT_USHORT(Port, Value);
}

VOID
DDKAPI
ScsiPortWritePortUlong(
    IN PULONG Port,
    IN ULONG Value)
{
    WRITE_PORT_ULONG(Port, Value);
}

VOID
DDKAPI
ScsiPortWriteRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG Count)
{
    WRITE_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}

VOID
DDKAPI
ScsiPortWriteRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count)
{
    WRITE_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}

VOID
DDKAPI
ScsiPortWriteRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count)
{
    WRITE_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}

VOID
DDKAPI
ScsiPortWriteRegisterUchar(
    IN PUCHAR  Register,
    IN ULONG  Value)
{
    WRITE_REGISTER_UCHAR(Register, Value);
}

VOID
DDKAPI
ScsiPortWriteRegisterUshort(
    IN PUSHORT Register,
    IN USHORT Value)
{
    WRITE_REGISTER_USHORT(Register, Value);
}

VOID
DDKAPI
ScsiPortWriteRegisterUlong(
    IN PULONG Register,
    IN ULONG Value)
{
    WRITE_REGISTER_ULONG(Register, Value);
}

