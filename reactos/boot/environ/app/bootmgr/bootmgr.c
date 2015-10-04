/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Manager
 * FILE:            boot/environ/app/bootmgr/bootmgr.c
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

BL_BOOT_ERROR BmpErrorBuffer;
PBL_BOOT_ERROR BmpInternalBootError;
BL_PACKED_BOOT_ERROR BmpPackedBootError;

BOOLEAN BmBootIniUsed;
WCHAR BmpFileNameBuffer[128];
PWCHAR ParentFileName = L"";

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
    Status = BlFileOpen(DeviceHandle, FinalPath, 1, &FileHandle);
    if (!NT_SUCCESS(Status))
    {
        BootDirectory = BcdDirectory;
        goto Quickie;
    }

    /* Save the boot directory */
    BootDirectory = L"\\EFI\\Microsoft\\Boot";

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
        Status = BlFileClose(FileHandle);
    }

    /* Close the boot device */
    if (DeviceHandle != -1)
    {
        Status = BlDeviceClose(DeviceHandle);
    }

    /* Return back to the caller */
    return Status;
}

NTSTATUS
BmOpenBootIni (
    VOID
    )
{
    /* Don't yet handled boot.ini */
    return STATUS_NOT_FOUND;
}

ULONG
BmpFatalErrorMessageFilter (
    _In_ NTSTATUS ErrorStatus,
    _Out_ PULONG ErrorResourceId
    )
{
    ULONG Result;

    /* Assume no message for now, check for known status message */
    Result = 0;
    switch (ErrorStatus)
    {
        /* Convert each status to a resource ID */
        case STATUS_UNEXPECTED_IO_ERROR:
            *ErrorResourceId = 9017;
            Result = 1;
            break;
        case STATUS_IMAGE_CHECKSUM_MISMATCH:
            *ErrorResourceId = 9018;
            break;
        case STATUS_INVALID_IMAGE_WIN_64:
            *ErrorResourceId = 9016;
            break;
        case 0xC0000428:
            *ErrorResourceId = 9019;
            Result = 2;
            break;
        case 0xC0210000:
            *ErrorResourceId = 9013;
            break;
    }

    /* Return the type of message */
    return Result;
}

VOID
BmErrorPurge (
    VOID
    )
{
    /* Check if a boot error is present */
    if (BmpPackedBootError.BootError)
    {
        /* Purge it */
        BlMmFreeHeap(BmpPackedBootError.BootError);
        BmpPackedBootError.BootError = NULL;
    }

    /* Zero out the packed buffer */
    BmpPackedBootError.Size = 0;
    BmpInternalBootError = NULL;
    RtlZeroMemory(&BmpErrorBuffer, sizeof(BmpErrorBuffer));
}

VOID
BmpErrorLog (
    _In_ ULONG ErrorCode,
    _In_ NTSTATUS ErrorStatus,
    _In_ ULONG ErrorMsgId,
    _In_ PWCHAR FileName,
    _In_ ULONG HelpMsgId
    )
{
    PWCHAR ErrorMsgString;

    /* Check if we already had an error */
    if (BmpInternalBootError)
    {
        /* Purge it */
        BmErrorPurge();
    }

    /* Find the string for this error ID */
    ErrorMsgString = BlResourceFindMessage(ErrorMsgId);
    if (ErrorMsgString)
    {
        /* Fill out the error buffer */
        BmpErrorBuffer.Unknown1 = 0;
        BmpErrorBuffer.Unknown2 = 0;
        BmpErrorBuffer.ErrorString = ErrorMsgString;
        BmpErrorBuffer.FileName = FileName;
        BmpErrorBuffer.ErrorCode = ErrorCode;
        BmpErrorBuffer.ErrorStatus = ErrorStatus;
        BmpErrorBuffer.HelpMsgId = HelpMsgId;
        BmpInternalBootError = &BmpErrorBuffer;
    }
}

VOID
BmFatalErrorEx (
    _In_ ULONG ErrorCode,
    _In_ ULONG_PTR Parameter1,
    _In_ ULONG_PTR Parameter2,
    _In_ ULONG_PTR Parameter3,
    _In_ ULONG_PTR Parameter4
    )
{
    PWCHAR FileName, Buffer;
    NTSTATUS ErrorStatus;
    WCHAR FormatString[256];
    ULONG ErrorResourceId, ErrorHelpId;
    BOOLEAN Restart, NoError;

    /* Assume no buffer for now */
    Buffer = NULL;

    /* Check what error code is being raised */
    switch (ErrorCode)
    {
        /* Error reading the BCD */
        case BL_FATAL_ERROR_BCD_READ:

            /* Check if we have a name for the BCD file */
            if (Parameter1)
            {
                /* Check if the name fits into our buffer */
                FileName = (PWCHAR)Parameter1;
                if (wcslen(FileName) < sizeof(BmpFileNameBuffer))
                {
                    /* Copy it in there */
                    Buffer = BmpFileNameBuffer;
                    wcsncpy(BmpFileNameBuffer,
                            FileName,
                            RTL_NUMBER_OF(BmpFileNameBuffer));
                }
            }

            /* If we don't have a buffer, use an empty one */
            if (!Buffer)
            {
                Buffer = ParentFileName;
            }

            /* The NTSTATUS code is in parameter 2*/
            ErrorStatus = (NTSTATUS)Parameter2;

            /* Build the error string */
            swprintf(FormatString,
                     L"\nAn error occurred (%08x) while attempting"
                     L"to read the boot configuration data file %s\n",
                     ErrorStatus,
                     Buffer);

            /* Select the resource ID message */
            ErrorResourceId = 9002;
            break;

        default:

            /* The rest is not yet handled */
            EfiPrintf(L"Unexpected fatal error: %lx\n", ErrorCode);
            while (1);
            break;
    }

    /* Check if the BCD option for restart is set */
    BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                           BcdLibraryBoolean_RestartOnFailure,
                           &Restart);
    if (Restart)
    {
        /* Yes, so no error should be shown since we'll auto-restart */
        NoError = TRUE;
    }
    else
    {
        /* Check if the option for not showing errors is set in the BCD */
        BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                               BcdBootMgrBoolean_NoErrorDisplay,
                               &NoError);
    }

    /* Do we want an error? */
    if (!NoError)
    {
        /* Yep, print it and then raise an error */
        BlStatusPrint(FormatString);
        BlStatusError(1, ErrorCode, Parameter1, Parameter2, Parameter3);
    }

    /* Get the help message ID */
    ErrorHelpId = BmpFatalErrorMessageFilter(ErrorStatus, &ErrorResourceId);
    BmpErrorLog(ErrorCode, ErrorStatus, ErrorResourceId, Buffer, ErrorHelpId);
}

NTSTATUS
BmpFwGetFullPath (
    _In_ PWCHAR FileName,
    _Out_ PWCHAR* FullPath
    )
{
    NTSTATUS Status;
    ULONG BootDirLength, BootDirLengthWithNul;
    ULONG PathLength, FullPathLength;

    /* Compute the length of the directory, and add a NUL */
    BootDirLength = wcslen(BootDirectory);
    BootDirLengthWithNul = BootDirLength + 1;
    if (BootDirLengthWithNul < BootDirLength)
    {
        /* This would overflow */
        BootDirLengthWithNul = -1;
        Status = STATUS_INTEGER_OVERFLOW;
    }
    else
    {
        /* We have space */
        Status = STATUS_SUCCESS;
    }

    /* Fail on overflow */
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Add the length of the file, make sure it fits */
    PathLength = wcslen(FileName);
    FullPathLength = PathLength + BootDirLength;
    if (FullPathLength < PathLength)
    {
        /* Nope */
        FullPathLength = -1;
        Status = STATUS_INTEGER_OVERFLOW;
    }
    else
    {
        /* All good */
        Status = STATUS_SUCCESS;
    }

    /* Fail on overflow */
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Allocate the full path */
    FullPathLength = FullPathLength * sizeof(WCHAR);
    *FullPath = BlMmAllocateHeap(FullPathLength);
    if (*FullPath)
    {
        /* Copy the directory followed by the file name */
        wcsncpy(*FullPath, BootDirectory, FullPathLength / sizeof(WCHAR));
        wcsncat(*FullPath, FileName, FullPathLength / sizeof(WCHAR));
    }
    else
    {
        /* Bail out since we have no memory */
        Status = STATUS_NO_MEMORY;
    }

Quickie:
    /* Return to caller */
    return Status;
}

NTSTATUS
BmOpenDataStore (
    _Out_ PHANDLE Handle
    )
{
    NTSTATUS Status;
    PBL_DEVICE_DESCRIPTOR BcdDevice;
    PWCHAR BcdPath, FullPath, PathBuffer;
    BOOLEAN HavePath;
    ULONG PathLength, PathLengthWithNul, FullSize;
    PVOID FinalBuffer;
    UNICODE_STRING BcdString;

    PathBuffer = NULL;
    BcdDevice = NULL;
    BcdPath = NULL;
    HavePath = FALSE;

    /* Check if a boot.ini file exists */
    Status = BmOpenBootIni();
    if (NT_SUCCESS(Status))
    {
        BmBootIniUsed = TRUE;
    }

    /* Check on which device the BCD is */
    Status = BlGetBootOptionDevice(BlpApplicationEntry.BcdData,
                                   BcdBootMgrDevice_BcdDevice,
                                   &BcdDevice,
                                   NULL);
    if (!NT_SUCCESS(Status))
    {
        /* It's not on a custom device, so it must be where we are */
        Status = BlGetBootOptionDevice(BlpApplicationEntry.BcdData,
                                       BcdLibraryDevice_ApplicationDevice,
                                       &BcdDevice,
                                       NULL);
        if (!NT_SUCCESS(Status))
        {
            /* This BCD option is required */
            goto Quickie;
        }
    }

    /* Next, check what file contains the BCD */
    Status = BlGetBootOptionString(BlpApplicationEntry.BcdData,
                                   BcdBootMgrString_BcdFilePath,
                                   &BcdPath);
    if (NT_SUCCESS(Status))
    {
        /* We don't handle custom BCDs yet */
        EfiPrintf(L"Not handled\n");
        Status = STATUS_NOT_IMPLEMENTED;
        goto Quickie;
    }

    /* Now check if the BCD is on a remote share */
    if (BcdDevice->DeviceType == UdpDevice)
    {
        /* Nope. Nope. Nope */
        EfiPrintf(L"Not handled\n");
        Status = STATUS_NOT_IMPLEMENTED;
        goto Quickie;
    }

    /* Otherwise, compute the hardcoded path of the BCD */
    Status = BmpFwGetFullPath(L"\\BCD", &FullPath);
    if (!NT_SUCCESS(Status))
    {
        /* User the raw path */
        PathBuffer = BcdPath;
    }
    else
    {
        /* Use the path we got */
        PathBuffer = FullPath;
        HavePath = TRUE;
    }

    /* Check if we failed to get the BCD path */
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Add a NUL to the path, make sure it'll fit */
    Status = STATUS_SUCCESS;
    PathLength = wcslen(PathBuffer);
    PathLengthWithNul = PathLength + 1;
    if (PathLengthWithNul < PathLength)
    {
        PathLengthWithNul = -1;
        Status = STATUS_INTEGER_OVERFLOW;
    }

    /* Bail out if it doesn't fit */
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Now add the size of the path to the device path, check if it fits */
    PathLengthWithNul = PathLengthWithNul * sizeof(WCHAR);
    FullSize = PathLengthWithNul + BcdDevice->Size;
    if (FullSize < BcdDevice->Size)
    {
        FullSize = -1;
        Status = STATUS_INTEGER_OVERFLOW;
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    /* Bail out if it doesn't fit */
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Allocate a final structure to hold both entities */
    FinalBuffer = BlMmAllocateHeap(FullSize);
    if (!FinalBuffer)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Copy the device path and file path into the final buffer */
    RtlCopyMemory(FinalBuffer, BcdDevice, BcdDevice->Size);
    RtlCopyMemory((PVOID)((ULONG_PTR)FinalBuffer + BcdDevice->Size),
                  PathBuffer,
                  PathLengthWithNul);

    /* Now tell the BCD engine to open the store */
    BcdString.Length = FullSize;
    BcdString.MaximumLength = FullSize;
    BcdString.Buffer = FinalBuffer;
    Status = BcdOpenStoreFromFile(&BcdString, Handle);

    /* Free our final buffer */
    BlMmFreeHeap(FinalBuffer);

Quickie:
    /* Did we allocate a device? */
    if (BcdDevice)
    {
        /* Free it */
        BlMmFreeHeap(BcdDevice);
    }

    /* Is this the failure path? */
    if (!NT_SUCCESS(Status))
    {
        /* Raise a fatal error */
        BmFatalErrorEx(1, (ULONG_PTR)PathBuffer, Status, 0, 0);
    }

    /* Did we get an allocated path? */
    if ((PathBuffer) && (HavePath))
    {
        /* Free it */
        BlMmFreeHeap(PathBuffer);
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
    HANDLE BcdHandle;

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

    /* Load and initialize the boot configuration database (BCD) */
    Status = BmOpenDataStore(&BcdHandle);
    EfiPrintf(L"BCD Open: %lx\r\n", Status);

    /* do more stuff!! */
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

