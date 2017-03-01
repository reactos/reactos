/******************************************************************************
 *
 * Module Name: exsystem - Interface to OS services
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2017, Intel Corp.
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

#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exsystem")


/*******************************************************************************
 *
 * FUNCTION:    AcpiExSystemWaitSemaphore
 *
 * PARAMETERS:  Semaphore       - Semaphore to wait on
 *              Timeout         - Max time to wait
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Implements a semaphore wait with a check to see if the
 *              semaphore is available immediately. If it is not, the
 *              interpreter is released before waiting.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExSystemWaitSemaphore (
    ACPI_SEMAPHORE          Semaphore,
    UINT16                  Timeout)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (ExSystemWaitSemaphore);


    Status = AcpiOsWaitSemaphore (Semaphore, 1, ACPI_DO_NOT_WAIT);
    if (ACPI_SUCCESS (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (Status == AE_TIME)
    {
        /* We must wait, so unlock the interpreter */

        AcpiExExitInterpreter ();
        Status = AcpiOsWaitSemaphore (Semaphore, 1, Timeout);

        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "*** Thread awake after blocking, %s\n",
            AcpiFormatException (Status)));

        /* Reacquire the interpreter */

        AcpiExEnterInterpreter ();
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExSystemWaitMutex
 *
 * PARAMETERS:  Mutex           - Mutex to wait on
 *              Timeout         - Max time to wait
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Implements a mutex wait with a check to see if the
 *              mutex is available immediately. If it is not, the
 *              interpreter is released before waiting.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExSystemWaitMutex (
    ACPI_MUTEX              Mutex,
    UINT16                  Timeout)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (ExSystemWaitMutex);


    Status = AcpiOsAcquireMutex (Mutex, ACPI_DO_NOT_WAIT);
    if (ACPI_SUCCESS (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (Status == AE_TIME)
    {
        /* We must wait, so unlock the interpreter */

        AcpiExExitInterpreter ();
        Status = AcpiOsAcquireMutex (Mutex, Timeout);

        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "*** Thread awake after blocking, %s\n",
            AcpiFormatException (Status)));

        /* Reacquire the interpreter */

        AcpiExEnterInterpreter ();
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExSystemDoStall
 *
 * PARAMETERS:  HowLong         - The amount of time to stall,
 *                                in microseconds
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Suspend running thread for specified amount of time.
 *              Note: ACPI specification requires that Stall() does not
 *              relinquish the processor, and delays longer than 100 usec
 *              should use Sleep() instead. We allow stalls up to 255 usec
 *              for compatibility with other interpreters and existing BIOSs.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExSystemDoStall (
    UINT32                  HowLong)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_ENTRY ();


    if (HowLong > 255) /* 255 microseconds */
    {
        /*
         * Longer than 255 usec, this is an error
         *
         * (ACPI specifies 100 usec as max, but this gives some slack in
         * order to support existing BIOSs)
         */
        ACPI_ERROR ((AE_INFO,
            "Time parameter is too large (%u)", HowLong));
        Status = AE_AML_OPERAND_VALUE;
    }
    else
    {
        AcpiOsStall (HowLong);
    }

    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExSystemDoSleep
 *
 * PARAMETERS:  HowLong         - The amount of time to sleep,
 *                                in milliseconds
 *
 * RETURN:      None
 *
 * DESCRIPTION: Sleep the running thread for specified amount of time.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExSystemDoSleep (
    UINT64                  HowLong)
{
    ACPI_FUNCTION_ENTRY ();


    /* Since this thread will sleep, we must release the interpreter */

    AcpiExExitInterpreter ();

    /*
     * For compatibility with other ACPI implementations and to prevent
     * accidental deep sleeps, limit the sleep time to something reasonable.
     */
    if (HowLong > ACPI_MAX_SLEEP)
    {
        HowLong = ACPI_MAX_SLEEP;
    }

    AcpiOsSleep (HowLong);

    /* And now we must get the interpreter again */

    AcpiExEnterInterpreter ();
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExSystemSignalEvent
 *
 * PARAMETERS:  ObjDesc         - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Provides an access point to perform synchronization operations
 *              within the AML.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExSystemSignalEvent (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (ExSystemSignalEvent);


    if (ObjDesc)
    {
        Status = AcpiOsSignalSemaphore (ObjDesc->Event.OsSemaphore, 1);
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExSystemWaitEvent
 *
 * PARAMETERS:  TimeDesc        - The 'time to delay' object descriptor
 *              ObjDesc         - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Provides an access point to perform synchronization operations
 *              within the AML. This operation is a request to wait for an
 *              event.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExSystemWaitEvent (
    ACPI_OPERAND_OBJECT     *TimeDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (ExSystemWaitEvent);


    if (ObjDesc)
    {
        Status = AcpiExSystemWaitSemaphore (ObjDesc->Event.OsSemaphore,
            (UINT16) TimeDesc->Integer.Value);
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExSystemResetEvent
 *
 * PARAMETERS:  ObjDesc         - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Reset an event to a known state.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExSystemResetEvent (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_SEMAPHORE          TempSemaphore;


    ACPI_FUNCTION_ENTRY ();


    /*
     * We are going to simply delete the existing semaphore and
     * create a new one!
     */
    Status = AcpiOsCreateSemaphore (ACPI_NO_UNIT_LIMIT, 0, &TempSemaphore);
    if (ACPI_SUCCESS (Status))
    {
        (void) AcpiOsDeleteSemaphore (ObjDesc->Event.OsSemaphore);
        ObjDesc->Event.OsSemaphore = TempSemaphore;
    }

    return (Status);
}
