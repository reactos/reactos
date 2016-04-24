/******************************************************************************
 *
 * Module Name: pswalk - Parser routines to walk parsed op tree(s)
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

#define _COMPONENT          ACPI_PARSER
        ACPI_MODULE_NAME    ("pswalk")


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsDeleteParseTree
 *
 * PARAMETERS:  SubtreeRoot         - Root of tree (or subtree) to delete
 *
 * RETURN:      None
 *
 * DESCRIPTION: Delete a portion of or an entire parse tree.
 *
 ******************************************************************************/

void
AcpiPsDeleteParseTree (
    ACPI_PARSE_OBJECT       *SubtreeRoot)
{
    ACPI_PARSE_OBJECT       *Op = SubtreeRoot;
    ACPI_PARSE_OBJECT       *Next = NULL;
    ACPI_PARSE_OBJECT       *Parent = NULL;


    ACPI_FUNCTION_TRACE_PTR (PsDeleteParseTree, SubtreeRoot);


    /* Visit all nodes in the subtree */

    while (Op)
    {
        /* Check if we are not ascending */

        if (Op != Parent)
        {
            /* Look for an argument or child of the current op */

            Next = AcpiPsGetArg (Op, 0);
            if (Next)
            {
                /* Still going downward in tree (Op is not completed yet) */

                Op = Next;
                continue;
            }
        }

        /* No more children, this Op is complete. */

        Next = Op->Common.Next;
        Parent = Op->Common.Parent;

        AcpiPsFreeOp (Op);

        /* If we are back to the starting point, the walk is complete. */

        if (Op == SubtreeRoot)
        {
            return_VOID;
        }

        if (Next)
        {
            Op = Next;
        }
        else
        {
            Op = Parent;
        }
    }

    return_VOID;
}
