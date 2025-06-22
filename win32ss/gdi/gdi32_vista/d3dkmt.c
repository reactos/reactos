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
        /* Fallback to XDDM */
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

/* 
 * The two functions below are NT5 hacks,
 * at some point when this code moves to main gdi32 this will be erased.
 */

HDC
FASTCALL
IntCreateDICWLoc(
    LPCWSTR lpwszDriver,
    LPCWSTR lpwszDevice,
    LPCWSTR lpwszOutput,
    PDEVMODEW lpInitData,
    ULONG iType)
{
    UNICODE_STRING Device, Output;
    HDC hdc = NULL;
    BOOL Display = FALSE, Default = FALSE;
    HANDLE UMdhpdev = 0;

    if ((!lpwszDevice) && (!lpwszDriver))
    {
        Default = TRUE;  // Ask Win32k to set Default device.
        Display = TRUE;   // Most likely to be DISPLAY.
    }
    else
    {
        if ((lpwszDevice) && (wcslen(lpwszDevice) != 0))  // First
        {
            if (!_wcsnicmp(lpwszDevice, L"\\\\.\\DISPLAY",11)) Display = TRUE;
            RtlInitUnicodeString(&Device, lpwszDevice);
        }
        else
        {
            if (lpwszDriver) // Second
            {
                if ((!_wcsnicmp(lpwszDriver, L"DISPLAY",7)) ||
                        (!_wcsnicmp(lpwszDriver, L"\\\\.\\DISPLAY",11))) Display = TRUE;
                RtlInitUnicodeString(&Device, lpwszDriver);
            }
        }
    }

    if (lpwszOutput) RtlInitUnicodeString(&Output, lpwszOutput);

    // Handle Print device or something else.
    if (!Display)
    {
        // WIP - GDI Print Commit coming in soon.
        DPRINT1("Not a DISPLAY device! %wZ\n", &Device);
        return NULL; // Return NULL until then.....
    }

    hdc = NtGdiOpenDCW((Default ? NULL : &Device),
                       (PDEVMODEW) lpInitData,
                       (lpwszOutput ? &Output : NULL),
                       iType,             // DCW 0 and ICW 1.
                       Display,
                       NULL,
                       &UMdhpdev );
 
    return hdc;
}

HDC
WINAPI
CreateDCWKmt (
    LPCWSTR lpwszDriver,
    LPCWSTR lpwszDevice,
    LPCWSTR lpwszOutput,
    CONST DEVMODEW *lpInitData)
{
    return IntCreateDICWLoc(lpwszDriver,
                         lpwszDevice,
                         lpwszOutput,
                         (PDEVMODEW)lpInitData,
                         0);
}



NTSTATUS
WINAPI
D3DKMTOpenAdapterFromGdiDisplayName(_Inout_ D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME* unnamedParam1)
{
    D3DKMT_OPENADAPTERFROMHDC OpenAdapterFromHdc;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    OpenAdapterFromHdc.hDc = CreateDCWKmt(unnamedParam1->DeviceName, unnamedParam1->DeviceName, 0, 0);
    if (OpenAdapterFromHdc.hDc)
    {
        Status = NtGdiDdDDIOpenAdapterFromHdc(&OpenAdapterFromHdc);
        if (Status == STATUS_PROCEDURE_NOT_FOUND)
        {
            /* Fallback to XDDM */
            Status = STATUS_SUCCESS;
        }
        else if (NT_SUCCESS(Status))
        {
          unnamedParam1->hAdapter = OpenAdapterFromHdc.hAdapter;
          unnamedParam1->AdapterLuid = OpenAdapterFromHdc.AdapterLuid;
          unnamedParam1->VidPnSourceId = OpenAdapterFromHdc.VidPnSourceId;
        }
        NtGdiDeleteObjectApp(OpenAdapterFromHdc.hDc);
    }
    return Status;
}

NTSTATUS
WINAPI
D3DKMTOpenAdapterFromLuid(_Inout_ CONST D3DKMT_OPENADAPTERFROMLUID* unnamedParam1)
{
    NTSTATUS Status = STATUS_SUCCESS;
    Status = NtGdiDdDDIOpenAdapterFromLuid(unnamedParam1);
    if (Status == STATUS_PROCEDURE_NOT_FOUND)
    {
        /* Fallback to XDDM */
        return STATUS_SUCCESS;
    }
    return Status;
}

NTSTATUS
WINAPI
D3DKMTQueryVideoMemoryInfo(
    _Inout_ D3DKMT_QUERYVIDEOMEMORYINFO *unnamedParam1)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    Status = NtGdiDdQueryVideoMemoryInfo(unnamedParam1);
    if (Status == STATUS_PROCEDURE_NOT_FOUND)
    {
        /* Fallback to XDDM */
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
    }

    return Status;
}
