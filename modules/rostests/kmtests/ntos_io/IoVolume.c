/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Kernel-Mode Test Suite Volume Device test
 * COPYRIGHT:   Copyright 2019 Thomas Faber (thomas.faber@reactos.org)
 */

#include <kmt_test.h>


static
void
TestIoVolumeDeviceToDosName(void)
{
    NTSTATUS Status;
    ULONG VolumeNumber;
    WCHAR VolumeDeviceNameBuffer[32];
    UNICODE_STRING VolumeDeviceName;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DosName;
    UNICODE_STRING DosVolumePrefix = RTL_CONSTANT_STRING(L"\\\\?\\Volume");

    RtlInitEmptyUnicodeString(&VolumeDeviceName,
                              VolumeDeviceNameBuffer,
                              sizeof(VolumeDeviceNameBuffer));
    VolumeNumber = 0;
    while (1)
    {
        VolumeNumber++;
        Status = RtlStringCbPrintfW(VolumeDeviceName.Buffer,
                                    VolumeDeviceName.MaximumLength,
                                    L"\\Device\\HarddiskVolume%lu",
                                    VolumeNumber);
        if (!NT_SUCCESS(Status))
        {
            trace("RtlStringCbPrintfW(0x%lx) failed with %lx\n",
                  VolumeNumber, Status);
            break;
        }

        RtlInitUnicodeString(&VolumeDeviceName, VolumeDeviceNameBuffer);
        Status = IoGetDeviceObjectPointer(&VolumeDeviceName,
                                          READ_CONTROL,
                                          &FileObject,
                                          &DeviceObject);
        if (!NT_SUCCESS(Status))
        {
            trace("IoGetDeviceObjectPointer(%wZ) failed with %lx\n",
                  &VolumeDeviceName, Status);
            continue;
        }

        Status = IoVolumeDeviceToDosName(DeviceObject, &DosName);
        ok_eq_hex(Status, STATUS_SUCCESS);
        if (!skip(NT_SUCCESS(Status), "No DOS name\n"))
        {
            trace("DOS name for %wZ is %wZ\n", &VolumeDeviceName, &DosName);
            if (DosName.Length == 2 * sizeof(WCHAR))
            {
                ok(DosName.Buffer[0] >= L'A' &&
                   DosName.Buffer[0] <= L'Z' &&
                   DosName.Buffer[1] == L':',
                   "Unexpected drive letter: %wZ\n", &DosName);
            }
            else
            {
                ok(RtlPrefixUnicodeString(&DosVolumePrefix, &DosName, FALSE),
                   "Unexpected volume path: %wZ\n", &DosName);
            }
            RtlFreeUnicodeString(&DosName);
        }
        ObDereferenceObject(FileObject);
    }
    ok(VolumeNumber > 1, "No volumes found\n");
}

START_TEST(IoVolume)
{
    TestIoVolumeDeviceToDosName();
}
