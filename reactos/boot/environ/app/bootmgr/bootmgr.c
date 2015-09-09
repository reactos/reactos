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

DEFINE_GUID(GUID_WINDOWS_BOOTMGR,
            0x9DEA862C,
            0x5CDD,
            0x4E70,
            0xAC, 0xC1, 0xF3, 0x2B, 0x34, 0x4D, 0x47, 0x95);

ULONGLONG ApplicationStartTime;
ULONGLONG PostTime;
GUID BmApplicationIdentifier;
PWCHAR BootDirectory;

/* FUNCTIONS *****************************************************************/

NTSTATUS
BmpFwGetApplicationDirectoryPath (
    _In_ PUNICODE_STRING ApplicationDirectoryPath
    )
{
    NTSTATUS Status;
    ULONG i, AppPathLength;
    PWCHAR ApplicationPath, PathCopy;

    /* Clear the incoming string */
    ApplicationDirectoryPath->Length = 0;
    ApplicationDirectoryPath->MaximumLength = 0;
    ApplicationDirectoryPath->Buffer = 0;

    /* Get the boot application path */
    ApplicationPath = NULL;
    Status = BlGetBootOptionString(BlpApplicationEntry.BcdData,
                                   BcdLibraryString_ApplicationPath,
                                   &ApplicationPath);
    if (NT_SUCCESS(Status))
    {
        /* Calculate the length of the application path */
        for (i = wcslen(ApplicationPath) - 1; i > 0; i--)
        {
            /* Keep going until the path separator */
            if (ApplicationPath[i] == OBJ_NAME_PATH_SEPARATOR)
            {
                break;
            }
        }

        /* Check if we have space for one more character */
        AppPathLength = i + 1;
        if (AppPathLength < i)
        {
            /* Nope, we'll overflow */
            AppPathLength = -1;
            Status = STATUS_INTEGER_OVERFLOW;
        }
        else
        {
            /* Go ahead */
            Status = STATUS_SUCCESS;
        }

        /* No overflow? */
        if (NT_SUCCESS(Status))
        {
            /* Check if it's safe to multiply by two */
            if ((AppPathLength * sizeof(WCHAR)) > 0xFFFFFFFF)
            {
                /* Nope */
                AppPathLength = -1;
                Status = STATUS_INTEGER_OVERFLOW;
            }
            else
            {
                /* We're good, do the multiplication */
                Status = STATUS_SUCCESS;
                AppPathLength *= sizeof(WCHAR);
            }

            /* Allocate a copy for the string */
            if (NT_SUCCESS(Status))
            {
                PathCopy = BlMmAllocateHeap(AppPathLength);
                if (PathCopy)
                {
                    /* NULL-terminate it */
                    RtlCopyMemory(PathCopy,
                                  ApplicationPath,
                                  AppPathLength - sizeof(UNICODE_NULL));
                    PathCopy[AppPathLength] = UNICODE_NULL;

                    /* Finally, initialize the outoing string */
                    RtlInitUnicodeString(ApplicationDirectoryPath, PathCopy);
                }
                else
                {
                    /* No memory, fail */
                    Status = STATUS_NO_MEMORY;
                }
            }
        }
    }

    /* Check if we had an application path */
    if (ApplicationPath)
    {
        /* No longer need this, free it */
        BlMmFreeHeap(ApplicationPath);
    }

    /* All done! */
    return Status;

}
NTSTATUS
BmFwInitializeBootDirectoryPath (
    VOID
    )
{
    PWCHAR FinalPath;
    NTSTATUS Status;
    PWCHAR BcdDirectory;
    UNICODE_STRING BcdPath;
    ULONG FinalSize;
    ULONG FileHandle, DeviceHandle;

    /* Initialize everything for failure */
    BcdPath.MaximumLength = 0;
    BcdPath.Buffer = NULL;
    BcdDirectory = NULL;
    FinalPath = NULL;
    FileHandle = -1;
    DeviceHandle = -1;

    /* Try to open the boot device */
    Status = BlpDeviceOpen(BlpBootDevice, 1u, 0, &DeviceHandle);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Device open failed: %lx\r\n", Status);
        goto Quickie;
    }

    /* Get the directory path */
    Status = BmpFwGetApplicationDirectoryPath(&BcdPath);
    BcdDirectory = BcdPath.Buffer;
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"path failed: %lx\n", Status);
        goto Quickie;
    }

    /* Add the BCD file name to it */
    FinalSize = BcdPath.MaximumLength + sizeof(L"\\BCD") - sizeof(UNICODE_NULL);
    if (FinalSize < BcdPath.MaximumLength)
    {
        goto Quickie;
    }

    /* Allocate space for the final path */
    FinalPath = BlMmAllocateHeap(FinalSize);
    if (!FinalPath)
    {
        goto Quickie;
    }

    /* Build it */
    RtlZeroMemory(FinalPath, FinalSize);
    RtlCopyMemory(FinalPath, BcdDirectory, BcdPath.MaximumLength);
    wcsncat(FinalPath, L"\\BCD", FinalSize / sizeof(WCHAR));

    /* Try to open the file */
    EfiPrintf(L"Opening: %s\r\n", FinalPath);
#if 0
    Status = BlFileOpen(DeviceHandle, FinalPath, 1u, &FileHandle);
    if (!NT_SUCCESS(Status))
    {
        BootDirectory = BcdDirectory;
        goto Quickie;
    }

    BootDirectory = L"\\EFI\\Microsoft\\Boot";
#endif

Quickie:
    /* Free all the allocations we made */
    if (BcdDirectory)
    {
        Status = BlMmFreeHeap(BcdDirectory);
    }
    if (FinalPath)
    {
        Status = BlMmFreeHeap(FinalPath);
    }

    /* Close the BCD file */
    if (FileHandle != -1)
    {
        //Status = BlFileClose(FileHandle);
    }

    /* Close the boot device */
    if (DeviceHandle != -1)
    {
        Status = BlDeviceClose(DeviceHandle);
    }

    /* Return back to the caller */
    return Status;
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

    EfiPrintf(L"We are A-OK!\r\n");
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

