/*
 * PROJECT:     ReactOS browseui
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Settings header file
 * COPYRIGHT:   Copyright 2023 Carl Bialorucki <cbialo2@outlook.com>
 */

#include "precomp.h"

#define BWM_SETTINGCHANGE  (WM_USER + 300)
#define BWM_GETSETTINGSPTR (WM_USER + 301)

struct ShellSettings
{
    BOOL fLocked = TRUE;
    BOOL fShowGoButton = TRUE;
    BOOL fStatusBarVisible = TRUE;

    void Save();
    void Load();
    void Reset() { ShellSettings().Save(); }
};

struct CabinetStateSettings : CABINETSTATE
{
    BOOL fFullPathAddress = TRUE;

    void Load();
};

extern CabinetStateSettings gCabinetState;
