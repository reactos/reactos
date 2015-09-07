/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Manager
 * FILE:            boot/environ/app/bootmgr.c
 * PURPOSE:         Boot Manager Entrypoint
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bootmgr.h"

/* DATA VARIABLES ************************************************************/

#include <initguid.h>
DEFINE_GUID(GUID_WINDOWS_BOOTMGR,
            0x9DEA862C,
            0x5CDD,
            0x4E70,
            0xAC, 0xC1, 0xF3, 0x2B, 0x34, 0x4D, 0x47, 0x95);

ULONGLONG ApplicationStartTime;
ULONGLONG PostTime;
GUID BmApplicationIdentifier;

/* FUNCTIONS *****************************************************************/

PGUID
BlGetApplicationIdentifier (
    VOID
    )
{
    return NULL;
}

PWCHAR BootDirectory;

NTSTATUS
BmFwInitializeBootDirectoryPath()
{
#if 0
    PWCHAR FinalPath;
    NTSTATUS Status;
    PWCHAR BcdDirectory;
    UNICODE_STRING BcdPath;
    ULONG FinalSize, FileHandle, DeviceHandle;

    BcdPath.MaximumLength = 0;
    BcdPath.Buffer = NULL;

    FinalPath = NULL;

    FileHandle = -1;
    DeviceHandle = -1;

    Status = BlpDeviceOpen(BlpBootDevice, 1u, 0, &DeviceHandle);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    Status = BmpFwGetApplicationDirectoryPath(&BcdPath);
    BcdDirectory = BcdPath.Buffer;
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    FinalSize = BcdPath.MaximumLength + sizeof(L"\\BCD") - sizeof(UNICODE_NULL);
    if (FinalSize < BcdPath.MaximumLength)
    {
        goto Quickie;
    }

    FinalPath = BlMmAllocateHeap(FinalSize);
    if (!FinalPath)
    {
        goto Quickie;
    }

    RtlZeroMemory(FinalPath, FinalSize);
    RtlCopyMemory(FinalPath, BcdDirectory, BcdPath.MaximumLength);
    wcsncat(FinalPath, L"\\BCD", FinalSize / sizeof(WCHAR));

    EfiPrintf(L"Opening: %s\r\n", FinalPath);
    Status = BlFileOpen(DeviceHandle, FinalPath, 1u, &FileHandle);
    if (!NT_SUCCESS(Status))
    {
        BootDirectory = BcdDirectory;
        goto Quickie;
    }

    BootDirectory = L"\\EFI\\Microsoft\\Boot";

Quickie:
    if (BcdDirectory)
    {
        Status = BlMmFreeHeap(BcdDirectory);
    }
    if (FinalPath)
    {
        Status = BlMmFreeHeap(FinalPath);
    }
    if (FileHandle != -1)
    {
        Status = BlFileClose(FileHandle);
    }
    if (DeviceHandle != -1)
    {
        Status = BlDeviceClose(DeviceHandle);
    }
    return Status;
#else
    return STATUS_NOT_IMPLEMENTED;
#endif
}


/*++
 * @name BmMain
 *
 *     The BmMain function implements the Windows Boot Application entrypoint for
 *     the Boot Manager.
 *
 * @param  BootParameters
 *         Pointer to the Boot Application Parameter Block.
 *
 * @return NT_SUCCESS if the image was loaded correctly, relevant error code
 *         otherwise.
 *
 *--*/
NTSTATUS
BmMain (
    _In_ PBOOT_APPLICATION_PARAMETER_BLOCK BootParameters
    )
{
    NTSTATUS Status;
    BL_LIBRARY_PARAMETERS LibraryParameters;
    PBL_RETURN_ARGUMENTS ReturnArguments;
    BOOLEAN RebootOnError;
    PGUID AppIdentifier;
//    HANDLE BcdHandle;

    EfiPrintf(L"ReactOS UEFI Boot Manager Initializing...\n");

    /* Reading the BCD can change this later on */
    RebootOnError = FALSE;

    /* Save the start/end-of-POST time */
    ApplicationStartTime = __rdtsc();
    PostTime = ApplicationStartTime;

    /* Setup the boot library parameters for this application */
    BlSetupDefaultParameters(&LibraryParameters);
    LibraryParameters.TranslationType = BlNone;
    LibraryParameters.LibraryFlags = 0x400 | 0x8;
    LibraryParameters.MinimumAllocationCount = 16;
    LibraryParameters.MinimumHeapSize = 512 * 1024;

    /* Initialize the boot library */
    Status = BlInitializeLibrary(BootParameters, &LibraryParameters);
    if (!NT_SUCCESS(Status))
    {
        /* Check for failure due to invalid application entry */
        if (Status != STATUS_INVALID_PARAMETER_9)
        {
            /* Specifically print out what happened */
            EfiPrintf(L"BlInitializeLibrary failed 0x%x\r\n", Status);
        }

        /* Go to exit path */
        goto Quickie;
    }

    /* Get the application identifier */
    AppIdentifier = BlGetApplicationIdentifier();
    if (!AppIdentifier)
    {
        /* None was given, so set our default one */
        AppIdentifier = (PGUID)&GUID_WINDOWS_BOOTMGR;
    }
    
    /* Save our identifier */
    BmApplicationIdentifier = *AppIdentifier;

    /* Initialize the file system to open a handle to our root boot directory */
    BmFwInitializeBootDirectoryPath();

    //Status = BmOpenDataStore(&BcdHandle);

    EfiPrintf(L"We are A-OK!\n");
    EfiStall(10000000);

Quickie:
    /* Check if we should reboot */
    if ((RebootOnError) ||
        (BlpApplicationEntry.Flags & BL_APPLICATION_ENTRY_REBOOT_ON_ERROR))
    {
        /* Reboot the box */
        BlFwReboot();
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Return back to the caller with the error argument encoded */
        ReturnArguments = (PVOID)((ULONG_PTR)BootParameters + BootParameters->ReturnArgumentsOffset);
        ReturnArguments->Version = BL_RETURN_ARGUMENTS_VERSION;
        ReturnArguments->Status = Status;

        /* Tear down the boot library*/
        BlDestroyLibrary();
    }

    /* Return back status */
    return Status;
}

