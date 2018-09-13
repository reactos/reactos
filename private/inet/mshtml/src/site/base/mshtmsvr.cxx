//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       mshtmsvr.cxx
//
//  Contents:   Implementation for server-side trident
//
//  History:    04-23-98    AnandRa     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MSHTMSVR_H_
#define X_MSHTMSVR_H_
#include "mshtmsvr.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_MSHTMSVR_HXX_
#define X_MSHTMSVR_HXX_
#include "mshtmsvr.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

extern HRESULT CreateStreamOnFile(
        LPCTSTR lpstrFile,
        DWORD dwSTGM,
        LPSTREAM * ppstrm);
EXTERN_C const GUID CLSID_HTMLServerDoc;


#define SVRTRI_BUFSIZE      4096
#define STRLEN_MSIE     5

MtDefine(CDocSvr, CDoc, "CDocSvr")
PerfDbgTag(tagCDocSvrNoClone, "CDocSvr", "Disable cloning")


//+------------------------------------------------------------------------
//
//  Member:     AddServerDoc
//
//  Synopsis:   Add this document to the TLS cache.
//
//  Notes:      This will null out the passed in document.  Use
//              GetServerDoc to retrieve the doc.  This is for
//              refcount bookkeeping.  
//
//-------------------------------------------------------------------------

HRESULT
AddServerDoc(CDocSvr **ppDoc)
{
    HRESULT hr = S_OK;

    if (!*ppDoc)
        goto Cleanup;
        
    if (!TLS(paryDocSvr))
    {
        TLS(paryDocSvr) = new CTlsDocSvrAry;
        if (!TLS(paryDocSvr))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR(TLS(paryDocSvr)->Append(*ppDoc));
    if (hr)
        goto Cleanup;

    (*ppDoc)->AddRef();
    *ppDoc = NULL;
    
Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     GetServerDoc
//
//  Synopsis:   Get a document of the passed in filename frm the TLS cache.
//
//-------------------------------------------------------------------------

void
GetServerDoc(TCHAR *pchFileName, CDocSvr **ppDoc)
{
    long        i;
    TCHAR       achPath[MAX_PATH];
    TCHAR *     pchExt;
    LPOLESTR    pchFile = NULL;
    
    if (TLS(paryDocSvr))
    {
        for (i = 0; i < TLS(paryDocSvr)->Size(); i++)
        {
            GetFullPathName(pchFileName, ARRAY_SIZE(achPath), achPath, &pchExt);
            if (OK((*TLS(paryDocSvr))[i]->GetCurFile(&pchFile)))
            {
                if (_tcsiequal(achPath, pchFile))
                {
                    *ppDoc = (*TLS(paryDocSvr))[i];
                    CoTaskMemFree(pchFile);
                    return;
                }
            }
            CoTaskMemFree(pchFile);
            pchFile = NULL;
        }
    }
}


BOOL WINAPI 
SvrTri_NormalizeUA(
    CHAR  *pchUA,                      // [in] User agent string
    DWORD *pdwUA                       // [out] User agend id
    )
{
    char *  pchMSIE = "MSIE ";
    char *  pchUAPtr = pchUA;
    long    lVer;
    long    cch;
    
    if (!pchUA || !pdwUA)
        return FALSE;

    *pdwUA = USERAGENT_DEFAULT;

    for (;;)
    {
        pchUAPtr = strchr(pchUAPtr, 'M');
        if (!pchUAPtr)
            goto Cleanup;

        cch = strlen(pchUAPtr);
        if (STRLEN_MSIE < cch &&
            CompareStringA(g_lcidUserDefault, 0, pchMSIE, STRLEN_MSIE,
                pchUAPtr, STRLEN_MSIE) == 2)
            break;
            
        // Increment pchMSIE to get to the next char beyond the 'M'
        pchUAPtr++;
    }

    pchUAPtr += 5;
    
    lVer = atol(pchUAPtr);
    if (lVer >= 5)
    {
        *pdwUA = USERAGENT_IE5;
    }
    else if (lVer == 4)
    {
        *pdwUA = USERAGENT_IE4;
    }
    else if (lVer == 3)
    {
        *pdwUA = USERAGENT_IE3;
    }
    
Cleanup:
    return TRUE;
}


HRESULT
GetServerInfo(
    VOID *                      pvSrvContext,   // [in] Server Context
    PFN_SVR_GETINFO_CALLBACK    pfnInfo,        // [in] GetInfo callback
    CStrInW *                   pcstrInUA,
    CStrInW *                   pcstrInQS,
    CStrInW *                   pcstrInURL)
{
    CHAR        pchBuffer[SVRTRI_BUFSIZE];
    HRESULT     hr = S_OK;
    ULONG       cch;
    CHAR *      pchTemp;
    
    //
    // Get the user agent
    //
    
    if (!(*pfnInfo)(pvSrvContext, SVRINFO_USERAGENT, pchBuffer, SVRTRI_BUFSIZE))
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    pcstrInUA->Init(pchBuffer, -1);

    //
    // Get the Query string
    //
    
    if (!(*pfnInfo)(pvSrvContext, SVRINFO_QUERY_STRING, pchBuffer, SVRTRI_BUFSIZE))
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    pcstrInQS->Init(pchBuffer, -1);

    //
    // Get the url.  This is retrieved by combining the protocol with 
    // the virtual path.
    //
    
    if (!(*pfnInfo)(pvSrvContext, SVRINFO_PROTOCOL, pchBuffer, 20))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Strp out /1.1 if it exists.  E.g. HTTP/1.1
    //
    
    pchTemp = strchr(pchBuffer, '/');
    cch = pchTemp ? pchTemp - pchBuffer : strlen(pchBuffer);
    strcpy(pchBuffer + cch, "://");
    cch += 3;

    //
    // Now get host name
    //
    
    if (!(*pfnInfo)(
            pvSrvContext, 
            SVRINFO_HOST, 
            pchBuffer + cch, 
            SVRTRI_BUFSIZE - cch))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    cch = strlen(pchBuffer);
    pchBuffer[cch] = '/';
    cch++;

    //
    // Finally get virtual path
    //
    
    if (!(*pfnInfo)(
            pvSrvContext, 
            SVRINFO_PATH, 
            pchBuffer + cch, 
            SVRTRI_BUFSIZE - cch))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pcstrInURL->Init(pchBuffer, -1);

Cleanup:
    RRETURN(hr);
}


HRESULT
RetrieveDocument(TCHAR *pchFileName, IDispatch *pDisp, CDocSvr **ppDoc)
{
    HRESULT     hr = S_OK;
    MSG         msg;
    IUnknown *  pUnk = NULL;
    CDocSvr *   pDoc;
    CDocSvr *   pDocSvrToClone = NULL;
    IUnknown *  pUnk2 = NULL;
    BOOL        fAdd = FALSE;
    
    hr = THR(CoCreateInstance(
            CLSID_HTMLServerDoc,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IUnknown,
            (void **)&pUnk));
    if (hr)
        goto Cleanup;
    
    hr = THR(pUnk->QueryInterface(CLSID_HTMLServerDoc, (void **)&pDoc));
    if (hr)
        goto Cleanup;

    *ppDoc = pDoc;
    pUnk = NULL;        // Transfer ref over to *ppDoc
    
    //
    // Set up the doc with the initial settings.
    //
    
    pDoc->SetLoadfFromPrefs();
    pDoc->_fGotAmbientDlcontrol = TRUE;
    pDoc->_dwLoadf = 
                DLCTL_NO_SCRIPTS | 
                DLCTL_NO_JAVA |
                DLCTL_NO_RUNACTIVEXCTLS |
                DLCTL_NO_DLACTIVEXCTLS |
                DLCTL_NO_FRAMEDOWNLOAD |
                DLCTL_NO_CLIENTPULL |
                DLCTL_SILENT;
    pDoc->_fNoFixupURLsOnPaste = TRUE;
    
    if (pDisp)
    {
        pDoc->_pDispSvr = pDisp;
        pDisp->AddRef();
    }

#if DBG==1 || defined(PERFTAGS)
    if (!IsPerfDbgEnabled(tagCDocSvrNoClone))
#endif
    {
        GetServerDoc(pchFileName, &pDocSvrToClone);
    }

    if (!pDocSvrToClone)
    {
        //
        // Didn't find the requested document in the cache.  Create it 
        // and put it into the cache.
        //
        
        hr = THR(CoCreateInstance(
                CLSID_HTMLServerDoc,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IUnknown,
                (void **)&pUnk2));
        if (hr)
            goto Cleanup;
        
        hr = THR(pUnk2->QueryInterface(
                CLSID_HTMLServerDoc, (void **)&pDocSvrToClone));
        if (hr)
            goto Cleanup;

        //
        // Set up the doc with the initial settings.
        //
        
        pDocSvrToClone->SetLoadfFromPrefs();
        pDocSvrToClone->_fGotAmbientDlcontrol = TRUE;
        pDocSvrToClone->_dwLoadf = pDoc->_dwLoadf | DLCTL_NO_BEHAVIORS;
        pDocSvrToClone->_fNoFixupURLsOnPaste = TRUE;
    
        //
        // Now load the document with this file.
        //

        hr = THR(pDocSvrToClone->Load(pchFileName, 0));
        if (hr)
            goto Cleanup;
            
        //
        // Push a loop till we're done.  This api is synchronous.
        //
        
        while (pDocSvrToClone->_pPrimaryMarkup->LoadStatus() < LOADSTATUS_DONE)
        {
            ::GetMessage(&msg, NULL, 0, 0);
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

#if DBG==1 || defined(PERFTAGS)
        if (!IsPerfDbgEnabled(tagCDocSvrNoClone))
#endif
        {
            fAdd = TRUE;
        }
    }
    
    Assert(pDocSvrToClone);
    hr = THR(pDocSvrToClone->Clone(pDoc));
    if (hr)
        goto Cleanup;

    if (fAdd)
    {
        hr = THR(AddServerDoc(&pDocSvrToClone));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pUnk);
    ReleaseInterface(pUnk2);
    RRETURN(hr);
}

void WINAPI
SvrTri_ClearCache()
{
    extern DWORD g_dwTls;
    CPtrAry<CDocSvr *> *pary;
    
    if (!TlsGetValue(g_dwTls))
        return;

    pary = TLS(paryDocSvr);
    if (pary)
    {
        TLS(paryDocSvr) = NULL;
        pary->ReleaseAll();
        delete pary;
    }
}


BOOL WINAPI
SvrTri_GetDLText(
    VOID *pvSrvContext,                // [in] Server Context
    DWORD dwUA,                        // [in] User Agent
    CHAR *pchFileName,                 // [in] Physical file name of htm file
    IDispatch *pDisp,                  // [in] OA 'Server' object for scripting
    PFN_SVR_GETINFO_CALLBACK pfnInfo,  // [in] GetInfo callback
    PFN_SVR_MAPPER_CALLBACK pfnMapper, // [in] Mapper callback
    PFN_SVR_WRITER_CALLBACK pfnWriter, // [in] Writer callback
    DWORD *rgdwUAEquiv,                // [in, out] Array of ua equivalences
    DWORD cUAEquivMax,                 // [in] Size of array of ua equiv
    DWORD *pcUAEquiv                   // [out] # of UA Equivalencies filled in
    )
{
    if (pcUAEquiv)
    {
        *pcUAEquiv = 0;
    }
    
    if (dwUA >= USERAGENT_IE5)
        return TRUE;

    CEnsureThreadState ets;
    if (FAILED(ets._hr))
        return FALSE;
        
    HRESULT     hr = S_OK;
    CDocSvr *   pDoc = NULL;
    CStrInW     cstrInFile(pchFileName);

    CoInitialize(NULL);

    //
    // Retrieve the document first.
    //

    hr = THR(RetrieveDocument(cstrInFile, pDisp, &pDoc));
    if (hr)
        goto Cleanup;
        
    //
    // Now write it out the contents of pDoc.
    //

    if (pfnWriter)
    {
        hr = THR(pDoc->DoSave(pvSrvContext, pfnWriter));
        if (hr)
            goto Cleanup;
    }
    
Cleanup:
    if (pDoc)
    {
        pDoc->Release();
    }
    CoUninitialize();
        
    return hr ? FALSE : TRUE;
}


BEGIN_TEAROFF_TABLE_(CDocSvr, IServiceProvider)
        TEAROFF_METHOD(CDocSvr, QueryService, queryservice, (REFGUID guidService, REFIID riid, void **ppvObject))
END_TEAROFF_TABLE()

//+------------------------------------------------------------------------
//
//  Member:     CreateServerDoc
//
//  Synopsis:   Creates a new server-side doc instance.
//
//  Arguments:  pUnkOuter   Outer unknown
//
//-------------------------------------------------------------------------

CBase *
CreateServerDoc(IUnknown * pUnkOuter)
{
    CBase * pBase;

    Assert(!pUnkOuter);
    pBase = new CDocSvr;
    return(pBase);
}


//+---------------------------------------------------------------
//
//  Member:     CDocSvr::PrivateQueryInterface
//
//  Synopsis:   QueryInterface on our private unknown
//
//---------------------------------------------------------------

HRESULT
CDocSvr::PrivateQueryInterface(REFIID riid, void **ppv)
{
    if (riid == CLSID_HTMLServerDoc)
    {
        *ppv = this;
        return S_OK;
    }

    return super::PrivateQueryInterface(riid, ppv);
}


//+---------------------------------------------------------------
//
//  Member:     CDocSvr::Passivate
//
//  Synopsis:   First stage destruction. 
//
//---------------------------------------------------------------

void
CDocSvr::Passivate()
{
    ClearInterface(&_pDispSvr);
    super::Passivate();
}


//+---------------------------------------------------------------
//
//  Member:     CDocSvr::DoSave
//
//  Synopsis:   Save the doc out. 
//
//---------------------------------------------------------------

HRESULT
CDocSvr::DoSave(void *pvSrvContext, PFN_SVR_WRITER_CALLBACK pfnWriter)
{
    HRESULT         hr = S_OK;
    IStream *       pStm = NULL;
    ULARGE_INTEGER  luZero = {0, 0};
    LARGE_INTEGER   lZero =  {0, 0};
    TCHAR           achFileName[MAX_PATH];
    TCHAR           achPathName[MAX_PATH];
    DWORD           dwRet;
    BYTE            pBuf[SVRTRI_BUFSIZE];
    ULONG           cbReal = 0;

#if 0    
    Assert(_pPrimaryMarkup->_LoadStatus == LOADSTATUS_DONE);
#endif

    //
    // BUGBUG: Need to fire some event here maybe?
    //
        

    dwRet = GetTempPath(ARRAY_SIZE(achPathName), achPathName);
    if (!(dwRet && dwRet < ARRAY_SIZE(achPathName)))
        goto Cleanup;

    if (!GetTempFileName(achPathName, _T("tri"), 0, achFileName))
        goto Cleanup;

    hr = THR(CreateStreamOnFile(
             achFileName,
             STGM_READWRITE | STGM_SHARE_DENY_WRITE |
                     STGM_CREATE | STGM_DELETEONRELEASE,
             &pStm));
    if (hr)
        goto Cleanup;

    hr = THR(pStm->SetSize(luZero));
    if (hr)
        goto Cleanup;

    hr = THR(SaveToStream(pStm, WBF_FORMATTED, GetCodePage()));
    if (hr)
        goto Cleanup;

    hr = THR(pStm->Seek(lZero, STREAM_SEEK_SET, NULL));
    if (hr)
        goto Cleanup;

    for (;;)
    {
        hr = THR_NOTRACE(pStm->Read(pBuf, SVRTRI_BUFSIZE, &cbReal));
        if (!cbReal || hr)
            break;
            
        (*pfnWriter)(pvSrvContext, pBuf, cbReal);
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CDocSvr::QueryService
//
//  Synopsis:   Override of super's QueryService.  
//
//---------------------------------------------------------------

HRESULT
CDocSvr::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr;
    
    hr = THR_NOTRACE(super::QueryService(guidService, riid, ppv));
    if (hr == E_NOINTERFACE)
    {
        if (_pDispSvr && guidService == SID_SServerOM)
        {
            hr = THR_NOTRACE(_pDispSvr->QueryInterface(riid, ppv));
        }
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CDocSvr::Clone
//
//  Synopsis:   Clones this document and outputs a new one.  
//              Assumption is that the new document is empty.
//
//---------------------------------------------------------------

HRESULT
CDocSvr::Clone(CDocSvr *pDoc)
{
    HRESULT             hr;
    CProgSink *         pProgSink = NULL;
    CDwnDoc *           pDwnDoc = NULL;
    
    pDwnDoc = new CDwnDoc;
    if (pDwnDoc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pDoc->_pDwnDoc = pDwnDoc;
    pDwnDoc->SetDoc(pDoc);
    pDwnDoc->SetBindf(_pDwnDoc->GetBindf());
    pDwnDoc->SetDocBindf(_pDwnDoc->GetDocBindf());
    pDwnDoc->SetDownf(_pDwnDoc->GetDownf());
    pDwnDoc->SetLoadf(_pDwnDoc->GetLoadf());
    pDwnDoc->SetRefresh(_pDwnDoc->GetRefresh());
    pDwnDoc->SetDocCodePage(_pDwnDoc->GetDocCodePage());
    pDwnDoc->SetURLCodePage(_pDwnDoc->GetURLCodePage());
    hr = THR(pDwnDoc->SetAcceptLanguage(_pDwnDoc->GetAcceptLanguage()));
    if (hr)
        goto Cleanup;
    hr = THR(pDwnDoc->SetUserAgent(_pDwnDoc->GetUserAgent()));
    if (hr)
        goto Cleanup;
    hr = THR(pDwnDoc->SetDocReferer(_pDwnDoc->GetDocReferer()));
    if (hr)
        goto Cleanup;
    hr = THR(pDwnDoc->SetSubReferer(_pDwnDoc->GetSubReferer()));
    if (hr)
        goto Cleanup;
    
    pProgSink = new CProgSink(pDoc, pDoc->_pPrimaryMarkup);
    if (pProgSink == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    Assert(!pDoc->_pPrimaryMarkup->_pProgSink);
    pDoc->_pPrimaryMarkup->_pProgSink = pProgSink;
    pProgSink = NULL;   // Transfer ref over.

    {
        CDoc * pDocSource = _pPrimaryMarkup->Doc();
        CMarkupPointer mpSourceBegin( pDocSource ), mpSourceEnd( pDocSource ), mpTarget( pDoc );

        hr = THR( mpSourceBegin.MoveToContainer( _pPrimaryMarkup, TRUE ) );
        if (hr)
            goto Cleanup;
        hr = THR( mpSourceEnd.MoveToContainer( _pPrimaryMarkup, FALSE ) );
        if (hr)
            goto Cleanup;
        hr = THR( mpTarget.MoveToContainer( pDoc->_pPrimaryMarkup, TRUE ) );
        if (hr)
            goto Cleanup;

        hr = THR( _pPrimaryMarkup->Doc()->Copy( & mpSourceBegin, & mpSourceEnd, & mpTarget ) );
        if (hr)
            goto Cleanup;
    }

    pDoc->PeerDequeueTasks(0);
    
Cleanup:
    if (pProgSink)
    {
        pProgSink->Release();
    }
    RRETURN(hr);
}

