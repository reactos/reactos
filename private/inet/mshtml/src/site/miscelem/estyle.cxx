//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       estyle.cxx
//
//  Contents:   CStyleElement & related
//
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx" // for CStreamWriteBuf
#endif

#ifndef X_TYPES_H_
#define X_TYPES_H_
#include "types.h" // for s_enumdeschtmlReadyState
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_ESTYLE_HXX_
#define X_ESTYLE_HXX_
#include "estyle.hxx"
#endif

#ifndef X_ELINK_HXX_
#define X_ELINK_HXX_
#include "elink.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#if defined(_M_ALPHA)
#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif
#endif

#define _cxx_
#include "estyle.hdl"

MtDefine(CStyleElement, Elements, "CStyleElement")

const CElement::CLASSDESC CStyleElement::s_classdesc =
{
    {
        &CLSID_HTMLStyleElement,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLStyleElement,             // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLStyleElement,      // _apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};


HRESULT CStyleElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(ppElement);

    *ppElement = new CStyleElement(pDoc);
    return *ppElement ? S_OK : E_OUTOFMEMORY;
}

CStyleElement::CStyleElement(CDoc *pDoc)
    : CElement(ETAG_STYLE, pDoc)
{
    _pStyleSheet = NULL;
    _fDirty = FALSE;
    _fParseFinished = TRUE;
    _readyStateStyle = READYSTATE_UNINITIALIZED;
    _fExplicitEndTag = TRUE;
}

HRESULT
CStyleElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_HTML_TEAROFF(this, IHTMLElement2, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *) *ppv)->AddRef();
        RRETURN(S_OK);
    }

    RRETURN(super::PrivateQueryInterface(iid, ppv));
}


void
CStyleElement::Notify(CNotification *pNF)
{
    // call super (important in all cases, including ENTERTREE)
    super::Notify(pNF);

    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_ENTERTREE:
        _fEnterTreeCalled = TRUE;
        if (_fParseFinished)
        {
            // setText can only be called when both this element is in the tree
            // and the parsectx::Finish() has been called.  The reason for this 
            // is that SetText needs to set up the absolute URL path, and any
            // base tags above it might not be in the tree yet either.

            // When a STYLE sheet moves between trees (as in paste or innerHTML etc)
            // I get an exit then an enter - in this case, I don't need to 
            // re-parse the text
            if ( !_pStyleSheet )
                IGNORE_HR(SetText(_cstrText));
            else
            {
                // Insert the existing SS into this Markup
                CMarkup * pMarkup = GetMarkup();
                CStyleSheetArray * pStyleSheets = NULL;

                if (pMarkup)
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
    
                    // When exiting the tree the style rules are disable. Reenable them if they were
                    //     not also disabled on the element.
                    if(!GetAAdisabled())
                        IGNORE_HR(_pStyleSheet->ChangeStatus(CS_ENABLERULES, FALSE, NULL) );

                    IGNORE_HR(OnCssChange(/*fStable = */ FALSE, /* fRecomputePeers = */FALSE));

                }
            }
        }
        break;
    case NTYPE_STOP_1:
        // if the directly linked sheet already came down,
        // this will stop any of its imports.
        if (_pStyleSheet)
            _pStyleSheet->StopDownloads(FALSE);  

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
                if (pStyleSheets)
                    pStyleSheets->ReleaseStyleSheet( _pStyleSheet, FALSE );

                _pStyleSheet->StopDownloads(TRUE);

                if ( !(pNF->DataAsDWORD() & EXITTREE_DESTROY) ) 
                    Doc()->EnsureFormatCacheChange( ELEMCHNG_CLEARCACHES );
            }
            _fEnterTreeCalled = FALSE;
        }
    }
}


void
CStyleElement::Passivate (void)
{
    if (_pStyleSheet)
    {
        // Removed from StyleSheetArray in the ExitTree notification

        // Halt all stylesheet downloading.
        _pStyleSheet->StopDownloads( TRUE );

        // Let go of our reference
        _pStyleSheet->Release();    // this will subrel ourselves
        _pStyleSheet = NULL;
    }

    if (_pSSATemp)
    {
        _pSSATemp->Release();
        _pSSATemp = NULL;
    }

    super::Passivate();
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
CStyleElement::ContextThunk_InvokeExReady(DISPID dispid,
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

//+---------------------------------------------------------------
//
//  Member : CStyleElement::Save
//
//  Synopsis    :   Standard Save routine
//
//+---------------------------------------------------------------
HRESULT
CStyleElement::Save(CStreamWriteBuff * pStreamWriteBuff, BOOL fEnd)
{
    HRESULT hr = S_OK;
    DWORD   dwOld;

    // No styles for plaintext mode
    if (pStreamWriteBuff->TestFlag(WBF_SAVE_PLAINTEXT))
        return S_OK;

    if (!fEnd)
    {
        hr = THR(pStreamWriteBuff->NewLine());
        if(hr)
            goto Cleanup;
    }

    //
    // Save tagname and attributes.
    //

    hr = THR(super::Save(pStreamWriteBuff, fEnd));
    if (hr)
        goto Cleanup;

    if (fEnd)
    {
        // New line after </STYLE>
        hr = THR(pStreamWriteBuff->NewLine());
        goto Cleanup;
    }

    //
    // Tell the write buffer to just write this string
    // literally, without checking for any entity references.
    //

    dwOld = pStreamWriteBuff->ClearFlags(WBF_ENTITYREF);

    //
    // Tell the stream to now not perform any fancy indenting
    // or such stuff.
    //

    pStreamWriteBuff->BeginPre();

    if ( _fDirty )
    {   // This stylesheet has been touched through the OM, we need to
        // use the internal data to get the contents to persist.
        _cstrText.Free();
        if ( _pStyleSheet )
        {
            hr = _pStyleSheet->GetString( &_cstrText );
            if (hr)
                goto Cleanup;
        }
    }
    hr = THR(pStreamWriteBuff->Write((LPTSTR)_cstrText));
    if (hr)
        goto Cleanup;

    if ( _fDirty )  // Don't leave our string around
        _cstrText.Free();

    pStreamWriteBuff->EndPre();
    pStreamWriteBuff->SetFlags(dwOld);

Cleanup:
    RRETURN(hr);
}

void CStyleElement::SetDirty( void )
{
    _cstrText.Free();
    _fDirty = TRUE;
}

//+---------------------------------------------------------------
//
//  Member:     CStyleElement::SetText
//
//  Synopsis:   Sets the text owned by the style
//
//  BUGBUG: If this is called more than once per lifetime of a
//  style element, we need to fix the CreateNewStyleSheet call
//  so the old CStyleSheet is taken care of etc.  Right now the
//  assumption is that this is never called more than once
//  per lifetime of a style element.
//
//+---------------------------------------------------------------

HRESULT
CStyleElement::SetText(TCHAR *pch)
{
    CCSSParser *pcssp;
    HRESULT hr = S_OK;
    LPCTSTR szType;
    LPCTSTR pcszMedia;
    CDoc *  pDoc = Doc();
    CMarkup * pMarkup = GetMarkup();
    CStyleSheetArray *pSSA;
    TCHAR   cBuf[pdlUrlLen];

    szType = GetAAtype();
    if ( szType && StrCmpIC( _T("text/css"), szType ) )
        goto Save_Contents;

    Assert( "Already have a stylesheet on this element!" && !_pStyleSheet );

    SetReadyStateStyle( READYSTATE_LOADING );

    if (pMarkup)
    {
        hr = pMarkup->EnsureStyleSheets();
        if (hr)
            goto Cleanup;

        pSSA = pMarkup->GetStyleSheetArray();
    }
    else
    {
        pSSA = new CStyleSheetArray( NULL, NULL, 0 );
        if (!pSSA || pSSA->_fInvalid )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _pSSATemp = pSSA;
    }

    hr = THR(pSSA->CreateNewStyleSheet(this, &_pStyleSheet));
    if (hr)
        goto Cleanup;

    _pStyleSheet->AddRef(); // since the style elem is hanging onto the stylesheet ptr
                            // Note this results in a subref on us.

    // Setting the disabled status BEFORE adding all the rules (Write()ing to the parser, below)
    // is more efficient, because we don't have to walk the rules list.
    if ( GetAAdisabled() )
    {
        hr = THR( _pStyleSheet->ChangeStatus( 0, FALSE, NULL ) );   // 0 means disable rules
        if (hr)
            goto Save_Contents;
    }

    if ( NULL != ( pcszMedia = GetAAmedia() ) )
    {
        hr = THR( _pStyleSheet->SetMediaType( TranslateMediaTypeString( pcszMedia ), FALSE ) );
        if (hr)
            goto Save_Contents;
    }

    Assert(!_pStyleSheet->_achAbsoluteHref && "absoluteHref already computed.");
    hr = pDoc->ExpandUrl(_T(""), ARRAY_SIZE(cBuf), cBuf, NULL);
    if (hr)
        goto Cleanup;

    MemAllocString(Mt(CStyleElement), cBuf, &_pStyleSheet->_achAbsoluteHref);
    if (_pStyleSheet->_achAbsoluteHref == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if(pch && *pch)
    {

    #ifdef XMV_PARSE
        pcssp = new CCSSParser(_pStyleSheet, NULL, IsInMarkup() && GetMarkupPtr()->IsXML());
    #else
        pcssp = new CCSSParser(_pStyleSheet, NULL);
    #endif
        if (!pcssp)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // BUGBUG: can these fail? (dbau)
        pcssp->Open();
        pcssp->Write(pch, _tcslen(pch));
        pcssp->Close();
        delete pcssp;

        if (IsInPrimaryMarkup())
        {
            // (alexz) we don't need to do OnCssChangeStable here, and it is also unsafe to do so
            // (this is not a stable moment when we can go out to scripts and other external components) 
            IGNORE_HR(pDoc->ForceRelayout());
        }

        // _pStyleSheet might be freed by OnCssChange
        if(_pStyleSheet)
            _pStyleSheet->CheckImportStatus();
    }

Save_Contents:
    _cstrText.Set(pch);

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStyleElement::OnReadyStateChange
//
//----------------------------------------------------------------------------

void
CStyleElement::OnReadyStateChange()
{   // do not call super::OnReadyStateChange here - we handle firing the event ourselves
    SetReadyStateStyle(_readyStateStyle);
}

//+------------------------------------------------------------------------
//
//  Member:     CStyleElement::SetReadyStateStyle
//
//  Synopsis:   Use this to set the ready state;
//              it fires OnReadyStateChange if needed.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CStyleElement::SetReadyStateStyle(long readyStateStyle)
{
    long readyState;

    _readyStateStyle = readyStateStyle;

    readyState = min ((long)_readyStateStyle, super::GetReadyState());

    if ((long)_readyStateFired != readyState)
    {
        _readyStateFired = readyState;

        Fire_onreadystatechange();

        if (_readyStateStyle == READYSTATE_COMPLETE)
            Fire_onload();
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CStyleElement:get_readyState
//
//+------------------------------------------------------------------------------

HRESULT
CStyleElement::get_readyState(BSTR * p)
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
CStyleElement::get_readyState(VARIANT * pVarRes)
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

//+----------------------------------------------------------------------------
//
//  Member:     CStyleElement:get_styleSheet
//
//+------------------------------------------------------------------------------

HRESULT
CStyleElement::get_styleSheet(IHTMLStyleSheet** ppHTMLStyleSheet)
{
    HRESULT hr = S_OK;

    if (!ppHTMLStyleSheet)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHTMLStyleSheet = NULL;

    // We may not have a stylesheet if we've been passivated
    if ( _pStyleSheet )
    {
        hr = _pStyleSheet->QueryInterface(IID_IHTMLStyleSheet,
                                              (void**)ppHTMLStyleSheet);
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ));
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
CStyleElement::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    
    Assert( Doc()->_pPrimaryMarkup ); // BUGBUG (alexz) this may not always be true

    switch (dispid)
    {
    case DISPID_CElement_disabled:
        // Passing ChangeStatus() 0 means disable rules
        if ( _pStyleSheet )
        {
            hr = THR( _pStyleSheet->ChangeStatus( GetAAdisabled() ? 0 : CS_ENABLERULES, FALSE, NULL ) );
            if ( !( OK( hr ) ) )
                goto Cleanup;

            hr = THR( OnCssChange(/*fStable = */ TRUE, /* fRecomputePeers = */TRUE) );
            if (hr)
                goto Cleanup;
        }
        break;

    case DISPID_CStyleElement_type:
        {
            LPCTSTR szType = GetAAtype();

            if ( szType && StrCmpIC( _T("text/css"), szType ) )
            {
                if ( _pStyleSheet )
                {
                    CMarkup * pMarkup = GetMarkup();
                    CStyleSheetArray * pStyleSheets = NULL;
                    
                    if (pMarkup)
                        pStyleSheets = pMarkup->GetStyleSheetArray();

                    // Halt all stylesheet downloading.
                    _pStyleSheet->StopDownloads( TRUE );

                    // Tell the top-level stylesheet collection to let go of it's reference
                    if (pStyleSheets)
                        pStyleSheets->ReleaseStyleSheet( _pStyleSheet, FALSE );

                    // Let go of our reference
                    _pStyleSheet->Release();    // this will subrel ourselves
                    _pStyleSheet = NULL;

                    // Rerender, since our SS is gone.
                    
                    hr = THR( OnCssChange(/*fStable = */ TRUE, /* fRecomputePeers = */TRUE) );
                    if (hr)
                        goto Cleanup;
                }

            }
            else
            {   // We're the right type - make sure we have a stylesheet attached.
                hr = EnsureStyleSheet();
            }
        }
        break;

    case DISPID_CStyleElement_media:
        if ( _pStyleSheet )
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
        break;
    }

    hr = THR(super::OnPropertyChange(dispid, dwFlags));

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     CStyleElement:EnsureStyleSheet
//      Makes sure that we have a stylesheet built for this element.  This is
//  only called if we build a style element through a createElement call from
//  the OM, as opposed to a regular parsing pass (which will call SetText, which
//  will create our stylesheet in order automatically).
//      This particular CStyleElement should be living in the HEAD, or this function
//  may not work properly.  Note that from where this function is currently
//  called (the OM's createElement method), this is always true, so we'll throw
//  an assert if it's not true.
//
//+------------------------------------------------------------------------------
HRESULT
CStyleElement::EnsureStyleSheet( void )
{
    HRESULT hr = S_OK;

    if ( !_pStyleSheet )    // We aren't already ref'ing a stylesheet, so we need a new stylesheet object.
    {
        CMarkup *pMarkup = GetMarkup();
        CStyleSheetArray * pStyleSheets;

        if (pMarkup)
        {
            hr = pMarkup->EnsureStyleSheets();
            if ( hr )
                goto Cleanup;

            pStyleSheets = pMarkup->GetStyleSheetArray();
        }
        else
        {
            pStyleSheets = new CStyleSheetArray( NULL, NULL, 0 );
            if (!pStyleSheets || pStyleSheets->_fInvalid )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            // Track the StyleSheetArray in this element because we don't have a Markup
            _pSSATemp = pStyleSheets;
        }
        Assert(pStyleSheets);

        // Figure out where this <style> stylesheet lives (i.e. what should its index in the
        // stylesheet collection be?).

        long nSSInHead;
        CTreeNode *pNode;
        CLinkElement *pLink;
        CStyleElement *pStyle;
        CElement *pHeadElement = pMarkup ? pMarkup->GetHeadElement() : NULL;

        nSSInHead = 0;
     
        if (pHeadElement)
        {
            CChildIterator ci ( pHeadElement );

            while ( (pNode = ci.NextChild()) != NULL )
            {
                if ( pNode->Tag() == ETAG_LINK )
                {
                    pLink = DYNCAST( CLinkElement, pNode->Element() );
                    if ( pLink->_pStyleSheet ) // faster than IsLinkedStyleSheet() and adequate here
                        ++nSSInHead;
                }
                else if ( pNode->Tag() == ETAG_STYLE )
                {
                    pStyle = DYNCAST( CStyleElement, pNode->Element() );
                    if ( pStyle == this )
                        break;
                    if ( pStyle->_pStyleSheet ) // Not all STYLE elements create a SS.
                        ++nSSInHead;
                }
            }
        }
        else
        {
            // Get the next available ID based on the size of the Style Sheet collection
            nSSInHead = pStyleSheets->Size();
        }

        hr = pStyleSheets->CreateNewStyleSheet( this, &_pStyleSheet, nSSInHead );
        if ( hr )
            goto Cleanup;

        _pStyleSheet->AddRef(); // since the style elem is hanging onto the stylesheet ptr
                                // Note this results in a subref on us.
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CStyleElement::Clone
//
//  Synopsis:   Make a new one just like this one
//
//-------------------------------------------------------------------------

HRESULT
CStyleElement::Clone(CElement **ppElementClone, CDoc *pDoc)
{
    HRESULT hr;

    hr = THR(super::Clone(ppElementClone, pDoc));
    if (hr)
        goto Cleanup;

    if (_cstrText)
    {
        hr = THR(DYNCAST(CStyleElement, *ppElementClone)->SetText(_cstrText));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

