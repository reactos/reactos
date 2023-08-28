/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement MRU List of shdocvw.dll
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <objbase.h>
#include <oleauto.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shlwapi.h>
#include "shdocvw.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

extern "C" void __cxa_pure_virtual(void)
{
    ::DebugBreak();
}

class CMruBase
    : public IMruDataList
{
protected:
    LONG            m_cRefs         = 1;
    DWORD           m_dwFlags       = 0;
    BOOL            m_bFlag1        = FALSE;
    BOOL            m_bChecked      = FALSE;
    HKEY            m_hKey          = NULL;
    DWORD           m_cSlotRooms    = 0;
    DWORD           m_cSlots        = 0;
    SLOTCOMPARE     m_fnCompare     = NULL;
    SLOTITEMDATA *  m_pSlots        = NULL;

public:
    CMruBase()
    {
    }
    virtual ~CMruBase();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return ::InterlockedIncrement(&m_cRefs);
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        if (::InterlockedDecrement(&m_cRefs) == 0)
        {
            delete this;
            return 0;
        }
        return m_cRefs;
    }

    // IMruDataList methods
    STDMETHODIMP InitData(UINT cCapacity, UINT flags, HKEY hKey, LPCWSTR pszSubKey,
                          SLOTCOMPARE fnCompare) override;
    STDMETHODIMP AddData(const BYTE *pbData, DWORD cbData, UINT *piSlot) override;
    STDMETHODIMP FindData(const BYTE *pbData, DWORD cbData, UINT *piSlot) override;
    STDMETHODIMP GetData(UINT iSlot, BYTE *pbData, DWORD cbData) override;
    STDMETHODIMP QueryInfo(UINT iSlot, UINT *puSlot, DWORD *pcbData) override;
    STDMETHODIMP Delete(UINT iSlot) override;

    // Non-standard methods
    virtual HRESULT _IsEqual(const SLOTITEMDATA *pSlot, LPCITEMIDLIST pidl, UINT cbPidl) const;
    virtual HRESULT _DeleteValue(LPCWSTR pszValue);
    virtual HRESULT _InitSlots() = 0;
    virtual void _SaveSlots() = 0;
    virtual UINT _UpdateSlots(UINT iSlot) = 0;
    virtual void _SlotString(DWORD dwSlot, LPWSTR psz, DWORD cch) = 0;
    virtual HRESULT _GetSlot(UINT iSlot, UINT *puSlot) = 0;
    virtual HRESULT _RemoveSlot(UINT iSlot, UINT *uSlot) = 0;

    static void* operator new(size_t size)
    {
        return ::LocalAlloc(LPTR, size);
    }
    static void operator delete(void *ptr)
    {
        ::LocalFree(ptr);
    }
};

CMruBase::~CMruBase()
{
    if (m_hKey)
    {
        ::RegCloseKey(m_hKey);
        m_hKey = NULL;
    }

    if (m_pSlots)
    {
        for (UINT iSlot = 0; iSlot < m_cSlots; ++iSlot)
        {
            if (m_pSlots[iSlot].pidl)
            {
                ::LocalFree(m_pSlots[iSlot].pidl);
                m_pSlots[iSlot].pidl = NULL;
            }
        }

        ::LocalFree(m_pSlots);
        m_pSlots = NULL;
    }
}

STDMETHODIMP CMruBase::QueryInterface(REFIID riid, void **ppvObj)
{
    if (!ppvObj)
        return E_POINTER;
    if (IsEqualGUID(riid, IID_IMruDataList))
    {
        *ppvObj = static_cast<IMruDataList*>(this);
        AddRef();
        return S_OK;
    }
    ERR("%s: E_NOINTERFACE\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

STDMETHODIMP
CMruBase::InitData(
    UINT cCapacity,
    UINT flags,
    HKEY hKey,
    LPCWSTR pszSubKey,
    SLOTCOMPARE fnCompare)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruBase::AddData(const BYTE *pbData, DWORD cbData, UINT *piSlot)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruBase::FindData(const BYTE *pbData, DWORD cbData, UINT *piSlot)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruBase::GetData(UINT iSlot, BYTE *pbData, DWORD cbData)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruBase::QueryInfo(UINT iSlot, UINT *puSlot, DWORD *pcbData)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruBase::Delete(UINT iSlot)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

HRESULT CMruBase::_IsEqual(const SLOTITEMDATA *pSlot, LPCITEMIDLIST pidl, UINT cbPidl) const
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

HRESULT CMruBase::_DeleteValue(LPCWSTR pszValue)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

class CMruLongList
    : public CMruBase
{
protected:
    UINT *m_puSlotData = NULL;

    void _ImportShortList();

    HRESULT _InitSlots() override;
    void _SaveSlots() override;
    UINT _UpdateSlots(UINT iSlot) override;
    void _SlotString(DWORD dwSlot, LPWSTR psz, DWORD cch) override;
    HRESULT _GetSlot(UINT iSlot, UINT *puSlot) override;
    HRESULT _RemoveSlot(UINT iSlot, UINT *uSlot) override;

public:
    CMruLongList()
    {
    }

    ~CMruLongList() override
    {
        if (m_puSlotData)
        {
            ::LocalFree(m_puSlotData);
            m_puSlotData = NULL;
        }
    }
};

HRESULT CMruLongList::_InitSlots()
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

void CMruLongList::_SaveSlots()
{
    FIXME("Stub\n");
}

UINT CMruLongList::_UpdateSlots(UINT iSlot)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

void CMruLongList::_SlotString(DWORD dwSlot, LPWSTR psz, DWORD cch)
{
    FIXME("Stub\n");
}

HRESULT CMruLongList::_GetSlot(UINT iSlot, UINT *puSlot)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

HRESULT CMruLongList::_RemoveSlot(UINT iSlot, UINT *uSlot)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

void CMruLongList::_ImportShortList()
{
    FIXME("Stub\n");
}

EXTERN_C HRESULT
CMruLongList_CreateInstance(DWORD dwUnused1, void **ppv, DWORD dwUnused3)
{
    UNREFERENCED_PARAMETER(dwUnused1);
    UNREFERENCED_PARAMETER(dwUnused3);

    CMruLongList *pMruList = new CMruLongList();
    *ppv = static_cast<IMruDataList*>(pMruList);
    return S_OK;
}

class CMruNode
    : public CMruLongList
{
protected:
    UINT m_uSlotData = 0;
    CMruNode *m_pParent = NULL;
    IShellFolder *m_pShellFolder = NULL;

public:
    CMruNode() { }
    CMruNode(CMruNode *pParent, UINT uSlotData);
    ~CMruNode() override;

    CMruNode *GetParent();
};

CMruNode::CMruNode(CMruNode *pParent, UINT uSlotData)
{
    m_uSlotData = uSlotData;
    m_pParent = pParent;
    pParent->AddRef();
}

CMruNode::~CMruNode()
{
    if (m_pParent)
    {
        m_pParent->Release();
        m_pParent = NULL;
    }

    if (m_pShellFolder)
    {
        m_pShellFolder->Release();
        m_pShellFolder = NULL;
    }
}

CMruNode *CMruNode::GetParent()
{
    if (m_pParent)
        m_pParent->AddRef();
    return m_pParent;
}

class CMruPidlList
    : public IMruPidlList
    , public CMruNode
{
protected:
    LPBYTE m_pbSlots = NULL;
    DWORD m_cbSlots = 0;
    HANDLE m_hMutex = NULL;

    BOOL _LoadNodeSlots()
    {
        DWORD cbSlots = m_cbSlots;
        if (SHGetValueW(m_hKey, NULL, L"NodeSlots", NULL, m_pbSlots, &cbSlots) != ERROR_SUCCESS)
            return FALSE;
        m_cbSlots = cbSlots;
        return TRUE;
    }

    void _SaveNodeSlots()
    {
        SHSetValueW(m_hKey, NULL, L"NodeSlots", REG_BINARY, m_pbSlots, m_cbSlots);
    }

public:
    CMruPidlList()
    {
    }

    virtual ~CMruPidlList()
    {
        m_pbSlots = (LPBYTE)::LocalFree(m_pbSlots);
        if (m_hMutex)
        {
            ::CloseHandle(m_hMutex);
            m_hMutex = NULL;
        }
    }

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return CMruBase::AddRef();
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        return CMruBase::Release();
    }

    // IMruPidlList methods
    STDMETHODIMP InitList(UINT cMRUSize, HKEY hKey, LPCWSTR pszName) override;
    STDMETHODIMP UsePidl(LPCITEMIDLIST pidl, UINT *puSlots) override;
    STDMETHODIMP QueryPidl(
        LPCITEMIDLIST pidl,
        UINT cSlots,
        UINT *puSlots,
        UINT *pcSlots) override;
    STDMETHODIMP PruneKids(LPCITEMIDLIST pidl) override;
};

STDMETHODIMP CMruPidlList::QueryInterface(REFIID riid, void **ppvObj)
{
    if (!ppvObj)
        return E_POINTER;

    if (::IsEqualGUID(riid, IID_IMruPidlList) || ::IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObj = static_cast<IMruPidlList*>(this);
        AddRef();
        return S_OK;
    }

    ERR("%s: E_NOINTERFACE\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

STDMETHODIMP CMruPidlList::InitList(UINT cMRUSize, HKEY hKey, LPCWSTR pszName)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruPidlList::UsePidl(LPCITEMIDLIST pidl, UINT *puSlots)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruPidlList::QueryPidl(
    LPCITEMIDLIST pidl,
    UINT cSlots,
    UINT *puSlots,
    UINT *pcSlots)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruPidlList::PruneKids(LPCITEMIDLIST pidl)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

EXTERN_C HRESULT CMruPidlList_CreateInstance(DWORD dwUnused1, void **ppv, DWORD dwUnused3)
{
    UNREFERENCED_PARAMETER(dwUnused1);
    UNREFERENCED_PARAMETER(dwUnused3);

    *ppv = NULL;

    CMruPidlList *pMruList = new CMruPidlList();
    if (pMruList == NULL)
        return E_OUTOFMEMORY;

    *ppv = static_cast<IMruPidlList*>(pMruList);
    return S_OK;
}
