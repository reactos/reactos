/******************************************************************************
 *
 * Module Name: psloop - Main AML parse loop
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2022, Intel Corp.
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

/*
 * Parse the AML and build an operation tree as most interpreters, (such as
 * Perl) do. Parsing is done by hand rather than with a YACC generated parser
 * to tightly constrain stack and dynamic memory usage. Parsing is kept
 * flexible and the code fairly compact by parsing based on a list of AML
 * opcode templates in AmlOpInfo[].
 */

#include "acpi.h"
#include "accommon.h"
#include "acinterp.h"
#include "acparser.h"
#include "acdispat.h"
#include "amlcode.h"
#include "acconvert.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_PARSER
        ACPI_MODULE_NAME    ("psloop")


/* Local prototypes */

static ACPI_STATUS
AcpiPsGetArguments (
    ACPI_WALK_STATE         *WalkState,
    UINT8                   *AmlOpStart,
    ACPI_PARSE_OBJECT       *Op);


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetArguments
 *
 * PARAMETERS:  WalkState           - Current state
 *              AmlOpStart          - Op start in AML
 *              Op                  - Current Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get arguments for passed Op.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiPsGetArguments (
    ACPI_WALK_STATE         *WalkState,
    UINT8                   *AmlOpStart,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_PARSE_OBJECT       *Arg = NULL;


    ACPI_FUNCTION_TRACE_PTR (PsGetArguments, WalkState);


    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
        "Get arguments for opcode [%s]\n", Op->Common.AmlOpName));

    switch (Op->Common.AmlOpcode)
    {
    case AML_BYTE_OP:       /* AML_BYTEDATA_ARG */
    case AML_WORD_OP:       /* AML_WORDDATA_ARG */
    case AML_DWORD_OP:      /* AML_DWORDATA_ARG */
    case AML_QWORD_OP:      /* AML_QWORDATA_ARG */
    case AML_STRING_OP:     /* AML_ASCIICHARLIST_ARG */

        /* Fill in constant or string argument directly */

        AcpiPsGetNextSimpleArg (&(WalkState->ParserState),
            GET_CURRENT_ARG_TYPE (WalkState->ArgTypes), Op);
        break;

    case AML_INT_NAMEPATH_OP:   /* AML_NAMESTRING_ARG */

        Status = AcpiPsGetNextNamepath (WalkState,
            &(WalkState->ParserState), Op, ACPI_POSSIBLE_METHOD_CALL);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        WalkState->ArgTypes = 0;
        break;

    default:
        /*
         * Op is not a constant or string, append each argument to the Op
         */
        while (GET_CURRENT_ARG_TYPE (WalkState->ArgTypes) &&
            !WalkState->ArgCount)
        {
            WalkState->Aml = WalkState->ParserState.Aml;

            switch (Op->Common.AmlOpcode)
            {
            case AML_METHOD_OP:
            case AML_BUFFER_OP:
            case AML_PACKAGE_OP:
            case AML_VARIABLE_PACKAGE_OP:
            case AML_WHILE_OP:

                break;

            default:

                ASL_CV_CAPTURE_COMMENTS (WalkState);
                break;
            }

            Status = AcpiPsGetNextArg (WalkState, &(WalkState->ParserState),
                GET_CURRENT_ARG_TYPE (WalkState->ArgTypes), &Arg);
            if (ACPI_FAILURE (Status))
            {
                return_ACPI_STATUS (Status);
            }

            if (Arg)
            {
                AcpiPsAppendArg (Op, Arg);
            }

            INCREMENT_ARG_LIST (WalkState->ArgTypes);
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
            "Final argument count: %8.8X pass %u\n",
            WalkState->ArgCount, WalkState->PassNumber));

        /* Special processing for certain opcodes */

        switch (Op->Common.AmlOpcode)
        {
        case AML_METHOD_OP:
            /*
             * Skip parsing of control method because we don't have enough
             * info in the first pass to parse it correctly.
             *
             * Save the length and address of the body
             */
            Op->Named.Data = WalkState->ParserState.Aml;
            Op->Named.Length = (UINT32)
                (WalkState->ParserState.PkgEnd - WalkState->ParserState.Aml);

            /* Skip body of method */

            WalkState->ParserState.Aml = WalkState->ParserState.PkgEnd;
            WalkState->ArgCount = 0;
            break;

        case AML_BUFFER_OP:
        case AML_PACKAGE_OP:
        case AML_VARIABLE_PACKAGE_OP:

            if ((Op->Common.Parent) &&
                (Op->Common.Parent->Common.AmlOpcode == AML_NAME_OP) &&
                (WalkState->PassNumber <= ACPI_IMODE_LOAD_PASS2))
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
                    "Setup Package/Buffer: Pass %u, AML Ptr: %p\n",
                    WalkState->PassNumber, AmlOpStart));

                /*
                 * Skip parsing of Buffers and Packages because we don't have
                 * enough info in the first pass to parse them correctly.
                 */
                Op->Named.Data = AmlOpStart;
                Op->Named.Length = (UINT32)
                    (WalkState->ParserState.PkgEnd - AmlOpStart);

                /* Skip body */

                WalkState->ParserState.Aml = WalkState->ParserState.PkgEnd;
                WalkState->ArgCount = 0;
            }
            break;

        case AML_WHILE_OP:

            if (WalkState->ControlState)
            {
                WalkState->ControlState->Control.PackageEnd =
                    WalkState->ParserState.PkgEnd;
            }
            break;

        default:

            /* No action for all other opcodes */

            break;
        }

        break;
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsParseLoop
 *
 * PARAMETERS:  WalkState           - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Parse AML (pointed to by the current parser state) and return
 *              a tree of ops.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsParseLoop (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_PARSE_OBJECT       *Op = NULL;     /* current op */
    ACPI_PARSE_STATE        *ParserState;
    UINT8                   *AmlOpStart = NULL;
    UINT8                   OpcodeLength;


    ACPI_FUNCTION_TRACE_PTR (PsParseLoop, WalkState);


    if (WalkState->DescendingCallback == NULL)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    ParserState = &WalkState->ParserState;
    WalkState->ArgTypes = 0;

#ifndef ACPI_CONSTANT_EVAL_ONLY

    if (WalkState->WalkType & ACPI_WALK_METHOD_RESTART)
    {
        /* We are restarting a preempted control method */

        if (AcpiPsHasCompletedScope (ParserState))
        {
            /*
             * We must check if a predicate to an IF or WHILE statement
             * was just completed
             */
            if ((ParserState->Scope->ParseScope.Op) &&
               ((ParserState->Scope->ParseScope.Op->Common.AmlOpcode == AML_IF_OP) ||
                (ParserState->Scope->ParseScope.Op->Common.AmlOpcode == AML_WHILE_OP)) &&
                (WalkState->ControlState) &&
                (WalkState->ControlState->Common.State ==
                    ACPI_CONTROL_PREDICATE_EXECUTING))
            {
                /*
                 * A predicate was just completed, get the value of the
                 * predicate and branch based on that value
                 */
                WalkState->Op = NULL;
                Status = AcpiDsGetPredicateValue (WalkState, ACPI_TO_POINTER (TRUE));
                if (ACPI_FAILURE (Status) && !ACPI_CNTL_EXCEPTION (Status))
                {
                    if (Status == AE_AML_NO_RETURN_VALUE)
                    {
                        ACPI_EXCEPTION ((AE_INFO, Status,
                            "Invoked method did not return a value"));
                    }

                    ACPI_EXCEPTION ((AE_INFO, Status, "GetPredicate Failed"));
                    return_ACPI_STATUS (Status);
                }

                Status = AcpiPsNextParseState (WalkState, Op, Status);
            }

            AcpiPsPopScope (ParserState, &Op,
                &WalkState->ArgTypes, &WalkState->ArgCount);
            ACPI_DEBUG_PRINT ((ACPI_DB_PARSE, "Popped scope, Op=%p\n", Op));
        }
        else if (WalkState->PrevOp)
        {
            /* We were in the middle of an op */

            Op = WalkState->PrevOp;
            WalkState->ArgTypes = WalkState->PrevArgTypes;
        }
    }
#endif

    /* Iterative parsing loop, while there is more AML to process: */

    while ((ParserState->Aml < ParserState->AmlEnd) || (Op))
    {
        ASL_CV_CAPTURE_COMMENTS (WalkState);

        AmlOpStart = ParserState->Aml;
        if (!Op)
        {
            Status = AcpiPsCreateOp (WalkState, AmlOpStart, &Op);
            if (ACPI_FAILURE (Status))
            {
                /*
                 * ACPI_PARSE_MODULE_LEVEL means that we are loading a table by
                 * executing it as a control method. However, if we encounter
                 * an error while loading the table, we need to keep trying to
                 * load the table rather than aborting the table load. Set the
                 * status to AE_OK to proceed with the table load.
                 */
                if ((WalkState->ParseFlags & ACPI_PARSE_MODULE_LEVEL) &&
                    ((Status == AE_ALREADY_EXISTS) || (Status == AE_NOT_FOUND)))
                {
                    Status = AE_OK;
                }
                if (Status == AE_CTRL_PARSE_CONTINUE)
                {
                    continue;
                }

                if (Status == AE_CTRL_PARSE_PENDING)
                {
                    Status = AE_OK;
                }

                if (Status == AE_CTRL_TERMINATE)
                {
                    return_ACPI_STATUS (Status);
                }

                Status = AcpiPsCompleteOp (WalkState, &Op, Status);
                if (ACPI_FAILURE (Status))
                {
                    return_ACPI_STATUS (Status);
                }
                if (AcpiNsOpensScope (
                    AcpiPsGetOpcodeInfo (WalkState->Opcode)->ObjectType))
                {
                    /*
                     * If the scope/device op fails to parse, skip the body of
                     * the scope op because the parse failure indicates that
                     * the device may not exist.
                     */
                    ACPI_INFO (("Skipping parse of AML opcode: %s (0x%4.4X)",
                        AcpiPsGetOpcodeName (WalkState->Opcode), WalkState->Opcode));

                    /*
                     * Determine the opcode length before skipping the opcode.
                     * An opcode can be 1 byte or 2 bytes in length.
                     */
                    OpcodeLength = 1;
                    if ((WalkState->Opcode & 0xFF00) == AML_EXTENDED_OPCODE)
                    {
                        OpcodeLength = 2;
                    }
                    WalkState->ParserState.Aml = WalkState->Aml + OpcodeLength;

                    WalkState->ParserState.Aml =
                        AcpiPsGetNextPackageEnd(&WalkState->ParserState);
                    WalkState->Aml = WalkState->ParserState.Aml;
                }

                continue;
            }

            AcpiExStartTraceOpcode (Op, WalkState);
        }

        /*
         * Start ArgCount at zero because we don't know if there are
         * any args yet
         */
        WalkState->ArgCount = 0;

        switch (Op->Common.AmlOpcode)
        {
        case AML_BYTE_OP:
        case AML_WORD_OP:
        case AML_DWORD_OP:
        case AML_QWORD_OP:

            break;

        default:

            ASL_CV_CAPTURE_COMMENTS (WalkState);
            break;
        }

        /* Are there any arguments that must be processed? */

        if (WalkState->ArgTypes)
        {
            /* Get arguments */

            Status = AcpiPsGetArguments (WalkState, AmlOpStart, Op);
            if (ACPI_FAILURE (Status))
            {
                Status = AcpiPsCompleteOp (WalkState, &Op, Status);
                if (ACPI_FAILURE (Status))
                {
                    return_ACPI_STATUS (Status);
                }
                if ((WalkState->ControlState) &&
                    ((WalkState->ControlState->Control.Opcode == AML_IF_OP) ||
                    (WalkState->ControlState->Control.Opcode == AML_WHILE_OP)))
                {
                    /*
                     * If the if/while op fails to parse, we will skip parsing
                     * the body of the op.
                     */
                    ParserState->Aml =
                        WalkState->ControlState->Control.AmlPredicateStart + 1;
                    ParserState->Aml =
                        AcpiPsGetNextPackageEnd (ParserState);
                    WalkState->Aml = ParserState->Aml;

                    ACPI_ERROR ((AE_INFO, "Skipping While/If block"));
                    if (*WalkState->Aml == AML_ELSE_OP)
                    {
                        ACPI_ERROR ((AE_INFO, "Skipping Else block"));
                        WalkState->ParserState.Aml = WalkState->Aml + 1;
                        WalkState->ParserState.Aml =
                            AcpiPsGetNextPackageEnd (ParserState);
                        WalkState->Aml = ParserState->Aml;
                    }
                    ACPI_FREE(AcpiUtPopGenericState (&WalkState->ControlState));
                }
                Op = NULL;
                continue;
            }
        }

        /* Check for arguments that need to be processed */

        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
            "Parseloop: argument count: %8.8X\n", WalkState->ArgCount));

        if (WalkState->ArgCount)
        {
            /*
             * There are arguments (complex ones), push Op and
             * prepare for argument
             */
            Status = AcpiPsPushScope (ParserState, Op,
                WalkState->ArgTypes, WalkState->ArgCount);
            if (ACPI_FAILURE (Status))
            {
                Status = AcpiPsCompleteOp (WalkState, &Op, Status);
                if (ACPI_FAILURE (Status))
                {
                    return_ACPI_STATUS (Status);
                }

                continue;
            }

            Op = NULL;
            continue;
        }

        /*
         * All arguments have been processed -- Op is complete,
         * prepare for next
         */
        WalkState->OpInfo = AcpiPsGetOpcodeInfo (Op->Common.AmlOpcode);
        if (WalkState->OpInfo->Flags & AML_NAMED)
        {
            if (Op->Common.AmlOpcode == AML_REGION_OP ||
                Op->Common.AmlOpcode == AML_DATA_REGION_OP)
            {
                /*
                 * Skip parsing of control method or opregion body,
                 * because we don't have enough info in the first pass
                 * to parse them correctly.
                 *
                 * Completed parsing an OpRegion declaration, we now
                 * know the length.
                 */
                Op->Named.Length = (UINT32) (ParserState->Aml - Op->Named.Data);
            }
        }

        if (WalkState->OpInfo->Flags & AML_CREATE)
        {
            /*
             * Backup to beginning of CreateXXXfield declaration (1 for
             * Opcode)
             *
             * BodyLength is unknown until we parse the body
             */
            Op->Named.Length = (UINT32) (ParserState->Aml - Op->Named.Data);
        }

        if (Op->Common.AmlOpcode == AML_BANK_FIELD_OP)
        {
            /*
             * Backup to beginning of BankField declaration
             *
             * BodyLength is unknown until we parse the body
             */
            Op->Named.Length = (UINT32) (ParserState->Aml - Op->Named.Data);
        }

        /* This op complete, notify the dispatcher */

        if (WalkState->AscendingCallback != NULL)
        {
            WalkState->Op = Op;
            WalkState->Opcode = Op->Common.AmlOpcode;

            Status = WalkState->AscendingCallback (WalkState);
            Status = AcpiPsNextParseState (WalkState, Op, Status);
            if (Status == AE_CTRL_PENDING)
            {
                Status = AE_OK;
            }
            else if ((WalkState->ParseFlags & ACPI_PARSE_MODULE_LEVEL) &&
                (ACPI_AML_EXCEPTION(Status) || Status == AE_ALREADY_EXISTS ||
                Status == AE_NOT_FOUND))
            {
                /*
                 * ACPI_PARSE_MODULE_LEVEL flag means that we are currently
                 * loading a table by executing it as a control method.
                 * However, if we encounter an error while loading the table,
                 * we need to keep trying to load the table rather than
                 * aborting the table load (setting the status to AE_OK
                 * continues the table load). If we get a failure at this
                 * point, it means that the dispatcher got an error while
                 * trying to execute the Op.
                 */
                Status = AE_OK;
            }
        }

        Status = AcpiPsCompleteOp (WalkState, &Op, Status);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

    } /* while ParserState->Aml */

    Status = AcpiPsCompleteFinalOp (WalkState, Op, Status);
    return_ACPI_STATUS (Status);
}
