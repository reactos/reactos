/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     file system folder
 * COPYRIGHT:   Copyright 1997 Marcus Meissner
 *              Copyright 1998, 1999, 2002 Juergen Schmied
 *              Copyright 2019 Katayama Hirofumi MZ
 *              Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

static HRESULT SHELL32_GetCLSIDForDirectory(LPCWSTR pwszDir, LPCWSTR KeyName, CLSID* pclsidFolder);


HKEY OpenKeyFromFileType(LPWSTR pExtension, LPCWSTR KeyName)
{
    HKEY hkey;

    WCHAR FullName[MAX_PATH];
    DWORD dwSize = sizeof(FullName);
    wsprintf(FullName, L"%s\\%s", pExtension, KeyName);

    LONG res = RegOpenKeyExW(HKEY_CLASSES_ROOT, FullName, 0, KEY_READ, &hkey);
    if (!res)
        return hkey;

    res = RegGetValueW(HKEY_CLASSES_ROOT, pExtension, NULL, RRF_RT_REG_SZ, NULL, FullName, &dwSize);
    if (res)
    {
        WARN("Failed to get progid for extension %S (%x), error %d\n", pExtension, pExtension, res);
        return NULL;
    }

    wcscat(FullName, L"\\");
    wcscat(FullName, KeyName);

    hkey = NULL;
    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, FullName, 0, KEY_READ, &hkey);
    if (res)
        WARN("Could not open key %S for extension %S\n", KeyName, pExtension);

    return hkey;
}

LPWSTR ExtensionFromPidl(PCUIDLIST_RELATIVE pidl)
{
    if (!_ILIsValue(pidl))
    {
        ERR("Invalid pidl!\n");
        return NULL;
    }

    FileStructW* pDataW = _ILGetFileStructW(pidl);
    if (!pDataW)
    {
        ERR("Invalid pidl!\n");
        return NULL;
    }

    LPWSTR pExtension = PathFindExtensionW(pDataW->wszName);
    if (!pExtension || *pExtension == UNICODE_NULL)
    {
        WARN("No extension for %S!\n", pDataW->wszName);
        return NULL;
    }
    return pExtension;
}

HRESULT GetCLSIDForFileTypeFromExtension(LPWSTR pExtension, LPCWSTR KeyName, CLSID* pclsid)
{
    HKEY hkeyProgId = OpenKeyFromFileType(pExtension, KeyName);
    if (!hkeyProgId)
    {
        WARN("OpenKeyFromFileType failed for key %S\n", KeyName);
        return S_FALSE;
    }

    WCHAR wszCLSIDValue[CHARS_IN_GUID];
    DWORD dwSize = sizeof(wszCLSIDValue);
    LONG res = RegGetValueW(hkeyProgId, NULL, NULL, RRF_RT_REG_SZ, NULL, wszCLSIDValue, &dwSize);
    RegCloseKey(hkeyProgId);
    if (res)
    {
        ERR("OpenKeyFromFileType succeeded but RegGetValueW failed\n");
        return S_FALSE;
    }

#if 0
    {
        res = RegGetValueW(HKEY_LOCAL_MACHINE,
                           L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved",
                           wszCLSIDValue,
                           RRF_RT_REG_SZ,
                           NULL,
                           NULL,
                           NULL);
        if (res != ERROR_SUCCESS)
        {
            ERR("DropHandler extension %S not approved\n", wszName);
            return E_ACCESSDENIED;
        }
    }
#endif

    if (RegGetValueW(HKEY_LOCAL_MACHINE,
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Blocked",
                     wszCLSIDValue,
                     RRF_RT_REG_SZ,
                     NULL,
                     NULL,
                     NULL) == ERROR_SUCCESS)
    {
        ERR("Extension %S  not approved\n", wszCLSIDValue);
        return E_ACCESSDENIED;
    }

    HRESULT hres = CLSIDFromString (wszCLSIDValue, pclsid);
    if (FAILED_UNEXPECTEDLY(hres))
        return hres;

    return S_OK;
}

HRESULT GetCLSIDForFileType(PCUIDLIST_RELATIVE pidl, LPCWSTR KeyName, CLSID* pclsid)
{
    LPWSTR pExtension = ExtensionFromPidl(pidl);
    if (!pExtension)
        return S_FALSE;

    return GetCLSIDForFileTypeFromExtension(pExtension, KeyName, pclsid);
}

static HRESULT
getDefaultIconLocation(LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT uFlags)
{
    if (!HCR_GetIconW(L"Folder", szIconFile, NULL, cchMax, piIndex))
    {
        lstrcpynW(szIconFile, swShell32Name, cchMax);
        *piIndex = -IDI_SHELL_FOLDER;
    }

    if (uFlags & GIL_OPENICON)
    {
        // next icon
        if (*piIndex < 0)
            (*piIndex)--;
        else
            (*piIndex)++;
    }

    return S_OK;
}

static BOOL
getShellClassInfo(LPCWSTR Entry, LPWSTR pszValue, DWORD cchValueLen, LPCWSTR IniFile)
{
    return GetPrivateProfileStringW(L".ShellClassInfo", Entry, NULL, pszValue, cchValueLen, IniFile);
}

static HRESULT
getIconLocationForFolder(IShellFolder * psf, PCITEMID_CHILD pidl, UINT uFlags,
                         LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    DWORD dwFileAttrs;
    WCHAR wszPath[MAX_PATH];
    WCHAR wszIniFullPath[MAX_PATH];

    if (uFlags & GIL_DEFAULTICON)
        goto Quit;

    // get path
    if (!ILGetDisplayNameExW(psf, pidl, wszPath, 0))
        goto Quit;
    if (!PathIsDirectoryW(wszPath))
        goto Quit;

    // read-only or system folder?
    dwFileAttrs = _ILGetFileAttributes(ILFindLastID(pidl), NULL, 0);
    if ((dwFileAttrs & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)) == 0)
        goto Quit;

    // build the full path of ini file
    StringCchCopyW(wszIniFullPath, _countof(wszIniFullPath), wszPath);
    PathAppendW(wszIniFullPath, L"desktop.ini");

    WCHAR wszValue[MAX_PATH], wszTemp[MAX_PATH];
    if (getShellClassInfo(L"IconFile", wszValue, _countof(wszValue), wszIniFullPath))
    {
        // wszValue --> wszTemp
        ExpandEnvironmentStringsW(wszValue, wszTemp, _countof(wszTemp));

        // wszPath + wszTemp --> wszPath
        if (PathIsRelativeW(wszTemp))
            PathAppendW(wszPath, wszTemp);
        else
            StringCchCopyW(wszPath, _countof(wszPath), wszTemp);

        // wszPath --> szIconFile
        GetFullPathNameW(wszPath, cchMax, szIconFile, NULL);

        *piIndex = GetPrivateProfileIntW(L".ShellClassInfo", L"IconIndex", 0, wszIniFullPath);
        return S_OK;
    }
    else if (getShellClassInfo(L"CLSID", wszValue, _countof(wszValue), wszIniFullPath) &&
             HCR_GetIconW(wszValue, szIconFile, NULL, cchMax, piIndex))
    {
        return S_OK;
    }
    else if (getShellClassInfo(L"CLSID2", wszValue, _countof(wszValue), wszIniFullPath) &&
             HCR_GetIconW(wszValue, szIconFile, NULL, cchMax, piIndex))
    {
        return S_OK;
    }
    else if (getShellClassInfo(L"IconResource", wszValue, _countof(wszValue), wszIniFullPath))
    {
        // wszValue --> wszTemp
        ExpandEnvironmentStringsW(wszValue, wszTemp, _countof(wszTemp));

        // parse the icon location
        *piIndex = PathParseIconLocationW(wszTemp);

        // wszPath + wszTemp --> wszPath
        if (PathIsRelativeW(wszTemp))
            PathAppendW(wszPath, wszTemp);
        else
            StringCchCopyW(wszPath, _countof(wszPath), wszTemp);

        // wszPath --> szIconFile
        GetFullPathNameW(wszPath, cchMax, szIconFile, NULL);
        return S_OK;
    }

Quit:
    return getDefaultIconLocation(szIconFile, cchMax, piIndex, uFlags);
}

HRESULT CFSExtractIcon_CreateInstance(IShellFolder * psf, LPCITEMIDLIST pidl, REFIID iid, LPVOID * ppvOut)
{
    CComPtr<IDefaultExtractIconInit> initIcon;
    HRESULT hr;
    int icon_idx = 0;
    UINT flags = 0; // FIXME: Use it!
    WCHAR wTemp[MAX_PATH] = L"";

    hr = SHCreateDefaultExtractIcon(IID_PPV_ARG(IDefaultExtractIconInit,&initIcon));
    if (FAILED(hr))
        return hr;

    if (_ILIsFolder (pidl))
    {
        if (SUCCEEDED(getIconLocationForFolder(psf,
                          pidl, 0, wTemp, _countof(wTemp),
                          &icon_idx,
                          &flags)))
        {
            initIcon->SetNormalIcon(wTemp, icon_idx);
            // FIXME: if/when getIconLocationForFolder does something for
            //        GIL_FORSHORTCUT, code below should be uncommented. and
            //        the following line removed.
            initIcon->SetShortcutIcon(wTemp, icon_idx);
        }
        if (SUCCEEDED(getIconLocationForFolder(psf,
                          pidl, GIL_DEFAULTICON, wTemp, _countof(wTemp),
                          &icon_idx,
                          &flags)))
        {
            initIcon->SetDefaultIcon(wTemp, icon_idx);
        }
        // if (SUCCEEDED(getIconLocationForFolder(psf,
        //                   pidl, GIL_FORSHORTCUT, wTemp, _countof(wTemp),
        //                   &icon_idx,
        //                   &flags)))
        // {
        //     initIcon->SetShortcutIcon(wTemp, icon_idx);
        // }
        if (SUCCEEDED(getIconLocationForFolder(psf,
                          pidl, GIL_OPENICON, wTemp, _countof(wTemp),
                          &icon_idx,
                          &flags)))
        {
            initIcon->SetOpenIcon(wTemp, icon_idx);
        }
    }
    else
    {
        LPWSTR pExtension = ExtensionFromPidl(pidl);
        HKEY hkey = pExtension ? OpenKeyFromFileType(pExtension, L"DefaultIcon") : NULL;
        if (!hkey)
            WARN("Could not open DefaultIcon key!\n");

        DWORD dwSize = sizeof(wTemp);
        if (hkey && !SHQueryValueExW(hkey, NULL, NULL, NULL, wTemp, &dwSize))
        {
            WCHAR sNum[5];
            if (ParseFieldW (wTemp, 2, sNum, 5))
                icon_idx = _wtoi(sNum);
            else
                icon_idx = 0; /* sometimes the icon number is missing */
            ParseFieldW (wTemp, 1, wTemp, MAX_PATH);
            PathUnquoteSpacesW(wTemp);

            if (!wcscmp(L"%1", wTemp)) /* icon is in the file */
            {
                ILGetDisplayNameExW(psf, pidl, wTemp, ILGDN_FORPARSING);
                icon_idx = 0;

                INT ret = ExtractIconExW(wTemp, -1, NULL, NULL, 0);
                if (ret <= 0)
                {
                    StringCbCopyW(wTemp, sizeof(wTemp), swShell32Name);
                    if (lstrcmpiW(pExtension, L".exe") == 0 || lstrcmpiW(pExtension, L".scr") == 0)
                        icon_idx = -IDI_SHELL_EXE;
                    else
                        icon_idx = -IDI_SHELL_DOCUMENT;
                }
            }

            initIcon->SetNormalIcon(wTemp, icon_idx);
        }
        else
        {
            initIcon->SetNormalIcon(swShell32Name, 0);
        }

        if (hkey)
            RegCloseKey(hkey);
    }

    return initIcon->QueryInterface(iid, ppvOut);
}

/*
CFileSysEnum should do an initial FindFirstFile and do a FindNextFile as each file is
returned by Next. When the enumerator is created, it can do numerous additional operations
including formatting a drive, reconnecting a network share drive, and requesting a disk
be inserted in a removable drive.
*/


class CFileSysEnum :
    public CEnumIDListBase
{
private:
    HRESULT _AddFindResult(LPWSTR sParentDir, const WIN32_FIND_DATAW& FindData, DWORD dwFlags)
    {
#define SUPER_HIDDEN (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)

        // Does it need special handling because it is hidden?
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        {
            DWORD dwHidden = FindData.dwFileAttributes & SUPER_HIDDEN;

            // Is it hidden, but are we not asked to include hidden?
            if (dwHidden == FILE_ATTRIBUTE_HIDDEN && !(dwFlags & SHCONTF_INCLUDEHIDDEN))
                return S_OK;

            // Is it a system file, but are we not asked to include those?
            if (dwHidden == SUPER_HIDDEN && !(dwFlags & SHCONTF_INCLUDESUPERHIDDEN))
                return S_OK;
        }

        BOOL bDirectory = (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        HRESULT hr;
        if (bDirectory)
        {
            // Skip the current and parent directory nodes
            if (!strcmpW(FindData.cFileName, L".") || !strcmpW(FindData.cFileName, L".."))
                return S_OK;

            // Does this directory need special handling?
            if ((FindData.dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)) != 0)
            {
                WCHAR Tmp[MAX_PATH];
                CLSID clsidFolder;

                PathCombineW(Tmp, sParentDir, FindData.cFileName);

                hr = SHELL32_GetCLSIDForDirectory(Tmp, L"CLSID", &clsidFolder);
                if (SUCCEEDED(hr))
                {
                    ERR("Got CLSID override for '%S'\n", Tmp);
                }
            }
        }
        else
        {
            CLSID clsidFile;
            LPWSTR pExtension = PathFindExtensionW(FindData.cFileName);
            if (pExtension)
            {
                // FIXME: Cache this?
                hr = GetCLSIDForFileTypeFromExtension(pExtension, L"CLSID", &clsidFile);
                if (hr == S_OK)
                {
                    HKEY hkey;
                    hr = SHRegGetCLSIDKeyW(clsidFile, L"ShellFolder", FALSE, FALSE, &hkey);
                    if (SUCCEEDED(hr))
                    {
                        ::RegCloseKey(hkey);

                        // This should be presented as directory!
                        bDirectory = TRUE;
                        TRACE("Treating '%S' as directory!\n", FindData.cFileName);
                    }
                }
            }
        }

        LPITEMIDLIST pidl = NULL;
        if (bDirectory)
        {
            if (dwFlags & SHCONTF_FOLDERS)
            {
                TRACE("(%p)->  (folder=%s)\n", this, debugstr_w(FindData.cFileName));
                pidl = _ILCreateFromFindDataW(&FindData);
            }
        }
        else
        {
            if (dwFlags & SHCONTF_NONFOLDERS)
            {
                TRACE("(%p)->  (file  =%s)\n", this, debugstr_w(FindData.cFileName));
                pidl = _ILCreateFromFindDataW(&FindData);
            }
        }

        if (pidl && !AddToEnumList(pidl))
        {
            FAILED_UNEXPECTEDLY(E_FAIL);
            return E_FAIL;
        }

        return S_OK;
    }

public:
    CFileSysEnum()
    {

    }

    ~CFileSysEnum()
    {
    }

    HRESULT WINAPI Initialize(LPWSTR sPathTarget, DWORD dwFlags)
    {
        TRACE("(%p)->(path=%s flags=0x%08x)\n", this, debugstr_w(sPathTarget), dwFlags);

        if (!sPathTarget || !sPathTarget[0])
        {
            WARN("No path for CFileSysEnum, empty result!\n");
            return S_FALSE;
        }

        WCHAR szFindPattern[MAX_PATH];
        HRESULT hr = StringCchCopyW(szFindPattern, _countof(szFindPattern), sPathTarget);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        /* FIXME: UNSAFE CRAP */
        PathAddBackslashW(szFindPattern);

        hr = StringCchCatW(szFindPattern, _countof(szFindPattern), L"*.*");
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;


        WIN32_FIND_DATAW FindData;
        HANDLE hFind = FindFirstFileW(szFindPattern, &FindData);
        if (hFind == INVALID_HANDLE_VALUE)
            return HRESULT_FROM_WIN32(GetLastError());

        do
        {
            hr = _AddFindResult(sPathTarget, FindData, dwFlags);

            if (FAILED_UNEXPECTEDLY(hr))
                break;

        } while(FindNextFileW(hFind, &FindData));

        if (SUCCEEDED(hr))
        {
            DWORD dwError = GetLastError();
            if (dwError != ERROR_NO_MORE_FILES)
            {
                hr = HRESULT_FROM_WIN32(dwError);
                FAILED_UNEXPECTEDLY(hr);
            }
        }
        TRACE("(%p)->(hr=0x%08x)\n", this, hr);
        FindClose(hFind);
        return hr;
    }

    BEGIN_COM_MAP(CFileSysEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
    END_COM_MAP()
};


/***********************************************************************
 *   IShellFolder implementation
 */

CFSFolder::CFSFolder()
{
    m_pclsid = &CLSID_ShellFSFolder;
    m_sPathTarget = NULL;
    m_pidlRoot = NULL;
    m_bGroupPolicyActive = 0;
}

CFSFolder::~CFSFolder()
{
    TRACE("-- destroying IShellFolder(%p)\n", this);

    SHFree(m_pidlRoot);
    SHFree(m_sPathTarget);
}


static const shvheader GenericSFHeader[] = {
    {IDS_SHV_COLUMN_NAME, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 15},
    {IDS_SHV_COLUMN_COMMENTS, SHCOLSTATE_TYPE_STR, LVCFMT_LEFT, 0},
    {IDS_SHV_COLUMN_TYPE, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 10},
    {IDS_SHV_COLUMN_SIZE, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN_MODIFIED, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 12},
    {IDS_SHV_COLUMN_ATTRIBUTES, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 10}
};

#define GENERICSHELLVIEWCOLUMNS 6

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

static HRESULT SHELL32_GetCLSIDForDirectory(LPCWSTR pwszDir, LPCWSTR KeyName, CLSID* pclsidFolder)
{
    WCHAR wszCLSIDValue[CHARS_IN_GUID];
    WCHAR wszDesktopIni[MAX_PATH];

    StringCchCopyW(wszDesktopIni, MAX_PATH, pwszDir);
    StringCchCatW(wszDesktopIni, MAX_PATH, L"\\desktop.ini");

    if (GetPrivateProfileStringW(L".ShellClassInfo",
                                 KeyName,
                                 L"",
                                 wszCLSIDValue,
                                 CHARS_IN_GUID,
                                 wszDesktopIni))
    {
        return CLSIDFromString(wszCLSIDValue, pclsidFolder);
    }
    return E_FAIL;
}

HRESULT SHELL32_GetFSItemAttributes(IShellFolder * psf, LPCITEMIDLIST pidl, LPDWORD pdwAttributes)
{
    DWORD dwFileAttributes, dwShellAttributes;

    if (!_ILIsFolder(pidl) && !_ILIsValue(pidl))
    {
        ERR("Got wrong type of pidl!\n");
        *pdwAttributes &= SFGAO_CANLINK;
        return S_OK;
    }

    dwFileAttributes = _ILGetFileAttributes(pidl, NULL, 0);

    /* Set common attributes */
    dwShellAttributes = SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANLINK | SFGAO_CANRENAME | SFGAO_CANDELETE |
                        SFGAO_HASPROPSHEET | SFGAO_DROPTARGET | SFGAO_FILESYSTEM;

    BOOL bDirectory = (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

    if (!bDirectory)
    {
        // https://git.reactos.org/?p=reactos.git;a=blob;f=dll/shellext/zipfldr/res/zipfldr.rgs;hb=032b5aacd233cd7b83ab6282aad638c161fdc400#l9
        WCHAR szFileName[MAX_PATH];
        LPWSTR pExtension;

        if (_ILSimpleGetTextW(pidl, szFileName, _countof(szFileName)) && (pExtension = PathFindExtensionW(szFileName)))
        {
            CLSID clsidFile;
            // FIXME: Cache this?
            HRESULT hr = GetCLSIDForFileTypeFromExtension(pExtension, L"CLSID", &clsidFile);
            if (hr == S_OK)
            {
                HKEY hkey;
                hr = SHRegGetCLSIDKeyW(clsidFile, L"ShellFolder", FALSE, FALSE, &hkey);
                if (SUCCEEDED(hr))
                {
                    DWORD dwAttributes = 0;
                    DWORD dwSize = sizeof(dwAttributes);
                    LSTATUS Status;

                    Status = SHRegGetValueW(hkey, NULL, L"Attributes", RRF_RT_REG_DWORD, NULL, &dwAttributes, &dwSize);
                    if (Status == STATUS_SUCCESS)
                    {
                        TRACE("Augmenting '%S' with dwAttributes=0x%x\n", szFileName, dwAttributes);
                        dwShellAttributes |= dwAttributes;
                    }
                    ::RegCloseKey(hkey);

                    // This should be presented as directory!
                    bDirectory = TRUE;
                    TRACE("Treating '%S' as directory!\n", szFileName);
                }
            }
        }
    }

    // This is a directory
    if (bDirectory)
    {
        dwShellAttributes |= (SFGAO_FOLDER | /*SFGAO_HASSUBFOLDER |*/ SFGAO_STORAGE);

        // Is this a real directory?
        if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            dwShellAttributes |= (SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR);
        }
    }
    else
    {
        dwShellAttributes |= SFGAO_STREAM;
    }

    if (dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        dwShellAttributes |=  SFGAO_HIDDEN;

    if (dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        dwShellAttributes |=  SFGAO_READONLY;

    if (SFGAO_LINK & *pdwAttributes)
    {
        char ext[MAX_PATH];

        if (_ILGetExtension(pidl, ext, MAX_PATH) && !lstrcmpiA(ext, "lnk"))
            dwShellAttributes |= SFGAO_LINK;
    }

    if (SFGAO_HASSUBFOLDER & *pdwAttributes)
    {
        CComPtr<IShellFolder> psf2;
        if (SUCCEEDED(psf->BindToObject(pidl, 0, IID_PPV_ARG(IShellFolder, &psf2))))
        {
            CComPtr<IEnumIDList> pEnumIL;
            if (SUCCEEDED(psf2->EnumObjects(0, SHCONTF_FOLDERS, &pEnumIL)))
            {
                if (pEnumIL->Skip(1) == S_OK)
                    dwShellAttributes |= SFGAO_HASSUBFOLDER;
            }
        }
    }

    *pdwAttributes = dwShellAttributes;

    TRACE ("-- 0x%08x\n", *pdwAttributes);
    return S_OK;
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
            /* We are creating an id list without ensuring that the items exist.
               If we have a remaining path, this must be a folder.
               We have to do it now because it is set as a file by default */
            if (szNext)
            {
                pidlTemp->mkid.abID[0] = PT_FOLDER;
            }
            hr = S_OK;
        }
        else
        {
            /* build the full pathname to the element */
            lstrcpynW(szPath, m_sPathTarget, MAX_PATH - 1);
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
    return ShellObjectCreatorInit<CFileSysEnum>(m_sPathTarget, dwFlags, IID_PPV_ARG(IEnumIDList, ppEnumIDList));
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

    CComPtr<IShellFolder> pSF;
    HRESULT hr;

    if (!m_pidlRoot || !ppvOut || !pidl || !pidl->mkid.cb)
    {
        ERR("CFSFolder::BindToObject: Invalid parameters\n");
        return E_INVALIDARG;
    }

    /* Get the pidl data */
    FileStruct* pData = &_ILGetDataPointer(pidl)->u.file;
    FileStructW* pDataW = _ILGetFileStructW(pidl);

    if (!pDataW)
    {
        ERR("CFSFolder::BindToObject: Invalid pidl!\n");
        return E_INVALIDARG;
    }

    *ppvOut = NULL;

    /* Create the target folder info */
    PERSIST_FOLDER_TARGET_INFO pfti = {0};
    pfti.dwAttributes = -1;
    pfti.csidl = -1;
    PathCombineW(pfti.szTargetParsingName, m_sPathTarget, pDataW->wszName);

    /* Get the CLSID to bind to */
    CLSID clsidFolder;
    if (_ILIsFolder(pidl))
    {
        if ((pData->uFileAttribs & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)) != 0)
        {
            hr = SHELL32_GetCLSIDForDirectory(pfti.szTargetParsingName, L"CLSID", &clsidFolder);

            if (SUCCEEDED(hr))
            {
                /* We got a GUID from a desktop.ini, let's try it */
                hr = SHELL32_BindToSF(m_pidlRoot, &pfti, pidl, &clsidFolder, riid, ppvOut);
                if (SUCCEEDED(hr))
                {
                    TRACE("-- returning (%p) %08x, (%s)\n", *ppvOut, hr, wine_dbgstr_guid(&clsidFolder));
                    return hr;
                }

                /* Something went wrong, re-try it with a normal ShellFSFolder */
                ERR("CFSFolder::BindToObject: %s failed to bind, using fallback (0x%08x)\n", wine_dbgstr_guid(&clsidFolder), hr);
            }
        }
        /* No system folder or the custom class failed */
        clsidFolder = CLSID_ShellFSFolder;
    }
    else
    {
        hr = GetCLSIDForFileType(pidl, L"CLSID", &clsidFolder);
        if (hr == S_FALSE)
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        if (hr != S_OK)
            return hr;
    }

    hr = SHELL32_BindToSF(m_pidlRoot, &pfti, pidl, &clsidFolder, riid, ppvOut);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    TRACE ("-- returning (%p) %08x\n", *ppvOut, hr);

    return S_OK;

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
    LPPIDLDATA pData1 = _ILGetDataPointer(pidl1);
    LPPIDLDATA pData2 = _ILGetDataPointer(pidl2);
    FileStructW* pDataW1 = _ILGetFileStructW(pidl1);
    FileStructW* pDataW2 = _ILGetFileStructW(pidl2);
    BOOL bIsFolder1 = _ILIsFolder(pidl1);
    BOOL bIsFolder2 = _ILIsFolder(pidl2);
    LPWSTR pExtension1, pExtension2;

    if (!pDataW1 || !pDataW2 || LOWORD(lParam) >= GENERICSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    /* When sorting between a File and a Folder, the Folder gets sorted first */
    if (bIsFolder1 != bIsFolder2)
    {
        return MAKE_COMPARE_HRESULT(bIsFolder1 ? -1 : 1);
    }

    int result;
    switch (LOWORD(lParam))
    {
        case 0: /* Name */
            result = wcsicmp(pDataW1->wszName, pDataW2->wszName);
            break;
        case 1: /* Comments */
            result = 0;
            break;
        case 2: /* Type */
            pExtension1 = PathFindExtensionW(pDataW1->wszName);
            pExtension2 = PathFindExtensionW(pDataW2->wszName);
            result = wcsicmp(pExtension1, pExtension2);
            break;
        case 3: /* Size */
            if (pData1->u.file.dwFileSize > pData2->u.file.dwFileSize)
                result = 1;
            else if (pData1->u.file.dwFileSize < pData2->u.file.dwFileSize)
                result = -1;
            else
                result = 0;
            break;
        case 4: /* Modified */
            result = pData1->u.file.uFileDate - pData2->u.file.uFileDate;
            if (result == 0)
                result = pData1->u.file.uFileTime - pData2->u.file.uFileTime;
            break;
        case 5: /* Attributes */
            return SHELL32_CompareDetails(this, lParam, pidl1, pidl2);
    }

    if (result == 0)
        return SHELL32_CompareChildren(this, lParam, pidl1, pidl2);

    return MAKE_COMPARE_HRESULT(result);
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

        BOOL bIsDropTarget = IsEqualIID (riid, IID_IDropTarget);
        BOOL bIsShellView = !bIsDropTarget && IsEqualIID (riid, IID_IShellView);

        if (bIsDropTarget || bIsShellView)
        {
            DWORD dwDirAttributes = _ILGetFileAttributes(ILFindLastID(m_pidlRoot), NULL, 0);

            if ((dwDirAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)) != 0)
            {
                CLSID clsidFolder;
                hr = SHELL32_GetCLSIDForDirectory(m_sPathTarget, L"UICLSID", &clsidFolder);
                if (SUCCEEDED(hr))
                {
                    CComPtr<IPersistFolder> spFolder;
                    hr = SHCoCreateInstance(NULL, &clsidFolder, NULL, IID_PPV_ARG(IPersistFolder, &spFolder));
                    if (!FAILED_UNEXPECTEDLY(hr))
                    {
                        hr = spFolder->Initialize(m_pidlRoot);

                        if (!FAILED_UNEXPECTEDLY(hr))
                        {
                            hr = spFolder->QueryInterface(riid, ppvOut);
                        }
                    }
                }
                else
                {
                    // No desktop.ini, or no UICLSID present, continue as if nothing happened
                    hr = E_INVALIDARG;
                }
            }
        }

        if (!SUCCEEDED(hr))
        {
            // No UICLSID handler found, continue to the default handlers
            if (bIsDropTarget)
            {
                hr = CFSDropTarget_CreateInstance(m_sPathTarget, riid, ppvOut);
            }
            else if (IsEqualIID (riid, IID_IContextMenu))
            {
                HKEY hKeys[16];
                UINT cKeys = 0;
                AddClassKeyToArray(L"Directory\\Background", hKeys, &cKeys);

                DEFCONTEXTMENU dcm;
                dcm.hwnd = hwndOwner;
                dcm.pcmcb = this;
                dcm.pidlFolder = m_pidlRoot;
                dcm.psf = this;
                dcm.cidl = 0;
                dcm.apidl = NULL;
                dcm.cKeys = cKeys;
                dcm.aKeys = hKeys;
                dcm.punkAssociationInfo = NULL;
                hr = SHCreateDefaultContextMenu (&dcm, riid, ppvOut);
            }
            else if (bIsShellView)
            {
                SFV_CREATE sfvparams = {sizeof(SFV_CREATE), this, NULL, this};
                hr = SHCreateShellFolderView(&sfvparams, (IShellView**)ppvOut);
            }
            else
            {
                hr = E_INVALIDARG;
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
        LPCITEMIDLIST rpidl = ILFindLastID(m_pidlRoot);

        if (_ILIsFolder(rpidl) || _ILIsValue(rpidl))
        {
            SHELL32_GetFSItemAttributes(this, rpidl, rgfInOut);
        }
        else if (_ILIsDrive(rpidl))
        {
            IShellFolder *psfParent = NULL;
            hr = SHBindToParent(m_pidlRoot, IID_PPV_ARG(IShellFolder, &psfParent), NULL);
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
    LPVOID pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
           this, hwndOwner, cidl, apidl, shdebugstr_guid (&riid), prgfInOut, ppvOut);

    if (ppvOut)
    {
        *ppvOut = NULL;

        if (cidl == 1 && _ILIsValue(apidl[0]))
        {
            hr = _CreateExtensionUIObject(apidl[0], riid, ppvOut);
            if(hr != S_FALSE)
                return hr;
        }

        if (IsEqualIID(riid, IID_IContextMenu) && (cidl >= 1))
        {
            HKEY hKeys[16];
            UINT cKeys = 0;
            AddFSClassKeysToArray(apidl[0], hKeys, &cKeys);

            DEFCONTEXTMENU dcm;
            dcm.hwnd = hwndOwner;
            dcm.pcmcb = this;
            dcm.pidlFolder = m_pidlRoot;
            dcm.psf = this;
            dcm.cidl = cidl;
            dcm.apidl = apidl;
            dcm.cKeys = cKeys;
            dcm.aKeys = hKeys;
            dcm.punkAssociationInfo = NULL;
            hr = SHCreateDefaultContextMenu (&dcm, riid, &pObj);
        }
        else if (IsEqualIID (riid, IID_IDataObject))
        {
            if (cidl >= 1)
            {
                hr = IDataObject_Constructor (hwndOwner, m_pidlRoot, apidl, cidl, TRUE, (IDataObject **)&pObj);
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        else if ((IsEqualIID (riid, IID_IExtractIconA) || IsEqualIID (riid, IID_IExtractIconW)) && (cidl == 1))
        {
            if (_ILIsValue(apidl[0]))
                hr = _GetIconHandler(apidl[0], riid, (LPVOID*)&pObj);
            if (hr != S_OK)
                hr = CFSExtractIcon_CreateInstance(this, apidl[0], riid, &pObj);
        }
        else if (IsEqualIID (riid, IID_IDropTarget))
        {
            /* only interested in attempting to bind to shell folders, not files (except exe), so if we fail, rebind to root */
            if (cidl != 1 || FAILED(hr = this->_GetDropTarget(apidl[0], (LPVOID*) &pObj)))
            {
                hr = CFSDropTarget_CreateInstance(m_sPathTarget, riid, (LPVOID*) &pObj);
            }
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
BOOL SHELL_FS_HideExtension(LPCWSTR szPath)
{
    HKEY hKey;
    DWORD dwData, dwDataSize = sizeof(DWORD);
    BOOL doHide = FALSE; /* The default value is FALSE (win98 at least) */
    LONG lError;

    lError = RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                             0, NULL, 0, KEY_ALL_ACCESS, NULL,
                             &hKey, NULL);
    if (lError == ERROR_SUCCESS)
    {
        lError = RegQueryValueExW(hKey, L"HideFileExt", NULL, NULL, (LPBYTE)&dwData, &dwDataSize);
        if (lError == ERROR_SUCCESS)
            doHide = dwData;
        RegCloseKey(hKey);
    }

    if (!doHide)
    {
        LPCWSTR DotExt = PathFindExtensionW(szPath);
        if (*DotExt != 0)
        {
            WCHAR classname[MAX_PATH];
            LONG classlen = sizeof(classname);
            lError = RegQueryValueW(HKEY_CLASSES_ROOT, DotExt, classname, &classlen);
            if (lError == ERROR_SUCCESS)
            {
                lError = RegOpenKeyW(HKEY_CLASSES_ROOT, classname, &hKey);
                if (lError == ERROR_SUCCESS)
                {
                    lError = RegQueryValueExW(hKey, L"NeverShowExt", NULL, NULL, NULL, NULL);
                    if (lError == ERROR_SUCCESS)
                        doHide = TRUE;

                    RegCloseKey(hKey);
                }
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
    if (!strRet)
        return E_INVALIDARG;

    /* If it is a complex pidl, let the child handle it */
    if (!_ILIsPidlSimple (pidl)) /* complex pidl */
    {
        return SHELL32_GetDisplayNameOfChild(this, pidl, dwFlags, strRet);
    }
    else if (pidl && !pidl->mkid.cb) /* empty pidl */
    {
        /* If it is an empty pidl return only the path of the folder */
        if ((GET_SHGDN_FOR(dwFlags) & SHGDN_FORPARSING) &&
            (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER) &&
            m_sPathTarget)
        {
            return SHSetStrRet(strRet, m_sPathTarget);
        }
        return E_INVALIDARG;
    }

    int len = 0;
    LPWSTR pszPath = (LPWSTR)CoTaskMemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    if ((GET_SHGDN_FOR(dwFlags) & SHGDN_FORPARSING) &&
        (GET_SHGDN_RELATION(dwFlags) != SHGDN_INFOLDER) &&
        m_sPathTarget)
    {
        lstrcpynW(pszPath, m_sPathTarget, MAX_PATH);
        PathAddBackslashW(pszPath);
        len = wcslen(pszPath);
    }
    _ILSimpleGetTextW(pidl, pszPath + len, MAX_PATH + 1 - len);
    if (!_ILIsFolder(pidl)) SHELL_FS_ProcessDisplayFilename(pszPath, dwFlags);

    strRet->uType = STRRET_WSTR;
    strRet->pOleStr = pszPath;

    TRACE ("-- (%p)->(%s)\n", this, strRet->uType == STRRET_CSTR ? strRet->cStr : debugstr_w(strRet->pOleStr));
    return S_OK;
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
    BOOL bIsFolder = _ILIsFolder (ILFindLastID (pidl));

    TRACE ("(%p)->(%p,pidl=%p,%s,%u,%p)\n", this, hwndOwner, pidl,
           debugstr_w (lpName), dwFlags, pPidlOut);

    FileStructW* pDataW = _ILGetFileStructW(pidl);
    if (!pDataW)
    {
        ERR("Got garbage pidl\n");
        return E_INVALIDARG;
    }

    /* build source path */
    PathCombineW(szSrc, m_sPathTarget, pDataW->wszName);

    /* build destination path */
    if (dwFlags == SHGDN_NORMAL || dwFlags & SHGDN_INFOLDER)
        PathCombineW(szDest, m_sPathTarget, lpName);
    else
        lstrcpynW(szDest, lpName, MAX_PATH);

    if(!(dwFlags & SHGDN_FORPARSING) && SHELL_FS_HideExtension(szSrc)) {
        WCHAR *ext = PathFindExtensionW(szSrc);
        if(*ext != '\0') {
            INT len = wcslen(szDest);
            lstrcpynW(szDest + len, ext, MAX_PATH - len);
        }
    }

    TRACE ("src=%s dest=%s\n", debugstr_w(szSrc), debugstr_w(szDest));
    if (!wcscmp(szSrc, szDest))
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
        return SHSetStrRet(&psd->str, GenericSFHeader[iColumn].colnameid);
    }
    else
    {
        hr = S_OK;
        psd->str.uType = STRRET_CSTR;
        /* the data from the pidl */
        switch (iColumn)
        {
            case 0:                /* name */
                hr = GetDisplayNameOf (pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
                break;
            case 1:                /* FIXME: comments */
                psd->str.cStr[0] = 0;
                break;
            case 2:                /* type */
                _ILGetFileType(pidl, psd->str.cStr, MAX_PATH);
                break;
            case 3:                /* size */
                _ILGetFileSize(pidl, psd->str.cStr, MAX_PATH);
                break;
            case 4:                /* date */
                _ILGetFileDate(pidl, psd->str.cStr, MAX_PATH);
                break;
            case 5:                /* attributes */
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

/************************************************************************
 * CFSFolder::GetClassID
 */
HRESULT WINAPI CFSFolder::GetClassID(CLSID * lpClassId)
{
    TRACE ("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = *m_pclsid;

    return S_OK;
}

/************************************************************************
 * CFSFolder::Initialize
 *
 * NOTES
 *  m_sPathTarget is not set. Don't know how to handle in a non rooted environment.
 */
HRESULT WINAPI CFSFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    WCHAR wszTemp[MAX_PATH];

    TRACE ("(%p)->(%p)\n", this, pidl);

    SHFree(m_pidlRoot);     /* free the old pidl */
    m_pidlRoot = ILClone (pidl); /* set my pidl */

    SHFree (m_sPathTarget);
    m_sPathTarget = NULL;

    /* set my path */
    if (SHGetPathFromIDListW (pidl, wszTemp))
    {
        int len = wcslen(wszTemp);
        m_sPathTarget = (WCHAR *)SHAlloc((len + 1) * sizeof(WCHAR));
        if (!m_sPathTarget)
            return E_OUTOFMEMORY;
        memcpy(m_sPathTarget, wszTemp, (len + 1) * sizeof(WCHAR));
    }

    TRACE ("--(%p)->(%s)\n", this, debugstr_w(m_sPathTarget));
    return S_OK;
}

/**************************************************************************
 * CFSFolder::GetCurFolder
 */
HRESULT WINAPI CFSFolder::GetCurFolder(PIDLIST_ABSOLUTE * pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    if (!pidl)
        return E_POINTER;

    *pidl = ILClone(m_pidlRoot);
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

    if (m_pidlRoot)
        __SHFreeAndNil(&m_pidlRoot);    /* free the old */
    if (m_sPathTarget)
        __SHFreeAndNil(&m_sPathTarget);

    /*
     * Root path and pidl
     */
    m_pidlRoot = ILClone(pidlRootx);

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
                m_sPathTarget = (WCHAR *)SHAlloc((len + 1) * sizeof(WCHAR));
                if (!m_sPathTarget)
                    return E_OUTOFMEMORY;
                memcpy(m_sPathTarget, wszTemp, (len + 1) * sizeof(WCHAR));
            }
        }
        else if (ppfti->szTargetParsingName[0])
        {
            int len = wcslen(ppfti->szTargetParsingName);
            m_sPathTarget = (WCHAR *)SHAlloc((len + 1) * sizeof(WCHAR));
            if (!m_sPathTarget)
                return E_OUTOFMEMORY;
            memcpy(m_sPathTarget, ppfti->szTargetParsingName,
                   (len + 1) * sizeof(WCHAR));
        }
        else if (ppfti->pidlTargetFolder)
        {
            if (SHGetPathFromIDListW(ppfti->pidlTargetFolder, wszTemp))
            {
                int len = wcslen(wszTemp);
                m_sPathTarget = (WCHAR *)SHAlloc((len + 1) * sizeof(WCHAR));
                if (!m_sPathTarget)
                    return E_OUTOFMEMORY;
                memcpy(m_sPathTarget, wszTemp, (len + 1) * sizeof(WCHAR));
            }
        }
    }

    TRACE("--(%p)->(target=%s)\n", this, debugstr_w(m_sPathTarget));
    pdump(m_pidlRoot);
    return (m_sPathTarget) ? S_OK : E_FAIL;
}

HRESULT WINAPI CFSFolder::GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO * ppfti)
{
    FIXME("(%p)->(%p)\n", this, ppfti);
    ZeroMemory(ppfti, sizeof (*ppfti));
    return E_NOTIMPL;
}

HRESULT CFSFolder::_CreateExtensionUIObject(PCUIDLIST_RELATIVE pidl, REFIID riid, LPVOID *ppvOut)
{
    WCHAR buf[MAX_PATH];

    sprintfW(buf, L"ShellEx\\{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             riid.Data1, riid.Data2, riid.Data3,
             riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
             riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);

    CLSID clsid;
    HRESULT hr;

    hr = GetCLSIDForFileType(pidl, buf, &clsid);
    if (hr != S_OK)
        return hr;

    hr = _CreateShellExtInstance(&clsid, pidl, riid, ppvOut);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

HRESULT CFSFolder::_GetDropTarget(LPCITEMIDLIST pidl, LPVOID *ppvOut)
{
    HRESULT hr;

    TRACE("CFSFolder::_GetDropTarget entered\n");

    if (_ILIsFolder (pidl))
    {
        CComPtr<IShellFolder> psfChild;
        hr = this->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfChild));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return psfChild->CreateViewObject(NULL, IID_IDropTarget, ppvOut);
    }

    CLSID clsid;
    hr = GetCLSIDForFileType(pidl, L"shellex\\DropHandler", &clsid);
    if (hr != S_OK)
        return hr;

    hr = _CreateShellExtInstance(&clsid, pidl, IID_IDropTarget, ppvOut);
    if (FAILED_UNEXPECTEDLY(hr))
        return S_FALSE;

    return S_OK;
}

HRESULT CFSFolder::_GetIconHandler(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppvOut)
{
    CLSID clsid;
    HRESULT hr;

    hr = GetCLSIDForFileType(pidl, L"shellex\\IconHandler", &clsid);
    if (hr != S_OK)
        return hr;

    hr = _CreateShellExtInstance(&clsid, pidl, riid, ppvOut);
    if (FAILED_UNEXPECTEDLY(hr))
        return S_FALSE;

    return S_OK;
}

HRESULT CFSFolder::_CreateShellExtInstance(const CLSID *pclsid, LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr;
    WCHAR wszPath[MAX_PATH];

    FileStructW* pDataW = _ILGetFileStructW(pidl);
    if (!pDataW)
    {
        ERR("Got garbage pidl\n");
        return E_INVALIDARG;
    }

    PathCombineW(wszPath, m_sPathTarget, pDataW->wszName);

    CComPtr<IPersistFile> pp;
    hr = SHCoCreateInstance(NULL, pclsid, NULL, IID_PPV_ARG(IPersistFile, &pp));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    pp->Load(wszPath, 0);

    hr = pp->QueryInterface(riid, ppvOut);
    if (hr != S_OK)
    {
        ERR("Failed to query for interface IID_IShellExtInit hr %x pclsid %s\n", hr, wine_dbgstr_guid(pclsid));
        return hr;
    }
    return hr;
}

HRESULT WINAPI CFSFolder::CallBack(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg != DFM_MERGECONTEXTMENU && uMsg != DFM_INVOKECOMMAND)
        return S_OK;

    /* no data object means no selection */
    if (!pdtobj)
    {
        if (uMsg == DFM_INVOKECOMMAND && wParam == 0)
        {
            PUITEMID_CHILD pidlChild = ILClone(ILFindLastID(m_pidlRoot));
            LPITEMIDLIST pidlParent = ILClone(m_pidlRoot);
            ILRemoveLastID(pidlParent);
            HRESULT hr = SH_ShowPropertiesDialog(m_sPathTarget, pidlParent, &pidlChild);
            if (FAILED(hr))
                ERR("SH_ShowPropertiesDialog failed\n");
            ILFree(pidlChild);
            ILFree(pidlParent);
        }
        else if (uMsg == DFM_MERGECONTEXTMENU)
        {
            QCMINFO *pqcminfo = (QCMINFO *)lParam;
            HMENU hpopup = CreatePopupMenu();
            _InsertMenuItemW(hpopup, 0, TRUE, 0, MFT_STRING, MAKEINTRESOURCEW(IDS_PROPERTIES), MFS_ENABLED);
            Shell_MergeMenus(pqcminfo->hmenu, hpopup, pqcminfo->indexMenu++, pqcminfo->idCmdFirst, pqcminfo->idCmdLast, MM_ADDSEPARATOR);
            DestroyMenu(hpopup);
        }

        return S_OK;
    }

    if (uMsg != DFM_INVOKECOMMAND || wParam != DFM_CMD_PROPERTIES)
        return S_OK;

    return Shell_DefaultContextMenuCallBack(this, pdtobj);
}

static HBITMAP DoLoadPicture(LPCWSTR pszFileName)
{
    // create stream from file
    HRESULT hr;
    CComPtr<IStream> pStream;
    hr = SHCreateStreamOnFileEx(pszFileName, STGM_READ, FILE_ATTRIBUTE_NORMAL,
                                FALSE, NULL, &pStream);
    if (FAILED(hr))
        return NULL;

    // load the picture
    HBITMAP hbm = NULL;
    CComPtr<IPicture> pPicture;
    OleLoadPicture(pStream, 0, FALSE, IID_IPicture, (LPVOID *)&pPicture);

    // get the bitmap handle
    if (pPicture)
    {
        pPicture->get_Handle((OLE_HANDLE *)&hbm);

        // copy the bitmap handle
        hbm = (HBITMAP)CopyImage(hbm, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    }

    return hbm;
}

HRESULT WINAPI CFSFolder::GetCustomViewInfo(ULONG unknown, SFVM_CUSTOMVIEWINFO_DATA *data)
{
    if (data == NULL)
    {
        return E_POINTER;
    }
    if (data->cbSize != sizeof(*data))
    {
        // NOTE: You have to set the cbData member before SFVM_GET_CUSTOMVIEWINFO call.
        return E_INVALIDARG;
    }

    data->hbmBack = NULL;
    data->clrText = CLR_INVALID;
    data->clrTextBack = CLR_INVALID;

    WCHAR szPath[MAX_PATH], szIniFile[MAX_PATH];

    // does the folder exists?
    if (!SHGetPathFromIDListW(m_pidlRoot, szPath) || !PathIsDirectoryW(szPath))
    {
        return E_INVALIDARG;
    }

    // don't use custom view in network path for security
    if (PathIsNetworkPath(szPath))
    {
        return E_ACCESSDENIED;
    }

    // build the ini file path
    StringCchCopyW(szIniFile, _countof(szIniFile), szPath);
    PathAppend(szIniFile, L"desktop.ini");

    static LPCWSTR TheGUID = L"{BE098140-A513-11D0-A3A4-00C04FD706EC}";
    static LPCWSTR Space = L" \t\n\r\f\v";

    // get info from ini file
    WCHAR szImage[MAX_PATH], szText[64];

    // load the image
    szImage[0] = UNICODE_NULL;
    GetPrivateProfileStringW(TheGUID, L"IconArea_Image", L"", szImage, _countof(szImage), szIniFile);
    if (szImage[0])
    {
        StrTrimW(szImage, Space);
        if (PathIsRelativeW(szImage))
        {
            PathAppendW(szPath, szImage);
            StringCchCopyW(szImage, _countof(szImage), szPath);
        }
        data->hbmBack = DoLoadPicture(szImage);
    }

    // load the text color
    szText[0] = UNICODE_NULL;
    GetPrivateProfileStringW(TheGUID, L"IconArea_Text", L"", szText, _countof(szText), szIniFile);
    if (szText[0])
    {
        StrTrimW(szText, Space);

        LPWSTR pchEnd = NULL;
        COLORREF cr = (wcstol(szText, &pchEnd, 0) & 0xFFFFFF);

        if (pchEnd && !*pchEnd)
            data->clrText = cr;
    }

    // load the text background color
    szText[0] = UNICODE_NULL;
    GetPrivateProfileStringW(TheGUID, L"IconArea_TextBackground", L"", szText, _countof(szText), szIniFile);
    if (szText[0])
    {
        StrTrimW(szText, Space);

        LPWSTR pchEnd = NULL;
        COLORREF cr = (wcstol(szText, &pchEnd, 0) & 0xFFFFFF);

        if (pchEnd && !*pchEnd)
            data->clrTextBack = cr;
    }

    if (data->hbmBack != NULL || data->clrText != CLR_INVALID || data->clrTextBack != CLR_INVALID)
        return S_OK;

    return E_FAIL;
}

HRESULT WINAPI CFSFolder::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = E_NOTIMPL;
    switch (uMsg)
    {
    case SFVM_GET_CUSTOMVIEWINFO:
        hr = GetCustomViewInfo((ULONG)wParam, (SFVM_CUSTOMVIEWINFO_DATA *)lParam);
        break;
    }
    return hr;
}
