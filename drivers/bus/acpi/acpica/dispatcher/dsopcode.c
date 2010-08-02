/******************************************************************************
 *
 * Module Name: dsopcode - Dispatcher Op Region support and handling of
 *                         "control" opcodes
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
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

#define __DSOPCODE_C__

#include "acpi.h"
#include "accommon.h"
#include "acparser.h"
#include "amlcode.h"
#include "acdispat.h"
#include "acinterp.h"
#include "acnamesp.h"
#include "acevents.h"
#include "actables.h"

#define _COMPONENT          ACPI_DISPATCHER
        ACPI_MODULE_NAME    ("dsopcode")

/* Local prototypes */

static ACPI_STATUS
AcpiDsExecuteArguments (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_NAMESPACE_NODE     *ScopeNode,
    UINT32                  AmlLength,
    UINT8                   *AmlStart);

static ACPI_STATUS
AcpiDsInitBufferField (
    UINT16                  AmlOpcode,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     *BufferDesc,
    ACPI_OPERAND_OBJECT     *OffsetDesc,
    ACPI_OPERAND_OBJECT     *LengthDesc,
    ACPI_OPERAND_OBJECT     *ResultDesc);


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsExecuteArguments
 *
 * PARAMETERS:  Node                - Object NS node
 *              ScopeNode           - Parent NS node
 *              AmlLength           - Length of executable AML
 *              AmlStart            - Pointer to the AML
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Late (deferred) execution of region or field arguments
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiDsExecuteArguments (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_NAMESPACE_NODE     *ScopeNode,
    UINT32                  AmlLength,
    UINT8                   *AmlStart)
{
    ACPI_STATUS             Status;
    ACPI_PARSE_OBJECT       *Op;
    ACPI_WALK_STATE         *WalkState;


    ACPI_FUNCTION_TRACE (DsExecuteArguments);


    /*
     * Allocate a new parser op to be the root of the parsed tree
     */
    Op = AcpiPsAllocOp (AML_INT_EVAL_SUBTREE_OP);
    if (!Op)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Save the Node for use in AcpiPsParseAml */

    Op->Common.Node = ScopeNode;

    /* Create and initialize a new parser state */

    WalkState = AcpiDsCreateWalkState (0, NULL, NULL, NULL);
    if (!WalkState)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    Status = AcpiDsInitAmlWalk (WalkState, Op, NULL, AmlStart,
                    AmlLength, NULL, ACPI_IMODE_LOAD_PASS1);
    if (ACPI_FAILURE (Status))
    {
        AcpiDsDeleteWalkState (WalkState);
        goto Cleanup;
    }

    /* Mark this parse as a deferred opcode */

    WalkState->ParseFlags = ACPI_PARSE_DEFERRED_OP;
    WalkState->DeferredNode = Node;

    /* Pass1: Parse the entire declaration */

    Status = AcpiPsParseAml (WalkState);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    /* Get and init the Op created above */

    Op->Common.Node = Node;
    AcpiPsDeleteParseTree (Op);

    /* Evaluate the deferred arguments */

    Op = AcpiPsAllocOp (AML_INT_EVAL_SUBTREE_OP);
    if (!Op)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    Op->Common.Node = ScopeNode;

    /* Create and initialize a new parser state */

    WalkState = AcpiDsCreateWalkState (0, NULL, NULL, NULL);
    if (!WalkState)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /* Execute the opcode and arguments */

    Status = AcpiDsInitAmlWalk (WalkState, Op, NULL, AmlStart,
                    AmlLength, NULL, ACPI_IMODE_EXECUTE);
    if (ACPI_FAILURE (Status))
    {
        AcpiDsDeleteWalkState (WalkState);
        goto Cleanup;
    }

    /* Mark this execution as a deferred opcode */

    WalkState->DeferredNode = Node;
    Status = AcpiPsParseAml (WalkState);

Cleanup:
    AcpiPsDeleteParseTree (Op);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsGetBufferFieldArguments
 *
 * PARAMETERS:  ObjDesc         - A valid BufferField object
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Get BufferField Buffer and Index.  This implements the late
 *              evaluation of these field attributes.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsGetBufferFieldArguments (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_OPERAND_OBJECT     *ExtraDesc;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_PTR (DsGetBufferFieldArguments, ObjDesc);


    if (ObjDesc->Common.Flags & AOPOBJ_DATA_VALID)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Get the AML pointer (method object) and BufferField node */

    ExtraDesc = AcpiNsGetSecondaryObject (ObjDesc);
    Node = ObjDesc->BufferField.Node;

    ACPI_DEBUG_EXEC(AcpiUtDisplayInitPathname (ACPI_TYPE_BUFFER_FIELD, Node, NULL));
    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "[%4.4s] BufferField Arg Init\n",
        AcpiUtGetNodeName (Node)));

    /* Execute the AML code for the TermArg arguments */

    Status = AcpiDsExecuteArguments (Node, AcpiNsGetParentNode (Node),
                ExtraDesc->Extra.AmlLength, ExtraDesc->Extra.AmlStart);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsGetBankFieldArguments
 *
 * PARAMETERS:  ObjDesc         - A valid BankField object
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Get BankField BankValue.  This implements the late
 *              evaluation of these field attributes.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsGetBankFieldArguments (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_OPERAND_OBJECT     *ExtraDesc;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_PTR (DsGetBankFieldArguments, ObjDesc);


    if (ObjDesc->Common.Flags & AOPOBJ_DATA_VALID)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Get the AML pointer (method object) and BankField node */

    ExtraDesc = AcpiNsGetSecondaryObject (ObjDesc);
    Node = ObjDesc->BankField.Node;

    ACPI_DEBUG_EXEC(AcpiUtDisplayInitPathname (ACPI_TYPE_LOCAL_BANK_FIELD, Node, NULL));
    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "[%4.4s] BankField Arg Init\n",
        AcpiUtGetNodeName (Node)));

    /* Execute the AML code for the TermArg arguments */

    Status = AcpiDsExecuteArguments (Node, AcpiNsGetParentNode (Node),
                ExtraDesc->Extra.AmlLength, ExtraDesc->Extra.AmlStart);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsGetBufferArguments
 *
 * PARAMETERS:  ObjDesc         - A valid Buffer object
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Get Buffer length and initializer byte list.  This implements
 *              the late evaluation of these attributes.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsGetBufferArguments (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_PTR (DsGetBufferArguments, ObjDesc);


    if (ObjDesc->Common.Flags & AOPOBJ_DATA_VALID)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Get the Buffer node */

    Node = ObjDesc->Buffer.Node;
    if (!Node)
    {
        ACPI_ERROR ((AE_INFO,
            "No pointer back to NS node in buffer obj %p", ObjDesc));
        return_ACPI_STATUS (AE_AML_INTERNAL);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Buffer Arg Init\n"));

    /* Execute the AML code for the TermArg arguments */

    Status = AcpiDsExecuteArguments (Node, Node,
                ObjDesc->Buffer.AmlLength, ObjDesc->Buffer.AmlStart);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsGetPackageArguments
 *
 * PARAMETERS:  ObjDesc         - A valid Package object
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Get Package length and initializer byte list.  This implements
 *              the late evaluation of these attributes.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsGetPackageArguments (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_PTR (DsGetPackageArguments, ObjDesc);


    if (ObjDesc->Common.Flags & AOPOBJ_DATA_VALID)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Get the Package node */

    Node = ObjDesc->Package.Node;
    if (!Node)
    {
        ACPI_ERROR ((AE_INFO,
            "No pointer back to NS node in package %p", ObjDesc));
        return_ACPI_STATUS (AE_AML_INTERNAL);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Package Arg Init\n"));

    /* Execute the AML code for the TermArg arguments */

    Status = AcpiDsExecuteArguments (Node, Node,
                ObjDesc->Package.AmlLength, ObjDesc->Package.AmlStart);
    return_ACPI_STATUS (Status);
}


/*****************************************************************************
 *
 * FUNCTION:    AcpiDsGetRegionArguments
 *
 * PARAMETERS:  ObjDesc         - A valid region object
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Get region address and length.  This implements the late
 *              evaluation of these region attributes.
 *
 ****************************************************************************/

ACPI_STATUS
AcpiDsGetRegionArguments (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ExtraDesc;


    ACPI_FUNCTION_TRACE_PTR (DsGetRegionArguments, ObjDesc);


    if (ObjDesc->Region.Flags & AOPOBJ_DATA_VALID)
    {
        return_ACPI_STATUS (AE_OK);
    }

    ExtraDesc = AcpiNsGetSecondaryObject (ObjDesc);
    if (!ExtraDesc)
    {
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    /* Get the Region node */

    Node = ObjDesc->Region.Node;

    ACPI_DEBUG_EXEC (AcpiUtDisplayInitPathname (ACPI_TYPE_REGION, Node, NULL));

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "[%4.4s] OpRegion Arg Init at AML %p\n",
        AcpiUtGetNodeName (Node), ExtraDesc->Extra.AmlStart));

    /* Execute the argument AML */

    Status = AcpiDsExecuteArguments (Node, AcpiNsGetParentNode (Node),
                ExtraDesc->Extra.AmlLength, ExtraDesc->Extra.AmlStart);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsInitializeRegion
 *
 * PARAMETERS:  ObjHandle       - Region namespace node
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Front end to EvInitializeRegion
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsInitializeRegion (
    ACPI_HANDLE             ObjHandle)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;


    ObjDesc = AcpiNsGetAttachedObject (ObjHandle);

    /* Namespace is NOT locked */

    Status = AcpiEvInitializeRegion (ObjDesc, FALSE);
    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsInitBufferField
 *
 * PARAMETERS:  AmlOpcode       - CreateXxxField
 *              ObjDesc         - BufferField object
 *              BufferDesc      - Host Buffer
 *              OffsetDesc      - Offset into buffer
 *              LengthDesc      - Length of field (CREATE_FIELD_OP only)
 *              ResultDesc      - Where to store the result
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform actual initialization of a buffer field
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiDsInitBufferField (
    UINT16                  AmlOpcode,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     *BufferDesc,
    ACPI_OPERAND_OBJECT     *OffsetDesc,
    ACPI_OPERAND_OBJECT     *LengthDesc,
    ACPI_OPERAND_OBJECT     *ResultDesc)
{
    UINT32                  Offset;
    UINT32                  BitOffset;
    UINT32                  BitCount;
    UINT8                   FieldFlags;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_PTR (DsInitBufferField, ObjDesc);


    /* Host object must be a Buffer */

    if (BufferDesc->Common.Type != ACPI_TYPE_BUFFER)
    {
        ACPI_ERROR ((AE_INFO,
            "Target of Create Field is not a Buffer object - %s",
            AcpiUtGetObjectTypeName (BufferDesc)));

        Status = AE_AML_OPERAND_TYPE;
        goto Cleanup;
    }

    /*
     * The last parameter to all of these opcodes (ResultDesc) started
     * out as a NameString, and should therefore now be a NS node
     * after resolution in AcpiExResolveOperands().
     */
    if (ACPI_GET_DESCRIPTOR_TYPE (ResultDesc) != ACPI_DESC_TYPE_NAMED)
    {
        ACPI_ERROR ((AE_INFO,
            "(%s) destination not a NS Node [%s]",
            AcpiPsGetOpcodeName (AmlOpcode),
            AcpiUtGetDescriptorName (ResultDesc)));

        Status = AE_AML_OPERAND_TYPE;
        goto Cleanup;
    }

    Offset = (UINT32) OffsetDesc->Integer.Value;

    /*
     * Setup the Bit offsets and counts, according to the opcode
     */
    switch (AmlOpcode)
    {
    case AML_CREATE_FIELD_OP:

        /* Offset is in bits, count is in bits */

        FieldFlags = AML_FIELD_ACCESS_BYTE;
        BitOffset  = Offset;
        BitCount   = (UINT32) LengthDesc->Integer.Value;

        /* Must have a valid (>0) bit count */

        if (BitCount == 0)
        {
            ACPI_ERROR ((AE_INFO,
                "Attempt to CreateField of length zero"));
            Status = AE_AML_OPERAND_VALUE;
            goto Cleanup;
        }
        break;

    case AML_CREATE_BIT_FIELD_OP:

        /* Offset is in bits, Field is one bit */

        BitOffset  = Offset;
        BitCount   = 1;
        FieldFlags = AML_FIELD_ACCESS_BYTE;
        break;

    case AML_CREATE_BYTE_FIELD_OP:

        /* Offset is in bytes, field is one byte */

        BitOffset  = 8 * Offset;
        BitCount   = 8;
        FieldFlags = AML_FIELD_ACCESS_BYTE;
        break;

    case AML_CREATE_WORD_FIELD_OP:

        /* Offset is in bytes, field is one word */

        BitOffset  = 8 * Offset;
        BitCount   = 16;
        FieldFlags = AML_FIELD_ACCESS_WORD;
        break;

    case AML_CREATE_DWORD_FIELD_OP:

        /* Offset is in bytes, field is one dword */

        BitOffset  = 8 * Offset;
        BitCount   = 32;
        FieldFlags = AML_FIELD_ACCESS_DWORD;
        break;

    case AML_CREATE_QWORD_FIELD_OP:

        /* Offset is in bytes, field is one qword */

        BitOffset  = 8 * Offset;
        BitCount   = 64;
        FieldFlags = AML_FIELD_ACCESS_QWORD;
        break;

    default:

        ACPI_ERROR ((AE_INFO,
            "Unknown field creation opcode %02x",
            AmlOpcode));
        Status = AE_AML_BAD_OPCODE;
        goto Cleanup;
    }

    /* Entire field must fit within the current length of the buffer */

    if ((BitOffset + BitCount) >
        (8 * (UINT32) BufferDesc->Buffer.Length))
    {
        ACPI_ERROR ((AE_INFO,
            "Field [%4.4s] at %d exceeds Buffer [%4.4s] size %d (bits)",
            AcpiUtGetNodeName (ResultDesc),
            BitOffset + BitCount,
            AcpiUtGetNodeName (BufferDesc->Buffer.Node),
            8 * (UINT32) BufferDesc->Buffer.Length));
        Status = AE_AML_BUFFER_LIMIT;
        goto Cleanup;
    }

    /*
     * Initialize areas of the field object that are common to all fields
     * For FieldFlags, use LOCK_RULE = 0 (NO_LOCK),
     * UPDATE_RULE = 0 (UPDATE_PRESERVE)
     */
    Status = AcpiExPrepCommonFieldObject (ObjDesc, FieldFlags, 0,
                                            BitOffset, BitCount);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    ObjDesc->BufferField.BufferObj = BufferDesc;

    /* Reference count for BufferDesc inherits ObjDesc count */

    BufferDesc->Common.ReferenceCount = (UINT16)
        (BufferDesc->Common.ReferenceCount + ObjDesc->Common.ReferenceCount);


Cleanup:

    /* Always delete the operands */

    AcpiUtRemoveReference (OffsetDesc);
    AcpiUtRemoveReference (BufferDesc);

    if (AmlOpcode == AML_CREATE_FIELD_OP)
    {
        AcpiUtRemoveReference (LengthDesc);
    }

    /* On failure, delete the result descriptor */

    if (ACPI_FAILURE (Status))
    {
        AcpiUtRemoveReference (ResultDesc);     /* Result descriptor */
    }
    else
    {
        /* Now the address and length are valid for this BufferField */

        ObjDesc->BufferField.Flags |= AOPOBJ_DATA_VALID;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsEvalBufferFieldOperands
 *
 * PARAMETERS:  WalkState       - Current walk
 *              Op              - A valid BufferField Op object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get BufferField Buffer and Index
 *              Called from AcpiDsExecEndOp during BufferField parse tree walk
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsEvalBufferFieldOperands (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_PARSE_OBJECT       *NextOp;


    ACPI_FUNCTION_TRACE_PTR (DsEvalBufferFieldOperands, Op);


    /*
     * This is where we evaluate the address and length fields of the
     * CreateXxxField declaration
     */
    Node =  Op->Common.Node;

    /* NextOp points to the op that holds the Buffer */

    NextOp = Op->Common.Value.Arg;

    /* Evaluate/create the address and length operands */

    Status = AcpiDsCreateOperands (WalkState, NextOp);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    /* Resolve the operands */

    Status = AcpiExResolveOperands (Op->Common.AmlOpcode,
                    ACPI_WALK_OPERANDS, WalkState);
    if (ACPI_FAILURE (Status))
    {
        ACPI_ERROR ((AE_INFO, "(%s) bad operand(s) (%X)",
            AcpiPsGetOpcodeName (Op->Common.AmlOpcode), Status));

        return_ACPI_STATUS (Status);
    }

    /* Initialize the Buffer Field */

    if (Op->Common.AmlOpcode == AML_CREATE_FIELD_OP)
    {
        /* NOTE: Slightly different operands for this opcode */

        Status = AcpiDsInitBufferField (Op->Common.AmlOpcode, ObjDesc,
                    WalkState->Operands[0], WalkState->Operands[1],
                    WalkState->Operands[2], WalkState->Operands[3]);
    }
    else
    {
        /* All other, CreateXxxField opcodes */

        Status = AcpiDsInitBufferField (Op->Common.AmlOpcode, ObjDesc,
                    WalkState->Operands[0], WalkState->Operands[1],
                                      NULL, WalkState->Operands[2]);
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsEvalRegionOperands
 *
 * PARAMETERS:  WalkState       - Current walk
 *              Op              - A valid region Op object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get region address and length
 *              Called from AcpiDsExecEndOp during OpRegion parse tree walk
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsEvalRegionOperands (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *OperandDesc;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_PARSE_OBJECT       *NextOp;


    ACPI_FUNCTION_TRACE_PTR (DsEvalRegionOperands, Op);


    /*
     * This is where we evaluate the address and length fields of the
     * OpRegion declaration
     */
    Node =  Op->Common.Node;

    /* NextOp points to the op that holds the SpaceID */

    NextOp = Op->Common.Value.Arg;

    /* NextOp points to address op */

    NextOp = NextOp->Common.Next;

    /* Evaluate/create the address and length operands */

    Status = AcpiDsCreateOperands (WalkState, NextOp);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Resolve the length and address operands to numbers */

    Status = AcpiExResolveOperands (Op->Common.AmlOpcode,
                ACPI_WALK_OPERANDS, WalkState);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    /*
     * Get the length operand and save it
     * (at Top of stack)
     */
    OperandDesc = WalkState->Operands[WalkState->NumOperands - 1];

    ObjDesc->Region.Length = (UINT32) OperandDesc->Integer.Value;
    AcpiUtRemoveReference (OperandDesc);

    /*
     * Get the address and save it
     * (at top of stack - 1)
     */
    OperandDesc = WalkState->Operands[WalkState->NumOperands - 2];

    ObjDesc->Region.Address = (ACPI_PHYSICAL_ADDRESS)
                                OperandDesc->Integer.Value;
    AcpiUtRemoveReference (OperandDesc);

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "RgnObj %p Addr %8.8X%8.8X Len %X\n",
        ObjDesc,
        ACPI_FORMAT_NATIVE_UINT (ObjDesc->Region.Address),
        ObjDesc->Region.Length));

    /* Now the address and length are valid for this opregion */

    ObjDesc->Region.Flags |= AOPOBJ_DATA_VALID;

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsEvalTableRegionOperands
 *
 * PARAMETERS:  WalkState       - Current walk
 *              Op              - A valid region Op object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get region address and length
 *              Called from AcpiDsExecEndOp during DataTableRegion parse tree walk
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsEvalTableRegionOperands (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     **Operand;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_PARSE_OBJECT       *NextOp;
    UINT32                  TableIndex;
    ACPI_TABLE_HEADER       *Table;


    ACPI_FUNCTION_TRACE_PTR (DsEvalTableRegionOperands, Op);


    /*
     * This is where we evaluate the SignatureString and OemIDString
     * and OemTableIDString of the DataTableRegion declaration
     */
    Node =  Op->Common.Node;

    /* NextOp points to SignatureString op */

    NextOp = Op->Common.Value.Arg;

    /*
     * Evaluate/create the SignatureString and OemIDString
     * and OemTableIDString operands
     */
    Status = AcpiDsCreateOperands (WalkState, NextOp);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Resolve the SignatureString and OemIDString
     * and OemTableIDString operands
     */
    Status = AcpiExResolveOperands (Op->Common.AmlOpcode,
                ACPI_WALK_OPERANDS, WalkState);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Operand = &WalkState->Operands[0];

    /* Find the ACPI table */

    Status = AcpiTbFindTable (Operand[0]->String.Pointer,
                Operand[1]->String.Pointer, Operand[2]->String.Pointer,
                &TableIndex);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    AcpiUtRemoveReference (Operand[0]);
    AcpiUtRemoveReference (Operand[1]);
    AcpiUtRemoveReference (Operand[2]);

    Status = AcpiGetTableByIndex (TableIndex, &Table);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    ObjDesc->Region.Address = (ACPI_PHYSICAL_ADDRESS) ACPI_TO_INTEGER (Table);
    ObjDesc->Region.Length = Table->Length;

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "RgnObj %p Addr %8.8X%8.8X Len %X\n",
        ObjDesc,
        ACPI_FORMAT_NATIVE_UINT (ObjDesc->Region.Address),
        ObjDesc->Region.Length));

    /* Now the address and length are valid for this opregion */

    ObjDesc->Region.Flags |= AOPOBJ_DATA_VALID;

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsEvalDataObjectOperands
 *
 * PARAMETERS:  WalkState       - Current walk
 *              Op              - A valid DataObject Op object
 *              ObjDesc         - DataObject
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get the operands and complete the following data object types:
 *              Buffer, Package.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsEvalDataObjectOperands (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ArgDesc;
    UINT32                  Length;


    ACPI_FUNCTION_TRACE (DsEvalDataObjectOperands);


    /* The first operand (for all of these data objects) is the length */

    /*
     * Set proper index into operand stack for AcpiDsObjStackPush
     * invoked inside AcpiDsCreateOperand.
     */
    WalkState->OperandIndex = WalkState->NumOperands;

    Status = AcpiDsCreateOperand (WalkState, Op->Common.Value.Arg, 1);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiExResolveOperands (WalkState->Opcode,
                    &(WalkState->Operands [WalkState->NumOperands -1]),
                    WalkState);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Extract length operand */

    ArgDesc = WalkState->Operands [WalkState->NumOperands - 1];
    Length = (UINT32) ArgDesc->Integer.Value;

    /* Cleanup for length operand */

    Status = AcpiDsObjStackPop (1, WalkState);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    AcpiUtRemoveReference (ArgDesc);

    /*
     * Create the actual data object
     */
    switch (Op->Common.AmlOpcode)
    {
    case AML_BUFFER_OP:

        Status = AcpiDsBuildInternalBufferObj (WalkState, Op, Length, &ObjDesc);
        break;

    case AML_PACKAGE_OP:
    case AML_VAR_PACKAGE_OP:

        Status = AcpiDsBuildInternalPackageObj (WalkState, Op, Length, &ObjDesc);
        break;

    default:
        return_ACPI_STATUS (AE_AML_BAD_OPCODE);
    }

    if (ACPI_SUCCESS (Status))
    {
        /*
         * Return the object in the WalkState, unless the parent is a package -
         * in this case, the return object will be stored in the parse tree
         * for the package.
         */
        if ((!Op->Common.Parent) ||
            ((Op->Common.Parent->Common.AmlOpcode != AML_PACKAGE_OP) &&
             (Op->Common.Parent->Common.AmlOpcode != AML_VAR_PACKAGE_OP) &&
             (Op->Common.Parent->Common.AmlOpcode != AML_NAME_OP)))
        {
            WalkState->ResultObj = ObjDesc;
        }
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsEvalBankFieldOperands
 *
 * PARAMETERS:  WalkState       - Current walk
 *              Op              - A valid BankField Op object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get BankField BankValue
 *              Called from AcpiDsExecEndOp during BankField parse tree walk
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsEvalBankFieldOperands (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *OperandDesc;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_PARSE_OBJECT       *NextOp;
    ACPI_PARSE_OBJECT       *Arg;


    ACPI_FUNCTION_TRACE_PTR (DsEvalBankFieldOperands, Op);


    /*
     * This is where we evaluate the BankValue field of the
     * BankField declaration
     */

    /* NextOp points to the op that holds the Region */

    NextOp = Op->Common.Value.Arg;

    /* NextOp points to the op that holds the Bank Register */

    NextOp = NextOp->Common.Next;

    /* NextOp points to the op that holds the Bank Value */

    NextOp = NextOp->Common.Next;

    /*
     * Set proper index into operand stack for AcpiDsObjStackPush
     * invoked inside AcpiDsCreateOperand.
     *
     * We use WalkState->Operands[0] to store the evaluated BankValue
     */
    WalkState->OperandIndex = 0;

    Status = AcpiDsCreateOperand (WalkState, NextOp, 0);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiExResolveToValue (&WalkState->Operands[0], WalkState);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    ACPI_DUMP_OPERANDS (ACPI_WALK_OPERANDS,
        AcpiPsGetOpcodeName (Op->Common.AmlOpcode), 1);
    /*
     * Get the BankValue operand and save it
     * (at Top of stack)
     */
    OperandDesc = WalkState->Operands[0];

    /* Arg points to the start Bank Field */

    Arg = AcpiPsGetArg (Op, 4);
    while (Arg)
    {
        /* Ignore OFFSET and ACCESSAS terms here */

        if (Arg->Common.AmlOpcode == AML_INT_NAMEDFIELD_OP)
        {
            Node = Arg->Common.Node;

            ObjDesc = AcpiNsGetAttachedObject (Node);
            if (!ObjDesc)
            {
                return_ACPI_STATUS (AE_NOT_EXIST);
            }

            ObjDesc->BankField.Value = (UINT32) OperandDesc->Integer.Value;
        }

        /* Move to next field in the list */

        Arg = Arg->Common.Next;
    }

    AcpiUtRemoveReference (OperandDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsExecBeginControlOp
 *
 * PARAMETERS:  WalkList        - The list that owns the walk stack
 *              Op              - The control Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handles all control ops encountered during control method
 *              execution.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsExecBeginControlOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_GENERIC_STATE      *ControlState;


    ACPI_FUNCTION_NAME (DsExecBeginControlOp);


    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH, "Op=%p Opcode=%2.2X State=%p\n", Op,
        Op->Common.AmlOpcode, WalkState));

    switch (Op->Common.AmlOpcode)
    {
    case AML_WHILE_OP:

        /*
         * If this is an additional iteration of a while loop, continue.
         * There is no need to allocate a new control state.
         */
        if (WalkState->ControlState)
        {
            if (WalkState->ControlState->Control.AmlPredicateStart ==
                (WalkState->ParserState.Aml - 1))
            {
                /* Reset the state to start-of-loop */

                WalkState->ControlState->Common.State = ACPI_CONTROL_CONDITIONAL_EXECUTING;
                break;
            }
        }

        /*lint -fallthrough */

    case AML_IF_OP:

        /*
         * IF/WHILE: Create a new control state to manage these
         * constructs. We need to manage these as a stack, in order
         * to handle nesting.
         */
        ControlState = AcpiUtCreateControlState ();
        if (!ControlState)
        {
            Status = AE_NO_MEMORY;
            break;
        }
        /*
         * Save a pointer to the predicate for multiple executions
         * of a loop
         */
        ControlState->Control.AmlPredicateStart = WalkState->ParserState.Aml - 1;
        ControlState->Control.PackageEnd = WalkState->ParserState.PkgEnd;
        ControlState->Control.Opcode = Op->Common.AmlOpcode;


        /* Push the control state on this walk's control stack */

        AcpiUtPushGenericState (&WalkState->ControlState, ControlState);
        break;

    case AML_ELSE_OP:

        /* Predicate is in the state object */
        /* If predicate is true, the IF was executed, ignore ELSE part */

        if (WalkState->LastPredicate)
        {
            Status = AE_CTRL_TRUE;
        }

        break;

    case AML_RETURN_OP:

        break;

    default:
        break;
    }

    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsExecEndControlOp
 *
 * PARAMETERS:  WalkList        - The list that owns the walk stack
 *              Op              - The control Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handles all control ops encountered during control method
 *              execution.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsExecEndControlOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_GENERIC_STATE      *ControlState;


    ACPI_FUNCTION_NAME (DsExecEndControlOp);


    switch (Op->Common.AmlOpcode)
    {
    case AML_IF_OP:

        ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH, "[IF_OP] Op=%p\n", Op));

        /*
         * Save the result of the predicate in case there is an
         * ELSE to come
         */
        WalkState->LastPredicate =
            (BOOLEAN) WalkState->ControlState->Common.Value;

        /*
         * Pop the control state that was created at the start
         * of the IF and free it
         */
        ControlState = AcpiUtPopGenericState (&WalkState->ControlState);
        AcpiUtDeleteGenericState (ControlState);
        break;


    case AML_ELSE_OP:

        break;


    case AML_WHILE_OP:

        ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH, "[WHILE_OP] Op=%p\n", Op));

        ControlState = WalkState->ControlState;
        if (ControlState->Common.Value)
        {
            /* Predicate was true, the body of the loop was just executed */

            /*
             * This loop counter mechanism allows the interpreter to escape
             * possibly infinite loops. This can occur in poorly written AML
             * when the hardware does not respond within a while loop and the
             * loop does not implement a timeout.
             */
            ControlState->Control.LoopCount++;
            if (ControlState->Control.LoopCount > ACPI_MAX_LOOP_ITERATIONS)
            {
                Status = AE_AML_INFINITE_LOOP;
                break;
            }

            /*
             * Go back and evaluate the predicate and maybe execute the loop
             * another time
             */
            Status = AE_CTRL_PENDING;
            WalkState->AmlLastWhile = ControlState->Control.AmlPredicateStart;
            break;
        }

        /* Predicate was false, terminate this while loop */

        ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
            "[WHILE_OP] termination! Op=%p\n",Op));

        /* Pop this control state and free it */

        ControlState = AcpiUtPopGenericState (&WalkState->ControlState);
        AcpiUtDeleteGenericState (ControlState);
        break;


    case AML_RETURN_OP:

        ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
            "[RETURN_OP] Op=%p Arg=%p\n",Op, Op->Common.Value.Arg));

        /*
         * One optional operand -- the return value
         * It can be either an immediate operand or a result that
         * has been bubbled up the tree
         */
        if (Op->Common.Value.Arg)
        {
            /* Since we have a real Return(), delete any implicit return */

            AcpiDsClearImplicitReturn (WalkState);

            /* Return statement has an immediate operand */

            Status = AcpiDsCreateOperands (WalkState, Op->Common.Value.Arg);
            if (ACPI_FAILURE (Status))
            {
                return (Status);
            }

            /*
             * If value being returned is a Reference (such as
             * an arg or local), resolve it now because it may
             * cease to exist at the end of the method.
             */
            Status = AcpiExResolveToValue (&WalkState->Operands [0], WalkState);
            if (ACPI_FAILURE (Status))
            {
                return (Status);
            }

            /*
             * Get the return value and save as the last result
             * value.  This is the only place where WalkState->ReturnDesc
             * is set to anything other than zero!
             */
            WalkState->ReturnDesc = WalkState->Operands[0];
        }
        else if (WalkState->ResultCount)
        {
            /* Since we have a real Return(), delete any implicit return */

            AcpiDsClearImplicitReturn (WalkState);

            /*
             * The return value has come from a previous calculation.
             *
             * If value being returned is a Reference (such as
             * an arg or local), resolve it now because it may
             * cease to exist at the end of the method.
             *
             * Allow references created by the Index operator to return unchanged.
             */
            if ((ACPI_GET_DESCRIPTOR_TYPE (WalkState->Results->Results.ObjDesc[0]) == ACPI_DESC_TYPE_OPERAND) &&
                ((WalkState->Results->Results.ObjDesc [0])->Common.Type == ACPI_TYPE_LOCAL_REFERENCE) &&
                ((WalkState->Results->Results.ObjDesc [0])->Reference.Class != ACPI_REFCLASS_INDEX))
            {
                Status = AcpiExResolveToValue (&WalkState->Results->Results.ObjDesc [0], WalkState);
                if (ACPI_FAILURE (Status))
                {
                    return (Status);
                }
            }

            WalkState->ReturnDesc = WalkState->Results->Results.ObjDesc [0];
        }
        else
        {
            /* No return operand */

            if (WalkState->NumOperands)
            {
                AcpiUtRemoveReference (WalkState->Operands [0]);
            }

            WalkState->Operands [0]     = NULL;
            WalkState->NumOperands      = 0;
            WalkState->ReturnDesc       = NULL;
        }


        ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
            "Completed RETURN_OP State=%p, RetVal=%p\n",
            WalkState, WalkState->ReturnDesc));

        /* End the control method execution right now */

        Status = AE_CTRL_TERMINATE;
        break;


    case AML_NOOP_OP:

        /* Just do nothing! */
        break;


    case AML_BREAK_POINT_OP:

        /*
         * Set the single-step flag. This will cause the debugger (if present)
         * to break to the console within the AML debugger at the start of the
         * next AML instruction.
         */
        ACPI_DEBUGGER_EXEC (
            AcpiGbl_CmSingleStep = TRUE);
        ACPI_DEBUGGER_EXEC (
            AcpiOsPrintf ("**break** Executed AML BreakPoint opcode\n"));

        /* Call to the OSL in case OS wants a piece of the action */

        Status = AcpiOsSignal (ACPI_SIGNAL_BREAKPOINT,
                    "Executed AML Breakpoint opcode");
        break;


    case AML_BREAK_OP:
    case AML_CONTINUE_OP: /* ACPI 2.0 */


        /* Pop and delete control states until we find a while */

        while (WalkState->ControlState &&
                (WalkState->ControlState->Control.Opcode != AML_WHILE_OP))
        {
            ControlState = AcpiUtPopGenericState (&WalkState->ControlState);
            AcpiUtDeleteGenericState (ControlState);
        }

        /* No while found? */

        if (!WalkState->ControlState)
        {
            return (AE_AML_NO_WHILE);
        }

        /* Was: WalkState->AmlLastWhile = WalkState->ControlState->Control.AmlPredicateStart; */

        WalkState->AmlLastWhile = WalkState->ControlState->Control.PackageEnd;

        /* Return status depending on opcode */

        if (Op->Common.AmlOpcode == AML_BREAK_OP)
        {
            Status = AE_CTRL_BREAK;
        }
        else
        {
            Status = AE_CTRL_CONTINUE;
        }
        break;


    default:

        ACPI_ERROR ((AE_INFO, "Unknown control opcode=%X Op=%p",
            Op->Common.AmlOpcode, Op));

        Status = AE_AML_BAD_OPCODE;
        break;
    }

    return (Status);
}

