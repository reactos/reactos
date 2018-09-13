#include <wininetp.h>
#include <urlmon.h>
#include <splugin.hxx>
#include <security.h>
#include "auth.h"

#define SSP_SPM_NT_DLL      "security.dll"
#define SSP_SPM_WIN95_DLL   "secur32.dll"

#define MAX_SILENT_RETRIES  3
#define OUTPUT_BUFFER_LEN   10000

#define HEADER_IDX          0
#define REALM_IDX           1
#define HOST_IDX            2
#define URL_IDX             3
#define METHOD_IDX          4
#define USER_IDX            5
#define PASS_IDX            6
#define NONCE_IDX           7
#define NC_IDX              8
#define HWND_IDX            9
#define NUM_BUFF            10

#define ISC_MODE_AUTH        0
#define ISC_MODE_PREAUTH     1
#define ISC_MODE_UI          2

struct DIGEST_PKG_DATA
{
    LPSTR szAppCtx;
    LPSTR szUserCtx;
};

/*-----------------------------------------------------------------------------
    DIGEST_CTX
-----------------------------------------------------------------------------*/

// Globals
PSecurityFunctionTable DIGEST_CTX::g_pFuncTbl = NULL;
CredHandle DIGEST_CTX::g_hCred;


/*---------------------------------------------------------------------------
DIGEST_CTX::GetFuncTbl
---------------------------------------------------------------------------*/
VOID DIGEST_CTX::GetFuncTbl()
{
    HINSTANCE hSecLib;
    INIT_SECURITY_INTERFACE addrProcISI = NULL;
#ifndef UNIX
    OSVERSIONINFO   VerInfo;

    VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

    GetVersionEx (&VerInfo);

    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        hSecLib = LoadLibrary (SSP_SPM_NT_DLL);
    }
    else if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
#endif /* UNIX */
    {
        hSecLib = LoadLibrary (SSP_SPM_WIN95_DLL);
    }
        
    addrProcISI = (INIT_SECURITY_INTERFACE) GetProcAddress(hSecLib, 
                    SECURITY_ENTRYPOINT_ANSI);       
        
    g_pFuncTbl = (*addrProcISI)();
}

/*---------------------------------------------------------------------------
DIGEST_CTX::GetRequestUri
---------------------------------------------------------------------------*/
LPSTR DIGEST_CTX::GetRequestUri()
{
    LPSTR szUrl;
    DWORD cbUrl;

    URL_COMPONENTS sUrl;        

    memset(&sUrl, 0, sizeof(sUrl));
    sUrl.dwStructSize = sizeof(sUrl);
    sUrl.dwHostNameLength = -1; 
    sUrl.dwUrlPathLength = -1; 
    sUrl.dwExtraInfoLength = -1; 

    szUrl = _pRequest->GetURL();

    // Generate request-uri
    if (InternetCrackUrl(szUrl, strlen(szUrl), 0, &sUrl))
    {
        cbUrl = sUrl.dwUrlPathLength;
        szUrl = new CHAR[cbUrl+1];

        if (!szUrl)
        {
            // Alloc failure. Return NULL. We will
            // use _pRequest->GetURL instead.
            return NULL;
        }
    
        memcpy(szUrl, sUrl.lpszUrlPath, cbUrl);
        szUrl[cbUrl] = '\0';
    }
    else
    {
        // ICU failed. Return NULL which
        // will cause _pRequest->GetURL
        // to be used.
        return NULL;
    }

    return szUrl;
}


/*---------------------------------------------------------------------------
DIGEST_CTX::InitSecurityBuffers
---------------------------------------------------------------------------*/
VOID DIGEST_CTX::InitSecurityBuffers(LPSTR szOutBuf, DWORD cbOutBuf,
    LPDWORD pdwSecFlags, DWORD dwISCMode)
{
    // Input Buffer.    
    _SecBuffInDesc.cBuffers = NUM_BUFF;
    _SecBuffInDesc.pBuffers = _SecBuffIn;

    // Set Header
    _SecBuffIn[HEADER_IDX].pvBuffer     = _szData;
    _SecBuffIn[HEADER_IDX].cbBuffer     = _cbData;
    _SecBuffIn[HEADER_IDX].BufferType   = SECBUFFER_TOKEN;
    
    // If credentials are supplied will be set to
    // ISC_REQ_USE_SUPPLIED_CREDS.
    // If prompting for auth dialog will be set to
    // ISC_REQ_PROMPT_FOR_CREDS.
    *pdwSecFlags = 0;
    
    // Set realm if no header, otherwise NULL.
    if (_SecBuffIn[HEADER_IDX].pvBuffer)
    {
        _SecBuffIn[REALM_IDX].pvBuffer  = NULL;
        _SecBuffIn[REALM_IDX].cbBuffer  = 0;
    }
    else
    {
        // We are preauthenticating using the realm
        _SecBuffIn[REALM_IDX].pvBuffer = _pPWC->lpszRealm;
        _SecBuffIn[REALM_IDX].cbBuffer = strlen(_pPWC->lpszRealm);
    }
    
    // Host.
    _SecBuffIn[HOST_IDX].pvBuffer     = _pPWC->lpszHost;
    _SecBuffIn[HOST_IDX].cbBuffer     = strlen(_pPWC->lpszHost);
    _SecBuffIn[HOST_IDX].BufferType   = SECBUFFER_TOKEN;

    
    // Request URI.    
    if (!_szRequestUri)
    {
        _szRequestUri = GetRequestUri();
        if (_szRequestUri)
            _SecBuffIn[URL_IDX].pvBuffer     = _szRequestUri;
        else
            _SecBuffIn[URL_IDX].pvBuffer = _pRequest->GetURL();
    }

    _SecBuffIn[URL_IDX].cbBuffer     = strlen((LPSTR) _SecBuffIn[URL_IDX].pvBuffer);
    _SecBuffIn[URL_IDX].BufferType   = SECBUFFER_TOKEN;


    // HTTP method.
    _SecBuffIn[METHOD_IDX].cbBuffer = 
        MapHttpMethodType(_pRequest->GetMethodType(), (LPCSTR*) &_SecBuffIn[METHOD_IDX].pvBuffer);
    _SecBuffIn[METHOD_IDX].BufferType   = SECBUFFER_TOKEN;

    // User and pass might be provided from pwc entry. Use only if
    // we have a challenge header (we don't pre-auth using supplied creds).
    if (dwISCMode == ISC_MODE_AUTH && _pPWC->lpszUser && *_pPWC->lpszUser 
        && _pPWC->lpszPass && *_pPWC->lpszPass)
    {
        // User.
        _SecBuffIn[USER_IDX].pvBuffer     = _pPWC->lpszUser;
        _SecBuffIn[USER_IDX].cbBuffer     = strlen(_pPWC->lpszUser);
        _SecBuffIn[USER_IDX].BufferType   = SECBUFFER_TOKEN;

        // Pass.
        _SecBuffIn[PASS_IDX].pvBuffer     = _pPWC->lpszPass;
        _SecBuffIn[PASS_IDX].cbBuffer     = strlen(_pPWC->lpszPass);
        _SecBuffIn[PASS_IDX].BufferType   = SECBUFFER_TOKEN;
        *pdwSecFlags = ISC_REQ_USE_SUPPLIED_CREDS;
    }
    else
    {
        // User.
        _SecBuffIn[USER_IDX].pvBuffer     = NULL;  
        _SecBuffIn[USER_IDX].cbBuffer     = 0;
        _SecBuffIn[USER_IDX].BufferType   = SECBUFFER_TOKEN;

        // Pass.
        _SecBuffIn[PASS_IDX].pvBuffer     = NULL;
        _SecBuffIn[PASS_IDX].cbBuffer     = 0;
        _SecBuffIn[PASS_IDX].BufferType   = SECBUFFER_TOKEN;
    }

    if (dwISCMode == ISC_MODE_UI)
        *pdwSecFlags = ISC_REQ_PROMPT_FOR_CREDS;
        
    // Out Buffer.
    _SecBuffOutDesc.cBuffers    = 1;
    _SecBuffOutDesc.pBuffers    = _SecBuffOut;
    _SecBuffOut[0].pvBuffer     = szOutBuf;
    _SecBuffOut[0].cbBuffer     = cbOutBuf;
    _SecBuffOut[0].BufferType   = SECBUFFER_TOKEN;
}


/*---------------------------------------------------------------------------
    Constructor
---------------------------------------------------------------------------*/
DIGEST_CTX::DIGEST_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy,
                 SPMData *pSPM, PWC* pPWC)
    : AUTHCTX(pSPM, pPWC)
{
    SECURITY_STATUS ssResult;
    _fIsProxy = fIsProxy;
    _pRequest = pRequest;

    _szAlloc      = NULL;
    _szData       = NULL;
    _pvContext    = NULL;
    _szRequestUri = NULL;
    _cbData       = 0;
    _cbContext    = 0;
    _nRetries     = 0;
    
    
    // Zero out the security buffers and request context.
    memset(&_SecBuffInDesc,  0, sizeof(_SecBuffInDesc));
    memset(&_SecBuffOutDesc, 0, sizeof(_SecBuffInDesc));
    memset(_SecBuffIn,       0, sizeof(_SecBuffIn));
    memset(_SecBuffOut,      0, sizeof(_SecBuffOut));
    memset(&_hCtxt,          0, sizeof(_hCtxt));
        
    // Is this the first time that the digest SSPI package
    // is being called for this process.
    if (!g_pFuncTbl)
    {
        // Get the global SSPI dispatch table.
        GetFuncTbl();

        DIGEST_PKG_DATA             PkgData;
        SEC_WINNT_AUTH_IDENTITY_EXA SecIdExA;

        // Logon with szAppCtx = szUserCtx = NULL.
        PkgData.szAppCtx = PkgData.szUserCtx = NULL;
        memset(&SecIdExA, 0, sizeof(SEC_WINNT_AUTH_IDENTITY_EXA));

        SecIdExA.Version = sizeof(SEC_WINNT_AUTH_IDENTITY_EXA);
        SecIdExA.User = (unsigned char*) &PkgData;
        SecIdExA.UserLength = sizeof(DIGEST_PKG_DATA);
        
        // Get the global credentials handle.
        ssResult = (*(g_pFuncTbl->AcquireCredentialsHandleA))
            (NULL, "Digest", SECPKG_CRED_OUTBOUND, NULL, &SecIdExA, NULL, 0, &g_hCred, NULL);
    }
}


/*---------------------------------------------------------------------------
DIGEST_CTX::PromptForCreds
---------------------------------------------------------------------------*/
DWORD DIGEST_CTX::PromptForCreds(HWND hWnd)
{
    SECURITY_STATUS ssResult;
        
    // Prompt for the credentials.
    INET_ASSERT(_pvContext);
    _cbContext = OUTPUT_BUFFER_LEN;

    DWORD sf;
    InitSecurityBuffers((LPSTR) _pvContext, _cbContext, &sf, ISC_MODE_UI);

    _SecBuffIn[HWND_IDX].pvBuffer = &hWnd;
    _SecBuffIn[HWND_IDX].cbBuffer = sizeof(HWND);

    ssResult = (*(g_pFuncTbl->InitializeSecurityContextA))(&g_hCred, &_hCtxt, NULL, sf, 
        0, 0, &_SecBuffInDesc, 0, &_hCtxt, &_SecBuffOutDesc, NULL, NULL);

    _cbContext = _SecBuffOutDesc.pBuffers[0].cbBuffer;    

    if (ssResult == SEC_E_NO_CREDENTIALS)
        return ERROR_CANCELLED;

    return (DWORD) ssResult;
}


/*---------------------------------------------------------------------------
    Destructor
---------------------------------------------------------------------------*/
DIGEST_CTX::~DIGEST_CTX()
{
    if (_szAlloc)
        delete _szAlloc;

    if (_pvContext)
        delete _pvContext;

    if (_szRequestUri)
        delete _szRequestUri;
}


/*---------------------------------------------------------------------------
    PreAuthUser
---------------------------------------------------------------------------*/
DWORD DIGEST_CTX::PreAuthUser(OUT LPSTR pBuff, IN OUT LPDWORD pcbBuff)
{
    SECURITY_STATUS ssResult = SEC_E_OK;
    INET_ASSERT(_pSPMData == _pPWC->pSPM);

    // If a response has been generated copy into output buffer.
    if (_cbContext)
    {
        memcpy(pBuff, _pvContext, _cbContext);
        *pcbBuff = _cbContext;
    }
    // Otherwise attempt to preauthenticate.
    else
    {
        // Call into the SSPI package.
        DWORD sf;
        InitSecurityBuffers(pBuff, *pcbBuff, &sf, ISC_MODE_PREAUTH);
    
        ssResult = (*(g_pFuncTbl->InitializeSecurityContext))(&g_hCred, NULL, NULL, sf, 
            0, 0, &_SecBuffInDesc, 0, &_hCtxt, &_SecBuffOutDesc, NULL, NULL);
    
        *pcbBuff = _SecBuffOut[0].cbBuffer;
    }
            
    return (DWORD) ssResult;
}

/*---------------------------------------------------------------------------
    UpdateFromHeaders
---------------------------------------------------------------------------*/
DWORD DIGEST_CTX::UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy)
{
    DWORD dwError, cbExtra, dwAuthIdx;
    LPSTR szAuthHeader, szExtra, szScheme;
    LPSTR szRealm; 
    DWORD cbRealm;
    
    // Get the associated header.
    if ((dwError = FindHdrIdxFromScheme(&dwAuthIdx)) != ERROR_SUCCESS)
        goto exit;

    // If this auth ctx does not have pwc then it has been
    // just been constructed in response to a 401.
    if (!_pPWC)
    {
        // Get any realm.
        dwError = GetAuthHeaderData(pRequest, fIsProxy, "Realm", 
            &szRealm, &cbRealm, ALLOCATE_BUFFER, dwAuthIdx);

        _pPWC = FindOrCreatePWC(pRequest, fIsProxy, _pSPMData, szRealm);

        if (_pPWC)
        {
            INET_ASSERT(_pPWC->pSPM == _pSPMData);
            _pPWC->nLockCount++;
        }
        else
        {
            dwError = ERROR_INTERNET_INTERNAL_ERROR;
            goto exit;
        }
    }

    // Updating the buffer - delete old one if necessary.
    if (_szAlloc)
    {
        delete _szAlloc;
        _szAlloc = _szData = NULL;
        _cbData = 0;
    }

    // Get the entire authentication header.
    dwError = GetAuthHeaderData(pRequest, fIsProxy, NULL,
        &_szAlloc, &_cbData, ALLOCATE_BUFFER, dwAuthIdx);
    
    // Point just past scheme
    _szData = _szAlloc;
    while (*_szData != ' ')
    {
        _szData++;
        _cbData--;
    }

    // The request will be retried.
    dwError = ERROR_SUCCESS;
exit:
    return dwError;
}



/*---------------------------------------------------------------------------
    PostAuthUser
---------------------------------------------------------------------------*/
DWORD DIGEST_CTX::PostAuthUser()
{
    INET_ASSERT(_pSPMData == _pPWC->pSPM);

    DWORD dwError;
    SECURITY_STATUS ssResult;

    // Allocate an output buffer if not done so already.
    if (!_pvContext)
    {
        _pvContext = new CHAR[OUTPUT_BUFFER_LEN];
        if (!_pvContext)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
    }

    _cbContext = OUTPUT_BUFFER_LEN;


    if (_nRetries++ < MAX_SILENT_RETRIES)
    {
        // If we pre-authenticated, treat as second
        // or subsequent attempt. We depend on the
        // server correctly sending stale=FALSE (or no stale)
        // if the credentials sent during pre-auth were bad.
        // In this case the digest pkg will return SEC_E_NO_CREDENTIALS
        // and we will prompt for credentials.
        // BUGBUG - Use ApplyControlToken
        if (_nRetries == 1 && _pRequest->GetPWC())
        {
            // Increment num of retries to 2
            _nRetries++;

            // The dwLower member has to have the correct value
            // so that secur32.dll can route to correct provider.
            _hCtxt.dwLower = g_hCred.dwLower;
        }

        // Call into the SSPI package.

        DWORD sf;
        InitSecurityBuffers((LPSTR) _pvContext, _cbContext, &sf, ISC_MODE_AUTH);
        ssResult = (*(g_pFuncTbl->InitializeSecurityContext))
            (&g_hCred, (_nRetries == 1 ? NULL : &_hCtxt), NULL, sf, 
            0, 0, &_SecBuffInDesc, 0, &_hCtxt, &_SecBuffOutDesc, NULL, NULL);
        _cbContext = _SecBuffOutDesc.pBuffers[0].cbBuffer;
        
        switch(ssResult)
        {
            case SEC_E_OK:
            {
                dwError = ERROR_INTERNET_FORCE_RETRY;
                break;
            }
            case SEC_E_NO_CREDENTIALS:
            {
                dwError = ERROR_INTERNET_INCORRECT_PASSWORD;
                break;
            }
            case SEC_E_CONTEXT_EXPIRED:
            {
                dwError = ERROR_INTERNET_LOGIN_FAILURE_DISPLAY_ENTITY_BODY;
                break;
            }
            default:
                dwError = ERROR_INTERNET_LOGIN_FAILURE;
        }
    }
    else
    {
        _cbContext = 0;
        _nRetries = 0;
        dwError = ERROR_INTERNET_INCORRECT_PASSWORD;
    }

exit:
    _pRequest->SetPWC(NULL);
    return dwError;
}

/*---------------------------------------------------------------------------
    Flush creds
---------------------------------------------------------------------------*/
VOID DIGEST_CTX::FlushCreds()
{
    DWORD ssResult;
    if (g_pFuncTbl)
    {
        DWORD sf = ISC_REQ_NULL_SESSION;
        ssResult = (*(g_pFuncTbl->InitializeSecurityContext))(&g_hCred, NULL, NULL, sf, 
            0, 0, NULL, 0, NULL, NULL, NULL, NULL);
    }
}

/*---------------------------------------------------------------------------
    Logoff
---------------------------------------------------------------------------*/
VOID DIGEST_CTX::Logoff()
{
    DWORD ssResult;
    if (g_pFuncTbl)
    {
        ssResult = (*(g_pFuncTbl->FreeCredentialsHandle))(&g_hCred);
    }
}

