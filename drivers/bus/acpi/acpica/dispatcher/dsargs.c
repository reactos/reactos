/******************************************************************************
 *
 * Module Name: dsargs - Support for execution of dynamic arguments for static
 *                       objects (regions, fields, buffer fields, etc.)
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

#define _COMPONENT          ACPI_DISPATCHER
        ACPI_MODULE_NAME    ("dsargs")

/* Local prototypes */

static ACPI_STATUS
AcpiDsExecuteArguments (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_NAMESPACE_NODE     *ScopeNode,
    UINT32                  AmlLength,
    UINT8                   *AmlStart);


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


    /* Allocate a new parser op to be the root of the parsed tree */

    Op = AcpiPsAllocOp (AML_INT_EVAL_SUBTREE_OP, AmlStart);
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

    Op = AcpiPsAllocOp (AML_INT_EVAL_SUBTREE_OP, AmlStart);
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
 * DESCRIPTION: Get BufferField Buffer and Index. This implements the late
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

    ACPI_DEBUG_EXEC (AcpiUtDisplayInitPathname (
        ACPI_TYPE_BUFFER_FIELD, Node, NULL));

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "[%4.4s] BufferField Arg Init\n",
        AcpiUtGetNodeName (Node)));

    /* Execute the AML code for the TermArg arguments */

    Status = AcpiDsExecuteArguments (Node, Node->Parent,
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
 * DESCRIPTION: Get BankField BankValue. This implements the late
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

    ACPI_DEBUG_EXEC (AcpiUtDisplayInitPathname (
        ACPI_TYPE_LOCAL_BANK_FIELD, Node, NULL));

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "[%4.4s] BankField Arg Init\n",
        AcpiUtGetNodeName (Node)));

    /* Execute the AML code for the TermArg arguments */

    Status = AcpiDsExecuteArguments (Node, Node->Parent,
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
 * DESCRIPTION: Get Buffer length and initializer byte list. This implements
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
            "No pointer back to namespace node in buffer object %p",
            ObjDesc));
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
 * DESCRIPTION: Get Package length and initializer byte list. This implements
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
            "No pointer back to namespace node in package %p", ObjDesc));
        return_ACPI_STATUS (AE_AML_INTERNAL);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Package Arg Init\n"));

    /* Execute the AML code for the TermArg arguments */

    Status = AcpiDsExecuteArguments (Node, Node,
        ObjDesc->Package.AmlLength, ObjDesc->Package.AmlStart);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsGetRegionArguments
 *
 * PARAMETERS:  ObjDesc         - A valid region object
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Get region address and length. This implements the late
 *              evaluation of these region attributes.
 *
 ******************************************************************************/

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

    ACPI_DEBUG_EXEC (AcpiUtDisplayInitPathname (
        ACPI_TYPE_REGION, Node, NULL));

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
        "[%4.4s] OpRegion Arg Init at AML %p\n",
        AcpiUtGetNodeName (Node), ExtraDesc->Extra.AmlStart));

    /* Execute the argument AML */

    Status = AcpiDsExecuteArguments (Node, ExtraDesc->Extra.ScopeNode,
        ExtraDesc->Extra.AmlLength, ExtraDesc->Extra.AmlStart);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiUtAddAddressRange (ObjDesc->Region.SpaceId,
        ObjDesc->Region.Address, ObjDesc->Region.Length, Node);
    return_ACPI_STATUS (Status);
}
