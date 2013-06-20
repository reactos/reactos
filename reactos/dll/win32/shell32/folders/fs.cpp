
/*
 * file system folder
 *
 * Copyright 1997             Marcus Meissner
 * Copyright 1998, 1999, 2002 Juergen Schmied
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

/*
CFileSysEnum should do an initial FindFirstFile and do a FindNextFile as each file is
returned by Next. When the enumerator is created, it can do numerous additional operations
including formatting a drive, reconnecting a network share drive, and requesting a disk
be inserted in a removable drive.
*/

/***********************************************************************
*   IShellFolder implementation
*/

class CFileSysEnum :
    public IEnumIDListImpl
{
    private:
    public:
        CFileSysEnum();
        ~CFileSysEnum();
        HRESULT WINAPI Initialize(LPWSTR sPathTarget, DWORD dwFlags);

        BEGIN_COM_MAP(CFileSysEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

CFileSysEnum::CFileSysEnum()
{
}

CFileSysEnum::~CFileSysEnum()
{
}

HRESULT WINAPI CFileSysEnum::Initialize(LPWSTR sPathTarget, DWORD dwFlags)
{
    return CreateFolderEnumList(sPathTarget, dwFlags);
}

/**************************************************************************
* registers clipboardformat once
*/
void CFSFolder::SF_RegisterClipFmt()
{
    TRACE ("(%p)\n", this);

    if (!cfShellIDList)
        cfShellIDList = RegisterClipboardFormatW(CFSTR_SHELLIDLIST);
}

CFSFolder::CFSFolder()
{
    pclsid = (CLSID *)&CLSID_ShellFSFolder;
    sPathTarget = NULL;
    pidlRoot = NULL;
    cfShellIDList = 0;
    fAcceptFmt = FALSE;
}

CFSFolder::~CFSFolder()
{
    TRACE("-- destroying IShellFolder(%p)\n", this);

    SHFree(pidlRoot);
    SHFree(sPathTarget);
}


static const shvheader GenericSFHeader[] = {
    {IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12},
    {IDS_SHV_COLUMN5, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 5}
};

#define GENERICSHELLVIEWCOLUMNS 5

/**************************************************************************
 *  SHELL32_CreatePidlFromBindCtx  [internal]
 *
 *  If the caller bound File System Bind Data, assume it is the
 *   find data for the path.
 *  This allows binding of paths that don't exist.
 */
LPITEMIDLIST SHELL32_CreatePidlFromBindCtx(IBindCtx *pbc, LPCWSTR path)
{
    static WCHAR szfsbc[] = L"File System Bind Data";
    IFileSystemBindData *fsbd = NULL;
    LPITEMIDLIST pidl = NULL;
    IUnknown *param = NULL;
    WIN32_FIND_DATAW wfd;
    HRESULT r;

    TRACE("%p %s\n", pbc, debugstr_w(path));

    if (!pbc)
        return NULL;

    /* see if the caller bound File System Bind Data */
    r = pbc->GetObjectParam((LPOLESTR)szfsbc, &param);
    if (FAILED(r))
        return NULL;

    r = param->QueryInterface(IID_IFileSystemBindData,
                              (LPVOID*)&fsbd );
    if (SUCCEEDED(r))
    {
        r = fsbd->GetFindData(&wfd);
        if (SUCCEEDED(r))
        {
            lstrcpynW(&wfd.cFileName[0], path, MAX_PATH);
            pidl = _ILCreateFromFindDataW(&wfd);
        }
        fsbd->Release();
    }

    return pidl;
}

/**************************************************************************
* CFSFolder::ParseDisplayName {SHELL32}
*
* Parse a display name.
*
* PARAMS
*  hwndOwner       [in]  Parent window for any message's
*  pbc             [in]  optional FileSystemBindData context
*  lpszDisplayName [in]  Unicode displayname.
*  pchEaten        [out] (unicode) characters processed
*  ppidl           [out] complex pidl to item
*  pdwAttributes   [out] items attributes
*
* NOTES
*  Every folder tries to parse only its own (the leftmost) pidl and creates a
*  subfolder to evaluate the remaining parts.
*  Now we can parse into namespaces implemented by shell extensions
*
*  Behaviour on win98: lpszDisplayName=NULL -> crash
*                      lpszDisplayName="" -> returns mycoputer-pidl
*
* FIXME
*    pdwAttributes is not set
*    pchEaten is not set like in windows
*/
HRESULT WINAPI CFSFolder::ParseDisplayName(HWND hwndOwner,
        LPBC pbc,
        LPOLESTR lpszDisplayName,
        DWORD *pchEaten, LPITEMIDLIST *ppidl,
        DWORD *pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;
    LPCWSTR szNext = NULL;
    WCHAR szElement[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    LPITEMIDLIST pidlTemp = NULL;
    DWORD len;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
           this, hwndOwner, pbc, lpszDisplayName, debugstr_w (lpszDisplayName),
           pchEaten, ppidl, pdwAttributes);

    if (!ppidl)
        return E_INVALIDARG;

    if (!lpszDisplayName)
    {
        *ppidl = NULL;
        return E_INVALIDARG;
    }

    *ppidl = NULL;

    if (pchEaten)
        *pchEaten = 0; /* strange but like the original */

    pidlTemp = SHELL32_CreatePidlFromBindCtx(pbc, lpszDisplayName);
    if (!pidlTemp && *lpszDisplayName)
    {
        /* get the next element */
        szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);

        /* build the full pathname to the element */
        lstrcpynW(szPath, sPathTarget, MAX_PATH - 1);
        PathAddBackslashW(szPath);
        len = wcslen(szPath);
        lstrcpynW(szPath + len, szElement, MAX_PATH - len);

        /* get the pidl */
        hr = _ILCreateFromPathW(szPath, &pidlTemp);

        if (SUCCEEDED(hr))
        {
            if (szNext && *szNext)
            {
                /* try to analyse the next element */
                hr = SHELL32_ParseNextElement(this, hwndOwner, pbc,
                                              &pidlTemp, (LPOLESTR) szNext, pchEaten, pdwAttributes);
            }
            else
            {
                /* it's the last element */
                if (pdwAttributes && *pdwAttributes)
                    hr = SHELL32_GetItemAttributes(this, pidlTemp, pdwAttributes);
            }
        }
    }

    if (SUCCEEDED(hr))
        *ppidl = pidlTemp;
    else
        *ppidl = NULL;

    TRACE("(%p)->(-- pidl=%p ret=0x%08x)\n", this, ppidl ? *ppidl : 0, hr);

    return hr;
}

/**************************************************************************
* CFSFolder::EnumObjects
* PARAMETERS
*  HWND          hwndOwner,    //[in ] Parent Window
*  DWORD         grfFlags,     //[in ] SHCONTF enumeration mask
*  LPENUMIDLIST* ppenumIDList  //[out] IEnumIDList interface
*/
HRESULT WINAPI CFSFolder::EnumObjects(
    HWND hwndOwner,
    DWORD dwFlags,
    LPENUMIDLIST *ppEnumIDList)
{
    CComObject<CFileSysEnum> *theEnumerator;
    CComPtr<IEnumIDList>      result;
    HRESULT                   hResult;

    TRACE("(%p)->(HWND=%p flags=0x%08x pplist=%p)\n", this, hwndOwner, dwFlags, ppEnumIDList);

    if (ppEnumIDList == NULL)
        return E_POINTER;
    *ppEnumIDList = NULL;
    ATLTRY (theEnumerator = new CComObject<CFileSysEnum>);
    if (theEnumerator == NULL)
        return E_OUTOFMEMORY;
    hResult = theEnumerator->QueryInterface (IID_IEnumIDList, (void **)&result);
    if (FAILED (hResult))
    {
        delete theEnumerator;
        return hResult;
    }
    hResult = theEnumerator->Initialize (sPathTarget, dwFlags);
    if (FAILED (hResult))
        return hResult;
    *ppEnumIDList = result.Detach();

    TRACE("-- (%p)->(new ID List: %p)\n", this, *ppEnumIDList);

    return S_OK;
}

/**************************************************************************
* CFSFolder::BindToObject
* PARAMETERS
*  LPCITEMIDLIST pidl,       //[in ] relative pidl to open
*  LPBC          pbc,        //[in ] optional FileSystemBindData context
*  REFIID        riid,       //[in ] Initial Interface
*  LPVOID*       ppvObject   //[out] Interface*
*/
HRESULT WINAPI CFSFolder::BindToObject(
    LPCITEMIDLIST pidl,
    LPBC pbc,
    REFIID riid,
    LPVOID * ppvOut)
{
    TRACE("(%p)->(pidl=%p,%p,%s,%p)\n", this, pidl, pbc,
          shdebugstr_guid(&riid), ppvOut);

    return SHELL32_BindToChild(pidlRoot, sPathTarget, pidl, riid, ppvOut);
}

/**************************************************************************
*  CFSFolder::BindToStorage
* PARAMETERS
*  LPCITEMIDLIST pidl,       //[in ] complex pidl to store
*  LPBC          pbc,        //[in ] reserved
*  REFIID        riid,       //[in ] Initial storage interface
*  LPVOID*       ppvObject   //[out] Interface* returned
*/
HRESULT WINAPI CFSFolder::BindToStorage(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    LPVOID *ppvOut)
{
    FIXME("(%p)->(pidl=%p,%p,%s,%p) stub\n", this, pidl, pbcReserved,
          shdebugstr_guid (&riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
*  CFSFolder::CompareIDs
*/

HRESULT WINAPI CFSFolder::CompareIDs(LPARAM lParam,
                                     LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int nReturn;

    TRACE("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", this, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs(this, lParam, pidl1, pidl2);
    TRACE("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
* CFSFolder::CreateViewObject
*/
HRESULT WINAPI CFSFolder::CreateViewObject(HWND hwndOwner,
        REFIID riid, LPVOID * ppvOut)
{
    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n", this, hwndOwner, shdebugstr_guid (&riid),
           ppvOut);

    if (ppvOut)
    {
        *ppvOut = NULL;

        if (IsEqualIID (riid, IID_IDropTarget))
            hr = this->QueryInterface (IID_IDropTarget, ppvOut);
        else if (IsEqualIID (riid, IID_IContextMenu))
        {
            FIXME ("IContextMenu not implemented\n");
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
    }
    TRACE("-- (%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

/**************************************************************************
*  CFSFolder::GetAttributesOf
*
* PARAMETERS
*  UINT            cidl,     //[in ] num elements in pidl array
*  LPCITEMIDLIST*  apidl,    //[in ] simple pidl array
*  ULONG*          rgfInOut) //[out] result array
*
*/
HRESULT WINAPI CFSFolder::GetAttributesOf(UINT cidl,
        LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    HRESULT hr = S_OK;

    TRACE("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n", this, cidl, apidl,
          rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if(cidl == 0)
    {
        IShellFolder *psfParent = NULL;
        LPCITEMIDLIST rpidl = NULL;

        hr = SHBindToParent(pidlRoot, IID_IShellFolder, (LPVOID*)&psfParent, (LPCITEMIDLIST*)&rpidl);
        if(SUCCEEDED(hr))
        {
            SHELL32_GetItemAttributes (psfParent, rpidl, rgfInOut);
            psfParent->Release();
        }
    }
    else
    {
        while (cidl > 0 && *apidl)
        {
            pdump(*apidl);
            SHELL32_GetItemAttributes(this, *apidl, rgfInOut);
            apidl++;
            cidl--;
        }
    }
    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE("-- result=0x%08x\n", *rgfInOut);

    return hr;
}

/**************************************************************************
*  CFSFolder::GetUIObjectOf
*
* PARAMETERS
*  HWND           hwndOwner, //[in ] Parent window for any output
*  UINT           cidl,      //[in ] array size
*  LPCITEMIDLIST* apidl,     //[in ] simple pidl array
*  REFIID         riid,      //[in ] Requested Interface
*  UINT*          prgfInOut, //[   ] reserved
*  LPVOID*        ppvObject) //[out] Resulting Interface
*
* NOTES
*  This function gets asked to return "view objects" for one or more (multiple
*  select) items:
*  The viewobject typically is an COM object with one of the following
*  interfaces:
*  IExtractIcon,IDataObject,IContextMenu
*  In order to support icon positions in the default Listview your DataObject
*  must implement the SetData method (in addition to GetData :) - the shell
*  passes a barely documented "Icon positions" structure to SetData when the
*  drag starts, and GetData's it if the drop is in another explorer window that
*  needs the positions.
*/
HRESULT WINAPI CFSFolder::GetUIObjectOf(HWND hwndOwner,
                                        UINT cidl, LPCITEMIDLIST * apidl, REFIID riid,
                                        UINT * prgfInOut, LPVOID * ppvOut)
{
    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
           this, hwndOwner, cidl, apidl, shdebugstr_guid (&riid), prgfInOut, ppvOut);

    if (ppvOut)
    {
        *ppvOut = NULL;

        if (IsEqualIID (riid, IID_IContextMenu) && (cidl >= 1))
            hr = CDefFolderMenu_Create2(pidlRoot, hwndOwner, cidl, apidl, (IShellFolder*)this, NULL, 0, NULL, (IContextMenu**)&pObj);
        else if (IsEqualIID (riid, IID_IDataObject))
        {
            if (cidl >= 1) {
                hr = IDataObject_Constructor (hwndOwner, pidlRoot, apidl, cidl, (IDataObject **)&pObj);
            }
            else
            {
                hr = IDataObject_Constructor (hwndOwner, pidlRoot, (LPCITEMIDLIST*)&pidlRoot, 1, (IDataObject **)&pObj);
            }
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
            hr = this->QueryInterface(IID_IDropTarget, (LPVOID*)&pObj);
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
    }
    TRACE("(%p)->hr=0x%08x\n", this, hr);
    return hr;
}

static const WCHAR AdvancedW[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
static const WCHAR HideFileExtW[] = L"HideFileExt";
static const WCHAR NeverShowExtW[] = L"NeverShowExt";

/******************************************************************************
 * SHELL_FS_HideExtension [Internal]
 *
 * Query the registry if the filename extension of a given path should be
 * hidden.
 *
 * PARAMS
 *  szPath [I] Relative or absolute path of a file
 *
 * RETURNS
 *  TRUE, if the filename's extension should be hidden
 *  FALSE, otherwise.
 */
BOOL SHELL_FS_HideExtension(LPWSTR szPath)
{
    HKEY hKey;
    DWORD dwData;
    DWORD dwDataSize = sizeof (DWORD);
    BOOL doHide = FALSE; /* The default value is FALSE (win98 at least) */

    if (!RegCreateKeyExW(HKEY_CURRENT_USER, AdvancedW, 0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, 0)) {
        if (!RegQueryValueExW(hKey, HideFileExtW, 0, 0, (LPBYTE) &dwData, &dwDataSize))
            doHide = dwData;
        RegCloseKey (hKey);
    }

    if (!doHide) {
        LPWSTR ext = PathFindExtensionW(szPath);

        if (*ext != '\0') {
            WCHAR classname[MAX_PATH];
            LONG classlen = sizeof(classname);

            if (!RegQueryValueW(HKEY_CLASSES_ROOT, ext, classname, &classlen))
                if (!RegOpenKeyW(HKEY_CLASSES_ROOT, classname, &hKey)) {
                    if (!RegQueryValueExW(hKey, NeverShowExtW, 0, NULL, NULL, NULL))
                        doHide = TRUE;
                    RegCloseKey(hKey);
                }
        }
    }
    return doHide;
}

void SHELL_FS_ProcessDisplayFilename(LPWSTR szPath, DWORD dwFlags)
{
    /*FIXME: MSDN also mentions SHGDN_FOREDITING which is not yet handled. */
    if (!(dwFlags & SHGDN_FORPARSING) &&
            ((dwFlags & SHGDN_INFOLDER) || (dwFlags == SHGDN_NORMAL))) {
        if (SHELL_FS_HideExtension(szPath) && szPath[0] != '.')
            PathRemoveExtensionW(szPath);
    }
}

/**************************************************************************
*  CFSFolder::GetDisplayNameOf
*  Retrieves the display name for the specified file object or subfolder
*
* PARAMETERS
*  LPCITEMIDLIST pidl,    //[in ] complex pidl to item
*  DWORD         dwFlags, //[in ] SHGNO formatting flags
*  LPSTRRET      lpName)  //[out] Returned display name
*
* FIXME
*  if the name is in the pidl the ret value should be a STRRET_OFFSET
*/

HRESULT WINAPI CFSFolder::GetDisplayNameOf(LPCITEMIDLIST pidl,
        DWORD dwFlags, LPSTRRET strRet)
{
    LPWSTR pszPath;

    HRESULT hr = S_OK;
    int len = 0;

    TRACE("(%p)->(pidl=%p,0x%08x,%p)\n", this, pidl, dwFlags, strRet);
    pdump(pidl);

    if (!pidl || !strRet)
        return E_INVALIDARG;

    pszPath = (LPWSTR)CoTaskMemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    if (_ILIsDesktop(pidl)) /* empty pidl */
    {
        if ((GET_SHGDN_FOR(dwFlags) & SHGDN_FORPARSING) &&
                (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER))
        {
            if (sPathTarget)
                lstrcpynW(pszPath, sPathTarget, MAX_PATH);
        }
        else
            hr = E_INVALIDARG; /* pidl has to contain exactly one non null SHITEMID */
    }
    else if (_ILIsPidlSimple(pidl))
    {
        if ((GET_SHGDN_FOR(dwFlags) & SHGDN_FORPARSING) &&
                (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER) &&
                sPathTarget)
        {
            lstrcpynW(pszPath, sPathTarget, MAX_PATH);
            PathAddBackslashW(pszPath);
            len = wcslen(pszPath);
        }
        _ILSimpleGetTextW(pidl, pszPath + len, MAX_PATH + 1 - len);
        if (!_ILIsFolder(pidl)) SHELL_FS_ProcessDisplayFilename(pszPath, dwFlags);
    } else
        hr = SHELL32_GetDisplayNameOfChild(this, pidl, dwFlags, pszPath, MAX_PATH);

    if (SUCCEEDED(hr)) {
        /* Win9x always returns ANSI strings, NT always returns Unicode strings */
        if (GetVersion() & 0x80000000)
        {
            strRet->uType = STRRET_CSTR;
            if (!WideCharToMultiByte(CP_ACP, 0, pszPath, -1, strRet->cStr, MAX_PATH,
                                     NULL, NULL))
                strRet->cStr[0] = '\0';
            CoTaskMemFree(pszPath);
        }
        else
        {
            strRet->uType = STRRET_WSTR;
            strRet->pOleStr = pszPath;
        }
    } else
        CoTaskMemFree(pszPath);

    TRACE ("-- (%p)->(%s)\n", this, strRet->uType == STRRET_CSTR ? strRet->cStr : debugstr_w(strRet->pOleStr));
    return hr;
}

/**************************************************************************
*  CFSFolder::SetNameOf
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
HRESULT WINAPI CFSFolder::SetNameOf(
    HWND hwndOwner,
    LPCITEMIDLIST pidl,
    LPCOLESTR lpName,
    DWORD dwFlags,
    LPITEMIDLIST * pPidlOut)
{
    WCHAR szSrc[MAX_PATH + 1], szDest[MAX_PATH + 1];
    LPWSTR ptr;
    BOOL bIsFolder = _ILIsFolder (ILFindLastID (pidl));

    TRACE ("(%p)->(%p,pidl=%p,%s,%u,%p)\n", this, hwndOwner, pidl,
           debugstr_w (lpName), dwFlags, pPidlOut);

    /* build source path */
    lstrcpynW(szSrc, sPathTarget, MAX_PATH);
    ptr = PathAddBackslashW (szSrc);
    if (ptr)
        _ILSimpleGetTextW (pidl, ptr, MAX_PATH + 1 - (ptr - szSrc));

    /* build destination path */
    if (dwFlags == SHGDN_NORMAL || dwFlags & SHGDN_INFOLDER) {
        lstrcpynW(szDest, sPathTarget, MAX_PATH);
        ptr = PathAddBackslashW (szDest);
        if (ptr)
            lstrcpynW(ptr, lpName, MAX_PATH + 1 - (ptr - szDest));
    } else
        lstrcpynW(szDest, lpName, MAX_PATH);

    if(!(dwFlags & SHGDN_FORPARSING) && SHELL_FS_HideExtension(szSrc)) {
        WCHAR *ext = PathFindExtensionW(szSrc);
        if(*ext != '\0') {
            INT len = wcslen(szDest);
            lstrcpynW(szDest + len, ext, MAX_PATH - len);
        }
    }

    TRACE ("src=%s dest=%s\n", debugstr_w(szSrc), debugstr_w(szDest));
    if (!memcmp(szSrc, szDest, (wcslen(szDest) + 1) * sizeof(WCHAR)))
    {
        /* src and destination is the same */
        HRESULT hr = S_OK;
        if (pPidlOut)
            hr = _ILCreateFromPathW(szDest, pPidlOut);

        return hr;
    }


    if (MoveFileW (szSrc, szDest))
    {
        HRESULT hr = S_OK;

        if (pPidlOut)
            hr = _ILCreateFromPathW(szDest, pPidlOut);

        SHChangeNotify (bIsFolder ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM,
                        SHCNF_PATHW, szSrc, szDest);

        return hr;
    }

    return E_FAIL;
}

HRESULT WINAPI CFSFolder::GetDefaultSearchGUID(GUID * pguid)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CFSFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CFSFolder::GetDefaultColumn(DWORD dwRes,
        ULONG * pSort, ULONG * pDisplay)
{
    TRACE ("(%p)\n", this);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;

    return S_OK;
}

HRESULT WINAPI CFSFolder::GetDefaultColumnState(UINT iColumn,
        DWORD * pcsFlags)
{
    TRACE ("(%p)\n", this);

    if (!pcsFlags || iColumn >= GENERICSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    *pcsFlags = GenericSFHeader[iColumn].pcsFlags;

    return S_OK;
}

HRESULT WINAPI CFSFolder::GetDetailsEx(LPCITEMIDLIST pidl,
                                       const SHCOLUMNID * pscid, VARIANT * pv)
{
    FIXME ("(%p)\n", this);

    return E_NOTIMPL;
}

HRESULT WINAPI CFSFolder::GetDetailsOf(LPCITEMIDLIST pidl,
                                       UINT iColumn, SHELLDETAILS * psd)
{
    HRESULT hr = E_FAIL;

    TRACE ("(%p)->(%p %i %p)\n", this, pidl, iColumn, psd);

    if (!psd || iColumn >= GENERICSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    if (!pidl)
    {
        /* the header titles */
        psd->fmt = GenericSFHeader[iColumn].fmt;
        psd->cxChar = GenericSFHeader[iColumn].cxChar;
        psd->str.uType = STRRET_CSTR;
        LoadStringA(shell32_hInstance, GenericSFHeader[iColumn].colnameid,
                    psd->str.cStr, MAX_PATH);
        return S_OK;
    }
    else
    {
        hr = S_OK;
        psd->str.uType = STRRET_CSTR;
        /* the data from the pidl */
        switch (iColumn)
        {
            case 0:                /* name */
                hr = GetDisplayNameOf (pidl,
                                       SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
                break;
            case 1:                /* size */
                _ILGetFileSize(pidl, psd->str.cStr, MAX_PATH);
                break;
            case 2:                /* type */
                _ILGetFileType(pidl, psd->str.cStr, MAX_PATH);
                break;
            case 3:                /* date */
                _ILGetFileDate(pidl, psd->str.cStr, MAX_PATH);
                break;
            case 4:                /* attributes */
                _ILGetFileAttributes(pidl, psd->str.cStr, MAX_PATH);
                break;
        }
    }

    return hr;
}

HRESULT WINAPI CFSFolder::MapColumnToSCID (UINT column,
        SHCOLUMNID * pscid)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

/****************************************************************************
 * ISFHelper for IShellFolder implementation
 */

/****************************************************************************
 * CFSFolder::GetUniqueName
 *
 * creates a unique folder name
 */

HRESULT WINAPI CFSFolder::GetUniqueName(LPWSTR pwszName, UINT uLen)
{
    IEnumIDList *penum;
    HRESULT hr;
    WCHAR wszText[MAX_PATH];
    WCHAR wszNewFolder[25];
    const WCHAR wszFormat[] = L"%s %d";

    LoadStringW(shell32_hInstance, IDS_NEWFOLDER, wszNewFolder, _countof(wszNewFolder));

    TRACE ("(%p)(%p %u)\n", this, pwszName, uLen);

    if (uLen < _countof(wszNewFolder) + 3)
        return E_POINTER;

    lstrcpynW (pwszName, wszNewFolder, uLen);

    hr = EnumObjects(0, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &penum);
    if (penum)
    {
        LPITEMIDLIST pidl;
        DWORD dwFetched;
        int i = 1;

next:
        penum->Reset ();
        while (S_OK == penum->Next(1, &pidl, &dwFetched) && dwFetched)
        {
            _ILSimpleGetTextW(pidl, wszText, MAX_PATH);
            if (0 == lstrcmpiW(wszText, pwszName))
            {
                _snwprintf(pwszName, uLen, wszFormat, wszNewFolder, i++);
                if (i > 99)
                {
                    hr = E_FAIL;
                    break;
                }
                goto next;
            }
        }

        penum->Release();
    }
    return hr;
}

/****************************************************************************
 * CFSFolder::AddFolder
 *
 * adds a new folder.
 */

HRESULT WINAPI CFSFolder::AddFolder(HWND hwnd, LPCWSTR pwszName,
                                    LPITEMIDLIST * ppidlOut)
{
    WCHAR wszNewDir[MAX_PATH];
    DWORD bRes;
    HRESULT hres = E_FAIL;

    TRACE ("(%p)(%s %p)\n", this, debugstr_w(pwszName), ppidlOut);

    wszNewDir[0] = 0;
    if (sPathTarget)
        lstrcpynW(wszNewDir, sPathTarget, MAX_PATH);
    PathAppendW(wszNewDir, pwszName);

    bRes = CreateDirectoryW(wszNewDir, NULL);
    if (bRes)
    {
        SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW, wszNewDir, NULL);

        hres = S_OK;

        if (ppidlOut)
            hres = _ILCreateFromPathW(wszNewDir, ppidlOut);
    }
    else
    {
        WCHAR wszText[128 + MAX_PATH];
        WCHAR wszTempText[128];
        WCHAR wszCaption[256];

        /* Cannot Create folder because of permissions */
        LoadStringW(shell32_hInstance, IDS_CREATEFOLDER_DENIED, wszTempText,
                    _countof(wszTempText));
        LoadStringW(shell32_hInstance, IDS_CREATEFOLDER_CAPTION, wszCaption,
                     _countof(wszCaption));
        swprintf(wszText, wszTempText, wszNewDir);
        MessageBoxW(hwnd, wszText, wszCaption, MB_OK | MB_ICONEXCLAMATION);
    }

    return hres;
}

/****************************************************************************
 * BuildPathsList
 *
 * Builds a list of paths like the one used in SHFileOperation from a table of
 * PIDLs relative to the given base folder
 */
WCHAR *
BuildPathsList(LPCWSTR wszBasePath, int cidl, LPCITEMIDLIST *pidls)
{
    WCHAR *pwszPathsList;
    WCHAR *pwszListPos;
    int iPathLen, i;

    iPathLen = wcslen(wszBasePath);
    pwszPathsList = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR) * cidl + 1);
    pwszListPos = pwszPathsList;

    for (i = 0; i < cidl; i++)
    {
        if (!_ILIsFolder(pidls[i]) && !_ILIsValue(pidls[i]))
            continue;

        wcscpy(pwszListPos, wszBasePath);
        pwszListPos += iPathLen;
        /* FIXME: abort if path too long */
        _ILSimpleGetTextW(pidls[i], pwszListPos, MAX_PATH - iPathLen);
        pwszListPos += wcslen(pwszListPos) + 1;
    }
    *pwszListPos = 0;
    return pwszPathsList;
}

/****************************************************************************
 * CFSFolder::DeleteItems
 *
 * deletes items in folder
 */
HRESULT WINAPI CFSFolder::DeleteItems(UINT cidl, LPCITEMIDLIST *apidl)
{
    UINT i;
    SHFILEOPSTRUCTW op;
    WCHAR wszPath[MAX_PATH];
    WCHAR *wszPathsList;
    HRESULT ret;
    WCHAR *wszCurrentPath;

    TRACE ("(%p)(%u %p)\n", this, cidl, apidl);
    if (cidl == 0) return S_OK;

    if (sPathTarget)
        lstrcpynW(wszPath, sPathTarget, MAX_PATH);
    else
        wszPath[0] = '\0';
    PathAddBackslashW(wszPath);
    wszPathsList = BuildPathsList(wszPath, cidl, apidl);

    ZeroMemory(&op, sizeof(op));
    op.hwnd = GetActiveWindow();
    op.wFunc = FO_DELETE;
    op.pFrom = wszPathsList;
    op.fFlags = FOF_ALLOWUNDO;
    if (SHFileOperationW(&op))
    {
        WARN("SHFileOperation failed\n");
        ret = E_FAIL;
    }
    else
        ret = S_OK;

    /* we currently need to manually send the notifies */
    wszCurrentPath = wszPathsList;
    for (i = 0; i < cidl; i++)
    {
        LONG wEventId;

        if (_ILIsFolder(apidl[i]))
            wEventId = SHCNE_RMDIR;
        else if (_ILIsValue(apidl[i]))
            wEventId = SHCNE_DELETE;
        else
            continue;

        /* check if file exists */
        if (GetFileAttributesW(wszCurrentPath) == INVALID_FILE_ATTRIBUTES)
        {
            LPITEMIDLIST pidl = ILCombine(pidlRoot, apidl[i]);
            SHChangeNotify(wEventId, SHCNF_IDLIST, pidl, NULL);
            SHFree(pidl);
        }

        wszCurrentPath += wcslen(wszCurrentPath) + 1;
    }
    HeapFree(GetProcessHeap(), 0, wszPathsList);
    return ret;
}

/****************************************************************************
 * CFSFolder::CopyItems
 *
 * copies items to this folder
 */
HRESULT WINAPI CFSFolder::CopyItems(IShellFolder * pSFFrom, UINT cidl,
                                    LPCITEMIDLIST * apidl)
{
    IPersistFolder2 *ppf2 = NULL;
    WCHAR szSrcPath[MAX_PATH];
    WCHAR szTargetPath[MAX_PATH];
    SHFILEOPSTRUCTW op;
    LPITEMIDLIST pidl;
    LPWSTR pszSrc, pszTarget, pszSrcList, pszTargetList, pszFileName;
    int res, length;
    HRESULT hr;
    STRRET strRet;

    TRACE ("(%p)->(%p,%u,%p)\n", this, pSFFrom, cidl, apidl);

    hr = pSFFrom->QueryInterface (IID_IPersistFolder2, (LPVOID *) & ppf2);
    if (SUCCEEDED(hr))
    {
        hr = ppf2->GetCurFolder(&pidl);
        if (FAILED(hr))
        {
            ppf2->Release();
            return hr;
        }
        ppf2->Release();

        hr = pSFFrom->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strRet);
        if (FAILED(hr))
        {
            SHFree(pidl);
            return hr;
        }

        hr = StrRetToBufW(&strRet, pidl, szSrcPath, MAX_PATH);
        if (FAILED(hr))
        {
            SHFree(pidl);
            return hr;
        }
        SHFree(pidl);

        pszSrc = PathAddBackslashW(szSrcPath);

        wcscpy(szTargetPath, sPathTarget);
        pszTarget = PathAddBackslashW(szTargetPath);

        pszSrcList = BuildPathsList(szSrcPath, cidl, apidl);
        pszTargetList = BuildPathsList(szTargetPath, cidl, apidl);

        if (!pszSrcList || !pszTargetList)
        {
            if (pszSrcList)
                HeapFree(GetProcessHeap(), 0, pszSrcList);

            if (pszTargetList)
                HeapFree(GetProcessHeap(), 0, pszTargetList);

            SHFree(pidl);
            ppf2->Release();
            return E_OUTOFMEMORY;
        }

        ZeroMemory(&op, sizeof(op));
        if (!pszSrcList[0])
        {
            /* remove trailing backslash */
            pszSrc--;
            pszSrc[0] = L'\0';
            op.pFrom = szSrcPath;
        }
        else
        {
            op.pFrom = pszSrcList;
        }

        if (!pszTargetList[0])
        {
            /* remove trailing backslash */
            if (pszTarget - szTargetPath > 3)
            {
                pszTarget--;
                pszTarget[0] = L'\0';
            }
            else
            {
                pszTarget[1] = L'\0';
            }

            op.pTo = szTargetPath;
        }
        else
        {
            op.pTo = pszTargetList;
        }
        op.hwnd = GetActiveWindow();
        op.wFunc = FO_COPY;
        op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR;

        res = SHFileOperationW(&op);

        if (res == DE_SAMEFILE)
        {
            length = wcslen(szTargetPath);

            pszFileName = wcsrchr(pszSrcList, '\\');
            pszFileName++;

            if (LoadStringW(shell32_hInstance, IDS_COPY_OF, pszTarget, MAX_PATH - length))
            {
                wcscat(szTargetPath, L" ");
            }

            wcscat(szTargetPath, pszFileName);
            op.pTo = szTargetPath;

            res = SHFileOperationW(&op);
        }

        HeapFree(GetProcessHeap(), 0, pszSrcList);
        HeapFree(GetProcessHeap(), 0, pszTargetList);

        if (res)
            return E_FAIL;
        else
            return S_OK;
    }
    return E_FAIL;
}

/************************************************************************
 * CFSFolder::GetClassID
 */
HRESULT WINAPI CFSFolder::GetClassID(CLSID * lpClassId)
{
    TRACE ("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = *pclsid;

    return S_OK;
}

/************************************************************************
 * CFSFolder::Initialize
 *
 * NOTES
 *  sPathTarget is not set. Don't know how to handle in a non rooted environment.
 */
HRESULT WINAPI CFSFolder::Initialize(LPCITEMIDLIST pidl)
{
    WCHAR wszTemp[MAX_PATH];

    TRACE ("(%p)->(%p)\n", this, pidl);

    SHFree (pidlRoot);     /* free the old pidl */
    pidlRoot = ILClone (pidl); /* set my pidl */

    SHFree (sPathTarget);
    sPathTarget = NULL;

    /* set my path */
    if (SHGetPathFromIDListW (pidl, wszTemp))
    {
        int len = wcslen(wszTemp);
        sPathTarget = (WCHAR *)SHAlloc((len + 1) * sizeof(WCHAR));
        if (!sPathTarget)
            return E_OUTOFMEMORY;
        memcpy(sPathTarget, wszTemp, (len + 1) * sizeof(WCHAR));
    }

    TRACE ("--(%p)->(%s)\n", this, debugstr_w(sPathTarget));
    return S_OK;
}

/**************************************************************************
 * CFSFolder::GetCurFolder
 */
HRESULT WINAPI CFSFolder::GetCurFolder(LPITEMIDLIST * pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    if (!pidl)
        return E_POINTER;

    *pidl = ILClone(pidlRoot);
    return S_OK;
}

/**************************************************************************
 * CFSFolder::InitializeEx
 *
 * FIXME: error handling
 */
HRESULT WINAPI CFSFolder::InitializeEx(IBindCtx * pbc, LPCITEMIDLIST pidlRootx,
                                       const PERSIST_FOLDER_TARGET_INFO * ppfti)
{
    WCHAR wszTemp[MAX_PATH];

    TRACE("(%p)->(%p,%p,%p)\n", this, pbc, pidlRootx, ppfti);
    if (ppfti)
        TRACE("--%p %s %s 0x%08x 0x%08x\n",
              ppfti->pidlTargetFolder, debugstr_w (ppfti->szTargetParsingName),
              debugstr_w (ppfti->szNetworkProvider), ppfti->dwAttributes,
              ppfti->csidl);

    pdump (pidlRootx);
    if (ppfti && ppfti->pidlTargetFolder)
        pdump(ppfti->pidlTargetFolder);

    if (pidlRoot)
        __SHFreeAndNil(&pidlRoot);    /* free the old */
    if (sPathTarget)
        __SHFreeAndNil(&sPathTarget);

    /*
     * Root path and pidl
     */
    pidlRoot = ILClone(pidlRootx);

    /*
     *  the target folder is spezified in csidl OR pidlTargetFolder OR
     *  szTargetParsingName
     */
    if (ppfti)
    {
        if (ppfti->csidl != -1)
        {
            if (SHGetSpecialFolderPathW(0, wszTemp, ppfti->csidl,
                                        ppfti->csidl & CSIDL_FLAG_CREATE)) {
                int len = wcslen(wszTemp);
                sPathTarget = (WCHAR *)SHAlloc((len + 1) * sizeof(WCHAR));
                if (!sPathTarget)
                    return E_OUTOFMEMORY;
                memcpy(sPathTarget, wszTemp, (len + 1) * sizeof(WCHAR));
            }
        }
        else if (ppfti->szTargetParsingName[0])
        {
            int len = wcslen(ppfti->szTargetParsingName);
            sPathTarget = (WCHAR *)SHAlloc((len + 1) * sizeof(WCHAR));
            if (!sPathTarget)
                return E_OUTOFMEMORY;
            memcpy(sPathTarget, ppfti->szTargetParsingName,
                   (len + 1) * sizeof(WCHAR));
        }
        else if (ppfti->pidlTargetFolder)
        {
            if (SHGetPathFromIDListW(ppfti->pidlTargetFolder, wszTemp))
            {
                int len = wcslen(wszTemp);
                sPathTarget = (WCHAR *)SHAlloc((len + 1) * sizeof(WCHAR));
                if (!sPathTarget)
                    return E_OUTOFMEMORY;
                memcpy(sPathTarget, wszTemp, (len + 1) * sizeof(WCHAR));
            }
        }
    }

    TRACE("--(%p)->(target=%s)\n", this, debugstr_w(sPathTarget));
    pdump(pidlRoot);
    return (sPathTarget) ? S_OK : E_FAIL;
}

HRESULT WINAPI CFSFolder::GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO * ppfti)
{
    FIXME("(%p)->(%p)\n", this, ppfti);
    ZeroMemory(ppfti, sizeof (*ppfti));
    return E_NOTIMPL;
}

/****************************************************************************
 * ISFDropTarget implementation
 */
BOOL CFSFolder::QueryDrop(DWORD dwKeyState, LPDWORD pdwEffect)
{
    DWORD dwEffect = *pdwEffect;

    *pdwEffect = DROPEFFECT_NONE;

    if (fAcceptFmt) { /* Does our interpretation of the keystate ... */
        *pdwEffect = KeyStateToDropEffect (dwKeyState);

        /* ... matches the desired effect ? */
        if (dwEffect & *pdwEffect) {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT WINAPI CFSFolder::DragEnter(IDataObject *pDataObject,
                                    DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    FORMATETC fmt;

    TRACE("(%p)->(DataObject=%p)\n", this, pDataObject);

    InitFormatEtc (fmt, cfShellIDList, TYMED_HGLOBAL);

    fAcceptFmt = (S_OK == pDataObject->QueryGetData(&fmt)) ?
                 TRUE : FALSE;

    QueryDrop(dwKeyState, pdwEffect);

    return S_OK;
}

HRESULT WINAPI CFSFolder::DragOver(DWORD dwKeyState, POINTL pt,
                                   DWORD *pdwEffect)
{
    TRACE("(%p)\n", this);

    if (!pdwEffect)
        return E_INVALIDARG;

    QueryDrop(dwKeyState, pdwEffect);

    return S_OK;
}

HRESULT WINAPI CFSFolder::DragLeave()
{
    TRACE("(%p)\n", this);

    fAcceptFmt = FALSE;

    return S_OK;
}

HRESULT WINAPI CFSFolder::Drop(IDataObject *pDataObject,
                               DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    FIXME("(%p) object dropped\n", this);

    return E_NOTIMPL;
}
