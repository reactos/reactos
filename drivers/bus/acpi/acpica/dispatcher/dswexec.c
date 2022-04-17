/******************************************************************************
 *
 * Module Name: dswexec - Dispatcher method execution callbacks;
 *                        dispatch to interpreter.
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
#include "acparser.h"
#include "amlcode.h"
#include "acdispat.h"
#include "acinterp.h"
#include "acnamesp.h"
#include "acdebug.h"
#ifdef ACPI_EXEC_APP
#include "aecommon.h"
#endif

#define _COMPONENT          ACPI_DISPATCHER
        ACPI_MODULE_NAME    ("dswexec")

/*
 * Dispatch table for opcode classes
 */
static ACPI_EXECUTE_OP      AcpiGbl_OpTypeDispatch [] =
{
    AcpiExOpcode_0A_0T_1R,
    AcpiExOpcode_1A_0T_0R,
    AcpiExOpcode_1A_0T_1R,
    AcpiExOpcode_1A_1T_0R,
    AcpiExOpcode_1A_1T_1R,
    AcpiExOpcode_2A_0T_0R,
    AcpiExOpcode_2A_0T_1R,
    AcpiExOpcode_2A_1T_1R,
    AcpiExOpcode_2A_2T_1R,
    AcpiExOpcode_3A_0T_0R,
    AcpiExOpcode_3A_1T_1R,
    AcpiExOpcode_6A_0T_1R
};


/*****************************************************************************
 *
 * FUNCTION:    AcpiDsGetPredicateValue
 *
 * PARAMETERS:  WalkState       - Current state of the parse tree walk
 *              ResultObj       - if non-zero, pop result from result stack
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get the result of a predicate evaluation
 *
 ****************************************************************************/

ACPI_STATUS
AcpiDsGetPredicateValue (
    ACPI_WALK_STATE         *WalkState,
    ACPI_OPERAND_OBJECT     *ResultObj)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_OPERAND_OBJECT     *LocalObjDesc = NULL;


    ACPI_FUNCTION_TRACE_PTR (DsGetPredicateValue, WalkState);


    WalkState->ControlState->Common.State = 0;

    if (ResultObj)
    {
        Status = AcpiDsResultPop (&ObjDesc, WalkState);
        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Could not get result from predicate evaluation"));

            return_ACPI_STATUS (Status);
        }
    }
    else
    {
        Status = AcpiDsCreateOperand (WalkState, WalkState->Op, 0);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        Status = AcpiExResolveToValue (&WalkState->Operands [0], WalkState);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        ObjDesc = WalkState->Operands [0];
    }

    if (!ObjDesc)
    {
        ACPI_ERROR ((AE_INFO,
            "No predicate ObjDesc=%p State=%p",
            ObjDesc, WalkState));

        return_ACPI_STATUS (AE_AML_NO_OPERAND);
    }

    /*
     * Result of predicate evaluation must be an Integer
     * object. Implicitly convert the argument if necessary.
     */
    Status = AcpiExConvertToInteger (ObjDesc, &LocalObjDesc,
        ACPI_IMPLICIT_CONVERSION);
    if (ACPI_FAILURE (Status))
    {
        goto Cleanup;
    }

    if (LocalObjDesc->Common.Type != ACPI_TYPE_INTEGER)
    {
        ACPI_ERROR ((AE_INFO,
            "Bad predicate (not an integer) ObjDesc=%p State=%p Type=0x%X",
            ObjDesc, WalkState, ObjDesc->Common.Type));

        Status = AE_AML_OPERAND_TYPE;
        goto Cleanup;
    }

    /* Truncate the predicate to 32-bits if necessary */

    (void) AcpiExTruncateFor32bitTable (LocalObjDesc);

    /*
     * Save the result of the predicate evaluation on
     * the control stack
     */
    if (LocalObjDesc->Integer.Value)
    {
        WalkState->ControlState->Common.Value = TRUE;
    }
    else
    {
        /*
         * Predicate is FALSE, we will just toss the
         * rest of the package
         */
        WalkState->ControlState->Common.Value = FALSE;
        Status = AE_CTRL_FALSE;
    }

    /* Predicate can be used for an implicit return value */

    (void) AcpiDsDoImplicitReturn (LocalObjDesc, WalkState, TRUE);


Cleanup:

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
        "Completed a predicate eval=%X Op=%p\n",
        WalkState->ControlState->Common.Value, WalkState->Op));

    /* Break to debugger to display result */

    AcpiDbDisplayResultObject (LocalObjDesc, WalkState);

    /*
     * Delete the predicate result object (we know that
     * we don't need it anymore)
     */
    if (LocalObjDesc != ObjDesc)
    {
        AcpiUtRemoveReference (LocalObjDesc);
    }
    AcpiUtRemoveReference (ObjDesc);

    WalkState->ControlState->Common.State = ACPI_CONTROL_NORMAL;
    return_ACPI_STATUS (Status);
}


/*****************************************************************************
 *
 * FUNCTION:    AcpiDsExecBeginOp
 *
 * PARAMETERS:  WalkState       - Current state of the parse tree walk
 *              OutOp           - Where to return op if a new one is created
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Descending callback used during the execution of control
 *              methods. This is where most operators and operands are
 *              dispatched to the interpreter.
 *
 ****************************************************************************/

ACPI_STATUS
AcpiDsExecBeginOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       **OutOp)
{
    ACPI_PARSE_OBJECT       *Op;
    ACPI_STATUS             Status = AE_OK;
    UINT32                  OpcodeClass;


    ACPI_FUNCTION_TRACE_PTR (DsExecBeginOp, WalkState);


    Op = WalkState->Op;
    if (!Op)
    {
        Status = AcpiDsLoad2BeginOp (WalkState, OutOp);
        if (ACPI_FAILURE (Status))
        {
            goto ErrorExit;
        }

        Op = *OutOp;
        WalkState->Op = Op;
        WalkState->Opcode = Op->Common.AmlOpcode;
        WalkState->OpInfo = AcpiPsGetOpcodeInfo (Op->Common.AmlOpcode);

        if (AcpiNsOpensScope (WalkState->OpInfo->ObjectType))
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
                "(%s) Popping scope for Op %p\n",
                AcpiUtGetTypeName (WalkState->OpInfo->ObjectType), Op));

            Status = AcpiDsScopeStackPop (WalkState);
            if (ACPI_FAILURE (Status))
            {
                goto ErrorExit;
            }
        }
    }

    if (Op == WalkState->Origin)
    {
        if (OutOp)
        {
            *OutOp = Op;
        }

        return_ACPI_STATUS (AE_OK);
    }

    /*
     * If the previous opcode was a conditional, this opcode
     * must be the beginning of the associated predicate.
     * Save this knowledge in the current scope descriptor
     */
    if ((WalkState->ControlState) &&
        (WalkState->ControlState->Common.State ==
            ACPI_CONTROL_CONDITIONAL_EXECUTING))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "Exec predicate Op=%p State=%p\n",
            Op, WalkState));

        WalkState->ControlState->Common.State =
            ACPI_CONTROL_PREDICATE_EXECUTING;

        /* Save start of predicate */

        WalkState->ControlState->Control.PredicateOp = Op;
    }


    OpcodeClass = WalkState->OpInfo->Class;

    /* We want to send namepaths to the load code */

    if (Op->Common.AmlOpcode == AML_INT_NAMEPATH_OP)
    {
        OpcodeClass = AML_CLASS_NAMED_OBJECT;
    }

    /*
     * Handle the opcode based upon the opcode type
     */
    switch (OpcodeClass)
    {
    case AML_CLASS_CONTROL:

        Status = AcpiDsExecBeginControlOp (WalkState, Op);
        break;

    case AML_CLASS_NAMED_OBJECT:

        if (WalkState->WalkType & ACPI_WALK_METHOD)
        {
            /*
             * Found a named object declaration during method execution;
             * we must enter this object into the namespace. The created
             * object is temporary and will be deleted upon completion of
             * the execution of this method.
             *
             * Note 10/2010: Except for the Scope() op. This opcode does
             * not actually create a new object, it refers to an existing
             * object. However, for Scope(), we want to indeed open a
             * new scope.
             */
            if (Op->Common.AmlOpcode != AML_SCOPE_OP)
            {
                Status = AcpiDsLoad2BeginOp (WalkState, NULL);
            }
            else
            {
                Status = AcpiDsScopeStackPush (
                    Op->Named.Node, Op->Named.Node->Type, WalkState);
                if (ACPI_FAILURE (Status))
                {
                    return_ACPI_STATUS (Status);
                }
            }
        }
        break;

    case AML_CLASS_EXECUTE:
    case AML_CLASS_CREATE:

        break;

    default:

        break;
    }

    /* Nothing to do here during method execution */

    return_ACPI_STATUS (Status);


ErrorExit:
    Status = AcpiDsMethodError (Status, WalkState);
    return_ACPI_STATUS (Status);
}


/*****************************************************************************
 *
 * FUNCTION:    AcpiDsExecEndOp
 *
 * PARAMETERS:  WalkState       - Current state of the parse tree walk
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Ascending callback used during the execution of control
 *              methods. The only thing we really need to do here is to
 *              notice the beginning of IF, ELSE, and WHILE blocks.
 *
 ****************************************************************************/

ACPI_STATUS
AcpiDsExecEndOp (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_PARSE_OBJECT       *Op;
    ACPI_STATUS             Status = AE_OK;
    UINT32                  OpType;
    UINT32                  OpClass;
    ACPI_PARSE_OBJECT       *NextOp;
    ACPI_PARSE_OBJECT       *FirstArg;
#ifdef ACPI_EXEC_APP
    char                    *Namepath;
    ACPI_OPERAND_OBJECT     *ObjDesc;
#endif

    ACPI_FUNCTION_TRACE_PTR (DsExecEndOp, WalkState);


    Op = WalkState->Op;
    OpType = WalkState->OpInfo->Type;
    OpClass = WalkState->OpInfo->Class;

    if (OpClass == AML_CLASS_UNKNOWN)
    {
        ACPI_ERROR ((AE_INFO, "Unknown opcode 0x%X", Op->Common.AmlOpcode));
        return_ACPI_STATUS (AE_NOT_IMPLEMENTED);
    }

    FirstArg = Op->Common.Value.Arg;

    /* Init the walk state */

    WalkState->NumOperands = 0;
    WalkState->OperandIndex = 0;
    WalkState->ReturnDesc = NULL;
    WalkState->ResultObj = NULL;

    /* Call debugger for single step support (DEBUG build only) */

    Status = AcpiDbSingleStep (WalkState, Op, OpClass);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Decode the Opcode Class */

    switch (OpClass)
    {
    case AML_CLASS_ARGUMENT:    /* Constants, literals, etc. */

        if (WalkState->Opcode == AML_INT_NAMEPATH_OP)
        {
            Status = AcpiDsEvaluateNamePath (WalkState);
            if (ACPI_FAILURE (Status))
            {
                goto Cleanup;
            }
        }
        break;

    case AML_CLASS_EXECUTE:     /* Most operators with arguments */

        /* Build resolved operand stack */

        Status = AcpiDsCreateOperands (WalkState, FirstArg);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }

        /*
         * All opcodes require operand resolution, with the only exceptions
         * being the ObjectType and SizeOf operators.
         */
        if (!(WalkState->OpInfo->Flags & AML_NO_OPERAND_RESOLVE))
        {
            /* Resolve all operands */

            Status = AcpiExResolveOperands (WalkState->Opcode,
                &(WalkState->Operands [WalkState->NumOperands -1]),
                WalkState);
        }

        if (ACPI_SUCCESS (Status))
        {
            /*
             * Dispatch the request to the appropriate interpreter handler
             * routine. There is one routine per opcode "type" based upon the
             * number of opcode arguments and return type.
             */
            Status = AcpiGbl_OpTypeDispatch[OpType] (WalkState);
        }
        else
        {
            /*
             * Treat constructs of the form "Store(LocalX,LocalX)" as noops when the
             * Local is uninitialized.
             */
            if  ((Status == AE_AML_UNINITIALIZED_LOCAL) &&
                (WalkState->Opcode == AML_STORE_OP) &&
                (WalkState->Operands[0]->Common.Type == ACPI_TYPE_LOCAL_REFERENCE) &&
                (WalkState->Operands[1]->Common.Type == ACPI_TYPE_LOCAL_REFERENCE) &&
                (WalkState->Operands[0]->Reference.Class ==
                 WalkState->Operands[1]->Reference.Class) &&
                (WalkState->Operands[0]->Reference.Value ==
                 WalkState->Operands[1]->Reference.Value))
            {
                Status = AE_OK;
            }
            else
            {
                ACPI_EXCEPTION ((AE_INFO, Status,
                    "While resolving operands for [%s]",
                    AcpiPsGetOpcodeName (WalkState->Opcode)));
            }
        }

        /* Always delete the argument objects and clear the operand stack */

        AcpiDsClearOperands (WalkState);

        /*
         * If a result object was returned from above, push it on the
         * current result stack
         */
        if (ACPI_SUCCESS (Status) &&
            WalkState->ResultObj)
        {
            Status = AcpiDsResultPush (WalkState->ResultObj, WalkState);
        }
        break;

    default:

        switch (OpType)
        {
        case AML_TYPE_CONTROL:    /* Type 1 opcode, IF/ELSE/WHILE/NOOP */

            /* 1 Operand, 0 ExternalResult, 0 InternalResult */

            Status = AcpiDsExecEndControlOp (WalkState, Op);

            break;

        case AML_TYPE_METHOD_CALL:
            /*
             * If the method is referenced from within a package
             * declaration, it is not a invocation of the method, just
             * a reference to it.
             */
            if ((Op->Asl.Parent) &&
               ((Op->Asl.Parent->Asl.AmlOpcode == AML_PACKAGE_OP) ||
                (Op->Asl.Parent->Asl.AmlOpcode == AML_VARIABLE_PACKAGE_OP)))
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
                    "Method Reference in a Package, Op=%p\n", Op));

                Op->Common.Node = (ACPI_NAMESPACE_NODE *)
                    Op->Asl.Value.Arg->Asl.Node;
                AcpiUtAddReference (Op->Asl.Value.Arg->Asl.Node->Object);
                return_ACPI_STATUS (AE_OK);
            }

            ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
                "Method invocation, Op=%p\n", Op));

            /*
             * (AML_METHODCALL) Op->Asl.Value.Arg->Asl.Node contains
             * the method Node pointer
             */
            /* NextOp points to the op that holds the method name */

            NextOp = FirstArg;

            /* NextOp points to first argument op */

            NextOp = NextOp->Common.Next;

            /*
             * Get the method's arguments and put them on the operand stack
             */
            Status = AcpiDsCreateOperands (WalkState, NextOp);
            if (ACPI_FAILURE (Status))
            {
                break;
            }

            /*
             * Since the operands will be passed to another control method,
             * we must resolve all local references here (Local variables,
             * arguments to *this* method, etc.)
             */
            Status = AcpiDsResolveOperands (WalkState);
            if (ACPI_FAILURE (Status))
            {
                /* On error, clear all resolved operands */

                AcpiDsClearOperands (WalkState);
                break;
            }

            /*
             * Tell the walk loop to preempt this running method and
             * execute the new method
             */
            Status = AE_CTRL_TRANSFER;

            /*
             * Return now; we don't want to disturb anything,
             * especially the operand count!
             */
            return_ACPI_STATUS (Status);

        case AML_TYPE_CREATE_FIELD:

            ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
                "Executing CreateField Buffer/Index Op=%p\n", Op));

            Status = AcpiDsLoad2EndOp (WalkState);
            if (ACPI_FAILURE (Status))
            {
                break;
            }

            Status = AcpiDsEvalBufferFieldOperands (WalkState, Op);
            if (ACPI_FAILURE (Status))
            {
                break;
            }

#ifdef ACPI_EXEC_APP
            /*
             * AcpiExec support for namespace initialization file (initialize
             * BufferFields in this code.)
             */
            Namepath = AcpiNsGetExternalPathname (Op->Common.Node);
            Status = AeLookupInitFileEntry (Namepath, &ObjDesc);
            if (ACPI_SUCCESS (Status))
            {
                Status = AcpiExWriteDataToField (ObjDesc, Op->Common.Node->Object, NULL);
                if (ACPI_FAILURE (Status))
                {
                    ACPI_EXCEPTION ((AE_INFO, Status, "While writing to buffer field"));
                }
            }
            ACPI_FREE (Namepath);
            Status = AE_OK;
#endif
            break;


        case AML_TYPE_CREATE_OBJECT:

            ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
                "Executing CreateObject (Buffer/Package) Op=%p Child=%p ParentOpcode=%4.4X\n",
                Op, Op->Named.Value.Arg, Op->Common.Parent->Common.AmlOpcode));

            switch (Op->Common.Parent->Common.AmlOpcode)
            {
            case AML_NAME_OP:
                /*
                 * Put the Node on the object stack (Contains the ACPI Name
                 * of this object)
                 */
                WalkState->Operands[0] = (void *)
                    Op->Common.Parent->Common.Node;
                WalkState->NumOperands = 1;

                Status = AcpiDsCreateNode (WalkState,
                    Op->Common.Parent->Common.Node, Op->Common.Parent);
                if (ACPI_FAILURE (Status))
                {
                    break;
                }

                ACPI_FALLTHROUGH;

            case AML_INT_EVAL_SUBTREE_OP:

                Status = AcpiDsEvalDataObjectOperands (WalkState, Op,
                    AcpiNsGetAttachedObject (Op->Common.Parent->Common.Node));
                break;

            default:

                Status = AcpiDsEvalDataObjectOperands (WalkState, Op, NULL);
                break;
            }

            /*
             * If a result object was returned from above, push it on the
             * current result stack
             */
            if (WalkState->ResultObj)
            {
                Status = AcpiDsResultPush (WalkState->ResultObj, WalkState);
            }
            break;

        case AML_TYPE_NAMED_FIELD:
        case AML_TYPE_NAMED_COMPLEX:
        case AML_TYPE_NAMED_SIMPLE:
        case AML_TYPE_NAMED_NO_OBJ:

            Status = AcpiDsLoad2EndOp (WalkState);
            if (ACPI_FAILURE (Status))
            {
                break;
            }

            if (Op->Common.AmlOpcode == AML_REGION_OP)
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
                    "Executing OpRegion Address/Length Op=%p\n", Op));

                Status = AcpiDsEvalRegionOperands (WalkState, Op);
                if (ACPI_FAILURE (Status))
                {
                    break;
                }
            }
            else if (Op->Common.AmlOpcode == AML_DATA_REGION_OP)
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
                    "Executing DataTableRegion Strings Op=%p\n", Op));

                Status = AcpiDsEvalTableRegionOperands (WalkState, Op);
                if (ACPI_FAILURE (Status))
                {
                    break;
                }
            }
            else if (Op->Common.AmlOpcode == AML_BANK_FIELD_OP)
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
                    "Executing BankField Op=%p\n", Op));

                Status = AcpiDsEvalBankFieldOperands (WalkState, Op);
                if (ACPI_FAILURE (Status))
                {
                    break;
                }
            }
            break;

        case AML_TYPE_UNDEFINED:

            ACPI_ERROR ((AE_INFO,
                "Undefined opcode type Op=%p", Op));
            return_ACPI_STATUS (AE_NOT_IMPLEMENTED);

        case AML_TYPE_BOGUS:

            ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
                "Internal opcode=%X type Op=%p\n",
                WalkState->Opcode, Op));
            break;

        default:

            ACPI_ERROR ((AE_INFO,
                "Unimplemented opcode, class=0x%X "
                "type=0x%X Opcode=0x%X Op=%p",
                OpClass, OpType, Op->Common.AmlOpcode, Op));

            Status = AE_NOT_IMPLEMENTED;
            break;
        }
    }

    /*
     * ACPI 2.0 support for 64-bit integers: Truncate numeric
     * result value if we are executing from a 32-bit ACPI table
     */
    (void) AcpiExTruncateFor32bitTable (WalkState->ResultObj);

    /*
     * Check if we just completed the evaluation of a
     * conditional predicate
     */
    if ((ACPI_SUCCESS (Status)) &&
        (WalkState->ControlState) &&
        (WalkState->ControlState->Common.State ==
            ACPI_CONTROL_PREDICATE_EXECUTING) &&
        (WalkState->ControlState->Control.PredicateOp == Op))
    {
        Status = AcpiDsGetPredicateValue (WalkState, WalkState->ResultObj);
        WalkState->ResultObj = NULL;
    }


Cleanup:

    if (WalkState->ResultObj)
    {
        /* Break to debugger to display result */

        AcpiDbDisplayResultObject (WalkState->ResultObj,WalkState);

        /*
         * Delete the result op if and only if:
         * Parent will not use the result -- such as any
         * non-nested type2 op in a method (parent will be method)
         */
        AcpiDsDeleteResultIfNotUsed (Op, WalkState->ResultObj, WalkState);
    }

#ifdef _UNDER_DEVELOPMENT

    if (WalkState->ParserState.Aml == WalkState->ParserState.AmlEnd)
    {
        AcpiDbMethodEnd (WalkState);
    }
#endif

    /* Invoke exception handler on error */

    if (ACPI_FAILURE (Status))
    {
        Status = AcpiDsMethodError (Status, WalkState);
    }

    /* Always clear the object stack */

    WalkState->NumOperands = 0;
    return_ACPI_STATUS (Status);
}
