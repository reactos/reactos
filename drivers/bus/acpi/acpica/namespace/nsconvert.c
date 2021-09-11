/******************************************************************************
 *
 * Module Name: nsconvert - Object conversions for objects returned by
 *                          predefined methods
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
#include "acnamesp.h"
#include "acinterp.h"
#include "acpredef.h"
#include "amlresrc.h"

#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nsconvert")


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsConvertToInteger
 *
 * PARAMETERS:  OriginalObject      - Object to be converted
 *              ReturnObject        - Where the new converted object is returned
 *
 * RETURN:      Status. AE_OK if conversion was successful.
 *
 * DESCRIPTION: Attempt to convert a String/Buffer object to an Integer.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsConvertToInteger (
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ReturnObject)
{
    ACPI_OPERAND_OBJECT     *NewObject;
    ACPI_STATUS             Status;
    UINT64                  Value = 0;
    UINT32                  i;


    switch (OriginalObject->Common.Type)
    {
    case ACPI_TYPE_STRING:

        /* String-to-Integer conversion */

        Status = AcpiUtStrtoul64 (OriginalObject->String.Pointer, &Value);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
        break;

    case ACPI_TYPE_BUFFER:

        /* Buffer-to-Integer conversion. Max buffer size is 64 bits. */

        if (OriginalObject->Buffer.Length > 8)
        {
            return (AE_AML_OPERAND_TYPE);
        }

        /* Extract each buffer byte to create the integer */

        for (i = 0; i < OriginalObject->Buffer.Length; i++)
        {
            Value |= ((UINT64)
                OriginalObject->Buffer.Pointer[i] << (i * 8));
        }
        break;

    default:

        return (AE_AML_OPERAND_TYPE);
    }

    NewObject = AcpiUtCreateIntegerObject (Value);
    if (!NewObject)
    {
        return (AE_NO_MEMORY);
    }

    *ReturnObject = NewObject;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsConvertToString
 *
 * PARAMETERS:  OriginalObject      - Object to be converted
 *              ReturnObject        - Where the new converted object is returned
 *
 * RETURN:      Status. AE_OK if conversion was successful.
 *
 * DESCRIPTION: Attempt to convert a Integer/Buffer object to a String.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsConvertToString (
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ReturnObject)
{
    ACPI_OPERAND_OBJECT     *NewObject;
    ACPI_SIZE               Length;
    ACPI_STATUS             Status;


    switch (OriginalObject->Common.Type)
    {
    case ACPI_TYPE_INTEGER:
        /*
         * Integer-to-String conversion. Commonly, convert
         * an integer of value 0 to a NULL string. The last element of
         * _BIF and _BIX packages occasionally need this fix.
         */
        if (OriginalObject->Integer.Value == 0)
        {
            /* Allocate a new NULL string object */

            NewObject = AcpiUtCreateStringObject (0);
            if (!NewObject)
            {
                return (AE_NO_MEMORY);
            }
        }
        else
        {
            Status = AcpiExConvertToString (OriginalObject,
                &NewObject, ACPI_IMPLICIT_CONVERT_HEX);
            if (ACPI_FAILURE (Status))
            {
                return (Status);
            }
        }
        break;

    case ACPI_TYPE_BUFFER:
        /*
         * Buffer-to-String conversion. Use a ToString
         * conversion, no transform performed on the buffer data. The best
         * example of this is the _BIF method, where the string data from
         * the battery is often (incorrectly) returned as buffer object(s).
         */
        Length = 0;
        while ((Length < OriginalObject->Buffer.Length) &&
                (OriginalObject->Buffer.Pointer[Length]))
        {
            Length++;
        }

        /* Allocate a new string object */

        NewObject = AcpiUtCreateStringObject (Length);
        if (!NewObject)
        {
            return (AE_NO_MEMORY);
        }

        /*
         * Copy the raw buffer data with no transform. String is already NULL
         * terminated at Length+1.
         */
        memcpy (NewObject->String.Pointer,
            OriginalObject->Buffer.Pointer, Length);
        break;

    default:

        return (AE_AML_OPERAND_TYPE);
    }

    *ReturnObject = NewObject;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsConvertToBuffer
 *
 * PARAMETERS:  OriginalObject      - Object to be converted
 *              ReturnObject        - Where the new converted object is returned
 *
 * RETURN:      Status. AE_OK if conversion was successful.
 *
 * DESCRIPTION: Attempt to convert a Integer/String/Package object to a Buffer.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsConvertToBuffer (
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ReturnObject)
{
    ACPI_OPERAND_OBJECT     *NewObject;
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     **Elements;
    UINT32                  *DwordBuffer;
    UINT32                  Count;
    UINT32                  i;


    switch (OriginalObject->Common.Type)
    {
    case ACPI_TYPE_INTEGER:
        /*
         * Integer-to-Buffer conversion.
         * Convert the Integer to a packed-byte buffer. _MAT and other
         * objects need this sometimes, if a read has been performed on a
         * Field object that is less than or equal to the global integer
         * size (32 or 64 bits).
         */
        Status = AcpiExConvertToBuffer (OriginalObject, &NewObject);
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
        break;

    case ACPI_TYPE_STRING:

        /* String-to-Buffer conversion. Simple data copy */

        NewObject = AcpiUtCreateBufferObject
            (OriginalObject->String.Length);
        if (!NewObject)
        {
            return (AE_NO_MEMORY);
        }

        memcpy (NewObject->Buffer.Pointer,
            OriginalObject->String.Pointer, OriginalObject->String.Length);
        break;

    case ACPI_TYPE_PACKAGE:
        /*
         * This case is often seen for predefined names that must return a
         * Buffer object with multiple DWORD integers within. For example,
         * _FDE and _GTM. The Package can be converted to a Buffer.
         */

        /* All elements of the Package must be integers */

        Elements = OriginalObject->Package.Elements;
        Count = OriginalObject->Package.Count;

        for (i = 0; i < Count; i++)
        {
            if ((!*Elements) ||
                ((*Elements)->Common.Type != ACPI_TYPE_INTEGER))
            {
                return (AE_AML_OPERAND_TYPE);
            }
            Elements++;
        }

        /* Create the new buffer object to replace the Package */

        NewObject = AcpiUtCreateBufferObject (ACPI_MUL_4 (Count));
        if (!NewObject)
        {
            return (AE_NO_MEMORY);
        }

        /* Copy the package elements (integers) to the buffer as DWORDs */

        Elements = OriginalObject->Package.Elements;
        DwordBuffer = ACPI_CAST_PTR (UINT32, NewObject->Buffer.Pointer);

        for (i = 0; i < Count; i++)
        {
            *DwordBuffer = (UINT32) (*Elements)->Integer.Value;
            DwordBuffer++;
            Elements++;
        }
        break;

    default:

        return (AE_AML_OPERAND_TYPE);
    }

    *ReturnObject = NewObject;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsConvertToUnicode
 *
 * PARAMETERS:  Scope               - Namespace node for the method/object
 *              OriginalObject      - ASCII String Object to be converted
 *              ReturnObject        - Where the new converted object is returned
 *
 * RETURN:      Status. AE_OK if conversion was successful.
 *
 * DESCRIPTION: Attempt to convert a String object to a Unicode string Buffer.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsConvertToUnicode (
    ACPI_NAMESPACE_NODE     *Scope,
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ReturnObject)
{
    ACPI_OPERAND_OBJECT     *NewObject;
    char                    *AsciiString;
    UINT16                  *UnicodeBuffer;
    UINT32                  UnicodeLength;
    UINT32                  i;


    if (!OriginalObject)
    {
        return (AE_OK);
    }

    /* If a Buffer was returned, it must be at least two bytes long */

    if (OriginalObject->Common.Type == ACPI_TYPE_BUFFER)
    {
        if (OriginalObject->Buffer.Length < 2)
        {
            return (AE_AML_OPERAND_VALUE);
        }

        *ReturnObject = NULL;
        return (AE_OK);
    }

    /*
     * The original object is an ASCII string. Convert this string to
     * a unicode buffer.
     */
    AsciiString = OriginalObject->String.Pointer;
    UnicodeLength = (OriginalObject->String.Length * 2) + 2;

    /* Create a new buffer object for the Unicode data */

    NewObject = AcpiUtCreateBufferObject (UnicodeLength);
    if (!NewObject)
    {
        return (AE_NO_MEMORY);
    }

    UnicodeBuffer = ACPI_CAST_PTR (UINT16, NewObject->Buffer.Pointer);

    /* Convert ASCII to Unicode */

    for (i = 0; i < OriginalObject->String.Length; i++)
    {
        UnicodeBuffer[i] = (UINT16) AsciiString[i];
    }

    *ReturnObject = NewObject;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsConvertToResource
 *
 * PARAMETERS:  Scope               - Namespace node for the method/object
 *              OriginalObject      - Object to be converted
 *              ReturnObject        - Where the new converted object is returned
 *
 * RETURN:      Status. AE_OK if conversion was successful
 *
 * DESCRIPTION: Attempt to convert a Integer object to a ResourceTemplate
 *              Buffer.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsConvertToResource (
    ACPI_NAMESPACE_NODE     *Scope,
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ReturnObject)
{
    ACPI_OPERAND_OBJECT     *NewObject;
    UINT8                   *Buffer;


    /*
     * We can fix the following cases for an expected resource template:
     * 1. No return value (interpreter slack mode is disabled)
     * 2. A "Return (Zero)" statement
     * 3. A "Return empty buffer" statement
     *
     * We will return a buffer containing a single EndTag
     * resource descriptor.
     */
    if (OriginalObject)
    {
        switch (OriginalObject->Common.Type)
        {
        case ACPI_TYPE_INTEGER:

            /* We can only repair an Integer==0 */

            if (OriginalObject->Integer.Value)
            {
                return (AE_AML_OPERAND_TYPE);
            }
            break;

        case ACPI_TYPE_BUFFER:

            if (OriginalObject->Buffer.Length)
            {
                /* Additional checks can be added in the future */

                *ReturnObject = NULL;
                return (AE_OK);
            }
            break;

        case ACPI_TYPE_STRING:
        default:

            return (AE_AML_OPERAND_TYPE);
        }
    }

    /* Create the new buffer object for the resource descriptor */

    NewObject = AcpiUtCreateBufferObject (2);
    if (!NewObject)
    {
        return (AE_NO_MEMORY);
    }

    Buffer = ACPI_CAST_PTR (UINT8, NewObject->Buffer.Pointer);

    /* Initialize the Buffer with a single EndTag descriptor */

    Buffer[0] = (ACPI_RESOURCE_NAME_END_TAG | ASL_RDESC_END_TAG_SIZE);
    Buffer[1] = 0x00;

    *ReturnObject = NewObject;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsConvertToReference
 *
 * PARAMETERS:  Scope               - Namespace node for the method/object
 *              OriginalObject      - Object to be converted
 *              ReturnObject        - Where the new converted object is returned
 *
 * RETURN:      Status. AE_OK if conversion was successful
 *
 * DESCRIPTION: Attempt to convert a Integer object to a ObjectReference.
 *              Buffer.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsConvertToReference (
    ACPI_NAMESPACE_NODE     *Scope,
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ReturnObject)
{
    ACPI_OPERAND_OBJECT     *NewObject = NULL;
    ACPI_STATUS             Status;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_GENERIC_STATE      ScopeInfo;
    char                    *Name;


    ACPI_FUNCTION_NAME (NsConvertToReference);


    /* Convert path into internal presentation */

    Status = AcpiNsInternalizeName (OriginalObject->String.Pointer, &Name);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Find the namespace node */

    ScopeInfo.Scope.Node = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE, Scope);
    Status = AcpiNsLookup (&ScopeInfo, Name,
        ACPI_TYPE_ANY, ACPI_IMODE_EXECUTE,
        ACPI_NS_SEARCH_PARENT | ACPI_NS_DONT_OPEN_SCOPE, NULL, &Node);
    if (ACPI_FAILURE (Status))
    {
        /* Check if we are resolving a named reference within a package */

        ACPI_ERROR_NAMESPACE (&ScopeInfo,
            OriginalObject->String.Pointer, Status);
        goto ErrorExit;
    }

    /* Create and init a new internal ACPI object */

    NewObject = AcpiUtCreateInternalObject (ACPI_TYPE_LOCAL_REFERENCE);
    if (!NewObject)
    {
        Status = AE_NO_MEMORY;
        goto ErrorExit;
    }
    NewObject->Reference.Node = Node;
    NewObject->Reference.Object = Node->Object;
    NewObject->Reference.Class = ACPI_REFCLASS_NAME;

    /*
     * Increase reference of the object if needed (the object is likely a
     * null for device nodes).
     */
    AcpiUtAddReference (Node->Object);

ErrorExit:
    ACPI_FREE (Name);
    *ReturnObject = NewObject;
    return (Status);
}
