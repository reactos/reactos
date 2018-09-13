/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    auth.h

Abstract:

    Private include file for 

Author:

    Rajeev Dujari (rajeevd) 28-Jul-97
    
Revision History:

--*/

//
// manifests
//
 
#define HTTP_AUTHORIZATION_SZ           "Authorization:"
#define HTTP_AUTHORIZATION_LEN          (sizeof(HTTP_AUTHORIZATION_SZ)-1)

#define HTTP_PROXY_AUTHORIZATION_SZ     "Proxy-Authorization:"
#define HTTP_PROXY_AUTHORIZATION_LEN    (sizeof(HTTP_PROXY_AUTHORIZATION_SZ)-1)


//
// prototypes - versions of spluginx.hxx for basic auth
//


void UrlZonesDetach (void);

#ifdef __cplusplus
extern "C" {
#endif

extern DWORD g_cSspiContexts; // refcount of sspi contexts

DWORD SSPI_Unload();

DWORD_PTR SSPI_InitScheme (LPSTR pszScheme);

#ifdef __cplusplus
} // end extern "C" {
#endif
