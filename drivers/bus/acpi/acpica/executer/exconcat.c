/******************************************************************************
 *
 * Module Name: exconcat - Concatenate-type AML operators
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
#include "amlresrc.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exconcat")

/* Local Prototypes */

static ACPI_STATUS
AcpiExConvertToObjectTypeString (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc);


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDoConcatenate
 *
 * PARAMETERS:  Operand0            - First source object
 *              Operand1            - Second source object
 *              ActualReturnDesc    - Where to place the return object
 *              WalkState           - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Concatenate two objects with the ACPI-defined conversion
 *              rules as necessary.
 * NOTE:
 * Per the ACPI spec (up to 6.1), Concatenate only supports Integer,
 * String, and Buffer objects. However, we support all objects here
 * as an extension. This improves the usefulness of both Concatenate
 * and the Printf/Fprintf macros. The extension returns a string
 * describing the object type for the other objects.
 * 02/2016.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExDoConcatenate (
    ACPI_OPERAND_OBJECT     *Operand0,
    ACPI_OPERAND_OBJECT     *Operand1,
    ACPI_OPERAND_OBJECT     **ActualReturnDesc,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     *LocalOperand0 = Operand0;
    ACPI_OPERAND_OBJECT     *LocalOperand1 = Operand1;
    ACPI_OPERAND_OBJECT     *TempOperand1 = NULL;
    ACPI_OPERAND_OBJECT     *ReturnDesc;
    char                    *Buffer;
    ACPI_OBJECT_TYPE        Operand0Type;
    ACPI_OBJECT_TYPE        Operand1Type;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (ExDoConcatenate);


    /* Operand 0 preprocessing */

    switch (Operand0->Common.Type)
    {
    case ACPI_TYPE_INTEGER:
    case ACPI_TYPE_STRING:
    case ACPI_TYPE_BUFFER:

        Operand0Type = Operand0->Common.Type;
        break;

    default:

        /* For all other types, get the "object type" string */

        Status = AcpiExConvertToObjectTypeString (
            Operand0, &LocalOperand0);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }

        Operand0Type = ACPI_TYPE_STRING;
        break;
    }

    /* Operand 1 preprocessing */

    switch (Operand1->Common.Type)
    {
    case ACPI_TYPE_INTEGER:
    case ACPI_TYPE_STRING:
    case ACPI_TYPE_BUFFER:

        Operand1Type = Operand1->Common.Type;
        break;

    default:

        /* For all other types, get the "object type" string */

        Status = AcpiExConvertToObjectTypeString (
            Operand1, &LocalOperand1);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }

        Operand1Type = ACPI_TYPE_STRING;
        break;
    }

    /*
     * Convert the second operand if necessary. The first operand (0)
     * determines the type of the second operand (1) (See the Data Types
     * section of the ACPI specification). Both object types are
     * guaranteed to be either Integer/String/Buffer by the operand
     * resolution mechanism.
     */
    switch (Operand0Type)
    {
    case ACPI_TYPE_INTEGER:

        Status = AcpiExConvertToInteger (LocalOperand1, &TempOperand1,
            ACPI_IMPLICIT_CONVERSION);
        break;

    case ACPI_TYPE_BUFFER:

        Status = AcpiExConvertToBuffer (LocalOperand1, &TempOperand1);
        break;

    case ACPI_TYPE_STRING:

        switch (Operand1Type)
        {
        case ACPI_TYPE_INTEGER:
        case ACPI_TYPE_STRING:
        case ACPI_TYPE_BUFFER:

            /* Other types have already been converted to string */

            Status = AcpiExConvertToString (
                LocalOperand1, &TempOperand1, ACPI_IMPLICIT_CONVERT_HEX);
            break;

        default:

            Status = AE_OK;
            break;
        }
        break;

    default:

        ACPI_ERROR ((AE_INFO, "Invalid object type: 0x%X",
            Operand0->Common.Type));
        Status = AE_AML_INTERNAL;
    }

    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    /* Take care with any newly created operand objects */

    if ((LocalOperand1 != Operand1) &&
        (LocalOperand1 != TempOperand1))
    {
        AcpiUtRemoveReference (LocalOperand1);
    }

    LocalOperand1 = TempOperand1;

    /*
     * Both operands are now known to be the same object type
     * (Both are Integer, String, or Buffer), and we can now perform
     * the concatenation.
     *
     * There are three cases to handle, as per the ACPI spec:
     *
     * 1) Two Integers concatenated to produce a new Buffer
     * 2) Two Strings concatenated to produce a new String
     * 3) Two Buffers concatenated to produce a new Buffer
     */
    switch (Operand0Type)
    {
    case ACPI_TYPE_INTEGER:

        /* Result of two Integers is a Buffer */
        /* Need enough buffer space for two integers */

        ReturnDesc = AcpiUtCreateBufferObject (
            (ACPI_SIZE) ACPI_MUL_2 (AcpiGbl_IntegerByteWidth));
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        Buffer = (char *) ReturnDesc->Buffer.Pointer;

        /* Copy the first integer, LSB first */

        memcpy (Buffer, &Operand0->Integer.Value,
            AcpiGbl_IntegerByteWidth);

        /* Copy the second integer (LSB first) after the first */

        memcpy (Buffer + AcpiGbl_IntegerByteWidth,
            &LocalOperand1->Integer.Value, AcpiGbl_IntegerByteWidth);
        break;

    case ACPI_TYPE_STRING:

        /* Result of two Strings is a String */

        ReturnDesc = AcpiUtCreateStringObject (
            ((ACPI_SIZE) LocalOperand0->String.Length +
            LocalOperand1->String.Length));
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        Buffer = ReturnDesc->String.Pointer;

        /* Concatenate the strings */

        strcpy (Buffer, LocalOperand0->String.Pointer);
        strcat (Buffer, LocalOperand1->String.Pointer);
        break;

    case ACPI_TYPE_BUFFER:

        /* Result of two Buffers is a Buffer */

        ReturnDesc = AcpiUtCreateBufferObject (
            ((ACPI_SIZE) Operand0->Buffer.Length +
            LocalOperand1->Buffer.Length));
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        Buffer = (char *) ReturnDesc->Buffer.Pointer;

        /* Concatenate the buffers */

        memcpy (Buffer, Operand0->Buffer.Pointer,
            Operand0->Buffer.Length);
        memcpy (Buffer + Operand0->Buffer.Length,
            LocalOperand1->Buffer.Pointer,
            LocalOperand1->Buffer.Length);
        break;

    default:

        /* Invalid object type, should not happen here */

        ACPI_ERROR ((AE_INFO, "Invalid object type: 0x%X",
            Operand0->Common.Type));
        Status = AE_AML_INTERNAL;
        goto Cleanup;
    }

    *ActualReturnDesc = ReturnDesc;

Cleanup:
    if (LocalOperand0 != Operand0)
    {
        AcpiUtRemoveReference (LocalOperand0);
    }

    if (LocalOperand1 != Operand1)
    {
        AcpiUtRemoveReference (LocalOperand1);
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExConvertToObjectTypeString
 *
 * PARAMETERS:  ObjDesc             - Object to be converted
 *              ReturnDesc          - Where to place the return object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert an object of arbitrary type to a string object that
 *              contains the namestring for the object. Used for the
 *              concatenate operator.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiExConvertToObjectTypeString (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc)
{
    ACPI_OPERAND_OBJECT     *ReturnDesc;
    const char              *TypeString;


    TypeString = AcpiUtGetTypeName (ObjDesc->Common.Type);

    ReturnDesc = AcpiUtCreateStringObject (
        ((ACPI_SIZE) strlen (TypeString) + 9)); /* 9 For "[ Object]" */
    if (!ReturnDesc)
    {
        return (AE_NO_MEMORY);
    }

    strcpy (ReturnDesc->String.Pointer, "[");
    strcat (ReturnDesc->String.Pointer, TypeString);
    strcat (ReturnDesc->String.Pointer, " Object]");

    *ResultDesc = ReturnDesc;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExConcatTemplate
 *
 * PARAMETERS:  Operand0            - First source object
 *              Operand1            - Second source object
 *              ActualReturnDesc    - Where to place the return object
 *              WalkState           - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Concatenate two resource templates
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExConcatTemplate (
    ACPI_OPERAND_OBJECT     *Operand0,
    ACPI_OPERAND_OBJECT     *Operand1,
    ACPI_OPERAND_OBJECT     **ActualReturnDesc,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ReturnDesc;
    UINT8                   *NewBuf;
    UINT8                   *EndTag;
    ACPI_SIZE               Length0;
    ACPI_SIZE               Length1;
    ACPI_SIZE               NewLength;


    ACPI_FUNCTION_TRACE (ExConcatTemplate);


    /*
     * Find the EndTag descriptor in each resource template.
     * Note1: returned pointers point TO the EndTag, not past it.
     * Note2: zero-length buffers are allowed; treated like one EndTag
     */

    /* Get the length of the first resource template */

    Status = AcpiUtGetResourceEndTag (Operand0, &EndTag);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Length0 = ACPI_PTR_DIFF (EndTag, Operand0->Buffer.Pointer);

    /* Get the length of the second resource template */

    Status = AcpiUtGetResourceEndTag (Operand1, &EndTag);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Length1 = ACPI_PTR_DIFF (EndTag, Operand1->Buffer.Pointer);

    /* Combine both lengths, minimum size will be 2 for EndTag */

    NewLength = Length0 + Length1 + sizeof (AML_RESOURCE_END_TAG);

    /* Create a new buffer object for the result (with one EndTag) */

    ReturnDesc = AcpiUtCreateBufferObject (NewLength);
    if (!ReturnDesc)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /*
     * Copy the templates to the new buffer, 0 first, then 1 follows. One
     * EndTag descriptor is copied from Operand1.
     */
    NewBuf = ReturnDesc->Buffer.Pointer;
    memcpy (NewBuf, Operand0->Buffer.Pointer, Length0);
    memcpy (NewBuf + Length0, Operand1->Buffer.Pointer, Length1);

    /* Insert EndTag and set the checksum to zero, means "ignore checksum" */

    NewBuf[NewLength - 1] = 0;
    NewBuf[NewLength - 2] = ACPI_RESOURCE_NAME_END_TAG | 1;

    /* Return the completed resource template */

    *ActualReturnDesc = ReturnDesc;
    return_ACPI_STATUS (AE_OK);
}
