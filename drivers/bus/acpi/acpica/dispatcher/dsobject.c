/******************************************************************************
 *
 * Module Name: dsobject - Dispatcher object management routines
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
#include "acparser.h"
#include "amlcode.h"
#include "acdispat.h"
#include "acnamesp.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_DISPATCHER
        ACPI_MODULE_NAME    ("dsobject")


#ifndef ACPI_NO_METHOD_EXECUTION
/*******************************************************************************
 *
 * FUNCTION:    AcpiDsBuildInternalObject
 *
 * PARAMETERS:  WalkState       - Current walk state
 *              Op              - Parser object to be translated
 *              ObjDescPtr      - Where the ACPI internal object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Translate a parser Op object to the equivalent namespace object
 *              Simple objects are any objects other than a package object!
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsBuildInternalObject (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    ACPI_OPERAND_OBJECT     **ObjDescPtr)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (DsBuildInternalObject);


    *ObjDescPtr = NULL;
    if (Op->Common.AmlOpcode == AML_INT_NAMEPATH_OP)
    {
        /*
         * This is a named object reference. If this name was
         * previously looked up in the namespace, it was stored in
         * this op. Otherwise, go ahead and look it up now
         */
        if (!Op->Common.Node)
        {
            /* Check if we are resolving a named reference within a package */

            if ((Op->Common.Parent->Common.AmlOpcode == AML_PACKAGE_OP) ||
                (Op->Common.Parent->Common.AmlOpcode == AML_VARIABLE_PACKAGE_OP))
            {
                /*
                 * We won't resolve package elements here, we will do this
                 * after all ACPI tables are loaded into the namespace. This
                 * behavior supports both forward references to named objects
                 * and external references to objects in other tables.
                 */
                goto CreateNewObject;
            }
            else
            {
                Status = AcpiNsLookup (WalkState->ScopeInfo,
                    Op->Common.Value.String,
                    ACPI_TYPE_ANY, ACPI_IMODE_EXECUTE,
                    ACPI_NS_SEARCH_PARENT | ACPI_NS_DONT_OPEN_SCOPE, NULL,
                    ACPI_CAST_INDIRECT_PTR (
                        ACPI_NAMESPACE_NODE, &(Op->Common.Node)));
                if (ACPI_FAILURE (Status))
                {
                    ACPI_ERROR_NAMESPACE (WalkState->ScopeInfo,
                        Op->Common.Value.String, Status);
                    return_ACPI_STATUS (Status);
                }
            }
        }
    }

CreateNewObject:

    /* Create and init a new internal ACPI object */

    ObjDesc = AcpiUtCreateInternalObject (
        (AcpiPsGetOpcodeInfo (Op->Common.AmlOpcode))->ObjectType);
    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    Status = AcpiDsInitObjectFromOp (
        WalkState, Op, Op->Common.AmlOpcode, &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        AcpiUtRemoveReference (ObjDesc);
        return_ACPI_STATUS (Status);
    }

    /*
     * Handling for unresolved package reference elements.
     * These are elements that are namepaths.
     */
    if ((Op->Common.Parent->Common.AmlOpcode == AML_PACKAGE_OP) ||
        (Op->Common.Parent->Common.AmlOpcode == AML_VARIABLE_PACKAGE_OP))
    {
        ObjDesc->Reference.Resolved = TRUE;

        if ((Op->Common.AmlOpcode == AML_INT_NAMEPATH_OP) &&
            !ObjDesc->Reference.Node)
        {
            /*
             * Name was unresolved above.
             * Get the prefix node for later lookup
             */
            ObjDesc->Reference.Node = WalkState->ScopeInfo->Scope.Node;
            ObjDesc->Reference.Aml = Op->Common.Aml;
            ObjDesc->Reference.Resolved = FALSE;
        }
    }

    *ObjDescPtr = ObjDesc;
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsBuildInternalBufferObj
 *
 * PARAMETERS:  WalkState       - Current walk state
 *              Op              - Parser object to be translated
 *              BufferLength    - Length of the buffer
 *              ObjDescPtr      - Where the ACPI internal object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Translate a parser Op package object to the equivalent
 *              namespace object
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsBuildInternalBufferObj (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  BufferLength,
    ACPI_OPERAND_OBJECT     **ObjDescPtr)
{
    ACPI_PARSE_OBJECT       *Arg;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_PARSE_OBJECT       *ByteList;
    UINT32                  ByteListLength = 0;


    ACPI_FUNCTION_TRACE (DsBuildInternalBufferObj);


    /*
     * If we are evaluating a Named buffer object "Name (xxxx, Buffer)".
     * The buffer object already exists (from the NS node), otherwise it must
     * be created.
     */
    ObjDesc = *ObjDescPtr;
    if (!ObjDesc)
    {
        /* Create a new buffer object */

        ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_BUFFER);
        *ObjDescPtr = ObjDesc;
        if (!ObjDesc)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }
    }

    /*
     * Second arg is the buffer data (optional) ByteList can be either
     * individual bytes or a string initializer. In either case, a
     * ByteList appears in the AML.
     */
    Arg = Op->Common.Value.Arg;         /* skip first arg */

    ByteList = Arg->Named.Next;
    if (ByteList)
    {
        if (ByteList->Common.AmlOpcode != AML_INT_BYTELIST_OP)
        {
            ACPI_ERROR ((AE_INFO,
                "Expecting bytelist, found AML opcode 0x%X in op %p",
                ByteList->Common.AmlOpcode, ByteList));

            AcpiUtRemoveReference (ObjDesc);
            return (AE_TYPE);
        }

        ByteListLength = (UINT32) ByteList->Common.Value.Integer;
    }

    /*
     * The buffer length (number of bytes) will be the larger of:
     * 1) The specified buffer length and
     * 2) The length of the initializer byte list
     */
    ObjDesc->Buffer.Length = BufferLength;
    if (ByteListLength > BufferLength)
    {
        ObjDesc->Buffer.Length = ByteListLength;
    }

    /* Allocate the buffer */

    if (ObjDesc->Buffer.Length == 0)
    {
        ObjDesc->Buffer.Pointer = NULL;
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "Buffer defined with zero length in AML, creating\n"));
    }
    else
    {
        ObjDesc->Buffer.Pointer =
            ACPI_ALLOCATE_ZEROED (ObjDesc->Buffer.Length);
        if (!ObjDesc->Buffer.Pointer)
        {
            AcpiUtDeleteObjectDesc (ObjDesc);
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        /* Initialize buffer from the ByteList (if present) */

        if (ByteList)
        {
            memcpy (ObjDesc->Buffer.Pointer, ByteList->Named.Data,
                ByteListLength);
        }
    }

    ObjDesc->Buffer.Flags |= AOPOBJ_DATA_VALID;
    Op->Common.Node = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE, ObjDesc);
    return_ACPI_STATUS (AE_OK);
}

/*******************************************************************************
 *
 * FUNCTION:    AcpiDsCreateNode
 *
 * PARAMETERS:  WalkState       - Current walk state
 *              Node            - NS Node to be initialized
 *              Op              - Parser object to be translated
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create the object to be associated with a namespace node
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsCreateNode (
    ACPI_WALK_STATE         *WalkState,
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;


    ACPI_FUNCTION_TRACE_PTR (DsCreateNode, Op);


    /*
     * Because of the execution pass through the non-control-method
     * parts of the table, we can arrive here twice. Only init
     * the named object node the first time through
     */
    if (AcpiNsGetAttachedObject (Node))
    {
        return_ACPI_STATUS (AE_OK);
    }

    if (!Op->Common.Value.Arg)
    {
        /* No arguments, there is nothing to do */

        return_ACPI_STATUS (AE_OK);
    }

    /* Build an internal object for the argument(s) */

    Status = AcpiDsBuildInternalObject (
        WalkState, Op->Common.Value.Arg, &ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Re-type the object according to its argument */

    Node->Type = ObjDesc->Common.Type;

    /* Attach obj to node */

    Status = AcpiNsAttachObject (Node, ObjDesc, Node->Type);

    /* Remove local reference to the object */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}

#endif /* ACPI_NO_METHOD_EXECUTION */


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsInitObjectFromOp
 *
 * PARAMETERS:  WalkState       - Current walk state
 *              Op              - Parser op used to init the internal object
 *              Opcode          - AML opcode associated with the object
 *              RetObjDesc      - Namespace object to be initialized
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize a namespace object from a parser Op and its
 *              associated arguments. The namespace object is a more compact
 *              representation of the Op and its arguments.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsInitObjectFromOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    UINT16                  Opcode,
    ACPI_OPERAND_OBJECT     **RetObjDesc)
{
    const ACPI_OPCODE_INFO  *OpInfo;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (DsInitObjectFromOp);


    ObjDesc = *RetObjDesc;
    OpInfo = AcpiPsGetOpcodeInfo (Opcode);
    if (OpInfo->Class == AML_CLASS_UNKNOWN)
    {
        /* Unknown opcode */

        return_ACPI_STATUS (AE_TYPE);
    }

    /* Perform per-object initialization */

    switch (ObjDesc->Common.Type)
    {
    case ACPI_TYPE_BUFFER:
        /*
         * Defer evaluation of Buffer TermArg operand
         */
        ObjDesc->Buffer.Node = ACPI_CAST_PTR (
            ACPI_NAMESPACE_NODE, WalkState->Operands[0]);
        ObjDesc->Buffer.AmlStart = Op->Named.Data;
        ObjDesc->Buffer.AmlLength = Op->Named.Length;
        break;

    case ACPI_TYPE_PACKAGE:
        /*
         * Defer evaluation of Package TermArg operand and all
         * package elements. (01/2017): We defer the element
         * resolution to allow forward references from the package
         * in order to provide compatibility with other ACPI
         * implementations.
         */
        ObjDesc->Package.Node = ACPI_CAST_PTR (
            ACPI_NAMESPACE_NODE, WalkState->Operands[0]);

        if (!Op->Named.Data)
        {
            return_ACPI_STATUS (AE_OK);
        }

        ObjDesc->Package.AmlStart = Op->Named.Data;
        ObjDesc->Package.AmlLength = Op->Named.Length;
        break;

    case ACPI_TYPE_INTEGER:

        switch (OpInfo->Type)
        {
        case AML_TYPE_CONSTANT:
            /*
             * Resolve AML Constants here - AND ONLY HERE!
             * All constants are integers.
             * We mark the integer with a flag that indicates that it started
             * life as a constant -- so that stores to constants will perform
             * as expected (noop). ZeroOp is used as a placeholder for optional
             * target operands.
             */
            ObjDesc->Common.Flags = AOPOBJ_AML_CONSTANT;

            switch (Opcode)
            {
            case AML_ZERO_OP:

                ObjDesc->Integer.Value = 0;
                break;

            case AML_ONE_OP:

                ObjDesc->Integer.Value = 1;
                break;

            case AML_ONES_OP:

                ObjDesc->Integer.Value = ACPI_UINT64_MAX;

                /* Truncate value if we are executing from a 32-bit ACPI table */

#ifndef ACPI_NO_METHOD_EXECUTION
                (void) AcpiExTruncateFor32bitTable (ObjDesc);
#endif
                break;

            case AML_REVISION_OP:

                ObjDesc->Integer.Value = ACPI_CA_VERSION;
                break;

            default:

                ACPI_ERROR ((AE_INFO,
                    "Unknown constant opcode 0x%X", Opcode));
                Status = AE_AML_OPERAND_TYPE;
                break;
            }
            break;

        case AML_TYPE_LITERAL:

            ObjDesc->Integer.Value = Op->Common.Value.Integer;

#ifndef ACPI_NO_METHOD_EXECUTION
            if (AcpiExTruncateFor32bitTable (ObjDesc))
            {
                /* Warn if we found a 64-bit constant in a 32-bit table */

                ACPI_WARNING ((AE_INFO,
                    "Truncated 64-bit constant found in 32-bit table: %8.8X%8.8X => %8.8X",
                    ACPI_FORMAT_UINT64 (Op->Common.Value.Integer),
                    (UINT32) ObjDesc->Integer.Value));
            }
#endif
            break;

        default:

            ACPI_ERROR ((AE_INFO, "Unknown Integer type 0x%X",
                OpInfo->Type));
            Status = AE_AML_OPERAND_TYPE;
            break;
        }
        break;

    case ACPI_TYPE_STRING:

        ObjDesc->String.Pointer = Op->Common.Value.String;
        ObjDesc->String.Length = (UINT32) strlen (Op->Common.Value.String);

        /*
         * The string is contained in the ACPI table, don't ever try
         * to delete it
         */
        ObjDesc->Common.Flags |= AOPOBJ_STATIC_POINTER;
        break;

    case ACPI_TYPE_METHOD:
        break;

    case ACPI_TYPE_LOCAL_REFERENCE:

        switch (OpInfo->Type)
        {
        case AML_TYPE_LOCAL_VARIABLE:

            /* Local ID (0-7) is (AML opcode - base AML_FIRST_LOCAL_OP) */

            ObjDesc->Reference.Value = ((UINT32) Opcode) - AML_FIRST_LOCAL_OP;
            ObjDesc->Reference.Class = ACPI_REFCLASS_LOCAL;

#ifndef ACPI_NO_METHOD_EXECUTION
            Status = AcpiDsMethodDataGetNode (ACPI_REFCLASS_LOCAL,
                ObjDesc->Reference.Value, WalkState,
                ACPI_CAST_INDIRECT_PTR (ACPI_NAMESPACE_NODE,
                    &ObjDesc->Reference.Object));
#endif
            break;

        case AML_TYPE_METHOD_ARGUMENT:

            /* Arg ID (0-6) is (AML opcode - base AML_FIRST_ARG_OP) */

            ObjDesc->Reference.Value = ((UINT32) Opcode) - AML_FIRST_ARG_OP;
            ObjDesc->Reference.Class = ACPI_REFCLASS_ARG;

#ifndef ACPI_NO_METHOD_EXECUTION
            Status = AcpiDsMethodDataGetNode (ACPI_REFCLASS_ARG,
                ObjDesc->Reference.Value, WalkState,
                ACPI_CAST_INDIRECT_PTR (ACPI_NAMESPACE_NODE,
                    &ObjDesc->Reference.Object));
#endif
            break;

        default: /* Object name or Debug object */

            switch (Op->Common.AmlOpcode)
            {
            case AML_INT_NAMEPATH_OP:

                /* Node was saved in Op */

                ObjDesc->Reference.Node = Op->Common.Node;
                ObjDesc->Reference.Class = ACPI_REFCLASS_NAME;
                if (Op->Common.Node)
                {
                    ObjDesc->Reference.Object = Op->Common.Node->Object;
                }
                break;

            case AML_DEBUG_OP:

                ObjDesc->Reference.Class = ACPI_REFCLASS_DEBUG;
                break;

            default:

                ACPI_ERROR ((AE_INFO,
                    "Unimplemented reference type for AML opcode: 0x%4.4X", Opcode));
                return_ACPI_STATUS (AE_AML_OPERAND_TYPE);
            }
            break;
        }
        break;

    default:

        ACPI_ERROR ((AE_INFO, "Unimplemented data type: 0x%X",
            ObjDesc->Common.Type));

        Status = AE_AML_OPERAND_TYPE;
        break;
    }

    return_ACPI_STATUS (Status);
}
