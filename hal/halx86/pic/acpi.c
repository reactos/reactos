/*
 * PROJECT:         ReactOS HAL
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            hal/halx86/pic/acpi.c
 * PURPOSE:         ACPI part PIC HALs code
 * PROGRAMMERS:     Copyright 2021 Vadim Galyant <vgal@rambler.ru>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
HalpAcpiApplyFadtSettings(_In_ PFADT Fadt)
{
    ;
}

/* EOF */
