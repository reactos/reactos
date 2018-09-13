/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    regutil.h

Abstract:

    This is the include file for the registry utility functions.

Author:

    Steve Wood (stevewo) 10-Mar-1992

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
// #include <io.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys\types.h>
#include <sys\stat.h>


#define VALUE_BUFFER_SIZE (4096 * 100)

void
RegInitialize( void );

typedef struct _REG_UNICODE_FILE {
    LARGE_INTEGER LastWriteTime;
    PWSTR FileContents;
    PWSTR EndOfFile;
    PWSTR BeginLine;
    PWSTR EndOfLine;
    PWSTR NextLine;
} REG_UNICODE_FILE, *PREG_UNICODE_FILE;

NTSTATUS
RegReadBinaryFile(
    IN PUNICODE_STRING FileName,
    OUT PVOID *ValueBuffer,
    OUT PULONG ValueLength
    );

NTSTATUS
RegLoadAsciiFileAsUnicode(
    IN PUNICODE_STRING FileName,
    OUT PREG_UNICODE_FILE UnicodeFile
    );

BOOLEAN DebugOutput;
BOOLEAN SummaryOutput;

BOOLEAN
RegGetNextLine(
    IN OUT PREG_UNICODE_FILE UnicodeFile,
    OUT PULONG IndentAmount,
    OUT PWSTR *FirstEqual
    );

BOOLEAN
RegGetKeyValue(
    IN PUNICODE_STRING KeyValue,
    IN OUT PREG_UNICODE_FILE UnicodeFile,
    OUT PULONG ValueType,
    OUT PVOID *ValueBuffer,
    OUT PULONG ValueLength
    );

BOOLEAN
RegGetMultiString(
    IN OUT PUNICODE_STRING ValueString,
    OUT PUNICODE_STRING MultiString
    );

void
RegDumpKeyValue(
    FILE *fh,
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation,
    ULONG IndentLevel
    );

//
// routines for creating security descriptors (regacl.c)
//

BOOLEAN
RegInitializeSecurity(
    VOID
    );

BOOLEAN
RegCreateSecurity(
    IN PUNICODE_STRING Description,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor
    );

VOID
RegDestroySecurity(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );


