/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rtdeltre.c

Abstract:

    NT level registry api test program #4, basic non-error paths.

    Sub-tree delete for the registry.

    rtdeltre <KeyPath>

    Will ennumerate and delete the subkeys and values of KeyPath,
    and each of their subkeys, and so on.

    Example:

        rtdeltre \REGISTRY\MACHINE\TEST\bigkey

Author:

    Bryan Willman (bryanwi)  10-Dec-91

Revision History:

--*/

#include "cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORK_SIZE   1024

void __cdecl main(int argc, char *);
void processargs();

void print(PUNICODE_STRING);

void
Delete(
    HANDLE  Handle
    );

UNICODE_STRING  WorkName;
WCHAR           workbuffer[WORK_SIZE];

void
__cdecl main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE          BaseHandle;

    //
    // Process args
    //

    WorkName.MaximumLength = WORK_SIZE;
    WorkName.Length = 0L;
    WorkName.Buffer = &(workbuffer[0]);

    processargs(argc, argv);


    //
    // Set up and open KeyPath
    //

    printf("regtest3: starting\n");

    InitializeObjectAttributes(
        &ObjectAttributes,
        &WorkName,
        0,
        (HANDLE)NULL,
        NULL
        );
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    status = NtOpenKey(
                &BaseHandle,
                DELETE | KEY_ENUMERATE_SUB_KEYS,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(status)) {
        printf("regtest3: t0: %08lx\n", status);
        exit(1);
    }

    Delete(BaseHandle);
}


void
Delete(
    HANDLE  Handle
    )
{
    NTSTATUS    status;
    PKEY_BASIC_INFORMATION KeyInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG   NamePos;
    ULONG   index;
    STRING  enumname;
    HANDLE  WorkHandle;
    ULONG   ResultLength;
    static  char buffer[WORK_SIZE];

    KeyInformation = (PKEY_BASIC_INFORMATION)buffer;
    NamePos = WorkName.Length;

    //
    // Enumerate node's children and apply ourselves to each one
    //

    index = 0;
    do {

        RtlZeroMemory(KeyInformation, WORK_SIZE);
        status = NtEnumerateKey(
                    Handle,
                    index,
                    KeyBasicInformation,
                    KeyInformation,
                    WORK_SIZE,
                    &ResultLength
                    );

        if (status == STATUS_NO_MORE_ENTRIES) {

            WorkName.Length = NamePos;
            break;

        } else if (!NT_SUCCESS(status)) {

            printf("regtest3: dump1: status = %08lx\n", status);
            exit(1);

        }

        enumname.Buffer = &(KeyInformation->Name[0]);
        enumname.Length = KeyInformation->NameLength;
        enumname.MaximumLength = KeyInformation->NameLength;

        RtlAppendStringToString((PSTRING)&WorkName, (PSTRING)&enumname);

        InitializeObjectAttributes(
            &ObjectAttributes,
            &enumname,
            OBJ_CASE_INSENSITIVE,
            Handle,
            NULL
            );

        status = NtOpenKey(
                    &WorkHandle,
                    DELETE | KEY_ENUMERATE_SUB_KEYS,
                    &ObjectAttributes
                    );
        if (!NT_SUCCESS(status)) {
            printf("regtest3: couldn't delete %wZ: %08lx\n", &enumname,status);
            index++;
        } else {
            Delete(WorkHandle);
            NtClose(WorkHandle);
        }

        WorkName.Length = NamePos;

    } while (TRUE);

    //
    // If we're here, then we have delt with all children, so deal with
    // the node we were applied to
    //

    NtDeleteKey(Handle);
    NtClose(Handle);        // Force it to actually go away
    return;
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
        printf("Usage: %s <KeyPath>\n",
                argv[0]);
        exit(1);
    }

    RtlInitAnsiString(
        &temp,
        argv[1]
        );

    RtlAnsiStringToUnicodeString(
        &WorkName,
        &temp,
        TRUE
        );

    return;
}
