/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rtdmp.c

Abstract:

    NT level registry api test program #3, basic non-error paths.

    Dump out a sub-tree of the registry.

    rtdmp <KeyPath>

    Will ennumerate and dump out the subkeys and values of KeyPath,
    and then apply itself recursively to each subkey it finds.

    It assumes data values are null terminated strings.

    Example:

        rtdmp \REGISTRY\MACHINE\TEST\bigkey


        \REGISTRY\MACHINE\TEST\bigkey::

            ValueTest1_01 type=0 ti=1
            "This is a test string"

            ValueTest1_01 type=0 ti=2
            "This is a test string"

        \REGISTRY\MACHCINE\TEST\bigkey\child_key_1::

            ValueTest1_01 type=0 ti=1
            "This is a test string"

            ValueTest1_01 type=0 ti=2
            "This is a test string"
Author:

    Bryan Willman (bryanwi)  10-Dec-91

Revision History:

--*/

#include "cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORK_SIZE 16384

void __cdecl main(int, char *);
void processargs();

void print(PUNICODE_STRING);

void
DumpValues(
    HANDLE  Handle
    );

void
Dump(
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

    printf("rtdmp: starting\n");

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
        printf("rtdmp: t0: %08lx\n", status);
        exit(1);
    }

    Dump(BaseHandle);
}


void
Dump(
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
    PUCHAR  p;

    KeyInformation = (PKEY_BASIC_INFORMATION)buffer;
    NamePos = WorkName.Length;

    //
    // Print name of node we are about to dump out
    //
    print(&WorkName);
    printf("::\n\n");

    //
    // Print out node's values
    //
    DumpValues(Handle);

    //
    // Enumerate node's children and apply ourselves to each one
    //

    for (index = 0; TRUE; index++) {

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
            return;

        } else if (!NT_SUCCESS(status)) {

            printf("rtdmp: dump1: status = %08lx\n", status);
            exit(1);

        }

        enumname.Buffer = &(KeyInformation->Name[0]);
        enumname.Length = KeyInformation->NameLength;
        enumname.MaximumLength = KeyInformation->NameLength;

        p = WorkName.Buffer;
        p += WorkName.Length;
        *p = '\\';
        p++;
        *p = '\0';
        WorkName.Length += 2;

        RtlAppendStringToString((PSTRING)&WorkName, (PSTRING)&enumname);

        InitializeObjectAttributes(
            &ObjectAttributes,
            &enumname,
            0,
            Handle,
            NULL
            );
        ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

        status = NtOpenKey(
                    &WorkHandle,
                    MAXIMUM_ALLOWED,
                    &ObjectAttributes
                    );
        if (!NT_SUCCESS(status)) {
            printf("rtdmp: dump2: %08lx\n", status);
            exit(1);
        }

        Dump(WorkHandle);
        NtClose(WorkHandle);
        WorkName.Length = NamePos;
    }
}


void
DumpValues(
    HANDLE  Handle
    )
{
    NTSTATUS    status;
    static  char        tempbuffer[WORK_SIZE];
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    ULONG   index;
    ULONG   ResultLength;
    PULONG  p;
    ULONG i;
    UNICODE_STRING valname;

    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)tempbuffer;

    for (index = 0; TRUE; index++) {

        RtlZeroMemory(KeyValueInformation, WORK_SIZE);
        status = NtEnumerateValueKey(
                    Handle,
                    index,
                    KeyValueFullInformation,
                    KeyValueInformation,
                    WORK_SIZE,
                    &ResultLength
                    );
        if (status == STATUS_NO_MORE_ENTRIES) {

            return;

        } else if (!NT_SUCCESS(status)) {

            printf("rtdmp: dumpvalues: status = %08lx\n", status);
            exit(1);

        }

        printf("\t");
        valname.Length = KeyValueInformation->NameLength;
        valname.MaximumLength = KeyValueInformation->NameLength;
        valname.Buffer = (PWSTR)&(KeyValueInformation->Name[0]);
        printf("'");
        print(&valname);
        printf("'\n");
        printf(
            "\ttitle index = %d\ttype = ",
            KeyValueInformation->TitleIndex
            );
        printf("REG_BINARY\n\tValue = (%lx)\n", KeyValueInformation->DataLength);
        p = (PULONG)KeyValueInformation + KeyValueInformation->DataOffset;
        i = 1;
        while (i <= KeyValueInformation->DataLength) {
            printf( "  %08lx", *p++ );
            if ((i % 8) == 0) {
                printf( "\n" );
            }
            i += sizeof( ULONG );
        }
        printf("\n\n");
    }
}


void
print(
    PUNICODE_STRING  String
    )
{
    static  ANSI_STRING temp;
    static  char        tempbuffer[WORK_SIZE];

    temp.MaximumLength = WORK_SIZE;
    temp.Length = 0L;
    temp.Buffer = tempbuffer;

    RtlUnicodeStringToAnsiString(&temp, String, FALSE);
    printf("%s", temp.Buffer);
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
        FALSE
        );

    return;
}
