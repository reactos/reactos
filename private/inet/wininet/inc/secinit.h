/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    secinit.h

Abstract:

    Contains prototypes for indirected security functions

Author:

    Sophia Chung (sophiac) 7-Feb-1996

Revision History:

--*/

#if !defined(_SECINIT_)

#define _SECINIT_

#if defined(__cplusplus)
extern "C" {
#endif

#include <sspi.h>

extern CRITICAL_SECTION InitializationSecLock;

extern PSecurityFunctionTable   GlobalSecFuncTable;
extern WIN_VERIFY_TRUST_FN      pWinVerifyTrust;
extern WT_HELPER_PROV_DATA_FROM_STATE_DATA_FN pWTHelperProvDataFromStateData;
extern SSL_CRACK_CERTIFICATE_FN pSslCrackCertificate;
extern SSL_FREE_CERTIFICATE_FN  pSslFreeCertificate;

extern HCERTSTORE   g_hMyCertStore;
extern BOOL         g_bOpenMyCertStore;
extern BOOL         g_fDoSpecialMagicForSGCCerts;

#define g_EnumerateSecurityPackages \
        (*(GlobalSecFuncTable->EnumerateSecurityPackagesA))
#define g_AcquireCredentialsHandle  \
        (*(GlobalSecFuncTable->AcquireCredentialsHandleA))
#define g_FreeCredentialsHandle     \
        (*(GlobalSecFuncTable->FreeCredentialHandle))
#define g_InitializeSecurityContext \
        (*(GlobalSecFuncTable->InitializeSecurityContextA))
#define g_DeleteSecurityContext     \
        (*(GlobalSecFuncTable->DeleteSecurityContext))
#define g_QueryContextAttributes    \
        (*(GlobalSecFuncTable->QueryContextAttributesA))
#define g_FreeContextBuffer         \
        (*(GlobalSecFuncTable->FreeContextBuffer))
#define g_SealMessage               \
        (*((SEAL_MESSAGE_FN)GlobalSecFuncTable->Reserved3))
#define g_UnsealMessage             \
        (*((UNSEAL_MESSAGE_FN)GlobalSecFuncTable->Reserved4))

LONG WINAPI WinVerifySecureChannel(HWND hwnd, WINTRUST_DATA *pWTD);

// Don't use WinVerifyTrust directly to verify secure channel connections.
// Use the wininet wrapper WinVerifySecureChannel instead.
#define g_WinVerifyTrust \
        pWinVerifyTrust


#define g_SslCrackCertificate \
        pSslCrackCertificate

#define g_SslFreeCertificate pSslFreeCertificate
        

typedef PSecurityFunctionTable  (APIENTRY *INITSECURITYINTERFACE) (VOID);

#define CRYPT_INSTALL_DEFAULT_CONTEXT_NAME      "CryptInstallDefaultContext"

typedef BOOL
(WINAPI * CRYPT_INSTALL_DEFAULT_CONTEXT_FN)
(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwDefaultType,
    IN const void *pvDefaultPara,
    IN DWORD dwFlags,
    IN void *pvReserved,
    OUT HCRYPTDEFAULTCONTEXT *phDefaultContext
);

#define CRYPT_UNINSTALL_DEFAULT_CONTEXT_NAME    "CryptUninstallDefaultContext"
    
typedef BOOL
(WINAPI * CRYPT_UNINSTALL_DEFAULT_CONTEXT_FN)   
(
    HCRYPTDEFAULTCONTEXT hDefaultContext,
    IN DWORD dwFlags,
    IN void *pvReserved
);

typedef PCCERT_CHAIN_CONTEXT
(WINAPI *CERT_FIND_CHAIN_IN_STORE_FN)
(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN DWORD dwFindFlags,
    IN DWORD dwFindType,
    IN const void *pvFindPara,
    IN PCCERT_CHAIN_CONTEXT pPrevChainContext
);

#define CERT_FIND_CHAIN_IN_STORE_NAME            "CertFindChainInStore"

typedef VOID
(WINAPI *CERT_FREE_CERTIFICATE_CHAIN_FN)
(
    IN PCCERT_CHAIN_CONTEXT pChainContext
);

#define CERT_FREE_CERTIFICATE_CHAIN_NAME        "CertFreeCertificateChain"


extern CRYPT_INSTALL_DEFAULT_CONTEXT_FN g_CryptInstallDefaultContext;
extern CRYPT_UNINSTALL_DEFAULT_CONTEXT_FN g_CryptUninstallDefaultContext;
extern CERT_FIND_CHAIN_IN_STORE_FN        g_CertFindChainInStore;
extern CERT_FREE_CERTIFICATE_CHAIN_FN     g_CertFreeCertificateChain;

extern HCRYPTPROV GlobalFortezzaCryptProv;

#define LOCK_SECURITY()   EnterCriticalSection( &InitializationSecLock )
#define UNLOCK_SECURITY() LeaveCriticalSection( &InitializationSecLock )


//
// prototypes
//

VOID
SecurityInitialize(
    VOID
    );

VOID
SecurityTerminate(
    VOID
    );

DWORD
ReopenMyCertStore(
    VOID
    );

DWORD
CloseMyCertStore(
    VOID
    );

DWORD
LoadSecurity(
    VOID
    );

VOID
UnloadSecurity(
    VOID
    );

DWORD
LoadWinTrust(
    VOID
    );


BOOL
IsFortezzaInstalled(
    VOID
    );

BOOL AttemptedFortezzaLogin(
    VOID
    );

DWORD FortezzaLogOn(
    HWND hwnd
    );

#if defined(__cplusplus)
}
#endif

#endif // _SECINIT_
