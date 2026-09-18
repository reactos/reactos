/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     x2APIC detection and mode enable
 * COPYRIGHT:   Copyright 2026 Alex Mendoza <05alex.mendozaa@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include <x86x64/Cpuid.h>
#include "x2apicp.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/**
 * @brief
 * Checks whether the current processor supports x2APIC mode.
 *
 * @return
 * TRUE if CPUID.01H:ECX.x2APIC[bit 21] is set, FALSE otherwise.
 **/
BOOLEAN
NTAPI
X2ApicIsSupported(VOID)
{
    CPUID_VERSION_INFO_REGS VersionInfo;

    __cpuid(VersionInfo.AsInt32, CPUID_VERSION_INFO);

    return VersionInfo.Ecx.Bits.x2APIC != 0;
}


/**
 * @brief
 * Switches the current processor's local APIC into x2APIC mode.
 *
 * @remarks
 * This must only be called after verifying support with X2ApicIsSupported,
 * and before you try to do any MSR based register access.
 *
 * @return
 * None.
 **/
VOID
NTAPI
X2ApicEnable(VOID)
{
    X2APIC_BASE_ADDRESS_REGISTER BaseRegister;

    BaseRegister.LongLong = __readmsr(MSR_APIC_BASE);

    /* xAPIC (Enable) must already be set before x2APIC can be enabled */
    BaseRegister.Enable = 1;
    BaseRegister.EnableX2Apic = 1;

    __writemsr(MSR_APIC_BASE, BaseRegister.LongLong);
}
