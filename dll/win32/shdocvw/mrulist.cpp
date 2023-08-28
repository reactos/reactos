/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement shdocvw.dll
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
#include "shdocvw.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

struct SLOTITEMDATA
{
    // FIXME
};

class CMruBase : public IMruDataList
{
protected:
    LONG m_cRefs = 1;
    HKEY m_hKey = NULL;
    // FIXME: Add members

public:
    CMruBase()
    {
    }
    virtual ~CMruBase();

    // IUnknown
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

    // IMruDataList
    STDMETHODIMP InitData(
        UINT a2,
        UINT a3,
        HKEY hKey,
        LPCWSTR a5,
        void *a6) override;
    STDMETHODIMP AddData(BYTE *a2, UINT a3, UINT *a4) override;
    STDMETHODIMP FindData(const BYTE *a2, UINT a3, int *a4) override;
    STDMETHODIMP GetData(SLOTITEMDATA *a2, BYTE *a3, UINT a4) override;
    STDMETHODIMP QueryInfo(SLOTITEMDATA *a2, UINT *a3, UINT *a4) override;
    STDMETHODIMP Delete(int a2) override;

    static void *operator new(size_t size)
    {
        return LocalAlloc(LMEM_FIXED, size);
    }
    static void operator delete(void *ptr)
    {
        LocalFree(ptr);
    }
};

CMruBase::~CMruBase()
{
    if (m_hKey)
    {
        ::RegCloseKey(m_hKey);
        m_hKey = NULL;
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

STDMETHODIMP CMruBase::InitData(
    UINT a2,
    UINT a3,
    HKEY hKey,
    LPCWSTR a5,
    void *a6)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruBase::AddData(BYTE *a2, UINT a3, UINT *a4)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruBase::FindData(const BYTE *a2, UINT a3, int *a4)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruBase::GetData(SLOTITEMDATA *a2, BYTE *a3, UINT a4)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruBase::QueryInfo(SLOTITEMDATA *a2, UINT *a3, UINT *a4)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

STDMETHODIMP CMruBase::Delete(int a2)
{
    FIXME("Stub\n");
    return E_NOTIMPL;
}

class CMruPidlList
    : public CMruBase
    , public IMruPidlList
{
protected:
    HLOCAL m_hLocal = NULL;
    HANDLE m_hMutex = NULL;
    // FIXME: Add members

public:
    CMruPidlList() : CMruBase()
    {
    }

    ~CMruPidlList() override
    {
        m_hLocal = ::LocalFree(m_hLocal);
        if (m_hMutex)
        {
            ::CloseHandle(m_hMutex);
            m_hMutex = NULL;
        }
    }

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return CMruBase::AddRef();
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        return CMruBase::Release();
    }

    // IMruPidlList
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

    CMruPidlList *pMruList = new CMruPidlList();
    if (pMruList == NULL)
        return E_OUTOFMEMORY;

    *ppv = static_cast<IMruPidlList*>(pMruList);
    return S_OK;
}
