/******************************************************************************
 *
 * Module Name: hwacpi - ACPI Hardware Initialization/Mode Interface
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

#include "acpi.h"
#include "accommon.h"


#define _COMPONENT          ACPI_HARDWARE
        ACPI_MODULE_NAME    ("hwacpi")


#if (!ACPI_REDUCED_HARDWARE) /* Entire module */
/******************************************************************************
 *
 * FUNCTION:    AcpiHwSetMode
 *
 * PARAMETERS:  Mode            - SYS_MODE_ACPI or SYS_MODE_LEGACY
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Transitions the system into the requested mode.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiHwSetMode (
    UINT32                  Mode)
{

    ACPI_STATUS             Status;
    UINT32                  Retry;


    ACPI_FUNCTION_TRACE (HwSetMode);


    /* If the Hardware Reduced flag is set, machine is always in acpi mode */

    if (AcpiGbl_ReducedHardware)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /*
     * ACPI 2.0 clarified that if SMI_CMD in FADT is zero,
     * system does not support mode transition.
     */
    if (!AcpiGbl_FADT.SmiCommand)
    {
        ACPI_ERROR ((AE_INFO, "No SMI_CMD in FADT, mode transition failed"));
        return_ACPI_STATUS (AE_NO_HARDWARE_RESPONSE);
    }

    /*
     * ACPI 2.0 clarified the meaning of ACPI_ENABLE and ACPI_DISABLE
     * in FADT: If it is zero, enabling or disabling is not supported.
     * As old systems may have used zero for mode transition,
     * we make sure both the numbers are zero to determine these
     * transitions are not supported.
     */
    if (!AcpiGbl_FADT.AcpiEnable && !AcpiGbl_FADT.AcpiDisable)
    {
        ACPI_ERROR ((AE_INFO,
            "No ACPI mode transition supported in this system "
            "(enable/disable both zero)"));
        return_ACPI_STATUS (AE_OK);
    }

    switch (Mode)
    {
    case ACPI_SYS_MODE_ACPI:

        /* BIOS should have disabled ALL fixed and GP events */

        Status = AcpiHwWritePort (AcpiGbl_FADT.SmiCommand,
            (UINT32) AcpiGbl_FADT.AcpiEnable, 8);
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO, "Attempting to enable ACPI mode\n"));
        break;

    case ACPI_SYS_MODE_LEGACY:
        /*
         * BIOS should clear all fixed status bits and restore fixed event
         * enable bits to default
         */
        Status = AcpiHwWritePort (AcpiGbl_FADT.SmiCommand,
            (UINT32) AcpiGbl_FADT.AcpiDisable, 8);
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "Attempting to enable Legacy (non-ACPI) mode\n"));
        break;

    default:

        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Could not write ACPI mode change"));
        return_ACPI_STATUS (Status);
    }

    /*
     * Some hardware takes a LONG time to switch modes. Give them 3 sec to
     * do so, but allow faster systems to proceed more quickly.
     */
    Retry = 3000;
    while (Retry)
    {
        if (AcpiHwGetMode () == Mode)
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
                "Mode %X successfully enabled\n", Mode));
            return_ACPI_STATUS (AE_OK);
        }
        AcpiOsStall (ACPI_USEC_PER_MSEC);
        Retry--;
    }

    ACPI_ERROR ((AE_INFO, "Hardware did not change modes"));
    return_ACPI_STATUS (AE_NO_HARDWARE_RESPONSE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiHwGetMode
 *
 * PARAMETERS:  none
 *
 * RETURN:      SYS_MODE_ACPI or SYS_MODE_LEGACY
 *
 * DESCRIPTION: Return current operating state of system. Determined by
 *              querying the SCI_EN bit.
 *
 ******************************************************************************/

UINT32
AcpiHwGetMode (
    void)
{
    ACPI_STATUS             Status;
    UINT32                  Value;


    ACPI_FUNCTION_TRACE (HwGetMode);


    /* If the Hardware Reduced flag is set, machine is always in acpi mode */

    if (AcpiGbl_ReducedHardware)
    {
        return_UINT32 (ACPI_SYS_MODE_ACPI);
    }

    /*
     * ACPI 2.0 clarified that if SMI_CMD in FADT is zero,
     * system does not support mode transition.
     */
    if (!AcpiGbl_FADT.SmiCommand)
    {
        return_UINT32 (ACPI_SYS_MODE_ACPI);
    }

    Status = AcpiReadBitRegister (ACPI_BITREG_SCI_ENABLE, &Value);
    if (ACPI_FAILURE (Status))
    {
        return_UINT32 (ACPI_SYS_MODE_LEGACY);
    }

    if (Value)
    {
        return_UINT32 (ACPI_SYS_MODE_ACPI);
    }
    else
    {
        return_UINT32 (ACPI_SYS_MODE_LEGACY);
    }
}

#endif /* !ACPI_REDUCED_HARDWARE */
