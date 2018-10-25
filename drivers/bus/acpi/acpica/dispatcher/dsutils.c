/*******************************************************************************
 *
 * Module Name: dsutils - Dispatcher utilities
 *
 ******************************************************************************/

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
#include "acparser.h"
#include "amlcode.h"
#include "acdispat.h"
#include "acinterp.h"
#include "acnamesp.h"
#include "acdebug.h"

#define _COMPONENT          ACPI_DISPATCHER
        ACPI_MODULE_NAME    ("dsutils")


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsClearImplicitReturn
 *
 * PARAMETERS:  WalkState           - Current State
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Clear and remove a reference on an implicit return value. Used
 *              to delete "stale" return values (if enabled, the return value
 *              from every operator is saved at least momentarily, in case the
 *              parent method exits.)
 *
 ******************************************************************************/

void
AcpiDsClearImplicitReturn (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_FUNCTION_NAME (DsClearImplicitReturn);


    /*
     * Slack must be enabled for this feature
     */
    if (!AcpiGbl_EnableInterpreterSlack)
    {
        return;
    }

    if (WalkState->ImplicitReturnObj)
    {
        /*
         * Delete any "stale" implicit return. However, in
         * complex statements, the implicit return value can be
         * bubbled up several levels.
         */
        ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
            "Removing reference on stale implicit return obj %p\n",
            WalkState->ImplicitReturnObj));

        AcpiUtRemoveReference (WalkState->ImplicitReturnObj);
        WalkState->ImplicitReturnObj = NULL;
    }
}


#ifndef ACPI_NO_METHOD_EXECUTION
/*******************************************************************************
 *
 * FUNCTION:    AcpiDsDoImplicitReturn
 *
 * PARAMETERS:  ReturnDesc          - The return value
 *              WalkState           - Current State
 *              AddReference        - True if a reference should be added to the
 *                                    return object
 *
 * RETURN:      TRUE if implicit return enabled, FALSE otherwise
 *
 * DESCRIPTION: Implements the optional "implicit return".  We save the result
 *              of every ASL operator and control method invocation in case the
 *              parent method exit. Before storing a new return value, we
 *              delete the previous return value.
 *
 ******************************************************************************/

BOOLEAN
AcpiDsDoImplicitReturn (
    ACPI_OPERAND_OBJECT     *ReturnDesc,
    ACPI_WALK_STATE         *WalkState,
    BOOLEAN                 AddReference)
{
    ACPI_FUNCTION_NAME (DsDoImplicitReturn);


    /*
     * Slack must be enabled for this feature, and we must
     * have a valid return object
     */
    if ((!AcpiGbl_EnableInterpreterSlack) ||
        (!ReturnDesc))
    {
        return (FALSE);
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
        "Result %p will be implicitly returned; Prev=%p\n",
        ReturnDesc,
        WalkState->ImplicitReturnObj));

    /*
     * Delete any "stale" implicit return value first. However, in
     * complex statements, the implicit return value can be
     * bubbled up several levels, so we don't clear the value if it
     * is the same as the ReturnDesc.
     */
    if (WalkState->ImplicitReturnObj)
    {
        if (WalkState->ImplicitReturnObj == ReturnDesc)
        {
            return (TRUE);
        }
        AcpiDsClearImplicitReturn (WalkState);
    }

    /* Save the implicit return value, add a reference if requested */

    WalkState->ImplicitReturnObj = ReturnDesc;
    if (AddReference)
    {
        AcpiUtAddReference (ReturnDesc);
    }

    return (TRUE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsIsResultUsed
 *
 * PARAMETERS:  Op                  - Current Op
 *              WalkState           - Current State
 *
 * RETURN:      TRUE if result is used, FALSE otherwise
 *
 * DESCRIPTION: Check if a result object will be used by the parent
 *
 ******************************************************************************/

BOOLEAN
AcpiDsIsResultUsed (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_WALK_STATE         *WalkState)
{
    const ACPI_OPCODE_INFO  *ParentInfo;

    ACPI_FUNCTION_TRACE_PTR (DsIsResultUsed, Op);


    /* Must have both an Op and a Result Object */

    if (!Op)
    {
        ACPI_ERROR ((AE_INFO, "Null Op"));
        return_UINT8 (TRUE);
    }

    /*
     * We know that this operator is not a
     * Return() operator (would not come here.) The following code is the
     * optional support for a so-called "implicit return". Some AML code
     * assumes that the last value of the method is "implicitly" returned
     * to the caller. Just save the last result as the return value.
     * NOTE: this is optional because the ASL language does not actually
     * support this behavior.
     */
    (void) AcpiDsDoImplicitReturn (WalkState->ResultObj, WalkState, TRUE);

    /*
     * Now determine if the parent will use the result
     *
     * If there is no parent, or the parent is a ScopeOp, we are executing
     * at the method level. An executing method typically has no parent,
     * since each method is parsed separately. A method invoked externally
     * via ExecuteControlMethod has a ScopeOp as the parent.
     */
    if ((!Op->Common.Parent) ||
        (Op->Common.Parent->Common.AmlOpcode == AML_SCOPE_OP))
    {
        /* No parent, the return value cannot possibly be used */

        ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
            "At Method level, result of [%s] not used\n",
            AcpiPsGetOpcodeName (Op->Common.AmlOpcode)));
        return_UINT8 (FALSE);
    }

    /* Get info on the parent. The RootOp is AML_SCOPE */

    ParentInfo = AcpiPsGetOpcodeInfo (Op->Common.Parent->Common.AmlOpcode);
    if (ParentInfo->Class == AML_CLASS_UNKNOWN)
    {
        ACPI_ERROR ((AE_INFO,
            "Unknown parent opcode Op=%p", Op));
        return_UINT8 (FALSE);
    }

    /*
     * Decide what to do with the result based on the parent. If
     * the parent opcode will not use the result, delete the object.
     * Otherwise leave it as is, it will be deleted when it is used
     * as an operand later.
     */
    switch (ParentInfo->Class)
    {
    case AML_CLASS_CONTROL:

        switch (Op->Common.Parent->Common.AmlOpcode)
        {
        case AML_RETURN_OP:

            /* Never delete the return value associated with a return opcode */

            goto ResultUsed;

        case AML_IF_OP:
        case AML_WHILE_OP:
            /*
             * If we are executing the predicate AND this is the predicate op,
             * we will use the return value
             */
            if ((WalkState->ControlState->Common.State ==
                    ACPI_CONTROL_PREDICATE_EXECUTING) &&
                (WalkState->ControlState->Control.PredicateOp == Op))
            {
                goto ResultUsed;
            }
            break;

        default:

            /* Ignore other control opcodes */

            break;
        }

        /* The general control opcode returns no result */

        goto ResultNotUsed;

    case AML_CLASS_CREATE:
        /*
         * These opcodes allow TermArg(s) as operands and therefore
         * the operands can be method calls. The result is used.
         */
        goto ResultUsed;

    case AML_CLASS_NAMED_OBJECT:

        if ((Op->Common.Parent->Common.AmlOpcode == AML_REGION_OP)       ||
            (Op->Common.Parent->Common.AmlOpcode == AML_DATA_REGION_OP)  ||
            (Op->Common.Parent->Common.AmlOpcode == AML_PACKAGE_OP)      ||
            (Op->Common.Parent->Common.AmlOpcode == AML_BUFFER_OP)       ||
            (Op->Common.Parent->Common.AmlOpcode == AML_VARIABLE_PACKAGE_OP) ||
            (Op->Common.Parent->Common.AmlOpcode == AML_INT_EVAL_SUBTREE_OP) ||
            (Op->Common.Parent->Common.AmlOpcode == AML_BANK_FIELD_OP))
        {
            /*
             * These opcodes allow TermArg(s) as operands and therefore
             * the operands can be method calls. The result is used.
             */
            goto ResultUsed;
        }

        goto ResultNotUsed;

    default:
        /*
         * In all other cases. the parent will actually use the return
         * object, so keep it.
         */
        goto ResultUsed;
    }


ResultUsed:
    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
        "Result of [%s] used by Parent [%s] Op=%p\n",
        AcpiPsGetOpcodeName (Op->Common.AmlOpcode),
        AcpiPsGetOpcodeName (Op->Common.Parent->Common.AmlOpcode), Op));

    return_UINT8 (TRUE);


ResultNotUsed:
    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
        "Result of [%s] not used by Parent [%s] Op=%p\n",
        AcpiPsGetOpcodeName (Op->Common.AmlOpcode),
        AcpiPsGetOpcodeName (Op->Common.Parent->Common.AmlOpcode), Op));

    return_UINT8 (FALSE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsDeleteResultIfNotUsed
 *
 * PARAMETERS:  Op              - Current parse Op
 *              ResultObj       - Result of the operation
 *              WalkState       - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Used after interpretation of an opcode. If there is an internal
 *              result descriptor, check if the parent opcode will actually use
 *              this result. If not, delete the result now so that it will
 *              not become orphaned.
 *
 ******************************************************************************/

void
AcpiDsDeleteResultIfNotUsed (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_OPERAND_OBJECT     *ResultObj,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE_PTR (DsDeleteResultIfNotUsed, ResultObj);


    if (!Op)
    {
        ACPI_ERROR ((AE_INFO, "Null Op"));
        return_VOID;
    }

    if (!ResultObj)
    {
        return_VOID;
    }

    if (!AcpiDsIsResultUsed (Op, WalkState))
    {
        /* Must pop the result stack (ObjDesc should be equal to ResultObj) */

        Status = AcpiDsResultPop (&ObjDesc, WalkState);
        if (ACPI_SUCCESS (Status))
        {
            AcpiUtRemoveReference (ResultObj);
        }
    }

    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsResolveOperands
 *
 * PARAMETERS:  WalkState           - Current walk state with operands on stack
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Resolve all operands to their values. Used to prepare
 *              arguments to a control method invocation (a call from one
 *              method to another.)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsResolveOperands (
    ACPI_WALK_STATE         *WalkState)
{
    UINT32                  i;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE_PTR (DsResolveOperands, WalkState);


    /*
     * Attempt to resolve each of the valid operands
     * Method arguments are passed by reference, not by value. This means
     * that the actual objects are passed, not copies of the objects.
     */
    for (i = 0; i < WalkState->NumOperands; i++)
    {
        Status = AcpiExResolveToValue (&WalkState->Operands[i], WalkState);
        if (ACPI_FAILURE (Status))
        {
            break;
        }
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsClearOperands
 *
 * PARAMETERS:  WalkState           - Current walk state with operands on stack
 *
 * RETURN:      None
 *
 * DESCRIPTION: Clear all operands on the current walk state operand stack.
 *
 ******************************************************************************/

void
AcpiDsClearOperands (
    ACPI_WALK_STATE         *WalkState)
{
    UINT32                  i;


    ACPI_FUNCTION_TRACE_PTR (DsClearOperands, WalkState);


    /* Remove a reference on each operand on the stack */

    for (i = 0; i < WalkState->NumOperands; i++)
    {
        /*
         * Remove a reference to all operands, including both
         * "Arguments" and "Targets".
         */
        AcpiUtRemoveReference (WalkState->Operands[i]);
        WalkState->Operands[i] = NULL;
    }

    WalkState->NumOperands = 0;
    return_VOID;
}
#endif


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsCreateOperand
 *
 * PARAMETERS:  WalkState       - Current walk state
 *              Arg             - Parse object for the argument
 *              ArgIndex        - Which argument (zero based)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Translate a parse tree object that is an argument to an AML
 *              opcode to the equivalent interpreter object. This may include
 *              looking up a name or entering a new name into the internal
 *              namespace.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsCreateOperand (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Arg,
    UINT32                  ArgIndex)
{
    ACPI_STATUS             Status = AE_OK;
    char                    *NameString;
    UINT32                  NameLength;
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_PARSE_OBJECT       *ParentOp;
    UINT16                  Opcode;
    ACPI_INTERPRETER_MODE   InterpreterMode;
    const ACPI_OPCODE_INFO  *OpInfo;


    ACPI_FUNCTION_TRACE_PTR (DsCreateOperand, Arg);


    /* A valid name must be looked up in the namespace */

    if ((Arg->Common.AmlOpcode == AML_INT_NAMEPATH_OP) &&
        (Arg->Common.Value.String) &&
        !(Arg->Common.Flags & ACPI_PARSEOP_IN_STACK))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH, "Getting a name: Arg=%p\n", Arg));

        /* Get the entire name string from the AML stream */

        Status = AcpiExGetNameString (ACPI_TYPE_ANY,
            Arg->Common.Value.Buffer, &NameString, &NameLength);

        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        /* All prefixes have been handled, and the name is in NameString */

        /*
         * Special handling for BufferField declarations. This is a deferred
         * opcode that unfortunately defines the field name as the last
         * parameter instead of the first. We get here when we are performing
         * the deferred execution, so the actual name of the field is already
         * in the namespace. We don't want to attempt to look it up again
         * because we may be executing in a different scope than where the
         * actual opcode exists.
         */
        if ((WalkState->DeferredNode) &&
            (WalkState->DeferredNode->Type == ACPI_TYPE_BUFFER_FIELD) &&
            (ArgIndex == (UINT32)
                ((WalkState->Opcode == AML_CREATE_FIELD_OP) ? 3 : 2)))
        {
            ObjDesc = ACPI_CAST_PTR (
                ACPI_OPERAND_OBJECT, WalkState->DeferredNode);
            Status = AE_OK;
        }
        else    /* All other opcodes */
        {
            /*
             * Differentiate between a namespace "create" operation
             * versus a "lookup" operation (IMODE_LOAD_PASS2 vs.
             * IMODE_EXECUTE) in order to support the creation of
             * namespace objects during the execution of control methods.
             */
            ParentOp = Arg->Common.Parent;
            OpInfo = AcpiPsGetOpcodeInfo (ParentOp->Common.AmlOpcode);

            if ((OpInfo->Flags & AML_NSNODE) &&
                (ParentOp->Common.AmlOpcode != AML_INT_METHODCALL_OP) &&
                (ParentOp->Common.AmlOpcode != AML_REGION_OP) &&
                (ParentOp->Common.AmlOpcode != AML_INT_NAMEPATH_OP))
            {
                /* Enter name into namespace if not found */

                InterpreterMode = ACPI_IMODE_LOAD_PASS2;
            }
            else
            {
                /* Return a failure if name not found */

                InterpreterMode = ACPI_IMODE_EXECUTE;
            }

            Status = AcpiNsLookup (WalkState->ScopeInfo, NameString,
                ACPI_TYPE_ANY, InterpreterMode,
                ACPI_NS_SEARCH_PARENT | ACPI_NS_DONT_OPEN_SCOPE, WalkState,
                ACPI_CAST_INDIRECT_PTR (ACPI_NAMESPACE_NODE, &ObjDesc));
            /*
             * The only case where we pass through (ignore) a NOT_FOUND
             * error is for the CondRefOf opcode.
             */
            if (Status == AE_NOT_FOUND)
            {
                if (ParentOp->Common.AmlOpcode == AML_CONDITIONAL_REF_OF_OP)
                {
                    /*
                     * For the Conditional Reference op, it's OK if
                     * the name is not found;  We just need a way to
                     * indicate this to the interpreter, set the
                     * object to the root
                     */
                    ObjDesc = ACPI_CAST_PTR (
                        ACPI_OPERAND_OBJECT, AcpiGbl_RootNode);
                    Status = AE_OK;
                }
                else if (ParentOp->Common.AmlOpcode == AML_EXTERNAL_OP)
                {
                    /*
                     * This opcode should never appear here. It is used only
                     * by AML disassemblers and is surrounded by an If(0)
                     * by the ASL compiler.
                     *
                     * Therefore, if we see it here, it is a serious error.
                     */
                    Status = AE_AML_BAD_OPCODE;
                }
                else
                {
                    /*
                     * We just plain didn't find it -- which is a
                     * very serious error at this point
                     */
                    Status = AE_AML_NAME_NOT_FOUND;
                }
            }

            if (ACPI_FAILURE (Status))
            {
                ACPI_ERROR_NAMESPACE (WalkState->ScopeInfo,
                    NameString, Status);
            }
        }

        /* Free the namestring created above */

        ACPI_FREE (NameString);

        /* Check status from the lookup */

        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        /* Put the resulting object onto the current object stack */

        Status = AcpiDsObjStackPush (ObjDesc, WalkState);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        AcpiDbDisplayArgumentObject (ObjDesc, WalkState);
    }
    else
    {
        /* Check for null name case */

        if ((Arg->Common.AmlOpcode == AML_INT_NAMEPATH_OP) &&
            !(Arg->Common.Flags & ACPI_PARSEOP_IN_STACK))
        {
            /*
             * If the name is null, this means that this is an
             * optional result parameter that was not specified
             * in the original ASL. Create a Zero Constant for a
             * placeholder. (Store to a constant is a Noop.)
             */
            Opcode = AML_ZERO_OP;       /* Has no arguments! */

            ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
                "Null namepath: Arg=%p\n", Arg));
        }
        else
        {
            Opcode = Arg->Common.AmlOpcode;
        }

        /* Get the object type of the argument */

        OpInfo = AcpiPsGetOpcodeInfo (Opcode);
        if (OpInfo->ObjectType == ACPI_TYPE_INVALID)
        {
            return_ACPI_STATUS (AE_NOT_IMPLEMENTED);
        }

        if ((OpInfo->Flags & AML_HAS_RETVAL) ||
            (Arg->Common.Flags & ACPI_PARSEOP_IN_STACK))
        {
            /*
             * Use value that was already previously returned
             * by the evaluation of this argument
             */
            Status = AcpiDsResultPop (&ObjDesc, WalkState);
            if (ACPI_FAILURE (Status))
            {
                /*
                 * Only error is underflow, and this indicates
                 * a missing or null operand!
                 */
                ACPI_EXCEPTION ((AE_INFO, Status,
                    "Missing or null operand"));
                return_ACPI_STATUS (Status);
            }
        }
        else
        {
            /* Create an ACPI_INTERNAL_OBJECT for the argument */

            ObjDesc = AcpiUtCreateInternalObject (OpInfo->ObjectType);
            if (!ObjDesc)
            {
                return_ACPI_STATUS (AE_NO_MEMORY);
            }

            /* Initialize the new object */

            Status = AcpiDsInitObjectFromOp (
                WalkState, Arg, Opcode, &ObjDesc);
            if (ACPI_FAILURE (Status))
            {
                AcpiUtDeleteObjectDesc (ObjDesc);
                return_ACPI_STATUS (Status);
            }
        }

        /* Put the operand object on the object stack */

        Status = AcpiDsObjStackPush (ObjDesc, WalkState);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        AcpiDbDisplayArgumentObject (ObjDesc, WalkState);
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiDsCreateOperands
 *
 * PARAMETERS:  WalkState           - Current state
 *              FirstArg            - First argument of a parser argument tree
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert an operator's arguments from a parse tree format to
 *              namespace objects and place those argument object on the object
 *              stack in preparation for evaluation by the interpreter.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiDsCreateOperands (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *FirstArg)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_PARSE_OBJECT       *Arg;
    ACPI_PARSE_OBJECT       *Arguments[ACPI_OBJ_NUM_OPERANDS];
    UINT32                  ArgCount = 0;
    UINT32                  Index = WalkState->NumOperands;
    UINT32                  i;


    ACPI_FUNCTION_TRACE_PTR (DsCreateOperands, FirstArg);


    /* Get all arguments in the list */

    Arg = FirstArg;
    while (Arg)
    {
        if (Index >= ACPI_OBJ_NUM_OPERANDS)
        {
            return_ACPI_STATUS (AE_BAD_DATA);
        }

        Arguments[Index] = Arg;
        WalkState->Operands [Index] = NULL;

        /* Move on to next argument, if any */

        Arg = Arg->Common.Next;
        ArgCount++;
        Index++;
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
        "NumOperands %d, ArgCount %d, Index %d\n",
        WalkState->NumOperands, ArgCount, Index));

    /* Create the interpreter arguments, in reverse order */

    Index--;
    for (i = 0; i < ArgCount; i++)
    {
        Arg = Arguments[Index];
        WalkState->OperandIndex = (UINT8) Index;

        Status = AcpiDsCreateOperand (WalkState, Arg, Index);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH,
            "Created Arg #%u (%p) %u args total\n",
            Index, Arg, ArgCount));
        Index--;
    }

    return_ACPI_STATUS (Status);


Cleanup:
    /*
     * We must undo everything done above; meaning that we must
     * pop everything off of the operand stack and delete those
     * objects
     */
    AcpiDsObjStackPopAndDelete (ArgCount, WalkState);

    ACPI_EXCEPTION ((AE_INFO, Status, "While creating Arg %u", Index));
    return_ACPI_STATUS (Status);
}


/*****************************************************************************
 *
 * FUNCTION:    AcpiDsEvaluateNamePath
 *
 * PARAMETERS:  WalkState       - Current state of the parse tree walk,
 *                                the opcode of current operation should be
 *                                AML_INT_NAMEPATH_OP
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Translate the -NamePath- parse tree object to the equivalent
 *              interpreter object, convert it to value, if needed, duplicate
 *              it, if needed, and push it onto the current result stack.
 *
 ****************************************************************************/

ACPI_STATUS
AcpiDsEvaluateNamePath (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_PARSE_OBJECT       *Op = WalkState->Op;
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_OPERAND_OBJECT     *NewObjDesc;
    UINT8                   Type;


    ACPI_FUNCTION_TRACE_PTR (DsEvaluateNamePath, WalkState);


    if (!Op->Common.Parent)
    {
        /* This happens after certain exception processing */

        goto Exit;
    }

    if ((Op->Common.Parent->Common.AmlOpcode == AML_PACKAGE_OP) ||
        (Op->Common.Parent->Common.AmlOpcode == AML_VARIABLE_PACKAGE_OP) ||
        (Op->Common.Parent->Common.AmlOpcode == AML_REF_OF_OP))
    {
        /* TBD: Should we specify this feature as a bit of OpInfo->Flags of these opcodes? */

        goto Exit;
    }

    Status = AcpiDsCreateOperand (WalkState, Op, 0);
    if (ACPI_FAILURE (Status))
    {
        goto Exit;
    }

    if (Op->Common.Flags & ACPI_PARSEOP_TARGET)
    {
        NewObjDesc = *Operand;
        goto PushResult;
    }

    Type = (*Operand)->Common.Type;

    Status = AcpiExResolveToValue (Operand, WalkState);
    if (ACPI_FAILURE (Status))
    {
        goto Exit;
    }

    if (Type == ACPI_TYPE_INTEGER)
    {
        /* It was incremented by AcpiExResolveToValue */

        AcpiUtRemoveReference (*Operand);

        Status = AcpiUtCopyIobjectToIobject (
            *Operand, &NewObjDesc, WalkState);
        if (ACPI_FAILURE (Status))
        {
            goto Exit;
        }
    }
    else
    {
        /*
         * The object either was anew created or is
         * a Namespace node - don't decrement it.
         */
        NewObjDesc = *Operand;
    }

    /* Cleanup for name-path operand */

    Status = AcpiDsObjStackPop (1, WalkState);
    if (ACPI_FAILURE (Status))
    {
        WalkState->ResultObj = NewObjDesc;
        goto Exit;
    }

PushResult:

    WalkState->ResultObj = NewObjDesc;

    Status = AcpiDsResultPush (WalkState->ResultObj, WalkState);
    if (ACPI_SUCCESS (Status))
    {
        /* Force to take it from stack */

        Op->Common.Flags |= ACPI_PARSEOP_IN_STACK;
    }

Exit:

    return_ACPI_STATUS (Status);
}
