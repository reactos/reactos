/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rtunload.c

Abstract:

    NT level registry api test program, basic non-error paths.

    Perform an NtUnloadKey call to unlink a hive file from the registry.

    rtunload  <KeyPath>

    Example:

        rtunload \registry\user\JVert

Author:

    John Vert (jvert) 17-Apr-92

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORK_SIZE   1024

void __cdecl main(int, char *);
void processargs();

UNICODE_STRING  KeyPath;
WCHAR           KeyPathBuffer[WORK_SIZE];

void
__cdecl main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES KeyAttributes;
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

    processargs(argc, argv);


    printf("rtunload: starting\n");

    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    //
    // Set up KeyPath
    //

    InitializeObjectAttributes(
        &KeyAttributes,
        &KeyPath,
        OBJ_CASE_INSENSITIVE,
        (HANDLE)NULL,
        NULL
        );

    status = NtUnloadKey(&KeyAttributes);

    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);

    if (!NT_SUCCESS(status)) {
        printf("rtunload: key unload failed status = %08lx\n", status);
        exit(1);
    } else {
        printf("rtunload: success!\n");
    }

    exit(0);
}

void
processargs(
    int argc,
    char *argv[]
    )
{
    ANSI_STRING temp;

    if ( (argc != 2) )
    {
        printf("Usage: %s <KeyName>\n",
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

    return;
}
