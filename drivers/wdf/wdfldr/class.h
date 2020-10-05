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
    _In_ PWDF_CLASS_LIBRARY_INFO ClassLibInfo,
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PUNICODE_STRING SourceString);

VOID
NTAPI
ClassUnload(
    _In_ PCLASS_MODULE ClassModule,
    _In_ BOOLEAN RemoveFromList);

VOID
NTAPI
ClassCleanupAndFree(
    _In_ PCLASS_MODULE ClassModule);

PCLASS_CLIENT_MODULE
NTAPI
ClassClientCreate();

NTSTATUS
NTAPI
ClassLinkInClient(
    _In_ PCLASS_MODULE ClassModule,
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PCLASS_CLIENT_MODULE ClassClientModule);

NTSTATUS
NTAPI
ReferenceClassVersion(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PCLASS_MODULE* ClassModule);

PCLASS_MODULE
NTAPI
FindClassByServiceNameLocked(
    _In_  PUNICODE_STRING Path,
    _Out_ PLIBRARY_MODULE* LibModule
);

PLIST_ENTRY
NTAPI
LibraryAddToClassListLocked(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PCLASS_MODULE ClassModule);

VOID
NTAPI
ClassRemoveFromLibraryList(
    _In_ PCLASS_MODULE ClassModule
);

VOID
NTAPI
DereferenceClassVersion(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PWDF_COMPONENT_GLOBALS Globals);
