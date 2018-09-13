/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    edithive.h

Abstract:

    include for user-mode hive editing library

Author:

    John Vert (jvert) 27-Mar-1992

Revision History:

--*/

#include "ntos.h"
#include "cmp.h"



#define TYPE_SIMPLE     0
#define TYPE_LOG        1
#define TYPE_ALT        2


HANDLE
EhOpenHive(
    IN PUNICODE_STRING FileName,
    OUT PHANDLE RootCell,
    OUT PUNICODE_STRING RootName,
    IN ULONG HiveType
    );

VOID
EhCloseHive(
    IN HANDLE Hive
    );

NTSTATUS
EhEnumerateValueKey(
    IN HANDLE HiveHandle,
    IN HANDLE CellHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    );

NTSTATUS
EhEnumerateKey(
    IN HANDLE HiveHandle,
    IN HANDLE CellHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    );

NTSTATUS
EhOpenChildByName(
    HANDLE HiveHandle,
    HANDLE KeyHandle,
    PUNICODE_STRING  Name,
    PHANDLE ChildCell
    );

NTSTATUS
EhCreateChild(
    IN HANDLE HiveHandle,
    IN HANDLE CellHandle,
    IN PUNICODE_STRING  Name,
    OUT PHANDLE ChildCell,
    OUT PULONG Disposition OPTIONAL
    );

NTSTATUS
EhQueryKey(
    IN HANDLE HiveHandle,
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    );

NTSTATUS
EhQueryValueKey(
    IN HANDLE HiveHandle,
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    );

NTSTATUS
EhSetValueKey(
    IN HANDLE HiveHandle,
    IN HANDLE CellHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    );

NTSTATUS
EhDeleteValueKey(
    IN HANDLE Hive,
    IN HANDLE Cell,
    IN PUNICODE_STRING ValueName         // RAW
    );

PSECURITY_DESCRIPTOR
EhGetKeySecurity(
    IN HANDLE HiveHandle,
    IN HANDLE KeyHandle
    );
