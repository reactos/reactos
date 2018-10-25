/******************************************************************************
 *
 * Module Name: psobject - Support for parse objects
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2017, Intel Corp.
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
#include "acconvert.h"

#define _COMPONENT          ACPI_PARSER
        ACPI_MODULE_NAME    ("psobject")


/* Local prototypes */

static ACPI_STATUS
AcpiPsGetAmlOpcode (
    ACPI_WALK_STATE         *WalkState);


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetAmlOpcode
 *
 * PARAMETERS:  WalkState           - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Extract the next AML opcode from the input stream.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiPsGetAmlOpcode (
    ACPI_WALK_STATE         *WalkState)
{
    UINT32                  AmlOffset;


    ACPI_FUNCTION_TRACE_PTR (PsGetAmlOpcode, WalkState);


    WalkState->Aml = WalkState->ParserState.Aml;
    WalkState->Opcode = AcpiPsPeekOpcode (&(WalkState->ParserState));

    /*
     * First cut to determine what we have found:
     * 1) A valid AML opcode
     * 2) A name string
     * 3) An unknown/invalid opcode
     */
    WalkState->OpInfo = AcpiPsGetOpcodeInfo (WalkState->Opcode);

    switch (WalkState->OpInfo->Class)
    {
    case AML_CLASS_ASCII:
    case AML_CLASS_PREFIX:
        /*
         * Starts with a valid prefix or ASCII char, this is a name
         * string. Convert the bare name string to a namepath.
         */
        WalkState->Opcode = AML_INT_NAMEPATH_OP;
        WalkState->ArgTypes = ARGP_NAMESTRING;
        break;

    case AML_CLASS_UNKNOWN:

        /* The opcode is unrecognized. Complain and skip unknown opcodes */

        if (WalkState->PassNumber == 2)
        {
            AmlOffset = (UINT32) ACPI_PTR_DIFF (WalkState->Aml,
                WalkState->ParserState.AmlStart);

            ACPI_ERROR ((AE_INFO,
                "Unknown opcode 0x%.2X at table offset 0x%.4X, ignoring",
                WalkState->Opcode,
                (UINT32) (AmlOffset + sizeof (ACPI_TABLE_HEADER))));

            ACPI_DUMP_BUFFER ((WalkState->ParserState.Aml - 16), 48);

#ifdef ACPI_ASL_COMPILER
            /*
             * This is executed for the disassembler only. Output goes
             * to the disassembled ASL output file.
             */
            AcpiOsPrintf (
                "/*\nError: Unknown opcode 0x%.2X at table offset 0x%.4X, context:\n",
                WalkState->Opcode,
                (UINT32) (AmlOffset + sizeof (ACPI_TABLE_HEADER)));

            ACPI_ERROR ((AE_INFO,
                "Aborting disassembly, AML byte code is corrupt"));

            /* Dump the context surrounding the invalid opcode */

            AcpiUtDumpBuffer (((UINT8 *) WalkState->ParserState.Aml - 16),
                48, DB_BYTE_DISPLAY,
                (AmlOffset + sizeof (ACPI_TABLE_HEADER) - 16));
            AcpiOsPrintf (" */\n");

            /*
             * Just abort the disassembly, cannot continue because the
             * parser is essentially lost. The disassembler can then
             * randomly fail because an ill-constructed parse tree
             * can result.
             */
            return_ACPI_STATUS (AE_AML_BAD_OPCODE);
#endif
        }

        /* Increment past one-byte or two-byte opcode */

        WalkState->ParserState.Aml++;
        if (WalkState->Opcode > 0xFF) /* Can only happen if first byte is 0x5B */
        {
            WalkState->ParserState.Aml++;
        }

        return_ACPI_STATUS (AE_CTRL_PARSE_CONTINUE);

    default:

        /* Found opcode info, this is a normal opcode */

        WalkState->ParserState.Aml +=
            AcpiPsGetOpcodeSize (WalkState->Opcode);
        WalkState->ArgTypes = WalkState->OpInfo->ParseArgs;
        break;
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsBuildNamedOp
 *
 * PARAMETERS:  WalkState           - Current state
 *              AmlOpStart          - Begin of named Op in AML
 *              UnnamedOp           - Early Op (not a named Op)
 *              Op                  - Returned Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Parse a named Op
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsBuildNamedOp (
    ACPI_WALK_STATE         *WalkState,
    UINT8                   *AmlOpStart,
    ACPI_PARSE_OBJECT       *UnnamedOp,
    ACPI_PARSE_OBJECT       **Op)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_PARSE_OBJECT       *Arg = NULL;


    ACPI_FUNCTION_TRACE_PTR (PsBuildNamedOp, WalkState);


    UnnamedOp->Common.Value.Arg = NULL;
    UnnamedOp->Common.ArgListLength = 0;
    UnnamedOp->Common.AmlOpcode = WalkState->Opcode;

    /*
     * Get and append arguments until we find the node that contains
     * the name (the type ARGP_NAME).
     */
    while (GET_CURRENT_ARG_TYPE (WalkState->ArgTypes) &&
          (GET_CURRENT_ARG_TYPE (WalkState->ArgTypes) != ARGP_NAME))
    {
        ASL_CV_CAPTURE_COMMENTS (WalkState);
        Status = AcpiPsGetNextArg (WalkState, &(WalkState->ParserState),
            GET_CURRENT_ARG_TYPE (WalkState->ArgTypes), &Arg);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        AcpiPsAppendArg (UnnamedOp, Arg);
        INCREMENT_ARG_LIST (WalkState->ArgTypes);
    }

    /* are there any inline comments associated with the NameSeg?? If so, save this. */

    ASL_CV_CAPTURE_COMMENTS (WalkState);

#ifdef ACPI_ASL_COMPILER
    if (AcpiGbl_CurrentInlineComment != NULL)
    {
        UnnamedOp->Common.NameComment = AcpiGbl_CurrentInlineComment;
        AcpiGbl_CurrentInlineComment = NULL;
    }
#endif

    /*
     * Make sure that we found a NAME and didn't run out of arguments
     */
    if (!GET_CURRENT_ARG_TYPE (WalkState->ArgTypes))
    {
        return_ACPI_STATUS (AE_AML_NO_OPERAND);
    }

    /* We know that this arg is a name, move to next arg */

    INCREMENT_ARG_LIST (WalkState->ArgTypes);

    /*
     * Find the object. This will either insert the object into
     * the namespace or simply look it up
     */
    WalkState->Op = NULL;

    Status = WalkState->DescendingCallback (WalkState, Op);
    if (ACPI_FAILURE (Status))
    {
        if (Status != AE_CTRL_TERMINATE)
        {
            ACPI_EXCEPTION ((AE_INFO, Status, "During name lookup/catalog"));
        }
        return_ACPI_STATUS (Status);
    }

    if (!*Op)
    {
        return_ACPI_STATUS (AE_CTRL_PARSE_CONTINUE);
    }

    Status = AcpiPsNextParseState (WalkState, *Op, Status);
    if (ACPI_FAILURE (Status))
    {
        if (Status == AE_CTRL_PENDING)
        {
            Status = AE_CTRL_PARSE_PENDING;
        }
        return_ACPI_STATUS (Status);
    }

    AcpiPsAppendArg (*Op, UnnamedOp->Common.Value.Arg);

#ifdef ACPI_ASL_COMPILER

    /* save any comments that might be associated with UnnamedOp. */

    (*Op)->Common.InlineComment     = UnnamedOp->Common.InlineComment;
    (*Op)->Common.EndNodeComment    = UnnamedOp->Common.EndNodeComment;
    (*Op)->Common.CloseBraceComment = UnnamedOp->Common.CloseBraceComment;
    (*Op)->Common.NameComment       = UnnamedOp->Common.NameComment;
    (*Op)->Common.CommentList       = UnnamedOp->Common.CommentList;
    (*Op)->Common.EndBlkComment     = UnnamedOp->Common.EndBlkComment;
    (*Op)->Common.CvFilename        = UnnamedOp->Common.CvFilename;
    (*Op)->Common.CvParentFilename  = UnnamedOp->Common.CvParentFilename;
    (*Op)->Named.Aml                = UnnamedOp->Common.Aml;

    UnnamedOp->Common.InlineComment     = NULL;
    UnnamedOp->Common.EndNodeComment    = NULL;
    UnnamedOp->Common.CloseBraceComment = NULL;
    UnnamedOp->Common.NameComment       = NULL;
    UnnamedOp->Common.CommentList       = NULL;
    UnnamedOp->Common.EndBlkComment     = NULL;
#endif

    if ((*Op)->Common.AmlOpcode == AML_REGION_OP ||
        (*Op)->Common.AmlOpcode == AML_DATA_REGION_OP)
    {
        /*
         * Defer final parsing of an OperationRegion body, because we don't
         * have enough info in the first pass to parse it correctly (i.e.,
         * there may be method calls within the TermArg elements of the body.)
         *
         * However, we must continue parsing because the opregion is not a
         * standalone package -- we don't know where the end is at this point.
         *
         * (Length is unknown until parse of the body complete)
         */
        (*Op)->Named.Data = AmlOpStart;
        (*Op)->Named.Length = 0;
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsCreateOp
 *
 * PARAMETERS:  WalkState           - Current state
 *              AmlOpStart          - Op start in AML
 *              NewOp               - Returned Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get Op from AML
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsCreateOp (
    ACPI_WALK_STATE         *WalkState,
    UINT8                   *AmlOpStart,
    ACPI_PARSE_OBJECT       **NewOp)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_PARSE_OBJECT       *Op;
    ACPI_PARSE_OBJECT       *NamedOp = NULL;
    ACPI_PARSE_OBJECT       *ParentScope;
    UINT8                   ArgumentCount;
    const ACPI_OPCODE_INFO  *OpInfo;


    ACPI_FUNCTION_TRACE_PTR (PsCreateOp, WalkState);


    Status = AcpiPsGetAmlOpcode (WalkState);
    if (Status == AE_CTRL_PARSE_CONTINUE)
    {
        return_ACPI_STATUS (AE_CTRL_PARSE_CONTINUE);
    }
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Create Op structure and append to parent's argument list */

    WalkState->OpInfo = AcpiPsGetOpcodeInfo (WalkState->Opcode);
    Op = AcpiPsAllocOp (WalkState->Opcode, AmlOpStart);
    if (!Op)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    if (WalkState->OpInfo->Flags & AML_NAMED)
    {
        Status = AcpiPsBuildNamedOp (WalkState, AmlOpStart, Op, &NamedOp);
        AcpiPsFreeOp (Op);

#ifdef ACPI_ASL_COMPILER
        if (AcpiGbl_DisasmFlag && WalkState->Opcode == AML_EXTERNAL_OP &&
            Status == AE_NOT_FOUND)
        {
            /*
             * If parsing of AML_EXTERNAL_OP's name path fails, then skip
             * past this opcode and keep parsing. This is a much better
             * alternative than to abort the entire disassembler. At this
             * point, the ParserState is at the end of the namepath of the
             * external declaration opcode. Setting WalkState->Aml to
             * WalkState->ParserState.Aml + 2 moves increments the
             * WalkState->Aml past the object type and the paramcount of the
             * external opcode.
             */
            WalkState->Aml = WalkState->ParserState.Aml + 2;
            WalkState->ParserState.Aml = WalkState->Aml;
            return_ACPI_STATUS (AE_CTRL_PARSE_CONTINUE);
        }
#endif
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        *NewOp = NamedOp;
        return_ACPI_STATUS (AE_OK);
    }

    /* Not a named opcode, just allocate Op and append to parent */

    if (WalkState->OpInfo->Flags & AML_CREATE)
    {
        /*
         * Backup to beginning of CreateXXXfield declaration
         * BodyLength is unknown until we parse the body
         */
        Op->Named.Data = AmlOpStart;
        Op->Named.Length = 0;
    }

    if (WalkState->Opcode == AML_BANK_FIELD_OP)
    {
        /*
         * Backup to beginning of BankField declaration
         * BodyLength is unknown until we parse the body
         */
        Op->Named.Data = AmlOpStart;
        Op->Named.Length = 0;
    }

    ParentScope = AcpiPsGetParentScope (&(WalkState->ParserState));
    AcpiPsAppendArg (ParentScope, Op);

    if (ParentScope)
    {
        OpInfo = AcpiPsGetOpcodeInfo (ParentScope->Common.AmlOpcode);
        if (OpInfo->Flags & AML_HAS_TARGET)
        {
            ArgumentCount = AcpiPsGetArgumentCount (OpInfo->Type);
            if (ParentScope->Common.ArgListLength > ArgumentCount)
            {
                Op->Common.Flags |= ACPI_PARSEOP_TARGET;
            }
        }

        /*
         * Special case for both Increment() and Decrement(), where
         * the lone argument is both a source and a target.
         */
        else if ((ParentScope->Common.AmlOpcode == AML_INCREMENT_OP) ||
                (ParentScope->Common.AmlOpcode == AML_DECREMENT_OP))
        {
            Op->Common.Flags |= ACPI_PARSEOP_TARGET;
        }
    }

    if (WalkState->DescendingCallback != NULL)
    {
        /*
         * Find the object. This will either insert the object into
         * the namespace or simply look it up
         */
        WalkState->Op = *NewOp = Op;

        Status = WalkState->DescendingCallback (WalkState, &Op);
        Status = AcpiPsNextParseState (WalkState, Op, Status);
        if (Status == AE_CTRL_PENDING)
        {
            Status = AE_CTRL_PARSE_PENDING;
        }
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsCompleteOp
 *
 * PARAMETERS:  WalkState           - Current state
 *              Op                  - Returned Op
 *              Status              - Parse status before complete Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Complete Op
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsCompleteOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       **Op,
    ACPI_STATUS             Status)
{
    ACPI_STATUS             Status2;


    ACPI_FUNCTION_TRACE_PTR (PsCompleteOp, WalkState);


    /*
     * Finished one argument of the containing scope
     */
    WalkState->ParserState.Scope->ParseScope.ArgCount--;

    /* Close this Op (will result in parse subtree deletion) */

    Status2 = AcpiPsCompleteThisOp (WalkState, *Op);
    if (ACPI_FAILURE (Status2))
    {
        return_ACPI_STATUS (Status2);
    }

    *Op = NULL;

    switch (Status)
    {
    case AE_OK:

        break;

    case AE_CTRL_TRANSFER:

        /* We are about to transfer to a called method */

        WalkState->PrevOp = NULL;
        WalkState->PrevArgTypes = WalkState->ArgTypes;
        return_ACPI_STATUS (Status);

    case AE_CTRL_END:

        AcpiPsPopScope (&(WalkState->ParserState), Op,
            &WalkState->ArgTypes, &WalkState->ArgCount);

        if (*Op)
        {
            WalkState->Op = *Op;
            WalkState->OpInfo = AcpiPsGetOpcodeInfo ((*Op)->Common.AmlOpcode);
            WalkState->Opcode = (*Op)->Common.AmlOpcode;

            Status = WalkState->AscendingCallback (WalkState);
            Status = AcpiPsNextParseState (WalkState, *Op, Status);

            Status2 = AcpiPsCompleteThisOp (WalkState, *Op);
            if (ACPI_FAILURE (Status2))
            {
                return_ACPI_STATUS (Status2);
            }
        }

        Status = AE_OK;
        break;

    case AE_CTRL_BREAK:
    case AE_CTRL_CONTINUE:

        /* Pop off scopes until we find the While */

        while (!(*Op) || ((*Op)->Common.AmlOpcode != AML_WHILE_OP))
        {
            AcpiPsPopScope (&(WalkState->ParserState), Op,
                &WalkState->ArgTypes, &WalkState->ArgCount);
        }

        /* Close this iteration of the While loop */

        WalkState->Op = *Op;
        WalkState->OpInfo = AcpiPsGetOpcodeInfo ((*Op)->Common.AmlOpcode);
        WalkState->Opcode = (*Op)->Common.AmlOpcode;

        Status = WalkState->AscendingCallback (WalkState);
        Status = AcpiPsNextParseState (WalkState, *Op, Status);

        Status2 = AcpiPsCompleteThisOp (WalkState, *Op);
        if (ACPI_FAILURE (Status2))
        {
            return_ACPI_STATUS (Status2);
        }

        Status = AE_OK;
        break;

    case AE_CTRL_TERMINATE:

        /* Clean up */
        do
        {
            if (*Op)
            {
                Status2 = AcpiPsCompleteThisOp (WalkState, *Op);
                if (ACPI_FAILURE (Status2))
                {
                    return_ACPI_STATUS (Status2);
                }

                AcpiUtDeleteGenericState (
                    AcpiUtPopGenericState (&WalkState->ControlState));
            }

            AcpiPsPopScope (&(WalkState->ParserState), Op,
                &WalkState->ArgTypes, &WalkState->ArgCount);

        } while (*Op);

        return_ACPI_STATUS (AE_OK);

    default:  /* All other non-AE_OK status */

        do
        {
            if (*Op)
            {
                Status2 = AcpiPsCompleteThisOp (WalkState, *Op);
                if (ACPI_FAILURE (Status2))
                {
                    return_ACPI_STATUS (Status2);
                }
            }

            AcpiPsPopScope (&(WalkState->ParserState), Op,
                &WalkState->ArgTypes, &WalkState->ArgCount);

        } while (*Op);


#if 0
        /*
         * TBD: Cleanup parse ops on error
         */
        if (*Op == NULL)
        {
            AcpiPsPopScope (ParserState, Op,
                &WalkState->ArgTypes, &WalkState->ArgCount);
        }
#endif
        WalkState->PrevOp = NULL;
        WalkState->PrevArgTypes = WalkState->ArgTypes;
        return_ACPI_STATUS (Status);
    }

    /* This scope complete? */

    if (AcpiPsHasCompletedScope (&(WalkState->ParserState)))
    {
        AcpiPsPopScope (&(WalkState->ParserState), Op,
            &WalkState->ArgTypes, &WalkState->ArgCount);
        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE, "Popped scope, Op=%p\n", *Op));
    }
    else
    {
        *Op = NULL;
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsCompleteFinalOp
 *
 * PARAMETERS:  WalkState           - Current state
 *              Op                  - Current Op
 *              Status              - Current parse status before complete last
 *                                    Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Complete last Op.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsCompleteFinalOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    ACPI_STATUS             Status)
{
    ACPI_STATUS             Status2;


    ACPI_FUNCTION_TRACE_PTR (PsCompleteFinalOp, WalkState);


    /*
     * Complete the last Op (if not completed), and clear the scope stack.
     * It is easily possible to end an AML "package" with an unbounded number
     * of open scopes (such as when several ASL blocks are closed with
     * sequential closing braces). We want to terminate each one cleanly.
     */
    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE, "AML package complete at Op %p\n", Op));
    do
    {
        if (Op)
        {
            if (WalkState->AscendingCallback != NULL)
            {
                WalkState->Op = Op;
                WalkState->OpInfo = AcpiPsGetOpcodeInfo (Op->Common.AmlOpcode);
                WalkState->Opcode = Op->Common.AmlOpcode;

                Status = WalkState->AscendingCallback (WalkState);
                Status = AcpiPsNextParseState (WalkState, Op, Status);
                if (Status == AE_CTRL_PENDING)
                {
                    Status = AcpiPsCompleteOp (WalkState, &Op, AE_OK);
                    if (ACPI_FAILURE (Status))
                    {
                        return_ACPI_STATUS (Status);
                    }
                }

                if (Status == AE_CTRL_TERMINATE)
                {
                    Status = AE_OK;

                    /* Clean up */
                    do
                    {
                        if (Op)
                        {
                            Status2 = AcpiPsCompleteThisOp (WalkState, Op);
                            if (ACPI_FAILURE (Status2))
                            {
                                return_ACPI_STATUS (Status2);
                            }
                        }

                        AcpiPsPopScope (&(WalkState->ParserState), &Op,
                            &WalkState->ArgTypes, &WalkState->ArgCount);

                    } while (Op);

                    return_ACPI_STATUS (Status);
                }

                else if (ACPI_FAILURE (Status))
                {
                    /* First error is most important */

                    (void) AcpiPsCompleteThisOp (WalkState, Op);
                    return_ACPI_STATUS (Status);
                }
            }

            Status2 = AcpiPsCompleteThisOp (WalkState, Op);
            if (ACPI_FAILURE (Status2))
            {
                return_ACPI_STATUS (Status2);
            }
        }

        AcpiPsPopScope (&(WalkState->ParserState), &Op, &WalkState->ArgTypes,
            &WalkState->ArgCount);

    } while (Op);

    return_ACPI_STATUS (Status);
}
