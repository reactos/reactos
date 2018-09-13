/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rtrestor.c

Abstract:

    NT level registry api test program, basic non-error paths.

    Perform an NtRestoreKey call to load part of the registry from a file.

    rtrestor  <KeyPath> <FileName>

    Example:

        rtrestor \registry\machine\user userfile.rd

Author:

    Bryan Willman (bryanwi)  24-Jan-92

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORK_SIZE   1024

void __cdecl main(int, char *);
void processargs();

UNICODE_STRING  KeyPath;
WCHAR           KeyPathBuffer[WORK_SIZE];

UNICODE_STRING  FileName;
WCHAR           FileNameBuffer[WORK_SIZE];

BOOLEAN HiveVolatile;

void
__cdecl main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK  IoStatus;
    HANDLE  FileHandle;
    HANDLE  KeyHandle;
    BOOLEAN WasEnabled;

    //
    // Process args
    //

    KeyPath.MaximumLength = WORK_SIZE;
    KeyPath.Length = 0L;
    KeyPath.Buffer = &(KeyPathBuffer[0]);

    FileName.MaximumLength = WORK_SIZE;
    FileName.Length = 0L;
    FileName.Buffer = &(FileNameBuffer[0]);

    processargs(argc, argv);


    //
    // Set up and open FileName
    //

    printf("rtrestor: starting\n");

    InitializeObjectAttributes(
        &ObjectAttributes,
        &FileName,
        0,
        (HANDLE)NULL,
        NULL
        );
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    status = NtCreateFile(
                &FileHandle,
                GENERIC_READ | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                0,                                      // AllocationSize
                FILE_ATTRIBUTE_NORMAL,
                0,                                      // ShareAccess
                FILE_OPEN_IF,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,                                   // EaBuffer
                0                                       // EaLength
                );

    if (!NT_SUCCESS(status)) {
        printf("rtsave: file open failed status = %08lx\n", status);
        exit(1);
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyPath,
        0,
        (HANDLE)NULL,
        NULL
        );
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    status = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(status)) {
        printf("rtsave: key open failed status = %08lx\n", status);
        exit(1);
    }

    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    if (HiveVolatile) {
        status = NtRestoreKey(KeyHandle, FileHandle, REG_WHOLE_HIVE_VOLATILE);
    } else {
        status = NtRestoreKey(KeyHandle, FileHandle, 0);
    }

    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);

    if (!NT_SUCCESS(status)) {
        printf("rtrestor: NtRestorKey failed status = %08lx\n", status);
        exit(1);
    }

    printf("rtsave: success\n");
    exit(0);
}

void
processargs(
    int argc,
    char *argv[]
    )
{
    ANSI_STRING temp;
    UNICODE_STRING DosFileName;

    if ( (argc < 3) || (argc > 4) )
    {
        printf("Usage: %s <KeyName> <FileName> [VOLATILE]\n",
                argv[0]);
        exit(1);
    }

    RtlInitAnsiString(
        &temp,
        argv[1]
        );

    RtlAnsiStringToUnicodeString(
        &KeyPath,
        &temp,
        TRUE
        );

    RtlInitAnsiString(
        &temp,
        argv[2]
        );

    RtlAnsiStringToUnicodeString(
        &DosFileName,
        &temp,
        TRUE
        );

    RtlDosPathNameToNtPathName_U( DosFileName.Buffer,
                                  &FileName,
                                  NULL,
                                  NULL);

    if ((argc==4) && (_stricmp(argv[3],"volatile")==0)) {
        HiveVolatile = TRUE;
    } else {
        HiveVolatile = FALSE;
    }

    return;
}
