#ifndef __CDFSUBS_HPP_INCLUDED__
#define __CDFSUBS_HPP_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

BOOL CreateCDFCacheEntry(LPSTR szCDFUrl, LPSTR szLocalFileName);
BOOL SubscribeChannel(HWND hwnd, LPSTR szName, LPSTR szURL, BOOL bSilent);
BOOL WINAPI CDFAutoSubscribe(HWND hwnd, LPSTR szFriendlyName, LPSTR szCDFUrl,
                             LPSTR szCDFFilePath, BOOL bSilent);
HRESULT Ansi2Unicode(const char * src, wchar_t **dest);

#ifdef __cplusplus
}
#endif

#endif
