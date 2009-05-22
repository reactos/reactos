/*
 *    Virtual Admin Tools Folder
 *
 *    Copyright 2008                Johannes Anderwald
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
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);


/* List shortcuts of
 * CSIDL_COMMON_ADMINTOOLS
 * Note: CSIDL_ADMINTOOLS is ignored, tested with Window XP SP3+
 */

/***********************************************************************
 *     AdminTools folder implementation
 */

typedef struct {
    IShellFolder2Vtbl *lpVtbl;
    IPersistFolder2Vtbl *lpVtblPersistFolder2;

    LONG ref;

    CLSID *pclsid;

    LPITEMIDLIST pidlRoot;  /* absolute pidl */
    LPWSTR szTarget;

    int dwAttributes;        /* attributes returned by GetAttributesOf FIXME: use it */
} IGenericSFImpl;

static const shvheader AdminToolsSFHeader[] = {
    {IDS_SHV_COLUMN8, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12}
};

#define COLUMN_NAME                0
#define COLUMN_SIZE                1
#define COLUMN_TYPE                2
#define COLUMN_DATE                3

#define AdminToolsHELLVIEWCOLUMNS (4)


#define _IPersistFolder2_Offset ((INT_PTR)(&(((IGenericSFImpl*)0)->lpVtblPersistFolder2)))
#define _ICOM_THIS_From_IPersistFolder2(class, name) class* This = (class*)(((char*)name)-_IPersistFolder2_Offset);

#define _IUnknown_(This)    (IShellFolder*)&(This->lpVtbl)
#define _IShellFolder_(This)    (IShellFolder*)&(This->lpVtbl)

#define _IPersist_(This)    (IPersist*)&(This->lpVtblPersistFolder2)
#define _IPersistFolder_(This)    (IPersistFolder*)&(This->lpVtblPersistFolder2)
#define _IPersistFolder2_(This)    (IPersistFolder2*)&(This->lpVtblPersistFolder2)

/**************************************************************************
 *    ISF_AdminTools_fnQueryInterface
 *
 * NOTE does not support IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_AdminTools_fnQueryInterface(
                IShellFolder2 * iface, REFIID riid, LPVOID * ppvObj)
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

static ULONG WINAPI ISF_AdminTools_fnAddRef (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE ("(%p)->(count=%lu)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI ISF_AdminTools_fnRelease (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE ("(%p)->(count=%lu)\n", This, refCount + 1);

    if (!refCount)
    {
        TRACE ("-- destroying IShellFolder(%p)\n", This);
        if (This->pidlRoot)
            SHFree (This->pidlRoot);
        HeapFree(GetProcessHeap(), 0, This->szTarget);
        HeapFree(GetProcessHeap(), 0, This);
        return 0;
    }
    return refCount;
}

/**************************************************************************
 *    ISF_AdminTools_fnParseDisplayName
 *
 */
static HRESULT WINAPI ISF_AdminTools_fnParseDisplayName (IShellFolder2 * iface,
                HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
                DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
           This, hwndOwner, pbc, lpszDisplayName, debugstr_w(lpszDisplayName),
           pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;

	MessageBoxW(NULL, lpszDisplayName, L"ParseDisplayName", MB_OK);

    return E_NOTIMPL;
}

/**************************************************************************
 *  CreateAdminToolsEnumList()
 */
static BOOL CreateAdminToolsEnumList(IEnumIDList *list, IGenericSFImpl *This, DWORD dwFlags)
{
    TRACE("(%p)->(flags=0x%08x)\n", list, dwFlags);
   /* enumerate the elements in %windir%\desktop */
    return CreateFolderEnumList(list, This->szTarget, dwFlags);
}

/**************************************************************************
 *        ISF_AdminTools_fnEnumObjects
 */
static HRESULT WINAPI ISF_AdminTools_fnEnumObjects (IShellFolder2 * iface,
                HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(HWND=%p flags=0x%08lx pplist=%p)\n",
           This, hwndOwner, dwFlags, ppEnumIDList);

    if(!ppEnumIDList) return E_OUTOFMEMORY;
    *ppEnumIDList = IEnumIDList_Constructor();
    if (*ppEnumIDList)
        CreateAdminToolsEnumList(*ppEnumIDList, This, dwFlags);

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    return (*ppEnumIDList) ? S_OK : E_OUTOFMEMORY;
}

/**************************************************************************
 *        ISF_AdminTools_fnBindToObject
 */
static HRESULT WINAPI ISF_AdminTools_fnBindToObject (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n", This,
            pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild (This->pidlRoot, NULL, pidl, riid, ppvOut);
}

/**************************************************************************
 *    ISF_AdminTools_fnBindToStorage
 */
static HRESULT WINAPI ISF_AdminTools_fnBindToStorage (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n",
           This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
 *     ISF_AdminTools_fnCompareIDs
 */
static HRESULT WINAPI ISF_AdminTools_fnCompareIDs (IShellFolder2 * iface,
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
 *    ISF_AdminTools_fnCreateViewObject
 */
static HRESULT WINAPI ISF_AdminTools_fnCreateViewObject (IShellFolder2 * iface,
                              HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{

    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

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
 *  ISF_AdminTools_fnGetAttributesOf
 */
static HRESULT WINAPI ISF_AdminTools_fnGetAttributesOf (IShellFolder2 * iface,
                UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    HRESULT hr = S_OK;
    static const DWORD dwAdminToolsAttributes =
        SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n",
           This, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if(cidl == 0) {
        *rgfInOut &= dwAdminToolsAttributes;
    } else {
        while (cidl > 0 && *apidl) {
            pdump (*apidl);
            if (_ILIsAdminTools(*apidl)) {
                *rgfInOut &= dwAdminToolsAttributes;
            } else {
                SHELL32_GetItemAttributes (_IShellFolder_ (This), *apidl, rgfInOut);
            }
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
 *    ISF_AdminTools_fnGetUIObjectOf
 *
 * PARAMETERS
 *  HWND           hwndOwner, //[in ] Parent window for any output
 *  UINT           cidl,      //[in ] array size
 *  LPCITEMIDLIST* apidl,     //[in ] simple pidl array
 *  REFIID         riid,      //[in ] Requested Interface
 *  UINT*          prgfInOut, //[   ] reserved
 *  LPVOID*        ppvObject) //[out] Resulting Interface
 *
 */
static HRESULT WINAPI ISF_AdminTools_fnGetUIObjectOf (IShellFolder2 * iface,
                HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
       This, hwndOwner, cidl, apidl, shdebugstr_guid (riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IContextMenu))
    {
        hr = CDefFolderMenu_Create2(This->pidlRoot, hwndOwner, cidl, apidl, (IShellFolder*)iface, NULL, 0, NULL, (IContextMenu**)&pObj);
    }
    else if (IsEqualIID (riid, &IID_IDataObject) && (cidl >= 1))
    {
        pObj = (LPUNKNOWN) IDataObject_Constructor( hwndOwner,
                                                  This->pidlRoot, apidl, cidl);
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
        hr = IShellFolder_QueryInterface (iface,
                                          &IID_IDropTarget, (LPVOID *) & pObj);
    }
    else if ((IsEqualIID(riid,&IID_IShellLinkW) ||
              IsEqualIID(riid,&IID_IShellLinkA)) && (cidl == 1))
    {
        pidl = ILCombine (This->pidlRoot, apidl[0]);
        hr = IShellLink_ConstructFromFile(NULL, riid, pidl, (LPVOID*)&pObj);
        SHFree (pidl);
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
 *    ISF_AdminTools_fnGetDisplayNameOf
 *
 */
static HRESULT WINAPI ISF_AdminTools_fnGetDisplayNameOf (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    HRESULT hr = S_OK;
    LPWSTR pszPath, pOffset;


    TRACE ("(%p)->(pidl=%p,0x%08x,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    pszPath = CoTaskMemAlloc((MAX_PATH +1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    ZeroMemory(pszPath, (MAX_PATH +1) * sizeof(WCHAR));

    if (_ILIsAdminTools (pidl))
    {
        if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
            (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING))
            wcscpy(pszPath, This->szTarget);
        else if (!HCR_GetClassNameW(&CLSID_AdminFolderShortcut, pszPath, MAX_PATH))
            hr = E_FAIL;
    }
    else if (_ILIsPidlSimple(pidl))
    {
        if ((GET_SHGDN_FOR(dwFlags) & SHGDN_FORPARSING) &&
            (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER) &&
            This->szTarget)
        {
            wcscpy(pszPath, This->szTarget);
            pOffset = PathAddBackslashW(pszPath);
            if (pOffset)
            {
                if (!_ILSimpleGetTextW(pidl, pOffset, MAX_PATH + 1 - (pOffset - pszPath)))
                    hr = E_FAIL;
            }
            else
                hr = E_FAIL;
        }
        else
        {
            if (_ILSimpleGetTextW(pidl, pszPath, MAX_PATH + 1))
            {
                if (SHELL_FS_HideExtension(pszPath))
                    PathRemoveExtensionW(pszPath);
            }
            else
                hr = E_FAIL;
        }
    }
    else if (_ILIsSpecialFolder(pidl)) 
    {
        BOOL bSimplePidl = _ILIsPidlSimple(pidl);

        if (bSimplePidl) 
        {
            if (!_ILSimpleGetTextW(pidl, pszPath, MAX_PATH))
                hr = E_FAIL;
        }
        else if ((dwFlags & SHGDN_FORPARSING) && !bSimplePidl) 
        {
            int len = 0;

            wcscpy(pszPath, This->szTarget);
            PathAddBackslashW(pszPath);
            len = wcslen(pszPath);

            if (!SUCCEEDED(SHELL32_GetDisplayNameOfChild(iface, pidl, dwFlags | SHGDN_INFOLDER, pszPath + len, MAX_PATH + 1 - len)))
            {
                CoTaskMemFree(pszPath);
                return E_OUTOFMEMORY;
            }

        } 
    }

    if (SUCCEEDED(hr))
    {
        strRet->uType = STRRET_WSTR;
        strRet->u.pOleStr = pszPath;
        TRACE ("-- (%p)->(%s,0x%08x)\n", This, debugstr_w(strRet->u.pOleStr), hr);
    }
    else
        CoTaskMemFree(pszPath);

    return hr;
}

/**************************************************************************
 *  ISF_AdminTools_fnSetNameOf
 *  Changes the name of a file object or subfolder, possibly changing its item
 *  identifier in the process.
 *
 * PARAMETERS
 *  HWND          hwndOwner,  //[in ] Owner window for output
 *  LPCITEMIDLIST pidl,       //[in ] simple pidl of item to change
 *  LPCOLESTR     lpszName,   //[in ] the items new display name
 *  DWORD         dwFlags,    //[in ] SHGNO formatting flags
 *  LPITEMIDLIST* ppidlOut)   //[out] simple pidl returned
 */
static HRESULT WINAPI ISF_AdminTools_fnSetNameOf (IShellFolder2 * iface,
                HWND hwndOwner, LPCITEMIDLIST pidl,    /* simple pidl */
                LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)->(%p,pidl=%p,%s,%lu,%p)\n", This, hwndOwner, pidl,
           debugstr_w (lpName), dwFlags, pPidlOut);

    return E_FAIL;
}

static HRESULT WINAPI ISF_AdminTools_fnGetDefaultSearchGUID(IShellFolder2 *iface,
                GUID * pguid)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_AdminTools_fnEnumSearches (IShellFolder2 *iface,
                IEnumExtraSearch ** ppenum)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_AdminTools_fnGetDefaultColumn (IShellFolder2 * iface,
                DWORD dwRes, ULONG * pSort, ULONG * pDisplay)
{
    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}
static HRESULT WINAPI ISF_AdminTools_fnGetDefaultColumnState (
                IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    if (!pcsFlags || iColumn >= AdminToolsHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = AdminToolsSFHeader[iColumn].pcsFlags;
    return S_OK;

}

static HRESULT WINAPI ISF_AdminTools_fnGetDetailsEx (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_AdminTools_fnGetDetailsOf (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    WCHAR buffer[MAX_PATH] = {0};
    HRESULT hr = E_FAIL;

    TRACE("(%p)->(%p %i %p): stub\n", This, pidl, iColumn, psd);

    if (iColumn >= AdminToolsHELLVIEWCOLUMNS)
        return E_FAIL;

    psd->fmt = AdminToolsSFHeader[iColumn].fmt;
    psd->cxChar = AdminToolsSFHeader[iColumn].cxChar;
    if (pidl == NULL)
    {
        psd->str.uType = STRRET_WSTR;
        if (LoadStringW(shell32_hInstance, AdminToolsSFHeader[iColumn].colnameid, buffer, MAX_PATH))
            hr = SHStrDupW(buffer, &psd->str.u.pOleStr);

        return hr;
    }

    psd->str.uType = STRRET_CSTR;
    switch (iColumn)
    {
    case COLUMN_NAME:
        psd->str.uType = STRRET_WSTR;
        hr = IShellFolder_GetDisplayNameOf(iface, pidl,
                   SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
        break;
    case COLUMN_SIZE:
        _ILGetFileSize (pidl, psd->str.u.cStr, MAX_PATH);
        break;
    case COLUMN_TYPE:
        _ILGetFileType (pidl, psd->str.u.cStr, MAX_PATH);
        break;
    case COLUMN_DATE:
        _ILGetFileDate (pidl, psd->str.u.cStr, MAX_PATH);
        break;
    }

    return hr;
}

static HRESULT WINAPI ISF_AdminTools_fnMapColumnToSCID (
                IShellFolder2 * iface, UINT column, SHCOLUMNID * pscid)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p): stub\n", This);
    return E_NOTIMPL;
}

static IShellFolder2Vtbl vt_ShellFolder2 =
{
    ISF_AdminTools_fnQueryInterface,
    ISF_AdminTools_fnAddRef,
    ISF_AdminTools_fnRelease,
    ISF_AdminTools_fnParseDisplayName,
    ISF_AdminTools_fnEnumObjects,
    ISF_AdminTools_fnBindToObject,
    ISF_AdminTools_fnBindToStorage,
    ISF_AdminTools_fnCompareIDs,
    ISF_AdminTools_fnCreateViewObject,
    ISF_AdminTools_fnGetAttributesOf,
    ISF_AdminTools_fnGetUIObjectOf,
    ISF_AdminTools_fnGetDisplayNameOf,
    ISF_AdminTools_fnSetNameOf,
    /* ShellFolder2 */
    ISF_AdminTools_fnGetDefaultSearchGUID,
    ISF_AdminTools_fnEnumSearches,
    ISF_AdminTools_fnGetDefaultColumn,
    ISF_AdminTools_fnGetDefaultColumnState,
    ISF_AdminTools_fnGetDetailsEx,
    ISF_AdminTools_fnGetDetailsOf,
    ISF_AdminTools_fnMapColumnToSCID
};

/************************************************************************
 *    IPF_AdminTools_QueryInterface
 */
static HRESULT WINAPI IPF_AdminTools_QueryInterface (
               IPersistFolder2 * iface, REFIID iid, LPVOID * ppvObj)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    return IUnknown_QueryInterface (_IUnknown_ (This), iid, ppvObj);
}

/************************************************************************
 *    IPF_AdminTools_AddRef
 */
static ULONG WINAPI IPF_AdminTools_AddRef (IPersistFolder2 * iface)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_AddRef (_IUnknown_ (This));
}

/************************************************************************
 *    IPF_AdminTools_Release
 */
static ULONG WINAPI IPF_AdminTools_Release (IPersistFolder2 * iface)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IUnknown_Release (_IUnknown_ (This));
}

/************************************************************************
 *    IPF_AdminTools_GetClassID
 */
static HRESULT WINAPI IPF_AdminTools_GetClassID (
               IPersistFolder2 * iface, CLSID * lpClassId)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    memcpy(lpClassId, &CLSID_AdminFolderShortcut, sizeof(CLSID));

    return S_OK;
}

/************************************************************************
 *    IPF_AdminTools_Initialize
 *
 */
static HRESULT WINAPI IPF_AdminTools_Initialize (
               IPersistFolder2 * iface, LPCITEMIDLIST pidl)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);
    if (This->pidlRoot)
        SHFree((LPVOID)This->pidlRoot);

    This->pidlRoot = ILClone(pidl);
    return S_OK;
}

/**************************************************************************
 *    IPF_AdminTools_fnGetCurFolder
 */
static HRESULT WINAPI IPF_AdminTools_GetCurFolder (
               IPersistFolder2 * iface, LPITEMIDLIST * pidl)
{
    _ICOM_THIS_From_IPersistFolder2 (IGenericSFImpl, iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    *pidl = ILClone (This->pidlRoot);
    return S_OK;
}

static IPersistFolder2Vtbl vt_PersistFolder2 =
{
    IPF_AdminTools_QueryInterface,
    IPF_AdminTools_AddRef,
    IPF_AdminTools_Release,
    IPF_AdminTools_GetClassID,
    IPF_AdminTools_Initialize,
    IPF_AdminTools_GetCurFolder
};

/**************************************************************************
 *    ISF_AdminTools_Constructor
 */
HRESULT WINAPI ISF_AdminTools_Constructor (
                IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IGenericSFImpl *sf;
    HRESULT hr;

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    sf = HeapAlloc( GetProcessHeap(), 0, sizeof(*sf) );
    if (!sf)
        return E_OUTOFMEMORY;

    sf->szTarget = HeapAlloc( GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR) );
    if (!sf->szTarget)
    {
        HeapFree(GetProcessHeap(), 0, sf);
        return E_OUTOFMEMORY;
    }
    if (!SHGetSpecialFolderPathW(NULL, sf->szTarget, CSIDL_COMMON_ADMINTOOLS, FALSE))
    {
        HeapFree(GetProcessHeap(), 0, sf->szTarget);
        HeapFree(GetProcessHeap(), 0, sf);
        return E_FAIL;
    }

    sf->ref = 1;
    sf->lpVtbl = &vt_ShellFolder2;
    sf->lpVtblPersistFolder2 = &vt_PersistFolder2;
    sf->pidlRoot = _ILCreateAdminTools();    /* my qualified pidl */

    hr = IUnknown_QueryInterface( _IUnknown_(sf), riid, ppv );
    IUnknown_Release( _IUnknown_(sf) );

    TRACE ("--(%p)\n", *ppv);
    return hr;
}
