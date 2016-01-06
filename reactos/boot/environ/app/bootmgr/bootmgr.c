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

BOOLEAN BmDisplayStateCached;

/* FUNCTIONS *****************************************************************/

NTSTATUS
BmGetOptionList (
    _In_ HANDLE BcdHandle,
    _In_ PGUID ObjectId,
    _In_ PBL_BCD_OPTION *OptionList
    )
{
    NTSTATUS Status;
    HANDLE ObjectHandle;
    ULONG ElementSize, ElementCount, i, OptionsSize;
    BcdElementType Type;
    PBCD_ELEMENT_HEADER Header;
    PBCD_ELEMENT BcdElements;
    PBL_BCD_OPTION Options, Option, PreviousOption, DeviceOptions;
    PBCD_DEVICE_OPTION DeviceOption;
    GUID DeviceId;
    PVOID DeviceData;

    /* Open the BCD object requested */
    ObjectHandle = NULL;
    BcdElements = NULL;
    Status = BcdOpenObject(BcdHandle, ObjectId, &ObjectHandle);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Do the initial enumeration to get the size needed */
    ElementSize = 0;
    Status = BcdEnumerateAndUnpackElements(BcdHandle,
                                           ObjectHandle,
                                           NULL,
                                           &ElementSize,
                                           &ElementCount);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        /* If we got success, that doesn't make any sense */
        if (NT_SUCCESS(Status))
        {
            Status = STATUS_INVALID_PARAMETER;
        }

        /* Bail out */
        goto Quickie;
    }

    /* Allocate a large-enough buffer */
    BcdElements = BlMmAllocateHeap(ElementSize);
    if (!BcdElements)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Now do the real enumeration to fill out the elements buffer */
    Status = BcdEnumerateAndUnpackElements(BcdHandle,
                                           ObjectHandle,
                                           BcdElements,
                                           &ElementSize,
                                           &ElementCount);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Go through each BCD option to add the sizes up */
    OptionsSize = 0;
    for (i = 0; i < ElementCount; i++)
    {
        OptionsSize += BcdElements[i].Header->Size + sizeof(BL_BCD_OPTION);
    }

    /* Allocate the required BCD option list */
    Options = BlMmAllocateHeap(OptionsSize);
    if (!Options)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Zero it out */
    RtlZeroMemory(Options, OptionsSize);

    /* Start going through each option */
    PreviousOption = NULL;
    Option = Options;
    EfiPrintf(L"BCD Options found: %d\r\n", ElementCount);
    for (i = 0; i < ElementCount; i++)
    {
        /* Read the header and type */
        Header = BcdElements[i].Header;
        Type.PackedValue = Header->Type;

        /* Check if this option isn't already present */
        if (!MiscGetBootOption(Options, Type.PackedValue))
        {
            /* It's a new option. Did we have an existing one? */
            if (PreviousOption)
            {
                /* Link it to this new one */
                PreviousOption->NextEntryOffset = (ULONG_PTR)Option -
                                                  (ULONG_PTR)Options;
            }

            /* Capture the type, size, data, and offset */
            Option->Type = Type.PackedValue;
            Option->DataSize = Header->Size;
            RtlCopyMemory(Option + 1, BcdElements[i].Body, Header->Size);
            Option->DataOffset = sizeof(BL_BCD_OPTION);

            /* Check if this was a device */
            if (Type.Format == BCD_TYPE_DEVICE)
            {
                /* Grab its GUID */
                DeviceOption = (PBCD_DEVICE_OPTION)(Option + 1);
                DeviceId = DeviceOption->AssociatedEntry;

                /* Look up the options for that GUID */
                Status = BmGetOptionList(BcdHandle, &DeviceId, &DeviceOptions);
                if (NT_SUCCESS(Status))
                {
                    /* Device data is after the device option */
                    DeviceData = (PVOID)((ULONG_PTR)DeviceOption + Header->Size);

                    /* Copy it */
                    RtlCopyMemory(DeviceData,
                                  DeviceOptions,
                                  BlGetBootOptionListSize(DeviceOptions));

                    /* Don't need this anymore */
                    BlMmFreeHeap(DeviceOptions);

                    /* Write the offset of the device options */
                    Option->ListOffset = (ULONG_PTR)DeviceData -
                                         (ULONG_PTR)Option;
                }
            }

            /* Save the previous option and go to the next one */
            PreviousOption = Option;
            Option = (PBL_BCD_OPTION)((ULONG_PTR)Option +
                                      BlGetBootOptionSize(Option));
        }
    }

    /* Return the pointer back, we've made it! */
    *OptionList = Options;
    Status = STATUS_SUCCESS;

Quickie:
    /* Did we allocate a local buffer? Free it if so */
    if (BcdElements)
    {
        BlMmFreeHeap(BcdElements);
    }

    /* Was the key open? Close it if so */
    if (ObjectHandle)
    {
        BiCloseKey(ObjectHandle);
    }

    /* Return the option list parsing status */
    return Status;
}

NTSTATUS
BmpUpdateApplicationOptions (
    _In_ HANDLE BcdHandle
    )
{
    NTSTATUS Status;
    PBL_BCD_OPTION Options;

    /* Get the boot option list */
    Status = BmGetOptionList(BcdHandle, &BmApplicationIdentifier, &Options);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Append the options, free the local buffer, and return success */
    BlAppendBootOptions(&BlpApplicationEntry, Options);
    BlMmFreeHeap(Options);
    return STATUS_SUCCESS;
}

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
        Status = RtlULongAdd(i, 1, &AppPathLength);
        if (NT_SUCCESS(Status))
        {
            /* Check if it's safe to multiply by two */
            Status = RtlULongMult(AppPathLength, sizeof(WCHAR), &AppPathLength);
            if (NT_SUCCESS(Status))
            {
                /* Allocate a copy for the string */
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
    BootDirectory = L"\\EFI\\Boot"; /* Should be EFI\\ReactOS\\Boot */

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
                     L"\nAn error occurred (%08x) while attempting "
                     L"to read the boot configuration data file %s\n",
                     ErrorStatus,
                     Buffer);

            /* Select the resource ID message */
            ErrorResourceId = 9002;
            break;

        case BL_FATAL_ERROR_BCD_PARSE:

            /* File name isin parameter 1 */
            FileName = (PWCHAR)Parameter1;

            /* The NTSTATUS code is in parameter 2*/
            ErrorStatus = (NTSTATUS)Parameter2;

            /* Build the error string */
            swprintf(FormatString,
                     L"\nThe boot configuration file %s is invalid (%08x).\n",
                     FileName,
                     ErrorStatus);

            /* Select the resource ID message */
            ErrorResourceId = 9015;
            break;

        case BL_FATAL_ERROR_GENERIC:

            /* The NTSTATUS code is in parameter 1*/
            ErrorStatus = (NTSTATUS)Parameter1;

            /* Build the error string */
            swprintf(FormatString,
                     L"\nThe boot manager experienced an error (%08x).\n",
                     ErrorStatus);

            /* Select the resource ID message */
            ErrorResourceId = 9005;
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
    ULONG BootDirLength, PathLength;

    /* Compute the length of the directory, and add a NUL */
    BootDirLength = wcslen(BootDirectory);
    Status = RtlULongAdd(BootDirLength, 1, &BootDirLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Add the length of the file, make sure it fits */
    PathLength = wcslen(FileName);
    Status = RtlULongAdd(PathLength, BootDirLength, &PathLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Convert to bytes */
    Status = RtlULongLongToULong(PathLength * sizeof(WCHAR), &PathLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Allocate the full path */
    *FullPath = BlMmAllocateHeap(PathLength);
    if (*FullPath)
    {
        /* Copy the directory followed by the file name */
        wcsncpy(*FullPath, BootDirectory, PathLength / sizeof(WCHAR));
        wcsncat(*FullPath, FileName, PathLength / sizeof(WCHAR));
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

VOID
BmCloseDataStore (
    _In_ HANDLE Handle
    )
{
    /* Check if boot.ini data needs to be freed */
    if (BmBootIniUsed)
    {
        EfiPrintf(L"Not handled\r\n");
    }

    /* Dereference the hive and close the key */
    BiDereferenceHive(Handle);
    BiCloseKey(Handle);
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
    ULONG PathLength, FullSize;
    PVOID FinalBuffer;
    UNICODE_STRING BcdString;

    /* Initialize variables */
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
        EfiPrintf(L"Not handled: %s\r\n", BcdPath);
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
    PathLength = wcslen(PathBuffer);
    Status = RtlULongAdd(PathLength, 1, &PathLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Convert to bytes */
    Status = RtlULongLongToULong(PathLength * sizeof(WCHAR), &PathLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Now add the size of the path to the device path, check if it fits */
    Status = RtlULongAdd(PathLength, BcdDevice->Size, &FullSize);
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
                  PathLength);

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
        BmFatalErrorEx(BL_FATAL_ERROR_BCD_READ,
                       (ULONG_PTR)PathBuffer,
                       Status,
                       0,
                       0);
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

typedef struct _BL_BSD_LOG_OBJECT
{
    ULONG DeviceId;
    ULONG FileId;
    ULONG Unknown;
    ULONG Size;
    ULONG Flags;
} BL_BSD_LOG_OBJECT, *PBL_BSD_LOG_OBJECT;

BL_BSD_LOG_OBJECT BsdpLogObject;
BOOLEAN BsdpLogObjectInitialized;

VOID
BlBsdInitializeLog (
    _In_ PBL_DEVICE_DESCRIPTOR LogDevice,
    _In_ PWCHAR LogPath,
    _In_ ULONG Flags
    )
{
    NTSTATUS Status;

    /* Don't initialize twice */
    if (BsdpLogObjectInitialized)
    {
        return;
    }

    /* Set invalid IDs for now */
    BsdpLogObject.DeviceId = -1;
    BsdpLogObject.FileId = -1;

    /* Open the BSD device */
    Status = BlpDeviceOpen(LogDevice,
                           BL_DEVICE_READ_ACCESS | BL_DEVICE_WRITE_ACCESS,
                           0,
                           &BsdpLogObject.DeviceId);
    if (!NT_SUCCESS(Status))
    {
        /* Welp that didn't work */
        goto FailurePath;
    }

    /* Now open the BSD itself */
    Status = BlFileOpen(BsdpLogObject.DeviceId,
                        LogPath,
                        BL_FILE_READ_ACCESS | BL_FILE_WRITE_ACCESS,
                        &BsdpLogObject.FileId);
    if (!NT_SUCCESS(Status))
    {
        /* D'oh */
        goto FailurePath;
    }

    /* The BSD is open. Start doing stuff to it */
    EfiPrintf(L"Unimplemented BSD path\r\n");
    Status =  STATUS_NOT_IMPLEMENTED;

FailurePath:
    /* Close the BSD if we had it open */
    if (BsdpLogObject.FileId != -1)
    {
        BlFileClose(BsdpLogObject.FileId);
    }

    /* Close the device if we had it open */
    if (BsdpLogObject.DeviceId != -1)
    {
        BlDeviceClose(BsdpLogObject.DeviceId);
    }

    /* Set BSD object to its uninitialized state */
    BsdpLogObjectInitialized = FALSE;
    BsdpLogObject.FileId = 0;
    BsdpLogObject.DeviceId = 0;
    BsdpLogObject.Flags = 0;
    BsdpLogObject.Unknown = 0;
    BsdpLogObject.Size = 0;
}

VOID
BmpInitializeBootStatusDataLog (
    VOID
    )
{
    NTSTATUS Status;
    PBL_DEVICE_DESCRIPTOR BsdDevice;
    PWCHAR BsdPath;
    ULONG Flags;
    BOOLEAN PreserveBsd;

    /* Initialize locals */
    BsdPath = NULL;
    BsdDevice = NULL;
    Flags = 0;

    /* Check if the BSD is stored in a custom device */
    Status = BlGetBootOptionDevice(BlpApplicationEntry.BcdData,
                                   BcdLibraryDevice_BsdLogDevice,
                                   &BsdDevice,
                                   NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Nope, use the boot device */
        BsdDevice = BlpBootDevice;
    }

    /* Check if the path is custom as well */
    Status = BlGetBootOptionString(BlpApplicationEntry.BcdData,
                                   BcdLibraryString_BsdLogPath,
                                   &BsdPath);
    if (!NT_SUCCESS(Status))
    {
        /* Nope, use our default path */
        Status = BmpFwGetFullPath(L"\\bootstat.dat", &BsdPath);
        if (!NT_SUCCESS(Status))
        {
            BsdPath = NULL;
        }

        /* Set preserve flag */
        Flags = 1;
    }
    else
    {
        /* Set preserve flag */
        Flags = 1;
    }

    /* Finally, check if the BSD should be preserved */
    Status = BlGetBootOptionBoolean(BlpApplicationEntry.BcdData,
                                    BcdLibraryBoolean_PreserveBsdLog,
                                    &PreserveBsd);
    if (!(NT_SUCCESS(Status)) || !(PreserveBsd))
    {
        /* We failed to read, or we were asked not to preserve it */
        Flags = 0;
    }

    /* Initialize the log */
    BlBsdInitializeLog(BsdDevice, BsdPath, Flags);

    /* Free the BSD device descriptor if we had one */
    if (BsdDevice)
    {
        BlMmFreeHeap(BsdDevice);
    }

    /* Free the BSD path if we had one */
    if ((Flags) && (BsdPath))
    {
        BlMmFreeHeap(BsdPath);
    }
}

VOID
BmFwMemoryInitialize (
    VOID
    )
{
    NTSTATUS Status;
    PHYSICAL_ADDRESS PhysicalAddress;
    BL_ADDRESS_RANGE AddressRange;

    /* Select the range below 1MB */
    AddressRange.Maximum = 0xFFFFF;
    AddressRange.Minimum = 0;

    /* Allocate one reserved page with the "reserved" attribute */
    Status = MmPapAllocatePhysicalPagesInRange(&PhysicalAddress,
                                               BlApplicationReserved,
                                               1,
                                               BlMemoryReserved,
                                               0,
                                               &MmMdlUnmappedAllocated,
                                               &AddressRange,
                                               BL_MM_REQUEST_DEFAULT_TYPE);
    if (!NT_SUCCESS(Status))
    {
        /* Print a message on error, but keep going */
        BlStatusPrint(L"BmFwMemoryInitialize: Failed to allocate a page below 1MB. Status: 0x%08x\r\n",
                      Status);
    }
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
    NTSTATUS Status, LibraryStatus;
    BL_LIBRARY_PARAMETERS LibraryParameters;
    PBL_RETURN_ARGUMENTS ReturnArguments;
    BOOLEAN RebootOnError;
    PGUID AppIdentifier;
    HANDLE BcdHandle;
    PBL_BCD_OPTION EarlyOptions;
    PWCHAR Stylesheet;

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
    if (NT_SUCCESS(Status))
    {
        /* Copy the boot options */
        Status = BlCopyBootOptions(BlpApplicationEntry.BcdData, &EarlyOptions);
        if (NT_SUCCESS(Status))
        {
            /* Update them */
            Status = BmpUpdateApplicationOptions(BcdHandle);
            if (!NT_SUCCESS(Status))
            {
                /* Log a fatal error */
                BmFatalErrorEx(BL_FATAL_ERROR_BCD_PARSE,
                               (ULONG_PTR)L"\\BCD",
                               Status,
                               0,
                               0);
            }
        }
    }

#ifdef _SECURE_BOOT
    /* Initialize the secure boot machine policy */
    Status = BmSecureBootInitializeMachinePolicy();
    if (!NT_SUCCESS(Status))
    {
        BmFatalErrorEx(BL_FATAL_ERROR_SECURE_BOOT, Status, 0, 0, 0);
    }
#endif

    /* Copy the library parameters and add the re-initialization flag */
    RtlCopyMemory(&LibraryParameters,
                  &BlpLibraryParameters, 
                  sizeof(LibraryParameters));
    LibraryParameters.LibraryFlags |= (BL_LIBRARY_FLAG_REINITIALIZE_ALL |
                                       BL_LIBRARY_FLAG_REINITIALIZE);

    /* Now that we've parsed the BCD, re-initialize the library */
    LibraryStatus = BlInitializeLibrary(BootParameters, &LibraryParameters);
    if (!NT_SUCCESS(LibraryStatus) && (NT_SUCCESS(Status)))
    {
        Status = LibraryStatus;
    }

    /* Initialize firmware-specific memory regions */
    //BmFwMemoryInitialize();

    /* Initialize the boot status data log (BSD) */
    BmpInitializeBootStatusDataLog();

    /* Find our XSL stylesheet */
    Stylesheet = BlResourceFindHtml();
    if (!Stylesheet)
    {
        /* Awe, no XML. This is actually fatal lol. Can't boot without XML. */
        Status = STATUS_NOT_FOUND;
        EfiPrintf(L"BlResourceFindMessage failed 0x%x\r\n", STATUS_NOT_FOUND);
        goto Quickie;
    }

    /* do more stuff!! */
    EfiPrintf(BlResourceFindMessage(BM_MSG_TEST));
    EfiPrintf(Stylesheet);
    EfiStall(10000000);

//Failure:
    /* Check if we got here due to an internal error */
    if (BmpInternalBootError)
    {
        /* If XML is available, display the error */
#if 0
        if (XmlLoaded)
        {
            BmDisplayDumpError(0, 0);
            BmErrorPurge();
        }
#endif

        /* Don't do a fatal error -- return back to firmware */
        goto Quickie;
    }

    /* Log a general fatal error once we're here */
    BmFatalErrorEx(BL_FATAL_ERROR_GENERIC, Status, 0, 0, 0);

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

