/******************************************************************************
 *
 * Name: acnamesp.h - Namespace subcomponent prototypes and defines
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

#ifndef __ACNAMESP_H__
#define __ACNAMESP_H__


/* To search the entire name space, pass this as SearchBase */

#define ACPI_NS_ALL                 ((ACPI_HANDLE)0)

/*
 * Elements of AcpiNsProperties are bit significant
 * and should be one-to-one with values of ACPI_OBJECT_TYPE
 */
#define ACPI_NS_NORMAL              0
#define ACPI_NS_NEWSCOPE            1   /* a definition of this type opens a name scope */
#define ACPI_NS_LOCAL               2   /* suppress search of enclosing scopes */

/* Flags for AcpiNsLookup, AcpiNsSearchAndEnter */

#define ACPI_NS_NO_UPSEARCH         0
#define ACPI_NS_SEARCH_PARENT       0x01
#define ACPI_NS_DONT_OPEN_SCOPE     0x02
#define ACPI_NS_NO_PEER_SEARCH      0x04
#define ACPI_NS_ERROR_IF_FOUND      0x08
#define ACPI_NS_PREFIX_IS_SCOPE     0x10
#define ACPI_NS_EXTERNAL            0x20
#define ACPI_NS_TEMPORARY           0x40
#define ACPI_NS_OVERRIDE_IF_FOUND   0x80

/* Flags for AcpiNsWalkNamespace */

#define ACPI_NS_WALK_NO_UNLOCK      0
#define ACPI_NS_WALK_UNLOCK         0x01
#define ACPI_NS_WALK_TEMP_NODES     0x02

/* Object is not a package element */

#define ACPI_NOT_PACKAGE_ELEMENT    ACPI_UINT32_MAX
#define ACPI_ALL_PACKAGE_ELEMENTS   (ACPI_UINT32_MAX-1)

/* Always emit warning message, not dependent on node flags */

#define ACPI_WARN_ALWAYS            0


/*
 * nsinit - Namespace initialization
 */
ACPI_STATUS
AcpiNsInitializeObjects (
    void);

ACPI_STATUS
AcpiNsInitializeDevices (
    UINT32                  Flags);


/*
 * nsload -  Namespace loading
 */
ACPI_STATUS
AcpiNsLoadNamespace (
    void);

ACPI_STATUS
AcpiNsLoadTable (
    UINT32                  TableIndex,
    ACPI_NAMESPACE_NODE     *Node);


/*
 * nswalk - walk the namespace
 */
ACPI_STATUS
AcpiNsWalkNamespace (
    ACPI_OBJECT_TYPE        Type,
    ACPI_HANDLE             StartObject,
    UINT32                  MaxDepth,
    UINT32                  Flags,
    ACPI_WALK_CALLBACK      DescendingCallback,
    ACPI_WALK_CALLBACK      AscendingCallback,
    void                    *Context,
    void                    **ReturnValue);

ACPI_NAMESPACE_NODE *
AcpiNsGetNextNode (
    ACPI_NAMESPACE_NODE     *Parent,
    ACPI_NAMESPACE_NODE     *Child);

ACPI_NAMESPACE_NODE *
AcpiNsGetNextNodeTyped (
    ACPI_OBJECT_TYPE        Type,
    ACPI_NAMESPACE_NODE     *Parent,
    ACPI_NAMESPACE_NODE     *Child);

/*
 * nsparse - table parsing
 */
ACPI_STATUS
AcpiNsParseTable (
    UINT32                  TableIndex,
    ACPI_NAMESPACE_NODE     *StartNode);

ACPI_STATUS
AcpiNsExecuteTable (
    UINT32                  TableIndex,
    ACPI_NAMESPACE_NODE     *StartNode);

ACPI_STATUS
AcpiNsOneCompleteParse (
    UINT32                  PassNumber,
    UINT32                  TableIndex,
    ACPI_NAMESPACE_NODE     *StartNode);


/*
 * nsaccess - Top-level namespace access
 */
ACPI_STATUS
AcpiNsRootInitialize (
    void);

ACPI_STATUS
AcpiNsLookup (
    ACPI_GENERIC_STATE      *ScopeInfo,
    char                    *Name,
    ACPI_OBJECT_TYPE        Type,
    ACPI_INTERPRETER_MODE   InterpreterMode,
    UINT32                  Flags,
    ACPI_WALK_STATE         *WalkState,
    ACPI_NAMESPACE_NODE     **RetNode);


/*
 * nsalloc - Named object allocation/deallocation
 */
ACPI_NAMESPACE_NODE *
AcpiNsCreateNode (
    UINT32                  Name);

void
AcpiNsDeleteNode (
    ACPI_NAMESPACE_NODE     *Node);

void
AcpiNsRemoveNode (
    ACPI_NAMESPACE_NODE     *Node);

void
AcpiNsDeleteNamespaceSubtree (
    ACPI_NAMESPACE_NODE     *ParentHandle);

void
AcpiNsDeleteNamespaceByOwner (
    ACPI_OWNER_ID           OwnerId);

void
AcpiNsDetachObject (
    ACPI_NAMESPACE_NODE     *Node);

void
AcpiNsDeleteChildren (
    ACPI_NAMESPACE_NODE     *Parent);

int
AcpiNsCompareNames (
    char                    *Name1,
    char                    *Name2);


/*
 * nsconvert - Dynamic object conversion routines
 */
ACPI_STATUS
AcpiNsConvertToInteger (
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ReturnObject);

ACPI_STATUS
AcpiNsConvertToString (
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ReturnObject);

ACPI_STATUS
AcpiNsConvertToBuffer (
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ReturnObject);

ACPI_STATUS
AcpiNsConvertToUnicode (
    ACPI_NAMESPACE_NODE     *Scope,
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ReturnObject);

ACPI_STATUS
AcpiNsConvertToResource (
    ACPI_NAMESPACE_NODE     *Scope,
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ReturnObject);

ACPI_STATUS
AcpiNsConvertToReference (
    ACPI_NAMESPACE_NODE     *Scope,
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ReturnObject);


/*
 * nsdump - Namespace dump/print utilities
 */
void
AcpiNsDumpTables (
    ACPI_HANDLE             SearchBase,
    UINT32                  MaxDepth);

void
AcpiNsDumpEntry (
    ACPI_HANDLE             Handle,
    UINT32                  DebugLevel);

void
AcpiNsDumpPathname (
    ACPI_HANDLE             Handle,
    const char              *Msg,
    UINT32                  Level,
    UINT32                  Component);

void
AcpiNsPrintPathname (
    UINT32                  NumSegments,
    const char              *Pathname);

ACPI_STATUS
AcpiNsDumpOneObject (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue);

void
AcpiNsDumpObjects (
    ACPI_OBJECT_TYPE        Type,
    UINT8                   DisplayType,
    UINT32                  MaxDepth,
    ACPI_OWNER_ID           OwnerId,
    ACPI_HANDLE             StartHandle);

void
AcpiNsDumpObjectPaths (
    ACPI_OBJECT_TYPE        Type,
    UINT8                   DisplayType,
    UINT32                  MaxDepth,
    ACPI_OWNER_ID           OwnerId,
    ACPI_HANDLE             StartHandle);


/*
 * nseval - Namespace evaluation functions
 */
ACPI_STATUS
AcpiNsEvaluate (
    ACPI_EVALUATE_INFO      *Info);

void
AcpiNsExecModuleCodeList (
    void);


/*
 * nsarguments - Argument count/type checking for predefined/reserved names
 */
void
AcpiNsCheckArgumentCount (
    char                        *Pathname,
    ACPI_NAMESPACE_NODE         *Node,
    UINT32                      UserParamCount,
    const ACPI_PREDEFINED_INFO  *Info);

void
AcpiNsCheckAcpiCompliance (
    char                        *Pathname,
    ACPI_NAMESPACE_NODE         *Node,
    const ACPI_PREDEFINED_INFO  *Predefined);

void
AcpiNsCheckArgumentTypes (
    ACPI_EVALUATE_INFO          *Info);


/*
 * nspredef - Return value checking for predefined/reserved names
 */
ACPI_STATUS
AcpiNsCheckReturnValue (
    ACPI_NAMESPACE_NODE         *Node,
    ACPI_EVALUATE_INFO          *Info,
    UINT32                      UserParamCount,
    ACPI_STATUS                 ReturnStatus,
    ACPI_OPERAND_OBJECT         **ReturnObject);

ACPI_STATUS
AcpiNsCheckObjectType (
    ACPI_EVALUATE_INFO          *Info,
    ACPI_OPERAND_OBJECT         **ReturnObjectPtr,
    UINT32                      ExpectedBtypes,
    UINT32                      PackageIndex);


/*
 * nsprepkg - Validation of predefined name packages
 */
ACPI_STATUS
AcpiNsCheckPackage (
    ACPI_EVALUATE_INFO          *Info,
    ACPI_OPERAND_OBJECT         **ReturnObjectPtr);


/*
 * nsnames - Name and Scope manipulation
 */
UINT32
AcpiNsOpensScope (
    ACPI_OBJECT_TYPE        Type);

char *
AcpiNsGetExternalPathname (
    ACPI_NAMESPACE_NODE     *Node);

UINT32
AcpiNsBuildNormalizedPath (
    ACPI_NAMESPACE_NODE     *Node,
    char                    *FullPath,
    UINT32                  PathSize,
    BOOLEAN                 NoTrailing);

char *
AcpiNsGetNormalizedPathname (
    ACPI_NAMESPACE_NODE     *Node,
    BOOLEAN                 NoTrailing);

char *
AcpiNsBuildPrefixedPathname (
    ACPI_GENERIC_STATE      *PrefixScope,
    const char              *InternalPath);

char *
AcpiNsNameOfCurrentScope (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiNsHandleToName (
    ACPI_HANDLE             TargetHandle,
    ACPI_BUFFER             *Buffer);

ACPI_STATUS
AcpiNsHandleToPathname (
    ACPI_HANDLE             TargetHandle,
    ACPI_BUFFER             *Buffer,
    BOOLEAN                 NoTrailing);

BOOLEAN
AcpiNsPatternMatch (
    ACPI_NAMESPACE_NODE     *ObjNode,
    char                    *SearchFor);

ACPI_STATUS
AcpiNsGetNodeUnlocked (
    ACPI_NAMESPACE_NODE     *PrefixNode,
    const char              *ExternalPathname,
    UINT32                  Flags,
    ACPI_NAMESPACE_NODE     **OutNode);

ACPI_STATUS
AcpiNsGetNode (
    ACPI_NAMESPACE_NODE     *PrefixNode,
    const char              *ExternalPathname,
    UINT32                  Flags,
    ACPI_NAMESPACE_NODE     **OutNode);

ACPI_SIZE
AcpiNsGetPathnameLength (
    ACPI_NAMESPACE_NODE     *Node);


/*
 * nsobject - Object management for namespace nodes
 */
ACPI_STATUS
AcpiNsAttachObject (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_OPERAND_OBJECT     *Object,
    ACPI_OBJECT_TYPE        Type);

ACPI_OPERAND_OBJECT *
AcpiNsGetAttachedObject (
    ACPI_NAMESPACE_NODE     *Node);

ACPI_OPERAND_OBJECT *
AcpiNsGetSecondaryObject (
    ACPI_OPERAND_OBJECT     *ObjDesc);

ACPI_STATUS
AcpiNsAttachData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_OBJECT_HANDLER     Handler,
    void                    *Data);

ACPI_STATUS
AcpiNsDetachData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_OBJECT_HANDLER     Handler);

ACPI_STATUS
AcpiNsGetAttachedData (
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_OBJECT_HANDLER     Handler,
    void                    **Data);


/*
 * nsrepair - General return object repair for all
 * predefined methods/objects
 */
ACPI_STATUS
AcpiNsSimpleRepair (
    ACPI_EVALUATE_INFO      *Info,
    UINT32                  ExpectedBtypes,
    UINT32                  PackageIndex,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr);

ACPI_STATUS
AcpiNsWrapWithPackage (
    ACPI_EVALUATE_INFO      *Info,
    ACPI_OPERAND_OBJECT     *OriginalObject,
    ACPI_OPERAND_OBJECT     **ObjDescPtr);

ACPI_STATUS
AcpiNsRepairNullElement (
    ACPI_EVALUATE_INFO      *Info,
    UINT32                  ExpectedBtypes,
    UINT32                  PackageIndex,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr);

void
AcpiNsRemoveNullElements (
    ACPI_EVALUATE_INFO      *Info,
    UINT8                   PackageType,
    ACPI_OPERAND_OBJECT     *ObjDesc);


/*
 * nsrepair2 - Return object repair for specific
 * predefined methods/objects
 */
ACPI_STATUS
AcpiNsComplexRepairs (
    ACPI_EVALUATE_INFO      *Info,
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_STATUS             ValidateStatus,
    ACPI_OPERAND_OBJECT     **ReturnObjectPtr);


/*
 * nssearch - Namespace searching and entry
 */
ACPI_STATUS
AcpiNsSearchAndEnter (
    UINT32                  EntryName,
    ACPI_WALK_STATE         *WalkState,
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_INTERPRETER_MODE   InterpreterMode,
    ACPI_OBJECT_TYPE        Type,
    UINT32                  Flags,
    ACPI_NAMESPACE_NODE     **RetNode);

ACPI_STATUS
AcpiNsSearchOneScope (
    UINT32                  EntryName,
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_OBJECT_TYPE        Type,
    ACPI_NAMESPACE_NODE     **RetNode);

void
AcpiNsInstallNode (
    ACPI_WALK_STATE         *WalkState,
    ACPI_NAMESPACE_NODE     *ParentNode,
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_OBJECT_TYPE        Type);


/*
 * nsutils - Utility functions
 */
ACPI_OBJECT_TYPE
AcpiNsGetType (
    ACPI_NAMESPACE_NODE     *Node);

UINT32
AcpiNsLocal (
    ACPI_OBJECT_TYPE        Type);

void
AcpiNsPrintNodePathname (
    ACPI_NAMESPACE_NODE     *Node,
    const char              *Msg);

ACPI_STATUS
AcpiNsBuildInternalName (
    ACPI_NAMESTRING_INFO    *Info);

void
AcpiNsGetInternalNameLength (
    ACPI_NAMESTRING_INFO    *Info);

ACPI_STATUS
AcpiNsInternalizeName (
    const char              *DottedName,
    char                    **ConvertedName);

ACPI_STATUS
AcpiNsExternalizeName (
    UINT32                  InternalNameLength,
    const char              *InternalName,
    UINT32                  *ConvertedNameLength,
    char                    **ConvertedName);

ACPI_NAMESPACE_NODE *
AcpiNsValidateHandle (
    ACPI_HANDLE             Handle);

void
AcpiNsTerminate (
    void);

#endif /* __ACNAMESP_H__ */
