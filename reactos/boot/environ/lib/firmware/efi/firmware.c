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

