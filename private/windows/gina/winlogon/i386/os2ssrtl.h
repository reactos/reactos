/****************************** Module Header ******************************\
* Module Name: os2ssrtl.h
*
* Copyright (c) 1993, Microsoft Corporation
*
* Import file for the OS/2 Subsystem migration code support module
*
* History:
* 03-30-93 OferP      Created.
\***************************************************************************/

#ifndef __OS2SSRTL_H
#define __OS2SSRTL_H

#define FWD             +1L             // these are used with
#define BWD             -1L             // Or2SkipWWS

#define NULL_DELIM      0               // these are used with
#define CRLF_DELIM      1               // Or2IterateEnvironment

typedef VOID (*PFN_ENVIRONMENT_PROCESSOR)(
    IN ULONG DispatchTableIndex,
    IN PVOID UserParameter,
    IN PWSTR Name,
    IN ULONG NameLen,
    IN PWSTR Value,
    IN ULONG ValueLen
    );

typedef struct _ENVIRONMENT_DISPATCH_TABLE_ENTRY {
    PWSTR VarName;
    PWSTR Delimiters;
    PFN_ENVIRONMENT_PROCESSOR DispatchFunction;
    PVOID UserParameter;
} ENVIRONMENT_DISPATCH_TABLE_ENTRY, *PENVIRONMENT_DISPATCH_TABLE_ENTRY;

typedef PENVIRONMENT_DISPATCH_TABLE_ENTRY ENVIRONMENT_DISPATCH_TABLE;

typedef struct _ENVIRONMENT_SEARCH_RECORD {
    ULONG DispatchTableIndex;
    PWSTR Name;
    ULONG NameLen;
    PWSTR Value;
    ULONG ValueLen;
} ENVIRONMENT_SEARCH_RECORD, *PENVIRONMENT_SEARCH_RECORD;

VOID
Or2SkipWWS(
    IN OUT PWSTR *Str,
    IN LONG Direction
    );

VOID
Or2UnicodeStrupr(
    IN OUT PWSTR Str
    );

BOOLEAN
Or2UnicodeEqualCI(
    IN PWSTR Str1,
    IN PWSTR Str2,
    IN ULONG Count
    );

BOOLEAN
Or2AppendPathToPath(
    IN PVOID HeapHandle,
    IN PWSTR SrcPath,
    IN OUT PUNICODE_STRING DestPath,
    IN BOOLEAN ExpandIt
    );

BOOLEAN
Or2ReplacePathByPath(
    IN PVOID HeapHandle,
    IN PWSTR SrcPath,
    IN OUT PUNICODE_STRING DestPath
    );

VOID
Or2CheckSemicolon(
    IN OUT PUNICODE_STRING Str
    );

BOOLEAN
Or2GetEnvPath(
    OUT PUNICODE_STRING Data,
    IN PVOID HeapHandle,
    IN USHORT MaxSiz,
    IN HANDLE EnvKey,
    IN PWSTR ValueName,
    IN BOOLEAN ExpandIt
    );

VOID
Or2IterateEnvironment(
    IN PWSTR Environment,
    IN ENVIRONMENT_DISPATCH_TABLE DispatchTable,
    IN ULONG NumberOfDispatchItems,
    IN ULONG DelimOption
    );

VOID
Or2FillInSearchRecordDispatchFunction(
    IN ULONG DispatchTableIndex,
    IN PVOID UserParameter,
    IN PWSTR Name,
    IN ULONG NameLen,
    IN PWSTR Value,
    IN ULONG ValueLen
    );

NTSTATUS
Or2GetFileStamps(
    IN HANDLE hFile,
    OUT PLARGE_INTEGER pTimeStamp,
    OUT PLARGE_INTEGER pSizeStamp
    );

#endif

