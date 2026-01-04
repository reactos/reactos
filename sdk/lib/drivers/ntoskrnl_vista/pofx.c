
#include <ntdef.h>
#include <ntifs.h>
#include <debug.h>


NTKRNLVISTAAPI
NTSTATUS
NTAPI
PoFxRegisterDevice(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ PPO_FX_DEVICE Device,
    _Out_ POHANDLE *Handle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxUnregisterDevice(
    _In_ POHANDLE Handle)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxStartDevicePowerManagement(
    _In_ POHANDLE Handle)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxActivateComponent(
    _In_ POHANDLE Handle,
    _In_ ULONG Component,
    _In_ ULONG Flags)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxCompleteDevicePowerNotRequired(
    _In_ POHANDLE Handle)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxIdleComponent(
    _In_ POHANDLE Handle,
    _In_ ULONG Component,
    _In_ ULONG Flags)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxCompleteIdleCondition(
    _In_ POHANDLE Handle,
    _In_ ULONG Component)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxCompleteIdleState(
    _In_ POHANDLE Handle,
    _In_ ULONG Component)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxSetDeviceIdleTimeout(
    _In_ POHANDLE Handle,
    _In_ ULONGLONG IdleTimeout)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
PoFxReportDevicePoweredOn(
    _In_ POHANDLE Handle)
{
    UNIMPLEMENTED;
}
