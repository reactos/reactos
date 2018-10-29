/******************************************************************************
 *
 * Name: hwxfsleep.c - ACPI Hardware Sleep/Wake External Interfaces
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
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

#define EXPORT_ACPI_INTERFACES

#include "acpi.h"
#include "accommon.h"

#define _COMPONENT          ACPI_HARDWARE
        ACPI_MODULE_NAME    ("hwxfsleep")

/* Local prototypes */

#if (!ACPI_REDUCED_HARDWARE)
static ACPI_STATUS
AcpiHwSetFirmwareWakingVector (
    ACPI_TABLE_FACS         *Facs,
    ACPI_PHYSICAL_ADDRESS   PhysicalAddress,
    ACPI_PHYSICAL_ADDRESS   PhysicalAddress64);
#endif

static ACPI_STATUS
AcpiHwSleepDispatch (
    UINT8                   SleepState,
    UINT32                  FunctionId);

/*
 * Dispatch table used to efficiently branch to the various sleep
 * functions.
 */
#define ACPI_SLEEP_FUNCTION_ID          0
#define ACPI_WAKE_PREP_FUNCTION_ID      1
#define ACPI_WAKE_FUNCTION_ID           2

/* Legacy functions are optional, based upon ACPI_REDUCED_HARDWARE */

static ACPI_SLEEP_FUNCTIONS         AcpiSleepDispatch[] =
{
    {ACPI_STRUCT_INIT (LegacyFunction,
                       ACPI_HW_OPTIONAL_FUNCTION (AcpiHwLegacySleep)),
     ACPI_STRUCT_INIT (ExtendedFunction,
                       AcpiHwExtendedSleep) },
    {ACPI_STRUCT_INIT (LegacyFunction,
                       ACPI_HW_OPTIONAL_FUNCTION (AcpiHwLegacyWakePrep)),
     ACPI_STRUCT_INIT (ExtendedFunction,
                       AcpiHwExtendedWakePrep) },
    {ACPI_STRUCT_INIT (Legacy_function,
                       ACPI_HW_OPTIONAL_FUNCTION (AcpiHwLegacyWake)),
     ACPI_STRUCT_INIT (ExtendedFunction,
                       AcpiHwExtendedWake) }
};


/*
 * These functions are removed for the ACPI_REDUCED_HARDWARE case:
 *      AcpiSetFirmwareWakingVector
 *      AcpiEnterSleepStateS4bios
 */

#if (!ACPI_REDUCED_HARDWARE)
/*******************************************************************************
 *
 * FUNCTION:    AcpiHwSetFirmwareWakingVector
 *
 * PARAMETERS:  Facs                - Pointer to FACS table
 *              PhysicalAddress     - 32-bit physical address of ACPI real mode
 *                                    entry point
 *              PhysicalAddress64   - 64-bit physical address of ACPI protected
 *                                    mode entry point
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Sets the FirmwareWakingVector fields of the FACS
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiHwSetFirmwareWakingVector (
    ACPI_TABLE_FACS         *Facs,
    ACPI_PHYSICAL_ADDRESS   PhysicalAddress,
    ACPI_PHYSICAL_ADDRESS   PhysicalAddress64)
{
    ACPI_FUNCTION_TRACE (AcpiHwSetFirmwareWakingVector);


    /*
     * According to the ACPI specification 2.0c and later, the 64-bit
     * waking vector should be cleared and the 32-bit waking vector should
     * be used, unless we want the wake-up code to be called by the BIOS in
     * Protected Mode. Some systems (for example HP dv5-1004nr) are known
     * to fail to resume if the 64-bit vector is used.
     */

    /* Set the 32-bit vector */

    Facs->FirmwareWakingVector = (UINT32) PhysicalAddress;

    if (Facs->Length > 32)
    {
        if (Facs->Version >= 1)
        {
            /* Set the 64-bit vector */

            Facs->XFirmwareWakingVector = PhysicalAddress64;
        }
        else
        {
            /* Clear the 64-bit vector if it exists */

            Facs->XFirmwareWakingVector = 0;
        }
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiSetFirmwareWakingVector
 *
 * PARAMETERS:  PhysicalAddress     - 32-bit physical address of ACPI real mode
 *                                    entry point
 *              PhysicalAddress64   - 64-bit physical address of ACPI protected
 *                                    mode entry point
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Sets the FirmwareWakingVector fields of the FACS
 *
 ******************************************************************************/

ACPI_STATUS
AcpiSetFirmwareWakingVector (
    ACPI_PHYSICAL_ADDRESS   PhysicalAddress,
    ACPI_PHYSICAL_ADDRESS   PhysicalAddress64)
{

    ACPI_FUNCTION_TRACE (AcpiSetFirmwareWakingVector);

    if (AcpiGbl_FACS)
    {
        (void) AcpiHwSetFirmwareWakingVector (AcpiGbl_FACS,
            PhysicalAddress, PhysicalAddress64);
    }

    return_ACPI_STATUS (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiSetFirmwareWakingVector)


/*******************************************************************************
 *
 * FUNCTION:    AcpiEnterSleepStateS4bios
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform a S4 bios request.
 *              THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEnterSleepStateS4bios (
    void)
{
    UINT32                  InValue;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiEnterSleepStateS4bios);


    /* Clear the wake status bit (PM1) */

    Status = AcpiWriteBitRegister (ACPI_BITREG_WAKE_STATUS, ACPI_CLEAR_STATUS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiHwClearAcpiStatus ();
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * 1) Disable all GPEs
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

    ACPI_FLUSH_CPU_CACHE ();

    Status = AcpiHwWritePort (AcpiGbl_FADT.SmiCommand,
        (UINT32) AcpiGbl_FADT.S4BiosRequest, 8);

    do {
        AcpiOsStall (ACPI_USEC_PER_MSEC);
        Status = AcpiReadBitRegister (ACPI_BITREG_WAKE_STATUS, &InValue);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

    } while (!InValue);

    return_ACPI_STATUS (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiEnterSleepStateS4bios)

#endif /* !ACPI_REDUCED_HARDWARE */


/*******************************************************************************
 *
 * FUNCTION:    AcpiHwSleepDispatch
 *
 * PARAMETERS:  SleepState          - Which sleep state to enter/exit
 *              FunctionId          - Sleep, WakePrep, or Wake
 *
 * RETURN:      Status from the invoked sleep handling function.
 *
 * DESCRIPTION: Dispatch a sleep/wake request to the appropriate handling
 *              function.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiHwSleepDispatch (
    UINT8                   SleepState,
    UINT32                  FunctionId)
{
    ACPI_STATUS             Status;
    ACPI_SLEEP_FUNCTIONS    *SleepFunctions = &AcpiSleepDispatch[FunctionId];


#if (!ACPI_REDUCED_HARDWARE)
    /*
     * If the Hardware Reduced flag is set (from the FADT), we must
     * use the extended sleep registers (FADT). Note: As per the ACPI
     * specification, these extended registers are to be used for HW-reduced
     * platforms only. They are not general-purpose replacements for the
     * legacy PM register sleep support.
     */
    if (AcpiGbl_ReducedHardware)
    {
        Status = SleepFunctions->ExtendedFunction (SleepState);
    }
    else
    {
        /* Legacy sleep */

        Status = SleepFunctions->LegacyFunction (SleepState);
    }

    return (Status);

#else
    /*
     * For the case where reduced-hardware-only code is being generated,
     * we know that only the extended sleep registers are available
     */
    Status = SleepFunctions->ExtendedFunction (SleepState);
    return (Status);

#endif /* !ACPI_REDUCED_HARDWARE */
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEnterSleepStatePrep
 *
 * PARAMETERS:  SleepState          - Which sleep state to enter
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Prepare to enter a system sleep state.
 *              This function must execute with interrupts enabled.
 *              We break sleeping into 2 stages so that OSPM can handle
 *              various OS-specific tasks between the two steps.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEnterSleepStatePrep (
    UINT8                   SleepState)
{
    ACPI_STATUS             Status;
    ACPI_OBJECT_LIST        ArgList;
    ACPI_OBJECT             Arg;
    UINT32                  SstValue;


    ACPI_FUNCTION_TRACE (AcpiEnterSleepStatePrep);


    Status = AcpiGetSleepTypeData (SleepState,
        &AcpiGbl_SleepTypeA, &AcpiGbl_SleepTypeB);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Execute the _PTS method (Prepare To Sleep) */

    ArgList.Count = 1;
    ArgList.Pointer = &Arg;
    Arg.Type = ACPI_TYPE_INTEGER;
    Arg.Integer.Value = SleepState;

    Status = AcpiEvaluateObject (NULL, METHOD_PATHNAME__PTS, &ArgList, NULL);
    if (ACPI_FAILURE (Status) && Status != AE_NOT_FOUND)
    {
        return_ACPI_STATUS (Status);
    }

    /* Setup the argument to the _SST method (System STatus) */

    switch (SleepState)
    {
    case ACPI_STATE_S0:

        SstValue = ACPI_SST_WORKING;
        break;

    case ACPI_STATE_S1:
    case ACPI_STATE_S2:
    case ACPI_STATE_S3:

        SstValue = ACPI_SST_SLEEPING;
        break;

    case ACPI_STATE_S4:

        SstValue = ACPI_SST_SLEEP_CONTEXT;
        break;

    default:

        SstValue = ACPI_SST_INDICATOR_OFF; /* Default is off */
        break;
    }

    /*
     * Set the system indicators to show the desired sleep state.
     * _SST is an optional method (return no error if not found)
     */
    AcpiHwExecuteSleepMethod (METHOD_PATHNAME__SST, SstValue);
    return_ACPI_STATUS (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiEnterSleepStatePrep)


/*******************************************************************************
 *
 * FUNCTION:    AcpiEnterSleepState
 *
 * PARAMETERS:  SleepState          - Which sleep state to enter
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enter a system sleep state
 *              THIS FUNCTION MUST BE CALLED WITH INTERRUPTS DISABLED
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEnterSleepState (
    UINT8                   SleepState)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiEnterSleepState);


    if ((AcpiGbl_SleepTypeA > ACPI_SLEEP_TYPE_MAX) ||
        (AcpiGbl_SleepTypeB > ACPI_SLEEP_TYPE_MAX))
    {
        ACPI_ERROR ((AE_INFO, "Sleep values out of range: A=0x%X B=0x%X",
            AcpiGbl_SleepTypeA, AcpiGbl_SleepTypeB));
        return_ACPI_STATUS (AE_AML_OPERAND_VALUE);
    }

    Status = AcpiHwSleepDispatch (SleepState, ACPI_SLEEP_FUNCTION_ID);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiEnterSleepState)


/*******************************************************************************
 *
 * FUNCTION:    AcpiLeaveSleepStatePrep
 *
 * PARAMETERS:  SleepState          - Which sleep state we are exiting
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform the first state of OS-independent ACPI cleanup after a
 *              sleep. Called with interrupts DISABLED.
 *              We break wake/resume into 2 stages so that OSPM can handle
 *              various OS-specific tasks between the two steps.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiLeaveSleepStatePrep (
    UINT8                   SleepState)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiLeaveSleepStatePrep);


    Status = AcpiHwSleepDispatch (SleepState, ACPI_WAKE_PREP_FUNCTION_ID);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiLeaveSleepStatePrep)


/*******************************************************************************
 *
 * FUNCTION:    AcpiLeaveSleepState
 *
 * PARAMETERS:  SleepState          - Which sleep state we are exiting
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform OS-independent ACPI cleanup after a sleep
 *              Called with interrupts ENABLED.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiLeaveSleepState (
    UINT8                   SleepState)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiLeaveSleepState);


    Status = AcpiHwSleepDispatch (SleepState, ACPI_WAKE_FUNCTION_ID);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiLeaveSleepState)
