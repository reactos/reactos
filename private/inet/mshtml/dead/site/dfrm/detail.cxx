//----------------------------------------------------------------------------
//
//  Maintained by: alexa
//
//  Microsoft DataDocs
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       src\doc\datadoc\detail.cxx
//
//  Contents:   Implementation of the detail frame.
//
//  Classes:    CDetailFrame
//
//  Functions:  None.
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#include "dfrm.hxx"


DeclareTag(tagDetailFrame,"src\\ddoc\\datadoc\\detail.cxx","DetailFrame");
DeclareTag(tagDetailFrameTemplate,"src\\ddoc\\datadoc\\detail.cxx","DetailFrameTemplate");
extern TAG tagPropose;
extern TAG tagDataFrameDebugPaint;


const   DWORD SET_DEFAULT_CONTROL_VALUES = 1;

#if PRODUCT_97
PROP_DESC CDetailFrameTemplate::s_apropdesc[] =
{
    PROP_MEMBER(CSTRING,
                CDetailFrameTemplate,
                _TBag._cstrName,
                NULL,
                "Name",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(CSTRING,
                CDetailFrameTemplate,
                _IBag._cstrTag,
                NULL,
                "Tag",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(LONG,
                CDetailFrameTemplate,
                _TBag._ID,
                0,
                "ID",
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(LONG,
                CDetailFrameTemplate,
                _dwHelpContextID,
                0,
                "HelpContextID",
                PROP_DESC_BYTESWAPLONG)
   // PROP_MEMBER(LONG,
   //             CDetailFrameTemplate,
   //             _ulBitFlags,
   //             SITE_FLAG_DEFAULTVALUE,
   //             "BitFlags",
   //             PROP_DESC_BYTESWAPLONG)
    PROP_VARARG(LONG,
                sizeof(DWORD),
                0,
                "ObjectSize",
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(SHORT,
                CDetailFrameTemplate,
                _TBag._usTabIndex,
                (ULONG)-1L,
                "TabIndex",
                PROP_DESC_BYTESWAPSHORT)
    PROP_MEMBER(USERDEFINED,
                CDetailFrameTemplate,
                _rcl,
                NULL,
                "SizeAndPosition",
                PROP_DESC_BYTESWAPRECTL)

    // the above stuff is copied from the CSite implementation
    // now we first save the unnamed struct, this includes all bitfields

    PROP_MEMBER(LONG,
                CDetailFrameTemplate,
                _IBag._colorBack,
                NULL,
                "BackColor",
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(LONG,
                CDetailFrameTemplate,
                _IBag._colorFore,
                NULL,
                "ForeColor",
                PROP_DESC_BYTESWAPLONG)
    PROP_NOPERSIST(LONG, sizeof(long), NULL)   // Old WhatsThisHelpID
    PROP_MEMBER(CSTRING,
                CDetailFrameTemplate,
                _TBag._cstrControlTipText,
                NULL,
                "ControlTipText",
                PROP_DESC_NOBYTESWAP)
    PROP_NOPERSIST(CSTRING, sizeof(CStr), NULL)
};
#endif PRODUCT_97



CSite::CLASSDESC CDetailFrameTemplate::s_classdesc =
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
            SITEDESC_CANSELECT,
            &IID_IDataFrame,                // _piidDispinterface
            0,                              // _lAccRole
            &CSite::s_PropCats,             // _pPropCats;
            NULL
        },
        NULL,                               //_pfnTearoff
    },
    ETAG_DETAILFRAME,                       // _etag
};



CSite::CLASSDESC CDetailFrameInstance::s_classdesc =
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
            SITEDESC_PARENT |           // _dwFlags
            SITEDESC_BASEFRAME |
            SITEDESC_DETAILFRAME |
            SITEDESC_CANSELECT,
            &IID_IDataFrame,                // _piidDispinterface
            0,                              // _lAccRole
            &CSite::s_PropCats,             // _pPropCats;
            NULL
        },
        NULL,                               //_pfnTearoff
    },
    ETAG_DETAILFRAME,                       // _etag
};


//+-----------------------------------------------------------------------
//  Constructors, Destructors, Creation API
//------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Member      CDetailFrame::constructor
//
//  Synopsis    Template Constructor
//
//  Arguments   pDataDoc        pointer to a DataDoc
//              pParent         pointer to a parent object (template)
//
//----------------------------------------------------------------------------

CDetailFrame::CDetailFrame(CDoc * pDoc, CSite * pParent)
    : super(pDoc, pParent)
{
    #if DBG==1
    _ulRecycleCounter=0;
    _ulRecycleCreation=0;
    #endif
    _fAutoSize = FALSE;
    // This flag is set in listbox rows. We transform the height separately to maintain
    // uniform row heights (although the positions won't be distributed evenly... istvanc)
    _fNoClientClip = TRUE;
}




//+---------------------------------------------------------------------------
//
//  Member      CDetailFrame::constructor
//
//  Synopsis    Instance Constructor
//
//  Arguments   pDataDoc        pointer to a DataDoc
//              pParent         pointer to a parent object (instance)
//              pTemplate       pointer to a detail frame template.
//
//----------------------------------------------------------------------------

CDetailFrame::CDetailFrame(
        CDoc * pDoc,
        CSite * pParent,
        CDetailFrame * pTemplate)
    : super(pDoc, pParent, pTemplate)
{
    _fAutoSize = FALSE;
     // This flag is set in listbox rows. We transform the height separately to maintain
    // uniform row heights (although the positions won't be distributed evenly... istvanc)
    _fNoClientClip = TRUE;
}



//+---------------------------------------------------------------------------
//
//  Member      CDetailFrame::operator new.
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
CDetailFrame::operator new (size_t s, CDetailFrame * pOriginal)
{
    TraceTag((tagDetailFrame, "CDetailFrame::operator new "));
    void * pNew = ::operator new(s);
    if (pNew)
        memcpy(pNew, pOriginal, sizeof(CDetailFrame));
    return pNew;
}




//----------------------------------------------------------------------------
//  Member:     CreateInstance
//
//  Synopsis:   Create an instance of the CDetailFrame class.
//
//  Arguments:  pDataDoc            the form
//              ppfrFrame           returns the generated frame
//              pParent             parent of the new guy
//              hrow                the row to use
//
//
//  Returns:    Returns S_OK if everything is fine,
//              E_INVALIDARG if ppFrame is NULL.
//
//----------------------------------------------------------------------------

HRESULT
CDetailFrame::CreateInstance (
        CDoc * pDoc,
        CSite * pParent,
        OUT CBaseFrame **ppFrame,
        HROW hrow)
{
    TraceTag((tagDetailFrame, "CDetailFrame::CreateLayout"));


    HRESULT hr;

    Assert (ppFrame);
    Assert(_fOwnTBag);

    CDetailFrame * pDetailFrame;

    if (_apNode[NEXT_NODE])
    {
        pDetailFrame = (CDetailFrame*)_apNode[NEXT_NODE];

        Edge eLeft = (Edge)(1 - getOwner()->getDirection());
        if (pDetailFrame->_rcl[eLeft] != _rcl[eLeft] ||
            pDetailFrame->_rcl.Size() != _rcl.Size())
        {
            // Assert (FALSE);
            EmptyRecycle();
            goto NewInstance;
        }

        Assert(pDetailFrame->_pParent == 0);

        pDetailFrame->_pParent = pParent;

        pDetailFrame->_fInvalidated = TRUE; // to avoid invalidations of controls inside detailframe
        hr = pDetailFrame->MoveToRow(hrow, SET_DEFAULT_CONTROL_VALUES);
        pDetailFrame->_fInvalidated = FALSE;

        if (FAILED(hr))
        {
            goto RecycleFail;
        }

        _apNode[NEXT_NODE] = _apNode[NEXT_NODE]->_apNode[NEXT_NODE];
        if (_apNode[NEXT_NODE])
        {
            _apNode[NEXT_NODE]->_apNode[PREV_NODE] = 0;
        }

        #if DBG==1
        _ulRecycleCreation++;
        #endif
    }
    else
    {

NewInstance:
        pDetailFrame = new (this) CDetailFrameInstance(pDoc, pParent, this);
        if (pDetailFrame)
        {
            hr = pDetailFrame->InitInstance();
            if (hr)
            {
                goto Error;
            }
            else
            {
    #           ifndef  ABSOLUTE_COORDINATES
                NORMALIZE( *( &pDetailFrame->_rcl ) );
    #           endif


                hr =  BuildInstance (pDetailFrame);
                if (hr)
                    goto Error;

                pDetailFrame->_fInvalidated = TRUE; // to avoid invalidations of controls inside detailframe
                hr = pDetailFrame->MoveToRow(hrow, SET_DEFAULT_CONTROL_VALUES);
                pDetailFrame->_fInvalidated = FALSE;
            }
        }
        else
            goto MemoryError;
    }

Cleanup:
    *ppFrame = pDetailFrame;
    #if DBG==1
    if (pDetailFrame)
    {
        Assert(!pDetailFrame->IsInSiteList());
        pDetailFrame->SetInSiteList(FALSE);
    }
    #endif
    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

Error:
    pDetailFrame->Release();
    pDetailFrame = NULL;
    goto Cleanup;

RecycleFail:
    pDetailFrame->_pParent = 0;
    pDetailFrame = 0;
    goto Cleanup;
}



//+---------------------------------------------------------------
//
//  Member:     CDetailFrame::Notify
//
//  Synopsis:   Handle notification
//
//---------------------------------------------------------------
HRESULT
CDetailFrame::Notify(SITE_NOTIFICATION sn, DWORD dw)
{
    HRESULT hr = S_OK;

    switch (sn)
    {
    case SN_AFTERLOAD:
        ((CDataFrame *)_pParent)->_pDetail = this;
        hr = THR(super::Notify(sn, dw));
        break;

    default:
        hr = THR(super::Notify(sn, dw));
    }

    RRETURN1(hr, S_FALSE);
}





//+---------------------------------------------------------------------------
//
//  Member:     AddToSites
//
//  Synopsis:   overloaded for matrix and index management
//
//  Returns:    Returns result from parent
//
//----------------------------------------------------------------------------

HRESULT
CDetailFrame::AddToSites(CSite *pSite, DWORD dwOperations)
{
    HRESULT hr=S_OK;
    fmListBoxStyles    ls;
    BOOL            fListBox;
    IMorphDataControl * pMordControl=0;

    fListBox =  (!getOwner()->GetListBoxStyle(&ls) && ls > 0);
    if (fListBox)
    {
        // the office listbox does not need the controls in
        //  the collection
        dwOperations &= ~(ADDSITE_INCOLLECTION |
                          ADDSITE_NEWSITE |
                          ADDSITE_ASSIGNNAME |
                          ADDSITE_SETCAPTION);
    }

    if (dwOperations)
    {
        hr = THR(super::AddToSites(pSite, dwOperations));

        if (hr)
            goto Cleanup;

        // BUGBUG: We should push default property settings
        //  to the controls here

        #if PRODUCT_97

        if (_fOwnTBag && (dwOperations & ADDSITE_ADDTOLIST))
        {
            CMatrix * pMatrix = TBag()->_pMatrix;
            if (pMatrix)
            {
                hr = pMatrix->AddSite(pSite, FALSE);
                if (hr)
                {
                    pMatrix->Release();    // rebuild it later
                    goto Cleanup;
                }
            }
        }
        #endif PRODUCT_97

        if (fListBox)
        {
            Assert(pSite->TestClassFlag(SITEDESC_OLEDATASITE));
            if ( ! ((COleDataSite*)pSite)->_fFakeControl )
            {
                hr = pSite->QueryInterface(IID_IMorphDataControl, (void **) &pMordControl);
                Assert( (!hr) && "We should have only MorphDataControls inside a listbox.");
                if ( ! hr )
                {
                    //  BUGBUG: This comes back as soon as the repeater doesn't
                    //          crash when it's empty and painting.

                    // pMordControl->SetBordersSuppress(TRUE);
                    pMordControl->Release();
                }
            }
        }

        if (dwOperations & ADDSITE_FIREEVENT)
        {
            OnDataChange(DISPID_AddControl, FALSE);
        }
    }

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     DeleteSite
//
//  Synopsis:   overloaded for matrix and index management
//
//  Returns:    Returns result from parent
//
//----------------------------------------------------------------------------

HRESULT
CDetailFrame::DeleteSite(CSite * pSite, DWORD dwFlags)
{
    HRESULT hr;

    if (_fOwnTBag)
    {
        if (getOwner()->getDetail()->getTemplate() == this)
        {
            getOwner()->RemoveRelated(pSite, dwFlags);
        }

        #if PRODUCT_97

        CMatrix * pMatrix = TBag()->_pMatrix;
        if (pMatrix)
        {
            hr = pMatrix->RemoveSite(pSite, FALSE);
            if (hr)
            {
                pMatrix->Release();    // rebuild it later
            }
        }

        #endif
    }

    hr =  super::DeleteSite(pSite, dwFlags);

    #if PRODUCT_97
    if (hr && _fOwnTBag)
    {
        CMatrix * pMatrix = TBag()->_pMatrix;
        if (pMatrix)
        {
            pMatrix->Release();
        }
    }
    #endif

    RRETURN(hr);
}





//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrame::HitTestPointl, public
//
//  Synopsis:   Determines if the given point is inside the site.
//
//  Arguments:  [ptl]       -- point to check, in document coordinates
//              [prclClip]  -- optional clipping rectangle to check inside
//              [ppSite]    -- return the site hit
//              [pMessage]  -- Ptr to a message struct.  Can be NULL
//              [dwFlags]   -- HT_ flags
//
//  Returns:    HTC
//
//  note:       This override provides the limited hit detection logic
//              necessary for the listbox. It disables resize border and grab
//              handle detection, forcing the arrow cursor to appear all
//              over the listbox.
//
//----------------------------------------------------------------------------

HTC
CDetailFrame::HitTestPointl(
    POINTL ptl,
    RECTL * prclClip,
    CSite ** ppSite,
    CMessage *pMessage,
    DWORD dwFlags)
{
    TraceTag((tagDetailFrame,"CBaseFrame::HitTestPointl"));

    HTC htc;

    if ( getOwner()->TBag()->_eListBoxStyle )
    {
        RECTL rcl;

        if (prclClip)
        {
            IntersectRectl(&rcl, &_rcl, prclClip);
        }
        else
        {
            rcl = _rcl;
        }

        if ( PtlInRectl(&rcl, ptl) )
        {
            htc = HTC_YES;
            if (ppSite)
            {
                *ppSite = this;
            }
        }
        else
        {
            htc = HTC_NO;
        }
        if (pMessage)
        {
           pMessage->pSiteHit = *ppSite;
           pMessage->htc = htc;
        }
    }
    else
    {
// BUGBUG (garybu) Hit test point removed.
//        htc = super::HitTestPointl(ptl, prclClip, ppSite, pMessage, dwFlags);
htc=HTC_NO;
    }

    return htc;
}




#if PRODUCT_97

//+---------------------------------------------------------------
//
//  Member:     CDetailFrame::Move
//
//  Synopsis:   Move and/or resize the site
//
//  Arguments:  rcl         New position in document coordinates
//              dwFlags]    Specifies flags
//                          SITEMOVE_NOFIREEVENT  -- Don't fire a move event
//                          SITEMOVE_NOINVALIDATE -- Don't invalidate the rect
//                          SITEMOVE_NOSETEXTENT  -- Don't call SetExtent on the
//                                                   object
//
//  Returns     HRESULT
//
//---------------------------------------------------------------

HRESULT
CDetailFrame::Move(RECTL *prcl, DWORD dwFlags)
{
    if (_fOwnTBag)
    {
        DirtyMatrix();
    }

    RRETURN(super::Move(prcl, dwFlags));
}


//+-------------------------------------------------------------------------
//
//  Method:     CDetailFrame::SetProposed
//
//  Synopsis:   Remember the deferred move rectangle for this site
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CDetailFrame::SetProposed(CSite * pSite, const CRectl * prcl)
{
    HRESULT hr;

    TraceTag((tagPropose, "%ls/%d CDetailFrame::SetProposed %d,%d,%d,%d to %d,%d,%d,%d",
        TBag()->_cstrName, TBag()->_ID,
        prcl->left - pSite->_rcl.left, prcl->top - pSite->_rcl.top, prcl->right - pSite->_rcl.right, prcl->bottom - pSite->_rcl.bottom,
        prcl->left, prcl->top, prcl->right, prcl->bottom));

    if (pSite == this)
    {
        return super::SetProposed(pSite, prcl);
    }

    hr = EnsureMatrix();

    if (!hr)
    {
        hr = TBag()->_pMatrix->SetProposed(pSite, prcl);
    }

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDetailFrame::GetProposed
//
//  Synopsis:   Get back the deferred move rectangle for this site
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CDetailFrame::GetProposed(CSite * pSite, CRectl * prcl)
{
    HRESULT hr;

    if (pSite == this)
    {
        return super::GetProposed(pSite, prcl);
    }

    hr = EnsureMatrix();

    if (!hr)
    {
        if (TBag()->_pMatrix->getOuter() == this)
        {
            hr = TBag()->_pMatrix->GetProposed(pSite, prcl);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    TraceTag((tagPropose, "%ls/%d CDetailFrame::GetProposed %d,%d,%d,%d to %d,%d,%d,%d",
        TBag()->_cstrName, TBag()->_ID,
        prcl->left - pSite->_rcl.left, prcl->top - pSite->_rcl.top, prcl->right - pSite->_rcl.right, prcl->bottom - pSite->_rcl.bottom,
        prcl->left, prcl->top, prcl->right, prcl->bottom));

    RRETURN(hr);


}

//+-------------------------------------------------------------------------
//
//  Method:     CDetailFrame::CalcControlPositions
//
//  Synopsis:   Calculate the proposed positions for the oledatasites
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CDetailFrame::CalcControlPositions(DWORD dwFlags)
{
    HRESULT hr;
    hr = EnsureMatrix();
    if (hr)
        goto Cleanup;

    hr = TBag()->_pMatrix->DoGridResize(this, dwFlags);
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CDetailFrame::DrawFeedbackRect
//
//  Synopsis:   Draw feedback rectangle
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

void
CDetailFrame::DrawFeedbackRect(HDC hDC)
{
    /*
        Draw only if we already have the matrix and
        the outer is this row...
    */
    CMatrix * pMatrix = TBag()->_pMatrix;

    if (pMatrix && pMatrix->getOuter() == this)
    {
        if (_fProposedSet)
        {
            CSite::DrawFeedbackRect(hDC);
        }

        pMatrix->DrawGhosts(_pDoc, hDC);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     DirtyMatrix
//
//  Synopsis:   Makes the matrix dirty if exists
//
//----------------------------------------------------------------------------
void
CDetailFrame::DirtyMatrix()
{
    CMatrix * pMatrix = TBag()->_pMatrix;
    if (pMatrix)
    {
        pMatrix->SetDirty();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     EnsureMatrix
//
//  Synopsis:   Gets the template matrix object pointer.
//
//  Returns:    Returns S_OK if everything is fine, else matrix error
//
//----------------------------------------------------------------------------

HRESULT
CDetailFrame::EnsureMatrix (void)
{
    TraceTag((tagDetailFrame, "CDetailFrame::EnsureMatrix"));

    HRESULT hr = S_OK;
    CTBag * pTBag = TBag();

    if (!pTBag->_pMatrix)
    {
        CMatrix * pMatrix = new CMatrix(&pTBag->_pMatrix);
        if (!pMatrix)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pMatrix->Init(this);
            if (hr)
            {
                pMatrix = NULL;
            }
        }
        pTBag->_pMatrix = pMatrix;
    }

    RRETURN(hr);
}




#endif PRODUCT_9



//+-------------------------------------------------------------------------
//
//  Method:     CDetailFrame::CalcProposedPositions
//
//  Synopsis:   Calculate the proposed positions for children
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CDetailFrame::CalcProposedPositions()
{
    HRESULT hr=S_OK;
    CRectl rclPropose;

    TraceTag((tagPropose, "%ls/%d CDetailFrame::CalcProposedPositions",
        TBag()->_cstrName, TBag()->_ID));

    if (!_fProposedSet)
    {
        SetProposed(this, &_rcl);
    }

    #if PRODUCT_97
    hr = CalcControlPositions(0);
    if (hr)
        goto Cleanup;
    #endif

    GetProposed(this, &rclPropose);
    if (!_fMark1 && rclPropose != _rcl)  // indicates that this one is forcing the action
    {
        hr = ProposedFriendFrames();
        if (hr)
        {
            goto Cleanup;
        }
    }

Cleanup:

    RRETURN(hr);
}





#if defined(PRODUCT_97)
//+-------------------------------------------------------------------------
//
//  Method:     CDetailFrame::ProposedFriendFrames
//
//  Synopsis:   Calculate the proposed positions for associated frames
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CDetailFrame::ProposedFriendFrames(void)
{
    Assert(_fProposedSet);

    if (!_fMark1)
    {
        // this prevents the repeater callback
        super::ProposedFriendFrames();
        BOOL    fIsVertical = getOwner()->IsVertical();
        CRectl  rclProposed;
        CRectl  rclDelta;

        CBaseFrame *pRelatedOne, *pRelatedTwo;

        pRelatedOne = getOwner()->_pFooter;
        pRelatedTwo = getOwner()->_pHeader;

        GetProposed(this, &rclProposed);

        rclDelta = rclProposed;
        rclDelta -= _rcl;

        Edge e = (Edge) ( getOwner()->IsVertical());
        Edge eOther = (Edge) (e+2);
        rclDelta[e]      = 0;
        rclDelta[eOther] = 0;

        if (this != getOwner()->_pDetail)
        {
            getOwner()->_pDetail->ProposedDelta(&rclDelta);
        }

        if (pRelatedOne)
        {
            pRelatedOne->ProposedDelta(&rclDelta);
        }
        if (pRelatedTwo)
        {
            pRelatedTwo->ProposedDelta(&rclDelta);
        }
    }
    return S_OK;
}
#endif



//+-------------------------------------------------------------------------
//
//  Method:     CDetailFrame::MoveToProposed
//
//  Synopsis:   Move children to the calculated proposed positions
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CDetailFrame::MoveToProposed(DWORD dwFlags)
{
    HRESULT hr = S_OK;

    TraceTag((tagPropose, "%ls/%d CDetailFrame::MoveToProposed",
        TBag()->_cstrName, TBag()->_ID));

    // cannot call super because the Matrix is doing the move
    if (_fProposedSet)
    {
        CRectl rclPropose;

        hr = GetProposed(this, &rclPropose);
        if (hr)
            goto Cleanup;

        if (rclPropose != _rcl)
        {
            Move(&rclPropose, IsVisible() ? dwFlags : dwFlags | SITEMOVE_NOINVALIDATE);
            if (hr)
                goto Cleanup;
            _fIsDirtyRectangle = TRUE;
        }

        TraceTag((tagPropose, "%ls/%d CDetailFrame::MoveToProposed %d,%d,%d,%d",
            TBag()->_cstrName, TBag()->_ID,
            rclPropose.left, rclPropose.top, rclPropose.right, rclPropose.bottom));
    }

    #if PRODUCT_97
    hr = EnsureMatrix();
    if (hr)
        goto Cleanup;

    hr = TBag()->_pMatrix->MoveSites(dwFlags);
    #endif PRODUCT_97

    if ( _fOwnTBag )
        OptimizeBackgroundPainting();

    if ((_fIsDirtyRectangle || IsDirtyBelow()) && _pParent)
    {
        ((CBaseFrame*)_pParent)->SetDirtyBelow(TRUE);
    }


    _fProposedSet = FALSE;

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member      CDetailFrame::ProposedDelta
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
CDetailFrame::ProposedDelta(CRectl *prclDelta)
{
    // we want to get resized

    // this case is only happening when
    //  the detailtemplate is there but the
    //  repeater is not. The template now should
    //  not resize to the dataframe sizing
    //  therefore we will bail out
    if (_fOwnTBag && !_fVisible)
        return S_OK;

    super::ProposedDelta(prclDelta);

    CRectl rcl;
    GetProposed(this, &rcl);

    Edge e = (Edge) (1 - getOwner()->IsVertical());
    Edge eOther = (e == edgeLeft ? edgeTop : edgeLeft);
    Edge eOpposite = (Edge) (e+2);
    Edge eOtherOp = (Edge) (eOther+2);

    // restore edges in the direction of repeat
    // BUGBUG very ugly hack temporary to allow generate work
    if (_rcl[e] == 0 && _rcl[eOpposite] == 0)
        ;
    else
    {
        rcl[e] = _rcl[e];
        rcl[eOpposite] = _rcl[eOpposite];
    }
    // rcl = _rcl;

    rcl[e] += (*prclDelta)[e];
    rcl[eOpposite] += (*prclDelta)[eOpposite];

    rcl[eOther]     +=  (*prclDelta)[eOther];
    rcl[eOtherOp]   +=  (*prclDelta)[eOtherOp];

    SetProposed(this, &rcl);

    return S_OK;
}
//--end of method--------------------------------------------------------------




//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrame::OptimizeBackgroundPainting
//
//  Synopsis:   Decide if the detail frame's background needs to be Painted.
//
//  Arguments:  None
//
//  Note:       Sets the flag _fPaintBackground.
//              Sums the area of the child controls to see if they fill the
//              frame completely.
//
//----------------------------------------------------------------------------
void
CDetailFrame::OptimizeBackgroundPainting(void)
{
    int i;
#ifndef _MAC
    LONGLONG uvlAreaSum = 0;
#else
    LARGE_INTEGER uvlAreaSum = {0,0};
#endif
    CRect rc;

    Assert(_fOwnTBag);

    //  BUGBUG: I needed to disable the pixel conversion for the optimization
    //  as Gary's hack below doesn't work; it produces zero all over the place
    //  completely suppressing background painting.

    for ( i = _arySites.Size(); --i >= 0; )
    {
        if ( _arySites[i]->IsAutoSize() )
        {
            _fPaintBackground = TRUE;
            return;
        }
        else
        {
#ifndef _MAC
            uvlAreaSum += _arySites[i]->_rcl.Area();
#else
            // BUGBUG how big can uvlAreaSum get?
            uvlAreaSum.LowPart += _arySites[i]->_rcl.Area();
#endif
        }
    }

#ifndef _MAC
    _fPaintBackground = uvlAreaSum < _rcl.Area();
#else
    _fPaintBackground = uvlAreaSum.LowPart < _rcl.Area();
#endif
}



//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrame::DrawBackground
//
//  Synopsis:   Paint the background
//
//----------------------------------------------------------------------------

HRESULT
CDetailFrame::DrawBackground(CFormDrawInfo *pDI)
{
    TraceTag((tagDetailFrame,"CDetailFrame::PaintBackground "));
    DWORD color;
    HBRUSH hBrush;
    HPEN hPen;
    RECT *prc = &_rc;

    if (pDI->_fAfterStartSite)
    {
        if (getTemplate()->_fPaintBackground)
        {
#if DBG == 1
            if (IsTagEnabled(tagDataFrameDebugPaint))
            {
                if (TestClassFlag(SITEDESC_HEADERFRAME))
                {
                    color = RGB(255,255,255); // white
                }
                else
                {
                    color = RGB(0, 255, 0);    // green
                }
                if (TestClassFlag(SITEDESC_TEMPLATE))
                {
                    color ^= 0x00555555;
                }
            }
#if 0   // MAC BUGBUG - Is this correct?  Causes hPen to not be set, which upsets SelectObject()
        //              Should this whole DBG block get moved down just before the GetCachedBrush?
            else
#endif
#endif
            if (getOwner()->TBag()->_eListBoxStyle || !_pDoc->_fDesignMode)
            {
                if (_fSelected)
                {
                    color = GetSysColorQuick(COLOR_HIGHLIGHT);
                }
                else
                {
                    color = ColorRefFromOleColor(_colorBack);
                }
                hPen = (HPEN)GetStockObject(NULL_PEN);
            }
            else
            {
                color = ColorRefFromOleColor(_colorBack);
                hPen = (HPEN)GetStockObject(BLACK_PEN);
            }

            hBrush = GetCachedBrush(color);

            if (GetCurrentObject( pDI->_hdc, OBJ_BRUSH ) != hBrush)
                SelectObject(pDI->_hdc, hBrush);

            if (GetCurrentObject( pDI->_hdc, OBJ_PEN ) != hPen)
                SelectObject(pDI->_hdc, hPen);

            if (GetROP2( pDI->_hdc) != R2_COPYPEN)
                SetROP2( pDI->_hdc, R2_COPYPEN );

            Rectangle(pDI->_hdc, prc->left, prc->top, prc->right, prc->bottom);

            ReleaseCachedBrush(hBrush);
        }
    }
    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrame::Draw, public
//
//  Synopsis:   Draws the interior of a frame in the passed rectangular area
//
//  Arguments:  hDC          device context
//              prcSite      The site's rectangle.
//              prcPaint     Dirty area to Paint
//
//  Returns:    hr
//
//  Note:       called from the Display method, controls should override for
//              Painting
//              Optimization: doesn't Paint if the background doesn't show through.
//
//----------------------------------------------------------------------------

HRESULT
CDetailFrame::Draw(CFormDrawInfo *pDI)
{
    CDataFrame * pdfrOwner = getOwner();
    RECT *prcSite = &_rc;

    TraceTag((tagDetailFrame,"CDetailFrame::Paint "));

    HRESULT hr =  THR(super::Draw(pDI));

    if (!hr         &&
        _fCurrent   &&
        ((pdfrOwner == _pDoc->_pSiteUIActive) ||
         (pdfrOwner->TBag()->_eListBoxStyle == fmListBoxStylesComboBox)) )
    {
        RECT rc;
        CRectl rcl(_rcl);
        CRectl rclViewPort;

        pdfrOwner->GetDetailSectionRectl(&rclViewPort);

        rcl.right = rclViewPort.right;
        if ( pdfrOwner->TBag()->_eListStyle == fmListStyleOption )
        {
            int c;
            CSite ** ppSite;

            for ( ppSite = _arySites, c = _arySites.Size(); --c >= 0; ppSite++ )
            {
                Assert((*ppSite)->TestClassFlag(SITEDESC_OLEDATASITE));
                if ( ((COleDataSite*)(*ppSite))->_fFakeControl )
                    break;
            }
            if ( c >= 0 )
            {
                rcl.left = max((*ppSite)->_rcl.right,rclViewPort.left);
            }
        }
        else
        {
            rcl.left = rclViewPort.left;
        }

        pDI->WindowFromDocument(&rc,&rcl);
        DrawFocusRect(pDI->_hdc, &rc);
    }

    RRETURN (hr);
}









//+---------------------------------------------------------------------------
//
//  Member:     MoveToRow
//
//  Synopsis:   Set this layout currency.
//              calls every child to refetch
//
//  Arguments:  hrow        HROW to display
//              dwFlags     ignored for now, used in repeater
//
//  Returns:    Returns always S_OK.
//
//----------------------------------------------------------------------------

HRESULT
CDetailFrame::MoveToRow(HROW hrow, DWORD dwFlags)
{
    TraceTag((tagDetailFrame, "CDetailFrame::MoveToRow"));

    HRESULT hr = S_OK;
    CSite   **  ppSite;
    CSite   **  ppSiteEnd;

    #if DBG==1
    HROW *pHrow = &_hrow;
    #endif

    if (_hrow != hrow)
    {
        if (_hrow)
        {
            // If the there is a dirty active site then save the data prior to
            // releasing the HROW.

            //BUGBUG:  This can be removed when page down/up de-select the
            //         current control.  De-select logic saves a dirty control
            //         data.  **TerryLu**
            Assert(_pDoc);
            if (_pDoc->_pSiteCurrent && _pDoc->_pSiteCurrent->_fValDirty)
                IGNORE_HR(_pDoc->_pSiteCurrent->SaveData());
            // BUGBUG: we should have a better error handling

            // if currency is removed, release the old hrow
            CDataLayerCursor *pCursor;

            hr = THR(getOwner()->LazyGetDataLayerCursor(&pCursor));
            if (SUCCEEDED(hr))
            {
                Assert(pCursor);
                pCursor->ReleaseRows(1, &_hrow);
            }
            _hrow = NULL;
#ifndef PRODUCT_97
            ResetSelectionPointer();
            if (_fSelected)
            {
                SetSelected(FALSE);
            }
#else
            SetSelectionStates(SS_SETSELECTIONSTATE|SS_CLEARSELECTION);
#endif
        }

        _hrow = hrow;

        //  Hook up to the selection tree
        if ( _hrow )
        {
            Assert(getOwner());
            SetSelectionStates();
            RefreshData();
        }

    }

    if (!hrow && (dwFlags & SET_DEFAULT_CONTROL_VALUES))
    {
        // it's a 0-row (new empty row).
        // we need to set all the controls to the default values
        VARIANT     variant;
        EXCEPINFO   except;

        InitEXCEPINFO(&except);

        variant.vt = VT_BSTR;
        variant.bstrVal = NULL;
        ppSite = _arySites;
        ppSiteEnd = ppSite + _arySites.Size();
        for (; ppSite < ppSiteEnd; ppSite++)
        {

        // Make sure it's a valid control name and a real COleSite derived
        // class before we even try to connect up.
            CSite *pSite = *ppSite;
            if (!hr && pSite->SiteDesc()->TestFlag(SITEDESC_OLEDATASITE) &&
                ! ((COleDataSite*)pSite)->_fFakeControl)
            {
                ((COleSite *)pSite)->CacheDispatch();
                IDispatch   *pDispatch = ((COleSite *)pSite)->_pDisp;
                if (pDispatch)
                {
                    pSite->_fValRestoring = TRUE;
                    hr = THR(SetDispProp(pDispatch, DISPID_VALUE,
                                IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                &variant, &except));
                    pSite->_fValRestoring = FALSE;
                    Assert(SUCCEEDED(hr));
                    FreeEXCEPINFO(&except);
                }
            }
        }
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     RefreshData
//
//  Synopsis:   stuffs new data into the controls on the detailframe
//              API is called from MoveToRow and from the repeater
//              in case of an update
//
//  Returns:    S_OK or Failure
//
//----------------------------------------------------------------------------
HRESULT
CDetailFrame::RefreshData(void)
{
    CSite   **  ppSite;
    CSite   **  ppSiteEnd;

    Assert(_hrow);

    ppSite = _arySites;
    if (ppSite)
    {
        ppSiteEnd = ppSite + _arySites.Size();

        for (; ppSite < ppSiteEnd; ppSite++)
        {
            IGNORE_HR((*ppSite)->RefreshData());
            // BUGBUG: we should have a better error handling
        }
    }
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     MoveToRecycle
//
//  Synopsis:   moves a detailframe to the recycle list
//                  the passed frame is treated as the start of a sublist
//                  the whole list is moved over
//
//  Returns:    number of layouts moved into the recycle
//
//----------------------------------------------------------------------------
int
CDetailFrame::MoveToRecycle(CBaseFrame *pbfr)
{
    Assert(pbfr);

    TraceTag((tagDetailFrame,"CDetailFrame::MoveToRecycle"));

    Assert(_fOwnTBag);
    CBaseFrame *pbfrCur, *pbfrLast;
    int     iNrRecycled = 0;

    pbfrCur = pbfrLast = pbfr;


    while (pbfrCur)
    {
        // first walk the complete list and set those kids up
        pbfrCur->MoveToRow(0);   // reset the currency of the layout to 0
        Assert(pbfrCur != _pDoc->_pSiteUIActive);
        Assert(pbfrCur != _pDoc->_pSiteCurrent);

        if (pbfrCur->IsInSiteList())
        {
            pbfrCur->_pParent->DeleteSite(pbfrCur, DELSITE_MOVETORECYCLE |
                                                   DELSITE_NOPASSIVATE |
                                                   DELSITE_NOTABADJUST |
                                                   DELSITE_NODELETESTG |
                                                   DELSITE_NOREGENERATE |
                                                   DELSITE_NOINVALIDATE);
            pbfrCur->SetInSiteList(FALSE);
        }
        #if DBG==1
        _ulRecycleCounter++;
        #endif

        pbfrCur->_pParent = 0;
        if (!pbfrCur->_apNode[NEXT_NODE])
        {
            pbfrLast = pbfrCur;
        }
        pbfrCur = pbfrCur->_apNode[NEXT_NODE];
        iNrRecycled++;
    }

    if (_apNode[NEXT_NODE])
    {
        pbfrLast->_apNode[NEXT_NODE] = _apNode[NEXT_NODE];
        _apNode[NEXT_NODE]->_apNode[PREV_NODE] = pbfrLast;
    }
    else
    {
        pbfrLast->_apNode[NEXT_NODE] = 0;
    }
    pbfr->_apNode[PREV_NODE] = 0;
    _apNode[NEXT_NODE] = pbfr;

    return (iNrRecycled);

}
//-+ End of Method-------------------------------------------------------------







//+---------------------------------------------------------------------------
//
//  Member:     EmptyRecycle
//
//  Synopsis:   Empties recycle list
//
//----------------------------------------------------------------------------

void
CDetailFrame::EmptyRecycle()
{
    Assert(_fOwnTBag);

    // the template holds the recycle list, so get rid of it
    // get rid of all frames in the recycle list
    CBaseFrame *p, *pNext;
    for (p = _apNode[NEXT_NODE]; p; p = pNext)
    {
        pNext = p->_apNode[NEXT_NODE];
        // if "p" is on the site list of the parent, then the parent is
        // not null, and that frame will be detached during super::Detach ().

        Assert(!p->IsInSiteList() && "Found some site in the recycle with a Parent !");
        p->Detach ();
        // All the frames should be released.
        p->Release ();
    }
    _apNode[NEXT_NODE] = 0;

}
//-+ End of Method-------------------------------------------------------------

#if 0 //ndef PRODUCT_97

//+---------------------------------------------------------------
//
//  Member:     CDetailFrame::DragEnter
//
//  Synopsis:   Setup for possible drop
//
//  Arguments   pDataObject     pointer to a Data Object
//
//  Returns     HRESULT
//
//---------------------------------------------------------------
HRESULT
CDetailFrame::DragEnter( LPDATAOBJECT pDataObj,
    DWORD grfKeyState,
    POINTL ptlScreen,
    LPDWORD pdwEffect)
{
    HRESULT             hr = S_OK;
    CCursorTracker *    pTrack = NULL;

    TraceTag((tagDetailFrame,"CDetailFrame::DragEnter"));

    hr = FireDragEvent(fmDragStateEnter, grfKeyState, ptlScreen, pdwEffect);
    if (hr)
        goto Cleanup;

    Assert(_pDoc);
// BUGBUG (garybu) DESink removed from form
//    _pDoc->_pDESink->GetTracker((void **)&pTrack);

    if (pTrack && _pDoc->_fNonOverlapping )
    {
        POINTL ptl;
        _pDoc->DocumentFromScreen(&ptl, *(POINT*)&ptlScreen);
        ((CGridDragTracker *)pTrack)->DragEnter(ptl);
    }
    hr = THR(super::DragEnter(pDataObj, grfKeyState, ptlScreen, pdwEffect));

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     CDetailFrame::DragOver
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
CDetailFrame::DragOver(DWORD grfKeyState, POINTL ptlScreen, LPDWORD pdwEffect)
{
    TraceTag((tagDetailFrame,"CDetailFrame::DragOver"));

    HRESULT             hr = S_OK;
    FireDragEvent(fmDragStateOver, grfKeyState, ptlScreen, pdwEffect);

    CCursorTracker *    pTrack = NULL;
    int                 c;
    CSite **            ppSite;

    Assert(_pDoc);
// BUGBUG (garybu) DESink removed from form
//     _pDoc->_pDESink->GetTracker((void **)&pTrack);
pTrack = NULL;
    if (pTrack && _pDoc->_fNonOverlapping )
    {
        c = _pDoc->_pSiteCurrent->GetSelectedSites(&ppSite);
        if ( AreMyChildren(c, ppSite) )
        {
            HWND hWnd = _pDoc->_pInPlace->_hwnd;
            HDC hDC = ::GetDC(hWnd);

            // Calculate new feedback location
            POINTL ptl;
            _pDoc->DocumentFromScreen(&ptl, *(POINT*)&ptlScreen);
            ((CGridDragTracker *)pTrack)->DragOver(hDC, ptl, _pDoc->_sizelDragFeedback);
            ::ReleaseDC(hWnd, hDC);

            hr = THR(super::DragOver(grfKeyState, ptlScreen, pdwEffect));
        }
        else
        {
            Assert(pdwEffect);
            *pdwEffect = DROPEFFECT_NONE;
        }
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CDetailFrame::DragLeave
//
//  Synopsis:   Remove any user feedback
//
//  Arguments   None
//
//  Returns     HRESULT
//
//---------------------------------------------------------------
HRESULT
CDetailFrame::DragLeave(void)
{
    HRESULT             hr = S_OK;
    DWORD               dwEffect;
    POINTL              ptl;
    TraceTag((tagDetailFrame,"CDetailFrame::DragLeave"));


    FireDragEvent(fmDragStateLeave, 0, ptl, &dwEffect);

    Assert(_pDoc);


    CCursorTracker *    pTrack = NULL;

// BUGBUG (garybu) DESink removed from form
//    _pDoc->_pDESink->GetTracker((void **)&pTrack);
    if ( pTrack && _pDoc->_fNonOverlapping )
    {
        HWND hWnd = _pDoc->_pInPlace->_hwnd;
        HDC hDC = ::GetDC(hWnd);

        ((CGridDragTracker *)pTrack)->DragLeave(hDC);

        ::ReleaseDC(hWnd, hDC);
    }

    hr = THR(super::DragLeave());

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CDetailFrame::Drop
//
//  Synopsis:   Handle the drop operation
//
//---------------------------------------------------------------

//  BUGBUG should be global?
#define DROPEFFECT_ALL (DROPEFFECT_NONE | DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK)

HRESULT
CDetailFrame::Drop(
        LPDATAOBJECT pDataObj,
        DWORD        grfKeyState,
        POINTL       ptlScreen,
        LPDWORD      pdwEffect)
{
    HRESULT     hr = S_OK;
    TraceTag((tagDetailFrame,"CDetailFrame::Drop"));

    Assert(_pDoc);


    if (!FireDropEvent(grfKeyState, ptlScreen, pdwEffect))
        goto Cleanup;

    //
    // Find out what the effect is and execute it
    // If our operation fails we return DROPEFFECT_NONE
    //
    DragOver(grfKeyState, ptlScreen, pdwEffect);
    DragLeave();

    int         c;
    CSite **    ppSite;

    if ( _pDoc->_fInDragDrop )
    {
        Assert(_pDoc->_fNative);
        if ( _pDoc->_fInDragDrop &&
            (*pdwEffect & DROPEFFECT_MOVE) == DROPEFFECT_MOVE &&
             _pDoc->_fNonOverlapping )
        {
            c = _pDoc->_pSiteCurrent->GetSelectedSites(&ppSite);
            if ( AreMyChildren(c, ppSite) )
            {
                //super::Drop(pDataObj, grfKeyState, ptlScreen, pdwEffect);
                CCursorTracker *    pTrack = NULL;

// BUGBUG (garybu) DESink removed from form
//                _pDoc->_pDESink->GetTracker((void **)&pTrack);
                ((CGridDragTracker *)pTrack)->Commit(0);
                *pdwEffect = DROPEFFECT_NONE;
            }
            else
            {
                Assert(pdwEffect);
                *pdwEffect = DROPEFFECT_NONE;
            }
        }

    }
    else
    {
        Assert(pdwEffect);
        *pdwEffect = DROPEFFECT_NONE;
    }

Cleanup:
    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrame::DragHide
//
//  Synopsis:   Remove any user feedback
//
//----------------------------------------------------------------------------

void
CDetailFrame::DragHide()
{
    CCursorTracker *    pTrack = NULL;

    Assert(_pDoc);
// BUGBUG (garybu) DESink removed from form
//    _pDoc->_pDESink->GetTracker((void **)&pTrack);
    if ( pTrack && _pDoc->_fNonOverlapping )
    {
        HWND hWnd = _pDoc->_pInPlace->_hwnd;
        HDC hDC = ::GetDC(hWnd);

        ((CGridDragTracker *)pTrack)->DragHide(hDC);

        ::ReleaseDC(hWnd, hDC);
    }
    else
    {
        super::DragHide();
    }
}



//+---------------------------------------------------------------
//
//  Member:     CDetailFrame::DrawDragFeedback
//
//  Synopsis:   Draw user feedback
//
//  Arguments   None
//
//  Returns     Nothing
//---------------------------------------------------------------
void
CDetailFrame::DrawDragFeedback(void)
{
    TraceTag((tagDetailFrame,"CDetailFrame::DrawDragFeedback"));

    Assert(_pDoc);

    if (! _pDoc->_fNonOverlapping)
        super::DrawDragFeedback();

}


#endif PRODUCT_97

//+---------------------------------------------------------------------------
//
//  Member:     UpdatePropertyChanges
//
//  Synopsis:   checks if Tbag contains information
//              about changed properties
//              if called on the template updates instances in the recycle list
//              if called on the instance, updates the instance
//  Argument:   BOOL fCompleted: this is a two pass function. First pass updates
//              after all updates, the bags get emptied
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT CDetailFrame::UpdatePropertyChanges(UPDATEPROPS updFlag)
{
    TraceTag((tagDetailFrame,"CDetailFrame::UpdatePropertyChanges"));

    HRESULT hr = S_OK;
    CTBag * pTBag = TBag();
    Assert(pTBag);


    switch (updFlag)
    {
        case UpdatePropsPrepareTemplates:
            if (pTBag->_propertyChanges.NeedCreateToFit())
            {
                CDataFrame *pRoot = RootFrame(this);
                Assert(pRoot);
                pRoot->TBag()->_propertyChanges.SetCreateToFit(TRUE);
                getTemplate()->_fIsDirtyRectangle = TRUE;
            }
            if (pTBag->_propertyChanges.IsTemplateModified())
            {
                // BUGBUG: currently, we will always regenerate when this happens
                // so we propagate this one up....
                CDataFrame *pRoot = RootFrame(this);
                Assert(pRoot);
                pRoot->TBag()->_propertyChanges.SetRegenerate(TRUE);
#if TEST
                // The next two lines are only here to trigger
                //  the CalcProposedPos path before CreateToFit
                //  in ::Generate()
                CRectl      rcl;
                pRoot->GetProposed(pRoot, &rcl);
                pRoot->SetProposed(pRoot, &rcl);
#endif
                getTemplate()->_fIsDirtyRectangle = TRUE;
            }
            else if (pTBag->_propertyChanges.IsDirty())
            {
                // the stuff was called on the instance, so we should inspect the properties
                //  and do the right stuff.
                DISPID  dispid;
                pTBag->_propertyChanges.EnumReset();
                while (pTBag->_propertyChanges.EnumNextDispID(&dispid))
                {
                    Assert(!(dispid>=DISPID_TemplateModifyStart && dispid <= DISPID_TemplateModifyEnd) && "NYI: The instance is not able to modify it's structure");
                    SetPropertyFromTemplate(dispid);
                }
            }
            break;

        case UpdatePropsEmptyBags:
            Assert(_fOwnTBag);
            if (pTBag->_propertyChanges.IsTemplateModified())
            {
                // called on the template, update the recycle bin
                // BUGBUG: if the template was changed dramatically, like
                //  controls are added/deleted, we are killing those guys
                //  right now. That should be changed
                EmptyRecycle();
            }
            pTBag->_propertyChanges.Passivate();

            //
            //  BUGBUG: Needs a flag indicating that sites were in fact
            //          moved/resized, recalc only then.
            //
            OptimizeBackgroundPainting();
            break;

        case UpdatePropsCloseAction:
            Assert(_fOwnTBag);
            _fIsDirtyRectangle = FALSE;
            SetDirtyBelow(FALSE);
            break;



    }

    // now iterate the children
    ULONG   cChild;
    cChild = _arySites.Size();
    CSite   **ppSite;

    for (ppSite = _arySites;
         cChild > 0;
         cChild--, ppSite++)
    {
        hr = (*ppSite)->UpdatePropertyChanges(updFlag);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}
//-+ End of Method-------------------------------------------------------------


//----------------------------------------------------------------------------
//
//  Member:     BuildInstance
//
//  Synopsis:   To build a layout instance  and build the binding information
//              for the control instances.
//  SideEffect: Sets the CreateInfo sturcture for the subframes and calls
//              CFrame::Build
//
//  Arguments:  pNewInstance            newly created repeater frame.
//              info                    repeater's CreateInfo structure.
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT CDetailFrame::BuildInstance (CDetailFrame * pNewInstance)
{
    TraceTag((tagDetailFrame, "CDetailFrame::BuildInstance"));



    CCreateInfo childInfo;
    childInfo._pBindSource = pNewInstance;
    childInfo._fSetBindid = FALSE;
    RRETURN(super::BuildInstance (pNewInstance, &childInfo));
}



//+------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Detaches the frame, removing any internal self-
//              references.
//
//-------------------------------------------------------------------------

void
CDetailFrame::Detach ()
{
    Assert(!_fSelected);

    TraceTag((tagDetailFrame, "CDetailFrame::Detach"));

    if (!_fOwnTBag && _hrow != NULL)
    {
        CDataLayerCursor *pCursor;

        getOwner()->LazyGetDataLayerCursor(&pCursor);
        if (pCursor != NULL)
        {
            pCursor->ReleaseRows(1, &_hrow);
            _hrow = NULL;
        }
    }

    #if PRODUCT_97
    if (_fOwnTBag)
    {
        CMatrix * pMatrix = TBag()->_pMatrix;
        if (pMatrix)
        {
            pMatrix->Detach();
            pMatrix->Release();
            pMatrix = NULL;
        }
        EmptyRecycle();
    }
    #endif

#if FIXED
    // Reset the seletion tree's pointer to here.
    if ( _pSelectionElement )
        _pSelectionElement->pDetailFrame = NULL;
#endif

    super::Detach();
}



//+---------------------------------------------------------------------------
//
//  Member:     GetData
//
//  Synopsis:   Get Data from the Binding Source (Ole Cursor) for the subframes.
//              Called from the control subframes.
//
//  Arguments:  dla     is the data layer accessor.
//              lpv     pointer to destination.
//
//  Returns:    Returns S_OK if everything is fine, E_UNEXPECTED if layout is
//              not bound but subframes are bound.
//
//----------------------------------------------------------------------------

HRESULT CDetailFrame::GetData (CDataLayerAccessor * pDla, LPVOID lpv)
{
    TraceTag((tagDetailFrame, "CDetailFrame::GetData"));


    // this operation should be only on the instance, therefore
    Assert (!_fOwnTBag);

    HRESULT hr = S_OK;

    if (pDla && (_hrow != NULL))
    {
        hr = pDla->GetValue(_hrow, lpv);
        if (hr)
        {
            hr = S_FALSE;   // BUGBUG: rich error ?
        }
    }

    RRETURN1(hr, S_FALSE);
}




//+---------------------------------------------------------------------------
//
//  Member:     SetData
//
//  Synopsis:   Set the Layout Data (HROW?). Private:
//
//  Arguments:  dla     is the data layer accessor
//              lpv     pointer to source
//
//  Returns:    Returns E_UNEXPECTED.
//
//----------------------------------------------------------------------------

HRESULT CDetailFrame::SetData (CDataLayerAccessor * pDla, LPVOID lpv)
{
    TraceTag((tagDetailFrame, "CDetailFrame::SetData"));


    // this operation should be only on the instance, therefore
    Assert (!_fOwnTBag);

    HRESULT hr;

    if (pDla && (_hrow != NULL))
        hr = pDla->SetValue(_hrow, lpv);
    else
        hr = E_UNEXPECTED;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrame::HandleMessage, public
//
//  Synopsis:   Turns the mouseclick into a selection event
//              in the listbox case
//
//  Arguments:  As per wndproc
//
//  Returns:    S_FALSE if not handled,
//              S_OK    if processed
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CDetailFrame::HandleMessage(CMessage *pMessage, CSite *pChild)
{
    HRESULT hr = S_FALSE;

    if ( pMessage->message >= WM_MOUSEFIRST &&
         pMessage->message <= WM_MOUSELAST &&
         (getOwner()->TBag()->_eListBoxStyle || ! _pDoc->_fDesignMode) )
    {
        if (pChild)
        {
            hr = THR(_pParent->HandleMessage(pMessage, this));
        }
    }
    else
    {
        hr = THR(super::HandleMessage(pMessage, pChild));
    }

    RRETURN1(hr, S_FALSE);
}



//+------------------------------------------------------------------------
//
//  Member:     CDetailFrame::PaintSelectionFeedback
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
CDetailFrame::PaintSelectionFeedback(CFormDrawInfo *pDI, RECT *prc, DWORD dwSelInfo)
{
    if ( ! getOwner()->TBag()->_eListBoxStyle )
        super::PaintSelectionFeedback(pDI, prc, dwSelInfo);
}





//+------------------------------------------------------------------------
//
//  Member:     CBaseFrame::SuppressControlBorders
//
//  Synopsis:   Changes the controls' borders in the listbox case.
//
//  Arguments:  fSelect     the selected state
//
//  Returns     Nothing
//
//  note:       MordControl specific!!
//
//-------------------------------------------------------------------------

void
CDetailFrame::SuppressControlBorders(BOOL fSuppress)
{
    HRESULT hr;
    CSite ** ppSite;
    IMorphDataControl * pMordControl;
    int c;

    Assert(_fOwnTBag);

    for ( ppSite = _arySites, c = _arySites.Size(); --c >= 0;  ppSite++ )
    {
        hr = (*ppSite)->QueryInterface(IID_IMorphDataControl, (void **) &pMordControl);
        if ( ! hr )
        {
            pMordControl->SetBordersSuppress(fSuppress);
            pMordControl->Release();
        }
    }

}




//+------------------------------------------------------------------------
//
//  Member:     CDetailFrame::SelectSite
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
CDetailFrame::SelectSite(CSite * pSite, DWORD dwFlags)
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
            if (!_pSelectionElement)
            {
                hr = THR(AddToSelectionTree(dwFlags));
                if (hr)
                    goto Cleanup;
                //  set the bit indicating layoutframe selection
                _pSelectionElement->aryfControls.SetReservedBit(
                        SRB_LAYOUTFRAME_SELECTED);
                Verify(!SetSelected(TRUE));
            }
            break;
        case SS_REMOVEFROMSELECTION:
            if (_pSelectionElement)
            {
                hr = THR(RemoveFromSelectionTree(dwFlags));
                if (hr)
                    goto Cleanup;
                if (_pSelectionElement)
                {
                    _pSelectionElement->aryfControls.ResetReservedBit(
                            SRB_LAYOUTFRAME_SELECTED);
                }
                Verify(!SetSelected(FALSE));
            }
            break;
        case SS_SETSELECTIONSTATE:
            if (dwFlags & SS_CLEARSELECTION)
            {
                SetSelectionElement(NULL);
            }
            hr = super::SelectSite(pSite, dwFlags);
            break;
        default:
            Assert(FALSE && "These should be exclusive cases");
        }
    }
    else
    {
        switch(dwFlags & (SS_ADDTOSELECTION | SS_REMOVEFROMSELECTION | SS_SETSELECTIONSTATE))
        {
        case SS_ADDTOSELECTION:
            hr = THR(SelectSubtree(pSite, dwFlags));
            if (hr)
                goto Cleanup;
            break;
        case SS_REMOVEFROMSELECTION:
            hr = THR(DeselectSubtree(pSite, dwFlags));
            if (hr)
                goto Cleanup;
            break;
        case SS_SETSELECTIONSTATE:
            SetSelectionState(pSite);
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
//  Member      CDetailFrame::SetSelectionState
//
//  Synopsis    Sets the selection state according to the selection tree
//
//  Arguments   [pSite]   site pointer
//
//  Returns     nothing
//
//----------------------------------------------------------------------------

void
CDetailFrame::SetSelectionState(CSite * pSite)
{
    int iIndex = pSite->getIndex();

    TraceTag((tagDetailFrame, "CDetailFrame::SetSelectionState(pSite)"));


    Assert((iIndex >= 0) && "Invalid control index");

    if ( _pSelectionElement &&
         _pSelectionElement->aryfControls[iIndex])
    {
        Verify(!pSite->SetSelected(TRUE));
#if PRODUCT_97
        if (pSite->TestClassFlag(SITEDESC_DATAFRAME))
        {
            PLY * pPly;

            pPly = _pSelectionElement->arySubLevels.Find(iIndex);
            if ( pPly )
            {
                Assert(pPly->parySelectors);
                ((CDataFrame *)pSite)->SetSelectedQualifiers(pPly->parySelectors);
            }
        }
#endif PRODUCT_97
        return;
    }

    // if we get here the pNewInstance not supposed to be selected
    if (pSite->_fSelected)
    {
        Verify(!pSite->SetSelected(FALSE));
    }
    if (pSite->TestClassFlag(SITEDESC_DATAFRAME))
    {
        ((CDataFrame *)pSite)->SetSelectedQualifiers(NULL);
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrame::SelectChild
//
//  Synopsis:   Merge the child  into the datadoc
//              selection tree. Add this layout to the tree if it is not already
//              there. Continue up the tree if necessary.
//
//  Arguments:  pSite   site to add
//
//
//  Returns:    S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT
CDetailFrame::SelectSubtree(CSite * pSite, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    int iIndex = pSite->getIndex();

    TraceTag((tagDetailFrame,"CDetailFrame::SelectSubtree"));

    Assert(dwFlags & SS_ADDTOSELECTION);

    Assert((iIndex >= 0) && "Invalid control index");
    SelectUnit * psuAnchor = getOwner()->getAnchor();

    hr = THR(AddToSelectionTree(dwFlags));
    if ( hr )
        goto Error;

    //  set the bit indicating the selected control
    _pSelectionElement->aryfControls.Set(iIndex);

    // BUGBUG this will have to be revisited for hierarchical dataframes (istvanc)
    if ((dwFlags & SS_MERGESELECTION) && psuAnchor->qStart.IsValid())
    {
        hr = _pSelectionElement->Merge(psuAnchor);
        if (hr)
            goto Error;
    }
    else
    {
        // BUGBUG we should do this in ClearSelection ! (istvanc)
        psuAnchor->aryfControls.Clear();
        psuAnchor->aryfControls.Set(iIndex);
    }

#if PRODUCT_97
    if (pSite->TestClassFlag(SITEDESC_DATAFRAME))
    {
        PLY ply;

        if ( ! _pSelectionElement->arySubLevels.Find(iIndex) )
        {
            ply.idx = iIndex;
            ply.parySelectors = ((CDataFrame *)pSite)->GetSelectedQualifiers();
            hr = THR(_pSelectionElement->arySubLevels.AppendIndirect(&ply));
            if ( hr )
                goto Error;
        }
    }
#endif PRODUCT_97

Cleanup:
    RRETURN1(hr, S_FALSE);

Error:
    goto Cleanup;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrame::AddToSelectionTree
//
//  Synopsis:   Add this layout to the selection tree as a new path
//
//
//  Returns:    S_OK if everything is fine
//              E_OUTOFMEMORY if allocations fail
//              or propagates errors from OLE-DB
//
//----------------------------------------------------------------------------

HRESULT
CDetailFrame::AddToSelectionTree(DWORD dwFlags)
{
    HRESULT hr = S_OK;

    TraceTag((tagDetailFrame,"CDetailFrame::AddToSelectionTree"));

    Assert(dwFlags & SS_ADDTOSELECTION);

    if (_pSelectionElement)
        goto Cleanup;

    _pSelectionElement = new SelectUnit;
    if ( ! _pSelectionElement )
        goto MemoryError;

    hr = THR(GetQualifier(&_pSelectionElement->qStart));
    if ( hr )
        goto Error;

    Assert(getOwner());
    hr = THR(getOwner()->SelectSite(this, dwFlags));
    if (hr)
        goto Error;

Cleanup:
    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

Error:
    delete _pSelectionElement;
    _pSelectionElement = NULL;
    goto Cleanup;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrame::DeselectSubtree
//
//  Synopsis:   Remove the child from the datadoc
//              selection tree. Remove this site from the tree if none
//              of its children or itself is selected.
//              Continue up the tree if necessary.
//
//  Arguments:  pSite       site to deselect
//
//  Returns:    S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT
CDetailFrame::DeselectSubtree(CSite * pSite, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    int iIndex = pSite->getIndex();

    TraceTag((tagDetailFrame,"CDetailFrame::DeselectSubtree"));

    Assert(dwFlags & SS_REMOVEFROMSELECTION);

    Assert((iIndex >= 0) && "Invalid control index");

    //  BUGBUG: If it is a slected range based on an ordered bookmark range,
    //          this code will deselect all the instances of the control
    //          in that range. To prevent that we should split the selection
    //          range into three. Do we want to?

    //  Reset the bit indicating the selected control
    _pSelectionElement->aryfControls.Reset(iIndex);

#ifndef PRODUCT_97
    if (pSite->TestClassFlag(SITEDESC_DATAFRAME))
    {
        PLY ply;
        void * p;

        // BUGBUG who owns at this point the parySelectors ? (istvanc)
        if ( NULL != (p = _pSelectionElement->arySubLevels.Find(iIndex)) )
        {
            ply.idx = iIndex;
            ply.parySelectors = ((CDataFrame *)pSite)->GetSelectedQualifiers();
            _pSelectionElement->arySubLevels.DeleteByValueIndirect(p);
        }
    }
#endif PRODUCT_97

    hr = THR(RemoveFromSelectionTree(dwFlags));

    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrame::RemoveFromSelectionTree
//
//  Synopsis:   Remove this layout from the selection tree
//
//
//  Returns:    S_OK if everything is fine
//              E_OUTOFMEMORY if allocations fail
//              or propagates errors from OLE-DB
//
//----------------------------------------------------------------------------
HRESULT
CDetailFrame::RemoveFromSelectionTree(DWORD dwFlags)
{
    HRESULT hr = S_OK;
    SelectUnit su;

    TraceTag((tagDetailFrame,"CDetailFrame::RemoveFromSelectionTree"));

    Assert(dwFlags & SS_REMOVEFROMSELECTION);

    // BUGBUG this won't work when a control AND the row is selected
    // because deselecting the control would remove the row from
    // the selection tree fix for 97 (istvanc)
    if ( (_pSelectionElement->arySubLevels.Size()) ||
         !_pSelectionElement->aryfControls.IsEmpty() )
        goto Cleanup;

    //  get own qualifier, build own single-record selector from it
    //  call up the chain to deselect self from the dataframe's selector list
    //      this would could cause selector splitting in the dataframe
    //  unhook selector<-->layout
    //  bound? need to call with pSelector

    su.pqEnd = NULL;
    su.Clear();

    hr = GetQualifier(&su.qStart);
    if ( hr )
        goto Error;

    // replace pointer inside to allow match in dataframe
    _pSelectionElement = &su;
    Assert(getOwner());
    hr = THR(getOwner()->SelectSite(this, dwFlags));
    if ( hr )
        goto Error;

    // BUGBUG have to revisit ownership of pointers (istvanc)
    // delete _pSelectionElement; deleted in DeselectSubtree
    _pSelectionElement = NULL;      //  Do I still need this or can I trust BuildSelection() ?

Cleanup:
    RRETURN(hr);

Error:
    goto Cleanup;
}




//+------------------------------------------------------------------------
//
//  Member:     CDetailFrame::SetSelected
//
//  Synopsis:   Sets the selection flag for a single site.  Invalidates
//              the border, but does not fire events or update the selected
//              list.
//
//              May be overridden by derived classes, but should always
//              be called as part of derived implementations.
//
//  Arguments:  [fSelected]         -- New state for flag.
//
//-------------------------------------------------------------------------

HRESULT
CDetailFrame::SetSelected(BOOL fSelected)
{
    HRESULT     hr;
    BOOL fState = ENSURE_BOOL(_fSelected);

    hr = super::SetSelected(fSelected);
    if (hr)
        goto Cleanup;

    //  If we have the listbox style, then we need to do the
    //    appropriate color mangling to draw listbox-style
    //    selection feedback

    if (fSelected != fState)
    {
        fmListBoxStyles eListBoxStyle;

        eListBoxStyle = (fmListBoxStyles) getOwner()->TBag()->_eListBoxStyle;
        if (eListBoxStyle != 0 || getTemplate()->_fPaintBackground)
        {
            Assert(_pParent->TestClassFlag(SITEDESC_BASEFRAME));
            ((CBaseFrame*)_pParent)->_fSelectionChanged = TRUE;

            //  Invalidate is necessary to udpate the row's background color
            if (!_fInvalidated)
            {
                Invalidate(NULL, 0);
            }

            if (eListBoxStyle)
            {
                int             c;
                CSite **        ppSite;
                COleDataSite *  pDS;
                VARIANT         varTemp;

                for (ppSite = _arySites, c = _arySites.Size();
                     --c >= 0;
                     ppSite++ )
                {
                    if ( (*ppSite)->TestClassFlag(SITEDESC_OLEDATASITE) &&
                         !((COleDataSite*)(*ppSite))->_fFakeControl )
                    {
                        pDS = (COleDataSite*)*ppSite;
                        Assert(pDS);
                        varTemp.boolVal = fSelected;                       // Pass fSelected to
                        pDS->GetMorphInterface()->DebugHook(102254 + 1, // set fUseHighlightColor
                                                            &varTemp);
                    }
                }
            }
        }
    }

Cleanup:

    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrame::GetQualifier
//
//  Synopsis:   Get the qualifier of the layout's currency. If the layout
//              is unbound, it sets the qualifier accordingly.
//
//  Arguments:  pq      pointer to a QUALIFIER.
//
//  Returns:    Returns S_OK if everything is fine.
//              E_INVALIDARG if pCurrency is NULL.
//
//----------------------------------------------------------------------------

HRESULT CDetailFrame::GetQualifier (QUALIFIER * pq)
{
    TraceTag((tagDetailFrame, "CDetailFrame::GetQualifier"));

    CDataLayerCursor * pCursor;
    HRESULT hr = S_OK;

    if ( ! pq )
        return E_INVALIDARG;


    hr = THR(getOwner()->LazyGetDataLayerCursor(&pCursor));
    if ( hr || !_hrow )
    {
        pq->type = _fOwnTBag ? QUALI_DETAIL : QUALI_UNBOUND_LAYOUT;
        return S_OK;
    }

    hr = pCursor->CreateBookmark(getOwner()->getGroupID(), _hrow,
                                 &pq->bookmark );
    if ( hr )
        goto Error;

    pq->type = QUALI_BOOKMARK;
    hr = S_OK;

Cleanup:

    RRETURN(hr);

Error:
    pq->type = QUALI_UNKNOWN;

    goto Cleanup;
}




//+---------------------------------------------------------------------------
//
//  Member:     GetRecordNumber
//
//  Synopsis:   Get the record number of the detail frame
//
//  Arguments:  pulRecordNumber
//
//----------------------------------------------------------------------------

HRESULT
CDetailFrame::GetRecordNumber (ULONG *pulRecordNumber)
{
    return getOwner()->GetRecordNumber(_hrow, pulRecordNumber);
}



BOOL
CDetailFrame::EnsureHRow ()
{
    BOOL fEnsure = TRUE;

    if (!_hrow)
    {
        CDataLayerCursor    *   pCursor;
        CDataFrame          *   pdfr = getOwner();
        HRESULT hr = THR(pdfr->LazyGetDataLayerCursor(&pCursor));
        if (SUCCEEDED(hr))
        {
            const CDataLayerBookmark &rdlb =
                CDataLayerBookmark::ChapterSimple(pdfr->getGroupID(),
                                                  CDataLayerBookmark::TheLast );
            hr = pCursor->NewRowAfter(rdlb, &_hrow);
            if (SUCCEEDED(hr))
            {
                Assert (_hrow);
            }
            else
            {
                fEnsure = FALSE;
            }
        }
        else
        {
            fEnsure = FALSE;
        }
    }
    return fEnsure;
}





//+---------------------------------------------------------------------------
//
//  Member:     CSite::CDetailFrame, CSite
//
//  Synopsis:   Is this site visible?
//
//  Returns:    BOOL
//
//----------------------------------------------------------------------------

BOOL
CDetailFrame::IsVisible( )
{
    if (_pParent->IsVisible())
    {
        return (_fOwnTBag ? getOwner()->_pDetail == this : _fVisible);
    }

    return FALSE;
}






//+----------------------------------------------------------------------------
//
//  CDetailFrameInstance implementation
//
//----------------------------------------------------------------------------






// Cloning Constructor of CBaseFrame from the CBaseFrameTemplate.
CDetailFrameInstance::CDetailFrameInstance(
        CDoc * pDoc,
        CSite * pParent,
        CDetailFrame * pTemplate)
    : super(pDoc, pParent, pTemplate)
{
}



//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrameInstance destructor
//
//+---------------------------------------------------------------------------

CDetailFrameInstance::~CDetailFrameInstance()
{
}






//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrameInstance::EnsureIBag
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
CDetailFrameInstance::EnsureIBag()
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





//+----------------------------------------------------------------------------
//
//  CDetailFrameTemplate implementation
//
//----------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  CDetailFrameControls implementation
//
//-------------------------------------------------------------------------

/*
IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(CDetailFrameControls, CDetailFrameTemplate, _Controls)


STDMETHODIMP
CDetailFrameControls::QueryInterface(REFIID iid, LPVOID * ppv)
{
    //  BUGBUG we only have to provide this because it's declared
    //    by the subobject macro; this is a little silly

    return CControls::QueryInterface(iid, ppv);
}


CSite *
CDetailFrameControls::ParentSite( )
{
    return MyCDetailFrameTemplate();
}
*/


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
CDetailFrame::UpdatePosRect1 ()
{
    // do CreateToFit in the client rectangle
    CRectl rclDataFrame;

    getOwner()->GetDetailSectionRectl (&rclDataFrame);
    HRESULT hr = CreateToFit (&rclDataFrame, SITEMOVE_NOFIREEVENT);
    RRETURN1 (hr, S_FALSE);
}



//+---------------------------------------------------------------------------
//
//  Member:     ScrollBy, public
//
//  Synopsis:   Scroll the detail frame by
//
//  Arguments:  dxl     X delta to move in himetric
//              dyl     Y delta to move in himetric
//              xAction Horizontal scroll action
//              yAction Vertical scroll action
//
//----------------------------------------------------------------------------

HRESULT
CDetailFrame::ScrollBy(
        long dxl,
        long dyl,
        fmScrollAction xAction,
        fmScrollAction yAction)
{
    // control is trying to scroll itself into the view, since detail frame
    // is always enclosing the child controls we don't have to do anything.
    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDetailFrame::GetClientRectl (virtual)
//
//  Synopsis:   Get content area.
//
//----------------------------------------------------------------------------

void
CDetailFrame::GetClientRectl (RECTL *prcl)
{
    if (_pParent->TestClassFlag(SITEDESC_REPEATER))
    {
        *prcl = _rcl;
    }
    else
    {
        CRectl  rcl;
        getOwner()->GetDetailSectionRectl (&rcl);
        ((CRectl *)prcl)->Intersect(&rcl, &_rcl);
    }
    return;
}






//
//  end of file
//
//----------------------------------------------------------------------------
