/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Bootloader support functions
 * COPYRIGHT:   Copyright 2017-2024 Hermès Bélusca-Maïto <hermes.belusca@sfr.fr>
 */

#pragma once

NTSTATUS
InstallBootManagerAndBootEntries(
    _In_ PUSETUP_DATA pSetupData,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING SystemRootPath,
    /**/_In_opt_ PPARTENTRY SystemPartition,/**/ // FIXME: Redundant param.
    _In_ PCUNICODE_STRING DestinationArcPath,
    _In_ ULONG_PTR Options);

NTSTATUS
InstallBootcodeToRemovable(
    _In_ PUSETUP_DATA pSetupData,
    _In_ PCUNICODE_STRING RemovableRootPath, // == SystemRootPath
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING DestinationArcPath,
    _In_ PCWSTR FileSystemName);

/* EOF */
