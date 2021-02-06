/*
 * PROJECT:         ReactOS HAL
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            hal/halx86/apic/apicacpi.h
 * PURPOSE:         ACPI header for APIC HALs 
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

#ifndef _APICACPI_H_
#define _APICACPI_H_

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

#define PIC_FLAGS_POLARITY_CONFORMS     0
#define PIC_FLAGS_POLARITY_ACTIVE_HIGH  1
#define PIC_FLAGS_POLARITY_RESERVED     2
#define PIC_FLAGS_POLARITY_ACTIVE_LOW   3
#define PIC_FLAGS_POLARITY_MASK         (3)

#define PIC_FLAGS_TRIGGER_CONFORMS      0
#define PIC_FLAGS_TRIGGER_EDGE          1
#define PIC_FLAGS_TRIGGER_RESERVED      2
#define PIC_FLAGS_TRIGGER_LEVEL         3
#define PIC_FLAGS_TRIGGER_MASK          (3)

typedef union _HAL_PIC_VECTOR_FLAGS
{
    struct
    {
        ULONG Polarity    :2;
        ULONG TriggerMode :2;
        ULONG Reserved    :28;
    };
    ULONG AsULONG;

} HAL_PIC_VECTOR_FLAGS, *PHAL_PIC_VECTOR_FLAGS;

#endif /* !_APICACPI_H_ */
