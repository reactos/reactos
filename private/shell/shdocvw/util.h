#ifndef _UTIL_H_
#define _UTIL_H_

#include "mshtmdid.h"
#include "shlwapi.h"
#include <htmlhelp.h>
#include "mshtml.h"     // for IHTMLElement

#ifndef UNIX
#include <webcheck.h>
#else
#include <subsmgr.h>
#endif
#include "shui.h"

#ifdef __cplusplus
extern "C" {                        /* Assume C declarations for C++. */
#endif   /* __cplusplus */


extern HICON g_hiconSplat;
extern HICON g_hiconSplatSm;


void    LoadCommonIcons(void);
#ifndef POSTPOSTSPLIT
BOOL    ViewIDFromViewMode(UINT uViewMode, SHELLVIEWID *pvid);
void    SaveDefaultFolderSettings();
HRESULT DropOnMailRecipient(IDataObject *pdtobj, DWORD grfKeyState);
HRESULT SendDocToMailRecipient(LPCITEMIDLIST pidl, UINT uiCodePage, DWORD grfKeyState);

#endif

//
// Automation support.
//
HRESULT CDDEAuto_Navigate(BSTR str, HWND *phwnd, long);
HRESULT CDDEAuto_get_LocationURL(BSTR * pstr, HWND hwnd);
HRESULT CDDEAuto_get_LocationTitle(BSTR * pstr, HWND hwnd);
HRESULT CDDEAuto_get_HWND(long * phwnd);
HRESULT CDDEAuto_Exit(void);

BOOL    _InitComCtl32();

// 
//  Menu utility functions
//

void    Menu_Replace(HMENU hmenuDst, HMENU hmenuSrc);
#define  LoadMenuPopup(id) SHLoadMenuPopup(MLGetHinst(), id)
void    Menu_AppendMenu(HMENU hmenuDst, HMENU hmenuSrc);
TCHAR   StripMneumonic(LPTSTR szMenu);


DWORD   CommonDragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt);

int PropBag_ReadInt4(IPropertyBag* pPropBag, LPWSTR pszKey, int iDefault);

DWORD SHRandom(void);

STDAPI_(BOOL) UrlIsInCache(LPCTSTR pszURL);
STDAPI_(BOOL) UrlIsMappedOrInCache(LPCTSTR pszURL);
STDAPI_(BOOL) UrlIsInstalledEntry(LPCTSTR pszURL);

#ifdef UNICODE
#define IsSubscribable IsSubscribableW
#else // UNICODE
#define IsSubscribable IsSubscribableA
#endif // UNICODE

BOOL IsFileUrl(LPCSTR psz);
BOOL IsFileUrlW(LPCWSTR pcwzUrl);
BOOL IsEmptyStream(IStream* pstm);
BOOL IsInternetExplorerApp();
BOOL IsTopFrameBrowser(IServiceProvider *psp, IUnknown *punk);
BOOL IsSubscribableW(LPCWSTR psz);
BOOL IsSubscribableA(LPCSTR psz);

#define GEN_DEBUGSTRW(str)  ((str) ? (str) : L"<Null Str>")
#define GEN_DEBUGSTRA(str)  ((str) ? (str) : "<Null Str>")

#ifdef UNICODE
#define GEN_DEBUGSTR  GEN_DEBUGSTRW
#else // UNICODE
#define GEN_DEBUGSTR  GEN_DEBUGSTRA
#endif // UNICODE

HRESULT URLSubRegQueryA(LPCSTR pszKey, LPCSTR pszValue, BOOL fUseHKCU, 
                           LPSTR pszUrlOut, DWORD cchSize, DWORD dwSubstitutions);
HRESULT URLSubRegQueryW(LPCWSTR pszKey, LPCWSTR pszValue, BOOL fUseHKCU, 
                           LPWSTR pszUrlOut, DWORD cchSize, DWORD dwSubstitutions);
#ifdef UNICODE
#define URLSubRegQuery URLSubRegQueryW
#else
#define URLSubRegQuery URLSubRegQueryA
#endif

#define FillExecInfo(_info, _hwnd, _verb, _file, _params, _dir, _show) \
        (_info).hwnd            = _hwnd;        \
        (_info).lpVerb          = _verb;        \
        (_info).lpFile          = _file;        \
        (_info).lpParameters    = _params;      \
        (_info).lpDirectory     = _dir;         \
        (_info).nShow           = _show;        \
        (_info).fMask           = 0;            \
        (_info).cbSize          = sizeof(SHELLEXECUTEINFO);

void    _DeletePidlDPA(HDPA hdpa);

STDAPI_(BOOL) GetShortcutFileName(LPCTSTR pszTarget, LPCTSTR pszTitle, LPCTSTR pszDir, LPTSTR pszOut, int cchOut);
    

//-----------------------------------------------------------------------------
#define PropagateMessage SHPropagateMessage

//-----------------------------------------------------------------------------

BOOL PrepareURLForExternalApp(LPCWSTR psz, LPWSTR pszOut, LPDWORD pcchOut);


#define PrepareURLForDisplayUTF8  PrepareURLForDisplayUTF8W 
#define PrepareURLForDisplay      PrepareURLForDisplayW

STDAPI_(BOOL) PrepareURLForDisplayW(LPCWSTR pwz, LPWSTR pwzOut, LPDWORD pcbOut);
HRESULT PrepareURLForDisplayUTF8W(LPCWSTR pwz, LPWSTR pwzOut, LPDWORD pcbOut, BOOL fUTF8Enabled);
BOOL ParseURLFromOutsideSourceA (LPCSTR psz, LPSTR pszOut, LPDWORD pcchOut, LPBOOL pbWasSearchURL);
BOOL ParseURLFromOutsideSourceW (LPCWSTR psz, LPWSTR pszOut, LPDWORD pcchOut, LPBOOL pbWasSearchURL);

BOOL UTF8Enabled();


HRESULT         FormatUrlForDisplay(LPWSTR pszURL, LPWSTR pszFriendly, UINT cbBuf, BOOL fSeperate, DWORD dwCodePage);
BOOL    __cdecl _FormatMessage(LPCWSTR szTemplate, LPWSTR szBuf, UINT cchBuf, ...);
EXECUTION_STATE _SetThreadExecutionState(EXECUTION_STATE esFlags);


//=--------------------------------------------------------------------------=
// string helpers.
//

STDAPI_(BSTR) SysAllocStringA(LPCSTR pszAnsiStr);
STDAPI_(BSTR) LoadBSTR(UINT uID);

#ifdef UNICODE
#define SysAllocStringT(psz)    SysAllocString(psz)
#else
#define SysAllocStringT(psz)    SysAllocStringA(psz)
#endif

// BUGBUG:: Need to handle two different implementations of SHChangeRegister...
typedef ULONG (* PFNSHCHANGENOTIFYREGISTER)(HWND hwnd, int fSources, LONG fEvents, UINT wMsg, int cEntries, SHChangeNotifyEntry *pshcne);
typedef BOOL  (* PFNSHCHANGENOTIFYDEREGISTER)(unsigned long ulID);


extern PFNSHCHANGENOTIFYREGISTER    g_pfnSHChangeNotifyRegister;
extern PFNSHCHANGENOTIFYDEREGISTER  g_pfnSHChangeNotifyDeregister;
extern BOOL g_fNewNotify;

#define SZ_BINDCTXKEY_SITE         L"Site"

#define MAX_PAGES 16  // limit on the number of pages we can have

BOOL CALLBACK AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam);

void    IEPlaySound(LPCTSTR pszSound, BOOL fSysSound);

ULONG   RegisterNotify(HWND hwnd, UINT nMsg, LPCITEMIDLIST pidl, DWORD dwEvents, UINT uFlags, BOOL fRecursive);
BOOL    bIsValidString(LPCSTR pszString, ULONG cbLen);
void    Cabinet_FlagsToParams(UINT uFlags, LPTSTR pszParams);
HRESULT BindToAncesterFolder(LPCITEMIDLIST pidlAncester, LPCITEMIDLIST pidl, IShellFolder** ppsfParent, LPCITEMIDLIST *ppidlChild);

// logical defines for grfKeyState bits
#define FORCE_COPY (MK_LBUTTON | MK_CONTROL)                // means copy
#define FORCE_LINK (MK_LBUTTON | MK_CONTROL | MK_SHIFT)     // means link

HRESULT IsChildOrSelf(HWND hwndParent, HWND hwnd);

extern HIMAGELIST g_himlSysSmall;
extern HIMAGELIST g_himlSysLarge;

void    _InitSysImageLists();

#ifndef POSTPOSTSPLIT
HRESULT CreateFromRegKey(LPCTSTR pszKey, LPCTSTR pszValue, REFIID riid, void **ppv);
#endif

STDAPI_(LPCITEMIDLIST) VariantToConstIDList(const VARIANT *pv);
STDAPI_(LPITEMIDLIST) VariantToIDList(const VARIANT *pv);
STDAPI VariantToBuffer(const VARIANT* pvar, void *pv, UINT cb);
STDAPI VariantToGUID(VARIANT *pvar, LPGUID pguid);
STDAPI InitVariantFromBuffer(VARIANT *pvar, const void *pv, UINT cb);
STDAPI_(BOOL) InitVariantFromIDList(VARIANT* pvar, LPCITEMIDLIST pidl);
STDAPI_(BOOL) InitVariantFromGUID(VARIANT *pvar, GUID *pguid);
STDAPI_(void) InitVariantFromIDListInProc(VARIANT *pvar, LPCITEMIDLIST pidl);

extern const VARIANT c_vaEmpty;
//
// BUGBUG: Remove this ugly const to non-const casting if we can
//  figure out how to put const in IDL files.
//
#define PVAREMPTY ((VARIANT*)&c_vaEmpty)

#ifndef POSTPOSTSPLIT
extern UINT g_cfURL;
extern UINT g_cfHIDA;
extern UINT g_cfFileDescA;
extern UINT g_cfFileDescW;
extern UINT g_cfFileContents;

#ifdef UNICODE
#define g_cfFileDesc    g_cfFileDescW
#else
#define g_cfFileDesc    g_cfFileDescA
#endif

void InitClipboardFormats();
#endif

void ReleaseStgMediumHGLOBAL(STGMEDIUM *pstg);
void* DataObj_GetDataOfType(IDataObject* pdtobj, UINT cfType, STGMEDIUM *pstg);

LONG OpenRegUSKey(LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);       // dllreg.cpp

//-------------------------------------------------------------------------

extern int g_cxEdge;
extern int g_cyEdge;
extern int g_cxIcon;
extern int g_cyIcon;
extern int g_cxSmIcon;
extern int g_cySmIcon;

#ifndef POSTPOSTSPLIT
enum TRI_STATE
{
    TRI_UNKNOWN = 2,
    TRI_TRUE = TRUE,
    TRI_FALSE = FALSE
};

#endif //POSTPOSTSPLIT


BOOL        IsSameObject(IUnknown* punk1, IUnknown* punk2);

#define TrimWhiteSpaceW(psz)        StrTrimW(psz, L" \t")
#define TrimWhiteSpaceA(psz)        StrTrimA(psz, " \t")

#ifdef UNICODE
#define TrimWhiteSpace      TrimWhiteSpaceW
#else
#define TrimWhiteSpace      TrimWhiteSpaceA
#endif

LPCTSTR     SkipLeadingSlashes(LPCTSTR pszURL);

extern const LARGE_INTEGER c_li0;
extern const DISPPARAMS c_dispparamsNoArgs;
#ifndef UNIX
#define g_dispparamsNoArgs ((DISPPARAMS)c_dispparamsNoArgs) // prototype was incorrect!
#else
#define g_dispparamsNoArgs c_dispparamsNoArgs // prototype was incorrect!
#endif
BOOL IsEmptyStream(IStream* pstm);

void SetParentHwnd(HWND hwnd, HWND hwndParent);
#ifndef UNICODE

#ifndef POSTPOSTSPLIT
#define ADJUST_TO_WCHAR_POS     0
#define ADJUST_TO_TCHAR_POS     1
int AdjustECPosition(char *psz, int iPos, int iType);
#endif 

HRESULT MapNbspToSp(LPCWSTR lpwszIn, LPTSTR lpszOut, int cbszOut);
HRESULT GetDisplayableTitle(LPTSTR psz, LPCWSTR wszTitle, int cch);
#endif

LPITEMIDLIST GetTravelLogPidl(IBrowserService * pbs);

BOOL ILIsWeb(LPCITEMIDLIST pidl);

#define AnsiToUnicode(pstr, pwstr, cch)     SHAnsiToUnicode(pstr, pwstr, cch)
#define UnicodeToAnsi(pwstr, pstr, cch)     SHUnicodeToAnsi(pwstr, pstr, cch)

#define UnicodeToTChar(pwstr, pstr, cch)    SHUnicodeToTChar(pwstr, pstr, cch)
#define AnsiToTChar(pstr, ptstr, cch)       SHAnsiToTChar(pstr, ptstr, cch)
#define TCharToAnsi(ptstr, pstr, cch)       SHTCharToAnsi(ptstr, pstr, cch)

//Function for doing drag and drop given a pidl
HRESULT DragDrop(HWND hwnd, IShellFolder* psfParent, LPCITEMIDLIST pidl, DWORD dwPrefEffect, DWORD* pdwEffect);
HRESULT _SetPreferedDropEffect(IDataObject *pdtobj, DWORD dwEffect);
DWORD GetPreferedDropEffect(IDataObject *pdtobj);

//Function for getting icon index corresponding to htm files
int _GetIEHTMLImageIndex();
int _GetHTMLImageIndex();
int IEMapPIDLToSystemImageListIndex(IShellFolder *psfParent, LPCITEMIDLIST pidlChild, int * piSelectedImage);
void IEInvalidateImageList(void);

extern UINT g_cfURL;
extern UINT g_cfFileDescA;
extern UINT g_cfFileContents;
extern UINT g_cfPreferedEffect;
#ifdef UNICODE
#define g_cfFileDesc    g_cfFileDescW
#else
#define g_cfFileDesc    g_cfFileDescA
#endif

void InitClipboardFormats();

BOOL IsExpandableFolder(IShellFolder* psf, LPCITEMIDLIST pidl);

extern BOOL IsGlobalOffline(void);
extern void SetGlobalOffline(BOOL fOffline);

BOOL GetInfoTip(IShellFolder* psf, LPCITEMIDLIST pidl, LPTSTR pszText, int cchTextMax);
BOOL GetHistoryFolderPath(LPTSTR pszPath, int cchPath);
IStream * OpenPidlOrderStream(LPCITEMIDLIST pidlRoot
                                , LPCITEMIDLIST pidl       
                                , LPCSTR pszKey
                                , DWORD   grfMode);


COLORREF RegGetColorRefString( HKEY hkey, LPTSTR RegValue, COLORREF Value);
int SearchMapInt(const int *src, const int *dst, int cnt, int val);
#ifdef DEBUG
int SearchDW(DWORD *pdwBuf, int cbBuf, DWORD dwVal);
#endif

STDAPI_(LPITEMIDLIST) IEILCreate(UINT cbSize);

#ifndef POSTPOSTSPLIT
// this is for the file menus recently visited list.  
//  it represents the count of entries both back and forward 
//  that should be on the menu.
#define CRECENTMENU_MAXEACH     5
#endif

BOOL VerbExists(LPCTSTR pszExtension, LPCTSTR pszVerb, LPTSTR pszCommand);

HRESULT CreateLinkToPidl(LPCITEMIDLIST pidlTo, LPCTSTR pszDir, LPCTSTR pszTitle, LPTSTR pszOut, int cchOut);

//  the shell32 implementation of ILClone is different for win95 an ie4.
//  it doesnt check for NULL in the old version, but it does in the new...
//  so we need to redefine it to always check
STDAPI_(LPITEMIDLIST) SafeILClone(LPCITEMIDLIST pidl);
#define ILClone         SafeILClone      

STDAPI_(void) _SHUpdateImageW( LPCWSTR pszHashItem, int iIndex, UINT uFlags, int iImageIndex );
STDAPI_(void) _SHUpdateImageA( LPCSTR pszHashItem, int iIndex, UINT uFlags, int iImageIndex );
STDAPI_(int)  _SHHandleUpdateImage( LPCITEMIDLIST pidlExtra );

STDAPI CActiveDesktop_InternalCreateInstance(LPUNKNOWN * ppunk, REFIID riid);

BOOL ExecItemFromFolder(HWND hwnd, LPCSTR pszVerb, IShellFolder* psf, LPCITEMIDLIST pidlItem);

HRESULT NavToUrlUsingIEA(LPCSTR szUrl, BOOL fNewWindow);
HRESULT NavToUrlUsingIEW(LPCWSTR wszUrl, BOOL fNewWindow);

#ifdef UNICODE
#define NavToUrlUsingIE             NavToUrlUsingIEW
#else // UNICODE
#define NavToUrlUsingIE             NavToUrlUsingIEA
#endif // UNICODE

DWORD WaitForSendMessageThread(HANDLE hThread, DWORD dwTimeout);
BOOL  CreateNewFolder(HWND hwndOwner, LPCITEMIDLIST pidlParent, LPTSTR szPathNew, int nSize);

HRESULT ContextMenu_GetCommandStringVerb(LPCONTEXTMENU pcm, UINT idCmd, LPTSTR pszVerb, int cchVerb);

#ifdef __cplusplus
}                                   /* End of extern "C" {. */
#endif   /* __cplusplus */


STDMETHODIMP GetHTMLDoc2(IUnknown *punk, IHTMLDocument2 **ppHtmlDoc);
STDAPI_(IBindCtx *) CreateBindCtxForUI(IUnknown * punkSite);



// _FrameTrack flags
#define TRACKHOT        0x0001
#define TRACKEXPAND     0x0002
#define TRACKNOCHILD    0x0004
void FrameTrack(HDC hdc, LPRECT prc, UINT uFlags);


#ifdef DEBUG

#ifdef UNICODE
#define DebugFillInputString          DebugFillInputStringW
#else // UNICODE
#define DebugFillInputString          DebugFillInputStringA
#endif // UNICODE

void DebugFillInputStringA(LPSTR pszBuffer, DWORD cchSize);
void DebugFillInputStringW(LPWSTR pwzBuffer, DWORD cchSize);
#endif // DEBUG

void GetWebLocaleAsRFC1766(LPTSTR pszLocale, int cchLocale);

BOOL IsExplorerWindow(HWND hwnd);
BOOL IsFolderWindow(HWND hwnd);
BOOL FindBrowserWindow(void);

BOOL IsVK_TABCycler(MSG * pMsg);

// Review chrisny:  this can be moved into an object easily to handle generic droptarget, dropcursor
// , autoscrool, etc. . .
void _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject);
void _DragMove(HWND hwndTarget, const POINTL ptStart);


BOOL IsFeaturePotentiallyAvailable(REFCLSID rclsid);
STDAPI IEBindToObjectWithBC(LPCITEMIDLIST pidl, IBindCtx * pbc, IShellFolder **ppsfOut);

STDAPI_(UINT) GetWheelMsg();

STDAPI GetCacheLocation(LPTSTR pszCacheLocation, DWORD dwSize);

STDAPI StringToStrRet(LPCTSTR pString, STRRET *pstrret);

STDAPI_(HCURSOR) LoadHandCursor(DWORD dwRes);

HRESULT GetNavTargetName(IShellFolder* psf, LPCITEMIDLIST pidl, LPTSTR pszUrl, UINT cMaxChars);
STDAPI GetLinkInfo(IShellFolder* psf, LPCITEMIDLIST pidlItem, BOOL* pfAvailable, BOOL* pfSticky);

int GetAvgCharWidth(HWND hwnd);
void FixAmpersands(LPWSTR pszToFix, UINT cchMax);
// PostShellIEBroadcastMessage is commented out since it is not used right now
// STDAPI_(LRESULT)  PostShellIEBroadcastMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL IsInetcplRestricted(LPWSTR pszCommand);
BOOL IsNamedWindow(HWND hwnd, LPCTSTR pszClass);
BOOL HasExtendedChar(LPCWSTR pszQuery);
void ConvertToUtf8Escaped(LPWSTR pszQuery, int cch);
HRESULT SHPathPrepareForWriteWrap(HWND hwnd, IUnknown *punkEnableModless, LPCTSTR pszPath, UINT wFunc, DWORD dwFlags);

BOOL SHIsRestricted2W(HWND hwnd, BROWSER_RESTRICTIONS rest, LPCWSTR pwzUrl, DWORD dwReserved);

HRESULT IExtractIcon_GetIconLocation(
    IUnknown *punk,
    IN  UINT   uInFlags,
    OUT LPTSTR pszIconFile,
    IN  UINT   cchIconFile,
    OUT PINT   pniIcon,
    OUT PUINT  puOutFlags);

HRESULT IExtractIcon_Extract(
    IUnknown *punk,
    IN  LPCTSTR pszIconFile,
    IN  UINT    iIcon,
    OUT HICON * phiconLarge,
    OUT HICON * phiconSmall,
    IN  UINT    ucIconSize);
    
// Takes in lpszPath and returns the other form (SFN or LFN).
void GetPathOtherFormA(LPSTR lpszPath, LPSTR lpszNewPath, DWORD dwSize);

BOOL AccessAllowed(
    LPCWSTR pwszURL1,
    LPCWSTR pwszURL2);

#endif // _UTIL_H_

