//+---------------------------------------------------------------------------
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       layout.cxx
//
//  Contents:   Implementation of CLayout and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_DRAWINFO_HXX_
#define X_DRAWINFO_HXX_
#include "drawinfo.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_FILTER_HXX_
#define X_FILTER_HXX_
#include "filter.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif

#ifndef X_ELABEL_HXX_
#define X_ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_AVUNDO_HXX_
#define X_AVUNDO_HXX_
#include "avundo.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_INITGUID_H_
#define X_INITGUID_H_
#define INIT_GUID
#include <initguid.h>
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_XBAG_HXX_
#define X_XBAG_HXX_
#include "xbag.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include <mshtmdid.h>
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

#ifndef X_DISPITEMPLUS_HXX_
#define X_DISPITEMPLUS_HXX_
#include "dispitemplus.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPCONTAINERPLUS_HXX_
#define X_DISPCONTAINERPLUS_HXX_
#include "dispcontainerplus.hxx"
#endif

#ifndef X_DISPSCROLLERPLUS_HXX_
#define X_DISPSCROLLERPLUS_HXX_
#include "dispscrollerplus.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_BODYLYT_HXX_
#define X_BODYLYT_HXX_
#include "bodylyt.hxx"
#endif

#ifndef X_SCROLLBAR_HXX_
#define X_SCROLLBAR_HXX_
#include "scrollbar.hxx"
#endif

#ifndef X_SCROLLBARCONTROLLER_HXX_
#define X_SCROLLBARCONTROLLER_HXX_
#include "scrollbarcontroller.hxx"
#endif

#ifndef _X_SELDRAG_HXX_
#define _X_SELDRAG_HXX_
#include "seldrag.hxx"
#endif

#ifndef X_OPTSHOLD_HXX_
#define X_OPTSHOLD_HXX_
#include "optshold.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx" // needed for EVENTPARAM
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_TAREALYT_HXX_
#define X_TAREALYT_HXX_
#include "tarealyt.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_ADORNER_HXX_
#define X_ADORNER_HXX_
#include "adorner.hxx"
#endif

#ifndef X_COLOR3D_HXX_
#define X_COLOR3D_HXX_
#include "color3d.hxx"
#endif

#ifndef X_IMGLYT_HXX_
#define X_IMGLYT_HXX_
#include "imglyt.hxx"
#endif

// BUGBUG: Need to figure out correct public place for this error.
// Note: error code is first outside of range reserved for OLE.
#define S_MSG_KEY_IGNORED \
    MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_ITF, 0x201)

MtDefine(CLayout, Layout, "CLayout")
MtDefine(CRequest, Layout, "CRequest")
MtDefine(CBgRecalcInfo, Layout, "CBgRecalcInfo")
MtDefine(CLayout_aryRequests_pv, Layout, "CLayout RequestQueue")
MtDefine(CLayoutDetach_aryChildLytElements_pv, Locals, "CLayout::Detach aryChildLytElements::_pv")
MtDefine(CLayoutScopeFlag, Locals, "CLayout::CScopeFlag")

DeclareTag(tagCalcSize,    "CalcSize",         "Trace calls to CalcSize");
DeclareTag(tagShowZeroGreyBorder,    "Edit",         "Show Zero Grey Border in Red");

ExternTag(tagLayout);
ExternTag(tagLayoutNoShort);

PerfDbgExtern(tagPaintWait);

const COLORREF ZERO_GREY_COLOR = RGB(0xC0,0xC0,0xC0);


//+----------------------------------------------------------------------------
//
//  Member:     GetDispNodeElement
//
//  Synopsis:   Return the element associated with a display node
//
//-----------------------------------------------------------------------------

CElement *
GetDispNodeElement(
    CDispNode *   pDispNode)
{
    CElement *  pElement;

    Assert(pDispNode);

    pDispNode->GetDispClient()->GetOwner(pDispNode, (void **)&pElement);

    if (!pElement)
    {
        pElement = DYNCAST(CAdorner, (CAdorner *)pDispNode->GetDispClient())->GetElement();
    }

    Assert(pElement);
    Assert(DYNCAST(CElement, pElement));

    return pElement;
}


//+----------------------------------------------------------------------------
//
//  Member:     CRequest::CRequest/~CRequest
//
//  Synopsis:   Construct/destruct a CRequest object
//
//-----------------------------------------------------------------------------

inline
CRequest::CRequest(
    REQUESTFLAGS    rf,
    CElement *      pElement)
{
    Assert(pElement);

    pElement->AddRef();

    _cRefs    = 1;
    _pElement = pElement;
    _grfFlags = rf;
}

inline
CRequest::~CRequest()
{
    _pElement->DelRequestPtr();
    _pElement->Release();

    if (_pAdorner)
    {
        _pAdorner->Release();
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CRequest::AddRef/Release
//
//  Synopsis:   Maintain CRequest reference count
//
//-----------------------------------------------------------------------------

inline DWORD
CRequest::AddRef()
{
    Assert(_cRefs > 0);
    return ++_cRefs;
}

inline DWORD
CRequest::Release()
{
    Assert(_cRefs > 0);
    --_cRefs;
    if (!_cRefs)
    {
        delete this;
        return 0;
    }
    return _cRefs;
}


//+----------------------------------------------------------------------------
//
//  Member:     CRequest::SetFlag/ClearFlag/IsFlagSet
//
//  Synopsis:   Manage CRequest flags
//
//-----------------------------------------------------------------------------

inline void
CRequest::SetFlag(REQUESTFLAGS rf)
{
    _grfFlags |= rf;
}

inline void
CRequest::ClearFlag(REQUESTFLAGS rf)
{
    _grfFlags &= ~rf;
}

inline BOOL
CRequest::IsFlagSet(REQUESTFLAGS rf) const
{
    return _grfFlags & rf;
}

//+----------------------------------------------------------------------------
//
//  Member:     CRequest::QueuedOnLayout/DequeueFromLayout/GetLayout/SetLayout
//
//  Synopsis:   Manage CRequest layout pointers
//
//-----------------------------------------------------------------------------

inline BOOL
CRequest::QueuedOnLayout(
    CLayout *   pLayout) const
{
    return  _pLayoutMeasure  == pLayout
        ||  _pLayoutPosition == pLayout
        ||  _pLayoutAdorner  == pLayout;
}

void
CRequest::DequeueFromLayout(
    CLayout *   pLayout)
{
    if (_pLayoutMeasure == pLayout)
    {
        _pLayoutMeasure = NULL;
    }

    if (_pLayoutPosition == pLayout)
    {
        _pLayoutPosition = NULL;
    }

    if (_pLayoutAdorner == pLayout)
    {
        _pLayoutAdorner = NULL;
    }
}

CLayout *
CRequest::GetLayout(
    REQUESTFLAGS    rf) const
{
    switch (rf)
    {
    default:
    case RF_MEASURE:    return _pLayoutMeasure;
    case RF_POSITION:   return _pLayoutPosition;
    case RF_ADDADORNER: return _pLayoutAdorner;
    }
}

inline void
CRequest::SetLayout(
    REQUESTFLAGS    rf,
    CLayout *       pLayout)
{
    switch (rf)
    {
    case RF_MEASURE:    _pLayoutMeasure  = pLayout; break;
    case RF_POSITION:   _pLayoutPosition = pLayout; break;
    case RF_ADDADORNER: _pLayoutAdorner  = pLayout; break;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CRequest::GetElement/GetAdorner/SetAdorner/GetAuto/SetAuto
//
//  Synopsis:   Various CRequest accessors
//
//-----------------------------------------------------------------------------

inline CElement *
CRequest::GetElement() const
{
    return _pElement;
}

inline CAdorner *
CRequest::GetAdorner() const
{
    return _pAdorner;
}

inline void
CRequest::SetAdorner(CAdorner * pAdorner)
{
    Assert(pAdorner);

    if (pAdorner != _pAdorner)
    {
        if (_pAdorner)
        {
            _pAdorner->Release();
        }

        pAdorner->AddRef();
        _pAdorner = pAdorner;
    }
}

inline CPoint &
CRequest::GetAuto()
{
    return _ptAuto;
}

void
CRequest::SetAuto(const CPoint & ptAuto, BOOL fAutoValid)
{
    if (fAutoValid)
    {
        _ptAuto    = ptAuto;
        _grfFlags |= RF_AUTOVALID;
    }
    else
    {
        _grfFlags &= ~RF_AUTOVALID;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CLayout::CLayout
//
//  Synopsis:   Normal constructor.
//
//  Arguments:  CElement * - element that owns the layout
//
//---------------------------------------------------------------

CLayout::CLayout(CElement * pElementLayout)
{
    Assert(pElementLayout);

    _pElementOwner = pElementLayout;

    __pvChain = pElementLayout->Doc();
    WHEN_DBG( _pDocDbg = pElementLayout->Doc() );

    Assert( _pDocDbg && _pDocDbg->AreLookasidesClear( this, LOOKASIDE_LAYOUT_NUMBER ) );

    _ulRefs = 1;

    _fOwnTabOrder        = TRUE;
    _fContentsAffectSize = TRUE;
    _fSizeThis           = TRUE;
    _fVisible            = TRUE;
    _fAutoBelow          = FALSE;
    _fPositionSet        = FALSE;
    _fContainsRelative   = FALSE;
    _fIsEditable         = FALSE;
    _fEditableDirty      = TRUE;

    _fAllowSelectionInDialog = FALSE;
}

CLayout::~CLayout()
{
    Assert(!_pDispNode);
}


CDoc *
CLayout::Doc() const
{
    Assert( !_fDetached || !_fHasMarkupPtr );
    Assert( _pDocDbg == ( _fHasMarkupPtr ? ( (CMarkup*)__pvChain )->Doc() : (CDoc *)__pvChain ) );
    return _fHasMarkupPtr ? ( (CMarkup*)__pvChain )->Doc() : (CDoc *)__pvChain;
}

CFlowLayout *
CElement::HasFlowLayout()
{
    CLayout     * pLayout = GetUpdatedLayout();

    return (pLayout ? pLayout->IsFlowLayout() : NULL);
}

CTableLayout *
CElement::HasTableLayout()
{
    CLayout * pLayout = GetUpdatedLayout();
    CTableLayout * pTableLayout = pLayout
                                ? pLayout->IsTableLayout()
                                : NULL;
    return pTableLayout;
}


//+----------------------------------------------------------------------------
//
//  Member:     Init, virtual
//
//  NOTE:       every derived class overriding it should call super::Init
//
//-----------------------------------------------------------------------------

HRESULT
CLayout::Init()
{
    HRESULT         hr = S_OK;
    CPeerHolder *   pPeerHolder = _pElementOwner->GetRenderPeerHolder();

    if (pPeerHolder)
    {
        hr = THR(pPeerHolder->OnLayoutAvailable(this));
    }

    RRETURN (hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     OnExitTree
//
//  Synopsis:   Dequeue the pending layout request (if any)
//
//-----------------------------------------------------------------------------
HRESULT
CLayout::OnExitTree()
{
    Reset(TRUE);

    DestroyDispNode();

    //
    // Make sure we do a full recalc if we get added back into a tree somewhere.
    //
    _fSizeThis = TRUE;

    if ( _fTextSelected )
    {
        SetSelected( FALSE, FALSE );
    }

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     Reset
//
//  Synopsis:   Dequeue the pending layout request (if any)
//
//  Arguments:  fForce - TRUE to always reset, FALSE only reset if there are no pending requests
//
//-----------------------------------------------------------------------------
void
CLayout::Reset(
    BOOL    fForce)
{
    if (fForce)
    {
        RemoveLayoutRequest();
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Reset the channel
//
//-----------------------------------------------------------------------------
void
CLayout::Detach()
{
    // This must happen after the filter has been
    // detached so that the filter has a chance
    // to restore things first
    if (_fSurface)
    {
        Assert(Doc()->_cSurface > 0);
        Doc()->_cSurface--;
    }

    if (_f3DSurface)
    {
        Assert(Doc()->_c3DSurface > 0);
        Doc()->_c3DSurface--;
    }

    DestroyDispNode();

    FlushRequests();

    _fDetached = TRUE;
}


//+------------------------------------------------------------------------
//
//  Member:     CLayout::QueryInterface, IUnknown
//
//-------------------------------------------------------------------------
HRESULT
CLayout::QueryInterface(REFIID riid, LPVOID * ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;

    if(riid == IID_IUnknown)
    {
        *ppv = this;
    }

    if(*ppv == NULL)
    {
        hr = E_NOINTERFACE;
    }
    else
    {
        ((LPUNKNOWN)* ppv)->AddRef();
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member: CLayout::RestrictPointToClientRect
//
//  Params: [ppt]: The point to be clipped to the client rect of the elements
//                 layout. The point coming in is assumed to be in global
//                 client window coordinates.
//
//  Descr:  This function converts a point in site relative coordinates
//          to global client window coordinates.
//
//----------------------------------------------------------------------------
void
CLayout::RestrictPointToClientRect(POINT *ppt)
{
    RECT rcClient;

    Assert(ppt);

    GetClientRect(&rcClient, COORDSYS_GLOBAL);

    ppt->x = max(ppt->x, rcClient.left);
    ppt->x = min(ppt->x, (long)(rcClient.right - 1));
    ppt->y = max(ppt->y, rcClient.top);
    ppt->y = min(ppt->y, (long)(rcClient.bottom - 1));
}


//+------------------------------------------------------------------------
//
//  Member:     CLayout::GetClientRect
//
//  Synopsis:   Return client rectangle
//
//              This routine, by default, returns the rectangle in which
//              content should be measured/rendered, the region inside
//              the borders (and scrollbars - if a vertical scrollbar is
//              allowed, space for it is always removed, even if it is not
//              visible).
//              The coordinates are relative to the top of the content -
//              meaning that, for scrolling layouts, the scroll offset is
//              included.
//
//              The following flags can be used to modify this behavior:
//
//                  COORDSYS_xxxx   - Target coordinate system
//                  CLIENTRECT_CONTENT     - Return standard client rectangle (default)
//                  CLIENTRECT_USERECT     - Use the passed rectangle (this is useful
//                                    only for determing dimensions, not location)
//                  CLIENTRECT_BACKGROUND  - Return the rectangle for the background
//                                    (This generally the same as the default rectangle
//                                     except no space is reserved for the vertical
//                                     scrollbar and scroll offsets are not applied
//                                     if the background is fixed)
//                  CLIENTRECT_VISIBLECONTENT - Return the client area rectangle including
//                                    any area made open by hidden (but enabled)
//                                    scrollbars. Such available space is removed
//                                    by default.
//
//              CLIENTRECT_USERECT may not be specified with COORDSYS_GLOBAL
//
//  Arguments:  prc  - returns the client rect
//              cs   - COORDSYS_xxxx
//              crt  - CLIENTRECT_xxxx
//              pdci - doc calc info (only required when CLIENTRECT_USERECT is specified)
//
//-------------------------------------------------------------------------

void
CLayout::GetClientRect(
    CRect *             prc,
    COORDINATE_SYSTEM   cs,
    CLIENTRECT          crt,
    CDocInfo *          pdci) const
{
    Assert(prc);
    Assert( crt != CLIENTRECT_USERECT
        ||  cs  == COORDSYS_CONTENT);

    if (crt != CLIENTRECT_USERECT)
    {
        CDispNode * pDispNode = GetElementDispNode();

        if (pDispNode)
        {
            pDispNode->GetClientRect(prc, crt);
            pDispNode->TransformRect(prc, COORDSYS_CONTENT, cs);
        }
        else
        {
            *prc = g_Zero.rc;
        }
    }

    else
    {
//
//  BUGBUG: Replace this with a CDispNode::xxxxx static method that takes a size and a bunch of parameters
//          that populate an on-the-static DISPEXTRAS - it then calls the same routines that the display
//          tree uses to answer the question for a real CDispNode (as in the above). This leaves all the code
//          etc. in one place (the display tree). (brendand)
//

        CDispNodeInfo   dni;

        Assert(pdci);

        GetDispNodeInfo(&dni, pdci, TRUE);

        if (dni.GetBorderType() != DISPNODEBORDER_NONE)
        {
            CRect   rcBorders;

            dni.GetBorderWidths(&rcBorders);

            prc->left   += pdci->DocPixelsFromWindowX(rcBorders.left);
            prc->top    += pdci->DocPixelsFromWindowY(rcBorders.top);
            prc->right  -= pdci->DocPixelsFromWindowX(rcBorders.right);
            prc->bottom -= pdci->DocPixelsFromWindowY(rcBorders.bottom);
        }

        if (crt != CLIENTRECT_BACKGROUND)
        {
            if (dni.IsVScrollbarAllowed())
            {
                prc->right -= g_sizeScrollbar.cx;
            }
        }

        if (dni.IsHScrollbarForced())
        {
            prc->bottom -= g_sizeScrollbar.cy;
        }

        prc->MoveToOrigin();

        if (prc->right < prc->left)
        {
            prc->right = prc->left;
        }
        if (prc->bottom < prc->top)
        {
            prc->bottom = prc->top;
        }
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CLayout::GetClippedClientRect
//
//  Synopsis:   Return the clipped client rectangle
//
//              This routine functions the same as GetClientRect except
//              that CLIENTRECT_USERECT is illegal.
//
//  Arguments:  prc  - returns the client rect
//              cs   - COORDSYS_xxxx
//              crt  - CLIENTRECT_xxxx
//
//-------------------------------------------------------------------------

void
CLayout::GetClippedClientRect(
    CRect *             prc,
    COORDINATE_SYSTEM   cs,
    CLIENTRECT          crt) const
{
    Assert(prc);
    Assert(crt != CLIENTRECT_USERECT);

    CDispNode * pDispNode = GetElementDispNode();

    if (pDispNode)
    {
        pDispNode->GetClippedClientRect(prc, crt);
        pDispNode->TransformRect(prc, COORDSYS_CONTENT, cs);
    }
    else
    {
        *prc = g_Zero.rc;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CLayout::CalcSize
//
//  Synopsis:   Calculate the size of the object
//
//              Container elements call this on children whenever they need the size
//              of the child during measuring. (The child may or may not measure its
//              contents in response.)
//
//              NOTE: There is no standard helper for assisting containers with
//                    re-sizing immediate descendents. Each container must implement
//                    its own algorithms for sizing children.
//
//  Arguments:  pci       - Current device/transform plus
//                  _smMode     - Type of size to calculate/return
//                                  SIZEMODE_NATURAL  - Return object size using pDI._sizeParent,
//                                                      available size, user specified values,
//                                                      and object contents as input
//                                  SIZEMODE_MMWIDTH  - Return the min/maximum width of the object
//                                  SIZEMODE_SET      - Override/set the object's size
//                                                      (must be minwidth <= size.cx <= maxwidth)
//                                  SIZEMODE_PAGE     - Return object size on the current page
//                                  SIZEMODE_MINWIDTH - Return the minimum width of the object
//                  _grfLayout  - One or more LAYOUT_xxxx flags
//                  _hdc        - Measuring HDC (cannot be used for rendering)
//                  _sizeParent - Size of parent site
//                  _yBaseLine  - y offset of baseline (returned for SIZEMODE_NATURAL)
//              psize     - Varies with passed SIZEMODE
//                          NOTE: Available size is usually that space between the where
//                                the site will be positioned and the right-hand edge. Percentage
//                                sizes are based upon the parent size, all others use available
//                                size.
//                              SIZEMODE_NATURAL  - [in]  Size available to the object
//                                                  [out] Object size
//                              SIZEMODE_MMWIDTH  - [in]  Size available to the object
//                                                  [out] Maximum width in psize->cx
//                                                        Minimum width in psize->cy
//                                                        (If the minimum cannot be calculated,
//                                                         psize->cy will be less than zero)
//                              SIZEMODE_SET      - [in]  Size object should become
//                                                  [out] Object size
//                              SIZEMODE_PAGE     - [in]  Available space on the current page
//                                                  [out] Size of object on the current page
//                              SIZEMODE_MINWIDTH - [in]  Size available to the object
//                                                  [out] Minimum width in psize->cx
//              psizeDefault - Default size (optional)
//
//              pcch        - return the number of characters layed out by
//                            layout object
//
//  Returns:    S_OK if the calcsize is successful
//
//--------------------------------------------------------------------------

DWORD
CLayout::CalcSize( CCalcInfo * pci, 
                   SIZE * psize, 
                   SIZE * psizeDefault)
{
    Assert(pci);
    Assert(psize);
    Assert(ElementOwner());
    CScopeFlag      csfCalcing(this);
    CElement::CLock LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
    CSaveCalcInfo   sci(pci, this);
    CSize           size;
    SIZE            sizeOriginal;
    CTreeNode     * pTreeNode = GetFirstBranch();
    DWORD           grfReturn;

    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);

    if (pci->_grfLayout & LAYOUT_FORCE)
    {
        _fSizeThis         = TRUE;
        _fAutoBelow        = FALSE;
        _fPositionSet      = FALSE;
        _fContainsRelative = FALSE;
    }

    //
    // Ensure the display nodes are correct
    // (If they change, then force measuring since borders etc. may need re-sizing)
    //

    if (    (   pci->_smMode == SIZEMODE_NATURAL
            ||  pci->_smMode == SIZEMODE_SET
            ||  pci->_smMode == SIZEMODE_FULLSIZE)
        &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
    {
        grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
        _fSizeThis = TRUE;
    }

    // If this object needs sizing, then determine its size
    if (pci->_smMode != SIZEMODE_SET)
    {
        if (_fSizeThis)
        {
            CUnitValue  uvx;
            CUnitValue  uvy;

            // If no defaults are supplied, assume zero as the default
            if (!psizeDefault)
            {
                psizeDefault = &g_Zero.size;
            }

            // Set size from user specified or default values
            // (Also, "pin" user specified values to nothing less than zero)
            uvx = pTreeNode->GetCascadedwidth();
            size.cx = (uvx.IsNullOrEnum() || (pci->_grfLayout & LAYOUT_USEDEFAULT)
                            ? psizeDefault->cx
                            : max(0L, uvx.XGetPixelValue(pci, pci->_sizeParent.cx,
                                                        pTreeNode->GetFontHeightInTwips(&uvx))));

            uvy = pTreeNode->GetCascadedheight();
            size.cy = (uvy.IsNullOrEnum() || (pci->_grfLayout & LAYOUT_USEDEFAULT)
                            ? psizeDefault->cy
                            : max(0L, uvy.YGetPixelValue(pci, pci->_sizeParent.cy,
                                                        pTreeNode->GetFontHeightInTwips(&uvy))));

            _fContentsAffectSize = (    (   uvx.IsNullOrEnum()
                                        ||  uvy.IsNullOrEnum())
                                    &&  !(pci->_grfLayout & LAYOUT_USEDEFAULT));


            // For BODY elements, ensure the size is at least as large as the parent
            if (ElementOwner()->TestClassFlag(CElement::ELEMENTDESC_BODY))
            {
                size.cx = max(size.cx, pci->_sizeParent.cx);
                size.cy = max(size.cy, pci->_sizeParent.cy);
                _fContentsAffectSize = FALSE;
            }
            else if (ElementOwner()->Tag() == ETAG_ROOT)
            {
                _fContentsAffectSize = FALSE;
            }
        }
        else
        {
            GetSize(&size);
        }
    }

    // If the object's size is being set, take the passed size
    else
    {
        size = *psize;
    }

    // Return the size of the object (as per the request type)
    switch (pci->_smMode)
    {
    case SIZEMODE_NATURAL:
    case SIZEMODE_SET:
    case SIZEMODE_FULLSIZE:
        // Use the object's size
        *psize = size;

        _fSizeThis  = FALSE;
        grfReturn  |= LAYOUT_THIS |
                      (size.cx != sizeOriginal.cx
                            ? LAYOUT_HRESIZE
                            : 0)  |
                      (size.cy != sizeOriginal.cy
                            ? LAYOUT_VRESIZE
                            : 0);

        //
        // If size changed, resize display nodes
        //

        if (    _pDispNode
            &&  (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE)))
        {
            SizeDispNode(pci, size);

            if (ElementOwner()->IsAbsolute())
            {
                ElementOwner()->SendNotification(NTYPE_ELEMENT_SIZECHANGED);
            }
        }
        break;

    case SIZEMODE_MMWIDTH:
        // Use the object's width, unless it is a percentage, then use zero
        psize->cx =
        psize->cy = (pTreeNode->GetCascadedwidth().GetUnitType() == CUnitValue::UNIT_PERCENT
                            ? 0
                            : size.cx);
        break;

    case SIZEMODE_PAGE:
        // Use the object's size if it fits, otherwise use zero
        psize->cx = (size.cx < psize->cx
                            ? size.cx
                            : 0);
        psize->cy = (size.cy < psize->cy
                            ? size.cy
                            : 0);
        break;

    default:
        // Use the object's width, unless it is a percentage, then use zero
        Assert(pci->_smMode == SIZEMODE_MINWIDTH);
        psize->cx =
        psize->cy = (pTreeNode->GetCascadedwidth().GetUnitType() == CUnitValue::UNIT_PERCENT
                            ? 0
                            : size.cx);
        break;
    }

    TraceTagEx((tagCalcSize, TAG_NONAME,
               "CalcSize  : Layout(0x%x, %S) size(%d,%d) mode(%S)",
               this,
               ElementOwner()->TagName(),
               psize->cx,
               psize->cy,
               (    pci->_smMode == SIZEMODE_NATURAL
                    ? _T("NATURAL")
                :   pci->_smMode == SIZEMODE_SET
                    ? _T("SET")
                :   pci->_smMode == SIZEMODE_FULLSIZE
                    ? _T("FULLSIZE")
                :   pci->_smMode == SIZEMODE_MMWIDTH
                    ? _T("MMWIDTH")
                    : _T("MINWIDTH"))));

    return grfReturn;
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetRect
//
//  Synopsis:   Return the current rectangle of the layout
//
//  Arguments:  psize - Pointer to CSize
//              cs    - Coordinate system for returned values
//
//-----------------------------------------------------------------------------

void
CLayout::GetRect(
    CRect *             prc,
    COORDINATE_SYSTEM   cs) const
{
    Assert(prc);

    if (_pDispNode)
    {
        CSize   size;

        _pDispNode->GetSize(&size);
        prc->SetRect(_pDispNode->GetPosition(), size);

        switch (cs)
        {
        case COORDSYS_CONTAINER:
            prc->MoveToOrigin();
            break;

        case COORDSYS_CONTENT:
        case COORDSYS_GLOBAL:
            _pDispNode->TransformRect(prc, COORDSYS_PARENT, cs);
            break;

        default:
            Assert(cs == COORDSYS_PARENT);
            break;
        }
    }
    else
    {
        *prc = g_Zero.rc;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetClippedRect
//
//  Synopsis:   Return the current clipped rectangle of the layout
//
//  Arguments:  prc   - Pointer to CRect
//              cs    - Coordinate system for returned values
//
//-----------------------------------------------------------------------------

void
CLayout::GetClippedRect(
    CRect *             prc,
    COORDINATE_SYSTEM   cs) const
{
    Assert(prc);

    if (_pDispNode)
    {
        _pDispNode->GetClippedBounds(prc, cs);
    }
    else
    {
        *prc = g_Zero.rc;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetSize
//
//  Synopsis:   Return the current width/height of the layout
//
//  Arguments:  psize - Pointer to CSize
//
//-----------------------------------------------------------------------------

void
CLayout::GetSize(
    CSize * psize) const
{
    Assert(psize);

// BUGBUG: The following would be a nice assert. Unfortunately, it's not easily wired in right now
//         we should do the work to make it possible (brendand)
//    Assert((((CLayout *)this)->TestLock(CElement::ELEMENTLOCK_SIZING)) || !_fSizeThis);

    if (_pDispNode)
    {
        _pDispNode->GetSize(psize);
    }
    else
    {
        *psize = g_Zero.size;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetContentSize
//
//  Synopsis:   Return the width/height of the content
//
//  Arguments:  psize - Pointer to CSize
//
//-----------------------------------------------------------------------------

void
CLayout::GetContentSize(
    CSize * psize,
    BOOL    fActualSize) const
{
    CDispNode * pDispNode = GetElementDispNode();

    Assert(psize);

    if (pDispNode)
    {
        if (pDispNode->IsScroller())
        {
            DYNCAST(CDispScroller, pDispNode)->GetContentSize(psize);
        }
        else
        {
            CRect   rc;

            GetClientRect(&rc);

            psize->cx = rc.Width();
            psize->cy = rc.Height();
        }
    }
    else
    {
        *psize = g_Zero.size;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetPosition
//
//  Synopsis:   Return the top/left of a layout relative to its container
//
//  Arguments:  ppt     - Pointer to CPoint
//              cs    - Coordinate system for returned values
//
//-----------------------------------------------------------------------------

void
CLayout::GetPosition(
    CPoint *            ppt,
    COORDINATE_SYSTEM   cs) const
{
    Assert(ppt);

    if (    _pDispNode
        &&  _pDispNode->GetParentNode())
    {
        *ppt = _pDispNode->GetPosition();

        if (cs != COORDSYS_PARENT)
        {
            _pDispNode->TransformPoint(ppt, COORDSYS_PARENT, cs);
        }
    }
    else if (ElementOwner()->Tag() == ETAG_TR)
    {
        CElement *  pElement = ElementOwner();
        CTreeNode * pNode    = NULL;
        CLayout *   pLayout  = (CLayout *)this;

        ppt->y = pLayout->GetYProposed();
        ppt->x = pLayout->GetXProposed();

        if (cs != COORDSYS_PARENT)
        {
            *ppt = g_Zero.pt;

            Assert(cs == COORDSYS_GLOBAL);

            if (pElement)
            {
                pNode = pElement->GetFirstBranch();

                if (pNode)
                {
                    CElement * pElementZParent = pNode->ZParent();

                    if (pElementZParent)
                    {
                        CLayout  *  pParentLayout = pElementZParent->GetCurNearestLayout();
                        CDispNode * pDispNode     = pParentLayout->GetElementDispNode(pElementZParent);

                        if (pDispNode)
                        {
                            pDispNode->TransformPoint(ppt, COORDSYS_CONTENT, COORDSYS_GLOBAL);
                        }
                    }
                }
            }
        }
        else
        {
            // We need to determine if the layout's parent is RTL. If so, we need to mirror the layout
            // in the same way a disp node is and get the top left.
            if (pElement)
            {
                pNode = pElement->GetFirstBranch()->GetUpdatedParentLayoutNode();
            }

            BOOL fRTL = (pNode && pNode->GetCharFormat()->_fRTL);

            if (fRTL)
            {
                CSize size(0, 0);
                if (pElement)
                {
                    pElement->GetBoundingSize(size);
                }
                ppt->x = - ppt->x - size.cx;
            }

        }
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CLayout::SetPosition
//
//  Synopsis:   Set the top/left of a layout relative to its container
//
//  Arguments:  ppt     - Pointer to CPoint
//
//-----------------------------------------------------------------------------

void
CLayout::SetPosition(
    const CPoint &  pt,
    BOOL            fNotifyAuto)
{
    CPoint  ptOriginal;

    Assert(_pDispNode);

    ptOriginal = _pDispNode->GetPosition();

    _pDispNode->SetPosition(pt);

    if (    fNotifyAuto
        &&  _fPositionSet
        &&  pt != ptOriginal
        &&  (_fAutoBelow || _fContainsRelative)
        &&  !ElementOwner()->IsZParent())
    {
        CSize size(pt.x - ptOriginal.x, pt.y - ptOriginal.y);

        if (_fAutoBelow)
        {
            long    cpStart, cpEnd;

            ElementOwner()->GetFirstAndLastCp(&cpStart, &cpEnd);
            NotifyTranslatedRange(size, cpStart, cpEnd);
        }

        if (_fContainsRelative)
        {
            TranslateRelDispNodes(size);
        }
    }

    // if this layout has a window, tell the view that it will have to
    // reorder windows according to Z order
    if (ElementOwner()->GetHwnd())
    {
        GetView()->SetFlag(CView::VF_DIRTYZORDER);
    }

    _fPositionSet = TRUE;

    if (ElementOwner()->ShouldFireEvents())
    {
// BUGBUG: Queue and fire these after the measuring pass is complete (brendand)
        if (pt.x != ptOriginal.x)
        {
            ElementOwner()->FireOnChanged(DISPID_IHTMLELEMENT_OFFSETLEFT);
            ElementOwner()->FireOnChanged(DISPID_IHTMLELEMENT2_CLIENTLEFT);
        }

        if (pt.y != ptOriginal.y)
        {
            ElementOwner()->FireOnChanged(DISPID_IHTMLELEMENT_OFFSETTOP);
            ElementOwner()->FireOnChanged(DISPID_IHTMLELEMENT2_CLIENTTOP);
        }

    }
}


//+--------------------------------------------------------------------------
//
// Member:      CLayout::HandleTranslatedRange
//
// Synopsis:    Update the position of the current layout if it is a zparent
//              or any relative children if the content before the current
//              layout changes.
//
// Arguments:   size - size by which the current layout/relative children
//              need to be offset by.
//
//---------------------------------------------------------------------------
void
CLayout::HandleTranslatedRange(
    const CSize &   size)
{
    Assert(_fContainsRelative || ElementOwner()->IsZParent());

    if ((   _fContainsRelative
        ||  GetFirstBranch()->GetFancyFormat()->_fAutoPositioned))
    {
        CRequest * pRequest = ElementOwner()->GetRequestPtr();

        if (!pRequest || !pRequest->IsFlagSet(CRequest::RF_POSITION))
        {
            if (_fPositionSet)
            {
                if(ElementOwner()->IsZParent())
                {
                    CPoint pt;

                    GetPosition(&pt);
                    SetPosition(pt + size);
                }
                else
                {
                    Assert(!pRequest);

                    TranslateRelDispNodes(size);
                }
            }
        }
        else if (pRequest && pRequest->IsFlagSet(CRequest::RF_AUTOVALID))
        {
            pRequest->ClearFlag(CRequest::RF_AUTOVALID);
        }
    }
}


//+----------------------------------------------------------------------------
//
//  Method:     CLayout::QueueRequest
//
//  Synopsis:   Add a request to the request queue
//
//  Arguments:  rf       - Request type
//              pElement - Element to queue
//
//-----------------------------------------------------------------------------

CRequest *
CLayout::QueueRequest(
    CRequest::REQUESTFLAGS  rf,
    CElement *              pElement)
{
    Assert(pElement);

    //
    //  It is illegal to queue measuring requests while handling measuring requests,
    //  to queue measuring or positioning requests while handling positioning requests,
    //  to queue any requests while handling adorner requests
    //

    Assert(!TestLock(CElement::ELEMENTLOCK_PROCESSMEASURE)  || rf != CRequest::RF_MEASURE);
    Assert(!TestLock(CElement::ELEMENTLOCK_PROCESSPOSITION) || (    rf != CRequest::RF_MEASURE
                                                                &&  rf != CRequest::RF_POSITION));
    Assert(!TestLock(CElement::ELEMENTLOCK_PROCESSADORNERS));

    CRequests * pRequests     = RequestQueue();
    CRequest *  pRequest      = NULL;
    BOOL        fQueueRequest = TRUE;

    //
    //  If no request queue exists, create one
    //

    if (!pRequests)
    {
        pRequests = new CRequests();

        if (!pRequests ||
            !SUCCEEDED(AddRequestQueue(pRequests)))
        {
            delete pRequests;
            goto Error;
        }
    }

    //
    //  Add a request for the element
    //  If the element does not have a request, create one and add to the queue
    //  If the request is already in our queue, just update its state
    //

    pRequest = pElement->GetRequestPtr();

    if (!pRequest)
    {
        pRequest = new CRequest(rf, pElement);

        if (    !pRequest
            ||  !SUCCEEDED(pElement->SetRequestPtr(pRequest)))
        {
            goto Error;
        }
    }
    else
    {
        if (!pRequest->QueuedOnLayout(this))
        {
            pRequest->AddRef();
        }
        else
        {
            fQueueRequest = FALSE;
        }

        pRequest->SetFlag(rf);
    }

    if (    fQueueRequest
        &&  !SUCCEEDED(pRequests->Append(pRequest)))
        goto Error;

    //
    //  Save the layout responsible for the request type
    //

    pRequest->SetLayout(rf, this);

    //
    //  Post an appropriate layout task
    //

    PostLayoutRequest(rf == CRequest::RF_MEASURE
                            ? LAYOUT_MEASURE
                            : rf == CRequest::RF_POSITION
                                    ? LAYOUT_POSITION
                                    : LAYOUT_ADORNERS);

Cleanup:
    return pRequest;

Error:
    if (pRequest)
    {
        pRequest->DequeueFromLayout(this);
        pRequest->Release();
        pRequest = NULL;
    }
    goto Cleanup;
}


//+----------------------------------------------------------------------------
//
//  Method:     CLayout::FlushRequests
//
//  Synopsis:   Empty the request queue
//
//-----------------------------------------------------------------------------
void
CLayout::FlushRequests()
{
    if (HasRequestQueue())
    {
        CRequests * pRequests = DeleteRequestQueue();
        CRequest ** ppRequest;
        int         cRequests;

        for (ppRequest = &pRequests->Item(0), cRequests = pRequests->Size();
             cRequests;
             ppRequest++, cRequests--)
        {
            (*ppRequest)->DequeueFromLayout(this);
            (*ppRequest)->Release();
        }

        delete pRequests;
    }
}


//+----------------------------------------------------------------------------
//
//  Method:     CLayout::ProcessRequest
//
//  Synopsis:   Process a single request
//
//  Arguments:  pci      - Current CCalcInfo
//              pRequest - Request to process
//                  - or -
//              pElement - CElement whose outstanding request should be processed
//
//-----------------------------------------------------------------------------
#pragma warning(disable:4702)           // Unreachable code (invalidly occurs on the IsFlagSet/GetLayout inlines)

BOOL
CLayout::ProcessRequest(
    CCalcInfo * pci,
    CRequest  * pRequest)
{
    Assert(pci);
    Assert(pci->_grfLayout & (LAYOUT_MEASURE | LAYOUT_POSITION | LAYOUT_ADORNERS));
    Assert(pRequest);

    BOOL    fCompleted = TRUE;

    //
    // BUGBUG (EricVas)
    //
    // Sometimes the element has already left the tree, but the detach of its
    // layout has not been called to dequeue the requests...
    //
    // When Srini fixes this for real, replace this test with an assert that
    // the element in in the tree.
    //
    // Also, beware of elements jumping from tree to tree.
    //
    if (pRequest->GetElement()->IsInMarkup())
    {
        if (    pRequest->IsFlagSet(CRequest::RF_MEASURE)
            &&  pRequest->GetLayout(CRequest::RF_MEASURE) == this)
        {
            if (pci->_grfLayout & LAYOUT_MEASURE)
            {
                CElement::CLock LockRequests(ElementOwner(), CElement::ELEMENTLOCK_PROCESSMEASURE);
                HandleElementMeasureRequest(pci, 
                                            pRequest->GetElement(), 
                                            ElementOwner()->IsEditable(TRUE));
                pRequest->ClearFlag(CRequest::RF_MEASURE);
            }
            else
            {
                fCompleted = FALSE;
            }
        }

        if (    pRequest->IsFlagSet(CRequest::RF_POSITION)
            &&  pRequest->GetLayout(CRequest::RF_POSITION) == this)
        {
            if (pci->_grfLayout & LAYOUT_POSITION)
            {
                CElement::CLock LockRequests(ElementOwner(), CElement::ELEMENTLOCK_PROCESSPOSITION);
                HandlePositionRequest(pRequest->GetElement(),
                                      pRequest->GetAuto(),
                                      pRequest->IsFlagSet(CRequest::RF_AUTOVALID));
                pRequest->ClearFlag(CRequest::RF_POSITION);
            }
            else
            {
                fCompleted = FALSE;
            }
        }

        if (    pRequest->IsFlagSet(CRequest::RF_ADDADORNER)
            &&  pRequest->GetLayout(CRequest::RF_ADDADORNER) == this)
        {
            if (pci->_grfLayout & LAYOUT_ADORNERS)
            {
                CElement::CLock LockRequests(ElementOwner(), CElement::ELEMENTLOCK_PROCESSADORNERS);
                HandleAddAdornerRequest(pRequest->GetAdorner());
                pRequest->ClearFlag(CRequest::RF_ADDADORNER);
            }
            else
            {
                fCompleted = FALSE;
            }
        }
    }

    return fCompleted;
}

#pragma warning(default:4702)           // Unreachable code

void
CLayout::ProcessRequest(
    CElement *  pElement)
{
    if (    pElement
        &&  !pElement->IsPositionStatic()) 
    {
        CRequest * pRequest = pElement->GetRequestPtr();

        if (pRequest)
        {
            CCalcInfo   CI(this);
            CSize       size;

            GetSize(&size);
            CI.SizeToParent((SIZE *)&size);
            CI._grfLayout |= LAYOUT_MEASURE | LAYOUT_POSITION;

            ProcessRequest(&CI, pRequest);

            if (!pRequest->IsFlagSet(CRequest::RF_ADDADORNER))
            {
                CRequests * pRequests = RequestQueue();

                if (    pRequests
                    &&  pRequests->DeleteByValue(pRequest))
                {
                    pRequest->DequeueFromLayout(this);
                    pRequest->Release();
                }

                if (    pRequests
                    &&  !pRequests->Size())
                {
                    DeleteRequestQueue();
                    delete pRequests;
                }
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CLayout::ProcessRequests
//
//  Synopsis:   Process each pending request in the request queue
//
//  Arguments:  pci  - Current CCalcInfo
//              size - Size available to the child element
//
//-----------------------------------------------------------------------------

void
CLayout::ProcessRequests(
    CCalcInfo *     pci,
    const CSize &   size)
{
    Assert(pci);
    Assert(HasRequestQueue());
    Assert(GetView());

    CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_PROCESSREQUESTS);
    CElement::CLock LockRequests(ElementOwner(), pci->_grfLayout & LAYOUT_MEASURE
                                                        ? CElement::ELEMENTLOCK_PROCESSMEASURE
                                                        : pci->_grfLayout & LAYOUT_POSITION
                                                                ? CElement::ELEMENTLOCK_PROCESSPOSITION
                                                                : CElement::ELEMENTLOCK_PROCESSADORNERS);

    CSaveCalcInfo   sci(pci);
    CRequests *     pRequests;
    CRequest **     ppRequest;
    int             cRequests;
    BOOL            fCompleted = TRUE;

    pci->SizeToParent((SIZE *)&size);

    pRequests = RequestQueue();

    if (pRequests)
    {
        cRequests = pRequests->Size();

        if (cRequests)
        {
            for (ppRequest = &pRequests->Item(0);
                cRequests;
                ppRequest++, cRequests--)
            {
                if (ProcessRequest(pci, (*ppRequest)))
                {
                    (*ppRequest)->DequeueFromLayout(this);
                }
                else
                {
                    fCompleted = FALSE;
                }
            }
        }

        if (fCompleted)
        {
            pRequests = DeleteRequestQueue();

            cRequests = pRequests->Size();

            if (cRequests)
            {
                for (ppRequest = &pRequests->Item(0);
                    cRequests;
                    ppRequest++, cRequests--)
                {
                    (*ppRequest)->Release();
                }
            }

            delete pRequests;
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestPoint
//
//  Synopsis:   Determines if the passed CPoint hits the layout and/or one of its
//              contained elements
//
//  Arguments:  ppNodeElement - Location at which to return CTreeNode of hit element
//              grfFlags      - HT_ flags
//
//  Returns:    HTC
//
//----------------------------------------------------------------------------

HTC
CLayout::HitTestPoint(
    const CPoint &  pt,
    CTreeNode **    ppNodeElement,
    DWORD           grfFlags)
{
    Assert(ppNodeElement);
    return HTC_NO;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestRect
//
//  Synopsis:   Determines if the given rect intersects the given site.
//
//  Arguments:  prc rectange to check, in document coordinates
//
//  Returns:    BOOL
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestRect(RECT *prc)
{
// BUGBUG: Should this routine be used any longer? (brendand)
    return FALSE;
}


HRESULT
CLayout::GetChildElementTopLeft(POINT & pt, CElement * pChild)
{
    Assert(0&&"WE should never get here");
    pt.x = pt.y = -1;
    return S_OK;
}

void
CLayout::GetMarginInfo(CParentInfo *ppri,
                       LONG * plLeftMargin, LONG * plTopMargin,
                       LONG * plRightMargin, LONG *plBottomMargin)
{
    CTreeNode * pNodeLayout  = GetFirstBranch();
    const CFancyFormat * pFF = pNodeLayout->GetFancyFormat();
    const CParaFormat *  pPF = pNodeLayout->GetParaFormat();

    Assert(plLeftMargin || plRightMargin || plTopMargin || plBottomMargin);

    if (plTopMargin)
        *plTopMargin = 0;

    if (plBottomMargin)
        *plBottomMargin = 0;

    if (plLeftMargin)
        *plLeftMargin = 0;

    if (plRightMargin)
        *plRightMargin = 0;

    if (!pFF->_fHasMargins)
        return;

    //
    // For block elements, top & bottom margins are treated as
    // before & afterspace, left & right margins are accumulated into the
    // indent's. So, ignore margin's since they are already factored in.
    //
    if (     !ElementOwner()->IsBlockElement()
        ||   !ElementOwner()->IsInlinedElement())
    {
        if (plLeftMargin && !pFF->_cuvMarginLeft.IsNull())
        {
            *plLeftMargin =  pFF->_cuvMarginLeft.XGetPixelValue(
                                            ppri,
                                            ppri->_sizeParent.cx,
                                            pPF->_lFontHeightTwips);
        }
        if (plRightMargin && !pFF->_cuvMarginRight.IsNull())
        {
            *plRightMargin = pFF->_cuvMarginRight.XGetPixelValue(
                                            ppri,
                                            ppri->_sizeParent.cx,
                                            pPF->_lFontHeightTwips);
        }

        if (plTopMargin && !pFF->_cuvMarginTop.IsNull())
        {
            *plTopMargin = pFF->_cuvMarginTop.YGetPixelValue(
                                            ppri,
                                            ppri->_sizeParent.cx,
                                            pPF->_lFontHeightTwips);

        }
        if(plBottomMargin && !pFF->_cuvMarginBottom.IsNull())
        {
            *plBottomMargin = pFF->_cuvMarginBottom.YGetPixelValue(
                                            ppri,
                                            ppri->_sizeParent.cx,
                                            pPF->_lFontHeightTwips);
        }
    }
}


//+------------------------------------------------------------------------
//
//  Method:     SetIsAdorned
//
//  Synopsis:   Mark or clear a layout as adorned
//
//  Arguments:  fAdorned - TRUE/FALSE value
//
//-------------------------------------------------------------------------

VOID
CLayout::SetIsAdorned(BOOL fAdorned)
{
    _fAdorned = fAdorned;
}


//+------------------------------------------------------------------------
//
//  Member:     CLayout::PreDrag
//
//  Synopsis:   Prepares for an OLE drag/drop operation
//
//  Arguments:  dwKeyState  Starting key / button state
//              ppDO        Data object to return
//              ppDS        Drop source to return
//
//-------------------------------------------------------------------------

HRESULT
CLayout::PreDrag(DWORD dwKeyState,
                 IDataObject **ppDO,
                 IDropSource **ppDS)
{
    RRETURN(E_FAIL);
}


//+------------------------------------------------------------------------
//
//  Member:     CLayout::PostDrag
//
//  Synopsis:   Cleans up after an OLE drag/drop operation
//
//  Arguments:  hrDrop      The hr that DoDragDrop came back with
//              dwEffect    The effect of the drag/drop
//
//-------------------------------------------------------------------------

HRESULT
CLayout::PostDrag(HRESULT hrDrop, DWORD dwEffect)
{
    RRETURN(E_FAIL);
}

static HRESULT
CreateDataObject(CDoc * pDoc,
                 IUniformResourceLocator * pUrlToDrag,
                 IDataObject ** ppDataObj)
{
    HRESULT hr;
    IDataObject * pLinkDataObj = NULL;
    CGenDataObject * pDataObj;

    hr = THR(pUrlToDrag->QueryInterface(IID_IDataObject, (void **) &pLinkDataObj));
    if (hr)
        goto Cleanup;

    pDataObj = new CGenDataObject(pDoc);
    if (!pDataObj)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pDataObj->_pLinkDataObj = pLinkDataObj;
    pLinkDataObj = NULL;

    *ppDataObj = pDataObj;

Cleanup:
    ReleaseInterface(pLinkDataObj);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CLayout::DoDrag
//
//  Synopsis:   Start an OLE drag/drop operation
//
//  Arguments:  dwKeyState   Starting key / button state
//              pURLToDrag   Specifies the URL data object if we are
//                           dragging a URL (from <A> or <AREA>).
//
//-------------------------------------------------------------------------

HRESULT
CLayout::DoDrag(DWORD dwKeyState,
                IUniformResourceLocator * pURLToDrag /* = NULL */,
                BOOL fCreateDataObjOnly /* = FALSE */)
{
    HRESULT         hr          = NOERROR;

    LPDATAOBJECT    pDataObj    = NULL;
    LPDROPSOURCE    pDropSource = NULL;
    DWORD           dwEffect, dwEffectAllowed ;
    CElement::CLock Lock(ElementOwner());
    HWND            hwndOverlay = NULL;
    CDoc *          pDoc        = Doc();
#ifndef NO_EDIT
    CParentUndoUnit * pPUU = NULL;
    CDragStartInfo * pDragStartInfo = pDoc->_pDragStartInfo;
    CLayout *       pTopLayout;

    if (!fCreateDataObjOnly)
        pPUU = pDoc->OpenParentUnit( pDoc, IDS_UNDODRAGDROP );
#endif // NO_EDIT

    Assert(ElementOwner()->IsInMarkup());
    Assert(pDoc->GetPrimaryElementClient());
    pTopLayout = pDoc->GetPrimaryElementClient()->GetUpdatedLayout();

    Assert(pTopLayout);

    if (fCreateDataObjOnly || !pDragStartInfo || !pDragStartInfo->_pDataObj)
    {
        Assert(!pDoc->_pDragDropSrcInfo);
        if (pURLToDrag)
        {
            pDoc->_pDragDropSrcInfo = new CDragDropSrcInfo;
            if (!pDoc->_pDragDropSrcInfo)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            pDoc->_pDragDropSrcInfo->_srcType = DRAGDROPSRCTYPE_URL;

            hr = CreateDataObject(pDoc, pURLToDrag, &pDataObj);
            if (!hr)
                hr = THR(CDummyDropSource::Create(dwKeyState, pDoc, &pDropSource));
        }
        else
            hr = THR(PreDrag(dwKeyState, &pDataObj, &pDropSource));
        if (hr)
        {
            if (S_FALSE == hr)
                hr = S_OK;
            goto Cleanup;
        }

        IGNORE_HR(pDoc->SetDataObjectSecurity(pDataObj));

        if (pDragStartInfo)
        {
            pDragStartInfo->_pDataObj = pDataObj;
            pDragStartInfo->_pDropSource = pDropSource;
            pDataObj->AddRef();
            pDropSource->AddRef();
            if (fCreateDataObjOnly)
                goto Cleanup;
        }
    }
    else
    {
        pDataObj = pDragStartInfo->_pDataObj;
        pDropSource = pDragStartInfo->_pDropSource;
        pDataObj->AddRef();
        pDropSource->AddRef();
    }

    _fIsDragDropSrc = TRUE;
    // Setting this makes checking for self-drag easier
    pTopLayout->_fIsDragDropSrc = TRUE;
    pDoc->_fIsDragDropSrc = TRUE;
    pDoc->GetRootDoc()->_fIsDragDropSrc = TRUE;

    // Make sure that no object has capture because OLE will want it
    pDoc->SetMouseCapture(NULL, NULL);

    // Force a synchronous redraw; this is necessary, since
    // the drag-drop feedback is drawn with an XOR pen.
    pDoc->UpdateForm();

    // Throw an overlay window over the current site in order
    // to prevent a move of a control into the same control.

    if (ElementOwner()->IsEditable(TRUE) &&
        pDoc->_pElemCurrent &&
        pDoc->_pElemCurrent->GetHwnd())
    {
        hwndOverlay = pDoc->CreateOverlayWindow(pDoc->_pElemCurrent->GetHwnd());
    }

    if (pDragStartInfo && pDragStartInfo->_dwEffectAllowed != DROPEFFECT_UNINITIALIZED)
        dwEffectAllowed = pDragStartInfo->_dwEffectAllowed;
    else
    {
        if (pURLToDrag)
            dwEffectAllowed = DROPEFFECT_LINK;
        else if (ElementOwner()->IsEditable())
            dwEffectAllowed = DROPEFFECT_COPY | DROPEFFECT_MOVE;
        else // do not allow move if the site cannot be edited
            dwEffectAllowed = DROPEFFECT_COPY;
    }

    hr = THR(DoDragDrop(
            pDataObj,
            pDropSource,
            dwEffectAllowed,
            &dwEffect));

    if (pDragStartInfo)
        pDragStartInfo->_pElementDrag->Fire_ondragend(0, dwEffect);

    // Guard against unexpected drop-effect (e.g. VC5 returns DROPEFFECT_MOVE
    // even when we specify that only DROPEFFECT_COPY is allowed - bug #39911)
    if (DRAGDROP_S_DROP == hr &&
        DROPEFFECT_NONE != dwEffect &&
        !(dwEffect & dwEffectAllowed))
    {
        AssertSz(FALSE, "Unexpected drop effect returned by the drop target");

        if (DROPEFFECT_LINK == dwEffectAllowed)
        {
            dwEffect = DROPEFFECT_LINK;
        }
        else
        {
            Assert(DROPEFFECT_COPY == dwEffectAllowed && DROPEFFECT_MOVE == dwEffect);
            dwEffect = DROPEFFECT_COPY;
        }
    }

    if (hwndOverlay)
    {
        DestroyWindow(hwndOverlay);
    }

    if (    !pURLToDrag
        &&  ElementOwner()->IsInMarkup()
        &&  (   !pDragStartInfo
             ||     pDragStartInfo->_pElementDrag->GetFirstBranch()))
    {
        hr = THR(PostDrag(hr, dwEffect));
        if (hr)
            goto Cleanup;
    }
    else
    {
        // DoDragDrop returns either DRAGDROP_S_DROP (for successful drops)
        // or some code for failure/user-cancel. We don't care, so we just
        // set hr to S_OK
        hr = S_OK;
    }

Cleanup:

    if (!fCreateDataObjOnly)
    {
#ifndef NO_EDIT
        pDoc->CloseParentUnit(pPUU, S_OK);
#endif // NO_EDIT

        if (pDoc->_pDragDropSrcInfo)
        {
            //
            // marka - BUGBUG - SelDragDropSrcInfo is now refcounted.
            // to do - make normal DragDropSrcInfo an object too
            //
            if(DRAGDROPSRCTYPE_SELECTION == pDoc->_pDragDropSrcInfo->_srcType)
            {
                CSelDragDropSrcInfo * pDragInfo = DYNCAST(CSelDragDropSrcInfo, pDoc->_pDragDropSrcInfo);
                pDragInfo->Release();
            }
            else
            {
                delete pDoc->_pDragDropSrcInfo;
            }

            pDoc->_pDragDropSrcInfo = NULL;
        }
    }
    ReleaseInterface(pDataObj);
    ReleaseInterface(pDropSource);

    _fIsDragDropSrc = FALSE;
    pTopLayout->_fIsDragDropSrc = FALSE;
    pDoc->_fIsDragDropSrc = FALSE;
    pDoc->GetRootDoc()->_fIsDragDropSrc = FALSE;
    Assert(fCreateDataObjOnly || !pDoc->_pDragDropSrcInfo);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CLayout::DropHelper
//
//  Synopsis:   Start an OLE drag/drop operation
//
//  Arguments:  ptlScreen   Screen loc of obj.
//              dwAllowed   Allowed list of drop effects
//              pdwEffect   The effect of the drop
//
//  Notes:      For now, this just handles right button dragging, but any
//              other info can be added here later.
//
//-------------------------------------------------------------------------

HRESULT
CLayout::DropHelper(POINTL ptlScreen, DWORD dwAllowed, DWORD *pdwEffect, LPTSTR lptszFileType)
{
    HRESULT hr = S_OK;

    if (Doc()->_fRightBtnDrag)
    {
        int     iSelection;

        *pdwEffect = DROPEFFECT_NONE;

        if (!Doc()->_fSlowClick)
        {
            hr = THR(Doc()->ShowDragContextMenu(ptlScreen, dwAllowed, &iSelection, lptszFileType));
            if (S_OK == hr)
            {
                *pdwEffect = iSelection;
            }
            else if (S_FALSE == hr)
            {
                hr = S_OK; // no need to propagate S_FALSE
            }
        }
    }

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::PageBreak
//
//  Synopsis:   Impose page break constraints
//
//  Returns:    S_OK because there are no pagebreak constraints on CLayout.
//
//-----------------------------------------------------------------------------

HRESULT
CLayout::PageBreak(LONG yStart, LONG yIdeal, CStackPageBreaks * paryValues)
{
    // No constraints.
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::ContainsPageBreak
//
//  Synopsis:   Checks whether layout has page-break-before or after.
//
//  Arguments:  yTop    - top position to look for
//              yBottom - bottom position to look for
//              pyBreak - OUT: pagebreak position found
//
//  Note:       All y-coordinates are relative to top of the layout.
//
//  Returns:    S_OK if a pagebreak was found - else S_FALSE.
//
//-----------------------------------------------------------------------------

HRESULT
CLayout::ContainsPageBreak(long yTop, long yBottom, long * pyBreak, BOOL * pfPageBreakAfterAtTopLevel)
{
    HRESULT hr = S_FALSE;

    Assert(pyBreak && yTop >= 0);

    //
    // Pagebreak before.
    //

    if (HasPageBreakBefore() && yTop <= 0)
    {
        *pyBreak = 0;
        hr = S_OK;
        goto Cleanup;
    }

    //
    // Pagebreak after.
    //

    if (HasPageBreakAfter())
    {
        CDispNode * pDispNode;
        SIZE        sizeDispNode;

        //
        // Find out if the bottom of this layout is above yBottom.
        //

        pDispNode = GetElementDispNode();
        if (!pDispNode)
            goto Cleanup;

        pDispNode->GetSize(&sizeDispNode);
        if (sizeDispNode.cy <= yBottom)
        {
            *pyBreak = sizeDispNode.cy;
            hr = S_OK;

            if (pfPageBreakAfterAtTopLevel)
                *pfPageBreakAfterAtTopLevel = TRUE;

            goto Cleanup;
        }
    }

Cleanup:

    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::HasPageBreakBefore / After
//
//  Synopsis:   Checks whether layout has page-break-before or after.
//
//-----------------------------------------------------------------------------

BOOL
CLayout::HasPageBreakBefore()
{
    return ElementOwner() ? ElementOwner()->HasPageBreakBefore() : FALSE;
}

BOOL
CLayout::HasPageBreakAfter()
{
    return ElementOwner() ? ElementOwner()->HasPageBreakAfter() : FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DragEnter
//
//  Synopsis:   Setup for possible drop
//
//----------------------------------------------------------------------------

HRESULT
CLayout::DragEnter(
        IDataObject *pDataObj,
        DWORD grfKeyState,
        POINTL ptlScreen,
        DWORD *pdwEffect)
{
    HRESULT hr;

    Doc()->_fDragFeedbackVis = FALSE;

    if (!ElementOwner()->IsEditable())
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    hr = THR(ParseDragData(pDataObj));
    if ( hr == S_FALSE )
    {
        //
        // S_FALSE is returned by ParseData - if the DragDrop cannot accept the HTML
        // you want to paste.
        //
        *pdwEffect = DROPEFFECT_NONE;     
        return S_OK;
    }
    if (hr)
        goto Cleanup;

    hr = THR(InitDragInfo(pDataObj, ptlScreen));
    if (hr)
        goto Cleanup;

    hr = THR(DragOver(grfKeyState, ptlScreen, pdwEffect));

    if (hr)
    {
        hr = THR(DragLeave());
    }

Cleanup:

    RRETURN1 (hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLayout::ParseDragData
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CLayout::ParseDragData(IDataObject *pDataObj)
{
    TCHAR   szText[MAX_PATH];


    // Start with flags set to default values.

    Doc()->_fOKEmbed = FALSE;
    Doc()->_fOKLink = FALSE;
    Doc()->_fFromCtrlPalette = FALSE;


    // Now set flags based on content of data object.

    if (OK(GetcfCLSIDFmt(pDataObj, szText)))
    {
        //
        //  Special combination of flags means copy from control box
        //
        Doc()->_fFromCtrlPalette = 1;
    }
    else
    {
        OBJECTDESCRIPTOR objdesc;

        // The explicit check for S_OK is required here because
        // OleQueryXXXFromData() returns other success codes.
        Doc()->_fOKEmbed = OleQueryCreateFromData(pDataObj) == S_OK;
        Doc()->_fOKLink = OleQueryLinkFromData(pDataObj) == S_OK;

        // BUGBUG: (anandra) Try to get the object descriptor immediately
        // to see if we can create a site for this thing.  This will
        // eventually change when we want to support dragging of html
        // files into the form

        if (!OK(GetObjectDescriptor(pDataObj, &objdesc)))
        {
            Doc()->_fOKEmbed = FALSE;
            Doc()->_fOKLink = FALSE;
        }
    }

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DragOver
//
//  Synopsis:   Determine whether this would be a move, copy, link
//              or null operation and manage UI feedback
//
//----------------------------------------------------------------------------

HRESULT
CLayout::DragOver(DWORD grfKeyState, POINTL ptlScreen, LPDWORD pdwEffect)
{
    HRESULT hr      = S_OK;

    CDoc* pDoc = Doc();
    Assert(pdwEffect != NULL);

    grfKeyState &= MK_CONTROL | MK_SHIFT;

    if (!ElementOwner()->IsEditable())
    {
        *pdwEffect = DROPEFFECT_NONE;               // No Drop into design mode
    }
    else if (pDoc->_fFromCtrlPalette)
    {
        *pdwEffect &= DROPEFFECT_COPY;              // Drag from Control Palette
    }
    else if (grfKeyState == (MK_CONTROL | MK_SHIFT) &&
            pDoc->_fOKLink &&
            (*pdwEffect & DROPEFFECT_LINK))
    {
        *pdwEffect = DROPEFFECT_LINK;               // Control-Shift equals create link
    }
    else if (grfKeyState == MK_CONTROL &&
            (*pdwEffect & DROPEFFECT_COPY))
    {
        *pdwEffect = DROPEFFECT_COPY;               // Control key = copy.
    }
    else if (*pdwEffect & DROPEFFECT_MOVE)
    {
        *pdwEffect = DROPEFFECT_MOVE;               // Default to move
    }
    else if (*pdwEffect & DROPEFFECT_COPY)
    {
        *pdwEffect = DROPEFFECT_COPY;               // If can't move, default to copy
    }
    else if ((pDoc->_fOKLink) &&
            (*pdwEffect & DROPEFFECT_LINK))
    {
        *pdwEffect = DROPEFFECT_LINK;               // If can't copy, default to link
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }


    //
    // Drag & Drop with pointers in the same flow layout will do a copy not a move
    // ( as a delete across flow layouts is not allowed).
    //
    if ( *pdwEffect & DROPEFFECT_MOVE && 
         pDoc->_pDragDropSrcInfo && 
         DRAGDROPSRCTYPE_SELECTION == pDoc->_pDragDropSrcInfo->_srcType)
    {
        CSelDragDropSrcInfo * pDragInfo = DYNCAST(CSelDragDropSrcInfo, pDoc->_pDragDropSrcInfo);
        if ( ! pDragInfo->IsInSameFlow() )
        {
            *pdwEffect = DROPEFFECT_COPY;
        }
    }

    //
    // Draw or erase feedback as appropriate.
    //    
    if (*pdwEffect == DROPEFFECT_NONE)
    {
        // Erase previous feedback.
        DragHide();
    }
    else
    {
        hr = THR(UpdateDragFeedback( ptlScreen ));
        if (pDoc->_fSlowClick)
        {
            *pdwEffect = DROPEFFECT_NONE ;
        }
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DragLeave
//
//  Synopsis:   Remove any user feedback
//
//----------------------------------------------------------------------------

HRESULT
CLayout::DragLeave()
{
    DragHide();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DragHide
//
//  Synopsis:   Remove any user feedback
//
//----------------------------------------------------------------------------

void
CLayout::DragHide()
{
    if (Doc()->_fDragFeedbackVis)
    {
        DrawDragFeedback();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::OnSetCursor, public
//
//  Synopsis:   Sets the cursor to the appropriate one for what part of the
//              site it's over.
//
//  Arguments:  [pt] -- Physical location of the cursor in pixels.
//
//  Returns:    TRUE if processed
//
//----------------------------------------------------------------------------

BOOL
CLayout::OnSetCursor(CMessage *pMessage)
{
    HTC htc = pMessage->htc;
    BOOL fDesignMode = ElementOwner()->IsEditable(TRUE);

    // Don't show re-size cursors in browse mode

    if ((!fDesignMode && htc >= HTC_GRPTOPBORDER) ||
        ( fDesignMode && ElementOwner()->TestClassFlag(CElement::ELEMENTDESC_NOSELECT))
       )
    {
        htc = HTC_NONCLIENT;
    }

    LPCTSTR idc = Doc()->GetCursorForHTC( pMessage->htc );

    if (!fDesignMode)
    {
        ElementOwner()->SetCursorStyle(idc);
    }
    else
    {
        SetCursorIDC(idc);
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawZeroBorder
//
//  Synopsis:   Draw the "Zero Grey Border" around the input.
//
//----------------------------------------------------------------------------

void
CLayout::DrawZeroBorder(CFormDrawInfo *pDI)
{
    Assert(GetUpdatedParentLayout()->ElementOwner()->IsEditable() && GetView()->IsFlagSet(CView::VF_ZEROBORDER));

    CColorValue cv        = ElementOwner()->GetFirstBranch()->GetCascadedbackgroundColor();
    CDispNode * pDispNode = GetElementDispNode(ElementOwner());
    pDI->_hdc = NULL;       // Whack the DC so we force client clipping.
    HDC         hdc       = pDI->GetDC(TRUE);
    COLORREF    cr;

    if ( cv._dwValue == 0 )
    {
        cr = 0x00ffffff & (~(cv.GetColorRef()));
    }
    else
    {
        cr = ZERO_GREY_COLOR ;
    }

#if DBG == 1
    if ( IsTagEnabled(tagShowZeroGreyBorder))
    {
        cr = RGB(0xFF,0x00,0x00);
    }
#endif

    if ( ! pDispNode->HasBorder() )
    {
        HBRUSH hbr, hbrOld;
        hbr = ::CreateSolidBrush(cr );
        hbrOld = (HBRUSH) ::SelectObject( hdc, hbr );

        RECT rcContent;
        GetClientRect( & rcContent, COORDSYS_CONTENT );

        PatBltRect( hdc, (RECT*) &rcContent , 1, PATCOPY );
        SelectBrush( hdc, hbrOld );
        DeleteBrush( hbr );
    }
    else
    {
        HPEN hPen, hPenOld;
        RECT rcBorder;
        pDispNode->GetBorderWidths( &rcBorder );

        hPen = CreatePen( PS_SOLID, 1, cr );
        hPenOld = SelectPen( hdc, hPen );
        RECT rcContent;

        GetRect( & rcContent, COORDSYS_CONTENT );

        int left, top, bottom, right;
        left = rcContent.left;
        top = rcContent.top;
        right = rcContent.right;
        bottom = rcContent.bottom;

        if ( rcBorder.left == 0 )
        {
            MoveToEx(  hdc, left, top , NULL );
            LineTo( hdc, left , bottom );
        }
        if ( rcBorder.right == 0 )
        {
            MoveToEx(  hdc, (right-left), top , NULL );
            LineTo( hdc, (right-left), bottom );
        }
        if ( rcBorder.top == 0 )
        {
            MoveToEx(  hdc, left , top, NULL );
            LineTo( hdc, (right-left), top );
        }
        if ( rcBorder.bottom == 0 )
        {
            MoveToEx(  hdc, left , bottom, NULL );
            LineTo( hdc, (right-left) , bottom );
        }
        SelectPen( hdc, hPenOld );
        DeletePen( hPen );
    }
}

void
DrawTextSelectionForRect(HDC hdc, CRect *prc, CRect *prcClip, BOOL fSwapColor)
{
    static short bBrushBits [8] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55} ;
    HBITMAP hbm;
    HBRUSH hBrush, hBrushOld;
    COLORREF crOldBk, crOldFg;
    CPoint ptOrg;

    // Text selection feedback for sites is painting every other pixel
    // with the color being used for painting the background for selections.

    // Select the color we want to paint evey other pixel with
    crOldBk = SetBkColor (hdc, GetSysColor (
                                            fSwapColor ? COLOR_HIGHLIGHTTEXT : COLOR_HIGHLIGHT));

    hbm = CreateBitmap (8, 8, 1, 1, (LPBYTE)bBrushBits);
    hBrush = CreatePatternBrush (hbm);

    ptOrg = prc->TopLeft();
    if (ptOrg.x != 0)
    {
        ptOrg.x -= ptOrg.x % 8;
    }
    if (ptOrg.y != 0)
    {
        ptOrg.y -= ptOrg.y % 8;
    }

    SetBrushOrgEx( hdc, ptOrg.x, ptOrg.y, NULL );

    hBrushOld = (HBRUSH)SelectObject (hdc, hBrush);

    // Now, for monochrome bitmap brushes, 0: foreground, 1:background.
    // We've set the background color to the selection color, set the fg
    // color to black, so that when we OR, every other screen pixel will
    // retain its color, and the remaining with have the selection color
    // OR'd into them.
    crOldFg = SetTextColor (hdc, RGB(0,0,0));

    PatBlt (hdc, prc->left, prc->top,
            prc->right - prc->left,
            prc->bottom - prc->top,
            DST_PAT_OR);            

    // Now, set the fg color to white so that when we AND, every other screen
    // pixel still retains its color, while the remaining have just the
    // selection in them. This gives us the effect of transparency.
    SetTextColor (hdc, RGB(0xff,0xff,0xff));

    PatBlt (hdc, prc->left, prc->top,
            prc->right - prc->left,
            prc->bottom - prc->top,
            DST_PAT_AND);   
            
    SelectObject (hdc, hBrushOld);
    DeleteObject (hBrush);
    DeleteObject (hbm);

    SetTextColor (hdc, crOldFg);
    SetBkColor   (hdc, crOldBk);
}

//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawTextSelectionForSite
//
//  Synopsis:   Draw the text selection feed back for the site
//
//----------------------------------------------------------------------------
void
CLayout::DrawTextSelectionForSite(CFormDrawInfo *pDI)
{
    if (_fTextSelected)
    {
        CRect rcContent;
        GetClippedRect( & rcContent, COORDSYS_CONTENT );
        DrawTextSelectionForRect(pDI->GetDC(), & rcContent ,& pDI->_rcClip , _fSwapColor);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::Draw
//
//  Synopsis:   Draw the site and its children to the screen.
//
//----------------------------------------------------------------------------

void
CLayout::Draw(CFormDrawInfo *pDI, CDispNode *)
{
    return;
}


void
CLayout::DrawBackground(
    CFormDrawInfo *     pDI,
    BACKGROUNDINFO *    pbginfo,
    RECT *              prcDraw)
{
    CRect       rcBackground;
    SIZE        sizeImg;
    CDoc    *   pDoc      = Doc();
    BOOL        fPrintDoc = pDoc->IsPrintDoc();
    HDC         hdc;
    COLORREF    crBack    = pbginfo->crBack;
    CImgCtx *   pImgCtx   = pbginfo->pImgCtx;
    ULONG       ulState;

    if (pImgCtx)
    {
        ulState = pImgCtx->GetState(FALSE, &sizeImg);
    }
    else
    {
        ulState = 0; 
        sizeImg = g_Zero.size;
    }

    Assert(pDoc);

    if (!(ulState & IMGLOAD_COMPLETE))
    {
        pImgCtx = NULL;
    }

    if (!prcDraw)
    {
        GetClientRect(&rcBackground, CLIENTRECT_BACKGROUND);
        pDI->TransformToDeviceCoordinates(&rcBackground);
    }
    else
    {
        rcBackground = *prcDraw;
    }

    IntersectRect(&rcBackground, &pDI->_rcClip, &rcBackground);

    hdc = pDI->GetDC();

    // N.B. (johnv) We only blt the background if we do not have an
    // image, or if the image rect is not identical to the clip rectangle.
    // We can also blt four times (around the image) if this turns out
    // to be faster.
    if (    crBack != COLORREF_NONE
        &&  (   !pImgCtx
            ||  !(ulState & IMGTRANS_OPAQUE)
            ||  !EqualRect(&rcBackground, &pbginfo->rcImg)))
    {
        PatBltBrush(hdc, &rcBackground, PATCOPY, crBack);
    }

    if (pImgCtx)
    {
        // For Printdoc, convert pixels to printer units

        if (fPrintDoc)
        {
            sizeImg.cx = pDI->DocPixelsFromWindowX(sizeImg.cx);
            sizeImg.cy = pDI->DocPixelsFromWindowY(sizeImg.cy);
        }

        if (sizeImg.cx == 0 || sizeImg.cy == 0)
            return;

        if (crBack == COLORREF_NONE)
            crBack = pbginfo->crTrans;

        pImgCtx->Tile(hdc,
                    &pbginfo->ptBackOrg,
                    &pbginfo->rcImg,
                    fPrintDoc
                        ? &sizeImg
                        : NULL,
                    crBack,
                    pDoc->GetImgAnimState(pbginfo->lImgCtxCookie),
                    pDI->DrawImageFlags());
    }

    WHEN_DBG(CDebugPaint::PausePaint(tagPaintWait));
}


HRESULT
CLayout::GetElementsInZOrder(CPtrAry<CElement *> *paryElements,
                           CElement            *pElementThis,
                           RECT                *prcDraw,
                           HRGN                 hrgn,
                           BOOL                 fIncludeNotVisible)
{
    AssertSz(FALSE, "This GetElementsInZOrder should not be called.");
    return(E_NOTIMPL);
}


HRESULT
CLayout::GetDC(LPRECT prc, DWORD dwFlags, HDC *phDC)
{
    CDoc    * pDoc = Doc();

    Assert(pDoc);

    //
    // Any changes here should be copied to CFilter::GetDC
    //

    Assert((dwFlags & 0xFF00) == 0);

    dwFlags |=  (   (pDoc->_bufferDepth & OFFSCR_BPP) << 16)
                |   (pDoc->_cSurface   ? OFFSCR_SURFACE   : 0)
                |   (pDoc->_c3DSurface ? OFFSCR_3DSURFACE : 0);

    return pDoc->GetDC(prc, dwFlags, phDC);
}


HRESULT
CLayout::ReleaseDC(HDC hdc)
{
    return Doc()->ReleaseDC(hdc);
}


//+-----------------------------------------------------------------------
//
//  Function:   Invalidate
//
//  Synopsis:   Invalidate the passed rectangle or region
//
//------------------------------------------------------------------------
void
CLayout::Invalidate(
    const RECT&         rc,
    COORDINATE_SYSTEM   cs)
{
    if (    Doc()->_state >= OS_INPLACE
        &&  GetFirstBranch()
        &&  _pDispNode)
    {
        _pDispNode->Invalidate((const CRect&) rc, cs);
    }
}


void
CLayout::Invalidate(
    LPCRECT prc,
    int     cRects,
    LPCRECT prcClip)
{
    if (    Doc()->_state >= OS_INPLACE
        &&  GetFirstBranch()
        &&  _pDispNode)
    {
        if (!prc)
        {
            CSize   size;
            _pDispNode->GetSize(&size);
            CRect rc(size);

            _pDispNode->Invalidate(rc, COORDSYS_CONTAINER);
        }
        else
        {
            Assert( !cRects
                ||  prc);
            for (int i=0; i < cRects; i++, prc++)
            {
                _pDispNode->Invalidate((CRect &)*prc, COORDSYS_CONTENT);
            }
        }
    }
}

void
CLayout::Invalidate(
    HRGN    hrgn)
{
    if (    Doc()->_state >= OS_INPLACE
        &&  GetFirstBranch()
        &&  _pDispNode)
    {
        _pDispNode->Invalidate(hrgn, COORDSYS_CONTENT);
    }
}


extern void CalcBgImgRect(CTreeNode * pNode, CFormDrawInfo * pDI,
                          const SIZE * psizeObj, const SIZE * psizeImg,
                          CPoint *pptBackOrig, RECT * prcBackClip);

//+------------------------------------------------------------------------
//
//  Member:     GetBackgroundInfo
//
//  Synopsis:   Fills out a background info for which has details on how
//              to display a background color &| background image.
//
//-------------------------------------------------------------------------

BOOL
CLayout::GetBackgroundInfo(
    CFormDrawInfo *     pDI,
    BACKGROUNDINFO *    pbginfo,
    BOOL                fAll,
    BOOL                fRightToLeft)
{
    Assert(pDI || !fAll);

          CPoint            ptBackOrig;
          CTreeNode    *    pNode = GetFirstBranch();
    const CFancyFormat *    pFF   = pNode->GetFancyFormat();
          CColorValue       cv    = (CColorValue)(pFF->_ccvBackColor);

    pbginfo->pImgCtx       = ElementOwner()->GetBgImgCtx();
    pbginfo->lImgCtxCookie = pFF->_lImgCtxCookie;
    pbginfo->fFixed        = (  pbginfo->pImgCtx
                            &&  pFF->_fBgFixed);

    pbginfo->crBack  = cv.IsDefined()
                            ? cv.GetColorRef()
                            : COLORREF_NONE;
    pbginfo->crTrans = COLORREF_NONE;

    if (    fAll
        &&  pbginfo->pImgCtx)
    {
        CSize sizeImg, sizeClient;

        pbginfo->pImgCtx->GetState(FALSE, &sizeImg);

        // client size, for the purposes of background image positioning,
        // is the greater of our content size or our client rect
        CRect rcClient;
        GetClientRect(&rcClient, CLIENTRECT_BACKGROUND);

        if (!pbginfo->fFixed)
        {
            GetContentSize(&sizeClient, FALSE);
            sizeClient.Max(rcClient.Size());
        }
        else
        {
            sizeClient = rcClient.Size();
        }

        pbginfo->xScroll = 0;
        pbginfo->yScroll = 0;

        CalcBgImgRect(pNode, pDI, &sizeClient, &sizeImg, &ptBackOrig, &pbginfo->rcImg);
        OffsetRect(&pbginfo->rcImg, pDI->_rc.left, pDI->_rc.top);

        // Tiling does not know how to handle right to left images.
        // Set the pptBackOrig->x in relation to rcBounds.Width().
        pbginfo->ptBackOrg.x = ptBackOrig.x;
        if (fRightToLeft)
            pbginfo->ptBackOrg.x -= pDI->_rc.Width();
        pbginfo->ptBackOrg.y = ptBackOrig.y;

        IntersectRect(&pbginfo->rcImg, pDI->ClipRect(), &pbginfo->rcImg);
    }

    return TRUE;
}


//
// Scrolling
//

//+------------------------------------------------------------------------
//
//  Member:     Attach/DetachScrollbarController
//
//  Synopsis:   Manage association between this CLayout and the CScrollbarController
//
//-------------------------------------------------------------------------

void
CLayout::AttachScrollbarController(
    CDispNode * pDispNode,
    CMessage *  pMessage)
{
    CScrollbarController::StartScrollbarController(
        this,
        DYNCAST(CDispScroller, pDispNode),
        Doc(),
        g_sizeScrollbar.cx,
        pMessage);
}

void
CLayout::DetachScrollbarController(
    CDispNode * pDispNode)
{
    CScrollbarController *  pSBC = TLS(pSBC);

    if (    pSBC
        &&  pSBC->GetLayout() == this)
    {
        CScrollbarController::StopScrollbarController();
    }
}


//+------------------------------------------------------------------------
//
//  Member:     ScrollElementIntoView
//
//  Synopsis:   Scroll the element into view
//
//-------------------------------------------------------------------------

HRESULT
CLayout::ScrollElementIntoView( CElement *  pElement,
                                SCROLLPIN   spVert,
                                SCROLLPIN   spHorz,
                                BOOL fScrollBits)
{

    //
    //  BUGBUG:
    //  This code should NOT test for CFlowLayout, using CElement::GetBoundingRect should suffice. Unfortunately,
    //  deep underneath this funtion (specifically in CDisplay::RegionFromElement) that behaves differently when
    //  called from a "scroll into view" routine. CFlowLayout::ScrollRangeIntoView can and does pass the correct
    //  flags such that everything works right - but CElement::GetBoundingRect cannot and does not so the rectangle
    //  it gets differs slightly thus affecting scroll into view. Blah!
    //
    //  Eventually, CDisplay::RegionFromElement should not have such odd dependencies or they should be formalized.
    //  Until then, this dual branch needs to exist. (brendand)
    //

    if (IsFlowLayout())
    {
        long    cpMin;
        long    cpMost;

        Assert(ElementOwner()->IsInMarkup());

        if (!pElement)
        {
            pElement = ElementOwner();
        }

        if (pElement != ElementOwner())
        {
            if (!pElement->IsAbsolute())
            {
                cpMin  = max(GetContentFirstCp(), pElement->GetFirstCp());
                cpMost = min(GetContentLastCp(),  pElement->GetLastCp());
            }
            else
            {
                // since this layout is absolutely positioned, we don't want
                // to use the CPs, since these position its location-in-source.
                // instead it is just as simple to get the rect, and scroll
                // that into view directly.
                CRect rc;

                Assert(pElement->GetUpdatedLayout() && " youre about to crash");
                pElement->GetUpdatedLayout()->GetRect(&rc, COORDSYS_PARENT);

                ScrollRectIntoView(rc, spVert, spHorz, fScrollBits);
                return S_OK;
            }
        }
        else
        {
            cpMin  =
            cpMost = -1;
        }

        RRETURN1(ScrollRangeIntoView(cpMin, cpMost, spVert, spHorz, fScrollBits), S_FALSE);
    }

    else
    {
        CRect   rc;

        Assert(ElementOwner()->IsInMarkup());

        if (!pElement)
        {
            pElement = ElementOwner();
        }

        pElement->GetBoundingRect(&rc);
        ScrollRectIntoView(rc, spVert, spHorz, fScrollBits);
        return S_OK;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::ScrollRangeIntoView
//
//  Synopsis:   Scroll the given range into view
//
//  Arguments:  cpStart     First cp of range
//              cpEnd       Last cp of range
//              spVert      vertical scroll pin option
//              spHorz      horizontal scroll pin option
//
//----------------------------------------------------------------------------

HRESULT
CLayout::ScrollRangeIntoView( long        cpMin,
                              long        cpMost,
                              SCROLLPIN   spVert,
                              SCROLLPIN   spHorz,
                              BOOL        fScrollBits)
{
    CSize   size;

    ElementOwner()->SendNotification(NTYPE_ELEMENT_ENSURERECALC);

    GetSize(&size);

    ScrollRectIntoView(CRect(size), spVert, spHorz, fScrollBits);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::ScrollRectIntoView
//
//  Synopsis:   Scroll the given rectangle (in content coordinates) into view.
//
//  Arguments:  rc          rect in content coordinates
//              spVert      vertical scroll pin option
//              spHorz      horizontal scroll pin option
//
//----------------------------------------------------------------------------

BOOL
CLayout::ScrollRectIntoView( const CRect & rc,
                             SCROLLPIN     spVert,
                             SCROLLPIN     spHorz,
                             BOOL          fScrollBits)
{
    Assert(spVert != SP_MAX && spHorz != SP_MAX);
    CPaintCaret hc( ElementOwner()->Doc()->_pCaret ); // Hide the caret for scrolling
    BOOL fScrolledIntoView = FALSE;
    
    if (_pDispNode)
    {
        if (OpenView(FALSE, TRUE))
        {

            fScrolledIntoView = _pDispNode->ScrollRectIntoView(
                                    rc,
                                    (CRect::SCROLLPIN) spVert,
                                    (CRect::SCROLLPIN) spHorz,
                                    fScrollBits);

            EndDeferred();
        }
    }
    else
    {
        CLayout *   pLayout = GetUpdatedParentLayout();

        if (pLayout)
        {
            fScrolledIntoView = pLayout->ScrollElementIntoView(ElementOwner(), spVert, spHorz, fScrollBits);
        }
    }

    return fScrolledIntoView;
}


HRESULT BUGCALL
CLayout::HandleMessage(CMessage  * pMessage)
{
    HRESULT     hr          = S_FALSE;
    CDispNode * pDispNode   = GetElementDispNode();
    BOOL        fIsScroller = (pDispNode && pDispNode->IsScroller());
    CDoc*       pDoc        = Doc();
    BOOL        fInBrowse   = !pDoc->_fDesignMode;

    if (!ElementOwner()->CanHandleMessage())
    {
        // return into ElementOwner()'s HandleMessage
        goto Cleanup;
    }

    //
    //  Handle scrollbar messages
    //

    if (    fIsScroller
        &&  (   (pMessage->htc == HTC_HSCROLLBAR && pMessage->pNodeHit->Element() == ElementOwner())
            ||  (pMessage->htc == HTC_VSCROLLBAR && pMessage->pNodeHit->Element() == ElementOwner()))
        &&  (   (  pMessage->message >= WM_MOUSEFIRST
#ifndef WIN16
                &&  pMessage->message != WM_MOUSEWHEEL
#endif
                &&  pMessage->message <= WM_MOUSELAST)
            ||  pMessage->message == WM_SETCURSOR))
    {
        // BUGBUG (MohanB) how to get the child here?
        hr = HandleScrollbarMessage(pMessage, ElementOwner());
        if (hr != S_FALSE)
            goto Cleanup;
    }

    switch (pMessage->message)
    {
    case WM_CONTEXTMENU:
        if (!pDoc->_pInPlace->_fBubbleInsideOut)
        {
            hr = THR(ElementOwner()->OnContextMenu(
                    (short) LOWORD(pMessage->lParam),
                    (short) HIWORD(pMessage->lParam),
                    CONTEXT_MENU_DEFAULT));
        }
        else
            hr = S_OK;
        break;

#ifndef NO_MENU
    case WM_MENUSELECT:
        hr = THR(ElementOwner()->OnMenuSelect(
                GET_WM_MENUSELECT_CMD(pMessage->wParam, pMessage->lParam),
                GET_WM_MENUSELECT_FLAGS(pMessage->wParam, pMessage->lParam),
                GET_WM_MENUSELECT_HMENU(pMessage->wParam, pMessage->lParam)));
        break;

    case WM_INITMENUPOPUP:
        hr = THR(ElementOwner()->OnInitMenuPopup(
                (HMENU) pMessage->wParam,
                (int) LOWORD(pMessage->lParam),
                (BOOL) HIWORD(pMessage->lParam)));
        break;
#endif // NO_MENU

    case WM_KEYDOWN:
        // BUGBUG (MohanB) how to get the child here?
        hr = THR(HandleKeyDown(pMessage, ElementOwner()));
        break;


#ifndef WIN16
    case WM_MOUSEWHEEL:
        // (jenlc) should bobble up the message to CBodyElement
        //
        hr = S_FALSE;
        break;
#endif // ndef WIN16

    case WM_HSCROLL:
        if (fIsScroller)
        {
            hr = THR(OnScroll(
                    0,
                    LOWORD(pMessage->wParam),
                    HIWORD(pMessage->wParam),
                    FALSE));
        }
        break;

    case WM_VSCROLL:
        if (fIsScroller)
        {
            hr = THR(OnScroll(
                    1,
                    LOWORD(pMessage->wParam),
                    HIWORD(pMessage->wParam),
                    FALSE));
        }
        break;

    case WM_CHAR:
        if (pMessage->wParam == VK_RETURN && !pDoc->_fDesignMode)
        {
            hr = THR(pDoc->ActivateDefaultButton(pMessage));
            if (S_OK != hr && ElementOwner()->GetParentForm())
            {
                MessageBeep(0);
            }
            break;
        }

        if (    fInBrowse
            &&  pMessage->wParam == VK_SPACE
            &&  fIsScroller)
        {
            OnScroll(
                1,
                pMessage->dwKeyState & MK_SHIFT
                        ? SB_PAGEUP
                        : SB_PAGEDOWN,
                0,
                FALSE,
                (pMessage->wParam&0x4000000)
                        ? 50  // BUGBUG: For now we are using the mouse delay - should use Api to find system key repeat rate set in control panel.
                        : MAX_SCROLLTIME);
            hr = S_OK;
            break;
        }
        break;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     PrepareMessage
//
//  Synopsis:   Prepare the CMessage for the layout (e.g., ensure the
//              content point exists)
//
//  Arguments:  pMessage  - CMessage to prepare
//              pDispNode - CDispNode to use (defaults to layout display node)
//
//--------------------------------------------------------------------------

void
CLayout::PrepareMessage(
    CMessage *  pMessage,
    CDispNode * pDispNode)
{
    if (!pMessage->IsContentPointValid())
    {
        if (!pDispNode)
            pDispNode = GetElementDispNode();

        if (pDispNode)
        {
            pMessage->ptContent = pMessage->pt;
            pMessage->pDispNode = pDispNode;
            pDispNode->TransformPoint(&pMessage->ptContent,
                                      COORDSYS_GLOBAL,
                                      COORDSYS_CONTENT);
        }
    }
}


ExternTag(tagMsoCommandTarget);

void
CLayout::AdjustSizeForBorder(SIZE * pSize, CDocInfo * pdci, BOOL fInflate)
{
    CBorderInfo bInfo;

    if (ElementOwner()->GetBorderInfo(pdci, &bInfo, FALSE))
    {
        int iXWidths = pdci->DocPixelsFromWindowX( bInfo.aiWidths[BORDER_RIGHT]
                        + bInfo.aiWidths[BORDER_LEFT], FALSE);
        int iYWidths = pdci->DocPixelsFromWindowY( bInfo.aiWidths[BORDER_TOP]
                        + bInfo.aiWidths[BORDER_BOTTOM], FALSE);

        pSize->cx += fInflate ? iXWidths : -iXWidths;
        pSize->cy += fInflate ? iYWidths : -iYWidths;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   PercentSize
//              PercentWidth
//              PercentHeight
//
//  Synopsis:   Handy helpers to check for percentage dimensions
//
//----------------------------------------------------------------------------

BOOL
CLayout::PercentSize()
{
    const CFancyFormat *pFF=GetFirstBranch()->GetFancyFormat();
    return (pFF->_fHeightPercent || pFF->_fWidthPercent );
}

BOOL
CLayout::PercentWidth()
{
    return (GetFirstBranch()->GetFancyFormat()->_fWidthPercent);
}

BOOL
CLayout::PercentHeight()
{
    return (GetFirstBranch()->GetFancyFormat()->_fHeightPercent );
}


//+---------------------------------------------------------------------------
//
//  Member:     CSite::Move
//
//  Synopsis:   Move and/or resize the control
//
//  Arguments:  [rc]      -- New position
//              [dwFlags] -- Specifies flags
//
//  Notes:      prcpixels is in parent content relative coords of the site.
//              Move will take into account to offset it appropriately
//              with it's region parent.
//
//----------------------------------------------------------------------------

HRESULT
CLayout::Move(RECT *prcpixels, DWORD dwFlags)
{
    HRESULT         hr = S_OK;
    RECT            rcWindow;
    CDocInfo        DCI(ElementOwner());
    CBorderInfo     borderinfo;
    int             xWidth2, yWidth2;
    BOOL            fChanged = FALSE;
    DWORD           dwNotificationFlags;
    CRect           rcContainer;


    // Only call RequestLayout if we're just moving the object, to minimize
    // repainting.

    dwNotificationFlags = (dwFlags & SITEMOVE_NORESIZE)
                          ? ELEMCHNG_SITEPOSITION | ELEMCHNG_CLEARCACHES
                          : ELEMCHNG_SIZECHANGED  | ELEMCHNG_CLEARCACHES;

#ifndef NO_EDIT
    {
        CUndoPropChangeNotificationPlaceHolder
                notfholder( !(dwFlags & SITEMOVE_NOFIREEVENT) &&
                            Doc()->LoadStatus() == LOADSTATUS_DONE,
                            ElementOwner(), DISPID_UNKNOWN, dwNotificationFlags );
#endif // NO_EDIT

    Assert(prcpixels);

    // we need to see if we are in a right to left situation
    // if we are we will need to set our left at a correct distance from our
    // parent's left
    CTreeNode * pParentNode = GetFirstBranch()->GetUpdatedParentLayoutNode();
    BOOL fRTL = (pParentNode != NULL && pParentNode->GetCharFormat()->_fRTL);

    CLayout* pParentLayout = GetFirstBranch()->GetUpdatedParentLayout();

    // We might not get a pParentLayout at this point: e.g. an OBJECT tag in
    // the HEAD (recall <HTML> has no layout) (bug #70791), in which
    // case we play it safe and init rcContainer to a 0 pixel rect
    // (it shouldn't be used in any meaningful way anyways).

    // if we have scroll bars, we need to take of the size of the scrolls.
    if ( pParentLayout )
    {
        if(pParentLayout->GetElementDispNode() &&
           pParentLayout->GetElementDispNode()->IsScroller())
        {
            pParentLayout->GetClientRect(&rcContainer);
        }
        else
        {
            Assert(pParentNode);

            pParentNode->Element()->GetBoundingRect(&rcContainer);
        }
    }
    else
    {
        rcContainer.top = rcContainer.left = 0;
        rcContainer.bottom = rcContainer.right = 0;        
    }

    if (fRTL)
    {
        prcpixels->left += rcContainer.Width();
        prcpixels->right += rcContainer.Width();
    }
    
    //
    // Account for any zooming.
    //

    DCI.WindowFromDocPixels ( (POINT *)&rcWindow.left,
        *(POINT *)&prcpixels->left );
    DCI.WindowFromDocPixels ( (POINT *)&rcWindow.right,
        *(POINT *)&prcpixels->right );

    //
    // Finally rcWindow is in parent site relative document coords.
    //

    if (ElementOwner()->TestClassFlag(CElement::ELEMENTDESC_EXBORDRINMOV))
    {
        ElementOwner()->GetBorderInfo( &DCI, &borderinfo, FALSE );

        xWidth2 = borderinfo.aiWidths[BORDER_RIGHT] + borderinfo.aiWidths[BORDER_LEFT];
        yWidth2 = borderinfo.aiWidths[BORDER_TOP] + borderinfo.aiWidths[BORDER_BOTTOM];
    }
    else
    {
        xWidth2 = 0;
        yWidth2 = 0;
    }


    if (!(dwFlags & SITEMOVE_NORESIZE))
    {
        // If the ELEMENTDESC flag is set

        // (ferhane)
        //  Pass in 1 when the size is not defined as a percent. If the size is defined as a 
        //  percentage of the container, then the CUnitValue::SetFloatValueKeepUnits needs the
        //  container size to be passed for the proper percentage calculation.
        //
        if (!PercentWidth())
        {
            rcContainer.right = 1;
            rcContainer.left = 0;
        }

        if (!PercentHeight())
        {
            rcContainer.bottom = 1;
            rcContainer.top = 0;
        }

        // Set Attributes
        hr = THR ( ElementOwner()->SetDim ( STDPROPID_XOBJ_HEIGHT,
                            (float)(rcWindow.bottom - rcWindow.top - yWidth2),
                            CUnitValue::UNIT_PIXELS,
                            rcContainer.bottom - rcContainer.top,
                            NULL,
                            FALSE,
                            &fChanged ) );
        if ( hr )
            goto Cleanup;

        hr = THR ( ElementOwner()->SetDim ( STDPROPID_XOBJ_WIDTH,
                            (float)(rcWindow.right - rcWindow.left - xWidth2),
                            CUnitValue::UNIT_PIXELS,
                            rcContainer.right - rcContainer.left,
                            NULL,
                            FALSE,
                            &fChanged ) );
        if ( hr )
            goto Cleanup;

        // Set In-line style
        hr = THR ( ElementOwner()->SetDim ( STDPROPID_XOBJ_HEIGHT,
                            (float)(rcWindow.bottom - rcWindow.top - yWidth2),
                            CUnitValue::UNIT_PIXELS,
                            rcContainer.bottom - rcContainer.top,
                            NULL,
                            TRUE,
                            &fChanged ) );
        if ( hr )
            goto Cleanup;

        hr = THR ( ElementOwner()->SetDim ( STDPROPID_XOBJ_WIDTH,
                            (float)(rcWindow.right - rcWindow.left - xWidth2),
                            CUnitValue::UNIT_PIXELS,
                            rcContainer.right - rcContainer.left,
                            NULL,
                            TRUE,
                            &fChanged ) );
        if ( hr )
            goto Cleanup;
    }

//BUGBUG: (FerhanE)
//          We will make the TOP and LEFT behavior for percentages the same with the 
//          behavior for width and height above in the 5.x tree. 5.0 is not changed for 
//          these attributes.
//
    if (!(dwFlags & SITEMOVE_RESIZEONLY))
    {
        hr = THR ( ElementOwner()->SetDim ( STDPROPID_XOBJ_TOP,
                            (float)rcWindow.top,
                            CUnitValue::UNIT_PIXELS,
                            1,
                            NULL,
                            TRUE,
                            &fChanged ) );
        if ( hr )
            goto Cleanup;

        hr = THR ( ElementOwner()->SetDim ( STDPROPID_XOBJ_LEFT,
                            (float)rcWindow.left,
                            CUnitValue::UNIT_PIXELS,
                            1,
                            NULL,
                            TRUE,
                            &fChanged ) );
        if ( hr )
            goto Cleanup;
    }

    // Only fire off a change notification if something changed
    if (fChanged && !(dwFlags & SITEMOVE_NOFIREEVENT))
    {
        ElementOwner()->OnPropertyChange( DISPID_UNKNOWN, dwNotificationFlags );
    }

Cleanup:

#ifndef NO_EDIT
        notfholder.SetHR( fChanged ? hr : S_FALSE );
    }
#endif // NO_EDIT

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::TransformPoint
//
//  Synopsis:   Transform a point from the source coordinate system to the
//              destination coordinate system
//
//  Arguments:  ppt             point to transform
//              source          source coordinate system
//              destination     destination coordinate system
//
//----------------------------------------------------------------------------

void
CLayout::TransformPoint(
    CPoint *            ppt,
    COORDINATE_SYSTEM   source,
    COORDINATE_SYSTEM   destination,
    CDispNode *         pDispNode) const
{
    if(!pDispNode)
        pDispNode = GetElementDispNode();

    if(pDispNode)
    {
        pDispNode->TransformPoint(ppt, source, destination);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::TransformRect
//
//  Synopsis:   Transform a rect from the source coordinate system to the
//              destination coordinate system with optional clipping.
//
//  Arguments:  prc             rect to transform
//              source          source coordinate system
//              destination     destination coordinate system
//              fClip           TRUE to clip the rectangle
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CLayout::TransformRect(
    RECT *              prc,
    COORDINATE_SYSTEM   source,
    COORDINATE_SYSTEM   destination,
    BOOL                fClip,
    CDispNode *         pDispNode) const
{
    if (!pDispNode)
        pDispNode = _pDispNode;

    if (pDispNode)
    {
        pDispNode->TransformRect((CRect *)prc, source, destination, fClip);
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     SetSurfaceFlags
//
//  Synopsis:   Set/clear the surface flags
//
//----------------------------------------------------------------------------

void
CLayout::SetSurfaceFlags(BOOL fSurface, BOOL f3DSurface, BOOL fDontFilter /* = FALSE */)
{
    //
    // f3DSurface is illegal without fSurface
    //
    Assert(!f3DSurface || fSurface);
    fSurface |= f3DSurface;

    if (    ElementOwner()->HasFilterPtr()
        &&  !fDontFilter)
    {
        DWORD dwFlags = 0;
        if (fSurface)
            dwFlags |= FILTER_STATUS_SURFACE;
        if (f3DSurface)
            dwFlags |= FILTER_STATUS_3DSURFACE;
        ElementOwner()->GetFilterPtr()->FilteredSetStatusBits(FILTER_STATUS_SURFACE | FILTER_STATUS_3DSURFACE, dwFlags);
    }
    else
    {
        // Normalize the BOOLs
        fSurface = !!fSurface;
        f3DSurface = !!f3DSurface;

        if ((unsigned)fSurface != _fSurface)
        {
            if (fSurface)
                Doc()->_cSurface++;
            else
                Doc()->_cSurface--;
            _fSurface = (unsigned)fSurface;
        }
        if ((unsigned)f3DSurface != _f3DSurface)
        {
            if (f3DSurface)
                Doc()->_c3DSurface++;
            else
                Doc()->_c3DSurface--;
            _f3DSurface = (unsigned)f3DSurface;
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     SetSiteTextSelection
//
//  Synopsis:   Set's a sites text selection status
//
//  Arguments:  none
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------
void
CLayout::SetSiteTextSelection (BOOL fSelected, BOOL fSwap)
{
    _fTextSelected = fSelected ;
    _fSwapColor = fSwap;
}


//+---------------------------------------------------------------------------
//
//  Member:     HandleKeyDown
//
//  Synopsis:   Helper for keydown handling
//
//  Arguments:  [pMessage]  -- message
//              [pChild]    -- pointer to child when bubbling allowed
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CLayout::HandleKeyDown(CMessage * pMessage, CElement * pElemChild)
{
    BOOL    fRunMode = !ElementOwner()->IsEditable(TRUE);
    BOOL    fAlt     = pMessage->dwKeyState & FALT;
    BOOL    fCtrl    = pMessage->dwKeyState & FCONTROL;
    HRESULT hr       = S_FALSE;

    if (    fRunMode
        &&  !fAlt)
    {
        CDispNode * pDispNode   = GetElementDispNode();
        BOOL        fIsScroller = pDispNode && pDispNode->IsScroller();
        BOOL        fDirect     = pElemChild == NULL;

        if (fIsScroller)
        {
            if (    !fDirect
                ||  SUCCEEDED(hr))
            {
                hr = HandleScrollbarMessage(pMessage, pElemChild);
            }
        }
    }

    if (    hr == S_FALSE
        &&  !fAlt
        &&  !fCtrl)
    {
        switch (pMessage->wParam)
        {
        case VK_RETURN:
            break;

        case VK_ESCAPE:
            if (fRunMode)
            {
                hr = THR(Doc()->ActivateCancelButton(pMessage));
            }
            break;
        }
    }
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::GetOwner
//
//  Synopsis:   Return the logical owner of the CDispClient interface - This
//              is always either a CElement or NULL
//
//  Arguments:  ppv - Location at which to return the owner
//
//----------------------------------------------------------------------------

void
CLayout::GetOwner(
    CDispNode * pDispNode,
    void **     ppv)
{
    Assert(pDispNode);
    Assert(pDispNode == GetElementDispNode());
    Assert(ppv);
    *ppv = ElementOwner();
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClient
//
//  Synopsis:   Draw display leaf nodes
//
//  Arguments:  prcBounds       bounding rect of display leaf node
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//----------------------------------------------------------------------------

void
CLayout::DrawClient(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    Assert(pClientData);

    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;

    {
        // we set draw surface information separately for Draw() and
        // the stuff below, because the Draw method of some subclasses
        // (like CFlowLayout) puts pDI into a special device coordinate
        // mode
        CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
        Draw(pDI, pDispNode);
    }

    {
        // see comment above
        CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);

        //
        // BUGBUG - marka - this will eventually be managed as an effect
        // on the display tree - with a callback for "DrawEffect"
        // For now selected borders wont' paint right
        //
        DrawTextSelectionForSite(pDI);
          
    }
  
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClientBackground
//
//  Synopsis:   Draw the background
//
//  Arguments:  prcBounds       bounding rect of display leaf
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CLayout::DrawClientBackground(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          pClientData,
    DWORD           dwFlags)
{
    Assert(pClientData);

    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
    BACKGROUNDINFO  bi;

    pDI->SetDeviceCoordinateMode();
    GetBackgroundInfo(pDI, &bi, TRUE, pDispNode->IsRightToLeft());
    pDI->TransformToDeviceCoordinates(&bi.ptBackOrg);
        
    if (bi.crBack != COLORREF_NONE || bi.pImgCtx)
        DrawBackground(pDI, &bi, (RECT *)&pDI->_rc);

    if ( GetView()->IsFlagSet(CView::VF_ZEROBORDER) ) 
    {
        CLayout* pParentLayout = GetUpdatedParentLayout();
        if ( pParentLayout && pParentLayout->ElementOwner()->IsEditable() )
        {
            // see comment above
            CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
        
            DrawZeroBorder(pDI);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClientBorder
//
//  Synopsis:   Draw the border
//
//  Arguments:  prcBounds       bounding rect of display leaf
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CLayout::DrawClientBorder(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          pClientData,
    DWORD           dwFlags)
{
    Assert(pClientData);

    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
    CBorderInfo     bi;

    ElementOwner()->GetBorderInfo(pDI, &bi, TRUE);

    bi.aiWidths[BORDER_TOP]    = pDI->DocPixelsFromWindowY(bi.aiWidths[BORDER_TOP], FALSE);
    bi.aiWidths[BORDER_RIGHT]  = pDI->DocPixelsFromWindowX(bi.aiWidths[BORDER_RIGHT], FALSE);
    bi.aiWidths[BORDER_BOTTOM] = pDI->DocPixelsFromWindowY(bi.aiWidths[BORDER_BOTTOM], FALSE);
    bi.aiWidths[BORDER_LEFT]   = pDI->DocPixelsFromWindowX(bi.aiWidths[BORDER_LEFT], FALSE);

    ::DrawBorder(pDI, (RECT *)prcBounds, &bi);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClientScrollbar
//
//  Synopsis:   Draw horizontal/vertical scrollbar
//
//  Arguments:  iDirection      0 for horizontal scrollbar, 1 for vertical
//              prcBounds       bounding rect of the scrollbar
//              prcRedraw       rect to be redrawn
//              contentSize     size of content
//              containerSize   size of container that displays the content
//              scrollAmount    current scroll position
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//----------------------------------------------------------------------------

void
CLayout::DrawClientScrollbar(
    int            iDirection,
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    long           contentSize,
    long           containerSize,
    long           scrollAmount,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    CFormDrawInfo * pDI;
    CFormDrawInfo   DI;

#ifdef  IE5_ZOOM

    long            wNumerXOrig = 1;
    long            wNumerYOrig = 1;
    long            wDenomOrig = 1;
    BOOL            fRestoreScale = FALSE;

#endif  // IE5_ZOOM

    if (!pClientData)
    {
        DI.Init(this);
        DI._hdc = NULL;
        pDI = &DI;
    }
    else
    {
        pDI = (CFormDrawInfo *)pClientData;
    }

#ifdef  IE5_ZOOM

    if (Tag() == ETAG_BODY && pDI->IsZoomed())
    {
        // Body scroll bars should not be scaled
        wNumerXOrig = pDI->GetNumerX();
        wNumerYOrig = pDI->GetNumerY();
        wDenomOrig  = pDI->GetDenom();
        pDI->zoom(1, 1, 1);
        fRestoreScale = TRUE;
    }

#endif  // IE5_ZOOM

    CSetDrawSurface         sds(pDI, prcBounds, prcRedraw, pDispSurface);
    pDI->SetDeviceCoordinateMode();
    
    HDC                     hdc  = pDI->GetDC(TRUE);
    CScrollbarController *  pSBC = TLS(pSBC);
    CScrollbarParams        params;
    CRect                   rcHimetricBounds;
    ThreeDColors            colors;
    
    Assert(pSBC != NULL);

    params._pColors = &colors;
    params._buttonWidth = ((const CRect*)prcBounds)->Size(1-iDirection);
    params._fFlat       = Doc()->_dwFlagsHostInfo & DOCHOSTUIFLAG_FLAT_SCROLLBAR;
    params._fForceDisabled = ! ElementOwner()->IsEnabled();
#ifdef UNIX // Used for Motif scrollbar
    params._bDirection = iDirection;
#endif

    CScrollbar::Draw(
        iDirection,
        pDI->_rc,
        pDI->_rcClip,
        contentSize,
        containerSize,
        scrollAmount,
        (pSBC->GetLayout() == this
            ? pSBC->GetPartPressed()
            : CScrollbar::SB_NONE),
        hdc,
        params,
        pDI,
        dwFlags);

#ifdef  IE5_ZOOM

    if (fRestoreScale)
        pDI->zoom(wNumerXOrig, wNumerYOrig, wDenomOrig);

#endif  // IE5_ZOOM
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClientScrollbarFiller
//
//  Synopsis:   Draw dead region between scrollbars
//
//  Arguments:  prcBounds       bounding rect of the dead region
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//----------------------------------------------------------------------------

void
CLayout::DrawClientScrollbarFiller(
    const RECT *   prcBounds,
    const RECT *   prcRedraw,
    CDispSurface * pDispSurface,
    CDispNode *    pDispNode,
    void *         pClientData,
    DWORD          dwFlags)
{
    HDC hdc;

    if (SUCCEEDED(pDispSurface->GetDC(&hdc, *prcBounds, *prcRedraw, FALSE)))
    {
        HBRUSH  hbr;

        hbr = GetCachedBrush(::GetSysColor(COLOR_3DFACE));
        ::FillRect(hdc, prcRedraw, hbr);
        ReleaseCachedBrush(hbr);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestScrollbar
//
//  Synopsis:   Process a "hit" on a scrollbar
//
//  Arguments:  iDirection      0 for horizontal scrollbar, 1 for vertical
//              pptHit          hit test point
//              pDispNode       pointer to display node
//              pClientData     client-specified data value for hit testing pass
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestScrollbar(
    int            iDirection,
    const POINT *  pptHit,
    CDispNode *    pDispNode,
    void *         pClientData)
{
    CHitTestInfo *  phti;

    Assert(pClientData);

    phti = (CHitTestInfo *)pClientData;

    if (phti->_grfFlags & HT_IGNORESCROLL)
        return FALSE;

    phti->_htc          = (iDirection == 0) ? HTC_HSCROLLBAR : HTC_VSCROLLBAR;
    phti->_pNodeElement = GetFirstBranch();
    phti->_ptContent    = *pptHit;
    phti->_pDispNode    = pDispNode;

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestScrollbarFiller
//
//  Synopsis:   Process a "hit" on a scrollbar filler
//
//  Arguments:  pptHit          hit test point
//              pDispNode       pointer to display node
//              pClientData     client-specified data value for hit testing pass
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestScrollbarFiller(
    const POINT *  pptHit,
    CDispNode *    pDispNode,
    void *         pClientData)
{
    CHitTestInfo *  phti;

    Assert(pClientData);

    phti = (CHitTestInfo *)pClientData;

    phti->_htc          = HTC_YES;
    phti->_pNodeElement = ElementContent()->GetFirstBranch();
    phti->_ptContent    = *pptHit;
    phti->_pDispNode    = pDispNode;

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestContent
//
//  Synopsis:   Determine if the given display leaf node contains the hit point.
//
//  Arguments:  pptHit          hit test point
//              pDispNode       pointer to display node
//              pClientData     client-specified data value for hit testing pass
//
//  Returns:    TRUE if the display leaf node contains the point
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestContent(
    const POINT *   pptHit,
    CDispNode *     pDispNode,
    void *          pClientData)
{
    Assert(pptHit);
    Assert(pDispNode);
    Assert(pClientData);
    POINT ptNodeHit = *pptHit;


    CHitTestInfo *  phti = (CHitTestInfo *)pClientData;

    CPeerHolder *   pPeerHolder = ElementOwner()->GetRenderPeerHolder();

    if (pPeerHolder &&
        pPeerHolder->TestRenderFlags(BEHAVIORRENDERINFO_HITTESTING))
    {
        //
        // delegate hit testing to peer
        //

        HRESULT hr;
        BOOL    fHit;

        // Adjust for RTL. The hdc for peers is adjusted to origin top/left as well.
        if(pDispNode->IsRightToLeft())
        {
            CSize size;
            pDispNode->GetSize(&size);
            ptNodeHit.x += size.cx;
        }

        // (treat hr error as no hit)
        hr = THR(pPeerHolder->HitTestPoint(&ptNodeHit, &fHit));
        if (hr || !fHit)
            goto Cleanup;   // done, no hit
    }

    phti->_htc          = HTC_YES;
    phti->_pNodeElement = ElementContent()->GetFirstBranch();
    phti->_ptContent    = ptNodeHit;
    phti->_pDispNode    = pDispNode;

    phti->_phtr->_fWantArrow = TRUE;

Cleanup:
    return (phti->_htc != HTC_NO);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestFuzzy
//
//  Synopsis:   Determine if the given display leaf node contains the hit point.
//
//  Arguments:  pptHitInParentCoords    hit test point in parent coordinates
//              pDispNode               pointer to display node
//              pClientData             client-specified data for hit testing
//
//  Returns:    TRUE if the display leaf node contains the point
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestFuzzy(
    const POINT *   pptHitInParentCoords,
    CDispNode *     pDispNode,
    void *          pClientData)
{
    Assert(pptHitInParentCoords);
    Assert(pDispNode);
    Assert(pClientData);

    //
    // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
    //
    // BUGBUG remove extra clause from if - and assert below when removing fathit
    //

    if (!DoFuzzyHitTest() || !pDispNode->IsFatHitTest() )
        return FALSE;

    Assert( !pDispNode->IsFatHitTest() || 
                ( pDispNode->IsFatHitTest() && 
                  ElementOwner()->_etag == ETAG_TABLE &&
                  DYNCAST(CTableLayout , this)->GetFatHitTest() ));
            
    CHitTestInfo *  phti = (CHitTestInfo *)pClientData;

    phti->_htc          = HTC_YES;
    phti->_pNodeElement = ElementContent()->GetFirstBranch();
    phti->_phtr->_fWantArrow = TRUE;

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HitTestBorder
//
//  Synopsis:   Hit test the border for this layout.
//
//  Arguments:  pptHit          point to hit test
//              pDispNode       display node
//              pClientData     client data
//
//  Returns:    TRUE if the given point hits this node's border.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CLayout::HitTestBorder(
        const POINT *pptHit,
        CDispNode *pDispNode,
        void *pClientData)
{
    Assert(pptHit);
    Assert(pDispNode);
    Assert(pClientData);

    CHitTestInfo *  phti = (CHitTestInfo *)pClientData;

    phti->_htc          = HTC_YES;
    phti->_pNodeElement = GetFirstBranch();
    phti->_phtr->_fWantArrow = TRUE;
    phti->_ptContent    = *pptHit;
    phti->_pDispNode    = pDispNode;

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::ProcessDisplayTreeTraversal
//
//  Synopsis:   Process results of display tree traversal.
//
//  Arguments:  pClientData     pointer to data defined by client
//
//  Returns:    TRUE to continue traversal
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CLayout::ProcessDisplayTreeTraversal(void *pClientData)
{
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::GetZOrderForSelf
//
//  Synopsis:   Return Z order for this container.
//
//  Returns:    Z order for this container.
//
//----------------------------------------------------------------------------

LONG
CLayout::GetZOrderForSelf()
{
    Assert(!GetFirstBranch()->IsPositionStatic());

    return GetFirstBranch()->GetCascadedzIndex();
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::GetZOrderForChild
//
//  Synopsis:   Return the z order for the display item with the given cookie.
//
//  Arguments:  cookie - Client-specified data value for display item
//
//  Returns:    Z order for the item.
//
//----------------------------------------------------------------------------

LONG
CLayout::GetZOrderForChild(
    void *  cookie)
{
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::CompareZOrder
//
//  Synopsis:   Compare the z-order of two display nodes
//
//  Arguments:  pDispNode1 - Display node owned by this display client
//              pDispNode2 - Display node to compare against
//
//  Returns:    Greater than zero if pDispNode1 is greater
//              Less than zero if pDispNode1 is less
//              Zero if they are equal
//
//----------------------------------------------------------------------------

LONG
CLayout::CompareZOrder(
    CDispNode * pDispNode1,
    CDispNode * pDispNode2)
{
    Assert(pDispNode1);
    Assert(pDispNode2);
    Assert(pDispNode1 == _pDispNode);

    CElement *  pElement1 = ElementOwner();
    CElement *  pElement2 = ::GetDispNodeElement(pDispNode2);

    //
    //  Compare element z-order
    //  If the same element is associated with both display nodes,
    //  then the second display node is for an adorner (which always come
    //  on top of the element)
    //

    return pElement1 != pElement2
                ? pElement1->CompareZOrder(pElement2)
                : -1;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::HandleViewChange
//
//  Synopsis:   Respond to changes of this layout's in-view status.
//
//  Arguments:  flags           flags containing state transition info
//              prcClient       client rect in global coordinates
//              prcClip         clip rect in global coordinates
//              pDispNode       node which moved
//
//----------------------------------------------------------------------------

void
CLayout::HandleViewChange(
     DWORD          flags,
     const RECT *   prcClient,
     const RECT *   prcClip,
     CDispNode *    pDispNode)
{
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLayout::NotifyScrollEvent
//
//  Synopsis:   A scroll has occured in the display and now we can do
//              something with this information, like fire the script
//              event
//
//  Arugments:  prcScroll        - Rectangle scrolled
//              psizeScrollDelta - Amount scrolled
//
//----------------------------------------------------------------------------

void
CLayout::NotifyScrollEvent(
    RECT *  prcScroll,
    SIZE *  psizeScrollDelta)
{
    ElementOwner()->Fire_onscroll();

    Doc()->DeferSetCursor();

    if( Doc()->_pCaret )
        Doc()->_pCaret->UpdateCaret( FALSE, FALSE );
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetPeerLayersInfo
//
//  Synopsis:   Return peer rendering layers
//
//-----------------------------------------------------------------------------

DWORD
CLayout::GetPeerLayersInfo()
{
    CPeerHolder *   pPeerHolder = ElementOwner()->GetRenderPeerHolder();

    return pPeerHolder ? pPeerHolder->RenderFlags() : 0;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetClientLayersInfo
//
//  Synopsis:   Return client rendering layers
//
//-----------------------------------------------------------------------------

DWORD
CLayout::GetClientLayersInfo(CDispNode *pDispNodeFor)
{
    if (_pDispNode != pDispNodeFor)     // if draw request is for nodes other then primary
        return 0;                       // no layers

    return GetPeerLayersInfo();
}


//+----------------------------------------------------------------------------
//
//  Member:     CLayout::DrawClientLayers
//
//  Synopsis:   Give a peer a chance to render
//
//-----------------------------------------------------------------------------

void
CLayout::DrawClientLayers(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    CPeerHolder *   pPeerHolder = ElementOwner()->GetRenderPeerHolder();
    CFormDrawInfo * pDI         = (CFormDrawInfo *)pClientData;

    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);

    Assert(pPeerHolder && pPeerHolder->TestRenderFlags(dwFlags));
    Assert(pDI);

    pPeerHolder->Draw(pDI, dwFlags);
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CLayout::DumpDebugInfo
//
//  Synopsis:   Dump debugging information for the given display node.
//
//  Arguments:  hFile           file handle to dump into
//              level           recursive tree level
//              childNumber     number of this child within its parent
//              pDispNode       pointer to display node
//              cookie          cookie value (only if present)
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CLayout::DumpDebugInfo(
        HANDLE hFile,
        long level,
        long childNumber,
        CDispNode *pDispNode,
        void *cookie)
{
    if (!pDispNode->IsOwned())
    {
        WriteString(hFile, _T("<br>\r\n<font class=background>content</font><br>\r\n"));
    }
    else
    {
        WriteHelp(
            hFile,
            _T("<<br>\r\n<<font class=tag>&lt;<0s>&gt;<</font><<br>\r\n"),
                ElementOwner()->TagName());
    }
}
#endif


//+----------------------------------------------------------------------------
//
//  Member:     GetElementDispNode
//
//  Synopsis:   Return the display node for the pElement
//
//              There are up to two display nodes associated with a layout:
//              The display node that directly represents the layout to its
//              parent and the display node that establishes the container
//              coordinate system for the layout (the primary display node).
//
//              The first of these is always kept in _pDispNode while the second
//              will be different when a filter is active. Parents that need
//              the display node to anchor into the display tree should never
//              request the primary display node (that is, fPrimary should be
//              FALSE for these uses). However, the layout itself should always
//              request the primary display node when accessing its own display
//              node.
//
//  Arguments:  pElement   - CElement whose display node is to obtained
//              fForParent - If TRUE (the default), return the display node a parent
//                           inserts into the tree. Otherwise, return the primary
//                           display node.
//                           NOTE: This only makes a difference with layouts that
//                                 have a filter.
//
//  Returns:    Pointer to the layout CDispNode if one exists, NULL otherwise
//
//-----------------------------------------------------------------------------
CDispNode *
CLayout::GetElementDispNode(
    CElement *  pElement) const
{
    Assert( !pElement
        ||  pElement == ElementOwner());

    return _pDispNode;
}


//+----------------------------------------------------------------------------
//
//  Member:     SetElementDispNode
//
//  Synopsis:   Set the display node for an element
//              NOTE: This is only supported for elements with layouts or
//                    those that are relatively positioned
//
//-----------------------------------------------------------------------------
void
CLayout::SetElementDispNode(
    CElement *  pElement,
    CDispNode * pDispNode)
{
    Assert( !pElement
        ||  pElement == ElementOwner());

    _pDispNode = pDispNode;

    if (pElement && pElement->HasFilterPtr())
        pElement->GetFilterPtr()->EnsureFilterState();
}


//+----------------------------------------------------------------------------
//
//  Member:     GetFirstContentDispNode
//
//  Synopsis:   Return the first content node
//
//              Only container-type display nodes have child content nodes.
//              This routine will return the first, unowned content node in the
//              flow layer under a container display or NULL.
//
//  Arguments:  pDispNode - Parent CDispNode (defaults to layout display node)
//
//  Returns:    Pointer to flow CDispNode if one exists, NULL otherwise
//
//-----------------------------------------------------------------------------
CDispItemPlus *
CLayout::GetFirstContentDispNode(
    CDispNode * pDispNode) const
{
    if (!pDispNode)
        pDispNode = GetElementDispNode();

    pDispNode = (   pDispNode
                &&  pDispNode->IsContainer()
                    ? DYNCAST(CDispContainer, pDispNode)->GetFirstChildNodeInLayer(DISPNODELAYER_FLOW)
                    : NULL);

    while ( pDispNode
        &&  pDispNode->IsOwned())
    {
        pDispNode = pDispNode->GetNextSiblingNode(TRUE);
    }
    Assert( !pDispNode
        ||  (   pDispNode->GetNodeType() == DISPNODETYPE_ITEMPLUS
            &&  !pDispNode->IsOwned()));

    return pDispNode
                ? DYNCAST(CDispItemPlus, pDispNode)
                : NULL;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetLayerParentDispNode
//
//  Synopsis:   Return the display node that parents a particular layer
//
//  Returns:    Pointer to layer parent CDispNode if it exists, NULL otherwise
//
//-----------------------------------------------------------------------------
CDispNode *
CLayout::GetLayerParentDispNode(
    DISPNODELAYER   layer) const
{
    CDispNode * pDispNode = GetElementDispNode();

    return (pDispNode->IsContainer()
                ? pDispNode
                : NULL);
}


//+----------------------------------------------------------------------------
//
//  Member:     GetDispNodeInfo
//
//  Synopsis:   Retrieve values useful for determining what type of display
//              node to create
//
//  Arguments:  pdni     - Pointer to CDispNodeInfo to fill
//              pdci     - Current CDocInfo (only required when fBorders == TRUE)
//              fBorders - If TRUE, retrieve border information
//
//-----------------------------------------------------------------------------

void
CLayout::GetDispNodeInfo(
    CDispNodeInfo * pdni,
    CDocInfo *      pdci,
    BOOL            fBorders) const
{
    CElement *              pElement  = ElementOwner();
    CTreeNode *             pTreeNode = pElement->GetFirstBranch();
    const CFancyFormat *    pFF       = pTreeNode->GetFancyFormat();
    BACKGROUNDINFO          bi;
    HWND                    hwnd;

    //
    //  Get general information
    //

    pdni->_etag  = pElement->Tag();

    pdni->_layer =  pdni->_etag == ETAG_BODY
                ||  pdni->_etag == ETAG_FRAMESET
                ||  pdni->_etag == ETAG_FRAME
                ||  (stylePosition)pFF->_bPositionType == stylePositionstatic
                ||  (stylePosition)pFF->_bPositionType == stylePositionNotSet
                            ? DISPNODELAYER_FLOW
                            : pFF->_lZIndex >= 0
                                    ? DISPNODELAYER_POSITIVEZ
                                    : DISPNODELAYER_NEGATIVEZ;

    //
    //  Determine if insets are required
    //

    if (TestLayoutDescFlag(LAYOUTDESC_TABLECELL))
    {
        htmlCellVAlign  fVAlign;

        fVAlign = (htmlCellVAlign)pTreeNode->GetParaFormat()->_bTableVAlignment;

        pdni->_fHasInset = (    fVAlign != htmlCellVAlignNotSet
                            &&  fVAlign != htmlCellVAlignTop);
    }
    else
    {
        pdni->_fHasInset = TestLayoutDescFlag(LAYOUTDESC_HASINSETS);
    }

    //
    //  Determine background information
    //

    const_cast<CLayout *>(this)->GetBackgroundInfo(NULL, &bi, FALSE);

    pdni->_fHasBackground      = (bi.crBack != COLORREF_NONE || bi.pImgCtx) ||
                                 GetView()->IsFlagSet(CView::VF_ZEROBORDER) ; // we always call DrawClientBackground when ZEROBORDER is on

    pdni->_fHasFixedBackground = (!!bi.pImgCtx && bi.fFixed);


    if (pdni->_etag == ETAG_IMAGE) 
    {
        pdni->_fIsOpaque = const_cast<CImageLayout *>(DYNCAST(const CImageLayout, this))->IsOpaque();
    }
    else
        pdni->_fIsOpaque = FALSE;

    // if there is a background image that doesn't cover the whole site, then we cannont be 
    // opaque
    //
    // BUGBUG (carled) we are too close to RC0 to do the full fix.  Bug #66092 is opened for the ie6
    // timeframe to clean this up.  the imagehelper fx (above) should be REMOVED!! gone. bad
    // instead we need a virtual function on CLayout called BOOL CanBeOpaque(). The def imple
    // should contain the if stmt below. CImageLayout should override and use the contents 
    // of CImgHelper::IsOpaque, (and call super). Framesets could possibly override and set
    // to false.  Input type=Image should override and do the same things as CImageLayout
    //
    pdni->_fIsOpaque  =    !TestLayoutDescFlag(LAYOUTDESC_NEVEROPAQUE)
                       &&  (   pdni->_fIsOpaque
                            || bi.crBack != COLORREF_NONE
                            ||  (   !!bi.pImgCtx
                                 &&  !!(bi.pImgCtx->GetState() & (IMGTRANS_OPAQUE))
                                 &&  pFF->_cuvBgPosX.GetRawValue() == 0
                                 &&  pFF->_cuvBgPosY.GetRawValue() == 0
                                 &&  pFF->_fBgRepeatX
                                 &&  pFF->_fBgRepeatY)
                            ||  (   (hwnd = pElement->GetHwnd()) != NULL
                                 &&  !(::GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT))
                           );

    //
    //  Determine direction, overflow, and scroll properties
    //

    pdni->_fRTL        = pTreeNode->GetCascadedBlockDirection() == styleDirRightToLeft;
    pdni->_overflowX   = pTreeNode->GetCascadedoverflowX();
    pdni->_overflowY   = pTreeNode->GetCascadedoverflowY();
    pdni->_fIsScroller = pTreeNode->IsScrollingParent();

    // In design mode, we want to treat overflow:hidden containers as overflow:visible
    // so editors can get to all their content.  This fakes out the display tree
    // so it creates CDispContainer*'s instead of CDispScroller, and hence doesn't
    // clip as hidden normally does. (KTam: #59722)
    // The initial fix is too aggressive; text areas implicitly set overflowX hidden
    // if they're in wordwrap mode.  Fix is to not do this munging for text areas..
    // Revisit this for IE6.
    if ( Doc()->_fDesignMode && pdni->_etag != ETAG_TEXTAREA )
    {
        if ( pdni->_overflowX == styleOverflowHidden )
        {
            pdni->_overflowX = styleOverflowVisible;
            pdni->_fIsScroller = FALSE;
        }
        if ( pdni->_overflowY == styleOverflowHidden )
        {
            pdni->_overflowY = styleOverflowVisible;
            pdni->_fIsScroller = FALSE;
        }
    }

    if (pdni->_etag == ETAG_BODY)
    {
        DYNCAST(const CBodyLayout, this)->UpdateScrollInfo(pdni);
    }
    else if (pdni->_etag == ETAG_OBJECT)
    {
        //  Never allow scroll bars on an object.  The object is responsible for that.
        //  Bug #77073  (greglett)
        pdni->_sp._fHSBAllowed =
        pdni->_sp._fHSBForced  =
        pdni->_sp._fVSBAllowed =
        pdni->_sp._fVSBForced  = FALSE;
    }
    else
    {
        GetDispNodeScrollbarProperties(pdni);
    }

    //
    //  Determine appearance properties
    //

    pdni->_fHasUserClip = ( pdni->_etag != ETAG_BODY
                        &&  (stylePosition)pFF->_bPositionType == stylePositionabsolute
                        &&  (   !pFF->_cuvClipTop.IsNullOrEnum()
                            ||  !pFF->_cuvClipBottom.IsNullOrEnum()
                            ||  !pFF->_cuvClipLeft.IsNullOrEnum()
                            ||  !pFF->_cuvClipRight.IsNullOrEnum()));

    pdni->_visibility   = VisibilityModeFromStyle(pTreeNode->GetCascadedvisibility());

    //
    //  Get border information (if requested)
    //

    if (fBorders)
    {
        Assert(pdci);

        pdni->_dnbBorders = pdni->_etag == ETAG_SELECT
                                ? DISPNODEBORDER_NONE
                                : (DISPNODEBORDER)pElement->GetBorderInfo(pdci, &(pdni->_bi), FALSE);

        Assert( pdni->_dnbBorders == DISPNODEBORDER_NONE
            ||  pdni->_dnbBorders == DISPNODEBORDER_SIMPLE
            ||  pdni->_dnbBorders == DISPNODEBORDER_COMPLEX);

        pdni->_fIsOpaque = pdni->_fIsOpaque
                            && (pdni->_dnbBorders == DISPNODEBORDER_NONE
                                || pdni->_bi.abStyles[BORDER_TOP] != fmBorderStyleDouble
                                && pdni->_bi.abStyles[BORDER_LEFT] != fmBorderStyleDouble
                                && pdni->_bi.abStyles[BORDER_BOTTOM] != fmBorderStyleDouble
                                && pdni->_bi.abStyles[BORDER_RIGHT] != fmBorderStyleDouble);

    }
}


//+----------------------------------------------------------------------------
//
//  Member:     GetDispNodeScrollbarProperties
//
//  Synopsis:   Set the scrollbar related properties of a CDispNodeInfo
//
//  Arguments:  pdni - Pointer to CDispNodeInfo
//
//-----------------------------------------------------------------------------

void
 CLayout::GetDispNodeScrollbarProperties(
    CDispNodeInfo * pdni) const
{
    switch (pdni->_overflowX)
    {
    case styleOverflowNotSet:
    case styleOverflowVisible:
    case styleOverflowHidden:
        pdni->_sp._fHSBAllowed =
        pdni->_sp._fHSBForced  = FALSE;
        break;

    case styleOverflowAuto:
        pdni->_sp._fHSBAllowed = TRUE;
        pdni->_sp._fHSBForced  = FALSE;
        break;

    case styleOverflowScroll:
        pdni->_sp._fHSBAllowed =
        pdni->_sp._fHSBForced  = TRUE;
        break;

#if DBG==1
    default:
        AssertSz(FALSE, "Illegal value for overflow style attribute!");
        break;
#endif
    }

    switch (pdni->_overflowY)
    {
    case styleOverflowNotSet:
    case styleOverflowVisible:
    case styleOverflowHidden:
        pdni->_sp._fVSBAllowed =
        pdni->_sp._fVSBForced  = FALSE;
        break;

    case styleOverflowAuto:
        pdni->_sp._fVSBAllowed = TRUE;
        pdni->_sp._fVSBForced  = FALSE;
        break;

    case styleOverflowScroll:
        pdni->_sp._fVSBAllowed =
        pdni->_sp._fVSBForced  = TRUE;
        break;

#if DBG==1
    default:
        AssertSz(FALSE, "Illegal value for overflow style attribute!");
        break;
#endif
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNode
//
//  Synopsis:   Ensure an appropriate display node exists
//
//              For all but FRAMESETs, if a container node is created, a single
//              CDispItemPlus node will also be created and inserted as the first
//              child in the flow layer.
//
//  Arugments:  pdci   - Current CDocInfo
//              fForce - Forcibly update the display node(s)
//
//  Returns:    S_OK    if successful
//              S_FALSE if nodes were created/destroyed/changed in a significant way
//              E_FAIL  otherwise
//
//-----------------------------------------------------------------------------

HRESULT
CLayout::EnsureDispNode(
    CDocInfo *  pdci,
    BOOL        fForce)
{
    CDispNode *     pDispNodeElement;
    CDispNode *     pDispNodeContent;
    CDispNodeInfo   dni;
    HRESULT         hr = S_OK;

    Assert(pdci);

    //
    //  Get display node attributes
    //

    GetDispNodeInfo(&dni, pdci, TRUE);
    pDispNodeContent = NULL;

    //
    //  If the wrong type of display node exists, replace it
    //
    //  A new display is needed when:
    //      a) No display node exists
    //      b) If being forced to create a display node
    //      c) A different type of border is needed (none vs. simple vs. complex)
    //      d) The type of display node does not match current inset/scrolling/user-clip settings
    //

    pDispNodeElement = GetElementDispNode();

    if (    !pDispNodeElement
        ||  fForce
        ||  dni.GetBorderType() != pDispNodeElement->GetBorderType()
        ||  dni.HasInset()      != pDispNodeElement->HasInset()
        ||  dni.IsScroller()    != pDispNodeElement->IsScroller()
        ||  dni.HasUserClip()   != pDispNodeElement->HasUserClip()
        ||  dni.IsRTL()         != pDispNodeElement->IsRightToLeft())
    {
        CDispNode * pDispNode;
        BOOL        fRequireContainer;

        //
        //  Create the appropriate type of display node
        //

        fRequireContainer = (   pDispNodeElement
                            &&  pDispNodeElement->IsContainer()
                            &&  DYNCAST(CDispContainer, pDispNodeElement)->CountChildren() > 1);

        pDispNode = (dni.IsScroller()
                        ? (CDispNode *)CDispRoot::CreateDispScroller(this,
                                                                    FALSE,
                                                                    dni.HasUserClip(),
                                                                    dni.HasInset(),
                                                                    dni.GetBorderType(),
                                                                    dni.IsRTL())
                        :   fRequireContainer
                        ||  dni.IsTag(ETAG_BODY)
                        ||  dni.IsTag(ETAG_TABLE)
                        ||  dni.IsTag(ETAG_FRAMESET)
                        ||  dni.IsTag(ETAG_TR)
                            ? (CDispNode *)CDispRoot::CreateDispContainer(this,
                                                                        FALSE,
                                                                        dni.HasUserClip(),
                                                                        dni.HasInset(),
                                                                        dni.IsTag(ETAG_TR)? DISPNODEBORDER_NONE:
                                                                                            dni.GetBorderType(),
                                                                        dni.IsRTL())
                            : (CDispNode *)CDispRoot::CreateDispItemPlus(this,
                                                                        FALSE,
                                                                        dni.HasUserClip(),
                                                                        dni.HasInset(),
                                                                        dni.GetBorderType(),
                                                                        dni.IsRTL()));

        if (!pDispNode)
            goto Error;

        //
        //  Mark the node as owned and possibly filtered
        //

        pDispNode->SetOwned();
        pDispNode->SetFiltered(ElementOwner()->HasFilterPtr());

        //
        //  Anchor the display node
        //  (If a display node previously existed, the new node must take its place in the tree.
        //   Otherwise, just save the pointer.)
        //

        if (pDispNodeElement)
        {
            Assert(_pDispNode);
            Assert(_pDispNode == pDispNodeElement);

            // if we're replacing a scroller with another scroller, copy the
            // scroll offset
            if (dni.IsScroller() && pDispNodeElement->IsScroller())
            {
                CDispScroller* pOldScroller = DYNCAST(CDispScroller, pDispNodeElement);
                CDispScroller* pNewScroller = DYNCAST(CDispScroller, pDispNode);
                pNewScroller->CopyScrollOffset(pOldScroller);
            }
            
            DetachScrollbarController(pDispNodeElement);

            pDispNode->SetLayerType(pDispNodeElement->GetLayerType());
            pDispNode->ReplaceNode(pDispNodeElement);

            if (_pDispNode == pDispNodeElement)
            {
                _pDispNode = pDispNode;
            }
        }
        else
        {
            Assert(!_pDispNode);

            _pDispNode = pDispNode;
        }

        pDispNodeElement = pDispNode;

        hr = S_FALSE;
    }

    //
    //  The display node is the right type, but its borders may have changed
    //

    else if (pDispNodeElement->HasBorder())
    {
        CRect   rcBordersOld;
        CRect   rcBordersNew;

        pDispNodeElement->GetBorderWidths(&rcBordersOld);
        dni.GetBorderWidths(&rcBordersNew);

        if (rcBordersOld != rcBordersNew)
        {
            hr = S_FALSE;
        }
    }

    //
    //  Ensure a single content node if necessary
    //  NOTE: This routine never removes content nodes
    //

    if (    pDispNodeElement->IsContainer()
        &&  !dni.IsTag(ETAG_FRAMESET)
        &&  !dni.IsTag(ETAG_TR))
    {
        CDispNode * pDispNodeCurrent;

        pDispNodeCurrent =
        pDispNodeContent = GetFirstContentDispNode();

        if (    !pDispNodeContent
            ||  fForce
            ||  dni.IsRTL() != pDispNodeContent->IsRightToLeft())
        {
            BOOL    fDirectionChanged =     pDispNodeContent
                                        &&  pDispNodeContent->IsRightToLeft() != dni.IsRTL();

            pDispNodeContent = CDispRoot::CreateDispItemPlus(this,
                                                            FALSE,
                                                            FALSE,
                                                            FALSE,
                                                            DISPNODEBORDER_NONE,
                                                            dni.IsRTL());
            if (!pDispNodeContent)
                goto Error;

            pDispNodeContent->SetOwned(FALSE);
            pDispNodeContent->SetLayerType(DISPNODELAYER_FLOW);

            Assert((CPoint &)pDispNodeContent->GetPosition() == (CPoint &)g_Zero.pt);

            if (!pDispNodeCurrent)
            {
                DYNCAST(CDispInteriorNode, pDispNodeElement)->InsertChildInFlow(pDispNodeContent);
            }
            else
            {
                Assert(!pDispNodeCurrent->IsFiltered());
                Assert(fForce || dni.IsRTL() == pDispNodeContent->IsRightToLeft());

                pDispNodeContent->ReplaceNode(pDispNodeCurrent);

                if (fDirectionChanged)
                {
                    if (pDispNodeContent->IsRightToLeft())
                    {
                        pDispNodeContent->SetPositionTopRight(g_Zero.pt);
                    }
                    else
                    {
                        pDispNodeContent->SetPosition(g_Zero.pt);
                    }
                }
            }

            Assert(DYNCAST(CDispInteriorNode, pDispNodeElement)->GetFirstChildNodeInLayer(DISPNODELAYER_FLOW) == pDispNodeContent);

            hr = S_FALSE;
        }
    }

    //
    //  Set the display node properties
    //  NOTE: These changes do not require notifying the caller since they do not affect
    //        the size or position of the display node
    //

    EnsureDispNodeLayer(dni, pDispNodeElement);
    EnsureDispNodeBackground(dni, pDispNodeElement);
    EnsureDispNodeVisibility(dni.GetVisibility(), ElementOwner(), pDispNodeElement);

    pDispNodeElement->SetAffectsScrollBounds(   (   !ElementOwner()->IsRelative()
                                                &&  !ElementOwner()->IsInheritingRelativeness())
                                            ||  ElementOwner()->IsEditable(TRUE));

    if (dni.IsScroller())
    {
        EnsureDispNodeScrollbars(pdci, dni, pDispNodeElement);
    }

    if (ElementOwner()->HasFilterPtr())
        ElementOwner()->GetFilterPtr()->EnsureFilterState();

    return hr;

Error:
    if (pDispNodeContent)
    {
        pDispNodeContent->Destroy();
    }

    if (pDispNodeElement)
    {
        pDispNodeElement->Destroy();
    }

    _pDispNode = NULL;
    return E_FAIL;
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNodeLayer
//
//  Synopsis:   Set the layer type of the container display node
//
//              NOTE: If a filter node exists, it is given the same layer type
//
//  Arguments:  dni       - CDispNodeInfo with display node properties
//              pDispNode - Display node to set (defaults to layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::EnsureDispNodeLayer(
    DISPNODELAYER           layer,
    CDispNode *             pDispNode)
{
    if (!pDispNode)
        pDispNode = GetElementDispNode();

    if (    pDispNode
        &&  pDispNode->GetLayerType() != layer)
    {
        pDispNode->ExtractFromTree();
        pDispNode->SetLayerType(layer);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNodeBackground
//
//  Synopsis:   Set the background characteristics of the container display node
//
//  Arguments:  dni       - CDispNodeInfo with display node properties
//              pDispNode - Display node to set (defaults to layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::EnsureDispNodeBackground(
    const CDispNodeInfo &   dni,
    CDispNode *             pDispNode)
{
    if (!pDispNode)
    {
        if (Tag() == ETAG_TABLE)
        {   // NOTE: table might have a caption, and we don't want to set background image on the main 
            // display node of the table that contains the caption display node. (bug #65617)
            // therefore we ensure backround only on the table's GRID node.
            CTableLayout *pTableLayout = DYNCAST(CTableLayout, this);
            pDispNode = pTableLayout->GetTableOuterDispNode();
        }
        else
        {
            pDispNode = GetElementDispNode();
        }
    }

    if (pDispNode)
    {
        // Suppress backgrounds when printing unless explicitly asked for.
        BOOL fPaintBackground = dni.HasBackground()
                                && (Doc()->PaintBackground()
                                    || Tag() == ETAG_BUTTON
                                    || Tag() == ETAG_TEXTAREA
                                    || Tag() == ETAG_INPUT
                                    && DYNCAST(CInput, ElementOwner())->IsButton());

        // Fixed backgrounds imply backgrounds.
        Assert(!dni.HasFixedBackground() || dni.HasBackground());

        pDispNode->SetBackground(fPaintBackground);

        if (pDispNode->IsScroller())
        {
            pDispNode->SetFixedBackground(dni.HasFixedBackground() && fPaintBackground);
        }
        pDispNode->SetOpaque(dni.IsOpaque());
    }
}

//+----------------------------------------------------------------------------
//
// Synposis:    Given a dispnode container, create a child flow node if one
//              does not exist.
//
//-----------------------------------------------------------------------------
CDispNode *
EnsureContentNode(CDispNode * pDispNode)
{
    Assert(pDispNode->IsContainer());

    CDispContainer * pDispContainer = DYNCAST(CDispContainer, pDispNode);
    CDispNode      * pDispContent = pDispContainer->GetFirstChildNodeInLayer(DISPNODELAYER_FLOW);

// BUGBUG, srinib (what if the first child node is not a flow node
// but a layout node in flow layer).
    if(!pDispContent)
    {
        CSize       size;
        BOOL        fRightToLeft = pDispContainer->IsRightToLeft();

        pDispContent = CDispRoot::CreateDispItemPlus(pDispContainer->GetDispClient(),
                                                    FALSE,
                                                    FALSE,
                                                    FALSE,
                                                    DISPNODEBORDER_NONE,
                                                    fRightToLeft);

        if (!pDispContent)
            goto Error;

        pDispContainer->GetSize(&size);

        pDispContent->SetOwned(FALSE);
        pDispContent->SetLayerType(DISPNODELAYER_FLOW);
        pDispContent->SetSize(size, FALSE);
        if (!pDispNode->IsScroller())
        {
            pDispContent->SetAffectsScrollBounds(pDispNode->AffectsScrollBounds());
        }
        if(fRightToLeft)
        {
            pDispContent->FlipBounds();
        }
        pDispContent->SetVisible(pDispContainer->IsVisible());

        pDispContainer->InsertChildInFlow(pDispContent);

        Assert(pDispContainer->GetFirstChildNodeInLayer(DISPNODELAYER_FLOW) == pDispContent);
    }

Error:
    return pDispContent;
}

//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNodeIsContainer
//
//  Synopsis:   Ensure that the display node is a container display node
//              NOTE: This routine is not a replacement for EnsureDispNode and
//                    only works after calling EnsureDispNode.
//
//              For all layouts but FRAMESETs, if a container node is created, a single
//              CDispItemPlus node will also be created and inserted as the first
//              child in the flow layer.
//
//  Returns:    Pointer to CDispContainer if successful, NULL otherwise
//
//-----------------------------------------------------------------------------
CDispContainer *
CLayout::EnsureDispNodeIsContainer(
    CElement *  pElement)
{
    Assert( !pElement
        ||  pElement->GetUpdatedNearestLayout() == this);
    Assert( !pElement
        ||  pElement == ElementOwner()
        ||  !pElement->NeedsLayout());

    CDispNode *         pDispNodeOld = GetElementDispNode(pElement);
    CDispContainer *    pDispNodeNew = NULL;
    CRect               rc;

    if (!pDispNodeOld)
        goto Cleanup;

    if (pDispNodeOld->IsContainer())
    {
        pDispNodeNew = DYNCAST(CDispContainer, pDispNodeOld);
        goto Cleanup;
    }

    //
    //  Create a basic container using the properties of the current node as a guide
    //

    Assert(pDispNodeOld->GetNodeType() == DISPNODETYPE_ITEMPLUS);
    Assert( (   pElement
            &&  pElement != ElementOwner())
        ||  !GetFirstContentDispNode());

    pDispNodeNew = CDispRoot::CreateDispContainer(DYNCAST(CDispItemPlus, pDispNodeOld));

    if (!pDispNodeNew)
        goto Cleanup;

    //
    //  Set the background flag on display nodes for relatively positioned text
    //  (Since text that has a background is difficult to detect, the code always
    //   assumes a background exists and lets the subsequent calls to draw the
    //   background handle it)
    //

    if (    pElement
        &&  pElement != ElementOwner())
    {
        Assert( !pElement->NeedsLayout()
            &&  pElement->IsRelative());
        pDispNodeNew->SetBackground(TRUE);
    }

    //
    //  Replace the existing node
    //

    pDispNodeNew->ReplaceNode(pDispNodeOld);
    SetElementDispNode(pElement, pDispNodeNew);

    //
    //  Ensure a single flow node if necessary
    //

    if (ElementOwner()->Tag() != ETAG_FRAMESET)
    {
        if(!EnsureContentNode(pDispNodeNew))
            goto Cleanup;
    }

Cleanup:
    return pDispNodeNew;
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNodeScrollbars
//
//  Synopsis:   Set the scroller properties of a display node
//              NOTE: The call is ignored if CDispNode is not a CDispScrollerPlus
//
//  Arguments:  sp        - CScrollbarProperties object
//              pDispNode - CDispNode to set (default to the layout display node)
//
//-----------------------------------------------------------------------------

void
CLayout::EnsureDispNodeScrollbars(
    CDocInfo *                      pdci,
    const CScrollbarProperties &    sp,
    CDispNode *                     pDispNode)
{
    if (!pDispNode)
        pDispNode = GetElementDispNode();

    if (    pDispNode
        &&  pDispNode->IsScroller())
    {
        Assert(!sp._fHSBForced || sp._fHSBAllowed);
        Assert(!sp._fVSBForced || sp._fVSBAllowed);

#ifdef  IE5_ZOOM

        long    wNumerXOrig = 1;
        long    wNumerYOrig = 1;
        long    wDenomOrig = 1;
        BOOL    fRestoreScale = FALSE;

        // Body scroll bars should not be scaled
        if (Tag() == ETAG_BODY && pdci->IsZoomed())
        {
            wNumerXOrig = pdci->GetNumerX();
            wNumerYOrig = pdci->GetNumerY();
            wDenomOrig  = pdci->GetDenom();
            pdci->zoom(1, 1, 1);
            fRestoreScale = TRUE;
        }

#endif //IE5_ZOOM

        long cySB = pdci->DocPixelsFromWindowY(sp._fHSBAllowed ? g_sizeScrollbar.cy : 0);
        long cxSB = pdci->DocPixelsFromWindowX(sp._fVSBAllowed ? g_sizeScrollbar.cx : 0);

#ifdef  IE5_ZOOM

        if (fRestoreScale)
            pdci->zoom(wNumerXOrig, wNumerYOrig, wDenomOrig);

#endif //IE5_ZOOM

        DYNCAST(CDispScroller, pDispNode)->SetHorizontalScrollbarHeight(cySB, sp._fHSBForced);
        DYNCAST(CDispScroller, pDispNode)->SetVerticalScrollbarWidth(cxSB, sp._fVSBForced);
    }

#if DBG==1
    else if (pDispNode)
    {
        Assert(!sp._fHSBAllowed);
        Assert(!sp._fVSBAllowed);
        Assert(!sp._fHSBForced);
        Assert(!sp._fVSBForced);
    }
#endif
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureDispNodeVisibility
//
//  Synopsis:   Set the visibility mode on display node corresponding to
//              this layout
//
//              NOTE: If a filter node exists, it is given the same visibility mode
//
//  Arguments:  dni       - CDispNodeInfo with display node properties
//              pDispNode - Display node to set (defaults to layout display node)
//
//-----------------------------------------------------------------------------
void            
CLayout::EnsureDispNodeVisibility(CElement *pElement, CDispNode * pDispNode)
{
    if (!pElement)
        pElement = ElementOwner();

    if (pElement && pElement->GetFirstBranch())
    {
        VISIBILITYMODE vm;

        vm = VisibilityModeFromStyle(pElement->GetFirstBranch()->GetCascadedvisibility());

        EnsureDispNodeVisibility( vm, pElement, pDispNode);
    }
}



void
CLayout::EnsureDispNodeVisibility(
    VISIBILITYMODE  visibilityMode,
    CElement *      pElement,
    CDispNode *     pDispNode)
{
    Assert(pElement);

    if (!pDispNode)
        pDispNode = GetElementDispNode(pElement);

    if (pDispNode)
    {
        CElement *  pElementParent = pElement->GetFirstBranch()->Parent()->Element();

        Verify(OpenView());

        if (visibilityMode == VISIBILITYMODE_INHERIT)
        {
            visibilityMode = pElement->GetFirstBranch()->GetCharFormat()->_fVisibilityHidden
                                    ? VISIBILITYMODE_INVISIBLE
                                    : VISIBILITYMODE_VISIBLE;
        }

        if (    visibilityMode == VISIBILITYMODE_INVISIBLE
            &&  pElementParent->IsEditable())
        {
            visibilityMode = VISIBILITYMODE_VISIBLE;
        }

        Assert(visibilityMode != VISIBILITYMODE_INHERIT);
        pDispNode->SetVisible(visibilityMode == VISIBILITYMODE_VISIBLE);

        EnsureContentVisibility(pDispNode, visibilityMode == VISIBILITYMODE_VISIBLE);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureContentVisibility
//
//  Synopsis:   Ensure the visibility of the content node is correct
//
//  Arguments:  pDispNode - Parent CDispNode of the content node
//              fVisible  - TRUE to make visible, FALSE otherwise
//
//-----------------------------------------------------------------------------

void
CLayout::EnsureContentVisibility(
    CDispNode * pDispNode,
    BOOL        fVisible)
{
    CDispNode * pContentNode = GetFirstContentDispNode(pDispNode);

    if (pContentNode)
    {
        pContentNode->SetVisible(fVisible);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     ExtractDispNodes
//
//  Synopsis:   Remove all children in the range from the tree
//
//  Arguments:  pDispNodeStart   - First node to adjust, NULL starts with first child
//              pDispNodeEnd     - Last node to adjust, NULL ends with last child
//              fRestrictToLayer - Restrict search to starting layer (ignore if pDispNodeStart is NULL)
//
//-----------------------------------------------------------------------------

void
CLayout::ExtractDispNodes(
    CDispNode * pDispNodeStart,
    CDispNode * pDispNodeEnd,
    BOOL        fRestrictToLayer)
{
    CDispNode * pDispNode = GetElementDispNode();

    //
    //  If there is nothing to do, exit
    //

    if (!pDispNode)
        goto Cleanup;

    if (!pDispNode->IsContainer())
        goto Cleanup;

    //
    //  Determine the start node (if none was supplied)
    //

    if (!pDispNodeStart)
    {
        pDispNodeStart   = DYNCAST(CDispContainer, pDispNode)->GetFirstChildNode();
        fRestrictToLayer = FALSE;
    }

    if (!pDispNodeStart)
        goto Cleanup;

    //
    //  Find the end node (if none was supplied)
    //

    if (!pDispNodeEnd)
    {
        for (pDispNode = pDispNodeStart;
             pDispNode;
             pDispNode = pDispNode->GetNextSiblingNode(fRestrictToLayer))
        {
            pDispNodeEnd = pDispNode;
        }
    }
    Assert(pDispNodeEnd);

    //
    //  Extract the nodes
    //

    do
    {
        pDispNode    = pDispNodeEnd;
        pDispNodeEnd = pDispNode->GetPreviousSiblingNode();

        pDispNode->ExtractFromTree();

        if (!pDispNode->IsOwned())
        {
            pDispNode->Destroy();
        }

    } while (   pDispNodeEnd
            &&  pDispNode != pDispNodeStart);
    Assert(pDispNode == pDispNodeStart);

Cleanup:
    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     SetPositionAware
//
//  Synopsis:   Set/clear the position aware flag on the display node
//
//              NOTE: If a filter node exists, it is given the same position awareness
//
//  Arguments:  fPositionAware - Value to set
//              pDispNode      - Display node to set (defaults to layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::SetPositionAware(
    BOOL        fPositionAware,
    CDispNode * pDispNode)
{
    if (!pDispNode)
        pDispNode = GetElementDispNode();

    pDispNode->SetPositionAware(fPositionAware);
}


//+----------------------------------------------------------------------------
//
//  Member:     SetInsertionAware
//
//  Synopsis:   Set/clear the insertion aware flag on the display node
//
//              NOTE: If a filter node exists, it is given the same position awareness
//
//  Arguments:  fInsertionAware - Value to set
//              pDispNode       - Display node to set (defaults to layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::SetInsertionAware(
    BOOL        fInsertionAware,
    CDispNode * pDispNode)
{
    if (!pDispNode)
        pDispNode = GetElementDispNode();

    pDispNode->SetInsertionAware(fInsertionAware);
}


//+----------------------------------------------------------------------------
//
//  Member:     SizeDispNode
//
//  Synopsis:   Adjust the size of the container display node
//
//  Arugments:  pci            - Current CCalcInfo
//              size           - The width/height of the entire layout
//              fInvalidateAll - If TRUE, force a full invalidation
//
//-----------------------------------------------------------------------------
void
CLayout::SizeDispNode(
    CCalcInfo *     pci,
    const SIZE &    size,
    BOOL            fInvalidateAll)
{
    CDoc *          pDoc;
    CElement *      pElement;
    CDispNode *     pDispNodeElement;
    CSize           sizeOriginal;
    ELEMENT_TAG     etag;
    DISPNODEBORDER  dnb;

    Assert(pci);
    if (!_pDispNode)
        goto Cleanup;

    pDispNodeElement = GetElementDispNode();

    //
    //  Set the border size (if any)
    //  NOTE: These are set before the size because a change in border widths
    //        forces a full invalidation of the display node. If a full
    //        invalidation is necessary, less code is executed when the
    //        display node's size is set.
    //

    pDoc           = Doc();
    pElement       = ElementOwner();
    etag           = pElement->Tag();
    dnb            = pDispNodeElement->GetBorderType();
    fInvalidateAll = !pDispNodeElement->IsContainer();

    pDispNodeElement->GetSize(&sizeOriginal);

    if (dnb != DISPNODEBORDER_NONE)
    {
        CRect       rcBorderWidths;
        CRect       rc;
        CBorderInfo bi;

        pDispNodeElement->GetBorderWidths(&rcBorderWidths);

        pElement->GetBorderInfo(pci, &bi, FALSE);

        rc.left   = pci->DocPixelsFromWindowX(bi.aiWidths[BORDER_LEFT]);
        rc.top    = pci->DocPixelsFromWindowY(bi.aiWidths[BORDER_TOP]);
        rc.right  = pci->DocPixelsFromWindowX(bi.aiWidths[BORDER_RIGHT]);
        rc.bottom = pci->DocPixelsFromWindowY(bi.aiWidths[BORDER_BOTTOM]);

        if (rc != rcBorderWidths)
        {
            if (dnb == DISPNODEBORDER_SIMPLE)
            {
                pDispNodeElement->SetBorderWidths(rc.top);
            }
            else
            {
                pDispNodeElement->SetBorderWidths(rc);
            }

            fInvalidateAll = TRUE;
        }
    }

    //
    //  Determine if a full invalidation is necessary
    //  (A full invalidation is necessary only when there is a fixed
    //   background located at a percentage of the width/height)
    //

    if (    !fInvalidateAll
        &&  pDispNodeElement->HasBackground())
    {
        const CFancyFormat *    pFF = pElement->GetFirstBranch()->GetFancyFormat();

        fInvalidateAll =    pFF->_lImgCtxCookie
                    &&  (   pFF->_cuvBgPosX.GetUnitType() == CUnitValue::UNIT_PERCENT
                        ||  pFF->_cuvBgPosY.GetUnitType() == CUnitValue::UNIT_PERCENT);
    }

    //
    //  Size the display node
    //  NOTE: Set only the width/height since top/left are managed
    //        by the layout engine which inserts this node into the
    //        display tree.
    //

    pDispNodeElement->SetSize(size, fInvalidateAll);

    //
    //  If the display node has an explicit user clip, size it
    //

    if (pDispNodeElement->HasUserClip())
    {
        SizeDispNodeUserClip(pci, size, pDispNodeElement);
    }

    //
    //  Fire related events
    //

    if (    (CSize &)size != sizeOriginal
        &&  !IsDisplayNone()
        &&  pDoc->_state >= OS_INPLACE
        &&  pDoc->_fFiredOnLoad)
    {
        GetView()->AddEventTask(pElement, DISPID_EVMETH_ONRESIZE);
    }

    if (pElement->ShouldFireEvents())
    {
        if (size.cx != sizeOriginal.cx)
        {
            pElement->FireOnChanged(DISPID_IHTMLELEMENT_OFFSETWIDTH);
            pElement->FireOnChanged(DISPID_IHTMLELEMENT2_CLIENTWIDTH);
        }

        if (size.cy != sizeOriginal.cy)
        {
            pElement->FireOnChanged(DISPID_IHTMLELEMENT_OFFSETHEIGHT);
            pElement->FireOnChanged(DISPID_IHTMLELEMENT2_CLIENTHEIGHT);
        }
    }

Cleanup:
    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     SizeDispNodeInsets
//
//  Synopsis:   Size the insets of the display node
//
//  Arguments:  va        - CSS verticalAlign value
//              cy        - Content height or baseline delta
//              pDispNode - CDispNode to set (defaults to the layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::SizeDispNodeInsets(
    styleVerticalAlign  va,
    long                cy,
    CDispNode *         pDispNode)
{
    if (!pDispNode)
        pDispNode = GetElementDispNode();

    if (    pDispNode
        &&  pDispNode->HasInset())
    {
        CSize   size;
        CSize   sizeInset;
        long    cyHeight;

        pDispNode->GetSize(&size);
        cyHeight = size.cy;

        Assert(va == styleVerticalAlignBaseline || cy >= 0);
        // BUGBUG: Assert removed for NT5 B3 (bug #75434)
        // IE6: Reenable for IE6+
        // Assert(cy <= cyHeight);

        switch (va)
        {
        case styleVerticalAlignTop:
            sizeInset = g_Zero.size;
            break;

        case styleVerticalAlignMiddle:
            sizeInset.cx = 0;
            sizeInset.cy = (cyHeight - cy) / 2;
            break;

        case styleVerticalAlignBottom:
            sizeInset.cx = 0;
            sizeInset.cy = cyHeight - cy;
            break;

        case styleVerticalAlignBaseline:
            sizeInset.cx = 0;
            sizeInset.cy = cy;
            break;
        }

        pDispNode->SetInset(sizeInset);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     SizeDispNodeUserClip
//
//  Synopsis:   Calculate and set the user clip based on user settings
//              The default is infinite (represented by LONG_MIN/MAX) and all values
//              are relative to the origin (0,0)
//
//  Arguments:  pdci      - Current CDocInfo
//              size      - Current width/height
//              pDispNode - Display node on which to set the user clip (defaults to the layout display node)
//
//-----------------------------------------------------------------------------
void
CLayout::SizeDispNodeUserClip(
    CDocInfo *      pdci,
    const CSize &   size,
    CDispNode *     pDispNode)
{
    CElement *  pElement  = ElementOwner();
    CTreeNode * pTreeNode = pElement->GetFirstBranch();
    CRect       rc;
    CUnitValue  uv;

    if (!pDispNode)
        pDispNode = GetElementDispNode();

    if (!pDispNode)
        goto Cleanup;

    Assert(pdci);
    Assert(pTreeNode);
    Assert(pDispNode->HasUserClip());

    rc.SetRect(LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX);

    uv = pTreeNode->GetCascadedclipLeft();
    if (    !uv.IsNull()
        &&  (   CUnitValue::IsScalerUnit(uv.GetUnitType())
            ||  uv.GetUnitType() == CUnitValue::UNIT_PERCENT))
    {
        rc.left = uv.XGetPixelValue(pdci, size.cx, pTreeNode->GetFontHeightInTwips(&uv));
    }

    uv = pTreeNode->GetCascadedclipRight();
    if (    !uv.IsNull()
        &&  (   CUnitValue::IsScalerUnit(uv.GetUnitType())
            ||  uv.GetUnitType() == CUnitValue::UNIT_PERCENT))
    {
        rc.right = uv.XGetPixelValue(pdci, size.cx, pTreeNode->GetFontHeightInTwips(&uv));
    }

    uv = pTreeNode->GetCascadedclipTop();
    if (    !uv.IsNull()
        &&  (   CUnitValue::IsScalerUnit(uv.GetUnitType())
            ||  uv.GetUnitType() == CUnitValue::UNIT_PERCENT))
    {
        rc.top = uv.XGetPixelValue(pdci, size.cy, pTreeNode->GetFontHeightInTwips(&uv));
    }

    uv = pTreeNode->GetCascadedclipBottom();
    if (    !uv.IsNull()
        &&  (   CUnitValue::IsScalerUnit(uv.GetUnitType())
            ||  uv.GetUnitType() == CUnitValue::UNIT_PERCENT))
    {
        rc.bottom = uv.XGetPixelValue(pdci, size.cy, pTreeNode->GetFontHeightInTwips(&uv));
    }

    pDispNode->SetUserClip(rc);

Cleanup:
    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     SizeContentDispNode
//
//  Synopsis:   Adjust the size of the content node (if it exists)
//              NOTE: Unlike SizeDispNode above, this routine assumes that
//                    the passed size is correct and uses it unmodified
//
//  Arugments:  size           - The width/height of the content node
//              fInvalidateAll - If TRUE, force a full invalidation
//
//-----------------------------------------------------------------------------
void
CLayout::SizeContentDispNode(
    const SIZE &    size,
    BOOL            fInvalidateAll)
{
    CDispItemPlus * pDispContent;
    CSize           sizeContent;

    pDispContent = GetFirstContentDispNode();
    sizeContent  = size;

    if (pDispContent)
    {
        CView *     pView = GetView();
        CDispNode * pDispElement;
        CSize       sizeOriginal;
        CRect       rc;

        Assert(pView);

        pDispElement = GetElementDispNode();
        pDispElement->GetClientRect(&rc, CLIENTRECT_CONTENT);
        pDispContent->GetSize(&sizeOriginal);

        //
        //  Ensure the passed size is correct
        //
        //    1) Scrolling containers always use the passed size
        //    2) Non-scrolling containers limit the size their client rectangle
        //    3) When editing psuedo-borders are enabled, ensure the size no less than the client rectangle
        //

        if (!pDispElement->IsScroller())
        {
            sizeContent = rc.Size();
        }

        if (    pView->IsFlagSet(CView::VF_ZEROBORDER)
            &&  ElementOwner()->IsEditable())
        {
            sizeContent.Max(rc.Size());
        }

        //
        //  If the size differs, set the new size
        //  (Invalidate the entire area for all changes to non-CFlowLayouts
        //   or anytime the width changes)
        //

        if (sizeOriginal != sizeContent)
        {
            fInvalidateAll =    fInvalidateAll
                            ||  !TestLayoutDescFlag(LAYOUTDESC_FLOWLAYOUT)
                            ||  sizeOriginal.cx != sizeContent.cx;

            pDispContent->SetSize(sizeContent, fInvalidateAll);
        }
        else if (fInvalidateAll)
        {
            CSize   size;

            pDispContent->GetSize(&size);
            pDispContent->Invalidate(CRect(size), COORDSYS_CONTAINER);
        }
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     TranslateDispNodes
//
//  Synopsis:   Adjust the position of a range of display nodes by the passed amount
//
//  Arguments:  size             - Amount by which to adjust
//              pDispNodeStart   - First node to adjust, NULL starts with first child
//              pDispNodeEnd     - Last node to adjust, NULL ends with last child
//              fRestrictToLayer - Restrict search to starting layer (ignore if pDispNodeStart is NULL)
//
//-----------------------------------------------------------------------------
void
CLayout::TranslateDispNodes(
    const SIZE &    size,
    CDispNode *     pDispNodeStart,
    CDispNode *     pDispNodeEnd,
    BOOL            fRestrictToLayer,
    BOOL            fExtractHidden)
{
    CDispNode * pDispNode = GetElementDispNode();

    //
    //  If there is nothing to do, exit
    //

    if (!pDispNode)
        goto Cleanup;

    if (    !pDispNode->IsContainer()
        ||  !DYNCAST(CDispContainer, pDispNode)->CountChildren())
        goto Cleanup;

    if (    !size.cx
        &&  !size.cy
        &&  !fExtractHidden)
        goto Cleanup;

    //
    //  Check for reasonable values
    //

    Assert(size.cx > (LONG_MIN / 2));
    Assert(size.cx < (LONG_MAX / 2));
    Assert(size.cy > (LONG_MIN / 2));
    Assert(size.cy < (LONG_MAX / 2));

    //
    //  Determine the start node (if none was supplied)
    //

    if (!pDispNodeStart)
    {
        pDispNodeStart   = DYNCAST(CDispContainer, pDispNode)->GetFirstChildNode();
        fRestrictToLayer = FALSE;
    }

    //
    //  Translate the nodes
    //

    pDispNode = pDispNodeStart;

    while (pDispNode)
    {
        CDispNode * pDispNodeCur = pDispNode;
        void *      pvOwner;

        pDispNode = pDispNodeCur->GetNextSiblingNode(fRestrictToLayer);

        //
        // if the current disp node is a text flow node or if the disp node
        // owner is not hidden then translate it or extract the disp node
        //
        if(pDispNodeCur->GetDispClient() == this)
        {
            pDispNodeCur->SetPosition(pDispNodeCur->GetPosition() + size);
        }
        else
        {
            pDispNodeCur->GetDispClient()->GetOwner(pDispNodeCur, &pvOwner);

            if (pvOwner)
            {
                CElement *  pElement = DYNCAST(CElement, (CElement *)pvOwner);

                if(fExtractHidden && pElement->IsDisplayNone())
                {
                    pDispNodeCur->ExtractFromTree();
                }
                else if (size.cx || size.cy)
                {
                    if(pElement->NeedsLayout())
                    {
                        pElement->GetUpdatedLayout()->SetPosition(pDispNodeCur->GetPosition() + size);
                    }
                    else
                    {
                        pDispNodeCur->SetPosition(pDispNodeCur->GetPosition() + size);
                    }
               }
            }
        }

        if (pDispNodeCur == pDispNodeEnd)
            break;
    }

Cleanup:
    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     DestroyDispNode
//
//  Synopsis:   Disconnect/destroy all display nodes
//              NOTE: This only needs to destroy the container node since
//                    all other created nodes will be destroyed as a by-product.
//
//-----------------------------------------------------------------------------
void
CLayout::DestroyDispNode()
{
    if (_pDispNode)
    {
        DetachScrollbarController(_pDispNode);

        Verify(OpenView());
        _pDispNode->Destroy();
        _pDispNode = NULL;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     HandleScrollbarMessage
//
//  Synopsis:   Process a possible message for the scrollbar
//
//  Arguments:  pMessage - Message
//              pElement - Target element
//
//----------------------------------------------------------------------------
HRESULT
CLayout::HandleScrollbarMessage(
    CMessage *  pMessage,
    CElement *  pElement)
{
extern SIZE g_sizeScrollButton;

    CDispNode * pDispNode = GetElementDispNode();
    CDoc *      pDoc      = Doc();
    HRESULT     hr        = S_FALSE;

    if (    !pDispNode
        ||  !ElementOwner()->IsEnabled())
        return hr;

    Assert(pDispNode->IsScroller());

    switch (pMessage->message)
    {
    case WM_SETCURSOR:
        SetCursorIDC(IDC_ARROW);
        hr = S_OK;
        break;

    //
    //  Ignore up messages to simple fall-through
    //  ("Real" up messages are sent to the scrollbar directly
    //   since it captures the mouse on the cooresponding down message)
    //
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        break;

#ifndef UNIX
    case WM_MBUTTONDOWN:
#endif
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
        break;

    case WM_LBUTTONDBLCLK:
        pDoc->_fGotDblClk = FALSE;

#ifdef UNIX
    case WM_MBUTTONDOWN:
#endif
    case WM_LBUTTONDOWN:
        AttachScrollbarController(pDispNode, pMessage);
        hr = S_OK;
        break;

    case WM_KEYDOWN:
        Assert(VK_PRIOR < VK_DOWN);
        Assert(VK_NEXT  > VK_PRIOR);
        Assert(VK_END   > VK_PRIOR);
        Assert(VK_HOME  > VK_PRIOR);
        Assert(VK_LEFT  > VK_PRIOR);
        Assert(VK_UP    > VK_PRIOR);
        Assert(VK_RIGHT > VK_PRIOR);

        if (    !(pMessage->dwKeyState & FALT)
            &&  pMessage->wParam >= VK_PRIOR
            &&  pMessage->wParam <= VK_DOWN)
        {
            UINT            uCode;
            long            cAmount;
            int             iDirection;
            CDispNodeInfo   dni;

            GetDispNodeInfo(&dni);

            uCode      = SB_THUMBPOSITION;
            cAmount    = 0;
            iDirection = 1;

            switch (pMessage->wParam)
            {
            case VK_END:
                cAmount = LONG_MAX;

            case VK_HOME:
                uCode = SB_THUMBPOSITION;
                break;

            case VK_NEXT:
                uCode = SB_PAGEDOWN;
                break;

            case VK_PRIOR:
                uCode = SB_PAGEUP;
                break;

            case VK_LEFT:
                iDirection = 0;
                // falling through

            case VK_UP:
                uCode = SB_LINEUP;
                break;

            case VK_RIGHT:
                iDirection = 0;
                // falling through

            case VK_DOWN:
                uCode = SB_LINEDOWN;
                break;
            }

            // Scroll only if scolling is allowed (IE5 #67686)
            if (    iDirection == 1 && dni.IsVScrollbarAllowed()
                ||  iDirection == 0 && dni.IsHScrollbarAllowed()
               )
            {              
                hr = OnScroll(iDirection, uCode, cAmount, FALSE, (pMessage->wParam&0x4000000)
                                                                    ? 50  // BUGBUG: For now we are using the mouse delay - should use Api to find system key repeat rate set in control panel.
                                                                    : MAX_SCROLLTIME);
            }
        }
        break;
    }

    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:     OnScroll
//
//  Synopsis:   Compute scrolling info.
//
//  Arguments:  iDirection  0 - Horizontal scrolling, 1 - Vertical scrolling
//              uCode       scrollbar event code (SB_xxxx)
//              lPosition   new scroll position
//              fRepeat     TRUE if the previous scroll action is repeated
//              lScrollTime time in millisecs to scroll (smoothly)
//
//  Return:     HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CLayout::OnScroll(
    int     iDirection,
    UINT    uCode,
    long    lPosition,
    BOOL    fRepeat,
    LONG    lScrollTime)
{
extern WORD wConvScroll(WORD wparam);
extern SIZE g_sizeSystemChar;

    HRESULT hr = S_OK;

    //
    //  Ignore requests that arrive while the document is not in-place active
    //

    if (    Doc()->_state < OS_INPLACE
        &&  !Doc()->IsPrintDoc())
    {
        hr = OLE_E_INVALIDRECT;
    }

    //
    //  Scroll the appropriate direction
    //

    else
    {
        //
        //  If vertical, flip the direction and SB_xxxx code
        //

        if (_fVertical)
        {
            iDirection = !iDirection;
            uCode      = wConvScroll((WORD)uCode);
        }

        //
        //  Scroll the appropriate amount
        //

        switch (uCode)
        {
        case SB_LINEUP:
            if (iDirection)
                ScrollByPercent(CSize(0, -125), lScrollTime);
            else
                ScrollBy(CSize(-g_sizeSystemChar.cx, 0), lScrollTime);
            break;

        case SB_LINEDOWN:
            if (iDirection)
                ScrollByPercent(CSize(0, 125), lScrollTime);
            else
                ScrollBy(CSize(g_sizeSystemChar.cx, 0), lScrollTime);
            break;

        case SB_PAGEUP:
        case SB_PAGEDOWN:
            {
                if (iDirection)
                    ScrollByPercent(CSize(0, (uCode == SB_PAGEUP ? -875 : 875)), lScrollTime);
                else
                {
                    CRect   rc;
                    CSize   size;

                    GetClientRect(&rc);

                    size             = g_Zero.size;
                    size[iDirection] = (uCode == SB_PAGEUP ? -1 : 1) *
                                            max(1L,
                                            rc.Size(iDirection) - (((CSize &)g_sizeSystemChar)[iDirection] * 2L));

                    ScrollBy(size, lScrollTime);
                }
            }
            break;

        case SB_TOP:
            if (iDirection)
            {
                ScrollToY(0);
            }
            else
            {
                ScrollToX(0);
            }
            break;

        case SB_BOTTOM:
            if (iDirection)
            {
                ScrollToY(LONG_MAX);
            }
            else
            {
                ScrollToX(LONG_MAX);
            }
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            if (iDirection)
            {
                ScrollToY(lPosition);
            }
            else
            {
                ScrollToX(lPosition);
            }
            break;

        case SB_ENDSCROLL:
            break;
        }
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     ScrollBy
//              ScrollByPercent
//              ScrollTo
//              ScrollToX
//              ScrollToY
//
//  Synopsis:   Various scroll helpers
//
//  Arguments:  various size values (either percent or fixed amounts)
//
//-----------------------------------------------------------------------------
BOOL
CLayout::ScrollBy(
    const CSize &   sizeDelta,
    LONG            lScrollTime)
{
    CDispNode * pDispNode   = GetElementDispNode();
    BOOL        fRet        = FALSE;

    if (    pDispNode
        &&  pDispNode->IsScroller()
        &&  sizeDelta != g_Zero.size)
    {
        CSize   sizeOffset;

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

        sizeOffset += sizeDelta;

        fRet = ScrollTo(sizeOffset, lScrollTime);
    }
    return fRet;
}


BOOL
CLayout::ScrollByPercent(
    const CSize &   sizePercent,
    LONG            lScrollTime)
{
    CDispNode * pDispNode   = GetElementDispNode();
    BOOL        fRet        = FALSE;

    if ( pDispNode &&
         pDispNode->IsScroller() &&
         sizePercent != g_Zero.size)
    {
        CRect   rc;
        CSize   sizeOffset;
        CSize   sizeDelta;

        pDispNode->GetClientRect(&rc, CLIENTRECT_CONTENT);

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

        sizeDelta.cx = (sizePercent.cx
                            ? (rc.Width() * sizePercent.cx) / 1000L
                            : 0);
        sizeDelta.cy = (sizePercent.cy
                            ? (rc.Height() * sizePercent.cy) / 1000L
                            : 0);

        sizeOffset += sizeDelta;

        fRet = ScrollTo(sizeOffset, lScrollTime);
    }
    return fRet;
}


BOOL
CLayout::ScrollTo(
    const CSize &   sizeOffset,
    LONG            lScrollTime)
{
    CDispNode * pDispNode   = GetElementDispNode();
    BOOL        fRet        = FALSE;

    if (pDispNode && pDispNode->IsScroller() && OpenView(FALSE, TRUE))
    {
        CView *     pView        = GetView();
        CElement *  pElement     = ElementOwner();
        BOOL        fLayoutDirty = pView->HasLayoutTask(this);
        BOOL        fScrollBits  = !fLayoutDirty && lScrollTime >= 0;
        CSize       sizeOffsetCurrent;
        CPaintCaret hc( pElement->Doc()->_pCaret ); // Hide the caret for scrolling

        //
        //  If layout is needed, perform it prior to the scroll
        //  (This ensures that container and content sizes are correct before
        //   adjusting the scroll offset)
        //

        if (fLayoutDirty)
        {
            DoLayout(pView->GetLayoutFlags() | LAYOUT_MEASURE);
        }

        //
        // If the incoming offset has is different, scroll and fire the event
        //

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffsetCurrent);

        if (sizeOffset != sizeOffsetCurrent)
        {
            AddRef();
            //
            //  Set the new scroll offset
            //  (If no layout work was pending, do an immediate scroll)
            //  NOTE: Setting the scroll offset will force a synchronous invalidate/render
            //

            fRet = DYNCAST(CDispScroller, pDispNode)->SetScrollOffsetSmoothly(sizeOffset, fScrollBits, lScrollTime);
            
            //
            //  Ensure all deferred calls are executed
            //

            EndDeferred();

            Release();
        }
    }
    return fRet;
}


void
CLayout::ScrollToX(
    long    x,
    LONG    lScrollTime)
{
    CDispNode * pDispNode = GetElementDispNode();

    if (pDispNode && pDispNode->IsScroller())
    {
        CSize   sizeOffset;

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

        sizeOffset.cx = x;

        ScrollTo(sizeOffset, lScrollTime);
    }
}


void
CLayout::ScrollToY(
    long    y,
    LONG    lScrollTime)
{
    CDispNode * pDispNode = GetElementDispNode();

    if (pDispNode && pDispNode->IsScroller())
    {
        CSize   sizeOffset;

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

        sizeOffset.cy = y;

        ScrollTo(sizeOffset, lScrollTime);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     GetXScroll
//              GetYScroll
//
//  Synopsis:   Helpers to retrieve scroll offsets
//
//-----------------------------------------------------------------------------
long
CLayout::GetXScroll() const
{
    CDispNode * pDispNode = GetElementDispNode();

    if (    pDispNode
        &&  pDispNode->IsScroller())
    {
        CSize   sizeOffset;

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);
        return sizeOffset.cx;
    }
    else
        return 0;
}

long
CLayout::GetYScroll() const
{
    CDispNode * pDispNode = GetElementDispNode();

    if (    pDispNode
        &&  pDispNode->IsScroller())
    {
        CSize   sizeOffset;

        DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);
        return sizeOffset.cy;
    }
    else
        return 0;
}


//+----------------------------------------------------------------------------
//
//  Member:     DoLayout
//
//  Synopsis:   Perform layout
//
//  Arguments:  grfLayout - Collection of LAYOUT_xxxx flags
//
//-----------------------------------------------------------------------------
void
CLayout::DoLayout(DWORD grfLayout)
{
    Assert(grfLayout & (LAYOUT_MEASURE | LAYOUT_POSITION | LAYOUT_ADORNERS));

    //
    //  If the element is not hidden, layout its content
    //

    if (!IsDisplayNone())
    {
        CCalcInfo   CI(this);
        CSize       size;

        GetSize(&size);

        CI._grfLayout |= grfLayout;

        //
        //  If requested, measure
        //

        if (grfLayout & LAYOUT_MEASURE)
        {
            if (_fForceLayout)
            {
                CI._grfLayout |= LAYOUT_FORCE;
            }

            CalcSize(&CI, &size);

            Reset(FALSE);
        }
        _fForceLayout = FALSE;

        //
        //  Process outstanding layout requests (e.g., sizing positioned elements, adding adorners)
        //

        if (HasRequestQueue())
        {
            ProcessRequests(&CI, size);
        }
    }

    //
    //  Otherwise, clear dirty state and dequeue the layout request
    //

    else
    {
        FlushRequests();
        Reset(TRUE);
    }

    Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
}


//+----------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Respond to a notification
//
//  Arguments:  pnf - Notification sent
//
//-----------------------------------------------------------------------------
void
CLayout::Notify(
    CNotification * pnf)
{
    Assert(!pnf->IsReceived(_snLast));

    if (!TestLock(CElement::ELEMENTLOCK_SIZING))
    {
        // If the the current layout is hidden, then forward the current notification
        // to the parent as a resize notfication so that parents keep track of the dirty
        // range.
        if(    !pnf->IsFlagSet(NFLAGS_DESCENDENTS)
           &&  (   pnf->IsType(NTYPE_ELEMENT_REMEASURE)
                || pnf->IsType(NTYPE_ELEMENT_RESIZE)
                || pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)
                || pnf->IsType(NTYPE_CHARS_RESIZE))
           &&  IsDisplayNone())
        {
            pnf->ChangeTo(NTYPE_ELEMENT_RESIZE, ElementOwner());
        }
        else switch (pnf->Type())
        {
            case NTYPE_ELEMENT_RESIZE:
                if (!pnf->IsHandled())
                {
                    Assert(pnf->Element() != ElementOwner());

                    //  Always "dirty" the layout associated with the element
                    pnf->Element()->DirtyLayout(pnf->LayoutFlags());

                    //  Handle absolute elements by noting that one is dirty
                    if (pnf->Element()->IsAbsolute())
                    {
                        QueueRequest(CRequest::RF_MEASURE, pnf->Element());
        
                        if (pnf->IsFlagSet(NFLAGS_ANCESTORS))
                        {
                            pnf->SetHandler(ElementOwner());
                        }
                    }
                }
                break;

            case NTYPE_ELEMENT_REMEASURE:
                pnf->ChangeTo(NTYPE_ELEMENT_RESIZE, ElementOwner());
                break;

            case NTYPE_CLEAR_DIRTY:
                _fSizeThis = FALSE;
                break;

            case NTYPE_TRANSLATED_RANGE:
                Assert(pnf->IsDataValid());
                HandleTranslatedRange(pnf->DataAsSize());
                break;

            case NTYPE_ZPARENT_CHANGE:
                if (!ElementOwner()->IsPositionStatic())
                {
                    ElementOwner()->ZChangeElement();
                }
                else if (_fContainsRelative)
                {
                    ZChangeRelDispNodes();
                }
                break;

            case NTYPE_DISPLAY_CHANGE :
            case NTYPE_VISIBILITY_CHANGE:
                HandleVisibleChange(pnf->IsType(NTYPE_VISIBILITY_CHANGE));
                break;

            case NTYPE_ZERO_GRAY_CHANGE:
                HandleZeroGrayChange( pnf );
                break;

            default:
                if (IsInvalidationNotification(pnf))
                {
                    Invalidate();

                    // We've now handled the notification, so set the handler
                    // (so we can know not to keep sending it if SENDUNTILHANDLED
                    // is true.
                    pnf->SetHandler(ElementOwner());
                }
                break;
        }
    }

#if DBG==1
    // Update _snLast unless this is a self-only notification. Self-only
    // notification are an anachronism and delivered immediately, thus
    // breaking the usual order of notifications.
    if (!pnf->SendToSelfOnly() && pnf->SerialNumber() != (DWORD)-1)
    {
        _snLast = pnf->SerialNumber();
    }
#endif
}


//+----------------------------------------------------------------------------
//
//  Member:     GetAutoPosition
//
//  Synopsis:   Get the auto position of a given layout for which, this is the
//              z-parent
//
//  Arguments:  pLayout - Layout to position
//              ppt     - Returned top/left (in parent content relative coordinates)
//
//-----------------------------------------------------------------------------
void
CLayout::GetAutoPosition(
    CElement  *  pElement,
    CElement  *  pElementZParent,
    CDispNode ** ppDNZParent,
    CLayout   *  pLayoutParent,
    CPoint    *  ppt,
    BOOL         fAutoValid)
{
    CElement  * pElementLParent = pLayoutParent->ElementOwner();
    CDispNode * pDispNodeParent;

    Assert(pLayoutParent);
    Assert( ElementOwner()->IsScrollingParent()
        ||  !ElementOwner()->IsPositionStatic());

    //
    // get the inflow position relative to the layout parent, if the pt
    // passed in is not valid.
    //
    if(!fAutoValid)
    {
        pLayoutParent->GetPositionInFlow(pElement, ppt);

        //
        // GetPositionInFlow may have caused a recalc, which may have
        // replaced the dispnode. So, we need to grab the new dispnode ptr.
        //
        *ppDNZParent = pElementZParent->GetUpdatedNearestLayout()->GetElementDispNode(pElementZParent);
    }

    //
    // if the ZParent is ancestor of the LParent, then translate the point
    // to ZParent's coordinate system.
    // BUGBUG (srinib) - We are determining if ZParent is an ancesstor of
    // LParent here by comparing the source order. Searching the branch
    // could be cheaper
    //
    if(     pElementZParent == pElementLParent
        ||  pElementZParent->GetFirstCp() < pElementLParent->GetFirstCp())
    {
// BUGBUG - donmarsh when you have a routine to translate from one
// dispnode to another, please replace this code.(srinib)
        if(pElementZParent != pElementLParent)
        {
            pDispNodeParent = pLayoutParent->GetElementDispNode();

            while(  pDispNodeParent
                &&  pDispNodeParent != *ppDNZParent)
            {
                pDispNodeParent->TransformPoint(ppt, COORDSYS_CONTENT, COORDSYS_PARENT);
                pDispNodeParent = pDispNodeParent->GetParentNode();
            }
        }
    }
    else
    {
        CPoint pt = g_Zero.pt;

        Assert(pElementZParent->IsRelative() && !pElementZParent->NeedsLayout());

        pElementZParent->GetUpdatedParentLayout()->GetFlowPosition(*ppDNZParent, &pt);

        ppt->x -= pt.x;
        ppt->y -= pt.y;
    }

}


//+----------------------------------------------------------------------------
//
//  Member:     HandleVisibleChange
//
//  Synopsis:   Respond to a change in the display or visibility property
//
//-----------------------------------------------------------------------------

void
CLayout::HandleVisibleChange(BOOL fVisibility)
{
    CView *     pView        = Doc()->GetView();
    CElement *  pElement     = ElementOwner();
    CTreeNode * pTreeNode    = pElement->GetFirstBranch();
    HWND        hwnd         = pElement->GetHwnd();
    BOOL        fDisplayNone = pTreeNode->IsDisplayNone();
    BOOL        fHidden      = pTreeNode->IsVisibilityHidden();

    pView->OpenView();

    if(fVisibility)
    {
        EnsureDispNodeVisibility(VisibilityModeFromStyle(pTreeNode->GetCascadedvisibility()), pElement);
    }

    if (hwnd && Doc()->_pInPlace)
    {
        CDispNode * pDispNode = GetElementDispNode(pElement);
        CRect       rc;
        UINT        uFlags = (  !fDisplayNone
                            &&  !fHidden
                            &&  (   !pDispNode
                                ||  pDispNode->IsInView())
                                        ? SWP_SHOWWINDOW
                                        : SWP_HIDEWINDOW);

        ::GetWindowRect(hwnd, &rc);
        ::MapWindowPoints(HWND_DESKTOP, Doc()->_pInPlace->_hwnd, (POINT *)&rc, 2);
        pView->DeferSetWindowPos(hwnd, &rc, uFlags, NULL);
    }

    // Special stuff for OLE sites
    if (pElement->TestClassFlag(CElement::ELEMENTDESC_OLESITE))
    {
        COleSite *  pSiteOle    = DYNCAST(COleSite, pElement);

        if (fHidden || fDisplayNone)
        {
            // When an OCX without a hwnd goes invisible, we need to call
            // SetObjectsRects with -ve rect. This lets the control hide
            // any internal windows (IE5 #66118)
            if (!hwnd)
            {
                RECT rcNeg = { -1, -1, -1, -1 };

                DeferSetObjectRects(
                    pSiteOle->_pInPlaceObject, 
                    &rcNeg,
                    &g_Zero.rc,
                    NULL,
                    FALSE);
            }
        }
        else
        {
            //
            // transition up to at least baseline state if going visible.
            // Only do this if going visible cuz otherwise it causes
            // problems with the deskmovr. MikeSch has details. (anandra)
            //
    
            OLE_SERVER_STATE    stateBaseline   = pSiteOle->BaselineState(Doc()->State());
            if (pSiteOle->State() < stateBaseline)
            {
                pView->DeferTransition(pSiteOle);
            }
        }
    }

    if(!fVisibility)
    {
        if (!ElementOwner()->IsPositionStatic())
        {
            if (!fDisplayNone)
            {
                pElement->ZChangeElement();
            }
            else
            {
                CDispNode * pDispNode = GetElementDispNode();

                if (pDispNode)
                {
                    pDispNode->ExtractFromTree();
                }
            }
        }

        if (    !fDisplayNone
            &&  IsDirty()
            &&  !_fSizeThis)
        {
            PostLayoutRequest(LAYOUT_MEASURE);
        }
    }
}

//+====================================================================================
//
// Method: HandleZeroGrayChange
//
// Synopsis: The ZeroGrayBorder bit on the view has been toggled. We either flip on that 
//           we have a background or not on our dispnode accordingly.
//
//------------------------------------------------------------------------------------

VOID
CLayout::HandleZeroGrayChange( CNotification* pnf )
{
    BACKGROUNDINFO          bi;
    
    if ( GetView()->IsFlagSet(CView::VF_ZEROBORDER) )
    {
        if ( _pDispNode &&  
             ElementOwner()->_etag != ETAG_OBJECT &&
             ElementOwner()->_etag != ETAG_BODY ) // don't draw for Object tags - it may interfere with them
        {
            _pDispNode->SetBackground( TRUE );
        }
    }
    else
    {
        //
        // Only if we don't have a background will we clear the bit.
        //
        if ( _pDispNode )
        {
            GetBackgroundInfo(NULL, &bi, FALSE);

            BOOL fHasBack = (bi.crBack != COLORREF_NONE || bi.pImgCtx);
            if ( ! fHasBack )
                _pDispNode->SetBackground( FALSE );
        }            
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     HandleElementMeasureRequest
//
//  Synopsis:   Respond to a request to measure an absolutely positioned element
//              NOTE: Due to property changes, it is possible for the element
//                    to no longer be absolutely positioned by the time the
//                    request is handled
//
//  Arguments:  pci        - CCalcInfo to use
//              pElement   - Element to position
//              fEditable  - TRUE the element associated with this layout is editable,
//                           FALSE otherwise
//
//-----------------------------------------------------------------------------

void
CLayout::HandleElementMeasureRequest(
    CCalcInfo * pci,
    CElement *  pElement,
    BOOL        fEditable)
{
    Assert(pci);
    Assert(pElement);

    CTreeNode *     pTreeNode = pElement->GetFirstBranch();
    CLayout *       pLayout   = pElement->GetUpdatedLayout();
    CNotification   nf;

    if (    pLayout
        &&  pTreeNode->GetCascadedposition() == stylePositionabsolute)
    {
        Assert(pElement->GetUpdatedParentLayout() == this);

        if (    !pTreeNode->IsDisplayNone()
            ||  fEditable)
        {
            CalcAbsolutePosChild(pci, pLayout);

            nf.ElementSizechanged(pElement);
            GetView()->Notify(&nf);

            pElement->ZChangeElement();
        }

        else
        {
            nf.ClearDirty(ElementOwner());
            ElementOwner()->Notify(&nf);

            if (    pLayout
                &&  pLayout->GetElementDispNode())
            {
                pLayout->GetElementDispNode()->ExtractFromTree();
            }
        }

        // if this absolute element, has a height specified in 
        // percentages, we need a flag to be set the parent flowlayout
        // in order for this resized during a vertical-only resize
        if (IsFlowLayout())
        {
           const CFancyFormat *pFF=pTreeNode->GetFancyFormat();

            if (pFF->_fHeightPercent)
                DYNCAST(CFlowLayout, this)->_fChildHeightPercent = TRUE;

            if (pFF->_fWidthPercent)
                DYNCAST(CFlowLayout, this)->_fChildWidthPercent = TRUE;
        }
    }
}


void 
CLayout::CalcAbsolutePosChild(CCalcInfo *pci, CLayout *pChildLayout)
{
    CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
    SIZE    sizeLayout = pci->_sizeParent;

    pChildLayout->CalcSize(pci, &sizeLayout);
    
    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     HandlePositionNotification/Request
//
//  Synopsis:   Respond to a z-order or position change notification
//
//  Arguments:  pElement   - Element to position
//              ptAuto     - Top/Left values for "auto"
//              fAutoValid - TRUE if ptAuto is valid, FALSE otherwise
//
//  Returns:    TRUE if handled, FALSE otherwise
//
//-----------------------------------------------------------------------------

BOOL
CLayout::HandlePositionNotification(CNotification * pnf)
{
// BUGBUG: Handle if z-parent or _fContainsRelative (brendand)
    BOOL    fHandle = ElementOwner()->IsZParent();

    if (fHandle)
    {
        if (    !TestLock(CElement::ELEMENTLOCK_PROCESSREQUESTS)
            ||  TestLock(CElement::ELEMENTLOCK_PROCESSMEASURE))
        {
            CRequest *  pRequest = QueueRequest(CRequest::RF_POSITION, pnf->Element());

            if (pRequest)
            {
                pRequest->SetAuto(pnf->DataAsPoint(), pnf->IsDataValid());
            }
        }
        else
        {
            CRequest * pRequest = pnf->Element()->GetRequestPtr();

            if(!pRequest || !pRequest->IsFlagSet(CRequest::RF_POSITION))
            {
                HandlePositionRequest(pnf->Element(), pnf->DataAsPoint(), pnf->IsDataValid());
            }
            else if (pRequest && pRequest->IsFlagSet(CRequest::RF_AUTOVALID))
            {
                pRequest->ClearFlag(CRequest::RF_AUTOVALID);
            }
        }
    }

    return fHandle;
}

void
CLayout::HandlePositionRequest(
    CElement *      pElement,
    const CPoint &  ptAuto,
    BOOL            fAutoValid)
{

// BUGBUG: Handle if z-parent or _fContainsRelative (brendand)
    Assert(ElementOwner()->IsZParent());
    Assert(!TestLock(CElement::ELEMENTLOCK_SIZING));
    Assert(GetElementDispNode());
    Assert(pElement->GetFirstBranch());
#if DBG==1
    {
        long    cp  = pElement->GetFirstCp() - GetContentFirstCp();
        long    cch = pElement->GetElementCch();

        Assert( !IsDirty()
            ||  (   IsFlowLayout()
                &&  DYNCAST(CFlowLayout, this)->IsRangeBeforeDirty(cp, cch)));
    }
#endif

          CTreeNode    * pTreeNode = pElement->GetFirstBranch();
    const CFancyFormat * pFF = pTreeNode->GetFancyFormat();
    const CCharFormat  * pCF = pTreeNode->GetCharFormat();
          BOOL           fRelative = pFF->IsRelative();
          BOOL           fAbsolute = pFF->IsAbsolute();

    //
    // Layouts inside relative positioned elements do not know their
    // relative position untill the parent is measured. So, they fire
    // a z-change notification to get positioned into the tree.
    // fRelative is different from pCF->_fRelative, fRelative means
    // is the current element relative. pCF->_fRelative is true if
    // an element is inheriting relativeness from an ancestor the
    // does not have layoutness (an image inside a relative span,
    // if the span has layoutness then the image does not inherit
    // relativeness).
    //
    if (    !IsDisplayNone()
        &&  !pCF->IsDisplayNone()
        &&  (fAbsolute || pCF->_fRelative || fRelative))
    {
        // BUGBUG: Re-write this: If _fContainsRelative, then get the element z-parent and its
        //         display node. Then use that node as the parent for all other processing.
        //         (brendand)
        CElement *  pElementOwner   = ElementOwner();
        CTreeNode * pTreeNode       = pElement->GetFirstBranch();
        CLayout   * pLayout         = pElement->GetUpdatedNearestLayout();
        CElement  * pElementZParent = pTreeNode->ZParent();
        CLayout   * pLayoutZParent  = pElementZParent->GetUpdatedNearestLayout();
        CDispNode * pDNElement;
        CDispNode * pDNZParent;

        pDNZParent = pLayoutZParent->EnsureDispNodeIsContainer(pElementZParent);

        if (pDNZParent)
        {
            Assert(pLayout);

            pDNElement = pLayout->GetElementDispNode(pElement);

            Assert(pDNZParent->IsContainer());

            if(pDNElement)
            {
                CDoc *          pDoc          = pElementOwner->Doc();
                BOOL            fLeftAuto     = pFF->IsLeftAuto();
                BOOL            fRightAuto    = pFF->IsRightAuto();
                BOOL            fTopAuto      = pFF->IsTopAuto();
                BOOL            fBottomAuto   = pFF->IsBottomAuto();
                BOOL            fLeftPercent  = pFF->_cuvLeft.GetUnitType() == CUnitValue::UNIT_PERCENT;
                BOOL            fTopPercent   = pFF->_cuvTop.GetUnitType() == CUnitValue::UNIT_PERCENT;
                CLayout       * pLayoutParent = pElement->GetUpdatedParentLayout();
                long            lFontHeight   = pTreeNode->GetCharFormat()->GetHeightInTwips(Doc());
                long            xLeftMargin   = 0;
                long            yTopMargin    = 0;
                long            xRightMargin  = 0;
                long            yBottomMargin = 0;
                BOOL            fRTLParent;
                CPoint          pt(g_Zero.pt);
                CRect           rc(g_Zero.rc);
                CSize size;

                CElement  * pParent         = pLayoutParent->ElementOwner();

                if ( pParent->Tag() == ETAG_TR )
                {
                    if (pElement->Tag() == ETAG_TD || pElement->Tag() == ETAG_TH)
                    {
                        if (pParent->IsPositionStatic())  // if row itself is not postioned
                        {
                            // the parent layout should be the table
                            pLayoutParent = pParent->GetUpdatedParentLayout();
                        }
                    }
                }

                fRTLParent = pLayoutParent->_pDispNode->IsRightToLeft();

                if (    !fAbsolute
                    ||  fLeftAuto
                    ||  fTopAuto)
                {
                    pt = ptAuto;

                    GetAutoPosition(pElement, pElementZParent, &pDNZParent, pLayoutParent, &pt, fAutoValid);

                    //
                    // Get auto position may have caused a calc which might result in
                    // morphing the display node for the current element.
                    //
                    pDNElement = pLayout->GetElementDispNode(pElement);
                }

                pDNElement->GetSize(&size);

                //
                // if the we are positioning an absolute element with top/left specified
                // then clear the auto values.
                //
                if (fAbsolute)
                {
                    if (!fLeftAuto || !fRightAuto)
                        pt.x = 0;

                    if (!fTopAuto)
                        pt.y = 0;

                    //
                    // account for margins on absolute elements.
                    // Margins is already added onto relatively positioned
                    // elements. Due to the algorithm we need to have
                    // margin info available for absolute in opposite flows.
                    if (pLayout->ElementOwner() == pElement)
                    {
                        CCalcInfo CI(&pDoc->_dci, pLayoutParent);

                        pLayout->GetMarginInfo(&CI,
                                               &xLeftMargin,
                                               &yTopMargin,
                                               &xRightMargin,
                                               &yBottomMargin);

                        if(!fRTLParent)
                            pt.x += xLeftMargin;
                        else
                            pt.x -= xRightMargin;
                        pt.y += yTopMargin;
                    }
                }

                //
                // if the element is positioned, compute the top/left based
                // on the top/left/right/bottom styles specified for the element
                //
                if (pFF->IsPositioned())
                {
                    //
                    //  Get the client rectangle used for percent and auto positioning
                    //  NOTE: Sanitize the rectangle so that each direction is even
                    //        (The extra pixel is given to the size of the object over the location)
                    //
                    if (    fLeftPercent
                        ||  fTopPercent
                        || !fRightAuto
                        || !fBottomAuto
                        ||  fRTLParent)
                    {
                        if (fRelative)
                            pLayoutParent->GetClientRect(&rc);
                        else
                            pLayoutZParent->GetClientRect(&rc);

                        rc.right  &= ~0x00000001;
                        rc.bottom &= ~0x00000001;
                    }

                    if(!fRTLParent)
                    {

                        // is there is no left offsetting the right offset
                        if(!fLeftAuto)
                        {
                            // adjust left position
                            pt.x += pFF->_cuvLeft.XGetPixelValue(&pDoc->_dci, rc.Width(), lFontHeight);
                        }
                        else if(!fRightAuto)
                        {
                            if(fRelative)
                            {
                                // adjust the relative position in the flow
                                // kind of redundant to get the right x and then get
                                // the left again so just adjust the left
                                pt.x -= pFF->_cuvRight.XGetPixelValue(&pDoc->_dci, rc.Width(), lFontHeight);
                            }
                            else
                            {
                                pt.x = rc.right;
                                pt.x -= pFF->_cuvRight.XGetPixelValue(&pDoc->_dci, rc.Width(), lFontHeight);
                                pt.x -= xRightMargin;
                                //place the top/left now
                                pt.x -= size.cx;
                            }
                        }
                    }
                    else
                    {
                        // We are right to left. Move pt.x to left side. We will position top/left
                        pt.x -= size.cx;

                        if(!fRightAuto)
                        {
                            // adjust right position
                            pt.x -= pFF->_cuvRight.XGetPixelValue(&pDoc->_dci, rc.Width(), lFontHeight);
                        }
                        else if(!fLeftAuto)
                        {

                            if(fRelative)
                            {
                                // adjust the relative position in the flow
                                pt.x += pFF->_cuvLeft.XGetPixelValue(&pDoc->_dci, rc.Width(), lFontHeight);
                            }
                            else
                            {
                                pt.x = rc.left;
                                pt.x += pFF->_cuvLeft.XGetPixelValue(&pDoc->_dci, rc.Width(), lFontHeight);
                                pt.x += xLeftMargin;
                            }
                        }
                    }

                    // adjust top position
                    pt.y += pFF->_cuvTop.YGetPixelValue(&pDoc->_dci, rc.Height(), lFontHeight);

                    if(!fBottomAuto)
                    {

                        // It is possible that we are overconstrained. Give top priority and
                        // don't go in here
                        if(fTopAuto)
                        {
                            if(fRelative)
                            {
                                // adjust the relative position in the flow
                                pt.y -= pFF->_cuvBottom.YGetPixelValue(&pDoc->_dci, rc.Height(), lFontHeight);
                            }
                            else
                            {
                                pt.y = rc.bottom;
                                pt.y -= pFF->_cuvBottom.YGetPixelValue(&pDoc->_dci, rc.Height(), lFontHeight);
                                pt.y -= yBottomMargin;
                                pt.y -= size.cy;
                            }
                        }
                    }

                    // BUGBUG: Layer type should be set when the node is created.... (brendand)

                    pDNElement->SetLayerType(pFF->_lZIndex >= 0
                                                    ? DISPNODELAYER_POSITIVEZ
                                                    : DISPNODELAYER_NEGATIVEZ);
                    DYNCAST(CDispContainer, pDNZParent)->InsertChildInZLayer(pDNElement, pFF->_lZIndex);
                }
                else
                {
                    DYNCAST(CDispContainer, pDNZParent)->InsertChildInFlow(pDNElement);
                }

                if (pElement->NeedsLayout())
                {
                    pLayout->SetPosition(pt);

                    if (pFF->IsPositioned())
                    {
                        CNotification   nf;

                        nf.ElementPositionchanged(pElement);
                        GetView()->Notify(&nf);
                    }
                }
                else
                    pDNElement->SetPosition(pt);
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     HandleAddAdornerNotification/Request
//
//  Synopsis:   Insert the display node for a adorner
//
//  Arguments:  pAdorner - CAdorner whose display node is to be inserted
//
//-----------------------------------------------------------------------------

BOOL
CLayout::HandleAddAdornerNotification(
    CNotification * pnf)
{
    CElement *  pElement  = ElementOwner();
    CTreeNode * pTreeNode = pElement->GetFirstBranch();
    BOOL        fHandle   = FALSE;

    //
    //  Adorners for BODYs or FRAMESETs are always anchored at the absolute top of that element
    //  (since there is no parent element in the current design under which to anchor them)
    //

    if (pnf->Element() == pElement)
    {
        if (    pElement->Tag() == ETAG_BODY
            ||  pElement->Tag() == ETAG_FRAMESET)
        {
            Assert(pnf->DataAsPtr());
            DYNCAST(CAdorner, (CAdorner *)pnf->DataAsPtr())->GetDispNode()->SetExtraCookie((void *)ADL_ALWAYSONTOP);
            fHandle = TRUE;
        }
    }

    //
    //  Adorners are handled by the layout that owns the display node under which they are to be anchored
    //  This divides into several cases:
    //
    //      1) Adorners anchored at the "absolute" top are anchored under the first scrolling or
    //         filtered parent (since they mark a logical top within the display tree)
    //
    //      2) Adorners anchored on the element itself are anchored under the positioned parent
    //         (since that is the same parent under which the associated element is anchored)
    //         NOTE: Adorners anchored on an element are currently reserved for positioned elements -
    //               If that changes, then these rules will need appropriate modification
    //
    //      3) Adorners for the flow layer are anchored under either the first scrolling
    //         parent, first filtered parent, or first positioned parent found
    //         (since they establish the nearest flow layer)
    //
    //  These rules are modified slightly when the positioned parent does not have its own layout (such
    //  as with a relatively positioned element). In that case, while the adorner is still anchored under
    //  the positioned parent, it is the nearest layout of that positioned parent which handles the
    //  request
    //

    else
    {
        //
        //  Determine if this layout is at a logical top or is a z-parent
        //
        //      1) All scrolling and filtered elements mark a logical top
        //      2) The layout is (logically) a z-parent if it is a logical top,
        //         is itself a z-parent, or is the nearest layout to the actual
        //         z-parent
        //

        BOOL    fAtTop    = pTreeNode->IsScrollingParent()
                        ||  pTreeNode->HasFilterPtr();
        BOOL    fZParent  = fAtTop
                        ||  pTreeNode->IsZParent()
                        ||  (   (   !pnf->Element()->IsPositionStatic()
                                ||  pnf->Element()->IsInheritingRelativeness())
                            &&  pnf->Element()->GetFirstBranch()->ZParent()->GetUpdatedNearestLayout() == this);

        if (    fAtTop
            ||  fZParent)
        {
            CAdorner *      pAdorner  = DYNCAST(CAdorner, (CAdorner *)pnf->DataAsPtr());
            CDispNode *     pDispNode = pAdorner->GetDispNode();
            ADORNERLAYER    adl       = (ADORNERLAYER)(DWORD)(DWORD_PTR)pDispNode->GetExtraCookie();

            Assert( adl != ADL_TOPOFFLOW
                ||  pAdorner->GetElement()->IsPositionStatic());

            fHandle =   (   adl == ADL_ALWAYSONTOP
                        &&  fAtTop)
                    ||  (   adl == ADL_ONELEMENT
                        &&  fZParent)
                    ||  adl == ADL_TOPOFFLOW;
        }
    }

    if (fHandle)
    {
        if (!TestLock(CElement::ELEMENTLOCK_PROCESSREQUESTS))
        {
            CRequest *  pRequest = QueueRequest(CRequest::RF_ADDADORNER, pnf->Element());

            if (pRequest)
            {
                pRequest->SetAdorner(DYNCAST(CAdorner, (CAdorner *)pnf->DataAsPtr()));
            }
        }
        else
        {
            HandleAddAdornerRequest(DYNCAST(CAdorner, (CAdorner *)pnf->DataAsPtr()));
        }
    }

    return fHandle;
}

void
CLayout::HandleAddAdornerRequest(
    CAdorner *  pAdorner)
{
    Assert(pAdorner);
    Assert(pAdorner->GetElement());
    Assert(!TestLock(CElement::ELEMENTLOCK_SIZING));
    Assert(GetElementDispNode());
    Assert(TestLock(CElement::ELEMENTLOCK_PROCESSREQUESTS));
#if DBG==1
    {
        long    cp  = pAdorner->GetElement()->GetFirstCp() - GetContentFirstCp();
        long    cch = pAdorner->GetElement()->GetElementCch();

        Assert( !IsDirty()
            ||  (   IsFlowLayout()
                &&  DYNCAST(CFlowLayout, this)->IsRangeBeforeDirty(cp, cch)));
    }
#endif

    CDispNode * pDispAdorner = pAdorner->GetDispNode();
    CElement *  pElement     = pAdorner->GetElement();

    if (pDispAdorner)
    {
        switch ((ADORNERLAYER)(DWORD)(DWORD_PTR)pDispAdorner->GetExtraCookie())
        {

        //
        //  Anchor "always on top" adorners as the last node(s) of the current postive-z layer
        //

        default:
        case ADL_ALWAYSONTOP:
            {
                CDispInteriorNode * pDispParent = EnsureDispNodeIsContainer();
                CDispNode *         pDispNode   = pDispParent->GetLastChildNodeInLayer(DISPNODELAYER_POSITIVEZ);

                if (pDispNode)
                {
                    pDispNode->InsertNextSiblingNode(pDispAdorner);
                }
                else
                {
                    Assert (pDispParent);
                    pDispParent->InsertChildInZLayer(pDispAdorner, pAdorner->GetZOrderForSelf());
                }
            }
            break;

        //
        //  Anchor adorners that sit on the element as the element's next sibling
        //

        case ADL_ONELEMENT:
            {
                Assert(pElement->GetUpdatedNearestLayout());
                Assert(pElement->GetUpdatedNearestLayout()->GetElementDispNode(pElement));

                CLayout *   pLayout   = pElement->GetUpdatedNearestLayout();
                CDispNode * pDispNode = pLayout->GetElementDispNode(pElement);

                if (pDispNode)
                {
                    Assert(pDispNode->GetParentNode());

                    pDispNode->InsertNextSiblingNode(pDispAdorner);
                }
            }
            break;

        //
        //  Anchor "top of flow" adorners in the flow layer of this layout or
        //  the element's positioned z-parent, whichever is closer
        //  (In either case, this layout "owns" the display node under which the
        //   adorner is added)
        //

        case ADL_TOPOFFLOW:
            {
                CElement *          pElementParent = pElement->GetFirstBranch()->ZParent();
                CDispInteriorNode * pDispParent;

                Assert( pElementParent == ElementOwner()
                    ||  pElementParent->IsPositionStatic()
                    ||  (   pElementParent->GetFirstBranch()->GetCascadedposition() == stylePositionrelative
                        &&  !pElementParent->GetUpdatedLayout()
                        &&  pElementParent->GetUpdatedNearestLayout() == this));

                if (pElementParent->IsPositionStatic())
                {
                    pElementParent = ElementOwner();
                }

                pDispParent = EnsureDispNodeIsContainer(pElementParent);

                if (pDispParent)
                {
                    pDispParent->InsertChildInFlow(pDispAdorner);
                }
            }
            break;
        }

        pAdorner->PositionChanged();
    }
}

HRESULT
CLayout::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr = S_OK;;

    switch(dispid)
    {
    case DISPID_UNKNOWN:
    case DISPID_CElement_className:
    case DISPID_A_BACKGROUNDIMAGE:
    case DISPID_BACKCOLOR:
        EnsureDispNodeBackground();
        break;
    }

    RRETURN(hr);
}

CMarkup *
CLayout::GetContentMarkup() const
{
    if (ElementOwner()->HasSlaveMarkupPtr())
    {
        return ElementOwner()->GetSlaveMarkupPtr();
    }

    // get the owner's markup
    Assert( !_fDetached );
    Assert( _pMarkupDbg == ( _fHasMarkupPtr ? (CMarkup*)__pvChain : NULL ) );
    return _fHasMarkupPtr ? (CMarkup*)__pvChain : NULL;
}

CElement *
CLayout::ElementContent() const
{
    if (ElementOwner()->HasSlaveMarkupPtr())
    {
        return ElementOwner()->GetSlaveMarkupPtr()->FirstElement();
    }
    return ElementOwner();
}


//+---------------------------------------------------------------------------
//
//  Member:     GetContentTextLength
//
//  Synopsis:   Return the number of charaters owned by the element associated
//              with the layout
//
//----------------------------------------------------------------------------

long
CLayout::GetContentTextLength()
{
    if (ElementOwner()->HasSlaveMarkupPtr())
    {
        return ElementOwner()->GetSlaveMarkupPtr()->GetContentTextLength();
    }
    else
    {
        return ElementOwner()->GetElementCch();
    }
}

//
// get the first cp under the influence of the layout element
//
long
CLayout::GetContentFirstCp()
{
    if (ElementOwner()->HasSlaveMarkupPtr())
    {
        return ElementOwner()->GetSlaveMarkupPtr()->GetContentFirstCp();
    }
    else
    {
        return ElementOwner()->GetFirstCp();
    }
}

//
// get the last cp under the influence of  the layout element
//

long
CLayout::GetContentLastCp()
{
    if (ElementOwner()->HasSlaveMarkupPtr())
    {
        return ElementOwner()->GetSlaveMarkupPtr()->GetContentLastCp();
    }
    else
    {
        return ElementOwner()->GetLastCp();
    }
}

void
CLayout::GetContentTreeExtent(CTreePos ** pptpStart, CTreePos ** pptpEnd)
{
    if (ElementOwner()->HasSlaveMarkupPtr())
    {
        ElementOwner()->GetSlaveMarkupPtr()->GetContentTreeExtent(pptpStart, pptpEnd);
    }
    else
    {
        ElementOwner()->GetTreeExtent(pptpStart, pptpEnd);
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     RegionFromElement
//
//  Synopsis:   Return the bounding rectangle for an element, if the element is
//              this instance's owner. The RECT returned is in client coordinates.
//
//  Arguments:  pElement - pointer to the element
//              CDataAry<RECT> *  - rectangle array to contain
//              dwflags - flags define the type of changes required
//              (CLEARFORMATS) etc.
//
//-----------------------------------------------------------------------------
void
CLayout::RegionFromElement( CElement * pElement,
                            CDataAry<RECT> * paryRects,
                            RECT * prcBound,
                            DWORD  dwFlags)
{
    Assert( pElement && paryRects);

    if (!pElement || !paryRects)
        return;

    // if the element passed is the element that owns this instance,
    if ( _pElementOwner == pElement )
    {
        CRect rect;

        if (!prcBound)
        {
            prcBound = &rect;
        }

        // If the element is not shown, bounding rectangle is all zeros.
        if ( pElement->IsDisplayNone() )
        {
            // return (0,0,0,0) if the display is set to 'none'
            *prcBound = g_Zero.rc;
        }
        else
        {
            // return the rectangle that this CLayout covers
            GetRect( prcBound, dwFlags & RFE_SCREENCOORD
                                ? COORDSYS_GLOBAL
                                : COORDSYS_PARENT);
        }

        paryRects->AppendIndirect(prcBound);
    }
    else
    {
        // we should not reach here, since anything that can have children
        // is actually a flow layout since the CLayout does not know how
        // to wrap text.
        AssertSz( FALSE, "CLayout::RegionFromElement should not be called");
   }
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::NotifyMeasuredRange
//
//  Synopsis:   Send a measured range notification
//
//  Arguments:  cpStart - Starting cp of range
//              cpEnd   - Ending cp of range
//
//----------------------------------------------------------------------------

void
CLayout::NotifyMeasuredRange(
    long    cpStart,
    long    cpEnd)
{
    CNotification   nf;

    Assert( cpStart >= 0
        &&  cpEnd   >= 0);

    nf.MeasuredRange(cpStart, cpEnd - cpStart);

    GetView()->Notify(&nf);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::NotifyTranslatedRange
//
//  Synopsis:   Send a translated range notification
//
//  Arguments:  size    - Size of translation
//              cpStart - Starting cp of range
//              cpEnd   - Ending cp of range
//
//----------------------------------------------------------------------------

void
CLayout::NotifyTranslatedRange(
    const CSize &   size,
    long            cpStart,
    long            cpEnd)
{
    CNotification   nf;

    Assert( cpStart >= 0
        &&  cpEnd   >= 0);
    Assert(cpEnd >= cpStart);

    nf.TranslatedRange(cpStart, cpEnd - cpStart);
    nf.SetData(size);

    if (_fAutoBelow)
    {
        Assert(GetContentMarkup());
        GetContentMarkup()->Notify(&nf);
    }

    GetView()->Notify(&nf);
}


//+---------------------------------------------------------------------------
//
//  Member:     CLayout::ComponentFromPoint
//
//  Synopsis:   Return the component hit by this point.
//
//  Arguments:  x,y     coordinates of point
//
//  Returns:    the component that was hit
//
//  Notes:
//
//----------------------------------------------------------------------------

_htmlComponent
CLayout::ComponentFromPoint(long x, long y)
{
    if (_pDispNode)
    {
        CPoint pt(x,y);
        _pDispNode->TransformPoint(
            &pt,
            COORDSYS_GLOBAL,
            COORDSYS_CONTAINER);

        CRect rcContainer(_pDispNode->GetBounds().Size());
        if (rcContainer.Contains(pt))
        {
            if (_pDispNode->IsScroller())
            {
                CRect rcScrollbar;
                CScrollbar::SCROLLBARPART part;
                CDispScroller* pScroller = DYNCAST(CDispScroller, _pDispNode);
                const CSize& sizeContent = pScroller->GetContentSize();
                CSize sizeOffset;
                pScroller->GetScrollOffset(&sizeOffset);

                CFormDrawInfo DI;
                DI.Init(this);

                // check vertical scrollbar
                pScroller->GetClientRect(&rcScrollbar, CLIENTRECT_VSCROLLBAR);
                part = CScrollbar::GetPart(
                    1,
                    rcScrollbar,
                    pt,
                    sizeContent.cy,
                    rcScrollbar.Height(),
                    sizeOffset.cy,
                    rcScrollbar.Width(),
                    &DI,
                    FALSE); // VSCROLL is never has RTL concerns

                switch (part)
                {
                case CScrollbar::SB_PREVBUTTON: return htmlComponentSbUp;
                case CScrollbar::SB_NEXTBUTTON: return htmlComponentSbDown;
                case CScrollbar::SB_PREVTRACK:  return htmlComponentSbPageUp;
                case CScrollbar::SB_NEXTTRACK:  return htmlComponentSbPageDown;
                case CScrollbar::SB_THUMB:      return htmlComponentSbVThumb;
                }

                // check horizontal scrollbar
                pScroller->GetClientRect(&rcScrollbar, CLIENTRECT_HSCROLLBAR);
                part = CScrollbar::GetPart(
                    0,
                    rcScrollbar,
                    pt,
                    sizeContent.cx,
                    rcScrollbar.Width(),
                    sizeOffset.cx,
                    rcScrollbar.Height(),
                    &DI,
                    pScroller->IsRightToLeft());

                switch (part)
                {
                case CScrollbar::SB_PREVBUTTON: return htmlComponentSbLeft;
                case CScrollbar::SB_NEXTBUTTON: return htmlComponentSbRight;
                case CScrollbar::SB_PREVTRACK:  return htmlComponentSbPageLeft;
                case CScrollbar::SB_NEXTTRACK:  return htmlComponentSbPageRight;
                case CScrollbar::SB_THUMB:      return htmlComponentSbHThumb;
                }
            }

            return htmlComponentClient;
        }
    }

    return htmlComponentOutside;
}

//+====================================================================================
//
// Method: ShowSelected
//
// Synopsis: The "selected-ness" of this layout has changed. We need to set it's
//           properties, and invalidate it.
//
//------------------------------------------------------------------------------------

VOID
CLayout::ShowSelected( 
                        CTreePos* ptpStart, 
                        CTreePos* ptpEnd, 
                        BOOL fSelected, 
                        BOOL fLayoutCompletelyEnclosed ,
                        BOOL fFireOM )
{
    SetSelected( fSelected, TRUE );
}

//+====================================================================================
//
// Method:  SetSelected
//
// Synopsis:Set the Text Selected ness of the layout
//
//------------------------------------------------------------------------------------


VOID
CLayout::SetSelected( BOOL fSelected , BOOL fInvalidate )
{
    // select the element
    const CCharFormat *pCF = GetFirstBranch()->GetCharFormat();

    // Set the site text selected bits appropriately
    SetSiteTextSelection (
        fSelected,
        pCF->SwapSelectionColors());

    if ( fInvalidate )
        Invalidate();
}


CDispFilter *
CLayout::GetFilter()
{
    if (ElementOwner()->HasFilterPtr())
        return (ElementOwner()->GetFilterPtr()->GetDispFilter());
    else
        return NULL;
}

//+====================================================================================
//
// Method:  GetParentTopLeft
//
// Synopsis:Get the top left coordinate of the parent disp node
//
//------------------------------------------------------------------------------------
void
CLayout::GetParentTopLeft(CPoint * ppt)
{
    Assert(ppt);

    *ppt = g_Zero.pt;

    CDispNode * pDispNode = GetElementDispNode();
    CDispNode * pParentNode = NULL;

    if(pDispNode)
    {
        pParentNode = pDispNode->GetParentNode();

        if(pParentNode)
        {
            *ppt = pParentNode->GetPosition();
            pParentNode->TransformPoint(ppt, COORDSYS_PARENT, COORDSYS_CONTENT);
        }
    }
}


//+====================================================================================
//
// Method:  GetContentRect
//
// Synopsis:Get the content rect in the specified coordinate system
//
//------------------------------------------------------------------------------------
void    
CLayout::GetContentRect(CRect *prc, COORDINATE_SYSTEM cs) const
{
    CPoint ptTopLeft(0,0);
    CSize  sizeLayout;
    RECT   rcClient;
    CDispNode * pDispNode;

    TransformPoint(&ptTopLeft, COORDSYS_CONTENT, cs);
    GetContentSize(&sizeLayout);
    pDispNode = GetElementDispNode();

    if (pDispNode && !pDispNode->IsRightToLeft())
    {
        prc->left   = ptTopLeft.x;
        prc->right  = prc->left + sizeLayout.cx;
    }
    else
    {
        prc->right  = ptTopLeft.x + 1;
        prc->left   = prc->right - sizeLayout.cx;
    }
    prc->top    = ptTopLeft.y;
    prc->bottom = prc->top + sizeLayout.cy;

    GetClientRect(&rcClient, cs);
    prc->Union(rcClient);
}


