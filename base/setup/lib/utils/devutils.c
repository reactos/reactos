/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device utility functions
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"
#include "devutils.h"

/* FUNCTIONS *****************************************************************/

/**
 * @brief
 * Open an existing device given by its NT-style path, which is assumed to be
 * for a disk device or a partition. The open is for synchronous I/O access.
 *
 * @param[in]   DevicePath
 * Supplies the NT-style path to the device to open.
 *
 * @param[out]  DeviceHandle
 * If successful, receives the NT handle of the opened device.
 * Once the handle is no longer in use, call NtClose() to close it.
 *
 * @param[in]   DesiredAccess
 * An ACCESS_MASK value combination that determines the requested access
 * to the device. Because the open is for synchronous access, SYNCHRONIZE
 * is automatically added to the access mask.
 *
 * @param[in]   ShareAccess
 * Specifies the type of share access for the device.
 *
 * @return  An NTSTATUS code indicating success or failure.
 **/
NTSTATUS
pOpenDeviceEx(
    _In_ PCWSTR DevicePath,
    _Out_ PHANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareAccess)
{
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    RtlInitUnicodeString(&Name, DevicePath);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    return NtOpenFile(DeviceHandle,
                      DesiredAccess | SYNCHRONIZE,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      ShareAccess,
                      /* FILE_NON_DIRECTORY_FILE | */
                      FILE_SYNCHRONOUS_IO_NONALERT);
}

/**
 * @brief
 * Open an existing device given by its NT-style path, which is assumed to be
 * for a disk device or a partition. The open is share read/write/delete, for
 * synchronous I/O and read access.
 *
 * @param[in]   DevicePath
 * @param[out]  DeviceHandle
 * See the DevicePath and DeviceHandle parameters of pOpenDeviceEx().
 *
 * @return  An NTSTATUS code indicating success or failure.
 *
 * @see pOpenDeviceEx()
 **/
NTSTATUS
pOpenDevice(
    _In_ PCWSTR DevicePath,
    _Out_ PHANDLE DeviceHandle)
{
    return pOpenDeviceEx(DevicePath,
                         DeviceHandle,
                         FILE_READ_DATA | FILE_READ_ATTRIBUTES,
                         FILE_SHARE_VALID_FLAGS // FILE_SHARE_READ,WRITE,DELETE
                         );
}

/* EOF */
