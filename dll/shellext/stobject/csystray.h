/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/stobject/csystray.h
 * PURPOSE:     Systray shell service object
 * PROGRAMMERS: Robert Naumann
 *              David Quintana <gigaherz@gmail.com>
 *              Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 */

#pragma once

extern const GUID CLSID_SysTray;


typedef CWinTraits <
    WS_POPUP | WS_DLGFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
    WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_PALETTEWINDOW
> CMessageWndClass;

class CSysTray :
    public CComCoClass<CSysTray, &CLSID_SysTray>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl<CSysTray, CWindow, CMessageWndClass>,
    public IOleCommandTarget
{
    CComPtr<IOleCommandTarget> pctNetShell;

    // TODO: keep icon handlers here

    DWORD dwServicesEnabled;
    UINT wm_SHELLHOOK;
    UINT wm_DESTROYWINDOW;

    static DWORD WINAPI s_SysTrayThreadProc(PVOID param);
    HRESULT SysTrayMessageLoop();
    HRESULT SysTrayThreadProc();
    HRESULT CreateSysTrayThread();
    HRESULT DestroySysTrayWindow();

    HRESULT InitIcons();
    HRESULT ShutdownIcons();
    HRESULT UpdateIcons();
    HRESULT ProcessIconMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult);

    HRESULT InitNetShell();
    HRESULT ShutdownNetShell();

    VOID GetServicesEnabled();

public:
    HRESULT NotifyIcon(INT code, UINT uId, HICON hIcon, LPCWSTR szTip, DWORD dwstate = 0);

    HWND GetHWnd() { return m_hWnd; }

    VOID EnableService(DWORD dwServiceFlag, BOOL bEnable);
    BOOL IsServiceEnabled(DWORD dwServiceFlag);

protected:
    BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult, DWORD dwMsgMapID = 0);

public:
    CSysTray();
    virtual ~CSysTray();

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    DECLARE_WND_CLASS_EX(_T("SystemTray_Main"), CS_GLOBALCLASS, COLOR_3DFACE)

    DECLARE_REGISTRY_RESOURCEID(IDR_SYSTRAY)
    DECLARE_NOT_AGGREGATABLE(CSysTray)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSysTray)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
    END_COM_MAP()

};
