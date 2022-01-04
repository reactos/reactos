/******************************************************************************
 *
 * Module Name: psscope - Parser scope stack management routines
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

#define _COMPONENT          ACPI_PARSER
        ACPI_MODULE_NAME    ("psscope")


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetParentScope
 *
 * PARAMETERS:  ParserState         - Current parser state object
 *
 * RETURN:      Pointer to an Op object
 *
 * DESCRIPTION: Get parent of current op being parsed
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
AcpiPsGetParentScope (
    ACPI_PARSE_STATE        *ParserState)
{

    return (ParserState->Scope->ParseScope.Op);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsHasCompletedScope
 *
 * PARAMETERS:  ParserState         - Current parser state object
 *
 * RETURN:      Boolean, TRUE = scope completed.
 *
 * DESCRIPTION: Is parsing of current argument complete?  Determined by
 *              1) AML pointer is at or beyond the end of the scope
 *              2) The scope argument count has reached zero.
 *
 ******************************************************************************/

BOOLEAN
AcpiPsHasCompletedScope (
    ACPI_PARSE_STATE        *ParserState)
{

    return ((BOOLEAN)
            ((ParserState->Aml >= ParserState->Scope->ParseScope.ArgEnd ||
             !ParserState->Scope->ParseScope.ArgCount)));
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsInitScope
 *
 * PARAMETERS:  ParserState         - Current parser state object
 *              Root                - the Root Node of this new scope
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Allocate and init a new scope object
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsInitScope (
    ACPI_PARSE_STATE        *ParserState,
    ACPI_PARSE_OBJECT       *RootOp)
{
    ACPI_GENERIC_STATE      *Scope;


    ACPI_FUNCTION_TRACE_PTR (PsInitScope, RootOp);


    Scope = AcpiUtCreateGenericState ();
    if (!Scope)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    Scope->Common.DescriptorType = ACPI_DESC_TYPE_STATE_RPSCOPE;
    Scope->ParseScope.Op = RootOp;
    Scope->ParseScope.ArgCount = ACPI_VAR_ARGS;
    Scope->ParseScope.ArgEnd = ParserState->AmlEnd;
    Scope->ParseScope.PkgEnd = ParserState->AmlEnd;

    ParserState->Scope = Scope;
    ParserState->StartOp = RootOp;

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsPushScope
 *
 * PARAMETERS:  ParserState         - Current parser state object
 *              Op                  - Current op to be pushed
 *              RemainingArgs       - List of args remaining
 *              ArgCount            - Fixed or variable number of args
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Push current op to begin parsing its argument
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsPushScope (
    ACPI_PARSE_STATE        *ParserState,
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  RemainingArgs,
    UINT32                  ArgCount)
{
    ACPI_GENERIC_STATE      *Scope;


    ACPI_FUNCTION_TRACE_PTR (PsPushScope, Op);


    Scope = AcpiUtCreateGenericState ();
    if (!Scope)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    Scope->Common.DescriptorType = ACPI_DESC_TYPE_STATE_PSCOPE;
    Scope->ParseScope.Op = Op;
    Scope->ParseScope.ArgList = RemainingArgs;
    Scope->ParseScope.ArgCount = ArgCount;
    Scope->ParseScope.PkgEnd = ParserState->PkgEnd;

    /* Push onto scope stack */

    AcpiUtPushGenericState (&ParserState->Scope, Scope);

    if (ArgCount == ACPI_VAR_ARGS)
    {
        /* Multiple arguments */

        Scope->ParseScope.ArgEnd = ParserState->PkgEnd;
    }
    else
    {
        /* Single argument */

        Scope->ParseScope.ArgEnd = ACPI_TO_POINTER (ACPI_MAX_PTR);
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsPopScope
 *
 * PARAMETERS:  ParserState         - Current parser state object
 *              Op                  - Where the popped op is returned
 *              ArgList             - Where the popped "next argument" is
 *                                    returned
 *              ArgCount            - Count of objects in ArgList
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Return to parsing a previous op
 *
 ******************************************************************************/

void
AcpiPsPopScope (
    ACPI_PARSE_STATE        *ParserState,
    ACPI_PARSE_OBJECT       **Op,
    UINT32                  *ArgList,
    UINT32                  *ArgCount)
{
    ACPI_GENERIC_STATE      *Scope = ParserState->Scope;


    ACPI_FUNCTION_TRACE (PsPopScope);


    /* Only pop the scope if there is in fact a next scope */

    if (Scope->Common.Next)
    {
        Scope = AcpiUtPopGenericState (&ParserState->Scope);

        /* Return to parsing previous op */

        *Op = Scope->ParseScope.Op;
        *ArgList = Scope->ParseScope.ArgList;
        *ArgCount = Scope->ParseScope.ArgCount;
        ParserState->PkgEnd = Scope->ParseScope.PkgEnd;

        /* All done with this scope state structure */

        AcpiUtDeleteGenericState (Scope);
    }
    else
    {
        /* Empty parse stack, prepare to fetch next opcode */

        *Op = NULL;
        *ArgList = 0;
        *ArgCount = 0;
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
        "Popped Op %p Args %X\n", *Op, *ArgCount));
    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsCleanupScope
 *
 * PARAMETERS:  ParserState         - Current parser state object
 *
 * RETURN:      None
 *
 * DESCRIPTION: Destroy available list, remaining stack levels, and return
 *              root scope
 *
 ******************************************************************************/

void
AcpiPsCleanupScope (
    ACPI_PARSE_STATE        *ParserState)
{
    ACPI_GENERIC_STATE      *Scope;


    ACPI_FUNCTION_TRACE_PTR (PsCleanupScope, ParserState);


    if (!ParserState)
    {
        return_VOID;
    }

    /* Delete anything on the scope stack */

    while (ParserState->Scope)
    {
        Scope = AcpiUtPopGenericState (&ParserState->Scope);
        AcpiUtDeleteGenericState (Scope);
    }

    return_VOID;
}
