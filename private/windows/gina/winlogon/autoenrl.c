/****************************** Module Header ******************************\
* Module Name: autoenrl.c
*
* Copyright (c) 1997, Microsoft Corporation
*
* Module for Public Key certificate auto enrollment
*
* History:
* 11-21-97 jeffspel       Created.
* 01-30-98 jeffspel       changed to include machine auto enrollment
\***************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "tchar.h"



#define AE_DEFAULT_REFRESH_RATE 8 // 8 hour default autoenrollment rate

#define SYSTEM_POLICIES_KEY          TEXT("Software\\Policies\\Microsoft\\Windows\\System")

#define MAX_TEMPLATE_NAME_VALUE_SIZE             64 // sizeof (CERT_NAME_VALUE) + wcslen(wszCERTTYPE_DC)
#define MAX_DN_SIZE 256

#define MACHINE_AUTOENROLL_INITIAL_DELAY         10 // seconds
#define USER_AUTOENROLL_INITIAL_DELAY         120 // seconds

#if DBG

DWORD g_AutoenrollDebugLevel = AE_ERROR ; //| AE_WARNING | AE_INFO | AE_TRACE;

#endif



PISECURITY_DESCRIPTOR AEMakeGenericSecurityDesc();

//
// Structure to hold information passed to Auto Enrollment threads
//

HANDLE g_hUserMutex = 0;
HANDLE g_hMachineMutex = 0;

#define DS_ATTR_COMMON_NAME     TEXT("cn")
#define DS_ATTR_DNS_NAME        TEXT("dNSHostName")
#define DS_ATTR_EMAIL_ADDR      TEXT("mail")
#define DS_ATTR_OBJECT_GUID     TEXT("objectGUID")
#define DS_ATTR_UPN             TEXT("userPrincipalName")

static HINSTANCE            g_hInstSecur32 = NULL;
static HINSTANCE            g_hInstWldap32 = NULL;
static PFNLDAP_INIT         g_pfnldap_init = NULL;
static PFNLDAP_BIND_S       g_pfnldap_bind_s = NULL;
static PFNLDAP_SET_OPTION   g_pfnldap_set_option = NULL;
static PFNLDAP_SEARCH_EXT_S g_pfnldap_search_ext_s = NULL;
static PFNLDAP_FIRST_ENTRY  g_pfnldap_first_entry = NULL;
static PFNLDAP_EXPLODE_DN   g_pfnldap_explode_dn = NULL;
static PFNLDAP_GET_VALUES   g_pfnldap_get_values = NULL;
static PFNLDAP_VALUE_FREE   g_pfnldap_value_free = NULL;
static PFNLDAP_MSGFREE      g_pfnldap_msgfree = NULL;
static PFNLDAP_UNBIND       g_pfnldap_unbind = NULL;
static PFNLDAPGETLASTERROR  g_pfnLdapGetLastError = NULL;
static PFNLDAPMAPERRORTOWIN32 g_pfnLdapMapErrorToWin32 = NULL;
static PFNGETUSERNAMEEX     g_pfnGetUserNameEx = NULL;


#ifndef CERT_ENTERPRISE_SYSTEM_STORE_REGPATH
#define CERT_ENTERPRISE_SYSTEM_STORE_REGPATH L"Software\\Microsoft\\EnterpriseCertificates"
#endif


typedef struct _AUTO_ENROLL_THREAD_INFO_
{
    BOOL                fMachineEnrollment;
    HANDLE              hNotifyEvent;
    HANDLE              hTimer;
    HANDLE              hToken;
    HANDLE              hNotifyWait;
    HANDLE              hTimerWait;
} AUTO_ENROLL_THREAD_INFO, *PAUTO_ENROLL_THREAD_INFO;

//
// Structure to hold internal information needed perform Auto Enrollment
//

typedef struct _INTERNAL_CA_LIST_
{
    HCAINFO hCAInfo;
    LPWSTR  wszName;
    BYTE    CACertHash[20];
    LPWSTR  wszDNSName;
    LPWSTR  *awszCertificateTemplates;
} INTERNAL_CA_LIST, *PINTERNAL_CA_LIST;

typedef struct _INTERNAL_INFO_
{
    BOOL                fMachineEnrollment;
    HANDLE              hToken;
    HCERTSTORE          hRootStore;
    HCERTSTORE          hCAStore;
    HCERTSTORE          hMYStore;   
    LPTSTR             * awszldap_UPN;
    LPTSTR             wszConstructedUPN;
    LPTSTR             * awszEmail;
    LPTSTR              wszDN;
    CERT_NAME_BLOB      blobDN;

    // List of all ca's
    DWORD               ccaList;
    PINTERNAL_CA_LIST   acaList;

} INTERNAL_INFO, *PINTERNAL_INFO;


typedef struct _AE_INSTANCE_INFO_
{
    PCCTL_CONTEXT       pCTLContext;
    PINTERNAL_INFO      pInternalInfo;
    PCCERT_CONTEXT      pOldCert;
    BOOL                fRenewalOK;
    DWORD               dwRandomIndex;
    LPWSTR              pwszCertType;
    LPWSTR              pwszAEIdentifier;
    CERT_EXTENSIONS     *pCertTypeExtensions;
    DWORD               dwCertTypeFlags;
    LARGE_INTEGER        ftExpirationOffset;

} AE_INSTANCE_INFO, *PAE_INSTANCE_INFO;



// Key usage masks
typedef struct _KUMASK {
    DWORD dwMask;
    LPSTR pszAlg;
} KUMASK;


KUMASK g_aKUMasks[] =
{
    {~CERT_KEY_AGREEMENT_KEY_USAGE, szOID_RSA_RSA },
    {~CERT_KEY_ENCIPHERMENT_KEY_USAGE, szOID_OIWSEC_dsa },
    {~CERT_KEY_ENCIPHERMENT_KEY_USAGE, szOID_X957_DSA },
    {~CERT_KEY_ENCIPHERMENT_KEY_USAGE, szOID_ANSI_X942_DH },
    {~CERT_KEY_ENCIPHERMENT_KEY_USAGE, szOID_RSA_DH },
    {~CERT_KEY_AGREEMENT_KEY_USAGE, szOID_OIWSEC_rsaXchg },
    {~CERT_KEY_ENCIPHERMENT_KEY_USAGE, szOID_INFOSEC_mosaicKMandUpdSig }
};

DWORD g_cKUMasks = sizeof(g_aKUMasks)/sizeof(g_aKUMasks[0]);


#define DEFAULT_AUTO_ENROLL_PROV    "pautoenr.dll"

#define AUTOENROLL_EVENT_LOG_SUBKEY L"System\\CurrentControlSet\\Services\\EventLog\\System\\AutoEnroll"
#define SZ_AUTO_ENROLL              L"AutoEnroll"

HRESULT 
myGetConfigDN(
    IN  LDAP *pld,
    OUT LPWSTR *pwszConfigDn
    );

LPWSTR 
HelperExtensionToString(PCERT_EXTENSION Extension);

HRESULT
aeRobustLdapBind(
    OUT LDAP ** ppldap,
    IN BOOL fGC);

// 
// Time skew margin for fast CA's
//
#define FILETIME_TICKS_PER_SECOND  10000000

#define DEFAULT_AUTOENROLL_SKEW  60*60*1  // 1 hour

// 
// ERROR values to be logged as events when auto enrollment fails
//


#ifndef szOID_ALT_NAME_OBJECT_GUID
#define szOID_ALT_NAME_OBJECT_GUID "1.3.6.1.4.1.311.25.1"
#endif  

//
// memory allocation and free routines
void *AEAlloc(
              IN DWORD cb
              )
{
    return LocalAlloc(LMEM_ZEROINIT, cb);
}

void AEFree(
            void *p
            )
{
    LocalFree(p);
}

HRESULT GetExceptionError(EXCEPTION_POINTERS const *pep)
{
    if((pep == NULL) || (pep->ExceptionRecord == NULL))
    {
        return E_UNEXPECTED;
    }

    return pep->ExceptionRecord->ExceptionCode;
}

//
// Name:    AELogTestResult
//
// Description: This function logs the result of a certificate 
// test into the AE_CERT_TEST_ARRAY
//
void AELogTestResult(PAE_CERT_TEST_ARRAY    *ppAEData,
                     DWORD                  idTest,
                     ...)
{
    va_list ArgList;
    va_start(ArgList, idTest);

    if((*ppAEData == NULL) ||
       ((*ppAEData)->cTests ==  (*ppAEData)->cMaxTests))
    {
        PAE_CERT_TEST_ARRAY pAENew = NULL;
        DWORD               cAENew = ((*ppAEData)?(*ppAEData)->cMaxTests:0) + 
                                     AE_CERT_TEST_SIZE_INCREMENT;
        // We need to grow the array

        pAENew = LocalAlloc(LMEM_FIXED, sizeof(AE_CERT_TEST_ARRAY) + 
                                        (cAENew - ANYSIZE_ARRAY)*sizeof(AE_CERT_TEST));
        if(pAENew == NULL)
        {
            return;
        }
        pAENew->dwVersion = AE_CERT_TEST_ARRAY_VERSION;
        pAENew->fRenewalOK = ((*ppAEData)?(*ppAEData)->fRenewalOK:FALSE);
        pAENew->cTests = ((*ppAEData)?(*ppAEData)->cTests:0);
        pAENew->cMaxTests = cAENew;
        if((*ppAEData) && (pAENew->cTests != 0))
        {
            CopyMemory(pAENew->Test, (*ppAEData)->Test, sizeof(AE_CERT_TEST)*pAENew->cTests);
        }

        if(*ppAEData)
        {
            AEFree(*ppAEData);
        }

        (*ppAEData) = pAENew;
    }

    (*ppAEData)->Test[(*ppAEData)->cTests].idTest = idTest;

    if(FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
        g_hInstance,
        idTest,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (PVOID)&(*ppAEData)->Test[(*ppAEData)->cTests].pwszReason,
        0,
        &ArgList))
    {

        (*ppAEData)->cTests++;
    }

}

void AEFreeTestResult(PAE_CERT_TEST_ARRAY    *ppAEData)
{
    DWORD iTest = 0;
    if((ppAEData == NULL) || (*ppAEData == NULL))
    {
        return;
    }

    for(iTest = 0; iTest < (*ppAEData)->cTests; iTest++)
    {
        if((*ppAEData)->Test[iTest].pwszReason)
        {
            LocalFree((*ppAEData)->Test[iTest].pwszReason);
        }
    }

    AEFree(*ppAEData);
    *ppAEData = NULL;
    
}

//
// Name:    LogAutoEnrollmentEvent
//
// Description: This function registers an event in the event log of the
//              local machine.
//
void LogAutoEnrollmentEvent(
                            IN DWORD  dwEventId,
                            IN HANDLE hToken,
                            ...
                            )
{
    HANDLE      hEventSource = 0;
    BYTE        FastBuffer[256];
    PTOKEN_USER ptgUser;
    DWORD       cbUser;
    BOOL        fAlloced = FALSE;
    PSID        pSID = NULL;

    WORD dwEventType = 0;

    LPWSTR      awszStrings[20];
    WORD        cStrings = 0;
    LPWSTR      wszString = NULL;


    va_list ArgList;
    va_start(ArgList, hToken);

    for(wszString = va_arg(ArgList, LPWSTR); wszString != NULL; wszString = va_arg(ArgList, LPWSTR))
    {
        awszStrings[cStrings++] = wszString;
        if(cStrings >= ARRAYSIZE(awszStrings))
        {
            break;
        }
    }

    va_end(ArgList);

    // event logging code
    hEventSource = RegisterEventSourceW(NULL, EVENTLOG_SOURCE);

    if(NULL == hEventSource)
        goto Ret;


    // check if the token is non zero is so then impersonating so get the SID
    if (hToken)
    {
        ptgUser = (PTOKEN_USER)FastBuffer; // try fast buffer first
        cbUser = 256;

        if (!GetTokenInformation(
                        hToken,    // identifies access token
                        TokenUser, // TokenUser info type
                        ptgUser,   // retrieved info buffer
                        cbUser,  // size of buffer passed-in
                        &cbUser  // required buffer size
                        ))
        {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                //
                // try again with the specified buffer size
                //

                if (NULL != (ptgUser = (PTOKEN_USER)AEAlloc(cbUser)))
                {
                    fAlloced = TRUE;

                    // get the user info and assign the sid if able to
                    if (GetTokenInformation(
                                    hToken,    // identifies access token
                                    TokenUser, // TokenUser info type
                                    ptgUser,   // retrieved info buffer
                                    cbUser,  // size of buffer passed-in
                                    &cbUser  // required buffer size
                                    ))
                    {
                        pSID = ptgUser->User.Sid;
                    }
                }
            }

        }
        else
        {
            // assign the sid when fast buffer worked
            pSID = ptgUser->User.Sid;
        }
    }
    switch(dwEventId >> 30)
    {
    case 0:
        dwEventType = EVENTLOG_SUCCESS;
        break;
    case 1:
        dwEventType = EVENTLOG_INFORMATION_TYPE;
        break;
    case 2:
        dwEventType = EVENTLOG_WARNING_TYPE;
        break;
    case 3:
        dwEventType = EVENTLOG_ERROR_TYPE;
        break;
    }

    // UNDONE - probably want a string to go with the error code
    if (!ReportEventW(hEventSource, // handle of event source
                 dwEventType,  // event type
                 0,                    // event category
                 dwEventId,            // event ID
                 pSID,                 // current user's SID
                 cStrings,             // strings in lpszStrings
                 0,                    // no bytes of raw data
                 (LPCWSTR*)awszStrings,// array of error strings
                 NULL                  // no raw data
                 ))
    {
        goto Ret;
    }

Ret:

    if (hEventSource)
        (VOID) DeregisterEventSource(hEventSource);
    return;
}

//
// Name:    LogAutoEnrollmentError
//
// Description: This function registers an event in the event log of the
//              local machine.
//

void LogAutoEnrollmentError(
                            IN HRESULT hr,
                            IN DWORD   dwEventId,
                            IN BOOL fMachineEnrollment,
                            IN HANDLE hToken,
                            IN LPWSTR wszCertType,
                            IN LPWSTR wszCA
                            )
{
    HKEY        hRegKey = 0;
	WCHAR       szMsg[512];
    HANDLE      hEventSource = 0;
    LPWSTR      lpszStrings[4];
    WORD        cStrings = 0;

    BYTE        FastBuffer[256];
    PTOKEN_USER ptgUser;
    DWORD       cbUser;
    BOOL        fAlloced = FALSE;
    PSID        pSID = NULL;

    WORD dwEventType = 0;


    // event logging code
    hEventSource = RegisterEventSourceW(NULL, EVENTLOG_SOURCE);

    if(NULL == hEventSource)
        goto Ret;

    wsprintfW(szMsg, L"0x%lx", hr);        
    lpszStrings[cStrings++] = szMsg;


    FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (WCHAR *) &lpszStrings[cStrings++],
            0,
            NULL);
    
    if(wszCertType)
    {
        lpszStrings[cStrings++] = wszCertType;
    }

    if(wszCA)
    {
        lpszStrings[cStrings++] = wszCA;
    }   


    // check if the token is non zero is so then impersonating so get the SID
    if (hToken)
    {
        ptgUser = (PTOKEN_USER)FastBuffer; // try fast buffer first
        cbUser = 256;

        if (!GetTokenInformation(
                        hToken,    // identifies access token
                        TokenUser, // TokenUser info type
                        ptgUser,   // retrieved info buffer
                        cbUser,  // size of buffer passed-in
                        &cbUser  // required buffer size
                        ))
        {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                //
                // try again with the specified buffer size
                //

                if (NULL != (ptgUser = (PTOKEN_USER)AEAlloc(cbUser)))
                {
                    fAlloced = TRUE;

                    // get the user info and assign the sid if able to
                    if (GetTokenInformation(
                                    hToken,    // identifies access token
                                    TokenUser, // TokenUser info type
                                    ptgUser,   // retrieved info buffer
                                    cbUser,  // size of buffer passed-in
                                    &cbUser  // required buffer size
                                    ))
                    {
                        pSID = ptgUser->User.Sid;
                    }
                }
            }

        }
        else
        {
            // assign the sid when fast buffer worked
            pSID = ptgUser->User.Sid;
        }
    }
    switch(dwEventId >> 30)
    {
    case 0:
        dwEventType = EVENTLOG_SUCCESS;
        break;
    case 1:
        dwEventType = EVENTLOG_INFORMATION_TYPE;
        break;
    case 2:
        dwEventType = EVENTLOG_WARNING_TYPE;
        break;
    case 3:
        dwEventType = EVENTLOG_ERROR_TYPE;
        break;
    }

    // UNDONE - probably want a string to go with the error code
    if (!ReportEventW(hEventSource, // handle of event source
                 dwEventType,  // event type
                 0,                    // event category
                 dwEventId,            // event ID
                 pSID,                 // current user's SID
                 cStrings,             // strings in lpszStrings
                 0,                    // no bytes of raw data
                 (LPCWSTR*)lpszStrings,// array of error strings
                 NULL                  // no raw data
                 ))
    {
        goto Ret;
    }

Ret:
    if (hRegKey)
        RegCloseKey(hRegKey);
    if (hEventSource)
        (VOID) DeregisterEventSource(hEventSource);

    if((cStrings == 2) && lpszStrings[1])
    {
        AEFree(lpszStrings[1]);
    }
    return;
}

//+-------------------------------------------------------------------------
//  Microsoft Auto Enrollment Object Identifiers
//+-------------------------------------------------------------------------

//
// Name:    LoadAndCallEnrollmentProvider
//
// Description: This function loads the specified Auto Enrollment provider,
//              and calls the entry point to that provider.  It then
//              unloads the provider.
//
  
BOOL LoadAndCallEnrollmentProvider(
                                   IN BOOL fMachineEnrollment,
                                   IN PAUTO_ENROLL_INFO pEnrollmentInfo
                                   )
{
    HANDLE  hAutoEnrollProv = 0;
    FARPROC pEntryPoint = NULL;
    BOOL    fRet = FALSE;

    AE_BEGIN(L"LoadAndCallEnrollmentProvider");

    // load the auto enrollment provider and get the entry point
    if (NULL == (hAutoEnrollProv =
        LoadLibraryA(pEnrollmentInfo->pszAutoEnrollProvider)))
    {
        AE_DEBUG((AE_ERROR, L"Could not load auto-enrollment provider %ls\n\r", pEnrollmentInfo->pszAutoEnrollProvider));
        goto Ret;
    }

    if (NULL == (pEntryPoint = GetProcAddress(hAutoEnrollProv,
                                              "ProvAutoEnrollment")))
    {
        AE_DEBUG((AE_ERROR, L"Entry point ProvAutoEnrollment not found in %ls\n\r", pEnrollmentInfo->pszAutoEnrollProvider));
        goto Ret;
    }

    if (FALSE == pEntryPoint(fMachineEnrollment, pEnrollmentInfo))
    {
        AE_DEBUG((AE_ERROR, L"Enrollment Failed, wizard returned %lx error\n", GetLastError()));
        LogAutoEnrollmentError(HRESULT_FROM_WIN32(GetLastError()),
                               EVENT_AE_ENROLLMENT_FAILED,
                               fMachineEnrollment,
                               NULL,
                               pEnrollmentInfo->pwszCertType, pEnrollmentInfo->pwszCAAuthority);
        goto Ret;
    }


    AE_DEBUG((AE_WARNING, L"Enrolled for a %ls certificate\n", pEnrollmentInfo->pwszCertType));


    fRet = TRUE;
Ret:
    if (hAutoEnrollProv)
        FreeLibrary(hAutoEnrollProv);
    AE_END();
    return fRet;
}

//
// Name:    InitInternalInfo
//
// Description: This function initializes information needed to proceed with
//              auto enrollment.
//
  
HRESULT InitInternalInfo(
                         LDAP *pld,
                      IN BOOL fMachineEnrollment,
                      IN HANDLE hToken,
                      OUT PINTERNAL_INFO pInternalInfo
                      )
{
    HRESULT     hrLocal = S_OK;
    HRESULT     hrNetwork  = S_OK;
    DWORD       dwOpenStoreFlags = CERT_SYSTEM_STORE_CURRENT_USER;
    DWORD       dwErr = 0;
    BOOL        fRet = FALSE;
    DWORD       ldaperr;
    DWORD       cNameBuffer;
    LDAPMessage * SearchResult = NULL;
    LDAPMessage * PrincipalAttributes = NULL;
    HCAINFO     hCACurrent = NULL;
    DWORD       iCAIndex, cCA;
    DWORD       cbHash;
    struct l_timeval        timeout;

    

    // Initialize LDAP session
    LPTSTR wszSearchUser = TEXT("(objectCategory=user)");
    LPTSTR wszSearchComputer = TEXT("(objectCategory=computer)");

    // We want the following attributes
    LPTSTR AttrsUser[] = {
                        DS_ATTR_COMMON_NAME,
                        DS_ATTR_EMAIL_ADDR,
                        DS_ATTR_OBJECT_GUID,
                        DS_ATTR_UPN,
                        NULL,
                      };
    LPTSTR AttrsComputer[] = {
                        DS_ATTR_COMMON_NAME,
                        DS_ATTR_DNS_NAME,
                        DS_ATTR_EMAIL_ADDR,
                        DS_ATTR_OBJECT_GUID,
                        DS_ATTR_UPN,
                        NULL,
                      };

    AE_BEGIN(L"InitInternalInfo");
    pInternalInfo->fMachineEnrollment = fMachineEnrollment;

    pInternalInfo->hToken = hToken;

    if (fMachineEnrollment)
    {
        dwOpenStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
    }

    // open the appropriate ROOT store
    if (NULL == (pInternalInfo->hRootStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_W, 0, 0, dwOpenStoreFlags | CERT_STORE_READONLY_FLAG, L"ROOT")))
    {
        hrLocal = HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_ERROR, L"Unable to open ROOT store (%lx)\n\r", hrLocal));
        goto Ret;
    }

    // open the appropriate CA store
    if (NULL == (pInternalInfo->hCAStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_W, 0, 0, dwOpenStoreFlags | CERT_STORE_READONLY_FLAG, L"CA")))
    {
        hrLocal = HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_ERROR, L"Unable to open CA store (%lx)\n\r", hrLocal));
        goto Ret;
    }

    // open the appropriate MY store
    if (NULL == (pInternalInfo->hMYStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_W, 0, 0, dwOpenStoreFlags, L"MY")))
    {
        hrLocal = HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_ERROR, L"Unable to open MY store (%lx)\n\r", hrLocal));
        goto Ret;
    }
    if(!CertControlStore(pInternalInfo->hMYStore, 0, CERT_STORE_CTRL_AUTO_RESYNC, NULL))
    {
        hrLocal = HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_ERROR, L"Unable configure MY store for auto-resync(%lx)\n\r", hrLocal));
        goto Ret;
    }

    cNameBuffer = MAX_DN_SIZE;
    pInternalInfo->wszDN = AEAlloc(cNameBuffer*sizeof(TCHAR));
    if(pInternalInfo->wszDN == NULL)
    {
        hrLocal = E_OUTOFMEMORY;
        goto Ret;
    }
    if(!g_pfnGetUserNameEx(NameFullyQualifiedDN, pInternalInfo->wszDN, &cNameBuffer))
    {
        hrLocal  = HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_ERROR, L"GetUserNameEx Failed (%lx)\n\r", hrLocal));

        goto Ret;
    } 
    // Normalize the directory DN into a 
    // real BER encoded name
    pInternalInfo->blobDN.cbData = 0;
    CertStrToName(X509_ASN_ENCODING,
                  pInternalInfo->wszDN,
                  CERT_X500_NAME_STR,
                  NULL,
                  NULL,
                  &pInternalInfo->blobDN.cbData,
                  NULL);
    if(pInternalInfo->blobDN.cbData == 0)
    {
        hrLocal  = HRESULT_FROM_WIN32(GetLastError());
        goto Ret; 
    }
    pInternalInfo->blobDN.pbData = AEAlloc(pInternalInfo->blobDN.cbData);
    if(pInternalInfo->blobDN.pbData == NULL)
    {
        hrLocal = E_OUTOFMEMORY;
        goto Ret;
    }
    if(!CertStrToName(X509_ASN_ENCODING,
                  pInternalInfo->wszDN,
                  CERT_X500_NAME_STR,
                  NULL,
                  pInternalInfo->blobDN.pbData,
                  &pInternalInfo->blobDN.cbData,
                  NULL))
    {
        hrLocal  = HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_ERROR, L"Could not encode DN (%lx)\n\r", hrLocal));
        goto Ret;
    }
                                

    timeout.tv_sec = 300;
    timeout.tv_usec = 0;


    ldaperr = g_pfnldap_search_ext_s(pld, 
                  pInternalInfo->wszDN,
                  LDAP_SCOPE_BASE,
                  (fMachineEnrollment?wszSearchComputer:wszSearchUser),
                  (fMachineEnrollment?AttrsComputer:AttrsUser),
                  0,
                  NULL,
                  NULL,
                  &timeout,
                  10000,
                  &SearchResult);

    if(ldaperr != LDAP_SUCCESS)
    {
        hrNetwork  = HRESULT_FROM_WIN32(g_pfnLdapMapErrorToWin32(ldaperr));
        AE_DEBUG((AE_ERROR, L"ldap_search_ext_s failed (%lx)\n\r", hrLocal));
        goto Ret;
    }


    PrincipalAttributes = 
        g_pfnldap_first_entry(pld, 
                         SearchResult); 

    if(NULL == PrincipalAttributes)
    {
        AE_DEBUG((AE_ERROR, L"no user entity found\n\r"));
        hrNetwork  = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
        goto Ret;
    }
    if(fMachineEnrollment)
    {
        pInternalInfo->awszldap_UPN = g_pfnldap_get_values(pld, 
                                      PrincipalAttributes, 
                                      DS_ATTR_DNS_NAME);

        if((pInternalInfo->awszldap_UPN) &&
            (*pInternalInfo->awszldap_UPN))
        {
            AE_DEBUG((AE_INFO, L"ldap DNS Name %ls\n\r", *pInternalInfo->awszldap_UPN));
        }

    }
    else
    {
        pInternalInfo->awszldap_UPN = g_pfnldap_get_values(pld, 
                                          PrincipalAttributes, 
                                          DS_ATTR_UPN);
        if((pInternalInfo->awszldap_UPN == NULL) ||
           (*pInternalInfo->awszldap_UPN == NULL))
        {
            LPTSTR wszUPNBuffer = NULL;
            DWORD  cbUPNBuffer = 0;
            LPTSTR *awszExplodedDN, * pwszCurrent;
            // Build a UPN.  The UPN is built from 
            // The username (without the SAM domain), 


            // Get a buffer that will be big enough
            GetUserName(NULL, &cbUPNBuffer);
            if(cbUPNBuffer == 0)
            {
                hrLocal  = HRESULT_FROM_WIN32(GetLastError());
                goto Ret;
            }

            cbUPNBuffer += _tcslen(pInternalInfo->wszDN)*sizeof(TCHAR);

            wszUPNBuffer = AEAlloc(cbUPNBuffer);
            if(wszUPNBuffer == NULL)
            {
                hrLocal = E_OUTOFMEMORY;
                goto Ret;
            }
            if(!GetUserName(wszUPNBuffer, &cbUPNBuffer))
            {
                hrLocal  = HRESULT_FROM_WIN32(GetLastError());
                goto Ret;
            }

            awszExplodedDN = g_pfnldap_explode_dn(pInternalInfo->wszDN, 0);
            if(awszExplodedDN != NULL)
            {
                _tcscat(wszUPNBuffer, TEXT("@"));
                pwszCurrent = awszExplodedDN;
                while(*pwszCurrent)
                {
                    if(0 == _tcsncmp(*pwszCurrent, TEXT("DC="), 3))
                    {
                        _tcscat(wszUPNBuffer, (*pwszCurrent)+3);
                        _tcscat(wszUPNBuffer, TEXT("."));
                    }
                    pwszCurrent++;
                }
                // remove the trailing '.' or "@" if there were no DC=

                wszUPNBuffer[_tcslen(wszUPNBuffer)-1] = 0;
            }
            pInternalInfo->wszConstructedUPN = wszUPNBuffer;
            AE_DEBUG((AE_INFO, L"Constructed UPN %ls\n\r", pInternalInfo->wszConstructedUPN));
        }
        else
        {
            AE_DEBUG((AE_INFO, L"ldap UPN %ls\n\r", *pInternalInfo->awszldap_UPN));

        }
    }
    pInternalInfo->awszEmail = g_pfnldap_get_values(pld, 
                                      PrincipalAttributes, 
                                      DS_ATTR_EMAIL_ADDR);

    if((pInternalInfo->awszEmail) &&
        (*pInternalInfo->awszEmail))
    {
        AE_DEBUG((AE_INFO, L"E-mail name %ls\n\r", *pInternalInfo->awszEmail));
    }
    // Build up the list of CA's
    // Build up the list of CA's
    hrNetwork = CAEnumFirstCA((LPCWSTR)pld, 
                       CA_FLAG_SCOPE_IS_LDAP_HANDLE |
                       (fMachineEnrollment?CA_FIND_LOCAL_SYSTEM:0), 
                       &hCACurrent);
    if(hrNetwork != S_OK)
    {
        goto Ret;
    }
    if((hCACurrent == NULL) || (0 == (cCA =  CACountCAs(hCACurrent))))
    {
        pInternalInfo->ccaList = 0;
        AE_DEBUG((AE_WARNING, L"No CA's available for auto-enrollment\n\r"));
        goto Ret;
    }


    pInternalInfo->acaList = (PINTERNAL_CA_LIST)AEAlloc(sizeof(INTERNAL_CA_LIST) * cCA);
    if(pInternalInfo->acaList == NULL)
    {
        hrLocal = E_OUTOFMEMORY;
        goto Ret;
    }
    ZeroMemory(pInternalInfo->acaList, sizeof(INTERNAL_CA_LIST) * cCA);
    AE_DEBUG((AE_INFO, L" %d CA's in enterprise\n\r", cCA));

    pInternalInfo->ccaList = 0;
    hrLocal = S_OK;
    hrNetwork = S_OK;

    for(iCAIndex = 0; iCAIndex < cCA; iCAIndex++ )       
    {
        PCCERT_CONTEXT pCert = NULL;
        LPWSTR *awszName = NULL;
        HCAINFO hCANew = NULL;

        if(iCAIndex > 0)
        {
            hrNetwork = CAEnumNextCA(hCACurrent, &hCANew);
        }
        // Clean up from previous

        if(pInternalInfo->acaList[pInternalInfo->ccaList].wszName)
        {
            AEFree(pInternalInfo->acaList[pInternalInfo->ccaList].wszName);
        }
        if(pInternalInfo->acaList[pInternalInfo->ccaList].wszDNSName)
        {
            AEFree(pInternalInfo->acaList[pInternalInfo->ccaList].wszDNSName);
        }
        if(pInternalInfo->acaList[iCAIndex].awszCertificateTemplates)
        {
            CAFreeCAProperty(pInternalInfo->acaList[pInternalInfo->ccaList].hCAInfo,
                             pInternalInfo->acaList[iCAIndex].awszCertificateTemplates);
        }

        if(pInternalInfo->acaList[pInternalInfo->ccaList].hCAInfo)
        {
            CACloseCA(pInternalInfo->acaList[pInternalInfo->ccaList].hCAInfo);
            pInternalInfo->acaList[pInternalInfo->ccaList].hCAInfo = NULL;
        }
         
        if((hrNetwork != S_OK) ||
           (hrLocal != S_OK))
        {
            break;
        }

        if(iCAIndex > 0)
        {
            hCACurrent = hCANew; 
        }

        if(hCACurrent == NULL)
        {
            break;
        }

        pInternalInfo->acaList[pInternalInfo->ccaList].hCAInfo = hCACurrent;

        hrNetwork = CAGetCAProperty(hCACurrent, 
                             CA_PROP_NAME,
                             & awszName);
        if(hrNetwork != S_OK)
        {
            AE_DEBUG((AE_INFO, L"No name property for ca\n\r"));
            // skip to the next one.
            hrNetwork = S_OK;
            continue;
        }
        if((awszName != NULL) && (*awszName != NULL))
        {
            pInternalInfo->acaList[pInternalInfo->ccaList].wszName = AEAlloc(sizeof(WCHAR)*(wcslen(*awszName)+1));
            if(pInternalInfo->acaList[pInternalInfo->ccaList].wszName == NULL)
            {        
                CAFreeCAProperty(hCACurrent, awszName);
                hrLocal = E_OUTOFMEMORY;
                continue;
            }
            wcscpy(pInternalInfo->acaList[pInternalInfo->ccaList].wszName, *awszName);
        }
        else
        {
            AE_DEBUG((AE_INFO, L"No name property for ca\n\r"));
            if(awszName != NULL)
            {        
                CAFreeCAProperty(hCACurrent, awszName);
            }
            // skip to the next one
            continue;
        }

        CAFreeCAProperty(hCACurrent, awszName);
        hrNetwork = CAGetCAProperty(hCACurrent, 
                             CA_PROP_DNSNAME,
                             & awszName);
        if(hrNetwork != S_OK)
        {
            AE_DEBUG((AE_INFO, L"No DNS property for CA %ls\n\r", pInternalInfo->acaList[pInternalInfo->ccaList].wszName));
            hrNetwork = S_OK;
            continue;
        }
        if((awszName != NULL) && (*awszName != NULL))
        {
            pInternalInfo->acaList[pInternalInfo->ccaList].wszDNSName = AEAlloc(sizeof(WCHAR)*(wcslen(*awszName)+1));
            if(pInternalInfo->acaList[pInternalInfo->ccaList].wszDNSName == NULL)
            {
                CAFreeCAProperty(hCACurrent, awszName);
                hrLocal = E_OUTOFMEMORY;
                continue;
            }
            wcscpy(pInternalInfo->acaList[pInternalInfo->ccaList].wszDNSName, *awszName);
        }
        else
        {
            AE_DEBUG((AE_INFO, L"No DNS property for CA %ls\n\r", pInternalInfo->acaList[pInternalInfo->ccaList].wszName));
            if(awszName != NULL)
            {        
                CAFreeCAProperty(hCACurrent, awszName);
            }
            continue;
        }


        CAFreeCAProperty(hCACurrent, awszName);
        hrNetwork = CAGetCAProperty(hCACurrent, 
                             CA_PROP_CERT_TYPES,
                             & pInternalInfo->acaList[pInternalInfo->ccaList].awszCertificateTemplates);
        if(hrNetwork != S_OK)
        {
            AE_DEBUG((AE_INFO, L"No cert type property for CA %ls\n\r", pInternalInfo->acaList[pInternalInfo->ccaList].wszName));
            continue;
        }

        hrNetwork = CAGetCACertificate(hCACurrent, &pCert);
        if(hrNetwork != S_OK)
        {
            AE_DEBUG((AE_INFO, L"No certificate property for CA %ls\n\r", pInternalInfo->acaList[pInternalInfo->ccaList].wszName));
            continue;
        }
        cbHash = sizeof(pInternalInfo->acaList[pInternalInfo->ccaList].CACertHash);

        if(!CertGetCertificateContextProperty(pCert,
                                          CERT_SHA1_HASH_PROP_ID,
                                          pInternalInfo->acaList[pInternalInfo->ccaList].CACertHash,
                                          &cbHash))
        {
            continue;
        }
        CertFreeCertificateContext(pCert);
        AE_DEBUG((AE_INFO, L"CA %ls\\%ls available\n\r", 
                 pInternalInfo->acaList[pInternalInfo->ccaList].wszName,
                 pInternalInfo->acaList[pInternalInfo->ccaList].wszDNSName));
        pInternalInfo->ccaList++;
    }
    if(pInternalInfo->ccaList == 0)
    {
        AE_DEBUG((AE_WARNING, L"No CA's available for auto-enrollment\n\r"));
    }

    fRet = TRUE;
Ret:
    if (hrLocal != S_OK)
    {
        LogAutoEnrollmentError(hrLocal,
                               EVENT_AE_LOCAL_CYCLE_INIT_FAILED,
                               pInternalInfo->fMachineEnrollment,
                               pInternalInfo->hToken,
                               NULL, NULL);
    }
    if (hrNetwork != S_OK)
    {
        LogAutoEnrollmentError(hrNetwork,
                               EVENT_AE_NETWORK_CYCLE_INIT_FAILED,
                               pInternalInfo->fMachineEnrollment,
                               pInternalInfo->hToken,
                               NULL, NULL);
    }

    if(SearchResult)
    {
        g_pfnldap_msgfree(SearchResult);
    }

    AE_END();
    if(hrLocal != S_OK)
    {
        return hrLocal;
    }
    return hrNetwork;
}

//
// Name:    FreeInternalInfo
//
// Description: This function frees and resources which were needed for
//              auto enrollment.
//
  
void FreeInternalInfo(
                      IN PINTERNAL_INFO pInternalInfo
                      )
{
    DWORD i;
    if (pInternalInfo->hRootStore)
        CertCloseStore(pInternalInfo->hRootStore, 0);
    if (pInternalInfo->hCAStore)
        CertCloseStore(pInternalInfo->hCAStore, 0);
    if (pInternalInfo->hMYStore)
        CertCloseStore(pInternalInfo->hMYStore, 0);

    if(pInternalInfo->awszldap_UPN)
    {
        g_pfnldap_value_free(pInternalInfo->awszldap_UPN);
    }

    if(pInternalInfo->awszEmail)
    {
        g_pfnldap_value_free(pInternalInfo->awszEmail);
    }

    if(pInternalInfo->wszDN)
    {
        AEFree(pInternalInfo->wszDN);
    }
    if(pInternalInfo->blobDN.pbData)
    {
        AEFree(pInternalInfo->blobDN.pbData);
    }

    if(pInternalInfo->wszConstructedUPN)
    {
        AEFree(pInternalInfo->wszConstructedUPN);
    }

    if( pInternalInfo->acaList )
    {

        for(i=0; i <pInternalInfo->ccaList; i++)
        {
            if(pInternalInfo->acaList[i].wszName)
            {
                AEFree(pInternalInfo->acaList[i].wszName);
            }
            if(pInternalInfo->acaList[i].wszDNSName)
            {
                AEFree(pInternalInfo->acaList[i].wszDNSName);
            }
            if(pInternalInfo->acaList[i].awszCertificateTemplates)
            {

                CAFreeCAProperty(pInternalInfo->acaList[i].hCAInfo,
                                 pInternalInfo->acaList[i].awszCertificateTemplates);
            }
            if(pInternalInfo->acaList[i].hCAInfo)
            {
                CACloseCA(pInternalInfo->acaList[i].hCAInfo);
            }
        }
        AEFree(pInternalInfo->acaList);
    }
}


//
// Name:    InitInstance
//
// Description: This function initializes information needed to proceed with
//              auto enrollment.
//
BOOL InitInstance(
                      IN PCCTL_CONTEXT pCTLContext,
                      IN PINTERNAL_INFO pInternalInfo,
                      OUT PAE_INSTANCE_INFO pInstance
                      )
{
    FILETIME    ft;


    pInstance->pCTLContext = CertDuplicateCTLContext(pCTLContext);
    pInstance->pInternalInfo = pInternalInfo;

    // choose a random CA order
    // Get the current time
    GetSystemTimeAsFileTime(&ft); 

    // use mod to get something marginally random
    // It's an index into the main list of ca's
    if(pInternalInfo->ccaList)
    {
        pInstance->dwRandomIndex = ft.dwLowDateTime %
                    pInternalInfo->ccaList;
    }
    else
    {
        pInstance->dwRandomIndex = 0;
    }

    return TRUE;
}



//
// Name:    FreeInstance
//
// Description: This function frees and resources which were needed for
//              auto enrollment.
//
  
void FreeInstance(
                      IN PAE_INSTANCE_INFO pInstance
                      )
{
    if (pInstance->pOldCert)
        CertFreeCertificateContext(pInstance->pOldCert);

    if (pInstance->pwszCertType)
        AEFree(pInstance->pwszCertType);

    if (pInstance->pwszAEIdentifier)
        AEFree(pInstance->pwszAEIdentifier);

    if (pInstance->pCertTypeExtensions)             // use LocalFree b/c certcli.dll
        LocalFree(pInstance->pCertTypeExtensions);  // uses LocalAlloc to alloc

    if(pInstance->pCTLContext)
    {
        CertFreeCTLContext(pInstance->pCTLContext);
    }
}


// Name:    SetEnrollmentType
//
// Description: This function retrieves additional enrollment information
//              needed to enroll for a cert.
//
  
BOOL SetEnrollmentCertType(
                                 IN PAE_INSTANCE_INFO pInstance,
                                 OUT PAUTO_ENROLL_INFO pEnrollmentInfo
                                 )
{
    BOOL    fRet = FALSE;

    // copy over the cert extensions, this is freed by the FreeInternalInfo
    // function, but after being used in the EnrollmentInfo struct
    pEnrollmentInfo->CertExtensions.cExtension =
            pInstance->pCertTypeExtensions->cExtension;
    pEnrollmentInfo->CertExtensions.rgExtension =
            pInstance->pCertTypeExtensions->rgExtension;

    // copy over the cert type name, this is freed by the FreeInternalInfo
    // function, but after being used in the EnrollmentInfo struct
    pEnrollmentInfo->pwszCertType = pInstance->pwszCertType;


    // The auto enrollment ID is used to uniquely tie an autoenrolled cert to
    // it's autoenrollment object. 
    pEnrollmentInfo->pwszAutoEnrollmentID = pInstance->pwszAEIdentifier;
    

    // copy over the handle to the MY store, this is freed by the
    // FreeInternalInfo  function, but after being used in the
    // EnrollmentInfo struct
    pEnrollmentInfo->hMYStore = pInstance->pInternalInfo->hMYStore;

    // copy over the pointer to the cert context to be renewed, this
    // is freed by the FreeInternalInfo  function, but after being used in the
    // EnrollmentInfo struct
    if ((pInstance->pOldCert) && (pInstance->fRenewalOK))
    {
        pEnrollmentInfo->pOldCert = pInstance->pOldCert;
    }

    // Enrollment controll chooses provider type  based on cert type
    pEnrollmentInfo->dwProvType = 0;
    // Enrollment controll chooses key spec  based on cert type
    pEnrollmentInfo->dwKeySpec = 0;

    // UNDONE - currently the gen key flags is hard coded to 0x0
    pEnrollmentInfo->dwGenKeyFlags = 0;

    fRet = TRUE;
//Ret:
    return fRet;
}



// Name:    GetCertTypeInfo
//
// Description: This function retrieves information (the extensions) for the
//              cert type specified in the ListIdentifier field of the auto enrollment
//              object (CTL) in the internal info structure.  In addition the
//              function makes a call to check if the current entity has permission
//              to enroll for this cert type.
//

BOOL GetCertTypeInfo(
                     IN OUT PAE_INSTANCE_INFO pInstance,
                     IN LDAP * pld,
                     OUT BOOL *pfPermissionToEnroll
                     )
{
    HRESULT     hr = S_OK;
    HCERTTYPE   hCertType = 0;
    DWORD       dwErr = 0;
    BOOL        fRet = FALSE;
    LPWSTR      *awszName = NULL;

    LPWSTR      wszCertTypeName = NULL;

    CERT_EXTENSIONS     CertTypeExtensions;
    AE_BEGIN(L"GetCertTypeInfo");

    *pfPermissionToEnroll = FALSE;



    AE_DEBUG((AE_INFO, L"Found auto-enrollment object with cert type: %ls\n\r", 
              pInstance->pCTLContext->pCtlInfo->ListIdentifier.pbData));

    wszCertTypeName = wcschr((LPWSTR)pInstance->pCTLContext->pCtlInfo->ListIdentifier.pbData, L'|');
    if(wszCertTypeName)
    {
        wszCertTypeName++;
    }
    else
    {
        wszCertTypeName = (LPWSTR)pInstance->pCTLContext->pCtlInfo->ListIdentifier.pbData;
    }

    

    // get a handle to the cert type
    if (S_OK != (hr = CAFindCertTypeByName(wszCertTypeName,
                                     (HCAINFO)pld,                    // special optimization, ldap handle passed as scope
                                     CT_FLAG_SCOPE_IS_LDAP_HANDLE |
                                     (pInstance->pInternalInfo->fMachineEnrollment?
                                       CT_ENUM_MACHINE_TYPES  | CT_FIND_LOCAL_SYSTEM :
                                       CT_ENUM_USER_TYPES), 
                                     &hCertType)))
    {
        AE_DEBUG((AE_WARNING, L"Unknown cert type: %ls\n\r", pInstance->pwszCertType));
        goto Ret;
    }
    // get the extensions for the cert type
    if (S_OK != (hr = CAGetCertTypeProperty(hCertType,
                                      CERTTYPE_PROP_DN,
                                      &awszName)))
    {
        AE_DEBUG((AE_WARNING, L"Could not get cert type full name: %ls\n\r",
                 pInstance->pCTLContext->pCtlInfo->ListIdentifier.pbData));
        goto Ret;
    }
    if((awszName == NULL) || (*awszName == NULL))
    {
        AE_DEBUG((AE_WARNING, L"Could not get cert type full name: %ls\n\r",
                 pInstance->pCTLContext->pCtlInfo->ListIdentifier.pbData));
        hr = CERTSRV_E_PROPERTY_EMPTY;
        goto Ret;
    }

    if (NULL == (pInstance->pwszCertType = (LPWSTR)AEAlloc(
        (wcslen(*awszName) + 1)*sizeof(WCHAR))))
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }
    wcscpy(pInstance->pwszCertType, *awszName);

    if (NULL == (pInstance->pwszAEIdentifier = (LPWSTR)AEAlloc(
        (wcslen((LPWSTR)pInstance->pCTLContext->pCtlInfo->ListIdentifier.pbData) + 1)*sizeof(WCHAR))))
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }
    wcscpy(pInstance->pwszAEIdentifier, (LPWSTR)pInstance->pCTLContext->pCtlInfo->ListIdentifier.pbData);

    // get the extensions for the cert type
    if (S_OK != (hr = CAGetCertTypeExtensions(hCertType,
                                        &pInstance->pCertTypeExtensions)))
    {
        AE_DEBUG((AE_WARNING, L"Could not get cert type extensions: %ls\n\r", pInstance->pwszCertType));
        goto Ret;
    }

    // get the extensions for the cert type
    if (S_OK != (hr = CAGetCertTypeFlags(hCertType,
                                   &pInstance->dwCertTypeFlags)))
    {
        AE_DEBUG((AE_WARNING, L"Could not get cert type flags: %ls\n\r", pInstance->pwszCertType));
        goto Ret;
    }

    // get the expiration offset
    if (S_OK != (hr = CAGetCertTypeExpiration(hCertType,
                                        NULL,
                                        (LPFILETIME)&pInstance->ftExpirationOffset)))
    {
        AE_DEBUG((AE_WARNING, L"Could not get cert type expirations: %ls\n\r", pInstance->pwszCertType));
        goto Ret;
    }

    *pfPermissionToEnroll = (S_OK == CACertTypeAccessCheck(hCertType, pInstance->pInternalInfo->hToken));


    fRet = TRUE;
Ret:
    if (hr != S_OK)
    {
        LogAutoEnrollmentError(hr,
                               EVENT_UAE_UNKNOWN_CERT_TYPE,
                               pInstance->pInternalInfo->fMachineEnrollment,
                               pInstance->pInternalInfo->hToken, 
                               (LPWSTR)pInstance->pCTLContext->pCtlInfo->ListIdentifier.pbData, NULL);
    }
    // close the handle to the cert type
    if (hCertType)
    {
        if(awszName)
        {
            CAFreeCertTypeProperty(hCertType, awszName);
        }
        CACloseCertType(hCertType);
    }

    AE_END();
    return fRet;
}




//
// Name:    CompareEnhancedKeyUsageExtensions
//
// Description: This function checks if a the enhanced key usage extensions
//              in a certificate contain the enhanced key usage extensions
//              from the auto enrollment object (CTL),
//
  
HRESULT CompareEnhancedKeyUsageExtensions(
                                       IN PAE_INSTANCE_INFO         pInstance,
                                       IN PCCERT_CONTEXT            pCertContext,
                                       IN OUT PAE_CERT_TEST_ARRAY  *ppAEData
                                       )
{
    HRESULT             hr = S_OK;
    PCERT_ENHKEY_USAGE  pCertUsage = NULL;
    DWORD               cbCertUsage;
    PCERT_ENHKEY_USAGE  pAEObjUsage = NULL;
    DWORD               cbAEObjUsage;
    PCERT_EXTENSION     pAEObjUsageExt;
    PCERT_EXTENSION     pCertUsageExt;
    DWORD               i;
    DWORD               j;

    LPWSTR wszCertEKU = NULL;
    LPWSTR wszTemplateEKU = NULL;



    // get the enhanced key usages from the auto enrollment obj extensions
    pCertUsageExt = CertFindExtension(szOID_ENHANCED_KEY_USAGE,
                            pCertContext->pCertInfo->cExtension,
                            pCertContext->pCertInfo->rgExtension);



    if(pCertUsageExt)
    {
        if (!CryptDecodeObject(CRYPT_ASN_ENCODING, szOID_ENHANCED_KEY_USAGE, 
                               pCertUsageExt->Value.pbData,
                               pCertUsageExt->Value.cbData,
                               0, NULL, &cbCertUsage))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Ret;
        }

        if (NULL == (pCertUsage = (PCERT_ENHKEY_USAGE)AEAlloc(cbCertUsage)))
        {
            hr = E_OUTOFMEMORY;
            goto Ret;
        }

        if (!CryptDecodeObject(CRYPT_ASN_ENCODING, szOID_ENHANCED_KEY_USAGE, 
                               pCertUsageExt->Value.pbData,
                               pCertUsageExt->Value.cbData,
                               0, pCertUsage, &cbCertUsage))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Ret;
        }
    }
    else
    {
        // No usage, so this cert is good for everything
        goto Ret;

    }



    // get the enhanced key usages from the auto enrollment obj extensions
    pAEObjUsageExt = CertFindExtension(szOID_ENHANCED_KEY_USAGE,
                            pInstance->pCertTypeExtensions->cExtension,
                            pInstance->pCertTypeExtensions->rgExtension);



    if(pAEObjUsageExt)
    {
        if (!CryptDecodeObject(CRYPT_ASN_ENCODING, szOID_ENHANCED_KEY_USAGE, 
                               pAEObjUsageExt->Value.pbData,
                               pAEObjUsageExt->Value.cbData,
                               0, NULL, &cbAEObjUsage))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Ret;
        }

        if (NULL == (pAEObjUsage = (PCERT_ENHKEY_USAGE)AEAlloc(cbAEObjUsage)))
        {
            hr = E_OUTOFMEMORY;
            goto Ret;
        }

        if (!CryptDecodeObject(CRYPT_ASN_ENCODING, szOID_ENHANCED_KEY_USAGE, 
                               pAEObjUsageExt->Value.pbData,
                               pAEObjUsageExt->Value.cbData,
                               0, pAEObjUsage, &cbAEObjUsage))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Ret;
        }
    }
    else
    {
        // The template requires no usage extension, so
        // because the cert has a usage extension, we fail
        // the test.
        goto Failed;

    }


    // check if the number of usages is smaller in the cert then in the
    // auto enrollment object
    if (pCertUsage->cUsageIdentifier < pAEObjUsage->cUsageIdentifier)
    {
        goto Failed;
    }

    // check if all the usages found in the auto enrollment object are in
    // the cert
    for (i=0;i<pAEObjUsage->cUsageIdentifier;i++)
    {
        for (j=0;j<pCertUsage->cUsageIdentifier;j++)
        {
            if (0 == strcmp(pCertUsage->rgpszUsageIdentifier[j],
                            pAEObjUsage->rgpszUsageIdentifier[i]))
            {
                break;
            }
        }
        if (j == pCertUsage->cUsageIdentifier)
        {
            goto Failed;
        }
    }

Ret:

    if(wszCertEKU)
    {
        AEFree(wszCertEKU);
    }

    if(wszTemplateEKU)
    {
        AEFree(wszTemplateEKU);
    }


    if (pCertUsage)
        AEFree(pCertUsage);
    if (pAEObjUsage)
        AEFree(pAEObjUsage);
    return hr;


Failed:


    // Log a failure of this test

    // Build extension strings
    wszCertEKU = HelperExtensionToString(pCertUsageExt);

    if(wszCertEKU == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }

    wszTemplateEKU = HelperExtensionToString(pAEObjUsageExt);

    if(wszTemplateEKU == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }

    AELogTestResult(ppAEData,
                    AE_TEST_EXTENSION_EKU,
                    wszCertEKU,
                    wszTemplateEKU);


    goto Ret;
}


#define MAX_KEY_USAGE_SIZE  20   // sizeof(CRYPT_BIT_BLOB) for 64bit + sizeof(DWORD)
//
// Name:    CompareKeyUsageExtensions
//
// Description: This function checks if the key usages
//              in a certificate are a superset of the key usages
//              from the auto enrollment object (CTL),
//
  

HRESULT CompareKeyUsageExtensions(
                               IN PAE_INSTANCE_INFO pInstance,
                               IN PCCERT_CONTEXT pCertContext,
                               IN OUT PAE_CERT_TEST_ARRAY  *ppAEData
                               )
{
    HRESULT             hr = S_OK;
    PCERT_EXTENSION     pCertUsageExt;
    PCERT_EXTENSION     pAEObjUsageExt;
    DWORD               i;
    DWORD               dwMask = (DWORD)-1;
    BYTE                bCertUsageBuffer[MAX_KEY_USAGE_SIZE];
    BYTE                bAEObjUsageBuffer[MAX_KEY_USAGE_SIZE];
    PCRYPT_BIT_BLOB     pCertUsage = (PCRYPT_BIT_BLOB)bCertUsageBuffer;
    PCRYPT_BIT_BLOB     pAEObjUsage = (PCRYPT_BIT_BLOB)bAEObjUsageBuffer;
    DWORD               dwKeyUsage;


    LPWSTR              wszCertKU = NULL;
    LPWSTR              wszTemplateKU = NULL;



    // get the key usages from the cert
    pCertUsageExt = CertFindExtension(szOID_KEY_USAGE,
                            pCertContext->pCertInfo->cExtension,
                            pCertContext->pCertInfo->rgExtension);

    // get the key usages from the auto enrollment obj extensions
    pAEObjUsageExt = CertFindExtension(szOID_KEY_USAGE,
                            pInstance->pCertTypeExtensions->cExtension,
                            pInstance->pCertTypeExtensions->rgExtension);

    // If the cert has no key usage extension, then it's good in general
    if (NULL == pCertUsageExt)
    {
        goto Ret;
    }

    // If the type requires no extension, and the cert has one,
    // then the cert is too limited.
    if(pAEObjUsageExt == NULL)
    {
        goto Failed;
    }

    // Decode the key usage into their basic bit's
    dwKeyUsage = MAX_KEY_USAGE_SIZE;
    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          X509_KEY_USAGE,
                          pCertUsageExt->Value.pbData,
                          pCertUsageExt->Value.cbData,
                          0, 
                          (PVOID *)pCertUsage,
                          &dwKeyUsage))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Ret;
    }

    // Decode the key usage into their basic bit's
    dwKeyUsage = MAX_KEY_USAGE_SIZE;
    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          X509_KEY_USAGE,
                          pAEObjUsageExt->Value.pbData,
                          pAEObjUsageExt->Value.cbData,
                          0, 
                          (PVOID *)pAEObjUsage,
                          &dwKeyUsage))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Ret;
    }

    // Get the mask based on algs
    for(i=0; i < g_cKUMasks; i++)
    {
        if(strcmp(pCertContext->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId, g_aKUMasks[i].pszAlg) == 0)
        {
            dwMask = g_aKUMasks[i].dwMask;
            break;
        }
    }


    // see if auto enroll obj keys usages are a sub set of cert's
    if (pAEObjUsage->cbData > pCertUsage->cbData)
    {
        goto Failed;
    }

    for (i=0;i<pAEObjUsage->cbData;i++)
    {
        BYTE bMask = 0xff;
        if(i < sizeof(DWORD))
        {
            bMask = ((PBYTE)&dwMask)[i];
        }
        if ((pAEObjUsage->pbData[i] & bMask ) !=
            ((pAEObjUsage->pbData[i] & bMask )  &
             pCertUsage->pbData[i]))
        {
            goto Failed;
        }
    }



Ret:

    if(wszCertKU)
    {
        AEFree(wszCertKU);
    }

    if(wszTemplateKU)
    {
        AEFree(wszTemplateKU);
    }

    return hr;
Failed:


    // Log a failure of this test

    // Build extension strings
    wszCertKU = HelperExtensionToString(pCertUsageExt);

    if(wszCertKU == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }

    wszTemplateKU = HelperExtensionToString(pAEObjUsageExt);
    if(wszTemplateKU == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }

    AELogTestResult(ppAEData,
                    AE_TEST_EXTENSION_KU,
                    wszCertKU,
                    wszTemplateKU);
    goto Ret;

}
//
// Name:    CompareBasicConstraints
//
// Description: This function checks if the basic constraints
//              in a certificate are a superset of the basic
//              constraints from the auto enrollment object (CTL),
//
  
HRESULT CompareBasicConstraints(
                                IN PAE_INSTANCE_INFO pInstance,
                                IN PCCERT_CONTEXT pCertContext,
                                IN OUT PAE_CERT_TEST_ARRAY  *ppAEData
                               )
{
    HRESULT                         hr = S_OK;
    PCERT_EXTENSION                 pCertConstraints;
    CERT_BASIC_CONSTRAINTS2_INFO    CertConstraintInfo = {FALSE, FALSE, 0};   
    PCERT_EXTENSION                 pAEObjConstraints;
    CERT_BASIC_CONSTRAINTS2_INFO    AEObjConstraintInfo = {FALSE, FALSE, 0};
    DWORD                           cb;
    DWORD                           i;

    LPWSTR wszCertBC = NULL;

    LPWSTR wszTemplateBC = NULL;


    // get the basic constraints from the cert
    pCertConstraints = CertFindExtension(szOID_BASIC_CONSTRAINTS2,
                            pCertContext->pCertInfo->cExtension,
                            pCertContext->pCertInfo->rgExtension);

    // get the basic constraints from the auto enrollment obj extensions
    pAEObjConstraints = CertFindExtension(szOID_BASIC_CONSTRAINTS2,
                            pInstance->pCertTypeExtensions->cExtension,
                            pInstance->pCertTypeExtensions->rgExtension);


    // decode the objects
    if(pCertConstraints)
    {
        cb = sizeof(CertConstraintInfo);
        if (!CryptDecodeObject(CRYPT_ASN_ENCODING, szOID_BASIC_CONSTRAINTS2, 
                               pCertConstraints->Value.pbData,
                               pCertConstraints->Value.cbData,
                               0, &CertConstraintInfo, &cb))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Ret;
        }
    }

    if(pAEObjConstraints)
    {
        cb = sizeof(AEObjConstraintInfo);
        if (!CryptDecodeObject(CRYPT_ASN_ENCODING, szOID_BASIC_CONSTRAINTS2, 
                               pAEObjConstraints->Value.pbData,
                               pAEObjConstraints->Value.cbData,
                               0, &AEObjConstraintInfo, &cb))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Ret;
        }
    }
    // see if auto enroll obj constraints are the same as the cert's
    if (AEObjConstraintInfo.fCA != CertConstraintInfo.fCA)
    {
        goto Failed;
    }
    if (CertConstraintInfo.fCA)
    {
        if (CertConstraintInfo.fPathLenConstraint !=
            AEObjConstraintInfo.fPathLenConstraint)
        {
            goto Failed;
        }
        if (CertConstraintInfo.fPathLenConstraint)
        {
            if (CertConstraintInfo.dwPathLenConstraint >
                AEObjConstraintInfo.dwPathLenConstraint)
            {
                goto Failed;
            }
        }
    }

Ret:

        // Build extension strings

    if(wszCertBC)
    {
        AEFree(wszCertBC);
    }

    if(wszTemplateBC)
    {
        AEFree(wszTemplateBC);
    }

    
    return hr;
Failed:


    // Log a failure of this test

    // Build extension strings
    wszCertBC = HelperExtensionToString(pCertConstraints);

    if(wszCertBC == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }

    wszTemplateBC = HelperExtensionToString(pAEObjConstraints);
    if(wszTemplateBC == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Ret;
    }

    AELogTestResult(ppAEData,
                    AE_TEST_EXTENSION_BC,
                    wszCertBC,
                    wszTemplateBC);
    goto Ret;

}


//
// Name:    VerifyTemplateName
//
// Description: 
//
  
HRESULT VerifyTemplateName(
                             IN PAE_INSTANCE_INFO pInstance,
                             IN PCCERT_CONTEXT pCertContext,
                             IN OUT PAE_CERT_TEST_ARRAY  *ppAEData
                             )
{
    HRESULT hr = S_OK;
    BOOL   fMatch = FALSE;
    PCERT_NAME_VALUE pTemplateName = NULL;



    if(pInstance->dwCertTypeFlags & CT_FLAG_ADD_TEMPLATE_NAME)
    {
        DWORD           cbTemplateName = MAX_TEMPLATE_NAME_VALUE_SIZE;
        BYTE           pbName[MAX_TEMPLATE_NAME_VALUE_SIZE];  // Only needs to be as big as wszDomainController


        PCERT_EXTENSION pCertType = CertFindExtension(szOID_ENROLL_CERTTYPE_EXTENSION,
                                                        pCertContext->pCertInfo->cExtension,
                                                        pCertContext->pCertInfo->rgExtension);


        if(pCertType == NULL)
        {
            goto Failed;
        }
        if(!CryptDecodeObject(X509_ASN_ENCODING,
                          X509_UNICODE_ANY_STRING,
                          pCertType->Value.pbData,
                          pCertType->Value.cbData,
                          0,
                          pbName,
                          &cbTemplateName))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Ret;
        }
        pTemplateName = (PCERT_NAME_VALUE)pbName;
        if(pTemplateName->Value.pbData == NULL)
        {
            goto Failed;
        }

        if(wcscmp((LPWSTR)  pTemplateName->Value.pbData, pInstance->pwszCertType) != 0)
        {
            goto Failed;
        }
    }


Ret:
    return hr;


Failed:


    {
        WCHAR wszTemplateName[MAX_PATH];
        if(pTemplateName)
        {
            wcscpy(wszTemplateName, (LPWSTR)  pTemplateName->Value.pbData);
        }
        else
        {
            if(!LoadString(g_hInstance, IDS_AUTOENROLL_TEMPLATE_EXT, wszTemplateName, MAX_PATH))
            {
                wcscpy(wszTemplateName, L"No template name");
            }

        }
        AELogTestResult(ppAEData,
                    AE_TEST_EXTENSION_TEMPLATE,
                    wszTemplateName,
                    pInstance->pwszCertType);

    }

    goto Ret;
}

//
// Name:    VerifyCommonExtensions
//
// Description: This function checks if a the extensions in a certificate
//              contain the appropriate extensions from the certificate template
//
  
HRESULT VerifyCommonExtensions(
                     IN PAE_INSTANCE_INFO       pInstance,
                     IN PCCERT_CONTEXT          pCertContext,
                     IN OUT PAE_CERT_TEST_ARRAY  *ppAEData
                     )
{
    HRESULT hr = S_OK;


    AE_BEGIN(L"VerifyCommonExtensions");


    if (S_OK != (hr = CompareEnhancedKeyUsageExtensions(pInstance,
                                                        pCertContext,
                                                        ppAEData)))
    {
        goto Ret;
    }

    // check key usages
    if (S_OK != (hr = CompareKeyUsageExtensions(pInstance,
                                                pCertContext,
                                                ppAEData)))
    {
        goto Ret;
    }


    // check basic constraints
    if (S_OK != (hr = CompareBasicConstraints(pInstance,
                                              pCertContext,
                                              ppAEData)))
    {
        goto Ret;
    }

    // Check to see if the cert extension should be there, and if it matches.
    // check basic constraints
    if (S_OK != (hr = VerifyTemplateName(pInstance,
                                         pCertContext,
                                         ppAEData)))
    {
        goto Ret;
    }



Ret:
    AE_END();
    return hr;
}

//
// Name:    VerifyCertificateNaming
//
// Description: Determine whether the acutal naming information
//              for the user matches that in the certificate.
//
  
HRESULT VerifyCertificateNaming(
                     IN PAE_INSTANCE_INFO           pInstance,
                     IN PCCERT_CONTEXT              pCert,
                     IN OUT PAE_CERT_TEST_ARRAY  *  ppAEData
                     )
{

    HRESULT             hr = S_OK;
    PCERT_NAME_INFO     pInfo = NULL;

    DWORD           cbInfo = 0;
    DWORD           iRDN, iATTR;
    DWORD           iExtension;

    BOOL            fSubjectUPNMatch = FALSE;
    BOOL            fSubjectEmailMatch = FALSE;

    BOOL            fAltSubjectEmailMatch = FALSE;
    BOOL            fDNMatch = FALSE;
    BOOL            fDNSMatch = FALSE;
    BOOL            fObjIDMatch = FALSE;
    BOOL            fAltSubjectUPNMatch = FALSE;

    BOOL            fDisplaySubjectName = FALSE;
    BOOL            fDisplayAltSubjectName = FALSE;

    AE_BEGIN(L"VerifyCertificateNaming");


    // First, check if the cert type specifies enrollee supplied subject name
    if(0 != (pInstance->dwCertTypeFlags & CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT))
    {
        // We don't care what's in the cert, so return.
        goto Ret;
    }

    fSubjectEmailMatch = fAltSubjectEmailMatch = ((pInstance->pInternalInfo->awszEmail == NULL) ||
               (*pInstance->pInternalInfo->awszEmail == NULL));

    fObjIDMatch = (0 == (pInstance->dwCertTypeFlags & CT_FLAG_ADD_OBJ_GUID));

    // Verify the names in the Subject Name


    if(!CryptDecodeObjectEx(pCert->dwCertEncodingType,
                        X509_NAME,
                        pCert->pCertInfo->Subject.pbData,
                        pCert->pCertInfo->Subject.cbData,
                        CRYPT_ENCODE_ALLOC_FLAG,
                        NULL,
                        &pInfo,
                        &cbInfo))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_ERROR, L"Could not decode certificate name (%lx)\n\r", hr));
        goto Ret;
    }

    AE_DEBUG((AE_TRACE, L"Comparing Subject Name\n\r"));

    for(iRDN = 0; iRDN < pInfo->cRDN; iRDN++)
    {
        for(iATTR = 0; iATTR < pInfo->rgRDN[iRDN].cRDNAttr; iATTR++)
        {
            LPTSTR wszRDNAttr = NULL;
            DWORD  cszRDNAttr = 0;

            // Get this name string
            cszRDNAttr = CertRDNValueToStr(pInfo->rgRDN[iRDN].rgRDNAttr[iATTR].dwValueType,
                                           &pInfo->rgRDN[iRDN].rgRDNAttr[iATTR].Value,
                                            NULL,
                                            0);
            if(cszRDNAttr == 0)
            {
                continue;
            }

            wszRDNAttr = AEAlloc(cszRDNAttr * sizeof(TCHAR));
            if(wszRDNAttr == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Ret;
            }
            cszRDNAttr = CertRDNValueToStr(pInfo->rgRDN[iRDN].rgRDNAttr[iATTR].dwValueType,
                                           &pInfo->rgRDN[iRDN].rgRDNAttr[iATTR].Value,
                                            wszRDNAttr,
                                            cszRDNAttr);
            if(cszRDNAttr == 0)
            {
                // We couldn't convert the name for some reason
                AEFree(wszRDNAttr);
                continue;
            }

            if(strcmp(szOID_COMMON_NAME, pInfo->rgRDN[iRDN].rgRDNAttr[iATTR].pszObjId) == 0)
            {
                // If there are published UPN's, then
                // we should check against those for a match,
                // otherwise, we check against the generated UPN
                if((pInstance->pInternalInfo->awszldap_UPN != NULL) &&
                   (*pInstance->pInternalInfo->awszldap_UPN != NULL))
                {
                    LPTSTR *pwszCurrentName = pInstance->pInternalInfo->awszldap_UPN;
                    while(*pwszCurrentName)
                    {
                        if(_tcscmp(*pwszCurrentName, wszRDNAttr) == 0)
                        {
                            fSubjectUPNMatch = TRUE;
                            break;
                        }
                        pwszCurrentName++;
                    }
                }
                else if(pInstance->pInternalInfo->wszConstructedUPN != NULL)
                {
                    if(_tcscmp(pInstance->pInternalInfo->wszConstructedUPN, wszRDNAttr) == 0)
                    {
                        fSubjectUPNMatch = TRUE;
                    }
                }

            }

            if(strcmp(szOID_RSA_emailAddr, pInfo->rgRDN[iRDN].rgRDNAttr[iATTR].pszObjId) == 0)
            {
                // If there are published e-mails, then
                // we should check against those for a match
                if((pInstance->pInternalInfo->awszEmail != NULL) &&
                   (*pInstance->pInternalInfo->awszEmail != NULL))
                {
                    LPTSTR *pwszCurrentEmail = pInstance->pInternalInfo->awszEmail;
                    while(*pwszCurrentEmail)
                    {
                        if(_tcscmp(*pwszCurrentEmail, wszRDNAttr) == 0)
                        {
                            fSubjectEmailMatch = TRUE;
                            break;
                        }
                        pwszCurrentEmail++;
                    }
                }
                else
                {
                    // We have no e-mail name for this subject, yet there
                    // was one in the cert.
                    fSubjectEmailMatch = FALSE;
                }
            }
            AEFree(wszRDNAttr);
        }
    }



    // Now check the extensions

    for(iExtension = 0; iExtension < pCert->pCertInfo->cExtension; iExtension++)
    {
        if((strcmp(pCert->pCertInfo->rgExtension[iExtension].pszObjId, szOID_SUBJECT_ALT_NAME) == 0) ||
            (strcmp(pCert->pCertInfo->rgExtension[iExtension].pszObjId, szOID_SUBJECT_ALT_NAME2) == 0))
        {
            PCERT_ALT_NAME_INFO pAltName = NULL;
            DWORD               cbAltName = 0;
            DWORD               iAltName;
            // Now, check the AltSubjectName fields.
            if(!CryptDecodeObjectEx(pCert->dwCertEncodingType,
                                X509_ALTERNATE_NAME,
                                pCert->pCertInfo->rgExtension[iExtension].Value.pbData,
                                pCert->pCertInfo->rgExtension[iExtension].Value.cbData,
                                CRYPT_ENCODE_ALLOC_FLAG,
                                NULL,
                                &pAltName,
                                &cbAltName))
            {
                continue;
            }

            for(iAltName = 0; iAltName < pAltName->cAltEntry; iAltName++)
            {
                switch(pAltName->rgAltEntry[iAltName].dwAltNameChoice)
                {
                    case CERT_ALT_NAME_RFC822_NAME:
                        {
                            // If there are published e-mails, then
                            // we should check against those for a match
                            if((pInstance->pInternalInfo->awszEmail != NULL) &&
                               (*pInstance->pInternalInfo->awszEmail != NULL))
                            {
                                LPTSTR *pwszCurrentEmail = pInstance->pInternalInfo->awszEmail;
                                while(*pwszCurrentEmail)
                                {
                                    if(_tcscmp(*pwszCurrentEmail, pAltName->rgAltEntry[iAltName].pwszRfc822Name) == 0)
                                    {
                                        fAltSubjectEmailMatch = TRUE;
                                        break;
                                    }
                                    pwszCurrentEmail++;
                                }
                            }
                            else
                            {
                                fAltSubjectEmailMatch = FALSE;
                            }
                        }
                        break;
                    case CERT_ALT_NAME_DIRECTORY_NAME:
                        {
                            if(CertCompareCertificateName(pCert->dwCertEncodingType,
                                                          &pInstance->pInternalInfo->blobDN,
                                                          &pAltName->rgAltEntry[iAltName].DirectoryName))
                            {
                                fDNMatch = TRUE;
                            }
                        }
                        break;
                    case CERT_ALT_NAME_DNS_NAME:
                        {
                            if(pInstance->pInternalInfo->fMachineEnrollment)
                            {
                                if((pInstance->pInternalInfo->awszldap_UPN != NULL) &&
                                   (*pInstance->pInternalInfo->awszldap_UPN != NULL))
                                {
                                    LPTSTR *pwszCurrentName = pInstance->pInternalInfo->awszldap_UPN;
                                    while(*pwszCurrentName)
                                    {
                                        if(_tcscmp(*pwszCurrentName, pAltName->rgAltEntry[iAltName].pwszDNSName) == 0)
                                        {
                                            fDNSMatch = TRUE;
                                            break;
                                        }
                                        pwszCurrentName++;
                                    }
                                }
                            }
                            else
                            {
                                fDNSMatch = FALSE;
                            }
                        }
                        break;
                    case CERT_ALT_NAME_OTHER_NAME:

                        if(strcmp(pAltName->rgAltEntry[iAltName].pOtherName->pszObjId, 
                            szOID_ALT_NAME_OBJECT_GUID) == 0)
                                  
                        {
                            if(pInstance->dwCertTypeFlags & CT_FLAG_ADD_OBJ_GUID)
                            {
                                // Object ID's should always be the same, so don't compare them
                                // for now.
                                fObjIDMatch = TRUE;
                            }
                            else
                            {
                                // We had an obj-id, but we shouldn't 
                                fObjIDMatch = FALSE;
                            }
                        } else if (strcmp(pAltName->rgAltEntry[iAltName].pOtherName->pszObjId, 
                                    szOID_NT_PRINCIPAL_NAME) == 0)
                        {
                            PCERT_NAME_VALUE    PrincipalNameBlob = NULL;
                            DWORD               PrincipalNameBlobSize = 0;
                            if(CryptDecodeObjectEx(pCert->dwCertEncodingType,
                                                X509_UNICODE_ANY_STRING,
                                                pAltName->rgAltEntry[iAltName].pOtherName->Value.pbData,
                                                pAltName->rgAltEntry[iAltName].pOtherName->Value.cbData,
                                                CRYPT_DECODE_ALLOC_FLAG,
                                                NULL,
                                                (PVOID)&PrincipalNameBlob,
                                                &PrincipalNameBlobSize))
                            {

                                // If there are published UPN's, then
                                // we should check against those for a match,
                                // otherwise, we check against the generated UPN
                                if((pInstance->pInternalInfo->awszldap_UPN != NULL) &&
                                   (*pInstance->pInternalInfo->awszldap_UPN != NULL))
                                {
                                    LPTSTR *pwszCurrentName = pInstance->pInternalInfo->awszldap_UPN;
                                    while(*pwszCurrentName)
                                    {
                                        if(_tcscmp(*pwszCurrentName, 
                                                  (LPWSTR)PrincipalNameBlob->Value.pbData) == 0)
                                        {
                                            fAltSubjectUPNMatch = TRUE;
                                            break;
                                        }
                                        pwszCurrentName++;
                                    }
                                }
                                else if(pInstance->pInternalInfo->wszConstructedUPN != NULL)
                                {
                                    if(_tcscmp(pInstance->pInternalInfo->wszConstructedUPN, 
                                               (LPWSTR)PrincipalNameBlob->Value.pbData) == 0)
                                    {
                                        fAltSubjectUPNMatch = TRUE;
                                    }
                                }
                                LocalFree(PrincipalNameBlob);
                            }

                        }
                        break;
                    default:
                        break;

                }
            }
            LocalFree(pAltName);
        }
    }

    if(((pInstance->pInternalInfo->fMachineEnrollment)?
        ((!fSubjectUPNMatch)||(!fDNSMatch)):
          ((!fSubjectUPNMatch) && (!fAltSubjectUPNMatch))))
    {
        // We didn't find an appropriate UPN in either the subject or alt subject
        DWORD cUPNChars = 0;
        LPWSTR wszUPN = NULL;
        LPTSTR *pwszCurrentName = pInstance->pInternalInfo->awszldap_UPN;
        if(pInstance->pInternalInfo->wszConstructedUPN)
        {
            cUPNChars += wcslen(pInstance->pInternalInfo->wszConstructedUPN)+1;
        }

        while((NULL != pwszCurrentName) && (NULL != *pwszCurrentName))
        {
            cUPNChars += wcslen(*pwszCurrentName++)+1;
        }
        wszUPN = AEAlloc((cUPNChars+1)*sizeof(WCHAR));
        if(wszUPN == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Ret;
        }
        wszUPN[0] = 0;
        if(pInstance->pInternalInfo->wszConstructedUPN)
        {
            wcscat(wszUPN, pInstance->pInternalInfo->wszConstructedUPN);
            wcscat(wszUPN, L",");
        }
        pwszCurrentName = pInstance->pInternalInfo->awszldap_UPN;
        while((NULL != pwszCurrentName) && (NULL != *pwszCurrentName))
        {
            wcscat(wszUPN, *pwszCurrentName++);
            wcscat(wszUPN, L",");
        }

        // Kill the last ','
        wszUPN[cUPNChars-1] = 0;


        AELogTestResult(ppAEData,
                        pInstance->pInternalInfo->fMachineEnrollment?AE_TEST_NAME_SUBJECT_DNS:AE_TEST_NAME_UPN,
                        wszUPN);
        AEFree(wszUPN);
        fDisplaySubjectName = TRUE;
        fDisplayAltSubjectName = TRUE;
    }

    if((pInstance->dwCertTypeFlags & CT_FLAG_ADD_EMAIL) && 
       ((!fSubjectEmailMatch) || 
       (!fAltSubjectEmailMatch)))
    {
        // We didn't find an appropriate UPN in either the subject or alt subject
        DWORD cEmailChars = 0;
        LPWSTR wszEmail = NULL;
        LPTSTR *pwszCurrentEmail = pInstance->pInternalInfo->awszEmail;
        while((NULL != pwszCurrentEmail) && (NULL != *pwszCurrentEmail))
        {
            cEmailChars += wcslen(*pwszCurrentEmail++)+1;
        }
        wszEmail = AEAlloc((cEmailChars+1)*sizeof(WCHAR));
        if(wszEmail == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Ret;
        }
        wszEmail[0] = 0;

        pwszCurrentEmail = pInstance->pInternalInfo->awszEmail;
        while((NULL != pwszCurrentEmail) && (NULL != *pwszCurrentEmail))
        {
            wcscat(wszEmail, *pwszCurrentEmail++);
            wcscat(wszEmail, L",");
        }
        // Kill the last ','
        wszEmail[cEmailChars-1] = 0;

        if(!fSubjectEmailMatch && fAltSubjectEmailMatch)
        {
            AELogTestResult(ppAEData,
                        AE_TEST_NAME_SUBJECT_EMAIL,
                        wszEmail);
        }
        else if(!fAltSubjectEmailMatch && fSubjectEmailMatch)
        {
            AELogTestResult(ppAEData,
                        AE_TEST_NAME_ALT_SUBJECT_EMAIL,
                        wszEmail);
        }
        else if((!fAltSubjectEmailMatch) && (!fSubjectEmailMatch))
        {
            AELogTestResult(ppAEData,
                        AE_TEST_NAME_BOTH_SUBJECT_EMAIL,
                        wszEmail);
        }
        
        AEFree(wszEmail);
        if(!fSubjectEmailMatch)
        {
            fDisplaySubjectName = TRUE;
        }
        if(!fAltSubjectEmailMatch)
        {
            fDisplayAltSubjectName = TRUE;
        }

    }

    if((pInstance->dwCertTypeFlags & CT_FLAG_ADD_DIRECTORY_PATH) && 
       (!fDNMatch))
    {
        AELogTestResult(ppAEData,
                    AE_TEST_NAME_DIRECTORY_NAME,
                    pInstance->pInternalInfo->wszDN);
        fDisplayAltSubjectName = TRUE;


    }

    if(!fObjIDMatch)
    {

        AELogTestResult(ppAEData,
            (pInstance->dwCertTypeFlags & CT_FLAG_ADD_OBJ_GUID)?AE_TEST_NAME_NO_OBJID:AE_TEST_NAME_OBJID);
        fDisplayAltSubjectName = TRUE;
    }

    if(fDisplaySubjectName)
    {

        DWORD cNameStr = 0;
        LPWSTR wszNameStr = NULL;
        cNameStr = CertNameToStr(X509_ASN_ENCODING,
                      &pCert->pCertInfo->Subject,
                      CERT_X500_NAME_STR,
                      NULL,
                      0);

        if(cNameStr)
        {
            wszNameStr = (LPWSTR)AEAlloc(cNameStr*sizeof(WCHAR));
            if(wszNameStr == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Ret;
            }

            cNameStr = CertNameToStr(X509_ASN_ENCODING,
                          &pCert->pCertInfo->Subject,
                          CERT_X500_NAME_STR,
                          wszNameStr,
                          cNameStr);




            AELogTestResult(ppAEData,
                        AT_TEST_SUBJECT_NAME,
                        wszNameStr);
            AEFree(wszNameStr);
        }

    }

    if(fDisplayAltSubjectName)
    {
        DWORD cbFormat = 0;
        LPWSTR wszFormat = NULL;
        for(iExtension = 0; iExtension < pCert->pCertInfo->cExtension; iExtension++)
        {
            if((strcmp(pCert->pCertInfo->rgExtension[iExtension].pszObjId, szOID_SUBJECT_ALT_NAME) == 0) ||
                (strcmp(pCert->pCertInfo->rgExtension[iExtension].pszObjId, szOID_SUBJECT_ALT_NAME2) == 0))
            {


                wszFormat = HelperExtensionToString(&pCert->pCertInfo->rgExtension[iExtension]);

                if(wszFormat == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Ret;
                }


                AELogTestResult(ppAEData,
                        AT_TEST_ALT_SUBJECT_NAME,
                        wszFormat);
                LocalFree(wszFormat);
            }
        }

    }
Ret:
    if(pInfo)
    {
        LocalFree(pInfo);
    }

    return hr;
}


//
// Name:    VerifyCertificateChaining
//
// Description: This function checks if if the certificate has expired, or has been revoked
//
HRESULT VerifyCertificateChaining(
                     IN PAE_INSTANCE_INFO           pInstance,
                     IN PCCERT_CONTEXT              pCert,
                     IN OUT PAE_CERT_TEST_ARRAY  *  ppAEData
                     ) 
{

    HRESULT hr = S_OK;
    HRESULT hrChainStatus = S_OK;

    CERT_CHAIN_PARA             ChainParams;
    CERT_CHAIN_POLICY_PARA      ChainPolicy;
    CERT_CHAIN_POLICY_STATUS    PolicyStatus;
    PCCERT_CHAIN_CONTEXT        pChainContext = NULL;
    PCTL_INFO                   pCTLInfo = NULL;
    LARGE_INTEGER               ftTime;


    AE_BEGIN(L"VerifyCertificateChaining");

    if(*ppAEData)
    {
        (*ppAEData)->fRenewalOK = FALSE;
    }

    pCTLInfo = pInstance->pCTLContext->pCtlInfo;
    // Build the certificate chain for trust
    // operations
    ChainParams.cbSize = sizeof(ChainParams);
    ChainParams.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;

    ChainParams.RequestedUsage.Usage.cUsageIdentifier = 0;
    ChainParams.RequestedUsage.Usage.rgpszUsageIdentifier = NULL;

    ChainPolicy.cbSize = sizeof(ChainPolicy);
    ChainPolicy.dwFlags = 0;  // ignore nothing
    ChainPolicy.pvExtraPolicyPara = NULL;

    PolicyStatus.cbSize = sizeof(PolicyStatus);
    PolicyStatus.dwError = 0;
    PolicyStatus.lChainIndex = -1;
    PolicyStatus.lElementIndex = -1;
    PolicyStatus.pvExtraPolicyStatus = NULL;

    // Build a small time skew into the chain building in order to deal
    // with servers that may skew slightly fast.
    GetSystemTimeAsFileTime((LPFILETIME)&ftTime);
    ftTime.QuadPart += Int32x32To64(FILETIME_TICKS_PER_SECOND, DEFAULT_AUTOENROLL_SKEW);

    // Build a cert chain for the current status of the cert..
    if(!CertGetCertificateChain(pInstance->pInternalInfo->fMachineEnrollment?HCCE_LOCAL_MACHINE:HCCE_CURRENT_USER,
                                pCert,
                                (LPFILETIME)&ftTime,
                                NULL,
                                &ChainParams,
                                CERT_CHAIN_REVOCATION_CHECK_END_CERT |
                                CERT_CHAIN_REVOCATION_CHECK_CHAIN,
                                NULL,
                                &pChainContext))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_WARNING, L"Could not build certificate chain (%lx)\n\r", hr));

        goto Ret;
    }
    
    if(!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_BASE,
                                          pChainContext,
                                          &ChainPolicy,
                                          &PolicyStatus))
    {
        hrChainStatus = HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_WARNING, L"Base Chain Policy failed (%lx) - must get new cert\n\r", PolicyStatus.dwError));
    }
    else
    {
        hrChainStatus = PolicyStatus.dwError;
    }
    if((S_OK ==  hrChainStatus) ||
       (CRYPT_E_NO_REVOCATION_CHECK ==  hrChainStatus) ||
       (CRYPT_E_REVOCATION_OFFLINE ==  hrChainStatus))
    {
        // The cert is still currently acceptable by trust standards,
        // so we can renew it.

        if(NULL == (*ppAEData))
        {
            (*ppAEData) = (PAE_CERT_TEST_ARRAY)LocalAlloc(LMEM_FIXED, sizeof(AE_CERT_TEST_ARRAY) + 
                                            (AE_CERT_TEST_SIZE_INCREMENT - ANYSIZE_ARRAY)*sizeof(AE_CERT_TEST));
            if((*ppAEData) == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Ret;

            }
            (*ppAEData)->dwVersion = AE_CERT_TEST_ARRAY_VERSION;
            (*ppAEData)->cTests = 0;
            (*ppAEData)->cMaxTests = AE_CERT_TEST_SIZE_INCREMENT;
        }
        (*ppAEData)->fRenewalOK = TRUE;
        hrChainStatus = S_OK;
    }
    else
    {
        LPWSTR wszChainStatus = NULL;
        if(0 == FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                hrChainStatus,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (WCHAR *) &wszChainStatus,
                0,
                NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Ret;
        }
    
        // The cert has expired or has been revoked or something,
        // we must re-enroll
        AELogTestResult(ppAEData,
                    AE_TEST_CHAIN_FAIL,
                    hrChainStatus,
                    wszChainStatus);
        AEFree(wszChainStatus);
        
    }
    // Verify that the immediate CA of the cert exists in the
    // Autoenrollment Object.  Empty ctl's imply any CA is
    // ok.
    if(pCTLInfo->cCTLEntry)
    {
        DWORD i;
        BYTE  pHash[20];
        DWORD cbHash;
        cbHash = sizeof(pHash);
        AE_DEBUG((AE_TRACE, L"Verifying Issuer presence in auto-enrollment object\n\r"));


        if((pChainContext == NULL) ||
            (pChainContext->rgpChain == NULL) ||
            (pChainContext->cChain < 1) ||
            (pChainContext->rgpChain[0]->rgpElement == NULL) ||
            (pChainContext->rgpChain[0]->cElement < 2))
        {
            hr = E_POINTER;
            goto Ret;
        }

        if(!CertGetCertificateContextProperty(pChainContext->rgpChain[0]->rgpElement[1]->pCertContext,
                                          CERT_SHA1_HASH_PROP_ID,
                                          pHash,
                                          &cbHash))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            AE_DEBUG((AE_ERROR, L"Could not get certificate Hash (%lx)\n\r",hr));
            goto Ret;
        }

        for(i=0; i < pCTLInfo->cCTLEntry; i++)
        {
            if(pCTLInfo->rgCTLEntry[i].SubjectIdentifier.pbData == NULL)
                continue;

            if(pCTLInfo->rgCTLEntry[i].SubjectIdentifier.cbData != cbHash)
                continue;

            if(memcmp(pCTLInfo->rgCTLEntry[i].SubjectIdentifier.pbData,
                      pHash,
                      cbHash) == 0)
            {
                break;
            }
        }
        if(i == pCTLInfo->cCTLEntry)
        {
            AE_DEBUG((AE_WARNING, L"Issuer not in auto-enrollment list - must renew\n\r"));

            AELogTestResult(ppAEData,
                            AE_TEST_ISSUER_FAIL);
        }
    }


    if(pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
        pChainContext = NULL;
    }


    // only check expiration status if the cert is otherwise ok
    if(hrChainStatus == S_OK)
    {

        // Nudge the evaluation of the cert chain by the expiration
        // offset so we know if is expired by that time in the future.
        GetSystemTimeAsFileTime((LPFILETIME)&ftTime);
        // Build the certificate chain for trust
        // operations
        ChainParams.cbSize = sizeof(ChainParams);
        ChainParams.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;

        ChainParams.RequestedUsage.Usage.cUsageIdentifier = 0;
        ChainParams.RequestedUsage.Usage.rgpszUsageIdentifier = NULL;

        if(pInstance->ftExpirationOffset.QuadPart < 0)
        {
            LARGE_INTEGER ftHalfLife;
            ftHalfLife.QuadPart = (((LARGE_INTEGER *)&pCert->pCertInfo->NotAfter)->QuadPart - 
                                   ((LARGE_INTEGER *)&pCert->pCertInfo->NotBefore)->QuadPart)/2;
    

            if(ftHalfLife.QuadPart > (- pInstance->ftExpirationOffset.QuadPart))
            {
                // Assume that the old cert is not time nesting invalid
                ftTime.QuadPart -= pInstance->ftExpirationOffset.QuadPart;
            }
            else
            {
                ftTime.QuadPart += ftHalfLife.QuadPart;
            }
        }
        else
        {
            ftTime = pInstance->ftExpirationOffset;
        }

        // Is this a renewal, or an enroll on behalf of...
        if(!CertGetCertificateChain(pInstance->pInternalInfo->fMachineEnrollment?HCCE_LOCAL_MACHINE:HCCE_CURRENT_USER,
                                    pCert,
                                    (LPFILETIME)&ftTime,
                                    NULL,
                                    &ChainParams,
                                    0,
                                    NULL,
                                    &pChainContext))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            AE_DEBUG((AE_WARNING, L"Could not build certificate chain (%lx)\n\r", hr));

            goto Ret;
        }

        // Verify revocation and expiration of the certificate
        ChainPolicy.cbSize = sizeof(ChainPolicy);
        ChainPolicy.dwFlags = 0;  // ignore nothing
        ChainPolicy.pvExtraPolicyPara = NULL;

        PolicyStatus.cbSize = sizeof(PolicyStatus);
        PolicyStatus.dwError = 0;
        PolicyStatus.lChainIndex = -1;
        PolicyStatus.lElementIndex = -1;
        PolicyStatus.pvExtraPolicyStatus = NULL;

        if(!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_BASE,
                                              pChainContext,
                                              &ChainPolicy,
                                              &PolicyStatus))
        {
            hrChainStatus = HRESULT_FROM_WIN32(GetLastError());
            AE_DEBUG((AE_WARNING, L"Base Chain Policy failed (%lx) - must get new cert\n\r", hr));
        }
        else
        {
            hrChainStatus = PolicyStatus.dwError;
        }

        if((S_OK != hrChainStatus) &&
           (CRYPT_E_NO_REVOCATION_CHECK != hrChainStatus) &&
           (CRYPT_E_REVOCATION_OFFLINE != hrChainStatus))
        {
            // The cert has expired or has been revoked or something,
            // we must re-enroll
            AELogTestResult(ppAEData,
                            AT_TEST_PENDING_EXPIRATION);
        }
    }

Ret:
    if(pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
    }

    AE_END();
    
    return hr;    
}

HRESULT VerifyAutoenrolledCertificate(
                     IN PAE_INSTANCE_INFO           pInstance,
                     IN PCCERT_CONTEXT              pCert,
                     IN OUT PAE_CERT_TEST_ARRAY  *  ppAEData
                     )
{
    HRESULT hr = S_OK;

    hr = VerifyCommonExtensions(pInstance, pCert, ppAEData);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = VerifyCertificateNaming(pInstance, pCert, ppAEData);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = VerifyCertificateChaining(pInstance, pCert, ppAEData);


    return hr;
}


//
// Name:    IsOldCertificateValid
//
// Description: This function checks if the certificate in the pOldCert
//              member is valid to satisfy this auto-enrollment request,
//              or if it should be renewed. 
//
HRESULT IsOldCertificateValid(
                     IN PAE_INSTANCE_INFO pInstance,
                     OUT BOOL *pfNeedNewCert
                     ) 
{

    HRESULT hr = S_OK;

    PAE_CERT_TEST_ARRAY         pAEData = NULL;


    AE_BEGIN(L"IsOldCertificateValid");

    *pfNeedNewCert = TRUE;

    if((pInstance == NULL) ||
       (pInstance->pCTLContext == NULL) ||
       (pInstance->pCTLContext->pCtlInfo == NULL))
    {
        goto Ret;
    }
    pInstance->fRenewalOK = FALSE;


    hr =  VerifyAutoenrolledCertificate(pInstance,
                                        pInstance->pOldCert,
                                        &pAEData);

    if(FAILED(hr))
    {
        goto Ret;
    }

    if(pAEData)
    {
        pInstance->fRenewalOK = pAEData->fRenewalOK;
    }

    // Scan the verification results
    if((pAEData) && (pAEData->cTests > 0))
    {
        BOOL  fFailed = FALSE;
        DWORD cFailureMessage= 0;
        DWORD iFailure;
        LPWSTR wszFailureMessage = NULL;
        LPWSTR wszCurrent;
        DWORD  cTestArray = 0;
        DWORD  cbTestArray = 0;
        DWORD  * aidTestArray = NULL;
        DWORD  iTest;

        if(CertGetCertificateContextProperty(pInstance->pOldCert,
                                          AE_CERT_TEST_ARRAY_PROPID,
                                          NULL,
                                          &cbTestArray))
        {
            aidTestArray = (DWORD *)AEAlloc(cbTestArray);
            CertGetCertificateContextProperty(pInstance->pOldCert,
                                                      AE_CERT_TEST_ARRAY_PROPID,
                                                      aidTestArray,
                                                      &cbTestArray);
            cTestArray = cbTestArray/sizeof(aidTestArray[0]);
        }




        // Check to see of these are ignored failures.

        for(iFailure=0; iFailure < pAEData->cTests; iFailure++)
        {

            if(FAILED(pAEData->Test[iFailure].idTest))
            {
                // Are we ignoring this test?
                for(iTest = 0; iTest < cTestArray; iTest++)
                {
                    if(aidTestArray[iTest] == pAEData->Test[iFailure].idTest)
                        break;
                }
                if(iTest != cTestArray)
                {
                    if(pAEData->Test[iFailure].pwszReason)
                    {
                        LocalFree(pAEData->Test[iFailure].pwszReason);
                    }
                    pAEData->Test[iFailure].pwszReason = NULL;
                    pAEData->Test[iFailure].idTest = S_OK;

                    continue;
                }
                fFailed = TRUE;
            }
            if(pAEData->Test[iFailure].pwszReason)
            {
                cFailureMessage += wcslen(pAEData->Test[iFailure].pwszReason);
            }
        }
        cFailureMessage += 5;
        if(aidTestArray)
        {
            AEFree(aidTestArray);
        }

        if(fFailed)
        {
            wszFailureMessage = (LPWSTR)AEAlloc(cFailureMessage*sizeof(WCHAR));
            if(wszFailureMessage == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Ret;
            }

            wcscpy(wszFailureMessage, L"\n\r\n\r");
            wszCurrent = wszFailureMessage+2; 

            for(iFailure=0; iFailure < pAEData->cTests; iFailure++)
            {
                if(pAEData->Test[iFailure].pwszReason)
                {
                    wcscpy(wszCurrent, pAEData->Test[iFailure].pwszReason);
                    wszCurrent += wcslen(pAEData->Test[iFailure].pwszReason);
                }
            }

            LogAutoEnrollmentEvent(pInstance->fRenewalOK?
                                    EVENT_OLD_CERT_VERIFY_RENEW_WARNING:
                                    EVENT_OLD_CERT_VERIFY_REENROLL_WARNING,
                                    pInstance->pInternalInfo->hToken,
                                    pInstance->pwszCertType,
                                    wszFailureMessage,
                                    NULL);
            AEFree(wszFailureMessage);
            // Report the event here
            goto Ret;
        }
    }





    *pfNeedNewCert = FALSE;

Ret:

    if(pAEData)
    {

        AEFreeTestResult(&pAEData);
    }

    if(hr != S_OK)
    {
        LogAutoEnrollmentError(hr,
                               EVENT_UAE_VERIFICATION_FAILURE,
                               pInstance->pInternalInfo->fMachineEnrollment,
                               pInstance->pInternalInfo->hToken,
                               pInstance->pwszCertType, NULL);
    }

    AE_END();


    
    return hr;    
}


//
// Name:    IsOldCertificateValid
//
// Description: This function checks if the certificate in the pOldCert
//              member is valid to satisfy this auto-enrollment request,
//              or if it should be renewed. 
//
HRESULT VerifyEnrolledCertificate(
                     IN PAE_INSTANCE_INFO pInstance,
                     IN PCCERT_CONTEXT    pCert
                     ) 
{

    HRESULT hr = S_OK;

    PAE_CERT_TEST_ARRAY         pAEData = NULL;
    CRYPT_DATA_BLOB AETestArray = {0, NULL};

    AE_BEGIN(L"IsOldCertificateValid");
    AETestArray.cbData = 0;
    AETestArray.pbData = NULL;


    if((pInstance == NULL) ||
       (pInstance->pCTLContext == NULL) ||
       (pInstance->pCTLContext->pCtlInfo == NULL))
    {
        goto Ret;
    }
    pInstance->fRenewalOK = FALSE;


    hr =  VerifyAutoenrolledCertificate(pInstance,
                                        pCert,
                                        &pAEData);

    if(FAILED(hr))
    {
        goto Ret;
    }


    // Scan the verification results
    if((pAEData) && (pAEData->cTests > 0))
    {
        BOOL  fFailed = FALSE;
        DWORD cFailureMessage= 0;
        DWORD iFailure;
        LPWSTR wszFailureMessage = NULL;
        LPWSTR wszCurrent;
        DWORD  cTestArray = 0;
        DWORD  iTestArray = 0;

        cTestArray = pAEData->cTests;
        AETestArray.cbData = cTestArray*sizeof(DWORD);

        if(AETestArray.cbData)
        {
            AETestArray.pbData = AEAlloc(AETestArray.cbData);
        }
        else
        {
            AETestArray.pbData = NULL;
        }



        // Check to see of these are ignored failures.

        for(iFailure=0; iFailure < pAEData->cTests; iFailure++)
        {

            if(FAILED(pAEData->Test[iFailure].idTest))
            {
                fFailed = TRUE;
                if(AETestArray.pbData)
                {
                    ((DWORD *)AETestArray.pbData)[iTestArray++] = pAEData->Test[iFailure].idTest;
                }
            }
            if(pAEData->Test[iFailure].pwszReason)
            {
                cFailureMessage += wcslen(pAEData->Test[iFailure].pwszReason);
            }
        }
        cFailureMessage += 5;

        wszFailureMessage = (LPWSTR)AEAlloc(cFailureMessage*sizeof(WCHAR));
        if(wszFailureMessage == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Ret;
        }

        wcscpy(wszFailureMessage, L"\n\r\n\r");
        wszCurrent = wszFailureMessage+2; 

        for(iFailure=0; iFailure < pAEData->cTests; iFailure++)
        {
            if(pAEData->Test[iFailure].pwszReason)
            {
                wcscpy(wszCurrent, pAEData->Test[iFailure].pwszReason);
                wszCurrent += wcslen(pAEData->Test[iFailure].pwszReason);
            }
        }

        LogAutoEnrollmentEvent(EVENT_ENROLLED_CERT_VERIFY_WARNING,
                                pInstance->pInternalInfo->hToken,
                                pInstance->pwszCertType,
                                wszFailureMessage,
                                NULL);
        AEFree(wszFailureMessage);

        // Report the event here
    }


    CertSetCertificateContextProperty(pCert,
                                      AE_CERT_TEST_ARRAY_PROPID,
                                      0,
                                      AETestArray.pbData?&AETestArray:NULL);



Ret:

    if(pAEData)
    {

        AEFreeTestResult(&pAEData);
    }

    if(hr != S_OK)
    {
        LogAutoEnrollmentError(hr,
                               EVENT_UAE_VERIFICATION_FAILURE,
                               pInstance->pInternalInfo->fMachineEnrollment,
                               pInstance->pInternalInfo->hToken,
                               pInstance->pwszCertType, NULL);
    }

    AE_END();
    
    return hr;    
}

//
// Name:    FindExistingEnrolledCertificate
//
// Description: This function searches for an existing certificate
//              enrolled with this auto-enrollment object.
//
  
BOOL FindExistingEnrolledCertificate(IN PAE_INSTANCE_INFO pInstance,
                                     OUT PCCERT_CONTEXT  *ppCert)
{
    PCCERT_CONTEXT  pCertContext = NULL;
    PCCERT_CONTEXT  pPrevContext = NULL;
    DWORD           i;
    BOOL            fRet = FALSE;

    DWORD           dwEnrollPropId = CERT_AUTO_ENROLL_PROP_ID;


    LPWSTR          wszEnrollmentId = NULL;
    DWORD           cbEnrollmentId = 0;
    DWORD           cbCurrentId=0;

    AE_BEGIN(L"FindExistingEnrolledCertificate");


    if(pInstance->pwszCertType == NULL)
    {
        return FALSE;
    }

    cbEnrollmentId = sizeof(WCHAR) * (wcslen(pInstance->pwszAEIdentifier) + 1);
    wszEnrollmentId = (WCHAR *)AEAlloc(cbEnrollmentId);
    if(wszEnrollmentId == NULL)
    {
        return FALSE;
    }

    if(*ppCert)
    {
        CertFreeCertificateContext(*ppCert);
    }
    *ppCert = NULL;

        // check if cert from CA is in the MY store
     while(pCertContext = CertFindCertificateInStore(
                pInstance->pInternalInfo->hMYStore,
                X509_ASN_ENCODING, 
                0, 
                CERT_FIND_PROPERTY,
                &dwEnrollPropId, 
                pPrevContext))
     {

        pPrevContext = pCertContext;

        // check if this is an auto enroll cert and
        // if the cert type is correct

        cbCurrentId = cbEnrollmentId;
        if(!CertGetCertificateContextProperty(pCertContext,
                                              CERT_AUTO_ENROLL_PROP_ID, 
                                              wszEnrollmentId, 
                                              &cbCurrentId))
        {
            continue;
        }

        if(wcscmp(wszEnrollmentId, pInstance->pwszAEIdentifier) != 0)
        {
            continue;
        }
        AE_DEBUG((AE_INFO, L"Found auto-enrolled certificate for %ls cert\n\r", pInstance->pwszCertType));

        *ppCert = pCertContext;
        pCertContext = NULL;
        break;

    }
    fRet = TRUE; 

    if (pCertContext)
        CertFreeCertificateContext(pCertContext);
    if(wszEnrollmentId)
    {
        AEFree(wszEnrollmentId);
    }
    AE_END();
    return fRet;
}



//
// Name:        
//
// Description: This function calls a function to determine if an auto
//              enrollment is to occur and if so it trys to enroll with
//              different CAs until either an enrollment is successful or
//              the list of CAs is exhausted.
//
  
void EnrollmentWithCTL(
                       IN PAE_INSTANCE_INFO pInstance,
                       IN LDAP *            pld
                       )
{
    INTERNAL_INFO       pInternalInfo;
    BOOL                fNeedToEnroll = FALSE;
    AUTO_ENROLL_INFO    EnrollmentInfo;
    DWORD               *pdwCAEntries = NULL;
    DWORD               i;
    BOOL                fPermitted;
    BOOL                fAnyAcceptableCAs = FALSE;
    DWORD               dwFailureCode = E_FAIL;


    AE_BEGIN(L"EnrollmentWithCTL");

    if(!GetCertTypeInfo(pInstance,
                      pld,
                     &fPermitted))
    {
        goto Ret;
    }
    if(!fPermitted)
    {
        AE_DEBUG((AE_INFO, L"Not permitted to enroll for %ls cert type\n", pInstance->pwszCertType));

        goto Ret;
    }

    if(!FindExistingEnrolledCertificate(pInstance, &pInstance->pOldCert))
    {
        goto Ret;
    }

    if(pInstance->pOldCert)
    {
        if(FAILED(IsOldCertificateValid(pInstance, &fNeedToEnroll)))
        {
            goto Ret;
        }
        if(!fNeedToEnroll)
        {
            goto Ret;
        }
        if(!pInstance->fRenewalOK)
        {
            CRYPT_DATA_BLOB Archived;
            Archived.cbData = 0;
            Archived.pbData = NULL;

            // We force an archive on the old cert and close it.
            CertSetCertificateContextProperty(pInstance->pOldCert,
                                              CERT_ARCHIVED_PROP_ID,
                                              0,
                                              &Archived);
        }
    }

    // It looks like we need to enroll
    // for a cert.  


    do
    {
        // Loop through all avaialble ca's to find one
        // that supports this cert type, and is in our CTL

        for (i=0;i<pInstance->pInternalInfo->ccaList;i++)
        {
            DWORD dwIndex;
            LPWSTR *pwszCertType;
            DWORD iCTL;
            dwIndex = (i + pInstance->dwRandomIndex) % pInstance->pInternalInfo->ccaList;
            AE_DEBUG((AE_TRACE, L"Trying CA %s\\%s\n\r", pInstance->pInternalInfo->acaList[dwIndex].wszDNSName, pInstance->pInternalInfo->acaList[dwIndex].wszName));

            // Does this CA support our cert type.
            pwszCertType = pInstance->pInternalInfo->acaList[dwIndex].awszCertificateTemplates;
            if(pwszCertType == NULL)
            {
                AE_DEBUG((AE_TRACE, L"There are no cert types supported on this CA\n\r"));
                continue;
            }
            while(*pwszCertType)
            {
                if(wcscmp(*pwszCertType, pInstance->pwszCertType) == 0)
                {
                    break;
                }
                pwszCertType++;
            }
            if(*pwszCertType == NULL)
            {
                AE_DEBUG((AE_TRACE, L"The cert type %s is not supported on this CA\n\r", pInstance->pwszCertType));
                continue;
            }

            // Is this CA in our CTL List
            if(pInstance->pCTLContext->pCtlInfo->cCTLEntry > 0)
            {
                for(iCTL = 0; iCTL < pInstance->pCTLContext->pCtlInfo->cCTLEntry; iCTL++)
                {
                    PCTL_ENTRY pEntry= &pInstance->pCTLContext->pCtlInfo->rgCTLEntry[iCTL];

                    if(pEntry->SubjectIdentifier.cbData != sizeof(pInstance->pInternalInfo->acaList[dwIndex].CACertHash))
                    {
                        continue;
                    }

                    if(memcmp(pEntry->SubjectIdentifier.pbData, 
                              pInstance->pInternalInfo->acaList[dwIndex].CACertHash, 
                              pEntry->SubjectIdentifier.cbData) == 0)
                    {
                        break;
                    }         
                }
                if(iCTL == pInstance->pCTLContext->pCtlInfo->cCTLEntry)
                {
                    AE_DEBUG((AE_TRACE, L"The CA is not supported by the auto-enrollment object\n\r"));
                    continue;
                }
            }

            ZeroMemory(&EnrollmentInfo, sizeof(EnrollmentInfo));

            // Yes, we may enroll at this CA!
            fAnyAcceptableCAs = TRUE;

            EnrollmentInfo.pwszCAMachine = pInstance->pInternalInfo->acaList[dwIndex].wszDNSName;
            EnrollmentInfo.pwszCAAuthority = pInstance->pInternalInfo->acaList[dwIndex].wszName;
            EnrollmentInfo.pszAutoEnrollProvider = DEFAULT_AUTO_ENROLL_PROV;
            EnrollmentInfo.fRenewal = pInstance->fRenewalOK && (pInstance->pOldCert != NULL);



            if(!SetEnrollmentCertType(pInstance, &EnrollmentInfo))
            {
                AE_DEBUG((AE_TRACE, L"SetEnrollmentCertType failed\n\r"));
                continue;
            }
        

            // load the provider and call the entry point
            if (LoadAndCallEnrollmentProvider(pInstance->pInternalInfo->fMachineEnrollment,
                                              &EnrollmentInfo))
            {
                PCCERT_CONTEXT pNewCert = NULL;
                // Succeeded, 
                // verify the cert that was retrieved
                if(!FindExistingEnrolledCertificate(pInstance, &pNewCert))
                {
                    continue;
                }
                
                VerifyEnrolledCertificate(pInstance, pNewCert);

                CertFreeCertificateContext(pNewCert);

                break;
            }
        }

        if(i == pInstance->pInternalInfo->ccaList)
        {
            // 
            // If we have a preexisting cert, then we may need to try twice, first
            // to renew, and then to re-enroll

            if(pInstance->pOldCert)
            {

                // Try again, but re-enrolling this time
                CRYPT_DATA_BLOB Archived;
                Archived.cbData = 0;
                Archived.pbData = NULL;

                // We force an archive on the old cert and close it.
                CertSetCertificateContextProperty(pInstance->pOldCert,
                                                  CERT_ARCHIVED_PROP_ID,
                                                  0,
                                                  &Archived);
                CertFreeCertificateContext(pInstance->pOldCert);
                pInstance->pOldCert = NULL;
                pInstance->fRenewalOK = FALSE;
                continue;
            }


            AE_DEBUG((AE_WARNING, L"Auto-enrollment not performed\n\r"));
            break;
            // failed
        }
        else
        {
            break;
        }

    } while (TRUE);
Ret:
    AE_END();
    return;
}

#define SHA1_HASH_LENGTH    20

PCCERT_CONTEXT FindCertificateInOtherStore(
    IN HCERTSTORE hOtherStore,
    IN PCCERT_CONTEXT pCert
    )
{
    BYTE rgbHash[SHA1_HASH_LENGTH];
    CRYPT_DATA_BLOB HashBlob;

    HashBlob.pbData = rgbHash;
    HashBlob.cbData = SHA1_HASH_LENGTH;
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &HashBlob.cbData
            ) || SHA1_HASH_LENGTH != HashBlob.cbData)
        return NULL;

    return CertFindCertificateInStore(
            hOtherStore,
            0,                  // dwCertEncodingType
            0,                  // dwFindFlags
            CERT_FIND_SHA1_HASH,
            (const void *) &HashBlob,
            NULL                //pPrevCertContext
            );
}

//
// Name:    UpdateEnterpriseRoots
//
// Description: This function enumerates all of the roots in the DS based
// enterprise root store, and moves them into the local machine root store.
//

HRESULT WINAPI UpdateEnterpriseRoots(LDAP *pld)
{

    HRESULT hr = S_OK;
    LPWSTR wszLdapRootStore = NULL;
    LPWSTR wszConfig = NULL;
    HCERTSTORE hEnterpriseRoots = NULL,
               hRootStore = NULL;
    PCCERT_CONTEXT pContext = NULL,
                   pOtherCert = NULL;

    static  LPWSTR s_wszEnterpriseRoots =  L"ldap:///CN=Certification Authorities,CN=Public Key Services,CN=Services,%s?cACertificate?one?objectCategory=certificationAuthority";

    hr = myGetConfigDN(pld, &wszConfig);
    if(hr != S_OK)
    {
        goto error;
    }


    wszLdapRootStore = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*(wcslen(wszConfig)+wcslen(s_wszEnterpriseRoots)));
    if(wszLdapRootStore == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    wsprintf(wszLdapRootStore, 
             s_wszEnterpriseRoots,
             wszConfig);

    // BUGBUG:  Make the enterprise root store later
    hRootStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_REGISTRY_W, 
                                0, 
                                0, 
                                CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE, 
                                L"ROOT");
    if(hRootStore == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_ERROR, L"Unable to open ROOT store (%lx)\n\r", hr));
        goto error;
    }

    hEnterpriseRoots = CertOpenStore(CERT_STORE_PROV_LDAP, 
                  0,
                  0,
                  CERT_STORE_READONLY_FLAG,
                  wszLdapRootStore);
    
    if(hEnterpriseRoots == NULL)
    {
        DWORD err = GetLastError();

        // BUGBUG: Checking against NO_SUCH_OBJECT 
        // is a workaround for the fact that 
        // the ldap provider returns LDAP errors
        if((err == LDAP_NO_SUCH_OBJECT) ||
           (err == ERROR_FILE_NOT_FOUND))
        {
            // There was no store, so there are no certs
            hr = S_OK;
            goto error;
        }


        hr = HRESULT_FROM_WIN32(err);

        AE_DEBUG((AE_ERROR, L"Unable to open ROOT store (%lx)\n\r", hr));
        goto error;
    }


    while(pContext = CertEnumCertificatesInStore(hEnterpriseRoots, pContext))
    {
        if (pOtherCert = FindCertificateInOtherStore(hRootStore, pContext)) {
            CertFreeCertificateContext(pOtherCert);
        } 
        else
        {
            CertAddCertificateContextToStore(hRootStore,
                                         pContext,
                                         CERT_STORE_ADD_ALWAYS,
                                         NULL);
        }
    }

    while(pContext = CertEnumCertificatesInStore(hRootStore, pContext))
    {
        if (pOtherCert = FindCertificateInOtherStore(hEnterpriseRoots, pContext)) {
            CertFreeCertificateContext(pOtherCert);
        } 
        else
        {
            CertDeleteCertificateFromStore(CertDuplicateCertificateContext(pContext));
        }
    }


error:

    if(hr != S_OK)
    {
            LogAutoEnrollmentError(hr,
                                   EVENT_UPDATE_ENTERPRISE_ROOT_FAILURE,
                                   TRUE,
                                   NULL,
                                   NULL, NULL);

    }

    if(wszLdapRootStore)
    {
        LocalFree(wszLdapRootStore);
    }

    if(wszConfig)
    {
        LocalFree(wszConfig);
    }
    if(hEnterpriseRoots)
    {
        CertCloseStore(hEnterpriseRoots,0);
    }
    if(hRootStore)
    {
        CertCloseStore(hRootStore,0);
    }

    return hr;
}

//
// Name:    UpdateNTAuthTrust
//
// Description: This function enumerates all of the roots in the DS based
// NTAuth store, and moves them into the local machine NTAuth.
//

HRESULT WINAPI UpdateNTAuthTrust(LDAP *pld)
{

    HRESULT hr = S_OK;
    LPWSTR wszNTAuth = NULL;
    LPWSTR wszConfig = NULL;
    HCERTSTORE hDSAuthRoots = NULL,
               hAuthStore = NULL;
    PCCERT_CONTEXT pContext = NULL,
                   pOtherCert = NULL;

    static  LPWSTR s_wszNTAuthRoots =  L"ldap:///CN=Public Key Services,CN=Services,%s?cACertificate?one?cn=NTAuthCertificates";

    hr = myGetConfigDN(pld, &wszConfig);
    if(hr != S_OK)
    {
        goto error;
    }


    wszNTAuth = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*(wcslen(wszConfig)+wcslen(s_wszNTAuthRoots)));
    if(wszNTAuth == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    wsprintf(wszNTAuth, 
             s_wszNTAuthRoots,
             wszConfig);

    // BUGBUG:  Make the enterprise root store later
    hAuthStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_REGISTRY_W, 
                                0, 
                                0, 
                                CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE, 
                                L"NTAuth");
    if(hAuthStore == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_ERROR, L"Unable to open NTAuth store (%lx)\n\r", hr));
        goto error;
    }

    hDSAuthRoots = CertOpenStore(CERT_STORE_PROV_LDAP, 
                  0,
                  0,
                  CERT_STORE_READONLY_FLAG,
                  wszNTAuth);
    
    if(hDSAuthRoots == NULL)
    {
         DWORD err = GetLastError();
       // BUGBUG: Checking against NO_SUCH_OBJECT 
        // is a workaround for the fact that 
        // the ldap provider returns LDAP errors
        if((err == LDAP_NO_SUCH_OBJECT) ||
           (err == ERROR_FILE_NOT_FOUND))
        {
            // There was no store, so there are no certs
            hr = S_OK;
            goto error;
        }
        
        hr = HRESULT_FROM_WIN32(err);
        AE_DEBUG((AE_ERROR, L"Unable to open ROOT store (%lx)\n\r", hr));
        goto error;
    }


    while(pContext = CertEnumCertificatesInStore(hDSAuthRoots, pContext))
    {
        if (pOtherCert = FindCertificateInOtherStore(hAuthStore, pContext)) {
            CertFreeCertificateContext(pOtherCert);
        } 
        else
        {
            CertAddCertificateContextToStore(hAuthStore,
                                         pContext,
                                         CERT_STORE_ADD_ALWAYS,
                                         NULL);
        }
    }


    while(pContext = CertEnumCertificatesInStore(hAuthStore, pContext))
    {
        if (pOtherCert = FindCertificateInOtherStore(hDSAuthRoots, pContext)) {
            CertFreeCertificateContext(pOtherCert);
        } 
        else
        {
            CertDeleteCertificateFromStore(CertDuplicateCertificateContext(pContext));
        }
    }


error:
    if(hr != S_OK)
    {
            LogAutoEnrollmentError(hr,
                                   EVENT_UPDATE_NTAUTH_FAILURE,
                                   TRUE,
                                   NULL,
                                   NULL, NULL);

    }
    
    if(wszNTAuth)
    {
        LocalFree(wszNTAuth);
    }

    if(wszConfig)
    {
        LocalFree(wszConfig);
    }
    if(hDSAuthRoots)
    {
        CertCloseStore(hDSAuthRoots,0);
    }
    if(hAuthStore)
    {
        CertCloseStore(hAuthStore,0);
    }

    return hr;
}

//
// Name:    ProcessAutoEnrollment
//
// Description: This function retrieves the appropriate auto enrollment
//              objects (CTLs) and then calls a function to proceed with
//              auto enrollment for each of the objects.

//

DWORD WINAPI ProcessAutoEnrollment(
                                        BOOL   fMachineEnrollment,
                                        HANDLE hToken
                                        )
{
    DWORD                       i;
    DWORD                       dwOpenStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG;
    HCERTSTORE                  hACRSStore = 0;
    PCCTL_CONTEXT               pCTLContext = NULL;
    PCCTL_CONTEXT               pPrevCTLContext = NULL;
    CTL_FIND_USAGE_PARA         CTLFindUsage;
    LPSTR                       pszCTLUsageOID;
    CERT_PHYSICAL_STORE_INFO    PhysicalStoreInfo;
    DWORD                       dwRet = 0;
    INTERNAL_INFO               InternalInfo;
    BOOL                        fInitialized = FALSE;
    LDAP                        *pld = NULL;



    __try
    {
        AE_DEBUG((AE_TRACE, L"ProcessAutoEnrollment:%ls\n\r", fMachineEnrollment?L"Machine":L"User"));
        memset(&InternalInfo, 0, sizeof(InternalInfo));
    
        memset(&PhysicalStoreInfo, 0, sizeof(PhysicalStoreInfo));
        memset(&CTLFindUsage, 0, sizeof(CTLFindUsage));
        CTLFindUsage.cbSize = sizeof(CTLFindUsage);

        // since this is a user we need to impersonate the user
        if (!fMachineEnrollment)
        {
            if (hToken)
            {
                if (!ImpersonateLoggedOnUser(hToken))
                {
                    dwRet = GetLastError();
                    AE_DEBUG((AE_ERROR, L"Could not impersonate user: (%lx)\n\r", dwRet));
                    LogAutoEnrollmentError(HRESULT_FROM_WIN32(dwRet),
                                           EVENT_AE_SECURITY_INIT_FAILED,
                                           fMachineEnrollment,
                                           hToken,
                                           NULL, NULL);

                    goto Ret;
                }
            }
        }


        if(fMachineEnrollment)
        {
            dwRet = aeRobustLdapBind(&pld, FALSE);
            if(dwRet != S_OK)
            {
                goto Ret;
            }

            UpdateEnterpriseRoots(pld);
            UpdateNTAuthTrust(pld);
        }



        // if the auto enrollment is for a user then we need to shut off inheritance
        // from the local machine store so that we don't try and enroll for certs
        // which are meant to be for the machine
        if (!fMachineEnrollment)
        {
		    dwOpenStoreFlags = CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG;

            PhysicalStoreInfo.cbSize = sizeof(PhysicalStoreInfo);
            PhysicalStoreInfo.dwFlags = CERT_PHYSICAL_STORE_OPEN_DISABLE_FLAG;

            if (!CertRegisterPhysicalStore(L"ACRS", 
                                           CERT_SYSTEM_STORE_CURRENT_USER,
                                           CERT_PHYSICAL_STORE_LOCAL_MACHINE_NAME, 
                                           &PhysicalStoreInfo,
                                           NULL))
            {
                dwRet = GetLastError();
                AE_DEBUG((AE_ERROR, L"Could not register ACRS store: (%lx)\n\r", dwRet ));
                LogAutoEnrollmentError(HRESULT_FROM_WIN32(dwRet),
                                       EVENT_AE_LOCAL_CYCLE_INIT_FAILED,
                                       fMachineEnrollment,
                                       hToken,
                                       NULL, NULL);
                goto Ret;
            }
        }

        // open the ACRS store and fine the CTL based on the auto enrollment usage
        if (0 == (hACRSStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                              0, 0, dwOpenStoreFlags, L"ACRS")))
        {
            dwRet = GetLastError();
            AE_DEBUG((AE_ERROR, L"Could not open ACRS store: (%lx)\n\r", dwRet ));
            LogAutoEnrollmentError(HRESULT_FROM_WIN32(dwRet),
                                   EVENT_AE_LOCAL_CYCLE_INIT_FAILED,
                                   fMachineEnrollment,
                                   hToken,
                                   NULL, NULL);
            goto Ret;
        }

        // look for the Auto Enrollment usage
        CTLFindUsage.SubjectUsage.cUsageIdentifier = 1;
        pszCTLUsageOID = szOID_AUTO_ENROLL_CTL_USAGE;
        CTLFindUsage.SubjectUsage.rgpszUsageIdentifier = &pszCTLUsageOID;
  
        for (i=0;;i++)
        {
            AE_INSTANCE_INFO Instance;

            memset(&Instance, 0, sizeof(Instance));

            if (NULL == (pCTLContext = CertFindCTLInStore(hACRSStore,
                                                         X509_ASN_ENCODING,
                                                         CTL_FIND_SAME_USAGE_FLAG,
                                                         CTL_FIND_USAGE,
                                                         &CTLFindUsage,
                                                         pPrevCTLContext)))
            {
                // Freed by CertFindCTLInStore.
                pPrevCTLContext = NULL;
                break;
            }
            pPrevCTLContext = pCTLContext;

            if(!fInitialized)
            {
                if(pld == NULL)
                {   dwRet = aeRobustLdapBind(&pld, FALSE);
                    if(dwRet != S_OK)
                    {
                        goto Ret;
                    }
                }
                // Initialize the internal information needed for an auto enrollment
                if (S_OK != (dwRet = InitInternalInfo(pld,
                                                      fMachineEnrollment,
                                                      hToken, 
                                                      &InternalInfo)))
                {
                    break;
                }
                if(InternalInfo.ccaList == 0)
                {
                    // No CA's
                    break;
                }
                fInitialized = TRUE;
            }

            if(!InitInstance(pPrevCTLContext,
                                &InternalInfo,
                                &Instance))
            {
                dwRet = E_FAIL;
                break;
            }
               // UNDONE - perform a WinVerifyTrust check on this auto
            // enrollment object (CTL) to make sure it is trusted

            // have a CTL so do the Enrollment
            EnrollmentWithCTL(&Instance, pld);

            FreeInstance(&Instance);
        }
    }
    __except ( dwRet = GetExceptionError(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER )
    {
        AE_DEBUG((AE_ERROR, L"Exception Caught (%lx)\n\r", dwRet ));
        goto Ret;
    }
Ret:
    __try
    {


        FreeInternalInfo(&InternalInfo);


        if(pPrevCTLContext)
        {
            CertFreeCTLContext(pPrevCTLContext);
        }
        if (hACRSStore)
            CertCloseStore(hACRSStore, 0);
        if (hToken)
        {
            if (!fMachineEnrollment)
            {
                RevertToSelf();
            }
        }
        if(pld != NULL)
        {
            g_pfnldap_unbind(pld);
        }
        AE_END();
    }
    __except ( dwRet = GetExceptionError(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER )
    {

        return dwRet;
    }
    return dwRet;
}



//*************************************************************
//
//  AutoEnrollmentThread()
//
//  Purpose:    Background thread for AutoEnrollment.
//
//  Parameters: pInfo   - AutoEnrollment info
//
//  Return:     0
//
//*************************************************************

VOID AutoEnrollmentThread (PVOID pVoid, BOOLEAN fTimeout)
{
    HINSTANCE hInst;
    HKEY hKey;
    HKEY hCurrent ;
    DWORD dwType, dwSize, dwResult;
    LONG lTimeout;
    LARGE_INTEGER DueTime;
    PAUTO_ENROLL_THREAD_INFO pInfo = pVoid;
    DWORD   dwWaitResult;
    // This is executed in a worker thread, so we need to be safe.

    AE_BEGIN(L"AutoEnrollmentThread");

    if(fTimeout)
    {
        AE_END();
        return ;
    }

    dwWaitResult = WaitForSingleObject(pInfo->fMachineEnrollment?g_hMachineMutex:g_hUserMutex, 0);

    if((dwWaitResult == WAIT_FAILED) ||
        (dwWaitResult == WAIT_TIMEOUT))
    {
        AE_DEBUG((AE_ERROR, L"Mutex Contention\n\r" ));
        AE_END();
        return;
    }

    __try
    {


        // Process the auto-enrollment.
        ProcessAutoEnrollment(
                              pInfo->fMachineEnrollment,
                              pInfo->hToken
                              );
        


        // 
        // Build a timer event to ping us
        // in about 8 hours if we don't get
        // notified.


        lTimeout = AE_DEFAULT_REFRESH_RATE;


        //
        // Query for the refresh timer value
        //
        hCurrent = HKEY_LOCAL_MACHINE ;

        if (pInfo->fMachineEnrollment || NT_SUCCESS( RtlOpenCurrentUser( KEY_READ, &hCurrent ) ) )
        {

            if (RegOpenKeyEx (hCurrent,
                              SYSTEM_POLICIES_KEY,
                              0, KEY_READ, &hKey) == ERROR_SUCCESS) {

                dwSize = sizeof(lTimeout);
                RegQueryValueEx (hKey,
                                 TEXT("AutoEnrollmentRefreshTime"),
                                 NULL,
                                 &dwType,
                                 (LPBYTE) &lTimeout,
                                 &dwSize);

                RegCloseKey (hKey);
            }

            if (!pInfo->fMachineEnrollment)
            {
                RegCloseKey( hCurrent );
            }
        }


        //
        // Limit the timeout to once every 1080 hours (45 days)
        //

        if (lTimeout >= 1080) {
            lTimeout = 1080;
        }

        if (lTimeout < 0) {
            lTimeout = 0;
        }


        //
        // Convert hours to milliseconds
        //

        lTimeout =  lTimeout * 60 * 60 * 1000;


        //
        // Special case 0 milliseconds to be 7 seconds
        //

        if (lTimeout == 0) {
            lTimeout = 7000;
        }


        DueTime.QuadPart = Int32x32To64(-10000, lTimeout);

        if(!SetWaitableTimer (pInfo->hTimer, &DueTime, 0, NULL, 0, FALSE))
        {
            AE_DEBUG((AE_WARNING, L"Could not reset timer (%lx)\n\r", GetLastError()));
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
    }
    ReleaseMutex(pInfo->fMachineEnrollment?g_hMachineMutex:g_hUserMutex);
    AE_END();
    return ;
}

//*************************************************************
//
//  AutoEnrollmentWorker()
//
//  Purpose:    Called by the worker thread mechanism to launch an auto-enrollment thread.
//
//  Parameters: pInfo   - AutoEnrollment info
//
//  Return:     0
//
//*************************************************************

VOID AutoEnrollmentWorker (PVOID pVoid, BOOLEAN fTimeout)
{
    if(fTimeout)
    {
        return ;
    }

}
//+---------------------------------------------------------------------------
//
//  Function:   StartAutoEnrollThread
//
//  Synopsis:   Starts a thread which causes Autoenrollment
//
//  Arguments:  
//              fMachineEnrollment - indicates if enrolling for a machine
//
//  History:    01-11-98   jeffspel   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


HANDLE RegisterAutoEnrollmentProcessing(
                               IN BOOL fMachineEnrollment,
                               IN HANDLE hToken
                               )
{
    DWORD                       dwThreadId;
    HANDLE                      hWait = 0;
    PAUTO_ENROLL_THREAD_INFO    pThreadInfo = NULL;
    TCHAR szEventName[60];
    LARGE_INTEGER DueTime;
    HKEY          hKeySafeBoot = NULL;
    DWORD         dwStatus = ERROR_SUCCESS;
    
    SECURITY_ATTRIBUTES sa = {0,NULL, FALSE};
    AE_DEBUG((AE_TRACE, L"RegisterAutoEnrollmentProcessing:%ls\n\r",fMachineEnrollment?L"Machine":L"User"));
    __try
    {


        //
        // We don't do autoenrollment in safe boot
        //

        // copied from the service controller code
        dwStatus = RegOpenKey(HKEY_LOCAL_MACHINE,
                              L"system\\currentcontrolset\\control\\safeboot\\option",
                              &hKeySafeBoot);

        if (dwStatus == ERROR_SUCCESS) {

            DWORD dwSafeBoot = 0;
            DWORD cbSafeBoot = sizeof(dwSafeBoot);
            //
            // we did in fact boot under safeboot control
            //

            dwStatus = RegQueryValueEx(hKeySafeBoot,
                                       L"OptionValue",
                                       NULL,
                                       NULL,
                                       (LPBYTE)&dwSafeBoot,
                                       &cbSafeBoot);

            if (dwStatus != ERROR_SUCCESS) {
                dwSafeBoot = 0;
            }
            if(dwSafeBoot)
            {
                goto error;
            }
        }




            
        if((g_hInstSecur32 == NULL) ||
            (g_hInstWldap32 == NULL))
        {
            goto error;
        }

        if (NULL == (pThreadInfo = AEAlloc(sizeof(AUTO_ENROLL_THREAD_INFO))))
        {
            goto error;
        }

        ZeroMemory(pThreadInfo, sizeof(AUTO_ENROLL_THREAD_INFO));

        pThreadInfo->fMachineEnrollment = fMachineEnrollment;

        // if this is a user auto enrollment then duplicate the thread token
        if (!pThreadInfo->fMachineEnrollment)
        {
            if (!DuplicateToken(hToken, SecurityImpersonation,
                                &pThreadInfo->hToken))
            {
                AE_DEBUG((AE_ERROR, L"Could not acquire user token: (%lx)\n\r", GetLastError()));
                goto error;
            }

        }



        sa.nLength = sizeof(sa);
        sa.bInheritHandle = FALSE;
        sa.lpSecurityDescriptor = AEMakeGenericSecurityDesc();

        pThreadInfo->hNotifyEvent = CreateEvent(&sa, FALSE, FALSE, fMachineEnrollment?
                                                                     MACHINE_AUTOENROLLMENT_TRIGGER_EVENT:
                                                                     USER_AUTOENROLLMENT_TRIGGER_EVENT);

        if(sa.lpSecurityDescriptor)
        {
            LocalFree(sa.lpSecurityDescriptor);
        }


        if(pThreadInfo->hNotifyEvent == NULL)
        {
            AE_DEBUG((AE_ERROR, L"Could not create GPO Notification Event: (%lx)\n\r", GetLastError()));
            goto error;
        }
        if(!RegisterGPNotification(pThreadInfo->hNotifyEvent,
                                   pThreadInfo->fMachineEnrollment))
        {
            AE_DEBUG((AE_ERROR, L"Could not register for GPO Notification: (%lx)\n\r", GetLastError()));
            goto error;

        }

        if(pThreadInfo->fMachineEnrollment)
        {
            wsprintf (szEventName, TEXT("AUTOENRL: machine refresh timer for %d:%d"),
                  GetCurrentProcessId(), GetCurrentThreadId());
        }
        else
        {
            wsprintf (szEventName, TEXT("AUTOENRL: user refresh timer for %d:%d"),
                  GetCurrentProcessId(), GetCurrentThreadId());
        }
        pThreadInfo->hTimer = CreateWaitableTimer (NULL, FALSE, szEventName);


        if(pThreadInfo->hTimer == NULL)
        {
            goto error;
        }

        if (! RegisterWaitForSingleObject(&pThreadInfo->hNotifyWait,
                                          pThreadInfo->hNotifyEvent, 
                                          AutoEnrollmentThread,
                                          (PVOID)pThreadInfo, 
                                          INFINITE,
                                          0))
        {
            AE_DEBUG((AE_ERROR, L"RegisterWait failed: (%lx)\n\r", GetLastError() ));
            goto error;
        }


         if (! RegisterWaitForSingleObject(&pThreadInfo->hTimerWait,
                     pThreadInfo->hTimer, 
                     AutoEnrollmentThread,
                     (void*)pThreadInfo,
                     INFINITE,
                     0))
        {
            AE_DEBUG((AE_ERROR, L"RegisterWait failed: (%lx)\n\r", GetLastError()));
            goto error;
        }

        // Seed the timer with about 5 minutes, so we can come back
        // and run an auto-enroll later without blocking this thread.

         DueTime.QuadPart = Int32x32To64(-10000,  
                                         (fMachineEnrollment?MACHINE_AUTOENROLL_INITIAL_DELAY:
                                                            USER_AUTOENROLL_INITIAL_DELAY)
                                          * 1000);
        if(!SetWaitableTimer (pThreadInfo->hTimer, &DueTime, 0, NULL, 0, FALSE))
        {
            AE_DEBUG((AE_WARNING, L"Could not reset timer (%lx)\n\r", GetLastError()));
        }
        
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        goto error;
    }
    
    AE_RETURN(pThreadInfo);

error:

    if(pThreadInfo)
    {
        if(pThreadInfo->hTimerWait)
        {
            UnregisterWaitEx(pThreadInfo->hTimerWait, INVALID_HANDLE_VALUE );
        }
        if(pThreadInfo->hTimer)
        {
            CloseHandle(pThreadInfo->hTimer);
        }
        if(pThreadInfo->hNotifyWait)
        {
            UnregisterWaitEx(pThreadInfo->hNotifyWait, INVALID_HANDLE_VALUE);
        }
        if(pThreadInfo->hNotifyEvent)
        {
            CloseHandle(pThreadInfo->hNotifyEvent);
        }
        if(pThreadInfo->hToken)
        {
            CloseHandle(pThreadInfo->hToken);
        }

        AEFree(pThreadInfo);
    
    }
    
    AE_RETURN(NULL);
} 

BOOL DeRegisterAutoEnrollment(HANDLE hAuto)
{
    PAUTO_ENROLL_THREAD_INFO    pThreadInfo = (PAUTO_ENROLL_THREAD_INFO)hAuto;

    if(pThreadInfo == NULL)
    {
        return FALSE;
    }
    if(pThreadInfo->hTimerWait)
    {
        UnregisterWaitEx(pThreadInfo->hTimerWait, INVALID_HANDLE_VALUE);
    }
    if(pThreadInfo->hTimer)
    {
        CloseHandle(pThreadInfo->hTimer);
    }

    if(pThreadInfo->hNotifyWait)
    {
        UnregisterWaitEx(pThreadInfo->hNotifyWait, INVALID_HANDLE_VALUE );
    }

    if(pThreadInfo->hNotifyEvent)
    {
        UnregisterGPNotification(pThreadInfo->hNotifyEvent);
        CloseHandle(pThreadInfo->hNotifyEvent);
    }
    if(pThreadInfo->hToken)
    {
        CloseHandle(pThreadInfo->hToken);
    }

    AEFree(pThreadInfo);
    return TRUE;

}


VOID InitializeAutoEnrollmentSupport (VOID)
{

    // NOTE, auto-enrollment is registered by
    // the GPO download startup code.

    AE_BEGIN(L"InitializeAutoEnrollmentSupport");


    g_hUserMutex = CreateMutex(NULL, FALSE, NULL);
    if(g_hUserMutex)
    {
        g_hMachineMutex = CreateMutex(NULL, FALSE, NULL);
    }
    if((g_hUserMutex == NULL) || (g_hMachineMutex == NULL))
    {
        AE_DEBUG((AE_ERROR, L"Could not create enrollment mutex (%lx)\n\r", GetLastError()));
        AE_END();

        return;
    }

    //
    // Load some functions we need
    //

    g_hInstWldap32 = LoadLibrary (TEXT("wldap32.dll"));

    if (!g_hInstWldap32) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);
        goto Exit;
    }

    g_pfnldap_init = (PFNLDAP_INIT) GetProcAddress (g_hInstWldap32,
#ifdef UNICODE
                                        "ldap_initW");
#else
                                        "ldap_initA");
#endif

    if (!g_pfnldap_init) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);

        goto Exit;
    }

    g_pfnldap_bind_s = (PFNLDAP_BIND_S) GetProcAddress (g_hInstWldap32,
#ifdef UNICODE
                                        "ldap_bind_sW");
#else
                                        "ldap_bind_sA");
#endif

    if (!g_pfnldap_bind_s) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);

        goto Exit;
    }

    g_pfnldap_set_option= (PFNLDAP_SET_OPTION) GetProcAddress (g_hInstWldap32,
                                        "ldap_set_option");

    if (!g_pfnldap_set_option) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);

        goto Exit;
    }

    g_pfnldap_search_ext_s = (PFNLDAP_SEARCH_EXT_S) GetProcAddress (g_hInstWldap32,
#ifdef UNICODE
                                        "ldap_search_ext_sW");
#else
                                        "ldap_search_ext_sA");
#endif

    if (!g_pfnldap_search_ext_s) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);
        goto Exit;
    }
    g_pfnldap_explode_dn = (PFNLDAP_EXPLODE_DN) GetProcAddress (g_hInstWldap32,
#ifdef UNICODE
                                        "ldap_explode_dnW");
#else
                                        "ldap_explode_dnA");
#endif

    if (!g_pfnldap_explode_dn) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);
        goto Exit;
    }
    g_pfnldap_first_entry = (PFNLDAP_FIRST_ENTRY) GetProcAddress (g_hInstWldap32,
                                        "ldap_first_entry");

    if (!g_pfnldap_first_entry) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);
        goto Exit;
    }
    g_pfnldap_get_values = (PFNLDAP_GET_VALUES) GetProcAddress (g_hInstWldap32,
#ifdef UNICODE
                                        "ldap_get_valuesW");
#else
                                        "ldap_get_valuesA");
#endif

    if (!g_pfnldap_get_values) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);

        goto Exit;
    }


    g_pfnldap_value_free = (PFNLDAP_VALUE_FREE) GetProcAddress (g_hInstWldap32,
#ifdef UNICODE
                                        "ldap_value_freeW");
#else
                                        "ldap_value_freeA");
#endif

    if (!g_pfnldap_value_free) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);
        goto Exit;
    }


    g_pfnldap_msgfree = (PFNLDAP_MSGFREE) GetProcAddress (g_hInstWldap32,
                                        "ldap_msgfree");

    if (!g_pfnldap_msgfree) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);
        goto Exit;
    }


    g_pfnldap_unbind = (PFNLDAP_UNBIND) GetProcAddress (g_hInstWldap32,
                                        "ldap_unbind");

    if (!g_pfnldap_unbind) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);
        goto Exit;
    }

    g_pfnLdapGetLastError = (PFNLDAPGETLASTERROR) GetProcAddress (g_hInstWldap32,
                                        "LdapGetLastError");

    if (!g_pfnLdapGetLastError) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);
        goto Exit;
    }

    g_pfnLdapMapErrorToWin32 = (PFNLDAPMAPERRORTOWIN32) GetProcAddress (g_hInstWldap32,
                                        "LdapMapErrorToWin32");

    if (!g_pfnLdapMapErrorToWin32) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);
        goto Exit;
    }

    g_hInstSecur32 = LoadLibrary (TEXT("secur32.dll"));

    if (!g_hInstSecur32) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);

        goto Exit;
    }


    g_pfnGetUserNameEx = (PFNGETUSERNAMEEX)GetProcAddress (g_hInstSecur32,
#ifdef UNICODE
                                        "GetUserNameExW");
#else
                                        "GetUserNameExA");
#endif
    if (!g_pfnGetUserNameEx) {
        LogAutoEnrollmentError(GetLastError(), EVENT_AE_INITIALIZATION_FAILED, TRUE, NULL, NULL, NULL);
        goto Exit;
    }


    AE_END();
    return;
Exit:

    if(g_hInstSecur32)
    {
        FreeLibrary(g_hInstSecur32);
        g_hInstSecur32 = NULL;

    }
    if(g_hInstWldap32)
    {
        FreeLibrary(g_hInstWldap32);
        g_hInstWldap32 = NULL;
    }

    AE_END();

    return;

}

#if DBG
void
AEDebugLog(long Mask,  LPCWSTR Format, ...)
{
    va_list ArgList;
    int     Level = 0;
    int     PrefixSize = 0;
    int     iOut;
    WCHAR    wszOutString[MAX_DEBUG_BUFFER];
    long    OriginalMask = Mask;

    if (Mask & g_AutoenrollDebugLevel)
    {

	    // Make the prefix first:  "Process.Thread> GINA-XXX"

	    iOut = wsprintfW(
			    wszOutString,
			    L"%3d.%3d> AUTOENRL: ",
			    GetCurrentProcessId(),
			    GetCurrentThreadId());

	    va_start(ArgList, Format);

	    if (wvsprintf(&wszOutString[iOut], Format, ArgList) < 0)
	    {
	        static WCHAR wszOverFlow[] = L"\n<256 byte OVERFLOW!>\n";

	        // Less than zero indicates that the string would not fit into the
	        // buffer.  Output a special message indicating overflow.

	        wcscpy(
		    &wszOutString[(sizeof(wszOutString) - sizeof(wszOverFlow))/sizeof(WCHAR)],
		    wszOverFlow);
	    }
	    va_end(ArgList);
	    OutputDebugStringW(wszOutString);
    }
}
#endif

HRESULT 
myGetConfigDN(
    IN  LDAP *pld,
    OUT LPWSTR *pwszConfigDn
    )

{

    HRESULT hr;
    ULONG  LdapError;

    LDAPMessage  *SearchResult = NULL;
    LDAPMessage  *Entry = NULL;
    WCHAR        *Attr = NULL;
    BerElement   *BerElement;
    WCHAR        **Values = NULL;

    WCHAR  *AttrArray[3];
    struct l_timeval        timeout;

    WCHAR  *ConfigurationNamingContext = L"configurationNamingContext";
    WCHAR  *ObjectClassFilter          = L"objectCategory=*";

    //
    // Set the out parameters to null
    //
    if(pwszConfigDn)
    {
        *pwszConfigDn = NULL;
    }

    timeout.tv_sec = 300;
    timeout.tv_usec = 0;
    //
    // Query for the ldap server oerational attributes to obtain the default
    // naming context.
    //
    AttrArray[0] = ConfigurationNamingContext;
    AttrArray[1] = NULL;  // this is the sentinel

    LdapError = g_pfnldap_search_ext_s(pld,
                               NULL,
                               LDAP_SCOPE_BASE,
                               ObjectClassFilter,
                               AttrArray,
                               FALSE,
                               NULL,
                               NULL,
                               &timeout,
                               10000,
                               &SearchResult);

    hr = HRESULT_FROM_WIN32(g_pfnLdapMapErrorToWin32(LdapError));

    if (S_OK == hr) {

        Entry = g_pfnldap_first_entry(pld, SearchResult);

        if (Entry) {

            Values = g_pfnldap_get_values(pld, 
                                        Entry, 
                                        ConfigurationNamingContext);

            if (Values && Values[0]) {
                (*pwszConfigDn) = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*(wcslen(Values[0])+1));
                wcscpy((*pwszConfigDn), Values[0]);
            }

            g_pfnldap_value_free(Values);

        }

        if (pwszConfigDn && (!(*pwszConfigDn))) {
            //
            // We could get the default domain - bail out
            //
            hr =  HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO);

        }
        if(SearchResult)
        {
            g_pfnldap_msgfree(SearchResult);
        }

    }

    return hr;
}


LPWSTR HelperExtensionToString(PCERT_EXTENSION Extension)
{
    LPWSTR wszFormat = NULL;
    DWORD cbFormat = 0;


    CryptFormatObject(X509_ASN_ENCODING,
                      0,
                      0,
                      NULL,
                      Extension->pszObjId,
                      Extension->Value.pbData,
                      Extension->Value.cbData,
                      NULL,
                      &cbFormat);
    if(cbFormat)
    {
        wszFormat = (LPWSTR)AEAlloc(cbFormat);
        if(wszFormat == NULL)
        {
            return NULL;
        }

        CryptFormatObject(X509_ASN_ENCODING,
                          0,
                          0,
                          NULL,
                          Extension->pszObjId,
                          Extension->Value.pbData,
                          Extension->Value.cbData,
                          wszFormat,
                          &cbFormat);

    }

    return wszFormat;
}


//*************************************************************
//
//  MakeGenericSecurityDesc()
//
//  Purpose:    manufacture a security descriptor with generic
//              access
//
//  Parameters:
//
//  Return:     pointer to SECURITY_DESCRIPTOR or NULL on error
//
//  Comments:
//
//  History:    Date        Author     Comment
//              4/12/99     NishadM    Created
//
//*************************************************************

PISECURITY_DESCRIPTOR AEMakeGenericSecurityDesc()
{
    PISECURITY_DESCRIPTOR       psd = 0;
    SID_IDENTIFIER_AUTHORITY    authNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    authWORLD = SECURITY_WORLD_SID_AUTHORITY;

    PACL    pAcl = 0;
    PSID    psidSystem = 0,
            psidAdmin = 0,
            psidEveryOne = 0;
    DWORD   cbMemSize;
    DWORD   cbAcl;
    DWORD   aceIndex;
    BOOL    bSuccess = FALSE;

    //
    // Get the system sid
    //

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         goto Exit;
    }

    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         goto Exit;
    }

    //
    // Get the EveryOne sid
    //

    if (!AllocateAndInitializeSid(&authWORLD, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidEveryOne)) {

        goto Exit;
    }

    cbAcl = (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidAdmin))  +
            (2 * GetLengthSid (psidEveryOne))  +
            sizeof(ACL) +
            (6 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    //
    // Allocate space for the SECURITY_DESCRIPTOR + ACL
    //

    cbMemSize = sizeof( SECURITY_DESCRIPTOR ) + cbAcl;

    psd = (PISECURITY_DESCRIPTOR) GlobalAlloc(GMEM_FIXED, cbMemSize);

    if (!psd) {
        goto Exit;
    }

    //
    // increment psd by sizeof SECURITY_DESCRIPTOR
    //

    pAcl = (PACL) ( ( (unsigned char*)(psd) ) + sizeof(SECURITY_DESCRIPTOR) );

    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        goto Exit;
    }

    //
    // GENERIC_ALL for local system
    //

    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        goto Exit;
    }

    //
    // GENERIC_ALL for Administrators
    //

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        goto Exit;
    }

    //
    // GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE for world
    //

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE, psidEveryOne)) {
        goto Exit;
    }

    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION)) {
        goto Exit;
    }

    if (!SetSecurityDescriptorDacl(psd, TRUE, pAcl, FALSE)) {
        goto Exit;
    }

    bSuccess = TRUE;
Exit:
    if (psidSystem) {
        FreeSid(psidSystem);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (psidEveryOne) {
        FreeSid(psidEveryOne);
    }

    if (!bSuccess && psd) {
        GlobalFree(psd);
        psd = 0;
    }

    return psd;
}

HRESULT
aeRobustLdapBind(
    OUT LDAP ** ppldap,
    IN BOOL fGC)
{
    DWORD dwErr = ERROR_SUCCESS;
    HRESULT hr = S_OK;
    BOOL fForceRediscovery = FALSE;
    DWORD dwGetDCFlags = DS_RETURN_DNS_NAME | DS_BACKGROUND_ONLY;
    PDOMAIN_CONTROLLER_INFO pDomainInfo = NULL;
    LDAP *pld = NULL;
    LPWSTR   wszDomainControllerName = NULL;

    ULONG   ldaperr;

    if(fGC)
    {
        dwGetDCFlags |= DS_GC_SERVER_REQUIRED;
    }

    do {
        // netapi32!DsGetDcName is delay loaded, so wrap
        if(fForceRediscovery)
        {
           dwGetDCFlags |= DS_FORCE_REDISCOVERY;
        }
        ldaperr = LDAP_SERVER_DOWN;

        __try
        {
            // Get the GC location
            dwErr = DsGetDcName(NULL,     // Delayload wrapped
                                NULL, 
                                NULL, 
                                NULL,
                                 dwGetDCFlags,
                                &pDomainInfo);
        }
        __except(dwErr = GetExceptionError(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
        {
        }
        if(dwErr != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
            goto error;
        }

        if((pDomainInfo == NULL) || 
           ((pDomainInfo->Flags && DS_GC_FLAG) == 0) ||
           ((pDomainInfo->Flags && DS_DNS_CONTROLLER_FLAG) == 0) ||
           (pDomainInfo->DomainControllerName == NULL))
        {
            if(!fForceRediscovery)
            {
                fForceRediscovery = TRUE;
                continue;
            }
            hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO);
            goto error;
        }


        wszDomainControllerName = pDomainInfo->DomainControllerName;


        // skip past forward slashes (why are they there?)
        while(*wszDomainControllerName == L'\\')
        {
            wszDomainControllerName++;
        }

        // bind to ds
        if((pld = g_pfnldap_init(wszDomainControllerName, fGC?LDAP_GC_PORT:LDAP_PORT)) == NULL)
        {
            ldaperr = g_pfnLdapGetLastError();
        }
        else
        {
	    ldaperr = g_pfnldap_bind_s(pld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
        }
        hr = HRESULT_FROM_WIN32(g_pfnLdapMapErrorToWin32(ldaperr));

        if(fForceRediscovery)
        {
            break;
        }
        fForceRediscovery = TRUE;

    } while(ldaperr == LDAP_SERVER_DOWN);

    if(hr == S_OK)
    {
        *ppldap = pld;
        pld = NULL;
    }

error:

    if(pld)
    {
        g_pfnldap_unbind(pld);
    }
    // we know netapi32 was already loaded safely (that's where we got pDomainInfo), so no need to wrap
    if(pDomainInfo)
    {
        NetApiBufferFree(pDomainInfo);     // Delayload wrapped
    }
    return hr;
}

