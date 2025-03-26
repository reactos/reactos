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
GetMountMgrHandle(
    _In_ ACCESS_MASK DesiredAccess)
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
                        DesiredAccess | SYNCHRONIZE,
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

VOID
DumpBuffer(
    _In_ PVOID Buffer,
    _In_ ULONG Length)
{
#define LINE_SIZE   (75 + 2)
    ULONG i;
    PBYTE Ptr1, Ptr2;
    CHAR  LineBuffer[LINE_SIZE];
    PCHAR Line;
    ULONG LineSize;

    Ptr1 = Ptr2 = Buffer;
    while ((ULONG_PTR)Buffer + Length - (ULONG_PTR)Ptr1 > 0)
    {
        Ptr1 = Ptr2;
        Line = LineBuffer;

        /* Print the address */
        Line += _snprintf(Line, LINE_SIZE + LineBuffer - Line, "%08Ix ", (ULONG_PTR)Ptr1);

        /* Print up to 16 bytes... */

        /* ... in hexadecimal form first... */
        i = 0;
        while (i++ <= 0x0F && ((ULONG_PTR)Buffer + Length - (ULONG_PTR)Ptr1 > 0))
        {
            Line += _snprintf(Line, LINE_SIZE + LineBuffer - Line, " %02x", *Ptr1);
            ++Ptr1;
        }

        /* ... align with spaces if needed... */
        RtlFillMemory(Line, (0x0F + 2 - i) * 3 + 2, ' ');
        Line += (0x0F + 2 - i) * 3 + 2;

        /* ... then in character form. */
        i = 0;
        while (i++ <= 0x0F && ((ULONG_PTR)Buffer + Length - (ULONG_PTR)Ptr2 > 0))
        {
            *Line++ = ((*Ptr2 >= 0x20 && *Ptr2 <= 0x7E) || (*Ptr2 >= 0x80 && *Ptr2 < 0xFF) ? *Ptr2 : '.');
            ++Ptr2;
        }

        /* Newline */
        *Line++ = '\r';
        *Line++ = '\n';

        /* Finally display the line */
        LineSize = Line - LineBuffer;
        printf("%.*s", (int)LineSize, LineBuffer);
    }
}
