/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    WdfAssert.h

Abstract:

    Contains prototypes for dealing with run time asserts

Author:

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFASSERT_H_
#define _WDFASSERT_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)

//
// Including here because RtlAssert is not declared in XP and Win2K headers for
// free builds
//
NTSYSAPI
VOID
NTAPI
RtlAssert(
    __in PVOID FailedAssertion,
    __in PVOID FileName,
    __in ULONG LineNumber,
    __in_opt PSTR Message
    );



//
// WDFVERIFY is active both on checked and free build only if
// the wdf verifier is tuned on
//
#define WDFVERIFY(exp) {                                            \
    if ((WdfDriverGlobals->DriverFlags & WdfVerifyOn) && !(exp)) {  \
        RtlAssert( #exp, __FILE__, __LINE__, NULL );                \
    }                                                               \
}

#define VERIFY_IS_IRQL_PASSIVE_LEVEL()  WDFVERIFY(KeGetCurrentIrql() == PASSIVE_LEVEL)

//
// Following macro is obsolete and it will be phased out in due course
//
#define IS_AT_PASSIVE()                 WDFVERIFY(KeGetCurrentIrql() == PASSIVE_LEVEL)

//
// Compile time active "assert".  File will not compile if this assert is FALSE.
//
// This compile time assert is designed to catch mismatch in the values of the
// declared constants.  So suppress the OACR warning #6326 generated about the
// potential comparison of constants.
//
#define WDFCASSERT(c)   {              \
    __pragma(warning(suppress: 6326))  \
    switch(0) case (c): case 0: ;      \
    }



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFASSERT_H_

