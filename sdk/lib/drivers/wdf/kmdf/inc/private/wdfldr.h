/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - exported functions
 * COPYRIGHT:   Copyright 2021 Max Korostil (mrmks04@yandex.ru)
 */


#ifndef __WDFLDR_H__
#define __WDFLDR_H__

#include <ntddk.h>


typedef ULONG WDF_MAJOR_VERSION;
typedef ULONG WDF_MINOR_VERSION;
typedef ULONG WDF_BUILD_NUMBER;

typedef struct _WDF_BIND_INFO* PWDF_BIND_INFO;
typedef struct _WDF_CLASS_BIND_INFO* PWDF_CLASS_BIND_INFO;
typedef struct _WDF_LIBRARY_INFO* PWDF_LIBRARY_INFO;
typedef PVOID WDF_COMPONENT_GLOBALS, *PWDF_COMPONENT_GLOBALS;

typedef
NTSTATUS
(NTAPI *PWDF_LDR_DIAGNOSTICS_VALUE_BY_NAME_AS_ULONG)(
    PUNICODE_STRING ValueName,
    PULONG          Value);

typedef
NTSTATUS
(NTAPI *PFN_CLASS_LIBRARY_INIT)();

typedef
VOID
(NTAPI *PFN_CLASS_LIBRARY_DEINIT)();

typedef
NTSTATUS
(NTAPI *PFN_CLASS_LIBRARY_BIND_CLIENT)(
    PWDF_CLASS_BIND_INFO ClassBindInfo,
    PWDF_COMPONENT_GLOBALS Globals);

typedef
VOID
(NTAPI *PFN_CLASS_LIBRARY_UNBIND_CLIENT)(
    PWDF_CLASS_BIND_INFO ClassBindInfo,
    PWDF_COMPONENT_GLOBALS Globals);

typedef
NTSTATUS
(NTAPI *PFN_WDF_VERSION_BIND_CLASS)(
    PWDF_BIND_INFO BindInfo,
    PWDF_COMPONENT_GLOBALS Globals,
    PWDF_CLASS_BIND_INFO ClassBindInfo);

typedef
NTSTATUS
(NTAPI *PFN_CLIENT_BIND_CLASS)(
    PFN_WDF_VERSION_BIND_CLASS BindFunction,
    PWDF_BIND_INFO BindInfo,
    PWDF_COMPONENT_GLOBALS Globals,
    PWDF_CLASS_BIND_INFO ClassBindInfo
);

typedef
VOID
(NTAPI *PFN_WDF_VERSION_UNBIND_CLASS)(
    PWDF_BIND_INFO BindInfo,
    PWDF_COMPONENT_GLOBALS Globals,
    PWDF_CLASS_BIND_INFO ClassBindInfo);

typedef
VOID
(NTAPI *PFN_CLIENT_UNBIND_CLASS)(
    PFN_WDF_VERSION_UNBIND_CLASS UnbindFunction,
    PWDF_BIND_INFO BindInfo,
    PWDF_COMPONENT_GLOBALS Globals,
    PWDF_CLASS_BIND_INFO ClassBindInfo
);


typedef struct _WDF_INTERFACE_HEADER {
    const GUID *InterfaceType;
    ULONG InterfaceSize;
} WDF_INTERFACE_HEADER, *PWDF_INTERFACE_HEADER;


typedef struct _WDF_CLASS_VERSION {
    ULONG Major;
    ULONG Minor;
    ULONG Build;
} WDF_CLASS_VERSION, *PWDF_CLASS_VERSION;

typedef struct _WDF_CLASS_BIND_INFO {
    ULONG             Size;
    PWCHAR           ClassName;
    WDF_CLASS_VERSION Version;
    VOID(NTAPI** FunctionTable)();
    ULONG FunctionTableCount;
    PVOID ClassBindInfo;
    PFN_CLIENT_BIND_CLASS ClientBindClass;
    PFN_CLIENT_UNBIND_CLASS ClientUnbindClass;
    PVOID ClassModule;
} WDF_CLASS_BIND_INFO, *PWDF_CLASS_BIND_INFO;

typedef struct _WDF_CLASS_LIBRARY_INFO {
    ULONG Size;
    WDF_CLASS_VERSION Version;
    PFN_CLASS_LIBRARY_INIT ClassLibraryInitialize;
    PFN_CLASS_LIBRARY_DEINIT ClassLibraryDeinitialize;
    PFN_CLASS_LIBRARY_BIND_CLIENT ClassLibraryBindClient;
    PFN_CLASS_LIBRARY_UNBIND_CLIENT ClassLibraryUnbindClient;
} WDF_CLASS_LIBRARY_INFO, *PWDF_CLASS_LIBRARY_INFO;

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS
NTAPI
WdfRegisterClassLibrary(
    _In_ PWDF_CLASS_LIBRARY_INFO ClassLibInfo,
    _In_ PUNICODE_STRING SourceString,
    _In_ PUNICODE_STRING ObjectName);

NTSTATUS
NTAPI
WdfLdrDiagnosticsValueByNameAsULONG(
    _In_ PUNICODE_STRING ValueName,
    _Out_ PULONG Value);

NTSTATUS
NTAPI
WdfRegisterLibrary(
    _In_ PWDF_LIBRARY_INFO LibraryInfo,
    _In_ PUNICODE_STRING ServicePath,
    _In_ PCUNICODE_STRING LibraryDeviceName
);

VOID
NTAPI
DllUnload();

NTSTATUS
NTAPI
DllInitialize(
    _In_ PUNICODE_STRING RegistryPath);


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
    _In_ PWDF_INTERFACE_HEADER LoaderInterface);

#ifdef __cplusplus
}
#endif

#endif //__WDFLDR_H__
