/******************************************************************************
 *
 * Module Name: evxfregn - External Interfaces, ACPI Operation Regions and
 *                         Address Spaces.
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2021, Intel Corp.
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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
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
#include "acnamesp.h"
#include "acevents.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evxfregn")


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallAddressSpaceHandler
 *
 * PARAMETERS:  Device          - Handle for the device
 *              SpaceId         - The address space ID
 *              Handler         - Address of the handler
 *              Setup           - Address of the setup function
 *              Context         - Value passed to the handler on each access
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for all OpRegions of a given SpaceId.
 *
 * NOTE: This function should only be called after AcpiEnableSubsystem has
 * been called. This is because any _REG methods associated with the Space ID
 * are executed here, and these methods can only be safely executed after
 * the default handlers have been installed and the hardware has been
 * initialized (via AcpiEnableSubsystem.)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallAddressSpaceHandler (
    ACPI_HANDLE             Device,
    ACPI_ADR_SPACE_TYPE     SpaceId,
    ACPI_ADR_SPACE_HANDLER  Handler,
    ACPI_ADR_SPACE_SETUP    Setup,
    void                    *Context)
{
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInstallAddressSpaceHandler);


    /* Parameter validation */

    if (!Device)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Convert and validate the device handle */

    Node = AcpiNsValidateHandle (Device);
    if (!Node)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    /* Install the handler for all Regions for this Space ID */

    Status = AcpiEvInstallSpaceHandler (
        Node, SpaceId, Handler, Setup, Context);
    if (ACPI_FAILURE (Status))
    {
        goto UnlockAndExit;
    }

    /* Run all _REG methods for this address space */

    AcpiEvExecuteRegMethods (Node, SpaceId, ACPI_REG_CONNECT);


UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallAddressSpaceHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiRemoveAddressSpaceHandler
 *
 * PARAMETERS:  Device          - Handle for the device
 *              SpaceId         - The address space ID
 *              Handler         - Address of the handler
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a previously installed handler.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRemoveAddressSpaceHandler (
    ACPI_HANDLE             Device,
    ACPI_ADR_SPACE_TYPE     SpaceId,
    ACPI_ADR_SPACE_HANDLER  Handler)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *HandlerObj;
    ACPI_OPERAND_OBJECT     *RegionObj;
    ACPI_OPERAND_OBJECT     **LastObjPtr;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiRemoveAddressSpaceHandler);


    /* Parameter validation */

    if (!Device)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Convert and validate the device handle */

    Node = AcpiNsValidateHandle (Device);
    if (!Node ||
        ((Node->Type != ACPI_TYPE_DEVICE)    &&
         (Node->Type != ACPI_TYPE_PROCESSOR) &&
         (Node->Type != ACPI_TYPE_THERMAL)   &&
         (Node != AcpiGbl_RootNode)))
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    /* Make sure the internal object exists */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (!ObjDesc)
    {
        Status = AE_NOT_EXIST;
        goto UnlockAndExit;
    }

    /* Find the address handler the user requested */

    HandlerObj = ObjDesc->CommonNotify.Handler;
    LastObjPtr = &ObjDesc->CommonNotify.Handler;
    while (HandlerObj)
    {
        /* We have a handler, see if user requested this one */

        if (HandlerObj->AddressSpace.SpaceId == SpaceId)
        {
            /* Handler must be the same as the installed handler */

            if (HandlerObj->AddressSpace.Handler != Handler)
            {
                Status = AE_BAD_PARAMETER;
                goto UnlockAndExit;
            }

            /* Matched SpaceId, first dereference this in the Regions */

            ACPI_DEBUG_PRINT ((ACPI_DB_OPREGION,
                "Removing address handler %p(%p) for region %s "
                "on Device %p(%p)\n",
                HandlerObj, Handler, AcpiUtGetRegionName (SpaceId),
                Node, ObjDesc));

            RegionObj = HandlerObj->AddressSpace.RegionList;

            /* Walk the handler's region list */

            while (RegionObj)
            {
                /*
                 * First disassociate the handler from the region.
                 *
                 * NOTE: this doesn't mean that the region goes away
                 * The region is just inaccessible as indicated to
                 * the _REG method
                 */
                AcpiEvDetachRegion (RegionObj, TRUE);

                /*
                 * Walk the list: Just grab the head because the
                 * DetachRegion removed the previous head.
                 */
                RegionObj = HandlerObj->AddressSpace.RegionList;
            }

            /* Remove this Handler object from the list */

            *LastObjPtr = HandlerObj->AddressSpace.Next;

            /* Now we can delete the handler object */

            AcpiOsReleaseMutex (HandlerObj->AddressSpace.ContextMutex);
            AcpiUtRemoveReference (HandlerObj);
            goto UnlockAndExit;
        }

        /* Walk the linked list of handlers */

        LastObjPtr = &HandlerObj->AddressSpace.Next;
        HandlerObj = HandlerObj->AddressSpace.Next;
    }

    /* The handler does not exist */

    ACPI_DEBUG_PRINT ((ACPI_DB_OPREGION,
        "Unable to remove address handler %p for %s(%X), DevNode %p, obj %p\n",
        Handler, AcpiUtGetRegionName (SpaceId), SpaceId, Node, ObjDesc));

    Status = AE_NOT_EXIST;

UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiRemoveAddressSpaceHandler)
