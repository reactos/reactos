//
// Class definition of CFileCabinet
//
// History:
//  01-12-93 GeorgeP     Created
//

#ifndef _SVCS_H_
#define _SVCS_H_


#include <docobj.h>
#define HLINK_NO_GUIDS
#define USE_SYSTEM_URL_MONIKER
#include <hlink.h>
#include <hliface.h>
#include <htiface.h>
#include <exdisp.h>
#include <urlhist.h>

#define FILEMENU        0
#define EDITMENU        1
#define VIEWMENU        2
#define TOOLSMENU       3
#define HELPMENU        4
#define NUMMENUS        5

extern UINT const c_auMenuIDs[NUMMENUS];

#define FOCUS_VIEW      0x0000
#define FOCUS_TREE      0x0001
#define FOCUS_DRIVES    0x0002

typedef struct _WINVIEW
{
        BOOL UNUSED:1;      // unused
        BOOL bToolBar:1;
        BOOL bStatusBar:1;
        BOOL bITBar:1;
#ifdef WANT_MENUONOFF
        BOOL bMenuBar:1;
#endif // WANT_MENUONOFF
} WINVIEW;

#define TBOFFSET_NONE 50
#define TBOFFSET_STD 0
#define TBOFFSET_HIST 1
#define TBOFFSET_VIEW 2

typedef struct CShellViews
{
    HDPA m_dpaViews;
} CShellViews;
void CShellViews_Delete(CShellViews* );


#define SECONDS *1000
#define ENUMERATIONTIMEOUT (5 SECONDS)

// interesting functions in fcext.c
//
// REVIEW: No need to these member functions in this header.
//
STDMETHODIMP CFileCabinet_QueryInterface(IShellBrowser * psb, REFIID riid, void **ppvObj);
STDMETHODIMP_(ULONG) CFileCabinet_AddRef(IShellBrowser * psb);
STDMETHODIMP_(ULONG) CFileCabinet_Release(IShellBrowser * psb);
STDMETHODIMP CFileCabinet_GetWindow(LPSHELLBROWSER psb, HWND *phwnd);
STDMETHODIMP CFileCabinet_EnableModeless(LPSHELLBROWSER psb, BOOL fEnable);
STDMETHODIMP CFileCabinet_TranslateAccelerator(LPSHELLBROWSER psb, LPMSG pmsg, WORD wID);
STDMETHODIMP CFileCabinet_GetControlWindow(LPSHELLBROWSER psb, UINT id, HWND *lphwnd);
STDMETHODIMP CFileCabinet_SendControlMsg(LPSHELLBROWSER psb, UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret);
STDMETHODIMP CFileCabinet_QueryActiveShellView(LPSHELLBROWSER psb, LPSHELLVIEW * ppsv);
STDMETHODIMP CFileCabinet_GetUIWindow(IShellBrowser * psb, UINT uWindow, HWND *phWnd);
STDMETHODIMP CFileCabinet_GetUIWindowRect(IShellBrowser * psb, UINT uWindow, LPRECT prc);
STDMETHODIMP CFileCabinet_GetMenu(IShellBrowser * psb, BOOL bReset, HMENU *phMenu);
STDMETHODIMP CFileCabinet_SetToolbarItems(IShellBrowser * psb, LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags);
STDMETHODIMP CFileCabinet_GetViewStateStream(IShellBrowser * psb, DWORD grfMode, LPSTREAM *pStrm);

HMENU Cabinet_MenuTemplate(BOOL bViewer, BOOL bExplorer);

// "Really" private cabinet messages
#define CWMP_ACTIVATEPENDING    (WM_USER+50)
#define CWMP_CANCELNAVIGATION   (WM_USER+51)

BOOL ViewIDFromViewMode(UINT uViewMode, SHELLVIEWID *pvid);

#endif // _SVCS_H_
