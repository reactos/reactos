/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - class functions
 * COPYRIGHT:   Copyright 2019 mrmks04 (mrmks04@yandex.ru)
 */


#pragma once

#include "common.h"


NTSTATUS
NTAPI
ClassOpen(
	PCLASS_MODULE ClassModule,
	PUNICODE_STRING DeviceObject
);

PCLASS_MODULE
NTAPI
ClassCreate(
	PWDF_CLASS_LIBRARY_INFO ClassLibInfo,
	PLIBRARY_MODULE LibModule,
	PUNICODE_STRING SourceString
);

VOID
NTAPI
ClassUnload(
	PCLASS_MODULE ClassModule,
	BOOLEAN RemoveFromList
);

VOID
NTAPI
ClassCleanupAndFree(
	PCLASS_MODULE ClassModule
);

PCLASS_CLIENT_MODULE
NTAPI
ClassClientCreate();

NTSTATUS
NTAPI
ClassLinkInClient(
	PCLASS_MODULE ClassModule,
	PWDF_CLASS_BIND_INFO ClassBindInfo,
	PWDF_BIND_INFO BindInfo,
	PCLASS_CLIENT_MODULE ClassClientModule
);

NTSTATUS
NTAPI
ReferenceClassVersion(
	PWDF_CLASS_BIND_INFO ClassBindInfo,
	PWDF_BIND_INFO BindInfo,
	PCLASS_MODULE* ClassModule
);

PCLASS_MODULE
NTAPI
FindClassByServiceNameLocked(
	IN PUNICODE_STRING Path,
	OUT PLIBRARY_MODULE* LibModule
);

PLIST_ENTRY
NTAPI
LibraryAddToClassListLocked(
	PLIBRARY_MODULE LibModule,
	PCLASS_MODULE ClassModule
);

VOID
NTAPI
ClassRemoveFromLibraryList(
	PCLASS_MODULE ClassModule
);

VOID
NTAPI
DereferenceClassVersion(
	PWDF_CLASS_BIND_INFO ClassBindInfo,
	PWDF_BIND_INFO BindInfo,
	PWDF_COMPONENT_GLOBALS Globals
);
