/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rtrest2.c

Abstract:

    NT level registry api test program, basic error path

    Creates a key "Key1" and a subkey, "Key2"

    Calls NtSaveKey on Key1, then NtRestoreKey while a handle to Key2 is
    still open.

    rtrest2  <KeyPath> <FileName>

    Example:

        rtrest2 \registry\machine\system\test tempfile

Author:

    John Vert (jvert) 13-Jun-1992

Revision History:

--*/

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
                MAXIMUM_ALLOWED,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(status)) {
        printf("rtsave: key open failed status = %08lx\n", status);
        exit(1);
    }

    status = NtRestoreKey(KeyHandle, FileHandle);

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

    if ( (argc != 3) )
    {
        printf("Usage: %s <KeyName> <FileName>\n",
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
        &FileName,
        &temp,
        TRUE
        );

    return;
}
