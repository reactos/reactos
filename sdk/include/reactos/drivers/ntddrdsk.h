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

#ifdef __cplusplus
extern "C" {
#endif

//
// This guid identifies a RAM disk volume (RamdiskBootDiskGuid)
// {D9B257FC-684E-4DCB-AB79-03CFA2F6B750}
//
DEFINE_GUID(RAMDISK_BOOTDISK_GUID, 0xD9B257FC, 0x684E, 0x4DCB, 0xAB, 0x79, 0x03, 0xCF, 0xA2, 0xF6, 0xB7, 0x50);

//
// This guid identifies a RAM disk bus
// {9D6D66A6-0B0C-4563-9077-A0E9A7955AE4}
//
DEFINE_GUID(GUID_BUS_TYPE_RAMDISK, 0x9D6D66A6, 0x0B0C, 0x4563, 0x90, 0x77, 0xA0, 0xE9, 0xA7, 0x95, 0x5A, 0xE4);

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
// Note:  For devices that support multiple units, it should be suffixed
//        with the Ascii representation of the unit number.
//
#define DD_RAMDISK_DEVICE_NAME              "\\Device\\Ramdisk"
#define DD_RAMDISK_DEVICE_NAME_U           L"\\Device\\Ramdisk"

//
// IoControlCode values for ramdisk devices.
//
#define IOCTL_RAMDISK_BASE                  FILE_DEVICE_VIRTUAL_DISK
#define FSCTL_CREATE_RAM_DISK               CTL_CODE(FILE_DEVICE_VIRTUAL_DISK, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Disk Types
//
#define RAMDISK_REGISTRY_DISK               1 // Loaded from the registry
#define RAMDISK_MEMORY_MAPPED_DISK          2 // Loaded from a file and mapped in memory
#define RAMDISK_BOOT_DISK                   3 // Used as a boot device "ramdisk(0)"
#define RAMDISK_WIM_DISK                    4 // Used as an installation device

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
