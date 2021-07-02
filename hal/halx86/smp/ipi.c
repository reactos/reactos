/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:        hal/halx86/smp/ipi.c
 * PURPOSE:     Core source file for IPI management
 * PROGRAMMERS:  Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include <smp.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

VOID
NTAPI
HalRequestIpi(KAFFINITY TargetProcessors)
{
    HalpRequestIpi(TargetProcessors);
}
