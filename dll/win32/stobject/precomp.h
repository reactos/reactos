#pragma once

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <tchar.h>

#define COBJMACROS
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <wincon.h>
#include <ddeml.h>
#include <shlguid_undoc.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlwapi_undoc.h>
#include <tchar.h>
#include <strsafe.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>

#include <shellapi.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "resource.h"

extern const GUID CLSID_SysTray;

extern HINSTANCE g_hInstance;

#define ID_ICON_VOLUME 0x4CB

/* --------------- CSysTray callbacks ------------------------------ */

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
    // TODO: keep icon handlers here

    HWND hwndSysTray;

    static DWORD WINAPI s_SysTrayThreadProc(PVOID param);
    HRESULT SysTrayMessageLoop();
    HRESULT SysTrayThreadProc();
    HRESULT CreateSysTrayThread();
    HRESULT DestroySysTrayWindow();

    HRESULT InitIcons();
    HRESULT ShutdownIcons();
    HRESULT UpdateIcons();
    HRESULT ProcessIconMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    HRESULT NotifyIcon(INT code, UINT uId, HICON hIcon, LPCWSTR szTip);

    HWND GetHWnd() { return m_hWnd; }

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

/* --------------- Icon callbacks ------------------------------ */

extern HRESULT STDMETHODCALLTYPE Volume_Init(_In_ CSysTray * pSysTray);
extern HRESULT STDMETHODCALLTYPE Volume_Shutdown(_In_ CSysTray * pSysTray);
extern HRESULT STDMETHODCALLTYPE Volume_Update(_In_ CSysTray * pSysTray);
extern HRESULT STDMETHODCALLTYPE Volume_Message(_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* --------------- Utils ------------------------------ */

static __inline ULONG
Win32DbgPrint(const char *filename, int line, const char *lpFormat, ...)
{
    char szMsg[512];
    char *szMsgStart;
    const char *fname;
    va_list vl;
    ULONG uRet;

    fname = strrchr(filename, '\\');
    if (fname == NULL)
    {
        fname = strrchr(filename, '/');
    }

    if (fname == NULL)
        fname = filename;
    else
        fname++;

    szMsgStart = szMsg + sprintf(szMsg, "[%10lu] %s:%d: ", GetTickCount(), fname, line);

    va_start(vl, lpFormat);
    uRet = (ULONG) vsprintf(szMsgStart, lpFormat, vl);
    va_end(vl);

    OutputDebugStringA(szMsg);

    return uRet;
}

#define DbgPrint(fmt, ...) \
    Win32DbgPrint(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#if 1
#define FAILED_UNEXPECTEDLY(hr) (FAILED(hr) && (DbgPrint("Unexpected failure %08x.\n", hr), TRUE))
#else
#define FAILED_UNEXPECTEDLY(hr) FAILED(hr)
#endif

