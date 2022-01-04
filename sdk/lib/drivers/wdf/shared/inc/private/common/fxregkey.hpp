/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRegKey.hpp

Abstract:

    This is the C++ header for FxRegKey which represents a key into the registry

Author:




Revision History:

--*/

#ifndef _FXREGKEY_H_
#define _FXREGKEY_H_

class FxRegKey : public FxPagedObject {

public:
    FxRegKey(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    __drv_maxIRQL(PASSIVE_LEVEL)
    ~FxRegKey(
        VOID
        );

    _Must_inspect_result_
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    Create(
        __in_opt  HANDLE ParentKey,
        __in      PCUNICODE_STRING KeyName,
        __in      ACCESS_MASK DesiredAccess = KEY_ALL_ACCESS,
        __in      ULONG CreateOptions = REG_OPTION_NON_VOLATILE,
        __out_opt PULONG CreateDisposition = NULL
        )
    {
        return _Create(ParentKey,
                       KeyName,
                       &m_Key,
                       DesiredAccess,
                       CreateOptions,
                       CreateDisposition);
    }

    static
    _Must_inspect_result_
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    _Create(
        __in_opt  HANDLE ParentKey,
        __in      PCUNICODE_STRING KeyName,
        __out     HANDLE* NewKey,
        __in      ACCESS_MASK DesiredAccess = KEY_ALL_ACCESS,
        __in      ULONG CreateOptions = REG_OPTION_NON_VOLATILE,
        __out_opt PULONG CreateDisposition = NULL
        );

    _Must_inspect_result_
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    Open(
        __in_opt HANDLE ParentKey,
        __in     PCUNICODE_STRING KeyName,
        __in     ACCESS_MASK DesiredAccess = KEY_ALL_ACCESS
        )
    {
        return _OpenKey(ParentKey, KeyName, &m_Key, DesiredAccess);
    }

    static
    _Must_inspect_result_
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    _OpenKey(
        __in_opt HANDLE ParentKey,
        __in     PCUNICODE_STRING KeyName,
        __out    HANDLE* Key,
        __in     ACCESS_MASK DesiredAccess = KEY_ALL_ACCESS
        );

    __inline
    VOID
    SetHandle(
        __in HANDLE Key
        )
    {
        m_Key = Key;
    }

    __inline
    HANDLE
    GetHandle(
        VOID
        )
    {
        return m_Key;
    }

    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    Close(
        VOID
        )
    {
        HANDLE key;

        key = m_Key;
        m_Key = NULL;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
        //
        // For special cases where, due to user-mode restrictions,
        // we cannot open a specific handle for write access,
        // so we reuse a pre-opened one multiple times.
        //
        // In this case we do not want to close it when we close
        // the FxRegKey object.
        //
        if (m_CanCloseHandle == FALSE) {
            return STATUS_SUCCESS;
        }
#endif
        return _Close(key);
    }

    static
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    _Close(
        __in HANDLE Key
        );

    _Must_inspect_result_
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    SetValue(
        __in PCUNICODE_STRING ValueName,
        __in ULONG ValueType,
        __in_bcount(ValueLength) PVOID Value,
        __in ULONG ValueLength
        )
    {
        return _SetValue(m_Key,
                         ValueName,
                         ValueType,
                         Value,
                         ValueLength);
    }

    static
    _Must_inspect_result_
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    _SetValue(
        _In_ HANDLE Key,
        _In_ PCUNICODE_STRING ValueName,
        _In_ ULONG ValueType,
        _In_reads_bytes_(ValueLength) PVOID Value,
        _In_ ULONG ValueLength
        );

    _Must_inspect_result_
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    QueryValue(
        __in PCUNICODE_STRING ValueName,
        __in ULONG ValueLength,
        __out_bcount_opt(ValueLength) PVOID Value,
        __out_opt PULONG ValueLengthQueried,
        __out_opt PULONG ValueType
        )
    {
        return _QueryValue(m_Globals,
                           m_Key,
                           ValueName,
                           ValueLength,
                           Value,
                           ValueLengthQueried,
                           ValueType);
    }

    static
    _Must_inspect_result_
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    _QueryValue(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in HANDLE Key,
        __in PCUNICODE_STRING ValueName,
        __in ULONG ValueLength,
        __out_bcount_opt(ValueLength) PVOID Value,
        __out_opt PULONG ValueLengthQueried,
        __out_opt PULONG ValueType
        );

    static
    _Must_inspect_result_
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    _QueryULong(
        __in  HANDLE Key,
        __in  PCUNICODE_STRING ValueName,
        __out PULONG Value
        );

    static
    _Must_inspect_result_
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    _QueryQuadWord(
        __in  HANDLE Key,
        __in  PCUNICODE_STRING ValueName,
        __out PLARGE_INTEGER Value
        );

    static
    BOOLEAN
    __inline
    _IsValidSzType(
        __in ULONG RegValueType
        )
    {
        return (RegValueType == REG_SZ) || (RegValueType == REG_EXPAND_SZ);
    }

    static
    _Must_inspect_result_
    NTSTATUS
    _VerifyMultiSzString(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PCUNICODE_STRING RegValueName,
        __in_bcount(DataLength) PWCHAR DataString,
        __in ULONG DataLength
        );

private:

    static
    __out_range(Length, (Length+sizeof(KEY_VALUE_PARTIAL_INFORMATION)-1))
    ULONG
    _ComputePartialSize(
        __in_bound ULONG Length
        )
    {
        return FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + Length;
    }

protected:

    HANDLE m_Key;
    PFX_DRIVER_GLOBALS m_Globals;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
private:

    //
    // If FALSE, then closing or destroying the FxRegKey
    // will have no effect on the HKEY m_Key.
    //
    BOOLEAN m_CanCloseHandle;

public:

    VOID
    __inline
    SetCanCloseHandle(
        BOOLEAN CanCloseHandle
        )
    {
        m_CanCloseHandle = CanCloseHandle;
    }
#endif
};

#endif // _FXREGKEY_H_
