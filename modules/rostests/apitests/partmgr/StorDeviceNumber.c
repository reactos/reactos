/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for IOCTL_STORAGE_GET_DEVICE_NUMBER
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"
#include <ntddstor.h>

static LPCSTR wine_dbgstr_us(const UNICODE_STRING *us)
{
    if (!us) return "(null)";
    return wine_dbgstr_wn(us->Buffer, us->Length / sizeof(WCHAR));
}

/* Flags combination allowing all the read, write and delete share modes.
 * Currently similar to FILE_SHARE_VALID_FLAGS. */
#define FILE_SHARE_ALL \
    (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)

static BOOLEAN
Test_Device_StorDeviceNumber(
    _In_ PCWSTR NtDeviceName)
{
    BOOLEAN Success = FALSE; // Suppose failure.
    NTSTATUS Status;
    HANDLE DeviceHandle1 = NULL, DeviceHandle2 = NULL;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    STORAGE_DEVICE_NUMBER DeviceNumber;
    UNICODE_STRING DeviceName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR NtLegacyDeviceName[MAX_PATH];

    ULONG BufferSize;
    struct { OBJECT_NAME_INFORMATION; WCHAR Buffer[MAX_PATH]; } DeviceName1Buffer;
    PUNICODE_STRING DeviceName1 = &DeviceName1Buffer.Name;
    struct { OBJECT_NAME_INFORMATION; WCHAR Buffer[MAX_PATH]; } DeviceName2Buffer;
    PUNICODE_STRING DeviceName2 = &DeviceName2Buffer.Name;

    /* Open a handle to the device */
    RtlInitUnicodeString(&DeviceName, NtDeviceName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&DeviceHandle1,
                        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_ALL,
                        /* FILE_NON_DIRECTORY_FILE | */ FILE_SYNCHRONOUS_IO_NONALERT);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Device '%s': Opening failed\n", wine_dbgstr_us(&DeviceName));
        goto Quit;
    }

    /* Verify the device information before proceeding further */
    Status = NtQueryVolumeInformationFile(DeviceHandle1,
                                          &IoStatusBlock,
                                          &DeviceInfo,
                                          sizeof(DeviceInfo),
                                          FileFsDeviceInformation);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("FileFsDeviceInformation('%s') failed, Status 0x%08lx\n",
             wine_dbgstr_us(&DeviceName), Status);
        goto Quit;
    }

    /* Ignore volumes that are NOT on usual disks */
    switch (DeviceInfo.DeviceType)
    {
    /* Testable devices */
    case FILE_DEVICE_CD_ROM:
    // case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
    case FILE_DEVICE_DISK:
    // case FILE_DEVICE_DISK_FILE_SYSTEM:
    // case FILE_DEVICE_NETWORK:
    // case FILE_DEVICE_NETWORK_FILE_SYSTEM:
    case FILE_DEVICE_VIRTUAL_DISK:
        break;

    /* Untestable devices */
    default:
        skip("Device '%s': Cannot test, device type %lu\n",
             wine_dbgstr_us(&DeviceName), DeviceInfo.DeviceType);
        goto Quit;
    }
#if 0
    if (DeviceInfo.DeviceType != FILE_DEVICE_DISK &&
        DeviceInfo.DeviceType != FILE_DEVICE_VIRTUAL_DISK &&
        DeviceInfo.DeviceType != FILE_DEVICE_CD_ROM)
    {
        skip("Device '%s': Cannot test, device type %lu\n",
             wine_dbgstr_us(&DeviceName), DeviceInfo.DeviceType);
        goto Quit;
    }
#endif

    /*
     * Retrieve the storage device number.
     * Note that this call is unsupported if this is a dynamic volume.
     * NOTE: Usually fails for floppy disks.
     */
    Status = NtDeviceIoControlFile(DeviceHandle1,
                                   NULL, NULL, NULL,
                                   &IoStatusBlock,
                                   IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                   NULL, 0,
                                   &DeviceNumber, sizeof(DeviceNumber));
    if (!NT_SUCCESS(Status))
    {
        skip("Device '%s': Couldn't retrieve disk number\n", wine_dbgstr_us(&DeviceName));
        goto Quit;
    }
    ok(DeviceNumber.DeviceType == DeviceInfo.DeviceType,
       "Device '%s': Device type mismatch\n", wine_dbgstr_us(&DeviceName));

    /* NOTE: this value is set to 0xFFFFFFFF (-1) for the disks that
     * represent the physical paths of a multipath I/O (MPIO) disk. */
    ok(DeviceNumber.DeviceNumber != ULONG_MAX,
       "Device '%s': Invalid disk number reported\n", wine_dbgstr_us(&DeviceName));
    if (DeviceNumber.DeviceNumber == ULONG_MAX)
        goto Quit;

    switch (DeviceInfo.DeviceType)
    {
    /* Testable devices */
    case FILE_DEVICE_CD_ROM:
    // case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
    {
        /* CD-ROMs don't have partitions, their partition number is -1 */
        ok(DeviceNumber.PartitionNumber == ULONG_MAX,
           "Device '%s': Invalid partition number (%lu) reported, expected ULONG_MAX (-1)\n",
           wine_dbgstr_us(&DeviceName), DeviceNumber.PartitionNumber);

        /* Map to an NT device name */
        RtlStringCchPrintfW(NtLegacyDeviceName, _countof(NtLegacyDeviceName),
                            L"\\Device\\CdRom%lu",
                            DeviceNumber.DeviceNumber);
        break;
    }

    case FILE_DEVICE_DISK:
    // case FILE_DEVICE_DISK_FILE_SYSTEM:
    case FILE_DEVICE_VIRTUAL_DISK:
    {
        /* Check whether this is a floppy or a partitionable device */
        if (DeviceInfo.Characteristics & FILE_FLOPPY_DISKETTE)
        {
            /* Floppies don't have partitions, their partition number is -1 */
            ok(DeviceNumber.PartitionNumber == ULONG_MAX,
               "Device '%s': Invalid partition number (%lu) reported, expected ULONG_MAX (-1)\n",
               wine_dbgstr_us(&DeviceName), DeviceNumber.PartitionNumber);

            /* Map to an NT device name */
            RtlStringCchPrintfW(NtLegacyDeviceName, _countof(NtLegacyDeviceName),
                                L"\\Device\\Floppy%lu",
                                DeviceNumber.DeviceNumber);
        }
        else
        {
            /* The device is partitionable, so it must have a valid partition number */
            ok(DeviceNumber.PartitionNumber != ULONG_MAX,
               "Device '%s': Invalid partition number (%lu) reported; unpartitionable device?\n",
               wine_dbgstr_us(&DeviceName), DeviceNumber.PartitionNumber);
            if (DeviceNumber.PartitionNumber == ULONG_MAX)
                goto Quit;

            /* Map to an NT device name */
            RtlStringCchPrintfW(NtLegacyDeviceName, _countof(NtLegacyDeviceName),
                                L"\\Device\\Harddisk%lu\\Partition%lu",
                                DeviceNumber.DeviceNumber, DeviceNumber.PartitionNumber);
        }
        break;
    }

    /* Untestable devices */
    default:
        skip("Device '%s': Cannot test, device type %lu\n",
             wine_dbgstr_us(&DeviceName), DeviceInfo.DeviceType);
        goto Quit;
    }

    /* Open the device using the legacy path */
    RtlInitUnicodeString(&DeviceName, NtLegacyDeviceName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&DeviceHandle2,
                        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_ALL,
                        /* FILE_NON_DIRECTORY_FILE | */ FILE_SYNCHRONOUS_IO_NONALERT);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Device '%s': Opening failed\n", wine_dbgstr_us(&DeviceName));
        goto Quit;
    }

    /*
     * Verify whether both retrieved handles refer to the same device.
     * Since we're not running on Windows 10, we cannot use kernel32!CompareObjectHandles()
     * or ntdll!NtCompareObjects(), therefore we have to rely on comparing
     * whether the devices referred by both handles have the same canonical name.
     */
    Status = NtQueryObject(DeviceHandle1,
                           ObjectNameInformation,
                           &DeviceName1Buffer,
                           sizeof(DeviceName1Buffer),
                           &BufferSize);
    ok_ntstatus(Status, STATUS_SUCCESS);

    Status = NtQueryObject(DeviceHandle2,
                           ObjectNameInformation,
                           &DeviceName2Buffer,
                           sizeof(DeviceName2Buffer),
                           &BufferSize);
    ok_ntstatus(Status, STATUS_SUCCESS);

    Success = RtlEqualUnicodeString(DeviceName1, DeviceName2, FALSE);
    ok(Success, "Devices '%s' and '%s' are not the same!\n",
       wine_dbgstr_us(DeviceName1), wine_dbgstr_us(DeviceName2));

Quit:
    /* Cleanup */
    if (DeviceHandle2)
        NtClose(DeviceHandle2);
    if (DeviceHandle1)
        NtClose(DeviceHandle1);

    return Success;
}

static BOOLEAN
Test_Drive_StorDeviceNumber(
    _In_ WCHAR Drive)
{
    WCHAR NtDeviceName[] = L"\\DosDevices\\?:";
    NtDeviceName[sizeof("\\DosDevices\\")-1] = Drive;
    return Test_Device_StorDeviceNumber(NtDeviceName);
}

START_TEST(StorDeviceNumber)
{
    DWORD Drives;
    UCHAR i;

    /* Enumerate existing testable drives */
    Drives = GetLogicalDrives();
    if (Drives == 0)
    {
        skip("Drives map unavailable, error 0x%lx\n", GetLastError());
        goto otherTests;
    }
    for (i = 0; i <= 'Z'-'A'; ++i)
    {
        WCHAR DriveName[] = L"?:\\";
        UINT DriveType;

        /* Skip non-existing drives */
        if (!(Drives & (1 << i)))
            continue;

        /* Retrieve the drive type and see whether we can test it */
        DriveName[0] = L'A' + i;
        DriveType = GetDriveTypeW(DriveName);

        switch (DriveType)
        {
        case DRIVE_REMOVABLE:
        case DRIVE_FIXED:
        case DRIVE_CDROM:
        case DRIVE_RAMDISK:
        {
            Test_Drive_StorDeviceNumber(L'A' + i);
            break;
        }

        case DRIVE_UNKNOWN:
        case DRIVE_NO_ROOT_DIR:
        case DRIVE_REMOTE:
        default:
            /* Unhandled drive type, just skip it silently */
            trace("Drive %c with unhandled type %u\n", 'A' + i, DriveType);
            break;
        }
    }

otherTests:
    /* Test the drive containing SystemRoot */
    Test_Drive_StorDeviceNumber(SharedUserData->NtSystemRoot[0]);

    /* Test \??\PhysicalDrive0, if it exists */
    Test_Device_StorDeviceNumber(L"\\??\\PhysicalDrive0");
}
