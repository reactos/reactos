/*******************************************************************************
 *
 * Module Name: utmutex - local mutex support
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
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

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utmutex")

/* Local prototypes */

static ACPI_STATUS
AcpiUtCreateMutex (
    ACPI_MUTEX_HANDLE       MutexId);

static void
AcpiUtDeleteMutex (
    ACPI_MUTEX_HANDLE       MutexId);


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtMutexInitialize
 *
 * PARAMETERS:  None.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create the system mutex objects. This includes mutexes,
 *              spin locks, and reader/writer locks.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtMutexInitialize (
    void)
{
    UINT32                  i;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (UtMutexInitialize);


    /* Create each of the predefined mutex objects */

    for (i = 0; i < ACPI_NUM_MUTEX; i++)
    {
        Status = AcpiUtCreateMutex (i);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    /* Create the spinlocks for use at interrupt level or for speed */

    Status = AcpiOsCreateLock (&AcpiGbl_GpeLock);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiOsCreateLock (&AcpiGbl_HardwareLock);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiOsCreateLock (&AcpiGbl_ReferenceCountLock);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Mutex for _OSI support */

    Status = AcpiOsCreateMutex (&AcpiGbl_OsiMutex);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Create the reader/writer lock for namespace access */

    Status = AcpiUtCreateRwLock (&AcpiGbl_NamespaceRwLock);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

#ifdef ACPI_DEBUGGER

    /* Debugger Support */

    Status = AcpiOsCreateMutex (&AcpiGbl_DbCommandReady);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiOsCreateMutex (&AcpiGbl_DbCommandComplete);
#endif

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtMutexTerminate
 *
 * PARAMETERS:  None.
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete all of the system mutex objects. This includes mutexes,
 *              spin locks, and reader/writer locks.
 *
 ******************************************************************************/

void
AcpiUtMutexTerminate (
    void)
{
    UINT32                  i;


    ACPI_FUNCTION_TRACE (UtMutexTerminate);


    /* Delete each predefined mutex object */

    for (i = 0; i < ACPI_NUM_MUTEX; i++)
    {
        AcpiUtDeleteMutex (i);
    }

    AcpiOsDeleteMutex (AcpiGbl_OsiMutex);

    /* Delete the spinlocks */

    AcpiOsDeleteLock (AcpiGbl_GpeLock);
    AcpiOsDeleteLock (AcpiGbl_HardwareLock);
    AcpiOsDeleteLock (AcpiGbl_ReferenceCountLock);

    /* Delete the reader/writer lock */

    AcpiUtDeleteRwLock (&AcpiGbl_NamespaceRwLock);

#ifdef ACPI_DEBUGGER
    AcpiOsDeleteMutex (AcpiGbl_DbCommandReady);
    AcpiOsDeleteMutex (AcpiGbl_DbCommandComplete);
#endif

    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtCreateMutex
 *
 * PARAMETERS:  MutexID         - ID of the mutex to be created
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a mutex object.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiUtCreateMutex (
    ACPI_MUTEX_HANDLE       MutexId)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE_U32 (UtCreateMutex, MutexId);


    if (!AcpiGbl_MutexInfo[MutexId].Mutex)
    {
        Status = AcpiOsCreateMutex (&AcpiGbl_MutexInfo[MutexId].Mutex);
        AcpiGbl_MutexInfo[MutexId].ThreadId = ACPI_MUTEX_NOT_ACQUIRED;
        AcpiGbl_MutexInfo[MutexId].UseCount = 0;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtDeleteMutex
 *
 * PARAMETERS:  MutexID         - ID of the mutex to be deleted
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete a mutex object.
 *
 ******************************************************************************/

static void
AcpiUtDeleteMutex (
    ACPI_MUTEX_HANDLE       MutexId)
{

    ACPI_FUNCTION_TRACE_U32 (UtDeleteMutex, MutexId);


    AcpiOsDeleteMutex (AcpiGbl_MutexInfo[MutexId].Mutex);

    AcpiGbl_MutexInfo[MutexId].Mutex = NULL;
    AcpiGbl_MutexInfo[MutexId].ThreadId = ACPI_MUTEX_NOT_ACQUIRED;

    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtAcquireMutex
 *
 * PARAMETERS:  MutexID         - ID of the mutex to be acquired
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Acquire a mutex object.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtAcquireMutex (
    ACPI_MUTEX_HANDLE       MutexId)
{
    ACPI_STATUS             Status;
    ACPI_THREAD_ID          ThisThreadId;


    ACPI_FUNCTION_NAME (UtAcquireMutex);


    if (MutexId > ACPI_MAX_MUTEX)
    {
        return (AE_BAD_PARAMETER);
    }

    ThisThreadId = AcpiOsGetThreadId ();

#ifdef ACPI_MUTEX_DEBUG
    {
        UINT32                  i;
        /*
         * Mutex debug code, for internal debugging only.
         *
         * Deadlock prevention. Check if this thread owns any mutexes of value
         * greater than or equal to this one. If so, the thread has violated
         * the mutex ordering rule. This indicates a coding error somewhere in
         * the ACPI subsystem code.
         */
        for (i = MutexId; i < ACPI_NUM_MUTEX; i++)
        {
            if (AcpiGbl_MutexInfo[i].ThreadId == ThisThreadId)
            {
                if (i == MutexId)
                {
                    ACPI_ERROR ((AE_INFO,
                        "Mutex [%s] already acquired by this thread [%u]",
                        AcpiUtGetMutexName (MutexId),
                        (UINT32) ThisThreadId));

                    return (AE_ALREADY_ACQUIRED);
                }

                ACPI_ERROR ((AE_INFO,
                    "Invalid acquire order: Thread %u owns [%s], wants [%s]",
                    (UINT32) ThisThreadId, AcpiUtGetMutexName (i),
                    AcpiUtGetMutexName (MutexId)));

                return (AE_ACQUIRE_DEADLOCK);
            }
        }
    }
#endif

    ACPI_DEBUG_PRINT ((ACPI_DB_MUTEX,
        "Thread %u attempting to acquire Mutex [%s]\n",
        (UINT32) ThisThreadId, AcpiUtGetMutexName (MutexId)));

    Status = AcpiOsAcquireMutex (
        AcpiGbl_MutexInfo[MutexId].Mutex, ACPI_WAIT_FOREVER);
    if (ACPI_SUCCESS (Status))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_MUTEX,
            "Thread %u acquired Mutex [%s]\n",
            (UINT32) ThisThreadId, AcpiUtGetMutexName (MutexId)));

        AcpiGbl_MutexInfo[MutexId].UseCount++;
        AcpiGbl_MutexInfo[MutexId].ThreadId = ThisThreadId;
    }
    else
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Thread %u could not acquire Mutex [0x%X]",
            (UINT32) ThisThreadId, MutexId));
    }

    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtReleaseMutex
 *
 * PARAMETERS:  MutexID         - ID of the mutex to be released
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Release a mutex object.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtReleaseMutex (
    ACPI_MUTEX_HANDLE       MutexId)
{
    ACPI_FUNCTION_NAME (UtReleaseMutex);


    ACPI_DEBUG_PRINT ((ACPI_DB_MUTEX, "Thread %u releasing Mutex [%s]\n",
        (UINT32) AcpiOsGetThreadId (), AcpiUtGetMutexName (MutexId)));

    if (MutexId > ACPI_MAX_MUTEX)
    {
        return (AE_BAD_PARAMETER);
    }

    /*
     * Mutex must be acquired in order to release it!
     */
    if (AcpiGbl_MutexInfo[MutexId].ThreadId == ACPI_MUTEX_NOT_ACQUIRED)
    {
        ACPI_ERROR ((AE_INFO,
            "Mutex [0x%X] is not acquired, cannot release", MutexId));

        return (AE_NOT_ACQUIRED);
    }

#ifdef ACPI_MUTEX_DEBUG
    {
        UINT32                  i;
        /*
         * Mutex debug code, for internal debugging only.
         *
         * Deadlock prevention. Check if this thread owns any mutexes of value
         * greater than this one. If so, the thread has violated the mutex
         * ordering rule. This indicates a coding error somewhere in
         * the ACPI subsystem code.
         */
        for (i = MutexId; i < ACPI_NUM_MUTEX; i++)
        {
            if (AcpiGbl_MutexInfo[i].ThreadId == AcpiOsGetThreadId ())
            {
                if (i == MutexId)
                {
                    continue;
                }

                ACPI_ERROR ((AE_INFO,
                    "Invalid release order: owns [%s], releasing [%s]",
                    AcpiUtGetMutexName (i), AcpiUtGetMutexName (MutexId)));

                return (AE_RELEASE_DEADLOCK);
            }
        }
    }
#endif

    /* Mark unlocked FIRST */

    AcpiGbl_MutexInfo[MutexId].ThreadId = ACPI_MUTEX_NOT_ACQUIRED;

    AcpiOsReleaseMutex (AcpiGbl_MutexInfo[MutexId].Mutex);
    return (AE_OK);
}
