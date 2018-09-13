
#ifdef INET_DEBUG
#define CERT_CONTEXT_ARRAY_ALLOC_UNIT 2 // made small for testing
#else
#define CERT_CONTEXT_ARRAY_ALLOC_UNIT 10
#endif

#define ClearCreds(CredHandle) \
            CredHandle.dwLower = CredHandle.dwUpper = 0

#define IsCredClear(CredHandle) \
            (( CredHandle.dwLower == 0 && CredHandle.dwUpper == 0 ) ? TRUE : FALSE )


class CERT_CONTEXT_ARRAY
{

private:

    //
    // number of cert chains in array
    //

    DWORD           _cCertContexts;

    //
    // number of slots allocated in array
    //

    DWORD           _cAlloced;

    //
    // array of Cert Context pointers
    //

    PCCERT_CONTEXT*   _ppCertContexts;

    //
    // Index of Cert Chain, selected to be used by user.
    //

    INT             _iSelected;

    //
    // Not Equal to ERROR_SUCCESS upon error at intialization.
    //

    DWORD           _error;

    // Critical section to guard the Cred Handle
    CRITICAL_SECTION _cs ; 

    // Cred Handle created for the selected cert context which we should re-use
    // to prevent multiple prompts to the user.
    CredHandle _hCreds;


public:

    CERT_CONTEXT_ARRAY();
    ~CERT_CONTEXT_ARRAY();

    DWORD
    AddCertContext(
        PCCERT_CONTEXT    pCertContext
        )
    {
        DWORD error = ERROR_SUCCESS;
        INET_ASSERT(pCertContext);

        //
        // If the Array is already full, Realloc
        //

        if ( _cAlloced <= _cCertContexts )
        {
            INET_ASSERT(_cAlloced == _cCertContexts);

            _ppCertContexts = (PCCERT_CONTEXT *)
                        REALLOCATE_MEMORY(_ppCertContexts,
                                      (sizeof(PCERT_CONTEXT)*
                                        (CERT_CONTEXT_ARRAY_ALLOC_UNIT+_cAlloced)),
                                      LMEM_MOVEABLE
                                      );

            _cAlloced += CERT_CONTEXT_ARRAY_ALLOC_UNIT;

            if ( _ppCertContexts == NULL )
            {
                error = GetLastError();
                goto quit;
            }
        }

        //
        // Store new Pointer into array
        //
        PCCERT_CONTEXT pNewCertContext;
        pNewCertContext= CertDuplicateCertificateContext(pCertContext);

        if (pNewCertContext == NULL)
        {
            error = GetLastError();
            goto quit;
        }

        _ppCertContexts[_cCertContexts] = pNewCertContext;

        _cCertContexts++;

    quit:

        return error;
    }


    VOID
    SelectCertContext(
        INT index
        )
    {
        INET_ASSERT((index >= 0 && index < (INT) _cCertContexts) || index == -1);

        _iSelected = index;
    }


    PCCERT_CONTEXT
    GetCertContext(
        DWORD dwIndex
        )
    {

        INET_ASSERT(dwIndex < _cCertContexts);
        return _ppCertContexts[dwIndex];
    }

    PCCERT_CONTEXT
    GetSelectedCertContext(
        VOID
        )
    {
        INET_ASSERT(_iSelected >= 0 || _iSelected == -1);

        if ( _iSelected == -1 )
            return NULL;

        return GetCertContext((DWORD) _iSelected);
    }


    DWORD
    GetError(
        VOID
        )
    {
        return _error;
    }

    DWORD
    GetArraySize(
        VOID
        )
    {
        return _cCertContexts;
    }

    VOID 
    LockCredHandle( )
    {
        EnterCriticalSection(&_cs);
    }

    VOID 
    UnlockCredHandle( )
    {
        LeaveCriticalSection(&_cs);
    }

    CredHandle 
    GetCredHandle( )
    {
        return _hCreds;
    }

    VOID 
    SetCredHandle(CredHandle hCreds )
    {
        _hCreds = hCreds;
    }
};


typedef HRESULT
(WINAPI * WIN_VERIFY_TRUST_FN)
(   
    IN OPTIONAL HWND hwnd,
    IN          GUID *pgActionID,
    IN          WINTRUST_DATA *pWinTrustData
);

typedef CRYPT_PROVIDER_DATA * (WINAPI * WT_HELPER_PROV_DATA_FROM_STATE_DATA_FN)
(
    IN HANDLE hStateData
);


#define WIN_VERIFY_TRUST_NAME       TEXT("WinVerifyTrust")
#define WT_HELPER_PROV_DATA_FROM_STATE_DATA_NAME TEXT("WTHelperProvDataFromStateData")

#define ADVAPI_DLLNAME              TEXT("advapi32.dll")
#define WINTRUST_DLLNAME            TEXT("wintrust.dll")
#define SOFTPUB_DLLNAME             TEXT("softpub.dll")

#define SP_REG_KEY_SCHANNEL_BASE    TEXT("System\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL")
#define SP_REG_WINTRUST             TEXT("Wintrust")
#define CLIENT_AUTH_TYPE            L"ClientAuth"
#define CHAIN_BUFFER_SIZE           32768
#define ISSUER_SIZE_FIELD_SIZE      2

DWORD
CliAuthSelectCredential(
    IN PCtxtHandle       phContext,
    IN LPTSTR            pszPackageName,
    IN CERT_CONTEXT_ARRAY  *pCertContextArray,
    OUT PCredHandle      phCredential);


DWORD
CliAuthAcquireCertContexts(
    IN  PCtxtHandle      phContext,
    IN  LPTSTR           pszPackageName,
    OUT CERT_CONTEXT_ARRAY **ppCertContextArray
    );

