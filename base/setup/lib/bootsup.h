/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Bootloader support functions
 * COPYRIGHT:   Copyright 2017-2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

NTSTATUS
InstallBootManagerAndBootEntries(
    _In_ ARCHITECTURE_TYPE ArchType,
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING DestinationArcPath,
    _In_ ULONG_PTR Options);

NTSTATUS
InstallBootcodeToRemovable(
    _In_ ARCHITECTURE_TYPE ArchType,
    _In_ PCUNICODE_STRING RemovableRootPath,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING DestinationArcPath);

/* EOF */
