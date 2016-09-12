/******************************************************************************
 *
 * Module Name: dsobject - Dispatcher object management routines
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
#include "acparser.h"
#include "amlcode.h"
#include "acdispat.h"
#include "acnamesp.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_DISPATCHER
        ACPI_MODULE_NAME    ("dsobject")

/* Local prototypes */

static ACPI_STATUS
AcpiDsBuildInternalObject (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    ACPI_OPERAND_OBJECT     **ObjDescPtr);


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

static ACPI_STATUS
AcpiDsBuildInternalObject (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    ACPI_OPERAND_OBJECT     **ObjDescPtr)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;
    ACPI_OBJECT_TYPE        Type;


    ACPI_FUNCTION_TRACE (DsBuildInternalObject);


    *ObjDescPtr = NULL;
    if (Op->Common.AmlOpcode == AML_INT_NAMEPATH_OP)
    {
        /*
         * This is a named object reference. If this name was
         * previously looked up in the namespace, it was stored in this op.
         * Otherwise, go ahead and look it up now
         */
        if (!Op->Common.Node)
        {
            Status = AcpiNsLookup (WalkState->ScopeInfo,
                Op->Common.Value.String,
                ACPI_TYPE_ANY, ACPI_IMODE_EXECUTE,
                ACPI_NS_SEARCH_PARENT | ACPI_NS_DONT_OPEN_SCOPE, NULL,
                ACPI_CAST_INDIRECT_PTR (ACPI_NAMESPACE_NODE, &(Op->Common.Node)));
            if (ACPI_FAILURE (Status))
            {
                /* Check if we are resolving a named reference within a package */

                if ((Status == AE_NOT_FOUND) && (AcpiGbl_EnableInterpreterSlack) &&

                    ((Op->Common.Parent->Common.AmlOpcode == AML_PACKAGE_OP) ||
                     (Op->Common.Parent->Common.AmlOpcode == AML_VAR_PACKAGE_OP)))
                {
                    /*
                     * We didn't find the target and we are populating elements
                     * of a package - ignore if slack enabled. Some ASL code
                     * contains dangling invalid references in packages and
                     * expects that no exception will be issued. Leave the
                     * element as a null element. It cannot be used, but it
                     * can be overwritten by subsequent ASL code - this is
                     * typically the case.
                     */
                    ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
                        "Ignoring unresolved reference in package [%4.4s]\n",
                        WalkState->ScopeInfo->Scope.Node->Name.Ascii));

                    return_ACPI_STATUS (AE_OK);
                }
                else
                {
                    ACPI_ERROR_NAMESPACE (Op->Common.Value.String, Status);
                }

                return_ACPI_STATUS (Status);
            }
        }

        /* Special object resolution for elements of a package */

        if ((Op->Common.Parent->Common.AmlOpcode == AML_PACKAGE_OP) ||
            (Op->Common.Parent->Common.AmlOpcode == AML_VAR_PACKAGE_OP))
        {
            /*
             * Attempt to resolve the node to a value before we insert it into
             * the package. If this is a reference to a common data type,
             * resolve it immediately. According to the ACPI spec, package
             * elements can only be "data objects" or method references.
             * Attempt to resolve to an Integer, Buffer, String or Package.
             * If cannot, return the named reference (for things like Devices,
             * Methods, etc.) Buffer Fields and Fields will resolve to simple
             * objects (int/buf/str/pkg).
             *
             * NOTE: References to things like Devices, Methods, Mutexes, etc.
             * will remain as named references. This behavior is not described
             * in the ACPI spec, but it appears to be an oversight.
             */
            ObjDesc = ACPI_CAST_PTR (ACPI_OPERAND_OBJECT, Op->Common.Node);

            Status = AcpiExResolveNodeToValue (
                ACPI_CAST_INDIRECT_PTR (ACPI_NAMESPACE_NODE, &ObjDesc),
                WalkState);
            if (ACPI_FAILURE (Status))
            {
                return_ACPI_STATUS (Status);
            }

            /*
             * Special handling for Alias objects. We need to setup the type
             * and the Op->Common.Node to point to the Alias target. Note,
             * Alias has at most one level of indirection internally.
             */
            Type = Op->Common.Node->Type;
            if (Type == ACPI_TYPE_LOCAL_ALIAS)
            {
                Type = ObjDesc->Common.Type;
                Op->Common.Node = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE,
                    Op->Common.Node->Object);
            }

            switch (Type)
            {
            /*
             * For these types, we need the actual node, not the subobject.
             * However, the subobject did not get an extra reference count above.
             *
             * TBD: should ExResolveNodeToValue be changed to fix this?
             */
            case ACPI_TYPE_DEVICE:
            case ACPI_TYPE_THERMAL:

                AcpiUtAddReference (Op->Common.Node->Object);

                /*lint -fallthrough */
            /*
             * For these types, we need the actual node, not the subobject.
             * The subobject got an extra reference count in ExResolveNodeToValue.
             */
            case ACPI_TYPE_MUTEX:
            case ACPI_TYPE_METHOD:
            case ACPI_TYPE_POWER:
            case ACPI_TYPE_PROCESSOR:
            case ACPI_TYPE_EVENT:
            case ACPI_TYPE_REGION:

                /* We will create a reference object for these types below */
                break;

            default:
                /*
                 * All other types - the node was resolved to an actual
                 * object, we are done.
                 */
                goto Exit;
            }
        }
    }

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

Exit:
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
 * FUNCTION:    AcpiDsBuildInternalPackageObj
 *
 * PARAMETERS:  WalkState       - Current walk state
 *              Op              - Parser object to be translated
 *              ElementCount    - Number of elements in the package - this is
 *                                the NumElements argument to Package()
 *              ObjDescPtr      - Where the ACPI internal object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Translate a parser Op package object to the equivalent
 *              namespace object
 *
 * NOTE: The number of elements in the package will be always be the NumElements
 * count, regardless of the number of elements in the package list. If
 * NumElements is smaller, only that many package list elements are used.
 * if NumElements is larger, the Package object is padded out with
 * objects of type Uninitialized (as per ACPI spec.)
 *
 * Even though the ASL compilers do not allow NumElements to be smaller
 * than the Package list length (for the fixed length package opcode), some
 * BIOS code modifies the AML on the fly to adjust the NumElements, and
 * this code compensates for that. This also provides compatibility with
 * other AML interpreters.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsBuildInternalPackageObj (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  ElementCount,
    ACPI_OPERAND_OBJECT     **ObjDescPtr)
{
    ACPI_PARSE_OBJECT       *Arg;
    ACPI_PARSE_OBJECT       *Parent;
    ACPI_OPERAND_OBJECT     *ObjDesc = NULL;
    ACPI_STATUS             Status = AE_OK;
    UINT32                  i;
    UINT16                  Index;
    UINT16                  ReferenceCount;


    ACPI_FUNCTION_TRACE (DsBuildInternalPackageObj);


    /* Find the parent of a possibly nested package */

    Parent = Op->Common.Parent;
    while ((Parent->Common.AmlOpcode == AML_PACKAGE_OP) ||
           (Parent->Common.AmlOpcode == AML_VAR_PACKAGE_OP))
    {
        Parent = Parent->Common.Parent;
    }

    /*
     * If we are evaluating a Named package object "Name (xxxx, Package)",
     * the package object already exists, otherwise it must be created.
     */
    ObjDesc = *ObjDescPtr;
    if (!ObjDesc)
    {
        ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_PACKAGE);
        *ObjDescPtr = ObjDesc;
        if (!ObjDesc)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        ObjDesc->Package.Node = Parent->Common.Node;
    }

    /*
     * Allocate the element array (array of pointers to the individual
     * objects) based on the NumElements parameter. Add an extra pointer slot
     * so that the list is always null terminated.
     */
    ObjDesc->Package.Elements = ACPI_ALLOCATE_ZEROED (
        ((ACPI_SIZE) ElementCount + 1) * sizeof (void *));

    if (!ObjDesc->Package.Elements)
    {
        AcpiUtDeleteObjectDesc (ObjDesc);
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    ObjDesc->Package.Count = ElementCount;

    /*
     * Initialize the elements of the package, up to the NumElements count.
     * Package is automatically padded with uninitialized (NULL) elements
     * if NumElements is greater than the package list length. Likewise,
     * Package is truncated if NumElements is less than the list length.
     */
    Arg = Op->Common.Value.Arg;
    Arg = Arg->Common.Next;
    for (i = 0; Arg && (i < ElementCount); i++)
    {
        if (Arg->Common.AmlOpcode == AML_INT_RETURN_VALUE_OP)
        {
            if (Arg->Common.Node->Type == ACPI_TYPE_METHOD)
            {
                /*
                 * A method reference "looks" to the parser to be a method
                 * invocation, so we special case it here
                 */
                Arg->Common.AmlOpcode = AML_INT_NAMEPATH_OP;
                Status = AcpiDsBuildInternalObject (
                    WalkState, Arg, &ObjDesc->Package.Elements[i]);
            }
            else
            {
                /* This package element is already built, just get it */

                ObjDesc->Package.Elements[i] =
                    ACPI_CAST_PTR (ACPI_OPERAND_OBJECT, Arg->Common.Node);
            }
        }
        else
        {
            Status = AcpiDsBuildInternalObject (
                WalkState, Arg, &ObjDesc->Package.Elements[i]);
        }

        if (*ObjDescPtr)
        {
            /* Existing package, get existing reference count */

            ReferenceCount = (*ObjDescPtr)->Common.ReferenceCount;
            if (ReferenceCount > 1)
            {
                /* Make new element ref count match original ref count */

                for (Index = 0; Index < (ReferenceCount - 1); Index++)
                {
                    AcpiUtAddReference ((ObjDesc->Package.Elements[i]));
                }
            }
        }

        Arg = Arg->Common.Next;
    }

    /* Check for match between NumElements and actual length of PackageList */

    if (Arg)
    {
        /*
         * NumElements was exhausted, but there are remaining elements in the
         * PackageList. Truncate the package to NumElements.
         *
         * Note: technically, this is an error, from ACPI spec: "It is an error
         * for NumElements to be less than the number of elements in the
         * PackageList". However, we just print a message and
         * no exception is returned. This provides Windows compatibility. Some
         * BIOSs will alter the NumElements on the fly, creating this type
         * of ill-formed package object.
         */
        while (Arg)
        {
            /*
             * We must delete any package elements that were created earlier
             * and are not going to be used because of the package truncation.
             */
            if (Arg->Common.Node)
            {
                AcpiUtRemoveReference (
                    ACPI_CAST_PTR (ACPI_OPERAND_OBJECT, Arg->Common.Node));
                Arg->Common.Node = NULL;
            }

            /* Find out how many elements there really are */

            i++;
            Arg = Arg->Common.Next;
        }

        ACPI_INFO ((
            "Actual Package length (%u) is larger than "
            "NumElements field (%u), truncated",
            i, ElementCount));
    }
    else if (i < ElementCount)
    {
        /*
         * Arg list (elements) was exhausted, but we did not reach NumElements count.
         * Note: this is not an error, the package is padded out with NULLs.
         */
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "Package List length (%u) smaller than NumElements "
            "count (%u), padded with null elements\n",
            i, ElementCount));
    }

    ObjDesc->Package.Flags |= AOPOBJ_DATA_VALID;
    Op->Common.Node = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE, ObjDesc);
    return_ACPI_STATUS (Status);
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
         * Defer evaluation of Package TermArg operand
         */
        ObjDesc->Package.Node = ACPI_CAST_PTR (
            ACPI_NAMESPACE_NODE, WalkState->Operands[0]);
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

            /* Local ID (0-7) is (AML opcode - base AML_LOCAL_OP) */

            ObjDesc->Reference.Value = ((UINT32) Opcode) - AML_LOCAL_OP;
            ObjDesc->Reference.Class = ACPI_REFCLASS_LOCAL;

#ifndef ACPI_NO_METHOD_EXECUTION
            Status = AcpiDsMethodDataGetNode (ACPI_REFCLASS_LOCAL,
                ObjDesc->Reference.Value, WalkState,
                ACPI_CAST_INDIRECT_PTR (ACPI_NAMESPACE_NODE,
                    &ObjDesc->Reference.Object));
#endif
            break;

        case AML_TYPE_METHOD_ARGUMENT:

            /* Arg ID (0-6) is (AML opcode - base AML_ARG_OP) */

            ObjDesc->Reference.Value = ((UINT32) Opcode) - AML_ARG_OP;
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
                ObjDesc->Reference.Object = Op->Common.Node->Object;
                ObjDesc->Reference.Class = ACPI_REFCLASS_NAME;
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
