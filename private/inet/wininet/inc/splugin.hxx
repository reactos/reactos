/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    splugin.hxx

Abstract:

    This file contains definitions for splugin.cxx


Author:

    Arthur Bierer (arthurbi) 25-Dec-1995

Revision History:

    Rajeev Dujari (rajeevd)  01-Oct-1996 overhaul

--*/

#ifndef SPLUGIN_HXX
#define SPLUGIN_HXX

// macros for code prettiness
#define ALLOCATE_BUFFER 0x1
#define GET_SCHEME      0x2

#define IS_PROXY  TRUE
#define IS_SERVER FALSE

#define IS_USER   TRUE
#define IS_PASS   FALSE

// states set on request handle
#define AUTHSTATE_NONE       0
#define AUTHSTATE_NEGOTIATE  1
#define AUTHSTATE_CHALLENGE  2
#define AUTHSTATE_NEEDTUNNEL 3
#define AUTHSTATE_LAST       AUTHSTATE_NEEDTUNNEL
// warning !!! do not add any more AUTHSTATEs without increasing size of bitfield


struct PWC;
class HTTP_REQUEST_HANDLE_OBJECT;


//-----------------------------------------------------------------------------
//
//  AUTHCTX
//
class AUTHCTX
{

public:

    // States
    enum SPMState
    {
        STATE_NOTLOADED = 0,
        STATE_LOADED,
        STATE_ERROR
    };

    // AUTHCTX SPM Schemes
    enum SPMScheme
    {
        SCHEME_BASIC = 0,
        SCHEME_DIGEST,
        SCHEME_NTLM,
        SCHEME_MSN,
        SCHEME_DPA,
        SCHEME_KERBEROS,
        SCHEME_NEGOTIATE,
        SCHEME_UNKNOWN
    };

class SPMData
{
public:
    LPSTR           szScheme;
    DWORD           cbScheme;
    DWORD           dwFlags;
    SPMScheme       eScheme;
    SPMState        eState;
    SPMData        *pNext;

    SPMData(LPSTR szScheme, DWORD dwFlags);
    ~SPMData();
};


    // Global linked list of SPM providers.
    static SPMData  *g_pSPMList;

    // Global spm list state
    static SPMState  g_eState;

    // SubScheme - specifically for the negotiate
    // package - can be either NTLM or Kerberos.
    SPMScheme       _eSubScheme;
    DWORD           _dwSubFlags;
public:

    // Instance specific;
    HTTP_REQUEST_HANDLE_OBJECT *_pRequest;

    SPMData   *_pSPMData;
    LPVOID     _pvContext;
    PWC       *_pPWC;
    BOOL       _fIsProxy;

    // Constructor
    AUTHCTX(SPMData *pSPM, PWC* pPWC);

    // Destructor
    virtual ~AUTHCTX();

    // ------------------------  Static Functions -----------------------------

    static VOID Enumerate();
    static VOID UnloadAll();
    static AUTHCTX* CreateAuthCtx(HTTP_REQUEST_HANDLE_OBJECT *pReq, BOOL fIsProxy);

    static AUTHCTX* CreateAuthCtx(HTTP_REQUEST_HANDLE_OBJECT *pReq, BOOL fIsProxy,
        LPSTR szScheme);

    static AUTHCTX* CreateAuthCtx(HTTP_REQUEST_HANDLE_OBJECT *pReq, BOOL fIsProxy,
        PWC* pPWC);

    static AUTHCTX::SPMState GetSPMListState();

    static PWC* SearchPwcList (PWC* pwc, LPSTR lpszHost,
        LPSTR lpszUri, LPSTR lpszRealm, SPMData *pSPM);

    static PWC* FindOrCreatePWC
    (
        HTTP_REQUEST_HANDLE_OBJECT *pRequest,
        BOOL     fIsProxy,
        SPMData *pSPM,
        LPSTR    lpszRealm
    );

    static DWORD GetAuthHeaderData
    (
        HTTP_REQUEST_HANDLE_OBJECT *pRequest,
        BOOL      fIsProxy,
        LPSTR     szItem,
        LPSTR    *pszData,
        LPDWORD   pcbData,
        DWORD     dwIndex,
        DWORD     dwFlags
    );


    
    // ------------------------ Base class functions ---------------------------

    DWORD FindHdrIdxFromScheme(LPDWORD pdwIndex);

    LPSTR     GetScheme();
    DWORD     GetFlags();
    SPMState  GetState();
    SPMScheme GetSchemeType();


    // ------------------------------ Overrides--------------------------------

    // Called before request to generate any pre-authentication headers.
    virtual DWORD PreAuthUser(IN LPSTR pBuf, IN OUT LPDWORD pcbBuf) = 0;

    // Retrieves response header data
    virtual DWORD UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy) = 0;

    // Called after UpdateFromHeaders to update authentication context.
    virtual DWORD PostAuthUser() = 0;

};


//-----------------------------------------------------------------------------
//
//  BASIC_CTX
//

class BASIC_CTX : public AUTHCTX
{

public:
    BASIC_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy, SPMData* pSPM, PWC* pPWC);
    ~BASIC_CTX();


    // virtual overrides
    DWORD PreAuthUser(IN LPSTR pBuf, IN OUT LPDWORD pcbBuf);

    DWORD UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy);

    DWORD PostAuthUser();


};



//-----------------------------------------------------------------------------
//
//  PLUG_CTX
//
class PLUG_CTX : public AUTHCTX
{

public:

    // Class specific data.
    LPSTR _szAlloc;
    LPSTR _szData;
    DWORD _cbData;
    BOOL  _fNTLMProxyAuth;
    
    // Class specific funcs.
    DWORD Load();
    DWORD ClearAuthUser(LPVOID *ppvContext, LPSTR szServer);


    DWORD wQueryHeadersAlloc
    (
        IN HINTERNET hRequestMapped,
        IN DWORD dwQuery,
        OUT LPDWORD lpdwQueryIndex,
        OUT LPSTR *lppszOutStr,
        OUT LPDWORD lpdwSize
    );

    DWORD CrackAuthenticationHeader
    (
        IN HINTERNET hRequestMapped,
        IN BOOL      fIsProxy,
        IN     DWORD dwAuthenticationIndex,
        IN OUT LPSTR *lppszAuthHeader,
        IN OUT LPSTR *lppszExtra,
        IN OUT DWORD *lpdwExtra,
           OUT LPSTR *lppszAuthScheme
    );

    VOID ResolveProtocol();
    
    PLUG_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy, SPMData* pSPM, PWC* pPWC);
    ~PLUG_CTX();

    // virtual overrides
    DWORD PreAuthUser(IN LPSTR pBuf, IN OUT LPDWORD pcbBuf);

    DWORD UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy);

    DWORD PostAuthUser();

};

//-----------------------------------------------------------------------------
//
//  DIGEST_CTX
//
class DIGEST_CTX : public AUTHCTX
{

protected:
    VOID  GetFuncTbl();
    VOID  InitSecurityBuffers(LPSTR szBuffOut, DWORD cbBuffOut, 
        LPDWORD dwSecFlags, DWORD dwISCMode);
    LPSTR GetRequestUri();
public:

    static PSecurityFunctionTable g_pFuncTbl;
    static CredHandle             g_hCred;
    
    // Class specific data.
    SecBuffer      _SecBuffIn[10];
    SecBufferDesc  _SecBuffInDesc;
    SecBuffer      _SecBuffOut[1];
    SecBufferDesc  _SecBuffOutDesc;
    CtxtHandle     _hCtxt;

    LPSTR _szAlloc;
    LPSTR _szData;
    LPSTR _szRequestUri;
    DWORD _cbContext;
    DWORD _cbData;
    DWORD _nRetries;
    
    DIGEST_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy, SPMData* pSPM, PWC* pPWC);

    ~DIGEST_CTX();

    DWORD PromptForCreds(HWND hWnd);
    
    // virtual overrides
    DWORD PreAuthUser(IN LPSTR pBuf, IN OUT LPDWORD pcbBuf);

    DWORD UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy);

    DWORD PostAuthUser();
    static VOID  FlushCreds();
    static VOID  Logoff();

};


struct PWC // PassWord Cache entry
{
    PWC*      pNext;        // next item in linked list
    DWORD     nLockCount;   // number of references
    AUTHCTX::SPMData   *pSPM;        // Scheme
    LPSTR     lpszHost;     // name of server or proxy
    LPSTR     lpszUrl;      // url pattern with wildcard
    LPSTR     lpszRealm;    // realm, optional, may be null
    LPSTR     lpszUser;     // username, may be null
    LPSTR     lpszPass;     // password, may be null
    LPSTR     lpszNonce;    // nonce value
    LPSTR     lpszOpaque;   // opaque value.
    BOOL      fPreAuth;     // allow preauthorization

    DWORD SetUser (LPSTR lpszUser);
    DWORD SetPass (LPSTR lpszUser);
    LPSTR GetUser (void) { return lpszUser;}
    LPSTR GetPass (void) { return lpszPass;}
};


#ifndef ARRAY_ELEMENTS
#define ARRAY_ELEMENTS(rg)                (sizeof(rg) / sizeof((rg)[0]))
#endif

// password cache locking
void  AuthOpen   (void);
void  AuthClose  (void);
void  AuthLock   (void);
void  AuthUnlock (void);

// worker thread calls
DWORD AuthOnRequest    (HINTERNET hRequest);
DWORD AuthOnResponse   (HINTERNET hRequest);

// dialog serialization
BOOL  AuthInDialog     (AUTHCTX *pAuthCtx, INTERNET_AUTH_NOTIFY_DATA *pNotify, BOOL *pfMustLock);
void  AuthNotify       (PWC *pwc, DWORD error);

// cleanup
void  AuthFlush (void); // flush password cache
void  AuthUnload (void); // unload everything



#endif // SPLUGIN_HXX
