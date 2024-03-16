/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     D3DKMT dxgkrnl syscalls
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#include <gdi32_vista.h>
#include <d3dkmddi.h>

NTSTATUS
WINAPI
D3DKMTCreateAllocation(_Inout_ PD3DKMT_CREATEALLOCATION unnamedParam1)
{
    return NtGdiDdDDICreateAllocation(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCheckMonitorPowerState(_In_ PD3DKMT_CHECKMONITORPOWERSTATE unnamedParam1)
{
    return NtGdiDdDDICheckMonitorPowerState(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCheckOcclusion(_In_ PD3DKMT_CHECKOCCLUSION unnamedParam1)
{
    return NtGdiDdDDICheckOcclusion(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCloseAdapter(_In_ PD3DKMT_CLOSEADAPTER unnamedParam1)
{
    return NtGdiDdDDICloseAdapter(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCreateContext(_Inout_ PD3DKMT_CREATECONTEXT unnamedParam1)
{
    return NtGdiDdDDICreateContext(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCreateDevice(_Inout_ PD3DKMT_CREATEDEVICE unnamedParam1)
{
    return NtGdiDdDDICreateDevice(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCreateOverlay(_Inout_ PD3DKMT_CREATEOVERLAY unnamedParam1)
{
    return NtGdiDdDDICreateOverlay(unnamedParam1);
}

#if 0


\\

NTSTATUS
WINAPI
D3DKMTCreateSynchronizationObject(D3DKMT_CREATESYNCHRONIZATIONOBJECT *unnamedParam1)
{
    return NtGdiDdDDICreateSynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyAllocation(_In_ D3DKMT_DESTROYALLOCATION *unnamedParam1)
{
    return NtGdiDdDDIDestroyAllocation(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyContext(_In_ D3DKMT_DESTROYCONTEXT *unnamedParam1)
{
    return NtGdiDdDDIDestroyContext(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyDevice(_In_ D3DKMT_DESTROYDEVICE *unnamedParam1)
{
    return NtGdiDdDDIDestroyDevice(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyOverlay(_In_ D3DKMT_DESTROYOVERLAY *unnamedParam1)
{
    return NtGdiDdDDIDestroyOverlay(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroySynchronizationObject(_In_ D3DKMT_DESTROYSYNCHRONIZATIONOBJECT *unnamedParam1)
{
    return NtGdiDdDDIDestroySynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTEscape(_In_ D3DKMT_ESCAPE *unnamedParam1)
{
    return NtGdiDdDDIEscape(unnamedParam1);
}

/* Windows 7 *****************************************************************************************/

NTSTATUS
WINAPI
D3DKMTCheckVidPnExclusiveOwnership(_In_ D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *unnamedParam1)
{
    return NtGdiDdDDICheckVidPnExclusiveOwnership(unnamedParam1);
}
#endif