#ifndef _ASSOC_H_
#define _ASSOC_H_

// STDAPI AssociateMIMEA(HWND hwndParent, DWORD dwInFlags, LPCSTR pcszFile, LPCSTR pcszMIMEContentType, LPSTR pszAppBuf, UINT cchAppBuf);
// STDAPI AssociateMIMEW(HWND hwndParent, DWORD dwInFlags, LPCWSTR pcszFile, LPCWSTR pcszMIMEContentType, LPWSTR pszAppBuf, UINT cchAppBuf);

STDAPI AssociateURLA(HWND hwndParent, DWORD dwInFlags, LPCSTR pcszFile, LPCSTR pcszURL, LPSTR pszAppBuf, UINT cchAppBuf);
STDAPI AssociateURLW(HWND hwndParent, DWORD dwInFlags, LPCWSTR pcszFile, LPCWSTR pcszURL, LPWSTR pszAppBuf, UINT cchAppBuf);

#ifdef UNICODE
// #define AssociateMIME     AssociateMIMEW
#define AssociateURL      AssociateURLW
#else
// #define AssociateMIME     AssociateMIMEA
#define AssociateURL      AssociateURLA
#endif

#endif // _ASSOC_H_
