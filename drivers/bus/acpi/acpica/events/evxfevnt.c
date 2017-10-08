/******************************************************************************
 *
 * Module Name: evxfevnt - External Interfaces, ACPI event disable/enable
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2017, Intel Corp.
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
#include "actables.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evxfevnt")


#if (!ACPI_REDUCED_HARDWARE) /* Entire module */
/*******************************************************************************
 *
 * FUNCTION:    AcpiEnable
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Transfers the system into ACPI mode.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEnable (
    void)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (AcpiEnable);


    /* ACPI tables must be present */

    if (AcpiGbl_FadtIndex == ACPI_INVALID_TABLE_INDEX)
    {
        return_ACPI_STATUS (AE_NO_ACPI_TABLES);
    }

    /* If the Hardware Reduced flag is set, machine is always in acpi mode */

    if (AcpiGbl_ReducedHardware)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Check current mode */

    if (AcpiHwGetMode() == ACPI_SYS_MODE_ACPI)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_INIT,
            "System is already in ACPI mode\n"));
    }
    else
    {
        /* Transition to ACPI mode */

        Status = AcpiHwSetMode (ACPI_SYS_MODE_ACPI);
        if (ACPI_FAILURE (Status))
        {
            ACPI_ERROR ((AE_INFO, "Could not transition to ACPI mode"));
            return_ACPI_STATUS (Status);
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_INIT,
            "Transition to ACPI mode successful\n"));
    }

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiEnable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiDisable
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Transfers the system into LEGACY (non-ACPI) mode.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDisable (
    void)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (AcpiDisable);


    /* If the Hardware Reduced flag is set, machine is always in acpi mode */

    if (AcpiGbl_ReducedHardware)
    {
        return_ACPI_STATUS (AE_OK);
    }

    if (AcpiHwGetMode() == ACPI_SYS_MODE_LEGACY)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_INIT,
            "System is already in legacy (non-ACPI) mode\n"));
    }
    else
    {
        /* Transition to LEGACY mode */

        Status = AcpiHwSetMode (ACPI_SYS_MODE_LEGACY);

        if (ACPI_FAILURE (Status))
        {
            ACPI_ERROR ((AE_INFO,
                "Could not exit ACPI mode to legacy mode"));
            return_ACPI_STATUS (Status);
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_INIT,
            "ACPI mode disabled\n"));
    }

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiDisable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiEnableEvent
 *
 * PARAMETERS:  Event           - The fixed eventto be enabled
 *              Flags           - Reserved
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable an ACPI event (fixed)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEnableEvent (
    UINT32                  Event,
    UINT32                  Flags)
{
    ACPI_STATUS             Status = AE_OK;
    UINT32                  Value;


    ACPI_FUNCTION_TRACE (AcpiEnableEvent);


    /* If Hardware Reduced flag is set, there are no fixed events */

    if (AcpiGbl_ReducedHardware)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Decode the Fixed Event */

    if (Event > ACPI_EVENT_MAX)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /*
     * Enable the requested fixed event (by writing a one to the enable
     * register bit)
     */
    Status = AcpiWriteBitRegister (
        AcpiGbl_FixedEventInfo[Event].EnableRegisterId,
        ACPI_ENABLE_EVENT);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Make sure that the hardware responded */

    Status = AcpiReadBitRegister (
        AcpiGbl_FixedEventInfo[Event].EnableRegisterId, &Value);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (Value != 1)
    {
        ACPI_ERROR ((AE_INFO,
            "Could not enable %s event", AcpiUtGetEventName (Event)));
        return_ACPI_STATUS (AE_NO_HARDWARE_RESPONSE);
    }

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiEnableEvent)


/*******************************************************************************
 *
 * FUNCTION:    AcpiDisableEvent
 *
 * PARAMETERS:  Event           - The fixed event to be disabled
 *              Flags           - Reserved
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Disable an ACPI event (fixed)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDisableEvent (
    UINT32                  Event,
    UINT32                  Flags)
{
    ACPI_STATUS             Status = AE_OK;
    UINT32                  Value;


    ACPI_FUNCTION_TRACE (AcpiDisableEvent);


    /* If Hardware Reduced flag is set, there are no fixed events */

    if (AcpiGbl_ReducedHardware)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Decode the Fixed Event */

    if (Event > ACPI_EVENT_MAX)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /*
     * Disable the requested fixed event (by writing a zero to the enable
     * register bit)
     */
    Status = AcpiWriteBitRegister (
        AcpiGbl_FixedEventInfo[Event].EnableRegisterId,
        ACPI_DISABLE_EVENT);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiReadBitRegister (
        AcpiGbl_FixedEventInfo[Event].EnableRegisterId, &Value);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (Value != 0)
    {
        ACPI_ERROR ((AE_INFO,
            "Could not disable %s events", AcpiUtGetEventName (Event)));
        return_ACPI_STATUS (AE_NO_HARDWARE_RESPONSE);
    }

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiDisableEvent)


/*******************************************************************************
 *
 * FUNCTION:    AcpiClearEvent
 *
 * PARAMETERS:  Event           - The fixed event to be cleared
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear an ACPI event (fixed)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiClearEvent (
    UINT32                  Event)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (AcpiClearEvent);


    /* If Hardware Reduced flag is set, there are no fixed events */

    if (AcpiGbl_ReducedHardware)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Decode the Fixed Event */

    if (Event > ACPI_EVENT_MAX)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /*
     * Clear the requested fixed event (By writing a one to the status
     * register bit)
     */
    Status = AcpiWriteBitRegister (
        AcpiGbl_FixedEventInfo[Event].StatusRegisterId,
        ACPI_CLEAR_STATUS);

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiClearEvent)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetEventStatus
 *
 * PARAMETERS:  Event           - The fixed event
 *              EventStatus     - Where the current status of the event will
 *                                be returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Obtains and returns the current status of the event
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetEventStatus (
    UINT32                  Event,
    ACPI_EVENT_STATUS       *EventStatus)
{
    ACPI_STATUS             Status;
    ACPI_EVENT_STATUS       LocalEventStatus = 0;
    UINT32                  InByte;


    ACPI_FUNCTION_TRACE (AcpiGetEventStatus);


    if (!EventStatus)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Decode the Fixed Event */

    if (Event > ACPI_EVENT_MAX)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Fixed event currently can be dispatched? */

    if (AcpiGbl_FixedEventHandlers[Event].Handler)
    {
        LocalEventStatus |= ACPI_EVENT_FLAG_HAS_HANDLER;
    }

    /* Fixed event currently enabled? */

    Status = AcpiReadBitRegister (
        AcpiGbl_FixedEventInfo[Event].EnableRegisterId, &InByte);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (InByte)
    {
        LocalEventStatus |=
            (ACPI_EVENT_FLAG_ENABLED | ACPI_EVENT_FLAG_ENABLE_SET);
    }

    /* Fixed event currently active? */

    Status = AcpiReadBitRegister (
        AcpiGbl_FixedEventInfo[Event].StatusRegisterId, &InByte);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (InByte)
    {
        LocalEventStatus |= ACPI_EVENT_FLAG_STATUS_SET;
    }

    (*EventStatus) = LocalEventStatus;
    return_ACPI_STATUS (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiGetEventStatus)

#endif /* !ACPI_REDUCED_HARDWARE */
