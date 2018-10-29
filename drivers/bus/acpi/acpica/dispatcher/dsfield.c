/******************************************************************************
 *
 * Module Name: dsfield - Dispatcher field routines
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
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
#include "acdispat.h"
#include "acinterp.h"
#include "acnamesp.h"
#include "acparser.h"

#ifdef ACPI_EXEC_APP
#include "aecommon.h"
#endif


#define _COMPONENT          ACPI_DISPATCHER
        ACPI_MODULE_NAME    ("dsfield")

/* Local prototypes */

#ifdef ACPI_ASL_COMPILER
#include "acdisasm.h"

static ACPI_STATUS
AcpiDsCreateExternalRegion (
    ACPI_STATUS             LookupStatus,
    ACPI_PARSE_OBJECT       *Op,
    char                    *Path,
    ACPI_WALK_STATE         *WalkState,
    ACPI_NAMESPACE_NODE     **Node);
#endif

static ACPI_STATUS
AcpiDsGetFieldNames (
    ACPI_CREATE_FIELD_INFO  *Info,
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Arg);


#ifdef ACPI_ASL_COMPILER
/*******************************************************************************
 *
 * FUNCTION:    AcpiDsCreateExternalRegion (iASL Disassembler only)
 *
 * PARAMETERS:  LookupStatus    - Status from NsLookup operation
 *              Op              - Op containing the Field definition and args
 *              Path            - Pathname of the region
 *  `           WalkState       - Current method state
 *              Node            - Where the new region node is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Add region to the external list if NOT_FOUND. Create a new
 *              region node/object.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiDsCreateExternalRegion (
    ACPI_STATUS             LookupStatus,
    ACPI_PARSE_OBJECT       *Op,
    char                    *Path,
    ACPI_WALK_STATE         *WalkState,
    ACPI_NAMESPACE_NODE     **Node)
{
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;


    if (LookupStatus != AE_NOT_FOUND)
    {
        return (LookupStatus);
    }

    /*
     * Table disassembly:
     * OperationRegion not found. Generate an External for it, and
     * insert the name into the namespace.
     */
    AcpiDmAddOpToExternalList (Op, Path, ACPI_TYPE_REGION, 0, 0);

    Status = AcpiNsLookup (WalkState->ScopeInfo, Path, ACPI_TYPE_REGION,
       ACPI_IMODE_LOAD_PASS1, ACPI_NS_SEARCH_PARENT, WalkState, Node);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Must create and install a region object for the new node */

    ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_REGION);
    if (!ObjDesc)
    {
        return (AE_NO_MEMORY);
    }

    ObjDesc->Region.Node = *Node;
    Status = AcpiNsAttachObject (*Node, ObjDesc, ACPI_TYPE_REGION);
    return (Status);
}
#endif


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsCreateBufferField
 *
 * PARAMETERS:  Op                  - Current parse op (CreateXXField)
 *              WalkState           - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute the CreateField operators:
 *              CreateBitFieldOp,
 *              CreateByteFieldOp,
 *              CreateWordFieldOp,
 *              CreateDwordFieldOp,
 *              CreateQwordFieldOp,
 *              CreateFieldOp       (all of which define a field in a buffer)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsCreateBufferField (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_PARSE_OBJECT       *Arg;
    ACPI_NAMESPACE_NODE     *Node;
    ACPI_STATUS             Status;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *SecondDesc = NULL;
    UINT32                  Flags;


    ACPI_FUNCTION_TRACE (DsCreateBufferField);


    /*
     * Get the NameString argument (name of the new BufferField)
     */
    if (Op->Common.AmlOpcode == AML_CREATE_FIELD_OP)
    {
        /* For CreateField, name is the 4th argument */

        Arg = AcpiPsGetArg (Op, 3);
    }
    else
    {
        /* For all other CreateXXXField operators, name is the 3rd argument */

        Arg = AcpiPsGetArg (Op, 2);
    }

    if (!Arg)
    {
        return_ACPI_STATUS (AE_AML_NO_OPERAND);
    }

    if (WalkState->DeferredNode)
    {
        Node = WalkState->DeferredNode;
        Status = AE_OK;
    }
    else
    {
        /* Execute flag should always be set when this function is entered */

        if (!(WalkState->ParseFlags & ACPI_PARSE_EXECUTE))
        {
            ACPI_ERROR ((AE_INFO,
                "Parse execute mode is not set"));
            return_ACPI_STATUS (AE_AML_INTERNAL);
        }

        /* Creating new namespace node, should not already exist */

        Flags = ACPI_NS_NO_UPSEARCH | ACPI_NS_DONT_OPEN_SCOPE |
            ACPI_NS_ERROR_IF_FOUND;

        /*
         * Mark node temporary if we are executing a normal control
         * method. (Don't mark if this is a module-level code method)
         */
        if (WalkState->MethodNode &&
            !(WalkState->ParseFlags & ACPI_PARSE_MODULE_LEVEL))
        {
            Flags |= ACPI_NS_TEMPORARY;
        }

        /* Enter the NameString into the namespace */

        Status = AcpiNsLookup (WalkState->ScopeInfo,
            Arg->Common.Value.String, ACPI_TYPE_ANY,
            ACPI_IMODE_LOAD_PASS1, Flags, WalkState, &Node);
        if (ACPI_FAILURE (Status))
        {
            ACPI_ERROR_NAMESPACE (WalkState->ScopeInfo,
                Arg->Common.Value.String, Status);
            return_ACPI_STATUS (Status);
        }
    }

    /*
     * We could put the returned object (Node) on the object stack for later,
     * but for now, we will put it in the "op" object that the parser uses,
     * so we can get it again at the end of this scope.
     */
    Op->Common.Node = Node;

    /*
     * If there is no object attached to the node, this node was just created
     * and we need to create the field object. Otherwise, this was a lookup
     * of an existing node and we don't want to create the field object again.
     */
    ObjDesc = AcpiNsGetAttachedObject (Node);
    if (ObjDesc)
    {
        return_ACPI_STATUS (AE_OK);
    }

    /*
     * The Field definition is not fully parsed at this time.
     * (We must save the address of the AML for the buffer and index operands)
     */

    /* Create the buffer field object */

    ObjDesc = AcpiUtCreateInternalObject (ACPI_TYPE_BUFFER_FIELD);
    if (!ObjDesc)
    {
        Status = AE_NO_MEMORY;
        goto Cleanup;
    }

    /*
     * Remember location in AML stream of the field unit opcode and operands
     * -- since the buffer and index operands must be evaluated.
     */
    SecondDesc = ObjDesc->Common.NextObject;
    SecondDesc->Extra.AmlStart = Op->Named.Data;
    SecondDesc->Extra.AmlLength = Op->Named.Length;
    ObjDesc->BufferField.Node = Node;

    /* Attach constructed field descriptors to parent node */

    Status = AcpiNsAttachObject (Node, ObjDesc, ACPI_TYPE_BUFFER_FIELD);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }


Cleanup:

    /* Remove local reference to the object */

    AcpiUtRemoveReference (ObjDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsGetFieldNames
 *
 * PARAMETERS:  Info            - CreateField info structure
 *  `           WalkState       - Current method state
 *              Arg             - First parser arg for the field name list
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Process all named fields in a field declaration. Names are
 *              entered into the namespace.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiDsGetFieldNames (
    ACPI_CREATE_FIELD_INFO  *Info,
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Arg)
{
    ACPI_STATUS             Status;
    UINT64                  Position;
    ACPI_PARSE_OBJECT       *Child;

#ifdef ACPI_EXEC_APP
    UINT64                  Value = 0;
    ACPI_OPERAND_OBJECT     *ResultDesc;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    char                    *NamePath;
#endif


    ACPI_FUNCTION_TRACE_PTR (DsGetFieldNames, Info);


    /* First field starts at bit zero */

    Info->FieldBitPosition = 0;

    /* Process all elements in the field list (of parse nodes) */

    while (Arg)
    {
        /*
         * Four types of field elements are handled:
         * 1) Name - Enters a new named field into the namespace
         * 2) Offset - specifies a bit offset
         * 3) AccessAs - changes the access mode/attributes
         * 4) Connection - Associate a resource template with the field
         */
        switch (Arg->Common.AmlOpcode)
        {
        case AML_INT_RESERVEDFIELD_OP:

            Position = (UINT64) Info->FieldBitPosition +
                (UINT64) Arg->Common.Value.Size;

            if (Position > ACPI_UINT32_MAX)
            {
                ACPI_ERROR ((AE_INFO,
                    "Bit offset within field too large (> 0xFFFFFFFF)"));
                return_ACPI_STATUS (AE_SUPPORT);
            }

            Info->FieldBitPosition = (UINT32) Position;
            break;

        case AML_INT_ACCESSFIELD_OP:
        case AML_INT_EXTACCESSFIELD_OP:
            /*
             * Get new AccessType, AccessAttribute, and AccessLength fields
             * -- to be used for all field units that follow, until the
             * end-of-field or another AccessAs keyword is encountered.
             * NOTE. These three bytes are encoded in the integer value
             * of the parseop for convenience.
             *
             * In FieldFlags, preserve the flag bits other than the
             * ACCESS_TYPE bits.
             */

            /* AccessType (ByteAcc, WordAcc, etc.) */

            Info->FieldFlags = (UINT8)
                ((Info->FieldFlags & ~(AML_FIELD_ACCESS_TYPE_MASK)) |
                ((UINT8) ((UINT32) (Arg->Common.Value.Integer & 0x07))));

            /* AccessAttribute (AttribQuick, AttribByte, etc.) */

            Info->Attribute = (UINT8)
                ((Arg->Common.Value.Integer >> 8) & 0xFF);

            /* AccessLength (for serial/buffer protocols) */

            Info->AccessLength = (UINT8)
                ((Arg->Common.Value.Integer >> 16) & 0xFF);
            break;

        case AML_INT_CONNECTION_OP:
            /*
             * Clear any previous connection. New connection is used for all
             * fields that follow, similar to AccessAs
             */
            Info->ResourceBuffer = NULL;
            Info->ConnectionNode = NULL;
            Info->PinNumberIndex = 0;

            /*
             * A Connection() is either an actual resource descriptor (buffer)
             * or a named reference to a resource template
             */
            Child = Arg->Common.Value.Arg;
            if (Child->Common.AmlOpcode == AML_INT_BYTELIST_OP)
            {
                Info->ResourceBuffer = Child->Named.Data;
                Info->ResourceLength = (UINT16) Child->Named.Value.Integer;
            }
            else
            {
                /* Lookup the Connection() namepath, it should already exist */

                Status = AcpiNsLookup (WalkState->ScopeInfo,
                    Child->Common.Value.Name, ACPI_TYPE_ANY,
                    ACPI_IMODE_EXECUTE, ACPI_NS_DONT_OPEN_SCOPE,
                    WalkState, &Info->ConnectionNode);
                if (ACPI_FAILURE (Status))
                {
                    ACPI_ERROR_NAMESPACE (WalkState->ScopeInfo,
                        Child->Common.Value.Name, Status);
                    return_ACPI_STATUS (Status);
                }
            }
            break;

        case AML_INT_NAMEDFIELD_OP:

            /* Lookup the name, it should already exist */

            Status = AcpiNsLookup (WalkState->ScopeInfo,
                (char *) &Arg->Named.Name, Info->FieldType,
                ACPI_IMODE_EXECUTE, ACPI_NS_DONT_OPEN_SCOPE,
                WalkState, &Info->FieldNode);
            if (ACPI_FAILURE (Status))
            {
                ACPI_ERROR_NAMESPACE (WalkState->ScopeInfo,
                    (char *) &Arg->Named.Name, Status);
                return_ACPI_STATUS (Status);
            }
            else
            {
                Arg->Common.Node = Info->FieldNode;
                Info->FieldBitLength = Arg->Common.Value.Size;

                /*
                 * If there is no object attached to the node, this node was
                 * just created and we need to create the field object.
                 * Otherwise, this was a lookup of an existing node and we
                 * don't want to create the field object again.
                 */
                if (!AcpiNsGetAttachedObject (Info->FieldNode))
                {
                    Status = AcpiExPrepFieldValue (Info);
                    if (ACPI_FAILURE (Status))
                    {
                        return_ACPI_STATUS (Status);
                    }
#ifdef ACPI_EXEC_APP
                    NamePath = AcpiNsGetExternalPathname (Info->FieldNode);
                    ObjDesc = AcpiUtCreateIntegerObject (Value);
                    if (ACPI_SUCCESS (AeLookupInitFileEntry (NamePath, &Value)))
                    {
                        AcpiExWriteDataToField (ObjDesc,
                            AcpiNsGetAttachedObject (Info->FieldNode),
                            &ResultDesc);
                    }
                    AcpiUtRemoveReference (ObjDesc);
                    ACPI_FREE (NamePath);
#endif
                }
            }

            /* Keep track of bit position for the next field */

            Position = (UINT64) Info->FieldBitPosition +
                (UINT64) Arg->Common.Value.Size;

            if (Position > ACPI_UINT32_MAX)
            {
                ACPI_ERROR ((AE_INFO,
                    "Field [%4.4s] bit offset too large (> 0xFFFFFFFF)",
                    ACPI_CAST_PTR (char, &Info->FieldNode->Name)));
                return_ACPI_STATUS (AE_SUPPORT);
            }

            Info->FieldBitPosition += Info->FieldBitLength;
            Info->PinNumberIndex++; /* Index relative to previous Connection() */
            break;

        default:

            ACPI_ERROR ((AE_INFO,
                "Invalid opcode in field list: 0x%X",
                Arg->Common.AmlOpcode));
            return_ACPI_STATUS (AE_AML_BAD_OPCODE);
        }

        Arg = Arg->Common.Next;
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsCreateField
 *
 * PARAMETERS:  Op              - Op containing the Field definition and args
 *              RegionNode      - Object for the containing Operation Region
 *  `           WalkState       - Current method state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new field in the specified operation region
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsCreateField (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_NAMESPACE_NODE     *RegionNode,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status;
    ACPI_PARSE_OBJECT       *Arg;
    ACPI_CREATE_FIELD_INFO  Info;


    ACPI_FUNCTION_TRACE_PTR (DsCreateField, Op);


    /* First arg is the name of the parent OpRegion (must already exist) */

    Arg = Op->Common.Value.Arg;

    if (!RegionNode)
    {
        Status = AcpiNsLookup (WalkState->ScopeInfo, Arg->Common.Value.Name,
            ACPI_TYPE_REGION, ACPI_IMODE_EXECUTE,
            ACPI_NS_SEARCH_PARENT, WalkState, &RegionNode);
#ifdef ACPI_ASL_COMPILER
        Status = AcpiDsCreateExternalRegion (Status, Arg,
            Arg->Common.Value.Name, WalkState, &RegionNode);
#endif
        if (ACPI_FAILURE (Status))
        {
            ACPI_ERROR_NAMESPACE (WalkState->ScopeInfo,
                Arg->Common.Value.Name, Status);
            return_ACPI_STATUS (Status);
        }
    }

    memset (&Info, 0, sizeof (ACPI_CREATE_FIELD_INFO));

    /* Second arg is the field flags */

    Arg = Arg->Common.Next;
    Info.FieldFlags = (UINT8) Arg->Common.Value.Integer;
    Info.Attribute = 0;

    /* Each remaining arg is a Named Field */

    Info.FieldType = ACPI_TYPE_LOCAL_REGION_FIELD;
    Info.RegionNode = RegionNode;

    Status = AcpiDsGetFieldNames (&Info, WalkState, Arg->Common.Next);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsInitFieldObjects
 *
 * PARAMETERS:  Op              - Op containing the Field definition and args
 *  `           WalkState       - Current method state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: For each "Field Unit" name in the argument list that is
 *              part of the field declaration, enter the name into the
 *              namespace.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsInitFieldObjects (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status;
    ACPI_PARSE_OBJECT       *Arg = NULL;
    ACPI_NAMESPACE_NODE     *Node;
    UINT8                   Type = 0;
    UINT32                  Flags;


    ACPI_FUNCTION_TRACE_PTR (DsInitFieldObjects, Op);


    /* Execute flag should always be set when this function is entered */

    if (!(WalkState->ParseFlags & ACPI_PARSE_EXECUTE))
    {
        if (WalkState->ParseFlags & ACPI_PARSE_DEFERRED_OP)
        {
            /* BankField Op is deferred, just return OK */

            return_ACPI_STATUS (AE_OK);
        }

        ACPI_ERROR ((AE_INFO,
            "Parse deferred mode is not set"));
        return_ACPI_STATUS (AE_AML_INTERNAL);
    }

    /*
     * Get the FieldList argument for this opcode. This is the start of the
     * list of field elements.
     */
    switch (WalkState->Opcode)
    {
    case AML_FIELD_OP:

        Arg = AcpiPsGetArg (Op, 2);
        Type = ACPI_TYPE_LOCAL_REGION_FIELD;
        break;

    case AML_BANK_FIELD_OP:

        Arg = AcpiPsGetArg (Op, 4);
        Type = ACPI_TYPE_LOCAL_BANK_FIELD;
        break;

    case AML_INDEX_FIELD_OP:

        Arg = AcpiPsGetArg (Op, 3);
        Type = ACPI_TYPE_LOCAL_INDEX_FIELD;
        break;

    default:

        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Creating new namespace node(s), should not already exist */

    Flags = ACPI_NS_NO_UPSEARCH | ACPI_NS_DONT_OPEN_SCOPE |
        ACPI_NS_ERROR_IF_FOUND;

    /*
     * Mark node(s) temporary if we are executing a normal control
     * method. (Don't mark if this is a module-level code method)
     */
    if (WalkState->MethodNode &&
        !(WalkState->ParseFlags & ACPI_PARSE_MODULE_LEVEL))
    {
        Flags |= ACPI_NS_TEMPORARY;
    }

#ifdef ACPI_EXEC_APP
        Flags |= ACPI_NS_OVERRIDE_IF_FOUND;
#endif
    /*
     * Walk the list of entries in the FieldList
     * Note: FieldList can be of zero length. In this case, Arg will be NULL.
     */
    while (Arg)
    {
        /*
         * Ignore OFFSET/ACCESSAS/CONNECTION terms here; we are only interested
         * in the field names in order to enter them into the namespace.
         */
        if (Arg->Common.AmlOpcode == AML_INT_NAMEDFIELD_OP)
        {
            Status = AcpiNsLookup (WalkState->ScopeInfo,
                (char *) &Arg->Named.Name, Type, ACPI_IMODE_LOAD_PASS1,
                Flags, WalkState, &Node);
            if (ACPI_FAILURE (Status))
            {
                ACPI_ERROR_NAMESPACE (WalkState->ScopeInfo,
                    (char *) &Arg->Named.Name, Status);
                if (Status != AE_ALREADY_EXISTS)
                {
                    return_ACPI_STATUS (Status);
                }

                /* Name already exists, just ignore this error */

                Status = AE_OK;
            }

            Arg->Common.Node = Node;
        }

        /* Get the next field element in the list */

        Arg = Arg->Common.Next;
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsCreateBankField
 *
 * PARAMETERS:  Op              - Op containing the Field definition and args
 *              RegionNode      - Object for the containing Operation Region
 *              WalkState       - Current method state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new bank field in the specified operation region
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsCreateBankField (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_NAMESPACE_NODE     *RegionNode,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status;
    ACPI_PARSE_OBJECT       *Arg;
    ACPI_CREATE_FIELD_INFO  Info;


    ACPI_FUNCTION_TRACE_PTR (DsCreateBankField, Op);


    /* First arg is the name of the parent OpRegion (must already exist) */

    Arg = Op->Common.Value.Arg;
    if (!RegionNode)
    {
        Status = AcpiNsLookup (WalkState->ScopeInfo, Arg->Common.Value.Name,
            ACPI_TYPE_REGION, ACPI_IMODE_EXECUTE,
            ACPI_NS_SEARCH_PARENT, WalkState, &RegionNode);
#ifdef ACPI_ASL_COMPILER
        Status = AcpiDsCreateExternalRegion (Status, Arg,
            Arg->Common.Value.Name, WalkState, &RegionNode);
#endif
        if (ACPI_FAILURE (Status))
        {
            ACPI_ERROR_NAMESPACE (WalkState->ScopeInfo,
                Arg->Common.Value.Name, Status);
            return_ACPI_STATUS (Status);
        }
    }

    /* Second arg is the Bank Register (Field) (must already exist) */

    Arg = Arg->Common.Next;
    Status = AcpiNsLookup (WalkState->ScopeInfo, Arg->Common.Value.String,
        ACPI_TYPE_ANY, ACPI_IMODE_EXECUTE,
        ACPI_NS_SEARCH_PARENT, WalkState, &Info.RegisterNode);
    if (ACPI_FAILURE (Status))
    {
        ACPI_ERROR_NAMESPACE (WalkState->ScopeInfo,
            Arg->Common.Value.String, Status);
        return_ACPI_STATUS (Status);
    }

    /*
     * Third arg is the BankValue
     * This arg is a TermArg, not a constant
     * It will be evaluated later, by AcpiDsEvalBankFieldOperands
     */
    Arg = Arg->Common.Next;

    /* Fourth arg is the field flags */

    Arg = Arg->Common.Next;
    Info.FieldFlags = (UINT8) Arg->Common.Value.Integer;

    /* Each remaining arg is a Named Field */

    Info.FieldType = ACPI_TYPE_LOCAL_BANK_FIELD;
    Info.RegionNode = RegionNode;

    /*
     * Use Info.DataRegisterNode to store BankField Op
     * It's safe because DataRegisterNode will never be used when create
     * bank field \we store AmlStart and AmlLength in the BankField Op for
     * late evaluation. Used in AcpiExPrepFieldValue(Info)
     *
     * TBD: Or, should we add a field in ACPI_CREATE_FIELD_INFO, like
     * "void *ParentOp"?
     */
    Info.DataRegisterNode = (ACPI_NAMESPACE_NODE*) Op;

    Status = AcpiDsGetFieldNames (&Info, WalkState, Arg->Common.Next);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsCreateIndexField
 *
 * PARAMETERS:  Op              - Op containing the Field definition and args
 *              RegionNode      - Object for the containing Operation Region
 *  `           WalkState       - Current method state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new index field in the specified operation region
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsCreateIndexField (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_NAMESPACE_NODE     *RegionNode,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status;
    ACPI_PARSE_OBJECT       *Arg;
    ACPI_CREATE_FIELD_INFO  Info;


    ACPI_FUNCTION_TRACE_PTR (DsCreateIndexField, Op);


    /* First arg is the name of the Index register (must already exist) */

    Arg = Op->Common.Value.Arg;
    Status = AcpiNsLookup (WalkState->ScopeInfo, Arg->Common.Value.String,
        ACPI_TYPE_ANY, ACPI_IMODE_EXECUTE,
        ACPI_NS_SEARCH_PARENT, WalkState, &Info.RegisterNode);
    if (ACPI_FAILURE (Status))
    {
        ACPI_ERROR_NAMESPACE (WalkState->ScopeInfo,
            Arg->Common.Value.String, Status);
        return_ACPI_STATUS (Status);
    }

    /* Second arg is the data register (must already exist) */

    Arg = Arg->Common.Next;
    Status = AcpiNsLookup (WalkState->ScopeInfo, Arg->Common.Value.String,
        ACPI_TYPE_ANY, ACPI_IMODE_EXECUTE,
        ACPI_NS_SEARCH_PARENT, WalkState, &Info.DataRegisterNode);
    if (ACPI_FAILURE (Status))
    {
        ACPI_ERROR_NAMESPACE (WalkState->ScopeInfo,
            Arg->Common.Value.String, Status);
        return_ACPI_STATUS (Status);
    }

    /* Next arg is the field flags */

    Arg = Arg->Common.Next;
    Info.FieldFlags = (UINT8) Arg->Common.Value.Integer;

    /* Each remaining arg is a Named Field */

    Info.FieldType = ACPI_TYPE_LOCAL_INDEX_FIELD;
    Info.RegionNode = RegionNode;

    Status = AcpiDsGetFieldNames (&Info, WalkState, Arg->Common.Next);
    return_ACPI_STATUS (Status);
}
