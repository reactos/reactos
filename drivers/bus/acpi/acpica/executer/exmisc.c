/******************************************************************************
 *
 * Module Name: exmisc - ACPI AML (p-code) execution - specific opcodes
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
#include "amlresrc.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exmisc")


/*******************************************************************************
 *
 * FUNCTION:    AcpiExGetObjectReference
 *
 * PARAMETERS:  ObjDesc             - Create a reference to this object
 *              ReturnDesc          - Where to store the reference
 *              WalkState           - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Obtain and return a "reference" to the target object
 *              Common code for the RefOfOp and the CondRefOfOp.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExGetObjectReference (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ReturnDesc,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     *ReferenceObj;
    ACPI_OPERAND_OBJECT     *ReferencedObj;


    ACPI_FUNCTION_TRACE_PTR (ExGetObjectReference, ObjDesc);


    *ReturnDesc = NULL;

    switch (ACPI_GET_DESCRIPTOR_TYPE (ObjDesc))
    {
    case ACPI_DESC_TYPE_OPERAND:

        if (ObjDesc->Common.Type != ACPI_TYPE_LOCAL_REFERENCE)
        {
            return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
        }

        /*
         * Must be a reference to a Local or Arg
         */
        switch (ObjDesc->Reference.Class)
        {
        case ACPI_REFCLASS_LOCAL:
        case ACPI_REFCLASS_ARG:
        case ACPI_REFCLASS_DEBUG:

            /* The referenced object is the pseudo-node for the local/arg */

            ReferencedObj = ObjDesc->Reference.Object;
            break;

        default:

            ACPI_ERROR ((AE_INFO, "Invalid Reference Class 0x%2.2X",
                ObjDesc->Reference.Class));
            return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
        }
        break;

    case ACPI_DESC_TYPE_NAMED:
        /*
         * A named reference that has already been resolved to a Node
         */
        ReferencedObj = ObjDesc;
        break;

    default:

        ACPI_ERROR ((AE_INFO, "Invalid descriptor type 0x%X",
            ACPI_GET_DESCRIPTOR_TYPE (ObjDesc)));
        return_ACPI_STATUS (AE_TYPE);
    }


    /* Create a new reference object */

    ReferenceObj = AcpiUtCreateInternalObject (ACPI_TYPE_LOCAL_REFERENCE);
    if (!ReferenceObj)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    ReferenceObj->Reference.Class = ACPI_REFCLASS_REFOF;
    ReferenceObj->Reference.Object = ReferencedObj;
    *ReturnDesc = ReferenceObj;

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
        "Object %p Type [%s], returning Reference %p\n",
        ObjDesc, AcpiUtGetObjectTypeName (ObjDesc), *ReturnDesc));

    return_ACPI_STATUS (AE_OK);
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
 * DESCRIPTION: Concatenate two objects OF THE SAME TYPE.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExDoConcatenate (
    ACPI_OPERAND_OBJECT     *Operand0,
    ACPI_OPERAND_OBJECT     *Operand1,
    ACPI_OPERAND_OBJECT     **ActualReturnDesc,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     *LocalOperand1 = Operand1;
    ACPI_OPERAND_OBJECT     *ReturnDesc;
    char                    *NewBuf;
    const char              *TypeString;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (ExDoConcatenate);


    /*
     * Convert the second operand if necessary. The first operand
     * determines the type of the second operand, (See the Data Types
     * section of the ACPI specification.)  Both object types are
     * guaranteed to be either Integer/String/Buffer by the operand
     * resolution mechanism.
     */
    switch (Operand0->Common.Type)
    {
    case ACPI_TYPE_INTEGER:

        Status = AcpiExConvertToInteger (Operand1, &LocalOperand1, 16);
        break;

    case ACPI_TYPE_STRING:
        /*
         * Per the ACPI spec, Concatenate only supports int/str/buf.
         * However, we support all objects here as an extension.
         * This improves the usefulness of the Printf() macro.
         * 12/2015.
         */
        switch (Operand1->Common.Type)
        {
        case ACPI_TYPE_INTEGER:
        case ACPI_TYPE_STRING:
        case ACPI_TYPE_BUFFER:

            Status = AcpiExConvertToString (
                Operand1, &LocalOperand1, ACPI_IMPLICIT_CONVERT_HEX);
            break;

        default:
            /*
             * Just emit a string containing the object type.
             */
            TypeString = AcpiUtGetTypeName (Operand1->Common.Type);

            LocalOperand1 = AcpiUtCreateStringObject (
                ((ACPI_SIZE) strlen (TypeString) + 9)); /* 9 For "[Object]" */
            if (!LocalOperand1)
            {
                Status = AE_NO_MEMORY;
                goto Cleanup;
            }

            strcpy (LocalOperand1->String.Pointer, "[");
            strcat (LocalOperand1->String.Pointer, TypeString);
            strcat (LocalOperand1->String.Pointer, " Object]");
            Status = AE_OK;
            break;
        }
        break;

    case ACPI_TYPE_BUFFER:

        Status = AcpiExConvertToBuffer (Operand1, &LocalOperand1);
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

    /*
     * Both operands are now known to be the same object type
     * (Both are Integer, String, or Buffer), and we can now perform the
     * concatenation.
     */

    /*
     * There are three cases to handle:
     *
     * 1) Two Integers concatenated to produce a new Buffer
     * 2) Two Strings concatenated to produce a new String
     * 3) Two Buffers concatenated to produce a new Buffer
     */
    switch (Operand0->Common.Type)
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

        NewBuf = (char *) ReturnDesc->Buffer.Pointer;

        /* Copy the first integer, LSB first */

        memcpy (NewBuf, &Operand0->Integer.Value,
            AcpiGbl_IntegerByteWidth);

        /* Copy the second integer (LSB first) after the first */

        memcpy (NewBuf + AcpiGbl_IntegerByteWidth,
            &LocalOperand1->Integer.Value, AcpiGbl_IntegerByteWidth);
        break;

    case ACPI_TYPE_STRING:

        /* Result of two Strings is a String */

        ReturnDesc = AcpiUtCreateStringObject (
            ((ACPI_SIZE) Operand0->String.Length +
            LocalOperand1->String.Length));
        if (!ReturnDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        NewBuf = ReturnDesc->String.Pointer;

        /* Concatenate the strings */

        strcpy (NewBuf, Operand0->String.Pointer);
        strcat (NewBuf, LocalOperand1->String.Pointer);
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

        NewBuf = (char *) ReturnDesc->Buffer.Pointer;

        /* Concatenate the buffers */

        memcpy (NewBuf, Operand0->Buffer.Pointer,
            Operand0->Buffer.Length);
        memcpy (NewBuf + Operand0->Buffer.Length,
            LocalOperand1->Buffer.Pointer,
            LocalOperand1->Buffer.Length);
        break;

    default:

        /* Invalid object type, should not happen here */

        ACPI_ERROR ((AE_INFO, "Invalid object type: 0x%X",
            Operand0->Common.Type));
        Status =AE_AML_INTERNAL;
        goto Cleanup;
    }

    *ActualReturnDesc = ReturnDesc;

Cleanup:
    if (LocalOperand1 != Operand1)
    {
        AcpiUtRemoveReference (LocalOperand1);
    }
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDoMathOp
 *
 * PARAMETERS:  Opcode              - AML opcode
 *              Integer0            - Integer operand #0
 *              Integer1            - Integer operand #1
 *
 * RETURN:      Integer result of the operation
 *
 * DESCRIPTION: Execute a math AML opcode. The purpose of having all of the
 *              math functions here is to prevent a lot of pointer dereferencing
 *              to obtain the operands.
 *
 ******************************************************************************/

UINT64
AcpiExDoMathOp (
    UINT16                  Opcode,
    UINT64                  Integer0,
    UINT64                  Integer1)
{

    ACPI_FUNCTION_ENTRY ();


    switch (Opcode)
    {
    case AML_ADD_OP:                /* Add (Integer0, Integer1, Result) */

        return (Integer0 + Integer1);

    case AML_BIT_AND_OP:            /* And (Integer0, Integer1, Result) */

        return (Integer0 & Integer1);

    case AML_BIT_NAND_OP:           /* NAnd (Integer0, Integer1, Result) */

        return (~(Integer0 & Integer1));

    case AML_BIT_OR_OP:             /* Or (Integer0, Integer1, Result) */

        return (Integer0 | Integer1);

    case AML_BIT_NOR_OP:            /* NOr (Integer0, Integer1, Result) */

        return (~(Integer0 | Integer1));

    case AML_BIT_XOR_OP:            /* XOr (Integer0, Integer1, Result) */

        return (Integer0 ^ Integer1);

    case AML_MULTIPLY_OP:           /* Multiply (Integer0, Integer1, Result) */

        return (Integer0 * Integer1);

    case AML_SHIFT_LEFT_OP:         /* ShiftLeft (Operand, ShiftCount, Result)*/

        /*
         * We need to check if the shiftcount is larger than the integer bit
         * width since the behavior of this is not well-defined in the C language.
         */
        if (Integer1 >= AcpiGbl_IntegerBitWidth)
        {
            return (0);
        }
        return (Integer0 << Integer1);

    case AML_SHIFT_RIGHT_OP:        /* ShiftRight (Operand, ShiftCount, Result) */

        /*
         * We need to check if the shiftcount is larger than the integer bit
         * width since the behavior of this is not well-defined in the C language.
         */
        if (Integer1 >= AcpiGbl_IntegerBitWidth)
        {
            return (0);
        }
        return (Integer0 >> Integer1);

    case AML_SUBTRACT_OP:           /* Subtract (Integer0, Integer1, Result) */

        return (Integer0 - Integer1);

    default:

        return (0);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDoLogicalNumericOp
 *
 * PARAMETERS:  Opcode              - AML opcode
 *              Integer0            - Integer operand #0
 *              Integer1            - Integer operand #1
 *              LogicalResult       - TRUE/FALSE result of the operation
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute a logical "Numeric" AML opcode. For these Numeric
 *              operators (LAnd and LOr), both operands must be integers.
 *
 *              Note: cleanest machine code seems to be produced by the code
 *              below, rather than using statements of the form:
 *                  Result = (Integer0 && Integer1);
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExDoLogicalNumericOp (
    UINT16                  Opcode,
    UINT64                  Integer0,
    UINT64                  Integer1,
    BOOLEAN                 *LogicalResult)
{
    ACPI_STATUS             Status = AE_OK;
    BOOLEAN                 LocalResult = FALSE;


    ACPI_FUNCTION_TRACE (ExDoLogicalNumericOp);


    switch (Opcode)
    {
    case AML_LAND_OP:               /* LAnd (Integer0, Integer1) */

        if (Integer0 && Integer1)
        {
            LocalResult = TRUE;
        }
        break;

    case AML_LOR_OP:                /* LOr (Integer0, Integer1) */

        if (Integer0 || Integer1)
        {
            LocalResult = TRUE;
        }
        break;

    default:

        Status = AE_AML_INTERNAL;
        break;
    }

    /* Return the logical result and status */

    *LogicalResult = LocalResult;
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDoLogicalOp
 *
 * PARAMETERS:  Opcode              - AML opcode
 *              Operand0            - operand #0
 *              Operand1            - operand #1
 *              LogicalResult       - TRUE/FALSE result of the operation
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute a logical AML opcode. The purpose of having all of the
 *              functions here is to prevent a lot of pointer dereferencing
 *              to obtain the operands and to simplify the generation of the
 *              logical value. For the Numeric operators (LAnd and LOr), both
 *              operands must be integers. For the other logical operators,
 *              operands can be any combination of Integer/String/Buffer. The
 *              first operand determines the type to which the second operand
 *              will be converted.
 *
 *              Note: cleanest machine code seems to be produced by the code
 *              below, rather than using statements of the form:
 *                  Result = (Operand0 == Operand1);
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExDoLogicalOp (
    UINT16                  Opcode,
    ACPI_OPERAND_OBJECT     *Operand0,
    ACPI_OPERAND_OBJECT     *Operand1,
    BOOLEAN                 *LogicalResult)
{
    ACPI_OPERAND_OBJECT     *LocalOperand1 = Operand1;
    UINT64                  Integer0;
    UINT64                  Integer1;
    UINT32                  Length0;
    UINT32                  Length1;
    ACPI_STATUS             Status = AE_OK;
    BOOLEAN                 LocalResult = FALSE;
    int                     Compare;


    ACPI_FUNCTION_TRACE (ExDoLogicalOp);


    /*
     * Convert the second operand if necessary. The first operand
     * determines the type of the second operand, (See the Data Types
     * section of the ACPI 3.0+ specification.)  Both object types are
     * guaranteed to be either Integer/String/Buffer by the operand
     * resolution mechanism.
     */
    switch (Operand0->Common.Type)
    {
    case ACPI_TYPE_INTEGER:

        Status = AcpiExConvertToInteger (Operand1, &LocalOperand1, 16);
        break;

    case ACPI_TYPE_STRING:

        Status = AcpiExConvertToString (
            Operand1, &LocalOperand1, ACPI_IMPLICIT_CONVERT_HEX);
        break;

    case ACPI_TYPE_BUFFER:

        Status = AcpiExConvertToBuffer (Operand1, &LocalOperand1);
        break;

    default:

        Status = AE_AML_INTERNAL;
        break;
    }

    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    /*
     * Two cases: 1) Both Integers, 2) Both Strings or Buffers
     */
    if (Operand0->Common.Type == ACPI_TYPE_INTEGER)
    {
        /*
         * 1) Both operands are of type integer
         *    Note: LocalOperand1 may have changed above
         */
        Integer0 = Operand0->Integer.Value;
        Integer1 = LocalOperand1->Integer.Value;

        switch (Opcode)
        {
        case AML_LEQUAL_OP:             /* LEqual (Operand0, Operand1) */

            if (Integer0 == Integer1)
            {
                LocalResult = TRUE;
            }
            break;

        case AML_LGREATER_OP:           /* LGreater (Operand0, Operand1) */

            if (Integer0 > Integer1)
            {
                LocalResult = TRUE;
            }
            break;

        case AML_LLESS_OP:              /* LLess (Operand0, Operand1) */

            if (Integer0 < Integer1)
            {
                LocalResult = TRUE;
            }
            break;

        default:

            Status = AE_AML_INTERNAL;
            break;
        }
    }
    else
    {
        /*
         * 2) Both operands are Strings or both are Buffers
         *    Note: Code below takes advantage of common Buffer/String
         *          object fields. LocalOperand1 may have changed above. Use
         *          memcmp to handle nulls in buffers.
         */
        Length0 = Operand0->Buffer.Length;
        Length1 = LocalOperand1->Buffer.Length;

        /* Lexicographic compare: compare the data bytes */

        Compare = memcmp (Operand0->Buffer.Pointer,
            LocalOperand1->Buffer.Pointer,
            (Length0 > Length1) ? Length1 : Length0);

        switch (Opcode)
        {
        case AML_LEQUAL_OP:             /* LEqual (Operand0, Operand1) */

            /* Length and all bytes must be equal */

            if ((Length0 == Length1) &&
                (Compare == 0))
            {
                /* Length and all bytes match ==> TRUE */

                LocalResult = TRUE;
            }
            break;

        case AML_LGREATER_OP:           /* LGreater (Operand0, Operand1) */

            if (Compare > 0)
            {
                LocalResult = TRUE;
                goto Cleanup;   /* TRUE */
            }
            if (Compare < 0)
            {
                goto Cleanup;   /* FALSE */
            }

            /* Bytes match (to shortest length), compare lengths */

            if (Length0 > Length1)
            {
                LocalResult = TRUE;
            }
            break;

        case AML_LLESS_OP:              /* LLess (Operand0, Operand1) */

            if (Compare > 0)
            {
                goto Cleanup;   /* FALSE */
            }
            if (Compare < 0)
            {
                LocalResult = TRUE;
                goto Cleanup;   /* TRUE */
            }

            /* Bytes match (to shortest length), compare lengths */

            if (Length0 < Length1)
            {
                LocalResult = TRUE;
            }
            break;

        default:

            Status = AE_AML_INTERNAL;
            break;
        }
    }

Cleanup:

    /* New object was created if implicit conversion performed - delete */

    if (LocalOperand1 != Operand1)
    {
        AcpiUtRemoveReference (LocalOperand1);
    }

    /* Return the logical result and status */

    *LogicalResult = LocalResult;
    return_ACPI_STATUS (Status);
}
