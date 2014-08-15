/*
 *    Virtual Admin Tools Folder
 *
 *    Copyright 2008                Johannes Anderwald
 *    Copyright 2009                Andrew Hill
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/*
This folder should not exist. It is just a file system folder...
*/

/* List shortcuts of
 * CSIDL_COMMON_ADMINTOOLS
 * Note: CSIDL_ADMINTOOLS is ignored, tested with Window XP SP3+
 */

/***********************************************************************
 *     AdminTools folder implementation
 */

class CDesktopFolderEnumY :
    public IEnumIDListImpl
{
    private:
    public:
        CDesktopFolderEnumY();
        ~CDesktopFolderEnumY();
        HRESULT WINAPI Initialize(LPWSTR szTarget, DWORD dwFlags);

        BEGIN_COM_MAP(CDesktopFolderEnumY)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

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

CDesktopFolderEnumY::CDesktopFolderEnumY()
{
}

CDesktopFolderEnumY::~CDesktopFolderEnumY()
{
}

HRESULT WINAPI CDesktopFolderEnumY::Initialize(LPWSTR szTarget, DWORD dwFlags)
{
    TRACE("(%p)->(flags=0x%08x)\n", this, dwFlags);
    /* enumerate the elements in %windir%\desktop */
    return CreateFolderEnumList(szTarget, dwFlags);
}

CAdminToolsFolder::CAdminToolsFolder()
{
    pclsid = NULL;

    pidlRoot = NULL;  /* absolute pidl */
    szTarget = NULL;

    dwAttributes = 0;        /* attributes returned by GetAttributesOf FIXME: use it */
}

CAdminToolsFolder::~CAdminToolsFolder()
{
    TRACE ("-- destroying IShellFolder(%p)\n", this);
    if (pidlRoot)
        SHFree(pidlRoot);
    HeapFree(GetProcessHeap(), 0, szTarget);
}

HRESULT WINAPI CAdminToolsFolder::FinalConstruct()
{
    szTarget = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    if (szTarget == NULL)
        return E_OUTOFMEMORY;
    if (!SHGetSpecialFolderPathW(NULL, szTarget, CSIDL_COMMON_ADMINTOOLS, FALSE))
        return E_FAIL;

    pidlRoot = _ILCreateAdminTools();    /* my qualified pidl */
    if (pidlRoot == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

/**************************************************************************
 *    CAdminToolsFolder::ParseDisplayName
 *
 */
HRESULT WINAPI CAdminToolsFolder::ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
        DWORD *pchEaten, PIDLIST_RELATIVE *ppidl, DWORD *pdwAttributes)
{
    TRACE("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
          this, hwndOwner, pbc, lpszDisplayName, debugstr_w(lpszDisplayName),
          pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;

    MessageBoxW(NULL, lpszDisplayName, L"ParseDisplayName", MB_OK);

    return E_NOTIMPL;
}

/**************************************************************************
 *        CAdminToolsFolder::EnumObjects
 */
HRESULT WINAPI CAdminToolsFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    CComObject<CDesktopFolderEnumY>        *theEnumerator;
    CComPtr<IEnumIDList>                    result;
    HRESULT                                 hResult;

    TRACE ("(%p)->(HWND=%p flags=0x%08x pplist=%p)\n", this, hwndOwner, dwFlags, ppEnumIDList);

    if (ppEnumIDList == NULL)
        return E_POINTER;
    *ppEnumIDList = NULL;
    ATLTRY (theEnumerator = new CComObject<CDesktopFolderEnumY>);
    if (theEnumerator == NULL)
        return E_OUTOFMEMORY;
    hResult = theEnumerator->QueryInterface(IID_PPV_ARG(IEnumIDList, &result));
    if (FAILED (hResult))
    {
        delete theEnumerator;
        return hResult;
    }
    hResult = theEnumerator->Initialize (szTarget, dwFlags);
    if (FAILED (hResult))
        return hResult;
    *ppEnumIDList = result.Detach ();

    TRACE ("-- (%p)->(new ID List: %p)\n", this, *ppEnumIDList);

    return S_OK;
}

/**************************************************************************
 *        CAdminToolsFolder::BindToObject
 */
HRESULT WINAPI CAdminToolsFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n", this,
           pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    return SHELL32_BindToChild(pidlRoot, NULL, pidl, riid, ppvOut);
}

/**************************************************************************
 *    CAdminToolsFolder::BindToStorage
 */
HRESULT WINAPI CAdminToolsFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n",
           this, pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
 *     CAdminToolsFolder::CompareIDs
 */
HRESULT WINAPI CAdminToolsFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    int nReturn;

    TRACE ("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", this, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs (this, lParam, pidl1, pidl2);
    TRACE ("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
 *    CAdminToolsFolder::CreateViewObject
 */
HRESULT WINAPI CAdminToolsFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    CComPtr<IShellView>                    pShellView;
    HRESULT                                hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n", this,
           hwndOwner, shdebugstr_guid (&riid), ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, IID_IDropTarget))
    {
        WARN ("IDropTarget not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID (riid, IID_IShellView))
    {
        hr = IShellView_Constructor ((IShellFolder *)this, &pShellView);
        if (pShellView)
            hr = pShellView->QueryInterface(riid, ppvOut);
    }
    TRACE ("-- (%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

/**************************************************************************
 *  ISF_AdminTools_fnGetAttributesOf
 */
HRESULT WINAPI CAdminToolsFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    HRESULT hr = S_OK;
    static const DWORD dwAdminToolsAttributes =
        SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n",
           this, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

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
                SHELL32_GetItemAttributes (this, *apidl, rgfInOut);
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
 *    CAdminToolsFolder::GetUIObjectOf
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
HRESULT WINAPI CAdminToolsFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    LPITEMIDLIST pidl;
    CComPtr<IUnknown>                    pObj;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
           this, hwndOwner, cidl, apidl, shdebugstr_guid (&riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, IID_IContextMenu))
    {
        hr = CDefFolderMenu_Create2(pidlRoot, hwndOwner, cidl, apidl, (IShellFolder *)this, NULL, 0, NULL, (IContextMenu **)&pObj);
    }
    else if (IsEqualIID (riid, IID_IDataObject) && (cidl >= 1))
    {
        hr = IDataObject_Constructor(hwndOwner, pidlRoot, apidl, cidl, (IDataObject **)&pObj);
    }
    else if (IsEqualIID (riid, IID_IExtractIconA) && (cidl == 1))
    {
        pidl = ILCombine (pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconA_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, IID_IExtractIconW) && (cidl == 1))
    {
        pidl = ILCombine (pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconW_Constructor (pidl);
        SHFree (pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, IID_IDropTarget) && (cidl >= 1))
    {
        hr = this->QueryInterface(IID_IDropTarget, (LPVOID *)&pObj);
    }
    else if ((IsEqualIID(riid, IID_IShellLinkW) ||
              IsEqualIID(riid, IID_IShellLinkA)) && (cidl == 1))
    {
        pidl = ILCombine (pidlRoot, apidl[0]);
        hr = IShellLink_ConstructFromFile(NULL, riid, pidl, (LPVOID*)&pObj);
        SHFree (pidl);
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj.Detach();
    TRACE ("(%p)->hr=0x%08x\n", this, hr);
    return hr;
}

/**************************************************************************
 *    CAdminToolsFolder::GetDisplayNameOf
 *
 */
HRESULT WINAPI CAdminToolsFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    HRESULT hr = S_OK;
    LPWSTR pszPath, pOffset;

    TRACE ("(%p)->(pidl=%p,0x%08x,%p)\n", this, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    pszPath = (LPWSTR)CoTaskMemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    ZeroMemory(pszPath, (MAX_PATH + 1) * sizeof(WCHAR));

    if (!pidl->mkid.cb)
    {
        if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING))
            wcscpy(pszPath, szTarget);
        else if (!HCR_GetClassNameW(CLSID_AdminFolderShortcut, pszPath, MAX_PATH))
            hr = E_FAIL;
    }
    else if (_ILIsPidlSimple(pidl))
    {
        if ((GET_SHGDN_FOR(dwFlags) & SHGDN_FORPARSING) &&
                (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER) &&
                szTarget)
        {
            wcscpy(pszPath, szTarget);
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

            wcscpy(pszPath, szTarget);
            PathAddBackslashW(pszPath);
            len = wcslen(pszPath);

            if (!SUCCEEDED(SHELL32_GetDisplayNameOfChild(this, pidl, dwFlags | SHGDN_INFOLDER, pszPath + len, MAX_PATH + 1 - len)))
            {
                CoTaskMemFree(pszPath);
                return E_OUTOFMEMORY;
            }

        }
    }

    if (SUCCEEDED(hr))
    {
        strRet->uType = STRRET_WSTR;
        strRet->pOleStr = pszPath;
        TRACE ("-- (%p)->(%s,0x%08x)\n", this, debugstr_w(strRet->pOleStr), hr);
    }
    else
        CoTaskMemFree(pszPath);

    return hr;
}

/**************************************************************************
 *  CAdminToolsFolder::SetNameOf
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
HRESULT WINAPI CAdminToolsFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl,    /* simple pidl */
        LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    FIXME ("(%p)->(%p,pidl=%p,%s,%lu,%p)\n", this, hwndOwner, pidl,
           debugstr_w (lpName), dwFlags, pPidlOut);

    return E_FAIL;
}

HRESULT WINAPI CAdminToolsFolder::GetDefaultSearchGUID(GUID *pguid)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CAdminToolsFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CAdminToolsFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}
HRESULT WINAPI CAdminToolsFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    if (!pcsFlags || iColumn >= AdminToolsHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = AdminToolsSFHeader[iColumn].pcsFlags;
    return S_OK;

}

HRESULT WINAPI CAdminToolsFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    FIXME ("(%p): stub\n", this);

    return E_NOTIMPL;
}

HRESULT WINAPI CAdminToolsFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    WCHAR buffer[MAX_PATH] = {0};
    HRESULT hr = E_FAIL;

    TRACE("(%p)->(%p %i %p): stub\n", this, pidl, iColumn, psd);

    if (iColumn >= AdminToolsHELLVIEWCOLUMNS)
        return E_FAIL;

    psd->fmt = AdminToolsSFHeader[iColumn].fmt;
    psd->cxChar = AdminToolsSFHeader[iColumn].cxChar;
    if (pidl == NULL)
    {
        psd->str.uType = STRRET_WSTR;
        if (LoadStringW(shell32_hInstance, AdminToolsSFHeader[iColumn].colnameid, buffer, MAX_PATH))
            hr = SHStrDupW(buffer, &psd->str.pOleStr);

        return hr;
    }

    psd->str.uType = STRRET_CSTR;
    switch (iColumn)
    {
        case COLUMN_NAME:
            psd->str.uType = STRRET_WSTR;
            hr = GetDisplayNameOf(pidl,
                                  SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
            break;
        case COLUMN_SIZE:
            _ILGetFileSize (pidl, psd->str.cStr, MAX_PATH);
            break;
        case COLUMN_TYPE:
            _ILGetFileType (pidl, psd->str.cStr, MAX_PATH);
            break;
        case COLUMN_DATE:
            _ILGetFileDate (pidl, psd->str.cStr, MAX_PATH);
            break;
    }

    return hr;
}

HRESULT WINAPI CAdminToolsFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    FIXME ("(%p): stub\n", this);
    return E_NOTIMPL;
}

/************************************************************************
 *    CAdminToolsFolder::GetClassID
 */
HRESULT WINAPI CAdminToolsFolder::GetClassID(CLSID *lpClassId)
{
    TRACE ("(%p)\n", this);

    memcpy(lpClassId, &CLSID_AdminFolderShortcut, sizeof(CLSID));

    return S_OK;
}

/************************************************************************
 *    CAdminToolsFolder::Initialize
 *
 */
HRESULT WINAPI CAdminToolsFolder::Initialize(LPCITEMIDLIST pidl)
{
    if (pidlRoot)
        SHFree((LPVOID)pidlRoot);

    pidlRoot = ILClone(pidl);
    return S_OK;
}

/**************************************************************************
 *    CAdminToolsFolder::GetCurFolder
 */
HRESULT WINAPI CAdminToolsFolder::GetCurFolder(LPITEMIDLIST *pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    *pidl = ILClone (pidlRoot);
    return S_OK;
}
