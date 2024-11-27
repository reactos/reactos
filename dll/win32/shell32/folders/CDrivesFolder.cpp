/*
 *    Virtual Workplace folder
 *
 *    Copyright 1997                Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
 *    Copyright 2009                Andrew Hill
 *    Copyright 2017-2024           Katayama Hirofumi MZ
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
#include <process.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/*
CDrivesFolder should create a CRegFolder to represent the virtual items that exist only in
the registry. The CRegFolder is aggregated by the CDrivesFolder.
The CDrivesFolderEnum class should enumerate only drives on the system. Since the CRegFolder
implementation of IShellFolder::EnumObjects enumerates the virtual items, the
CDrivesFolderEnum is only responsible for returning the physical items.

2. At least on my XP system, the drive pidls returned are of type PT_DRIVE1, not PT_DRIVE
3. The parsing name returned for my computer is incorrect. It should be "My Computer"
*/

static int iDriveIconIds[7] = { IDI_SHELL_DRIVE,       /* DRIVE_UNKNOWN */
                                IDI_SHELL_CDROM,       /* DRIVE_NO_ROOT_DIR*/
                                IDI_SHELL_3_14_FLOPPY, /* DRIVE_REMOVABLE*/
                                IDI_SHELL_DRIVE,       /* DRIVE_FIXED*/
                                IDI_SHELL_NETDRIVE,    /* DRIVE_REMOTE*/
                                IDI_SHELL_CDROM,       /* DRIVE_CDROM*/
                                IDI_SHELL_RAMDISK      /* DRIVE_RAMDISK*/
                                };

static int iDriveTypeIds[7] = { IDS_DRIVE_FIXED,       /* DRIVE_UNKNOWN */
                                IDS_DRIVE_FIXED,       /* DRIVE_NO_ROOT_DIR*/
                                IDS_DRIVE_FLOPPY,      /* DRIVE_REMOVABLE*/
                                IDS_DRIVE_FIXED,       /* DRIVE_FIXED*/
                                IDS_DRIVE_NETWORK,     /* DRIVE_REMOTE*/
                                IDS_DRIVE_CDROM,       /* DRIVE_CDROM*/
                                IDS_DRIVE_FIXED        /* DRIVE_RAMDISK*/
                                };

static const REQUIREDREGITEM g_RequiredItems[] =
{
    { CLSID_ControlPanel, 0, 0x50 },
};
static const REGFOLDERINFO g_RegFolderInfo =
{
    PT_COMPUTER_REGITEM,
    _countof(g_RequiredItems), g_RequiredItems,
    CLSID_MyComputer,
    L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}",
    L"MyComputer",
};

static const CLSID* IsRegItem(PCUITEMID_CHILD pidl)
{
    if (pidl && pidl->mkid.cb == 2 + 2 + sizeof(CLSID))
    {
        if (pidl->mkid.abID[0] == PT_SHELLEXT || pidl->mkid.abID[0] == PT_GUID) // FIXME: Remove PT_GUID when CRegFolder is fixed
            return (const CLSID*)(&pidl->mkid.abID[2]);
    }
    return NULL;
}

BOOL _ILGetDriveType(LPCITEMIDLIST pidl)
{
    WCHAR szDrive[8];
    if (!_ILGetDrive(pidl, szDrive, _countof(szDrive)))
    {
        ERR("pidl %p is not a drive\n", pidl);
        return DRIVE_UNKNOWN;
    }
    return ::GetDriveTypeW(szDrive);
}

BOOL SHELL32_IsShellFolderNamespaceItemHidden(LPCWSTR SubKey, REFCLSID Clsid)
{
    // If this function returns true, the item should be hidden in DefView but not in the Explorer folder tree.
    WCHAR path[MAX_PATH], name[CHARS_IN_GUID];
    wsprintfW(path, L"%s\\%s", REGSTR_PATH_EXPLORER, SubKey);
    SHELL32_GUIDToStringW(Clsid, name);
    DWORD data = 0, size = sizeof(data);
    return !RegGetValueW(HKEY_CURRENT_USER, path, name, RRF_RT_DWORD, NULL, &data, &size) && data;
}

/***********************************************************************
*   IShellFolder implementation
*/

#define RETRY_COUNT 3
#define RETRY_SLEEP 250
static BOOL TryToLockOrUnlockDrive(HANDLE hDrive, BOOL bLock)
{
    DWORD dwError, dwBytesReturned;
    DWORD dwCode = (bLock ? FSCTL_LOCK_VOLUME : FSCTL_UNLOCK_VOLUME);
    for (DWORD i = 0; i < RETRY_COUNT; ++i)
    {
        if (DeviceIoControl(hDrive, dwCode, NULL, 0, NULL, 0, &dwBytesReturned, NULL))
            return TRUE;

        dwError = GetLastError();
        if (dwError == ERROR_INVALID_FUNCTION)
            break; /* don't sleep if function is not implemented */

        Sleep(RETRY_SLEEP);
    }
    SetLastError(dwError);
    return FALSE;
}

// NOTE: See also https://support.microsoft.com/en-us/help/165721/how-to-ejecting-removable-media-in-windows-nt-windows-2000-windows-xp
static BOOL DoEjectDrive(const WCHAR *physical, UINT nDriveType, INT *pnStringID)
{
    /* GENERIC_WRITE isn't needed for umount */
    DWORD dwAccessMode = GENERIC_READ;
    DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    HANDLE hDrive = CreateFile(physical, dwAccessMode, dwShareMode, 0, OPEN_EXISTING, 0, NULL);
    if (hDrive == INVALID_HANDLE_VALUE)
        return FALSE;

    BOOL bResult, bNeedUnlock = FALSE;
    DWORD dwBytesReturned, dwError = NO_ERROR;
    PREVENT_MEDIA_REMOVAL removal;
    do
    {
        bResult = TryToLockOrUnlockDrive(hDrive, TRUE);
        if (!bResult)
        {
            dwError = GetLastError();
            *pnStringID = IDS_CANTLOCKVOLUME; /* Unable to lock volume */
            break;
        }
        bResult = DeviceIoControl(hDrive, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
        if (!bResult)
        {
            dwError = GetLastError();
            *pnStringID = IDS_CANTDISMOUNTVOLUME; /* Unable to dismount volume */
            bNeedUnlock = TRUE;
            break;
        }
        removal.PreventMediaRemoval = FALSE;
        bResult = DeviceIoControl(hDrive, IOCTL_STORAGE_MEDIA_REMOVAL, &removal, sizeof(removal), NULL,
                                  0, &dwBytesReturned, NULL);
        if (!bResult)
        {
            *pnStringID = IDS_CANTEJECTMEDIA; /* Unable to eject media */
            dwError = GetLastError();
            bNeedUnlock = TRUE;
            break;
        }
        bResult = DeviceIoControl(hDrive, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
        if (!bResult)
        {
            *pnStringID = IDS_CANTEJECTMEDIA; /* Unable to eject media */
            dwError = GetLastError();
            bNeedUnlock = TRUE;
            break;
        }
    } while (0);

    if (bNeedUnlock)
    {
        TryToLockOrUnlockDrive(hDrive, FALSE);
    }

    CloseHandle(hDrive);

    SetLastError(dwError);
    return bResult;
}

static DWORD CALLBACK DoFormatDriveThread(LPVOID args)
{
    UINT nDrive = PtrToUlong(args);
    WCHAR szPath[] = { LOWORD(L'A' + nDrive), L'\0' }; // Arbitrary, just needs to include nDrive
    CStubWindow32 stub;
    HRESULT hr = stub.CreateStub(CStubWindow32::TYPE_FORMATDRIVE, szPath, NULL);
    if (FAILED(hr))
        return hr;
    SHFormatDrive(stub, nDrive, SHFMT_ID_DEFAULT, 0);
    return stub.DestroyWindow();
}

static HRESULT DoFormatDriveAsync(HWND hwnd, UINT nDrive)
{
    BOOL succ = SHCreateThread(DoFormatDriveThread, UlongToPtr(nDrive), CTF_PROCESS_REF, NULL);
    return succ ? S_OK : E_FAIL;
}

HRESULT CALLBACK DrivesContextMenuCallback(IShellFolder *psf,
                                           HWND         hwnd,
                                           IDataObject  *pdtobj,
                                           UINT         uMsg,
                                           WPARAM       wParam,
                                           LPARAM       lParam)
{
    if (uMsg != DFM_MERGECONTEXTMENU && uMsg != DFM_INVOKECOMMAND)
        return SHELL32_DefaultContextMenuCallBack(psf, pdtobj, uMsg);

    PIDLIST_ABSOLUTE pidlFolder;
    PUITEMID_CHILD *apidl;
    UINT cidl;
    UINT nDriveType;
    DWORD dwFlags;
    HRESULT hr = SH_GetApidlFromDataObject(pdtobj, &pidlFolder, &apidl, &cidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    WCHAR szDrive[8] = {0};
    if (!_ILGetDrive(apidl[0], szDrive, _countof(szDrive)))
    {
        ERR("pidl is not a drive\n");
        SHFree(pidlFolder);
        _ILFreeaPidl(apidl, cidl);
        return E_FAIL;
    }
    nDriveType = GetDriveTypeW(szDrive);
    GetVolumeInformationW(szDrive, NULL, 0, NULL, NULL, &dwFlags, NULL, 0);

// custom command IDs
#if 0 // Disabled until our menu building system is fixed
#define CMDID_FORMAT        0
#define CMDID_EJECT         1
#define CMDID_DISCONNECT    2
#else
/* FIXME: These IDs should start from 0, however there is difference
 * between ours and Windows' menu building systems, which should be fixed. */
#define CMDID_FORMAT        1
#define CMDID_EJECT         2
#define CMDID_DISCONNECT    3
#endif

    if (uMsg == DFM_MERGECONTEXTMENU)
    {
        QCMINFO *pqcminfo = (QCMINFO *)lParam;

        UINT idCmdFirst = pqcminfo->idCmdFirst;
        UINT idCmd = 0;
        if (!(dwFlags & FILE_READ_ONLY_VOLUME) && nDriveType != DRIVE_REMOTE)
        {
            /* add separator and Format */
            idCmd = idCmdFirst + CMDID_FORMAT;
            _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
            _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, idCmd, MFT_STRING, MAKEINTRESOURCEW(IDS_FORMATDRIVE), MFS_ENABLED);
        }
        if (nDriveType == DRIVE_REMOVABLE || nDriveType == DRIVE_CDROM)
        {
            /* add separator and Eject */
            idCmd = idCmdFirst + CMDID_EJECT;
            _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
            _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, idCmd, MFT_STRING, MAKEINTRESOURCEW(IDS_EJECT), MFS_ENABLED);
        }
        if (nDriveType == DRIVE_REMOTE)
        {
            /* add separator and Disconnect */
            idCmd = idCmdFirst + CMDID_DISCONNECT;
            _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
            _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, idCmd, MFT_STRING, MAKEINTRESOURCEW(IDS_DISCONNECT), MFS_ENABLED);
        }

        if (idCmd)
#if 0 // see FIXME above
            pqcminfo->idCmdFirst = ++idCmd;
#else
            pqcminfo->idCmdFirst = (idCmd + 2);
#endif
        hr = S_OK;
    }
    else if (uMsg == DFM_INVOKECOMMAND)
    {
        WCHAR wszBuf[4] = L"A:\\";
        wszBuf[0] = (WCHAR)szDrive[0];

        INT nStringID = 0;
        DWORD dwError = NO_ERROR;

        if (wParam == DFM_CMD_PROPERTIES)
        {
            ATLASSERT(pdtobj);
            hr = SHELL32_ShowFilesystemItemPropertiesDialogAsync(pdtobj);
            // Not setting nStringID because SHOpenPropSheet already displayed an error box
        }
        else
        {
            if (wParam == CMDID_FORMAT)
            {
                hr = DoFormatDriveAsync(hwnd, szDrive[0] - 'A');
            }
            else if (wParam == CMDID_EJECT)
            {
                /* do eject */
                WCHAR physical[10];
                wsprintfW(physical, _T("\\\\.\\%c:"), szDrive[0]);

                if (DoEjectDrive(physical, nDriveType, &nStringID))
                {
                    SHChangeNotify(SHCNE_MEDIAREMOVED, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, wszBuf, NULL);
                }
                else
                {
                    dwError = GetLastError();
                }
            }
            else if (wParam == CMDID_DISCONNECT)
            {
                /* do disconnect */
                wszBuf[2] = UNICODE_NULL;
                dwError = WNetCancelConnection2W(wszBuf, 0, FALSE);
                if (dwError == NO_ERROR)
                {
                    SHChangeNotify(SHCNE_DRIVEREMOVED, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, wszBuf, NULL);
                }
                else
                {
                    nStringID = IDS_CANTDISCONNECT;
                }
            }
        }

        if (nStringID != 0)
        {
            /* show error message */
            WCHAR szFormat[128], szMessage[128];
            LoadStringW(shell32_hInstance, nStringID, szFormat, _countof(szFormat));
            wsprintfW(szMessage, szFormat, dwError);
            MessageBoxW(hwnd, szMessage, NULL, MB_ICONERROR);
        }
    }

    SHFree(pidlFolder);
    _ILFreeaPidl(apidl, cidl);

    return hr;
}

HRESULT CDrivesContextMenu_CreateInstance(PCIDLIST_ABSOLUTE pidlFolder,
                                          HWND hwnd,
                                          UINT cidl,
                                          PCUITEMID_CHILD_ARRAY apidl,
                                          IShellFolder *psf,
                                          IContextMenu **ppcm)
{
    HKEY hKeys[2];
    UINT cKeys = 0;
    AddClassKeyToArray(L"Drive", hKeys, &cKeys);
    AddClassKeyToArray(L"Folder", hKeys, &cKeys);

    return CDefFolderMenu_Create2(pidlFolder, hwnd, cidl, apidl, psf, DrivesContextMenuCallback, cKeys, hKeys, ppcm);
}

static HRESULT
getIconLocationForDrive(IShellFolder *psf, PCITEMID_CHILD pidl, UINT uFlags,
                        LPWSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    WCHAR wszPath[MAX_PATH];
    WCHAR wszAutoRunInfPath[MAX_PATH];
    WCHAR wszValue[MAX_PATH], wszTemp[MAX_PATH];

    // get path
    if (!ILGetDisplayNameExW(psf, pidl, wszPath, 0))
        return E_FAIL;
    if (!PathIsDirectoryW(wszPath))
        return E_FAIL;

    // build the full path of autorun.inf
    StringCchCopyW(wszAutoRunInfPath, _countof(wszAutoRunInfPath), wszPath);
    PathAppendW(wszAutoRunInfPath, L"autorun.inf");

    // autorun.inf --> wszValue
    if (GetPrivateProfileStringW(L"autorun", L"icon", NULL, wszValue, _countof(wszValue),
                                 wszAutoRunInfPath) && wszValue[0] != 0)
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

    return E_FAIL;
}

static HRESULT
getLabelForDrive(LPWSTR wszPath, LPWSTR wszLabel)
{
    WCHAR wszAutoRunInfPath[MAX_PATH];
    WCHAR wszTemp[MAX_PATH];

    if (!PathIsDirectoryW(wszPath))
        return E_FAIL;

    StringCchCopyW(wszAutoRunInfPath, _countof(wszAutoRunInfPath), wszPath);
    PathAppendW(wszAutoRunInfPath, L"autorun.inf");

    if (GetPrivateProfileStringW(L"autorun", L"label", NULL, wszTemp, _countof(wszTemp),
                                 wszAutoRunInfPath) && wszTemp[0] != 0)
    {
        StringCchCopyW(wszLabel, _countof(wszTemp), wszTemp);
        return S_OK;
    }

    return E_FAIL;
}

BOOL IsDriveFloppyA(LPCSTR pszDriveRoot);

HRESULT CDrivesExtractIcon_CreateInstance(IShellFolder * psf, LPCITEMIDLIST pidl, REFIID riid, LPVOID * ppvOut)
{
    CComPtr<IDefaultExtractIconInit> initIcon;
    HRESULT hr = SHCreateDefaultExtractIcon(IID_PPV_ARG(IDefaultExtractIconInit, &initIcon));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CHAR* pszDrive = _ILGetDataPointer(pidl)->u.drive.szDriveName;
    UINT DriveType = GetDriveTypeA(pszDrive);
    if (DriveType > DRIVE_RAMDISK)
        DriveType = DRIVE_FIXED;

    WCHAR wTemp[MAX_PATH];
    int icon_idx, reg_idx;
    UINT flags = 0;

    switch (DriveType)
    {
        case DRIVE_FIXED:
        case DRIVE_UNKNOWN:
            reg_idx = IDI_SHELL_DRIVE;
            break;
        case DRIVE_CDROM:
            reg_idx = IDI_SHELL_CDROM;
            break;
        case DRIVE_REMOTE:
            reg_idx = IDI_SHELL_NETDRIVE;
            break;
        case DRIVE_REMOVABLE:
            if (!IsDriveFloppyA(pszDrive))
                reg_idx = IDI_SHELL_REMOVEABLE;
            else
                reg_idx = IDI_SHELL_3_14_FLOPPY;
            break;
        case DRIVE_RAMDISK:
            reg_idx = IDI_SHELL_RAMDISK;
            break;
        case DRIVE_NO_ROOT_DIR:
        default:
            reg_idx = IDI_SHELL_DOCUMENT;
            break;
    }

    hr = getIconLocationForDrive(psf, pidl, 0, wTemp, _countof(wTemp),
                                 &icon_idx, &flags);
    if (SUCCEEDED(hr))
    {
        initIcon->SetNormalIcon(wTemp, icon_idx);
    }
    else if (HLM_GetIconW(reg_idx - 1, wTemp, _countof(wTemp), &icon_idx))
    {
        initIcon->SetNormalIcon(wTemp, icon_idx);
    }
    else if ((DriveType == DRIVE_FIXED || DriveType == DRIVE_UNKNOWN) &&
             (HCR_GetIconW(L"Drive", wTemp, NULL, _countof(wTemp), &icon_idx)))
    {
        initIcon->SetNormalIcon(wTemp, icon_idx);
    }
    else
    {
        if (DriveType == DRIVE_REMOVABLE && !IsDriveFloppyA(pszDrive))
        {
            icon_idx = IDI_SHELL_REMOVEABLE;
        }
        else
        {
            icon_idx = iDriveIconIds[DriveType];
        }
        initIcon->SetNormalIcon(swShell32Name, -icon_idx);
    }

    return initIcon->QueryInterface(riid, ppvOut);
}

class CDrivesFolderEnum :
    public CEnumIDListBase
{
    public:
        HRESULT WINAPI Initialize(HWND hwndOwner, DWORD dwFlags, IEnumIDList* pRegEnumerator)
        {
            /* enumerate the folders */
            if (dwFlags & SHCONTF_FOLDERS)
            {
                WCHAR wszDriveName[] = {'A', ':', '\\', '\0'};
                DWORD dwDrivemap = GetLogicalDrives();

                while (wszDriveName[0] <= 'Z')
                {
                    if(dwDrivemap & 0x00000001L)
                        AddToEnumList(_ILCreateDrive(wszDriveName));
                    wszDriveName[0]++;
                    dwDrivemap = dwDrivemap >> 1;
                }
            }

            /* Enumerate the items of the reg folder */
            AppendItemsFromEnumerator(pRegEnumerator);

            return S_OK;
        }

        BEGIN_COM_MAP(CDrivesFolderEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

/***********************************************************************
*   IShellFolder [MyComputer] implementation
*/

static const shvheader MyComputerSFHeader[] = {
    {IDS_SHV_COLUMN_NAME, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 15},
    {IDS_SHV_COLUMN_TYPE, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 10},
    {IDS_SHV_COLUMN_DISK_CAPACITY, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN_DISK_AVAILABLE, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN_COMMENTS, SHCOLSTATE_TYPE_STR, LVCFMT_LEFT, 10},
};

static const DWORD dwComputerAttributes =
    SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET |
    SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_HASSUBFOLDER;
static const DWORD dwControlPanelAttributes =
    SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_CANLINK;
static const DWORD dwDriveAttributes =
    SFGAO_HASSUBFOLDER | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR |
    SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANRENAME | SFGAO_CANLINK;

CDrivesFolder::CDrivesFolder()
{
    pidlRoot = NULL;
}

CDrivesFolder::~CDrivesFolder()
{
    TRACE("-- destroying IShellFolder(%p)\n", this);
    SHFree(pidlRoot);
}

HRESULT WINAPI CDrivesFolder::FinalConstruct()
{
    pidlRoot = _ILCreateMyComputer();    /* my qualified pidl */
    if (pidlRoot == NULL)
        return E_OUTOFMEMORY;

    REGFOLDERINITDATA RegInit = { static_cast<IShellFolder*>(this), &g_RegFolderInfo };
    HRESULT hr = CRegFolder_CreateInstance(&RegInit,
                                           pidlRoot,
                                           IID_PPV_ARG(IShellFolder2, &m_regFolder));

    return hr;
}

/**************************************************************************
*    CDrivesFolder::ParseDisplayName
*/
HRESULT WINAPI CDrivesFolder::ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
        DWORD * pchEaten, PIDLIST_RELATIVE * ppidl, DWORD * pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n", this,
          hwndOwner, pbc, lpszDisplayName, debugstr_w (lpszDisplayName),
          pchEaten, ppidl, pdwAttributes);

    if (!ppidl)
        return hr;

    *ppidl = NULL;

    if (!lpszDisplayName)
        return hr;

    /* handle CLSID paths */
    if (lpszDisplayName[0] == L':' && lpszDisplayName[1] == L':')
    {
        return m_regFolder->ParseDisplayName(hwndOwner, pbc, lpszDisplayName, pchEaten, ppidl,
                                             pdwAttributes);
    }

    if (lpszDisplayName[0] &&
        ((L'A' <= lpszDisplayName[0] && lpszDisplayName[0] <= L'Z') ||
         (L'a' <= lpszDisplayName[0] && lpszDisplayName[0] <= L'z')) &&
        lpszDisplayName[1] == L':' && lpszDisplayName[2] == L'\\')
    {
        // "C:\..."
        WCHAR szRoot[8];
        PathBuildRootW(szRoot, ((*lpszDisplayName - 1) & 0x1F));

        if (SHIsFileSysBindCtx(pbc, NULL) != S_OK && !(BindCtx_GetMode(pbc, 0) & STGM_CREATE))
        {
            if (::GetDriveType(szRoot) == DRIVE_NO_ROOT_DIR)
                return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        }

        CComHeapPtr<ITEMIDLIST> pidlTemp(_ILCreateDrive(szRoot));
        if (!pidlTemp)
            return E_OUTOFMEMORY;

        if (lpszDisplayName[3])
        {
            CComPtr<IShellFolder> pChildFolder;
            hr = BindToObject(pidlTemp, pbc, IID_PPV_ARG(IShellFolder, &pChildFolder));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            ULONG chEaten;
            CComHeapPtr<ITEMIDLIST> pidlChild;
            hr = pChildFolder->ParseDisplayName(hwndOwner, pbc, &lpszDisplayName[3], &chEaten,
                                                &pidlChild, pdwAttributes);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            hr = SHILCombine(pidlTemp, pidlChild, ppidl);
        }
        else
        {
            *ppidl = pidlTemp.Detach();
            if (pdwAttributes && *pdwAttributes)
                GetAttributesOf(1, (PCUITEMID_CHILD_ARRAY)ppidl, pdwAttributes);
            hr = S_OK;
        }
    }

    TRACE("(%p)->(-- ret=0x%08x)\n", this, hr);

    return hr;
}

/**************************************************************************
*        CDrivesFolder::EnumObjects
*/
HRESULT WINAPI CDrivesFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    CComPtr<IEnumIDList> pRegEnumerator;
    m_regFolder->EnumObjects(hwndOwner, dwFlags, &pRegEnumerator);

    return ShellObjectCreatorInit<CDrivesFolderEnum>(hwndOwner, dwFlags, pRegEnumerator, IID_PPV_ARG(IEnumIDList, ppEnumIDList));
}

/**************************************************************************
*        CDrivesFolder::BindToObject
*/
HRESULT WINAPI CDrivesFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    TRACE("(%p)->(pidl=%p,%p,%s,%p)\n", this,
          pidl, pbcReserved, shdebugstr_guid(&riid), ppvOut);

    if (!pidl)
        return E_INVALIDARG;

    if (_ILIsSpecialFolder(pidl))
        return m_regFolder->BindToObject(pidl, pbcReserved, riid, ppvOut);

    CHAR* pchDrive = _ILGetDataPointer(pidl)->u.drive.szDriveName;

    PERSIST_FOLDER_TARGET_INFO pfti = {0};
    pfti.dwAttributes = -1;
    pfti.csidl = -1;
    pfti.szTargetParsingName[0] = *pchDrive;
    pfti.szTargetParsingName[1] = L':';
    pfti.szTargetParsingName[2] = L'\\';

    HRESULT hr = SHELL32_BindToSF(pidlRoot,
                                  &pfti,
                                  pidl,
                                  &CLSID_ShellFSFolder,
                                  riid,
                                  ppvOut);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

/**************************************************************************
*    CDrivesFolder::BindToStorage
*/
HRESULT WINAPI CDrivesFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    FIXME("(%p)->(pidl=%p,%p,%s,%p) stub\n", this,
          pidl, pbcReserved, shdebugstr_guid (&riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
*     CDrivesFolder::CompareIDs
*/

HRESULT WINAPI CDrivesFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    HRESULT hres;

    if (!pidl1 || !pidl2)
    {
        ERR("Got null pidl pointer (%Ix %p %p)!\n", lParam, pidl1, pidl2);
        return E_INVALIDARG;
    }

    if (_ILIsSpecialFolder(pidl1) || _ILIsSpecialFolder(pidl2))
        return m_regFolder->CompareIDs(lParam, pidl1, pidl2);

    UINT iColumn = LOWORD(lParam);
    if (!_ILIsDrive(pidl1) || !_ILIsDrive(pidl2) || iColumn >= _countof(MyComputerSFHeader))
        return E_INVALIDARG;

    CHAR* pszDrive1 = _ILGetDataPointer(pidl1)->u.drive.szDriveName;
    CHAR* pszDrive2 = _ILGetDataPointer(pidl2)->u.drive.szDriveName;

    int result;
    switch (MyComputerSFHeader[iColumn].colnameid)
    {
        case IDS_SHV_COLUMN_NAME:
        {
            result = _stricmp(pszDrive1, pszDrive2);
            hres = MAKE_COMPARE_HRESULT(result);
            break;
        }
        case IDS_SHV_COLUMN_TYPE:
        {
            /* We want to return immediately because SHELL32_CompareDetails also compares children. */
            return SHELL32_CompareDetails(this, lParam, pidl1, pidl2);
        }
        case IDS_SHV_COLUMN_DISK_CAPACITY:
        case IDS_SHV_COLUMN_DISK_AVAILABLE:
        {
            ULARGE_INTEGER Drive1Available, Drive1Total, Drive2Available, Drive2Total;

            if (GetVolumeInformationA(pszDrive1, NULL, 0, NULL, NULL, NULL, NULL, 0))
                GetDiskFreeSpaceExA(pszDrive1, &Drive1Available, &Drive1Total, NULL);
            else
                Drive1Available.QuadPart = Drive1Total.QuadPart = 0;

            if (GetVolumeInformationA(pszDrive2, NULL, 0, NULL, NULL, NULL, NULL, 0))
                GetDiskFreeSpaceExA(pszDrive2, &Drive2Available, &Drive2Total, NULL);
            else
                Drive2Available.QuadPart = Drive2Total.QuadPart = 0;

            LARGE_INTEGER Diff;
            if (lParam == 3) /* Size */
                Diff.QuadPart = Drive1Total.QuadPart - Drive2Total.QuadPart;
            else /* Size available */
                Diff.QuadPart = Drive1Available.QuadPart - Drive2Available.QuadPart;

            hres = MAKE_COMPARE_HRESULT(Diff.QuadPart);
            break;
        }
        case IDS_SHV_COLUMN_COMMENTS:
            hres = MAKE_COMPARE_HRESULT(0);
            break;
        DEFAULT_UNREACHABLE;
    }

    if (HRESULT_CODE(hres) == 0)
        return SHELL32_CompareChildren(this, lParam, pidl1, pidl2);

    return hres;
}

/**************************************************************************
*    CDrivesFolder::CreateViewObject
*/
HRESULT WINAPI CDrivesFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
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
        DEFCONTEXTMENU dcm = { hwndOwner, this, pidlRoot, this };
        hr = SHCreateDefaultContextMenu(&dcm, riid, ppvOut);
    }
    else if (IsEqualIID(riid, IID_IShellView))
    {
            SFV_CREATE sfvparams = { sizeof(SFV_CREATE), this, NULL, static_cast<IShellFolderViewCB*>(this) };
            hr = SHCreateShellFolderView(&sfvparams, (IShellView**)ppvOut);
    }
    TRACE("-- (%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

/**************************************************************************
*  CDrivesFolder::GetAttributesOf
*/
HRESULT WINAPI CDrivesFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD * rgfInOut)
{
    TRACE("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n",
          this, cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    /* FIXME: always add SFGAO_CANLINK */
    if(cidl == 0)
        *rgfInOut &= dwComputerAttributes;
    else
    {
        for (UINT i = 0; i < cidl; ++i)
        {
            if (_ILIsDrive(apidl[i]))
            {
                *rgfInOut &= dwDriveAttributes;

                if (_ILGetDriveType(apidl[i]) == DRIVE_CDROM)
                    *rgfInOut &= ~SFGAO_CANRENAME; // CD-ROM drive cannot rename
            }
            else if (_ILIsControlPanel(apidl[i]))
            {
                *rgfInOut &= dwControlPanelAttributes;
            }
            else if (_ILIsSpecialFolder(*apidl))
            {
                m_regFolder->GetAttributesOf(1, &apidl[i], rgfInOut);
            }
            else
            {
                ERR("Got unknown pidl type!\n");
            }
        }
    }

    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE("-- result=0x%08x\n", *rgfInOut);
    return S_OK;
}

/**************************************************************************
*    CDrivesFolder::GetUIObjectOf
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
HRESULT WINAPI CDrivesFolder::GetUIObjectOf(HWND hwndOwner,
    UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
    REFIID riid, UINT *prgfInOut, LPVOID *ppvOut)
{
    LPVOID pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n", this,
          hwndOwner, cidl, apidl, shdebugstr_guid (&riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, IID_IContextMenu) && (cidl >= 1))
    {
        if (_ILIsDrive(apidl[0]))
            hr = CDrivesContextMenu_CreateInstance(pidlRoot, hwndOwner, cidl, apidl, static_cast<IShellFolder*>(this), (IContextMenu**)&pObj);
        else
            hr = m_regFolder->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, &pObj);
    }
    else if (IsEqualIID (riid, IID_IDataObject) && (cidl >= 1))
    {
        hr = IDataObject_Constructor(hwndOwner,
                                     pidlRoot, apidl, cidl, TRUE, (IDataObject **)&pObj);
    }
    else if ((IsEqualIID (riid, IID_IExtractIconA) || IsEqualIID (riid, IID_IExtractIconW)) && (cidl == 1))
    {
        if (_ILIsDrive(apidl[0]))
            hr = CDrivesExtractIcon_CreateInstance(this, apidl[0], riid, &pObj);
        else
            hr = m_regFolder->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, &pObj);
    }
    else if (IsEqualIID (riid, IID_IDropTarget) && (cidl == 1))
    {
        CComPtr<IShellFolder> psfChild;
        hr = this->BindToObject(apidl[0], NULL, IID_PPV_ARG(IShellFolder, &psfChild));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return psfChild->CreateViewObject(NULL, riid, ppvOut);
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
*    CDrivesFolder::GetDisplayNameOf
*/
HRESULT WINAPI CDrivesFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    LPWSTR pszPath;
    HRESULT hr = S_OK;

    TRACE("(%p)->(pidl=%p,0x%08x,%p)\n", this, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    if (!_ILIsPidlSimple (pidl))
    {
        return SHELL32_GetDisplayNameOfChild(this, pidl, dwFlags, strRet);
    }
    else if (_ILIsSpecialFolder(pidl))
    {
        return m_regFolder->GetDisplayNameOf(pidl, dwFlags, strRet);
    }
    else if (!_ILIsDrive(pidl))
    {
        ERR("Wrong pidl type\n");
        return E_INVALIDARG;
    }

    pszPath = (LPWSTR)CoTaskMemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    pszPath[0] = 0;

    _ILSimpleGetTextW(pidl, pszPath, MAX_PATH);    /* append my own path */
    /* long view "lw_name (C:)" */
    if (!(dwFlags & SHGDN_FORPARSING))
    {
        WCHAR wszDrive[18] = {0};

        lstrcpynW(wszDrive, pszPath, 4);
        pszPath[0] = L'\0';

        if (!SUCCEEDED(getLabelForDrive(wszDrive, pszPath)))
        {
            DWORD dwVolumeSerialNumber, dwMaximumComponentLength, dwFileSystemFlags;

            GetVolumeInformationW(wszDrive, pszPath,
                                  MAX_PATH - 7,
                                  &dwVolumeSerialNumber,
                                  &dwMaximumComponentLength, &dwFileSystemFlags, NULL, 0);
            pszPath[MAX_PATH-1] = L'\0';

            if (!wcslen(pszPath))
            {
                UINT DriveType, ResourceId;
                DriveType = GetDriveTypeW(wszDrive);

                switch (DriveType)
                {
                    case DRIVE_FIXED:
                        ResourceId = IDS_DRIVE_FIXED;
                        break;
                    case DRIVE_REMOTE:
                        ResourceId = IDS_DRIVE_NETWORK;
                        break;
                    case DRIVE_CDROM:
                        ResourceId = IDS_DRIVE_CDROM;
                        break;
                    default:
                        ResourceId = 0;
                }

                if (ResourceId)
                {
                    dwFileSystemFlags = LoadStringW(shell32_hInstance, ResourceId, pszPath, MAX_PATH);
                    if (dwFileSystemFlags > MAX_PATH - 7)
                        pszPath[MAX_PATH-7] = L'\0';
                }
            }
        }
        wcscat(pszPath, L" (");
        wszDrive[2] = L'\0';
        wcscat(pszPath, wszDrive);
        wcscat(pszPath, L")");
    }

    if (SUCCEEDED(hr))
    {
        strRet->uType = STRRET_WSTR;
        strRet->pOleStr = pszPath;
    }
    else
        CoTaskMemFree(pszPath);

    TRACE("-- (%p)->(%s)\n", this, strRet->uType == STRRET_CSTR ? strRet->cStr : debugstr_w(strRet->pOleStr));
    return hr;
}

/**************************************************************************
*  CDrivesFolder::SetNameOf
*  Changes the name of a file object or subfolder, possibly changing its item
*  identifier in the process.
*
* PARAMETERS
*  hwndOwner  [in]   Owner window for output
*  pidl       [in]   simple pidl of item to change
*  lpszName   [in]   the items new display name
*  dwFlags    [in]   SHGNO formatting flags
*  ppidlOut   [out]  simple pidl returned
*/
HRESULT WINAPI CDrivesFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl,
                                        LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    WCHAR szName[30];

    if (_ILIsDrive(pidl))
    {
        if (_ILSimpleGetTextW(pidl, szName, _countof(szName)))
            SetVolumeLabelW(szName, lpName);
        if (pPidlOut)
            *pPidlOut = _ILCreateDrive(szName);
        return S_OK;
    }

    return m_regFolder->SetNameOf(hwndOwner, pidl, lpName, dwFlags, pPidlOut);
}

HRESULT WINAPI CDrivesFolder::GetDefaultSearchGUID(GUID * pguid)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDrivesFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDrivesFolder::GetDefaultColumn (DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    TRACE("(%p)\n", this);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;
    return S_OK;
}

HRESULT WINAPI CDrivesFolder::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF * pcsFlags)
{
    TRACE("(%p)\n", this);

    if (!pcsFlags || iColumn >= _countof(MyComputerSFHeader))
        return E_INVALIDARG;
    *pcsFlags = MyComputerSFHeader[iColumn].colstate;
    return S_OK;
}

HRESULT WINAPI CDrivesFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDrivesFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    HRESULT hr;

    TRACE("(%p)->(%p %i %p)\n", this, pidl, iColumn, psd);

    if (!psd || iColumn >= _countof(MyComputerSFHeader))
        return E_INVALIDARG;

    if (!pidl)
    {
        psd->fmt = MyComputerSFHeader[iColumn].fmt;
        psd->cxChar = MyComputerSFHeader[iColumn].cxChar;
        return SHSetStrRet(&psd->str, MyComputerSFHeader[iColumn].colnameid);
    }
    else if (!_ILIsDrive(pidl))
    {
        switch (MyComputerSFHeader[iColumn].colnameid)
        {
            case IDS_SHV_COLUMN_NAME:
            case IDS_SHV_COLUMN_TYPE:
                return m_regFolder->GetDetailsOf(pidl, iColumn, psd);
            case IDS_SHV_COLUMN_DISK_CAPACITY:
            case IDS_SHV_COLUMN_DISK_AVAILABLE:
                return SHSetStrRet(&psd->str, ""); /* blank col */
            case IDS_SHV_COLUMN_COMMENTS:
                return m_regFolder->GetDetailsOf(pidl, 2, psd); /* 2 = comments */
            DEFAULT_UNREACHABLE;
        }
    }
    else
    {
        ULARGE_INTEGER ulTotalBytes, ulFreeBytes;
        CHAR* pszDrive = _ILGetDataPointer(pidl)->u.drive.szDriveName;
        UINT DriveType = GetDriveTypeA(pszDrive);
        if (DriveType > DRIVE_RAMDISK)
            DriveType = DRIVE_FIXED;

        switch (MyComputerSFHeader[iColumn].colnameid)
        {
            case IDS_SHV_COLUMN_NAME:
                hr = GetDisplayNameOf(pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
                break;
            case IDS_SHV_COLUMN_TYPE:
                if (DriveType == DRIVE_REMOVABLE && !IsDriveFloppyA(pszDrive))
                    hr = SHSetStrRet(&psd->str, IDS_DRIVE_REMOVABLE);
                else
                    hr = SHSetStrRet(&psd->str, iDriveTypeIds[DriveType]);
                break;
            case IDS_SHV_COLUMN_DISK_CAPACITY:
            case IDS_SHV_COLUMN_DISK_AVAILABLE:
                psd->str.cStr[0] = 0x00;
                psd->str.uType = STRRET_CSTR;
                if (GetVolumeInformationA(pszDrive, NULL, 0, NULL, NULL, NULL, NULL, 0))
                {
                    GetDiskFreeSpaceExA(pszDrive, &ulFreeBytes, &ulTotalBytes, NULL);
                    if (iColumn == 2)
                        StrFormatByteSize64A(ulTotalBytes.QuadPart, psd->str.cStr, MAX_PATH);
                    else
                        StrFormatByteSize64A(ulFreeBytes.QuadPart, psd->str.cStr, MAX_PATH);
                }
                hr = S_OK;
                break;
            case IDS_SHV_COLUMN_COMMENTS:
                hr = SHSetStrRet(&psd->str, ""); /* FIXME: comments */
                break;
            DEFAULT_UNREACHABLE;
        }
    }

    return hr;
}

HRESULT WINAPI CDrivesFolder::MapColumnToSCID(UINT column, SHCOLUMNID * pscid)
{
    FIXME("(%p)\n", this);
    return E_NOTIMPL;
}

/************************************************************************
 *    CDrivesFolder::GetClassID
 */
HRESULT WINAPI CDrivesFolder::GetClassID(CLSID *lpClassId)
{
    TRACE("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_MyComputer;
    return S_OK;
}

/************************************************************************
 *    CDrivesFolder::Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
HRESULT WINAPI CDrivesFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    return S_OK;
}

/**************************************************************************
 *    CDrivesFolder::GetCurFolder
 */
HRESULT WINAPI CDrivesFolder::GetCurFolder(PIDLIST_ABSOLUTE *pidl)
{
    TRACE("(%p)->(%p)\n", this, pidl);

    if (!pidl)
        return E_INVALIDARG; /* xp doesn't have this check and crashes on NULL */

    *pidl = ILClone(pidlRoot);
    return S_OK;
}

/**************************************************************************
 *    CDrivesFolder::ShouldShow
 */
HRESULT WINAPI CDrivesFolder::ShouldShow(IShellFolder *psf, PCIDLIST_ABSOLUTE pidlFolder, PCUITEMID_CHILD pidlItem)
{
    if (const CLSID* pClsid = IsRegItem(pidlItem))
        return SHELL32_IsShellFolderNamespaceItemHidden(L"HideMyComputerIcons", *pClsid) ? S_FALSE : S_OK;
    return S_OK;
}

/************************************************************************/
/* IContextMenuCB interface */

HRESULT WINAPI CDrivesFolder::CallBack(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    enum { IDC_PROPERTIES };
    /* no data object means no selection */
    if (!pdtobj)
    {
        if (uMsg == DFM_INVOKECOMMAND && wParam == IDC_PROPERTIES)
        {
            // "System" properties
            return SHELL_ExecuteControlPanelCPL(hwndOwner, L"sysdm.cpl") ? S_OK : E_FAIL;
        }
        else if (uMsg == DFM_MERGECONTEXTMENU) // TODO: DFM_MERGECONTEXTMENU_BOTTOM
        {
            QCMINFO *pqcminfo = (QCMINFO *)lParam;
            HMENU hpopup = CreatePopupMenu();
            _InsertMenuItemW(hpopup, 0, TRUE, IDC_PROPERTIES, MFT_STRING, MAKEINTRESOURCEW(IDS_PROPERTIES), MFS_ENABLED);
            pqcminfo->idCmdFirst = Shell_MergeMenus(pqcminfo->hmenu, hpopup, pqcminfo->indexMenu, pqcminfo->idCmdFirst, pqcminfo->idCmdLast, MM_ADDSEPARATOR);
            DestroyMenu(hpopup);
            return S_OK;
        }
    }
    return SHELL32_DefaultContextMenuCallBack(psf, pdtobj, uMsg);
}
