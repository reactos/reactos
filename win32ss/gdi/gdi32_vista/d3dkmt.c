/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     D3DKMT dxgkrnl syscalls
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#include <gdi32_vista.h>
#include <d3dkmddi.h>
#include <winuser.h>

/*
 * <d3dkmthk.h> hides D3DKMT_OPENADAPTERFROMLUID and D3DKMT_QUERYVIDEOMEMORYINFO
 * behind DXGKDDI_INTERFACE_VERSION gates of WIN8 and WDDM2_2, while the tree
 * targets Vista. for now but honestly this can change.
 */

#if (DXGKDDI_INTERFACE_VERSION < DXGKDDI_INTERFACE_VERSION_WIN8)
typedef struct _D3DKMT_OPENADAPTERFROMLUID
{
    LUID            AdapterLuid;
    D3DKMT_HANDLE   hAdapter;
} D3DKMT_OPENADAPTERFROMLUID;
#endif

#if (DXGKDDI_INTERFACE_VERSION < DXGKDDI_INTERFACE_VERSION_WDDM2_2)
typedef struct _D3DKMT_QUERYVIDEOMEMORYINFO
{
    HANDLE                      hProcess;
    D3DKMT_HANDLE               hAdapter;
    D3DKMT_MEMORY_SEGMENT_GROUP MemorySegmentGroup;
    UINT64                      Budget;
    UINT64                      CurrentUsage;
    UINT64                      CurrentReservation;
    UINT64                      AvailableForReservation;
    UINT                        PhysicalAdapterIndex;
} D3DKMT_QUERYVIDEOMEMORYINFO;
#endif

#define D3DKMT_EMU_ADAPTER_TAG  0x0ada0000u
#define D3DKMT_EMU_DEVICE_TAG   0x0de00000u
#define D3DKMT_EMU_INDEX_MASK   0x0000ffffu

#define D3DKMT_EMU_MAX_ADAPTERS 16
#define D3DKMT_EMU_MAX_DEVICES  64

typedef struct _D3DKMT_EMU_ADAPTER
{
    LONG InUse;
    LUID AdapterLuid;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_EMU_ADAPTER;

typedef struct _D3DKMT_EMU_DEVICE
{
    LONG InUse;
    D3DKMT_HANDLE hAdapter;
} D3DKMT_EMU_DEVICE;

static D3DKMT_EMU_ADAPTER D3DKMTEmuAdapters[D3DKMT_EMU_MAX_ADAPTERS];
static D3DKMT_EMU_DEVICE D3DKMTEmuDevices[D3DKMT_EMU_MAX_DEVICES];

static
D3DKMT_EMU_ADAPTER*
D3DKMTEmuGetAdapter(
    _In_ D3DKMT_HANDLE hAdapter)
{
    ULONG Index;

    if ((hAdapter & ~D3DKMT_EMU_INDEX_MASK) != D3DKMT_EMU_ADAPTER_TAG)
        return NULL;

    Index = hAdapter & D3DKMT_EMU_INDEX_MASK;
    if (Index == 0 || Index > D3DKMT_EMU_MAX_ADAPTERS)
        return NULL;

    if (!D3DKMTEmuAdapters[Index - 1].InUse)
        return NULL;

    return &D3DKMTEmuAdapters[Index - 1];
}

static
D3DKMT_EMU_DEVICE*
D3DKMTEmuGetDevice(
    _In_ D3DKMT_HANDLE hDevice)
{
    ULONG Index;

    if ((hDevice & ~D3DKMT_EMU_INDEX_MASK) != D3DKMT_EMU_DEVICE_TAG)
        return NULL;

    Index = hDevice & D3DKMT_EMU_INDEX_MASK;
    if (Index == 0 || Index > D3DKMT_EMU_MAX_DEVICES)
        return NULL;

    if (!D3DKMTEmuDevices[Index - 1].InUse)
        return NULL;

    return &D3DKMTEmuDevices[Index - 1];
}

static
D3DKMT_HANDLE
D3DKMTEmuOpenAdapter(
    _In_ const LUID* AdapterLuid,
    _In_ D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId)
{
    ULONG Index;

    for (Index = 0; Index < D3DKMT_EMU_MAX_ADAPTERS; Index++)
    {
        if (InterlockedCompareExchange(&D3DKMTEmuAdapters[Index].InUse, 1, 0) == 0)
        {
            D3DKMTEmuAdapters[Index].AdapterLuid = *AdapterLuid;
            D3DKMTEmuAdapters[Index].VidPnSourceId = VidPnSourceId;
            return D3DKMT_EMU_ADAPTER_TAG | (Index + 1);
        }
    }

    return 0;
}

/* fake LUID */
static
VOID
D3DKMTEmuMakeLuid(
    _In_ D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId,
    _Out_ LUID* AdapterLuid)
{
    AdapterLuid->HighPart = 0x524f5300; /* 'ROS\0' */
    AdapterLuid->LowPart = VidPnSourceId + 1;
}

/* Not just a syscall even in wine. */
NTSTATUS
WINAPI
D3DKMTOpenAdapterFromGdiDisplayName(_Inout_ D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME* unnamedParam1)
{
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId = 0;
    DISPLAY_DEVICEW DisplayDevice;
    D3DKMT_HANDLE hAdapter;
    LUID AdapterLuid;
    DWORD Index = 0;

    if (!unnamedParam1)
        return STATUS_INVALID_PARAMETER;

    /* Locate the GDI display device with this name to obtain its index, which
       doubles as the VidPN source ID */
    DisplayDevice.cb = sizeof(DisplayDevice);
    while (EnumDisplayDevicesW(NULL, Index, &DisplayDevice, 0))
    {
        if (wcsncmp(DisplayDevice.DeviceName,
                    unnamedParam1->DeviceName,
                    ARRAYSIZE(unnamedParam1->DeviceName)) == 0)
        {
            VidPnSourceId = Index;
            break;
        }

        DisplayDevice.cb = sizeof(DisplayDevice);
        Index++;
    }

    if (!DisplayDevice.DeviceName[0] || VidPnSourceId != Index)
        return STATUS_INVALID_PARAMETER;

    D3DKMTEmuMakeLuid(VidPnSourceId, &AdapterLuid);

    hAdapter = D3DKMTEmuOpenAdapter(&AdapterLuid, VidPnSourceId);
    if (!hAdapter)
        return STATUS_INSUFFICIENT_RESOURCES;

    unnamedParam1->hAdapter = hAdapter;
    unnamedParam1->AdapterLuid = AdapterLuid;
    unnamedParam1->VidPnSourceId = VidPnSourceId;

    return STATUS_SUCCESS;
}

NTSTATUS
WINAPI
D3DKMTOpenAdapterFromLuid(_Inout_ CONST D3DKMT_OPENADAPTERFROMLUID* unnamedParam1)
{
    D3DKMT_OPENADAPTERFROMLUID* Desc = (D3DKMT_OPENADAPTERFROMLUID*)unnamedParam1;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
    D3DKMT_HANDLE hAdapter;

    if (!unnamedParam1)
        return STATUS_INVALID_PARAMETER;

    /* Recover the source ID for LUIDs we minted ourselves; anything else is a
       caller-allocated LUID, for which the primary output is the best match. */
    if (unnamedParam1->AdapterLuid.HighPart == 0x524f5300 &&
        unnamedParam1->AdapterLuid.LowPart > 0)
    {
        VidPnSourceId = unnamedParam1->AdapterLuid.LowPart - 1;
    }
    else
    {
        VidPnSourceId = 0;
    }

    hAdapter = D3DKMTEmuOpenAdapter(&unnamedParam1->AdapterLuid, VidPnSourceId);
    if (!hAdapter)
        return STATUS_INSUFFICIENT_RESOURCES;

    Desc->hAdapter = hAdapter;

    return STATUS_SUCCESS;
}

NTSTATUS
WINAPI
D3DKMTCloseAdapter(_In_ const D3DKMT_CLOSEADAPTER* unnamedParam1)
{
    D3DKMT_EMU_ADAPTER* Adapter;
    NTSTATUS Status;

    if (!unnamedParam1)
        return STATUS_INVALID_PARAMETER;

    Status = NtGdiDdDDICloseAdapter(unnamedParam1);
    if (Status != STATUS_PROCEDURE_NOT_FOUND)
        return Status;

    Adapter = D3DKMTEmuGetAdapter(unnamedParam1->hAdapter);
    if (!Adapter)
        return STATUS_INVALID_PARAMETER;

    InterlockedExchange(&Adapter->InUse, 0);

    return STATUS_SUCCESS;
}

NTSTATUS
WINAPI
D3DKMTCreateDevice(_Inout_ D3DKMT_CREATEDEVICE* unnamedParam1)
{
    NTSTATUS Status;
    ULONG Index;

    if (!unnamedParam1)
        return STATUS_INVALID_PARAMETER;

    Status = NtGdiDdDDICreateDevice(unnamedParam1);
    if (Status != STATUS_PROCEDURE_NOT_FOUND)
        return Status;

    if (!D3DKMTEmuGetAdapter(unnamedParam1->hAdapter))
        return STATUS_INVALID_PARAMETER;

    for (Index = 0; Index < D3DKMT_EMU_MAX_DEVICES; Index++)
    {
        if (InterlockedCompareExchange(&D3DKMTEmuDevices[Index].InUse, 1, 0) == 0)
        {
            D3DKMTEmuDevices[Index].hAdapter = unnamedParam1->hAdapter;

            unnamedParam1->hDevice = D3DKMT_EMU_DEVICE_TAG | (Index + 1);
            unnamedParam1->pCommandBuffer = NULL;
            unnamedParam1->CommandBufferSize = 0;
            unnamedParam1->pAllocationList = NULL;
            unnamedParam1->AllocationListSize = 0;
            unnamedParam1->pPatchLocationList = NULL;
            unnamedParam1->PatchLocationListSize = 0;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
WINAPI
D3DKMTDestroyDevice(_In_ const D3DKMT_DESTROYDEVICE* unnamedParam1)
{
    D3DKMT_EMU_DEVICE* Device;
    NTSTATUS Status;

    if (!unnamedParam1)
        return STATUS_INVALID_PARAMETER;

    Status = NtGdiDdDDIDestroyDevice(unnamedParam1);
    if (Status != STATUS_PROCEDURE_NOT_FOUND)
        return Status;

    Device = D3DKMTEmuGetDevice(unnamedParam1->hDevice);
    if (!Device)
        return STATUS_INVALID_PARAMETER;

    InterlockedExchange(&Device->InUse, 0);

    return STATUS_SUCCESS;
}

NTSTATUS
WINAPI
D3DKMTSetVidPnSourceOwner(_In_ const D3DKMT_SETVIDPNSOURCEOWNER* unnamedParam1)
{
    NTSTATUS Status;

    if (!unnamedParam1)
        return STATUS_INVALID_PARAMETER;

    Status = NtGdiDdDDISetVidPnSourceOwner(unnamedParam1);
    if (Status != STATUS_PROCEDURE_NOT_FOUND)
        return Status;

    /* A zero VidPnSourceCount releases ownership, which always succeeds. */
    if (unnamedParam1->VidPnSourceCount == 0)
        return STATUS_SUCCESS;

    if (!D3DKMTEmuGetDevice(unnamedParam1->hDevice))
        return STATUS_INVALID_PARAMETER;

    return STATUS_PROCEDURE_NOT_FOUND;
}

NTSTATUS
WINAPI
D3DKMTCheckVidPnExclusiveOwnership(_In_ CONST D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP* unnamedParam1)
{
    if (!unnamedParam1)
        return STATUS_INVALID_PARAMETER;

    return STATUS_PROCEDURE_NOT_FOUND;
}

NTSTATUS
WINAPI
D3DKMTQueryVideoMemoryInfo(_Inout_ D3DKMT_QUERYVIDEOMEMORYINFO* unnamedParam1)
{
    if (!unnamedParam1)
        return STATUS_INVALID_PARAMETER;

    if (!D3DKMTEmuGetAdapter(unnamedParam1->hAdapter))
        return STATUS_INVALID_PARAMETER;

    return STATUS_PROCEDURE_NOT_FOUND;
}
