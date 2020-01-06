#ifndef _FXREGKEY_H_
#define _FXREGKEY_H_

#include <ntddk.h>
#include "fxpagedobject.h"
#include "common/fxglobals.h"

class FxRegKey : public FxPagedObject {

public:


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

    static
    __drv_maxIRQL(PASSIVE_LEVEL)
    NTSTATUS
    _Close(
        __in HANDLE Key
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
    _QueryValue(
        __in_opt PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in HANDLE Key,
        __in PCUNICODE_STRING ValueName,
        __in ULONG ValueLength,
        __out_bcount_opt(ValueLength) PVOID Value,
        __out_opt PULONG ValueLengthQueried,
        __out_opt PULONG ValueType
        );

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
    //__out_range(Length, (Length+sizeof(KEY_VALUE_PARTIAL_INFORMATION)-1))
    ULONG
    _ComputePartialSize(
        ULONG Length
        )
    {
        return FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + Length;
    }

};

#endif //_FXREGKEY_H_