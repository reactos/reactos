/******************************************************************************
 *
 * Module Name: evxface - External interfaces for ACPI events
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
#include "acnamesp.h"
#include "acevents.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evxface")

#if (!ACPI_REDUCED_HARDWARE)

/* Local prototypes */

static ACPI_STATUS
AcpiEvInstallGpeHandler (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT32                  Type,
    BOOLEAN                 IsRawHandler,
    ACPI_GPE_HANDLER        Address,
    void                    *Context);

#endif


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallNotifyHandler
 *
 * PARAMETERS:  Device          - The device for which notifies will be handled
 *              HandlerType     - The type of handler:
 *                                  ACPI_SYSTEM_NOTIFY: System Handler (00-7F)
 *                                  ACPI_DEVICE_NOTIFY: Device Handler (80-FF)
 *                                  ACPI_ALL_NOTIFY:    Both System and Device
 *              Handler         - Address of the handler
 *              Context         - Value passed to the handler on each GPE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for notifications on an ACPI Device,
 *              ThermalZone, or Processor object.
 *
 * NOTES:       The Root namespace object may have only one handler for each
 *              type of notify (System/Device). Device/Thermal/Processor objects
 *              may have one device notify handler, and multiple system notify
 *              handlers.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallNotifyHandler (
    ACPI_HANDLE             Device,
    UINT32                  HandlerType,
    ACPI_NOTIFY_HANDLER     Handler,
    void                    *Context)
{
    ACPI_NAMESPACE_NODE     *Node = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE, Device);
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *HandlerObj;
    ACPI_STATUS             Status;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (AcpiInstallNotifyHandler);


    /* Parameter validation */

    if ((!Device) || (!Handler) || (!HandlerType) ||
        (HandlerType > ACPI_MAX_NOTIFY_HANDLER_TYPE))
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Root Object:
     * Registering a notify handler on the root object indicates that the
     * caller wishes to receive notifications for all objects. Note that
     * only one global handler can be registered per notify type.
     * Ensure that a handler is not already installed.
     */
    if (Device == ACPI_ROOT_OBJECT)
    {
        for (i = 0; i < ACPI_NUM_NOTIFY_TYPES; i++)
        {
            if (HandlerType & (i+1))
            {
                if (AcpiGbl_GlobalNotify[i].Handler)
                {
                    Status = AE_ALREADY_EXISTS;
                    goto UnlockAndExit;
                }

                AcpiGbl_GlobalNotify[i].Handler = Handler;
                AcpiGbl_GlobalNotify[i].Context = Context;
            }
        }

        goto UnlockAndExit; /* Global notify handler installed, all done */
    }

    /*
     * All Other Objects:
     * Caller will only receive notifications specific to the target
     * object. Note that only certain object types are allowed to
     * receive notifications.
     */

    /* Are Notifies allowed on this object? */

    if (!AcpiEvIsNotifyObject (Node))
    {
        Status = AE_TYPE;
        goto UnlockAndExit;
    }

    /* Check for an existing internal object, might not exist */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (!ObjDesc)
    {
        /* Create a new object */

        ObjDesc = AcpiUtCreateInternalObject (Node->Type);
        if (!ObjDesc)
        {
            Status = AE_NO_MEMORY;
            goto UnlockAndExit;
        }

        /* Attach new object to the Node, remove local reference */

        Status = AcpiNsAttachObject (Device, ObjDesc, Node->Type);
        AcpiUtRemoveReference (ObjDesc);
        if (ACPI_FAILURE (Status))
        {
            goto UnlockAndExit;
        }
    }

    /* Ensure that the handler is not already installed in the lists */

    for (i = 0; i < ACPI_NUM_NOTIFY_TYPES; i++)
    {
        if (HandlerType & (i+1))
        {
            HandlerObj = ObjDesc->CommonNotify.NotifyList[i];
            while (HandlerObj)
            {
                if (HandlerObj->Notify.Handler == Handler)
                {
                    Status = AE_ALREADY_EXISTS;
                    goto UnlockAndExit;
                }

                HandlerObj = HandlerObj->Notify.Next[i];
            }
        }
    }

    /* Create and populate a new notify handler object */

    HandlerObj = AcpiUtCreateInternalObject (ACPI_TYPE_LOCAL_NOTIFY);
    if (!HandlerObj)
    {
        Status = AE_NO_MEMORY;
        goto UnlockAndExit;
    }

    HandlerObj->Notify.Node = Node;
    HandlerObj->Notify.HandlerType = HandlerType;
    HandlerObj->Notify.Handler = Handler;
    HandlerObj->Notify.Context = Context;

    /* Install the handler at the list head(s) */

    for (i = 0; i < ACPI_NUM_NOTIFY_TYPES; i++)
    {
        if (HandlerType & (i+1))
        {
            HandlerObj->Notify.Next[i] =
                ObjDesc->CommonNotify.NotifyList[i];

            ObjDesc->CommonNotify.NotifyList[i] = HandlerObj;
        }
    }

    /* Add an extra reference if handler was installed in both lists */

    if (HandlerType == ACPI_ALL_NOTIFY)
    {
        AcpiUtAddReference (HandlerObj);
    }


UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallNotifyHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiRemoveNotifyHandler
 *
 * PARAMETERS:  Device          - The device for which the handler is installed
 *              HandlerType     - The type of handler:
 *                                  ACPI_SYSTEM_NOTIFY: System Handler (00-7F)
 *                                  ACPI_DEVICE_NOTIFY: Device Handler (80-FF)
 *                                  ACPI_ALL_NOTIFY:    Both System and Device
 *              Handler         - Address of the handler
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a handler for notifies on an ACPI device
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRemoveNotifyHandler (
    ACPI_HANDLE             Device,
    UINT32                  HandlerType,
    ACPI_NOTIFY_HANDLER     Handler)
{
    ACPI_NAMESPACE_NODE     *Node = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE, Device);
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *HandlerObj;
    ACPI_OPERAND_OBJECT     *PreviousHandlerObj;
    ACPI_STATUS             Status = AE_OK;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (AcpiRemoveNotifyHandler);


    /* Parameter validation */

    if ((!Device) || (!Handler) || (!HandlerType) ||
        (HandlerType > ACPI_MAX_NOTIFY_HANDLER_TYPE))
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Root Object. Global handlers are removed here */

    if (Device == ACPI_ROOT_OBJECT)
    {
        for (i = 0; i < ACPI_NUM_NOTIFY_TYPES; i++)
        {
            if (HandlerType & (i+1))
            {
                Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
                if (ACPI_FAILURE (Status))
                {
                    return_ACPI_STATUS (Status);
                }

                if (!AcpiGbl_GlobalNotify[i].Handler ||
                    (AcpiGbl_GlobalNotify[i].Handler != Handler))
                {
                    Status = AE_NOT_EXIST;
                    goto UnlockAndExit;
                }

                ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
                    "Removing global notify handler\n"));

                AcpiGbl_GlobalNotify[i].Handler = NULL;
                AcpiGbl_GlobalNotify[i].Context = NULL;

                (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);

                /* Make sure all deferred notify tasks are completed */

                AcpiOsWaitEventsComplete ();
            }
        }

        return_ACPI_STATUS (AE_OK);
    }

    /* All other objects: Are Notifies allowed on this object? */

    if (!AcpiEvIsNotifyObject (Node))
    {
        return_ACPI_STATUS (AE_TYPE);
    }

    /* Must have an existing internal object */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    /* Internal object exists. Find the handler and remove it */

    for (i = 0; i < ACPI_NUM_NOTIFY_TYPES; i++)
    {
        if (HandlerType & (i+1))
        {
            Status = AcpiUtAcquireMutex (ACPI_MTX_NAMESPACE);
            if (ACPI_FAILURE (Status))
            {
                return_ACPI_STATUS (Status);
            }

            HandlerObj = ObjDesc->CommonNotify.NotifyList[i];
            PreviousHandlerObj = NULL;

            /* Attempt to find the handler in the handler list */

            while (HandlerObj &&
                  (HandlerObj->Notify.Handler != Handler))
            {
                PreviousHandlerObj = HandlerObj;
                HandlerObj = HandlerObj->Notify.Next[i];
            }

            if (!HandlerObj)
            {
                Status = AE_NOT_EXIST;
                goto UnlockAndExit;
            }

            /* Remove the handler object from the list */

            if (PreviousHandlerObj) /* Handler is not at the list head */
            {
                PreviousHandlerObj->Notify.Next[i] =
                    HandlerObj->Notify.Next[i];
            }
            else /* Handler is at the list head */
            {
                ObjDesc->CommonNotify.NotifyList[i] =
                    HandlerObj->Notify.Next[i];
            }

            (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);

            /* Make sure all deferred notify tasks are completed */

            AcpiOsWaitEventsComplete ();
            AcpiUtRemoveReference (HandlerObj);
        }
    }

    return_ACPI_STATUS (Status);


UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiRemoveNotifyHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallExceptionHandler
 *
 * PARAMETERS:  Handler         - Pointer to the handler function for the
 *                                event
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Saves the pointer to the handler function
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallExceptionHandler (
    ACPI_EXCEPTION_HANDLER  Handler)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInstallExceptionHandler);


    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Don't allow two handlers. */

    if (AcpiGbl_ExceptionHandler)
    {
        Status = AE_ALREADY_EXISTS;
        goto Cleanup;
    }

    /* Install the handler */

    AcpiGbl_ExceptionHandler = Handler;

Cleanup:
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallExceptionHandler)


#if (!ACPI_REDUCED_HARDWARE)
/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallSciHandler
 *
 * PARAMETERS:  Address             - Address of the handler
 *              Context             - Value passed to the handler on each SCI
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for a System Control Interrupt.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallSciHandler (
    ACPI_SCI_HANDLER        Address,
    void                    *Context)
{
    ACPI_SCI_HANDLER_INFO   *NewSciHandler;
    ACPI_SCI_HANDLER_INFO   *SciHandler;
    ACPI_CPU_FLAGS          Flags;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInstallSciHandler);


    if (!Address)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Allocate and init a handler object */

    NewSciHandler = ACPI_ALLOCATE (sizeof (ACPI_SCI_HANDLER_INFO));
    if (!NewSciHandler)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    NewSciHandler->Address = Address;
    NewSciHandler->Context = Context;

    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        goto Exit;
    }

    /* Lock list during installation */

    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);
    SciHandler = AcpiGbl_SciHandlerList;

    /* Ensure handler does not already exist */

    while (SciHandler)
    {
        if (Address == SciHandler->Address)
        {
            Status = AE_ALREADY_EXISTS;
            goto UnlockAndExit;
        }

        SciHandler = SciHandler->Next;
    }

    /* Install the new handler into the global list (at head) */

    NewSciHandler->Next = AcpiGbl_SciHandlerList;
    AcpiGbl_SciHandlerList = NewSciHandler;


UnlockAndExit:

    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);

Exit:
    if (ACPI_FAILURE (Status))
    {
        ACPI_FREE (NewSciHandler);
    }
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallSciHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiRemoveSciHandler
 *
 * PARAMETERS:  Address             - Address of the handler
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a handler for a System Control Interrupt.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRemoveSciHandler (
    ACPI_SCI_HANDLER        Address)
{
    ACPI_SCI_HANDLER_INFO   *PrevSciHandler;
    ACPI_SCI_HANDLER_INFO   *NextSciHandler;
    ACPI_CPU_FLAGS          Flags;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiRemoveSciHandler);


    if (!Address)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Remove the SCI handler with lock */

    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

    PrevSciHandler = NULL;
    NextSciHandler = AcpiGbl_SciHandlerList;
    while (NextSciHandler)
    {
        if (NextSciHandler->Address == Address)
        {
            /* Unlink and free the SCI handler info block */

            if (PrevSciHandler)
            {
                PrevSciHandler->Next = NextSciHandler->Next;
            }
            else
            {
                AcpiGbl_SciHandlerList = NextSciHandler->Next;
            }

            AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
            ACPI_FREE (NextSciHandler);
            goto UnlockAndExit;
        }

        PrevSciHandler = NextSciHandler;
        NextSciHandler = NextSciHandler->Next;
    }

    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    Status = AE_NOT_EXIST;


UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiRemoveSciHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallGlobalEventHandler
 *
 * PARAMETERS:  Handler         - Pointer to the global event handler function
 *              Context         - Value passed to the handler on each event
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Saves the pointer to the handler function. The global handler
 *              is invoked upon each incoming GPE and Fixed Event. It is
 *              invoked at interrupt level at the time of the event dispatch.
 *              Can be used to update event counters, etc.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallGlobalEventHandler (
    ACPI_GBL_EVENT_HANDLER  Handler,
    void                    *Context)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInstallGlobalEventHandler);


    /* Parameter validation */

    if (!Handler)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Don't allow two handlers. */

    if (AcpiGbl_GlobalEventHandler)
    {
        Status = AE_ALREADY_EXISTS;
        goto Cleanup;
    }

    AcpiGbl_GlobalEventHandler = Handler;
    AcpiGbl_GlobalEventHandlerContext = Context;


Cleanup:
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallGlobalEventHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallFixedEventHandler
 *
 * PARAMETERS:  Event           - Event type to enable.
 *              Handler         - Pointer to the handler function for the
 *                                event
 *              Context         - Value passed to the handler on each GPE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Saves the pointer to the handler function and then enables the
 *              event.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallFixedEventHandler (
    UINT32                  Event,
    ACPI_EVENT_HANDLER      Handler,
    void                    *Context)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInstallFixedEventHandler);


    /* Parameter validation */

    if (Event > ACPI_EVENT_MAX)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Do not allow multiple handlers */

    if (AcpiGbl_FixedEventHandlers[Event].Handler)
    {
        Status = AE_ALREADY_EXISTS;
        goto Cleanup;
    }

    /* Install the handler before enabling the event */

    AcpiGbl_FixedEventHandlers[Event].Handler = Handler;
    AcpiGbl_FixedEventHandlers[Event].Context = Context;

    Status = AcpiEnableEvent (Event, 0);
    if (ACPI_FAILURE (Status))
    {
        ACPI_WARNING ((AE_INFO,
            "Could not enable fixed event - %s (%u)",
            AcpiUtGetEventName (Event), Event));

        /* Remove the handler */

        AcpiGbl_FixedEventHandlers[Event].Handler = NULL;
        AcpiGbl_FixedEventHandlers[Event].Context = NULL;
    }
    else
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "Enabled fixed event %s (%X), Handler=%p\n",
            AcpiUtGetEventName (Event), Event, Handler));
    }


Cleanup:
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallFixedEventHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiRemoveFixedEventHandler
 *
 * PARAMETERS:  Event           - Event type to disable.
 *              Handler         - Address of the handler
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Disables the event and unregisters the event handler.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRemoveFixedEventHandler (
    UINT32                  Event,
    ACPI_EVENT_HANDLER      Handler)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (AcpiRemoveFixedEventHandler);


    /* Parameter validation */

    if (Event > ACPI_EVENT_MAX)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Disable the event before removing the handler */

    Status = AcpiDisableEvent (Event, 0);

    /* Always Remove the handler */

    AcpiGbl_FixedEventHandlers[Event].Handler = NULL;
    AcpiGbl_FixedEventHandlers[Event].Context = NULL;

    if (ACPI_FAILURE (Status))
    {
        ACPI_WARNING ((AE_INFO,
            "Could not disable fixed event - %s (%u)",
            AcpiUtGetEventName (Event), Event));
    }
    else
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "Disabled fixed event - %s (%X)\n",
            AcpiUtGetEventName (Event), Event));
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiRemoveFixedEventHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvInstallGpeHandler
 *
 * PARAMETERS:  GpeDevice       - Namespace node for the GPE (NULL for FADT
 *                                defined GPEs)
 *              GpeNumber       - The GPE number within the GPE block
 *              Type            - Whether this GPE should be treated as an
 *                                edge- or level-triggered interrupt.
 *              IsRawHandler    - Whether this GPE should be handled using
 *                                the special GPE handler mode.
 *              Address         - Address of the handler
 *              Context         - Value passed to the handler on each GPE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Internal function to install a handler for a General Purpose
 *              Event.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiEvInstallGpeHandler (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT32                  Type,
    BOOLEAN                 IsRawHandler,
    ACPI_GPE_HANDLER        Address,
    void                    *Context)
{
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_GPE_HANDLER_INFO   *Handler;
    ACPI_STATUS             Status;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (EvInstallGpeHandler);


    /* Parameter validation */

    if ((!Address) || (Type & ~ACPI_GPE_XRUPT_TYPE_MASK))
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Allocate and init handler object (before lock) */

    Handler = ACPI_ALLOCATE_ZEROED (sizeof (ACPI_GPE_HANDLER_INFO));
    if (!Handler)
    {
        Status = AE_NO_MEMORY;
        goto UnlockAndExit;
    }

    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (!GpeEventInfo)
    {
        Status = AE_BAD_PARAMETER;
        goto FreeAndExit;
    }

    /* Make sure that there isn't a handler there already */

    if ((ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags) ==
            ACPI_GPE_DISPATCH_HANDLER) ||
        (ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags) ==
            ACPI_GPE_DISPATCH_RAW_HANDLER))
    {
        Status = AE_ALREADY_EXISTS;
        goto FreeAndExit;
    }

    Handler->Address = Address;
    Handler->Context = Context;
    Handler->MethodNode = GpeEventInfo->Dispatch.MethodNode;
    Handler->OriginalFlags = (UINT8) (GpeEventInfo->Flags &
        (ACPI_GPE_XRUPT_TYPE_MASK | ACPI_GPE_DISPATCH_MASK));

    /*
     * If the GPE is associated with a method, it may have been enabled
     * automatically during initialization, in which case it has to be
     * disabled now to avoid spurious execution of the handler.
     */
    if (((ACPI_GPE_DISPATCH_TYPE (Handler->OriginalFlags) ==
            ACPI_GPE_DISPATCH_METHOD) ||
         (ACPI_GPE_DISPATCH_TYPE (Handler->OriginalFlags) ==
            ACPI_GPE_DISPATCH_NOTIFY)) &&
        GpeEventInfo->RuntimeCount)
    {
        Handler->OriginallyEnabled = TRUE;
        (void) AcpiEvRemoveGpeReference (GpeEventInfo);

        /* Sanity check of original type against new type */

        if (Type != (UINT32) (GpeEventInfo->Flags & ACPI_GPE_XRUPT_TYPE_MASK))
        {
            ACPI_WARNING ((AE_INFO, "GPE type mismatch (level/edge)"));
        }
    }

    /* Install the handler */

    GpeEventInfo->Dispatch.Handler = Handler;

    /* Setup up dispatch flags to indicate handler (vs. method/notify) */

    GpeEventInfo->Flags &= ~(ACPI_GPE_XRUPT_TYPE_MASK | ACPI_GPE_DISPATCH_MASK);
    GpeEventInfo->Flags |= (UINT8) (Type | (IsRawHandler ?
        ACPI_GPE_DISPATCH_RAW_HANDLER : ACPI_GPE_DISPATCH_HANDLER));

    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);


UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);

FreeAndExit:
    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    ACPI_FREE (Handler);
    goto UnlockAndExit;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallGpeHandler
 *
 * PARAMETERS:  GpeDevice       - Namespace node for the GPE (NULL for FADT
 *                                defined GPEs)
 *              GpeNumber       - The GPE number within the GPE block
 *              Type            - Whether this GPE should be treated as an
 *                                edge- or level-triggered interrupt.
 *              Address         - Address of the handler
 *              Context         - Value passed to the handler on each GPE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for a General Purpose Event.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallGpeHandler (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT32                  Type,
    ACPI_GPE_HANDLER        Address,
    void                    *Context)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInstallGpeHandler);


    Status = AcpiEvInstallGpeHandler (GpeDevice, GpeNumber, Type,
        FALSE, Address, Context);

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallGpeHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallGpeRawHandler
 *
 * PARAMETERS:  GpeDevice       - Namespace node for the GPE (NULL for FADT
 *                                defined GPEs)
 *              GpeNumber       - The GPE number within the GPE block
 *              Type            - Whether this GPE should be treated as an
 *                                edge- or level-triggered interrupt.
 *              Address         - Address of the handler
 *              Context         - Value passed to the handler on each GPE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for a General Purpose Event.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallGpeRawHandler (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    UINT32                  Type,
    ACPI_GPE_HANDLER        Address,
    void                    *Context)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInstallGpeRawHandler);


    Status = AcpiEvInstallGpeHandler (GpeDevice, GpeNumber, Type,
        TRUE, Address, Context);

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallGpeRawHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiRemoveGpeHandler
 *
 * PARAMETERS:  GpeDevice       - Namespace node for the GPE (NULL for FADT
 *                                defined GPEs)
 *              GpeNumber       - The event to remove a handler
 *              Address         - Address of the handler
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a handler for a General Purpose AcpiEvent.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRemoveGpeHandler (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    ACPI_GPE_HANDLER        Address)
{
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_GPE_HANDLER_INFO   *Handler;
    ACPI_STATUS             Status;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (AcpiRemoveGpeHandler);


    /* Parameter validation */

    if (!Address)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);

    /* Ensure that we have a valid GPE number */

    GpeEventInfo = AcpiEvGetGpeEventInfo (GpeDevice, GpeNumber);
    if (!GpeEventInfo)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    /* Make sure that a handler is indeed installed */

    if ((ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags) !=
            ACPI_GPE_DISPATCH_HANDLER) &&
        (ACPI_GPE_DISPATCH_TYPE (GpeEventInfo->Flags) !=
            ACPI_GPE_DISPATCH_RAW_HANDLER))
    {
        Status = AE_NOT_EXIST;
        goto UnlockAndExit;
    }

    /* Make sure that the installed handler is the same */

    if (GpeEventInfo->Dispatch.Handler->Address != Address)
    {
        Status = AE_BAD_PARAMETER;
        goto UnlockAndExit;
    }

    /* Remove the handler */

    Handler = GpeEventInfo->Dispatch.Handler;
    GpeEventInfo->Dispatch.Handler = NULL;

    /* Restore Method node (if any), set dispatch flags */

    GpeEventInfo->Dispatch.MethodNode = Handler->MethodNode;
    GpeEventInfo->Flags &=
        ~(ACPI_GPE_XRUPT_TYPE_MASK | ACPI_GPE_DISPATCH_MASK);
    GpeEventInfo->Flags |= Handler->OriginalFlags;

    /*
     * If the GPE was previously associated with a method and it was
     * enabled, it should be enabled at this point to restore the
     * post-initialization configuration.
     */
    if (((ACPI_GPE_DISPATCH_TYPE (Handler->OriginalFlags) ==
            ACPI_GPE_DISPATCH_METHOD) ||
         (ACPI_GPE_DISPATCH_TYPE (Handler->OriginalFlags) ==
            ACPI_GPE_DISPATCH_NOTIFY)) &&
        Handler->OriginallyEnabled)
    {
        (void) AcpiEvAddGpeReference (GpeEventInfo);
        if (ACPI_GPE_IS_POLLING_NEEDED (GpeEventInfo))
        {
            /* Poll edge triggered GPEs to handle existing events */

            AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
            (void) AcpiEvDetectGpe (
                GpeDevice, GpeEventInfo, GpeNumber);
            Flags = AcpiOsAcquireLock (AcpiGbl_GpeLock);
        }
    }

    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);

    /* Make sure all deferred GPE tasks are completed */

    AcpiOsWaitEventsComplete ();

    /* Now we can free the handler object */

    ACPI_FREE (Handler);
    return_ACPI_STATUS (Status);

UnlockAndExit:
    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiRemoveGpeHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiAcquireGlobalLock
 *
 * PARAMETERS:  Timeout         - How long the caller is willing to wait
 *              Handle          - Where the handle to the lock is returned
 *                                (if acquired)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Acquire the ACPI Global Lock
 *
 * Note: Allows callers with the same thread ID to acquire the global lock
 * multiple times. In other words, externally, the behavior of the global lock
 * is identical to an AML mutex. On the first acquire, a new handle is
 * returned. On any subsequent calls to acquire by the same thread, the same
 * handle is returned.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiAcquireGlobalLock (
    UINT16                  Timeout,
    UINT32                  *Handle)
{
    ACPI_STATUS             Status;


    if (!Handle)
    {
        return (AE_BAD_PARAMETER);
    }

    /* Must lock interpreter to prevent race conditions */

    AcpiExEnterInterpreter ();

    Status = AcpiExAcquireMutexObject (Timeout,
        AcpiGbl_GlobalLockMutex, AcpiOsGetThreadId ());

    if (ACPI_SUCCESS (Status))
    {
        /* Return the global lock handle (updated in AcpiEvAcquireGlobalLock) */

        *Handle = AcpiGbl_GlobalLockHandle;
    }

    AcpiExExitInterpreter ();
    return (Status);
}

ACPI_EXPORT_SYMBOL (AcpiAcquireGlobalLock)


/*******************************************************************************
 *
 * FUNCTION:    AcpiReleaseGlobalLock
 *
 * PARAMETERS:  Handle      - Returned from AcpiAcquireGlobalLock
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Release the ACPI Global Lock. The handle must be valid.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiReleaseGlobalLock (
    UINT32                  Handle)
{
    ACPI_STATUS             Status;


    if (!Handle || (Handle != AcpiGbl_GlobalLockHandle))
    {
        return (AE_NOT_ACQUIRED);
    }

    Status = AcpiExReleaseMutexObject (AcpiGbl_GlobalLockMutex);
    return (Status);
}

ACPI_EXPORT_SYMBOL (AcpiReleaseGlobalLock)

#endif /* !ACPI_REDUCED_HARDWARE */
