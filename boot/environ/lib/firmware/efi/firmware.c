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
PHYSICAL_ADDRESS EfiRsdt;

EFI_GUID EfiGraphicsOutputProtocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID EfiUgaDrawProtocol = EFI_UGA_DRAW_PROTOCOL_GUID;
EFI_GUID EfiLoadedImageProtocol = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID EfiDevicePathProtocol = EFI_DEVICE_PATH_PROTOCOL_GUID;
EFI_GUID EfiSimpleTextInputExProtocol = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;
EFI_GUID EfiBlockIoProtocol = EFI_BLOCK_IO_PROTOCOL_GUID;
EFI_GUID EfiRootAcpiTableGuid = EFI_ACPI_20_TABLE_GUID;
EFI_GUID EfiRootAcpiTable10Guid = EFI_ACPI_TABLE_GUID;
EFI_GUID EfiGlobalVariable = EFI_GLOBAL_VARIABLE;
EFI_GUID BlpEfiSecureBootPrivateNamespace = { 0x77FA9ABD , 0x0359, 0x4D32, { 0xBD, 0x60, 0x28, 0xF4, 0xE7, 0x8F, 0x78, 0x4B } };

WCHAR BlScratchBuffer[8192];

BOOLEAN BlpFirmwareChecked;
BOOLEAN BlpFirmwareEnabled;

/* FUNCTIONS *****************************************************************/

EFI_DEVICE_PATH *
EfiIsDevicePathParent (
    _In_ EFI_DEVICE_PATH *DevicePath1,
    _In_ EFI_DEVICE_PATH *DevicePath2
    )
{
    EFI_DEVICE_PATH* CurrentPath1;
    EFI_DEVICE_PATH* CurrentPath2;
    USHORT Length1, Length2;

    /* Start with the current nodes */
    CurrentPath1 = DevicePath1;
    CurrentPath2 = DevicePath2;

    /* Loop each element of the device path */
    while (!(IsDevicePathEndType(CurrentPath1)) &&
           !(IsDevicePathEndType(CurrentPath2)))
    {
        /* Check if the element has a different length */
        Length1 = DevicePathNodeLength(CurrentPath1);
        Length2 = DevicePathNodeLength(CurrentPath2);
        if (Length1 != Length2)
        {
            /* Then they're not related */
            return NULL;
        }

        /* Check if the rest of the element data matches */
        if (RtlCompareMemory(CurrentPath1, CurrentPath2, Length1) != Length1)
        {
            /* Nope, not related */
            return NULL;
        }

        /* Move to the next element */
        CurrentPath1 = NextDevicePathNode(CurrentPath1);
        CurrentPath2 = NextDevicePathNode(CurrentPath2);
    }

    /* If the last element in path 1 is empty, then path 2 is the child (deeper) */
    if (!IsDevicePathEndType(CurrentPath1))
    {
        return DevicePath2;
    }

    /* If the last element in path 2 is empty, then path 1 is the child (deeper) */
    if (!IsDevicePathEndType(CurrentPath2))
    {
        return DevicePath1;
    }

    /* They're both the end, so they're identical, so there's no parent */
    return NULL;
}

EFI_DEVICE_PATH*
EfiGetLeafNode (
    _In_ EFI_DEVICE_PATH *DevicePath
    )
{
    EFI_DEVICE_PATH *NextDevicePath;

    /* Make sure we're not already at the end */
    if (!IsDevicePathEndType(DevicePath))
    {
        /* Grab the next node element, and keep going until the end */
        for (NextDevicePath = NextDevicePathNode(DevicePath);
             !IsDevicePathEndType(NextDevicePath);
             NextDevicePath = NextDevicePathNode(NextDevicePath))
        {
            /* Save the current node we're at  */
            DevicePath = NextDevicePath;
        }
    }

    /* This now contains the deepest (leaf) node */
    return DevicePath;
}

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
        /* Switch to real mode */
        BlpArchSwitchContext(BlRealMode);

        /* Call EFI directly */
        if (EfiConOut != NULL)
        {
            EfiConOut->OutputString(EfiConOut, BlScratchBuffer);
        }

        /* Switch back to protected mode */
        BlpArchSwitchContext(BlProtectedMode);
    }

    /* All done */
    va_end(args);
}

BOOLEAN EfiProtHashTableInitialized;
ULONG EfiProtHashTableId;

typedef struct _BL_EFI_PROTOCOL
{
    LIST_ENTRY ListEntry;
    EFI_GUID* Protocol;
    PVOID Interface;
    LONG ReferenceCount;
    BOOLEAN AddressMapped;
} BL_EFI_PROTOCOL, *PBL_EFI_PROTOCOL;

NTSTATUS
EfiVmOpenProtocol (
    _In_ EFI_HANDLE Handle,
    _In_ EFI_GUID* Protocol,
    _Outptr_ PVOID* Interface
    )
{
    BOOLEAN AddressMapped;
    PLIST_ENTRY HashList, NextEntry;
    PHYSICAL_ADDRESS InterfaceAddress, TranslatedAddress;
    NTSTATUS Status;
    BL_HASH_ENTRY HashEntry;
    PBL_HASH_VALUE HashValue;
    PBL_EFI_PROTOCOL EfiProtocol;
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;
    PVOID InterfaceVa;

    /* Initialize failure paths */
    AddressMapped = FALSE;
    HashList = NULL;
    InterfaceAddress.QuadPart = 0;

    /* Have we initialized the protocol table yet? */
    if (!EfiProtHashTableInitialized)
    {
        /* Nope -- create the hash table */
        Status = BlHtCreate(0, NULL, NULL, &EfiProtHashTableId);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Remember for next time */
        EfiProtHashTableInitialized = TRUE;
    }

    /* Check if we already have a list of protocols for this handle */
    HashEntry.Flags = BL_HT_VALUE_IS_INLINE;
    HashEntry.Size = sizeof(Handle);
    HashEntry.Value = Handle;
    Status = BlHtLookup(EfiProtHashTableId, &HashEntry, &HashValue);
    if (NT_SUCCESS(Status))
    {
        /* We do -- the hash value is the list itself */
        HashList = (PLIST_ENTRY)HashValue->Data;
        NextEntry = HashList->Flink;

        /* Iterate over it */
        while (NextEntry != HashList)
        {
            /* Get each protocol in the list, checking for a match */
            EfiProtocol = CONTAINING_RECORD(NextEntry,
                                            BL_EFI_PROTOCOL,
                                            ListEntry);
            if (EfiProtocol->Protocol == Protocol)
            {
                /* Match found -- add a reference and return it */
                EfiProtocol->ReferenceCount++;
                *Interface = EfiProtocol->Interface;
                return STATUS_SUCCESS;
            }

            /* Try the next entry */
            NextEntry = NextEntry->Flink;
        }
    }

    /* Switch to real mode for firmware call */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(BlRealMode);
    }

    /* Check if this is EFI 1.02 */
    if (EfiST->Hdr.Revision == EFI_1_02_SYSTEM_TABLE_REVISION)
    {
        /* Use the old call */
        EfiStatus = EfiBS->HandleProtocol(Handle,
                                          Protocol,
                                          (PVOID*)&InterfaceAddress);
    }
    else
    {
        /* Use the EFI 2.00 API instead */
        EfiStatus = EfiBS->OpenProtocol(Handle,
                                        Protocol,
                                        (PVOID*)&InterfaceAddress,
                                        EfiImageHandle,
                                        NULL,
                                        EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    }

    /* Switch back to protected mode if needed */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Check the result, and bail out on failure */
    Status = EfiGetNtStatusCode(EfiStatus);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Check what address the interface lives at, and translate it */
    InterfaceVa = PhysicalAddressToPtr(InterfaceAddress);
    if (BlMmTranslateVirtualAddress(InterfaceVa, &TranslatedAddress))
    {
        /* We expect firmware to be 1:1 mapped, fail if not */
        if (InterfaceAddress.QuadPart != TranslatedAddress.QuadPart)
        {
            return STATUS_NOT_SUPPORTED;
        }
    }
    else
    {
        /* Create a virtual (1:1) mapping for the interface */
        Status = BlMmMapPhysicalAddressEx(&InterfaceVa,
                                          BlMemoryFixed,
                                          PAGE_SIZE,
                                          InterfaceAddress);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Remember for cleanup */
        AddressMapped = TRUE;
    }

    /* The caller now has the interface */
    *Interface = InterfaceVa;

    /* Did we already have some protocols on this handle? */
    if (!HashList)
    {
        /* Nope, this is the first time -- so allocate the list */
        HashList = BlMmAllocateHeap(sizeof(*HashList));
        if (!HashList)
        {
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        /* Initialize it */
        InitializeListHead(HashList);

        /* And then store it in the hash table for this handle */
        Status = BlHtStore(EfiProtHashTableId,
                           &HashEntry,
                           HashList,
                           sizeof(*HashList));
        if (!NT_SUCCESS(Status))
        {
            BlMmFreeHeap(HashList);
            goto Quickie;
        }
    }

    /* Finally, allocate a protocol tracker structure */
    EfiProtocol = BlMmAllocateHeap(sizeof(*EfiProtocol));
    if (!EfiProtocol)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* And store this information in case the protocol is needed again */
    EfiProtocol->Protocol = Protocol;
    EfiProtocol->Interface = *Interface;
    EfiProtocol->ReferenceCount = 1;
    EfiProtocol->AddressMapped = AddressMapped;
    InsertTailList(HashList, &EfiProtocol->ListEntry);

    /* Passthru to success case */
    AddressMapped = FALSE;

Quickie:
    /* Failure path -- did we map anything ?*/
    if (AddressMapped)
    {
        /* Get rid of it */
        BlMmUnmapVirtualAddressEx(InterfaceVa, PAGE_SIZE);
        *Interface = NULL;
    }

    /* Return the failure */
    return Status;
}

NTSTATUS
EfiOpenProtocol (
    _In_ EFI_HANDLE Handle,
    _In_ EFI_GUID *Protocol,
    _Outptr_ PVOID* Interface
    )
{
    EFI_STATUS EfiStatus;
    NTSTATUS Status;
    BL_ARCH_MODE OldMode;

    /* Are we using virtual memory/ */
    if (MmTranslationType != BlNone)
    {
        /* We need complex tracking to make this work */
        Status = EfiVmOpenProtocol(Handle, Protocol, Interface);
    }
    else
    {
        /* Are we in protected mode? */
        OldMode = CurrentExecutionContext->Mode;
        if (OldMode != BlRealMode)
        {
            /* Switch to real mode */
            BlpArchSwitchContext(BlRealMode);
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
EfiVmpCloseProtocol (
    _In_ EFI_HANDLE Handle,
    _In_ EFI_GUID* Protocol
    )
{
    EFI_STATUS EfiStatus;
    BL_ARCH_MODE OldMode;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* Switch to real mode */
        BlpArchSwitchContext(BlRealMode);
    }

    /* Are we on legacy 1.02? */
    if (EfiST->FirmwareRevision == EFI_1_02_SYSTEM_TABLE_REVISION)
    {
        /* Nothing to close */
        EfiStatus = EFI_SUCCESS;
    }
    else
    {
        /* Use the UEFI version */
        EfiStatus = EfiBS->CloseProtocol(Handle,
                                         Protocol,
                                         EfiImageHandle,
                                         NULL);

        /* Normalize not found as success */
        if (EfiStatus == EFI_NOT_FOUND)
        {
            EfiStatus = EFI_SUCCESS;
        }
    }

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert to NT status */
    return EfiGetNtStatusCode(EfiStatus);
}

NTSTATUS
EfiVmpFreeInterfaceEntry (
    _In_ EFI_HANDLE Handle,
    _In_ PBL_EFI_PROTOCOL EfiProtocol
    )
{
    NTSTATUS Status;
    BL_HASH_ENTRY HashEntry;

    /* Assume success */
    Status = STATUS_SUCCESS;

    /* Is this the last protocol on this handle? */
    if (IsListEmpty(&EfiProtocol->ListEntry))
    {
        /* Delete the hash table entry for this handle */
        HashEntry.Value = Handle;
        HashEntry.Size = sizeof(Handle);
        HashEntry.Flags = BL_HT_VALUE_IS_INLINE;
        Status = BlHtDelete(EfiProtHashTableId, &HashEntry);

        /* This will free the list head itself */
        BlMmFreeHeap(EfiProtocol->ListEntry.Flink);
    }
    else
    {
        /* Simply remove this entry */
        RemoveEntryList(&EfiProtocol->ListEntry);
    }

    /* Had we virtually mapped this protocol? */
    if (EfiProtocol->AddressMapped)
    {
        /* Unmap it */
        BlMmUnmapVirtualAddressEx(EfiProtocol->Interface, PAGE_SIZE);
    }

    /* Free the protocol entry, and return */
    BlMmFreeHeap(EfiProtocol);
    return Status;
}

NTSTATUS
EfiVmCloseProtocol (
    _In_ EFI_HANDLE Handle,
    _In_ EFI_GUID* Protocol
    )
{
    BL_HASH_ENTRY HashEntry;
    PLIST_ENTRY ListHead, NextEntry;
    NTSTATUS Status, CloseStatus;
    PBL_HASH_VALUE HashValue;
    PBL_EFI_PROTOCOL EfiProtocol;

    /* Lookup the list entry for this handle */
    HashEntry.Size = sizeof(Handle);
    HashEntry.Flags = BL_HT_VALUE_IS_INLINE;
    HashEntry.Value = Handle;
    Status = BlHtLookup(EfiProtHashTableId, &HashEntry, &HashValue);
    if (!NT_SUCCESS(Status))
    {
        /* This handle was never used for any protocols  */
        return STATUS_INVALID_PARAMETER;
    }

    /* Iterate through the list of opened protocols */
    ListHead = (PLIST_ENTRY)HashValue->Data;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get this protocol entry and check for a match */
        EfiProtocol = CONTAINING_RECORD(NextEntry, BL_EFI_PROTOCOL, ListEntry);
        if (EfiProtocol->Protocol == Protocol)
        {
            /* Drop a reference -- was it the last one? */
            if (EfiProtocol->ReferenceCount-- == 1)
            {
                /* Yep -- free this entry */
                Status = EfiVmpFreeInterfaceEntry(Handle, EfiProtocol);

                /* Call firmware to close the protocol */
                CloseStatus = EfiVmpCloseProtocol(Handle, Protocol);
                if (!NT_SUCCESS(CloseStatus))
                {
                    /* Override free status if close was a failure */
                    Status = CloseStatus;
                }

                /* Return final status */
                return Status;
            }
        }

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }

    /* This protocol was never opened */
    return STATUS_INVALID_PARAMETER;
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
        Status = EfiVmCloseProtocol(Handle, Protocol);
    }
    else
    {
        /* Are we on legacy 1.02? */
        if (EfiST->FirmwareRevision == EFI_1_02_SYSTEM_TABLE_REVISION)
        {
            /* Nothing to close */
            EfiStatus = EFI_SUCCESS;
        }
        else
        {
            /* Are we in protected mode? */
            OldMode = CurrentExecutionContext->Mode;
            if (OldMode != BlRealMode)
            {
                /* Switch to real mode */
                BlpArchSwitchContext(BlRealMode);
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
EfiGetVariable (
    _In_ PWCHAR VariableName,
    _In_ EFI_GUID* VendorGuid,
    _Out_opt_ PULONG Attributes,
    _Inout_ PULONG DataSize,
    _Out_ PVOID Data
    )
{
    EFI_STATUS EfiStatus;
    NTSTATUS Status;
    BL_ARCH_MODE OldMode;
    ULONG LocalAttributes;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        EfiPrintf(L"getvar vm path\r\n");
        EfiStall(10000000);
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Call the runtime API */
    EfiStatus = EfiRT->GetVariable(VariableName,
                                   VendorGuid,
                                   (UINT32*)&LocalAttributes,
                                   (UINTN*)DataSize,
                                   Data);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Return attributes back to the caller if asked to */
    if (Attributes)
    {
        *Attributes = LocalAttributes;
    }

    /* Convert the error to an NTSTATUS and return it */
    Status = EfiGetNtStatusCode(EfiStatus);
    return Status;
}

NTSTATUS
BlpSecureBootEFIIsEnabled (
    VOID
    )
{
    NTSTATUS Status;
    BOOLEAN SetupMode, SecureBoot;
    ULONG DataSize;

    /* Assume setup mode enabled, and no secure boot */
    SecureBoot = FALSE;
    SetupMode = TRUE;

    /* Get the SetupMode variable */
    DataSize = sizeof(SetupMode);
    Status = EfiGetVariable(L"SetupMode",
                            &EfiGlobalVariable,
                            NULL,
                            &DataSize,
                            &SetupMode);
    if (NT_SUCCESS(Status))
    {
        /* If it worked, get the SecureBoot variable */
        DataSize = sizeof(SecureBoot);
        Status = EfiGetVariable(L"SecureBoot",
                                &EfiGlobalVariable,
                                NULL,
                                &DataSize,
                                &SecureBoot);
        if (NT_SUCCESS(Status))
        {
            /* In setup mode or without secureboot turned on, return failure */
            if ((SecureBoot != TRUE) || (SetupMode))
            {
                Status = STATUS_INVALID_SIGNATURE;
            }

            // BlpSbdiStateFlags |= 8u;
        }
    }

    /* Return secureboot status */
    return Status;
}

NTSTATUS
BlSecureBootIsEnabled (
    _Out_ PBOOLEAN SecureBootEnabled
    )
{
    NTSTATUS Status;

    /* Have we checked before ? */
    if (!BlpFirmwareChecked)
    {
        /* First time checking */
        Status = BlpSecureBootEFIIsEnabled();
        if NT_SUCCESS(Status)
        {
            /* Yep, it's on */
            BlpFirmwareEnabled = TRUE;
        }

        /* Don't check again */
        BlpFirmwareChecked = TRUE;
    }

    /* Return the firmware result */
    *SecureBootEnabled = BlpFirmwareEnabled;
    return STATUS_SUCCESS;
}

NTSTATUS
BlSecureBootCheckForFactoryReset (
    VOID
    )
{
    BOOLEAN SecureBootEnabled;
    NTSTATUS Status;
    ULONG DataSize;

    /* Initialize locals */
    DataSize = 0;
    SecureBootEnabled = FALSE;

    /* Check if secureboot is enabled */
    Status = BlSecureBootIsEnabled(&SecureBootEnabled);
    if (!(NT_SUCCESS(Status)) || !(SecureBootEnabled))
    {
        /* It's not. Check if there's a revocation list */
        Status = EfiGetVariable(L"RevocationList",
                                &BlpEfiSecureBootPrivateNamespace,
                                NULL,
                                &DataSize,
                                NULL);
        if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_TOO_SMALL))
        {
            /* We don't support this yet */
            EfiPrintf(L"Not yet supported\r\n");
            Status = STATUS_NOT_IMPLEMENTED;
        }
    }

    /* Return back to the caller */
    return Status;
}

NTSTATUS
EfiConInReset (
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
        EfiPrintf(L"coninreset vm path\r\n");
        EfiStall(10000000);
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Make the EFI call */
    EfiStatus = EfiConIn->Reset(EfiConIn, FALSE);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

NTSTATUS
EfiConInExReset (
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
        EfiPrintf(L"conreset vm path\r\n");
        EfiStall(10000000);
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Make the EFI call */
    EfiStatus = EfiConInEx->Reset(EfiConInEx, FALSE);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Convert the error to an NTSTATUS */
    return EfiGetNtStatusCode(EfiStatus);
}

NTSTATUS
EfiConInExSetState (
    _In_ EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *ConInEx,
    _In_ EFI_KEY_TOGGLE_STATE* KeyToggleState
    )
{
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;
    PHYSICAL_ADDRESS ConInExPhys, KeyTogglePhys;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* Translate pointers from virtual to physical */
        BlMmTranslateVirtualAddress(ConInEx, &ConInExPhys);
        ConInEx = (EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL*)PhysicalAddressToPtr(ConInExPhys);
        BlMmTranslateVirtualAddress(KeyToggleState, &KeyTogglePhys);
        KeyToggleState = (EFI_KEY_TOGGLE_STATE*)PhysicalAddressToPtr(KeyTogglePhys);

        /* Switch to real mode */
        BlpArchSwitchContext(BlRealMode);
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
        /* Switch to real mode */
        BlpArchSwitchContext(BlRealMode);
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
    PHYSICAL_ADDRESS MemoryMapSizePhysical, MemoryMapPhysical, MapKeyPhysical;
    PHYSICAL_ADDRESS DescriptorSizePhysical, DescriptorVersionPhysical;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* Convert all of the addresses to physical */
        BlMmTranslateVirtualAddress(MemoryMapSize, &MemoryMapSizePhysical);
        MemoryMapSize = (UINTN*)PhysicalAddressToPtr(MemoryMapSizePhysical);
        BlMmTranslateVirtualAddress(MemoryMap, &MemoryMapPhysical);
        MemoryMap = (EFI_MEMORY_DESCRIPTOR*)PhysicalAddressToPtr(MemoryMapPhysical);
        BlMmTranslateVirtualAddress(MapKey, &MapKeyPhysical);
        MapKey = (UINTN*)PhysicalAddressToPtr(MapKeyPhysical);
        BlMmTranslateVirtualAddress(DescriptorSize, &DescriptorSizePhysical);
        DescriptorSize = (UINTN*)PhysicalAddressToPtr(DescriptorSizePhysical);
        BlMmTranslateVirtualAddress(DescriptorVersion, &DescriptorVersionPhysical);
        DescriptorVersion = (UINTN*)PhysicalAddressToPtr(DescriptorVersionPhysical);

        /* Switch to real mode */
        BlpArchSwitchContext(BlRealMode);
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
        /* Switch to real mode */
        BlpArchSwitchContext(BlRealMode);
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
        /* Switch to real mode */
        BlpArchSwitchContext(BlRealMode);
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
        EfiPrintf(L"conqmode vm path\r\n");
        EfiStall(10000000);
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
        EfiPrintf(L"setmode vm path\r\n");
        EfiStall(10000000);
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
        EfiPrintf(L"sattr vm path\r\n");
        EfiStall(10000000);
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
        EfiPrintf(L"setcursor vm path\r\n");
        EfiStall(10000000);
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
        EfiPrintf(L"enablecurso vm path\r\n");
        EfiStall(10000000);
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

NTSTATUS
EfiConOutOutputString (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _In_ PWCHAR String
    )
{
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        EfiPrintf(L"output string vm path\r\n");
        EfiStall(10000000);
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Make the EFI call */
    EfiStatus = TextInterface->OutputString(TextInterface, String);

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
        EfiPrintf(L"readmode vm path\r\n");
        EfiStall(10000000);
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
    PHYSICAL_ADDRESS GopInterfacePhys, FrameBufferPhys, FrameBufferSizePhys;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* Translate pointer to physical */
        BlMmTranslateVirtualAddress(GopInterface, &GopInterfacePhys);
        GopInterface = PhysicalAddressToPtr(GopInterfacePhys);

        /* Translate pointer to physical */
        BlMmTranslateVirtualAddress(FrameBuffer, &FrameBufferPhys);
        FrameBuffer = PhysicalAddressToPtr(FrameBufferPhys);

        /* Translate pointer to physical */
        BlMmTranslateVirtualAddress(FrameBufferSize, &FrameBufferSizePhys);
        FrameBufferSize = PhysicalAddressToPtr(FrameBufferSizePhys);

        /* Switch to real mode */
        BlpArchSwitchContext(BlRealMode);
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
    PHYSICAL_ADDRESS GopInterfacePhys, ModePhys, InformationPhys;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* Translate pointer to physical */
        if (!BlMmTranslateVirtualAddress(GopInterface, &GopInterfacePhys))
        {
            return STATUS_UNSUCCESSFUL;
        }
        GopInterface = PhysicalAddressToPtr(GopInterfacePhys);

        /* Translate pointer to physical */
        if (!BlMmTranslateVirtualAddress(Mode, &ModePhys))
        {
            return STATUS_UNSUCCESSFUL;
        }
        Mode = PhysicalAddressToPtr(ModePhys);

        /* Translate pointer to physical */
        if (!BlMmTranslateVirtualAddress(Information, &InformationPhys))
        {
            return STATUS_UNSUCCESSFUL;
        }
        Information = PhysicalAddressToPtr(InformationPhys);

        /* Switch to real mode */
        BlpArchSwitchContext(BlRealMode);
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
        EfiPrintf(L"gopsmode vm path\r\n");
        EfiStall(10000000);
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
        EfiPrintf(L"Console video mode set to 0x%x\r\n", Mode);
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
    PVOID InputBuffer;
    BOOLEAN TranslateResult;
    PHYSICAL_ADDRESS BufferPhys;

    /* Bail out if we're missing parameters */
    if (!(Buffer) || !(HandleCount))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if a buffer was passed in*/
    InputBuffer = *Buffer;
    if (InputBuffer)
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
        /* Translate the input buffer from virtual to physical */
        TranslateResult = BlMmTranslateVirtualAddress(InputBuffer, &BufferPhys);
        InputBuffer = TranslateResult ? PhysicalAddressToPtr(BufferPhys) : NULL;

        /* Switch to real mode */
        BlpArchSwitchContext(BlRealMode);
    }

    /* Try the first time */
    EfiStatus = EfiBS->LocateHandle(SearchType,
                                    Protocol,
                                    NULL,
                                    &BufferSize,
                                    InputBuffer);

    /* Switch back to protected mode if we came from there */
    if (OldMode != BlRealMode)
    {
        BlpArchSwitchContext(OldMode);
    }

    /* Check result of first search */
    if (EfiStatus == EFI_BUFFER_TOO_SMALL)
    {
        /* Did we have an existing buffer? */
        if (*Buffer)
        {
            /* Free it */
            BlMmFreeHeap(*Buffer);
        }

        /* Allocate a new one */
        InputBuffer = BlMmAllocateHeap(BufferSize);
        *Buffer = InputBuffer;
        if (!InputBuffer)
        {
            /* No space, fail */
            return STATUS_NO_MEMORY;
        }

        if (OldMode != BlRealMode)
        {
            /* Translate the input buffer from virtual to physical */
            TranslateResult = BlMmTranslateVirtualAddress(InputBuffer,
                                                          &BufferPhys);
            InputBuffer = TranslateResult ? PhysicalAddressToPtr(BufferPhys) : NULL;

            /* Switch to real mode */
            BlpArchSwitchContext(BlRealMode);
        }

        /* Try again */
        EfiStatus = EfiBS->LocateHandle(SearchType,
                                        Protocol,
                                        NULL,
                                        &BufferSize,
                                        InputBuffer);

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
        EfiPrintf(L"reset vm path\r\n");
        EfiStall(10000000);
        return;
    }

    /* Call the EFI runtime */
    EfiRT->ResetSystem(ResetType, EFI_SUCCESS, 0, NULL);
}

NTSTATUS
EfiConnectController (
    _In_ EFI_HANDLE ControllerHandle
    )
{
    BL_ARCH_MODE OldMode;
    EFI_STATUS EfiStatus;

    /* Is this EFI 1.02? */
    if (EfiST->Hdr.Revision == EFI_1_02_SYSTEM_TABLE_REVISION)
    {
        /* This function didn't exist back then */
        return STATUS_NOT_SUPPORTED;
    }

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* FIXME: Not yet implemented */
        EfiPrintf(L"connectctrl vm path\r\n");
        EfiStall(10000000);
        return STATUS_NOT_IMPLEMENTED;
    }

    /* Make the EFI call */
    EfiStatus = EfiBS->ConnectController(ControllerHandle, NULL, NULL, TRUE);

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
    PHYSICAL_ADDRESS MemoryPhysical;

    /* Are we in protected mode? */
    OldMode = CurrentExecutionContext->Mode;
    if (OldMode != BlRealMode)
    {
        /* Translate output address */
        BlMmTranslateVirtualAddress(Memory, &MemoryPhysical);
        Memory = (EFI_PHYSICAL_ADDRESS*)PhysicalAddressToPtr(MemoryPhysical);

        /* Switch to real mode */
        BlpArchSwitchContext(BlRealMode);
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

NTSTATUS
EfipGetSystemTable (
    _In_ EFI_GUID *TableGuid,
    _Out_ PPHYSICAL_ADDRESS TableAddress
    )
{
    ULONG i;
    NTSTATUS Status;

    /* Assume failure */
    Status = STATUS_NOT_FOUND;

    /* Loop through the configuration tables */
    for (i = 0; i < EfiST->NumberOfTableEntries; i++)
    {
        /* Check if this one matches the one we want */
        if (RtlEqualMemory(&EfiST->ConfigurationTable[i].VendorGuid,
                           TableGuid,
                           sizeof(*TableGuid)))
        {
            /* Return its address */
            TableAddress->QuadPart = (ULONG_PTR)EfiST->ConfigurationTable[i].VendorTable;
            Status = STATUS_SUCCESS;
            break;
        }
    }

    /* Return the search result */
    return Status;
}

NTSTATUS
EfipGetRsdt (
    _Out_ PPHYSICAL_ADDRESS FoundRsdt
    )
{
    NTSTATUS Status;
    ULONG Length;
    PHYSICAL_ADDRESS RsdpAddress, Rsdt;
    PRSDP Rsdp;

    /* Assume failure */
    Length = 0;
    Rsdp = NULL;

    /* Check if we already know it */
    if (EfiRsdt.QuadPart)
    {
        /* Return it */
        *FoundRsdt = EfiRsdt;
        return STATUS_SUCCESS;
    }

    /* Otherwise, look for the ACPI 2.0 RSDP (XSDT really) */
    Status = EfipGetSystemTable(&EfiRootAcpiTableGuid, &RsdpAddress);
    if (!NT_SUCCESS(Status))
    {
        /* Didn't fint it, look for the ACPI 1.0 RSDP (RSDT really) */
        Status = EfipGetSystemTable(&EfiRootAcpiTable10Guid, &RsdpAddress);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Map it */
    Length = sizeof(*Rsdp);
    Status = BlMmMapPhysicalAddressEx((PVOID*)&Rsdp,
                                      0,
                                      Length,
                                      RsdpAddress);
    if (NT_SUCCESS(Status))
    {
        /* Check the revision (anything >= 2.0 is XSDT) */
        if (Rsdp->Revision)
        {
            /* Check if the table is bigger than just its header */
            if (Rsdp->Length > Length)
            {
                /* Capture the real length */
                Length = Rsdp->Length;

                /* Unmap our header mapping */
                BlMmUnmapVirtualAddressEx(Rsdp, sizeof(*Rsdp));

                /* And map the whole thing now */
                Status = BlMmMapPhysicalAddressEx((PVOID*)&Rsdp,
                                                  0,
                                                  Length,
                                                  RsdpAddress);
                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }
            }

            /* Read the XSDT address from the table*/
            Rsdt = Rsdp->XsdtAddress;
        }
        else
        {
            /* ACPI 1.0 so just read the RSDT */
            Rsdt.QuadPart = Rsdp->RsdtAddress;
        }

        /* Save it for later */
        EfiRsdt = Rsdt;

        /* And return it back */
        *FoundRsdt = Rsdt;
    }

    /* Check if we had mapped the RSDP */
    if (Rsdp)
    {
        /* Unmap it */
        BlMmUnmapVirtualAddressEx(Rsdp, Length);
    }

    /* Return search result back to caller */
    return Status;
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
            OsType = BlEfiRuntimeCodeMemory;
            break;

        case EfiRuntimeServicesData:
            OsType = BlEfiRuntimeDataMemory;
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
    ULONGLONG Pages, StartPage, EndPage, EfiBufferPage;
    UINTN EfiMemoryMapSize, MapKey, DescriptorSize, DescriptorVersion;
    EFI_PHYSICAL_ADDRESS EfiBuffer = 0;
    EFI_MEMORY_DESCRIPTOR* EfiMemoryMap;
    EFI_STATUS EfiStatus;
    BL_ARCH_MODE OldMode;
    EFI_MEMORY_DESCRIPTOR EfiDescriptor;
    BL_MEMORY_TYPE MemoryType;
    PBL_MEMORY_DESCRIPTOR Descriptor;
    BL_MEMORY_ATTR Attribute;
    PVOID LibraryBuffer;

    /* Initialize EFI memory map attributes */
    EfiMemoryMapSize = MapKey = DescriptorSize = DescriptorVersion = 0;
    LibraryBuffer = NULL;

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
        if (NT_SUCCESS(Status))
        {
            Status = STATUS_UNSUCCESSFUL;
        }
        goto Quickie;
    }

    /* Add 4 more descriptors just in case things changed */
    EfiMemoryMapSize += (4 * DescriptorSize);
    Pages = BYTES_TO_PAGES(EfiMemoryMapSize);

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
        /* Round the map to pages */
        Pages = BYTES_TO_PAGES(EfiMemoryMapSize);

        /* Allocate a large enough buffer */
        Status = MmPapAllocatePagesInRange(&LibraryBuffer,
                                           BlLoaderData,
                                           Pages,
                                           0,
                                           0,
                                           0,
                                           0);
        if (!NT_SUCCESS(Status))
        {
            EfiPrintf(L"Failed to allocate mapped VM for EFI map: %lx\r\n", Status);
            goto Quickie;
        }

        /* Call EFI to get the memory map */
        EfiMemoryMap = LibraryBuffer;
        Status = EfiGetMemoryMap(&EfiMemoryMapSize,
                                 LibraryBuffer,
                                 &MapKey,
                                 &DescriptorSize,
                                 &DescriptorVersion);
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
    EfiPrintf(L"UEFI MEMORY MAP\r\n\r\n");
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
                    /* Set the appropriate flag on the descriptor */
                    Descriptor->Flags |= BlMemoryBelow1MB;
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
            /* Set the appropriate flag on the descriptor */
            Descriptor->Flags |= BlMemoryBelow1MB;
        }

        /* Add the descriptor to the list, requesting coalescing as asked */
        Status = MmMdAddDescriptorToList(MemoryMap,
                                         Descriptor,
                                         BL_MM_ADD_DESCRIPTOR_TRUNCATE_FLAG |
                                         ((Flags & BL_MM_FLAG_REQUEST_COALESCING) ?
                                          BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG : 0));
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

    /* Check if we are using the local UEFI buffer */
    if (!UseEfiBuffer)
    {
        goto Quickie;
    }

    /* Free the EFI buffer */
    Status = EfiFreePages(Pages, EfiBuffer);
    if (!NT_SUCCESS(Status))
    {
        /* Keep the pages marked 'in use' and fake success */
        Status = STATUS_SUCCESS;
        goto Quickie;
    }

    /* Get the base page of the EFI buffer */
    EfiBufferPage = EfiBuffer >> PAGE_SHIFT;
    Pages = (EfiBufferPage + Pages) - EfiBufferPage;

    /* Don't try freeing below */
    EfiBuffer = 0;

    /* Find the current descriptor for the allocation */
    Descriptor = MmMdFindDescriptorFromMdl(MemoryMap,
                                           BL_MM_REMOVE_PHYSICAL_REGION_FLAG,
                                           EfiBufferPage);
    if (!Descriptor)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Quickie;
    }

    /* Convert it to a free descriptor */
    Descriptor = MmMdInitByteGranularDescriptor(Descriptor->Flags,
                                                BlConventionalMemory,
                                                EfiBufferPage,
                                                0,
                                                Pages);
    if (!Descriptor)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quickie;
    }

    /* Remove the region from the memory map */
    Status = MmMdRemoveRegionFromMdlEx(MemoryMap,
                                       BL_MM_REMOVE_PHYSICAL_REGION_FLAG,
                                       EfiBufferPage,
                                       Pages,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        MmMdFreeDescriptor(Descriptor);
        goto Quickie;
    }

    /* Add it back as free memory */
    Status = MmMdAddDescriptorToList(MemoryMap,
                                     Descriptor,
                                     BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG);

Quickie:
    /* Free the EFI buffer, if we had one */
    if (EfiBuffer != 0)
    {
        EfiFreePages(Pages, EfiBuffer);
    }

    /* Free the library-allocated buffer, if we had one */
    if (LibraryBuffer != 0)
    {
        MmPapFreePages(LibraryBuffer, BL_MM_INCLUDE_MAPPED_ALLOCATED);
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

    /* Check if we have valid firmware data */
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
        else if (FirmwareData->Version >= BL_FIRMWARE_DESCRIPTOR_VERSION)
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

NTSTATUS
BlFwGetParameters (
    _In_ PBL_FIRMWARE_DESCRIPTOR Parameters
    )
{
    /* Make sure we got an argument */
    if (!Parameters)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Copy the static data */
    *Parameters = *EfiFirmwareParameters;
    return STATUS_SUCCESS;
}

NTSTATUS
BlFwEnumerateDevice (
    _In_ PBL_DEVICE_DESCRIPTOR Device
    )
{
    NTSTATUS Status;
    ULONG PathProtocols, BlockProtocols;
    EFI_HANDLE* PathArray;
    EFI_HANDLE* BlockArray;

    /* Initialize locals */
    BlockArray = NULL;
    PathArray = NULL;
    PathProtocols = 0;
    BlockProtocols = 0;

    /* Enumeration only makes sense on disks or partitions */
    if ((Device->DeviceType != DiskDevice) &&
        (Device->DeviceType != LegacyPartitionDevice) &&
        (Device->DeviceType != PartitionDevice))
    {
        return STATUS_NOT_SUPPORTED;
    }

    /* Enumerate the list of device paths */
    Status = EfiLocateHandleBuffer(ByProtocol,
                                   &EfiDevicePathProtocol,
                                   &PathProtocols,
                                   &PathArray);
    if (NT_SUCCESS(Status))
    {
        /* Loop through each one */
        Status = STATUS_NOT_FOUND;
        while (PathProtocols)
        {
            /* Attempt to connect the driver for this device epath */
            Status = EfiConnectController(PathArray[--PathProtocols]);
            if (NT_SUCCESS(Status))
            {
                /* Now enumerate any block I/O devices the driver added */
                Status = EfiLocateHandleBuffer(ByProtocol,
                                               &EfiBlockIoProtocol,
                                               &BlockProtocols,
                                               &BlockArray);
                if (!NT_SUCCESS(Status))
                {
                    break;
                }

                /* Loop through each one */
                while (BlockProtocols)
                {
                    /* Check if one of the new devices is the one we want */
                    Status = BlockIoEfiCompareDevice(Device,
                                                     BlockArray[--BlockProtocols]);
                    if (NT_SUCCESS(Status))
                    {
                        /* Yep, all done */
                        goto Quickie;
                    }
                }

                /* Move on to the next device path */
                BlMmFreeHeap(BlockArray);
                BlockArray = NULL;
            }
        }
    }

Quickie:
    /* We're done -- free the array of device path protocols, if any */
    if (PathArray)
    {
        BlMmFreeHeap(PathArray);
    }

    /* We're done -- free the array of block I/O protocols, if any */
    if (BlockArray)
    {
        BlMmFreeHeap(BlockArray);
    }

    /* Return if we found the device or not */
    return Status;
}

/*++
 * @name EfiGetEfiStatusCode
 *
 *     The EfiGetEfiStatusCode routine converts an NT Status to an EFI status.
 *
 * @param  Status
 *         NT Status code to be converted.
 *
 * @remark Only certain, specific NT status codes are converted to EFI codes.
 *
 * @return The corresponding EFI Status code, EFI_NO_MAPPING otherwise.
 *
 *--*/
EFI_STATUS
EfiGetEfiStatusCode(
    _In_ NTSTATUS Status
    )
{
    switch (Status)
    {
        case STATUS_NOT_SUPPORTED:
            return EFI_UNSUPPORTED;
        case STATUS_DISK_FULL:
            return EFI_VOLUME_FULL;
        case STATUS_INSUFFICIENT_RESOURCES:
            return EFI_OUT_OF_RESOURCES;
        case STATUS_MEDIA_WRITE_PROTECTED:
            return EFI_WRITE_PROTECTED;
        case STATUS_DEVICE_NOT_READY:
            return EFI_NOT_STARTED;
        case STATUS_DEVICE_ALREADY_ATTACHED:
            return EFI_ALREADY_STARTED;
        case STATUS_MEDIA_CHANGED:
            return EFI_MEDIA_CHANGED;
        case STATUS_INVALID_PARAMETER:
            return EFI_INVALID_PARAMETER;
        case STATUS_ACCESS_DENIED:
            return EFI_ACCESS_DENIED;
        case STATUS_BUFFER_TOO_SMALL:
            return EFI_BUFFER_TOO_SMALL;
        case STATUS_DISK_CORRUPT_ERROR:
            return EFI_VOLUME_CORRUPTED;
        case STATUS_REQUEST_ABORTED:
            return EFI_ABORTED;
        case STATUS_NO_MEDIA:
            return EFI_NO_MEDIA;
        case STATUS_IO_DEVICE_ERROR:
            return EFI_DEVICE_ERROR;
        case STATUS_INVALID_BUFFER_SIZE:
            return EFI_BAD_BUFFER_SIZE;
        case STATUS_NOT_FOUND:
            return EFI_NOT_FOUND;
        case STATUS_DRIVER_UNABLE_TO_LOAD:
            return EFI_LOAD_ERROR;
        case STATUS_NO_MATCH:
            return EFI_NO_MAPPING;
        case STATUS_SUCCESS:
            return EFI_SUCCESS;
        case STATUS_TIMEOUT:
            return EFI_TIMEOUT;
        default:
            return EFI_NO_MAPPING;
    }
}

/*++
 * @name EfiGetNtStatusCode
 *
 *     The EfiGetNtStatusCode routine converts an EFI Status to an NT status.
 *
 * @param  EfiStatus
 *         EFI Status code to be converted.
 *
 * @remark Only certain, specific EFI status codes are converted to NT codes.
 *
 * @return The corresponding NT Status code, STATUS_UNSUCCESSFUL otherwise.
 *
 *--*/
NTSTATUS
EfiGetNtStatusCode (
    _In_ EFI_STATUS EfiStatus
    )
{
    switch (EfiStatus)
    {
        case EFI_NOT_READY:
        case EFI_NOT_FOUND:
            return STATUS_NOT_FOUND;
        case EFI_NO_MEDIA:
            return STATUS_NO_MEDIA;
        case EFI_MEDIA_CHANGED:
            return STATUS_MEDIA_CHANGED;
        case EFI_ACCESS_DENIED:
        case EFI_SECURITY_VIOLATION:
            return STATUS_ACCESS_DENIED;
        case EFI_TIMEOUT:
        case EFI_NO_RESPONSE:
            return STATUS_TIMEOUT;
        case EFI_NO_MAPPING:
            return STATUS_NO_MATCH;
        case EFI_NOT_STARTED:
            return STATUS_DEVICE_NOT_READY;
        case EFI_ALREADY_STARTED:
            return STATUS_DEVICE_ALREADY_ATTACHED;
        case EFI_ABORTED:
            return STATUS_REQUEST_ABORTED;
        case EFI_VOLUME_FULL:
            return STATUS_DISK_FULL;
        case EFI_DEVICE_ERROR:
            return STATUS_IO_DEVICE_ERROR;
        case EFI_WRITE_PROTECTED:
            return STATUS_MEDIA_WRITE_PROTECTED;
        /* @FIXME: ReactOS Headers don't yet have this */
        //case EFI_OUT_OF_RESOURCES:
            //return STATUS_INSUFFICIENT_NVRAM_RESOURCES;
        case EFI_VOLUME_CORRUPTED:
            return STATUS_DISK_CORRUPT_ERROR;
        case EFI_BUFFER_TOO_SMALL:
            return STATUS_BUFFER_TOO_SMALL;
        case EFI_SUCCESS:
            return STATUS_SUCCESS;
        case  EFI_LOAD_ERROR:
            return STATUS_DRIVER_UNABLE_TO_LOAD;
        case EFI_INVALID_PARAMETER:
            return STATUS_INVALID_PARAMETER;
        case EFI_UNSUPPORTED:
            return STATUS_NOT_SUPPORTED;
        case EFI_BAD_BUFFER_SIZE:
            return STATUS_INVALID_BUFFER_SIZE;
        default:
            return STATUS_UNSUCCESSFUL;
    }
}
