/*
 * Copyright 2004, 2005 Martin Fuchs
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <precomp.h>

#include "ntobjenum.h"
#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(ntobjshex);

static struct RootKeyEntry {
    HKEY key;
    PCWSTR keyName;
} RootKeys [] = {
    { HKEY_CLASSES_ROOT, L"HKEY_CLASSES_ROOT" },
    { HKEY_CURRENT_USER, L"HKEY_CURRENT_USER" },
    { HKEY_LOCAL_MACHINE, L"HKEY_LOCAL_MACHINE" },
    { HKEY_USERS, L"HKEY_USERS" },
    { HKEY_CURRENT_CONFIG, L"HKEY_CURRENT_CONFIG" }
};

typedef NTSTATUS(__stdcall* pfnNtGenericOpen)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
typedef NTSTATUS(__stdcall* pfnNtOpenFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);

const LPCWSTR ObjectTypeNames [] = {
    L"Directory", L"SymbolicLink",
    L"Mutant", L"Section", L"Event", L"Semaphore",
    L"Timer", L"Key", L"EventPair", L"IoCompletion",
    L"Device", L"File", L"Controller", L"Profile",
    L"Type", L"Desktop", L"WindowStatiom", L"Driver",
    L"Token", L"Process", L"Thread", L"Adapter", L"Port",
    0
};

const LPCWSTR RegistryTypeNames [] = {
    L"REG_NONE",
    L"REG_SZ",
    L"REG_EXPAND_SZ",
    L"REG_BINARY",
    L"REG_DWORD",
    L"REG_DWORD_BIG_ENDIAN",
    L"REG_LINK",
    L"REG_MULTI_SZ",
    L"REG_RESOURCE_LIST",
    L"REG_FULL_RESOURCE_DESCRIPTOR",
    L"REG_RESOURCE_REQUIREMENTS_LIST ",
    L"REG_QWORD"
};

static DWORD NtOpenObject(OBJECT_TYPE type, PHANDLE phandle, DWORD access, LPCWSTR path)
{
    UNICODE_STRING ustr;

    RtlInitUnicodeString(&ustr, path);

    OBJECT_ATTRIBUTES open_struct = { sizeof(OBJECT_ATTRIBUTES), 0x00, &ustr, 0x40 };

    if (type != FILE_OBJECT)
        access |= STANDARD_RIGHTS_READ;

    IO_STATUS_BLOCK ioStatusBlock;

    switch (type)
    {
    case DIRECTORY_OBJECT:      return NtOpenDirectoryObject(phandle, access, &open_struct);
    case SYMBOLICLINK_OBJECT:   return NtOpenSymbolicLinkObject(phandle, access, &open_struct);
    case MUTANT_OBJECT:         return NtOpenMutant(phandle, access, &open_struct);
    case SECTION_OBJECT:        return NtOpenSection(phandle, access, &open_struct);
    case EVENT_OBJECT:          return NtOpenEvent(phandle, access, &open_struct);
    case SEMAPHORE_OBJECT:      return NtOpenSemaphore(phandle, access, &open_struct);
    case TIMER_OBJECT:          return NtOpenTimer(phandle, access, &open_struct);
    case KEY_OBJECT:            return NtOpenKey(phandle, access, &open_struct);
    case EVENTPAIR_OBJECT:      return NtOpenEventPair(phandle, access, &open_struct);
    case IOCOMPLETITION_OBJECT: return NtOpenIoCompletion(phandle, access, &open_struct);
    case FILE_OBJECT:           return NtOpenFile(phandle, access, &open_struct, &ioStatusBlock, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0);
    default:
        return ERROR_INVALID_FUNCTION;
    }
}

OBJECT_TYPE MapTypeNameToType(LPCWSTR TypeName, DWORD cbTypeName)
{
    if (!TypeName)
        return UNKNOWN_OBJECT_TYPE;

    for (UINT i = 0; i < _countof(ObjectTypeNames); i++)
    {
        LPCWSTR typeName = ObjectTypeNames[i];
        if (!StrCmpNW(typeName, TypeName, cbTypeName / sizeof(WCHAR)))
        {
            return (OBJECT_TYPE) i;
        }
    }

    return UNKNOWN_OBJECT_TYPE;
}

HRESULT ReadRegistryValue(HKEY root, PCWSTR path, PCWSTR valueName, PVOID * valueData, PDWORD valueLength)
{
    HKEY hkey;

    DWORD res;
    if (root)
    {
        res = RegOpenKeyExW(root, *path == '\\' ? path + 1 : path, 0, STANDARD_RIGHTS_READ | KEY_QUERY_VALUE, &hkey);
    }
    else
    {
        res = NtOpenObject(KEY_OBJECT, (PHANDLE) &hkey, STANDARD_RIGHTS_READ | KEY_QUERY_VALUE, path);
    }
    if (!NT_SUCCESS(res))
    {
        ERR("RegOpenKeyExW failed for path %S with status=%x\n", path, res);
        return HRESULT_FROM_NT(res);
    }

    res = RegQueryValueExW(hkey, valueName, NULL, NULL, NULL, valueLength);
    if (!NT_SUCCESS(res))
    {
        ERR("RegQueryValueExW failed for path %S with status=%x\n", path, res);
        return HRESULT_FROM_NT(res);
    }

    if (*valueLength > 0)
    {
        PBYTE data = (PBYTE) CoTaskMemAlloc(*valueLength);;
        *valueData = data;

        res = RegQueryValueExW(hkey, valueName, NULL, NULL, data, valueLength);
        if (!NT_SUCCESS(res))
        {
            CoTaskMemFree(data);
            *valueData = NULL;

            RegCloseKey(hkey);

            ERR("RegOpenKeyExW failed for path %S with status=%x\n", path, res);
            return HRESULT_FROM_NT(res);
        }
    }
    else
    {
        *valueData = NULL;
    }

    RegCloseKey(hkey);

    return S_OK;
}

HRESULT GetNTObjectSymbolicLinkTarget(LPCWSTR path, LPCWSTR entryName, PUNICODE_STRING LinkTarget)
{
    HANDLE handle;
    WCHAR buffer[MAX_PATH];
    LPWSTR pend = buffer;

    StringCbCopyExW(buffer, sizeof(buffer), path, &pend, NULL, 0);

    if (pend[-1] != '\\')
        *pend++ = '\\';

    StringCbCatW(buffer, sizeof(buffer), entryName);

    DbgPrint("GetNTObjectSymbolicLinkTarget %d\n", buffer);

    LinkTarget->Length = 0;

    DWORD err = NtOpenObject(SYMBOLICLINK_OBJECT, &handle, 0, buffer);
    if (!NT_SUCCESS(err))
        return HRESULT_FROM_NT(err);

    err = NT_SUCCESS(NtQuerySymbolicLinkObject(handle, LinkTarget, NULL));
    if (!NT_SUCCESS(err))
        return HRESULT_FROM_NT(err);

    NtClose(handle);

    return S_OK;
}

class CEnumRegRoot :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{
    UINT m_idx;

public:
    CEnumRegRoot()
        : m_idx(0)
    {
    }

    ~CEnumRegRoot()
    {
    }

    HRESULT EnumerateNext(LPITEMIDLIST* ppidl)
    {
        if (m_idx >= _countof(RootKeys))
            return S_FALSE;

        RootKeyEntry& key = RootKeys[m_idx++];

        PCWSTR name = key.keyName;
        DWORD cchName = wcslen(name);

        REG_ENTRY_TYPE otype = REG_ENTRY_ROOT;

        DWORD entryBufferLength = FIELD_OFFSET(RegPidlEntry, entryName) + sizeof(WCHAR) + cchName * sizeof(WCHAR);

        // allocate space for the terminator
        entryBufferLength += 2;

        RegPidlEntry* entry = (RegPidlEntry*) CoTaskMemAlloc(entryBufferLength);
        if (!entry)
            return E_OUTOFMEMORY;

        memset(entry, 0, entryBufferLength);

        entry->cb = FIELD_OFFSET(RegPidlEntry, entryName);
        entry->magic = REGISTRY_PIDL_MAGIC;
        entry->entryType = otype;
        entry->rootKey = key.key;

        if (cchName > 0)
        {
            entry->entryNameLength = cchName * sizeof(WCHAR);
            StringCbCopyNW(entry->entryName, entryBufferLength, name, entry->entryNameLength);
            entry->cb += entry->entryNameLength + sizeof(WCHAR);
        }
        else
        {
            entry->entryNameLength = 0;
            entry->entryName[0] = 0;
            entry->cb += sizeof(WCHAR);
        }

        *ppidl = (LPITEMIDLIST) entry;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
    {
        if (pceltFetched)
            *pceltFetched = 0;

        while (celt-- > 0)
        {
            HRESULT hr = EnumerateNext(rgelt);
            if (hr != S_OK)
                return hr;

            if (pceltFetched)
                (*pceltFetched)++;
            if (rgelt)
                rgelt++;
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt)
    {
        while (celt > 0)
        {
            HRESULT hr = EnumerateNext(NULL);
            if (FAILED(hr))
                return hr;
            if (hr != S_OK)
                break;
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Reset()
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumIDList **ppenum)
    {
        return E_NOTIMPL;
    }

    DECLARE_NOT_AGGREGATABLE(CEnumRegRoot)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CEnumRegRoot)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
    END_COM_MAP()

};

class CEnumRegKey :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{
    PCWSTR m_path;
    HKEY m_hkey;
    BOOL m_values;
    int m_idx;

public:
    CEnumRegKey()
        : m_path(NULL), m_hkey(NULL), m_values(FALSE), m_idx(0)
    {
    }

    ~CEnumRegKey()
    {
        RegCloseKey(m_hkey);
    }

    HRESULT Initialize(PCWSTR path, HKEY root)
    {
        m_path = path;

        DWORD res;
        if (root)
        {
            res = RegOpenKeyExW(root, *path == '\\' ? path + 1 : path, 0, STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &m_hkey);
        }
        else
        {
            res = NtOpenObject(KEY_OBJECT, (PHANDLE) &m_hkey, STANDARD_RIGHTS_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, path);
        }
        if (!NT_SUCCESS(res))
        {
            ERR("RegOpenKeyExW failed for path %S with status=%x\n", path, res);
            return HRESULT_FROM_NT(res);
        }

        return S_OK;
    }

    HRESULT NextKey(LPITEMIDLIST* ppidl)
    {
        WCHAR name[MAX_PATH];
        DWORD cchName = _countof(name);

        WCHAR className[MAX_PATH];
        DWORD cchClass = _countof(className);

        if (RegEnumKeyExW(m_hkey, m_idx++, name, &cchName, 0, className, &cchClass, NULL))
            return S_FALSE;

        name[cchName] = 0;
        className[cchClass] = 0;

        REG_ENTRY_TYPE otype = REG_ENTRY_KEY;

        DWORD entryBufferLength = FIELD_OFFSET(RegPidlEntry, entryName) + sizeof(WCHAR) + cchName * sizeof(WCHAR);

        if (cchClass > 0)
        {
            entryBufferLength += sizeof(WCHAR) + cchClass * sizeof(WCHAR);
        }

        // allocate space for the terminator
        entryBufferLength += 2;

        RegPidlEntry* entry = (RegPidlEntry*) CoTaskMemAlloc(entryBufferLength);
        if (!entry)
            return E_OUTOFMEMORY;

        memset(entry, 0, entryBufferLength);

        entry->cb = FIELD_OFFSET(RegPidlEntry, entryName);
        entry->magic = REGISTRY_PIDL_MAGIC;
        entry->entryType = otype;

        if (cchName > 0)
        {
            entry->entryNameLength = cchName * sizeof(WCHAR);
            StringCbCopyNW(entry->entryName, entryBufferLength, name, entry->entryNameLength);
            entry->cb += entry->entryNameLength + sizeof(WCHAR);
        }
        else
        {
            entry->entryNameLength = 0;
            entry->entryName[0] = 0;
            entry->cb += sizeof(WCHAR);
        }

        if (cchClass)
        {
            PWSTR contentData = (PWSTR) ((PBYTE) entry + entry->cb);
            DWORD remainingSpace = entryBufferLength - entry->cb;

            entry->contentsLength = cchClass * sizeof(WCHAR);
            StringCbCopyNW(contentData, remainingSpace, className, entry->contentsLength);

            entry->cb += entry->contentsLength + sizeof(WCHAR);
        }

        *ppidl = (LPITEMIDLIST) entry;
        return S_OK;
    }

    HRESULT NextValue(LPITEMIDLIST* ppidl)
    {
        WCHAR name[MAX_PATH];
        DWORD cchName = _countof(name);
        DWORD type = 0;
        DWORD dataSize = 0;

        if (RegEnumValueW(m_hkey, m_idx++, name, &cchName, 0, &type, NULL, &dataSize))
            return S_FALSE;

        REG_ENTRY_TYPE otype = REG_ENTRY_VALUE;

        DWORD entryBufferLength = FIELD_OFFSET(RegPidlEntry, entryName) + sizeof(WCHAR) + cchName * sizeof(WCHAR);

        BOOL copyData = dataSize < 32;
        if (copyData)
        {
            entryBufferLength += dataSize + sizeof(WCHAR);

            otype = REG_ENTRY_VALUE_WITH_CONTENT;
        }

        RegPidlEntry* entry = (RegPidlEntry*) CoTaskMemAlloc(entryBufferLength + 2);
        if (!entry)
            return E_OUTOFMEMORY;

        memset(entry, 0, entryBufferLength);

        entry->cb = FIELD_OFFSET(RegPidlEntry, entryName);
        entry->magic = REGISTRY_PIDL_MAGIC;
        entry->entryType = otype;
        entry->contentType = type;

        if (cchName > 0)
        {
            entry->entryNameLength = cchName * sizeof(WCHAR);
            StringCbCopyNW(entry->entryName, entryBufferLength, name, entry->entryNameLength);
            entry->cb += entry->entryNameLength + sizeof(WCHAR);
        }
        else
        {
            entry->entryNameLength = 0;
            entry->entryName[0] = 0;
            entry->cb += sizeof(WCHAR);
        }

        if (copyData)
        {
            PBYTE contentData = (PBYTE) ((PBYTE) entry + entry->cb);

            entry->contentsLength = dataSize;

            // In case it's an unterminated string, RegGetValue will add the NULL termination
            dataSize += sizeof(WCHAR);

            if (!RegQueryValueExW(m_hkey, name, NULL, NULL, contentData, &dataSize))
            {
                entry->cb += entry->contentsLength + sizeof(WCHAR);
            }
            else
            {
                entry->contentsLength = 0;
                entry->cb += sizeof(WCHAR);
            }

        }

        *ppidl = (LPITEMIDLIST) entry;
        return S_OK;
    }

    HRESULT EnumerateNext(LPITEMIDLIST* ppidl)
    {
        if (!m_values)
        {
            HRESULT hr = NextKey(ppidl);
            if (hr != S_FALSE)
                return hr;

            // switch to values.
            m_values = TRUE;
            m_idx = 0;
        }

        return NextValue(ppidl);
    }

    virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
    {
        if (pceltFetched)
            *pceltFetched = 0;

        while (celt-- > 0)
        {
            HRESULT hr = EnumerateNext(rgelt);
            if (hr != S_OK)
                return hr;

            if (pceltFetched)
                (*pceltFetched)++;
            if (rgelt)
                rgelt++;
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt)
    {
        while (celt > 0)
        {
            HRESULT hr = EnumerateNext(NULL);
            if (FAILED(hr))
                return hr;
            if (hr != S_OK)
                break;
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Reset()
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumIDList **ppenum)
    {
        return E_NOTIMPL;
    }

    DECLARE_NOT_AGGREGATABLE(CEnumRegKey)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CEnumRegKey)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
    END_COM_MAP()
};

class CEnumNTDirectory :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{
    WCHAR buffer[MAX_PATH];
    HANDLE m_directory;
    BOOL m_first;
    ULONG m_enumContext;
    PWSTR m_pend;

public:
    CEnumNTDirectory()
        : m_directory(NULL), m_first(TRUE), m_enumContext(0), m_pend(NULL)
    {
    }

    ~CEnumNTDirectory()
    {
        NtClose(m_directory);
    }

    HRESULT Initialize(PCWSTR path)
    {
        StringCbCopyExW(buffer, sizeof(buffer), path, &m_pend, NULL, 0);

        DWORD err = NtOpenObject(DIRECTORY_OBJECT, &m_directory, FILE_LIST_DIRECTORY, buffer);
        if (!NT_SUCCESS(err))
        {
            ERR("NtOpenDirectoryObject failed for path %S with status=%x\n", buffer, err);
            return HRESULT_FROM_NT(err);
        }

        if (m_pend[-1] != '\\')
            *m_pend++ = '\\';

        return S_OK;
    }

    HRESULT EnumerateNext(LPITEMIDLIST* ppidl)
    {
        BYTE dirbuffer[2048];
        if (!NT_SUCCESS(NtQueryDirectoryObject(m_directory, dirbuffer, 2048, TRUE, m_first, &m_enumContext, NULL)))
            return S_FALSE;

        // if ppidl is NULL, assume the caller was Skip(),
        // so we don't care about the info
        if (!ppidl)
            return S_OK;

        m_first = FALSE;
        POBJECT_DIRECTORY_INFORMATION info = (POBJECT_DIRECTORY_INFORMATION) dirbuffer;

        if (info->Name.Buffer)
        {
            StringCbCopyNW(m_pend, sizeof(buffer), info->Name.Buffer, info->Name.Length);
        }

        OBJECT_TYPE otype = MapTypeNameToType(info->TypeName.Buffer, info->TypeName.Length);

        DWORD entryBufferLength = FIELD_OFFSET(NtPidlEntry, entryName) + sizeof(WCHAR);
        if (info->Name.Buffer)
            entryBufferLength += info->Name.Length;

        if (otype < 0)
        {
            entryBufferLength += FIELD_OFFSET(NtPidlTypeData, typeName) + sizeof(WCHAR);

            if (info->TypeName.Buffer)
            {
                entryBufferLength += info->TypeName.Length;
            }
        }

        // allocate space for the terminator
        entryBufferLength += 2;

        NtPidlEntry* entry = (NtPidlEntry*) CoTaskMemAlloc(entryBufferLength);
        if (!entry)
            return E_OUTOFMEMORY;

        memset(entry, 0, entryBufferLength);

        entry->cb = FIELD_OFFSET(NtPidlEntry, entryName);
        entry->magic = NT_OBJECT_PIDL_MAGIC;
        entry->objectType = otype;

        if (info->Name.Buffer)
        {
            entry->entryNameLength = info->Name.Length;
            StringCbCopyNW(entry->entryName, entryBufferLength, info->Name.Buffer, info->Name.Length);
            entry->cb += entry->entryNameLength + sizeof(WCHAR);
        }
        else
        {
            entry->entryNameLength = 0;
            entry->entryName[0] = 0;
            entry->cb += sizeof(WCHAR);
        }

        if (otype < 0)
        {
            NtPidlTypeData * typedata = (NtPidlTypeData*) ((PBYTE) entry + entry->cb);
            DWORD remainingSpace = entryBufferLength - ((PBYTE) (typedata->typeName) - (PBYTE) entry);

            if (info->TypeName.Buffer)
            {
                typedata->typeNameLength = info->TypeName.Length;
                StringCbCopyNW(typedata->typeName, remainingSpace, info->TypeName.Buffer, info->TypeName.Length);

                entry->cb += typedata->typeNameLength + sizeof(WCHAR);
            }
            else
            {
                typedata->typeNameLength = 0;
                typedata->typeName[0] = 0;
                entry->cb += typedata->typeNameLength + sizeof(WCHAR);
            }
        }

        *ppidl = (LPITEMIDLIST) entry;

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
    {
        if (pceltFetched)
            *pceltFetched = 0;

        while (celt-- > 0)
        {
            HRESULT hr = EnumerateNext(rgelt);
            if (hr != S_OK)
                return hr;

            if (pceltFetched)
                (*pceltFetched)++;
            if (rgelt)
                rgelt++;
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt)
    {
        while (celt > 0)
        {
            HRESULT hr = EnumerateNext(NULL);
            if (FAILED(hr))
                return hr;
            if (hr != S_OK)
                break;
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Reset()
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumIDList **ppenum)
    {
        return E_NOTIMPL;
    }

    DECLARE_NOT_AGGREGATABLE(CEnumNTDirectory)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CEnumNTDirectory)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
    END_COM_MAP()
};

HRESULT GetEnumRegistryRoot(IEnumIDList ** ppil)
{
    return ShellObjectCreator<CEnumRegRoot>(IID_PPV_ARG(IEnumIDList, ppil));
}

HRESULT GetEnumRegistryKey(LPCWSTR path, HKEY root, IEnumIDList ** ppil)
{
    return ShellObjectCreatorInit<CEnumRegKey>(path, root, IID_PPV_ARG(IEnumIDList, ppil));
}

HRESULT GetEnumNTDirectory(LPCWSTR path, IEnumIDList ** ppil)
{
    return ShellObjectCreatorInit<CEnumNTDirectory>(path, IID_PPV_ARG(IEnumIDList, ppil));
}