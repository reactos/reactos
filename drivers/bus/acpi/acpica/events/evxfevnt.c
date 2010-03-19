/******************************************************************************
 *
 * Module Name: evxfevnt - External Interfaces, ACPI event disable/enable
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/


#define __EVXFEVNT_C__

#include "acpi.h"
#include "accommon.h"
#include "acevents.h"
#include "acnamesp.h"
#include "actables.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evxfevnt")

/* Local prototypes */

static ACPI_STATUS
AcpiEvGetGpeDevice (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Context);


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

    if (!AcpiTbTablesLoaded ())
    {
        return_ACPI_STATUS (AE_NO_ACPI_TABLES);
    }

    /* Check current mode */

    if (AcpiHwGetMode() == ACPI_SYS_MODE_ACPI)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_INIT, "System is already in ACPI mode\n"));
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

        ACPI_DEBUG_PRINT ((ACPI_DB_INIT, "ACPI mode disabled\n"));
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
 * FUNCTION:    AcpiSetGpeType
 *
 * PARAMETERS:  GpeDevice       - Parent GPE Device
 *              GpeNumber       - GPE level within the GPE block
 *              Type            - New GPE type
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Set the type of an individual GPE
 *
 ******************************************************************************/

ACPI_STATUS
AcpiSetGpeType (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT8                   Type)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;


    ACPI_FUNCTION_TRACE (AcpiSetGpeType);


    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (!GpeEventInfo)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    if ((GpeEventInfo->Flags & ACPI_GPE_TYPE_MASK) == Type)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Set the new type (will disable GPE if currently enabled) */

    Status = AcpiEvSetGpeType (GpeEventInfo, Type);

UnlockAndExit:
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiSetGpeType)


/*******************************************************************************
 *
 * FUNCTION:    AcpiEnableGpe
 *
 * PARAMETERS:  GpeDevice       - Parent GPE Device
 *              GpeNumber       - GPE level within the GPE block
 *              Flags           - Just enable, or also wake enable?
 *                                Called from ISR or not
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable an ACPI event (general purpose)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEnableGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT32                  Flags)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;


    ACPI_FUNCTION_TRACE (AcpiEnableGpe);


    /* Use semaphore lock if not executing at interrupt level */

    if (Flags & ACPI_NOT_ISR)
    {
        Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (!GpeEventInfo)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    /* Perform the enable */

    Status = AcpiEvEnableGpe (GpeEventInfo, TRUE);

UnlockAndExit:
    if (Flags & ACPI_NOT_ISR)
    {
        (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    }
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiEnableGpe)


/*******************************************************************************
 *
 * FUNCTION:    AcpiDisableGpe
 *
 * PARAMETERS:  GpeDevice       - Parent GPE Device
 *              GpeNumber       - GPE level within the GPE block
 *              Flags           - Just disable, or also wake disable?
 *                                Called from ISR or not
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Disable an ACPI event (general purpose)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDisableGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT32                  Flags)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;


    ACPI_FUNCTION_TRACE (AcpiDisableGpe);


    /* Use semaphore lock if not executing at interrupt level */

    if (Flags & ACPI_NOT_ISR)
    {
        Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (!GpeEventInfo)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    Status = AcpiEvDisableGpe (GpeEventInfo);

UnlockAndExit:
    if (Flags & ACPI_NOT_ISR)
    {
        (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    }
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiDisableGpe)


/*******************************************************************************
 *
 * FUNCTION:    AcpiDisableEvent
 *
 * PARAMETERS:  Event           - The fixed eventto be enabled
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
 * FUNCTION:    AcpiClearGpe
 *
 * PARAMETERS:  GpeDevice       - Parent GPE Device
 *              GpeNumber       - GPE level within the GPE block
 *              Flags           - Called from an ISR or not
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear an ACPI event (general purpose)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiClearGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT32                  Flags)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;


    ACPI_FUNCTION_TRACE (AcpiClearGpe);


    /* Use semaphore lock if not executing at interrupt level */

    if (Flags & ACPI_NOT_ISR)
    {
        Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (!GpeEventInfo)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    Status = AcpiHwClearGpe (GpeEventInfo);

UnlockAndExit:
    if (Flags & ACPI_NOT_ISR)
    {
        (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    }
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiClearGpe)


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
    ACPI_STATUS             Status = AE_OK;


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

    /* Get the status of the requested fixed event */

    Status = AcpiReadBitRegister (
                AcpiGbl_FixedEventInfo[Event].StatusRegisterId, EventStatus);

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetEventStatus)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetGpeStatus
 *
 * PARAMETERS:  GpeDevice       - Parent GPE Device
 *              GpeNumber       - GPE level within the GPE block
 *              Flags           - Called from an ISR or not
 *              EventStatus     - Where the current status of the event will
 *                                be returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get status of an event (general purpose)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetGpeStatus (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT32                  Flags,
    ACPI_EVENT_STATUS       *EventStatus)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;


    ACPI_FUNCTION_TRACE (AcpiGetGpeStatus);


    /* Use semaphore lock if not executing at interrupt level */

    if (Flags & ACPI_NOT_ISR)
    {
        Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (!GpeEventInfo)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    /* Obtain status on the requested GPE number */

    Status = AcpiHwGetGpeStatus (GpeEventInfo, EventStatus);

UnlockAndExit:
    if (Flags & ACPI_NOT_ISR)
    {
        (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    }
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetGpeStatus)


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallGpeBlock
 *
 * PARAMETERS:  GpeDevice           - Handle to the parent GPE Block Device
 *              GpeBlockAddress     - Address and SpaceID
 *              RegisterCount       - Number of GPE register pairs in the block
 *              InterruptNumber     - H/W interrupt for the block
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create and Install a block of GPE registers
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallGpeBlock (
    ACPI_HANDLE             GpeDevice,
    ACPI_GENERIC_ADDRESS    *GpeBlockAddress,
    UINT32                  RegisterCount,
    UINT32                  InterruptNumber)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_GPE_BLOCK_INFO     *GpeBlock;


    ACPI_FUNCTION_TRACE (AcpiInstallGpeBlock);


    if ((!GpeDevice)       ||
        (!GpeBlockAddress) ||
        (!RegisterCount))
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Node = AcpiNsValidateHandle (GpeDevice);
    if (!Node)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    /*
     * For user-installed GPE Block Devices, the GpeBlockBaseNumber
     * is always zero
     */
    Status = AcpiEvCreateGpeBlock (Node, GpeBlockAddress, RegisterCount,
                0, InterruptNumber, &GpeBlock);
    if (ACPI_FAILURE (Status))
    {
        goto UnlockAndExit;
    }

    /* Run the _PRW methods and enable the GPEs */

    Status = AcpiEvInitializeGpeBlock (Node, GpeBlock);
    if (ACPI_FAILURE (Status))
    {
        goto UnlockAndExit;
    }

    /* Get the DeviceObject attached to the node */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (!ObjDesc)
    {
        /* No object, create a new one */

        ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_DEVICE);
        if (!ObjDesc)
        {
            Status = AE_NO_MEMORY;
            goto UnlockAndExit;
        }

        Status = AcpiNsAttachObject (Node, ObjDesc, ACPI_TYPE_DEVICE);

        /* Remove local reference to the object */

        AcpiUtRemoveReference (ObjDesc);

        if (ACPI_FAILURE (Status))
        {
            goto UnlockAndExit;
        }
    }

    /* Install the GPE block in the DeviceObject */

    ObjDesc->Device.GpeBlock = GpeBlock;


UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallGpeBlock)


/*******************************************************************************
 *
 * FUNCTION:    AcpiRemoveGpeBlock
 *
 * PARAMETERS:  GpeDevice           - Handle to the parent GPE Block Device
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a previously installed block of GPE registers
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRemoveGpeBlock (
    ACPI_HANDLE             GpeDevice)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node;


    ACPI_FUNCTION_TRACE (AcpiRemoveGpeBlock);


    if (!GpeDevice)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Node = AcpiNsValidateHandle (GpeDevice);
    if (!Node)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    /* Get the DeviceObject attached to the node */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (!ObjDesc ||
        !ObjDesc->Device.GpeBlock)
    {
        return_ACPI_STATUS (AE_NULL_OBJECT);
    }

    /* Delete the GPE block (but not the DeviceObject) */

    Status = AcpiEvDeleteGpeBlock (ObjDesc->Device.GpeBlock);
    if (ACPI_SUCCESS (Status))
    {
        ObjDesc->Device.GpeBlock = NULL;
    }

UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiRemoveGpeBlock)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetGpeDevice
 *
 * PARAMETERS:  Index               - System GPE index (0-CurrentGpeCount)
 *              GpeDevice           - Where the parent GPE Device is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Obtain the GPE device associated with the input index. A NULL
 *              gpe device indicates that the gpe number is contained in one of
 *              the FADT-defined gpe blocks. Otherwise, the GPE block device.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetGpeDevice (
    UINT32                  Index,
    ACPI_HANDLE             *GpeDevice)
{
    ACPI_GPE_DEVICE_INFO    Info;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiGetGpeDevice);


    if (!GpeDevice)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    if (Index >= AcpiCurrentGpeCount)
    {
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    /* Setup and walk the GPE list */

    Info.Index = Index;
    Info.Status = AE_NOT_EXIST;
    Info.GpeDevice = NULL;
    Info.NextBlockBaseIndex = 0;

    Status = AcpiEvWalkGpeList (AcpiEvGetGpeDevice, &Info);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    *GpeDevice = ACPI_CAST_PTR (ACPI_HANDLE, Info.GpeDevice);
    return_ACPI_STATUS (Info.Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetGpeDevice)


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvGetGpeDevice
 *
 * PARAMETERS:  GPE_WALK_CALLBACK
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Matches the input GPE index (0-CurrentGpeCount) with a GPE
 *              block device. NULL if the GPE is one of the FADT-defined GPEs.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiEvGetGpeDevice (
    ACPI_GPE_XRUPT_INFO     *GpeXruptInfo,
    ACPI_GPE_BLOCK_INFO     *GpeBlock,
    void                    *Context)
{
    ACPI_GPE_DEVICE_INFO    *Info = Context;


    /* Increment Index by the number of GPEs in this block */

    Info->NextBlockBaseIndex +=
        (GpeBlock->RegisterCount * ACPI_GPE_REGISTER_WIDTH);

    if (Info->Index < Info->NextBlockBaseIndex)
    {
        /*
         * The GPE index is within this block, get the node. Leave the node
         * NULL for the FADT-defined GPEs
         */
        if ((GpeBlock->Node)->Type == ACPI_TYPE_DEVICE)
        {
            Info->GpeDevice = GpeBlock->Node;
        }

        Info->Status = AE_OK;
        return (AE_CTRL_END);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiDisableAllGpes
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Disable and clear all GPEs in all GPE blocks
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDisableAllGpes (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiDisableAllGpes);


    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiHwDisableAllGpes ();
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);

    return_ACPI_STATUS (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiEnableAllRuntimeGpes
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable all "runtime" GPEs, in all GPE blocks
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEnableAllRuntimeGpes (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiEnableAllRuntimeGpes);


    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiHwEnableAllRuntimeGpes ();
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);

    return_ACPI_STATUS (Status);
}


