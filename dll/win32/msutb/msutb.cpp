/*
 * PROJECT:     ReactOS msutb.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Language Bar (Tipbar)
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(msutb);

HINSTANCE g_hInst = NULL;
UINT g_wmTaskbarCreated = 0;
UINT g_uACP = CP_ACP;
DWORD g_dwOSInfo = 0;
CRITICAL_SECTION g_cs;

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

/***********************************************************************/

class CCicLibMenu : public ITfMenu
{
public:
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
public:
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
    return gModule.DllCanUnloadNow();
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
