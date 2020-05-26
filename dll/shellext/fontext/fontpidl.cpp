/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     pidl handling
 * COPYRIGHT:   Copyright 2019,2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

LPITEMIDLIST _ILCreate(LPCWSTR lpString, ULONG Index)
{
    // Because the FontPidlEntry contains one WCHAR, we do not need to take the null terminator into account
    size_t cbData = sizeof(FontPidlEntry) + wcslen(lpString) * sizeof(WCHAR);
    FontPidlEntry* pidl = (FontPidlEntry*)CoTaskMemAlloc(cbData + sizeof(WORD));
    if (!pidl)
        return NULL;

    ZeroMemory(pidl, cbData + sizeof(WORD));

    pidl->cb = (WORD)cbData;
    pidl->Magic = 'fp';
    pidl->Index = Index;

    wcscpy(pidl->Name, lpString);
    // Should be zero already, but make sure it is
    *(WORD*)((char*)pidl + cbData) = 0;

    return (LPITEMIDLIST)pidl;
}


const FontPidlEntry* _FontFromIL(LPCITEMIDLIST pidl)
{
    const FontPidlEntry* fontEntry = (const FontPidlEntry*)pidl;
    if (fontEntry->Magic == 'fp')
        return fontEntry;
    return NULL;
}
