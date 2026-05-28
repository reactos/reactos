/*
 * PROJECT:     ReactOS runtime library (RTL)
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of processor related routines
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/


BOOLEAN
NTAPI
RtlIsProcessorFeaturePresent(
    _In_ ULONG ProcessorFeature)
{
    if (ProcessorFeature >= RTL_NUMBER_OF(SharedUserData->ProcessorFeatures))
    {
        return FALSE;
    }

    return SharedUserData->ProcessorFeatures[ProcessorFeature] != 0;
}

ULONG
NTAPI
RtlGetCurrentProcessorNumber(
    VOID)
{
    return NtGetCurrentProcessorNumber();
}

VOID
NTAPI
RtlGetCurrentProcessorNumberEx(
    _Out_ PPROCESSOR_NUMBER ProcessorNumber)
{
    NtGetCurrentProcessorNumberEx(ProcessorNumber);
}
