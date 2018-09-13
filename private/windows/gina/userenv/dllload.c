//*************************************************************
//
//  DLL loading functions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "uenv.h"

//
// file global variables containing pointers to APIs and
// loaded modules
//

NETAPI32_API    g_NetApi32Api;
SECUR32_API     g_Secur32Api;
LDAP_API        g_LdapApi;
ICMP_API        g_IcmpApi;
WSOCK32_API     g_WSock32Api;
DS_API          g_DsApi;
SHELL32_API     g_Shell32Api;
SHLWAPI_API     g_ShlwapiApi;

CRITICAL_SECTION g_ApiDLLCritSec;

//*************************************************************
//
//  InitializeAPIs()
//
//  Purpose:    initializes API structures for delay loaded
//              modules
//
//  Parameters: none
//
//
//  Return:     none
//
//*************************************************************

void InitializeAPIs( void )
{
    ZeroMemory( &g_NetApi32Api, sizeof( NETAPI32_API ) );
    ZeroMemory( &g_Secur32Api,  sizeof( SECUR32_API ) );
    ZeroMemory( &g_LdapApi,     sizeof( LDAP_API ) );
    ZeroMemory( &g_IcmpApi,     sizeof( ICMP_API ) );
    ZeroMemory( &g_WSock32Api,  sizeof( WSOCK32_API ) );
    ZeroMemory( &g_DsApi,       sizeof( DS_API ) );
    ZeroMemory( &g_Shell32Api,  sizeof( SHELL32_API ) );
    ZeroMemory( &g_ShlwapiApi,  sizeof( SHLWAPI_API ) );
}

//*************************************************************
//
//  InitializeApiDLLsCritSec()
//
//  Purpose:    initializes a CRITICAL_SECTION for synch'ing
//              DLL loads
//
//  Parameters: none
//
//
//  Return:     none
//
//*************************************************************

void InitializeApiDLLsCritSec( void )
{
    InitializeCriticalSection( &g_ApiDLLCritSec );
}

//*************************************************************
//
//  CloseApiDLLsCritSec()
//
//  Purpose:    clean up CRITICAL_SECTION for synch'ing
//              DLL loads
//
//  Parameters: none
//
//
//  Return:     none
//
//*************************************************************

void CloseApiDLLsCritSec( void )
{
    DeleteCriticalSection( &g_ApiDLLCritSec );
}

//*************************************************************
//
//  LoadNetAPI32()
//
//  Purpose:    Loads netapi32.dll
//
//  Parameters: pNETAPI32 - pointer to a NETAPI32_API structure to
//                         initialize
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

PNETAPI32_API LoadNetAPI32 ()
{
    BOOL bResult = FALSE;
    PNETAPI32_API pNetAPI32 = &g_NetApi32Api;

    EnterCriticalSection( &g_ApiDLLCritSec );
    
    if ( pNetAPI32->hInstance ) {
        //
        // module is already loaded and initialized
        //
        LeaveCriticalSection( &g_ApiDLLCritSec );

        return pNetAPI32;
    }
    
    pNetAPI32->hInstance = LoadLibrary (TEXT("netapi32.dll"));

    if (!pNetAPI32->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to load netapi32 with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnDsGetDcName = (PFNDSGETDCNAME) GetProcAddress (pNetAPI32->hInstance,
#ifdef UNICODE
                                                                 "DsGetDcNameW");
#else
                                                                 "DsGetDcNameA");
#endif

    if (!pNetAPI32->pfnDsGetDcName) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find DsGetDcName with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnDsGetSiteName = (PFNDSGETSITENAME) GetProcAddress (pNetAPI32->hInstance,
#ifdef UNICODE
                                                                     "DsGetSiteNameW");
#else
                                                                     "DsGetSiteNameA");
#endif

    if (!pNetAPI32->pfnDsGetSiteName) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find DsGetSiteName with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnDsRoleGetPrimaryDomainInformation = (PFNDSROLEGETPRIMARYDOMAININFORMATION)GetProcAddress (pNetAPI32->hInstance,
                                                       "DsRoleGetPrimaryDomainInformation");

    if (!pNetAPI32->pfnDsRoleGetPrimaryDomainInformation) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find pfnDsRoleGetPrimaryDomainInformation with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnDsRoleFreeMemory = (PFNDSROLEFREEMEMORY)GetProcAddress (pNetAPI32->hInstance,
                                                                          "DsRoleFreeMemory");

    if (!pNetAPI32->pfnDsRoleFreeMemory) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find pfnDsRoleFreeMemory with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnNetApiBufferFree = (PFNNETAPIBUFFERFREE) GetProcAddress (pNetAPI32->hInstance,
                                                                           "NetApiBufferFree");

    if (!pNetAPI32->pfnNetApiBufferFree) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find NetApiBufferFree with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnNetUserGetGroups = (PFNNETUSERGETGROUPS) GetProcAddress (pNetAPI32->hInstance,
                                                                           "NetUserGetGroups");

    if (!pNetAPI32->pfnNetUserGetGroups) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find NetUserGetGroups with %d."),
                 GetLastError()));
        goto Exit;
    }


    pNetAPI32->pfnNetUserGetInfo = (PFNNETUSERGETINFO) GetProcAddress (pNetAPI32->hInstance,
                                                                      "NetUserGetInfo");

    if (!pNetAPI32->pfnNetUserGetInfo) {
        DebugMsg((DM_WARNING, TEXT("LoadNetAPI32:  Failed to find NetUserGetInfo with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        LogEvent (TRUE, IDS_FAILED_NETAPI32, GetLastError());
        if ( pNetAPI32->hInstance ) {
            FreeLibrary( pNetAPI32->hInstance );
        }
        ZeroMemory( pNetAPI32, sizeof( NETAPI32_API ) );
        pNetAPI32 = 0;
    }

    LeaveCriticalSection( &g_ApiDLLCritSec );

    return pNetAPI32;
}


//*************************************************************
//
//  LoadSecur32()
//
//  Purpose:    Loads secur32.dll
//
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

PSECUR32_API LoadSecur32 ()
{
    BOOL bResult = FALSE;
    PSECUR32_API pSecur32 = &g_Secur32Api;

    EnterCriticalSection( &g_ApiDLLCritSec );
    
    if ( pSecur32->hInstance ) {
        //
        // module is already loaded and initialized
        //
        LeaveCriticalSection( &g_ApiDLLCritSec );

        return pSecur32;
    }
    
    //
    // Load secur32.dll
    //

    pSecur32->hInstance = LoadLibrary (TEXT("secur32.dll"));

    if (!pSecur32->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to load secur32 with %d."),
                 GetLastError()));
        goto Exit;
    }

    pSecur32->pfnGetUserNameEx = (PFNGETUSERNAMEEX)GetProcAddress (pSecur32->hInstance,
#ifdef UNICODE
                                        "GetUserNameExW");
#else
                                        "GetUserNameExA");
#endif


    if (!pSecur32->pfnGetUserNameEx) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find GetUserNameEx with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnGetComputerObjectName = (PFNGETCOMPUTEROBJECTNAME)GetProcAddress (pSecur32->hInstance,
#ifdef UNICODE
                                        "GetComputerObjectNameW");
#else
                                        "GetComputerObjectNameA");
#endif


    if (!pSecur32->pfnGetComputerObjectName) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find GetComputerObjectName with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnAcceptSecurityContext = (ACCEPT_SECURITY_CONTEXT_FN)GetProcAddress (pSecur32->hInstance,
                                        "AcceptSecurityContext");

    if (!pSecur32->pfnAcceptSecurityContext) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find AcceptSecurityContext with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnAcquireCredentialsHandle = (ACQUIRE_CREDENTIALS_HANDLE_FN)GetProcAddress (pSecur32->hInstance,
#ifdef UNICODE
                                        "AcquireCredentialsHandleW");
#else
                                        "AcquireCredentialsHandleA");
#endif


    if (!pSecur32->pfnAcquireCredentialsHandle) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find AcquireCredentialsHandle with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnDeleteSecurityContext = (DELETE_SECURITY_CONTEXT_FN)GetProcAddress (pSecur32->hInstance,
                                        "DeleteSecurityContext");

    if (!pSecur32->pfnDeleteSecurityContext) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find DeleteSecurityContext with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnFreeContextBuffer = (FREE_CONTEXT_BUFFER_FN)GetProcAddress (pSecur32->hInstance,
                                        "FreeContextBuffer");

    if (!pSecur32->pfnFreeContextBuffer) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find FreeContextBuffer with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnFreeCredentialsHandle = (FREE_CREDENTIALS_HANDLE_FN)GetProcAddress (pSecur32->hInstance,
                                        "FreeCredentialsHandle");

    if (!pSecur32->pfnFreeCredentialsHandle) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find FreeCredentialsHandle with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnInitializeSecurityContext = (INITIALIZE_SECURITY_CONTEXT_FN)GetProcAddress (pSecur32->hInstance,
#ifdef UNICODE
                                        "InitializeSecurityContextW");
#else
                                        "InitializeSecurityContextA");
#endif


    if (!pSecur32->pfnInitializeSecurityContext) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find InitializeSecurityContext with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnQuerySecurityContextToken = (QUERY_SECURITY_CONTEXT_TOKEN_FN)GetProcAddress (pSecur32->hInstance,
                                        "QuerySecurityContextToken");

    if (!pSecur32->pfnQuerySecurityContextToken) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find QuerySecurityContextToken with %d."),
                 GetLastError()));
        goto Exit;
    }


    pSecur32->pfnQuerySecurityPackageInfo = (QUERY_SECURITY_PACKAGE_INFO_FN)GetProcAddress (pSecur32->hInstance,
#ifdef UNICODE
                                        "QuerySecurityPackageInfoW");
#else
                                        "QuerySecurityPackageInfoA");
#endif


    if (!pSecur32->pfnQuerySecurityPackageInfo) {
        DebugMsg((DM_WARNING, TEXT("LoadSecur32:  Failed to find QuerySecurityPackageInfo with %d."),
                 GetLastError()));
        goto Exit;
    }



    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        LogEvent (TRUE, IDS_FAILED_SECUR32, GetLastError());
        if ( pSecur32->hInstance ) {
            FreeLibrary( pSecur32->hInstance );
        }
        ZeroMemory( pSecur32, sizeof( SECUR32_API ) );
        pSecur32 = 0;
    }

    LeaveCriticalSection( &g_ApiDLLCritSec );

    return pSecur32;
}


//*************************************************************
//
//  LoadLDAP()
//
//  Purpose:    Loads wldap32.dll
//
//  Parameters: pLDAP - pointer to a  LDAP_API structure to
//                      initialize
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

PLDAP_API LoadLDAP ()
{
    BOOL bResult = FALSE;
    PLDAP_API pLDAP = &g_LdapApi;

    EnterCriticalSection( &g_ApiDLLCritSec );
    
    if ( pLDAP->hInstance ) {
        //
        // module is already loaded and initialized
        //
        LeaveCriticalSection( &g_ApiDLLCritSec );

        return pLDAP;
    }

    //
    // Load wldap32.dll
    //

    pLDAP->hInstance = LoadLibrary (TEXT("wldap32.dll"));

    if (!pLDAP->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to load wldap32 with %d."),
                 GetLastError()));
        goto Exit;
    }

    pLDAP->pfnldap_open = (PFNLDAP_OPEN) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_openW");
#else
                                        "ldap_openA");
#endif

    if (!pLDAP->pfnldap_open) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_open with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_bind_s = (PFNLDAP_BIND_S) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_bind_sW");
#else
                                        "ldap_bind_sA");
#endif

    if (!pLDAP->pfnldap_bind_s) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_bind_s with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_search_s = (PFNLDAP_SEARCH_S) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_search_sW");
#else
                                        "ldap_search_sA");
#endif

    if (!pLDAP->pfnldap_search_s) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_search_s with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_search_ext_s = (PFNLDAP_SEARCH_EXT_S) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_search_ext_sW");
#else
                                        "ldap_search_ext_sA");
#endif

    if (!pLDAP->pfnldap_search_ext_s) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_search_ext_s with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_get_values = (PFNLDAP_GET_VALUES) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_get_valuesW");
#else
                                        "ldap_get_valuesA");
#endif

    if (!pLDAP->pfnldap_get_values) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_get_values with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_value_free = (PFNLDAP_VALUE_FREE) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_value_freeW");
#else
                                        "ldap_value_freeA");
#endif

    if (!pLDAP->pfnldap_value_free) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_value_free with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_get_values_len = (PFNLDAP_GET_VALUES_LEN) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_get_values_lenW");
#else
                                        "ldap_get_values_lenA");
#endif

    if (!pLDAP->pfnldap_get_values_len) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_get_values_len with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_value_free_len = (PFNLDAP_VALUE_FREE_LEN) GetProcAddress (pLDAP->hInstance,
                                        "ldap_value_free_len");

    if (!pLDAP->pfnldap_value_free_len) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_value_free_len with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_msgfree = (PFNLDAP_MSGFREE) GetProcAddress (pLDAP->hInstance,
                                        "ldap_msgfree");

    if (!pLDAP->pfnldap_msgfree) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_msgfree with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_unbind = (PFNLDAP_UNBIND) GetProcAddress (pLDAP->hInstance,
                                        "ldap_unbind");

    if (!pLDAP->pfnldap_unbind) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_unbind with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnLdapGetLastError = (PFNLDAPGETLASTERROR) GetProcAddress (pLDAP->hInstance,
                                        "LdapGetLastError");

    if (!pLDAP->pfnLdapGetLastError) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find pfnLdapGetLastError with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_first_entry = (PFNLDAP_FIRST_ENTRY) GetProcAddress (pLDAP->hInstance,
                                        "ldap_first_entry");

    if (!pLDAP->pfnldap_first_entry) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_first_entry with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_next_entry = (PFNLDAP_NEXT_ENTRY) GetProcAddress (pLDAP->hInstance,
                                        "ldap_next_entry");

    if (!pLDAP->pfnldap_next_entry) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_next_entry with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_get_dn = (PFNLDAP_GET_DN) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_get_dnW");
#else
                                        "ldap_get_dnA");
#endif

    if (!pLDAP->pfnldap_get_dn) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_get_dn with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_set_option = (PFNLDAP_SET_OPTION) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_set_optionW");
#else
                                        "ldap_set_option");
#endif

    if (!pLDAP->pfnldap_set_option) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_set_option with %d."),
                 GetLastError()));
        goto Exit;
    }


    pLDAP->pfnldap_memfree = (PFNLDAP_MEMFREE) GetProcAddress (pLDAP->hInstance,
#ifdef UNICODE
                                        "ldap_memfreeW");
#else
                                        "ldap_memfreeA");
#endif

    if (!pLDAP->pfnldap_memfree) {
        DebugMsg((DM_WARNING, TEXT("LoadLDAP:  Failed to find ldap_memfree with %d."),
                 GetLastError()));
        goto Exit;
    }


    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        LogEvent (TRUE, IDS_FAILED_WLDAP32, GetLastError());
        if ( pLDAP->hInstance ) {
            FreeLibrary( pLDAP->hInstance );
        }
        ZeroMemory( pLDAP, sizeof( LDAP_API ) );

        pLDAP = 0;
    }

    LeaveCriticalSection( &g_ApiDLLCritSec );

    return pLDAP;
}


//*************************************************************
//
//  LoadIcmp()
//
//  Purpose:    Loads cmp.dll
//
//  Parameters: pIcmp - pointer to a ICMP_API structure to
//                         initialize
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

PICMP_API LoadIcmp ()
{
    BOOL bResult = FALSE;
    PICMP_API pIcmp = &g_IcmpApi;

    EnterCriticalSection( &g_ApiDLLCritSec );
    
    if ( pIcmp->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( &g_ApiDLLCritSec );

        return pIcmp;
    }

    pIcmp->hInstance = LoadLibrary (TEXT("icmp.dll"));

    if (!pIcmp->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadIcmp:  Failed to load icmp with %d."),
                 GetLastError()));
        goto Exit;
    }


    pIcmp->pfnIcmpCreateFile = (PFNICMPCREATEFILE) GetProcAddress (pIcmp->hInstance,
                                                                   "IcmpCreateFile");

    if (!pIcmp->pfnIcmpCreateFile) {
        DebugMsg((DM_WARNING, TEXT("LoadIcmp:  Failed to find IcmpCreateFile with %d."),
                 GetLastError()));
        goto Exit;
    }


    pIcmp->pfnIcmpCloseHandle = (PFNICMPCLOSEHANDLE) GetProcAddress (pIcmp->hInstance,
                                                                   "IcmpCloseHandle");

    if (!pIcmp->pfnIcmpCloseHandle) {
        DebugMsg((DM_WARNING, TEXT("LoadIcmp:  Failed to find IcmpCloseHandle with %d."),
                 GetLastError()));
        goto Exit;
    }


    pIcmp->pfnIcmpSendEcho = (PFNICMPSENDECHO) GetProcAddress (pIcmp->hInstance,
                                                                   "IcmpSendEcho");

    if (!pIcmp->pfnIcmpSendEcho) {
        DebugMsg((DM_WARNING, TEXT("LoadIcmp:  Failed to find IcmpSendEcho with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        LogEvent (TRUE, IDS_FAILED_ICMP, GetLastError());
        if ( pIcmp->hInstance ) {
            FreeLibrary( pIcmp->hInstance );
        }
        ZeroMemory( pIcmp, sizeof( ICMP_API ) );

        pIcmp = 0;
    }

    LeaveCriticalSection( &g_ApiDLLCritSec );

    return pIcmp;
}


//*************************************************************
//
//  LoadWSock()
//
//  Purpose:    Loads cmp.dll
//
//  Parameters: pWSock32 - pointer to a WSOCK32_API structure to
//                         initialize
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

PWSOCK32_API LoadWSock32 ()
{
    BOOL bResult = FALSE;
    PWSOCK32_API pWSock32 = &g_WSock32Api;

    EnterCriticalSection( &g_ApiDLLCritSec );
    
    if ( pWSock32->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( &g_ApiDLLCritSec );

        return pWSock32;
    }

    pWSock32->hInstance = LoadLibrary (TEXT("wsock32.dll"));

    if (!pWSock32->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadWSock32:  Failed to load wsock32 with %d."),
                 GetLastError()));
        goto Exit;
    }


    pWSock32->pfninet_addr = (LPFN_INET_ADDR) GetProcAddress (pWSock32->hInstance,
                                                                   "inet_addr");

    if (!pWSock32->pfninet_addr) {
        DebugMsg((DM_WARNING, TEXT("LoadWSock32:  Failed to find inet_addr with %d."),
                 GetLastError()));
        goto Exit;
    }


    pWSock32->pfngethostbyname = (LPFN_GETHOSTBYNAME) GetProcAddress (pWSock32->hInstance,
                                                                   "gethostbyname");

    if (!pWSock32->pfngethostbyname) {
        DebugMsg((DM_WARNING, TEXT("LoadWSock32:  Failed to find gethostbyname with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        LogEvent (TRUE, IDS_FAILED_WSOCK32, GetLastError());
        if ( pWSock32->hInstance ) {
            FreeLibrary( pWSock32->hInstance );
        }
        ZeroMemory( pWSock32, sizeof( WSOCK32_API ) );

        pWSock32 = 0;
    }

    LeaveCriticalSection( &g_ApiDLLCritSec );

    return pWSock32;
}

//*************************************************************
//
//  LoadDSAPI()
//
//  Purpose:    Loads ntdsapi.dll
//
//  Parameters: pDSApi - pointer to a DS_API structure to initialize
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

PDS_API LoadDSApi()
{
    BOOL bResult = FALSE;
    PDS_API pDSApi = &g_DsApi;
    
    EnterCriticalSection( &g_ApiDLLCritSec );
    
    if ( pDSApi->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( &g_ApiDLLCritSec );

        return pDSApi;
    }
    
    pDSApi->hInstance = LoadLibrary (TEXT("ntdsapi.dll"));

    if (!pDSApi->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadDSApi:  Failed to load ntdsapi with %d."),
                 GetLastError()));
        goto Exit;
    }


    pDSApi->pfnDsCrackNames = (PFN_DSCRACKNAMES) GetProcAddress (pDSApi->hInstance,
#ifdef UNICODE
                                                                 "DsCrackNamesW");
#else
                                                                 "DsCrackNamesA");
#endif

    if (!pDSApi->pfnDsCrackNames) {
        DebugMsg((DM_WARNING, TEXT("LoadDSApi:  Failed to find DsCrackNames with %d."),
                 GetLastError()));
        goto Exit;
    }


    pDSApi->pfnDsFreeNameResult = (PFN_DSFREENAMERESULT) GetProcAddress (pDSApi->hInstance,
#ifdef UNICODE
                                                                 "DsFreeNameResultW");
#else
                                                                 "DsFreeNameResultA");
#endif

    if (!pDSApi->pfnDsFreeNameResult) {
        DebugMsg((DM_WARNING, TEXT("LoadDSApi:  Failed to find DsFreeNameResult with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        LogEvent (TRUE, IDS_FAILED_DSAPI, GetLastError());
        if ( pDSApi->hInstance ) {
            FreeLibrary( pDSApi->hInstance );
        }
        ZeroMemory( pDSApi, sizeof( DS_API ) );

        pDSApi = 0;
    }

    LeaveCriticalSection( &g_ApiDLLCritSec );
    
    return pDSApi;
}

//*************************************************************
//
//  LoadShlwapiPI()
//
//  Purpose:    Loads shell32.dll
//
//  Parameters: nonde
//
//  Return:     pointer to SHELL32_API
//
//*************************************************************

PSHELL32_API LoadShell32Api()
{
    BOOL bResult = FALSE;
    PSHELL32_API pShell32Api = &g_Shell32Api;
    
    EnterCriticalSection( &g_ApiDLLCritSec );
    
    if ( pShell32Api->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( &g_ApiDLLCritSec );

        return pShell32Api;
    }
    
    pShell32Api->hInstance = LoadLibrary (TEXT("shell32.dll"));

    if (!pShell32Api->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to load ntdsapi with %d."),
                 GetLastError()));
        goto Exit;
    }


    pShell32Api->pfnShChangeNotify = (PFNSHCHANGENOTIFY) GetProcAddress (pShell32Api->hInstance, "SHChangeNotify");

    if (!pShell32Api->pfnShChangeNotify) {
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find SHChangeNotify with %d."),
                 GetLastError()));
        goto Exit;
    }

    pShell32Api->pfnShGetSpecialFolderPath = (PFNSHGETSPECIALFOLDERPATH) GetProcAddress (pShell32Api->hInstance,
#ifdef UNICODE
                                                                 "SHGetSpecialFolderPathW");
#else
                                                                 "SHGetSpecialFolderPathA");
#endif

    if (!pShell32Api->pfnShGetSpecialFolderPath) {
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find SHGetSpecialFolderPath with %d."),
                 GetLastError()));
        goto Exit;
    }

    pShell32Api->pfnShGetFolderPath = (PFNSHGETFOLDERPATH) GetProcAddress (pShell32Api->hInstance,
#ifdef UNICODE
                                                                 "SHGetFolderPathW");
#else
                                                                 "SHGetFolderPathA");
#endif


    if (!pShell32Api->pfnShGetFolderPath) {
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find SHGetFolderPath with %d."),
                 GetLastError()));
        goto Exit;
    }


    pShell32Api->pfnShSetFolderPath = (PFNSHSETFOLDERPATH) GetProcAddress (pShell32Api->hInstance,
#ifdef UNICODE
                                                                 (LPCSTR)SHSetFolderW_Ord);
#else
                                                                 (LPCSTR)SHSetFolderA_Ord);
#endif

    if (!pShell32Api->pfnShSetFolderPath) {
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find SHSetFolderPath with %d."),
                 GetLastError()));
        goto Exit;
    }

    
    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        LogEvent (TRUE, IDS_FAILED_SHELL32API, GetLastError());
        if ( pShell32Api->hInstance ) {
            FreeLibrary( pShell32Api->hInstance );
        }
        ZeroMemory( pShell32Api, sizeof( SHELL32_API ) );

        pShell32Api = 0;
    }

    LeaveCriticalSection( &g_ApiDLLCritSec );
    
    return pShell32Api;
}


//*************************************************************
//
//  LoadShwapiAPI()
//
//  Purpose:    Loads shlwapi.dll
//
//  Parameters: nonde
//
//  Return:     pointer to SHLWAPI_API
//
//*************************************************************

PSHLWAPI_API LoadShlwapiApi()
{
    BOOL bResult = FALSE;
    PSHLWAPI_API pShlwapiApi = &g_ShlwapiApi;
    
    EnterCriticalSection( &g_ApiDLLCritSec );
    
    if ( pShlwapiApi->hInstance ) {
        //
        // module already loaded and initialized
        //
        LeaveCriticalSection( &g_ApiDLLCritSec );

        return pShlwapiApi;
    }
    
    pShlwapiApi->hInstance = LoadLibrary (TEXT("shlwapi.dll"));

    if (!pShlwapiApi->hInstance) {
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to load ntdsapi with %d."),
                 GetLastError()));
        goto Exit;
    }


    pShlwapiApi->pfnPathGetArgs = (PFNPATHGETARGS) GetProcAddress (pShlwapiApi->hInstance,
#ifdef UNICODE
                                                                 "PathGetArgsW");
#else
                                                                 "PathGetArgsA");
#endif

    if (!pShlwapiApi->pfnPathGetArgs) {
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find PathGetArgs with %d."),
                 GetLastError()));
        goto Exit;
    }

    pShlwapiApi->pfnPathUnquoteSpaces = (PFNPATHUNQUOTESPACES) GetProcAddress (pShlwapiApi->hInstance,
#ifdef UNICODE
                                                                 "PathUnquoteSpacesW");
#else
                                                                 "PathUnquoteSpacesA");
#endif

    if (!pShlwapiApi->pfnPathUnquoteSpaces) {
        DebugMsg((DM_WARNING, TEXT("LoadShlwapiApi:  Failed to find PathUnquoteSpaces with %d."),
                 GetLastError()));
        goto Exit;
    }

    //
    // Success
    //

    bResult = TRUE;

Exit:

    if (!bResult) {
        LogEvent (TRUE, IDS_FAILED_SHELL32API, GetLastError());
        if ( pShlwapiApi->hInstance ) {
            FreeLibrary( pShlwapiApi->hInstance );
        }
        ZeroMemory( pShlwapiApi, sizeof( SHLWAPI_API ) );

        pShlwapiApi = 0;
    }

    LeaveCriticalSection( &g_ApiDLLCritSec );
    
    return pShlwapiApi;
}

