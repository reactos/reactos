/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Core source file for Uniprocessor (UP) alternative functions
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
HalRequestIpi(
    _In_ KAFFINITY TargetProcessors)
{
    /* This should never be called in UP mode */
    __debugbreak();
}

BOOLEAN
NTAPI
HalStartNextProcessor(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ PKPROCESSOR_STATE ProcessorState)
{
    /* Always return false on UP systems */
    return FALSE;
}
