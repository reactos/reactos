/*
 *	IParseDisplayName implementation for DEVENUM.dll
 *
 * Copyright (C) 2002 Robert Shearman
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES ON THIS FILE:
 * - Implements IParseDisplayName interface which creates a moniker
 *   from a string in a special format
 */
#include "devenum_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(devenum);

static HRESULT WINAPI DEVENUM_IParseDisplayName_QueryInterface(
    LPPARSEDISPLAYNAME iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IParseDisplayName))
    {
	*ppvObj = (LPVOID)iface;
	IParseDisplayName_AddRef(iface);
	return S_OK;
    }

    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

/**********************************************************************
 * DEVENUM_IParseDisplayName_AddRef (also IUnknown)
 */
static ULONG WINAPI DEVENUM_IParseDisplayName_AddRef(LPPARSEDISPLAYNAME iface)
{
    TRACE("\n");

    DEVENUM_LockModule();

    return 2; /* non-heap based object */
}

/**********************************************************************
 * DEVENUM_IParseDisplayName_Release (also IUnknown)
 */
static ULONG WINAPI DEVENUM_IParseDisplayName_Release(LPPARSEDISPLAYNAME iface)
{
    TRACE("\n");

    DEVENUM_UnlockModule();

    return 1; /* non-heap based object */
}

/**********************************************************************
 * DEVENUM_IParseDisplayName_ParseDisplayName
 *
 *  Creates a moniker referenced to by the display string argument
 *
 * POSSIBLE BUGS:
 *  Might not handle more complicated strings properly (ie anything
 *  not in "@device:sw:{CLSID1}\<filter name or CLSID>" format
 */
static HRESULT WINAPI DEVENUM_IParseDisplayName_ParseDisplayName(
    LPPARSEDISPLAYNAME iface,
    IBindCtx *pbc,
    LPOLESTR pszDisplayName,
    ULONG *pchEaten,
    IMoniker **ppmkOut)
{
    LPOLESTR pszBetween = NULL;
    LPOLESTR pszClass = NULL;
    IEnumMoniker * pEm = NULL;
    MediaCatMoniker * pMoniker = NULL;
    CLSID clsidDevice;
    HRESULT res = S_OK;

    TRACE("(%p, %s, %p, %p)\n", pbc, debugstr_w(pszDisplayName), pchEaten, ppmkOut);

    *ppmkOut = NULL;
    if (pchEaten)
        *pchEaten = strlenW(pszDisplayName);

    pszDisplayName = strchrW(pszDisplayName, '{');
    pszBetween = strchrW(pszDisplayName, '}') + 2;

    /* size = pszBetween - pszDisplayName - 1 (for '\\' after CLSID)
     * + 1 (for NULL character)
     */
    pszClass = CoTaskMemAlloc((int)(pszBetween - pszDisplayName) * sizeof(WCHAR));
    if (!pszClass)
        return E_OUTOFMEMORY;

    strncpyW(pszClass, pszDisplayName, (int)(pszBetween - pszDisplayName) - 1);
    pszClass[(int)(pszBetween - pszDisplayName) - 1] = 0;

    TRACE("Device CLSID: %s\n", debugstr_w(pszClass));

    res = CLSIDFromString(pszClass, &clsidDevice);

    if (SUCCEEDED(res))
    {
        res = DEVENUM_ICreateDevEnum_CreateClassEnumerator((ICreateDevEnum *)(char*)&DEVENUM_CreateDevEnum, &clsidDevice, &pEm, 0);
        if (res == S_FALSE) /* S_FALSE means no category */
            res = MK_E_NOOBJECT;
    }

    if (SUCCEEDED(res))
    {
        pMoniker = DEVENUM_IMediaCatMoniker_Construct();
        if (pMoniker)
        {
            if (RegCreateKeyW(((EnumMonikerImpl *)pEm)->hkey,
                               pszBetween,
                               &pMoniker->hkey) == ERROR_SUCCESS)
                *ppmkOut = (LPMONIKER)pMoniker;
            else
            {
                IMoniker_Release((LPMONIKER)pMoniker);
                res = MK_E_NOOBJECT;
            }
        }
    }

    if (pEm)
        IEnumMoniker_Release(pEm);

    if (pszClass)
        CoTaskMemFree(pszClass);

    TRACE("-- returning: %lx\n", res);
    return res;
}

/**********************************************************************
 * IParseDisplayName_Vtbl
 */
static IParseDisplayNameVtbl IParseDisplayName_Vtbl =
{
    DEVENUM_IParseDisplayName_QueryInterface,
    DEVENUM_IParseDisplayName_AddRef,
    DEVENUM_IParseDisplayName_Release,
    DEVENUM_IParseDisplayName_ParseDisplayName
};

/* The one instance of this class */
ParseDisplayNameImpl DEVENUM_ParseDisplayName = { &IParseDisplayName_Vtbl };
