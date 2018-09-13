//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       jsprot.cxx
//
//  Contents:   Implementation of the javascript: protocol
//
//  History:    01-14-97    AnandRa     Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_JSPROT_HXX_
#define X_JSPROT_HXX_
#include "jsprot.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_ROSTM_HXX_
#define X_ROSTM_HXX_
#include "rostm.hxx"
#endif

#ifndef X_ENCODE_HXX_
#define X_ENCODE_HXX_
#include "encode.hxx"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_HTIFRAME_H_
#define X_HTIFRAME_H_
#include <htiframe.h>
#endif

MtDefine(CJSProtocol,  Protocols, "CJSProtocol")
MtDefine(JSProtResult, Protocols, "JavaScript protocol evaluation (temp)")
MtDefine(CJSProtocolParseAndBind_pbOutput, Protocols, "CJSProtocol::ParseAndBind pbOutput")

HRESULT VariantToPrintableString (VARIANT * pvar, CStr * pstr);

#define WRITTEN_SCRIPT _T("<<HTML><<SCRIPT LANGUAGE=<0s>>var __w=<1s>;if(__w!=null)document.write(__w);<</SCRIPT><</HTML>")

//+---------------------------------------------------------------------------
//
//  Function:   CreateJSProtocol
//
//  Synopsis:   Creates a javascript: Async Pluggable protocol
//
//  Arguments:  pUnkOuter   Controlling IUnknown
//
//----------------------------------------------------------------------------

CBase * 
CreateJSProtocol(IUnknown *pUnkOuter)
{
    return new CJSProtocol(pUnkOuter);
}


CJSProtocolCF   g_cfJSProtocol(CreateJSProtocol);

//+---------------------------------------------------------------------------
//
//  Method:     CJSProtocolCF::ParseUrl
//
//  Synopsis:   per IInternetProtocolInfo
//
//----------------------------------------------------------------------------

HRESULT
CJSProtocolCF::ParseUrl(
    LPCWSTR     pwzUrl,
    PARSEACTION ParseAction,
    DWORD       dwFlags,
    LPWSTR      pwzResult,
    DWORD       cchResult,
    DWORD *     pcchResult,
    DWORD       dwReserved)
{
    CStr    cstr;
    HRESULT hr = INET_E_DEFAULT_ACTION;

    if (!pcchResult || !pwzResult)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    if (ParseAction == PARSE_SECURITY_URL)
    {
        hr = THR(UnwrapSpecialUrl(pwzUrl, cstr));
        if (hr)
            goto Cleanup;

        *pcchResult = cstr.Length() + 1;
        
        if (cstr.Length() + 1 > cchResult)
        {
            // Not enough room
            hr = S_FALSE;
            goto Cleanup;
        }

        _tcscpy(pwzResult, cstr);
    }
    else
    {
        hr = THR_NOTRACE(super::ParseUrl(
            pwzUrl,
            ParseAction,
            dwFlags,
            pwzResult,
            cchResult,
            pcchResult,
            dwReserved));
    }
    
Cleanup:    
    RRETURN2(hr, INET_E_DEFAULT_ACTION, S_FALSE);
}


const CBase::CLASSDESC CJSProtocol::s_classdesc =
{
    &CLSID_JSProtocol,              // _pclsid
};


//+---------------------------------------------------------------------------
//
//  Method:     CJSProtocol::CJSProtocol
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CJSProtocol::CJSProtocol(IUnknown *pUnkOuter) : super(pUnkOuter)
{
}


//+---------------------------------------------------------------------------
//
//  Method:     CJSProtocol::~CJSProtocol
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CJSProtocol::~CJSProtocol()
{
}


//+---------------------------------------------------------------------------
//
//  Method:     CJSProtocol::ParseAndBind
//
//  Synopsis:   Actually perform the binding & execution of script.
//
//----------------------------------------------------------------------------

HRESULT
CJSProtocol::ParseAndBind()
{
    HRESULT         hr = S_OK;
    TCHAR *         pchScript = NULL;
    ITargetFrame2 * pTF2 = NULL;
    CDoc *          pDoc = NULL;
    IOleContainer * pOleContainer = NULL;
    CVariant        Var;
    CStr            cstrResult;
    CStr            cstrProtocol;
    CROStmOnBuffer *prostm = NULL;
    UINT            uProt;
    TCHAR *         pchSourceUrl = NULL;
    TCHAR *         pchSourceUrlTmp = NULL;
    long            i = 1;
    TCHAR *         pchOutput = NULL;
    BYTE *          pbOutput = NULL;
    long            cb = 0;
        
    
    // skip protocol part
    pchScript = _tcschr(_cstrURL, ':');
    if (!pchScript)
    {
        hr = MK_E_SYNTAX;
        goto Cleanup;
    }

    hr = THR(cstrProtocol.Set(_cstrURL, pchScript - _cstrURL));
    if (hr)
        goto Cleanup;

    // Go past the :
    pchScript++;
    
    uProt = GetUrlScheme(_cstrURL);
    Assert(URL_SCHEME_VBSCRIPT == uProt || 
           URL_SCHEME_JAVASCRIPT == uProt);
    
    //
    // Do the binding.
    //
    
    //
    // Extract out the source url.  This is appended to the
    // url with a \1.
    //

    pchSourceUrlTmp = _tcschr(pchScript, _T('\1'));
    pchSourceUrl = _tcsrchr(pchScript, _T('\1'));

    // if no source url appended, get end of string.
    if (!pchSourceUrlTmp)
        pchSourceUrlTmp = _tcsrchr(pchScript, _T('\0'));

    // just a sanity check !
    Assert(pchSourceUrlTmp);

    // remove any ';' from end of script code that is fed to doc.write to avoid
    // syntax errors
    while (*(pchSourceUrlTmp-i) == _T(';'))
        i++;

    *(pchSourceUrlTmp - --i) = 0;
    pchSourceUrl = (pchSourceUrl && *(pchSourceUrl+1)) ? pchSourceUrl+1 : NULL;

    // We can query the doc directly when we're trying to handle
    // scripting in a download task for an element of the doc.
    hr = THR(QueryService(
            CLSID_HTMLDocument,
            CLSID_HTMLDocument,
            (void **)&pDoc));

    if (hr)
    {
        // We didn't succeed, so try the old method of querying
        // up to shdocvw.
        hr = THR(QueryService(
                IID_ITargetFrame2, 
                IID_ITargetFrame2,
                (void **)&pTF2));
        if (hr)
            goto Cleanup;

        hr = THR(pTF2->GetFramesContainer(&pOleContainer));
        if (hr)
            goto Cleanup;
        
        if (pOleContainer)
        {
            //
            // Found document.  Execute script in its context.
            //

            hr = THR(pOleContainer->QueryInterface(
                    CLSID_HTMLDocument,
                    (void **)&pDoc));
            if (hr)
                goto Cleanup;
        }
    }
    if (pDoc)
    {        

        if (pDoc->_pScriptCollection)
        {
            //
            // Only allow script to execute if security context's match
            //

            if (pchSourceUrl && !pDoc->AccessAllowed(pchSourceUrl))
            {
                hr = E_ACCESSDENIED;
                goto Cleanup;
            }
            
            // Any errors from this are ignored so URLMON doesn't throw
            // an exception to IE which causes them to shutdown due to an
            // unexpected error. -- TerryLu.
            IGNORE_HR(pDoc->_pScriptCollection->ParseScriptText(
                        cstrProtocol,           // pchLanguage
                        NULL,                   // pMarkup
                        NULL,                   // pchType
                        pchScript,              // pchCode
                        DEFAULT_OM_SCOPE,       // pchItemName
                        NULL,                   // pchDelimiter
                        0,                      // ulOffset
                        0,                      // ulStartingLine
                        NULL,                   // pSourceObject
                        SCRIPTTEXT_ISVISIBLE | SCRIPTTEXT_ISEXPRESSION, // dwFlags
                        &Var,                   // pvarResult
                        NULL));                 // pExcepInfo

            if (V_VT(&Var) != VT_EMPTY)
            {
                hr = THR(VariantToPrintableString(&Var, &cstrResult));
                if (hr)
                    goto Cleanup;

                hr = THR(MemAllocString(Mt(JSProtResult), cstrResult, &pchOutput));
                if (hr)
                    goto Cleanup;
            }
            else
            {
                //
                // There was no output from the script.  Since we got back
                // a document from the target frame, abort now.
                //
                
                hr = E_ABORT;
            }
        }
    }
    else
    {
        //
        // New document.  Execute script in this new 
        // document's context.
        //

#if defined(WIN16)
        // BUGWIN16: need to investigate this random compiler quirkiness !!
        hr = THR(Format(
                FMT_OUT_ALLOC,
                &pchOutput,
                0,
                WRITTEN_SCRIPT,
                (LPCSTR)cstrProtocol,
                pchScript));
#else
        hr = THR(Format(
                FMT_OUT_ALLOC,
                &pchOutput,
                0,
                WRITTEN_SCRIPT,
                (LPTSTR)cstrProtocol,
                pchScript));
#endif
        if (hr)
            goto Cleanup;
    }

    //
    // Convert string into a stream.
    //

    if (pchOutput)
    {
        cb = WideCharToMultiByte(
                _bindinfo.dwCodePage,
                0, 
                pchOutput, 
                -1, 
                NULL, 
                0,
                NULL, 
                NULL);

        pbOutput = new(Mt(CJSProtocolParseAndBind_pbOutput)) BYTE[cb + 1];
        if (!pbOutput)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        WideCharToMultiByte(
                _bindinfo.dwCodePage,
                0, 
                pchOutput, 
                -1, 
                (char *)pbOutput, 
                cb,
                NULL, 
                NULL);
        
        prostm = new CROStmOnBuffer();
        if (!prostm)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(prostm->Init(pbOutput, cb));
        if (hr)
            goto Cleanup;
            
        _pStm = (IStream *)prostm;
        _pStm->AddRef();
    }
    
Cleanup:
    if (!_fAborted)
    {
        if (!hr)
        {
            _bscf |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
            _pProtSink->ReportData(_bscf, cb, cb);
        }
        if (_pProtSink)
        {
            _pProtSink->ReportResult(hr, 0, 0);
        }
    }

    ReleaseInterface(pTF2);
    ReleaseInterface(pOleContainer);
    if (prostm)
    {
        prostm->Release();
    }
    MemFreeString(pchOutput);
    delete pbOutput;
    RRETURN(hr);
}

