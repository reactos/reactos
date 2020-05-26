/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     pidl handling
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once

#include <pshpack1.h>
struct FontPidlEntry
{
    WORD cb;
    WORD Magic;
    ULONG Index;        // Informative only

    WCHAR Name[1];
};
#include <poppack.h>


LPITEMIDLIST _ILCreate(LPCWSTR lpString, ULONG Index);
const FontPidlEntry* _FontFromIL(LPCITEMIDLIST pidl);

