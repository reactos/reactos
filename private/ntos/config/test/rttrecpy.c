/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rttrecpy.c

Abstract:

    NT level registry api test program

    tree copy for the registry.

    rtdeltre <SourceKeyPath> <DestKeyPath>

    Will tree-copy the given registry subtree.

    Example:

        rttrecpy \REGISTRY\MACHINE\TEST\bigkey \registry\machine\test\bigcopy

Author:

    John Vert (jvert) 22-Oct-1992

Revision History:

--*/

#include "cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORK_SIZE   1024

void __cdecl main(int argc, char *);
void processargs();

void
Copy(
    HANDLE  Source,
    HANDLE  Dest
    );

UNICODE_STRING  SourceName;
UNICODE_STRING  DestName;
BOOLEAN CopySecurity;

void
__cdecl main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE  SourceHandle;
    HANDLE  DestHandle;

    //
    // Process args
    //
    processargs(argc, argv);


    //
    // Set up and open KeyPath
    //

    printf("rttrecpy: starting\n");

    InitializeObjectAttributes(
        &ObjectAttributes,
        &SourceName,
        OBJ_CASE_INSENSITIVE,
        (HANDLE)NULL,
        NULL
        );

    Status = NtOpenKey(
                &SourceHandle,
                KEY_READ,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(Status)) {
        printf("rttrecpy: NtOpenKey %wS failed %08lx\n", &SourceName, Status);
        exit(1);
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &DestName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreateKey(&DestHandle,
                         KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if (!NT_SUCCESS(Status)) {
        printf("rttrecpy: NtCreateKey %wS failed %08lx\n",DestName,Status);
        exit(1);
    }

    Copy(SourceHandle, DestHandle);
}


void
Copy(
    HANDLE  Source,
    HANDLE  Dest
    )
{
    NTSTATUS    Status;
    PKEY_BASIC_INFORMATION KeyInformation;
    PKEY_VALUE_FULL_INFORMATION KeyValue;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG   NamePos;
    ULONG   index;
    STRING  enumname;
    HANDLE  SourceChild;
    HANDLE  DestChild;
    ULONG   ResultLength;
    static  char buffer[WORK_SIZE];
    static  char SecurityBuffer[WORK_SIZE];
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    UNICODE_STRING ValueName;
    UNICODE_STRING KeyName;


    //
    // Enumerate source node's values and copy them to target node.
    //
    KeyValue = (PKEY_VALUE_FULL_INFORMATION)buffer;
    for (index = 0; TRUE; index++) {
        Status = NtEnumerateValueKey(Source,
                                     index,
                                     KeyValueFullInformation,
                                     buffer,
                                     WORK_SIZE,
                                     &ResultLength);

        if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_NO_MORE_ENTRIES) {

                //
                // done with the values
                //
                break;
            } else {
                printf("rttrecpy: NtEnumerateValueKey failed %08lx\n",Status);
                break;
            }
        }

        ValueName.Buffer = KeyValue->Name;
        ValueName.Length = KeyValue->NameLength;

        Status = NtSetValueKey(Dest,
                               &ValueName,
                               KeyValue->TitleIndex,
                               KeyValue->Type,
                               buffer+KeyValue->DataOffset,
                               KeyValue->DataLength);
        if (!NT_SUCCESS(Status)) {
            printf("rttrecpy: NtSetValueKey failed to set value %wS\n",&ValueName);
        }

    }

    //
    // Enumerate node's children and apply ourselves to each one
    //

    KeyInformation = (PKEY_BASIC_INFORMATION)buffer;
    if (CopySecurity) {
        SecurityDescriptor = SecurityBuffer;
    } else {
        SecurityDescriptor = NULL;
    }
    for (index = 0; TRUE; index++) {

        Status = NtEnumerateKey(Source,
                                index,
                                KeyBasicInformation,
                                KeyInformation,
                                WORK_SIZE,
                                &ResultLength);

        if (Status == STATUS_NO_MORE_ENTRIES) {

            break;

        } else if (!NT_SUCCESS(Status)) {
            printf("rttrecpy: NtEnumerateKey failed Status = %08lx\n", Status);
            exit(1);
        }

        KeyName.Buffer = KeyInformation->Name;
        KeyName.Length = KeyInformation->NameLength;

        InitializeObjectAttributes(
            &ObjectAttributes,
            &KeyName,
            OBJ_CASE_INSENSITIVE,
            Source,
            NULL
            );

        Status = NtOpenKey(
                    &SourceChild,
                    KEY_READ,
                    &ObjectAttributes
                    );
        if (!NT_SUCCESS(Status)) {
            printf("rttrecpy: NtOpenKey %wS failed: %08lx\n", Status);
            exit(1);
        }

        if (CopySecurity) {
            Status = NtQuerySecurityObject(SourceChild,
                                           DACL_SECURITY_INFORMATION,
                                           SecurityDescriptor,
                                           WORK_SIZE,
                                           &ResultLength);
            if (!NT_SUCCESS(Status)) {
                printf("rttrecpy: NtQuerySecurityObject failed %08lx\n",Status);
            }
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   Dest,
                                   SecurityDescriptor);
        Status = NtCreateKey(&DestChild,
                             KEY_READ | KEY_WRITE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             0,
                             NULL);
        if (!NT_SUCCESS(Status)) {
            printf("rttrecpy: NtCreateKey %wS failed %08lx\n",Status);
            exit(1);
        }

        Copy(SourceChild, DestChild);
        NtClose(SourceChild);
        NtClose(DestChild);

    }

    return;
}


void
processargs(
    int argc,
    char *argv[]
    )
{
    ANSI_STRING temp;
    char **p;

    if ( (argc > 4) || (argc < 3) )
    {
        printf("Usage: %s [-s] <SourceKey> <DestKey>\n",
                argv[0]);
        exit(1);
    }

    p=argv+1;
    if (_stricmp(*p,"-s")==0) {
        CopySecurity = TRUE;
        ++p;
    } else {
        CopySecurity = FALSE;
    }

    RtlInitAnsiString(
        &temp,
        *p
        );

    ++p;

    RtlAnsiStringToUnicodeString(
        &SourceName,
        &temp,
        TRUE
        );

    RtlInitAnsiString(&temp,
                      *p);

    RtlAnsiStringToUnicodeString(&DestName,
                                 &temp,
                                 TRUE);

    return;
}
