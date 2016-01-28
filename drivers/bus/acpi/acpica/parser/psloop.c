/******************************************************************************
 *
 * Module Name: psloop - Main AML parse loop
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

#define _COMPONENT          ACPI_PARSER
        ACPI_MODULE_NAME    ("psloop")


/* Local prototypes */

static ACPI_STATUS
AcpiPsGetArguments (
    ACPI_WALK_STATE         *WalkState,
    UINT8                   *AmlOpStart,
    ACPI_PARSE_OBJECT       *Op);

static void
AcpiPsLinkModuleCode (
    ACPI_PARSE_OBJECT       *ParentOp,
    UINT8                   *AmlStart,
    UINT32                  AmlLength,
    ACPI_OWNER_ID           OwnerId);


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
    const ACPI_OPCODE_INFO  *OpInfo;


    ACPI_FUNCTION_TRACE_PTR (PsGetArguments, WalkState);


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


        /*
         * Handle executable code at "module-level". This refers to
         * executable opcodes that appear outside of any control method.
         */
        if ((WalkState->PassNumber <= ACPI_IMODE_LOAD_PASS2) &&
            ((WalkState->ParseFlags & ACPI_PARSE_DISASSEMBLE) == 0))
        {
            /*
             * We want to skip If/Else/While constructs during Pass1 because we
             * want to actually conditionally execute the code during Pass2.
             *
             * Except for disassembly, where we always want to walk the
             * If/Else/While packages
             */
            switch (Op->Common.AmlOpcode)
            {
            case AML_IF_OP:
            case AML_ELSE_OP:
            case AML_WHILE_OP:
                /*
                 * Currently supported module-level opcodes are:
                 * IF/ELSE/WHILE. These appear to be the most common,
                 * and easiest to support since they open an AML
                 * package.
                 */
                if (WalkState->PassNumber == ACPI_IMODE_LOAD_PASS1)
                {
                    AcpiPsLinkModuleCode (Op->Common.Parent, AmlOpStart,
                        (UINT32) (WalkState->ParserState.PkgEnd - AmlOpStart),
                        WalkState->OwnerId);
                }

                ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
                    "Pass1: Skipping an If/Else/While body\n"));

                /* Skip body of if/else/while in pass 1 */

                WalkState->ParserState.Aml = WalkState->ParserState.PkgEnd;
                WalkState->ArgCount = 0;
                break;

            default:
                /*
                 * Check for an unsupported executable opcode at module
                 * level. We must be in PASS1, the parent must be a SCOPE,
                 * The opcode class must be EXECUTE, and the opcode must
                 * not be an argument to another opcode.
                 */
                if ((WalkState->PassNumber == ACPI_IMODE_LOAD_PASS1) &&
                    (Op->Common.Parent->Common.AmlOpcode == AML_SCOPE_OP))
                {
                    OpInfo = AcpiPsGetOpcodeInfo (Op->Common.AmlOpcode);
                    if ((OpInfo->Class == AML_CLASS_EXECUTE) &&
                        (!Arg))
                    {
                        ACPI_WARNING ((AE_INFO,
                            "Unsupported module-level executable opcode "
                            "0x%.2X at table offset 0x%.4X",
                            Op->Common.AmlOpcode,
                            (UINT32) (ACPI_PTR_DIFF (AmlOpStart,
                                WalkState->ParserState.AmlStart) +
                                sizeof (ACPI_TABLE_HEADER))));
                    }
                }
                break;
            }
        }

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
        case AML_VAR_PACKAGE_OP:

            if ((Op->Common.Parent) &&
                (Op->Common.Parent->Common.AmlOpcode == AML_NAME_OP) &&
                (WalkState->PassNumber <= ACPI_IMODE_LOAD_PASS2))
            {
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
 * FUNCTION:    AcpiPsLinkModuleCode
 *
 * PARAMETERS:  ParentOp            - Parent parser op
 *              AmlStart            - Pointer to the AML
 *              AmlLength           - Length of executable AML
 *              OwnerId             - OwnerId of module level code
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Wrap the module-level code with a method object and link the
 *              object to the global list. Note, the mutex field of the method
 *              object is used to link multiple module-level code objects.
 *
 ******************************************************************************/

static void
AcpiPsLinkModuleCode (
    ACPI_PARSE_OBJECT       *ParentOp,
    UINT8                   *AmlStart,
    UINT32                  AmlLength,
    ACPI_OWNER_ID           OwnerId)
{
    ACPI_OPERAND_OBJECT     *Prev;
    ACPI_OPERAND_OBJECT     *Next;
    ACPI_OPERAND_OBJECT     *MethodObj;
    ACPI_NAMESPACE_NODE     *ParentNode;


    ACPI_FUNCTION_TRACE (PsLinkModuleCode);


    /* Get the tail of the list */

    Prev = Next = AcpiGbl_ModuleCodeList;
    while (Next)
    {
        Prev = Next;
        Next = Next->Method.Mutex;
    }

    /*
     * Insert the module level code into the list. Merge it if it is
     * adjacent to the previous element.
     */
    if (!Prev ||
       ((Prev->Method.AmlStart + Prev->Method.AmlLength) != AmlStart))
    {
        /* Create, initialize, and link a new temporary method object */

        MethodObj = AcpiUtCreateInternalObject (ACPI_TYPE_METHOD);
        if (!MethodObj)
        {
            return_VOID;
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
            "Create/Link new code block: %p\n", MethodObj));

        if (ParentOp->Common.Node)
        {
            ParentNode = ParentOp->Common.Node;
        }
        else
        {
            ParentNode = AcpiGbl_RootNode;
        }

        MethodObj->Method.AmlStart = AmlStart;
        MethodObj->Method.AmlLength = AmlLength;
        MethodObj->Method.OwnerId = OwnerId;
        MethodObj->Method.InfoFlags |= ACPI_METHOD_MODULE_LEVEL;

        /*
         * Save the parent node in NextObject. This is cheating, but we
         * don't want to expand the method object.
         */
        MethodObj->Method.NextObject =
            ACPI_CAST_PTR (ACPI_OPERAND_OBJECT, ParentNode);

        if (!Prev)
        {
            AcpiGbl_ModuleCodeList = MethodObj;
        }
        else
        {
            Prev->Method.Mutex = MethodObj;
        }
    }
    else
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
            "Appending to existing code block: %p\n", Prev));

        Prev->Method.AmlLength += AmlLength;
    }

    return_VOID;
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


    ACPI_FUNCTION_TRACE_PTR (PsParseLoop, WalkState);


    if (WalkState->DescendingCallback == NULL)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    ParserState = &WalkState->ParserState;
    WalkState->ArgTypes = 0;

#if (!defined (ACPI_NO_METHOD_EXECUTION) && !defined (ACPI_CONSTANT_EVAL_ONLY))

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
                if (ACPI_FAILURE (Status) &&
                    ((Status & AE_CODE_MASK) != AE_CODE_CONTROL))
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
        AmlOpStart = ParserState->Aml;
        if (!Op)
        {
            Status = AcpiPsCreateOp (WalkState, AmlOpStart, &Op);
            if (ACPI_FAILURE (Status))
            {
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

                continue;
            }

            AcpiExStartTraceOpcode (Op, WalkState);
        }


        /*
         * Start ArgCount at zero because we don't know if there are
         * any args yet
         */
        WalkState->ArgCount  = 0;

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

                continue;
            }
        }

        /* Check for arguments that need to be processed */

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
