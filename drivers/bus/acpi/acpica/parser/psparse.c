/******************************************************************************
 *
 * Module Name: psparse - Parser top level AML parse routines
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2020, Intel Corp.
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

/*
 * Parse the AML and build an operation tree as most interpreters,
 * like Perl, do. Parsing is done by hand rather than with a YACC
 * generated parser to tightly constrain stack and dynamic memory
 * usage. At the same time, parsing is kept flexible and the code
 * fairly compact by parsing based on a list of AML opcode
 * templates in AmlOpInfo[]
 */

#include "acpi.h"
#include "accommon.h"
#include "acparser.h"
#include "acdispat.h"
#include "amlcode.h"
#include "acinterp.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_PARSER
        ACPI_MODULE_NAME    ("psparse")


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetOpcodeSize
 *
 * PARAMETERS:  Opcode          - An AML opcode
 *
 * RETURN:      Size of the opcode, in bytes (1 or 2)
 *
 * DESCRIPTION: Get the size of the current opcode.
 *
 ******************************************************************************/

UINT32
AcpiPsGetOpcodeSize (
    UINT32                  Opcode)
{

    /* Extended (2-byte) opcode if > 255 */

    if (Opcode > 0x00FF)
    {
        return (2);
    }

    /* Otherwise, just a single byte opcode */

    return (1);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsPeekOpcode
 *
 * PARAMETERS:  ParserState         - A parser state object
 *
 * RETURN:      Next AML opcode
 *
 * DESCRIPTION: Get next AML opcode (without incrementing AML pointer)
 *
 ******************************************************************************/

UINT16
AcpiPsPeekOpcode (
    ACPI_PARSE_STATE        *ParserState)
{
    UINT8                   *Aml;
    UINT16                  Opcode;


    Aml = ParserState->Aml;
    Opcode = (UINT16) ACPI_GET8 (Aml);

    if (Opcode == AML_EXTENDED_PREFIX)
    {
        /* Extended opcode, get the second opcode byte */

        Aml++;
        Opcode = (UINT16) ((Opcode << 8) | ACPI_GET8 (Aml));
    }

    return (Opcode);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsCompleteThisOp
 *
 * PARAMETERS:  WalkState       - Current State
 *              Op              - Op to complete
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform any cleanup at the completion of an Op.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsCompleteThisOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_PARSE_OBJECT       *Prev;
    ACPI_PARSE_OBJECT       *Next;
    const ACPI_OPCODE_INFO  *ParentInfo;
    ACPI_PARSE_OBJECT       *ReplacementOp = NULL;
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE_PTR (PsCompleteThisOp, Op);


    /* Check for null Op, can happen if AML code is corrupt */

    if (!Op)
    {
        return_ACPI_STATUS (AE_OK);  /* OK for now */
    }

    AcpiExStopTraceOpcode (Op, WalkState);

    /* Delete this op and the subtree below it if asked to */

    if (((WalkState->ParseFlags & ACPI_PARSE_TREE_MASK) != ACPI_PARSE_DELETE_TREE) ||
         (WalkState->OpInfo->Class == AML_CLASS_ARGUMENT))
    {
        return_ACPI_STATUS (AE_OK);
    }

    /* Make sure that we only delete this subtree */

    if (Op->Common.Parent)
    {
        Prev = Op->Common.Parent->Common.Value.Arg;
        if (!Prev)
        {
            /* Nothing more to do */

            goto Cleanup;
        }

        /*
         * Check if we need to replace the operator and its subtree
         * with a return value op (placeholder op)
         */
        ParentInfo = AcpiPsGetOpcodeInfo (Op->Common.Parent->Common.AmlOpcode);

        switch (ParentInfo->Class)
        {
        case AML_CLASS_CONTROL:

            break;

        case AML_CLASS_CREATE:
            /*
             * These opcodes contain TermArg operands. The current
             * op must be replaced by a placeholder return op
             */
            ReplacementOp = AcpiPsAllocOp (
                AML_INT_RETURN_VALUE_OP, Op->Common.Aml);
            if (!ReplacementOp)
            {
                Status = AE_NO_MEMORY;
            }
            break;

        case AML_CLASS_NAMED_OBJECT:
            /*
             * These opcodes contain TermArg operands. The current
             * op must be replaced by a placeholder return op
             */
            if ((Op->Common.Parent->Common.AmlOpcode == AML_REGION_OP)       ||
                (Op->Common.Parent->Common.AmlOpcode == AML_DATA_REGION_OP)  ||
                (Op->Common.Parent->Common.AmlOpcode == AML_BUFFER_OP)       ||
                (Op->Common.Parent->Common.AmlOpcode == AML_PACKAGE_OP)      ||
                (Op->Common.Parent->Common.AmlOpcode == AML_BANK_FIELD_OP)   ||
                (Op->Common.Parent->Common.AmlOpcode == AML_VARIABLE_PACKAGE_OP))
            {
                ReplacementOp = AcpiPsAllocOp (
                    AML_INT_RETURN_VALUE_OP, Op->Common.Aml);
                if (!ReplacementOp)
                {
                    Status = AE_NO_MEMORY;
                }
            }
            else if ((Op->Common.Parent->Common.AmlOpcode == AML_NAME_OP) &&
                     (WalkState->PassNumber <= ACPI_IMODE_LOAD_PASS2))
            {
                if ((Op->Common.AmlOpcode == AML_BUFFER_OP) ||
                    (Op->Common.AmlOpcode == AML_PACKAGE_OP) ||
                    (Op->Common.AmlOpcode == AML_VARIABLE_PACKAGE_OP))
                {
                    ReplacementOp = AcpiPsAllocOp (Op->Common.AmlOpcode,
                        Op->Common.Aml);
                    if (!ReplacementOp)
                    {
                        Status = AE_NO_MEMORY;
                    }
                    else
                    {
                        ReplacementOp->Named.Data = Op->Named.Data;
                        ReplacementOp->Named.Length = Op->Named.Length;
                    }
                }
            }
            break;

        default:

            ReplacementOp = AcpiPsAllocOp (
                AML_INT_RETURN_VALUE_OP, Op->Common.Aml);
            if (!ReplacementOp)
            {
                Status = AE_NO_MEMORY;
            }
        }

        /* We must unlink this op from the parent tree */

        if (Prev == Op)
        {
            /* This op is the first in the list */

            if (ReplacementOp)
            {
                ReplacementOp->Common.Parent = Op->Common.Parent;
                ReplacementOp->Common.Value.Arg = NULL;
                ReplacementOp->Common.Node = Op->Common.Node;
                Op->Common.Parent->Common.Value.Arg = ReplacementOp;
                ReplacementOp->Common.Next = Op->Common.Next;
            }
            else
            {
                Op->Common.Parent->Common.Value.Arg = Op->Common.Next;
            }
        }

        /* Search the parent list */

        else while (Prev)
        {
            /* Traverse all siblings in the parent's argument list */

            Next = Prev->Common.Next;
            if (Next == Op)
            {
                if (ReplacementOp)
                {
                    ReplacementOp->Common.Parent = Op->Common.Parent;
                    ReplacementOp->Common.Value.Arg = NULL;
                    ReplacementOp->Common.Node = Op->Common.Node;
                    Prev->Common.Next = ReplacementOp;
                    ReplacementOp->Common.Next = Op->Common.Next;
                    Next = NULL;
                }
                else
                {
                    Prev->Common.Next = Op->Common.Next;
                    Next = NULL;
                }
            }
            Prev = Next;
        }
    }


Cleanup:

    /* Now we can actually delete the subtree rooted at Op */

    AcpiPsDeleteParseTree (Op);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsNextParseState
 *
 * PARAMETERS:  WalkState           - Current state
 *              Op                  - Current parse op
 *              CallbackStatus      - Status from previous operation
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Update the parser state based upon the return exception from
 *              the parser callback.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsNextParseState (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    ACPI_STATUS             CallbackStatus)
{
    ACPI_PARSE_STATE        *ParserState = &WalkState->ParserState;
    ACPI_STATUS             Status = AE_CTRL_PENDING;


    ACPI_FUNCTION_TRACE_PTR (PsNextParseState, Op);


    switch (CallbackStatus)
    {
    case AE_CTRL_TERMINATE:
        /*
         * A control method was terminated via a RETURN statement.
         * The walk of this method is complete.
         */
        ParserState->Aml = ParserState->AmlEnd;
        Status = AE_CTRL_TERMINATE;
        break;

    case AE_CTRL_BREAK:

        ParserState->Aml = WalkState->AmlLastWhile;
        WalkState->ControlState->Common.Value = FALSE;
        Status = AE_CTRL_BREAK;
        break;

    case AE_CTRL_CONTINUE:

        ParserState->Aml = WalkState->AmlLastWhile;
        Status = AE_CTRL_CONTINUE;
        break;

    case AE_CTRL_PENDING:

        ParserState->Aml = WalkState->AmlLastWhile;
        break;

#if 0
    case AE_CTRL_SKIP:

        ParserState->Aml = ParserState->Scope->ParseScope.PkgEnd;
        Status = AE_OK;
        break;
#endif

    case AE_CTRL_TRUE:
        /*
         * Predicate of an IF was true, and we are at the matching ELSE.
         * Just close out this package
         */
        ParserState->Aml = AcpiPsGetNextPackageEnd (ParserState);
        Status = AE_CTRL_PENDING;
        break;

    case AE_CTRL_FALSE:
        /*
         * Either an IF/WHILE Predicate was false or we encountered a BREAK
         * opcode. In both cases, we do not execute the rest of the
         * package;  We simply close out the parent (finishing the walk of
         * this branch of the tree) and continue execution at the parent
         * level.
         */
        ParserState->Aml = ParserState->Scope->ParseScope.PkgEnd;

        /* In the case of a BREAK, just force a predicate (if any) to FALSE */

        WalkState->ControlState->Common.Value = FALSE;
        Status = AE_CTRL_END;
        break;

    case AE_CTRL_TRANSFER:

        /* A method call (invocation) -- transfer control */

        Status = AE_CTRL_TRANSFER;
        WalkState->PrevOp = Op;
        WalkState->MethodCallOp = Op;
        WalkState->MethodCallNode = (Op->Common.Value.Arg)->Common.Node;

        /* Will return value (if any) be used by the caller? */

        WalkState->ReturnUsed = AcpiDsIsResultUsed (Op, WalkState);
        break;

    default:

        Status = CallbackStatus;
        if ((CallbackStatus & AE_CODE_MASK) == AE_CODE_CONTROL)
        {
            Status = AE_OK;
        }
        break;
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsParseAml
 *
 * PARAMETERS:  WalkState       - Current state
 *
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Parse raw AML and return a tree of ops
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsParseAml (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status;
    ACPI_THREAD_STATE       *Thread;
    ACPI_THREAD_STATE       *PrevWalkList = AcpiGbl_CurrentWalkList;
    ACPI_WALK_STATE         *PreviousWalkState;


    ACPI_FUNCTION_TRACE (PsParseAml);

    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
        "Entered with WalkState=%p Aml=%p size=%X\n",
        WalkState, WalkState->ParserState.Aml,
        WalkState->ParserState.AmlSize));

    if (!WalkState->ParserState.Aml)
    {
        return_ACPI_STATUS (AE_BAD_ADDRESS);
    }

    /* Create and initialize a new thread state */

    Thread = AcpiUtCreateThreadState ();
    if (!Thread)
    {
        if (WalkState->MethodDesc)
        {
            /* Executing a control method - additional cleanup */

            AcpiDsTerminateControlMethod (WalkState->MethodDesc, WalkState);
        }

        AcpiDsDeleteWalkState (WalkState);
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    WalkState->Thread = Thread;

    /*
     * If executing a method, the starting SyncLevel is this method's
     * SyncLevel
     */
    if (WalkState->MethodDesc)
    {
        WalkState->Thread->CurrentSyncLevel =
            WalkState->MethodDesc->Method.SyncLevel;
    }

    AcpiDsPushWalkState (WalkState, Thread);

    /*
     * This global allows the AML debugger to get a handle to the currently
     * executing control method.
     */
    AcpiGbl_CurrentWalkList = Thread;

    /*
     * Execute the walk loop as long as there is a valid Walk State. This
     * handles nested control method invocations without recursion.
     */
    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE, "State=%p\n", WalkState));

    Status = AE_OK;
    while (WalkState)
    {
        if (ACPI_SUCCESS (Status))
        {
            /*
             * The ParseLoop executes AML until the method terminates
             * or calls another method.
             */
            Status = AcpiPsParseLoop (WalkState);
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
            "Completed one call to walk loop, %s State=%p\n",
            AcpiFormatException (Status), WalkState));

        if (WalkState->MethodPathname && WalkState->MethodIsNested)
        {
            /* Optional object evaluation log */

            ACPI_DEBUG_PRINT_RAW ((ACPI_DB_EVALUATION, "%-26s:  %*s%s\n",
                "   Exit nested method",
                (WalkState->MethodNestingDepth + 1) * 3, " ",
                &WalkState->MethodPathname[1]));

            ACPI_FREE (WalkState->MethodPathname);
            WalkState->MethodIsNested = FALSE;
        }
        if (Status == AE_CTRL_TRANSFER)
        {
            /*
             * A method call was detected.
             * Transfer control to the called control method
             */
            Status = AcpiDsCallControlMethod (Thread, WalkState, NULL);
            if (ACPI_FAILURE (Status))
            {
                Status = AcpiDsMethodError (Status, WalkState);
            }

            /*
             * If the transfer to the new method method call worked,
             * a new walk state was created -- get it
             */
            WalkState = AcpiDsGetCurrentWalkState (Thread);
            continue;
        }
        else if (Status == AE_CTRL_TERMINATE)
        {
            Status = AE_OK;
        }
        else if ((Status != AE_OK) && (WalkState->MethodDesc))
        {
            /* Either the method parse or actual execution failed */

            AcpiExExitInterpreter ();
            if (Status == AE_ABORT_METHOD)
            {
                AcpiNsPrintNodePathname (
                    WalkState->MethodNode, "Aborting method");
                AcpiOsPrintf ("\n");
            }
            else
            {
                ACPI_ERROR_METHOD ("Aborting method",
                    WalkState->MethodNode, NULL, Status);
            }
            AcpiExEnterInterpreter ();

            /* Check for possible multi-thread reentrancy problem */

            if ((Status == AE_ALREADY_EXISTS) &&
                (!(WalkState->MethodDesc->Method.InfoFlags &
                    ACPI_METHOD_SERIALIZED)))
            {
                /*
                 * Method is not serialized and tried to create an object
                 * twice. The probable cause is that the method cannot
                 * handle reentrancy. Mark as "pending serialized" now, and
                 * then mark "serialized" when the last thread exits.
                 */
                WalkState->MethodDesc->Method.InfoFlags |=
                    ACPI_METHOD_SERIALIZED_PENDING;
            }
        }

        /* We are done with this walk, move on to the parent if any */

        WalkState = AcpiDsPopWalkState (Thread);

        /* Reset the current scope to the beginning of scope stack */

        AcpiDsScopeStackClear (WalkState);

        /*
         * If we just returned from the execution of a control method or if we
         * encountered an error during the method parse phase, there's lots of
         * cleanup to do
         */
        if (((WalkState->ParseFlags & ACPI_PARSE_MODE_MASK) ==
            ACPI_PARSE_EXECUTE &&
            !(WalkState->ParseFlags & ACPI_PARSE_MODULE_LEVEL)) ||
            (ACPI_FAILURE (Status)))
        {
            AcpiDsTerminateControlMethod (WalkState->MethodDesc, WalkState);
        }

        /* Delete this walk state and all linked control states */

        AcpiPsCleanupScope (&WalkState->ParserState);
        PreviousWalkState = WalkState;

        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
            "ReturnValue=%p, ImplicitValue=%p State=%p\n",
            WalkState->ReturnDesc, WalkState->ImplicitReturnObj, WalkState));

        /* Check if we have restarted a preempted walk */

        WalkState = AcpiDsGetCurrentWalkState (Thread);
        if (WalkState)
        {
            if (ACPI_SUCCESS (Status))
            {
                /*
                 * There is another walk state, restart it.
                 * If the method return value is not used by the parent,
                 * The object is deleted
                 */
                if (!PreviousWalkState->ReturnDesc)
                {
                    /*
                     * In slack mode execution, if there is no return value
                     * we should implicitly return zero (0) as a default value.
                     */
                    if (AcpiGbl_EnableInterpreterSlack &&
                        !PreviousWalkState->ImplicitReturnObj)
                    {
                        PreviousWalkState->ImplicitReturnObj =
                            AcpiUtCreateIntegerObject ((UINT64) 0);
                        if (!PreviousWalkState->ImplicitReturnObj)
                        {
                            return_ACPI_STATUS (AE_NO_MEMORY);
                        }
                    }

                    /* Restart the calling control method */

                    Status = AcpiDsRestartControlMethod (WalkState,
                        PreviousWalkState->ImplicitReturnObj);
                }
                else
                {
                    /*
                     * We have a valid return value, delete any implicit
                     * return value.
                     */
                    AcpiDsClearImplicitReturn (PreviousWalkState);

                    Status = AcpiDsRestartControlMethod (WalkState,
                        PreviousWalkState->ReturnDesc);
                }
                if (ACPI_SUCCESS (Status))
                {
                    WalkState->WalkType |= ACPI_WALK_METHOD_RESTART;
                }
            }
            else
            {
                /* On error, delete any return object or implicit return */

                AcpiUtRemoveReference (PreviousWalkState->ReturnDesc);
                AcpiDsClearImplicitReturn (PreviousWalkState);
            }
        }

        /*
         * Just completed a 1st-level method, save the final internal return
         * value (if any)
         */
        else if (PreviousWalkState->CallerReturnDesc)
        {
            if (PreviousWalkState->ImplicitReturnObj)
            {
                *(PreviousWalkState->CallerReturnDesc) =
                    PreviousWalkState->ImplicitReturnObj;
            }
            else
            {
                 /* NULL if no return value */

                *(PreviousWalkState->CallerReturnDesc) =
                    PreviousWalkState->ReturnDesc;
            }
        }
        else
        {
            if (PreviousWalkState->ReturnDesc)
            {
                /* Caller doesn't want it, must delete it */

                AcpiUtRemoveReference (PreviousWalkState->ReturnDesc);
            }
            if (PreviousWalkState->ImplicitReturnObj)
            {
                /* Caller doesn't want it, must delete it */

                AcpiUtRemoveReference (PreviousWalkState->ImplicitReturnObj);
            }
        }

        AcpiDsDeleteWalkState (PreviousWalkState);
    }

    /* Normal exit */

    AcpiExReleaseAllMutexes (Thread);
    AcpiUtDeleteGenericState (ACPI_CAST_PTR (ACPI_GENERIC_STATE, Thread));
    AcpiGbl_CurrentWalkList = PrevWalkList;
    return_ACPI_STATUS (Status);
}
