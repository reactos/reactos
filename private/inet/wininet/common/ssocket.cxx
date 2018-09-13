/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ssocket.cxx

Abstract:

    Contains secure sockets functions and ICSecureSocket methods

    Contents:
        SecurityPkgInitialize
        ReadCertificateIntoCertInfoStruct
        ChkCertificateCommonNameIsValid
        ChkCertificateExpired
        ICSecureSocket::ICSecureSocket
        ICSecureSocket::~ICSecureSocket
        ICSecureSocket::Connect
        CFsm_SecureConnect::RunSM
        ICSecureSocket::Connect_Fsm
        ICSecureSocket::SecureHandshakeWithServer
        CFsm_SecureHandshake::RunSM
        ICSecureSocket::SecureHandshake_Fsm
        ICSecureSocket::NegotiateSecConnection
        CFsm_SecureNegotiate::RunSM
        ICSecureSocket::SecureNegotiate_Fsm
        ICSecureSocket::SSPINegotiateLoop
        CFsm_NegotiateLoop::RunSM
        ICSecureSocket::NegotiateLoop_Fsm
        ICSecureSocket::Disconnect
        ICSecureSocket::Send
        CFsm_SecureSend::RunSM
        ICSecureSocket::Send_Fsm
        ICSecureSocket::Receive
        CFsm_SecureReceive::RunSM
        ICSecureSocket::Receive_Fsm
        ICSecureSocket::SetHostName
        (ICSecureSocket::EncryptData)
        (ICSecureSocket::DecryptData)
        (ICSecureSocket::TerminateSecConnection)
        ICSecureSocket::GetCertInfo

Author:

    Richard L Firth (rfirth) 08-Apr-1997

Environment:

    Win32 user mode

Revision History:

    08-Apr-1997 rfirth
        Created from ixport.cxx

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include <ierrui.hxx>

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsecapi.h>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <softpub.h>

}

//
//
//  List of encryption packages:  PCT, SSL, etc
//

//
// BUGBUG [arthurbi] The SSL and PCT package names
//  are hard coded into the stucture below.  We need
//  to be more flexible in case someone write a FOO security
//  package.
//

// BUGBUG:  Don't change the order of the packages below.  some old SSL2 sites deny the UNISP
// provider, and if we walk down the list to PCT1 or SSL3, things hang.
struct _SEC_PROVIDER SecProviders[] =
{
    UNISP_NAME, INVALID_CRED_VALUE , ENC_CAPS_PCT | ENC_CAPS_SSL | ENC_CAPS_SCHANNEL_CREDS, FALSE, SP_PROT_CLIENTS, NULL,
    UNISP_NAME, INVALID_CRED_VALUE , ENC_CAPS_SSL | ENC_CAPS_SCHANNEL_CREDS, FALSE, SP_PROT_SSL2_CLIENT, NULL,
//    PCT1SP_NAME, INVALID_CRED_VALUE , ENC_CAPS_PCT| ENC_CAPS_SCHANNEL_CREDS, FALSE, SP_PROT_PCT1_CLIENT, NULL,
//    SSL3SP_NAME, INVALID_CRED_VALUE , ENC_CAPS_SSL| ENC_CAPS_SCHANNEL_CREDS, FALSE, SP_PROT_SSL3_CLIENT, NULL,
    NULL,        INVALID_CRED_VALUE , FALSE,        FALSE, 0
};



//
// dwEncFlags - Global Status of calling and initalizing the SCHANNEL and various
//   other encyrption support DLL & APIs.  Failure in the process will
//   cause this to be set to an error state, success prevents re-initalizaiton
//

DWORD dwEncFlags = 0;

//
// GlobalSecureProtocolsCopy - Copy of the current protocols the user wants to use
//   changing them allows us to restrict to specific protocols
//
DWORD GlobalSecureProtocolsCopy = DEFAULT_SECURE_PROTOCOLS;


#ifdef SECPKG_ATTR_PROTO_INFO
PRIVATE
LPTSTR
ProtoInfoToString(
    IN const PSecPkgContext_ProtoInfo pProtoInfo);
#endif

//
// general security package functions
//
BOOL
SecurityPkgInitialize(
    BOOL fForce
    )
/*++

Routine Description:

    This function finds a list of security packages that are supported
    on the client's machine, check if pct or ssl is supported, and
    create a credential handle for each supported pkg.

Arguments:

    None

Return Value:

    TRUE if at least one security pkg is found; otherwise FALSE

--*/
{
    TimeStamp         tsExpiry;
    SECURITY_STATUS   scRet;
    PSecPkgInfo       pPackageInfo = NULL;
    ULONG             cPackages;
    ULONG             fCapabilities;
    ULONG             i;
    ULONG             j;
    DWORD             cProviders = 0;

    SCHANNEL_CRED DefaultCredData = {SCHANNEL_CRED_VERSION,
                                     0,
                                     NULL,
                                     0,
                                     0,
                                     NULL,
                                     0,
                                     NULL,
                                     SP_PROT_CLIENTS,
                                     0,
                                     0,
                                     0,
                                     SCH_CRED_MANUAL_CRED_VALIDATION |
                                     SCH_CRED_NO_DEFAULT_CREDS
                                    };

    //
    // Set new DWORD for our copy of the global protocol settings.
    //
    bool    fSame = (GlobalSecureProtocolsCopy==GlobalSecureProtocols);
    GlobalSecureProtocolsCopy = GlobalSecureProtocols;

    //
    //  check if this routine has been called.  if yes, return TRUE
    //  if we've found a supported pkg; otherwise FALSE
    //

    if ( dwEncFlags == ENC_CAPS_NOT_INSTALLED )
       return FALSE;
    else if ((dwEncFlags&ENC_CAPS_TYPE_MASK) && fSame && !fForce)
       return TRUE;

    //
    //  Initialize dwEncFlags
    //

    dwEncFlags = ENC_CAPS_NOT_INSTALLED;

    //
    //  Check if at least one security package is supported
    //


    scRet = g_EnumerateSecurityPackages( &cPackages,
                                         &pPackageInfo );

    if ( scRet != STATUS_SUCCESS )
    {
        DEBUG_PRINT(API,
                    ERROR,
                    ("EnumerateSecurityPackages failed, error %lx\n",
                    scRet
                    ));

        SetLastError( scRet );
        return FALSE;
    }

    for ( i = 0; i < cPackages ; i++ )
    {
        //
        //  Use only if the package name is the PCT/SSL package
        //

        fCapabilities = pPackageInfo[i].fCapabilities;

        if ( fCapabilities & SECPKG_FLAG_STREAM )
        {
            //
            //  Check if the package supports server side authentication
            //  and all recv/sent messages are tamper proof
            //

            if ( fCapabilities & SECPKG_FLAG_CLIENT_ONLY ||
                 !(fCapabilities & SECPKG_FLAG_PRIVACY ))
            {
                continue;
            }

            //
            //  Check if the pkg matches one of our known packages
            //

            for ( j = 0; SecProviders[j].pszName != NULL; j++ )
            {
                if ( !stricmp( pPackageInfo[i].Name, SecProviders[j].pszName ) )
                {
                    CredHandle OldCred;
                    PVOID pCredData = NULL;

                    //
                    //  Create a credential handle for each supported pkg
                    //

                    INET_ASSERT((SecProviders[j].dwFlags & ENC_CAPS_SCHANNEL_CREDS));

                    pCredData = &DefaultCredData;

                    if (SecProviders[j].pCertCtxt != NULL) {
                        DefaultCredData.cCreds = 1;
                        DefaultCredData.paCred = &SecProviders[j].pCertCtxt;
                    }

                    //
                    // Enable Supported protocols in the Default Cred Data, then acquire the Credential
                    //

                    DefaultCredData.grbitEnabledProtocols = (GlobalSecureProtocols & SecProviders[j].dwProtocolFlags);

                    OldCred.dwUpper = SecProviders[j].hCreds.dwUpper;
                    OldCred.dwLower = SecProviders[j].hCreds.dwLower;

                    // Zero out previous credentials
                    SecProviders[j].hCreds.dwUpper = SecProviders[j].hCreds.dwLower = 0;

                    scRet = g_AcquireCredentialsHandle(
                                      NULL,
                                      SecProviders[j].pszName, // Package
                                      SECPKG_CRED_OUTBOUND,
                                      NULL,
                                      pCredData,
                                      NULL,
                                      NULL,
                                      &(SecProviders[j].hCreds), // Handle
                                      &tsExpiry );

                    if(!IS_CRED_INVALID(&OldCred))
                    {
                        g_FreeCredentialsHandle(&OldCred);
                    }

                    DefaultCredData.cCreds = 0;
                    DefaultCredData.paCred = NULL;

                    if ( scRet != STATUS_SUCCESS )
                    {
                        DEBUG_PRINT(API,
                                    WARNING,
                                    ("AcquireCredentialHandle failed, error %lx\n",
                                    scRet
                                    ));

                        SecProviders[j].fEnabled = FALSE;

                        SecProviders[j].hCreds.dwUpper = 0xffffffff;
                        SecProviders[j].hCreds.dwLower = 0xffffffff;


                    }
                    else
                    {
                        DEBUG_PRINT(
                             API,
                             INFO,
                             ("AcquireCredentialHandle() supports %s, acquires %x:%x\n",
                             SecProviders[j].pszName,
                             SecProviders[j].hCreds.dwUpper,
                             SecProviders[j].hCreds.dwLower
                             ));

                        SecProviders[j].fEnabled = TRUE;
                        cProviders++;
                        dwEncFlags |= SecProviders[j].dwFlags;
                    }
                }
            }
        }
    }

    if ( !cProviders )
    {
        //
        //  No security packages were found, return FALSE to caller
        //

        DEBUG_PRINT(API,
                    ERROR,
                    ("No security packages were found, error %lx\n",
                    SEC_E_SECPKG_NOT_FOUND
                    ));

        g_FreeContextBuffer( pPackageInfo );

        SetLastError( (DWORD) SEC_E_SECPKG_NOT_FOUND );

        return FALSE;
    }

    //
    //  Successfully found a security package(s)
    //

    return TRUE;
}


DWORD
QuerySecurityInfo(
                  IN CtxtHandle *hContext,
                  OUT LPINTERNET_SECURITY_INFO pInfo)
{
    SECURITY_STATUS      scRet;

    scRet = g_QueryContextAttributes(hContext,
                                     SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                     &pInfo->pCertificate );

    if (scRet != ERROR_SUCCESS)
    {

        //
        // Map the SSPI error.
        //
        return MapInternetError((DWORD) scRet);

    }

    scRet = g_QueryContextAttributes(hContext,
                                     SECPKG_ATTR_CONNECTION_INFO,
                                     &pInfo->dwProtocol );

    if (scRet != ERROR_SUCCESS)
    {

        //
        // Map the SSPI error.
        //
        return MapInternetError((DWORD) scRet);
    }

    pInfo->dwSize = sizeof(INTERNET_SECURITY_INFO);

    return ERROR_SUCCESS;
}

// Helper function to detect Fortezza connections.
BOOL IsCertificateFortezza(PCCERT_CONTEXT pCertContext)
{
    INET_ASSERT(pCertContext != NULL);
    if (pCertContext == NULL)
        return FALSE;

    LPSTR pszOid = pCertContext->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId;

    if (pszOid)
    {
        if (strcmp(pszOid, szOID_INFOSEC_mosaicUpdatedSig) == 0 ||
             strcmp(pszOid, szOID_INFOSEC_mosaicKMandUpdSig) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}


LONG WinVerifySecureChannel(HWND hwnd, WINTRUST_DATA *pWTD)
/*++

Routine Description:

    Wininet's wrapper for secure channel WinVerifyTrust calls.

Arguments:

    hWnd - in case WinVerifyTrust needs to do UI.
    pWTD - pointer to WINTRUST_DATA containing details about the
           secure channel. Passed to WinVerifyTrust.
Return Value:

    WIN32 error code.

--*/
{
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    BOOL async;
    LONG  lResult;
    BOOL  bFortezza;
    GUID  gHTTPS = HTTPSPROV_ACTION;

    if (lpThreadInfo != NULL) {
        async = _InternetGetAsync(lpThreadInfo);
        _InternetSetAsync(lpThreadInfo, FALSE);
    }
    bFortezza = IsCertificateFortezza(pWTD->pCert->psCertContext);

    if (bFortezza && g_CryptInstallDefaultContext == NULL)
    {
        // HACK: we have no way to verify a connection without
        // a crypt32 which has the new APIs exposed. Till IE5 picks up
        // the new crypto bits we will assume Fortezza connections
        // verify correctly.
        lResult = ERROR_SUCCESS;
    }
    else
    {
        HCRYPTDEFAULTCONTEXT hCryptDefaultContext = NULL;

        if (bFortezza)
        {
            if (!g_CryptInstallDefaultContext(
                        GlobalFortezzaCryptProv,
                        CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID,
                        szOID_INFOSEC_mosaicUpdatedSig,         // check with John Banes
                        0,                                      // dwFlags
                        NULL,                                   // pvReserved
                        &hCryptDefaultContext
                        ))
            {
                lResult = GetLastError();
                goto quit;
            }
        }

        lResult = g_WinVerifyTrust(hwnd, &gHTTPS, pWTD);

        DEBUG_PUT(("WinVerifyTrust returned: %x\n", lResult));

        if (hCryptDefaultContext)
        {
            // Ignore error code while freeing since we can't do anything
            // meaningful about it here.
            BOOL bResult;
            bResult = g_CryptUninstallDefaultContext(
                        hCryptDefaultContext,
                        0,
                        NULL);
            INET_ASSERT(bResult);
        }
    }

quit:

    if (lpThreadInfo != NULL) {
        _InternetSetAsync(lpThreadInfo, async);
    }
    return lResult;
}


//
// ICSecureSocket methods
//


ICSecureSocket::ICSecureSocket(
    IN DWORD dwErrorFlags,
    IN INTERNET_SCHEME tScheme
    )

/*++

Routine Description:

    ICSecureSocket constructor

Arguments:

    tScheme - which protocol scheme we are creating the socket for

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "ICSecureSocket::ICSecureSocket",
                 "{%#x}",
                 this
                 ));

    SIGN_SECURE_SOCKET();

    m_hContext.dwLower = m_hContext.dwUpper = 0;
    m_dwProviderIndex = 0;
    m_dwFlags |= SF_SECURE;
    m_dwErrorFlags = dwErrorFlags;
    m_lpszHostName = NULL;
    m_pdblbufBuffer = NULL;
    m_pSecurityInfo = NULL;

    DEBUG_LEAVE(0);
}


ICSecureSocket::~ICSecureSocket()

/*++

Routine Description:

    ICSecureSocket destructor. Virtual function

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "ICSecureSocket::~ICSecureSocket",
                 "{%#x [%q, sock=%#x, port=%d]}",
                 this,
                 GetHostName(),
                 GetSocket(),
                 GetSourcePort()
                 ));

    CHECK_SECURE_SOCKET();
    INET_ASSERT(IsSecure());

    if (m_pdblbufBuffer != NULL) {
        delete m_pdblbufBuffer;
    }

    // Free security context associated with this object if it's
    // still allocated.
    TerminateSecConnection();

    /* SCLE ref */
    SetSecurityEntry(NULL);
    if (m_lpszHostName != NULL) {
        m_lpszHostName = (LPSTR)FREE_MEMORY(m_lpszHostName);
        INET_ASSERT(m_lpszHostName == NULL);
    }
    //if ( _pCertChainList )
    //    delete _pCertChainList;

    DEBUG_LEAVE(0);
}


DWORD
ICSecureSocket::Connect(
    IN LONG Timeout,
    IN INT Retries,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Initiate secure connection with server

Arguments:

    Timeout - maximum amount of time (mSec) to wait for connection

    Retries - maximum number of attempts to connect

    dwFlags - flags controlling request

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Couldn't create FSM

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::Connect",
                 "{%#x [%#x]} %d, %d, %#x",
                 this,
                 m_Socket,
                 Timeout,
                 Retries,
                 dwFlags
                 ));

    INET_ASSERT(IsSecure());

    DWORD error = DoFsm(new CFsm_SecureConnect(Timeout,
                                               Retries,
                                               dwFlags,
                                               this
                                               ));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_SecureConnect::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CFsm_SecureConnect::RunSM",
                 "%#x",
                 Fsm
                 ));

    ICSecureSocket * pSecureSocket = (ICSecureSocket *)Fsm->GetContext();
    CFsm_SecureConnect * stateMachine = (CFsm_SecureConnect *)Fsm;
    DWORD error;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pSecureSocket->Connect_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::Connect_Fsm(
    IN CFsm_SecureConnect * Fsm
    )
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::Connect_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_SecureConnect & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if (fsm.GetState() != FSM_STATE_INIT) {
        switch (fsm.GetFunctionState()) {
        case FSM_STATE_2:
            goto connect_continue;

        case FSM_STATE_3:
            goto negotiate_continue;

        default:
            error = ERROR_INTERNET_INTERNAL_ERROR;

            INET_ASSERT(FALSE);

            goto quit;
        }
    }

    m_dwProviderIndex = 0;
    //SetNonSecure();

    //
    // Hack for SSL2 Client Hello, set to FALSE,
    //  but if we fail on the first recv, fReOpenSocket
    //  is set to TRUE.
    //

    do {

        //
        // Attempt to do the connect
        //

        fsm.SetFunctionState(FSM_STATE_2);
        error = ICSocket::Connect(fsm.m_Timeout, fsm.m_Retries, fsm.m_dwFlags);

connect_continue:

        if (error != ERROR_SUCCESS) {
            break;
        }
        if (m_dwFlags & SF_ENCRYPT) {
            fsm.SetFunctionState(FSM_STATE_3);
            error = SecureHandshakeWithServer(fsm.m_dwFlags, &fsm.m_bAttemptReconnect);
            if (error == ERROR_IO_PENDING) {
                break;
            }

negotiate_continue:

            //
            // SSL2 hack for old IIS servers.
            //  We re-open the socket, and call again.
            //

            if ((error != ERROR_SUCCESS) && fsm.m_bAttemptReconnect) {
                Disconnect(fsm.m_dwFlags);
            }
        }
    } while (fsm.m_bAttemptReconnect);

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
        if ((error != ERROR_SUCCESS) && IsOpen()) {
            Disconnect(fsm.m_dwFlags);
        }
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::SecureHandshakeWithServer(
    IN DWORD dwFlags,
    OUT LPBOOL lpbAttemptReconnect
    )

/*++

Routine Description:

    For SSL/PCT or some secure channel this function attempts to use
    an arbitrary Socket for handshaking with a server. The assumption
    is made that caller can recall this function on failure

Arguments:

    dwFlags             -

    lpbAttemptReconnect -

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                Dword,
                "ICSecureSocket::SecureHandshakeWithServer",
                "%#x, %#x [%B]",
                dwFlags,
                lpbAttemptReconnect,
                *lpbAttemptReconnect
                ));

    INET_ASSERT(IsSecure());

    DWORD error = DoFsm(new CFsm_SecureHandshake(dwFlags,
                                                 lpbAttemptReconnect,
                                                 this
                                                 ));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_SecureHandshake::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CFsm_SecureHandshake::RunSM",
                 "%#x",
                 Fsm
                 ));

    ICSecureSocket * pSecureSocket = (ICSecureSocket *)Fsm->GetContext();
    CFsm_SecureHandshake * stateMachine = (CFsm_SecureHandshake *)Fsm;
    DWORD error;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pSecureSocket->SecureHandshake_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::SecureHandshake_Fsm(
    IN CFsm_SecureHandshake * Fsm
    )
{


    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::SecureHandshake_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_SecureHandshake & fsm = *Fsm;
    DWORD error = fsm.GetError();
    DWORD dwSecureFlags;
    DWORD dwCertFlags;
    BOOL fErrorInvalidCa;

    if (fsm.GetState() != FSM_STATE_INIT) {
        switch (fsm.GetFunctionState()) {
        case FSM_STATE_2:
            goto negotiate_continue;

        default:
            error = ERROR_INTERNET_INTERNAL_ERROR;

            INET_ASSERT(FALSE);

            goto quit;
        }
    }

    //INET_ASSERT(fsm.m_dwFlags & SF_ENCRYPT);
    INET_ASSERT(m_Socket != INVALID_SOCKET);

    *fsm.m_lpbAttemptReconnect = FALSE;

    error = ERROR_SUCCESS;

    //
    // Save Off Flags in our Internal Object.
    //   Treat SF_ENCRYPT just like SF_DECRYPT
    //

    m_dwFlags |= SF_DECRYPT;
    m_dwFlags |= fsm.m_dwFlags;

    INET_ASSERT(!(m_dwFlags
                & ~(SF_NON_BLOCKING
                    | SF_SECURE
                    | SF_ENCRYPT
                    | SF_DECRYPT
                    | SF_INDICATE
                    | SF_SENDING_DATA
                    )));

    //
    // Allocate Internal Buffer for SSL/PCT data.
    //

    if (m_pdblbufBuffer == NULL) {

        BOOL fInitSuccess;

        m_pdblbufBuffer = new DBLBUFFER();
        if (m_pdblbufBuffer == NULL) {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }
        fInitSuccess = m_pdblbufBuffer->InitBuffer(TRUE);
        if (!fInitSuccess) {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }
    }

    // First make sure the security dlls are loaded.
    error = LoadSecurity();

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    // If the user has the Fortezza CSP but has not logged on to the card yet.
    // return back an error to indicate that we need to put up additional UI.

    // if (IsFortezzaInstalled( ) && !AttemptedFortezzaLogin( ))
    //{
    //    error = ERROR_INTERNET_FORTEZZA_LOGIN_NEEDED;
    //    goto quit;
    //}

    //
    //  dwEncFlags is a global flag set to the
    //  supported security pkg mask
    //

    LOCK_SECURITY();

    if (dwEncFlags == ENC_CAPS_NOT_INSTALLED) {
         error = (DWORD)SEC_E_SECPKG_NOT_FOUND;
    } else if (dwEncFlags == 0) {


         //
         //  first time thru, do the load.
         //

         DEBUG_PRINT(SOCKETS,
                     INFO,
                     ("Loading security dll\n"
                     ));

        if ( !SecurityPkgInitialize() ) {
             error = GetLastError();
             UNLOCK_SECURITY();
             goto quit;
        }
    }

    UNLOCK_SECURITY();

    if ( error != ERROR_SUCCESS )
    {
        goto quit;
    }


    //
    // If we succeed in loading or and initalizing the Security DLLs, we
    //      attempt to negotiate the connection
    //

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("Negotiate secure channel\n"
                ));

    //
    // Turn of Encryption/Decryption before the handshake,
    // since the NegotiateSecConnection does its own Send and Recvs
    // of specialized data.
    //

    m_dwFlags &= ~(SF_ENCRYPT | SF_DECRYPT);
    fsm.SetFunctionState(FSM_STATE_2);
    error = NegotiateSecConnection(fsm.m_dwFlags,
                                   fsm.m_lpbAttemptReconnect
                                   );
    if (error == ERROR_IO_PENDING) {
        goto quit;
    }

negotiate_continue:

    m_dwFlags |= (SF_ENCRYPT | SF_DECRYPT);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    //  Find out what size Key we're using, and set the flags
    //   acordingly.
    //

    dwSecureFlags = 0;
    if ((m_pSecurityInfo && !m_pSecurityInfo->InCache()) || !IsValidCacheEntry()) {
        error = VerifyTrust();
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    //
    // we've got a secure connection, set the flags.
    //

    SetSecure();

    if(m_pSecurityInfo)
    {
        INTERNET_SECURITY_INFO ciInfo;
        m_pSecurityInfo->CopyOut(ciInfo);

        if(ciInfo.dwCipherStrength < 56)
        {
            SetSecureFlags(SECURITY_FLAG_STRENGTH_WEAK);
        }
        else if (ciInfo.dwCipherStrength==80 &&
                 (ciInfo.aiCipher == CALG_SKIPJACK || ciInfo.aiCipher==CALG_TEK))
        {
            SetSecureFlags(SECURITY_FLAG_FORTEZZA);
        }
        else if(ciInfo.dwCipherStrength < 96)
        {
             SetSecureFlags(SECURITY_FLAG_STRENGTH_MEDIUM);
        }
        else
        {
             SetSecureFlags(SECURITY_FLAG_STRENGTH_STRONG);
        }
        if(ciInfo.pCertificate)
        {
            CertFreeCertificateContext(ciInfo.pCertificate);
            ciInfo.pCertificate = NULL;
        }
    }

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}


BOOL ICSecureSocket:: IsValidCacheEntry()
{

    INTERNET_SECURITY_INFO ciCert;
    INTERNET_SECURITY_INFO ciInfo;
    DWORD error;

    error = QuerySecurityInfo(&m_hContext, &ciCert);
    if (error != ERROR_SUCCESS) {
        return FALSE;
    }

    if (ciCert.pCertificate == NULL) {
        return FALSE;
    }

    if(m_pSecurityInfo)
    {
        m_pSecurityInfo->CopyOut(ciInfo);
        if(ciInfo.pCertificate->cbCertEncoded  != ciCert.pCertificate->cbCertEncoded ||
            memcmp(ciInfo.pCertificate->pbCertEncoded , ciCert.pCertificate->pbCertEncoded , ciInfo.pCertificate->cbCertEncoded))
        {
            CertFreeCertificateContext(ciInfo.pCertificate);
            GlobalCertCache.Remove(GetHostName());
            SECURITY_CACHE_LIST_ENTRY* pSecurityInfo = new SECURITY_CACHE_LIST_ENTRY(GetHostName());
            (*m_ppSecurityInfo)->Release();
            *m_ppSecurityInfo = pSecurityInfo;
            pSecurityInfo->AddRef();
            SetSecurityEntry(&pSecurityInfo);
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

DWORD
ICSecureSocket::VerifyTrust(
    )

/*++

Routine Description:

    This function establishes a secure channel with the server by
    performing the security handshake protocol.  It will walk
    the list of installed security packages until it finds a package
    that succeeds in the security handshake.  If one package fails
    it is up to the caller to re-call NegotiateSecConnection with
    a re-opened socket so another socket can attempt the connection.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/
{

    // We've done our handshake, now update the security info
    INTERNET_SECURITY_INFO ciCert;
    DWORD dwCertFlags = 0;
    WINTRUST_DATA           sWTD;
    WINTRUST_CERT_INFO      sWTCI;
    HTTPSPolicyCallbackData polHttps;
    DWORD                   cbServerName;
    DWORD error;
    HINTERNET  hInternet;
    HINTERNET  hInternetMapped;
    LPINTERNET_THREAD_INFO lpThreadInfo = NULL;

    DWORD dwFlags = 0;    // HTTPS policy flags to ignore errors

    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::VerifyTrust",
                 "{%#x",
                 this
                 ));

    // HACK HACK: 67640
    // WinVerifyTrust can do a nested HttpSendRequest which causes the hObject's on the 
    // thread to get messed up. This happens only when the ceritificate has a URL for 
    // a CRL in it. We save and restore these values to workaround the problem.
    // Need to work out a better solution to handle this but it is too close to ship to 
    // try anything with greater code impact. 
    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo != NULL) {
        hInternet = lpThreadInfo->hObject;
        hInternetMapped = lpThreadInfo->hObjectMapped;
    }


    error = QuerySecurityInfo(&m_hContext, &ciCert);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    if(m_pSecurityInfo)
    {
        *m_pSecurityInfo = &ciCert;
        dwCertFlags = m_pSecurityInfo->GetSecureFlags();

    }

    if (ciCert.pCertificate == NULL) {
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    //    if(!GlobalWarnOnBadCertRecving)
    //    {
    //       dwCertFlags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
    //    }

    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));
    sWTD.cbStruct               = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice             = WTD_UI_NONE;
    sWTD.pPolicyCallbackData    = (LPVOID)&polHttps;
    sWTD.dwUnionChoice          = WTD_CHOICE_CERT;
    sWTD.pCert                  = &sWTCI;
    sWTD.pwszURLReference       = NULL;
    if (GlobalEnableRevocation)
        sWTD.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;


    memset(&sWTCI, 0x00, sizeof(WINTRUST_CERT_INFO));
    sWTCI.cbStruct              = sizeof(WINTRUST_CERT_INFO);
    sWTCI.psCertContext         = (CERT_CONTEXT *)ciCert.pCertificate;
    sWTCI.chStores              = 1;
    sWTCI.pahStores  = (HCERTSTORE *)&ciCert.pCertificate->hCertStore;


    memset(&polHttps, 0x00, sizeof(HTTPSPolicyCallbackData));
    polHttps.cbStruct =  sizeof(HTTPSPolicyCallbackData);
    polHttps.dwAuthType = AUTHTYPE_SERVER;
    polHttps.fdwChecks = dwCertFlags;
    
    cbServerName = MultiByteToWideChar(CP_ACP, 0, m_lpszHostName, -1, NULL, 0);

    polHttps.pwszServerName = new WCHAR[cbServerName+1];

    if(polHttps.pwszServerName == 0)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    sWTCI.pcwszDisplayName      = polHttps.pwszServerName;

    cbServerName = MultiByteToWideChar(CP_ACP, 0, m_lpszHostName, -1, polHttps.pwszServerName, cbServerName);

    error = LoadWinTrust();
    if(ERROR_SUCCESS == error)
    {
        error = WinVerifySecureChannel(NULL, &sWTD);
    }

    error = MapInternetError(error);


    // Handle revocation problem as special case
    if (ERROR_INTERNET_SEC_CERT_REV_FAILED == error)
        dwFlags |= DLG_FLAGS_SEC_CERT_REV_FAILED;

    //
    // If there was problem with the certificate and the caller requested
    // combined SSL errors cycle through all possible certificate errors.
    //

    if (ERROR_SUCCESS != error &&
        (m_dwErrorFlags & INTERNET_ERROR_MASK_COMBINED_SEC_CERT) &&
        m_pSecurityInfo)
    {
        BOOL  fCertError = FALSE;
 
        do
        {
            if (ERROR_INTERNET_INVALID_CA == error)
            {
                polHttps.fdwChecks |=  DLG_FLAGS_IGNORE_INVALID_CA;
                dwFlags |= DLG_FLAGS_INVALID_CA;
                fCertError = TRUE;
            }
            else if (ERROR_INTERNET_SEC_CERT_CN_INVALID == error)
            {
                polHttps.fdwChecks |= DLG_FLAGS_IGNORE_CERT_CN_INVALID;
                dwFlags |= DLG_FLAGS_SEC_CERT_CN_INVALID;
                fCertError = TRUE;
            }
            else if (ERROR_INTERNET_SEC_CERT_DATE_INVALID == error)
            {
                polHttps.fdwChecks |= DLG_FLAGS_IGNORE_CERT_DATE_INVALID;
                dwFlags |= DLG_FLAGS_SEC_CERT_DATE_INVALID;
                fCertError = TRUE;
            }
            else
            {
                //
                // Pass all other errors through.
                //

                break;
            }

            error = WinVerifySecureChannel(NULL, &sWTD);

            error = MapInternetError(error);

        } while (ERROR_SUCCESS != error);

        if (fCertError && !GlobalWarnOnBadCertRecving)  // Handle cert warning supression
        {
            const DWORD dwSuppressedErrors = DLG_FLAGS_SEC_CERT_CN_INVALID;
            if ((dwFlags&~dwSuppressedErrors) == 0)     // If the problem is combination of invalid date and/or CN
                dwFlags = 0, fCertError = FALSE;        // ... accept the certificate without  an error
        }

        //
        // Change the error only if one of the known certifciate errors was
        // encountered.
        //

        if (fCertError)
        {
            error = ERROR_INTERNET_SEC_CERT_ERRORS;
            m_pSecurityInfo->SetSecureFlags(dwFlags);
        }
        else if (dwFlags & DLG_FLAGS_SEC_CERT_REV_FAILED)
        {
            INET_ASSERT(dwFlags==DLG_FLAGS_SEC_CERT_REV_FAILED);
            error = ERROR_INTERNET_SEC_CERT_REV_FAILED;
            m_pSecurityInfo->SetSecureFlags(DLG_FLAGS_SEC_CERT_REV_FAILED);
        }
    }

    delete polHttps.pwszServerName;

    if(ciCert.pCertificate)
    {
        CertFreeCertificateContext(ciCert.pCertificate);
    }

    if (error != ERROR_SUCCESS)
    {
        goto quit;
    }

    if(m_pSecurityInfo && (!m_pSecurityInfo->InCache()))
    {
        // Add it to the cache if it's not already there.
        /* SCLE ref */
        GlobalCertCache.Add(m_pSecurityInfo);

    }

quit:                                 
    if (lpThreadInfo != NULL) {
        _InternetSetObjectHandle(lpThreadInfo, hInternet, hInternetMapped);
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::NegotiateSecConnection(
    IN DWORD dwFlags,
    OUT LPBOOL lpbAttemptReconnect
    )

/*++

Routine Description:

    This function establishes a secure channel with the server by
    performing the security handshake protocol.  It will walk
    the list of installed security packages until it finds a package
    that succeeds in the security handshake.  If one package fails
    it is up to the caller to re-call NegotiateSecConnection with
    a re-opened socket so another socket can attempt the connection.

Arguments:

    dwFlags    - Socket Flags that may need to be passed on to Socket Calls
                (needed to support Async I/O)

    lpbAttemptReconnect - on return, if this value is TRUE, the caller should call
                          this function again, and it will try another protocol.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::NegotiateSecConnection",
                 "{%#x [%#x]} %#x, %#x [%B]",
                 this,
                 m_Socket,
                 dwFlags,
                 lpbAttemptReconnect,
                 *lpbAttemptReconnect
                 ));

    INET_ASSERT(IsSecure());

    DWORD error = DoFsm(new CFsm_SecureNegotiate(dwFlags,
                                                 lpbAttemptReconnect,
                                                 this
                                                 ));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_SecureNegotiate::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CFsm_SecureNegotiate::RunSM",
                 "%#x",
                 Fsm
                 ));

    ICSecureSocket * pSecureSocket = (ICSecureSocket *)Fsm->GetContext();
    CFsm_SecureNegotiate * stateMachine = (CFsm_SecureNegotiate *)Fsm;
    DWORD error;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pSecureSocket->SecureNegotiate_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::SecureNegotiate_Fsm(
    IN CFsm_SecureNegotiate * Fsm
    )
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::SecureNegotiate_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_SecureNegotiate & fsm = *Fsm;
    DWORD error = fsm.GetError();
    DWORD dwSSPIFlags = 0;

    if (fsm.GetState() != FSM_STATE_INIT) {
        switch (fsm.GetFunctionState()) {
        case FSM_STATE_2:
            goto send_continue;

        case FSM_STATE_3:
            goto negotiate_loop_continue;

        default:
            error = ERROR_INTERNET_INTERNAL_ERROR;

            INET_ASSERT(FALSE);

            goto quit;
        }
    }

    INET_ASSERT(IsOpen());

    *fsm.m_lpbAttemptReconnect = FALSE;

    //
    // set OutBuffer for InitializeSecurityContext call
    //

    fsm.m_OutBuffer.cBuffers = 1;
    fsm.m_OutBuffer.pBuffers = fsm.m_OutBuffers;
    fsm.m_OutBuffer.ulVersion = SECBUFFER_VERSION;

    if(GlobalSecureProtocols != GlobalSecureProtocolsCopy)
    {
        LOCK_SECURITY();
        //ReInit the credentials if our settings have changed.
        SecurityPkgInitialize();
        UNLOCK_SECURITY();
    }

    //
    // Pick the provider we're going to use.
    //

    while ((SecProviders[GetProviderIndex()].pszName != NULL)
           && ( !SecProviders[GetProviderIndex()].fEnabled
             || !(SecProviders[GetProviderIndex()].dwProtocolFlags & GlobalSecureProtocols) ) ) {

        //
        // Next provider
        //

        SetProviderIndex(GetProviderIndex() + 1);
    }

    if (SecProviders[GetProviderIndex()].pszName == NULL) {

        //
        // BUGBUG shouldn't we error out here?
        //

        SetProviderIndex(0);
        goto error_exit;
    }

    DWORD i;

    i = GetProviderIndex();

    DEBUG_PRINT(API,
                INFO,
                ("Starting handshake protocol with pkg %d - %s\n",
                i,
                SecProviders[i].pszName
                ));


    //
    // 1. initiate a client HELLO message and generate a token
    //

    fsm.m_OutBuffers[0].pvBuffer = NULL;
    fsm.m_OutBuffers[0].BufferType = SECBUFFER_TOKEN;

    SECURITY_STATUS scRet;
    DWORD ContextAttr;
    TimeStamp tsExpiry;

    fsm.m_bDoingClientAuth = FALSE;

    // Resynchronize the certificate store to catch
    // recently installed certificates
    if(g_bOpenMyCertStore && g_hMyCertStore == NULL)
        ReopenMyCertStore();
    if (g_hMyCertStore)
        CertControlStore(g_hMyCertStore, 0, CERT_STORE_CTRL_AUTO_RESYNC, NULL);

    //
    // We need a credential handle,
    //  if we're doing client do the magic to get a specialized
    //  one otherwise use the standard global one.
    //

    if ( IsCredClear(fsm.m_hCreds) )
    {
        fsm.m_hCreds = SecProviders[i].hCreds;

        if (GetCertContextArray())
        {
            if (GetCertContextArray()->GetSelectedCertContext())
            {
                error = CliAuthSelectCredential(
                            &m_hContext,
                            SecProviders[i].pszName,
                            GetCertContextArray(),
                            &fsm.m_hCreds);

                if (error != ERROR_SUCCESS) {
                    goto quit;
                }

                fsm.m_bDoingClientAuth = TRUE;
            }

            dwSSPIFlags |= ISC_REQ_USE_SUPPLIED_CREDS;
        }
    }

    scRet = g_InitializeSecurityContext(&fsm.m_hCreds,
                                        NULL,
                                        (LPSTR)GetHostName(),
                                        ISC_REQ_SEQUENCE_DETECT
                                        | ISC_REQ_REPLAY_DETECT
                                        | ISC_REQ_CONFIDENTIALITY
                                        | ISC_REQ_ALLOCATE_MEMORY
                                        | dwSSPIFlags,
                                        0,
                                        SECURITY_NATIVE_DREP,
                                        NULL,       // default, don't do hack.
                                        0,
                                        &m_hContext,
                                        &fsm.m_OutBuffer, // address where output data go
                                        &ContextAttr,
                                        &tsExpiry
                                        );

    DEBUG_PRINT(API,
                INFO,
                ("1. InitializeSecurityContext returned %s [%x]. hContext = %#x:%#x\n",
                InternetMapSSPIError((DWORD)scRet),
                scRet,
                m_hContext.dwUpper,
                m_hContext.dwLower
                ));

    if (scRet == SEC_E_INVALID_HANDLE) {
         SecProviders[i].fEnabled = FALSE;
    }
    if (scRet == SEC_E_INVALID_TOKEN) {
        error = ERROR_INTERNET_CANNOT_CONNECT;
    } else {

        //
        // Turn the error in to one we understand */
        //

        error = MapInternetError((DWORD)scRet);
    }
    if (scRet != SEC_I_CONTINUE_NEEDED) {
        goto error_exit;
    }

    DEBUG_PRINT(API,
                INFO,
                ("1. OutBuffer is <%x, %d, %x>\n",
                fsm.m_OutBuffers[0].pvBuffer,
                fsm.m_OutBuffers[0].cbBuffer,
                fsm.m_OutBuffers[0].BufferType
                ));

    if ((fsm.m_OutBuffers[0].cbBuffer != 0)
    && (fsm.m_OutBuffers[0].pvBuffer != NULL)) {

        //
        // Send response to server if there is one
        //

        fsm.SetFunctionState(FSM_STATE_2);
        error = ICSocket::Send(fsm.m_OutBuffers[0].pvBuffer,
                               fsm.m_OutBuffers[0].cbBuffer,
                               0
                               );
        if (error == ERROR_IO_PENDING) {
            goto quit;
        }

send_continue:

        g_FreeContextBuffer(fsm.m_OutBuffers[0].pvBuffer);
        fsm.m_OutBuffers[0].pvBuffer = NULL;
        if (error != ERROR_SUCCESS) {

            //
            // We should deal with this better
            //

            goto error_exit;
        }
    }

    fsm.SetFunctionState(FSM_STATE_3);
    error = SSPINegotiateLoop(NULL, fsm.m_dwFlags, fsm.m_hCreds, TRUE, fsm.m_bDoingClientAuth);

    //
    // We're not actually deleting the handle, rather we're no longer keeping
    //  a reference to the Credential handle in our fsm after we hand it off
    //

    if ( fsm.m_bDoingClientAuth )
    {
        ClearCreds(fsm.m_hCreds);
        fsm.m_bDoingClientAuth = FALSE;
    }
    if (error == ERROR_IO_PENDING) {
        goto quit;
    }

negotiate_loop_continue:
error_exit:

    if (error == ERROR_INTERNET_CANNOT_CONNECT) {

        //
        // error was a CANNOT_CONNECT, so try the next protocol.
        //

        SetProviderIndex(GetProviderIndex() + 1);

        if (SecProviders[GetProviderIndex()].pszName == NULL) {
            SetProviderIndex(0);
            *fsm.m_lpbAttemptReconnect = FALSE;
        } else {
            *fsm.m_lpbAttemptReconnect = TRUE;
        }
    }

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::SSPINegotiateLoop(
    OUT DBLBUFFER * pdblbufBuffer,
    IN DWORD dwFlags,
    CredHandle hCreds,
    IN BOOL bDoInitialRead,
    IN BOOL bDoingClientAuth
    )

/*++

Routine Description:

    This function completes the handshakes needed to establish a
    security protocol.  The initial handshakes are either generated
    by NegotiateSecureConnection, when generating a new connection, or
    during a receive when a REDO request is received.

Arguments:

    pdblbufBuffer - an input buffer into which to put any Extra data left over
                    after the handshake.  This data is assumed to be application
                    data, and will be decrypted later.

    dwFlags    - Socket Flags that may need to be passed on to Socket Calls
                (needed to support Async I/O)

    bDoInitialRead - if TRUE, this function will do a read before calling
                     InitializeSecurityContext, otherwise, it passes in 0 bytes of data.

Return Value:

    ERROR_SUCCESS - we successfully completed our connection.
    ERROR_INTERNET_CANNOT_CONNECT - The connection was dropped on us, possibly because we used a bad
                                    protocol.  Try the next protocol.

    ERROR_*                       - Other internet error, disconnect.


Comments:

    BUGBUG (hack alert) [arthurbi]
    Do to a bug in IIS 1.0 Servers we cannot connect because
    we send a "Client SSL 3 Message".  This message confuses the
    server and causes it to close the socket.  The fix is to
    reopen the socket and send a "Client SSL 2 Message."  Newer
    versions of the server will be fixed.


--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::SSPINegotiateLoop",
                 "{%#x [%#x]} %#x, %#x, %B",
                 this,
                 m_Socket,
                 pdblbufBuffer,
                 dwFlags,
                 bDoInitialRead
                 ));

    INET_ASSERT(IsSecure());

    DWORD error = DoFsm(new CFsm_NegotiateLoop(pdblbufBuffer,
                                               dwFlags,
                                               bDoInitialRead,
                                               bDoingClientAuth,
                                               hCreds,
                                               this
                                               ));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_NegotiateLoop::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CFsm_NegotiateLoop::RunSM",
                 "%#x",
                 Fsm
                 ));

    ICSecureSocket * pSecureSocket = (ICSecureSocket *)Fsm->GetContext();
    CFsm_NegotiateLoop * stateMachine = (CFsm_NegotiateLoop *)Fsm;
    DWORD error;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pSecureSocket->NegotiateLoop_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::NegotiateLoop_Fsm(
    IN CFsm_NegotiateLoop * Fsm
    )
{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::NegotiateLoop_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_NegotiateLoop & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if (fsm.GetState() != FSM_STATE_INIT) {
        switch (fsm.GetFunctionState()) {
        case FSM_STATE_2:
            goto receive_continue;

        case FSM_STATE_3:
            goto send_continue;

        default:
            error = ERROR_INTERNET_INTERNAL_ERROR;

            INET_ASSERT(FALSE);

            goto quit;
        }
    }

    INET_ASSERT(IsOpen());

    fsm.m_dwProviderIndex = GetProviderIndex();

    fsm.m_dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT
                      | ISC_REQ_REPLAY_DETECT
                      | ISC_REQ_CONFIDENTIALITY
                      | ISC_REQ_ALLOCATE_MEMORY
                      | ISC_RET_EXTENDED_ERROR;

    //
    // set OutBuffer for InitializeSecurityContext call
    //

    fsm.m_OutBuffer.cBuffers = 1;
    fsm.m_OutBuffer.pBuffers = fsm.m_OutBuffers;
    fsm.m_OutBuffer.ulVersion = SECBUFFER_VERSION;

    //
    // If we have a selected cert chain, then try to
    // generate a credential from that list.
    //

    if (IsCredClear(fsm.m_hCreds))
    {
        fsm.m_hCreds = SecProviders[fsm.m_dwProviderIndex].hCreds;

        if ( GetCertContextArray() &&
             GetCertContextArray()->GetSelectedCertContext() )
        {
            error = CliAuthSelectCredential(
                        &m_hContext,
                        SecProviders[fsm.m_dwProviderIndex].pszName,
                        GetCertContextArray(),
                        &fsm.m_hCreds);

            if (error != ERROR_SUCCESS) {
                goto quit;
            }

            fsm.m_bDoingClientAuth = TRUE;
        }
    }

    if (fsm.m_bDoingClientAuth ||
        GetCertContextArray() )
    {
        fsm.m_dwSSPIFlags |= ISC_REQ_USE_SUPPLIED_CREDS;
    }

    fsm.m_scRet = SEC_I_CONTINUE_NEEDED;

    while (fsm.m_scRet == SEC_I_CONTINUE_NEEDED ||
           fsm.m_scRet == SEC_E_INCOMPLETE_MESSAGE ||
           fsm.m_scRet == SEC_I_INCOMPLETE_CREDENTIALS) {

        //
        //  send to target server
        //  if we've got a SEC_E_INCOMPLETE_MESSAGE we need to do a read
        //  again because we didn't get the entire message we expected from
        //  the server.
        //


        //
        //  receive response from server and pass token into security pkg
        //    BUT only if we haven't already received extra data
        //    from SSPI which we need to process in lu of actual data
        //    data from WinSock, and if the package has not returned
        //    one of the defined warnings that indicates that we should
        //    pass the previous buffer again.
        //


        // Make sure fsm.m_lpszBuffer holds the input data to be passed
        // to initialize security context.  There are 4 cases:
        // 1) We have Extra Data, so we don't need to do a socket receive
        // 2) We were called during a re-negotiate, so if this is the first
        //    time through the loop, we have 0 bytes.
        // 3) We are recovering from a SEC_I_INCOMPLETE_CREDENTIALS, so
        //    use the same buffer again.
        // 4) We do a SocketReceive
        // We'll indicate 1 and 3 by having the fsm.m_dwBytesReceived count being the number of bytes
        // left in the buffer to be re-sent or sent to the sspi call.
        // If bytes-received is zero, then either we are doing a Redo, or we need to receive
        // data.  fsm.m_bDoRead let's us know if for some reason we should do or not do this read



        if ((0 == fsm.m_dwBytesReceived) || (fsm.m_scRet == SEC_E_INCOMPLETE_MESSAGE)) {
            if (fsm.m_bDoRead) {
                fsm.SetFunctionState(FSM_STATE_2);
                error = ICSocket::Receive((LPVOID *)&fsm.m_lpszBuffer,
                                          &fsm.m_dwBufferLength,
                                          &fsm.m_dwBufferLeft,
                                          &fsm.m_dwBytesReceived,
                                          0,
                                          SF_EXPAND,
                                          &fsm.m_bEofReceive
                                          );
                if (error == ERROR_IO_PENDING) {
                    goto done;
                }

receive_continue:

                if ((error != ERROR_SUCCESS) || fsm.m_bEofReceive) {

                    DEBUG_PRINT(API,
                                ERROR,
                                ("SocketReceive failed\n"
                                ));

                    if (error == ERROR_SUCCESS) {
                        error = ERROR_INTERNET_CANNOT_CONNECT;
                    }
                    break;
                }
            } else {
                fsm.m_bDoRead = TRUE;
            }
        }

        if (fsm.m_scRet == SEC_I_INCOMPLETE_CREDENTIALS) {

            CERT_CONTEXT_ARRAY* pCertContextArray;

            //
            // If've already done Client Auth, and it fails again
            //  then we fail.
            //

            if (fsm.m_bDoingClientAuth) {
                error = ERROR_CANCELLED;
                goto quit;
            }

            //
            // If we don't already have a cert chain list,
            // then get one, and make our selection
            //

            INET_ASSERT(!GetCertContextArray());

            pCertContextArray = NULL;

            //delete pCertChainList;
            //SetCertChainList(NULL);

            error = CliAuthAcquireCertContexts(
                        &m_hContext,
                        SecProviders[fsm.m_dwProviderIndex].pszName,
                        &pCertContextArray
                        );

            SetCertContextArray(pCertContextArray);

            if (error == ERROR_SUCCESS) {
                error = ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED;
            }

            fsm.m_scRet = error;
            break;
        }

        //
        // InBuffers[1] is for getting extra data that
        //  SSPI/SCHANNEL doesn't proccess on this
        //  run around the loop.
        //

        fsm.m_InBuffers[0].pvBuffer   = fsm.m_lpszBuffer;
        fsm.m_InBuffers[0].cbBuffer   = fsm.m_dwBytesReceived;
        fsm.m_InBuffers[0].BufferType = SECBUFFER_TOKEN;

        fsm.m_InBuffers[1].pvBuffer   = NULL;
        fsm.m_InBuffers[1].cbBuffer   = 0;
        fsm.m_InBuffers[1].BufferType = SECBUFFER_EMPTY;

        //
        // Initialize these so if we fail, pvBuffer contains NULL,
        // so we don't try to free random garbage at the quit
        //

        fsm.m_OutBuffers[0].pvBuffer   = NULL;
        fsm.m_OutBuffers[0].BufferType = SECBUFFER_TOKEN;
        fsm.m_OutBuffers[0].cbBuffer   = 0;

        SecBufferDesc InBuffer;

        InBuffer.cBuffers        = 2;
        InBuffer.pBuffers        = fsm.m_InBuffers;
        InBuffer.ulVersion       = SECBUFFER_VERSION;

        DWORD ContextAttr;
        TimeStamp tsExpiry;

        fsm.m_scRet = g_InitializeSecurityContext(&fsm.m_hCreds,
                                                  &m_hContext,
                                                  NULL,
                                                  fsm.m_dwSSPIFlags,
                                                  0,
                                                  SECURITY_NATIVE_DREP,
                                                  &InBuffer,
                                                  0,
                                                  NULL,
                                                  &fsm.m_OutBuffer,
                                                  &ContextAttr,
                                                  &tsExpiry
                                                  );

        DEBUG_PRINT(API,
                    INFO,
                    ("3. InitializeSecurityContext returned %s [%x]\n",
                    InternetMapSSPIError((DWORD)fsm.m_scRet),
                    fsm.m_scRet
                    ));

        if (fsm.m_scRet == STATUS_SUCCESS ||
             fsm.m_scRet == SEC_I_CONTINUE_NEEDED ||
             (FAILED(fsm.m_scRet) && (0 != (ContextAttr & ISC_RET_EXTENDED_ERROR))))
        {
            if  (fsm.m_OutBuffers[0].cbBuffer != 0    &&
                 fsm.m_OutBuffers[0].pvBuffer != NULL )
            {

                //
                // Send response to server if there is one
                //

                fsm.SetFunctionState(FSM_STATE_3);
                error = ICSocket::Send(fsm.m_OutBuffers[0].pvBuffer,
                                       fsm.m_OutBuffers[0].cbBuffer,
                                       0
                                       );
                if (error == ERROR_IO_PENDING) {
                    goto done;
                }

send_continue:

                g_FreeContextBuffer(fsm.m_OutBuffers[0].pvBuffer);
                fsm.m_OutBuffers[0].pvBuffer = NULL;
            }
        }


        if ( fsm.m_scRet == STATUS_SUCCESS )
        {
            DEBUG_PRINT(API,
                     INFO,
                    ("NegotiateSecConnection succeeded.\n"));


            if (fsm.m_pdblbufBuffer)
            {
                if ( fsm.m_InBuffers[1].BufferType == SECBUFFER_EXTRA )
                {

                    fsm.m_pdblbufBuffer->CopyIn(
                        (LPBYTE) (fsm.m_lpszBuffer + (fsm.m_dwBytesReceived - fsm.m_InBuffers[1].cbBuffer)),
                        fsm.m_InBuffers[1].cbBuffer
                        );

                }
                else
                {
                    fsm.m_pdblbufBuffer->SetInputBufferSize(0);
                }
            }



            //
            // Bail out to quit
            //

            break;
        }
        else if (FAILED(fsm.m_scRet) && (fsm.m_scRet != SEC_E_INCOMPLETE_MESSAGE))
        {

             //
             //  free security context handle and delete the local
             //  data structures associated with the handle and
             //  try another pkg if available
             //

             DEBUG_PRINT(API,
                         INFO,
                         ("3. InitializeSecurityContext failed, %lx\n",
                         fsm.m_scRet
                         ));


             // Turn the error in to one we understand */
             error = MapInternetError((DWORD)fsm.m_scRet);

             TerminateSecConnection();
             /* Break out to try next protocol */
             break;
        }

        if ((fsm.m_scRet != SEC_E_INCOMPLETE_MESSAGE)
        && (fsm.m_scRet != SEC_I_INCOMPLETE_CREDENTIALS)) {

            DEBUG_PRINT(API,
                        INFO,
                        ("3. OutBuffer is <%x, %d, %x>\n",
                        fsm.m_OutBuffers[0].pvBuffer,
                        fsm.m_OutBuffers[0].cbBuffer,
                        fsm.m_OutBuffers[0].BufferType
                        ));

            if (fsm.m_InBuffers[1].BufferType == SECBUFFER_EXTRA) {

                //
                // skip next recv and set up buffers
                //  so InitalizeSecurityContext pulls its
                //  info from the Extra it returned previously.
                //

                DEBUG_PRINT(API,
                         INFO,
                         ("Got SECBUFFER_EXTRA, moving %d bytes to front of buffer\n",
                         fsm.m_InBuffers[1].cbBuffer
                         ));

                INET_ASSERT(fsm.m_InBuffers[1].cbBuffer > 0);

                MoveMemory(
                        fsm.m_lpszBuffer,             // dest
                        fsm.m_lpszBuffer + (fsm.m_dwBytesReceived - fsm.m_InBuffers[1].cbBuffer),
                        fsm.m_InBuffers[1].cbBuffer   // size
                        );

                fsm.m_dwBytesReceived = fsm.m_InBuffers[1].cbBuffer;
                fsm.m_dwBufferLeft   = fsm.m_dwBufferLength - fsm.m_dwBytesReceived;
            } else {

                //
                // prepare for next receive
                //

                fsm.m_dwBufferLeft = fsm.m_dwBufferLength;
                fsm.m_dwBytesReceived = 0;
            }
        }
    }

quit:

    if (fsm.m_lpszBuffer != NULL) {
         fsm.m_lpszBuffer = (LPSTR)FREE_MEMORY(fsm.m_lpszBuffer);
         INET_ASSERT(fsm.m_lpszBuffer == NULL);
    }

done:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::Disconnect(
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Undoes the work of ConnectSocket - i.e. closes a connected socket. We make
    callbacks to inform the app that this socket is being closed

Arguments:

    dwFlags - controlling operation

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::Disconnect",
                 "{%#x} %#x",
                 m_Socket,
                 dwFlags
                 ));

    INET_ASSERT(IsSecure());

    DWORD error = ICSocket::Disconnect(dwFlags);

    //
    // delete security context handle for the connection
    //

    if ((m_dwFlags & (SF_ENCRYPT | SF_DECRYPT))
    && dwEncFlags != ENC_CAPS_NOT_INSTALLED) {
        TerminateSecConnection();
    }

    //
    // Zero out the pending input buffer
    //

    if (m_pdblbufBuffer != NULL) {
        m_pdblbufBuffer->SetInputBufferSize(0);
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::Send(
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Sends data over a secure connection

Arguments:

    lpBuffer        - pointer to user data to send

    dwBufferLength  - length of user data

    dwFlags         - flags controlling operation

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::Send",
                 "{%#x [%#x]} %#x, %d, %#x",
                 this,
                 m_Socket,
                 lpBuffer,
                 dwBufferLength,
                 dwFlags
                 ));

    INET_ASSERT(lpBuffer != NULL);
    INET_ASSERT((int)dwBufferLength > 0);
    INET_ASSERT(IsSecure());

    DWORD error = DoFsm(new CFsm_SecureSend(lpBuffer,
                                            dwBufferLength,
                                            dwFlags,
                                            this
                                            ));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_SecureSend::RunSM(
    IN CFsm * Fsm
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    Fsm -

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CFsm_SecureSend::RunSM",
                 "%#x",
                 Fsm
                 ));

    ICSecureSocket * pSecureSocket = (ICSecureSocket *)Fsm->GetContext();
    CFsm_SecureSend * stateMachine = (CFsm_SecureSend *)Fsm;
    DWORD error;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pSecureSocket->Send_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::Send_Fsm(
    IN CFsm_SecureSend * Fsm
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    Fsm -

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::Send_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_SecureSend & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if (fsm.GetState() == FSM_STATE_INIT) {

        //
        // Log The Data BEFORE we Encrypt It ( if we do )
        //

        DEBUG_DUMP_API(SOCKETS,
                       "sending data:\n",
                       fsm.m_lpBuffer,
                       fsm.m_dwBufferLength
                       );

    }

    while (((int)fsm.m_dwBufferLength > 0) && (error == ERROR_SUCCESS)) {

        LPVOID lpBuffer;
        DWORD dwLength;
        DWORD dwBytes;

        if (m_dwFlags & SF_ENCRYPT) {

            DWORD dwBytesEncrypted;

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("Encrypting data..\n"
                        ));

            error = EncryptData(fsm.m_lpBuffer,
                                fsm.m_dwBufferLength,
                                &fsm.m_lpCryptBuffer,
                                &fsm.m_dwCryptBufferLength,
                                &dwBytesEncrypted
                                );
            if (error != ERROR_SUCCESS) {
                break;
            }

            INET_ASSERT(fsm.m_lpCryptBuffer != NULL);
            INET_ASSERT((int)fsm.m_dwCryptBufferLength > 0);
            INET_ASSERT(dwBytesEncrypted <= fsm.m_dwBufferLength);

            lpBuffer = fsm.m_lpCryptBuffer;
            dwLength = fsm.m_dwCryptBufferLength;
            dwBytes = dwBytesEncrypted;
        } else {
            lpBuffer = fsm.m_lpBuffer;
            dwLength = fsm.m_dwBufferLength;
            dwBytes = dwLength;
        }

        fsm.m_lpBuffer = (LPVOID)((LPBYTE)fsm.m_lpBuffer + dwBytes);
        fsm.m_dwBufferLength -= dwBytes;

        error = ICSocket::Send(lpBuffer, dwLength, fsm.m_dwFlags);
        if (error != ERROR_SUCCESS) {
            break;
        }
    }

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();

        //
        // Free Encryption Buffer if doing SSL/PCT
        //

        if (fsm.m_lpCryptBuffer != NULL ) {
            fsm.m_lpCryptBuffer = (LPVOID)FREE_MEMORY(fsm.m_lpCryptBuffer);
            INET_ASSERT(fsm.m_lpCryptBuffer == NULL);
        }
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::Receive(
    IN OUT LPVOID* lplpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwBufferRemaining,
    IN OUT LPDWORD lpdwBytesReceived,
    IN DWORD dwExtraSpace,
    IN DWORD dwFlags,
    OUT LPBOOL lpbEof
    )

/*++

Routine Description:

    Receives and decrypts data from a secure connection

Arguments:

    lplpBuffer          - see ICSocket::Receive
    lpdwBufferLength    -
    lpdwBufferRemaining -
    lpdwBytesReceived   -
    dwExtraSpace        -
    dwFlags             -
    lpbEof              -

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    INET_ASSERT(lplpBuffer != NULL);
    INET_ASSERT(lpdwBufferLength != NULL);
    INET_ASSERT((*lpdwBufferLength == 0) ? (dwFlags & SF_EXPAND) : TRUE);
    INET_ASSERT(IsSecure());

    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::Receive",
                 "%#x [%#x], %#x [%d], %#x [%d], %#x [%d], %d, %#x, %#x [%B]",
                 lplpBuffer,
                 *lplpBuffer,
                 lpdwBufferLength,
                 *lpdwBufferLength,
                 lpdwBufferRemaining,
                 *lpdwBufferRemaining,
                 lpdwBytesReceived,
                 *lpdwBytesReceived,
                 dwExtraSpace,
                 dwFlags,
                 lpbEof,
                 *lpbEof
                 ));

    DWORD error = DoFsm(new CFsm_SecureReceive(lplpBuffer,
                                               lpdwBufferLength,
                                               lpdwBufferRemaining,
                                               lpdwBytesReceived,
                                               dwExtraSpace,
                                               dwFlags,
                                               lpbEof,
                                               this
                                               ));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_SecureReceive::RunSM(
    IN CFsm * Fsm
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    Fsm -

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CFsm_SecureReceive::RunSM",
                 "%#x",
                 Fsm
                 ));

    ICSecureSocket * pSecureSocket = (ICSecureSocket *)Fsm->GetContext();
    CFsm_SecureReceive * stateMachine = (CFsm_SecureReceive *)Fsm;
    DWORD error;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pSecureSocket->Receive_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::Receive_Fsm(
    IN CFsm_SecureReceive * Fsm
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    Fsm -

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::Receive_Fsm",
                 "%#x",
                 Fsm
                 ));

    //INET_ASSERT(m_dwFlags & SF_DECRYPT);

    CFsm_SecureReceive & fsm = *Fsm;
    DWORD error = fsm.GetError();
    LPVOID * lplpBuffer;
    LPDWORD lpdwBufferLength;
    LPDWORD lpdwBufferLeft;
    LPDWORD lpdwBytesReceived;

    if (fsm.GetState() != FSM_STATE_INIT) {
        switch (fsm.GetFunctionState()) {
        case FSM_STATE_2:
            goto negotiate_continue;

        case FSM_STATE_3:
            goto receive_continue;

        default:
            error = ERROR_INTERNET_INTERNAL_ERROR;

            INET_ASSERT(FALSE);

            goto quit;
        }
    }

    //
    // if we weren't given a buffer, but the caller told us its okay to resize
    // then we allocate the initial buffer
    //

    if ((fsm.m_dwBufferLength == 0) || (fsm.m_dwBufferLeft == 0)) {

        INET_ASSERT((fsm.m_dwBufferLength == 0) ? (fsm.m_dwBufferLeft == 0) : TRUE);

        if (fsm.m_dwFlags & SF_EXPAND) {

            //
            // allocate a fixed memory buffer
            //

            //
            // BUGBUG - the initial buffer size should come from the handle
            //          object
            //

            fsm.m_dwBufferLeft = DEFAULT_RECEIVE_BUFFER_INCREMENT;
            if (fsm.m_dwBufferLength == 0) {
                fsm.m_bAllocated = TRUE;
            }
            fsm.m_dwBufferLength += fsm.m_dwBufferLeft;

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("resizing %#x to %d\n",
                        fsm.m_hBuffer,
                        fsm.m_dwBufferLength
                        ));

            fsm.m_hBuffer = ResizeBuffer(fsm.m_hBuffer,
                                         fsm.m_dwBufferLength,
                                         FALSE);
            if (fsm.m_hBuffer == (HLOCAL)NULL) {
                error = GetLastError();

                INET_ASSERT(error != ERROR_SUCCESS);

                fsm.m_bAllocated = FALSE;
            }
        } else {

            //
            // the caller didn't say its okay to resize
            //

            error = ERROR_INSUFFICIENT_BUFFER;
        }
    } else if (fsm.m_hBuffer == (HLOCAL)NULL) {
        error = ERROR_INSUFFICIENT_BUFFER;
    }
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // keep the app informed (if requested to do so)
    //

    if (fsm.m_dwFlags & SF_INDICATE) {
        InternetIndicateStatus(INTERNET_STATUS_RECEIVING_RESPONSE,
                               NULL,
                               0
                               );
    }

    fsm.m_dwReadFlags = fsm.m_dwFlags;

    //
    // Loop Through our Reads, assembling enough unencrypted bytes
    //  to return back to the client.  In the non-SSL/PCT case, we should
    //  be able to quit after one iteration.
    //

    do {

        LPVOID * lplpReadBuffer;
        LPDWORD lpdwReadBufferLength;
        LPDWORD lpdwReadBufferLeft;
        LPDWORD lpdwReadBufferReceived;

        //
        // If we're attempting to read SSL/PCT data, we need examine, whether
        // we have all the bytes decrypted and read already in our scratch buffer.
        //

        if (m_dwFlags & SF_DECRYPT) {

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("Decrypting data..\n"
                        ));

            if (m_pdblbufBuffer != NULL) {

                DEBUG_DUMP_API(SOCKETS,
                               "About to decrypt this data:\n",
                               (LPBYTE)m_pdblbufBuffer->GetInputBufferPointer(),
                               m_pdblbufBuffer->GetInputBufferSize()
                               );

            }

            fsm.m_dwDecryptError = DecryptData(&fsm.m_dwInputBytesLeft,
                                               (LPBYTE)fsm.m_hBuffer,
                                               &fsm.m_dwBufferLeft,
                                               &fsm.m_dwBytesReceived,
                                               &fsm.m_dwBytesRead
                                               );

            if (fsm.m_dwDecryptError == SEC_E_INCOMPLETE_MESSAGE &&
                fsm.m_bEof &&
                m_pdblbufBuffer->GetInputBufferSize() > 0) {

                error = ERROR_HTTP_INVALID_SERVER_RESPONSE;
                goto error_exit;

            }
            else if (fsm.m_dwDecryptError == SEC_I_RENEGOTIATE) {

                CredHandle hDummyCreds;

                //
                // BUGBUG - don't have to do this - Receive() called from
                //          SSPINegotiateLoop() won't come back through here
                //

                m_dwFlags &= ~(SF_ENCRYPT | SF_DECRYPT);
                ClearCreds(hDummyCreds);

                fsm.SetFunctionState(FSM_STATE_2);
                error = SSPINegotiateLoop(m_pdblbufBuffer,
                                          fsm.m_dwFlags,
                                          hDummyCreds,
                                          FALSE,
                                          FALSE);
                if (error == ERROR_IO_PENDING) {
                    goto error_exit;
                }

negotiate_continue:

                m_dwFlags |= (SF_ENCRYPT | SF_DECRYPT);

                if (error != ERROR_SUCCESS) {
                    break;
                }

                fsm.m_dwDecryptError = (ULONG)SEC_E_INCOMPLETE_MESSAGE;

                //
                // If there was extra data, and it was shoved back into
                // dblbuffer, then we should redo the decryption, since
                // it now has extra input data to process.
                //

                if (m_pdblbufBuffer->GetInputBufferSize() > 0) {
                    continue;
                }

                //
                // Okay, here we've received 0 bytes, so so we have to
                // receive more data, and process it.  Do this by zero-ing
                // out the input buffer, and setting the decrypt_error to be
                // Incomplete.
                //

            }

            //
            // If we have no buffer left to fill, or the caller ask for a single recv
            // and we've managed to read something into the buffer, then return by breaking.
            //

            if ((fsm.m_dwBufferLeft == 0)
            || (!(fsm.m_dwFlags & SF_RECEIVE_ALL) && (fsm.m_dwBytesRead > 0))) {
                break;  // we're done.
            }

            INET_ASSERT(error == ERROR_SUCCESS);

            //
            // BUGBUG [arthurbi] GetInputBufferSize needs to be called before getting
            //   the pointer, because the pointer may be moved around while generating
            //   the size.
            //

            DWORD remaining;
            DWORD inputSize;

            inputSize = m_pdblbufBuffer->GetInputBufferSize();
            remaining = m_pdblbufBuffer->GetInputBufferRemaining();
            fsm.m_dwBufferLengthDummy = inputSize + remaining;
            fsm.m_dwBufferLeftDummy = remaining;
            fsm.m_dwBufferReceivedDummy = inputSize;
            fsm.m_lpBufferDummy = m_pdblbufBuffer->GetInputBufferPointer();

            //
            // We need to be careful, and only recv one block of data at a time
            // if we're not we break keep-alive by doing too many reads.
            //
            // So unless we know ( by the non-0 return ) exactly how many bytes
            // to read, we shut off SF_RECEIVE_ALL.
            //

            fsm.m_dwReadFlags &= ~(SF_RECEIVE_ALL
                                   | SF_INDICATE
                                   | SF_EXPAND
                                   | SF_COMPRESS
                                   );

            if (fsm.m_dwInputBytesLeft != 0) {

                //
                // don't add RECEIVE_ALL if NO_WAIT already set by caller - they
                // are mutually exclusive
                //

                if (!(fsm.m_dwReadFlags & SF_NO_WAIT)) {
                    fsm.m_dwReadFlags |= SF_RECEIVE_ALL;
                }
                fsm.m_dwBufferLeftDummy = min(fsm.m_dwInputBytesLeft,
                                              fsm.m_dwBufferLeftDummy);
            }
            lplpReadBuffer = (LPVOID *)&fsm.m_lpBufferDummy;
            lpdwReadBufferLength = &fsm.m_dwBufferLengthDummy;
            lpdwReadBufferLeft = &fsm.m_dwBufferLeftDummy;
            lpdwReadBufferReceived = &fsm.m_dwBufferReceivedDummy;
        } else {
            lplpReadBuffer = &fsm.m_hBuffer;
            lpdwReadBufferLength = &fsm.m_dwBufferLength;
            lpdwReadBufferLeft = &fsm.m_dwBufferLeft;
            lpdwReadBufferReceived = &fsm.m_dwBytesReceived;
        }

        //
        // receive some data, assuming the socket is not closed.
        //

        if (!fsm.m_bEof) {
            //fsm.m_dwBytesReceivedPre = *lpdwReadBufferReceived;
            fsm.SetFunctionState(FSM_STATE_3);
            error = ICSocket::Receive(lplpReadBuffer,
                                      lpdwReadBufferLength,
                                      lpdwReadBufferLeft,
                                      lpdwReadBufferReceived,
                                      fsm.m_dwExtraSpace,
                                      fsm.m_dwReadFlags,
                                      &fsm.m_bEof
                                      );
            if (error == ERROR_IO_PENDING) {
                goto error_exit;
            }

receive_continue:

            //fsm.m_dwBytesRead += fsm.m_dwByReceived - fsm.m_dwDCBufferRecvPre;
            if (error != ERROR_SUCCESS) {
                goto quit;
            }

            //
            // Once again, for SSL/PCT we need to update our input buffer after the read.
            //

            if (m_dwFlags & SF_DECRYPT) {
                m_pdblbufBuffer->SetInputBufferSize(fsm.m_dwBufferReceivedDummy);
            }
        }
    } while ((m_dwFlags & SF_DECRYPT)
             && (error == ERROR_SUCCESS)
             && (fsm.m_dwDecryptError == SEC_E_INCOMPLETE_MESSAGE)
             && (!fsm.m_bEof || (m_pdblbufBuffer->GetInputBufferSize() > 0)));

    if (error == ERROR_SUCCESS) {

        //
        // inform the app that we finished, and tell it how much we received
        // this time
        //

        if (fsm.m_dwFlags & SF_INDICATE) {
            InternetIndicateStatus(INTERNET_STATUS_RESPONSE_RECEIVED,
                                   &fsm.m_dwBytesRead,
                                   sizeof(fsm.m_dwBytesRead)
                                   );
        }

        //
        // if we received the entire response and the caller specified
        // SF_COMPRESS then we shrink the buffer to fit. We may end up growing
        // the buffer to contain dwExtraSpace if it is not zero and we just
        // happened to fill the current buffer
        //

        if (fsm.m_bEof && (fsm.m_dwFlags & SF_COMPRESS)) {

            fsm.m_dwBufferLeft = fsm.m_dwExtraSpace;

            //
            // include any extra that the caller required
            //

            fsm.m_dwBufferLength = fsm.m_dwBytesReceived + fsm.m_dwExtraSpace;

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("shrinking buffer %#x to %d (%#x) bytes (includes %d extra)\n",
                        fsm.m_hBuffer,
                        fsm.m_dwBufferLength,
                        fsm.m_dwBufferLength,
                        fsm.m_dwExtraSpace
                        ));

            fsm.m_hBuffer = ResizeBuffer(fsm.m_hBuffer,
                                         fsm.m_dwBufferLength,
                                         FALSE);

            INET_ASSERT((fsm.m_hBuffer == NULL)
                        ? ((fsm.m_dwBytesReceived + fsm.m_dwExtraSpace) == 0)
                        : TRUE
                        );

        }

        DEBUG_PRINT_API(SOCKETS,
                        INFO,
                        ("read %d bytes @ %#x from socket %#x\n",
                        fsm.m_dwBytesRead,
                        (LPBYTE)fsm.m_hBuffer + *fsm.m_lpdwBytesReceived,
                        m_Socket
                        ));

        DEBUG_DUMP_API(SOCKETS,
                       "received data:\n",
                       (LPBYTE)fsm.m_hBuffer + *fsm.m_lpdwBytesReceived,
                       fsm.m_dwBytesRead
                       );

    }

quit:

    //
    // if we failed but allocated a buffer then we need to free it (we were
    // leaking this buffer if the request was cancelled)
    //

    if ((error != ERROR_SUCCESS) && fsm.m_bAllocated && (fsm.m_hBuffer != NULL)) {
//dprintf("SocketReceive() freeing allocated buffer %#x\n", hBuffer);
        fsm.m_hBuffer = (HLOCAL)FREE_MEMORY(fsm.m_hBuffer);

        INET_ASSERT(fsm.m_hBuffer == NULL);

        fsm.m_dwBufferLength = 0;
        fsm.m_dwBufferLeft = 0;
        fsm.m_dwBytesReceived = 0;
        fsm.m_bEof = TRUE;
    }

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("returning: lpBuffer=%#x, bufferLength=%d, bufferLeft=%d, bytesReceived=%d\n",
                fsm.m_hBuffer,
                fsm.m_dwBufferLength,
                fsm.m_dwBufferLeft,
                fsm.m_dwBytesReceived
                ));

    //
    // update output parameters
    //

    *fsm.m_lplpBuffer = (LPVOID)fsm.m_hBuffer;
    *fsm.m_lpdwBufferLength = fsm.m_dwBufferLength;
    *fsm.m_lpdwBufferRemaining = fsm.m_dwBufferLeft;
    *fsm.m_lpdwBytesReceived = fsm.m_dwBytesReceived;

    //
    // Hack, we hide eof's from caller, since we may have buffered data sitting around
    //

    if ((m_dwFlags & SF_DECRYPT) && (fsm.m_dwBytesRead != 0)) {
        fsm.m_bEof = FALSE;
    }

    *fsm.m_lpbEof = fsm.m_bEof;

    //
    // map any sockets error to WinInet error
    //

    if (error != ERROR_SUCCESS) {
        error = MapInternetError(error);
    }

error_exit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSecureSocket::SetHostName(
    IN LPSTR lpszHostName
    )

/*++

Routine Description:

    Set name of server we are connected to. Find or create a security cache
    entry for this name

Arguments:

    lpszHostName    - name to set

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::SetHostName",
                 "{%#x [%q %#x/%d]} %q",
                 this,
                 m_lpszHostName,
                 GetSocket(),
                 GetSourcePort(),
                 lpszHostName
                 ));

    INET_ASSERT(IsSecure());
    INET_ASSERT((lpszHostName != NULL) || (m_lpszHostName == NULL));

    DWORD error = ERROR_SUCCESS;

    if (lpszHostName != NULL) {
        if (m_lpszHostName != NULL) {
            m_lpszHostName = (LPSTR)FREE_MEMORY(m_lpszHostName);

            INET_ASSERT(m_lpszHostName == NULL);

        }
        m_lpszHostName = NewString(lpszHostName);
        if (m_lpszHostName == NULL) {
            error = ERROR_NOT_ENOUGH_MEMORY;
        } else if (m_pSecurityInfo == NULL) {
            /* SCLE ref */
            m_pSecurityInfo = GlobalCertCache.Find(lpszHostName);
            if (m_pSecurityInfo == NULL) {
                /* SCLE ref */
                m_pSecurityInfo = new SECURITY_CACHE_LIST_ENTRY(lpszHostName);
            }
        }
    }

    DEBUG_LEAVE(error);

    return error;
}

//
// private ICSecureSocket methods
//


DWORD
ICSecureSocket::EncryptData(
    IN LPVOID lpBuffer,
    IN DWORD dwInBufferLen,
    OUT LPVOID * lplpBuffer,
    OUT LPDWORD lpdwOutBufferLen,
    OUT LPDWORD lpdwInBufferBytesEncrypted
    )

/*++

Routine Description:

    This function encrypts data in the lplpbuffer.

Arguments:

    lpBuffer         - pointer to buffer containing unencrypted user data

    dwInBufferLen    - length of input buffer

    lplpBuffer       - pointer to pointer to encrypted user buffer

    lpdwOutBufferLen - pointer to length of output lplpbuffer

    lpdwInBufferBytesEncrypted - pointer to length of bytes read and encrypted in output buffer

Return Value:

    Error Code

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::EncryptData",
                 "%#x, %d, %#x, %#x, %#x",
                 lpBuffer,
                 dwInBufferLen,
                 lplpBuffer,
                 lpdwOutBufferLen,
                 lpdwInBufferBytesEncrypted
                 ));

    SECURITY_STATUS scRet = STATUS_SUCCESS;
    SecBufferDesc Buffer;
    SecBuffer Buffers[3];
    HLOCAL hBuffer;
    DWORD error;
    DWORD dwMaxDataBufferSize;
    DWORD dwExtraInputBufferLen;
    SecPkgContext_StreamSizes Sizes;

    INET_ASSERT(IsSecure());
    INET_ASSERT(lpBuffer != NULL);
    INET_ASSERT(dwInBufferLen != 0);
    INET_ASSERT(lplpBuffer != NULL);
    INET_ASSERT(lpdwOutBufferLen != NULL);
    INET_ASSERT(lpdwInBufferBytesEncrypted != NULL);

    hBuffer = (HLOCAL) *lplpBuffer;
    *lpdwOutBufferLen = 0;
    *lpdwInBufferBytesEncrypted = 0;

    //INET_ASSERT(hBuffer == NULL );

    //
    //  find the header and trailer sizes
    //

    scRet = g_QueryContextAttributes(&m_hContext,
                                     SECPKG_ATTR_STREAM_SIZES,
                                     &Sizes );
    if (scRet != ERROR_SUCCESS) {

        //
        // Map the SSPI error.
        //

        DEBUG_PRINT(API,
                    INFO,
                    ("QueryContextAttributes returned, %s [%x] (%s)\n",
                    InternetMapSSPIError((DWORD)scRet),
                    scRet,
                    InternetMapError(scRet)
                    ));

        error = MapInternetError((DWORD) scRet);
        goto quit;
    } else {

        DEBUG_PRINT(API,
                    INFO,
                    ("QueryContextAttributes returned header=%d, trailer=%d, maxmessage=%d\n",
                    Sizes.cbHeader,
                    Sizes.cbTrailer,
                    Sizes.cbMaximumMessage
                    ));
    }

    INET_ASSERT(Sizes.cbMaximumMessage > (Sizes.cbHeader + Sizes.cbTrailer));

    //
    // Figure out the max SSL packet we can send over the wire.
    //  If the data is too big to send, then remeber how much
    //  we did send, and how much we didn't send.
    //

    dwMaxDataBufferSize = Sizes.cbMaximumMessage - (Sizes.cbHeader + Sizes.cbTrailer);

    dwExtraInputBufferLen =
            (dwMaxDataBufferSize < dwInBufferLen ) ?
                (dwInBufferLen - dwMaxDataBufferSize) : 0;

    dwInBufferLen =
            ( dwExtraInputBufferLen > 0 ) ?
            dwMaxDataBufferSize :
            dwInBufferLen;

    DEBUG_PRINT(API,
                INFO,
                ("resizing %#x to %d\n",
                hBuffer,
                dwInBufferLen + Sizes.cbHeader + Sizes.cbTrailer
                ));

    hBuffer = ResizeBuffer(hBuffer,
                           dwInBufferLen + Sizes.cbHeader + Sizes.cbTrailer,
                           FALSE );

    if (hBuffer == (HLOCAL)NULL) {
        error = GetLastError();

        INET_ASSERT(error != ERROR_SUCCESS);

        goto quit;
    }

    //
    // prepare data for SecBuffer
    //

    Buffers[0].pvBuffer = hBuffer;
    Buffers[0].cbBuffer = Sizes.cbHeader;
    Buffers[0].BufferType = SECBUFFER_TOKEN;

    Buffers[1].pvBuffer = (LPBYTE)hBuffer + Sizes.cbHeader;
    memcpy(Buffers[1].pvBuffer,
           lpBuffer,
           dwInBufferLen);

    Buffers[1].cbBuffer = dwInBufferLen;
    Buffers[1].BufferType = SECBUFFER_DATA;

    //
    // check if security pkg supports trailer: PCT does
    //

    if ( Sizes.cbTrailer ) {
         Buffers[2].pvBuffer = (LPBYTE)hBuffer + Sizes.cbHeader + dwInBufferLen;
         Buffers[2].cbBuffer = Sizes.cbTrailer;
         Buffers[2].BufferType = SECBUFFER_TOKEN;
    } else {
         Buffers[2].pvBuffer = NULL;
         Buffers[2].cbBuffer = 0;
         Buffers[2].BufferType = SECBUFFER_EMPTY;
    }

    Buffer.cBuffers = 3;
    Buffer.pBuffers = Buffers;
    Buffer.ulVersion = SECBUFFER_VERSION;

    scRet = g_SealMessage(&m_hContext,
                          0,
                          &Buffer,
                          0);

    DEBUG_PRINT(API,
                INFO,
                ("SealMessage returned, %s [%x]\n",
                InternetMapSSPIError((DWORD)scRet),
                scRet
                ));


    if (scRet != ERROR_SUCCESS) {

        //
        // Map the SSPI error.
        //

        DEBUG_PRINT(API,
                    ERROR,
                    ("SealMessage returned, %s [%x]\n",
                    InternetMapSSPIError((DWORD)scRet),
                    scRet
                    ));

        error = MapInternetError((DWORD) scRet);

        if (hBuffer != NULL) {
            FREE_MEMORY(hBuffer);
        }
        goto quit;
    } else {
        error = ERROR_SUCCESS;
    }

    *lplpBuffer = Buffers[0].pvBuffer;
    *lpdwOutBufferLen = Sizes.cbHeader + Buffers[1].cbBuffer +
                        Buffers[2].cbBuffer;
    *lpdwInBufferBytesEncrypted = dwInBufferLen;

    DEBUG_PRINT(API,
                INFO,
                ("SealMessage returned Buffer = %x, EncryptBytes = %d, UnencryptBytes=%d\n",
                *lplpBuffer,
                *lpdwOutBufferLen,
                dwInBufferLen
                ));

quit:

    DEBUG_LEAVE(error);

    return error;
}


#define SSLPCT_SMALLESTHEADERCHUNK      3


DWORD
ICSecureSocket::DecryptData(
    OUT DWORD * lpdwBytesNeeded,
    OUT LPBYTE lpOutBuffer,
    IN OUT LPDWORD lpdwOutBufferLeft,
    IN OUT LPDWORD lpdwOutBufferReceived,
    IN OUT LPDWORD lpdwOutBufferBytesRead
    )

/*++

Routine Description:

    This function decrypts data into the lpOutBuffer. It attempts to fill lpOutBuffer.
    If it fails, it may do so because more bytes are
    needed to fill lplpEncDecBuffer or lplpEndDecBuffer is not big enough to fully
    contain a complete server generated SSL/PCT message.


Return Value:

    Error Code

--*/

{
    INET_ASSERT(IsSecure());
    INET_ASSERT(lpOutBuffer);
    INET_ASSERT(lpdwOutBufferBytesRead);
    INET_ASSERT(lpdwBytesNeeded);

    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSecureSocket::DecryptData",
                 "{%#x [%#x:%#x], %#x} %#x [%d], %#x, %#x [%d], %#x [%d], %#x [%d]",
                 &m_hContext,
                 m_hContext.dwUpper,
                 m_hContext.dwLower,
                 m_pdblbufBuffer,
                 lpdwBytesNeeded,
                 *lpdwBytesNeeded,
                 lpOutBuffer,
                 lpdwOutBufferLeft,
                 *lpdwOutBufferLeft,
                 lpdwOutBufferReceived,
                 *lpdwOutBufferReceived,
                 lpdwOutBufferBytesRead,
                 *lpdwOutBufferBytesRead
                 ));

    SecBufferDesc Buffer;
    SecBuffer Buffers[4];   // the 4 buffers are: header, data, trailer, extra
    DWORD scRet = ERROR_SUCCESS;

    *lpdwBytesNeeded = 0;

    //
    //  HOW THIS THING WORKS:
    //  We sit in a loop, attempting to fill our passed in buffer with
    //  decrypted data.  If there is no decrypted data we check to
    //  see if there is encrypted data sitting in our buffer.
    //
    //  Assuming there is enough we decrypt a chunk, and place it in the
    //  output buffer of our double buffer class. We reloop and try to
    //  copy it to our passed in byffer.
    //
    //  If there is more encrypted data, and more space to fill in
    //  the user buffer, we attempt to decrypt the next chunk of this.
    //
    //  If we do not have enough data, we return with an error, and
    //  expect a network read to be done.
    //

    do {

        //
        // Check to see if we can fill up User buffer.
        //

        m_pdblbufBuffer->CopyOut(
            lpOutBuffer,
            lpdwOutBufferLeft,
            lpdwOutBufferReceived,
            lpdwOutBufferBytesRead
        );

        //
        // If we've filled our output buffer, than exit with ERROR_SUCCESS
        //

        if ( *lpdwOutBufferLeft == 0)
        {
            break;
        }

        //
        // If we've got less than ~3 bytes return so we can read more data.
        //

        if (m_pdblbufBuffer->GetInputBufferSize() < SSLPCT_SMALLESTHEADERCHUNK) {
            scRet = (DWORD) SEC_E_INCOMPLETE_MESSAGE;
            break;
        }

        //
        // prepare data the SecBuffer for a call to SSL/PCT decryption code.
        //

        Buffers[0].pvBuffer = m_pdblbufBuffer->GetInputBufferPointer( );
        Buffers[0].cbBuffer = m_pdblbufBuffer->GetInputBufferSize(); // # of bytes to decrypt
        Buffers[0].BufferType = SECBUFFER_DATA;

        int i;

        for ( i = 1; i < 4; i++ )
        {
            //
            // clear other 3 buffers for receving result from SSPI package
            //

            Buffers[i].pvBuffer = NULL;
            Buffers[i].cbBuffer = 0;
            Buffers[i].BufferType = SECBUFFER_EMPTY;
        }

        Buffer.cBuffers = 4; // the 4 buffers are: header, data, trailer, extra
        Buffer.pBuffers = Buffers;
        Buffer.ulVersion = SECBUFFER_VERSION;

        //
        // Decrypt the DATA !!!
        //

        scRet = g_UnsealMessage(&m_hContext,
                                &Buffer,
                                0,
                                NULL );

        DEBUG_PRINT(API,
                    INFO,
                    ("UnsealMessage returned, %s [%x]\n",
                    InternetMapSSPIError((DWORD)scRet),
                    scRet
                    ));



        if ( scRet != ERROR_SUCCESS &&
             scRet != SEC_I_RENEGOTIATE)
        {
            DEBUG_PRINT(API,
                        ERROR,
                        ("UnsealMessage failed, error %lx\n",
                        scRet
                        ));

            INET_ASSERT( scRet != SEC_E_MESSAGE_ALTERED );

            if ( scRet == SEC_E_INCOMPLETE_MESSAGE )
            {
                DWORD dwAddlBufferNeeded = Buffers[1].cbBuffer;

                DEBUG_PRINT(API,
                             INFO,
                             ("UnsealMessage short of %d bytes\n",
                             dwAddlBufferNeeded
                             ));

                 //
                 // If we're missing data, return to get the missing data.
                 // But make sure we have enough room first!
                 //

                if (!m_pdblbufBuffer->ResizeBufferIfNeeded(dwAddlBufferNeeded)) {
                    scRet = ERROR_NOT_ENOUGH_MEMORY;
                }
                *lpdwBytesNeeded = dwAddlBufferNeeded;
                break;
            }
            else if ( scRet == 0x00090317 /*SEC_I_CONTEXT_EXPIRED*/)
            {
                //
                // Ignore this error and treat this like a simple terminator
                //  to end the connection.
                //

                scRet = ERROR_SUCCESS;
            }
            else
            {
                break;
            }
        }



        //
        // Success we decrypted a block
        //

        LPBYTE  lpExtraBuffer;
        DWORD   dwExtraBufferSize;
        LPBYTE  lpDecryptedBuffer;
        DWORD   dwDecryptedBufferSize;


        lpDecryptedBuffer       =   (LPBYTE) Buffers[1].pvBuffer;
        dwDecryptedBufferSize   =   Buffers[1].cbBuffer;

        //
        // BUGBUG [arthurbi] this is hack to work with the OLD SSLSSPI.DLL .
        //  They return extra on the second buffer instead of the third.
        //

        if ( Buffers[2].BufferType == SECBUFFER_EXTRA )
        {
            lpExtraBuffer   = (LPBYTE) Buffers[2].pvBuffer;
            dwExtraBufferSize = Buffers[2].cbBuffer;
        }
        else if ( Buffers[3].BufferType == SECBUFFER_EXTRA )
        {
            lpExtraBuffer   = (LPBYTE) Buffers[3].pvBuffer;
            dwExtraBufferSize = Buffers[3].cbBuffer;
        }
        else
        {
            lpExtraBuffer = NULL;
            dwExtraBufferSize = 0;
        }


        m_pdblbufBuffer->SetOutputInputBuffer(
            lpDecryptedBuffer,
            dwDecryptedBufferSize,
            lpExtraBuffer,
            dwExtraBufferSize,
            FALSE // don't combine.
        );

        if ( dwDecryptedBufferSize == 0 )
            break;  // No more data to process

        INET_ASSERT( *lpdwOutBufferLeft );  // don't expect to get here this way.

    } while ( *lpdwOutBufferLeft && scRet == ERROR_SUCCESS );



    DEBUG_PRINT(API,
         INFO,
         ("DecryptData returning, "
          "OutBuffer = %x, DecryptBytesRecv = %d\n",
         lpOutBuffer,
         *lpdwOutBufferBytesRead
         ));

    DEBUG_LEAVE((DWORD)scRet);

    return ( scRet );
}


VOID
ICSecureSocket::TerminateSecConnection(
    VOID
    )

/*++

Routine Description:

    This function deletes the security context handle which result
    in deleting the local data structures with which they are associated.

Arguments:

    None

Return Value:

    None

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "ICSecureSocket::TerminateSecConnection",
                 "{%#x [%#x:%#x]}",
                 this,
                 m_hContext.dwUpper,
                 m_hContext.dwLower
                 ));

    INET_ASSERT(IsSecure());

    //INET_ASSERT(m_hContext.dwLower != 0);
    //INET_ASSERT(m_hContext.dwUpper != 0);

    if (GlobalSecFuncTable) {
        if (!((m_hContext.dwLower == 0) && (m_hContext.dwUpper == 0))) {
            // There are cases where because of circular dependencies
            // schannel could get unloaded before wininet. In that case
            // this call could fault. This usually happens when the process 
            // is shutting down.
            __try {
                g_DeleteSecurityContext(&m_hContext);
            } __except(EXCEPTION_EXECUTE_HANDLER) {
            }
            ENDEXCEPT

            m_hContext.dwLower = m_hContext.dwUpper = 0;
        }
    } else {

        DEBUG_PRINT(API,
                    ERROR,
                    ("Attempting to Delete a security context, with a NULL SSPI func table!(missing SCHANNEL.DLL?)\n"
                    ));

    }

    DEBUG_LEAVE(0);
}

#ifdef SECPKG_ATTR_PROTO_INFO
/*++

ProtoInfoToString:

    This routine converts an SSPI SecPkgContext_ProtoInfo structure into a
    string.  The returned string must be released via LocalFree.

Arguments:

    pProtoInfo supplies the SecPkgContext_ProtoInfo structure to be converted to
    string representation.

Return Value:

    Non-NULL is the address of the returned string.  This must be freed via
        LocalFree once it is no longer needed.

    NULL implies no memory is available.

Author:

    Doug Barlow (dbarlow) 4/23/1996

--*/


PRIVATE
LPTSTR
ProtoInfoToString(
    IN const PSecPkgContext_ProtoInfo pProtoInfo)
{
    TCHAR
        szValue[32],
        szSep[8];
    LPTSTR
        szFinal
            = NULL;
    DWORD
        length;

    length = GetLocaleInfo(
                LOCALE_USER_DEFAULT,
                LOCALE_SDECIMAL,
                szSep,
                sizeof(szSep) / sizeof(TCHAR));
    if (0 >= length)
        lstrcpy(szSep, TEXT("."));

    length = wsprintf(
                szValue,
                TEXT("%d%s%d"),
                pProtoInfo->majorVersion,
                szSep,
                pProtoInfo->minorVersion);
    INET_ASSERT(sizeof(szValue) / sizeof(TCHAR) > length);

    length = lstrlen(pProtoInfo->sProtocolName);
    length += 2;                    // Space and Trailing NULL
    length += lstrlen(szValue);
    szFinal = (LPTSTR)ALLOCATE_MEMORY(LMEM_FIXED, length * sizeof(TCHAR));
    if (NULL != szFinal)
    {
        lstrcpy(szFinal, pProtoInfo->sProtocolName);
        lstrcat(szFinal, TEXT(" "));
        lstrcat(szFinal, szValue);
    }
    return szFinal;
}
#endif
