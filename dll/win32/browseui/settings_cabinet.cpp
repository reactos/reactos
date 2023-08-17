/*
 * PROJECT:     ReactOS browseui
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Stores the global cabinet state for the file explorer UI.
 * COPYRIGHT:   Copyright 2023 Carl Bialorucki <cbialo2@outlook.com>
 */

#include "precomp.h"

CabinetStateSettings gCabinetState;

BOOL CabinetStateSettings::Load()
{
    /* Ideally we would use ReadCabinetState, but unfortunately Wine's implementation is incomplete. */
    fFullPathTitle = SHRegGetBoolUSValueW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CabinetState",
                                          L"FullPath", FALSE, FALSE);

    fFullPathAddress = SHRegGetBoolUSValueW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CabinetState",
                                            L"FullPathAddress", FALSE, TRUE);

    return TRUE;
}