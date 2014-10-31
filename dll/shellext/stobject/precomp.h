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

extern HINSTANCE g_hInstance;

#define ID_ICON_VOLUME (WM_APP + 0x4CB)

#include "csystray.h"

typedef HRESULT(STDMETHODCALLTYPE * PFNSTINIT)     (_In_ CSysTray * pSysTray);
typedef HRESULT(STDMETHODCALLTYPE * PFNSTSHUTDOWN) (_In_ CSysTray * pSysTray);
typedef HRESULT(STDMETHODCALLTYPE * PFNSTUPDATE)   (_In_ CSysTray * pSysTray);
typedef HRESULT(STDMETHODCALLTYPE * PFNSTMESSAGE)  (_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct SysTrayIconHandlers_t
{
    PFNSTINIT        pfnInit;
    PFNSTSHUTDOWN    pfnShutdown;
    PFNSTUPDATE      pfnUpdate;
    PFNSTMESSAGE     pfnMessage;
};

extern SysTrayIconHandlers_t g_IconHandlers[];
extern const int g_NumIcons;

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

