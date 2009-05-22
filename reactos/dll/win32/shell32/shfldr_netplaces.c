/*
 *	Network Places (Neighbourhood) folder
 *
 *	Copyright 1997			Marcus Meissner
 *	Copyright 1998, 1999, 2002	Juergen Schmied
 *	Copyright 2003                  Mike McCormack for Codeweavers
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

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/***********************************************************************
*   IShellFolder implementation
*/

typedef struct {
    const IShellFolder2Vtbl  *lpVtbl;
    LONG                       ref;
    const IPersistFolder2Vtbl *lpVtblPersistFolder2;

    /* both paths are parsible from the desktop */
    LPITEMIDLIST pidlRoot;	/* absolute pidl */
} IGenericSFImpl;

static const IShellFolder2Vtbl vt_ShellFolder2;
static const IPersistFolder2Vtbl vt_NP_PersistFolder2;


#define _IPersistFolder2_Offset ((INT_PTR)(&(((IGenericSFImpl*)0)->lpVtblPersistFolder2)))
#define _ICOM_THIS_From_IPersistFolder2(class, name) class* This = (class*)(((char*)name)-_IPersistFolder2_Offset);

#define _IUnknown_(This)	(IUnknown*)&(This->lpVtbl)
#define _IShellFolder_(This)	(IShellFolder*)&(This->lpVtbl)
#define _IPersistFolder2_(This)	(IPersistFolder2*)&(This->lpVtblPersistFolder2)

static shvheader NetworkPlacesSFHeader[] = {
    {IDS_SHV_COLUMN8, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN13, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN_WORKGROUP, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
	{IDS_SHV_NETWORKLOCATION, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15}
};

#define COLUMN_NAME          0
#define COLUMN_CATEGORY      1
#define COLUMN_WORKGROUP     2
#define COLUMN_NETLOCATION   3

#define NETWORKPLACESSHELLVIEWCOLUMNS 4

/**************************************************************************
*	ISF_NetworkPlaces_Constructor
*/
HRESULT WINAPI ISF_NetworkPlaces_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IGenericSFImpl *sf;

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    sf = (IGenericSFImpl *) HeapAlloc ( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (IGenericSFImpl));
    if (!sf)
        return E_OUTOFMEMORY;

    sf->ref = 0;
    sf->lpVtbl = &vt_ShellFolder2;
    sf->lpVtblPersistFolder2 = &vt_NP_PersistFolder2;
    sf->pidlRoot = _ILCreateNetHood();	/* my qualified pidl */

    if (!SUCCEEDED (IUnknown_QueryInterface (_IUnknown_ (sf), riid, ppv)))
    {
        IUnknown_Release (_IUnknown_ (sf));
        return E_NOINTERFACE;
    }

    TRACE ("--(%p)\n", sf);
    return S_OK;
}

/**************************************************************************
 *	ISF_NetworkPlaces_fnQueryInterface
 *
 * NOTE
 *     supports not IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_NetworkPlaces_fnQueryInterface (IShellFolder2 *iface, REFIID riid, LPVOID *ppvObj)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) ||
        IsEqualIID (riid, &IID_IShellFolder) ||
        IsEqualIID (riid, &IID_IShellFolder2))
    {
        *ppvObj = This;
    }
    else if (IsEqualIID (riid, &IID_IPersist) ||
             IsEqualIID (riid, &IID_IPersistFolder) ||
             IsEqualIID (riid, &IID_IPersistFolder2))
    {
        *ppvObj = _IPersistFolder2_ (This);
    }

    if (*ppvObj)
    {
        IUnknown_AddRef ((IUnknown *) (*ppvObj));
        TRACE ("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
        return S_OK;
    }
    TRACE ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ISF_NetworkPlaces_fnAddRef (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI ISF_NetworkPlaces_fnRelease (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount + 1);

    if (!refCount) {
        TRACE ("-- destroying IShellFolder(%p)\n", This);
        SHFree (This->pidlRoot);
        HeapFree (GetProcessHeap(), 0, This);
    }
    return refCount;
}

/**************************************************************************
*	ISF_NetworkPlaces_fnParseDisplayName
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnParseDisplayName (IShellFolder2 * iface,
               HWND hwndOwner, LPBC pbcReserved, LPOLESTR lpszDisplayName,
               DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    HRESULT hr = E_UNEXPECTED;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n", This,
            hwndOwner, pbcReserved, lpszDisplayName, debugstr_w (lpszDisplayName),
            pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;		/* strange but like the original */

    TRACE ("(%p)->(-- ret=0x%08x)\n", This, hr);

    return hr;
}

/**************************************************************************
*		ISF_NetworkPlaces_fnEnumObjects
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnEnumObjects (IShellFolder2 * iface,
               HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(HWND=%p flags=0x%08x pplist=%p)\n", This,
            hwndOwner, dwFlags, ppEnumIDList);

    *ppEnumIDList = NULL; //IEnumIDList_Constructor();

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);
 return S_FALSE;
   // return (*ppEnumIDList) ? S_OK : E_OUTOFMEMORY;
}

/**************************************************************************
*		ISF_NetworkPlaces_fnBindToObject
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnBindToObject (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n", This,
            pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild (This->pidlRoot, NULL, pidl, riid, ppvOut);
}

/**************************************************************************
*	ISF_NetworkPlaces_fnBindToStorage
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnBindToStorage (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n", This,
            pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
* 	ISF_NetworkPlaces_fnCompareIDs
*/

static HRESULT WINAPI ISF_NetworkPlaces_fnCompareIDs (IShellFolder2 * iface,
               LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    int nReturn;

    TRACE ("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", This, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs (_IShellFolder_ (This), lParam, pidl1, pidl2);
    TRACE ("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
*	ISF_NetworkPlaces_fnCreateViewObject
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnCreateViewObject (IShellFolder2 * iface,
               HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n", This,
            hwndOwner, shdebugstr_guid (riid), ppvOut);

    if (!ppvOut)
        return hr;

	*ppvOut = NULL;

	if (IsEqualIID (riid, &IID_IDropTarget))
    {
	    WARN ("IDropTarget not implemented\n");
	    hr = E_NOTIMPL;
	}
    else if (IsEqualIID (riid, &IID_IContextMenu))
    {
	    WARN ("IContextMenu not implemented\n");
	    hr = E_NOTIMPL;
	}
    else if (IsEqualIID (riid, &IID_IShellView))
    {
	    pShellView = IShellView_Constructor ((IShellFolder *) iface);
	    if (pShellView)
        {
            hr = IShellView_QueryInterface (pShellView, riid, ppvOut);
            IShellView_Release (pShellView);
	    }
    }
    TRACE ("-- (%p)->(interface=%p)\n", This, ppvOut);
    return hr;
}

/**************************************************************************
*  ISF_NetworkPlaces_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnGetAttributesOf (IShellFolder2 * iface,
               UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    static const DWORD dwNethoodAttributes =
        SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR | 
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;
    HRESULT hr = S_OK;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n", This,
            cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if(cidl == 0) {
        *rgfInOut = dwNethoodAttributes;
    }
    else
    {
        while (cidl > 0 && *apidl)
        {
            pdump (*apidl);
            SHELL32_GetItemAttributes (_IShellFolder_ (This), *apidl, rgfInOut);
            apidl++;
            cidl--;
        }
    }

    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE ("-- result=0x%08x\n", *rgfInOut);
    return hr;
}

/**************************************************************************
*	ISF_NetworkPlaces_fnGetUIObjectOf
*
* PARAMETERS
*  hwndOwner [in]  Parent window for any output
*  cidl      [in]  array size
*  apidl     [in]  simple pidl array
*  riid      [in]  Requested Interface
*  prgfInOut [   ] reserved
*  ppvObject [out] Resulting Interface
*
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnGetUIObjectOf (IShellFolder2 * iface,
               HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid,
               UINT * prgfInOut, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n", This,
            hwndOwner, cidl, apidl, shdebugstr_guid (riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

	*ppvOut = NULL;

	if (IsEqualIID (riid, &IID_IContextMenu) && (cidl >= 1))
    {
        hr = CDefFolderMenu_Create2(This->pidlRoot, hwndOwner, cidl, apidl, (IShellFolder*)iface, NULL, 0, NULL, (IContextMenu**)&pObj);
	}
    else if (IsEqualIID (riid, &IID_IDataObject) && (cidl >= 1))
    {
	    pObj = (LPUNKNOWN) IDataObject_Constructor (hwndOwner, This->pidlRoot, apidl, cidl);
	    hr = S_OK;
	}
    else if (IsEqualIID (riid, &IID_IExtractIconA) && (cidl == 1))
    {
	    pidl = ILCombine (This->pidlRoot, apidl[0]);
	    pObj = (LPUNKNOWN) IExtractIconA_Constructor (pidl);
	    SHFree (pidl);
	    hr = S_OK;
	}
    else if (IsEqualIID (riid, &IID_IExtractIconW) && (cidl == 1))
    {
	    pidl = ILCombine (This->pidlRoot, apidl[0]);
	    pObj = (LPUNKNOWN) IExtractIconW_Constructor (pidl);
	    SHFree (pidl);
	    hr = S_OK;
	}
    else if (IsEqualIID (riid, &IID_IDropTarget) && (cidl >= 1))
    {
	    hr = IShellFolder_QueryInterface (iface, &IID_IDropTarget, (LPVOID *) & pObj);
	}
    else
	    hr = E_NOINTERFACE;

	if (SUCCEEDED(hr) && !pObj)
	    hr = E_OUTOFMEMORY;

	*ppvOut = pObj;
    TRACE ("(%p)->hr=0x%08x\n", This, hr);
    return hr;
}

/**************************************************************************
*	ISF_NetworkPlaces_fnGetDisplayNameOf
*
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnGetDisplayNameOf (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)->(pidl=%p,0x%08x,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

/**************************************************************************
*  ISF_NetworkPlaces_fnSetNameOf
*  Changes the name of a file object or subfolder, possibly changing its item
*  identifier in the process.
*
* PARAMETERS
*  hwndOwner [in]  Owner window for output
*  pidl      [in]  simple pidl of item to change
*  lpszName  [in]  the items new display name
*  dwFlags   [in]  SHGNO formatting flags
*  ppidlOut  [out] simple pidl returned
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnSetNameOf (IShellFolder2 * iface,
               HWND hwndOwner, LPCITEMIDLIST pidl,	/*simple pidl */
               LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)->(%p,pidl=%p,%s,%u,%p)\n", This,
            hwndOwner, pidl, debugstr_w (lpName), dwFlags, pPidlOut);
    return E_FAIL;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnGetDefaultSearchGUID (
               IShellFolder2 * iface, GUID * pguid)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnEnumSearches (IShellFolder2 * iface,
               IEnumExtraSearch ** ppenum)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnGetDefaultColumn (IShellFolder2 * iface,
               DWORD dwRes, ULONG * pSort, ULONG * pDisplay)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)\n", This);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnGetDefaultColumnState (
               IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)\n", This);

    if (!pcsFlags || iColumn >= NETWORKPLACESSHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = NetworkPlacesSFHeader[iColumn].pcsFlags;
    return S_OK;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnGetDetailsEx (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnGetDetailsOf (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    WCHAR buffer[MAX_PATH] = {0};
    HRESULT hr = E_FAIL;

    if (iColumn >= NETWORKPLACESSHELLVIEWCOLUMNS)
        return E_FAIL;

    psd->fmt = NetworkPlacesSFHeader[iColumn].fmt;
    psd->cxChar = NetworkPlacesSFHeader[iColumn].cxChar;
    if (pidl == NULL)
    {
        psd->str.uType = STRRET_WSTR;
        if (LoadStringW(shell32_hInstance, NetworkPlacesSFHeader[iColumn].colnameid, buffer, MAX_PATH))
            hr = SHStrDupW(buffer, &psd->str.u.pOleStr);

        return hr;
    }

    if (iColumn == COLUMN_NAME)
        return IShellFolder2_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL, &psd->str);

    FIXME ("(%p)->(%p %i %p)\n", This, pidl, iColumn, psd);

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnMapColumnToSCID (IShellFolder2 * iface,
               UINT column, SHCOLUMNID * pscid)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)\n", This);

    return E_NOTIMPL;
}

static const IShellFolder2Vtbl vt_ShellFolder2 = {
    ISF_NetworkPlaces_fnQueryInterface,
    ISF_NetworkPlaces_fnAddRef,
    ISF_NetworkPlaces_fnRelease,
    ISF_NetworkPlaces_fnParseDisplayName,
    ISF_NetworkPlaces_fnEnumObjects,
    ISF_NetworkPlaces_fnBindToObject,
    ISF_NetworkPlaces_fnBindToStorage,
    ISF_NetworkPlaces_fnCompareIDs,
    ISF_NetworkPlaces_fnCreateViewObject,
    ISF_NetworkPlaces_fnGetAttributesOf,
    ISF_NetworkPlaces_fnGetUIObjectOf,
    ISF_NetworkPlaces_fnGetDisplayNameOf,
    ISF_NetworkPlaces_fnSetNameOf,
    /* ShellFolder2 */
    ISF_NetworkPlaces_fnGetDefaultSearchGUID,
    ISF_NetworkPlaces_fnEnumSearches,
    ISF_NetworkPlaces_fnGetDefaultColumn,
    ISF_NetworkPlaces_fnGetDefaultColumnState,
    ISF_NetworkPlaces_fnGetDetailsEx,
    ISF_NetworkPlaces_fnGetDetailsOf,
    ISF_NetworkPlaces_fnMapColumnToSCID
};

/************************************************************************
 *	INPFldr_PersistFolder2_QueryInterface
 */
static HRESULT WINAPI INPFldr_PersistFolder2_QueryInterface (IPersistFolder2 * iface,
               REFIID iid, LPVOID * ppvObj)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    return IUnknown_QueryInterface (_IUnknown_ (This), iid, ppvObj);
}

/************************************************************************
 *	INPFldr_PersistFolder2_AddRef
 */
static ULONG WINAPI INPFldr_PersistFolder2_AddRef (IPersistFolder2 * iface)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_AddRef (_IUnknown_ (This));
}

/************************************************************************
 *	ISFPersistFolder_Release
 */
static ULONG WINAPI INPFldr_PersistFolder2_Release (IPersistFolder2 * iface)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return IUnknown_Release (_IUnknown_ (This));
}

/************************************************************************
 *	INPFldr_PersistFolder2_GetClassID
 */
static HRESULT WINAPI INPFldr_PersistFolder2_GetClassID (
               IPersistFolder2 * iface, CLSID * lpClassId)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_NetworkPlaces;

    return S_OK;
}

/************************************************************************
 *	INPFldr_PersistFolder2_Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
static HRESULT WINAPI INPFldr_PersistFolder2_Initialize (
               IPersistFolder2 * iface, LPCITEMIDLIST pidl)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    return E_NOTIMPL;
}

/**************************************************************************
 *	IPersistFolder2_fnGetCurFolder
 */
static HRESULT WINAPI INPFldr_PersistFolder2_GetCurFolder (
               IPersistFolder2 * iface, LPITEMIDLIST * pidl)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    if (!pidl)
        return E_POINTER;

    *pidl = ILClone (This->pidlRoot);

    return S_OK;
}

static const IPersistFolder2Vtbl vt_NP_PersistFolder2 =
{
    INPFldr_PersistFolder2_QueryInterface,
    INPFldr_PersistFolder2_AddRef,
    INPFldr_PersistFolder2_Release,
    INPFldr_PersistFolder2_GetClassID,
    INPFldr_PersistFolder2_Initialize,
    INPFldr_PersistFolder2_GetCurFolder
};
