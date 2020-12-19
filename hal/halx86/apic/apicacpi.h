/*
 * PROJECT:         ReactOS HAL
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            hal/halx86/apic/apicacpi.h
 * PURPOSE:         ACPI header for APIC HALs 
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

#ifndef _APICACPI_H_
#define _APICACPI_H_

#define MAX_IOAPIC 64

typedef struct _HALP_MP_INFO_TABLE
{
    ULONG LocalApicversion;
    ULONG ProcessorCount;
    ULONG ActiveProcessorCount;
    ULONG Reserved1;
    ULONG IoApicCount;
    ULONG Reserved2;
    ULONG Reserved3;
    BOOLEAN ImcrPresent; // When the IMCR presence bit is set, the IMCR is present and PIC Mode is implemented; otherwise, Virtual Wire Mode is implemented.
    UCHAR Pad[3];
    ULONG LocalApicPA; // The 32-bit physical address at which each processor can access its local interrupt controller
    ULONG IoApicVA[MAX_IOAPIC];
    ULONG IoApicPA[MAX_IOAPIC];
    ULONG IoApicIrqBase[MAX_IOAPIC]; // Global system interrupt base 

} HALP_MP_INFO_TABLE, *PHALP_MP_INFO_TABLE;


#endif /* !_APICACPI_H_ */
