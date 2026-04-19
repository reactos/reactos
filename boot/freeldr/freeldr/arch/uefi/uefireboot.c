/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Request firmware setup menu on next boot
 * COPYRIGHT:   Copyright 2026 Ahmed Arif <arif.img@outlook.com>
 */

#include <uefildr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

extern EFI_SYSTEM_TABLE *GlobalSystemTable;

#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI    0x0000000000000001ULL

#define EFI_OS_INDICATIONS_ATTRIBUTES       \
    (EFI_VARIABLE_NON_VOLATILE |            \
     EFI_VARIABLE_BOOTSERVICE_ACCESS |      \
     EFI_VARIABLE_RUNTIME_ACCESS)

static EFI_GUID EfiGlobalVariableGuid = EFI_GLOBAL_VARIABLE;

BOOLEAN
UefiFirmwareSetupSupported(VOID)
{
    EFI_STATUS Status;
    UINT64 OsIndicationsSupported = 0;
    UINTN Size = sizeof(OsIndicationsSupported);

    Status = GlobalSystemTable->RuntimeServices->GetVariable(
        EFI_OS_INDICATIONS_SUPPORT_VARIABLE_NAME,
        &EfiGlobalVariableGuid,
        NULL,
        &Size,
        &OsIndicationsSupported);
    if (Status != EFI_SUCCESS)
    {
        WARN("Failed to query firmware setup support, status 0x%Ix\n",
             Status);
        return FALSE;
    }

    if (Size != sizeof(OsIndicationsSupported))
    {
        WARN("Firmware setup support variable has unexpected size %Iu\n",
             Size);
        return FALSE;
    }

    return !!(OsIndicationsSupported & EFI_OS_INDICATIONS_BOOT_TO_FW_UI);
}

static EFI_STATUS
UefiSetFirmwareSetupBoot(VOID)
{
    EFI_STATUS Status;
    UINT64 OsIndications = 0;
    UINTN Size = sizeof(OsIndications);

    Status = GlobalSystemTable->RuntimeServices->GetVariable(
        EFI_OS_INDICATIONS_VARIABLE_NAME,
        &EfiGlobalVariableGuid,
        NULL,
        &Size,
        &OsIndications);
    if (Status == EFI_NOT_FOUND)
    {
        OsIndications = 0;
    }
    else
    {
        if (Status != EFI_SUCCESS)
        {
            WARN("Failed to query firmware setup request variable, status 0x%Ix\n",
                 Status);
            return Status;
        }

        if (Size != sizeof(OsIndications))
        {
            WARN("Firmware setup request variable has unexpected size %Iu\n",
                 Size);
            return EFI_INVALID_PARAMETER;
        }
    }

    OsIndications |= EFI_OS_INDICATIONS_BOOT_TO_FW_UI;

    return GlobalSystemTable->RuntimeServices->SetVariable(
        EFI_OS_INDICATIONS_VARIABLE_NAME,
        &EfiGlobalVariableGuid,
        EFI_OS_INDICATIONS_ATTRIBUTES,
        sizeof(OsIndications),
        &OsIndications);
}

VOID
UefiBootToFirmware(VOID)
{
    EFI_STATUS Status;

    if (!UefiFirmwareSetupSupported())
    {
        UiMessageBox("Firmware setup is not supported by this system.");
        return;
    }

    Status = UefiSetFirmwareSetupBoot();
    if (Status != EFI_SUCCESS)
    {
        WARN("Failed to request firmware setup, status 0x%Ix\n",
             Status);
        UiMessageBox("Unable to request firmware setup.");
        return;
    }

    GlobalSystemTable->RuntimeServices->ResetSystem(
        EfiResetCold, EFI_SUCCESS, 0, NULL);

    UiMessageBox("Unable to reboot into firmware setup.");
}
