/******************************************************************************
 *
 * Module Name: exresop - AML Interpreter operand/object resolution
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
#include "amlcode.h"
#include "acparser.h"
#include "acinterp.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exresop")

/* Local prototypes */

static ACPI_STATUS
AcpiExCheckObjectType (
    ACPI_OBJECT_TYPE        TypeNeeded,
    ACPI_OBJECT_TYPE        ThisType,
    void                    *Object);


/*******************************************************************************
 *
 * FUNCTION:    AcpiExCheckObjectType
 *
 * PARAMETERS:  TypeNeeded          Object type needed
 *              ThisType            Actual object type
 *              Object              Object pointer
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check required type against actual type
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiExCheckObjectType (
    ACPI_OBJECT_TYPE        TypeNeeded,
    ACPI_OBJECT_TYPE        ThisType,
    void                    *Object)
{
    ACPI_FUNCTION_ENTRY ();


    if (TypeNeeded == ACPI_TYPE_ANY)
    {
        /* All types OK, so we don't perform any typechecks */

        return (AE_OK);
    }

    if (TypeNeeded == ACPI_TYPE_LOCAL_REFERENCE)
    {
        /*
         * Allow the AML "Constant" opcodes (Zero, One, etc.) to be reference
         * objects and thus allow them to be targets. (As per the ACPI
         * specification, a store to a constant is a noop.)
         */
        if ((ThisType == ACPI_TYPE_INTEGER) &&
            (((ACPI_OPERAND_OBJECT *) Object)->Common.Flags &
                AOPOBJ_AML_CONSTANT))
        {
            return (AE_OK);
        }
    }

    if (TypeNeeded != ThisType)
    {
        ACPI_ERROR ((AE_INFO,
            "Needed type [%s], found [%s] %p",
            AcpiUtGetTypeName (TypeNeeded),
            AcpiUtGetTypeName (ThisType), Object));

        return (AE_AML_OPERAND_TYPE);
    }

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExResolveOperands
 *
 * PARAMETERS:  Opcode              - Opcode being interpreted
 *              StackPtr            - Pointer to the operand stack to be
 *                                    resolved
 *              WalkState           - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert multiple input operands to the types required by the
 *              target operator.
 *
 *      Each 5-bit group in ArgTypes represents one required
 *      operand and indicates the required Type. The corresponding operand
 *      will be converted to the required type if possible, otherwise we
 *      abort with an exception.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExResolveOperands (
    UINT16                  Opcode,
    ACPI_OPERAND_OBJECT     **StackPtr,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status = AE_OK;
    UINT8                   ObjectType;
    UINT32                  ArgTypes;
    const ACPI_OPCODE_INFO  *OpInfo;
    UINT32                  ThisArgType;
    ACPI_OBJECT_TYPE        TypeNeeded;
    UINT16                  TargetOp = 0;


    ACPI_FUNCTION_TRACE_U32 (ExResolveOperands, Opcode);


    OpInfo = AcpiPsGetOpcodeInfo (Opcode);
    if (OpInfo->Class == AML_CLASS_UNKNOWN)
    {
        return_ACPI_STATUS (AE_AML_BAD_OPCODE);
    }

    ArgTypes = OpInfo->RuntimeArgs;
    if (ArgTypes == ARGI_INVALID_OPCODE)
    {
        ACPI_ERROR ((AE_INFO, "Unknown AML opcode 0x%X",
            Opcode));

        return_ACPI_STATUS (AE_AML_INTERNAL);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
        "Opcode %X [%s] RequiredOperandTypes=%8.8X\n",
        Opcode, OpInfo->Name, ArgTypes));

    /*
     * Normal exit is with (ArgTypes == 0) at end of argument list.
     * Function will return an exception from within the loop upon
     * finding an entry which is not (or cannot be converted
     * to) the required type; if stack underflows; or upon
     * finding a NULL stack entry (which should not happen).
     */
    while (GET_CURRENT_ARG_TYPE (ArgTypes))
    {
        if (!StackPtr || !*StackPtr)
        {
            ACPI_ERROR ((AE_INFO, "Null stack entry at %p",
                StackPtr));

            return_ACPI_STATUS (AE_AML_INTERNAL);
        }

        /* Extract useful items */

        ObjDesc = *StackPtr;

        /* Decode the descriptor type */

        switch (ACPI_GET_DESCRIPTOR_TYPE (ObjDesc))
        {
        case ACPI_DESC_TYPE_NAMED:

            /* Namespace Node */

            ObjectType = ((ACPI_NAMESPACE_NODE *) ObjDesc)->Type;

            /*
             * Resolve an alias object. The construction of these objects
             * guarantees that there is only one level of alias indirection;
             * thus, the attached object is always the aliased namespace node
             */
            if (ObjectType == ACPI_TYPE_LOCAL_ALIAS)
            {
                ObjDesc = AcpiNsGetAttachedObject (
                    (ACPI_NAMESPACE_NODE *) ObjDesc);
                *StackPtr = ObjDesc;
                ObjectType = ((ACPI_NAMESPACE_NODE *) ObjDesc)->Type;
            }
            break;

        case ACPI_DESC_TYPE_OPERAND:

            /* ACPI internal object */

            ObjectType = ObjDesc->Common.Type;

            /* Check for bad ACPI_OBJECT_TYPE */

            if (!AcpiUtValidObjectType (ObjectType))
            {
                ACPI_ERROR ((AE_INFO,
                    "Bad operand object type [0x%X]", ObjectType));

                return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
            }

            if (ObjectType == (UINT8) ACPI_TYPE_LOCAL_REFERENCE)
            {
                /* Validate the Reference */

                switch (ObjDesc->Reference.Class)
                {
                case ACPI_REFCLASS_DEBUG:

                    TargetOp = AML_DEBUG_OP;

                    /*lint -fallthrough */

                case ACPI_REFCLASS_ARG:
                case ACPI_REFCLASS_LOCAL:
                case ACPI_REFCLASS_INDEX:
                case ACPI_REFCLASS_REFOF:
                case ACPI_REFCLASS_TABLE:    /* DdbHandle from LOAD_OP or LOAD_TABLE_OP */
                case ACPI_REFCLASS_NAME:     /* Reference to a named object */

                    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
                        "Operand is a Reference, Class [%s] %2.2X\n",
                        AcpiUtGetReferenceName (ObjDesc),
                        ObjDesc->Reference.Class));
                    break;

                default:

                    ACPI_ERROR ((AE_INFO,
                        "Unknown Reference Class 0x%2.2X in %p",
                        ObjDesc->Reference.Class, ObjDesc));

                    return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
                }
            }
            break;

        default:

            /* Invalid descriptor */

            ACPI_ERROR ((AE_INFO, "Invalid descriptor %p [%s]",
                ObjDesc, AcpiUtGetDescriptorName (ObjDesc)));

            return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
        }

        /* Get one argument type, point to the next */

        ThisArgType = GET_CURRENT_ARG_TYPE (ArgTypes);
        INCREMENT_ARG_LIST (ArgTypes);

        /*
         * Handle cases where the object does not need to be
         * resolved to a value
         */
        switch (ThisArgType)
        {
        case ARGI_REF_OR_STRING:        /* Can be a String or Reference */

            if ((ACPI_GET_DESCRIPTOR_TYPE (ObjDesc) ==
                ACPI_DESC_TYPE_OPERAND) &&
                (ObjDesc->Common.Type == ACPI_TYPE_STRING))
            {
                /*
                 * String found - the string references a named object and
                 * must be resolved to a node
                 */
                goto NextOperand;
            }

            /*
             * Else not a string - fall through to the normal Reference
             * case below
             */
            /*lint -fallthrough */

        case ARGI_REFERENCE:            /* References: */
        case ARGI_INTEGER_REF:
        case ARGI_OBJECT_REF:
        case ARGI_DEVICE_REF:
        case ARGI_TARGETREF:     /* Allows implicit conversion rules before store */
        case ARGI_FIXED_TARGET:  /* No implicit conversion before store to target */
        case ARGI_SIMPLE_TARGET: /* Name, Local, or Arg - no implicit conversion  */
        case ARGI_STORE_TARGET:

            /*
             * Need an operand of type ACPI_TYPE_LOCAL_REFERENCE
             * A Namespace Node is OK as-is
             */
            if (ACPI_GET_DESCRIPTOR_TYPE (ObjDesc) == ACPI_DESC_TYPE_NAMED)
            {
                goto NextOperand;
            }

            Status = AcpiExCheckObjectType (
                ACPI_TYPE_LOCAL_REFERENCE, ObjectType, ObjDesc);
            if (ACPI_FAILURE (Status))
            {
                return_ACPI_STATUS (Status);
            }
            goto NextOperand;

        case ARGI_DATAREFOBJ:  /* Store operator only */
            /*
             * We don't want to resolve IndexOp reference objects during
             * a store because this would be an implicit DeRefOf operation.
             * Instead, we just want to store the reference object.
             * -- All others must be resolved below.
             */
            if ((Opcode == AML_STORE_OP) &&
                ((*StackPtr)->Common.Type == ACPI_TYPE_LOCAL_REFERENCE) &&
                ((*StackPtr)->Reference.Class == ACPI_REFCLASS_INDEX))
            {
                goto NextOperand;
            }
            break;

        default:

            /* All cases covered above */

            break;
        }

        /*
         * Resolve this object to a value
         */
        Status = AcpiExResolveToValue (StackPtr, WalkState);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        /* Get the resolved object */

        ObjDesc = *StackPtr;

        /*
         * Check the resulting object (value) type
         */
        switch (ThisArgType)
        {
        /*
         * For the simple cases, only one type of resolved object
         * is allowed
         */
        case ARGI_MUTEX:

            /* Need an operand of type ACPI_TYPE_MUTEX */

            TypeNeeded = ACPI_TYPE_MUTEX;
            break;

        case ARGI_EVENT:

            /* Need an operand of type ACPI_TYPE_EVENT */

            TypeNeeded = ACPI_TYPE_EVENT;
            break;

        case ARGI_PACKAGE:   /* Package */

            /* Need an operand of type ACPI_TYPE_PACKAGE */

            TypeNeeded = ACPI_TYPE_PACKAGE;
            break;

        case ARGI_ANYTYPE:

            /* Any operand type will do */

            TypeNeeded = ACPI_TYPE_ANY;
            break;

        case ARGI_DDBHANDLE:

            /* Need an operand of type ACPI_TYPE_DDB_HANDLE */

            TypeNeeded = ACPI_TYPE_LOCAL_REFERENCE;
            break;


        /*
         * The more complex cases allow multiple resolved object types
         */
        case ARGI_INTEGER:

            /*
             * Need an operand of type ACPI_TYPE_INTEGER,
             * But we can implicitly convert from a STRING or BUFFER
             * Aka - "Implicit Source Operand Conversion"
             */
            Status = AcpiExConvertToInteger (ObjDesc, StackPtr, 16);
            if (ACPI_FAILURE (Status))
            {
                if (Status == AE_TYPE)
                {
                    ACPI_ERROR ((AE_INFO,
                        "Needed [Integer/String/Buffer], found [%s] %p",
                        AcpiUtGetObjectTypeName (ObjDesc), ObjDesc));

                    return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
                }

                return_ACPI_STATUS (Status);
            }

            if (ObjDesc != *StackPtr)
            {
                AcpiUtRemoveReference (ObjDesc);
            }
            goto NextOperand;

        case ARGI_BUFFER:
            /*
             * Need an operand of type ACPI_TYPE_BUFFER,
             * But we can implicitly convert from a STRING or INTEGER
             * Aka - "Implicit Source Operand Conversion"
             */
            Status = AcpiExConvertToBuffer (ObjDesc, StackPtr);
            if (ACPI_FAILURE (Status))
            {
                if (Status == AE_TYPE)
                {
                    ACPI_ERROR ((AE_INFO,
                        "Needed [Integer/String/Buffer], found [%s] %p",
                        AcpiUtGetObjectTypeName (ObjDesc), ObjDesc));

                    return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
                }

                return_ACPI_STATUS (Status);
            }

            if (ObjDesc != *StackPtr)
            {
                AcpiUtRemoveReference (ObjDesc);
            }
            goto NextOperand;

        case ARGI_STRING:
            /*
             * Need an operand of type ACPI_TYPE_STRING,
             * But we can implicitly convert from a BUFFER or INTEGER
             * Aka - "Implicit Source Operand Conversion"
             */
            Status = AcpiExConvertToString (
                ObjDesc, StackPtr, ACPI_IMPLICIT_CONVERT_HEX);
            if (ACPI_FAILURE (Status))
            {
                if (Status == AE_TYPE)
                {
                    ACPI_ERROR ((AE_INFO,
                        "Needed [Integer/String/Buffer], found [%s] %p",
                        AcpiUtGetObjectTypeName (ObjDesc), ObjDesc));

                    return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
                }

                return_ACPI_STATUS (Status);
            }

            if (ObjDesc != *StackPtr)
            {
                AcpiUtRemoveReference (ObjDesc);
            }
            goto NextOperand;

        case ARGI_COMPUTEDATA:

            /* Need an operand of type INTEGER, STRING or BUFFER */

            switch (ObjDesc->Common.Type)
            {
            case ACPI_TYPE_INTEGER:
            case ACPI_TYPE_STRING:
            case ACPI_TYPE_BUFFER:

                /* Valid operand */
               break;

            default:
                ACPI_ERROR ((AE_INFO,
                    "Needed [Integer/String/Buffer], found [%s] %p",
                    AcpiUtGetObjectTypeName (ObjDesc), ObjDesc));

                return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
            }
            goto NextOperand;

        case ARGI_BUFFER_OR_STRING:

            /* Need an operand of type STRING or BUFFER */

            switch (ObjDesc->Common.Type)
            {
            case ACPI_TYPE_STRING:
            case ACPI_TYPE_BUFFER:

                /* Valid operand */
               break;

            case ACPI_TYPE_INTEGER:

                /* Highest priority conversion is to type Buffer */

                Status = AcpiExConvertToBuffer (ObjDesc, StackPtr);
                if (ACPI_FAILURE (Status))
                {
                    return_ACPI_STATUS (Status);
                }

                if (ObjDesc != *StackPtr)
                {
                    AcpiUtRemoveReference (ObjDesc);
                }
                break;

            default:
                ACPI_ERROR ((AE_INFO,
                    "Needed [Integer/String/Buffer], found [%s] %p",
                    AcpiUtGetObjectTypeName (ObjDesc), ObjDesc));

                return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
            }
            goto NextOperand;

        case ARGI_DATAOBJECT:
            /*
             * ARGI_DATAOBJECT is only used by the SizeOf operator.
             * Need a buffer, string, package, or RefOf reference.
             *
             * The only reference allowed here is a direct reference to
             * a namespace node.
             */
            switch (ObjDesc->Common.Type)
            {
            case ACPI_TYPE_PACKAGE:
            case ACPI_TYPE_STRING:
            case ACPI_TYPE_BUFFER:
            case ACPI_TYPE_LOCAL_REFERENCE:

                /* Valid operand */
                break;

            default:

                ACPI_ERROR ((AE_INFO,
                    "Needed [Buffer/String/Package/Reference], found [%s] %p",
                    AcpiUtGetObjectTypeName (ObjDesc), ObjDesc));

                return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
            }
            goto NextOperand;

        case ARGI_COMPLEXOBJ:

            /* Need a buffer or package or (ACPI 2.0) String */

            switch (ObjDesc->Common.Type)
            {
            case ACPI_TYPE_PACKAGE:
            case ACPI_TYPE_STRING:
            case ACPI_TYPE_BUFFER:

                /* Valid operand */
                break;

            default:

                ACPI_ERROR ((AE_INFO,
                    "Needed [Buffer/String/Package], found [%s] %p",
                    AcpiUtGetObjectTypeName (ObjDesc), ObjDesc));

                return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
            }
            goto NextOperand;

        case ARGI_REGION_OR_BUFFER: /* Used by Load() only */

            /*
             * Need an operand of type REGION or a BUFFER
             * (which could be a resolved region field)
             */
            switch (ObjDesc->Common.Type)
            {
            case ACPI_TYPE_BUFFER:
            case ACPI_TYPE_REGION:

                /* Valid operand */
                break;

            default:

                ACPI_ERROR ((AE_INFO,
                    "Needed [Region/Buffer], found [%s] %p",
                    AcpiUtGetObjectTypeName (ObjDesc), ObjDesc));

                return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
            }
            goto NextOperand;

        case ARGI_DATAREFOBJ:

            /* Used by the Store() operator only */

            switch (ObjDesc->Common.Type)
            {
            case ACPI_TYPE_INTEGER:
            case ACPI_TYPE_PACKAGE:
            case ACPI_TYPE_STRING:
            case ACPI_TYPE_BUFFER:
            case ACPI_TYPE_BUFFER_FIELD:
            case ACPI_TYPE_LOCAL_REFERENCE:
            case ACPI_TYPE_LOCAL_REGION_FIELD:
            case ACPI_TYPE_LOCAL_BANK_FIELD:
            case ACPI_TYPE_LOCAL_INDEX_FIELD:
            case ACPI_TYPE_DDB_HANDLE:

                /* Valid operand */
                break;

            default:

                if (AcpiGbl_EnableInterpreterSlack)
                {
                    /*
                     * Enable original behavior of Store(), allowing any
                     * and all objects as the source operand. The ACPI
                     * spec does not allow this, however.
                     */
                    break;
                }

                if (TargetOp == AML_DEBUG_OP)
                {
                    /* Allow store of any object to the Debug object */

                    break;
                }

                ACPI_ERROR ((AE_INFO,
                    "Needed Integer/Buffer/String/Package/Ref/Ddb]"
                    ", found [%s] %p",
                    AcpiUtGetObjectTypeName (ObjDesc), ObjDesc));

                return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
            }
            goto NextOperand;

        default:

            /* Unknown type */

            ACPI_ERROR ((AE_INFO,
                "Internal - Unknown ARGI (required operand) type 0x%X",
                ThisArgType));

            return_ACPI_STATUS (AE_BAD_PARAMETER);
        }

        /*
         * Make sure that the original object was resolved to the
         * required object type (Simple cases only).
         */
        Status = AcpiExCheckObjectType (
            TypeNeeded, (*StackPtr)->Common.Type, *StackPtr);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

NextOperand:
        /*
         * If more operands needed, decrement StackPtr to point
         * to next operand on stack
         */
        if (GET_CURRENT_ARG_TYPE (ArgTypes))
        {
            StackPtr--;
        }
    }

    ACPI_DUMP_OPERANDS (WalkState->Operands,
        AcpiPsGetOpcodeName (Opcode), WalkState->NumOperands);

    return_ACPI_STATUS (Status);
}
