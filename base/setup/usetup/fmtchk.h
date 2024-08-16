/*
 * PROJECT:     ReactOS text-mode setup
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Filesystem Format and ChkDsk support functions
 * COPYRIGHT:   Copyright 2003 Casper S. Hornstrup <chorns@users.sourceforge.net>
 *              Copyright 2006 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

VOID
StartFormat(
    _Inout_ PFORMAT_VOLUME_INFO FmtInfo,
    _In_ PFILE_SYSTEM_ITEM SelectedFileSystem);

VOID
EndFormat(
    _In_ NTSTATUS Status);

VOID
StartCheck(
    _Inout_ PCHECK_VOLUME_INFO ChkInfo);

VOID
EndCheck(
    _In_ NTSTATUS Status);

/* EOF */
