/******************************************************************************
 *
 * Module Name: acapps - common include for ACPI applications/tools
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

#ifndef _ACCONVERT
#define _ACCONVERT

/* Definitions for comment state */

#define ASL_COMMENT_STANDARD    1
#define ASLCOMMENT_INLINE       2
#define ASL_COMMENT_OPEN_PAREN  3
#define ASL_COMMENT_CLOSE_PAREN 4
#define ASL_COMMENT_CLOSE_BRACE 5

/* Definitions for comment print function*/

#define AML_COMMENT_STANDARD    1
#define AMLCOMMENT_INLINE       2
#define AML_COMMENT_END_NODE    3
#define AML_NAMECOMMENT         4
#define AML_COMMENT_CLOSE_BRACE 5
#define AML_COMMENT_ENDBLK      6
#define AML_COMMENT_INCLUDE     7


#ifdef ACPI_ASL_COMPILER
/*
 * cvcompiler
 */
void
CvProcessComment (
    ASL_COMMENT_STATE       CurrentState,
    char                    *StringBuffer,
    int                     c1);

void
CvProcessCommentType2 (
    ASL_COMMENT_STATE       CurrentState,
    char                    *StringBuffer);

UINT32
CvCalculateCommentLengths(
   ACPI_PARSE_OBJECT        *Op);

void
CvProcessCommentState (
    char                    input);

char*
CvAppendInlineComment (
    char                    *InlineComment,
    char                    *ToAdd);

void
CvAddToCommentList (
    char*                   ToAdd);

void
CvPlaceComment (
    UINT8                   Type,
    char                    *CommentString);

UINT32
CvParseOpBlockType (
    ACPI_PARSE_OBJECT       *Op);

ACPI_COMMENT_NODE*
CvCommentNodeCalloc (
    void);

void
CgWriteAmlDefBlockComment (
    ACPI_PARSE_OBJECT       *Op);

void
CgWriteOneAmlComment (
    ACPI_PARSE_OBJECT       *Op,
    char*                   CommentToPrint,
    UINT8                   InputOption);

void
CgWriteAmlComment (
    ACPI_PARSE_OBJECT       *Op);


/*
 * cvparser
 */
void
CvInitFileTree (
    ACPI_TABLE_HEADER       *Table,
    FILE                    *RootFile);

void
CvClearOpComments (
    ACPI_PARSE_OBJECT       *Op);

ACPI_FILE_NODE*
CvFilenameExists (
    char                    *Filename,
    ACPI_FILE_NODE           *Head);

void
CvLabelFileNode (
    ACPI_PARSE_OBJECT       *Op);

void
CvCaptureListComments (
    ACPI_PARSE_STATE        *ParserState,
    ACPI_COMMENT_NODE       *ListHead,
    ACPI_COMMENT_NODE       *ListTail);

void
CvCaptureCommentsOnly (
    ACPI_PARSE_STATE        *ParserState);

void
CvCaptureComments (
    ACPI_WALK_STATE         *WalkState);

void
CvTransferComments (
    ACPI_PARSE_OBJECT       *Op);

/*
 * cvdisasm
 */
void
CvSwitchFiles (
    UINT32                  level,
    ACPI_PARSE_OBJECT       *op);

BOOLEAN
CvFileHasSwitched (
    ACPI_PARSE_OBJECT       *Op);


void
CvCloseParenWriteComment (
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  Level);

void
CvCloseBraceWriteComment (
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  Level);

void
CvPrintOneCommentList (
    ACPI_COMMENT_NODE       *CommentList,
    UINT32                  Level);

void
CvPrintOneCommentType (
    ACPI_PARSE_OBJECT       *Op,
    UINT8                   CommentType,
    char*                   EndStr,
    UINT32                  Level);


#endif

#endif /* _ACCONVERT */
