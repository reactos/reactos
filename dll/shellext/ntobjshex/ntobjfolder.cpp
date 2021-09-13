/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/ntobjshex/ntobjfolder.cpp
 * PURPOSE:     NT Object Namespace shell extension
 * PROGRAMMERS: David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"

#include <wine/unicode.h>

// {845B0FB2-66E0-416B-8F91-314E23F7C12D}
const GUID CLSID_NtObjectFolder = { 0x845b0fb2, 0x66e0, 0x416b, { 0x8f, 0x91, 0x31, 0x4e, 0x23, 0xf7, 0xc1, 0x2d } };

// {F4C430C3-3A8D-4B56-A018-E598DA60C2E0}
static const GUID GUID_NtObjectColumns = { 0xf4c430c3, 0x3a8d, 0x4b56, { 0xa0, 0x18, 0xe5, 0x98, 0xda, 0x60, 0xc2, 0xe0 } };

enum NtObjectColumns
{
    NTOBJECT_COLUMN_NAME = 0,
    NTOBJECT_COLUMN_TYPE,
    NTOBJECT_COLUMN_LINKTARGET,
    NTOBJECT_COLUMN_END
};

CNtObjectFolderExtractIcon::CNtObjectFolderExtractIcon() :
    m_NtPath(NULL),
    m_pcidlChild(NULL)
{

}

CNtObjectFolderExtractIcon::~CNtObjectFolderExtractIcon()
{
    if (m_pcidlChild)
        ILFree((LPITEMIDLIST) m_pcidlChild);
}

HRESULT CNtObjectFolderExtractIcon::Initialize(LPCWSTR ntPath, PCIDLIST_ABSOLUTE parent, UINT cidl, PCUITEMID_CHILD_ARRAY apidl)
{
    m_NtPath = ntPath;
    if (cidl != 1)
        return E_INVALIDARG;
    m_pcidlChild = ILClone(apidl[0]);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolderExtractIcon::GetIconLocation(
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
    case KEY_OBJECT:
        GetModuleFileNameW(g_hInstance, szIconFile, cchMax);
        *piIndex = -IDI_REGISTRYKEY;
        *pwFlags = flags;
        return S_OK;
    default:
        GetModuleFileNameW(g_hInstance, szIconFile, cchMax);
        *piIndex = -IDI_NTOBJECTITEM;
        *pwFlags = flags;
        return S_OK;
    }
}

HRESULT STDMETHODCALLTYPE CNtObjectFolderExtractIcon::Extract(
    LPCWSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize)
{
    return SHDefExtractIconW(pszFile, nIconIndex, 0, phiconLarge, phiconSmall, nIconSize);
}

//-----------------------------------------------------------------------------
// CNtObjectFolder

CNtObjectFolder::CNtObjectFolder()
{
}

CNtObjectFolder::~CNtObjectFolder()
{
}

// IShellFolder

HRESULT STDMETHODCALLTYPE CNtObjectFolder::EnumObjects(
    HWND hwndOwner,
    SHCONTF grfFlags,
    IEnumIDList **ppenumIDList)
{
    return GetEnumNTDirectory(m_NtPath, ppenumIDList);
}

BOOL STDMETHODCALLTYPE CNtObjectFolder::IsSymLink(const NtPidlEntry * info)
{
    return info->objectType == SYMBOLICLINK_OBJECT;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::ResolveSymLink(
    const NtPidlEntry * info,
    LPITEMIDLIST * fullPidl)
{
    WCHAR wbLink[MAX_PATH] = { 0 };
    UNICODE_STRING link;
    RtlInitEmptyUnicodeString(&link, wbLink, sizeof(wbLink));

    *fullPidl = NULL;

    HRESULT hr = GetNTObjectSymbolicLinkTarget(m_NtPath, info->entryName, &link);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    WCHAR path[MAX_PATH];

    if (link.Length == 0)
        return E_UNEXPECTED;

    if (link.Buffer[1] == L':' && isalphaW(link.Buffer[0]))
    {
        StringCbCopyNW(path, sizeof(path), link.Buffer, link.Length);

        CComPtr<IShellFolder> psfDesktop;
        hr = SHGetDesktopFolder(&psfDesktop);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return psfDesktop->ParseDisplayName(NULL, NULL, path, NULL, fullPidl, NULL);
    }

    StringCbCopyW(path, sizeof(path), L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{845B0FB2-66E0-416B-8F91-314E23F7C12D}");
    PathAppend(path, link.Buffer);

    CComPtr<IShellFolder> psfDesktop;
    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    LPITEMIDLIST pidl;

    hr = psfDesktop->ParseDisplayName(NULL, NULL, path, NULL, &pidl, NULL);
    if (FAILED(hr))
        return hr;

    *fullPidl = pidl;

    DumpIdList(pidl);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::InternalBindToObject(
    PWSTR path,
    const NtPidlEntry * info,
    LPITEMIDLIST first,
    LPCITEMIDLIST rest,
    LPITEMIDLIST fullPidl,
    LPBC pbcReserved,
    IShellFolder** ppsfChild)
{

    if (info->objectType == KEY_OBJECT)
    {
        return ShellObjectCreatorInit<CRegistryFolder>(fullPidl, path, (HKEY) NULL, IID_PPV_ARG(IShellFolder, ppsfChild));
    }

    return ShellObjectCreatorInit<CNtObjectFolder>(fullPidl, path, IID_PPV_ARG(IShellFolder, ppsfChild));
}

// IPersistFolder
HRESULT STDMETHODCALLTYPE CNtObjectFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    m_shellPidl = ILClone(pidl);

    StringCbCopyW(m_NtPath, sizeof(m_NtPath), L"\\");

    return S_OK;
}

// Internal
HRESULT STDMETHODCALLTYPE CNtObjectFolder::Initialize(PCIDLIST_ABSOLUTE pidl, PCWSTR ntPath)
{
    m_shellPidl = ILClone(pidl);

    StringCbCopyW(m_NtPath, sizeof(m_NtPath), ntPath);

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
    case NTOBJECT_COLUMN_LINKTARGET:
        *pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT | SHCOLSTATE_SLOW;
        return S_OK;
    }

    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetDetailsEx(
    LPCITEMIDLIST pidl,
    const SHCOLUMNID *pscid,
    VARIANT *pv)
{
    const NtPidlEntry * info;

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
        }
        else if (IsEqualGUID(pscid->fmtid, GUID_NtObjectColumns))
        {
            if (pscid->pid == NTOBJECT_COLUMN_LINKTARGET && info->objectType == SYMBOLICLINK_OBJECT)
            {
                WCHAR wbLink[MAX_PATH] = { 0 };
                UNICODE_STRING link;
                RtlInitEmptyUnicodeString(&link, wbLink, sizeof(wbLink));

                HRESULT hr = GetNTObjectSymbolicLinkTarget(m_NtPath, info->entryName, &link);

                if (!FAILED_UNEXPECTEDLY(hr) && link.Length > 0)
                {
                    return MakeVariantString(pv, link.Buffer);
                }
            }

            V_VT(pv) = VT_EMPTY;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetDetailsOf(
    LPCITEMIDLIST pidl,
    UINT iColumn,
    SHELLDETAILS *psd)
{
    const NtPidlEntry * info;

    TRACE("GetDetailsOf\n");

    if (pidl)
    {
        HRESULT hr = GetInfoFromPidl(pidl, &info);
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
        case NTOBJECT_COLUMN_LINKTARGET:
        {
            psd->fmt = LVCFMT_LEFT;

            if (info->objectType == SYMBOLICLINK_OBJECT)
            {
                WCHAR wbLink[MAX_PATH] = { 0 };
                UNICODE_STRING link;
                RtlInitEmptyUnicodeString(&link, wbLink, sizeof(wbLink));

                HRESULT hr = GetNTObjectSymbolicLinkTarget(m_NtPath, info->entryName, &link);

                if (!FAILED_UNEXPECTEDLY(hr) && link.Length > 0)
                {
                    MakeStrRetFromString(link.Buffer, link.Length, &(psd->str));
                    return S_OK;
                }
            }

            MakeStrRetFromString(L"", &(psd->str));
            return S_OK;
        }
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
    case NTOBJECT_COLUMN_LINKTARGET:
        pscid->fmtid = GUID_NtObjectColumns;
        pscid->pid = NTOBJECT_COLUMN_LINKTARGET;
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT CNtObjectFolder::CompareIDs(LPARAM lParam, const NtPidlEntry * first, const NtPidlEntry * second)
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
    case NTOBJECT_COLUMN_NAME:
        return CompareName(lParam, first, second);

    case NTOBJECT_COLUMN_TYPE:
        return MAKE_COMPARE_HRESULT(second->objectType - first->objectType);

    case NTOBJECT_COLUMN_LINKTARGET:
        // Can't sort by link target yet
        return E_INVALIDARG;
    }

    DbgPrint("Unsupported sorting mode.\n");
    return E_INVALIDARG;
}

ULONG CNtObjectFolder::ConvertAttributes(const NtPidlEntry * entry, PULONG inMask)
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

BOOL CNtObjectFolder::IsFolder(const NtPidlEntry * info)
{
    return (info->objectType == DIRECTORY_OBJECT) ||
        (info->objectType == SYMBOLICLINK_OBJECT) ||
        (info->objectType == KEY_OBJECT);
}

HRESULT CNtObjectFolder::GetInfoFromPidl(LPCITEMIDLIST pcidl, const NtPidlEntry ** pentry)
{
    NtPidlEntry * entry = (NtPidlEntry*) &(pcidl->mkid);

    if (entry->cb < sizeof(NtPidlEntry))
    {
        DbgPrint("PCIDL too small %l (required %l)\n", entry->cb, sizeof(NtPidlEntry));
        return E_INVALIDARG;
    }

    if (entry->magic != NT_OBJECT_PIDL_MAGIC)
    {
        DbgPrint("PCIDL magic mismatch %04x (expected %04x)\n", entry->magic, NT_OBJECT_PIDL_MAGIC);
        return E_INVALIDARG;
    }

    *pentry = entry;
    return S_OK;
}