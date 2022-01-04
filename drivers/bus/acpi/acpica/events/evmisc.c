/******************************************************************************
 *
 * Module Name: evmisc - Miscellaneous event manager support functions
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

#include "acpi.h"
#include "accommon.h"
#include "acevents.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evmisc")


/* Local prototypes */

static void ACPI_SYSTEM_XFACE
AcpiEvNotifyDispatch (
    void                    *Context);


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvIsNotifyObject
 *
 * PARAMETERS:  Node            - Node to check
 *
 * RETURN:      TRUE if notifies allowed on this object
 *
 * DESCRIPTION: Check type of node for a object that supports notifies.
 *
 *              TBD: This could be replaced by a flag bit in the node.
 *
 ******************************************************************************/

BOOLEAN
AcpiEvIsNotifyObject (
    ACPI_NAMESPACE_NODE     *Node)
{

    switch (Node->Type)
    {
    case ACPI_TYPE_DEVICE:
    case ACPI_TYPE_PROCESSOR:
    case ACPI_TYPE_THERMAL:
        /*
         * These are the ONLY objects that can receive ACPI notifications
         */
        return (TRUE);

    default:

        return (FALSE);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvQueueNotifyRequest
 *
 * PARAMETERS:  Node            - NS node for the notified object
 *              NotifyValue     - Value from the Notify() request
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Dispatch a device notification event to a previously
 *              installed handler.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvQueueNotifyRequest (
    ACPI_NAMESPACE_NODE     *Node,
    UINT32                  NotifyValue)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *HandlerListHead = NULL;
    ACPI_GENERIC_STATE      *Info;
    UINT8                   HandlerListId = 0;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_NAME (EvQueueNotifyRequest);


    /* Are Notifies allowed on this object? */

    if (!AcpiEvIsNotifyObject (Node))
    {
        return (AE_TYPE);
    }

    /* Get the correct notify list type (System or Device) */

    if (NotifyValue <= ACPI_MAX_SYS_NOTIFY)
    {
        HandlerListId = ACPI_SYSTEM_HANDLER_LIST;
    }
    else
    {
        HandlerListId = ACPI_DEVICE_HANDLER_LIST;
    }

    /* Get the notify object attached to the namespace Node */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (ObjDesc)
    {
        /* We have an attached object, Get the correct handler list */

        HandlerListHead = ObjDesc->CommonNotify.NotifyList[HandlerListId];
    }

    /*
     * If there is no notify handler (Global or Local)
     * for this object, just ignore the notify
     */
    if (!AcpiGbl_GlobalNotify[HandlerListId].Handler && !HandlerListHead)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "No notify handler for Notify, ignoring (%4.4s, %X) node %p\n",
            AcpiUtGetNodeName (Node), NotifyValue, Node));

        return (AE_OK);
    }

    /* Setup notify info and schedule the notify dispatcher */

    Info = AcpiUtCreateGenericState ();
    if (!Info)
    {
        return (AE_NO_MEMORY);
    }

    Info->Common.DescriptorType = ACPI_DESC_TYPE_STATE_NOTIFY;

    Info->Notify.Node = Node;
    Info->Notify.Value = (UINT16) NotifyValue;
    Info->Notify.HandlerListId = HandlerListId;
    Info->Notify.HandlerListHead = HandlerListHead;
    Info->Notify.Global = &AcpiGbl_GlobalNotify[HandlerListId];

    ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
        "Dispatching Notify on [%4.4s] (%s) Value 0x%2.2X (%s) Node %p\n",
        AcpiUtGetNodeName (Node), AcpiUtGetTypeName (Node->Type),
        NotifyValue, AcpiUtGetNotifyName (NotifyValue, ACPI_TYPE_ANY), Node));

    Status = AcpiOsExecute (OSL_NOTIFY_HANDLER,
        AcpiEvNotifyDispatch, Info);
    if (ACPI_FAILURE (Status))
    {
        AcpiUtDeleteGenericState (Info);
    }

    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvNotifyDispatch
 *
 * PARAMETERS:  Context         - To be passed to the notify handler
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Dispatch a device notification event to a previously
 *              installed handler.
 *
 ******************************************************************************/

static void ACPI_SYSTEM_XFACE
AcpiEvNotifyDispatch (
    void                    *Context)
{
    ACPI_GENERIC_STATE      *Info = (ACPI_GENERIC_STATE *) Context;
    ACPI_OPERAND_OBJECT     *HandlerObj;


    ACPI_FUNCTION_ENTRY ();


    /* Invoke a global notify handler if installed */

    if (Info->Notify.Global->Handler)
    {
        Info->Notify.Global->Handler (Info->Notify.Node,
            Info->Notify.Value,
            Info->Notify.Global->Context);
    }

    /* Now invoke the local notify handler(s) if any are installed */

    HandlerObj = Info->Notify.HandlerListHead;
    while (HandlerObj)
    {
        HandlerObj->Notify.Handler (Info->Notify.Node,
            Info->Notify.Value,
            HandlerObj->Notify.Context);

        HandlerObj = HandlerObj->Notify.Next[Info->Notify.HandlerListId];
    }

    /* All done with the info object */

    AcpiUtDeleteGenericState (Info);
}


#if (!ACPI_REDUCED_HARDWARE)
/******************************************************************************
 *
 * FUNCTION:    AcpiEvTerminate
 *
 * PARAMETERS:  none
 *
 * RETURN:      none
 *
 * DESCRIPTION: Disable events and free memory allocated for table storage.
 *
 ******************************************************************************/

void
AcpiEvTerminate (
    void)
{
    UINT32                  i;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (EvTerminate);


    if (AcpiGbl_EventsInitialized)
    {
        /*
         * Disable all event-related functionality. In all cases, on error,
         * print a message but obviously we don't abort.
         */

        /* Disable all fixed events */

        for (i = 0; i < ACPI_NUM_FIXED_EVENTS; i++)
        {
            Status = AcpiDisableEvent (i, 0);
            if (ACPI_FAILURE (Status))
            {
                ACPI_ERROR ((AE_INFO,
                    "Could not disable fixed event %u", (UINT32) i));
            }
        }

        /* Disable all GPEs in all GPE blocks */

        Status = AcpiEvWalkGpeList (AcpiHwDisableGpeBlock, NULL);
        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Could not disable GPEs in GPE block"));
        }

        Status = AcpiEvRemoveGlobalLockHandler ();
        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Could not remove Global Lock handler"));
        }

        AcpiGbl_EventsInitialized = FALSE;
    }

    /* Remove SCI handlers */

    Status = AcpiEvRemoveAllSciHandlers ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_ERROR ((AE_INFO,
            "Could not remove SCI handler"));
    }

    /* Deallocate all handler objects installed within GPE info structs */

    Status = AcpiEvWalkGpeList (AcpiEvDeleteGpeHandlers, NULL);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Could not delete GPE handlers"));
    }


    /* Return to original mode if necessary */

    if (AcpiGbl_OriginalMode == ACPI_SYS_MODE_LEGACY)
    {
        Status = AcpiDisable ();
        if (ACPI_FAILURE (Status))
        {
            ACPI_WARNING ((AE_INFO, "AcpiDisable failed"));
        }
    }
    return_VOID;
}

#endif /* !ACPI_REDUCED_HARDWARE */
