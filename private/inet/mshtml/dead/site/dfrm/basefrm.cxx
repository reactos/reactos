//----------------------------------------------------------------------------
//
//  Maintained  by   Lajosf, alexa
//
//  Copyright   (C) Microsoft Corporation, 1994-1995.
//              All rights Reserved.
//              Information contained herein is Proprietary and Confidential.
//
//  File        src\ddoc\datadoc\basefrm.cxx
//
//  Contents    Implements CBaseFrame
//
//  Classes     CBaseFrame
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#include "dfrm.hxx"
#include "idobj.hxx"
#include "cdatadsp.hxx"

#define _cxx_
#include "basefrm.hdl"

#if PRODUCT_97
const CLSID * CBaseFrame::s_aclsidPages[] =
{
    &CLSID_CCDGenericPropertyPage,
    &CLSID_CControlPropertyPage,
    &CLSID_CDataFramePropertyPage,
    NULL
};
#endif

KEY_MAP CBaseFrame::s_aKeyActions[]= {
    {VK_UP,     KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)RotateControl,NAVIGATE_UP},
    {VK_DOWN,   KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)RotateControl,NAVIGATE_DOWN},
    {VK_LEFT,   KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)RotateControl,NAVIGATE_LEFT},
    {VK_RIGHT,  KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)RotateControl,NAVIGATE_RIGHT},
    {VK_RETURN, KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)ProcessEnterKey,0},
};

int CBaseFrame::s_cKeyActions = ARRAY_SIZE(s_aKeyActions);


DeclareTag(tagBaseFrame,"src\\ddoc\\datadoc\\basefrm.cxx","Frame site");

//+---------------------------------------------------------------------------
//
//  Member      CBaseFrameIBag::operator new.
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

#if 0
void*
CBaseFrame::CBaseFrameIBag::operator new (size_t s, CBaseFrame::CBaseFrameIBag * pOriginal)
{
    TraceTag((tagBaseFrame, "CBaseFrameIBag::operator new "));
    void * pNew = ::operator new(s);
    if (pNew)
        memcpy(pNew, pOriginal, sizeof(CBaseFrame::CBaseFrameIBag));
    return pNew;
}
#endif


//+-----------------------------------------------------------------------
//  Constructors, destructors, initialization, passivation.
//------------------------------------------------------------------------

CBaseFrame::CBaseFrame(CDoc * pDoc, CSite * pParent)
    : super(pDoc, pParent, ETAG_NULL)
{
    _fOpaque = TRUE;
    _fOwnTBag = TRUE;
    _pTemplate = this;
    _fVisible = TRUE;
    _fSelectChild = TRUE;
    _fEnabled = TRUE;
    _colorBack = RGB(255,255,255);
}


//+---------------------------------------------------------------------------
//
//  MEMBER      CBaseFrame::constructor for instances
//
//  Synopsis    Creates CBaseFrame from template.
//
//  Arguments   pForm       Pointer to the enclosing form.
//              pTemplate   Pointer to a Template object.
//              info        Binding mechanizm structure for this frame.
//
//  Note        This constructor is called during the instantiation pass of the
//              DataDoc. It is called with a "new" syntax of the operator new
//              from the template class. For example: new (this) CBaseFrame
//              (this, info);
//
//              Assumption: The "new (param)" syntax of new is used.
//
//              SideEffects: The Memeber data should not be reinitialized,
//              since the memcpy was called during the NEW operator.
//
//----------------------------------------------------------------------------

CBaseFrame::CBaseFrame(CDoc * pDoc, CSite * pParent, CBaseFrame * pTemplate)
    : super(pDoc, pParent, pTemplate)
{
    _fOwnTBag = FALSE;
    _pTemplate = pTemplate;
    _pSelectionElement = NULL;      //  review: do we need this?
    _fVisible = TRUE;
    _fSelectChild = TRUE;
    _fEnabled = TRUE;
}




//+---------------------------------------------------------------------------
//
//  Member      CBaseFrame::operator new.
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
CBaseFrame::operator new (size_t s, CBaseFrame * pOriginal)
{
    TraceTag((tagBaseFrame,"CBaseFrame::operator new "));
    void * pNew = ::operator new(s);
    if (pNew)
        memcpy(pNew, pOriginal, sizeof(CBaseFrame));
    return pNew;
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

HRESULT CBaseFrame::InitInstance ()
{
    TraceTag((tagBaseFrame, "CBaseFrame::InitInstance"));

    CBaseFrame *pTemplate = getTemplate();
    Assert (pTemplate);

    HRESULT hr;
    hr = super::Clone(pTemplate);
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CBaseFrame::PrivateQueryInterface, public
//
//  Synopsis:   Expose our IFaces
//
//  Arguments   iid     interface id of the queried interface
//              ppv     resulted interface pointer
//
//  Returns     HRESULT
//---------------------------------------------------------------------------
STDMETHODIMP
CBaseFrame::PrivateQueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    TraceTag((tagBaseFrame,"CBaseFrame::PrivateQueryInterface"));

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_TEAROFF(this, IDataFrame, _pUnkOuter)
        QI_TEAROFF2(this, IDispatch, IDataFrame, _pUnkOuter)
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
//  Function:   CBaseFrame::CreateSite, public
//
//  Synopsis:   Creates an empty site of the given type. After this, the
//              site's properties will be loaded and then InitSite will be
//              called.
//
//  Arguments:  [type]    -- Type of site to create.
//
//  Returns:    the site
//
//  Notes:      Each type of site must add custom code to this method.
//
//----------------------------------------------------------------------------

CSite *
CBaseFrame::CreateSite(ELEMENT_TAG type)
{
    switch (type)
    {
    case ETAG_DATAFRAME:
        return new CDataFrameTemplate(_pDoc, this);

    case ETAG_ROOTDATAFRAME:
        return new CRootDataFrame(_pDoc, this);

    case ETAG_DETAILFRAME:
        return new CDetailFrameTemplate(_pDoc, this);

    case ETAG_OLEDATASITE:
        return new COleDataSiteTemplate(this);

    case ETAG_HEADERFRAME:
        return new CHeaderFrameTemplate(_pDoc, this);
        break;

    default:
        TraceTag((tagError, "Cannot create unknown site type %d.  "
                  "Possible corrupt stream.", type));
        Assert(!"Unknown site type encountered during load! I don't know how "
               "to create it!");
        return NULL;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     AddToSites
//
//  Synopsis:   overloaded for index management
//
//  Returns:    Returns result from parent
//
//----------------------------------------------------------------------------

HRESULT
CBaseFrame::AddToSites(CSite *pSite, DWORD dwOperations)
{
    HRESULT hr;
    hr = super::AddToSites(pSite, dwOperations);

    if (!hr && _fOwnTBag && (dwOperations & ADDSITE_ADDTOLIST))
    {
        int i = _arySites.Size()-1;
        Assert(i >= 0);
        _arySites[i]->SetIndex(i);

    }

    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     DeleteSite
//
//  Synopsis:   overloaded for index management
//
//  Returns:    Returns result from parent
//
//----------------------------------------------------------------------------

HRESULT
CBaseFrame::DeleteSite(CSite * pSite, DWORD dwFlags)
{
    int     pos = pSite->getIndex();
    HRESULT hr;

    TraceTag((tagBaseFrame,"CBaseFrame::DeleteSite"));

    #if DBG==1
    if (dwFlags & DELSITE_MOVETORECYCLE)
    {
        TraceTag((tagBaseFrame,"CBaseFrame::DeleteSite - special fork for MoveToRecycle"));
    }
    #endif

    hr =  super::DeleteSite(pSite, dwFlags);

    if (_fOwnTBag && SUCCEEDED(hr))
    {
        int i;

        for ( i = pos; i < _arySites.Size(); i++)
        {
            _arySites[i]->SetIndex(i);
        }
    }

    if (dwFlags & DELSITE_MOVETORECYCLE)
    {
        pSite->_pParent=0;
    }

    //  Propagate the layout change
    if (  ! (dwFlags & DELSITE_NOREGENERATE) )
        hr = OnDataChange(DISPID_DeleteControl, FALSE);

    RRETURN(hr);
}




//+---------------------------------------------------------------
//
//  Member:     CBaseFrame::SiteTypeFromCLSID
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
CBaseFrame::SiteTypeFromCLSID(REFCLSID clsid)
{
//    ELEMENT_TAG st = super::SiteTypeFromCLSID(clsid);
ELEMENT_TAG st = ETAG_NULL;
Assert(0);

    switch(st)
    {
    case ETAG_ROOTDATAFRAME:
        st = ETAG_DATAFRAME;
        break;
    case ETAG_OBJECT:
        st = ETAG_OLEDATASITE;
        break;
    }

    return st;
}




//+------------------------------------------------------------------------
//
//  Member:     CBaseFrame::InsertNewControl
//
//  Synopsis:   Make sure that the control is actually created in the
//              template for this Base Frame, in case this is actually
//              an instance frame.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CBaseFrame::InsertNewSite(
            REFCLSID clsid,
            LPCTSTR pstrName,
            const RECTL * prcl,
            DWORD dwOperations,
            CSite ** ppSite)
{
    HRESULT     hr;

    CBaseFrame * pTemplate = getTemplate();

    if (pTemplate != this)
    {
        // offset the rectangle relative to top left
        OffsetRect((RECT *)prcl, pTemplate->_rcl.left - _rcl.left, pTemplate->_rcl.top - _rcl.top);
        hr = pTemplate->InsertNewSite(
                clsid,
                NULL,
                prcl,
                dwOperations,
                ppSite);
    }
    else
    {
        hr = InsertDOFromCLSID( clsid, (RECTL *)prcl, dwOperations, ppSite );
    }

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CBaseFrame::GetIndex, public
//
//  Synopsis:   Returns the parentindex of the frame
//
//  Returns:    the parentindex
//
//  Note:       By default it takes the index from the template bag.
//              If that is -1 (uninitialized) it tries to Find the
//              frame in the parent's _arySites.
//
//----------------------------------------------------------------------------

int CBaseFrame::getIndex(void)
{
    int index;

    index = TBag()->_iIndex;
    if ( index == -1 )
    {
        Assert(_pParent);
        DWORD dw;
        CSite * pSite;

        index = 0;
        for (pSite = _pParent->GetFirstSite(&dw);
            pSite;
            pSite = _pParent->GetNextSite(&dw))
        {
            if (pSite == this)
            {
                TBag()->_iIndex = index;
                break;
            }
            index++;
        }
        if (!pSite)
            index = -1;
    }

    return index;
}



//+---------------------------------------------------------------
//
//  Member:     CBaseFrame::Move
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
CBaseFrame::Move(RECTL *prcl, DWORD dwFlags)
{
    BOOL f;

    TraceTag((tagBaseFrame,"CBaseFrame::Move"));

    if (!(dwFlags & SITEMOVE_NOFIREEVENT))
    {
        f = _pDoc->_fDeferedPropertyUpdate;
        _pDoc->_fDeferedPropertyUpdate = TRUE;
    }

    HRESULT hr = THR(super::Move(prcl, dwFlags));
    if (hr)
        goto Cleanup;

    if (!(dwFlags & SITEMOVE_NOFIREEVENT))
    {
        if (!f)
        {
            // to force create to fit
            PropagateChange(STDPROPID_XOBJ_WIDTH);
            SetDeferedPropertyUpdate(f);
        }
    }

Cleanup:

    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     MoveSiteBy
//
//  Synopsis:   Move the site with delta
//
//----------------------------------------------------------------------------

void
CBaseFrame::MoveSiteBy (long x, long y)
{
    if (x || y)
    {
        // BUGBUG for now until relative coordinate system
        CRectl rcl = _rcl;
        rcl.OffsetRect(x, y);
        Move(&rcl, SITEMOVE_NOFIREEVENT | SITEMOVE_NOINVALIDATE | SITEMOVE_NOSETEXTENT);
    }
    return;
}


//+---------------------------------------------------------------
//
//  Member:     CBaseFrame::Notify
//
//  Synopsis:   Handle notification
//
//---------------------------------------------------------------
HRESULT
CBaseFrame::Notify(SITE_NOTIFICATION sn, DWORD dw)
{
    HRESULT hr = S_OK;

    switch (sn)
    {
    case SN_AMBIENTPROPCHANGE:
        // for now set _fDesignFeedback to false always
        hr = THR(super::Notify(sn, dw));
        break;

    default:
        hr = THR(super::Notify(sn, dw));
    }

    RRETURN1(hr, S_FALSE);
}


//+--------------------------------------------------------------------------
//
//  Member:     CBaseFrame::ConstructErrorInfo
//
//  Synopsis:   takes an hr and an optional IDS for the action part
//              puts those values in the error object
//
//              hr          common hr
//              ids         error string for the action part if
//                            -1 no action part
//
//  Returns     TRUE if error was handled
//
//---------------------------------------------------------------------------
BOOL
CBaseFrame::ConstructErrorInfo(HRESULT hr, int ids)
{
    BOOL fRet = FALSE;

    if (ids != -1)
    {
        PutErrorInfoText(EPART_ACTION, ids);
    }
    switch (hr)
    {
        case E_OUTOFMEMORY:
            PutErrorInfoText(EPART_ERROR, IDS_ERR_OD_E_OUTOFMEMORY);
            fRet = TRUE;
        default:
            PutErrorInfoText(EPART_ERROR, IDS_ERR_DDOCGENERAL);
            fRet = TRUE;
            break;
    }
    return fRet;
}

//+--------------------------------------------------------------------------
//
//  Member:     CBaseFrame::OnDataChanged
//
//  Synopsis:   called when data is changed
//
//  Arguments:  dispid      ID for changed data
//              fInvalidate Invalidate rectange if it is TRU
//
//  Returns     Nothing
//
//---------------------------------------------------------------------------

HRESULT
CBaseFrame::OnDataChange (DISPID dispid, BOOL fInvalidate)
{
    TraceTag((tagBaseFrame,"CBaseFrame::OnDataChange "));

    HRESULT hr;

    if (!_pDoc->_fInstancePropagating)
    {
        OnPropertyChange(dispid, 0);
        _pDoc->OnDataChange();
    }
    hr = PropagateChange(dispid);


    if (fInvalidate && !_pDoc->_fDeferedPropertyUpdate)
        Invalidate(NULL, 0);

    RRETURN(hr);
}




//+--------------------------------------------------------------------------
//
//  Member:     CBaseFrame::PropagateChange
//
//  Synopsis:   helper method, stuffs DISPID in change bags and call root
//
//  Arguments:  dispid      ID for changed data
//
//  Returns     Nothing
//
//---------------------------------------------------------------------------
HRESULT
CBaseFrame::PropagateChange(DISPID dispid)
{
    // the following  code is the propagation code
    HRESULT hr = S_OK;
    if (_fOwnTBag)
    {
        TBag()->_propertyChanges.AddDispID(dispid);
        Assert(RootFrame(this));
        if (!_pDoc->_fDeferedPropertyUpdate)
        {
            hr = RootFrame(this)->UpdatePropertyChanges(UpdatePropsPrepareTemplates);
        }

    }

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     SetPropertyFromTemplate
//
//  Synopsis:   Called during property propagation
//
//  Arguments:  DISPID      dispid that changed
//
//----------------------------------------------------------------------------
void CBaseFrame::SetPropertyFromTemplate(DISPID dispid)
{
    switch (dispid)
    {
        case DISPID_BACKCOLOR:
        case DISPID_FORECOLOR:
            // if parent is invalidated don't invalidate
            if (!_pParent->_fInvalidated)
            {
                Invalidate(NULL, 0);
            }
            break;
        default:
            // Assert(FALSE && "Property Propagation on Baseframe not implemented");
            break;
    }
}
//---End of Method------------------------------------------------------------






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
//  Info:       The _rcl of the controls is an actual coordinates of the control
//              in the document space. Note that repeater has it's own virtual
//              space, but the subframes of the repeater have _rcl in the document
//              space.
//              The _rclPropose of the control is used when _fAutoSize is
//              set to 1, or we want to propogate instance changes, so in those
//              cases _rclPropose is a desired size of the control.
//              The rclView that is passed to this member function is a view
//              rectangle on this frame.
//
//  Note:       The soul purpose of the CreateToFit is to set the _rclPropose
//              of the frame. The _rcl of the frame will be set in 2 places
//              1. At the root level of NotifyChildResize (note, it's not
//              nessasary the CRootDataFrame); 2. In the repeater, that keeps
//              it's own virtual space.
//
//  Note:       CreateToFit is "extremely" recursive function. Also note if
//              you call CreateToFit from outside (first time) you have to
//              pass a flag SITEMOVE_NOTIFYPARENT, otherwise the topmost
//              _rcl will never be set (look at the code of NotifyChildResize).
//
//----------------------------------------------------------------------------

HRESULT
CBaseFrame::CreateToFit (IN const CRectl * prclView, IN DWORD dwFlags)
{
    TraceTag((tagBaseFrame, "CBaseFrame::CreateToFit"));

    HRESULT         hr = S_OK;
    CRectl          rclSubView;
    CBaseFrame *    pTemplate;


    pTemplate = getTemplate();

    // We need to look at the template and if the template is dirty
    // we need to reset the subframe instances coordinates.

    // Note: we have to do it only when CreateToFit is call on the instance,
    // we don't need to do it on the template (just an optimization).

    // Note: if the frame was cahced during previous TEMPLATE_RESIZE
    // it needs to fetch the coordinates from the template. We check
    // that condition by checking  IsDirtyRectangle ().

    if (IsAnythingDirty ())
    {
        // Note: the parent is responsible to set the _rcl of a child
        // to the correct position. ==> A parent of this object had
        // to do it for this object.

        Edge        e;
        CRectl      rcl;
        CSizel      szl;

        rcl = pTemplate->_rcl;
        // POPULATEDIR means populate down
        e = (dwFlags & SITEMOVE_POPULATEDIR) ? edgeLeft : edgeRight;
        szl[edgeHorizontal] = _rcl[e] - rcl[e];
        e = (Edge) (e + edgeVertical);
        szl[edgeVertical]   = _rcl[e] - rcl[e];

        // if the template is dirty or the rectangle is dirty
        // or some frame below this one is dirty, reset the children's _rcl
        // to the template's one. The trick is we need to move
        // template _rcl to the current _rcl.

        // Go thru the sites and reset their positions according to their
        // templates.
        // the rectangle of template changed, so reset the resctangle

        CSite       **  ppSite = _arySites;
        CSite       **  ppSiteEnd = ppSite + _arySites.Size();
        CSite       *   pSite;

        // Go thru the sites and reset their positions according to their
        // templates.

        for (; ppSite < ppSiteEnd; ppSite++)
        {
            pSite = *ppSite;
            if (pSite->IsVisible() && !pSite->TestClassFlag(SITEDESC_REPEATER))
            {
                rcl = pSite->GetTemplate()->_rcl;
                rcl.OffsetRect (szl);
                pSite->Move (&rcl, dwFlags | SITEMOVE_NOFIREEVENT | SITEMOVE_NOMOVECHILDREN);
            }
            // Note if pSite is a repeater, it's template is DetailFrame
            // therefore the _rcl of the repeater will be set to the _rcl
            // of the detail section which is exactly what we need.
        }

        #if 0
        if (IsAutoSize ())
        {
            // it means the _rcl of this frame may change during this CreateToFit.
            rclSubView = rclView;
        }
        else
        {
            rclSubView.Intersect (&rclView, &_rcl);
            // If there is no intersection then we don't need to do anything.

            // Note: if there is repeated below, the repeater might be empty,
            // and wont have any detail frames inside. So we shouldn't assume
            // from the programing model that there is an instance of detail
            // if it's not visible.
        }
        #else
        rclSubView = *prclView;
        #endif

        hr = super::CreateToFit (&rclSubView, dwFlags);

        // Note: if it's an instance and the Dirty was set we need to reset it.
        _fIsDirtyRectangle = FALSE;
    }

    RRETURN1(hr, S_FALSE);
}









//+---------------------------------------------------------------------------
//
//  Member      CBaseFrame::BuildInstance
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
CBaseFrame::BuildInstance (CBaseFrame *pNewInstance, CCreateInfo * pcinfo)
{
    TraceTag((tagBaseFrame,"CBaseFrame::BuildInstance "));
    HRESULT hr = S_OK;

    Assert(this == getTemplate());
    Assert(pNewInstance);

    int iSitesCount = FRAMESCOUNTER;
    for (int i = 0; i < iSitesCount; i++)
    {
        CBaseFrame * pChild;
        hr = THR(FRAME(i)->CreateInstance (
                _pDoc,
                pNewInstance,
                (CSite**)&pChild,
                pcinfo));
        if (hr)
            goto Cleanup;
        if (pChild)
        {
            hr = THR(pNewInstance->AddToSites(
                    pChild,
                    ADDSITE_ADDTOLIST |
                        ADDSITE_AFTERINIT |
                        ADDSITE_NOINVALIDATE));
            if (hr)
            {
                FRAME(i)->Release();
                goto Cleanup;
            };
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBaseFrame::GetTypeInfo
//
//  Synopsis:   Delegates to superclass.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CBaseFrame::GetTypeInfo(UINT itinfo, ULONG lcid, ITypeInfo ** ppTI)
{
    TraceTag((tagBaseFrame, "CBaseFrame::GetTypeInfo"));

    RRETURN(super::GetTypeInfo(itinfo, lcid, ppTI));
}


//+-------------------------------------------------------------------------
//
//  Method:     CBaseFrame::GetTypeInfoCount
//
//  Synopsis:   Delegates to superclass.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CBaseFrame::GetTypeInfoCount(UINT * pctinfo)
{
    TraceTag((tagBaseFrame, "CBaseFrame::GetTypeInfoCount"));

    RRETURN(super::GetTypeInfoCount(pctinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     CBaseFrame::GetIDsOfNames
//
//  Synopsis:   Delegates to superclass.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CBaseFrame::GetIDsOfNames(
        REFIID riid,
        LPTSTR * rgsz,
        UINT c,
        LCID lcid,
        DISPID * rgdispid)
{
    TraceTag((tagBaseFrame, "CBaseFrame::GetIDsOfNames"));

    RRETURN(super::GetIDsOfNames(riid, rgsz, c, lcid, rgdispid));
}


//+---------------------------------------------------------------
//
//  Member:     CBaseFrame::Invoke
//
// Synopsis:    Provides access to properties and members of the control
//
// Arguments:   dispidMember    Member id to invoke
//              riid            Interface ID being accessed
//              wFlags          Flags describing context of call
//              pdispparams     Structure containing arguments
//              pvarResult      Place to put result
//              pexcepinfo      Pointer to exception information struct
//              puArgErr        Indicates which argument is incorrect
//
//  Returns     HRESULT
//
//---------------------------------------------------------------
STDMETHODIMP
CBaseFrame::Invoke(DISPID dispidMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS FAR *pdispparams,
        VARIANT FAR *pvarResult,
        EXCEPINFO FAR *pexcepinfo,
        UINT FAR *puArgErr)
{
    HRESULT hr = NOERROR;

    TraceTag((tagBaseFrame,"CBaseFrame::Invoke"));

    hr = super::Invoke(
                    dispidMember,
                    riid,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    puArgErr);

    RRETURN(hr);
}


//+-------------------***************************-----------------------------
//
//  Most, if not all of the below code should ultimately end up as CSite
//  functionality. The code itself should probably go into C_pForm.
//
//--------------------***************************-----------------------------




BOOL CBaseFrame::AreMyChildren(int c, CSite ** ppSite)
{
    for ( ; c > 0; c--, ppSite++)
    {
        if ((*ppSite)->_pParent != this)
            return FALSE;
    }

    return TRUE;
}




//+---------------------------------------------------------------------------
//
//  Member:     CBaseFrame::ProcessEnterKey
//
//  Synopsis:   Sets the selection/currency to the control above the current
//              one.
//
//  Returns:    Returns S_OK if everything is fine, S_FALSE if there was no
//              control to jump to. This will allow to traverse multiple
//              containers.
//
//  Comments:   @todo: what if there's an initial multiple selection?
//              What's the starting point?
//----------------------------------------------------------------------------

HRESULT __stdcall CBaseFrame::ProcessEnterKey(long Direction)
{
    return S_FALSE;
}






//+---------------------------------------------------------------------------
//
//  Member:     CBaseFrame::ControlAbove
//
//  Synopsis:   Sets the selection/currency to the control above the current
//              one.
//
//  Returns:    Returns S_OK if everything is fine, S_FALSE if there was no
//              control to jump to. This will allow to traverse multiple
//              containers.
//
//  Comments:   @todo: what if there's an initial multiple selection?
//              What's the starting point?
//----------------------------------------------------------------------------

HRESULT __stdcall CBaseFrame::RotateControl(long Direction)
{
    RRETURN1(THR(NavigateToNextControl((NAVIGATE_DIRECTION)Direction)),S_FALSE);
}






//+---------------------------------------------------------------------------
//
//  Member:     CBaseFrame::NavigateToNextControl
//
//  Synopsis:   Transfers the highlight to the "next" control. Uses the
//              navigation code to determine the "next" control.
//              in design mode it plays with the selection,
//              in run mode uses UI activation.
//
//  Returns:    Returns S_OK if everything is fine, S_FALSE if there was no
//              control to jump to. This will allow to traverse multiple
//              containers.
//
//  Comments:   @todo: what if there's an initial multiple selection?
//              What's the starting point?
//----------------------------------------------------------------------------
HRESULT CBaseFrame::NavigateToNextControl(NAVIGATE_DIRECTION Direction)
{
    HRESULT hr = S_FALSE;
    CSite * psCurrent;
    CSite * psNext = NULL;

    TraceTag((tagBaseFrame,"CBaseFrame::NavigateToNextControl"));

    //  we just entered the navigation handler chain
    psCurrent = _pDoc->_pSiteCurrent;

    if ( ! psCurrent )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = NextControl(Direction, psCurrent, &psNext,this);
    }

    if ( ! hr )
    {
        Assert(psNext);

        if ( _pDoc->_fDesignMode )
        {
//            BUGBUG (garybu) DESink removed from form.
//            _pDoc->_pDESink->SelectSites(1, &psNext, 0);  //  also sets up _psiteFocus
        }
        else
        {
            hr = THR(psNext->BecomeCurrentAndActive());
        }
    }

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CBaseFrame::NextControl
//
//  Synopsis:   Selects the next site in the arrow direction.
//
//  Arguments:  Direction:  enum telling the direction (up,down,left,right)
//              psCurrent:  the currently active/selected site
//              ppsNext:    points to where the Nex site should be returned to
//
//  Returns:    Returns S_OK if everything is fine, S_FALSE if there was no
//              control to jump to. This will allow to traverse multiple
//              containers.
//
//  Comments:   @todo: what if there's an initial multiple selection?
//              What's the starting point?
//
//  Comments:   It's the caller's problem to do anything with the site
//              returned as 'next'.
//----------------------------------------------------------------------------

HRESULT CBaseFrame::NextControl(NAVIGATE_DIRECTION Direction,
                                  CSite * psCurrent,
                                  CSite ** ppsNext,
                                  CSite * pSiteCaller,
                                  BOOL fStrictInDirection,
                                  int iFirstFrame,
                                  int iLastFrame)
{
    //  Get the control that has the keyboard focus.
    //  Retrieve its coordinates (rcl)
    //  Get all the controls to the right of it
    //  see if there are any controls that have "clashing" y coords
    //  if yes, select the nearest from these
    //  if not, select the nearest.

    HRESULT hr = S_FALSE;
/*
    CSite * psNextControl;
    CDataFrame * pdfr;

    TraceTag((tagBaseFrame,"CBaseFrame::NextControl"));


    Assert(psCurrent);
    Assert(ppsNext);

    //  consider: set up some dummy starting rectangle if psCurrent = NULL

    //  check for strong/weak directional check
    //  distinguish between primary and secondary directions
    //  if no template->this is a form, not a datadoc->weak detection
    pdfr = getOwner();
    if ( pdfr )
    {
        fStrictInDirection = pdfr->IsPrimaryDirection(Direction);
    }
    else
    {
        fStrictInDirection = FALSE;
    }


    hr = THR(super::NextControl(Direction, psCurrent, &psNextControl, this, fStrictInDirection));

    if ( ! hr )
    {
        if ( psNextControl->IsSelectChild() )
        {
            hr = THR(psNextControl->NextControl(Direction,psCurrent,ppsNext, this));
        }
        else
        {
            *ppsNext = psNextControl;
        }
    }
    else if ( ! pdfr->TBag()->_fArrowStaysInFrame && _pParent != pSiteCaller)
    {
        Assert(_pParent);
        hr = _pParent ? THR(_pParent->NextControl(Direction,psCurrent,ppsNext,this)) : S_FALSE;
    }
*/
    RRETURN1(hr, S_FALSE);
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
CBaseFrame::HandleMessage(CMessage * pMessage, CSite * pSite)
{
    HRESULT hr = S_FALSE;

    TraceTag((tagBaseFrame,"CBaseFrame::HandleMessage"));

    if (pMessage && (pMessage->message >= WM_KEYFIRST && pMessage->message <= WM_KEYLAST))
    {
        getOwner()->FireKeyboardEvent(pMessage->message, (int*)&(pMessage->wParam));
        if (!pMessage->wParam)
        {
            // key was eaten by VB event handler
            hr = S_OK;
            goto Cleanup;
        }

        if (pMessage->message == WM_KEYDOWN)
        {
            hr = DoKeyAction((int)pMessage->wParam, TRUE, 0);
            if (!hr)
                goto Cleanup;
        }
    }
    else if (pMessage->message >= WM_MOUSEFIRST && pMessage->message <= WM_MOUSELAST)
    {
        POINT pt;
        CPointl ptl;

        pt.x = MAKEPOINTS(pMessage->lParam).x;
        pt.y = MAKEPOINTS(pMessage->lParam).y;

        //  Transform the mouse coordinates into himetric
        _pDoc->HimetricFromDevice(&ptl, pt);
        getOwner()->FireMouseEvent(pMessage->message, ptl.x, ptl.y, pMessage->wParam);
    }


    if (S_FALSE == hr)
    {
        hr = THR(super::HandleMessage(pMessage, pSite));
    }

Cleanup:
    RRETURN1(hr,S_FALSE);

}
//+-End-of-method-------------------------------------------------------------






//+---------------------------------------------------------------------------
//
//  Member:     CBaseFrame::DoKeyAction
//
//  Synopsis:   Look up and execute the method handler for this keystroke
//
//  Returns:    the result of the action, or S_FALSE if no action for the key.
//
//  Comments:   Uses the class static table
//
//----------------------------------------------------------------------------

HRESULT
CBaseFrame::DoKeyAction(UINT vk, BOOL fDown, UINT flags)
{
    HRESULT hr = S_FALSE;
    KEY_MAP * pKeyTable;

    pKeyTable = FindKeyHandler(vk, fDown, flags);

    if (pKeyTable)
    {
        hr = THR((((CBase *) this)->*pKeyTable->pfnKeyHandler)
        (pKeyTable->lParam));

    }

    RRETURN1(hr, S_FALSE);
}





//+---------------------------------------------------------------------------
//
//  Member:     CBaseFrame::DoKeyAction
//
//  Synopsis:   Look up and execute the method handler for this keystroke
//
//  Returns:    the result of the action, or S_FALSE if no action for the key.
//
//  Comments:   Uses the class static table
//
//----------------------------------------------------------------------------

KEY_MAP *
CBaseFrame::FindKeyHandler(UINT vk, BOOL fDown, UINT flags)
{
    HRESULT hr = S_FALSE;
    KEY_MAP * pKeyTable;
    KEY_MAP * pKeyTableEnd;
    int cKeyActions;

    pKeyTable = GetKeyMap();
    cKeyActions = GetKeyMapSize();

    if ( pKeyTable == NULL )
        goto CleanUp;

    for ( pKeyTableEnd = pKeyTable + cKeyActions - 1; pKeyTable <= pKeyTableEnd; pKeyTable++ )
    {
        if ( (pKeyTable->vk == vk) &&
             (fDown == pKeyTable->fDown) &&
             (pKeyTable->fModifiers == MODIFIERS(flags)) )
        {
            return (pKeyTable);
        }
    }

CleanUp:
    return 0;
}






//+---------------------------------------------------------------------------
//
//  Member:     CBaseFrame::GetKeyMapSize
//
//  Synopsis:   Returns the keymap of the class
//
//  Returns:    address of the keymap
//
//  Comments:   virtual
//
//----------------------------------------------------------------------------

KEY_MAP * CBaseFrame::GetKeyMap(void)
{
    return(NULL);
}






//+---------------------------------------------------------------------------
//
//  Member:     CBaseFrame::GetKeyMapSize
//
//  Synopsis:   Returns the size of the keymap of the class
//
//  Returns:    size of the keymap
//
//  Comments:   virtual
//
//----------------------------------------------------------------------------

int CBaseFrame::GetKeyMapSize(void)
{
    return -1;
}




//+---------------------------------------------------------------------------
//
//  Member:    CBaseFrame::FireKeyboardEvent
//
//  Synopsis:  checks the msg, fires needed events
//
//  Returns:
//
//  Comments:
//
//----------------------------------------------------------------------------

#pragma warning(disable:4702)
// compiler bug: bogus "unreachable code" on class-to-interfacepointer casts

void
CBaseFrame::FireKeyboardEvent(UINT msg, int *pInfo)
{
    CReturnInteger ri(*pInfo);

    switch (msg)
    {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            getOwner()->FireStdControlEvent_KeyDown((IDispatch *) &ri,VBShiftState());
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            getOwner()->FireStdControlEvent_KeyUp((IDispatch *) &ri, VBShiftState());
            break;

        case WM_CHAR:
        case WM_SYSCHAR:
            getOwner()->FireStdControlEvent_KeyPress((IDispatch *) &ri);
            break;
    }

    *pInfo = ri.GetValue();

    return;
}
//-end-of-Method--------------------------------------------------------------




//+---------------------------------------------------------------------------
//
//  Member:    CBaseFrame::FireMouseEvent
//
//  Synopsis:  checks the msg, fires needed events
//
//  Returns:
//
//  Comments:
//
//----------------------------------------------------------------------------

void
CBaseFrame::FireMouseEvent(UINT msg, int x, int y, WORD wParam)
{
    POINTF       ptf;
    VARIANT_BOOL fCancel;

    _pDoc->UserFromDocument(&ptf, x, y);

    switch (msg)
    {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            getOwner()->FireDataFrameEvents_MouseDown(
                    // this calculates VB_LBUTTON, VB_RBUTTON, VB_MBUTTON
                    1 << ((msg - WM_LBUTTONDOWN) / 3),
                    VBShiftState(),
                    ptf.x,
                    ptf.y);
            break;

        case WM_MOUSEMOVE:
            getOwner()->FireDataFrameEvents_MouseMove(
                    VBButtonState(wParam),
                    VBShiftState(),
                    ptf.x,
                    ptf.y);
            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            if (_iMouseClickType < 2)
            {
                getOwner()->FireDataFrameEvents_MouseUp(
                        // this calculates VB_LBUTTON, VB_RBUTTON, VB_MBUTTON
                        1 << ((msg - WM_LBUTTONDOWN) / 3),
                        VBShiftState(),
                        ptf.x,
                        ptf.y);
            }
            if (_iMouseClickType == 0)
            {
                getOwner()->FireDataFrameEvents_Click();
            }
            _iMouseClickType = 0;
            break;

        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        {
            // BUGBUG1 need to handle cancel
            // BUGBUG2 compiler bug...
            fCancel = VB_FALSE;
            CReturnBoolean rb(fCancel);

            getOwner()->FireDataFrameEvents_DblClick((IReturnBoolean *) &rb);
            fCancel = rb.GetValue();

            _iMouseClickType = (fCancel) ? 2 : 1;
            break;
        }
    }
    return;
}
//-end-of-Method--------------------------------------------------------------





//+---------------------------------------------------------------------------
//
//  Member:    CBaseFrame::FireDragEvent
//
//  Synopsis:  Helper method fire the drag/drop events
//
//  Returns:   Returns S_OK if cool, S_FALSE if action is canceled
//
//
//----------------------------------------------------------------------------
HRESULT
CBaseFrame::FireDragEvent(fmDragState fmdragstate, DWORD grfKeystate, POINTL pt, DWORD *pdwEffect, VARIANT_BOOL *pfCancel)
{
    long            keyState = grfKeystate;
    POINTL          ptl;
    POINTF          ptf;
    HRESULT         hr=S_OK;
    CReturnBoolean  rb(VB_FALSE);
    CReturnEffect   re((fmDropEffect) *pdwEffect);

    Assert(_pDoc->_pInPlace->_pIDataAutoWrapper);

    if (fmdragstate != fmDragStateLeave)
    {
        _pDoc->DocumentFromScreen(&ptl, *(POINT *)&pt);
        _pDoc->UserFromDocument(&ptf, ptl.x, ptl.y);
    }
    else
    {
        ptf.x = .0f;
        ptf.y = .0f;
    }

    // now fire the event...
    getOwner()->FireDataFrameEvents_BeforeDragOver(
                          (IReturnBoolean *) &rb,
                          _pDoc->_pInPlace->_pIDataAutoWrapper,
                          ptf.x,
                          ptf.y,
                          fmdragstate,
                          (IReturnEffect *) &re,
                          VBShiftState(keyState));
    *pfCancel = rb.GetValue();
    *pdwEffect &= (DWORD) re.GetValue();

    RRETURN(hr);
}
//+-End-of-method-------------------------------------------------------------





//+---------------------------------------------------------------------------
//
//  Member:    CBaseFrame::FireDropEvent
//
//  Synopsis:  Helper method fire the drop event
//
//  Returns:   Returns S_OK if cool, S_FALSE if action is canceled
//
//
//----------------------------------------------------------------------------
HRESULT
CBaseFrame::FireDropEvent(DWORD grfKeystate, POINTL pt, DWORD *pdwEffect, VARIANT_BOOL *pfCancel)
{
    long            keyState = grfKeystate;
    POINTL          ptl;
    POINTF          ptf;
    HRESULT         hr=S_OK;
    CReturnBoolean  rb(VB_FALSE);
    CReturnEffect   re((fmDropEffect) *pdwEffect);

    Assert(_pDoc->_pInPlace->_pIDataAutoWrapper);

    _pDoc->DocumentFromScreen(&ptl, *(POINT *)&pt);
    _pDoc->UserFromDocument(&ptf, ptl.x, ptl.y);

    // now fire the event...
    getOwner()->FireDataFrameEvents_BeforeDropOrPaste(
                          (IReturnBoolean *) &rb,
                          fmActionDragDrop,
                          _pDoc->_pInPlace->_pIDataAutoWrapper,
                          ptf.x,
                          ptf.y,
                          (IReturnEffect *) &re,
                          VBShiftState(keyState));
    *pfCancel = rb.GetValue();
    *pdwEffect = (DWORD) re.GetValue();

    RRETURN(hr);
}
//+-End-of-method-------------------------------------------------------------
#pragma warning(default:4702)



#if PRODUCT_97
//-----------------------------------------------------------------------------
//  Member      CBaseFrame::GetRecordState
//
//  Synopsis    Gets the _iRecordState property
//
//  Arguments   piRecordState  pointer where the property goes
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CBaseFrame::GetRecordState(int * piRecordState)
{
    TraceTag((tagBaseFrame,"CBaseFrame::GetRecordState"));

    if (piRecordState == NULL)
        return E_INVALIDARG;

    *piRecordState = 0; // BUGBUG implement
    return S_OK;
};


//-----------------------------------------------------------------------------
//  Member      CBaseFrame::SetRecordState
//
//  Synopsis    Sets the _iRecordState property
//
//  Arguments   iRecordState  new value for property
//
//  Returns     HRESULT
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CBaseFrame::SetRecordState(int iRecordState)
{
    TraceTag((tagBaseFrame,"CBaseFrame::SetRecordState"));

    // BUGBUG implement

    return S_OK;
};


//-----------------------------------------------------------------------------
//
//  Member      CBaseFrame::GetLayoutCurrent
//
//  Synopsis    Gets the _fLayoutCurrent property
//
//  Arguments   pfLayoutCurrent      pointer where the _fLayoutCurrent address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CBaseFrame::GetLayoutCurrent(VARIANT_BOOL* pfLayoutCurrent)
{
    TraceTag((tagBaseFrame,"CBaseFrame::GetLayoutCurrent"));
    RRETURN(GetBool(pfLayoutCurrent, _fCurrent));
}

//-----------------------------------------------------------------------------
//
//  Member      CBaseFrame::SetLayoutCurrent
//
//  Synopsis    Sets the _fLayoutCurrent property
//
//  Arguments   fLayoutCurrent      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CBaseFrame::SetLayoutCurrent(VARIANT_BOOL  fLayoutCurrent)
{
    TraceTag((tagBaseFrame,"CBaseFrame::SetLayoutCurrent"));

    HRESULT hr = getOwner()->SetCurrent(this, TRUE);
    RRETURN(CloseErrorInfo(hr));
}

HRESULT CBaseFrame::GetActiveInstance (IDispatch ** ppdispActiveInstance) { return getOwner()->GetActiveInstance(ppdispActiveInstance); }
HRESULT CBaseFrame::GetAllowAdditions (VARIANT_BOOL * pfAllowAdditions) { return getOwner()->GetAllowAdditions(pfAllowAdditions); }
HRESULT CBaseFrame::SetAllowAdditions (VARIANT_BOOL fAllowAdditions) { return getOwner()->SetAllowAdditions(fAllowAdditions); }
HRESULT CBaseFrame::GetAllowDeletions (VARIANT_BOOL * pfAllowDeletions) { return getOwner()->GetAllowDeletions(pfAllowDeletions); }
HRESULT CBaseFrame::SetAllowDeletions (VARIANT_BOOL fAllowDeletions) { return getOwner()->SetAllowDeletions(fAllowDeletions); }
HRESULT CBaseFrame::GetAllowEditing (VARIANT_BOOL * pfAllowEditing) { return getOwner()->GetAllowEditing(pfAllowEditing); }
HRESULT CBaseFrame::SetAllowEditing (VARIANT_BOOL fAllowEditing) { return getOwner()->SetAllowEditing(fAllowEditing); }
HRESULT CBaseFrame::GetAllowFilters (VARIANT_BOOL * pfAllowFilters) { return getOwner()->GetAllowFilters(pfAllowFilters); }
HRESULT CBaseFrame::SetAllowFilters (VARIANT_BOOL fAllowFilters) { return getOwner()->SetAllowFilters(fAllowFilters); }
HRESULT CBaseFrame::GetAllowUpdating (VARIANT_BOOL * piAllowUpdating) { return getOwner()->GetAllowUpdating(piAllowUpdating); }
HRESULT CBaseFrame::SetAllowUpdating (VARIANT_BOOL iAllowUpdating) { return getOwner()->SetAllowUpdating(iAllowUpdating); }
HRESULT CBaseFrame::GetAlternateBackColor (OLE_COLOR * pcolorAlternateBackColor) { return getOwner()->GetAlternateBackColor(pcolorAlternateBackColor); }
HRESULT CBaseFrame::SetAlternateBackColor (OLE_COLOR colorAlternateBackColor) { return getOwner()->SetAlternateBackColor(colorAlternateBackColor); }
HRESULT CBaseFrame::GetAlternateInterval (int * piAlternateInterval) { return getOwner()->GetAlternateInterval(piAlternateInterval); }
HRESULT CBaseFrame::SetAlternateInterval (int iAlternateInterval) { return getOwner()->SetAlternateInterval(iAlternateInterval); }
HRESULT CBaseFrame::GetAutoEject (VARIANT_BOOL * pfAutoEject) { return getOwner()->GetAutoEject(pfAutoEject); }
HRESULT CBaseFrame::SetAutoEject (VARIANT_BOOL fAutoEject) { return getOwner()->SetAutoEject(fAutoEject); }
HRESULT CBaseFrame::GetColumnSpacing (int * piColumnSpacing) { return getOwner()->GetColumnSpacing(piColumnSpacing); }
HRESULT CBaseFrame::SetColumnSpacing (int iColumnSpacing) { return getOwner()->SetColumnSpacing(iColumnSpacing); }
HRESULT CBaseFrame::GetRowSpacing (int * piRowSpacing) { return getOwner()->GetRowSpacing(piRowSpacing); }
HRESULT CBaseFrame::SetRowSpacing (int iRowSpacing) { return getOwner()->SetRowSpacing(iRowSpacing); }
HRESULT CBaseFrame::GetCommitSync (int * piCommitSync) { return getOwner()->GetCommitSync(piCommitSync); }
HRESULT CBaseFrame::SetCommitSync (int iCommitSync) { return getOwner()->SetCommitSync(iCommitSync); }
HRESULT CBaseFrame::GetCommitWhat (int * piCommitWhat) { return getOwner()->GetCommitWhat(piCommitWhat); }
HRESULT CBaseFrame::SetCommitWhat (int iCommitWhat) { return getOwner()->SetCommitWhat(iCommitWhat); }
HRESULT CBaseFrame::GetCommitWhen (int * piCommitWhen) { return getOwner()->GetCommitWhen(piCommitWhen); }
HRESULT CBaseFrame::SetCommitWhen (int iCommitWhen) { return getOwner()->SetCommitWhen(iCommitWhen); }
HRESULT CBaseFrame::GetContinousForm (VARIANT_BOOL * pfContinousForm) { return getOwner()->GetContinousForm(pfContinousForm); }
HRESULT CBaseFrame::SetContinousForm (VARIANT_BOOL fContinousForm) { return getOwner()->SetContinousForm(fContinousForm); }
HRESULT CBaseFrame::GetDataOnly (VARIANT_BOOL * pfDataOnly) { return getOwner()->GetDataOnly(pfDataOnly); }
HRESULT CBaseFrame::SetDataOnly (VARIANT_BOOL fDataOnly) { return getOwner()->SetDataOnly(fDataOnly); }
HRESULT CBaseFrame::GetDateGrouping (int * piDateGrouping) { return getOwner()->GetDateGrouping(piDateGrouping); }
HRESULT CBaseFrame::SetDateGrouping (int iDateGrouping) { return getOwner()->SetDateGrouping(iDateGrouping); }
HRESULT CBaseFrame::GetDefaultEditing (int * piDefaultEditing) { return getOwner()->GetDefaultEditing(piDefaultEditing); }
HRESULT CBaseFrame::SetDefaultEditing (int iDefaultEditing) { return getOwner()->SetDefaultEditing(iDefaultEditing); }

HRESULT CBaseFrame::GetFooter (IDataFrame ** ppFooter) { return getOwner()->GetFooter(ppFooter); }
HRESULT CBaseFrame::GetHeader (IDataFrame ** ppHeader) { return getOwner()->GetHeader(ppHeader); }
HRESULT CBaseFrame::GetDirtyDataColor (OLE_COLOR * pcolorDirtyDataColor) { return getOwner()->GetDirtyDataColor(pcolorDirtyDataColor); }
HRESULT CBaseFrame::SetDirtyDataColor (OLE_COLOR colorDirtyDataColor) { return getOwner()->SetDirtyDataColor(colorDirtyDataColor); }
HRESULT CBaseFrame::GetDirtyPencil (VARIANT_BOOL * pfDirtyPencil) { return getOwner()->GetDirtyPencil(pfDirtyPencil); }
HRESULT CBaseFrame::SetDirtyPencil (VARIANT_BOOL fDirtyPencil) { return getOwner()->SetDirtyPencil(fDirtyPencil); }
HRESULT CBaseFrame::GetDisplayWhen (int * piDisplayWhen) { return getOwner()->GetDisplayWhen(piDisplayWhen); }
HRESULT CBaseFrame::SetDisplayWhen (int iDisplayWhen) { return getOwner()->SetDisplayWhen(iDisplayWhen); }
HRESULT CBaseFrame::GetFooterText (BSTR * pbstrFooter) { return getOwner()->GetFooterText(pbstrFooter); }
HRESULT CBaseFrame::SetFooterText (LPTSTR bstrFooter) { return getOwner()->SetFooterText(bstrFooter); }
HRESULT CBaseFrame::GetForceNewPage (int * piForceNewPage) { return getOwner()->GetForceNewPage(piForceNewPage); }
HRESULT CBaseFrame::SetForceNewPage (int iForceNewPage) { return getOwner()->SetForceNewPage(iForceNewPage); }
HRESULT CBaseFrame::GetFormatCount (long * plFormatCount) { return getOwner()->GetFormatCount(plFormatCount); }
HRESULT CBaseFrame::SetFormatCount (long lFormatCount) { return getOwner()->SetFormatCount(lFormatCount); }
HRESULT CBaseFrame::GetFullIntervals (VARIANT_BOOL * pfFullIntervals) { return getOwner()->GetFullIntervals(pfFullIntervals); }
HRESULT CBaseFrame::SetFullIntervals (VARIANT_BOOL fFullIntervals) { return getOwner()->SetFullIntervals(fFullIntervals); }
HRESULT CBaseFrame::GetGroupInterval (int * piGroupInterval) { return getOwner()->GetGroupInterval(piGroupInterval); }
HRESULT CBaseFrame::SetGroupInterval (int iGroupInterval) { return getOwner()->SetGroupInterval(iGroupInterval); }
HRESULT CBaseFrame::GetGroupOn (int * piGroupOn) { return getOwner()->GetGroupOn(piGroupOn); }
HRESULT CBaseFrame::SetGroupOn (int iGroupOn) { return getOwner()->SetGroupOn(iGroupOn); }
HRESULT CBaseFrame::GetHeaderText (BSTR * pbstrHeader) { return getOwner()->GetHeaderText(pbstrHeader); }
HRESULT CBaseFrame::SetHeaderText (LPTSTR bstrHeader) { return getOwner()->SetHeaderText(bstrHeader); }
HRESULT CBaseFrame::GetHideDuplicates (VARIANT_BOOL * pfHideDuplicates) { return getOwner()->GetHideDuplicates(pfHideDuplicates); }
HRESULT CBaseFrame::SetHideDuplicates (VARIANT_BOOL fHideDuplicates) { return getOwner()->SetHideDuplicates(fHideDuplicates); }
HRESULT CBaseFrame::GetItemsAcross (int * piItemsAcross) { return getOwner()->GetItemsAcross(piItemsAcross); }
HRESULT CBaseFrame::SetItemsAcross (int iItemsAcross) { return getOwner()->SetItemsAcross(iItemsAcross); }
HRESULT CBaseFrame::GetKeepTogether (int * piKeepTogether) { return getOwner()->GetKeepTogether(piKeepTogether); }
HRESULT CBaseFrame::SetKeepTogether (int iKeepTogether) { return getOwner()->SetKeepTogether(iKeepTogether); }
HRESULT CBaseFrame::GetLayoutForPrint (int * piLayoutForPrint) { return getOwner()->GetLayoutForPrint(piLayoutForPrint); }
HRESULT CBaseFrame::SetLayoutForPrint (int iLayoutForPrint) { return getOwner()->SetLayoutForPrint(iLayoutForPrint); }
HRESULT CBaseFrame::GetLockRecord (VARIANT_BOOL * pfLockRecord) { return getOwner()->GetLockRecord(pfLockRecord); }
HRESULT CBaseFrame::SetLockRecord (VARIANT_BOOL fLockRecord) { return getOwner()->SetLockRecord(fLockRecord); }
HRESULT CBaseFrame::GetMoveLayout (VARIANT_BOOL * pfMoveLayout) { return getOwner()->GetMoveLayout(pfMoveLayout); }
HRESULT CBaseFrame::SetMoveLayout (VARIANT_BOOL fMoveLayout) { return getOwner()->SetMoveLayout(fMoveLayout); }
HRESULT CBaseFrame::GetNavigationButtons (VARIANT_BOOL * pfNavigationButtons) { return getOwner()->GetNavigationButtons(pfNavigationButtons); }
HRESULT CBaseFrame::SetNavigationButtons (VARIANT_BOOL fNavigationButtons) { return getOwner()->SetNavigationButtons(fNavigationButtons); }
HRESULT CBaseFrame::GetNewRowOrCol (int * piNewRowOrCol) { return getOwner()->GetNewRowOrCol(piNewRowOrCol); }
HRESULT CBaseFrame::SetNewRowOrCol (int iNewRowOrCol) { return getOwner()->SetNewRowOrCol(iNewRowOrCol); }
HRESULT CBaseFrame::GetNewStar (VARIANT_BOOL * pfNewStar) { return getOwner()->GetNewStar(pfNewStar); }
HRESULT CBaseFrame::SetNewStar (VARIANT_BOOL fNewStar) { return getOwner()->SetNewStar(fNewStar); }
HRESULT CBaseFrame::GetNextRecord (VARIANT_BOOL * pfNextRecord) { return getOwner()->GetNextRecord(pfNextRecord); }
HRESULT CBaseFrame::SetNextRecord (VARIANT_BOOL fNextRecord) { return getOwner()->SetNextRecord(fNextRecord); }
HRESULT CBaseFrame::GetNormalDataColor (OLE_COLOR * pcolorNormalDataColor) { return getOwner()->GetNormalDataColor(pcolorNormalDataColor); }
HRESULT CBaseFrame::SetNormalDataColor (OLE_COLOR colorNormalDataColor) { return getOwner()->SetNormalDataColor(colorNormalDataColor); }
HRESULT CBaseFrame::GetOneFooterPerPage (VARIANT_BOOL * pfOneFooterPerPage) { return getOwner()->GetOneFooterPerPage(pfOneFooterPerPage); }
HRESULT CBaseFrame::SetOneFooterPerPage (VARIANT_BOOL fOneFooterPerPage) { return getOwner()->SetOneFooterPerPage(fOneFooterPerPage); }
HRESULT CBaseFrame::GetOneHeaderPerPage (VARIANT_BOOL * pfOneHeaderPerPage) { return getOwner()->GetOneHeaderPerPage(pfOneHeaderPerPage); }
HRESULT CBaseFrame::SetOneHeaderPerPage (VARIANT_BOOL fOneHeaderPerPage) { return getOwner()->SetOneHeaderPerPage(fOneHeaderPerPage); }
HRESULT CBaseFrame::GetOpenArgs (BSTR * pbstrOpenArgs) { return getOwner()->GetOpenArgs(pbstrOpenArgs); }
HRESULT CBaseFrame::SetOpenArgs (LPTSTR bstrOpenArgs) { return getOwner()->SetOpenArgs(bstrOpenArgs); }
HRESULT CBaseFrame::GetOutline (int * piOutline) { return getOwner()->GetOutline(piOutline); }
HRESULT CBaseFrame::SetOutline (int iOutline) { return getOwner()->SetOutline(iOutline); }
HRESULT CBaseFrame::GetOutlineCollapseIcon (IDispatch** pOutlineCollapseIcon) { return getOwner()->GetOutlineCollapseIcon(pOutlineCollapseIcon); }
HRESULT CBaseFrame::SetOutlineCollapseIcon (IDispatch* OutlineCollapseIcon) { return getOwner()->SetOutlineCollapseIcon(OutlineCollapseIcon); }
HRESULT CBaseFrame::SetOutlineCollapseIconByRef (IDispatch * OutlineCollapseIcon) { return SetOutlineCollapseIcon(OutlineCollapseIcon);}
HRESULT CBaseFrame::GetOutlineExpandIcon (IDispatch** pOutlineExpandIcon) { return getOwner()->GetOutlineExpandIcon(pOutlineExpandIcon); }
HRESULT CBaseFrame::SetOutlineExpandIcon (IDispatch* OutlineExpandIcon) { return getOwner()->SetOutlineExpandIcon(OutlineExpandIcon); }
HRESULT CBaseFrame::SetOutlineExpandIconByRef (IDispatch * OutlineExpandIcon) { return SetOutlineExpandIcon(OutlineExpandIcon);}
HRESULT CBaseFrame::GetOutlineLeafIcon (IDispatch** pOutlineLeafIcon) { return getOwner()->GetOutlineLeafIcon(pOutlineLeafIcon); }
HRESULT CBaseFrame::SetOutlineLeafIcon (IDispatch* OutlineLeafIcon) { return getOwner()->SetOutlineLeafIcon(OutlineLeafIcon); }
HRESULT CBaseFrame::SetOutlineLeafIconByRef (IDispatch* OutlineLeafIcon) { return SetOutlineLeafIcon(OutlineLeafIcon); }
HRESULT CBaseFrame::GetOutlinePrint (int * piOutlinePrint) { return getOwner()->GetOutlinePrint(piOutlinePrint); }
HRESULT CBaseFrame::SetOutlinePrint (int iOutlinePrint) { return getOwner()->SetOutlinePrint(iOutlinePrint); }
HRESULT CBaseFrame::GetOutlineShowWhen (int * piOutlineShowWhen) { return getOwner()->GetOutlineShowWhen(piOutlineShowWhen); }
HRESULT CBaseFrame::SetOutlineShowWhen (int iOutlineShowWhen) { return getOwner()->SetOutlineShowWhen(iOutlineShowWhen); }
HRESULT CBaseFrame::GetParentPosition (int * piParentPosition) { return getOwner()->GetParentPosition(piParentPosition); }
HRESULT CBaseFrame::SetParentPosition (int iParentPosition) { return getOwner()->SetParentPosition(iParentPosition); }
HRESULT CBaseFrame::GetPrintCount (long * plPrintCount) { return getOwner()->GetPrintCount(plPrintCount); }
HRESULT CBaseFrame::SetPrintCount (long lPrintCount) { return getOwner()->SetPrintCount(lPrintCount); }
HRESULT CBaseFrame::GetPrintSection (VARIANT_BOOL * pfPrintSection) { return getOwner()->GetPrintSection(pfPrintSection); }
HRESULT CBaseFrame::SetPrintSection (VARIANT_BOOL fPrintSection) { return getOwner()->SetPrintSection(fPrintSection); }
HRESULT CBaseFrame::GetQBFAutoExecute (VARIANT_BOOL * pfQBFAutoExecute) { return getOwner()->GetQBFAutoExecute(pfQBFAutoExecute); }
HRESULT CBaseFrame::SetQBFAutoExecute (VARIANT_BOOL fQBFAutoExecute) { return getOwner()->SetQBFAutoExecute(fQBFAutoExecute); }
HRESULT CBaseFrame::GetQBFAvailable (VARIANT_BOOL * pfQBFAvailable) { return getOwner()->GetQBFAvailable(pfQBFAvailable); }
HRESULT CBaseFrame::SetQBFAvailable (VARIANT_BOOL fQBFAvailable) { return getOwner()->SetQBFAvailable(fQBFAvailable); }
HRESULT CBaseFrame::GetQBFMode (VARIANT_BOOL * pfQBFMode) { return getOwner()->GetQBFMode(pfQBFMode); }
HRESULT CBaseFrame::SetQBFMode (VARIANT_BOOL fQBFMode) { return getOwner()->SetQBFMode(fQBFMode); }
HRESULT CBaseFrame::GetQBFShowData (VARIANT_BOOL * pfQBFShowData) { return getOwner()->GetQBFShowData(pfQBFShowData); }
HRESULT CBaseFrame::SetQBFShowData (VARIANT_BOOL fQBFShowData) { return getOwner()->SetQBFShowData(fQBFShowData); }
HRESULT CBaseFrame::GetReadOnlyDataColor (OLE_COLOR * pcolorReadOnlyDataColor) { return getOwner()->GetReadOnlyDataColor(pcolorReadOnlyDataColor); }
HRESULT CBaseFrame::SetReadOnlyDataColor (OLE_COLOR colorReadOnlyDataColor) { return getOwner()->SetReadOnlyDataColor(colorReadOnlyDataColor); }
HRESULT CBaseFrame::GetRecordCount (long * plRecordCount) { return getOwner()->GetRecordCount(plRecordCount); }
HRESULT CBaseFrame::SetRecordCount (long lRecordCount) { return getOwner()->SetRecordCount(lRecordCount); }
HRESULT CBaseFrame::GetRecordLocks (int * piRecordLocks) { return getOwner()->GetRecordLocks(piRecordLocks); }
HRESULT CBaseFrame::SetRecordLocks (int iRecordLocks) { return getOwner()->SetRecordLocks(iRecordLocks); }
HRESULT CBaseFrame::GetRecordPosition (int * piRecordPosition) { return getOwner()->GetRecordPosition(piRecordPosition); }
HRESULT CBaseFrame::SetRecordPosition (int iRecordPosition) { return getOwner()->SetRecordPosition(iRecordPosition); }
HRESULT CBaseFrame::GetRecordSelectors (VARIANT_BOOL * pfRecordSelectors) { return getOwner()->GetRecordSelectors(pfRecordSelectors); }
HRESULT CBaseFrame::SetRecordSelectors (VARIANT_BOOL fRecordSelectors) { return getOwner()->SetRecordSelectors(fRecordSelectors); }
HRESULT CBaseFrame::GetRecordset (IDispatch ** ppdispRecordset) { return getOwner()->GetRecordset(ppdispRecordset); }
HRESULT CBaseFrame::GetRecordsetClone (IDispatch ** ppdispRecordsetClone) { return getOwner()->GetRecordsetClone(ppdispRecordsetClone); }
HRESULT CBaseFrame::GetRecordSourceSample (VARIANT_BOOL * pfRecordSourceSample) { return getOwner()->GetRecordSourceSample(pfRecordSourceSample); }
HRESULT CBaseFrame::SetRecordSourceSample (VARIANT_BOOL fRecordSourceSample) { return getOwner()->SetRecordSourceSample(fRecordSourceSample); }
HRESULT CBaseFrame::GetRecordSourceSync (int * piRecordSourceSync) { return getOwner()->GetRecordSourceSync(piRecordSourceSync); }
HRESULT CBaseFrame::SetRecordSourceSync (int iRecordSourceSync) { return getOwner()->SetRecordSourceSync(iRecordSourceSync); }
HRESULT CBaseFrame::GetRecordSourceType (int * piRecordSourceType) { return getOwner()->GetRecordSourceType(piRecordSourceType); }
HRESULT CBaseFrame::SetRecordSourceType (int iRecordSourceType) { return getOwner()->SetRecordSourceType(iRecordSourceType); }
HRESULT CBaseFrame::GetRepeat (VARIANT_BOOL * pfRepeat) { return getOwner()->GetRepeat(pfRepeat); }
HRESULT CBaseFrame::SetRepeat (VARIANT_BOOL fRepeat) { return getOwner()->SetRepeat(fRepeat); }
HRESULT CBaseFrame::GetRequeryWhen (int * piRequeryWhen) { return getOwner()->GetRequeryWhen(piRequeryWhen); }
HRESULT CBaseFrame::SetRequeryWhen (int iRequeryWhen) { return getOwner()->SetRequeryWhen(iRequeryWhen); }
HRESULT CBaseFrame::GetResetPages (VARIANT_BOOL * pfResetPages) { return getOwner()->GetResetPages(pfResetPages); }
HRESULT CBaseFrame::SetResetPages (VARIANT_BOOL fResetPages) { return getOwner()->SetResetPages(fResetPages); }
HRESULT CBaseFrame::GetScrollHeight (long * plScrollHeight) { return getOwner()->GetScrollHeight(plScrollHeight); }
HRESULT CBaseFrame::GetScrollLeft (long * plScrollLeft) { return getOwner()->GetScrollLeft(plScrollLeft); }
HRESULT CBaseFrame::SetScrollLeft (long lScrollLeft) { return getOwner()->SetScrollLeft(lScrollLeft); }
HRESULT CBaseFrame::GetScrollTop (long * plScrollTop) { return getOwner()->GetScrollTop(plScrollTop); }
HRESULT CBaseFrame::SetScrollTop (long lScrollTop) { return getOwner()->SetScrollTop(lScrollTop); }
HRESULT CBaseFrame::GetScrollWidth (long * plScrollWidth) { return getOwner()->GetScrollWidth(plScrollWidth); }
HRESULT CBaseFrame::GetSelectedControlBackCol (OLE_COLOR * pcolor) { return getOwner()->GetSelectedControlBackCol(pcolor); }
HRESULT CBaseFrame::SetSelectedControlBackCol (OLE_COLOR color) { return getOwner()->SetSelectedControlBackCol(color); }
HRESULT CBaseFrame::GetShowGridLines (int * piShowGridLines) { return getOwner()->GetShowGridLines(piShowGridLines); }
HRESULT CBaseFrame::SetShowGridLines (int iShowGridLines) { return getOwner()->SetShowGridLines(iShowGridLines); }
HRESULT CBaseFrame::GetShowNewRowAtBottom (VARIANT_BOOL * pfShowNewRowAtBottom) { return getOwner()->GetShowNewRowAtBottom(pfShowNewRowAtBottom); }
HRESULT CBaseFrame::SetShowNewRowAtBottom (VARIANT_BOOL fShowNewRowAtBottom) { return getOwner()->SetShowNewRowAtBottom(fShowNewRowAtBottom); }
HRESULT CBaseFrame::GetShowWhen (int * piShowWhen) { return getOwner()->GetShowWhen(piShowWhen); }
HRESULT CBaseFrame::SetShowWhen (int iShowWhen) { return getOwner()->SetShowWhen(iShowWhen); }
HRESULT CBaseFrame::GetSnakingDirection (int * piDirection) { return getOwner()->GetSnakingDirection(piDirection); }
HRESULT CBaseFrame::SetSnakingDirection (int iDirection) { return getOwner()->SetSnakingDirection(iDirection); }
HRESULT CBaseFrame::GetSpecialEffect (int * piSpecialEffect) { return getOwner()->GetSpecialEffect(piSpecialEffect); }
HRESULT CBaseFrame::SetSpecialEffect (int iSpecialEffect) { return getOwner()->SetSpecialEffect(iSpecialEffect); }
HRESULT CBaseFrame::GetViewMode (int * piViewMode) { return getOwner()->GetViewMode(piViewMode); }
HRESULT CBaseFrame::SetViewMode (int iViewMode) { return getOwner()->SetViewMode(iViewMode); }
HRESULT CBaseFrame::GetLinkMasterFields (BSTR * pbstrLinkMasterFields) { return getOwner()->GetLinkMasterFields(pbstrLinkMasterFields); }
HRESULT CBaseFrame::SetLinkMasterFields (LPTSTR lpwstrLinkMasterFields) { return getOwner()->SetLinkMasterFields(lpwstrLinkMasterFields); }
HRESULT CBaseFrame::GetLinkChildFields (BSTR * pbstrLinkChildFields) { return getOwner()->GetLinkChildFields(pbstrLinkChildFields); }
HRESULT CBaseFrame::SetLinkChildFields (LPTSTR lpwstrLinkChildFields) { return getOwner()->SetLinkChildFields(lpwstrLinkChildFields); }
HRESULT CBaseFrame::GetDataDirty (VARIANT_BOOL * pfDataDirty) { return getOwner()->GetDataDirty(pfDataDirty); }
HRESULT CBaseFrame::SetDataDirty (VARIANT_BOOL fDataDirty) { return getOwner()->SetDataDirty(fDataDirty); }
HRESULT CBaseFrame::GetNewData (VARIANT_BOOL * pfNewData) { return getOwner()->GetNewData(pfNewData); }
HRESULT CBaseFrame::SetNewData (VARIANT_BOOL fNewData) { return getOwner()->SetNewData(fNewData); }
HRESULT CBaseFrame::GetRecordSelector (IControl ** ppRecordSelector) { return getOwner()->GetRecordSelector(ppRecordSelector); }
HRESULT CBaseFrame::SetRecordSelector (IControl * pRecordSelector) { return getOwner()->SetRecordSelector(pRecordSelector); }
HRESULT CBaseFrame::GetArrowEnterFieldBehavior (int * piArrowEnterFieldBehavior) { return getOwner()->GetArrowEnterFieldBehavior(piArrowEnterFieldBehavior); }
HRESULT CBaseFrame::SetArrowEnterFieldBehavior (int    iArrowEnterFieldBehavior) { return getOwner()->SetArrowEnterFieldBehavior(iArrowEnterFieldBehavior); }
HRESULT CBaseFrame::GetArrowKeyBehavior (int * piArrowKeyBehavior) { return getOwner()->GetArrowKeyBehavior(piArrowKeyBehavior); }
HRESULT CBaseFrame::SetArrowKeyBehavior (int    iArrowKeyBehavior) { return getOwner()->SetArrowKeyBehavior(iArrowKeyBehavior); }
HRESULT CBaseFrame::GetArrowKeys (int * piArrowKeys) { return getOwner()->GetArrowKeys(piArrowKeys); }
HRESULT CBaseFrame::SetArrowKeys (int    iArrowKeys) { return getOwner()->SetArrowKeys(iArrowKeys); }
HRESULT CBaseFrame::GetDynamicTabOrder (VARIANT_BOOL * pfDynamicTabOrder) { return getOwner()->GetDynamicTabOrder(pfDynamicTabOrder); }
HRESULT CBaseFrame::SetDynamicTabOrder (VARIANT_BOOL    fDynamicTabOrder) { return getOwner()->SetDynamicTabOrder(fDynamicTabOrder); }
HRESULT CBaseFrame::GetMoveAfterEnter (int * piMoveAfterEnter) { return getOwner()->GetMoveAfterEnter(piMoveAfterEnter); }
HRESULT CBaseFrame::SetMoveAfterEnter (int   piMoveAfterEnter) { return getOwner()->SetMoveAfterEnter(piMoveAfterEnter); }
HRESULT CBaseFrame::GetTabEnterFieldBehavior (int * piTabEnterFieldBehavior) { return getOwner()->GetTabEnterFieldBehavior(piTabEnterFieldBehavior); }
HRESULT CBaseFrame::SetTabEnterFieldBehavior (int    iTabEnterFieldBehavior) { return getOwner()->SetTabEnterFieldBehavior(iTabEnterFieldBehavior); }
HRESULT CBaseFrame::GetTabOut (int * piTabOut) { return getOwner()->GetTabOut(piTabOut); }
HRESULT CBaseFrame::SetTabOut (int    iTabOut) { return getOwner()->SetTabOut(iTabOut); }
HRESULT CBaseFrame::GetShowFooters(VARIANT_BOOL *pfShowFooters) { return getOwner()->GetShowFooters(pfShowFooters) ;}
HRESULT CBaseFrame::SetShowFooters(VARIANT_BOOL fShowFooters) { return getOwner()->SetShowFooters(fShowFooters) ;}
HRESULT CBaseFrame::GetDirection (fmRepeatDirection * piDirection) { return getOwner()->GetDirection(piDirection); }
HRESULT CBaseFrame::SetDirection (fmRepeatDirection iDirection) { return getOwner()->SetDirection(iDirection); }
HRESULT CBaseFrame::GetMinCols (long * plMinCols) { return getOwner()->GetMinCols (plMinCols); }
HRESULT CBaseFrame::SetMinCols (long lMinCols) { return getOwner()->SetMinCols (lMinCols); }
HRESULT CBaseFrame::GetMaxCols (long * plMaxCols) { return getOwner()->GetMaxCols (plMaxCols); }
HRESULT CBaseFrame::SetMaxCols (long lMaxCols) { return getOwner()->SetMaxCols (lMaxCols); }
HRESULT CBaseFrame::GetNewRecordShow (VARIANT_BOOL *pfNewRecordShow) { return getOwner()->GetNewRecordShow (pfNewRecordShow); }
HRESULT CBaseFrame::SetNewRecordShow (VARIANT_BOOL fNewRecordShow) { return getOwner()->SetNewRecordShow (fNewRecordShow); }
HRESULT CBaseFrame::GetDirty (VARIANT_BOOL * pfDirty) { return getOwner()->GetDirty(pfDirty); }
HRESULT CBaseFrame::SetDirty (VARIANT_BOOL fDirty) { return getOwner()->SetDirty(fDirty); }
HRESULT CBaseFrame::GetDatabase (BSTR * pbstrDatabase) { return getOwner()->GetDatabase(pbstrDatabase); }
HRESULT CBaseFrame::SetDatabase (LPTSTR lpwstrDatabase) { return getOwner()->SetDatabase(lpwstrDatabase); }
HRESULT CBaseFrame::GetDetail (IDataFrame ** ppDetail) { return getOwner()->GetDetail(ppDetail); }
// #if DBG==1
HRESULT CBaseFrame::GetRunTestcode (int * piRunTestcode) { Assert(piRunTestcode); *piRunTestcode =-1; return S_OK; }
HRESULT CBaseFrame::SetRunTestcode (int   iRunTestcode)  { return getOwner()->SetRunTestcode(iRunTestcode); }
// #endif



#endif // PRODUCT_97



//+---------------------------------------------------------------------------
//
//  Member:     SetCurrent
//
//  Synopsis:   Set the layout to be currently positioned.
//              Set/Reset the record selector of the layout to be current.
//
//  Arguments:  yesNo           set/reset.
//
//----------------------------------------------------------------------------

HRESULT
CBaseFrame::SetCurrent (BOOL fCurrent)
{
    TraceTag((tagBaseFrame, "CBaseFrame::SetCurrent"));

    _fCurrent = fCurrent;

    return S_OK;
}

/*
    Property methods implemented on base frames
*/
//+--------------------------------------------------------------------------
//
//  Member:     CBaseFrame::GetBackColor
//
//  Synopsis:   returns the _colorBack property
//
//  Arguments:  pColor      placeholder for property
//
//  Returns     HRESULT
//
//---------------------------------------------------------------------------

STDMETHODIMP
CBaseFrame::GetBackColor(OLE_COLOR * pColor)
{
    TraceTag((tagBaseFrame,"CBaseFrame::GetBackColor"));
    RRETURN(GetColor(pColor, _colorBack));
}


//+--------------------------------------------------------------------------
//
//  Member:     CBaseFrame::SetBackColor
//
//  Synopsis:   Sets the _colorBack property
//
//  Arguments:  Color      new value for property
//
//  Returns     HRESULT
//
//---------------------------------------------------------------------------

STDMETHODIMP
CBaseFrame::SetBackColor(OLE_COLOR Color)
{
    TraceTag((tagBaseFrame,"CBaseFrame::SetBackColor"));

    RRETURN(SetColor(&_colorBack, Color, DISPID_BACKCOLOR));
}

//-----------------------------------------------------------------------------
//
//  Member      CBaseFrame::GetForeColor
//
//  Synopsis    Gets the _colorForeColor property
//
//  Arguments   pcolorForeColor      pointer where the _colorForeColor address goes to
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CBaseFrame::GetForeColor(OLE_COLOR* pcolorForeColor)
{
    RRETURN(GetColor(pcolorForeColor, _colorFore));
};

//-----------------------------------------------------------------------------
//
//  Member      CBaseFrame::SetForeColor
//
//  Synopsis    Sets the _colorForeColor property
//
//  Arguments   colorForeColor      new value for property
//
//  Returns     HRESULT
//-----------------------------------------------------------------------------

STDMETHODIMP
CBaseFrame::SetForeColor(OLE_COLOR  colorForeColor)
{
    TraceTag((tagBaseFrame,"CBaseFrame::SetForeColor"));
    RRETURN(SetColor(&(_colorFore), colorForeColor, DISPID_FORECOLOR));

};




//+-------------------------------------------------------------------------
//
//  Method:     CBaseFrame::RefreshData
//
//  Synopsis:   Notify that the rowset we bound to is changed
//              the base implementation just tells all the children about it
//
//--------------------------------------------------------------------------
HRESULT
CBaseFrame::RefreshData()
{
    int c = _arySites.Size();
    CSite ** ppSite = _arySites;

    while (c--)
    {
        IGNORE_HR((*ppSite++)->RefreshData());
    }

    RRETURN (super::RefreshData());
}


//+-------------------------------------------------------------------------
//
//  Method:     CBaseFrame::DataSourceChanged
//
//  Synopsis:   Notify that the rowset we bound to is changed
//              the base implementation just tells all the children about it
//
//--------------------------------------------------------------------------
void
CBaseFrame::DataSourceChanged()
{
    super::DataSourceChanged();

    int c = _arySites.Size();
    CSite ** ppSite = _arySites;

    while (c--)
    {
        (*ppSite++)->DataSourceChanged();
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CBaseFrame::SetHeight
//
//  Synopsis:   instead of calling move, like the parentsite impl
//              this one just calls proposeMove, and let the property
//              update code do the rest
//
//--------------------------------------------------------------------------
HRESULT
CBaseFrame::SetHeight(long Height)
{
#if 0
    return super::SetHeight(Height);
#else
    CRectl rcl;

    GetProposed(this, &rcl);

    rcl.bottom = rcl.top + Height;

    SetProposed(this, &rcl);

    PropagateChange(STDPROPID_XOBJ_HEIGHT);

    return S_OK;
#endif
}
//--End of Method-----------------------------------------------------------




//+-------------------------------------------------------------------------
//
//  Method:     CBaseFrame::SetWidth
//
//  Synopsis:   instead of calling move, like the parentsite impl
//              this one just calls proposeMove, and let the property
//              update code do the rest
//
//--------------------------------------------------------------------------
HRESULT
CBaseFrame::SetWidth(long Width)
{
#if 0
    return super::SetWidth(Width);
#else
    CRectl rcl;

    GetProposed(this, &rcl);

    rcl.right = rcl.left + Width;

    SetProposed(this, &rcl);

    PropagateChange(STDPROPID_XOBJ_WIDTH);

    return S_OK;
#endif
}
//--End of Method-----------------------------------------------------------



//+-------------------------------------------------------------------------
//
//  Method:     CBaseFrame::GetRowSource
//
//  Synopsis:   instead of calling CSite::GetRowSource, the owner (DataFrame)
//              gets the RowSource.
//
//--------------------------------------------------------------------------
HRESULT
CBaseFrame:: GetRowSource (BSTR * pbstrRowSource)
{
    return getOwner()->GetRowSource(pbstrRowSource);
}
//--End of Method-----------------------------------------------------------



//+-------------------------------------------------------------------------
//
//  Method:     CBaseFrame::SetRowSource
//
//  Synopsis:   instead of calling CSite::SetRowSource, the owner (DataFrame)
//              stores the RowSource.
//
//--------------------------------------------------------------------------
HRESULT
CBaseFrame::SetRowSource (LPTSTR bstrRowSource)
{
    return getOwner()->SetRowSource(bstrRowSource);
}
//--End of Method-----------------------------------------------------------



//+-------------------------------------------------------------------------
//
//  Method:     CBaseFrame::SetLeft
//
//  Synopsis:   instead of calling move, like the parentsite impl
//              this one just calls proposeMove, and let the property
//              update code do the rest
//
//--------------------------------------------------------------------------
HRESULT
CBaseFrame::SetLeft(long Left)
{
#if 0
    return super::SetLeft(Left);
#else
    CRectl rcl;

    GetProposed(this, &rcl);

    rcl.OffsetRect(Left - rcl.left, 0);

    SetProposed(this, &rcl);

    PropagateChange(STDPROPID_XOBJ_LEFT);

    return S_OK;
#endif
}
//--End of Method-----------------------------------------------------------



//+-------------------------------------------------------------------------
//
//  Method:     CBaseFrame::SetTop
//
//  Synopsis:   instead of calling move, like the parentsite impl
//              this one just calls proposeMove, and let the property
//              update code do the rest
//
//--------------------------------------------------------------------------
HRESULT
CBaseFrame::SetTop(long Top)
{
#if 0
    return super::SetTop(Top);
#else
    CRectl rcl;

    GetProposed(this, &rcl);

    rcl.OffsetRect(0, (Top - rcl.top));

    SetProposed(this, &rcl);

    PropagateChange(STDPROPID_XOBJ_TOP);

    return S_OK;
#endif
}
//--End of Method-----------------------------------------------------------





//+-------------------------------------------------------------------------
//
//  Method:     CBaseFrame::Move
//
//  Synopsis:   instead of calling move, like the parentsite impl
//              this one just calls proposeMove, and let the property
//              update code do the rest
//
//--------------------------------------------------------------------------
HRESULT
CBaseFrame::Move(long Left, long Top, long Width, long Height)
{

    CRectl rcl;

    if (Width < 0 || Height < 0)
        return E_INVALIDARG;

    rcl.left = Left;
    rcl.top = Top;
    rcl.right = rcl.left + Width;
    rcl.bottom = rcl.top + Height;

    if (rcl != _rcl)
    {
        SetProposed(this, &rcl);
        }
    PropagateChange(STDPROPID_XOBJ_WIDTH);

    return S_OK;

}
//--End of Method-----------------------------------------------------------



BOOL CBaseFrame::IsAutosizeHeight ()
{
    BOOL f = IsAutosize();
    if (f)
    {
        fmEnAutoSize enAutoSize;
        GetAutosizeStyle (&enAutoSize);
        f = enAutoSize & fmEnAutoSizeHorizontal;
    }
    return f;
}

BOOL CBaseFrame::IsAutosizeWidth ()
{
    BOOL f = IsAutosize();
    if (f)
    {
        fmEnAutoSize enAutoSize;
        GetAutosizeStyle (&enAutoSize);
        f = enAutoSize & fmEnAutoSizeVertical;
    }
    return f;
}


BOOL CBaseFrame::IsAutosizeDir (int iDirection)
{
    Assert (iDirection == 0 || iDirection == 1);
    BOOL f = IsAutosize();
    if (f)
    {
        fmEnAutoSize enAutoSize;
        GetAutosizeStyle (&enAutoSize);
        f = enAutoSize & (1<<iDirection);
    }
    return f;
}


BOOL CBaseFrame::IsAutosize ()
{
   fmEnAutoSize enAutoSize;
   GetAutosizeStyle (&enAutoSize);
   return enAutoSize != fmEnAutoSizeNone;
}

// BUGBUG: can't pass compilation of GetControls
// HRESULT CBaseFrame::GetControls (IControls ** ppControls) { return getOwner()->GetControls(ppControls); }
HRESULT CBaseFrame::GetMouseIcon (IDispatch ** ppDisp) { return getOwner()->GetMouseIcon(ppDisp); }
HRESULT CBaseFrame::SetMouseIcon (IDispatch * pDisp) { return getOwner()->SetMouseIcon(pDisp); }
HRESULT CBaseFrame::SetMouseIconByRef (IDispatch * pDisp) { return SetMouseIcon(pDisp);}
HRESULT CBaseFrame::GetMousePointer (fmMousePointer * pMousePointer) { return getOwner()->GetMousePointer(pMousePointer); }
HRESULT CBaseFrame::SetMousePointer (fmMousePointer MousePointer) { return getOwner()->SetMousePointer(MousePointer); }
HRESULT CBaseFrame::GetScrollBars (fmScrollBars * piScrollBars) { return getOwner()->GetScrollBars(piScrollBars); }
HRESULT CBaseFrame::SetScrollBars (fmScrollBars iScrollBars) { return getOwner()->SetScrollBars(iScrollBars); }
HRESULT CBaseFrame::GetShow3D (int * piShow3D) { return getOwner()->GetShow3D(piShow3D); }
HRESULT CBaseFrame::SetShow3D (int iShow3D) { return getOwner()->SetShow3D(iShow3D); }
HRESULT CBaseFrame::GetEnabled (VARIANT_BOOL * pfEnabled) { return getOwner()->GetEnabled(pfEnabled); }
HRESULT CBaseFrame::SetEnabled (VARIANT_BOOL fEnabled) { return getOwner()->SetEnabled(fEnabled); }
HRESULT CBaseFrame::GetDeferedPropertyUpdate (VARIANT_BOOL *pfDefered) { return getOwner()->GetDeferedPropertyUpdate(pfDefered) ; };
HRESULT CBaseFrame::SetDeferedPropertyUpdate (VARIANT_BOOL fDefered) { return getOwner()->SetDeferedPropertyUpdate(fDefered) ; };
HRESULT CBaseFrame::GetShowHeaders(VARIANT_BOOL *pfShowHeaders) { return getOwner()->GetShowHeaders(pfShowHeaders) ;}
HRESULT CBaseFrame::SetShowHeaders(VARIANT_BOOL  fShowHeaders) { return getOwner()->SetShowHeaders(fShowHeaders) ;}
HRESULT CBaseFrame::GetListBoxStyle (fmListBoxStyles * peListBoxStyle) { return getOwner()->GetListBoxStyle(peListBoxStyle); }
HRESULT CBaseFrame::SetListBoxStyle (fmListBoxStyles    eListBoxStyle) { return getOwner()->SetListBoxStyle(eListBoxStyle); }
HRESULT CBaseFrame::GetMultiSelect (fmMultiSelect * peMultiSelect) { return getOwner()->GetMultiSelect(peMultiSelect); }
HRESULT CBaseFrame::SetMultiSelect (fmMultiSelect    eMultiSelect) { return getOwner()->SetMultiSelect(eMultiSelect); }
HRESULT CBaseFrame::GetListStyle (fmListStyle * peListStyle) { return getOwner()->GetListStyle(peListStyle); }
HRESULT CBaseFrame::SetListStyle (fmListStyle    eListStyle) { return getOwner()->SetListStyle(eListStyle); }
HRESULT CBaseFrame::GetAutosizeStyle (fmEnAutoSize *enAutosize) { return getOwner()->GetAutosizeStyle(enAutosize); }
HRESULT CBaseFrame::SetAutosizeStyle (fmEnAutoSize enAutosize) { return getOwner()->SetAutosizeStyle(enAutosize); }
HRESULT CBaseFrame::GetListIndex (long *plListindex) { return getOwner()->GetListIndex(plListindex); }
HRESULT CBaseFrame::SetListIndex (long lListindex) { return getOwner()->SetListIndex(lListindex); }
HRESULT CBaseFrame::GetListItemSelected (long lItemIndex, VARIANT_BOOL *pfSelected) { return getOwner()->GetListItemSelected(lItemIndex, pfSelected); }
HRESULT CBaseFrame::SetListItemSelected (long lItemIndex, VARIANT_BOOL fSelected) { return getOwner()->SetListItemSelected(lItemIndex, fSelected); }
HRESULT CBaseFrame::GetTopIndex (long *plTopIndex) { return getOwner()->GetTopIndex(plTopIndex); }
HRESULT CBaseFrame::SetTopIndex (long lTopIndex) { return getOwner()->SetTopIndex(lTopIndex); }
HRESULT CBaseFrame::GetVisibleCount (long *plVisibleCount) { return getOwner()->GetVisibleCount(plVisibleCount); }
HRESULT CBaseFrame::GetMinRows (long * plMinRows) { return getOwner()->GetMinRows (plMinRows); }
HRESULT CBaseFrame::SetMinRows (long lMinRows) { return getOwner()->SetMinRows (lMinRows); }
HRESULT CBaseFrame::GetMaxRows (long * plMaxRows) { return getOwner()->GetMaxRows (plMaxRows); }
HRESULT CBaseFrame::SetMaxRows (long lMaxRows) { return getOwner()->SetMaxRows (lMaxRows); }

HRESULT CBaseFrame::GetRecordNumber (ULONG *pulRecordNumber)
{
    *pulRecordNumber = 0;
    return E_FAIL;
}

//
//  end of file
//
/////////////////////////////////////////////////////////////////////////////////
