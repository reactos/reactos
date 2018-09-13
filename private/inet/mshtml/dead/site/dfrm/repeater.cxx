//-----------------------------------------------------------------------------
//
//  Maintained by: alexa
//
//  Microsoft DataDocs
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       src\doc\datadoc\repeater.cxx
//
//  Contents:   Repeater Frame implementation.
//
//  Classes:    CRepeaterFrame
//
//  Functions:  None.
//
//  History:
//  08/23/94    created by alexa
//  12/02/94    Lajosf  Get rid of DebugBreak in Move
//  02/25/95    JerryD Integrate DIRT OLE DB provider
//
//-----------------------------------------------------------------------------

#include "headers.hxx"
#include "dfrm.hxx"

DeclareTag(tagRepeaterFrame,"src\\ddoc\\datadoc\\repeater.cxx","RepeaterFrame");
extern TAG tagPropose;
extern TAG tagDataFrameDebugPaint;
const int cRepeatRate = 60;
const int cMaxRepeatDivider=4;

static const BOOL CREATE_TO_FIT_OPTIMIZATION = 1;
//static const int MAXLONG = 0x7FFFFFFF;
//static const int MINLONG = 0x80000000;
static const int MAXLOCAL = 64;

static unsigned int s_MaxCached = 92;

// Note: the last 2 items in the key actions array should be VK_UP,VK_DOWN.
KEY_MAP CRepeaterFrame::s_aKeyActions[]= {
    {VK_HOME,  KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, fmScrollActionBegin},
    {VK_END,   KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, fmScrollActionEnd},
    {VK_PRIOR, KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, fmScrollActionPageUp},
    {VK_NEXT,  KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, fmScrollActionPageDown},
    {VK_SPACE, KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)ProcessSpaceKey,0},
    {VK_UP,    KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, fmScrollActionLineUp},
    {VK_DOWN,  KEY_DOWN, NO_MODIFIERS, (KEYHANDLER)KeyScroll, fmScrollActionLineDown},
};

int CRepeaterFrame::s_cKeyActions = ARRAY_SIZE(s_aKeyActions);



//+-----------------------------------------------
//  This is a helper array for row/page scrolling
//------------------------------------------------
int CRepeaterFrame::s_aiRowOffset[4] =
{
    0,      //  These two elements are initialized
    0,      //  on each call, they hold the [+/-] number of visible rows
    -1,     //  The last two are static for row up/down.
    1
};



inline BOOL IsEnoughRealEstate(HRESULT hr)
{
    return SUCCEEDED(hr)? hr & 1 : 0;
}

CSite::CLASSDESC CRepeaterFrame::s_classdesc =
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
            SITEDESC_PARENT |               // _dwFlags
            SITEDESC_BASEFRAME  |
            SITEDESC_REPEATER,
            &IID_IDataFrame,                // _piidDispinterface
            0,                              // _lAccRole
            &CSite::s_PropCats,             // _pPropCats;
            NULL
        },
        s_apfnIControlElement,              // _pfnTearOff
    },
    ETAG_NULL,                              // _etag
};



//+---------------------------------------------------------------------------
//
//   POPULATE_UP = 0        POPULATE_DOWN = 1
//
//
//   Repeater                DetailFrame             DetailFrame
//
//  +------------------+    +------------------+    +------------------+
//  | _apNode[BEGIN=0] |--->| _apNode[NEXT=1]  |--->| _apNode[NEXT=1]  |-->NULL
//  +------------------+    +------------------+    +------------------+
//
//  +------------------+    +------------------+    +------------------+
//  | _apNode[END=1]   |--->| _apNode[PREV=0]  |--->| _apNode[PREV=0]  |-->NULL
//  +------------------+    +------------------+    +------------------+
//
//----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame
//
//  Synopsis:   Constructor ,creates CrepaterFrame from CLayouTemplate.
//              This constructor is called during the instantiation pass of the
//              DataDoc. It is called with a "new" syntax of the operator new
//              from the template class. For example:
//              new (this) CBaseFrame (this, info);
//
//  Arguments:  pTemplate       Pointer to a Template object.
//
//  Assumption: The "new (param)" syntax of new is used.
//
//----------------------------------------------------------------------------

CRepeaterFrame::CRepeaterFrame(
        CDoc * pDoc,
        CBaseFrame * pParent,
        CDetailFrame *pTemplate)
    : CBaseFrame(pDoc, pParent, pTemplate)
{
    TraceTag((tagRepeaterFrame,"CRepeaterFrame::constructor"));

    _iIndex = -1;
    _uRepeatInPage = 0;
    _fPopulateDirection = POPULATE_DOWN;

    _uCached = 0;                       // Number of cached layout frames.

    _pCurrent = NULL;

    _fSelectChild = TRUE;

    // repeaters always behave as streamed
    _fStreamed = TRUE;

}




//+---------------------------------------------------------------------------
//
//  Member:     InitInstance
//
//  Synopsis:   InitInstance member function is called after the clone
//              constructor (from Temaplate). This member function gets called
//              only for instance creation.
//
//  Arguments:  info        Procreation info structure,
//                          contains binding information
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::Init ()
{
    TraceTag((tagRepeaterFrame, "CRepeaterFrame::InitInstance"));

    HRESULT hr;

    hr = super::Init ();

    RRETURN(hr);
}




//+-------------------------------------------------------------------------
//
//  Method:     CRepeaterFrame::DataSourceChanged
//
//  Synopsis:   Notify that the rowset we bound to is changed
//
//--------------------------------------------------------------------------

void
CRepeaterFrame::DataSourceChanged()
{
    super::DataSourceChanged();
    FlushCache();
}




//+---------------------------------------------------------------------------
//
//  Member:     UpdatePropertyChanges
//
//  Synopsis:   checks if Tbag contains information
//              about changed properties
//
//  Argument:   BOOL fCompleted: this is a two pass function. First pass updates
//              after all updates, the bags get emptied
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::UpdatePropertyChanges(UPDATEPROPS updFlag)
{
    TraceTag((tagRepeaterFrame,"CRepeaterFrame::UpdatePropertyChanges"));

    HRESULT hr = S_OK;
    CBaseFrame *pbfr;
    CDetailFrame *pbfrTemplate = getRepeatedTemplate();

    if (pbfrTemplate->TBag()->_propertyChanges.IsTemplateModified()
        || TBag()->_propertyChanges.IsTemplateModified())
    {
        // give one guy the chance to propagate his changes back to the rootframe
        hr = pbfrTemplate->UpdatePropertyChanges(updFlag);
        FlushCache();
        pbfrTemplate->EmptyRecycle();
        goto Cleanup;
    }
    // if we came here, it means that we did not had to throw all those guys away
    //  ergo, walk the cache and call UpdatePropertyChanges on them

    pbfr = _apNode[BEGIN_NODE];

    while (SUCCEEDED(hr) && pbfr)
    {
        hr = pbfr->UpdatePropertyChanges(updFlag);
        pbfr = pbfr->_apNode[NEXT_NODE];
    }

Cleanup:
    RRETURN(hr);
}
//-+ End of Method-------------------------------------------------------------



//+---------------------------------------------------------------------------
//
//  Member:     ~CRepeaterFram
//
//  Synopsis:    Destructor.
//
//----------------------------------------------------------------------------

CRepeaterFrame::~CRepeaterFrame ()
{
    TraceTag((tagRepeaterFrame,"CRepeaterFrame::destructor"));

    // 2. All the subframes supposed to be deleted
    Assert (FRAMESCOUNTER == 0);

    // 3. All layout frames supposed to be detached from cache list.
    Assert (_apNode[BEGIN_NODE] == NULL &&  _apNode[END_NODE] == NULL);
}





//+---------------------------------------------------------------------------
//
//  Member:     CreateToFit
//
//  Synopsis:   Populate the repeater rectangle. Public
//              Populate rectangle with already generated instances and if there
//              is enough real estate create new layout frame instances.
//
//  Arguments:  CRectl &rclView         The view rectangle to populate.
//              dwFlags                 flags to use for site movement
//
//  Returns:    Returns S_OK if everything is fine, E_OUTOFMEMORY.
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::CreateToFit (IN const CRectl * prclView, IN DWORD dwFlags)
{
    TraceTag((tagRepeaterFrame, "CRepeaterFrame::CreateToFit"));



    _fDirection = getDirection();   // Has to be initialized before anything else.

    HRESULT         hr;
    CRectl          rclRealEstate;  // The "Real Estate" rectangle.
    CRectl          rcl;
    CSizel          szl;
    CSizel          szlRepeater;
    long            lRowTop;
    CBaseFrame  *   plfr = NULL;
    Edge            eRepeated = (Edge) _fDirection;
    Edge            eNonRepeated = (Edge) (1  - _fDirection);
    Edge            eTop = eRepeated;
    Edge            eBottom = (Edge)(eRepeated + 2);
    ULONG           uRowNumber = 0;
    unsigned int    uItemsNonRepDir = getItemsNonRepDir();
    CDataFrame  *   pdfr = getOwner();

    // Note: logicaly you might think that if _rcl is not
    // intersects with rclView then we don't need to populate at all, or at
    // least populate the intersect rectangle, but unfortunateley in order to
    // ensure on the outer level of the dataframe that enclose the repeater
    // that all the controls pushed from the view we need to poulate the view
    // size.
    // Although population should start from the correct position
    // _pointlPopulated.TopLeft(), not from rclView.TopLeft().

    _fPopulateDirection = ENSURE_BOOL(dwFlags & SITEMOVE_POPULATEDIR);

    CalcRectangle ();       // Calculate virtual size of the repeater.
    pdfr->GetDetailSectionRectl (&rcl);

    if (prclView->IsRectEmpty() || rcl.IsRectEmpty() || _rcl.IsRectEmpty())
    {
        rclRealEstate.SetRectEmpty();
        goto PopulateRealEstate;
    }

    if (_pCurrent)
    {
        hr = pdfr->FixupRow(_pCurrent);
        if (hr)
        {
            goto ErrorCantFixupRow;     // we couldn't fix the row
        }
    }

    if (_arySites.Size())
    {
        plfr = getLayoutFrame((_fPopulateDirection == POPULATE_DOWN) ?
               getTopFrameIdx() : getBottomFrameIdx() );
        // fix the row in case it was deleted
        if (plfr != _pCurrent)          // optimization not to fix current row twice
        {
            hr = pdfr->FixupRow((CDetailFrame *)plfr);
            if (hr)
            {
                goto ErrorCantFixupRow;     // we couldn't fix the row
            }
        }
        // the reason we calculate the row number of the detail frame is
        // that if template is changed we need to change the repeaters
        // "top" position.
        // BUGBUG: wont work for changing row spacing (we need to be able to
        // getOldPadding ()
        lRowTop = plfr->_rcl[eRepeated];
        // Note: we need to calculate lRowTop before calling FixupPosition
    }
    else
    {
        lRowTop = rcl[eRepeated];
        int iStartingPoint  = !_fPopulateDirection;

        if (!_apNode[iStartingPoint])
        {
            hr = GetNextLayoutFrame(0,0);
            if (hr)
            {
               // this can fail due to a variety of reasons, like cursor empty
               // no recordsource set, recordsource corrupt etc.
               // for now, we do not want to report this as an error back, so let's set
               // the return code
               goto Cleanup;
            }

        }
        plfr = _apNode[iStartingPoint];
        Assert (plfr);
    }

    if (_pCurrent)
    {
        // Move the _pCurrent to the correct position
        hr = FixupPosition (_pCurrent);
        if (hr)
        {
            goto ErrorCantFixupRow;
        }
    }

    if (_pCurrent != plfr)      // this check is an optimization not to call
    {                           // FixupPosition twice.
        hr = FixupPosition (plfr);
        if (hr)
        {
            goto ErrorCantFixupRow;
        }
    }

    if (!getTemplate()->_rcl.IsRectEmpty())
    {
        // rowNumber is 0-based (not to confuse with record number)
        uRowNumber = (plfr->_rcl[eRepeated] - _rcl[eRepeated])/(plfr->_rcl.Dimension(eRepeated) + getPadding(eRepeated));
    }
    szl = getTemplate()->_rcl.Size();
    szlRepeater = _rcl.Size();
    _rcl[eRepeated] = lRowTop - uRowNumber * (szl[eRepeated] + getPadding (eRepeated));
    if (_rcl[eRepeated] > rcl[eRepeated])
        _rcl[eRepeated] = rcl[eRepeated];
    _rcl.SetSize (szlRepeater);

    if (pdfr->IsScrollBar(eRepeated) && _arySites.Size())
    {
        // NOTE: the following code can be easily optimized.
        // Check if we need to slide the repeater
        CBaseFrame * plfrTop = getLayoutFrame(getTopFrameIdx());
        CBaseFrame * plfrBottom = getLayoutFrame(getBottomFrameIdx());
        LONG         lDim;

        if (plfrBottom->_rcl[eBottom] == _rcl[eBottom] &&
            _rcl[eBottom] < rcl[eBottom])
        {
            // we have reached the bottom of the repeater and the reapeater's
            // bottom is above the detail section
            _fPopulateDirection = POPULATE_UP;
            plfr = plfrBottom;
            lDim = _rcl[eBottom] - _rcl[eTop];
            _rcl[eBottom] = rcl[eBottom];
            _rcl[eTop] = _rcl[eBottom] - lDim;
        }
        else if (plfrTop->_rcl[eTop] == _rcl[eTop] &&
                 _rcl[eTop] > rcl[eTop])
        {
            // we have reached the top of the repeater and the reapeater's
            // top is below the detail section
            _fPopulateDirection = POPULATE_DOWN;
            plfr = plfrTop;
            lDim = _rcl[eTop] - _rcl[eBottom];
            _rcl[eTop] = rcl[eTop];
            _rcl[eBottom] = _rcl[eTop] - lDim;
        }

    }

    // Calculate the position of the real estate in the repeated direction.
    // make sure we start populating from the top of the detail section
    rclRealEstate[eNonRepeated] = _rcl[eNonRepeated];
    rclRealEstate[(Edge)(eNonRepeated + 2)] = _rcl[(Edge)(eNonRepeated + 2)];
    if (_fPopulateDirection == POPULATE_DOWN)
    {
        rclRealEstate[eTop] = rcl[eTop];
        rclRealEstate[eBottom] = (*(CRectl *)prclView)[eBottom] >= rcl[eTop]?
                                 min((*(CRectl *)prclView)[eBottom], rcl[eBottom]) :
                                 rcl[eTop]; // to ensure correct rectangle (r.bottom >= r.top)
    }
    else
    {
        rclRealEstate[eBottom] = rcl[eBottom];
        rclRealEstate[eTop] = (*(CRectl *)prclView)[eTop] <= rcl[eBottom]?
                              max((*(CRectl *)prclView)[eTop], rcl[eTop]) : rcl[eBottom];
        if (_rcl[eBottom] < rcl[eBottom])   // if _rcl's bottom is above rcl's bottom
        {
            _rcl[eBottom] = rcl[eBottom];   // reset the _rcl
            _rcl[eTop] = _rcl[eBottom] - szlRepeater[eRepeated];
        }
    }

    #ifndef ABSOLUTE_COORDINATES
    NORMALIZE(rclRealEstate);                   // Make the topLeft == (0,0).
    #endif

PopulateRealEstate:
    hr = Populate (plfr, rclRealEstate, 0, NO_LIMIT, dwFlags);

    if (SUCCEEDED(hr))
    {
        RemoveNotPopulatedSites();
#ifndef PRODUCT_96
        CalcProposedRectangle ();
#endif
    }

Cleanup:
    RRETURN1 (hr, S_FALSE);

ErrorCantFixupRow:
    FlushCache ();
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Member:     AddToSites
//
//  Synopsis:   overloaded for index management
//              this one special cases the ADDSITE_ADDTOLIST
//              currently only used from the repeater
//
//  Returns:    Returns result from parent
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::AddToSites(CSite *pSite, DWORD dwOperations, int iZOrder)
{
    HRESULT hr;

    if (dwOperations & ADDSITE_ADDTOLIST)
    {
        int c = FRAMESCOUNTER;

        if (iZOrder >= c)
            iZOrder = c;

        hr = THR(_arySites.EnsureSize(c + 1));

        if (hr)
            goto Cleanup;

        _arySites.Insert(iZOrder, pSite);
    }

    if (dwOperations & ADDSITE_AFTERINIT)
    {
        if (!(dwOperations & ADDSITE_NOINVALIDATE))
            pSite->Invalidate(NULL, 0);

        //  Transition to initial OLE state.
        //  Enforce minimum state = LOADED, maximum state = INPLACE

#ifdef NEVER
        // This transitioning does not make much sense for non-ole
        // sites so only do it for ole sites.
        // BUGBUG: (anandra) TransitionTo is now gone so need to
        // rework this.
        if (pSite->SiteDesc()->TestFlag(CSite::SITEDESC_OLESITE))
        {
            stateInit = _pDoc->State();

            if (stateInit < OS_LOADED)
                stateInit = OS_LOADED;
            else if (stateInit > OS_INPLACE)
                stateInit = OS_INPLACE;

            hr = THR(((COleSite *)pSite)->TransitionToBaselineState(stateInit));
            if (hr)
                goto Error;
        }
#endif
    }

Cleanup:

    RRETURN(hr);

#ifdef NEVER
// See bugbug above
Error:

    Verify(_arySites.DeleteByValue(pSite));
    goto Cleanup;
#endif
}




void
CRepeaterFrame::RemoveNotPopulatedSites()
{
    CSite       *   pSite;
    unsigned int    i;

    for (i = 0; i < (unsigned int)_arySites.Size(); i++)
    {
        pSite = _arySites[i];
        Assert(pSite);
        if (!((CBaseFrame*)pSite)->IsPopulated ())
        {
            // if site is not overlapping
            // delete from the site list.
            ((CBaseFrame*)pSite)->SetInSiteList(FALSE);
            pSite->Release();
            _arySites.Delete(i);
            i--;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CalcRectangle, virtual
//
//  Synopsis:   Calaculates the _rcl (virtual size) of the repeater.
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::CalcRectangle (BOOL fProposed)
{
    Edge            eTop = (Edge) getDirection();
    Edge            eLeft = (Edge)(1 - eTop);
    HRESULT         hr;
    ULONG           ulRows;
    unsigned int    uItemsNonRepDir = getItemsNonRepDir();
    CSizel          szl;
    CSizel          szlSpacing (getPadding(0), getPadding(1));
    long            lRepeatedDimension;
    CRectl          rclPropose;

    if (fProposed && getTemplate()->_fProposedSet)
    {
        getTemplate()->GetProposed(getTemplate(), &rclPropose);
        szl = rclPropose.Size();
    }
    else
    {
        szl = getTemplate()->_rcl.Size();
    }

    szl += szlSpacing;
    lRepeatedDimension = szl[eTop];

    hr = getOwner()->GetNumberOfRecords (&ulRows);
//  Assert (ulRows != 0);
//  Note: if cursor size is 0 the repeater's size is also 0
    Assert (uItemsNonRepDir != UNLIMITED);

    szl[eTop] *= (ulRows + uItemsNonRepDir -1)/uItemsNonRepDir;
    szl[eLeft] *= uItemsNonRepDir;

    szl -= szlSpacing;

    if (fProposed)
    {
        GetProposed(this, &rclPropose);
        rclPropose.SetSize (szl);
        SetProposed(this, &rclPropose);
    }
    else
    {
        _rcl.SetSize (szl);
    }

    return;
}




//+---------------------------------------------------------------------------
//
//  Member:     CalcProposedRectangle
//
//  Synopsis:   Calaculates the bounding rectangle of the repeater.
//              to calculate the repeater's rectangle after CreateToFit or
//              Scrolling.
//
//  Note:       The argument is passed only for DEBUGing purposes.
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::CalcProposedRectangle ()
{
    TraceTag((tagRepeaterFrame, "CRepeaterFrame::CalcBoundsRectangle"));



    if (_arySites.Size())
    {
        CRectl          rclPropose;
        CBaseFrame  *   pdfr;
        Edge            e = (Edge)getDirection();
        Edge            eNonRepeat = (Edge) (1 - e);

        GetProposed(this, &rclPropose);

        pdfr = getLayoutFrame (getTopFrameIdx());
        Assert (pdfr);
        rclPropose[e] = min (_rcl[e], pdfr->_rcl[e] );

        pdfr = getLayoutFrame (getBottomFrameIdx());
        Assert (pdfr);
        e = (Edge)(e + 2);
        rclPropose[e] = max (_rcl[e], pdfr->_rcl[e] );

#ifndef PRODUCT_97
        rclPropose[eNonRepeat] = pdfr->_rcl[eNonRepeat];    // instead of calling CalcRowRectangle
        eNonRepeat = (Edge) (eNonRepeat + 2);
        rclPropose[eNonRepeat] = pdfr->_rcl[eNonRepeat];
#else
        // Note: the dimension for not repeated direction is already
        // calculated in CalcRowRectangle.
#endif
        SetProposed(this, &rclPropose);
    }
    else
    {
        SetProposed(this, &_rcl);
    }

    return;
}



//+---------------------------------------------------------------------------
//
//  Member:     CalcRowRectangle
//
//  Synopsis:   Calaculates the bounding rectangle of a row/column in the
//              repeater.
//
//  @todo:      Optimization: Should be inline, called only ones from Populate
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::CalcRowRectangle (CRectl& rclRow, CRectl *prcl)
{
    TraceTag ((tagRepeaterFrame, "CRepeaterFrame::CalcRowRectangle"));
    if (_fPopulateDirection == POPULATE_DOWN)
    {
        rclRow.right = max (rclRow.right,  prcl->right);
        rclRow.bottom = max (rclRow.bottom, prcl->bottom);
    }
    else
    {
        rclRow.left = min (rclRow.left,  prcl->left);
        rclRow.top = min (rclRow.top, prcl->top);
    }

#if PRODUCT_97

    // Note: the _fDirection is set to NonRepeatedDirection at this point.
    // if the repeater is vertical then _fDirection now is set to horizontal.
    // Note IsVertical at this point is not acurate since the _fDirection is
    // flipped. (That's why I can't have an itellegent assert here).

    // One of the things we can do at this point is to set the maximum size
    // of the non repeated dimension of the proposed rectangle.

    CRectl rclPropose;

    GetProposed(this, &rclPropose);

    Edge        edgeNonRepeat = (Edge)_fDirection;

    if (rclPropose[edgeNonRepeat])
    {
        rclPropose[edgeNonRepeat] =
            min (rclPropose[edgeNonRepeat], rclRow[edgeNonRepeat]);
    }
    else
    {
        // If rclPropose is empty, then
        rclPropose[edgeNonRepeat] = rclRow[edgeNonRepeat];
    }

    edgeNonRepeat = (Edge)((Edge)_fDirection + 2);

    rclPropose[edgeNonRepeat] =
        max (rclPropose[edgeNonRepeat], rclRow[edgeNonRepeat]);

    SetProposed(this, &rclPropose);
#endif
    return;
}





#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::PaintBackground
//
//  Synopsis:   Paint the background
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::DrawBackground(CFormDrawInfo *pDI)
{
    RECT *prc = &_rc;

    if (pDI->_fAfterStartSite)
    {
        if (IsTagEnabled(tagDataFrameDebugPaint))
        {
            CRectl rcl;
            GetClientRectl(&rcl);
            if (rcl.IsRectEmpty())
            {
                goto Cleanup;
            }
            CBaseFrame *plfrBottom;
            if (_arySites.Size())
            {
                CBaseFrame *plfrTop = getLayoutFrame(getTopFrameIdx());
                Assert(plfrTop->_rcl.left <= rcl.left);
                Assert(plfrTop->_rcl.top <= rcl.top);
                plfrBottom = getLayoutFrame(getBottomFrameIdx());
            }
            else
            {
                plfrBottom = NULL;
            }
            if (getPadding(0) ||
                getPadding(1) ||
                !plfrBottom ||
                (plfrBottom &&
                plfrBottom->_rcl.bottom < rcl.bottom ||
                plfrBottom->_rcl.right < rcl.right))
            {
                HBRUSH hBrush = GetCachedBrush(RGB(0, 0, 255));
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
        }
    }

Cleanup:
    return S_OK;
}


#endif  // DBG == 1




//+------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::SelectSite
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
CRepeaterFrame::SelectSite(CSite * pSite, DWORD dwFlags)
{
    if (dwFlags & SS_SETSELECTIONSTATE)
    {
        if (pSite == this)
        {
            int             c;
            CSite **        ppSite;

            // set state on all children
            for (ppSite = _arySites, c = _arySites.Size();
                 --c >= 0;
                 ppSite++ )
            {
                Verify(!(*ppSite)->SelectSite((*ppSite), dwFlags));
            }

            return S_OK;
        }
        else
        {
            // Optimization: if _fCurrentSelected is set we check only
            // if the new instance is the current
            if (_fCurrentSelected && pSite != _pCurrent)
            {
                // right now pSite should be a detailframe
                Assert(pSite->TestClassFlag(SITEDESC_DETAILFRAME));

                ((CDetailFrame *)pSite)->SetSelectionElement(NULL);
                if (pSite->_fSelected)
                {
                    Verify(!pSite->SetSelected(FALSE));
                }

                return S_OK;
            }
        }
    }

    RRETURN(getOwner()->SelectSite(pSite, dwFlags));
}


//+---------------------------------------------------------------------------
//
//  Member:     CacheLookUpRow
//
//  Synopsis:   Walks the cache and tries to find the hrow
//
//  Arguments:  hrow        row to look for
//
//  Returns:    found frame, or 0, if not in cache
//
//
//----------------------------------------------------------------------------

CBaseFrame *
CRepeaterFrame::CacheLookUpRow(HROW hrow)
{
    CBaseFrame *plfr=_apNode[BEGIN_NODE];
    CDataLayerCursor *pCursor = NULL;

    if (THR(getOwner()->LazyGetDataLayerCursor(&pCursor)))
    {
        if (hrow != NULL || plfr->getHRow() != NULL)
        {
            plfr = NULL;
        }
    }
    else
    {
        while (plfr)
        {
            if (pCursor->IsSameRow(plfr->getHRow(), hrow))
            {
                break;
            }
            plfr = plfr->_apNode[NEXT_NODE];
        }
    }

    return plfr;
}
//-+ End of Method-------------------------------------------------------------





//+---------------------------------------------------------------------------
//
//  Member:     CacheFrame
//
//  Synopsis:   Insert the layout frame at the begining/end of the
//              double-linked list of layout frames. or in the middle
//
//  Arguments:  plfr        newly created layout frame to be inserted.
//              plfrBase    basepoint to insert
//
//  Assumption: 1. _fPopulateDirection is set.
//              2. Reference counter of the plfr is already 1 after the creation
//                 so we don't need to AddRef it.
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::CacheFrame (CBaseFrame *plfr, CBaseFrame *plfrBase)
{
    TraceTag ((tagRepeaterFrame, "CRepeaterFrame::CacheFrame"));

    Assert (plfr); // && plfr->_ulRefs == 1);

    if (!plfrBase)
    {
        // if based is not passed, set the base to the
        // BEGIN(POPULATE_UP) or END(POPULATE_DOWN) of the list
        plfrBase = _apNode[_fPopulateDirection];
    }

    if (plfrBase)
    {
        // get the NEXT(POPULATE_DOWN)/PREV(POPULATE_UP) pointer of the base.
        CBaseFrame * plfrNext = plfrBase->_apNode[_fPopulateDirection];

        // set the NEXT(POPULATE_DOWN)/PREV(POPULATE_UP) pointer of the new guy
        // to the next pointer of the base.
        plfr->_apNode[_fPopulateDirection]  = plfrNext;

        // set the NEXT(POPULATE_DOWN)/PREV(POPULATE_UP) pointer of the base to
        // the new guy
        plfrBase->_apNode[_fPopulateDirection] = plfr;

        // set the prev pointer of the new guy to the base
        plfr->_apNode[!_fPopulateDirection] = plfrBase;

        if (plfrNext)
        {
            // if the NEXT(POPULATE_DOWN)/PREV(POPULATE_UP) pointer is not null
            // set his  PREV(POPULATE_UP)/NEXT(POPULATE_DOWN) entry to the new guy
            plfrNext->_apNode[!_fPopulateDirection] = plfr;
        }


        // if the point of insertion of the new frame is at the
        // begining(POPULATE_UP)/end(POPULATE_DOWN)
        // of the cache list
        if (plfrBase == _apNode[_fPopulateDirection])
        {
            _apNode[_fPopulateDirection] = plfr;
        }
    }
    else
    {
        // Case of the first frame is added to the cache
        Assert(_uCached == 0 && _apNode[BEGIN_NODE] == 0 && _apNode[END_NODE] == 0);

        _apNode[BEGIN_NODE]      =
        _apNode[END_NODE]        = plfr;

        plfr->_apNode[PREV_NODE] =
        plfr->_apNode[NEXT_NODE] = NULL;
    }

    _uCached++;

    plfr->SetPopulated (TRUE);
    plfr->_fInvalidated = FALSE;

    // Adjust cache.
    PutCacheFrameInRecycle();

#if DBG==1
    AssertIfCacheInvalid ();
#endif

    return;
}



//+---------------------------------------------------------------------------
//
//  Member:     RemoveFromCache
//
//  Synopsis:   Remove frame from the cache list
//
//  Arguments:  plfr        frame to be removed from cahce
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::RemoveFromCache (CBaseFrame *plfr)
{
    TraceTag ((tagRepeaterFrame, "CRepeaterFrame::RemoveFromCache"));
    Assert (plfr);

    if (!plfr->_apNode[PREV_NODE])
    {
        _apNode[BEGIN_NODE] = plfr->_apNode[NEXT_NODE];
    }
    else
    {
        (plfr->_apNode[PREV_NODE])->_apNode[NEXT_NODE] = plfr->_apNode[NEXT_NODE];
    }

    if (!plfr->_apNode[NEXT_NODE])
    {
        _apNode[END_NODE] = plfr->_apNode[PREV_NODE];
    }
    else
    {
        (plfr->_apNode[NEXT_NODE])->_apNode[PREV_NODE] = plfr->_apNode[PREV_NODE];
    }

    plfr->_apNode[NEXT_NODE] = plfr->_apNode[PREV_NODE] = 0;

    _uCached--;

#if DBG==1
    AssertIfCacheInvalid ();
#endif
}




#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     AssertIfCacheInvalid
//
//  Synopsis:   Run a cache check.
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::AssertIfCacheInvalid ()
{
    CBaseFrame *plfr = _apNode[BEGIN_NODE];
    int         i = 0;
    while (plfr)
    {
        i++;
        Assert(plfr->_pParent);
        plfr = plfr->_apNode[NEXT_NODE];
    }
    Assert (i == (int)_uCached);
}


#endif


#if DBG==1 && !defined(PRODUCT_97)
void CRepeaterFrame::CheckFrameSizes(CBaseFrame *plfr)
{
    #if defined (FRAME_TEST)
    #pragma message("COmpiling Checkframesizes")
    CSite       **  ppSite = plfr->_arySites;
    CSite       **  ppSiteEnd = ppSite + plfr->_arySites.Size();
    CSite       *   pSite;
    long            lWidth, lHeight;

    extern DECLARE_GLOBAL(long, g_lRowWidth);
    extern DECLARE_GLOBAL(long, g_lRowHeight);

    if (IsListBoxStyle())
    {
        for (lHeight = 0, lWidth = 0; ppSite < ppSiteEnd; ppSite++)
        {
            pSite = *ppSite;
            lHeight = max(lHeight, pSite->_rcl.Height());
            lWidth =+ pSite->_rcl.Width();
        }

        Assert(plfr->_rcl.Width() == g_lRowWidth);
        Assert(plfr->_rcl.Height() == g_lRowHeight);
        Assert(lWidth == g_lRowWidth);
        Assert(lHeight == g_lRowHeight);
    }
    #endif
}
#endif





//+---------------------------------------------------------------------------
//
//  Member:     Detach (DeleteCache)
//
//  Synopsis:   Remove all the layout frames from the cache list.
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::Detach ()
{
    TraceTag ((tagRepeaterFrame, "CRepeaterFrame::Detach"));

    if (_pCurrent)
        _pCurrent->Release();
    _pCurrent = NULL;

    // 2. Get Rid of the cache
    FlushCache();

    super::Detach ();
}









//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame :: GetNextLayoutFrame
//
//  Parameter:  plfrOrigin          get the next layout frame from the passed
//                                  laout.
//              pplfr               pointer to the pointer to the next layout.
//                                  can be 0, if not of intrest
//              ULONG ulPrefetch    can be greater 1, is 1 by default
//
//  Synopsis:   Get next row, either from Cursor or from the array of cached
//              rows. Get next(POPULATE_DOWN)/previous(POPULATE_DOWN).
//
//  Assumption: _fPopulatedirection     is set.
//
//  Returns:    Sets the pointer to a cached or newly created layout frame in
//              *pplfr.
//              1. Returns S_OK if returns a cached pointer or a pointer to a
//                 newly created rpw.
//              2. Returns S_FALSE and *pplfr = NULL in case of the reaching
//                 the last row in set (in case of POPULATE_DOWN);
//                 or reaching the first row in set (in case of POPULATE_UP).
//              3. Returns an error if GetNextRows fails.
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::GetNextLayoutFrame (IN CBaseFrame *plfrOrigin,
                                    OUT CBaseFrame **pplfr,
                                    ULONG ulPrefetch)
{
    TraceTag ((tagRepeaterFrame, "CRepeaterFrame::GetNextLayoutFrame"));

    HRESULT hr = S_OK;
    ULONG   ulFetched;
    HROW    ahrow[MAXLOCAL];

    Assert(ulPrefetch <= MAXLOCAL);

    if ((long)ulPrefetch <= 0)
    {
        ulPrefetch = 1;
    }
    else if (ulPrefetch > MAXLOCAL)
    {
        ulPrefetch = MAXLOCAL;
    }

    // now first get the data in by telling the cursor to fetch
    // ulPrefetch number of rows

    CBaseFrame  *   plfrCurrent = plfrOrigin;
    CBaseFrame  *   plfr = NULL;
    unsigned        k;
    BOOL            fCreate;
    BOOL            fCheckOutCurrent;
    CDataLayerCursor * pCursor = NULL;

    hr = GetNextRows (ahrow, ulPrefetch, &ulFetched, plfrOrigin);
    getOwner()->LazyGetDataLayerCursor(&pCursor);

    if (hr)
    {
        if (hr == S_FALSE)
        {
            if (ulFetched == 0 && plfrOrigin)
            {
                goto Error;     // this is the case when we ask for Next Layout
            }                   // from the Empty layout
        }
        else
        {
            // this is the case when there is no cursor or the cursor is empty
            // we always want a fake instance unless it's a ListBox
            if (plfrOrigin)
            {
                goto Error;     // real error
            }
            // otherwise we need to create a fake layout
        }
        if (!IsListBoxStyle())
        {
            // create the new=empty row
            ahrow [0] = 0;
            ulFetched = 1;
            hr = S_OK;
        }
        else
        {
            goto Error;
        }

    }

    Assert (ulFetched);

    fCheckOutCurrent = _pCurrent && !_pCurrent->IsInSiteList();

    // now we walk through the fetched hrows and figure out if
    // they are already in the cache

    for ( k = 0; k < ulFetched; k++)
    {
        if (plfrCurrent)
        {
            plfr = plfrCurrent->_apNode[_fPopulateDirection];
        }

        fCreate = TRUE;

        if (plfr)
        {
            #if DBG==1
            HROW   hrowCur = plfr->getHRow();
            #endif
            fCreate = ! (pCursor ? pCursor->IsSameRow(plfr->getHRow(), ahrow[k])
                                 : plfr->getHRow() == ahrow[k]);
        }
        else if (fCheckOutCurrent)
        {
            #if DBG==1
            HROW    hrowCur = _pCurrent->getHRow();
            #endif
            fCreate = ! (pCursor ? pCursor->IsSameRow(_pCurrent->getHRow(), ahrow[k])
                                 : _pCurrent->getHRow() == ahrow[k]);

            if (!fCreate)
            {
                // remove _pCurrent from the cache list and reinsert it into the cahce
                // after plfrCurrent
                RemoveFromCache (_pCurrent);
                CacheFrame (_pCurrent, plfrCurrent);
            }
        }

        if (fCreate)
        {
            // plfrOrigin can be 0 here, which will indicate an append in
            // population direction
            hr = CreateLayout (ahrow[k], &plfr, plfrCurrent);
            if (FAILED(hr))
            {
                // release unused HROWS
                for (; k < ulFetched; k++)
                {
                    ReleaseRow(&ahrow[k]);
                }
                goto Error;
            }
        }
        else
        {
            // if we already had this one,
            // we got to release the addtional reference....
            ReleaseRow(&ahrow[k]);
        }
        plfrCurrent = plfr;
    }

    // set the next layout frame (OUT parameter).
    if (pplfr)
    {
        if (plfrOrigin)
        {
            *pplfr = (CBaseFrame*) plfrOrigin->_apNode[_fPopulateDirection];
        }
        else
        {
            *pplfr = (CBaseFrame*) _apNode[_fPopulateDirection];
        }
    }

Cleanup:
    RRETURN1 (hr, S_FALSE);

Error:
    if (pplfr)
    {
        *pplfr = NULL;
    }
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Member:     GetNextRows
//
//  Synopsis:   get next n rows from the cursor
//
//  Arguments:  pRows       pointer to an array of rows
//              pulRows     pointer to a number of rows to fetch from the
//                          cursor.
//              plfrStart   starting layout for currency setting, can be 0
//
//  Returns:    Returns S_OK if everything is fine, S_FALSE on less records
//              available then we have asked for, else error.
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::GetNextRows (OUT HROW *pRows,
                             IN  ULONG ulFetch,
                             OUT ULONG *pulFetched,
                             CBaseFrame *plfrStart)
{
    TraceTag ((tagRepeaterFrame, "CRepeaterFrame::GetNextRows"));

    Assert (ulFetch);           // parameters valididty check
    Assert (pulFetched);
    Assert (pRows);
    HROW                hrowCurrent = 0;
    HRESULT             hr = S_OK;
    En_FetchDirection   iFetch = getFetchDirection(_fPopulateDirection);
    ULONG               ulPreFetched = 0;
    *pulFetched = 0;

    if (!plfrStart)
    {
        plfrStart = _apNode[_fPopulateDirection];
    }
    if (plfrStart)
    {
        hrowCurrent = plfrStart->getHRow();
        if (hrowCurrent == 0)
        {
            // we trying to get the new row from the empty row

            // if the empty row was added because of the _fNewRecordShow
            // property was set ON, then
            if (IsNewRecordShow())
            {
                if (_fPopulateDirection == POPULATE_UP)
                {
                    // get the rows from the end of the data set.
                    hr = getOwner()->GetLastRow (pRows);
                    if (SUCCEEDED(hr))
                    {
                        ulPreFetched++;
                        ulFetch--;
                        hrowCurrent = *pRows;
                        pRows++;
                        goto Fetch;
                    }
                    else
                    {
                        goto Cleanup;   // Error
                    }
                }
            }
            hr = S_FALSE;
            goto Cleanup;
        }
    }

Fetch:
    if (ulFetch)
    {
        hr = getOwner()->GetNextRows(pRows, ulFetch, pulFetched, iFetch, hrowCurrent);
        if (SUCCEEDED(hr))
        {
            if (hr == S_FALSE &&
                ulFetch > (*pulFetched) &&
                _fPopulateDirection == POPULATE_DOWN &&
                IsNewRecordShow())
            {
                // if the new reocrd show is set ON, then adjust the rows array
                // for an EMPTY row.
                pRows += (*pulFetched);
                *pRows = NULL;
                ulPreFetched++;
                hr = S_OK;
            }
        }
    }
Cleanup:
    *pulFetched += ulPreFetched;
    RRETURN1 (hr, S_FALSE);

}





//+---------------------------------------------------------------------------
//
//  Member:     CreateLayout
//
//  Synopsis:   Create an instance of the CBaseFrame class and cache it.
//
//  Arguments:  hrow                HROW
//              pplfr               Pointer to (result) pointer to CBaseFrame
//              plfrBase            Pointer to where to insert the new frame in the
//                                  list, can be 0 if append operation
//
//  Assumption: _fPopulateDirection is set.
//
//  Returns:    returns the generated CBaseFrame object.
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::CreateLayout (HROW hrow,
                              OUT CBaseFrame **pplfr,
                              CBaseFrame *plfrBase)
{
    TraceTag ((tagRepeaterFrame, "CRepeaterFrame::CreateLayout"));

//    ErrorOpen(this); BUGBUG removed!

    // first thing to do: check if we can remove one guy from the cache..
    PutCacheFrameInRecycle();

    // Create a layout for a newly fetched row.
    // Note: the following call will set the _pParent of the layout frame to
    //       this frame and the way selection code works today we need _pParent
    //       to be set.

    HRESULT hr = getRepeatedTemplate()->CreateInstance (_pDoc, this, pplfr, hrow);

    if (SUCCEEDED(hr))
    {
        // Add the new layout to the array of rows.
        CBaseFrame *plfr = *pplfr;
        if (plfr)
        {
            Assert (plfr->_pParent);
            CacheFrame (plfr, plfrBase);
            if (!_pCurrent)
            {
                SetCurrent(plfr, SETCURRENT_NOFIREEVENT | SETCURRENT_NOINVALIDATE);
            }
        }
    }

//    ErrorCloseAndDisplay(hr, 0, 0);   BUGBUG removed!
    RRETURN (hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::CreateNewCurrentLayout
//
//  Synopsis:   Create the new layout for a given bookmark and make that layout
//              current.
//
//  Arguments:  dlBookmark      bookmark for the new row
//              lRecordNumber   row number
//              pplfr           pointer to the new CDetailFrame.
//
//  Returns:    S_OK, or any other data layer reported errors.
//              S_FALSE - call create to fit after this function.
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::CreateNewCurrentLayout (IN  CDataLayerBookmark  &dlBookmark,
                                        IN  long lRecordNumber,
                                        OUT CDetailFrame **pplfr)
{
    CDataLayerCursor *  pCursor = NULL;
    HROW                hrowNewCurrent;
    HRESULT             hr;
    CBaseFrame      *   plfr;
    int                 iDirection = getDirection ();
    long                lxy [2];
    BOOL                fCreateToFit = FALSE;

    hr = getOwner()->LazyGetDataLayerCursor(&pCursor);
    if (hr)
        goto Error;

    hr = pCursor->GetRowAt(dlBookmark, &hrowNewCurrent);
    // check the hrow returned because we don't care about BOOKMARK_SKIPPED or
    // other, we want an HROW here...
    if (!hrowNewCurrent)
        goto Error;

    plfr = CacheLookUpRow(hrowNewCurrent);
    if (!plfr)
    {
        getOwner()->CalcRowPosition(lRecordNumber,
                                &lxy[iDirection],
                                &lxy[1 - iDirection]);
        lxy[0] += _rcl[edgeLeft];
        lxy[1] += _rcl[edgeTop];
        hr = CreateLayoutRelativeToVisibleRange (lxy[iDirection],
                                                 hrowNewCurrent,
                                                 &plfr,
                                                 NULL,
                                                 &fCreateToFit);
        if (FAILED(hr))
            goto Error;
    }
    else
    {
        // The hRow is already in the cache so we can release it now.
        pCursor->ReleaseRows(1, &hrowNewCurrent);
    }

    SetCurrent(plfr, SETCURRENT_NOFIREEVENT);

    // Ensure the correct position of the new row.
    if (!plfr->IsInSiteList())
    {
        IGNORE_HR(FixupPosition (plfr, lRecordNumber));
    }

Cleanup:
    *pplfr = (CDetailFrame *)plfr;
    if (fCreateToFit && !hr)
    {
        hr = S_FALSE;
    }
    RRETURN1 (hr, S_FALSE);

Error:
    plfr = NULL;
    goto Cleanup;
}




//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::CreateLayoutRelativeToVisibleRange
//
//  Synopsis:   create layout and insert at the given position relatively to a
//              currently visible range.
//
//  Arguments:  lPosition       - position based on record number
//              hrow            - new row
//              pplfr           - pointer to base frame pointer
//              piInsertAt      - pointer to an insertAt index
//              pfCreateToFit   - pointer to a fCreateToFit flag
//
//  Returns:    Returns same as CreateLayout
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::CreateLayoutRelativeToVisibleRange (IN  long lPosition,
                                                    IN  HROW hrow,
                                                    OUT CBaseFrame **pplfr,
                                                    OUT int *piInsertAt,
                                                    OUT BOOL *pfCreateToFit)
{
    Edge        eTop;
    Edge        eBottom;
    CBaseFrame *plfr;

    Assert (pplfr);
    // Set the _fPopulateDirection, that the newly created frame
    // will be placed correctly in the cache.
    FlushNotVisibleCache();
    if (_arySites.Size())
    {
        eTop = (Edge) getDirection ();
        plfr = getLayoutFrame(getTopFrameIdx());
        if (lPosition < plfr->_rcl[eTop])
        {
            // Enforce the cache insertion at the BEGIN(POPULATE_UP).
            _fPopulateDirection = POPULATE_UP;
            if (piInsertAt)
                *piInsertAt = 0;
        }
        else
        {
            // Enforce the cache insertion at the END(POPULATE_DOWN)
            _fPopulateDirection = POPULATE_DOWN;
            eBottom = (Edge) (eTop + 2);
            if (piInsertAt)
                *piInsertAt = LASTFRAME;
            plfr = getLayoutFrame(getBottomFrameIdx());
            if (plfr->_rcl[eBottom] > lPosition)
            {
                CRectl rclDataFrame;
                getOwner ()->GetDetailSectionRectl (&rclDataFrame);
                long lPageSize = rclDataFrame.Dimension(eTop);

                if (lPosition >= _rcl[eBottom] - lPageSize)
                {
                    // this is the case when we are scrolled to the end
                    // of the repeater
                    _fPopulateDirection = POPULATE_UP;
                }
                else
                {
                    // this is the case when visible rows got deleted in the cursor,
                    // or new rows were inserted (dynamic cursor) in the visible range
                    FlushCache ();
                    if (pfCreateToFit)
                        *pfCreateToFit = TRUE;
                }
            }
        }
    }
    RRETURN(CreateLayout(hrow, pplfr, 0));
}





//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::FixupPosition
//
//  Synopsis:   Set the position of the detail frame based on it's record number
//
//  Arguments:  plfr            pointer to CDetailFrame.
//              lRecordNumber   record number (if == 0, means we have to get the
//                              row number from the cursor).
//
//  Retruns:    error if the detail frame has bad _hrow
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::FixupPosition (CBaseFrame *plfr, unsigned long ulRecordNumber)
{
    int     iDirection;
    long    lxy [2];
    HRESULT hr = S_OK;

    Assert (plfr);

    iDirection = getDirection ();
    if (ulRecordNumber == 0)
    {
        if (plfr->getHRow())
        {
            hr = getOwner ()->GetRecordNumber (plfr->getHRow (), &ulRecordNumber);
            if (hr)
            {
                goto Cleanup;
            }
        }
        else
        {
            // BUGBUG: nothing to do, stays where it is.
            goto Cleanup;
        }
    }
    getOwner()->CalcRowPosition(ulRecordNumber,
                                &lxy[iDirection],
                                &lxy[1 - iDirection]);
    plfr->MoveSiteBy(_rcl[edgeLeft] + lxy[0] - plfr->_rcl[edgeLeft],
                     _rcl[edgeTop] + lxy[1] - plfr->_rcl[edgeTop]);

Cleanup:
    RRETURN(hr);
}






//+---------------------------------------------------------------------------
//
//  Member:     PopulateWithOneFrame
//
//  Synopsis:   Populate real estate rectangle with one layout frame.
//              Add the passed frame as a subframe and check if there is real
//              estate left.
//
//  Arguments:  rclRealEstate               real estate rectangle to populate
//              rclView                     view rectangle.
//              plfr                        layout frame to populate with.
//              iInsertAt                   index for _arySites (insert the frame
//                                          At this position).
//              dwFlags                     flags passed to Popoluate.
//
//  Assumption: _fPopulateDirection is set.
//
//  Returns:    Returns S_OK if populated and no real estate left.
//              S_FALSE if populated and there is more real estate
//              left (rclRealEstate is not empty).
//              Eror result if Adding new site have failed.
//
//  Note:       I hope it's clear from the comment, that S_OK and S_FALSE are
//              both good return value, and
//              if (SUCCEEDED(hr))
//                  fEnoughRealEstate = return_code & 1;
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::PopulateWithOneFrame (INOUT CRectl& rclRealEstate,
                                      IN    CRectl& rclView,
                                      INOUT CBaseFrame * plfr,
                                      IN    int iInsertAt,
                                      IN    DWORD dwFlags)
{
    TraceTag((tagRepeaterFrame, "CRepeaterFrame::PopulateWithOneFrame"));


    Assert (plfr);

    HRESULT     hr = S_OK;
    CRectl      rclPropose;
    DWORD       dwSubFlags = SITEMOVE_NOFIREEVENT | SITEMOVE_NOINVALIDATE;
    long        dx, dy;
    Edge        e0, e1;         // (left/top) or (right/bottom) pair
    BOOL        fDontOptimize = FALSE;

    if (!plfr->IsInSiteList())
    {
        hr = AddToSites(plfr, ADDSITE_ADDTOLIST | ADDSITE_AFTERINIT | ADDSITE_NOINVALIDATE, iInsertAt);
        if (hr)
            goto Error;
        plfr->AddRef();
        ((CBaseFrame*)plfr)->SetInSiteList(TRUE);
    }
#if 0
    else
    {
        int i = Find (plfr);
        if (_fPopulateDirection == POPULATE_UP)
        {
            _arySites.BringToFront (i);
        }
        else
        {
            _arySites.SendToBack (i);
        }
    }
#endif

    if (_fPopulateDirection == POPULATE_UP)
    {
        e0 = edgeRight;
        e1 = edgeBottom;
    }
    else
    {
        e0 = edgeLeft;
        e1 = edgeTop;
    }
    // if templates are/were not dirty
    // we have to move the layout frame and all of it's children to the
    // right place, otherwise only the frame itself.
    if (plfr->IsAnythingDirty ())
    {
        fDontOptimize = TRUE;
        dwSubFlags |= SITEMOVE_NOMOVECHILDREN;
        // Get the size of the frame from the template
        plfr->_rcl = plfr->getTemplate()->_rcl;
    }

    dx = rclRealEstate[e0] - plfr->_rcl[e0];
    dy = rclRealEstate[e1] - plfr->_rcl[e1];
    if (dx || dy )
        plfr->MoveSiteBy (dx, dy);
    _rclProposeRow = plfr->_rcl;
    plfr->_fProposedSet = TRUE;

    // @TODO: Here is a place for an optimization, we should call
    // CreateToFit if it's needed only.

    if (fDontOptimize)
    {
        hr = plfr->CreateToFit (&rclView, dwFlags);
        if (!SUCCEEDED(hr))
           goto Error;
    }
    else if (getOwner()->TBag()->_fRefreshRows)
    {
        plfr->RefreshData();
    }

    // CreateToFit sets the rclPropose, and we need to reset the _rcl with it.
    // Note: If the layout template was dirty the _rclPropose was modeled
    // after the template _rcl, therefore we might need to move plfr
    // to the old position (rectangle rcl has the correct coordinates)

    dx = rclRealEstate[e0] - plfr->_rcl[e0];
    dy = rclRealEstate[e1] - plfr->_rcl[e1];
    if (dx || dy )
        plfr->MoveSiteBy (dx, dy);

    hr = (HRESULT) EnoughRealEstate (rclRealEstate, plfr->_rcl);

    plfr->SetPopulated(TRUE);       // the repeater is populated with this
                                    // frame
    plfr->_fProposedSet = FALSE;

    plfr->SelectSite(plfr, SS_SETSELECTIONSTATE);

    #if DBG==1 && !defined(PRODUCT_97)
    CheckFrameSizes(plfr);
    #endif

Error:
    RRETURN1 (hr, S_FALSE);
}





//+---------------------------------------------------------------------------
//
//  Member:     EnoughRealEstate
//
//  Synopsis:   Calculate new real estate rectangle, after adding a new
//              subframe to it. Check if there is enough space in the
//              Real Estate rectangle for the embedded pRect.
//              note: this code works for both horizontal and vertical
//              repeatition.
//
//
//  Arguments:  rclRealEstate           real estate rectangle to populate
//              pRect                   pointer to added rectangle
//
//  Returns:    Returns TRUE if there is enough real estate.
//
//----------------------------------------------------------------------------

BOOL
CRepeaterFrame::EnoughRealEstate (INOUT CRectl& rclRealEstate,
                                  IN const CRectl& rcl)
{
    TraceTag((tagRepeaterFrame, "CRepeaterFrame::EnoughRealEstate"));



    // Assert (!rclRealEstate.IsRectEmpty());
    long lShrinkBy = getPadding(_fDirection);


    Assert (rcl.Dimension(_fDirection) > 0);
    Assert (rclRealEstate.Dimension (_fDirection) >= 0);
    lShrinkBy += rcl.Dimension (_fDirection);

    if (lShrinkBy > rclRealEstate.Dimension (_fDirection))
    {
        rclRealEstate.SetRectEmpty ();
    }
    else
    {
        if (_fPopulateDirection == POPULATE_DOWN)
            rclRealEstate[(Edge) (_fDirection + edgeLeft)]  += lShrinkBy;
        else
            rclRealEstate[(Edge) (_fDirection + edgeRight)] -= lShrinkBy;
    }

    return !rclRealEstate.IsRectEmpty ();
}




//+---------------------------------------------------------------------------
//
//  Member:     SetCurrent
//
//  Synopsis:   Set current layout. Private.
//
//  Arguments:  uiRow           index into the subframes list.
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::SetCurrent (CBaseFrame *plfr, DWORD dwFlags)
{

    HRESULT hr = S_OK;
    BOOL    fFireEvents;
    CDataFrame * pdfrOwner = getOwner();
    CDataFrame::CTBag * pTBag = pdfrOwner->TBag();

    TraceTag((tagRepeaterFrame, "CRepeaterFrame::SetCurrent"));


    if (_pCurrent != plfr)
    {
        if (_pCurrent)
        {
            hr = _pCurrent->SetCurrent(FALSE);
            if (hr)
                goto Cleanup;

            if ( !_pCurrent->_fInvalidated && _pCurrent->IsInSiteList() &&
                 !(dwFlags & SETCURRENT_NOINVALIDATE) )
            {
                _pCurrent->Invalidate(NULL, 0);
            }
            _pCurrent->Release();
            _pCurrent = NULL;
        }

        if (plfr)
        {
            hr = plfr->SetCurrent(TRUE);
            if (hr)
                goto Cleanup;

            if ( !plfr->_fInvalidated && plfr->IsInSiteList() &&
                 !(dwFlags & SETCURRENT_NOINVALIDATE) )
            {
                plfr->Invalidate(NULL, 0);
            }
            _pCurrent = (CDetailFrame *)plfr;   // BUGBUG: change the interfaces
                                                // to use CDetailFrame
            _pCurrent->AddRef();
        }

        fFireEvents = TRUE;
    }
    else
    {
        fFireEvents = FALSE;
    }

    if ( (! (dwFlags & SETCURRENT_NOFIREEVENT))    &&
            (fFireEvents || (pTBag->_eListBoxStyle                             &&
                             pTBag->_eMultiSelect == fmMultiSelectSingle &&
                            _fSelectionChanged))
          )
    {
        pdfrOwner->OnPropertyChange(DISPID_DATADOC_ListIndex, 0);
        pdfrOwner->OnPropertyChange(DISPID_DATADOC_ListItemSelected, 0);
    }


Cleanup:
    _fSelectionChanged = FALSE;
    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     IsTopRow
//
//  Synopsis:   Checks if the row with the given index is a top visible row.
//
//  Arguments:  uIndex          index of the row.
//
//  Returns:    Returns TRUE if the row is a top visible row.
//
//----------------------------------------------------------------------------

BOOL
CRepeaterFrame::IsTopRow (unsigned int uIndex)
{
    TraceTag((tagRepeaterFrame, "CRepeaterFrame::IsTopRow"));

    return (uIndex < (getTopFrameIdx() + getItemsNonRepDir()));
}





//+---------------------------------------------------------------------------
//
//  Member:     IsBottomRow
//
//  Synopsis:   Check if passed "uIndex" is of the layout in the bottom row.
//
//  Arguments:  uIndex          index of the row
//
//  Returns:    Returns TRUE if the row is the bottom visible row.
//
//----------------------------------------------------------------------------

BOOL
CRepeaterFrame::IsBottomRow (unsigned int uIndex)
{
    TraceTag((tagRepeaterFrame, "CRepeaterFrame::IsBottomRow"));

    return (uIndex + getItemsNonRepDir() >= getFramesCounter());
}




//+---------------------------------------------------------------------------
//
//  Member:     IsTopRowFirstInSet
//
//  Synopsis:   Checks if the top visible row in the repeater is a first row
//              in the data set.
//
//  Returns:    Returns TRUE if the top visible row in the repeater is a first
//              row in the data set.
//
//----------------------------------------------------------------------------

BOOL
CRepeaterFrame::IsTopRowFirstInSet ()
{
    TraceTag((tagRepeaterFrame, "CRepeaterFrame::IsTopRowFirstInSet"));
    _fPopulateDirection = POPULATE_UP;  // BUGBUG: we probably don't need it here
    Assert (getFramesCounter());
    Edge        eTop = (Edge)getDirection();
    CBaseFrame *plfr = getLayoutFrame(getTopFrameIdx());
    return plfr->_rcl[eTop] == _rcl[eTop];
}





//+---------------------------------------------------------------------------
//
//  Member:     IsBottomRowLastInSet
//
//  Synopsis:   Checks if the bottom visible row in the repeater is the last
//              row in the data set.
//
//  Returns:    Returns TRUE if the bottom layout frame is the last in the set.
//
//----------------------------------------------------------------------------

BOOL
CRepeaterFrame::IsBottomRowLastInSet ()
{
    TraceTag((tagRepeaterFrame, "CRepeaterFrame::IsBottomRowLastInSet"));
    _fPopulateDirection = POPULATE_DOWN;
    Assert (getFramesCounter());
    Edge        eBottom = (Edge)((int)getDirection() + 2);
    CBaseFrame *plfr = getLayoutFrame(getBottomFrameIdx());
    return plfr->_rcl[eBottom] == _rcl[eBottom];
}





//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::ProcessSpaceKey
//
//  Synopsis:   Space will allow row [de]selection in the listbox.
//
//----------------------------------------------------------------------------

HRESULT __stdcall CRepeaterFrame::ProcessSpaceKey(long Direction)
{
    HRESULT hr = S_FALSE;
    DWORD   dwFlags;

    CDataFrame::CTBag * pTBag = getOwner()->TBag();

    if ( pTBag->_eListBoxStyle == fmListBoxStylesListBox )
    {

        if ( _pCurrent )
        {

            if (pTBag->_eMultiSelect == fmMultiSelectMulti)
            {
                dwFlags = (_pCurrent->_fSelected) ? SS_REMOVEFROMSELECTION :
                                                    SS_ADDTOSELECTION;
                hr = THR(_pCurrent->SelectSite(_pCurrent, dwFlags));
            }
            // else
            // {
                // in case of not multi select we treet "space" as any other keyboard character

                // dwFlags = SS_ADDTOSELECTION|SS_CLEARSELECTION;
                // hr = THR(_pCurrent->SelectSite(_pCurrent, dwFlags));
                // if ( pTBag->_eMultiSelect == fmMultiSelectSingle )
                // {
                //     //  This is gimmick. The current row is set to itself to force
                //     //  firing the "Listindex changed" notification to the listbox.
                //     SetCurrent(_pCurrent);
                // }
            //}
        }
    }

    RRETURN1(hr, S_FALSE);
}






//+---------------------------------------------------------------------------
//
//  Member:     KeyScroll
//
//  Synopsis:   Scroll the dataframe/listbox so that the current row is
//              correctly positioned on the screen
//
//  Arguments:  lParam: fmScrollAction
//
//  Returns:    Returns S_OK if everything is fine
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::KeyScroll (long lParam)
{
    HRESULT hr;
    CDataFrame  *   pdfrOwner = getOwner();
    int             iDirection = getDirection();
    int             iOther = 1 - iDirection;
    CRectl          rclPage;
    long            lRow;
    long            lStripe;
    long            lPosition;
    long            lStart, lEnd;
    CBaseFrame  *   pNewCurrent;
    CDetailFrame *  pCurrent;       //  Used for substitution if _pCurrent is NULL
    fmScrollAction  action[2];
    int             iMove;
    CSite       **  ppStart, ** ppEnd;
    unsigned        uFrames;
    BOOL            fListBoxStyle = IsListBoxStyle ();
    CDataFrame::CTBag * pTBag = pdfrOwner->TBag();
    BOOL            fLineOperation = FALSE; // lineUp, lineDown
    BOOL            fShiftSelect = FALSE;

    TraceTag((tagRepeaterFrame,"CRepeaterFrame::Page"));


    if (!_pCurrent)
    {
        if ( getFramesCounter() > 0 )
        {
            pCurrent = getLayoutFrame(0);
        }
        else
        {
            //  Bail out if the repeater is empty
            return S_FALSE;
        }

        if ( lParam != fmScrollActionEnd )
        {
            lParam = fmScrollActionBegin;
        }
    }
    else
    {
        pCurrent = _pCurrent;
    }

    _pDoc->_fDisableOffScreenPaint = TRUE;

    if ( GetKeyState(VK_SHIFT) & 0x8000 &&
         (pdfrOwner->TBag()->_eMultiSelect == fmMultiSelectExtended) )
    {
        fShiftSelect = TRUE;
    }

    if ( pTBag->_eMultiSelect != fmMultiSelectMulti )
    {
        RootFrame(this)->ClearSelection(TRUE);
    }
    pdfrOwner->GetDetailSectionRectl(&rclPage);
    lStart = rclPage[(Edge)iDirection];
    lEnd = rclPage[(Edge)(iDirection + 2)];
    lRow = getTemplate()->_rcl.Dimension(iDirection);
    lStripe = lRow + getPadding(iDirection);

    // after the following switch statement the lPosition has the relative
    // to the top of the repeater delta value.
    switch (lParam)
    {
    case fmScrollActionPageUp:
    case fmScrollActionPageDown:
        lPosition = _uRepeatInPage * lStripe;
        if (fListBoxStyle && _uRepeatInPage > 1)
        {
            lPosition -= lStripe;
        }
        break;
    case fmScrollActionLineUp:
    case fmScrollActionLineDown:
        lPosition = lStripe;
        fLineOperation = TRUE;
        break;
    case fmScrollActionBegin:
    case fmScrollActionEnd:
        lPosition = _rcl.Dimension(iDirection);
        break;
    default:
        Assert(FALSE && "Invalid scroll action for KeyScroll");
    }

    // after the following switch statement the lPosition has the absolute value
    switch (lParam)
    {
    case fmScrollActionBegin:
    case fmScrollActionPageUp:
    case fmScrollActionLineUp:
        lPosition = pCurrent->_rcl[(Edge)iDirection] - lPosition;
        iMove = -1;
        break;
    case fmScrollActionEnd:
    case fmScrollActionPageDown:
    case fmScrollActionLineDown:
        lPosition = pCurrent->_rcl[(Edge)iDirection] + lPosition;
        iMove = 1;
        break;
    default:
        Assert(FALSE && "Invalid scroll action for KeyScroll");
    }

    // check if lStart is in the visible range

    action[iDirection] = (fmScrollAction)lParam;
    if (lPosition < lStart || (lPosition + lRow) > lEnd)
    {
        long lDelta[2];

        // we have to scroll
        action[iOther] = fmScrollActionNoChange;
        lDelta[iOther] = 0;
        if (fListBoxStyle)
        {
            // in the listbox we use the delta
            lDelta[iDirection] = lPosition < lStart ? lPosition - lStart : lPosition + lStripe - lEnd;
            if (lParam == fmScrollActionPageUp || lParam == fmScrollActionPageDown ||
               (!pCurrent->IsInSiteList() && (lParam == fmScrollActionLineUp || lParam == fmScrollActionLineDown)))
            {
                action[iDirection] = fmScrollActionAbsoluteChange;
                pdfrOwner->_fPixelScrollingDisable = TRUE;  // to avoid scrolling one row...
            }
        }
        else
        {
            // in grid we use the action
            lDelta[iDirection] = 0;
        }

        lPosition -=  pCurrent->_rcl[(Edge)iDirection];

        // subtract current rectangle during ScrollRegion (ugly hack for performance)
        pdfrOwner->_fSubtractCurrent = fLineOperation && pCurrent->IsInSiteList();
        hr = ScrollBy(lDelta[0], lDelta[1], action[0], action[1]);
        pdfrOwner->_fSubtractCurrent = FALSE;
        if (!SUCCEEDED(hr))
            goto Cleanup;

        lPosition += pCurrent->_rcl[(Edge)iDirection];
    }


    uFrames = _arySites.Size();
    if (uFrames)
    {
        // find frame at lPosition
        if (iMove < 0)
        {
            // search up
            ppEnd = _arySites;
            ppStart = ppEnd + uFrames - 1;
        }
        else
        {
            ppStart = _arySites;
            ppEnd = ppStart + uFrames - 1;
        }
        while (ppStart != ppEnd)
        {
            CRectl rcl = (*ppStart)->_rcl;
            if (rcl[(Edge)iDirection] <= lPosition &&
                rcl[(Edge)(iDirection + 2)] > lPosition &&
                rcl[(Edge)iOther] == _pCurrent->_rcl[(Edge)iOther])
            {
                break;
            }
            ppStart += iMove;
        }
        pNewCurrent = (CBaseFrame *)(*ppStart);
    }
    else
    {
        pNewCurrent = NULL;
    }

    _fCurrentSelected = !fShiftSelect && (pTBag->_eMultiSelect != fmMultiSelectMulti);
    if (!fShiftSelect)
    {
        pdfrOwner->SetAnchorSite(pNewCurrent);
    }

    if ( pTBag->_eMultiSelect != fmMultiSelectMulti )
    {
        if ( pNewCurrent )
        {
            hr = THR(pNewCurrent->SelectSite(pNewCurrent,
                fShiftSelect ? (SS_ADDTOSELECTION|SS_MERGESELECTION) : SS_ADDTOSELECTION));
            if (hr)
                goto Cleanup;
        }
    }

    hr = SetCurrent(pNewCurrent);
    if (hr)
        goto Cleanup;

    pdfrOwner->SetSelectionStates();
    _fCurrentSelected = FALSE;

    // BUGBUG for now...
    if (fListBoxStyle)
    {
        _pDoc->UpdateForm();
    }

Cleanup:
    if (FAILED(hr))
    {
        int      idsAction;
        idsAction = ( (IDS_ACT_DDOCSCROLLGENERAL+lParam) > IDS_ACT_DDOCSCROLLEND) ?
                        IDS_ACT_DDOCSCROLLGENERAL :
                        (IDS_ACT_DDOCSCROLLGENERAL+lParam);
        ConstructErrorInfo(hr, idsAction);
    }

    RRETURN1(hr, S_FALSE);
}




//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::NextControl
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
//              @todo: implement it!
//
//  Comments:   It's the caller's problem to do anything with the site
//              returned as 'next'.
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::NextControl(NAVIGATE_DIRECTION Direction,
                            CSite * psCurrent,
                            CSite ** ppsNext,
                            CSite *pSiteCaller,
                            BOOL fStrictInDirection,
                            int iFirstFrame,
                            int iLastFrame)
{
    HRESULT hr = S_FALSE;

/*
    CSite * psNextControl;
    CSite * psCurrentChild;
    CSite * ps;
    CRectl rclCurrentLayout;
    int iCurrentChild;
    ESnaking    eSnakingDirection;
    CDataFrame * pdfr;
    BOOL fLayoutAddRefd = FALSE;

    Assert(psCurrent);      //  @todo: remove it later, when the repeater can be selected too.
    Assert(ppsNext);

    //  This tries to find a frame on-screen, in the repeater. Top and bottom frame is
    //  specified so that we don't select into the header cells
    //  Review: what about design mode?
    pdfr = getOwner();
    if (pdfr)
    {
        fStrictInDirection = pdfr->IsPrimaryDirection(Direction);
    }
    else
    {
        fStrictInDirection = FALSE;
    }

    hr = THR(super::NextControl(Direction, psCurrent, &psNextControl, this, fStrictInDirection,
                                      getTopFrameIdx(), getBottomFrameIdx()));

    if ( hr )
    {
        //  Try to scroll

        //  First locate the layoutframe which propagated the action to us.
        ps = psCurrent;
        psCurrentChild = NULL;

        while ( ! psCurrentChild && ps )
        {
            if ( ps->_pParent == this )
            {
                psCurrentChild = ps;
            }
            ps = ps->_pParent;
        }

        iCurrentChild = Find(psCurrentChild);

        if (iCurrentChild >= 0)
        {
            fLayoutAddRefd = TRUE;
            psCurrentChild->AddRef();       //  to prevent releasing it if it
                                            //  scrolls off the screen or cache
        }

        getOwner()->GetSnakingType((unsigned *)&eSnakingDirection);

        switch ( Direction )
        {
            case NAVIGATE_UP:
                if ( getItemsDown () == UNLIMITED )
                {
                    _fPopulateDirection = POPULATE_UP;
                     hr = getOwner()->OnScroll(getDirection(), KB_LINEUP, 0);
                }
                else
                {
                    if ( iCurrentChild > (int)getTopFrameIdx() ) //  if == 0 then this is the first row and we should scroll
                    {
                        //  need to get to the previous layout column
                        iCurrentChild--;
                        //psNextControl = (*Sites())[iCurrentChild];
                        //  and fix up the position of the reference rectangle!
                    }
                }
                break;
            case NAVIGATE_DOWN:
                if ( getItemsDown() == UNLIMITED )
                {
                    hr = getOwner()->OnScroll(getDirection(), KB_LINEDOWN, 0);
                }
                else
                {
                    if ( iCurrentChild < (int)getBottomFrameIdx() )
                    {
                        //  need to get to the previous layout column
                        iCurrentChild++;
                        //psNextControl = (*Sites())[iCurrentChild];
                        //  and fix up the position of the reference rectangle!
                    }
                }
                break;
            case NAVIGATE_LEFT:
                if ( getItemsAcross() == UNLIMITED )
                {
                    hr = getOwner()->OnScroll(getDirection(), KB_LINEUP, 0);
                }
                else
                {
                    if ( iCurrentChild > (int)getTopFrameIdx()) //  if == 0 then this is the first row and we should scroll
                    {
                        //  need to get to the previous layout column
                        iCurrentChild--;
                        //psNextControl = (*Sites())[iCurrentChild];
                        //  and fix up the position of the reference rectangle!
                    }
                }
                break;
            case NAVIGATE_RIGHT:
                if ( getItemsAcross() == UNLIMITED )
                {
                    hr = getOwner()->OnScroll(getDirection(), KB_LINEDOWN, 0);
                }
                else
                {
                    if ( iCurrentChild < (int)getBottomFrameIdx()) //  if == 0 then this is the first row and we should scroll
                    {
                        //  need to get to the previous layout column
                        iCurrentChild++;
                        //psNextControl = (*Sites())[iCurrentChild];
                        //  and fix up the position of the reference rectangle!
                    }
                }
                break;

            default:
                break;
        }

        //  then try again

        if (!hr)
        {
            hr = THR(super::NextControl(Direction, psCurrent, &psNextControl, this,
                                              fStrictInDirection,
                                              getTopFrameIdx(), getBottomFrameIdx()));
        }
    }


    if ( !hr )
    {
        Assert(psNextControl->TestClassFlag(SITEDESC_DETAILFRAME));
        SetCurrent ((CDetailFrame *)psNextControl); // BUGBUG: laszlog! please review it.
        if (TestFlag(SITE_FLAG_SELECTCHILD))
        {
            hr = THR(psNextControl->NextControl(Direction,psCurrent,ppsNext, this));
        }
        else
        {
            hr = S_OK;
            *ppsNext = psNextControl;
        }
    }
    else if ( ! pdfr->TBag()->_fArrowStaysInFrame && _pParent != pSiteCaller)
    {
        Assert(_pParent);

        hr = THR(_pParent->NextControl(Direction, psCurrent, ppsNext, this));
    }


//Cleanup:
    if ( fLayoutAddRefd )
        psCurrentChild->Release();
*/

    RRETURN1(hr, S_FALSE);
}










//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::HitTestDetailFrames, public
//
//  Synopsis:   Figures out which detail frame was hit by the mouse
//
//  Arguments:  msg:            The mouse message
//              pt:             The mouse coordinates
//              gfrKeyState:    The key state
//
//  Returns:    S_FALSE if not handled,
//              S_OK    if processed
//
//----------------------------------------------------------------------------

CDetailFrame *
CRepeaterFrame::HitTestDetailFrames(const CPointl& ptl)
{
    int i = _arySites.Size();
    CSite ** ppSite = _arySites;

    while(i--)
    {

// BUGBUG (garybu) no more hitest pointl
//        if ((*ppSite)->HitTestPointl(ptl, NULL, NULL, NULL, 0) != HTC_NO)
//        {
//            return (CDetailFrame *)(*ppSite);
//        }
        ppSite++;
    }

    return NULL;
}




//+---------------------------------------------------------------------------
//
//  Function:   ForcePointIntoRect, helper
//
//  Synopsis:   Maps a point to be inside a specified rectangle
//
//  Arguments:  pPtl:           The point to be modified
//              prcl:           The bounding rectangle
//              pDiff:          The precomputed distances between the
//                              point and the rect.
//
//  Returns:    S_FALSE if not handled,
//              S_OK    if processed
//
//----------------------------------------------------------------------------

static void ForcePointIntoRect(CPointl * pPtl, CRectl * prcl, CSizel * pDiff)
{
    Edge e;

    for ( e = edgeLeft; e <= edgeTop; (int&)e += 1 )
    {
        if ( pDiff[e][e] * pDiff[1-e][e] > 0 )
        {
            if ( (*pPtl)[e] <= (*prcl)[e] )
            {
                 (*pPtl)[e] = (*prcl)[e] + 1;
            }
            else
            {
                 (*pPtl)[e] = (*prcl)[OPPOSITE(e)] - 1;
            }
        }
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::KillScrollTimer, public
//
//  Synopsis:   Kills the repeater's drag-scroll timer
//
//----------------------------------------------------------------------------

void CRepeaterFrame::KillScrollTimer(void)
{
    FormsKillTimer(this, F3REPEATER_REPEAT_TIMER);
    _fTimerAlive = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::DragSelect, public helper
//
//  Synopsis:   Implements the repeater's drag selection behaviour
//
//  Arguments:  pTBag:          Cached DataFrame TBag for properties
//              pdet:           The detail fram eunder the mouse
//              pdfrOwner:      The owner DataFrame
//
//----------------------------------------------------------------------------

void CRepeaterFrame::DragSelect(CDataFrame::CTBag * pTBag, CDetailFrame * pdet, CDataFrame * pdfrOwner, DWORD dwDragSelectFlags)
{
    BOOL fNeedBuildSelection;

    if ( _pCurrent == pdet &&
         ((pTBag->_eListBoxStyle != fmListBoxStylesComboBox) ||
          (_pCurrent->_fSelected == (unsigned int)ENSURE_BOOL(dwDragSelectFlags & DRAGSELECT_MOUSE_INSIDE))) )
    {
        return;
    }


    if ( dwDragSelectFlags & DRAGSELECT_SETCURRENTONLY )
    {
        if ( pdfrOwner->_parySelectedQualifiers )
        {
            Assert( (pTBag->_eListBoxStyle == fmListBoxStylesComboBox) &&
                    (pTBag->_eMultiSelect == fmMultiSelectSingle) );

            RootFrame(pdfrOwner)->ClearSelection(TRUE);
            fNeedBuildSelection = TRUE;
        }
        else
        {
            fNeedBuildSelection = FALSE;
        }
    }
    else
    {
        if ( !(dwDragSelectFlags & DRAGSELECT_CTRL_DRAG) &&
             (pTBag->_eMultiSelect == fmMultiSelectExtended) )
        {
            //  Right now reuse the shift_click extended selection code
            IGNORE_HR(pdet->SelectSite(pdet, SS_ADDTOSELECTION|SS_MERGESELECTION|SS_CLEARSELECTION));
        }
        else
        {
            //  Invert the outgoing row's selection state
            //  BUGBUG: What happens in the grid case?!

            //  Note: We suppose that the Current frame is always
            //  visible (or at least hasn't been destroyed yet.
            //  This needs verification with large rows, of which
            //  only one can fit in the repeater.
            Assert(_pCurrent);

            //  If this is a control-drag, don't invert the outgoing row's
            //  selection state
            if ( (pTBag->_eMultiSelect == fmMultiSelectSingle) ||
                 ! (dwDragSelectFlags & DRAGSELECT_CTRL_DRAG) )
            {
                IGNORE_HR(_pCurrent->SelectSite(_pCurrent,
                    (!_pCurrent->_fSelected) && (pTBag->_eMultiSelect != fmMultiSelectSingle) ?
                    SS_ADDTOSELECTION : SS_REMOVEFROMSELECTION));
            }

            //  invert the incoming row's selection state
            IGNORE_HR(pdet->SelectSite(pdet, pdet->_fSelected ?
                SS_REMOVEFROMSELECTION : SS_ADDTOSELECTION));
        }

        fNeedBuildSelection = TRUE;
    }

    if ( fNeedBuildSelection )
    {
        pdfrOwner->SetSelectionStates();
    }

    SetCurrent(pdet);
    _pDoc->UpdateForm();
}





//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::DeselectListboxLines
//
//  Synopsis:   Robustness. Whenever the dataframe's selection tree is
//              discarded, this function is called to reset all the
//              back pointers to the selection tree.
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::DeselectListboxLines(void)
{
    int c;
    CSite ** ppSite;

    for ( ppSite = _arySites, c = _arySites.Size();
          --c >= 0;
          ppSite++ )
    {
        Assert((*ppSite)->TestClassFlag(SITEDESC_DETAILFRAME));
        ((CDetailFrame*)(*ppSite))->ResetSelectionPointer();
    }
}





//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::HandleMessage
//
//  Synopsis:   Handle keystrokes.
//
//  Returns:    Returns S_OK if keystroke processed, S_FALSE if not.
//
//  Comments:   Handle the keystrokes routed to the layout frame.
//              Have to bubble up in the layout instance hierarchy.
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CRepeaterFrame::HandleMessage(CMessage *pMessage, CSite *pChild)
{
    HRESULT hr;
    CDataFrame * pdfrOwner = getOwner();
    CDataFrame::CTBag * pTBag;
    CDetailFrame * pdet;
    POINT pt;
    CPointl ptl;
    CPointl ptlOriginal;
    BOOL fDirection;
    CRectl rcl;
    BOOL fMouseOutside;
    DWORD dwDragSelectFlags = 0;
    BOOL  fSuperCalled=FALSE;
    BOOL  fShiftSelect = FALSE;

    TraceTag((tagRepeaterFrame,"CRepeaterFrame::HandleMessage"));

    if ( ! pdfrOwner->_fEnabled )
        return S_FALSE;

    if (pMessage->message >= WM_KEYFIRST && pMessage->message <= WM_KEYLAST)
    {
        if (pChild)
        {
            hr = THR(_pParent->HandleMessage(pMessage, this));
        }
    }
    else if (pMessage->message >= WM_MOUSEFIRST && pMessage->message <= WM_MOUSELAST)
    {
        pt.x = MAKEPOINTS(pMessage->lParam).x;
        pt.y = MAKEPOINTS(pMessage->lParam).y;

        //  Transform the mouse coordinates into himetric
        _pDoc->HimetricFromDevice(&ptl, pt);
        ptlOriginal = ptl;  //  save it for event firing
        pTBag = pdfrOwner->TBag();
        BOOL    fListBoxStyle = pTBag->_eListBoxStyle;

        if ( !fListBoxStyle && (pMessage->htc > HTC_YES || _pDoc->_fDesignMode) )
        {
            goto CallSuper;
        }

        hr = S_OK;

        //  get the viewport rectangle
        pdfrOwner->GetDetailSectionRectl(&rcl);
        fMouseOutside = ! rcl.PtInRect(ptl);
        if ( fMouseOutside && pTBag->_eListBoxStyle == fmListBoxStylesComboBox )
        {
            dwDragSelectFlags |= DRAGSELECT_SETCURRENTONLY;
        }
        if ( !fMouseOutside )
        {
            dwDragSelectFlags |= DRAGSELECT_MOUSE_INSIDE;
        }
        if ( pMessage->wParam & MK_CONTROL )
        {
            dwDragSelectFlags |= DRAGSELECT_CTRL_DRAG;
        }

        if ( ((pMessage->message == WM_LBUTTONUP) || (pMessage->wParam & MK_LBUTTON)) && fMouseOutside )
        {
            CSizel diff[2];

            fDirection = getDirection();

            //  to make point mapping easier
            rcl.InflateRect(-1, -1);

            diff[0] = ptl - rcl.TopLeft();
            diff[1] = ptl - rcl.BottomRight();

            if ( (pMessage->message != WM_LBUTTONUP) && (diff[0][(Edge)fDirection] * diff[1][(Edge)fDirection] >= 0) )
            {
                //  we need to drag-scroll
                if ( ! _fTimerAlive )
                {
                    //  kick off the timer that causes drag-scrolling
                    hr = THR(FormsSetTimer(
                            this,
                            (PFN_VOID_ONTICK)CRepeaterFrame::OnTick,
                            F3REPEATER_REPEAT_TIMER,
                            cRepeatRate));

                    if (hr)
                        goto Cleanup;

                    _fTimerAlive = TRUE;
                }
            }
            else if ( _fTimerAlive )
            {
                KillScrollTimer();
            }

            //  just map the point inside the detail section and select
            //  the closest detail frame

            ForcePointIntoRect(&ptl, &rcl, diff);

        }
        else if ( _fTimerAlive )
        {
            KillScrollTimer();
        }

        pdet = HitTestDetailFrames(ptl);
        if ( ! pdet )
            goto Cleanup;

        switch (pMessage->message)
        {
            case WM_LBUTTONDOWN:
                #if PRODUCT_97
                if ( pTBag->_eListStyle != fmListStyleMark )
                #else

                #endif
                {

                    //  All the listbox styles support selection
                    //  by clicking anywhere. The toggle-button functionality will
                    //  arrive later.

                    //  Capture the mouse
                    if ( ! _fMouseCaptured )
                    {
                        if ( pTBag->_eListBoxStyle == fmListBoxStylesListBox )
                        {
                            TakeCapture(TRUE);
                        }
                        else
                        {
                            //  Set the combo tracking flag
                            TakeCapture(TRUE);
                            _fComboMouseTracking = TRUE;
                        }
                        _fMouseCaptured = TRUE;
                    }

                    if ( (pMessage->wParam & MK_SHIFT) &&
                         (pTBag->_eMultiSelect == fmMultiSelectExtended) )
                    {
                        fShiftSelect = TRUE;
                    }

                    if ( (pTBag->_eMultiSelect != fmMultiSelectSingle) ||
                         !pdet->_fSelected )
                    {
                        DWORD   dwFlags = fShiftSelect ? SS_MERGESELECTION : 0;

                        if ( (!pdet->_fSelected) ||
                             !( (pTBag->_eMultiSelect == fmMultiSelectMulti) ||
                                ((pTBag->_eMultiSelect == fmMultiSelectExtended) &&
                                  (pMessage->wParam & MK_CONTROL)) ) )
                        {
                            if (pTBag->_eMultiSelect == fmMultiSelectSingle ||
                                 ( !(pMessage->wParam & MK_CONTROL) &&
                                    (pTBag->_eMultiSelect == fmMultiSelectExtended)))
                            {
                                dwFlags |= SS_ADDTOSELECTION|SS_CLEARSELECTION;
                            }
                            else
                            {
                                dwFlags |= SS_ADDTOSELECTION;
                            }
                        }
                        else
                        {
                            dwFlags |= SS_REMOVEFROMSELECTION;
                        }

                        IGNORE_HR(pdet->SelectSite(pdet, dwFlags));

                        SetCurrent(pdet);
                        pdfrOwner->SetSelectionStates();
                    }
                }

                break;

            case WM_LBUTTONUP:

                KillScrollTimer();

                if ( pTBag->_eListBoxStyle == fmListBoxStylesListBox )
                {
                    TakeCapture(FALSE);
                }
                else
                {
                    TakeCapture(FALSE);
                    _fComboMouseTracking = FALSE;
                }
                _fMouseCaptured = FALSE;

                //  if the mouse was released outside the frame during
                //  a dragscroll, complete the selection cycle.
                DragSelect(pTBag, pdet, pdfrOwner, dwDragSelectFlags);


                //  BUGBUG: Need to consolidate the positive and negative
                //  selection here as soon as we have that extra range.

                break;

            case WM_MOUSEMOVE:
                if ( ! _fMouseCaptured && (pMessage->wParam & MK_LBUTTON) )
                {
                    //  Capture the mouse
                    if ( pTBag->_eListBoxStyle == fmListBoxStylesListBox )
                    {
                        TakeCapture(TRUE);
                    }
                    else
                    {
                        //  Set the combo tracking flag
                        TakeCapture(TRUE);
                        _fComboMouseTracking = TRUE;
                    }
                    _fMouseCaptured = TRUE;
                }

                if ( (pMessage->wParam & MK_LBUTTON) ||
                     ((pTBag->_eListBoxStyle == fmListBoxStylesComboBox) &&
                      (_fComboMouseTracking || ! fMouseOutside)) )
                {
                    DragSelect(pTBag, pdet, pdfrOwner, dwDragSelectFlags);
                }
                break;

            default:
                goto CallSuper;
        }
    }
    else
    {
        goto CallSuper;
    }

Cleanup:
    if ( !hr && !fSuperCalled && (pMessage->message >= WM_MOUSEFIRST && pMessage->message <= WM_MOUSELAST) )
    {
        getOwner()->FireMouseEvent(pMessage->message, ptlOriginal.x, ptlOriginal.y, pMessage->wParam);
    }
    RRETURN1(hr, S_FALSE);

CallSuper:
    hr = THR(super::HandleMessage(pMessage, pChild));
    fSuperCalled = TRUE;
    goto Cleanup;

}





//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::OnTick
//
//  Synopsis:   Timer click handler. Does the drag-scrolling if the mouse
//              is captured and outside the detail rectangle.
//
//  Returns:    HRESULT
//
//  Comments:   Strange BUGCALL convention because of VC2 thunk naming bug
//
//----------------------------------------------------------------------------

HRESULT BUGCALL CRepeaterFrame::OnTick(UINT id)
{
    HRESULT hr = S_OK;
    POINT pt;
    CPointl ptl;
    CRectl rcl;
    BOOL fDirection;
    CDetailFrame * pdet;
    CDataFrame * pdfrOwner = getOwner();

    //  Scroll the repeater

    ::GetCursorPos(&pt);
    _pDoc->DocumentFromScreen(&ptl,pt);

    GetClientRectl(&rcl);

    CSizel diff[2];

    fDirection = getDirection();

    //  to make point mapping easier
    rcl.InflateRect(-1, -1);

    diff[0] = ptl - rcl.TopLeft();
    diff[1] = ptl - rcl.BottomRight();

    if ( diff[0][(Edge)fDirection] * diff[1][(Edge)fDirection] >= 0 )
    {
        //  we need to dragscroll
        long dx, dy, lHeight;
        int  iTimer;
        BOOL fUseDiff0;

        fUseDiff0 = ! ( labs(diff[0][(Edge)(1-fDirection)]) < labs(diff[1][(Edge)(1-fDirection)]) );
        dx = (1-fDirection) * diff[fUseDiff0][(Edge)(1-fDirection)];

        fUseDiff0 = ! ( labs(diff[0][(Edge)fDirection]) < labs(diff[1][(Edge)fDirection]) );
        dy = fDirection * diff[fUseDiff0][(Edge)fDirection];

        lHeight = getTemplate()->_rcl.Height();
        Assert(lHeight);

        if (dy > 0 && dy < lHeight)
        {
            dy = lHeight;
        }

        // now calculate how fast to set the timer....
        iTimer = abs ((int)(dy / lHeight)) + 1;
        iTimer = max (iTimer, cMaxRepeatDivider);

        // to avoid flickering caused by ScrollRegion which scrolls
        // the current row if it's not invalidated and then current row
        // will be invalidated by SetCurrent, we need to invalidate
        // the current row.
        Assert(_pCurrent);
        // if current stays visible during scroll, subtract it from the scrollable region
        pdfrOwner->_fSubtractCurrent = _pCurrent->IsInSiteList();
        hr = ScrollBy(dx, dy,
                      (fmScrollAction)((1-fDirection) * fmScrollActionAbsoluteChange),
                      (fmScrollAction)(fDirection * fmScrollActionAbsoluteChange));
        pdfrOwner->_fSubtractCurrent = FALSE;

        if ( hr )
        {
            if (hr == S_FALSE)
            {
                hr = S_OK;
            }
            goto Cleanup;
        }


        //  just map the point inside the detail section and select
        //  the closest detail frame

        ForcePointIntoRect(&ptl, &rcl, diff);

        pdet = HitTestDetailFrames(ptl);
        if ( pdet )
        {
            DWORD dwDragSelectFlags = 0;

            if ( _fComboMouseTracking )
            {
                dwDragSelectFlags |= DRAGSELECT_SETCURRENTONLY;
            }
            if ( GetKeyState(VK_CONTROL) & 0x8000 )
            {
                dwDragSelectFlags |= DRAGSELECT_CTRL_DRAG;
            }

            DragSelect(pdfrOwner->TBag(), pdet, pdfrOwner, dwDragSelectFlags);

        }

        //  Set the timer to be faster
        hr = THR(FormsSetTimer(this,
                               (PFN_VOID_ONTICK)CRepeaterFrame::OnTick,
                               F3REPEATER_REPEAT_TIMER,
                               cRepeatRate/iTimer));
    }
    else
    {
        KillScrollTimer();
    }



Cleanup:
    RRETURN(hr);
}




//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::GetKeyMap
//
//  Synopsis:   Returns the keymap of the class
//
//  Returns:    address of the keymap
//
//  Comments:   virtual
//
//----------------------------------------------------------------------------

KEY_MAP * CRepeaterFrame::GetKeyMap(void)
{
    return s_aKeyActions;
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

int
CRepeaterFrame::GetKeyMapSize(void)
{
    if ( getOwner()->_fEnabled )
    {
        if (IsListBoxStyle())
        {
            // in case of a list box VK_UP, VK_DOWN are handled by KeyScroll
            return s_cKeyActions;
        }
        else
        {
            // in case of a grid VK_UP, VK_DOWN are handled by NextControl
            return s_cKeyActions - 2;
        }
    }
    else
    {
        return 0;
    }
}





//+---------------------------------------------------------------------------
//
//  Member:     AreInColumn
//
//  Synopsis:   Check if two frames are in the same column.
//
//  Arguments:  pFrame1             pointer to a first layout frame
//              pFrame2             pointer to a second layout frame
//
//  Returns:    Return TRUE if in the same column
//
//----------------------------------------------------------------------------

BOOL
CRepeaterFrame::AreInColumn (CBaseFrame *pFrame1, CBaseFrame *pFrame2)
{
    TraceTag((tagRepeaterFrame, "CRepeaterFrame::AreInColumn"));

    return pFrame1->_rcl.left == pFrame2->_rcl.left;
}




//+---------------------------------------------------------------------------
//
//  Member:   GetNextSiteInTabOrder, virtual
//
//  Synopsis:   Return the next site in the tab order.
//
//  Arguments:  pSite           Starting site in the pParent.
//              fBack           True if going backward.
//
//  Returns:    the next site in the tab order or NULL if at the end.
//
//----------------------------------------------------------------------------

CSite *
CRepeaterFrame::GetNextSiteInTabOrder(CSite * pSite, CMessage * pMessage)
{
    HRESULT hr = E_FAIL;
    int i = -1;
    BOOL fBack = pMessage->dwKeyState & FSHIFT;

    TraceTag((tagRepeaterFrame,"CRepeaterFrame::GetNextSiteInTabOrder"));

    if (!_arySites.Size())
        goto Error;

    //  BUGBUG: This is a hack to prevent the run-mode crash with
    //  repeated empty rows

    if ( (! _pDoc->_fDesignMode) &&
         (0 == _pTemplate->_arySites.Size()) )
    {
        goto Cleanup;       //  Make the caller bounce back
    }

    if (pSite)
    {
        i = Find(pSite);
    }

    if (i < 0)
        goto Error;

    i += fBack ? -1 : 1;

    if (i >= _arySites.Size())
    {
        //  we need to scroll the repeater
        hr = getOwner()->OnScroll(getDirection(),
                fBack ? KB_LINEUP : KB_LINEDOWN, 0);

        if (!hr)
        {
            goto Error;
        }

        i = fBack ? getTopFrameIdx() : getBottomFrameIdx();
        Assert((i >= 0) && (i < _arySites.Size()));

    }

    // Find that control.
    pSite = _arySites[i];

Cleanup:
    return pSite;

Error:
    pSite = NULL;
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::MoveToRecycle(), protected
//
//  Synopsis:   moves the passed layoutframe into the recycle list, thereby
//              removing it from all ties to real life
//
//  SideEffect:
//              1. Assumption: any passed in frame is already removed from
//                  the parents sitelist, because it should be invisble
//              2. The method will manipulate the main roots of the cachetree
//                  if need be
//
//  Arguments:  CBaseFrame * plfr -> the layout to recycle
//
//  Returns:    S_OK if everything is fine
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::MoveToRecycle(CBaseFrame *plfr)
{
    // first get this one out of it's current list relationships
    Assert(plfr);

    TraceTag((tagRepeaterFrame,"CRepeaterFrame::MoveToRecycle"));
    RemoveFromCache(plfr);
    getRepeatedTemplate()->MoveToRecycle(plfr);
}
//-+ End of Method-------------------------------------------------------------





//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::FlushCache(), protected
//
//  Synopsis:   moves the entire list of frames into the recycle list, thereby
//              removing it from all ties to real life
//
//  SideEffect:
//
//  Arguments:  CBaseFrame * plfr -> the layout to recycle
//
//  Returns:    S_OK if everything is fine
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::FlushCache(void)
{
    if (_apNode[BEGIN_NODE])
    {
        getRepeatedTemplate()->MoveToRecycle(_apNode[BEGIN_NODE]);
        if ( _pCurrent )
        {
            _pCurrent->Release();
            _pCurrent = NULL;
        }
        _uCached = 0;
    }
    _apNode[BEGIN_NODE] = _apNode[END_NODE] = NULL;
}
//-+ End of Method-------------------------------------------------------------





//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::PutCacheFrameInRecycle(), protected
//
//  Synopsis:   checks if frames in the cache can be recycled
//              it will not move:
//                  all frames who are active
//
//  SideEffect: it will probably remove currently visible frames
//
//  Returns:    S_OK if everything is fine
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::PutCacheFrameInRecycle(void)
{
    if (_uRepeatInPage && _uCached >= (_uRepeatInPage * 2))
    {
        CBaseFrame *plfrRecycle = _apNode[!_fPopulateDirection];
        while (plfrRecycle && _uCached >= (_uRepeatInPage * 2))
        {
            CBaseFrame *plfrNext = plfrRecycle->_apNode[_fPopulateDirection];
            if (!plfrRecycle->IsInSiteList() && !plfrRecycle->IsPopulated() && plfrRecycle != _pCurrent)
            {
                MoveToRecycle(plfrRecycle);
            }
            plfrRecycle = plfrNext;
        }
    }
}
//-+ End of Method-------------------------------------------------------------





//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::FlushNotVisibleCache, protected
//
//  Synopsis:   Put into recycle all the non visible cache
//
//  Returns:    S_OK if everything is fine
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::FlushNotVisibleCache (void)
{
    CBaseFrame *plfrRecycle = _apNode[BEGIN_NODE];
    while (plfrRecycle)
    {
        CBaseFrame *plfrNext = plfrRecycle->_apNode[NEXT_NODE];
        if (!plfrRecycle->IsInSiteList() && plfrRecycle != _pCurrent)
        {
            MoveToRecycle(plfrRecycle);
        }
        plfrRecycle = plfrNext;
    }
}





//+---------------------------------------------------------------------------
//
//  Member:     DeletingRows
//
//  Synopsis:   deleting rows from the cursor
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::DeletingRows (ULONG cRows, const HROW *ahrows)
{
    if (cRows == 1)
    {
        CBaseFrame *plfr, *plfrNext;
        plfr = CacheLookUpRow(ahrows[0]);
        if (plfr)
        {
            if (plfr == _pCurrent)
            {
                plfrNext = plfr->_apNode[NEXT_NODE];
                if (!plfrNext)
                {
                    plfrNext = plfr->_apNode[PREV_NODE];
                }
                SetCurrent(plfrNext);
            }
            // BUGBUG this will do a lot of work with the selection, optimize it later
            IGNORE_HR(plfr->MoveToRow(NULL));
            MoveToRecycle(plfr);
        }
    }
    else
    {
        FlushCache();
    }
    return;
}





//+-------------------------------------------------------------------------
//
//  Method:     CRepeaterFrame::SetProposed
//
//  Synopsis:   Remember the deferred move rectangle for this site
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CRepeaterFrame::SetProposed(CSite * pSite, const CRectl * prcl)
{
    TraceTag((tagPropose, "%ls/%d CRepeater::SetProposed %d,%d,%d,%d to %d,%d,%d,%d",
        TBag()->_cstrName, TBag()->_ID,
        prcl->left - pSite->_rcl.left, prcl->top - pSite->_rcl.top, prcl->right - pSite->_rcl.right, prcl->bottom - pSite->_rcl.bottom,
        prcl->left, prcl->top, prcl->right, prcl->bottom));

    if (pSite == this)
    {
        return super::SetProposed(pSite, prcl);
    }

    _rclProposeRow = *prcl;

    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Method:     CRepeaterFrame::GetProposed
//
//  Synopsis:   Get back the deferred move rectangle for this site
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CRepeaterFrame::GetProposed(CSite * pSite, CRectl * prcl)
{
    if (pSite == this)
    {
        return super::GetProposed(pSite, prcl);
    }

    *prcl = _rclProposeRow;

    TraceTag((tagPropose, "%ls/%d CRepeater::GetProposed %d,%d,%d,%d to %d,%d,%d,%d",
        TBag()->_cstrName, TBag()->_ID,
        prcl->left - pSite->_rcl.left, prcl->top - pSite->_rcl.top, prcl->right - pSite->_rcl.right, prcl->bottom - pSite->_rcl.bottom,
        prcl->left, prcl->top, prcl->right, prcl->bottom));

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CRepeaterFrame::CalcProposedPositions
//
//  Synopsis:   Calculate the proposed positions for children
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CRepeaterFrame::CalcProposedPositions()
{
    getTemplate()->CalcProposedPositions();

    if (!_fProposedSet)
    {
        if (IsAnythingDirty())
        {
            CalcRectangle(TRUE);    // pass fPropose = TRUE
        }
        else
        {
            SetProposed(this, &_rcl);
        }
    }
    else if (!_fMark1)
    {
        HRESULT hr = ProposedFriendFrames();
        if (hr)
        {
            RRETURN (hr);
        }
    }

    TraceTag((tagPropose, "%ls/%d CRepeater::CalcProposedPositions",
    TBag()->_cstrName, TBag()->_ID));

    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CRepeaterFrame::DrawFeedbackRect
//
//  Synopsis:   Draw feedback rectangle (only for children)
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

void
CRepeaterFrame::DrawFeedbackRect(HDC hDC)
{
    CSite ** ppSite;
    CSite ** ppSiteEnd;

    for (ppSite = _arySites, ppSiteEnd = ppSite + _arySites.Size();
        ppSite < ppSiteEnd; ppSite++)
    {
        if ((*ppSite)->IsVisible())
        {
            (*ppSite)->DrawFeedbackRect(hDC);
        }
    }
}
// end of method













//+-------------------------------------------------------------------------
//
//  Method:     CRepeaterFrame::ProposedFriendFrames
//
//  Synopsis:   Calculate the proposed positions for associated frames
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CRepeaterFrame::ProposedFriendFrames(void)
{
    return super::ProposedFriendFrames();
}




//+---------------------------------------------------------------------------
//
//  Member      CRepeaterFrame::ProposedDelta
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
CRepeaterFrame::ProposedDelta(CRectl *prclDelta)
{
    // we want to get resized

    super::ProposedDelta(prclDelta);

    CRectl rcl = _rcl;

    Edge e = (Edge) (1 - getOwner()->IsVertical());
    Edge eOther = (e == edgeLeft ? edgeTop : edgeLeft);
    Edge eOpposite = (Edge) (e+2);
    Edge eOtherOp = (Edge) (eOther+2);

    rcl[e] += (*prclDelta)[e];
    rcl[eOpposite] += (*prclDelta)[eOpposite];

    rcl[eOther]     +=  (*prclDelta)[eOther];
    rcl[eOtherOp]   +=  (*prclDelta)[eOtherOp];

    SetProposed(this, &rcl);

    // we could come from the detail template at this point
    if (!getTemplate()->_fMark1 || IsAnythingDirty())
    {
        (*prclDelta)[eOther] = 0;
        (*prclDelta)[eOtherOp] = 0;
        getTemplate()->ProposedDelta(prclDelta);
        CalcRectangle (TRUE);   // fProposed = TRUE
    }

    return S_OK;
}
//--end of method--------------------------------------------------------------



//+-------------------------------------------------------------------------
//
//  Method:     CRepeaterFrame::ReleaseRow
//
//  Synopsis:   Releases an HROW. Method is save to be called
//              with already released handles
//              released rows get filled with 0
//
//  Returns:    HRESULT
//      BUGBUG: should be moved to dataframe, also change code in detail then
//
//--------------------------------------------------------------------------

HRESULT
CRepeaterFrame::ReleaseRow(HROW *phrow)
{
    CDataLayerCursor *pCursor;
    HRESULT          hr = S_OK;
    TraceTag((tagRepeaterFrame,"CRepeaterFrame::ReleaseRow"));

    Assert(phrow);

    if (*phrow)
    {
        hr = THR(getOwner()->LazyGetDataLayerCursor(&pCursor));
        if (SUCCEEDED(hr))
        {
            Assert(pCursor);
            pCursor->ReleaseRows(1, phrow);
            *phrow=0;
        }
    }

    RRETURN(hr);
}




//+-------------------------------------------------------------------------
//
//  Method:     CRepeaterFrame::PrepareToScroll
//
//  Synopsis:   Prepares the repeater to be scrolled .
//              It caclulates the delta in the direction of the scroll.
//
//  Arguments:  iDirection          0 or 1 ( HORIZONTAl or VERTICAL) scrollbar
//              uCode               type of scrollbar event.
//              fKeyboardScrolling  TRUE is it's a keyboard action.
//              lPosition           position  of the scroll bar Thumb
//              pDelta              pointer to the result (how much the repeater
//                                  is scrolled by).
//
//  Note:       pDelta[iDirection] will be set, the other dimension will
//              remain untouched by this function.
//
//  Returns:    S_OK is returned in case of suceess, error is returned if cursor
//              is not getting the requested row.
//
//--------------------------------------------------------------------------

#define XCHG(eTop, eBottom)    e = eTop; eTop = eBottom; eBottom = e;

HRESULT
CRepeaterFrame::PrepareToScroll (IN     int  iDirection,
                                 IN     UINT uCode,
                                 IN     BOOL fKeyboardScrolling,
                                 IN     long lPosition, OUT long *pDelta,
                                 IN     CBaseFrame *plfr)
{

    HRESULT         hr = S_OK;
    if (iDirection != getDirection ())
        return hr;

    HROW            hrow = NULL;
    int             uLimit = UNLIMITED - 1;
    CRectl          rclRow;
    CRectl          rclDataFrame;
    CRectl          rclRealEstate;
    Edge            eTop = (Edge) getDirection ();
    Edge            eBottom = (Edge) (eTop + 2);
    Edge            e = (Edge)(1 - eTop);
    long            lPageSize;
    long            lPadding;
    int             iLayoutIndex;
    ULONG           ulRecordNumber;
    BOOL            fNotPrepared = TRUE;
    BOOL            fDontPopulateNextPartOfRow = TRUE;
    BOOL            fRealEstate;
    BOOL            fNeedToPopulate = TRUE;
    long            lViewDelta = 0;
    long            lDelta, l;
    unsigned int    uItemsNonRepDir = getItemsNonRepDir();
    int             uAdjustInRow = uItemsNonRepDir - 1;     // need to be signed int
    BOOL            fDeltaCalculated;
    int             iInsertAt = FRAME_NOTFOUND;
    BOOL            fListBoxStyle = IsListBoxStyle();
    long            lScrollTop;
    CDataLayerCursor *pCursor;

    hr = THR(getOwner()->LazyGetDataLayerCursor(&pCursor));
    if (hr)
        goto Error;

    fDeltaCalculated = TRUE;
    getOwner ()->GetDetailSectionRectl (&rclDataFrame);
    rclRealEstate[eTop] = rclDataFrame[eTop];       // in repeated direction the real Estate
    rclRealEstate[eBottom] = rclDataFrame[eBottom]; // is modeled after DataFrame
    rclRealEstate[e] = _rcl[e];                     // in non repeated direction real Estate
    e = (Edge)(e+2);                                // is modeled after the Repater's _rcl.
    rclRealEstate[e] = _rcl[e];
    lPageSize = rclDataFrame.Dimension(eTop);
    lPadding = getPadding(eTop);
    lScrollTop = rclDataFrame[eTop] - _rcl[eTop];

    // Step 1a: find the HROW to populate from
    // Step 1b: set the populate direction (UP/DOWN).
    // Step 1c: if the layout already is created for HROW use it, otherwise
    //          create  one.
    // Step 1d: In order to find out the delta that we need to ScrollBy
    //          we need to Populate the repeater, since in case of the
    //          autosize detail frames we don't know the size of the rows
    //          upfront.
    switch (uCode)
    {
        case fmScrollActionBegin:
begin:
            uLimit++;
            iLayoutIndex = getTopFrameIdx ();
            plfr = getLayoutFrame(iLayoutIndex);
            hr = getOwner()->GetFirstRow (&hrow);
            if (!SUCCEEDED(hr))
                goto Error;

            if (pCursor->IsSameRow(plfr->getHRow(), hrow))
                goto Optimization;

            plfr = NULL;        // we need to create a layout for the first row.
            iInsertAt = 0;
            _fPopulateDirection = POPULATE_DOWN;
            rclRealEstate[eTop] = _rcl[eTop];
            rclRealEstate [eBottom] = rclRealEstate[eTop] + lPageSize;
            pDelta[iDirection] = _rcl[eTop] - rclDataFrame[eTop];
            break;

        case fmScrollActionEnd:
end:
            uLimit++;
            iLayoutIndex = getBottomFrameIdx ();
            plfr = getLayoutFrame(iLayoutIndex);
            if (!IsNewRecordShow())
            {
                hr = getOwner()->GetLastRow (&hrow);
                if (!SUCCEEDED(hr))
                    goto Error;

            }
            // else hrow = NULL (empty row)

            if (pCursor->IsSameRow(plfr->getHRow(), hrow))
                goto Optimization;

            plfr = NULL;        // we need to create a layout for the last row.
            iInsertAt = LASTFRAME;
            _fPopulateDirection = POPULATE_UP;
            rclRealEstate[eBottom] = _rcl[eBottom];
            rclRealEstate[eTop] = rclRealEstate[eBottom] - lPageSize;
            pDelta[iDirection] = _rcl.Dimension(eTop) - lPageSize -
                      (rclDataFrame[eTop] - _rcl[eTop]);
            break;

        case fmScrollActionAbsoluteChange:
absolute:
            if (lPosition <= 0)
            {
                goto begin;
            }
            else  if (lPosition >= (_rcl.Dimension(iDirection) - lPageSize))
            {
                goto end;
            }
            else
            {
                uLimit++;
                long    lTemplateDim = getTemplate()->_rcl.Dimension (iDirection) +
                                       lPadding;
                iInsertAt = (lPosition <= lScrollTop)? 0: LASTFRAME;
                ulRecordNumber = lPosition/lTemplateDim*uItemsNonRepDir + 1;
                // +1 is because record number is 1 based
                hr = getOwner()->GetRowAt(&hrow, ulRecordNumber);
                if (!SUCCEEDED(hr))
                    goto Error;

                iLayoutIndex = getTopFrameIdx ();
                plfr = getLayoutFrame(iLayoutIndex);

                if (pCursor->IsSameRow(plfr->getHRow(), hrow))
                    goto Optimization;
                else
                {
                    plfr = 0;   // to make populate work
                }

                _fPopulateDirection = POPULATE_DOWN;
                getOwner()->CalcRowPosition (ulRecordNumber, &lPosition, 0);
                rclRealEstate[eTop] = _rcl[eTop] + lPosition;
                rclRealEstate [eBottom] = rclRealEstate[eTop] + lPageSize;
                pDelta[iDirection] = lPosition - (rclDataFrame[eTop] - _rcl[eTop]);
            }
            break;

        case SB_INSERTAT:
            _fPopulateDirection = POPULATE_DOWN;
            Assert (plfr);              // plfr is a newly created row.
            l = _rcl[eTop] + lPosition; // get the absolute position.
            if (l > rclDataFrame[eTop] && l < rclDataFrame[eBottom])
            {

                iInsertAt = Find (plfr->_apNode[PREV_NODE]);
                Assert (iInsertAt != FRAME_NOTFOUND);
                iInsertAt++;
                // note: I can not populate less then a page, since I need to
                // call UpdatePosRects after this function and kick out all the
                // rows that are pushed out of the visible region (detail section)
                rclRealEstate[eTop] = l;
                rclRealEstate [eBottom] = l + lPageSize;
                // no scrolling, delta == 0, just population
                // fDeltaCalculated = TRUE;
            }
            else
            {
                if (l > rclDataFrame[eBottom])
                {
                    _fPopulateDirection = POPULATE_UP;
                    iInsertAt = LASTFRAME;
                    lPageSize = -lPageSize;
                    lPadding = -lPadding;
                    XCHG(eTop, eBottom);
                }
                else
                {
                    iInsertAt = 0;
                }
                rclRealEstate[eTop] = _rcl[eTop] + lPosition;
                rclRealEstate [eBottom] = rclRealEstate[eTop] + lPageSize;
                pDelta[iDirection] = lPosition - (rclDataFrame[eTop] - _rcl[eTop]);
            }
            break;

        case fmScrollActionFocusRequest:
            lDelta = pDelta[iDirection];
            Assert (lDelta);
            if (abs(lDelta) >= lPageSize)
            {
                uCode = fmScrollActionAbsoluteChange;
                goto absolute;
            }
            uLimit++;

            Assert (_arySites.Size());
            if (lDelta > 0)
            {
                _fPopulateDirection = POPULATE_DOWN;
                iLayoutIndex = getBottomFrameIdx ();
                iInsertAt = LASTFRAME;
            }
            else
            {
                _fPopulateDirection = POPULATE_UP;
                XCHG(eTop, eBottom);
                iLayoutIndex = getTopFrameIdx ();
                iInsertAt = 0;
            }

            plfr = getLayoutFrame(iLayoutIndex);
            rclRow = plfr->_rcl;
            rclRow[eBottom] += lPadding;

            // DOWN: if (the BOTTOM of the visible row is fully visible)
            // UP:   if (the TOP    of the visible row is fully visible)
            if (rclRow[eBottom] == rclDataFrame[eBottom])
            {
                // DOWN/UP: populate from the NEW next/previous row.
                hr = GetNextLayoutFrame (plfr, 0);
                if (hr)
                    goto Error;
                plfr = plfr->_apNode[_fPopulateDirection];
                // DOWN: Populate from the bottom of the data frame + 1 page.
                // UP:   Populate from the TOP of the data frame - 1 page.
                rclRealEstate [eTop] = rclDataFrame[eBottom];
            }
            else
            {
                // DOWN/UP: We start populate from the last/firt row
                rclRealEstate[eTop] = rclRow[eTop];
            }
            rclRealEstate [eBottom] = rclDataFrame[eBottom] + lDelta;
            break;

        case fmScrollActionLineDown:
            uLimit = 0;
            // FALL THROUGH

        case fmScrollActionPageDown:
            uLimit++;
            _fPopulateDirection = POPULATE_DOWN;
            iLayoutIndex = getBottomFrameIdx(); // - uItemsNonRepDir + 1;
            fNotPrepared = FALSE;
            iInsertAt = LASTFRAME;
            // FALL THROUGH

        case fmScrollActionLineUp:
            if (fNotPrepared)
                uLimit = 0;
            // FALL THROUGH

        case fmScrollActionPageUp:
            if (fNotPrepared)
            {
                uLimit++;
                _fPopulateDirection = POPULATE_UP;
                iLayoutIndex = getTopFrameIdx ();
                lPageSize = -lPageSize;
                lPadding = -lPadding;
                XCHG(eTop, eBottom);
                iInsertAt = 0;
            }

            // The following code is common for PAGE_UP/DOWN, NEXT/PREV_LINE
            if (iLayoutIndex==-1)
                goto Error;

            plfr = getLayoutFrame(iLayoutIndex);
            rclRow = plfr->_rcl;

            rclRow[eBottom] += lPadding;

            // DOWN/UP: We start populate from the last/firt row
            rclRealEstate [eBottom] = rclRow[eTop] + lPageSize;

            // DOWN: if (the BOTTOM of the visible row is fully visible)
            // UP:   if (the TOP    of the visible row is fully visible)
            if (rclRow[eBottom] == rclDataFrame[eBottom] ||
                fKeyboardScrolling || fListBoxStyle)
            {
                // DOWN/UP: populate from the NEW next/previous row.
                hr = GetNextLayoutFrame (plfr, 0);
                if (hr)
                    goto Error;
                plfr = plfr->_apNode[_fPopulateDirection];
                Assert (plfr);

                // DOWN: Populate from the bottom of the data frame + 1 page.
                // UP:   Populate from the TOP of the data frame - 1 page.
                rclRealEstate [eTop] = rclRow [eBottom];
                if (rclDataFrame.Dimension(iDirection) <= rclRow.Dimension(iDirection))
                {
                    // this is the case when we have only 1 row visible
                    rclRealEstate[eBottom] = rclRealEstate[eTop] + lPageSize;
                }
            }
            else
            {
                // DOWN/UP: we are here when last/first row is not fully visible
                // so we are populating from the top/bottom of the last/first row.
                rclRealEstate[eTop] = rclRow[eTop];

                // We need to start populate from the first layout in row.
                if (uLimit != 1)
                {
                    iLayoutIndex += (_fPopulateDirection == POPULATE_UP)?
                                    uAdjustInRow : -uAdjustInRow;
                    Assert (iLayoutIndex >= 0 && iLayoutIndex < _arySites.Size());
                    plfr = getLayoutFrame(iLayoutIndex);
                }
                else
                {
                    // everything is populated, we just need to do pixel scrolling
                    fNeedToPopulate = FALSE;
                }

                if (_uRepeatInPage == 1 && rclRow[eTop] != rclDataFrame[eTop])
                {
                    // this is the case where we want to see the rest of the row
                    // DOWN/UP: from the bottom/top of the view +/- pageSize
                    lViewDelta = rclDataFrame[eBottom] - rclRow[eTop];
                }
            }
            fDeltaCalculated = FALSE;   // need to calculate plDelta[iDirection]
            break;

        default:
            goto Cleanup;
    }

    // Step 2: Make sure we have the layout
    if (!plfr)
    {
        // Check, if we already have this hrow in the cache
        plfr = CacheLookUpRow(hrow);
        if (!plfr)
        {
            BOOL fInvalidated = _fInvalidated;
            _fInvalidated = TRUE;   // to avoid individual invalidation from controls
            BOOL fOldPopulate = _fPopulateDirection;    //push populate flag
            hr = CreateLayoutRelativeToVisibleRange (_rcl[eTop] + lPosition,
                                                     hrow, &plfr, &iInsertAt, 0);
            _fPopulateDirection = fOldPopulate;         //pop populate flag
            _fInvalidated = fInvalidated;
            if (!SUCCEEDED(hr))
                goto Error;
        }
        else
        {
            // The hRow is already in the cache so we can release it now.
            ReleaseRow(&hrow);
        }
    }

    // Step 3: Populate
    if (fNeedToPopulate)
    {
        fRealEstate = Populate (plfr, rclRealEstate, lViewDelta, uLimit,
                                SITEMOVE_NOFIREEVENT | SITEMOVE_NOINVALIDATE,
                                iInsertAt);
        _fProposedSet = FALSE;  // Clear the flag, which was set by Populate
    }

    // Step 4: calculate/adjust the DELTA
    e = (Edge)getDirection ();
    eTop = e;
    eBottom = (Edge)(e + 2);
    if (_fPopulateDirection == POPULATE_DOWN)
    {
        e = eBottom;    // e is bottom/right for (vertical/horizontal rep-tion)
        iLayoutIndex = getBottomFrameIdx();
    }
    else
    {
        // e = eTop;
        iLayoutIndex = getTopFrameIdx();
    }
    plfr = getLayoutFrame(iLayoutIndex);

    if (!fDeltaCalculated)
    {
        if (_fPopulateDirection == POPULATE_DOWN)
        {
            if (IsBottomRowLastInSet ())
                lPadding = 0;
            lDelta = plfr->_rcl[eBottom] + lPadding - rclDataFrame[eBottom];
            Assert (lDelta >= 0);
            pDelta[iDirection] = min(lPageSize, lDelta);
        }
        else
        {
            if (IsTopRowFirstInSet())
                lPadding = 0;
            lDelta = plfr->_rcl[eTop] + lPadding - rclDataFrame[eTop];
            Assert (lDelta <= 0);
            pDelta[iDirection] = max(lPageSize, lDelta);
        }
    }
    else
    {
        if (uCode == fmScrollActionFocusRequest)
        {
            // if the row can fit entirily into the detail section, then make
            // sure the whole row is scrolled in, not just the portion of it.
            if (plfr->_rcl.Dimension(iDirection) < lPageSize)
            {
                if (plfr->_rcl[e] != rclDataFrame[e] + lDelta)
                {
                    pDelta[iDirection] = plfr->_rcl[e] - rclDataFrame[e];
                }
            }
        }
    }

    // Setp 5: Align the top row on top of the data frame
    if (fListBoxStyle)
    {
        CSite       *   pSite;
        CSite       **  ppSite;
        CSite       **  ppSiteEnd;
        l = rclDataFrame[eTop];
        lDelta = pDelta[iDirection];
        for (ppSite = _arySites, ppSiteEnd = ppSite + _arySites.Size();
             ppSite < ppSiteEnd; ppSite++)
        {
            pSite = *ppSite;
            if (pSite->_rcl[eBottom] - lDelta > l)
            {
                break;
            }
        }
        pDelta[iDirection] = pSite->_rcl[eTop] - l;
    }


Cleanup:
    RRETURN1 (hr, S_FALSE);

Optimization:
    ReleaseRow(&hrow);

Error:
    pDelta[iDirection] = 0;
    goto Cleanup;
}





//+---------------------------------------------------------------------------
//
//  Member:     Populate
//
//  Synopsis:   Populate (Up or Down) method. Private.
//              Populate the real estate rectangle (Up or Down).
//              Generate (create) layout instances to populate the passed
//              in rectangle.
//
//  Assumption: _fPopulateDirection is set to POPULATE_UP/POPULATE_DOWN.
//
//  Arguments:  plfr                    Pointer to a layout frame to start
//                                      population from.
//              rclRealEstate           real estate rectangle to populate
//              lViewDelta              view adjustement for the "plfr" row.
//              uLimit                  passed if limited number of ROW/COLUMNs
//                                      of layouts should be created.
//                                      Note: it's not number of layouts.
//              dwFlags                 flags to use for moving sites around
//
//  Returns:    Returns S_OK if populated and no real estate left.
//              S_FALSE if populated and there is more real estate
//              left (rclRealEstate is not empty).
//              Eror result if fetching data from the cursor failed.
//
//  SideEffects:This function calculates also number of repeated rows in page.
//
//  Note:       Whenever we say Row it could also mean a Column, for example:
//              If it's AcrossDown snaking, then we mean Row.
//              If it's DownAcorss snaking, then we mean Column.
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::Populate (IN    CBaseFrame *plfr,
                          INOUT CRectl& rclRealEstate,
                          IN    long lViewDelta,
                          IN    unsigned int uLimit,
                          IN    DWORD dwFlags,
                          IN    int iInsertAt)
{
    TraceTag ((tagRepeaterFrame, "CRepeaterFrame::Populate"));

    HRESULT         hr = S_OK;
    _fDirection = getDirection();   // Has to be initialized before anything else.

    // to avoid bitmap paint
    _pDoc->_fDisableOffScreenPaint = TRUE;

    unsigned int    i, j;
    CBaseFrame  *   plfrPrev = NULL;
    CSite       *   pSite;
    CSite       **  ppSite;
    CSite       **  ppSiteEnd;
    unsigned int    uItemsNonRepDir = getItemsNonRepDir();
    BOOL            fStopByRealEstate = uItemsNonRepDir == UNLIMITED;
    BOOL            fEnoughRealEstate = !rclRealEstate.IsRectEmpty ();
    CRectl          rclPropose;
    Edge            e = (Edge) (!_fDirection);
    Edge            eAdjust, eViewBegin;
    int             lDimension;
    CRectl          rclView;
    unsigned        uRepeatInPage;
    BOOL            fNewFrame;

    if (iInsertAt == FRAME_NOTFOUND)
    {
        iInsertAt = _fPopulateDirection == POPULATE_UP? 0: LASTFRAME;
    }

    uRepeatInPage = 0;             // Number of rows/columns per page (view).

#if PRODUCT_97
    // Note: during this population the _rclPropose will be partly set
    // (the coordinates in the non-repeated direction will be set), to finish
    // calculate the _rclPropose, the CalcProposeRectangle needs to be called.
    // Set the proposed rectangle
    rclPropose.SetRectEmpty ();
    SetProposed(this, &rclPropose);
#endif

    // Zap the "Populated" flag in all of the layouts, that are currently in the
    // site array of the repeater.
    for (ppSite = _arySites, ppSiteEnd = ppSite + _arySites.Size();
         ppSite < ppSiteEnd; ppSite++)
    {
        pSite = *ppSite;
        ((CBaseFrame*)pSite)->SetPopulated (FALSE);    // they are all marked not to show up on the screen
        Assert(((CBaseFrame*)pSite)->IsInSiteList ());
    }

    if (!fEnoughRealEstate || getTemplate()->_rcl.IsRectEmpty())
        goto Cleanup;

    Assert (plfr);                          // parameters validity check

    // to avoid individual invalidation from controls inside
    _fInvalidated = TRUE;

    if (_fPopulateDirection == POPULATE_DOWN)
    {
        eAdjust = edgeRight;
        eViewBegin = (Edge)_fDirection;     // edgeTop for vertical repeteation
        lDimension = MAXLONG;
        if (rclRealEstate[e] < 0)           // to avoid integere overflow.
            lDimension += rclRealEstate[e]; // when we calculate Dimension
    }
    else
    {
        eAdjust = edgeLeft;
        eViewBegin = (Edge)(_fDirection + 2);// edgeBottom for vertical repeteation
        lDimension = MINLONG + 1;            // big negative number
        if (rclRealEstate[(Edge)(e+2)] > 0)  // to avoid integer overflow...
            lDimension  += rclRealEstate[(Edge)(e+2)];
    }

    // This loop starts always with the begining of the row
    // (not in the middle of it).
    for (i = 0; plfr;)
    {
        // Step 1: is fitting layouts in the non-repeatition direction.
        _fDirection = !_fDirection;

        CRectl  rclSubRealEstate (rclRealEstate);   // Proposed real estate
                                                    // rectangle for the row.
        // Prepare the rclRow rectangle
        CRectl  rclRow (rclRealEstate);
        if (_fPopulateDirection == POPULATE_DOWN)
        {
            // in case of populate down the final (right and bottom) coordinates
            // will be calculated as MAX of all the frames in the row
            rclRow.right = rclRow.bottom = MINLONG;
        }
        else
        {
            // in case of "populate up" the final (left and top) coordinates
            // will be calculated as MIN of all the frames in the row
            rclRow.left = rclRow.top = MAXLONG;
        }

        if (!fStopByRealEstate && uItemsNonRepDir != 1)
        {
            // in case when we need to generate many columns, and our original
            // real estate is not sufficient we need to unlimit the
            // right/bottom in case of populate down or
            // left/top in case of populate up.
            rclSubRealEstate [eAdjust] =
            rclSubRealEstate [(Edge)(eAdjust + 1)] = lDimension;
        }

        for (j = 0; plfr; )
        {
            // Calculate iInsertAt.
            if (plfr->IsInSiteList())
            {
                iInsertAt = LASTFRAME;  // indicator, that it's not set
                fNewFrame = FALSE;
            }
            else
            {
                fNewFrame = TRUE;
                if (plfrPrev)
                {
                    // The following check is an optimization not to call Find
                    // on the previous layout all the time.
                    if (iInsertAt == LASTFRAME)
                    {
                        // This code will be called only for the row(s) that were
                        // inserted in between of the 2 existing rows.
                        iInsertAt = Find(plfrPrev);
                    }
                    Assert (POPULATE_DOWN == 1);        // !Important: for the next
                    iInsertAt += _fPopulateDirection;   // statement (this line).
                }
            }

            rclView = rclSubRealEstate;             // TODO: to optimize.
            rclView[eViewBegin] += lViewDelta;
            hr = PopulateWithOneFrame (rclSubRealEstate, rclView, plfr,
                                       iInsertAt,
                                       dwFlags | SITEMOVE_NOINVALIDATE);
            plfrPrev = plfr;

            if (!SUCCEEDED(hr))
                goto Error;

            if (fNewFrame && (dwFlags & SITEMOVE_INVALIDATENEW))
            {
                RECT rc;
                _pDoc->DeviceFromHimetric(&rc, &plfr->_rcl);
                _pDoc->Invalidate(&rc, NULL, NULL, 0);
            }
            CalcRowRectangle (rclRow, &plfr->_rcl);

            fEnoughRealEstate = hr;
            if (fStopByRealEstate? fEnoughRealEstate: ++j < uItemsNonRepDir)
            {
                hr = GetNextLayoutFrame (plfr, &plfr);
                if (hr) // (!SUCCEEDED(hr))
                    goto Error;
            }
            else
                break;
        }
        uRepeatInPage++;
        // Step 2: is fiting the row into the repeater rectangle
        _fDirection = !_fDirection;
        fEnoughRealEstate = EnoughRealEstate (rclRealEstate, rclRow);
        if (++i < uLimit && fEnoughRealEstate)
        {
            hr = GetNextLayoutFrame (plfr, &plfr);
            if (hr) // (!SUCCEEDED(hr))
                goto Error;
        }
        else
            break;
        lViewDelta = 0;          // can be != 0 only for the first row.
    }

    // If the population occured because the Template is dirty, we need to
    // mark all of the frames, that are cached and not in the _arySites
    // as Dirty.
    if (getTemplate()->_fIsDirtyRectangle)
    {
        plfr = _apNode[BEGIN_NODE];
        while (plfr)
        {
            if (!plfr->IsPopulated())
            {
                plfr->_fIsDirtyRectangle = TRUE;
            }
            else
            {
                Assert(plfr->IsInSiteList());
            }
            #if DBG==1 && !defined(PRODUCT_97)
            CheckFrameSizes(plfr);
            #endif
            plfr = plfr->_apNode[NEXT_NODE];
        }
        getTemplate()->EmptyRecycle();
    }

    hr = (HRESULT) fEnoughRealEstate;

Cleanup:

    _uRepeatInPage = uRepeatInPage;

    if (!(dwFlags & SITEMOVE_NOINVALIDATE))
    {
        getOwner ()->GetDetailSectionRectl (&rclView);
        RECT rc;
        _pDoc->DeviceFromHimetric(&rc, &rclView);
        _pDoc->Invalidate(&rc, 0, 0, 0);
    }

    RRETURN1 (hr, S_FALSE);

Error:
    // rich error setting
    if (FAILED(hr))
    {
        if (!ConstructErrorInfo(hr))
        {
            PutErrorInfoText(EPART_ERROR, IDS_ERR_DDOCPOPULATEFAILED);
        }
    }
    goto Cleanup;
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
CRepeaterFrame::UpdatePosRects1 ()
{
    // if repeater was just populated by DataFrame, then we don't need to do
    // it again.
    CRectl          rclDataFrame;
    Edge            eTop;
    Edge            eBottom;
    long            x1, x2, x3, x4, l1, l2, l3, lPadding;
    unsigned int    i;
    CSite       *   pSite;
    unsigned int    uItemsNonRepDir = getItemsNonRepDir();

    getOwner()->GetDetailSectionRectl (&rclDataFrame);

    _fDirection = getDirection ();
    lPadding = getPadding (_fDirection);
    eTop = (Edge)_fDirection;
    eBottom = (Edge) (eTop + 2);

    x3 = rclDataFrame[eTop];
    x4 = rclDataFrame[eBottom];
    l2 = x4 - x3;
    if (l2)
    {
        _uRepeatInPage = 0;
        for (i = 0; i < (unsigned int)_arySites.Size(); i++)
        {
            pSite = _arySites[i];
            Assert(pSite);
            // check if the site within the data frame visiual range
            x1 = pSite->_rcl[eTop];
            x2 = pSite->_rcl[eBottom];
            if (_fPopulateDirection == POPULATE_DOWN)
                x2 += lPadding;
            else
                x1 -= lPadding;
            l1 = x2 - x1;
            l3 = x4 - x1;
            if (!(l3 > 0 && l3 < l1 + l2))
            {
                // if site is not overlapping
                // delete from the site list.
                ((CBaseFrame*)pSite)->SetInSiteList(FALSE);
                ((CBaseFrame*)pSite)->SetPopulated(FALSE);
                pSite->Release();
                _arySites.Delete(i);
                i--;
            }
            else
            {
                _uRepeatInPage++;
                ((CBaseFrame*)pSite)->SetPopulated(TRUE);
            }
        }
        _uRepeatInPage = (_uRepeatInPage + uItemsNonRepDir - 1)/uItemsNonRepDir;
    }
    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Member:     ScrollBy, public
//
//  Synopsis:   Scroll the detail frame into view
//
//              dxl     X delta to move in himetric
//              dyl     Y delta to move in himetric
//              xAction Horizontal scroll action
//              yAction Vertical scroll action
//
//----------------------------------------------------------------------------

HRESULT
CRepeaterFrame::ScrollBy (
        long dxl,
        long dyl,
        fmScrollAction xAction,
        fmScrollAction yAction)
{
    // call from one of the children to scroll by
    HRESULT hr = S_OK;

    int             iDirection;
    long            alDelta[2];
    long            lDelta;
    long            lPosition;
    CRectl          rclDataFrame;
    fmScrollAction  action[2];

    if (!dxl && !dyl)
        goto Cleanup;   // no need for fancy stuff.

    iDirection = getDirection();
    alDelta[0] = dxl;
    alDelta[1] = dyl;
    action[0] = xAction;
    action[1] = yAction;
    lDelta = alDelta[iDirection];
    alDelta[ 1 - iDirection] = 0;       // BUGBUG: review it.

    getOwner ()->GetDetailSectionRectl (&rclDataFrame);
    // calculate lPosition for PrepareToScroll, which is relative to the
    // begining of the repeater's _rcl (note the lDelta is relative to the
    // beginig of the dataframe)
    lPosition = lDelta + rclDataFrame[(Edge)iDirection] - _rcl[(Edge)iDirection];
    hr = PrepareToScroll (iDirection, action[iDirection], FALSE,
                          lPosition, alDelta, 0);
    if (hr)
        goto Cleanup;

    hr = getOwner()->ScrollBy (alDelta[0], alDelta[1], xAction, yAction);

Cleanup:
    RRETURN1 (hr, S_FALSE);
}





//+---------------------------------------------------------------------------
//
//  Member:     MoveSiteBy
//
//  Synopsis:   Move the site with delta
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::MoveSiteBy (long x, long y)
{
    if (_pCurrent && !_pCurrent->IsInSiteList())
    {
        _pCurrent->MoveSiteBy(x, y);
    }
    super::MoveSiteBy(x, y);
}




//+---------------------------------------------------------------------------
//
//  Member:     CRepeaterFrame::GetClientRectl (virtual)
//
//  Synopsis:   Get content area.
//
//----------------------------------------------------------------------------

void
CRepeaterFrame::GetClientRectl (RECTL *prcl)
{
    CRectl  rcl;
    getOwner()->GetDetailSectionRectl (&rcl);
    ((CRectl *)prcl)->Intersect(&rcl, &_rcl);
    return;
}


#if OLD_CODE
HRESULT
CRepeaterFrame::ScrollBy (
        CBaseFrame *plfr,
        long dxl,
        long dyl,
        fmScrollAction xAction,
        fmScrollAction yAction)
{
    HRESULT hr;

    int iDirection = getDirection ();
    long alDelta[2];
    long lDelta;
    alDelta[0] = dxl;
    alDelta[1] = dyl;
    lDelta = alDelta[iDirection];

    // if the frame is already in the array site, it means the delta
    // is already calculated correctly, we don't need to prepare the
    // repeater for scroll, just need to do pixel scrolling.
    if (Find(plfr) != FRAME_NOTFOUND && lDelta)
    {
        Assert (plfr == _pCurrent);
        hr = PrepareToScroll (iDirection, fmScrollActionFocusRequest, FALSE,
                              lDelta, alDelta, plfr);
        // BUGBUG: do better error handling
    }

    hr = getOwner()->ScrollBy (alDelta[0], alDelta[1], xAction, yAction);
    RRETURN1 (hr, S_FALSE);
}
#endif



//+---------------------------------------------------------------
//
//  Member:     CRepeaterFrame::DragEnter
//
//  Synopsis:   only here for eventfiring in case of empty repeater
//
//  Arguments   pDataObject     pointer to a Data Object
//
//  Returns     HRESULT
//
//---------------------------------------------------------------
HRESULT
CRepeaterFrame::DragEnter( LPDATAOBJECT pDataObj,
    DWORD grfKeyState,
    POINTL ptlScreen,
    LPDWORD pdwEffect)
{
    HRESULT             hr = S_FALSE;
    VARIANT_BOOL        fCancel;

    if (!_arySites.Size())
    {
        hr = FireDragEvent(fmDragStateEnter, grfKeyState, ptlScreen, pdwEffect, &fCancel);

        if (!fCancel)
        {
            // only if we are empty....
            *pdwEffect = DROPEFFECT_NONE;
        }   
    }

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     CRepeaterFrame::DragOver
//
//  Synopsis:   only for eventfiring in empty repeater
//
//  Arguments   grfKeyState     Key status
//              ptlScreen       where the mouse is
//              pdwEffect       special effects flags
//
//  Returns     HRESULT
//
//---------------------------------------------------------------
HRESULT
CRepeaterFrame::DragOver(DWORD grfKeyState, POINTL ptlScreen, LPDWORD pdwEffect)
{
    VARIANT_BOOL fCancel;

    if (!_arySites.Size())
    {
        FireDragEvent(fmDragStateOver, grfKeyState, ptlScreen, pdwEffect, &fCancel);

        if (!fCancel)
        {
            // only if we are empty....
            *pdwEffect = DROPEFFECT_NONE;
        }
    }

    RRETURN(S_OK);
}


//+---------------------------------------------------------------
//
//  Member:     CRepeaterFrame::DragLeave
//
//  Synopsis:   Remove any user feedback
//
//  Arguments   None
//
//  Returns     HRESULT
//
//---------------------------------------------------------------
HRESULT
CRepeaterFrame::DragLeave(void)
{
    POINTL              ptlScreen;
    DWORD               dwEffect;
    VARIANT_BOOL        fCancel;

    if (!_arySites.Size())
    {
        FireDragEvent(fmDragStateLeave, 0, ptlScreen, &dwEffect, &fCancel);

        if (!fCancel)
        {
            // only if we are empty....
            dwEffect = DROPEFFECT_NONE;
        }    
    }

    RRETURN(S_OK);
}


//+---------------------------------------------------------------
//
//  Member:     CRepeaterFrame::Drop
//
//  Synopsis:   Handle the drop operation
//
//---------------------------------------------------------------

//  BUGBUG should be global?
#define DROPEFFECT_ALL (DROPEFFECT_NONE | DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK)

HRESULT
CRepeaterFrame::Drop(
        LPDATAOBJECT pDataObj,
        DWORD        grfKeyState,
        POINTL       ptlScreen,
        LPDWORD      pdwEffect)
{
    HRESULT         hr;
    VARIANT_BOOL    fCancel;

    hr = FireDropEvent(grfKeyState, ptlScreen, pdwEffect, &fCancel);

    if (!fCancel)
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    RRETURN(hr);
}








//
//  end of file
//
//----------------------------------------------------------------------------

