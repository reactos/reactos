// cookie.h - header for external cookie funcs code.


BOOL OpenTheCookieJar();
void CloseTheCookieJar();
VOID PurgeCookieJarOfStaleCookies();


#define COOKIE_SECURE   INTERNET_COOKIE_IS_SECURE
#define COOKIE_SESSION  INTERNET_COOKIE_IS_SESSION // never saved to disk
#define COOKIE_NOUI     4

BOOL InternetGetCookieEx( LPCSTR pchURL, LPCSTR pchCookieName, LPSTR pchCookieData OPTIONAL,
                          LPDWORD pcchCookieData, DWORD dwFlags, LPVOID lpReserved);


BOOL InternetSetCookieEx( LPCSTR  pchURL, LPCSTR  pchCookieName, LPCSTR  pchCookieData,
                          DWORD dwFlags, LPVOID lpReserved);                         
