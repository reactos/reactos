//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       qflowlyt.cxx
//
//  Contents:   Implementation of CFlowLayout and related classes.
//
//  This is the version customized for hosting Quill.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_QUILGLUE_HXX_
#define X_QUILGLUE_HXX_
#include "quilglue.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef _X_SELDRAG_HXX_
#define _X_SELDRAG_HXX_
#include "seldrag.hxx"
#endif

#ifndef X_LSM_HXX_
#define X_LSM_HXX_
#include "lsm.hxx"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X_DOCPRINT_HXX_
#define X_DOCPRINT_HXX_
#include "docprint.hxx" // CPrintPage
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"   // CPrintDoc
#endif

#ifndef X_SBBASE_HXX_
#define X_SBBASE_HXX_
#include "sbbase.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X__ELABEL_HXX_
#define X__ELABEL_HXX_
#include "elabel.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPITEM_HXX_
#define X_DISPITEM_HXX_
#include "dispitem.hxx"
#endif

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_FILTER_HXX_
#define X_FILTER_HXX_
#include "filter.hxx"
#endif

#ifndef X_ADORNER_HXX_
#define X_ADORNER_HXX_
#include "adorner.hxx"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#define DO_PROFILE  0

#if DO_PROFILE==1
#include "icapexp.h"
#endif

// force functions to load through dynamic wrappers
//

#ifndef WIN16
#ifdef WINCOMMCTRLAPI
#ifndef X_COMCTRLP_H_
#define X_COMCTRLP_H_
#undef WINCOMMCTRLAPI
#define WINCOMMCTRLAPI
#include "comctrlp.h"
#endif
#endif
#endif // ndef WIN16

MtDefine(CFlowLayout, Layout, "CFlowLayout")
MtDefine(CFlowLayout_aryLayouts_pv, CFlowLayout, "CFlowLayout::__aryLayouts::_pv")
MtDefine(CFlowLayout_pDropTargetSelInfo, CFlowLayout, "CFlowLayout::_pDropTargetSelInfo")
MtDefine(CFlowLayoutScrollRangeInfoView_aryRects_pv, Locals, "CFlowLayout::ScrollRangeIntoView aryRects::_pv")
MtDefine(CFlowLayoutBranchFromPointEx_aryRects_pv, Locals, "CFlowLayout::BranchFromPointEx aryRects::_pv")
MtDefine(CFlowLayoutDrop_aryLayouts_pv, Locals, "CFlowLayout::Drop aryLayouts::_pv")
MtDefine(CFlowLayoutGetChildElementTopLeft_aryRects_pv, Locals, "CFlowLayout::GetChildElementTopLeft aryRects::_pv")
MtDefine(CFlowLayoutPaginate_aryValues_pv, Locals, "CFlowLayout::Paginate aryValues::_pv")
MtDefine(CFlowLayoutNotify_aryRects_pv, Locals, "CFlowLayout::Notify aryRects::_pv")

MtDefine(CStackPageBreaks, CFlowLayout, "CStackPageBreaks")
MtDefine(CStackPageBreaks_aryYPos_pv, CStackPageBreaks, "CStackPageBreaks::_aryYPos::_pv")
MtDefine(CStackPageBreaks_aryXWidthSplit_pv, CStackPageBreaks, "CStackPageBreaks::_aryXWidthSplit::_pv")

ExternTag(tagCalcSize);

DeclareTag(tagUpdateDragFeedback, "Selection", "Update Drag Feedback")
DeclareTag(tagNotifyText, "NotifyText", "Trace text notifications");
DeclareTag(tagRepeatHeaderFooter, "Print", "Repeat table headers and footers on pages");


//+----------------------------------------------------------------------------
//
//  Member:     Listen
//
//  Synopsis:   Listen/stop listening to notifications
//
//  Arguments:  fListen - TRUE to listen, FALSE otherwise
//
//-----------------------------------------------------------------------------
void
CFlowLayout::Listen(
    BOOL    fListen)
{
    if ((unsigned)fListen != _fListen)
    {
        if (_fListen)
        {
            Reset();
        }

        _fListen = (unsigned)fListen;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     IsListening
//
//  Synopsis:   Return TRUE if accepting text notifications
//              NOTE: Make this inline! (brendand)
//
//-----------------------------------------------------------------------------
BOOL
CFlowLayout::IsListening()
{
    return !!_fListen;
}


//-----------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Respond to a tree notification
//
//  Arguments:  pnf - Pointer to the tree notification
//
//-----------------------------------------------------------------------------
void
CFlowLayout::Notify(
    CNotification * pnf)
{
    Assert(!pnf->IsReceived(_snLast));

    BOOL    fHandle = TRUE;
    DWORD   dwData;
    HRESULT hr      = S_OK;

    //
    //  Respond to the notification if:
    //      a) The channel is enabled
    //      b) The text is not sizing
    //      c) Or it is a position/z-change notification
    //

    if (    IsListening()
        && (    !TestLock(CElement::ELEMENTLOCK_SIZING)
            ||  IsPositionNotification(pnf)))
    {
        BOOL    fRangeSet = FALSE;

        //
        //  For notifications originating from this element,
        //  set the range to the content rather than that in the container (if they differ)
        //

        if (    pnf->Element() == ElementOwner()
            &&  ElementOwner()->HasSlaveMarkupPtr())
        {
            pnf->SetTextRange(GetContentFirstCp(), GetContentTextLength());
            fRangeSet = TRUE;
        }

#if DBG==1
        long    cp     = _dtr._cp;
        long    cchNew = _dtr._cchNew;
        long    cchOld = _dtr._cchOld;

        if (pnf->IsTextChange())
        {
            Assert(pnf->Cp() >= 0);
            Assert(pnf->Cp() >= GetContentFirstCp());
        }
#endif

        //
        // notify QuillLayout to adjust display structures
        //
        if (FExternalLayout())
        {
            if (_pQuillGlue)
                _pQuillGlue->Notify(pnf);

            // REVIEW alexmog: even though we don't really need
            //                 the rest of this code, we keep running
            //                 to get apropriate stuff dirtied.
            //                 In particular _dtr need to accumulate
            //                 changes to become dirty, which later
            //                 will tell DoLayout to actually do layout.
        }

        //
        //  If the notification is already "handled",
        //  Make adjustments to cached values for any text changes
        //

        if (    pnf->IsHandled()
            &&  pnf->IsTextChange())
        {
            _dtr.Adjust(pnf, GetContentFirstCp());

            GetDisplay()->Notify(pnf);
        }

        //
        //  If characters or an element are invalidating,
        //  then immediately invalidate the appropriate rectangles
        //
        //  We should not do further invalidation in QuillLayout,
	    //  as proper invalidation is already done through the previous Notify
        //
        else if (IsInvalidationNotification(pnf) && !FExternalLayout())
        {
            long    cp  = pnf->Cp() - GetContentFirstCp();
            long    cch = pnf->Cch();

            Assert( pnf->IsType(NTYPE_ELEMENT_INVALIDATE)
                ||  pnf->IsType(NTYPE_CHARS_INVALIDATE));
            Assert(cp  >= 0);
            Assert(cch >= 0);

            //
            //  Obtain the rectangles if the request range is measured
            //

            if (    IsRangeBeforeDirty(cp, cch)
                &&  (cp + cch) < GetDisplay()->_dcpCalcMax)
            {
                CDataAry<RECT>  aryRects(Mt(CFlowLayoutNotify_aryRects_pv));

                GetDisplay()->RegionFromRange(&aryRects, pnf->Cp(), pnf->Cch());

                Invalidate(&aryRects[0], NULL, NULL, 0, aryRects.Size());
            }

            //
            //  Otherwise, if a dirty region exists, extend the dirty region to encompass it
            //  NOTE: Requests within unmeasured regions are handled automatically during
            //        the measure
            //

            else if (IsDirty())
            {
                _dtr.Accumulate(pnf, GetContentFirstCp());
            }
        }


        //
        //  Handle z-order and position change notifications
        //

        else if (IsPositionNotification(pnf))
        {
            fHandle = HandlePositionNotification(pnf);
        }

        //
        //  Handle translated ranges
        //

        else if (pnf->IsType(NTYPE_TRANSLATED_RANGE))
        {
            Assert(pnf->IsDataValid());
            HandleTranslatedRange(pnf->DataAsSize());
        }

        //
        //  Handle z-parent changes
        //

        else if (pnf->IsType(NTYPE_ZPARENT_CHANGE))
        {
            if (!ElementOwner()->IsPositionStatic())
            {
                ElementOwner()->ZChangeElement();
            }

            else if (_fContainsRelative)
            {
                ZChangeRelDispNodes();
            }
        }

        //
        //  Handle changes to CSS display
        //

        else if (pnf->IsType(NTYPE_DISPLAY_CHANGE))
        {
            CView *     pView    = GetView();
            CElement *  pElement = ElementOwner();

            if (pView)
            {
                HWND    hwnd = pElement->GetHwnd();

                if (hwnd)
                {
                    CRect   rc;

                    ::GetWindowRect(hwnd, &rc);
                    ::MapWindowPoints(HWND_DESKTOP, Doc()->_pInPlace->_hwnd, (POINT *)&rc, 2);

                    pView->DeferSetWindowPos(hwnd,
                                            &rc,
                                            (pnf->DataAsInt()
                                                ? SWP_SHOWWINDOW
                                                : SWP_HIDEWINDOW),
                                            NULL);
                }
            }

            if (!ElementOwner()->IsPositionStatic())
            {
                if (pnf->DataAsInt())
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
        }

#ifdef BRENDAN_FILTER
        //
        //  Handle filter notifications
        //

        else if (IsFilterNotification(pnf))
        {
            fHandle =   !!GetFilterDispNode()
                    ||  ElementOwner()->HasFilterPtr();

            if (fHandle)
            {
                Assert(ElementOwner()->HasFilterPtr());
                if (pnf->IsType(NTYPE_OPEN_FILTER))
                {
                    ElementOwner()->GetFilterPtr()->OpenFilter();
                }
                else
                {
                    Assert(pnf->IsType(NTYPE_CLOSE_FILTER));
                    ElementOwner()->GetFilterPtr()->CloseFilter();
                }
            }
        }
#endif

        //
        //  Insert adornments as needed
        //

        else if (pnf->IsType(NTYPE_ELEMENT_ADD_ADORNER))
        {
            fHandle = HandleAddAdorner(pnf);
        }

        //
        //  Otherwise, accumulate the information
        //

        else if (   pnf->IsTextChange()
                ||  pnf->IsLayoutChange())
        {
            BOOL    fWasDirty        = IsDirty();
            BOOL    fHadRequests     = HasRequestQueue();
            BOOL    fIsAbsolute      = FALSE;

            Assert(!IsLocked());

            //
            //  Always dirty the layout of resizing/morphing elements
            //

            if (    pnf->Element()
                &&  pnf->Element() != ElementOwner()
                &&      (   pnf->IsType(NTYPE_ELEMENT_RESIZE)
                        ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)))
            {
                pnf->Element()->DirtyLayout(pnf->LayoutFlags());

                //
                //  For absolute elements, simply note the need to re-calc them
                //

                if(pnf->Element()->IsAbsolute())
                {
                    fIsAbsolute    = TRUE;
                    QueueRequest(pnf);
                }
            }

            //
            //  Otherwise, collect the range covered by the notification
            //

            if (    (   (   !fIsAbsolute
                        ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE))
                    ||  pnf->Element() == ElementOwner())
                &&  pnf->Cp() >= 0)
            {
                _fTopDown =     _fTopDown
                            ||  (   (   pnf->IsType(NTYPE_CHARS_ADDED)
                                    ||  pnf->IsType(NTYPE_CHARS_DELETED))
                                &&  pnf->Cch() > 1);

                _dtr.Accumulate(pnf, GetContentFirstCp());

                //
                //  Mark forced layout if requested
                //   

                if (    pnf->Element() == ElementOwner()
                    &&  (pnf->LayoutFlags() & LAYOUT_FORCE))
                {
                    _fForceLayout = TRUE;
                }

                //
                //  Invalidate cached min/max values when content changes size
                //

                if (    !_fPreserveMinMax
                    &&  _fMinMaxValid
                    &&  _fContentsAffectSize
                    &&  (   pnf->IsTextChange()
                        ||  pnf->IsType(NTYPE_ELEMENT_RESIZE)
                        ||  pnf->IsType(NTYPE_ELEMENT_MINMAX)
                        ||  pnf->IsType(NTYPE_ELEMENT_REMEASURE)
                        ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)
                        ||  pnf->IsType(NTYPE_CHARS_RESIZE)))
                {
                    ResetMinMax();
                }
            }

            //
            //  If the layout is transitioning to dirty for the first time and
            //  is not set to get called by its containing layout engine (via CalcSize),
            //  post a layout request
            //  (For purposes of posting a layout request, transitioning to dirty
            //   means that previously no text changes were recorded and no absolute
            //   descendents needed sizing and now at least of those states is TRUE)
            //

            if (!FExternalLayout() || !(_pQuillGlue && _pQuillGlue->_fLayoutRequested))
            {
            if (    !pnf->IsFlagSet(NFLAGS_DONOTLAYOUT)
                &&  !_fSizeThis
                &&  !fWasDirty
                &&  !fHadRequests
                &&  (   IsDirty()
                    ||  HasRequestQueue()))
            {
                PostLayoutRequest(pnf->LayoutFlags());
            }
            }
            else
            {
            // $REVIEW alexmog: are there cases when we forget to request layout?
            Assert(TRUE);
            }
        }
#if DBG==1
        if (_dtr._cp != -1 && !pnf->_fNoTextValidate )
        {
            Assert(_dtr._cp >= 0);
            Assert(_dtr._cp <= GetContentTextLength());
            Assert(_dtr._cchNew >= 0);
            Assert(_dtr._cchOld >= 0);
            Assert((_dtr._cp + _dtr._cchNew) <= GetContentTextLength());
        }

        TraceTagEx((tagNotifyText, TAG_NONAME,
                   "NotifyText: (%d) Element(0x%x,%S) cp(%d-%d) cchNew(%d-%d) cchOld(%d-%d)",
                   pnf->SerialNumber(),
                   ElementOwner(),
                   ElementOwner()->TagName(),
                   cp, _dtr._cp,
                   cchNew, _dtr._cchNew,
                   cchOld, _dtr._cchOld));
#endif

        //
        //  Reset the range if previously set
        //

        if (fRangeSet)
        {
            pnf->ClearTextRange();
        }
    }

    switch (pnf->Type())
    {
    case NTYPE_AMBIENT_PROP_CHANGE:
        pnf->Data(&dwData);
        if ((DISPID)dwData == DISPID_UNKNOWN)
        {
            // BUGBUG -- This call should be removed! (lylec, garybu)
            //           Beta 2 Bug #22961
            Invalidate();
        }
        break;

    case NTYPE_DOC_STATE_CHANGE:
        pnf->Data(&dwData);
        if (Doc()->State() <= OS_RUNNING)
        {
            _dp.StopBackgroundRecalc();
        }
        break;
        
    case NTYPE_SELECT_CHANGE:
        // Fire this onto the form
        Doc()->OnSelectChange();
        break;

    case NTYPE_FORCE_DIRTY:
        // BUGBUG: Fix this....Having no lines, but a range, when CalcSize is called should set _fSizeThis!
        // BUGBUG:  Setting _fSizeThis here is a HACK!! This flag should never be set outside
        //          of RequestResize, etc. Unfortunately, the SN_FORCEDIRTY notification comes
        //          when responding to the effects of RequestResize and, so, calling RequestResize
        //          from here would be problematic. The correct fix (which will be pursued
        //          post-BETA 1) is to re-work the relationship of RequestResize and managing
        //          text changes. (brendand)
        _fSizeThis = TRUE;

        _dp.FlushRecalc();
        if (FExternalLayout())
        {
            if (_pQuillGlue)
                _pQuillGlue->NukeLayout();
        }
        Reset();
        break;

    case NTYPE_ELEMENT_EXITTREE:
        Reset();
        break;

    case NTYPE_ELEMENT_ENSURERECALC:
        {
            long    cp  = pnf->Cp() - GetContentFirstCp();
            long    cch = pnf->Cch();

            ElementOwner()->SendNotification(NTYPE_ELEMENT_ENSURERECALC);

            if (    !IsRangeBeforeDirty(cp, cch) 
                ||  GetDisplay()->_dcpCalcMax <= (cp + cch))
            {
                GetDisplay()->WaitForRecalc((cp + cch), -1);
            }
        }
        break;
    }

    //
    //  If the the current layout is hidden, then forward the current notification
    //  to the parent as a resize notfication so that parents keep track of the dirty
    //  range.
    //

    if (    !pnf->IsFlagSet(NFLAGS_DESCENDENTS)
        &&  (   pnf->IsType(NTYPE_ELEMENT_REMEASURE)
            ||  pnf->IsType(NTYPE_ELEMENT_RESIZE)
            ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)
            ||  pnf->IsType(NTYPE_CHARS_RESIZE))
        &&  (   Tag() != ETAG_BODY
            &&  IsDisplayNone()))
    {
        pnf->ChangeTo(NTYPE_ELEMENT_RESIZE, ElementOwner());
    }

    //
    //  Handle the notification
    //
    //

    if (    fHandle
        &&  (   !pnf->Element()
            ||  pnf->IsType(NTYPE_ELEMENT_REMEASURE)
            ||  pnf->Element() != ElementOwner()))
    {
        pnf->SetHandler(ElementOwner());
    }

    WHEN_DBG(_snLast = pnf->SerialNumber());
}


//+----------------------------------------------------------------------------
//
//  Member:     Reset
//
//  Synopsis:   Reset the channel (clearing any dirty state)
//
//-----------------------------------------------------------------------------
void
CFlowLayout::Reset()
{
    super::Reset();

    if (IsDirty())
    {
        _fTopDown = FALSE;
        _dtr.Reset();
    }

#ifdef QUILL
    if (_pQuillGlue)
        _pQuillGlue->GetTextLayout()->Reset();
#endif // QUILL
}

//+----------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Reset the channel
//
//-----------------------------------------------------------------------------
void
CFlowLayout::Detach()
{
    // flushes the region cache and rel line cache.
    _dp.Detach();
    if (_pQuillGlue)
        _pQuillGlue->Detach();

    super::Detach();
}


//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::CFlowLayout
//
//  Synopsis:   Normal constructor.
//
//  Arguments:  CElement * - element that owns the layout
//
//----------------------------------------------------------------------------

CFlowLayout::CFlowLayout(CElement * pElementFlowLayout)
                : CLayout(pElementFlowLayout)
{
    _fAutoScroll = TRUE;
    _xMax      =
    _xMin      = -1;
    _fCanHaveChildren     = TRUE;
    ElementOwner()->_fOwnsRuns = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::CFlowLayout
//
//  Synopsis:   Normal destructor.
//
//  Arguments:  CElement * - element that owns the layout
//
//----------------------------------------------------------------------------

CFlowLayout::~CFlowLayout()
{
    Assert(_fDetached);

    if (_pQuillGlue)
    {
        _pQuillGlue->Release();
        _pQuillGlue = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::Init
//
//  Synopsis:   Initialization function to initialize the line array, and
//              scroll and background recalc information if necessary.
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::Init()
{
    HRESULT hr;

    hr = super::Init();
    if(hr)
    {
        goto Cleanup;
    }

    if(!_dp.Init())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    _dp._fPrinting  = Doc()->IsPrintDoc();
    _dp._dxCaret    = ElementOwner()->IsEditable() ? 1 : 0;

    // initialize QuillGlue
    if (FExternalLayout() && !_pQuillGlue)
    {
        _pQuillGlue = (CQuillGlue *)new CQuillGlue();
        if (!_pQuillGlue)
            {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
            }

        // addref member object, so that it never tries to delete itself
        _pQuillGlue->AddRef();

        hr = _pQuillGlue->Init(this);
        if (hr)
            {
            _pQuillGlue->Release();
            _pQuillGlue = NULL;
            goto Cleanup;
            }
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::OnExitTree
//
//  Synopsis:   Deinitilialize the display on exit tree
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::OnExitTree()
{
    HRESULT hr;

    hr = super::OnExitTree();
    if(hr)
    {
        goto Cleanup;
    }

    _dp.FlushRecalc();

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     FindAndDirtyFlowLayouts
//
//  Synopsis:   Notify nested CFlowLayouts of text changes which require a
//              full-recalc
//
//-----------------------------------------------------------------------------
void
CFlowLayout::FindAndDirtyFlowLayouts (
    long    iRunStart,
    long    iRunStop )
{
#ifdef MERGEFUN // iRuns
    CTreePosList &  eruns = GetMarkup()->GetList();
    CTreeNode *     pNodeFlowLayoutLastNotified = NULL;

    Assert(iRunStart < eruns.NumRuns());

    if(iRunStart >= eruns.NumRuns())
        return;

    if(iRunStop >= eruns.NumRuns())
    {
        AssertSz(FALSE, "dirty range is not within the tree");
        iRunStop = eruns.NumRuns() - 1;
    }

    //
    // Place the CFlowLayout in sizing mode so that size change requests which
    // result from the notification are not propagated to the root
    // NOTE: Due to odd trees created by overlapping tags and content between
    //       table elements (e.g., an A starting between TRs), it is possible
    //       to never "see" pFlowLayoutTop. As a result, the loop is also set
    //       to terminate once it arrives at a CTxtEdit.
    //

    CSite::CLock    Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);

    for ( ; iRunStart <= iRunStop; iRunStart++ )
    {
        CTreeNode * pNode = eruns.GetBranchAbs( iRunStart );
        CTreeNode * pNodeFlowLayout;
        BOOL        fFirst = TRUE;

        for (pNodeFlowLayout = pNode->GetFlowLayoutNode();
             DifferentScope(pNodeFlowLayout, GetFirstBranch());
             pNodeFlowLayout = pNodeFlowLayout->Parent()->GetFlowLayoutNode())
        {
            Assert(pNodeFlowLayout);

            if (SameScope(pNodeFlowLayout, pNodeFlowLayoutLastNotified))
                break;

            if (fFirst)
            {
                fFirst = FALSE;
                pNodeFlowLayoutLastNotified = pNodeFlowLayout;
            }

            {
                CNotification   nf;

                nf.ForceDirty(pNodeFlowLayout->ElementOwner());
                pNodeFlowLayout->ElementOwner()->Notify(&nf);
            }

            if (pNodeFlowLayout->IsRoot())
                break;
        }
    }
#endif
}


void
CFlowLayout::DoLayout(
    DWORD   grfLayout)
{
    // requested layout happens, can now enqueue more
    if (_pQuillGlue)
        _pQuillGlue->_fLayoutRequested = FALSE;

    // Hidden layout should just accumulate changes, and
    // are measured when unhidden.
    if(!IsDisplayNone() || Tag() == ETAG_BODY)
    {
        CCalcInfo   CI(this);
        CSize       size;

        GetSize(&size);

        CI._grfLayout |= grfLayout;

        if (_fForceLayout)
        {
            CI._grfLayout |= LAYOUT_FORCE;
            _fForceLayout = FALSE;
        }

        //
        //  First, recalculate the text (if needed)
        //

        // $REVIEW alexmog: it is not enough to call IsDiry() to determine if
        // the external layout needs to be updated. For performance, we should
        // have ITestLayoutElement::IsLayoutDirty()
        
        if (IsDirty() || FExternalLayout())
        {
            CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
            CalcTextSize(&CI, &size);
        }

        //
        // If any absolutely positioned sites need sizing, do so now
        //

        if (HasRequestQueue())
        {
            long xParentWidth;
            long yParentHeight;

            _dp.GetViewWidthAndHeightForChild(
                &CI,
                &xParentWidth,
                &yParentHeight,
                CI._smMode == SIZEMODE_MMWIDTH);

            ProcessRequests(&CI, CSize(xParentWidth, yParentHeight));
        }

        //
        // Reset the layout
        // (This is usually done when the text is recalculated.
        //  However, that is skipped if only absolute descendents needed sizing.)
        //

        Reset();
    }
    else
    {
        FlushRequests();
        RemoveLayoutRequest();
    }
}


//+------------------------------------------------------------------------
//
//  Member:     ResizePercentHeightSites
//
//  Synopsis:   Send an ElementResize for any immediate contained sites
//              whose size is a percentage
//
//-------------------------------------------------------------------------

void
CFlowLayout::ResizePercentHeightSites()
{
    CNotification   nf;
    CLayout *       pLayout;
    DWORD           dw;

    //
    // If no contained sites are affected, immediately return
    //

    if (!_fChildHeightPercent)
        return;

    //
    // Otherwise, iterate through all sites, sending an ElementResize notification for those affected
    // (Also, note that resizing a percentage height site cannot affect min/max values)
    // NOTE: With "autoclear", the min/max can vary after resizing percentage sized
    //       descendents. However, the calculated min/max values, which used for table
    //       sizing, should take into account those changes since doing so would likely
    //       break how tables layout relative to Netscape. (brendand)
    //
    Assert(!_fPreserveMinMax);
    _fPreserveMinMax = TRUE;

    for (pLayout = GetFirstLayout(&dw); pLayout; pLayout = GetNextLayout(&dw))
    {
        if (pLayout->PercentHeight())
        {
            nf.ElementResize(pLayout->ElementOwner(), NFLAGS_CLEANCHANGE);
            GetContentMarkup()->Notify( nf );
        }
    }
    ClearLayoutIterator(dw, FALSE);

    _fPreserveMinMax = FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFlowLayout::CalcSize
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

DWORD
CFlowLayout::CalcSize(
    CCalcInfo * pci,
    SIZE *      psize,
    SIZE *      psizeDefault)
{
    CSaveCalcInfo   sci(pci, this);

    BOOL    fNormalMode = (     pci->_smMode == SIZEMODE_NATURAL
                            ||  pci->_smMode == SIZEMODE_SET
                            ||  pci->_smMode == SIZEMODE_FULLSIZE);
    BOOL    fRecalcText = FALSE;
    BOOL    fWidthChanged, fHeightChanged;
    CSize   sizeOriginal;
    DWORD   grfReturn;

    Assert(!IsDisplayNone() || Tag() == ETAG_BODY);

    Listen();

    Assert(pci);
    Assert(psize);

#if DO_PROFILE
    // Start icecap if we're in a table cell.
    if (ElementOwner()->Tag() == ETAG_TD ||
        ElementOwner()->Tag() == ETAG_TH)
    {
        StartCAP();
    }
#endif

    //
    // Set default return values and initial state
    //

    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    grfReturn = (pci->_grfLayout & LAYOUT_FORCE);

    if (pci->_grfLayout & LAYOUT_FORCE)
    {
        _fSizeThis         = TRUE;
        _fAutoBelow        = FALSE;
        _fPositioned       = FALSE;
        _fContainsRelative = FALSE;
    }
    fWidthChanged  = (fNormalMode
                        ? psize->cx != sizeOriginal.cx
                        : FALSE);
    fHeightChanged = (fNormalMode
                        ? psize->cy != sizeOriginal.cy
                        : FALSE);

    //
    // If height has changed, mark percentage sized children as in need of sizing
    // (Width changes cause a full re-calc and thus do not need to resize each
    //  percentage-sized site)
    //

    if (    fNormalMode
        &&  !fWidthChanged
        &&  !ElementOwner()->_fContainsVertPercentAttr
        &&  fHeightChanged)
    {
        long    fContentsAffectSize = _fContentsAffectSize;

        _fContentsAffectSize = FALSE;
        ResizePercentHeightSites();
        _fContentsAffectSize = fContentsAffectSize;
    }

    //
    // For changes which invalidate the entire layout, dirty all of the text
    //

    fRecalcText =   (fNormalMode && (   IsDirty()
                                    ||  _fSizeThis
                                    ||  fWidthChanged
                                    ||  fHeightChanged))
                ||  (pci->_grfLayout & LAYOUT_FORCE)
                ||  (pci->_smMode == SIZEMODE_MMWIDTH && !_fMinMaxValid)
                ||  (pci->_smMode == SIZEMODE_MINWIDTH && !_fMinMaxValid);

    //
    // Cache sizes and recalculate the text (if required)
    //

    if (fRecalcText)
    {
        CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);

        pci->EnsureFilterOpen();

        //
        // If dirty, ensure display tree nodes exist
        //

        if (    _fSizeThis
            &&  fNormalMode
            &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
        {
            grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
        }

        //
        // Calculate the text
        //

        CalcTextSize(pci, psize, psizeDefault);

        //
        // For normal modes, cache values and request layout
        //

        if (fNormalMode)
        {
            grfReturn    |= LAYOUT_THIS  |
                            (psize->cx != sizeOriginal.cx
                                    ? LAYOUT_HRESIZE
                                    : 0) |
                            (psize->cy != sizeOriginal.cy
                                    ? LAYOUT_VRESIZE
                                    : 0);

            //
            // If size changes occurred, size the display nodes
            //

            if (    _pDispNode
                &&  (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE)))

            {
                CSize   sizeContent(0, 0);

                if (!FExternalLayout())
                {
                    sizeContent.SetSize(_dp.GetMaxWidth(), _dp.GetHeight());
                }
                else
                {
                    if (_pQuillGlue)
                        sizeContent.SetSize(_pQuillGlue->GetLongestLineWidth(), _pQuillGlue->GetNATURALBestHeight());
                }

                SizeDispNode(
                            pci,
                            *psize,
                            sizeContent);
                SizeContentDispNode(sizeContent);
            }

            //
            // Mark the site clean
            //

            _fSizeThis = FALSE;
        }

        //
        // For min/max mode, cache the values and note that they are now valid
        //

        else if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            _xMax = psize->cx;
            _xMin = psize->cy;
            _fMinMaxValid = TRUE;
        }

        else if (pci->_smMode == SIZEMODE_MINWIDTH)
        {
            _xMin = psize->cx;
        }
    }

    //
    // If any absolutely positioned sites need sizing, do so now
    //

    if (    pci->_smMode == SIZEMODE_NATURAL
        &&  HasRequestQueue())
    {
        long xParentWidth;
        long yParentHeight;

        _dp.GetViewWidthAndHeightForChild(
                pci,
                &xParentWidth,
                &yParentHeight,
                pci->_smMode == SIZEMODE_MMWIDTH);

        ProcessRequests(pci, CSize(xParentWidth, yParentHeight));
    }

    //
    // Lastly, return the requested size
    //

    switch (pci->_smMode)
    {
    case SIZEMODE_SET:
    case SIZEMODE_NATURAL:
    case SIZEMODE_FULLSIZE:
        Assert(!_fSizeThis);
        GetSize(psize);
        Reset();
        break;

    case SIZEMODE_MMWIDTH:
        Assert(_fMinMaxValid);
        psize->cx = _xMax;
        psize->cy = _xMin;
        if (!fRecalcText && psizeDefault)
        {
            GetSize(psizeDefault);
        }
        break;

    case SIZEMODE_MINWIDTH:
        psize->cx = _xMin;
        psize->cy = 0;
        break;
    }

#if DO_PROFILE
    // Start icecap if we're in a table cell.
    if (ElementOwner()->Tag() == ETAG_TD ||
        ElementOwner()->Tag() == ETAG_TH)
        StopCAP();
#endif

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


//+-------------------------------------------------------------------------
//
//  Method:     CFlowLayout::CalcTextSize
//
//  Synopsis:   Calculate the size of the contained text
//
//--------------------------------------------------------------------------

void
CFlowLayout::CalcTextSize(
    CCalcInfo * pci,
    SIZE      * psize,
    SIZE      * psizeDefault)
{
    CElement *  pElement = ElementOwner();
    BOOL        fNormalMode = ( pci->_smMode == SIZEMODE_NATURAL
                            ||  pci->_smMode == SIZEMODE_SET);
    BOOL        fFullRecalc;
    CRect       rcViewCurrent;
    CRect       rcView;
    long        cxView, cyView;
    long        cxAdjustment = 0;
    long        cyAdjustment = 0;
    long        yViewHeightOld = _dp.GetViewHeight();

    // Hidden layouts should just accumulate changes, and
    // are measured when unhidden.
    Assert(!IsDisplayNone() || Tag() == ETAG_BODY);

    _dp._fRTL = (GetFirstBranch()->GetCascadedBlockDirection() == styleDirRightToLeft);

    //
    // Adjust the incoming size for max/min width requests
    //

    if (pci->_smMode == SIZEMODE_MMWIDTH)
    {
        psize->cx = lMaximumWidth;
        psize->cy = lMaximumWidth;
    }
    else if (pci->_smMode == SIZEMODE_MINWIDTH)
    {
        psize->cx = 1;
        psize->cy = lMaximumWidth;
    }

    //
    // Get the current "view" rectangle
    //

    GetClientRect((CRect *)&rcViewCurrent, 0, pci);

    //
    // Construct the "view" rectangle from the available size
    // Also, determine the amount of space to allow for things external to the view
    // (For sites which size to their content, calculate the full amount;
    //  otherwise, simply take that which is left over after determining the
    //  view size. Additionally, ensure their view size is never less than 1
    //  pixel, since recalc will not take place for smaller sizes.)
    //

    if (_fContentsAffectSize)
    {
        long    lMinimum = (pci->_smMode == SIZEMODE_MINWIDTH
                                ? 1
                                : 0);

        rcView.top    = 0;
        rcView.left   = 0;
        rcView.right  = 0x7FFFFFFF;
        rcView.bottom = 0x7FFFFFFF;

// BUGBUG: Do we want to include the scrollbar width in the cxAdjustment for min/max requests?
//         This code currently does...is that correct? (brendand)
        GetClientRect((CRect *)&rcView, GCR_USERECT, pci);

        cxAdjustment  = 0x7FFFFFFF - (rcView.right - rcView.left);
        cyAdjustment  = 0x7FFFFFFF - (rcView.bottom - rcView.top);

        rcView.right  = rcView.left + max(lMinimum, psize->cx - cxAdjustment);
        rcView.bottom = rcView.top  + max(lMinimum, psize->cy - cyAdjustment);


    }

    else
    {
        rcView.top    = 0;
        rcView.left   = 0;
        rcView.right  = psize->cx;
        rcView.bottom = psize->cy;

        GetClientRect((CRect *)&rcView, GCR_USERECT, pci);
    }

    cxView = max(0L, rcView.right - rcView.left);
    cyView = max(0L, rcView.bottom - rcView.top);

    if (!_fContentsAffectSize)
    {
        cxAdjustment = psize->cx - cxView;
        cyAdjustment = psize->cy - cyView;
    }

    //
    // Determine if a full recalc of the text is necessary
    // NOTE: SetViewSize must always be called first
    //

    fFullRecalc =   _dp.SetViewSize(rcView)
                || (   pElement->_fContainsVertPercentAttr
                    &&  _dp.GetViewHeight() != yViewHeightOld)
                ||  !fNormalMode
                ||  (pci->_grfLayout & LAYOUT_FORCE);

    //
    // If the text will be fully recalc'd, cancel any outstanding changes
    //

    if (fFullRecalc || FExternalLayout())
    {
        CancelChanges();
    }

    //
    // If only a partial recalc is necessary, commit the changes
    //
    if (!IsCommitted() && !FExternalLayout())
    {
        Assert(pci->_smMode != SIZEMODE_MMWIDTH);
        Assert(pci->_smMode != SIZEMODE_MINWIDTH);

        CommitChanges(pci);
    }

    //
    // Otherwise, completely recalculate the text
    //

    else if (fFullRecalc || 
             FExternalLayout()) //incremental layout still requires external layout to be updated
    {
        CSaveCalcInfo   sci(pci, this);
        BOOL            fWordWrap = _dp.GetWordWrap();

        if (pci->_smMode != SIZEMODE_MMWIDTH &&
            pci->_smMode != SIZEMODE_MINWIDTH)
        {
            long          xParentWidth;
            long          yParentHeight;

            _dp.GetViewWidthAndHeightForChild(
                pci,
                &xParentWidth,
                &yParentHeight,
                pci->_smMode == SIZEMODE_MMWIDTH);

            //
            // BUGBUG(SujalP, SriniB and BrendanD): These 2 lines are really needed here
            // and in all places where we instantiate a CI. However, its expensive right
            // now to add these lines at all the places and hence we are removing them
            // from here right now. We have also removed them from CDisplay::UpdateView()
            // for the same reason. (Bug#s: 58809, 62517, 62977)
            //
            pci->SizeToParent(xParentWidth, yParentHeight);
        }

        //
        // BUGBUG: Logically, this should be set elsewhere, but this is most
        //         convenient spot with the current architecture (brendand)
        //

        SetOpaqueFlag();

        if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            _dp.SetWordWrap(FALSE);
        }

        if (    fNormalMode
            &&  !(pci->_grfLayout & LAYOUT_FORCE)
            &&  _fMinMaxValid
            &&  (!FExternalLayout() ?
                    (_dp._fMinMaxCalced
                     &&  cxView >= _dp._xMaxWidth) :
                    (_pQuillGlue
                     && _pQuillGlue->FMinMaxValid()
                     && cxView >= _pQuillGlue->GetMMWIDTHMaxWidth())))
        {
            Assert(_dp._xWidthView  == cxView);
            Assert(!_fChildWidthPercent);
            Assert(!ContainsChildLayout());

            if (!FExternalLayout())
            {
                _dp.RecalcLineShift(pci, pci->_grfLayout);
            }
            else
            {
                if (_pQuillGlue)
                    _pQuillGlue->RecalcLineShift(pci, pci->_grfLayout);
            }
        }

        else
        {
            _fAutoBelow        = FALSE;
            _fContainsRelative = FALSE;
            if (!FExternalLayout())
            {
                _dp.RecalcView(pci, fFullRecalc);
            }
            else
            {
                if (_pQuillGlue)
                    _pQuillGlue->RecalcView(pci, fFullRecalc);
            }
        }

        // BUGBUG (srinib) - if we get here through DoLayout, we need
        // inval the entire flowlayout. DoLayout does not inval or
        // position layouts(Is this right? Something to think about.)
        if (!_fSizeThis && 
			!FExternalLayout(/*otherwise, how would external layout be smart?*/))
        {
            Invalidate();
        }

        if (fNormalMode)
        {
            _dp._fMinMaxCalced = FALSE;
            if (_pQuillGlue)
                _pQuillGlue->ResetMinMax();
        }

        if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            _dp.SetWordWrap(fWordWrap);
        }
    }

    //
    //  Propagate CCalcInfo state as appropriate
    //

    if (fNormalMode)
    {
        CLine *     pli;
        unsigned    i;

        //
        // For normal calculations, determine the baseline of the "first" line
        // (skipping over any aligned sites at the beginning of the text)
        //

        pci->_yBaseLine = 0;

        if (!FExternalLayout())
        {
            for (i=0; i < _dp.Count(); i++)
            {
                pli = _dp.Elem(i);
                if (!pli->IsFrame())
                {
        // BUGBUG: Is this correct given the new meaning of the CLine members? (brendand)
                    pci->_yBaseLine = pli->_yHeight - pli->_yDescent;
                    break;
                }
            }
        }
        else
        {
            if (_pQuillGlue)
                pci->_yBaseLine = _pQuillGlue->GetNATURALBaseline();
        }
    }

    //
    // Determine the size from the calculated text
    //

    if (pci->_smMode != SIZEMODE_SET)
    {
        switch (pci->_smMode)
        {
        case SIZEMODE_FULLSIZE:
        case SIZEMODE_NATURAL:
            if (_fContentsAffectSize || pci->_smMode == SIZEMODE_FULLSIZE)
            {
                if (!FExternalLayout())
                {
                    _dp.GetSize(psize);
                }
                else
                {
                    if (_pQuillGlue)
                    {
                        psize->cx = _pQuillGlue->GetLongestLineWidth();
                        psize->cy = _pQuillGlue->GetNATURALBestHeight();
                    }
                }
                ((CSize *)psize)->Max(((CRect &)rcView).Size());
            }
            else
            {
                psize->cx = cxView;
                psize->cy = cyView;
            }
            break;

       case SIZEMODE_MMWIDTH:
            if (!FExternalLayout())
            {
                psize->cx = _dp.GetWidth();
                psize->cy = _dp._xMinWidth;
            }
            else
            {
                if (_pQuillGlue)
                {
                    psize->cx = _pQuillGlue->GetLongestLineWidth();
                    psize->cy = _pQuillGlue->GetMMWIDTHMinWidth();
                }
            }
            if (psizeDefault)
            {
                if (!FExternalLayout())
                {
                    psizeDefault->cx = _dp.GetWidth() + cxAdjustment;
                    psizeDefault->cy = _dp.GetHeight() + cyAdjustment;
                }
                else
                {
                    if (_pQuillGlue)
                    {
                        psizeDefault->cx = _pQuillGlue->GetLongestLineWidth() + cxAdjustment;
                        psizeDefault->cy = _pQuillGlue->GetNATURALBestHeight() + cyAdjustment;
                    }
                }
            }
            break;

        case SIZEMODE_MINWIDTH:
        {
            if (!FExternalLayout())
            {
                psize->cx = _dp.GetWidth();
                psize->cy = _dp.GetHeight();
            }
            else
            {
                if (_pQuillGlue)
                {
                    psize->cx = _pQuillGlue->GetLongestLineWidth();
                    psize->cy = _pQuillGlue->GetNATURALBestHeight();
                }
            }
            _dp.FlushRecalc();
            break;
        }

#if DBG==1
        default:
            AssertSz(0, "CFlowLayout::CalcTextSize: Unknown SIZEMODE_xxxx");
            break;
#endif
        }

        psize->cx += cxAdjustment;
        psize->cy += (pci->_smMode == SIZEMODE_MMWIDTH
                              ? cxAdjustment
                              : cyAdjustment);
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   ViewChange
//
//  Synopsis:   Called when the visible view has changed. Notifies the
//              Doc so that proper Ole notifications can be sent
//
//-----------------------------------------------------------------------------

void
CFlowLayout::ViewChange(BOOL fUpdate)
{
    Doc()->OnViewChange(DVASPECT_CONTENT);
#if 0
    if(fUpdate)
        Doc()->UpdateForm();
#endif
}


//+----------------------------------------------------------------------------
//
//  Member:     CFlowLayout::ScrollIntoView
//
//  Synopsis:   CFlowLayout overrides ScrollIntoView to be sure that it layout
//              is completely calced. This will assure that the CFlowLayout
//              has the correct _sizeProposed. If the parent has not yet calced
//              upto the element that owns this layout, _rc is not set, so wait
//              for the parent to finish recalc.
//
//  Arguments:  spVert   - Where to "pin" the element
//              spHorz   - Where to "pin" the element
//
//-----------------------------------------------------------------------------
HRESULT
CFlowLayout::ScrollIntoView(SCROLLPIN spVert, SCROLLPIN spHorz, BOOL fFirstVisible)
{
    HRESULT hr;

// BUGBUG: Generalize this so that this routine no longer needs to be virtual, enabling the
//         implementation on CLayout to handle all requests (brendand)
    hr = THR(WaitForParentToRecalc(GetContentLastCp(), -1, NULL));
    if (hr)
        goto Cleanup;

    hr = THR(super::ScrollIntoView(spVert, spHorz, fFirstVisible));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     CFlowLayout::ScrollElementIntoView
//
//  Synopsis:   Scroll a give element under the influence of the layout engine
//              into the view.
//
//  Arguments:  pElement - CElement to make visible
//              spVert   - Where to "pin" the element
//              spHorz   - Where to "pin" the element
//
//-----------------------------------------------------------------------------
HRESULT
CFlowLayout::ScrollElementIntoView(
    CElement *  pElement,
    SCROLLPIN   spVert,
    SCROLLPIN   spHorz,
    BOOL        fFirstVisible)
{
    HRESULT     hr = E_FAIL;
    LONG        cpMin, cpMost;

    Assert(ElementOwner()->IsInMarkup());

    if(pElement != NULL)
    {
        cpMin  = max(GetContentFirstCp(), pElement->GetFirstCp());
        cpMost = min(GetContentLastCp(),  pElement->GetLastCp());
    }
    else
    {
        // Scroll top/bottom into view
        cpMin  =
        cpMost = -1;
    }

    hr = THR (ScrollRangeIntoView (cpMin, cpMost, spVert, spHorz));

    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CFlowLayout::ScrollRangeIntoView
//
//  Synopsis:   Scroll an arbitrary range into view
//
//  Arguments:  cpMin:   Starting cp of the range
//              cpMost:  Ending cp of the range
//              spVert:  Where to "pin" the range
//              spHorz:  Where to "pin" the range
//
//-------------------------------------------------------------------------

HRESULT
CFlowLayout::ScrollRangeIntoView(
    long        cpMin,
    long        cpMost,
    SCROLLPIN   spVert,
    SCROLLPIN   spHorz)
{
extern void BoundingRectForAnArrayOfRectsWithEmptyOnes(RECT *prcBound, CDataAry<RECT> * paryRects);

    CStackDataAry<RECT, 5> aryRects(Mt(CFlowLayoutScrollRangeInfoView_aryRects_pv));
    HRESULT     hr = S_OK;

    if (!_pDispNode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (cpMin >= 0)
    {
        CRect     rc;
        CCalcInfo CI(this);

        hr = THR(WaitForParentToRecalc(cpMost, -1, &CI));
        if (hr)
            goto Cleanup;

        _dp.RegionFromElement( ElementOwner(),      // the element
                                 &aryRects,         // rects returned here
                                 NULL,              // offset the rects
                                 NULL,              // ask RFE to get CFormDrawInfo
                                 0,                 // coord w/ respect to the display and not the client rc
                                 cpMin,             // give me the rects from here ..
                                 cpMost,            // ... till here
                                 NULL);             // dont need bounds of the element!

        // Calculate and return the total bounding rect
        BoundingRectForAnArrayOfRectsWithEmptyOnes(&rc, &aryRects);

        if(spVert == SP_TOPLEFT)
        {
            // Though RegionFromElement has already called WaitForRecalc,
            // it calls it until top is recalculated. In order to scroll
            // ptStart to the to of the screen, we need to wait until
            // another screen is recalculated.
            if (!_dp.WaitForRecalc(-1, rc.top + _dp.GetViewHeight()))
            {
                hr = E_FAIL;
                goto Cleanup;
            }
        }

        ScrollRectIntoView(rc, spVert, spHorz);
    }
    else
    {
        super::ScrollIntoView(spVert, spHorz);
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
// Member:      CFlowLayout::GetSiteWidth
//
// Synopsis:    returns the width and x proposed position of any given site
//              taking the margins into account.
//
//-----------------------------------------------------------------------------

BOOL
CFlowLayout::GetSiteWidth(CLayout   *pLayout,
                          CCalcInfo *pci,
                          BOOL       fBreakAtWord,
                          LONG       xWidthMax,
                          INT       *pxMinSiteWidth,
                          LONG      *pxWidth,
                          LONG      *pyHeight)
{
    LONG xLeftMargin, xRightMargin, yTopMargin, yBottomMargin;
    SIZE sizeObj;

    Assert (pLayout && pxWidth && pxMinSiteWidth);

    *pxWidth        = 0;
    *pxMinSiteWidth = 0;
    if(pyHeight)
        *pyHeight       = 0;

    if(pLayout->IsDisplayNone())
        return FALSE;

    //
    // measure the site
    //
    if(MeasureSite(pLayout,
                   pci,
                   xWidthMax,
                   fBreakAtWord,
                   &sizeObj,
                   pxMinSiteWidth))
    {
        return TRUE;
    }

    // get the margin info for the site
    pLayout->GetMarginInfo(pci, &xLeftMargin, &yTopMargin, &xRightMargin, &yBottomMargin);

    // not adjust the size and proposed x pos to include margins
    *pxWidth         = max(0L, xLeftMargin + xRightMargin + sizeObj.cx);
    *pxMinSiteWidth += max(0L, xLeftMargin + xRightMargin);
    if (pyHeight)
    {
        *pyHeight = max(0L, sizeObj.cy + yTopMargin + yBottomMargin);
    }

    return FALSE;
}


//+----------------------------------------------------------------------------
//
// Member:      CFlowLayout::MeasureSite()
//
// Synopsis:    Measure width and height of a embedded site
//
//-----------------------------------------------------------------------------
int
CFlowLayout::MeasureSite(CLayout   *pLayout,
                         CCalcInfo *pci,
                         LONG       xWidthMax,
                         BOOL       fBreakAtWord,
                         SIZE      *psizeObj,
                         int       *pxMinWidth)
{
    CSaveCalcInfo   sci(pci);
    LONG            lRet = 0;

    Assert(pci->_smMode != SIZEMODE_SET);

    if (!pLayout->ElementOwner()->IsInMarkup())
    {
        psizeObj->cx = psizeObj->cy = 0;
        return lRet;
    }

    if (fBreakAtWord)
    {
        long xParentWidth;
        long yParentHeight;

        _dp.GetViewWidthAndHeightForChild(
                pci,
                &xParentWidth,
                &yParentHeight);

        // Set the appropriate parent width
        pci->SizeToParent(xParentWidth, yParentHeight);

        // set available size in sizeObj
        psizeObj->cx = xWidthMax;
        psizeObj->cy = pci->_sizeParent.cy;

        // Ensure the available size does not exceed that of the view
        // (For example, when word-breaking is disabled, the available size
        //  is set exceedingly large. However, percentage sized sites should
        //  still not grow themselves past the view width.)
        if (pci->_smMode == SIZEMODE_NATURAL                &&
            pLayout->PercentSize() )
        {
            if (pci->_sizeParent.cx < psizeObj->cx)
            {
                psizeObj->cx = pci->_sizeParent.cx;
            }
            if (pci->_sizeParent.cy < psizeObj->cy)
            {
                psizeObj->cy = pci->_sizeParent.cy;
            }
        }

        //
        // If the site is absolutely positioned, only use SIZEMODE_NATURAL
        //
        if (pLayout->ElementOwner()->IsAbsolute())
        {
            pci->_smMode = SIZEMODE_NATURAL;
        }

        // Mark the site for sizing if it is already marked
        // or it is percentage sized and the view size has changed and
        // the site doesn't already know whether to resize.
        if (!pLayout->ElementOwner()->TestClassFlag(CElement::ELEMENTDESC_NOPCTRESIZE))
            pLayout->_fSizeThis =   pLayout->_fSizeThis
                                ||  pLayout->PercentSize();

        pLayout->CalcSize(pci, psizeObj);

        if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            Assert(pxMinWidth);
            *pxMinWidth = psizeObj->cy;
        }
    }
    else
    {
        pLayout->GetSize(psizeObj);
    }

    return lRet;
}


//+------------------------------------------------------------------------
//
//  Member:     AllCharsAffected
//
//  Synopsis:   Notifies text services that view of data changed
//
//-------------------------------------------------------------------------
HRESULT CFlowLayout::AllCharsAffected()
{
    CNotification   nf;

    nf.CharsResize( GetContentFirstCp(),
                      GetContentTextLength(),
                      GetFirstBranch() );
    GetContentMarkup()->Notify(nf);
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CommitChanges
//
//  Synopsis:   Commit any outstanding text changes to the display
//
//-------------------------------------------------------------------------
DeclareTag(tagLineBreakCheckOnCommit, "Commit", "Disable Line break check on commit");

void
CFlowLayout::CommitChanges(
    CCalcInfo * pci)
{
    long            cp;
    long            cchOld;
    long            cchNew;

    //
    //  Ignore unnecessary or recursive requests
    //
    if (!IsDirty() || (IsDisplayNone() && (Tag() != ETAG_BODY)))
        goto Cleanup;

    //
    //  Reset dirty state (since changes they are now being handled)
    //

    cp       = Cp() + GetContentFirstCp();
    cchOld   = CchOld();
    cchNew   = CchNew();

    CancelChanges();

    WHEN_DBG(Lock());

    //
    //  Recalculate the display to account for the pending changes
    //

    {
        CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
        CSaveCalcInfo   sci(pci, this);

        pci->EnsureFilterOpen();

        GetDisplay()->UpdateView(pci, cp, cchOld, cchNew);
    }

    //
    //  Fire a "content changed" notification (to our host)
    //

    OnTextChange();

    WHEN_DBG(Unlock());

Cleanup:
    return;
}

//+------------------------------------------------------------------------
//
//  Member:     CancelChanges
//
//  Synopsis:   Cancel any outstanding text changes to the display
//
//-------------------------------------------------------------------------
void
CFlowLayout::CancelChanges()
{
    Reset();
}

//+------------------------------------------------------------------------
//
//  Member:     IsCommitted
//
//  Synopsis:   Verify that all changes are committed
//
//-------------------------------------------------------------------------
BOOL
CFlowLayout::IsCommitted()
{
    return !IsDirty();
}


extern BOOL IntersectRgnRect(HRGN hrgn, RECT *prc, RECT *prcIntersect);

//+---------------------------------------------------------------------------
//
//  Member:     Draw
//
//  Synopsis:   Paint the object.
//
//----------------------------------------------------------------------------

void
CFlowLayout::Draw(CFormDrawInfo *pDI)
{
    if (!FExternalLayout())
    {
        GetDisplay()->Render(pDI, pDI->_rc, pDI->_rcClip, -1);
    }
    else
    {
        if (_pQuillGlue)
            _pQuillGlue->Render(pDI, pDI->_rc, pDI->_rcClip, -1);
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     GetPositionInFlow
//
//  Synopsis:   Return the position of a layout derived from its position within
//              the document text flow
//
//  Arguments:  pElement - element to position
//              ppt      - Returned top/left (in parent content relative coordinates)
//
//-----------------------------------------------------------------------------
void
CFlowLayout::GetPositionInFlow(
    CElement *  pElement,
    CPoint   *  ppt)
{
    CLinePtr rp(&_dp);
    CTreePos *  ptpStart;

    Assert(pElement);
    Assert(ppt);

    if(pElement->IsRelative() && !pElement->HasLayout())
    {
        GetFlowPosition(pElement, ppt);
    }
    else
    {
        ppt->x = ppt->y = 0;

        // get the tree extent of the element of the layout passed in
        pElement->GetTreeExtent(&ptpStart, NULL);

        if (!FExternalLayout())
        {
            Verify(_dp.RenderedPointFromTp(ptpStart->GetCp(), ptpStart, FALSE, *ppt, &rp, TA_TOP) >= 0);

            if(pElement->HasLayout())
            {
                ppt->y += pElement->GetLayoutPtr()->GetYProposed();
            }
            ppt->y += rp->GetYTop();
        }
        else
        {
            if (_pQuillGlue)
            {
                RECT rect;
                _pQuillGlue->GetCharacterMetrics(ptpStart->GetCp(), 0, &rect, NULL, NULL);
                ppt->x = rect.left;
                ppt->y = rect.top;
            }
        }
    }
}

//----------------------------------------------------------------------------
//
//  Member:     CFlowLayout::BranchFromPoint, public
//
//  Synopsis:   Does a hit test against our object, determining where on the
//              object it hit.
//
//  Arguments:  [pt]           -- point to hit test in local coordinates
//              [ppNodeElement]   -- return the node hit
//
//  Returns:    HTC
//
//  Notes:      The node returned is guaranteed to be in the tree
//              so it is legal to look at the parent for this element.
//
//----------------------------------------------------------------------------

HTC
CFlowLayout::BranchFromPoint(
    DWORD            dwFlags,
    POINT            pt,
    CTreeNode **     ppNodeBranch,
    HITTESTRESULTS * presultsHitTest)
{
    HTC           htc = HTC_YES;
    CLinePtr      rp(&_dp);
    CTreePos  *   ptp = NULL;
    LONG          cp;
    BOOL          fPsuedoHit;
    BOOL fAllowEOL          = dwFlags & HT_ALLOWEOL              ? TRUE  : FALSE;
    BOOL fIgnoreBeforeAfter = dwFlags & HT_DONTIGNOREBEFOREAFTER ? FALSE : TRUE;
    BOOL fExactFit          = dwFlags & HT_NOEXACTFIT            ? FALSE : TRUE;
    BOOL fVirtual           = dwFlags & HT_VIRTUALHITTEST        ? TRUE  : FALSE;

    Assert(ElementOwner()->IsVisible(FALSE));

    *ppNodeBranch = NULL;

    if (!FExternalLayout())
    {
        if ((cp = _dp.CpFromPoint(pt, &rp, &ptp, NULL,
                                  fAllowEOL, fIgnoreBeforeAfter, fExactFit,
                                  &presultsHitTest->fRightOfCp, &fPsuedoHit)) == -1 ) // fExactFit=TRUE to look at whole characters
        {
            htc = HTC_NO;
            goto Cleanup;
        }
    }
    else
    {
        if (!_pQuillGlue ||
            ((cp = _pQuillGlue->CpFromPoint(pt, &rp, &ptp, NULL,
                                  fAllowEOL, fIgnoreBeforeAfter, fExactFit,
                                  &presultsHitTest->fRightOfCp, &fPsuedoHit)) == -1 )) // fExactFit=TRUE to look at whole characters
        {
            htc = HTC_NO;
            goto Cleanup;
        }
    }

    if (   cp < GetContentFirstCp()
        || cp > GetContentLastCp()
       )
    {
        htc = HTC_NO;
        goto Cleanup;
    }

    presultsHitTest->cpHit  = cp;
    if (!FExternalLayout())
    {
        presultsHitTest->iliHit = rp;
        presultsHitTest->ichHit = rp.RpGetIch();
    }
    htc = BranchFromPointEx(pt, rp, ptp, NULL, ppNodeBranch, fPsuedoHit,
                            &presultsHitTest->fWantArrow,
                            fIgnoreBeforeAfter, fVirtual);

Cleanup:
    if (htc != HTC_YES)
        presultsHitTest->fWantArrow = TRUE;
    return htc;
}

extern BOOL PointInRectAry(POINT pt, CStackDataAry<RECT, 1> &aryRects);

HTC
CFlowLayout::BranchFromPointEx(
    POINT           pt,
    CLinePtr  &     rp,
    CTreePos  *     ptp,
    CTreeNode *     pNodeRelative,
    CTreeNode **    ppNodeBranch,
    BOOL            fPsuedoHit,
    BOOL *          pfWantArrow,
    BOOL            bIgnoreBeforeAfter,
    BOOL            fVirtual)
{
    HTC         htc = HTC_YES;
    CLayout   * pLayout;
    CTreeNode * pNode = NULL;

    // Get the branch corresponding to the cp hit.
    Assert(ptp);
    pNode = ptp->GetBranch();
    *pfWantArrow = FALSE;

    if (bIgnoreBeforeAfter && !FExternalLayout())
    {
        CMarkup *     pMarkup = GetContentMarkup();
        LONG          xPos;
        LONG          cpClipStart, cpClipFinish;
        CStackDataAry<RECT, 1> aryRects(Mt(CFlowLayoutBranchFromPointEx_aryRects_pv));
        const CCharFormat    * pCF = pNode->GetCharFormat();
        const CParaFormat    * pPF = pNode->GetParaFormat();
        BOOL          fRTLDisplay, fRTLLine;
        LONG          xRelOffset = 0;
        LONG          yRelOffset = 0;
        DWORD         dwFlags = 0;

        fRTLDisplay = GetDisplay()->IsRTL();
        fRTLLine = pPF->HasRTL(SameScope(pNode, ElementOwner()));

        if (!pNodeRelative)
        {
            CCalcInfo     CI(this);
            pNodeRelative = pMarkup->SearchBranchForBlockElement(pNode, this);
            pNode->GetRelTopLeft(ElementContent(), &CI, &xRelOffset, &yRelOffset);
        }
        else
        {
            dwFlags |= CDisplay::RFE_NONRELATIVE;
        }

        xPos = pt.x;

        // If we hit the white space before or after a line,
        // then we will be positioned at the last character on the line. This
        // means that the element we have hit is really the element above
        // that character. What we need however, is the block element
        // above that character.
        cpClipStart = cpClipFinish = GetContentFirstCp();
        rp.RpBeginLine();
        cpClipStart += rp.GetCp();
        rp.RpEndLine();
        cpClipFinish += rp.GetCp();

        _dp.RegionFromElement(pNodeRelative->Element(), &aryRects, NULL,
                                NULL, dwFlags, cpClipStart, cpClipFinish);
        if (!PointInRectAry(pt, aryRects))
        {
            pNode = NULL;
            htc = HTC_NO;
            goto Cleanup;
        }

        // If we are within the region of the element but outside
        // the actual text of the line (last line in a paragraph),
        // then we claim that the para was hit.
        // NOTE: That we do not test for relative elements here.
        // That is because if the rel element was a phrase elt
        // and we were not within the text range then the above
        // PointInRectAry() check would have failed. If the rel
        // elt were a block element, then we would find that block
        // element here.
        else if ( (!fRTLDisplay && (xPos <  rp->GetTextLeft()  + xRelOffset ||
                             xPos >= rp->GetTextRight() + xRelOffset )) ||
                  (fRTLDisplay  && (-xPos < rp->GetRTLTextRight() + xRelOffset ||
                             -xPos >= rp->GetRTLTextLeft() + xRelOffset ))
                )
        {
            pNode = pMarkup->SearchBranchForBlockElement(pNode, this);
            *pfWantArrow = TRUE;
        }

        // In our quest to hit test bullets as list items, we have
        // CpFromPoint return the cp of the first character of the list item.
        // However, if that item is covered by elements which are below the
        // <li> then we have not really hit them since  we have hit the
        // bullet. This hack determines if we have hit a line with a bullet
        // in it, and if so, if the area occupied by the bullet has been hit.
        // If both the conditions are satisfied, we walk up the tree to find
        // the first list item element and claim that it was that element
        // which was hit.
        if (rp->_fHasBulletOrNum)
        {
            if(fRTLDisplay)
                xPos = -xPos;
            // Is it in the area occupied by the bullet?
            if (!fRTLLine ? xPos < rp->GetTextLeft()  + xRelOffset
                      : xPos > rp->_xLineWidth - rp->GetRTLTextRight() + xRelOffset)
            {
                // Walk the parent chain till we hit the list item element.
                while (!pNode->Element()->HasFlag(TAGDESC_LISTITEM))
                {
                    pNode = pNode->Parent();

                    // We should find the list item element, since the line
                    // has the _fHasBulletOrNum flag on.
                    Assert(pNode);
                }
                *pfWantArrow = TRUE;
            }
        }

        //
        // Lets say a phrase element was hit, but the hit was a psuedo hit.
        // (A psuedo hit is when the cursor is on the line, but not on the
        // character). If its a psuedo hit and we are on the block element
        // (decided above), then we have to say that the block elt containing
        // the phrase element was hit. However, if the phrase elt was a
        // relatively positioned elt, (and we have a psuedo hit), then the
        // block element is not hit -- ie no elt is hit.
        //
        if (!*pfWantArrow && fPsuedoHit)
        {
            if (pNodeRelative->Element()->IsBlockElement())
            {
                pNode = pNodeRelative;
                *pfWantArrow = TRUE;
            }
            else
            {
                pNode = NULL;
                htc = HTC_NO;
                goto Cleanup;
            }
        }
    }

    if (pNode && DifferentScope(pNode, ElementOwner()))
    {
        // some element is hit, need to test element visibility.
        // If the element is not visible, should walk upward through
        // parent chain until either we hit the CFlowLayout or one element
        // which is visible.
        //
        CCharFormat * pCF;

        while (DifferentScope(pNode, ElementOwner()))
        {
            pCF = (CCharFormat *) pNode->GetCharFormat();
            if (pCF->IsDisplayNone() || pCF->IsVisibilityHidden())
            {
                pNode = pNode->Parent();
            }
            else
                break;
        }
        if (SameScope(pNode, ElementOwner()))
        {
            htc      = HTC_NO;
            pNode = NULL;
            goto Cleanup;
        }
    }

    //
    // If the element is a site, it might not be where we think it is if
    // it's a region or an olesite. The olesite might be a non-rectangular
    // ocx, so we need to ask it.  Double check.
    //
    pLayout = pNode->GetLayout();
    if (pLayout &&
        pLayout != this &&
        (!pNode->IsPositionStatic() ||
        pNode->Element()->TestClassFlag(CElement::ELEMENTDESC_OLESITE)))
    {
        CTreeNode * pNodeElement2;
        CMessage   msgTemp;

        msgTemp.pt = pt;
        htc = pLayout->ElementOwner()->HitTestPoint(&msgTemp, &pNodeElement2, 0);
        if (htc == HTC_NO)
        {
            pNode = NULL;
        }
    }

Cleanup:
    *ppNodeBranch = pNode;
    return htc;
}

//+------------------------------------------------------------------------
//
//  Member:     GetFirstLayout
//
//  Synopsis:   Enumeration method to loop thru children (start)
//
//  Arguments:  [pdw]       cookie to be used in further enum
//              [fBack]     go from back
//
//  Returns:    Layout
//
//-------------------------------------------------------------------------
CLayout *
CFlowLayout::GetFirstLayout(DWORD * pdw, BOOL fBack, BOOL fRaw)
{
    Assert(!fRaw);

    if (ElementOwner()->GetFirstBranch())
    {
        CChildIterator * pLayoutIterator = new
                CChildIterator(
                    ElementOwner(),
                    NULL,
                    CHILDITERATOR_USELAYOUT);
        * pdw = DWORD(pLayoutIterator);
        return CFlowLayout::GetNextLayout(pdw, fBack, fRaw);
    }
    else
    {
        // If CTxtSite is not in the tree, no need to walk through
        // CChildIterator
        //
        * pdw = DWORD(NULL);
        return NULL;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     GetNextLayout
//
//  Synopsis:   Enumeration method to loop thru children
//
//  Arguments:  [pdw]       cookie to be used in further enum
//              [fBack]     go from back
//
//  Returns:    Layout
//
//-------------------------------------------------------------------------
CLayout *
CFlowLayout::GetNextLayout(DWORD * pdw, BOOL fBack, BOOL fRaw)
{
    CLayout * pLayout = NULL;

    Assert(!fRaw);

    {
        CChildIterator * pLayoutWalker =
                        (CChildIterator *) (* pdw);
        if (pLayoutWalker)
        {
            CTreeNode * pNode = fBack ? pLayoutWalker->PreviousChild()
                                    : pLayoutWalker->NextChild();
            pLayout = pNode ? pNode->GetLayout() : NULL;
        }
    }
    return pLayout;
}


//+---------------------------------------------------------------------------
//
// Member:      ContainsChildLayout
//
//----------------------------------------------------------------------------
BOOL
CFlowLayout::ContainsChildLayout(BOOL fRaw)
{
    Assert(!fRaw);

    {
        DWORD     dw;
        CLayout * pLayout = GetFirstLayout(&dw, FALSE, fRaw);
        ClearLayoutIterator(dw, fRaw);
        return pLayout ? TRUE : FALSE;
    }
}


//+---------------------------------------------------------------------------
//
// Member:      ClearLayoutIterator
//
//----------------------------------------------------------------------------
void
CFlowLayout::ClearLayoutIterator(DWORD dw, BOOL fRaw)
{
    if (!fRaw)
    {
        CChildIterator * pLayoutWalker = (CChildIterator *) dw;
        if (pLayoutWalker)
            delete pLayoutWalker;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::GetElementsInZOrder, public
//
//  Synopsis:   Gets a complete list of elements which we are responsible for
//              (for things like painting and hit-testing), with the list
//              sorted by ZOrder (index 0 = bottom-most element).
//
//  Arguments:  [paryElements] -- Array to fill with elements.
//              [prcDraw]      -- Rectangle which elements must intersect with
//                                to be included in this list. Can be NULL.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::GetElementsInZOrder(
    CPtrAry<CElement *> *   paryElements,
     CElement *             pElementThis,
     RECT *                 prcDraw,
     HRGN                   hrgn,
     BOOL                   fIncludeNotVisible/* ==FALSE */)
{
    CLayout *   pLayout;
    DWORD       dw = 0;
    BOOL        fRegionFound = FALSE;
    HRESULT     hr = S_OK;
    CDoc *      pDoc;

    Assert(pElementThis);

    //
    // First, add all our direct children that intersect the draw rect
    // (This includes in-flow sites and those with relative positioning)
    //

    if (pElementThis == ElementOwner())
    {
        for (pLayout = GetFirstLayout(&dw); pLayout; pLayout = GetNextLayout(&dw))
        {
            // If this site is positioned then it will be added in the lower loop.
            if (pLayout->GetFirstBranch()->IsPositionStatic())
            {
                hr = THR(paryElements->Append(pLayout->ElementOwner()));
                if (hr)
                    goto Cleanup;
            }

        }
    }

    //
    // Next, add absolutely positioned elements for which this site is the
    // RenderParent.
    //

    pDoc = Doc();
    if (pDoc->_fRegionCollection && ElementOwner()->IsZParent())
    {
        long        cpElemStart;
        long        cpMax = GetDisplay()->GetMaxCpCalced();
        long        lIndex;
        long        lArySize;
        CElement *  pElement;
        CTreeNode * pNode;
        CCollectionCache * pCollectionCache;

        hr = THR(pDoc->PrimaryMarkup()->EnsureCollectionCache(CMarkup::REGION_COLLECTION));
        if (!hr)
        {
            pCollectionCache = pDoc->PrimaryMarkup()->CollectionCache();

            lArySize = pCollectionCache->SizeAry(CMarkup::REGION_COLLECTION);

            for (lIndex = 0; lIndex < lArySize; lIndex++)
            {
                hr = THR(pCollectionCache->GetIntoAry(CMarkup::REGION_COLLECTION,
                                                              lIndex,
                                                              &pElement));

                Assert(pElement);
                pNode = pElement->GetFirstBranch();

                cpElemStart = pElement->GetFirstCp();

                if (!pElement->IsPositionStatic() &&
                    pElementThis == pNode->ZParent())
                {
                    CLayout * pLayoutParent;

                    pLayout = pElement->GetLayout();

                    if (pLayout)
                    {
                        if (    !fIncludeNotVisible
                            &&  !pLayout->ElementOwner()->IsVisible(FALSE))
                        {
                            continue;
                        }
                    }
                    else
                    {
                        const CCharFormat *pCF = pNode->GetCharFormat();

                        // BUGBUG -- will this mess up visibility:visible
                        // for child elements of pElement? (lylec)
                        //
                        if (    pCF->IsVisibilityHidden()
                            ||  pCF->IsDisplayNone())
                        {
                            continue;
                        }
                    }

                    pLayoutParent = pElement->GetParentLayout();

                    // (srinib) Table lies to its parent layout that it is calced until
                    // it sees an end table. So tablecell's containing positioned elements
                    // have to take care of this issue, because they may not be calced yet.

                    // If the parent layout has not calc the current element
                    if( cpMax <= cpElemStart ||
                        (pLayoutParent &&
                         pLayoutParent->IsFlowLayout() &&
                         pLayoutParent->IsFlowLayout()->GetDisplay()->GetMaxCpCalced() <= cpElemStart))
                    {
/*
BUGBUG: This no longer should apply...does any of this? (brendand)
                        if (pLayout)
                            pLayout->_fForceInval = TRUE;
*/
                        continue;
                    }

                    fRegionFound = TRUE;

                    hr = paryElements->Append(pElement);
                    if (hr)
                        goto Cleanup;
                }
            }
        }
    }

    if (fRegionFound)
    {
        qsort(*paryElements,
              paryElements->Size(),
              sizeof(CElement *),
              CompareElementsByZIndex);
    }

Cleanup:
    ClearLayoutIterator(dw, FALSE);
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     SetZOrder
//
//  Synopsis:   set z order for site
//
//  Arguments:  [pLayout]   set z order for this layout
//              [zorder]    to set
//              [fUpdate]   update windows and invalidate
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CFlowLayout::SetZOrder(CLayout * pLayout, LAYOUT_ZORDER zorder, BOOL fUpdate)
{
    HRESULT     hr = S_OK;

    if (Doc()->TestLock(FORMLOCK_ARYSITE))
    {
        hr = CTL_E_METHODNOTAPPLICABLE;
        goto Cleanup;
    }

    if (fUpdate)
    {
        Doc()->FixZOrder(pLayout->ElementOwner());

        Invalidate();
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     IsElementBlockInContext
//
//  Synopsis:   Return whether the element is a block in the current context
//              In general: Elements, if marked, are blocks and sites are not.
//              The exception is CFlowLayouts which are blocks when considered
//              from within themselves and are not when considered
//              from within their parent
//
//  Arguments:
//              [pElement] Element to examine to see if it should be
//                         treated as no scope in the current context.
//
//----------------------------------------------------------------------------

BOOL
CFlowLayout::IsElementBlockInContext (CElement *pElement)
{
    // The given element is block element by default or
    // a block element within its scope.
    if (   pElement->IsBlockElement()
        || pElement->Tag() == ETAG_ROOT
        || (pElement->HasFlowLayout()
            && pElement == ElementContent()))
        return TRUE;
    else
        return FALSE;

}


//+------------------------------------------------------------------------
//
//  Member:     PreDrag
//
//  Synopsis:   Perform stuff before drag/drop occurs
//
//  Arguments:  ppDO    Data object to return
//              ppDS    Drop source to return
//
//-------------------------------------------------------------------------

HRESULT
CFlowLayout::PreDrag(
    DWORD           dwKeyState,
    IDataObject **  ppDO,
    IDropSource **  ppDS)
{
    HRESULT hr = S_OK;

    CDoc* pDoc = Doc();

    CSelDragDropSrcInfo *   pDragInfo;

    // Setup some info for drag feedback
    Assert(! pDoc->_pDragDropSrcInfo);
    pDragInfo = new CSelDragDropSrcInfo( pDoc, ElementContent() ) ;

    if (!pDragInfo)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    hr = THR( pDragInfo->GetDataObjectAndDropSource( ppDO, ppDS ) );
    if ( hr )
        goto Cleanup;

    pDoc->_pDragDropSrcInfo = pDragInfo;


Cleanup:

    RRETURN(hr);   

}


//+------------------------------------------------------------------------
//
//  Member:     PostDrag
//
//  Synopsis:   Handle the result of an OLE drag/drop operation
//
//  Arguments:  hrDrop      The hr that DoDragDrop came back with
//              dwEffect    The effect of the drag/drop
//
//-------------------------------------------------------------------------

HRESULT
CFlowLayout::PostDrag(HRESULT hrDrop, DWORD dwEffect)
{
#ifdef MERGEFUN // Edit team: figure out better way to send sel-change notifs
    CCallMgr                callmgr(GetPed());
#endif
    HRESULT                 hr;

    CParentUndo             pu( Doc() );

    hr = hrDrop;

    if( IsEditable() )
        pu.Start( IDS_UNDODRAGDROP );

    if (hr == DRAGDROP_S_CANCEL)
    {
        //
        // marka - BUGBUG we may have to restore selection here.
        // for now I don't think we need to.
        //

        //Invalidate();
        //pSel->Update(FALSE, this);

        hr = S_OK;
        goto Cleanup;
    }

    if (hr != DRAGDROP_S_DROP)
        goto Cleanup;

    hr = S_OK;

    switch(dwEffect)
    {
    case DROPEFFECT_NONE:
    case DROPEFFECT_COPY:
        Invalidate();
        // pSel->Update(FALSE, this);
        break ;

    case DROPEFFECT_LINK:
        break;

    case 3: // BugBug - this is for TriEdit - faking out a position with Drag & Drop.
        {
            Assert(Doc()->_pDragDropSrcInfo);
            if (Doc()->_pDragDropSrcInfo)
            {
                CSelDragDropSrcInfo * pDragInfo;

                Assert(DRAGDROPSRCTYPE_SELECTION == Doc()->_pDragDropSrcInfo->_srcType);

                pDragInfo = DYNCAST(CSelDragDropSrcInfo, Doc()->_pDragDropSrcInfo);
                pDragInfo->PostDragSelect();
            }

        }
        break;

    case DROPEFFECT_MOVE:
        Assert(ElementOwner()->IsEditable());

        if (Doc()->_fSlowClick)
            goto Cleanup;

        Assert(Doc()->_pDragDropSrcInfo);
        if (Doc()->_pDragDropSrcInfo)
        {
            CSelDragDropSrcInfo * pDragInfo;

            Assert(DRAGDROPSRCTYPE_SELECTION == Doc()->_pDragDropSrcInfo->_srcType);


            pDragInfo = DYNCAST(CSelDragDropSrcInfo, Doc()->_pDragDropSrcInfo);
            pDragInfo->PostDragDelete();

#ifdef NEVER
            // marka - I'm not sure we still have to do this
            // someone seems to have thought it important enough to write an essay here
            // I'll leave it until scrolling is re-enabled on drag again.
            //

            // The update that happens implicitly by the update of the range may
            // have the effect of scrolling the window. This in turn may have the
            // effect in the drag drop case of scrolling non-inverted text into
            // the place where the selection was. The logic in the selection
            // assumes that the selection is inverted and so reinverts it to turn
            // off the selection. Of course, it is obvious what happens in the
            // case where non-inverted text is scrolled into the selection area.
            // To simplify the processing here, we just say the whole window is
            // invalid so we are guaranteed to get the right painting for the
            // selection.
            // BUGBUG: (ricksa) This solution does have the disadvantage of causing
            // a flash during drag and drop. We probably want to come back and
            // investigate a better way to update the screen.
            Invalidate();
#endif

        }
        break;

    default:
        Assert(FALSE && "Unrecognized drop effect");
        break;
    }

Cleanup:

    pu.Finish(hr);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     Drop
//
//  Synopsis:
//
//----------------------------------------------------------------------------
#define DROPEFFECT_ALL (DROPEFFECT_NONE | DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK)

HRESULT
CFlowLayout::Drop(
    IDataObject *   pDataObj,
    DWORD           grfKeyState,
    POINTL          ptlScreen,
    DWORD *         pdwEffect)
{
    HRESULT             hr        = S_OK;


    CDoc* pDoc = Doc();
    DWORD               dwAllowed = *pdwEffect;
    TCHAR               pszFileType[4] = _T("");
    CPoint pt;
    IMarkupServices* pMarkupServices = NULL;

    hr = THR ( pDoc->QueryInterface( IID_IMarkupServices, ( void** ) & pMarkupServices ));
    if ( hr )
        goto Cleanup;

    // Can be null if dragenter was handled by script
    if (!_pDropTargetSelInfo)
    {
        *pdwEffect = DROPEFFECT_NONE ;
        return S_OK;
    }

    //
    // BUGBUG - put this in a resource
    //

    hr = THR( pMarkupServices->BeginUndoUnit(_T ("Drag & Drop")));
    if ( hr )
        goto Cleanup;

    //
    // Find out what the effect is and execute it
    // If our operation fails we return DROPEFFECT_NONE
    //
    DragOver(grfKeyState, ptlScreen, pdwEffect);

    IGNORE_HR(DropHelper(ptlScreen, dwAllowed, pdwEffect, pszFileType));

    if (DROPEFFECT_NONE == *pdwEffect)
        goto Cleanup;

    if (Doc()->_fSlowClick && *pdwEffect == DROPEFFECT_MOVE)
    {
        *pdwEffect = DROPEFFECT_NONE ;
        goto Cleanup;
    }

    //
    // We're all ok at this point. We delegate the handling of the actual
    // drop operation to the DropTarget.
    //


    pt.x = ptlScreen.x;
    pt.y = ptlScreen.y;
    ScreenToClient( pDoc->_pInPlace->_hwnd, (POINT*) & pt );

    //
    // We DON'T TRANSFORM THE POINT, AS MOVEPOINTERTOPOINT is in Global Coords
    //
    _pDropTargetSelInfo->Drop( this, pDataObj, grfKeyState, pt, pdwEffect );


Cleanup:
    // Erase any feedback that's showing.
    DragHide();

    Assert(_pDropTargetSelInfo);
    delete _pDropTargetSelInfo;
    _pDropTargetSelInfo = NULL;

    if ( pMarkupServices )
        IGNORE_HR( pMarkupServices->EndUndoUnit() );

    ReleaseInterface( pMarkupServices );

    RRETURN(hr);

}


//+---------------------------------------------------------------------------
//
//  Member:     DragLeave
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::DragLeave()
{
#ifdef MERGEFUN // Edit team: figure out better way to send sel-change notifs
    CCallMgr        callmgr(GetPed());
#endif
    HRESULT         hr        = S_OK;

    if (!_pDropTargetSelInfo)
        goto Cleanup;

    hr = THR(super::DragLeave());

Cleanup:
    if (_pDropTargetSelInfo)
    {
        delete _pDropTargetSelInfo;
        _pDropTargetSelInfo = NULL;
    }
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     ParseDragData
//
//  Synopsis:   Drag/drop helper override
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::ParseDragData(IDataObject *pDO)
{
    DWORD   dwFlags = 0;
    HRESULT hr;

    // Start with flags set to default values.

    Doc()->_fOKEmbed = FALSE;
    Doc()->_fOKLink = FALSE;
    Doc()->_fFromCtrlPalette = FALSE;

    if (!ElementOwner()->IsEditable() || !ElementOwner()->IsEnabled())
    {
        // no need to do anything else, bcos we're read only.
        hr = S_FALSE;
        goto Cleanup;
    }


    // Allow only plain text to be pasted in input text controls
    if (!ElementOwner()->GetFirstBranch()->GetContainer()->HasFlag(TAGDESC_ACCEPTHTML) &&
            pDO->QueryGetData(&g_rgFETC[iAnsiFETC]) != NOERROR)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    {
        CLightDTEngine ldte;

        hr = THR(ldte.GetDataObjectInfo(pDO, &dwFlags));
        if (hr)
            goto Cleanup;
    }

    if (dwFlags & DOI_CANPASTEPLAIN)
    {
        hr = S_OK;
    }
    else
    {
        hr = THR(super::ParseDragData(pDO));
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}



//+---------------------------------------------------------------------------
//
//  Member:     DrawDragFeedback
//
//  Synopsis:
//
//----------------------------------------------------------------------------

void
CFlowLayout::DrawDragFeedback()
{
    Assert(_pDropTargetSelInfo);

    _pDropTargetSelInfo->DrawDragFeedback();

}

//+------------------------------------------------------------------------
//
//  Member:     InitDragInfo
//
//  Synopsis:   Setup a struct to enable drawing of the drag feedback
//
//  Arguments:  pDO         The data object
//              ptlScreen   Screen loc of obj.
//
//  Notes:      This assumes that the DO has been parsed and
//              any appropriate data on the form has been set.
//
//-------------------------------------------------------------------------

HRESULT
CFlowLayout::InitDragInfo(IDataObject *pDO, POINTL ptlScreen)
{

    CPoint      pt;
    pt.x = ptlScreen.x;
    pt.y = ptlScreen.y;

    ScreenToClient( Doc()->_pInPlace->_hwnd, (CPoint*) & pt );


    //
    // We DON'T TRANSFORM THE POINT, AS MOVEPOINTERTOPOINT is in Global Coords
    //

    Assert(!_pDropTargetSelInfo);
    _pDropTargetSelInfo = new CDropTargetInfo( Doc(), pt );
    if (!_pDropTargetSelInfo)
        RRETURN(E_OUTOFMEMORY);


    return S_OK;

}


//+---------------------------------------------------------------------------
//
//  Member:     UpdateDragFeedback
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::UpdateDragFeedback(POINTL ptlScreen)
{
    CSelDragDropSrcInfo *pDragInfo = NULL;
    CDoc* pDoc = Doc();
    CPoint      pt;

    // Can be null if dragenter was handled by script
    if (!_pDropTargetSelInfo)
        return S_OK;

    pt.x = ptlScreen.x;
    pt.y = ptlScreen.y;

    ScreenToClient( pDoc->_pInPlace->_hwnd, (POINT*) & pt );

    //
    // We DON'T TRANSFORM THE POINT, AS MOVEPOINTERTOPOINT is in Global Coords
    //

    if ( ( pDoc->_fIsDragDropSrc )  &&
         ( pDoc->_pDragDropSrcInfo) &&
         ( pDoc->_pDragDropSrcInfo->_srcType == DRAGDROPSRCTYPE_SELECTION ) )
    {
        pDragInfo = DYNCAST( CSelDragDropSrcInfo, pDoc->_pDragDropSrcInfo );
    }
    _pDropTargetSelInfo->UpdateDragFeedback( pt, pDragInfo  );

    TraceTag((tagUpdateDragFeedback, "Update Drag Feedback: pt:%ld,%ld After Transform:%ld,%ld\n", ptlScreen.x, ptlScreen.y, pt.x, pt.y ));

#ifdef NEVER

#ifdef MERGEFUN // Edit team: figure out better way to send sel-change notifs
    CCallMgr        callmgr(GetPed());
#endif
    POINT           pt;
    long            cpCur;

    pt.x = ptlScreen.x;
    pt.y = ptlScreen.y;

    // first, find out where we are
    ::ScreenToClient(Doc()->_pInPlace->_hwnd, &pt);
// BUGBUG: DISPLAY_TREE - This is not right - Need to figure out what to pass CpFromPoint etc. (brendand)
    cpCur = _dp.CpFromPointReally(pt, NULL, NULL, NULL, FALSE);

    if (cpCur == GetContentLastCp())
    {
        CTxtPtr tp(GetContentMarkup(), cpCur);

        if (WCH_TXTSITEEND == tp.GetPrevChar())
        {
            cpCur--;
        }
    }

    // If feedback is currently displayed and the feedback location
    // is changing, then erase the current feedback.
    if (cpCur != _pDropTargetSelInfo->_cpCur)
    {
        DrawDragFeedback();
    }

    _pDropTargetSelInfo->_cpCur = cpCur;

    // Draw new feedback.
    if (!Doc()->_fDragFeedbackVis)
    {
        DrawDragFeedback();
    }

    Doc()->_fSlowClick = FALSE;

    // Update state info on whether this is a "slow click"
    if (_fIsDragDropSrc && Doc()->_pDragDropSrcInfo)
    {
        Assert(Doc()->_pDragDropSrcInfo);
        if (Doc()->_pDragDropSrcInfo->_srcType == DRAGDROPSRCTYPE_SELECTION)
        {
            CSelDragDropSrcInfo *   pDragInfo;

            pDragInfo = DYNCAST(CSelDragDropSrcInfo, Doc()->_pDragDropSrcInfo);
            if (cpCur >= pDragInfo->_cpMin && cpCur <= pDragInfo->_cpMax)
            {
                Doc()->_fSlowClick = TRUE;
            }
        }
    }
#endif
    return S_OK;
}


ExternTag(tagMsoCommandTarget);

//+-------------------------------------------------------------------------
//
//  Method:     QueryStatus
//
//  Synopsis:   Called to discover if a given command is supported
//              and if it is, what's its state.  (disabled, up or down)
//
//--------------------------------------------------------------------------

HRESULT
CFlowLayout::QueryStatus(
        GUID *          pguidCmdGroup,
        ULONG           cCmds,
        MSOCMD          rgCmds[],
        MSOCMDTEXT *    pcmdtext,
        BOOL            fStopBobble)
{
    TraceTag((tagMsoCommandTarget, "CFlowLayout::QueryStatus"));

    Assert(ElementOwner()->IsCmdGroupSupported(pguidCmdGroup));
    Assert(cCmds == 1);

    MSOCMD *    pCmd = &rgCmds[0];
    HRESULT     hr   = S_OK;
    CMarkup    *pMarkup = GetContentMarkup();
    ULONG      cmdID;

    Assert(!pCmd->cmdf);

    cmdID = ElementOwner()->IDMFromCmdID( pguidCmdGroup, pCmd->cmdID );

    switch (cmdID)
    {
    case IDM_TABORDER:
        // BUGBUG (jenlc) do we still support this command?
        //
        pCmd->cmdf = MSOCMDSTATE_UP;
        break;

    case IDM_SELECTALL:
        if (ElementOwner()->HasFlag(TAGDESC_CONTAINER) && ElementOwner()->DisallowSelection())
        {
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
        }
        break;
    }

    if (!pCmd->cmdf && (hr == S_OK || hr == MSOCMDERR_E_NOTSUPPORTED))
        hr = THR_NOTRACE(super::QueryStatus(pguidCmdGroup, 1, pCmd, pcmdtext, fStopBobble));


    RRETURN_NOTRACE(hr);
}



//+-------------------------------------------------------------------------
//
//  Method:     Exec
//
//  Synopsis:   Called to execute a given command.  If the command is not
//              consumed, it may be routed to there objects on the routing
//              chain.
//
//--------------------------------------------------------------------------
extern HRESULT DoTabOrderDlg(CBase * pBase, CDoc * pDoc, HWND hwndOwner);

HRESULT
CFlowLayout::Exec(
        GUID *       pguidCmdGroup,
        DWORD        nCmdID,
        DWORD        nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut,
        BOOL         fStopBobble)
{
    TraceTag((tagMsoCommandTarget, "CFlowLayout::Exec"));

    Assert(ElementOwner()->IsCmdGroupSupported(pguidCmdGroup));

    UINT    idm;
    HRESULT hr = MSOCMDERR_E_NOTSUPPORTED;

    switch (idm = ElementOwner()->IDMFromCmdID(pguidCmdGroup, nCmdID))
    {

    case IDM_TABORDER:
        // BUGBUG (jenlc) do we still support this command?
        //
        hr = THR_NOTRACE(DoTabOrderDlg(Doc(), Doc(), Doc()->InPlace()->_hwnd));
        ElementOwner()->SetErrorInfo(hr);
        break;
    } // end Switch(cmdId)


    if (hr == MSOCMDERR_E_NOTSUPPORTED)
        hr = THR_NOTRACE(super::Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut,
                fStopBobble));

    RRETURN_NOTRACE(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     NotifyKillSelection
//
//  Synopsis:   We need to call this because docs inside other frames need
//              to kill their selections when some doc inside one frame
//              gets focus.
//
//--------------------------------------------------------------------------

HRESULT
CFlowLayout::NotifyKillSelection()
{
    CDoc *  pDoc = Doc()->GetRootDoc();

    // If our doc is not the only doc, then we need notify
    // the other docs so that they can kill their selections
    if (pDoc != Doc())
    {
        CNotification   nf;

        nf.KillSelection(pDoc->GetPrimaryElementClient());
        pDoc->BroadcastNotify(&nf);
    }
    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     WaitForParentToRecalc
//
//  Synopsis:   Waits for a site to finish recalcing. First it waits
//              for all txtsites above this txtsite to finish recalcing.
//
//  Params:     [pSite]: The site to wait for finishing recacl.
//              [pDI]:   The DI
//
//  Return:     HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CFlowLayout::WaitForParentToRecalc(CElement *pElement, CCalcInfo * pci)
{
    HRESULT hr = S_OK;
    LONG cpElementStart, cchElement;
    LONG cpThisStart, cchThis;
    CTreePos *ptpElement;
    CTreePos *ptpThis;

    pElement->GetTreeExtent(&ptpElement, NULL);
    cpElementStart = ptpElement->GetCp();
    cchElement = pElement->GetElementCch() + 2;

    ElementOwner()->GetTreeExtent(&ptpThis, NULL);
    cpThisStart = ptpThis->GetCp();
    cchThis = ElementOwner()->GetElementCch() + 2;

    if (cpElementStart < cpThisStart ||
        cpElementStart + cchElement > cpThisStart + cchThis)
    {
        hr = S_FALSE;
        goto Cleanup;
    }
    if (cchElement < 1)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    //BUGBUG MERGEFUN (carled) this should be rewritten to take a ptp and thus
    // avoid the expensive GetCp call above.  The need to do this will be driven
    // by the perf numbers
    hr = THR(WaitForParentToRecalc(cpElementStart + cchElement, -1, pci));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     WaitForParentToRecalc
//
//  Synopsis:   Waits for a this site to finish recalcing upto cpMax/yMax.
//              If first waits for all txtsites above this to finish recalcing.
//
//  Params:     [cpMax]: The cp to calculate too
//              [yMax]:  The y position to calculate too
//              [pci]:   The CCalcInfo
//
//  Return:     HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CFlowLayout::WaitForParentToRecalc(
    LONG cpMax,     //@parm Position recalc up to (-1 to ignore)
    LONG yMax,      //@parm ypos to recalc up to (-1 to ignore)
    CCalcInfo * pci)
{
    HRESULT hr = S_OK;
    CFlowLayout *pFlowLayoutParent;

    Assert(!TestLock(CElement::ELEMENTLOCK_RECALC));

    if (!TestLock(CElement::ELEMENTLOCK_SIZING))
    {
#ifdef DEBUG
        // BUGBUG(sujalp): We should never recurse when we are not SIZING.
        // This code to catch the case in which we recurse when we are not
        // SIZING.
        CElement::CLock LockRecalc(ElementOwner(), CElement::ELEMENTLOCK_RECALC);
#endif

        pFlowLayoutParent = GetFirstBranch()->Parent()->GetFlowLayout();
        if (pFlowLayoutParent)
        {
            hr = THR(pFlowLayoutParent->WaitForParentToRecalc(ElementOwner(), pci));
            if (hr)
                goto Cleanup;
        }
    }

    if (!_dp.WaitForRecalc(cpMax, yMax, pci))
    {
        hr = S_FALSE;
        goto Cleanup;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//
//  Member:     GetNextFlowLayout
//
//  Synopsis:   Get next text site in the specified direction from the
//              specified position
//
//  Arguments:  [iDir]       -  UP/DOWN/LEFT/RIGHT
//              [ptPosition] -  position in the current txt site
//              [pElementChild] -  The child element from where this call came
//              [pcp]        -  The cp in the found site where the caret
//                              should be placed.
//              [pfCaretNotAtBOL] - Is the caret at BOL?
//
//-----------------------------------------------------------------------------
CFlowLayout *
CFlowLayout::GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp, BOOL *pfCaretNotAtBOL)
{
    CFlowLayout *pFlowLayout = NULL;   // Stores the new txtsite found in the given dirn.

    Assert(pcp);
    Assert(!pElementLayout || pElementLayout->GetParentLayout() == this);

    if (pElementLayout == NULL)
    {
        CLayout *pParentLayout = GetParentLayout();
        // By default ask our parent to get the next flowlayout.
        if (pParentLayout)
        {
            pFlowLayout = pParentLayout->GetNextFlowLayout(iDir, ptPosition, ElementOwner(), pcp, pfCaretNotAtBOL);
        }
    }
    else
    {
        CTreePos *       ptpStart;  // extent of the element
        CTreePos *       ptpFinish;

        // Start off with the txtsite found as being this one
        pFlowLayout = this;
        *pcp = 0;

        // find the extent of the element passed in.
        pElementLayout->GetTreeExtent(&ptpStart, &ptpFinish);

        switch(iDir)
        {
        case NAVIGATE_UP:
        case NAVIGATE_DOWN:
            {
                CLinePtr rp(&_dp); // The line in this site where the child lives
                POINT    pt;       // The point where the site resides.

                // find the line where the given layout lives.
                if (_dp.RenderedPointFromTp(ptpStart->GetCp(), ptpStart, FALSE, pt, &rp, TA_TOP, NULL) < 0)
                    goto Cleanup;

                // Now navigate from this line ... either up/down
                pFlowLayout = _dp.MoveLineUpOrDown(iDir, rp, ptPosition.x, pcp, pfCaretNotAtBOL);
                break;
            }
        case NAVIGATE_LEFT:
            // position ptpStart just before the child layout
            ptpStart = ptpStart->PreviousTreePos();

            if(ptpStart)
            {
                // Now let's get the txt site that's interesting to us
                pFlowLayout = ptpStart->GetBranch()->GetFlowLayout();

                // and the cp...
                *pcp = ptpStart->GetCp();
            }
            break;

        case NAVIGATE_RIGHT:
            // Position the ptpFinish just after the child layout.
            ptpFinish = ptpFinish->PreviousTreePos();

            if(ptpFinish)
            {
                // Now let's get the txt site that's interesting to us
                pFlowLayout = ptpFinish->GetBranch()->GetFlowLayout();

                // and the cp...
                *pcp = ptpFinish->GetCp();
                break;
            }
        }
    }

Cleanup:
    return pFlowLayout;
}

//+--------------------------------------------------------------------------
//
//  Member : GetChildElementTopLeft
//
//  Synopsis : CSite virtual override, the job of this function is to
//      do the actual work in reporting the top left posisiton of elements
//      that is it resposible for.
//              This is primarily used as a helper function for CElement's::
//      GetElementTopLeft.
//
//----------------------------------------------------------------------------
HRESULT
CFlowLayout::GetChildElementTopLeft(POINT & pt, CElement * pChild)
{
    Assert(pChild && !pChild->HasLayout());

    // handle a couple special cases. we won't hit
    // these when coming in from the OM, but if this fx is
    // used internally, we might. so here they are
    switch ( pChild->Tag())
    {
    case ETAG_MAP :
        {
            pt.x = pt.y = 0;
        }
        break;

    case ETAG_AREA :
        {
            RECT rectBound;
            DYNCAST(CAreaElement, pChild)->GetBoundingRect(&rectBound);
            pt.x = rectBound.left;
            pt.y = rectBound.top;
        }
        break;

    default:
        {
            CTreePos * ptpStart;
            CTreePos * ptpEnd;
            LONG       cpStart;

            pt.x = pt.y = -1;

            // get the extent of this element
            pChild->GetTreeExtent(&ptpStart, &ptpEnd);

            if (!ptpStart || !ptpEnd)
                goto Cleanup;

            cpStart = ptpStart->GetCp();

            {
                CStackDataAry<RECT, 1> aryRects(Mt(CFlowLayoutGetChildElementTopLeft_aryRects_pv));

                _dp.RegionFromElement(pChild, &aryRects, NULL,
                    NULL, FALSE, cpStart, cpStart + 1);

                if(aryRects.Size())
                {
                    pt.x = aryRects[0].left;
                    pt.y = aryRects[0].top;
                }
            }

/*
BUGBUG: Borders are no longer included with the display tree (brendand) DISPLAY_TREE
            // If we are a cell, fix up the returned pt to account for
            // vertical alignment (done via table insets)
            ELEMENT_TAG etag = ElementOwner()->Tag();
            if ( (etag == ETAG_TD) || (etag == ETAG_TH) || (etag == ETAG_CAPTION) )
            {
                CTableCellLayout *pTableCellLayout = DYNCAST(CTableCell, ElementOwner())->Layout();

                // view insets usually include border, so get rid of it
                CBorderInfo borderinfo(FALSE);  // no init
                if ( pTableCellLayout->GetCellBorderInfo(NULL, &borderinfo, FALSE) )
                {
                    pt.x -= borderinfo.aiWidths[BORDER_LEFT];
                    pt.y -= borderinfo.aiWidths[BORDER_TOP];
                }
            }
*/
        }
        break;
    }

Cleanup:
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     IsZeroWidthWrapper
//
//  Synopsis:   Helper function for CTableCell::IsZeroWidth(). We do it this
//              way to avoid build dependencies on quillsite.h and quilglue.h.
//-----------------------------------------------------------------------------
BOOL
CFlowLayout::IsZeroWidthWrapper()
{
    if (!FExternalLayout())
    {
        return (_dp.NoContent() && _dp.LineCount() <= 1);
    }
    else
    {
        return (!_pQuillGlue || _pQuillGlue->NoContent());
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     GetPositionInTxtSite
//
//  Synopsis:   For this txtsite, find the correct position for the cp to be in
//              within this txtsite. It may so happen that the ideal position
//              may be within a txtsite within this one -- handle those cases too.
//
//  Arguments:  [iDir]       -  UP/DOWN/LEFT/RIGHT
//              [ptPosition] -  position in the current txt site
//              [pcp]        -  The cp in the found site where the caret
//                              should be placed.
//              [pfCaretNotAtBOL]: Is the caret at BOL?
//
//-----------------------------------------------------------------------------
CFlowLayout *
CFlowLayout::GetPositionInFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, LONG *pcp, BOOL *pfCaretNotAtBOL)
{
    CFlowLayout  *pFlowLayout = this; // The txtsite we found ... by default us

    Assert(pcp);

    switch(iDir)
    {
    case NAVIGATE_UP:
    case NAVIGATE_DOWN:
    {
        POINT    pt = ptPosition;  // The desired position of the caret
        CLinePtr rp(&_dp);         // The line in which the point ptPosition is
        CRect    rc;               // Rect used to get the client rect

        // Be sure that the point is within this site's client rect
        RestrictPointToClientRect(&pt);

        // Construct a point within this site's client rect (based on
        // the direction we are traversing.
        GetClientRect(&rc);

        rc.MoveTo(pt);

        // Find the line within this txt site where we want to be placed.
        rp = _dp.LineFromPos(rc, (CDisplay::LFP_ZORDERSEARCH   |
                                  CDisplay::LFP_IGNOREALIGNED  |
                                  CDisplay::LFP_IGNORERELATIVE |
                                  (iDir == NAVIGATE_UP
                                        ? CDisplay::LFP_INTERSECTBOTTOM
                                        : 0)));

        if (rp < 0)
        {
            *pcp = 0;
        }
        else
        {
            // Found the line ... let's navigate to it.
            pFlowLayout = _dp.NavigateToLine(iDir, rp, pt, pcp, pfCaretNotAtBOL);
        }
        break;
    }

    case NAVIGATE_LEFT:
    {
        // We have come to this site while going left in a site outside this site.
        // So position ourselves just after the last valid character.
        *pcp = GetContentLastCp() - 1;
#ifdef DEBUG
        {
            CRchTxtPtr rtp(GetPed());
            rtp.SetCp(*pcp);
            Assert(WCH_TXTSITEEND == rtp._rpTX.GetChar());
        }
#endif
        break;
    }

    case NAVIGATE_RIGHT:
        // We have come to this site while going right in a site outside this site.
        // So position ourselves just before the first character.
        *pcp = GetContentFirstCp();
#ifdef DEBUG
        {
            CRchTxtPtr rtp(GetPed());
            rtp.SetCp(*pcp);
            Assert(IsTxtSiteBreak(rtp._rpTX.GetPrevChar()));
        }
#endif
        break;
    }

    return pFlowLayout;
}


//+---------------------------------------------------------------------------
//
//  Member:     HandleSetCursor
//
//  Synopsis:   Helper for handling set cursor
//
//  Arguments:  [pMessage]  -- message
//              [fIsOverEmptyRegion -- is it over the empty region around text?
//
//  Returns:    Returns S_OK if keystroke processed, S_FALSE if not.
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::HandleSetCursor(CMessage * pMessage, BOOL fIsOverEmptyRegion)
{
    HRESULT     hr = S_OK;
    LPCTSTR     idcNew = IDC_ARROW;
    RECT        rc;
    POINT       pt = pMessage->pt;
    LONG        xy = pt.x;

    // If we're in a top-to-bottom language flip the coords
    if(_fVertical)
    {
#ifndef DISPLAY_TREE
// BUGBUG: Oop, Aack! What to do? (brendand)
        pt.x = pt.y;
        pt.y = _rc.right - _rc.left - xy;
#endif
    }

    if (  pMessage->pNodeHit && pMessage->pNodeHit->HasLayout()
        &&  DifferentScope(pMessage->pNodeHit, ElementOwner()))
    {
        // There's a site under the mouse, so let the default handling
        // take place.
        hr = S_FALSE;
        goto Cleanup;
    }

    GetClientRect((CRect *)&rc);

    Assert(pMessage->IsContentPointValid());

    if (PtInRect(&rc, pMessage->ptContent))
    {
        if (fIsOverEmptyRegion)
        {
            idcNew = IDC_ARROW;
        }
        else
        {

            CDoc* pDoc = Doc();
            
            if (! pDoc->IsPointInSelection( pt ) )
            {
                // If CDoc is a HTML dialog, do not show IBeam cursor.

                if (ElementOwner()->IsEditable()
                    || !( pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_DIALOG)
                    ||   _fAllowSelectionInDialog

                   )
                    idcNew = IDC_IBEAM;
            }
        }
    }

#ifdef NEVER    // GetSelectionBarWidth returns 0
    else if (GetSelectionBarWidth() && PointInSelectionBar(pt))
    {
        // The selection bar is a vertical trench on the left side of the text
        // object that allows one to select multiple lines by swiping the trench
        idcNew = MAKEINTRESOURCE(IDC_SELBAR);
    }
#endif

    ElementOwner()->SetCursorStyle(idcNew);

Cleanup:

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Method:     OnSetCursor (virtual)
//
//  Synopsis:   sets cursor
//
//----------------------------------------------------------------------------
BOOL
CFlowLayout::OnSetCursor(CMessage *pMessage)
{
    if ( SameScope(ElementOwner(), pMessage->pNodeHit->GetNearestLayoutNode()) && (HTC_YES == pMessage->htc) )
        return FALSE; // cursor will be set to text caret later
    else
        return super::OnSetCursor(pMessage);
}


void
CFlowLayout::ResetMinMax()
{
    CNotification   nf;

    _fMinMaxValid        = FALSE;
    _dp._fMinMaxCalced = FALSE;
    if (_pQuillGlue)
        _pQuillGlue->ResetMinMax();

    nf.ElementMinmax(ElementOwner());
    ElementOwner()->GetMarkup()->Notify(&nf);
}

LONG
CFlowLayout::GetMaxLineWidth()
{
    return _dp.GetMaxPixelWidth() - _dp.GetCaret();
}


HRESULT BUGCALL
CFlowLayout::HandleMessage(
    CMessage * pMessage,
    CElement * pElem,
    CTreeNode *pNodeContext)
{
    HRESULT    hr    = S_FALSE;
    CDoc     * pDoc  = Doc();

    // BUGBUG: This must go into TLS!! (brendand)
    //
    static BOOL     g_fAfterDoubleClick = FALSE;

#ifdef MERGEFUN // Edit team: figure out better way to send sel-change notifs
    CCallMgr callmgr(ped);
#endif

    BOOL        fCapture;
    BOOL        fLbuttonDown;
    BOOL        fInBrowseMode = !ElementOwner()->IsEditable();
    CDispNode * pDispNode     = GetElementDispNode(NULL, FALSE);
    BOOL        fIsScroller   = (pDispNode && pDispNode->IsScroller());

    Assert(pNodeContext && pNodeContext->Element() == ElementOwner());

    //
    //  Prepare the message for this layout
    //

    PrepareMessage(pMessage);

    // First, forward mouse messages to the scrollbars (if any)
    // (Keyboard messages are handled below and then notify the scrollbar)
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
        hr = HandleScrollbarMessage(pMessage, pElem);
        if (hr != S_FALSE)
            goto Cleanup;
    }

    //
    //  In Edit mode, if no element was hit, resolve to the closest element
    //

    if (    !fInBrowseMode
        &&  !pMessage->pNodeHit
        &&  (   (   pMessage->message >= WM_MOUSEFIRST
                &&  pMessage->message <= WM_MOUSELAST)
            ||  pMessage->message == WM_SETCURSOR
            ||  pMessage->message == WM_CONTEXTMENU)
        &&  (   !pElem
            ||  pElem->GetFirstBranch()->GetNearestLayout() == this))
    {
        CTreeNode * pNode;
        HTC         htc;

        pMessage->resultsHitTest.fWantArrow = FALSE;
        pMessage->resultsHitTest.fRightOfCp = FALSE;

        htc = BranchFromPoint(HT_DONTIGNOREBEFOREAFTER,
                            pMessage->ptContent,
                            &pNode,
                            &pMessage->resultsHitTest);
        if (HTC_YES == htc && pNode)
        {
            pMessage->SetNodeHit(pNode);
        }
    }

    if (!pDoc->_pElementOMCapture || (pMessage->message < WM_MOUSEFIRST)
        || (pMessage->message > WM_MOUSELAST))
    {
        hr = pDoc->HandleSelectionMessage( pMessage, FALSE ) ;
        if (hr != S_FALSE)
            goto Cleanup;
    }

    switch(pMessage->message)
    {
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        g_fAfterDoubleClick = TRUE;
        hr = HandleButtonDblClk(pMessage);
        ElementOwner()->TakeCapture(TRUE);
        break;

    case WM_MBUTTONDOWN:
        if (    Tag() == ETAG_BODY
            &&  !Doc()->GetRootDoc()->_fDisableReaderMode
            &&  pDispNode
            &&  pDispNode->IsScroller())
        {
            ExecReaderMode(pMessage, TRUE);
            hr = S_OK;
        }
        break;

    // BUGBUG (sujalp): The button downs are handled by the TSR!
    //
    case WM_LBUTTONDOWN:
         g_fAfterDoubleClick = FALSE;
         // fall through

    // BUGBUG (sujalp): The button downs are handled by the TSR!
    //
    case WM_RBUTTONDOWN:
        // Send to text engine
        //
        if(_fEatLeftDown && pMessage->message == WM_LBUTTONDOWN)
        {
            _fEatLeftDown = FALSE;
        }
        hr = S_OK;

        // In case we lost the capture inside the TxSendMessage
        //
        ElementOwner()->TakeCapture(TRUE);

        break;

    case WM_LBUTTONUP:
        // We come here only when we get a BUTTONUP but do not have capture
        // (e.g. double click overlapped with the preceding triple click !)
        //
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    {
        fCapture =  pDoc->GetCaptureObject() == (void *) ElementOwner()
                 && ::GetCapture() == pDoc->_pInPlace->_hwnd;
        if (fCapture)
        {
            hr = THR(ElementOwner()->BecomeUIActive());
            if (hr)
                goto Cleanup;

            ElementOwner()->TakeCapture(FALSE);
        }
        else
        {
            if (pMessage->message == WM_RBUTTONUP)
                hr = S_OK;
        }

        if (hr != S_FALSE && pMessage->message == WM_RBUTTONUP)
        {
            // Cause WM_CONTEXTMENU to be sent.
            //
            hr = THR(super::HandleMessage(pMessage, pElem, pNodeContext));
        }
        break;
    }

    case WM_MOUSEMOVE:
        fLbuttonDown = !!(GetKeyState(VK_LBUTTON) & 0x8000);

        if (fLbuttonDown && pDoc->_state >= OS_INPLACE)
        {
            // if we came in with a lbutton down (lbuttonDown = TRUE) and now
            // it is up, it might be that we lost the mouse up event due to a
            // DoDragDrop loop. In this case we have to UI activate ourselves
            //
            if (!(GetKeyState(VK_LBUTTON) & 0x8000))
            {
                hr = THR(ElementOwner()->BecomeUIActive());
                if (hr)
                    goto Cleanup;
            }
        }
        break;

#ifndef WIN16
    case WM_MOUSEWHEEL:
        if (ElementOwner()->Tag() == ETAG_BODY)
        {
            hr = THR(HandleMouseWheel(pMessage));
        }
        break;
#endif // ndef WIN16

    case WM_CONTEXTMENU:
        {
            int idMenu = CONTEXT_MENU_DEFAULT;

            if (   pDoc->GetSelectionType() == SELECTION_TYPE_Selection
                && !IsEditable(TRUE))
            {
                idMenu = CONTEXT_MENU_TEXTSELECT;
            }

            hr = THR(ElementOwner()->OnContextMenu(
                    (short) LOWORD(pMessage->lParam),
                    (short) HIWORD(pMessage->lParam),
                    idMenu));
        }
        break;

    case WM_MOUSEACTIVATE:
        pMessage->lresult = MA_ACTIVATE;
        if(!::IsChild((HWND) pMessage->wParam, GetFocus()))
            _fEatLeftDown = TRUE;
        break;

    case WM_SETCURSOR:
        // Are we over empty region?
        //
        hr = THR(HandleSetCursor(
                pMessage,
                pMessage->resultsHitTest.fWantArrow
                              && (!pElem ||
                                   pElem->GetFirstBranch()->GetNearestLayout() == this)));
        break;

    case WM_ERASEBKGND:
        // We should not receive these any more
        Assert(FALSE);
        hr = S_OK;
        break;

    case WM_SYSKEYDOWN:
        hr = THR(HandleSysKeyDown(pMessage));
        break;

    case WM_SYSCOLORCHANGE:
        Invalidate();
        hr = S_OK;
        break;

    case WM_KEYDOWN:
        Assert(hr == S_FALSE);
        switch (pMessage->wParam)
        {
        case VK_TAB:
            if (ElementOwner()->IsEditable(TRUE))
            {
                // Inform CRootSite whether this VK_TAB WM_KEYDOWN message is
                // for keyboard navigation in design mode.
                //
#ifdef OLDSELECTION
                if (!pDoc->_fInPre && pSel)
                {
                        //
                        // marka SEL-TO-DO
                        //
                     // pSel->InPre()
                    pDoc->_fInPre = (pSel->InPre()) ? (TRUE) : (FALSE);
                }
#endif
            }
            break;
        }
        break;

#if 0
    case WM_INPUTLANGCHANGEREQUEST:
        if (pSel)
        {
            pSel->CheckChangeFont (ElementOwner(), LOWORD(pMessage->lParam));
        }
        hr = S_FALSE;   // cause default window to allow kb switch.
        break;
#endif

    default:
        if (hr != S_FALSE)
            goto Cleanup;
        break;
    }

    if (    fIsScroller
        &&  (   hr == S_FALSE
            ||  hr == S_MSG_KEY_IGNORED
            ||  pMessage->message == WM_SETFOCUS
            ||  pMessage->message == WM_KILLFOCUS))
    {
        BOOL    fKeyIgnored = (hr == S_MSG_KEY_IGNORED);

        hr = HandleScrollbarMessage(pMessage, pElem);

        if (hr == S_FALSE)
        {
            hr = super::HandleMessage(pMessage, pElem, pNodeContext);
            if (    hr == S_FALSE
                &&  fKeyIgnored)
            {
                hr = S_OK;
            }
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


ExternTag(tagPaginate);




class CStackPageBreaks
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CStackPageBreaks))
    CStackPageBreaks();

    HRESULT Insert(long yPosTop, long yPosBottom, long xWidthSplit);
    long GetSplit();

    CStackPtrAry<long, 20> _aryYPos;
    CStackPtrAry<long, 20> _aryXWidthSplit;
};


CStackPageBreaks::CStackPageBreaks() : _aryYPos(Mt(CStackPageBreaks_aryYPos_pv)),
                                       _aryXWidthSplit(Mt(CStackPageBreaks_aryXWidthSplit_pv))
{
}

HRESULT
CStackPageBreaks::Insert(long yPosTop, long yPosBottom, long xWidthSplit)
{
    int     iStart = 0, iEnd = 0, cRanges = _aryYPos.Size() - 1, iCount, iSize;

    HRESULT hr = S_OK;

    Assert(yPosTop <= yPosBottom);
    Assert(xWidthSplit >= 0);


    // 0. Insert first range.
    if (!_aryYPos.Size())
    {
        hr = _aryYPos.Insert(0, yPosTop);
        if (hr)
            goto Cleanup;

        hr = _aryYPos.Insert(1, yPosBottom);
        if (hr)
            goto Cleanup;

        hr = _aryXWidthSplit.Insert(0, xWidthSplit);

        // Done.
        goto Cleanup;
    }

    // 1. Find beginning of range.
    while (iStart < cRanges && yPosTop > _aryYPos[iStart])
        iStart++;

    // If necessary, beginning creates new range.
    if (yPosTop < _aryYPos[iStart])
    {
        hr = _aryYPos.Insert(iStart, yPosTop);
        if (hr)
            goto Cleanup;

        hr = _aryXWidthSplit.Insert(iStart, _aryXWidthSplit[iStart-1]);
        if (hr)
            goto Cleanup;

        cRanges++;
    }

    // 2. Find end of range.
    iEnd = iStart;

    while (iEnd < cRanges && yPosBottom > _aryYPos[iEnd+1])
        iEnd++;

    // If necessary, end creates new range.
    if (iEnd < cRanges && yPosBottom < _aryYPos[iEnd+1])
    {
        hr = _aryYPos.Insert(iEnd+1, yPosBottom);
        if (hr)
            goto Cleanup;

        hr = _aryXWidthSplit.Insert(iEnd+1, _aryXWidthSplit[iEnd]);
        if (hr)
            goto Cleanup;

        cRanges++;
    }

    // 3. Increment the ranges covered.
    iSize = _aryXWidthSplit.Size(); // protecting from overflowing the split array
    for (iCount = iStart ; iCount <= iEnd && iCount < iSize ; iCount++)
    {
        _aryXWidthSplit[iCount] += xWidthSplit;
    }

#if DBG == 1
    if (IsTagEnabled(tagPaginate))
    {
        TraceTag((tagPaginate, "Dumping page break ranges:"));
        for (int iCount = 0; iCount < _aryXWidthSplit.Size(); iCount++)
        {
            TraceTag((tagPaginate, "%d - %d : %d", iCount, _aryYPos[iCount], _aryXWidthSplit[iCount]));
        }
    }
#endif

Cleanup:

    RRETURN(hr);
}


long
CStackPageBreaks::GetSplit()
{
    if (!_aryYPos.Size())
    {
        Assert(!"Empty array");
        return 0;
    }

    int iPosMin = _aryXWidthSplit.Size()-1, iCount;
    long xWidthSplitMin = _aryXWidthSplit[iPosMin];

    for (iCount = iPosMin-1 ; iCount > 0 ; iCount--)
    {
        if (_aryXWidthSplit[iCount] < xWidthSplitMin)
        {
            xWidthSplitMin = _aryXWidthSplit[iCount];
            iPosMin = iCount;
        }
    }

    return _aryYPos[iPosMin+1]-1;
}


//+---------------------------------------------------------------------------
//
//  Member:     AppendNewPage
//
//  Synopsis:   Adds a new page to the print doc.  Called by Paginate.  This
//              is a helper function used to reuse code and clean up paginate.
//
//  Arguments:  paryPP               Page array to add to
//              pPP                  New page
//              pPPHeaderFooter      Header footer buffer page
//              yHeader              Height of header to be repeated
//              yFooter              Height of footer
//              yFullPageHeight      Height of a full (new) page
//              pyTotalPrinted       Height of content paginated sofar
//              pySpaceLeftOnPage    Height of y-space left on page
//              pyPageHeight         Height of page
//              pfRejectFirstFooter  Should the first repeated footer be rejected
//
//----------------------------------------------------------------------------

HRESULT AppendNewPage(CDataAry<CPrintPage> * paryPP,
                      CPrintPage * pPP,
                      CPrintPage * pPPHeaderFooter,
                      long yHeader,
                      long yFooter,
                      long yFullPageHeight,
                      long * pyTotalPrinted,
                      long * pySpaceLeftOnPage,
                      long * pyPageHeight,
                      BOOL * pfRejectFirstFooter)
{
    HRESULT hr = S_OK;

    Assert(pfRejectFirstFooter && paryPP && pPP &&
           pPPHeaderFooter && pyTotalPrinted && pySpaceLeftOnPage && pyPageHeight);

    TraceTag((tagPaginate, "Appending block of size %d", pPP->yPageHeight));

    if (yFooter)
    {
        pPP->fReprintTableFooter = !(*pfRejectFirstFooter);
        pPP->pTableFooter = pPPHeaderFooter->pTableFooter;
        pPP->rcTableFooter = pPPHeaderFooter->rcTableFooter;
        *pfRejectFirstFooter = FALSE;
    }
    
    hr = THR(paryPP->AppendIndirect(pPP));
    if (hr)
        goto Cleanup;

    *pyTotalPrinted += pPP->yPageHeight;
    pPP->yPageHeight = pPP->xPageWidth = 0;
    *pySpaceLeftOnPage = *pyPageHeight = yFullPageHeight - yHeader - yFooter;
    pPP->fReprintTableFooter = FALSE;

    if (yHeader)
    {
        pPP->fReprintTableHeader = TRUE;
        pPP->pTableHeader = pPPHeaderFooter->pTableHeader;
        pPP->rcTableHeader = pPPHeaderFooter->rcTableHeader;
    }

Cleanup:

    RRETURN(hr);
}


BOOL
RepeatHeaderFooter()
{
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::Paginate
//
//  Synopsis:   Paginates the document
//
//  Arguments:  pdci  -- incoming Docinfo struct
//
//  Returns :   S_OK if all went well, if breakpoints are found, some error if the recursively
//              called routine fails
//
//  Notes:      This algorithm does not do any kind of sophisticated breaking
//              for aligned lines -- these are mostly just split wherever.
//              Tables are the only type of lines that are split intelligently.
//
//              If the space left on a page is <= 5% of the printable area, we
//              pagebreak there and go on to the next page.
//
//----------------------------------------------------------------------------

#define WASTABLE_SPACE_PER_PAGE     15

HRESULT
CFlowLayout::Paginate(LONG * plViewWidth, LONG yInitialPageHeight, BOOL fFillLastPage)
{
    Assert(GetDisplay());
    HRESULT     hr = S_OK;
    LONG        ili, iliCount = GetDisplay()->LineCount();
    CPrintPage  PP;
    LONG        yPageHeight,       // The height of the current line (might be different from yFullPageHeight for first page when printing OE headers)
                yFullPageHeight;   // The full height of a page (every page beside potentially the first one)
    LONG        yTotalPrinted = 0;
    LONG        yToTopOfLine = 0;
    LONG        yHeader = 0;
    LONG        yFooter = 0;
    CPrintPage  PPHeaderFooter;
    CPrintDoc * pPrintDoc = DYNCAST(CPrintDoc, Doc());
    LONG        ySpaceLeftOnPage;
    CDataAry<CPrintPage> * paryPP = pPrintDoc->_paryPrintPage;
    long        cp = GetContentFirstCp();
    RECT        rcSite;

    // Sanity check.
    Assert(Doc()->GetRootDoc() == Doc());
    Assert(Doc()->IsPrintDoc());
    Assert(paryPP);

    // Initialize plViewWidth
    *plViewWidth = 0;

    if (ElementOwner()->Tag() != ETAG_BODY)
    {
        Assert(!"Paginate called on a non-body element");
        return S_OK;
    }

    PP.yPageHeight = PP.xPageWidth = 0;
    PP.fReprintTableHeader = FALSE;
    PP.fReprintTableFooter = FALSE;
    yFullPageHeight = pPrintDoc->_dci._sizeDst.cy;
    yPageHeight = yInitialPageHeight ? yInitialPageHeight : yFullPageHeight;
    ySpaceLeftOnPage = yPageHeight;

    if (Doc() && Doc()->GetPrimaryElementClient())
    {
        Doc()->GetPrimaryElementClient()->GetLayout()->GetRect(&rcSite);
        yToTopOfLine = rcSite.top;
    }

    for (ili = 0; ili < iliCount; ili++)
    {
        CLine * pli = GetDisplay()->Elem(ili);
        LONG yLineHeight = pli->_yHeight;
        BOOL fRejectFirstFooter = FALSE;

        TraceTag((tagPaginate, "--- LINE %d --- ForceNewLine is %s", ili, pli->_fForceNewLine ? "ON" : "off"));

        yHeader = 0;
        yFooter = 0;
        PPHeaderFooter.pTableHeader = NULL;
        PPHeaderFooter.pTableFooter = NULL;

        if (pli->_fSingleSite && pli->_fHasNestedRunOwner)
        {
            // Is there a nested pagebreak?
            CLayout * pLayoutToSplit = GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL)->GetLayout();
            if (pLayoutToSplit && pLayoutToSplit->Tag() == ETAG_TABLE)
            {
                CTableLayout * pTableLayout = DYNCAST(CTableLayout, pLayoutToSplit);

                // TODO: read style that requests reprinting of headers and footers.
                // For now, we never reprint them.  For now use a debug tracetag.
                if (RepeatHeaderFooter()
                   WHEN_DBG( || IsTagEnabled(tagRepeatHeaderFooter)) )
                {
                    pTableLayout->GetHeaderFooterRects(&(PPHeaderFooter.rcTableHeader), &(PPHeaderFooter.rcTableFooter));
                    yHeader = PPHeaderFooter.rcTableHeader.bottom - PPHeaderFooter.rcTableHeader.top;
                    yFooter = PPHeaderFooter.rcTableFooter.bottom - PPHeaderFooter.rcTableFooter.top;
                    PPHeaderFooter.pTableHeader = pTableLayout->GetHeader();
                    PPHeaderFooter.pTableFooter = pTableLayout->GetFooter();
   
                    fRejectFirstFooter = !PPHeaderFooter.pTableFooter || ySpaceLeftOnPage < 2*(yHeader + yFooter);
   
                    // Check if there is enough space for header and footer.
                    if (fRejectFirstFooter)
                    {
                        if (!PPHeaderFooter.pTableFooter || yFullPageHeight < 2*(yHeader + yFooter))
                            yFooter = 0;
                    }
                    else
                    {
                        ySpaceLeftOnPage -= yFooter;
                        yPageHeight -= yFooter;
                    }
    
                    if (!PPHeaderFooter.pTableHeader || yFullPageHeight < 2*yHeader)
                    {
                        yHeader = 0;
                    }
                }
            }
        }

        //
        // First handle explicit page breaks (if we have any).
        //

        if (pPrintDoc->_fHasPageBreaks)
        {
            LONG yLineAlreadyPrinted = 0; // part of the line already printed
            LONG yBreakLayout = 0;
            BOOL fPageBreakBeforeOrInside = pli->_fPageBreakBefore, fCheckForAnotherPageBreak = FALSE, fPageBreakAfterAtTopLevel = FALSE;

            do
            {
                if (!fPageBreakBeforeOrInside && pli->_fSingleSite && pli->_fHasNestedRunOwner)
                {
                    // Is there a nested pagebreak?
                    CLayout * pLayoutToSplit = GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL)->GetLayout();

                    // If we already had pagebreak on this line, add 1 to avoid retrieving the same break again.
                    if (   pLayoutToSplit
                        && THR(pLayoutToSplit->ContainsPageBreak(
                                yLineAlreadyPrinted + (fCheckForAnotherPageBreak ? 1 : 0),
                                ySpaceLeftOnPage,
                                &yBreakLayout,
                                &fPageBreakAfterAtTopLevel)) == S_OK )
                    {
                        fPageBreakBeforeOrInside = TRUE;
                        yBreakLayout -= yLineAlreadyPrinted;  // make relative to part of line already printed
                    }
                }

                fCheckForAnotherPageBreak = FALSE;

                if (fPageBreakBeforeOrInside)
                {
                    TraceTag((tagPaginate,  "\t%d page break BEFORE found, breaking", ili));

                    PP.yPageHeight += yBreakLayout;

                    hr = THR(AppendNewPage(paryPP,
                                           &PP,
                                           &PPHeaderFooter,
                                           yHeader,
                                           yFooter,
                                           yFullPageHeight,
                                           &yTotalPrinted,
                                           &ySpaceLeftOnPage,
                                           &yPageHeight,
                                           &fRejectFirstFooter));
                    if (hr)
                        goto Cleanup;
                    
                    yLineHeight -= yBreakLayout;
                    yLineAlreadyPrinted += yBreakLayout;
                    fPageBreakBeforeOrInside = FALSE; // Pagebreak processed.
                    fCheckForAnotherPageBreak = !fPageBreakAfterAtTopLevel; // Look for another one on the same line.
                }
            }
            while (fCheckForAnotherPageBreak); // If we had a pagebreak, see if there is another one.

            if (fPageBreakAfterAtTopLevel)
            {
                // If we encountered a page break after on this line, we are done with it.

                // Assert that we are indeed done with this line.
                Assert(yLineAlreadyPrinted == pli->_yHeight);
                Assert(!yLineHeight);

                // Add back to space for footer not needed.
                yPageHeight += yFooter;

                continue;
            }
        }

        if (pli->_fForceNewLine)
        {

            // We collect the longest line on the page,
            // and the longest line overall
            if (pli->_xLineWidth > PP.xPageWidth)
            {
                // Longest line on page is the page width
                PP.xPageWidth = pli->_xLineWidth;

                // Longest line across pages is the view's width
                if (pli->_xLineWidth > *plViewWidth)
                {
                   *plViewWidth = pli->_xLineWidth;
                }
            }

            if (PP.yPageHeight + yLineHeight <= yPageHeight)
            {
                // line fits on this page, just add it to the page and continue

                PP.yPageHeight += yLineHeight;
                ySpaceLeftOnPage -= yLineHeight;
                TraceTag((tagPaginate, "\t(line %d) block fits, yli=%d yPage=%d", ili, yLineHeight, PP.yPageHeight));
            }
            else
            {
                BOOL    fTrySplit = FALSE;
                int     yLineAlreadyPrinted = 0;
                CLayout * pLayoutToSplit = NULL;

                TraceTag((tagPaginate, "\t%d block does not fit", ili));

                // first let's see if this is a line that doesn't want to be split
                // i.e. it contains an image/control/applet

                if (pli->TryToKeepTogether() && yLineHeight <= yFullPageHeight)
                {
                    // break right here

                   hr = THR(AppendNewPage(paryPP,
                                          &PP,
                                          &PPHeaderFooter,
                                          yHeader,
                                          yFooter,
                                          yFullPageHeight,
                                          &yTotalPrinted,
                                          &ySpaceLeftOnPage,
                                          &yPageHeight,
                                          &fRejectFirstFooter));

                    // now add in the image
                    Assert(yLineHeight <= yFullPageHeight);
                    PP.yPageHeight = yLineHeight;
                    ySpaceLeftOnPage -= yLineHeight;
                }
                else if (pli->_fSingleSite && pli->_fHasNestedRunOwner)
                {
                    // we can split this.  firstly, this might be a table

                    // skip null runs, position rtp at start of run, and get the site
                    pLayoutToSplit = GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL)->GetLayout();

                    if (pLayoutToSplit && pLayoutToSplit->Tag() == ETAG_TABLE)
                    {
                        Assert(DYNCAST(CLayout, this) != pLayoutToSplit && "must be true for a table");
                        fTrySplit = TRUE;
                    }
                    else
                    {
                        pLayoutToSplit = NULL;
                    }
                }

                ySpaceLeftOnPage = yPageHeight - PP.yPageHeight;

                while (yLineHeight >= ySpaceLeftOnPage)
                {
                    long ySubBlock, yPageBreak;

                    Assert(!!fTrySplit == !!pLayoutToSplit && "these two must be zero or non-zero together");

                    if (pli->IsClear())
                    {
                        // the line is empty, so nothing to split
                        ySubBlock = ySpaceLeftOnPage;
                    }
                    else if (ySpaceLeftOnPage <= (yFullPageHeight * WASTABLE_SPACE_PER_PAGE / 100))
                    {
                        // so, only a small fraction of space will be wasted.
                        // why split a line?  let's go to the next page.

                        ySubBlock = 0;
                    }
                    else if (   pPrintDoc->_fHasPageBreaks
                             && pli->_fSingleSite && pli->_fHasNestedRunOwner
                             && GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL)->GetLayout()
                             && THR(GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL)->GetLayout()->ContainsPageBreak(yLineAlreadyPrinted + 1, ySpaceLeftOnPage, &yPageBreak)) == S_OK )
                    {
                        ySubBlock = yPageBreak - (yToTopOfLine + yLineAlreadyPrinted);
                    }
                    else
                    {
                        CStackPageBreaks    aryValues;

                        // Start out with one completely breakable range.
                        aryValues.Insert(yToTopOfLine + yLineAlreadyPrinted + ySpaceLeftOnPage - (WASTABLE_SPACE_PER_PAGE * yFullPageHeight / 100),
                                         yToTopOfLine + yLineAlreadyPrinted + ySpaceLeftOnPage,
                                         0);

                        if (fTrySplit && pLayoutToSplit &&
                            (pLayoutToSplit->PageBreak(
                                            yToTopOfLine + yLineAlreadyPrinted + ySpaceLeftOnPage - (WASTABLE_SPACE_PER_PAGE * yFullPageHeight / 100),
                                            yToTopOfLine + yLineAlreadyPrinted + ySpaceLeftOnPage,
                                            &aryValues) == S_OK))
                        {
                            ySubBlock = aryValues.GetSplit() - (yToTopOfLine + yLineAlreadyPrinted);
                        }
                        else
                        {
                            // splitting is potentially very expensive.. we won't try
                            // after the first failure.

                            if (fTrySplit)
                            {
                                fTrySplit = FALSE;
                                pLayoutToSplit = NULL;
                            }
                            ySubBlock = ySpaceLeftOnPage;
                        }
                    }

                    PP.yPageHeight += ySubBlock;

                    TraceTag((tagPaginate, "       %d inside split loop, subblock is %d, spaceleft is %d", ili, ySubBlock, ySpaceLeftOnPage));

                    hr = THR(AppendNewPage(paryPP,
                                           &PP,
                                           &PPHeaderFooter,
                                           yHeader,
                                           yFooter,
                                           yFullPageHeight,
                                           &yTotalPrinted,
                                           &ySpaceLeftOnPage,
                                           &yPageHeight,
                                           &fRejectFirstFooter));
 
                    yLineAlreadyPrinted += ySubBlock;
                    yLineHeight -= ySubBlock;
                }

                Assert(yLineHeight < yPageHeight);
                PP.yPageHeight = yLineHeight;
            }

            // might want to Assert(pli->_fForceNewLine && "otherwise yTopOfLine will be incorrect");
            yToTopOfLine += pli->_yHeight;  // NOTE: *NOT* yLineHeight, that gets changed above
        }
#if DBG == 1
        else
        {
            // if we cared about splitting aligned lines, we'd probably add this
            // to a stack of (left- or right-) aligned lines, so that at the end
            // of the page we could ask them about their choice of split points
            // and pick the best one
            ;
        }
#endif // DBG == 1

        // track the number of characters for the rich text pointer
        cp += pli->_cch;

        // Add back to space for footer not needed.
        yPageHeight += yFooter;
    }

    // while ((yTotalPrinted + PP.yPageHeight) < (GetMaxYScroll() + GetDisplay()->_yHeightView))
    // BUGBUG: This while statement is soft of wrong for the OE header case.  If OE
    // decides to have aligned sites in their page the page break between OE headers
    // and the main doc might truncate those aligned sites.
    while ( fFillLastPage
        &&  ((yTotalPrinted + PP.yPageHeight) < (GetContentHeight() )))
    {
        // some things remain to be printed, either the final (partial)
        // page from the split block above, or some aligned lines that
        // projected beyond the last line that had fForceNewLine turned on.

        ySpaceLeftOnPage = yPageHeight - PP.yPageHeight;

        PP.yPageHeight += ySpaceLeftOnPage;

        hr = THR(paryPP->AppendIndirect(&PP));
        TraceTag((tagPaginate, "Appending block of size %d", PP.yPageHeight));
        if (hr)
            goto Cleanup;

        yTotalPrinted += PP.yPageHeight;
        PP.yPageHeight = PP.xPageWidth = 0;
        yPageHeight = yFullPageHeight;
    }

    // If there are any lines on the current page, add it to the PP array.

    if (PP.yPageHeight)
    {
        hr = THR(paryPP->AppendIndirect(&PP));
        TraceTag((tagPaginate, "Appending block of size %d", PP.yPageHeight));
    }

    // and finally, set the height of the last page to the full height, so that
    // any background image gets rendered for the entire page.

    if (fFillLastPage && paryPP->Size())
    {
        (paryPP->Item(paryPP->Size() - 1)).yPageHeight = yPageHeight;
    }

    // Prevent further recalcs.

    GetDisplay()->_fNoUpdateView = TRUE;

 Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::PageBreak
//
//  Synopsis:   Given a start and end point, returns an array of suitable page-
//              breaking points between these values, favouring the end point
//              (which is assumed to be the end of the page)
//
//  Arguments:  yStart    -- start looking for suitable breakpoints PAST this point
//              yIdeal    -- this is the ideal breakpt (also, don't look past here)
//              aryValues -- stack to store possible breakpoints in (0=bad, n-1=good)
//
//  Returns :   S_OK if breakpoints are found, some error if the recursively
//              called routine fails
//
//----------------------------------------------------------------------------

HRESULT
CFlowLayout::PageBreak(LONG yStart, LONG yIdeal, CStackPageBreaks * paryValues)
{
    HRESULT     hr = S_OK;
    CDispNode * pDispNode = GetElementDispNode();
    LONG        ili, iliCount = GetDisplay()->LineCount();
    long        cp = GetDisplay()->GetFirstCp();
    LONG        yHeight;
    RECT        rcCurSite;

    // Provide small disincentive for cutting across flowlayout in case there is cutting space
    // elsewhere.  This prevents that we cut table rows when there is a gap available.

    if (pDispNode)
    {
        CRect rcBounds;
        CSize sizeOffset;

        pDispNode->GetBounds(&rcBounds);
        pDispNode->GetTransformOffset(&sizeOffset, CDispNode::COORDSYS_PARENT, CDispNode::COORDSYS_GLOBAL);
        rcBounds.OffsetRect(sizeOffset.cx, sizeOffset.cy);
        paryValues->Insert(rcBounds.top, rcBounds.bottom-1, 3);
    }

    GetClientRect((CRect *)&rcCurSite, COORDSYS_GLOBAL);

    yHeight = rcCurSite.top;

    Assert(yStart < yIdeal);

    for (ili = 0; ili < iliCount; ili++)
    {
        CLine * pli = GetDisplay()->Elem(ili);
        LONG yLineHeight = pli->_yHeight;
        BOOL fNotInteresting;

        // track the number of characters for the rich text pointer
        //cp += pli->_cch;

        if (!pli->_fForceNewLine)
        {
            continue;
        }

        fNotInteresting = yHeight < yStart;

        yHeight += yLineHeight;

        if (fNotInteresting)
            continue;

        {
            CLayout * pLayoutToSplit = NULL;

            // must try and recurse into this line
            if (pli->_fSingleSite && pli->_fHasNestedRunOwner)
            {
                // might be a table

                // skip null runs, position rtp at start of run, and get the site
                pLayoutToSplit = GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL)->GetNearestLayout();

                if (pLayoutToSplit && pLayoutToSplit->Tag() == ETAG_TABLE)
                {
                    hr = pLayoutToSplit->PageBreak(yStart, yIdeal, paryValues);
                    if (FAILED(hr))
                        goto Cleanup;
                }
            }
            else if (pli->IsClear())
            {
                hr = paryValues->Insert(yHeight-yLineHeight+1, yHeight-1, pli->_xLineWidth);
                if (FAILED(hr))
                    goto Cleanup;
                break;
            }
        }

        if (yHeight-yLineHeight > yIdeal)
            break;

        Assert(yHeight-yLineHeight <= yIdeal && "loop was not properly terminated!");
        hr = paryValues->Insert(yHeight-yLineHeight+1, yHeight-1, pli->_xLineWidth);
        if (hr)
            goto Cleanup;

        // track the number of characters for formatting node
        cp += pli->_cch;
    }

Cleanup:

    RRETURN1(hr, S_FALSE);
}


//+----------------------------------------------------------------------------
//
//  Member:     CFlowLayout::ContainsPageBreak
//
//  Synopsis:   Checks whether flowlayout has page-break-before or nested layout
//              element sets page-break-before or -after.
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
CFlowLayout::ContainsPageBreak(long yTop, long yBottom, long * pyBreak, BOOL * pfPageBreakAfterAtTopLevel)
{
    CDispNode * pDispNode;
    BOOL        fPageBreakAfter = HasPageBreakAfter();
    long        ili, iliCount, cp;
    long        yHeight = 0;
    RECT        rcBorders;
    SIZE        sizeDispNode;
    HRESULT     hr = S_FALSE;

    Assert(pyBreak);

    if (HasPageBreakBefore() && yTop <= 0)
        goto FoundBreak;

    iliCount = GetDisplay()->LineCount();
    cp = GetContentFirstCp();

    // Init yHeight starting at top border.
    pDispNode = GetElementDispNode();
    if (pDispNode)
    {
        pDispNode->GetBorderWidths(&rcBorders);
        yHeight = rcBorders.top;

        if (pDispNode->HasInset())
        {
            const CSize & sizeInset = pDispNode->GetInset();
            yHeight += sizeInset.cy;
        }
    }

    for (ili = 0 ; ili < iliCount && yHeight <= yBottom ; ili++)
    {
        CLine * pli = GetDisplay()->Elem(ili);
        long yLineHeight = pli->_yHeight;

        if (pli->_fPageBreakBefore && yHeight >= yTop && yHeight <= yBottom)
            goto FoundBreak;

        // BUGBUG: We are only covering one layout per line scenarios here.  If we find
        // that INSIDE page breaks for multiple layouts on the same line is important,
        // we can structure this for-loop the same way as CTableLayout deals with pagebreaks
        // on multiple TDs.
        if (yHeight + yLineHeight >= yTop && pli->_fSingleSite && pli->_fHasNestedRunOwner)
        {
            // Is there a nested pagebreak?
            CLayout * pLayoutToSplit = GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL)->GetLayout();
            long      yNestedBreak;

            if (   pLayoutToSplit
                && THR(pLayoutToSplit->ContainsPageBreak(
                        (yTop > yHeight) ? (yTop - yHeight) : 0,
                        yBottom - yHeight,
                        &yNestedBreak)) == S_OK
                && yNestedBreak + yHeight >= yTop
                && yNestedBreak + yHeight <= yBottom )
            {
                yHeight += yNestedBreak;
                goto FoundBreak;
            }
        }

        yHeight += yLineHeight;

        if (yHeight >= yTop && yHeight <= yBottom)
        {
            if (pli->_fSingleSite && pli->_fHasNestedRunOwner)
            {
                CLayout * pLayoutToSplit = GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL)->GetLayout();

                if (pLayoutToSplit && pLayoutToSplit->HasPageBreakAfter())
                    goto FoundBreak;
            }
        }

        cp += pli->_cch;
    }

    if (fPageBreakAfter)
    {
        //
        // Find out if the bottom of this layout is above yBottom.
        //

        pDispNode = GetElementDispNode();
        if (!pDispNode)
            goto Cleanup;

        pDispNode->GetSize(&sizeDispNode);
        if (yTop <= sizeDispNode.cy && sizeDispNode.cy <= yBottom)
        {
            *pyBreak = sizeDispNode.cy;
            hr = S_OK;

            if (pfPageBreakAfterAtTopLevel)
                *pfPageBreakAfterAtTopLevel = TRUE;

            goto Cleanup;
        }
    }

    // No Break found.

Cleanup:

    RRETURN1(hr, S_FALSE);

FoundBreak:

    *pyBreak = yHeight;
    hr = S_OK;
    goto Cleanup;
}


#if DBG == 1
BOOL
CFlowLayout::IsInPlace()
{
    // lie if in a printdoc.
    return Doc()->IsPrintDoc() || Doc()->_state >= OS_INPLACE;
}
#endif

//+----------------------------------------------------------------------------
//
// member: ReaderModeScroll
//
//-----------------------------------------------------------------------------
void
CFlowLayout::ReaderModeScroll(int dx, int dy)
{
    CRect   rc;
    long    cxWidth;
    long    cyHeight;

    Assert(GetElementDispNode(NULL, FALSE));
    Assert(GetElementDispNode(NULL, FALSE)->IsScroller());

    GetElementDispNode(NULL, FALSE)->GetClientRect(&rc, CDispNode::CLIENTRECT_CONTENT);

    cxWidth  = rc.Width();
    cyHeight = rc.Height();

    //
    //  Scroll slowly if moving in a single dimension
    //

    if (    !dx
        ||  !dy)
    {
        if (abs(dx) == 1 || abs(dy) == 1)
        {
            Sleep(100);
        }
        else if (abs(dx) == 2 || abs(dy) == 2)
        {
            Sleep(50);
        }
    }

    //
    //  Calculate the scroll delta
    //  (Use a larger amount if the incoming delta is large)
    //

    if (abs(dx) > 10)
    {
        dx = (dx > 0
                ? cxWidth / 2
                : -1 * (cxWidth / 2));
    }
    else if (abs(dx) > 8)
    {
        dx = (dx > 0
                ? cxWidth / 4
                : -1 * (cxWidth / 4));
    }
    else
    {
        dx = dx * 2;
    }

    if (abs(dy) > 10)
    {
        dy = (dy > 0
                ? cyHeight / 2
                : -1 * (cyHeight / 2));
    }
    else if (abs(dy) > 8)
    {
        dy = (dy > 0
                ? cyHeight / 4
                : -1 * (cyHeight / 4));
    }
    else
    {
        dy = dy * 2;
    }

    //
    //  Scroll the content
    //

    ScrollBy(CSize(dx, dy));
}

//+----------------------------------------------------------------------------
//
//  Member:     XCaretToRelative
//
//  Synopsis:   Converts a xCaret value from global client window coordinates
//              to site relative coordinates.
//
//  Arguments:  [xCaret]: The xCaret position in global client window coordinates
//
//-----------------------------------------------------------------------------
LONG CFlowLayout::XCaretToRelative(LONG xCaret)
{
// BUGBUG: Do something (brendand)
    return xCaret;
}

//+----------------------------------------------------------------------------
//
//  Member:     XCaretToAbsolute
//
//  Synopsis:   Converts a xCaret value from site relative coordinates to
//              global client window coordinates
//
//  Arguments:  [xCaretReally]: The xCaret position in site relative coords
//
//-----------------------------------------------------------------------------
LONG CFlowLayout::XCaretToAbsolute(LONG xCaretReally)
{
// BUGBUG: Do something (brendand)
    return xCaretReally;
}

//+----------------------------------------------------------------------------
//
// _ReaderMode_OriginWndProc
// _ReaderMode_Scroll
// _ReaderMode_TranslateDispatch
//
// Reader Mode Auto-Scroll helper routines, paste from classic MSHTML code
// These are callback functions for DoReaderMode.
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
_ReaderMode_OriginWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc, hdcCompat;
    HBITMAP     hbmp = NULL;
    PAINTSTRUCT ps;

    switch (msg)
    {
    case WM_PAINT:
    case WM_ERASEBKGND:
        hdc = BeginPaint(hwnd, &ps);

        if ((hbmp = (HBITMAP)GetWindowLong(hwnd, 0)) != NULL &&
                (hdcCompat = CreateCompatibleDC(hdc)) != NULL)
        {
            BITMAP   bmp;
            HBITMAP  hbmpOld;
            HBRUSH   hbrushOld;
            HPEN     hpenOld, hpen;
            COLORREF crColor;

            GetObject(hbmp, sizeof(BITMAP), &bmp);
            hbmpOld   = (HBITMAP) SelectObject(hdcCompat, hbmp);
            hbrushOld = (HBRUSH) SelectObject(hdc, GetStockObject(NULL_BRUSH));
            crColor   = RGB(0, 0, 0);
            hpen      = CreatePen(PS_SOLID, 2, crColor);
            hpenOld   = (HPEN) SelectObject(hdc, hpen);

            BitBlt(hdc, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcCompat, 0, 0, SRCCOPY);
            Ellipse(hdc, 0, 0, bmp.bmWidth + 1, bmp.bmHeight + 1);

            SelectObject(hdcCompat, hbmpOld);
            SelectObject(hdc, hbrushOld);
            SelectObject(hdc, hpenOld);
            DeleteObject(hpen);
            DeleteObject(hdcCompat);
        }
        EndPaint(hwnd, &ps);
        break;

    default:
        return (DefWindowProc(hwnd, msg, wParam, lParam));
        break;
    }


    return 0;
}

#ifndef WIN16
BOOL CALLBACK
_ReaderMode_Scroll(PREADERMODEINFO prmi, int dx, int dy)
{
    CFlowLayout * pLayout = (CFlowLayout *) prmi->lParam;

    pLayout->ReaderModeScroll(dx, dy);
    return TRUE;
}

BOOL CALLBACK
_ReaderMode_TranslateDispatch(LPMSG lpmsg)
{
    BOOL   fResult = FALSE;
    LPRECT lprc;

    if (lpmsg->message == WM_MBUTTONUP)
    {
        // If the button up click comes within the "neutral zone" rectangle
        // around the point where the button went down then we want
        // continue reader mode so swallow the message by returning
        // TRUE.  Otherwise, let the message go through so that we cancel
        // out of panning mode.
        //
        if ((lprc = (LPRECT)GetProp(lpmsg->hwnd, TEXT("ReaderMode"))) != NULL)
        {
            POINT ptMouse = {LOWORD(lpmsg->lParam), HIWORD(lpmsg->lParam)};
            if (PtInRect(lprc, ptMouse))
            {
                fResult = TRUE;
            }
        }
    }
    return fResult;
}
#endif //ndef WIN16

//+----------------------------------------------------------------------------
//
// member: ExecReaderMode
//
// Execure ReaderMode auto scroll. Use DoReaderMode in COMCTL32.DLL
//
//-----------------------------------------------------------------------------
void
CFlowLayout::ExecReaderMode(CMessage * pMessage, BOOL fByMouse)
{
#ifndef WIN16
    POINT       pt;
    RECT        rc;
    HWND        hwndInPlace = NULL;
    HBITMAP     hbmp = NULL;
    BITMAP      bmp;
    HINSTANCE   hinst;
    CDoc     *  pDoc  = Doc();
    BOOL        fOptSmoothScroll = pDoc->_pOptionSettings->fSmoothScrolling;
    CDispNode * pDispNode        = GetElementDispNode(NULL, FALSE);

    Assert(pDispNode);
    Assert(pDispNode->IsScroller());

    CSize           size;
    const CSize &   sizeContent = DYNCAST(CDispScroller, pDispNode)->GetContentSize();
    BOOL            fEnableVScroll;
    BOOL            fEnableHScroll;

    pDispNode->GetSize(&size);

    fEnableVScroll = (size.cy < sizeContent.cy);
    fEnableHScroll = (size.cx < sizeContent.cx);

    READERMODEINFO rmi =
    {
        sizeof(rmi),
        NULL,
        0,
        &rc,
        _ReaderMode_Scroll,
        _ReaderMode_TranslateDispatch,
        (LPARAM) this
    };

    // force smooth scrolling
    //
    pDoc->_pOptionSettings->fSmoothScrolling = TRUE;

    if (!pDoc->InPlace() || !pDoc->InPlace()->_hwnd)
    {
        // not InPlace Activated yet, unable to do auto-scroll reader mode.
        //
        goto Cleanup;
    }

    rmi.hwnd = hwndInPlace = pDoc->InPlace()->_hwnd;

    hinst = (HINSTANCE) GetModuleHandleA("comctl32.dll");
    Assert(hinst && "GetModuleHandleA(COMCTL32) returns NULL");
    if (hinst)
    {
        if (fEnableVScroll && fEnableHScroll)
        {
            hbmp = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_2DSCROLL));
        }
        else if (fEnableVScroll || fEnableHScroll)
        {
            rmi.fFlags |= (fEnableVScroll)
                            ? (RMF_VERTICALONLY) : (RMF_HORIZONTALONLY);
            hbmp = LoadBitmap(
                    hinst,
                    MAKEINTRESOURCE( (fEnableVScroll)
                                     ? (IDB_VSCROLL) : (IDB_HSCROLL)));
        }
    }
    if (!hbmp)
    {
        // 1) no scroll bars are enabled, no need for auto-scroll reader mode.
        // 2) LoadBitmap fails, unable to do auto-scroll reader mode.
        //
        goto Cleanup;
    }

    if (fByMouse)
    {
        pt.x = LOWORD(pMessage->lParam);
        pt.y = HIWORD(pMessage->lParam);
        SetRect(&rc, pt.x, pt.y, pt.x, pt.y);
    }
    else
    {
        GetWindowRect(hwndInPlace, &rc);
        MapWindowPoints(NULL, hwndInPlace, (LPPOINT) &rc, 2);
        SetRect(&rc, rc.left + rc.right / 2, rc.top  + rc.bottom / 2,
                     rc.left + rc.right / 2, rc.top  + rc.bottom / 2);
        rmi.fFlags |= RMF_ZEROCURSOR;
    }

    // Make the "neutral zone" be the size of the origin bitmap window.
    //
    GetObject(hbmp, sizeof(BITMAP), &bmp);
    InflateRect(&rc, bmp.bmWidth / 2, bmp.bmHeight / 2);

    SetProp(hwndInPlace, TEXT("ReaderMode"), (HANDLE)&rc);

    {
#define ORIGIN_CLASS TEXT("MSHTML40_Origin_Class")

        HWND     hwndT, hwndOrigin;
        WNDCLASS wc;
        HRGN     hrgn;

        hwndT = GetParent(hwndInPlace);

        if (!(::GetClassInfo(GetResourceHInst(), ORIGIN_CLASS, &wc)))
        {
            wc.style         = CS_SAVEBITS;
            wc.lpfnWndProc   = _ReaderMode_OriginWndProc;
            wc.cbClsExtra    = 0;
            wc.cbWndExtra    = 4;
            wc.hInstance     = GetResourceHInst(),
            wc.hIcon         = NULL;
            wc.hCursor       = NULL;
            wc.lpszMenuName  = NULL;
            wc.hbrBackground = NULL;
            wc.lpszClassName = ORIGIN_CLASS;
            RegisterClass(&wc);
        }
        MapWindowPoints(hwndInPlace, hwndT, (LPPOINT)&rc, 2);
        hwndOrigin = CreateWindowEx(
                0,
                ORIGIN_CLASS,
                NULL,
                WS_CHILD | WS_CLIPSIBLINGS,
                rc.left,
                rc.top,
                rc.right - rc.left,
                rc.bottom - rc.top,
                hwndT,
                NULL,
                wc.hInstance,
                NULL);
        // Shove the bitmap into the first window long so that it can
        // be used for painting the origin bitmap in the window.
        //
        SetWindowLong(hwndOrigin, 0, (LONG)hbmp);
        hrgn = CreateEllipticRgn(0, 0, bmp.bmWidth + 1, bmp.bmHeight + 1);
        SetWindowRgn(hwndOrigin, hrgn, FALSE);

        MapWindowPoints(hwndT, hwndInPlace, (LPPOINT)&rc, 2);

        SetWindowPos(
                hwndOrigin,
                HWND_TOP,
                0,
                0,
                0,
                0,
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        UpdateWindow(hwndOrigin);

        DoReaderMode(&rmi);

        DestroyWindow(hwndOrigin);
        DeleteObject(hbmp);

        // USER owns the hrgn, it will clean it up
        //
        // DeleteObject(hrgn);
    }
    RemoveProp(hwndInPlace, TEXT("ReaderMode"));

Cleanup:
    pDoc->_pOptionSettings->fSmoothScrolling = fOptSmoothScroll;
#endif // ndef WIN16
    return;
}

#ifndef WIN16
//
// helper function: determine how many lines to scroll per mouse wheel
//
LONG
WheelScrollLines()
{
    LONG uScrollLines = 3; // reasonable default

    if ((g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS)
            || (g_dwPlatformID == VER_PLATFORM_WIN32_NT
                            && g_dwPlatformVersion < 0x00040000))
    {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Desktop"),
                0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
        {
            TCHAR szData[128];
            DWORD dwKeyDataType;
            DWORD dwDataBufSize = ARRAY_SIZE(szData);

            if (RegQueryValueEx(hKey, _T("WheelScrollLines"), NULL,
                        &dwKeyDataType, (LPBYTE) &szData, &dwDataBufSize)
                    == ERROR_SUCCESS)
            {
                uScrollLines = _tcstoul(szData, NULL, 10);
            }
            RegCloseKey(hKey);
        }
    }
    else if (g_dwPlatformID == VER_PLATFORM_WIN32_NT &&
                 g_dwPlatformVersion >= 0x00040000)
    {
        ::SystemParametersInfo(
                   SPI_GETWHEELSCROLLLINES,
                   0,
                   &uScrollLines,
                   0);
    }

    return uScrollLines;
}

HRESULT
CFlowLayout::HandleMouseWheel(CMessage * pMessage)
{
    // WM_MOUSEWHEEL       - line scroll mode
    // CTRL+WM_MOUSEWHEEL  - increase/decrease baseline font size.
    // SHIFT+WM_MOUSEWHEEL - navigate backward/forward
    //
    HRESULT hr          = S_OK;
    BOOL    fControl    = (pMessage->dwKeyState & FCONTROL)
                        ? (TRUE) : (FALSE);
    BOOL    fShift      = (pMessage->dwKeyState & FSHIFT)
                        ? (TRUE) : (FALSE);
    BOOL    fEditable   = ElementOwner()->IsEditable(TRUE);
    short   zDelta;
    short   zDeltaStep;
    short   zDeltaCount;

    const static LONG idmZoom[] = { IDM_BASELINEFONT1,
                                    IDM_BASELINEFONT2,
                                    IDM_BASELINEFONT3,
                                    IDM_BASELINEFONT4,
                                    IDM_BASELINEFONT5,
                                    0 };
    short       iZoom;
    MSOCMD      msocmd;

    if (!fControl && !fShift)
    {
        // mousewheel scrolling, allow partial circle scrolling.
        //
        zDelta = (short) HIWORD(pMessage->wParam);

        if (zDelta != 0)
        {
            long uScrollLines = WheelScrollLines();
            WORD wCode    = (uScrollLines >= 0)
                          ? ((zDelta > 0) ? (SB_LINEUP) : (SB_LINEDOWN))
                          : ((zDelta > 0) ? (SB_PAGEUP) : (SB_PAGEDOWN));
            LONG yPercent = (uScrollLines >= 0)
                          ? ((-zDelta * PERCENT_PER_LINE * uScrollLines) / WHEEL_DELTA)
                          : ((-zDelta * PERCENT_PER_PAGE * abs(uScrollLines)) / WHEEL_DELTA);

            ScrollByPercent(0, yPercent);
        }
    }
    else
    {
        CDoc *  pDoc = Doc();

        // navigate back/forward or zoomin/zoomout, should wait until full
        // wheel circle is accumulated.
        //
        zDelta = ((short) HIWORD(pMessage->wParam)) + pDoc->_iWheelDeltaRemainder;
        zDeltaStep = (zDelta < 0) ? (-1) : (1);
        zDeltaCount = zDelta / WHEEL_DELTA;
        pDoc->_iWheelDeltaRemainder  = zDelta - zDeltaCount * WHEEL_DELTA;

        for (; zDeltaCount != 0; zDeltaCount = zDeltaCount - zDeltaStep)
        {
            if (fShift)
            {
                hr = pDoc->Exec(
                        (GUID *) &CGID_MSHTML,
                        (zDelta > 0) ? (IDM_GOFORWARD) : (IDM_GOBACKWARD),
                        0,
                        NULL,
                        NULL);
                if (hr)
                    goto Cleanup;
            }
            else // fControl
            {
                if (!fEditable)
                {
                    // get current baseline font size
                    //
                    for (iZoom = 0; idmZoom[iZoom]; iZoom ++)
                    {
                        msocmd.cmdID = idmZoom[iZoom];
                        msocmd.cmdf  = 0;
                        hr = THR(pDoc->QueryStatus(
                                (GUID *) &CGID_MSHTML,
                                1,
                                &msocmd,
                                NULL));
                        if (hr)
                            goto Cleanup;

                        if (msocmd.cmdf == MSOCMDSTATE_DOWN)
                            break;
                    }

                    Assert(idmZoom[iZoom] != 0);
                    if (!idmZoom[iZoom])
                    {
                        hr = E_FAIL;
                        goto Cleanup;
                    }

                    iZoom -= zDeltaStep;

                    if (iZoom >= 0 && idmZoom[iZoom])
                    {
                        // set new baseline font size
                        //
                        hr = THR(pDoc->Exec(
                                (GUID *) &CGID_MSHTML,
                                idmZoom[iZoom],
                                0,
                                NULL,
                                NULL));
                        if (hr)
                            goto Cleanup;
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}
#endif // ndef WIN16

HRESULT
CFlowLayout::HandleButtonDblClk(CMessage *pMessage)
{
    // Repaint window to show any exposed portions
    //
    ViewChange(Doc()->_state >= OS_INPLACE ? TRUE : FALSE);
    return S_OK;
}

WORD ConvVKey(WORD vKey);
void TestMarkupServices(CElement *pElement);
void TestSelectionRenderServices( CMarkup* pMarkup, CElement* pTestElement);
void DumpFormatCaches();

HRESULT
CFlowLayout::HandleSysKeyDown(CMessage *pMessage)
{
    HRESULT    hr    = S_FALSE;
    CDoc     * pDoc  = Doc();

#if DBG == 1
    CMarkup * pMarkup   = GetContentMarkup();
#endif

    // BUGBUG: (anandra) Most of these should be handled as commands
    // not keydowns.  Use ::TranslateAccelerator to perform translation.
    //

    if (_fVertical)
        pMessage->wParam = ConvVKey(pMessage->wParam);

    if(pMessage->wParam == VK_BACK && (pMessage->lParam & SYS_ALTERNATE))
    {
        Sound();
        hr = S_OK;
    }
#if DBG == 1
    else if ((pMessage->wParam == VK_F4) && !(pMessage->dwKeyState & MK_ALT)
            && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
#ifdef MERGEFUN // iRuns
         CTreePosList & eruns = GetList();
         LONG iStart, iEnd, iDelta, i;
         CElement * pElement;

        if (GetKeyState(VK_SHIFT) & 0x8000)
        {
            iStart = 0;
            iEnd = eruns.Count() - 1;
            iDelta = +1;
        }
        else
        {
            iStart = eruns.Count() - 1;
            iEnd = 0;
            iDelta = -1;
        }

        pElement = NULL;
        for (i=iStart; i != iEnd; i += iDelta)
        {
            if (eruns.GetRunAbs(i).Cch())
            {
                pElement = eruns.GetRunAbs(i).Branch()->ElementOwner();
                break;
            }
        }
        if (pElement)
        {
            SCROLLPIN sp = (iDelta > 0) ? SP_TOPLEFT : SP_BOTTOMRIGHT;
            hr = ScrollElementIntoView( pElement, sp, sp );
        }
        else
        {
            hr = S_OK;
        }
#endif
    }
    else if (   pMessage->wParam == VK_F11
             && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
        if (GetKeyState(VK_SHIFT) & 0x8000)
            DumpFormatCaches();
        else
            pMarkup->DumpTree();
        hr = S_OK;
    }
    else if (   pMessage->wParam == VK_F12
             && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
        if (GetKeyState(VK_SHIFT) & 0x8000)
            TestMarkupServices(ElementOwner());
        else
            DumpLines();
        hr = S_OK;
    }
    else if (   pMessage->wParam == VK_F10
             && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
        //if (GetKeyState(VK_SHIFT) & 0x8000)
            TestSelectionRenderServices( pMarkup , ElementContent() );
        //else
        //      pMarkup->DumpTree();

        hr = S_OK;
    }
    else if (   pMessage->wParam == VK_F9
             && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
#ifdef MERGEFUN // iRuns
        CTreePosList & elementRuns = GetList();

        CNotification  nf;

        for (int i = 0; i < long(elementRuns.Count()); i++)
        {
            elementRuns.VoidCachedInfoOnBranch(elementRuns.GetBranchAbs(i));
        }
        pDoc->InvalidateTreeCache();

        nf.CharsResize(
                0,
                pMarkup->GetContentTextLength(),
                0,
                elementRuns.NumRuns(),
                GetFirstBranch());
        elementRuns.Notify(nf);
#endif
        hr = S_OK;
    }
    else if(   pMessage->wParam == VK_F8
            && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
        pMarkup->DumpClipboardText( );
    }
#endif

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::HitTestContent
//
//  Synopsis:   Determine if the given display leaf node contains the hit point.
//
//  Arguments:  pptHit          hit test point
//              pDispNode       pointer to display node
//              pClientData     client-specified data value for hit testing pass
//
//  Returns:    TRUE if the display leaf node contains the point
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CFlowLayout::HitTestContent(
    const POINT *   pptHit,
    CDispNode *     pDispNode,
    void *          pClientData)
{
    Assert(pptHit);
    Assert(pDispNode);
    Assert(pClientData);

    CHitTestInfo *  phti         = (CHitTestInfo *)pClientData;
    CDispNode *     pDispElement = GetElementDispNode(NULL, FALSE);

    //
    //  First, if allowed, see if a child element is hit
    //  NOTE: Only check content when the hit display node is a content node
    //
    phti->_htc = (  !(phti->_grfFlags & HT_ELEMENTSWITHLAYOUT)
                &&  (   !pDispElement->IsContainer()
                    ||  pDispElement != pDispNode))
                       ? BranchFromPoint(phti->_grfFlags,
                                         *pptHit,
                                         &phti->_pNodeElement,
                                         phti->_phtr)
                       : HTC_NO;

    //
    //  If no child was hit, use default handling
    //

    if (phti->_htc == HTC_NO)
    {
        super::HitTestContent(pptHit, pDispNode, pClientData);
    }

    //
    //  Otherwise, save the point and CDispNode associated with the hit
    //

    else
    {
        phti->_ptContent = *pptHit;
        phti->_pDispNode = pDispNode;
    }

    return (phti->_htc != HTC_NO);
}


//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::NotifyScrollEvent
//
//  Synopsis:   Respond to a change in the scroll position of the display node
//
//----------------------------------------------------------------------------

void
CFlowLayout::NotifyScrollEvent()
{
// BUGBUG: Add 1st visible cp tracking here....(brendand)?
    super::NotifyScrollEvent();
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CFlowLayout::DumpDebugInfo
//
//  Synopsis:   Dump debugging information for the given display node.
//
//  Arguments:  hFile           file handle to dump into
//              level           recursive tree level
//              childNumber     number of this child within its parent
//              pDispNode       pointer to display node
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CFlowLayout::DumpDebugInfo(
        HANDLE hFile,
        long level,
        long childNumber,
        CDispNode *pDispNode,
        void *cookie)
{
    if (    pDispNode->GetNodeType()  != DISPNODETYPE_ITEM
        ||  pDispNode->GetLayerType() != DISPNODELAYER_FLOW)
    {
        super::DumpDebugInfo(hFile, level, childNumber, pDispNode, cookie);
    }
    else
    {
#if 0
        WriteString(hFile, _T("<br>\r\n"));
        WriteString(hFile, _T("<xmp>"));
        _dp.DumpLineText(hFile, (long)cookie);
        WriteString(hFile, _T("\r\n</xmp>"));
#else
        WriteString(hFile, _T("<br>\r\n<font class=flow>text node</font>\r\n<br>"));
#endif
    }
}
#endif

//+----------------------------------------------------------------------------
//
//  Member:     GetElementDispNode
//
//  Synopsis:   Return the display node for the pElement
//
//  Arguments:  pElement   - CElement whose display node is to obtained
//              fForParent - If TRUE (the default), return the display node a parent
//                           inserts into the tree. Otherwise, return the primary
//                           display node.
//                           NOTE: This only makes a difference with layouts that
//                                 have a filter.
//
//  Returns:    Pointer to the element CDispNode if one exists, NULL otherwise
//
//-----------------------------------------------------------------------------
CDispNode *
CFlowLayout::GetElementDispNode(
    CElement *  pElement,
    BOOL        fForParent) const
{
    return (    !pElement
            ||  pElement == ElementOwner()
                    ? super::GetElementDispNode(pElement, fForParent)
                    : pElement->IsRelative()
                        ? ((CFlowLayout *)this)->_dp.FindElementDispNode(pElement)
                        : NULL);
}

//+-------------------------------------------------------------------------
//
//  Method:     YieldCurrencyHelper
//
//  Synopsis:   Relinquish currency
//
//  Arguments:  pElemNew    New site that wants currency
//
//--------------------------------------------------------------------------
HRESULT
CFlowLayout::YieldCurrencyHelper(CElement * pElemNew)
{
    HRESULT hr = S_OK;

    Assert(pElemNew != ElementOwner());

#ifndef NO_IME
    // Restore the IMC if we've temporarily disabled it.  See OnSetFocus().

    if (Doc()->_himcCache)
    {
        ImmAssociateContext(Doc()->_pInPlace->_hwnd, Doc()->_himcCache);
        Doc()->_himcCache = NULL;
    }

    // BUGBUG (cthrash) Need to complete implementation in mshtmled.
    // if (IsIMEComposition())
    // {
    //     CFlowLayout * pLayout = this;
    //     _ime->TerminateIMEComposition(*pLayout, CIme::TERMINATE_NORMAL);
    // }
#endif

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     BecomeCurrentHelper
//
//  Synopsis:   Force currency on the site.
//
//  Notes:      This is the method that external objects should call
//              to force sites to become current.
//
//--------------------------------------------------------------------------
HRESULT
CFlowLayout::BecomeCurrentHelper(long lSubDivision, BOOL *pfYieldFailed, CMessage * pMessage)
{
    BOOL    fOnSetFocus = ::GetFocus() == Doc()->_pInPlace->_hwnd;
    HRESULT hr          = S_OK;

    Doc()->_fInhibitFocusFiring = TRUE;

    // BUG44836 -- moved here from OnSetFocus -- OnSetFocus() was calling
    // this fn more aggressively than we want.
    //
    IGNORE_HR(NotifyKillSelection());

    // Call OnSetFocus directly if the doc's window already had focus. We
    // don't need to do this if the window didn't have focus because when
    // we take focus in BecomeCurrent, the window message handler does this.
    //
    if (fOnSetFocus)
    {
        // if our inplace window did have the focus, then fire the onfocus
        // only if onblur was previously fired and the body is becoming the
        // current site and currency did not change in onchange or onpropchange
        //
        if (ElementOwner() == Doc()->_pElemCurrent && ElementOwner() == Doc()->GetPrimaryElementClient())
            Doc()->Fire_onfocus();
    }

    Doc()->_fInhibitFocusFiring = FALSE;

    RRETURN1(hr, S_FALSE);
}


HRESULT
CFlowLayout::CreateControlRange(int cnt, CLayout ** ppLayout, IDispatch ** ppDisp)
{
    return S_OK;

    // BUGBUG (MohanB) This will be fixed by MarkA
/*
    HRESULT             hr = E_INVALIDARG;

    CAutoTxtSiteRange * pATSR = NULL;

    if (!ppDisp)
        goto Cleanup;

    pATSR = new CAutoTxtSiteRange(this);
    if (!pATSR)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pATSR->QueryInterface(IID_IDispatch, (void **) ppDisp));
    pATSR->Release();
    if (hr)
    {
        *ppDisp = NULL;
        goto Cleanup;
    }

    if ((ppLayout) && (cnt))
    {
         //Initialize: copy the _arySitesSelected from this to ppDisp
        for(; cnt>0; cnt--, ppLayout++)
        {
            hr = THR(pATSR->_aryLayoutsSelected.Append(*ppLayout));
            if (hr)
                goto Error;
        }
   }

Cleanup:
    RRETURN(hr);

Error:
    if (pATSR)
    {
        delete pATSR;
        (*ppDisp) = NULL;
    }
    goto Cleanup;
*/
}


void
CFlowLayout::ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected)
{
    Assert(ptpStart && ptpEnd && ptpStart->GetMarkup() == ptpStart->GetMarkup());

    // If this has a slave markup, but the selection is in the main markup, then
    // select the element (as opposed to part or all of its content)
    if  ( (    ElementOwner()->HasSlaveMarkupPtr()
           &&  ptpStart->GetMarkup() != ElementOwner()->GetSlaveMarkupPtr() ) ||
          ( ElementOwner()->_etag == ETAG_HTMLAREA ) )
    {
#if DBG == 1
        {
/*
            CTreePos * ptpElemStart, * ptpElemEnd;

            ElementOwner()->GetTreeExtent(&ptpElemStart, &ptpElemEnd);
            Assert(ptpElemStart == ptpStart && ptpElemEnd == ptpEnd);
*/
        }
#endif
        // select the element
        const CCharFormat *pCF = GetFirstBranch()->GetCharFormat();

        // Set the site text selected bits appropriately
        SetSiteTextSelection (
            fSelected,
            pCF->SwapSelectionColors());

        Invalidate();
    }
    else
    {
		if (!FExternalLayout())
		{
			_dp.ShowSelected( ptpStart, ptpEnd, fSelected);
		}
		else
		{
			if (_pQuillGlue)
				_pQuillGlue->ShowSelected(ptpStart, ptpEnd, fSelected);
		}
    }
    OnSelectionChange();
}

//----------------------------------------------------------------------------
//  RegionFromElement
//
//  DESCRIPTION:
//      This is a virtual wrapper function to wrap the RegionFromElement that is
//      also implemented on the flow layout.
//      The RECT returned is in client coordinates.
//
//----------------------------------------------------------------------------

void
CFlowLayout::RegionFromElement( CElement * pElement,
                                CDataAry<RECT> * paryRects,
                                RECT * prcBound)
{
    Assert( pElement);
    Assert( prcBound );

    if ( !pElement || !prcBound )
        return;

    // Is the element passed the same element with the owner?
    if ( _pElementOwner == pElement )
    {
        // call CLayout implementation.
        super::RegionFromElement( pElement, paryRects, prcBound);
    }
    else
    {
//
// BUGBUG: [FerhanE]
//      Change this part to support relative coordinates, after Srini implements that,
//
        // Delegate the call to the CDisplay implementation
        _dp.RegionFromElement( pElement,          // the element
                               paryRects,         // rects returned here
                               NULL,              // offset the rects returned
                               NULL,              // ask RFE to get CFormDrawInfo
                               0,                 // coord w/ respect to the client rc.
                               -1,                // Get the complete focus
                               -1,                //
                               prcBound);         // bounds of the element!
    }
}
