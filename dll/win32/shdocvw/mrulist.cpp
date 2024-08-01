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
#include <shlwapi_undoc.h>
#include <strsafe.h>
#include "shdocvw.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

class CSafeMutex;
class CMruBase;
    class CMruShortList;
    class CMruLongList;
        class CMruNode;
            class CMruPidlList;
class CMruClassFactory;

extern "C" void __cxa_pure_virtual(void)
{
    ERR("__cxa_pure_virtual\n");
    ::DebugBreak();
}

BOOL IEILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL bUnknown)
{
    UINT cb1 = ILGetSize(pidl1), cb2 = ILGetSize(pidl2);
    if (cb1 == cb2 && memcmp(pidl1, pidl2, cb1) == 0)
        return TRUE;

    FIXME("%p, %p\n", pidl1, pidl2);
    return FALSE;
}

// The flags for SLOTITEMDATA.dwFlags
#define SLOT_LOADED         0x1
#define SLOT_SET            0x2

// The flags for CMruBase.m_dwFlags
#define COMPARE_BY_MEMCMP       0x0
#define COMPARE_BY_STRCMPIW     0x1
#define COMPARE_BY_STRCMPW      0x2
#define COMPARE_BY_IEILISEQUAL  0x3
#define COMPARE_BY_MASK         0xF

class CSafeMutex
{
protected:
    HANDLE m_hMutex;

public:
    CSafeMutex() : m_hMutex(NULL)
    {
    }
    ~CSafeMutex()
    {
        if (m_hMutex)
        {
            ::ReleaseMutex(m_hMutex);
            m_hMutex = NULL;
        }
    }

    HRESULT Enter(HANDLE hMutex)
    {
        DWORD wait = ::WaitForSingleObject(hMutex, 500);
        if (wait != WAIT_OBJECT_0)
            return E_FAIL;

        m_hMutex = hMutex;
        return S_OK;
    }
};

class CMruBase
    : public IMruDataList
{
protected:
    LONG            m_cRefs         = 1;        // Reference count
    DWORD           m_dwFlags       = 0;        // The COMPARE_BY_... flags
    BOOL            m_bNeedSave     = FALSE;    // The flag that indicates whether it needs saving
    BOOL            m_bChecked      = FALSE;    // The checked flag
    HKEY            m_hKey          = NULL;     // A registry key
    DWORD           m_cSlotRooms    = 0;        // Rooms for slots
    DWORD           m_cSlots        = 0;        // The # of slots
    SLOTCOMPARE     m_fnCompare     = NULL;     // The comparison function
    SLOTITEMDATA *  m_pSlots        = NULL;     // Slot data

    HRESULT _LoadItem(UINT iSlot);
    HRESULT _AddItem(UINT iSlot, LPCVOID pvData, DWORD cbData);
    HRESULT _GetItem(UINT iSlot, SLOTITEMDATA **ppItem);
    void _DeleteItem(UINT iSlot);

    HRESULT _GetSlotItem(UINT iSlot, SLOTITEMDATA **ppItem);
    void _CheckUsedSlots();
    HRESULT _UseEmptySlot(UINT *piSlot);

public:
    CMruBase();
    virtual ~CMruBase();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return ::InterlockedIncrement(&m_cRefs);
    }
    STDMETHODIMP_(ULONG) Release() override;

    // IMruDataList methods
    STDMETHODIMP InitData(UINT cCapacity, UINT flags, HKEY hKey,
                          LPCWSTR pszSubKey OPTIONAL,
                          SLOTCOMPARE fnCompare OPTIONAL) override;
    STDMETHODIMP AddData(LPCVOID pvData, DWORD cbData, UINT *piSlot) override;
    STDMETHODIMP FindData(LPCVOID pvData, DWORD cbData, UINT *piSlot) override;
    STDMETHODIMP GetData(UINT iSlot, LPVOID pvData, DWORD cbData) override;
    STDMETHODIMP QueryInfo(UINT iSlot, UINT *piGotSlot, DWORD *pcbData) override;
    STDMETHODIMP Delete(UINT iSlot) override;

    // Non-standard methods
    virtual BOOL _IsEqual(const SLOTITEMDATA *pItem, LPCVOID pvData, UINT cbData) const;
    virtual DWORD _DeleteValue(LPCWSTR pszValue);
    virtual HRESULT _InitSlots() = 0;
    virtual void _SaveSlots() = 0;
    virtual UINT _UpdateSlots(UINT iSlot) = 0;
    virtual void _SlotString(UINT iSlot, LPWSTR psz, DWORD cch) = 0;
    virtual HRESULT _GetSlot(UINT iSlot, UINT *puSlot) = 0;
    virtual HRESULT _RemoveSlot(UINT iSlot, UINT *puSlot) = 0;

    static void* operator new(size_t size)
    {
        return ::LocalAlloc(LPTR, size);
    }
    static void operator delete(void *ptr)
    {
        ::LocalFree(ptr);
    }
};

CMruBase::CMruBase()
{
    ::InterlockedIncrement(&SHDOCVW_refCount);
}

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
            m_pSlots[iSlot].pvData = ::LocalFree(m_pSlots[iSlot].pvData);
        }

        m_pSlots = (SLOTITEMDATA*)::LocalFree(m_pSlots);
    }

    ::InterlockedDecrement(&SHDOCVW_refCount);
}

STDMETHODIMP CMruBase::QueryInterface(REFIID riid, void **ppvObj)
{
    if (!ppvObj)
        return E_POINTER;
    if (IsEqualGUID(riid, IID_IMruDataList) || IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObj = static_cast<IMruDataList*>(this);
        AddRef();
        return S_OK;
    }
    ERR("%s: E_NOINTERFACE\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CMruBase::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        _SaveSlots();
        delete this;
        return 0;
    }
    return m_cRefs;
}

HRESULT CMruBase::_LoadItem(UINT iSlot)
{
    DWORD cbData;
    WCHAR szValue[12];

    SLOTITEMDATA *pItem = &m_pSlots[iSlot];
    _SlotString(iSlot, szValue, _countof(szValue));

    if (SHGetValueW(m_hKey, NULL, szValue, NULL, NULL, &cbData) == ERROR_SUCCESS &&
        cbData > 0)
    {
        pItem->pvData = ::LocalAlloc(LPTR, cbData);
        if (pItem->pvData)
        {
            pItem->cbData = cbData;
            if (SHGetValueW(m_hKey, NULL, szValue, NULL, pItem->pvData, &cbData) != ERROR_SUCCESS)
                pItem->pvData = ::LocalFree(pItem->pvData);
        }
    }

    pItem->dwFlags |= SLOT_LOADED;
    if (!pItem->pvData)
        return E_FAIL;

    return S_OK;
}

HRESULT CMruBase::_GetSlotItem(UINT iSlot, SLOTITEMDATA **ppItem)
{
    if (!(m_pSlots[iSlot].dwFlags & SLOT_LOADED))
        _LoadItem(iSlot);

    SLOTITEMDATA *pItem = &m_pSlots[iSlot];
    if (!pItem->pvData)
        return E_OUTOFMEMORY;

    *ppItem = pItem;
    return S_OK;
}

HRESULT CMruBase::_GetItem(UINT iSlot, SLOTITEMDATA **ppItem)
{
    HRESULT hr = _GetSlot(iSlot, &iSlot);
    if (FAILED(hr))
        return hr;
    return _GetSlotItem(iSlot, ppItem);
}

void CMruBase::_DeleteItem(UINT iSlot)
{
    WCHAR szBuff[12];

    _SlotString(iSlot, szBuff, _countof(szBuff));
    _DeleteValue(szBuff);

    m_pSlots[iSlot].pvData = ::LocalFree(m_pSlots[iSlot].pvData);
}

void CMruBase::_CheckUsedSlots()
{
    UINT iGotSlot;
    for (UINT iSlot = 0; iSlot < m_cSlots; ++iSlot)
        _GetSlot(iSlot, &iGotSlot);

    m_bChecked = TRUE;
}

HRESULT CMruBase::_AddItem(UINT iSlot, LPCVOID pvData, DWORD cbData)
{
    SLOTITEMDATA *pItem = &m_pSlots[iSlot];

    WCHAR szBuff[12];
    _SlotString(iSlot, szBuff, _countof(szBuff));

    if (SHSetValueW(m_hKey, NULL, szBuff, REG_BINARY, pvData, cbData) != ERROR_SUCCESS)
        return E_OUTOFMEMORY;

    if (cbData >= pItem->cbData || !pItem->pvData)
    {
        ::LocalFree(pItem->pvData);
        pItem->pvData = ::LocalAlloc(LPTR, cbData);
    }

    if (!pItem->pvData)
        return E_FAIL;

    pItem->cbData = cbData;
    pItem->dwFlags = (SLOT_LOADED | SLOT_SET);
    CopyMemory(pItem->pvData, pvData, cbData);
    return S_OK;
}

STDMETHODIMP
CMruBase::InitData(
    UINT cCapacity,
    UINT flags,
    HKEY hKey,
    LPCWSTR pszSubKey OPTIONAL,
    SLOTCOMPARE fnCompare OPTIONAL)
{
    m_dwFlags = flags;
    m_fnCompare = fnCompare;
    m_cSlotRooms = cCapacity;

    if (pszSubKey)
        ::RegCreateKeyExWrapW(hKey, pszSubKey, 0, NULL, 0, MAXIMUM_ALLOWED, NULL, &m_hKey, NULL);
    else
        m_hKey = SHRegDuplicateHKey(hKey);

    if (!m_hKey)
        return E_FAIL;

    m_pSlots = (SLOTITEMDATA*)::LocalAlloc(LPTR, m_cSlotRooms * sizeof(SLOTITEMDATA));
    if (!m_pSlots)
        return E_OUTOFMEMORY;

    return _InitSlots();
}

STDMETHODIMP CMruBase::AddData(LPCVOID pvData, DWORD cbData, UINT *piSlot)
{
    UINT iSlot;
    HRESULT hr = FindData(pvData, cbData, &iSlot);
    if (FAILED(hr))
    {
        iSlot = _UpdateSlots(m_cSlots);
        hr = _AddItem(iSlot, pvData, cbData);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        iSlot = _UpdateSlots(iSlot);
        hr = S_OK;
    }

    if (piSlot)
        *piSlot = iSlot;

    return hr;
}

STDMETHODIMP CMruBase::FindData(LPCVOID pvData, DWORD cbData, UINT *piSlot)
{
    if (m_cSlots <= 0)
        return E_FAIL;

    UINT iSlot = 0;
    SLOTITEMDATA *pItem;
    while (FAILED(_GetItem(iSlot, &pItem)) || !_IsEqual(pItem, pvData, cbData))
    {
        if (++iSlot >= m_cSlots)
            return E_FAIL;
    }

    *piSlot = iSlot;
    return S_OK;
}

STDMETHODIMP CMruBase::GetData(UINT iSlot, LPVOID pvData, DWORD cbData)
{
    SLOTITEMDATA *pItem;
    HRESULT hr = _GetItem(iSlot, &pItem);
    if (FAILED(hr))
        return hr;

    if (cbData < pItem->cbData)
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    CopyMemory(pvData, pItem->pvData, pItem->cbData);
    return hr;
}

STDMETHODIMP CMruBase::QueryInfo(UINT iSlot, UINT *piGotSlot, DWORD *pcbData)
{
    UINT iGotSlot;
    HRESULT hr = _GetSlot(iSlot, &iGotSlot);
    if (FAILED(hr))
        return hr;

    if (piGotSlot)
        *piGotSlot = iGotSlot;

    if (pcbData)
    {
        SLOTITEMDATA *pItem;
        hr = _GetSlotItem(iGotSlot, &pItem);
        if (SUCCEEDED(hr))
            *pcbData = pItem->cbData;
    }

    return hr;
}

STDMETHODIMP CMruBase::Delete(UINT iSlot)
{
    UINT uSlot;
    HRESULT hr = _RemoveSlot(iSlot, &uSlot);
    if (FAILED(hr))
        return hr;

    _DeleteItem(uSlot);
    return hr;
}

BOOL CMruBase::_IsEqual(const SLOTITEMDATA *pItem, LPCVOID pvData, UINT cbData) const
{
    if (m_fnCompare)
        return m_fnCompare(pvData, pItem->pvData, cbData) == 0;

    switch (m_dwFlags & COMPARE_BY_MASK)
    {
        case COMPARE_BY_MEMCMP:
            if (pItem->cbData != cbData)
                return FALSE;
            return memcmp(pvData, pItem->pvData, cbData) == 0;

        case COMPARE_BY_STRCMPIW:
            return StrCmpIW((LPCWSTR)pvData, (LPCWSTR)pItem->pvData) == 0;

        case COMPARE_BY_STRCMPW:
            return StrCmpW((LPCWSTR)pvData, (LPCWSTR)pItem->pvData) == 0;

        case COMPARE_BY_IEILISEQUAL:
            return IEILIsEqual((LPCITEMIDLIST)pvData, (LPCITEMIDLIST)pItem->pvData, FALSE);

        default:
            ERR("0x%08X\n", m_dwFlags);
            return FALSE;
    }
}

DWORD CMruBase::_DeleteValue(LPCWSTR pszValue)
{
    return SHDeleteValueW(m_hKey, NULL, pszValue);
}

HRESULT CMruBase::_UseEmptySlot(UINT *piSlot)
{
    if (!m_bChecked)
        _CheckUsedSlots();

    if (!m_cSlotRooms)
        return E_FAIL;

    UINT iSlot = 0;
    for (SLOTITEMDATA *pItem = m_pSlots; (pItem->dwFlags & SLOT_SET); ++pItem)
    {
        if (++iSlot >= m_cSlotRooms)
            return E_FAIL;
    }

    m_pSlots[iSlot].dwFlags |= SLOT_SET;
    *piSlot = iSlot;
    ++m_cSlots;

    return S_OK;
}

class CMruShortList
    : public CMruBase
{
protected:
    LPWSTR m_pszSlotData = NULL;

    HRESULT _InitSlots() override;
    void _SaveSlots() override;
    UINT _UpdateSlots(UINT iSlot) override;
    void _SlotString(UINT iSlot, LPWSTR psz, DWORD cch) override;
    HRESULT _GetSlot(UINT iSlot, UINT *puSlot) override;
    HRESULT _RemoveSlot(UINT iSlot, UINT *puSlot) override;
    friend class CMruLongList;

public:
    CMruShortList()
    {
    }

    ~CMruShortList() override
    {
        m_pszSlotData = (LPWSTR)::LocalFree(m_pszSlotData);
    }
};

HRESULT CMruShortList::_InitSlots()
{
    DWORD cbData = (m_cSlotRooms + 1) * sizeof(WCHAR);
    m_pszSlotData = (LPWSTR)LocalAlloc(LPTR, cbData);
    if (!m_pszSlotData)
        return E_OUTOFMEMORY;

    if (SHGetValueW(m_hKey, NULL, L"MRUList", NULL, m_pszSlotData, &cbData) == ERROR_SUCCESS)
        m_cSlots = (cbData / sizeof(WCHAR)) - 1;

    m_pszSlotData[m_cSlots] = UNICODE_NULL;
    return S_OK;
}

void CMruShortList::_SaveSlots()
{
    if (m_bNeedSave)
    {
        DWORD cbData = (m_cSlots + 1) * sizeof(WCHAR);
        SHSetValueW(m_hKey, NULL, L"MRUList", REG_SZ, m_pszSlotData, cbData);
        m_bNeedSave = FALSE;
    }
}

// NOTE: MRUList uses lowercase alphabet for history of most recently used items.
UINT CMruShortList::_UpdateSlots(UINT iSlot)
{
    UINT iData, cDataToMove = iSlot;

    if (iSlot == m_cSlots)
    {
        if (SUCCEEDED(_UseEmptySlot(&iData)))
        {
            ++cDataToMove;
        }
        else
        {
            // This code is getting the item index from a lowercase letter.
            iData = m_pszSlotData[m_cSlots - 1] - L'a';
            --cDataToMove;
        }
    }
    else
    {
        iData = m_pszSlotData[iSlot] - L'a';
    }

    if (cDataToMove)
    {
        MoveMemory(m_pszSlotData + 1, m_pszSlotData, cDataToMove * sizeof(WCHAR));
        m_pszSlotData[0] = (WCHAR)(L'a' + iData);
        m_bNeedSave = TRUE;
    }

    return iData;
}

void CMruShortList::_SlotString(UINT iSlot, LPWSTR psz, DWORD cch)
{
    if (cch >= 2)
    {
        psz[0] = (WCHAR)(L'a' + iSlot);
        psz[1] = UNICODE_NULL;
    }
}

HRESULT CMruShortList::_GetSlot(UINT iSlot, UINT *puSlot)
{
    if (iSlot >= m_cSlots)
        return E_FAIL;

    UINT iData = m_pszSlotData[iSlot] - L'a';
    if (iData >= m_cSlotRooms)
        return E_FAIL;

    *puSlot = iData;
    m_pSlots[iData].dwFlags |= SLOT_SET;
    return S_OK;
}

HRESULT CMruShortList::_RemoveSlot(UINT iSlot, UINT *puSlot)
{
    HRESULT hr = _GetSlot(iSlot, puSlot);
    if (FAILED(hr))
        return hr;

    MoveMemory(&m_pszSlotData[iSlot], &m_pszSlotData[iSlot + 1], (m_cSlots - iSlot) * sizeof(WCHAR));
    --m_cSlots;
    m_pSlots->dwFlags &= ~SLOT_SET;
    m_bNeedSave = TRUE;

    return hr;
}

class CMruLongList
    : public CMruBase
{
protected:
    UINT *m_puSlotData = NULL;      // The slot data

    void _ImportShortList();

    HRESULT _InitSlots() override;
    void _SaveSlots() override;
    UINT _UpdateSlots(UINT iSlot) override;
    void _SlotString(UINT iSlot, LPWSTR psz, DWORD cch) override;
    HRESULT _GetSlot(UINT iSlot, UINT *puSlot) override;
    HRESULT _RemoveSlot(UINT iSlot, UINT *puSlot) override;

public:
    CMruLongList()
    {
    }

    ~CMruLongList() override
    {
        m_puSlotData = (UINT*)::LocalFree(m_puSlotData);
    }
};

HRESULT CMruLongList::_InitSlots()
{
    DWORD cbData = (m_cSlotRooms + 1) * sizeof(UINT);
    m_puSlotData = (UINT*)LocalAlloc(LPTR, cbData);
    if (!m_puSlotData)
        return E_OUTOFMEMORY;

    if (SHGetValueW(m_hKey, NULL, L"MRUListEx", NULL, m_puSlotData, &cbData) == ERROR_SUCCESS)
        m_cSlots = (cbData / sizeof(UINT)) - 1;
    else
        _ImportShortList();

    m_puSlotData[m_cSlots] = MAXDWORD;
    return S_OK;
}

void CMruLongList::_SaveSlots()
{
    if (m_bNeedSave)
    {
        SHSetValueW(m_hKey, NULL, L"MRUListEx", REG_BINARY, m_puSlotData,
                    (m_cSlots + 1) * sizeof(UINT));
        m_bNeedSave = FALSE;
    }
}

UINT CMruLongList::_UpdateSlots(UINT iSlot)
{
    UINT cSlotsToMove, uSlotData;

    cSlotsToMove = iSlot;
    if (iSlot == m_cSlots)
    {
        if (SUCCEEDED(_UseEmptySlot(&uSlotData)))
        {
            ++cSlotsToMove;
        }
        else
        {
            uSlotData = m_puSlotData[m_cSlots - 1];
            --cSlotsToMove;
        }
    }
    else
    {
        uSlotData = m_puSlotData[iSlot];
    }

    if (cSlotsToMove > 0)
    {
        MoveMemory(m_puSlotData + 1, m_puSlotData, cSlotsToMove * sizeof(UINT));
        m_puSlotData[0] = uSlotData;
        m_bNeedSave = TRUE;
    }

    return uSlotData;
}

void CMruLongList::_SlotString(UINT iSlot, LPWSTR psz, DWORD cch)
{
    StringCchPrintfW(psz, cch, L"%d", iSlot);
}

HRESULT CMruLongList::_GetSlot(UINT iSlot, UINT *puSlot)
{
    if (iSlot >= m_cSlots)
        return E_FAIL;

    UINT uSlotData = m_puSlotData[iSlot];
    if (uSlotData >= m_cSlotRooms)
        return E_FAIL;

    *puSlot = uSlotData;
    m_pSlots[uSlotData].dwFlags |= SLOT_SET;
    return S_OK;
}

HRESULT CMruLongList::_RemoveSlot(UINT iSlot, UINT *puSlot)
{
    HRESULT hr = _GetSlot(iSlot, puSlot);
    if (FAILED(hr))
        return hr;

    MoveMemory(&m_puSlotData[iSlot], &m_puSlotData[iSlot + 1], (m_cSlots - iSlot) * sizeof(UINT));
    --m_cSlots;
    m_pSlots[0].dwFlags &= ~SLOT_SET;
    m_bNeedSave = TRUE;

    return hr;
}

void CMruLongList::_ImportShortList()
{
    CMruShortList *pShortList = new CMruShortList();
    if (!pShortList)
        return;

    HRESULT hr = pShortList->InitData(m_cSlotRooms, 0, m_hKey, NULL, NULL);
    if (SUCCEEDED(hr))
    {
        for (;;)
        {
            UINT iSlot;
            hr = pShortList->_GetSlot(m_cSlots, &iSlot);
            if (FAILED(hr))
                break;

            SLOTITEMDATA *pItem;
            hr = pShortList->_GetSlotItem(iSlot, &pItem);
            if (FAILED(hr))
                break;

            _AddItem(iSlot, pItem->pvData, pItem->cbData);
            pShortList->_DeleteItem(iSlot);

            m_puSlotData[m_cSlots++] = iSlot;
        }

        m_bNeedSave = TRUE;
    }

    SHDeleteValueW(m_hKey, NULL, L"MRUList");
    pShortList->Release();
}

EXTERN_C HRESULT
CMruLongList_CreateInstance(DWORD_PTR dwUnused1, void **ppv, DWORD_PTR dwUnused3)
{
    UNREFERENCED_PARAMETER(dwUnused1);
    UNREFERENCED_PARAMETER(dwUnused3);

    TRACE("%p %p %p\n", dwUnused1, ppv, dwUnused3);

    if (!ppv)
        return E_POINTER;

    CMruLongList *pMruList = new CMruLongList();
    *ppv = static_cast<IMruDataList*>(pMruList);
    TRACE("%p\n", *ppv);

    return S_OK;
}

class CMruNode
    : public CMruLongList
{
protected:
    UINT m_iSlot = 0;                       // The slot index
    CMruNode *m_pParent = NULL;             // The parent
    IShellFolder *m_pShellFolder = NULL;    // The shell folder

    BOOL _InitLate();
    BOOL _IsEqual(SLOTITEMDATA *pItem, LPCVOID pvData, UINT cbData);
    DWORD _DeleteValue(LPCWSTR pszValue) override;

    HRESULT _CreateNode(UINT iSlot, CMruNode **ppNewNode);
    HRESULT _AddPidl(UINT iSlot, LPCITEMIDLIST pidl);
    HRESULT _FindPidl(LPCITEMIDLIST pidl, UINT *piSlot);
    HRESULT _GetPidlSlot(LPCITEMIDLIST pidl, BOOL bAdd, UINT *piSlot);

public:
    CMruNode() { }
    CMruNode(CMruNode *pParent, UINT iSlot);
    ~CMruNode() override;

    CMruNode *GetParent();

    HRESULT BindToSlot(UINT iSlot, IShellFolder **ppSF);
    HRESULT GetNode(BOOL bAdd, LPCITEMIDLIST pidl, CMruNode **pNewNode);
    HRESULT GetNodeSlot(UINT *pnNodeSlot);
    HRESULT SetNodeSlot(UINT nNodeSlot);

    HRESULT RemoveLeast(UINT *pnNodeSlot);
    HRESULT Clear(CMruPidlList *pList);
};

CMruNode::CMruNode(CMruNode *pParent, UINT iSlot)
{
    m_iSlot = iSlot;
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

HRESULT CMruNode::_CreateNode(UINT iSlot, CMruNode **ppNewNode)
{
    CMruNode *pNewNode = new CMruNode(this, iSlot);
    if (!pNewNode)
        return E_OUTOFMEMORY;

    WCHAR szSubKey[12];
    _SlotString(iSlot, szSubKey, _countof(szSubKey));

    HRESULT hr = pNewNode->InitData(m_cSlotRooms, 0, m_hKey, szSubKey, NULL);
    if (FAILED(hr))
        pNewNode->Release();
    else
        *ppNewNode = pNewNode;

    return hr;
}

HRESULT CMruNode::GetNode(BOOL bAdd, LPCITEMIDLIST pidl, CMruNode **ppNewNode)
{
    if (!pidl || !pidl->mkid.cb)
    {
        *ppNewNode = this;
        AddRef();
        return S_OK;
    }

    if (!_InitLate())
        return E_FAIL;

    UINT iSlot;
    HRESULT hr = _GetPidlSlot(pidl, bAdd, &iSlot);
    if (FAILED(hr))
    {
        if (!bAdd)
        {
            *ppNewNode = this;
            AddRef();
            return S_FALSE;
        }
        return hr;
    }

    CMruNode *pNewNode;
    hr = _CreateNode(iSlot, &pNewNode);
    if (SUCCEEDED(hr))
    {
        _SaveSlots();

        LPCITEMIDLIST pidl2 = (LPCITEMIDLIST)((LPBYTE)pidl + pidl->mkid.cb);
        pNewNode->GetNode(bAdd, pidl2, ppNewNode);
        pNewNode->Release();
    }

    return hr;
}

HRESULT CMruNode::BindToSlot(UINT iSlot, IShellFolder **ppSF)
{
    SLOTITEMDATA *pItem;
    HRESULT hr = _GetSlotItem(iSlot, &pItem);
    if (FAILED(hr))
        return hr;

    return m_pShellFolder->BindToObject((LPITEMIDLIST)pItem->pvData,
                                        NULL,
                                        IID_IShellFolder,
                                        (void **)ppSF);
}

BOOL CMruNode::_InitLate()
{
    if (!m_pShellFolder)
    {
        if (m_pParent)
            m_pParent->BindToSlot(m_iSlot, &m_pShellFolder);
        else
            SHGetDesktopFolder(&m_pShellFolder);
    }
    return !!m_pShellFolder;
}

BOOL CMruNode::_IsEqual(SLOTITEMDATA *pItem, LPCVOID pvData, UINT cbData)
{
    return m_pShellFolder->CompareIDs(0x10000000,
                                      (LPITEMIDLIST)pItem->pvData,
                                      (LPCITEMIDLIST)pvData) == 0;
}

HRESULT CMruNode::GetNodeSlot(UINT *pnNodeSlot)
{
    DWORD dwData, cbData = sizeof(dwData);
    DWORD error = SHGetValueW(m_hKey, NULL, L"NodeSlot", NULL, &dwData, (pnNodeSlot ? &cbData : NULL));
    if (error != ERROR_SUCCESS)
        return E_FAIL;
    *pnNodeSlot = (UINT)dwData;
    return S_OK;
}

HRESULT CMruNode::SetNodeSlot(UINT nNodeSlot)
{
    DWORD dwData = nNodeSlot;
    if (SHSetValueW(m_hKey, NULL, L"NodeSlot", REG_DWORD, &dwData, sizeof(dwData)) != ERROR_SUCCESS)
        return E_FAIL;
    return S_OK;
}

HRESULT CMruNode::_AddPidl(UINT iSlot, LPCITEMIDLIST pidl)
{
    return CMruBase::_AddItem(iSlot, pidl, sizeof(WORD) + pidl->mkid.cb);
}

DWORD CMruNode::_DeleteValue(LPCWSTR pszValue)
{
    CMruBase::_DeleteValue(pszValue);
    return SHDeleteKeyW(m_hKey, pszValue);
}

HRESULT CMruNode::_FindPidl(LPCITEMIDLIST pidl, UINT *piSlot)
{
    return FindData(pidl, sizeof(WORD) + pidl->mkid.cb, piSlot);
}

HRESULT CMruNode::_GetPidlSlot(LPCITEMIDLIST pidl, BOOL bAdd, UINT *piSlot)
{
    LPITEMIDLIST pidlFirst = ILCloneFirst(pidl);
    if (!pidlFirst)
        return E_OUTOFMEMORY;

    UINT iSlot;
    HRESULT hr = _FindPidl(pidlFirst, &iSlot);
    if (SUCCEEDED(hr))
    {
        *piSlot = _UpdateSlots(iSlot);
        hr = S_OK;
    }
    else if (bAdd)
    {
        *piSlot = _UpdateSlots(m_cSlots);
        hr = _AddPidl(*piSlot, pidlFirst);
    }

    ILFree(pidlFirst);
    return hr;
}

HRESULT CMruNode::RemoveLeast(UINT *pnNodeSlot)
{
    if (!m_cSlots)
    {
        GetNodeSlot(pnNodeSlot);
        return S_FALSE;
    }

    UINT uSlot;
    HRESULT hr = _GetSlot(m_cSlots - 1, &uSlot);
    if (FAILED(hr))
        return hr;

    CMruNode *pNode;
    hr = _CreateNode(uSlot, &pNode);
    if (SUCCEEDED(hr))
    {
        hr = pNode->RemoveLeast(pnNodeSlot);
        pNode->Release();
    }

    if (hr == S_FALSE)
    {
        Delete(m_cSlots - 1);
        if (m_cSlots || SUCCEEDED(GetNodeSlot(0)))
            return S_OK;
    }

    return hr;
}

class CMruPidlList
    : public CMruNode
    , public IMruPidlList
{
protected:
    LPBYTE m_pbNodeSlots = NULL;    // The node slots (contains SLOT_... flags)
    DWORD m_cMaxNodeSlots = 0;      // The upper bound of the node slot index
    HANDLE m_hMutex = NULL;         // The mutex (for sync)

    BOOL _LoadNodeSlots();
    void _SaveNodeSlots();
    HRESULT _InitNodeSlots();

public:
    CMruPidlList() { }
    ~CMruPidlList() override;

    HRESULT GetEmptySlot(UINT *pnNodeSlot);
    void EmptyNodeSlot(UINT nNodeSlot);

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
    STDMETHODIMP InitList(UINT cMRUSize, HKEY hKey, LPCWSTR pszSubKey) override;
    STDMETHODIMP UsePidl(LPCITEMIDLIST pidl, UINT *pnNodeSlot) override;
    STDMETHODIMP QueryPidl(
        LPCITEMIDLIST pidl,
        UINT cSlots,
        UINT *pnNodeSlots,
        UINT *pcNodeSlots) override;
    STDMETHODIMP PruneKids(LPCITEMIDLIST pidl) override;
};

CMruPidlList::~CMruPidlList()
{
    m_pbNodeSlots = (LPBYTE)::LocalFree(m_pbNodeSlots);
    if (m_hMutex)
    {
        ::CloseHandle(m_hMutex);
        m_hMutex = NULL;
    }
}

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

BOOL CMruPidlList::_LoadNodeSlots()
{
    DWORD cbNodeSlots = m_cSlotRooms * sizeof(BYTE);
    if (SHGetValueW(m_hKey, NULL, L"NodeSlots", NULL, m_pbNodeSlots, &cbNodeSlots) != ERROR_SUCCESS)
        return FALSE;
    m_cMaxNodeSlots = cbNodeSlots / sizeof(BYTE);
    return TRUE;
}

void CMruPidlList::_SaveNodeSlots()
{
    DWORD cbNodeSlots = m_cMaxNodeSlots * sizeof(BYTE);
    SHSetValueW(m_hKey, NULL, L"NodeSlots", REG_BINARY, m_pbNodeSlots, cbNodeSlots);
}

HRESULT CMruPidlList::_InitNodeSlots()
{
    m_pbNodeSlots = (BYTE*)LocalAlloc(LPTR, m_cSlotRooms * sizeof(BYTE));
    if (!m_pbNodeSlots)
        return E_OUTOFMEMORY;

    _LoadNodeSlots();
    m_bNeedSave = TRUE;
    _SaveNodeSlots();

    return S_OK;
}

HRESULT CMruPidlList::GetEmptySlot(UINT *pnNodeSlot)
{
    *pnNodeSlot = 0;

    if (!_LoadNodeSlots())
        return E_FAIL;

    if (m_cMaxNodeSlots < m_cSlotRooms)
    {
        m_pbNodeSlots[m_cMaxNodeSlots] = SLOT_SET;
        *pnNodeSlot = ++m_cMaxNodeSlots;
        _SaveNodeSlots();
        return S_OK;
    }

    for (UINT iNodeSlot = 0; iNodeSlot < m_cMaxNodeSlots; ++iNodeSlot)
    {
        if (m_pbNodeSlots[iNodeSlot] & SLOT_SET)
            continue;

        m_pbNodeSlots[iNodeSlot] = SLOT_SET;
        *pnNodeSlot = iNodeSlot + 1; // nNodeSlot is 1-base
        _SaveNodeSlots();
        return S_OK;
    }

    HRESULT hr = E_FAIL;
    if (SUCCEEDED(RemoveLeast(pnNodeSlot)) && *pnNodeSlot)
        hr = S_OK;

    _SaveNodeSlots();
    return hr;
}

STDMETHODIMP CMruPidlList::InitList(UINT cMRUSize, HKEY hKey, LPCWSTR pszSubKey)
{
    TRACE("%p -> %u %p %s\n", this, cMRUSize, hKey, debugstr_w(pszSubKey));

    HRESULT hr = InitData(cMRUSize, 0, hKey, pszSubKey, NULL);
    if (FAILED(hr))
    {
        ERR("0x%08lX\n", hr);
        return hr;
    }

    hr = _InitNodeSlots();
    if (FAILED(hr))
    {
        ERR("0x%08lX\n", hr);
        return hr;
    }

    m_hMutex = ::CreateMutexW(NULL, FALSE, L"Shell.CMruPidlList");
    if (!m_hMutex)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR("0x%08lX\n", hr);
    }

    return hr;
}

STDMETHODIMP CMruPidlList::UsePidl(LPCITEMIDLIST pidl, UINT *pnNodeSlot)
{
    TRACE("%p -> %p %p\n", this, pidl, pnNodeSlot);

    CSafeMutex mutex;
    HRESULT hr = mutex.Enter(m_hMutex);
    if (FAILED(hr))
    {
        ERR("0x%08lX\n", hr);
        return hr;
    }

    *pnNodeSlot = 0;

    CMruNode *pNode;
    hr = GetNode(TRUE, pidl, &pNode);
    if (FAILED(hr))
    {
        ERR("0x%08lX\n", hr);
        return hr;
    }

    hr = pNode->GetNodeSlot(pnNodeSlot);
    if (FAILED(hr))
    {
        hr = GetEmptySlot(pnNodeSlot);
        if (SUCCEEDED(hr))
        {
            hr = pNode->SetNodeSlot(*pnNodeSlot);
        }
    }

    pNode->Release();
    return hr;
}

STDMETHODIMP CMruPidlList::QueryPidl(
    LPCITEMIDLIST pidl,
    UINT cSlots,
    UINT *pnNodeSlots,
    UINT *pcNodeSlots)
{
    TRACE("%p -> %p %u %p %p\n", this, pidl, cSlots, pnNodeSlots, pcNodeSlots);

    CSafeMutex mutex;
    HRESULT hr = mutex.Enter(m_hMutex);
    if (FAILED(hr))
    {
        ERR("0x%08lX\n", hr);
        return hr;
    }

    *pcNodeSlots = 0;

    CMruNode *pNode;
    hr = GetNode(FALSE, pidl, &pNode);
    if (FAILED(hr))
    {
        ERR("0x%08lX\n", hr);
        return hr;
    }

    while (pNode && *pcNodeSlots < cSlots)
    {
        CMruNode *pParent = pNode->GetParent();
        if (SUCCEEDED(pNode->GetNodeSlot(&pnNodeSlots[*pcNodeSlots])))
            ++(*pcNodeSlots);
        else if (hr == S_OK && !*pcNodeSlots)
            hr = S_FALSE;

        pNode->Release();
        pNode = pParent;
    }

    if (pNode)
        pNode->Release();

    if (SUCCEEDED(hr) && !*pcNodeSlots)
        hr = E_FAIL;

    return hr;
}

STDMETHODIMP CMruPidlList::PruneKids(LPCITEMIDLIST pidl)
{
    TRACE("%p -> %p\n", this, pidl);

    CSafeMutex mutex;
    HRESULT hr = mutex.Enter(m_hMutex);
    if (FAILED(hr))
    {
        ERR("0x%08lX\n", hr);
        return hr;
    }

    if (!_LoadNodeSlots())
        return hr;

    CMruNode *pNode;
    hr = GetNode(FALSE, pidl, &pNode);
    if (FAILED(hr))
        return hr;

    if (hr == S_OK)
        hr = pNode->Clear(this);
    else
        hr = E_FAIL;

    pNode->Release();

    _SaveNodeSlots();
    return hr;
}

void CMruPidlList::EmptyNodeSlot(UINT nNodeSlot)
{
    m_pbNodeSlots[nNodeSlot - 1] = 0; // nNodeSlot is 1-base
    m_bNeedSave = TRUE;
}

EXTERN_C HRESULT CMruPidlList_CreateInstance(DWORD_PTR dwUnused1, void **ppv, DWORD_PTR dwUnused3)
{
    UNREFERENCED_PARAMETER(dwUnused1);
    UNREFERENCED_PARAMETER(dwUnused3);

    TRACE("%p %p %p\n", dwUnused1, ppv, dwUnused3);

    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    CMruPidlList *pMruList = new CMruPidlList();
    if (pMruList == NULL)
        return E_OUTOFMEMORY;

    *ppv = static_cast<IMruPidlList*>(pMruList);
    TRACE("%p\n", *ppv);
    return S_OK;
}

HRESULT CMruNode::Clear(CMruPidlList *pList)
{
    UINT uSlot, nNodeSlot;
    HRESULT hr;

    while (SUCCEEDED(_GetSlot(0, &uSlot)))
    {
        CMruNode *pNode;
        hr = _CreateNode(uSlot, &pNode);
        if (SUCCEEDED(hr))
        {
            hr = pNode->GetNodeSlot(&nNodeSlot);
            if (SUCCEEDED(hr))
                pList->EmptyNodeSlot(nNodeSlot);

            pNode->Clear(pList);
            pNode->Release();
        }
        Delete(0);
    }

    return S_OK;
}

class CMruClassFactory : public IClassFactory
{
protected:
    LONG m_cRefs = 1;

public:
    CMruClassFactory()
    {
        ::InterlockedIncrement(&SHDOCVW_refCount);
    }
    virtual ~CMruClassFactory()
    {
        ::InterlockedDecrement(&SHDOCVW_refCount);
    }

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return ::InterlockedIncrement(&m_cRefs);
    }
    STDMETHODIMP_(ULONG) Release()
    {
        if (::InterlockedDecrement(&m_cRefs) == 0)
        {
            delete this;
            return 0;
        }
        return m_cRefs;
    }

    // IClassFactory methods
    STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
    STDMETHODIMP LockServer(BOOL fLock);

    static void* operator new(size_t size)
    {
        return ::LocalAlloc(LPTR, size);
    }
    static void operator delete(void *ptr)
    {
        ::LocalFree(ptr);
    }
};

STDMETHODIMP CMruClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (!ppvObj)
        return E_POINTER;
    if (IsEqualGUID(riid, IID_IClassFactory) || IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObj = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }
    ERR("%s: E_NOINTERFACE\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

STDMETHODIMP CMruClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (IsEqualGUID(riid, IID_IMruDataList))
        return CMruLongList_CreateInstance(0, ppvObject, 0);

    if (IsEqualGUID(riid, IID_IMruPidlList))
        return CMruPidlList_CreateInstance(0, ppvObject, 0);

    return E_NOINTERFACE;
}

STDMETHODIMP CMruClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        ::InterlockedIncrement(&SHDOCVW_refCount);
    else
        ::InterlockedDecrement(&SHDOCVW_refCount);
    return S_OK;
}

EXTERN_C HRESULT CMruClassFactory_CreateInstance(REFIID riid, void **ppv)
{
    CMruClassFactory *pFactory = new CMruClassFactory();
    if (!pFactory)
        return E_OUTOFMEMORY;

    HRESULT hr = pFactory->QueryInterface(riid, ppv);
    pFactory->Release();
    return hr;
}
