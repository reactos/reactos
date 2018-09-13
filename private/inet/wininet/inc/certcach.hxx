/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    certcach.hxx

Abstract:

    Contains class definition for certificate cache object.
    The class acts a container for common certificates.

    Contents:
        SECURITY_CACHE_LIST
        SECURITY_CACHE_LIST_ENTRY

Author:

    Arthur L Bierer (arthurbi) 20-Apr-1996

Revision History:

    20-Apr-1996 arthurbi
        Created

--*/

//
// Flags, use wininet.w defined ones, so we don't collide.
//

#define CERTCACHE_FLAG_FOUND_CERT                   INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS
#define CERTCACHE_FLAG_IGNORE_CERT_CN_INVALID_SEND  INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP


//
// SECURITY_INFO_LIST_ENTRY - contains all security info
// pertaining to all connections to a server.
//

class SECURITY_CACHE_LIST_ENTRY {

friend class SECURITY_CACHE_LIST;

private:

    //
    // _List - Generic List entry structure.
    //

    LIST_ENTRY _List;

    //
    // _cRef - Reference count for this element.
    //

    LONG _cRef;

    //
    // _CertInfo - Certificate and other security
    //             attributes for the connection to
    //             this machine.
    //

    INTERNET_SECURITY_INFO _CertInfo;

    //
    // _dwSecurityFlags - Overrides for warnings.
    //

    DWORD    _dwSecurityFlags;

    //
    // _ServerName - The name of the server
    //

    ICSTRING _ServerName;

    //
    // _pCertChainList - If there is Client Authentication do be done with this server,
    //          then we'll cache it and remeber it later.
    //

    CERT_CONTEXT_ARRAY  *_pCertContextArray;

    //
    // _fInCache - indicates this element is held by the cache
    //
    
    BOOL _fInCache;



#if INET_DEBUG
    DWORD m_Signature;
#endif

public:

    LONG AddRef(VOID);
    LONG Release(VOID);


    //
    // Cleans up object, so it can be reused
    //

    BOOL InCache() { return _fInCache; }

    VOID
    Clear();

    SECURITY_CACHE_LIST_ENTRY(
        IN LPSTR lpszHostName
        );

    ~SECURITY_CACHE_LIST_ENTRY();

    //
    // Copy CERT_INFO IN Method -
    //  copies a structure into our object.
    //

    SECURITY_CACHE_LIST_ENTRY& operator=(LPINTERNET_SECURITY_INFO Cert)
    {

        if(_CertInfo.pCertificate)
        {
            CertFreeCertificateContext(_CertInfo.pCertificate);
        }
        _CertInfo.dwSize =                 sizeof(_CertInfo);
        _CertInfo.pCertificate =          CertDuplicateCertificateContext(Cert->pCertificate);
        _CertInfo.dwProtocol  =            Cert->dwProtocol;

        _CertInfo.aiCipher =               Cert->aiCipher;
        _CertInfo.dwCipherStrength =       Cert->dwCipherStrength;
        _CertInfo.aiHash =                 Cert->aiHash;
        _CertInfo.dwHashStrength =         Cert->dwHashStrength;
        _CertInfo.aiExch =                 Cert->aiExch;
        _CertInfo.dwExchStrength =         Cert->dwExchStrength;

        return *this;
    }

    //
    // Copy CERT_INFO OUT Method -
    //  need to copy ourselves out.
    //

    VOID
    CopyOut(INTERNET_SECURITY_INFO &Cert)
    {
        Cert.dwSize =                 sizeof(Cert);
        Cert.pCertificate =          CertDuplicateCertificateContext(_CertInfo.pCertificate);
        Cert.dwProtocol  =            _CertInfo.dwProtocol;

        Cert.aiCipher =               _CertInfo.aiCipher;
        Cert.dwCipherStrength =       _CertInfo.dwCipherStrength;
        Cert.aiHash =                 _CertInfo.aiHash;
        Cert.dwHashStrength =         _CertInfo.dwHashStrength;
        Cert.aiExch =                 _CertInfo.aiExch;
        Cert.dwExchStrength =         _CertInfo.dwExchStrength;
    }

    //
    // Sets and Gets the Client Authentication CertChain -
    //  we piggy back this pointer into the cache so we can cache
    //  previously generated and selected client auth certs.
    //

    VOID SetCertContextArray(CERT_CONTEXT_ARRAY *pCertContextArray) {
        if (_pCertContextArray) {
            delete _pCertContextArray;
        }
        _pCertContextArray = pCertContextArray;
    }

    CERT_CONTEXT_ARRAY * GetCertContextArray() {
        return _pCertContextArray;
    }

    DWORD GetSecureFlags() {
        return _dwSecurityFlags;
    }

    VOID SetSecureFlags(DWORD dwFlags) {
        _dwSecurityFlags |= dwFlags;
    }

    VOID ClearSecureFlags(DWORD dwFlags) {
        _dwSecurityFlags &= (~dwFlags);
    }
};



class SECURITY_CACHE_LIST {

private:

    //
    // _List - serialized list of SECURITY_CACHE_LIST_ENTRY objects
    //

    SERIALIZED_LIST _List;

#if INET_DEBUG
    DWORD m_Signature;
#endif

public:

    SECURITY_CACHE_LIST_ENTRY *
    Find(
        IN LPSTR lpszHostname
        );

    VOID Remove(
        IN LPSTR lpszHostname
        );

    VOID Initialize(VOID) {
        InitializeSerializedList(&_List);
#if INET_DEBUG
        m_Signature = 0x4c436553;   // "SeCL"
#endif
    }

    VOID Terminate(VOID) {

        DEBUG_ENTER((DBG_OBJECTS,
                     None,
                     "SECURITY_CACHE_LIST::Terminate",
                     "{%#x}",
                     this
                     ));

        ClearList();

        TerminateSerializedList(&_List);

        DEBUG_LEAVE(0);
    }

    VOID
    ClearList(
        VOID
        );

    DWORD
    Add(
        IN SECURITY_CACHE_LIST_ENTRY * entry
        );

#if 0

    BOOL
    IsCertInCache(
        IN LPSTR lpszHostname
        )
    {
        SECURITY_CACHE_LIST_ENTRY *entry =
               Find(lpszHostname);

        if ( entry )
            return TRUE;

        return FALSE;
    }
#endif

};
