
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
    public CEnumIDListBase
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
    SF_RegisterClipFmt();
    fAcceptFmt = FALSE;
    m_bGroupPolicyActive = 0;
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
    IFileSystemBindData *fsbd = NULL;
    LPITEMIDLIST pidl = NULL;
    IUnknown *param = NULL;
    WIN32_FIND_DATAW wfd;
    HRESULT r;

    TRACE("%p %s\n", pbc, debugstr_w(path));

    if (!pbc)
        return NULL;

    /* see if the caller bound File System Bind Data */
    r = pbc->GetObjectParam((LPOLESTR)STR_FILE_SYS_BIND_DATA, &param);
    if (FAILED(r))
        return NULL;

    r = param->QueryInterface(IID_PPV_ARG(IFileSystemBindData,&fsbd));
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
        DWORD *pchEaten, PIDLIST_RELATIVE *ppidl,
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

    if (*lpszDisplayName)
    {
        /* get the next element */
        szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);

        pidlTemp = SHELL32_CreatePidlFromBindCtx(pbc, szElement);
        if (pidlTemp != NULL)
        {
            hr = S_OK;
        }
        else
        {
            /* build the full pathname to the element */
            lstrcpynW(szPath, sPathTarget, MAX_PATH - 1);
            PathAddBackslashW(szPath);
            len = wcslen(szPath);
            lstrcpynW(szPath + len, szElement, MAX_PATH - len);

            /* get the pidl */
            hr = _ILCreateFromPathW(szPath, &pidlTemp);
        }

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
                    hr = SHELL32_GetFSItemAttributes(this, pidlTemp, pdwAttributes);
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
    return ShellObjectCreatorInit<CFileSysEnum>(sPathTarget, dwFlags, IID_IEnumIDList, ppEnumIDList);
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
    PCUIDLIST_RELATIVE pidl,
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
    PCUIDLIST_RELATIVE pidl,
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
                                     PCUIDLIST_RELATIVE pidl1,
                                     PCUIDLIST_RELATIVE pidl2)
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
    CComPtr<IShellView> pShellView;
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
        PCUITEMID_CHILD_ARRAY apidl, DWORD * rgfInOut)
{
    HRESULT hr = S_OK;

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if(cidl == 0)
    {
        LPCITEMIDLIST rpidl = ILFindLastID(pidlRoot);

        if (_ILIsFolder(rpidl) || _ILIsValue(rpidl))
        {
            SHELL32_GetFSItemAttributes(this, rpidl, rgfInOut);
        }
        else if (_ILIsDrive(rpidl))
        {
            IShellFolder *psfParent = NULL;
            hr = SHBindToParent(pidlRoot, IID_PPV_ARG(IShellFolder, &psfParent), NULL);
            if(SUCCEEDED(hr))
            {
                hr = psfParent->GetAttributesOf(1, &rpidl, (SFGAOF*)rgfInOut);
                psfParent->Release();
            }
        }
        else
        {
            ERR("Got and unknown pidl!\n");
        }
    }
    else
    {
        while (cidl > 0 && *apidl)
        {
            pdump(*apidl);
            if(_ILIsFolder(*apidl) || _ILIsValue(*apidl))
                SHELL32_GetFSItemAttributes(this, *apidl, rgfInOut);
            else
                ERR("Got an unknown type of pidl!!!\n");
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
                                        UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
                                        REFIID riid, UINT * prgfInOut,
                                        LPVOID * ppvOut)
{
    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
           this, hwndOwner, cidl, apidl, shdebugstr_guid (&riid), prgfInOut, ppvOut);

    if (ppvOut)
    {
        *ppvOut = NULL;

        if (IsEqualIID(riid, IID_IContextMenu) && (cidl >= 1))
        {
            IContextMenu  * pCm = NULL;
            hr = CDefFolderMenu_Create2(pidlRoot, hwndOwner, cidl, apidl, static_cast<IShellFolder*>(this), NULL, 0, NULL, &pCm);
            pObj = pCm;
        }
        else if (IsEqualIID (riid, IID_IDataObject))
        {
            if (cidl >= 1) 
            {
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
            pObj = IExtractIconA_Constructor (pidl);
            SHFree (pidl);
            hr = S_OK;
        }
        else if (IsEqualIID (riid, IID_IExtractIconW) && (cidl == 1))
        {
            pidl = ILCombine (pidlRoot, apidl[0]);
            pObj = IExtractIconW_Constructor (pidl);
            SHFree (pidl);
            hr = S_OK;
        }
        else if (IsEqualIID (riid, IID_IDropTarget))
        {
            /* only interested in attempting to bind to shell folders, not files (except exe), so if we fail, rebind to root */
            if (cidl != 1 || FAILED(hr = this->_GetDropTarget(apidl[0], (LPVOID*) &pObj)))
            {
                IDropTarget * pDt = NULL;
                hr = this->QueryInterface(IID_PPV_ARG(IDropTarget, &pDt));
                pObj = pDt;
            }
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

HRESULT WINAPI CFSFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl,
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
    PCUITEMID_CHILD pidl,
    LPCOLESTR lpName,
    DWORD dwFlags,
    PITEMID_CHILD *pPidlOut)
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

HRESULT WINAPI CFSFolder::GetDetailsEx(PCUITEMID_CHILD pidl,
                                       const SHCOLUMNID * pscid, VARIANT * pv)
{
    FIXME ("(%p)\n", this);

    return E_NOTIMPL;
}

HRESULT WINAPI CFSFolder::GetDetailsOf(PCUITEMID_CHILD pidl,
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
    CComPtr<IEnumIDList> penum;
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
                                    LPCITEMIDLIST * apidl, BOOL bCopy)
{
    CComPtr<IPersistFolder2> ppf2 = NULL;
    WCHAR szSrcPath[MAX_PATH];
    WCHAR szTargetPath[MAX_PATH];
    SHFILEOPSTRUCTW op;
    LPITEMIDLIST pidl;
    LPWSTR pszSrc, pszTarget, pszSrcList, pszTargetList, pszFileName;
    int res, length;
    HRESULT hr;
    STRRET strRet;

    TRACE ("(%p)->(%p,%u,%p)\n", this, pSFFrom, cidl, apidl);

    hr = pSFFrom->QueryInterface (IID_PPV_ARG(IPersistFolder2, &ppf2));
    if (SUCCEEDED(hr))
    {
        hr = ppf2->GetCurFolder(&pidl);
        if (FAILED(hr))
        {
            return hr;
        }

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
            op.fFlags = 0;
        }
        else
        {
            op.pTo = pszTargetList;
            op.fFlags = FOF_MULTIDESTFILES;
        }
        op.hwnd = GetActiveWindow();
        op.wFunc = bCopy ? FO_COPY : FO_MOVE;
        op.fFlags |= FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR;

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

BOOL
CFSFolder::GetUniqueFileName(LPWSTR pwszBasePath, LPCWSTR pwszExt, LPWSTR pwszTarget, BOOL bShortcut)
{
    WCHAR wszLink[40];

    if (!bShortcut)
    {
        if (!LoadStringW(shell32_hInstance, IDS_LNK_FILE, wszLink, _countof(wszLink)))
            wszLink[0] = L'\0';
    }

    if (!bShortcut)
        swprintf(pwszTarget, L"%s%s%s", wszLink, pwszBasePath, pwszExt);
    else
        swprintf(pwszTarget, L"%s%s", pwszBasePath, pwszExt);

    for (UINT i = 2; PathFileExistsW(pwszTarget); ++i)
    {
        if (!bShortcut)
            swprintf(pwszTarget, L"%s%s (%u)%s", wszLink, pwszBasePath, i, pwszExt);
        else
            swprintf(pwszTarget, L"%s (%u)%s", pwszBasePath, i, pwszExt);
    }

    return TRUE;
}

/****************************************************************************
 * IDropTarget implementation
 */
BOOL CFSFolder::QueryDrop(DWORD dwKeyState, LPDWORD pdwEffect)
{
    /* TODO Windows does different drop effects if dragging across drives. 
    i.e., it will copy instead of move if the directories are on different disks. */

    DWORD dwEffect = DROPEFFECT_MOVE;

    *pdwEffect = DROPEFFECT_NONE;

    if (fAcceptFmt) { /* Does our interpretation of the keystate ... */
        *pdwEffect = KeyStateToDropEffect (dwKeyState);

        if (*pdwEffect == DROPEFFECT_NONE)
            *pdwEffect = dwEffect;

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
    TRACE("(%p)->(DataObject=%p)\n", this, pDataObject);
    FORMATETC fmt;
    FORMATETC fmt2;
    fAcceptFmt = FALSE;

    InitFormatEtc (fmt, cfShellIDList, TYMED_HGLOBAL);
    InitFormatEtc (fmt2, CF_HDROP, TYMED_HGLOBAL);

    if (SUCCEEDED(pDataObject->QueryGetData(&fmt)))
        fAcceptFmt = TRUE;
    else if (SUCCEEDED(pDataObject->QueryGetData(&fmt2)))
        fAcceptFmt = TRUE;

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
    TRACE("(%p) object dropped, effect %u\n", this, *pdwEffect);
    
    BOOL fIsOpAsync = FALSE;
    CComPtr<IAsyncOperation> pAsyncOperation;

    if (SUCCEEDED(pDataObject->QueryInterface(IID_PPV_ARG(IAsyncOperation, &pAsyncOperation))))
    {
        if (SUCCEEDED(pAsyncOperation->GetAsyncMode(&fIsOpAsync)) && fIsOpAsync)
        {
            _DoDropData *data = static_cast<_DoDropData*>(HeapAlloc(GetProcessHeap(), 0, sizeof(_DoDropData)));
            data->This = this;
            // Need to maintain this class in case the window is closed or the class exists temporarily (when dropping onto a folder).
            pDataObject->AddRef();
            pAsyncOperation->StartOperation(NULL);
            CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDataObject, &data->pStream);
            this->AddRef();
            data->dwKeyState = dwKeyState;
            data->pt = pt;
            // Need to dereference as pdweffect gets freed.
            data->pdwEffect = *pdwEffect;
            SHCreateThread(CFSFolder::_DoDropThreadProc, data, NULL, NULL);
            return S_OK;
        }
    }
    return this->_DoDrop(pDataObject, dwKeyState, pt, pdwEffect);
}

HRESULT WINAPI CFSFolder::_DoDrop(IDataObject *pDataObject,
                               DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
    TRACE("(%p) performing drop, effect %u\n", this, *pdwEffect);
    FORMATETC fmt;
    FORMATETC fmt2;
    STGMEDIUM medium;

    InitFormatEtc (fmt, cfShellIDList, TYMED_HGLOBAL);
    InitFormatEtc (fmt2, CF_HDROP, TYMED_HGLOBAL);

    HRESULT hr;
    bool bCopy = TRUE;
    bool bLinking = FALSE;

    /* Figure out what drop operation we're doing */
    if (pdwEffect)
    {
        TRACE("Current drop effect flag %i\n", *pdwEffect);
        if ((*pdwEffect & DROPEFFECT_MOVE) == DROPEFFECT_MOVE)
            bCopy = FALSE;
        if ((*pdwEffect & DROPEFFECT_LINK) == DROPEFFECT_LINK)
            bLinking = TRUE;
    }

    if (SUCCEEDED(pDataObject->QueryGetData(&fmt)))
    {
        hr = pDataObject->GetData(&fmt, &medium);
        TRACE("CFSTR_SHELLIDLIST.\n");

        /* lock the handle */
        LPIDA lpcida = (LPIDA)GlobalLock(medium.hGlobal);
        if (!lpcida)
        {
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }

        /* convert the data into pidl */
        LPITEMIDLIST pidl;
        LPITEMIDLIST *apidl = _ILCopyCidaToaPidl(&pidl, lpcida);
        if (!apidl)
        {
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }

        CComPtr<IShellFolder> psfDesktop;
        CComPtr<IShellFolder> psfFrom = NULL;
        CComPtr<IShellFolder> psfTarget = NULL;

        hr = this->QueryInterface(IID_PPV_ARG(IShellFolder, &psfTarget));
        if (FAILED(hr))
        {
            ERR("psfTarget setting failed\n");
            SHFree(pidl);
            _ILFreeaPidl(apidl, lpcida->cidl);
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }

        /* Grab the desktop shell folder */
        hr = SHGetDesktopFolder(&psfDesktop);
        if (FAILED(hr))
        {
            ERR("SHGetDesktopFolder failed\n");
            SHFree(pidl);
            _ILFreeaPidl(apidl, lpcida->cidl);
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }

        /* Find source folder, this is where the clipboard data was copied from */
        if (_ILIsDesktop(pidl))
        {
            /* use desktop shell folder */
            psfFrom = psfDesktop;
        }
        else 
        {
            hr = psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfFrom));
            if (FAILED(hr))
            {
                ERR("no IShellFolder\n");
                SHFree(pidl);
                _ILFreeaPidl(apidl, lpcida->cidl);
                ReleaseStgMedium(&medium);
                return E_FAIL;
            }
        }

        if (bLinking)
        {
            CComPtr<IPersistFolder2> ppf2 = NULL;
            STRRET strFile;
            WCHAR wszTargetPath[MAX_PATH];
            LPITEMIDLIST targetpidl;
            WCHAR wszPath[MAX_PATH];
            WCHAR wszTarget[MAX_PATH];

            hr = this->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
            if (SUCCEEDED(hr))
            {
                hr = ppf2->GetCurFolder(&targetpidl);
                if (SUCCEEDED(hr))
                {
                    hr = psfDesktop->GetDisplayNameOf(targetpidl, SHGDN_FORPARSING, &strFile);
                    ILFree(targetpidl);
                    if (SUCCEEDED(hr)) 
                    {
                        hr = StrRetToBufW(&strFile, NULL, wszTargetPath, _countof(wszTargetPath));
                    }
                }
            }

            if (FAILED(hr)) 
            {
                ERR("Error obtaining target path");
            }

            TRACE("target path = %s", debugstr_w(wszTargetPath));

            /* We need to create a link for each pidl in the copied items, so step through the pidls from the clipboard */
            for (UINT i = 0; i < lpcida->cidl; i++)
            {
                //Find out which file we're copying
                STRRET strFile;
                hr = psfFrom->GetDisplayNameOf(apidl[i], SHGDN_FORPARSING, &strFile);
                if (FAILED(hr)) 
                {
                    ERR("Error source obtaining path");
                    break;
                }

                hr = StrRetToBufW(&strFile, apidl[i], wszPath, _countof(wszPath));
                if (FAILED(hr)) 
                {
                    ERR("Error putting source path into buffer");
                    break;
                }
                TRACE("source path = %s", debugstr_w(wszPath));

                // Creating a buffer to hold the combined path
                WCHAR buffer_1[MAX_PATH] = L"";
                WCHAR *lpStr1;
                lpStr1 = buffer_1;

                LPWSTR pwszFileName = PathFindFileNameW(wszPath);
                LPWSTR pwszExt = PathFindExtensionW(wszPath);
                LPWSTR placementPath = PathCombineW(lpStr1, wszTargetPath, pwszFileName);
                CComPtr<IPersistFile> ppf;

                //Check to see if it's already a link. 
                if (!wcsicmp(pwszExt, L".lnk"))
                {
                    //It's a link so, we create a new one which copies the old.
                    if(!GetUniqueFileName(placementPath, pwszExt, wszTarget, TRUE)) 
                    {
                        ERR("Error getting unique file name");
                        hr = E_FAIL;
                        break;
                    }
                    hr = IShellLink_ConstructFromFile(NULL, IID_IPersistFile, ILCombine(pidl, apidl[i]), (LPVOID*)&ppf);
                    if (FAILED(hr)) {
                        ERR("Error constructing link from file");
                        break;
                    }

                    hr = ppf->Save(wszTarget, FALSE);
					if (FAILED(hr))
						break;
					SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, wszTarget, NULL);
                }
                else
                {
                    //It's not a link, so build a new link using the creator class and fill it in.
                    //Create a file name for the link
                    if (!GetUniqueFileName(placementPath, L".lnk", wszTarget, TRUE))
                    {
                        ERR("Error creating unique file name");
                        hr = E_FAIL;
                        break;
                    }

                    CComPtr<IShellLinkW> pLink;
                    hr = CShellLink::_CreatorClass::CreateInstance(NULL, IID_PPV_ARG(IShellLinkW, &pLink));
                    if (FAILED(hr)) {
                        ERR("Error instantiating IShellLinkW");
                        break;
                    }

                    WCHAR szDirPath[MAX_PATH], *pwszFile;
                    GetFullPathName(wszPath, MAX_PATH, szDirPath, &pwszFile);
                    if (pwszFile) pwszFile[0] = 0;

                    hr = pLink->SetPath(wszPath);
                    if(FAILED(hr))
                        break;

                    hr = pLink->SetWorkingDirectory(szDirPath);
                    if(FAILED(hr))
                        break;

                    hr = pLink->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
                    if(FAILED(hr))
                        break;

                    hr = ppf->Save(wszTarget, TRUE);
					if (FAILED(hr))
						break;
					SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, wszTarget, NULL);
                }
            }
        }
        else 
        {
            hr = this->CopyItems(psfFrom, lpcida->cidl, (LPCITEMIDLIST*)apidl, bCopy);
        }

        SHFree(pidl);
        _ILFreeaPidl(apidl, lpcida->cidl);
        ReleaseStgMedium(&medium);
    }
    else if (SUCCEEDED(pDataObject->QueryGetData(&fmt2)))
    {
        FORMATETC fmt2;
        InitFormatEtc (fmt2, CF_HDROP, TYMED_HGLOBAL);
        if (SUCCEEDED(pDataObject->GetData(&fmt2, &medium)) /* && SUCCEEDED(pDataObject->GetData(&fmt2, &medium))*/)
        {
            CComPtr<IPersistFolder2> ppf2 = NULL;
            STRRET strFile;
            WCHAR wszTargetPath[MAX_PATH + 1];
            LPWSTR pszSrcList;
            LPITEMIDLIST targetpidl;
            CComPtr<IShellFolder> psfDesktop = NULL;
            hr = SHGetDesktopFolder(&psfDesktop);
            if (FAILED(hr))
            {
                ERR("SHGetDesktopFolder failed\n");
                return E_FAIL;
            }

            hr = this->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
            if (SUCCEEDED(hr))
            {
                hr = ppf2->GetCurFolder(&targetpidl);
                if (SUCCEEDED(hr))
                {
                    hr = psfDesktop->GetDisplayNameOf(targetpidl, SHGDN_FORPARSING, &strFile);
                    ILFree(targetpidl);
                    if (SUCCEEDED(hr)) 
                    {
                        hr = StrRetToBufW(&strFile, NULL, wszTargetPath, _countof(wszTargetPath));
                        //Double NULL terminate.
                        wszTargetPath[wcslen(wszTargetPath) + 1] = '\0';
                    }
                }
            }
            if (FAILED(hr)) 
            {
                ERR("Error obtaining target path");
                return E_FAIL;
            }

            LPDROPFILES lpdf = (LPDROPFILES) GlobalLock(medium.hGlobal);
            if (!lpdf)
            {
                ERR("Error locking global\n");
                return E_FAIL;
            }
            pszSrcList = (LPWSTR) (((byte*) lpdf) + lpdf->pFiles);
            TRACE("Source file (just the first) = %s\n", debugstr_w(pszSrcList));
            TRACE("Target path = %s\n", debugstr_w(wszTargetPath));

            SHFILEOPSTRUCTW op;
            ZeroMemory(&op, sizeof(op));
            op.pFrom = pszSrcList;
            op.pTo = wszTargetPath;
            op.hwnd = GetActiveWindow();
            op.wFunc = bCopy ? FO_COPY : FO_MOVE;
            op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR;
            hr = SHFileOperationW(&op);
            return hr;
        }
        ERR("Error calling GetData\n");
        hr = E_FAIL;
    }
    else 
    {
        ERR("No viable drop format.\n");
        hr = E_FAIL;
    }    
    return hr;
}

DWORD WINAPI CFSFolder::_DoDropThreadProc(LPVOID lpParameter) {
    CoInitialize(NULL);
    _DoDropData *data = static_cast<_DoDropData*>(lpParameter);
    CComPtr<IDataObject> pDataObject;
    HRESULT hr = CoGetInterfaceAndReleaseStream (data->pStream, IID_PPV_ARG(IDataObject, &pDataObject));

    if (SUCCEEDED(hr))
    {
        CComPtr<IAsyncOperation> pAsyncOperation;
        hr = data->This->_DoDrop(pDataObject, data->dwKeyState, data->pt, &data->pdwEffect);
        if (SUCCEEDED(pDataObject->QueryInterface(IID_PPV_ARG(IAsyncOperation, &pAsyncOperation))))
        {
            pAsyncOperation->EndOperation(hr, NULL, data->pdwEffect);
        }
    }
    //Release the CFSFolder and data object holds in the copying thread.
    data->This->Release();
    //Release the parameter from the heap.
    HeapFree(GetProcessHeap(), 0, data);
    CoUninitialize();
    return 0;
}

HRESULT WINAPI CFSFolder::_GetDropTarget(LPCITEMIDLIST pidl, LPVOID *ppvOut) {
    HKEY hKey;
    HRESULT hr;

    TRACE("CFSFolder::_GetDropTarget entered\n");

    if (_ILGetGUIDPointer (pidl) || _ILIsFolder (pidl))
        return this->BindToObject(pidl, NULL, IID_IDropTarget, ppvOut);

    STRRET strFile;
    hr = this->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strFile);
    if (hr == S_OK)
    {
        WCHAR wszPath[MAX_PATH];
        hr = StrRetToBufW(&strFile, pidl, wszPath, _countof(wszPath));

        if (hr == S_OK)
        {
            LPCWSTR pwszExt = PathFindExtensionW(wszPath);
            if (pwszExt[0])
            {
                /* enumerate dynamic/static for a given file class */
                if (RegOpenKeyExW(HKEY_CLASSES_ROOT, pwszExt, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                {
                    /* load dynamic extensions from file extension key, for example .jpg */
                    _LoadDynamicDropTargetHandlerForKey(hKey, wszPath, ppvOut);
                    RegCloseKey(hKey);
                }

                WCHAR wszTemp[40];
                DWORD dwSize = sizeof(wszTemp);
                if (RegGetValueW(HKEY_CLASSES_ROOT, pwszExt, NULL, RRF_RT_REG_SZ, NULL, wszTemp, &dwSize) == ERROR_SUCCESS)
                {
                    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszTemp, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                    {
                        /* load dynamic extensions from progid key, for example jpegfile */
                        _LoadDynamicDropTargetHandlerForKey(hKey, wszPath, ppvOut);
                        RegCloseKey(hKey);
                    }
                }
            }
        }
    }
    else
        ERR("GetDisplayNameOf failed: %x\n", hr);

    return hr;
}

HRESULT WINAPI CFSFolder::_LoadDynamicDropTargetHandlerForKey(HKEY hRootKey, LPCWSTR pwcsname, LPVOID *ppvOut) 
{
    TRACE("CFSFolder::_LoadDynamicDropTargetHandlerForKey entered\n");

    WCHAR wszName[MAX_PATH], *pwszClsid;
    DWORD dwSize = sizeof(wszName);
    HRESULT hr;

    if (RegGetValueW(hRootKey, L"shellex\\DropHandler", NULL, RRF_RT_REG_SZ, NULL, wszName, &dwSize) == ERROR_SUCCESS)
    {
        CLSID clsid;
        hr = CLSIDFromString(wszName, &clsid);
        if (hr == S_OK)
            pwszClsid = wszName;

        if (m_bGroupPolicyActive)
        {
            if (RegGetValueW(HKEY_LOCAL_MACHINE,
                             L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved",
                             pwszClsid,
                             RRF_RT_REG_SZ,
                             NULL,
                             NULL,
                             NULL) == ERROR_SUCCESS)
            {
                hr = _LoadDynamicDropTargetHandler(&clsid, pwcsname, ppvOut);
            }
        }
        else
        {
            hr = _LoadDynamicDropTargetHandler(&clsid, pwcsname, ppvOut);
        }
    }
    else
        return E_FAIL;
    return hr;
}

HRESULT WINAPI CFSFolder::_LoadDynamicDropTargetHandler(const CLSID *pclsid, LPCWSTR pwcsname, LPVOID *ppvOut)
{
    TRACE("CFSFolder::_LoadDynamicDropTargetHandler entered\n");
    HRESULT hr;

    CComPtr<IPersistFile> pp;
    hr = SHCoCreateInstance(NULL, pclsid, NULL, IID_PPV_ARG(IPersistFile, &pp));
    if (hr != S_OK)
    {
        ERR("SHCoCreateInstance failed %x\n", GetLastError());
    }
    pp->Load(pwcsname, 0);

    hr = pp->QueryInterface(IID_PPV_ARG(IDropTarget, (IDropTarget**) ppvOut));
    if (hr != S_OK)
    {
        ERR("Failed to query for interface IID_IShellExtInit hr %x pclsid %s\n", hr, wine_dbgstr_guid(pclsid));
        return hr;
    }
    return hr;
}