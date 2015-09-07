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

EFI_GUID EfiGraphicsOutputProtocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID EfiUgaDrawProtocol = EFI_UGA_DRAW_PROTOCOL_GUID;
EFI_GUID EfiLoadedImageProtocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID EfiDevicePathProtocol = EFI_DEVICE_PATH_PROTOCOL_GUID;
EFI_GUID EfiSimpleTextInputExProtocol = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;

WCHAR BlScratchBuffer[8192];

/* FUNCTIONS *****************************************************************/

VOID
EfiPrintf (
    _In_ PWCHAR Format,
    ...
    )
{
    va_list args;
    va_start(args, Format);

    /* Capture the buffer in our scratch pad, and NULL-terminate */
    vsnwprintf(BlScratchBuffer, RTL_NUMBER_OF(BlScratchBuffer) - 1, Format, args);
    BlScratchBuffer[RTL_NUMBER_OF(BlScratchBuffer) - 1] = UNICODE_NULL;

    /* Check which mode we're in */
    if (CurrentExecutionContext->Mode == BlRealMode)
    {
        /* Call EFI directly */
        EfiConOut->OutputString(EfiConOut, BlScratchBuffer);
    }
    else
    {
        /* FIXME: @TODO: Not yet supported */
    }

    /* All done */
    va_end(args);
}

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
EfiCloseProtocol (
    _In_ EFI_HANDLE Handle,
    _In_ EFI_GUID *Protocol
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
        /* Are we on legacy 1.02? */
        if (EfiST->FirmwareRevision == EFI_1_02_SYSTEM_TABLE_REVISION)
        {
            /* Nothing to close */
            EfiStatus = STATUS_SUCCESS;
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

            /* Use the UEFI version */
            EfiStatus = EfiBS->CloseProtocol(Handle, Protocol, EfiImageHandle, NULL);

            /* Switch back to protected mode if we came from there */
            if (OldMode != BlRealMode)
            {
                BlpArchSwitchContext(OldMode);
            }

            /* Normalize not found as success */
            if (EfiStatus == EFI_NOT_FOUND)
            {
                EfiStatus = EFI_SUCCESS;
            }
        }

        /* Convert the error to an NTSTATUS */
        Status = EfiGetNtStatusCode(EfiStatus);
    }

    /* All done */
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
EfiStall (
    _In_ ULONG StallTime
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
    EfiStatus = EfiBS->Stall(StallTime);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

NTSTATUS
EfiConOutQueryMode (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _In_ ULONG Mode,
    _In_ UINTN* Columns,
    _In_ UINTN* Rows
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
    EfiStatus = TextInterface->QueryMode(TextInterface, Mode, Columns, Rows);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

NTSTATUS
EfiConOutSetMode (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _In_ ULONG Mode
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
    EfiStatus = TextInterface->SetMode(TextInterface, Mode);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

NTSTATUS
EfiConOutSetAttribute (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _In_ ULONG Attribute
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
    EfiStatus = TextInterface->SetAttribute(TextInterface, Attribute);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

NTSTATUS
EfiConOutSetCursorPosition (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _In_ ULONG Column,
    _In_ ULONG Row
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
    EfiStatus = TextInterface->SetCursorPosition(TextInterface, Column, Row);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

NTSTATUS
EfiConOutEnableCursor (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _In_ BOOLEAN Visible
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
    EfiStatus = TextInterface->EnableCursor(TextInterface, Visible);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

VOID
EfiConOutReadCurrentMode (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _Out_ EFI_SIMPLE_TEXT_OUTPUT_MODE* Mode
    )
{
    BL_ARCH_MODE OldMode;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        return;
    }

    /* Make the EFI call */
    RtlCopyMemory(Mode, TextInterface->Mode, sizeof(*Mode));

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }
}

VOID
EfiGopGetFrameBuffer (
    _In_ EFI_GRAPHICS_OUTPUT_PROTOCOL *GopInterface,
    _Out_ PHYSICAL_ADDRESS* FrameBuffer,
    _Out_ UINTN *FrameBufferSize
    )
{
    BL_ARCH_MODE OldMode;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        return;
    }

    /* Make the EFI call */
    FrameBuffer->QuadPart = GopInterface->Mode->FrameBufferBase;
    *FrameBufferSize = GopInterface->Mode->FrameBufferSize;

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }
}

NTSTATUS
EfiGopGetCurrentMode (
    _In_ EFI_GRAPHICS_OUTPUT_PROTOCOL *GopInterface,
    _Out_ UINTN* Mode, 
    _Out_ EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Information
    )
{
    BL_ARCH_MODE OldMode;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Make the EFI call */
    *Mode = GopInterface->Mode->Mode;
    RtlCopyMemory(Information, GopInterface->Mode->Info, sizeof(*Information));

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Return back */
    return STATUS_SUCCESS;
}

NTSTATUS
EfiGopSetMode (
    _In_ EFI_GRAPHICS_OUTPUT_PROTOCOL *GopInterface,
    _In_ ULONG Mode
    )
{
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;
    BOOLEAN ModeChanged;
    NTSTATUS Status;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Make the EFI call */
    if (Mode == GopInterface->Mode->Mode)
    {
        EfiStatus = EFI_SUCCESS;
        ModeChanged = FALSE;
    }
    {
        EfiStatus = GopInterface->SetMode(GopInterface, Mode);
        ModeChanged = TRUE;
    }

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Print out to the debugger if the mode was changed */
    Status = EfiGetNtStatusCode(EfiStatus);
    if ((ModeChanged) && (NT_SUCCESS(Status)))
    {
        /* FIXME @TODO: Should be BlStatusPrint */
        EfiPrintf(L"Console video mode  set to 0x%x\r\r\n", Mode);
    }

    /* Convert the error to an NTSTATUS */
    return Status;
}

NTSTATUS
EfiLocateHandleBuffer (
    _In_ EFI_LOCATE_SEARCH_TYPE SearchType,
    _In_ EFI_GUID *Protocol,
    _Inout_ PULONG HandleCount,
    _Inout_ EFI_HANDLE** Buffer
    )
{
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;
    UINTN BufferSize;

    /* Bail out if we're missing parameters */
    if (!(Buffer) || !(HandleCount))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if a buffer was passed in*/
    if (*Buffer)
    {
        /* Then we should already have a buffer size*/
        BufferSize = sizeof(EFI_HANDLE) * *HandleCount;
    }
    else
    {
        /* Then no buffer size exists */
        BufferSize = 0;
    }

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Try the first time */
    EfiStatus = EfiBS->LocateHandle(SearchType, Protocol, NULL, &BufferSize, *Buffer);
    if (EfiStatus == EFI_BUFFER_TOO_SMALL)
    {
        /* Did we have an existing buffer? */
        if (*Buffer)
        {
            /* Free it */
            BlMmFreeHeap(*Buffer);
        }

        /* Allocate a new one */
        *Buffer = BlMmAllocateHeap(BufferSize);
        if (!(*Buffer))
        {
            /* No space, fail */
            return STATUS_NO_MEMORY;
        }

        /* Try again */
        EfiStatus = EfiBS->LocateHandle(SearchType, Protocol, NULL, &BufferSize, *Buffer);

        /* Switch back to protected mode if we came from there */
        if (OldMode != BlRealMode)
        {
            BlpArchSwitchContext(OldMode);
        }
    }

    /* Return the number of handles */
    *HandleCount = BufferSize / sizeof(EFI_HANDLE);

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

VOID
EfiResetSystem (
    _In_ EFI_RESET_TYPE ResetType
    )
{
    BL_ARCH_MODE OldMode;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        return;
    }

    /* Call the EFI runtime */
    EfiRT->ResetSystem(ResetType, EFI_SUCCESS, 0, NULL);
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
        EfiPrintf(L"Got strange EFI status for memory map: %lx\r\n", Status);
        if (NT_SUCCESS(Status))
        {
            Status = STATUS_UNSUCCESSFUL;
        }
        goto Quickie;
    }

    /* Add 4 more descriptors just in case things changed */
    EfiMemoryMapSize += (4 * DescriptorSize);
    Pages = BYTES_TO_PAGES(EfiMemoryMapSize);
    EfiPrintf(L"Memory map size: %lx bytes, %d pages\r\n", EfiMemoryMapSize, Pages);

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
            EfiPrintf(L"EFI allocation failed: %lx\r\n", Status);
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
        EfiPrintf(L"Failed to get EFI memory map: %lx\r\n", Status);
        goto Quickie;
    }

    /* Did we get correct data from firmware? */
    if (((EfiMemoryMapSize % DescriptorSize)) ||
        (DescriptorSize < sizeof(EFI_MEMORY_DESCRIPTOR)))
    {
        EfiPrintf(L"Incorrect descriptor size\r\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Quickie;
    }

    /* Did we boot from a RAM disk? */
    if ((BlpBootDevice->DeviceType == LocalDevice) &&
        (BlpBootDevice->Local.Type == RamDiskDevice))
    {
        /* We don't handle this yet */
        EfiPrintf(L"RAM boot not supported\r\n");
        Status = STATUS_NOT_IMPLEMENTED;
        goto Quickie;
    }
    else
    {
        /* We didn't, so there won't be any need to find the memory descriptor */
        HaveRamDisk = FALSE;
    }

    /* Loop the EFI memory map */
#if 0
    EfiPrintf(L"UEFI MEMORY MAP\n\r\n");
    EfiPrintf(L"TYPE        START              END                   ATTRIBUTES\r\n");
    EfiPrintf(L"===============================================================\r\n");
#endif
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
#if 0
        EfiPrintf(L"%08X    0x%016I64X-0x%016I64X    0x%I64X\r\n",
                   MemoryType,
                   StartPage << PAGE_SHIFT,
                   EndPage << PAGE_SHIFT,
                   EfiDescriptor.Attribute);
#endif
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
                    EfiPrintf(L"Failed to add zero page descriptor: %lx\r\n", Status);
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
                    EfiPrintf(L"Failed to add 1MB descriptor: %lx\r\n", Status);
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
            EfiPrintf(L"RAM boot not supported\r\n");
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
            EfiPrintf(L"Failed to add full descriptor: %lx\r\n", Status);
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
            EfiConInExSetState(EfiConInEx, &KeyToggleState);
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

