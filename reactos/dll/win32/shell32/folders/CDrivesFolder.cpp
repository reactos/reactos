/*
 *    Virtual Workplace folder
 *
 *    Copyright 1997                Marcus Meissner
 *    Copyright 1998, 1999, 2002    Juergen Schmied
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/*
CDrivesFolder should create a CRegFolder to represent the virtual items that exist only in
the registry. The CRegFolder is aggregated by the CDrivesFolder.
The CDrivesFolderEnum class should enumerate only drives on the system. Since the CRegFolder
implementation of IShellFolder::EnumObjects enumerates the virtual items, the
CDrivesFolderEnum is only responsible for returning the physical items.

2. At least on my XP system, the drive pidls returned are of type PT_DRIVE1, not PT_DRIVE
3. The parsing name returned for my computer is incorrect. It should be "My Computer"
*/

/***********************************************************************
*   IShellFolder implementation
*/

HRESULT CALLBACK DrivesContextMenuCallback(IShellFolder *psf,
                                           HWND         hwnd,
                                           IDataObject  *pdtobj,
                                           UINT         uMsg,
                                           WPARAM       wParam,
                                           LPARAM       lParam)
{
    if (uMsg != DFM_MERGECONTEXTMENU && uMsg != DFM_INVOKECOMMAND)
        return S_OK;

    PIDLIST_ABSOLUTE pidlFolder;
    PUITEMID_CHILD *apidl;
    UINT cidl;
    HRESULT hr = SH_GetApidlFromDataObject(pdtobj, &pidlFolder, &apidl, &cidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    char szDrive[8] = {0};
    if (!_ILGetDrive(apidl[0], szDrive, sizeof(szDrive)))
    {
        ERR("pidl is not a drive\n");
        SHFree(pidlFolder);
        _ILFreeaPidl(apidl, cidl);
        return E_FAIL;
    }

    if (uMsg == DFM_MERGECONTEXTMENU)
    {
        QCMINFO *pqcminfo = (QCMINFO *)lParam;
        DWORD dwFlags;

        if (GetVolumeInformationA(szDrive, NULL, 0, NULL, NULL, &dwFlags, NULL, 0))
        {
            /* Disable format if read only */
            if (!(dwFlags & FILE_READ_ONLY_VOLUME) && GetDriveTypeA(szDrive) != DRIVE_REMOTE)
            {
                _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, 0, MFT_SEPARATOR, NULL, 0);
                _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, pqcminfo->idCmdFirst++, MFT_STRING, MAKEINTRESOURCEW(IDS_FORMATDRIVE), MFS_ENABLED);
            }
        }
    }
    else if (uMsg == DFM_INVOKECOMMAND)
    {
        if (wParam == DFM_CMD_PROPERTIES)
        {
            WCHAR wszBuf[4];
            wcscpy(wszBuf, L"A:\\");
            wszBuf[0] = (WCHAR)szDrive[0];
            if (!SH_ShowDriveProperties(wszBuf, pidlFolder, apidl))
                hr = E_FAIL;
        }
        else
        {
            SHFormatDrive(hwnd, szDrive[0] - 'A', SHFMT_ID_DEFAULT, 0);
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

HRESULT CDrivesExtractIcon_CreateInstance(IShellFolder * psf, LPCITEMIDLIST pidl, REFIID riid, LPVOID * ppvOut)
{
    CComPtr<IDefaultExtractIconInit> initIcon;
    HRESULT hr = SHCreateDefaultExtractIcon(IID_PPV_ARG(IDefaultExtractIconInit, &initIcon));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CHAR* pszDrive = _ILGetDataPointer(pidl)->u.drive.szDriveName;
    WCHAR wTemp[MAX_PATH];
    int icon_idx = -1;

    if (pszDrive)
    {
        switch(GetDriveTypeA(pszDrive))
        {
            case DRIVE_REMOVABLE:
                icon_idx = IDI_SHELL_3_14_FLOPPY;
                break;
            case DRIVE_CDROM:
                icon_idx = IDI_SHELL_CDROM;
                break;
            case DRIVE_REMOTE:
                icon_idx = IDI_SHELL_NETDRIVE;
                break;
            case DRIVE_RAMDISK:
                icon_idx = IDI_SHELL_RAMDISK;
                break;
            case DRIVE_NO_ROOT_DIR:
                icon_idx = IDI_SHELL_CDROM;
                break;
        }
    }

    if (icon_idx != -1)
    {
        initIcon->SetNormalIcon(swShell32Name, -icon_idx);
    }
    else
    {
        if (HCR_GetIconW(L"Drive", wTemp, NULL, MAX_PATH, &icon_idx))
            initIcon->SetNormalIcon(wTemp, icon_idx);
        else
            initIcon->SetNormalIcon(swShell32Name, -IDI_SHELL_DRIVE);
    }

    return initIcon->QueryInterface(riid, ppvOut);
}

class CDrivesFolderEnum :
    public CEnumIDListBase
{
    public:
        CDrivesFolderEnum();
        ~CDrivesFolderEnum();
        HRESULT WINAPI Initialize(HWND hwndOwner, DWORD dwFlags);
        BOOL CreateMyCompEnumList(DWORD dwFlags);

        BEGIN_COM_MAP(CDrivesFolderEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

/***********************************************************************
*   IShellFolder [MyComputer] implementation
*/

static const shvheader MyComputerSFHeader[] = {
    {IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 15},
    {IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 10},
    {IDS_SHV_COLUMN6, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN7, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
};

#define MYCOMPUTERSHELLVIEWCOLUMNS 4

static const DWORD dwComputerAttributes =
    SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET |
    SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_CANLINK;
static const DWORD dwControlPanelAttributes =
    SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_CANLINK;
static const DWORD dwDriveAttributes =
    SFGAO_HASSUBFOLDER | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR |
    SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANRENAME | SFGAO_CANLINK;

CDrivesFolderEnum::CDrivesFolderEnum()
{
}

CDrivesFolderEnum::~CDrivesFolderEnum()
{
}

HRESULT WINAPI CDrivesFolderEnum::Initialize(HWND hwndOwner, DWORD dwFlags)
{
    if (CreateMyCompEnumList(dwFlags) == FALSE)
        return E_FAIL;

    return S_OK;
}

/**************************************************************************
 *  CDrivesFolderEnum::CreateMyCompEnumList()
 */

BOOL CDrivesFolderEnum::CreateMyCompEnumList(DWORD dwFlags)
{
    BOOL bRet = TRUE;
    static const WCHAR MyComputer_NameSpaceW[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\Namespace";

    TRACE("(%p)->(flags=0x%08x)\n", this, dwFlags);

    /* enumerate the folders */
    if (dwFlags & SHCONTF_FOLDERS)
    {
        WCHAR wszDriveName[] = {'A', ':', '\\', '\0'};
        DWORD dwDrivemap = GetLogicalDrives();
        HKEY hKey;
        UINT i;

        while (bRet && wszDriveName[0] <= 'Z')
        {
            if(dwDrivemap & 0x00000001L)
                bRet = AddToEnumList(_ILCreateDrive(wszDriveName));
            wszDriveName[0]++;
            dwDrivemap = dwDrivemap >> 1;
        }

        TRACE("-- (%p)-> enumerate (mycomputer shell extensions)\n", this);
        for (i = 0; i < 2; i++)
        {
            if (bRet && ERROR_SUCCESS == RegOpenKeyExW(i == 0 ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
                    MyComputer_NameSpaceW, 0, KEY_READ, &hKey))
            {
                WCHAR wszBuf[50];
                DWORD dwSize, j = 0;
                LONG ErrorCode;
                LPITEMIDLIST pidl;

                while (bRet)
                {
                    dwSize = sizeof(wszBuf) / sizeof(wszBuf[0]);
                    ErrorCode = RegEnumKeyExW(hKey, j, wszBuf, &dwSize, 0, NULL, NULL, NULL);
                    if (ERROR_SUCCESS == ErrorCode)
                    {
                        if (wszBuf[0] != L'{')
                        {
                            dwSize = sizeof(wszBuf);
                            RegGetValueW(hKey, wszBuf, NULL, RRF_RT_REG_SZ, NULL, wszBuf, &dwSize);
                        }

                        /* FIXME: shell extensions - the type should be PT_SHELLEXT (tested) */
                        pidl = _ILCreateGuidFromStrW(wszBuf);
                        if (pidl != NULL)
                            bRet = AddToEnumList(pidl);
                        else
                            ERR("Invalid MyComputer namespace extesion: %s\n", wszBuf);
                        j++;
                    }
                    else if (ERROR_NO_MORE_ITEMS == ErrorCode)
                        break;
                    else
                        bRet = FALSE;
                }
                RegCloseKey(hKey);
            }
        }
    }
    return bRet;
}

CDrivesFolder::CDrivesFolder()
{
    pidlRoot = NULL;
}

CDrivesFolder::~CDrivesFolder()
{
    TRACE ("-- destroying IShellFolder(%p)\n", this);
    SHFree(pidlRoot);
}

HRESULT WINAPI CDrivesFolder::FinalConstruct()
{
    pidlRoot = _ILCreateMyComputer();    /* my qualified pidl */
    if (pidlRoot == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = CRegFolder_CreateInstance(&CLSID_MyComputer, 
                                           pidlRoot, 
                                           L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", 
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
    LPCWSTR szNext = NULL;
    LPITEMIDLIST pidlTemp = NULL;

    TRACE("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n", this,
          hwndOwner, pbc, lpszDisplayName, debugstr_w (lpszDisplayName),
          pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
        *pchEaten = 0;        /* strange but like the original */

    /* handle CLSID paths */
    if (lpszDisplayName[0] == ':' && lpszDisplayName[1] == ':')
        return m_regFolder->ParseDisplayName(hwndOwner, pbc, lpszDisplayName, pchEaten, ppidl, pdwAttributes);

    if (PathGetDriveNumberW(lpszDisplayName) < 0)
        return E_INVALIDARG;

    pidlTemp = _ILCreateDrive(lpszDisplayName);
    if (!pidlTemp)
        return E_OUTOFMEMORY;

    if (lpszDisplayName[2] == L'\\')
    {
        szNext = &lpszDisplayName[3];
    }

    if (szNext && *szNext)
    {
        hr = SHELL32_ParseNextElement (this, hwndOwner, pbc, &pidlTemp,
                                       (LPOLESTR) szNext, pchEaten, pdwAttributes);
    }
    else
    {
        hr = S_OK;
        if (pdwAttributes && *pdwAttributes)
        {
            if (_ILIsDrive(pidlTemp))
                *pdwAttributes &= dwDriveAttributes;
            else if (_ILIsSpecialFolder(pidlTemp))
                m_regFolder->GetAttributesOf(1, &pidlTemp, pdwAttributes);
            else
                ERR("Got an unkown pidl here!\n");
        }
    }

    *ppidl = pidlTemp;

    TRACE ("(%p)->(-- ret=0x%08x)\n", this, hr);

    return hr;
}

/**************************************************************************
*        CDrivesFolder::EnumObjects
*/
HRESULT WINAPI CDrivesFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    return ShellObjectCreatorInit<CDrivesFolderEnum>(hwndOwner, dwFlags, IID_PPV_ARG(IEnumIDList, ppEnumIDList));
}

/**************************************************************************
*        CDrivesFolder::BindToObject
*/
HRESULT WINAPI CDrivesFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    TRACE("(%p)->(pidl=%p,%p,%s,%p)\n", this,
          pidl, pbcReserved, shdebugstr_guid(&riid), ppvOut);

    if (_ILIsSpecialFolder(pidl))
        return m_regFolder->BindToObject(pidl, pbcReserved, riid, ppvOut);

    LPITEMIDLIST pidlChild = ILCloneFirst (pidl);
    if (!pidlChild)
        return E_OUTOFMEMORY;

    CComPtr<IShellFolder> psf;
    HRESULT hr = SHELL32_CoCreateInitSF(pidlRoot, 
                                        NULL, 
                                        pidlChild, 
                                        &CLSID_ShellFSFolder, 
                                        -1, 
                                        IID_PPV_ARG(IShellFolder, &psf));

    ILFree(pidlChild);

    if (FAILED(hr))
        return hr;

    if (_ILIsPidlSimple (pidl))
    {
        return psf->QueryInterface(riid, ppvOut);
    }
    else
    {
        return psf->BindToObject(ILGetNext (pidl), pbcReserved, riid, ppvOut);
    }
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

    if (!_ILIsDrive(pidl1) || !_ILIsDrive(pidl2) || LOWORD(lParam) >= MYCOMPUTERSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    CHAR* pszDrive1 = _ILGetDataPointer(pidl1)->u.drive.szDriveName;
    CHAR* pszDrive2 = _ILGetDataPointer(pidl2)->u.drive.szDriveName;

    int result;
    switch(LOWORD(lParam)) 
    {
        case 0:        /* name */
        {
            result = stricmp(pszDrive1, pszDrive2);
            hres = MAKE_COMPARE_HRESULT(result);
            break;
        }
        case 1:        /* Type */
        {
            /* We want to return immediately because SHELL32_CompareDetails also compares children. */
            return SHELL32_CompareDetails(this, lParam, pidl1, pidl2);
        }
        case 2:       /* Size */
        case 3:       /* Size Available */
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
            if (lParam == 2) /* Size */
                Diff.QuadPart = Drive1Total.QuadPart - Drive2Total.QuadPart;
            else /* Size available */
                Diff.QuadPart = Drive1Available.QuadPart - Drive2Available.QuadPart;

            hres = MAKE_COMPARE_HRESULT(Diff.QuadPart);
            break;
        }
        default:
            return E_INVALIDARG;
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
        WARN("IContextMenu not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID(riid, IID_IShellView))
    {
        hr = CDefView_Constructor(this, riid, ppvOut);
    }
    TRACE ("-- (%p)->(interface=%p)\n", this, ppvOut);
    return hr;
}

static BOOL _ILIsControlPanel(LPCITEMIDLIST pidl)
{
    GUID *guid = _ILGetGUIDPointer(pidl);

    TRACE("(%p)\n", pidl);

    if (guid)
        return IsEqualIID(*guid, CLSID_ControlPanel);
    return FALSE;
}

/**************************************************************************
*  CDrivesFolder::GetAttributesOf
*/
HRESULT WINAPI CDrivesFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD * rgfInOut)
{
    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08x))\n",
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
                *rgfInOut &= dwDriveAttributes;
            else if (_ILIsControlPanel(apidl[i]))
                *rgfInOut &= dwControlPanelAttributes;
            else if (_ILIsSpecialFolder(*apidl))
                m_regFolder->GetAttributesOf(1, &apidl[i], rgfInOut);
            else
                ERR("Got unknown pidl type!\n");
        }
    }

    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE ("-- result=0x%08x\n", *rgfInOut);
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
        hr = IDataObject_Constructor (hwndOwner,
                                      pidlRoot, apidl, cidl, (IDataObject **)&pObj);
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
    TRACE ("(%p)->hr=0x%08x\n", this, hr);
    return hr;
}

/**************************************************************************
*    CDrivesFolder::GetDisplayNameOf
*/
HRESULT WINAPI CDrivesFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    LPWSTR pszPath;
    HRESULT hr = S_OK;

    TRACE ("(%p)->(pidl=%p,0x%08x,%p)\n", this, pidl, dwFlags, strRet);
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
        DWORD dwVolumeSerialNumber, dwMaximumComponentLength, dwFileSystemFlags;
        static const WCHAR wszOpenBracket[] = {' ', '(', 0};
        static const WCHAR wszCloseBracket[] = {')', 0};

        lstrcpynW(wszDrive, pszPath, 4);
        pszPath[0] = L'\0';
        GetVolumeInformationW(wszDrive, pszPath,
                                MAX_PATH - 7,
                                &dwVolumeSerialNumber,
                                &dwMaximumComponentLength, &dwFileSystemFlags, NULL, 0);
        pszPath[MAX_PATH-1] = L'\0';
        if (!wcslen(pszPath))
        {
            UINT DriveType, ResourceId;
            DriveType = GetDriveTypeW(wszDrive);
            switch(DriveType)
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
        wcscat (pszPath, wszOpenBracket);
        wszDrive[2] = L'\0';
        wcscat (pszPath, wszDrive);
        wcscat (pszPath, wszCloseBracket);
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
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDrivesFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDrivesFolder::GetDefaultColumn (DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    TRACE ("(%p)\n", this);

    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;
    return S_OK;
}

HRESULT WINAPI CDrivesFolder::GetDefaultColumnState(UINT iColumn, DWORD * pcsFlags)
{
    TRACE ("(%p)\n", this);

    if (!pcsFlags || iColumn >= MYCOMPUTERSHELLVIEWCOLUMNS)
        return E_INVALIDARG;
    *pcsFlags = MyComputerSFHeader[iColumn].pcsFlags;
    return S_OK;
}

HRESULT WINAPI CDrivesFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    FIXME ("(%p)\n", this);
    return E_NOTIMPL;
}

HRESULT WINAPI CDrivesFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    HRESULT hr;

    TRACE ("(%p)->(%p %i %p)\n", this, pidl, iColumn, psd);

    if (!psd || iColumn >= MYCOMPUTERSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    if (!pidl)
    {
        psd->fmt = MyComputerSFHeader[iColumn].fmt;
        psd->cxChar = MyComputerSFHeader[iColumn].cxChar;
        return SHSetStrRet(&psd->str, MyComputerSFHeader[iColumn].colnameid);
    }
    else if (_ILIsSpecialFolder(pidl))
    {
        return m_regFolder->GetDetailsOf(pidl, iColumn, psd);
    }
    else
    {
        char szPath[MAX_PATH];
        ULARGE_INTEGER ulBytes;

        psd->str.cStr[0] = 0x00;
        psd->str.uType = STRRET_CSTR;
        switch (iColumn)
        {
            case 0:        /* name */
                hr = GetDisplayNameOf(pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
                break;
            case 1:        /* type */
                _ILGetFileType(pidl, psd->str.cStr, MAX_PATH);
                break;
            case 2:        /* total size */
                _ILSimpleGetText (pidl, szPath, MAX_PATH);
                if (GetVolumeInformationA(szPath, NULL, 0, NULL, NULL, NULL, NULL, 0))
                {
                    GetDiskFreeSpaceExA(szPath, NULL, &ulBytes, NULL);
                    StrFormatByteSize64A(ulBytes.QuadPart, psd->str.cStr, MAX_PATH);
                }
                break;
            case 3:        /* free size */
                _ILSimpleGetText (pidl, szPath, MAX_PATH);
                if (GetVolumeInformationA(szPath, NULL, 0, NULL, NULL, NULL, NULL, 0))
                {
                    GetDiskFreeSpaceExA(szPath, &ulBytes, NULL, NULL);
                    StrFormatByteSize64A(ulBytes.QuadPart, psd->str.cStr, MAX_PATH);
                }
                break;
        }
        hr = S_OK;
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
    TRACE ("(%p)\n", this);

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
HRESULT WINAPI CDrivesFolder::Initialize(LPCITEMIDLIST pidl)
{
    return S_OK;
}

/**************************************************************************
 *    CDrivesFolder::GetCurFolder
 */
HRESULT WINAPI CDrivesFolder::GetCurFolder(LPITEMIDLIST *pidl)
{
    TRACE("(%p)->(%p)\n", this, pidl);

    if (!pidl)
        return E_INVALIDARG; /* xp doesn't have this check and crashes on NULL */

    *pidl = ILClone(pidlRoot);
    return S_OK;
}
