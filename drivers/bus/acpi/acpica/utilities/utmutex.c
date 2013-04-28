/*******************************************************************************
 *
 * Module Name: utmutex - local mutex support
 *
 ******************************************************************************/

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


#define __UTMUTEX_C__

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

    /* Create the spinlocks for use at interrupt level */

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

    /* Mutex for _OSI support */
    Status = AcpiOsCreateMutex (&AcpiGbl_OsiMutex);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Create the reader/writer lock for namespace access */

    Status = AcpiUtCreateRwLock (&AcpiGbl_NamespaceRwLock);
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

    /* Delete the reader/writer lock */

    AcpiUtDeleteRwLock (&AcpiGbl_NamespaceRwLock);
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
         * Deadlock prevention.  Check if this thread owns any mutexes of value
         * greater than or equal to this one.  If so, the thread has violated
         * the mutex ordering rule.  This indicates a coding error somewhere in
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

    Status = AcpiOsAcquireMutex (AcpiGbl_MutexInfo[MutexId].Mutex,
                ACPI_WAIT_FOREVER);
    if (ACPI_SUCCESS (Status))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_MUTEX, "Thread %u acquired Mutex [%s]\n",
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
    ACPI_THREAD_ID          ThisThreadId;


    ACPI_FUNCTION_NAME (UtReleaseMutex);


    ThisThreadId = AcpiOsGetThreadId ();
    ACPI_DEBUG_PRINT ((ACPI_DB_MUTEX, "Thread %u releasing Mutex [%s]\n",
        (UINT32) ThisThreadId, AcpiUtGetMutexName (MutexId)));

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
         * Deadlock prevention.  Check if this thread owns any mutexes of value
         * greater than this one.  If so, the thread has violated the mutex
         * ordering rule.  This indicates a coding error somewhere in
         * the ACPI subsystem code.
         */
        for (i = MutexId; i < ACPI_NUM_MUTEX; i++)
        {
            if (AcpiGbl_MutexInfo[i].ThreadId == ThisThreadId)
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


