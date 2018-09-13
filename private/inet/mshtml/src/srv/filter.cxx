//+---------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//   File:      filter.cxx
//
//  Contents:   Http Filter object implementation
//
//  Classes:    CHttpFilter
//
//  History:   03-Sep-98   tomfakes  Created
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MSHTMSRV_HXX_
#define X_MSHTMSRV_HXX_
#include "mshtmsrv.hxx"
#endif


DWORD   CIISFilter::s_dwNotifyFlags = 
        SF_NOTIFY_ORDER_DEFAULT |
        SF_NOTIFY_PREPROC_HEADERS |
        SF_NOTIFY_URL_MAP;

ULONG   CIISFilter::s_ulTotalRequests = 0;
ULONG   CIISFilter::s_ulRequestsFoundInCache = 0;
ULONG   CIISFilter::s_ulRequestsToTrident = 0;

//+----------------------------------------------------------------------------
//
// Method: GetVersion
//
//+----------------------------------------------------------------------------
BOOL
CIISFilter::GetVersion(HTTP_FILTER_VERSION *pVer)
{
    BOOL        fRet = TRUE;
    DWORD       dwCurrentVersion = HTTP_FILTER_REVISION;    // Bug in httpfilt.h

    if (pVer->dwServerFilterVersion < dwCurrentVersion)
    {
        fRet = FALSE;
        goto Cleanup;
    }

    pVer->dwFilterVersion       = dwCurrentVersion;
    strcpy(pVer->lpszFilterDesc, "MSHTMSRV 1.0");

    pVer->dwFlags = s_dwNotifyFlags;

Cleanup:
    return fRet;
}


//+----------------------------------------------------------------------------
//
// Method: Terminate
//
//+----------------------------------------------------------------------------
BOOL
CIISFilter::Terminate(DWORD)
{
    Assert(g_pApp);
    g_pApp->ReleaseApp();

    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Method: FilterProc
//
//+----------------------------------------------------------------------------
DWORD           
CIISFilter::FilterProc(HTTP_FILTER_CONTEXT *pctx, DWORD dwNotifType, LPVOID pvNotification)
{
    DWORD       dwRet;
    HRESULT     hr = S_OK;

    // Only handle the notifications we registered for
    Assert((dwNotifType & s_dwNotifyFlags) == dwNotifType);
    Assert(g_pApp);

    switch (dwNotifType)
    {
    case SF_NOTIFY_PREPROC_HEADERS:
        hr = THR(PreProcHeaders(pctx, (HTTP_FILTER_PREPROC_HEADERS *) pvNotification));
        if (hr)
            goto Cleanup;

        break;

    case SF_NOTIFY_URL_MAP:
        hr = THR(UrlMap(pctx, (HTTP_FILTER_URL_MAP *) pvNotification));
        if (hr)
            goto Cleanup;

        break;

    default:
        Assert(FALSE);
        break;
    }

Cleanup:
    if (!hr)
    {
        dwRet = SF_STATUS_REQ_NEXT_NOTIFICATION;
    }
    else
    {
        dwRet = SF_STATUS_REQ_ERROR;
        SetLastError(hr);
    }

    return dwRet;
}


//+----------------------------------------------------------------------------
//
// Method: PreProcHeaders
//
//+----------------------------------------------------------------------------
HRESULT         
CIISFilter::PreProcHeaders(HTTP_FILTER_CONTEXT *pContext, HTTP_FILTER_PREPROC_HEADERS *pPreProc)
{
    HRESULT         hr = E_FAIL;    // BUGBUG (tomfakes) better error code?
    CHAR            chUserAgent[256];
    DWORD           dwUA;
    ULONG           ulSize;
    BOOL            fRet;
        
    ulSize = sizeof(chUserAgent);
    fRet = pPreProc->GetHeader(pContext, "HTTP_USER_AGENT:", chUserAgent, &ulSize);
    if (!fRet) 
    {
        chUserAgent[0] = '\0';
    }

    // normalize UA
    fRet = g_pApp->NormalizeUA(chUserAgent, &dwUA);
    if (!fRet)
        goto Cleanup;

    if (dwUA != USERAGENT_IE5)
    {
        CHAR            chUrl[MAX_URL_LENGTH];
        ULONG           ulSize;
        BOOL            fRet;
        
        ulSize = sizeof(chUrl);
        fRet = pPreProc->GetHeader(pContext, "url", chUrl, &ulSize);
        if (!fRet) 
        {
            chUrl[0] = '\0';
        }

        //
        // Only do this stuff for HTML files, don't intercept ISAPI or CGI or ASP...
        //
        if (IsStaticHTMLFile(chUrl))
        {
            // If the HTML is cached, we need to make that call here.
            if (g_pApp->IsHTMLCached(chUrl))
            {
                //
                // TODO Redirect to the cached HTML
                //
                s_ulRequestsFoundInCache++;
            }
            else
            {
                //
                // Redirect to the Extension to invoke Trident
                //
                long    lExtLen = lstrlenA(g_pApp->GetExtensionUrl());

                ulSize++;       // Catch the trailing \0
                Assert((ulSize + lExtLen) < MAX_URL_LENGTH);
                memmove(chUrl +  lExtLen, chUrl, ulSize);
                memcpy(chUrl, (LPBYTE) g_pApp->GetExtensionUrl(), lExtLen);

                fRet = pPreProc->SetHeader(pContext, "url", chUrl);
                if (!fRet)
                    goto Cleanup;

                s_ulRequestsToTrident++;
            }
        }
    }
    // else - do nothing, there will be no re-direct and no cache lookup

    hr = S_OK;
    s_ulTotalRequests++;

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
// Method: UrlMap
//
//  This method can only be used to map physical paths of the same type, ie
//  static file to static file.  This method is only used in the case where
//  we find the item in the cache and need to return the path to that file
//  as the physical file for this url
//
//+----------------------------------------------------------------------------
HRESULT         
CIISFilter::UrlMap(HTTP_FILTER_CONTEXT *pContext, HTTP_FILTER_URL_MAP *pUrlMap)
{
    HRESULT         hr = S_OK;

//Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
// Method: IsStaticHTMLFile
//
//+----------------------------------------------------------------------------
BOOL
CIISFilter::IsStaticHTMLFile(LPCSTR pszUrl)
{
    //BUGBUG (tomfakes) Make this a better test
    if (strncmp(pszUrl, g_pApp->GetExtensionUrl(), lstrlenA(g_pApp->GetExtensionUrl())))
        return FALSE;
    else
        return TRUE;
}
