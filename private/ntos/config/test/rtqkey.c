/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rtqkey.c

Abstract:

    NT level registry api test program, basic non-error paths.

    Do a query on a key.

    rtqkey <KeyPath> [infotypenumber] [bufferlength]

    Example:

        rtqkey \REGISTRY\MACHINE\TEST\bigkey 1 100

Author:

    Bryan Willman (bryanwi)  9-Apr-92

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

UCHAR       Buffer[1024*64];

ULONG       InfoType = KeyFullInformation;
ULONG       BufferSize = -1;

void
__cdecl main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE          BaseHandle;
    ULONG   Sizes[] = { sizeof(KEY_BASIC_INFORMATION),
                        sizeof(KEY_NODE_INFORMATION),
                        sizeof(KEY_FULL_INFORMATION) };
    ULONG       ResultLength;
    PKEY_BASIC_INFORMATION pbasic;
    PKEY_NODE_INFORMATION  pnode;
    PKEY_FULL_INFORMATION  pfull;

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

    printf("rtqkey: starting\n");

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
                KEY_QUERY_VALUE,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(status)) {
        printf("rtqkey: t0: %08lx\n", status);
        exit(1);
    }

    //
    // make test call
    //
    RtlFillMemory((PVOID)&(Buffer[0]), 1024*64, 0xaa);

    if (BufferSize == -1) {
        BufferSize = Sizes[InfoType];
    }

    status = NtQueryKey(
                BaseHandle,
                InfoType,
                (PVOID)&(Buffer[0]),
                BufferSize,
                &ResultLength
                );

    printf("status = %08lx  ResultLength = %08lx\n", status, ResultLength);
    switch (InfoType) {
    case KeyBasicInformation:
        pbasic = (PKEY_BASIC_INFORMATION)Buffer;
        printf("LastWriteTime: %08lx:%08lx\n", pbasic->LastWriteTime.HighPart,
                pbasic->LastWriteTime.LowPart);
        printf("TitleIndex: %08lx\n", pbasic->TitleIndex);
        printf("NameLength: %08lx\n", pbasic->NameLength);
        printf("Name: '%.*ws'\n", pbasic->NameLength/2, &(pbasic->Name));
        break;

    case KeyNodeInformation:
        pnode = (PKEY_NODE_INFORMATION)Buffer;
        printf("LastWriteTime: %08lx:%08lx\n", pnode->LastWriteTime.HighPart,
                pnode->LastWriteTime.LowPart);
        printf("TitleIndex: %08lx\n", pnode->TitleIndex);
        printf("ClassOffset: %08lx\n", pnode->ClassOffset);
        printf("ClassLength: %08lx\n", pnode->ClassLength);
        printf("NameLength: %08lx\n", pnode->NameLength);
        printf("Name: '%.*ws'\n", pnode->NameLength/2, &(pnode->Name));
        printf("Class: '%.*ws'\n", pnode->ClassLength/2,
                    (PWSTR)((PUCHAR)pnode + pnode->ClassOffset));
        break;

    case KeyFullInformation:
        pfull = (PKEY_FULL_INFORMATION)Buffer;
        printf("LastWriteTime: %08lx:%08lx\n", pfull->LastWriteTime.HighPart,
                pfull->LastWriteTime.LowPart);
        printf("TitleIndex: %08lx\n", pfull->TitleIndex);
        printf("ClassOffset: %08lx\n", pfull->ClassOffset);
        printf("ClassLength: %08lx\n", pfull->ClassLength);

        printf("SubKeys: %08lx       MaxNameLen: %08lx      MaxClassLen: %08lx\n",
                pfull->SubKeys, pfull->MaxNameLen, pfull->MaxClassLen);

        printf(" Values: %08lx  MaxValueNameLen: %08lx  MaxValueDataLen: %08lx\n",
                pfull->Values, pfull->MaxValueNameLen, pfull->MaxValueDataLen);

        printf("Class: '%.*ws'\n", pfull->ClassLength/2, pfull->Class);

        break;
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

    if ( (argc < 2) )
    {
        printf("Usage: %s <KeyPath> [infotype] [bufferlen]\n",
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

    if (argc > 2) {
        InfoType = atoi(argv[2]);
    }

    if (argc > 3) {
        BufferSize = atoi(argv[3]);
    }

    return;
}
