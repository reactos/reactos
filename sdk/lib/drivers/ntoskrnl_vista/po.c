/*
 * PROJECT:     ReactOS Kernel - Vista+ APIs
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Po functions of Vista+
 * COPYRIGHT:   2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include <ntdef.h>
#include <ntifs.h>

NTKRNLVISTAAPI
NTSTATUS
NTAPI
PoRegisterPowerSettingCallback(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ LPCGUID SettingGuid,
    _In_ PPOWER_SETTING_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Outptr_opt_ PVOID *Handle)
{
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(APC_LEVEL)
NTKRNLVISTAAPI
NTSTATUS
NTAPI
PoUnregisterPowerSettingCallback(
    _Inout_ PVOID Handle)
{
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
BOOLEAN
NTAPI
PoQueryWatchdogTime(
    _In_ PDEVICE_OBJECT Pdo,
    _Out_ PULONG SecondsRemaining)
{
    return FALSE;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
VOID
NTAPI
PoSetSystemWake(
    _Inout_ struct _IRP *Irp)
{

}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
BOOLEAN
NTAPI
PoGetSystemWake(
    _In_ struct _IRP *Irp)
{
    return FALSE;
}
