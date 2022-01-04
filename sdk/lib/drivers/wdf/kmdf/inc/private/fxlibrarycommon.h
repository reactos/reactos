//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef __FX_LIBRARY_COMMON_H__
#define __FX_LIBRARY_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern ULONG WdfLdrDbgPrintOn;
extern PCHAR WdfLdrType;

extern WDFVERSION WdfVersion;

extern RTL_OSVERSIONINFOW  gOsVersion;

#define _LIT_(a)    # a
#define LITERAL(a) _LIT_(a)

#define __PrintUnfiltered(...)          \
    DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__);

#define __Print(_x_)                                                           \
{                                                                              \
    if (WdfLdrDbgPrintOn) {                                                    \
        DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "%s: ", WdfLdrType); \
        __PrintUnfiltered _x_                                                  \
    }                                                                          \
}

#define WDF_ENHANCED_VERIFIER_OPTIONS_VALUE_NAME      L"EnhancedVerifierOptions"

typedef
NTSTATUS
(*PFN_RTL_GET_VERSION)(
    __out PRTL_OSVERSIONINFOW VersionInformation
    );

_Must_inspect_result_
NTSTATUS
FxLibraryCommonCommission(
    VOID
    );

_Must_inspect_result_
NTSTATUS
FxLibraryCommonDecommission(
    VOID
    );

_Must_inspect_result_
NTSTATUS
FxLibraryCommonRegisterClient(
    __inout PWDF_BIND_INFO        Info,
    __deref_out PWDF_DRIVER_GLOBALS * WdfDriverGlobals,
    __in_opt PCLIENT_INFO          ClientInfo
    );

_Must_inspect_result_
NTSTATUS
FxLibraryCommonUnregisterClient(
    __in PWDF_BIND_INFO        Info,
    __in PWDF_DRIVER_GLOBALS   WdfDriverGlobals
    );

VOID
GetEnhancedVerifierOptions(
    __in PCLIENT_INFO ClientInfo,
    __out PULONG Options
    );

VOID
LibraryLogEvent(
    __in PDRIVER_OBJECT DriverObject,
    __in NTSTATUS       ErrorCode,
    __in NTSTATUS       FinalStatus,
    __in PWSTR          ErrorInsertionString,
    __in_bcount(RawDataLen) PVOID    RawDataBuf,
    __in USHORT         RawDataLen
);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FX_LIBRARY_COMMON_H__
