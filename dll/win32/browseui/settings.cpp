/*
 * PROJECT:     ReactOS browseui
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Stores settings for the file explorer UI.
 * COPYRIGHT:   Copyright 2023 Carl Bialorucki <cbialo2@outlook.com>
 */

#include "precomp.h"

CabinetStateSettings gCabinetState;

void ShellSettings::Save()
{
    SHRegSetUSValueW(L"Software\\Microsoft\\Internet Explorer\\Main", L"StatusBarOther",
                     REG_DWORD, &fStatusBarVisible, sizeof(fStatusBarVisible), SHREGSET_FORCE_HKCU);

    SHRegSetUSValueW(L"Software\\Microsoft\\Internet Explorer\\Main", L"ShowGoButton",
                     REG_DWORD, &fShowGoButton, sizeof(fShowGoButton), SHREGSET_FORCE_HKCU);

    SHRegSetUSValueW(L"Software\\Microsoft\\Internet Explorer\\Toolbar", L"Locked",
                     REG_DWORD, &fLocked, sizeof(fLocked), SHREGSET_FORCE_HKCU);
}

void ShellSettings::Load()
{
    fStatusBarVisible = SHRegGetBoolUSValueW(L"Software\\Microsoft\\Internet Explorer\\Main",
                                             L"StatusBarOther", FALSE, FALSE);

    fShowGoButton = SHRegGetBoolUSValueW(L"Software\\Microsoft\\Internet Explorer\\Main",
                                         L"ShowGoButton", FALSE, TRUE);

    fLocked = SHRegGetBoolUSValueW(L"Software\\Microsoft\\Internet Explorer\\Toolbar",
                                   L"Locked", FALSE, TRUE);
}

void CabinetStateSettings::Load()
{
    ReadCabinetState(this, this->cLength);

    /* Overrides */
    fFullPathTitle = SHRegGetBoolUSValueW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CabinetState",
                                          L"FullPath", FALSE, FALSE);

    fFullPathAddress = SHRegGetBoolUSValueW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CabinetState",
                                            L"FullPathAddress", FALSE, TRUE);
}
