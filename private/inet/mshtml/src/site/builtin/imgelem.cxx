//+---------------------------------------------------------------------
//
//   File:      image.cxx
//
//  Contents:   Img element class, etc..
//
//  Classes:    CImgElement, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_POSTDATA_HXX_
#define X_POSTDATA_HXX_
#include "postdata.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_TYPES_H_
#define X_TYPES_H_
#include "types.h" // for s_enumdeschtmlReadyState
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_MSRATING_HXX_
#define X_MSRATING_HXX_
#include "msrating.hxx" // AreRatingsEnabled()
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_IMGLYT_HXX_
#define X_IMGLYT_HXX_
#include "imglyt.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
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
#include "img.hdl"

ExternTag(tagMsoCommandTarget);
MtDefine(CImgElement, Elements, "CImgElement")
MtDefine(CImageElementFactory, Elements, "CImageElementFactory")

#ifndef WIN16
extern NEWIMGTASKFN NewImgTaskArt;
#endif


#ifndef NO_PROPERTY_PAGE
const CLSID * CImgElement::s_apclsidPages[] =
{
    // Browse-time pages
    &CLSID_CImageBrowsePropertyPage,
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
    &CLSID_CInlineStylePropertyPage,
#endif // DBG==1       
    NULL
};
#endif // NO_PROPERTY_PAGE



const CElement::CLASSDESC CImgElement::s_classdesc =
{
    {
        &CLSID_HTMLImg,                 // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_CARETINS_SL |
        ELEMENTDESC_NEVERSCROLL |
        ELEMENTDESC_EXBORDRINMOV,       // _dwFlags
        &IID_IHTMLImgElement,           // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLImgElement,    // _pfnTearOff
    NULL,                               // _pAccelsDesign
    NULL                                // _pAccelsRun
};

CImgElement::CImgElement (ELEMENT_TAG eTag, CDoc *pDoc)
      : super(eTag, pDoc)
{
#ifdef WIN16
    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
    m_ElementOffset = ((BYTE *) (void *) (CElement *)this) - ((BYTE *) this);
#endif
    _pMap = NULL;
    _fCanClickImage = FALSE;
}


//+------------------------------------------------------------------------
//
//  Member:     Init
//
//  Synopsis:   
//
//-------------------------------------------------------------------------

HRESULT
CImgElement::Init()
{
    HRESULT hr = S_OK;

    _pImage = new CImgHelper(Doc(), this, FALSE);

    if (!_pImage)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    hr = super::Init();

Cleanup:
    RRETURN(hr);
}

HRESULT
CImgElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert (ppElement);

    *ppElement = new CImgElement(pht->GetTag(), pDoc);

    RRETURN ((*ppElement) ? S_OK : E_OUTOFMEMORY);
}

//+------------------------------------------------------------------------
//
//  Member:     CImgElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CImgElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_TEAROFF(this, IDispatchEx, NULL);
        QI_HTML_TEAROFF(this, IHTMLElement2, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *) *ppv)->AddRef();
        RRETURN(S_OK);
    }

    RRETURN(super::PrivateQueryInterface(iid, ppv));
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
CImgElement::ContextThunk_InvokeExReady(DISPID dispid,
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

    hr = THR_NOTRACE(ReadyStateInvoke(dispid, wFlags, _pImage->_readyStateFired, pvarResult));
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


void
CImgElement::EnsureMap()
{
    CDoc * pDoc = Doc();

    _pMap = NULL;
    if (pDoc->_pMapHead == NULL)
        return;

    LPCTSTR pchUSEMAP = GetAAuseMap();

    if (pchUSEMAP == NULL || *pchUSEMAP == 0)
        return;

    pchUSEMAP = _tcschr(pchUSEMAP, _T('#'));

    if (pchUSEMAP == NULL)
        return;

    pchUSEMAP += 1;

    if (*pchUSEMAP == 0)
        return;

    LONG lSourceIndexMin = LONG_MAX;
    LONG lSourceIndex;
    CMapElement * pMap;
    LPCTSTR pchName;
    BOOL fEqual;

    // Find the map which has the same name and smallest source index

    for (pMap = pDoc->_pMapHead; pMap; pMap = pMap->_pMapNext)
    {
        pchName = pMap->GetAAname();
        fEqual = pchName ? !FormsStringICmp(pchUSEMAP, pchName) : FALSE;

        if (!fEqual)
        {
            pchName = pMap->GetAAid();
            fEqual = pchName ? !FormsStringICmp(pchUSEMAP, pchName) : FALSE;
        }

        if (!fEqual)
        {
            pchName = pMap->GetAAuniqueName();
            fEqual = pchName ? !FormsStringICmp(pchUSEMAP, pchName) : FALSE;
        }

        if (fEqual)
        {
            lSourceIndex = pMap->GetSourceIndex();

            if (lSourceIndex < lSourceIndexMin)
            {
                _pMap = pMap;
                lSourceIndexMin = lSourceIndex;
            }
        }
    }
}

 
//+---------------------------------------------------------------------------
//
//  Member:     CImgElement::Notify
//
//  Synopsis:   Receives notifications
//
//+---------------------------------------------------------------------------

void
CImgElement::Notify(CNotification *pNF)
{
    CAreaElement *  pArea;
    HRESULT         hr = S_OK;
    CElement::CLock  Lock(this);

    super::Notify(pNF);
    Assert(_pImage);

    _pImage->Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_QUERYFOCUSSABLE:
        if (!IsEditable(TRUE))
        {
            CQueryFocus *   pQueryFocus = (CQueryFocus *) pNF->DataAsPtr();

            pQueryFocus->_fRetVal = FALSE;

            // uses a client-side image map?
            if (EnsureAndGetMap())
            {
                CAreaElement *  pArea;

                if (S_OK == THR(_pMap->GetAreaContaining(pQueryFocus->_lSubDivision, &pArea)))
                {
                    // BUGBUG (MohanB) what about nohref?
                    pQueryFocus->_fRetVal = pArea && (pArea->GetUrl() || pArea->GetAAtabIndex() != htmlTabIndexNotSet);
                }
            }

            // is a server-side image map?
            else if (GetAAisMap())
            {
                CAnchorElement * pAnchor = GetContainingAnchor();
                pQueryFocus->_fRetVal = pAnchor && pAnchor->GetUrl();
            }
        }
        break;
    case NTYPE_ELEMENT_QUERYTABBABLE:
        if (!IsEditable(TRUE))
        {
            CQueryFocus *   pQueryFocus = (CQueryFocus *) pNF->DataAsPtr();

            // Assume that focussability is already checked for, and only make
            // sure that tabIndex is non-negative for subdivision
            Assert(IsFocussable(pQueryFocus->_lSubDivision));
            pQueryFocus->_fRetVal = TRUE;

            // uses a client-side image map?
            if (EnsureAndGetMap())
            {
                CAreaElement *  pArea;

                if (S_OK == THR(_pMap->GetAreaContaining(pQueryFocus->_lSubDivision, &pArea)))
                {
                    Assert(pArea);
                    if (pArea)
                    {
                        short tabIndex = pArea->GetAAtabIndex();

                        if (tabIndex != htmlTabIndexNotSet && tabIndex < 0)
                        {
                            pQueryFocus->_fRetVal = FALSE;
                        }
                    }
                }
            }
        }
        break;
    case NTYPE_ELEMENT_SETFOCUS:
        {
            CHyperlink *    pHyperlink  = NULL;

            // uses a client-side image map?
            if (EnsureAndGetMap())
            {
                CAreaElement *  pArea;

                if (S_OK == THR(_pMap->GetAreaContaining(Doc()->_lSubCurrent, &pArea)))
                {
                    // BUGBUG (MohanB) What about nohref?
                    pHyperlink = pArea;
                }
            }

            // is a server-side image map?
            else if (GetAAisMap())
            {
                pHyperlink = GetContainingAnchor();
            }

            if (pHyperlink)
            {
                IGNORE_HR(pHyperlink->SetStatusText());
            }
        }
        break;

    case NTYPE_AREA_TABINDEX_CHANGE:
    case NTYPE_AREA_FOCUS:
        EnsureMap();
        if (_pMap)
        {
            long    l;

            pNF->Data((void **)&pArea);

            if (OK(_pMap->SearchArea(pArea, &l)))
            {
                pNF->SetFlag(NFLAGS_SENDENDED);

                if (pNF->Type() == NTYPE_AREA_FOCUS)
                {
                    //
                    // Search for the area in the map.  If found, then make
                    // myself current.
                    //

                    hr = THR(focusHelper(l));
                    if (hr)
                        goto Cleanup;
                }
                else
                {
                    Assert(NTYPE_AREA_TABINDEX_CHANGE == pNF->Type());
                    OnTabIndexChange();
                }
            }
        }
    
    case NTYPE_ELEMENT_ENTERTREE:
        EnterTree();
        break;

    case NTYPE_BASE_URL_CHANGE:
        OnPropertyChange( DISPID_CImgElement_useMap, ((PROPERTYDESC *)&s_propdescCImgElementuseMap)->GetdwFlags() );
        OnPropertyChange( DISPID_CImgElement_src, ((PROPERTYDESC *)&s_propdescCImgElementsrc)->GetdwFlags() );
        OnPropertyChange( DISPID_CImgElement_dynsrc, ((PROPERTYDESC *)&s_propdescCImgElementdynsrc)->GetdwFlags() );
        OnPropertyChange( DISPID_CImgElement_lowsrc, ((PROPERTYDESC *)&s_propdescCImgElementlowsrc)->GetdwFlags() );
        break;
    }

Cleanup:
    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     EnterTree
//
//+---------------------------------------------------------------------------
HRESULT
CImgElement::EnterTree()
{
    EnsureMap();

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     HandleMessage
//
//  Synopsis:   Handle messages bubbling when the passed site is non null
//
//  Arguments:  [pMessage]  -- message
//              [pChild]    -- pointer to child when bubbling allowed
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CImgElement::HandleMessage(CMessage * pMessage)
{
    HRESULT hr              = S_FALSE;
    BOOL    fInBrowseMode   = !IsEditable(TRUE);
    TCHAR * pchUrl;
    TCHAR   cBuf[pdlUrlLen];
    TCHAR * pchExpandedUrl  = cBuf;
    IUniformResourceLocator *   pURLToDrag      = NULL;

    EnsureMap();
    if (_pMap)
    {
        CAreaElement *  pArea = NULL;

        IGNORE_HR(_pMap->GetAreaContaining(pMessage->lSubDivision, &pArea));
        if (fInBrowseMode && pArea)
        {
            switch(pMessage->message)
            {
            case WM_CHAR:
            case WM_SYSCHAR:
                switch (pMessage->wParam)
                {
                case VK_RETURN:

                    // For <AREA>, pressing Enter is same as clicking with mouse
                    if (GetCurrentArea())
                    {
                        pMessage->lSubDivision = Doc()->_lSubCurrent;
                        pMessage->SetNodeClk(GetFirstBranch());
                        hr = S_OK;
                    }
                    break ;
                }
                break ;

            case WM_MOUSEWHEEL:
                if (   (pMessage->dwKeyState & FSHIFT)
                    && (((short) HIWORD(pMessage->wParam)) > 0))
                {
                    pMessage->SetNodeClk(GetFirstBranch());
                    hr = S_OK;
                }
                break;

            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                Doc()->SetMouseCapture(
                    MOUSECAPTURE_METHOD(CImgElement, HandleCaptureMessageForArea, handlecapturemessageforarea),
                    (void *) this,
                    TRUE);

                // Set the limits for a mouse move before showing
                // the no entry cursor
                _rcWobbleZone.left   = pMessage->pt.x - g_sizeDragMin.cx;
                _rcWobbleZone.right  = pMessage->pt.x + g_sizeDragMin.cx + 1;
                _rcWobbleZone.top    = pMessage->pt.y - g_sizeDragMin.cy;
                _rcWobbleZone.bottom = pMessage->pt.y + g_sizeDragMin.cy + 1;

                // Can click while mouse is inside wobble zone
                _fCanClickImage = TRUE;
                hr = S_OK;
                break;

            case WM_MOUSEOVER:
                if (!pArea->_fHasMouseOverCancelled)
                {
                    pArea->SetStatusText();
                }
                break;

            case WM_SETCURSOR:
                {
                    TCHAR * pchUrl;

                    if (S_OK == pArea->GetUrlComponent(NULL, URLCOMP_WHOLE, &pchUrl) && pchUrl)
                    {
                        SetCursorStyle(pArea->GetHyperlinkCursor());
                        MemFreeString(pchUrl);
                        hr = S_OK;
                    }
                }
                break;

            case WM_LBUTTONUP:
                if (_fCanClickImage)    // If click is allowed,
                {
                    Assert(SameScope(pMessage->pNodeHit, this));
                    pMessage->SetNodeClk(GetFirstBranch());
                }
                // Release the mouse capture.
                Doc()->SetMouseCapture(NULL, NULL);
                _fCanClickImage = FALSE;
                hr = S_OK;
                break;

            case WM_RBUTTONUP:
                hr = S_FALSE;

                // Release the mouse capture.
                Doc()->SetMouseCapture(NULL, NULL, TRUE);
                _fCanClickImage = FALSE;
                break;

            case WM_MOUSEMOVE:
                if (!_fCanClickImage)
                    break;

                // If the user moved the mouse outside of the wobble zone,
                if(!PtInRect(&_rcWobbleZone, pMessage->pt))
                {
                    CDoc * pDoc = Doc();

                    _fCanClickImage = FALSE;                     // Disable click
                    SetCursorStyle(IDC_NO);

                    // Release the mouse capture.
                    pDoc->SetMouseCapture(NULL, NULL, TRUE);

                    // initiate drag-drop
                    if (!pDoc->_fIsDragDropSrc)
                    {
                        // fully resolve URL
                        if (    (S_OK == THR(pDoc->ExpandUrl(pArea->GetAAhref(), ARRAY_SIZE(cBuf), pchExpandedUrl, this)))
                            &&  (S_OK == THR(CreateLinkDataObject(pchExpandedUrl, NULL, &pURLToDrag))))
                        {
                            if (!DragElement(GetCurLayout(), pMessage->dwKeyState, pURLToDrag, pMessage->lSubDivision))
                            {
                                // release the capture and let someone else handle the
                                // WM_MOUSEMOVE by leaving hr=S_FALSE
                                pDoc->SetMouseCapture(NULL,NULL);
                                break;
                            }
                        }
                    }
                }
                hr = S_OK;
                break;
            }
        }
    }

    if (!hr)
        goto Cleanup;

    if (fInBrowseMode)
    {
        if (hr == S_FALSE)
        {
            CAnchorElement * pAnchorElement = GetContainingAnchor();

            if (pAnchorElement)
            {
                switch (pMessage->message)
                {

                // Need to handle 'Enter' for server-side image maps (IE5 bug #4091)                        
                case WM_CHAR:
                case WM_SYSCHAR:
                    if (pMessage->wParam == VK_RETURN && GetAAisMap())
                    {
                        // Pressing Enter is same as clicking with mouse
                        pMessage->SetNodeClk(GetFirstBranch());
                        hr = S_OK;
                    }
                    break ;
                case WM_LBUTTONDOWN:
                    // If it is not a image map then this message is really
                    // meant for the anchor element.
                    // BUGBUG (sujalp): Other messages to send to it??
                    if (GetAAisMap())
                    {
                        hr = S_OK;
                         Doc()->SetMouseCapture(MOUSECAPTURE_METHOD(CImgElement, HandleCaptureMessageForImage, handlecapturemessageforimage),
                                                (void *)this, TRUE);
                    }
                    else
                    {
                        // NOTE (sujalp): We have to simulate as if the message
                        // was received by the anchor (pAnchorElement) directly and hence
                        // we pass pAnchorElement and not NULL , though it is coming via
                        // the child.
                        hr = THR (pAnchorElement->HandleMessage (pMessage)) ;
                    }
                    break;

                case WM_RBUTTONDOWN:

                    // See sujal's note, above
                    hr = THR(pAnchorElement->HandleMessage(pMessage));
                    break;

                case WM_SETCURSOR:

                    SetCursorStyle(pAnchorElement->GetHyperlinkCursor());

                    hr = pAnchorElement->GetUrlComponent(NULL, URLCOMP_WHOLE, &pchUrl);
                    if (!hr && pchUrl)
                    {
                        TCHAR *pchFriendlyUrl;
                        BOOL fShowStatusText = TRUE;
                        CDoc * pDoc = Doc();

                        if (GetAAisMap())
                        {
                            Assert(pMessage->IsContentPointValid());
                            pchFriendlyUrl = GetFriendlyUrl(
                                                    pchUrl,
                                                    pDoc->_cstrUrl,
                                                    pDoc->_pOptionSettings->fShowFriendlyUrl,
                                                    TRUE,
                                                    pMessage->ptContent.x,
                                                    pMessage->ptContent.y);
                        }
                        else
                        {
                            pchFriendlyUrl = GetFriendlyUrl(pchUrl, pDoc->_cstrUrl,
                                                            pDoc->_pOptionSettings->fShowFriendlyUrl, TRUE);

                            // if previous element under mouse is not the same as current element under the mouse,
                            // set the status text for the anchor that is under (or IS) the curent element.
                            fShowStatusText = DifferentScope(pDoc->_pNodeLastMouseOver, this);
                        }

                        if (fShowStatusText)
                            pDoc->SetStatusText(pchFriendlyUrl, STL_ROLLSTATUS);

                        MemFreeString(pchFriendlyUrl);
                        MemFreeString(pchUrl);
                    }

                    hr = S_OK;
                    break;

                default:
                     ;
                }
            }

            // Prepare for drag-drop
            switch (pMessage->message)
            {
                case WM_LBUTTONDOWN:
                case WM_RBUTTONDOWN:
                    _rcWobbleZone.left   = pMessage->pt.x - g_sizeDragMin.cx;
                    _rcWobbleZone.right  = pMessage->pt.x + g_sizeDragMin.cx + 1;
                    _rcWobbleZone.top    = pMessage->pt.y - g_sizeDragMin.cy;
                    _rcWobbleZone.bottom = pMessage->pt.y + g_sizeDragMin.cy + 1;

                    // Can click while mouse is inside wobble zone
                    _fCanClickImage = TRUE;
                    break;
            }
        }
    }


    // WM_CONTEXTMENU message should always be handled.
    if (pMessage->message == WM_CONTEXTMENU)
    {
        Assert(_pImage);
        hr = THR(_pImage->ShowImgContextMenu(pMessage));
    }

    // And process the message if it hasn't been already.
    if (hr == S_FALSE)
    {
        switch (pMessage->message)
        {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            if (fInBrowseMode)
            {
                // if it was not NULL it was handled before
                Doc()->SetMouseCapture(
                                       MOUSECAPTURE_METHOD(CImgElement, HandleCaptureMessageForImage, handlecapturemessageforimage),
                                       (void *)this,
                                       TRUE);
                hr = S_OK;
            }
            break;

#ifndef NO_MENU
        case WM_MENUSELECT:
        case WM_INITMENUPOPUP:
            hr = S_FALSE;
            break;
#endif // NO_MENU

        case WM_SETCURSOR:
            if ( ! IsEditable() )
                SetCursorStyle(IDC_ARROW);
            else
                SetCursorStyle(IDC_SIZEALL);
            hr = S_OK;
            break;
        }

        if (hr == S_FALSE)
        {
            hr = THR(super::HandleMessage(pMessage));
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     HandleCaptureMessageForImage
//
//  Synopsis:   Tracks mouse while user is clicking on an IMG in an A
//
//  Arguments:  [pMessage]  -- message
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CImgElement::HandleCaptureMessageForImage (CMessage * pMessage)
{
    HRESULT     hr = S_OK;

    switch (pMessage->message)
    {
    case WM_LBUTTONUP:
        if (_fCanClickImage)
        {
            pMessage->SetNodeClk(GetFirstBranch());
        }
        // Fall thru

    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        Doc()->SetMouseCapture(NULL, NULL);
        if (pMessage->message == WM_RBUTTONUP)
            hr = S_FALSE;
        break;

    case WM_MOUSEMOVE:
    {
        // If the user moves the mouse outside the wobble zone,
        // show the no-entry , plus disallow a subsequent OnClick
        POINT ptCursor = { LOWORD(pMessage->lParam), HIWORD(pMessage->lParam) };
        CDoc *pDoc = Doc();

        if ( _fCanClickImage && !PtInRect(&_rcWobbleZone, ptCursor))
        {
            _fCanClickImage = FALSE;
        }

        // initiate drag-drop
        if (!_fCanClickImage && !pDoc->_fIsDragDropSrc)
        {
            Assert(!pDoc->_pDragDropSrcInfo);
            if (!pDoc->_pElementOMCapture)
            {
                DragElement(GetCurLayout(), pMessage->dwKeyState, NULL, -1);
            }
        }
        // Intentional drop through to WM_SETCURSOR - WM_SETCURSOR is NOT sent
        // while the Capture is set
    }

    case WM_SETCURSOR:
        {
            LPCTSTR idc;
            CAnchorElement * pAnchorElement = GetContainingAnchor();
            CRect   rc;


            GetCurLayout()->GetClientRect(&rc);

            if (pAnchorElement && PtInRect(&rc, pMessage->ptContent))
                idc = pAnchorElement->GetHyperlinkCursor();
            else
                idc = IDC_ARROW;

            SetCursorStyle(idc);
            hr = S_OK;
        }
        break;
    }

    if (hr == S_FALSE)
        hr = THR(super::HandleMessage(pMessage));

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     HandleCaptureMessageForArea
//
//  Synopsis:   Tracks mouse while user is clicking on an IMG with an AREA
//
//  Arguments:  [pMessage]  -- message
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CImgElement::HandleCaptureMessageForArea(CMessage * pMessage)
{
    RRETURN1(HandleMessage(pMessage), S_FALSE);
}


//=------------------------------------------------------------------------=
//
// Function:    DocPtToImgPt
//
// Synopsis:    Converts the point from a position relative to the document
//              to being a position relative to the upper left corner of
//              the iamge.
//
// Arguments:   POINT *ppt - The point to be converted.
//
//=------------------------------------------------------------------------=
void
CImgElement::DocPtToImgPt(POINT *ppt)
{
// BUGBUG: Call CDispNode::GlobalToContentPoint (brendand)
}

//+---------------------------------------------------------------------------
//
//  Member:     FindEnclosingAnchorScope
//
//  Synopsis:   Finds the enclosing <A HREF> tag
//
//  Returns:    NULL if no enclosing <A> or if enclosing <A> has no HREF
//              Otherwise, returns the enclosing <A> element
//
//----------------------------------------------------------------------------

CAnchorElement*
CImgElement::FindEnclosingAnchorScope()
{
    CTreeNode * pNode = GetFirstBranch();

    if (pNode)
    {
        for (pNode = pNode->Parent(); pNode && !pNode->HasLayout(); pNode=pNode->Parent())
        {
            if (pNode->Tag() == ETAG_A)
            {
                if (NULL != (LPTSTR) DYNCAST(CAnchorElement, pNode->Element())->GetAAhref())
                {
                    return DYNCAST(CAnchorElement, pNode->Element());
                }
                else
                {
                    return NULL;
                }
            }
        }
    }

    return NULL;
}

HRESULT
CImgElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT             hr          = S_OK;
    CUnitValue          uvBorder    = GetAAborder();
    int                 i;
    CAnchorElement *    pAnchor     = FindEnclosingAnchorScope();
    LPCTSTR             szUseMap    = NULL;

    _fBelowAnchor = !!pAnchor;

    if (uvBorder.IsNull())
    {
        // check if the image is inside an anchor
        // or useMap is set and begin with #

        if (    _fBelowAnchor
            ||  (szUseMap = GetAAuseMap()) != NULL && _tcschr(szUseMap, _T('#')))
        {
            uvBorder.SetValue( 2, CUnitValue::UNIT_PIXELS );
        }
    }

    // Set the anchor border
    if (!uvBorder.IsNull())
    {
        COLORREF crColor;
        DWORD dwRawValue = uvBorder.GetRawValue();

        if (_fBelowAnchor)
            crColor = GetAnchorColor(pAnchor);
        else
        {
            if (szUseMap && _tcschr(szUseMap, _T('#')))
            {
                Assert(pCFI->_pNodeContext && SameScope(this, pCFI->_pNodeContext));
                CTreeNode *  pNode = pCFI->_pNodeContext->Parent();
                CColorValue  colorValue;

                while(pNode && pNode->Tag() != ETAG_BODY)
                    pNode = pNode->Parent();
                
                if (pNode)
                {
                    colorValue = DYNCAST(CBodyElement, pNode->Element())->GetAAlink();
                }

                crColor = colorValue.IsDefined() ? colorValue.GetColorRef()
                                               : Doc()->_pOptionSettings->crAnchor();
            }
            else
                crColor = 0x00000000;
        }

        pCFI->PrepareFancyFormat();

        for ( i = BORDER_TOP; i <= BORDER_LEFT; i++ )
        {
            pCFI->_ff()._bBorderStyles[i] = fmBorderStyleSingle;
            pCFI->_ff()._ccvBorderColors[i].SetValue( crColor, FALSE );
            pCFI->_ff()._cuvBorderWidths[i].SetRawValue( dwRawValue );
        }

        pCFI->UnprepareForDebug();

    }

    hr = THR(super::ApplyDefaultFormat(pCFI));
    if (hr || !_pImage)
        goto Cleanup;

    _pImage->SetImgAnim(pCFI->_pcf->IsDisplayNone()
                        || pCFI->_pcf->IsVisibilityHidden());

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CBlockElement::Save
//
//  Synopsis:   Save the tag to the specified stream.
//
//-------------------------------------------------------------------------

HRESULT
CImgElement::Save(CStreamWriteBuff * pStmWrBuff, BOOL fEnd)
{
    if (fEnd)
        return S_OK;        // No end IMG tag

    CElement * pelAnchorClose = NULL;
    HRESULT hr;

    if (pStmWrBuff->TestFlag(WBF_FOR_RTF_CONV))
    {
        //
        // RichEdit2.0 crashes when it gets rtf with nested field
        // tags (easily generated by the rtf to html converter for
        // <a><img></a> (images in anchors).  To work around this,
        // we close any anchors this image may be in before writing
        // the image tag, and reopen them immediately after.
        //
        pelAnchorClose = GetFirstBranch()->SearchBranchToFlowLayoutForTag(ETAG_A)->SafeElement();
    }

    if (pelAnchorClose)
    {
        hr = pelAnchorClose->WriteTag(pStmWrBuff, TRUE, TRUE);
        if (hr)
            goto Cleanup;
    }

    hr = super::Save(pStmWrBuff, fEnd);
    if (hr)
        goto Cleanup;

    if (pelAnchorClose)
    {
        hr = pelAnchorClose->WriteTag(pStmWrBuff, FALSE, TRUE);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     GetAnchorColor
//
//  Synopsis:   Link color for anchor is taken from body
//
//  Returns:    COLORREF
//
//----------------------------------------------------------------------------

COLORREF
CImgElement::GetAnchorColor(CAnchorElement * pAnchorElement)
{
    Assert(pAnchorElement->Tag() == ETAG_A);
    return(pAnchorElement->GetLinkColor());
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
CImgElement::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    switch (dispid)
    {
    case DISPID_CImgElement_useMap:
        // Force a revalidation of the map
        EnsureMap();
        break;
    case DISPID_CImgElement_src:
        if (_pImage)
        {
            hr = _pImage->SetImgSrc(IMGF_REQUEST_RESIZE | IMGF_INVALIDATE_FRAME);
        }
        break;
    case DISPID_CImgElement_lowsrc:
        if (_pImage)
        {
            LPCTSTR szUrl = GetAAsrc();

            if (!szUrl)
            {
                Assert(_pImage);
                hr = _pImage->FetchAndSetImgCtx(GetAAlowsrc(), IMGF_REQUEST_RESIZE | IMGF_INVALIDATE_FRAME);
            }

        }
        break;

#ifndef NO_AVI
    case DISPID_CImgElement_dynsrc:
        if (_pImage)
        {
            hr = _pImage->SetImgDynsrc();
        }
        break;
#endif
    }

    if (OK(hr))
        hr = THR(super::OnPropertyChange(dispid, dwFlags));

    RRETURN(hr);
}


#ifndef NO_DATABINDING
class CDBindMethodsImg : public CDBindMethodsSimple
{
    typedef CDBindMethodsSimple super;

public:
    CDBindMethodsImg() : super(VT_BSTR, DBIND_ONEWAY) {}
    ~CDBindMethodsImg()    {}

    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const;

};

static const CDBindMethodsImg DBindMethodsImg;

const CDBindMethods *
CImgElement::GetDBindMethods()
{
    return &DBindMethodsImg;
}

//+----------------------------------------------------------------------------
//
//  Function: BoundValueToElement, CDBindMethods
//
//  Synopsis: Transfer data into bound checkbox.  Only called if DBindKind
//            allows databinding.
//
//  Arguments:
//            [id]      - ID of binding point.  For the checkbox, is always
//                        DISPID_VALUE.
//            [pvData]  - pointer to data to transfer, in this case a boolean.
//
//-----------------------------------------------------------------------------
HRESULT
CDBindMethodsImg::BoundValueToElement(CElement *pElem,
                                      LONG,
                                      BOOL,
                                      LPVOID pvData) const
{
    // Implement in quickest, but most inefficient way possible:
    RRETURN(DYNCAST(CImgElement, pElem)->put_src(*(BSTR *)pvData));
}
#endif // ndef NO_DATABINDING


CAreaElement *
CImgElement::GetCurrentArea()
{
    CAreaElement * pArea;

    if (HasCurrency())
    {
        EnsureMap();
        if (    _pMap
            && S_OK == _pMap->GetAreaContaining(Doc()->_lSubCurrent, &pArea))
        {
            return pArea;
        }
    }
    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImgElement::QueryStatus, public
//
//  Synopsis:   Implements QueryStatus for CImgElement
//
//  Notes:      This override of CImgBase::QueryStatus allows special
//              handling of hyperlink context menu entries for images
//              with active areas in client side maps
//
//----------------------------------------------------------------------------

HRESULT
CImgElement::QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    int idm;

    TraceTag((tagMsoCommandTarget, "CImgElement::QueryStatus"));

    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));
    Assert(cCmds == 1);

    MSOCMD *        pCmd    = &rgCmds[0];
    HRESULT         hr      = S_OK;
    CAreaElement *  pArea   = GetCurrentArea();

    Assert(!pCmd->cmdf);

    // Give first chance to <AREA>
    if (pArea)
    {
        hr = THR_NOTRACE(pArea->QueryStatusHelper(pguidCmdGroup, cCmds, rgCmds, pcmdtext));
        if (hr || pCmd->cmdf)
            goto Cleanup;
    }

    idm = CBase::IDMFromCmdID(pguidCmdGroup, pCmd->cmdID);
    switch (idm)
    {
    case IDM_FOLLOWLINKC:
    case IDM_FOLLOWLINKN:
    case IDM_PRINTTARGET:
    case IDM_SAVETARGET:

        // Plug a ratings security hole.
        if ((idm == IDM_PRINTTARGET || idm == IDM_SAVETARGET) &&
            S_OK == AreRatingsEnabled())
        {
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;
        }
        break;
    case IDM_ADDFAVORITES:
    case IDM_COPYSHORTCUT:
        {
            CAnchorElement * pAnchorElement = GetContainingAnchor();

            if (pAnchorElement && pAnchorElement->GetAAhref())
            {
                pCmd->cmdf = MSOCMDSTATE_UP;
            }
        }
        break;

    case IDM_IMAGE:
        // When a single image is selected, allow to bring up an insert image dialog
        pCmd->cmdf = MSOCMDSTATE_UP;
    }

    if (!pCmd->cmdf)
    {
        Assert(_pImage);
        hr = _pImage->QueryStatus(
                    pguidCmdGroup,
                    1,
                    pCmd,
                    pcmdtext);
        if (!pCmd->cmdf)
        {
            hr = THR_NOTRACE(super::QueryStatus(pguidCmdGroup, 1, pCmd, pcmdtext));
        }
    }

Cleanup:
    RRETURN_NOTRACE(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CImgElement::Exec, public
//
//  Synopsis:   Executes a command on the CImgElement
//
//  Notes:      This override of CImgBase::Exec allows special
//              handling of hyperlink context menu entries for images
//              with active areas in client side maps
//
//----------------------------------------------------------------------------

HRESULT
CImgElement::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    TraceTag((tagMsoCommandTarget, "CImgElement::Exec"));

    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));

    int                 idm         = CBase::IDMFromCmdID(pguidCmdGroup, nCmdID);
    HRESULT             hr          = MSOCMDERR_E_NOTSUPPORTED;
    POINT               pt;
    DWORD               dwPt;
    CAreaElement *      pArea       = GetCurrentArea();
    CAnchorElement *    pAnchorElement = GetContainingAnchor();

    // Give first chance to <AREA>
    if (pArea)
    {
        hr = THR_NOTRACE(pArea->ExecHelper(
                    pguidCmdGroup,
                    nCmdID,
                    nCmdexecopt,
                    pvarargIn,
                    pvarargOut));

        if (hr != MSOCMDERR_E_NOTSUPPORTED)
            goto Cleanup;
    }

    switch (idm)
    {
    case IDM_FOLLOWLINKC:
    case IDM_FOLLOWLINKN:
    case IDM_SAVETARGET:
    case IDM_PRINTTARGET:

        if (pAnchorElement && GetAAisMap() &&
            (idm == IDM_FOLLOWLINKC || idm == IDM_FOLLOWLINKN))
        {
            if (!pvarargIn ||  V_VT(pvarargIn) != VT_I4)
            {
                AssertSz(0, "Missing argument ptMouse for click on server-side image map");
                break;
            }
            dwPt = (DWORD)V_I4(pvarargIn);
            pt.x = MAKEPOINTS(dwPt).x;
            pt.y = MAKEPOINTS(dwPt).y;
            ScreenToClient(Doc()->_pInPlace->_hwnd, &pt);
            hr = THR(ClickOnServerMap(pt, idm == IDM_FOLLOWLINKN));
        }
        break;

    case IDM_COPYSHORTCUT:
        if (pAnchorElement && pAnchorElement->GetAAhref())
            hr = THR(pAnchorElement->CopyLinkToClipboard(GetAAalt()));
        break;

    case IDM_ADDFAVORITES:

        TCHAR   cBuf[pdlUrlLen];
        TCHAR * pszURL = cBuf;
        TCHAR * pszTitle;

        if (pAnchorElement)
        {
            CDoc * pDoc = Doc();

            hr = THR(pDoc->ExpandUrl(
                    pAnchorElement->GetAAhref(),
                    ARRAY_SIZE(cBuf),
                    pszURL,
                    pAnchorElement));
            pszTitle = (LPTSTR) pAnchorElement->GetAAtitle();
            if (!pszTitle)
                pszTitle = (LPTSTR) GetAAalt();
            if (!pszTitle)
                pszTitle = (LPTSTR) GetAAtitle();

            if (!hr && pszURL)
                hr = pDoc->AddToFavorites(pszURL, pszTitle);
        }
        break;
    }

    if (hr == MSOCMDERR_E_NOTSUPPORTED && _pImage)
    {
        hr = THR_NOTRACE(_pImage->Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut));
    }

    if (hr == MSOCMDERR_E_NOTSUPPORTED)
        hr = THR_NOTRACE(super::Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut));

Cleanup:
    RRETURN_NOTRACE(hr);
}

HRESULT
CImgElement::ClickOnServerMap(POINT pt, BOOL fOpenInNewWindow)
{
    HRESULT hr      = S_OK;
    TCHAR * szTemp  = NULL;
    CAnchorElement * pAnchorElement = GetContainingAnchor();

// BUGBUG: Call CDispNode::GlobalToContentPoint (brendand)
// (krisma) Or should we assume that the point passed in has
// already been transformed? I vote for the latter.
    hr = THR(Format(FMT_OUT_ALLOC, &szTemp, 0,
        _T("<0s>?<1d>,<2d>"),
        (LPTSTR) pAnchorElement->GetAAhref(),
        (long)pt.x,
        (long)pt.y));
    if (hr)
        goto Cleanup;

    hr = THR(Doc()->FollowHyperlink(szTemp,
               (LPTSTR) pAnchorElement->GetAAtarget(),
               pAnchorElement, NULL, FALSE,
               fOpenInNewWindow ));
    delete szTemp; // must delete temp string, error or no error
Cleanup:
    // Do not want this to bubble, so..
    if (S_FALSE == hr)
        hr = S_OK;
    RRETURN(hr);
}


HRESULT
CImgElement::ClickAction(CMessage * pMessage)
{
    HRESULT             hr              = S_OK;
    CAnchorElement *    pAnchorElement  = GetContainingAnchor();
    BOOL                fOpenInNewWindow;
    POINT               ptClick;

    // If we're a clientside image map, pass it to the area.
    EnsureMap();
    if (_pMap)
    {
        CAreaElement *  pArea;

        hr = THR(_pMap->GetAreaContaining(pMessage ? pMessage->lSubDivision : 0, &pArea));
        if (hr)
            goto Cleanup;

        hr = THR(pArea->ClickAction(pMessage));
        goto Cleanup;
    }

    // Don't handle clicks if not surrouneded by an anchor
    if (!pAnchorElement)
        goto Cleanup;

    // Pass click to anchor if not server-side image map

    if (!GetAAisMap())
    {
        // if we're an image w/o a map in an anchor
        // send the click to the anchor
        hr = THR(pAnchorElement->ClickAction(pMessage));

        goto Cleanup;
    }

    // We come here only when the click is on a server-side image map
    if (pMessage && (pMessage->message == WM_LBUTTONUP || pMessage->message == WM_MOUSEWHEEL))
    {
        // We came here because of mouse click
        Assert(pMessage->IsContentPointValid());
        fOpenInNewWindow = (pMessage->message != WM_MOUSEWHEEL) && (pMessage->dwKeyState & MK_SHIFT);
        ptClick = pMessage->ptContent;
    }
    else
    {
        CRect   rc;

        // We came here because of 'Enter' keystroke or the 'click' method call
        fOpenInNewWindow = FALSE;

        ptClick = g_Zero.pt;
    }

    hr = THR(ClickOnServerMap(ptClick, fOpenInNewWindow));

Cleanup:
    // Do not want this to bubble, so..
    if (S_FALSE == hr)
        hr = S_OK;

    RRETURN(hr);
}

const CImageElementFactory::CLASSDESC CImageElementFactory::s_classdesc =
{
    {
        &CLSID_HTMLImageElementFactory,      // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        NULL,                                // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                                // _pcpi
        0,                                   // _dwFlags
        &IID_IHTMLImageElementFactory,       // _piidDispinterface
        &s_apHdlDescs,                       // _apHdlDesc
    },
    (void *)s_apfnIHTMLImageElementFactory,  // _apfnTearOff
};



HRESULT STDMETHODCALLTYPE
CImageElementFactory::create(VARIANT varWidth, VARIANT varHeight, IHTMLImgElement**ppnewElem )
{
    HRESULT hr;
    CElement    *pElement = NULL;
    CImgElement *pImgElem;
    CVariant varI4Width;
    CVariant varI4Height;

    // We must return into a ptr else there's no-one holding onto a ref!
    if ( !ppnewElem )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppnewElem = NULL;

    // actualy ( [ long width, long height ] )
    // Create an Image element parented to the root site

    // This is some temproary unfinished code just to test the call
    hr = THR(_pDoc->CreateElement(ETAG_IMG, &pElement));
    if ( hr )
        goto Cleanup;

    pImgElem = DYNCAST(CImgElement, pElement);

    // Set the flag that indicates we're parented invisibly to the resize
    // this stops unpleasantness with ResizeElement.
    // BUGBUG: Is this necessary any longer (it's a RequestResize carry-over) (brendand)
    pImgElem->_pImage->_fCreatedWithNew = TRUE;

    hr = THR(pImgElem->QueryInterface ( IID_IHTMLImgElement, (void **)ppnewElem ));

    // BUGBUG rgardner this doesn't work - it causes a risize request - & GPF!
    // Image code needs to be smart enough to spot images parented to the root site

    // Set the width & height if supplied
    if (varWidth.vt != VT_EMPTY && varWidth.vt != VT_ERROR)
        pImgElem->_pImage->_fSizeInCtor = TRUE;

    hr = THR(varI4Width.CoerceVariantArg(&varWidth, VT_I4) );
    if ( hr == S_OK )
    {
        hr = pImgElem->putWidth(V_I4(&varI4Width));
    }
    if ( !OK(hr) )
        goto Cleanup;

    hr = THR(varI4Height.CoerceVariantArg(&varHeight, VT_I4) );
    if ( hr == S_OK )
    {
        hr = pImgElem->putHeight(V_I4(&varI4Height));
    }
    if ( !OK(hr) )
        goto Cleanup;

Cleanup:
    if (OK(hr))
    {
        hr = S_OK; // not to propagate possible S_FALSE
    }
    else
    {
        ReleaseInterface(*(IUnknown**)ppnewElem);
    }

    CElement::ClearPtr(&pElement);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// Member: get_mimeType
//
//----------------------------------------------------------------------------
extern BSTR GetFileTypeInfo(TCHAR * pszFileName);

STDMETHODIMP
CImgElement::get_mimeType(BSTR *pMimeType)
{
    HRESULT hr = S_OK;
    TCHAR * pchCachedFile = NULL;

    if ( !pMimeType )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pMimeType = NULL;

    hr = _pImage->GetFile(&pchCachedFile);

    if (!hr && pchCachedFile)
    {
        *pMimeType = GetFileTypeInfo(pchCachedFile);
    }

    MemFreeString(pchCachedFile);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
// Member: get_fileSize
//
//----------------------------------------------------------------------------
STDMETHODIMP
CImgElement::get_fileSize(BSTR *pFileSize)
{
    HRESULT hr = S_OK;
    TCHAR   szBuf[64];
    TCHAR * pchCachedFile = NULL;

    if (pFileSize == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pFileSize = NULL;

    Assert(_pImage);
    hr = _pImage->GetFile(&pchCachedFile);
    if (!hr && pchCachedFile)
    {
        WIN32_FIND_DATA wfd;
        HANDLE hFind = FindFirstFile(pchCachedFile, &wfd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);

            Format(0, szBuf, ARRAY_SIZE(szBuf), _T("<0d>"), (long)wfd.nFileSizeLow);
            *pFileSize = SysAllocString(szBuf);
        }
    }
    else
    {
        *pFileSize = SysAllocString(_T("-1"));
        hr = S_OK;
    }

Cleanup:
    MemFreeString(pchCachedFile);
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
// Member: get_fileCreatedDate
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CImgElement::get_fileCreatedDate(BSTR * pFileCreatedDate)
{
    HRESULT hr = S_OK;
    TCHAR * pchCachedFile = NULL;

    if (pFileCreatedDate == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pFileCreatedDate = NULL;

    Assert(_pImage);
    hr = _pImage->GetFile(&pchCachedFile);
    if (!hr && pchCachedFile)
    {
        WIN32_FIND_DATA wfd;
        HANDLE hFind = FindFirstFile(pchCachedFile, &wfd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);
            // we always return the local time in a fixed format mm/dd/yyyy to make it possible to parse
            // FALSE means we do not want the time
            hr = THR(ConvertDateTimeToString(wfd.ftCreationTime, pFileCreatedDate, FALSE));
        }
    }

Cleanup:
    MemFreeString(pchCachedFile);
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
// Member: get_fileModifiedDate
//
//----------------------------------------------------------------------------
STDMETHODIMP
CImgElement::get_fileModifiedDate(BSTR * pFileModifiedDate)
{
    HRESULT hr = S_OK;
    TCHAR * pchCachedFile = NULL;

    if (pFileModifiedDate == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pFileModifiedDate = NULL;

    Assert(_pImage);
    hr = _pImage->GetFile(&pchCachedFile);
    if (!hr && pchCachedFile)
    {
        WIN32_FIND_DATA wfd;
        HANDLE hFind = FindFirstFile(pchCachedFile, &wfd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);
             // we always return the local time in a fixed format mm/dd/yyyy to make it possible to parse
            // FALSE means we do not want the time
            hr = THR(ConvertDateTimeToString(wfd.ftLastWriteTime, pFileModifiedDate, FALSE));
        }
    }

Cleanup:
    MemFreeString(pchCachedFile);
    RRETURN(SetErrorInfo(hr));
}

// BUGBUG (lmollico): get_fileUpdatedDate won't work if src=file://image
//+---------------------------------------------------------------------------
//
// Member: get_fileUpdatedDate
//
//----------------------------------------------------------------------------
STDMETHODIMP
CImgElement::get_fileUpdatedDate(BSTR * pFileUpdatedDate)
{
    HRESULT hr     = S_OK;
    TCHAR   cBuf[pdlUrlLen];
    TCHAR * pszUrl = cBuf;

    if (pFileUpdatedDate == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pFileUpdatedDate = NULL;

    hr = THR(Doc()->ExpandUrl(GetAAsrc(), ARRAY_SIZE(cBuf), pszUrl, this));
    if (hr)
        goto Cleanup;

    if (pszUrl)
    {
        BYTE                        buf[MAX_CACHE_ENTRY_INFO_SIZE];
        INTERNET_CACHE_ENTRY_INFO * pInfo = (INTERNET_CACHE_ENTRY_INFO *) buf;
        DWORD                       cInfo = sizeof(buf);

        if (RetrieveUrlCacheEntryFile(pszUrl, pInfo, &cInfo, 0))
        {
            // we always return the local time in a fixed format mm/dd/yyyy to make it possible to parse
            // FALSE means we do not want the time
            hr = THR(ConvertDateTimeToString(pInfo->LastModifiedTime, pFileUpdatedDate, FALSE));
            DoUnlockUrlCacheEntryFile(pszUrl, 0);
        }

    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
// Member: get_href
//
//----------------------------------------------------------------------------
STDMETHODIMP
CImgElement::get_href(BSTR *pHref)
{
    HRESULT hr = S_OK;
    LPCTSTR pchUrl = NULL;

    if (pHref == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pHref = NULL;

#ifndef NO_AVI
    if (_pImage && _pImage->_pBitsCtx)
        pchUrl = _pImage->_pBitsCtx->GetUrl();
    else
#endif // ndef NO_AVI
        if (_pImage && _pImage->_pImgCtx)
        pchUrl = _pImage->_pImgCtx->GetUrl();

    if (pchUrl)
        *pHref = SysAllocString(pchUrl);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
// Member: get_protocol
//
//----------------------------------------------------------------------------
extern TCHAR * ProtocolFriendlyName(TCHAR * szUrl);

STDMETHODIMP
CImgElement::get_protocol(BSTR * pProtocol)
{
    HRESULT hr      = S_OK;
    LPCTSTR pchUrl  = NULL;
    TCHAR * pResult;

    if (pProtocol == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pProtocol = NULL;

#ifndef NO_AVI
    if (_pImage && _pImage->_pBitsCtx)
        pchUrl = _pImage->_pBitsCtx->GetUrl();
    else
#endif // ndef NO_AVI
    if (_pImage && _pImage->_pImgCtx)
        pchUrl = _pImage->_pImgCtx->GetUrl();

    if (pchUrl)
    {
        pResult = ProtocolFriendlyName((TCHAR *)pchUrl);
        if (pResult)
        {
            int z = (_tcsncmp(pResult, 4, _T("URL:"), -1) == 0) ? (4) : (0);
            * pProtocol = SysAllocString(pResult + z);
            SysFreeString(pResult);
        }
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
CImgElement::get_nameProp(BSTR * pName)
{
    *pName = NULL;

    TCHAR   cBuf[pdlUrlLen];
    TCHAR   * pszUrl  = cBuf;
    TCHAR   * pszName = NULL;
    HRESULT   hr      = THR(Doc()->ExpandUrl(GetAAsrc(), ARRAY_SIZE(cBuf), pszUrl, this));
    if (!hr)
    {
        pszName = _tcsrchr(pszUrl, _T('/'));
        if (!pszName)
            pszName = pszUrl;
        else
            pszName ++;

        * pName = SysAllocString(pszName);
    }
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------
//
//  Method:     CImgElem::GetSubdivisionCount
//
//  Synopsis:   returns the count of subdivisions
//
//-----------------------------------------------------------------------------

HRESULT
CImgElement::GetSubDivisionCount(long *pc)
{
    HRESULT hr = S_OK;

    if (IsEditable(TRUE) || !EnsureAndGetMap())
    {
        hr = THR(super::GetSubDivisionCount(pc));
        goto Cleanup;
    }

    
    *pc = _pMap->GetAreaCount();

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Method:     CImgElem::GetSubdivisionTabs
//
//  Synopsis:   returns the subdivisions tabindices
//
//-----------------------------------------------------------------------------

HRESULT
CImgElement::GetSubDivisionTabs(long *pTabs, long c)
{
    HRESULT         hr = S_OK;

    if (!c)
        goto Cleanup;

    EnsureMap();
    if (!_pMap)
        goto Cleanup;

    hr = THR(_pMap->GetAreaTabs(pTabs, c));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Method:     CImgElem::SubDivisionFromPt
//
//  Synopsis:   returns the subdivisions tabindices
//
//-----------------------------------------------------------------------------

HRESULT
CImgElement::SubDivisionFromPt(POINT pt, long *plSub)
{
    RECT            rcView;
    HRESULT         hr = S_OK;
    CLayout *       pLayout;

    Assert(GetFirstBranch());

    *plSub = -1;

    EnsureMap();
    if (!_pMap)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pLayout = Layout();
    pLayout->GetClientRect(&rcView);
    if (PtInRect(&rcView, pt))
    {
        // BUGBUG (mikejoch) This is a bit of a hack. We should really fix up
        // the areas so their coordinates are relative to the display tree
        // rather than being fixed in a (0,0) == top left world. This is non-
        // trivial, however, so we just work around it for now by tweaking the
        // point back to the LTR coordinate model.
        if (pLayout->GetElementDispNode()->IsRightToLeft())
        {
            CSize size;
            pLayout->GetSize(&size);
            pt.x += size.cx;
        }
        hr = THR(_pMap->GetAreaContaining(pt, plSub));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CImgElement::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//----------------------------------------------------------------------------

HRESULT
CImgElement::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    CAreaElement *  pArea = NULL;
    HRESULT         hr = S_FALSE;
    const CParaFormat  *  pPF = GetFirstBranch()->GetParaFormat();
    BOOL            fRTL = pPF->HasRTL(FALSE);
    CLayout *       pLayout = Layout();
    CSize           size = g_Zero.size;
    if(fRTL)
    {
        // we are only interested in adjusting the x positioning
        // for RTL direction.
        CRect rcClient;
        pLayout->GetClientRect(&rcClient);
        size.cx = rcClient.Width();
    }

    *ppShape = NULL;

    EnsureMap();

    if (!_pMap)
    {
        // must be a server-side image map
        Assert(GetAAtabIndex() >= 0 || (GetAAisMap() && GetContainingAnchor()));

        hr = THR(super::GetFocusShape(lSubDivision, pdci, ppShape));
        goto Cleanup;
    }

    hr = THR(_pMap->GetAreaContaining(lSubDivision, &pArea));
    if (hr)
        goto Cleanup;

    switch(pArea->_nShapeType)
    {
    case SHAPE_TYPE_RECT:
        {
            CRectShape * pShape = new CRectShape;

            if (!pShape)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            pShape->_rect = pArea->_coords.Rect;
            if(fRTL)
            {
                pShape->OffsetShape(-size);
            }

            *ppShape = pShape;
            break;
        }
    case SHAPE_TYPE_CIRCLE:
        {
            CCircleShape * pShape = new CCircleShape;

            if (!pShape)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            pShape->Set(pArea->_coords.Circle.lx,
                        pArea->_coords.Circle.ly,
                        pArea->_coords.Circle.lradius);
            if(fRTL)
            {
                pShape->OffsetShape(-size);
            }
            *ppShape = pShape;
            break;
        }
    case SHAPE_TYPE_POLY:
        {
            CPolyShape * pShape;

            if (pArea->_ptList.Size() < 2)
            {
                hr = S_FALSE;
                goto Cleanup;
            }

            pShape = new CPolyShape;
            if (!pShape)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            pShape->_aryPoint.Copy(pArea->_ptList, FALSE);
            if(fRTL)
            {
                pShape->OffsetShape(-size);
            }
            *ppShape = pShape;
            break;
        }
    default:
        Assert(FALSE && "Invalid Shape");
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//
//  Function:   DoSubDivisionEvents
//
//  Synopsis:   Fire the specified event on the given subdivision.
//
//  Arguments:  [dispidEvent]   -- dispid of the event to fire.
//              [dispidProp]    -- dispid of prop containing event func.
//              [pvb]           -- Boolean return value
//              [pbTypes]       -- Pointer to array giving the types of parms
//              [...]           -- Parameters
//
//-----------------------------------------------------------------------------

HRESULT
CImgElement::DoSubDivisionEvents(
    long        lSubDivision,
    DISPID      dispidEvent,
    DISPID      dispidProp,
    VARIANT   * pvb,
    BYTE      * pbTypes, ...)
{
    CAreaElement *  pArea;

    if (lSubDivision < 0)
        return S_OK;

    EnsureMap();
    if (_pMap && OK(_pMap->GetAreaContaining(lSubDivision, &pArea)))
    {
        va_list     valParms;
        va_start(valParms, pbTypes);

        Assert(pArea->GetFirstBranch());
        pArea->BubbleEventHelper(
            pArea->GetFirstBranch(),
            0,
            dispidEvent,
            dispidProp,
            FALSE,
            pvb,
            pbTypes,
            valParms);
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CImgElement::ShowTooltip
//
//  Synopsis:   Show the tooltip for this element.
//
//----------------------------------------------------------------------------

HRESULT
CImgElement::ShowTooltip(CMessage *pmsg, POINT pt)
{
    HRESULT         hr = S_OK;
    CAreaElement *  pArea = NULL;
    CStr            strTitle;
    BOOL            fRTL = FALSE;
    CDoc *          pDoc = Doc();

    if (pDoc->_pInPlace == NULL)
        goto Cleanup;

    // If the mouse is on an <AREA>, show the area's tooltip
    EnsureMap();
    if (_pMap && pmsg->lSubDivision >= 0)
    {
        if (S_OK == THR(_pMap->GetAreaContaining(pmsg->lSubDivision, &pArea)))
        {
            // We only show title tooltip for area
            //

            IGNORE_HR(pArea->GetUrlTitle(&strTitle));
            if (strTitle.Length() > 0)
            {
                CRect   rc, rcImgClient;

                pArea->GetBoundingRect(&rc);

                Assert(_pImage);
                _pImage->Layout()->GetClientRect(&rcImgClient, COORDSYS_GLOBAL);
                rc.OffsetRect(rcImgClient.TopLeft().AsSize());

                if(IsRectEmpty(&rc))
                {
                    rc.left = pt.x - 10;
                    rc.right = pt.x + 10;
                    rc.top = pt.y - 10;
                    rc.bottom = pt.y + 10;
                }

                // Complex Text - determine if element is right to left for tooltip style setting
                fRTL = GetFirstBranch()->GetCharFormat()->_fRTL;

                // Ignore spurious WM_ERASEBACKGROUNDs generated by tooltips
                CServer::CLock Lock(pDoc, SERVERLOCK_IGNOREERASEBKGND);

                FormsShowTooltip(
                    strTitle,
                    pDoc->_pInPlace->_hwnd,
                    *pmsg,
                    &rc,
                    (DWORD_PTR) this,
                    fRTL);
            }
            goto Cleanup;
        }
    }

    // check to see if tooltip should display the title property
    //
    hr = THR(super::ShowTooltip(pmsg, pt));
    if (hr == S_OK)
        goto Cleanup;

    Assert(_pImage);
    hr = _pImage->ShowTooltip(pmsg, pt);

Cleanup:
    RRETURN1 (hr, S_FALSE);
}

HRESULT CImgElement::GetHeight(long *pl)
{
    VARIANT v;
    HRESULT hr;

    hr = THR(s_propdescCImgElementheight.a.HandleUnitValueProperty(
            HANDLEPROP_VALUE | (PROPTYPE_VARIANT << 16),
            &v,
            this,
            CVOID_CAST(GetAttrArray())));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&v) == VT_I4);
    Assert(pl);

    *pl = V_I4(&v);

Cleanup:
    RRETURN(hr);
}

HRESULT CImgElement::putHeight(long l)
{
    VARIANT v;

    if ( l < 0 )
        l = 0;

    V_VT(&v) = VT_I4;
    V_I4(&v) = l;

    RRETURN(s_propdescCImgElementheight.a.HandleUnitValueProperty(
            HANDLEPROP_SET | HANDLEPROP_AUTOMATION | HANDLEPROP_DONTVALIDATE | (PROPTYPE_VARIANT << 16),
            &v,
            this,
            CVOID_CAST(GetAttrArray())));
}

HRESULT CImgElement::GetWidth(long *pl)
{
    VARIANT v;
    HRESULT hr;

    hr = THR(s_propdescCImgElementwidth.a.HandleUnitValueProperty(
            HANDLEPROP_VALUE | (PROPTYPE_VARIANT << 16),
            &v,
            this,
            CVOID_CAST(GetAttrArray())));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&v) == VT_I4);
    Assert(pl);

    *pl = V_I4(&v);

Cleanup:
    RRETURN(hr);
}

HRESULT CImgElement::putWidth(long l)
{
    VARIANT v;

    if ( l < 0 )
        l = 0;

    V_VT(&v) = VT_I4;
    V_I4(&v) = l;

    RRETURN(s_propdescCImgElementwidth.a.HandleUnitValueProperty(
            HANDLEPROP_SET | HANDLEPROP_DONTVALIDATE | HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16),
            &v,
            this,
            CVOID_CAST(GetAttrArray())));
}

STDMETHODIMP CImgElement::put_height(long l)
{
    RRETURN(SetErrorInfoPSet(putHeight(l), DISPID_CImgElement_width));
}

STDMETHODIMP CImgElement::get_height(long *p)
{
    Assert(_pImage);
    RRETURN (_pImage->get_height(p));
}

STDMETHODIMP CImgElement::put_width(long l)
{
    RRETURN(SetErrorInfoPSet(putWidth(l), DISPID_CImgElement_width));
}

STDMETHODIMP CImgElement::get_width(long *p)
{
    Assert(_pImage);
    RRETURN (_pImage->get_width(p));
}

STDMETHODIMP
CImgElement::get_src(BSTR * pstrFullSrc)
{
    HRESULT hr;
    Assert(_pImage);
    hr = _pImage->get_src(pstrFullSrc);
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------
//
//  member : put_src
//
//  sysnopsis : impementation of the interface src property set
//          since this is a URL property we want the crlf striped out
//
//-------------------------------------------------------------------

STDMETHODIMP
CImgElement::put_src(BSTR bstrSrc)
{
    RRETURN(SetErrorInfo(s_propdescCImgElementsrc.b.SetUrlProperty(bstrSrc,
                        this,
                        (CVoid *)(void *)(GetAttrArray()))));
}


//+----------------------------------------------------------------------------
//
// Methods:     get/set_hspace
//
// Synopsis:    hspace for aligned images is 3 pixels by default, so we need
//              a method to identify if a default value is specified.
//
//-----------------------------------------------------------------------------

STDMETHODIMP CImgElement::put_hspace(long v)
{
    return s_propdescCImgElementhspace.b.SetNumberProperty(v, this, CVOID_CAST(GetAttrArray()));
}

STDMETHODIMP CImgElement::get_hspace(long * p)
{
    HRESULT hr = s_propdescCImgElementhspace.b.GetNumberProperty(p, this, CVOID_CAST(GetAttrArray()));

    if(!hr)
        *p = *p == -1 ? 0 : *p;

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : [get_/put_] onload
//
//  synopsis : store in this element's propdesc
//
//+----------------------------------------------------------------------------

HRESULT
CImgElement:: put_onload(VARIANT v)
{
    HRESULT hr = THR(s_propdescCImgElementonload .a.HandleCodeProperty(
                HANDLEPROP_SET | HANDLEPROP_AUTOMATION |
                (PROPTYPE_VARIANT << 16),
                &v,
                this,
                CVOID_CAST(GetAttrArray())));

    RRETURN( SetErrorInfo( hr ));
}

HRESULT
CImgElement:: get_onload(VARIANT *p)
{
    HRESULT hr = THR(s_propdescCImgElementonload.a.HandleCodeProperty(
                    HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16),
                    p,
                    this,
                    CVOID_CAST(GetAttrArray())));

    RRETURN( SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//  Member :    CImgElement::IsHSpaceDefined
//
//  Synopsis:   if hspace is defined on the image
//
//+---------------------------------------------------------------------------
BOOL
CImgElement::IsHSpaceDefined() const
{
    DWORD v;
    CAttrArray::FindSimple( *GetAttrArray(), &s_propdescCImgElementhspace.a, &v);

    return v != -1;
}


HRESULT
CImgElement::get_readyState(BSTR * p)
{
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr=THR(s_enumdeschtmlReadyState.StringFromEnum(_pImage->_readyStateFired, p));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CImgElement::get_readyState(VARIANT * pVarRes)
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
CImgElement::get_readyStateValue(long *plRetValue)
{
    HRESULT     hr = S_OK;

    if (!plRetValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plRetValue = _pImage->_readyStateFired;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


void
CImgElement::Passivate()
{
    if (_pImage)
    {
        _pImage->Passivate();
        delete _pImage;
        _pImage = NULL;
    }
    super::Passivate();
}
