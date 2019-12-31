#include "common\fxregkey.h"


#define AT_PASSIVE()     ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL)

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
