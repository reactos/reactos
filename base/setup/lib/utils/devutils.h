/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device utility functions
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

/* Flags combination allowing all the read, write and delete share modes.
 * Currently similar to FILE_SHARE_VALID_FLAGS. */
#define FILE_SHARE_ALL \
    (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)

/* FUNCTIONS *****************************************************************/

NTSTATUS
pOpenDeviceEx_UStr(
    _In_ PCUNICODE_STRING DevicePath,
    _Out_ PHANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess);

NTSTATUS
pOpenDevice_UStr(
    _In_ PCUNICODE_STRING DevicePath,
    _Out_ PHANDLE DeviceHandle);

NTSTATUS
pOpenDeviceEx(
    _In_ PCWSTR DevicePath,
    _Out_ PHANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess);

NTSTATUS
pOpenDevice(
    _In_ PCWSTR DevicePath,
    _Out_ PHANDLE DeviceHandle);

/* EOF */
