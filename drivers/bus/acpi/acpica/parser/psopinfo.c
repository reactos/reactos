/******************************************************************************
 *
 * Module Name: psopinfo - AML opcode information functions and dispatch tables
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

#include "acpi.h"
#include "accommon.h"
#include "acparser.h"
#include "acopcode.h"
#include "amlcode.h"


#define _COMPONENT          ACPI_PARSER
        ACPI_MODULE_NAME    ("psopinfo")


static const UINT8      AcpiGbl_ArgumentCount[] = {0,1,1,1,1,2,2,2,2,3,3,6};


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetOpcodeInfo
 *
 * PARAMETERS:  Opcode              - The AML opcode
 *
 * RETURN:      A pointer to the info about the opcode.
 *
 * DESCRIPTION: Find AML opcode description based on the opcode.
 *              NOTE: This procedure must ALWAYS return a valid pointer!
 *
 ******************************************************************************/

const ACPI_OPCODE_INFO *
AcpiPsGetOpcodeInfo (
    UINT16                  Opcode)
{
#ifdef ACPI_DEBUG_OUTPUT
    const char              *OpcodeName = "Unknown AML opcode";
#endif

    ACPI_FUNCTION_NAME (PsGetOpcodeInfo);


    /*
     * Detect normal 8-bit opcode or extended 16-bit opcode
     */
    if (!(Opcode & 0xFF00))
    {
        /* Simple (8-bit) opcode: 0-255, can't index beyond table  */

        return (&AcpiGbl_AmlOpInfo [AcpiGbl_ShortOpIndex [(UINT8) Opcode]]);
    }

    if (((Opcode & 0xFF00) == AML_EXTENDED_OPCODE) &&
        (((UINT8) Opcode) <= MAX_EXTENDED_OPCODE))
    {
        /* Valid extended (16-bit) opcode */

        return (&AcpiGbl_AmlOpInfo [AcpiGbl_LongOpIndex [(UINT8) Opcode]]);
    }

#if defined ACPI_ASL_COMPILER && defined ACPI_DEBUG_OUTPUT
#include "asldefine.h"

    switch (Opcode)
    {
    case AML_RAW_DATA_BYTE:
        OpcodeName = "-Raw Data Byte-";
        break;

    case AML_RAW_DATA_WORD:
        OpcodeName = "-Raw Data Word-";
        break;

    case AML_RAW_DATA_DWORD:
        OpcodeName = "-Raw Data Dword-";
        break;

    case AML_RAW_DATA_QWORD:
        OpcodeName = "-Raw Data Qword-";
        break;

    case AML_RAW_DATA_BUFFER:
        OpcodeName = "-Raw Data Buffer-";
        break;

    case AML_RAW_DATA_CHAIN:
        OpcodeName = "-Raw Data Buffer Chain-";
        break;

    case AML_PACKAGE_LENGTH:
        OpcodeName = "-Package Length-";
        break;

    case AML_UNASSIGNED_OPCODE:
        OpcodeName = "-Unassigned Opcode-";
        break;

    case AML_DEFAULT_ARG_OP:
        OpcodeName = "-Default Arg-";
        break;

    default:
        break;
    }
#endif

    /* Unknown AML opcode */

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
        "%s [%4.4X]\n", OpcodeName, Opcode));

    return (&AcpiGbl_AmlOpInfo [_UNK]);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetOpcodeName
 *
 * PARAMETERS:  Opcode              - The AML opcode
 *
 * RETURN:      A pointer to the name of the opcode (ASCII String)
 *              Note: Never returns NULL.
 *
 * DESCRIPTION: Translate an opcode into a human-readable string
 *
 ******************************************************************************/

const char *
AcpiPsGetOpcodeName (
    UINT16                  Opcode)
{
#if defined(ACPI_DISASSEMBLER) || defined (ACPI_DEBUG_OUTPUT)

    const ACPI_OPCODE_INFO  *Op;


    Op = AcpiPsGetOpcodeInfo (Opcode);

    /* Always guaranteed to return a valid pointer */

    return (Op->Name);

#else
    return ("OpcodeName unavailable");

#endif
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetArgumentCount
 *
 * PARAMETERS:  OpType              - Type associated with the AML opcode
 *
 * RETURN:      Argument count
 *
 * DESCRIPTION: Obtain the number of expected arguments for an AML opcode
 *
 ******************************************************************************/

UINT8
AcpiPsGetArgumentCount (
    UINT32                  OpType)
{

    if (OpType <= AML_TYPE_EXEC_6A_0T_1R)
    {
        return (AcpiGbl_ArgumentCount[OpType]);
    }

    return (0);
}


/*
 * This table is directly indexed by the opcodes It returns
 * an index into the opcode table (AcpiGbl_AmlOpInfo)
 */
const UINT8 AcpiGbl_ShortOpIndex[256] =
{
/*              0     1     2     3     4     5     6     7  */
/*              8     9     A     B     C     D     E     F  */
/* 0x00 */    0x00, 0x01, _UNK, _UNK, _UNK, _UNK, 0x02, _UNK,
/* 0x08 */    0x03, _UNK, 0x04, 0x05, 0x06, 0x07, 0x6E, _UNK,
/* 0x10 */    0x08, 0x09, 0x0a, 0x6F, 0x0b, 0x81, _UNK, _UNK,
/* 0x18 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x20 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x28 */    _UNK, _UNK, _UNK, _UNK, _UNK, 0x63, _PFX, _PFX,
/* 0x30 */    0x67, 0x66, 0x68, 0x65, 0x69, 0x64, 0x6A, 0x7D,
/* 0x38 */    0x7F, 0x80, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x40 */    _UNK, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC,
/* 0x48 */    _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC,
/* 0x50 */    _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC,
/* 0x58 */    _ASC, _ASC, _ASC, _UNK, _PFX, _UNK, _PFX, _ASC,
/* 0x60 */    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
/* 0x68 */    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, _UNK,
/* 0x70 */    0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22,
/* 0x78 */    0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a,
/* 0x80 */    0x2b, 0x2c, 0x2d, 0x2e, 0x70, 0x71, 0x2f, 0x30,
/* 0x88 */    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x72,
/* 0x90 */    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x73, 0x74,
/* 0x98 */    0x75, 0x76, _UNK, _UNK, 0x77, 0x78, 0x79, 0x7A,
/* 0xA0 */    0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x60, 0x61,
/* 0xA8 */    0x62, 0x82, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xB0 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xB8 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xC0 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xC8 */    _UNK, _UNK, _UNK, _UNK, 0x44, _UNK, _UNK, _UNK,
/* 0xD0 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xD8 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xE0 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xE8 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xF0 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xF8 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, 0x45,
};

/*
 * This table is indexed by the second opcode of the extended opcode
 * pair. It returns an index into the opcode table (AcpiGbl_AmlOpInfo)
 */
const UINT8 AcpiGbl_LongOpIndex[NUM_EXTENDED_OPCODE] =
{
/*              0     1     2     3     4     5     6     7  */
/*              8     9     A     B     C     D     E     F  */
/* 0x00 */    _UNK, 0x46, 0x47, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x08 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x10 */    _UNK, _UNK, 0x48, 0x49, _UNK, _UNK, _UNK, _UNK,
/* 0x18 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, 0x7B,
/* 0x20 */    0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51,
/* 0x28 */    0x52, 0x53, 0x54, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x30 */    0x55, 0x56, 0x57, 0x7e, _UNK, _UNK, _UNK, _UNK,
/* 0x38 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x40 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x48 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x50 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x58 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x60 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x68 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x70 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x78 */    _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x80 */    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
/* 0x88 */    0x7C,
};
