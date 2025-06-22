/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     D3DKMT dxgkrnl syscalls
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#include <gdi32_vista.h>
#include <d3dkmddi.h>
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
    }
    return Status;
}

/* Not just a syscall even in wine. */
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
    /* fallback here is perfectly fine! */
    return 1;
}
