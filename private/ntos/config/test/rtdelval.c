/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rtdelval.c

Abstract:

    NT level registry api test program, basic non-error paths.

    Delete a key's value entry (field)

    rtdelval <KeyPath> <value entry name>

    Example:

        rtdelval \REGISTRY\MACHINE\TEST\bigkey first_value_field

Author:

    Bryan Willman (bryanwi)  10-Jan-92

Revision History:

--*/

#include "cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORK_SIZE   1024

void __cdecl main(int, char *);
void processargs();

void
Delete(
    HANDLE  Handle
    );

UNICODE_STRING  WorkName;
WCHAR           workbuffer[WORK_SIZE];

UNICODE_STRING  ValueName;
WCHAR           valuebuffer[WORK_SIZE];

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


    ValueName.MaximumLength = WORK_SIZE;
    ValueName.Length = 0L;
    ValueName.Buffer = &(valuebuffer[0]);

    processargs(argc, argv);


    //
    // Set up and open KeyPath
    //

    printf("rtdelval: starting\n");

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
                KEY_SET_VALUE,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(status)) {
        printf("rtdelval: t0: %08lx\n", status);
        exit(1);
    }

    status = NtDeleteValueKey(BaseHandle, &ValueName);
    if (!NT_SUCCESS(status)) {
        printf("rtdelval: t1: %08lx\n", status);
        exit(1);
    }

    NtClose(BaseHandle);
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
        printf("Usage: %s <KeyPath> <value entry name>\n",
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

    RtlInitAnsiString(
        &temp,
        argv[2]
        );

    RtlAnsiStringToUnicodeString(
        &ValueName,
        &temp,
        TRUE
        );

    return;
}
