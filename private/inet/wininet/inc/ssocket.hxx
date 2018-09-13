/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ssocket.hxx

Abstract:

    Contains types, manifests, prototypes for Internet Secure Socket Class
    (ICSecureSocket) functions and methods (in common\ssocket.cxx)

Author:

    Richard L Firth (rfirth) 08-Apr-1997

Revision History:

    08-Apr-1997 rfirth
        Created (from ixport.hxx)

--*/

#define SECURITY_WIN32
#include <sspi.h>
#include <issperr.h>
#include <buffer.hxx>
#include <wincrerr.h>


//
// forward references
//

class CFsm_SecureConnect;
class CFsm_SecureHandshake;
class CFsm_SecureNegotiate;
class CFsm_NegotiateLoop;
class CFsm_SecureSend;
class CFsm_SecureReceive;

//
// classes
//

class ICSecureSocket : public ICSocket {

private:

    CtxtHandle m_hContext;
    DWORD m_dwProviderIndex;
    LPSTR m_lpszHostName;
    DBLBUFFER * m_pdblbufBuffer;
    DWORD m_dwErrorFlags;


    SECURITY_CACHE_LIST_ENTRY *m_pSecurityInfo;
    SECURITY_CACHE_LIST_ENTRY **m_ppSecurityInfo;

#if INET_DEBUG

#define SECURE_SOCKET_SIGNATURE 0x534c5353  // "SSLS"

#define SIGN_SECURE_SOCKET() \
    m_Signature = SECURE_SOCKET_SIGNATURE

#define CHECK_SECURE_SOCKET() \
    INET_ASSERT(m_Signature == SECURE_SOCKET_SIGNATURE)

#else

#define SIGN_SECURE_SOCKET() \
    /* NOTHING */

#define CHECK_SECURE_SOCKET() \
    /* NOTHING */

#endif

    VOID SetNonSecure(VOID)
    {
        if(m_pSecurityInfo)
        {
            m_pSecurityInfo->ClearSecureFlags(SECURITY_FLAG_SECURE);
        }
    }

    VOID SetSecure(VOID)
    {
        SetSecureFlags(SECURITY_FLAG_SECURE);
    }

    DWORD
    EncryptData(
        IN LPVOID lpBuffer,
        IN DWORD dwInBufferLen,
        OUT LPVOID * lplpBuffer,
        OUT LPDWORD lpdwOutBufferLen,
        OUT LPDWORD lpdwInBufferBytesEncrypted
        );

    DWORD
    DecryptData(
        OUT DWORD * lpdwBytesNeeded,
        OUT LPBYTE lpOutBuffer,
        IN OUT LPDWORD lpdwOutBufferLeft,
        IN OUT LPDWORD lpdwOutBufferReceived,
        IN OUT LPDWORD lpdwOutBufferBytesRead
        );

    VOID
    TerminateSecConnection(
        VOID
        );

    BOOL IsValidCacheEntry();

public:

    ICSecureSocket(DWORD dwErrorFlags = 0, INTERNET_SCHEME tScheme = INTERNET_SCHEME_DEFAULT);

    virtual ~ICSecureSocket(VOID);

    DWORD
    Connect(
        IN LONG Timeout,
        IN INT Retries,
        IN DWORD dwFlags
        );

    DWORD
    Connect_Fsm(
        IN CFsm_SecureConnect * Fsm
        );

    DWORD
    SecureHandshake_Fsm(
        IN CFsm_SecureHandshake * Fsm
        );

    DWORD
    SecureNegotiate_Fsm(
        IN CFsm_SecureNegotiate * Fsm
        );

    DWORD
    NegotiateLoop_Fsm(
        IN CFsm_NegotiateLoop * Fsm
        );

    DWORD
    NegotiateSecConnection(
        IN DWORD dwFlags,
        OUT LPBOOL lpbAttemptReconnect
        );

    DWORD
    SSPINegotiateLoop(
        OUT DBLBUFFER * pDoubleBuffer,
        IN DWORD dwFlags,
        IN CredHandle hCreds,
        IN BOOL fDoInitialRead,
        IN BOOL bDoingClientAuth
        );

    DWORD
    Disconnect(
        IN DWORD dwFlags
        );

    DWORD
    Send(
        IN LPVOID lpBuffer,
        IN DWORD dwBufferLength,
        IN DWORD dwFlags
        );

    DWORD
    Send_Fsm(
        IN CFsm_SecureSend * Fsm
        );

    DWORD
    Receive(
        IN OUT LPVOID* lplpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT LPDWORD lpdwBufferRemaining,
        IN OUT LPDWORD lpdwBytesReceived,
        IN DWORD dwExtraSpace,
        IN DWORD dwFlags,
        OUT LPBOOL lpbEof
        );

    DWORD
    Receive_Fsm(
        IN CFsm_SecureReceive * Fsm
        );

    DWORD
    SecureHandshakeWithServer(
        IN DWORD dwFlags,
        OUT LPBOOL lpfAttemptReconnect
        );

    DWORD
    VerifyTrust(
        VOID
        );

    SECURITY_CACHE_LIST_ENTRY * GetSecurityEntry() {
        if (m_pSecurityInfo != NULL) {
            m_pSecurityInfo->AddRef();
            return m_pSecurityInfo;
        }
        return NULL;
    }

    VOID SetSecurityEntry(SECURITY_CACHE_LIST_ENTRY **entry) {
        if (entry != NULL && *entry) {
            (*entry)->AddRef();
        }
        if (m_pSecurityInfo != NULL) {
            m_pSecurityInfo->Release();
        }

        m_pSecurityInfo = entry ? *entry : NULL;
        m_ppSecurityInfo = entry;
    }

    DWORD
    SetHostName(
        IN LPSTR lpszHostName
        );

    LPSTR GetHostName(VOID) const {

        INET_ASSERT(IsSecure());
        INET_ASSERT(m_lpszHostName != NULL);

        return m_lpszHostName;
    }

    //
    // GetCertChainList (and)
    //   SetCertChainList -
    //  Sets and Gets Client Authentication Cert Chains.
    //

    CERT_CONTEXT_ARRAY* GetCertContextArray(VOID) {
        if(m_pSecurityInfo)
        {
            return m_pSecurityInfo->GetCertContextArray();
        }
        return NULL;

    }

    VOID SetCertContextArray(CERT_CONTEXT_ARRAY* pNewCertContextArray) {
        if(m_pSecurityInfo)
        {
            m_pSecurityInfo->SetCertContextArray(pNewCertContextArray);
        }
    }

    //
    // GetSecureFlags AND SetSecureFlags AND GetCertInfo
    //  Allows setting and getting of a bitmask which
    //  stores various data bits on current socket connection.
    //


    DWORD GetSecurityInfo(LPINTERNET_SECURITY_INFO pInfo) {

        if(m_pSecurityInfo)
        {
            m_pSecurityInfo->CopyOut(*pInfo);
            return ERROR_SUCCESS;
        }
        else
        {
            return ERROR_INTERNET_INTERNAL_ERROR;
        }
    }

    VOID SetSecureFlags(DWORD Flags) {
        if(m_pSecurityInfo)
        {
            m_pSecurityInfo->SetSecureFlags(Flags);
        }
    }

    DWORD GetSecureFlags(VOID) {
        if(m_pSecurityInfo)
        {
            return m_pSecurityInfo->GetSecureFlags();
        }
        return 0;
    }

    DWORD GetProviderIndex(VOID) const {

        INET_ASSERT(IsSecure());

        return m_dwProviderIndex;
    }

    VOID SetProviderIndex(DWORD dwIndex) {

        INET_ASSERT(IsSecure());

        m_dwProviderIndex = dwIndex;
    }


};
