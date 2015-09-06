/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/firmware/efi/firmware.c
 * PURPOSE:         Boot Library Firmware Initialization for EFI
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

GUID EfiSimpleTextInputExProtocol = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;

PBL_FIRMWARE_DESCRIPTOR EfiFirmwareParameters;
BL_FIRMWARE_DESCRIPTOR EfiFirmwareData;
EFI_HANDLE EfiImageHandle;
EFI_SYSTEM_TABLE* EfiSystemTable;

EFI_SYSTEM_TABLE *EfiST;
EFI_BOOT_SERVICES *EfiBS;
EFI_RUNTIME_SERVICES *EfiRT;
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *EfiConOut;
EFI_SIMPLE_TEXT_INPUT_PROTOCOL *EfiConIn;
EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *EfiConInEx;

/* FUNCTIONS *****************************************************************/

NTSTATUS
EfiOpenProtocol (
    _In_ EFI_HANDLE Handle, 
    _In_ EFI_GUID *Protocol, 
    _Out_ PVOID* Interface
    )
{
    EFI_STATUS EfiStatus;
    NTSTATUS Status;
    BL_ARCH_MODE OldMode;

    /* Are we using virtual memory/ */
    if (MmTranslationType != BlNone)
    {
        /* We need complex tracking to make this work */
        //Status = EfiVmOpenProtocol(Handle, Protocol, Interface);
        Status = STATUS_NOT_SUPPORTED;
    }
    else
    {
        /* Are we in protected mode? */
        OldMode = CurrentExecutionContext->Mode;
        if (OldMode != BlRealMode)
        {
            /* FIXME: Not yet implemented */
            return STATUS_NOT_IMPLEMENTED;
        }

        /* Are we on legacy 1.02? */
        if (EfiST->FirmwareRevision == EFI_1_02_SYSTEM_TABLE_REVISION)
        {
            /* Make the legacy call */
            EfiStatus = EfiBS->HandleProtocol(Handle, Protocol, Interface);
        }
        else
        {
            /* Use the UEFI version */
            EfiStatus = EfiBS->OpenProtocol(Handle,
                                            Protocol,
                                            Interface,
                                            EfiImageHandle,
                                            NULL,
                                            EFI_OPEN_PROTOCOL_GET_PROTOCOL);

            /* Switch back to protected mode if we came from there */
            if (OldMode != BlRealMode)
            {
                BlpArchSwitchContext(OldMode);
            }
        }

        /* Convert the error to an NTSTATUS */
        Status = EfiGetNtStatusCode(EfiStatus);
    }

    /* Clear the interface on failure, and return the status */
    if (!NT_SUCCESS(Status))
    {
        *Interface = NULL;
    }

    return Status;
}

NTSTATUS
EfiConInExSetState (
    _In_ EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *ConInEx,
    _In_ EFI_KEY_TOGGLE_STATE* KeyToggleState
    )
{
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Make the EFI call */
    EfiStatus = ConInEx->SetState(ConInEx, KeyToggleState);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

NTSTATUS
EfiSetWatchdogTimer (
    VOID
    )
{
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Make the EFI call */
    EfiStatus = EfiBS->SetWatchdogTimer(0, 0, 0, NULL);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

NTSTATUS
EfiGetMemoryMap (
    _Out_ UINTN* MemoryMapSize,
    _Inout_ EFI_MEMORY_DESCRIPTOR *MemoryMap,
    _Out_ UINTN* MapKey,
    _Out_ UINTN* DescriptorSize,
    _Out_ UINTN* DescriptorVersion
    )
{
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Make the EFI call */
    EfiStatus = EfiBS->GetMemoryMap(MemoryMapSize,
                                    MemoryMap,
                                    MapKey,
                                    DescriptorSize,
                                    DescriptorVersion);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

NTSTATUS
EfiFreePages (
    _In_ ULONG Pages, 
    _In_ EFI_PHYSICAL_ADDRESS PhysicalAddress
    )
{
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Make the EFI call */
    EfiStatus = EfiBS->FreePages(PhysicalAddress, Pages);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

NTSTATUS
EfiAllocatePages (
    _In_ ULONG Type,
    _In_ ULONG Pages,
    _Inout_ EFI_PHYSICAL_ADDRESS* Memory
    )
{
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Make the EFI call */
    EfiStatus = EfiBS->AllocatePages(Type, EfiLoaderData, Pages, Memory);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

BL_MEMORY_ATTR
MmFwpGetOsAttributeType (
    _In_ ULONGLONG Attribute
    )
{
    BL_MEMORY_ATTR OsAttribute = 0;

    if (Attribute & EFI_MEMORY_UC)
    {
        OsAttribute = BlMemoryUncached;
    }

    if (Attribute & EFI_MEMORY_WC)
    {
        OsAttribute |= BlMemoryWriteCombined;
    }

    if (Attribute & EFI_MEMORY_WT)
    {
        OsAttribute |= BlMemoryWriteThrough;
    }

    if (Attribute & EFI_MEMORY_WB)
    {
        OsAttribute |= BlMemoryWriteBack;
    }

    if (Attribute & EFI_MEMORY_UCE)
    {
        OsAttribute |= BlMemoryUncachedExported;
    }

    if (Attribute & EFI_MEMORY_WP)
    {
        OsAttribute |= BlMemoryWriteProtected;
    }

    if (Attribute & EFI_MEMORY_RP)
    {
        OsAttribute |= BlMemoryReadProtected;
    }

    if (Attribute & EFI_MEMORY_XP)
    {
        OsAttribute |= BlMemoryExecuteProtected;
    }

    if (Attribute & EFI_MEMORY_RUNTIME)
    {
        OsAttribute |= BlMemoryRuntime;
    }

    return OsAttribute;
}

BL_MEMORY_TYPE
MmFwpGetOsMemoryType (
    _In_ EFI_MEMORY_TYPE MemoryType
    )
{
    BL_MEMORY_TYPE OsType;

    switch (MemoryType)
    {
        case EfiLoaderCode:
        case EfiLoaderData:
            OsType = BlLoaderMemory;
            break;

        case EfiBootServicesCode:
        case EfiBootServicesData:
            OsType = BlEfiBootMemory;
            break;

        case EfiRuntimeServicesCode:
        case EfiRuntimeServicesData:
            OsType = BlEfiRuntimeMemory;
            break;

        case EfiConventionalMemory:
            OsType = BlConventionalMemory;
            break;

        case EfiUnusableMemory:
            OsType = BlUnusableMemory;
            break;

        case EfiACPIReclaimMemory:
            OsType = BlAcpiReclaimMemory;
            break;

        case EfiACPIMemoryNVS:
            OsType = BlAcpiNvsMemory;
            break;

        case EfiMemoryMappedIO:
            OsType = BlDeviceIoMemory;
            break;

        case EfiMemoryMappedIOPortSpace:
            OsType = BlDevicePortMemory;
            break;

        case EfiPalCode:
            OsType = BlPalMemory;
            break;

        default:
            OsType = BlReservedMemory;
            break;
    }

    return OsType;
}

NTSTATUS
MmFwGetMemoryMap (
    _Out_ PBL_MEMORY_DESCRIPTOR_LIST MemoryMap,
    _In_ ULONG Flags
    )
{
    BL_LIBRARY_PARAMETERS LibraryParameters = BlpLibraryParameters;
    BOOLEAN UseEfiBuffer, HaveRamDisk;
    NTSTATUS Status;
    ULONGLONG Pages, StartPage, EndPage;
    UINTN EfiMemoryMapSize, MapKey, DescriptorSize, DescriptorVersion;
    EFI_PHYSICAL_ADDRESS EfiBuffer;
    EFI_MEMORY_DESCRIPTOR* EfiMemoryMap;
    EFI_STATUS EfiStatus;
    BL_ARCH_MODE OldMode;
    EFI_MEMORY_DESCRIPTOR EfiDescriptor;
    BL_MEMORY_TYPE MemoryType;
    PBL_MEMORY_DESCRIPTOR Descriptor;
    BL_MEMORY_ATTR Attribute;

    /* Initialize EFI memory map attributes */
    EfiMemoryMapSize = MapKey = DescriptorSize = DescriptorVersion = 0;

    /* Increment the nesting depth */
    MmDescriptorCallTreeCount++;

    /* Determine if we should use EFI or our own allocator at this point */
    UseEfiBuffer = Flags & BL_MM_FLAG_USE_FIRMWARE_FOR_MEMORY_MAP_BUFFERS;
    if (!(LibraryParameters.LibraryFlags & BL_LIBRARY_FLAG_INITIALIZATION_COMPLETED))
    {
        UseEfiBuffer = TRUE;
    }

    /* Bail out if we don't have a list to use */
    if (MemoryMap == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Free the current descriptor list */
    MmMdFreeList(MemoryMap);

    /* Call into EFI to get the size of the memory map */
    Status = EfiGetMemoryMap(&EfiMemoryMapSize,
                             NULL,
                             &MapKey,
                             &DescriptorSize,
                             &DescriptorVersion);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        /* This should've failed because our buffer was too small, nothing else */
        EarlyPrint(L"Got strange EFI status for memory map: %lx\n", Status);
        if (NT_SUCCESS(Status))
        {
            Status = STATUS_UNSUCCESSFUL;
        }
        goto Quickie;
    }

    /* Add 4 more descriptors just in case things changed */
    EfiMemoryMapSize += (4 * DescriptorSize);
    Pages = BYTES_TO_PAGES(EfiMemoryMapSize);
    EarlyPrint(L"Memory map size: %lx bytes, %d pages\n", EfiMemoryMapSize, Pages);

    /* Should we use EFI to grab memory? */
    if (UseEfiBuffer)
    {
        /* Yes -- request one more page to align up correctly */
        Pages++;

        /* Grab the required pages */
        Status = EfiAllocatePages(AllocateAnyPages,
                                  Pages,
                                  &EfiBuffer);
        if (!NT_SUCCESS(Status))
        {
            EarlyPrint(L"EFI allocation failed: %lx\n", Status);
            goto Quickie;
        }

        /* Free the pages for now */
        Status = EfiFreePages(Pages, EfiBuffer);
        if (!NT_SUCCESS(Status))
        {
            EfiBuffer = 0;
            goto Quickie;
        }

        /* Now round to the actual buffer size, removing the extra page */
        EfiBuffer = ROUND_TO_PAGES(EfiBuffer);
        Pages--;
        Status = EfiAllocatePages(AllocateAddress,
                                  Pages,
                                  &EfiBuffer);
        if (!NT_SUCCESS(Status))
        {
            EfiBuffer = 0;
            goto Quickie;
        }

        /* Get the final aligned size and proper buffer */
        EfiMemoryMapSize = EFI_PAGES_TO_SIZE(Pages);
        EfiMemoryMap = (EFI_MEMORY_DESCRIPTOR*)(ULONG_PTR)EfiBuffer;

        /* Switch to real mode if not already in it */
        OldMode = CurrentExecutionContext->Mode;
        if (OldMode != BlRealMode)
        {
            BlpArchSwitchContext(BlRealMode);
        }

        /* Call EFI to get the memory map */
        EfiStatus = EfiBS->GetMemoryMap(&EfiMemoryMapSize,
                                        EfiMemoryMap,
                                        &MapKey,
                                        &DescriptorSize,
                                        &DescriptorVersion);

        /* Switch back into the previous mode */
        if (OldMode != BlRealMode)
        {
            BlpArchSwitchContext(OldMode);
        }

        /* Convert the result code */
        Status = EfiGetNtStatusCode(EfiStatus);
    }
    else
    {
        /* We don't support this path yet */
        Status = STATUS_NOT_IMPLEMENTED;
    }

    /* So far so good? */
    if (!NT_SUCCESS(Status))
    {
        EarlyPrint(L"Failed to get EFI memory map: %lx\n", Status);
        goto Quickie;
    }

    /* Did we get correct data from firmware? */
    if (((EfiMemoryMapSize % DescriptorSize)) ||
        (DescriptorSize < sizeof(EFI_MEMORY_DESCRIPTOR)))
    {
        EarlyPrint(L"Incorrect descriptor size\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Quickie;
    }

    /* Did we boot from a RAM disk? */
    if ((BlpBootDevice->DeviceType == LocalDevice) &&
        (BlpBootDevice->Local.Type == RamDiskDevice))
    {
        /* We don't handle this yet */
        EarlyPrint(L"RAM boot not supported\n");
        Status = STATUS_NOT_IMPLEMENTED;
        goto Quickie;
    }
    else
    {
        /* We didn't, so there won't be any need to find the memory descriptor */
        HaveRamDisk = FALSE;
    }

    /* Loop the EFI memory map */
    EarlyPrint(L"UEFI MEMORY MAP\n\n");
    EarlyPrint(L"TYPE        START              END                   ATTRIBUTES\n");
    EarlyPrint(L"===============================================================\n");
    while (EfiMemoryMapSize != 0)
    {
        /* Check if this is an EFI buffer, but we're not in real mode */
        if ((UseEfiBuffer) && (OldMode != BlRealMode))
        {
            BlpArchSwitchContext(BlRealMode);
        }

        /* Capture it so we can go back to protected mode (if possible) */
        EfiDescriptor = *EfiMemoryMap;

        /* Go back to protected mode, if we had switched */
        if ((UseEfiBuffer) && (OldMode != BlRealMode))
        {
            BlpArchSwitchContext(OldMode);
        }

        /* Convert to OS memory type */
        MemoryType = MmFwpGetOsMemoryType(EfiDescriptor.Type);

        /* Round up or round down depending on where the memory is coming from */
        if (MemoryType == BlConventionalMemory)
        {
            StartPage = BYTES_TO_PAGES(EfiDescriptor.PhysicalStart);
        }
        else
        {
            StartPage = EfiDescriptor.PhysicalStart >> PAGE_SHIFT;
        }

        /* Calculate the ending page */
        EndPage = StartPage + EfiDescriptor.NumberOfPages;

        /* If after rounding, we ended up with 0 pages, skip this */
        if (StartPage == EndPage)
        {
            goto LoopAgain;
        }

        EarlyPrint(L"%08X    0x%016I64X-0x%016I64X    0x%I64X\n",
                   MemoryType,
                   StartPage << PAGE_SHIFT,
                   EndPage << PAGE_SHIFT,
                   EfiDescriptor.Attribute);

        /* Check for any range of memory below 1MB */
        if (StartPage < 0x100)
        {
            /* Does this range actually contain NULL? */
            if (StartPage == 0)
            {
                /* Manually create a reserved descriptof for this page */
                Attribute = MmFwpGetOsAttributeType(EfiDescriptor.Attribute);
                Descriptor = MmMdInitByteGranularDescriptor(Attribute,
                                                            BlReservedMemory,
                                                            0,
                                                            0,
                                                            1);
                if (!Descriptor)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                /* Add this descriptor into the list */
                Status = MmMdAddDescriptorToList(MemoryMap,
                                                 Descriptor,
                                                 BL_MM_ADD_DESCRIPTOR_TRUNCATE_FLAG);
                if (!NT_SUCCESS(Status))
                {
                    EarlyPrint(L"Failed to add zero page descriptor: %lx\n", Status);
                    break;
                }

                /* Now handle the rest of the range, unless this was it */
                StartPage = 1;
                if (EndPage == 1)
                {
                    goto LoopAgain;
                }
            }

            /* Does the range go beyond 1MB? */
            if (EndPage > 0x100)
            {
                /* Then create the descriptor for everything up until the megabyte */
                Attribute = MmFwpGetOsAttributeType(EfiDescriptor.Attribute);
                Descriptor = MmMdInitByteGranularDescriptor(Attribute,
                                                            MemoryType,
                                                            StartPage,
                                                            0,
                                                            0x100 - StartPage);
                if (!Descriptor)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                /* Check if this region is currently free RAM */
                if (Descriptor->Type == BlConventionalMemory)
                {
                    /* Set an unknown flag on the descriptor */
                    Descriptor->Flags |= 0x80000;
                }

                /* Add this descriptor into the list */
                Status = MmMdAddDescriptorToList(MemoryMap,
                                                 Descriptor,
                                                 BL_MM_ADD_DESCRIPTOR_TRUNCATE_FLAG);
                if (!NT_SUCCESS(Status))
                {
                    EarlyPrint(L"Failed to add 1MB descriptor: %lx\n", Status);
                    break;
                }

                /* Now handle the rest of the range above 1MB */
                StartPage = 0x100;
            }
        }

        /* Check if we loaded from a RAM disk */
        if (HaveRamDisk)
        {
            /* We don't handle this yet */
            EarlyPrint(L"RAM boot not supported\n");
            Status = STATUS_NOT_IMPLEMENTED;
            goto Quickie;
        }

        /* Create a descriptor for the current range */
        Attribute = MmFwpGetOsAttributeType(EfiDescriptor.Attribute);
        Descriptor = MmMdInitByteGranularDescriptor(Attribute,
                                                    MemoryType,
                                                    StartPage,
                                                    0,
                                                    EndPage - StartPage);
        if (!Descriptor)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        /* Check if this region is currently free RAM below 1MB */
        if ((Descriptor->Type == BlConventionalMemory) && (EndPage <= 0x100))
        {
            /* Set an unknown flag on the descriptor */
            Descriptor->Flags |= 0x80000;
        }

        /* Add the descriptor to the list, requesting coalescing as asked */
        Status = MmMdAddDescriptorToList(MemoryMap,
                                         Descriptor,
                                         BL_MM_ADD_DESCRIPTOR_TRUNCATE_FLAG |
                                         (Flags & BL_MM_FLAG_REQUEST_COALESCING) ?
                                         BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG : 0);
        if (!NT_SUCCESS(Status))
        {
            EarlyPrint(L"Failed to add full descriptor: %lx\n", Status);
            break;
        }

LoopAgain:
        /* Consume this descriptor, and move to the next one */
        EfiMemoryMapSize -= DescriptorSize;
        EfiMemoryMap = (PVOID)((ULONG_PTR)EfiMemoryMap + DescriptorSize);
    }

    /* FIXME: @TODO: Mark the EfiBuffer as free, since we're about to free it */
    /* For now, just "leak" the 1-2 pages... */

Quickie:
    /* Free the EFI buffer, if we had one */
    if (EfiBuffer != 0)
    {
        EfiFreePages(Pages, EfiBuffer);
    }

    /* On failure, free the memory map if one was passed in */
    if (!NT_SUCCESS(Status) && (MemoryMap != NULL))
    {
        MmMdFreeList(MemoryMap);
    }

    /* Decrement the nesting depth and return */
    MmDescriptorCallTreeCount--;
    return Status;
}

NTSTATUS
BlpFwInitialize (
    _In_ ULONG Phase,
    _In_ PBL_FIRMWARE_DESCRIPTOR FirmwareData
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    EFI_KEY_TOGGLE_STATE KeyToggleState;

    /* Check if we have vaild firmware data */
    if (!(FirmwareData) || !(FirmwareData->Version))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check which boot phase we're in */
    if (Phase != 0)
    {
        /* Memory manager is ready, open the extended input protocol */
        Status = EfiOpenProtocol(EfiST->ConsoleInHandle,
                                 &EfiSimpleTextInputExProtocol,
                                 (PVOID*)&EfiConInEx);
        if (NT_SUCCESS(Status))
        {
            /* Set the initial key toggle state */
            KeyToggleState = EFI_TOGGLE_STATE_VALID | 40;
            EfiConInExSetState(EfiST->ConsoleInHandle, &KeyToggleState);
        }

        /* Setup the watchdog timer */
        EfiSetWatchdogTimer();
    }
    else
    {
        /* Make a copy of the parameters */
        EfiFirmwareParameters = &EfiFirmwareData;

        /* Check which version we received */
        if (FirmwareData->Version == 1)
        {
            /* FIXME: Not supported */
            Status = STATUS_NOT_SUPPORTED;
        }
        else if (FirmwareData->Version >= 2)
        {
            /* Version 2 -- save the data */
            EfiFirmwareData = *FirmwareData;
            EfiSystemTable = FirmwareData->SystemTable;
            EfiImageHandle = FirmwareData->ImageHandle;

            /* Set the EDK-II style variables as well */
            EfiST = EfiSystemTable;
            EfiBS = EfiSystemTable->BootServices;
            EfiRT = EfiSystemTable->RuntimeServices;
            EfiConOut = EfiSystemTable->ConOut;
            EfiConIn = EfiSystemTable->ConIn;
            EfiConInEx = NULL;
        }
        else
        {
            /* Unknown version */
            Status = STATUS_NOT_SUPPORTED;
        }
    }

    /* Return the initialization state */
    return Status;
}

