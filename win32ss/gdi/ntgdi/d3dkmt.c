#include <win32k.h>
#include <reactos/rddm/rxgkinterface.h>
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

BOOLEAN
APIENTRY
NtGdiDdDDICheckExclusiveOwnership(VOID)
{
    // We don't support DWM at this time, excusive ownership is always false.
    return FALSE;
}

NTSTATUS
APIENTRY
NtGdiDdDDIGetProcessSchedulingPriorityClass(_In_  HANDLE                                unnamedParam1,
                                            _Out_ D3DKMT_SCHEDULINGPRIORITYCLASS        *unnamedParam2)
{
    return 1;
}

NTSTATUS
APIENTRY
NtGdiDdDDISetProcessSchedulingPriorityClass(
                    _In_ HANDLE                                     unnamedParam1,
                    _In_ D3DKMT_SCHEDULINGPRIORITYCLASS             unnamedParam2)
{
    return 1;
}

NTSTATUS
APIENTRY
NtGdiDdDDISharedPrimaryLockNotification(_In_ const D3DKMT_SHAREDPRIMARYLOCKNOTIFICATION* unnamedParam1)
{
    return 1;
}

NTSTATUS
APIENTRY
NtGdiDdDDISharedPrimaryUnLockNotification(_In_ const D3DKMT_SHAREDPRIMARYUNLOCKNOTIFICATION* unnamedParam1)
{
    return 1;
}

NTSTATUS
APIENTRY
NtGdiDdDDIOpenAdapterFromGdiDisplayName(_Inout_ D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME* unnamedParam1)
{
   return 0;
}

NTSTATUS
APIENTRY
NtGdiDdDDIOpenAdapterFromHdc(_Inout_ D3DKMT_OPENADAPTERFROMHDC* unnamedParam1)
{
    return 0;
}


NTSTATUS
APIENTRY
NtGdiDdDDIOpenAdapterFromDeviceName(_Inout_ D3DKMT_OPENADAPTERFROMDEVICENAME* unnamedParam1)
{
    return 0;
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
NtGdiDdDDICreateAllocation(_Inout_ D3DKMT_CREATEALLOCATION* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnCreateAllocation)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnCreateAllocation(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDICheckMonitorPowerState(_In_ const D3DKMT_CHECKMONITORPOWERSTATE* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnCheckMonitorPowerState)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnCheckMonitorPowerState(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDICheckOcclusion(_In_ const D3DKMT_CHECKOCCLUSION* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnCheckOcclusion)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnCheckOcclusion(unnamedParam1);
}


NTSTATUS
APIENTRY
NtGdiDdDDICloseAdapter(_In_ const D3DKMT_CLOSEADAPTER* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnCloseAdapter)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnCloseAdapter(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDICreateContext(_Inout_ D3DKMT_CREATECONTEXT* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnCreateContext)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnCreateContext(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDICreateDevice(_Inout_ D3DKMT_CREATEDEVICE* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnCreateDevice)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnCreateDevice(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDICreateOverlay(_Inout_ D3DKMT_CREATEOVERLAY* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnCreateOverlay)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnCreateOverlay(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDICreateSynchronizationObject(_Inout_ D3DKMT_CREATESYNCHRONIZATIONOBJECT* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnCreateSynchronizationObject)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnCreateSynchronizationObject(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIDestroyAllocation(_In_ const D3DKMT_DESTROYALLOCATION* unnamedParam1)
{
  if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnDestroyAllocation)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnDestroyAllocation(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIDestroyContext(_In_ const D3DKMT_DESTROYCONTEXT* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnDestroyContext)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnDestroyContext(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIDestroyDevice(_In_ const D3DKMT_DESTROYDEVICE* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnDestroyDevice)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnDestroyDevice(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIDestroyOverlay(_In_ const D3DKMT_DESTROYOVERLAY* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnDestroyOverlay)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnDestroyOverlay(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIDestroySynchronizationObject(_In_ const D3DKMT_DESTROYSYNCHRONIZATIONOBJECT* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnDestroySynchronizationObject)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnDestroySynchronizationObject(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIEscape(_In_ const D3DKMT_ESCAPE* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnEscape)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnEscape(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIFlipOverlay(_In_ const D3DKMT_FLIPOVERLAY* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnFlipOverlay)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnFlipOverlay(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIGetContextSchedulingPriority(_Inout_ D3DKMT_GETCONTEXTSCHEDULINGPRIORITY* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnGetContextSchedulingPriority)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnGetContextSchedulingPriority(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIGetDeviceState(_Inout_ D3DKMT_GETDEVICESTATE* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnGetDeviceState)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnGetDeviceState(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIGetDisplayModeList(_Inout_ D3DKMT_GETDISPLAYMODELIST* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnGetDisplayModeList)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnGetDisplayModeList(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIGetMultisampleMethodList(_Inout_ D3DKMT_GETMULTISAMPLEMETHODLIST* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnGetMultisampleMethodList)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnGetMultisampleMethodList(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIGetPresentHistory(_Inout_ D3DKMT_GETPRESENTHISTORY* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnGetPresentHistory)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnGetPresentHistory(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIGetRuntimeData(_In_ const D3DKMT_GETRUNTIMEDATA* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnGetRuntimeData)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnGetRuntimeData(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIGetScanLine(_In_ D3DKMT_GETSCANLINE* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnGetScanLine)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnGetScanLine(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIGetSharedPrimaryHandle(_Inout_ D3DKMT_GETSHAREDPRIMARYHANDLE* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnGetSharedPrimaryHandle)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnGetSharedPrimaryHandle(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIInvalidateActiveVidPn(_In_ const D3DKMT_INVALIDATEACTIVEVIDPN* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnInvalidateActiveVidPn)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnInvalidateActiveVidPn(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDILock(_Inout_ D3DKMT_LOCK* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnLock)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnLock(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIOpenResource(_Inout_ D3DKMT_OPENRESOURCE* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnOpenResource)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnOpenResource(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIPollDisplayChildren(_In_ const D3DKMT_POLLDISPLAYCHILDREN* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnPollDisplayChildren)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnPollDisplayChildren(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIPresent(_In_ D3DKMT_PRESENT* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnPresent)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnPresent(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIQueryAdapterInfo(_Inout_ const D3DKMT_QUERYADAPTERINFO* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnQueryAdapterInfo)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnQueryAdapterInfo(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIQueryAllocationResidency(_In_ const D3DKMT_QUERYALLOCATIONRESIDENCY* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnQueryAllocationResidency)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnQueryAllocationResidency(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIQueryResourceInfo(_Inout_ D3DKMT_QUERYRESOURCEINFO* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnQueryResourceInfo)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnQueryResourceInfo(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIQueryStatistics(_Inout_ const D3DKMT_QUERYSTATISTICS* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnQueryStatistics)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnQueryStatistics(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIReleaseProcessVidPnSourceOwners(_In_ HANDLE unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnReleaseProcessVidPnSourceOwners)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnReleaseProcessVidPnSourceOwners(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIRender(_In_ D3DKMT_RENDER* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnRender)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnRender(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDISetAllocationPriority(_In_ const D3DKMT_SETALLOCATIONPRIORITY* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnSetAllocationPriority)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnSetAllocationPriority(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDISetContextSchedulingPriority(_In_ const D3DKMT_SETCONTEXTSCHEDULINGPRIORITY* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnSetContextSchedulingPriority)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnSetContextSchedulingPriority(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDISetDisplayMode(_In_ const D3DKMT_SETDISPLAYMODE* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnSetDisplayMode)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnSetDisplayMode(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDISetDisplayPrivateDriverFormat(_In_ const D3DKMT_SETDISPLAYPRIVATEDRIVERFORMAT* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnSetDisplayPrivateDriverFormat)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnSetDisplayPrivateDriverFormat(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDISetGammaRamp(_In_ const D3DKMT_SETGAMMARAMP* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnSetGammaRamp)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnSetGammaRamp(unnamedParam1);
}


NTSTATUS
APIENTRY
NtGdiDdDDISetQueuedLimit(_Inout_ const D3DKMT_SETQUEUEDLIMIT* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnSetQueuedLimit)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnSetQueuedLimit(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDISetVidPnSourceOwner(_In_ const D3DKMT_SETVIDPNSOURCEOWNER* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnSetVidPnSourceOwner)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnSetVidPnSourceOwner(unnamedParam1);
}

NTSTATUS
WINAPI
NtGdiDdDDIUnlock(_In_ const D3DKMT_UNLOCK* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnUnlock)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnUnlock(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIUpdateOverlay(_In_ const D3DKMT_UPDATEOVERLAY* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnUpdateOverlay)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnUpdateOverlay(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIWaitForIdle(_In_ const D3DKMT_WAITFORIDLE* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnWaitForIdle)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnWaitForIdle(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIWaitForSynchronizationObject(_In_ const D3DKMT_WAITFORSYNCHRONIZATIONOBJECT* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnWaitForSynchronizationObject)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnWaitForSynchronizationObject(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDIWaitForVerticalBlankEvent(_In_ const D3DKMT_WAITFORVERTICALBLANKEVENT* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnWaitForVerticalBlankEvent)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnWaitForVerticalBlankEvent(unnamedParam1);
}

NTSTATUS
APIENTRY
NtGdiDdDDISignalSynchronizationObject(_In_ const D3DKMT_SIGNALSYNCHRONIZATIONOBJECT* unnamedParam1)
{
    if (!unnamedParam1)
        STATUS_INVALID_PARAMETER;

    if (!DxgAdapterCallbacks.RxgkIntPfnSignalSynchronizationObject)
        return STATUS_PROCEDURE_NOT_FOUND;

    return DxgAdapterCallbacks.RxgkIntPfnSignalSynchronizationObject(unnamedParam1);
}
