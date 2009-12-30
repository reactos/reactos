/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         See COPYING in the top level directory
 * FILE:            hal/halamd64/generic/x86bios.c
 * PURPOSE:         
 * PROGRAMMERS:     
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
//#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
x86BiosAllocateBuffer (
    ULONG *Size,
    USHORT *Segment,
    USHORT *Offset)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;;

}

BOOLEAN
NTAPI
x86BiosCall (
    ULONG InterruptNumber,
    X86_BIOS_REGISTERS *Registers)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
NTAPI
x86BiosFreeBuffer (
    USHORT Segment,
    USHORT Offset)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;;
}

NTSTATUS
x86BiosReadMemory (
    USHORT Segment,
    USHORT Offset,
    PVOID Buffer,
    ULONG Size)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;;
}

NTSTATUS
NTAPI
x86BiosWriteMemory (
    USHORT Segment,
    USHORT Offset,
    PVOID Buffer,
    ULONG Size)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;;
}

