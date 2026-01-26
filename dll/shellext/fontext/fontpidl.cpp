/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     pidl handling
 * COPYRIGHT:   Copyright 2019,2020 Mark Jansen <mark.jansen@reactos.org>
 *              Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(fontext);

#define FONTPIDL_MAGIC 0x7066 // 'fp'
#define ALIGN_DWORD(size) (((size) + 3) & ~3)

LPITEMIDLIST _ILCreate(LPCWSTR lpName, LPCWSTR lpFileName)
{
    ATLASSERT(lpName);
    ATLASSERT(lpFileName);
    ATLASSERT(lpFileName[0]);

    if (!lpFileName[0])
        return NULL;

    // SECURITY: Check string length
    HRESULT hr;
    size_t cbName, cbFileName;
    hr = StringCbLengthW(lpName, STRSAFE_MAX_CCH * sizeof(WCHAR), &cbName);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;
    hr = StringCbLengthW(lpFileName, STRSAFE_MAX_CCH * sizeof(WCHAR), &cbFileName);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;
    cbName += sizeof(UNICODE_NULL);
    cbFileName += sizeof(UNICODE_NULL);

    size_t ibName = ALIGN_DWORD(sizeof(FontPidlEntry));
    size_t ibFileName = ALIGN_DWORD(ibName + cbName);
    size_t cbData = ibFileName + cbFileName;
    if (cbData > MAXWORD - sizeof(WORD))
    {
        ATLASSERT(FALSE);
        return NULL;
    }

    FontPidlEntry* pidl = (FontPidlEntry*)CoTaskMemAlloc(cbData + sizeof(WORD));
    if (!pidl)
    {
        ERR("Out of memory\n");
        return NULL;
    }

    pidl->cb = (WORD)cbData;
    pidl->Magic = FONTPIDL_MAGIC;
    pidl->ibName = (WORD)ibName;
    pidl->ibFileName = (WORD)ibFileName;

    // SECURITY: Copy strings
    hr = StringCbCopyW(pidl->Name(), cbName, lpName);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        CoTaskMemFree(pidl);
        return NULL;
    }
    hr = StringCbCopyW(pidl->FileName(), cbFileName, lpFileName);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        CoTaskMemFree(pidl);
        return NULL;
    }

    *(PWORD)((PBYTE)pidl + cbData) = UNICODE_NULL;

    return (LPITEMIDLIST)pidl;
}

const FontPidlEntry* _FontFromIL(LPCITEMIDLIST pidl)
{
    if (!pidl || pidl->mkid.cb < sizeof(FontPidlEntry))
        return NULL;

    const FontPidlEntry* fontEntry = (const FontPidlEntry*)pidl;
    if (fontEntry->Magic != FONTPIDL_MAGIC)
        return NULL;

    // SECURITY: Check boundary
    if (fontEntry->ibName < sizeof(FontPidlEntry) || fontEntry->ibFileName < sizeof(FontPidlEntry))
    {
        ERR("Boundary\n");
        return NULL;
    }
    if (fontEntry->ibName >= fontEntry->cb || fontEntry->ibFileName >= fontEntry->cb)
    {
        ERR("Boundary\n");
        return NULL;
    }

    // SECURITY: Check alignment
    if (fontEntry->ibName % sizeof(WCHAR) != 0 || fontEntry->ibFileName % sizeof(WCHAR) != 0)
    {
        ERR("Alignment\n");
        return NULL;
    }

    // SECURITY: Check null termination
    size_t cbName, cbNameMax = fontEntry->cb - fontEntry->ibName;
    if (FAILED_UNEXPECTEDLY(StringCbLengthW(fontEntry->Name(), cbNameMax, &cbName)))
        return NULL;
    size_t cbFileName, cbFileNameMax = fontEntry->cb - fontEntry->ibFileName;
    if (FAILED_UNEXPECTEDLY(StringCbLengthW(fontEntry->FileName(), cbFileNameMax, &cbFileName)))
        return NULL;

    return fontEntry;
}
