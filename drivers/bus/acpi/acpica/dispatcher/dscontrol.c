/******************************************************************************
 *
 * Module Name: dscontrol - Support for execution control opcodes -
 *                          if/else/while/return
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
#include "amlcode.h"
#include "acdispat.h"
#include "acinterp.h"
#include "acdebug.h"

#define _COMPONENT          ACPI_DISPATCHER
        ACPI_MODULE_NAME    ("dscontrol")


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


    ACPI_DEBUG_PRINT ((ACPI_DB_DISPATCH, "Op=%p Opcode=%2.2X State=%p\n",
        Op, Op->Common.AmlOpcode, WalkState));

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

                WalkState->ControlState->Common.State =
                    ACPI_CONTROL_CONDITIONAL_EXECUTING;
                break;
            }
        }

        ACPI_FALLTHROUGH;

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
        ControlState->Control.AmlPredicateStart =
            WalkState->ParserState.Aml - 1;
        ControlState->Control.PackageEnd =
            WalkState->ParserState.PkgEnd;
        ControlState->Control.Opcode =
            Op->Common.AmlOpcode;
        ControlState->Control.LoopTimeout = AcpiOsGetTimer () +
           ((UINT64) AcpiGbl_MaxLoopIterations * ACPI_100NSEC_PER_SEC);

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
             * This infinite loop detection mechanism allows the interpreter
             * to escape possibly infinite loops. This can occur in poorly
             * written AML when the hardware does not respond within a while
             * loop and the loop does not implement a timeout.
             */
            if (ACPI_TIME_AFTER (AcpiOsGetTimer (),
                    ControlState->Control.LoopTimeout))
            {
                Status = AE_AML_LOOP_TIMEOUT;
                break;
            }

            /*
             * Go back and evaluate the predicate and maybe execute the loop
             * another time
             */
            Status = AE_CTRL_PENDING;
            WalkState->AmlLastWhile =
                ControlState->Control.AmlPredicateStart;
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
            Status = AcpiExResolveToValue (
                &WalkState->Operands [0], WalkState);
            if (ACPI_FAILURE (Status))
            {
                return (Status);
            }

            /*
             * Get the return value and save as the last result
             * value. This is the only place where WalkState->ReturnDesc
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
             * Allow references created by the Index operator to return
             * unchanged.
             */
            if ((ACPI_GET_DESCRIPTOR_TYPE (WalkState->Results->Results.ObjDesc[0]) ==
                    ACPI_DESC_TYPE_OPERAND) &&
                ((WalkState->Results->Results.ObjDesc [0])->Common.Type ==
                    ACPI_TYPE_LOCAL_REFERENCE) &&
                ((WalkState->Results->Results.ObjDesc [0])->Reference.Class !=
                    ACPI_REFCLASS_INDEX))
            {
                Status = AcpiExResolveToValue (
                    &WalkState->Results->Results.ObjDesc [0], WalkState);
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

            WalkState->Operands[0] = NULL;
            WalkState->NumOperands = 0;
            WalkState->ReturnDesc = NULL;
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

    case AML_BREAKPOINT_OP:

        AcpiDbSignalBreakPoint (WalkState);

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

        WalkState->AmlLastWhile =
            WalkState->ControlState->Control.PackageEnd;

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

        ACPI_ERROR ((AE_INFO, "Unknown control opcode=0x%X Op=%p",
            Op->Common.AmlOpcode, Op));

        Status = AE_AML_BAD_OPCODE;
        break;
    }

    return (Status);
}
