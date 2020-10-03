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
	PLIBRARY_MODULE LibModule
);


VOID
NTAPI
LibraryReleaseClientLock(
	PLIBRARY_MODULE LibModule
);


NTSTATUS
NTAPI
LibraryOpen(
	PLIBRARY_MODULE LibModule,
	PUNICODE_STRING ObjectName
);


VOID
NTAPI
LibraryClose(
	PLIBRARY_MODULE LibModule
);


PLIBRARY_MODULE
NTAPI
LibraryCreate(
	PWDF_LIBRARY_INFO LibInfo,
	PUNICODE_STRING DriverServiceName
);


PWDF_LIBRARY_INFO
NTAPI
LibraryCopyInfo(
	PLIBRARY_MODULE LibModule,
	PWDF_LIBRARY_INFO LibInfo
);


VOID
NTAPI
LibraryCleanupAndFree(
	PLIBRARY_MODULE LibModule
);


NTSTATUS
NTAPI
FxLdrQueryData(
	HANDLE KeyHandle,
	PUNICODE_STRING ValueName,
	ULONG Tag,
	PKEY_VALUE_PARTIAL_INFORMATION* KeyValPartialInfo
);


NTSTATUS
NTAPI
FxLdrQueryUlong(
	HANDLE KeyHandle,
	PUNICODE_STRING ValueName,
	PULONG Value
);


NTSTATUS
NTAPI
AuxKlibInitialize();


PLIST_ENTRY
NTAPI
LibraryAddToLibraryListLocked(
	PLIBRARY_MODULE LibModule
);


VOID
NTAPI
LibraryRemoveFromLibraryList(
	PLIBRARY_MODULE LibModule
);


PLIST_ENTRY
NTAPI
LibraryAddToClassListLocked(
	PLIBRARY_MODULE LibModule,
	PCLASS_MODULE ClassModule
);


VOID
NTAPI
LibraryReleaseClientReference(
	PLIBRARY_MODULE LibModule
);


NTSTATUS
NTAPI
LibraryUnlinkClient(
	PLIBRARY_MODULE LibModule,
	PWDF_BIND_INFO BindInfo
);

VOID
NTAPI
ClientCleanupAndFree(
	IN PCLIENT_MODULE ClientModule
);

VOID
NTAPI
LibraryUnload(
	PLIBRARY_MODULE LibModule,
	BOOLEAN RemoveFromList
);

NTSTATUS
NTAPI
LibraryLinkInClient(
	PLIBRARY_MODULE LibModule,
	PUNICODE_STRING DriverServiceName,
	PWDF_BIND_INFO BindInfo,
	PVOID Context,
	PCLIENT_MODULE* ClientModule
);
