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




#define __Print(_x_)                    \
{                                       \
    if (WdfLdrDbgPrintOn) {             \
    }                                   \
}

#define WDF_ENHANCED_VERIFIER_OPTIONS_VALUE_NAME      L"EnhancedVerifierOptions"

typedef
NTSTATUS
(*PFN_RTL_GET_VERSION)(
    OUT PRTL_OSVERSIONINFOW VersionInformation
    );

NTSTATUS
FxLibraryCommonCommission(
    VOID
    );

NTSTATUS
FxLibraryCommonDecommission(
    VOID
    );

NTSTATUS
FxLibraryCommonRegisterClient(
    PWDF_BIND_INFO        Info,
    PWDF_DRIVER_GLOBALS * WdfDriverGlobals,
    PCLIENT_INFO          ClientInfo
    );

NTSTATUS
FxLibraryCommonUnregisterClient(
    PWDF_BIND_INFO        Info,
    PWDF_DRIVER_GLOBALS   WdfDriverGlobals
    );

VOID
GetEnhancedVerifierOptions(
    PCLIENT_INFO ClientInfo,
    PULONG Options
    );

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FX_LIBRARY_COMMON_H__
