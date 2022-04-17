/******************************************************************************
 *
 * Name: acdebug.h - ACPI/AML debugger
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

#ifndef __ACDEBUG_H__
#define __ACDEBUG_H__

/* The debugger is used in conjunction with the disassembler most of time */

#ifdef ACPI_DISASSEMBLER
#include "acdisasm.h"
#endif


#define ACPI_DEBUG_BUFFER_SIZE      0x4000      /* 16K buffer for return objects */
#define ACPI_DEBUG_LENGTH_FORMAT    " (%.4X bits, %.3X bytes)"

typedef struct acpi_db_command_info
{
    const char              *Name;          /* Command Name */
    UINT8                   MinArgs;        /* Minimum arguments required */

} ACPI_DB_COMMAND_INFO;

typedef struct acpi_db_command_help
{
    UINT8                   LineCount;      /* Number of help lines */
    char                    *Invocation;    /* Command Invocation */
    char                    *Description;   /* Command Description */

} ACPI_DB_COMMAND_HELP;

typedef struct acpi_db_argument_info
{
    const char              *Name;          /* Argument Name */

} ACPI_DB_ARGUMENT_INFO;

typedef struct acpi_db_execute_walk
{
    UINT32                  Count;
    UINT32                  MaxCount;
    char                    NameSeg[ACPI_NAMESEG_SIZE + 1];

} ACPI_DB_EXECUTE_WALK;


#define PARAM_LIST(pl)                  pl

#define EX_NO_SINGLE_STEP               1
#define EX_SINGLE_STEP                  2
#define EX_ALL                          4


/*
 * dbxface - external debugger interfaces
 */
ACPI_DBR_DEPENDENT_RETURN_OK (
ACPI_STATUS
AcpiDbSingleStep (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  OpType))

ACPI_DBR_DEPENDENT_RETURN_VOID (
void
AcpiDbSignalBreakPoint (
    ACPI_WALK_STATE         *WalkState))


/*
 * dbcmds - debug commands and output routines
 */
ACPI_NAMESPACE_NODE *
AcpiDbConvertToNode (
    char                    *InString);

void
AcpiDbDisplayTableInfo (
    char                    *TableArg);

void
AcpiDbDisplayTemplate (
    char                    *BufferArg);

void
AcpiDbUnloadAcpiTable (
    char                    *Name);

void
AcpiDbSendNotify (
    char                    *Name,
    UINT32                  Value);

void
AcpiDbDisplayInterfaces (
    char                    *ActionArg,
    char                    *InterfaceNameArg);

ACPI_STATUS
AcpiDbSleep (
    char                    *ObjectArg);

void
AcpiDbTrace (
    char                    *EnableArg,
    char                    *MethodArg,
    char                    *OnceArg);

void
AcpiDbDisplayLocks (
    void);

void
AcpiDbDisplayResources (
    char                    *ObjectArg);

ACPI_HW_DEPENDENT_RETURN_VOID (
void
AcpiDbDisplayGpes (
    void))

void
AcpiDbDisplayHandlers (
    void);

ACPI_HW_DEPENDENT_RETURN_VOID (
void
AcpiDbGenerateGpe (
    char                    *GpeArg,
    char                    *BlockArg))

ACPI_HW_DEPENDENT_RETURN_VOID (
void
AcpiDbGenerateSci (
    void))

void
AcpiDbExecuteTest (
    char                    *TypeArg);


/*
 * dbconvert - miscellaneous conversion routines
 */
ACPI_STATUS
AcpiDbHexCharToValue (
    int                     HexChar,
    UINT8                   *ReturnValue);

ACPI_STATUS
AcpiDbConvertToPackage (
    char                    *String,
    ACPI_OBJECT             *Object);

ACPI_STATUS
AcpiDbConvertToObject (
    ACPI_OBJECT_TYPE        Type,
    char                    *String,
    ACPI_OBJECT             *Object);

UINT8 *
AcpiDbEncodePldBuffer (
    ACPI_PLD_INFO           *PldInfo);

void
AcpiDbDumpPldBuffer (
    ACPI_OBJECT             *ObjDesc);


/*
 * dbmethod - control method commands
 */
void
AcpiDbSetMethodBreakpoint (
    char                    *Location,
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDbSetMethodCallBreakpoint (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDbSetMethodData (
    char                    *TypeArg,
    char                    *IndexArg,
    char                    *ValueArg);

ACPI_STATUS
AcpiDbDisassembleMethod (
    char                    *Name);

void
AcpiDbDisassembleAml (
    char                    *Statements,
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDbEvaluatePredefinedNames (
    void);

void
AcpiDbEvaluateAll (
    char                    *NameSeg);


/*
 * dbnames - namespace commands
 */
void
AcpiDbSetScope (
    char                    *Name);

void
AcpiDbDumpNamespace (
    char                    *StartArg,
    char                    *DepthArg);

void
AcpiDbDumpNamespacePaths (
    void);

void
AcpiDbDumpNamespaceByOwner (
    char                    *OwnerArg,
    char                    *DepthArg);

ACPI_STATUS
AcpiDbFindNameInNamespace (
    char                    *NameArg);

void
AcpiDbCheckPredefinedNames (
    void);

ACPI_STATUS
AcpiDbDisplayObjects (
    char                    *ObjTypeArg,
    char                    *DisplayCountArg);

void
AcpiDbCheckIntegrity (
    void);

void
AcpiDbFindReferences (
    char                    *ObjectArg);

void
AcpiDbGetBusInfo (
    void);

ACPI_STATUS
AcpiDbDisplayFields (
    UINT32                  AddressSpaceId);


/*
 * dbdisply - debug display commands
 */
void
AcpiDbDisplayMethodInfo (
    ACPI_PARSE_OBJECT       *Op);

void
AcpiDbDecodeAndDisplayObject (
    char                    *Target,
    char                    *OutputType);

ACPI_DBR_DEPENDENT_RETURN_VOID (
void
AcpiDbDisplayResultObject (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState))

ACPI_STATUS
AcpiDbDisplayAllMethods (
    char                    *DisplayCountArg);

void
AcpiDbDisplayArguments (
    void);

void
AcpiDbDisplayLocals (
    void);

void
AcpiDbDisplayResults (
    void);

void
AcpiDbDisplayCallingTree (
    void);

void
AcpiDbDisplayObjectType (
    char                    *ObjectArg);

ACPI_DBR_DEPENDENT_RETURN_VOID (
void
AcpiDbDisplayArgumentObject (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState))


/*
 * dbexec - debugger control method execution
 */
void
AcpiDbExecute (
    char                    *Name,
    char                    **Args,
    ACPI_OBJECT_TYPE        *Types,
    UINT32                  Flags);

void
AcpiDbCreateExecutionThread (
    char                    *MethodNameArg,
    char                    **Arguments,
    ACPI_OBJECT_TYPE        *Types);

void
AcpiDbCreateExecutionThreads (
    char                    *NumThreadsArg,
    char                    *NumLoopsArg,
    char                    *MethodNameArg);

void
AcpiDbDeleteObjects (
    UINT32                  Count,
    ACPI_OBJECT             *Objects);

#ifdef ACPI_DBG_TRACK_ALLOCATIONS
UINT32
AcpiDbGetCacheInfo (
    ACPI_MEMORY_LIST        *Cache);
#endif


/*
 * dbfileio - Debugger file I/O commands
 */
ACPI_OBJECT_TYPE
AcpiDbMatchArgument (
    char                    *UserArgument,
    ACPI_DB_ARGUMENT_INFO   *Arguments);

void
AcpiDbCloseDebugFile (
    void);

void
AcpiDbOpenDebugFile (
    char                    *Name);

ACPI_STATUS
AcpiDbLoadAcpiTable (
    char                    *Filename);

ACPI_STATUS
AcpiDbLoadTables (
    ACPI_NEW_TABLE_DESC     *ListHead);


/*
 * dbhistry - debugger HISTORY command
 */
void
AcpiDbAddToHistory (
    char                    *CommandLine);

void
AcpiDbDisplayHistory (
    void);

char *
AcpiDbGetFromHistory (
    char                    *CommandNumArg);

char *
AcpiDbGetHistoryByIndex (
    UINT32                  CommanddNum);


/*
 * dbinput - user front-end to the AML debugger
 */
ACPI_STATUS
AcpiDbCommandDispatch (
    char                    *InputBuffer,
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op);

void ACPI_SYSTEM_XFACE
AcpiDbExecuteThread (
    void                    *Context);

ACPI_STATUS
AcpiDbUserCommands (
    void);

char *
AcpiDbGetNextToken (
    char                    *String,
    char                    **Next,
    ACPI_OBJECT_TYPE        *ReturnType);


/*
 * dbobject
 */
void
AcpiDbDecodeInternalObject (
    ACPI_OPERAND_OBJECT     *ObjDesc);

void
AcpiDbDisplayInternalObject (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState);

void
AcpiDbDecodeArguments (
    ACPI_WALK_STATE         *WalkState);

void
AcpiDbDecodeLocals (
    ACPI_WALK_STATE         *WalkState);

void
AcpiDbDumpMethodInfo (
    ACPI_STATUS             Status,
    ACPI_WALK_STATE         *WalkState);


/*
 * dbstats - Generation and display of ACPI table statistics
 */
void
AcpiDbGenerateStatistics (
    ACPI_PARSE_OBJECT       *Root,
    BOOLEAN                 IsMethod);

ACPI_STATUS
AcpiDbDisplayStatistics (
    char                    *TypeArg);


/*
 * dbutils - AML debugger utilities
 */
void
AcpiDbSetOutputDestination (
    UINT32                  Where);

void
AcpiDbDumpExternalObject (
    ACPI_OBJECT             *ObjDesc,
    UINT32                  Level);

void
AcpiDbPrepNamestring (
    char                    *Name);

ACPI_NAMESPACE_NODE *
AcpiDbLocalNsLookup (
    char                    *Name);

void
AcpiDbUint32ToHexString (
    UINT32                  Value,
    char                    *Buffer);

#endif  /* __ACDEBUG_H__ */
