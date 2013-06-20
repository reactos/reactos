/******************************************************************************
 *
 * Module Name: evxface - External interfaces for ACPI events
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


#define __EVXFACE_C__

#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"
#include "acevents.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evxface")


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

    /* Don't allow two handlers. */

    if (NULL != AcpiGbl_FixedEventHandlers[Event].Handler)
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
        ACPI_WARNING ((AE_INFO, "Could not enable fixed event 0x%X", Event));

        /* Remove the handler */

        AcpiGbl_FixedEventHandlers[Event].Handler = NULL;
        AcpiGbl_FixedEventHandlers[Event].Context = NULL;
    }
    else
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "Enabled fixed event %X, Handler=%p\n", Event, Handler));
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
            "Could not write to fixed event enable register 0x%X", Event));
    }
    else
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO, "Disabled fixed event %X\n", Event));
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiRemoveFixedEventHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallNotifyHandler
 *
 * PARAMETERS:  Device          - The device for which notifies will be handled
 *              HandlerType     - The type of handler:
 *                                  ACPI_SYSTEM_NOTIFY: SystemHandler (00-7f)
 *                                  ACPI_DEVICE_NOTIFY: DriverHandler (80-ff)
 *                                  ACPI_ALL_NOTIFY:  both system and device
 *              Handler         - Address of the handler
 *              Context         - Value passed to the handler on each GPE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for notifies on an ACPI device
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallNotifyHandler (
    ACPI_HANDLE             Device,
    UINT32                  HandlerType,
    ACPI_NOTIFY_HANDLER     Handler,
    void                    *Context)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *NotifyObj;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInstallNotifyHandler);


    /* Parameter validation */

    if ((!Device)  ||
        (!Handler) ||
        (HandlerType > ACPI_MAX_NOTIFY_HANDLER_TYPE))
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

    /*
     * Root Object:
     * Registering a notify handler on the root object indicates that the
     * caller wishes to receive notifications for all objects. Note that
     * only one <external> global handler can be regsitered (per notify type).
     */
    if (Device == ACPI_ROOT_OBJECT)
    {
        /* Make sure the handler is not already installed */

        if (((HandlerType & ACPI_SYSTEM_NOTIFY) &&
                AcpiGbl_SystemNotify.Handler)       ||
            ((HandlerType & ACPI_DEVICE_NOTIFY) &&
                AcpiGbl_DeviceNotify.Handler))
        {
            Status = AE_ALREADY_EXISTS;
            goto UnlockAndExit;
        }

        if (HandlerType & ACPI_SYSTEM_NOTIFY)
        {
            AcpiGbl_SystemNotify.Node    = Node;
            AcpiGbl_SystemNotify.Handler = Handler;
            AcpiGbl_SystemNotify.Context = Context;
        }

        if (HandlerType & ACPI_DEVICE_NOTIFY)
        {
            AcpiGbl_DeviceNotify.Node    = Node;
            AcpiGbl_DeviceNotify.Handler = Handler;
            AcpiGbl_DeviceNotify.Context = Context;
        }

        /* Global notify handler installed */
    }

    /*
     * All Other Objects:
     * Caller will only receive notifications specific to the target object.
     * Note that only certain object types can receive notifications.
     */
    else
    {
        /* Notifies allowed on this object? */

        if (!AcpiEvIsNotifyObject (Node))
        {
            Status = AE_TYPE;
            goto UnlockAndExit;
        }

        /* Check for an existing internal object */

        ObjDesc = AcpiNsGetAttachedObject (Node);
        if (ObjDesc)
        {
            /* Object exists - make sure there's no handler */

            if (((HandlerType & ACPI_SYSTEM_NOTIFY) &&
                    ObjDesc->CommonNotify.SystemNotify)   ||
                ((HandlerType & ACPI_DEVICE_NOTIFY) &&
                    ObjDesc->CommonNotify.DeviceNotify))
            {
                Status = AE_ALREADY_EXISTS;
                goto UnlockAndExit;
            }
        }
        else
        {
            /* Create a new object */

            ObjDesc = AcpiUtCreateInternalObject (Node->Type);
            if (!ObjDesc)
            {
                Status = AE_NO_MEMORY;
                goto UnlockAndExit;
            }

            /* Attach new object to the Node */

            Status = AcpiNsAttachObject (Device, ObjDesc, Node->Type);

            /* Remove local reference to the object */

            AcpiUtRemoveReference (ObjDesc);
            if (ACPI_FAILURE (Status))
            {
                goto UnlockAndExit;
            }
        }

        /* Install the handler */

        NotifyObj = AcpiUtCreateInternalObject (ACPI_TYPE_LOCAL_NOTIFY);
        if (!NotifyObj)
        {
            Status = AE_NO_MEMORY;
            goto UnlockAndExit;
        }

        NotifyObj->Notify.Node    = Node;
        NotifyObj->Notify.Handler = Handler;
        NotifyObj->Notify.Context = Context;

        if (HandlerType & ACPI_SYSTEM_NOTIFY)
        {
            ObjDesc->CommonNotify.SystemNotify = NotifyObj;
        }

        if (HandlerType & ACPI_DEVICE_NOTIFY)
        {
            ObjDesc->CommonNotify.DeviceNotify = NotifyObj;
        }

        if (HandlerType == ACPI_ALL_NOTIFY)
        {
            /* Extra ref if installed in both */

            AcpiUtAddReference (NotifyObj);
        }
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
 * PARAMETERS:  Device          - The device for which notifies will be handled
 *              HandlerType     - The type of handler:
 *                                  ACPI_SYSTEM_NOTIFY: SystemHandler (00-7f)
 *                                  ACPI_DEVICE_NOTIFY: DriverHandler (80-ff)
 *                                  ACPI_ALL_NOTIFY:  both system and device
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
    ACPI_OPERAND_OBJECT     *NotifyObj;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiRemoveNotifyHandler);


    /* Parameter validation */

    if ((!Device)  ||
        (!Handler) ||
        (HandlerType > ACPI_MAX_NOTIFY_HANDLER_TYPE))
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

    /* Root Object */

    if (Device == ACPI_ROOT_OBJECT)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "Removing notify handler for namespace root object\n"));

        if (((HandlerType & ACPI_SYSTEM_NOTIFY) &&
              !AcpiGbl_SystemNotify.Handler)        ||
            ((HandlerType & ACPI_DEVICE_NOTIFY) &&
              !AcpiGbl_DeviceNotify.Handler))
        {
            Status = AE_NOT_EXIST;
            goto UnlockAndExit;
        }

        if (HandlerType & ACPI_SYSTEM_NOTIFY)
        {
            AcpiGbl_SystemNotify.Node    = NULL;
            AcpiGbl_SystemNotify.Handler = NULL;
            AcpiGbl_SystemNotify.Context = NULL;
        }

        if (HandlerType & ACPI_DEVICE_NOTIFY)
        {
            AcpiGbl_DeviceNotify.Node    = NULL;
            AcpiGbl_DeviceNotify.Handler = NULL;
            AcpiGbl_DeviceNotify.Context = NULL;
        }
    }

    /* All Other Objects */

    else
    {
        /* Notifies allowed on this object? */

        if (!AcpiEvIsNotifyObject (Node))
        {
            Status = AE_TYPE;
            goto UnlockAndExit;
        }

        /* Check for an existing internal object */

        ObjDesc = AcpiNsGetAttachedObject (Node);
        if (!ObjDesc)
        {
            Status = AE_NOT_EXIST;
            goto UnlockAndExit;
        }

        /* Object exists - make sure there's an existing handler */

        if (HandlerType & ACPI_SYSTEM_NOTIFY)
        {
            NotifyObj = ObjDesc->CommonNotify.SystemNotify;
            if (!NotifyObj)
            {
                Status = AE_NOT_EXIST;
                goto UnlockAndExit;
            }

            if (NotifyObj->Notify.Handler != Handler)
            {
                Status = AE_BAD_PARAMETER;
                goto UnlockAndExit;
            }

            /* Remove the handler */

            ObjDesc->CommonNotify.SystemNotify = NULL;
            AcpiUtRemoveReference (NotifyObj);
        }

        if (HandlerType & ACPI_DEVICE_NOTIFY)
        {
            NotifyObj = ObjDesc->CommonNotify.DeviceNotify;
            if (!NotifyObj)
            {
                Status = AE_NOT_EXIST;
                goto UnlockAndExit;
            }

            if (NotifyObj->Notify.Handler != Handler)
            {
                Status = AE_BAD_PARAMETER;
                goto UnlockAndExit;
            }

            /* Remove the handler */

            ObjDesc->CommonNotify.DeviceNotify = NULL;
            AcpiUtRemoveReference (NotifyObj);
        }
    }


UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_NAMESPACE);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiRemoveNotifyHandler)


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
    ACPI_GPE_EVENT_INFO     *GpeEventInfo;
    ACPI_GPE_HANDLER_INFO   *Handler;
    ACPI_STATUS             Status;
    ACPI_CPU_FLAGS          Flags;


    ACPI_FUNCTION_TRACE (AcpiInstallGpeHandler);


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

    if ((GpeEventInfo->Flags & ACPI_GPE_DISPATCH_MASK) ==
            ACPI_GPE_DISPATCH_HANDLER)
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
    if (((Handler->OriginalFlags & ACPI_GPE_DISPATCH_METHOD) ||
         (Handler->OriginalFlags & ACPI_GPE_DISPATCH_NOTIFY)) &&
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
    GpeEventInfo->Flags |= (UINT8) (Type | ACPI_GPE_DISPATCH_HANDLER);

    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);


UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);

FreeAndExit:
    AcpiOsReleaseLock (AcpiGbl_GpeLock, Flags);
    ACPI_FREE (Handler);
    goto UnlockAndExit;
}

ACPI_EXPORT_SYMBOL (AcpiInstallGpeHandler)


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

    if ((GpeEventInfo->Flags & ACPI_GPE_DISPATCH_MASK) !=
            ACPI_GPE_DISPATCH_HANDLER)
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
    if ((Handler->OriginalFlags & ACPI_GPE_DISPATCH_METHOD) &&
        Handler->OriginallyEnabled)
    {
        (void) AcpiEvAddGpeReference (GpeEventInfo);
    }

    /* Now we can free the handler object */

    ACPI_FREE (Handler);


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

