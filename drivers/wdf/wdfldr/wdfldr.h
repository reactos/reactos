/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - exported functions
 * COPYRIGHT:   Copyright 2019 mrmks04 (mrmks04@yandex.ru)
 */


#pragma once

#include "common.h"


NTSTATUS
NTAPI
WdfRegisterLibrary(
    _In_ PWDF_LIBRARY_INFO LibraryInfo,
    _In_ PUNICODE_STRING ServicePath,
    _In_ PUNICODE_STRING LibraryDeviceName
);

NTSTATUS
NTAPI
WdfRegisterClassLibrary(
    _In_ PWDF_CLASS_LIBRARY_INFO ClassLibInfo,
    _In_ PUNICODE_STRING SourceString,
    _In_ PUNICODE_STRING ObjectName);

VOID
NTAPI
DllUnload();

NTSTATUS
NTAPI
DllInitialize(
    _In_ PUNICODE_STRING RegistryPath);


NTSTATUS
NTAPI
WdfLdrDiagnosticsValueByNameAsULONG(
    _In_ PUNICODE_STRING ValueName,
    _Out_ PULONG Value);


NTSTATUS
NTAPI
WdfVersionBind(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath,
    _Inout_ PWDF_BIND_INFO BindInfo,
    _Out_ PWDF_COMPONENT_GLOBALS *ComponentGlobals);


NTSTATUS
NTAPI
WdfVersionUnbind(
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PWDF_COMPONENT_GLOBALS ComponentGlobals);


NTSTATUS
NTAPI
WdfVersionBindClass(
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PWDF_COMPONENT_GLOBALS Globals,
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo);


VOID
NTAPI
WdfVersionUnbindClass(
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PWDF_COMPONENT_GLOBALS Globals,
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo);

NTSTATUS
NTAPI
WdfLdrQueryInterface(
    _In_ PWDF_LOADER_INTERFACE LoaderInterface);
