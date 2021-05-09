/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRegKey.cpp

Abstract:

Author:

Environment:

    user mode only

Revision History:

--*/

#include "FxSupportPch.hpp"

//#define UNICODE
//#define _UNICODE
#include <Winreg.h>

extern "C" {
#if defined(EVENT_TRACING)
#include "FxRegKeyUM.tmh"
#endif
}

FxRegKey::FxRegKey(
    PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxPagedObject(FX_TYPE_REG_KEY, sizeof(FxRegKey), FxDriverGlobals),
    m_Key(NULL),
    m_Globals(FxDriverGlobals),
    m_CanCloseHandle(TRUE)
{
}

__drv_maxIRQL(PASSIVE_LEVEL)
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "Can't apply kernel mode annotations.");
FxRegKey::~FxRegKey()
{
    if (m_Key != NULL) {
        if (m_CanCloseHandle == TRUE) {
            RegCloseKey((HKEY)m_Key);
        }
        m_Key = NULL;
    }
}

NTSTATUS
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "Can't apply kernel mode annotations.");
FxRegKey::_Close(
    __in HANDLE Key
    )
{
    DWORD err = RegCloseKey((HKEY)Key);

    if (ERROR_SUCCESS == err) {
        return STATUS_SUCCESS;
    }
    else {
        return WinErrorToNtStatus(err);
    }
}

_Must_inspect_result_
NTSTATUS
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "Can't apply kernel mode annotations.");
FxRegKey::_Create(
    __in_opt  HANDLE ParentKey,
    __in  PCUNICODE_STRING KeyName,
    __out HANDLE* NewKey,
    __in  ACCESS_MASK DesiredAccess,
    __in  ULONG CreateOptions,
    __out_opt PULONG CreateDisposition
    )
{
    HKEY parentKey;

    if (NULL == ParentKey)
    {
        parentKey = HKEY_LOCAL_MACHINE;
    }
    else
    {
        parentKey = (HKEY) ParentKey;
    }

    DWORD err = RegCreateKeyEx(parentKey,
                          KeyName->Buffer,
                          0,
                          NULL,
                          CreateOptions,
                          DesiredAccess,
                          NULL,
                          (PHKEY)NewKey,
                          CreateDisposition);

    if (ERROR_SUCCESS == err) {
        return STATUS_SUCCESS;
    }
    else {
        return WinErrorToNtStatus(err);
    }
}

_Must_inspect_result_
NTSTATUS
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "Can't apply kernel mode annotations.");
FxRegKey::_OpenKey(
    __in_opt  HANDLE ParentKey,
    __in  PCUNICODE_STRING KeyName,
    __out HANDLE* Key,
    __in  ACCESS_MASK DesiredAccess
    )
{
    HKEY parentKey;

    if (NULL == ParentKey)
    {
        parentKey = HKEY_LOCAL_MACHINE;
    }
    else
    {
        parentKey = (HKEY) ParentKey;
    }

    DWORD err = RegOpenKeyEx(parentKey,
                             KeyName->Buffer,
                             0,
                             DesiredAccess,
                             (PHKEY)Key);

    if (ERROR_SUCCESS == err) {
        return STATUS_SUCCESS;
    }
    else {
        return WinErrorToNtStatus(err);
    }
}

_Must_inspect_result_
NTSTATUS
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "Can't apply kernel mode annotations.");
FxRegKey::_SetValue(
    _In_ HANDLE Key,
    _In_ PCUNICODE_STRING ValueName,
    _In_ ULONG ValueType,
    _In_reads_bytes_(ValueLength) PVOID Value,
    _In_ ULONG ValueLength
    )
{
    DWORD err;

    err = RegSetValueEx((HKEY)Key,
                        ValueName->Buffer,
                        0,
                        ValueType,
                        (BYTE*)Value,
                        ValueLength);
    if (ERROR_SUCCESS == err) {
        return STATUS_SUCCESS;
    }
    else {
        return WinErrorToNtStatus(err);
    }
}

_Must_inspect_result_
NTSTATUS
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "Can't apply kernel mode annotations.");
FxRegKey::_QueryValue(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in HANDLE Key,
    __in PCUNICODE_STRING ValueName,
    __in ULONG ValueLength,
    __out_bcount_opt(ValueLength) PVOID Value,
    __out_opt PULONG ValueLengthQueried,
    __out_opt PULONG ValueType
    )
{
    DWORD err;
    NTSTATUS status;
    ULONG length;

    UNREFERENCED_PARAMETER(FxDriverGlobals);
    ASSERT(Key != HKEY_PERFORMANCE_DATA);

    length = ValueLength;

    err = RegQueryValueEx((HKEY)Key,
                           ValueName->Buffer,
                           NULL,
                           ValueType,
                           (LPBYTE)Value,
                           &length);

    if (ValueLengthQueried != NULL) {
        *ValueLengthQueried = length;
    }

    //
    // Please see the comment in FxRegKeyKm.cpp FxRegKey::_QueryValue about
    // the call to ZwQueryValueKey.
    //
    // If the user supplies a NULL data buffer, RegQueryValueEx will return
    // ERROR_SUCCESS. However, in order to satisfy UMDF-KMDF DDI parity as well
    // as internal mode-agnostic code, we must overwrite RegQueryValueEx's
    // return value of ERROR_SUCCESS (STATUS_SUCCESS) with STATUS_BUFFER_OVERFLOW.
    //
    // Other return values are overwritten because WinErrorToNtStatus does not map
    // all Win32 error codes that RegQueryValueEx returns to the same NTSTATUS
    // values that ZwQueryValueKey would return in the KM implementation of
    // FxRegKey::_QueryValue.
    //
    if (err == ERROR_SUCCESS) {
        if (Value != NULL) {
            status = STATUS_SUCCESS;
        }
        else {
            status = STATUS_BUFFER_OVERFLOW;
        }
    }
    else if (err == ERROR_MORE_DATA) {
        status = STATUS_BUFFER_OVERFLOW;
    }
    else if (err == ERROR_FILE_NOT_FOUND) {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    else {
        status = WinErrorToNtStatus(err);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "Can't apply kernel mode annotations.");
FxRegKey::_QueryULong(
    __in  HANDLE Key,
    __in  PCUNICODE_STRING ValueName,
    __out PULONG Value
    )
{
    DWORD err;
    NTSTATUS status;
    ULONG length, type;

    ASSERT(Key != HKEY_PERFORMANCE_DATA);

    type = REG_DWORD;
    length = sizeof(ULONG);

    err = RegQueryValueEx((HKEY)Key,
                           ValueName->Buffer,
                           NULL,
                           &type,
                           (LPBYTE)Value,
                           &length);

    if ((err == ERROR_SUCCESS || err == ERROR_MORE_DATA) &&
         type != REG_DWORD) {

        ASSERT(FALSE);

        status = STATUS_OBJECT_TYPE_MISMATCH;
    }
    else {
        if (ERROR_SUCCESS == err) {
            status = STATUS_SUCCESS;
        }
        else {
            status = WinErrorToNtStatus(err);
        }
    }

    return status;
}

_Must_inspect_result_
#pragma prefast(suppress:__WARNING_UNMATCHED_DECL_ANNO, "Can't apply kernel mode annotations.");
NTSTATUS
FxRegKey::_QueryQuadWord(
    __in  HANDLE Key,
    __in  PCUNICODE_STRING ValueName,
    __out PLARGE_INTEGER Value
    )
{
    UNREFERENCED_PARAMETER(Key);
    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(Value);

    return STATUS_UNSUCCESSFUL;
}


