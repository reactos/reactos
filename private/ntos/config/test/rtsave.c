/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rtsave.c

Abstract:

    NT level registry api test program, basic non-error paths.

    Perform an NtSaveKey call to dump part of the registry to a file.

    rtsave  <KeyPath> <FileName>

    Example:

        rtsave \registry\machine\user userfile.rd

Author:

    Bryan Willman (bryanwi)  22-Jan-92

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

void __cdecl main(int, char *[]);
void processargs();

UNICODE_STRING  KeyPath;
WCHAR           KeyPathBuffer[WORK_SIZE];

UNICODE_STRING  FileName;
WCHAR           FileNameBuffer[WORK_SIZE];

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

    printf("rtsave: starting\n");
    printf("rtsave: saving hive rooted at\n\t'%ws'\nto file\n\t'%ws'\n",
            KeyPath.Buffer, FileName.Buffer);

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
                GENERIC_WRITE | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                NULL,                                   // AllocationSize
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,                        // ShareAccess
                FILE_CREATE,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,                                   // EaBuffer
                0                                       // EaLength
                );

    if (!NT_SUCCESS(status)) {
        if (status == STATUS_OBJECT_NAME_COLLISION) {
            printf("rtsave: file '%ws' already exists!\n",
                    FileName.Buffer);
            exit(1);
        }
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
                MAXIMUM_ALLOWED,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(status)) {
        printf("rtsave: key open failed status = %08lx\n", status);
        exit(1);
    }

    RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &WasEnabled);

    status = NtSaveKey(KeyHandle, FileHandle);

    RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);

    if (!NT_SUCCESS(status)) {
        printf("rtsave: NtSaveKey failed status = %08lx\n", status);
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

    if ( (argc != 3) )
    {
        printf("Usage: %s <KeyName> <FileName>\nWhere <FileName> does NOT already exist\n",
                argv[0]);
        printf("Example: %s \\registry\\machine\\security d:\\backups\\security\n",
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
                                  NULL );

    return;
}
