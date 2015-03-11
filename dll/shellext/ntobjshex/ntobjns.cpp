/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\shellext\ntobjshex\ntobjns.cpp
 * PURPOSE:     NT Object Namespace shell extension
 * PROGRAMMERS: David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"
#include "ntobjutil.h"
#include <ntquery.h>
#include "util.h"

#define SHCIDS_ALLFIELDS 0x80000000L
#define SHCIDS_CANONICALONLY 0x10000000L

#define GET_SHGDN_FOR(dwFlags)         ((DWORD)dwFlags & (DWORD)0x0000FF00)
#define GET_SHGDN_RELATION(dwFlags)    ((DWORD)dwFlags & (DWORD)0x000000FF)

WINE_DEFAULT_DEBUG_CHANNEL(ntobjshex);

// {845B0FB2-66E0-416B-8F91-314E23F7C12D}
const GUID CLSID_NtObjectFolder = { 0x845b0fb2, 0x66e0, 0x416b, { 0x8f, 0x91, 0x31, 0x4e, 0x23, 0xf7, 0xc1, 0x2d } };

// {F4C430C3-3A8D-4B56-A018-E598DA60C2E0}
static const GUID GUID_NtObjectColumns = { 0xf4c430c3, 0x3a8d, 0x4b56, { 0xa0, 0x18, 0xe5, 0x98, 0xda, 0x60, 0xc2, 0xe0 } };

enum NtObjectColumns
{
    NTOBJECT_COLUMN_NAME = 0,
    NTOBJECT_COLUMN_TYPE = 1,
    NTOBJECT_COLUMN_CREATEDATE = 2,
    NTOBJECT_COLUMN_LINKTARGET = 3,
};

class CNtObjectFolderContextMenu :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu
{
    PCIDLIST_ABSOLUTE m_pcidlFolder;
    PCITEMID_CHILD    m_pcidlChild;
    UINT              m_idFirst;

    enum ItemOffsets
    {
        ITEM_Open = 0,
        ITEM_OpenNewWindow
    };

public:
    CNtObjectFolderContextMenu() :
        m_pcidlFolder(NULL),
        m_pcidlChild(NULL),
        m_idFirst(0)
    {

    }

    virtual ~CNtObjectFolderContextMenu()
    {
        if (m_pcidlFolder)
            ILFree((LPITEMIDLIST) m_pcidlFolder);
        if (m_pcidlChild)
            ILFree((LPITEMIDLIST) m_pcidlChild);
    }

    HRESULT Initialize(PCIDLIST_ABSOLUTE parent, UINT cidl, PCUITEMID_CHILD_ARRAY apidl)
    {
        m_pcidlFolder = ILClone(parent);
        if (cidl != 1)
            return E_INVALIDARG;
        m_pcidlChild = ILClone(apidl[0]);
        return S_OK;
    }

    // IContextMenu
    virtual HRESULT WINAPI QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
    {
        MENUITEMINFOW mii;

        m_idFirst = idCmdFirst;

        const NtPidlEntry * entry = (NtPidlEntry *) m_pcidlChild;

        static WCHAR open [] = L"Open";
        static WCHAR opennewwindow [] = L"Open in new window";

        if ((entry->objectType == DIRECTORY_OBJECT) ||
            (entry->objectType == SYMBOLICLINK_OBJECT) ||
            (entry->objectType == KEY_OBJECT))
        {
            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_SUBMENU | MIIM_ID;
            mii.fType = MFT_STRING;
            mii.wID = (idCmdFirst = m_idFirst + ITEM_Open);
            mii.dwTypeData = open;
            mii.cch = _countof(open);
            mii.fState = MFS_ENABLED | MFS_DEFAULT;
            mii.hSubMenu = NULL;
            InsertMenuItemW(hmenu, idCmdFirst, TRUE, &mii);

            if (!(uFlags & CMF_DEFAULTONLY) && idCmdFirst <= idCmdLast)
            {
                ZeroMemory(&mii, sizeof(mii));
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_SUBMENU | MIIM_ID;
                mii.fType = MFT_STRING;
                mii.wID = (idCmdFirst = m_idFirst + ITEM_OpenNewWindow);
                mii.dwTypeData = opennewwindow;
                mii.cch = _countof(opennewwindow);
                mii.fState = MFS_ENABLED;
                mii.hSubMenu = NULL;
                InsertMenuItemW(hmenu, idCmdFirst, FALSE, &mii);
            }
        }

        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, idCmdFirst - m_idFirst);
    }

    virtual HRESULT WINAPI InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
    {
        LPITEMIDLIST fullPidl = ILCombine(m_pcidlFolder, m_pcidlChild);

        if (LOWORD(lpici->lpVerb) == (m_idFirst + ITEM_Open) || !lpici->lpVerb)
        {
            SHELLEXECUTEINFO sei = { 0 };
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME;
            sei.lpIDList = fullPidl;
            sei.lpClass = L"folder";
            sei.hwnd = lpici->hwnd;
            sei.nShow = lpici->nShow;
            sei.lpVerb = L"open";
            BOOL bRes = ::ShellExecuteEx(&sei);

            ILFree(fullPidl);

            return bRes ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        }
        else if (LOWORD(lpici->lpVerb) == (m_idFirst + ITEM_OpenNewWindow))
        {
            SHELLEXECUTEINFO sei = { 0 };
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_IDLIST | SEE_MASK_CLASSNAME;
            sei.lpIDList = fullPidl;
            sei.lpClass = L"folder";
            sei.hwnd = lpici->hwnd;
            sei.nShow = lpici->nShow;
            sei.lpVerb = L"opennewwindow";
            BOOL bRes = ::ShellExecuteEx(&sei);

            ILFree(fullPidl);

            return bRes ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        }
        return E_NOTIMPL;
    }

    virtual HRESULT WINAPI GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
    {
        if (idCmd == m_idFirst)
        {
            if (uType == GCS_VERBW)
            {
                return StringCchCopyW((LPWSTR) pszName, cchMax, L"open");
            }
        }
        else if (idCmd == (m_idFirst + 1))
        {
            if (uType == GCS_VERBW)
            {
                return StringCchCopyW((LPWSTR) pszName, cchMax, L"opennewwindow");
            }
        }
        return E_NOTIMPL;
    }

    DECLARE_NOT_AGGREGATABLE(CNtObjectFolderContextMenu)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CNtObjectFolderContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    END_COM_MAP()

};

class CNtObjectFolderExtractIcon :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IExtractIconW
{
    PCIDLIST_ABSOLUTE m_pcidlFolder;
    PCITEMID_CHILD    m_pcidlChild;

public:
    CNtObjectFolderExtractIcon() :
        m_pcidlFolder(NULL),
        m_pcidlChild(NULL)
    {

    }

    virtual ~CNtObjectFolderExtractIcon()
    {
        if (m_pcidlFolder)
            ILFree((LPITEMIDLIST) m_pcidlFolder);
        if (m_pcidlChild)
            ILFree((LPITEMIDLIST) m_pcidlChild);
    }

    HRESULT Initialize(PCIDLIST_ABSOLUTE parent, UINT cidl, PCUITEMID_CHILD_ARRAY apidl)
    {
        m_pcidlFolder = ILClone(parent);
        if (cidl != 1)
            return E_INVALIDARG;
        m_pcidlChild = ILClone(apidl[0]);
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetIconLocation(
        UINT uFlags,
        LPWSTR szIconFile,
        UINT cchMax,
        INT *piIndex,
        UINT *pwFlags)
    {
        const NtPidlEntry * entry = (NtPidlEntry *) m_pcidlChild;

        if ((entry->cb < sizeof(NtPidlEntry)) || (entry->magic != NT_OBJECT_PIDL_MAGIC))
            return E_INVALIDARG;

        UINT flags = 0;

#define GIL_CHECKSHIELD 0x0200
#define GIL_SHIELD 0x0200
        if (uFlags & GIL_CHECKSHIELD && !(entry->objectInformation.GrantedAccess & STANDARD_RIGHTS_READ))
            flags |= GIL_SHIELD;

        switch (entry->objectType)
        {
        case DIRECTORY_OBJECT:
        case SYMBOLICLINK_OBJECT:
            GetModuleFileNameW(g_hInstance, szIconFile, cchMax);
            *piIndex = -((uFlags & GIL_OPENICON) ? IDI_NTOBJECTDIROPEN : IDI_NTOBJECTDIR);
            *pwFlags = flags;
            return S_OK;
        case DEVICE_OBJECT:
            GetModuleFileNameW(g_hInstance, szIconFile, cchMax);
            *piIndex = -IDI_NTOBJECTDEVICE;
            *pwFlags = flags;
            return S_OK;
        case PORT_OBJECT:
            GetModuleFileNameW(g_hInstance, szIconFile, cchMax);
            *piIndex = -IDI_NTOBJECTPORT;
            *pwFlags = flags;
            return S_OK;
        default:
            GetModuleFileNameW(g_hInstance, szIconFile, cchMax);
            *piIndex = -IDI_NTOBJECTITEM;
            *pwFlags = flags;
            return S_OK;
        }
    }

    virtual HRESULT STDMETHODCALLTYPE Extract(
        LPCWSTR pszFile,
        UINT nIconIndex,
        HICON *phiconLarge,
        HICON *phiconSmall,
        UINT nIconSize)
    {
        return SHDefExtractIconW(pszFile, nIconIndex, 0, phiconLarge, phiconSmall, nIconSize);
    }

    DECLARE_NOT_AGGREGATABLE(CNtObjectFolderExtractIcon)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CNtObjectFolderExtractIcon)
        COM_INTERFACE_ENTRY_IID(IID_IExtractIconW, IExtractIconW)
    END_COM_MAP()

};

class CNtObjectPidlManager
{
private:
    PWSTR m_ntPath;

    HDPA m_hDpa;
    UINT m_hDpaCount;

    int  DpaDeleteCallback(NtPidlEntry * info)
    {
        CoTaskMemFree(info);
        return 0;
    }

    static int CALLBACK s_DpaDeleteCallback(void *pItem, void *pData)
    {
        CNtObjectPidlManager * mf = (CNtObjectPidlManager*) pData;
        NtPidlEntry  * item = (NtPidlEntry*) pItem;
        return mf->DpaDeleteCallback(item);
    }

public:
    CNtObjectPidlManager() :
        m_ntPath(NULL),
        m_hDpa(NULL),
        m_hDpaCount(0)
    {
    }

    ~CNtObjectPidlManager()
    {
        DPA_DestroyCallback(m_hDpa, s_DpaDeleteCallback, this);
    }

    HRESULT Initialize(PWSTR ntPath)
    {
        m_ntPath = ntPath;

        m_hDpa = DPA_Create(10);

        if (!m_hDpa)
            return E_OUTOFMEMORY;

        HRESULT hr = EnumerateNtDirectory(m_hDpa, m_ntPath, &m_hDpaCount);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return S_OK;
    }

    HRESULT FindPidlInList(PCUITEMID_CHILD pcidl, NtPidlEntry ** pinfo)
    {
        HRESULT hr;

        if (!m_hDpa)
        {
            return E_FAIL;
        }

        NtPidlEntry * info = (NtPidlEntry *) pcidl;
        if ((info->cb < sizeof(NtPidlEntry)) || (info->magic != NT_OBJECT_PIDL_MAGIC))
        {
            ERR("FindPidlInList: Requested pidl is not of the correct type.\n");
            return E_INVALIDARG;
        }

        TRACE("Searching for pidl { cb=%d } in a list of %d items\n", pcidl->mkid.cb, m_hDpaCount);

        for (UINT i = 0; i < m_hDpaCount; i++)
        {
            NtPidlEntry * pInfo = (NtPidlEntry *) DPA_GetPtr(m_hDpa, i);
            ASSERT(pInfo);

            hr = CompareIDs(0, pInfo, pcidl);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            if (hr == S_OK)
            {
                *pinfo = pInfo;
                return S_OK;
            }
            else
            {
                TRACE("Comparison returned %d\n", (int) (short) (hr & 0xFFFF));
            }
        }

        ERR("PIDL NOT FOUND: Requested filename: %S\n", info->entryName);
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    HRESULT FindByName(LPCWSTR strParsingName, NtPidlEntry ** pinfo)
    {
        if (!m_hDpa)
        {
            return E_FAIL;
        }

        TRACE("Searching for '%S' in a list of %d items\n", strParsingName, m_hDpaCount);

        for (int i = 0; i < (int) m_hDpaCount; i++)
        {
            NtPidlEntry * pInfo = (NtPidlEntry *) DPA_GetPtr(m_hDpa, i);
            ASSERT(pInfo);

            int order = CompareStringW(GetThreadLocale(), NORM_IGNORECASE,
                pInfo->entryName, wcslen(pInfo->entryName),
                strParsingName, wcslen(strParsingName));

            if (order == CSTR_EQUAL)
            {
                *pinfo = pInfo;
                return S_OK;
            }
        }

        TRACE("Pidl not found\n");
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    HRESULT GetPidl(UINT index, NtPidlEntry ** pEntry)
    {
        *pEntry = NULL;

        NtPidlEntry * entry = (NtPidlEntry *) DPA_GetPtr(m_hDpa, index);
        if (!entry)
        {
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }

        *pEntry = entry;
        return S_OK;
    }

    HRESULT GetCount(UINT * count)
    {
        *count = m_hDpaCount;
        return S_OK;
    }


    static LPITEMIDLIST CreatePidlFromItem(NtPidlEntry * entry)
    {
        LPITEMIDLIST idl = (LPITEMIDLIST) CoTaskMemAlloc(entry->cb + 2);
        if (!idl)
            return NULL;
        memset(idl, 0, entry->cb + 2);
        memcpy(idl, entry, entry->cb);
        return idl;
    }

    HRESULT CompareIDs(LPARAM lParam, NtPidlEntry * first, NtPidlEntry * second)
    {
        if (LOWORD(lParam) != 0)
            return E_INVALIDARG;

        if (second->cb > first->cb)
            return MAKE_HRESULT(0, 0, (USHORT) 1);
        if (second->cb < first->cb)
            return MAKE_HRESULT(0, 0, (USHORT) -1);

        if (second->entryNameLength > first->entryNameLength)
            return MAKE_HRESULT(0, 0, (USHORT) 1);
        if (second->entryNameLength < first->entryNameLength)
            return MAKE_HRESULT(0, 0, (USHORT) -1);

        if (HIWORD(lParam) == SHCIDS_ALLFIELDS)
        {
            int ord = memcmp(second, first, first->cb);

            if (ord != 0)
                return MAKE_HRESULT(0, 0, (USHORT) ord);
        }
        else if (HIWORD(lParam) == SHCIDS_CANONICALONLY)
        {
            int ord = StrCmpNW(second->entryName, first->entryName, first->entryNameLength);

            if (ord != 0)
                return MAKE_HRESULT(0, 0, (USHORT) ord);
        }
        else
        {
            int ord = (int) second->objectType - (int) first->objectType;

            if (ord > 0)
                return MAKE_HRESULT(0, 0, (USHORT) 1);
            if (ord < 0)
                return MAKE_HRESULT(0, 0, (USHORT) -1);

            ord = StrCmpNW(second->entryName, first->entryName, first->entryNameLength / sizeof(WCHAR));

            if (ord != 0)
                return MAKE_HRESULT(0, 0, (USHORT) ord);
        }

        return S_OK;
    }

    HRESULT CompareIDs(LPARAM lParam, NtPidlEntry * first, LPCITEMIDLIST pcidl)
    {
        LPCITEMIDLIST p = pcidl;
        NtPidlEntry * second = (NtPidlEntry*) &(p->mkid);
        if ((second->cb < sizeof(NtPidlEntry)) || (second->magic != NT_OBJECT_PIDL_MAGIC))
            return E_INVALIDARG;

        return CompareIDs(lParam, first, second);
    }

    HRESULT CompareIDs(LPARAM lParam, LPCITEMIDLIST pcidl1, LPCITEMIDLIST pcidl2)
    {
        LPCITEMIDLIST p = pcidl1;
        NtPidlEntry * first = (NtPidlEntry*) &(p->mkid);
        if ((first->cb < sizeof(NtPidlEntry)) || (first->magic != NT_OBJECT_PIDL_MAGIC))
            return E_INVALIDARG;

        return CompareIDs(lParam, first, pcidl2);
    }

    ULONG ConvertAttributes(NtPidlEntry * entry, PULONG inMask)
    {
        ULONG mask = inMask ? *inMask : 0xFFFFFFFF;
        ULONG flags = SFGAO_HASPROPSHEET | SFGAO_CANLINK;

        if (entry->objectType == DIRECTORY_OBJECT)
            flags |= SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE;

        if (entry->objectType == SYMBOLICLINK_OBJECT)
            flags |= SFGAO_LINK | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE;

        if (entry->objectType == KEY_OBJECT)
            flags |= SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE;

        return flags & mask;
    }

};

class CNtObjectFolderEnum :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{
private:
    CComPtr<CNtObjectFolder> m_Folder;

    HWND m_HwndOwner;
    SHCONTF m_Flags;

    UINT m_Index;
    UINT m_Count;

public:
    CNtObjectFolderEnum() :
        m_HwndOwner(NULL),
        m_Flags(0),
        m_Index(0),
        m_Count(0)
    {
    }

    virtual ~CNtObjectFolderEnum()
    {
    }

    HRESULT Initialize(CNtObjectFolder * folder, HWND hwndOwner, SHCONTF flags)
    {
        m_Folder = folder;

        m_Folder->GetManager().GetCount(&m_Count);

        m_HwndOwner = hwndOwner;
        m_Flags = flags;

        return Reset();
    }

    virtual HRESULT STDMETHODCALLTYPE Next(
        ULONG celt,
        LPITEMIDLIST *rgelt,
        ULONG *pceltFetched)
    {
        if (pceltFetched)
            *pceltFetched = 0;

        if (m_Index >= m_Count)
            return S_FALSE;

        for (int i = 0; i < (int) celt;)
        {
            NtPidlEntry * tinfo;
            BOOL flagsOk = FALSE;

            do {
                HRESULT hr = m_Folder->GetManager().GetPidl(m_Index++, &tinfo);
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;

                switch (tinfo->objectType)
                {
                case SYMBOLICLINK_OBJECT:
                case DIRECTORY_OBJECT:
                case KEY_OBJECT:
                    flagsOk = (m_Flags & SHCONTF_FOLDERS) != 0;
                    break;
                default:
                    flagsOk = (m_Flags & SHCONTF_NONFOLDERS) != 0;
                    break;
                }
            } while (m_Index < m_Count && !flagsOk);

            if (flagsOk)
            {
                if (rgelt)
                    rgelt[i] = m_Folder->GetManager().CreatePidlFromItem(tinfo);
                i++;
            }

            if (m_Index == m_Count)
            {
                if (pceltFetched)
                    *pceltFetched = i;
                return (i == (int) celt) ? S_OK : S_FALSE;
            }
        }

        if (pceltFetched) *pceltFetched = celt;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt)
    {
        return Next(celt, NULL, NULL);
    }

    virtual HRESULT STDMETHODCALLTYPE Reset()
    {
        m_Index = 0;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumIDList **ppenum)
    {
        return ShellObjectCreatorInit<CNtObjectFolderEnum>(m_Folder, m_HwndOwner, m_Flags, IID_PPV_ARG(IEnumIDList, ppenum));
    }

    DECLARE_NOT_AGGREGATABLE(CNtObjectFolderEnum)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CNtObjectFolderEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
    END_COM_MAP()

};

//-----------------------------------------------------------------------------
// CNtObjectFolder

CNtObjectFolder::CNtObjectFolder() :
    m_PidlManager(NULL),
    m_shellPidl(NULL)
{
}

CNtObjectFolder::~CNtObjectFolder()
{
    TRACE("Destroying CNtObjectFolder %p\n", this);
}

// IShellFolder
HRESULT STDMETHODCALLTYPE CNtObjectFolder::ParseDisplayName(
    HWND hwndOwner,
    LPBC pbcReserved,
    LPOLESTR lpszDisplayName,
    ULONG *pchEaten,
    LPITEMIDLIST *ppidl,
    ULONG *pdwAttributes)
{
    HRESULT hr;
    NtPidlEntry * info;

    if (!ppidl)
        return E_POINTER;

    if (pchEaten)
        *pchEaten = 0;

    if (pdwAttributes)
        *pdwAttributes = 0;

    TRACE("CNtObjectFolder::ParseDisplayName name=%S (ntPath=%S)\n", lpszDisplayName, m_NtPath);

    hr = m_PidlManager->FindByName(lpszDisplayName, &info);
    if (FAILED(hr))
    {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    *ppidl = m_PidlManager->CreatePidlFromItem(info);

    if (pchEaten)
        *pchEaten = wcslen(info->entryName);

    if (pdwAttributes)
        *pdwAttributes = m_PidlManager->ConvertAttributes(info, pdwAttributes);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::EnumObjects(
    HWND hwndOwner,
    SHCONTF grfFlags,
    IEnumIDList **ppenumIDList)
{
    return ShellObjectCreatorInit<CNtObjectFolderEnum>(this, hwndOwner, grfFlags, IID_PPV_ARG(IEnumIDList, ppenumIDList));
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::BindToObject(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    void **ppvOut)
{
    NtPidlEntry * info;
    HRESULT hr;

    if (IsEqualIID(riid, IID_IShellFolder))
    {
        hr = m_PidlManager->FindPidlInList(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        if (!(info->objectInformation.GrantedAccess & (STANDARD_RIGHTS_READ | FILE_LIST_DIRECTORY)))
            return E_ACCESSDENIED;

        WCHAR path[MAX_PATH];
        StringCbCopyW(path, _countof(path), m_NtPath);
        PathAppendW(path, info->entryName);

        LPITEMIDLIST first = ILCloneFirst(pidl);
        LPCITEMIDLIST rest = ILGetNext(pidl);

        LPITEMIDLIST fullPidl = ILCombine(m_shellPidl, first);

        if (info->objectType == SYMBOLICLINK_OBJECT)
        {
            NtPidlSymlinkData * symlink = (NtPidlSymlinkData*) (((PBYTE) info) + FIELD_OFFSET(NtPidlEntry, entryName) + info->entryNameLength + sizeof(WCHAR));

            if (symlink->targetNameLength > 0)
            {
                if (symlink->targetName[1] == L':' && isalphaW(symlink->targetName[0]))
                {
                    ERR("TODO: Navigating to WIN32 PATH from NT PATH.\n");
                    return E_NOTIMPL;
                }

                StringCbCopyW(path, _countof(path), L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{845B0FB2-66E0-416B-8F91-314E23F7C12D}");
                PathAppend(path, symlink->targetName);

                CComPtr<IShellFolder> psfDesktop;
                hr = SHGetDesktopFolder(&psfDesktop);
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;

                hr = psfDesktop->ParseDisplayName(NULL, NULL, path, NULL, &first, NULL);
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;
            }
            else
            {
                return E_UNEXPECTED;
            }
        }

        CComPtr<IShellFolder> psfChild;

        if (info->objectType == KEY_OBJECT)
        {
            hr = ShellObjectCreatorInit<CRegistryFolder>(fullPidl, path, IID_PPV_ARG(IShellFolder, &psfChild));
        }
        else
        {
            hr = ShellObjectCreatorInit<CNtObjectFolder>(fullPidl, path, IID_PPV_ARG(IShellFolder, &psfChild));
        }

        ILFree(fullPidl);
        ILFree(first);

        if (rest->mkid.cb > 0)
        {
            return psfChild->BindToObject(rest, pbcReserved, riid, ppvOut);
        }

        return psfChild->QueryInterface(riid, ppvOut);
    }

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::BindToStorage(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    void **ppvObj)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::CompareIDs(
    LPARAM lParam,
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2)
{
    TRACE("CompareIDs\n");
    return m_PidlManager->CompareIDs(lParam, pidl1, pidl2);
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::CreateViewObject(
    HWND hwndOwner,
    REFIID riid,
    void **ppvOut)
{
    if (!IsEqualIID(riid, IID_IShellView))
        return E_NOINTERFACE;

    SFV_CREATE sfv;
    sfv.cbSize = sizeof(sfv);
    sfv.pshf = this;
    sfv.psvOuter = NULL;
    sfv.psfvcb = this;

    return SHCreateShellFolderView(&sfv, (IShellView**) ppvOut);
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetAttributesOf(
    UINT cidl,
    PCUITEMID_CHILD_ARRAY apidl,
    SFGAOF *rgfInOut)
{
    NtPidlEntry * info;
    HRESULT hr;

    TRACE("GetAttributesOf\n");

    if (cidl == 0)
    {
        *rgfInOut &= SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE;
        return S_OK;
    }

    for (int i = 0; i < (int) cidl; i++)
    {
        PCUITEMID_CHILD pidl = apidl[i];

        hr = m_PidlManager->FindPidlInList(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        // Update attributes.
        *rgfInOut = m_PidlManager->ConvertAttributes(info, rgfInOut);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetUIObjectOf(
    HWND hwndOwner,
    UINT cidl,
    PCUITEMID_CHILD_ARRAY apidl,
    REFIID riid,
    UINT *prgfInOut,
    void **ppvOut)
{
    TRACE("GetUIObjectOf\n");

    if (IsEqualIID(riid, IID_IContextMenu))
    {
        return ShellObjectCreatorInit<CNtObjectFolderContextMenu>(m_shellPidl, cidl, apidl, riid, ppvOut);
        //return CDefFolderMenu_Create2(m_shellPidl, hwndOwner, cidl, apidl, this, ContextMenuCallback, 0, NULL, (IContextMenu**) ppvOut);
    }

    if (IsEqualIID(riid, IID_IExtractIconW))
    {
        return ShellObjectCreatorInit<CNtObjectFolderExtractIcon>(m_shellPidl, cidl, apidl, riid, ppvOut);
    }

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetDisplayNameOf(
    LPCITEMIDLIST pidl,
    SHGDNF uFlags,
    STRRET *lpName)
{
    NtPidlEntry * info;
    HRESULT hr;

    TRACE("GetDisplayNameOf %p\n", pidl);

    hr = m_PidlManager->FindPidlInList(pidl, &info);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if ((GET_SHGDN_RELATION(uFlags) == SHGDN_NORMAL) &&
        (GET_SHGDN_FOR(uFlags) & SHGDN_FORPARSING))
    {
        WCHAR path[MAX_PATH] = { 0 };

        hr = GetFullName(m_shellPidl, uFlags, path, _countof(path));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        PathAppendW(path, info->entryName);

        hr = MakeStrRetFromString(path, lpName);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        LPCITEMIDLIST pidlNext = ILGetNext(pidl);

        if (pidlNext && pidlNext->mkid.cb > 0)
        {
            CComPtr<IShellFolder> psfChild;
            hr = BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfChild));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            WCHAR temp[MAX_PATH];
            STRRET childName;

            hr = psfChild->GetDisplayNameOf(pidlNext, uFlags | SHGDN_INFOLDER, &childName);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            hr = StrRetToBufW(&childName, pidlNext, temp, _countof(temp));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            PathAppendW(path, temp);
        }
    }
    else
    {
        MakeStrRetFromString(info->entryName, info->entryNameLength, lpName);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::SetNameOf(
    HWND hwnd,
    LPCITEMIDLIST pidl,
    LPCOLESTR lpszName,
    SHGDNF uFlags,
    LPITEMIDLIST *ppidlOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// IPersist
HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetClassID(CLSID *lpClassId)
{
    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_NtObjectFolder;
    return S_OK;
}

// IPersistFolder
HRESULT STDMETHODCALLTYPE CNtObjectFolder::Initialize(LPCITEMIDLIST pidl)
{
    m_shellPidl = ILClone(pidl);

    PCWSTR ntPath = L"\\";

    if (!m_PidlManager)
    {
        m_PidlManager = new CNtObjectPidlManager();

        StringCbCopy(m_NtPath, _countof(m_NtPath), ntPath);
    }

    return m_PidlManager->Initialize(m_NtPath);
}

// Internal
HRESULT STDMETHODCALLTYPE CNtObjectFolder::Initialize(LPCITEMIDLIST pidl, PCWSTR ntPath)
{
    TRACE("INITIALIZE %p CNtObjectFolder with ntPath %S\n", this, ntPath);

    if (!m_PidlManager)
        m_PidlManager = new CNtObjectPidlManager();

    StringCbCopy(m_NtPath, _countof(m_NtPath), ntPath);
    return m_PidlManager->Initialize(m_NtPath);
}

// IPersistFolder2
HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetCurFolder(LPITEMIDLIST * pidl)
{
    if (pidl)
        *pidl = ILClone(m_shellPidl);
    if (!m_shellPidl)
        return S_FALSE;
    return S_OK;
}

// IShellFolder2
HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetDefaultSearchGUID(
    GUID *lpguid)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::EnumSearches(
    IEnumExtraSearch **ppenum)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetDefaultColumn(
    DWORD dwReserved,
    ULONG *pSort,
    ULONG *pDisplay)
{
    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetDefaultColumnState(
    UINT iColumn,
    SHCOLSTATEF *pcsFlags)
{
    switch (iColumn)
    {
    case NTOBJECT_COLUMN_NAME:
        *pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;
        return S_OK;
    case NTOBJECT_COLUMN_TYPE:
        *pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;
        return S_OK;
    case NTOBJECT_COLUMN_CREATEDATE:
        *pcsFlags = SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT;
        return S_OK;
    case NTOBJECT_COLUMN_LINKTARGET:
        *pcsFlags = SHCOLSTATE_TYPE_STR;
        return S_OK;
    }

    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetDetailsEx(
    LPCITEMIDLIST pidl,
    const SHCOLUMNID *pscid,
    VARIANT *pv)
{
    NtPidlEntry * info;
    HRESULT hr;

    TRACE("GetDetailsEx\n");

    if (pidl)
    {
        hr = m_PidlManager->FindPidlInList(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        static const GUID storage = PSGUID_STORAGE;
        if (IsEqualGUID(pscid->fmtid, storage))
        {
            if (pscid->pid == PID_STG_NAME)
            {
                return MakeVariantString(pv, info->entryName);
            }
            else if (pscid->pid == PID_STG_STORAGETYPE)
            {
                if (info->objectType < 0)
                {
                    NtPidlTypeData * td = (NtPidlTypeData*) (((PBYTE) info) + FIELD_OFFSET(NtPidlEntry, entryName) + info->entryNameLength + sizeof(WCHAR));

                    if (td->typeNameLength > 0)
                    {
                        return MakeVariantString(pv, td->typeName);
                    }
                    else
                    {
                        return MakeVariantString(pv, L"Unknown");
                    }
                }
                else
                {
                    return MakeVariantString(pv, ObjectTypeNames[info->objectType]);
                }
            }
            else if (pscid->pid == PID_STG_WRITETIME)
            {
                DOUBLE varTime;
                SYSTEMTIME stime;
                FileTimeToSystemTime((FILETIME*) &(info->objectInformation.CreateTime), &stime);
                SystemTimeToVariantTime(&stime, &varTime);

                V_VT(pv) = VT_DATE;
                V_DATE(pv) = varTime;
                return S_OK;
            }
        }
        else if (IsEqualGUID(pscid->fmtid, GUID_NtObjectColumns))
        {
            if (pscid->pid == NTOBJECT_COLUMN_LINKTARGET)
            {
                if (info->objectType == SYMBOLICLINK_OBJECT)
                {
                    NtPidlSymlinkData * symlink = (NtPidlSymlinkData*) (((PBYTE) info) + FIELD_OFFSET(NtPidlEntry, entryName) + info->entryNameLength + sizeof(WCHAR));

                    if (symlink->targetNameLength > 0)
                    {
                        return MakeVariantString(pv, symlink->targetName);
                    }
                }

                V_VT(pv) = VT_EMPTY;
                return S_OK;
            }
        }
    }

    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetDetailsOf(
    LPCITEMIDLIST pidl,
    UINT iColumn,
    SHELLDETAILS *psd)
{
    NtPidlEntry * info;
    HRESULT hr;

    TRACE("GetDetailsOf\n");

    if (pidl)
    {
        hr = m_PidlManager->FindPidlInList(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        switch (iColumn)
        {
        case NTOBJECT_COLUMN_NAME:
            psd->fmt = LVCFMT_LEFT;

            MakeStrRetFromString(info->entryName, info->entryNameLength, &(psd->str));
            return S_OK;
        case NTOBJECT_COLUMN_TYPE:
            psd->fmt = LVCFMT_LEFT;

            if (info->objectType < 0)
            {
                NtPidlTypeData * td = (NtPidlTypeData*) (((PBYTE) info) + FIELD_OFFSET(NtPidlEntry, entryName) + info->entryNameLength + sizeof(WCHAR));

                if (td->typeNameLength > 0)
                    MakeStrRetFromString(td->typeName, td->typeNameLength, &(psd->str));
                else
                    MakeStrRetFromString(L"Unknown", &(psd->str));
            }
            else
                MakeStrRetFromString(ObjectTypeNames[info->objectType], &(psd->str));
            return S_OK;
        case NTOBJECT_COLUMN_CREATEDATE:
            psd->fmt = LVCFMT_LEFT;

            if (info->objectInformation.CreateTime.QuadPart != 0)
            {
                WCHAR dbuff[128];
                PWSTR tbuff;
                SYSTEMTIME stime;
                FileTimeToSystemTime((LPFILETIME) &(info->objectInformation.CreateTime), &stime);
                GetDateFormat(LOCALE_USER_DEFAULT, 0, &stime, NULL, dbuff, _countof(dbuff));
                tbuff = dbuff + wcslen(dbuff);
                *tbuff++ = L' ';
                GetTimeFormat(LOCALE_USER_DEFAULT, 0, &stime, NULL, tbuff, _countof(dbuff) - (tbuff - dbuff));

                MakeStrRetFromString(dbuff, &(psd->str));
                return S_OK;
            }

            MakeStrRetFromString(L"", &(psd->str));
            return S_OK;

        case NTOBJECT_COLUMN_LINKTARGET:
            psd->fmt = LVCFMT_LEFT;

            if (info->objectType == SYMBOLICLINK_OBJECT)
            {
                NtPidlSymlinkData * symlink = (NtPidlSymlinkData*) (((PBYTE) info) + FIELD_OFFSET(NtPidlEntry, entryName) + info->entryNameLength + sizeof(WCHAR));

                if (symlink->targetNameLength > 0)
                {
                    MakeStrRetFromString(symlink->targetName, symlink->targetNameLength, &(psd->str));
                    return S_OK;
                }
            }

            MakeStrRetFromString(L"", &(psd->str));
            return S_OK;
        }
    }
    else
    {
        switch (iColumn)
        {
        case NTOBJECT_COLUMN_NAME:
            psd->fmt = LVCFMT_LEFT;
            psd->cxChar = 30;

            // TODO: Make localizable
            MakeStrRetFromString(L"Object Name", &(psd->str));
            return S_OK;
        case NTOBJECT_COLUMN_TYPE:
            psd->fmt = LVCFMT_LEFT;
            psd->cxChar = 20;

            // TODO: Make localizable
            MakeStrRetFromString(L"Object Type", &(psd->str));
            return S_OK;
        case NTOBJECT_COLUMN_CREATEDATE:
            psd->fmt = LVCFMT_LEFT;
            psd->cxChar = 20;

            // TODO: Make localizable
            MakeStrRetFromString(L"Creation Time", &(psd->str));
            return S_OK;
        case NTOBJECT_COLUMN_LINKTARGET:
            psd->fmt = LVCFMT_LEFT;
            psd->cxChar = 30;

            // TODO: Make localizable
            MakeStrRetFromString(L"Symlink Target", &(psd->str));
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::MapColumnToSCID(
    UINT iColumn,
    SHCOLUMNID *pscid)
{
    static const GUID storage = PSGUID_STORAGE;
    switch (iColumn)
    {
    case NTOBJECT_COLUMN_NAME:
        pscid->fmtid = storage;
        pscid->pid = PID_STG_NAME;
        return S_OK;
    case NTOBJECT_COLUMN_TYPE:
        pscid->fmtid = storage;
        pscid->pid = PID_STG_STORAGETYPE;
        return S_OK;
    case NTOBJECT_COLUMN_CREATEDATE:
        pscid->fmtid = storage;
        pscid->pid = PID_STG_WRITETIME;
        return S_OK;
    case NTOBJECT_COLUMN_LINKTARGET:
        pscid->fmtid = GUID_NtObjectColumns;
        pscid->pid = NTOBJECT_COLUMN_LINKTARGET;
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case SFVM_DEFVIEWMODE:
    {
        FOLDERVIEWMODE* pViewMode = (FOLDERVIEWMODE*) lParam;
        *pViewMode = FVM_DETAILS;
        return S_OK;
    }
    }
    return E_NOTIMPL;
}
