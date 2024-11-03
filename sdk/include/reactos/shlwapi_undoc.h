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

#define SHELL_NO_POLICY ((DWORD)-1)

typedef struct tagPOLICYDATA
{
    DWORD policy;    /* flags value passed to SHRestricted */
    LPCWSTR appstr;  /* application str such as "Explorer" */
    LPCWSTR keystr;  /* name of the actual registry key / policy */
} POLICYDATA, *LPPOLICYDATA;

HANDLE WINAPI SHGlobalCounterCreate(REFGUID guid);
PVOID WINAPI SHInterlockedCompareExchange(PVOID *dest, PVOID xchg, PVOID compare);
LONG WINAPI SHGlobalCounterGetValue(HANDLE hGlobalCounter);
LONG WINAPI SHGlobalCounterIncrement(HANDLE hGlobalCounter);

#if FALSE && ((DLL_EXPORT_VERSION) >= _WIN32_WINNT_VISTA)
#define SHELL_GCOUNTER_DEFINE_GUID(name, a, b, c, d, e, f, g, h, i, j, k) enum { SHELLUNUSEDCOUNTERGUID_##name }
#define SHELL_GCOUNTER_DEFINE_HANDLE(name) enum { SHELLUNUSEDCOUNTERHANDLE_##name }
#define SHELL_GCOUNTER_PARAMETERS(handle, id) id
#define SHELL_GlobalCounterCreate(refguid, handle) ( (refguid), (handle), (void)0 )
#define SHELL_GlobalCounterIsInitialized(handle) ( (handle), TRUE )
#define SHELL_GlobalCounterGet(id) SHGlobalCounterGetValue_Vista(id)
#define SHELL_GlobalCounterIncrement(id) SHGlobalCounterIncrement_Vista(id)
#else
#define SHELL_GCOUNTER_DEFINE_GUID(name, a, b, c, d, e, f, g, h, i, j, k) const GUID name = { a, b, c, { d, e, f, g, h, i, j, k } }
#define SHELL_GCOUNTER_DEFINE_HANDLE(name) HANDLE name = NULL
#define SHELL_GCOUNTER_PARAMETERS(handle, id) handle
#define SHELL_GlobalCounterCreate(refguid, handle) \
  do { \
    EXTERN_C HANDLE SHELL_GetCachedGlobalCounter(HANDLE *phGlobalCounter, REFGUID rguid); \
    SHELL_GetCachedGlobalCounter(&(handle), (refguid)); \
  } while (0)
#define SHELL_GlobalCounterIsInitialized(handle) ( (handle) != NULL )
#define SHELL_GlobalCounterGet(handle) SHGlobalCounterGetValue(handle)
#define SHELL_GlobalCounterIncrement(handle) SHGlobalCounterIncrement(handle)
#endif
#define SHELL_GCOUNTER_DECLAREPARAMETERS(handle, id) SHELL_GCOUNTER_PARAMETERS(HANDLE handle, SHGLOBALCOUNTER id)

DWORD WINAPI
SHRestrictionLookup(
    _In_ DWORD policy,
    _In_ LPCWSTR key,
    _In_ const POLICYDATA *polTable,
    _Inout_ LPDWORD polArr);

BOOL WINAPI SHAboutInfoA(LPSTR lpszDest, DWORD dwDestLen);
BOOL WINAPI SHAboutInfoW(LPWSTR lpszDest, DWORD dwDestLen);
#ifdef UNICODE
#define SHAboutInfo SHAboutInfoW
#else
#define SHAboutInfo SHAboutInfoA
#endif

HMODULE WINAPI SHPinDllOfCLSID(REFIID refiid);
HRESULT WINAPI IUnknown_QueryStatus(IUnknown *lpUnknown, REFGUID pguidCmdGroup, ULONG cCmds, OLECMD *prgCmds, OLECMDTEXT* pCmdText);
HRESULT WINAPI IUnknown_Exec(IUnknown* lpUnknown, REFGUID pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT* pvaIn, VARIANT* pvaOut);
LONG WINAPI SHSetWindowBits(HWND hwnd, INT offset, UINT wMask, UINT wFlags);
HWND WINAPI SHSetParentHwnd(HWND hWnd, HWND hWndParent);
HRESULT WINAPI ConnectToConnectionPoint(IUnknown *lpUnkSink, REFIID riid, BOOL bAdviseOnly, IUnknown *lpUnknown, LPDWORD lpCookie, IConnectionPoint **lppCP);
BOOL WINAPI SHIsSameObject(IUnknown *lpInt1, IUnknown *lpInt2);
BOOL WINAPI SHLoadMenuPopup(HINSTANCE hInst, LPCWSTR szName);
void WINAPI SHPropagateMessage(HWND hWnd, UINT uiMsgId, WPARAM wParam, LPARAM lParam, BOOL bSend);
DWORD WINAPI SHRemoveAllSubMenus(HMENU hMenu);
UINT WINAPI SHEnableMenuItem(HMENU hMenu, UINT wItemID, BOOL bEnable);
DWORD WINAPI SHCheckMenuItem(HMENU hMenu, UINT uID, BOOL bCheck);
DWORD WINAPI SHRegisterClassA(WNDCLASSA *wndclass);
BOOL WINAPI SHSimulateDrop(IDropTarget *pDrop, IDataObject *pDataObj, DWORD grfKeyState, PPOINTL lpPt, DWORD* pdwEffect);
DWORD WINAPI SHGetCurColorRes(void);
HMENU WINAPI SHGetMenuFromID(HMENU hMenu, UINT uID);
DWORD WINAPI SHMenuIndexFromID(HMENU hMenu, UINT uID);
DWORD WINAPI SHWaitForSendMessageThread(HANDLE hand, DWORD dwTimeout);
DWORD WINAPI SHSendMessageBroadcastW(UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT WINAPI SHIsExpandableFolder(LPSHELLFOLDER lpFolder, LPCITEMIDLIST pidl);
DWORD WINAPI SHFillRectClr(HDC hDC, LPCRECT pRect, COLORREF cRef);
int WINAPI SHSearchMapInt(const int *lpKeys, const int *lpValues, int iLen, int iKey);
VOID WINAPI IUnknown_Set(IUnknown **lppDest, IUnknown *lpUnknown);
HRESULT WINAPI MayQSForward(IUnknown* lpUnknown, PVOID lpReserved, REFGUID riidCmdGrp, ULONG cCmds, OLECMD *prgCmds, OLECMDTEXT *pCmdText);
HRESULT WINAPI MayExecForward(IUnknown* lpUnknown, INT iUnk, REFGUID pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);
HRESULT WINAPI IsQSForward(REFGUID pguidCmdGroup,ULONG cCmds, OLECMD *prgCmds);
BOOL WINAPI SHIsChildOrSelf(HWND hParent, HWND hChild);
HRESULT WINAPI SHForwardContextMenuMsg(IUnknown* pUnk, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult, BOOL useIContextMenu2);
VOID WINAPI SHSetDefaultDialogFont(HWND hWnd, INT id);

HRESULT WINAPI SHRegGetCLSIDKeyW(REFGUID guid, LPCWSTR lpszValue, BOOL bUseHKCU, BOOL bCreate, PHKEY phKey);

BOOL WINAPI SHAddDataBlock(LPDBLIST* lppList, const DATABLOCK_HEADER *lpNewItem);
BOOL WINAPI SHRemoveDataBlock(LPDBLIST* lppList, DWORD dwSignature);
DATABLOCK_HEADER* WINAPI SHFindDataBlock(LPDBLIST lpList, DWORD dwSignature);
HRESULT WINAPI SHWriteDataBlockList(IStream* lpStream, LPDBLIST lpList);
HRESULT WINAPI SHReadDataBlockList(IStream* lpStream, LPDBLIST* lppList);
VOID WINAPI SHFreeDataBlockList(LPDBLIST lpList);

LONG
WINAPI
RegCreateKeyExWrapW(
    _In_ HKEY hKey,
    _In_ LPCWSTR lpSubKey,
    _In_ DWORD Reserved,
    _In_opt_ LPWSTR lpClass,
    _In_ DWORD dwOptions,
    _In_ REGSAM samDesired,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _Out_ PHKEY phkResult,
    _Out_opt_ LPDWORD lpdwDisposition);

/* Redirected to kernel32.ExpandEnvironmentStringsA/W */
DWORD WINAPI SHExpandEnvironmentStringsA(LPCSTR,LPSTR,DWORD);
DWORD WINAPI SHExpandEnvironmentStringsW(LPCWSTR,LPWSTR,DWORD);
#ifdef UNICODE
#define SHExpandEnvironmentStrings SHExpandEnvironmentStringsW
#else
#define SHExpandEnvironmentStrings SHExpandEnvironmentStringsA
#endif

/* Redirected to userenv.ExpandEnvironmentStringsForUserA/W */
#if (WINVER >= 0x0500)
BOOL WINAPI SHExpandEnvironmentStringsForUserA(HANDLE, LPCSTR, LPSTR, DWORD);
BOOL WINAPI SHExpandEnvironmentStringsForUserW(HANDLE, LPCWSTR, LPWSTR, DWORD);
#ifdef UNICODE
#define SHExpandEnvironmentStringsForUser SHExpandEnvironmentStringsForUserW
#else
#define SHExpandEnvironmentStringsForUser SHExpandEnvironmentStringsForUserA
#endif
#endif


BOOL WINAPI SHIsEmptyStream(IStream*);
HRESULT WINAPI IStream_Size(IStream *lpStream, ULARGE_INTEGER* lpulSize);
HRESULT WINAPI SHInvokeDefaultCommand(HWND,IShellFolder*,LPCITEMIDLIST);
HRESULT WINAPI SHPropertyBag_ReadType(IPropertyBag *ppb, LPCWSTR pszPropName, VARIANTARG *pvarg, VARTYPE vt);
HRESULT WINAPI SHPropertyBag_ReadBOOL(IPropertyBag *ppb, LPCWSTR pszPropName, BOOL *pbValue);
BOOL WINAPI SHPropertyBag_ReadBOOLOld(IPropertyBag *ppb, LPCWSTR pszPropName, BOOL bDefValue);
HRESULT WINAPI SHPropertyBag_ReadSHORT(IPropertyBag *ppb, LPCWSTR pszPropName, SHORT *psValue);
HRESULT WINAPI SHPropertyBag_ReadInt(IPropertyBag *ppb, LPCWSTR pszPropName, LPINT pnValue);
HRESULT WINAPI SHPropertyBag_ReadLONG(IPropertyBag *ppb, LPCWSTR pszPropName, LPLONG pValue);
HRESULT WINAPI SHPropertyBag_ReadDWORD(IPropertyBag *ppb, LPCWSTR pszPropName, DWORD *pdwValue);
HRESULT WINAPI SHPropertyBag_ReadBSTR(IPropertyBag *ppb, LPCWSTR pszPropName, BSTR *pbstr);
HRESULT WINAPI SHPropertyBag_ReadStr(IPropertyBag *ppb, LPCWSTR pszPropName, LPWSTR pszDst, int cchMax);
HRESULT WINAPI SHPropertyBag_ReadPOINTL(IPropertyBag *ppb, LPCWSTR pszPropName, POINTL *pptl);
HRESULT WINAPI SHPropertyBag_ReadPOINTS(IPropertyBag *ppb, LPCWSTR pszPropName, POINTS *ppts);
HRESULT WINAPI SHPropertyBag_ReadRECTL(IPropertyBag *ppb, LPCWSTR pszPropName, RECTL *prcl);
HRESULT WINAPI SHPropertyBag_ReadGUID(IPropertyBag *ppb, LPCWSTR pszPropName, GUID *pguid);
HRESULT WINAPI SHPropertyBag_ReadStream(IPropertyBag *ppb, LPCWSTR pszPropName, IStream **ppStream);

INT WINAPI
SHGetPerScreenResName(
    _Out_writes_(cchBuffer) LPWSTR pszBuffer,
    _In_ INT cchBuffer,
    _In_ DWORD dwReserved);

HRESULT WINAPI SHPropertyBag_Delete(IPropertyBag *ppb, LPCWSTR pszPropName);
HRESULT WINAPI SHPropertyBag_WriteBOOL(IPropertyBag *ppb, LPCWSTR pszPropName, BOOL bValue);
HRESULT WINAPI SHPropertyBag_WriteSHORT(IPropertyBag *ppb, LPCWSTR pszPropName, SHORT sValue);
HRESULT WINAPI SHPropertyBag_WriteInt(IPropertyBag *ppb, LPCWSTR pszPropName, INT nValue);
HRESULT WINAPI SHPropertyBag_WriteLONG(IPropertyBag *ppb, LPCWSTR pszPropName, LONG lValue);
HRESULT WINAPI SHPropertyBag_WriteDWORD(IPropertyBag *ppb, LPCWSTR pszPropName, DWORD dwValue);
HRESULT WINAPI SHPropertyBag_WriteStr(IPropertyBag *ppb, LPCWSTR pszPropName, LPCWSTR pszValue);
HRESULT WINAPI SHPropertyBag_WriteGUID(IPropertyBag *ppb, LPCWSTR pszPropName, const GUID *pguid);
HRESULT WINAPI SHPropertyBag_WriteStream(IPropertyBag *ppb, LPCWSTR pszPropName, IStream *pStream);
HRESULT WINAPI SHPropertyBag_WritePOINTL(IPropertyBag *ppb, LPCWSTR pszPropName, const POINTL *pptl);
HRESULT WINAPI SHPropertyBag_WritePOINTS(IPropertyBag *ppb, LPCWSTR pszPropName, const POINTS *ppts);
HRESULT WINAPI SHPropertyBag_WriteRECTL(IPropertyBag *ppb, LPCWSTR pszPropName, const RECTL *prcl);

HRESULT WINAPI SHCreatePropertyBagOnMemory(_In_ DWORD dwMode, _In_ REFIID riid, _Out_ void **ppvObj);

HRESULT WINAPI
SHCreatePropertyBagOnRegKey(
    _In_ HKEY hKey,
    _In_z_ LPCWSTR pszSubKey,
    _In_ DWORD dwMode,
    _In_ REFIID riid,
    _Out_ void **ppvObj);

HRESULT WINAPI
SHCreatePropertyBagOnProfileSection(
    _In_z_ LPCWSTR lpFileName,
    _In_opt_z_ LPCWSTR pszSection,
    _In_ DWORD dwMode,
    _In_ REFIID riid,
    _Out_ void **ppvObj);

EXTERN_C HRESULT WINAPI
IUnknown_QueryServicePropertyBag(
    _In_ IUnknown *punk,
    _In_ long flags,
    _In_ REFIID riid,
    _Outptr_ void **ppvObj);

HWND WINAPI SHCreateWorkerWindowA(WNDPROC wndProc, HWND hWndParent, DWORD dwExStyle,
                                  DWORD dwStyle, HMENU hMenu, LONG_PTR wnd_extra);

HWND WINAPI SHCreateWorkerWindowW(WNDPROC wndProc, HWND hWndParent, DWORD dwExStyle,
                                  DWORD dwStyle, HMENU hMenu, LONG_PTR wnd_extra);
#ifdef UNICODE
#define SHCreateWorkerWindow SHCreateWorkerWindowW
#else
#define SHCreateWorkerWindow SHCreateWorkerWindowA
#endif

HRESULT WINAPI IUnknown_SetOwner(IUnknown *iface, IUnknown *pUnk);
HRESULT WINAPI IUnknown_GetClassID(IUnknown *lpUnknown, CLSID *lpClassId);
HRESULT WINAPI IUnknown_QueryServiceExec(IUnknown *lpUnknown, REFIID service, const GUID *group, DWORD cmdId, DWORD cmdOpt, VARIANT *pIn, VARIANT *pOut);
HRESULT WINAPI IUnknown_UIActivateIO(IUnknown *unknown, BOOL activate, LPMSG msg);
HRESULT WINAPI IUnknown_TranslateAcceleratorOCS(IUnknown *lpUnknown, LPMSG lpMsg, DWORD dwModifiers);
HRESULT WINAPI IUnknown_OnFocusOCS(IUnknown *lpUnknown, BOOL fGotFocus);
HRESULT WINAPI IUnknown_HandleIRestrict(LPUNKNOWN lpUnknown, PVOID lpArg1, PVOID lpArg2, PVOID lpArg3, PVOID lpArg4);
HRESULT WINAPI IUnknown_HasFocusIO(IUnknown * punk);
HRESULT WINAPI IUnknown_TranslateAcceleratorIO(IUnknown * punk, MSG * pmsg);
HRESULT WINAPI IUnknown_OnFocusChangeIS(LPUNKNOWN lpUnknown, LPUNKNOWN pFocusObject, BOOL bFocus);

DWORD WINAPI SHAnsiToUnicode(LPCSTR lpSrcStr, LPWSTR lpDstStr, INT iLen);
INT WINAPI SHUnicodeToAnsi(LPCWSTR lpSrcStr, LPSTR lpDstStr, INT iLen);

DWORD WINAPI SHAnsiToUnicodeCP(DWORD dwCp, LPCSTR lpSrcStr, LPWSTR lpDstStr, int iLen);
DWORD WINAPI SHUnicodeToAnsiCP(UINT CodePage, LPCWSTR lpSrcStr, LPSTR lpDstStr, int dstlen);

PVOID WINAPI SHLockSharedEx(HANDLE hData, DWORD dwProcessId, BOOL bWriteAccess);

DWORD WINAPI SHGetValueGoodBootA(HKEY hkey, LPCSTR pSubKey, LPCSTR pValue,
                                 LPDWORD pwType, LPVOID pvData, LPDWORD pbData);
DWORD WINAPI SHGetValueGoodBootW(HKEY hkey, LPCWSTR pSubKey, LPCWSTR pValue,
                                 LPDWORD pwType, LPVOID pvData, LPDWORD pbData);
HRESULT WINAPI SHLoadRegUIStringA(HKEY hkey, LPCSTR value, LPSTR buf, DWORD size);
HRESULT WINAPI SHLoadRegUIStringW(HKEY hkey, LPCWSTR value, LPWSTR buf, DWORD size);
#ifdef UNICODE
#define SHGetValueGoodBoot SHGetValueGoodBootW
#define SHLoadRegUIString  SHLoadRegUIStringW
#else
#define SHGetValueGoodBoot SHGetValueGoodBootA
#define SHLoadRegUIString  SHLoadRegUIStringA
#endif

DWORD WINAPI
SHGetIniStringW(
    _In_z_ LPCWSTR appName,
    _In_z_ LPCWSTR keyName,
    _Out_writes_to_(outLen, return + 1) LPWSTR out,
    _In_ DWORD outLen,
    _In_z_ LPCWSTR filename);

BOOL WINAPI
SHSetIniStringW(
    _In_z_ LPCWSTR appName,
    _In_z_ LPCWSTR keyName,
    _In_opt_z_ LPCWSTR str,
    _In_z_ LPCWSTR filename);

DWORD WINAPI
SHGetIniStringUTF7W(
    _In_opt_z_ LPCWSTR lpAppName,
    _In_z_ LPCWSTR lpKeyName,
    _Out_writes_to_(nSize, return + 1) _Post_z_ LPWSTR lpReturnedString,
    _In_ DWORD nSize,
    _In_z_ LPCWSTR lpFileName);

BOOL WINAPI
SHSetIniStringUTF7W(
    _In_z_ LPCWSTR lpAppName,
    _In_z_ LPCWSTR lpKeyName,
    _In_opt_z_ LPCWSTR lpString,
    _In_z_ LPCWSTR lpFileName);

enum _shellkey_flags
{
    SHKEY_Root_HKCU = 0x1,
    SHKEY_Root_HKLM = 0x2,
    SHKEY_Key_Explorer = 0x00,
    SHKEY_Key_Shell = 0x10,
    SHKEY_Key_ShellNoRoam = 0x20,
    SHKEY_Key_Classes = 0x30,
    SHKEY_Subkey_Default = 0x0000,
    SHKEY_Subkey_ResourceName = 0x1000,
    SHKEY_Subkey_Handlers = 0x2000,
    SHKEY_Subkey_Associations = 0x3000,
    SHKEY_Subkey_Volatile = 0x4000,
    SHKEY_Subkey_MUICache = 0x5000,
    SHKEY_Subkey_FileExts = 0x6000
};

HKEY WINAPI SHGetShellKey(DWORD flags, LPCWSTR sub_key, BOOL create);

int
WINAPIV
ShellMessageBoxWrapW(
  _In_opt_ HINSTANCE hAppInst,
  _In_opt_ HWND hWnd,
  _In_ LPCWSTR lpcText,
  _In_opt_ LPCWSTR lpcTitle,
  _In_ UINT fuStyle,
  ...);

/* dwWhich flags for PathFileExistsDefExtW, PathFindOnPathExW,
 * and PathFileExistsDefExtAndAttributesW */
#define WHICH_PIF       (1 << 0)
#define WHICH_COM       (1 << 1)
#define WHICH_EXE       (1 << 2)
#define WHICH_BAT       (1 << 3)
#define WHICH_LNK       (1 << 4)
#define WHICH_CMD       (1 << 5)
#define WHICH_OPTIONAL  (1 << 6)

#define WHICH_DEFAULT   (WHICH_PIF | WHICH_COM | WHICH_EXE | WHICH_BAT | WHICH_LNK | WHICH_CMD)

/* dwClass flags for PathIsValidCharA and PathIsValidCharW */
#define PATH_CHAR_CLASS_LETTER      0x00000001
#define PATH_CHAR_CLASS_ASTERIX     0x00000002
#define PATH_CHAR_CLASS_DOT         0x00000004
#define PATH_CHAR_CLASS_BACKSLASH   0x00000008
#define PATH_CHAR_CLASS_COLON       0x00000010
#define PATH_CHAR_CLASS_SEMICOLON   0x00000020
#define PATH_CHAR_CLASS_COMMA       0x00000040
#define PATH_CHAR_CLASS_SPACE       0x00000080
#define PATH_CHAR_CLASS_OTHER_VALID 0x00000100
#define PATH_CHAR_CLASS_DOUBLEQUOTE 0x00000200
#define PATH_CHAR_CLASS_INVALID     0x00000000
#define PATH_CHAR_CLASS_ANY         0xffffffff

BOOL WINAPI PathFileExistsDefExtW(LPWSTR lpszPath, DWORD dwWhich);

BOOL WINAPI
PathFileExistsDefExtAndAttributesW(
    _Inout_ LPWSTR pszPath,
    _In_ DWORD dwWhich,
    _Out_opt_ LPDWORD pdwFileAttributes);

BOOL WINAPI PathFindOnPathExW(LPWSTR lpszFile, LPCWSTR *lppszOtherDirs, DWORD dwWhich);
VOID WINAPI FixSlashesAndColonW(LPWSTR);
BOOL WINAPI PathIsValidCharA(char c, DWORD dwClass);
BOOL WINAPI PathIsValidCharW(WCHAR c, DWORD dwClass);
BOOL WINAPI SHGetPathFromIDListWrapW(LPCITEMIDLIST pidl, LPWSTR pszPath);

BOOL WINAPI
IContextMenu_Invoke(
    _In_ IContextMenu *pContextMenu,
    _In_ HWND hwnd,
    _In_ LPCSTR lpVerb,
    _In_ UINT uFlags);

DWORD WINAPI SHGetObjectCompatFlags(IUnknown *pUnk, const CLSID *clsid);

/*
 * HACK! These functions are conflicting with <shobjidl.h> inline functions...
 * We provide a macro option SHLWAPI_ISHELLFOLDER_HELPERS for using these functions.
 */
#ifdef SHLWAPI_ISHELLFOLDER_HELPERS
HRESULT WINAPI
IShellFolder_GetDisplayNameOf(
    _In_ IShellFolder *psf,
    _In_ LPCITEMIDLIST pidl,
    _In_ SHGDNF uFlags,
    _Out_ LPSTRRET lpName,
    _In_ DWORD dwRetryFlags);

/* Flags for IShellFolder_GetDisplayNameOf */
#define SFGDNO_RETRYWITHFORPARSING  0x00000001
#define SFGDNO_RETRYALWAYS          0x80000000

HRESULT WINAPI
IShellFolder_ParseDisplayName(
    _In_ IShellFolder *psf,
    _In_ HWND hwndOwner,
    _In_ LPBC pbcReserved,
    _In_ LPOLESTR lpszDisplayName,
    _Out_ ULONG *pchEaten,
    _Out_ PIDLIST_RELATIVE *ppidl,
    _Out_ ULONG *pdwAttributes);

EXTERN_C HRESULT WINAPI
IShellFolder_CompareIDs(
    _In_ IShellFolder *psf,
    _In_ LPARAM lParam,
    _In_ PCUIDLIST_RELATIVE pidl1,
    _In_ PCUIDLIST_RELATIVE pidl2);
#endif /* SHLWAPI_ISHELLFOLDER_HELPERS */

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __SHLWAPI_UNDOC_H */
