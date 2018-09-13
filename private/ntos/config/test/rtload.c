/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rtload.c

Abstract:

    NT level registry api test program, basic non-error paths.

    Perform an NtLoadKey call to link a hive file into the registry.

    If KeyPath is not present, it loads the hive file at
        \Registry\User\FileName

    rtload  [ <KeyPath> ] <FileName>

    Example:

        rtload \registry\user\JVert JVUser

Author:

    John Vert (jvert) 15-Apr-92

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

UNICODE_STRING  FileName;
WCHAR           FileNameBuffer[WORK_SIZE];

OBJECT_ATTRIBUTES FileAttributes;
OBJECT_ATTRIBUTES KeyAttributes;
RTL_RELATIVE_NAME RelativeName;

void
__cdecl main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS status;
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
    // Set up FileName
    //

    printf("rtload: starting\n");


    status = NtLoadKey(&KeyAttributes, &FileAttributes);
    if (!NT_SUCCESS(status)) {
        printf("rtload: key load failed status = %08lx\n", status);
        exit(1);
    } else {
        printf("rtload: success!\n");
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
    UNICODE_STRING DosFileName;
    HANDLE UserHandle;
    PWSTR FilePart;
    NTSTATUS Status;

    if ( (argc != 2) && (argc != 3))
    {
        printf("Usage: %s [ <KeyName> ] <FileName>\n",
                argv[0]);
        exit(1);
    }
    if (argc == 3) {

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

        InitializeObjectAttributes(
            &FileAttributes,
            &FileName,
            OBJ_CASE_INSENSITIVE,
            (HANDLE)NULL,
            NULL
            );

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
    } else if (argc==2) {
        RtlInitAnsiString(&temp, argv[1]);
        RtlAnsiStringToUnicodeString(&DosFileName, &temp, TRUE);
        RtlDosPathNameToNtPathName_U( DosFileName.Buffer,
                                      &FileName,
                                      &FilePart,
                                      &RelativeName );

        InitializeObjectAttributes( &FileAttributes,
                                    &RelativeName.RelativeName,
                                    OBJ_CASE_INSENSITIVE,
                                    RelativeName.ContainingDirectory,
                                    NULL );

        RtlInitUnicodeString(&KeyPath, L"\\Registry\\User");
        InitializeObjectAttributes( &KeyAttributes,
                                    &KeyPath,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );
        Status = NtOpenKey( &UserHandle,
                            KEY_READ,
                            &KeyAttributes);
        if (!NT_SUCCESS(Status)) {
            printf("Couldn't open \\Registry\\User, status %08lx\n",Status);
            exit(1);
        }

        RtlInitUnicodeString(&KeyPath, FilePart);
        InitializeObjectAttributes( &KeyAttributes,
                                    &KeyPath,
                                    OBJ_CASE_INSENSITIVE,
                                    UserHandle,
                                    NULL );

    }


    return;
}
