
/*
 *    Virtual MyDocuments Folder
 *
 *    Copyright 2007    Johannes Anderwald
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

/***********************************************************************
*     MyDocumentsfolder implementation
*/

typedef struct {
    const IShellFolder2Vtbl *lpVtbl;
    const IPersistFolder2Vtbl *lpPF2;
    LONG ref;

    /* both paths are parsible from the MyDocuments */
    LPWSTR sPathTarget;     /* complete path to target used for enumeration and ChangeNotify */
    LPITEMIDLIST pidlRoot;  /* absolute pidl */

    UINT cfShellIDList;        /* clipboardformat for IDropTarget */
    BOOL fAcceptFmt;        /* flag for pending Drop */
} IGenericSFImpl, *LPIGenericSFImpl;

#define _IUnknown_(This)    (IShellFolder*)&(This->lpVtbl)
#define _IShellFolder_(This)    (IShellFolder*)&(This->lpVtbl)

static const shvheader MyDocumentsSFHeader[] = {
    {IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12},
    {IDS_SHV_COLUMN5, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 5}
};

#define MYDOCUMENTSSHELLVIEWCOLUMNS 5

/**************************************************************************
 *    ISF_MyDocuments_fnQueryInterface
 *
 * NOTES supports not IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_MyDocuments_fnQueryInterface(
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
    else if (IsEqualIID (riid, &IID_IPersistFolder) ||
             IsEqualIID (riid, &IID_IPersistFolder2))
    {
        *ppvObj = &This->lpPF2;
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

static ULONG WINAPI ISF_MyDocuments_fnAddRef (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI ISF_MyDocuments_fnRelease (IShellFolder2 * iface)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount + 1);

    if (!refCount)
    {
        TRACE ("-- destroying IShellFolder(%p)\n", This);
        SHFree (This->pidlRoot);
        HeapFree(GetProcessHeap(), 0, This->sPathTarget);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refCount;
}

static
HRESULT
WINAPI ISF_MyDocuments_fnParseDisplayName (IShellFolder2 * iface,
                HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
                DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    WCHAR szElement[MAX_PATH];
    LPCWSTR szNext = NULL;
    LPITEMIDLIST pidlTemp = NULL;
    HRESULT hr = S_OK;
    CLSID clsid;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
           This, hwndOwner, pbc, lpszDisplayName, debugstr_w(lpszDisplayName),
           pchEaten, ppidl, pdwAttributes);

    if (!lpszDisplayName || !ppidl)
        return E_INVALIDARG;

    *ppidl = 0;

    if (pchEaten)
        *pchEaten = 0;        /* strange but like the original */

    if (lpszDisplayName[0] == ':' && lpszDisplayName[1] == ':')
    {
        szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);
        TRACE ("-- element: %s\n", debugstr_w (szElement));
        CLSIDFromString (szElement + 2, &clsid);
        pidlTemp = _ILCreateGuid (PT_GUID, &clsid);
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
            lstrcpynW(szPath, This->sPathTarget, MAX_PATH);
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
            hr = SHELL32_ParseNextElement(iface, hwndOwner, pbc,
                    &pidlTemp, (LPOLESTR) szNext, pchEaten, pdwAttributes);
        }
        else
        {
            if (pdwAttributes && *pdwAttributes)
                hr = SHELL32_GetItemAttributes(_IShellFolder_ (This),
                                               pidlTemp, pdwAttributes);
        }
    }

    *ppidl = pidlTemp;

    TRACE ("(%p)->(-- ret=0x%08x)\n", This, hr);

    return hr;
}

/**************************************************************************
 *  CreateMyDocumentsEnumList()
 */
static BOOL CreateMyDocumentsEnumList(IEnumIDList *list, DWORD dwFlags)
{
    BOOL ret = TRUE;
    WCHAR szPath[MAX_PATH] = {0};

    TRACE("(%p)->(flags=0x%08x)\n", list, dwFlags);

   /* enumerate the elements in %windir%\desktop */
    ret = SHGetSpecialFolderPathW(0, szPath, CSIDL_PERSONAL, FALSE);
    ret = ret && CreateFolderEnumList(list, szPath, dwFlags);

    return ret;
}

/**************************************************************************
 *        ISF_MyDocuments_fnEnumObjects
 */
static HRESULT WINAPI ISF_MyDocuments_fnEnumObjects (IShellFolder2 * iface,
                HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(HWND=%p flags=0x%08x pplist=%p)\n",
           This, hwndOwner, dwFlags, ppEnumIDList);

    *ppEnumIDList = IEnumIDList_Constructor();
    if (*ppEnumIDList)
        CreateMyDocumentsEnumList(*ppEnumIDList, dwFlags);

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    return *ppEnumIDList ? S_OK : E_OUTOFMEMORY;
}

/**************************************************************************
 *        ISF_MyDocuments_fnBindToObject
 */
static HRESULT WINAPI ISF_MyDocuments_fnBindToObject (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n",
           This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild( This->pidlRoot, This->sPathTarget, pidl, riid, ppvOut );
}

/**************************************************************************
 *    ISF_MyDocuments_fnBindToStorage
 */
static
HRESULT
WINAPI
ISF_MyDocuments_fnBindToStorage (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n",
           This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
 *     ISF_MyDocuments_fnCompareIDs
 */
static HRESULT WINAPI ISF_MyDocuments_fnCompareIDs (IShellFolder2 * iface,
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
 *    ISF_MyDocuments_fnCreateViewObject
 */
static HRESULT WINAPI ISF_MyDocuments_fnCreateViewObject (IShellFolder2 * iface,
                              HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n",
           This, hwndOwner, shdebugstr_guid (riid), ppvOut);

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
 *  ISF_MyDocuments_fnGetAttributesOf
 */
static HRESULT WINAPI ISF_MyDocuments_fnGetAttributesOf (IShellFolder2 * iface,
                UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    HRESULT hr = S_OK;
    static const DWORD dwMyDocumentsAttributes =
        SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR | SFGAO_CANCOPY |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n",
           This, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

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
 *    ISF_MyDocuments_fnGetUIObjectOf
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
static HRESULT WINAPI ISF_MyDocuments_fnGetUIObjectOf (IShellFolder2 * iface,
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

static
HRESULT
WINAPI ISF_MyDocuments_fnGetDisplayNameOf (IShellFolder2 * iface,
                LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    HRESULT hr = S_OK;
    LPWSTR pszPath;

    TRACE ("(%p)->(pidl=%p,0x%08x,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    pszPath = CoTaskMemAlloc((MAX_PATH +1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    ZeroMemory(pszPath, (MAX_PATH +1) * sizeof(WCHAR));

    if (_ILIsMyDocuments (pidl))
    {
        if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
            (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING))
            wcscpy(pszPath, This->sPathTarget);
        else
            HCR_GetClassNameW(&CLSID_MyDocuments, pszPath, MAX_PATH);
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
                if (IsEqualIID (clsid, &CLSID_MyDocuments))
                {
                    bWantsForParsing = TRUE;
                }
                else
                {
                    /* get the "WantsFORPARSING" flag from the registry */
                    static const WCHAR clsidW[] =
                     { 'C','L','S','I','D','\\',0 };
                    static const WCHAR shellfolderW[] =
                     { '\\','s','h','e','l','l','f','o','l','d','e','r',0 };
                    static const WCHAR wantsForParsingW[] =
                     { 'W','a','n','t','s','F','o','r','P','a','r','s','i','n',
                     'g',0 };
                    WCHAR szRegPath[100];
                    LONG r;

                    wcscpy (szRegPath, clsidW);
                    SHELL32_GUIDToStringW (clsid, &szRegPath[6]);
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
                    hr = SHELL32_GetDisplayNameOfChild (iface, pidl, dwFlags,
                                                        pszPath,
                                                        MAX_PATH);
                            TRACE("CP\n");
                }
                else
                {
                    /* parsing name like ::{...} */
                    pszPath[0] = ':';
                    pszPath[1] = ':';
                    SHELL32_GUIDToStringW (clsid, &pszPath[2]);
                            TRACE("CP\n");
                }
            }
            else
            {
                /* user friendly name */
                HCR_GetClassNameW (clsid, pszPath, MAX_PATH);
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
                lstrcpynW(pszPath, This->sPathTarget, MAX_PATH - 1);
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
        hr = SHELL32_GetDisplayNameOfChild (iface, pidl, dwFlags,
                                            pszPath, MAX_PATH);
                        TRACE("CP\n");                                
    }

    if (SUCCEEDED(hr))
    {
        strRet->uType = STRRET_WSTR;
        strRet->u.pOleStr = pszPath;
    }
    else
        CoTaskMemFree(pszPath);

    TRACE ("-- (%p)->(%s,0x%08x)\n", This, debugstr_w(strRet->u.pOleStr), hr);
    return hr;
}

static HRESULT WINAPI ISF_MyDocuments_fnSetNameOf (IShellFolder2 * iface,
                HWND hwndOwner, LPCITEMIDLIST pidl,    /* simple pidl */
                LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)->(%p,pidl=%p,%s,%u,%p)\n", This, hwndOwner, pidl,
           debugstr_w (lpName), dwFlags, pPidlOut);

    return E_FAIL;
}

static HRESULT WINAPI ISF_MyDocuments_fnGetDefaultSearchGUID(IShellFolder2 *iface,
                GUID * pguid)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_MyDocuments_fnEnumSearches (IShellFolder2 *iface,
                IEnumExtraSearch ** ppenum)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static
HRESULT
WINAPI
ISF_MyDocuments_fnGetDefaultColumn (IShellFolder2 * iface,
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
static
HRESULT
WINAPI
ISF_MyDocuments_fnGetDefaultColumnState (IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    TRACE ("(%p)\n", This);

    if (!pcsFlags || iColumn >= MYDOCUMENTSSHELLVIEWCOLUMNS)
    return E_INVALIDARG;

    *pcsFlags = MyDocumentsSFHeader[iColumn].pcsFlags;

    return S_OK;
}

static
HRESULT
WINAPI
ISF_MyDocuments_fnGetDetailsEx (IShellFolder2 * iface, LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);

    return E_NOTIMPL;
}

static 
HRESULT
WINAPI
ISF_MyDocuments_fnGetDetailsOf (IShellFolder2 * iface, LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;

    HRESULT hr = S_OK;

    TRACE ("(%p)->(%p %i %p)\n", This, pidl, iColumn, psd);

    if (!psd || iColumn >= MYDOCUMENTSSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    if (!pidl)
    {
        psd->fmt = MyDocumentsSFHeader[iColumn].fmt;
        psd->cxChar = MyDocumentsSFHeader[iColumn].cxChar;
        psd->str.uType = STRRET_CSTR;
        LoadStringA (shell32_hInstance, MyDocumentsSFHeader[iColumn].colnameid,
                     psd->str.u.cStr, MAX_PATH);
        return S_OK;
    }

    /* the data from the pidl */
    psd->str.uType = STRRET_CSTR;
    switch (iColumn)
    {
    case 0:        /* name */
        hr = IShellFolder_GetDisplayNameOf(iface, pidl,
                   SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
        break;
    case 1:        /* size */
        _ILGetFileSize (pidl, psd->str.u.cStr, MAX_PATH);
        break;
    case 2:        /* type */
        _ILGetFileType (pidl, psd->str.u.cStr, MAX_PATH);
        break;
    case 3:        /* date */
        _ILGetFileDate (pidl, psd->str.u.cStr, MAX_PATH);
        break;
    case 4:        /* attributes */
        _ILGetFileAttributes (pidl, psd->str.u.cStr, MAX_PATH);
        break;
    }

    return hr;
}

static 
HRESULT
WINAPI
ISF_MyDocuments_fnMapColumnToSCID (IShellFolder2 * iface, UINT column, SHCOLUMNID * pscid)
{
    IGenericSFImpl *This = (IGenericSFImpl *)iface;
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static const IShellFolder2Vtbl vt_MCFldr_ShellFolder2 =
{
    ISF_MyDocuments_fnQueryInterface,
    ISF_MyDocuments_fnAddRef,
    ISF_MyDocuments_fnRelease,
    ISF_MyDocuments_fnParseDisplayName,
    ISF_MyDocuments_fnEnumObjects,
    ISF_MyDocuments_fnBindToObject,
    ISF_MyDocuments_fnBindToStorage,
    ISF_MyDocuments_fnCompareIDs,
    ISF_MyDocuments_fnCreateViewObject,
    ISF_MyDocuments_fnGetAttributesOf,
    ISF_MyDocuments_fnGetUIObjectOf,
    ISF_MyDocuments_fnGetDisplayNameOf,
    ISF_MyDocuments_fnSetNameOf,
    /* ShellFolder2 */
    ISF_MyDocuments_fnGetDefaultSearchGUID,
    ISF_MyDocuments_fnEnumSearches,
    ISF_MyDocuments_fnGetDefaultColumn,
    ISF_MyDocuments_fnGetDefaultColumnState,
    ISF_MyDocuments_fnGetDetailsEx,
    ISF_MyDocuments_fnGetDetailsOf,
    ISF_MyDocuments_fnMapColumnToSCID
};

static LPIGenericSFImpl __inline impl_from_IPersistFolder2( IPersistFolder2 *iface )
{
    return (LPIGenericSFImpl)((char*)iface - FIELD_OFFSET(IGenericSFImpl, lpPF2));
}

static HRESULT WINAPI
ISF_MyDocuments_PersistFolder2_fnQueryInterface (IPersistFolder2 * iface, REFIID iid, LPVOID * ppvObj)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)\n", This);

    return ISF_MyDocuments_fnQueryInterface ((IShellFolder2*)This, iid, ppvObj);
}

static ULONG WINAPI
ISF_MyDocuments_PersistFolder2_fnAddRef (IPersistFolder2 * iface)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return ISF_MyDocuments_fnAddRef((IShellFolder2*)This);
}

static ULONG WINAPI
ISF_MyDocuments_PersistFolder2_fnRelease (IPersistFolder2 * iface)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)->(count=%u)\n", This, This->ref);

    return ISF_MyDocuments_fnRelease ((IShellFolder2*)This);
}

static HRESULT WINAPI
ISF_MyDocuments_PersistFolder2_fnGetClassID (IPersistFolder2 * iface, CLSID * lpClassId)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    static GUID const CLSID_MyDocuments = 
    { 0x450d8fba, 0xad25, 0x11d0, {0x98,0xa8,0x08,0x00,0x36,0x1b,0x11,0x03} };

    TRACE ("(%p)\n", This);

    if (!lpClassId)
        return E_POINTER;

    memcpy(lpClassId, &CLSID_MyDocuments, sizeof(GUID));

    return S_OK;
}
static HRESULT WINAPI
ISF_MyDocuments_PersistFolder2_fnInitialize (IPersistFolder2 * iface, LPCITEMIDLIST pidl)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    return E_NOTIMPL;
}

static HRESULT WINAPI
ISF_MyDocuments_PersistFolder2_fnGetCurFolder (IPersistFolder2 * iface,
                                       LPITEMIDLIST * pidl)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    if (!pidl) return E_POINTER;
    *pidl = ILClone (This->pidlRoot);
    return S_OK;
}

static const IPersistFolder2Vtbl vt_FSFldr_PersistFolder2 =
{
    ISF_MyDocuments_PersistFolder2_fnQueryInterface,
    ISF_MyDocuments_PersistFolder2_fnAddRef,
    ISF_MyDocuments_PersistFolder2_fnRelease,
    ISF_MyDocuments_PersistFolder2_fnGetClassID,
    ISF_MyDocuments_PersistFolder2_fnInitialize,
    ISF_MyDocuments_PersistFolder2_fnGetCurFolder,
};


/**************************************************************************
 *    ISF_MyDocuments_Constructor
 */
HRESULT WINAPI ISF_MyDocuments_Constructor (
                IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    static IGenericSFImpl *cached_sf;
    WCHAR szMyPath[MAX_PATH];

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (!cached_sf)
    {
        IGenericSFImpl *sf;

        if (!SHGetSpecialFolderPathW( 0, szMyPath, CSIDL_PERSONAL, TRUE ))
            return E_UNEXPECTED;

        sf = LocalAlloc( LMEM_ZEROINIT, sizeof (IGenericSFImpl) );
        if (!sf)
            return E_OUTOFMEMORY;

        sf->ref = 1;
        sf->lpVtbl = &vt_MCFldr_ShellFolder2;
        sf->lpPF2 = &vt_FSFldr_PersistFolder2;
        sf->pidlRoot = _ILCreateMyDocuments();    /* my qualified pidl */
        sf->sPathTarget = SHAlloc( (wcslen(szMyPath) + 1)*sizeof(WCHAR) );
        wcscpy( sf->sPathTarget, szMyPath );

        if (InterlockedCompareExchangePointer((void *)&cached_sf, sf, NULL) != NULL)
        {
            /* some other thread already been here */
            SHFree( sf->pidlRoot );
            SHFree( sf->sPathTarget );
            LocalFree( sf );
        }
    }

    return IUnknown_QueryInterface( _IUnknown_(cached_sf), riid, ppv );
}
