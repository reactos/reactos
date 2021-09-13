/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/ntobjshex/regfolder.cpp
 * PURPOSE:     NT Object Namespace shell extension
 * PROGRAMMERS: David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"

// {1C6D6E08-2332-4A7B-A94D-6432DB2B5AE6}
const GUID CLSID_RegistryFolder = { 0x1c6d6e08, 0x2332, 0x4a7b, { 0xa9, 0x4d, 0x64, 0x32, 0xdb, 0x2b, 0x5a, 0xe6 } };

// {18A4B504-F6D8-4D8A-8661-6296514C2CF0}
//static const GUID GUID_RegistryColumns = { 0x18a4b504, 0xf6d8, 0x4d8a, { 0x86, 0x61, 0x62, 0x96, 0x51, 0x4c, 0x2c, 0xf0 } };

enum RegistryColumns
{
    REGISTRY_COLUMN_NAME = 0,
    REGISTRY_COLUMN_TYPE,
    REGISTRY_COLUMN_VALUE,
    REGISTRY_COLUMN_END
};

// -------------------------------
// CRegistryFolderExtractIcon
CRegistryFolderExtractIcon::CRegistryFolderExtractIcon() :
    m_pcidlFolder(NULL),
    m_pcidlChild(NULL)
{

}

CRegistryFolderExtractIcon::~CRegistryFolderExtractIcon()
{
    if (m_pcidlFolder)
        ILFree((LPITEMIDLIST)m_pcidlFolder);
    if (m_pcidlChild)
        ILFree((LPITEMIDLIST)m_pcidlChild);
}

HRESULT CRegistryFolderExtractIcon::Initialize(LPCWSTR ntPath, PCIDLIST_ABSOLUTE parent, UINT cidl, PCUITEMID_CHILD_ARRAY apidl)
{
    m_pcidlFolder = ILClone(parent);
    if (cidl != 1)
        return E_INVALIDARG;
    m_pcidlChild = ILClone(apidl[0]);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRegistryFolderExtractIcon::GetIconLocation(
    UINT uFlags,
    LPWSTR szIconFile,
    UINT cchMax,
    INT *piIndex,
    UINT *pwFlags)
{
    const RegPidlEntry * entry = (RegPidlEntry *)m_pcidlChild;

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

HRESULT STDMETHODCALLTYPE CRegistryFolderExtractIcon::Extract(
    LPCWSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize)
{
    return SHDefExtractIconW(pszFile, nIconIndex, 0, phiconLarge, phiconSmall, nIconSize);
}

// CRegistryFolder

CRegistryFolder::CRegistryFolder()
{
}

CRegistryFolder::~CRegistryFolder()
{
}

// IShellFolder
HRESULT STDMETHODCALLTYPE CRegistryFolder::EnumObjects(
    HWND hwndOwner,
    SHCONTF grfFlags,
    IEnumIDList **ppenumIDList)
{
    if (m_NtPath[0] == 0 && m_hRoot == NULL)
    {
        return GetEnumRegistryRoot(ppenumIDList);
    }
    else
    {
        return GetEnumRegistryKey(m_NtPath, m_hRoot, ppenumIDList);
    }
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::InternalBindToObject(
    PWSTR path,
    const RegPidlEntry * info,
    LPITEMIDLIST first,
    LPCITEMIDLIST rest,
    LPITEMIDLIST fullPidl,
    LPBC pbcReserved,
    IShellFolder** ppsfChild)
{
    if (wcslen(m_NtPath) == 0 && m_hRoot == NULL)
    {
        return ShellObjectCreatorInit<CRegistryFolder>(fullPidl, L"", info->rootKey, IID_PPV_ARG(IShellFolder, ppsfChild));
    }

    return ShellObjectCreatorInit<CRegistryFolder>(fullPidl, path, m_hRoot, IID_PPV_ARG(IShellFolder, ppsfChild));
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    m_shellPidl = ILClone(pidl);
    m_hRoot = NULL;

    StringCbCopyW(m_NtPath, sizeof(m_NtPath), L"");
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRegistryFolder::Initialize(PCIDLIST_ABSOLUTE pidl, PCWSTR ntPath, HKEY hRoot)
{
    m_shellPidl = ILClone(pidl);
    m_hRoot = hRoot;

    StringCbCopyW(m_NtPath, sizeof(m_NtPath), ntPath);
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

    TRACE("GetDetailsEx\n");

    if (pidl)
    {
        HRESULT hr = GetInfoFromPidl(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

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
                        PWSTR td = (PWSTR)(((PBYTE)info) + FIELD_OFFSET(RegPidlEntry, entryName) + info->entryNameLength + sizeof(WCHAR));

                        return MakeVariantString(pv, td);
                    }
                    return MakeVariantString(pv, L"Key");
                }

                return MakeVariantString(pv, RegistryTypeNames[info->contentType]);
            }
            else if (pscid->pid == PID_STG_CONTENTS)
            {
                PCWSTR strValueContents;

                hr = FormatContentsForDisplay(info, m_hRoot, m_NtPath, &strValueContents);
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;

                if (hr == S_FALSE)
                {
                    V_VT(pv) = VT_EMPTY;
                    return S_OK;
                }

                hr = MakeVariantString(pv, strValueContents);

                CoTaskMemFree((PVOID)strValueContents);

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

    TRACE("GetDetailsOf\n");

    if (pidl)
    {
        HRESULT hr = GetInfoFromPidl(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

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
                    PWSTR td = (PWSTR)(((PBYTE)info) + FIELD_OFFSET(RegPidlEntry, entryName) + info->entryNameLength + sizeof(WCHAR));

                    return MakeStrRetFromString(td, info->contentsLength, &(psd->str));
                }

                return MakeStrRetFromString(L"Key", &(psd->str));
            }

            return MakeStrRetFromString(RegistryTypeNames[info->entryType], &(psd->str));

        case REGISTRY_COLUMN_VALUE:
            psd->fmt = LVCFMT_LEFT;

            PCWSTR strValueContents;

            hr = FormatContentsForDisplay(info, m_hRoot, m_NtPath, &strValueContents);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            if (hr == S_FALSE)
            {
                return MakeStrRetFromString(L"(Empty)", &(psd->str));
            }

            hr = MakeStrRetFromString(strValueContents, &(psd->str));

            CoTaskMemFree((PVOID)strValueContents);

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

HRESULT CRegistryFolder::CompareIDs(LPARAM lParam, const RegPidlEntry * first, const RegPidlEntry * second)
{
    HRESULT hr;

    DWORD sortMode = lParam & 0xFFFF0000;
    DWORD column = lParam & 0x0000FFFF;

    if (sortMode == SHCIDS_ALLFIELDS)
    {
        if (column != 0)
            return E_INVALIDARG;

        int minsize = min(first->cb, second->cb);
        hr = MAKE_COMPARE_HRESULT(memcmp(second, first, minsize));
        if (hr != S_EQUAL)
            return hr;

        return MAKE_COMPARE_HRESULT(second->cb - first->cb);
    }

    switch (column)
    {
    case REGISTRY_COLUMN_NAME:
        return CompareName(lParam, first, second);

    case REGISTRY_COLUMN_TYPE:
        return MAKE_COMPARE_HRESULT(second->contentType - first->contentType);

    case REGISTRY_COLUMN_VALUE:
        // Can't sort by link target yet
        return E_INVALIDARG;
    }

    DbgPrint("Unsupported sorting mode.\n");
    return E_INVALIDARG;
}

ULONG CRegistryFolder::ConvertAttributes(const RegPidlEntry * entry, PULONG inMask)
{
    ULONG mask = inMask ? *inMask : 0xFFFFFFFF;
    ULONG flags = 0;

    if ((entry->entryType == REG_ENTRY_KEY) ||
        (entry->entryType == REG_ENTRY_ROOT))
        flags |= SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE;

    return flags & mask;
}

BOOL CRegistryFolder::IsFolder(const RegPidlEntry * info)
{
    return (info->entryType == REG_ENTRY_KEY) ||(info->entryType == REG_ENTRY_ROOT);
}

HRESULT CRegistryFolder::GetInfoFromPidl(LPCITEMIDLIST pcidl, const RegPidlEntry ** pentry)
{
    RegPidlEntry * entry = (RegPidlEntry*) &(pcidl->mkid);

    if (entry->cb < sizeof(RegPidlEntry))
    {
        DbgPrint("PCIDL too small %l (required %l)\n", entry->cb, sizeof(RegPidlEntry));
        return E_INVALIDARG;
    }

    if (entry->magic != REGISTRY_PIDL_MAGIC)
    {
        DbgPrint("PCIDL magic mismatch %04x (expected %04x)\n", entry->magic, REGISTRY_PIDL_MAGIC);
        return E_INVALIDARG;
    }

    *pentry = entry;
    return S_OK;
}

HRESULT CRegistryFolder::FormatValueData(DWORD contentType, PVOID td, DWORD contentsLength, PCWSTR * strContents)
{
    switch (contentType)
    {
    case 0:
    {
        PCWSTR strTodo = L"";
        DWORD bufferLength = (wcslen(strTodo) + 1) * sizeof(WCHAR);
        PWSTR strValue = (PWSTR)CoTaskMemAlloc(bufferLength);
        StringCbCopyW(strValue, bufferLength, strTodo);
        *strContents = strValue;
        return S_OK;
    }
    case REG_SZ:
    case REG_EXPAND_SZ:
    {
        PWSTR strValue = (PWSTR)CoTaskMemAlloc(contentsLength + sizeof(WCHAR));
        StringCbCopyNW(strValue, contentsLength + sizeof(WCHAR), (LPCWSTR)td, contentsLength);
        *strContents = strValue;
        return S_OK;
    }
    case REG_MULTI_SZ:
    {
        PCWSTR separator = L" "; // To match regedit
        size_t sepChars = wcslen(separator);
        int strings = 0;
        int stringChars = 0;

        PCWSTR strData = (PCWSTR)td;
        while (*strData)
        {
            size_t len = wcslen(strData);
            stringChars += len;
            strData += len + 1; // Skips null-terminator
            strings++;
        }

        int cch = stringChars + (strings - 1) * sepChars + 1;

        PWSTR strValue = (PWSTR)CoTaskMemAlloc(cch * sizeof(WCHAR));

        strValue[0] = 0;

        strData = (PCWSTR)td;
        while (*strData)
        {
            StrCatW(strValue, strData);
            strData += wcslen(strData) + 1;
            if (*strData)
                StrCatW(strValue, separator);
        }

        *strContents = strValue;
        return S_OK;
    }
    case REG_DWORD:
    {
        DWORD bufferLength = 64 * sizeof(WCHAR);
        PWSTR strValue = (PWSTR)CoTaskMemAlloc(bufferLength);
        StringCbPrintfW(strValue, bufferLength, L"0x%08x (%d)",
            *(DWORD*)td, *(DWORD*)td);
        *strContents = strValue;
        return S_OK;
    }
    case REG_QWORD:
    {
        DWORD bufferLength = 64 * sizeof(WCHAR);
        PWSTR strValue = (PWSTR)CoTaskMemAlloc(bufferLength);
        StringCbPrintfW(strValue, bufferLength, L"0x%016llx (%lld)",
            *(LARGE_INTEGER*)td, ((LARGE_INTEGER*)td)->QuadPart);
        *strContents = strValue;
        return S_OK;
    }
    case REG_BINARY:
    {
        DWORD bufferLength = (contentsLength * 3 + 1) * sizeof(WCHAR);
        PWSTR strValue = (PWSTR)CoTaskMemAlloc(bufferLength);
        PWSTR strTemp = strValue;
        PBYTE data = (PBYTE)td;
        for (DWORD i = 0; i < contentsLength; i++)
        {
            StringCbPrintfW(strTemp, bufferLength, L"%02x ", data[i]);
            strTemp += 3;
            bufferLength -= 3;
        }
        *strContents = strValue;
        return S_OK;
    }
    default:
    {
        PCWSTR strFormat = L"<Unimplemented value type %d>";
        DWORD bufferLength = (wcslen(strFormat) + 15) * sizeof(WCHAR);
        PWSTR strValue = (PWSTR)CoTaskMemAlloc(bufferLength);
        StringCbPrintfW(strValue, bufferLength, strFormat, contentType);
        *strContents = strValue;
        return S_OK;
    }
    }
}

HRESULT CRegistryFolder::FormatContentsForDisplay(const RegPidlEntry * info, HKEY rootKey, LPCWSTR ntPath, PCWSTR * strContents)
{
    PVOID td = (((PBYTE)info) + FIELD_OFFSET(RegPidlEntry, entryName) + info->entryNameLength + sizeof(WCHAR));

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
        HRESULT hr = ReadRegistryValue(rootKey, ntPath, info->entryName, &valueData, &valueLength);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            PCWSTR strEmpty = L"(Error reading value)";
            DWORD bufferLength = (wcslen(strEmpty) + 1) * sizeof(WCHAR);
            PWSTR strValue = (PWSTR)CoTaskMemAlloc(bufferLength);
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
        PWSTR strValue = (PWSTR)CoTaskMemAlloc(bufferLength);
        StringCbCopyW(strValue, bufferLength, strEmpty);
        *strContents = strValue;
        return S_OK;
    }

    PCWSTR strEmpty = L"(Empty)";
    DWORD bufferLength = (wcslen(strEmpty) + 1) * sizeof(WCHAR);
    PWSTR strValue = (PWSTR)CoTaskMemAlloc(bufferLength);
    StringCbCopyW(strValue, bufferLength, strEmpty);
    *strContents = strValue;
    return S_OK;
}
