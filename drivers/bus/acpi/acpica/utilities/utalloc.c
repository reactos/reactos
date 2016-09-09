/******************************************************************************
 *
 * Module Name: utalloc - local memory allocation routines
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
#include "acdebug.h"

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utalloc")


#if !defined (USE_NATIVE_ALLOCATE_ZEROED)
/*******************************************************************************
 *
 * FUNCTION:    AcpiOsAllocateZeroed
 *
 * PARAMETERS:  Size                - Size of the allocation
 *
 * RETURN:      Address of the allocated memory on success, NULL on failure.
 *
 * DESCRIPTION: Subsystem equivalent of calloc. Allocate and zero memory.
 *              This is the default implementation. Can be overridden via the
 *              USE_NATIVE_ALLOCATE_ZEROED flag.
 *
 ******************************************************************************/

void *
AcpiOsAllocateZeroed (
    ACPI_SIZE               Size)
{
    void                    *Allocation;


    ACPI_FUNCTION_ENTRY ();


    Allocation = AcpiOsAllocate (Size);
    if (Allocation)
    {
        /* Clear the memory block */

        memset (Allocation, 0, Size);
    }

    return (Allocation);
}

#endif /* !USE_NATIVE_ALLOCATE_ZEROED */


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtCreateCaches
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create all local caches
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtCreateCaches (
    void)
{
    ACPI_STATUS             Status;


    /* Object Caches, for frequently used objects */

    Status = AcpiOsCreateCache ("Acpi-Namespace", sizeof (ACPI_NAMESPACE_NODE),
        ACPI_MAX_NAMESPACE_CACHE_DEPTH, &AcpiGbl_NamespaceCache);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiOsCreateCache ("Acpi-State", sizeof (ACPI_GENERIC_STATE),
        ACPI_MAX_STATE_CACHE_DEPTH, &AcpiGbl_StateCache);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiOsCreateCache ("Acpi-Parse", sizeof (ACPI_PARSE_OBJ_COMMON),
        ACPI_MAX_PARSE_CACHE_DEPTH, &AcpiGbl_PsNodeCache);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiOsCreateCache ("Acpi-ParseExt", sizeof (ACPI_PARSE_OBJ_NAMED),
        ACPI_MAX_EXTPARSE_CACHE_DEPTH, &AcpiGbl_PsNodeExtCache);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiOsCreateCache ("Acpi-Operand", sizeof (ACPI_OPERAND_OBJECT),
        ACPI_MAX_OBJECT_CACHE_DEPTH, &AcpiGbl_OperandCache);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }


#ifdef ACPI_DBG_TRACK_ALLOCATIONS

    /* Memory allocation lists */

    Status = AcpiUtCreateList ("Acpi-Global", 0,
        &AcpiGbl_GlobalList);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    Status = AcpiUtCreateList ("Acpi-Namespace", sizeof (ACPI_NAMESPACE_NODE),
        &AcpiGbl_NsNodeList);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }
#endif

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtDeleteCaches
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Purge and delete all local caches
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtDeleteCaches (
    void)
{
#ifdef ACPI_DBG_TRACK_ALLOCATIONS
    char                    Buffer[7];


    if (AcpiGbl_DisplayFinalMemStats)
    {
        strcpy (Buffer, "MEMORY");
        (void) AcpiDbDisplayStatistics (Buffer);
    }
#endif

    (void) AcpiOsDeleteCache (AcpiGbl_NamespaceCache);
    AcpiGbl_NamespaceCache = NULL;

    (void) AcpiOsDeleteCache (AcpiGbl_StateCache);
    AcpiGbl_StateCache = NULL;

    (void) AcpiOsDeleteCache (AcpiGbl_OperandCache);
    AcpiGbl_OperandCache = NULL;

    (void) AcpiOsDeleteCache (AcpiGbl_PsNodeCache);
    AcpiGbl_PsNodeCache = NULL;

    (void) AcpiOsDeleteCache (AcpiGbl_PsNodeExtCache);
    AcpiGbl_PsNodeExtCache = NULL;


#ifdef ACPI_DBG_TRACK_ALLOCATIONS

    /* Debug only - display leftover memory allocation, if any */

    AcpiUtDumpAllocations (ACPI_UINT32_MAX, NULL);

    /* Free memory lists */

    AcpiOsFree (AcpiGbl_GlobalList);
    AcpiGbl_GlobalList = NULL;

    AcpiOsFree (AcpiGbl_NsNodeList);
    AcpiGbl_NsNodeList = NULL;
#endif

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtValidateBuffer
 *
 * PARAMETERS:  Buffer              - Buffer descriptor to be validated
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform parameter validation checks on an ACPI_BUFFER
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtValidateBuffer (
    ACPI_BUFFER             *Buffer)
{

    /* Obviously, the structure pointer must be valid */

    if (!Buffer)
    {
        return (AE_BAD_PARAMETER);
    }

    /* Special semantics for the length */

    if ((Buffer->Length == ACPI_NO_BUFFER)              ||
        (Buffer->Length == ACPI_ALLOCATE_BUFFER)        ||
        (Buffer->Length == ACPI_ALLOCATE_LOCAL_BUFFER))
    {
        return (AE_OK);
    }

    /* Length is valid, the buffer pointer must be also */

    if (!Buffer->Pointer)
    {
        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtInitializeBuffer
 *
 * PARAMETERS:  Buffer              - Buffer to be validated
 *              RequiredLength      - Length needed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Validate that the buffer is of the required length or
 *              allocate a new buffer. Returned buffer is always zeroed.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtInitializeBuffer (
    ACPI_BUFFER             *Buffer,
    ACPI_SIZE               RequiredLength)
{
    ACPI_SIZE               InputBufferLength;


    /* Parameter validation */

    if (!Buffer || !RequiredLength)
    {
        return (AE_BAD_PARAMETER);
    }

    /*
     * Buffer->Length is used as both an input and output parameter. Get the
     * input actual length and set the output required buffer length.
     */
    InputBufferLength = Buffer->Length;
    Buffer->Length = RequiredLength;

    /*
     * The input buffer length contains the actual buffer length, or the type
     * of buffer to be allocated by this routine.
     */
    switch (InputBufferLength)
    {
    case ACPI_NO_BUFFER:

        /* Return the exception (and the required buffer length) */

        return (AE_BUFFER_OVERFLOW);

    case ACPI_ALLOCATE_BUFFER:
        /*
         * Allocate a new buffer. We directectly call AcpiOsAllocate here to
         * purposefully bypass the (optionally enabled) internal allocation
         * tracking mechanism since we only want to track internal
         * allocations. Note: The caller should use AcpiOsFree to free this
         * buffer created via ACPI_ALLOCATE_BUFFER.
         */
        Buffer->Pointer = AcpiOsAllocate (RequiredLength);
        break;

    case ACPI_ALLOCATE_LOCAL_BUFFER:

        /* Allocate a new buffer with local interface to allow tracking */

        Buffer->Pointer = ACPI_ALLOCATE (RequiredLength);
        break;

    default:

        /* Existing buffer: Validate the size of the buffer */

        if (InputBufferLength < RequiredLength)
        {
            return (AE_BUFFER_OVERFLOW);
        }
        break;
    }

    /* Validate allocation from above or input buffer pointer */

    if (!Buffer->Pointer)
    {
        return (AE_NO_MEMORY);
    }

    /* Have a valid buffer, clear it */

    memset (Buffer->Pointer, 0, RequiredLength);
    return (AE_OK);
}
