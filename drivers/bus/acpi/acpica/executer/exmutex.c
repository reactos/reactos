/******************************************************************************
 *
 * Module Name: exmutex - ASL Mutex Acquire/Release functions
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
#include "acinterp.h"
#include "acevents.h"

#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exmutex")

/* Local prototypes */

static void
AcpiExLinkMutex (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_THREAD_STATE       *Thread);


/*******************************************************************************
 *
 * FUNCTION:    AcpiExUnlinkMutex
 *
 * PARAMETERS:  ObjDesc             - The mutex to be unlinked
 *
 * RETURN:      None
 *
 * DESCRIPTION: Remove a mutex from the "AcquiredMutex" list
 *
 ******************************************************************************/

void
AcpiExUnlinkMutex (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_THREAD_STATE       *Thread = ObjDesc->Mutex.OwnerThread;


    if (!Thread)
    {
        return;
    }

    /* Doubly linked list */

    if (ObjDesc->Mutex.Next)
    {
        (ObjDesc->Mutex.Next)->Mutex.Prev = ObjDesc->Mutex.Prev;
    }

    if (ObjDesc->Mutex.Prev)
    {
        (ObjDesc->Mutex.Prev)->Mutex.Next = ObjDesc->Mutex.Next;

        /*
         * Migrate the previous sync level associated with this mutex to
         * the previous mutex on the list so that it may be preserved.
         * This handles the case where several mutexes have been acquired
         * at the same level, but are not released in opposite order.
         */
        (ObjDesc->Mutex.Prev)->Mutex.OriginalSyncLevel =
            ObjDesc->Mutex.OriginalSyncLevel;
    }
    else
    {
        Thread->AcquiredMutexList = ObjDesc->Mutex.Next;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExLinkMutex
 *
 * PARAMETERS:  ObjDesc             - The mutex to be linked
 *              Thread              - Current executing thread object
 *
 * RETURN:      None
 *
 * DESCRIPTION: Add a mutex to the "AcquiredMutex" list for this walk
 *
 ******************************************************************************/

static void
AcpiExLinkMutex (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_THREAD_STATE       *Thread)
{
    ACPI_OPERAND_OBJECT     *ListHead;


    ListHead = Thread->AcquiredMutexList;

    /* This object will be the first object in the list */

    ObjDesc->Mutex.Prev = NULL;
    ObjDesc->Mutex.Next = ListHead;

    /* Update old first object to point back to this object */

    if (ListHead)
    {
        ListHead->Mutex.Prev = ObjDesc;
    }

    /* Update list head */

    Thread->AcquiredMutexList = ObjDesc;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExAcquireMutexObject
 *
 * PARAMETERS:  Timeout             - Timeout in milliseconds
 *              ObjDesc             - Mutex object
 *              ThreadId            - Current thread state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Acquire an AML mutex, low-level interface. Provides a common
 *              path that supports multiple acquires by the same thread.
 *
 * MUTEX:       Interpreter must be locked
 *
 * NOTE: This interface is called from three places:
 * 1) From AcpiExAcquireMutex, via an AML Acquire() operator
 * 2) From AcpiExAcquireGlobalLock when an AML Field access requires the
 *    global lock
 * 3) From the external interface, AcpiAcquireGlobalLock
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExAcquireMutexObject (
    UINT16                  Timeout,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_THREAD_ID          ThreadId)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_PTR (ExAcquireMutexObject, ObjDesc);


    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Support for multiple acquires by the owning thread */

    if (ObjDesc->Mutex.ThreadId == ThreadId)
    {
        /*
         * The mutex is already owned by this thread, just increment the
         * acquisition depth
         */
        ObjDesc->Mutex.AcquisitionDepth++;
        return_ACPI_STATUS (AE_OK);
    }

    /* Acquire the mutex, wait if necessary. Special case for Global Lock */

    if (ObjDesc == AcpiGbl_GlobalLockMutex)
    {
        Status = AcpiEvAcquireGlobalLock (Timeout);
    }
    else
    {
        Status = AcpiExSystemWaitMutex (ObjDesc->Mutex.OsMutex, Timeout);
    }

    if (ACPI_FAILURE (Status))
    {
        /* Includes failure from a timeout on TimeDesc */

        return_ACPI_STATUS (Status);
    }

    /* Acquired the mutex: update mutex object */

    ObjDesc->Mutex.ThreadId = ThreadId;
    ObjDesc->Mutex.AcquisitionDepth = 1;
    ObjDesc->Mutex.OriginalSyncLevel = 0;
    ObjDesc->Mutex.OwnerThread = NULL;      /* Used only for AML Acquire() */

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExAcquireMutex
 *
 * PARAMETERS:  TimeDesc            - Timeout integer
 *              ObjDesc             - Mutex object
 *              WalkState           - Current method execution state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Acquire an AML mutex
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExAcquireMutex (
    ACPI_OPERAND_OBJECT     *TimeDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_PTR (ExAcquireMutex, ObjDesc);


    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Must have a valid thread state struct */

    if (!WalkState->Thread)
    {
        ACPI_ERROR ((AE_INFO,
            "Cannot acquire Mutex [%4.4s], null thread info",
            AcpiUtGetNodeName (ObjDesc->Mutex.Node)));
        return_ACPI_STATUS (AE_AML_INTERNAL);
    }

    /*
     * Current sync level must be less than or equal to the sync level
     * of the mutex. This mechanism provides some deadlock prevention.
     */
    if (WalkState->Thread->CurrentSyncLevel > ObjDesc->Mutex.SyncLevel)
    {
        ACPI_ERROR ((AE_INFO,
            "Cannot acquire Mutex [%4.4s], "
            "current SyncLevel is too large (%u)",
            AcpiUtGetNodeName (ObjDesc->Mutex.Node),
            WalkState->Thread->CurrentSyncLevel));
        return_ACPI_STATUS (AE_AML_MUTEX_ORDER);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
        "Acquiring: Mutex SyncLevel %u, Thread SyncLevel %u, "
        "Depth %u TID %p\n",
        ObjDesc->Mutex.SyncLevel, WalkState->Thread->CurrentSyncLevel,
        ObjDesc->Mutex.AcquisitionDepth, WalkState->Thread));

    Status = AcpiExAcquireMutexObject ((UINT16) TimeDesc->Integer.Value,
        ObjDesc, WalkState->Thread->ThreadId);

    if (ACPI_SUCCESS (Status) && ObjDesc->Mutex.AcquisitionDepth == 1)
    {
        /* Save Thread object, original/current sync levels */

        ObjDesc->Mutex.OwnerThread = WalkState->Thread;
        ObjDesc->Mutex.OriginalSyncLevel =
            WalkState->Thread->CurrentSyncLevel;
        WalkState->Thread->CurrentSyncLevel =
            ObjDesc->Mutex.SyncLevel;

        /* Link the mutex to the current thread for force-unlock at method exit */

        AcpiExLinkMutex (ObjDesc, WalkState->Thread);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
        "Acquired: Mutex SyncLevel %u, Thread SyncLevel %u, Depth %u\n",
        ObjDesc->Mutex.SyncLevel, WalkState->Thread->CurrentSyncLevel,
        ObjDesc->Mutex.AcquisitionDepth));

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExReleaseMutexObject
 *
 * PARAMETERS:  ObjDesc             - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Release a previously acquired Mutex, low level interface.
 *              Provides a common path that supports multiple releases (after
 *              previous multiple acquires) by the same thread.
 *
 * MUTEX:       Interpreter must be locked
 *
 * NOTE: This interface is called from three places:
 * 1) From AcpiExReleaseMutex, via an AML Acquire() operator
 * 2) From AcpiExReleaseGlobalLock when an AML Field access requires the
 *    global lock
 * 3) From the external interface, AcpiReleaseGlobalLock
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExReleaseMutexObject (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (ExReleaseMutexObject);


    if (ObjDesc->Mutex.AcquisitionDepth == 0)
    {
        return_ACPI_STATUS (AE_NOT_ACQUIRED);
    }

    /* Match multiple Acquires with multiple Releases */

    ObjDesc->Mutex.AcquisitionDepth--;
    if (ObjDesc->Mutex.AcquisitionDepth != 0)
    {
        /* Just decrement the depth and return */

        return_ACPI_STATUS (AE_OK);
    }

    if (ObjDesc->Mutex.OwnerThread)
    {
        /* Unlink the mutex from the owner's list */

        AcpiExUnlinkMutex (ObjDesc);
        ObjDesc->Mutex.OwnerThread = NULL;
    }

    /* Release the mutex, special case for Global Lock */

    if (ObjDesc == AcpiGbl_GlobalLockMutex)
    {
        Status = AcpiEvReleaseGlobalLock ();
    }
    else
    {
        AcpiOsReleaseMutex (ObjDesc->Mutex.OsMutex);
    }

    /* Clear mutex info */

    ObjDesc->Mutex.ThreadId = 0;
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExReleaseMutex
 *
 * PARAMETERS:  ObjDesc             - The object descriptor for this op
 *              WalkState           - Current method execution state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Release a previously acquired Mutex.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExReleaseMutex (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState)
{
    UINT8                   PreviousSyncLevel;
    ACPI_THREAD_STATE       *OwnerThread;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (ExReleaseMutex);


    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    OwnerThread = ObjDesc->Mutex.OwnerThread;

    /* The mutex must have been previously acquired in order to release it */

    if (!OwnerThread)
    {
        ACPI_ERROR ((AE_INFO,
            "Cannot release Mutex [%4.4s], not acquired",
            AcpiUtGetNodeName (ObjDesc->Mutex.Node)));
        return_ACPI_STATUS (AE_AML_MUTEX_NOT_ACQUIRED);
    }

    /* Must have a valid thread ID */

    if (!WalkState->Thread)
    {
        ACPI_ERROR ((AE_INFO,
            "Cannot release Mutex [%4.4s], null thread info",
            AcpiUtGetNodeName (ObjDesc->Mutex.Node)));
        return_ACPI_STATUS (AE_AML_INTERNAL);
    }

    /*
     * The Mutex is owned, but this thread must be the owner.
     * Special case for Global Lock, any thread can release
     */
    if ((OwnerThread->ThreadId != WalkState->Thread->ThreadId) &&
        (ObjDesc != AcpiGbl_GlobalLockMutex))
    {
        ACPI_ERROR ((AE_INFO,
            "Thread %u cannot release Mutex [%4.4s] acquired by thread %u",
            (UINT32) WalkState->Thread->ThreadId,
            AcpiUtGetNodeName (ObjDesc->Mutex.Node),
            (UINT32) OwnerThread->ThreadId));
        return_ACPI_STATUS (AE_AML_NOT_OWNER);
    }

    /*
     * The sync level of the mutex must be equal to the current sync level. In
     * other words, the current level means that at least one mutex at that
     * level is currently being held. Attempting to release a mutex of a
     * different level can only mean that the mutex ordering rule is being
     * violated. This behavior is clarified in ACPI 4.0 specification.
     */
    if (ObjDesc->Mutex.SyncLevel != OwnerThread->CurrentSyncLevel)
    {
        ACPI_ERROR ((AE_INFO,
            "Cannot release Mutex [%4.4s], SyncLevel mismatch: "
            "mutex %u current %u",
            AcpiUtGetNodeName (ObjDesc->Mutex.Node),
            ObjDesc->Mutex.SyncLevel, WalkState->Thread->CurrentSyncLevel));
        return_ACPI_STATUS (AE_AML_MUTEX_ORDER);
    }

    /*
     * Get the previous SyncLevel from the head of the acquired mutex list.
     * This handles the case where several mutexes at the same level have been
     * acquired, but are not released in reverse order.
     */
    PreviousSyncLevel =
        OwnerThread->AcquiredMutexList->Mutex.OriginalSyncLevel;

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
        "Releasing: Object SyncLevel %u, Thread SyncLevel %u, "
        "Prev SyncLevel %u, Depth %u TID %p\n",
        ObjDesc->Mutex.SyncLevel, WalkState->Thread->CurrentSyncLevel,
        PreviousSyncLevel, ObjDesc->Mutex.AcquisitionDepth,
        WalkState->Thread));

    Status = AcpiExReleaseMutexObject (ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (ObjDesc->Mutex.AcquisitionDepth == 0)
    {
        /* Restore the previous SyncLevel */

        OwnerThread->CurrentSyncLevel = PreviousSyncLevel;
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
        "Released: Object SyncLevel %u, Thread SyncLevel, %u, "
        "Prev SyncLevel %u, Depth %u\n",
        ObjDesc->Mutex.SyncLevel, WalkState->Thread->CurrentSyncLevel,
        PreviousSyncLevel, ObjDesc->Mutex.AcquisitionDepth));

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExReleaseAllMutexes
 *
 * PARAMETERS:  Thread              - Current executing thread object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Release all mutexes held by this thread
 *
 * NOTE: This function is called as the thread is exiting the interpreter.
 * Mutexes are not released when an individual control method is exited, but
 * only when the parent thread actually exits the interpreter. This allows one
 * method to acquire a mutex, and a different method to release it, as long as
 * this is performed underneath a single parent control method.
 *
 ******************************************************************************/

void
AcpiExReleaseAllMutexes (
    ACPI_THREAD_STATE       *Thread)
{
    ACPI_OPERAND_OBJECT     *Next = Thread->AcquiredMutexList;
    ACPI_OPERAND_OBJECT     *ObjDesc;


    ACPI_FUNCTION_TRACE (ExReleaseAllMutexes);


    /* Traverse the list of owned mutexes, releasing each one */

    while (Next)
    {
        ObjDesc = Next;
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "Mutex [%4.4s] force-release, SyncLevel %u Depth %u\n",
            ObjDesc->Mutex.Node->Name.Ascii, ObjDesc->Mutex.SyncLevel,
            ObjDesc->Mutex.AcquisitionDepth));

        /* Release the mutex, special case for Global Lock */

        if (ObjDesc == AcpiGbl_GlobalLockMutex)
        {
            /* Ignore errors */

            (void) AcpiEvReleaseGlobalLock ();
        }
        else
        {
            AcpiOsReleaseMutex (ObjDesc->Mutex.OsMutex);
        }

        /* Update Thread SyncLevel (Last mutex is the important one) */

        Thread->CurrentSyncLevel = ObjDesc->Mutex.OriginalSyncLevel;

        /* Mark mutex unowned */

        Next = ObjDesc->Mutex.Next;

        ObjDesc->Mutex.Prev = NULL;
        ObjDesc->Mutex.Next = NULL;
        ObjDesc->Mutex.AcquisitionDepth = 0;
        ObjDesc->Mutex.OwnerThread = NULL;
        ObjDesc->Mutex.ThreadId = 0;
    }

    return_VOID;
}
