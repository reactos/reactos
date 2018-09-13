#ifndef __strings_h
#define __strings_h

HRESULT LocalAllocString(LPTSTR* ppResult, LPCTSTR pString);
HRESULT LocalAllocStringLen(LPTSTR* ppResult, UINT cLen);
void    LocalFreeString(LPTSTR* ppString);
HRESULT LocalQueryString(LPTSTR* ppResult, HKEY hk, LPCTSTR lpSubKey);

HRESULT StrRetFromString(LPSTRRET lpStrRet, LPCTSTR pString);

#endif
