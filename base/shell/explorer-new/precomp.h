#ifndef _EXPLORER_PRECOMP__H_
#define _EXPLORER_PRECOMP__H_
#define COBJMACROS
#include <windows.h>
#include <commctrl.h>
#include <oleidl.h>
#include <ole2.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <servprov.h>
#include <shlguid.h>
#include <ocidl.h>
#include <objidl.h>
#include <docobj.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>

#define USE_API_SHCREATEDESKTOP 1 /* Use SHCreateDesktop() */

#include "resource.h"
#include "comcsup.h"
#include "todo.h"
#include "undoc.h"

static ULONG __inline
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
        if (fname != NULL)
            fname++;
    }
    else
        fname++;

    if (fname == NULL)
        fname = filename;

    szMsgStart = szMsg + sprintf(szMsg, "%s:%d: ", fname, line);

    va_start(vl, lpFormat);
    uRet = (ULONG)vsprintf(szMsgStart, lpFormat, vl);
    va_end(vl);

    OutputDebugStringA(szMsg);

    return uRet;
}

#define ASSERT(cond) \
    if (!(cond)) { \
        Win32DbgPrint(__FILE__, __LINE__, "ASSERTION %s FAILED!\n", #cond); \
    }

#define DbgPrint(fmt, ...) \
    Win32DbgPrint(__FILE__, __LINE__, fmt, ##__VA_ARGS__);

extern HINSTANCE hExplorerInstance;
extern HANDLE hProcessHeap;
extern HKEY hkExplorer;

/*
 * dragdrop.c
 */

typedef struct _DROPTARGET_CALLBACKS
{
    HRESULT (*OnDragEnter)(IN IDropTarget *pDropTarget,
                           IN PVOID Context,
                           IN const FORMATETC *Format,
                           IN DWORD grfKeyState,
                           IN POINTL pt,
                           IN OUT DWORD *pdwEffect);
    HRESULT (*OnDragOver)(IN IDropTarget *pDropTarget,
                          IN PVOID Context,
                          IN DWORD grfKeyState,
                          IN POINTL pt,
                          IN OUT DWORD *pdwEffect);
    HRESULT (*OnDragLeave)(IN IDropTarget *pDropTarget,
                           IN PVOID Context);
    HRESULT (*OnDrop)(IN IDropTarget *pDropTarget,
                      IN PVOID Context,
                      IN const FORMATETC *Format,
                      IN DWORD grfKeyState,
                      IN POINTL pt,
                      IN OUT DWORD *pdwEffect);
} DROPTARGET_CALLBACKS, *PDROPTARGET_CALLBACKS;

IDropTarget *
CreateDropTarget(IN HWND hwndTarget,
                 IN DWORD nSupportedFormats,
                 IN const FORMATETC *Formats  OPTIONAL,
                 IN PVOID Context  OPTIONAL,
                 IN const DROPTARGET_CALLBACKS *Callbacks  OPTIONAL);

/*
 * explorer.c
 */

LONG
SetWindowStyle(IN HWND hWnd,
               IN LONG dwStyleMask,
               IN LONG dwStyle);

LONG
SetWindowExStyle(IN HWND hWnd,
                 IN LONG dwStyleMask,
                 IN LONG dwStyle);

HMENU
LoadPopupMenu(IN HINSTANCE hInstance,
              IN LPCTSTR lpMenuName);

HMENU
FindSubMenu(IN HMENU hMenu,
            IN UINT uItem,
            IN BOOL fByPosition);

BOOL
GetCurrentLoggedOnUserName(OUT LPTSTR szBuffer,
                           IN DWORD dwBufferSize);

BOOL
FormatMenuString(IN HMENU hMenu,
                 IN UINT uPosition,
                 IN UINT uFlags,
                 ...);

BOOL
GetExplorerRegValueSet(IN HKEY hKey,
                       IN LPCTSTR lpSubKey,
                       IN LPCTSTR lpValue);

/*
 * traywnd.c
 */

typedef HMENU (*PCREATECTXMENU)(IN HWND hWndOwner,
                                IN PVOID *ppcmContext,
                                IN PVOID Context  OPTIONAL);
typedef VOID (*PCTXMENUCOMMAND)(IN HWND hWndOwner,
                                IN UINT uiCmdId,
                                IN PVOID pcmContext  OPTIONAL,
                                IN PVOID Context  OPTIONAL);

typedef struct _TRAYWINDOW_CTXMENU
{
    PCREATECTXMENU CreateCtxMenu;
    PCTXMENUCOMMAND CtxMenuCommand;
} TRAYWINDOW_CTXMENU, *PTRAYWINDOW_CTXMENU;

extern const GUID IID_IShellDesktopTray;

#define INTERFACE ITrayWindow
DECLARE_INTERFACE_(ITrayWindow,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** ITrayWindow methods ***/
    STDMETHOD_(HRESULT,Open) (THIS) PURE;
    STDMETHOD_(HRESULT,Close) (THIS) PURE;
    STDMETHOD_(HWND,GetHWND) (THIS) PURE;
    STDMETHOD_(BOOL,IsSpecialHWND) (THIS_ HWND hWnd) PURE;
    STDMETHOD_(BOOL,IsHorizontal) (THIS) PURE;
    STDMETHOD_(HFONT,GetCaptionFonts) (THIS_ HFONT *phBoldCaption) PURE;
    STDMETHOD_(HWND,DisplayProperties) (THIS) PURE;
    STDMETHOD_(BOOL,ExecContextMenuCmd) (THIS_ UINT uiCmd) PURE;
    STDMETHOD_(BOOL,Lock) (THIS_ BOOL bLock) PURE;
};
#undef INTERFACE

#if defined(COBJMACROS)
/*** IUnknown methods ***/
#define ITrayWindow_QueryInterface(p,a,b)   (p)->lpVtbl->QueryInterface(p,a,b)
#define ITrayWindow_AddRef(p)               (p)->lpVtbl->AddRef(p)
#define ITrayWindow_Release(p)              (p)->lpVtbl->Release(p)
/*** ITrayWindow methods ***/
#define ITrayWindow_Open(p)                 (p)->lpVtbl->Open(p)
#define ITrayWindow_Close(p)                (p)->lpVtbl->Close(p)
#define ITrayWindow_GetHWND(p)              (p)->lpVtbl->GetHWND(p)
#define ITrayWindow_IsSpecialHWND(p,a)      (p)->lpVtbl->IsSpecialHWND(p,a)
#define ITrayWindow_IsHorizontal(p)         (p)->lpVtbl->IsHorizontal(p)
#define ITrayWindow_GetCaptionFonts(p,a)    (p)->lpVtbl->GetCaptionFonts(p,a)
#define ITrayWindow_DisplayProperties(p)    (p)->lpVtbl->DisplayProperties(p)
#define ITrayWindow_ExecContextMenuCmd(p,a) (p)->lpVtbl->ExecContextMenuCmd(p,a)
#define ITrayWindow_Lock(p,a)               (p)->lpVtbl->Lock(p,a)
#endif

BOOL
RegisterTrayWindowClass(VOID);

VOID
UnregisterTrayWindowClass(VOID);

ITrayWindow *
CreateTrayWindow(VOID);

VOID
TrayProcessMessages(IN OUT ITrayWindow *Tray);

VOID
TrayMessageLoop(IN OUT ITrayWindow *Tray);

/*
 * trayprop.h
 */

HWND
DisplayTrayProperties(ITrayWindow *Tray);

/*
 * desktop.c
 */

#define SHCNRF_InterruptLevel   (0x0001)
#define SHCNRF_ShellLevel   (0x0002)
#define SHCNRF_RecursiveInterrupt   (0x1000)
#define SHCNRF_NewDelivery  (0x8000)


HANDLE
DesktopCreateWindow(IN OUT ITrayWindow *Tray);

VOID
DesktopDestroyShellWindow(IN HANDLE hDesktop);

/*
 * taskband.c
 */

/* Internal Task Band CLSID */
extern const GUID CLSID_ITaskBand;

#define INTERFACE ITaskBand
DECLARE_INTERFACE_(ITaskBand,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** ITaskBand methods ***/
    STDMETHOD_(HRESULT,GetRebarBandID)(THIS_ DWORD *pdwBandID) PURE;
};
#undef INTERFACE

#if defined(COBJMACROS)
/*** IUnknown methods ***/
#define ITaskBand_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ITaskBand_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define ITaskBand_Release(p)            (p)->lpVtbl->Release(p)
/*** ITaskBand methods ***/
#define ITaskBand_GetRebarBandID(p,a)   (p)->lpVtbl->GetRebarBandID(p,a)
#endif

ITaskBand *
CreateTaskBand(IN OUT ITrayWindow *Tray);

/*
 * tbsite.c
 */

#define INTERFACE ITrayBandSite
DECLARE_INTERFACE_(ITrayBandSite,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IBandSiteStreamCallback ***/
    STDMETHOD_(HRESULT,OnLoad)(THIS_ IStream *pStm, REFIID riid, PVOID *pvObj) PURE;
    STDMETHOD_(HRESULT,OnSave)(THIS_ IUnknown *pUnk, IStream *pStm) PURE;
    /*** ITrayBandSite methods ***/
    STDMETHOD_(HRESULT,IsTaskBand) (THIS_ IUnknown *punk) PURE;
    STDMETHOD_(HRESULT,ProcessMessage) (THIS_ HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult) PURE;
    STDMETHOD_(HRESULT,AddContextMenus) (THIS_ HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags, IContextMenu **ppcm) PURE;
    STDMETHOD_(HRESULT,Lock) (THIS_ BOOL bLock) PURE;
};
#undef INTERFACE

#if defined(COBJMACROS)
/*** IUnknown methods ***/
#define ITrayBandSite_QueryInterface(p,a,b)             (p)->lpVtbl->QueryInterface(p,a,b)
#define ITrayBandSite_AddRef(p)                         (p)->lpVtbl->AddRef(p)
#define ITrayBandSite_Release(p)                        (p)->lpVtbl->Release(p)
/*** IBandSiteStreamCallback methods ***/
#define ITrayBandSite_OnLoad(p,a,b,c)                   (p)->lpVtbl->OnLoad(p,a,b,c)
#define ITrayBandSite_OnSave(p,a,b)                     (p)->lpVtbl->OnSave(p,a,b)
/*** ITrayBandSite methods ***/
#define ITrayBandSite_IsTaskBand(p,a)                   (p)->lpVtbl->IsTaskBand(p,a)
#define ITrayBandSite_ProcessMessage(p,a,b,c,d,e)       (p)->lpVtbl->ProcessMessage(p,a,b,c,d,e)
#define ITrayBandSite_AddContextMenus(p,a,b,c,d,e,f)    (p)->lpVtbl->AddContextMenus(p,a,b,c,d,e,f)
#define ITrayBandSite_Lock(p,a)                         (p)->lpVtbl->Lock(p,a)
#endif

ITrayBandSite *
CreateTrayBandSite(IN OUT ITrayWindow *Tray,
                   OUT HWND *phWndRebar,
                   OUT HWND *phWndTaskSwitch);

/*
 * startmnu.c
 */

extern const TRAYWINDOW_CTXMENU StartMenuBtnCtxMenu;

#define INTERFACE IStartMenuSite
DECLARE_INTERFACE_(IStartMenuSite,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface) (THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IStartMenuSite ***/
};
#undef INTERFACE

#if defined(COBJMACROS)
/*** IUnknown methods ***/
#define IStartMenuSite_QueryInterface(p,a,b)             (p)->lpVtbl->QueryInterface(p,a,b)
#define IStartMenuSite_AddRef(p)                         (p)->lpVtbl->AddRef(p)
#define IStartMenuSite_Release(p)                        (p)->lpVtbl->Release(p)
/*** IStartMenuSite methods ***/
#endif

IMenuPopup*
CreateStartMenu(IN ITrayWindow *Tray,
                OUT IMenuBand **ppMenuBand,
                IN HBITMAP hbmBanner  OPTIONAL,
                IN BOOL bSmallIcons);

HRESULT
UpdateStartMenu(IN OUT IMenuPopup *pMenuPopup,
                IN HBITMAP hbmBanner  OPTIONAL,
                IN BOOL bSmallIcons);

/*
 * trayntfy.c
 */

/* TrayClockWnd */
#define TCWM_GETMINIMUMSIZE (WM_USER + 0x100)
#define TCWM_UPDATETIME (WM_USER + 0x101)

/* TrayNotifyWnd */
#define TNWM_GETMINIMUMSIZE (WM_USER + 0x100)
#define TNWM_UPDATETIME (WM_USER + 0x101)
#define TNWM_SHOWCLOCK  (WM_USER + 0x102)

#define NTNWM_REALIGN   (0x1)

BOOL
RegisterTrayNotifyWndClass(VOID);

VOID
UnregisterTrayNotifyWndClass(VOID);

HWND
CreateTrayNotifyWnd(IN OUT ITrayWindow *TrayWindow,
                    IN BOOL bHideClock);

/*
 * taskswnd.c
 */

#define TSWM_ENABLEGROUPING (WM_USER + 1)
#define TSWM_UPDATETASKBARPOS   (WM_USER + 2)

BOOL
RegisterTaskSwitchWndClass(VOID);

VOID
UnregisterTaskSwitchWndClass(VOID);

HWND
CreateTaskSwitchWnd(IN HWND hWndParent,
                    IN OUT ITrayWindow *Tray);

#endif /* _EXPLORER_PRECOMP__H_ */
