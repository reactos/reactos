/*
 * PROJECT:         ReactOS HAL
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            hal/halx86/apic/apicmps.c
 * PURPOSE:         MPS part APIC HALs code
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#include "apic.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI 
DetectMP(_In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    // FIXME UNIMPLIMENTED;
    ASSERT(FALSE);
    return FALSE;
}

/* EOF */
