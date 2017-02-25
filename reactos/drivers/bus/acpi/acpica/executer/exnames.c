/******************************************************************************
 *
 * Module Name: exnames - interpreter/scanner name load/execute
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
#include "acinterp.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exnames")

/* Local prototypes */

static char *
AcpiExAllocateNameString (
    UINT32                  PrefixCount,
    UINT32                  NumNameSegs);

static ACPI_STATUS
AcpiExNameSegment (
    UINT8                   **InAmlAddress,
    char                    *NameString);


/*******************************************************************************
 *
 * FUNCTION:    AcpiExAllocateNameString
 *
 * PARAMETERS:  PrefixCount         - Count of parent levels. Special cases:
 *                                    (-1)==root,  0==none
 *              NumNameSegs         - count of 4-character name segments
 *
 * RETURN:      A pointer to the allocated string segment. This segment must
 *              be deleted by the caller.
 *
 * DESCRIPTION: Allocate a buffer for a name string. Ensure allocated name
 *              string is long enough, and set up prefix if any.
 *
 ******************************************************************************/

static char *
AcpiExAllocateNameString (
    UINT32                  PrefixCount,
    UINT32                  NumNameSegs)
{
    char                    *TempPtr;
    char                    *NameString;
    UINT32                   SizeNeeded;

    ACPI_FUNCTION_TRACE (ExAllocateNameString);


    /*
     * Allow room for all \ and ^ prefixes, all segments and a MultiNamePrefix.
     * Also, one byte for the null terminator.
     * This may actually be somewhat longer than needed.
     */
    if (PrefixCount == ACPI_UINT32_MAX)
    {
        /* Special case for root */

        SizeNeeded = 1 + (ACPI_NAME_SIZE * NumNameSegs) + 2 + 1;
    }
    else
    {
        SizeNeeded = PrefixCount + (ACPI_NAME_SIZE * NumNameSegs) + 2 + 1;
    }

    /*
     * Allocate a buffer for the name.
     * This buffer must be deleted by the caller!
     */
    NameString = ACPI_ALLOCATE (SizeNeeded);
    if (!NameString)
    {
        ACPI_ERROR ((AE_INFO,
            "Could not allocate size %u", SizeNeeded));
        return_PTR (NULL);
    }

    TempPtr = NameString;

    /* Set up Root or Parent prefixes if needed */

    if (PrefixCount == ACPI_UINT32_MAX)
    {
        *TempPtr++ = AML_ROOT_PREFIX;
    }
    else
    {
        while (PrefixCount--)
        {
            *TempPtr++ = AML_PARENT_PREFIX;
        }
    }


    /* Set up Dual or Multi prefixes if needed */

    if (NumNameSegs > 2)
    {
        /* Set up multi prefixes   */

        *TempPtr++ = AML_MULTI_NAME_PREFIX_OP;
        *TempPtr++ = (char) NumNameSegs;
    }
    else if (2 == NumNameSegs)
    {
        /* Set up dual prefixes */

        *TempPtr++ = AML_DUAL_NAME_PREFIX;
    }

    /*
     * Terminate string following prefixes. AcpiExNameSegment() will
     * append the segment(s)
     */
    *TempPtr = 0;

    return_PTR (NameString);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExNameSegment
 *
 * PARAMETERS:  InAmlAddress    - Pointer to the name in the AML code
 *              NameString      - Where to return the name. The name is appended
 *                                to any existing string to form a namepath
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Extract an ACPI name (4 bytes) from the AML byte stream
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiExNameSegment (
    UINT8                   **InAmlAddress,
    char                    *NameString)
{
    char                    *AmlAddress = (void *) *InAmlAddress;
    ACPI_STATUS             Status = AE_OK;
    UINT32                  Index;
    char                    CharBuf[5];


    ACPI_FUNCTION_TRACE (ExNameSegment);


    /*
     * If first character is a digit, then we know that we aren't looking
     * at a valid name segment
     */
    CharBuf[0] = *AmlAddress;

    if ('0' <= CharBuf[0] && CharBuf[0] <= '9')
    {
        ACPI_ERROR ((AE_INFO, "Invalid leading digit: %c", CharBuf[0]));
        return_ACPI_STATUS (AE_CTRL_PENDING);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_LOAD, "Bytes from stream:\n"));

    for (Index = 0;
        (Index < ACPI_NAME_SIZE) && (AcpiUtValidNameChar (*AmlAddress, 0));
        Index++)
    {
        CharBuf[Index] = *AmlAddress++;
        ACPI_DEBUG_PRINT ((ACPI_DB_LOAD, "%c\n", CharBuf[Index]));
    }


    /* Valid name segment  */

    if (Index == 4)
    {
        /* Found 4 valid characters */

        CharBuf[4] = '\0';

        if (NameString)
        {
            strcat (NameString, CharBuf);
            ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
                "Appended to - %s\n", NameString));
        }
        else
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_NAMES,
                "No Name string - %s\n", CharBuf));
        }
    }
    else if (Index == 0)
    {
        /*
         * First character was not a valid name character,
         * so we are looking at something other than a name.
         */
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "Leading character is not alpha: %02Xh (not a name)\n",
            CharBuf[0]));
        Status = AE_CTRL_PENDING;
    }
    else
    {
        /*
         * Segment started with one or more valid characters, but fewer than
         * the required 4
         */
        Status = AE_AML_BAD_NAME;
        ACPI_ERROR ((AE_INFO,
            "Bad character 0x%02x in name, at %p",
            *AmlAddress, AmlAddress));
    }

    *InAmlAddress = ACPI_CAST_PTR (UINT8, AmlAddress);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExGetNameString
 *
 * PARAMETERS:  DataType            - Object type to be associated with this
 *                                    name
 *              InAmlAddress        - Pointer to the namestring in the AML code
 *              OutNameString       - Where the namestring is returned
 *              OutNameLength       - Length of the returned string
 *
 * RETURN:      Status, namestring and length
 *
 * DESCRIPTION: Extract a full namepath from the AML byte stream,
 *              including any prefixes.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExGetNameString (
    ACPI_OBJECT_TYPE        DataType,
    UINT8                   *InAmlAddress,
    char                    **OutNameString,
    UINT32                  *OutNameLength)
{
    ACPI_STATUS             Status = AE_OK;
    UINT8                   *AmlAddress = InAmlAddress;
    char                    *NameString = NULL;
    UINT32                  NumSegments;
    UINT32                  PrefixCount = 0;
    BOOLEAN                 HasPrefix = FALSE;


    ACPI_FUNCTION_TRACE_PTR (ExGetNameString, AmlAddress);


    if (ACPI_TYPE_LOCAL_REGION_FIELD == DataType   ||
        ACPI_TYPE_LOCAL_BANK_FIELD == DataType     ||
        ACPI_TYPE_LOCAL_INDEX_FIELD == DataType)
    {
        /* Disallow prefixes for types associated with FieldUnit names */

        NameString = AcpiExAllocateNameString (0, 1);
        if (!NameString)
        {
            Status = AE_NO_MEMORY;
        }
        else
        {
            Status = AcpiExNameSegment (&AmlAddress, NameString);
        }
    }
    else
    {
        /*
         * DataType is not a field name.
         * Examine first character of name for root or parent prefix operators
         */
        switch (*AmlAddress)
        {
        case AML_ROOT_PREFIX:

            ACPI_DEBUG_PRINT ((ACPI_DB_LOAD, "RootPrefix(\\) at %p\n",
                AmlAddress));

            /*
             * Remember that we have a RootPrefix --
             * see comment in AcpiExAllocateNameString()
             */
            AmlAddress++;
            PrefixCount = ACPI_UINT32_MAX;
            HasPrefix = TRUE;
            break;

        case AML_PARENT_PREFIX:

            /* Increment past possibly multiple parent prefixes */

            do
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_LOAD, "ParentPrefix (^) at %p\n",
                    AmlAddress));

                AmlAddress++;
                PrefixCount++;

            } while (*AmlAddress == AML_PARENT_PREFIX);

            HasPrefix = TRUE;
            break;

        default:

            /* Not a prefix character */

            break;
        }

        /* Examine first character of name for name segment prefix operator */

        switch (*AmlAddress)
        {
        case AML_DUAL_NAME_PREFIX:

            ACPI_DEBUG_PRINT ((ACPI_DB_LOAD, "DualNamePrefix at %p\n",
                AmlAddress));

            AmlAddress++;
            NameString = AcpiExAllocateNameString (PrefixCount, 2);
            if (!NameString)
            {
                Status = AE_NO_MEMORY;
                break;
            }

            /* Indicate that we processed a prefix */

            HasPrefix = TRUE;

            Status = AcpiExNameSegment (&AmlAddress, NameString);
            if (ACPI_SUCCESS (Status))
            {
                Status = AcpiExNameSegment (&AmlAddress, NameString);
            }
            break;

        case AML_MULTI_NAME_PREFIX_OP:

            ACPI_DEBUG_PRINT ((ACPI_DB_LOAD, "MultiNamePrefix at %p\n",
                AmlAddress));

            /* Fetch count of segments remaining in name path */

            AmlAddress++;
            NumSegments = *AmlAddress;

            NameString = AcpiExAllocateNameString (
                PrefixCount, NumSegments);
            if (!NameString)
            {
                Status = AE_NO_MEMORY;
                break;
            }

            /* Indicate that we processed a prefix */

            AmlAddress++;
            HasPrefix = TRUE;

            while (NumSegments &&
                    (Status = AcpiExNameSegment (&AmlAddress, NameString)) ==
                        AE_OK)
            {
                NumSegments--;
            }

            break;

        case 0:

            /* NullName valid as of 8-12-98 ASL/AML Grammar Update */

            if (PrefixCount == ACPI_UINT32_MAX)
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
                    "NameSeg is \"\\\" followed by NULL\n"));
            }

            /* Consume the NULL byte */

            AmlAddress++;
            NameString = AcpiExAllocateNameString (PrefixCount, 0);
            if (!NameString)
            {
                Status = AE_NO_MEMORY;
                break;
            }

            break;

        default:

            /* Name segment string */

            NameString = AcpiExAllocateNameString (PrefixCount, 1);
            if (!NameString)
            {
                Status = AE_NO_MEMORY;
                break;
            }

            Status = AcpiExNameSegment (&AmlAddress, NameString);
            break;
        }
    }

    if (AE_CTRL_PENDING == Status && HasPrefix)
    {
        /* Ran out of segments after processing a prefix */

        ACPI_ERROR ((AE_INFO,
            "Malformed Name at %p", NameString));
        Status = AE_AML_BAD_NAME;
    }

    if (ACPI_FAILURE (Status))
    {
        if (NameString)
        {
            ACPI_FREE (NameString);
        }
        return_ACPI_STATUS (Status);
    }

    *OutNameString = NameString;
    *OutNameLength = (UINT32) (AmlAddress - InAmlAddress);

    return_ACPI_STATUS (Status);
}
