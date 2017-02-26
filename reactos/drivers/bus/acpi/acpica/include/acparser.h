/******************************************************************************
 *
 * Module Name: acparser.h - AML Parser subcomponent prototypes and defines
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

#ifndef __ACPARSER_H__
#define __ACPARSER_H__


#define OP_HAS_RETURN_VALUE             1

/* Variable number of arguments. This field must be 32 bits */

#define ACPI_VAR_ARGS                   ACPI_UINT32_MAX


#define ACPI_PARSE_DELETE_TREE          0x0001
#define ACPI_PARSE_NO_TREE_DELETE       0x0000
#define ACPI_PARSE_TREE_MASK            0x0001

#define ACPI_PARSE_LOAD_PASS1           0x0010
#define ACPI_PARSE_LOAD_PASS2           0x0020
#define ACPI_PARSE_EXECUTE              0x0030
#define ACPI_PARSE_MODE_MASK            0x0030

#define ACPI_PARSE_DEFERRED_OP          0x0100
#define ACPI_PARSE_DISASSEMBLE          0x0200

#define ACPI_PARSE_MODULE_LEVEL         0x0400

/******************************************************************************
 *
 * Parser interfaces
 *
 *****************************************************************************/

extern const UINT8      AcpiGbl_ShortOpIndex[];
extern const UINT8      AcpiGbl_LongOpIndex[];


/*
 * psxface - Parser external interfaces
 */
ACPI_STATUS
AcpiPsExecuteMethod (
    ACPI_EVALUATE_INFO      *Info);

ACPI_STATUS
AcpiPsExecuteTable (
    ACPI_EVALUATE_INFO      *Info);


/*
 * psargs - Parse AML opcode arguments
 */
UINT8 *
AcpiPsGetNextPackageEnd (
    ACPI_PARSE_STATE        *ParserState);

char *
AcpiPsGetNextNamestring (
    ACPI_PARSE_STATE        *ParserState);

void
AcpiPsGetNextSimpleArg (
    ACPI_PARSE_STATE        *ParserState,
    UINT32                  ArgType,
    ACPI_PARSE_OBJECT       *Arg);

ACPI_STATUS
AcpiPsGetNextNamepath (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_STATE        *ParserState,
    ACPI_PARSE_OBJECT       *Arg,
    BOOLEAN                 PossibleMethodCall);

/* Values for BOOLEAN above */

#define ACPI_NOT_METHOD_CALL            FALSE
#define ACPI_POSSIBLE_METHOD_CALL       TRUE

ACPI_STATUS
AcpiPsGetNextArg (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_STATE        *ParserState,
    UINT32                  ArgType,
    ACPI_PARSE_OBJECT       **ReturnArg);


/*
 * psfind
 */
ACPI_PARSE_OBJECT *
AcpiPsFindName (
    ACPI_PARSE_OBJECT       *Scope,
    UINT32                  Name,
    UINT32                  Opcode);

ACPI_PARSE_OBJECT*
AcpiPsGetParent (
    ACPI_PARSE_OBJECT       *Op);


/*
 * psobject - support for parse object processing
 */
ACPI_STATUS
AcpiPsBuildNamedOp (
    ACPI_WALK_STATE         *WalkState,
    UINT8                   *AmlOpStart,
    ACPI_PARSE_OBJECT       *UnnamedOp,
    ACPI_PARSE_OBJECT       **Op);

ACPI_STATUS
AcpiPsCreateOp (
    ACPI_WALK_STATE         *WalkState,
    UINT8                   *AmlOpStart,
    ACPI_PARSE_OBJECT       **NewOp);

ACPI_STATUS
AcpiPsCompleteOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       **Op,
    ACPI_STATUS             Status);

ACPI_STATUS
AcpiPsCompleteFinalOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    ACPI_STATUS             Status);


/*
 * psopinfo - AML Opcode information
 */
const ACPI_OPCODE_INFO *
AcpiPsGetOpcodeInfo (
    UINT16                  Opcode);

const char *
AcpiPsGetOpcodeName (
    UINT16                  Opcode);

UINT8
AcpiPsGetArgumentCount (
    UINT32                  OpType);


/*
 * psparse - top level parsing routines
 */
ACPI_STATUS
AcpiPsParseAml (
    ACPI_WALK_STATE         *WalkState);

UINT32
AcpiPsGetOpcodeSize (
    UINT32                  Opcode);

UINT16
AcpiPsPeekOpcode (
    ACPI_PARSE_STATE        *state);

ACPI_STATUS
AcpiPsCompleteThisOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op);

ACPI_STATUS
AcpiPsNextParseState (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    ACPI_STATUS             CallbackStatus);


/*
 * psloop - main parse loop
 */
ACPI_STATUS
AcpiPsParseLoop (
    ACPI_WALK_STATE         *WalkState);


/*
 * psscope - Scope stack management routines
 */
ACPI_STATUS
AcpiPsInitScope (
    ACPI_PARSE_STATE        *ParserState,
    ACPI_PARSE_OBJECT       *Root);

ACPI_PARSE_OBJECT *
AcpiPsGetParentScope (
    ACPI_PARSE_STATE        *state);

BOOLEAN
AcpiPsHasCompletedScope (
    ACPI_PARSE_STATE        *ParserState);

void
AcpiPsPopScope (
    ACPI_PARSE_STATE        *ParserState,
    ACPI_PARSE_OBJECT       **Op,
    UINT32                  *ArgList,
    UINT32                  *ArgCount);

ACPI_STATUS
AcpiPsPushScope (
    ACPI_PARSE_STATE        *ParserState,
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  RemainingArgs,
    UINT32                  ArgCount);

void
AcpiPsCleanupScope (
    ACPI_PARSE_STATE        *state);


/*
 * pstree - parse tree manipulation routines
 */
void
AcpiPsAppendArg(
    ACPI_PARSE_OBJECT       *op,
    ACPI_PARSE_OBJECT       *arg);

ACPI_PARSE_OBJECT*
AcpiPsFind (
    ACPI_PARSE_OBJECT       *Scope,
    char                    *Path,
    UINT16                  Opcode,
    UINT32                  Create);

ACPI_PARSE_OBJECT *
AcpiPsGetArg(
    ACPI_PARSE_OBJECT       *op,
    UINT32                   argn);

ACPI_PARSE_OBJECT *
AcpiPsGetDepthNext (
    ACPI_PARSE_OBJECT       *Origin,
    ACPI_PARSE_OBJECT       *Op);


/*
 * pswalk - parse tree walk routines
 */
ACPI_STATUS
AcpiPsWalkParsedAml (
    ACPI_PARSE_OBJECT       *StartOp,
    ACPI_PARSE_OBJECT       *EndOp,
    ACPI_OPERAND_OBJECT     *MthDesc,
    ACPI_NAMESPACE_NODE     *StartNode,
    ACPI_OPERAND_OBJECT     **Params,
    ACPI_OPERAND_OBJECT     **CallerReturnDesc,
    ACPI_OWNER_ID           OwnerId,
    ACPI_PARSE_DOWNWARDS    DescendingCallback,
    ACPI_PARSE_UPWARDS      AscendingCallback);

ACPI_STATUS
AcpiPsGetNextWalkOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    ACPI_PARSE_UPWARDS      AscendingCallback);

ACPI_STATUS
AcpiPsDeleteCompletedOp (
    ACPI_WALK_STATE         *WalkState);

void
AcpiPsDeleteParseTree (
    ACPI_PARSE_OBJECT       *root);


/*
 * psutils - parser utilities
 */
ACPI_PARSE_OBJECT *
AcpiPsCreateScopeOp (
    UINT8                   *Aml);

void
AcpiPsInitOp (
    ACPI_PARSE_OBJECT       *op,
    UINT16                  opcode);

ACPI_PARSE_OBJECT *
AcpiPsAllocOp (
    UINT16                  Opcode,
    UINT8                   *Aml);

void
AcpiPsFreeOp (
    ACPI_PARSE_OBJECT       *Op);

BOOLEAN
AcpiPsIsLeadingChar (
    UINT32                  c);

UINT32
AcpiPsGetName(
    ACPI_PARSE_OBJECT       *op);

void
AcpiPsSetName(
    ACPI_PARSE_OBJECT       *op,
    UINT32                  name);


/*
 * psdump - display parser tree
 */
UINT32
AcpiPsSprintPath (
    char                    *BufferStart,
    UINT32                  BufferSize,
    ACPI_PARSE_OBJECT       *Op);

UINT32
AcpiPsSprintOp (
    char                    *BufferStart,
    UINT32                  BufferSize,
    ACPI_PARSE_OBJECT       *Op);

void
AcpiPsShow (
    ACPI_PARSE_OBJECT       *op);


#endif /* __ACPARSER_H__ */
