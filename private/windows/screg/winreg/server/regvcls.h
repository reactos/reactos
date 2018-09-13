/*++









Copyright (c) 1992  Microsoft Corporation

Module Name:

    regvcls.h

Abstract:

    This file contains declarations for data structures
    needed for enumerating values under HKEY_CLASSES_ROOT

Author:

    Adam Edwards (adamed) 14-Nov-1997

Notes:

--*/

#ifdef LOCAL

#if !defined(_REGVCLS_H_)
#define _REGVCLS_H_

#include "regecls.h"

#define DEFAULT_VALUESTATE_SUBKEY_ALLOC 1

//
// Data types
//

typedef struct _ValueLocation {
    DWORD  dwOffset;
    BOOL   fUser;
} ValueLocation;

typedef struct _ValueState {

    HKEY                            hkLogical;
    HKEY                            hkUser;
    HKEY                            hkMachine;
    DWORD                           dwCurrent;
    DWORD                           cValues;
    unsigned                        fIgnoreResetOnRetry : 1;
    unsigned                        fDelete : 1;
    ValueLocation*                  rgIndex;

} ValueState;

//
// Value enumeration methods
//

void ValStateGetPhysicalIndexFromLogical(
    ValueState* pValState,
    HKEY        hkLogicalKey,
    DWORD       dwLogicalIndex,
    PHKEY       phkPhysicalKey,
    DWORD*      pdwPhysicalIndex);

NTSTATUS ValStateSetPhysicalIndexFromLogical(
    ValueState*                     pValState,
    DWORD                           dwLogicalIndex);

void ValStateRelease(ValueState* pValState);

void ValStateReleaseValues(
    PKEY_VALUE_BASIC_INFORMATION* ppValueInfo,
    DWORD                         cMaxValues);

NTSTATUS ValStateUpdate(ValueState* pValState);

NTSTATUS ValStateInitialize( 
    ValueState** ppValState,
    HKEY         hKey);

BOOL ValStateAddValueToSortedValues(
    PKEY_VALUE_BASIC_INFORMATION* ppValueInfo,
    LONG                          lNewValue);

NTSTATUS KeyStateGetValueState(
    HKEY         hKey,
    ValueState** ppValState);

NTSTATUS BaseRegGetClassKeyValueState(
    HKEY         hKey,
    DWORD        dwLogicalIndex,
    ValueState** ppValueState);

NTSTATUS EnumerateValue(
    HKEY                            hKey,
    DWORD                           dwValue,
    PKEY_VALUE_BASIC_INFORMATION    pSuggestedBuffer,
    DWORD                           dwSuggestedBufferLength,
    PKEY_VALUE_BASIC_INFORMATION*   ppResult);

//
// Multiple value query routines
//
NTSTATUS BaseRegQueryMultipleClassKeyValues(
    HKEY     hKey,
    PRVALENT val_list,
    DWORD    num_vals,
    LPSTR    lpvalueBuf,
    LPDWORD  ldwTotsize,
    PULONG   ldwRequiredLength);

NTSTATUS BaseRegQueryAndMergeValues(
    HKEY     hkUser,
    HKEY     hkMachine,
    PRVALENT val_list,
    DWORD    num_vals,
    LPSTR    lpvalueBuf,
    LPDWORD  ldwTotsize,
    PULONG   ldwRequiredLength);

#endif // !defined(_REGVCLS_H_)
#endif LOCAL















