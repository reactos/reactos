//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       oledsite.cxx
//
//  Contents:   Implementation of DataDoc control site
//
//  Classes:    COleDataSite
//
//  Maintained by IstvanC
//
//----------------------------------------------------------------------------


#define _OLEDSITE_CXX_   1

#include "headers.hxx"
#include "dfrm.hxx"
#include "cdatadsp.hxx"

DeclareTag(tagOleDataSite,"src\\ddoc\\datadoc\\oledsite.cxx","Ole Data Site");
DeclareTag(tagDataBind,"olecfrm.cxx","IDispatch ONLY");

#if PRODUCT_97
PROP_DESC COleDataSiteTemplate::s_apropdesc[] =
{
    PROP_MEMBER(CSTRING,
                COleDataSiteTemplate,
                _TBag._cstrName,
                NULL,
                "Name",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(CSTRING,
                COleDataSiteTemplate,
                _IBag._cstrTag,
                NULL,
                "Tag",
                PROP_DESC_NOBYTESWAP)
    PROP_MEMBER(LONG,
                COleDataSiteTemplate,
                _TBag._ID,
                0,
                "ID",
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(LONG,
                COleDataSiteTemplate,
                _dwHelpContextID,
                0,
                "HelpContextID",
                PROP_DESC_BYTESWAPLONG)
 //   PROP_MEMBER(LONG,
 //               COleDataSiteTemplate,
 //               _ulBitFlags,
 //               SITE_FLAG_DEFAULTVALUE,
 //               "BitFlags",
 //               PROP_DESC_BYTESWAPLONG)
    PROP_VARARG(LONG,
                sizeof(DWORD),
                0,
                "ObjectSize",
                PROP_DESC_BYTESWAPLONG)
    PROP_MEMBER(SHORT,
                COleDataSiteTemplate,
                _TBag._wclsid,
                (ULONG)WCLSID_INVALID,
                "CLSIDCacheIndex",
                PROP_DESC_BYTESWAPSHORT)
    PROP_MEMBER(SHORT,
                COleDataSiteTemplate,
                _TBag._usTabIndex,
                (ULONG)-1L,
                "TabIndex" ,
                PROP_DESC_BYTESWAPSHORT)
    // // We only want to store the position, not the size //
    PROP_CUSTOM(WPI_USERDEFINED,
                sizeof(POINTL),
                offsetof(COleDataSiteTemplate, _rcl),
                NULL,
                "Position" ,
                PROP_DESC_BYTESWAPPOINTL)

    // the above stuff is copied from the CSite implementation

    PROP_MEMBER(CSTRING,
                COleDataSiteTemplate,
                _TBag._cstrControlSource,
                NULL,
                "ControlSource",
                PROP_DESC_NOBYTESWAP)
    PROP_NOPERSIST(CSTRING, sizeof(CStr), "HeaderControl")   // was SIZE_AND_OFFSET_OF(COleDataSiteTemplate, _TBag._cstrHeaderControl)
    PROP_NOPERSIST(CSTRING, sizeof(CStr), "FooterControl")   // was SIZE_AND_OFFSET_OF(COleDataSiteTemplate, _TBag._cstrFooterControl)
    PROP_NOPERSIST(LONG, sizeof(long), NULL)   // Old WhatsThisHelpID
    PROP_MEMBER(CSTRING,
                COleDataSiteTemplate,
                _TBag._cstrControlTipText,
                NULL,
                "ControlTipText",
                PROP_DESC_NOBYTESWAP)
    PROP_NOPERSIST(CSTRING, sizeof(CStr), NULL,)   // Old StatusBarText
    PROP_VARARG(LONG,
                sizeof(LONG),
                -1,
                "RelatedAbove_ID",
                PROP_DESC_BYTESWAPLONG)
    //
    // We only want to store the the bottom right corner for Record selector sites
    //  (they have no associated control that could store the extents).
    //
    PROP_VARARG(LONG,
                sizeof(LONG),
                0L,
                "Right",
                PROP_DESC_BYTESWAPLONG)
    //
    // We only want to store the the bottom right corner for Record selector sites
    // (they have no associated control that could store the extents).
    //
    PROP_VARARG(LONG,
                sizeof(LONG),
                0L,
                "Bottom",
                PROP_DESC_BYTESWAPLONG)
    // iff none of the two properties "ControlSource" and "RowSource" have a
    // non-null value we don't need a CDataLayer (DLAS) in CSite::TBag.  So we
    // get these two using vararg's.
    // sizeof a pointer, offset immaterial
    PROP_VARARG(CSTRING,
                sizeof(CStr),
                NULL,
                "ControlSource",
                PROP_DESC_NOBYTESWAP)
    // sizeof a pointer, offset immaterial
    PROP_VARARG(CSTRING,
                sizeof(CStr),
                NULL,
                "RowSource" ,
                PROP_DESC_NOBYTESWAP)
};


const CLSID * COleDataSite::s_aclsidPages[4] =
{
    &CLSID_CCDGenericPropertyPage,
    &CLSID_CControlPropertyPage,
    &CLSID_CActivationPropertyPage,
    &CLSID_CControlSourcePropertyPage,
    NULL
};
#endif PRODUCT_97


CSite::CLASSDESC COleDataSiteTemplate::s_classdesc =
{
    {
        {
            &CLSID_CCDControl,              // _pclsid
            0,                              // _idrBase
#if PRODUCT_97
            s_aclsidPages,                  // _aClsidPages
#else
            NULL,
#endif
            ARRAY_SIZE(s_acpi),             // _ccp
            s_acpi,                         // _pcpi
#if PRODUCT_97
            ARRAY_SIZE(s_apropdesc),        // _cpropdesc
            s_apropdesc,                    // _ppropdesc
#else
            0,
            0,
#endif PRODUCT_97
            SITEDESC_OLEDATASITE |              // _dwFlags
            SITEDESC_TEMPLATE |
            SITEDESC_OLESITE,
            &IID_IControlElement,           // _piidDispinterface
            0,                              // _lAccRole
            &CSite::s_PropCats,             // _pPropCats;
            NULL
        },
        s_apfnIControlElement,                     // _pfnTearOff
    },
    ETAG_OLEDATASITE,                     // _st
};


CSite::CLASSDESC COleDataSiteInstance::s_classdesc =
{
    {
        {
            &CLSID_CCDControl,              // _pclsid
            0,                              // _idrBase
#if PRODUCT_97
            s_aclsidPages,                  // _aClsidPages
#else
            NULL,
#endif
            ARRAY_SIZE(s_acpi),             // _ccp
            s_acpi,                         // _pcpi
            0,                              // _cpropdesc
            NULL,                           // _ppropdesc
            SITEDESC_OLEDATASITE |
            SITEDESC_OLESITE,               // _dwFlags
            &IID_IControlElement,                  // _piidDispinterface
            0,                              // _lAccRole
            &CSite::s_PropCats,             // _pPropCats;
            NULL
        },
        s_apfnIControlElement,                 // _pfnTearOff
    },
    ETAG_OLEDATASITE,                     // _st
};



COleDataSite::COleDataSiteTBag::COleDataSiteTBag() :
    _cstrControlSource(CSTR_NOINIT)
{
    _fDispatchType          = BIT_None;
    _uiAccessorColumn       = (UINT) -1;
    _pRelated               = (COleDataSite*)-1;
}


COleDataSite::COleDataSiteTBag::~COleDataSiteTBag()
{
    CDLAConcrete *pdlac = _dl.getpDLAConcrete();

    if (pdlac)
    {
        pdlac->Passivate();
        delete pdlac;
#if DBG == 1
        _dl.SetpDLAConcrete(NULL);
#endif
    }
}


//+---------------------------------------------------------------
//
//  Member:     COleDataSite::COleDataSite, public
//
//  Synopsis:   create new Data Site
//
//  Effects:    creates new site
//
//  Arguments:  [pParent] --  parent
//
//  Notes:      can not fail
//
//---------------------------------------------------------------

COleDataSite::COleDataSite (CSite * pParent)
        :super(pParent, ETAG_NULL)
{
    _fOwnTBag = TRUE;
    _pTemplate = this;
    Assert(_pBindSource == NULL);
    Assert(_invokeIntf.pDispatch == NULL);
    _fAutoSize = FALSE;
}


//+---------------------------------------------------------------
//
//  Member:     COleDataSite::COleDataSite, public
//
//  Synopsis:   create new Data Site object from the template
//
//  Effects:    memcpy already was called during the new operator
//              (copied data from the template into this object).
//
//  Arguments:  [pfrTemplate] --  control template
//
//  Notes:      can not fail
//
//---------------------------------------------------------------

COleDataSite::COleDataSite (CSite * pParent, COleDataSite * pTemplate)
        :COleSite (pParent, pTemplate)
        //,_DBag(&pTemplate->_DBag)      BUGBUG temporary moved into COleSite
{
    _fOwnTBag = FALSE;
    _pTemplate = pTemplate;

    // this is to prevent invalidate rectangle in COleSite in OnViewChange
    // coming back from the control (istvanc)
    _fIsDirtyRectangle = TRUE;
    _pBindSource = NULL;
    _invokeIntf.pDispatch = NULL;
    _fAutoSize = FALSE;
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

void * COleDataSite::operator new (size_t s, COleSite * pOriginal)
{
    TraceTag((tagOleDataSite, "COleDataSite::operator new "));

    void * pNew = ::operator new(s);
    if (pNew)
        memcpy(pNew, pOriginal, sizeof(COleDataSite));
    return pNew;
}



COleDataSite::~COleDataSite()
{
}





//+---------------------------------------------------------------
//
//  Member:     COleDataSite::Detach
//
//  Synopsis:   Disconnect from the form.
//
//---------------------------------------------------------------

void
COleDataSite::Detach()
{
    Assert(!_fSelected);

    if (_fOwnTBag)
    {
        if (HasDLA())
        {
            CDataLayerAccessor *pdla;
            IGNORE_HR(LazyGetDLAccessor(&pdla));
            Assert(pdla);
            pdla->Passivate();
        }
        ReleaseInterface(TBag()->_pProvideInstance);
    }

    switch(TBag()->_fDispatchType)
    {
    case BIT_Morph:
        ReleaseInterface(_invokeIntf.pMorphData);
        break;
    case BIT_Disp:
        _invokeIntf.pDispatch = NULL;   // since this is _pDisp we let COleSite to release it !
        break;
    default:
        Assert( _fFakeControl && "_fDispatchType is not set");
    }

    super::Detach();
}




//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::HandleMessage, public
//
//  Synopsis:   Delegates to the owning dataframe
//
//  Arguments:  As per wndproc
//
//  Returns:    S_FALSE if not handled,
//              S_OK    if processed
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
COleDataSite::HandleMessage(CMessage *pMessage, CSite *pChild)
{
    Assert(_pParent->TestClassFlag(SITEDESC_DETAILFRAME));
    HRESULT hr = S_FALSE;

    if ( pMessage->message >= WM_MOUSEFIRST &&
         pMessage->message <= WM_MOUSELAST &&
         (((CDetailFrame*)_pParent)->getOwner()->TBag()->_eListBoxStyle) ||
            ( ! _pDoc->_fDesignMode &&
              getTemplate()->_fRecordSelector) )
    {
        if (pChild)
        {
            hr = THR(_pParent->HandleMessage(pMessage, this));
        }
    }
    else
    {
        hr = THR(super::HandleMessage(pMessage, pChild));
        if (hr == S_FALSE && pMessage->message >= WM_KEYFIRST && pMessage->message <= WM_KEYLAST)
        {
            if (pChild)
            {
                hr = THR(_pParent->HandleMessage(pMessage, this));
            }
        }
    }

    RRETURN1(hr, S_FALSE);
}




//+------------------------------------------------------------------------
//
//  Member:     COleDataSite::InsertedAt
//
//  Synopsis:   Called when a new OLE DataSite is inserted via the parent
//              site's InsertNewSite method.Calls super to do the
//              insertion, then sets _pRelated to NULL.
//              _pRelated is -1 for loading.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
COleDataSite::InsertedAt(
    CSite * pParent,
    REFCLSID clsid,
    LPCTSTR pstrName,
    const RECTL * prcl,
    DWORD dwOperations)
{
    HRESULT  hr;

    hr = THR(super::InsertedAt(pParent, clsid, pstrName, prcl, dwOperations));
    if ( hr )
        goto Cleanup;

    Assert((long)(TBag()->_pRelated) == -1);
    TBag()->_pRelated = NULL;

Cleanup:
    RRETURN(hr);
}



HRESULT
COleDataSite::ConnectControl1(IUnknown **ppUnkCreate, DWORD *pdwInitFlags)
{
    if (((CDetailFrame*)_pParent)->getOwner()->TBag()->_eListBoxStyle)
    {
        *pdwInitFlags &= ~(FSI_INCOLLECTION | FSI_ASSIGNNAME);
    }

    RRETURN(super::ConnectControl1(ppUnkCreate, pdwInitFlags));
}




//+-------------------------------------------------------------------------
//
//  Method:     COleDataSite::SetWidth
//
//  Synopsis:   instead of calling move, like the parentsite impl
//              this one just calls proposeMove, and let the property
//              update code do the rest
//
//--------------------------------------------------------------------------
HRESULT
COleDataSite::SetWidth(long Width)
{
    CRectl rcl;

    GetProposed(this, &rcl);

    rcl.right = rcl.left + Width;

    SetProposed(this, &rcl);

    OnPropertyChange(STDPROPID_XOBJ_WIDTH,NULL);

    return S_OK;
}
//--End of Method-----------------------------------------------------------




#if defined(PRODUCT_97)
//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::WriteProps, public
//
//  Synopsis:   Calls WriteProps to persist the properties for this site.
//              the oledatasite imp is adding the two related pointers
//
//  Arguments:  [pStm]      -- Stream to write to.
//              [ulObjSize] -- Number of bytes written by associated object.
//
//  Returns:    HRESULT
//
//  Notes:      Can be overridden so that additional var-args can be passed
//              to WriteProps if desired.
//
//----------------------------------------------------------------------------

HRESULT
COleDataSite::WriteProps(IStream * pStm, ULONG * pulObjSize)
{
    HRESULT             hr;
    CBase::CLASSDESC   *pclassdesc = BaseDesc();
    COleDataSite       *pRelated = TBag()->_pRelated;
    CStr                cstrEmpty;

    Assert(_fOwnTBag);

    CDataLayerAccessorSource *pdlas;

    if (TBag()->_dl.HasDLAS())
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
                          _fStreamed ? *pulObjSize++ : 0,
                          pRelated ? pRelated->getTemplate()->TBag()->_iIndex :
                                     (ULONG)-1,
                          TestFlag(OLEDATASITE_FLAG_FAKECONTROL) ? _rcl.right :
                                                                   0L,
                          TestFlag(OLEDATASITE_FLAG_FAKECONTROL) ? _rcl.bottom :
                                                                   0L,
                          pdlas ? pdlas->GetControlSource() : cstrEmpty,
                          pdlas ? pdlas->GetRowSource()     : cstrEmpty
                         ));

    if (hr)
        goto Cleanup;

    hr = CSite::WriteProps(pStm, pulObjSize);

Cleanup:

    RRETURN(hr);
}
//-+ End of Method-------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::ReadProps, public
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
//----------------------------------------------------------------------------

HRESULT
COleDataSite::ReadProps(USHORT usPropsVer,
                      USHORT usFormVer,
                      BYTE *pb,
                      USHORT cb,
                      ULONG * pulObjSize)
{
    HRESULT             hr;
    CBase::CLASSDESC   *pclassdesc = BaseDesc();
    CStr                cstrControlSource;
    CStr                cstrRowSource;

    Assert(_fOwnTBag);

    //  we will read the _ID of the Detail into the _pDetail pointer
    //  before generation there will be a fix up run
    hr = THR(::ReadProps(usPropsVer,
                         pb,
                         cb,
                         pclassdesc->_ppropdesc,
                         pclassdesc->_cpropdesc,
                         this,
                         pulObjSize,
                         &(TBag()->_pRelated),
                         &_rcl.right,
                         &_rcl.bottom,
                         &cstrControlSource, // var arg 1
                         &cstrRowSource ));  // var arg 2
    if (hr)
        goto Cleanup;

    Assert(((_rcl.bottom == 0) && (_rcl.right == 0)) ||
           TestFlag(OLEDATASITE_FLAG_FAKECONTROL) );

    if (TBag()->_dl.HasDLAS() ||
        cstrControlSource.Length() > 0 || cstrRowSource.Length() > 0 )
    {
        CDataLayerAccessorSource *pdlas;
        hr = THR(LazyGetDLAccessorSource(&pdlas));
        if (hr)
            goto Cleanup;
        hr = THR(pdlas->SetRowSource(cstrRowSource));
        if (hr)
            goto Cleanup;
        hr = THR(pdlas->SetControlSource(cstrControlSource));
    }

Cleanup:
    RRETURN(hr);
}
//-+ End of Method-------------------------------------------------------------




//+---------------------------------------------------------------------------
//
//  Member:     AfterLoad, public
//
//  Synopsis:   Walks over the Form and adjusts pointers
//              the method should only be called after loading
//              in that case, _pRelated[x] pointer will
//              contain the _ID value for the related cell
//              So we will have to search the tree
//              to find the site with the correct ID and setup the pointers.
//
//  Returns:    HRESULT
//
//
//----------------------------------------------------------------------------
HRESULT
COleDataSite::AfterLoad(DWORD dw)
{
    HRESULT hr=E_FAIL;

    Assert(_fOwnTBag);

    // just walk over the complete form

    CTBag            *pTBag = TBag();
    CBaseFrame      *pDetailFrame;

    hr = super::AfterLoad(dw);
    if (hr)
        goto Cleanup;

    if (((ULONG)pTBag->_pRelated) != (ULONG) -1)
    {
        // so we are either a footer or header cell
        pDetailFrame = ((CBaseFrame*)_pParent)->getOwner()->getDetail();

        // the number we have stored should be this guys arrayindex
        pTBag->_pRelated = (COleDataSite*) pDetailFrame->_arySites[(ULONG)pTBag->_pRelated];
    }
    else
    {
        pTBag->_pRelated = 0;
    }



    #if DBG==1
        if (pTBag->_pRelated)
        {
            Assert(pTBag->_pRelated->TestClassFlag(SITEDESC_OLEDATASITE));
        }
    #endif

Cleanup:

    RRETURN(hr);
}
//-+ End of Method-------------------------------------------------------------
#endif

//+---------------------------------------------------------------
//
//  Member:     CDetailFrame::Notify
//
//  Synopsis:   Handle notification
//
//---------------------------------------------------------------
HRESULT
COleDataSite::Notify(SITE_NOTIFICATION sn, DWORD dw)
{
    HRESULT hr = S_OK;

#if defined(PRODUCT_97)
    switch (sn)
    {
    case SN_AFTERLOAD:
        hr = AfterLoad();
        hr = THR(super::Notify(sn, dw));
        break;

    default:
        hr = THR(super::Notify(sn, dw));
    }
#endif

    RRETURN1(hr, S_FALSE);
}



//+---------------------------------------------------------------
//
//  Member:     COleSite::TransitionTo, public
//
//  Synopsis:   Prevent UI-activation in the listbox
//
//  Arguments:  [state] -- the target state
//
//  Returns:    E_FAIL if listbox, else delegates to super::
//
//------------------------------------------------------------------------

HRESULT
COleDataSite::TransitionTo(OLE_SERVER_STATE state, LPMSG pMsg)
{
    HRESULT         hr;

    Assert(_pParent && _pParent->TestClassFlag(SITEDESC_DETAILFRAME));

    if ( ((state >= OS_UIACTIVE) && ((CDetailFrame*)_pParent)->getOwner()->TBag()->_eListBoxStyle) ||
         (_fFakeControl && (state > OS_INPLACE)) )
    {
        hr = E_FAIL;
    }
    else
    {
        hr = THR(super::TransitionTo(state, pMsg));
    }

    RRETURN1(hr,S_FALSE);
}




#if DBG == 1 || defined(PRODUCT_97)
//+------------------------------------------------------------------------
//
//  Member:     COleDataSite::SelectSite
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
// BUGBUG not used in 96
HRESULT
COleDataSite::SelectSite(CSite * pSite, DWORD dwFlags)
{
    HRESULT hr = S_OK;

//#ifndef PRODUCT_97
//    Assert(FALSE && "SHOULDN'T BE CALLED IN 96");
//#else
    Assert(pSite == this);

    // called on itself, check keep selection and call parent
    if (dwFlags & SS_CLEARSELECTION)
    {
        RootFrame(this)->ClearSelection(TRUE);
    }
    switch(dwFlags & (SS_ADDTOSELECTION | SS_REMOVEFROMSELECTION | SS_SETSELECTIONSTATE))
    {
    case SS_ADDTOSELECTION:
        hr = THR(_pParent->SelectSite(this, dwFlags));
        if (hr)
            goto Cleanup;
        Verify(!SetSelected(TRUE));
        break;
    case SS_REMOVEFROMSELECTION:
        Verify(!SetSelected(FALSE));
        hr = THR(_pParent->SelectSite(this, dwFlags));
        if (hr)
            goto Cleanup;
        break;
    case SS_SETSELECTIONSTATE:
        // set state on itself
        if (dwFlags & SS_CLEARSELECTION)
        {
            SetSelected(FALSE);
        }
        else
        {
#if PRODUCT_97
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
        break;
    default:
        Assert(FALSE && "These should be exclusive cases");
    }

Cleanup:
//#endif PRODUCT_97

    RRETURN(hr);
}
#endif //DBG == 1 || defined(PRODUCT_97)




//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::Draw
//
//  Synopsis:   Draws the object
//
//  Arguments:  hdc                 HDC to draw on.
//              prcSite             The site's rectangle.
//              dvAspect            ????
//
//----------------------------------------------------------------------------

HRESULT
COleDataSite::Draw(CFormDrawInfo *pDI)
{
    HRESULT hr = S_OK;

    if ( _fFakeControl &&
         pDI->_fAfterStartSite )
    {
        unsigned int uState;
        COLORREF colorBack;
        CRect rc(&_rc);
        CRectl rcl(_rcl);
        HBRUSH hBrush;
        RECT *prc = &_rc;

        Assert(_pParent && _pParent->TestClassFlag(SITEDESC_DETAILFRAME));
        CDataFrame::CTBag * pTBag = ((CDetailFrame*)_pParent)->getOwner()->TBag();

        if ( pTBag->_eListBoxStyle )
        {
            if ( pTBag->_eMultiSelect == fmMultiSelectSingle )
            {
                uState = DFCS_BUTTONRADIO | DFCS_FLAT;
            }
            else
            {
                uState = DFCS_BUTTONCHECK | DFCS_FLAT;
            }

            if ( _pParent->_fSelected )
            {
                uState |= DFCS_CHECKED;
            }

            colorBack = ColorRefFromOleColor(((CDetailFrame*)_pParent)->_colorBack);

            //  Fill the rectangle with the row's background color
            hBrush = GetCachedBrush(colorBack);
            if (GetCurrentObject(pDI->_hdc, OBJ_BRUSH) != hBrush)
            {
                SelectObject(pDI->_hdc, hBrush);
            }

            PatBlt(pDI->_hdc, prc->left, prc->top, prc->right - prc->left,
                prc->bottom - prc->top, PATCOPY);
            ReleaseCachedBrush(hBrush);

            //  BUGBUG: Performance: Ideally, I'd like to cache the transormed
            //          values of RECORD_SELECTOR_SIZE (13 pixels)
            //          and RECORD_SELECTOR_CLEARANCE (2 pixels).
            //          Optimal solution would be to hook into
            //          zoom change events and cache the calculated values
            //          in someone's TBag().


            //  paint the glyph 2 pixels from top/left
            rcl.OffsetRect(HimetricFromHPix(RECORD_SELECTOR_CLEARANCE),
                           HimetricFromVPix(RECORD_SELECTOR_CLEARANCE));
            //  assume 13x13 pixel rects for the glyph
            rcl.right  = rcl.left + HimetricFromHPix(RECORD_SELECTOR_SIZE);
            rcl.bottom = rcl.top  + HimetricFromVPix(RECORD_SELECTOR_SIZE);
            pDI->WindowFromDocument(&rc, &rcl);
        }
        else
        {
            uState = DFCS_BUTTONPUSH;
        }
        FormsDrawGlyph ( pDI, (LPRECT)&rc, DFC_BUTTON, uState);
    }
    else
    {
#if PAINTTEST
        HDC hdc = pDI->_hdc;

        HFONT   hFont;
        IFont * pFontDefault;

        FormsGetDefaultFont(&pFontDefault);

        pFontDefault->get_hFont(&hFont);
        if ( GetCurrentObject(hdc, OBJ_FONT ) != hFont)
            SelectObject(hdc, hFont);

        COLORREF color;

        color = ColorRefFromOleColor(((CDetailFrame*)_pParent)->_colorBack);
        if (GetBkColor(hdc) != color)
        {
            SetBkColor(hdc, color);
        }
        color = ColorRefFromOleColor(((CDetailFrame*)_pParent)->_colorFore);
        if (GetTextColor(hdc) != color)
        {
            SetTextColor(hdc, color);
        }
        ExtTextOut(hdc, prc->left, prc->top,
            ETO_CLIPPED | ETO_OPAQUE, prc,
            _cstrValue, _cstrValue.Length(), NULL);
#else
        hr = THR(super::Draw(pDI));
#endif PAINTTEST
    }

    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::CreateControl
//
//  Synopsis:   Draws the object
//
//  Arguments:  hdc                 HDC to draw on.
//              prcSite             The site's rectangle.
//              dvAspect            ????
//
//----------------------------------------------------------------------------

HRESULT
COleDataSite::CreateControl()
{
    if ( _fFakeControl )
    {
        _pUnkCtrl = (IOleObject *)&_RSControl;
        _pUnkCtrl->AddRef();

        return S_OK;
    }
    else
    {
        RRETURN(super::CreateControl());
    }
}




//+-------------------------------------------------------------------------
//
//  Method:     COleDataSite::MoveToProposed
//
//  Synopsis:   Move children to the calculated proposed positions
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
COleDataSite::MoveToProposed(DWORD dwFlags)
{

    HRESULT hr = super::MoveToProposed(dwFlags);

    if (!hr && _fIsDirtyRectangle)
    {
        ((CBaseFrame*)_pParent)->SetDirtyBelow(TRUE);
    }

    RRETURN(hr);
}

#if PRODUCT_97
//+-------------------------------------------------------------------------
//
//  Method:     COleDatasite::SetProposed
//
//  Synopsis:   Deferred Move to this rectangle
//              overwritten: if related cells exist
//                  we propose move on them as well
//              this is only done for header/footer cells
//              the detail cells don't need to propagate
//              because header/footer will pick their changes up later
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
COleDataSite::SetProposed(CSite * pSite, const CRectl * prcl)
{
    CRectl rcl;
    Edge e = (Edge) (((CBaseFrame*)_pParent)->getOwner()->IsVertical());
    Assert(e == edgeLeft || e == edgeTop);
    Edge eOpposite = (Edge) (e+2);
    CTBag   *pTBag      = TBag();
    CMatrix *pMatrix;

    BOOL    fMove  = FALSE;

    HRESULT hr = super::SetProposed(this, prcl);
    if (hr)
        goto Error;

    rcl = *prcl;
    if (pTBag->_pRelated && rcl != _rcl)
    {
        pMatrix = ((CDetailFrame*)_pParent)->TBag()->_pMatrix;

        rcl    -= *prcl;
        rcl[e] = rcl[eOpposite] = 0;
        if (!rcl.IsRectNull())
        {
            if (pMatrix)
            {
                fMove = pMatrix->IsSiteMarkedForMove(this);
            }
            hr = TBag()->_pRelated->ProposedDelta(&rcl, fMove);
            if (hr)
                goto Error;
        }
    }

Error:
    RRETURN(hr);
}
//- end-of-method------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     COleDatasite::ProposedDelta
//
//  Synopsis:   gets called by related friend sites when they are moved
//
//  Arguments:  rclDelta    the delta the friend is moved
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
COleDataSite::ProposedDelta(CRectl *prclDelta, BOOL fMove)
{

    TraceTag((tagOleDataSite,"COleDataSite::ProposedDelta"));

    HRESULT hr=S_OK;
    CRectl rcl;
    GetProposed(this, &rcl);
    CMatrix   *pMatrix = ((CDetailFrame*)_pParent)->TBag()->_pMatrix;

    if (rcl == _rcl)
    {
        rcl -= *prclDelta;
        hr = SetProposed(this, &rcl);
        if (hr)
            goto Error;
        _fProposedSet = TRUE;
        if (fMove && pMatrix)
        {
            pMatrix->MarkForMove(this);
        }
    }
Error:
    RRETURN(hr);


}
//- end-of-method------------------------------------------------------------

#endif



//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::OnPropertyChange
//
//  Synopsis:   The controls should call OnChange,
//              which wil in turn call this in case
//              of not bound properties
//
//  Arguments:  DISPID dispid   - the used property id
//              IDISPATCH pDisp - the senders IDispatch
//                  if this parameter is 0, we are dealing
//                  with one of our own properties
//
//
//  Returns:    nothing
//
//----------------------------------------------------------------------------
HRESULT
COleDataSite::OnPropertyChange(DISPID dispid, IDispatch *pDisp)
{

    TraceTag((tagOleDataSite,"COleDataSite::OnPropertyChange"));

    Assert(_fOwnTBag);
    HRESULT     hr = S_OK;


    if (TBag()->_pProvideInstance || !pDisp)
    {
        TBag()->_propertyChanges.AddDispID(dispid);
    }
    else
    {
        VARIANT     variant;
        EXCEPINFO   except;
        HRESULT     hr;

        InitEXCEPINFO(&except);
        Assert(pDisp);

        VariantInit(&variant);

        hr = THR(GetDispProp(pDisp, dispid,
                            IID_NULL, LOCALE_SYSTEM_DEFAULT,
                            &variant, &except));

        Assert(SUCCEEDED(hr));
        FreeEXCEPINFO(&except);

        TBag()->_propertyChanges.AddDispIDAndVariant(dispid,&variant);
        VariantClear(&variant);
    }

    if (SUCCEEDED(hr) && !_pDoc->_fDeferedPropertyUpdate)
    {
        hr = UpdatePropertyChanges(UpdatePropsPrepareTemplates);
    }

    RRETURN(hr);
}
//--End of Method ------------------------------------------------------------




//+---------------------------------------------------------------
//
//  Member:     COleDataSite::UpdatePropertyChanges
//
//  Synopsis:   verifies if the template is changed and
//              either: updates the instance
//              or calls the rootframe to do it everywhere
//
//  Returns     HRESULT
//
//---------------------------------------------------------------
HRESULT COleDataSite::UpdatePropertyChanges(UPDATEPROPS updFlag)
{
    TraceTag((tagOleDataSite,"COleDataSite::UpdatePropertyChanges()"));
    HRESULT hr = S_OK;

    switch (updFlag)
    {

        case UpdatePropsEmptyBags:
            TBag()->_propertyChanges.Passivate();
            break;

        case UpdatePropsPrepareTemplates:
            if (TBag()->_propertyChanges.IsDirty())
            {
                if (_fOwnTBag)
                {
                    if (TBag()->_propertyChanges.NeedCreateToFit())
                    {
                        CDataFrame *pRoot = RootFrame(this);
                        Assert(pRoot);
                        pRoot->TBag()->_propertyChanges.SetCreateToFit(TRUE);
                        pRoot->TBag()->_propertyChanges.SetNeedGridResize(TRUE);
                        getTemplate()->_fIsDirtyRectangle = TRUE;
                    }

                    if (!_pDoc->_fInstancePropagating)
                    {
                        // if we are not propagating already, then start it
                        Assert(RootFrame(this));
                        hr = RootFrame(this)->UpdatePropertyChanges(updFlag);
                    }
                }
                else
                {
                    // when called on the instance, update the instance
                    hr = UpdateProperties();
                    if (!hr)
                    {
                        Invalidate(NULL, 0);
                    }
                }
            }
            break;

        case UpdatePropsPrepareDataBase:
            hr = BindIndirect();
            break;

        case UpdatePropsCloseAction:
            Assert(_fOwnTBag);
            _fIsDirtyRectangle = FALSE;
            break;

    }
    RRETURN(hr);
}
//--End of Method ------------------------------------------------------------





//+---------------------------------------------------------------
//
//  Member:     COleDataSite::UpdateProperties
//
//  Synopsis:   verifies if the template is changes and updates the instance
//
//  Returns     HRESULT
//
//---------------------------------------------------------------
HRESULT COleDataSite::UpdateProperties(void)
{
    TraceTag((tagOleDataSite,"COleDataSite::UpdateProperties()"));
    HRESULT hr = S_OK;
    HRESULT hrRefresh = S_OK;

    DISPID  dispid;

    if (TBag()->_propertyChanges.IsDataSourceModified())
    {
        // the controlsource for the control was changed...
        hrRefresh = RefreshData();
    }

    if (TBag()->_propertyChanges.NeedCreateToFit())
    {
        CDataFrame *pRoot = RootFrame(this);
        Assert(pRoot);
        pRoot->TBag()->_propertyChanges.SetCreateToFit(TRUE);
        getTemplate()->_fIsDirtyRectangle = TRUE;
    }

    if (TBag()->_pProvideInstance)
    {
        IControlInstance *pInstance;
        // first thing to do is: set the property on the template control
        hr = THR_NOTRACE(QueryControlInterface(IID_IControlInstance,
                                       (void **)&pInstance));
        if (SUCCEEDED(hr))
        {
            TBag()->_propertyChanges.EnumReset();
            while (TBag()->_propertyChanges.EnumNextDispID(&dispid))
            {
                IGNORE_HR(pInstance->SetPropertyFromTemplate(dispid));
            }
            pInstance->Release();
        }
    }
    else
    {
        VARIANT *pvar;
        EXCEPINFO   except;
        InitEXCEPINFO(&except);

        TBag()->_propertyChanges.EnumReset();

        while (SUCCEEDED(hr) && TBag()->_propertyChanges.EnumNextDispID(&dispid))
        {
            InitEXCEPINFO(&except);
            Verify(TBag()->_propertyChanges.EnumNextVariant(&pvar));
            hr = SetDispProp(_pDisp,
                dispid,
                IID_NULL,
                LOCALE_SYSTEM_DEFAULT,
                pvar,
                &except);

            Assert(SUCCEEDED(hr));
            FreeEXCEPINFO(&except);
        }
    }

    hr = FAILED(hrRefresh) ? hrRefresh : hr;
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COleDataSite::OnControlRequestEdit, COleSite
//
//  Synopsis:   Give us a chance to veto a
//              control's OnRequestEdit request, and cancel any forwarding
//              which might normally take place through our XObject.
//
//  Arguments:  dispid of property who's value control would like to change
//
//  Returns:    S_OK: normal processing
//              S_FALSE: allow the OnRequestEdit, but don't forward
//              E_.....: cancel the OnRequestEdit, and the forwarding
//
//--------------------------------------------------------------------------

HRESULT
COleDataSite::OnControlRequestEdit(DISPID dispid)
{
    CDataLayerAccessor *pdla;
    HRESULT hr = THR(LazyGetDLAccessor(&pdla));
    if (hr)
    {   // BUGBUG: Is E_OUTOFMEMORY cool here?
        goto Cleanup;
    }

    // if not the bound DISPID, all OK
    if (!pdla->IsBound() || dispid != pdla->getBoundID())
        goto Cleanup;

    // we're restoring the old value, don't give other folks
    //      a chance to veto!
    if (_fValRestoring)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // if notification was generated because we're stuffing the bound
    //      value, then broadcast the change
    if (_fValChanging)
        goto Cleanup;

    {
        // if we're already dirty, no problem -- broadcast the change
        if (_fValDirty)
            goto Cleanup;

        if (!pdla->IsWritable())
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (this != _pDoc->_pSiteCurrent)
            goto Cleanup;

        Assert(!_fValDirty);
        // Unclear if we want to consinder the control Dirty, or wait for
        //  OnControlChanged
        // _pDoc->_fValDirty = true;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     COleDataSite::OnControlChanged, COleSite
//
//  Synopsis:   Give us a chance to act on a
//              control's OnChanged notification,  and cancel any forwarding
//              which might normally take place through our XObject.
//
//  Arguments:  dispid of property who's value has changed
//
//  Returns:    HRESULT.  Any non-zero HRESULT, including S_FALSE, will
//              cancel OnChanged forwarding.
//
//--------------------------------------------------------------------------

HRESULT
COleDataSite::OnControlChanged(DISPID dispid)
{
    CDataLayerAccessor *pdla;
    HRESULT             hr = THR(LazyGetDLAccessor(&pdla));

    if (hr)
        goto Cleanup;                   // BUGBUG: Is E_OUTOFMEMORY cool here?

    hr = THR(COleSite::OnControlChanged(dispid));
    if (hr)
        goto Cleanup;

    // if not the bound DISPID, all OK
    if (_fOwnTBag && !_pDoc->_fInstancePropagating && (!pdla->IsBound() ||
         (dispid != pdla->getBoundID() && dispid != DISPID_VALUE)))
    {
        // call forwarding code for propagation of standard prop changed
        hr = OnPropertyChange(dispid, _pDisp);
        goto Cleanup;
    }

    // if we're in the act of resetting a readonly value, don't want to
    //      broadcast messages
    if (_fValRestoring)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // if notification was generated because we're stuffing the bound
    //      value, then broadcast the change
    if (_fValChanging)
        goto Cleanup;

    // Is there data to save?
    hr = OnControlChangedSaveData(dispid);
    if (hr)
    {
        // we have to restore the original value!
        _fValRestoring = TRUE;
        IGNORE_HR(RefreshData());
        _fValRestoring = FALSE;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}




//+-------------------------------------------------------------------------
//
//  Method:     COleDataSite::DataSourceChanged, COleSite
//
//  Synopsis:   Notify that the rowset we bound to is changed
//
//--------------------------------------------------------------------------

void
COleDataSite::DataSourceChanged()
{
    if (HasDLA())
    {
        CDataLayerAccessor *pdla;
        IGNORE_HR(LazyGetDLAccessor(&pdla));
        Assert(pdla);
        pdla->Passivate();
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     ConnectControl2
//
//  Synopsis:   mainly calls the parent, does some additional stuff
//              like figuring out the databinding interface
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------
HRESULT COleDataSite::ConnectControl2(IUnknown *pUnkCreate, DWORD dwInitFlags)
{
    HRESULT hr;

    Assert(pUnkCreate);

    if (getTemplate()!=this)
    {
        //dwInitFlags &= ~FSI_GETEXTENT;
    }
    hr = super::ConnectControl2(pUnkCreate, dwInitFlags);
    if (SUCCEEDED(hr) && _fOwnTBag)
    {
        Assert(_pParent->TestClassFlag(SITEDESC_BASEFRAME));
        hr = SetDLASource(((CBaseFrame *)_pParent)->getOwner());
        if (hr)
        {
            goto Cleanup;
        }

        Assert(TBag()->_fDispatchType == BIT_None);
#if DBG == 1
        // If debugging then check trace tag to do IDispatch only binding.
        if (!IsTagEnabled(tagDataBind))
        {
#endif
        // ITextControl or a generic IDispatched based control.
        hr = THR_NOTRACE(QueryControlInterface(IID_IMorphDataControl,
                                       (void **)&_invokeIntf.pMorphData));
        if (!hr)
        {
            TBag()->_fDispatchType = BIT_Morph;
        }

#if DBG == 1
        }       // Only do this if trace tag isn't on.
#endif
        if (TBag()->_fDispatchType == BIT_None)
        {
            CacheDispatch();
            hr = S_OK;
            if (_pDisp)
            {
                TBag()->_fDispatchType = BIT_Disp;
            }
            else if ( ! _fFakeControl )
            {
                hr = E_FAIL;
            }
        }

        IGNORE_HR(QueryControlInterface(IID_IProvideInstance, (void **)&TBag()->_pProvideInstance));
    }

Cleanup:
    RRETURN(hr);
}
//--+ End of Method-----------------------------------------------------------



//+---------------------------------------------------------------------------
//
//  Member:     ErrorClause
//
//  Synopsis:   Builds up the error clause.
//
//  Arguments:  errorAction    - Action that caused the error.
//
//  Returns:    Nothing
//
//----------------------------------------------------------------------------
/*
void
COleDataSite::BindErrorClause (int errorAction)
{
    int     errorDescr = IDS_ERR_DB_UNXPECT;

    switch (hr)
    {
    case DB_S_CANTCOERCE:
        errorDescr = IDS_ERR_DB_COERCE;
        break;
    case E_OUTOFMEMORY;
        errorDescr = IDS_ERR_DB_NOMEM;
        break;
    case DB_E_BADROWHANDLE:
        errorDescr = IDS_ERR_DB_HROW;
        break;
    case DB_E_DELETEDROW:
        errorDescr = IDS_ERR_DB_DELROW;
        break;
    case DB_E_WRITEONLYACCESSOR:
        errorDescr = IDS_ERR_DB_WRITE;
        break;
    case DB_E_BADACCESSORHANDLE:
        errorDescr = IDS_ERR_DB_BADACC;
        break;
    case DB_E_NOTREENTRANT:
        errorDescr = IDS_ERR_DB_NOTIFY;
        break;
    case DB_E_OBJECTOPEN:
        errorDescr = IDS_ERR_DB_OBJOPEN;
        break;
    case DB_E_READONLYACCESSOR:
        errorDescr = IDS_ERR_DB_READ;
        break;
    case DB_E_SCHEMAVIOLATION:
        errorDescr = IDS_ERR_DB_SCHEMA;
        break;
    default:
        errorDescr = IDS_ERR_DB_UNXPECT;
    }

    ErrorAppendDescription(IDS_ERROR_CLAUSE,
                           ErrorInstance(), errorAction,
                           ErrorInstance(), errorDescr);
}
*/



//+---------------------------------------------------------------
//
//  Member:     COleDataSite::BindIndirect
//
//  Synopsis:   get's called for header/footer to fetch the columnname
//
//  Arguments:  None
//
//  Notes:
//
//---------------------------------------------------------------
HRESULT
COleDataSite::BindIndirect(void)
{
    HRESULT         hr=S_OK;
    COleDataSite    *pRelated = TBag()->_pRelated;
    TCHAR           *ptchar=0;

    if (pRelated)
    {
        // we only do something for footers/headers

        hr = THR(pRelated->GetColumnName(&ptchar));
        if (!hr)
        {
            hr = THR(GetMorphInterface()->SetText(ptchar));
            if (hr)
                goto Error;
        }
        else if (hr != S_FALSE)
        {
            goto Error;
        }
        hr = S_OK;
    }
Error:
    RRETURN(hr);
}
//+---------------------------------------------------------------




//+---------------------------------------------------------------
//
//  Member:     COleDataSite::RefreshData, CSite
//
//  Synopsis:   RefreshData the data from the Bind Source.
//
//  Arguments:  None
//
//  Notes:
//
//---------------------------------------------------------------

HRESULT
COleDataSite::RefreshData ()
{
    BOOL                 fValChanging = _fValChanging;
    CTBag *              pTBag        = TBag();
    BIG_VARIANT          bigVariant;
    CDataLayerAccessor * pDLA;
    HRESULT              hr           = THR(LazyGetDLAccessor(&pDLA));

    _fValChanging = TRUE;

    if (hr)
    {
        goto Cleanup;
    }

    if (!pDLA->IsActive())
    {
        if (pTBag->_cstrControlSource)
        {
            hr = CreateAccessorByColumnName(pTBag->_cstrControlSource);
            if (hr)
                goto Cleanup;
        }
        else
            goto Cleanup;
    }

    if (!pDLA->IsBound())
        goto Cleanup;

    if (_pBindSource && _invokeIntf.pDispatch)
    {
        Assert(pTBag->_fDispatchType != BIT_None);

        bigVariant.vt = VT_EMPTY;

        // Read the data from the database.
        hr = THR(_pBindSource->GetData(pDLA, &bigVariant));
        if (!hr)
        {
            if (pTBag->_fDispatchType == BIT_Morph)
            {
                if (pDLA->IsWSTRBound())
                {
                    WIDE_STR    *pStr = (WIDE_STR *)&bigVariant;

                    // Write the data to the control.
#if PAINTTEST
                    hr = _cstrValue.Set(pStr->chBuff);
#else
                    hr = THR(_invokeIntf.pMorphData->SetText(
                                                MAKEBSTR(pStr->chBuff) ));
#endif

                // Variant isn't valid so set to empty.
                    bigVariant.vt = VT_EMPTY;
                }
                else
                {
                    hr = THR(_invokeIntf.pMorphData->SetValue(
                                                (VARIANT *) &bigVariant) );
                }
            }
            else if (pDLA->IsVTableBound())  // Dual interface dispatch?
            {
                // Yes, use the VTable binding to write to the control.
                hr = CDataLayerAccessor::VTableDispatch(
                                    _invokeIntf.pDispatch,
                                    pDLA->getControlType(),
                                    CDataLayerAccessor::VTBL_PROPSET,
                                    (VARIANT *)&bigVariant,
                                    pDLA->VTblPutDefOffset());
            }
            else
            {
                EXCEPINFO   except;

                InitEXCEPINFO(&except);

                // No, use Invoke to write to the control.
                hr = THR(SetDispProp(_invokeIntf.pDispatch,
                                     pDLA->getBoundID(),
                                     IID_NULL,
                                     g_lcidUserDefault,
                                     (VARIANT *)&bigVariant,
                                     &except));

                // BUGBUG: Rich text error handling...

                FreeEXCEPINFO(&except);
            }

            // Is it a special non-BSTR string?
            if (bigVariant.vt != VT_EMPTY)
            {
                // No, the contents is a normal VARIANT.
                VariantClear((VARIANT *)&bigVariant);
            }

            if (hr)
            {
                // BUGBUG: currently, when data pushing fails,
                //  we stop generating.
                hr = S_OK;
            }

        }
    }

Cleanup:
    if (!hr)
    {
        hr = super::RefreshData();
    }

    _fValChanging = fValChanging;

    RRETURN1(hr, S_FALSE);
}



//+---------------------------------------------------------------
//
//  Member:     COleDataSite::SaveData, CSite
//
//  Synopsis:   Save the data from the Bind Source.
//
//  Arguments:  None
//
//---------------------------------------------------------------

#pragma warning(disable:4702)
// compiler bug: bogus "unreachable code" on class-to-interfacepointer cast

HRESULT
COleDataSite::SaveData ()
{
    CTBag *                 pTBag   = TBag();
    VARIANT_BOOL            fCancel = VB_FALSE;
    CReturnBoolean          rb(fCancel);
    CDataLayerAccessor *    pDLA;
    HRESULT                 hr;

    GetTemplate()->FireControlElementEvents_BeforeUpdate((IDispatch *) &rb);
    fCancel = rb.GetValue();

    if (!fCancel && _pBindSource && _pBindSource->EnsureHRow())
    {
        hr = THR(LazyGetDLAccessor(&pDLA));
        if (hr)
        {   // BUGBUG: Is E_OUTOFMEMORY cool here?
            goto Cleanup;
        }

        if (_invokeIntf.pDispatch && pDLA->IsBound())
        {
            Assert(pTBag->_fDispatchType != BIT_None);

            VARIANT     variant;
            void       *pData = (void *)&variant;
            WIDE_STR    wStr;
            EXCEPINFO   except;

            VariantInit(&variant);

            // Get the data via IMorphDataControl or IDispatch?
            if (pTBag->_fDispatchType == BIT_Morph)
            {
                // Read the data from the MDC control.
                hr = THR(_invokeIntf.pMorphData->GetValue(&variant));
                if (!hr)
                {
                    if (pDLA->IsWSTRBound())
                    {
                        Assert(variant.vt == VT_BSTR);

                        wStr.uCount = FormsStringLen(variant.bstrVal);
                        if (wStr.uCount < MAX_WSTR_LEN)
                        {
#ifndef _MAC
                            Assert(sizeof(OLECHAR) == sizeof(wchar_t));
                            _tcscpy(wStr.chBuff, (TCHAR *)variant.bstrVal);
#else
                            Assert(sizeof(OLECHAR) == 1);
                            Verify(MultiByteToWideChar(CP_ACP, 0, wStr.chBuff,
                                        MAX_WSTR_LEN, variant.bstrVal,
                                        wStr.uCount ) );
#endif
                            wStr.uCount *= sizeof(TCHAR);

                            pData = (void *)&wStr;
                        }
                        else
                        {
                            wStr.uCount = 0;
                            wStr.chBuff[0] = _T('\0');
                        }

                        pData = (void *)&wStr;
                    }
                }
            }
            else
            {
                // Dual interface dispatch?
                if (pDLA->IsVTableBound())
                {
                    // Yes, use the VTable binding to write to the control.
                    hr = CDataLayerAccessor::VTableDispatch(
                                        _invokeIntf.pDispatch,
                                        pDLA->getControlType(),
                                        CDataLayerAccessor::VTBL_PROPGET,
                                        &variant,
                                        pDLA->VTblGetDefOffset());
                }
                else
                {
                    InitEXCEPINFO(&except);
                    // Read the data from the control.
                    hr = THR(GetDispProp(_invokeIntf.pDispatch,
                                         pDLA->getBoundID(),
                                         IID_NULL,
                                         g_lcidUserDefault,
                                         &variant,
                                         &except));
                    FreeEXCEPINFO(&except);
                }
            }

            if (!hr)
            {
                 // Write the data to the database.
                hr = THR(_pBindSource->SetData(pDLA, pData));
            }

            IGNORE_HR(VariantClear(&variant));
        }

        if (!hr)
        {
            hr = super::SaveData();
            if (!hr)
            {
                GetTemplate()->FireControlElementEvents_AfterUpdate();
            }
        }

    }
    else
        hr = S_FALSE;         // BUGBUG: Cancelled out of BeforeUpdate, might
                              //         wanta more explicit error message.

Cleanup:
    RRETURN1(hr, S_FALSE);
}
#pragma warning(default:4702)




//+---------------------------------------------------------------
//
//  Member:     COleDataSite::GetControlSource
//
//  Synopsis:   Access ControlSource property
//
//---------------------------------------------------------------
STDMETHODIMP
COleDataSite::GetControlSource(BSTR * pbstrControlSource)
{
    TraceTag((tagOleDataSite,"COleDataSite::GetControlSource"));

    if (pbstrControlSource  == NULL)
    {
        return E_INVALIDARG;
    }

    RRETURN(THR(TBag()->_cstrControlSource.AllocBSTR(pbstrControlSource)));
}
//---+End of Method-----------------------------------------------



//+---------------------------------------------------------------
//
//  Member:     COleDataSite::SetControlSource
//
//  Synopsis:   Sets ControlSource property
//
//---------------------------------------------------------------
STDMETHODIMP
COleDataSite::SetControlSource(LPTSTR pwstrControlSource)
{
    TraceTag((tagOleDataSite,"COleDataSite::SetControlSource"));

    HRESULT hr=S_OK;

    CTBag * pTBag = TBag();

    if (!pTBag->_cstrControlSource || !pwstrControlSource ||
            _tcsicmp(pTBag->_cstrControlSource, pwstrControlSource))
    {
        // passivate the DLA, the new accesssor will be created later
        if (HasDLA())
        {
            CDataLayerAccessor *pdla;
            IGNORE_HR(LazyGetDLAccessor(&pdla));
            Assert(pdla);
            pdla->Passivate();
        }
        hr = THR(TBag()->_cstrControlSource.Set(pwstrControlSource));
        if (!hr)
        {
            hr = OnPropertyChange(DISPID_ControlSource, 0);
        }
    }

    RRETURN(hr);
}
//---+End of Method-----------------------------------------------



//+---------------------------------------------------------------
//
//  Member:     COleDataSite::GetColumnName
//
//  Synopsis:   returns the pointer to the columnname
//
//---------------------------------------------------------------
HRESULT COleDataSite::GetColumnName(LPTSTR *pplptstrName)
{
    HRESULT hr=S_FALSE;
    CTBag   *pTBag = TBag();

    if (pTBag->_cstrControlSource)
    {
        *pplptstrName = pTBag->_cstrControlSource;
        hr = S_OK;
    }
    else if (pTBag->_uiAccessorColumn != (UINT)-1)
    {
        // so we have a columnnumber binding
        CDataLayerCursor   *pdlc;
        CDataLayerAccessor *pdla;
        hr = THR(LazyGetDLAccessor(&pdla));
        if (hr)
        {
            goto Error;
        }

        hr = THR(pdla->LazyGetDataLayerCursor(&pdlc));
        if (hr)
        {
            goto Error;
        }

        hr = THR(pdlc->GetColumnName(pTBag->_uiAccessorColumn, *pplptstrName));
        if (hr)
        {
            goto Error;
        }
    }
Error:
    RRETURN1(hr, S_FALSE);
}
//---+End of Method-----------------------------------------------



//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::SetControl
//
//  Synopsis:   Tell the dla what kind of control we'll be binding to.
//
//  Arguments:  None
//
//  Returns:    HRESULT (S_OK)
//
//  History:    03/15/95    TerryLu     Created
//
//----------------------------------------------------------------------------

HRESULT
COleDataSite::SetControl ()
{
    CTBag               *pTBag  = TBag();
    BindIType           dspType = pTBag->_fDispatchType;
    CDataLayerAccessor *pdla;
    HRESULT             hr      = THR(LazyGetDLAccessor(&pdla));

    if (hr)
    {   // BUGBUG: Is E_OUTOFMEMORY cool here?
        goto Cleanup;
    }

    // We have to talk to the control somehow.
    Assert(dspType != BIT_None);

    hr = THR(pdla->SetControl(_pDoc->_pClsTab, TBag()->_wclsid, dspType == BIT_Morph));

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::SetDLASource
//
//  Synopsis:   Set the DLA Source of our DLA to that of the passed in frame
//
//  Arguments:  pdfr - the frame in question
//
//  Returns:    S_OK        Succeeded
//              E_xxx       Failure from LazyGetDLAccessorSource
//
//----------------------------------------------------------------------------

HRESULT
COleDataSite::SetDLASource(CDataFrame* pdfr)
{
    CDataLayerAccessorSource *pdlas = NULL;
    CDataLayerAccessor       *pdla;
    HRESULT                   hr    = THR(LazyGetDLAccessor(&pdla));
    if (!hr)
    {
        if (pdfr)
        {
            hr = THR(pdfr->LazyGetDLAccessorSource(&pdlas));
        }
        if (!hr)
        {
            pdla->SetDLASource(pdlas);
        }
    }
    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::CreateAccessorByColumnNumber
//
//  Synopsis:   Create the accessor based on the control and the columnID to
//              bind to.
//
//  Arguments:  uiColumnID  - column ID
//
//  Returns:    HRESULT
//                  S_OK    - Success
//
//
//  History:    03/15/95    TerryLu     Created
//
//----------------------------------------------------------------------------

HRESULT
COleDataSite::CreateAccessorByColumnNumber (UINT uiColumnID)
{
    CDataLayerAccessor *pdla;
    HRESULT hr = THR(LazyGetDLAccessor(&pdla));
    if (!hr)
    {
        hr = THR(pdla->SetColumnNumber(uiColumnID));
        if (!hr)
        {
            TBag()->_uiAccessorColumn = uiColumnID;
            hr = THR(SetControl());
        }
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::CreateAccessorByColumnName
//
//  Synopsis:   Create the accessor based on the control and the columnID to
//              bind to.
//
//  Arguments:  pstrName  - column name to bind to
//
//  Returns:    HRESULT
//                  S_OK    - Success
//
//
//  History:    03/15/95    TerryLu     Created
//
//----------------------------------------------------------------------------

HRESULT
COleDataSite::CreateAccessorByColumnName (LPTSTR pstrName)
{
    CDataLayerAccessor *pdla;
    HRESULT hr = THR(LazyGetDLAccessor(&pdla));
    if (!hr)
    {
        hr = THR(pdla->SetColumnName(pstrName));
        if (!hr)
        {
            hr = THR(SetControl());
        }
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::CreateInstance
//
//  Special:    virtual
//
//  Synopsis:   Create an instance
//
//  Arguments:  [pDatadoc]  -- form
//              [ppFrame]   -- instance to return
//              [info]      -- create info
//
//  Returns:    HRESULT (S_OK)
//
//  History:    12/16/94    istvanc     Created
//              1/19/95     frankman    Changed creation, init needs to be called
//              2/7/95      alexa       Modified error handling.
//
//----------------------------------------------------------------------------

HRESULT COleDataSite::CreateInstance(
        CDoc * pDoc,
        CSite * pParent,
        OUT CSite **ppFrame,
        CCreateInfo * pcinfo)
{
    HRESULT hr = S_OK;
    Assert(_fOwnTBag);

    COleDataSiteInstance *pOleDataSite = new (this) COleDataSiteInstance (pParent, this);
    if (pOleDataSite)
    {
        if ( OK( hr = pOleDataSite->InitInstance () ) )
        {
            hr = BuildInstance (pOleDataSite, pcinfo);
            if (hr)
                goto Error;
        }
        else
            goto Error;
    }
    else
        goto MemoryError;

Cleanup:
    *ppFrame =  (CSite *)pOleDataSite;
    RRETURN (hr);

MemoryError:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

Error:
    //delete pOleControlFrame;
    pOleDataSite->Release();
    pOleDataSite = NULL;
    goto Cleanup;
}




//+---------------------------------------------------------------------------
//
//  Member:     BuildInstance
//
//  Synopsis:   To build a site instance  and build the binding information
//
//  Arguments:  pNewInstance            newly created ole data site
//              info                    detail frame's CreateInfo structure.
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT COleDataSite::BuildInstance (COleDataSite * pNewInstance, CCreateInfo * pcinfo)
{
    Assert(pNewInstance);

    if (pcinfo->_pBindSource)
        pNewInstance->_pBindSource = pcinfo->_pBindSource;

    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Member:     CreateToFit
//
//  Synopsis:   Set the proposed size of the control (in case of AutoSize).
//
//  Arguments:  rclView         bounding view rectangle.
//              dwFlags         parameter for the Move member function.
//
//
//----------------------------------------------------------------------------

HRESULT
COleDataSite::CreateToFit (IN const CRectl * prclView, IN DWORD dwFlags)
{
    HRESULT         hr = S_OK;
    IOleObject  *   pObj;
    CSizel          szl;

    // Note: if template of the control is dirty or the layout was dirty,
    // it means that _rcl is already reset by the parent to the template.

    if (_fAutoSize)
    {
        hr = THR(QueryControlInterface(IID_IOleObject, (LPVOID*)&pObj));
        Assert (pObj);      // OleSite have to support the IOleObject interface.
#if DBG==1
        if (!hr)
        {
#endif
            hr = THR(pObj->GetExtent(DVASPECT_CONTENT, &szl));
            if (!hr)
            {
                if (_rcl.Size () != szl)
                {
                    CRectl rcl;
                    rcl.left = _rcl.left;
                    rcl.top  = _rcl.top;
                    rcl.right = rcl.left + szl.cx;
                    rcl.bottom = rcl.top + szl.cy;
                    SetProposed(this, &rcl);
                }
            }
            ReleaseInterface(pObj);
#if DBG==1
        }
#endif
    }

    RRETURN (hr);
}

//+---End Of Method---------------------------------------------------------------



//+---------------------------------------------------------------
//
//  Member:     COleDataSiteInstance::COleDataSiteInstance, public
//
//  Synopsis:   create new Data Site object from the template
//
//  Effects:    memcpy already was called during the new operator (copied data from the template into this object).
//
//  Arguments:  [pfrTemplate] --  control template
//
//  Notes:      can not fail
//
//---------------------------------------------------------------

COleDataSiteInstance::COleDataSiteInstance (CSite * pParent, COleDataSite * pTemplate)
          :super(pParent, pTemplate)
{
}


COleDataSiteInstance::~COleDataSiteInstance()
{
}



//+---------------------------------------------------------------------------
//
//  Member:     InitInstance
//
//  Synopsis:   is called after the clone constructor (from Template).
//              this member function gets called only for instance creation.
//
//  Arguments:  info            Procreation info structure,
//                              contains binding information.
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT COleDataSiteInstance::InitInstance()
{
    HRESULT hr;
    COleDataSite * pTemplate = getTemplate();

    Assert(pTemplate);
    Assert(TBag()->_wclsid != WCLSID_INVALID);

    hr = COleSite::Clone(pTemplate);    // BUGBUG check if _pUnkOuter is set correctly...
    if (hr)
        goto Cleanup;

    hr = THR(CloneControl(pTemplate));
    if (hr)
        goto Cleanup;

    if ( _fFakeControl )
        goto Cleanup;

    // BUGBUG: if control is bindable, we need to handle error
    IGNORE_HR(::ConnectPrivateSink(
            _pUnkCtrl,
            IID_IPropertyNotifySink,
            CPIF_PNS | CPIF_NODISCONNECT,
            &_OSPNS));

Cleanup:
    RRETURN(hr);
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
COleDataSiteInstance::EnsureIBag()
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

//+------------------------------------------------------------------------
//
//  Member:     COleDataSiteInstance::CloneControl
//
//  Synopsis:   Clone the control in the template object
//
//  Arguments:  pTemplate   pointer to template object
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
COleDataSiteInstance::CloneControl(COleDataSite * pTemplate)
{
    HRESULT                 hr;
    IPersistStreamInit *    pPStm = NULL;
    IUnknown *              pUnkCreate = NULL;
    DWORD                   dwFlags = 0;
    IStream *               pStm    = NULL;
    IPersistStorage *       pPStg      = NULL;
    IStorage *              pStg = NULL;

    hr = THR(CreateControl());
    if (hr)
        goto Cleanup;

    hr = THR(ConnectControl1(&pUnkCreate, &dwFlags));
    if (hr)
        goto Cleanup;

    if (TBag()->_pProvideInstance)
    {
        hr = THR_NOTRACE(QueryControlInterface(
                IID_IPersistStreamInit,
                (void **) &pPStm));

        if (OK(hr))
        {
            hr = THR(pPStm->InitNew());
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        // doesn't support template/instance, let's stream it
        if (_fStreamed)
        {
            LONGLONG i64Start = 0;

            hr = THR(pTemplate->QueryControlInterface(
                    IID_IPersistStreamInit,
                    (void **) &pPStm));
            if (hr)
                goto Cleanup;

            hr = CreateStreamOnHGlobal(NULL, TRUE, &pStm);
            if (hr)
                goto Cleanup;

            hr = THR(pPStm->Save(pStm, TRUE));
            if (hr)
                goto Cleanup;

            ReleaseInterface(pPStm);

            hr = THR(QueryControlInterface(
                    IID_IPersistStreamInit,
                    (void **) &pPStm));
            if (hr)
                goto Cleanup;

            pStm->Seek(*(LARGE_INTEGER*)&i64Start, STREAM_SEEK_SET, NULL);

            hr = THR(pPStm->Load(pStm));
            if (hr)
                goto Cleanup;
        }
        else
        {
            hr = THR(pTemplate->QueryControlInterface(
                    IID_IPersistStorage,
                    (void **) &pPStg));
            if (hr)
                goto Cleanup;

            hr = CreateStorageOnHGlobal(NULL, &pStg);
            if (hr)
                goto Cleanup;

            hr = THR(OleSave(pPStg, pStg, TRUE));
            if (hr)
                goto Cleanup;

            ReleaseInterface(pPStg);

            hr = THR(QueryControlInterface(
                    IID_IPersistStorage,
                    (void **) &pPStg));
            if (hr)
                goto Cleanup;

            hr = THR(pPStg->Load(pStg));
            if (hr)
                goto Cleanup;
        }
    }

    hr = THR(ConnectControl2(pUnkCreate, dwFlags));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pUnkCreate);
    ReleaseInterface(pPStm);
    ReleaseInterface(pStm);
    ReleaseInterface(pPStg);
    ReleaseInterface(pStg);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     COleDataSiteInstance::CreateControl
//
//  Synopsis:   Create a control with GenerateInstance
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
COleDataSiteInstance::CreateControl()
{
    HRESULT hr = S_OK;

    if ( _fFakeControl )
    {
        hr = super::CreateControl();
    }
    else if (TBag()->_pProvideInstance)
    {
        Assert(_pUnkCtrl == 0);

        if (OK(TBag()->_pProvideInstance->GenerateInstance(
                _pUnkOuter,
                IID_IUnknown,
                (void **) &_pUnkCtrl)))
        {
            _fXAggregate = TRUE;
        }
        else
        {
            hr = TBag()->_pProvideInstance->GenerateInstance(
                NULL,
                IID_IUnknown,
                (void **) &_pUnkCtrl);
        }
    }
    else
    {
        hr = super::CreateControl();
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     ConnectControl2
//
//  Synopsis:   mainly calls the parent, does some addtional stuff
//              like figuring the databinding interface out
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------
HRESULT COleDataSiteInstance::ConnectControl2(IUnknown *pUnkCreate, DWORD dwInitFlags)
{
    HRESULT hr;

    Assert(pUnkCreate);

    hr = super::ConnectControl2(pUnkCreate, dwInitFlags);
    if (SUCCEEDED(hr))
    {
        switch(TBag()->_fDispatchType)
        {
        case BIT_Morph:
            hr = THR_NOTRACE(QueryControlInterface(IID_IMorphDataControl,
                                           (void **)&_invokeIntf.pMorphData));
            break;
        case BIT_Disp:
            CacheDispatch();
            _invokeIntf.pDispatch = _pDisp;
            break;
        default:
            Assert(_fFakeControl && "Control connection type is not set");
        }
    }

    RRETURN(hr);
}
//--+ End of Method-----------------------------------------------------------




