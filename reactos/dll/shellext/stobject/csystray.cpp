/*
* PROJECT:     ReactOS system libraries
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/shellext/stobject/csystray.cpp
* PURPOSE:     Systray shell service object implementation
* PROGRAMMERS: David Quintana <gigaherz@gmail.com>
*/

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(stobject);

SysTrayIconHandlers_t g_IconHandlers [] = {
        { Volume_Init, Volume_Shutdown, Volume_Update, Volume_Message },
        { Power_Init, Power_Shutdown, Power_Update, Power_Message }
};
const int g_NumIcons = _countof(g_IconHandlers);

CSysTray::CSysTray() {}
CSysTray::~CSysTray() {}

HRESULT CSysTray::InitNetShell()
{
    HRESULT hr = CoCreateInstance(CLSID_ConnectionTray, 0, 1u, IID_PPV_ARG(IOleCommandTarget, &pctNetShell));
    if (FAILED(hr))
        return hr;

    return pctNetShell->Exec(&CGID_ShellServiceObject,
                             OLECMDID_NEW,
                             OLECMDEXECOPT_DODEFAULT, NULL, NULL);
}

HRESULT CSysTray::ShutdownNetShell()
{
    if (!pctNetShell)
        return S_FALSE;
    HRESULT hr = pctNetShell->Exec(&CGID_ShellServiceObject,
                                   OLECMDID_SAVE,
                                   OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    pctNetShell.Release();
    return hr;
}

HRESULT CSysTray::InitIcons()
{
    TRACE("Initializing Notification icons...\n");
    for (int i = 0; i < g_NumIcons; i++)
    {
        HRESULT hr = g_IconHandlers[i].pfnInit(this);
        if (FAILED(hr))
            return hr;
    }

    return InitNetShell();
}

HRESULT CSysTray::ShutdownIcons()
{
    TRACE("Shutting down Notification icons...\n");
    for (int i = 0; i < g_NumIcons; i++)
    {
        HRESULT hr = g_IconHandlers[i].pfnShutdown(this);
        if (FAILED(hr))
            return hr;
    }

    return ShutdownNetShell();
}

HRESULT CSysTray::UpdateIcons()
{
    TRACE("Updating Notification icons...\n");
    for (int i = 0; i < g_NumIcons; i++)
    {
        HRESULT hr = g_IconHandlers[i].pfnUpdate(this);
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

HRESULT CSysTray::ProcessIconMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult)
{
    for (int i = 0; i < g_NumIcons; i++)
    {
        HRESULT hr = g_IconHandlers[i].pfnMessage(this, uMsg, wParam, lParam, lResult);
        if (FAILED(hr))
            return hr;

        if (hr == S_OK)
            return hr;
    }

    // Not handled by anyone, so return accordingly.
    return S_FALSE;
}

HRESULT CSysTray::NotifyIcon(INT code, UINT uId, HICON hIcon, LPCWSTR szTip)
{
    NOTIFYICONDATA nim = { 0 };

    TRACE("NotifyIcon code=%d, uId=%d, hIcon=%p, szTip=%S\n", code, uId, hIcon, szTip);

    nim.cbSize = sizeof(NOTIFYICONDATA);
    nim.uFlags = NIF_MESSAGE | NIF_ICON | NIF_STATE | NIF_TIP;
    nim.hIcon = hIcon;
    nim.uID = uId;
    nim.uCallbackMessage = uId;
    nim.dwState = 0;
    nim.dwStateMask = 0;
    nim.hWnd = m_hWnd;
    nim.uVersion = NOTIFYICON_VERSION;
    if (szTip)
        StringCchCopy(nim.szTip, _countof(nim.szTip), szTip);
    else
        nim.szTip[0] = 0;
    BOOL ret = Shell_NotifyIcon(code, &nim);
    return ret ? S_OK : E_FAIL;
}

DWORD WINAPI CSysTray::s_SysTrayThreadProc(PVOID param)
{
    CSysTray * st = (CSysTray*) param;
    return st->SysTrayThreadProc();
}

HRESULT CSysTray::SysTrayMessageLoop()
{
    BOOL ret;
    MSG msg;

    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (ret < 0)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return S_OK;
}

HRESULT CSysTray::SysTrayThreadProc()
{
    WCHAR strFileName[MAX_PATH];
    GetModuleFileNameW(g_hInstance, strFileName, MAX_PATH);
    HMODULE hLib = LoadLibraryW(strFileName);

    CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);

    Create(NULL);

    HRESULT ret = SysTrayMessageLoop();

    CoUninitialize();

    FreeLibraryAndExitThread(hLib, ret);
}

HRESULT CSysTray::CreateSysTrayThread()
{
    TRACE("CSysTray Init TODO: Initialize tray icon handlers.\n");

    HANDLE hThread = CreateThread(NULL, 0, s_SysTrayThreadProc, this, 0, NULL);

    CloseHandle(hThread);

    return S_OK;
}

HRESULT CSysTray::DestroySysTrayWindow()
{
    DestroyWindow();
    hwndSysTray = NULL;
    return S_OK;
}

// *** IOleCommandTarget methods ***
HRESULT STDMETHODCALLTYPE CSysTray::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSysTray::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (!IsEqualGUID(*pguidCmdGroup, CGID_ShellServiceObject))
        return E_FAIL;

    switch (nCmdID)
    {
    case OLECMDID_NEW: // init
        return CreateSysTrayThread();
    case OLECMDID_SAVE: // shutdown
        return DestroySysTrayWindow();
    }
    return S_OK;
}

BOOL CSysTray::ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult, DWORD dwMsgMapID)
{
    HRESULT hr;

    if (hWnd != m_hWnd)
        return FALSE;

    switch (uMsg)
    {
    case WM_NCCREATE:
    case WM_NCDESTROY:
        return FALSE;
    case WM_CREATE:
        InitIcons();
        SetTimer(1, 2000, NULL);
        return TRUE;
    case WM_TIMER:
        UpdateIcons();
        return TRUE;
    case WM_DESTROY:
        KillTimer(1);
        ShutdownIcons();
        return TRUE;
    }

    TRACE("SysTray message received %u (%08p %08p)\n", uMsg, wParam, lParam);

    hr = ProcessIconMessage(uMsg, wParam, lParam, lResult);
    if (FAILED(hr))
        return FALSE;

    return (hr == S_OK);
}
