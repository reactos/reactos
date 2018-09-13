/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    secinit.cxx

Abstract:

    Contains load function for security.dll on NT and secur32.dll on win95
    Also handles WinTrust.dll function loading.

Author:

    Sophia Chung (sophiac)  6-Feb-1996

Environment:

    User Mode - Win32

Revision History:

--*/
#include <wininetp.h>

//
// InitializationLock - protects against multiple threads loading security.dll
// (secur32.dll) and entry points
//

CRITICAL_SECTION InitializationSecLock = {0};

CRITICAL_SECTION InitFortezzaLock = {0};

HCRYPTPROV  GlobalFortezzaCryptProv;

//
// GlobalSecFuncTable - Pointer to Global Structure of Pointers that are used
//  for storing the entry points into the SCHANNEL.dll
//

PSecurityFunctionTable GlobalSecFuncTable = NULL;

//
// pWinVerifyTrust - Pointer to Entry Point in WINTRUST.DLL
//

WIN_VERIFY_TRUST_FN pWinVerifyTrust;
WT_HELPER_PROV_DATA_FROM_STATE_DATA_FN pWTHelperProvDataFromStateData;

//
// pSslCrackCertificate - Pointer to SCHANNEL.dll utility function that
//      is used for parsing X509 certificates.
//

SSL_CRACK_CERTIFICATE_FN pSslCrackCertificate;

//
// pSslFreeCertificate - Pointer to Schannel.dll function for freeing Certs
//

SSL_FREE_CERTIFICATE_FN  pSslFreeCertificate;

//
// hSecurity - NULL when security.dll/secur32.dll  is not loaded
//

HINSTANCE hSecurity = NULL;

//
// hWinTrust - NULL when WinTrust DLL is not loaded.
//

HINSTANCE hWinTrust = NULL;
BOOL g_fDoSpecialMagicForSGCCerts = FALSE;

HCERTSTORE g_hMyCertStore = NULL;
BOOL  g_bOpenMyCertStore = FALSE;

BOOL g_bFortezzaInstalled = FALSE;
BOOL g_bCheckedForFortezza = FALSE;
BOOL g_bAttemptedFortezzaLogin = FALSE;

CRYPT_INSTALL_DEFAULT_CONTEXT_FN g_CryptInstallDefaultContext = NULL;
CRYPT_UNINSTALL_DEFAULT_CONTEXT_FN g_CryptUninstallDefaultContext = NULL;
CERT_FIND_CHAIN_IN_STORE_FN g_CertFindChainInStore = NULL;
CERT_FREE_CERTIFICATE_CHAIN_FN g_CertFreeCertificateChain = NULL;

#define LOCK_FORTEZZA()   EnterCriticalSection( &InitFortezzaLock )
#define UNLOCK_FORTEZZA() LeaveCriticalSection( &InitFortezzaLock )

DWORD
LoadWinTrust(
    VOID
    )

/*++

Routine Description:

    This function loads the WinTrust.DLL and binds a pointer to a function
    that is needed in the WinTrust DLL.

Arguments:

    NONE.

Return Value:

    WINDOWS Error Code.

--*/

{
    DWORD error = ERROR_SUCCESS;

    LOCK_SECURITY();

    if( hWinTrust == NULL )
    {
        LPSTR lpszDllFileName = WINTRUST_DLLNAME;
        pWinVerifyTrust = NULL;

        //
        // Load the DLL
        //

        hWinTrust       = LoadLibrary(lpszDllFileName);

        if ( hWinTrust )
        {
            pWinVerifyTrust = (WIN_VERIFY_TRUST_FN)
                            GetProcAddress(hWinTrust, WIN_VERIFY_TRUST_NAME);
            pWTHelperProvDataFromStateData = (WT_HELPER_PROV_DATA_FROM_STATE_DATA_FN)
                            GetProcAddress(hWinTrust, WT_HELPER_PROV_DATA_FROM_STATE_DATA_NAME);
        }


        if ( !hWinTrust || !pWinVerifyTrust )
        {
            error = GetLastError();

            if ( error == ERROR_SUCCESS )
            {
                error = ERROR_INTERNET_INTERNAL_ERROR;
            }
        }
        {
            // To show SGC certificates we need to do some special magic (see schnlui.cxx) which 
            // depends on some fixes in Wintrust.dll. We have 
            // Figure out the version info for WinTrust.dll
            TCHAR rgchWinTrustFileName[MAX_PATH];
            
            g_fDoSpecialMagicForSGCCerts = FALSE;

            if (GetModuleFileName(hWinTrust, rgchWinTrustFileName, ARRAY_ELEMENTS(rgchWinTrustFileName)) != 0)
            {
                DWORD cbFileVersionBufSize;
                DWORD dwTemp = 0;

                if ((cbFileVersionBufSize = GetFileVersionInfoSize(rgchWinTrustFileName, &dwTemp)) != 0)
                {
                    BYTE* pVerBuffer = NULL;

                    pVerBuffer = (BYTE *) _alloca(cbFileVersionBufSize);
                     
                    if ( (pVerBuffer != NULL) && 
                         (GetFileVersionInfo(rgchWinTrustFileName, 0, cbFileVersionBufSize, pVerBuffer) != 0))
                    {
                        VS_FIXEDFILEINFO *lpVSFixedFileInfo;
                        unsigned uiLength;

                        if( VerQueryValue( pVerBuffer, TEXT("\\"),(LPVOID*)&lpVSFixedFileInfo, &uiLength) != 0
                            && uiLength != 0)
                        {
                            // NT5 Beta3 wintrust version is 5.131.2001.0 which is the Min version we need.
                            // 0x50083 ==> 5.131
                            // 0x7db0000 ==> 2001.0
                            if ((lpVSFixedFileInfo->dwFileVersionMS > 0x50083) 
                                || (lpVSFixedFileInfo->dwFileVersionMS == 0x50083 && lpVSFixedFileInfo->dwFileVersionLS >= 0x07db0000))
                                g_fDoSpecialMagicForSGCCerts = TRUE;
                        }
                    }
                }
            }
        }
            
    }

    INET_ASSERT(pWinVerifyTrust);


    if ( error != ERROR_SUCCESS )
    {
        if (hWinTrust)
        {
            FreeLibrary(hWinTrust);
            hWinTrust = NULL;
        }
    }

    UNLOCK_SECURITY();

    return error;
}



VOID
SecurityInitialize(
    VOID
    )
/*++

Routine Description:

    This function initializes the global lock required for the security
    pkgs.

Arguments:

    NONE.

Return Value:

    WINDOWS Error Code.

--*/
{
    InitializeCriticalSection( &InitializationSecLock );
    InitializeCriticalSection( &InitFortezzaLock );
}

VOID
SecurityTerminate(
    VOID
    )
/*++

Routine Description:

    This function Deletes the global lock required for the security
    pkgs.

Arguments:

    NONE.

Return Value:

    WINDOWS Error Code.

--*/
{
    DeleteCriticalSection(&InitializationSecLock);
    DeleteCriticalSection(&InitFortezzaLock);
}


VOID
UnloadSecurity(
    VOID
    )

/*++

Routine Description:

    This function terminates the global data required for the security
    pkgs and dynamically unloads security APIs from security.dll (NT)
    or secur32.dll (WIN95).

Arguments:

    NONE.

Return Value:

    WINDOWS Error Code.

--*/

{
    DWORD i;

    LOCK_SECURITY();

    //
    //  free all security pkg credential handles
    //

    for (i = 0; SecProviders[i].pszName != NULL; i++) {
         if (SecProviders[i].fEnabled)  {
             if (SecProviders[i].pCertCtxt == NULL && !IsCredClear(SecProviders[i].hCreds)) {
                // Beta1 Hack. Because of some circular dependency between dlls
                // both crypt32 and schannel's PROCESS_DETACH gets called before wininet.
                // This is catastrophic if we have a cert context attached to the credentials
                // handle. In this case we will just leak the handle since the process is dying
                // anyway. We really need to fix this.
                g_FreeCredentialsHandle(&SecProviders[i].hCreds);
            }
         }
#if 0 // See comments above.
         if (SecProviders[i].pCertCtxt != NULL) {
            CertFreeCertificateContext(SecProviders[i].pCertCtxt);
            SecProviders[i].pCertCtxt = NULL;
        }
#endif

    }

    //
    // close cert store. Protect against fault if DLL already unloaded
    //

    __try {
        if (g_hMyCertStore != NULL) {
            CertCloseStore(g_hMyCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }
    ENDEXCEPT
    g_hMyCertStore = NULL;
    g_bOpenMyCertStore = FALSE;

    // IMPORTANT : Don't free GlobalFortezzaCryptProv. When we free the cert context
    // from the SecProviders[] array above it gets freed automatically.
    if (GlobalFortezzaCryptProv != NULL)
    {
        GlobalFortezzaCryptProv = NULL;
    }


    //
    // unload dll
    //

    if (hSecurity != NULL) {
        FreeLibrary(hSecurity);
        hSecurity = NULL;
    }

    UNLOCK_SECURITY();

}

//  
DWORD
ReopenMyCertStore(
        VOID
        )
{
    DWORD Error = ERROR_SUCCESS;
    LOCK_SECURITY();

    if (g_hMyCertStore == NULL) {

        //
        // CRYPT32.DLL is delayloaded. Need SEH in case it fails
        //

        __try {
            g_hMyCertStore = CertOpenSystemStore(0, "MY");
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            Error = GetLastError();
        }
        ENDEXCEPT
    }

    UNLOCK_SECURITY();
    return Error;

}

DWORD
CloseMyCertStore(
	VOID
	)
{
	DWORD Error = ERROR_SUCCESS;

	LOCK_SECURITY();

    //
    // close cert store. Protect against fault if DLL already unloaded
    //

    __try {
        if (g_hMyCertStore != NULL) {
            CertCloseStore(g_hMyCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }
    ENDEXCEPT
    g_hMyCertStore = NULL;
	
	UNLOCK_SECURITY();
	return Error;
}

DWORD
LoadSecurity(
    VOID
    )
/*++

Routine Description:

    This function dynamically loads security APIs from security.dll (NT)
    or secur32.dll (WIN95).

Arguments:

    NONE.

Return Value:

    WINDOWS Error Code.
--*/
{
    DWORD Error = ERROR_SUCCESS;
    INITSECURITYINTERFACE pfInitSecurityInterface = NULL;

    LOCK_SECURITY();

    if (g_hMyCertStore == NULL) {

        //
        // CRYPT32.DLL is delayloaded. Need SEH in case it fails
        //

        __try {
            g_hMyCertStore = CertOpenSystemStore(0, "MY");
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            Error = GetLastError();
        }
        ENDEXCEPT
    }
    if( g_hMyCertStore != NULL)
        g_bOpenMyCertStore = TRUE; 

    if (Error == ERROR_SUCCESS) {
        Error = LoadWinTrust();
    }
    if ( Error != ERROR_SUCCESS )
    {
        goto quit;
    }

    if( hSecurity != NULL )
    {
        goto quit;
    }

        //
        // load dll.
        //

       //
       // This is better for performance. Rather than call through
       //    SSPI, we go right to the DLL doing the work.
       //

       hSecurity = LoadLibrary( "schannel" );

        if ( hSecurity == NULL ) {
            Error = GetLastError();
            goto quit;
        }

        //
        // get function addresses.
        //

#ifdef UNICODE
        pfInitSecurityInterface =
            (INITSECURITYINTERFACE) GetProcAddress( hSecurity,
                                                     "InitSecurityInterfaceW" );
#else
        pfInitSecurityInterface =
            (INITSECURITYINTERFACE) GetProcAddress( hSecurity,
                                                     "InitSecurityInterfaceA" );
#endif


        if ( pfInitSecurityInterface == NULL )
        {
             Error = GetLastError();
             goto quit;
        }

    //
    // Get SslCrackCertificate func pointer,
    //  utility function declared in SCHANNEL that
    //  is used for parsing X509 certificates.
    //

        pSslCrackCertificate =
            (SSL_CRACK_CERTIFICATE_FN) GetProcAddress( hSecurity,
                                                     SSL_CRACK_CERTIFICATE_NAME );


    if ( pSslCrackCertificate == NULL )
    {
        Error = GetLastError();
        goto quit;
    }



        pSslFreeCertificate =
            (SSL_FREE_CERTIFICATE_FN) GetProcAddress( hSecurity,
                                                     SSL_FREE_CERTIFICATE_NAME );


    if ( pSslFreeCertificate == NULL )
    {
        Error = GetLastError();
        goto quit;
    }

    GlobalSecFuncTable = (SecurityFunctionTable*) ((*pfInitSecurityInterface) ());

    if ( GlobalSecFuncTable == NULL ) {
         Error = GetLastError(); // BUGBUG does this work?
         goto quit;
    }

    HMODULE hCrypt32;
    hCrypt32 = GetModuleHandle("crypt32");

    INET_ASSERT(hCrypt32 != NULL);

    // We don't error out here because not finding these entry points
    // just affects Fortezza. The rest will still work fine.
    if (hCrypt32)
    {
        g_CryptInstallDefaultContext = (CRYPT_INSTALL_DEFAULT_CONTEXT_FN)
                                    GetProcAddress(hCrypt32, CRYPT_INSTALL_DEFAULT_CONTEXT_NAME);

        g_CryptUninstallDefaultContext = (CRYPT_UNINSTALL_DEFAULT_CONTEXT_FN)
                                    GetProcAddress(hCrypt32, CRYPT_UNINSTALL_DEFAULT_CONTEXT_NAME);

        g_CertFindChainInStore = (CERT_FIND_CHAIN_IN_STORE_FN)
                                    GetProcAddress(hCrypt32, CERT_FIND_CHAIN_IN_STORE_NAME);

        g_CertFreeCertificateChain = (CERT_FREE_CERTIFICATE_CHAIN_FN)
                                    GetProcAddress(hCrypt32, CERT_FREE_CERTIFICATE_CHAIN_NAME);
    }

quit:

    if ( Error != ERROR_SUCCESS )
    {
        FreeLibrary( hSecurity );
        hSecurity = NULL;
    }

    UNLOCK_SECURITY();

    return( Error );
}



// Fortezza related functionality.


//Private functions used by the Fortezza implementation.
static PCCERT_CONTEXT GetCurrentFortezzaCertContext();
static BOOL SetCurrentFortezzaCertContext(PCCERT_CONTEXT);
static DWORD AcquireFortezzaCryptProv(HWND, HCRYPTPROV *);
static DWORD ReleaseFortezzaCryptProv(HCRYPTPROV, BOOL);
static DWORD AcquireFortezzaCertContext(HCRYPTPROV, PCCERT_CONTEXT*);


// Should we do anything regarding Fortezza.
BOOL IsFortezzaInstalled ( )
{
    LOCK_FORTEZZA( );

    if (!g_bCheckedForFortezza)
    {
        g_bCheckedForFortezza = TRUE;
        g_bFortezzaInstalled = FALSE;

        // Try and get the Fortezza CSP context to see if it is present.
        HCRYPTPROV hCryptProv = NULL;
        if (GlobalEnableFortezza)
        {
            if (CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_FORTEZZA, CRYPT_SILENT))
            {
                // Weird: we should not be allowed to get the context without putting up UI.
                // But we will assume Fortezza is enabled.
                g_bFortezzaInstalled = TRUE;
                CryptReleaseContext(hCryptProv, 0);
            }
            else
            {
                DWORD dwError = GetLastError();

                // If the last error was NTE_PROV_TYPE_NOT_DEF it means that Fortezza CSP is not
                // installed and we should not be trying to get a Fortezza context.
                g_bFortezzaInstalled = ((dwError != NTE_PROV_TYPE_NOT_DEF) && (dwError != NTE_PROV_TYPE_NO_MATCH));

            }
        }
    }

    UNLOCK_FORTEZZA( );

    return g_bFortezzaInstalled;
}


BOOL AttemptedFortezzaLogin( )
{
    BOOL bRet ;

    LOCK_FORTEZZA();
    bRet = g_bAttemptedFortezzaLogin;
    UNLOCK_FORTEZZA();

    return bRet;
}


// Log's on to the fortezza card. Returns success if you are already logged on.  
DWORD FortezzaLogOn(HWND hwnd)
{
    DWORD dwError;
    LOCK_FORTEZZA();

    // If we are already logged on, don't bother. Just succeed.
    if (GetCurrentFortezzaCertContext() != NULL)
    {
        INET_ASSERT(g_bAttemptedFortezzaLogin);
        INET_ASSERT(GlobalFortezzaCryptProv);
        dwError = ERROR_SUCCESS;
    }
    else
    {
        HCRYPTPROV hProv;
        g_bAttemptedFortezzaLogin = TRUE;

        INET_ASSERT(GlobalFortezzaCryptProv == NULL);

        dwError = AcquireFortezzaCryptProv(hwnd, &GlobalFortezzaCryptProv);

        if (dwError == ERROR_SUCCESS)
        {
            INET_ASSERT(GlobalFortezzaCryptProv != NULL);
            PCCERT_CONTEXT pCertContext = NULL;
            
            dwError = AcquireFortezzaCertContext(GlobalFortezzaCryptProv, &pCertContext);

            if (dwError == ERROR_SUCCESS)
            {
                //Logged in succesfully.
                SetCurrentFortezzaCertContext(pCertContext);
            }
        }
    
        if (dwError != ERROR_SUCCESS && GlobalFortezzaCryptProv != NULL)    
        {
            ReleaseFortezzaCryptProv(GlobalFortezzaCryptProv, FALSE);
            GlobalFortezzaCryptProv = NULL;
        }
        
    }

    if (dwError == ERROR_SUCCESS)
    {
        INET_ASSERT(GetCurrentFortezzaCertContext());
        INET_ASSERT(GlobalFortezzaCryptProv != NULL);
    } 
                   
    UNLOCK_FORTEZZA( );
    
    return dwError;                  

}


DWORD FortezzaLogOff(HWND /* hwnd */)
{
    LOCK_FORTEZZA();

    BOOL bGotCertContext = (GetCurrentFortezzaCertContext() != NULL);
    SetCurrentFortezzaCertContext(NULL);
    ReleaseFortezzaCryptProv(GlobalFortezzaCryptProv, bGotCertContext);
    GlobalFortezzaCryptProv = NULL;

    UNLOCK_FORTEZZA( );

    return ERROR_SUCCESS;
}


DWORD FortezzaChangePersonality(HWND hwnd)
{
    DWORD dwError = ERROR_SUCCESS;

    LOCK_FORTEZZA( );
    
    PCCERT_CONTEXT pOldCertContext = GetCurrentFortezzaCertContext( );
    
    if (pOldCertContext != NULL)
    {
        INET_ASSERT(GlobalFortezzaCryptProv != NULL);
        HCRYPTPROV hNewCryptProv = NULL;
        PCCERT_CONTEXT pNewCertContext = NULL;
        
        // For the change personality to work we need to get a new handle to a 
        // Fortezza crypt provider without freeing the old one. ,
        // If we free the old one first it will re-prompt the user for the password.

        dwError = AcquireFortezzaCryptProv(hwnd, &hNewCryptProv);
        if (dwError == ERROR_SUCCESS)
        {
            dwError = AcquireFortezzaCertContext(hNewCryptProv, &pNewCertContext);

            if (dwError == ERROR_SUCCESS)
            {
                // free up the old CryptProv context
                ReleaseFortezzaCryptProv(GlobalFortezzaCryptProv, TRUE);
                GlobalFortezzaCryptProv = hNewCryptProv;
                // This will automatically free the old cert context.
                SetCurrentFortezzaCertContext(pNewCertContext);
            }
            else 
            {
                ReleaseFortezzaCryptProv(hNewCryptProv, FALSE);
            }
        }
    }
    else
    {
        // We are trying to change personalities when not logged on.
        // This is not allowed.                         

        dwError = ERROR_INVALID_PARAMETER;
    }

    UNLOCK_FORTEZZA( );
    
    return dwError;
}


// Entry points exported outside wininet.
INTERNETAPI
BOOL
WINAPI
InternetQueryFortezzaStatus(DWORD * pdwStatus, DWORD_PTR dwReserved)
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetQueryFortezzaStatus",
                     "%#x %#x",
                     pdwStatus, dwReserved
                     ));

    BOOL bRet;
    DWORD dwError = ERROR_SUCCESS;

    // Initialize the GlobalData since this is an exported entry point.
    if (dwReserved!=0)
    {
        dwError = ERROR_INVALID_PARAMETER;
    }
    else if (!GlobalDataInitialized)
    {
        dwError = GlobalDataInitialize( );
    }
    
    if (dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
    }
    else if (pdwStatus == NULL)
    {
        bRet = FALSE;
        dwError = ERROR_INVALID_PARAMETER;
    }
    else
    {
        if (IsFortezzaInstalled( ))
        {
            *pdwStatus |= (FORTSTAT_INSTALLED);
        }
        
        if (GetCurrentFortezzaCertContext() != NULL)
        {
            *pdwStatus |= (FORTSTAT_LOGGEDON);
        }
        bRet = TRUE;
    }

    if (!bRet)
    {
        SetLastError(dwError);
        DEBUG_ERROR(INET, dwError);
    }
    DEBUG_LEAVE_API(bRet);
    return bRet;
}
                        

INTERNETAPI
BOOL
WINAPI
InternetFortezzaCommand(DWORD dwCommand, HWND hwnd, DWORD_PTR dwReserved)
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetFortezzaCommand",
                     "%d, %#x, %#x",
                     dwCommand, hwnd, dwReserved
                     ));

    BOOL bRet = TRUE;
    DWORD dwError = ERROR_SUCCESS;

    // Initialize the GlobalData since this is an exported entry point.
    if (dwReserved!=0)
    {
        dwError = ERROR_INVALID_PARAMETER;
    }
    else if (!GlobalDataInitialized)
    {
        dwError = GlobalDataInitialize( );
    }
    
    // Next make sure that the security dlls are loaded.
    if (dwError == ERROR_SUCCESS)
        dwError = LoadSecurity( );

    // If all is fine, then try the actual command.
    if (dwError == ERROR_SUCCESS)
    {
        // Dispatch based on the command.
        switch (dwCommand) 
        {
            case FORTCMD_LOGON:
                dwError = FortezzaLogOn(hwnd);
                break;
            case FORTCMD_LOGOFF:
                dwError = FortezzaLogOff(hwnd);
                break;
            case FORTCMD_CHG_PERSONALITY:
                dwError = FortezzaChangePersonality(hwnd);
                break;
            default:
                dwError = ERROR_INVALID_PARAMETER;
        }
    }            

    if (dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
        DEBUG_ERROR(INET, dwError);
        SetLastError(dwError);
    }
    else
    {
        LOCK_SECURITY( );

        // If we were successful to this point we should re-init the security packages so 
        // we acquire a credentials handle with the new cert context selected correctly.

        // The Last error will be set by SecurityPkgInitialize if it fails.
        bRet = SecurityPkgInitialize(TRUE);

        UNLOCK_SECURITY( );
    }

    DEBUG_LEAVE_API(bRet);
    return bRet;
}



/*++
    Gets the cert context being used for Fortezza connections.
    
Routine description
    
Returns:

    pCertContext if one is in use. NULL otherwise.
*/            
    
PCCERT_CONTEXT GetCurrentFortezzaCertContext()
{
    PCCERT_CONTEXT pCertContext = NULL;
    DWORD dwIndex;

    LOCK_FORTEZZA( );

    // Find the unified service provider entry.
    for ( dwIndex = 0 ; SecProviders[dwIndex].pszName != NULL ; dwIndex++ )
    {
        if (0 == stricmp(UNISP_NAME, SecProviders[dwIndex].pszName))
        {
            pCertContext = SecProviders[dwIndex].pCertCtxt ;

            break;
        }
    }
    // Something is wrong if we didn't find the Unified Service provider in our list.

    INET_ASSERT(SecProviders[dwIndex].pszName != NULL);

    UNLOCK_FORTEZZA( );
    
    return pCertContext;
}

               
/*++
    Sets the passed in cert context to be the one that is used for Fortezza 
    connections.
    
Routine description
    This function simply takes a Fortezza context and remembers it on the unified 
    secure providers table. 
    
Arguments:

    pCertContext - The cert context to be saved away.
*/            

BOOL SetCurrentFortezzaCertContext(PCCERT_CONTEXT pCertContext)
{
    DWORD dwIndex;

    LOCK_FORTEZZA( );

    // Find the unified service provider entry.
    for ( dwIndex = 0 ; SecProviders[dwIndex].pszName != NULL ; dwIndex++ )
    {
        if (0 == stricmp(UNISP_NAME, SecProviders[dwIndex].pszName))
        {
            if (SecProviders[dwIndex].pCertCtxt)
                CertFreeCertificateContext(SecProviders[dwIndex].pCertCtxt);

            SecProviders[dwIndex].pCertCtxt = pCertContext;

            break;
        }
    }
    
    // Something is wrong if we didn't find the Unified Service provider in our list.

    INET_ASSERT(SecProviders[dwIndex].pszName != NULL);

    UNLOCK_FORTEZZA( );

    return TRUE;
}



/*++
    Acquire a fortezza crypt provider.

Routine Description
    This function calls the Fortezza CSP which might prompt the end-user for 
    the PIN # to read the certificates of the Fortezza card. If the user is 
    already logged on to the card the logon UI will not be shown. 

Arguments:

    hwnd - used to put up the pin UI.

    pCryptProv - returns a handle to a crypt provider if succesful.

Return Value:
    
    WINDOWS Error Code.

--*/
DWORD AcquireFortezzaCryptProv(HWND hwnd, HCRYPTPROV *pCryptProv)
{
    DWORD dwError = NOERROR;
    BOOL bResethwnd = FALSE;

    if (pCryptProv == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Set up to do the UI.

    if ( CryptSetProvParam(NULL, PP_CLIENT_HWND, (BYTE *)&hwnd, 0))
    {
        bResethwnd = TRUE;
    }

    //
    // Attempt to log on to Fortezza card. This call will typically
    // display a dialog box.
    //
    // Note that within the CryptAcquireContext function, the Fortezza
    // CSP populated the MY store with the Fortezza certificate chain.
    // At least, it will once it's finished.
    //

    if(!CryptAcquireContext(pCryptProv, NULL, NULL, PROV_FORTEZZA, 0))
    {
        dwError = GetLastError();
    }

    if (bResethwnd)
    {
        CryptSetProvParam(NULL, PP_CLIENT_HWND, NULL, 0);
    }

    return(dwError);
}        


/*++
    Releases the Fortezza Crypt Provider

Routine Description:
    Frees the crypt provider if the second argument is FALSE. 
    Does nothing if the second argument is TRUE.
    The crypto API has this strange behavior ( i am being gracious 
    in my description here) where when a hCryptProv is passed into 
    CertSetCertificateContextProperty it holds on to the pointer but does not increment the 
    refcount. When the cert context is freed it does free the hCryptProv. 
    To workaround this behavior we never free the hCryptProv, just the Fortezza cert context.

Arguments:
    hCryptProv - The CryptProv to free.
    bGotCertContext - did we get a Fortezza Cert Context using this provider. 
**/

DWORD ReleaseFortezzaCryptProv(HCRYPTPROV hCryptProv, BOOL bGotCertContext )
{
    DWORD dwError;

    if (hCryptProv==NULL)
    {
        dwError = NOERROR;
    }
    else
    {
        if (!CryptReleaseContext(hCryptProv, 0))
            dwError = GetLastError();
        else
            dwError = NOERROR;
    }

    return dwError;
}
        


DWORD AcquireFortezzaCertContext(HCRYPTPROV hFortezzaCryptProv, PCCERT_CONTEXT *ppCertContext)                                               
/*++

Routine Description:

    This function calls the Fortezza CSP which will prompt the
    user for the PIN # to read the certificates off the Fortezza
    card.

Arguments:

    [IN]    hCryptProv - Handle to the Fortezza Crypt Provider.
    [OUT]   ppCertContext - will have the cert context if returns succesfully.
Return Value:

    WINDOWS Error Code.

--*/
{
    DWORD error = ERROR_SUCCESS;   // Return code.
    DWORD status = ERROR_SUCCESS;  // Error value if one of the crypto APIs failed.
    CRYPT_HASH_BLOB HashBlob;
    BYTE            rgbHash[20];
    DWORD           cbHash;
    PBYTE           pbChain = NULL;
    DWORD           cbChain;
    PBYTE           pbCert;
    DWORD           cbCert;
    PCCERT_CONTEXT  pCertContext = NULL;
    BOOL            bResethwnd = FALSE;
    DWORD           dwIndex;

    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "AcquireFortezzaContext",
                 "%#x",
                 hFortezzaCryptProv
                 ));

    if (hFortezzaCryptProv == NULL || ppCertContext == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    LOCK_FORTEZZA();

    if (!IsFortezzaInstalled( ))
    {
        INET_ASSERT(FALSE); // Should not get called if Fortezza is not installed.
        goto done;       // Just ignore the request.
    }


    if(g_bOpenMyCertStore && g_hMyCertStore == NULL)
        ReopenMyCertStore();

    if(g_hMyCertStore == NULL)
    {
        status = SEC_E_NO_CREDENTIALS;
        goto done;
    }

    //
    // Read the appropriate leaf certificate from the card, and
    // obtain its MD5 thumbprint.
    //

    // Get length of certificate chain.
    if(!CryptGetProvParam(hFortezzaCryptProv, PP_CERTCHAIN, NULL, &cbChain, 0))
    {
        status = GetLastError();
        DEBUG_PRINT(API,
                    ERROR,
                    ("**** Error 0x%x reading certificate from CSP\n",
                    status
                    ));
        goto done;
    }

    // Allocate memory for certificate chain.
    pbChain = (BYTE *)ALLOCATE_MEMORY(LMEM_FIXED | LMEM_ZEROINIT, cbChain);
    if(pbChain == NULL)
    {
        status = ERROR_NOT_ENOUGH_MEMORY;
        DEBUG_PRINT(API,
                    ERROR,
                    ("**** Out of memory\n"));
        goto done;
    }

    // Download certificate chain from CSP.
    if(!CryptGetProvParam(hFortezzaCryptProv, PP_CERTCHAIN, pbChain, &cbChain, 0))
    {
        status = GetLastError();
        DEBUG_PRINT(API,
                    ERROR,
                    ("**** Error 0x%x reading certificate from CSP\n",
                    status
                    ));
        goto done;
    }

    // Parse out the leaf certificate.
    cbCert = *(PDWORD)pbChain;
    pbCert = pbChain + sizeof(DWORD);

    // Decode the leaf certificate.
    pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,
                                                pbCert,
                                                cbCert);
    if(pCertContext == NULL)
    {
        status = GetLastError();
        DEBUG_PRINT(API,
                    ERROR,
                    ("**** Error 0x%x parsing certificate\n",
                    status
                    ));
        goto done;
    }

    // Get thumbprint of certificate.
    cbHash = sizeof(rgbHash);
    if(!CertGetCertificateContextProperty(pCertContext,
                                          CERT_MD5_HASH_PROP_ID,
                                          rgbHash,
                                          &cbHash))
    {
        status = GetLastError();
        DEBUG_PRINT(API,
                    ERROR,
                    ("**** Error 0x%x reading MD5 property\n",
                    status
                    ));
        goto done;
    }

    // Free certificate chain.
    FREE_MEMORY(pbChain);
    pbChain = NULL;

    // Free certificate context.
    CertFreeCertificateContext(pCertContext);
    pCertContext = NULL;


    //
    // Search the "MY" certificate store for the certificate with
    // the matching thumbprint.
    //

    HashBlob.cbData = cbHash;
    HashBlob.pbData = rgbHash;
    if(g_bOpenMyCertStore && g_hMyCertStore == NULL)
        ReopenMyCertStore();
    pCertContext = CertFindCertificateInStore(g_hMyCertStore,
                                              X509_ASN_ENCODING,
                                              0,
                                              CERT_FIND_MD5_HASH,
                                              &HashBlob,
                                              NULL);
    if(pCertContext == NULL)
    {
        DEBUG_PRINT(API,
                    ERROR,
                    ("**** Leaf certificate not found in MY store\n"));

        status = SEC_E_NO_CREDENTIALS;
        goto done;
    }


    //
    // Attach the Fortezza hProv to the certificate context.
    //

    if(!CertSetCertificateContextProperty(
            pCertContext,
            CERT_KEY_PROV_HANDLE_PROP_ID,
            0,
            (PVOID)hFortezzaCryptProv))
    {
        status = GetLastError();
        DEBUG_PRINT(API,
                    ERROR,
                    ("**** Error 0x%x setting KEY_PROV_HANDLE property\n",
                    status
                    ));
        goto done;
    }


    INET_ASSERT(pCertContext != NULL);

    *ppCertContext = pCertContext;
    pCertContext = NULL;

    status = SEC_E_OK;

done:

    if(pbChain) FREE_MEMORY(pbChain);
    if(pCertContext) CertFreeCertificateContext(pCertContext);

    UNLOCK_FORTEZZA();
    DEBUG_LEAVE(error);
    return error;
}
