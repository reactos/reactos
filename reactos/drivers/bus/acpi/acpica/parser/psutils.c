/******************************************************************************
 *
 * Module Name: psutils - Parser miscellaneous utilities (Parser only)
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
        ACPI_MODULE_NAME    ("psutils")


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsCreateScopeOp
 *
 * PARAMETERS:  None
 *
 * RETURN:      A new Scope object, null on failure
 *
 * DESCRIPTION: Create a Scope and associated namepath op with the root name
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
AcpiPsCreateScopeOp (
    UINT8                   *Aml)
{
    ACPI_PARSE_OBJECT       *ScopeOp;


    ScopeOp = AcpiPsAllocOp (AML_SCOPE_OP, Aml);
    if (!ScopeOp)
    {
        return (NULL);
    }

    ScopeOp->Named.Name = ACPI_ROOT_NAME;
    return (ScopeOp);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsInitOp
 *
 * PARAMETERS:  Op              - A newly allocated Op object
 *              Opcode          - Opcode to store in the Op
 *
 * RETURN:      None
 *
 * DESCRIPTION: Initialize a parse (Op) object
 *
 ******************************************************************************/

void
AcpiPsInitOp (
    ACPI_PARSE_OBJECT       *Op,
    UINT16                  Opcode)
{
    ACPI_FUNCTION_ENTRY ();


    Op->Common.DescriptorType = ACPI_DESC_TYPE_PARSER;
    Op->Common.AmlOpcode = Opcode;

    ACPI_DISASM_ONLY_MEMBERS (strncpy (Op->Common.AmlOpName,
        (AcpiPsGetOpcodeInfo (Opcode))->Name,
        sizeof (Op->Common.AmlOpName)));
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsAllocOp
 *
 * PARAMETERS:  Opcode          - Opcode that will be stored in the new Op
 *              Aml             - Address of the opcode
 *
 * RETURN:      Pointer to the new Op, null on failure
 *
 * DESCRIPTION: Allocate an acpi_op, choose op type (and thus size) based on
 *              opcode. A cache of opcodes is available for the pure
 *              GENERIC_OP, since this is by far the most commonly used.
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT*
AcpiPsAllocOp (
    UINT16                  Opcode,
    UINT8                   *Aml)
{
    ACPI_PARSE_OBJECT       *Op;
    const ACPI_OPCODE_INFO  *OpInfo;
    UINT8                   Flags = ACPI_PARSEOP_GENERIC;


    ACPI_FUNCTION_ENTRY ();


    OpInfo = AcpiPsGetOpcodeInfo (Opcode);

    /* Determine type of ParseOp required */

    if (OpInfo->Flags & AML_DEFER)
    {
        Flags = ACPI_PARSEOP_DEFERRED;
    }
    else if (OpInfo->Flags & AML_NAMED)
    {
        Flags = ACPI_PARSEOP_NAMED_OBJECT;
    }
    else if (Opcode == AML_INT_BYTELIST_OP)
    {
        Flags = ACPI_PARSEOP_BYTELIST;
    }

    /* Allocate the minimum required size object */

    if (Flags == ACPI_PARSEOP_GENERIC)
    {
        /* The generic op (default) is by far the most common (16 to 1) */

        Op = AcpiOsAcquireObject (AcpiGbl_PsNodeCache);
    }
    else
    {
        /* Extended parseop */

        Op = AcpiOsAcquireObject (AcpiGbl_PsNodeExtCache);
    }

    /* Initialize the Op */

    if (Op)
    {
        AcpiPsInitOp (Op, Opcode);
        Op->Common.Aml = Aml;
        Op->Common.Flags = Flags;
    }

    return (Op);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsFreeOp
 *
 * PARAMETERS:  Op              - Op to be freed
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Free an Op object. Either put it on the GENERIC_OP cache list
 *              or actually free it.
 *
 ******************************************************************************/

void
AcpiPsFreeOp (
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_FUNCTION_NAME (PsFreeOp);


    if (Op->Common.AmlOpcode == AML_INT_RETURN_VALUE_OP)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_ALLOCATIONS,
            "Free retval op: %p\n", Op));
    }

    if (Op->Common.Flags & ACPI_PARSEOP_GENERIC)
    {
        (void) AcpiOsReleaseObject (AcpiGbl_PsNodeCache, Op);
    }
    else
    {
        (void) AcpiOsReleaseObject (AcpiGbl_PsNodeExtCache, Op);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    Utility functions
 *
 * DESCRIPTION: Low level character and object functions
 *
 ******************************************************************************/


/*
 * Is "c" a namestring lead character?
 */
BOOLEAN
AcpiPsIsLeadingChar (
    UINT32                  c)
{
    return ((BOOLEAN) (c == '_' || (c >= 'A' && c <= 'Z')));
}


/*
 * Get op's name (4-byte name segment) or 0 if unnamed
 */
UINT32
AcpiPsGetName (
    ACPI_PARSE_OBJECT       *Op)
{

    /* The "generic" object has no name associated with it */

    if (Op->Common.Flags & ACPI_PARSEOP_GENERIC)
    {
        return (0);
    }

    /* Only the "Extended" parse objects have a name */

    return (Op->Named.Name);
}


/*
 * Set op's name
 */
void
AcpiPsSetName (
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  name)
{

    /* The "generic" object has no name associated with it */

    if (Op->Common.Flags & ACPI_PARSEOP_GENERIC)
    {
        return;
    }

    Op->Named.Name = name;
}
