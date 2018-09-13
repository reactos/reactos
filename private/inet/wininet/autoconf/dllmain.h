#include <windows.h>
#include <olectl.h>
#include "cscpsite.h"

#ifdef unix
extern "C"
#endif /* unix */

typedef struct {

    //
    // Size of struct
    //

    DWORD dwStructSize;

    //
    // Buffer to Pass
    //

    LPSTR lpszScriptBuffer;

    //
    // Size of buffer above
    //

    DWORD dwScriptBufferSize;

}  AUTO_PROXY_EXTERN_STRUC, *LPAUTO_PROXY_EXTERN_STRUC;


BOOL CALLBACK InternetInitializeAutoProxyDll(DWORD dwVersion,
											 LPSTR lpszDownloadedTempFile,
											 LPSTR lpszMime,
											 AUTO_PROXY_HELPER_APIS *pAutoProxyCallbacks,
											 LPAUTO_PROXY_EXTERN_STRUC lpExtraData);
// This function frees the script engine and destroys the script site.

#ifdef unix
extern "C"
#endif  /* unix */
BOOL CALLBACK InternetDeInitializeAutoProxyDll(LPSTR lpszMime,DWORD dwReserved);
// This function is called when the host wants to run the script.
#ifndef unix
BOOL CALLBACK InternetGetProxyInfo(LPCSTR lpszUrl,
				   DWORD dwUrlLength,
				   LPCSTR lpszUrlHostName,
				   DWORD dwUrlHostNameLength,
				   LPSTR *lplpszProxyHostName,
				   LPDWORD lpdwProxyHostNameLength);
#endif /* unix */
 
