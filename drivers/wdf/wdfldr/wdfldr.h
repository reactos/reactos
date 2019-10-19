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
	PWDF_LIBRARY_INFO LibraryInfo,
	PUNICODE_STRING ServicePath,
	PUNICODE_STRING LibraryDeviceName
);

NTSTATUS
NTAPI
WdfRegisterClassLibrary(
	IN PWDF_CLASS_LIBRARY_INFO ClassLibInfo,
	IN PUNICODE_STRING SourceString,
	IN PUNICODE_STRING ObjectName
);

VOID
NTAPI
DllUnload();

NTSTATUS
NTAPI
DllInitialize(
	PUNICODE_STRING RegistryPath
);


NTSTATUS
NTAPI
WdfLdrDiagnosticsValueByNameAsULONG(
	PUNICODE_STRING ValueName,
	PULONG Value
);


NTSTATUS
NTAPI
WdfVersionBind(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath,
	PWDF_BIND_INFO BindInfo,
	PWDF_COMPONENT_GLOBALS *ComponentGlobals
);


NTSTATUS
NTAPI
WdfVersionUnbind(
	PUNICODE_STRING RegistryPath,
	PWDF_BIND_INFO BindInfo,
	void** ComponentGlobals
);


NTSTATUS
NTAPI
WdfVersionBindClass(
	PWDF_BIND_INFO BindInfo,
	PWDF_COMPONENT_GLOBALS Globals,
	PWDF_CLASS_BIND_INFO ClassBindInfo
);


VOID
NTAPI
WdfVersionUnbindClass(
	PWDF_BIND_INFO BindInfo,
	PWDF_COMPONENT_GLOBALS Globals,
	PWDF_CLASS_BIND_INFO ClassBindInfo
);

NTSTATUS
NTAPI
WdfLdrQueryInterface(
	IN PWDF_LOADER_INTERFACE LoaderInterface
);
