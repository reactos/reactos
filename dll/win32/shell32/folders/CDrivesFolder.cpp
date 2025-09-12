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

static int iDriveIconIds[7] = { IDI_SHELL_NOT_CONNECTED_HDD, /* DRIVE_UNKNOWN */
                                IDI_SHELL_NOT_CONNECTED_HDD, /* DRIVE_NO_ROOT_DIR*/
                                IDI_SHELL_REMOVEABLE,        /* DRIVE_REMOVABLE*/
                                IDI_SHELL_DRIVE,             /* DRIVE_FIXED*/
                                IDI_SHELL_NETDRIVE,          /* DRIVE_REMOTE*/
                                IDI_SHELL_CDROM,             /* DRIVE_CDROM*/
                                IDI_SHELL_RAMDISK            /* DRIVE_RAMDISK*/
                                };

static int iDriveTypeIds[7] = { IDS_DRIVE_FIXED,             /* DRIVE_UNKNOWN */
                                IDS_DRIVE_FIXED,             /* DRIVE_NO_ROOT_DIR*/
                                IDS_DRIVE_REMOVABLE,         /* DRIVE_REMOVABLE*/
                                IDS_DRIVE_FIXED,             /* DRIVE_FIXED*/
                                IDS_DRIVE_NETWORK,           /* DRIVE_REMOTE*/
                                IDS_DRIVE_CDROM,             /* DRIVE_CDROM*/
                                IDS_DRIVE_FIXED /* FIXME */  /* DRIVE_RAMDISK*/
                                };

static const REQUIREDREGITEM g_RequiredItems[] =
{
    { CLSID_ControlPanel, NULL, REGITEMORDER_MYCOMPUTER_CONTROLS },
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

static bool IsRegItem(PCUITEMID_CHILD pidl, REFCLSID clsid)
{
    const CLSID *pClass = IsRegItem(pidl);
    return pClass && *pClass == clsid;
}

static INT8 GetDriveNumber(PCUITEMID_CHILD pidl)
{
    if (!_ILIsDrive(pidl))
        return -1;
    BYTE letter = ((PIDLDATA*)pidl->mkid.abID)->u.drive.szDriveName[0];
    BYTE number = (letter | 32) - 'a';
    return number < 26 ? number : -1;
}

static INT8 GetDriveNumber(PCWSTR DrivePath)
{
    WORD number = (DrivePath[0] | 32) - 'a';
    return number < 26 ? number : -1;
}

UINT g_IsFloppyCache = 0;

static UINT GetCachedDriveType(INT8 DrvNum)
{
    if (DrvNum < 0 || DrvNum >= 26)
        return DRIVE_UNKNOWN;
    if (g_IsFloppyCache & (1 << DrvNum))
        return DRIVE_REMOVABLE;
    WCHAR DrvPath[] = { LOWORD('A' + DrvNum), ':', '\\', '\0' };
    return ::GetDriveTypeW(DrvPath);
}

static inline UINT GetCachedDriveType(PCWSTR DrivePath)
{
    return GetCachedDriveType(GetDriveNumber(DrivePath));
}

static bool IsFloppyDrive(PCWSTR DrivePath)
{
    extern BOOL IsDriveFloppyW(LPCWSTR pszDriveRoot);
    INT8 DrvNum = GetDriveNumber(DrivePath);
    if (DrvNum < 0)
        return false;
    if (g_IsFloppyCache & (1 << DrvNum))
        return true;
    UINT Type = GetCachedDriveType(DrvNum);
    if (Type == DRIVE_REMOVABLE && IsDriveFloppyW(DrivePath))
    {
        g_IsFloppyCache |= (1 << DrvNum);
        return true;
    }
    return false;
}

INT8 GetDrivePath(PCUITEMID_CHILD pidl, PWSTR Path)
{
    INT8 number = GetDriveNumber(pidl);
    if (number < 0)
        return number;
    Path[0] = 'A' + number;
    Path[1] = ':';
    Path[2] = '\\';
    Path[3] = '\0';
    return number;
}

static inline UINT _ILGetRemovableTypeId(LPCITEMIDLIST pidl)
{
    WCHAR buf[8];
    if (GetDrivePath(pidl, buf) >= 0 && IsFloppyDrive(buf))
        return SHDID_COMPUTER_DRIVE35; // TODO: 3.5-inch vs 5.25-inch
    return SHDID_COMPUTER_REMOVABLE;
}

UINT _ILGetDriveType(LPCITEMIDLIST pidl)
{
    WCHAR szDrive[8];
    if (!_ILGetDrive(pidl, szDrive, _countof(szDrive)))
    {
        ERR("pidl %p is not a drive\n", pidl);
        return DRIVE_UNKNOWN;
    }
    return GetCachedDriveType(GetDriveNumber(szDrive));
}

static UINT SHELL_GetAutoRunInfPath(PCWSTR DrvPath, PWSTR AriPath, BOOL ForInvoke = FALSE)
{
    INT8 DrvNum = GetDriveNumber(DrvPath);
    if (DrvNum < 0 || DrvPath[1] != ':' || (DrvPath[2] && (DrvPath[2] != '\\' || DrvPath[3])))
        return 0;
    if (!ForInvoke && IsFloppyDrive(DrvPath))
        return 0; // Don't read label nor icon from floppy
    if (GetCachedDriveType(DrvNum) <= DRIVE_NO_ROOT_DIR)
        return 0;
    return wsprintfW(AriPath, L"%c:\\autorun.inf", DrvPath[0]);
}

#if 0 // TODO: Call this when the shell is notified about insert disc events
bool SHELL_CanInvokeAutoRunOnDrive(PCWSTR DrvPath)
{
    INT8 DrvNum = GetDriveNumber(DrvPath);
    if (DrvNum < 0 || (SHRestricted(REST_NODRIVEAUTORUN) & (1 << DrvNum)))
        return false;
    return !(SHRestricted(REST_NODRIVETYPEAUTORUN) & (1 << GetCachedDriveType(DrvNum)));
}
#endif

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
    UINT nDriveType = GetCachedDriveType(szDrive);
    if (!GetVolumeInformationW(szDrive, NULL, 0, NULL, NULL, &dwFlags, NULL, 0))
    {
        if (nDriveType >= DRIVE_REMOTE)
            dwFlags = FILE_READ_ONLY_VOLUME;
        else
            dwFlags = 0; // Assume drive with unknown filesystem, allow format
    }

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
        if (!(dwFlags & FILE_READ_ONLY_VOLUME) && nDriveType != DRIVE_REMOTE && cidl == 1)
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
    CRegKeyHandleArray keys;
    AddClassKeyToArray(L"Drive", keys, keys);
    AddClassKeyToArray(L"Folder", keys, keys);
    return CDefFolderMenu_Create2(pidlFolder, hwnd, cidl, apidl, psf, DrivesContextMenuCallback, keys, keys, ppcm);
}

static HRESULT
getIconLocationForDrive(
    _In_ PCWSTR pszDrivePath,
    _Out_writes_z_(cchMax) PWSTR szIconFile,
    _In_ UINT cchMax,
    _Out_ int *piIndex,
    _Out_ UINT *pGilOut)
{
    WCHAR wszPath[MAX_PATH];
    WCHAR wszAutoRunInfPath[MAX_PATH];
    WCHAR wszValue[MAX_PATH], wszTemp[MAX_PATH];

    if (!SHELL_GetAutoRunInfPath(pszDrivePath, wszAutoRunInfPath))
        return E_FAIL;
    wcscpy(wszPath, pszDrivePath);

    // autorun.inf --> wszValue
    if (GetPrivateProfileStringW(L"autorun", L"icon", NULL, wszValue, _countof(wszValue),
                                 wszAutoRunInfPath) && wszValue[0] != 0)
    {
        // wszValue --> wszTemp
        ExpandEnvironmentStringsW(wszValue, wszTemp, _countof(wszTemp));

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
getLabelForDriveFromAutoRun(PCWSTR wszPath, LPWSTR szLabel, UINT cchMax)
{
    WCHAR wszAutoRunInfPath[MAX_PATH];
    WCHAR wszTemp[MAX_PATH];

    if (!SHELL_GetAutoRunInfPath(wszPath, wszAutoRunInfPath))
        return E_FAIL;

    if (GetPrivateProfileStringW(L"autorun", L"label", NULL, wszTemp, _countof(wszTemp),
                                 wszAutoRunInfPath) && wszTemp[0] != 0)
    {
        return StringCchCopyW(szLabel, cchMax, wszTemp);
    }
    return E_FAIL;
}

static inline HRESULT GetRawDriveLabel(PCWSTR DrivePath, LPWSTR szLabel, UINT cchMax)
{
    if (GetVolumeInformationW(DrivePath, szLabel, cchMax, NULL, NULL, NULL, NULL, 0))
        return *szLabel ? S_OK : S_FALSE;
    return HResultFromWin32(GetLastError());
}

static HRESULT GetDriveLabel(PCWSTR DrivePath, LPWSTR szLabel, UINT cchMax)
{
    HRESULT hr = getLabelForDriveFromAutoRun(DrivePath, szLabel, cchMax);
    return hr == S_OK ? S_OK : GetRawDriveLabel(DrivePath, szLabel, cchMax);
}

static bool GetRegCustomizedDriveIcon(
    _In_ INT8 DriveNum,
    _Out_writes_z_(cchMax) PWSTR szIconFile,
    _In_ UINT cchMax,
    _Out_ int *piIndex,
    _Out_ UINT *pGilOut)
{
    WCHAR szKey[200], chDrv = 'A' + DriveNum;
    wsprintfW(szKey, L"%s\\DriveIcons\\%c\\DefaultIcon", REGSTR_PATH_EXPLORER, chDrv);
    DWORD cb = cchMax * sizeof(*szIconFile);
    DWORD err = RegGetValueW(HKEY_LOCAL_MACHINE, szKey, NULL, RRF_RT_REG_SZ, NULL, szIconFile, &cb);
    if (err != ERROR_SUCCESS)
    {
        cb = cchMax * sizeof(*szIconFile);
        wsprintfW(szKey, L"Applications\\Explorer.exe\\Drives\\%c\\DefaultIcon", chDrv);
        err = RegGetValueW(HKEY_CLASSES_ROOT, szKey, NULL, RRF_RT_REG_SZ, NULL, szIconFile, &cb);
    }
    if (err != ERROR_SUCCESS)
        return false;
    *piIndex = PathParseIconLocationW(szIconFile);
    return *szIconFile != UNICODE_NULL;
}

HRESULT CDrivesExtractIcon_CreateInstance(IShellFolder * psf, LPCITEMIDLIST pidl, REFIID riid, LPVOID * ppvOut)
{
    CComPtr<IDefaultExtractIconInit> initIcon;
    HRESULT hr = SHCreateDefaultExtractIcon(IID_PPV_ARG(IDefaultExtractIconInit, &initIcon));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    WCHAR wTemp[MAX_PATH], szDrive[8];
    int icon_idx, reg_idx = 0;
    UINT GilOut = 0;
    BOOL bFloppy = FALSE;
    INT8 DriveNum = GetDrivePath(pidl, szDrive);
    UINT DriveType = GetCachedDriveType(DriveNum);
    if (DriveType > DRIVE_RAMDISK)
        DriveType = DRIVE_FIXED;

    switch (DriveType)
    {
        case DRIVE_REMOVABLE:
            bFloppy = IsFloppyDrive(szDrive);
            reg_idx = bFloppy ? IDI_SHELL_3_14_FLOPPY : IDI_SHELL_REMOVEABLE;
            break;
        case DRIVE_FIXED:
            reg_idx = IDI_SHELL_DRIVE;
            break;
        case DRIVE_REMOTE:
            reg_idx = IDI_SHELL_NETDRIVE;
            break;
        case DRIVE_CDROM:
            reg_idx = IDI_SHELL_CDROM;
            break;
        case DRIVE_RAMDISK:
            reg_idx = IDI_SHELL_RAMDISK;
            break;
    }

    hr = getIconLocationForDrive(szDrive, wTemp, _countof(wTemp),
                                 &icon_idx, &GilOut);
    if (SUCCEEDED(hr))
    {
        initIcon->SetNormalIcon(wTemp, icon_idx);
    }
    else if (DriveType > DRIVE_NO_ROOT_DIR &&
             GetRegCustomizedDriveIcon(DriveNum, wTemp, _countof(wTemp), &icon_idx, &GilOut))
    {
        initIcon->SetNormalIcon(wTemp, icon_idx);
    }
    else if (reg_idx && HLM_GetIconW(reg_idx - 1, wTemp, _countof(wTemp), &icon_idx))
    {
        initIcon->SetNormalIcon(wTemp, icon_idx);
    }
    else if (DriveType == DRIVE_FIXED &&
             HCR_GetIconW(L"Drive", wTemp, NULL, _countof(wTemp), &icon_idx))
    {
        initIcon->SetNormalIcon(wTemp, icon_idx);
    }
    else
    {
        if (DriveType == DRIVE_REMOVABLE && bFloppy)
            icon_idx = IDI_SHELL_3_14_FLOPPY;
        else
            icon_idx = iDriveIconIds[DriveType];
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
                DWORD dwDrivemap = GetLogicalDrives() & ~SHRestricted(REST_NODRIVES);
                for (; wszDriveName[0] <= 'Z'; wszDriveName[0]++)
                {
                    if (dwDrivemap & 1)
                        AddToEnumList(_ILCreateDrive(wszDriveName));
                    dwDrivemap >>= 1;
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
    SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_CANLINK;
static const DWORD dwControlPanelAttributes =
    SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_CANLINK;
static const DWORD dwDriveAttributes =
    SFGAO_HASSUBFOLDER | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR |
    SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANRENAME | SFGAO_CANLINK | SFGAO_CANCOPY;

CDrivesFolder::CDrivesFolder()
{
    pidlRoot = NULL;
    m_DriveDisplayMode = -1;
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

    if (((L'A' <= lpszDisplayName[0] && lpszDisplayName[0] <= L'Z') ||
         (L'a' <= lpszDisplayName[0] && lpszDisplayName[0] <= L'z')) &&
        lpszDisplayName[1] == L':' && (lpszDisplayName[2] == L'\\' || !lpszDisplayName[2]))
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

        if (lpszDisplayName[2] && lpszDisplayName[3])
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
            else if (IsRegItem(apidl[i], CLSID_ControlPanel))
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
    WCHAR szDrive[8];

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
    else if (GetDrivePath(pidl, szDrive) < 0)
    {
        ERR("Wrong pidl type\n");
        return E_INVALIDARG;
    }

    PWSTR pszPath = (LPWSTR)CoTaskMemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;
    pszPath[0] = UNICODE_NULL;
    szDrive[0] &= ~32; // Always uppercase

    /* long view "lw_name (C:)" */
    BOOL bEditLabel = GET_SHGDN_RELATION(dwFlags) == SHGDN_INFOLDER && (dwFlags & SHGDN_FOREDITING);
    if (!(dwFlags & SHGDN_FORPARSING))
    {
        if (m_DriveDisplayMode < 0)
        {
            DWORD err, type, data, cb = sizeof(data);
            err = SHRegGetUSValueW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                                   L"ShowDriveLettersFirst", &type, &data, &cb, FALSE, NULL, 0);
            m_DriveDisplayMode = (!err && type == REG_DWORD && cb == sizeof(data)) ? (BYTE)data : 0;
        }
        BOOL bRemoteFirst = m_DriveDisplayMode == 1;
        BOOL bNoLetter = m_DriveDisplayMode == 2;
        BOOL bAllFirst = m_DriveDisplayMode == 4;
        PWSTR pszLabel = pszPath;

        if (!bNoLetter && (bAllFirst || (bRemoteFirst && GetCachedDriveType(szDrive) == DRIVE_REMOTE)))
        {
            bNoLetter = TRUE; // Handling the letter now, don't append it again later
            if (!bEditLabel)
                pszLabel += wsprintfW(pszPath, L"(%c:) ", szDrive[0]);
        }

        if (GetDriveLabel(szDrive, pszLabel, MAX_PATH - 7) != S_OK && !bEditLabel)
        {
            UINT ResId = 0, DrvType = GetCachedDriveType(szDrive);
            if (DrvType == DRIVE_REMOVABLE)
                ResId = IsFloppyDrive(szDrive) ? IDS_DRIVE_FLOPPY : IDS_DRIVE_REMOVABLE;
            else if (DrvType < _countof(iDriveTypeIds))
                ResId = iDriveTypeIds[DrvType];

            if (ResId)
            {
                UINT len = LoadStringW(shell32_hInstance, ResId, pszLabel, MAX_PATH - 7);
                if (len > MAX_PATH - 7)
                    pszLabel[MAX_PATH-7] = UNICODE_NULL;
            }
        }

        if (!*pszLabel && !bEditLabel) // No label nor fallback description, use SHGDN_FORPARSING
            *pszPath = UNICODE_NULL;
        else if (!bNoLetter && !bEditLabel)
            wsprintfW(pszPath + wcslen(pszPath), L" (%c:)", szDrive[0]);
    }

    if (!*pszPath && !bEditLabel) // SHGDN_FORPARSING or failure above (except editing empty label)
    {
        if (GET_SHGDN_RELATION(dwFlags) == SHGDN_INFOLDER)
            szDrive[2] = UNICODE_NULL; // Remove backslash
        wcscpy(pszPath, szDrive);
    }
    strRet->uType = STRRET_WSTR;
    strRet->pOleStr = pszPath;

    TRACE("-- (%p)->(%s)\n", this, strRet->uType == STRRET_CSTR ? strRet->cStr : debugstr_w(strRet->pOleStr));
    return S_OK;
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
    if (_ILIsDrive(pidl))
    {
        WCHAR szDrive[8];
        HRESULT hr = GetDrivePath(pidl, szDrive) >= 0 ? SetDriveLabel(hwndOwner, szDrive, lpName) : E_FAIL;
        if (pPidlOut)
            *pPidlOut = SUCCEEDED(hr) ? _ILCreateDrive(szDrive) : NULL;
        return hr;
    }
    return m_regFolder->SetNameOf(hwndOwner, pidl, lpName, dwFlags, pPidlOut);
}

HRESULT CDrivesFolder::SetDriveLabel(HWND hwndOwner, PCWSTR DrivePath, PCWSTR Label)
{
    HRESULT hr = SetVolumeLabelW(DrivePath, *Label ? Label : NULL) ? S_OK : HResultFromWin32(GetLastError());
    if (SUCCEEDED(hr))
        SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATHW, DrivePath, DrivePath); // DisplayName changed
    else if (hwndOwner)
        SHELL_ErrorBox(hwndOwner, hr);
    return hr;
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

HRESULT WINAPI CDrivesFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    const CLSID *pCLSID = IsRegItem(pidl);
    if (pscid->fmtid == FMTID_ShellDetails)
    {
        switch (pscid->pid)
        {
            case PID_DESCRIPTIONID:
            {
                if (pCLSID)
                    return SHELL_CreateSHDESCRIPTIONID(pv, SHDID_ROOT_REGITEM, pCLSID);
                UINT id = SHDID_COMPUTER_OTHER;
                switch (_ILGetDriveType(pidl))
                {
                    case DRIVE_REMOVABLE: id = _ILGetRemovableTypeId(pidl); break;
                    case DRIVE_FIXED:     id = SHDID_COMPUTER_FIXED; break;
                    case DRIVE_REMOTE:    id = SHDID_COMPUTER_NETDRIVE; break;
                    case DRIVE_CDROM:     id = SHDID_COMPUTER_CDROM; break;
                    case DRIVE_RAMDISK:   id = SHDID_COMPUTER_RAMDISK; break;
                }
                return SHELL_CreateSHDESCRIPTIONID(pv, id, &CLSID_NULL);
            }
        }
    }
    if (pCLSID)
        return m_regFolder->GetDetailsEx(pidl, pscid, pv);
    return SH32_GetDetailsOfPKeyAsVariant(this, pidl, pscid, pv, FALSE);
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
                return m_regFolder->GetDetailsOf(pidl, SHFSF_COL_NAME, psd);
            case IDS_SHV_COLUMN_TYPE:
                return m_regFolder->GetDetailsOf(pidl, SHFSF_COL_TYPE, psd);
            case IDS_SHV_COLUMN_DISK_CAPACITY:
            case IDS_SHV_COLUMN_DISK_AVAILABLE:
                return SHSetStrRetEmpty(&psd->str);
            case IDS_SHV_COLUMN_COMMENTS:
                return m_regFolder->GetDetailsOf(pidl, SHFSF_COL_COMMENT, psd);
            DEFAULT_UNREACHABLE;
        }
    }
    else
    {
        ULARGE_INTEGER ulTotalBytes, ulFreeBytes;
        WCHAR szDrive[8];
        INT8 DriveNum = GetDrivePath(pidl, szDrive);
        UINT DriveType = GetCachedDriveType(DriveNum);
        if (DriveType > DRIVE_RAMDISK)
            DriveType = DRIVE_FIXED;

        switch (MyComputerSFHeader[iColumn].colnameid)
        {
            case IDS_SHV_COLUMN_NAME:
                hr = GetDisplayNameOf(pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
                break;
            case IDS_SHV_COLUMN_TYPE:
                if (DriveType == DRIVE_UNKNOWN)
                    hr = SHSetStrRetEmpty(&psd->str);
                else if (DriveType == DRIVE_REMOVABLE && IsFloppyDrive(szDrive))
                    hr = SHSetStrRet(&psd->str, IDS_DRIVE_FLOPPY);
                else
                    hr = SHSetStrRet(&psd->str, iDriveTypeIds[DriveType]);
                break;
            case IDS_SHV_COLUMN_DISK_CAPACITY:
            case IDS_SHV_COLUMN_DISK_AVAILABLE:
                psd->str.cStr[0] = 0x00;
                psd->str.uType = STRRET_CSTR;
                if (GetVolumeInformationW(szDrive, NULL, 0, NULL, NULL, NULL, NULL, 0))
                {
                    GetDiskFreeSpaceExW(szDrive, &ulFreeBytes, &ulTotalBytes, NULL);
                    if (iColumn == 2)
                        StrFormatByteSize64A(ulTotalBytes.QuadPart, psd->str.cStr, MAX_PATH);
                    else
                        StrFormatByteSize64A(ulFreeBytes.QuadPart, psd->str.cStr, MAX_PATH);
                }
                hr = S_OK;
                break;
            case IDS_SHV_COLUMN_COMMENTS:
                hr = SHSetStrRetEmpty(&psd->str); /* FIXME: comments */
                break;
            DEFAULT_UNREACHABLE;
        }
    }

    return hr;
}

HRESULT WINAPI CDrivesFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    switch (column < _countof(MyComputerSFHeader) ? MyComputerSFHeader[column].colnameid : ~0UL)
    {
        case IDS_SHV_COLUMN_NAME: return MakeSCID(*pscid, FMTID_Storage, PID_STG_NAME);
        case IDS_SHV_COLUMN_TYPE: return MakeSCID(*pscid, FMTID_Storage, PID_STG_STORAGETYPE);
        case IDS_SHV_COLUMN_COMMENTS: return MakeSCID(*pscid, FMTID_SummaryInformation, PIDSI_COMMENTS);
    }
    return E_INVALIDARG;
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

/**************************************************************************
 *    CDrivesFolder::MessageSFVCB (IShellFolderViewCB)
 */
STDMETHODIMP CDrivesFolder::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case SFVM_FSNOTIFY:
            if (lParam == SHCNE_DRIVEADD && wParam)
            {
                g_IsFloppyCache = 0;
                INT8 drive = GetDriveNumber(((PIDLIST_ABSOLUTE*)wParam)[0]);
                if (drive >= 0 && ((1UL << drive) & SHRestricted(REST_NODRIVES)))
                    return S_FALSE;
            }
            else if (lParam == SHCNE_DRIVEREMOVED)
            {
                g_IsFloppyCache = 0;
            }
            break;
    }
    return E_NOTIMPL;
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
