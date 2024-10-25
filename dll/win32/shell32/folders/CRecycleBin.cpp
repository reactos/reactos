/*
 * Trash virtual folder support. The trashing engine is implemented in trash.c
 *
 * Copyright (C) 2006 Mikolaj Zalewski
 * Copyright (C) 2009 Andrew Hill
 * Copyright (C) 2018 Russell Johnson
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

#include <mmsystem.h>
#include <ntquery.h>

WINE_DEFAULT_DEBUG_CHANNEL(CRecycleBin);

typedef struct
{
    int column_name_id;
    const GUID *fmtId;
    DWORD pid;
    int pcsFlags;
    int fmt;
    int cxChars;
} columninfo;

static const columninfo RecycleBinColumns[] =
{
    {IDS_SHV_COLUMN_NAME,     &FMTID_Storage,   PID_STG_NAME,        SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_LEFT,  25},
    {IDS_SHV_COLUMN_DELFROM,  &FMTID_Displaced, PID_DISPLACED_FROM,  SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_LEFT,  35},
    {IDS_SHV_COLUMN_DELDATE,  &FMTID_Displaced, PID_DISPLACED_DATE,  SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT,  15},
    {IDS_SHV_COLUMN_SIZE,     &FMTID_Storage,   PID_STG_SIZE,        SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN_TYPE,     &FMTID_Storage,   PID_STG_STORAGETYPE, SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT,  LVCFMT_LEFT,  15},
    {IDS_SHV_COLUMN_MODIFIED, &FMTID_Storage,   PID_STG_WRITETIME,   SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT,  15},
    /* {"creation time",  &FMTID_Storage,   PID_STG_CREATETIME, SHCOLSTATE_TYPE_DATE, LVCFMT_LEFT,  20}, */
    /* {"attribs",        &FMTID_Storage,   PID_STG_ATTRIBUTES, SHCOLSTATE_TYPE_STR,  LVCFMT_LEFT,  20}, */
};

#define COLUMN_NAME    0
#define COLUMN_DELFROM 1
#define COLUMN_DATEDEL 2
#define COLUMN_SIZE    3
#define COLUMN_TYPE    4
#define COLUMN_MTIME   5

#define COLUMNS_COUNT  6

// The ROS Recycle Bin PIDL format starts with a NT4/2000 Unicode FS PIDL followed by
// BBITEMDATA and BBITEMFOOTER. This makes it compatible with SHChangeNotify listeners.
#include "pshpack1.h"
#define BBITEMFILETYPE (PT_FS | PT_FS_UNICODE_FLAG | PT_FS_FILE_FLAG)
#define BBITEMFOLDERTYPE (PT_FS | PT_FS_UNICODE_FLAG | PT_FS_FOLDER_FLAG)
struct BBITEMDATA
{
    FILETIME DeletionTime;
#ifdef COLUMN_FATTS
    WORD AttribsHi; // Nobody needs this yet
#endif
    WORD RecycledPathOffset;
    WCHAR OriginalLocation[ANYSIZE_ARRAY];
    // ... @RecycledPathOffset WCHAR RecycledPath[ANYSIZE_ARRAY];
};
struct BBITEMFOOTER
{
    enum { ENDSIG = MAKEWORD('K', 'I') }; // "Killed item". MUST have the low bit set so _ILGetFileStructW returns NULL.
    WORD DataSize;
    WORD EndSignature;
};
#include "poppack.h"

static inline BOOL IsFolder(LPCITEMIDLIST pidl)
{
    return _ILGetFSType(pidl) & PT_FS_FOLDER_FLAG;
}

static BBITEMDATA* ValidateItem(LPCITEMIDLIST pidl)
{
    const UINT minstringsize = sizeof(L"X") + sizeof(""); // PT_FS strings
    const UINT minfs = sizeof(WORD) + FIELD_OFFSET(PIDLDATA, u.file.szNames) + minstringsize;
    const UINT mindatasize = FIELD_OFFSET(BBITEMDATA, OriginalLocation) + (sizeof(L"C:\\X") * 2);
    const UINT minsize = minfs + mindatasize + sizeof(BBITEMFOOTER);
    const BYTE type = _ILGetType(pidl);
    if ((type == BBITEMFILETYPE || type == BBITEMFOLDERTYPE) && pidl->mkid.cb >= minsize)
    {
        BBITEMFOOTER *pEnd = (BBITEMFOOTER*)((BYTE*)pidl + pidl->mkid.cb - sizeof(BBITEMFOOTER));
        if (pEnd->EndSignature == BBITEMFOOTER::ENDSIG && pEnd->DataSize >= mindatasize)
            return (BBITEMDATA*)((BYTE*)pEnd - pEnd->DataSize);
    }
    return NULL;
}

static LPITEMIDLIST CreateItem(LPCWSTR pszTrash, LPCWSTR pszOrig, const DELETED_FILE_INFO &Details)
{
    const BOOL folder = Details.Attributes & FILE_ATTRIBUTE_DIRECTORY;
    LPCWSTR pszName = PathFindFileNameW(pszTrash);
    SIZE_T ofsName = (SIZE_T)(pszName - pszTrash);
    SIZE_T cchName = wcslen(pszName) + 1, cbName = cchName * sizeof(WCHAR);
    SIZE_T cbFSNames = cbName + sizeof("") + 1; // Empty short name + 1 for WORD alignment
    SIZE_T cbFS = sizeof(WORD) + FIELD_OFFSET(PIDLDATA, u.file.szNames) + cbFSNames;
    SIZE_T cchTrash = ofsName + cchName, cbTrash = cchTrash * sizeof(WCHAR);
    SIZE_T cchOrig = wcslen(pszOrig) + 1, cbOrig = cchOrig * sizeof(WCHAR);
    SIZE_T cbData = FIELD_OFFSET(BBITEMDATA, OriginalLocation) + cbOrig + cbTrash;
    SIZE_T cb = cbFS + cbData + sizeof(BBITEMFOOTER);
    if (cb > 0xffff)
        return NULL;
    LPITEMIDLIST pidl = (LPITEMIDLIST)SHAlloc(cb + sizeof(WORD));
    if (!pidl)
        return pidl;

    pidl->mkid.cb = cb;
    pidl->mkid.abID[0] = folder ? BBITEMFOLDERTYPE : BBITEMFILETYPE;
    ILGetNext(pidl)->mkid.cb = 0; // Terminator
    FileStruct &fsitem = ((PIDLDATA*)pidl->mkid.abID)->u.file;
    fsitem.dummy = 0;
    C_ASSERT(sizeof(RECYCLEBINFILESIZETYPE) <= sizeof(fsitem.dwFileSize));
    fsitem.dwFileSize = Details.FileSize;
    fsitem.uFileAttribs = LOWORD(Details.Attributes);
    FileTimeToDosDateTime(&Details.LastModification, &fsitem.uFileDate, &fsitem.uFileTime);
    CopyMemory(fsitem.szNames, pszName, cbName);
    LPSTR pszShort = const_cast<LPSTR>(&fsitem.szNames[cbName]);
    pszShort[0] = '\0';
    pszShort[1] = '\0'; // Fill alignment padding (for ILIsEqual memcmp)

    BBITEMFOOTER *footer = (BBITEMFOOTER*)((BYTE*)pidl + cb - sizeof(BBITEMFOOTER));
    footer->DataSize = cbData;
    footer->EndSignature = BBITEMFOOTER::ENDSIG;

    BBITEMDATA *data = (BBITEMDATA*)((BYTE*)footer - footer->DataSize);
    data->DeletionTime = Details.DeletionTime;
#ifdef COLUMN_FATTS
    data->AttribsHi = HIWORD(Details.Attributes);
#endif
    data->RecycledPathOffset = FIELD_OFFSET(BBITEMDATA, OriginalLocation) + cbOrig;
    CopyMemory(data->OriginalLocation, pszOrig, cbOrig);
    CopyMemory((BYTE*)data + data->RecycledPathOffset, pszTrash, cbTrash);

    assert(!(((SIZE_T)&fsitem.szNames) & 1)); // WORD aligned please
    C_ASSERT(!(FIELD_OFFSET(BBITEMDATA, OriginalLocation) & 1)); // WORD aligned please
    assert(!(((SIZE_T)data) & 1)); // WORD aligned please
    assert(_ILGetFSType(pidl));
    assert(_ILIsPidlSimple(pidl));
    assert(*(WORD*)((BYTE*)pidl + pidl->mkid.cb - sizeof(WORD)) & 1); // ENDSIG bit
    assert(_ILGetFileStructW(pidl) == NULL); // Our custom footer is incompatible with WinXP pidl data
    assert(ValidateItem(pidl) == data);
    return pidl;
}

static inline UINT GetItemFileSize(LPCITEMIDLIST pidl)
{
    return _ILGetFSType(pidl) ? ((PIDLDATA*)pidl->mkid.abID)->u.file.dwFileSize : 0;
}

static inline LPCWSTR GetItemOriginalFullPath(const BBITEMDATA &Data)
{
    return Data.OriginalLocation;
}

static HRESULT GetItemOriginalFolder(const BBITEMDATA &Data, LPWSTR &Out)
{
    HRESULT hr = SHStrDupW(GetItemOriginalFullPath(Data), &Out);
    if (SUCCEEDED(hr))
        PathRemoveFileSpecW(Out);
    return hr;
}

static LPCWSTR GetItemOriginalFileName(const BBITEMDATA &Data)
{
    return PathFindFileNameW(GetItemOriginalFullPath(Data));
}

static inline LPCWSTR GetItemRecycledFullPath(const BBITEMDATA &Data)
{
    return (LPCWSTR)((BYTE*)&Data + Data.RecycledPathOffset);
}

static inline LPCWSTR GetItemRecycledFileName(LPCITEMIDLIST pidl, const BBITEMDATA &Data)
{
    C_ASSERT(BBITEMFILETYPE & PT_FS_UNICODE_FLAG);
    return (LPCWSTR)((LPPIDLDATA)pidl->mkid.abID)->u.file.szNames;
}

static int GetItemDriveNumber(LPCITEMIDLIST pidl)
{
    if (BBITEMDATA *pData = ValidateItem(pidl))
        return PathGetDriveNumberW(GetItemRecycledFullPath(*pData));
    WCHAR buf[MAX_PATH];
    return _ILSimpleGetTextW(pidl, buf, _countof(buf)) ? PathGetDriveNumberW(buf) : -1;
}

static HRESULT GetItemTypeName(PCUITEMID_CHILD pidl, const BBITEMDATA &Data, SHFILEINFOW &shfi)
{
    LPCWSTR path = GetItemRecycledFullPath(Data);
    UINT attribs = ((PIDLDATA*)pidl->mkid.abID)->u.file.uFileAttribs;
    if (SHGetFileInfoW(path, attribs, &shfi, sizeof(shfi), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES))
        return S_OK;
    shfi.szTypeName[0] = UNICODE_NULL;
    return E_FAIL;
}

static HDELFILE GetRecycleBinFileHandleFromItem(const BBITEMDATA &Data)
{
    RECYCLEBINFILEIDENTITY identity = { Data.DeletionTime, GetItemRecycledFullPath(Data) };
    return GetRecycleBinFileHandle(NULL, &identity);
}

/*
 * Recycle Bin folder
 */

static UINT GetDefaultRecycleDriveNumber()
{
    int drive = 0;
    WCHAR buf[MAX_PATH];
    if (GetWindowsDirectoryW(buf, _countof(buf)))
        drive = PathGetDriveNumberW(buf);
    return max(0, drive);
}

static inline LPCWSTR GetGlobalRecycleBinPath()
{
    return NULL;
}

static BOOL IsRecycleBinEmpty(IShellFolder *pSF)
{
    CComPtr<IEnumIDList> spEnumFiles;
    HRESULT hr = pSF->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &spEnumFiles);
    CComHeapPtr<ITEMIDLIST> spPidl;
    ULONG itemcount;
    return FAILED(hr) || spEnumFiles->Next(1, &spPidl, &itemcount) != S_OK;
}

static void CRecycleBin_ChangeNotifyBBItem(_In_ LONG Event, _In_opt_ LPCITEMIDLIST BBItem)
{
    LPITEMIDLIST pidlFolder = SHCloneSpecialIDList(NULL, CSIDL_BITBUCKET, FALSE);
    if (!pidlFolder)
        return;
    if (BBItem)
    {
        assert(ValidateItem(BBItem));
        if (LPITEMIDLIST pidlFull = ILCombine(pidlFolder, BBItem))
        {
            // Send notification for [Desktop][RecycleBin][BBItem]
            // FIXME: Windows monitors each RecycleBin FS folder on every drive
            //        instead of manually sending these?
            SHChangeNotify(Event, SHCNF_IDLIST, pidlFull, NULL);
            ILFree(pidlFull);
        }
    }
    else
    {
        SHChangeNotify(Event, SHCNF_IDLIST, pidlFolder, NULL);
    }
    ILFree(pidlFolder);
}

EXTERN_C void CRecycleBin_NotifyRecycled(LPCWSTR OrigPath, const WIN32_FIND_DATAW *pFind,
                                         const RECYCLEBINFILEIDENTITY *pFI)
{
    DELETED_FILE_INFO info;
    info.LastModification = pFind->ftLastWriteTime;
    info.DeletionTime = pFI->DeletionTime;
    info.FileSize = pFind->nFileSizeLow;
    info.Attributes = pFind->dwFileAttributes;
    if (LPITEMIDLIST pidl = CreateItem(pFI->RecycledFullPath, OrigPath, info))
    {
        CRecycleBin_ChangeNotifyBBItem(IsFolder(pidl) ? SHCNE_MKDIR : SHCNE_CREATE, pidl);
        ILFree(pidl);
    }
}

static void CRecycleBin_NotifyRemovedFromRecycleBin(LPCITEMIDLIST BBItem)
{
    CRecycleBin_ChangeNotifyBBItem(IsFolder(BBItem) ? SHCNE_RMDIR : SHCNE_DELETE, BBItem);

    CComHeapPtr<ITEMIDLIST> pidlBB(SHCloneSpecialIDList(NULL, CSIDL_BITBUCKET, FALSE));
    CComPtr<IShellFolder> pSF;
    if (pidlBB && SUCCEEDED(SHBindToObject(NULL, pidlBB, IID_PPV_ARG(IShellFolder, &pSF))))
    {
        if (IsRecycleBinEmpty(pSF))
            SHUpdateRecycleBinIcon();
    }
}

static HRESULT CRecyclerExtractIcon_CreateInstance(
    IShellFolder &FSFolder, LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr = FSFolder.GetUIObjectOf(NULL, 1, &pidl, riid, NULL, ppvOut);
    if (SUCCEEDED(hr))
        return hr;

    // In case the search fails we use a default icon
    ERR("Recycler could not retrieve the icon, this shouldn't happen\n");

    if (IsFolder(pidl))
        return SHELL_CreateFallbackExtractIconForFolder(riid, ppvOut);
    else
        return SHELL_CreateFallbackExtractIconForNoAssocFile(riid, ppvOut);
}

class CRecycleBinItemContextMenu :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu2
{
    private:
        LPITEMIDLIST                        apidl;
    public:
        CRecycleBinItemContextMenu();
        ~CRecycleBinItemContextMenu();
        HRESULT WINAPI Initialize(LPCITEMIDLIST pidl);

        // IContextMenu
        STDMETHOD(QueryContextMenu)(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
        STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpcmi) override;
        STDMETHOD(GetCommandString)(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen) override;

        // IContextMenu2
        STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

        BEGIN_COM_MAP(CRecycleBinItemContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        END_COM_MAP()
};

class CRecycleBinEnum :
    public CEnumIDListBase
{
    public:
        CRecycleBinEnum();
        ~CRecycleBinEnum();
        HRESULT WINAPI Initialize(DWORD dwFlags);
        BOOL CBEnumRecycleBin(IN HDELFILE hDeletedFile);
        static BOOL CALLBACK CBEnumRecycleBin(IN PVOID Context, IN HDELFILE hDeletedFile)
        {
            return static_cast<CRecycleBinEnum*>(Context)->CBEnumRecycleBin(hDeletedFile);
        }

        BEGIN_COM_MAP(CRecycleBinEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
        END_COM_MAP()
};

CRecycleBinEnum::CRecycleBinEnum()
{
}

CRecycleBinEnum::~CRecycleBinEnum()
{
}

HRESULT WINAPI CRecycleBinEnum::Initialize(DWORD dwFlags)
{
    LPCWSTR szDrive = GetGlobalRecycleBinPath();
    if (dwFlags & SHCONTF_NONFOLDERS)
    {
        TRACE("Starting Enumeration\n");

        if (!EnumerateRecycleBinW(szDrive, CBEnumRecycleBin, this))
        {
            WARN("Error: EnumerateCRecycleBinW failed\n");
            return E_FAIL;
        }
    }
    else
    {
        // do nothing
    }
    return S_OK;
}

BOOL CRecycleBinEnum::CBEnumRecycleBin(IN HDELFILE hDeletedFile)
{
    LPITEMIDLIST pidl = NULL;
    DELETED_FILE_INFO info;
    IRecycleBinFile *pRBF = IRecycleBinFileFromHDELFILE(hDeletedFile);
    BOOL ret = SUCCEEDED(pRBF->GetInfo(&info));
    if (ret)
    {
        pidl = CreateItem(info.RecycledFullPath.String, info.OriginalFullPath.String, info);
        ret = pidl != NULL;
        FreeRecycleBinString(&info.OriginalFullPath);
        FreeRecycleBinString(&info.RecycledFullPath);
    }
    if (pidl)
    {
        ret = AddToEnumList(pidl);
        if (!ret)
            ILFree(pidl);
    }
    CloseRecycleBinHandle(hDeletedFile);
    return ret;
}

/**************************************************************************
* IContextMenu2 Bitbucket Item Implementation
*/

CRecycleBinItemContextMenu::CRecycleBinItemContextMenu()
{
    apidl = NULL;
}

CRecycleBinItemContextMenu::~CRecycleBinItemContextMenu()
{
    ILFree(apidl);
}

HRESULT WINAPI CRecycleBinItemContextMenu::Initialize(LPCITEMIDLIST pidl)
{
    apidl = ILClone(pidl);
    if (apidl == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

enum { IDC_BB_RESTORE = 1, IDC_BB_CUT, IDC_BB_DELETE, IDC_BB_PROPERTIES };
static const CMVERBMAP g_BBItemVerbMap[] =
{
    { "undelete", IDC_BB_RESTORE },
    { "cut", IDC_BB_CUT },
    { "delete", IDC_BB_DELETE },
    { "properties", IDC_BB_PROPERTIES },
    { NULL }
};

HRESULT WINAPI CRecycleBinItemContextMenu::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    UINT idHigh = 0, id;

    TRACE("(%p)->(hmenu=%p indexmenu=%x cmdfirst=%x cmdlast=%x flags=%x )\n", this, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    id = idCmdFirst + IDC_BB_RESTORE;
    if (_InsertMenuItemW(hMenu, indexMenu, TRUE, id, MFT_STRING, MAKEINTRESOURCEW(IDS_RESTORE), 0))
    {
        idHigh = max(idHigh, id);
        indexMenu++;
    }
    id = idCmdFirst + IDC_BB_CUT;
    if (_InsertMenuItemW(hMenu, indexMenu, TRUE, id, MFT_STRING, MAKEINTRESOURCEW(IDS_CUT), MFS_DISABLED))
    {
        idHigh = max(idHigh, id);
        if (_InsertMenuItemW(hMenu, indexMenu++, TRUE, -1, MFT_SEPARATOR, NULL, 0))
            indexMenu++;
    }
    id = idCmdFirst + IDC_BB_DELETE;
    if (_InsertMenuItemW(hMenu, indexMenu, TRUE, id, MFT_STRING, MAKEINTRESOURCEW(IDS_DELETE), 0))
    {
        idHigh = max(idHigh, id);
        if (_InsertMenuItemW(hMenu, indexMenu++, TRUE, -1, MFT_SEPARATOR, NULL, 0))
            indexMenu++;
    }
    id = idCmdFirst + IDC_BB_PROPERTIES;
    if (_InsertMenuItemW(hMenu, indexMenu, TRUE, id, MFT_STRING, MAKEINTRESOURCEW(IDS_PROPERTIES), 0))
    {
        idHigh = max(idHigh, id);
        if (_InsertMenuItemW(hMenu, indexMenu++, TRUE, -1, MFT_SEPARATOR, NULL, 0))
            indexMenu++;
    }
    return idHigh ? MAKE_HRESULT(SEVERITY_SUCCESS, 0, idHigh - idCmdFirst + 1) : S_OK;
}

static BOOL ConfirmDelete(LPCMINVOKECOMMANDINFO lpcmi, UINT cidl, LPCITEMIDLIST pidl, const BBITEMDATA &Data)
{
    if (lpcmi->fMask & CMIC_MASK_FLAG_NO_UI)
    {
        return TRUE;
    }
    else if (cidl == 1)
    {
        const UINT ask = IsFolder(pidl) ? ASK_DELETE_FOLDER : ASK_DELETE_FILE;
        return SHELL_ConfirmYesNoW(lpcmi->hwnd, ask, GetItemOriginalFileName(Data));
    }
    WCHAR buf[MAX_PATH];
    wsprintfW(buf, L"%d", cidl);
    return SHELL_ConfirmYesNoW(lpcmi->hwnd, ASK_DELETE_MULTIPLE_ITEM, buf);
}

HRESULT WINAPI CRecycleBinItemContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    TRACE("(%p)->(invcom=%p verb=%p wnd=%p)\n", this, lpcmi, lpcmi->lpVerb, lpcmi->hwnd);

    int CmdId = SHELL_MapContextMenuVerbToCmdId(lpcmi, g_BBItemVerbMap);

    // Handle DefView accelerators
    if ((SIZE_T)lpcmi->lpVerb == FCIDM_SHVIEW_CUT)
        CmdId = IDC_BB_CUT;
    if ((SIZE_T)lpcmi->lpVerb == FCIDM_SHVIEW_DELETE)
        CmdId = IDC_BB_DELETE;
    if ((SIZE_T)lpcmi->lpVerb == FCIDM_SHVIEW_PROPERTIES)
        CmdId = IDC_BB_PROPERTIES;

    if (CmdId == IDC_BB_RESTORE || CmdId == IDC_BB_DELETE)
    {
        BBITEMDATA *pData = ValidateItem(apidl);
        if (!pData && FAILED_UNEXPECTEDLY(E_FAIL))
            return E_FAIL;
        HDELFILE hDelFile = GetRecycleBinFileHandleFromItem(*pData);
        if (!hDelFile && FAILED_UNEXPECTEDLY(E_FAIL))
            return E_FAIL;

        HRESULT hr = S_FALSE;
        if (CmdId == IDC_BB_RESTORE)
            hr = RestoreFileFromRecycleBin(hDelFile) ? S_OK : E_FAIL;
        else if (ConfirmDelete(lpcmi, 1, apidl, *pData))
            hr = DeleteFileInRecycleBin(hDelFile) ? S_OK : E_FAIL;

        if (hr == S_OK)
            CRecycleBin_NotifyRemovedFromRecycleBin(apidl);

        CloseRecycleBinHandle(hDelFile);
        return hr;
    }
    else if (CmdId == IDC_BB_CUT)
    {
        FIXME("implement cut\n");
        SHELL_ErrorBox(lpcmi->hwnd, ERROR_NOT_SUPPORTED);
        return E_NOTIMPL;
    }
    else if (CmdId == IDC_BB_PROPERTIES)
    {
        FIXME("implement properties\n");
        SHELL_ErrorBox(lpcmi->hwnd, ERROR_NOT_SUPPORTED);
        return E_NOTIMPL;
    }
    return E_UNEXPECTED;
}

HRESULT WINAPI CRecycleBinItemContextMenu::GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen)
{
    TRACE("(%p)->(idcom=%lx flags=%x %p name=%p len=%x)\n", this, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);
    return SHELL_GetCommandStringImpl(idCommand, uFlags, lpszName, uMaxNameLen, g_BBItemVerbMap);
}

HRESULT WINAPI CRecycleBinItemContextMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("CRecycleBin_IContextMenu2Item_HandleMenuMsg (%p)->(msg=%x wp=%lx lp=%lx)\n", this, uMsg, wParam, lParam);

    return E_NOTIMPL;
}

CRecycleBin::CRecycleBin()
{
    pidl = NULL;
    ZeroMemory(m_pFSFolders, sizeof(m_pFSFolders));
}

CRecycleBin::~CRecycleBin()
{
    SHFree(pidl);
    for (SIZE_T i = 0; i < _countof(m_pFSFolders); ++i)
    {
        if (m_pFSFolders[i])
            m_pFSFolders[i]->Release();
    }
}

IShellFolder* CRecycleBin::GetFSFolderForItem(LPCITEMIDLIST pidl)
{
    int drive = GetItemDriveNumber(pidl);
    if (drive < 0)
        drive = GetDefaultRecycleDriveNumber();
    if ((UINT)drive >= _countof(m_pFSFolders) && FAILED_UNEXPECTEDLY(E_FAIL))
        return NULL;

    if (!m_pFSFolders[drive])
    {
        HRESULT hr;
        PERSIST_FOLDER_TARGET_INFO pfti = {};
        if (FAILED_UNEXPECTEDLY(hr = GetRecycleBinPathFromDriveNumber(drive, pfti.szTargetParsingName)))
            return NULL;
        pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY;
        pfti.csidl = -1;
        CComHeapPtr<ITEMIDLIST> pidlRoot;
        pidlRoot.Attach(SHELL32_CreateSimpleIDListFromPath(pfti.szTargetParsingName, pfti.dwAttributes));
        if (!pidlRoot && FAILED_UNEXPECTEDLY(E_FAIL))
            return NULL;
        IShellFolder *psf;
        hr = SHELL32_CoCreateInitSF(pidlRoot, &pfti, NULL, &CLSID_ShellFSFolder, IID_PPV_ARG(IShellFolder, &psf));
        if (FAILED(hr))
            return NULL;
        m_pFSFolders[drive] = psf; // Reference count is 1 (for the m_pFSFolders cache)
    }
    m_pFSFolders[drive]->AddRef(); // AddRef for the caller
    return m_pFSFolders[drive];
}

/*************************************************************************
 * RecycleBin IPersistFolder2 interface
 */

HRESULT WINAPI CRecycleBin::GetClassID(CLSID *pClassID)
{
    TRACE("(%p, %p)\n", this, pClassID);
    if (pClassID == NULL)
        return E_INVALIDARG;
    *pClassID = GetClassID();
    return S_OK;
}

HRESULT WINAPI CRecycleBin::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    TRACE("(%p, %p)\n", this, pidl);

    SHFree((LPVOID)this->pidl);
    this->pidl = ILClone(pidl);
    if (this->pidl == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT WINAPI CRecycleBin::GetCurFolder(PIDLIST_ABSOLUTE *ppidl)
{
    TRACE("\n");
    return SHILClone((LPCITEMIDLIST)pidl, ppidl);
}

/*************************************************************************
 * RecycleBin IShellFolder2 interface
 */

HRESULT WINAPI CRecycleBin::ParseDisplayName(HWND hwnd, LPBC pbc,
        LPOLESTR pszDisplayName, ULONG *pchEaten, PIDLIST_RELATIVE *ppidl,
        ULONG *pdwAttributes)
{
    FIXME("stub\n");
    return E_NOTIMPL; // FIXME: Parse "D<Drive><UniqueId>.ext"
}

HRESULT WINAPI CRecycleBin::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    return ShellObjectCreatorInit<CRecycleBinEnum>(dwFlags, IID_PPV_ARG(IEnumIDList, ppEnumIDList));
}

HRESULT WINAPI CRecycleBin::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv)
{
    FIXME("(%p, %p, %p, %s, %p) - stub\n", this, pidl, pbc, debugstr_guid(&riid), ppv);
    return E_NOTIMPL;
}

HRESULT WINAPI CRecycleBin::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbc, REFIID riid, void **ppv)
{
    FIXME("(%p, %p, %p, %s, %p) - stub\n", this, pidl, pbc, debugstr_guid(&riid), ppv);
    return E_NOTIMPL;
}

static HRESULT CompareCanonical(const BBITEMDATA &Data1, const BBITEMDATA &Data2)
{
    // This assumes two files with the same original path cannot be deleted at
    // the same time (within the FAT/NTFS FILETIME resolution).
    int result = CompareFileTime(&Data1.DeletionTime, &Data2.DeletionTime);
    if (result == 0)
        result = _wcsicmp(GetItemOriginalFullPath(Data1), GetItemOriginalFullPath(Data2));
    return MAKE_COMPARE_HRESULT(result);
}

HRESULT WINAPI CRecycleBin::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    UINT column = UINT(lParam & SHCIDS_COLUMNMASK);
    if (column >= COLUMNS_COUNT || !_ILGetFSType(pidl1) || !_ILGetFSType(pidl2))
        return E_INVALIDARG;
    BBITEMDATA *pData1 = ValidateItem(pidl1), *pData2 = ValidateItem(pidl2);
    if ((!pData1 || !pData2) && column != COLUMN_NAME)
        return E_INVALIDARG;

    LPCWSTR pName1, pName2;
    FILETIME ft1, ft2;
    SHFILEINFOW shfi1, shfi2;
    int result;
    HRESULT hr = CFSFolder::CompareSortFoldersFirst(pidl1, pidl2);
    if (SUCCEEDED(hr))
        return hr;
    switch (column)
    {
        case COLUMN_NAME:
            if (pData1 && pData2)
            {
                if (lParam & SHCIDS_CANONICALONLY)
                    return CompareCanonical(*pData1, *pData2);
                pName1 = GetItemOriginalFileName(*pData1);
                pName2 = GetItemOriginalFileName(*pData2);
                result = CFSFolder::CompareUiStrings(pName1, pName2);
            }
            else
            {
                // We support comparing names even for non-Recycle items because
                // SHChangeNotify can broadcast regular FS items.
                if (IShellFolder *pSF = GetFSFolderForItem(pidl1))
                {
                    hr = pSF->CompareIDs(lParam, pidl1, pidl2);
                    pSF->Release();
                    return hr;
                }
                return E_INVALIDARG;
            }
            break;
        case COLUMN_DELFROM:
            if (SUCCEEDED(hr = GetItemOriginalFolder(*pData1, const_cast<LPWSTR&>(pName1))))
            {
                if (SUCCEEDED(hr = GetItemOriginalFolder(*pData2, const_cast<LPWSTR&>(pName2))))
                {
                    result = CFSFolder::CompareUiStrings(pName1, pName2);
                    SHFree(const_cast<LPWSTR>(pName2));
                }
                SHFree(const_cast<LPWSTR>(pName1));
            }
            return SUCCEEDED(hr) ? MAKE_COMPARE_HRESULT(result) : hr;
        case COLUMN_DATEDEL:
            result = CompareFileTime(&pData1->DeletionTime, &pData2->DeletionTime);
            break;
        case COLUMN_SIZE:
            result = GetItemFileSize(pidl1) - GetItemFileSize(pidl2);
            break;
        case COLUMN_TYPE:
            GetItemTypeName(pidl1, *pData1, shfi1);
            GetItemTypeName(pidl2, *pData2, shfi2);
            result = CFSFolder::CompareUiStrings(shfi1.szTypeName, shfi2.szTypeName);
            break;
        case COLUMN_MTIME:
            _ILGetFileDateTime(pidl1, &ft1);
            _ILGetFileDateTime(pidl2, &ft2);
            result = CompareFileTime(&ft1, &ft2);
            break;
    }
    return MAKE_COMPARE_HRESULT(result);
}

HRESULT WINAPI CRecycleBin::CreateViewObject(HWND hwndOwner, REFIID riid, void **ppv)
{
    CComPtr<IShellView> pShellView;
    HRESULT hr = E_NOINTERFACE;

    TRACE("(%p, %p, %s, %p)\n", this, hwndOwner, debugstr_guid(&riid), ppv);

    if (!ppv)
        return hr;
    *ppv = NULL;

    if (IsEqualIID (riid, IID_IDropTarget))
    {
        hr = CRecyclerDropTarget_CreateInstance(riid, ppv);
    }
    else if (IsEqualIID (riid, IID_IContextMenu) || IsEqualIID (riid, IID_IContextMenu2))
    {
        m_IsBackgroundMenu = true;
        hr = this->QueryInterface(riid, ppv);
    }
    else if (IsEqualIID (riid, IID_IShellView))
    {
        SFV_CREATE sfvparams = { sizeof(SFV_CREATE), this };
        hr = SHCreateShellFolderView(&sfvparams, (IShellView**)ppv);
    }
    else
        return hr;

    TRACE ("-- (%p)->(interface=%p)\n", this, ppv);
    return hr;

}

HRESULT WINAPI CRecycleBin::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
        SFGAOF *rgfInOut)
{
    TRACE("(%p, %d, {%p, ...}, {%x})\n", this, cidl, apidl ? apidl[0] : NULL, (unsigned int)*rgfInOut);
    HRESULT hr = S_OK;
    const SFGAOF ThisFolder = SFGAO_FOLDER | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET | SFGAO_CANRENAME | SFGAO_CANLINK;
    if (!cidl)
    {
        *rgfInOut &= ThisFolder;
        if (SHRestricted(REST_BITBUCKNOPROP))
            *rgfInOut &= ~SFGAO_HASPROPSHEET;
        return hr;
    }
    SFGAOF remain = SFGAO_LINK & *rgfInOut;
    *rgfInOut &= remain | SFGAO_HASPROPSHEET | SFGAO_CANDELETE | SFGAO_FILESYSTEM; // TODO: SFGAO_CANMOVE
    for (UINT i = 0; (*rgfInOut & remain) && i < cidl && SUCCEEDED(hr); ++i)
    {
        if (IShellFolder* pSF = GetFSFolderForItem(apidl[i]))
        {
            hr = pSF->GetAttributesOf(1, &apidl[i], rgfInOut);
            pSF->Release();
        }
    }
    return hr;
}

HRESULT WINAPI CRecycleBin::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid, UINT *prgfInOut, void **ppv)
{
    LPVOID pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p, %p %p)\n", this,
           hwndOwner, cidl, apidl, prgfInOut, ppv);

    if (!ppv)
        return hr;

    *ppv = NULL;
    assert(!cidl || (apidl && apidl[0]));

    if ((IsEqualIID (riid, IID_IContextMenu) || IsEqualIID(riid, IID_IContextMenu2)) && (cidl >= 1))
    {
        // FIXME: Handle multiple items
        hr = ShellObjectCreatorInit<CRecycleBinItemContextMenu>(apidl[0], riid, &pObj);
    }
    else if((IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)) && (cidl == 1))
    {
        if (IShellFolder *pSF = GetFSFolderForItem(apidl[0]))
        {
            hr = CRecyclerExtractIcon_CreateInstance(*pSF, apidl[0], riid, &pObj);
            pSF->Release();
        }
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppv = pObj;
    TRACE ("(%p)->hr=0x%08x\n", this, hr);
    return hr;
}

HRESULT WINAPI CRecycleBin::GetDisplayNameOf(PCUITEMID_CHILD pidl, SHGDNF uFlags, STRRET *pName)
{
    TRACE("(%p, %p, %x, %p)\n", this, pidl, (unsigned int)uFlags, pName);
    const BBITEMDATA *pData = ValidateItem(pidl);
    if (!pData)
        return E_INVALIDARG;

    if (IS_SHGDN_FOR_PARSING(uFlags))
    {
        LPCWSTR pszName = GetItemRecycledFullPath(*pData);
        if (uFlags & SHGDN_INFOLDER)
            pszName = PathFindFileNameW(pszName);
        pName->pOleStr = SHStrDupW(pszName);
    }
    else
    {
        if (uFlags & SHGDN_INFOLDER)
            pName->pOleStr = SHStrDupW(GetItemOriginalFileName(*pData));
        else
            pName->pOleStr = SHStrDupW(GetItemOriginalFullPath(*pData));
    }

    if (pName->pOleStr)
    {
        pName->uType = STRRET_WSTR;
        if (!IsFolder(pidl))
            SHELL_FS_ProcessDisplayFilename(pName->pOleStr, uFlags);
        return S_OK;
    }
    pName->uType = STRRET_CSTR;
    pName->cStr[0] = '\0';
    return E_OUTOFMEMORY;
}

HRESULT WINAPI CRecycleBin::SetNameOf(HWND hwnd, PCUITEMID_CHILD pidl, LPCOLESTR pszName,
                                      SHGDNF uFlags, PITEMID_CHILD *ppidlOut)
{
    TRACE("\n");
    return E_FAIL; /* not supported */
}

HRESULT WINAPI CRecycleBin::GetDefaultSearchGUID(GUID *pguid)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT WINAPI CRecycleBin::EnumSearches(IEnumExtraSearch **ppEnum)
{
    FIXME("stub\n");
    *ppEnum = NULL;
    return E_NOTIMPL;
}

HRESULT WINAPI CRecycleBin::GetDefaultColumn(DWORD dwReserved, ULONG *pSort, ULONG *pDisplay)
{
    TRACE("(%p, %x, %p, %p)\n", this, (unsigned int)dwReserved, pSort, pDisplay);
    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;
    return S_OK;
}

HRESULT WINAPI CRecycleBin::GetDefaultColumnState(UINT iColumn, SHCOLSTATEF *pcsFlags)
{
    TRACE("(%p, %d, %p)\n", this, iColumn, pcsFlags);
    if (iColumn >= COLUMNS_COUNT)
        return E_INVALIDARG;
    *pcsFlags = RecycleBinColumns[iColumn].pcsFlags;
    return S_OK;
}

HRESULT WINAPI CRecycleBin::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

HRESULT WINAPI CRecycleBin::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, LPSHELLDETAILS pDetails)
{
    HRESULT hr;
    FILETIME ft;
    SHFILEINFOW shfi;
    WCHAR buffer[MAX_PATH];

    TRACE("(%p, %p, %d, %p)\n", this, pidl, iColumn, pDetails);
    if (iColumn >= COLUMNS_COUNT)
        return E_FAIL;

    if (pidl == NULL)
    {
        pDetails->fmt = RecycleBinColumns[iColumn].fmt;
        pDetails->cxChar = RecycleBinColumns[iColumn].cxChars;
        return SHSetStrRet(&pDetails->str, RecycleBinColumns[iColumn].column_name_id);
    }

    if (iColumn == COLUMN_NAME)
        return GetDisplayNameOf(pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &pDetails->str);

    const BBITEMDATA *pData = ValidateItem(pidl);
    if (!pData && FAILED_UNEXPECTEDLY(E_INVALIDARG))
        return E_INVALIDARG;

    switch (iColumn)
    {
        case COLUMN_DATEDEL:
            CFSFolder::FormatDateTime(pData->DeletionTime, buffer, _countof(buffer));
            break;
        case COLUMN_DELFROM:
            if (SUCCEEDED(hr = GetItemOriginalFolder(*pData, pDetails->str.pOleStr)))
                pDetails->str.uType = STRRET_WSTR;
            return hr;
        case COLUMN_SIZE:
            *buffer = UNICODE_NULL;
            if (!IsFolder(pidl))
                CFSFolder::FormatSize(GetItemFileSize(pidl), buffer, _countof(buffer));
            break;
        case COLUMN_MTIME:
            _ILGetFileDateTime(pidl, &ft);
            CFSFolder::FormatDateTime(ft, buffer, _countof(buffer));
            break;
        case COLUMN_TYPE:
            GetItemTypeName(pidl, *pData, shfi);
            return SHSetStrRet(&pDetails->str, shfi.szTypeName);
        default:
            return E_FAIL;
    }
    return SHSetStrRet(&pDetails->str, buffer);
}

HRESULT WINAPI CRecycleBin::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    TRACE("(%p, %d, %p)\n", this, iColumn, pscid);
    if (iColumn >= COLUMNS_COUNT)
        return E_INVALIDARG;
    pscid->fmtid = *RecycleBinColumns[iColumn].fmtId;
    pscid->pid = RecycleBinColumns[iColumn].pid;
    return S_OK;
}

/*************************************************************************
 * RecycleBin IContextMenu interface
 */

enum { IDC_EMPTYRECYCLEBIN = 1, IDC_PROPERTIES };
static const CMVERBMAP g_BBFolderVerbMap[] =
{
    { "empty", IDC_EMPTYRECYCLEBIN },
    { "properties", IDC_PROPERTIES },
    { NULL }
};

HRESULT WINAPI CRecycleBin::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    TRACE("QueryContextMenu %p %p %u %u %u %u\n", this, hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags );

    if (!hMenu)
        return E_INVALIDARG;

    UINT idHigh = 0, id;

    WORD state = IsRecycleBinEmpty(this) ? MFS_DISABLED : MFS_ENABLED;
    id = idCmdFirst + IDC_EMPTYRECYCLEBIN;
    if (_InsertMenuItemW(hMenu, indexMenu, TRUE, id, MFT_STRING, MAKEINTRESOURCEW(IDS_EMPTY_BITBUCKET), state))
    {
        idHigh = max(idHigh, id);
        if (m_IsBackgroundMenu && !SHRestricted(REST_BITBUCKNOPROP))
        {
            id = idCmdFirst + IDC_PROPERTIES;
            if (_InsertMenuItemW(hMenu, ++indexMenu, TRUE, id, MFT_STRING, MAKEINTRESOURCEW(IDS_PROPERTIES), 0))
            {
                idHigh = max(idHigh, id);
                _InsertMenuItemW(hMenu, indexMenu++, TRUE, -1, MFT_SEPARATOR, NULL, 0);
            }
        }
    }
    return idHigh ? MAKE_HRESULT(SEVERITY_SUCCESS, 0, idHigh - idCmdFirst + 1) : S_OK;
}

HRESULT WINAPI CRecycleBin::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    TRACE("%p %p verb %p\n", this, lpcmi, lpcmi ? lpcmi->lpVerb : NULL);
    int CmdId = SHELL_MapContextMenuVerbToCmdId(lpcmi, g_BBFolderVerbMap);
    if (CmdId == IDC_EMPTYRECYCLEBIN)
    {
        HRESULT hr = SHEmptyRecycleBinW(lpcmi->hwnd, NULL, 0);
        TRACE("result %x\n", hr);
        if (hr != S_OK)
            return hr;
#if 0   // This is a nasty hack because lpcmi->hwnd might not be a shell browser.
        // Not required with working SHChangeNotify.
        CComPtr<IShellView> pSV;
        LPSHELLBROWSER lpSB = (LPSHELLBROWSER)SendMessage(lpcmi->hwnd, CWM_GETISHELLBROWSER, 0, 0);
        if (lpSB && SUCCEEDED(lpSB->QueryActiveShellView(&pSV)))
            pSV->Refresh();
#endif
        return hr;
    }
    else if (CmdId == IDC_PROPERTIES)
    {
        return SHELL_ShowItemIDListProperties((LPITEMIDLIST)CSIDL_BITBUCKET);
    }
    return E_INVALIDARG;
}

HRESULT WINAPI CRecycleBin::GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen)
{
    TRACE("%p %lu %u %p %p %u\n", this, idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);
    return SHELL_GetCommandStringImpl(idCommand, uFlags, lpszName, uMaxNameLen, g_BBFolderVerbMap);
}

/*************************************************************************
 * RecycleBin IShellPropSheetExt interface
 */

HRESULT WINAPI CRecycleBin::AddPages(LPFNSVADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    extern HRESULT RecycleBin_AddPropSheetPages(LPFNSVADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
    return RecycleBin_AddPropSheetPages(pfnAddPage, lParam);
}

HRESULT WINAPI CRecycleBin::ReplacePage(EXPPS uPageID, LPFNSVADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
    FIXME("%p %lu %p %lu\n", this, uPageID, pfnReplaceWith, lParam);

    return E_NOTIMPL;
}

/*************************************************************************
 * RecycleBin IShellExtInit interface
 */

HRESULT WINAPI CRecycleBin::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    TRACE("%p %p %p %p\n", this, pidlFolder, pdtobj, hkeyProgID );
    m_IsBackgroundMenu = false;
    return S_OK;
}

/**
 * Tests whether a file can be trashed
 * @param wszPath Path to the file to be trash
 * @returns TRUE if the file can be trashed, FALSE otherwise
 */
BOOL
TRASH_CanTrashFile(LPCWSTR wszPath)
{
    LONG ret;
    DWORD dwNukeOnDelete, dwType, VolSerialNumber, MaxComponentLength;
    DWORD FileSystemFlags, dwSize, dwDisposition;
    HKEY hKey;
    WCHAR szBuffer[10];
    WCHAR szKey[150] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\BitBucket\\Volume\\";

    if (wszPath[1] != L':')
    {
        /* path is UNC */
        return FALSE;
    }

    // Copy and retrieve the root path from get given string
    WCHAR wszRootPathName[MAX_PATH];
    StringCbCopyW(wszRootPathName, sizeof(wszRootPathName), wszPath);
    PathStripToRootW(wszRootPathName);

    // Test to see if the drive is fixed (non removable)
    if (GetDriveTypeW(wszRootPathName) != DRIVE_FIXED)
    {
        /* no bitbucket on removable media */
        return FALSE;
    }

    if (!GetVolumeInformationW(wszRootPathName, NULL, 0, &VolSerialNumber, &MaxComponentLength, &FileSystemFlags, NULL, 0))
    {
        ERR("GetVolumeInformationW failed with %u wszRootPathName=%s\n", GetLastError(), debugstr_w(wszRootPathName));
        return FALSE;
    }

    swprintf(szBuffer, L"%04X-%04X", LOWORD(VolSerialNumber), HIWORD(VolSerialNumber));
    wcscat(szKey, szBuffer);

    if (RegCreateKeyExW(HKEY_CURRENT_USER, szKey, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
    {
        ERR("RegCreateKeyExW failed\n");
        return FALSE;
    }

    if (dwDisposition  & REG_CREATED_NEW_KEY)
    {
        /* per default move to bitbucket */
        dwNukeOnDelete = 0;
        RegSetValueExW(hKey, L"NukeOnDelete", 0, REG_DWORD, (LPBYTE)&dwNukeOnDelete, sizeof(DWORD));
        /* per default unlimited size */
        dwSize = -1;
        RegSetValueExW(hKey, L"MaxCapacity", 0, REG_DWORD, (LPBYTE)&dwSize, sizeof(DWORD));
    }
    else
    {
        dwSize = sizeof(dwNukeOnDelete);
        ret = RegQueryValueExW(hKey, L"NukeOnDelete", NULL, &dwType, (LPBYTE)&dwNukeOnDelete, &dwSize);
        if (ret != ERROR_SUCCESS)
        {
            dwNukeOnDelete = 0;
            if (ret == ERROR_FILE_NOT_FOUND)
            {
                /* restore key and enable bitbucket */
                RegSetValueExW(hKey, L"NukeOnDelete", 0, REG_DWORD, (LPBYTE)&dwNukeOnDelete, sizeof(DWORD));
            }
        }
    }
    BOOL bCanTrash = !dwNukeOnDelete;
    // FIXME: Check if bitbucket is full (CORE-13743)
    RegCloseKey(hKey);
    return bCanTrash;
}

BOOL
TRASH_TrashFile(LPCWSTR wszPath)
{
    TRACE("(%s)\n", debugstr_w(wszPath));
    return DeleteFileToRecycleBin(wszPath);
}

static void TRASH_PlayEmptyRecycleBinSound()
{
    CRegKey regKey;
    CHeapPtr<WCHAR> pszValue;
    CHeapPtr<WCHAR> pszSndPath;
    DWORD dwType, dwSize;
    LONG lError;

    lError = regKey.Open(HKEY_CURRENT_USER,
                         L"AppEvents\\Schemes\\Apps\\Explorer\\EmptyRecycleBin\\.Current",
                         KEY_READ);
    if (lError != ERROR_SUCCESS)
        return;

    lError = regKey.QueryValue(NULL, &dwType, NULL, &dwSize);
    if (lError != ERROR_SUCCESS)
        return;

    if (!pszValue.AllocateBytes(dwSize))
        return;

    lError = regKey.QueryValue(NULL, &dwType, pszValue, &dwSize);
    if (lError != ERROR_SUCCESS)
        return;

    if (dwType == REG_EXPAND_SZ)
    {
        dwSize = ExpandEnvironmentStringsW(pszValue, NULL, 0);
        if (dwSize == 0)
            return;

        if (!pszSndPath.Allocate(dwSize))
            return;

        if (ExpandEnvironmentStringsW(pszValue, pszSndPath, dwSize) == 0)
            return;
    }
    else if (dwType == REG_SZ)
    {
        /* The type is REG_SZ, no need to expand */
        pszSndPath.Attach(pszValue.Detach());
    }
    else
    {
        /* Invalid type */
        return;
    }

    PlaySoundW(pszSndPath, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

/*************************************************************************
 * SHUpdateCRecycleBinIcon                                [SHELL32.@]
 *
 * Undocumented
 */
EXTERN_C HRESULT WINAPI SHUpdateRecycleBinIcon(void)
{
    FIXME("stub\n");

    // HACK! This dwItem2 should be the icon index in the system image list that has changed.
    // FIXME: Call SHMapPIDLToSystemImageListIndex
    DWORD dwItem2 = -1;

    SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_DWORD, NULL, &dwItem2);
    return S_OK;
}

/*************************************************************************
 *              SHEmptyRecycleBinA (SHELL32.@)
 */
HRESULT WINAPI SHEmptyRecycleBinA(HWND hwnd, LPCSTR pszRootPath, DWORD dwFlags)
{
    LPWSTR szRootPathW = NULL;
    int len;
    HRESULT hr;

    TRACE("%p, %s, 0x%08x\n", hwnd, debugstr_a(pszRootPath), dwFlags);

    if (pszRootPath)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pszRootPath, -1, NULL, 0);
        if (len == 0)
            return HRESULT_FROM_WIN32(GetLastError());
        szRootPathW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!szRootPathW)
            return E_OUTOFMEMORY;
        if (MultiByteToWideChar(CP_ACP, 0, pszRootPath, -1, szRootPathW, len) == 0)
        {
            HeapFree(GetProcessHeap(), 0, szRootPathW);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    hr = SHEmptyRecycleBinW(hwnd, szRootPathW, dwFlags);
    HeapFree(GetProcessHeap(), 0, szRootPathW);

    return hr;
}

HRESULT WINAPI SHEmptyRecycleBinW(HWND hwnd, LPCWSTR pszRootPath, DWORD dwFlags)
{
    WCHAR szBuffer[MAX_PATH];
    DWORD count;
    LONG ret;
    IShellFolder *pDesktop, *pRecycleBin;
    PIDLIST_ABSOLUTE pidlRecycleBin;
    PITEMID_CHILD pidl;
    HRESULT hr = S_OK;
    LPENUMIDLIST penumFiles;
    STRRET StrRet;

    TRACE("%p, %s, 0x%08x\n", hwnd, debugstr_w(pszRootPath), dwFlags);

    if (!(dwFlags & SHERB_NOCONFIRMATION))
    {
        hr = SHGetDesktopFolder(&pDesktop);
        if (FAILED(hr))
            return hr;
        hr = SHGetFolderLocation(NULL, CSIDL_BITBUCKET, NULL, 0, &pidlRecycleBin);
        if (FAILED(hr))
        {
            pDesktop->Release();
            return hr;
        }
        hr = pDesktop->BindToObject(pidlRecycleBin, NULL, IID_PPV_ARG(IShellFolder, &pRecycleBin));
        CoTaskMemFree(pidlRecycleBin);
        pDesktop->Release();
        if (FAILED(hr))
            return hr;
        hr = pRecycleBin->EnumObjects(hwnd, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &penumFiles);
        if (FAILED(hr))
        {
            pRecycleBin->Release();
            return hr;
        }

        count = 0;
        if (hr != S_FALSE)
        {
            while (penumFiles->Next(1, &pidl, NULL) == S_OK)
            {
                count++;
                pRecycleBin->GetDisplayNameOf(pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &StrRet);
                StrRetToBuf(&StrRet, pidl, szBuffer, _countof(szBuffer));
                CoTaskMemFree(pidl);
            }
            penumFiles->Release();
        }
        pRecycleBin->Release();

        switch (count)
        {
            case 0:
                /* no files, don't need confirmation */
                break;

            case 1:
                /* we have only one item inside the bin, so show a message box with its name */
                if (ShellMessageBoxW(shell32_hInstance, hwnd, MAKEINTRESOURCEW(IDS_DELETEITEM_TEXT), MAKEINTRESOURCEW(IDS_EMPTY_BITBUCKET),
                                   MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2, szBuffer) == IDNO)
                {
                    return S_OK;
                }
                break;

            default:
                /* we have more than one item, so show a message box with the count of the items */
                StringCbPrintfW(szBuffer, sizeof(szBuffer), L"%u", count);
                if (ShellMessageBoxW(shell32_hInstance, hwnd, MAKEINTRESOURCEW(IDS_DELETEMULTIPLE_TEXT), MAKEINTRESOURCEW(IDS_EMPTY_BITBUCKET),
                                   MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2, szBuffer) == IDNO)
                {
                    return S_OK;
                }
                break;
        }
    }

    if (dwFlags & SHERB_NOPROGRESSUI)
    {
        ret = EmptyRecycleBinW(pszRootPath);
    }
    else
    {
       /* FIXME
        * show a progress dialog
        */
        ret = EmptyRecycleBinW(pszRootPath);
    }

    if (!ret)
        return HRESULT_FROM_WIN32(GetLastError());

    CRecycleBin_ChangeNotifyBBItem(SHCNE_UPDATEDIR, NULL);
    if (!(dwFlags & SHERB_NOSOUND))
    {
        TRASH_PlayEmptyRecycleBinSound();
    }
    return S_OK;
}

HRESULT WINAPI SHQueryRecycleBinA(LPCSTR pszRootPath, LPSHQUERYRBINFO pSHQueryRBInfo)
{
    LPWSTR szRootPathW = NULL;
    int len;
    HRESULT hr;

    TRACE("%s, %p\n", debugstr_a(pszRootPath), pSHQueryRBInfo);

    if (pszRootPath)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pszRootPath, -1, NULL, 0);
        if (len == 0)
            return HRESULT_FROM_WIN32(GetLastError());
        szRootPathW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!szRootPathW)
            return E_OUTOFMEMORY;
        if (MultiByteToWideChar(CP_ACP, 0, pszRootPath, -1, szRootPathW, len) == 0)
        {
            HeapFree(GetProcessHeap(), 0, szRootPathW);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    hr = SHQueryRecycleBinW(szRootPathW, pSHQueryRBInfo);
    HeapFree(GetProcessHeap(), 0, szRootPathW);

    return hr;
}

HRESULT WINAPI SHQueryRecycleBinW(LPCWSTR pszRootPath, LPSHQUERYRBINFO pSHQueryRBInfo)
{
    TRACE("%s, %p\n", debugstr_w(pszRootPath), pSHQueryRBInfo);

    if (!pszRootPath || (pszRootPath[0] == 0) ||
        !pSHQueryRBInfo || (pSHQueryRBInfo->cbSize < sizeof(SHQUERYRBINFO)))
    {
        return E_INVALIDARG;
    }

    pSHQueryRBInfo->i64Size = 0;
    pSHQueryRBInfo->i64NumItems = 0;

    CComPtr<IRecycleBin> spRecycleBin;
    HRESULT hr;
    if (FAILED_UNEXPECTEDLY((hr = GetDefaultRecycleBin(pszRootPath, &spRecycleBin))))
        return hr;

    CComPtr<IRecycleBinEnumList> spEnumList;
    hr = spRecycleBin->EnumObjects(&spEnumList);
    if (!SUCCEEDED(hr))
        return hr;

    while (TRUE)
    {
        CComPtr<IRecycleBinFile> spFile;
        hr = spEnumList->Next(1, &spFile, NULL);
        if (hr == S_FALSE)
            return S_OK;

        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        ULARGE_INTEGER Size = {};
        if (FAILED_UNEXPECTEDLY((hr = spFile->GetFileSize(&Size))))
            return hr;

        pSHQueryRBInfo->i64Size += Size.QuadPart;
        pSHQueryRBInfo->i64NumItems++;
    }

    return S_OK;
}
