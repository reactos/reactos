#include <wininetp.h>
#include <urlmon.h>
#include <splugin.hxx>
#include "htuu.h"

/*---------------------------------------------------------------------------
BASIC_CTX
---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
    Constructor
---------------------------------------------------------------------------*/
BASIC_CTX::BASIC_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy, 
                    SPMData* pSPM, PWC* pPWC)
    : AUTHCTX(pSPM, pPWC)
{
    _fIsProxy = fIsProxy;
    _pRequest = pRequest;
}


/*---------------------------------------------------------------------------
    Destructor
---------------------------------------------------------------------------*/
BASIC_CTX::~BASIC_CTX()
{}


/*---------------------------------------------------------------------------
    PreAuthUser
---------------------------------------------------------------------------*/
DWORD BASIC_CTX::PreAuthUser(IN LPSTR pBuf, IN OUT LPDWORD pcbBuf)
{
    if (!_pPWC->lpszUser || !_pPWC->lpszPass)
        return ERROR_INVALID_PARAMETER;
            
    // Prefix the header value with the auth type.
    const static BYTE szBasic[] = "Basic ";

    #define BASIC_LEN sizeof(szBasic)-1

    memcpy (pBuf, szBasic, BASIC_LEN);
    pBuf += BASIC_LEN;
    
    // Generate rest of header value by uuencoding user:pass.
    char szUserPass[2 * MAX_FIELD_LENGTH + 2];
    DWORD cbUserPass;

    cbUserPass = wsprintf(szUserPass, "%s:%s", _pPWC->lpszUser, _pPWC->lpszPass);
    
    INET_ASSERT (cbUserPass < sizeof(szUserPass));

    HTUU_encode ((PBYTE) szUserPass, cbUserPass,
        pBuf, *pcbBuf);

    *pcbBuf = BASIC_LEN + lstrlen (pBuf);
    
    _pvContext = (LPVOID) 1;
    return ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------
    UpdateFromHeaders
---------------------------------------------------------------------------*/
DWORD BASIC_CTX::UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy)
{
    DWORD dwAuthIdx, cbRealm, dwError;
    LPSTR szRealm = NULL;
    
    // Get the associated header.
    if ((dwError = FindHdrIdxFromScheme(&dwAuthIdx)) != ERROR_SUCCESS)
        goto exit;

    // Get any realm.
    dwError = GetAuthHeaderData(pRequest, fIsProxy, "Realm", 
        &szRealm, &cbRealm, ALLOCATE_BUFFER, dwAuthIdx);

    // No realm is OK.
    if (dwError != ERROR_SUCCESS)
        szRealm = NULL;

    // If we already have a pwc, ensure that the realm matches. If not,
    // find or create a new one and set it in the auth context.
    if (_pPWC)
    {
        if (_pPWC->lpszRealm && szRealm && lstrcmp(_pPWC->lpszRealm, szRealm))
        {
            // Realms don't match - create a new pwc entry, release the old.
            _pPWC->nLockCount--;
            _pPWC = FindOrCreatePWC(pRequest, fIsProxy, _pSPMData, szRealm);
            INET_ASSERT(_pPWC->pSPM == _pSPMData);
            _pPWC->nLockCount++;
        }
    }
    // If no password cache is set in the auth context,
    // find or create one and set it in the auth context.
    else
    {            
        // Find or create a password cache entry.
        _pPWC = FindOrCreatePWC(pRequest, fIsProxy, _pSPMData, szRealm);
        if (!_pPWC)
        {
            dwError = ERROR_INTERNET_INTERNAL_ERROR;
            goto exit;
        }
        INET_ASSERT(_pPWC->pSPM == _pSPMData);
        _pPWC->nLockCount++;
    }

    if (!_pPWC)
    {
        INET_ASSERT(FALSE);
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
        goto exit;
    }

    dwError = ERROR_SUCCESS;
        
    exit:

    if (szRealm)
        delete []szRealm;

    return dwError;
}


/*---------------------------------------------------------------------------
    PostAuthUser
---------------------------------------------------------------------------*/
DWORD BASIC_CTX::PostAuthUser()
{
    DWORD dwRet;

    if (! _pvContext && !_pRequest->GetPWC() 
        && _pPWC->lpszUser && _pPWC->lpszPass)
        dwRet = ERROR_INTERNET_FORCE_RETRY;
    else
        dwRet = ERROR_INTERNET_INCORRECT_PASSWORD;

    _pRequest->SetPWC(NULL);
    _pvContext = (LPVOID) 1;
    return dwRet;
}




