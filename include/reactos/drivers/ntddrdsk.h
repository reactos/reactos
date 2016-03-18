/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            include/reactos/drivers/ntddrdsk.h
 * PURPOSE:         Constants and types for accessing the RAM disk device
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */
#ifndef _NTDDRDSK_H_
#define _NTDDRDSK_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus1
extern "C" {
#endif

//
// This guid identifies a RAM disk volume
//
DEFINE_GUID(RAMDISK_BOOTDISK_GUID, 0xd9b257fc, 0x684e, 0x4dcb, 0x79, 0xab, 0xf6, 0xa2, 0xcf, 0x03, 0x50, 0xb7);

//
// This guid identifies a RAM disk bus
//
DEFINE_GUID(GUID_BUS_TYPE_RAMDISK, 0x9d6d66a6, 0x0b0c, 0x4563, 0x90, 0x77, 0xa0, 0xe9, 0xa7, 0x95, 0x5a, 0xe4);

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//
#define DD_RAMDISK_DEVICE_NAME            "\\Device\\Ramdisk"

//
// IoControlCode values for ramdisk devices.
//
#define IOCTL_RAMDISK_BASE                FILE_DEVICE_VIRTUAL_DISK
#define FSCTL_CREATE_RAM_DISK             CTL_CODE(FILE_DEVICE_VIRTUAL_DISK, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Disk Types
//
#define RAMDISK_REGISTRY_DISK             1 // Loaded from the registry
#define RAMDISK_MEMORY_MAPPED_DISK        2 // Loaded from the registry
#define RAMDISK_BOOT_DISK                 3 // Used as a boot device
#define RAMDISK_WIM_DISK                  4 // Used as an installation device

//
// Options when creating a ramdisk
//
typedef struct _RAMDISK_CREATE_OPTIONS
{
    ULONG Readonly:1;
    ULONG Fixed:1;
    ULONG NoDriveLetter:1;
    ULONG NoDosDevice:1;
    ULONG Hidden:1;
    ULONG ExportAsCd:1;
} RAMDISK_CREATE_OPTIONS;

//
// This structure is passed in for a FSCTL_CREATE_RAM_DISK call
//
typedef struct _RAMDISK_CREATE_INPUT
{
    ULONG Version;
    GUID DiskGuid;
    ULONG DiskType;
    RAMDISK_CREATE_OPTIONS Options;
    LARGE_INTEGER DiskLength;
    LONG DiskOffset;
    union
    {
        struct
        {
            ULONG ViewCount;
            SIZE_T ViewLength;
            WCHAR FileName[ANYSIZE_ARRAY];
        };
        struct
        {
            ULONG_PTR BasePage;
            WCHAR DriveLetter;
        };
        PVOID BaseAddress;
    };
} RAMDISK_CREATE_INPUT, *PRAMDISK_CREATE_INPUT;

#ifdef __cplusplus
}
#endif

#endif // _NTDDRDSK_H_
