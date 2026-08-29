/*
 *
 * Copyright 2009 Austin English
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "t2embapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(t2embed);

LONG WINAPI TTLoadEmbeddedFont(HANDLE *phFontReference, ULONG ulFlags,
                               ULONG *pulPrivStatus, ULONG ulPrivs,
                               ULONG *pulStatus, READEMBEDPROC lpfnReadFromStream,
                               LPVOID lpvReadStream, LPWSTR szWinFamilyName,
                               LPSTR szMacFamilyName, TTLOADINFO *pTTLoadInfo)
{
    FIXME("%p, %#lx, %p, %lu, %p, %p, %p, %s, %s, %p stub\n", phFontReference,
          ulFlags, pulPrivStatus, ulPrivs, pulStatus, lpfnReadFromStream,
          lpvReadStream, debugstr_w(szWinFamilyName), szMacFamilyName,
          pTTLoadInfo);

    return E_API_NOTIMPL;
}

LONG WINAPI TTEmbedFont(HDC hDC, ULONG ulFlags, ULONG ulCharSet, ULONG *pulPrivStatus,
                         ULONG *pulStatus, WRITEEMBEDPROC lpfnWriteToStream, LPVOID lpvWriteStream,
                         USHORT *pusCharCodeSet, USHORT usCharCodeCount, USHORT usLanguage,
                         TTEMBEDINFO *pTTEmbedInfo)
{
    FIXME("%p, %#lx, %lu, %p, %p, %p, %p, %p, %u, %u, %p stub\n", hDC,
          ulFlags, ulCharSet, pulPrivStatus, pulStatus, lpfnWriteToStream,
          lpvWriteStream, pusCharCodeSet, usCharCodeCount, usLanguage,
          pTTEmbedInfo);

    return E_API_NOTIMPL;
}

LONG WINAPI TTGetEmbeddingType(HDC hDC, ULONG *status)
{
    OUTLINETEXTMETRICW otm;
    WORD fsType;

    TRACE("(%p %p)\n", hDC, status);

    if (!hDC)
        return E_HDCINVALID;

    otm.otmSize = sizeof(otm);
    if (!GetOutlineTextMetricsW(hDC, otm.otmSize, &otm))
        return E_NOTATRUETYPEFONT;

    if (!status)
        return E_PERMISSIONSINVALID;

    otm.otmfsType = (fsType = otm.otmfsType) & 0xf;
    if (otm.otmfsType == LICENSE_INSTALLABLE)
        *status = EMBED_INSTALLABLE;
    else if (otm.otmfsType & LICENSE_EDITABLE)
        *status = EMBED_EDITABLE;
    else if (otm.otmfsType & LICENSE_PREVIEWPRINT)
        *status = EMBED_PREVIEWPRINT;
    else if (otm.otmfsType & LICENSE_NOEMBEDDING)
        *status = EMBED_NOEMBEDDING;
    else
    {
        WARN("unrecognized flags, %#x\n", otm.otmfsType);
        *status = EMBED_INSTALLABLE;
    }

    TRACE("fsType 0x%04x, status %lu\n", fsType, *status);
    return E_NONE;
}

LONG WINAPI TTIsEmbeddingEnabledForFacename(LPCSTR facename, BOOL *enabled)
{
    DWORD index;
    HKEY hkey;
    LONG ret;

    TRACE("(%s %p)\n", debugstr_a(facename), enabled);

    if (!facename)
        return E_FACENAMEINVALID;

    if (!enabled)
        return E_PBENABLEDINVALID;

    *enabled = TRUE;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Shared Tools\\t2embed", 0, GENERIC_READ, &hkey))
        goto out;

    *enabled = TRUE;
    ret = ERROR_SUCCESS;
    index = 0;
    while (ret != ERROR_NO_MORE_ITEMS)
    {
        DWORD name_len, value_len, value, type;
        CHAR name[LF_FACESIZE];

        name_len = ARRAY_SIZE(name);
        value_len = sizeof(value);
        ret = RegEnumValueA(hkey, index++, name, &name_len, NULL, &type, (BYTE*)&value, &value_len);
        if (ret || type != REG_DWORD)
            continue;

        if (!lstrcmpiA(name, facename))
        {
            *enabled = !!value;
            break;
        }
    }
    RegCloseKey(hkey);

out:
    TRACE("embedding %s for %s\n", *enabled ? "enabled" : "disabled", debugstr_a(facename));
    return E_NONE;
}

LONG WINAPI TTIsEmbeddingEnabled(HDC hDC, BOOL *enabled)
{
    OUTLINETEXTMETRICA *otm;
    LONG ret;
    UINT len;

    TRACE("(%p %p)\n", hDC, enabled);

    if (!hDC)
        return E_HDCINVALID;

    len = GetOutlineTextMetricsA(hDC, 0, NULL);
    if (!len)
        return E_ERRORACCESSINGFACENAME;

    otm = HeapAlloc(GetProcessHeap(), 0, len);
    if (!otm)
        return E_NOFREEMEMORY;

    GetOutlineTextMetricsA(hDC, len, otm);
    ret = TTIsEmbeddingEnabledForFacename((LPCSTR)otm + (ULONG_PTR)otm->otmpFaceName, enabled);
    HeapFree(GetProcessHeap(), 0, otm);
    return ret;
}

LONG WINAPI TTDeleteEmbeddedFont(HANDLE hFontReference, ULONG flags, ULONG *status)
{
    FIXME("%p, %#lx, %p stub\n", hFontReference, flags, status);
    return E_API_NOTIMPL;
}
