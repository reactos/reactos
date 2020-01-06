#include "common/fxregkey.h"
#include "common/fxmacros.h"
#include "common/mxmemory.h"
#include "common/dbgtrace.h"


#define AT_PASSIVE()     ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL)

_Must_inspect_result_
NTSTATUS
FxRegKey::_VerifyMultiSzString(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PCUNICODE_STRING RegValueName,
    __in_bcount(DataLength) PWCHAR DataString,
    __in ULONG DataLength
    )
/*++

Routine Description:
    This static function checks the input buffer to verify that it contains
    a multi-sz string with a double-NULL termination at the end of the buffer.

Arguments:
    DataString - buffer containing multi-sz strings. If there are no strings
        in the buffer, the buffer should at least contain two UNICODE_NULL
        characters.

    DataLength - the size in bytes of the input buffer.

Return Value:
    STATUS_OBJECT_TYPE_MISMATCH - if the the data buffer is off odd-length,
        or it doesnt end with two UNICODE_NULL characters.

    STATUS_SUCCESS - if the buffer contains valid multi-sz strings.

  --*/
{
    ULONG numChars;

    if ((DataLength % 2) != 0)
    {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "Reg value name %wZ, DataLength %d, Data buffer length is invalid, "
            "STATUS_OBJECT_TYPE_MISMATCH",
            RegValueName, DataLength);
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    numChars = DataLength / sizeof(WCHAR);
    if (numChars < 2 ||
        DataString[numChars-1] != UNICODE_NULL ||
        DataString[numChars-2] != UNICODE_NULL)
    {

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "Read value name %wZ, DataLength %d, Data buffer from registry does not "
            "have double NULL terminal chars, STATUS_OBJECT_TYPE_MISMATCH",
            RegValueName, DataLength);

        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxRegKey::_OpenKey(
    __in_opt  HANDLE ParentKey,
    __in  PCUNICODE_STRING KeyName,
    __out HANDLE* Key,
    __in  ACCESS_MASK DesiredAccess
    )
{
    OBJECT_ATTRIBUTES oa;

    AT_PASSIVE();

    //
    // Force OBJ_KERNEL_HANDLE because we are never passing the handle back
    // up to a process and we don't want to create a handle in an arbitrary
    // process from which that process can close the handle out from underneath
    // us.
    //
    InitializeObjectAttributes(
        &oa,
        (PUNICODE_STRING)KeyName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        ParentKey,
        NULL);

    return ZwOpenKey(Key, DesiredAccess, &oa);
}

__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxRegKey::_Close(
    __in HANDLE Key
    )
{
    return ZwClose(Key);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxRegKey::_QueryULong(
    __in  HANDLE Key,
    __in  PCUNICODE_STRING ValueName,
    __out PULONG Value
    )
{
    NTSTATUS status;
    ULONG length;

    PKEY_VALUE_PARTIAL_INFORMATION pPartial;
    UCHAR buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)+(sizeof(ULONG))];

    length = sizeof(buffer);
    pPartial = (PKEY_VALUE_PARTIAL_INFORMATION) &buffer[0];

    status = ZwQueryValueKey(Key,
                             (PUNICODE_STRING)ValueName,
                             KeyValuePartialInformation,
                             pPartial,
                             length,
                             &length);

    if ((NT_SUCCESS(status) || status == STATUS_BUFFER_OVERFLOW) &&
         pPartial->Type != REG_DWORD)
    {
        status = STATUS_OBJECT_TYPE_MISMATCH;
    }

    if (NT_SUCCESS(status))
    {
        ASSERT(sizeof(ULONG) == pPartial->DataLength);

        RtlCopyMemory(Value, &pPartial->Data[0], sizeof(ULONG));
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxRegKey::_QueryValue(
    __in_opt PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in HANDLE Key,
    __in PCUNICODE_STRING ValueName,
    __in ULONG ValueLength,
    __out_bcount_opt(ValueLength) PVOID Value,
    __out_opt PULONG ValueLengthQueried,
    __out_opt PULONG ValueType
    )
{
    KEY_VALUE_PARTIAL_INFORMATION *pPartial, partial;
    NTSTATUS status;
    ULONG length;
    ULONG tag;

    tag = FxDriverGlobals ? FxDriverGlobals->Tag : FX_TAG;

    if (Value == NULL)
    {
        //
        // Caller wants just the length
        //
        pPartial = &partial;
        length = _ComputePartialSize(0);
        RtlZeroMemory(&partial, length);
    }
    else
    {
        length = _ComputePartialSize(ValueLength);
        pPartial = (PKEY_VALUE_PARTIAL_INFORMATION)
            MxMemory::MxAllocatePoolWithTag(PagedPool, length, tag);

        if (pPartial == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // We always pass a buffer of at least sizeof(KEY_VALUE_PARTIAL_INFORMATION)
    // to ZwQueryValueKey. This means that ZwQueryValueKey will write at least
    // some information to the buffer it receives, even if the user-supplied data
    // buffer is NULL or too small.
    //
    // According to ZwQueryValueKey's contract, this means that it will never return
    // STATUS_BUFFER_TOO_SMALL (returned when no data is written). Therefore, if the
    // user passes a NULL or insufficient buffer and the value exists in the registry,
    // ZwQueryValueKey will return STATUS_BUFFER_OVERFLOW.
    //
    status = ZwQueryValueKey(Key,
                             (PUNICODE_STRING)ValueName,
                             KeyValuePartialInformation,
                             pPartial,
                             length,
                             &length);

    if (NT_SUCCESS(status) && Value != NULL && (ValueLength >= pPartial->DataLength))
    {
        RtlCopyMemory(Value, &pPartial->Data[0], pPartial->DataLength);
    }

    if (NT_SUCCESS(status) || status == STATUS_BUFFER_OVERFLOW)
    {
        if (ValueLengthQueried != NULL)
        {
            *ValueLengthQueried = pPartial->DataLength;
        }
        if (ValueType != NULL)
        {
            *ValueType = pPartial->Type;
        }
    }

    if (pPartial != &partial)
    {
        MxMemory::MxFreePool(pPartial);
    }

    return status;
}
