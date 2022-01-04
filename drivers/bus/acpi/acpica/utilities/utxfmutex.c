/*******************************************************************************
 *
 * Module Name: utxfmutex - external AML mutex access functions
 *
 ******************************************************************************/

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
#include "acnamesp.h"


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utxfmutex")


/* Local prototypes */

static ACPI_STATUS
AcpiUtGetMutexObject (
    ACPI_HANDLE             Handle,
    ACPI_STRING             Pathname,
    ACPI_OPERAND_OBJECT     **RetObj);


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetMutexObject
 *
 * PARAMETERS:  Handle              - Mutex or prefix handle (optional)
 *              Pathname            - Mutex pathname (optional)
 *              RetObj              - Where the mutex object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get an AML mutex object. The mutex node is pointed to by
 *              Handle:Pathname. Either Handle or Pathname can be NULL, but
 *              not both.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiUtGetMutexObject (
    ACPI_HANDLE             Handle,
    ACPI_STRING             Pathname,
    ACPI_OPERAND_OBJECT     **RetObj)
{
    ACPI_NAMESPACE_NODE     *MutexNode;
    ACPI_OPERAND_OBJECT     *MutexObj;
    ACPI_STATUS             Status;


    /* Parameter validation */

    if (!RetObj || (!Handle && !Pathname))
    {
        return (AE_BAD_PARAMETER);
    }

    /* Get a the namespace node for the mutex */

    MutexNode = Handle;
    if (Pathname != NULL)
    {
        Status = AcpiGetHandle (
            Handle, Pathname, ACPI_CAST_PTR (ACPI_HANDLE, &MutexNode));
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
    }

    /* Ensure that we actually have a Mutex object */

    if (!MutexNode ||
        (MutexNode->Type != ACPI_TYPE_MUTEX))
    {
        return (AE_TYPE);
    }

    /* Get the low-level mutex object */

    MutexObj = AcpiNsGetAttachedObject (MutexNode);
    if (!MutexObj)
    {
        return (AE_NULL_OBJECT);
    }

    *RetObj = MutexObj;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiAcquireMutex
 *
 * PARAMETERS:  Handle              - Mutex or prefix handle (optional)
 *              Pathname            - Mutex pathname (optional)
 *              Timeout             - Max time to wait for the lock (millisec)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Acquire an AML mutex. This is a device driver interface to
 *              AML mutex objects, and allows for transaction locking between
 *              drivers and AML code. The mutex node is pointed to by
 *              Handle:Pathname. Either Handle or Pathname can be NULL, but
 *              not both.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiAcquireMutex (
    ACPI_HANDLE             Handle,
    ACPI_STRING             Pathname,
    UINT16                  Timeout)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *MutexObj;


    /* Get the low-level mutex associated with Handle:Pathname */

    Status = AcpiUtGetMutexObject (Handle, Pathname, &MutexObj);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Acquire the OS mutex */

    Status = AcpiOsAcquireMutex (MutexObj->Mutex.OsMutex, Timeout);
    return (Status);
}

ACPI_EXPORT_SYMBOL (AcpiAcquireMutex)


/*******************************************************************************
 *
 * FUNCTION:    AcpiReleaseMutex
 *
 * PARAMETERS:  Handle              - Mutex or prefix handle (optional)
 *              Pathname            - Mutex pathname (optional)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Release an AML mutex. This is a device driver interface to
 *              AML mutex objects, and allows for transaction locking between
 *              drivers and AML code. The mutex node is pointed to by
 *              Handle:Pathname. Either Handle or Pathname can be NULL, but
 *              not both.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiReleaseMutex (
    ACPI_HANDLE             Handle,
    ACPI_STRING             Pathname)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *MutexObj;


    /* Get the low-level mutex associated with Handle:Pathname */

    Status = AcpiUtGetMutexObject (Handle, Pathname, &MutexObj);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Release the OS mutex */

    AcpiOsReleaseMutex (MutexObj->Mutex.OsMutex);
    return (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiReleaseMutex)
