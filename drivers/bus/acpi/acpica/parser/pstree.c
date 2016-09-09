/******************************************************************************
 *
 * Module Name: pstree - Parser op tree manipulation/traversal/search
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
#include "amlcode.h"

#define _COMPONENT          ACPI_PARSER
        ACPI_MODULE_NAME    ("pstree")

/* Local prototypes */

#ifdef ACPI_OBSOLETE_FUNCTIONS
ACPI_PARSE_OBJECT *
AcpiPsGetChild (
    ACPI_PARSE_OBJECT       *op);
#endif


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetArg
 *
 * PARAMETERS:  Op              - Get an argument for this op
 *              Argn            - Nth argument to get
 *
 * RETURN:      The argument (as an Op object). NULL if argument does not exist
 *
 * DESCRIPTION: Get the specified op's argument.
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
AcpiPsGetArg (
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  Argn)
{
    ACPI_PARSE_OBJECT       *Arg = NULL;
    const ACPI_OPCODE_INFO  *OpInfo;


    ACPI_FUNCTION_ENTRY ();

/*
    if (Op->Common.AmlOpcode == AML_INT_CONNECTION_OP)
    {
        return (Op->Common.Value.Arg);
    }
*/
    /* Get the info structure for this opcode */

    OpInfo = AcpiPsGetOpcodeInfo (Op->Common.AmlOpcode);
    if (OpInfo->Class == AML_CLASS_UNKNOWN)
    {
        /* Invalid opcode or ASCII character */

        return (NULL);
    }

    /* Check if this opcode requires argument sub-objects */

    if (!(OpInfo->Flags & AML_HAS_ARGS))
    {
        /* Has no linked argument objects */

        return (NULL);
    }

    /* Get the requested argument object */

    Arg = Op->Common.Value.Arg;
    while (Arg && Argn)
    {
        Argn--;
        Arg = Arg->Common.Next;
    }

    return (Arg);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsAppendArg
 *
 * PARAMETERS:  Op              - Append an argument to this Op.
 *              Arg             - Argument Op to append
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Append an argument to an op's argument list (a NULL arg is OK)
 *
 ******************************************************************************/

void
AcpiPsAppendArg (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_PARSE_OBJECT       *Arg)
{
    ACPI_PARSE_OBJECT       *PrevArg;
    const ACPI_OPCODE_INFO  *OpInfo;


    ACPI_FUNCTION_ENTRY ();


    if (!Op)
    {
        return;
    }

    /* Get the info structure for this opcode */

    OpInfo = AcpiPsGetOpcodeInfo (Op->Common.AmlOpcode);
    if (OpInfo->Class == AML_CLASS_UNKNOWN)
    {
        /* Invalid opcode */

        ACPI_ERROR ((AE_INFO, "Invalid AML Opcode: 0x%2.2X",
            Op->Common.AmlOpcode));
        return;
    }

    /* Check if this opcode requires argument sub-objects */

    if (!(OpInfo->Flags & AML_HAS_ARGS))
    {
        /* Has no linked argument objects */

        return;
    }

    /* Append the argument to the linked argument list */

    if (Op->Common.Value.Arg)
    {
        /* Append to existing argument list */

        PrevArg = Op->Common.Value.Arg;
        while (PrevArg->Common.Next)
        {
            PrevArg = PrevArg->Common.Next;
        }
        PrevArg->Common.Next = Arg;
    }
    else
    {
        /* No argument list, this will be the first argument */

        Op->Common.Value.Arg = Arg;
    }

    /* Set the parent in this arg and any args linked after it */

    while (Arg)
    {
        Arg->Common.Parent = Op;
        Arg = Arg->Common.Next;

        Op->Common.ArgListLength++;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetDepthNext
 *
 * PARAMETERS:  Origin          - Root of subtree to search
 *              Op              - Last (previous) Op that was found
 *
 * RETURN:      Next Op found in the search.
 *
 * DESCRIPTION: Get next op in tree (walking the tree in depth-first order)
 *              Return NULL when reaching "origin" or when walking up from root
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
AcpiPsGetDepthNext (
    ACPI_PARSE_OBJECT       *Origin,
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_PARSE_OBJECT       *Next = NULL;
    ACPI_PARSE_OBJECT       *Parent;
    ACPI_PARSE_OBJECT       *Arg;


    ACPI_FUNCTION_ENTRY ();


    if (!Op)
    {
        return (NULL);
    }

    /* Look for an argument or child */

    Next = AcpiPsGetArg (Op, 0);
    if (Next)
    {
        return (Next);
    }

    /* Look for a sibling */

    Next = Op->Common.Next;
    if (Next)
    {
        return (Next);
    }

    /* Look for a sibling of parent */

    Parent = Op->Common.Parent;

    while (Parent)
    {
        Arg = AcpiPsGetArg (Parent, 0);
        while (Arg && (Arg != Origin) && (Arg != Op))
        {
            Arg = Arg->Common.Next;
        }

        if (Arg == Origin)
        {
            /* Reached parent of origin, end search */

            return (NULL);
        }

        if (Parent->Common.Next)
        {
            /* Found sibling of parent */

            return (Parent->Common.Next);
        }

        Op = Parent;
        Parent = Parent->Common.Parent;
    }

    return (Next);
}


#ifdef ACPI_OBSOLETE_FUNCTIONS
/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetChild
 *
 * PARAMETERS:  Op              - Get the child of this Op
 *
 * RETURN:      Child Op, Null if none is found.
 *
 * DESCRIPTION: Get op's children or NULL if none
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
AcpiPsGetChild (
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_PARSE_OBJECT       *Child = NULL;


    ACPI_FUNCTION_ENTRY ();


    switch (Op->Common.AmlOpcode)
    {
    case AML_SCOPE_OP:
    case AML_ELSE_OP:
    case AML_DEVICE_OP:
    case AML_THERMAL_ZONE_OP:
    case AML_INT_METHODCALL_OP:

        Child = AcpiPsGetArg (Op, 0);
        break;

    case AML_BUFFER_OP:
    case AML_PACKAGE_OP:
    case AML_METHOD_OP:
    case AML_IF_OP:
    case AML_WHILE_OP:
    case AML_FIELD_OP:

        Child = AcpiPsGetArg (Op, 1);
        break;

    case AML_POWER_RES_OP:
    case AML_INDEX_FIELD_OP:

        Child = AcpiPsGetArg (Op, 2);
        break;

    case AML_PROCESSOR_OP:
    case AML_BANK_FIELD_OP:

        Child = AcpiPsGetArg (Op, 3);
        break;

    default:

        /* All others have no children */

        break;
    }

    return (Child);
}
#endif
