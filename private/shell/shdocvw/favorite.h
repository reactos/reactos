
#ifndef _FAVORITE_H_
#define _FAVORITE_H_

STDAPI        AddToFavorites(HWND hwnd, LPCITEMIDLIST pidlCur, LPCTSTR pszTitle, BOOL fDisplayUI, IOleCommandTarget *pCommandTarget, IHTMLDocument2 *pDoc);
STDAPI_(BOOL) DoOrganizeFavDlg(HWND hwnd, LPSTR pszInitDir);
STDAPI_(BOOL) DoOrganizeFavDlgW(HWND hwnd, LPWSTR pszInitDir);

BOOL IsSubscribed(LPCITEMIDLIST pidlCur);
BOOL IsSubscribed(LPWSTR pwzUrl);

#endif 
