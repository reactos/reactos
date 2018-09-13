//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       elink.cxx
//
//  Contents:   CLinkElement & related
//
//
//----------------------------------------------------------------------------

#include "headers.hxx"      // for the world

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"     // for CBitsCtx
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"     // for CElement
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_ELINK_HXX_
#define X_ELINK_HXX_
#include "elink.hxx"        // for CLinkElement
#endif

#ifndef X_ESTYLE_HXX_
#define X_ESTYLE_HXX_
#include "estyle.hxx"       // for CStyleElement
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"      // for CTxtSite
#endif

#ifndef X_TYPES_H_
#define X_TYPES_H_
#include "types.h"          // for s_enumdeschtmlReadyState
#endif

#ifndef X_SCRPTLET_H_
#define X_SCRPTLET_H_
#include "scrptlet.h"       // for the scriptoid stuff
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#if defined(_M_ALPHA)
#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif
#endif

#define _cxx_
#include "link.hdl"

MtDefine(CLinkElement, Elements, "CLinkElement")
MtDefine(CLinkElementOnDwnChan_pbBuffer, Locals, "CLinkElement::OnDwnChan pbBuffer")
MtDefine(CLinkElementOnDwnChan_pchSrc, Locals, "CLinkElement::OnDwnChan pchSrc")
MtDefine(CLinkElementHandleLinkedObjects, Elements, "CLinkElement::HandleLinkedObjects");

EXTERN_C const GUID CLSID_ScriptletConstructor;

//+------------------------------------------------------------------------
//
//  Class: CLinkElement
//
//-------------------------------------------------------------------------

const CElement::CLASSDESC CLinkElement::s_classdesc =
{
    {
        &CLSID_HTMLLinkElement,             // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLLinkElement,              // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLLinkElement,       // _apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};


CLinkElement::CLinkElement(CDoc *pDoc)
      : CElement(ETAG_LINK, pDoc)
{
    _pStyleSheet = NULL;
    _fIsInitialized = FALSE;
    _readyStateLink = READYSTATE_UNINITIALIZED;
}

HRESULT
CLinkElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CLinkElement(pDoc);
    return *ppElementResult ? S_OK : E_OUTOFMEMORY;
}


//+----------------------------------------------------------------------------
//
//  Member:     CLinkElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-----------------------------------------------------------------------------

HRESULT
CLinkElement::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_INHERITS2(this, IUnknown, IHTMLLinkElement)
        QI_HTML_TEAROFF(this, IHTMLElement2, NULL)
        QI_HTML_TEAROFF(this, IHTMLLinkElement, NULL)
        QI_HTML_TEAROFF(this, IHTMLLinkElement2, NULL)
        QI_TEAROFF_DISPEX(this, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     InvokeExReady
//
//  Synopsis  :this is only here to handle readyState queries, everything
//      else is passed on to the super
//
//+------------------------------------------------------------------------

#ifdef USE_STACK_SPEW
#pragma check_stack(off)
#endif

STDMETHODIMP
CLinkElement::ContextThunk_InvokeExReady(DISPID dispid,
                            LCID lcid,
                            WORD wFlags,
                            DISPPARAMS *pdispparams,
                            VARIANT *pvarResult,
                            EXCEPINFO *pexcepinfo,
                            IServiceProvider *pSrvProvider)
{
    IUnknown * pUnkContext;

    // Magic macro which pulls context out of nowhere (actually eax)
    CONTEXTTHUNK_SETCONTEXT

    HRESULT  hr = S_OK;

    hr = THR(ValidateInvoke(pdispparams, pvarResult, pexcepinfo, NULL));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(ReadyStateInvoke(dispid, wFlags, _readyStateFired, pvarResult));
    if (hr == S_FALSE)
    {
        hr = THR_NOTRACE(super::ContextInvokeEx(dispid,
                                         lcid,
                                         wFlags,
                                         pdispparams,
                                         pvarResult,
                                         pexcepinfo,
                                         pSrvProvider,
                                         pUnkContext ? pUnkContext : (IUnknown*)this));
    }

Cleanup:
    RRETURN(hr);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(on)
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CLinkElement::Notify
//
//  Synopsis:   Receive notifications
//
//----------------------------------------------------------------------------

void
CLinkElement::Notify(CNotification *pNF)
{
    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_STOP_1:
        if ( _pBitsCtx )
            _pBitsCtx->SetLoad( FALSE, NULL, FALSE );  // stop the directly linked stylesheet
        if ( _pStyleSheet )
            _pStyleSheet->StopDownloads( FALSE );  // if the directly linked sheet already came down,
        break;

    case NTYPE_BASE_URL_CHANGE:
        OnPropertyChange( DISPID_CLinkElement_href, ((PROPERTYDESC *)&s_propdescCLinkElementhref)->GetdwFlags());
        break;
        
    case NTYPE_ELEMENT_ENTERTREE:
        // BUGBUG: This code is moved directly from Init2.  While init
        // is always called just once, this notification will be sent
        // each time this element enters a tree.  There may be bugs
        // dealing with this notification coming at the wrong time or
        // more than once.  BTW, there is also a matching SN_EXITTREE
        // notification (jbeda)
        if(!_fIsInitialized)
        {
            HRESULT hr;
            
            hr = OnPropertyChange(DISPID_CLinkElement_href, 0);
            if (!hr)
                _fIsInitialized = TRUE;
        }
        else
        {
            // Insert the existing SS into this Markup
            CMarkup * pMarkup = GetMarkup();
            CStyleSheetArray * pStyleSheets = NULL;

            if (pMarkup && _pStyleSheet)
            {
                // Check for the temporary holding SSA
                if (_pSSATemp && (_pSSATemp == _pStyleSheet->GetSSAContainer()))
                {
                    _pSSATemp->ReleaseStyleSheet( _pStyleSheet, FALSE );

                    // The Temp SSA's work is now done.
                    _pSSATemp->CBase::PrivateRelease();
                    _pSSATemp = NULL;
                }

                THR(pMarkup->EnsureStyleSheets());

                pStyleSheets = pMarkup->GetStyleSheetArray();

                THR(pStyleSheets->AddStyleSheet(_pStyleSheet));
                THR(EnsureStyleDownload());

                // When exiting the tree the style rules are disable. Reenable them if they were
                //     not also disabled on the element.
                if(!GetAAdisabled())
                    IGNORE_HR(_pStyleSheet->ChangeStatus(CS_ENABLERULES, FALSE, NULL) );

            }
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        {
            if (_pStyleSheet)
            {
                CMarkup * pMarkup = GetMarkup();
                CStyleSheetArray * pStyleSheets = NULL;

                if (pMarkup && !(pNF->DataAsDWORD() & EXITTREE_DESTROY))
                    pStyleSheets = pMarkup->GetStyleSheetArray();

                // Tell the top-level stylesheet collection to let go of it's reference
                // Do NOT force a re-render (might be fatal if everyone's passivating around us)
                if (pStyleSheets)
                    pStyleSheets->ReleaseStyleSheet( _pStyleSheet, FALSE );
            }

            if (_dwScriptDownloadCookie)
            {
                GetMarkup()->LeaveScriptDownload(&_dwScriptDownloadCookie);
                _dwScriptDownloadCookie = NULL;
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     CLinkElement::SetBitsCtx
//
//+---------------------------------------------------------------------------
void
CLinkElement::SetBitsCtx(CBitsCtx * pBitsCtx)
{
    if (_pBitsCtx)
    {
        _pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
        _pBitsCtx->Disconnect();
        _pBitsCtx->Release();
    }

    _pBitsCtx = pBitsCtx;

    if (pBitsCtx)
    {
        pBitsCtx->AddRef();

        if (pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
            OnDwnChan(pBitsCtx);
        else
        {
            pBitsCtx->SetProgSink(Doc()->GetProgSink());
            pBitsCtx->SetCallback(OnDwnChanCallback, this);
            pBitsCtx->SelectChanges(DWNCHG_COMPLETE, 0, TRUE);
        }
    }
}

//+------------------------------------------------------------------------
//
//  Method:     CLinkElement::OnDwnChan
//
//-------------------------------------------------------------------------
void
CLinkElement::OnDwnChan(CDwnChan * pDwnChan)
{
    ULONG       ulState = _pBitsCtx->GetState();
    CDoc *      pDoc = Doc();
    CMarkup *   pMarkup = GetMarkup();
    IStream *   pStream = NULL;
    char *      pbBuffer = NULL;
    TCHAR *     pchSrc = NULL;

    Assert(pDoc);

    if (ulState & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
    {
        SetReadyStateLink(READYSTATE_COMPLETE);
        pDoc->LeaveStylesheetDownload(&_dwStyleCookie);

        if (ulState & DWNLOAD_COMPLETE)
        {
            // If unsecure download, may need to remove lock icon on Doc
            Doc()->OnSubDownloadSecFlags(_pBitsCtx->GetUrl(), _pBitsCtx->GetSecFlags());
            
            if (_pStyleSheet)
            {
                if ((S_OK == _pBitsCtx->GetStream(&pStream)))
                {
#ifdef XMV_PARSE
                    CCSSParser  parser(_pStyleSheet, NULL, IsInMarkup() && GetMarkupPtr()->IsXML());
#else
                    CCSSParser  parser(_pStyleSheet, NULL);
#endif

                    parser.LoadFromStream(pStream, pDoc->GetCodePage());

                    // (this is not always stable moment)
                    IGNORE_HR( OnCssChange(/*fStable = */ FALSE, /* fRecomputePeers = */TRUE) );
                }
            }
            else
                TraceTag((tagError, "CLinkElement::OnChan bitsctx failed to get file!"));
        }
        else
            TraceTag((tagError, "CLinkElement::OnChan bitsctx failed to complete!"));

        if (_pStyleSheet)
        {
            _pStyleSheet->CheckImportStatus();
            if (_dwScriptDownloadCookie)
            {
                Assert (pMarkup);
                pMarkup->LeaveScriptDownload(&_dwScriptDownloadCookie);
                _dwScriptDownloadCookie = NULL;
            }
        }

        _pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
    }
    else
        Assert( "Unknown result returned from CStyleSheet's bitsCtx!" && FALSE );

    ReleaseInterface(pStream);
    delete pbBuffer;
    delete pchSrc;
    return;
}


//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange
//
//  Note:       Called after a property has changed to notify derived classes
//              of the change.  All properties (except those managed by the
//              derived class) are consistent before this call.
//
//              Also, fires a property change notification to the site.
//
//-------------------------------------------------------------------------

HRESULT
CLinkElement::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    switch (dispid)
    {
    case DISPID_CLinkElement_href:
    case DISPID_CLinkElement_rel:
    case DISPID_CLinkElement_type:
        hr = HandleLinkedObjects();
        break;

    case DISPID_CElement_disabled:
        // Passing ChangeStatus() 0 means disable rules
        if(_pStyleSheet)
        {
            hr = THR( _pStyleSheet->ChangeStatus( GetAAdisabled() ? 0 : CS_ENABLERULES, FALSE, NULL ) );
            {
                hr = THR( OnCssChange(/*fStable = */ TRUE, /* fRecomputePeers = */TRUE) );
                if (hr)
                    goto Cleanup;
            }
        }
        break;

    case DISPID_CLinkElement_media:
        {
            if(_pStyleSheet)
            {
                LPCTSTR pcszMedia;

                if ( NULL == ( pcszMedia = GetAAmedia() ) )
                    pcszMedia = _T("all");

                hr = THR( _pStyleSheet->SetMediaType( TranslateMediaTypeString( pcszMedia ), FALSE ) );
                if ( !( OK( hr ) ) )
                    goto Cleanup;

                hr = THR( OnCssChange(/*fStable = */ TRUE, /* fRecomputePeers = */TRUE) );
                if (hr)
                    goto Cleanup;
            }
        }
        break;
    }

    if (OK(hr))
        hr = THR(super::OnPropertyChange(dispid, dwFlags));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLinkElement::SetActivity
//
//  Synopsis:   Turns activity on or off depending on visibility and
//              in-place activation.
//
//----------------------------------------------------------------------------

void
CLinkElement::SetActivity()
{
}

//--------------------------------------------------------------------------
//
//  Method:     CLinkElement::Passivate
//
//  Synopsis:   Shutdown main object by releasing references to
//              other objects and generally cleaning up.  This
//              function is called when the main reference count
//              goes to zero.  The destructor is called when
//              the reference count for the main object and all
//              embedded sub-objects goes to zero.
//
//              Release any event connections held by the form.
//
//--------------------------------------------------------------------------

void
CLinkElement::Passivate(void)
{
    SetBitsCtx(NULL);

    if (_pStyleSheet)
    {
        // Removed from StyleSheetArray in the ExitTree notification

        // Halt all stylesheet downloading.
        _pStyleSheet->StopDownloads( TRUE );

        // Let go of our reference
        _pStyleSheet->Release();
        _pStyleSheet = NULL;
    }

    if (_pSSATemp)
    {
        _pSSATemp->Release();
        _pSSATemp = NULL;
    }

    super::Passivate();
}

//--------------------------------------------------------------------------
//
//  Method:     CLinkElement::HandleLinkedObjects()
//
//  Helper called by OnPropertyChange.  Checks whether the attribute
//  values on the link tag require us to link to a stylesheet, and
//  does the appropriate stylesheet creation/release.
//
//--------------------------------------------------------------------------

HRESULT
CLinkElement::HandleLinkedObjects(void)
{
    HRESULT     hr = S_OK;
    CDoc *      pDoc = Doc();
    CMarkup *   pMarkup;
    LPCTSTR     szUrl = GetAAhref();
    LPCTSTR     pcszMedia;
    LPCTSTR     pcszRel;
    LINKTYPE    linktype;
    TCHAR *     pchAbsUrl = NULL;
    CStyleSheetArray *pStyleSheets;

    pMarkup = GetMarkup();

    Assert (pDoc);

    if (pMarkup)
    {
        pStyleSheets = pMarkup->GetStyleSheetArray();
    }
    else
    {
        pStyleSheets = _pSSATemp;
    }

    linktype = GetLinkType();

    if (LINKTYPE_STYLESHEET != linktype || !szUrl || !(*szUrl))
    {
        // If we get here, it means the attributes on the LINK do not qualify it as
        // a linked stylesheet.  We check if we have a current linked stylesheet, and
        // let it go, forcing a re-render.

        if (_pStyleSheet)
        {
            Assert(pStyleSheets);
            hr = THR(pStyleSheets->ReleaseStyleSheet(
                    _pStyleSheet,
                    TRUE));
            if (hr)
                goto Cleanup;

            _pStyleSheet->Release();
            _pStyleSheet = NULL;

            if (_pSSATemp)
            {
                Assert(!pMarkup);
                _pSSATemp->Release();
                _pSSATemp = NULL;
            }
        }

        if(LINKTYPE_STYLESHEET != linktype)
            goto Cleanup; // done
        // Fall through for the empty href case, some pages need to have an empty stylesheet 
    }

    // If we get here, it means the attributes on the LINK qualify it as a linked stylesheet.
    SetReadyStateLink( READYSTATE_LOADING );

    // If we're already ref'ing a stylesheet, then it means that the HREF changed (most likely)
    // or there was no TYPE property and now there is (unlikely, in which case the following work
    // is wasted).  We reload our current stylesheet object with the href.
    if ( _pStyleSheet )
    {
        CDoc   *pRootDoc;

        pRootDoc = pDoc->GetRootDoc();

        // If we're in designMode and the don't downloadCSS flag is set on the main doc
        // then don't initiate download.  Probably thicket saving.
        if (!(pRootDoc->DesignMode() && pRootDoc->_fDontDownloadCSS))
            hr = _pStyleSheet->LoadFromURL( szUrl, TRUE );
    }
    // We aren't already ref'ing a stylesheet, so we need a new stylesheet object.
    else
    {
        long nSSInHead = -1;        // default to append

        if (pMarkup)
        {
            hr = pMarkup->EnsureStyleSheets();
            if ( hr )
                goto Cleanup;

            // Figure out where this <link> stylesheet lives (i.e. what should its index in the
            // stylesheet collection be?).  We only need to do this if we are turning an existing
            // link into a stylesheet link -- if this link is in the process of being constructed
            // then we're guaranteed the stylesheet belongs at the end.

            if ( _fIsInitialized )
            {
                Assert( pMarkup );
                CTreeNode *pNode;
                CLinkElement *pLink;
                CStyleElement *pStyle;

                Assert( pMarkup->GetHeadElement() );

                nSSInHead = 0;
            
                CChildIterator ci ( pMarkup->GetHeadElement() );

                while ( (pNode = ci.NextChild() ) != NULL )
                {
                    if ( pNode->Tag() == ETAG_LINK )
                    {
                        pLink = DYNCAST( CLinkElement, pNode->Element() );
                        if ( pLink == this )
                            break;
                        else if ( pLink->_pStyleSheet ) // faster than IsLinkedStyleSheet() and adequate here
                            ++nSSInHead;
                    }
                    else if ( pNode->Tag() == ETAG_STYLE )
                    {
                        pStyle = DYNCAST( CStyleElement, pNode->Element() );
                        if ( pStyle->_pStyleSheet ) // Not all STYLE elements create a SS.
                            ++nSSInHead;
                    }
                }
            }

            pStyleSheets = pMarkup->GetStyleSheetArray();
        }
        else
        {
            if (!_pSSATemp)
            {
                pStyleSheets = new CStyleSheetArray( NULL, NULL, 0 );
                if (!pStyleSheets || pStyleSheets->_fInvalid )
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                _pSSATemp = pStyleSheets;
            }
            else
                pStyleSheets = _pSSATemp;

            nSSInHead = pStyleSheets->Size();
        }

        hr = pStyleSheets->CreateNewStyleSheet( this, &_pStyleSheet, nSSInHead );
        if ( hr )
            goto Cleanup;

        _pStyleSheet->AddRef(); // since the link elem is hanging onto the stylesheet ptr
                                // Note this results in a subref on us.

        if ( szUrl && szUrl[0] )
        {
            TCHAR   cBuf[pdlUrlLen];

            Assert(!_pStyleSheet->_achAbsoluteHref && "absoluteHref already computed.");

            hr = pDoc->ExpandUrl(szUrl, ARRAY_SIZE(cBuf), cBuf, NULL);
            if (hr)
                goto Cleanup;

            MemAllocString(Mt(CLinkElementHandleLinkedObjects), cBuf, &_pStyleSheet->_achAbsoluteHref);
            if (_pStyleSheet->_achAbsoluteHref == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            hr = EnsureStyleDownload();
        }
    }

    if ( NULL != (pcszMedia = GetAAmedia() ) )
    {
        hr = THR( _pStyleSheet->SetMediaType( TranslateMediaTypeString( pcszMedia ), FALSE ) );
        if (hr)
            goto Cleanup;
    }

    pcszRel = GetAArel();
    if ( GetAAdisabled() || ( pcszRel && !StrCmpIC(_T("alternate stylesheet"), pcszRel ) ) )
    {
        hr = THR( _pStyleSheet->ChangeStatus( 0, FALSE, NULL ) );   // 0 means disable rules
    }

Cleanup:
    delete pchAbsUrl;
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLinkElement::EnsureStyleDownload
//
//----------------------------------------------------------------------------

HRESULT
CLinkElement::EnsureStyleDownload()
{
    HRESULT     hr;
    CDoc *      pDoc = Doc();
    CBitsCtx *  pBitsCtx = NULL;

    hr = THR(pDoc->NewDwnCtx(DWNCTX_BITS, _pStyleSheet->_achAbsoluteHref,
                this, (CDwnCtx **)&pBitsCtx));

    pDoc->EnterStylesheetDownload(&_dwStyleCookie);

    if (IsInMarkup())
    {
        GetMarkup()->EnterScriptDownload(&_dwScriptDownloadCookie);
        Assert (_dwScriptDownloadCookie);
    }

    SetBitsCtx(pBitsCtx);                                   // Save the bits context

    if (pBitsCtx)
        pBitsCtx->Release();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLinkElement::OnReadyStateChange
//
//----------------------------------------------------------------------------

void
CLinkElement::OnReadyStateChange()
{   // do not call super::OnReadyStateChange here - we handle firing the event ourselves
    SetReadyStateLink(_readyStateLink);
}

//+------------------------------------------------------------------------
//
//  Member:     CLinkElement::SetReadyStateLink
//
//  Synopsis:   Use this to set the ready state;
//              it fires OnReadyStateChange if needed.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CLinkElement::SetReadyStateLink(long readyStateLink)
{
    long readyState;

    _readyStateLink = readyStateLink;

    readyState = min ((long)_readyStateLink, super::GetReadyState());

    if ((long)_readyStateFired != readyState)
    {
        _readyStateFired = readyState;

        Fire_onreadystatechange();

        if (_readyStateLink == READYSTATE_COMPLETE)
            Fire_onload();
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLinkElement:get_readyState
//
//+------------------------------------------------------------------------------

HRESULT
CLinkElement::get_readyState(BSTR * p)
{
    HRESULT hr = S_OK;

    if ( !p )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR( s_enumdeschtmlReadyState.StringFromEnum(_readyStateFired, p) );

Cleanup:
    RRETURN( SetErrorInfo(hr) );
}

HRESULT
CLinkElement::get_readyState(VARIANT * pVarRes)
{
    HRESULT hr = S_OK;

    if (!pVarRes)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = get_readyState(&V_BSTR(pVarRes));
    if (!hr)
        V_VT(pVarRes) = VT_BSTR;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CLinkElement::get_readyStateValue(long *plRetValue)
{
    HRESULT     hr = S_OK;

    if (!plRetValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plRetValue = _readyStateFired;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------
//
//  Member:     CLinkElement:get_styleSheet
//
//+------------------------------------------------------------------------------

HRESULT
CLinkElement::get_styleSheet(IHTMLStyleSheet** ppHTMLStyleSheet)
{
    HRESULT hr = S_OK;

    if (!ppHTMLStyleSheet)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHTMLStyleSheet = NULL;

    if ( _pStyleSheet )
    {
        hr = _pStyleSheet->QueryInterface(IID_IHTMLStyleSheet,
                                              (void**)ppHTMLStyleSheet);
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}



//+----------------------------------------------------------------------------
//
//  Member:     CLinkElement::GetLinkType
//
//  Tests the attributes of the LINK element to determine what type link this is
//
//+------------------------------------------------------------------------------

CLinkElement::LINKTYPE
CLinkElement::GetLinkType()
{
    LPCTSTR     pchHref = GetAAhref();
    LPCTSTR     pchRel = GetAArel();
    LPCTSTR     pchType = GetAAtype();
    CTreeNode * pNodeContext = GetFirstBranch();

    if (!pNodeContext || (!IsInPrimaryMarkup() &&
        pNodeContext->Parent()->Tag() == ETAG_HEAD))
        return LINKTYPE_UNKNOWN;

    if (pchHref && pchRel)
    {
        if (0 == StrCmpIC(_T("stylesheet"), pchRel) ||          // if rel = "stylesheet"
            0 == StrCmpIC(_T("alternate stylesheet"), pchRel))  // or rel = "alternate stylesheet"
        {
            if (!pchType || 0 == StrCmpIC(_T("text/css"), pchType)) // if type = "text/css"
            {
                return LINKTYPE_STYLESHEET;                             // this is a stylesheet link
            }
        }
    }

    return LINKTYPE_UNKNOWN;
}
