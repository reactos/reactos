/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     D3DKMT dxgkrnl syscalls
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#include <gdi32_vista.h>
#include <d3dkmthk.h>
#include <debug.h>

NTSTATUS
WINAPI
D3DKMTCreateDevice(_Inout_ D3DKMT_CREATEDEVICE* unnamedParam1)
{
    NTSTATUS Status = STATUS_SUCCESS;
    Status = NtGdiDdDDICreateDevice(unnamedParam1);
    if (Status == STATUS_PROCEDURE_NOT_FOUND)
    {
        return STATUS_SUCCESS;
    }
    return Status;
}

NTSTATUS
WINAPI
D3DKMTDestroyDevice(_In_ CONST D3DKMT_DESTROYDEVICE* unnamedParam1)
{
    NTSTATUS Status = STATUS_SUCCESS;
    Status = NtGdiDdDDIDestroyDevice(unnamedParam1);
    if (Status == STATUS_PROCEDURE_NOT_FOUND)
    {
        /* Fallback to XDDM */
         return STATUS_SUCCESS;
    }
    return Status;
}

NTSTATUS
WINAPI
D3DKMTCloseAdapter(_In_ CONST D3DKMT_CLOSEADAPTER* unnamedParam1)
{
    NTSTATUS Status = STATUS_SUCCESS;
    Status = NtGdiDdDDICloseAdapter(unnamedParam1);
    if (Status == STATUS_PROCEDURE_NOT_FOUND)
    {
        /* Fallback to XDDM */
        return STATUS_SUCCESS;
    }
    return Status;
}

NTSTATUS
WINAPI
D3DKMTSetVidPnSourceOwner(_In_ CONST D3DKMT_SETVIDPNSOURCEOWNER* unnamedParam1)
{
    NTSTATUS Status = STATUS_SUCCESS;
    Status = NtGdiDdDDISetVidPnSourceOwner(unnamedParam1);
    if (Status == STATUS_PROCEDURE_NOT_FOUND)
    {
        /* Fallback to XDDM */
        return STATUS_SUCCESS;
    }
    return Status;
}

NTSTATUS
WINAPI
D3DKMTOpenAdapterFromGdiDisplayName(_Inout_ D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME* unnamedParam1)
{
    return 0;
}

NTSTATUS
WINAPI
D3DKMTOpenAdapterFromLuid(_Inout_ CONST D3DKMT_OPENADAPTERFROMLUID* unnamedParam1)
{
    return 0;
}

NTSTATUS
WINAPI
D3DKMTQueryVideoMemoryInfo(
    D3DKMT_QUERYVIDEOMEMORYINFO *unnamedParam1)
{
    //TODO: Call the win7+ syscall NtGdiDdQueryVideoMemoryInfo
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    DD_GETAVAILDRIVERMEMORYDATA Data = {0};
    HDC hdc = NULL;
    HANDLE hDirectDraw = NULL;

    if (!unnamedParam1)
        return STATUS_INVALID_PARAMETER;

    switch (unnamedParam1->MemorySegmentGroup)
    {
        case D3DKMT_MEMORY_SEGMENT_GROUP_LOCAL:
            Data.DDSCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
            break;
        case D3DKMT_MEMORY_SEGMENT_GROUP_NON_LOCAL:
            Data.DDSCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM;
            break;
        default:
            Data.DDSCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
            break;
    }

    hdc = NtGdiCreateCompatibleDC(NULL);
    if (!hdc)
        return STATUS_UNSUCCESSFUL;

    hDirectDraw = NtGdiDdCreateDirectDrawObject(hdc);
    if (!hDirectDraw)
    {
        NtGdiDeleteObjectApp(hdc);
        return STATUS_UNSUCCESSFUL;
    }

    Status = NtGdiDdGetAvailDriverMemory(hDirectDraw, &Data);

    if (NT_SUCCESS(Status))
    {
        unnamedParam1->Budget = Data.dwTotal;
        unnamedParam1->CurrentUsage = Data.dwTotal - Data.dwFree;
        unnamedParam1->CurrentReservation = 0;
        unnamedParam1->AvailableForReservation = Data.dwFree;
    }
    else
    {
        unnamedParam1->Budget = 0;
        unnamedParam1->CurrentUsage = 0;
        unnamedParam1->CurrentReservation = 0;
        unnamedParam1->AvailableForReservation = 0;
    }

    NtGdiDeleteObjectApp(hdc);

    return Status;
}
