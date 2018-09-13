//+---------------------------------------------------------------------
//
//   File:      eanchor.cxx
//
//  Contents:   Anchor element class
//
//  Classes:    CAnchorElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_TXTELEMS_HXX_
#define X_TXTELEMS_HXX_
#include "txtelems.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include <treepos.hxx>
#endif

#ifndef X_HLINK_H_
#define X_HLINK_H_
#include "hlink.h"
#endif

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include "urlmon.h"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif


#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#define _cxx_
#include "anchor.hdl"

MtDefine(CAnchorElement, Elements, "CAnchorElement")

ExternTag(tagMsoCommandTarget);

#ifdef WIN16
#define SetCursorStyle(x) ::SetCursor((HICON)x)
#endif

#ifndef NO_PROPERTY_PAGE
const CLSID * CAnchorElement::s_apclsidPages[] =
{
    // Browse-time pages
    &CLSID_CAnchorBrowsePropertyPage,
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
    &CLSID_CInlineStylePropertyPage,
#endif // DBG==1    
    NULL
};
#endif // NO_PROPERTY_PAGE


const CElement::CLASSDESC CAnchorElement::s_classdesc =
{
    {
        &CLSID_HTMLAnchorElement,            // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                      // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                              // _pcpi
        0,                                   // _dwFlags
        &IID_IHTMLAnchorElement,             // _piidDispinterface
        &s_apHdlDescs,                       // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLAnchorElement,      // _pfnTearOff
    NULL,                                    // _pAccelsDesign
    NULL                                     // _pAccelsRun
};


HRESULT
CAnchorElement::CreateElement(CHtmTag *pht, CDoc *pDoc, CElement **ppElement)
{
    HRESULT   hr = S_OK;

    Assert(pht->Is(ETAG_A));
    Assert(ppElement);
    *ppElement = new CAnchorElement(pDoc);

    if (!*ppElement)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


HRESULT
CAnchorElement::EnterTree()
{
    HRESULT hr = S_OK;
    TCHAR   cBuf[pdlUrlLen];
    TCHAR * pchNewUrl = cBuf;
    CDoc *  pDoc = Doc();

    if (pDoc->_fMarkupServicesParsing && pDoc->_fPasteIE40Absolutify && !pDoc->_fNoFixupURLsOnPaste)
    {
        LPCTSTR szUrl = GetAAhref();

        if (szUrl && *szUrl)
        {
            Assert( !IsInPrimaryMarkup() );

            hr = THR(
                Doc()->ExpandUrl(
                    szUrl, ARRAY_SIZE(cBuf), pchNewUrl, this,
                    URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE,
                    LPTSTR( Doc()->_cstrPasteUrl ) ) );

            if (hr)
                goto Cleanup;

            hr = THR( SetAAhref( pchNewUrl ) );

            if (hr)
                goto Cleanup;
        }
    }

Cleanup:

    RRETURN( hr );

}

void
CAnchorElement::Notify(CNotification *pNF)
{
    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_QUERYFOCUSSABLE:
        if (!IsEditable(TRUE))
        {
            CQueryFocus *   pQueryFocus     = (CQueryFocus *)pNF->DataAsPtr();
            CTreePos    *   ptp, *ptpEnd;

            if (!GetAAhref())
            {
                pQueryFocus->_fRetVal = FALSE;
                break;
            }
            // Check for server-side image map
            // If all that this contains is an IMG with isMap, don't take focus.
            // The image would take focus instead
            GetTreeExtent(&ptp, &ptpEnd);
            ptp = ptp->NextTreePos();
            if (    ptp != ptpEnd
                &&  ptp->IsBeginElementScope()
                &&  ptp->Branch()->Tag() == ETAG_IMG
                &&  DYNCAST(CImgElement, ptp->Branch()->Element())->GetAAisMap()
                &&  ptp->NextTreePos() == ptpEnd->PreviousTreePos())
            {
                pQueryFocus->_fRetVal = FALSE;
                break;
            }
        }
        break;

    case NTYPE_ELEMENT_SETFOCUS:
        if (GetFirstBranch())
        {
            IGNORE_HR(SetStatusText());
            SetActive(TRUE);
            UpdateFormats(GetFirstBranch());
        }
        break;

    case NTYPE_ELEMENT_ENTERTREE:
        EnterTree();
        break;

    case NTYPE_BASE_URL_CHANGE: 
        _fBaseUrlChanged = TRUE;
        OnPropertyChange( DISPID_CAnchorElement_href, 
                            ((PROPERTYDESC *)&s_propdescCAnchorElementhref)->GetdwFlags());
        break;
    }
}


HRESULT
CAnchorElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr = S_OK;

    Assert(Tag() == ETAG_A);

    pCFI->PrepareCharFormat();

    if (GetAAhref())
    {
        switch (Doc()->_pOptionSettings->nAnchorUnderline)
        {
            case ANCHORUNDERLINE_NO:
                pCFI->_cf()._fUnderline = FALSE;
                break;

            case ANCHORUNDERLINE_YES:
                pCFI->_cf()._fUnderline = TRUE;
                break;

            case ANCHORUNDERLINE_HOVER:
                if (_fHovered)
                {
                    pCFI->_cf()._fUnderline = TRUE;
                }
                break;
        }

        pCFI->_cf()._ccvTextColor.SetValue(GetLinkColor(), FALSE);
    }

    // anchors shouldn't inherit the cursor property. they have  'default'
    //   stylesheet.
    pCFI->_cf()._bCursorIdx = styleCursorAuto;

    pCFI->UnprepareForDebug();

    hr = THR(super::ApplyDefaultFormat(pCFI));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

COLORREF CAnchorElement::GetLinkColor()
{
    CDoc *pDoc = Doc();

    if (!_fVisitedValid)
    {
        // BUGBUG (dinartem): Need to figure out when to invalidate
        // the _fVisitedValid flag.

        _fVisited = pDoc->IsVisitedHyperlink(GetAAhref(), this);
        _fVisitedValid = TRUE;
    }

    CColorValue  color;

    if (!pDoc->_pOptionSettings->fAlwaysUseMyColors)
    {
        CBodyElement * pBody = NULL;

        if (GetFirstBranch())
        {
            CTreeNode * pNode;

            pNode = GetFirstBranch()->SearchBranchToRootForTag( ETAG_BODY );

            if (pNode)
                pBody = DYNCAST( CBodyElement, pNode->Element() );
        }

        if (pBody)
        {
            if (_fHovered && pDoc->_pOptionSettings->fUseHoverColor)
                ;//pBody->GetAAhLink();// When ScottI agrees with HLINK attribute on BODY ! :-)
            else if (_fActive)
                color = pBody->GetAAaLink();
            else if (_fVisited)
                color = pBody->GetAAvLink();
            else
                color = pBody->GetAAlink();
        }
    }

    if (_fHovered && pDoc->_pOptionSettings->fUseHoverColor)
    {
        return (color.IsDefined())
                ? color.GetColorRef()
                : pDoc->_pOptionSettings->crAnchorHovered();
    }
    else if (_fVisited)
    {
        return (color.IsDefined())
                ? color.GetColorRef()
                : pDoc->_pOptionSettings->crAnchorVisited();
    }
    else
    {
        return (color.IsDefined())
                ? color.GetColorRef()
                : pDoc->_pOptionSettings->crAnchor();
    }
}


HRESULT
CAnchorElement::UpdateAnchorFromHref()
{
    IHTMLEditingServices * pEd = NULL;

    IHTMLEditor *   phtmed;
    BOOL            fUpdateText;
    BSTR            bstrAnchorText = NULL;
    CMarkup *       pMarkup;
    OLECHAR *       pstrHref;
    HRESULT         hr = S_OK;
    MARKUP_CONTEXT_TYPE context;
    CDoc *          pDoc = Doc();    
    
    Assert( pDoc );

    CMarkupPointer  mpStart( pDoc );
    CMarkupPointer  mpEnd( pDoc );

    //
    // Get the text and href values
    //
    hr = THR( get_innerText( &bstrAnchorText ) );
    if (hr)
        goto Cleanup;

    pstrHref = (OLECHAR *)GetAAhref();
    if (! pstrHref)
        goto Cleanup;

    //
    // If anchor has text, and it's equal to the href already, we can bail
    //
    if ( bstrAnchorText && StrCmpIC( pstrHref, bstrAnchorText ) == 0 )
        goto Cleanup;

    pMarkup = GetMarkup();        
    
    if (! pMarkup )
        goto Cleanup;

    //
    // See what's inside the anchor
    //

    hr = THR( mpStart.MoveAdjacentToElement( this, ELEM_ADJ_AfterBegin ) );
    if (hr)
        goto Cleanup;

    hr = THR( mpEnd.MoveAdjacentToElement( this, ELEM_ADJ_BeforeEnd ) );
    if (hr)
        goto Cleanup;

    //
    // If there anything other than text in the anchor, it is best
    // not to update the anchor's text
    //
    fUpdateText = TRUE;
    while(! mpStart.IsEqualTo( &mpEnd ) )
    {           
        mpStart.Right( TRUE, &context, NULL, NULL, NULL, NULL );
        if ( context != CONTEXT_TYPE_Text )
        {
            fUpdateText = FALSE;
            break;
        }
    }

    if (! fUpdateText)
        goto Cleanup;

    //
    // Get a hold of yourself, now we have to call the autodetector to see
    // if the text and href are autodetectable and match the same pattern
    //
    hr = THR( mpStart.MoveAdjacentToElement( this, ELEM_ADJ_AfterBegin ) );
    if (hr)
        goto Cleanup;

    phtmed = pDoc->GetHTMLEditor();

    if (!phtmed)
        goto Cleanup;

    hr = THR(
        phtmed->QueryInterface(
            IID_IHTMLEditingServices, (void **) & pEd ) );

    if (hr)
        goto Cleanup;

    hr = THR( pEd->ShouldUpdateAnchorText( pstrHref, bstrAnchorText, &fUpdateText ) );
    if (hr)
        goto Cleanup;

    if ( fUpdateText )
    {
        hr = THR( pDoc->Remove( &mpStart, &mpEnd ) );
        if (hr)
            goto Cleanup;

        hr = THR( pDoc->InsertText( & mpStart, pstrHref, -1 ) );
        if (hr)
            goto Cleanup;
    }

Cleanup:
    if (bstrAnchorText)
        SysFreeString(bstrAnchorText);
    ReleaseInterface( pEd );
    return hr;
}


HRESULT
CAnchorElement::OnPropertyChange ( DISPID dispid, DWORD dwFlags )
{
    HRESULT hr = S_OK;
    //
    // If we are changing the HREF for the anchor, we may want to update the
    // text the anchor influences.
    //

    // Undoing of the modification of the tree will be handled by other objects in
    // the undo stack.  Mucking with the tree here will royally whack stuff.
    if (dispid == DISPID_CAnchorElement_href && TLS(nUndoState) == UNDO_BASESTATE)
    {
        // we must set this flag for NS compatibility (Carled)
        // if the change occured because of a base tag change, we don't treat
        // it as a direct OM modification
        _fOMSetHasOccurred = !_fBaseUrlChanged;
        _fBaseUrlChanged = FALSE;

        IGNORE_HR( UpdateAnchorFromHref() );
    }

    hr = THR( super::OnPropertyChange( dispid, dwFlags ) );

    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  member OnCaptureMessage
//
// Synopsis : This fn will be called whenever the Anchor has captured the mouse
//      it is also resposible for intiating the dragdrop behavior of anchors,
//      including the firing of ondragstart
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CAnchorElement::OnCaptureMessage(CMessage *pMessage)
{
    HRESULT                     hr = S_FALSE;
    CLayout                  *  pLayout = GetFirstBranch()->GetUpdatedNearestLayout();
    TCHAR   cBuf[pdlUrlLen];
    TCHAR *                     pchExpandedUrl = cBuf;
    CStr                        strText;
    IUniformResourceLocator *   pURLToDrag = NULL;

    switch (pMessage->message)
    {
    case WM_LBUTTONUP:
        Assert(GetFirstBranch());
        pMessage->SetNodeClk(GetFirstBranch());
        // fall-through

    case WM_MBUTTONUP:
        hr = S_OK;
        // fall-through

    case WM_RBUTTONUP:
        // Release mouse capture
        Doc()->SetMouseCapture(NULL, NULL);
        break;

    case WM_MOUSEMOVE:
    {
        // If the user moves the mouse outside the wobble zone,
        // show the no-entry , plus disallow a subsequent OnClick
        POINT ptCursor = { LOWORD(pMessage->lParam), HIWORD(pMessage->lParam) };
        CDoc *  pDoc = Doc();

        if ( _fCanClick && !PtInRect(&_rcWobbleZone, ptCursor))
        {
            _fCanClick = FALSE;
        }

        // initiate drag-drop
        if (!_fCanClick && !pDoc->_fIsDragDropSrc && !pDoc->_pElementOMCapture)
        {
            // fully resolve URL
            if (S_OK == THR(pDoc->ExpandUrl(GetAAhref(), ARRAY_SIZE(cBuf), pchExpandedUrl, this)))
            {
                if (pMessage->pNodeHit &&
                        pMessage->pNodeHit->TagType() == ETAG_IMG &&
                        S_OK == THR(strText.Set(
                            DYNCAST(CImgElement,pMessage->pNodeHit->Element())
                                ->GetAAalt())) ||
                    S_OK == THR(GetPlainTextInScope(&strText)))
                {
                    if (S_OK == THR(CreateLinkDataObject(pchExpandedUrl,
                                            strText,
                                            &pURLToDrag)))
                    {
                        if (!DragElement(pLayout, pMessage->dwKeyState, pURLToDrag, -1))
                        {
                            // release the capture and let someone else handle the
                            // WM_MOUSEMOVE by leaving hr=S_FALSE
                            pDoc->SetMouseCapture(NULL,NULL);
                            break;
                        }
                    }
                }
            }
        }
        // Intentional drop through to WM_SETCURSOR - WM_SETCURSOR is NOT sent
        // while the Capture is set
    }

    case WM_SETCURSOR:
    {
        LPCTSTR idc;
        if(_fCanClick)
        {
            Assert(pLayout);
            idc = GetHyperlinkCursor();
        }
        else
        {
            idc = IDC_NO;
        }

        SetCursorStyle(idc);
        hr = S_OK;
    }
    break;
    }

    ReleaseInterface(pURLToDrag);

    RRETURN1 ( hr, S_FALSE );
}

//+------------------------------------------------------------------------
//
//  Member:     CAnchorElement::HandleMessage
//
//  Synopsis:   Perform any element specific mesage handling
//
//  Arguments:  pmsg    Ptr to incoming message
//
//-------------------------------------------------------------------------

HRESULT
CAnchorElement::HandleMessage(CMessage *pMessage)
{
    // Only the marquee is allowed to cheat and pass the wrong
    // context in.  This is only done so that message bubbling will
    // skip above the marquee.  However, for operations on the anchor itself,
    // we need to use a correct context and therefore use GetFirstBranch()

    CDoc *      pDoc = Doc();
    HRESULT     hr = S_FALSE;
    CLock       Lock(this);
    BOOL        fInBrowseMode = !IsEditable();
    CTreeNode * pNodeContextReal = GetFirstBranch();

    Assert(pNodeContextReal);

    CTreeNode::CLock NodeLock(pNodeContextReal);

    if (    !fInBrowseMode

            // If htc is set to something other than HTC_NO or HTC_YES,
            // it tells me that the message is a mouse messaage and that
            // the mouse is over a scrollbar or some such uninteresting
            // region. I will let the base class take care of such a
            // message. Note that htc is left as HTC_NO for non-mouse
            // messages.
        ||  (pMessage->htc != HTC_YES && pMessage->htc != HTC_NO)

        || !GetAAhref())
    {
        goto Ignored;
    }

    // now deal with the rest of the messages
    switch ( pMessage -> message )
    {
    case WM_SETCURSOR:
        {
            TCHAR * pchUrl;

            hr = GetUrlComponent(NULL, URLCOMP_WHOLE, &pchUrl);
            if (hr == S_OK && pchUrl)
            {
                SetCursorStyle(GetHyperlinkCursor());
                MemFreeString(pchUrl);
                goto Cleanup;
            }
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        // Capture the mouse to CAnchorElement::::OnCaptureMessage
        pDoc->SetMouseCapture(
                   MOUSECAPTURE_METHOD(CAnchorElement, OnCaptureMessage, oncapturemessage),
                   (void *) this,
                   TRUE);

        // Set the limits for a mouse move before showing
        // the no entry cursor
        _rcWobbleZone.left   = LOWORD(pMessage->lParam) - g_sizeDragMin.cx;
        _rcWobbleZone.right  = LOWORD(pMessage->lParam) + g_sizeDragMin.cx + 1;
        _rcWobbleZone.top    = HIWORD(pMessage->lParam) - g_sizeDragMin.cy;
        _rcWobbleZone.bottom = HIWORD(pMessage->lParam) + g_sizeDragMin.cy + 1;
        // Can click while mouse is inside wobble zone
        _fCanClick = GetAAhref() ? TRUE : FALSE;

        // Give immediate feedback
        if(_fCanClick)
        {
            SetCursorStyle(GetHyperlinkCursor());
        }

        // The message has been handled. Kill it.
        hr = S_OK;
        break;

    case WM_CONTEXTMENU:

        hr = THR(OnContextMenu(
                    (short) LOWORD(pMessage->lParam),
                    (short) HIWORD(pMessage->lParam),
                    (IsEditable(TRUE)) ? (CONTEXT_MENU_DEFAULT)
                                   : (CONTEXT_MENU_ANCHOR)));
        break;

    case WM_CHAR:
    case WM_SYSCHAR:
        switch (pMessage->wParam)
        {
        case VK_RETURN:

            // This is being called only if anchor element has a HREF.
            // i.e. because we donot support tabbing to (and hence the RETURN on)
            // anchor elements which do not have HREFs.
            Assert (GetAAhref() != NULL) ;

            pMessage->SetNodeClk(pNodeContextReal);
            hr = S_OK;
            break ;
        }
        break ;

    case WM_MOUSEWHEEL:
        if ((pMessage->dwKeyState & FSHIFT) && (((short) HIWORD(pMessage->wParam)) > 0))
        {
            // This is being called only if anchor element has a HREF, because
            // we donot navigate to anchor elements which do not have HREFs.
            //
            Assert(GetAAhref() != NULL);
            pMessage->SetNodeClk(pNodeContextReal);
            hr = S_OK;
        }
        break;

    case WM_MOUSEOVER:
        if (!_fHasMouseOverCancelled)
        {
            SetStatusText();
        }
        // fall through
    case WM_MOUSELEAVE:
        _fHovered = pMessage->message == WM_MOUSEOVER ? TRUE : FALSE;

        UpdateFormats(pNodeContextReal);

        break;
    }

Ignored:
    if (S_FALSE == hr)
    {
        hr = THR(super::HandleMessage(pMessage));
    }
Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     CAnchorElement::UpdateFormats
//
//  Synopsis:   Clear format caches, recompute them and then do any
//              necessary invalidate or recalc.
//
//-------------------------------------------------------------------------

HRESULT
CAnchorElement::UpdateFormats(CTreeNode * pNodeContext)
{
    HRESULT hr = S_OK;
    LONG iCF = pNodeContext->_iCF;
    LONG iPF = pNodeContext->_iPF;
    LONG iFF = pNodeContext->_iFF;
    THREADSTATE * pts = GetThreadState();

    // Ensure that current formats are not going anywhere
    if (iCF >= 0) pts->_pCharFormatCache->AddRefData(iCF);
    if (iPF >= 0) pts->_pParaFormatCache->AddRefData(iPF);
    if (iFF >= 0) pts->_pFancyFormatCache->AddRefData(iFF);

    // Clear the format caches
    hr = THR(EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES));
    if (hr)
        goto Cleanup;

    // Cause the formats to get recomputed

    pNodeContext->GetFancyFormat();

    // if format have not changed, no invalidation necessary
    if (    pNodeContext->_iCF >= 0
        &&  pNodeContext->_iCF == iCF
        &&  pNodeContext->_iPF == iPF
        &&  pNodeContext->_iFF == iFF)
        goto Cleanup;

    // And invalidate or recalc based on differences between the old and new formats
    if (    pNodeContext->_iCF == -1
        ||  iCF == -1
        ||  iPF != pNodeContext->_iPF
        ||  iFF != pNodeContext->_iFF
        ||  !GetCharFormatEx(pNodeContext->_iCF)->CompareForLayout(GetCharFormatEx(iCF)))
    {
        ResizeElement();
    }
    else
    {
        Invalidate();
    }

Cleanup:
    // Release the old format caches
    if (iCF >= 0) pts->_pCharFormatCache->ReleaseData(iCF);
    if (iPF >= 0) pts->_pParaFormatCache->ReleaseData(iPF);
    if (iFF >= 0) pts->_pFancyFormatCache->ReleaseData(iFF);
    RRETURN(hr);
}

HRESULT
CAnchorElement::DoClick(CMessage * pMessage, CTreeNode *pNodeContext,
                        BOOL fFromLabel)
{
    HRESULT hr = S_OK;

    if(!pNodeContext)
        pNodeContext = GetFirstBranch();

    if(!pNodeContext)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    Assert(pNodeContext && pNodeContext->Element() == this);

    if (!pNodeContext->GetFlowLayout()->IsEditable())
    {
        hr = super::DoClick(pMessage, pNodeContext, fFromLabel);
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

HRESULT
CAnchorElement::ClickAction (CMessage *pmsg)
{
    HRESULT hr = super::ClickAction(pmsg);

    if (hr == S_OK && !_fVisited)
    {
        hr = THR(SetVisited());
        if (hr)
            goto Cleanup;
    }

Cleanup:
    // Do not want this to bubble, so..
    if (S_FALSE == hr)
        hr = S_OK;

    RRETURN(hr);
}

HRESULT
CAnchorElement::SetActive( BOOL fActive )
{
    HRESULT hr = S_OK;

    if ( fActive != _fActive)
    {
        hr = ExecPseudoClassEffect(_fVisited, fActive, _fVisited, _fActive);

        // BUGBUG (MohanB) Should force a cursor update here. See bug#8212
    }
    _fActive = fActive;
    return hr;
}

HRESULT
CAnchorElement::ExecPseudoClassEffect(BOOL fVisited, BOOL fActive,
                                      BOOL fOldVisited, BOOL fOldActive)
{
    HRESULT hr;
    CStyleSheetArray * pSS = GetMarkup()->GetStyleSheetArray();

    // color has changed
    hr = THR(EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES));
    if (hr)
        goto Cleanup;

    // First check the anchor itself for any styles that effect
    if(pSS)
    {
       if(pSS->TestForPseudoclassEffect(this,
                                        fVisited, fActive,
                                        fOldVisited, fOldActive))
        {
            ResizeElement();
        }
        else
        {
            CChildIterator  ci( this, NULL, CHILDITERATOR_DEEP );
            CTreeNode *     pNodeCurr;

            for( pNodeCurr = ci.NextChild();
                 pNodeCurr;
                 pNodeCurr = ci.NextChild() )
            {
                if( pSS->TestForPseudoclassEffect(pNodeCurr->Element(),
                                        fVisited, fActive,
                                        fOldVisited, fOldActive))
                {
                    // make sure we skip over the curr node's
                    // subtree
                    ci.ClearDeep();

                    // Resize the current element
                    ResizeElement();
                }
                else
                {
                    ci.SetDeep();
                }
            }

            Invalidate();
        }

    }
    else
    {
        Invalidate();
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CAnchorElement::SetVisited()
{
    HRESULT hr = S_OK;

    // BUGBUG: when a hyperlink is followed we just cause the clicked anchor to change color.
    // Netscape recolors all hyperlinks to the same URL, even in other frames. (dbau)

    if (!Doc()->IsOffline())
    {
        _fVisited = TRUE;
        _fVisitedValid = TRUE;

        // if we are not in the tree just leave.
        if (!GetFirstBranch())
            goto Cleanup;

        hr = ExecPseudoClassEffect(_fVisited, _fActive, FALSE, _fActive);
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// Member: get_mimeType
//
//-----------------------------------------------------------------------------
extern TCHAR * GetFileTypeInfo(TCHAR * pszFileName);

STDMETHODIMP
CAnchorElement::get_mimeType(BSTR * pMimeType)
{
    HRESULT   hr   = S_OK;
    TCHAR   * pUrl = NULL;

    * pMimeType = NULL;

    hr = get_href(&pUrl);
    if (hr)
        goto Cleanup;

    if (pUrl && !UrlIsOpaque(pUrl))
    {
        TCHAR * pCh = _tcschr(pUrl, _T('?'));
        if (pCh)
            pCh = _T('\0');

        * pMimeType = GetFileTypeInfo(pUrl);
        SysFreeString(pUrl);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
// Member: get_protocolLong
//
//-----------------------------------------------------------------------------
extern TCHAR * ProtocolFriendlyName(TCHAR * szUrl);

STDMETHODIMP
CAnchorElement::get_protocolLong(BSTR * pProtocol)
{
    HRESULT   hr      = S_OK;
    TCHAR   * pUrl    = NULL;
    TCHAR   * pResult = NULL;

    * pProtocol = NULL;

    hr = get_href(&pUrl);
    if (hr)
        goto Cleanup;

    if (pUrl)
    {
        pResult = ProtocolFriendlyName(pUrl);
        if (pResult)
        {
            int z = (_tcsncmp(pResult, 4, _T("URL:"), -1) == 0) ? (4) : (0);
            * pProtocol = SysAllocString(pResult + z);
            SysFreeString(pResult);
        }
        SysFreeString(pUrl);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
// Member: get_nameProp
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CAnchorElement::get_nameProp(BSTR * pName)
{
    *pName = NULL;

    TCHAR   * pUrl  = NULL;
    TCHAR   * pszName = NULL;
    HRESULT hr;

    hr = get_href(&pUrl);
    if (hr)
        goto Cleanup;

    if (pUrl)
    {
        pszName = _tcsrchr(pUrl, _T('/'));
        if (!pszName)
            pszName = pUrl;
        else if (*(pszName + 1) == _T('\0'))
        {
            *(pszName) = _T('\0');
            pszName = _tcsrchr(pUrl, _T('/'));
            if (!pszName)
                pszName = pUrl;
            else
                pszName ++;
        }
        else
            pszName ++;

        * pName = SysAllocString(pszName);
        SysFreeString(pUrl);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------
//
//  Member:     CDoc::EnumContainedURLs
//
//  Synopsis:   returns 2 arrays (containing the URLs and the associated anchor
//              strings) for a given document
//
//  Arguments:  paryURLs     - array containing URL strings
//              paryStrings  - array containing anchor strings
//
//-----------------------------------------------------------------------------

HRESULT
CDoc::EnumContainedURLs(CURLAry * paryURLs, CURLAry * paryStrings)
{
    HRESULT         hr = S_OK;
    CElement        *pAnchorOrAreaElement;
    CAnchorElement  *pAnchorElem=NULL;
    CAreaElement    *pAreaElem=NULL;
    LPCTSTR         lpctstrAnchorOrAreaAAhref;
    BOOL            fAnchorElem;
    CStr            *pStrURL;
    CStr            *pStrText;
    LONG            i, c;
    CCollectionCache *pCollectionCache;

    if (!paryURLs || !paryStrings)
    {
       hr = E_POINTER;
       goto Cleanup;
    }

    hr = THR (PrimaryMarkup()->EnsureCollectionCache(CMarkup::LINKS_COLLECTION));
    if (hr)
    {
        goto Cleanup;
    }

    pCollectionCache = PrimaryMarkup()->CollectionCache();

    c = pCollectionCache->SizeAry(CMarkup::LINKS_COLLECTION);
    for (i = 0; i < c; i++)
    {
        hr = THR (pCollectionCache->GetIntoAry(CMarkup::LINKS_COLLECTION,
                        i,
                        &pAnchorOrAreaElement));
        if (hr != S_OK)
        {
            goto Cleanup;
        }

        fAnchorElem = pAnchorOrAreaElement->_etag == ETAG_A;
        Assert(fAnchorElem || pAnchorOrAreaElement->_etag == ETAG_AREA);

        pAnchorElem = fAnchorElem ? DYNCAST (CAnchorElement, pAnchorOrAreaElement) : NULL;
        pAreaElem  = !fAnchorElem ? DYNCAST (CAreaElement, pAnchorOrAreaElement) : NULL;

        lpctstrAnchorOrAreaAAhref = fAnchorElem ? pAnchorElem->GetAAhref() : pAreaElem->GetAAhref();
        if (lpctstrAnchorOrAreaAAhref)
        {
            CStr strTmp;
            TCHAR   cBuf[pdlUrlLen];
            TCHAR *pachAbsoluteUrl = cBuf;

            hr = paryURLs->AppendIndirect(NULL, &pStrURL);
            if (hr)
                goto Cleanup;

            hr = paryStrings->AppendIndirect(NULL, &pStrText);
            if (hr)
                goto Cleanup;

            hr = THR(ExpandUrl(lpctstrAnchorOrAreaAAhref, ARRAY_SIZE(cBuf), pachAbsoluteUrl, pAnchorOrAreaElement));
            if (hr != S_OK || !pachAbsoluteUrl)
            {
                goto Cleanup;
            }

            hr = THR(pStrURL->Set(pachAbsoluteUrl));

            if (hr != S_OK)
            {
                goto Cleanup;
            }

            hr = pAnchorOrAreaElement->GetPlainTextInScope(&strTmp);
            if (hr != S_OK)
            {
                goto Cleanup;
            }

            hr = THR (pStrText->Set(strTmp));
            if (hr != S_OK)
            {
                goto Cleanup;
            }
        }
    }

Cleanup:
    RRETURN1 (hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CAnchorElement::ShowTooltip
//
//  Synopsis:   Displays the tooltip for the site.
//
//  Arguments:  [pt]    Mouse position in container window coordinates
//              msg     Message passed to tooltip for Processing
//
//--------------------------------------------------------------------------

ExternTag(tagFormatTooltips);

HRESULT
CAnchorElement::ShowTooltip(CMessage *pmsg, POINT pt)
{
    HRESULT     hr = S_FALSE;
    CDoc *      pDoc = Doc();
    TCHAR *     pchString;
    BOOL        fRTL = FALSE;

#if DBG == 1
    if (IsTagEnabled(tagFormatTooltips))
    {
        return super::ShowTooltip(pmsg, pt);
    }
#endif

    if (pDoc->State() < OS_INPLACE)
        goto Cleanup;

    pchString = (LPTSTR) GetAAtitle();

    if ( pchString)
    {
        RECT    rc;

        // Ignore spurious WM_ERASEBACKGROUNDs generated by tooltips
        CServer::CLock Lock(pDoc, SERVERLOCK_IGNOREERASEBKGND);

        hr = THR(GetElementRc(&rc,
                              GERC_ONALINE | GERC_CLIPPED,
                              &pt));
        if (hr)
            goto Cleanup;

        // COMPLEXSCRIPT - determine if element is right to left for tooltip style setting
        if(GetFirstBranch())
        {
            fRTL = GetFirstBranch()->GetCharFormat()->_fRTL;
        }

        FormsShowTooltip(
                pchString,
                pDoc->_pInPlace->_hwnd,
                *pmsg,
                &rc,
                (DWORD_PTR) this,
                fRTL);
        hr = S_OK;
    }

Cleanup:
    return hr;
}

#ifndef NO_DATABINDING
#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

class CDBindMethodsAnchor : public CDBindMethodsSimple
{
    typedef CDBindMethodsSimple super;

public:
    CDBindMethodsAnchor() : super(VT_BSTR, DBIND_ONEWAY) {}
    ~CDBindMethodsAnchor()  {}

    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const;

};

static const CDBindMethodsAnchor DBindMethodsAnchor;

const CDBindMethods *
CAnchorElement::GetDBindMethods()
{

    return &DBindMethodsAnchor;
}

//+----------------------------------------------------------------------------
//
//  Function: BoundValueToElement, CDBindMethods
//
//  Synopsis: Transfer data into bound image.  Only called if DBindKind
//            allows databinding.
//
//  Arguments:
//            [id]      - ID of binding point.  For anchor, is always
//                        DISPID_CAnchorElement_href.
//            [pvData]  - pointer to data to transfer, in this case a bstr.
//
//-----------------------------------------------------------------------------
HRESULT
CDBindMethodsAnchor::BoundValueToElement(CElement *pElem,
                                         LONG,
                                         BOOL,
                                         LPVOID pvData) const
{
    RRETURN1(DYNCAST(CAnchorElement, pElem)->put_href(*(BSTR *)pvData), S_FALSE);
}
#endif // ndef NO_DATABINDING


// URL accessors - CHyperlink overrides

HRESULT
CAnchorElement::SetUrl(BSTR bstrUrl)
{
    return (s_propdescCAnchorElementhref.b.SetUrlProperty(bstrUrl,
                                this,
                                (CVoid *)(void *)GetAttrArray()));
}


LPCTSTR
CAnchorElement::GetUrl() const
{
    return GetAAhref();
}


LPCTSTR
CAnchorElement::GetTarget() const
{
    return GetAAtarget();
}


HRESULT
CAnchorElement::GetUrlTitle(CStr * pstr)
{
    HRESULT hr;
    
    if (GetAAtitle())
    {
        pstr->Set(GetAAtitle());
        if (pstr->Length() > 0)
            return S_OK;
    }

    hr = THR(GetPlainTextInScope(pstr));

    if (OK(hr) && Doc()->_fVisualOrder)
    {
        LPTSTR pStart = *pstr;
        LPTSTR pEnd = pStart + pstr->Length() - 1;

        while (pStart < pEnd)
        {
            const TCHAR ch = *pStart;
            *pStart++ = *pEnd;
            *pEnd-- = ch;
        }
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CAnchorElement::QueryStatus, public
//
//  Synopsis:   Implements QueryStatus for CAnchorElement
//
//  Notes:      Delegates to base class
//
//----------------------------------------------------------------------------

HRESULT
CAnchorElement::QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    TraceTag((tagMsoCommandTarget, "CAnchorElement::QueryStatus"));

    Assert(cCmds == 1);
    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));

    HRESULT  hr   = S_OK;
    MSOCMD * pCmd = &rgCmds[0];

    Assert(!pCmd->cmdf);

    if (!hr && !pCmd->cmdf)
    {
        hr = THR_NOTRACE(super::QueryStatusHelper(pguidCmdGroup, cCmds, rgCmds, pcmdtext));
    }

    if (!hr && !pCmd->cmdf)
    {
        hr = THR_NOTRACE(CElement::QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext));
    }

    RRETURN_NOTRACE(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAnchorElement::Exec, public
//
//  Synopsis:   Executes a command on the CAnchorElement
//
//  Notes:      Delegates to base class
//
//----------------------------------------------------------------------------

HRESULT
CAnchorElement::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    TraceTag((tagMsoCommandTarget, "CAnchorElement::Exec"));

    HRESULT hr = MSOCMDERR_E_NOTSUPPORTED;

    if (hr == MSOCMDERR_E_NOTSUPPORTED)
    {
        hr = THR_NOTRACE(super::ExecHelper(
                    pguidCmdGroup,
                    nCmdID,
                    nCmdexecopt,
                    pvarargIn,
                    pvarargOut));
    }

    if (hr == MSOCMDERR_E_NOTSUPPORTED)
    {
        hr = THR_NOTRACE(CElement::Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut));
    }
    RRETURN_NOTRACE(hr);
}

//BUGBUG (MohanB) pdl parser should be fixed so to avoid doing this
HRESULT CAnchorElement::focus() { return super::focus(); };
HRESULT CAnchorElement::blur() { return super::blur(); };
