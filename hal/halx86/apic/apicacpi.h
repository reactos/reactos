/*
 * PROJECT:         ReactOS HAL
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            hal/halx86/apic/apicacpi.h
 * PURPOSE:         ACPI header for APIC HALs 
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

#ifndef _APICACPI_H_
#define _APICACPI_H_

#define MAX_CPUS 32
#define MAX_IOAPICS 64
#define MAX_INTI 0x800

#define LOCAL_APIC_VERSION_MAX 0x1F

typedef struct _HALP_MP_INFO_TABLE
{
    ULONG LocalApicversion;
    ULONG ProcessorCount;
    ULONG ActiveProcessorCount;
    ULONG Reserved1;
    ULONG IoApicCount;
    ULONG Reserved2;
    ULONG Reserved3;
    BOOLEAN ImcrPresent;      // When the IMCR presence bit is set, the IMCR is present and PIC Mode is implemented; otherwise, Virtual Wire Mode is implemented.
    UCHAR Pad[3];
    ULONG LocalApicPA;        // The 32-bit physical address at which each processor can access its local interrupt controller
    ULONG IoApicVA[MAX_IOAPICS];
    ULONG IoApicPA[MAX_IOAPICS];
    ULONG IoApicIrqBase[MAX_IOAPICS];  // Global system interrupt base 

} HALP_MP_INFO_TABLE, *PHALP_MP_INFO_TABLE;

typedef union _IO_APIC_VERSION_REGISTER
{
    struct {
        UCHAR ApicVersion;
        UCHAR Reserved0;
        UCHAR MaxRedirectionEntry;
        UCHAR Reserved2;
    };
    ULONG AsULONG;

} IO_APIC_VERSION_REGISTER, *PIO_APIC_VERSION_REGISTER;

typedef struct _IO_APIC_REGISTERS
{
    volatile ULONG IoRegisterSelect;
    volatile ULONG Reserved[3];
    volatile ULONG IoWindow;

} IO_APIC_REGISTERS, *PIO_APIC_REGISTERS;

typedef union _APIC_INTI_INFO
{
    struct
    {
        UCHAR Enabled      :1;
        UCHAR Type         :3;
        UCHAR TriggerMode  :2;
        UCHAR Polarity     :2;
        UCHAR Destinations;
        USHORT Entry;
    };
    ULONG AsULONG;

} APIC_INTI_INFO, *PAPIC_INTI_INFO;

#include <pshpack1.h>
typedef struct _LOCAL_APIC
{
    UCHAR ProcessorId;
    UCHAR Id;
    UCHAR ProcessorNumber;
    BOOLEAN ProcessorStarted;
    BOOLEAN FirstProcessor;

} LOCAL_APIC, *PLOCAL_APIC;
#define LOCAL_APIC_SIZE sizeof(LOCAL_APIC)
#include <poppack.h>

#include <pshpack1.h>
typedef struct _APIC_ADDRESS_USAGE
{
    struct _HalAddressUsage * Next;
    CM_RESOURCE_TYPE Type;
    UCHAR Flags;
    struct
    {
        ULONG Start;
        ULONG Length;
    } Element[MAX_IOAPICS + 2]; // Local APIC + null-end element

} APIC_ADDRESS_USAGE, *PAPIC_ADDRESS_USAGE;
#define APIC_ADDRESS_USAGE_SIZE sizeof(APIC_ADDRESS_USAGE)
#include <poppack.h>

#endif /* !_APICACPI_H_ */
