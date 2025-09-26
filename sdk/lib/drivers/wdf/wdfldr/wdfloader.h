/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - library functions
 * COPYRIGHT:   Copyright 2019 Max Korostil <mrmks04@yandex.ru>
 *              Copyright 2021 Victor Perevertkin <victor.perevertkin@reactos.org>
 *              Copyright 2024 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#include <fxldr.h>
#include <aux_klib.h>
#include <ntintsafe.h>
#include <ntstrsafe.h>

#define WDFLDR_TAG 'LfdW'

/* PRINT macros based on the open source segments of KMDF for consistency */
#define __PrintUnfiltered(...)          \
    DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)

#define DPRINT(_x_)                                                            \
do {                                                                           \
    if (WdfLdrDiags.DiagFlags & DIAGFLAG_ENABLED) {                            \
        DbgPrint("WdfLdr: %s - ", __FUNCTION__);                               \
        __PrintUnfiltered _x_;                                                 \
    }                                                                          \
} while (0)

#define DPRINT_VERBOSE(_x_)                                                    \
do {                                                                           \
    if (WdfLdrDiags.DiagFlags & DIAGFLAG_VERBOSE_LOGGING) {                    \
        DbgPrint("WdfLdr: %s - ", __FUNCTION__);                               \
        __PrintUnfiltered _x_;                                                 \
    }                                                                          \
} while (0)

#define DPRINT_ERROR(_x_)                                                      \
do {                                                                           \
    if (WdfLdrDiags.DiagFlags & DIAGFLAG_LOG_ERRORS) {                         \
        DbgPrint("WdfLdr: ERROR: %s - ", __FUNCTION__);                        \
        __PrintUnfiltered _x_;                                                 \
    }                                                                          \
} while (0)

#define DPRINT_TRACE_ENTRY()                                                   \
do {                                                                           \
    if (WdfLdrDiags.DiagFlags & DIAGFLAG_TRACE_FUNCTION_ENTRY) {               \
        DbgPrint("WdfLdr: ENTER: %s\n", __FUNCTION__);                         \
    }                                                                          \
} while (0)

#define DPRINT_TRACE_EXIT()                                                    \
do {                                                                           \
    if (WdfLdrDiags.DiagFlags & DIAGFLAG_TRACE_FUNCTION_EXIT) {                \
        DbgPrint("WdfLdr: EXIT: %s\n", __FUNCTION__);                          \
    }                                                                          \
} while (0)

// Legacy compatibility - use do-while pattern for consistency
#define __DBGPRINT(_x_)                                                        \
do {                                                                           \
    if (WdfLdrDiags.DiagFlags & DIAGFLAG_ENABLED) {                            \
        DbgPrint("Wdfldr: %s - ", __FUNCTION__);                               \
        __PrintUnfiltered _x_;                                                 \
    }                                                                          \
} while (0)

typedef struct _WDF_LDR_GLOBALS {
    OSVERSIONINFOEXW OsVersion;
    ERESOURCE LoadedModulesListLock;
    LIST_ENTRY LoadedModulesList;
} WDF_LDR_GLOBALS, *PWDF_LDR_GLOBALS;

//
// Diagnostics Flags
//
#define DIAGFLAG_ENABLED                0x00000001
#define DIAGFLAG_VERBOSE_LOGGING        0x00000002
#define DIAGFLAG_TRACE_FUNCTION_ENTRY   0x00000004
#define DIAGFLAG_TRACE_FUNCTION_EXIT    0x00000008
#define DIAGFLAG_LOG_ERRORS             0x00000010
#define DIAGFLAG_LOG_WARNINGS           0x00000020

//
// Enhanced diagnostics structure similar to Windows 10
//
typedef struct _WDFLDR_DIAGS {
    union {
        UINT32 DiagFlags;
        struct {
            UINT32 DbgPrintOn :      1;
            UINT32 Verbose :         1;
            UINT32 FunctionEntries : 1;
            UINT32 FunctionExits :   1;
            UINT32 Errors :          1;
            UINT32 Warnings :        1;
            UINT32 Reserved :       26;
        } DiagFlagsByName;
    };
} WDFLDR_DIAGS, *PWDFLDR_DIAGS;

extern WDFLDR_DIAGS WdfLdrDiags;
extern WDF_LDR_GLOBALS WdfLdrGlobals;

typedef struct _LIBRARY_MODULE {
    LONG              LibraryRefCount;
    LIST_ENTRY        LibraryListEntry;
    LONG              ClientRefCount;
    BOOLEAN           IsBootDriver;
    BOOLEAN           ImplicitlyLoaded;
    UNICODE_STRING    ServicePath;
    UNICODE_STRING    ImageName;
    PVOID             ImageAddress;
    ULONG             ImageSize;
    PFILE_OBJECT      LibraryFileObject;
    PDRIVER_OBJECT    LibraryDriverObject;
    PWDF_LIBRARY_INFO LibraryInfo;
    LIST_ENTRY        ClientsListHead;
    ERESOURCE         ClientsListLock;
    WDF_VERSION       Version;
    KEVENT            LoaderEvent;
    PKTHREAD          LoaderThread;
    LIST_ENTRY        ClassListHead;
} LIBRARY_MODULE, *PLIBRARY_MODULE;


typedef
NTSTATUS
(NTAPI *PWDF_CLASS_BIND)(
    PWDF_BIND_INFO BindInfo,
    PWDF_COMPONENT_GLOBALS* Globals,
    PWDF_CLASS_BIND_INFO ClassBindInfo);

typedef
VOID
(NTAPI *PWDF_CLASS_UNBIND)(
    PWDF_BIND_INFO BindInfo,
    PWDF_COMPONENT_GLOBALS Globals,
    PWDF_CLASS_BIND_INFO ClassBindInfo);


typedef struct _WDF_LOADER_INTERFACE_DIAGNOSTIC {
    WDF_INTERFACE_HEADER Header;
    PWDF_LDR_DIAGNOSTICS_VALUE_BY_NAME_AS_ULONG DiagnosticsValueByNameAsULONG;
} WDF_LOADER_INTERFACE_DIAGNOSTIC, *PWDF_LOADER_INTERFACE_DIAGNOSTIC;

typedef struct _WDF_LOADER_INTERFACE_CLASS_BIND {
    WDF_INTERFACE_HEADER Header;
    PWDF_CLASS_BIND ClassBind;
    PWDF_CLASS_UNBIND ClassUnbind;
} WDF_LOADER_INTERFACE_CLASS_BIND, *PWDF_LOADER_INTERFACE_CLASS_BIND;


typedef struct _CLIENT_MODULE {
    LIST_ENTRY     LibListEntry;
    PVOID          Globals;
    PVOID          Context;
    PVOID          ImageAddr;
    ULONG          ImageSize;
    UNICODE_STRING ImageName;
    PWDF_BIND_INFO Info;
    LIST_ENTRY     ClassListHead;
} CLIENT_MODULE, *PCLIENT_MODULE;


typedef struct _CLASS_MODULE {
    ERESOURCE      ClientsListLock;
    LIST_ENTRY     ClientsListHead;
    LIST_ENTRY     LibraryLinkage;
    UNICODE_STRING Service;
    PWDF_CLASS_LIBRARY_INFO ClassLibraryInfo;
    WDF_CLASS_VERSION Version;
    PFILE_OBJECT ClassFileObject;
    PDRIVER_OBJECT ClassDriverObject;
    LONG ClassRefCount;
    LONG ClientRefCount;
    PLIBRARY_MODULE Library;
    PVOID ImageAddress;
    ULONG ImageSize;
    UNICODE_STRING ImageName;
    BOOLEAN ImplicitlyLoaded;
    BOOLEAN IsBootDriver;
    BOOLEAN ImageAlreadyLoaded;
} CLASS_MODULE, *PCLASS_MODULE;


typedef struct _CLASS_CLIENT_MODULE {
    LIST_ENTRY           ClientLinkage;
    LIST_ENTRY           ClassLinkage;
    PCLIENT_MODULE       Client;
    PCLASS_MODULE        Class;
    PWDF_CLASS_BIND_INFO ClientClassBindInfo;
} CLASS_CLIENT_MODULE, *PCLASS_CLIENT_MODULE;

// class.c

PCLASS_MODULE
ClassCreate(
    _In_ PWDF_CLASS_LIBRARY_INFO ClassLibInfo,
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PUNICODE_STRING ServiceName);

NTSTATUS
ClassOpen(
    _Inout_ PCLASS_MODULE ClassModule,
    _In_ PUNICODE_STRING ObjectName);

PLIST_ENTRY
LibraryAddToClassListLocked(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PCLASS_MODULE ClassModule);

VOID
ClassRemoveFromLibraryList(
    _In_ PCLASS_MODULE ClassModule);

PLIST_ENTRY
LibraryUnloadClasses(
    _In_ PLIBRARY_MODULE LibModule);

PCLASS_CLIENT_MODULE
ClassClientCreate();

NTSTATUS
ReferenceClassVersion(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PCLASS_MODULE* ClassModule);

VOID
DereferenceClassVersion(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PWDF_COMPONENT_GLOBALS Globals);

NTSTATUS
ClassLinkInClient(
    _In_ PCLASS_MODULE ClassModule,
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PCLASS_CLIENT_MODULE ClassClientModule);

PCLASS_MODULE
FindClassByServiceNameLocked(
    _In_ PUNICODE_STRING Path,
    _Out_ PLIBRARY_MODULE* LibModule);

VOID
NTAPI
ClassAddReference(
    _In_ PCLASS_MODULE ClassModule);

VOID
ClassClose(
    _In_ PCLASS_MODULE ClassModule);

VOID
ClassReleaseClientReference(
    _In_ PCLASS_MODULE ClassModule);

// common.c

VOID
FxLdrAcquireLoadedModuleLock(VOID);

VOID
FxLdrReleaseLoadedModuleLock(VOID);

NTSTATUS
GetImageInfo(
    _In_ PCUNICODE_STRING ImageName,
    _Out_ PVOID* ImageBase,
    _Out_ PULONG ImageSize);

NTSTATUS
GetImageName(
    _In_ PCUNICODE_STRING ServicePath,
    _In_ PUNICODE_STRING ImageName);

VOID
GetNameFromPath(
    _In_ PCUNICODE_STRING Path,
    _Out_ PUNICODE_STRING Name);

// library.c

NTSTATUS
LibraryCreate(
    _In_opt_ PWDF_LIBRARY_INFO LibraryInfo,
    _In_ PCUNICODE_STRING ServicePath,
    _Out_ PLIBRARY_MODULE* OutLibraryModule);

NTSTATUS
LibraryOpen(
    _Inout_ PLIBRARY_MODULE LibModule,
    _In_ PCUNICODE_STRING ObjectName);

VOID
LibraryClose(
    _Inout_ PLIBRARY_MODULE LibModule);

NTSTATUS
LibraryFindOrLoad(
    _In_ PCUNICODE_STRING ServicePath,
    _Out_ PLIBRARY_MODULE* LibModule);

VOID
LibraryReference(
    _In_ PLIBRARY_MODULE LibModule);

VOID
LibraryDereference(
    _In_ PLIBRARY_MODULE LibModule);

VOID
LibraryUnload(
    _In_ PLIBRARY_MODULE LibModule);

VOID
LibraryFree(
    _In_ PLIBRARY_MODULE LibModule);

BOOLEAN
LibraryAcquireClientLock(
    _In_ PLIBRARY_MODULE LibModule);

VOID
LibraryReleaseClientLock(
    _In_ PLIBRARY_MODULE LibModule);

NTSTATUS
LibraryLinkInClient(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PUNICODE_STRING ServicePath,
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PVOID Context,
    _Out_ PCLIENT_MODULE* OutClientModule);

BOOLEAN
LibraryUnlinkClient(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PWDF_BIND_INFO BindInfo);

PLIBRARY_MODULE
FindLibraryByServicePathLocked(
    _In_ PCUNICODE_STRING ServicePath);

VOID
NTAPI
LibraryReleaseReference(
    _In_ PLIBRARY_MODULE LibModule);

NTSTATUS
NTAPI
FindModuleByClientService(
    _In_ PUNICODE_STRING RegistryPath,
    _Out_ PLIBRARY_MODULE* Library);

// Version management functions
NTSTATUS
NTAPI
ReferenceVersion(
    _In_ PWDF_BIND_INFO Info,
    _Out_ PLIBRARY_MODULE* Module);

NTSTATUS
NTAPI
DereferenceVersion(
    _In_ PWDF_BIND_INFO Info,
    _In_opt_ PWDF_COMPONENT_GLOBALS Globals);

// registry.c

NTSTATUS
BuildServicePath(
    _In_ PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation,
    _In_ PUNICODE_STRING ServicePath);

NTSTATUS
GetVersionServicePath(
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PUNICODE_STRING ServicePath);

BOOLEAN
ServiceCheckBootStart(
    _In_ PUNICODE_STRING Service);

NTSTATUS
FxLdrQueryUlong(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _Out_  PULONG Value);

NTSTATUS
FxLdrQueryData(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_ ULONG Tag,
    _Out_  PKEY_VALUE_PARTIAL_INFORMATION* KeyValPartialInfo);
