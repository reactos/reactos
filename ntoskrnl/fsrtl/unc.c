/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/unc.c
 * PURPOSE:         Manages UNC support routines for file system drivers.
 * PROGRAMMERS:     None.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlDeregisterUncProvider
 * @unimplemented
 *
 * FILLME
 *
 * @param Handle
 *        FILLME
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
VOID
NTAPI
FsRtlDeregisterUncProvider(IN HANDLE Handle)
{
    UNICODE_STRING DosDevicesUNC = RTL_CONSTANT_STRING(L"\\DosDevices\\UNC");

    DPRINT("FsRtlDeregisterUncProvider: Handle=%p\n", Handle);
    //
    // Normal implementation should look like:
    // - notify mup.sys?
    // - at last deregistration, destroy \DosDevices\UNC symbolic link
    //

    ZwClose(Handle);

    IoDeleteSymbolicLink(&DosDevicesUNC);
}

/*++
 * @name FsRtlRegisterUncProvider
 * @unimplemented
 *
 * FILLME
 *
 * @param Handle
 *        FILLME
 *
 * @param RedirectorDeviceName
 *        FILLME
 *
 * @param MailslotsSupported
 *
 * @return None
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
FsRtlRegisterUncProvider(IN OUT PHANDLE Handle,
                         IN PUNICODE_STRING RedirectorDeviceName,
                         IN BOOLEAN MailslotsSupported)
{
    UNICODE_STRING DevNull = RTL_CONSTANT_STRING(L"\\Device\\Null");
    UNICODE_STRING DosDevicesUNC = RTL_CONSTANT_STRING(L"\\DosDevices\\UNC");
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    HANDLE FileHandle;
    NTSTATUS Status;

    DPRINT("FsRtlRegisterUncProvider: Redirector=%wZ MailslotsSupported=%d\n",
           RedirectorDeviceName, MailslotsSupported);

    //
    // Current implementation is a hack, as it only supports one UNC provider.
    // However, it doesn't require to have a functional mup.sys driver.
    //

    //
    // Normal implementation should look like:
    // - at registration 1, creates symlink \DosDevices\UNC to new provider;
    //   returns handle to \Device\Null
    // - at registration 2, load mup.sys, register both providers to mup.sys
    //   and change \DosDevices\UNC to DD_MUP_DEVICE_NAME;
    //   returns handle to new provider
    // - at next registrations, register provider to mup.sys;
    //   returns handle to new provider
    //

    *Handle = (HANDLE)-1;
    InitializeObjectAttributes(&ObjectAttributes,
                               &DevNull,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwCreateFile(&FileHandle,
                          GENERIC_WRITE,
                          &ObjectAttributes,
                          &Iosb,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          0,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to open %wZ\n", &DevNull);
        return Status;
    }

    Status = IoCreateSymbolicLink(&DosDevicesUNC, RedirectorDeviceName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to create symbolic link %wZ -> %wZ\n", &DosDevicesUNC, RedirectorDeviceName);
        DPRINT1("FIXME: multiple unc provider registered?\n");
        ZwClose(FileHandle);
        return Status;
    }

    *Handle = FileHandle;
    return STATUS_SUCCESS;
}
