/******************************************************************************
 *
 * Module Name: evmisc - Miscellaneous event manager support functions
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

#include "acpi.h"
#include "accommon.h"
#include "acevents.h"
#include "acnamesp.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_EVENTS
        ACPI_MODULE_NAME    ("evmisc")


/* Local prototypes */

static void ACPI_SYSTEM_XFACE
AcpiEvNotifyDispatch (
    void                    *Context);

static UINT32
AcpiEvGlobalLockHandler (
    void                    *Context);

static ACPI_STATUS
AcpiEvRemoveGlobalLockHandler (
    void);


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
    ACPI_OPERAND_OBJECT     *HandlerObj = NULL;
    ACPI_GENERIC_STATE      *NotifyInfo;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_NAME (EvQueueNotifyRequest);


    /*
     * For value 3 (Ejection Request), some device method may need to be run.
     * For value 2 (Device Wake) if _PRW exists, the _PS0 method may need
     *   to be run.
     * For value 0x80 (Status Change) on the power button or sleep button,
     *   initiate soft-off or sleep operation?
     */
    ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
        "Dispatching Notify on [%4.4s] Node %p Value 0x%2.2X (%s)\n",
        AcpiUtGetNodeName (Node), Node, NotifyValue,
        AcpiUtGetNotifyName (NotifyValue)));

    /* Get the notify object attached to the NS Node */

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (ObjDesc)
    {
        /* We have the notify object, Get the right handler */

        switch (Node->Type)
        {
        /* Notify allowed only on these types */

        case ACPI_TYPE_DEVICE:
        case ACPI_TYPE_THERMAL:
        case ACPI_TYPE_PROCESSOR:

            if (NotifyValue <= ACPI_MAX_SYS_NOTIFY)
            {
                HandlerObj = ObjDesc->CommonNotify.SystemNotify;
            }
            else
            {
                HandlerObj = ObjDesc->CommonNotify.DeviceNotify;
            }
            break;

        default:

            /* All other types are not supported */

            return (AE_TYPE);
        }
    }

    /*
     * If there is any handler to run, schedule the dispatcher.
     * Check for:
     * 1) Global system notify handler
     * 2) Global device notify handler
     * 3) Per-device notify handler
     */
    if ((AcpiGbl_SystemNotify.Handler &&
            (NotifyValue <= ACPI_MAX_SYS_NOTIFY)) ||
        (AcpiGbl_DeviceNotify.Handler &&
            (NotifyValue > ACPI_MAX_SYS_NOTIFY))  ||
        HandlerObj)
    {
        NotifyInfo = AcpiUtCreateGenericState ();
        if (!NotifyInfo)
        {
            return (AE_NO_MEMORY);
        }

        if (!HandlerObj)
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
                "Executing system notify handler for Notify (%4.4s, %X) "
                "node %p\n",
                AcpiUtGetNodeName (Node), NotifyValue, Node));
        }

        NotifyInfo->Common.DescriptorType = ACPI_DESC_TYPE_STATE_NOTIFY;
        NotifyInfo->Notify.Node = Node;
        NotifyInfo->Notify.Value = (UINT16) NotifyValue;
        NotifyInfo->Notify.HandlerObj = HandlerObj;

        Status = AcpiOsExecute (
                    OSL_NOTIFY_HANDLER, AcpiEvNotifyDispatch, NotifyInfo);
        if (ACPI_FAILURE (Status))
        {
            AcpiUtDeleteGenericState (NotifyInfo);
        }
    }
    else
    {
        /* There is no notify handler (per-device or system) for this device */

        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "No notify handler for Notify (%4.4s, %X) node %p\n",
            AcpiUtGetNodeName (Node), NotifyValue, Node));
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
    ACPI_GENERIC_STATE      *NotifyInfo = (ACPI_GENERIC_STATE *) Context;
    ACPI_NOTIFY_HANDLER     GlobalHandler = NULL;
    void                    *GlobalContext = NULL;
    ACPI_OPERAND_OBJECT     *HandlerObj;


    ACPI_FUNCTION_ENTRY ();


    /*
     * We will invoke a global notify handler if installed. This is done
     * _before_ we invoke the per-device handler attached to the device.
     */
    if (NotifyInfo->Notify.Value <= ACPI_MAX_SYS_NOTIFY)
    {
        /* Global system notification handler */

        if (AcpiGbl_SystemNotify.Handler)
        {
            GlobalHandler = AcpiGbl_SystemNotify.Handler;
            GlobalContext = AcpiGbl_SystemNotify.Context;
        }
    }
    else
    {
        /* Global driver notification handler */

        if (AcpiGbl_DeviceNotify.Handler)
        {
            GlobalHandler = AcpiGbl_DeviceNotify.Handler;
            GlobalContext = AcpiGbl_DeviceNotify.Context;
        }
    }

    /* Invoke the system handler first, if present */

    if (GlobalHandler)
    {
        GlobalHandler (NotifyInfo->Notify.Node, NotifyInfo->Notify.Value,
            GlobalContext);
    }

    /* Now invoke the per-device handler, if present */

    HandlerObj = NotifyInfo->Notify.HandlerObj;
    if (HandlerObj)
    {
        HandlerObj->Notify.Handler (NotifyInfo->Notify.Node,
            NotifyInfo->Notify.Value,
            HandlerObj->Notify.Context);
    }

    /* All done with the info object */

    AcpiUtDeleteGenericState (NotifyInfo);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvGlobalLockHandler
 *
 * PARAMETERS:  Context         - From thread interface, not used
 *
 * RETURN:      ACPI_INTERRUPT_HANDLED
 *
 * DESCRIPTION: Invoked directly from the SCI handler when a global lock
 *              release interrupt occurs. Attempt to acquire the global lock,
 *              if successful, signal the thread waiting for the lock.
 *
 * NOTE: Assumes that the semaphore can be signaled from interrupt level. If
 * this is not possible for some reason, a separate thread will have to be
 * scheduled to do this.
 *
 ******************************************************************************/

static UINT32
AcpiEvGlobalLockHandler (
    void                    *Context)
{
    BOOLEAN                 Acquired = FALSE;
    ACPI_STATUS             Status;


    /*
     * Attempt to get the lock.
     *
     * If we don't get it now, it will be marked pending and we will
     * take another interrupt when it becomes free.
     */
    ACPI_ACQUIRE_GLOBAL_LOCK (AcpiGbl_FACS, Acquired);
    if (Acquired)
    {
        /* Got the lock, now wake the thread waiting for it */

        AcpiGbl_GlobalLockAcquired = TRUE;

        /* Send a unit to the semaphore */

        Status = AcpiOsSignalSemaphore (AcpiGbl_GlobalLockSemaphore, 1);
        if (ACPI_FAILURE (Status))
        {
            ACPI_ERROR ((AE_INFO, "Could not signal Global Lock semaphore"));
        }
    }

    return (ACPI_INTERRUPT_HANDLED);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvInitGlobalLockHandler
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for the global lock release event
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvInitGlobalLockHandler (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (EvInitGlobalLockHandler);


    /* Attempt installation of the global lock handler */

    Status = AcpiInstallFixedEventHandler (ACPI_EVENT_GLOBAL,
                AcpiEvGlobalLockHandler, NULL);

    /*
     * If the global lock does not exist on this platform, the attempt to
     * enable GBL_STATUS will fail (the GBL_ENABLE bit will not stick).
     * Map to AE_OK, but mark global lock as not present. Any attempt to
     * actually use the global lock will be flagged with an error.
     */
    if (Status == AE_NO_HARDWARE_RESPONSE)
    {
        ACPI_ERROR ((AE_INFO,
            "No response from Global Lock hardware, disabling lock"));

        AcpiGbl_GlobalLockPresent = FALSE;
        return_ACPI_STATUS (AE_OK);
    }

    AcpiGbl_GlobalLockPresent = TRUE;
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvRemoveGlobalLockHandler
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove the handler for the Global Lock
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiEvRemoveGlobalLockHandler (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (EvRemoveGlobalLockHandler);

    AcpiGbl_GlobalLockPresent = FALSE;
    Status = AcpiRemoveFixedEventHandler (ACPI_EVENT_GLOBAL,
                AcpiEvGlobalLockHandler);

    return_ACPI_STATUS (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiEvAcquireGlobalLock
 *
 * PARAMETERS:  Timeout         - Max time to wait for the lock, in millisec.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Attempt to gain ownership of the Global Lock.
 *
 * MUTEX:       Interpreter must be locked
 *
 * Note: The original implementation allowed multiple threads to "acquire" the
 * Global Lock, and the OS would hold the lock until the last thread had
 * released it. However, this could potentially starve the BIOS out of the
 * lock, especially in the case where there is a tight handshake between the
 * Embedded Controller driver and the BIOS. Therefore, this implementation
 * allows only one thread to acquire the HW Global Lock at a time, and makes
 * the global lock appear as a standard mutex on the OS side.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiEvAcquireGlobalLock (
    UINT16                  Timeout)
{
    ACPI_STATUS             Status = AE_OK;
    BOOLEAN                 Acquired = FALSE;


    ACPI_FUNCTION_TRACE (EvAcquireGlobalLock);


    /*
     * Only one thread can acquire the GL at a time, the GlobalLockMutex
     * enforces this. This interface releases the interpreter if we must wait.
     */
    Status = AcpiExSystemWaitMutex (AcpiGbl_GlobalLockMutex->Mutex.OsMutex,
                Timeout);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Update the global lock handle and check for wraparound. The handle is
     * only used for the external global lock interfaces, but it is updated
     * here to properly handle the case where a single thread may acquire the
     * lock via both the AML and the AcpiAcquireGlobalLock interfaces. The
     * handle is therefore updated on the first acquire from a given thread
     * regardless of where the acquisition request originated.
     */
    AcpiGbl_GlobalLockHandle++;
    if (AcpiGbl_GlobalLockHandle == 0)
    {
        AcpiGbl_GlobalLockHandle = 1;
    }

    /*
     * Make sure that a global lock actually exists. If not, just treat the
     * lock as a standard mutex.
     */
    if (!AcpiGbl_GlobalLockPresent)
    {
        AcpiGbl_GlobalLockAcquired = TRUE;
        return_ACPI_STATUS (AE_OK);
    }

    /* Attempt to acquire the actual hardware lock */

    ACPI_ACQUIRE_GLOBAL_LOCK (AcpiGbl_FACS, Acquired);
    if (Acquired)
    {
       /* We got the lock */

        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Acquired hardware Global Lock\n"));

        AcpiGbl_GlobalLockAcquired = TRUE;
        return_ACPI_STATUS (AE_OK);
    }

    /*
     * Did not get the lock. The pending bit was set above, and we must now
     * wait until we get the global lock released interrupt.
     */
    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Waiting for hardware Global Lock\n"));

    /*
     * Wait for handshake with the global lock interrupt handler.
     * This interface releases the interpreter if we must wait.
     */
    Status = AcpiExSystemWaitSemaphore (AcpiGbl_GlobalLockSemaphore,
                ACPI_WAIT_FOREVER);

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiEvReleaseGlobalLock
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Releases ownership of the Global Lock.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiEvReleaseGlobalLock (
    void)
{
    BOOLEAN                 Pending = FALSE;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (EvReleaseGlobalLock);


    /* Lock must be already acquired */

    if (!AcpiGbl_GlobalLockAcquired)
    {
        ACPI_WARNING ((AE_INFO,
            "Cannot release the ACPI Global Lock, it has not been acquired"));
        return_ACPI_STATUS (AE_NOT_ACQUIRED);
    }

    if (AcpiGbl_GlobalLockPresent)
    {
        /* Allow any thread to release the lock */

        ACPI_RELEASE_GLOBAL_LOCK (AcpiGbl_FACS, Pending);

        /*
         * If the pending bit was set, we must write GBL_RLS to the control
         * register
         */
        if (Pending)
        {
            Status = AcpiWriteBitRegister (
                        ACPI_BITREG_GLOBAL_LOCK_RELEASE, ACPI_ENABLE_EVENT);
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Released hardware Global Lock\n"));
    }

    AcpiGbl_GlobalLockAcquired = FALSE;

    /* Release the local GL mutex */

    AcpiOsReleaseMutex (AcpiGbl_GlobalLockMutex->Mutex.OsMutex);
    return_ACPI_STATUS (Status);
}


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
                    "Could not disable fixed event %d", (UINT32) i));
            }
        }

        /* Disable all GPEs in all GPE blocks */

        Status = AcpiEvWalkGpeList (AcpiHwDisableGpeBlock, NULL);

        /* Remove SCI handler */

        Status = AcpiEvRemoveSciHandler ();
        if (ACPI_FAILURE(Status))
        {
            ACPI_ERROR ((AE_INFO,
                "Could not remove SCI handler"));
        }

        Status = AcpiEvRemoveGlobalLockHandler ();
        if (ACPI_FAILURE(Status))
        {
            ACPI_ERROR ((AE_INFO,
                "Could not remove Global Lock handler"));
        }
    }

    /* Deallocate all handler objects installed within GPE info structs */

    Status = AcpiEvWalkGpeList (AcpiEvDeleteGpeHandlers, NULL);

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

