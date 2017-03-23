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
#include <undocshell.h>
#include <shellutils.h>

#include <shellapi.h>

#include <wine/debug.h>
#include <wine/unicode.h>

#include "resource.h"

extern HINSTANCE g_hInstance;

#define ID_ICON_VOLUME (WM_APP + 0x4CB)
#define ID_ICON_POWER  (WM_APP + 0x4CC)

#include "csystray.h"

typedef HRESULT(STDMETHODCALLTYPE * PFNSTINIT)     (_In_ CSysTray * pSysTray);
typedef HRESULT(STDMETHODCALLTYPE * PFNSTSHUTDOWN) (_In_ CSysTray * pSysTray);
typedef HRESULT(STDMETHODCALLTYPE * PFNSTUPDATE)   (_In_ CSysTray * pSysTray);
typedef HRESULT(STDMETHODCALLTYPE * PFNSTMESSAGE)  (_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult);

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
extern HRESULT STDMETHODCALLTYPE Volume_Message(_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult);

extern HRESULT STDMETHODCALLTYPE Power_Init(_In_ CSysTray * pSysTray);
extern HRESULT STDMETHODCALLTYPE Power_Shutdown(_In_ CSysTray * pSysTray);
extern HRESULT STDMETHODCALLTYPE Power_Update(_In_ CSysTray * pSysTray);
extern HRESULT STDMETHODCALLTYPE Power_Message(_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult);
