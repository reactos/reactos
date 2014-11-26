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

typedef struct IEThreadParamBlock
{
    long                            offset0;
    DWORD                   dwFlags;
    long                            offset8;
    IUnknown                      * offsetC;
    long                            offset10;
    IUnknown                      * offset14;
    LPITEMIDLIST            directoryPIDL;      // 0x18
    char                            offset1C[0x70-0x1C];    // unknown contents -- 0x1C..0x6c
    IUnknown                      * offset70;
    long                            offset74;        // unknown contents
    IUnknown                      * offset78;
    LPITEMIDLIST                    offset7C;
    LPITEMIDLIST                    offset80;
    LONG                            offset84;
    LONG                            offset88;
    LONG                            offset8C;
    LONG                            offset90;
    LONG                            offset94;
    LONG                            offset98;
    LONG                            offset9C;
    LONG                            offsetA0;
    char                            offsetA4[0xD8-0xA4];    // unknown contents -- 0xA4..0xD8
    LONG                            offsetD8;
    char                            offsetDC[0xF8-0xDC];    // unknown contents -- 0xDC..0xF8
    IUnknown                      * offsetF8;        // instance explorer
    LONG                            offsetFC;        // unknown contents
} IE_THREAD_PARAM_BLOCK, *PIE_THREAD_PARAM_BLOCK;

typedef struct ExplorerCommandLineParseResults
{
    LPWSTR                  strPath;
    LPITEMIDLIST            pidlPath;
    DWORD                   dwFlags;
    DWORD                           offsetC;
    DWORD                           offset10;
    DWORD                           offset14;
    DWORD                           offset18;
    DWORD                           offset1C;
    LPITEMIDLIST            pidlRoot;
    DWORD                           offset24;
    DWORD                           offset28;
    DWORD                           offset2C;
    DWORD                           offset30;
    GUID                    guidInproc;
} EXPLORER_CMDLINE_PARSE_RESULTS, *PEXPLORER_CMDLINE_PARSE_RESULTS;

#define SH_EXPLORER_CMDLINE_FLAG_ONE      0x00000001
#define SH_EXPLORER_CMDLINE_FLAG_S        0x00000002
// unknown/unused                         0x00000004
#define SH_EXPLORER_CMDLINE_FLAG_E        0x00000008
// unknown/unused                         0x00000010
// unknown/unused                         0x00000020
#define SH_EXPLORER_CMDLINE_FLAG_SELECT   0x00000040
#define SH_EXPLORER_CMDLINE_FLAG_EMBED    0x00000080
// unknown/unused                         0x00000100
#define SH_EXPLORER_CMDLINE_FLAG_IDLIST   0x00000200
#define SH_EXPLORER_CMDLINE_FLAG_INPROC   0x00000400
// unknown/unused                         0x00000800
#define SH_EXPLORER_CMDLINE_FLAG_NOUI     0x00001000
// unknown/unused                         0x00002000
#define SH_EXPLORER_CMDLINE_FLAG_N        0x00004000
// unknown/unused                         0x00008000
// unknown/unused                         0x00010000
#define SH_EXPLORER_CMDLINE_FLAG_SEPARATE 0x00020000
// unknown/unused                         0x00040000
// unknown/unused                         0x00080000
// unknown/unused                         0x00100000
// unknown/unused                         0x00200000
// unknown/unused                         0x00400000
// unknown/unused                         0x00800000
// unknown/unused                         0x01000000
#define SH_EXPLORER_CMDLINE_FLAG_STRING   0x02000000

#define WM_EXPLORER_OPEN_NEW_WINDOW (WM_USER+11)
#define WM_EXPLORER_1037 (WM_USER+13)

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

void WINAPI InitOCHostClass(long param8);
long WINAPI SHOpenFolderWindow(PIE_THREAD_PARAM_BLOCK parameters);
void WINAPI SHCreateSavedWindows(void);
BOOL WINAPI SHCreateFromDesktop(PEXPLORER_CMDLINE_PARSE_RESULTS parseResults);
UINT WINAPI SHExplorerParseCmdLine(PEXPLORER_CMDLINE_PARSE_RESULTS pParseResults);
void WINAPI UEMRegisterNotify(long param8, long paramC);
HRESULT WINAPI SHCreateBandForPidl(LPCITEMIDLIST param8, IUnknown *paramC, BOOL param10);
HRESULT WINAPI SHPidlFromDataObject(IDataObject *param8, long *paramC, long param10, FILEDESCRIPTORW *param14);
long WINAPI IDataObject_GetDeskBandState(long param8);
PIE_THREAD_PARAM_BLOCK WINAPI SHCreateIETHREADPARAM(long param8, long paramC, IUnknown *param10, IUnknown *param14);
PIE_THREAD_PARAM_BLOCK WINAPI SHCloneIETHREADPARAM(PIE_THREAD_PARAM_BLOCK param);
long WINAPI SHParseIECommandLine(long param8, long paramC);
void WINAPI SHDestroyIETHREADPARAM(PIE_THREAD_PARAM_BLOCK param);
BOOL WINAPI SHOnCWMCommandLine(HANDLE hSharedInfo);
LPITEMIDLIST WINAPI Channel_GetFolderPidl(void);
IUnknown *WINAPI ChannelBand_Create(LPITEMIDLIST pidl);
HRESULT WINAPI Channels_SetBandInfoSFB(IUnknown *param8);
HRESULT WINAPI IUnknown_SetBandInfoSFB(IUnknown *param8, long paramC);
HRESULT WINAPI Channel_QuickLaunch(void);
HRESULT WINAPI SHGetNavigateTarget(long param8, long paramC, long param10, long param14);
HRESULT WINAPI GetInfoTip(IUnknown *param8, long paramC, LPTSTR *param10, long cchMax);
HRESULT WINAPI SHEnumClassesOfCategories(long param8, long paramC, long param10, long param14, long param18);
HRESULT WINAPI SHWriteClassesOfCategories(long param8, long paramC, long param10, long param14, long param18, long param1C, long param20);
BOOL WINAPI SHIsExplorerBrowser();
HRESULT WINAPI SHOpenNewFrame(LPITEMIDLIST pidl, IUnknown *paramC, long param10, DWORD dwFlags);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __SHLWAPI_UNDOC_H */
