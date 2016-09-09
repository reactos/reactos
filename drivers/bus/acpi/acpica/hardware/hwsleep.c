/******************************************************************************
 *
 * Name: hwsleep.c - ACPI Hardware Sleep/Wake Support functions for the
 *                   original/legacy sleep/PM registers.
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include "acpi.h"
#include "accommon.h"

#define _COMPONENT          ACPI_HARDWARE
        ACPI_MODULE_NAME    ("hwsleep")


#if (!ACPI_REDUCED_HARDWARE) /* Entire module */
/*******************************************************************************
 *
 * FUNCTION:    AcpiHwLegacySleep
 *
 * PARAMETERS:  SleepState          - Which sleep state to enter
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enter a system sleep state via the legacy FADT PM registers
 *              THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwLegacySleep (
    UINT8                   SleepState)
{
    ACPI_BIT_REGISTER_INFO  *SleepTypeRegInfo;
    ACPI_BIT_REGISTER_INFO  *SleepEnableRegInfo;
    UINT32                  Pm1aControl;
    UINT32                  Pm1bControl;
    UINT32                  InValue;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (HwLegacySleep);


    SleepTypeRegInfo = AcpiHwGetBitRegisterInfo (ACPI_BITREG_SLEEP_TYPE);
    SleepEnableRegInfo = AcpiHwGetBitRegisterInfo (ACPI_BITREG_SLEEP_ENABLE);

    /* Clear wake status */

    Status = AcpiWriteBitRegister (ACPI_BITREG_WAKE_STATUS,
        ACPI_CLEAR_STATUS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Clear all fixed and general purpose status bits */

    Status = AcpiHwClearAcpiStatus ();
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * 1) Disable/Clear all GPEs
     * 2) Enable all wakeup GPEs
     */
    Status = AcpiHwDisableAllGpes ();
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }
    AcpiGbl_SystemAwakeAndRunning = FALSE;

    Status = AcpiHwEnableAllWakeupGpes ();
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Get current value of PM1A control */

    Status = AcpiHwRegisterRead (ACPI_REGISTER_PM1_CONTROL,
        &Pm1aControl);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }
    ACPI_DEBUG_PRINT ((ACPI_DB_INIT,
        "Entering sleep state [S%u]\n", SleepState));

    /* Clear the SLP_EN and SLP_TYP fields */

    Pm1aControl &= ~(SleepTypeRegInfo->AccessBitMask |
         SleepEnableRegInfo->AccessBitMask);
    Pm1bControl = Pm1aControl;

    /* Insert the SLP_TYP bits */

    Pm1aControl |= (AcpiGbl_SleepTypeA << SleepTypeRegInfo->BitPosition);
    Pm1bControl |= (AcpiGbl_SleepTypeB << SleepTypeRegInfo->BitPosition);

    /*
     * We split the writes of SLP_TYP and SLP_EN to workaround
     * poorly implemented hardware.
     */

    /* Write #1: write the SLP_TYP data to the PM1 Control registers */

    Status = AcpiHwWritePm1Control (Pm1aControl, Pm1bControl);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Insert the sleep enable (SLP_EN) bit */

    Pm1aControl |= SleepEnableRegInfo->AccessBitMask;
    Pm1bControl |= SleepEnableRegInfo->AccessBitMask;

    /* Flush caches, as per ACPI specification */

    ACPI_FLUSH_CPU_CACHE ();

    /* Write #2: Write both SLP_TYP + SLP_EN */

    Status = AcpiHwWritePm1Control (Pm1aControl, Pm1bControl);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (SleepState > ACPI_STATE_S3)
    {
        /*
         * We wanted to sleep > S3, but it didn't happen (by virtue of the
         * fact that we are still executing!)
         *
         * Wait ten seconds, then try again. This is to get S4/S5 to work on
         * all machines.
         *
         * We wait so long to allow chipsets that poll this reg very slowly
         * to still read the right value. Ideally, this block would go
         * away entirely.
         */
        AcpiOsStall (10 * ACPI_USEC_PER_SEC);

        Status = AcpiHwRegisterWrite (ACPI_REGISTER_PM1_CONTROL,
            SleepEnableRegInfo->AccessBitMask);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /* Wait for transition back to Working State */

    do
    {
        Status = AcpiReadBitRegister (ACPI_BITREG_WAKE_STATUS, &InValue);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

    } while (!InValue);

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiHwLegacyWakePrep
 *
 * PARAMETERS:  SleepState          - Which sleep state we just exited
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform the first state of OS-independent ACPI cleanup after a
 *              sleep.
 *              Called with interrupts ENABLED.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwLegacyWakePrep (
    UINT8                   SleepState)
{
    ACPI_STATUS             Status;
    ACPI_BIT_REGISTER_INFO  *SleepTypeRegInfo;
    ACPI_BIT_REGISTER_INFO  *SleepEnableRegInfo;
    UINT32                  Pm1aControl;
    UINT32                  Pm1bControl;


    ACPI_FUNCTION_TRACE (HwLegacyWakePrep);

    /*
     * Set SLP_TYPE and SLP_EN to state S0.
     * This is unclear from the ACPI Spec, but it is required
     * by some machines.
     */
    Status = AcpiGetSleepTypeData (ACPI_STATE_S0,
        &AcpiGbl_SleepTypeA, &AcpiGbl_SleepTypeB);
    if (ACPI_SUCCESS (Status))
    {
        SleepTypeRegInfo =
            AcpiHwGetBitRegisterInfo (ACPI_BITREG_SLEEP_TYPE);
        SleepEnableRegInfo =
            AcpiHwGetBitRegisterInfo (ACPI_BITREG_SLEEP_ENABLE);

        /* Get current value of PM1A control */

        Status = AcpiHwRegisterRead (ACPI_REGISTER_PM1_CONTROL,
            &Pm1aControl);
        if (ACPI_SUCCESS (Status))
        {
            /* Clear the SLP_EN and SLP_TYP fields */

            Pm1aControl &= ~(SleepTypeRegInfo->AccessBitMask |
                SleepEnableRegInfo->AccessBitMask);
            Pm1bControl = Pm1aControl;

            /* Insert the SLP_TYP bits */

            Pm1aControl |= (AcpiGbl_SleepTypeA <<
                SleepTypeRegInfo->BitPosition);
            Pm1bControl |= (AcpiGbl_SleepTypeB <<
                SleepTypeRegInfo->BitPosition);

            /* Write the control registers and ignore any errors */

            (void) AcpiHwWritePm1Control (Pm1aControl, Pm1bControl);
        }
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiHwLegacyWake
 *
 * PARAMETERS:  SleepState          - Which sleep state we just exited
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform OS-independent ACPI cleanup after a sleep
 *              Called with interrupts ENABLED.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwLegacyWake (
    UINT8                   SleepState)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (HwLegacyWake);


    /* Ensure EnterSleepStatePrep -> EnterSleepState ordering */

    AcpiGbl_SleepTypeA = ACPI_SLEEP_TYPE_INVALID;
    AcpiHwExecuteSleepMethod (METHOD_PATHNAME__SST, ACPI_SST_WAKING);

    /*
     * GPEs must be enabled before _WAK is called as GPEs
     * might get fired there
     *
     * Restore the GPEs:
     * 1) Disable/Clear all GPEs
     * 2) Enable all runtime GPEs
     */
    Status = AcpiHwDisableAllGpes ();
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiHwEnableAllRuntimeGpes ();
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Now we can execute _WAK, etc. Some machines require that the GPEs
     * are enabled before the wake methods are executed.
     */
    AcpiHwExecuteSleepMethod (METHOD_PATHNAME__WAK, SleepState);

    /*
     * Some BIOS code assumes that WAK_STS will be cleared on resume
     * and use it to determine whether the system is rebooting or
     * resuming. Clear WAK_STS for compatibility.
     */
    (void) AcpiWriteBitRegister (ACPI_BITREG_WAKE_STATUS,
        ACPI_CLEAR_STATUS);
    AcpiGbl_SystemAwakeAndRunning = TRUE;

    /* Enable power button */

    (void) AcpiWriteBitRegister(
            AcpiGbl_FixedEventInfo[ACPI_EVENT_POWER_BUTTON].EnableRegisterId,
            ACPI_ENABLE_EVENT);

    (void) AcpiWriteBitRegister(
            AcpiGbl_FixedEventInfo[ACPI_EVENT_POWER_BUTTON].StatusRegisterId,
            ACPI_CLEAR_STATUS);

    AcpiHwExecuteSleepMethod (METHOD_PATHNAME__SST, ACPI_SST_WORKING);
    return_ACPI_STATUS (Status);
}

#endif /* !ACPI_REDUCED_HARDWARE */
