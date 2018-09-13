//----------------------------------------------------------------------------
//
//  Maintained by: frankman
//
//  Microsoft DataDocs
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       src\doc\datadoc\headfrm.cxx
//
//  Contents:   Implementation of the header/footer frames.
//
//  Classes:    CHeaderFrame
//
//  Functions:  None.
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#include "dfrm.hxx"

DeclareTag(tagHeaderFrame,"src\\ddoc\\datadoc\\headfrm.cxx","HeaderFrame");
DeclareTag(tagHeaderFrameInstance,"src\\ddoc\\datadoc\\headfrm.cxx","HeaderFrameInstance");
DeclareTag(tagHeaderFrameTemplate,"src\\ddoc\\datadoc\\headfrm.cxx","HeaderFrameTemplate");

const unsigned long k_idHeader = 1;

#if PRODUCT_97

PROP_DESC CHeaderFrameTemplate::s_apropdesc[] =
{
    PROP_MEMBER(CSTRING,
                CHeaderFrameTemplate,
                _TBag._cstrName,
                NULL,
                "Name",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(CSTRING,
                CHeaderFrameTemplate,
                _IBag._cstrTag,
                NULL,
                "Tag",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(LONG,
                CHeaderFrameTemplate,
                _TBag._ID,
                0,
                "ID",
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(LONG,
                CHeaderFrameTemplate,
                _dwHelpContextID,
                0,
                "HelpContextID",
                PROP_DESC_BYTESWAPLONG)
//    PROP_MEMBER(LONG,
//                CHeaderFrameTemplate,
//                _ulBitFlags,
//                SITE_FLAG_DEFAULTVALUE,
//                "BitFlags",
//                PROP_DESC_BYTESWAPLONG)
    PROP_VARARG(LONG,
                sizeof(DWORD),
                0,
                "ObjectSize",
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(SHORT,
                CHeaderFrameTemplate,
                _TBag._usTabIndex,
                (ULONG)-1L,
                "TabIndex",
                PROP_DESC_BYTESWAPSHORT)
    PROP_MEMBER(USERDEFINED,
                CHeaderFrameTemplate,
                _rcl,
                NULL,
                "SizeAndPosition",
                PROP_DESC_BYTESWAPRECTL)

    // the above stuff is copied from the CSite implementation
    // now we first save the unnamed struct, this includes all bitfields

    PROP_MEMBER(LONG,
                CHeaderFrameTemplate,
                _IBag._colorBack,
                NULL,
                "BackColor",
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(LONG,
                CHeaderFrameTemplate,
                _IBag._colorFore,
                NULL,
                "ForeColor",
                PROP_DESC_BYTESWAPLONG)
    PROP_NOPERSIST(LONG, sizeof(long), NULL)   // Old WhatsThisHelpID
    PROP_MEMBER(CSTRING,
                CHeaderFrameTemplate,
                _TBag._cstrControlTipText,
                NULL,
                "ControlTipText",
                PROP_DESC_NOBYTESWAP)
    PROP_NOPERSIST(CSTRING, sizeof(CStr), NULL)   // Old StatusBarText
};

#endif PRODUCT_97


CSite::CLASSDESC CHeaderFrameTemplate::s_classdesc =
{
    {
        {
            &CLSID_CDataFrame,              // _pclsid
            IDR_BASE_FRAME,                 // _idrBase
#if PRODUCT_97
            s_aclsidPages,                  // _apClsidPages
#else
            NULL,
#endif
            ARRAY_SIZE(CSite::s_acpi),      // _ccp
            CSite::s_acpi,                  // _pcpi
#if PRODUCT_97
            ARRAY_SIZE(s_apropdesc),        // _cpropdesc
            s_apropdesc,                    // _ppropdesc
#else
            0,
            0,
#endif
            SITEDESC_PARENT |              // _dwFlags
            SITEDESC_BASEFRAME |
            SITEDESC_TEMPLATE |
            SITEDESC_DETAILFRAME |
            SITEDESC_HEADERFRAME,
            &IID_IDataFrame,                // _piidDispinterface
            0,                              // _lAccRole
            &CSite::s_PropCats,             // _pPropCats;
            NULL
        },
        NULL,                               //_pfnTearoff
    },
    ETAG_HEADERFRAME,                      // _st
};

CSite::CLASSDESC CHeaderFrameInstance::s_classdesc =
{
    {
        {
            &CLSID_CDataFrame,              // _pclsid
            IDR_BASE_FRAME,                 // _idrBase
#if PRODUCT_97
            s_aclsidPages,                  // _apClsidPages
#else
            NULL,
#endif
            ARRAY_SIZE(CSite::s_acpi),      // _ccp
            CSite::s_acpi,                  // _pcpi
            0,                              // _cpropdesc
            NULL,                           // _ppropdesc
            SITEDESC_PARENT |               // _dwflags
            SITEDESC_BASEFRAME |
            SITEDESC_DETAILFRAME |
            SITEDESC_HEADERFRAME,
            &IID_IDataFrame,               // _piidDispinterface
            0,                              // _lAccRole
            &CSite::s_PropCats,             // _pPropCats;
            NULL
        },
        NULL,                               //_pfnTearoff
    },
    ETAG_HEADERFRAME,                     // _st
};




//----------------------------------------------------------------------------
//  Member:     CreateInstance
//
//  Synopsis:   Create an instance of the CHeaderFrame class.
//
//  Arguments:  pForm               the form
//              ppfrFrame           returns the generated frame
//              pParent             parent of the new guy
//              info                info structure with binding information
//                                  this parameter is ignored for now
//
//
//  Returns:    Returns S_OK if everything is fine,
//              E_INVALIDARG if ppFrame is NULL.
//
//----------------------------------------------------------------------------
HRESULT
CHeaderFrame::CreateInstance(
        CDoc * pDoc,
        CSite * pParent,
        CSite **ppFrame,
        CCreateInfo * pcinfo)
{
    TraceTag((tagHeaderFrame, "CHeaderFrame::CreateInstance"));


    HRESULT hr;

    Assert (ppFrame);
    Assert(_fOwnTBag);      // should only be called on the template

    CHeaderFrame * pHeaderFrame;

    pHeaderFrame = new CHeaderFrameInstance(pDoc, pParent, this);
    if (pHeaderFrame)
    {
        hr = pHeaderFrame->InitInstance();
        if (hr)
        {
            goto Error;
        }
        hr =  BuildInstance (pHeaderFrame);
    }
    else
        goto MemoryError;

Cleanup:
    *ppFrame = pHeaderFrame;
    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

Error:
    pHeaderFrame->Release();
    pHeaderFrame = NULL;
    goto Cleanup;
}





//+---------------------------------------------------------------------------
//
//  Member      CHeaderFrame::constructor
//
//  Synopsis    Instance Constructor
//
//  Arguments   pDataDoc        pointer to a DataDoc
//              pParent         pointer to a parent object (instance)
//              pTemplate       pointer to a header frame template.
//
//----------------------------------------------------------------------------

CHeaderFrame::CHeaderFrame(
        CDoc * pDoc,
        CSite * pParent,
        CHeaderFrame * pTemplate)
    : super(pDoc, pParent, pTemplate)
{
    _fAutoSize = FALSE;
}
//--end of method--------------------------------------------------------------




//+---------------------------------------------------------------------------
//
//  Member      CHeaderFrame::operator new.
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
CHeaderFrame::operator new (size_t s, CHeaderFrame * pOriginal)
{
    TraceTag((tagHeaderFrame, "CDetailFrame::operator new "));
    void * pNew = ::operator new(s);
    if (pNew)
        memcpy(pNew, pOriginal, sizeof(CHeaderFrame));
    return pNew;
}
//--end of method--------------------------------------------------------------


//+---------------------------------------------------------------
//
//  Member:     CDetailFrame::Notify
//
//  Synopsis:   Handle notification
//
//---------------------------------------------------------------
HRESULT
CHeaderFrame::Notify(SITE_NOTIFICATION sn, DWORD dw)
{
    HRESULT hr = S_OK;

    switch (sn)
    {
    case SN_AFTERLOAD:
#if defined(PRODUCT_97)
        if (_fIsFooter)
            ((CDataFrame *)_pParent)->_pFooter = this;
        else
#endif
            ((CDataFrame *)_pParent)->_pHeader = this;
        hr = THR(super::Notify(sn, dw));
        break;

    default:
        hr = THR(super::Notify(sn, dw));
    }

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CHeaderFrame::GetQualifier
//
//  Synopsis:   Get the qualifier of the header
//
//  Arguments:  pq      pointer to a QUALIFIER.
//
//  Returns:    Returns S_OK if everything is fine.
//              E_INVALIDARG if pCurrency is NULL.
//
//----------------------------------------------------------------------------
HRESULT CHeaderFrame::GetQualifier (QUALIFIER * pq)
{
    TraceTag((tagHeaderFrame, "CHeaderFrame::GetQualifier"));

    if ( ! pq )
        return E_INVALIDARG;

    if (getOwner()->_pHeader == this)
    {
        pq->type = QUALI_HEADER;
    }
    else
    {
        pq->type = QUALI_FOOTER;
    }

    pq->id   = k_idHeader;

    RRETURN(S_OK);

}
//--end of method--------------------------------------------------------------







//+---------------------------------------------------------------------------
//
//  Member:     CHeaderFrame::IsVisible
//
//  Synopsis:   Is this site visible?
//
//  Returns:    BOOL
//
//----------------------------------------------------------------------------
BOOL
CHeaderFrame::IsVisible( )
{
    #if defined(PRODUCT_97)
    if (_pParent->IsVisible())
    {

        return (_fOwnTBag ? ((getOwner()->_pHeader == this) ||
                (getOwner()->_pFooter==this)) :
                _fVisible);


    }
    return FALSE;
    #else
    return TRUE;
    #endif
}
//--end of method--------------------------------------------------------------




#if defined(PRODUCT_97)
//+-------------------------------------------------------------------------
//
//  Method:     CHeaderFrame::ProposedFriendFrames(void)
//
//  Synopsis:   Calculate the proposed positions for associated frames
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CHeaderFrame::ProposedFriendFrames(void)
{
    Assert(_fProposedSet);

    TraceTag((tagHeaderFrame, "CDetailFrame::ProposedFriendFrames"));

    BOOL    fIsVertical = getOwner()->IsVertical();
    CRectl  rclProposed;
    CRectl  rclDelta;

    CBaseFrame *pRelatedOne, *pRelatedTwo;

    pRelatedOne = getOwner()->_pDetail;
    pRelatedTwo = getTemplate()->_fIsFooter ? getOwner()->_pHeader : getOwner()->_pFooter;

    GetProposed(this, &rclProposed);

    rclDelta = rclProposed;
    rclDelta -= _rcl;

    Edge e = (Edge) ( getOwner()->IsVertical());
    Edge eOther = (Edge) (e+2);
    if (!getTemplate()->_fIsFooter && rclDelta[eOther] != 0)
    {
        // header, wants to push detail down
        rclDelta[e] += rclDelta[eOther];
        rclDelta[eOther] = rclDelta[e];
    }
    else
    {
        // footer
        rclDelta[eOther]      = rclDelta[e];
        rclDelta[e] = 0;
    }


    pRelatedOne->ProposedDelta(&rclDelta);


    // movements in repeat direction are not of intrest for the other guy
    rclDelta[e]      = 0;
    rclDelta[eOther] = 0;

    if (pRelatedTwo)
    {
        pRelatedTwo->ProposedDelta(&rclDelta);
    }

    return S_OK;
}
#endif PRODUCT_97


#if PRODUCT_97
//+-------------------------------------------------------------------------
//
//  Method:     CHeaderFrame::CalcControlPositions
//
//  Synopsis:   Calculate the proposed positions for the children sites
//              by getting the values of the related sites in the detail
//              section
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CHeaderFrame::CalcControlPositions(DWORD dwFlags)
{
    TraceTag((tagHeaderFrame, "CDetailFrame::CalcControlPositions"));

    HRESULT hr = S_OK;
    CSite **ppSite, **ppSiteEnd;
    CDetailFrame *pDetail = (CDetailFrame*)getOwner()->getDetail()->getTemplate();
    CMatrix      *pMatrix = pDetail->TBag()->_pMatrix;

    if (pMatrix && pMatrix->getOuter() == pDetail)
    {
        // so we are working on the correct level

        // first set the related pointer blank
        for (ppSite = pDetail->_arySites, ppSiteEnd = ppSite + pDetail->_arySites.Size();
                 ppSite < ppSiteEnd; ppSite++)
        {
            ((COleDataSite*)(*ppSite))->TBag()->_pRelated = 0;
        }

        // now set up the header pointers

        for (ppSite = _arySites, ppSiteEnd = ppSite + _arySites.Size();
                 ppSite < ppSiteEnd; ppSite++)
        {
            Assert(((COleDataSite*)*ppSite)->TBag()->_pRelated);

            ((COleDataSite*)(*ppSite))->TBag()->_pRelated->TBag()->_pRelated = (COleDataSite*)(*ppSite);
        }

        // let the matrix do the rest
        hr = pMatrix->MoveRelatedCells(this, IsVertical());

        // now set them blank again
        for (ppSite = pDetail->_arySites, ppSiteEnd = ppSite + pDetail->_arySites.Size();
                 ppSite < ppSiteEnd; ppSite++)
        {
            ((COleDataSite*)(*ppSite))->TBag()->_pRelated = 0;
        }


    }
    else
    {
        hr = super::CalcControlPositions(dwFlags);
    }

    RRETURN(hr);


}
//--end of method--------------------------------------------------------------

#endif




//+---------------------------------------------------------------------------
//
//  Member:     CHeaderFrame::RemoveRelated
//
//  Synopsis:   get's called when a detail cell was removed
//              always called on template level
//
//  Arguments:  pSite       Detailsite that gets removed
//              dwFlags     DeleteSite flags
//
//  Returns:    HRESULT
//
//---------------------------------------------------------------
HRESULT CHeaderFrame::RemoveRelated(CSite *pSite, DWORD dwFlags)
{

    TraceTag((tagHeaderFrame, "CHeaderFrame::RemoveRelated"));
    HRESULT hr = S_OK;

    Assert(_fOwnTBag);

    CSite **ppSite, **ppSiteEnd;

    // now set them blank again
    for (ppSite = _arySites, ppSiteEnd = ppSite + _arySites.Size();
             ppSite < ppSiteEnd; ppSite++)
    {
        if (((COleDataSite*)(*ppSite))->TBag()->_pRelated == pSite)
        {
            dwFlags |= DELSITE_NOTABADJUST;
            hr = DeleteSite(*ppSite, dwFlags);
            break;
        }
    }


    RRETURN(hr);

}
//-end-of-method-------------------------------------------------




//+---------------------------------------------------------------------------
//
//  Member:     DeleteSite
//
//  Synopsis:   just adds  the NO tab adjust flag
//
//  Returns:    Returns result from parent
//
//----------------------------------------------------------------------------

HRESULT
CHeaderFrame::DeleteSite(CSite * pSite, DWORD dwFlags)
{
    RRETURN(super::DeleteSite(pSite, dwFlags | DELSITE_NOTABADJUST));
}
//-end-of-method-------------------------------------------------





//+---------------------------------------------------------------------------
//
//  Member      CHeaderFrame::ProposedDelta
//
//  Synopsis    this method gets called by associated frames when they were
//              moved.
//
//  Arguments   CRectl *rclDelta    the rectangle shows the diffs between
//                                  the original and the proposed rectangle
//                                  information.
//
//  Returns     HRESULT
//
//
//----------------------------------------------------------------------------
HRESULT
CHeaderFrame::ProposedDelta(CRectl *prclDelta)
{
    // we want to get resized
    super::ProposedDelta(prclDelta);

    CRectl rcl = _rcl;

    Edge e = (Edge) (1 - getOwner()->IsVertical());
    Edge eOther = (e == edgeLeft ? edgeTop : edgeLeft);
    Edge eOpposite = (Edge) (e+2);
    Edge eOtherOp = (Edge) (eOther+2);

    rcl[eOpposite] += (*prclDelta)[eOpposite];
    rcl[e] += (*prclDelta)[e];

    if (getTemplate()->_fIsFooter)
    {
        rcl[eOther]     +=  (*prclDelta)[eOtherOp];
        rcl[eOtherOp]   +=  (*prclDelta)[eOtherOp];
    }
    else
    {
        rcl[eOther]     +=  (*prclDelta)[eOther];
        rcl[eOtherOp]   +=  (*prclDelta)[eOther];
    }

    SetProposed(this, &rcl);

    return S_OK;
}
//--end of method--------------------------------------------------------------



///////////////////////////////////////////////////////////////////////////////
//
//  HeaderFrameInstance implemenation section
//
///////////////////////////////////////////////////////////////////////////////


// Cloning Constructor of CBaseFrame from the CBaseFrameTemplate.
CHeaderFrameInstance::CHeaderFrameInstance(
        CDoc * pDoc,
        CSite * pParent,
        CHeaderFrame * pTemplate)
    : super(pDoc, pParent, pTemplate)
{
}
//--end of method--------------------------------------------------------------



//+---------------------------------------------------------------------------
//
//  Member:     CHeaderFrameInstance::EnsureIBag
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
CHeaderFrameInstance::EnsureIBag()
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
//--end of method--------------------------------------------------------------






//+---------------------------------------------------------------------------
//
//  Member:     CHeaderFrame::GetClientRectl (virtual)
//
//  Synopsis:   Get content area.
//
//----------------------------------------------------------------------------
void
CHeaderFrame::GetClientRectl (RECTL *prcl)
{
    Assert (prcl);
    *prcl =  _rcl;  // note there is no need to call _pParent GetClientRectl
                    // and then intersect it with _rcl. That will happenned
                    // automaitvaly during ParentSite::Paint clip region
                    // will shrink on the way to paint children.
    return;
}
//---- end of method ------------------------------------------------------------






//+---------------------------------------------------------------------------
//
//  Member:     CHeaderFrame::Draw
//
//  Synopsis:   overwritten to draw in list/combobox cases
//
//----------------------------------------------------------------------------
HRESULT CHeaderFrame::Draw(CFormDrawInfo *pDI)
{
    TraceTag((tagHeaderFrameTemplate,"CHeaderFrameTemplate::Draw "));

    HRESULT hr;
    CRectl  rcl;
    COLORREF colorFore;
    HBRUSH hBrush;
    CSite **ppSite, **ppSiteEnd;
    RECT *prcSite = &_rc;


    hr = THR(super::Draw(pDI));

    // if we are not a listbox/combobox, just call super, do nothing special
    if ( getOwner()->TBag()->_eListBoxStyle == fmListBoxStylesNone )
        goto Cleanup;

    // so we are listbox/combobox, now draw borders

    // we need to draw a line at the bottom and lines
    // between all the controls....

    // we decided to just draw a one pixel line...

    colorFore = ColorRefFromOleColor(_colorFore);

    //  GetABrush
    hBrush = GetCachedBrush(colorFore);
    if (GetCurrentObject(pDI->_hdc, OBJ_BRUSH) != hBrush)
    {
        SelectObject(pDI->_hdc, hBrush);
    }

    // first draw the horizontal line at the bottom
    PatBlt(pDI->_hdc,   // handle of device context
            prcSite->left,   // x-coord. of upper-left corner of rect. to be filled
            prcSite->bottom-1, // y-coord. of upper-left corner of rect. to be filled
            prcSite->right-prcSite->left,    // width of rectangle to be filled
            1,  // height of rectangle to be filled
            PATCOPY);   // raster operation code

    // now go over the controls and draw vertical lines

    if (_arySites.Size() > 1)
    {
        // the loop skips the last control, we don't want
        // to draw a line at the very right
        for (ppSite = _arySites, ppSiteEnd = ppSite + _arySites.Size()-1;
                 ppSite < ppSiteEnd; ppSite++)
        {
            rcl = (*ppSite)->_rcl;
            pDI->WindowFromDocument((RECT*)&rcl, &rcl);

            PatBlt(pDI->_hdc,   // handle of device context
            rcl.right,    // x-coord. of upper-left corner of rect. to be filled
            rcl.top, // y-coord. of upper-left corner of rect. to be filled
            1,    // width of rectangle to be filled
            rcl.Height(),  // height of rectangle to be filled
            PATCOPY);   // raster operation code
        }
    }

    ReleaseCachedBrush(hBrush);

Cleanup:
    RRETURN(hr);

}
//---- end of method ------------------------------------------------------------


//+------------------------------------------------------------------------
//
//  Member:     CHeaderFrame::PaintSelectionFeedback
//
//  Synopsis:   Paints the object's selection feedback, if it exists and
//              painting it is appropriate
//
//  Arguments:  hdc         HDC to draw in
//              prc         Rect to draw in
//              dwSelInfo   Addt'l info about the site
//
//  Returns     Nothing
//
//-------------------------------------------------------------------------

void
CHeaderFrame::PaintSelectionFeedback(CFormDrawInfo *pDI, RECT *prc, DWORD dwSelInfo)
{
    super::PaintSelectionFeedback(pDI, prc, dwSelInfo |
            DSI_LOCKED |
            (getTemplate()->_fIsFooter ? DSI_NOTOPHANDLES : 0));
}
