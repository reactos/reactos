/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    rtsymlnk.c

Abstract:

    NT level registry symbolic link test program

    Turns a key into a symbolic link.

    rtsymlnk <KeyPath> <SymbolicLink>

    Example:

        rtsymlnk \Registry\User\The_User\Foo \Registry\User\The_User\Bar

Author:

    John Vert (jvert) 29-Apr-92

Revision History:

--*/

#include "cmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
__cdecl main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING  KeyName;
    UNICODE_STRING  LinkName;
    UNICODE_STRING  NullName;
    ANSI_STRING AnsiKeyName;
    ANSI_STRING AnsiLinkName;
    HANDLE KeyHandle;

    //
    // Process args
    //

    if (argc != 3) {
        printf("Usage: %s <KeyPath> <SymLink>\n",argv[0]);
        exit(1);
    }

    RtlInitAnsiString(&AnsiKeyName, argv[1]);
    Status = RtlAnsiStringToUnicodeString(&KeyName, &AnsiKeyName, TRUE);
    if (!NT_SUCCESS(Status)) {
        printf("RtlAnsiStringToUnicodeString failed %lx\n",Status);
        exit(1);
    }

    RtlInitAnsiString(&AnsiLinkName, argv[2]);
    Status = RtlAnsiStringToUnicodeString(&LinkName, &AnsiLinkName, TRUE);
    if (!NT_SUCCESS(Status)) {
        printf("RtlAnsiStringToUnicodeString failed %lx\n",Status);
        exit(1);
    }

    printf("rtsetsec: starting\n");

    //
    // Open node that we want to make a symbolic link.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        (HANDLE)NULL,
        NULL
        );

    Status = NtCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if (!NT_SUCCESS(Status)) {
        printf("rtsymlnk: NtCreateKey failed: %08lx\n", Status);
        exit(1);
    }

    NullName.Length = NullName.MaximumLength = 0;
    NullName.Buffer = NULL;

    Status = NtSetValueKey(KeyHandle,
                           &NullName,
                           0,
                           REG_LINK,
                           LinkName.Buffer,
                           LinkName.Length);
    if (!NT_SUCCESS(Status)) {
        printf("rtsymlnk: NtSetValueKey failed: %08lx\n",Status);
        exit(1);
    }

    Status = NtClose(KeyHandle);
    if (!NT_SUCCESS(Status)) {
        printf("rtsymlnk: NtClose failed: %08lx\n", Status);
        exit(1);
    }

    printf("rtsymlnk: successful\n");
}
