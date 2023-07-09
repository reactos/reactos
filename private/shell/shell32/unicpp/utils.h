#ifndef _UTILS_H_
#define _UTILS_H_

BOOL IsExplorerWindow(HWND hwnd);
BOOL IsFolderWindow(HWND hwnd);
BOOL IsTrayWindow(HWND hwnd);

BOOL MyInternetSetOption(HANDLE h, DWORD dw1, LPVOID lpv, DWORD dw2);
STDAPI_(BOOL) IsDesktopWindow(HWND hwnd);

STDAPI IsSafePage(IUnknown *punkSite);

#endif // _UTILS_H_

