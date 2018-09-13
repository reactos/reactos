/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    rtflush.c

Abstract:

    NT level registry test program, basic non-error paths.

    Flush a key.

    rtflush <KeyPath>

    Will flush the key named by <KeyPath>

    Example:

        rtflush \REGISTRY\MACHINE\TEST\bigkey

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

    printf("rtflush: starting\n");

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
                MAXIMUM_ALLOWED,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(status)) {
        printf("rtflush: t0: %08lx\n", status);
        exit(1);
    }

    status = NtFlushKey(BaseHandle);
    if (!NT_SUCCESS(status)) {
        printf("rtflush: t0: %08lx\n", status);
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
        FALSE
        );

    return;
}
