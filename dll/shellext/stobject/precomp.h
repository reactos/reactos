#ifndef _STOBJECT_PRECOMP_H_
#define _STOBJECT_PRECOMP_H_

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
#include <shlguid_undoc.h>
#include <shlobj.h>
#include <strsafe.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlstr.h>
#include <setupapi.h>
#include <shellapi.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(stobject);

#include "resource.h"

extern HINSTANCE g_hInstance;

#define ID_ICON_VOLUME  (WM_APP + 0x4CB)
#define ID_ICON_HOTPLUG (WM_APP + 0x4CC)
#define ID_ICON_POWER   (WM_APP + 0x4CD)

#define POWER_SERVICE_FLAG    0x00000001
#define HOTPLUG_SERVICE_FLAG  0x00000002
#define VOLUME_SERVICE_FLAG   0x00000004

#include "csystray.h"

typedef HRESULT(STDMETHODCALLTYPE * PFNSTINIT)     (_In_ CSysTray * pSysTray);
typedef HRESULT(STDMETHODCALLTYPE * PFNSTSHUTDOWN) (_In_ CSysTray * pSysTray);
typedef HRESULT(STDMETHODCALLTYPE * PFNSTUPDATE)   (_In_ CSysTray * pSysTray);
typedef HRESULT(STDMETHODCALLTYPE * PFNSTMESSAGE)  (_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult);

struct SysTrayIconHandlers_t
{
    DWORD            dwServiceFlag;
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

extern HRESULT STDMETHODCALLTYPE Hotplug_Init(_In_ CSysTray * pSysTray);
extern HRESULT STDMETHODCALLTYPE Hotplug_Shutdown(_In_ CSysTray * pSysTray);
extern HRESULT STDMETHODCALLTYPE Hotplug_Update(_In_ CSysTray * pSysTray);
extern HRESULT STDMETHODCALLTYPE Hotplug_Message(_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult);

extern HRESULT STDMETHODCALLTYPE Power_Init(_In_ CSysTray * pSysTray);
extern HRESULT STDMETHODCALLTYPE Power_Shutdown(_In_ CSysTray * pSysTray);
extern HRESULT STDMETHODCALLTYPE Power_Update(_In_ CSysTray * pSysTray);
extern HRESULT STDMETHODCALLTYPE Power_Message(_In_ CSysTray * pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult);

#define POWER_TIMER_ID   2
#define VOLUME_TIMER_ID  3
#define HOTPLUG_TIMER_ID 4

#endif /* _STOBJECT_PRECOMP_H_ */
