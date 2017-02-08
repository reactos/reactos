/******************************************************************************
 *
 * Module Name: utlock - Reader/Writer lock interfaces
 *
 *****************************************************************************/

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
        ACPI_MODULE_NAME    ("utlock")


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtCreateRwLock
 *              AcpiUtDeleteRwLock
 *
 * PARAMETERS:  Lock                - Pointer to a valid RW lock
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Reader/writer lock creation and deletion interfaces.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtCreateRwLock (
    ACPI_RW_LOCK            *Lock)
{
    ACPI_STATUS             Status;


    Lock->NumReaders = 0;
    Status = AcpiOsCreateMutex (&Lock->ReaderMutex);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiOsCreateMutex (&Lock->WriterMutex);
    return (Status);
}


void
AcpiUtDeleteRwLock (
    ACPI_RW_LOCK            *Lock)
{

    AcpiOsDeleteMutex (Lock->ReaderMutex);
    AcpiOsDeleteMutex (Lock->WriterMutex);

    Lock->NumReaders = 0;
    Lock->ReaderMutex = NULL;
    Lock->WriterMutex = NULL;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtAcquireReadLock
 *              AcpiUtReleaseReadLock
 *
 * PARAMETERS:  Lock                - Pointer to a valid RW lock
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Reader interfaces for reader/writer locks. On acquisition,
 *              only the first reader acquires the write mutex. On release,
 *              only the last reader releases the write mutex. Although this
 *              algorithm can in theory starve writers, this should not be a
 *              problem with ACPICA since the subsystem is infrequently used
 *              in comparison to (for example) an I/O system.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtAcquireReadLock (
    ACPI_RW_LOCK            *Lock)
{
    ACPI_STATUS             Status;


    Status = AcpiOsAcquireMutex (Lock->ReaderMutex, ACPI_WAIT_FOREVER);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Acquire the write lock only for the first reader */

    Lock->NumReaders++;
    if (Lock->NumReaders == 1)
    {
        Status = AcpiOsAcquireMutex (Lock->WriterMutex, ACPI_WAIT_FOREVER);
    }

    AcpiOsReleaseMutex (Lock->ReaderMutex);
    return (Status);
}


ACPI_STATUS
AcpiUtReleaseReadLock (
    ACPI_RW_LOCK            *Lock)
{
    ACPI_STATUS             Status;


    Status = AcpiOsAcquireMutex (Lock->ReaderMutex, ACPI_WAIT_FOREVER);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Release the write lock only for the very last reader */

    Lock->NumReaders--;
    if (Lock->NumReaders == 0)
    {
        AcpiOsReleaseMutex (Lock->WriterMutex);
    }

    AcpiOsReleaseMutex (Lock->ReaderMutex);
    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtAcquireWriteLock
 *              AcpiUtReleaseWriteLock
 *
 * PARAMETERS:  Lock                - Pointer to a valid RW lock
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Writer interfaces for reader/writer locks. Simply acquire or
 *              release the writer mutex associated with the lock. Acquisition
 *              of the lock is fully exclusive and will block all readers and
 *              writers until it is released.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtAcquireWriteLock (
    ACPI_RW_LOCK            *Lock)
{
    ACPI_STATUS             Status;


    Status = AcpiOsAcquireMutex (Lock->WriterMutex, ACPI_WAIT_FOREVER);
    return (Status);
}


void
AcpiUtReleaseWriteLock (
    ACPI_RW_LOCK            *Lock)
{

    AcpiOsReleaseMutex (Lock->WriterMutex);
}
