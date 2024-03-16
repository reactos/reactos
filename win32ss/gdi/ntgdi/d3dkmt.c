#include <win32k.h>
#include <reactos/drivers/wddm/dxgkinterface.h>
#include <debug.h>

/*
 * It looks like windows saves all the funciton pointers globally inside win32k,
 * Instead, we're going to keep it static to this file and keep it organized in struct
 * we obtained with the IOCTRL.
 */
static REACTOS_WIN32K_DXGKRNL_INTERFACE DxgAdapterCallbacks = {0};

/*
 * This looks like it's done inside DxDdStartupDxGraphics, but i'd rather keep this organized.
 * Dxg gets start inveitibly anyway it seems atleast on vista.
 */
VOID
DxStartupDxgkInt()
{
    DPRINT("DxStartupDxgkInt: Entry\n");
    //TODO: Let DxgKrnl know it's time to start all adapters, and obtain the win32k<->dxgkrnl interface via an IOCTRL.
}

/*
 * The following APIs all have the same idea,
 * Most of the parameters are stuffed in custom typedefs with a bunch of types inside them.
 * The idea here is this:
 * if we're dealing with a d3dkmt API that directly calls into a miniport if the function pointer doesn't
 * exist we're returning STATUS_PROCEDURE_NOT_FOUND.
 *
 * This essentially means the Dxgkrnl interface was never made as Win32k doesn't do any handling for these routines.
 */

NTSTATUS
APIENTRY
NtGdiDdDDICreateAllocation(_Inout_ PD3DKMT_CREATEALLOCATION unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.DxgkIntPfnCreateAllocation)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.DxgkIntPfnCreateAllocation(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDICheckMonitorPowerState(_In_ PD3DKMT_CHECKMONITORPOWERSTATE unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.DxgkIntPfnCheckMonitorPowerState)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.DxgkIntPfnCheckMonitorPowerState(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDICheckOcclusion(_In_ PD3DKMT_CHECKOCCLUSION unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.DxgkIntPfnCheckOcclusion)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.DxgkIntPfnCheckOcclusion(unnamedParam1);
}


NTSTATUS
APIENTRY
NtGdiDdDDICloseAdapter(_In_ PD3DKMT_CLOSEADAPTER unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.DxgkIntPfnCloseAdapter)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.DxgkIntPfnCloseAdapter(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDICreateContext(_Inout_ PD3DKMT_CREATECONTEXT unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.DxgkIntPfnCreateContext)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.DxgkIntPfnCreateContext(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDICreateDevice(_Inout_ PD3DKMT_CREATEDEVICE unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.DxgkIntPfnCreateDevice)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.DxgkIntPfnCreateDevice(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDICreateOverlay(_Inout_ PD3DKMT_CREATEOVERLAY unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.DxgkIntPfnCreateOverlay)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.DxgkIntPfnCreateOverlay(unnamedParam1);
}
