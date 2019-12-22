/*
Abstract:
    This module contains private interfaces to the WDF loader.  This interface
    is used by
    a) the stub code in the client driver
    b) the framework
    c) the framework loader
--*/

#ifndef __FXLDR_H__
#define __FXLDR_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct _LIBRARY_MODULE* PLIBRARY_MODULE;
typedef struct _WDF_LIBRARY_INFO* PWDF_LIBRARY_INFO;

typedef
VOID
(*WDFFUNC)(
    VOID
    );

typedef ULONG WDF_MAJOR_VERSION;
typedef ULONG WDF_MINOR_VERSION;
typedef ULONG WDF_BUILD_NUMBER;
typedef PVOID WDF_COMPONENT_GLOBALS, *PWDF_COMPONENT_GLOBALS;

//
// Version container
//
typedef struct _WDF_VERSION {
    WDF_MAJOR_VERSION  Major;
    WDF_MINOR_VERSION  Minor;
    WDF_BUILD_NUMBER   Build;
} WDF_VERSION;

//
// WDF bind information structure.
//
typedef struct _WDF_BIND_INFO {
    ULONG              Size;
    PWCHAR             Component;
    WDF_VERSION        Version;
    ULONG              FuncCount;
    __field_bcount(FuncCount*sizeof(WDFFUNC)) WDFFUNC* FuncTable;

#ifndef __FXLDRUM_H__
    PLIBRARY_MODULE    Module;     // Mgmt and diagnostic use only
#else
    PVOID              Module;
#endif
} WDF_BIND_INFO, * PWDF_BIND_INFO;

NTSTATUS
WdfBindClientHelper(
    _Inout_ PWDF_BIND_INFO       BindInfo,
    _In_ WDF_MAJOR_VERSION       FxMajorVersion,
    _In_ WDF_MINOR_VERSION       FxMinorVersion
    );

typedef
_Must_inspect_result_
NTSTATUS
(*PFNLIBRARYCOMMISSION)(
    VOID
    );

typedef
_Must_inspect_result_
NTSTATUS
(*PFNLIBRARYDECOMMISSION)(
    VOID
    );

typedef
_Must_inspect_result_
NTSTATUS
(*PFNLIBRARYREGISTERCLIENT)(
    __in PWDF_BIND_INFO             Info,
    __deref_out   PWDF_COMPONENT_GLOBALS   * ComponentGlobals,
    __deref_inout PVOID                    * Context
    );

typedef
_Must_inspect_result_
NTSTATUS
(*PFNLIBRARYUNREGISTERCLIENT)(
    __in PWDF_BIND_INFO             Info,
    __in PWDF_COMPONENT_GLOBALS     DriverGlobals
    );

#define WDF_REGISTRY_DBGPRINT_ON   L"DbgPrintOn"

typedef struct _WDF_LIBRARY_INFO {
    ULONG                             Size;
    PFNLIBRARYCOMMISSION              LibraryCommission;
    PFNLIBRARYDECOMMISSION            LibraryDecommission;
    PFNLIBRARYREGISTERCLIENT          LibraryRegisterClient;
    PFNLIBRARYUNREGISTERCLIENT        LibraryUnregisterClient;
    WDF_VERSION                       Version;
} WDF_LIBRARY_INFO, *PWDF_LIBRARY_INFO;

#define WDF_LIBRARY_COMMISSION          LibraryCommission
#define WDF_LIBRARY_DECOMMISSION        LibraryDecommission
#define WDF_LIBRARY_REGISTER_CLIENT     LibraryRegisterClient
#define WDF_LIBRARY_UNREGISTER_CLIENT   LibraryUnregisterClient

typedef struct _CLIENT_INFO
{
	ULONG           Size;
	PUNICODE_STRING RegistryPath;
} CLIENT_INFO, *PCLIENT_INFO;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //__FXLDR_H__