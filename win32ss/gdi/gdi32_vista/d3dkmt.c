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
D3DKMTCreateAllocation(_Inout_ D3DKMT_CREATEALLOCATION* unnamedParam1)
{
    return NtGdiDdDDICreateAllocation(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCheckMonitorPowerState(_In_ CONST D3DKMT_CHECKMONITORPOWERSTATE* unnamedParam1)
{
   return NtGdiDdDDICheckMonitorPowerState(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCheckOcclusion(_In_ CONST D3DKMT_CHECKOCCLUSION* unnamedParam1)
{
    return NtGdiDdDDICheckOcclusion(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCloseAdapter(_In_ CONST D3DKMT_CLOSEADAPTER* unnamedParam1)
{
    return NtGdiDdDDICloseAdapter(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCreateContext(_Inout_ D3DKMT_CREATECONTEXT* unnamedParam1)
{
    return NtGdiDdDDICreateContext(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCreateDevice(_Inout_ D3DKMT_CREATEDEVICE* unnamedParam1)
{
    return NtGdiDdDDICreateDevice(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTCreateOverlay(_Inout_ D3DKMT_CREATEOVERLAY* unnamedParam1)
{
    return NtGdiDdDDICreateOverlay(unnamedParam1);
}

BOOLEAN
WINAPI
D3DKMTCheckExclusiveOwnership(VOID)
{
    return NtGdiDdDDICheckExclusiveOwnership();
}

NTSTATUS
WINAPI
D3DKMTCreateSynchronizationObject(_Inout_ D3DKMT_CREATESYNCHRONIZATIONOBJECT* unnamedParam1)
{
    return NtGdiDdDDICreateSynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyAllocation(_In_ CONST D3DKMT_DESTROYALLOCATION* unnamedParam1)
{
    return NtGdiDdDDIDestroyAllocation(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyContext(_In_ CONST D3DKMT_DESTROYCONTEXT* unnamedParam1)
{
    return NtGdiDdDDIDestroyContext(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyDevice(_In_ CONST D3DKMT_DESTROYDEVICE* unnamedParam1)
{
    return NtGdiDdDDIDestroyDevice(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroyOverlay(_In_ CONST D3DKMT_DESTROYOVERLAY* unnamedParam1)
{
    return NtGdiDdDDIDestroyOverlay(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTDestroySynchronizationObject(_In_ CONST D3DKMT_DESTROYSYNCHRONIZATIONOBJECT* unnamedParam1)
{
    return NtGdiDdDDIDestroySynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTEscape(_In_ CONST D3DKMT_ESCAPE* unnamedParam1)
{
    return NtGdiDdDDIEscape(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTFlipOverlay(_In_ CONST D3DKMT_FLIPOVERLAY* unnamedParam1)
{
    return NtGdiDdDDIFlipOverlay(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetContextSchedulingPriority(_Inout_ D3DKMT_GETCONTEXTSCHEDULINGPRIORITY* unnamedParam1)
{
    return NtGdiDdDDIGetContextSchedulingPriority(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetDeviceState(_Inout_ D3DKMT_GETDEVICESTATE* unnamedParam1)
{
    return NtGdiDdDDIGetDeviceState(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetDisplayModeList(_Inout_ D3DKMT_GETDISPLAYMODELIST* unnamedParam1)
{
    return NtGdiDdDDIGetDisplayModeList(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetMultisampleMethodList(_Inout_ D3DKMT_GETMULTISAMPLEMETHODLIST* unnamedParam1)
{
    return NtGdiDdDDIGetMultisampleMethodList(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetPresentHistory(_Inout_ D3DKMT_GETPRESENTHISTORY* unnamedParam1)
{
    return NtGdiDdDDIGetPresentHistory(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetProcessSchedulingPriorityClass(
        _In_  HANDLE                                unnamedParam1,
        _Out_ D3DKMT_SCHEDULINGPRIORITYCLASS        *unnamedParam2)
{
    return NtGdiDdDDIGetProcessSchedulingPriorityClass(unnamedParam1, unnamedParam2);
}

NTSTATUS
WINAPI
D3DKMTGetRuntimeData(_In_ CONST D3DKMT_GETRUNTIMEDATA* unnamedParam1)
{
    return NtGdiDdDDIGetRuntimeData(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetScanLine(_In_ D3DKMT_GETSCANLINE* unnamedParam1)
{
    return NtGdiDdDDIGetScanLine(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTGetSharedPrimaryHandle(_Inout_ D3DKMT_GETSHAREDPRIMARYHANDLE* unnamedParam1)
{
    return NtGdiDdDDIGetSharedPrimaryHandle(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTInvalidateActiveVidPn(_In_ CONST D3DKMT_INVALIDATEACTIVEVIDPN* unnamedParam1)
{
    return NtGdiDdDDIInvalidateActiveVidPn(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTLock(_Inout_ D3DKMT_LOCK* unnamedParam1)
{
    return NtGdiDdDDILock(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTOpenAdapterFromDeviceName(_Inout_ D3DKMT_OPENADAPTERFROMDEVICENAME* unnamedParam1)
{
    return NtGdiDdDDIOpenAdapterFromDeviceName(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTOpenAdapterFromGdiDisplayName(_Inout_ D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME* unnamedParam1)
{
    return NtGdiDdDDIOpenAdapterFromGdiDisplayName(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTOpenAdapterFromHdc(_Inout_ D3DKMT_OPENADAPTERFROMHDC* unnamedParam1)
{
    return NtGdiDdDDIOpenAdapterFromHdc(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTOpenResource(_Inout_ D3DKMT_OPENRESOURCE* unnamedParam1)
{
    return NtGdiDdDDIOpenResource(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTPollDisplayChildren(_In_ CONST D3DKMT_POLLDISPLAYCHILDREN* unnamedParam1)
{
    return NtGdiDdDDIPollDisplayChildren(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTPresent(_In_ D3DKMT_PRESENT* unnamedParam1)
{
    return NtGdiDdDDIPresent(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTQueryAdapterInfo(_Inout_ CONST D3DKMT_QUERYADAPTERINFO* unnamedParam1)
{
    return NtGdiDdDDIQueryAdapterInfo(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTQueryAllocationResidency(_In_ CONST D3DKMT_QUERYALLOCATIONRESIDENCY* unnamedParam1)
{
    return NtGdiDdDDIQueryAllocationResidency(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTQueryResourceInfo(_Inout_ D3DKMT_QUERYRESOURCEINFO* unnamedParam1)
{
    return NtGdiDdDDIQueryResourceInfo(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTQueryStatistics(_Inout_ CONST D3DKMT_QUERYSTATISTICS* unnamedParam1)
{
    return NtGdiDdDDIQueryStatistics(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTReleaseProcessVidPnSourceOwners(_In_ HANDLE unnamedParam1)
{
    return NtGdiDdDDIReleaseProcessVidPnSourceOwners(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTRender(_In_ D3DKMT_RENDER* unnamedParam1)
{
    return NtGdiDdDDIRender(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetAllocationPriority(_In_ CONST D3DKMT_SETALLOCATIONPRIORITY* unnamedParam1)
{
    return NtGdiDdDDISetAllocationPriority(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetContextSchedulingPriority(_In_ CONST D3DKMT_SETCONTEXTSCHEDULINGPRIORITY* unnamedParam1)
{
    return NtGdiDdDDISetContextSchedulingPriority(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetDisplayMode(_In_ CONST D3DKMT_SETDISPLAYMODE* unnamedParam1)
{
    return NtGdiDdDDISetDisplayMode(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetDisplayPrivateDriverFormat(_In_ CONST D3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT* unnamedParam1)
{
    return NtGdiDdDDISetDisplayPrivateDriverFormat(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetGammaRamp(_In_ CONST D3DKMT_SETGAMMARAMP* unnamedParam1)
{
    return NtGdiDdDDISetGammaRamp(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetProcessSchedulingPriorityClass(
                _In_ HANDLE                                    unnamedParam1,
                _In_ D3DKMT_SCHEDULINGPRIORITYCLASS            unnamedParam2)
{
    return NtGdiDdDDISetProcessSchedulingPriorityClass(unnamedParam1,unnamedParam2);
}

NTSTATUS
WINAPI
D3DKMTSetQueuedLimit(_Inout_ CONST D3DKMT_SETQUEUEDLIMIT* unnamedParam1)
{
    return NtGdiDdDDISetQueuedLimit(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSetVidPnSourceOwner(_In_ CONST D3DKMT_SETVIDPNSOURCEOWNER* unnamedParam1)
{
    return NtGdiDdDDISetVidPnSourceOwner(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSharedPrimaryLockNotification(_In_ CONST D3DKMT_SHAREDPRIMARYLOCKNOTIFICATION* unnamedParam1)
{
    return NtGdiDdDDISharedPrimaryLockNotification(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSharedPrimaryUnLockNotification(_In_ CONST D3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION* unnamedParam1)
{
    return NtGdiDdDDISharedPrimaryUnLockNotification(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTSignalSynchronizationObject(_In_ CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECT* unnamedParam1)
{
    return NtGdiDdDDISignalSynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTUnlock(_In_ CONST D3DKMT_UNLOCK* unnamedParam1)
{
    return NtGdiDdDDIUnlock(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTUpdateOverlay(_In_ CONST D3DKMT_UPDATEOVERLAY* unnamedParam1)
{
    return NtGdiDdDDIUpdateOverlay(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTWaitForIdle(_In_ CONST D3DKMT_WAITFORIDLE* unnamedParam1)
{
    return NtGdiDdDDIWaitForIdle(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTWaitForSynchronizationObject(_In_ CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECT* unnamedParam1)
{
    return NtGdiDdDDIWaitForSynchronizationObject(unnamedParam1);
}

NTSTATUS
WINAPI
D3DKMTWaitForVerticalBlankEvent(_In_ CONST D3DKMT_WAITFORVERTICALBLANKEVENT* unnamedParam1)
{
    return NtGdiDdDDIWaitForVerticalBlankEvent(unnamedParam1);
}
