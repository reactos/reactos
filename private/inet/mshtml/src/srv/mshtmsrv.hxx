//+------------------------------------------------------------------------
//
//  File:       mshtmsrv.hxx
//
//  Contents:   ServerApp Class definition for Trident on Server
//
//  Classes:    CServerApp, CHttpFilter, CHttpExtension
//
//-------------------------------------------------------------------------

#ifndef I_MSHTMSRV_HXX_
#define I_MSHTMSRV_HXX_

#include <httpfilt.h>
#include <httpext.h>

#include "mshtmsvr.h"

MtExtern(CHttpExtension);
MtExtern(CIISFilter);
MtExtern(CServerApp);

#define MAX_URL_LENGTH      2048

//+----------------------------------------------------------------------------
//
// Class: CIISFilter
//
//+----------------------------------------------------------------------------
class CIISFilter
{
protected:
    static DWORD    s_dwNotifyFlags;

public:
    BOOL            GetVersion(HTTP_FILTER_VERSION *pVer);
    DWORD           FilterProc(HTTP_FILTER_CONTEXT *pctx, DWORD dwNotifType, LPVOID pvNotification);
    BOOL            Terminate(DWORD dwUnused);

    HRESULT         PreProcHeaders(HTTP_FILTER_CONTEXT *pContext, HTTP_FILTER_PREPROC_HEADERS *pPreProc);
    HRESULT         UrlMap(HTTP_FILTER_CONTEXT *pContext, HTTP_FILTER_URL_MAP *pUrlMap);

    BOOL            IsStaticHTMLFile(LPCSTR pszUrl);

public:
    // Statistics
    static  ULONG   s_ulTotalRequests;
    static  ULONG   s_ulRequestsFoundInCache;       // Cache hits
    static  ULONG   s_ulRequestsToTrident;          // Requests needing Trident to be invoked
};


//+----------------------------------------------------------------------------
//
// Class: CHttpExtension
//
//+----------------------------------------------------------------------------
class CIISExtension
{
protected:
    void            ReportError(LPEXTENSION_CONTROL_BLOCK pECB, CHAR *errorText, CHAR *status);
    void            DumpDebugInfo(LPEXTENSION_CONTROL_BLOCK pECB);

public:
    BOOL            GetVersion(HSE_VERSION_INFO *pVer);
    DWORD           ExtensionProc(LPEXTENSION_CONTROL_BLOCK pECB);
    BOOL            Terminate(DWORD dwFlags);
};


// typedefs for Trident APIs to be used for dynamic linking
// with GetProcAddress()

typedef BOOL
(WINAPI *TridentNormalizeUACall)(
    CHAR  *pchUA,                      // [in] User agent string
    DWORD *pdwUA                       // [out] User agend id
    );

typedef BOOL
(WINAPI *TridentGetDLTextCall)(
    VOID *pvSrvContext,                // [in] Server Context
    DWORD dwUA,                        // [in] User Agent (Normalized)
    CHAR *pchFileName,                 // [in] Physical file name of htm file
    IDispatch *pdisp,                  // [in] OA 'Server' object for scripting
    PFN_SVR_GETINFO_CALLBACK pfnInfo,  // [in] GetInfo callback
    PFN_SVR_MAPPER_CALLBACK pfnMapper, // [in] Mapper callback
    PFN_SVR_WRITER_CALLBACK pfnWriter, // [in] Writer callback
    DWORD *rgdwUAEquiv,                // [in, out] Array of ua equivalences
    DWORD cUAEquivMax,                 // [in] Size of array of ua equiv
    DWORD *pcUAEquiv                   // [out] # of UA Equivalencies filled in
    );

//+----------------------------------------------------------------------------
//
// Class: CServerApp
//
//+----------------------------------------------------------------------------
class CServerApp
{
public:
    CServerApp();
    ~CServerApp();

    CIISFilter  * GetIISFilter()
        { return &_iisFilter; }

    CIISExtension * GetIISExtension()
        { return &_iisExtension; }

    static HRESULT AddRefApp();

    LONG    ReleaseApp();

    BOOL    NormalizeUA(CHAR *pchUA, DWORD *pdwUA)
        { Assert(_TridentNormalizeUA); return _TridentNormalizeUA(pchUA, pdwUA); }

    BOOL    IsHTMLCached(LPSTR pszUrl);

    LPSTR   GetExtensionUrl()
        { return _szExtensionUrl; }

private:
    HRESULT                     Initialize(void);

private:        // Members
    LONG                        _lRef;
    HMODULE                     _hTrident;
    TridentNormalizeUACall      _TridentNormalizeUA;
    TridentGetDLTextCall        _TridentGetDLText;

    CIISFilter                  _iisFilter;
    CIISExtension               _iisExtension;
    CHAR                        _szExtensionUrl[MAX_URL_LENGTH];
};


extern CServerApp   * g_pApp;

#pragma INCMSG("--- End 'mshtmsrv.hxx'")
#else
#pragma INCMSG("*** Dup 'mshtmsrv.hxx'")
#endif
