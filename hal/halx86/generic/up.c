/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Core source file for UP alternative functions
 * COPYRIGHT:  Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
HalRequestIpi(KAFFINITY TargetProcessors)
{
    /* This should never be called in UP mode */
    __debugbreak();
}

BOOLEAN
NTAPI
HalStartNextProcessor(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PKPROCESSOR_STATE ProcessorState)
{
    /* Always return false on UP systems */
    return FALSE;
}
