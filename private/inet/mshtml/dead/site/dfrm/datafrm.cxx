//----------------------------------------------------------------------------
//
//  Maintained by: alexa
//
//  Microsoft DataDocs
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       src\doc\datadoc\datafrm.cxx
//
//  Contents:   Implementation of the data frame.
//
//  Classes:    CDataFrame
//
//  Functions:  None.
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#include "dfrm.hxx"

#define _cxx_
#include "datafrm.hdl"

DeclareTag(tagDataFrame,"src\\ddoc\\datadoc\\datafrm.cxx","DataFrame");
DeclareTag(tagDataFrameTemplate,"src\\ddoc\\datadoc\\datafrm.cxx","DataFrameTemplate");
DeclareTag(tagDataFrameDebugPaint,"src\\ddoc\\datadoc\\datafrm.cxx","DataFrameDebugPaint");

extern TAG tagPropose;

// BUGBUG: Move this to CDUTIL.HXX
extern LONG g_alHimetricFrom8Pixels[2];

#define MAKE_SCROLL_HORIZONTAL 0x1000

// Note: the last 4 items in the key actions array should be VK_LEFT, VK_RIGHT, VK_UP,VK_DOWN.
KEY_MAP CDataFrame::s_aKeyActions[]= {
    {VK_HOME,   KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, fmScrollActionBegin},
    {VK_END,    KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, fmScrollActionEnd},
    {VK_PRIOR,  KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, fmScrollActionPageUp},
    {VK_NEXT,   KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, fmScrollActionPageDown},
    {VK_INSERT, KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)InsertNewRecordAt,0},
    {VK_DELETE, KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)DeleteCurrentRow,0},
    {VK_SPACE,  KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)ProcessSpaceKey,0},
    {VK_LEFT,   KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, (fmScrollActionLineDown | MAKE_SCROLL_HORIZONTAL)},
    {VK_RIGHT,  KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, (fmScrollActionLineUp | MAKE_SCROLL_HORIZONTAL)},
    {VK_UP,     KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, fmScrollActionLineUp},
    {VK_DOWN,   KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, fmScrollActionLineDown},
};

int CDataFrame::s_cKeyActions = ARRAY_SIZE(s_aKeyActions);

//
// please look in root frame for the class descriptions
//





CDataFrame::CDataFrameTBag::CDataFrameTBag() :
    _cstrFooter(CSTR_NOINIT),
    _cstrHeader(CSTR_NOINIT),
    _cstrOpenArgs(CSTR_NOINIT)
{
    _fDirection             = edgeVertical;
    _fAllowAdditions        = TRUE;
    _fAllowDeletions        = TRUE;
    _fAllowEditing          = TRUE;
    _fContinousForm         = TRUE;
    _uItems[ACROSS]         = 1;
    _uItems[DOWN]           = UNLIMITED;
    _fNavigationButtons     = TRUE;
    _fNewStar               = TRUE;
    _fShowNewRowAtBottom    = TRUE;
    _enAutosize             = fmEnAutoSizeNone;
    _fNewRecordShow         = FALSE;
    _lMinCols               = 1;    // always show at least 1 column
    _lMaxCols               = -1;   // expand to fit all coloumns
    _eMultiSelect           = fmMultiSelectExtended;

}


CDataFrame::CDataFrameTBag::~CDataFrameTBag()
{
    delete _dl.getpDLAS();
#if DBG == 1
    _dl.SetpDLAS(NULL);
#endif
}




//+-----------------------------------------------------------------------
//  Constructors, Destructors, Creation API
//------------------------------------------------------------------------

CDataFrame::CDataFrame(CDoc * pDoc, CSite * pParent)
    : super(pDoc, pParent)
{
#ifdef PRODUCT_97
    Assert(_pBindSource == NULL);
#endif
}


CDataFrame::CDataFrame(CDoc * pDoc, CSite * pParent, CDataFrame * pTemplate)
    : super(pDoc, pParent, pTemplate)
{
#ifdef PRODUCT_97
    _pBindSource = NULL;
#endif
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::PrivateQueryInterface, public
//
//  Synopsis:   Expose our IFaces
//
//  Arguments   iid     interface id of the queried interface
//              ppv     resulted interface pointer
//
//  Returns     HRESULT
//---------------------------------------------------------------------------
STDMETHODIMP
CDataFrame::PrivateQueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    TraceTag((tagDataFrame,"CDataFrame::PrivateQueryInterface"));

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS(this, IDataFrameExpert)
    }

    if (!*ppv)
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}





//+---------------------------------------------------------------------------
//
//  Member:     InitNew
//
//  Synopsis:   Initialization member function.
//              Set default values. Called after constructor.
//              creates a detail template
//
//  Returns:    Success
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::InitNew()
{
    TraceTag((tagDataFrame, "CDataFrame::Init"));

    HRESULT     hr = E_FAIL;
    CTBag * pTBag = TBag();

    Assert(_fOwnTBag);

    SetRepeated(TRUE);  // BUGBUG temporarely start with this

    hr = InitSubFrameTemplate(&_pDetail, SITEDESC_DETAILFRAME);

    if (hr)
    {
        goto Cleanup;
    }

    _pDetail->TBag()->_usTabIndex = 0;

    // if the layout is static ==> it means 1 item Across and 1 item Down.
    // if the layout is repeated verticaly/horizontaly ==> it means
    // UNLIMITED items Down/Across.
    pTBag->_uItems[pTBag->_fDirection] = IsRepeated() ? UNLIMITED : 1;

    // the oposite of the repeattion ddirection by default is no snaking
    // (items = 1).
    pTBag->_uItems[!pTBag->_fDirection] = 1;

    pTBag->_iScrollbars = fmScrollBarsBoth;

    // SetAutoSize (VB_FALSE);

Cleanup:
    RRETURN (hr);

}




//+---------------------------------------------------------------------------
//
//  Member      CDataFrame::operator new.
//
//  Synopsis    During the creation of the object from the template we copy the
//              part of the context of template assuming that the template is
//              derived from the same class as the object, that is getting
//              allocated and then will be constructed
//              (by corresponding constructor).
//
//  Arguments   s           size of the object to allocate
//              pOriginal   pointer to the Layout Template object.
//
//  Returns     pointer to a newly allocate object..
//
//  NOTE        the memcpy is called to copy (s) bytes. Be carefull!
//              Don't use it unless fully understand!
//
//----------------------------------------------------------------------------

void*
CDataFrame::operator new (size_t s, CDataFrame * pOriginal)
{
    TraceTag((tagDataFrame, "CDataFrame::operator new "));
    void * pNew = ::operator new(s);
    if (pNew)
        memcpy(pNew, pOriginal, sizeof(CDataFrame));
    return pNew;
}




//+---------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Detaches the layout template from the template hierarchy.
//
//----------------------------------------------------------------------------

void
CDataFrame::Detach ()
{
    TraceTag((tagDataFrame, "CDataFrame::Detach"));

    super::Detach();

    if (_fOwnTBag)
    {
        TBag()->_suAnchor.Clear();

        if (HasDLAccessorSource())
        {
            CDataLayerAccessorSource *pdlas;
            IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
            Assert(pdlas);
            pdlas->Passivate(); // Must be called after super::Detach!
        }
        RetireRecordSelector();
    }
}


//+---------------------------------------------------------------
//
//  Member:     CDataFrame::OnUIActivate
//
//  Synopsis:   Invalidates the current row to make it draw
//              focus feedback.
//
//---------------------------------------------------------------

STDMETHODIMP
CDataFrame::OnUIActivate(void)
{
    TraceTag((tagDataFrame, "CDataFrame::OnUIActivate %d",TBag()-> _ID));

    InvalidateCurrent();
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CDataFrame::OnUIDeactivate
//
//  Synopsis:   Invalidates the current row to make it draw
//              focus feedback.
//
//---------------------------------------------------------------

STDMETHODIMP
CDataFrame::OnUIDeactivate(BOOL fUndoable)
{
    TraceTag((tagDataFrame, "CDataFrame::OnUIDeactivate %d",TBag()-> _ID));

    InvalidateCurrent();
    return S_OK;
}





//+---------------------------------------------------------------
//
//  Member:     CDataFrame::InvalidateCurrent
//
//  Synopsis:   Invalidates the current row to make it draw
//              focus feedback.
//
//---------------------------------------------------------------

void CDataFrame::InvalidateCurrent()
{
    CDetailFrame * pCurrent;

    if ( _pDetail->TestClassFlag(SITEDESC_REPEATER) &&
         (NULL != (pCurrent = ((CRepeaterFrame *)_pDetail)->_pCurrent)) )
    {
        if ( (! pCurrent->_fInvalidated) && pCurrent->IsInSiteList() )
        {
            pCurrent->Invalidate(NULL, 0);
        }
    }

    return;
}




//+---------------------------------------------------------------------------
//
//  Member:     CreateInstance
//
//  Synopsis:   Create and instance of the template
//
//  Arguments:  pDataDoc    Form
//              pParent     parent instance
//              ppFrame     pointer to frame to be returned
//              info        create info
//
//
//  Returns:    Returns S_OK if everything is fine,
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::CreateInstance (
    CDoc * pDoc,
    CSite * pParent,
    OUT CSite * *ppFrame,
    CCreateInfo * pcinfo)
{
    TraceTag((tagDataFrame, "CDataFrame::CreateInstance"));


    Assert (pParent);
    HRESULT hr;

    Assert (ppFrame);

    CDataFrame * pdfr = new (this) CDataFrameInstance (pDoc, pParent, this);

    if (pdfr)
    {
        hr = pdfr->InitInstance ();
        if (hr)
            goto Error;

        hr = BuildInstance (pdfr, pcinfo);
        if (hr)
            goto Error;
    }
    else
        goto MemoryError;


Cleanup:

    *ppFrame = pdfr;
    RRETURN (hr);

Error:

    pdfr->Release();
    pdfr = NULL;
    goto Cleanup;

MemoryError:

    hr = E_OUTOFMEMORY;
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Member      CDataFrame::BuildDetail
//
//  Synopsis    Build detail section in instance
//              recursive (not directly, it doesn't call itself, but it calls
//              create, which will call Build).
//
//  Arguments   pNewInstance        the newly (just) created instance.
//              info                Procreation info structure, contains binding
//                                  information.
//
//  Returns     HRESULT
//
//  NOTE        this pointer here is always a TEMPLATE.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::BuildDetail (CDataFrame *pNewInstance, CCreateInfo * pcinfo)
{
    TraceTag((tagDataFrame,"CDataFrame::BuildDetail "));
    HRESULT hr = S_OK;

    Assert(this == getTemplate() && _fOwnTBag);
    Assert(pNewInstance);
    Assert(_pDetail->SiteDesc()->_etag == ETAG_DETAILFRAME);
    CRectl  rclNew;

    if (ShowHeaders())
    {
        hr = InitSubFrameInstance(&pNewInstance->_pHeader, _pHeader, pcinfo);
        if (hr)
        {
            goto Cleanup;
        }
    }
    else if (_pHeader)
    {
        _pHeader->CBaseFrame::SetVisible(FALSE);
    }

    if (IsRepeated())
    {
        CRepeaterFrame * pRepeater = new CRepeaterFrame(_pDoc, pNewInstance, (CDetailFrame *)_pDetail);
        if (!pRepeater)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        hr = pRepeater->Init();
        if (hr)
        {
            pRepeater->Release();
            goto Cleanup;
        }

        hr = THR(pNewInstance->AddToSites(pRepeater, ADDSITE_ADDTOLIST | ADDSITE_AFTERINIT));
        if (hr )
        {
            goto Cleanup;
        }
        pRepeater->Move (&_pDetail->_rcl, SITEMOVE_NOMOVECHILDREN | SITEMOVE_NOFIREEVENT);
        pNewInstance->_pDetail = pRepeater;
        _fRepeaterBelow = TRUE;
    }
    else
    {
        hr = InitSubFrameInstance(&pNewInstance->_pDetail, _pDetail, pcinfo);
        if (hr)
        {
            goto Cleanup;
        }
        _fRepeaterBelow = FALSE;
    }

    #if defined(PRODUCT_97)
    if (ShowFooters())
    {
        Assert(_pFooter);

        hr = InitSubFrameInstance(&pNewInstance->_pFooter, _pFooter, pcinfo);
        if (hr)
        {
            goto Cleanup;
        }
    }
    else if (_pFooter)
    {
        _pFooter->CBaseFrame::SetVisible(FALSE);
    }

    #endif

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member      CDataFrame::InitSubFrameInstance
//
//  Synopsis    Creates and hooks up the instancee for a subframe
//              Detail, Header, Footer
//
//  Arguments   pNewInstance  The new dataframe instance
//              pTemplate       the template to create an instance from
//
//  Returns     nothing
//
//  NOTE        this pointer here is always a TEMPLATE.
//
//----------------------------------------------------------------------------
HRESULT CDataFrame::InitSubFrameInstance(CBaseFrame **ppNewInstance, CBaseFrame *pTemplate, CCreateInfo * pcinfo)
{
    HRESULT hr;

    hr = pTemplate->CreateInstance(_pDoc, this, (CSite**)ppNewInstance, pcinfo);
    if (hr)
    {
        goto Cleanup;
    }
    hr = THR(AddToSites(*ppNewInstance, ADDSITE_ADDTOLIST | ADDSITE_AFTERINIT));
    if (hr)
    {
        (*ppNewInstance)->Release();
        goto Cleanup;
    }
    else
    {
        (*ppNewInstance)->Move (&pTemplate->_rcl, SITEMOVE_NOMOVECHILDREN | SITEMOVE_NOFIREEVENT);
    }
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member      CDataFrame::BuildInstance
//
//  Synopsis    Additinal method to be called during "Create time". Note it's
//              recursive (not directly, it doesn't call itself, but it calls
//              create, which will call Build).
//
//  Arguments   pNewInstance        the newly (just) created instance.
//              info                Procreation info structure, contains binding
//                                  information.
//
//  Returns     HRESULT
//
//  NOTE        this pointer here is always a TEMPLATE.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::BuildInstance (CDataFrame *pNewInstance, CCreateInfo * pcinfo)
{
    TraceTag((tagDataFrame,"CDataFrame::BuildInstance "));
    HRESULT hr = S_OK;

    Assert(this == getTemplate());
    Assert(pNewInstance);

#ifdef PRODUCT_97
    if (pcinfo->_pBindSource)
    {
        pNewInstance->_pBindSource = pcinfo->_pBindSource;
    }
#endif
    // BUGBUG later caption/header/footer

    hr = BuildDetail(pNewInstance, pcinfo);

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member      CDataFrame::InitSubFrameTemplate
//
//  Synopsis    Creates and hooks up the template for a subframe
//              Detail, Header, Footer
//
//  Arguments   ppNewTemplate   The template pointer to create
//
//  Returns     nothing
//
//  NOTE        this pointer here is always a TEMPLATE.
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::InitSubFrameTemplate(CBaseFrame **ppNewTemplate, SITEDESC_FLAG sf, BOOL fFooter)
{
    CBaseFrame * pDetail = NULL;
    HRESULT     hr = E_FAIL;

    Assert(_fOwnTBag);

    Assert(ppNewTemplate);

    *ppNewTemplate = 0;

    switch (sf)
    {
        case SITEDESC_DETAILFRAME:
            pDetail = new CDetailFrameTemplate(_pDoc, this);
            break;
        case SITEDESC_HEADERFRAME:
            pDetail = new CHeaderFrameTemplate(_pDoc, this, fFooter);
            break;
        default:
            Assert(FALSE && "InitSubFrameTemplate does not handle this class");
            break;
    }

    if (!pDetail)
        goto MemoryError;

    hr = pDetail->Init();
    if (hr)
        goto Cleanup;

    pDetail->_rcl = _rcl;

    //  BUGBUG it isn't clear that the Detail section should have
    //    an ID, since it doesn't appear in any Controls collection

    Assert(pDetail->TBag()->_ID == 0);

    hr = _pDoc->GetUniqueID(pDetail, &pDetail->TBag()->_ID);

    if (hr)
        goto Cleanup;

    hr = AddToSites(pDetail, ADDSITE_ADDTOLIST | ADDSITE_AFTERINIT); // this does not addref it

    if (hr)
    {
        goto Cleanup;
    }
    pDetail->AddRef();

    *ppNewTemplate = pDetail;
    int y;

    switch (sf)
    {
        case SITEDESC_DETAILFRAME:
            break;
        case SITEDESC_HEADERFRAME:
            pDetail->CBaseFrame::SetVisible(TRUE);
            pDetail->_rcl = _pDetail->getTemplate()->_rcl;
            // get the height of the first control in the template
            if (_pDetail->getTemplate()->_arySites.Size())
            {
                pDetail->_rcl.bottom = pDetail->_rcl.top + _pDetail->getTemplate()->_arySites[0]->_rcl.Height();
            }
            y = pDetail->_rcl.Height();
            _rcl.bottom += y;
            if (fFooter)
            {
                pDetail->_rcl.bottom = _rcl.bottom;
                pDetail->_rcl.top = _rcl.bottom - _pDetail->getTemplate()->_rcl.Height();

            }
            else
            {
                CRectl  rclNew;
                rclNew = _pDetail->_rcl;
                rclNew.OffsetRect(0, y);
                _pDetail->Move(&rclNew,  SITEMOVE_NOFIREEVENT);
                if (_pDetail != _pDetail->getTemplate())
                {
                    rclNew = _pDetail->getTemplate()->_rcl;
                    rclNew.OffsetRect(0, y);
                    _pDetail->getTemplate()->Move(&rclNew);
                    IGNORE_HR(UpdatePosRects1());
                }
                #if defined(PRODUCT_97)
                if (_pFooter)
                {
                    _pFooter->_rcl.OffsetRect(0, y);
                    _pFooter->Move(&_pFooter->_rcl, SITEMOVE_NOFIREEVENT);
                }
                #endif
            }

            break;

        default:
            Assert(FALSE && "InitSubFrameTemplate does not handle this class");
            break;
    }


Cleanup:
    if (pDetail)
        pDetail->Release();

    RRETURN (hr);

MemoryError:

    hr = E_OUTOFMEMORY;
    goto Cleanup;
}



//+-------------------------------------------------------------------------
//
//  Method:     CDataFrame::SetProposed
//
//  Synopsis:   Remember the deferred move rectangle for this site
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CDataFrame::SetProposed(CSite * pSite, const CRectl * prcl)
{
    TraceTag((tagPropose, "%ls/%d CDataFrame::SetProposed %d,%d,%d,%d to %d,%d,%d,%d",
        TBag()->_cstrName, TBag()->_ID,
        prcl->left - pSite->_rcl.left, prcl->top - pSite->_rcl.top, prcl->right - pSite->_rcl.right, prcl->bottom - pSite->_rcl.bottom,
        prcl->left, prcl->top, prcl->right, prcl->bottom));

    if (pSite == this)
    {
        return super::SetProposed(this, prcl);
    }

    CTBag *pTBag = TBag();

    if (_pHeader && (pSite == _pHeader || pSite == _pHeader->getTemplate()))
    {
        pTBag->_rclProposeHeader = *prcl;
    }
    #if defined(PRODUCT_97)
    else if (_pFooter && (pSite == _pFooter || pSite == _pFooter->getTemplate()))
    {
        pTBag->_rclProposeFooter = *prcl;
    }
    #endif
    else if (pSite->TestClassFlag(SITEDESC_REPEATER))
    {
        pTBag->_rclProposeRepeater = *prcl;
    }
    else
    {
        pTBag->_rclProposeDetail = *prcl;
    }

    ((CBaseFrame *)pSite)->_fProposedSet1 = TRUE;


    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CDataFrame::GetProposed
//
//  Synopsis:   Get back the deferred move rectangle for this site
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CDataFrame::GetProposed(CSite * pSite, CRectl * prcl)
{
    CTBag *pTBag = TBag();
    CRectl *prclProposed;

    if (pSite == this)
    {
        return super::GetProposed(this, prcl);
    }

    if (_pHeader && (pSite == _pHeader || pSite == _pHeader->getTemplate()))
    {
        prclProposed = &pTBag->_rclProposeHeader;
    }
    #if defined(PRODUCT_97)
    else if (_pFooter && (pSite == _pFooter || pSite == _pFooter->getTemplate()))
    {
        prclProposed = &pTBag->_rclProposeFooter;
    }
    #endif
    else if (pSite->TestClassFlag(SITEDESC_REPEATER))
    {
        prclProposed = &pTBag->_rclProposeRepeater;
    }
    else
    {
        prclProposed = &pTBag->_rclProposeDetail;
    }

    *prcl = ((CBaseFrame *)pSite)->_fProposedSet1? *prclProposed : pSite->_rcl;

    TraceTag((tagPropose, "%ls/%d CDataFrame::GetProposed %d,%d,%d,%d to %d,%d,%d,%d",
        pTBag->_cstrName, pTBag->_ID,
        prcl->left - pSite->_rcl.left, prcl->top - pSite->_rcl.top, prcl->right - pSite->_rcl.right, prcl->bottom - pSite->_rcl.bottom,
        prcl->left, prcl->top, prcl->right, prcl->bottom));

    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Member:     CreateToFit
//
//  Synopsis:   Create layout instances to fit the passed view rectangle.
//
//  Arguments:  rclView         bounding view rectangle.
//              dwFlags         parameter for the Move member function.
//
//  Note:       dwFlags         bits can be set to SITEMOVE_NOTIFYPARENT
//                              SITEMOVE_POPULATEDIR
//
//  Returns:    Return Values:S_OK if everything is fine
//              E_OUTOFMEMORY if not enough memeory to build the Grid object.
//              (Subject to change is allocation of Grid object).
//
//  Info:       The only reason for CDataFrame::CreateToFit is to check if by
//              some reason repeater's _rcl bottom is above the dataframe's _rcl
//              bottom, then we have to repopulate the repeater from bottom up.
//               (cases: rows were deleted manualy or from the cursor, or
//              detail templates shrinked).
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::CreateToFit (IN const CRectl * prclView, IN DWORD dwFlags)
{
    TraceTag((tagDataFrame, "CDataFrame::CreateToFit"));



    HRESULT hr;

    Edge            eLeft = (Edge) (1  - getDirection());
    Edge            eRight = (Edge)(eLeft + 2);
    long            alSlide[2] = { 0, 0 };

    alSlide[eLeft] = _rcl[eLeft] - _pDetail->_rcl[eLeft];
    if (alSlide[eLeft])
    {
        // if dataframe is big enough (no scrollbar) and the left is not adjusted,
        // then slide the detail, header, footer in the non-repeated direction if needed
        _pDetail->_rcl[eLeft] += alSlide[eLeft];
        _pDetail->_rcl[eRight] += alSlide[eLeft];
    }
    if (_pHeader)
    {
        alSlide[eLeft] = _rcl[eLeft] - _pHeader->_rcl[eLeft];
        if (alSlide[eLeft])
            _pHeader->MoveSiteBy(alSlide[0], alSlide[1]);
    }
    #if defined(PRODUCT_97)
    if (_pFooter)
    {
        alSlide[eLeft] = _rcl[eLeft] - _pFooter->_rcl[eLeft];
        if (alSlide[eLeft])
            _pFooter->MoveSiteBy(alSlide[0], alSlide[1]);
    }
    #endif

    hr = super::CreateToFit (prclView, dwFlags | SITEMOVE_NOINVALIDATE);

    if (SUCCEEDED(hr))
    {
        unsigned int iDirection = TBag()->_fDirection;
        CRectl       rclDetailSection;
        GetDetailSectionRectl (&rclDetailSection);
        Edge eTop = (Edge) iDirection;
        Edge eBottom = (Edge) (iDirection + 2);
        long lSlideBy = _pDetail->_rcl[eBottom] - rclDetailSection[eBottom];
        long lOutBy = rclDetailSection[eTop] - _pDetail->_rcl[eTop];
        long lInsideBy;
        long alDelta[2];

        // Assert (lOutBy >= 0);   // repeater should not be entirely inside of the
                                   // detail section (at least snapped to the top).

        if (lSlideBy < 0 && lOutBy > 0)
        {
            // if the repeater's bottom is above the detail section.

            // populate the repeater BOTTOM_UP
            hr = _pDetail->CreateToFit (&rclDetailSection, dwFlags & ~SITEMOVE_POPULATEDIR);

            lInsideBy =  rclDetailSection[eTop] - _pDetail->_rcl[eTop];
            if (lInsideBy < 0)
            {
                alDelta[iDirection] = lInsideBy;
                alDelta[1-iDirection] = 0;
                // repeater is totaly inside of the
                // detailsection and not snapped to the top
                _pDetail->MoveSiteBy (alDelta[0], alDelta[1]);
            }
        }

        if (!(dwFlags & SITEMOVE_NOINVALIDATE))
        {
#if 0
            // Invalidate ScrollBars
            CRectl  rcl;
            int     i;
            for (i = 0; i < 2; i++)
            {
                if (IsScrollBar(i))
                {
                    GetScrollbarRectl(i, &rcl);
                    _pDoc->InvalidateRectl (&rcl, FALSE);
                }
            }
#endif
            Invalidate(NULL, 0);
        }
    }

    RRETURN1(hr, S_FALSE);
}




//+-------------------------------------------------------------------------
//
//  Method:     CDataFrame::GetNaturalExtent
//
//  Synopsis:   Get the natural extent for the site.
//
//--------------------------------------------------------------------------

HRESULT
CDataFrame::GetNaturalExtent(DWORD dwAspect, LONG lindex, DVTARGETDEVICE * ptd,
    HDC hicTargetDev, DVEXTENTINFO * pExtentInfo, LPSIZEL psizel)
{
    CRectl  rclPropose;
    HRESULT hr;

    if (psizel)
    {
        rclPropose.left   = _rcl.left;
        rclPropose.top    = _rcl.top;
        rclPropose.right  = rclPropose.left + pExtentInfo->sizelProposed.cx;
        rclPropose.bottom = rclPropose.top + pExtentInfo->sizelProposed.cy;

        SetProposed(this, &rclPropose);

        hr = CalcProposedPositions();

        GetProposed(this, &rclPropose);

        psizel->cx = rclPropose.right - rclPropose.left;
        psizel->cy = rclPropose.bottom - rclPropose.top;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    RRETURN(hr);
}



//+-------------------------------------------------------------------------
//
//  Method:     CDataFrame::CalcSizeFromProperties
//
//  Synopsis:   Calculate the size of the dataframe, based on properties
//              prclDataFrame       - pointer to the calculated rectangle
//              pszllHeader         - pointer to the header size.
//              pszlDetailTemplate  - pointer to the size of the detail template
//              lMinRows
//              lMaxRows
//
//  Assumption: top/left is set
//              pszlHeader is NULL when there is no header
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CDataFrame::CalcSizeFromProperties(OUT CRectl   *prclDataFrame,
                                   IN  CRectl   *prclHeader,
                                   IN  CSizel   *pszlDetailTemplate,
                                   IN  long     lMaxRows,
                                   IN  long     lMinRows)
{
    TraceTag((tagPropose, "%ls/%d CDataFrame::CalcSizeFromProperties",
        TBag()->_cstrName, TBag()->_ID));

    HRESULT hr = S_OK;

    Assert (prclDataFrame);
    Assert (pszlDetailTemplate);

    CTBag * pTBag = TBag ();
    CSizel  szl;
    CSizel  szlSpacing (pTBag->_uPadding[0], pTBag->_uPadding[1]);
    int     iItems;
    ULONG   ulRows;

    prclDataFrame->bottom = prclDataFrame->top;
    if (prclHeader)
    {
         prclDataFrame->bottom += prclHeader->Dimension(1);
         Assert( !CheckSiteRectl(prclDataFrame));
    }

    // Calculate the detail section according to the properties

    GetNumberOfRecords (&ulRows);

    // If autosize (integral height) always calculate rclDetailSection based on properties,
    // because if we do resize the rclDetailSection at this point is not integral height based
    // and therefore is wrong for further calculations

    iItems = ulRows;

    if (lMinRows > iItems)
    {
        // we have to display an empty columns/rows
        iItems = lMinRows;
    }
    else if (lMaxRows >= 0 && lMaxRows < iItems)
    {
        // in case IsAutosizeHeight (): we need a vertical scroll bar
        iItems = lMaxRows;
    }
    szl.cy = pszlDetailTemplate->cy * iItems;
    szl.cy -= szlSpacing.cy;
    prclDataFrame->bottom += szl.cy;

    Assert (!CheckSiteRectl(prclDataFrame));

    // Adjust the size of the dataframe rectangle to encount the scrollbars
    // Note: for PRODUCT_96 only horizontal scrollbar is outside of the DetailSection
    // (not on top of it), since only Vertical direction is autosized
    if (pszlDetailTemplate->cx > prclDataFrame->Dimension(0))
    {
        prclDataFrame->bottom += (&g_sizelScrollbar.cx)[1];
    }

    RRETURN (hr);
}



//+-------------------------------------------------------------------------
//
//  Method:     CDataFrame::CalcPropertiesFromSize
//
//  Synopsis:   Calculate properties based on the szie of the data frame
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CDataFrame::CalcPropertiesFromSize (IN CRectl *prclDataFrame,
                                    IN CRectl *prclHeader,
                                    IN CSizel *pszlDetailTemplate,
                                    OUT long *pMaxRows,
                                    OUT long *pMinRows)
{
    TraceTag((tagPropose, "%ls/%d CDataFrame::CalcPropertiesFromSize",
        TBag()->_cstrName, TBag()->_ID));

    Assert (prclDataFrame);
    Assert (pszlDetailTemplate);

    HRESULT hr = S_OK;

    CRectl  rclDetailSection (prclDataFrame);

    if (prclHeader)
    {
        if (rclDetailSection.Dimension(1) > prclHeader->Dimension(1))
        {
            rclDetailSection.top = prclHeader->bottom;
        }
        else
        {
            rclDetailSection.SetRectEmpty();
        }
    }

    // Subtruct horizontal scrollbar
    if (rclDetailSection.Dimension(1) > (&g_sizelScrollbar.cx)[1] &&
        pszlDetailTemplate->cx > rclDetailSection.Dimension(0))
    {
        rclDetailSection.bottom -= (&g_sizelScrollbar.cx)[1];
        rclDetailSection.bottom += pszlDetailTemplate->cy - 1;    // size up
    }

    // Don't call OnPropertyChange here since it was CreateListBox fault that the properties
    // were set to the wrong values to begin with.
    *pMaxRows = *pMinRows = pszlDetailTemplate->cy?
                            rclDetailSection.Dimension(1)/pszlDetailTemplate->cy : 0;

    RRETURN (hr);
}



//+-------------------------------------------------------------------------
//
//  Method:     CDataFrame::CalcProposedPositions
//
//  Synopsis:   Calculate the proposed positions for children
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CDataFrame::CalcProposedPositions()
{
    TraceTag((tagPropose, "%ls/%d CDataFrame::CalcProposedPositions",
        TBag()->_cstrName, TBag()->_ID));


    HRESULT         hr = S_OK;
    CRectl          rclPropose;
    CRectl          rclHeader;
    CRectl      *   prclHeader = NULL;
    CBaseFrame  *   pDetailTemplate = _pDetail->getTemplate();
    CSizel          szlTemplate (pDetailTemplate->_rcl.Size());

    CTBag       *   pTBag = TBag();
    long            lMinRows = pTBag->_lMinRows;
    long            lMaxRows = pTBag->_lMaxRows;
    SetScratchOnSites(&_arySites, FALSE);

    if (!IsAutosizeDir(1))
        goto Cleanup;

    GetProposed(this, &rclPropose);

    if (_pHeader)
    {
        GetProposed(_pHeader, &rclHeader);
        prclHeader = &rclHeader;
    }

    if (pTBag->_eListBoxStyle == fmListBoxStylesListBox && !TBag()->_fMinMaxPropertyChanged)
    {   // listbox
        // (frankman) the _fMinMaxPropertyChanged is set to TRUE when explicit propertysettings
        // for min/max rows happened.
        // this is the case when the NrOfRowsProperty on the listbox is set.
        // without the flag, this routine would ignore the min/max row settings
        CalcPropertiesFromSize (&rclPropose, prclHeader, &szlTemplate, &lMaxRows, &lMinRows);
    }

    // combobox (and listbox)
    CalcSizeFromProperties(&rclPropose, prclHeader, &szlTemplate, lMaxRows,lMinRows);

    SetProposed(this, &rclPropose);

Cleanup:
    SetScratchOnSites(&_arySites, FALSE);

    RRETURN(hr);
}



//+-------------------------------------------------------------------------
//
//  Method:     CDataFrame::MoveToProposed
//
//  Synopsis:   Move children to the calculated proposed positions
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CDataFrame::MoveToProposed(DWORD dwFlags)
{
    HRESULT hr;

    TraceTag((tagPropose, "%ls/%d CDataFrame::MoveToProposed",
        TBag()->_cstrName, TBag()->_ID));

    SetProposedDetailSection ();
    hr = super::MoveToProposed (dwFlags);

    if ((_fIsDirtyRectangle || IsDirtyBelow()) && _pParent
        && _pParent->TestClassFlag(SITEDESC_BASEFRAME))
    {
        ((CBaseFrame*)_pParent)->SetDirtyBelow(TRUE);
    }
#ifdef PRODUCT_97
    _fOutsideScrollbars = _fProposedOutsideScrollbars;
#endif
    RRETURN(hr);

}




//+-------------------------------------------------------------------------
//
//  Method:     CDataFrame::SetProposedDetailSection
//
//  Synopsis:   calculate the detail section properties based on the current
//              proposed sizes
//
//  Returns:    Can modify the detail section properties (Min/Max Rows/Cols).
//
//--------------------------------------------------------------------------

void
CDataFrame::SetProposedDetailSection ()
{
    CRectl          rclPropose;
    CTBag       *   pTBag = TBag ();

    CBaseFrame  *   pDetailTemplate = _pDetail->getTemplate();
    CSizel          szlTemplate (pDetailTemplate->_rcl.Size());
    long            lMax, lMin;

    lMax = pTBag->_lMaxRows;
    lMin = pTBag->_lMinRows;

    GetProposed(this, &rclPropose);
    CalcPropertiesFromSize (&rclPropose,
                            _pHeader? &pTBag->_rclProposeHeader: NULL,
                            &szlTemplate,
                            &lMax,
                            &lMin);

    if (lMax != pTBag->_lMaxRows)
    {
        pTBag->_lMaxRows = lMax;
        OnPropertyChange(DISPID_MaxRows, 0);
    }

    if (lMin != pTBag->_lMinRows)
    {
        pTBag->_lMaxRows = lMax;
        OnPropertyChange(DISPID_MinRows, 0);
    }
    return;
}




//+---------------------------------------------------------------------------
//
//  Member:     Generate
//
//  Synopsis:   Generate/refresh
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------
HRESULT CDataFrame::Generate(long lReserved)
{
    CRectl rclForm(0, 0, _pDoc->_sizel.cx, _pDoc->_sizel.cy);

    IGNORE_HR(Generate(rclForm));

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     Generate
//
//  Synopsis:   Generate frame instances.
//              Purpose: Generate method is only called for the root layout.
//              This is the ONLY method that DataDoc should call to generate
//              all the instances.
//              This routine should pick up the root of the selection tree.
//
//  Arguments:  rclBound            generate frames in this bound rectangle.
//
//  Returns:    Returns S_OK if everything is fine,
//              E_INVALIDARG if parameters are NULL.
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::Generate (IN CRectl& rclBound)
{
    TraceTag((tagDataFrame, "CDataFrame::Generate"));

    Assert(_pParent->TestClassFlag(SITEDESC_BASEFRAME));
    return ((CBaseFrame *)_pParent)->getOwner()->Generate(_pParent->_rcl);
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::RemoveRelated
//
//  Synopsis:   checks for header/footer and calls them if exists
//              always called on template level
//
//  Arguments:  pSite       Detailsite that gets removed
//              dwFlags     DeletSite flags
//
//  Returns:    HRESULT
//
//---------------------------------------------------------------
HRESULT CDataFrame::RemoveRelated(CSite *pSite, DWORD dwFlags)
{

    TraceTag((tagDataFrame, "CDataFrame::RemoveRelated"));
    HRESULT hr = S_OK;
    if (_pHeader)
    {
        hr = THR(_pHeader->getTemplate()->RemoveRelated(pSite, dwFlags));
    }
    if (hr)
        goto Error;

    #if defined(PRODUCT_97)
    if (_pFooter)
    {
        hr = THR(_pFooter->getTemplate()->RemoveRelated(pSite, dwFlags));
    }
    #endif
Error:
    RRETURN(hr);

}
//-end-of-method-------------------------------------------------





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::InsertedAt (virtual)
//
//  Synopsis:   Insert a new data frame on the form.
//
//  Arguments:  pParent     Add to this parent site
//              clsid       Our CLSID (always CLSID_CDataFrame)
//              pstrName    Name for data frame
//              rcl         Create at this location.
//              dwOperations Flags
//
//  Returns:    HRESULT
//
//---------------------------------------------------------------

HRESULT
CDataFrame::InsertedAt(
    CSite * pParent,
    REFCLSID clsid,
    LPCTSTR pstrName,
    const RECTL * prcl,
    DWORD dwOperations)
{
    HRESULT             hr;

    Assert(clsid == CLSID_CDataFrame);

    hr = THR(InitNew());
    if (hr)
        goto Cleanup;

    hr = super::InsertedAt(pParent, clsid, pstrName, prcl, dwOperations);
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN(hr);
}


#if DBG==1
//+---------------------------------------------------------------
//
//  Member:     CDataFrame::SiteTypeFromCLSID
//
//  Synopsis:   Return the site type to insert into this
//              parent site from the CLSID
//
//  Arguments:  clsid       Class to create
//
//  Returns:    ELEMENT_TAG
//
//---------------------------------------------------------------

ELEMENT_TAG
CDataFrame::SiteTypeFromCLSID(REFCLSID clsid)
{
    Assert(FALSE);
    return super::SiteTypeFromCLSID(clsid);
}
#endif


//+------------------------------------------------------------------------
//
//  Member:     CDataFrame::InsertNewControl
//
//  Synopsis:   New controls created on the data frame are rerouted
//              to the detail template.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CDataFrame::InsertNewSite(
            REFCLSID clsid,
            LPCTSTR pstrName,
            const RECTL * prcl,
            DWORD dwOperations,
            CSite ** ppSite)
{
    RRETURN(_pDetail->getTemplate()->InsertNewSite(
            clsid,
            pstrName,
            prcl,
            dwOperations,
            ppSite));
}


//+---------------------------------------------------------------
//
//  Member:     CDataFrame::DragOver
//
//  Synopsis:   Determine whether this would be a move, copy, link
//              or null operation and manage UI feedback
//
//  Arguments   grfKeyState     Key status
//              ptlScreen       where the mouse is
//              pdwEffect       special effects flags
//
//  Returns     HRESULT
//
//---------------------------------------------------------------
HRESULT
CDataFrame::DragOver(DWORD grfKeyState, POINTL ptlScreen, LPDWORD pdwEffect)
{
    TraceTag((tagDataFrame,"CDataFrame::DragOver"));

    // BUGBUG we don't allow drop into the dataframe yet

    *pdwEffect = DROPEFFECT_NONE;

    return S_OK;
}

#if defined(PRODUCT_97)
//+---------------------------------------------------------------------------
//
//  Member:     GetGroupID
//
//  Synopsis:   Gets the repeaters GroupID at the specified hRow,
//              *adding a reference to it*.
//              The reference must be subsequently freed.
//
//  Arguments:  pGroupID            where to store GroupID
//              hRow                which row to refer to
//
//  Returns:    Returns OLE DB's HRESULT.
//              On failure, fills in *pGroupID with 0's
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::GetGroupID (HROW hRow, CDataLayerChapter *pGroupID)
{
    TraceTag((tagDataFrame, "CDataFrame::GetGroupID"));



    CDataLayerAccessorSource *pdlas;
    HRESULT hr = THR(getTemplate()->LazyGetDLAccessorSource(&pdlas));
    if (!hr)
    {
        hr = THR(pdlas->CreateChapter(hRow, pGroupID));
    }
    if (hr)
    {
        *pGroupID = CDataLayerChapter::TheNull;
    }
    RRETURN(hr);
}
#endif // defined(PRODUCT_97)



//+-------------------------------------------------------------------------
//
//  Method:     CDataFrame::DataSourceChanged
//
//  Synopsis:   Notify that the rowset we bound to is changed
//
//--------------------------------------------------------------------------

void
CDataFrame::DataSourceChanged()
{
    super::DataSourceChanged();
    if (HasDLAccessorSource())
    {
        CDataLayerAccessorSource *pdlas;
        IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
        Assert(pdlas);
        pdlas->Passivate();
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     GetNextRows
//
//  Synopsis:   get next n rows from the cursor
//
//  Arguments:  pRows       pointer to an array of rows
//              ulFetch     number of rows to fetch
//              iFetch      (-1 Previous), (1 Next)
//              pulFetched  pointer to a number of rows fetched from the
//                          cursor.
//              hrowCurrent current row to fetch from + iFetch.
//
//  Assumption: Code assumes that ulFetch is always bigger then 0
//
//  Returns:    Returns S_OK if everything is fine, S_FALSE on no records
//              available, else error.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::GetNextRows (OUT HROW *pRows,
                         IN  ULONG ulFetch,
                         OUT ULONG *pulFetched,
                         En_FetchDirection iFetch,
                         HROW hrowCurrent)
{
    TraceTag ((tagDataFrame, "CDataFrame::GetNextRows"));

    CDataLayerCursor *pCursor;
    HRESULT hr = THR(LazyGetDataLayerCursor(&pCursor));


    if (SUCCEEDED(hr))
    {
        Assert(pCursor);
        Assert (ulFetch > 0);
        Assert (iFetch*iFetch == 1);
        hr = pCursor->GetRowsAt(getGroupID(), hrowCurrent,
                                iFetch, iFetch*ulFetch, pulFetched, pRows );
    }
    else
    {
        *pulFetched = 0;
    }
    RRETURN1 (hr, S_FALSE);
}



HRESULT
CDataFrame::AddRefHRow(HROW *pHRow)
{
    TraceTag ((tagDataFrame, "CDataFrame::AddRefHRow"));

    CDataLayerCursor *pCursor;
    HRESULT hr = THR(LazyGetDataLayerCursor(&pCursor));
    ULONG   ulRows;


    if (SUCCEEDED(hr))
    {
        Assert(pCursor);
        hr = pCursor->GetRowsAt(getGroupID(), *pHRow,
                                0, 1, &ulRows, pHRow);

        Assert(ulRows == 1);
    }

    RRETURN1 (hr, S_FALSE);

}




//+---------------------------------------------------------------------------
//
//  Member:     GetNumberOfRecords
//
//  Synopsis:   get number of rows for this chapter.
//
//  Arguments:  pulRows     pointer to the result
//
//  Returns:    Returns S_OK if everything is fine, S_FALSE on no records
//              available, else error.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::GetNumberOfRecords (ULONG * pulRows)
{
    TraceTag ((tagDataFrame, "CDataFrame::GetNumberOfRecords"));

    CDataLayerCursor    *   pCursor;

    HRESULT hr = THR(LazyGetDataLayerCursor(&pCursor));

    if (SUCCEEDED(hr))
    {
        Assert(pCursor);
        hr = pCursor->GetSize(getGroupID(), pulRows);
        if (!hr && IsNewRecordShow())
        {
            *pulRows += 1;
        }
    }
    else
    {
        // BUGBUG - rich error handling required
        *pulRows = 1;
        hr = S_FALSE;
    }

    RRETURN1 (hr, S_FALSE);
}




//+---------------------------------------------------------------------------
//
//  Member:     GetRecordNumber
//
//  Synopsis:   get row number of the specified HROW
//
//  Arguments:  hRow            specified hRow
//              pulRecordNumber   pointer to a storage
//
//  Returns:    Returns row number
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::GetRecordNumber (HROW hRow, OUT ULONG *pulRecordNumber)
{
    CDataLayerCursor    *   pCursor;
    ULONG                   uChapterSize = 0;
    HRESULT                 hr;


    if (!hRow)
        goto Error;

    hr = THR(LazyGetDataLayerCursor(&pCursor));
    if (hr)
        goto Error;

    hr = pCursor->GetPositionAndSize(getGroupID(), hRow, pulRecordNumber, &uChapterSize);



Cleanup:
    RRETURN(hr);

Error:
    hr = E_FAIL;
    *pulRecordNumber = 0;   // record numbers are 1 based!
    goto Cleanup;
}





//+---------------------------------------------------------------------------
//
//  Member:     CalcRowPosition
//
//  Synopsis:   calculate the row position in the repeater of the passed record
//              number.
//
//  Arguments:  ulRecordNumber        record number (1 based).
//              plRowPosition         pointer to the position of the row in
//                                    the repeater
//              plPositionInRow       pointer to the position in the row
//
//  Note:       positions are relative to the repeater's top.
//
//----------------------------------------------------------------------------

void
CDataFrame::CalcRowPosition (ULONG ulRecordNumber,
                             OUT long *plRowPosition,
                             OUT long *plPositionInRow)
{
        CTBag       *pTBag = TBag();
        unsigned int iDirection = pTBag->_fDirection;
        unsigned int uItemsNonRepDir = pTBag->_uItems[1-iDirection];
        long         lPadding = pTBag->_uPadding[iDirection];
        long         lTemplateDim;
        long         lRowNumber;

        // Record numbers are 1 based.
        lRowNumber = (ulRecordNumber - 1) / uItemsNonRepDir;

        if (plRowPosition)
        {
            lTemplateDim = _pDetail->getTemplate()->_rcl.Dimension (iDirection) + lPadding;
            *plRowPosition = lRowNumber * lTemplateDim;
        }
        if (plPositionInRow)
        {
            lTemplateDim = _pDetail->getTemplate()->_rcl.Dimension (1-iDirection)
                           + pTBag->_uPadding[1-iDirection];
            *plPositionInRow = (ulRecordNumber - 1 - lRowNumber*uItemsNonRepDir)
                               * lTemplateDim;
        }

        return;
}




//+---------------------------------------------------------------------------
//
//  Member:     GetRowAt
//
//  Synopsis:   get row at the position
//
//  Arguments:  pRows       pointer to an HROW (result)
//              ulPosition  absolute position in the cursor (row number).
//
//  Returns:    Returns S_OK if everything is fine, S_FALSE if row is not
//              available
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::GetRowAt (OUT HROW *pHRow, ULONG ulPosition)
{
    TraceTag ((tagDataFrame, "CDataFrame::GetRowAt"));

    CDataLayerCursor *pCursor;
    HRESULT hr = THR(LazyGetDataLayerCursor(&pCursor));

    if (SUCCEEDED(hr))
    {
        Assert(pCursor);
        hr = pCursor->GetRowAt (getGroupID(), ulPosition, pHRow);
    }
    else
    {
        // BUGBUG - rich error handling required
        *pHRow = 0;
        hr = S_FALSE;
    }
    RRETURN1 (hr, S_FALSE);
}





//+---------------------------------------------------------------------------
//
//  Member:     IsPrimaryDirection
//
//  Synopsis:   Determine if the requested navigational direction is
//              parallel to the primary repetition direction
//
//  Arguments:  Direction:  the nav. direction queried.
//
//  Returns:    TRUE/FALSE.
//
//----------------------------------------------------------------------------

BOOL
CDataFrame::IsPrimaryDirection(NAVIGATE_DIRECTION Direction)
{
    ESnaking eSnakingType;

    GetSnakingType((unsigned *)&eSnakingType);

    return IsRepeated() && (((eSnakingType & 0x1) && VERTICAL(Direction)) ||
                            ((0 == (eSnakingType & 0x1)) && HORIZONTAL(Direction)));
}




//+------------------------------------------------------------------------
//
//  Member:     CDataFrame::SelectSite
//
//  Synopsis:   Selects the site based on the flags passed in
//
//
//  Arguments:  [pSite]         -- The site to select (for parent sites)
//              [dwFlags]       -- Action flags:
//                  SS_ADDTOSELECTION       add it to the selection
//                  SS_REMOVEFROMSELECTION  remove it from selection
//                  SS_KEEPOLDSELECTION     keep old selection
//                  SS_SETSELECTIONSTATE    set flag according to state
//                  SS_MERGESELECTION       merge site into selection
//
//  Returns:    HRESULT
//
//  Notes:      This method will call parent objects or children objects
//              depending on the action and passes the child/parent along
//
//-------------------------------------------------------------------------
HRESULT
CDataFrame::SelectSite(CSite * pSite, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (pSite == this)
    {
        // called on itself, check keep selection and call parent
        if (dwFlags & SS_CLEARSELECTION)
        {
            RootFrame(this)->ClearSelection(TRUE);
        }
        switch(dwFlags & (SS_ADDTOSELECTION | SS_REMOVEFROMSELECTION | SS_SETSELECTIONSTATE))
        {
        case SS_ADDTOSELECTION:
            hr = THR(_pParent->SelectSite(this, dwFlags));   //AddToSelectionTree(dwFlags));
            if (hr)
                goto Cleanup;
            Verify(!SetSelected(TRUE));
            break;
        case SS_REMOVEFROMSELECTION:
            Verify(!SetSelected(FALSE));
            hr = THR(_pParent->SelectSite(this, dwFlags));   //RemoveFromSelectionTree(dwFlags));
            if (hr)
                goto Cleanup;
            break;
        case SS_SETSELECTIONSTATE:
            // set state on itself
            if (dwFlags & SS_CLEARSELECTION)
            {
                SetSelectedQualifiers(NULL);
                SetSelected(FALSE);
            }
            else
            {
#ifdef PRODUCT_97
                // if the form is in design mode we use the template selected flag
                if (_pDoc->_fDesignMode)
                {
                    if (!_fOwnTBag)
                    {
                        SetSelected(_pTemplate->_fSelected);
                    }
                }
                else
#endif
                    Verify(!_pParent->SelectSite(this, dwFlags));
            }

            // call state on all children
            Verify(!_pDetail->SelectSite(_pDetail, dwFlags));

            if (_pHeader)
            {
                Verify(!_pHeader->SelectSite(_pHeader, dwFlags));
            }
#ifdef PRODUCT_97
            if (_pFooter)
            {
                Verify(!_pFooter->SelectSite(_pFooter, dwFlags));
            }
#endif
            break;
        default:
            Assert(FALSE && "These should be exclusive cases");
        }
    }
    else
    {
        // right now pSite should be a detailframe
        Assert(pSite->TestClassFlag(SITEDESC_DETAILFRAME));

        switch(dwFlags & (SS_ADDTOSELECTION | SS_REMOVEFROMSELECTION | SS_SETSELECTIONSTATE))
        {
        case SS_ADDTOSELECTION:
            hr = THR(SelectSubtree(((CDetailFrame *)pSite)->_pSelectionElement, dwFlags));
            if (hr)
                goto Cleanup;
            break;
        case SS_REMOVEFROMSELECTION:
            hr = THR(DeselectSubtree(((CDetailFrame *)pSite)->_pSelectionElement, dwFlags));
            if (hr)
                goto Cleanup;
            break;
        case SS_SETSELECTIONSTATE:
            SetSelectionState((CDetailFrame *)pSite);
            break;
        default:
            Assert(FALSE && "These should be exclusive cases");
        }
    }

Cleanup:
    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member      CDataFrame::SetSelectionState
//
//  Synopsis    Sets the selection state according to the selection tree
//
//  Arguments   [pNewInstance]   detail frame instance.
//
//  Returns     nothing
//
//----------------------------------------------------------------------------

void
CDataFrame::SetSelectionState (CDetailFrame *pNewInstance)
{
    QUALIFIER q;
    SelectUnit * pSelector;

    TraceTag((tagDataFrame,"CDataFrame::SetSelectionState(CDetailFrame*)"));

    Assert(pNewInstance);

    if ( _parySelectedQualifiers )
    {
        //  get the current row's bookmark and find it in the list
        pNewInstance->GetQualifier(&q);
        if ( NULL != (pSelector = _parySelectedQualifiers->Find(q)) )
        {
            pNewInstance->SetSelectionElement(pSelector);
            //  if the layout itself is selected
            if ( pSelector->aryfControls.GetReservedBit(SRB_LAYOUTFRAME_SELECTED) )
            {
                Verify(!pNewInstance->SetSelected(TRUE));
            }

            return;
        }
    }

    // if we get here the pNewInstance not supposed to be selected
    pNewInstance->SetSelectionElement(NULL);
    if (pNewInstance->_fSelected)
    {
        Verify(!pNewInstance->SetSelected(FALSE));
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SelectSubtree
//
//  Synopsis:   Merge the subtree hanging off the pSelector pointer
//              into the datadoc selection tree.
//
//  Arguments:  pSelector:  points to the selector element which is the
//                          root of the subtree that's to be added
//                          to the selection
//              [dwFlags]   same as in SelectSite
//
//  Returns:    S_OK if everything is fine
//              S_FALSE if selector wasn't added
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::SelectSubtree(SelectUnit * pSelector, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    TraceTag((tagDataFrame,"CDataFrame::SelectSubtree"));

    Assert(dwFlags & SS_ADDTOSELECTION);
    Assert(pSelector);

    SelectUnit * psuAnchor = getAnchor();

    if (!_parySelectedQualifiers)
    {
        hr = THR(AddToSelectionTree(dwFlags));
        if ( hr )
            goto Error;
        Assert(_parySelectedQualifiers);
    }

    // BUGBUG this will have to be revisited for hierarchical dataframes (istvanc)
    if ((dwFlags & SS_MERGESELECTION) && psuAnchor->qStart.IsValid())
    {
        hr = THR(_parySelectedQualifiers->Merge(psuAnchor, pSelector));
        if ( hr )
            goto Error;
    }
    else
    {
        psuAnchor->qStart = pSelector->qStart;
        if (!_parySelectedQualifiers->Find(pSelector))
        {
            hr = THR(_parySelectedQualifiers->Append(pSelector));
            if ( hr )
                goto Error;
        }
        else
        {
            hr = S_FALSE;
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);

Error:
    goto Cleanup;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::AddToSelectionTree
//
//  Synopsis:   Add this repeater to the selection tree as part of a new path
//
//  Arguments:  [dwFlags]   same as in SelectSite
//
//  Returns:    S_OK if everything is fine
//              E_OUTOFMEMORY if allocations fail
//              or propagates errors from OLE-DB
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::AddToSelectionTree(DWORD dwFlags)
{
    HRESULT hr = S_OK;

    TraceTag((tagDataFrame,"CDataFrame::AddToSelectionTree"));

    Assert(dwFlags & SS_ADDTOSELECTION);
    Assert(_parySelectedQualifiers == NULL);

    _parySelectedQualifiers = new CArySelector;
    if (!_parySelectedQualifiers)
        goto MemoryError;

    //  call up the chain
    if (!IsRootFrame(this))
    {
        Assert(_pParent);
        hr = THR(_pParent->SelectSite(this, dwFlags));
        if (hr)
            goto Error;
    }

Cleanup:
    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

Error:
    delete _parySelectedQualifiers;
    _parySelectedQualifiers = NULL;
    goto Cleanup;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::DeselectSubtree
//
//  Synopsis:   Subtract the subtree hanging off the pSelector pointer
//              from the datadoc selection tree.
//
//  Arguments:  pSelector:  points to the selector element which is the
//                          root of the subtree that's to be added
//                          to the selection
//              [dwFlags]   same as in SelectSite
//
//  Returns:    S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::DeselectSubtree(SelectUnit * pSelector, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    SelectUnit * pSelectorInTree;

    TraceTag((tagDataFrame,"CDataFrame::DeselectSubtree"));

    Assert(dwFlags & SS_REMOVEFROMSELECTION);
    Assert(pSelector);

    //  Make sure that the selector is actually in this array
    pSelectorInTree = _parySelectedQualifiers->Find(pSelector);
    if ( ! pSelectorInTree)
        goto Cleanup;       //  There's nothing to deselect, we are done

    if ( ! (pSelector->pqEnd || pSelectorInTree->pqEnd) )
    {
        //  They are single-record selectors, nuke'm!
        _parySelectedQualifiers->DeleteByValue(pSelectorInTree);
        delete pSelectorInTree;
        pSelectorInTree = NULL;
        //  this deletes the removed selector
    }
    else
    {
        Assert(pSelectorInTree->qStart.type == QUALI_BOOKMARK);

        //  We have to do some splitting.
        if ( ! pSelector->pqEnd )
        {
            //  This is the simple case
            if ( pSelector->qStart == pSelectorInTree->qStart )
            {
                //  swap in the ith row's qualifier for qStart
                hr = ReplaceQualifierWithRelativeRow(&pSelectorInTree->qStart,&pSelectorInTree->qStart,+1);
                if ( hr )
                    goto Error;
            }
            else if ( pSelector->qStart == *pSelectorInTree->pqEnd )
            {
                //  swap in the previous row's qualifier for *pqEnd
                hr = ReplaceQualifierWithRelativeRow(pSelectorInTree->pqEnd,pSelectorInTree->pqEnd,-1);
                if ( hr )
                    goto Error;
            }
            else
            {
                SelectUnit * pSelectorNew = new SelectUnit;
                if ( ! pSelectorNew )
                    goto MemoryError;

                hr = pSelectorNew->Copy(*pSelectorInTree);
                if ( hr )
                    goto Error;

                hr = ReplaceQualifierWithRelativeRow(pSelectorInTree->pqEnd,&pSelector->qStart,-1);
                if ( hr )
                    goto Error;

                hr = ReplaceQualifierWithRelativeRow(&pSelectorNew->qStart,&pSelector->qStart,+1);
                if ( hr )
                    goto Error;

                if ( pSelectorNew->pqEnd )
                {
                    pSelectorNew->Normalize();
                }

                hr = _parySelectedQualifiers->Append(pSelectorNew);
                if ( hr )
                    goto Error;
            }

            if ( pSelectorInTree->pqEnd )
            {
                pSelectorInTree->Normalize();
            }
        }
        else
        {
            //  BUGBUG: need to implement handling overlapping ranges.
            hr = E_NOTIMPL;
            goto Error;
        }
    }

    //  unlink the subtree if it became empty.
    if ( 0 == _parySelectedQualifiers->Size() )
    {
        hr = THR(RemoveFromSelectionTree(dwFlags));
    }

Cleanup:
    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;

Error:
    goto Cleanup;
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::ReplaceQualifierWithRelativeRow
//
//  Synopsis:   Get a new qualifier based on a qualiier and a rowoffset.
//
//  Arguments:  pqToModify:     Accepts the new qualifier
//              pqBase:         The base row's qualifier
//              iRowOffset:     The offset from the base row.
//
//  Returns:    S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::ReplaceQualifierWithRelativeRow(QUALIFIER * pqToModify, QUALIFIER * pqBase, int iRowOffset)
{
    CDataLayerCursor * pCursor;
    HROW hrow = NULL;
    ULONG nRowsFetched;
    HRESULT hr;

    hr = THR(LazyGetDataLayerCursor(&pCursor));
    if ( hr )
        goto Cleanup;

    Assert (iRowOffset*iRowOffset == 1);    // iRowOffset should be -1 or 1
    hr = THR(pCursor->GetRowsAt(pqBase->bookmark,iRowOffset,1,&nRowsFetched,&hrow));
    if ( hr || (nRowsFetched < 1) )
        goto Cleanup;

    hr = THR(pCursor->CreateBookmark(getGroupID(), hrow, &pqToModify->bookmark));
    if ( hr )
        goto Cleanup;


Cleanup:
    pCursor->ReleaseRows(1, &hrow);
    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::RemoveFromSelectionTree
//
//  Synopsis:   Remove dataframe from the selection tree
//
//  Arguments:  [dwFlags]   same as in SelectSite
//
//  Returns:    S_OK if everything is fine
//              E_OUTOFMEMORY if allocations fail
//              or propagates errors from OLE-DB
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::RemoveFromSelectionTree(DWORD dwFlags)
{
    HRESULT hr = S_OK;

    TraceTag((tagDataFrame,"CDataFrame::RemoveFromSelectionTree"));

    Assert(dwFlags & SS_REMOVEFROMSELECTION);
    Assert(_parySelectedQualifiers);

    //  call up the chain
    if (!IsRootFrame(this))
    {
        Assert(_pParent);
        hr = THR(_pParent->SelectSite(this, dwFlags));
    }

    delete _parySelectedQualifiers;
    _parySelectedQualifiers = NULL;

    RRETURN(hr);
}




HRESULT
CDataFrame::RecordNumberFromSelector (SelectUnit * psu, ULONG *pulRecordNumber)
{
    HRESULT hr;
    CDataLayerCursor * pCursor;

    Assert(psu);
    Assert(pulRecordNumber);

    if ( psu->qStart.type == QUALI_BOOKMARK )
    {
        hr = LazyGetDataLayerCursor(&pCursor);
        if ( hr )
            RRETURN(hr);

        hr = pCursor->GetPositionAndSize(psu->qStart.bookmark,pulRecordNumber,NULL);
    }
    else if ( psu->qStart.type == QUALI_INDEX )
    {
        hr = S_OK;
        *pulRecordNumber = psu->qStart.row;
    }
    else if ( psu->qStart.type == QUALI_DETAIL )
    {
        //  The listbox is non-repeated and the detail section is selected
        hr = S_OK;
        *pulRecordNumber = 0;
    }
    else
    {
        hr = E_NOTIMPL;
    }

    RRETURN(hr);
}





HRESULT
CDataFrame::SelectorFromRecordNumber (ULONG ulRecordNumber, SelectUnit * psu)
{
    HRESULT hr;
    CDataLayerCursor * pCursor;
    HROW hrow = NULL;

    Assert(psu);

    hr = LazyGetDataLayerCursor(&pCursor);
    if ( hr )
        goto Cleanup;

    hr = pCursor->GetRowAt(getGroupID(), ulRecordNumber, &hrow);
    if ( hr )
    {
        if ( hr == S_FALSE )
        {
            hr = E_INVALIDARG;
        }
        goto Cleanup;
    }

    hr = pCursor->CreateBookmark(getGroupID(),hrow,&psu->qStart.bookmark);
    if ( hr )
        goto Cleanup;

    psu->qStart.type = QUALI_BOOKMARK;

Cleanup:
    pCursor->ReleaseRows(1, &hrow);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::OnListBoxStyleChange
//
//  Synopsis:   update the listboxness-dependent properties, like
//              borderless MordControls and the like
//
//
//  Returns:    S_OK if everything is fine
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::OnListBoxStyleChange(fmListBoxStyles eNewListBoxStyle)
{
    BOOL flag;

    flag = _pDoc->_fDeferedPropertyUpdate;

    SetDeferedPropertyUpdate(TRUE);
    // Remove selection so we don't have to worry about setting colors
    RootFrame(this)->ClearSelection(TRUE);
    //  We know that the detail section's template is always a CDetailFrameTemplate
    ((CDetailFrame*)_pDetail->getTemplate())->SuppressControlBorders(eNewListBoxStyle != fmListBoxStylesNone);
    SetDeferedPropertyUpdate(flag);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     RefreshData
//
//  Synopsis:   Fetch data. For the repeater it means "get the group id" to
//              present itself with to a DataLayerCursor.
//              Standard mechanizm of fetching data.
//
//  Returns:    Returns S_OK if everything is fine.
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::RefreshData ()
{
    TraceTag((tagDataFrame, "CDataFrame::RefreshData"));


    HRESULT hr = S_OK;

    Assert(_pDetail);
#if defined(PRODUCT_97)
    if (_pBindSource && _pBindSource->getHRow())    // BUGBUG: FM fix for 0 hRow
    {
        // next check: is this dataframe already set up for hierarchy ?
        if (HasDLAccessorSource())
        {
            CDataLayerAccessorSource *pdlas;
            IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
            Assert(pdlas);
            if (pdlas->GetLinkMasterFields() && pdlas->GetLinkChildFields())
            {
                hr = GetGroupID(_pBindSource->getHRow(), &_groupID);
                if (_pDetail->TestClassFlag(SITEDESC_REPEATER))
                {
                    ((CRepeaterFrame *)_pDetail)->FlushCache();
                }
            }
        }
    }
#endif // defined(PRODUCT_97)

    // BUGBUG - need to do the same on header,footer, etc.
    _pDetail->RefreshData();

    if (!hr)
        hr = super::RefreshData();

    RRETURN (hr);
}


//
// Keyboard Section
//





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetKeyMap
//
//  Synopsis:   Returns the keymap of the class
//
//  Returns:    address of the keymap
//
//  Comments:   virtual
//
//----------------------------------------------------------------------------

KEY_MAP * CDataFrame::GetKeyMap(void)
{
    return s_aKeyActions;
}





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetKeyMapSize
//
//  Synopsis:   Returns the size of the keymap of the class
//
//  Returns:    size of the keymap
//
//  Comments:   virtual
//
//----------------------------------------------------------------------------

int CDataFrame::GetKeyMapSize(void)
{
    if ( _fEnabled )
    {
        if (IsListBoxStyle())
        {
            // in case of a list box VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN are handled by KeyScroll
            return s_cKeyActions;
        }
        else
        {
            // in case of a grid VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN are handled by NextControl
            return s_cKeyActions - 4;
        }
    }
    else
    {
        return 0;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::HandleMessage, public
//
//  Synopsis:   Handles events in the dataframe. Main goal is to
//              manage the selection
//
//  Arguments:  As per wndproc
//
//  Returns:    S_FALSE if not handled,
//              S_OK    if processed
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CDataFrame::HandleMessage(CMessage *pMessage, CSite *pChild)
{

    HRESULT         hr      = S_FALSE;

    Assert(_pParent);

    if (pMessage->message == WM_LBUTTONDOWN || pMessage->message == WM_LBUTTONDBLCLK)
    {
        // handle the scrollbars messages
        if ( _fEnabled )
        {
            int     i;
            RECT    rc;
            CRectl  rcl;
            long    lPosition;
            long    lVisible;
            long    lSize;
            POINT   pt = { MAKEPOINTS(pMessage->lParam).x, MAKEPOINTS(pMessage->lParam).y };

            for (i = 0; i < 2; i++)
            {
                if (GetScrollbarRectl(i, &rcl))
                {
                    _pDoc->DeviceFromHimetric(&rc, &rcl);
                    if (PtInRect(&rc, pt))
                    {
                        Verify(GetScrollInfo(i, &lPosition, &lVisible, &lSize));
                        OnScrollbarLButtonDown(
                                this,
                                i,
                                &rc,
                                rcl.right - rcl.left,
                                rcl.bottom - rcl.top,
                                lPosition, lVisible, lSize,
                                _pDoc,
                                pt,
                                (PFN_ONSCROLL)CParentSite::OnScroll);
                        hr = S_OK;
                        break;
                    }
                }
            }
        }
    }

    if (_pDoc->_fDesignMode && pChild)
    {
        // Otherwise, if the form we're in is in design mode, let
        // the parent handle it. (As long as the message didn't originally
        // come from the parent, as signified by pChild != NULL).
        hr = THR(_pParent->HandleMessage(pMessage, pChild));
        goto Cleanup;
    }
    else
    {
        hr = THR(super::HandleMessage(pMessage, pChild));
    }

    if (hr == S_FALSE && pChild)
    {
        hr = _pParent->HandleMessage(pMessage, this);
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::SetAnchorSite, public
//
//  Synopsis:   Sets _pselAnchor to point to the site/row currently
//              anchoring the selection range for the extended multiselect
//              Shift-Click.
//
//  Arguments:  pSite:  The site to become the anchor
//
//  Returns:    S_OK on success
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::SetAnchorSite(CSite * pSite)
{
    HRESULT hr;
    SelectUnit * psuAnchor = getAnchor();

    while ( pSite && ! pSite->TestClassFlag(SITEDESC_DETAILFRAME) )
    {
        pSite = pSite->_pParent;
    }

    if ( ! pSite )
        return E_FAIL;

    CDetailFrame * pdet = (CDetailFrame *)pSite;

    psuAnchor->Clear();

    hr = THR(pdet->GetQualifier(&psuAnchor->qStart));
    psuAnchor->aryfControls.SetReservedBit(SRB_LAYOUTFRAME_SELECTED);

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     SetCurrent
//
//  Synopsis:   Set current layout. Private.
//
//  Arguments:  pfr             new current frame
//              dwFlags         SITEMOVE_ flags
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::SetCurrent (CBaseFrame * pfr, DWORD dwFlags)
{
    if (_pDetail->TestClassFlag(SITEDESC_REPEATER))
    {
        return ((CRepeaterFrame *)_pDetail)->SetCurrent(pfr, dwFlags);
    }
    else
    {
        Assert(_pDetail == pfr);
        _pDetail->SetCurrent(TRUE);
        return S_OK;
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     KeyScroll
//
//  Synopsis:   Scroll from keyboard
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::KeyScroll (long lParam)
{
    HRESULT hr;
    unsigned uCode = 0;

    // BUGBUG we call the repeater back if we come from a template
    if(_pDetail->TestClassFlag(SITEDESC_REPEATER))
    {
        if ( lParam & MAKE_SCROLL_HORIZONTAL)
        {
            //  The zeroes stand for Horizontal
            hr = THR(OnScroll(0, (lParam & ~MAKE_SCROLL_HORIZONTAL), GetScrollPos(0)));
        }
        else
        {
            hr = ((CRepeaterFrame *)_pDetail)->KeyScroll (lParam);
        }
    }
    else
    {
        hr = S_FALSE; // BUGBUG implement it for non-repeated case
    }

    RRETURN1(hr, S_FALSE);
}




//+---------------------------------------------------------------------------
//
//  Member:     ProcessSpaceKey
//
//  Synopsis:   Pass the space key to the repeater
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::ProcessSpaceKey (long lReserved)
{
    HRESULT hr;

    // BUGBUG we call the repeater back if we come from a template
    if(_pDetail->TestClassFlag(SITEDESC_REPEATER))
    {
        hr = THR(((CRepeaterFrame *)_pDetail)->ProcessSpaceKey(lReserved));
    }
    else
    {
        hr = S_FALSE; // BUGBUG implement it for non-repeated case
    }

    RRETURN1(hr, S_FALSE);
}




//+---------------------------------------------------------------------------
//
//  Member:     InsertNewRecordAt
//
//  Synopsis:   Inserts new record at the Begining/And/InPlace
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::InsertNewRecordAt (long lLocation)
{
    TraceTag((tagDataFrame, "CDataFrame::InsertNewrecordAt"));

    CDataLayerCursor *  pCursor;
    HRESULT             hr = S_OK;
    HROW                hrow, hrowCurrent;
    ULONG               ulRecordNumber;
    long                alDelta[2];
    long                lPosition;
    CTBag           *   pTBag = TBag();
    int                 iDirection = pTBag->_fDirection;
    Edge                eTop;
    Edge                eBottom;
    Edge                eLeft;
    CBaseFrame      *   plfr;
    CRectl              rcl;
    CRectl              rclDetailSection;
    CRepeaterFrame  *   pRepeater;
    CDataLayerBookmark  dlbookmark;
    BOOL                fNoCursorEvents;

    // prevent events firing back from the cursor
    fNoCursorEvents = pTBag->_fNoCursorEvents;
    pTBag->_fNoCursorEvents = TRUE;

    if ( pTBag->_eListBoxStyle != fmListBoxStylesNone )
        goto Cleanup;

    if (_pDetail->TestClassFlag(SITEDESC_REPEATER))
    {
        pRepeater = (CRepeaterFrame *)_pDetail;
        hr = THR(LazyGetDataLayerCursor(&pCursor));

        if (SUCCEEDED(hr))
        {
            Assert(pCursor);

            switch (lLocation)
            {
                case    AT_INPLACE:
                    if (pRepeater->_pCurrent)
                    {
                        // Step 1a: create new hrow
                        hr = pCursor->CreateBookmark(getOwner()->getGroupID(),
                                                     pRepeater->_pCurrent->_hrow,
                                                     &dlbookmark);
                        hr = pCursor->NewRowAfter (dlbookmark, &hrow);
                        if (hr)
                            goto Cleanup;

                        // Step 1b: get the record number and calculate the
                        // relative position in the repeater of the new row
                        GetRecordNumber (hrow, &ulRecordNumber);
                        CalcRowPosition (ulRecordNumber, &lPosition, 0);

                        // Step 1c: create layout for the new row
                        pCursor->GetRowAt(dlbookmark, &hrowCurrent);
                        pRepeater->_fPopulateDirection = POPULATE_DOWN;
                        CBaseFrame *plfrBase = pRepeater->CacheLookUpRow(hrowCurrent);
                        hr = pRepeater->CreateLayout (hrow, &plfr, plfrBase);
                        if (hr)
                        {
                            pCursor->ReleaseRows(1, &hrow);
                            goto Cleanup;
                        }

                        // Step 1d: prepare repeater for scrolling
                        Assert (plfr);
                        hr = pRepeater->PrepareToScroll (iDirection, SB_INSERTAT, FALSE, lPosition, alDelta, plfr);
                        if (hr)
                            goto Cleanup;

                        GetDetailSectionRectl (&rclDetailSection);
                        rcl.Intersect (&plfr->_rcl, &rclDetailSection);
                        eTop = (Edge) iDirection;
                        eBottom = (Edge) (iDirection + 2);
                        eLeft = (Edge) (1 - iDirection);
                        if (rcl[eTop] == plfr->_rcl[eTop] && rcl[eBottom] == plfr->_rcl[eBottom] &&
                            rcl[eLeft] == plfr->_rcl[eLeft])
                        {
                            // inserted row is fully visible
                            if (TBag()->_uItems[1-iDirection] != 1)
                            {
                                // snaking
                                _fPixelScrollingDisable = TRUE;
                                rcl = rclDetailSection; // rcl is a scrollable rectangle
                                rcl[eTop] = plfr->_rcl[eTop];
                                rcl[eBottom] = rclDetailSection[eBottom];
                                RECT rc;
                                _pDoc->DeviceFromHimetric(&rc, &rcl);
                                _pDoc->Invalidate(&rc, NULL, NULL, 0);
                            }
                            else
                            {
                                // not snaking
                                alDelta[iDirection] = plfr->_rcl.Dimension(iDirection);
                                alDelta[1-iDirection] = 0;

                                CRectl rclScroll (plfr->_rcl);
                                rclScroll[eBottom] = rclDetailSection[eBottom];
                                ScrollRegion (rclScroll, alDelta[0], alDelta[1]);
                                rcl = plfr->_rcl;
                            }
                            IGNORE_HR(UpdatePosRects1());
                        }
                        else
                        {
                            plfr->ScrollIntoView();
                        }

                        break;
                    }
                    Assert (FALSE);

                case    AT_BEGINNING:
                    hr = pCursor->NewRowAfter(
                            CDataLayerBookmark::ChapterSimple(getGroupID(),
                                CDataLayerBookmark::TheFirst ),
                            NULL );
                    if (SUCCEEDED(hr))
                    {
                        hr = KeyScroll(KB_TOP);
                    }
                    break;

                case    AT_END:
                    hr = pCursor->NewRowAfter(
                            CDataLayerBookmark::ChapterSimple(getGroupID(),
                                CDataLayerBookmark::TheLast ),
                            NULL );
                    if (SUCCEEDED(hr))
                    {
                        hr = KeyScroll(KB_BOTTOM);
                    }
                    break;

                default:
                    hr = E_INVALIDARG;
                    goto Cleanup;
            }
        }
    }

Cleanup:

    pTBag->_fNoCursorEvents = fNoCursorEvents;
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     DeleteCurrentRow
//
//  Synopsis:   Delete the current row.
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::DeleteCurrentRow (long lReserved)
{
    TraceTag((tagDataFrame, "CDataFrame::DeleteCurrentRow"));

    CDataLayerCursor    *   pCursor;

    HRESULT                 hr = S_OK;
    HROW                    hrow;
    CTBag               *   pTBag = TBag();
    int                     iDirection = pTBag->_fDirection;
    Edge                    eTop;
    Edge                    eBottom;
    Edge                    eLeft;
    CDetailFrame        *   plfr;
    CDetailFrame        *   plfrNext;
    CRectl                  rclRow, rclScroll;
    CRectl                  rclDetailSection;
    CRepeaterFrame      *   pRepeater;
    CDataLayerBookmark      dlbookmark;
    BOOL                    fNoCursorEvents;

    // prevent events firing back from the cursor
    fNoCursorEvents = pTBag->_fNoCursorEvents;
    pTBag->_fNoCursorEvents = TRUE;

    if ( pTBag->_eListBoxStyle != fmListBoxStylesNone )
        goto Cleanup;

    Assert(_pDetail);
    if (_pDetail->TestClassFlag(SITEDESC_REPEATER))
    {
        pRepeater = (CRepeaterFrame *)_pDetail;
        plfr = pRepeater->_pCurrent;
        if (plfr && plfr->_hrow)    // can't delete new record (_hrow == NULL)
        {
            hr = THR(LazyGetDataLayerCursor(&pCursor));

            if (SUCCEEDED(hr))
            {
                // Step 1: Get the next row before deleting the current one.

                pRepeater->_fPopulateDirection = POPULATE_DOWN;
                hr = pRepeater->GetNextLayoutFrame (plfr, (CBaseFrame **)&plfrNext);

                if (!SUCCEEDED(hr))
                    goto Cleanup;

                if (!plfrNext)
                {
                    // it means we are about to delete last row
                    // so we need to set the previous row  to be a new current

#if DBG == 1
                    ULONG   ulRows;
                    HROW    hrow;
                    HRESULT hr1 = pCursor->GetRowsAt(getGroupID(), plfr->_hrow,
                                                     1, 1, &ulRows, &hrow);
                    Assert (hr1 || !ulRows);    // the last row in set.
#endif

                    pRepeater->_fPopulateDirection = POPULATE_UP;
                    hr = pRepeater->GetNextLayoutFrame (plfr, (CBaseFrame **)&plfrNext);
                    if (!SUCCEEDED(hr))
                        goto Cleanup;
                    if (!plfrNext)
                    {
                        // this is the case when we have only 1 row in the repeater
                        // we don't want to delete it.

                        // in design mode have a hrow=0 trick
#if DBG == 1
                        ULONG   ulRows;
                        GetNumberOfRecords (&ulRows);
                        Assert (ulRows == 1);
#endif
                        hr = S_OK;
                        goto Cleanup;
                    }
                }

                BOOL fIsInSiteList = plfr->IsInSiteList ();
                BOOL fSelected = plfr->_fSelected;
                rclRow = plfr->_rcl;

                // Step 3: delete old current _hrow
                hrow = plfr->getHRow();
                Assert(pCursor);
                hr = pCursor->DeleteRows(1, &hrow);
                if (!SUCCEEDED(hr))
                    goto Cleanup;   // BUGBUG: rich error handling


                // Step 2: Make new row current
                SetCurrent(plfrNext, SETCURRENT_NOINVALIDATE);

                // Step 5: recycle the old current row.
                pRepeater->MoveToRecycle(plfr);
                pRepeater->CalcRectangle();

                // Step 3: Test if the current row wass visible in the detail section
                //         or have being scrolled out.
                GetDetailSectionRectl (&rclDetailSection);
                eTop = (Edge) iDirection;
                eBottom = (Edge) (iDirection + 2);
                eLeft = (Edge) (1 - iDirection);

                // 1. check if the deleted row is the last row
                if (pRepeater->_fPopulateDirection == POPULATE_UP)
                {
                    // then scroll the whole thing to the end of record set
                    hr = KeyScroll(fmScrollActionEnd);
                }
                else
                {
                    if (fIsInSiteList)
                    {
                        // if current row is in the site list its coordinates
                        // should be in the visible range.
//                      Assert (IsRowVisibleIn(eTop, eBottom, rclRow, rclDetailSection));
                        Assert (!(rclRow[eTop] >= rclDetailSection[eBottom] || rclRow[eBottom] <= rclDetailSection[eTop]));
                        CreateToFit (&_rcl, SITEMOVE_POPULATEDIR);
                    }
                    else
                    {
                        // move the new row to the correct position
                        plfrNext->Move (&rclRow, SITEMOVE_NOFIREEVENT |
                                                 SITEMOVE_NOINVALIDATE);
                        plfrNext->ScrollIntoView ();
                    }
                }

                if (fSelected)
                {
                    hr = THR(plfrNext->SelectSite(plfrNext, SS_ADDTOSELECTION));
                    if (hr)
                        goto Cleanup;
                }
#if NEVER
                if (fIsInSiteList)
                {
                    // if current row is in the site list its coordinates
                    // should be in the visible range.
                    Assert (IsRowVisibleIn (eTop, eBottom, plfr->_rcl, rclDetailSection));

                    // 1. check if the deleted row is the last row
                    if (pRepeater->_fPopulateDirection == POPULATE_UP)
                    {
                        // then scroll the whole thing to the end of record set
                        hr = KeyScroll(fmScrollActionEnd);
                    }
                    else
                    {
                        // prepare rcl as a real estate rectangle and populate the
                        // region
                        rcl = rclDetailSection;
                        rcl[eTop] = plfr->_rcl[eTop];
                        fRealEstate = pRepeater->Populate (plfrNext, rcl, 0, UNLIMITED,
                                SITEMOVE_NOFIREEVENT | SITEMOVE_NOINVALIDATE);

                        // BUGBUG: need to add check if after the last row there is a real
                        // estate so we need to do KeyScroll(KB_BOTTOM);

                        // prepare rcl as a scrollable rectangle and scroll the
                        // region
                        rcl = rclDetailSection;
                        rcl[eTop] = plfr->_rcl[eBottom];
                        if (rcl[eTop] >= rcl[eBottom])
                        {
                            rcl[eTop] = rclRow[eTop];
                        }
                        else
                        {
                            alDelta[iDirection] = -rclRow.Dimension(iDirection);
                            alDelta[1-iDirection] = 0;
                            ScrollRegion (rcl, alDelta[0], alDelta[1], TRUE);
                            rcl[eBottom] = rcDetailSection[eBottom];
                            rcl[eTop] = rcDetailSection[eBottom] + alDelta[iDirection];
                        }
                        RECT rc;
                        _pDoc->DeviceFromHimetric(&rc, &rcl);
                        _pDoc->Invalidate(&rc, NULL, NULL, 0);
                        IGNORE_HR(UpdatePosRects1());
                    }
                }
                else
                {
                    plfrNext->ScrollIntoView ();
                }
#endif

            }   //if (SUCCEEDED(hr))
        }   // if (plfr && plfr->_hrow)
    }//  if (_pDetail->TestClassFlag(SITEDESC_REPEATER))

Cleanup:

    pTBag->_fNoCursorEvents = fNoCursorEvents;
    RRETURN(hr);
}





#if defined(PRODUCT_97)

//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::WriteProps, public
//
//  Synopsis:   Calls WriteProps to persist the properties for this site.
//
//  Arguments:  [pStm]      -- Stream to write to.
//              [ulObjSize] -- Number of bytes written by associated object.
//
//  Returns:    HRESULT
//
//  Notes:      Can be overridden so that additional var-args can be passed
//              to WriteProps if desired.
//
//              Iff none of the four properties "Database", "LinkChildFields",
//              "LinkMasterFields" and "RowSource" have a non-null value we
//              don't need a CDataLayer (DLAS) in CSite::TBag.  So we get these
//              four using vararg's.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::WriteProps(IStream * pStm, ULONG * pulObjSize)
{
    HRESULT hr;
    CStr    cstrEmpty;
    CBase::CLASSDESC * pclassdesc = BaseDesc();
    CDataLayerAccessorSource *pdlas;

    Assert(_fOwnTBag);

    if (HasDLAccessorSource())
    {
        IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
        Assert(pdlas);
    }
    else
    {
        pdlas = NULL;
    }

    hr = THR(::WriteProps(pStm,
                        0,
                        TRUE,   // Enforce 64K limit
                        pclassdesc->_ppropdesc,
                        pclassdesc->_cpropdesc,
                        this,
                        _fStreamed ? *pulObjSize++ : 0, // First var arg
                        pdlas ? pdlas->GetDatabase()         : cstrEmpty,
                        pdlas ? pdlas->GetLinkChildFields()  : cstrEmpty,
                        pdlas ? pdlas->GetLinkMasterFields() : cstrEmpty,
                        pdlas ? pdlas->GetRowSource()        : cstrEmpty,
                        _pDetail->getTemplate()->TBag()->_ID,
                        _pHeader ? _pHeader->getTemplate()->TBag()->_ID : 0,
                        _pFooter ? _pFooter->getTemplate()->TBag()->_ID : 0 ));


    if (hr)
        goto Cleanup;

    hr = CSite::WriteProps(pStm, pulObjSize);

Cleanup:

    RRETURN(hr);
}
//-+ End of Method-------------------------------------------------------------





//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::ReadProps, public
//
//  Synopsis:   Calls ReadProps to read the properties for this site
//
//  Arguments:  [usPropsVer] -- Version number read from the stream
//              [pb]         -- Pointer to buffer containing props
//              [cb]         -- Count of bytes in [pb]
//              [pulObjSize] -- Place to put loaded object size property
//
//  Returns:    HRESULT
//
//  Notes:      Can be overridden so that additional var-args can be passed
//              to ReadProps if desired.
//
//              See note in WriteProps (above.)
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::ReadProps(USHORT usPropsVer,
                      USHORT usFormVer,
                      BYTE *pb,
                      USHORT cb,
                      ULONG * pulObjSize)
{
    HRESULT hr;
    CStr    cstrDatabase;
    CStr    cstrLinkChildFields;
    CStr    cstrLinkMasterFields;
    CStr    cstrRowSource;
    CBase::CLASSDESC * pclassdesc = BaseDesc();

    Assert(_fOwnTBag);

    //  we will read the _ID of the Detail into the _pDetail pointer
    //  before generation there will be a fix up run
    hr = THR(::ReadProps(usPropsVer,
                         pb,
                         cb,
                         pclassdesc->_ppropdesc,
                         pclassdesc->_cpropdesc,
                         this,
                         pulObjSize,            // var arg 1
                         &cstrDatabase,         // var arg 2
                         &cstrLinkChildFields,  // var arg 3
                         &cstrLinkMasterFields, // var arg 4
                         &cstrRowSource,        // var arg 5
                         &_pDetail,             // var arg 6
                         &_pHeader,             // var arg 7
                         &_pFooter ));          // var arg 8

    if (hr)
    {
        goto Cleanup;
    }

    if (HasDLAccessorSource() ||
        cstrDatabase.Length() > 0 ||
        cstrLinkChildFields.Length() > 0 ||
        cstrLinkMasterFields.Length() > 0 ||
        cstrRowSource.Length() > 0 )
    {
        CDataLayerAccessorSource *pdlas;
        hr = THR(LazyGetDLAccessorSource(&pdlas));
        if (hr)
            goto Cleanup;
        hr = THR(pdlas->SetDatabase(cstrDatabase));
        if (hr)
            goto Cleanup;
        hr = THR(pdlas->SetLinkChildFields(cstrLinkChildFields));
        if (hr)
            goto Cleanup;
        hr = THR(pdlas->SetLinkMasterFields(cstrLinkMasterFields));
        if (hr)
            goto Cleanup;
        hr = THR(pdlas->SetRowSource(cstrRowSource));
    }

Cleanup:
    RRETURN(hr);
}
//-+ End of Method-------------------------------------------------------------

#endif PRODUCT_97


//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame:::UpdatePropertyChanges, public
//
//  Synopsis:   Walks over the dataframe and it's children.
//              if the template is dirty, updates the properies on the instances
//
//  Argument:   BOOL fCompleted: this is a two pass function. First pass updates
//              after all updates, the bags get emptied
//
//  Returns:    HRESULT
//
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::UpdatePropertyChanges(UPDATEPROPS updFlag)
{
    HRESULT hr= S_OK;;

    TraceTag((tagDataFrame, "CDataFrame::UpdatePropertyChanges"));

    CTBag * pTBag = TBag();

    Assert(pTBag);

    switch (updFlag)
    {

        case UpdatePropsEmptyBags:
            pTBag->_propertyChanges.Passivate();
            hr = ForwardPropertyChanges(updFlag);
            break;

        case UpdatePropsPrepareTemplates:
            if (pTBag->_propertyChanges.IsTemplateModified())
            {
                // BUGBUG: currently, we will always regenerate when this happens
                CDataFrame *pRoot = RootFrame(this);
                Assert(pRoot);
                pRoot->TBag()->_propertyChanges.SetRegenerate(TRUE);
                if (pTBag->_propertyChanges.IsDataSourceModified())
                {
                    pRoot->TBag()->_propertyChanges.SetDataSourceModified(TRUE);
                }
                _fIsDirtyRectangle = TRUE;
            }

            if (pTBag->_propertyChanges.NeedCreateToFit())
            {
                CDataFrame *pRoot = RootFrame(this);
                Assert(pRoot);
                pRoot->TBag()->_propertyChanges.SetCreateToFit(TRUE);
                _fIsDirtyRectangle = TRUE;
            }

            if (pTBag->_propertyChanges.IsLinkInfoChanged())
            {
                CDataFrame *pRoot = RootFrame(this);
                Assert(pRoot);
                pRoot->TBag()->_propertyChanges.SetRegenerate(TRUE);
                pRoot->TBag()->_propertyChanges.SetDataSourceModified(TRUE);
                // BUGBUG: need the new API for that one....
                // hr = THR(SetRowsetSource(pTBag->_cstrDatabase, pTBag->_cstrRecordSource));
            }

            hr = ForwardPropertyChanges(updFlag);

            if (pTBag->_propertyChanges.NeedToScroll())
            {
                pTBag->_propertyChanges.SetNeedToInvalidate(ScrollByPropsChange ());
            }

            if (pTBag->_propertyChanges.NeedToInvalidate())
            {
                Invalidate(NULL, 0);
            }
            break;

        case UpdatePropsPrepareDataBase:
#if defined(PRODUCT_97)
            hr = SetRowsetSource(); // BUGBUG: Don't ignore this error...
#endif // defined(PRODUCT_97)
            hr = ForwardPropertyChanges(updFlag);
            break;

        case UpdatePropsCloseAction:
            Assert(_fOwnTBag);
            hr = ForwardPropertyChanges(updFlag);
            _fIsDirtyRectangle = FALSE;
            SetDirtyBelow(FALSE);
            break;

        #ifdef _DEBUG
        default:
            Assert(FALSE && "Wrong value called");
            break;
        #endif
    }
    RRETURN(hr);
}
//-+ End of Method-------------------------------------------------------------




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame:::ForwardPropertyChanges, protected
//
//  Synopsis:   Helpermethod for UpdatePropertyChanges
//
//  Returns:    HRESULT
//
//
//----------------------------------------------------------------------------
HRESULT
CDataFrame::ForwardPropertyChanges(UPDATEPROPS updFlag)
{
    HRESULT hr;
    Assert(_pDetail);

    #if defined(PRODUCT_97)
    CBaseFrame *pD, *pH, *pF;
    #else
    CBaseFrame *pD, *pH;
    #endif


    if (updFlag == UpdatePropsPrepareTemplates)
    {
        pD = _pDetail;
        pH = _pHeader;
        #if defined(PRODUCT_97)
        pF = _pFooter;
        #endif
    }
    else
    {
        pD = _pDetail->getTemplate();
        pH = _pHeader ? _pHeader->getTemplate() : 0;
        #if defined(PRODUCT_97)
        pF = _pFooter ? _pFooter->getTemplate() : 0;
        #endif
    }
    hr = pD->UpdatePropertyChanges(updFlag);
    if (hr)
    {
        goto Cleanup;
    }
    if (pH)
    {
        hr = pH->UpdatePropertyChanges(updFlag);
    }
    if (hr)
    {
        goto Cleanup;
    }
    #if defined(PRODUCT_97)
    if (pF)
    {
        hr = pF->UpdatePropertyChanges(updFlag);
    }
    #endif
Cleanup:
    RRETURN(hr);
}
//-+ End of Method-------------------------------------------------------------


#if defined(PRODUCT_97)
HRESULT
CDataFrame::SetRowsetSource()
{
    HRESULT hr = S_FALSE; // BUGBUG:  Really?  Why not S_OK

    // BUGBUG we should use a 'default' database in this case
    // for now we assume we are related to the parent rowset...

    if (HasDLAccessorSource())
    {
        CDataLayerAccessorSource *pdlas;
        IGNORE_HR(LazyGetDLAccessorSource(&pdlas));
        Assert(pdlas);
        if (!LPTSTR(pdlas->GetDatabase()) &&
            LPTSTR(pdlas->GetRowSource()) &&
            LPTSTR(pdlas->GetLinkMasterFields()) )
        {   // we should not setup any parent-child relationship before
            //  all the information is provided
            // so this could be a parent/child thing
            if (_pParent->TestClassFlag(SITEDESC_BASEFRAME))
            {
                CDataLayerAccessorSource *pdlasParent;
                hr = THR(((CBaseFrame *)_pParent)->getOwner()->
                         LazyGetDLAccessorSource(&pdlasParent) );
                if (!hr)
                {
                    hr = THR(pdlas->SetRowsetSource(pdlasParent));
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    RRETURN1(hr, S_FALSE);
}
#endif // defined(PRODUCT_97)





// Cloning Constructor of CBaseFrame from the CBaseFrameTemplate.
CDataFrameInstance::CDataFrameInstance(
        CDoc * pDoc,
        CSite * pParent,
        CDataFrame * pTemplate)
    : super(pDoc, pParent, pTemplate)
{
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrameInstance destructor
//
//+---------------------------------------------------------------------------

CDataFrameInstance::~CDataFrameInstance()
{
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrameInstance::EnsureIBag
//
//  Synopsis:   Allocate instance bag if not present
//
//  Arguments:
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
#if 0
HRESULT
CDataFrameInstance::EnsureIBag()
{
    HRESULT hr;

    if (!_fOwnIBag)
    {
        CIBag * pIBag = new (_pIBag) CIBag(_pIBag);
        if (!pIBag)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pIBag->Clone(_pIBag);
            if (hr)
            {
                delete pIBag;
            }
            else
            {
                _pIBag = pIBag;
                _fOwnIBag = TRUE;
            }
        }
    }
    else
    {
        hr = S_OK;
    }

    RRETURN(hr);
}
#endif


//+---------------------------------------------------------------------------
//
//  Function:   RootFrame
//
//  Synopsis:   takes a site and travers the tree up to the rootdataframe
//
//  Arguments:  CSite pSite to start from
//
//  Returns:    the rootframe or 0 in error condition
//
//----------------------------------------------------------------------------
CRootDataFrame * RootFrame(CSite *pSite)
{
    Assert(pSite);

    do
    {
        if (pSite->SiteDesc()->_etag == ETAG_ROOTDATAFRAME)
        {
            goto Cleanup;
        }
        pSite = pSite->_pParent;
    } while (pSite);

Cleanup:
    return ((CRootDataFrame*)pSite);
}




//+------------------------------------------------------------------------
//
//  CDataFrameControls implementation
//
//-------------------------------------------------------------------------


//IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(CDataFrameControls, CDataFrameTemplate, _Controls)


/*
STDMETHODIMP
CDataFrameControls::QueryInterface(REFIID iid, LPVOID * ppv)
{
    //  BUGBUG we only have to provide this because it's declared
    //    by the subobject macro; this is a little silly

    return CControls::QueryInterface(iid, ppv);
}


CSite *
CDataFrameControls::ParentSite( )
{
    return MyCDataFrameTemplate();
}
*/



//+---------------------------------------------------------------------------
//    S C R O L L I N G    A P I
//----------------------------------------------------------------------------





//+---------------------------------------------------------------------------
//
//  Member:     IsScrollBar
//
//  Synopsis:   check if there is a need for horizontal or vertical scroll bars
//
//----------------------------------------------------------------------------
BOOL
CDataFrame::IsScrollBar (int iDirection, CRectl *prclDetailView, CRectl *prclDetail)
{
    Assert (iDirection == 0 || iDirection == 1);
    BOOL    f = (1 << iDirection) & TBag()->_iScrollbars;
    int     iOppositeDirection;


    // if the detail is empty, bail out.
    if (prclDetail->IsRectEmpty())
    {
        return FALSE;
    }

    if (f)
    {
        f = prclDetailView->Dimension(iDirection) < prclDetail->Dimension(iDirection);

        /*
        The following code checks if the scrollbar in opposite direction obscures detail view in
        a way that suddenly we need a scrollbar (to compensate the obscured view by that scrollbar).
        In the current spec, if the needed vertical scrollbar obscures the horizontal
        view we do nothing about it, but if the horizontal scrollbar obscures the vertical view
        we have to create the vertical scrollbar.
        */
        if (!f && iDirection == 1)  // iderction == 1 is a check for vertical direction
        {
            iOppositeDirection = 1 - iDirection;    // opposite direction
            if ( ( (1 << iOppositeDirection) & TBag()->_iScrollbars) &&
                 prclDetailView->Dimension(iOppositeDirection) <
                 prclDetail->Dimension(iOppositeDirection))
            {
                // there is a scrollbar in the opposite direction
                // so we have to recalculate "f"
                f = prclDetailView->Dimension(iDirection) - (&g_sizelScrollbar.cx)[iDirection]
                    < prclDetail->Dimension(iDirection);

            }
        }
    }
    return f;
}




//+---------------------------------------------------------------------------
//
//  Member:     GetScrollbarRectl
//
//  Synopsis:   Get scrollbar rectangle in high metrics.
//
//  Arguments:  iDirection      0 or 1 ( HORIZONTAl or VERTICAL)
//              prc             pointer to the scrollbar rectangle (pixels).
//
//  Note:       this function can be optimized by using rcl [e] operator (alexa)
//
//  Retruns:    TRUE if there is a scroll bar in the given direction
//              FALSE oethrwise.
//----------------------------------------------------------------------------

BOOL
CDataFrame::GetScrollbarRectl(int iDirection, CRectl *prcl)
{
    CRectl rclDetailRegion;
    Assert (prcl);

    GetDetailRegion(&rclDetailRegion);

    BOOL f = IsScrollBar(iDirection, &rclDetailRegion, &_pDetail->_rcl);
    if (f)
    {
        *prcl = _rcl;
#ifdef PRODUCT_97
        if (_pCaption)
        {
            (*(CRectl *)prcl)[(Edge)getDirection()] += _pCaption->_rcl.Dimension (getDirection());
        }
#endif
        // for HORIZONTAL/VERTICAL scroll bar calculate the TOP/LEFT

        Edge e = (Edge)(1-iDirection);
        (*((CRectl *)prcl))[e] = _rcl[(Edge)(e+2)] - (&g_sizelScrollbar.cx)[e];

        // encount the space occupied by other scroll bar.

        if (IsScrollBar(1 - iDirection, &rclDetailRegion, &_pDetail->_rcl))
        {
            // for HORIZONTAL/VERTICAL scroll bar adjust RIGHT/BOTTOM

            (*((CRectl *)prcl))[(Edge)(iDirection+2)] -= (&g_sizelScrollbar.cx)[(Edge)iDirection];
        }
   }

    return f;
}




//+---------------------------------------------------------------------------
//
//  Member:     GetScrollbarRect
//
//  Synopsis:   Get the given ScrollBar rectangle in pixels.
//
//  Arguments:  iDirection      0 or 1 ( HORIZONTAl or VERTICAL)
//              prc             pointer to the scrollbar rectangle (pixels).
//
//  Retruns:    TRUE if there is a scroll bar in the given direction
//              FALSE oethrwise.
//
//----------------------------------------------------------------------------

BOOL
CDataFrame::GetScrollbarRect (int iDirection, RECT *prc)
{
    BOOL f;
    f = GetScrollbarRectl (iDirection, (CRectl *)prc);
    if (f)
    {
         Assert (_pDoc);
        _pDoc->DeviceFromHimetric (prc, (RECTL *)prc);
    }
    return f;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetScrollPos
//
//  Synopsis:   get scroll position (ScrollLeft or ScrollTop)
//
//  Arguments:  iDirection      0 - horizontal, 1 - vertical
//
//  Returns:    Returns scroll position
//
//----------------------------------------------------------------------------

long
CDataFrame::GetScrollPos (int iDirection)
{
    Assert (iDirection == 0 || iDirection == 1);
    CRectl rcl;
    GetDetailSectionRectl(&rcl);
    return rcl[(Edge)iDirection] - _pDetail->_rcl[(Edge)iDirection];
}




//+---------------------------------------------------------------------------
//
//  Member:     GetScrollInfo
//
//  Synopsis:   Get the given ScrollBar information.
//
//  Arguments:  iDirection      0 or 1 ( HORIZONTAl or VERTICAL)
//              plPosition      pointer to the top/left position  of the
//                              scrolling region.
//              plVisible       pointer to visible size of scrolling region.
//              plSize          pointer to the total size of scrolling region.
//
//              +----------------------------------+
//              | \                                |
//              |  \                               |
//              |   \                              |
//              |    \                             |
//              |     \_ position                  |
//           t  |        +--------------+-+        |
//           o  |        |              |^|        |
//           t  |        |              | |        |
//           a  |      v |              |V|        |
//           l  |      i |              |S|        |
//              |      s |              |B|        |
//           s  |      u |              | |        |
//           i  |      a |              | |        |
//           z  |      l |              | |        |
//           e  |        |              | |        |
//              |        |              |v|        |
//              |        +--------------+-+        |
//              |        |<  HSB       >|/|        |
//              |        +--------------+-+        |
//              |                                  |
//              |                                  |
//              |                                  |
//              +----------------------------------+
//
//  Retruns:    TRUE if there is a scroll bar in the given direction
//              FALSE oethrwise.
//
//----------------------------------------------------------------------------

BOOL
CDataFrame::GetScrollInfo (int iDirection,
                           long *plPosition,
                           long *plVisible,
                           long *plSize)
{
    BOOL f = IsScrollBar(iDirection);
    if (f)
    {
        CRectl rcl;
        long lTotal = _pDetail->_rcl.Dimension(iDirection);
        GetDetailSectionRectl(&rcl);
        long lVisible = rcl.Dimension (iDirection);
        Assert (lTotal >= lVisible);
        *plPosition = rcl[(Edge)iDirection] - _pDetail->_rcl[(Edge)iDirection];
        *plVisible = lVisible;
        *plSize = lTotal;
    }
    return f;
}


//+---------------------------------------------------------------------------
//
//  Member:     OnScroll
//
//  Synopsis:   ScrollBar events handler
//
//  Arguments:  iDirection      0 or 1 ( HORIZONTAl or VERTICAL) scrollbar
//              uCode           type of scrollbar event.
//              lPosition       position  of the scroll bar Thumb
//
//  Returns:    S_FALSE if scrolling disabled in this direction.
//
//-------------------------------------------------------------------------

HRESULT
CDataFrame::OnScroll (int iDirection, UINT uCode, long lPosition)
{
    Assert (iDirection == 0 || iDirection == 1);

    THREADSTATE *   pts;
    HRESULT         hr = S_OK;
    long            alDelta[2];
    long            lDelta;
    BOOL            fScrollInRepeatedDirection;
    int             iRepeatedDirection = getDirection();
    Edge            eTop = (Edge)iDirection;
    Edge            eBottom = (Edge) (eTop + 2);
    CRectl          rcl;
    long            lTemplateDim;
    long            lRow;
    BOOL            fKeyboardScrolling = FALSE;
    int             i_1 = 1;

    TCHAR       aszText[16];// Text to display in the Tracktip
    POINT       ptScreen;   // Screen coords of mouse position
    TRACKTIPPOS pos;        // Hint as to where tip should be displayed
                            // relative to the mouse position.
                            // TRACKTIPPOS_LEFT, _RIGHT, _BOTTOM, _TOP
    int         cchAvg;     // Number of characters on which to base the
                            // width of the tracktip window.  If this
                            // value is 0, the width of the window will
                            // be based on szText.

    pts = GetThreadState();

    GetDetailSectionRectl (&rcl);

    long        lDim =  rcl.Dimension(eTop);
    long        lDetailDim = _pDetail->_rcl.Dimension(iDirection);
    long        lDetailTop = _pDetail->_rcl[eTop];
    long        lDetailBottom = _pDetail->_rcl[eBottom];
    long        lTop = rcl[eTop];
    long        lBottom = rcl[eBottom];

    BOOL        fSync = IsListBoxStyle(); // BUGBUG for now...

    if (uCode >= KB_SCROLLING)
    {
        uCode -= KB_SCROLLING;
        fKeyboardScrolling = TRUE;
    }

    fScrollInRepeatedDirection = _pDetail->TestClassFlag(SITEDESC_REPEATER) &&
                                 iDirection == getDirection();
    switch (uCode)
    {
        case SB_THUMBTRACK:
            pts->pdfrThumbTracking = this;
            goto CalcDelta;

        case SB_THUMBPOSITION:
            if (pts->pdfrThumbTracking == this)
            {
                FormsHideTooltip(FALSE);
            }
            pts->pdfrThumbTracking = NULL;
CalcDelta:
            if (lPosition == 0)
                lDelta = lDetailTop - lTop;
            else  if (lPosition  > lDetailDim)
                lDelta = lDetailDim - lDim - (lTop - lDetailTop);
            else
                lDelta = lPosition - (lTop - lDetailTop);
            break;

        case SB_LINEUP:
            i_1 = -1;

        case SB_LINEDOWN:
            if (fScrollInRepeatedDirection)
            {
                lDelta = _pDetail->getTemplate()->_rcl.Dimension(iDirection);
            }
            else
            {
                lDelta = g_alHimetricFrom8Pixels[iDirection];
            }
            lDelta *= i_1;
            break;


        case SB_PAGEUP:
            lDelta = -lDim;
            break;

        case SB_PAGEDOWN:
            lDelta = lDim;
            break;


        default:
            goto Cleanup;
    }

    // Check boundary conditions
    if (lDetailBottom - lDelta < lBottom)
    {
        lDelta = lDetailBottom - lBottom;
    }
    if (lDetailTop - lDelta > lTop)
    {
        lDelta = lDetailTop - lTop;
    }

    if (!fScrollInRepeatedDirection)
    {
        // convert lDelta to a nearest pixel
        if (iDirection)
        {
            // vertical direction
            lDelta = VPixFromHimetric (lDelta);
            lDelta = HimetricFromVPix (lDelta);
        }
        else
        {
            // horizontal direction
            lDelta = HPixFromHimetric (lDelta);
            lDelta = HimetricFromHPix (lDelta);
        }
    }

    if (!fSync && uCode == SB_THUMBTRACK && fScrollInRepeatedDirection)
    {
        // show tooltip
        pts->pdfrThumbTracking = this;
        BOOL f = GetScrollbarRectl (iDirection, &rcl);
        Assert (f);
#ifndef _MAC
        rcl[eTop] += (long)(((__int64)lPosition)*rcl.Dimension(iDirection)/lDetailDim);
#else
        rcl[eTop] += (long)((lPosition)*rcl.Dimension(iDirection)/lDetailDim);
#endif
        _pDoc->DeviceFromHimetric (&ptScreen, rcl.TopLeft());
        ClientToScreen (_pDoc->_pInPlace->_hwnd, &ptScreen);
        pos = iDirection? TRACKTIPPOS_LEFT : TRACKTIPPOS_TOP;
        cchAvg = 0;
        lTemplateDim = _pDetail->getTemplate()->_rcl.Dimension (iDirection) +
                       TBag()->_uPadding[iDirection];
        lRow = lPosition/lTemplateDim * (TBag()->_uItems[1 - iDirection]) + 1;
        Assert (lRow > 0);
        Format(0, aszText, ARRAY_SIZE(aszText), _T("<0du>"), lRow);
        FormsShowTracktip(aszText, _pDoc->_pInPlace->_hwnd, ptScreen, pos, cchAvg);
        goto Cleanup;
    }

    if (lDelta)
    {
        alDelta[iDirection] = lDelta;
        alDelta[1 - iDirection] = 0;
        fmScrollAction rgAction[2];
        rgAction[iDirection] = ScrollCodeToAction(uCode);
        rgAction[1 - iDirection] = fmScrollActionNoChange;

        // _fPopulated = TRUE;

        _pDetail->PrepareToScroll (iDirection, rgAction[iDirection],
                                    fKeyboardScrolling, lPosition, alDelta, 0);
        hr = ScrollBy (alDelta[0], alDelta[1], rgAction[0], rgAction[1]);
        if (hr)
            goto Cleanup;

        if (fSync && _pDoc->_pInPlace)
        {
            _pDoc->UpdateForm();
        }


        // BUGBUG (robbear): Move the scroll event firing to ScrollBy
        //  and take advantage of the action parameters.

        // This code is just for now to fire a Scroll event
        // It's really an AfterScroll only for now, untill the core
        // code could support Before/Replace/After model.

        FireDataFrameEvents_Scroll(
                                        rgAction[0],
                                        rgAction[1],
                                        alDelta[0],
                                        alDelta[1],
                                        &alDelta[0],
                                        &alDelta[1]);

        // _fPopulated = FALSE;
    }

Cleanup:
    if (FAILED(hr))
    {
        ConstructErrorInfo(hr, IDS_ACT_DDOCSCROLLGENERAL);
    }
    RRETURN1(hr, S_FALSE);
}





//+---------------------------------------------------------------------------
//
//  Member:     GetFirstRow
//
//  Synopsis:   Get the top row in the rowset
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::GetFirstRow (HROW *pRow)
{
    TraceTag((tagDataFrame, "CDataFrame::GetFirstRow"));
    CDataLayerCursor *pCursor;
    HRESULT hr = THR(LazyGetDataLayerCursor(&pCursor));

    if (SUCCEEDED(hr))
    {
        Assert(pCursor);

        hr = pCursor->GetRowAt(CDataLayerBookmark::ChapterSimple(getGroupID(),
                                    CDataLayerBookmark::TheFirst ),
                               pRow );
    }

    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     GetLastRow
//
//  Synopsis:   Get the last row in the rowset
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::GetLastRow (HROW *pRow)
{
    TraceTag((tagDataFrame, "CDataFrame::GetLastRow"));
    CDataLayerCursor *pCursor;
    HRESULT hr = THR(LazyGetDataLayerCursor(&pCursor));

    if (SUCCEEDED(hr))
    {
        Assert(pCursor);

        hr = pCursor->GetRowAt(CDataLayerBookmark::ChapterSimple(getGroupID(),
                                    CDataLayerBookmark::TheLast ),
                               pRow );
    }

    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     FixupRow
//
//  Synopsis:   Check if hrow is still valid and change it to the next if not
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::FixupRow (CDetailFrame * pfr)
{
    CDataLayerCursor *pCursor;
    ULONG ulFetched;
    HROW hRow = 0;
    CDataLayerBookmark dlb;
    HRESULT hr = S_OK;

    // don't do anything unless we've been notified that rows
    // changed or it is set so we always check
    if (!TBag()->_fRefreshRows || !pfr->getHRow())
        goto Cleanup;

    hr = THR(LazyGetDataLayerCursor(&pCursor));
    if (hr)
        goto Cleanup;

    hr = THR(pCursor->CreateBookmark(getGroupID(), pfr->getHRow(), &dlb));
    if (hr)
    {
        // BUGBUG until the STD is fixed we cannot rely on that
        // we can fetch the bookmark from a potentially 'dead'
        // hrow, so for now we go to the beginning...
        // should be
        // goto Cleanup
        // later
        dlb = CDataLayerBookmark::ChapterSimple(getGroupID(),
                                                CDataLayerBookmark::TheFirst );
    }

    hr = pCursor->GetRowsAt(dlb, 0, 1, &ulFetched, &hRow);
    if (hr)
        goto Cleanup;

    if (!pCursor->IsSameRow(pfr->getHRow(), hRow))
    {
        // fix selection
        if (pfr->_pSelectionElement)
        {
            QUALIFIER q;

            q.type = QUALI_BOOKMARK;
            hr = THR(pCursor->CreateBookmark(getGroupID(), hRow, &q.bookmark));
            if (!hr && TBag()->_eMultiSelect == fmMultiSelectSingle)
            {
                pfr->_pSelectionElement->qStart = q;
            }
            else
            {
                // BUGBUG for now we deselect it
                DeselectSubtree(pfr->_pSelectionElement, NULL);
                // we ignore the error here because we want to fix the row anyway
                hr = S_OK;
            }
        }
        // BUGBUG this will do a lot of work with the selection, optimize it later
        hr = pfr->MoveToRow(hRow);
        if (hr)
        {
            goto Cleanup;
        }
        else
        {
            hRow = 0;   // so we don't release it...
        }
    }

Cleanup:

    if (hRow)
    {
        pCursor->ReleaseRows(1, &hRow);
    }

    RRETURN1(hr, S_FALSE);
}




//+---------------------------------------------------------------------------
//
//  Member:     LazyGetDLAccessorSource
//
//  Synopsis:   Get (and create iff nessacery)
//              Returns non-refcounted DLAS.
//
//  Returns:    Returns S_OK if everything is fine
//              E_OUTOFMEMORY iff appropriate
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::LazyGetDLAccessorSource(CDataLayerAccessorSource **ppDLASOut)
{

    Assert(ppDLASOut);

    HRESULT hr    = S_OK;
    CTBag  *pTBag = TBag();

    if (!(*ppDLASOut = pTBag->_dl.getpDLAS()))
    {
        pTBag->_dl.SetpDLAS(new CDataLayerAccessorSource);
        if (!(*ppDLASOut = pTBag->_dl.getpDLAS()))
        {
            hr = E_OUTOFMEMORY;
        }
    }
    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     LazyGetDataLayerCursor
//
//  Synopsis:   Get (and create iff nessacery)
//              Returns non-refcounted DL Cursor.
//
//  Returns:    Returns S_OK if everything is fine
//              E_OUTOFMEMORY iff appropriate
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::LazyGetDataLayerCursor(CDataLayerCursor **ppDataLayerCursor)
{


    CDataLayerAccessorSource *pdlas;
    HRESULT hr = THR(LazyGetDLAccessorSource(&pdlas));
    if (!hr)
    {
        hr = THR(pdlas->LazyGetDataLayerCursor(ppDataLayerCursor));
    }
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     ScrollBy, public
//
//  Synopsis:   Add the given delta to the form's scroll point and clamp
//              the scroll point to the legal range. If required, scroll
//              the bits on the screen and update the scrollbars.
//
//  Arguments:  dxl     X delta to move in himetric
//              dyl     Y delta to move in himetric
//              xAction Horizontal scroll action
//              yAction Vertical scroll action
//
//  Return:     S_FALSE     in case when delta is 0
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::ScrollBy (
        long dxl,
        long dyl,
        fmScrollAction xAction,
        fmScrollAction yAction)
{
    HRESULT hr = S_OK;
    CRectl rclScrollbar;
    // BUGBUG for now...
    BOOL    fSync = IsListBoxStyle();

    if (dxl || dyl)
    {
        long    alDelta [2] = { dxl, dyl };
        long    l;
        int     iRepeatedDir = getDirection();
        int     iNonRepeatedDir = 1 - iRepeatedDir;
        BOOL    fOthersScroll = FALSE;
        CRectl  rcl;
        RECT    rc;
        int     i;
        Edge    e;

        _pDoc->_fDisableOffScreenPaint = TRUE;
        // Step 1: Adjust delta in the 2 following cases
        //         a. Check for detail frame bottom/right being above the
        //            bottom/right of the detail section of the data frame.
        //         b. Check for top/left of detail frame being below the
        //            top/left of the detail section of the data frame.
        GetDetailSectionRectl(&rcl);
        for (i = 0; i < 2; i++)
        {
            l = alDelta[i];
            if (!l)
                continue;
            if (l > 0)
            {
                e = (Edge)(i+2);    // bottom/right
                if (_pDetail->_rcl[e] < rcl[e] + l)
                {
                    alDelta[i] = _pDetail->_rcl[e] - rcl[e];
                }
            }
            else
            {
                e = (Edge) i;       // top/left
                if (_pDetail->_rcl[e] > rcl[e] + l)
                {
                    alDelta[i] = _pDetail->_rcl[e] - rcl[e];
                }
            }
        }

        if (alDelta[0] == 0 && alDelta[1] == 0)
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        // Step 2: Moving the detail section by delta (move sites)
        _pDetail->MoveSiteBy(-alDelta[0], -alDelta[1]);

        // Step 1b: If there are footer and header move them as well.
        if (alDelta[iNonRepeatedDir])
        {
            l = alDelta[iRepeatedDir];
            alDelta[iRepeatedDir] = 0;
            if (_pHeader)
            {
                _pHeader->MoveSiteBy(-alDelta[0], -alDelta[1]);
                fOthersScroll = TRUE;
            }
            #if defined(PRODUCT_97)
            if (_pFooter)
            {
                _pFooter->MoveSiteBy(-alDelta[0], -alDelta[1]);
                fOthersScroll = TRUE;
            }
            #endif
            alDelta[iRepeatedDir] = l;
        }

        // Step 3: Scroll region.
        if (alDelta[iNonRepeatedDir] && alDelta[iRepeatedDir] == 0)
        {
            // scroll detail section and headers/footers
            GetScrollableRectl (&rcl);
            fOthersScroll = FALSE;
        }
        else
        {
            // scroll detail section
            // GetDetailSectionRectl(&rcl);
        }
        ScrollRegion (rcl, alDelta[0], alDelta[1]);
        if (fOthersScroll)
        {
            GetScrollableRectl (&rcl);
            CRectl rclScroll;
            // if headers and footers were not scrolled yet, do so here.
            if (_pHeader)
            {
                rclScroll.Intersect(&_pHeader->_rcl, &rcl);
                ScrollRegion(rclScroll, alDelta[0], alDelta[1]);
            }
            #if defined(PRODUCT_97)
            if (_pFooter)
            {
                rclScroll.Intersect(&_pFooter->_rcl, &rcl);
                ScrollRegion(rclScroll, alDelta[0], alDelta[1]);
            }
            #endif
        }

        // Step 4: UpdatePosRects
        IGNORE_HR(UpdatePosRects1());

        // Step 5: Update the scrollbars
        for (i = 0; i < 2; i++)
        {
            if (IsScrollBar(i))
            {
                long delta = alDelta[i];
                long lPosNew, lVisible, lTotal;
                if (delta)
                {
                    Verify(GetScrollInfo (i, &lPosNew, &lVisible, &lTotal));
                    if (i)
                    {
                        TBag()->_lScrollTop = lPosNew;
                        OnPropertyChange(DISPID_ScrollTop, 0);
                        OnPropertyChange(DISPID_TopIndex, 0);
                    }
                    else
                    {
                        TBag()->_lScrollLeft = lPosNew;
                        OnPropertyChange(DISPID_ScrollLeft, 0);
                    }
                    // here I need to notify property page.

                    Verify(GetScrollbarRect(i, &rc));
                    GetScrollbarRectl(i, &rclScrollbar);

                    _pDoc->OnViewChange();
                    if (_pDoc->_state >= OS_INPLACE)
                    {
                        UpdateScrollbar(
                            this,
                            i,
                            &rc,
                            rclScrollbar.right - rclScrollbar.left,
                            rclScrollbar.bottom - rclScrollbar.top,
                            lPosNew,
                            lVisible,
                            lTotal,
                            _pDoc,
                            lPosNew - delta,
                            TRUE);
                    }
                }
            }
        }   // end of "for" loop
    }   // end of if (dxl || dyl)
    else
    {
        hr = S_FALSE;       // no scrolling
    }

Cleanup:
    _fPixelScrollingDisable = FALSE;
    RRETURN1 (hr, S_FALSE);
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::ScrollRegion
//
//  Synopsis:   Do content pixel scrolling.
//
//  Arguments:  rcl         Region to scroll
//              dxl         delta to scroll in X direction
//              dyl         delta to scroll in y direction
//
//----------------------------------------------------------------------------

void
CDataFrame::ScrollRegion (const CRectl& rcl, long dxl, long dyl)
{
    _fScrolling = TRUE;

    // check if there is anything really scrolled
    if (_fPixelScrollingDisable ||
        rcl.left + dxl >= rcl.right || rcl.right + dxl <= rcl.left ||
        rcl.top + dyl >= rcl.bottom || rcl.bottom + dyl <= rcl.top)
    {
        // just invalidate
        RECT rc;
        _pDoc->DeviceFromHimetric(&rc, &rcl);
        _pDoc->Invalidate(&rc, NULL, NULL, INVAL_CHILDWINDOWS);
        _pDetail->_fInvalidated = TRUE;
    }
    else
    {
        RECT    rc;         // Rectangle to scroll in pixels
        SIZE    size;       // Size to scroll
        RECTL   rclScroll (rcl);

        if (_fSubtractCurrent)
        {
            Assert(_pDetail->TestClassFlag(SITEDESC_REPEATER));
            Assert(((CRepeaterFrame *)_pDetail)->_pCurrent);
            CRectl rclCurrent(((CRepeaterFrame *)_pDetail)->_pCurrent->_rcl);
            rclCurrent.bottom += dyl;
            rclCurrent.top += dyl;

            // we have to subtract the rectangle of the current
            if (rclCurrent.bottom >= rcl.bottom)
            {
                Assert(rclCurrent.top >= rcl.top && rclCurrent.top <= rcl.bottom);
                rclScroll.bottom = rclCurrent.top;
            }
            else if (rclCurrent.top <= rcl.top)
            {
                Assert(rclCurrent.bottom >= rcl.top && rclCurrent.bottom <= rcl.bottom);
                rclScroll.top = rclCurrent.bottom;
            }
            _fSubtractCurrent = FALSE;
        }

        long    dl[2] = {dxl, dyl};
        _pDoc->DeviceFromHimetric(&rc, &rclScroll);
        _pDoc->DeviceFromHimetric(&size, -dxl, -dyl);
        // Move bits on the screen.
        IGNORE_HR(ScrollRect(&rc, size.cx, size.cy, RDW_INVALIDATE | RDW_ALLCHILDREN));
        if (dl[1- getDirection()])
        {
            // if scrolling in non repeated direction
            InvalidateCurrent();
        }
        _pDetail->_fInvalidated = FALSE;
    }
    _fScrolling = FALSE;
    return;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::DrawBackground
//
//  Synopsis:   Paint the background
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::DrawBackground(CFormDrawInfo *pDI)
{
    HBRUSH  hBrush;
    CRectl  rcl;
    CRectl  rclIntersect;
    CTBag * pTBag = TBag();
    RECT *  prc = &_rc;

    // We paint background if detail not completely covers the section
    //  repeated and there is padding

    GetDetailSectionRectl(&rcl);
    rclIntersect.Intersect(&rcl, &_pDetail->_rcl);
    if (rclIntersect != rcl ||
        (pTBag->_fRepeated && (pTBag->_uPadding[0] > 0) || (pTBag->_uPadding[1] > 0)))
    {
#if DBG == 1
        if (IsTagEnabled(tagDataFrameDebugPaint))
        {
            DWORD color;

            color = RGB(255, 0, 0);    // red
            if (TestClassFlag(SITEDESC_TEMPLATE))
            {
                color ^= 0x00555555;
            }
            hBrush = GetCachedBrush(color);
        }
        else
#endif
            hBrush = GetCachedBrush(ColorRefFromOleColor(_colorBack));

        if (GetCurrentObject(pDI->_hdc, OBJ_BRUSH) != hBrush)
        {
            SelectObject(pDI->_hdc, hBrush);
        }

        PatBlt(pDI->_hdc,
                pDI->_rcClip.left,
                pDI->_rcClip.top,
                pDI->_rcClip.right - pDI->_rcClip.left,
                pDI->_rcClip.bottom - pDI->_rcClip.top,
                PATCOPY);
        ReleaseCachedBrush(hBrush);
    }
#if 0
    else
    {
        hBrush = HBRUSH( GetStockObject( HOLLOW_BRUSH ) );
    }
#endif

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::Paint
//
//  Synopsis:   Paint to the screen.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::Draw(CFormDrawInfo *pDI)
{
    TraceTag((tagDataFrame,"CBaseFrame::Paint"));

    HBRUSH  hBrush;
    int     i;
    RECT    rc;
    RECT    rcT;
    int     iScrollBarVisible = 0;
    RECT *  prcSite = &_rc;

    if (pDI->_fAfterStartSite)
    {
        // Paint scrollbars

        for (i = 0; i < 2; i++)
        {
            long   lPosition, lVisible, lSize;
            CRectl rcl;
            if (GetScrollInfo(i, &lPosition, &lVisible, &lSize))
            {
                if (GetScrollbarRectl(i, &rcl))
                {
                    pDI->WindowFromDocument(&rc, &rcl);
                    if (IntersectRect(&rcT, &rc, &pDI->_rcClip))
                    {
                        DrawScrollbar(
                            this, i, pDI, &rc, rcl.right - rcl.left,
                            rcl.bottom - rcl.top, lPosition, lVisible, lSize, TRUE);
                        iScrollBarVisible += 1 << i;
                    }
                }
            }
        }

        // Paint size box

        if (iScrollBarVisible == fmScrollBarsBoth)
        {
            // Use rectangle calculated for the vertical scrollbar above
            // to calculate the size box rectangle.
            rc.top = rc.bottom;
            rc.right = prcSite->right;
            rc.bottom = prcSite->bottom;

            hBrush = GetCachedBrush(GetSysColorQuick(COLOR_3DFACE));
            if (GetCurrentObject(pDI->_hdc, OBJ_BRUSH) != hBrush)
            {
                SelectObject(pDI->_hdc, hBrush);
            }
            PatBlt(pDI->_hdc, rc.left, rc.top, rc.right - rc.left,
                rc.bottom - rc.top, PATCOPY);
            ReleaseCachedBrush(hBrush);
        }
    }

    RRETURN(THR(super::Draw(pDI)));
}





//+---------------------------------------------------------------------------
//
//  Member:     UpdatePosRects1
//
//  Synopsis:   Loops through all embedded objects and moves detail section
//              according to the current scroll position.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::UpdatePosRects1()
{
    _pDetail->UpdatePosRects1();
    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Method:     CDataFrame::SubScrollbars
//
//  Synopsis:   Subtruct the scrollbars if there are any from the data frame
//              rectangle.
//
//--------------------------------------------------------------------------

void
CDataFrame::SubScrollbars (CRectl *prcl, unsigned fScrollbars)
{
    int     i, iOpposite;
    long    l;

    Assert (fScrollbars <= 3);
    for (i = 0; i < 2; i++)
    {
        if (fScrollbars & (1 << i))
        {
            iOpposite = 1 - i;
            l = (&g_sizelScrollbar.cx)[iOpposite];
            if (prcl->Dimension(iOpposite) >= l)
                (*prcl)[(Edge)(iOpposite + 2)] -= l;
        }
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::GetClientRectl
//
//  Synopsis:   Get content area .
//
//----------------------------------------------------------------------------

void
CDataFrame::GetClientRectl(RECTL *prcl)
{
    Assert (prcl);
    *prcl = _rcl;
    SubScrollbars((CRectl *)prcl, IsScrollBar(0) + (IsScrollBar(1) << 1));
    return;
}



//+---------------------------------------------------------------------------
//
//  Member:     GetDetailRegion
//
//  Synopsis:   get the detail section of the dataframe (not counting scrollbars)
//
//  Arguments:  prclDetail   pointer to the detail rcl
//              fProposed    if TRUE, return proposed rect
//
//----------------------------------------------------------------------------

void
CDataFrame::GetDetailRegion (CRectl *prclDetail)
{
    Edge    eTop = (Edge)getDirection();
    Edge    eBottom = (Edge)(eTop + 2);

    *prclDetail = _rcl;
    if (_pHeader)
    {
        (*(CRectl *)prclDetail)[eTop] = _pHeader->_rcl[eBottom];
    }
#if defined(PRODUCT_97)
    else if (_pCaption)
    {
        (*(CRectl *)prclDetail)[eTop] = _pCaption->_rcl[eBottom];
    }

    if (_pFooter)
    {
        (*(CRectl *)prclDetail)[eBottom] = _pFooter->_rcl[eTop];
    }
#endif
    if ((*(CRectl *)prclDetail)[eTop] > (*(CRectl *)prclDetail)[eBottom])
    {
        // this condition could happened during deffered property update, when we have a new header,
        // wich was not encounted yet by dataframe.
        (*(CRectl *)prclDetail)[eBottom] = (*(CRectl *)prclDetail)[eTop];
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     GetDetailSectionRectl
//
//  Synopsis:   get the detail section rcl.
//
//  Arguments:  prclDetail   pointer to the detail rcl
//
//----------------------------------------------------------------------------

void
CDataFrame::GetDetailSectionRectl (CRectl *prcl)
{
    int     i;
    long    lShrink;
    Edge    e;

    GetDetailRegion(prcl);
    CRectl  rcl (*prcl);        // Note: when we will call IsScrollBar function
                                // it is important to pass the same rectangle (&rcl)
    for (i = 0; i < 2; i++)
    {
        if (IsScrollBar (i, &rcl, &_pDetail->_rcl))
        {
            lShrink = (&g_sizelScrollbar.cx)[1-i];
            if (i == 0)
            {
                #if defined(PRODUCT_97)
                // horizontal scroll bar
                if (_pFooter)
                {
                    // if there is a footer we should not subtruct the size
                    // of the horizontal scroll bar from the detail region.
                    lShrink -=_pFooter->_rcl.Height();
                    if (lShrink < 0)
                        lShrink = 0;
                }
                #endif
            }
            e = (Edge)(1 - i + 2);
            if ((*((CRectl *)prcl))[e] > lShrink)
            {
                (*((CRectl *)prcl))[e] -= lShrink;
            }
        }
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     GetScrollableRectl
//
//  Synopsis:   get the detail section rcl.
//
//  Arguments:  prclDetail   pointer to the detail rcl
//
//----------------------------------------------------------------------------

void
CDataFrame::GetScrollableRectl (CRectl *prcl)
{
    *prcl = _rcl;
#ifdef PRODUCT_97
    if (_pCaption)
    {
        // Note: this code assumes the caption is always on top
        // (never on the left or anything else which is wird).
        prcl->top += _pCaption->_rcl.Height();
    }
#endif
    SubScrollbars ((CRectl *)prcl, IsScrollBar(0) + (IsScrollBar(1) << 1));
}




//+---------------------------------------------------------------------------
//
//  Member:     HitTestPointl, public
//
//  Synopsis:   Does a hit test against our object, determining where on the
//              object it hit.
//
//  Arguments:  [ptl] -- point to hit test in document coordinates
//              [prclClip]  -- optional clipping rectangle to check inside
//              [ppSite]    -- return the site hit
//              [pMessage]  -- Ptr to a message struct.  Can be NULL
//              [dwFlags]   -- HT_ flags
//
//  Returns:    HTC
//
//----------------------------------------------------------------------------

HTC
CDataFrame::HitTestPointl(
    POINTL ptl,
    RECTL * prclClip,
    CSite ** ppSite,
    CMessage *pMessage,
    DWORD dwFlags)
{
    HTC     htc = HTC_NO;
    int     i;
    int     iScrollBarVisible = 0;
    CRectl  rcl;
    CSite * pSite = NULL;

// BUGBUG (garybu) no more hittest pointl
//    htc = super::HitTestPointl (ptl, prclClip, &pSite, NULL, dwFlags);
//    if (htc == HTC_YES)
    {
        // Check if hit scroll bar

        for (i = 0; i < 2; i++)
        {
            if (GetScrollbarRectl(i, &rcl))
            {
                if (PtlInRectl(&rcl, ptl))
                {
                    htc = HTC_NONCLIENT;
                    goto Cleanup;
                }
                iScrollBarVisible += (1 << i);
            }
        }

        // Check if hit the size box

        if (iScrollBarVisible == fmScrollBarsBoth)
        {
            // Use rectangle calculated for the vertical scrollbar above
            // to calculate the size box rectangle.
            rcl.top = rcl.bottom;
            rcl.bottom = _rcl.bottom;
            rcl.right = _rcl.right;

            if (PtlInRectl(&rcl, ptl))
            {
                htc = HTC_BOTTOMRIGHTHANDLE;
                goto Cleanup;
            }
        }

        // if repeater is hit return dataframe
        if (pSite == _pDetail && _pDetail->TestClassFlag(SITEDESC_REPEATER))
        {
            pSite = this;
        }
    }

Cleanup:
    if (ppSite)
    {
        *ppSite = pSite;
    }
    if (pMessage)
    {
        pMessage->pSiteHit = pSite;
        pMessage->htc = htc;
    }
    return htc;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::ScrollByPropsChange
//
//  Synopsis:   scroll accordingly to the property changes
//
//  Arguments:  None
//
//  Returns:    Returns TRUE if the data frame still needs to be Invalidated
//              (Invalidate was not called).
//
//----------------------------------------------------------------------------

BOOL
CDataFrame::ScrollByPropsChange()
{
    CTBag   *   pTBag = TBag();
    int         iDirection = getDirection ();
    int         i;
    BOOL        f;
    long        lOldPosition, lNewPosition;
    BOOL        fInvalidate = TRUE;
    HRESULT     hr;

    for (i = 0; i < 2; i++ )
    {
        iDirection = 1 - iDirection;    // first scroll in the non repeated
                                        // direction, then in repeated direction
        f = IsScrollBar(iDirection);
        if (f)
        {
            lOldPosition = GetScrollPos (iDirection);
            lNewPosition = (&pTBag->_lScrollLeft)[iDirection];
            if (lNewPosition != lOldPosition)
            {
                hr = OnScroll (iDirection, SB_THUMBPOSITION, lNewPosition);
                if (!hr)
                    fInvalidate = FALSE;
            }
        }
    }

    return fInvalidate;
}


//+---------------------------------------------------------------------------
//
//  Member:     GetSnakingType
//
//  Synopsis:   Get SnakingType.
//
//  Arguments:  pSnakingDirection           pointer to a result of
//                                          snaking direction.
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::GetSnakingType (unsigned int *pSnakingDirection)
{
    TraceTag((tagDataFrame, "CDataFrame::DF_GetSnakingType"));

    if (!pSnakingDirection)
    {
        RRETURN(SetErrorInfoInvalidArg());
    }

    CTBag *pTBag = TBag ();
    BOOL   fDirection = pTBag->_fDirection;
    int    iSnaking =  fDirection;
    // if fDirection is DOWN, then the result is either DOWN or DOWNACROSS
    // if fDirection is ACROSS, then the result is either ACROSS or ACROSSDOWN

    if (pTBag->_uItems[!fDirection] != 1)
    {
        iSnaking += 2;
    }
    *pSnakingDirection = iSnaking;

    RRETURN (S_OK);
}






//+---------------------------------------------------------------------------
//
//  Member:     SetSnakingType
//
//  Synopsis:   Set SnakingType.
//
//  Arguments:  direction               repeatition
//              uItemsAcross            items in Across direction.
//              uItemsDown              items in Down direction.
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT CDataFrame::SetSnakingType (unsigned int direction, unsigned int uItemsAcross, unsigned int uItemsDown)
{
    TraceTag((tagDataFrame, "CDataFrame::DF_SetSnakingType"));

    HRESULT hr = S_OK;
    TBag()->_fDirection = direction;
    TBag()->_uItems [ACROSS] = uItemsAcross;
    TBag()->_uItems [DOWN] = uItemsDown;
    RRETURN (hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::RetireRecordSelector
//
//  Synopsis:   Retires the active record selector control
//
//  Arguments:  None
//
//  Returns:    Returns TRUE if the data frame still needs to be Invalidated
//              (Invalidate was not called).
//
//  Note:       It is also used to prepare the dataframe for a new
//              user-installed record selector
//
//----------------------------------------------------------------------------

void
CDataFrame::RetireRecordSelector(void)
{
    CTBag * pTBag = TBag();
    COleDataSite * pRecordSelector;
    BOOL fDeferedPropertyUpdate = _pDoc->_fDeferedPropertyUpdate;

    if ( pTBag->_pctrlRecordSelector )
    {
        _pDoc->_fDeferedPropertyUpdate = TRUE;

        Assert(pTBag->_pctrlRecordSelector->TestClassFlag(SITEDESC_OLEDATASITE));
        pRecordSelector = pTBag->_pctrlRecordSelector;
        pTBag->_pctrlRecordSelector = NULL;
        pRecordSelector->_fRecordSelector = FALSE;

        pRecordSelector->Release();
        _pDoc->_fDeferedPropertyUpdate = fDeferedPropertyUpdate;
    }
#if DBG == 1
    else
    {
        int c;
        CSite ** ppSite;
        for ( ppSite = _arySites, c = _arySites.Size(); --c >= 0; ppSite++ )
        {
            Assert( ! ((COleDataSite*)(*ppSite))->_fRecordSelector );
        }
    }
#endif
}



#if DBG==1

void  CheckProposedFlag(CDoc * pDoc)
{
    pDoc->_RootSite.DoCheckProposed();
}


#endif



#if defined(PRODUCT_97)
//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::UpdateObjectRects
//
//  Synopsis:   Tells all sites about new positions and clipping.
//
//  Arguments:  rcClip  The clipping rectangle.  If null, compute
//                      appropriate clipping rectange for this site.
//
//----------------------------------------------------------------------------

void
CDataFrame::UpdateObjectRects(RECT *prcClip)
{
    if ( TBag()->_eListBoxStyle != fmListBoxStylesNone )
    {
        if (_fScrolling)
        {
            super::UpdateObjectRects(prcClip);
        }
        else
        {
            CRectl  rclView;

            _pDoc->HimetricFromDevice(&rclView, prcClip);
            CreateToFit (&rclView, SITEMOVE_POPULATEDIR | SITEMOVE_NOINVALIDATE |
                                  SITEMOVE_INVALIDATENEW);
            // AllignTemplatesToPixels could off set dirty rectangle flag, we need to clear it.
            _pDetail->getTemplate()->_fIsDirtyRectangle = FALSE;
        }
    }
}
#endif




//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::CDLSink::CreateToFit
//
//  Synopsis:   Regenerate rows in dataframe and resize if necessary
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::CDLSink::CreateToFit()
{
    CRectl rclBound;
    HRESULT hr;

    CDataFrameTemplate * pdfr = MyDataFrame();
    if (pdfr->_TBag._fNoCursorEvents)
    {
        hr = S_OK;
    }
    else
    {
        BOOL fRefreshRows = pdfr->_TBag._fRefreshRows;
        pdfr->_TBag._fRefreshRows = TRUE;
        pdfr->_pDoc->_RootSite.GetClientRectl(&rclBound);
        hr = pdfr->CreateToFit(&rclBound, SITEMOVE_POPULATEDIR);
        pdfr->_TBag._fRefreshRows = fRefreshRows;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::CDLSink::AllChanged
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::CDLSink::AllChanged()
{
    // BUGBUG: for now always regenerate the whole dataframe
    RRETURN(CreateToFit());
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::CDLSink::RowsChanged
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::CDLSink::RowsChanged(ULONG cRows, const HROW *ahRows)
{
    // BUGBUG: for now always refresh the whole dataframe
    HRESULT hr;

    CDataFrameTemplate * pdfr = MyDataFrame();
    if (pdfr->_TBag._fNoCursorEvents)
    {
        hr = S_OK;
    }
    else
    {
        hr = pdfr->_pDetail->RefreshData();
        if (!hr)
        {
            pdfr->Invalidate(NULL, 0);
        }
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::CDLSink::FieldChanged
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::CDLSink::FieldChanged(HROW hRow, ULONG iColumn)
{
    // BUGBUG: for now always refresh the whole dataframe
    HRESULT hr;

    CDataFrameTemplate * pdfr = MyDataFrame();
    if (pdfr->_TBag._fNoCursorEvents)
    {
        hr = S_OK;
    }
    else
    {
        hr = pdfr->_pDetail->RefreshData();
        if (!hr)
        {
            pdfr->Invalidate(NULL, 0);
        }
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::CDLSink::RowsInserted
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::CDLSink::RowsInserted(ULONG cRows, const HROW *ahRows)
{
    // BUGBUG: for now always regenerate the whole dataframe
    RRETURN(CreateToFit());
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::CDLSink::DeletingRows
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::CDLSink::DeletingRows(ULONG cRows, const HROW *ahrows)
{
    // BUGBUG: for now always regenerate the whole dataframe
    CDataFrameTemplate * pdfr = MyDataFrame();
    CBaseFrame * pDetail = pdfr->getDetail();
    pDetail->DeletingRows(cRows, ahrows);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::CDLSink::RowsDeleted
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::CDLSink::RowsDeleted(ULONG , const HROW *)
{
    RRETURN(CreateToFit());
}


#if defined(VIADUCT)
//+---------------------------------------------------------------------------
//
//  Member:     CDataFrame::CDLSink::OnNileError
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

HRESULT
CDataFrame::CDLSink::OnNileError(HRESULT hr, BOOL)
{
    RRETURN1(MyDataFrame()->_pDoc->ShowLastErrorInfo(hr), S_FALSE);
}
#endif


