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

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <t2embapi.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(t2embed);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}

LONG WINAPI TTLoadEmbeddedFont(HANDLE *phFontReference, ULONG ulFlags,
                               ULONG *pulPrivStatus, ULONG ulPrivs,
                               ULONG *pulStatus, READEMBEDPROC lpfnReadFromStream,
                               LPVOID lpvReadStream, LPWSTR szWinFamilyName,
                               LPSTR szMacFamilyName, TTLOADINFO *pTTLoadInfo)
{
    FIXME("(%p 0x%08x %p 0x%08x %p %p %p %s %s %p) stub\n", phFontReference,
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
    FIXME("(%p 0x%08x 0x%08x %p %p %p %p %p %u %u %p) stub\n", hDC,
          ulFlags, ulCharSet, pulPrivStatus, pulStatus, lpfnWriteToStream,
          lpvWriteStream, pusCharCodeSet, usCharCodeCount, usLanguage,
          pTTEmbedInfo);

    return E_API_NOTIMPL;
}

LONG WINAPI TTGetEmbeddingType(HDC hDC, ULONG *status)
{
    FIXME("(%p %p) stub\n", hDC, status);
    if (status) *status = EMBED_NOEMBEDDING;
    return E_API_NOTIMPL;
}

LONG WINAPI TTIsEmbeddingEnabled(HDC hDC, BOOL *enabled)
{
    FIXME("(%p %p) stub\n", hDC, enabled);
    if (enabled) *enabled = FALSE;
    return E_API_NOTIMPL;
}

LONG WINAPI TTDeleteEmbeddedFont(HANDLE hFontReference, ULONG flags, ULONG *status)
{
    FIXME("(%p 0x%08x %p) stub\n", hFontReference, flags, status);
    return E_API_NOTIMPL;
}
