#ifndef _BRUTIL_H_
#define _BRUTIL_H_

#include <shldispp.h>

STDAPI_(BOOL) IsBrowseNewProcess();
STDAPI_(BOOL) IsBrowseNewProcessAndExplorer();
STDAPI IENavigateIEProcess(LPCTSTR pszPath, BOOL fIsInternetShortcut);
STDAPI NavFrameWithFile(LPCTSTR pszPath, IUnknown *punk);
STDAPI GetPathForItem(IShellFolder *psf, LPCITEMIDLIST pidl, LPTSTR pszPath, DWORD *pdwAttrib);
STDAPI EditBox_TranslateAcceleratorST(LPMSG lpmsg);
STDAPI _CLSIDFromExtension(LPCTSTR pszExt, CLSID *pclsid);
STDAPI GetLinkTargetIDList(LPCTSTR pszPath, LPTSTR pszTarget, DWORD cchTarget, LPITEMIDLIST *ppidl);
STDAPI_(void) PathToDisplayNameW(LPCTSTR pszPath, LPTSTR pszDisplayName, UINT cchDisplayName);
STDAPI_(void) PathToDisplayNameA(LPSTR pszPathA, LPSTR pszDisplayNameA, int cchDisplayName);
STDAPI DataObj_GetNameFromFileDescriptor(IDataObject *pdtobj, LPWSTR pszDisplayName, UINT cch);
STDAPI SHPidlFromDataObject2(IDataObject *pdtobj, LPITEMIDLIST * ppidl);
STDAPI SHPidlFromDataObject(IDataObject *pdtobj, LPITEMIDLIST *ppidl, LPWSTR pszDisplayNameW, DWORD cchDisplayName);
STDAPI_(LRESULT) SendShellIEBroadcastMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, UINT uTimeout);
STDAPI IEBindToParentFolder(LPCITEMIDLIST pidl, IShellFolder** ppsfParent, LPCITEMIDLIST *ppidlChild);
STDAPI GetDataObjectForPidl(LPCITEMIDLIST pidl, IDataObject ** ppdtobj);
STDAPI_(BOOL) ILIsFileSysFolder(LPCITEMIDLIST pidl);
STDAPI SHTitleFromPidl(LPCITEMIDLIST pidl, LPTSTR psz, DWORD cch, BOOL fFullPath);
STDAPI_(BOOL) IsBrowserFrameOptionsSet(IN IShellFolder * psf, IN BROWSERFRAMEOPTIONS dwMask);
STDAPI_(BOOL) IsBrowserFrameOptionsPidlSet(IN LPCITEMIDLIST pidl, IN BROWSERFRAMEOPTIONS dwMask);
STDAPI GetBrowserFrameOptions(IN IUnknown *punk, IN BROWSERFRAMEOPTIONS dwMask, OUT BROWSERFRAMEOPTIONS * pdwOptions);
STDAPI GetBrowserFrameOptionsPidl(IN LPCITEMIDLIST pidl, IN BROWSERFRAMEOPTIONS dwMask, OUT BROWSERFRAMEOPTIONS * pdwOptions);
STDAPI_(BOOL) IsFTPFolder(IShellFolder * psf);

// non-munging menu operations to work around the menu munging code
// in the shlwapi wrappers. for more info see the comment in the brutil.cpp.

STDAPI_(HMENU)  LoadMenu_PrivateNoMungeW(HINSTANCE hInstance, LPCWSTR lpMenuName);
STDAPI_(BOOL)   InsertMenu_PrivateNoMungeW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCWSTR lpNewItem);
STDAPI_(HMENU)  LoadMenuPopup_PrivateNoMungeW(UINT id);

//encode any incoming %1 so that people can't spoof our domain security code
HRESULT WrapSpecialUrl(BSTR * pbstrUrl);
HRESULT WrapSpecialUrlFlat(LPWSTR pszUrl, DWORD cchUrl);
BOOL IsSpecialUrl(LPCWSTR pchURL);

//
//      GetUIVersion()
//
//  returns the version of shell32
//  3 == win95 gold / NT4
//  4 == IE4 Integ / win98
//  5 == win2k
//

STDAPI_(UINT) GetUIVersion();

#endif // _BRUTIL_H_
