/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Utility functions
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"

LPCSTR wine_dbgstr_us(const UNICODE_STRING *us)
{
    if (!us) return "(null)";
    return wine_dbgstr_wn(us->Buffer, us->Length / sizeof(WCHAR));
}

/**
 * @brief
 * Retrieves a handle to the MountMgr controlling device.
 * The handle should be closed with NtClose() once it is no longer in use.
 **/
HANDLE
GetMountMgrHandle(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING MountMgrDevice;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE MountMgrHandle = NULL;

    RtlInitUnicodeString(&MountMgrDevice, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(&ObjectAttributes,
                               &MountMgrDevice,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&MountMgrHandle,
                        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        winetest_print("NtOpenFile(%s) failed, Status 0x%08lx\n",
                       wine_dbgstr_us(&MountMgrDevice), Status);
    }

    return MountMgrHandle;
}
