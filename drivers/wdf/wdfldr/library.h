/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - library functions
 * COPYRIGHT:   Copyright 2019 mrmks04 (mrmks04@yandex.ru)
 */


#pragma once

#include "common.h"


BOOLEAN
NTAPI
LibraryAcquireClientLock(
    _In_ PLIBRARY_MODULE LibModule);


VOID
NTAPI
LibraryReleaseClientLock(
    _In_ PLIBRARY_MODULE LibModule);


NTSTATUS
NTAPI
LibraryOpen(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PUNICODE_STRING ObjectName);


VOID
NTAPI
LibraryClose(
    _In_ PLIBRARY_MODULE LibModule);


PLIBRARY_MODULE
NTAPI
LibraryCreate(
    _In_ PWDF_LIBRARY_INFO LibInfo,
    _In_ PUNICODE_STRING DriverServiceName);


PWDF_LIBRARY_INFO
NTAPI
LibraryCopyInfo(
    _In_ PLIBRARY_MODULE LibModule,
    _Inout_ PWDF_LIBRARY_INFO LibInfo);


VOID
NTAPI
LibraryCleanupAndFree(
    _In_ PLIBRARY_MODULE LibModule);


NTSTATUS
NTAPI
FxLdrQueryData(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_ ULONG Tag,
    _In_ PKEY_VALUE_PARTIAL_INFORMATION* KeyValPartialInfo);


NTSTATUS
NTAPI
FxLdrQueryUlong(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_ PULONG Value);


NTSTATUS
NTAPI
AuxKlibInitialize();


PLIST_ENTRY
NTAPI
LibraryAddToLibraryListLocked(
    _In_ PLIBRARY_MODULE LibModule);


VOID
NTAPI
LibraryRemoveFromLibraryList(
    _In_ PLIBRARY_MODULE LibModule);


PLIST_ENTRY
NTAPI
LibraryAddToClassListLocked(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PCLASS_MODULE ClassModule);


VOID
NTAPI
LibraryReleaseClientReference(
    _In_ PLIBRARY_MODULE LibModule);


NTSTATUS
NTAPI
LibraryUnlinkClient(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PWDF_BIND_INFO BindInfo);

VOID
NTAPI
ClientCleanupAndFree(
    _In_  PCLIENT_MODULE ClientModule);

VOID
NTAPI
LibraryUnload(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ BOOLEAN RemoveFromList);

NTSTATUS
NTAPI
LibraryLinkInClient(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PUNICODE_STRING DriverServiceName,
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PVOID Context,
    _Out_ PCLIENT_MODULE* ClientModule);
