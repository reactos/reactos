/************************************************************
 *    IShellBrowser
 */

#ifndef __WINE_WINE_OBJ_SHELLBROWSER_H
#define __WINE_WINE_OBJ_SHELLBROWSER_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/* it's ok commented out, see obj_shellview.h
   typedef struct IShellBrowser IShellBrowser, *LPSHELLBROWSER;
*/

#define SID_SShellBrowser IID_IShellBrowser

DEFINE_GUID(SID_STopLevelBrowser, 0x4C96BE40L, 0x915C, 0x11CF, 0x99, 0xD3, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37);

/* targets for GetWindow/SendControlMsg */
#define FCW_STATUS		0x0001
#define FCW_TOOLBAR		0x0002
#define FCW_TREE		0x0003
#define FCW_INTERNETBAR		0x0006
#define FCW_PROGRESS		0x0008

/* wFlags for BrowseObject*/
#define SBSP_DEFBROWSER		0x0000
#define SBSP_SAMEBROWSER	0x0001
#define SBSP_NEWBROWSER		0x0002

#define SBSP_DEFMODE		0x0000
#define SBSP_OPENMODE		0x0010
#define SBSP_EXPLOREMODE	0x0020

#define SBSP_ABSOLUTE		0x0000
#define SBSP_RELATIVE		0x1000
#define SBSP_PARENT		0x2000
#define SBSP_NAVIGATEBACK	0x4000
#define SBSP_NAVIGATEFORWARD	0x8000

#define SBSP_ALLOW_AUTONAVIGATE		0x10000

#define SBSP_INITIATEDBYHLINKFRAME	0x80000000
#define SBSP_REDIRECT			0x40000000
#define SBSP_WRITENOHISTORY		0x08000000

/* uFlage for SetToolbarItems */
#define FCT_MERGE       0x0001
#define FCT_CONFIGABLE  0x0002
#define FCT_ADDTOEND    0x0004

#define ICOM_INTERFACE IShellBrowser
#define IShellBrowser_METHODS \
	ICOM_METHOD2(HRESULT, InsertMenusSB, HMENU, hmenuShared, LPOLEMENUGROUPWIDTHS, lpMenuWidths) \
	ICOM_METHOD3(HRESULT, SetMenuSB, HMENU, hmenuShared, HOLEMENU, holemenuReserved, HWND, hwndActiveObject) \
	ICOM_METHOD1(HRESULT, RemoveMenusSB, HMENU, hmenuShared) \
	ICOM_METHOD1(HRESULT, SetStatusTextSB, LPCOLESTR, lpszStatusText) \
	ICOM_METHOD1(HRESULT, EnableModelessSB, BOOL, fEnable) \
	ICOM_METHOD2(HRESULT, TranslateAcceleratorSB, LPMSG, lpmsg, WORD, wID) \
	ICOM_METHOD2(HRESULT, BrowseObject, LPCITEMIDLIST, pidl, UINT, wFlags) \
	ICOM_METHOD2(HRESULT, GetViewStateStream, DWORD, grfMode, LPSTREAM*, ppStrm) \
	ICOM_METHOD2(HRESULT, GetControlWindow, UINT, id, HWND*, lphwnd) \
	ICOM_METHOD5(HRESULT, SendControlMsg, UINT, id, UINT, uMsg, WPARAM, wParam, LPARAM, lParam, LRESULT*, pret) \
	ICOM_METHOD1(HRESULT, QueryActiveShellView, IShellView**, IShellView) \
	ICOM_METHOD1(HRESULT, OnViewWindowActive, IShellView*, IShellView) \
	ICOM_METHOD3(HRESULT, SetToolbarItems, LPTBBUTTON, lpButtons, UINT, nButtons, UINT, uFlags)
#define IShellBrowser_IMETHODS \
	IOleWindow_IMETHODS \
	IShellBrowser_METHODS
ICOM_DEFINE(IShellBrowser,IOleWindow)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IShellBrowser_QueryInterface(p,a,b)		ICOM_CALL2(QueryInterface,p,a,b)
#define IShellBrowser_AddRef(p)				ICOM_CALL(AddRef,p)
#define IShellBrowser_Release(p)			ICOM_CALL(Release,p)
/*** IShellBrowser methods ***/
#define IShellBrowser_GetWindow(p,a)			ICOM_CALL1(GetWindow,p,a)
#define IShellBrowser_ContextSensitiveHelp(p,a)		ICOM_CALL1(ContextSensitiveHelp,p,a)
#define IShellBrowser_InsertMenusSB(p,a,b)		ICOM_CALL2(InsertMenusSB,p,a,b)
#define IShellBrowser_SetMenuSB(p,a,b,c)		ICOM_CALL3(SetMenuSB,p,a,b,c)
#define IShellBrowser_RemoveMenusSB(p,a)		ICOM_CALL1(RemoveMenusSB,p,a)
#define IShellBrowser_SetStatusTextSB(p,a)		ICOM_CALL1(SetStatusTextSB,p,a)
#define IShellBrowser_EnableModelessSB(p,a)		ICOM_CALL1(EnableModelessSB,p,a)
#define IShellBrowser_TranslateAcceleratorSB(p,a,b)	ICOM_CALL2(TranslateAcceleratorSB,p,a,b)
#define IShellBrowser_BrowseObject(p,a,b)		ICOM_CALL2(BrowseObject,p,a,b)
#define IShellBrowser_GetViewStateStream(p,a,b)		ICOM_CALL2(GetViewStateStream,p,a,b)
#define IShellBrowser_GetControlWindow(p,a,b)		ICOM_CALL2(GetControlWindow,p,a,b)
#define IShellBrowser_SendControlMsg(p,a,b,c,d,e)	ICOM_CALL5(SendControlMsg,p,a,b,c,d,e)
#define IShellBrowser_QueryActiveShellView(p,a)		ICOM_CALL1(QueryActiveShellView,p,a)
#define IShellBrowser_OnViewWindowActive(p,a)		ICOM_CALL1(OnViewWindowActive,p,a)
#define IShellBrowser_SetToolbarItems(p,a,b,c)		ICOM_CALL3(SetToolbarItems,p,a,b,c)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_SHELLBROWSER_H */
