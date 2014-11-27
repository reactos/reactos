/*
 * ReactOS shlwapi
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __SHLWAPI_UNDOC_H
#define __SHLWAPI_UNDOC_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

BOOL WINAPI SHAboutInfoA(LPSTR lpszDest, DWORD dwDestLen);
BOOL WINAPI SHAboutInfoW(LPWSTR lpszDest, DWORD dwDestLen);
HRESULT WINAPI IUnknown_QueryStatus(IUnknown *lpUnknown, REFGUID pguidCmdGroup, ULONG cCmds, OLECMD *prgCmds, OLECMDTEXT* pCmdText);
HRESULT WINAPI IUnknown_Exec(IUnknown* lpUnknown, REFGUID pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT* pvaIn, VARIANT* pvaOut);
LONG WINAPI SHSetWindowBits(HWND hwnd, INT offset, UINT wMask, UINT wFlags);
HWND WINAPI SHSetParentHwnd(HWND hWnd, HWND hWndParent);
HRESULT WINAPI ConnectToConnectionPoint(IUnknown *lpUnkSink, REFIID riid, BOOL bAdviseOnly, IUnknown *lpUnknown, LPDWORD lpCookie, IConnectionPoint **lppCP);
BOOL WINAPI SHIsSameObject(IUnknown *lpInt1, IUnknown *lpInt2);
HRESULT WINAPI IUnknown_GetWindow(IUnknown *lpUnknown, HWND *lphWnd);
HRESULT WINAPI IUnknown_SetOwner(IUnknown *pUnk, ULONG arg);
HRESULT WINAPI IUnknown_SetSite(IUnknown *obj, IUnknown *site);
HRESULT WINAPI IUnknown_GetClassID(IUnknown *lpUnknown, CLSID *lpClassId);
HRESULT WINAPI IUnknown_QueryService(IUnknown* lpUnknown, REFGUID sid, REFIID riid, LPVOID *lppOut);
HRESULT WINAPI IUnknown_QueryServiceExec(IUnknown *lpUnknown, REFIID service, const GUID *group, DWORD cmdId, DWORD cmdOpt, VARIANT *pIn, VARIANT *pOut);
HRESULT WINAPI IUnknown_UIActivateIO(IUnknown *unknown, BOOL activate, LPMSG msg);
BOOL WINAPI SHLoadMenuPopup(HINSTANCE hInst, LPCWSTR szName);
void WINAPI SHPropagateMessage(HWND hWnd, UINT uiMsgId, WPARAM wParam, LPARAM lParam, BOOL bSend);
DWORD WINAPI SHRemoveAllSubMenus(HMENU hMenu);
UINT WINAPI SHEnableMenuItem(HMENU hMenu, UINT wItemID, BOOL bEnable);
DWORD WINAPI SHCheckMenuItem(HMENU hMenu, UINT uID, BOOL bCheck);
DWORD WINAPI SHRegisterClassA(WNDCLASSA *wndclass);
BOOL WINAPI SHSimulateDrop(IDropTarget *pDrop, IDataObject *pDataObj, DWORD grfKeyState, PPOINTL lpPt, DWORD* pdwEffect);
HRESULT WINAPI IUnknown_TranslateAcceleratorOCS(IUnknown *lpUnknown, LPMSG lpMsg, DWORD dwModifiers);
HRESULT WINAPI IUnknown_OnFocusOCS(IUnknown *lpUnknown, BOOL fGotFocus);
HRESULT WINAPI IUnknown_HandleIRestrict(LPUNKNOWN lpUnknown, PVOID lpArg1, PVOID lpArg2, PVOID lpArg3, PVOID lpArg4);
HMENU WINAPI SHGetMenuFromID(HMENU hMenu, UINT uID);
DWORD WINAPI SHGetCurColorRes(void);
DWORD WINAPI SHWaitForSendMessageThread(HANDLE hand, DWORD dwTimeout);
HRESULT WINAPI SHIsExpandableFolder(LPSHELLFOLDER lpFolder, LPCITEMIDLIST pidl);
DWORD WINAPI SHFillRectClr(HDC hDC, LPCRECT pRect, COLORREF cRef);
int WINAPI SHSearchMapInt(const int *lpKeys, const int *lpValues, int iLen, int iKey);
VOID WINAPI IUnknown_Set(IUnknown **lppDest, IUnknown *lpUnknown);
HRESULT WINAPI MayQSForward(IUnknown* lpUnknown, PVOID lpReserved, REFGUID riidCmdGrp, ULONG cCmds, OLECMD *prgCmds, OLECMDTEXT *pCmdText);
HRESULT WINAPI MayExecForward(IUnknown* lpUnknown, INT iUnk, REFGUID pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);
HRESULT WINAPI IsQSForward(REFGUID pguidCmdGroup,ULONG cCmds, OLECMD *prgCmds);
BOOL WINAPI SHIsChildOrSelf(HWND hParent, HWND hChild);


BOOL WINAPI SHIsEmptyStream(IStream*);
HRESULT WINAPI SHInvokeDefaultCommand(HWND,IShellFolder*,LPCITEMIDLIST);
HRESULT WINAPI SHPropertyBag_ReadPOINTL(IPropertyBag*,LPCWSTR,POINTL*);

HRESULT WINAPI SHGetPerScreenResName(OUT LPWSTR lpResName,
                                     IN INT cchResName,
                                     IN DWORD dwReserved);

HRESULT WINAPI SHPropertyBag_ReadStream(IPropertyBag*,LPCWSTR,IStream**);

HWND WINAPI SHCreateWorkerWindowA(LONG wndProc, HWND hWndParent, DWORD dwExStyle,
                        DWORD dwStyle, HMENU hMenu, LONG z);

HWND WINAPI SHCreateWorkerWindowW(LONG wndProc, HWND hWndParent, DWORD dwExStyle,
                        DWORD dwStyle, HMENU hMenu, LONG z);
#ifdef UNICODE
#define SHCreateWorkerWindow SHCreateWorkerWindowW
#else
#define SHCreateWorkerWindow SHCreateWorkerWindowA
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __SHLWAPI_UNDOC_H */
