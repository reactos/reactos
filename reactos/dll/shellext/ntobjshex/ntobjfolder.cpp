/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\shellext\ntobjshex\ntobjns.cpp
 * PURPOSE:     NT Object Namespace shell extension
 * PROGRAMMERS: David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"
#include "ntobjenum.h"
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

class CNtObjectFolderExtractIcon :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IExtractIconW
{
    PCITEMID_CHILD m_pcidlChild;
    LPCWSTR m_NtPath;

public:
    CNtObjectFolderExtractIcon() :
        m_pcidlChild(NULL), m_NtPath(NULL)
    {

    }

    virtual ~CNtObjectFolderExtractIcon()
    {
        if (m_pcidlChild)
            ILFree((LPITEMIDLIST) m_pcidlChild);
    }

    HRESULT Initialize(LPCWSTR ntPath, UINT cidl, PCUITEMID_CHILD_ARRAY apidl)
    {
        m_NtPath = ntPath;
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

public:
    CNtObjectPidlManager() :
        m_ntPath(NULL)
    {
    }

    ~CNtObjectPidlManager()
    {
    }

    HRESULT Initialize(PWSTR ntPath)
    {
        m_ntPath = ntPath;

        return S_OK;
    }

    static HRESULT CompareIDs(LPARAM lParam, const NtPidlEntry * first, const NtPidlEntry * second)
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
            case NTOBJECT_COLUMN_NAME:
            {
                bool f1 = (first->objectType == KEY_OBJECT) || (first->objectType == DIRECTORY_OBJECT);
                bool f2 = (second->objectType == KEY_OBJECT) || (second->objectType == DIRECTORY_OBJECT);

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

                    int minlength = min(first->entryNameLength, second->entryNameLength);
                    if (minlength > 0)
                    {
                        int ord = memcmp(first->entryName, second->entryName, minlength);
                        if (ord != 0)
                            return MAKE_HRESULT(0, 0, (USHORT) ord);
                    }
                    return S_OK;
                }
                else
                {
                    int minlength = min(first->entryNameLength, second->entryNameLength);
                    if (minlength > 0)
                    {
                        int ord = StrCmpNW(first->entryName, second->entryName, minlength / sizeof(WCHAR));
                        if (ord != 0)
                            return MAKE_HRESULT(0, 0, (USHORT) ord);
                    }

                    if (second->entryNameLength > first->entryNameLength)
                        return MAKE_HRESULT(0, 0, (USHORT) 1);
                    if (second->entryNameLength < first->entryNameLength)
                        return MAKE_HRESULT(0, 0, (USHORT) -1);

                    return S_OK;
                }
            }
            case NTOBJECT_COLUMN_TYPE:
            {
                int ord = second->objectType - first->objectType;
                if (ord > 0)
                    return MAKE_HRESULT(0, 0, (USHORT) 1);
                if (ord < 0)
                    return MAKE_HRESULT(0, 0, (USHORT) -1);

                return S_OK;
            }
            case NTOBJECT_COLUMN_LINKTARGET:
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

    static HRESULT CompareIDs(LPARAM lParam, const NtPidlEntry * first, LPCITEMIDLIST pcidl)
    {
        LPCITEMIDLIST p = pcidl;
        NtPidlEntry * second = (NtPidlEntry*) &(p->mkid);
        if ((second->cb < sizeof(NtPidlEntry)) || (second->magic != NT_OBJECT_PIDL_MAGIC))
            return E_INVALIDARG;

        return CompareIDs(lParam, first, second);
    }

    static HRESULT CompareIDs(LPARAM lParam, LPCITEMIDLIST pcidl1, LPCITEMIDLIST pcidl2)
    {
        LPCITEMIDLIST p = pcidl1;
        NtPidlEntry * first = (NtPidlEntry*) &(p->mkid);
        if ((first->cb < sizeof(NtPidlEntry)) || (first->magic != NT_OBJECT_PIDL_MAGIC))
            return E_INVALIDARG;

        return CompareIDs(lParam, first, pcidl2);
    }

    static ULONG ConvertAttributes(const NtPidlEntry * entry, PULONG inMask)
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

    BOOL IsFolder(LPCITEMIDLIST pcidl)
    {
        NtPidlEntry * entry = (NtPidlEntry*) &(pcidl->mkid);
        if ((entry->cb < sizeof(NtPidlEntry)) || (entry->magic != NT_OBJECT_PIDL_MAGIC))
            return FALSE;

        return (entry->objectType == DIRECTORY_OBJECT) ||
            (entry->objectType == SYMBOLICLINK_OBJECT) ||
            (entry->objectType == KEY_OBJECT);
    }

    HRESULT GetInfoFromPidl(LPCITEMIDLIST pcidl, const NtPidlEntry ** pentry)
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
    if (m_shellPidl)
        ILFree(m_shellPidl);
    if (m_PidlManager)
        delete m_PidlManager;
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
    if (!ppidl)
        return E_POINTER;

    if (pchEaten)
        *pchEaten = 0;

    if (pdwAttributes)
        *pdwAttributes = 0;

    TRACE("CNtObjectFolder::ParseDisplayName name=%S (ntPath=%S)\n", lpszDisplayName, m_NtPath);

    const NtPidlEntry * info;
    IEnumIDList * it;
    HRESULT hr = GetEnumNTDirectory(m_NtPath, &it);
    if (FAILED(hr))
        return hr;

    while (TRUE)
    {
        hr = it->Next(1, ppidl, NULL);

        if (FAILED(hr))
            return hr;

        if (hr != S_OK)
            break;

        hr = m_PidlManager->GetInfoFromPidl(*ppidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        if (StrCmpW(info->entryName, lpszDisplayName) == 0)
            break;
    }

    if (hr != S_OK)
    {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (pchEaten || pdwAttributes)
    {
        if (pchEaten)
            *pchEaten = wcslen(info->entryName);

        if (pdwAttributes)
            *pdwAttributes = m_PidlManager->ConvertAttributes(info, pdwAttributes);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::EnumObjects(
    HWND hwndOwner,
    SHCONTF grfFlags,
    IEnumIDList **ppenumIDList)
{
    return GetEnumNTDirectory(m_NtPath, ppenumIDList);
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::BindToObject(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    void **ppvOut)
{
    const NtPidlEntry * info;

    if (IsEqualIID(riid, IID_IShellFolder))
    {
        HRESULT hr = m_PidlManager->GetInfoFromPidl(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        WCHAR path[MAX_PATH];

        StringCbCopyW(path, _countof(path), m_NtPath);

        PathAppendW(path, info->entryName);

        LPITEMIDLIST first = ILCloneFirst(pidl);
        LPCITEMIDLIST rest = ILGetNext(pidl);

        LPITEMIDLIST fullPidl = ILCombine(m_shellPidl, first);

        if (info->objectType == SYMBOLICLINK_OBJECT)
        {
            WCHAR wbLink[MAX_PATH] = { 0 };
            UNICODE_STRING link;
            RtlInitEmptyUnicodeString(&link, wbLink, sizeof(wbLink));

            hr = GetNTObjectSymbolicLinkTarget(m_NtPath, info->entryName, &link);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            if (link.Length > 0)
            {
                if (link.Buffer[1] == L':' && isalphaW(link.Buffer[0]))
                {
                    CComPtr<IShellFolder> psfDesktop;
                    hr = SHGetDesktopFolder(&psfDesktop);
                    if (FAILED_UNEXPECTEDLY(hr))
                        return hr;

                    hr = psfDesktop->ParseDisplayName(NULL, NULL, path, NULL, &first, NULL);
                    if (FAILED_UNEXPECTEDLY(hr))
                        return hr;

                    return psfDesktop->BindToObject(rest, pbcReserved, riid, ppvOut);
                }

                StringCbCopyW(path, _countof(path), L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{845B0FB2-66E0-416B-8F91-314E23F7C12D}");
                PathAppend(path, link.Buffer);

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
            hr = ShellObjectCreatorInit<CRegistryFolder>(fullPidl, path, (HKEY) NULL, IID_PPV_ARG(IShellFolder, &psfChild));
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
    const NtPidlEntry * info;

    TRACE("GetAttributesOf %d\n", cidl);

    if (cidl == 0)
    {
        *rgfInOut &= SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE;
        return S_OK;
    }

    for (int i = 0; i < (int) cidl; i++)
    {
        PCUITEMID_CHILD pidl = apidl[i];

        HRESULT hr = m_PidlManager->GetInfoFromPidl(pidl, &info);
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
    DWORD res;
    TRACE("GetUIObjectOf\n");

    if (IsEqualIID(riid, IID_IContextMenu) ||
        IsEqualIID(riid, IID_IContextMenu2) ||
        IsEqualIID(riid, IID_IContextMenu3))
    {
        CComPtr<IContextMenu> pcm;

        HKEY keys[1];

        int nkeys = _countof(keys);
        if (cidl == 1 && m_PidlManager->IsFolder(apidl[0]))
        {
            res = RegOpenKey(HKEY_CLASSES_ROOT, L"Folder", keys + 0);
            if (!NT_SUCCESS(res))
                return HRESULT_FROM_NT(res);
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
        return ShellObjectCreatorInit<CNtObjectFolderExtractIcon>(m_NtPath, cidl, apidl, riid, ppvOut);
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

            hr = pqa->Init(ASSOCF_INIT_DEFAULTTOFOLDER, L"NTObjShEx.NTDirectory", NULL, hwndOwner);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            return pqa->QueryInterface(riid, ppvOut);
        }
    }

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CNtObjectFolder::GetDisplayNameOf(
    LPCITEMIDLIST pidl,
    SHGDNF uFlags,
    STRRET *lpName)
{
    const NtPidlEntry * info;

    TRACE("GetDisplayNameOf %p\n", pidl);

    HRESULT hr = m_PidlManager->GetInfoFromPidl(pidl, &info);
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
    m_shellPidl = ILClone(pidl);

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
    case NTOBJECT_COLUMN_LINKTARGET:
        *pcsFlags = SHCOLSTATE_TYPE_STR | SHCOLSTATE_SLOW;
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
        HRESULT hr = m_PidlManager->GetInfoFromPidl(pidl, &info);
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
        HRESULT hr = m_PidlManager->GetInfoFromPidl(pidl, &info);
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
    case SFVM_COLUMNCLICK:
        return S_FALSE;
    case SFVM_BACKGROUNDENUM:
        return S_OK;
    }
    return E_NOTIMPL;
}

HRESULT CNtObjectFolder::DefCtxMenuCallback(IShellFolder * /*psf*/, HWND /*hwnd*/, IDataObject * /*pdtobj*/, UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/)
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
