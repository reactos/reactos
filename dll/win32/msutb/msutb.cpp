/*
 * PROJECT:     ReactOS msutb.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Language Bar (Tipbar)
 * COPYRIGHT:   Copyright 2023-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(msutb);

HINSTANCE g_hInst = NULL;
UINT g_wmTaskbarCreated = 0;
UINT g_uACP = CP_ACP;
DWORD g_dwOSInfo = 0;
CRITICAL_SECTION g_cs;
LONG g_DllRefCount = 0;

EXTERN_C void __cxa_pure_virtual(void)
{
    ERR("__cxa_pure_virtual\n");
}

class CMsUtbModule : public CComModule
{
};

BEGIN_OBJECT_MAP(ObjectMap)
    //OBJECT_ENTRY(CLSID_MSUTBDeskBand, CDeskBand) // FIXME: Implement this
END_OBJECT_MAP()

CMsUtbModule gModule;

class CCicLibMenuItem;
class CTipbarAccItem;
class CUTBMenuItem;

/***********************************************************************/

class CCicLibMenu : public ITfMenu
{
protected:
    CicArray<CCicLibMenuItem*> m_MenuItems;
    LONG m_cRefs;

public:
    CCicLibMenu();
    virtual ~CCicLibMenu();

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    STDMETHOD(AddMenuItem)(
        UINT uId,
        DWORD dwFlags,
        HBITMAP hbmp,
        HBITMAP hbmpMask,
        const WCHAR *pch,
        ULONG cch,
        ITfMenu **ppSubMenu) override;
    STDMETHOD_(CCicLibMenu*, CreateSubMenu)();
    STDMETHOD_(CCicLibMenuItem*, CreateMenuItem)();
};

/***********************************************************************/

class CCicLibMenuItem
{
protected:
    DWORD m_uId;
    DWORD m_dwFlags;
    HBITMAP m_hbmp;
    HBITMAP m_hbmpMask;
    BSTR m_bstrText;
    ITfMenu *m_pMenu;

public:
    CCicLibMenuItem();
    virtual ~CCicLibMenuItem();

    BOOL Init(
        UINT uId,
        DWORD dwFlags,
        HBITMAP hbmp,
        HBITMAP hbmpMask,
        const WCHAR *pch,
        ULONG cch,
        ITfMenu *pMenu);
    HBITMAP CreateBitmap(HANDLE hBitmap);
};

/***********************************************************************/

class CTipbarAccessible : public IAccessible
{
protected:
    LONG m_cRefs;
    HWND m_hWnd;
    IAccessible *m_pStdAccessible;
    ITypeInfo *m_pTypeInfo;
    BOOL m_bInitialized;
    CicArray<CTipbarAccItem*> m_AccItems;
    LONG m_cSelection;
    friend class CUTBMenuWnd;

public:
    CTipbarAccessible(CTipbarAccItem *pItem);
    virtual ~CTipbarAccessible();

    HRESULT Initialize();

    BOOL AddAccItem(CTipbarAccItem *pItem);
    HRESULT RemoveAccItem(CTipbarAccItem *pItem);
    void ClearAccItems();
    CTipbarAccItem *AccItemFromID(INT iItem);
    INT GetIDOfItem(CTipbarAccItem *pTarget);

    LONG_PTR CreateRefToAccObj(WPARAM wParam);
    BOOL DoDefaultActionReal(INT nID);
    void NotifyWinEvent(DWORD event, CTipbarAccItem *pItem);
    void SetWindow(HWND hWnd);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDispatch methods
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
    STDMETHOD(GetTypeInfo)(
        UINT iTInfo,
        LCID lcid,
        ITypeInfo **ppTInfo);
    STDMETHOD(GetIDsOfNames)(
        REFIID riid,
        LPOLESTR *rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID *rgDispId);
    STDMETHOD(Invoke)(
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS *pDispParams,
        VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo,
        UINT *puArgErr);

    // IAccessible methods
    STDMETHOD(get_accParent)(IDispatch **ppdispParent);
    STDMETHOD(get_accChildCount)(LONG *pcountChildren);
    STDMETHOD(get_accChild)(VARIANT varChildID, IDispatch **ppdispChild);
    STDMETHOD(get_accName)(VARIANT varID, BSTR *pszName);
    STDMETHOD(get_accValue)(VARIANT varID, BSTR *pszValue);
    STDMETHOD(get_accDescription)(VARIANT varID, BSTR *description);
    STDMETHOD(get_accRole)(VARIANT varID, VARIANT *role);
    STDMETHOD(get_accState)(VARIANT varID, VARIANT *state);
    STDMETHOD(get_accHelp)(VARIANT varID, BSTR *help);
    STDMETHOD(get_accHelpTopic)(BSTR *helpfile, VARIANT varID, LONG *pidTopic);
    STDMETHOD(get_accKeyboardShortcut)(VARIANT varID, BSTR *shortcut);
    STDMETHOD(get_accFocus)(VARIANT *pvarID);
    STDMETHOD(get_accSelection)(VARIANT *pvarID);
    STDMETHOD(get_accDefaultAction)(VARIANT varID, BSTR *action);
    STDMETHOD(accSelect)(LONG flagsSelect, VARIANT varID);
    STDMETHOD(accLocation)(
        LONG *left,
        LONG *top,
        LONG *width,
        LONG *height,
        VARIANT varID);
    STDMETHOD(accNavigate)(LONG dir, VARIANT varStart, VARIANT *pvarEnd);
    STDMETHOD(accHitTest)(LONG left, LONG top, VARIANT *pvarID);
    STDMETHOD(accDoDefaultAction)(VARIANT varID);
    STDMETHOD(put_accName)(VARIANT varID, BSTR name);
    STDMETHOD(put_accValue)(VARIANT varID, BSTR value);
};

/***********************************************************************/

class CTipbarAccItem
{
public:
    CTipbarAccItem() { }
    virtual ~CTipbarAccItem() { }

    STDMETHOD_(BSTR, GetAccName)()
    {
        return SysAllocString(L"");
    }
    STDMETHOD_(BSTR, GetAccValue)()
    {
        return NULL;
    }
    STDMETHOD_(INT, GetAccRole)()
    {
        return 10;
    }
    STDMETHOD_(INT, GetAccState)()
    {
        return 256;
    }
    STDMETHOD_(void, GetAccLocation)(LPRECT lprc)
    {
        *lprc = { 0, 0, 0, 0 };
    }
    STDMETHOD_(BSTR, GetAccDefaultAction)()
    {
        return NULL;
    }
    STDMETHOD_(BOOL, DoAccDefaultAction)()
    {
        return S_OK;
    }
    STDMETHOD_(BOOL, DoAccDefaultActionReal)()
    {
        return FALSE;
    }
};

/***********************************************************************/

class CTipbarCoInitialize
{
public:
    BOOL m_bCoInit;

    CTipbarCoInitialize() : m_bCoInit(FALSE) { }
    ~CTipbarCoInitialize() { CoUninit(); }

    HRESULT EnsureCoInit()
    {
        if (m_bCoInit)
            return S_OK;
        HRESULT hr = ::CoInitialize(NULL);
        if (FAILED(hr))
            return hr;
        m_bCoInit = TRUE;
        return S_OK;
    }

    void CoUninit()
    {
        if (m_bCoInit)
        {
            ::CoUninitialize();
            m_bCoInit = FALSE;
        }
    }
};

/***********************************************************************/

class CUTBMenuWnd : public CTipbarAccItem, public CUIFMenu
{
protected:
    CTipbarCoInitialize m_coInit;
    CTipbarAccessible *m_pAccessible;
    UINT m_nMenuWndID;
    friend class CUTBMenuItem;

public:
    CUTBMenuWnd(HINSTANCE hInst, DWORD style, DWORD dwUnknown14);

    BOOL StartDoAccDefaultActionTimer(CUTBMenuItem *pTarget);

    CTipbarAccItem* GetAccItem()
    {
        return static_cast<CTipbarAccItem*>(this);
    }
    CUIFMenu* GetMenu()
    {
        return static_cast<CUIFMenu*>(this);
    }

    STDMETHOD_(BSTR, GetAccName)() override;
    STDMETHOD_(INT, GetAccRole)() override;
    STDMETHOD_(void, Initialize)() override;
    STDMETHOD_(void, OnCreate)(HWND hWnd) override;
    STDMETHOD_(void, OnDestroy)(HWND hWnd) override;
    STDMETHOD_(HRESULT, OnGetObject)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(LRESULT, OnShowWindow)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(void, OnTimer)(WPARAM wParam) override;
};

/***********************************************************************/

class CUTBMenuItem : public CTipbarAccItem, public CUIFMenuItem
{
protected:
    CUTBMenuWnd *m_pMenuWnd;
    friend class CUTBMenuWnd;

public:
    CUTBMenuItem(CUTBMenuWnd *pMenuWnd);
    ~CUTBMenuItem() override;

    CUIFMenuItem* GetMenuItem()
    {
        return static_cast<CUIFMenuItem*>(this);
    }

    STDMETHOD_(BOOL, DoAccDefaultAction)() override;
    STDMETHOD_(BOOL, DoAccDefaultActionReal)() override;
    STDMETHOD_(BSTR, GetAccDefaultAction)() override;
    STDMETHOD_(void, GetAccLocation)(LPRECT lprc) override;
    STDMETHOD_(BSTR, GetAccName)() override;
    STDMETHOD_(INT, GetAccRole)() override;
};

/***********************************************************************/

class CTrayIconWnd
{
public:
    //FIXME
    HWND GetNotifyWnd() { return NULL; };
};

/***********************************************************************/

class CTrayIconItem
{
protected:
    HWND m_hWnd;
    UINT m_uCallbackMessage;
    UINT m_uNotifyIconID;
    DWORD m_dwIconAddOrModify;
    BOOL m_bIconAdded;
    CTrayIconWnd *m_pTrayIconWnd;
    DWORD m_dw;
    GUID m_guid;
    RECT m_rcMenu;
    POINT m_ptCursor;

    CTrayIconItem(CTrayIconWnd *pTrayIconWnd);

    BOOL _Init(HWND hWnd, UINT uCallbackMessage, UINT uNotifyIconID, const GUID& rguid);
    BOOL UpdateMenuRectPoint();
    BOOL RemoveIcon();

    STDMETHOD_(BOOL, SetIcon)(HICON hIcon, LPCTSTR pszTip);
    STDMETHOD_(LRESULT, OnMsg)(WPARAM wParam, LPARAM lParam) { return 0; };
    STDMETHOD_(LRESULT, OnDelayMsg)(LPARAM lParam) { return 0; };
};

/***********************************************************************
 * CCicLibMenu
 */

CCicLibMenu::CCicLibMenu() : m_cRefs(1)
{
}

CCicLibMenu::~CCicLibMenu()
{
    for (size_t iItem = 0; iItem < m_MenuItems.size(); ++iItem)
    {
        delete m_MenuItems[iItem];
        m_MenuItems[iItem] = NULL;
    }
}

STDMETHODIMP CCicLibMenu::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfMenu))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCicLibMenu::AddRef()
{
    return ++m_cRefs;
}

STDMETHODIMP_(ULONG) CCicLibMenu::Release()
{
    if (--m_cRefs == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

STDMETHODIMP_(CCicLibMenu*) CCicLibMenu::CreateSubMenu()
{
    return new(cicNoThrow) CCicLibMenu();
}

STDMETHODIMP_(CCicLibMenuItem*) CCicLibMenu::CreateMenuItem()
{
    return new(cicNoThrow) CCicLibMenuItem();
}

STDMETHODIMP CCicLibMenu::AddMenuItem(
    UINT uId,
    DWORD dwFlags,
    HBITMAP hbmp,
    HBITMAP hbmpMask,
    const WCHAR *pch,
    ULONG cch,
    ITfMenu **ppSubMenu)
{
    if (ppSubMenu)
        *ppSubMenu = NULL;

    CCicLibMenu *pSubMenu = NULL;
    if (dwFlags & TF_LBMENUF_SUBMENU)
    {
        if (!ppSubMenu)
            return E_INVALIDARG;
        pSubMenu = CreateSubMenu();
    }

    CCicLibMenuItem *pMenuItem = CreateMenuItem();
    if (!pMenuItem)
        return E_OUTOFMEMORY;

    if (!pMenuItem->Init(uId, dwFlags, hbmp, hbmpMask, pch, cch, pSubMenu))
        return E_FAIL;

    if (ppSubMenu && pSubMenu)
    {
        *ppSubMenu = pSubMenu;
        pSubMenu->AddRef();
    }

    *m_MenuItems.Append(1) = pMenuItem;
    return S_OK;
}

/***********************************************************************
 * CCicLibMenuItem
 */

CCicLibMenuItem::CCicLibMenuItem()
{
    m_uId = 0;
    m_dwFlags = 0;
    m_hbmp = NULL;
    m_hbmpMask = NULL;
    m_bstrText = NULL;
    m_pMenu = NULL;
}

CCicLibMenuItem::~CCicLibMenuItem()
{
    if (m_pMenu)
    {
        m_pMenu->Release();
        m_pMenu = NULL;
    }

    if (m_hbmp)
    {
        ::DeleteObject(m_hbmp);
        m_hbmp = NULL;
    }

    if (m_hbmpMask)
    {
        ::DeleteObject(m_hbmpMask);
        m_hbmpMask = NULL;
    }

    ::SysFreeString(m_bstrText);
    m_bstrText = NULL;
}

BOOL CCicLibMenuItem::Init(
    UINT uId,
    DWORD dwFlags,
    HBITMAP hbmp,
    HBITMAP hbmpMask,
    const WCHAR *pch,
    ULONG cch,
    ITfMenu *pMenu)
{
    m_uId = uId;
    m_dwFlags = dwFlags;
    m_bstrText = ::SysAllocStringLen(pch, cch);
    if (!m_bstrText && cch)
        return FALSE;

    m_pMenu = pMenu;
    m_hbmp = CreateBitmap(hbmp);
    m_hbmpMask = CreateBitmap(hbmpMask);
    if (hbmp)
        ::DeleteObject(hbmp);
    if (hbmpMask)
        ::DeleteObject(hbmpMask);

    return TRUE;
}

HBITMAP CCicLibMenuItem::CreateBitmap(HANDLE hBitmap)
{
    if (!hBitmap)
        return NULL;

    HDC hDC = ::CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
    if (!hDC)
        return NULL;

    HBITMAP hbmMem = NULL;

    BITMAP bm;
    ::GetObject(hBitmap, sizeof(bm), &bm);

    HGDIOBJ hbmOld1 = NULL;
    HDC hdcMem1 = ::CreateCompatibleDC(hDC);
    if (hdcMem1)
        hbmOld1 = ::SelectObject(hdcMem1, hBitmap);

    HGDIOBJ hbmOld2 = NULL;
    HDC hdcMem2 = ::CreateCompatibleDC(hDC);
    if (hdcMem2)
    {
        hbmMem = ::CreateCompatibleBitmap(hDC, bm.bmWidth, bm.bmHeight);
        hbmOld2 = ::SelectObject(hdcMem2, hbmMem);
    }

    ::BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem1, 0, 0, SRCCOPY);

    if (hbmOld1)
        ::SelectObject(hdcMem1, hbmOld1);
    if (hbmOld2)
        ::SelectObject(hdcMem2, hbmOld2);

    ::DeleteDC(hDC);
    if (hdcMem1)
        ::DeleteDC(hdcMem1);
    if (hdcMem2)
        ::DeleteDC(hdcMem2);

    return hbmMem;
}

/***********************************************************************
 * CTipbarAccessible
 */

CTipbarAccessible::CTipbarAccessible(CTipbarAccItem *pItem)
{
    m_cRefs = 1;
    m_hWnd = NULL;
    m_pTypeInfo = NULL;
    m_pStdAccessible = NULL;
    m_bInitialized = FALSE;
    m_cSelection = 1;
    m_AccItems.Add(pItem);
    ++g_DllRefCount;
}

CTipbarAccessible::~CTipbarAccessible()
{
    m_pTypeInfo = m_pTypeInfo;
    if (m_pTypeInfo)
    {
        m_pTypeInfo->Release();
        m_pTypeInfo = NULL;
    }
    if (m_pStdAccessible)
    {
        m_pStdAccessible->Release();
        m_pStdAccessible = NULL;
    }
    --g_DllRefCount;
}

HRESULT CTipbarAccessible::Initialize()
{
    m_bInitialized = TRUE;

    HRESULT hr = ::CreateStdAccessibleObject(m_hWnd, OBJID_CLIENT, IID_IAccessible,
                                             (void **)&m_pStdAccessible);
    if (FAILED(hr))
        return hr;

    ITypeLib *pTypeLib;
    hr = ::LoadRegTypeLib(LIBID_Accessibility, 1, 0, 0, &pTypeLib);
    if (FAILED(hr))
        hr = ::LoadTypeLib(L"OLEACC.DLL", &pTypeLib);

    if (SUCCEEDED(hr))
    {
        hr = pTypeLib->GetTypeInfoOfGuid(IID_IAccessible, &m_pTypeInfo);
        pTypeLib->Release();
    }

    return hr;
}

BOOL CTipbarAccessible::AddAccItem(CTipbarAccItem *pItem)
{
    return m_AccItems.Add(pItem);
}

HRESULT CTipbarAccessible::RemoveAccItem(CTipbarAccItem *pItem)
{
    for (size_t iItem = 0; iItem < m_AccItems.size(); ++iItem)
    {
        if (m_AccItems[iItem] == pItem)
        {
            m_AccItems.Remove(iItem, 1);
            break;
        }
    }
    return S_OK;
}

void CTipbarAccessible::ClearAccItems()
{
    m_AccItems.clear();
}

CTipbarAccItem *CTipbarAccessible::AccItemFromID(INT iItem)
{
    if (iItem < 0 || (INT)m_AccItems.size() <= iItem)
        return NULL;
    return m_AccItems[iItem];
}

INT CTipbarAccessible::GetIDOfItem(CTipbarAccItem *pTarget)
{
    for (size_t iItem = 0; iItem < m_AccItems.size(); ++iItem)
    {
        if (pTarget == m_AccItems[iItem])
            return (INT)iItem;
    }
    return -1;
}

LONG_PTR CTipbarAccessible::CreateRefToAccObj(WPARAM wParam)
{
    return ::LresultFromObject(IID_IAccessible, wParam, this);
}

BOOL CTipbarAccessible::DoDefaultActionReal(INT nID)
{
    CTipbarAccItem *pItem = AccItemFromID(nID);
    if (!pItem)
        return FALSE;
    return pItem->DoAccDefaultActionReal();
}

void CTipbarAccessible::NotifyWinEvent(DWORD event, CTipbarAccItem *pItem)
{
    INT nID = GetIDOfItem(pItem);
    if (nID < 0)
        return;

    ::NotifyWinEvent(event, m_hWnd, -4, nID);
}

void CTipbarAccessible::SetWindow(HWND hWnd)
{
    m_hWnd = hWnd;
}

STDMETHODIMP CTipbarAccessible::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IDispatch) ||
        IsEqualIID(riid, IID_IAccessible))
    {
        *ppvObject = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CTipbarAccessible::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CTipbarAccessible::Release()
{
    LONG count = ::InterlockedDecrement(&m_cRefs);
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}

STDMETHODIMP CTipbarAccessible::GetTypeInfoCount(UINT *pctinfo)
{
    if (!pctinfo)
        return E_INVALIDARG;
    *pctinfo = (m_pTypeInfo == NULL);
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::GetTypeInfo(
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo)
{
    if (!ppTInfo)
        return E_INVALIDARG;
    *ppTInfo = NULL;
    if (iTInfo != 0)
        return TYPE_E_ELEMENTNOTFOUND;
    if (!m_pTypeInfo)
        return E_NOTIMPL;
    *ppTInfo = m_pTypeInfo;
    m_pTypeInfo->AddRef();
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::GetIDsOfNames(
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId)
{
    if (!m_pTypeInfo)
        return E_NOTIMPL;
    return m_pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
}

STDMETHODIMP CTipbarAccessible::Invoke(
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    if (!m_pTypeInfo)
        return E_NOTIMPL;
    return m_pTypeInfo->Invoke(this,
                               dispIdMember,
                               wFlags,
                               pDispParams,
                               pVarResult,
                               pExcepInfo,
                               puArgErr);
}

STDMETHODIMP CTipbarAccessible::get_accParent(IDispatch **ppdispParent)
{
    return m_pStdAccessible->get_accParent(ppdispParent);
}

STDMETHODIMP CTipbarAccessible::get_accChildCount(LONG *pcountChildren)
{
    if (!pcountChildren)
        return E_INVALIDARG;
    INT cItems = (INT)m_AccItems.size();
    if (!cItems)
        return E_FAIL;
    *pcountChildren = cItems - 1;
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::get_accChild(
    VARIANT varChildID,
    IDispatch **ppdispChild)
{
    if (!ppdispChild)
        return E_INVALIDARG;
    *ppdispChild = NULL;
    return S_FALSE;
}

STDMETHODIMP CTipbarAccessible::get_accName(
    VARIANT varID,
    BSTR *pszName)
{
    if (!pszName)
        return E_INVALIDARG;
    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    if (!pItem)
        return E_INVALIDARG;
    *pszName = pItem->GetAccName();
    if (!*pszName)
        return DISP_E_MEMBERNOTFOUND;
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::get_accValue(
    VARIANT varID,
    BSTR *pszValue)
{
    if (!pszValue)
        return E_INVALIDARG;
    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    if (!pItem)
        return E_INVALIDARG;
    *pszValue = pItem->GetAccValue();
    if (!*pszValue)
        return DISP_E_MEMBERNOTFOUND;
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::get_accDescription(
    VARIANT varID,
    BSTR *description)
{
    if (!description)
        return E_INVALIDARG;
    return m_pStdAccessible->get_accDescription(varID, description);
}

STDMETHODIMP CTipbarAccessible::get_accRole(
    VARIANT varID,
    VARIANT *role)
{
    if (!role)
        return E_INVALIDARG;
    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    if (!pItem)
        return E_INVALIDARG;
    V_VT(role) = VT_I4;
    V_I4(role) = pItem->GetAccRole();
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::get_accState(
    VARIANT varID,
    VARIANT *state)
{
    if (!state)
        return E_INVALIDARG;
    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    if (!pItem)
        return E_INVALIDARG;
    V_VT(state) = VT_I4;
    V_I4(state) = pItem->GetAccState();
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::get_accHelp(VARIANT varID, BSTR *help)
{
    return DISP_E_MEMBERNOTFOUND;
}

STDMETHODIMP CTipbarAccessible::get_accHelpTopic(
    BSTR *helpfile,
    VARIANT varID,
    LONG *pidTopic)
{
    return DISP_E_MEMBERNOTFOUND;
}

STDMETHODIMP CTipbarAccessible::get_accKeyboardShortcut(VARIANT varID, BSTR *shortcut)
{
    return DISP_E_MEMBERNOTFOUND;
}

STDMETHODIMP CTipbarAccessible::get_accFocus(VARIANT *pvarID)
{
    if (!pvarID)
        return E_INVALIDARG;
    V_VT(pvarID) = VT_EMPTY;
    return S_FALSE;
}

STDMETHODIMP CTipbarAccessible::get_accSelection(VARIANT *pvarID)
{
    if (!pvarID)
        return E_INVALIDARG;

    V_VT(pvarID) = VT_EMPTY;

    INT cItems = (INT)m_AccItems.size();
    if (cItems < m_cSelection)
        return S_FALSE;

    if (cItems > m_cSelection)
    {
        V_VT(pvarID) = VT_I4;
        V_I4(pvarID) = m_cSelection;
    }

    return S_OK;
}

STDMETHODIMP CTipbarAccessible::get_accDefaultAction(
    VARIANT varID,
    BSTR *action)
{
    if (!action)
        return E_INVALIDARG;
    *action = NULL;

    if (V_VT(&varID) != VT_I4)
        return E_INVALIDARG;

    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    if (!pItem)
        return DISP_E_MEMBERNOTFOUND;
    *action = pItem->GetAccDefaultAction();
    if (!*action)
        return S_FALSE;
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::accSelect(
    LONG flagsSelect,
    VARIANT varID)
{
    if ((flagsSelect & SELFLAG_ADDSELECTION) && (flagsSelect & SELFLAG_REMOVESELECTION))
        return E_INVALIDARG;
    if (flagsSelect & (SELFLAG_TAKEFOCUS | SELFLAG_ADDSELECTION | SELFLAG_EXTENDSELECTION))
        return S_FALSE;
    if (flagsSelect & SELFLAG_REMOVESELECTION)
        return S_OK;
    if (V_VT(&varID) != VT_I4)
        return E_INVALIDARG;
    if (flagsSelect & SELFLAG_TAKESELECTION)
    {
        m_cSelection = V_I4(&varID);
        return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CTipbarAccessible::accLocation(
    LONG *left,
    LONG *top,
    LONG *width,
    LONG *height,
    VARIANT varID)
{
    if (!left || !top || !width || !height)
        return E_INVALIDARG;

    if (!V_I4(&varID))
        return m_pStdAccessible->accLocation(left, top, width, height, varID);

    RECT rc;
    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    pItem->GetAccLocation(&rc);

    *left = rc.left;
    *top = rc.top;
    *width = rc.right - rc.left;
    *height = rc.bottom - rc.top;
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::accNavigate(
    LONG dir,
    VARIANT varStart,
    VARIANT *pvarEnd)
{
    if (m_AccItems.size() <= 1)
    {
        V_VT(pvarEnd) = VT_EMPTY;
        return S_OK;
    }

    switch (dir)
    {
        case NAVDIR_UP:
        case NAVDIR_LEFT:
        case NAVDIR_PREVIOUS:
            V_VT(pvarEnd) = VT_I4;
            V_I4(pvarEnd) = V_I4(&varStart) - 1;
            if (V_I4(&varStart) - 1 <= 0)
                V_I4(pvarEnd) = (INT)(m_AccItems.size() - 1);
            return S_OK;

        case NAVDIR_DOWN:
        case NAVDIR_RIGHT:
        case NAVDIR_NEXT:
            V_VT(pvarEnd) = VT_I4;
            V_I4(pvarEnd) = V_I4(&varStart) + 1;
            if ((INT)m_AccItems.size() <= V_I4(&varStart) + 1)
                V_I4(pvarEnd) = 1;
            return S_OK;

        case NAVDIR_FIRSTCHILD:
            V_VT(pvarEnd) = VT_I4;
            V_I4(pvarEnd) = 1;
            return S_OK;

        case NAVDIR_LASTCHILD:
            V_VT(pvarEnd) = VT_I4;
            V_I4(pvarEnd) = (INT)(m_AccItems.size() - 1);
            return S_OK;

        default:
            break;
    }

    V_VT(pvarEnd) = VT_EMPTY;
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::accHitTest(LONG left, LONG top, VARIANT *pvarID)
{
    if (!pvarID)
        return E_INVALIDARG;
    POINT Point = { left, top };
    RECT Rect;
    ::ScreenToClient(m_hWnd, &Point);
    ::GetClientRect(m_hWnd, &Rect);

    if (!::PtInRect(&Rect, Point))
    {
        V_VT(pvarID) = VT_EMPTY;
        return S_OK;
    }

    V_VT(pvarID) = VT_I4;
    V_I4(pvarID) = 0;

    for (size_t iItem = 1; iItem < m_AccItems.size(); ++iItem)
    {
        CTipbarAccItem *pItem = m_AccItems[iItem];
        if (pItem)
        {
            pItem->GetAccLocation(&Rect);
            if (::PtInRect(&Rect, Point))
            {
                V_I4(pvarID) = iItem;
                break;
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CTipbarAccessible::accDoDefaultAction(VARIANT varID)
{
    if (V_VT(&varID) != VT_I4)
        return E_INVALIDARG;
    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    if (!pItem)
        return DISP_E_MEMBERNOTFOUND;
    return pItem->DoAccDefaultAction() == 0;
}

STDMETHODIMP CTipbarAccessible::put_accName(VARIANT varID, BSTR name)
{
    return S_FALSE;
}

STDMETHODIMP CTipbarAccessible::put_accValue(VARIANT varID, BSTR value)
{
    return S_FALSE;
}

/***********************************************************************
 * CUTBMenuWnd
 */

CUTBMenuWnd::CUTBMenuWnd(HINSTANCE hInst, DWORD style, DWORD dwUnknown14)
    : CUIFMenu(hInst, style, dwUnknown14)
{
}

BOOL CUTBMenuWnd::StartDoAccDefaultActionTimer(CUTBMenuItem *pTarget)
{
    if (!m_pAccessible)
        return FALSE;

    m_nMenuWndID = m_pAccessible->GetIDOfItem(pTarget);
    if (!m_nMenuWndID || m_nMenuWndID == (UINT)-1)
        return FALSE;

    if (::IsWindow(m_hWnd))
    {
        ::KillTimer(m_hWnd, 11);
        ::SetTimer(m_hWnd, 11, 200, NULL);
    }

    return TRUE;
}

STDMETHODIMP_(BSTR) CUTBMenuWnd::GetAccName()
{
    WCHAR szText[64];
    LoadStringW(g_hInst, 0x142, szText, _countof(szText));
    return ::SysAllocString(szText);
}

STDMETHODIMP_(INT) CUTBMenuWnd::GetAccRole()
{
    return 9;
}

STDMETHODIMP_(void) CUTBMenuWnd::Initialize()
{
    CTipbarAccessible *pAccessible = new(cicNoThrow) CTipbarAccessible(GetAccItem());
    if (pAccessible)
        m_pAccessible = pAccessible;

    return CUIFObject::Initialize();
}

STDMETHODIMP_(void) CUTBMenuWnd::OnCreate(HWND hWnd)
{
    if (m_pAccessible)
        m_pAccessible->SetWindow(hWnd);
}

STDMETHODIMP_(void) CUTBMenuWnd::OnDestroy(HWND hWnd)
{
    if (m_pAccessible)
    {
        m_pAccessible->NotifyWinEvent(EVENT_OBJECT_DESTROY, GetAccItem());
        m_pAccessible->ClearAccItems();
        m_pAccessible->Release();
        m_pAccessible = NULL;
    }
    m_coInit.CoUninit();
}

STDMETHODIMP_(HRESULT)
CUTBMenuWnd::OnGetObject(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (lParam != -4)
        return S_OK;

    if (!m_pAccessible)
        return E_OUTOFMEMORY;

    if (m_pAccessible->m_bInitialized)
        return m_pAccessible->CreateRefToAccObj(wParam);

    if (SUCCEEDED(m_coInit.EnsureCoInit()))
    {
        HRESULT hr = m_pAccessible->Initialize();
        if (FAILED(hr))
        {
            m_pAccessible->Release();
            m_pAccessible = NULL;
            return hr;
        }

        m_pAccessible->NotifyWinEvent(EVENT_OBJECT_CREATE, GetAccItem());
        return m_pAccessible->CreateRefToAccObj(wParam);
    }

    return S_OK;
}

STDMETHODIMP_(LRESULT)
CUTBMenuWnd::OnShowWindow(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (m_pAccessible)
    {
        if (wParam)
        {
            m_pAccessible->NotifyWinEvent(EVENT_OBJECT_SHOW, GetAccItem());
            m_pAccessible->NotifyWinEvent(EVENT_OBJECT_FOCUS, GetAccItem());
        }
        else
        {
            m_pAccessible->NotifyWinEvent(EVENT_OBJECT_HIDE, GetAccItem());
        }
    }

    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

STDMETHODIMP_(void) CUTBMenuWnd::OnTimer(WPARAM wParam)
{
    if (wParam == 11)
    {
        ::KillTimer(m_hWnd, 11);
        if (m_pAccessible && m_nMenuWndID)
        {
            m_pAccessible->DoDefaultActionReal(m_nMenuWndID);
            m_nMenuWndID = 0;
        }
    }
}

/***********************************************************************
 * CUTBMenuItem
 */

CUTBMenuItem::CUTBMenuItem(CUTBMenuWnd *pMenuWnd)
    : CUIFMenuItem(pMenuWnd ? pMenuWnd->GetMenu() : NULL)
{
    m_pMenuWnd = pMenuWnd;
}

CUTBMenuItem::~CUTBMenuItem()
{
    if (m_hbmColor)
    {
        ::DeleteObject(m_hbmColor);
        m_hbmColor = NULL;
    }
    if (m_hbmMask)
    {
        ::DeleteObject(m_hbmMask);
        m_hbmMask = NULL;
    }
}

STDMETHODIMP_(BOOL) CUTBMenuItem::DoAccDefaultAction()
{
    if (!m_pMenuWnd)
        return FALSE;

    m_pMenuWnd->StartDoAccDefaultActionTimer(this);
    return TRUE;
}

STDMETHODIMP_(BOOL) CUTBMenuItem::DoAccDefaultActionReal()
{
    if (!m_pSubMenu)
        OnLButtonUp(0, 0);
    else
        ShowSubPopup();
    return TRUE;
}

STDMETHODIMP_(BSTR) CUTBMenuItem::GetAccDefaultAction()
{
    WCHAR szText[64];
    ::LoadStringW(g_hInst, 0x143, szText, _countof(szText));
    return ::SysAllocString(szText);
}

STDMETHODIMP_(void) CUTBMenuItem::GetAccLocation(LPRECT lprc)
{
    GetRect(lprc);
    ::ClientToScreen(m_pMenuWnd->m_hWnd, (LPPOINT)lprc);
    ::ClientToScreen(m_pMenuWnd->m_hWnd, (LPPOINT)&lprc->right);
}

STDMETHODIMP_(BSTR) CUTBMenuItem::GetAccName()
{
    return ::SysAllocString(m_pszMenuItemLeft);
}

STDMETHODIMP_(INT) CUTBMenuItem::GetAccRole()
{
    if (m_pMenuWnd->GetWndStyleEx() & WS_EX_NOPARENTNOTIFY)
        return 21;
    return 12;
}

/***********************************************************************
 * CTrayIconItem
 */

CTrayIconItem::CTrayIconItem(CTrayIconWnd *pTrayIconWnd)
{
    m_dwIconAddOrModify = NIM_ADD;
    m_pTrayIconWnd = pTrayIconWnd;
}

BOOL
CTrayIconItem::_Init(
    HWND hWnd,
    UINT uCallbackMessage,
    UINT uNotifyIconID,
    const GUID& rguid)
{
    m_hWnd = hWnd;
    m_uCallbackMessage = uCallbackMessage;
    m_uNotifyIconID = uNotifyIconID;
    m_guid = rguid;
    return TRUE;
}

BOOL CTrayIconItem::RemoveIcon()
{
    if (m_dwIconAddOrModify == NIM_MODIFY)
    {
        NOTIFYICONDATA NotifyIcon = { sizeof(NotifyIcon), m_hWnd, m_uNotifyIconID };
        NotifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        NotifyIcon.uCallbackMessage = m_uCallbackMessage;
        ::Shell_NotifyIcon(NIM_DELETE, &NotifyIcon);
    }

    m_dwIconAddOrModify = NIM_ADD;
    m_bIconAdded = TRUE;
    return TRUE;
}

BOOL CTrayIconItem::SetIcon(HICON hIcon, LPCTSTR pszTip)
{
    if (!hIcon)
        return FALSE;

    NOTIFYICONDATA NotifyIcon = { sizeof(NotifyIcon), m_hWnd, m_uNotifyIconID };
    NotifyIcon.uFlags = NIF_ICON | NIF_MESSAGE;
    NotifyIcon.uCallbackMessage = m_uCallbackMessage;
    NotifyIcon.hIcon = hIcon;
    if (pszTip)
    {
        NotifyIcon.uFlags |= NIF_TIP;
        StringCchCopy(NotifyIcon.szTip, _countof(NotifyIcon.szTip), pszTip);
    }

    ::Shell_NotifyIcon(m_dwIconAddOrModify, &NotifyIcon);

    m_dwIconAddOrModify = NIM_MODIFY;
    m_bIconAdded = NIM_MODIFY;
    return TRUE;
}

BOOL CTrayIconItem::UpdateMenuRectPoint()
{
    HWND hNotifyWnd = m_pTrayIconWnd->GetNotifyWnd();
    ::GetClientRect(hNotifyWnd, &m_rcMenu);
    ::ClientToScreen(hNotifyWnd, (LPPOINT)&m_rcMenu);
    ::ClientToScreen(hNotifyWnd, (LPPOINT)&m_rcMenu.right);
    ::GetCursorPos(&m_ptCursor);
    return TRUE;
}

/***********************************************************************
 *              GetLibTls (MSUTB.@)
 *
 * @unimplemented
 */
EXTERN_C LPVOID WINAPI
GetLibTls(VOID)
{
    FIXME("stub:()\n");
    return NULL;
}

/***********************************************************************
 *              GetPopupTipbar (MSUTB.@)
 *
 * @unimplemented
 */
EXTERN_C BOOL WINAPI
GetPopupTipbar(HWND hWnd, BOOL fWinLogon)
{
    FIXME("stub:(%p, %d)\n", hWnd, fWinLogon);
    return FALSE;
}

/***********************************************************************
 *              SetRegisterLangBand (MSUTB.@)
 *
 * @unimplemented
 */
EXTERN_C HRESULT WINAPI
SetRegisterLangBand(BOOL bRegister)
{
    FIXME("stub:(%d)\n", bRegister);
    return E_NOTIMPL;
}

/***********************************************************************
 *              ClosePopupTipbar (MSUTB.@)
 *
 * @unimplemented
 */
EXTERN_C VOID WINAPI
ClosePopupTipbar(VOID)
{
    FIXME("stub:()\n");
}

/***********************************************************************
 *              DllRegisterServer (MSUTB.@)
 *
 * @implemented
 */
STDAPI DllRegisterServer(VOID)
{
    TRACE("()\n");
    return gModule.DllRegisterServer(FALSE);
}

/***********************************************************************
 *              DllUnregisterServer (MSUTB.@)
 *
 * @implemented
 */
STDAPI DllUnregisterServer(VOID)
{
    TRACE("()\n");
    return gModule.DllUnregisterServer(FALSE);
}

/***********************************************************************
 *              DllCanUnloadNow (MSUTB.@)
 *
 * @implemented
 */
STDAPI DllCanUnloadNow(VOID)
{
    TRACE("()\n");
    return gModule.DllCanUnloadNow() && (g_DllRefCount == 0);
}

/***********************************************************************
 *              DllGetClassObject (MSUTB.@)
 *
 * @implemented
 */
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("()\n");
    return gModule.DllGetClassObject(rclsid, riid, ppv);
}

/**
 * @implemented
 */
HRESULT APIENTRY
MsUtbCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID iid,
    LPVOID *ppv)
{
    if (IsEqualCLSID(rclsid, CLSID_TF_CategoryMgr))
        return TF_CreateCategoryMgr((ITfCategoryMgr**)ppv);
    if (IsEqualCLSID(rclsid, CLSID_TF_DisplayAttributeMgr))
        return TF_CreateDisplayAttributeMgr((ITfDisplayAttributeMgr **)ppv);
    return cicRealCoCreateInstance(rclsid, pUnkOuter, dwClsContext, iid, ppv);
}

/**
 * @unimplemented
 */
BOOL ProcessAttach(HINSTANCE hinstDLL)
{
    ::InitializeCriticalSectionAndSpinCount(&g_cs, 0);

    g_hInst = hinstDLL;

    cicGetOSInfo(&g_uACP, &g_dwOSInfo);

    TFInitLib(MsUtbCoCreateInstance);
    cicInitUIFLib();

    //CTrayIconWnd::RegisterClassW(); //FIXME

    g_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    gModule.Init(ObjectMap, hinstDLL, NULL);
    ::DisableThreadLibraryCalls(hinstDLL);

    return TRUE;
}

/**
 * @implemented
 */
VOID ProcessDetach(HINSTANCE hinstDLL)
{
    cicDoneUIFLib();
    TFUninitLib();
    ::DeleteCriticalSection(&g_cs);
    gModule.Term();
}

/**
 * @implemented
 */
EXTERN_C BOOL WINAPI
DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD dwReason,
    _Inout_opt_ LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            TRACE("(%p, %lu, %p)\n", hinstDLL, dwReason, lpvReserved);
            if (!ProcessAttach(hinstDLL))
            {
                ProcessDetach(hinstDLL);
                return FALSE;
            }
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            ProcessDetach(hinstDLL);
            break;
        }
    }
    return TRUE;
}
