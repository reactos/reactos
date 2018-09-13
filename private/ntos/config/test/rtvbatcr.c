/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rtvbatcr.c

Abstract:

    NT level registry api test program, basic non-error paths.

    Do a batch create, VOLATILE.  (Same as rtbatcr, but creates keys volatile.)

    rtvbatcr    <KeyPath> <KeyName> <basename> <#children> <#values>

    Will attempt to create key <KeyName> as child of <KeyPath>  If
    <#children> and <#values> are 0, this is all it does.  If <KeyName>
    already exists, it will simply be used.

    Will create <#children> child cells, with names of the form
    <base>0  <base>1, etc.  Will create <#values> value entries,
    with similar names, for each created child key.  Data of
    values will be a constant string including their name.

    Example:

        rtvbatcr    \REGISTRY\MACHINE\TEST bigkey runa_ 100 100
        rtvbatcr    \REGISTRY\MACHINE\TEST\bigkey runa_1 runb_ 100 100

        Will create bigkey, give it 100 values calls runa_1 through
        runa_100, create 100 subkeys called runa_1 through runa_100
        for each of those children.

        It will then open bigkey\runa_1, and create 100 subkeys and
        100 values each for that.

Author:

    Bryan Willman (bryanwi)  10-Dec-91

Revision History:

--*/

#include "cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORK_SIZE   1024

void __cdecl main(int, char *);
void processargs();

ULONG           failure = 0;

UNICODE_STRING  KeyPath;
UNICODE_STRING  KeyName;
ULONG           NumberChildren;
ULONG           NumberValues;
UCHAR           BaseName[WORK_SIZE];
UCHAR           formatbuffer[WORK_SIZE];
STRING          format;

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
    HANDLE          WorkHandle;
    ULONG           Disposition;
    UNICODE_STRING  ClassName;
    ULONG           i;
    ULONG           j;
    PUCHAR  p;

    //
    // Process args
    //

    processargs(argc, argv);


    //
    // Set up and create/open KeyPath|KeyName
    //

    printf("rtvbatcr: starting\n");

    WorkName.MaximumLength = WORK_SIZE;
    WorkName.Length = 0L;
    WorkName.Buffer = &(workbuffer[0]);

    RtlCopyString((PSTRING)&WorkName, (PSTRING)&KeyPath);

    p = WorkName.Buffer;
    p += WorkName.Length;
    *p = '\\';
    p++;
    *p = '\0';
    WorkName.Length += 2;

    RtlAppendStringToString((PSTRING)&WorkName, (PSTRING)&KeyName);

    RtlInitUnicodeString(
        &ClassName,
        L"Test Class Name"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &WorkName,
        0,
        (HANDLE)NULL,
        NULL
        );
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

    status = NtCreateKey(
                &BaseHandle,
                MAXIMUM_ALLOWED,
                &ObjectAttributes,
                0,
                &ClassName,
                REG_OPTION_VOLATILE,
                &Disposition
                );
    if (!NT_SUCCESS(status)) {
        printf("rtvbatcr: t0: %08lx\n", status);
        failure++;
        goto punt;
    }


    //
    // Create NumberChildren subkeys
    //

    for (i = 0; i < NumberChildren; i++) {

        sprintf(formatbuffer, "%s%d", BaseName, i);
        RtlInitString(&format, formatbuffer);
        RtlAnsiStringToUnicodeString(&WorkName, &format, FALSE);


        InitializeObjectAttributes(
            &ObjectAttributes,
            &WorkName,
            0,
            BaseHandle,
            NULL
            );
        ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

        status = NtCreateKey(
                    &WorkHandle,
                    MAXIMUM_ALLOWED,
                    &ObjectAttributes,
                    0,
                    &ClassName,
                    REG_OPTION_VOLATILE,
                    &Disposition
                    );
        if (!NT_SUCCESS(status)) {
            printf("rtvbatcr: t1: status = %08lx i = %d\n", status, i);
            failure++;
        }

        //
        // Create NumberValues value entries for each (current) key
        //

        for (j = 0; j < NumberValues; j++) {

            sprintf(formatbuffer, "%s%d", BaseName, j);
            RtlInitString(&format, formatbuffer);
            RtlAnsiStringToUnicodeString(&WorkName, &format, FALSE);

            sprintf(
                formatbuffer, "This is a rtvbatcr value for %s%d", BaseName, j
                );

            status = NtSetValueKey(
                        WorkHandle,
                        &WorkName,
                        j,
                        j,
                        formatbuffer,
                        strlen(formatbuffer)+1
                        );
            if (!NT_SUCCESS(status)) {
                printf("rtvbatcr: t2: status = %08lx j = %d\n", status, j);
                failure++;
            }
        }
        NtClose(WorkHandle);
    }

punt:
    printf("rtvbatcr: %d failures\n", failure);
    exit(failure);
}


void
processargs(
    int argc,
    char *argv[]
    )
{
    ANSI_STRING temp;

    if ( (argc != 3) && (argc != 6) )
    {
        printf("Usage: %s <KeyPath> <KeyName> [<basename> <#children> <#values>]\n",
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
        &KeyName,
        &temp,
        TRUE
        );

    if (argc < 6) {

        NumberChildren = 0;
        NumberValues = 0;

    } else {

        strcpy(BaseName, argv[3]);
        NumberChildren = atoi(argv[4]);
        NumberValues = atoi(argv[5]);

    }
    return;
}
