/*
 *    Network Places (Neighbourhood) folder
 *
 *    Copyright 1997                Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
 *    Copyright 2003              Mike McCormack for Codeweavers
 *    Copyright 2009              Andrew Hill
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

CNetFolder::CNetFolder()
{
    pidlRoot = NULL;
}

CNetFolder::~CNetFolder()
{
    TRACE("-- destroying IShellFolder(%p)\n", this);
    SHFree(pidlRoot);
}

HRESULT WINAPI CNetFolder::FinalConstruct()
{
    pidlRoot = _ILCreateGuid(PT_GUID, CLSID_NetworkPlaces); /* my qualified pidl */
    if (pidlRoot == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

/**************************************************************************
*    CNetFolder::ParseDisplayName
*/
HRESULT WINAPI CNetFolder::ParseDisplayName(HWND hwndOwner, LPBC pbcReserved, LPOLESTR lpszDisplayName,
        DWORD *pchEaten, PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes)
{
    HRESULT hr = E_UNEXPECTED;

    TRACE("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n", this,
          hwndOwner, pbcReserved, lpszDisplayName, debugstr_w (lpszDisplayName),
          pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;        /* strange but like the original */

    TRACE("(%p)->(-- ret=0x%08x)\n", this, hr);

    return hr;
}

/**************************************************************************
*        CNetFolder::EnumObjects
*/
HRESULT WINAPI CNetFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    TRACE("(%p)->(HWND=%p flags=0x%08x pplist=%p)\n", this,
          hwndOwner, dwFlags, ppEnumIDList);

    *ppEnumIDList = NULL; //IEnumIDList_Constructor();

    TRACE("-- (%p)->(new ID List: %p)\n", this, *ppEnumIDList);
    return S_FALSE;
    // return (*ppEnumIDList) ? S_OK : E_OUTOFMEMORY;
}

/**************************************************************************
*        CNetFolder::BindToObject
*/
HRESULT WINAPI CNetFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n", this,
           pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    return E_NOTIMPL;
}

/**************************************************************************
*    CNetFolder::BindToStorage
*/
HRESULT WINAPI CNetFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    FIXME("(%p)->(pidl=%p,%p,%s,%p) stub\n", this,
          pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
*     CNetFolder::CompareIDs
*/

HRESULT WINAPI CNetFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    return E_NOTIMPL;
}

/**************************************************************************
*    CNetFolder::CreateViewObject
*/
HRESULT WINAPI CNetFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    CComPtr<IShellView> pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(hwnd=%p,%s,%p)\n", this,
          hwndOwner, shdebugstr_guid (&riid), ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IDropTarget))
    {
        WARN("IDropTarget not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        WARN("IContextMenu not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID(riid, IID_IShellView))
    {
        hr = CDefView_Constructor(this, riid, ppvOut);
    }
    TRACE("-- (%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

/**************************************************************************
*  CNetFolder::GetAttributesOf
*/
HRESULT WINAPI CNetFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    static const DWORD dwNethoodAttributes =
        SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;
    HRESULT hr = S_OK;

    TRACE("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n", this,
          cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if(cidl == 0)
        *rgfInOut = dwNethoodAttributes;
    else
    {
        /* FIXME: Implement when enumerating items is implemented */
    }

    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE("-- result=0x%08x\n", *rgfInOut);
    return hr;
}

/**************************************************************************
*    CNetFolder::GetUIObjectOf
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
HRESULT WINAPI CNetFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
        UINT * prgfInOut, LPVOID * ppvOut)
{
    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n", this,
          hwndOwner, cidl, apidl, shdebugstr_guid (&riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID(riid, IID_IContextMenu) && (cidl >= 1))
    {
        IContextMenu  * pCm = NULL;
        hr = CDefFolderMenu_Create2(pidlRoot, hwndOwner, cidl, apidl, static_cast<IShellFolder*>(this), NULL, 0, NULL, &pCm);
        pObj = pCm;
    }
    else if (IsEqualIID(riid, IID_IDataObject) && (cidl >= 1))
    {
        IDataObject * pDo = NULL;
        hr = IDataObject_Constructor (hwndOwner, pidlRoot, apidl, cidl, &pDo);
        pObj = pDo;
    }
    else if (IsEqualIID(riid, IID_IExtractIconA) && (cidl == 1))
    {
        pidl = ILCombine (pidlRoot, apidl[0]);
        pObj = IExtractIconA_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID(riid, IID_IExtractIconW) && (cidl == 1))
    {
        pidl = ILCombine (pidlRoot, apidl[0]);
        pObj = IExtractIconW_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID(riid, IID_IDropTarget) && (cidl >= 1))
    {
        IDropTarget * pDt = NULL;
        hr = this->QueryInterface(IID_PPV_ARG(IDropTarget, &pDt));
        pObj = pDt;
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj;
    TRACE("(%p)->hr=0x%08x\n", this, hr);
    return hr;
}

/**************************************************************************
*    CNetFolder::GetDisplayNameOf
*
*/
HRESULT WINAPI CNetFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    LPWSTR pszName;

    TRACE ("(%p)->(pidl=%p,0x%08lx,%p)\n", this, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    if (!pidl->mkid.cb)
    {
        pszName = (LPWSTR)CoTaskMemAlloc(MAX_PATH * sizeof(WCHAR));
        if (!pszName)
            return E_OUTOFMEMORY;

        if (LoadStringW(shell32_hInstance, IDS_NETWORKPLACE, pszName, MAX_PATH))
        {
            pszName[MAX_PATH-1] = L'\0';
            strRet->uType = STRRET_WSTR;
            strRet->pOleStr = pszName;
            return S_OK;
        }
        CoTaskMemFree(pszName);
        return E_FAIL;
    }

    return E_NOTIMPL;
}

/**************************************************************************
*  CNetFolder::SetNameOf
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
HRESULT WINAPI CNetFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl,    /*simple pidl */
                                     LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    FIXME("(%p)->(%p,pidl=%p,%s,%u,%p)\n", this,
          hwndOwner, pidl, debugstr_w (lpName), dwFlags, pPidlOut);
    return E_FAIL;
}

HRESULT WINAPI CNetFolder::GetDefaultSearchGUID(GUID *pguid)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CNetFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CNetFolder::GetDefaultColumn (DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    TRACE("(%p)\n", this);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

HRESULT WINAPI CNetFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    TRACE("(%p)\n", this);

    if (!pcsFlags || iColumn >= NETWORKPLACESSHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = NetworkPlacesSFHeader[iColumn].pcsFlags;
    return S_OK;
}

HRESULT WINAPI CNetFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CNetFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    WCHAR buffer[MAX_PATH] = {0};
    HRESULT hr = E_FAIL;

    if (iColumn >= NETWORKPLACESSHELLVIEWCOLUMNS)
        return E_FAIL;

    psd->fmt = NetworkPlacesSFHeader[iColumn].fmt;
    psd->cxChar = NetworkPlacesSFHeader[iColumn].cxChar;
    if (pidl == NULL)
    {
        psd->str.uType = STRRET_WSTR;
        if (LoadStringW(shell32_hInstance, NetworkPlacesSFHeader[iColumn].colnameid, buffer, _countof(buffer)))
            hr = SHStrDupW(buffer, &psd->str.pOleStr);

        return hr;
    }

    if (iColumn == COLUMN_NAME)
        return GetDisplayNameOf(pidl, SHGDN_NORMAL, &psd->str);

    FIXME("(%p)->(%p %i %p)\n", this, pidl, iColumn, psd);

    return E_NOTIMPL;
}

HRESULT WINAPI CNetFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    FIXME("(%p)\n", this);

    return E_NOTIMPL;
}

/************************************************************************
 *    CNetFolder::GetClassID
 */
HRESULT WINAPI CNetFolder::GetClassID(CLSID *lpClassId)
{
    TRACE("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_NetworkPlaces;

    return S_OK;
}

/************************************************************************
 *    CNetFolder::Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
HRESULT WINAPI CNetFolder::Initialize(LPCITEMIDLIST pidl)
{
    TRACE("(%p)->(%p)\n", this, pidl);

    return E_NOTIMPL;
}

/**************************************************************************
 *    CNetFolder::GetCurFolder
 */
HRESULT WINAPI CNetFolder::GetCurFolder(LPITEMIDLIST *pidl)
{
    TRACE("(%p)->(%p)\n", this, pidl);

    if (!pidl)
        return E_POINTER;

    *pidl = ILClone(pidlRoot);

    return S_OK;
}
