#ifndef UTILS_H
#define UTILS_H

#include <htmlhelp.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef UNICODE
HRESULT IExtractIcon_GetIconLocationThunk(IExtractIconW *peiw, UINT uFlags, LPSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags);
HRESULT IExtractIcon_ExtractThunk(IExtractIconW *peiw, LPCSTR pszFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize);
#endif

void FireSubscriptionEvent(int nCmdID, const SUBSCRIPTIONCOOKIE *pCookie);

int WCMessageBox(HWND hwnd, UINT idTextFmt, UINT idCaption, UINT uType, ...);

HRESULT GetCacheInfo(LPINTERNET_CACHE_CONFIG_INFOA *, DWORD *, DWORD *);
HRESULT SetCacheSize(LPINTERNET_CACHE_CONFIG_INFOA, DWORD);

HRESULT _CreatePropSheet(HWND, POOEBuf);
int     _GetCmdID(LPCSTR);
int     _CompareShortName(LPMYPIDL, LPMYPIDL);
int     _CompareURL(LPMYPIDL, LPMYPIDL);
int     _CompareNextUpdate(LPMYPIDL, LPMYPIDL);
int     _CompareLastUpdate(LPMYPIDL, LPMYPIDL);
int     _CompareIdentities(LPMYPIDL, LPMYPIDL);
int     _CompareGroup(LPMYPIDL, LPMYPIDL);
int     _CompareStatus(LPMYPIDL, LPMYPIDL);
int     _ComparePriority(LPMYPIDL, LPMYPIDL);
void    _GenerateEvent(LONG, LPITEMIDLIST, LPITEMIDLIST, BOOL bRefresh = TRUE);
int     _LaunchApp(HWND, LPCTSTR);
BOOL    _ValidateIDListArray(UINT cidl, LPCITEMIDLIST *ppidl);
UINT    BufferSize(POOEBuf);
VOID    CannotSaveProperty(HWND);
BOOL    ConfirmDelete(HWND, UINT, LPMYPIDL *);
void    CopyToMyPooe(POOEBuf, POOEntry);
void    CopyToOOEBuf(POOEntry, POOEBuf);
HRESULT CreateBSTRFromTSTR(BSTR *, LPCTSTR);
BOOL    DATE2DateTimeString(CFileTime&, LPTSTR);
BOOL    Date2LocalDateString(SYSTEMTIME *, LPTSTR, int);
void    VariantTimeToFileTime(double dt, FILETIME& ft);
void    FileTimeToVariantTime(FILETIME& ft, double *pdt);

void SetListViewToString (HWND hLV, LPCTSTR pszString);

void GlobalAutoDial_Enable (BOOL bEnable);
BOOL GlobalAutoDial_IsEnabled (void);
BOOL GlobalAutoDial_ShouldEnable (HWND hParentWnd);

BOOL FailsWin95SecurityCheck (HWND hParentWnd);

BOOL    IsHTTPPrefixed(LPCTSTR szURL);
HRESULT LoadOOEntryInfo(POOEBuf pooeBuf, SUBSCRIPTIONCOOKIE *pCookie, DWORD *pdwSize);
HRESULT LoadSubscription(LPCTSTR, LPMYPIDL *);
UINT    MergeMenuHierarchy(HMENU, HMENU, UINT, UINT, BOOL);
UINT    MergePopupMenu(HMENU *phMenu, UINT idResource, UINT uSubOffset, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast);
HRESULT MergeOOEBuf(OOEBuf *pBuf1, OOEBuf *pBuf2, DWORD fMask);
HRESULT SendUpdateRequests(HWND hwnd, CLSID *, UINT count);

HRESULT PersistUpdate(POOEntry, BOOL bCreate);
HRESULT SaveBufferChange(POOEBuf newBuf, BOOL bCreate);
HRESULT LoadGroupCookie(NOTIFICATIONCOOKIE *, SUBSCRIPTIONSCHEDULE subGroup);

DWORD Random(DWORD nMax);
void CreateCookie(GUID *pCookie);


BOOL _InitComCtl32();

//REVIEW Do we need the indirect versions?
#undef CreateDialogParam
#undef CreateDialog
#undef DialogBoxParam
#undef DialogBox
#undef PropertySheet

inline HWND CreateDialogParam(HINSTANCE  hInstance, LPCTSTR lpTemplateName,
                              HWND  hWndParent, DLGPROC lpDialogFunc,
                              LPARAM dwInitParam)
{
    _InitComCtl32();
#ifdef UNICODE
    return CreateDialogParamW(hInstance, lpTemplateName, hWndParent, 
                              lpDialogFunc, dwInitParam);
#else
    return CreateDialogParamA(hInstance, lpTemplateName, hWndParent, 
                              lpDialogFunc, dwInitParam);
#endif
}

inline HWND CreateDialog(HINSTANCE  hInstance, LPCTSTR lpTemplateName,
                         HWND  hWndParent, DLGPROC lpDialogFunc)
{
    return CreateDialogParam(hInstance, lpTemplateName, hWndParent, lpDialogFunc, 0L);
}

inline INT_PTR DialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplateName, HWND hWndParent,
                          DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    _InitComCtl32();
#ifdef UNICODE
    return DialogBoxParamW(hInstance, lpTemplateName, hWndParent, 
                           lpDialogFunc, dwInitParam);
#else
    return DialogBoxParamA(hInstance, lpTemplateName, hWndParent, 
                           lpDialogFunc, dwInitParam);
#endif
}

inline INT_PTR DialogBox(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent,
                     DLGPROC lpDialogFunc)
{
    return DialogBoxParam(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L);
}

inline INT_PTR PropertySheet(LPCPROPSHEETHEADER lppsph)
{
    _InitComCtl32();
#ifdef UNICODE
    return PropertySheetW(lppsph);
#else
    return PropertySheetA(lppsph);
#endif
}

#ifdef __cplusplus
};
#endif

#endif  UTILS_H
