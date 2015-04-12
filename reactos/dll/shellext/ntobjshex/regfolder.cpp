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

#define DFM_MERGECONTEXTMENU 1 // uFlags LPQCMINFO
#define DFM_INVOKECOMMAND 2 // idCmd pszArgs
#define DFM_INVOKECOMMANDEX 12 // idCmd PDFMICS
#define DFM_GETDEFSTATICID 14 // idCmd * 0

#define SHCIDS_ALLFIELDS 0x80000000L
#define SHCIDS_CANONICALONLY 0x10000000L

#define GET_SHGDN_FOR(dwFlags)         ((DWORD)dwFlags & (DWORD)0x0000FF00)
#define GET_SHGDN_RELATION(dwFlags)    ((DWORD)dwFlags & (DWORD)0x000000FF)

WINE_DEFAULT_DEBUG_CHANNEL(ntobjshex);

// {1C6D6E08-2332-4A7B-A94D-6432DB2B5AE6}
const GUID CLSID_RegistryFolder = { 0x1c6d6e08, 0x2332, 0x4a7b, { 0xa9, 0x4d, 0x64, 0x32, 0xdb, 0x2b, 0x5a, 0xe6 } };

// {18A4B504-F6D8-4D8A-8661-6296514C2CF0}
static const GUID GUID_RegistryColumns = { 0x18a4b504, 0xf6d8, 0x4d8a, { 0x86, 0x61, 0x62, 0x96, 0x51, 0x4c, 0x2c, 0xf0 } };

enum RegistryColumns
{
    REGISTRY_COLUMN_NAME = 0,
    REGISTRY_COLUMN_TYPE,
    REGISTRY_COLUMN_VALUE,
    REGISTRY_COLUMN_END
};

class CRegistryFolderExtractIcon :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IExtractIconW
{
    PCIDLIST_ABSOLUTE m_pcidlFolder;
    PCITEMID_CHILD    m_pcidlChild;

public:
    CRegistryFolderExtractIcon() :
        m_pcidlFolder(NULL),
        m_pcidlChild(NULL)
    {

    }

    virtual ~CRegistryFolderExtractIcon()
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
        const RegPidlEntry * entry = (RegPidlEntry *) m_pcidlChild;

        if ((entry->cb < sizeof(RegPidlEntry)) || (entry->magic != REGISTRY_PIDL_MAGIC))
            return E_INVALIDARG;

        UINT flags = 0;

        switch (entry->entryType)
        {
        case REG_ENTRY_KEY:
        case REG_ENTRY_ROOT:
            GetModuleFileNameW(g_hInstance, szIconFile, cchMax);
            *piIndex = -IDI_REGISTRYKEY;
            *pwFlags = flags;
            return S_OK;
        case REG_ENTRY_VALUE:
            GetModuleFileNameW(g_hInstance, szIconFile, cchMax);
            *piIndex = -IDI_REGISTRYVALUE;
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

    DECLARE_NOT_AGGREGATABLE(CRegistryFolderExtractIcon)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CRegistryFolderExtractIcon)
        COM_INTERFACE_ENTRY_IID(IID_IExtractIconW, IExtractIconW)
    END_COM_MAP()

};

class CRegistryPidlManager
{
private:
    PWSTR m_ntPath;
    HKEY m_hRoot;

    HDPA m_hDpa;
    UINT m_hDpaCount;

    int  DpaDeleteCallback(RegPidlEntry * info)
    {
        CoTaskMemFree(info);
        return 0;
    }

    static int CALLBACK s_DpaDeleteCallback(void *pItem, void *pData)
    {
        CRegistryPidlManager * mf = (CRegistryPidlManager*) pData;
        RegPidlEntry  * item = (RegPidlEntry*) pItem;
        return mf->DpaDeleteCallback(item);
    }

public:
    CRegistryPidlManager() :
        m_ntPath(NULL),
        m_hRoot(NULL),
        m_hDpa(NULL),
        m_hDpaCount(0)
    {
    }

    ~CRegistryPidlManager()
    {
        DPA_DestroyCallback(m_hDpa, s_DpaDeleteCallback, this);
    }

    HRESULT Initialize(PWSTR ntPath, HKEY hRoot)
    {
        m_ntPath = ntPath;
        m_hRoot = hRoot;
        m_hDpa = NULL;

        return S_OK;
    }

    HRESULT Enumerate()
    {
        if (m_hDpa)
            return S_OK;

        m_hDpa = DPA_Create(10);

        if (!m_hDpa)
            return E_OUTOFMEMORY;

        if (wcslen(m_ntPath) == 0 && m_hRoot == NULL)
        {
            HRESULT hr = EnumerateRootKeys(m_hDpa, &m_hDpaCount);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
        }
        else
        {
            HRESULT hr = EnumerateRegistryKey(m_hDpa, m_ntPath, m_hRoot, &m_hDpaCount);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
        }
        return S_OK;
    }

    HRESULT FindPidlInList(PCUITEMID_CHILD pcidl, const RegPidlEntry ** pinfo)
    {
        HRESULT hr;

        if (!m_hDpa)
        {
            hr = Enumerate();
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            if (!m_hDpa)
                return E_FAIL;
        }

        RegPidlEntry * info = (RegPidlEntry *) pcidl;
        if ((info->cb < sizeof(RegPidlEntry)) || (info->magic != REGISTRY_PIDL_MAGIC))
        {
            ERR("FindPidlInList: Requested pidl is not of the correct type.\n");
            return E_INVALIDARG;
        }

        TRACE("Searching for pidl { cb=%d } in a list of %d items\n", pcidl->mkid.cb, m_hDpaCount);

        for (UINT i = 0; i < m_hDpaCount; i++)
        {
            RegPidlEntry * pInfo = (RegPidlEntry *) DPA_GetPtr(m_hDpa, i);
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

    HRESULT FindByName(LPCWSTR strParsingName, RegPidlEntry ** pinfo)
    {
        HRESULT hr;

        if (!m_hDpa)
        {
            hr = Enumerate();
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            if (!m_hDpa)
                return E_FAIL;
        }


        TRACE("Searching for '%S' in a list of %d items\n", strParsingName, m_hDpaCount);

        for (int i = 0; i < (int) m_hDpaCount; i++)
        {
            RegPidlEntry * pInfo = (RegPidlEntry *) DPA_GetPtr(m_hDpa, i);
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

    HRESULT GetPidl(UINT index, RegPidlEntry ** pEntry)
    {
        HRESULT hr;

        if (!m_hDpa)
        {
            hr = Enumerate();
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            if (!m_hDpa)
                return E_FAIL;
        }

        *pEntry = NULL;

        RegPidlEntry * entry = (RegPidlEntry *) DPA_GetPtr(m_hDpa, index);
        if (!entry)
        {
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }

        *pEntry = entry;
        return S_OK;
    }

    HRESULT GetCount(UINT * count)
    {
        HRESULT hr;

        if (!m_hDpa)
        {
            hr = Enumerate();
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            if (!m_hDpa)
                return E_FAIL;
        }

        *count = m_hDpaCount;
        return S_OK;
    }


    static LPITEMIDLIST CreatePidlFromItem(const RegPidlEntry * entry)
    {
        LPITEMIDLIST idl = (LPITEMIDLIST) CoTaskMemAlloc(entry->cb + 2);
        if (!idl)
            return NULL;
        memset(idl, 0, entry->cb + 2);
        memcpy(idl, entry, entry->cb);
        return idl;
    }

    static HRESULT CompareIDs(LPARAM lParam, const RegPidlEntry * first, const RegPidlEntry * second)
    {
        if ((lParam & 0xFFFF0000) == SHCIDS_ALLFIELDS)
        {
            if (lParam != 0)
                return E_INVALIDARG;

            int minsize = min(first->cb, second->cb);
            int ord = memcmp(second, first, minsize);

            if (ord != 0)
                return MAKE_HRESULT(0, 0, (USHORT) ord);

            if (second->cb > first->cb)
                return MAKE_HRESULT(0, 0, (USHORT) 1);
            if (second->cb < first->cb)
                return MAKE_HRESULT(0, 0, (USHORT) -1);
        }
        else
        {
            bool canonical = ((lParam & 0xFFFF0000) == SHCIDS_CANONICALONLY);

            switch (lParam & 0xFFFF)
            {
            case REGISTRY_COLUMN_NAME:
            {
                bool f1 = (first->entryType == REG_ENTRY_KEY) || (first->entryType == REG_ENTRY_ROOT);
                bool f2 = (second->entryType == REG_ENTRY_KEY) || (second->entryType == REG_ENTRY_ROOT);

                if (f1 && !f2)
                    return MAKE_HRESULT(0, 0, (USHORT) -1);
                if (f2 && !f1)
                    return MAKE_HRESULT(0, 0, (USHORT) 1);

                if (canonical)
                {
                    // Shortcut: avoid comparing contents if not necessary when the results are not for display.
                    if (second->entryNameLength > first->entryNameLength)
                        return MAKE_HRESULT(0, 0, (USHORT) 1);
                    if (second->entryNameLength < first->entryNameLength)
                        return MAKE_HRESULT(0, 0, (USHORT) -1);
                }

                int minlength = min(first->entryNameLength, second->entryNameLength);
                int ord = StrCmpNW(first->entryName, second->entryName, minlength);

                if (ord != 0)
                    return MAKE_HRESULT(0, 0, (USHORT) ord);

                if (!canonical)
                {
                    if (second->entryNameLength > first->entryNameLength)
                        return MAKE_HRESULT(0, 0, (USHORT) 1);
                    if (second->entryNameLength < first->entryNameLength)
                        return MAKE_HRESULT(0, 0, (USHORT) -1);
                }

                return S_OK;
            }
            case REGISTRY_COLUMN_TYPE:
            {
                int ord = second->contentType - first->contentType;
                if (ord > 0)
                    return MAKE_HRESULT(0, 0, (USHORT) 1);
                if (ord < 0)
                    return MAKE_HRESULT(0, 0, (USHORT) -1);

                return S_OK;
            }
            case REGISTRY_COLUMN_VALUE:
            {
                // Can't sort by value
                return E_INVALIDARG;
            }
            default:
            {
                DbgPrint("Unsupported sorting mode.\n");
                return E_INVALIDARG;
            }
            }
        }

        return E_INVALIDARG;
    }

    static HRESULT CompareIDs(LPARAM lParam, const RegPidlEntry * first, LPCITEMIDLIST pcidl)
    {
        LPCITEMIDLIST p = pcidl;
        RegPidlEntry * second = (RegPidlEntry*) &(p->mkid);
        if ((second->cb < sizeof(RegPidlEntry)) || (second->magic != REGISTRY_PIDL_MAGIC))
            return E_INVALIDARG;

        return CompareIDs(lParam, first, second);
    }

    static HRESULT CompareIDs(LPARAM lParam, LPCITEMIDLIST pcidl1, LPCITEMIDLIST pcidl2)
    {
        LPCITEMIDLIST p = pcidl1;
        RegPidlEntry * first = (RegPidlEntry*) &(p->mkid);
        if ((first->cb < sizeof(RegPidlEntry)) || (first->magic != REGISTRY_PIDL_MAGIC))
            return E_INVALIDARG;

        return CompareIDs(lParam, first, pcidl2);
    }

    static ULONG ConvertAttributes(const RegPidlEntry * entry, PULONG inMask)
    {
        ULONG mask = inMask ? *inMask : 0xFFFFFFFF;
        ULONG flags = 0;

        if ((entry->entryType == REG_ENTRY_KEY) ||
            (entry->entryType == REG_ENTRY_ROOT))
            flags |= SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE;

        return flags & mask;
    }

    BOOL IsFolder(LPCITEMIDLIST pcidl)
    {
        const RegPidlEntry * entry;
        HRESULT hr = FindPidlInList(pcidl, &entry);
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        return (entry->entryType == REG_ENTRY_KEY) ||
            (entry->entryType == REG_ENTRY_ROOT);
    }

    static HRESULT FormatValueData(DWORD contentType, PVOID td, DWORD contentsLength, PCWSTR * strContents)
    {
        switch (contentType)
        {
        case 0:
        {
            PCWSTR strTodo = L"";
            DWORD bufferLength = (wcslen(strTodo) + 1) * sizeof(WCHAR);
            PWSTR strValue = (PWSTR) CoTaskMemAlloc(bufferLength);
            StringCbCopyW(strValue, bufferLength, strTodo);
            *strContents = strValue;
            return S_OK;
        }
        case REG_SZ:
        case REG_EXPAND_SZ:
        {
            PWSTR strValue = (PWSTR) CoTaskMemAlloc(contentsLength + sizeof(WCHAR));
            StringCbCopyNW(strValue, contentsLength + sizeof(WCHAR), (LPCWSTR) td, contentsLength);
            *strContents = strValue;
            return S_OK;
        }
        case REG_DWORD:
        {
            DWORD bufferLength = 64 * sizeof(WCHAR);
            PWSTR strValue = (PWSTR) CoTaskMemAlloc(bufferLength);
            StringCbPrintfW(strValue, bufferLength, L"0x%08x (%d)",
                *(DWORD*) td, *(DWORD*) td);
            *strContents = strValue;
            return S_OK;
        }
        case REG_QWORD:
        {
            DWORD bufferLength = 64 * sizeof(WCHAR);
            PWSTR strValue = (PWSTR) CoTaskMemAlloc(bufferLength);
            StringCbPrintfW(strValue, bufferLength, L"0x%016llx (%d)",
                *(LARGE_INTEGER*) td, ((LARGE_INTEGER*) td)->QuadPart);
            *strContents = strValue;
            return S_OK;
        }
        default:
        {
            PCWSTR strTodo = L"<TODO: Convert value for display>";
            DWORD bufferLength = (wcslen(strTodo) + 1) * sizeof(WCHAR);
            PWSTR strValue = (PWSTR) CoTaskMemAlloc(bufferLength);
            StringCbCopyW(strValue, bufferLength, strTodo);
            *strContents = strValue;
            return S_OK;
        }
        }
    }

    HRESULT FormatContentsForDisplay(const RegPidlEntry * info, PCWSTR * strContents)
    {
        PVOID td = (((PBYTE) info) + FIELD_OFFSET(RegPidlEntry, entryName) + info->entryNameLength + sizeof(WCHAR));

        if (info->entryType == REG_ENTRY_VALUE_WITH_CONTENT)
        {
            if (info->contentsLength > 0)
            {
                return FormatValueData(info->contentType, td, info->contentsLength, strContents);
            }
        }
        else if (info->entryType == REG_ENTRY_VALUE)
        {
            PVOID valueData;
            DWORD valueLength;
            HRESULT hr = ReadRegistryValue(NULL, m_ntPath, info->entryName, &valueData, &valueLength);
            if (FAILED_UNEXPECTEDLY(hr))
            {
                PCWSTR strEmpty = L"(Error reading value)";
                DWORD bufferLength = (wcslen(strEmpty) + 1) * sizeof(WCHAR);
                PWSTR strValue = (PWSTR) CoTaskMemAlloc(bufferLength);
                StringCbCopyW(strValue, bufferLength, strEmpty);
                *strContents = strValue;
                return S_OK;
            }

            if (valueLength > 0)
            {
                hr = FormatValueData(info->contentType, valueData, valueLength, strContents);

                CoTaskMemFree(valueData);

                return hr;
            }
        }
        else
        {
            PCWSTR strEmpty = L"";
            DWORD bufferLength = (wcslen(strEmpty) + 1) * sizeof(WCHAR);
            PWSTR strValue = (PWSTR) CoTaskMemAlloc(bufferLength);
            StringCbCopyW(strValue, bufferLength, strEmpty);
            *strContents = strValue;
            return S_OK;
        }

        PCWSTR strEmpty = L"(Empty)";
        DWORD bufferLength = (wcslen(strEmpty) + 1) * sizeof(WCHAR);
        PWSTR strValue = (PWSTR) CoTaskMemAlloc(bufferLength);
        StringCbCopyW(strValue, bufferLength, strEmpty);
        *strContents = strValue;
        return S_OK;
    }
};

class CRegistryFolderEnum :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{
private:
    CComPtr<CRegistryFolder> m_Folder;

    HWND m_HwndOwner;
    SHCONTF m_Flags;

    UINT m_Index;
    UINT m_Count;

public:
    CRegistryFolderEnum() :
        m_HwndOwner(NULL),
        m_Flags(0),
        m_Index(0),
        m_Count(0)
    {
    }

    virtual ~CRegistryFolderEnum()
    {
    }

    HRESULT Initialize(CRegistryFolder * folder, HWND hwndOwner, SHCONTF flags)
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
            RegPidlEntry * tinfo;
            BOOL flagsOk = FALSE;

            do {
                HRESULT hr = m_Folder->GetManager().GetPidl(m_Index++, &tinfo);
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;

                switch (tinfo->entryType)
                {
                case REG_ENTRY_KEY:
                case REG_ENTRY_ROOT:
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
        return ShellObjectCreatorInit<CRegistryFolderEnum>(m_Folder, m_HwndOwner, m_Flags, IID_PPV_ARG(IEnumIDList, ppenum));
    }

    DECLARE_NOT_AGGREGATABLE(CRegistryFolderEnum)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CRegistryFolderEnum)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
    END_COM_MAP()

};

//-----------------------------------------------------------------------------
// CRegistryFolder

CRegistryFolder::CRegistryFolder() :
    m_PidlManager(NULL),
    m_shellPidl(NULL)
{
}

CRegistryFolder::~CRegistryFolder()
{
    if (m_shellPidl)
        ILFree(m_shellPidl);
    if (m_PidlManager)
        delete m_PidlManager;
}

// IShellFolder
HRESULT STDMETHODCALLTYPE CRegistryFolder::ParseDisplayName(
    HWND hwndOwner,
    LPBC pbcReserved,
    LPOLESTR lpszDisplayName,
    ULONG *pchEaten,
    LPITEMIDLIST *ppidl,
    ULONG *pdwAttributes)
{
    HRESULT hr;
    RegPidlEntry * info;

    if (!ppidl)
        return E_POINTER;

    if (pchEaten)
        *pchEaten = 0;

    if (pdwAttributes)
        *pdwAttributes = 0;

    TRACE("CRegistryFolder::ParseDisplayName name=%S (ntPath=%S)\n", lpszDisplayName, m_NtPath);

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

HRESULT STDMETHODCALLTYPE CRegistryFolder::EnumObjects(
    HWND hwndOwner,
    SHCONTF grfFlags,
    IEnumIDList **ppenumIDList)
{
    return ShellObjectCreatorInit<CRegistryFolderEnum>(this, hwndOwner, grfFlags, IID_PPV_ARG(IEnumIDList, ppenumIDList));
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::BindToObject(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    void **ppvOut)
{
    const RegPidlEntry * info;
    HRESULT hr;

    if (IsEqualIID(riid, IID_IShellFolder))
    {
        hr = m_PidlManager->FindPidlInList(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        LPITEMIDLIST first = ILCloneFirst(pidl);
        LPCITEMIDLIST rest = ILGetNext(pidl);

        LPITEMIDLIST fullPidl = ILCombine(m_shellPidl, first);

        CComPtr<IShellFolder> psfChild;
        if (wcslen(m_NtPath) == 0 && m_hRoot == NULL)
        {
            hr = ShellObjectCreatorInit<CRegistryFolder>(fullPidl, L"", info->rootKey, IID_PPV_ARG(IShellFolder, &psfChild));
        }
        else
        {
            WCHAR path[MAX_PATH];

            StringCbCopyW(path, _countof(path), m_NtPath);

            PathAppendW(path, info->entryName);

            hr = ShellObjectCreatorInit<CRegistryFolder>(fullPidl, path, m_hRoot, IID_PPV_ARG(IShellFolder, &psfChild));
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

HRESULT STDMETHODCALLTYPE CRegistryFolder::BindToStorage(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    void **ppvObj)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::CompareIDs(
    LPARAM lParam,
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2)
{
    TRACE("CompareIDs\n");

    HRESULT hr = m_PidlManager->CompareIDs(lParam, pidl1, pidl2);
    if (hr != S_OK)
        return hr;

    LPCITEMIDLIST rest1 = ILGetNext(pidl1);
    LPCITEMIDLIST rest2 = ILGetNext(pidl2);

    bool hasNext1 = (rest1->mkid.cb > 0);
    bool hasNext2 = (rest2->mkid.cb > 0);

    if (hasNext1 || hasNext2)
    {
        if (hasNext1 && !hasNext2)
            return MAKE_HRESULT(0, 0, (USHORT) -1);

        if (hasNext2 && !hasNext1)
            return MAKE_HRESULT(0, 0, (USHORT) 1);

        LPCITEMIDLIST first1 = ILCloneFirst(pidl1);

        CComPtr<IShellFolder> psfNext;
        hr = BindToObject(first1, NULL, IID_PPV_ARG(IShellFolder, &psfNext));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return psfNext->CompareIDs(lParam, rest1, rest2);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::CreateViewObject(
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

HRESULT STDMETHODCALLTYPE CRegistryFolder::GetAttributesOf(
    UINT cidl,
    PCUITEMID_CHILD_ARRAY apidl,
    SFGAOF *rgfInOut)
{
    const RegPidlEntry * info;

    TRACE("GetAttributesOf\n");

    if (cidl == 0)
    {
        *rgfInOut &= SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE;
        return S_OK;
    }

    for (int i = 0; i < (int) cidl; i++)
    {
        PCUITEMID_CHILD pidl = apidl[i];

#ifndef DISABLE_STRICT_PIDL_CHECK
        HRESULT hr = m_PidlManager->FindPidlInList(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
#else
        info = (const RegPidlEntry *) pidl;
        if (info->magic != REGISTRY_PIDL_MAGIC)
            return E_INVALIDARG;
#endif

        // Update attributes.
        *rgfInOut = m_PidlManager->ConvertAttributes(info, rgfInOut);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::GetUIObjectOf(
    HWND hwndOwner,
    UINT cidl,
    PCUITEMID_CHILD_ARRAY apidl,
    REFIID riid,
    UINT *prgfInOut,
    void **ppvOut)
{
    TRACE("GetUIObjectOf\n");

    if (IsEqualIID(riid, IID_IContextMenu) ||
        IsEqualIID(riid, IID_IContextMenu2) ||
        IsEqualIID(riid, IID_IContextMenu3))
    {
        CComPtr<IContextMenu> pcm;

        HKEY keys [1];

        int nkeys = _countof(keys);
        if (cidl == 1 && m_PidlManager->IsFolder(apidl[0]))
        {
            RegOpenKey(HKEY_CLASSES_ROOT, L"Folder", keys + 0);
        }
        else
        {
            nkeys = 0;
        }

        HRESULT hr = CDefFolderMenu_Create2(m_shellPidl, hwndOwner, cidl, apidl, this, DefCtxMenuCallback, nkeys, keys, &pcm);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return pcm->QueryInterface(riid, ppvOut);
    }

    if (IsEqualIID(riid, IID_IExtractIconW))
    {
        return ShellObjectCreatorInit<CRegistryFolderExtractIcon>(m_shellPidl, cidl, apidl, riid, ppvOut);
    }

    if (IsEqualIID(riid, IID_IDataObject))
    {
        return CIDLData_CreateFromIDArray(m_shellPidl, cidl, apidl, (IDataObject**) ppvOut);
    }

    if (IsEqualIID(riid, IID_IQueryAssociations))
    {
        if (cidl == 1 && m_PidlManager->IsFolder(apidl[0]))
        {
            CComPtr<IQueryAssociations> pqa;
            HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa));
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            hr = pqa->Init(ASSOCF_INIT_DEFAULTTOFOLDER, L"NTObjShEx.RegFolder", NULL, hwndOwner);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            return pqa->QueryInterface(riid, ppvOut);
        }
    }

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::GetDisplayNameOf(
    LPCITEMIDLIST pidl,
    SHGDNF uFlags,
    STRRET *lpName)
{
    const RegPidlEntry * info;
    HRESULT hr;

    TRACE("GetDisplayNameOf %p\n", pidl);

#ifndef DISABLE_STRICT_PIDL_CHECK
    hr = m_PidlManager->FindPidlInList(pidl, &info);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
#else
    info = (const RegPidlEntry *) pidl;
    if (info->magic != REGISTRY_PIDL_MAGIC)
        return E_INVALIDARG;
#endif

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

        LPCITEMIDLIST pidlFirst = ILCloneFirst(pidl);
        LPCITEMIDLIST pidlNext = ILGetNext(pidl);

        if (pidlNext && pidlNext->mkid.cb > 0)
        {
            CComPtr<IShellFolder> psfChild;
            hr = BindToObject(pidlFirst, NULL, IID_PPV_ARG(IShellFolder, &psfChild));
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

        ILFree((LPITEMIDLIST) pidlFirst);
    }
    else
    {
        MakeStrRetFromString(info->entryName, info->entryNameLength, lpName);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::SetNameOf(
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
HRESULT STDMETHODCALLTYPE CRegistryFolder::GetClassID(CLSID *lpClassId)
{
    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_RegistryFolder;
    return S_OK;
}

// IPersistFolder
HRESULT STDMETHODCALLTYPE CRegistryFolder::Initialize(LPCITEMIDLIST pidl)
{
    m_shellPidl = ILClone(pidl);
    m_hRoot = NULL;

    PCWSTR ntPath = L"";

    if (!m_PidlManager)
    {
        m_PidlManager = new CRegistryPidlManager();

        StringCbCopy(m_NtPath, _countof(m_NtPath), ntPath);
    }

    return m_PidlManager->Initialize(m_NtPath, m_hRoot);
}

// Internal
HRESULT STDMETHODCALLTYPE CRegistryFolder::Initialize(LPCITEMIDLIST pidl, PCWSTR ntPath, HKEY hRoot)
{
    m_shellPidl = ILClone(pidl);
    m_hRoot = hRoot;

    if (!m_PidlManager)
        m_PidlManager = new CRegistryPidlManager();

    StringCbCopy(m_NtPath, _countof(m_NtPath), ntPath);
    return m_PidlManager->Initialize(m_NtPath, m_hRoot);
}

// IPersistFolder2
HRESULT STDMETHODCALLTYPE CRegistryFolder::GetCurFolder(LPITEMIDLIST * pidl)
{
    if (pidl)
        *pidl = ILClone(m_shellPidl);
    if (!m_shellPidl)
        return S_FALSE;
    return S_OK;
}

// IShellFolder2
HRESULT STDMETHODCALLTYPE CRegistryFolder::GetDefaultSearchGUID(
    GUID *lpguid)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::EnumSearches(
    IEnumExtraSearch **ppenum)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::GetDefaultColumn(
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

HRESULT STDMETHODCALLTYPE CRegistryFolder::GetDefaultColumnState(
    UINT iColumn,
    SHCOLSTATEF *pcsFlags)
{
    switch (iColumn)
    {
    case REGISTRY_COLUMN_NAME:
        *pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;
        return S_OK;
    case REGISTRY_COLUMN_TYPE:
        *pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT;
        return S_OK;
    case REGISTRY_COLUMN_VALUE:
        *pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT | SHCOLSTATE_SLOW;
        return S_OK;
    }

    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::GetDetailsEx(
    LPCITEMIDLIST pidl,
    const SHCOLUMNID *pscid,
    VARIANT *pv)
{
    const RegPidlEntry * info;
    HRESULT hr;

    TRACE("GetDetailsEx\n");

    if (pidl)
    {
#ifndef DISABLE_STRICT_PIDL_CHECK
        hr = m_PidlManager->FindPidlInList(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
#else
        info = (const RegPidlEntry *) pidl;
        if (info->magic != REGISTRY_PIDL_MAGIC)
            return E_INVALIDARG;
#endif

        static const GUID storage = PSGUID_STORAGE;
        if (IsEqualGUID(pscid->fmtid, storage))
        {
            if (pscid->pid == PID_STG_NAME)
            {
                if (info->entryNameLength > 0)
                {
                    return MakeVariantString(pv, info->entryName);
                }
                return  MakeVariantString(pv, L"(Default)");
            }
            else if (pscid->pid == PID_STG_STORAGETYPE)
            {
                if (info->entryType == REG_ENTRY_ROOT)
                {
                    return MakeVariantString(pv, L"Key");
                }

                if (info->entryType == REG_ENTRY_KEY)
                {
                    if (info->contentsLength > 0)
                    {
                        PWSTR td = (PWSTR)(((PBYTE) info) + FIELD_OFFSET(RegPidlEntry, entryName) + info->entryNameLength + sizeof(WCHAR));

                        return MakeVariantString(pv, td);
                    }
                    return MakeVariantString(pv, L"Key");
                }

                return MakeVariantString(pv, RegistryTypeNames[info->contentType]);
            }
            else if (pscid->pid == PID_STG_CONTENTS)
            {
                PCWSTR strValueContents;

                hr = m_PidlManager->FormatContentsForDisplay(info, &strValueContents);
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;

                if (hr == S_FALSE)
                {
                    V_VT(pv) = VT_EMPTY;
                    return S_OK;
                }

                hr = MakeVariantString(pv, strValueContents);

                CoTaskMemFree((PVOID) strValueContents);

                return hr;

            }
        }
    }

    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::GetDetailsOf(
    LPCITEMIDLIST pidl,
    UINT iColumn,
    SHELLDETAILS *psd)
{
    const RegPidlEntry * info;
    HRESULT hr;

    TRACE("GetDetailsOf\n");

    if (pidl)
    {
#ifndef DISABLE_STRICT_PIDL_CHECK
        hr = m_PidlManager->FindPidlInList(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
#else
        info = (const RegPidlEntry *) pidl;
        if (info->magic != REGISTRY_PIDL_MAGIC)
            return E_INVALIDARG;
#endif

        switch (iColumn)
        {
        case REGISTRY_COLUMN_NAME:
            psd->fmt = LVCFMT_LEFT;

            if (info->entryNameLength > 0)
            {
                return MakeStrRetFromString(info->entryName, info->entryNameLength, &(psd->str));
            }
            return MakeStrRetFromString(L"(Default)", &(psd->str));

        case REGISTRY_COLUMN_TYPE:
            psd->fmt = LVCFMT_LEFT;

            if (info->entryType == REG_ENTRY_ROOT)
            {
                return MakeStrRetFromString(L"Key", &(psd->str));
            }

            if (info->entryType == REG_ENTRY_KEY)
            {
                if (info->contentsLength > 0)
                {
                    PWSTR td = (PWSTR) (((PBYTE) info) + FIELD_OFFSET(RegPidlEntry, entryName) + info->entryNameLength + sizeof(WCHAR));

                    return MakeStrRetFromString(td, info->contentsLength, &(psd->str));
                }

                return MakeStrRetFromString(L"Key", &(psd->str));
            }

            return MakeStrRetFromString(RegistryTypeNames[info->entryType], &(psd->str));

        case REGISTRY_COLUMN_VALUE:
            psd->fmt = LVCFMT_LEFT;

            PCWSTR strValueContents;

            hr = m_PidlManager->FormatContentsForDisplay(info, &strValueContents);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            if (hr == S_FALSE)
            {
                return MakeStrRetFromString(L"(Empty)", &(psd->str));
            }

            hr = MakeStrRetFromString(strValueContents, &(psd->str));

            CoTaskMemFree((PVOID) strValueContents);

            return hr;
        }
    }
    else
    {
        switch (iColumn)
        {
        case REGISTRY_COLUMN_NAME:
            psd->fmt = LVCFMT_LEFT;
            psd->cxChar = 30;

            // TODO: Make localizable
            MakeStrRetFromString(L"Object Name", &(psd->str));
            return S_OK;
        case REGISTRY_COLUMN_TYPE:
            psd->fmt = LVCFMT_LEFT;
            psd->cxChar = 20;

            // TODO: Make localizable
            MakeStrRetFromString(L"Content Type", &(psd->str));
            return S_OK;
        case REGISTRY_COLUMN_VALUE:
            psd->fmt = LVCFMT_LEFT;
            psd->cxChar = 20;

            // TODO: Make localizable
            MakeStrRetFromString(L"Value", &(psd->str));
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::MapColumnToSCID(
    UINT iColumn,
    SHCOLUMNID *pscid)
{
    static const GUID storage = PSGUID_STORAGE;
    switch (iColumn)
    {
    case REGISTRY_COLUMN_NAME:
        pscid->fmtid = storage;
        pscid->pid = PID_STG_NAME;
        return S_OK;
    case REGISTRY_COLUMN_TYPE:
        pscid->fmtid = storage;
        pscid->pid = PID_STG_STORAGETYPE;
        return S_OK;
    case REGISTRY_COLUMN_VALUE:
        pscid->fmtid = storage;
        pscid->pid = PID_STG_CONTENTS;
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case SFVM_DEFVIEWMODE:
    {
        FOLDERVIEWMODE* pViewMode = (FOLDERVIEWMODE*) lParam;
        *pViewMode = FVM_DETAILS;
        return S_OK;
    }
    case SFVM_COLUMNCLICK:
        return S_FALSE;
    case SFVM_BACKGROUNDENUM:
        return S_OK;
    case SFVM_DEFITEMCOUNT:
        return m_PidlManager->GetCount((UINT*) lParam);
    }
    return E_NOTIMPL;
}

HRESULT CRegistryFolder::DefCtxMenuCallback(IShellFolder * /*psf*/, HWND /*hwnd*/, IDataObject * /*pdtobj*/, UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        return S_OK;
    case DFM_INVOKECOMMAND:
    case DFM_INVOKECOMMANDEX:
    case DFM_GETDEFSTATICID: // Required for Windows 7 to pick a default
        return S_FALSE;
    }
    return E_NOTIMPL;
}
