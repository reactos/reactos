/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - common functions and types
 * COPYRIGHT:   Copyright 2019 mrmks04 (mrmks04@yandex.ru)
 */


#pragma once

#include <ntddk.h>
#include <wdm.h>

typedef struct _WDF_BIND_INFO* PWDF_BIND_INFO;
typedef struct _WDF_CLASS_BIND_INFO* PWDF_CLASS_BIND_INFO;
typedef PVOID WDF_COMPONENT_GLOBALS, *PWDF_COMPONENT_GLOBALS;

BOOLEAN WdfLdrDiags;
LONG gKlibInitialized;
ERESOURCE Resource;
LIST_ENTRY gLibList;
typedef int (NTAPI* PRtlQueryModuleInformation)(PULONG, ULONG, PVOID);
PRtlQueryModuleInformation pfnRtlQueryModuleInformation;

#define WDFLDR_TAG 'LfdW'


typedef struct _WDF_INTERFACE_HEADER {
	PGUID InterfaceType;
	ULONG InterfaceSize;
} WDF_INTERFACE_HEADER, *PWDF_INTERFACE_HEADER;


typedef struct _WDF_VERSION {
	ULONG Major;
	ULONG Minor;
	ULONG Build;
} WDF_VERSION, *PWDF_VERSION;

typedef
NTSTATUS
(*PFNLIBRARYCOMMISSION)(
	VOID
	);

typedef
NTSTATUS
(*PFNLIBRARYDECOMMISSION)(
	VOID
	);

typedef
NTSTATUS
(*PFNLIBRARYREGISTERCLIENT)(
	PWDF_BIND_INFO Info,
	PWDF_COMPONENT_GLOBALS* ComponentGlobals,
	PVOID* Context
	);

typedef
NTSTATUS
(*PFNLIBRARYUNREGISTERCLIENT)(
	PWDF_BIND_INFO             Info,
	PWDF_COMPONENT_GLOBALS     DriverGlobals
	);

typedef struct _WDF_LIBRARY_INFO {
	ULONG Size;
	PFNLIBRARYCOMMISSION LibraryCommission;
	PFNLIBRARYDECOMMISSION LibraryDecommission;
	PFNLIBRARYREGISTERCLIENT LibraryRegisterClient;
	PFNLIBRARYUNREGISTERCLIENT LibraryUnregisterClient;
	WDF_VERSION Version;
} WDF_LIBRARY_INFO, *PWDF_LIBRARY_INFO;


typedef struct _LIBRARY_MODULE {
	LONG              LibraryRefCount;
	LIST_ENTRY        LibraryListEntry;
	LONG              ClientRefCount;
	BOOLEAN           IsBootDriver;
	BOOLEAN           ImplicitlyLoaded;
	BOOLEAN           ImageAlreadyLoaded;
	UNICODE_STRING    Service;
	UNICODE_STRING    ImageName;
	PVOID             ImageAddress;
	ULONG             ImageSize;
	PFILE_OBJECT      LibraryFileObject;
	PDRIVER_OBJECT    LibraryDriverObject;
	PWDF_LIBRARY_INFO LibraryInfo;
	LIST_ENTRY        ClientsListHead;
	ERESOURCE         ClientsListLock;
	WDF_VERSION       Version;
	LIST_ENTRY        ClassListHead;
} LIBRARY_MODULE, *PLIBRARY_MODULE;


typedef struct _WDF_BIND_INFO {
	ULONG           Size;
	wchar_t* Component;
	WDF_VERSION     Version;
	ULONG           FuncCount;
	void(__fastcall** FuncTable)();
	PLIBRARY_MODULE Module;
} WDF_BIND_INFO, *PWDF_BIND_INFO;


typedef struct _WDF_LOADER_INTERFACE {
	WDF_INTERFACE_HEADER Header;
	int(__stdcall* RegisterLibrary)(PWDF_LIBRARY_INFO, PUNICODE_STRING, PUNICODE_STRING);
	int(__stdcall* VersionBind)(PDRIVER_OBJECT, PUNICODE_STRING, PWDF_BIND_INFO, void***);
	NTSTATUS(__stdcall* VersionUnbind)(PUNICODE_STRING, PWDF_BIND_INFO, PWDF_COMPONENT_GLOBALS);
	NTSTATUS(__stdcall* DiagnosticsValueByNameAsULONG)(PUNICODE_STRING, PULONG);
} WDF_LOADER_INTERFACE, *PWDF_LOADER_INTERFACE;


typedef struct _CLIENT_MODULE {
	LIST_ENTRY     LibListEntry;
	PVOID          *Globals;
	PVOID          Context;
	PVOID          ImageAddr;
	ULONG          ImageSize;
	UNICODE_STRING ImageName;
	PWDF_BIND_INFO Info;
	LIST_ENTRY     ClassListHead;
} CLIENT_MODULE, *PCLIENT_MODULE;


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
	int (NTAPI* ClientBindClass)(int (NTAPI*)(PWDF_BIND_INFO, PVOID*, PWDF_CLASS_BIND_INFO), PWDF_BIND_INFO, PVOID*, PWDF_CLASS_BIND_INFO);
	VOID(NTAPI* ClientUnbindClass)(void (NTAPI*)(PWDF_BIND_INFO, PVOID*, PWDF_CLASS_BIND_INFO), PWDF_BIND_INFO, PVOID*, PWDF_CLASS_BIND_INFO);
	PVOID ClassModule;
} WDF_CLASS_BIND_INFO, *PWDF_CLASS_BIND_INFO;


typedef struct _WDF_CLASS_LIBRARY_INFO {
	ULONG Size;
	WDF_CLASS_VERSION Version;
	int (NTAPI* ClassLibraryInitialize)();
	void (NTAPI* ClassLibraryDeinitialize)();
	int (NTAPI* ClassLibraryBindClient)(PWDF_CLASS_BIND_INFO, void**);
	void (NTAPI* ClassLibraryUnbindClient)(PWDF_CLASS_BIND_INFO, void**);
} WDF_CLASS_LIBRARY_INFO, *PWDF_CLASS_LIBRARY_INFO;


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


typedef struct _CLIENT_INFO {
	ULONG           Size;
	PUNICODE_STRING RegistryPath;
} CLIENT_INFO, *PCLIENT_INFO;


NTSTATUS
NTAPI
GetImageName(
	IN PUNICODE_STRING DriverServiceName,
	IN ULONG Tag,
	IN PUNICODE_STRING ImageName
);

NTSTATUS
NTAPI
GetImageBase(
	IN PCUNICODE_STRING ImageName,
	OUT PVOID* ImageBase,
	OUT PULONG ImageSize
);

BOOLEAN
NTAPI
ServiceCheckBootStart(
	IN PUNICODE_STRING Service
);

NTSTATUS
NTAPI
FxLdrQueryUlong(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	OUT PULONG Value
);

NTSTATUS
NTAPI
FxLdrQueryData(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN ULONG Tag,
	OUT PKEY_VALUE_PARTIAL_INFORMATION* KeyValPartialInfo
);

VOID
FreeString(
	IN PUNICODE_STRING String
);

VOID
FxLdrAcquireLoadedModuleLock();

VOID
FxLdrReleaseLoadedModuleLock();

NTSTATUS
NTAPI
ConvertUlongToWString(
	ULONG Value,
	PUNICODE_STRING String
);

NTSTATUS
NTAPI
BuildServicePath(
	IN PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation,
	IN ULONG Tag,
	IN PUNICODE_STRING ServicePath
);

VOID
NTAPI
GetNameFromUnicodePath(
	IN PUNICODE_STRING Path,
	IN OUT PWCHAR Dest,
	IN LONG DestSize
);
