/******************************************************************************
 *
 * Module Name: exstorob - AML object store support, store to object
 *
 *****************************************************************************/

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
#include "acinterp.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exstorob")


/*******************************************************************************
 *
 * FUNCTION:    AcpiExStoreBufferToBuffer
 *
 * PARAMETERS:  SourceDesc          - Source object to copy
 *              TargetDesc          - Destination object of the copy
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Copy a buffer object to another buffer object.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExStoreBufferToBuffer (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *TargetDesc)
{
    UINT32                  Length;
    UINT8                   *Buffer;


    ACPI_FUNCTION_TRACE_PTR (ExStoreBufferToBuffer, SourceDesc);


    /* If Source and Target are the same, just return */

    if (SourceDesc == TargetDesc)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* We know that SourceDesc is a buffer by now */

    Buffer = ACPI_CAST_PTR (UINT8, SourceDesc->Buffer.Pointer);
    Length = SourceDesc->Buffer.Length;

    /*
     * If target is a buffer of length zero or is a static buffer,
     * allocate a new buffer of the proper length
     */
    if ((TargetDesc->Buffer.Length == 0) ||
        (TargetDesc->Common.Flags & AOPOBJ_STATIC_POINTER))
    {
        TargetDesc->Buffer.Pointer = ACPI_ALLOCATE (Length);
        if (!TargetDesc->Buffer.Pointer)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        TargetDesc->Buffer.Length = Length;
    }

    /* Copy source buffer to target buffer */

    if (Length <= TargetDesc->Buffer.Length)
    {
        /* Clear existing buffer and copy in the new one */

        memset (TargetDesc->Buffer.Pointer, 0, TargetDesc->Buffer.Length);
        memcpy (TargetDesc->Buffer.Pointer, Buffer, Length);

#ifdef ACPI_OBSOLETE_BEHAVIOR
        /*
         * NOTE: ACPI versions up to 3.0 specified that the buffer must be
         * truncated if the string is smaller than the buffer. However, "other"
         * implementations of ACPI never did this and thus became the defacto
         * standard. ACPI 3.0A changes this behavior such that the buffer
         * is no longer truncated.
         */

        /*
         * OBSOLETE BEHAVIOR:
         * If the original source was a string, we must truncate the buffer,
         * according to the ACPI spec. Integer-to-Buffer and Buffer-to-Buffer
         * copy must not truncate the original buffer.
         */
        if (OriginalSrcType == ACPI_TYPE_STRING)
        {
            /* Set the new length of the target */

            TargetDesc->Buffer.Length = Length;
        }
#endif
    }
    else
    {
        /* Truncate the source, copy only what will fit */

        memcpy (TargetDesc->Buffer.Pointer, Buffer,
            TargetDesc->Buffer.Length);

        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "Truncating source buffer from %X to %X\n",
            Length, TargetDesc->Buffer.Length));
    }

    /* Copy flags */

    TargetDesc->Buffer.Flags = SourceDesc->Buffer.Flags;
    TargetDesc->Common.Flags &= ~AOPOBJ_STATIC_POINTER;
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExStoreStringToString
 *
 * PARAMETERS:  SourceDesc          - Source object to copy
 *              TargetDesc          - Destination object of the copy
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Copy a String object to another String object
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExStoreStringToString (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *TargetDesc)
{
    UINT32                  Length;
    UINT8                   *Buffer;


    ACPI_FUNCTION_TRACE_PTR (ExStoreStringToString, SourceDesc);


    /* If Source and Target are the same, just return */

    if (SourceDesc == TargetDesc)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* We know that SourceDesc is a string by now */

    Buffer = ACPI_CAST_PTR (UINT8, SourceDesc->String.Pointer);
    Length = SourceDesc->String.Length;

    /*
     * Replace existing string value if it will fit and the string
     * pointer is not a static pointer (part of an ACPI table)
     */
    if ((Length < TargetDesc->String.Length) &&
       (!(TargetDesc->Common.Flags & AOPOBJ_STATIC_POINTER)))
    {
        /*
         * String will fit in existing non-static buffer.
         * Clear old string and copy in the new one
         */
        memset (TargetDesc->String.Pointer, 0,
            (ACPI_SIZE) TargetDesc->String.Length + 1);
        memcpy (TargetDesc->String.Pointer, Buffer, Length);
    }
    else
    {
        /*
         * Free the current buffer, then allocate a new buffer
         * large enough to hold the value
         */
        if (TargetDesc->String.Pointer &&
           (!(TargetDesc->Common.Flags & AOPOBJ_STATIC_POINTER)))
        {
            /* Only free if not a pointer into the DSDT */

            ACPI_FREE (TargetDesc->String.Pointer);
        }

        TargetDesc->String.Pointer =
            ACPI_ALLOCATE_ZEROED ((ACPI_SIZE) Length + 1);

        if (!TargetDesc->String.Pointer)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        TargetDesc->Common.Flags &= ~AOPOBJ_STATIC_POINTER;
        memcpy (TargetDesc->String.Pointer, Buffer, Length);
    }

    /* Set the new target length */

    TargetDesc->String.Length = Length;
    return_ACPI_STATUS (AE_OK);
}
