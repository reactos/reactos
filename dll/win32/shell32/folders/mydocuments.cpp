/*
 *    Virtual MyDocuments Folder
 *
 *    Copyright 2007    Johannes Anderwald
 *    Copyright 2009    Andrew Hill
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

WINE_DEFAULT_DEBUG_CHANNEL (mydocs);

/*
CFileSysEnumX should not exist. CMyDocsFolder should aggregate a CFSFolder which always
maps the contents of CSIDL_PERSONAL. Therefore, CMyDocsFolder::EnumObjects simply calls
CFSFolder::EnumObjects.
*/

/***********************************************************************
*     MyDocumentsfolder implementation
*/

class CFileSysEnumX :
    public IEnumIDListImpl
{
    private:
    public:
        CFileSysEnumX();
        ~CFileSysEnumX();
        HRESULT WINAPI Initialize(DWORD dwFlags);

        BEGIN_COM_MAP(CFileSysEnumX)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

static const shvheader MyDocumentsSFHeader[] = {
    {IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12},
    {IDS_SHV_COLUMN5, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 5}
};

#define MYDOCUMENTSSHELLVIEWCOLUMNS 5

CFileSysEnumX::CFileSysEnumX()
{
}

CFileSysEnumX::~CFileSysEnumX()
{
}

HRESULT WINAPI CFileSysEnumX::Initialize(DWORD dwFlags)
{
    WCHAR                                szPath[MAX_PATH];

    if (SHGetSpecialFolderPathW(0, szPath, CSIDL_PERSONAL, FALSE) == FALSE)
        return E_FAIL;
    return CreateFolderEnumList(szPath, dwFlags);
}

CMyDocsFolder::CMyDocsFolder()
{
    pidlRoot = NULL;
    sPathTarget = NULL;
    mFSDropTarget = NULL;
}

CMyDocsFolder::~CMyDocsFolder()
{
    TRACE ("-- destroying IShellFolder(%p)\n", this);
    SHFree(pidlRoot);
    HeapFree(GetProcessHeap(), 0, sPathTarget);
    mFSDropTarget->Release();
}

HRESULT WINAPI CMyDocsFolder::FinalConstruct()
{
    WCHAR                                szMyPath[MAX_PATH];

    if (!SHGetSpecialFolderPathW(0, szMyPath, CSIDL_PERSONAL, TRUE))
        return E_UNEXPECTED;

    pidlRoot = _ILCreateMyDocuments();    /* my qualified pidl */
    sPathTarget = (LPWSTR)SHAlloc((wcslen(szMyPath) + 1) * sizeof(WCHAR));
    wcscpy(sPathTarget, szMyPath);

    LPITEMIDLIST pidl = NULL;

    WCHAR szPath[MAX_PATH];
    lstrcpynW(szPath, sPathTarget, MAX_PATH);
    PathAddBackslashW(szPath);
    CComPtr<IShellFolder> psfDesktop = NULL;
    
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
        hr = psfDesktop->ParseDisplayName(NULL, NULL, szPath, NULL, &pidl, NULL);
    else
        ERR("Error getting desktop folder\n");

    if (SUCCEEDED(hr))
    {
        hr = psfDesktop->BindToObject(pidl, NULL, IID_IDropTarget, (LPVOID*) &mFSDropTarget);
        CoTaskMemFree(pidl);
        if (FAILED(hr))
            ERR("Error Binding");
    }
    else
        ERR("Error creating from %s\n", debugstr_w(szPath));

    return S_OK;
}

HRESULT WINAPI CMyDocsFolder::ParseDisplayName (HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
        DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    WCHAR szElement[MAX_PATH];
    LPCWSTR szNext = NULL;
    LPITEMIDLIST pidlTemp = NULL;
    HRESULT hr = S_OK;
    CLSID clsid;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
           this, hwndOwner, pbc, lpszDisplayName, debugstr_w(lpszDisplayName),
           pchEaten, ppidl, pdwAttributes);

    if (!lpszDisplayName || !ppidl)
        return E_INVALIDARG;

    *ppidl = 0;

    if (pchEaten)
        *pchEaten = 0;        /* strange but like the original */

    if (lpszDisplayName[0] == ':' && lpszDisplayName[1] == ':')
    {
        szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);
        TRACE("-- element: %s\n", debugstr_w (szElement));
        CLSIDFromString(szElement + 2, &clsid);
        pidlTemp = _ILCreateGuid (PT_GUID, clsid);
    }
    else if( (pidlTemp = SHELL32_CreatePidlFromBindCtx(pbc, lpszDisplayName)) )
    {
        *ppidl = pidlTemp;
        return S_OK;
    }
    else
    {
        /* it's a filesystem path on the desktop. Let a FSFolder parse it */

        if (*lpszDisplayName)
        {
            WCHAR szPath[MAX_PATH];
            LPWSTR pathPtr;

            /* build a complete path to create a simple pidl */
            lstrcpynW(szPath, sPathTarget, MAX_PATH);
            pathPtr = PathAddBackslashW(szPath);
            if (pathPtr)
            {
                lstrcpynW(pathPtr, lpszDisplayName, MAX_PATH - (pathPtr - szPath));
                hr = _ILCreateFromPathW(szPath, &pidlTemp);
            }
            else
            {
                /* should never reach here, but for completeness */
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
        }
        else
            pidlTemp = _ILCreateMyDocuments();

        szNext = NULL;
    }

    if (SUCCEEDED(hr) && pidlTemp)
    {
        if (szNext && *szNext)
        {
            hr = SHELL32_ParseNextElement(this, hwndOwner, pbc,
                                          &pidlTemp, (LPOLESTR) szNext, pchEaten, pdwAttributes);
        }
        else
        {
            if (pdwAttributes && *pdwAttributes)
                hr = SHELL32_GetItemAttributes(this, pidlTemp, pdwAttributes);
        }
    }

    *ppidl = pidlTemp;

    TRACE ("(%p)->(-- ret=0x%08x)\n", this, hr);

    return hr;
}

/**************************************************************************
 *        ISF_MyDocuments_fnEnumObjects
 */
HRESULT WINAPI CMyDocsFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    CComObject<CFileSysEnumX>                *theEnumerator;
    CComPtr<IEnumIDList>                    result;
    HRESULT                                    hResult;

    TRACE("(%p)->(HWND=%p flags=0x%08x pplist=%p)\n", this, hwndOwner, dwFlags, ppEnumIDList);

    if (ppEnumIDList == NULL)
        return E_POINTER;
    *ppEnumIDList = NULL;
    ATLTRY (theEnumerator = new CComObject<CFileSysEnumX>);
    if (theEnumerator == NULL)
        return E_OUTOFMEMORY;
    hResult = theEnumerator->QueryInterface(IID_IEnumIDList, (void **)&result);
    if (FAILED (hResult))
    {
        delete theEnumerator;
        return hResult;
    }
    hResult = theEnumerator->Initialize(dwFlags);
    if (FAILED (hResult))
        return hResult;
    *ppEnumIDList = result.Detach();

    TRACE("-- (%p)->(new ID List: %p)\n", this, *ppEnumIDList);

    return S_OK;
}

/**************************************************************************
 *        CMyDocsFolder::BindToObject
 */
HRESULT WINAPI CMyDocsFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    TRACE("(%p)->(pidl=%p,%p,%s,%p)\n",
           this, pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    return SHELL32_BindToChild( pidlRoot, sPathTarget, pidl, riid, ppvOut );
}

/**************************************************************************
 *    CMyDocsFolder::BindToStorage
 */
HRESULT WINAPI CMyDocsFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    FIXME("(%p)->(pidl=%p,%p,%s,%p) stub\n",
           this, pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
 *     CMyDocsFolder::CompareIDs
 */
HRESULT WINAPI CMyDocsFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int nReturn;

    TRACE ("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", this, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs (this, lParam, pidl1, pidl2);
    TRACE ("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
 *    CMyDocsFolder::CreateViewObject
 */
HRESULT WINAPI CMyDocsFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n",
           this, hwndOwner, shdebugstr_guid (&riid), ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, IID_IDropTarget))
    {
        hr = this->QueryInterface (IID_IDropTarget, ppvOut);
    }
    else if (IsEqualIID (riid, IID_IContextMenu))
    {
        WARN ("IContextMenu not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID (riid, IID_IShellView))
    {
        hr = IShellView_Constructor ((IShellFolder *)this, &pShellView);
        if (pShellView)
        {
            hr = pShellView->QueryInterface(riid, ppvOut);
            pShellView->Release();
        }
    }
    TRACE ("-- (%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

/**************************************************************************
 *  CMyDocsFolder::GetAttributesOf
 */
HRESULT WINAPI CMyDocsFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, DWORD *rgfInOut)
{
    HRESULT hr = S_OK;
    static const DWORD dwMyDocumentsAttributes =
        SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR | SFGAO_CANCOPY |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n",
           this, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if(cidl == 0) {
        *rgfInOut &= dwMyDocumentsAttributes;
    } else {
        while (cidl > 0 && *apidl) {
            pdump (*apidl);
            if (_ILIsMyDocuments(*apidl)) {
                *rgfInOut &= dwMyDocumentsAttributes;
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
 *    CMyDocsFolder::GetUIObjectOf
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
HRESULT WINAPI CMyDocsFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST *apidl,
        REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
           this, hwndOwner, cidl, apidl, shdebugstr_guid (&riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, IID_IContextMenu))
    {
        hr = CDefFolderMenu_Create2(pidlRoot, hwndOwner, cidl, apidl, (IShellFolder *)this, NULL, 0, NULL, (IContextMenu**)&pObj);
    }
    else if (IsEqualIID (riid, IID_IDataObject) && (cidl >= 1))
    {
        hr = IDataObject_Constructor( hwndOwner,
                                      pidlRoot, apidl, cidl, (IDataObject **)&pObj);
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
        hr = this->QueryInterface (IID_IDropTarget, (LPVOID *)&pObj);
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

    *ppvOut = pObj;
    TRACE ("(%p)->hr=0x%08x\n", this, hr);
    return hr;
}

HRESULT WINAPI CMyDocsFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    HRESULT hr = S_OK;
    LPWSTR pszPath;

    TRACE ("(%p)->(pidl=%p,0x%08x,%p)\n", this, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    pszPath = (LPWSTR)CoTaskMemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    ZeroMemory(pszPath, (MAX_PATH + 1) * sizeof(WCHAR));

    if (_ILIsMyDocuments (pidl))
    {
        if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING))
            wcscpy(pszPath, sPathTarget);
        else
            HCR_GetClassNameW(CLSID_MyDocuments, pszPath, MAX_PATH);
        TRACE("CP\n");
    }
    else if (_ILIsPidlSimple (pidl))
    {
        GUID const *clsid;

        if ((clsid = _ILGetGUIDPointer (pidl)))
        {
            if (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING)
            {
                int bWantsForParsing;

                /*
                 * We can only get a filesystem path from a shellfolder if the
                 *  value WantsFORPARSING in CLSID\\{...}\\shellfolder exists.
                 *
                 * Exception: The MyComputer folder doesn't have this key,
                 *   but any other filesystem backed folder it needs it.
                 */
                if (IsEqualIID (*clsid, CLSID_MyDocuments))
                {
                    bWantsForParsing = TRUE;
                }
                else
                {
                    /* get the "WantsFORPARSING" flag from the registry */
                    static const WCHAR clsidW[] = L"CLSID\\";
                    static const WCHAR shellfolderW[] = L"shellfolder";
                    static const WCHAR wantsForParsingW[] = L"WantsForParsing";
                    WCHAR szRegPath[100];
                    LONG r;

                    wcscpy (szRegPath, clsidW);
                    SHELL32_GUIDToStringW (*clsid, &szRegPath[6]);
                    wcscat (szRegPath, shellfolderW);
                    r = SHGetValueW(HKEY_CLASSES_ROOT, szRegPath,
                                    wantsForParsingW, NULL, NULL, NULL);
                    if (r == ERROR_SUCCESS)
                        bWantsForParsing = TRUE;
                    else
                        bWantsForParsing = FALSE;
                }

                if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                        bWantsForParsing)
                {
                    /*
                     * we need the filesystem path to the destination folder.
                     * Only the folder itself can know it
                     */
                    hr = SHELL32_GetDisplayNameOfChild (this, pidl, dwFlags,
                                                        pszPath,
                                                        MAX_PATH);
                    TRACE("CP\n");
                }
                else
                {
                    /* parsing name like ::{...} */
                    pszPath[0] = ':';
                    pszPath[1] = ':';
                    SHELL32_GUIDToStringW (*clsid, &pszPath[2]);
                    TRACE("CP\n");
                }
            }
            else
            {
                /* user friendly name */
                HCR_GetClassNameW (*clsid, pszPath, MAX_PATH);
                TRACE("CP\n");
            }
        }
        else
        {
            int cLen = 0;

            /* file system folder or file rooted at the desktop */
            if ((GET_SHGDN_FOR(dwFlags) == SHGDN_FORPARSING) &&
                    (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER))
            {
                lstrcpynW(pszPath, sPathTarget, MAX_PATH - 1);
                TRACE("CP %s\n", debugstr_w(pszPath));
            }

            if (!_ILIsDesktop(pidl))
            {
                PathAddBackslashW(pszPath);
                cLen = wcslen(pszPath);
                _ILSimpleGetTextW(pidl, pszPath + cLen, MAX_PATH - cLen);
                if (!_ILIsFolder(pidl))
                {
                    SHELL_FS_ProcessDisplayFilename(pszPath, dwFlags);
                    TRACE("CP\n");
                }
            }
        }
    }
    else
    {
        /* a complex pidl, let the subfolder do the work */
        hr = SHELL32_GetDisplayNameOfChild (this, pidl, dwFlags,
                                            pszPath, MAX_PATH);
        TRACE("CP\n");
    }

    if (SUCCEEDED(hr))
    {
        strRet->uType = STRRET_WSTR;
        strRet->pOleStr = pszPath;
    }
    else
        CoTaskMemFree(pszPath);

    TRACE ("-- (%p)->(%s,0x%08x)\n", this, debugstr_w(strRet->pOleStr), hr);
    return hr;
}

HRESULT WINAPI CMyDocsFolder::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,    /* simple pidl */
                                        LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    FIXME ("(%p)->(%p,pidl=%p,%s,%u,%p)\n", this, hwndOwner, pidl,
           debugstr_w (lpName), dwFlags, pPidlOut);

    return E_FAIL;
}

HRESULT WINAPI CMyDocsFolder::GetDefaultSearchGUID(GUID *pguid)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CMyDocsFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CMyDocsFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    TRACE ("(%p)\n", this);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

HRESULT WINAPI CMyDocsFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    TRACE ("(%p)\n", this);

    if (!pcsFlags || iColumn >= MYDOCUMENTSSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    *pcsFlags = MyDocumentsSFHeader[iColumn].pcsFlags;

    return S_OK;
}

HRESULT WINAPI CMyDocsFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    FIXME ("(%p)\n", this);

    return E_NOTIMPL;
}

HRESULT WINAPI CMyDocsFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd)
{
    HRESULT hr = S_OK;

    TRACE ("(%p)->(%p %i %p)\n", this, pidl, iColumn, psd);

    if (!psd || iColumn >= MYDOCUMENTSSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    if (!pidl)
    {
        psd->fmt = MyDocumentsSFHeader[iColumn].fmt;
        psd->cxChar = MyDocumentsSFHeader[iColumn].cxChar;
        psd->str.uType = STRRET_CSTR;
        LoadStringA (shell32_hInstance, MyDocumentsSFHeader[iColumn].colnameid,
                     psd->str.cStr, MAX_PATH);
        return S_OK;
    }

    /* the data from the pidl */
    psd->str.uType = STRRET_CSTR;
    switch (iColumn)
    {
        case 0:        /* name */
            hr = GetDisplayNameOf(pidl,
                                  SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
            break;
        case 1:        /* size */
            _ILGetFileSize (pidl, psd->str.cStr, MAX_PATH);
            break;
        case 2:        /* type */
            _ILGetFileType (pidl, psd->str.cStr, MAX_PATH);
            break;
        case 3:        /* date */
            _ILGetFileDate (pidl, psd->str.cStr, MAX_PATH);
            break;
        case 4:        /* attributes */
            _ILGetFileAttributes (pidl, psd->str.cStr, MAX_PATH);
            break;
    }

    return hr;
}

HRESULT WINAPI CMyDocsFolder::MapColumnToSCID (UINT column, SHCOLUMNID *pscid)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CMyDocsFolder::GetClassID(CLSID *lpClassId)
{
    static GUID const CLSID_MyDocuments =
    { 0x450d8fba, 0xad25, 0x11d0, {0x98, 0xa8, 0x08, 0x00, 0x36, 0x1b, 0x11, 0x03} };

    TRACE ("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    memcpy(lpClassId, &CLSID_MyDocuments, sizeof(GUID));

    return S_OK;
}

HRESULT WINAPI CMyDocsFolder::Initialize(LPCITEMIDLIST pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    return E_NOTIMPL;
}

HRESULT WINAPI CMyDocsFolder::GetCurFolder(LPITEMIDLIST *pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    if (!pidl) return E_POINTER;
    *pidl = ILClone (pidlRoot);
    return S_OK;
}

HRESULT WINAPI CMyDocsFolder::DragEnter(IDataObject *pDataObject,
                                    DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    return mFSDropTarget->DragEnter(pDataObject, dwKeyState, pt, pdwEffect);
}

HRESULT WINAPI CMyDocsFolder::DragOver(DWORD dwKeyState, POINTL pt,
                                   DWORD *pdwEffect)
{
    return mFSDropTarget->DragOver(dwKeyState, pt, pdwEffect);
}

HRESULT WINAPI CMyDocsFolder::DragLeave()
{
    return mFSDropTarget->DragLeave();
}

HRESULT WINAPI CMyDocsFolder::Drop(IDataObject *pDataObject,
                               DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    return mFSDropTarget->Drop(pDataObject, dwKeyState, pt, pdwEffect);
}