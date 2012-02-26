/******************************************************************************
 *
 * Module Name: dsobject - Dispatcher object management routines
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2011, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#define __DSOBJECT_C__

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

    Status = AcpiDsInitObjectFromOp (WalkState, Op, Op->Common.AmlOpcode,
                &ObjDesc);
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
     * individual bytes or a string initializer.  In either case, a
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
        ObjDesc->Buffer.Pointer = ACPI_ALLOCATE_ZEROED (
                                        ObjDesc->Buffer.Length);
        if (!ObjDesc->Buffer.Pointer)
        {
            AcpiUtDeleteObjectDesc (ObjDesc);
            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        /* Initialize buffer from the ByteList (if present) */

        if (ByteList)
        {
            ACPI_MEMCPY (ObjDesc->Buffer.Pointer, ByteList->Named.Data,
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
                Status = AcpiDsBuildInternalObject (WalkState, Arg,
                            &ObjDesc->Package.Elements[i]);
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
            Status = AcpiDsBuildInternalObject (WalkState, Arg,
                        &ObjDesc->Package.Elements[i]);
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

        ACPI_INFO ((AE_INFO,
            "Actual Package length (%u) is larger than NumElements field (%u), truncated\n",
            i, ElementCount));
    }
    else if (i < ElementCount)
    {
        /*
         * Arg list (elements) was exhausted, but we did not reach NumElements count.
         * Note: this is not an error, the package is padded out with NULLs.
         */
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "Package List length (%u) smaller than NumElements count (%u), padded with null elements\n",
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
     * parts of the table, we can arrive here twice.  Only init
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

    Status = AcpiDsBuildInternalObject (WalkState, Op->Common.Value.Arg,
                &ObjDesc);
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
 *              associated arguments.  The namespace object is a more compact
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
        ObjDesc->Buffer.Node      = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE,
                                        WalkState->Operands[0]);
        ObjDesc->Buffer.AmlStart  = Op->Named.Data;
        ObjDesc->Buffer.AmlLength = Op->Named.Length;
        break;


    case ACPI_TYPE_PACKAGE:

        /*
         * Defer evaluation of Package TermArg operand
         */
        ObjDesc->Package.Node      = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE,
                                        WalkState->Operands[0]);
        ObjDesc->Package.AmlStart  = Op->Named.Data;
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
                AcpiExTruncateFor32bitTable (ObjDesc);
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
            AcpiExTruncateFor32bitTable (ObjDesc);
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
        ObjDesc->String.Length = (UINT32) ACPI_STRLEN (Op->Common.Value.String);

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


