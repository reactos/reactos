/******************************************************************************
 *
 * Module Name: evxfgpe - External Interfaces for General Purpose Events (GPEs)
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2011, Intel Corp.
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


#define __EVXFGPE_C__

#include "acpi.h"
#include "accommon.h"
#include "acevents.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evxfgpe")


/*******************************************************************************
 *
 * FUNCTION:    AcpiUpdateAllGpes
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Complete GPE initialization and enable all GPEs that have
 *              associated _Lxx or _Exx methods and are not pointed to by any
 *              device _PRW methods (this indicates that these GPEs are
 *              generally intended for system or device wakeup. Such GPEs
 *              have to be enabled directly when the devices whose _PRW
 *              methods point to them are set up for wakeup signaling.)
 *
 * NOTE: Should be called after any GPEs are added to the system. Primarily,
 * after the system _PRW methods have been run, but also after a GPE Block
 * Device has been added or if any new GPE methods have been added via a
 * dynamic table load.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUpdateAllGpes (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiUpdateGpes);


    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (AcpiGbl_AllGpesInitialized)
    {
        goto UnlockAndExit;
    }

    Status = AcpiEvWalkGpeList (AcpiEvInitializeGpeBlock, NULL);
    if (ACPI_SUCCESS (Status))
    {
        AcpiGbl_AllGpesInitialized = TRUE;
    }

UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiUpdateAllGpes)


/*******************************************************************************
 *
 * FUNCTION:    AcpiEnableGpe
 *
 * PARAMETERS:  GpeDevice           - Parent GPE Device. NULL for GPE0/GPE1
 *              GpeNumber           - GPE level within the GPE block
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Add a reference to a GPE. On the first reference, the GPE is
 *              hardware-enabled.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEnableGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber)
{
    ACPI_STATUS             Status = AE_BAD_PARAMETER;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (AcpiEnableGpe);


    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (GpeEventInfo)
    {
        Status = AcpiEvAddGpeReference (GpeEventInfo);
    }

    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiEnableGpe)


/*******************************************************************************
 *
 * FUNCTION:    AcpiDisableGpe
 *
 * PARAMETERS:  GpeDevice           - Parent GPE Device. NULL for GPE0/GPE1
 *              GpeNumber           - GPE level within the GPE block
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a reference to a GPE. When the last reference is
 *              removed, only then is the GPE disabled (for runtime GPEs), or
 *              the GPE mask bit disabled (for wake GPEs)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDisableGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber)
{
    ACPI_STATUS             Status = AE_BAD_PARAMETER;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (AcpiDisableGpe);


    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (GpeEventInfo)
    {
        Status = AcpiEvRemoveGpeReference (GpeEventInfo);
    }

    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiDisableGpe)


/*******************************************************************************
 *
 * FUNCTION:    AcpiSetGpe
 *
 * PARAMETERS:  GpeDevice           - Parent GPE Device. NULL for GPE0/GPE1
 *              GpeNumber           - GPE level within the GPE block
 *              Action              - ACPI_GPE_ENABLE or ACPI_GPE_DISABLE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable or disable an individual GPE. This function bypasses
 *              the reference count mechanism used in the AcpiEnableGpe and
 *              AcpiDisableGpe interfaces -- and should be used with care.
 *
 * Note: Typically used to disable a runtime GPE for short period of time,
 * then re-enable it, without disturbing the existing reference counts. This
 * is useful, for example, in the Embedded Controller (EC) driver.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiSetGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT8                   Action)
{
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_STATUS             Status;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (AcpiSetGpe);


    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (!GpeEventInfo)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    /* Perform the action */

    switch (Action)
    {
    case ACPI_GPE_ENABLE:
        Status = AcpiEvEnableGpe (GpeEventInfo);
        break;

    case ACPI_GPE_DISABLE:
        Status = AcpiHwLowSetGpe (GpeEventInfo, ACPI_GPE_DISABLE);
        break;

    default:
        Status = AE_BAD_PARAMETER;
        break;
    }

UnlockAndExit:
    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiSetGpe)


/*******************************************************************************
 *
 * FUNCTION:    AcpiSetupGpeForWake
 *
 * PARAMETERS:  WakeDevice          - Device associated with the GPE (via _PRW)
 *              GpeDevice           - Parent GPE Device. NULL for GPE0/GPE1
 *              GpeNumber           - GPE level within the GPE block
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Mark a GPE as having the ability to wake the system. This
 *              interface is intended to be used as the host executes the
 *              _PRW methods (Power Resources for Wake) in the system tables.
 *              Each _PRW appears under a Device Object (The WakeDevice), and
 *              contains the info for the wake GPE associated with the
 *              WakeDevice.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiSetupGpeForWake (
    ACPI_HANDLE             WakeDevice,
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber)
{
    ACPI_STATUS             Status = AE_BAD_PARAMETER;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_NAMESPACE_NODE     *DeviceNode;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (AcpiSetupGpeForWake);


    /* Parameter Validation */

    if (!WakeDevice)
    {
        /*
         * By forcing WakeDevice to be valid, we automatically enable the
         * implicit notify feature on all hosts.
         */
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Handle root object case */

    if (WakeDevice == ACPI_ROOT_OBJECT)
    {
        DeviceNode = AcpiGbl_RootNode;
    }
    else
    {
        DeviceNode = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE, WakeDevice);
    }

    /* Validate WakeDevice is of type Device */

    if (DeviceNode->Type != ACPI_TYPE_DEVICE)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (GpeEventInfo)
    {
        /*
         * If there is no method or handler for this GPE, then the
         * WakeDevice will be notified whenever this GPE fires (aka
         * "implicit notify") Note: The GPE is assumed to be
         * level-triggered (for windows compatibility).
         */
        if ((GpeEventInfo->Flags & ACPI_GPE_DISPATCH_MASK) ==
                ACPI_GPE_DISPATCH_NONE)
        {
            GpeEventInfo->Flags =
                (ACPI_GPE_DISPATCH_NOTIFY | ACPI_GPE_LEVEL_TRIGGERED);
            GpeEventInfo->Dispatch.DeviceNode = DeviceNode;
        }

        GpeEventInfo->Flags |= ACPI_GPE_CAN_WAKE;
        Status = AE_OK;
    }

    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiSetupGpeForWake)


/*******************************************************************************
 *
 * FUNCTION:    AcpiSetGpeWakeMask
 *
 * PARAMETERS:  GpeDevice           - Parent GPE Device. NULL for GPE0/GPE1
 *              GpeNumber           - GPE level within the GPE block
 *              Action              - Enable or Disable
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Set or clear the GPE's wakeup enable mask bit. The GPE must
 *              already be marked as a WAKE GPE.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiSetGpeWakeMask (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT8                   Action)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_GPE_REGISTER_INFO  *GpeRegisterInfo;
    ACPI_CPU_FLAGS          Flags;
    UINT32                  RegisterBit;


    ACPI_FUNCTION_TRACE (AcpiSetGpeWakeMask);


    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

    /*
     * Ensure that we have a valid GPE number and that this GPE is in
     * fact a wake GPE
     */
    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (!GpeEventInfo)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    if (!(GpeEventInfo->Flags & ACPI_GPE_CAN_WAKE))
    {
        Status = AE_TYPE;
        goto UnlockAndExit;
    }

    GpeRegisterInfo = GpeEventInfo->RegisterInfo;
    if (!GpeRegisterInfo)
    {
        Status = AE_NOT_EXIST;
        goto UnlockAndExit;
    }

    RegisterBit = AcpiHwGetGpeRegisterBit (GpeEventInfo, GpeRegisterInfo);

    /* Perform the action */

    switch (Action)
    {
    case ACPI_GPE_ENABLE:
        ACPI_SET_BIT (GpeRegisterInfo->EnableForWake, (UINT8) RegisterBit);
        break;

    case ACPI_GPE_DISABLE:
        ACPI_CLEAR_BIT (GpeRegisterInfo->EnableForWake, (UINT8) RegisterBit);
        break;

    default:
        ACPI_ERROR ((AE_INFO, "%u, Invalid action", Action));
        Status = AE_BAD_PARAMETER;
        break;
    }

UnlockAndExit:
    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiSetGpeWakeMask)


/*******************************************************************************
 *
 * FUNCTION:    AcpiClearGpe
 *
 * PARAMETERS:  GpeDevice           - Parent GPE Device. NULL for GPE0/GPE1
 *              GpeNumber           - GPE level within the GPE block
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear an ACPI event (general purpose)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiClearGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (AcpiClearGpe);


    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (!GpeEventInfo)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    Status = AcpiHwClearGpe (GpeEventInfo);

UnlockAndExit:
    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiClearGpe)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetGpeStatus
 *
 * PARAMETERS:  GpeDevice           - Parent GPE Device. NULL for GPE0/GPE1
 *              GpeNumber           - GPE level within the GPE block
 *              EventStatus         - Where the current status of the event
 *                                    will be returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get the current status of a GPE (signalled/not_signalled)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetGpeStatus (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    ACPI_EVENT_STATUS       *EventStatus)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (AcpiGetGpeStatus);


    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

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
    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiGetGpeStatus)


/*******************************************************************************
 *
 * FUNCTION:    AcpiFinishGpe
 *
 * PARAMETERS:  GpeDevice           - Namespace node for the GPE Block
 *                                    (NULL for FADT defined GPEs)
 *              GpeNumber           - GPE level within the GPE block
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear and conditionally reenable a GPE. This completes the GPE
 *              processing. Intended for use by asynchronous host-installed
 *              GPE handlers. The GPE is only reenabled if the EnableForRun bit
 *              is set in the GPE info.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiFinishGpe (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber)
{
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_STATUS             Status;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (AcpiFinishGpe);


    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (!GpeEventInfo)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    Status = AcpiEvFinishGpe (GpeEventInfo);

UnlockAndExit:
    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiFinishGpe)


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

ACPI_EXPORT_SYMBOL (AcpiDisableAllGpes)


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

ACPI_EXPORT_SYMBOL (AcpiEnableAllRuntimeGpes)


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
 * DESCRIPTION: Create and Install a block of GPE registers. The GPEs are not
 *              enabled here.
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

    /* Install block in the DeviceObject attached to the node */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (!ObjDesc)
    {
        /*
         * No object, create a new one (Device nodes do not always have
         * an attached object)
         */
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

    /* Now install the GPE block in the DeviceObject */

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
