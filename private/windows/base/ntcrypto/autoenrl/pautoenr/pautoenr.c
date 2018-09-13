#include <windows.h>
#include <wincrypt.h>
#include <autoenr.h>
#include <cryptui.h>

#define MY_TEST_REG_ENTRY   "Software\\Microsoft\\Cryptography\\AutoEnroll"
#define PST_EVENT_INIT "PS_SERVICE_STARTED"


BOOL SmallTest(DWORD dw)
{
    HKEY    hRegKey = 0;
    DWORD   dwDisposition;
    BOOL    fRet = FALSE;

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, MY_TEST_REG_ENTRY,
                                        0, NULL, REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS, NULL, &hRegKey,
                                        &dwDisposition))
        goto Ret;

    if (ERROR_SUCCESS != RegSetValueEx(hRegKey, "AutoEnrollTest", 0,
                                       REG_BINARY, (BYTE*)&dw, sizeof(dw)))
        goto Ret;

    fRet = TRUE;
Ret:
    if (hRegKey)
        RegCloseKey(hRegKey);
    SetLastError(dw);
    return fRet;
}

void AutoEnrollErrorLogging(DWORD dwErr)
{
    // UNDONE - log the error along with some useful message
    SmallTest(dwErr);
}

#define FAST_BUFF_LEN   256



BOOL EnrollForACert(
                    IN BOOL fMachineEnrollment,
                    IN BOOL fRenewalRequired,
                    IN PAUTO_ENROLL_INFO pInfo
                    )
{
    CRYPTUI_WIZ_CERT_REQUEST_INFO       CertRequestInfo;
    CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW    NewKeyInfo;
    CRYPTUI_WIZ_CERT_TYPE               CertType;
    CRYPT_KEY_PROV_INFO                 ProviderInfo;
    PCCERT_CONTEXT                      pCertContext = NULL;
    PCCERT_CONTEXT                      pCert = NULL;
    DWORD                               dwCAStatus;
    DWORD                               dwAcquireFlags = 0;
    LPWSTR                              pwszProvName = NULL;
	WCHAR								rgwszMachineName[MAX_COMPUTERNAME_LENGTH + 1]; 
    DWORD                               cMachineName = MAX_COMPUTERNAME_LENGTH + 1;
    CRYPT_DATA_BLOB                     CryptData;
    DWORD                               dwErr = 0;
    BOOL                                fRet = FALSE;

    memset(&CertRequestInfo, 0, sizeof(CertRequestInfo));
    memset(&NewKeyInfo, 0, sizeof(NewKeyInfo));
    memset(&ProviderInfo, 0, sizeof(ProviderInfo));
    memset(&rgwszMachineName, 0, sizeof(rgwszMachineName));
    memset(&CryptData, 0, sizeof(CryptData));
    memset(&CertType, 0, sizeof(CertType));

    if (fMachineEnrollment)
    {
        dwAcquireFlags = CRYPT_MACHINE_KEYSET;
	    if (0 == GetComputerNameW(rgwszMachineName,
                                  &cMachineName))
        {
            goto Ret;
        }
        CertRequestInfo.pwszMachineName = rgwszMachineName;
    }
    
    // set up the provider info
    ProviderInfo.dwProvType = pInfo->dwProvType;

    ProviderInfo.pwszProvName = NULL;  // The wizard will choose one based
                                       // on the cert type

    // set the acquire context flags
    // UNDONE - need to add silent flag
    ProviderInfo.dwFlags = dwAcquireFlags;

    // set the key specification
    ProviderInfo.dwKeySpec = pInfo->dwKeySpec;

    // set up the new key info
    NewKeyInfo.dwSize = sizeof(NewKeyInfo);
    NewKeyInfo.pKeyProvInfo = &ProviderInfo;
    // set the flags to be passed when calling CryptGenKey
    NewKeyInfo.dwGenKeyFlags = pInfo->dwGenKeyFlags;

    // set the request info
    CertRequestInfo.dwSize = sizeof(CertRequestInfo);

    // UNDONE - if cert exists then check if expired (if so do renewal)
    if (pInfo->fRenewal)
    {
        CertRequestInfo.dwPurpose = CRYPTUI_WIZ_CERT_RENEW;
        CertRequestInfo.pRenewCertContext = pInfo->pOldCert;
    }
    else
    {
        CertRequestInfo.dwPurpose = CRYPTUI_WIZ_CERT_ENROLL;
        CertRequestInfo.pRenewCertContext = NULL;
    }

    // UNDONE - for now always gen a new key, later may allow using existing key
    // for things like renewal
    CertRequestInfo.dwPvkChoice = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW;
    CertRequestInfo.pPvkNew = &NewKeyInfo;

    // destination cert store is the MY store (!!!! hard coded !!!!)
    CertRequestInfo.pwszDesStore = L"MY";

    // set algorithm for hashing
    CertRequestInfo.pszHashAlg = NULL;

    // set the cert type
    CertRequestInfo.dwCertChoice = CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE;
    CertType.dwSize = sizeof(CertType);
    CertType.cCertType = 1;
    CertType.rgwszCertType = &pInfo->pwszCertType;
    CertRequestInfo.pCertType = &CertType;

    // set the requested cert extensions
    CertRequestInfo.pCertRequestExtensions = &pInfo->CertExtensions;

    // set post option  
    CertRequestInfo.dwPostOption = 0;

    // set the Cert Server machine and authority
    CertRequestInfo.pwszCALocation = pInfo->pwszCAMachine;
    CertRequestInfo.pwszCAName = pInfo->pwszCAAuthority;

    // certify and create a key at the same time
    if (!CryptUIWizCertRequest(CRYPTUI_WIZ_NO_UI, 0, NULL,
                               &CertRequestInfo, &pCertContext,     
                               &dwCAStatus))    
    {
        AutoEnrollErrorLogging(GetLastError());
        goto Ret;
    }

    if (CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED == dwCAStatus)
    {
        BYTE aHash[20];
        CRYPT_HASH_BLOB blobHash;

        blobHash.pbData = aHash;
        blobHash.cbData = sizeof(aHash);
        CryptData.cbData = (wcslen(pInfo->pwszAutoEnrollmentID) + 1) * sizeof(WCHAR);
        CryptData.pbData = (BYTE*)pInfo->pwszAutoEnrollmentID;
        
        // We need to get the real certificate of the store, as the one
        // passed back is self contained.
        if(!CertGetCertificateContextProperty(pCertContext,
                                          CERT_SHA1_HASH_PROP_ID,
                                          blobHash.pbData,
                                          &blobHash.cbData))
        {
            AutoEnrollErrorLogging(GetLastError());
            goto Ret;
        }

        pCert =  CertFindCertificateInStore(pInfo->hMYStore,
                                            pCertContext->dwCertEncodingType,
                                            0,
                                            CERT_FIND_SHA1_HASH,
                                            &blobHash,
                                            NULL);
        if(pCert == NULL)
        {
            AutoEnrollErrorLogging(GetLastError());
            goto Ret;
        }

        // place the auto enrollment property on the cert
        if (!CertSetCertificateContextProperty(pCert,
                        CERT_AUTO_ENROLL_PROP_ID, 0, &CryptData))
        {
            AutoEnrollErrorLogging(GetLastError());
            goto Ret;
        }
    }

    // UNDONE - request did not return cert so take appropriate action
//    else
//    {
//        goto Ret;
//    }

    fRet = TRUE;
Ret:
    if (pCertContext)
        CertFreeCertificateContext(pCertContext);

    if (pCert)
        CertFreeCertificateContext(pCert);

    if (pwszProvName)
        LocalFree(pwszProvName);

    return fRet;
}



BOOL
AutoEnrollWait(
    VOID
    )
/*++

    This routine determines if the protected storage service is
    pending start.  If the service is pending start, this routine
    waits until the service is running before returning to the
    caller.

    If the Service is running when this routine returns, the
    return value is TRUE.

    If the service is not running, or an error occurred, the
    return value is FALSE.

    When the return value is FALSE, the value is only advisory, and may not
    indicate the current state of the service.  The reasoning here is that
    if the service did not start the first time this call is made, is will
    not likely be running the next time around, and hence we avoid checking
    on subsequent calls.

    For current situations, the caller should ignore the return value; when
    the return value is FALSE, the caller should just try making the call
    into the service.  If the service is still down, the call into it will fail
    appropriately.

  Copied from \nt\private\ispu\common\unicode\nt.cpp - 9/14/99 - petesk

--*/
{
    SC_HANDLE schSCM;
    SC_HANDLE schService = NULL;
    DWORD dwStopCount = 0;
    static BOOL fDone = FALSE;
    static BOOL fSuccess = FALSE;

    BOOL fCheckDisabled = TRUE;

    HANDLE hToken = NULL;
    BOOL fSystemAccount = FALSE;

    if( fDone )
        return fSuccess;

    schSCM = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT );
    if(schSCM == NULL)
        return FALSE;

    //
    // open the protected storage service so we can query it's
    // current state.
    //

    schService = OpenServiceW(schSCM, L"ProtectedStorage", SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
    if( schService == NULL ) {
        fCheckDisabled = FALSE;
        schService = OpenServiceW(schSCM, L"ProtectedStorage", SERVICE_QUERY_STATUS);
    }

    if(schService == NULL)
        goto cleanup;

    //
    // check if the service is disabled.  If it is, bailout.
    //

    if( fCheckDisabled ) {
        LPQUERY_SERVICE_CONFIG pServiceConfig;
        BYTE TempBuffer[ 1024 ];
        DWORD cbServiceConfig;

        pServiceConfig = (LPQUERY_SERVICE_CONFIG)TempBuffer;
        cbServiceConfig = sizeof(TempBuffer);

        if(QueryServiceConfig( schService, pServiceConfig, cbServiceConfig, &cbServiceConfig )) {

            if( pServiceConfig->dwStartType == SERVICE_DISABLED ) {
                goto cleanup;
            }
        }
    }

    //
    // check if calling process is SYSTEM account.
    // if it is, use a larger timeout value.
    //

    if( OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) ) {

        do {

            BYTE FastBuffer[ 256 ];
            PTOKEN_USER TokenInformation;
            DWORD cbTokenInformation;
            SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
            PSID psidLocalSystem;

            TokenInformation = (PTOKEN_USER)FastBuffer;
            cbTokenInformation = sizeof(FastBuffer);

            if(!GetTokenInformation(
                                hToken,
                                TokenUser,
                                TokenInformation,
                                cbTokenInformation,
                                &cbTokenInformation
                                )) break;

            if(!AllocateAndInitializeSid(
                                &sia,
                                1,
                                SECURITY_LOCAL_SYSTEM_RID,
                                0, 0, 0, 0, 0, 0, 0,
                                &psidLocalSystem
                                )) break;

            fSystemAccount = EqualSid(
                                psidLocalSystem,
                                TokenInformation->User.Sid
                                );

            FreeSid( psidLocalSystem );

        } while (FALSE);

        CloseHandle( hToken );
    }



//
// number of seconds to Sleep per loop interation.
//

#define SLEEP_SECONDS (5)


    if( fSystemAccount ) {

        //
        // 15 minutes for SYSTEM account.
        //

        dwStopCount = 900 / SLEEP_SECONDS;

    } else {

        //
        //
        // loop checking the service status every 5 seconds, for up to 2 minutes
        // total (120 seconds, 5*24=120)
        //

        dwStopCount = 120 / SLEEP_SECONDS;
    }

    for( ; dwStopCount != 0 ; dwStopCount--, Sleep(SLEEP_SECONDS*1000) ) {
        SERVICE_STATUS sServiceStatus;

        //
        // find out current service status
        //

        if(!QueryServiceStatus( schService, &sServiceStatus ))
            break;

        //
        // if start pending, wait and re-query.
        //

        if( sServiceStatus.dwCurrentState == SERVICE_START_PENDING )
            continue;

        //
        // When SCM starts up, all services are marked SERVICE_STOPPED
        // unfortunately; rather than SERVICE_START_PENDING
        // Continue wait loop in this scenario.
        //

        if( sServiceStatus.dwCurrentState == SERVICE_STOPPED &&
            sServiceStatus.dwWin32ExitCode == ERROR_SERVICE_NEVER_STARTED ) {
            continue;
        }


        //
        // if service is running, indicate success
        //

        if( sServiceStatus.dwCurrentState == SERVICE_RUNNING ) {
            fSuccess = TRUE;
            break;
        }

        //
        // bail out on any other dwCurrentState
        // eg: service stopped, error condition, etc.
        //

        break;
    }

    fDone = TRUE;

cleanup:

    if(schService)
        CloseServiceHandle(schService);

    CloseServiceHandle(schSCM);

    return fSuccess;
}


//+---------------------------------------------------------------------------
//
//  Function:   ProvAutoEnrollment
//
//  Synopsis:   Entry point for the default MS auto enrollment client provider.
//
//  Arguments:  
//          fMachineEnrollment - TRUE if machine is enrolling, FALSE if user
//
//          pInfo - information needed to enroll (see AUTO_ENROLL_INFO struct
//                  in autoenrl.h)
//
//  History:    01-12-98   jeffspel   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL ProvAutoEnrollment(
                        IN BOOL fMachineEnrollment,
                        IN PAUTO_ENROLL_INFO pInfo
                        )
{
    BOOL                fRenewalRequired = FALSE;
    BOOL                fRet = FALSE;

    __try
    {
        // Wait for necessary resources
        if (!AutoEnrollWait())
            goto Ret;

        // enroll for a cert
        if (!EnrollForACert(fMachineEnrollment, fRenewalRequired, pInfo))
            goto Ret;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        goto Ret;
    }
    fRet = TRUE;
Ret:
    return fRet;
}

BOOLEAN
DllInitialize(
    IN PVOID hmod,
    IN ULONG Reason,
    IN PCONTEXT Context
    )
{

    return( TRUE );

}