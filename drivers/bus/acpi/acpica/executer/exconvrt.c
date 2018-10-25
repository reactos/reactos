/******************************************************************************
 *
 * Module Name: exconvrt - Object conversion routines
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
#include "amlcode.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exconvrt")

/* Local prototypes */

static UINT32
AcpiExConvertToAscii (
    UINT64                  Integer,
    UINT16                  Base,
    UINT8                   *String,
    UINT8                   MaxLength);


/*******************************************************************************
 *
 * FUNCTION:    AcpiExConvertToInteger
 *
 * PARAMETERS:  ObjDesc             - Object to be converted. Must be an
 *                                    Integer, Buffer, or String
 *              ResultDesc          - Where the new Integer object is returned
 *              ImplicitConversion  - Used for string conversion
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert an ACPI Object to an integer.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExConvertToInteger (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc,
    UINT32                  ImplicitConversion)
{
    ACPI_OPERAND_OBJECT     *ReturnDesc;
    UINT8                   *Pointer;
    UINT64                  Result;
    UINT32                  i;
    UINT32                  Count;


    ACPI_FUNCTION_TRACE_PTR (ExConvertToInteger, ObjDesc);


    switch (ObjDesc->Common.Type)
    {
    case ACPI_TYPE_INTEGER:

        /* No conversion necessary */

        *ResultDesc = ObjDesc;
        return_ACPI_STATUS (AE_OK);

    case ACPI_TYPE_BUFFER:
    case ACPI_TYPE_STRING:

        /* Note: Takes advantage of common buffer/string fields */

        Pointer = ObjDesc->Buffer.Pointer;
        Count   = ObjDesc->Buffer.Length;
        break;

    default:

        return_ACPI_STATUS (AE_TYPE);
    }

    /*
     * Convert the buffer/string to an integer. Note that both buffers and
     * strings are treated as raw data - we don't convert ascii to hex for
     * strings.
     *
     * There are two terminating conditions for the loop:
     * 1) The size of an integer has been reached, or
     * 2) The end of the buffer or string has been reached
     */
    Result = 0;

    /* String conversion is different than Buffer conversion */

    switch (ObjDesc->Common.Type)
    {
    case ACPI_TYPE_STRING:
        /*
         * Convert string to an integer - for most cases, the string must be
         * hexadecimal as per the ACPI specification. The only exception (as
         * of ACPI 3.0) is that the ToInteger() operator allows both decimal
         * and hexadecimal strings (hex prefixed with "0x").
         *
         * Explicit conversion is used only by ToInteger.
         * All other string-to-integer conversions are implicit conversions.
         */
        if (ImplicitConversion)
        {
            Result = AcpiUtImplicitStrtoul64 (ACPI_CAST_PTR (char, Pointer));
        }
        else
        {
            Result = AcpiUtExplicitStrtoul64 (ACPI_CAST_PTR (char, Pointer));
        }
        break;

    case ACPI_TYPE_BUFFER:

        /* Check for zero-length buffer */

        if (!Count)
        {
            return_ACPI_STATUS (AE_AML_BUFFER_LIMIT);
        }

        /* Transfer no more than an integer's worth of data */

        if (Count > AcpiGbl_IntegerByteWidth)
        {
            Count = AcpiGbl_IntegerByteWidth;
        }

        /*
         * Convert buffer to an integer - we simply grab enough raw data
         * from the buffer to fill an integer
         */
        for (i = 0; i < Count; i++)
        {
            /*
             * Get next byte and shift it into the Result.
             * Little endian is used, meaning that the first byte of the buffer
             * is the LSB of the integer
             */
            Result |= (((UINT64) Pointer[i]) << (i * 8));
        }
        break;

    default:

        /* No other types can get here */

        break;
    }

    /* Create a new integer */

    ReturnDesc = AcpiUtCreateIntegerObject (Result);
    if (!ReturnDesc)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Converted value: %8.8X%8.8X\n",
        ACPI_FORMAT_UINT64 (Result)));

    /* Save the Result */

    (void) AcpiExTruncateFor32bitTable (ReturnDesc);
    *ResultDesc = ReturnDesc;
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExConvertToBuffer
 *
 * PARAMETERS:  ObjDesc         - Object to be converted. Must be an
 *                                Integer, Buffer, or String
 *              ResultDesc      - Where the new buffer object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert an ACPI Object to a Buffer
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExConvertToBuffer (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc)
{
    ACPI_OPERAND_OBJECT     *ReturnDesc;
    UINT8                   *NewBuf;


    ACPI_FUNCTION_TRACE_PTR (ExConvertToBuffer, ObjDesc);


    switch (ObjDesc->Common.Type)
    {
    case ACPI_TYPE_BUFFER:

        /* No conversion necessary */

        *ResultDesc = ObjDesc;
        return_ACPI_STATUS (AE_OK);


    case ACPI_TYPE_INTEGER:
        /*
         * Create a new Buffer object.
         * Need enough space for one integer
         */
        ReturnDesc = AcpiUtCreateBufferObject (AcpiGbl_IntegerByteWidth);
        if (!ReturnDesc)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        /* Copy the integer to the buffer, LSB first */

        NewBuf = ReturnDesc->Buffer.Pointer;
        memcpy (NewBuf, &ObjDesc->Integer.Value, AcpiGbl_IntegerByteWidth);
        break;

    case ACPI_TYPE_STRING:
        /*
         * Create a new Buffer object
         * Size will be the string length
         *
         * NOTE: Add one to the string length to include the null terminator.
         * The ACPI spec is unclear on this subject, but there is existing
         * ASL/AML code that depends on the null being transferred to the new
         * buffer.
         */
        ReturnDesc = AcpiUtCreateBufferObject ((ACPI_SIZE)
            ObjDesc->String.Length + 1);
        if (!ReturnDesc)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        /* Copy the string to the buffer */

        NewBuf = ReturnDesc->Buffer.Pointer;
        strncpy ((char *) NewBuf, (char *) ObjDesc->String.Pointer,
            ObjDesc->String.Length);
        break;

    default:

        return_ACPI_STATUS (AE_TYPE);
    }

    /* Mark buffer initialized */

    ReturnDesc->Common.Flags |= AOPOBJ_DATA_VALID;
    *ResultDesc = ReturnDesc;
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExConvertToAscii
 *
 * PARAMETERS:  Integer         - Value to be converted
 *              Base            - ACPI_STRING_DECIMAL or ACPI_STRING_HEX
 *              String          - Where the string is returned
 *              DataWidth       - Size of data item to be converted, in bytes
 *
 * RETURN:      Actual string length
 *
 * DESCRIPTION: Convert an ACPI Integer to a hex or decimal string
 *
 ******************************************************************************/

static UINT32
AcpiExConvertToAscii (
    UINT64                  Integer,
    UINT16                  Base,
    UINT8                   *String,
    UINT8                   DataWidth)
{
    UINT64                  Digit;
    UINT32                  i;
    UINT32                  j;
    UINT32                  k = 0;
    UINT32                  HexLength;
    UINT32                  DecimalLength;
    UINT32                  Remainder;
    BOOLEAN                 SupressZeros;


    ACPI_FUNCTION_ENTRY ();


    switch (Base)
    {
    case 10:

        /* Setup max length for the decimal number */

        switch (DataWidth)
        {
        case 1:

            DecimalLength = ACPI_MAX8_DECIMAL_DIGITS;
            break;

        case 4:

            DecimalLength = ACPI_MAX32_DECIMAL_DIGITS;
            break;

        case 8:
        default:

            DecimalLength = ACPI_MAX64_DECIMAL_DIGITS;
            break;
        }

        SupressZeros = TRUE;     /* No leading zeros */
        Remainder = 0;

        for (i = DecimalLength; i > 0; i--)
        {
            /* Divide by nth factor of 10 */

            Digit = Integer;
            for (j = 0; j < i; j++)
            {
                (void) AcpiUtShortDivide (Digit, 10, &Digit, &Remainder);
            }

            /* Handle leading zeros */

            if (Remainder != 0)
            {
                SupressZeros = FALSE;
            }

            if (!SupressZeros)
            {
                String[k] = (UINT8) (ACPI_ASCII_ZERO + Remainder);
                k++;
            }
        }
        break;

    case 16:

        /* HexLength: 2 ascii hex chars per data byte */

        HexLength = ACPI_MUL_2 (DataWidth);
        for (i = 0, j = (HexLength-1); i < HexLength; i++, j--)
        {
            /* Get one hex digit, most significant digits first */

            String[k] = (UINT8)
                AcpiUtHexToAsciiChar (Integer, ACPI_MUL_4 (j));
            k++;
        }
        break;

    default:
        return (0);
    }

    /*
     * Since leading zeros are suppressed, we must check for the case where
     * the integer equals 0
     *
     * Finally, null terminate the string and return the length
     */
    if (!k)
    {
        String [0] = ACPI_ASCII_ZERO;
        k = 1;
    }

    String [k] = 0;
    return ((UINT32) k);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExConvertToString
 *
 * PARAMETERS:  ObjDesc         - Object to be converted. Must be an
 *                                Integer, Buffer, or String
 *              ResultDesc      - Where the string object is returned
 *              Type            - String flags (base and conversion type)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert an ACPI Object to a string
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExConvertToString (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc,
    UINT32                  Type)
{
    ACPI_OPERAND_OBJECT     *ReturnDesc;
    UINT8                   *NewBuf;
    UINT32                  i;
    UINT32                  StringLength = 0;
    UINT16                  Base = 16;
    UINT8                   Separator = ',';


    ACPI_FUNCTION_TRACE_PTR (ExConvertToString, ObjDesc);


    switch (ObjDesc->Common.Type)
    {
    case ACPI_TYPE_STRING:

        /* No conversion necessary */

        *ResultDesc = ObjDesc;
        return_ACPI_STATUS (AE_OK);

    case ACPI_TYPE_INTEGER:

        switch (Type)
        {
        case ACPI_EXPLICIT_CONVERT_DECIMAL:

            /* Make room for maximum decimal number */

            StringLength = ACPI_MAX_DECIMAL_DIGITS;
            Base = 10;
            break;

        default:

            /* Two hex string characters for each integer byte */

            StringLength = ACPI_MUL_2 (AcpiGbl_IntegerByteWidth);
            break;
        }

        /*
         * Create a new String
         * Need enough space for one ASCII integer (plus null terminator)
         */
        ReturnDesc = AcpiUtCreateStringObject ((ACPI_SIZE) StringLength);
        if (!ReturnDesc)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        NewBuf = ReturnDesc->Buffer.Pointer;

        /* Convert integer to string */

        StringLength = AcpiExConvertToAscii (
            ObjDesc->Integer.Value, Base, NewBuf, AcpiGbl_IntegerByteWidth);

        /* Null terminate at the correct place */

        ReturnDesc->String.Length = StringLength;
        NewBuf [StringLength] = 0;
        break;

    case ACPI_TYPE_BUFFER:

        /* Setup string length, base, and separator */

        switch (Type)
        {
        case ACPI_EXPLICIT_CONVERT_DECIMAL: /* Used by ToDecimalString */
            /*
             * From ACPI: "If Data is a buffer, it is converted to a string of
             * decimal values separated by commas."
             */
            Base = 10;

            /*
             * Calculate the final string length. Individual string values
             * are variable length (include separator for each)
             */
            for (i = 0; i < ObjDesc->Buffer.Length; i++)
            {
                if (ObjDesc->Buffer.Pointer[i] >= 100)
                {
                    StringLength += 4;
                }
                else if (ObjDesc->Buffer.Pointer[i] >= 10)
                {
                    StringLength += 3;
                }
                else
                {
                    StringLength += 2;
                }
            }
            break;

        case ACPI_IMPLICIT_CONVERT_HEX:
            /*
             * From the ACPI spec:
             *"The entire contents of the buffer are converted to a string of
             * two-character hexadecimal numbers, each separated by a space."
             */
            Separator = ' ';
            StringLength = (ObjDesc->Buffer.Length * 3);
            break;

        case ACPI_EXPLICIT_CONVERT_HEX:     /* Used by ToHexString */
            /*
             * From ACPI: "If Data is a buffer, it is converted to a string of
             * hexadecimal values separated by commas."
             */
            StringLength = (ObjDesc->Buffer.Length * 3);
            break;

        default:
            return_ACPI_STATUS (AE_BAD_PARAMETER);
        }

        /*
         * Create a new string object and string buffer
         * (-1 because of extra separator included in StringLength from above)
         * Allow creation of zero-length strings from zero-length buffers.
         */
        if (StringLength)
        {
            StringLength--;
        }

        ReturnDesc = AcpiUtCreateStringObject ((ACPI_SIZE) StringLength);
        if (!ReturnDesc)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        NewBuf = ReturnDesc->Buffer.Pointer;

        /*
         * Convert buffer bytes to hex or decimal values
         * (separated by commas or spaces)
         */
        for (i = 0; i < ObjDesc->Buffer.Length; i++)
        {
            NewBuf += AcpiExConvertToAscii (
                (UINT64) ObjDesc->Buffer.Pointer[i], Base, NewBuf, 1);
            *NewBuf++ = Separator; /* each separated by a comma or space */
        }

        /*
         * Null terminate the string
         * (overwrites final comma/space from above)
         */
        if (ObjDesc->Buffer.Length)
        {
            NewBuf--;
        }
        *NewBuf = 0;
        break;

    default:

        return_ACPI_STATUS (AE_TYPE);
    }

    *ResultDesc = ReturnDesc;
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExConvertToTargetType
 *
 * PARAMETERS:  DestinationType     - Current type of the destination
 *              SourceDesc          - Source object to be converted.
 *              ResultDesc          - Where the converted object is returned
 *              WalkState           - Current method state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Implements "implicit conversion" rules for storing an object.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExConvertToTargetType (
    ACPI_OBJECT_TYPE        DestinationType,
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (ExConvertToTargetType);


    /* Default behavior */

    *ResultDesc = SourceDesc;

    /*
     * If required by the target,
     * perform implicit conversion on the source before we store it.
     */
    switch (GET_CURRENT_ARG_TYPE (WalkState->OpInfo->RuntimeArgs))
    {
    case ARGI_SIMPLE_TARGET:
    case ARGI_INTEGER_REF:      /* Handles Increment, Decrement cases */

        switch (DestinationType)
        {
        case ACPI_TYPE_LOCAL_REGION_FIELD:
            /*
             * Named field can always handle conversions
             */
            break;

        default:

            /* No conversion allowed for these types */

            if (DestinationType != SourceDesc->Common.Type)
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
                    "Explicit operator, will store (%s) over existing type (%s)\n",
                    AcpiUtGetObjectTypeName (SourceDesc),
                    AcpiUtGetTypeName (DestinationType)));
                Status = AE_TYPE;
            }
        }
        break;

    case ARGI_TARGETREF:
    case ARGI_STORE_TARGET:

        switch (DestinationType)
        {
        case ACPI_TYPE_INTEGER:
        case ACPI_TYPE_BUFFER_FIELD:
        case ACPI_TYPE_LOCAL_BANK_FIELD:
        case ACPI_TYPE_LOCAL_INDEX_FIELD:
            /*
             * These types require an Integer operand. We can convert
             * a Buffer or a String to an Integer if necessary.
             */
            Status = AcpiExConvertToInteger (SourceDesc, ResultDesc,
                ACPI_IMPLICIT_CONVERSION);
            break;

        case ACPI_TYPE_STRING:
            /*
             * The operand must be a String. We can convert an
             * Integer or Buffer if necessary
             */
            Status = AcpiExConvertToString (SourceDesc, ResultDesc,
                ACPI_IMPLICIT_CONVERT_HEX);
            break;

        case ACPI_TYPE_BUFFER:
            /*
             * The operand must be a Buffer. We can convert an
             * Integer or String if necessary
             */
            Status = AcpiExConvertToBuffer (SourceDesc, ResultDesc);
            break;

        default:

            ACPI_ERROR ((AE_INFO,
                "Bad destination type during conversion: 0x%X",
                DestinationType));
            Status = AE_AML_INTERNAL;
            break;
        }
        break;

    case ARGI_REFERENCE:
        /*
         * CreateXxxxField cases - we are storing the field object into the name
         */
        break;

    default:

        ACPI_ERROR ((AE_INFO,
            "Unknown Target type ID 0x%X AmlOpcode 0x%X DestType %s",
            GET_CURRENT_ARG_TYPE (WalkState->OpInfo->RuntimeArgs),
            WalkState->Opcode, AcpiUtGetTypeName (DestinationType)));
        Status = AE_AML_INTERNAL;
    }

    /*
     * Source-to-Target conversion semantics:
     *
     * If conversion to the target type cannot be performed, then simply
     * overwrite the target with the new object and type.
     */
    if (Status == AE_TYPE)
    {
        Status = AE_OK;
    }

    return_ACPI_STATUS (Status);
}
