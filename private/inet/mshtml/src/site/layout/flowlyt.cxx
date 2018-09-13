//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       flowlyt.cxx
//
//  Contents:   Implementation of CFlowLayout and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
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

#ifndef X_DISPITEMPLUS_HXX_
#define X_DISPITEMPLUS_HXX_
#include "dispitemplus.hxx"
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

#ifndef X_ADORNER_HXX_
#define X_ADORNER_HXX_
#include "adorner.hxx"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifdef UNIX
extern "C" HANDLE MwGetPrimarySelectionData();
#include "mainwin.h"
#include "quxcopy.hxx"

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif
#endif //UNIX

#define DO_PROFILE  0
#define MAX_RECURSION_LEVEL 40

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
            Reset(TRUE);
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
//    Assert(!pnf->IsReceived(_snLast));

    DWORD   dwData;
    BOOL    fHandle = TRUE;

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
            Assert(pnf->Cp(GetContentFirstCp()) >= 0);
        }
#endif

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

        else if (IsInvalidationNotification(pnf))
        {
            //
            //  Invalidate the entire layout if the associated element initiated the request
            //

            if (   ElementOwner() == pnf->Element() 
                || pnf->IsType(NTYPE_ELEMENT_INVAL_Z_DESCENDANTS))
            {
                Invalidate();
            }


            //
            //  Otherwise, isolate the appropriate range and invalidate the derived rectangles
            //

            else
            {
                long    cpFirst = GetContentFirstCp();
                long    cpLast  = GetContentLastCp();
                long    cp      = pnf->Cp(cpFirst) - cpFirst;
                long    cch     = pnf->Cch(cpLast);

                Assert( pnf->IsType(NTYPE_ELEMENT_INVALIDATE)
                    ||  pnf->IsType(NTYPE_CHARS_INVALIDATE));
                Assert(cp  >= 0);
                Assert(cch >= 0);

                //
                //  Obtain the rectangles if the request range is measured
                //

                if (    IsRangeBeforeDirty(cp, cch)
                    &&  (cp + cch) <= GetDisplay()->_dcpCalcMax)
                {
                    CDataAry<RECT>  aryRects(Mt(CFlowLayoutNotify_aryRects_pv));
                    CTreeNode *pNotifiedNode = pnf->Element()->GetFirstBranch();
                    CTreeNode *pRelativeNode;
                    CRelDispNodeCache *pRDNC;

                    GetDisplay()->RegionFromRange(&aryRects, pnf->Cp(cpFirst), cch );

                    // If the notified element is relative or is contained
                    // within a relative element (i.e. what we call "inheriting"
                    // relativeness), then we need to find the element that's responsible
                    // for the relativeness, and invalidate its dispnode.
                    if ( pNotifiedNode->IsInheritingRelativeness() )
                    {
                        pRDNC = _dp.GetRelDispNodeCache();
                        if ( pRDNC ) 
                        {
                            // BUGBUG: this assert is legit; remove the above if clause
                            // once OnPropertyChange() is modified to not fire invalidate
                            // when its dwFlags have remeasure in them.  The problem is
                            // that OnPropertyChange is invalidating when we've been
                            // asked to remeasure, so the dispnodes/reldispnodcache may
                            // not have been created yet.
                            Assert( pRDNC && "Must have a RDNC if one of our descendants inherited relativeness!" );                       
                            // Find the element that's causing the notified element
                            // to be relative.  Search up to the flow layout owner.
                            pRelativeNode = pNotifiedNode->GetCurrentRelativeNode( ElementOwner() );
                            // Tell the relative dispnode cache to invalidate the
                            // requested region of the relative element
                            pRDNC->Invalidate( pRelativeNode->Element(), &aryRects[0], aryRects.Size() );
                        }
                    }
                    else
                    {
                        Invalidate(&aryRects[0], aryRects.Size());
                    }
                }

                //
                //  Otherwise, if a dirty region exists, extend the dirty region to encompass it
                //  NOTE: Requests within unmeasured regions are handled automatically during
                //        the measure
                //

                else if (IsDirty())
                {
                    _dtr.Accumulate(pnf, GetContentFirstCp(), GetContentLastCp(), FALSE);
                }
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
        //  Handle changes to CSS display and visibility
        //

        else if (   pnf->IsType(NTYPE_DISPLAY_CHANGE)
                ||  pnf->IsType(NTYPE_VISIBILITY_CHANGE))
        {
            HandleVisibleChange(pnf->IsType(NTYPE_VISIBILITY_CHANGE));

            if (_fContainsRelative)
            {
                if (pnf->IsType(NTYPE_VISIBILITY_CHANGE))
                    _dp.EnsureDispNodeVisibility();
                else
                    _dp.HandleDisplayChange();
            }
        }

        //
        //  Insert adornments as needed
        //

        else if (pnf->IsType(NTYPE_ELEMENT_ADD_ADORNER))
        {
            fHandle = HandleAddAdornerNotification(pnf);
        }

        //
        //  Otherwise, accumulate the information
        //

        else if (   pnf->IsTextChange()
                ||  pnf->IsLayoutChange())
        {
            long    cpFirst     = GetContentFirstCp();
            long    cpLast      = GetContentLastCp();
            BOOL    fIsAbsolute = FALSE;

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
                    fIsAbsolute = TRUE;

                    QueueRequest(CRequest::RF_MEASURE, pnf->Element());
                }
            }

            //
            //  Otherwise, collect the range covered by the notification
            //

            if (    (   (   !fIsAbsolute
                        ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE))
                    ||  pnf->Element() == ElementOwner())
                &&  pnf->Cp(cpFirst) >= 0)
            {
                //
                //  Accumulate the affected range
                //

                _dtr.Accumulate(pnf,
                                cpFirst,
                                cpLast,
                                (   pnf->Element() == ElementOwner()
                                &&  !fRangeSet));

                //
                // Content's are dirtied so reset the minmax flag on the display
                //
                _dp._fMinMaxCalced = FALSE;

                //
                //  Mark forced layout if requested
                //

                if (pnf->IsFlagSet(NFLAGS_FORCE))
                {
                    if (pnf->Element() == ElementOwner())
                    {
                        _fForceLayout = TRUE;
                    }
                    else
                    {
                        _fDTRForceLayout = TRUE;
                    }
                }

                //
                //  Invalidate cached min/max values when content changes size
                //

                if (    !_fPreserveMinMax
                    &&  _fMinMaxValid 
                    &&  (   pnf->IsType(NTYPE_ELEMENT_MINMAX)
                        ||  (   _fContentsAffectSize
                            &&  (   pnf->IsTextChange()
                                ||  pnf->IsType(NTYPE_ELEMENT_RESIZE)
                                ||  pnf->IsType(NTYPE_ELEMENT_REMEASURE)
                                ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE)
                                ||  pnf->IsType(NTYPE_CHARS_RESIZE)))))
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

             if (    fIsAbsolute
                 ||  (  !pnf->IsFlagSet(NFLAGS_DONOTLAYOUT)
                     &&  !_fSizeThis
                     &&  IsDirty()))
            {
                PostLayoutRequest(pnf->LayoutFlags() | LAYOUT_MEASURE);
            }
#if DBG==1
            else if (   !pnf->IsFlagSet(NFLAGS_DONOTLAYOUT)
                    &&  !_fSizeThis)
            {
                Assert( !IsDirty()
                    ||  !GetView()->IsActive()
                    ||  IsDisplayNone()
                    ||  GetView()->HasLayoutTask(this));
            }
#endif
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
    case NTYPE_DOC_STATE_CHANGE_1:
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

    case NTYPE_ELEMENT_EXITTREE_1:
        Reset(TRUE);
        break;

    case NTYPE_ZERO_GRAY_CHANGE:
        HandleZeroGrayChange( pnf );
        break; 
        
    case NTYPE_RANGE_ENSURERECALC:
    case NTYPE_ELEMENT_ENSURERECALC:

        fHandle = pnf->Element() != ElementOwner();

        //
        //  If the request is for this element and layout is dirty,
        //  convert the pending layout call to a dirty range in the parent layout
        //  (Processing the pending layout call immediately could result in measuring
        //   twice since the parent may be dirty as well - Converting it into a dirty
        //   range in the parent is only slightly more expensive than processing it
        //   immediately and prevents the double measuring, keeping things in the
        //   right order)
        //

        if (    pnf->Element() == ElementOwner()
            &&  IsDirty())
        {
            ElementOwner()->ResizeElement();
            WHEN_DBG(pnf->ResetSN());
        }

        //
        //  Otherwise, calculate up through the requesting element
        //

        else
        {
            CView * pView   = Doc()->GetView();
            long    cpFirst = GetContentFirstCp();
            long    cpLast  = GetContentLastCp();
            long    cp;
            long    cch;

            //
            //  If the requesting element is the element owner,
            //  calculate up through the end of the available content
            //

            if (pnf->Element() == ElementOwner())
            {
                cp  = GetDisplay()->_dcpCalcMax;
                cch = cpLast - (cpFirst + cp);
            }

            //
            //  Otherwise, calculate up through the element
            //

            else
            {
                ElementOwner()->SendNotification(NTYPE_ELEMENT_ENSURERECALC);

                cp  = pnf->Cp(cpFirst) - cpFirst;
                cch = pnf->Cch(cpLast);
            }

            if(pView->IsActive())
            {
                CView::CEnsureDisplayTree   edt(pView);

                if (    !IsRangeBeforeDirty(cp, cch)
                    ||  GetDisplay()->_dcpCalcMax <= (cp + cch))
                {
                    GetDisplay()->WaitForRecalc((cpFirst + cp + cch), -1);
                }

                if (    pnf->IsType(NTYPE_ELEMENT_ENSURERECALC)
                    &&  pnf->Element() != ElementOwner())
                {
                    ProcessRequest(pnf->Element() );
                }
            }
        }
        break;
    }


    //
    //  Handle the notification
    //
    //

    if (fHandle && pnf->IsFlagSet(NFLAGS_ANCESTORS))
    {
        pnf->SetHandler(ElementOwner());
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
//  Member:     Reset
//
//  Synopsis:   Reset the channel (clearing any dirty state)
//
//-----------------------------------------------------------------------------
void
CFlowLayout::Reset(BOOL fForce)
{
    super::Reset(fForce);
    CancelChanges();
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
    _xMax =
    _xMin = -1;
    _fCanHaveChildren = TRUE;
    Assert(ElementContent());
    ElementContent()->_fOwnsRuns = TRUE;
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
    _dp._dxCaret    = ElementOwner()->IsEditable(TRUE) ? 1 : 0;

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


void
CFlowLayout::DoLayout(
    DWORD   grfLayout)
{
    Assert(grfLayout & (LAYOUT_MEASURE | LAYOUT_POSITION | LAYOUT_ADORNERS));

    CElement *  pElementOwner = ElementOwner();

    //
    //  If this layout is no longer needed, ignore the request and remove it
    //

    if (    pElementOwner->HasLayout()
        &&  !pElementOwner->NeedsLayout())
    {
        ElementOwner()->DestroyLayout();
    }

    //
    //  Hidden layout should just accumulate changes
    //  (It will be measured when re-shown)
    //

    else if(    !IsDisplayNone()
            ||  Tag() == ETAG_BODY)
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
            // we want to do this each time inorder to
            // properly pick up things like opacity.
            if (_fForceLayout)
            {
                CI._grfLayout |= LAYOUT_FORCE;
            }

            EnsureDispNode(&CI, !!(CI._grfLayout & LAYOUT_FORCE));


            if (    IsDirty()
                ||  (CI._grfLayout & (LAYOUT_FORCE | LAYOUT_FORCEFIRSTCALC)))
            {
                CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);

                CalcTextSize(&CI, &size);

                if (CI._grfLayout & LAYOUT_FORCE)
                {
                    SizeDispNode(&CI, size);
                    SizeContentDispNode(CSize(_dp.GetMaxWidth(), _dp.GetHeight()));

                    if (pElementOwner->IsAbsolute())
                    {
                        ElementOwner()->SendNotification(NTYPE_ELEMENT_SIZECHANGED);
                    }
                }
            }

            Reset(FALSE);
        }
        _fForceLayout = FALSE;

        //
        //  Process outstanding layout requests (e.g., sizing positioned elements, adding adorners)
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

        Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
    }
    else
    {
        FlushRequests();
        RemoveLayoutRequest();
        Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
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
    DWORD_PTR       dw;
    BOOL            fFoundAtLeastOne = FALSE;

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

            fFoundAtLeastOne = TRUE;
        }
    }
    ClearLayoutIterator(dw, FALSE);

    _fPreserveMinMax = FALSE;

    // clear the flag if there was no work done.  oppurtunistic cleanup
    _fChildHeightPercent = fFoundAtLeastOne ;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFlowLayout::CalcSize
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

DWORD
CFlowLayout::CalcSize( CCalcInfo * pci,
                       SIZE      * psize,
                       SIZE      * psizeDefault)
{
    CSaveCalcInfo   sci(pci, this);
    CScopeFlag      csfCalcing(this);
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
        _fPositionSet      = FALSE;
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
        &&  !ContainsVertPercentAttr()
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
                SizeDispNode(pci, *psize);
                SizeContentDispNode(CSize(_dp.GetMaxWidth(), _dp.GetHeight()));
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
        Reset(FALSE);
        Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
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
    BOOL        fNormalMode = ( pci->_smMode == SIZEMODE_NATURAL
                            ||  pci->_smMode == SIZEMODE_SET);
    BOOL        fFullRecalc;
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

        GetClientRect(&rcView, CLIENTRECT_USERECT, pci);

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

        GetClientRect(&rcView, CLIENTRECT_USERECT, pci);
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
                || (    ContainsVertPercentAttr()
                    &&  _dp.GetViewHeight() != yViewHeightOld)
                ||  !fNormalMode
                ||  (pci->_grfLayout & LAYOUT_FORCE);

  
    if (fFullRecalc)
    {
        CSaveCalcInfo   sci(pci, this);
        BOOL            fWordWrap = _dp.GetWordWrap();

        if (_fDTRForceLayout)
            pci->_grfLayout |= LAYOUT_FORCE;

        //
        // If the text will be fully recalc'd, cancel any outstanding changes
        //

        CancelChanges();

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

        if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            _dp.SetWordWrap(FALSE);
        }

        if (    fNormalMode
            &&  !(pci->_grfLayout & LAYOUT_FORCE)
            &&  !ContainsHorzPercentAttr()          // if we contain a horz percent attr, then RecalcLineShift() is insufficient; we need to do a full RecalcView()
            &&  _fMinMaxValid
            &&  _dp._fMinMaxCalced
            &&  cxView >= _dp._xMaxWidth
            &&  !_fSizeToContent)
        {
            Assert(_dp._xWidthView  == cxView);
            Assert(!_fChildWidthPercent);
            Assert(!ContainsChildLayout());

            _dp.RecalcLineShift(pci, pci->_grfLayout);
        }

        else
        {
            _fAutoBelow        = FALSE;
            _fContainsRelative = FALSE;
            _dp.RecalcView(pci, fFullRecalc);
        }

        //
        // Inval since we are doing a full recalc
        //
        Invalidate();

        if (fNormalMode)
        {
            _dp._fMinMaxCalced = FALSE;
        }

        if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            _dp.SetWordWrap(fWordWrap);
        }
    }
    //
    // If only a partial recalc is necessary, commit the changes
    //
    else if (!IsCommitted())
    {
        Assert(pci->_smMode != SIZEMODE_MMWIDTH);
        Assert(pci->_smMode != SIZEMODE_MINWIDTH);

        CommitChanges(pci);
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
                _dp.GetSize(psize);
                ((CSize *)psize)->Max(((CRect &)rcView).Size());
            }
            else
            {
                psize->cx = cxView;
                psize->cy = cyView;
            }
            break;

        case SIZEMODE_MMWIDTH:
        {
            psize->cx = _dp.GetWidth();
            psize->cy = _dp._xMinWidth;

            if (psizeDefault)
            {
                psizeDefault->cx = _dp.GetWidth() + cxAdjustment;
                psizeDefault->cy = _dp.GetHeight() + cyAdjustment;
            }

            break;
        }
        case SIZEMODE_MINWIDTH:
        {

            psize->cx = _dp.GetWidth();
            psize->cy = _dp.GetHeight();
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
    SCROLLPIN   spHorz,
    BOOL        fScrollBits)
{
extern void BoundingRectForAnArrayOfRectsWithEmptyOnes(RECT *prcBound, CDataAry<RECT> * paryRects);

    HRESULT hr = S_OK;

    if (    _pDispNode
        &&  cpMin >= 0)
    {
        CStackDataAry<RECT, 5>  aryRects(Mt(CFlowLayoutScrollRangeInfoView_aryRects_pv));
        CRect                   rc;
        CCalcInfo               CI(this);

        hr = THR(WaitForParentToRecalc(cpMost, -1, &CI));
        if (hr)
            goto Cleanup;

        _dp.RegionFromElement( ElementOwner(),      // the element
                                 &aryRects,         // rects returned here
                                 NULL,              // offset the rects
                                 NULL,              // ask RFE to get CFormDrawInfo
                                 RFE_SCROLL_INTO_VIEW, // coord w/ respect to the display and not the client rc
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

        ScrollRectIntoView(rc, spVert, spHorz, fScrollBits);
        hr = S_OK;
    }
    else
    {
        hr = super::ScrollRangeIntoView(cpMin, cpMost, spVert, spHorz, fScrollBits);
    }


Cleanup:
    RRETURN1(hr, S_FALSE);
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
                          LONG      *pxWidth,
                          LONG      *pyHeight,
                          INT       *pxMinSiteWidth)

{
    CDoc *pDoc = Doc();
    LONG xLeftMargin, xRightMargin;
    LONG yTopMargin, yBottomMargin;
    SIZE sizeObj;

    Assert (pLayout && pxWidth);

    *pxWidth = 0;

    if (pxMinSiteWidth)
        *pxMinSiteWidth = 0;

    if (pyHeight)
        *pyHeight       = 0;

    if(pLayout->IsDisplayNone())
        return FALSE;

    // get the margin info for the site
    pLayout->GetMarginInfo(pci, &xLeftMargin, &yTopMargin, &xRightMargin, &yBottomMargin);

    //
    // measure the site
    //
    if (pDoc->_lRecursionLevel == MAX_RECURSION_LEVEL)
    {
        AssertSz(0, "Max recursion level reached!");
        sizeObj.cx = 0;
        sizeObj.cy = 0;
    }
    else
    {
        LONG lRet;

        pDoc->_lRecursionLevel++;
        lRet = MeasureSite(pLayout,
                   pci,
                   xWidthMax - xLeftMargin - xRightMargin,
                   fBreakAtWord,
                   &sizeObj,
                   pxMinSiteWidth);
        pDoc->_lRecursionLevel--;

        if (lRet)
        {
            return TRUE;
        }
    }
    
    //
    // Propagate the _fAutoBelow bit, if the child is auto positioned or
    // non-zparent children have auto positioned children
    //
    if (!_fAutoBelow)
    {
        const CFancyFormat * pFF = pLayout->GetFirstBranch()->GetFancyFormat();

        if (    pFF->IsAutoPositioned()
            ||  (   !pFF->IsZParent()
                &&  (pLayout->_fContainsRelative || pLayout->_fAutoBelow)))
        {
            _fAutoBelow = TRUE;
        }
    }

    // not adjust the size and proposed x pos to include margins
    *pxWidth         = max(0L, xLeftMargin + xRightMargin + sizeObj.cx);

    if (pxMinSiteWidth)
    {
        *pxMinSiteWidth += max(0L, xLeftMargin + xRightMargin);
    }

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

    // if the layout we are measuring (must be a child of ours)
    // is percent sized, then we should take this oppurtunity
    // to set some work-flags 
    {
        const CFancyFormat *pFF = pLayout->GetFirstBranch()->GetFancyFormat();

        if (pFF->_fHeightPercent )
        {
            _fChildHeightPercent = TRUE;
        }

        if (pFF->_fWidthPercent )
        {
            _fChildWidthPercent = TRUE;
        }
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

        if (pci->_smMode == SIZEMODE_MMWIDTH && pxMinWidth)
        {
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
    long cp;
    long cchOld;
    long cchNew;
    BOOL fForce = !!_fDTRForceLayout;

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

        if (fForce)
            pci->_grfLayout |= LAYOUT_FORCE;

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
    if (IsDirty())
    {
        _dtr.Reset();
    }
    _fDTRForceLayout = FALSE;
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
CFlowLayout::Draw(CFormDrawInfo *pDI, CDispNode * pDispNode)
{
    // CFlowLayout and text rendering know how to deal with device coordinates,
    // which are necessary for Win 9x because of the 16-bit coordinate limitation
    // in GDI.
    pDI->SetDeviceCoordinateMode();    

    GetDisplay()->Render(pDI, pDI->_rc, pDI->_rcClip, pDispNode);
}

//+----------------------------------------------------------------------------
//
//  Member:     GetTextNodeRange
//
//  Synopsis:   Return the range of lines that the given text flow node
//              owns
//
//  Arguments:  pDispNode   - text flow disp node.
//              piliStart   - return parameter for the index of the start line
//              piliFinish  - return parameter for the index of the last line
//
//-----------------------------------------------------------------------------
void
CFlowLayout::GetTextNodeRange(CDispNode * pDispNode, long * piliStart, long * piliFinish)
{
    Assert(pDispNode);
    Assert(piliStart);
    Assert(piliFinish);

    *piliStart = 0;
    *piliFinish = GetDisplay()->LineCount();

    //
    // First content disp node does not have a cookie
    //
    if (pDispNode != GetFirstContentDispNode())
    {
        *piliStart = (LONG)(LONG_PTR)pDispNode->GetExtraCookie();
    }

    pDispNode = pDispNode->GetNextSiblingNode(TRUE);

    while (pDispNode)
    {
        if (this == pDispNode->GetDispClient())
        {
            *piliFinish = (LONG)(LONG_PTR)pDispNode->GetExtraCookie();
            break;
        }
        pDispNode = pDispNode->GetNextSiblingNode(TRUE);
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

    if(pElement->IsRelative() && !pElement->NeedsLayout())
    {
        GetFlowPosition(pElement, ppt);
    }
    else
    {
        ppt->x = ppt->y = 0;

        // get the tree extent of the element of the layout passed in
        pElement->GetTreeExtent(&ptpStart, NULL);

        if (_dp.RenderedPointFromTp(ptpStart->GetCp(), ptpStart, FALSE, *ppt, &rp, TA_TOP) < 0)
            return;

        // if the NODE is RTL, mirror the point
        if(_dp.IsRTL())
            ppt->x = - ppt->x;

        if(pElement->NeedsLayout())
        {
            ppt->y += pElement->GetLayoutPtr()->GetYProposed();
        }
        ppt->y += rp->GetYTop();
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
    DWORD           dwFlags,
    POINT           pt,
    CTreeNode    ** ppNodeBranch,
    HITTESTRESULTS* presultsHitTest,
    BOOL            fNoPseudoHit,
    CDispNode     * pDispNode)
{
    HTC         htc = HTC_YES;
    CLinePtr    rp(&_dp);
    CTreePos  * ptp   = NULL;
    LONG        cp, ili, yLine;
    DWORD       dwCFPFlags = 0;
    CRect       rc(pt, pt);
    long        iliStart  = -1;
    long        iliFinish = -1;


    dwCFPFlags |= (dwFlags & HT_ALLOWEOL)
                    ? CDisplay::CFP_ALLOWEOL : 0;
    dwCFPFlags |= !(dwFlags & HT_DONTIGNOREBEFOREAFTER)
                    ? CDisplay::CFP_IGNOREBEFOREAFTERSPACE : 0;
    dwCFPFlags |= !(dwFlags & HT_NOEXACTFIT)
                    ? CDisplay::CFP_EXACTFIT : 0;
    dwCFPFlags |= fNoPseudoHit
                    ? CDisplay::CFP_NOPSEUDOHIT : 0;

    Assert(ElementOwner()->IsVisible(FALSE) || ElementOwner()->Tag() == ETAG_BODY);

    *ppNodeBranch = NULL;

    //
    // if the current layout has multiple text nodes then compute the
    // range of lines the belong to the current dispNode
    //
    if (_dp._fHasMultipleTextNodes && pDispNode)
    {
        GetTextNodeRange(pDispNode, &iliStart, &iliFinish);
    }

    if (pDispNode == GetElementDispNode())
    {
        GetClientRect(&rc); 
        rc.MoveTo(pt);
    }

    ili = _dp.LineFromPos(rc, &yLine, &cp, CDisplay::LFP_ZORDERSEARCH   |
                                           CDisplay::LFP_IGNORERELATIVE |
                                           CDisplay::LFP_IGNOREALIGNED  |
                                            (fNoPseudoHit
                                                ? CDisplay::LFP_EXACTLINEHIT
                                                : 0),
                                            iliStart,
                                            iliFinish);
    if(ili < 0)
    {
        htc = HTC_NO;
        goto Cleanup;
    }

    if ((cp = _dp.CpFromPointEx(ili, yLine, cp, pt, &rp, &ptp, NULL, dwCFPFlags,
                              &presultsHitTest->_fRightOfCp, &presultsHitTest->_fPseudoHit,
                              &presultsHitTest->_cchPreChars, NULL)) == -1 ) // fExactFit=TRUE to look at whole characters
    {
        htc = HTC_NO;
        goto Cleanup;
    }

    if (   cp < GetContentFirstCp()
        || cp > GetContentLastCp()
       )
    {
        htc = HTC_NO;
        goto Cleanup;
    }

    presultsHitTest->_cpHit  = cp;
    presultsHitTest->_iliHit = rp;
    presultsHitTest->_ichHit = rp.RpGetIch();

    if (_fIsEditable && ptp->IsNode() && ptp->ShowTreePos()
                     && (cp + 1 == ptp->GetBranch()->Element()->GetFirstCp()))
    {
        presultsHitTest->_fWantArrow = TRUE;
        htc = HTC_YES;
        *ppNodeBranch = ptp->GetBranch();
    }
    else
    {
        if (pDispNode)
        {
            pt.y += pDispNode->GetPosition().y;
        }

        htc = BranchFromPointEx(pt, rp, ptp, NULL, ppNodeBranch, presultsHitTest->_fPseudoHit,
                                &presultsHitTest->_fWantArrow,
                                !(dwFlags & HT_DONTIGNOREBEFOREAFTER));
    }

Cleanup:
    if (htc != HTC_YES)
        presultsHitTest->_fWantArrow = TRUE;
    return htc;
}

extern BOOL PointInRectAry(POINT pt, CStackDataAry<RECT, 1> &aryRects);

HTC
CFlowLayout::BranchFromPointEx(
    POINT        pt,
    CLinePtr  &  rp,
    CTreePos  *  ptp,
    CTreeNode *  pNodeRelative,  // (IN) non-NULL if we are hit-testing a relative element (NOT its flow position)
    CTreeNode ** ppNodeBranch,   // (OUT) returns branch that we hit
    BOOL         fPseudoHit,     // (IN) if true, text was NOT hit (CpFromPointEx() figures this out)
    BOOL       * pfWantArrow,    // (OUT) 
    BOOL         bIgnoreBeforeAfter)
{
    const CCharFormat * pCF;
    CTreeNode * pNode = NULL;
    CElement  * pElementStop = NULL;
    HTC         htc   = HTC_YES;
    BOOL        fVisible = TRUE;
    Assert(ptp);

    //
    // If we are on a line which contains an table, and we are not ignoring before and
    // aftrespace, then we want to hit that table...
    //
    if (!bIgnoreBeforeAfter)
    {
        CLine * pli = rp.CurLine();
        if (   pli
            && pli->_fSingleSite
            && pli->_cch == rp.RpGetIch()
           )
        {
            rp.RpBeginLine();
            _dp.FormattingNodeForLine(_dp.GetFirstCp() + rp.GetCp(), NULL, pli->_cch, NULL, &ptp, NULL);
        }
    }

    // Get the branch corresponding to the cp hit.
    pNode = ptp->GetBranch();
    
    // If we hit the white space around text, then find the block element
    // that we hit. For example, if we hit in the padding of a block
    // element then it is hit.
    if (bIgnoreBeforeAfter && fPseudoHit)
    {
        CStackDataAry<RECT, 1>  aryRects(Mt(CFlowLayoutBranchFromPointEx_aryRects_pv));
        CMarkup *   pMarkup = GetContentMarkup();
        LONG        cpClipStart;
        LONG        cpClipFinish;
        DWORD       dwFlags = RFE_HITTEST;

        //
        // If a relative node is passed in, the point is already in the
        // co-ordinate system established by the relative element, so
        // pass RFE_IGNORE_RELATIVE to ignore the relative top left when
        // computing the region.
        //
        if (pNodeRelative)
        {
            dwFlags |= RFE_IGNORE_RELATIVE;
            pNode   =  pNodeRelative;
        }

        cpClipStart = cpClipFinish = GetContentFirstCp();
        rp.RpBeginLine();
        cpClipStart += rp.GetCp();
        rp.RpEndLine();
        cpClipFinish += rp.GetCp();

        // walk up the tree and find the block element that we hit.
        while (pNode && !SameScope(pNode, ElementContent()))
        {
            if (!pNodeRelative)
                pNode = pMarkup->SearchBranchForBlockElement(pNode, this);

            if (!pNode)
            {
                // this is bad, somehow, a pNodeRelative was passed in for
                // an element that is not under this flowlayout. How to interpret
                // this? easiest (and safest) is to just bail
                htc = HTC_NO;
                goto Cleanup;
            }

            _dp.RegionFromElement(pNode->Element(), &aryRects, NULL,
                                    NULL, dwFlags, cpClipStart, cpClipFinish);

            if (PointInRectAry(pt, aryRects))
            {
                break;
            }
            else if (pNodeRelative)
            {
                htc = HTC_NO;
            }

            if (pNodeRelative || SameScope(pNode, ElementContent()))
                break;

            pNode = pNode->Parent();
        }

        *pfWantArrow = TRUE;
    }

    Assert(pNode);

    // pNode now points to the element we hit, but it might be
    // hidden.  If it's hidden, we need to walk up the parent
    // chain until we find an ancestor that isn't hidden,
    // or until we hit the layout owner or a relative element
    // (we want testing to stop on relative elements because
    // they exist in a different z-plane).  Note that
    // BranchFromPointEx may be called for hidden elements that
    // are inside a relative element.

    pElementStop = pNodeRelative
                                ? pNodeRelative->Element()
                                : ElementContent();

    while (DifferentScope(pNode, pElementStop))
    {
        pCF = pNode->GetCharFormat();
        if (pCF->IsDisplayNone() || pCF->IsVisibilityHidden())
        {
            fVisible = FALSE;
            pNode = pNode->Parent();
        }
        else
            break;
    }

    Assert(pNode);

    //
    // if we hit the layout element and it is a pseudo hit or
    // if the element hit is not visible then consider it a miss
    //

    // We want to show an arrow if we didn't hit text (fPseudoHit TRUE) OR
    // if we did hit text, but it wasn't visible (as determined by the loop
    // above which set fVisible FALSE).
    if (fPseudoHit || !fVisible)
    {
        // If we walked all the way up to the container, then we want
        // to return HTC_NO so the display tree will call HitTestContent
        // on the container's dispnode (i.e. "the background"), which will
        // return HTC_YES.

        // BUGBUG: relative elements need to be looked at

        // If it's relative, then htc was set earlier, so don't
        // touch it now.
        if ( !pNodeRelative )
            htc = SameScope( pNode, ElementContent() ) ? HTC_NO : HTC_YES;

        *pfWantArrow  = TRUE;
    }
    else
    {
        *pfWantArrow  = !!fPseudoHit;
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
CFlowLayout::GetFirstLayout(DWORD_PTR * pdw, BOOL fBack, BOOL fRaw)
{
    Assert(!fRaw);

    if (ElementOwner()->GetFirstBranch())
    {
        CChildIterator * pLayoutIterator = new
                CChildIterator(
                    ElementOwner(),
                    NULL,
                    CHILDITERATOR_USELAYOUT);
        * pdw = (DWORD_PTR)pLayoutIterator;

        return *pdw == NULL ? NULL : CFlowLayout::GetNextLayout(pdw, fBack, fRaw);
    }
    else
    {
        // If CTxtSite is not in the tree, no need to walk through
        // CChildIterator
        //
        * pdw = 0;
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
CFlowLayout::GetNextLayout(DWORD_PTR * pdw, BOOL fBack, BOOL fRaw)
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
            pLayout = pNode ? pNode->GetUpdatedLayout() : NULL;
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
        DWORD_PTR dw;
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
CFlowLayout::ClearLayoutIterator(DWORD_PTR dw, BOOL fRaw)
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
    DWORD_PTR   dw = 0;
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

                    pLayout = pElement->GetUpdatedLayout();

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

                    pLayoutParent = pElement->GetUpdatedParentLayout();

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
        Doc()->FixZOrder();

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
CFlowLayout::IsElementBlockInContext ( CElement * pElement )
{
    BOOL fRet = FALSE;

    if (pElement == ElementContent())
    {
        fRet = TRUE;
    }
    else if (!pElement->IsBlockElement() && !pElement->IsContainer() )
    {
        fRet = FALSE;
    }
    else if (!pElement->HasLayout())
    {
        fRet = TRUE;
    }
    else
    {
        BOOL fIsContainer = pElement->IsContainer();

        if (!fIsContainer)
        {
            fRet = TRUE;

            //
            // God, I hate this hack ...
            //

            if (pElement->Tag() == ETAG_FIELDSET)
            {
                CTreeNode * pNode = pElement->GetFirstBranch();

                if (pNode)
                {
                    if (pNode->GetCascadeddisplay() != styleDisplayBlock &&
                        !pNode->GetCascadedwidth().IsNullOrEnum()) // IsWidthAuto
                    {
                        fRet = FALSE;
                    }
                }
            }
        }
        else
        {
            //
            // HACK ALERT!
            //
            // For display purposes, contianer elements in their parent context must
            // indicate themselves as block elements.  We do this only for container
            // elements who have been explicity marked as display block.
            //

            if (fIsContainer)
            {
                CTreeNode * pNode = pElement->GetFirstBranch();

                if (pNode && pNode->GetCascadeddisplay() == styleDisplayBlock)
                {
                    fRet = TRUE;
                }
            }
        }
    }

    return fRet;
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

    case 7:
        // dropEffect ALL - do the same thing as 3

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
    CDoc *  pDoc            = Doc();
    DWORD   dwAllowed       = *pdwEffect;
    TCHAR   pszFileType[4]  = _T("");
    CPoint  pt;
    HRESULT hr              = S_OK;

    // Can be null if dragenter was handled by script
    if (!_pDropTargetSelInfo)
    {
        *pdwEffect = DROPEFFECT_NONE ;
        return S_OK;
    }

    //
    // Find out what the effect is and execute it
    // If our operation fails we return DROPEFFECT_NONE
    //
    DragOver(grfKeyState, ptlScreen, pdwEffect);

    IGNORE_HR(DropHelper(ptlScreen, dwAllowed, pdwEffect, pszFileType));


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
    hr = THR( _pDropTargetSelInfo->Drop( this, pDataObj, grfKeyState, pt, pdwEffect ));


Cleanup:
    // Erase any feedback that's showing.
    DragHide();

    Assert(_pDropTargetSelInfo);
    delete _pDropTargetSelInfo;
    _pDropTargetSelInfo = NULL;

    RRETURN1(hr,S_FALSE);

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
    CTreeNode* pNode = NULL;
    
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

    pNode = ElementOwner()->GetFirstBranch();
    if ( ! pNode )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    // Allow only plain text to be pasted in input text controls
    if (!pNode->SupportsHtml()  &&
            pDO->QueryGetData(&g_rgFETC[iAnsiFETC]) != NOERROR)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    {
        hr = THR(CTextXBag::GetDataObjectInfo(pDO, &dwFlags));
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
    _pDropTargetSelInfo = new CDropTargetInfo( this, Doc(), pt );
    if (!_pDropTargetSelInfo)
        RRETURN(E_OUTOFMEMORY);


    return S_OK;

}

//+====================================================================================
//
// Method: DragOver
//
// Synopsis: Delegate to Layout::DragOver - unless we don't have a _pDropTargetSelInfo
//
//------------------------------------------------------------------------------------

HRESULT 
CFlowLayout::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
    // Can be null if dragenter was handled by script
    if (!_pDropTargetSelInfo)
    {
        *pdwEffect = DROPEFFECT_NONE ;
        return S_OK;
    }

    return super::DragOver( grfKeyState, pt, pdwEffect );
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
    _pDropTargetSelInfo->UpdateDragFeedback( this, pt, pDragInfo  );

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

    Assert(!TestLock(CElement::ELEMENTLOCK_RECALC));

    if (!TestLock(CElement::ELEMENTLOCK_SIZING))
    {
#ifdef DEBUG
        // BUGBUG(sujalp): We should never recurse when we are not SIZING.
        // This code to catch the case in which we recurse when we are not
        // SIZING.
        CElement::CLock LockRecalc(ElementOwner(), CElement::ELEMENTLOCK_RECALC);
#endif
        ElementOwner()->SendNotification(NTYPE_ELEMENT_ENSURERECALC);
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
//              [pfAtLogicalBOL] - Is the caret at the logical BOL?
//
//-----------------------------------------------------------------------------
CFlowLayout *
CFlowLayout::GetNextFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, CElement *pElementLayout, LONG *pcp,
                               BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL)
{
    CFlowLayout *pFlowLayout = NULL;   // Stores the new txtsite found in the given dirn.

    Assert(pcp);
    Assert(!pElementLayout || pElementLayout->GetUpdatedParentLayout() == this);

    if (pElementLayout == NULL)
    {
        CLayout *pParentLayout = GetUpdatedParentLayout();
        // By default ask our parent to get the next flowlayout.
        if (pParentLayout)
        {
            pFlowLayout = pParentLayout->GetNextFlowLayout(iDir, ptPosition, ElementOwner(), pcp, pfCaretNotAtBOL, pfAtLogicalBOL);
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
                pFlowLayout = _dp.MoveLineUpOrDown(iDir, rp, ptPosition.x, pcp, pfCaretNotAtBOL, pfAtLogicalBOL);
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
    Assert(pChild && !pChild->NeedsLayout());

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
            CTreePos  * ptpStart;
            CTreePos  * ptpEnd;
            LONG        cpStart;
            ELEMENT_TAG etag = ElementOwner()->Tag();

            pt.x = pt.y = -1;

            // get the extent of this element
            pChild->GetTreeExtent(&ptpStart, &ptpEnd);

            if (!ptpStart || !ptpEnd)
                goto Cleanup;

            cpStart = ptpStart->GetCp();

            {
                CStackDataAry<RECT, 1> aryRects(Mt(CFlowLayoutGetChildElementTopLeft_aryRects_pv));

                _dp.RegionFromElement(pChild, &aryRects, NULL, NULL, RFE_ELEMENT_RECT, cpStart, cpStart);

                if(aryRects.Size())
                {
                    pt.x = aryRects[0].left;
                    pt.y = aryRects[0].top;
                }
            }

            // if we are for a table cell, then we need to adjust for the cell insets,
            // in case the content is vertically aligned.
            if ( (etag == ETAG_TD) || (etag == ETAG_TH) || (etag == ETAG_CAPTION) )
            {
                CDispNode * pDispNode = GetElementDispNode();
                if (pDispNode && pDispNode->HasInset())
                {
                    const CSize & sizeInset = pDispNode->GetInset();
                    pt.x += sizeInset.cx;
                    pt.y += sizeInset.cy;
                }
            }

        }
        break;
    }

Cleanup:
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetPositionInFlowLayout
//
//  Synopsis:   For this flowlayout, find the correct position for the cp to be in
//              within this flowlayout. It may so happen that the ideal position
//              may be within a flowlayout within this one -- handle those cases too.
//
//  Arguments:  [iDir]       -  UP/DOWN/LEFT/RIGHT
//              [ptPosition] -  position in the current txt site
//              [pcp]        -  The cp in the found site where the caret
//                              should be placed.
//              [pfCaretNotAtBOL]: Is the caret at BOL?
//              [pfAtLogicalBOL] : Is the caret at logical BOL?
//
//-----------------------------------------------------------------------------
CFlowLayout *
CFlowLayout::GetPositionInFlowLayout(NAVIGATE_DIRECTION iDir, POINT ptPosition, LONG *pcp,
                                     BOOL *pfCaretNotAtBOL, BOOL *pfAtLogicalBOL)
{
    CFlowLayout  *pFlowLayout = this; // The txtsite we found ... by default us

    Assert(pcp);

    switch(iDir)
    {
    case NAVIGATE_UP:
    case NAVIGATE_DOWN:
    {
        CPoint   ptGlobal(ptPosition);  // The desired position of the caret
        CPoint   ptContent;
        CLinePtr rp(&_dp);         // The line in which the point ptPosition is
        CRect    rcClient;         // Rect used to get the client rect

        // Be sure that the point is within this site's client rect
        RestrictPointToClientRect(&ptGlobal);
        ptContent = ptGlobal;
        TransformPoint(&ptContent, COORDSYS_GLOBAL, COORDSYS_CONTENT);

        // Construct a point within this site's client rect (based on
        // the direction we are traversing.
        GetClientRect(&rcClient);
        rcClient.MoveTo(ptContent);

        // Find the line within this txt site where we want to be placed.
        rp = _dp.LineFromPos(rcClient, (CDisplay::LFP_ZORDERSEARCH   |
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
            pFlowLayout = _dp.NavigateToLine(iDir, rp, ptGlobal, pcp, pfCaretNotAtBOL, pfAtLogicalBOL);
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
    BOOL fEditable = ElementOwner()->IsEditable();
    CElement* pElement = pMessage->pNodeHit->Element();
    CDoc* pDoc = Doc();
    BOOL fOverEditableElement = ( fEditable &&
                                  pElement && 
                                  ( pDoc->IsElementSiteSelectable( pElement ) ||
                                       ( pDoc->IsTablePart( pElement ) &&   // this is for mouse over borders of site-selected table-cells
                                         pDoc->IsPointInSelection( pt ) && 
                                         pDoc->GetSelectionType() == SELECTION_TYPE_Control )));

    // BUGBUG (MohanB) A hack to fix IE5 #60103; should be cleaned up in IE6.
    // MUSTFIX: We should set the default cursor (I-Beam for text, Arrow for the rest) only
    // after the message bubbles through all elements upto the root. This allows elements
    // (like anchor) which like to set non-default cursors over their content to do so.

    if (!fEditable && Tag() == ETAG_GENERIC)
        return S_FALSE;

    if ( ! fOverEditableElement )
        GetClientRect(&rc);
        
    Assert(pMessage->IsContentPointValid());
    if ( fOverEditableElement || PtInRect(&rc, pMessage->ptContent) )
    {
        if (fIsOverEmptyRegion)
        {
            idcNew = IDC_ARROW;
        }
        else
        {
            if ( fEditable  &&
                 ( pMessage->htc >= HTC_TOPBORDER || pMessage->htc == HTC_EDGE ) )
            {
                idcNew = pDoc->GetCursorForHTC( pMessage->htc );
            }        
            else if (! pDoc->IsPointInSelection( pt ) )
            {
                // If CDoc is a HTML dialog, do not show IBeam cursor.

                if ( fEditable
                    || !( pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_DIALOG)
                    ||   _fAllowSelectionInDialog
                    )
                {
                    //
                    // Adjust for Slave to make currency checkwork
                    //
                    if ( pElement && pElement->_etag == ETAG_TXTSLAVE )
                        pElement = pElement->MarkupMaster();

                    if ( pElement &&
                         pDoc->_fDesignMode &&  // BUGBUG: #19326 (KTam) in future we want to replace this with check for parent editability (e.g. HTMLAREA)
                         pDoc->IsElementSiteSelectable( pElement ) && 
                         pElement != pDoc->_pElemCurrent )
                    {
                        idcNew = IDC_SIZEALL;
                    } 
                    else
                        idcNew = IDC_IBEAM;                
                }                    
            }  
            else if ( pDoc->GetSelectionType() == SELECTION_TYPE_Control )
            {
                //
                // We are in a selection. But the Adorners didn't set the HTC_CODE.
                // Set the caret to the size all - to indicate they can click down and drag.
                //
                // This is a little ambiguous - they can click in and UI activate to type
                // but they can also start a move we decided on the below.
                //
                idcNew = IDC_SIZEALL;  
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
    if (    SameScope(ElementOwner(), pMessage->pNodeHit->GetUpdatedNearestLayoutNode())
        &&  (HTC_YES == pMessage->htc) )
        return FALSE; // cursor will be set to text caret later
    else
        return super::OnSetCursor(pMessage);
}


void
CFlowLayout::ResetMinMax()
{
    _fMinMaxValid      = FALSE;
    _dp._fMinMaxCalced = FALSE;
    MarkHasAlignedLayouts(FALSE);
}


LONG
CFlowLayout::GetMaxLineWidth()
{
    return _dp.GetMaxPixelWidth() - _dp.GetCaret();
}


HRESULT BUGCALL
CFlowLayout::HandleMessage(
    CMessage * pMessage)
{
    HRESULT    hr    = S_FALSE;
    CDoc     * pDoc  = Doc();

    // BUGBUG: This must go into TLS!! (brendand)
    //
    static BOOL     g_fAfterDoubleClick = FALSE;

    BOOL        fLbuttonDown;
    BOOL        fInBrowseMode = !ElementOwner()->IsEditable();
    CDispNode * pDispNode     = GetElementDispNode();
    BOOL        fIsScroller   = (pDispNode && pDispNode->IsScroller());

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
        hr = HandleScrollbarMessage(pMessage, ElementOwner());
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

        // BUGBUG (MohanB) Not sure if it's okay to nuke this pElem check
       )
    {
        CTreeNode * pNode;
        HTC         htc;

        pMessage->resultsHitTest._fWantArrow = FALSE;
        pMessage->resultsHitTest._fRightOfCp = FALSE;

        htc = BranchFromPoint(HT_DONTIGNOREBEFOREAFTER,
                            pMessage->ptContent,
                            &pNode,
                            &pMessage->resultsHitTest);

        if (HTC_YES == htc && pNode)
        {
            pMessage->SetNodeHit(pNode);
        }
    }

    switch(pMessage->message)
    {
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        g_fAfterDoubleClick = TRUE;
        hr = HandleButtonDblClk(pMessage);
        break;

    case WM_MBUTTONDOWN:
#ifdef UNIX
        if (!fInBrowseMode && pMessage->pNodeHit->Element()->IsEditable())
        {
            // Perform a middle button paste
            VARIANTARG varTextHandle;
            V_VT(&varTextHandle) = VT_EMPTY;
            g_uxQuickCopyBuffer.GetTextSelection(NULL, FALSE, &varTextHandle);
            
            if (V_VT(&varTextHandle) == VT_EMPTY )
            {
                hr = S_OK; // NO data
            }
            else
            {
                //Convert to V_BSTR
                HGLOBAL hUnicode = TextHGlobalAtoW(V_BYREF(&varTextHandle));
                TCHAR *pszText = (LPTSTR)GlobalLock(hUnicode);
                if (pszText)
                {
                    VARIANTARG varBSTR;
                    TCHAR* pT = pszText;
                    
                    while (*pT) // Filter reserved chars
                    {
                        if (!IsValidWideChar(*pT))
                            *pT = _T('?');
                        pT++;
                    }
                    V_VT(&varBSTR) = VT_BSTR;
                    V_BSTR(&varBSTR) = SysAllocString(pszText);
                    hr = THR(pDoc->Exec((GUID*)&CGID_MSHTML, IDM_PASTE, IDM_PASTESPECIAL, &varBSTR, NULL));
                    SysFreeString(V_BSTR(&varBSTR));
                }
                GlobalUnlock(hUnicode);
                GlobalFree(hUnicode);
            }
            goto Cleanup;
        }
#endif
        if (    Tag() == ETAG_BODY
            &&  !Doc()->GetRootDoc()->_fDisableReaderMode
            &&  pDispNode
            &&  pDispNode->IsScroller())
        {
            ExecReaderMode(pMessage, TRUE);
            hr = S_OK;
        }
        break;

#ifdef UNIX
    case WM_GETTEXTPRIMARY:
        hr = S_OK;

        // Give Mainwin the data currently selected so it can
        // provide it to X for a middle button paste.
       
        // Or, give MSHTML a chance to update selected data, this comes from
        // Trident DoSelection done.
        if (pMessage->lParam == IDM_CLEARSELECTION) 
        {
            CMessage theMessage;
            SelectionMessageToCMessage((SelectionMessage *)pMessage->wParam, &theMessage);
            {
                CDoc *pSelDoc = theMessage.pNodeHit->Element()->Doc(); 
                g_uxQuickCopyBuffer.NewTextSelection(pSelDoc); 
            }
            goto Cleanup;
        }
        
        // Retrieve Text to caller.
        {
            // Check to see if we're selecting a password, in which case
            // don't copy it.
            // 
            CElement *pEmOld = pDoc->_pElemCurrent;
            if (pEmOld && pEmOld->Tag() == ETAG_INPUT &&
                (DYNCAST(CInput, pEmOld))->GetType() == htmlInputPassword)
            {
                *(HANDLE *)pMessage->lParam = (HANDLE) NULL;
                goto Cleanup;
            }

            VARIANTARG varTextHandle;
            V_VT(&varTextHandle) = VT_EMPTY;
            
            hr = pDoc->Exec((GUID*)&CGID_MSHTML, IDM_COPY, 0, NULL, &varTextHandle);
            if (hr == S_OK && V_ISBYREF(&varTextHandle))
            {
                *(HANDLE *)pMessage->lParam = (HANDLE)V_BYREF(&varTextHandle);
            }
            else
            {
                *(HANDLE *)pMessage->lParam = (HANDLE) NULL;
                hr = E_FAIL;
            }
        }
        break;

    case WM_UNDOPRIMARYSELECTION:
        // Under Motif only one app can have a selection active
        // at a time. Clear selection here.
        {
            CMarkup *pMarkup = pDoc->GetCurrentMarkup();
            if (pMarkup) // Clear selection 
                pMarkup->ClearSegments(TRUE);
            g_uxQuickCopyBuffer.ClearSelection();
        }
        break;
#endif

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
        switch(pDoc->GetSelectionType())
        {
        case SELECTION_TYPE_Control:
            // Pass on the message to the first element that is selected
            {
                IHTMLElement *  pIElemSelected  = NULL;
                CElement *      pElemSelected   = NULL;

                if (    S_OK != pDoc->GetCurrentMarkup()->GetElementSegment(0, &pIElemSelected)
                    ||  !pIElemSelected
                    ||  S_OK != pIElemSelected->QueryInterface( CLSID_CElement, (void**)&pElemSelected)
                   )
                {
                    AssertSz(FALSE, "Cannot get selected element");
                    hr = S_OK;
                    goto Cleanup;
                }
                // if the site-selected element is different from the owner, pass the message to it
                Assert(pElemSelected);
                if (pElemSelected != ElementOwner())
                {
                    // Call HandleMessage directly because we do not bubbling here
                    hr = pElemSelected->HandleMessage(pMessage);
                }
            }
            break;

        case SELECTION_TYPE_Selection:
            // Display special menu for text selection in browse mode
            // BUGBUG (MohanB) Should show default menu for HTMLAREA?
            if (!IsEditable(TRUE))
            {
                int cx, cy;

                cx = (short)LOWORD(pMessage->lParam);
                cy = (short)HIWORD(pMessage->lParam);

                if (cx == -1 && cy == -1) // SHIFT+F10
                {
                    // Compute position at whcih to display the menu
                    IMarkupPointer *    pIStart     = NULL;
                    IMarkupPointer *    pIEnd       = NULL;
                    CMarkupPointer *    pStart      = NULL;
                    CMarkupPointer *    pEnd        = NULL;

                    if (    S_OK == pDoc->CreateMarkupPointer(&pStart)
                        &&  S_OK == pDoc->CreateMarkupPointer(&pEnd)
                        &&  S_OK == pStart->QueryInterface(IID_IMarkupPointer, (void**)&pIStart)
                        &&  S_OK == pEnd->QueryInterface(IID_IMarkupPointer, (void**)&pIEnd)
                        &&  S_OK == pDoc->GetCurrentMarkup()->MovePointersToSegment(0, pIStart, pIEnd)
                       )
                    {
                        CMarkupPointer * pmpSelMin;
                        POINT            ptSelMin;

                        ReleaseInterface(pIStart);
                        ReleaseInterface(pIEnd);
                        
                        if (OldCompare( pStart, pEnd ) > 0)
                            pmpSelMin = pEnd;
                        else
                            pmpSelMin = pStart;

                        if (GetDisplay()->PointFromTp(
                                pmpSelMin->GetCp(), NULL, FALSE, FALSE, ptSelMin, NULL, TA_BASELINE) != -1)
                        {
                            RECT rcWin;

                            GetWindowRect(pDoc->InPlace()->_hwnd, &rcWin);
                            cx = ptSelMin.x - GetXScroll() + rcWin.left - CX_CONTEXTMENUOFFSET;
                            cy = ptSelMin.y - GetYScroll() + rcWin.top  - CY_CONTEXTMENUOFFSET;
                        }
                        
                        ReleaseInterface(pStart);
                        ReleaseInterface(pEnd);
                    }
                }
                hr = THR(ElementOwner()->OnContextMenu(cx, cy, CONTEXT_MENU_TEXTSELECT));
            }
            break;
        }
        // For other selection types, let super handle the message
        break;

    case WM_SETCURSOR:
        // Are we over empty region?
        //
        hr = THR(HandleSetCursor(
                pMessage,
                pMessage->resultsHitTest._fWantArrow
                              && fInBrowseMode));
        // BUGBUG (MohanB) Not sure if it's okay to nuke this pElem check
        break;

    case WM_SYSKEYDOWN:
        hr = THR(HandleSysKeyDown(pMessage));
        break;
    }

    // Remember to call super
    if (hr == S_FALSE)
    {
        hr = super::HandleMessage(pMessage);
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

    CStackPtrAry<LONG_PTR, 20> _aryYPos;
    CStackPtrAry<LONG_PTR, 20> _aryXWidthSplit;
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
    while (iStart < cRanges && yPosTop > (LONG)_aryYPos[iStart])
        iStart++;

    // If necessary, beginning creates new range.
    if (yPosTop < (LONG)_aryYPos[iStart])
    {
        hr = _aryYPos.Insert(iStart, yPosTop);
        if (hr)
            goto Cleanup;

        hr = _aryXWidthSplit.Insert(iStart, (LONG)_aryXWidthSplit[iStart-1]);
        if (hr)
            goto Cleanup;

        cRanges++;
    }

    // 2. Find end of range.
    iEnd = iStart;

    while (iEnd < cRanges && yPosBottom > (LONG)_aryYPos[iEnd+1])
        iEnd++;

    // If necessary, end creates new range.
    if (iEnd < cRanges && yPosBottom < (LONG)_aryYPos[iEnd+1])
    {
        hr = _aryYPos.Insert(iEnd+1, yPosBottom);
        if (hr)
            goto Cleanup;

        hr = _aryXWidthSplit.Insert(iEnd+1, (LONG)_aryXWidthSplit[iEnd]);
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
            TraceTag((tagPaginate, "%d - %d : %d", iCount, (LONG)_aryYPos[iCount], (LONG)_aryXWidthSplit[iCount]));
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
    long xWidthSplitMin = (LONG)_aryXWidthSplit[iPosMin];

    for (iCount = iPosMin-1 ; iCount > 0 ; iCount--)
    {
        if ((LONG)_aryXWidthSplit[iCount] < xWidthSplitMin)
        {
            xWidthSplitMin = (LONG)_aryXWidthSplit[iCount];
            iPosMin = iCount;
        }
    }

    return (LONG)_aryYPos[iPosMin+1]-1;
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
//              xMaxPageWidthSofar   Width of broken line (max sofar)
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
                      long xMaxPageWidthSofar,
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
    pPP->yPageHeight = 0;
    pPP->xPageWidth = xMaxPageWidthSofar;
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
        Doc()->GetPrimaryElementClient()->GetUpdatedLayout()->GetRect(&rcSite);
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
            CLayout * pLayoutToSplit = GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL, NULL)->GetUpdatedLayout();
            if (pLayoutToSplit && pLayoutToSplit->Tag() == ETAG_TABLE)
            {
                CTableLayout * pTableLayout = DYNCAST(CTableLayout, pLayoutToSplit);

                // Read style that requests repeating of headers and footers.
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

        //
        // First handle explicit page breaks (if we have any).
        //

        if (pPrintDoc->_fHasPageBreaks)
        {
            LONG yLineAlreadyPrinted = 0; // part of the line already printed
            LONG yBreakLayout = 0;
            LONG yBeforeSpace = pli->_yBeforeSpace;  // before space (cleared after first break of this line)
            BOOL fFoundPageBreakInYRange = pli->_fPageBreakBefore, fCheckForAnotherPageBreak = FALSE, fPageBreakAfterAtTopLevel = FALSE;

            do
            {
                if (!fFoundPageBreakInYRange && pli->_fSingleSite && pli->_fHasNestedRunOwner)
                {
                    // Is there a nested pagebreak?
                    CLayout * pLayoutToSplit = GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL, NULL)->GetUpdatedLayout();

                    // If we already had pagebreak on this line, add 1 to avoid retrieving the same break again.
                    if (   pLayoutToSplit
                        && THR(pLayoutToSplit->ContainsPageBreak(
                                (yLineAlreadyPrinted ? (yLineAlreadyPrinted - yBeforeSpace + 1) : 0),
                                yLineAlreadyPrinted + ySpaceLeftOnPage - yBeforeSpace,  // Subtract _yBeforeSpace from first break (39057)
                                &yBreakLayout,
                                &fPageBreakAfterAtTopLevel)) == S_OK )
                    {
                        fFoundPageBreakInYRange = TRUE;

                        // Make break line relative to part of line already printed.
                        // Now add pli->_yBeforeSpace (39057).
                        yBreakLayout = yBreakLayout + yBeforeSpace - yLineAlreadyPrinted;
                    }
                }

                if (   !fFoundPageBreakInYRange
                    && pli->_fPageBreakAfter
                    && ySpaceLeftOnPage >= yLineHeight
                    && yLineHeight > 0 )
                {
                    fPageBreakAfterAtTopLevel = TRUE;
                    fFoundPageBreakInYRange = TRUE;
                }

                fCheckForAnotherPageBreak = FALSE;

                if (fFoundPageBreakInYRange)
                {
                    TraceTag((tagPaginate,  "\t%d page break BEFORE found, breaking", ili));

                    if (fPageBreakAfterAtTopLevel)
                        yBreakLayout = yLineHeight;

                    // Safety condition: If we are not cutting off a slice, break out to avoid infinite loop.
                    if (yLineAlreadyPrinted>0 && yBreakLayout==0)
                    {
                        Assert(!"Just avoided infinite loop.  Should not get here.");
                        break;
                    }

                    PP.yPageHeight += yBreakLayout;

                    hr = THR(AppendNewPage(paryPP,
                                           &PP,
                                           &PPHeaderFooter,
                                           yHeader,
                                           yFooter,
                                           yFullPageHeight,
                                           pli->_xLineWidth,  // Width of split line starts out new page's max width.
                                           &yTotalPrinted,
                                           &ySpaceLeftOnPage,
                                           &yPageHeight,
                                           &fRejectFirstFooter));
                    if (hr)
                        goto Cleanup;

                    yLineHeight -= yBreakLayout;
                    yLineAlreadyPrinted += yBreakLayout;
                    fFoundPageBreakInYRange = FALSE; // Pagebreak processed.
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
            // BUGBUG: This should be moved to beginning of the for-loop.
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
                // Don't waste more than 15% of the page though.

                if (   pli->TryToKeepTogether()
                    && yLineHeight <= yFullPageHeight
                    && ySpaceLeftOnPage <= WASTABLE_SPACE_PER_PAGE * yFullPageHeight / 100)
                {
                    // break right here

                   hr = THR(AppendNewPage(paryPP,
                                          &PP,
                                          &PPHeaderFooter,
                                          yHeader,
                                          yFooter,
                                          yFullPageHeight,
                                          0, // Since we are not breaking a line, the max width is zero sofar.
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
                    pLayoutToSplit = GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL, NULL)->GetUpdatedLayout();

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
                    else if (   pPrintDoc->_fHasPageBreaks && yLineAlreadyPrinted && ySpaceLeftOnPage
                             && pli->_fSingleSite && pli->_fHasNestedRunOwner
                             && GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL, NULL)->GetUpdatedLayout()
                             && THR(GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL, NULL)->GetUpdatedLayout()->ContainsPageBreak(yLineAlreadyPrinted - pli->_yBeforeSpace + 1, yLineAlreadyPrinted + ySpaceLeftOnPage - pli->_yBeforeSpace, &yPageBreak)) == S_OK )
                    {
                        ySubBlock = yPageBreak + pli->_yBeforeSpace - yLineAlreadyPrinted;
                        Assert(ySubBlock > 0);
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
                                           pli->_xLineWidth,  // Width of split line starts out new page's max width.
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
        &&  ((yTotalPrinted + PP.yPageHeight) < (GetContentHeight(FALSE) )))
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
    CDispNode * pDispNode = GetElementDispNode();
    LONG        ili, iliCount = GetDisplay()->LineCount();
    long        cp = GetDisplay()->GetFirstCp();
    LONG        yHeight;
    CRect       rcClient;
    HRESULT     hr = S_OK;

    if (!pDispNode)
        goto Cleanup;

    {
        CRect rcBounds;
        CSize sizeOffset;

        // Provide small disincentive for cutting across flowlayout in case there is cutting space
        // elsewhere.  This prevents that we cut table rows when there is a gap available.
        pDispNode->GetBounds(&rcBounds);
        if (rcBounds.bottom > rcBounds.top)
        {
            pDispNode->GetTransformOffset(&sizeOffset, COORDSYS_PARENT, COORDSYS_GLOBAL);
            rcBounds.OffsetRect(sizeOffset.cx, sizeOffset.cy);
            paryValues->Insert(rcBounds.top, rcBounds.bottom-1, 3);
        }

        // Obtain global client rect.
        pDispNode->GetClientRect(&rcClient, CLIENTRECT_CONTENT);
        pDispNode->GetTransformOffset(&sizeOffset, COORDSYS_CONTENT, COORDSYS_GLOBAL);
        rcClient.OffsetRect(sizeOffset.cx, sizeOffset.cy);
    }

    yHeight = rcClient.top;

    Assert(yStart < yIdeal);

    for (ili = 0; ili < iliCount; ili++)
    {
        CLine * pli = GetDisplay()->Elem(ili);
        LONG yLineHeight = pli->_yHeight;
        BOOL fNotInteresting;

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
                pLayoutToSplit = GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL, NULL)->GetUpdatedNearestLayout();

                if (pLayoutToSplit && pLayoutToSplit->Tag() == ETAG_TABLE)
                {
                    hr = pLayoutToSplit->PageBreak(yStart, yIdeal, paryValues);
                    if (FAILED(hr))
                        goto Cleanup;
                }
            }
            else if (pli->IsClear() && yLineHeight > 2)
            {
                hr = paryValues->Insert(yHeight-yLineHeight+1, yHeight-1, pli->_xLineWidth);
                if (FAILED(hr))
                    goto Cleanup;
            }
        }

        if (yHeight-yLineHeight > yIdeal)
            break;

        Assert(yHeight-yLineHeight <= yIdeal && "loop was not properly terminated!");
        if (yLineHeight > 2)
        {
            hr = paryValues->Insert(yHeight-yLineHeight+1, yHeight-1, pli->_xLineWidth);
            if (hr)
                goto Cleanup;
        }

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

    // If the vertical slice is non-existent (empty), we don't have a pagebreak.  Return S_FALSE.
    if (yTop > yBottom)
        goto Cleanup;

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
            CLayout * pLayoutToSplit = GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL, NULL)->GetUpdatedLayout();
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

        if (pli->_fPageBreakAfter && yHeight >= yTop && yHeight <= yBottom)
            goto FoundBreak;

        if (yHeight >= yTop && yHeight <= yBottom)
        {
            if (pli->_fSingleSite && pli->_fHasNestedRunOwner)
            {
                CLayout * pLayoutToSplit = GetDisplay()->FormattingNodeForLine(cp, NULL, pli->_cch, NULL, NULL, NULL)->GetUpdatedLayout();

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

    Assert(GetElementDispNode());
    Assert(GetElementDispNode()->IsScroller());

    GetElementDispNode()->GetClientRect(&rc, CLIENTRECT_CONTENT);

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

        if ((hbmp = (HBITMAP)GetWindowLongPtr(hwnd, 0)) != NULL &&
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
    CDispNode * pDispNode        = GetElementDispNode();

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
            wc.cbWndExtra    = sizeof(LONG_PTR);
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
        if (hwndOrigin)
        {
            // Shove the bitmap into the first window long so that it can
            // be used for painting the origin bitmap in the window.
            //
            SetWindowLongPtr(hwndOrigin, 0, (LONG_PTR)hbmp);
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
        }

        DeleteObject(hbmp);

        // USER owns the hrgn, it will clean it up
        //
        // DeleteObject(hrgn);
    }
    RemoveProp(hwndInPlace, TEXT("ReaderMode"));

Cleanup:
    pDoc->_pOptionSettings->fSmoothScrolling = !!fOptSmoothScroll;
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
    HRESULT hr          = S_FALSE;
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
        // Do not scroll if vscroll is disallowed. This prevents content of
        // frames with scrolling=no does not get scrolled (IE5 #31515).
        CDispNodeInfo   dni;
        GetDispNodeInfo(&dni);
        if (!dni.IsVScrollbarAllowed())
            goto Cleanup;

        // mousewheel scrolling, allow partial circle scrolling.
        //
        zDelta = (short) HIWORD(pMessage->wParam);

        if (zDelta != 0)
        {
            long uScrollLines = WheelScrollLines();
            LONG yPercent = (uScrollLines >= 0)
                          ? ((-zDelta * PERCENT_PER_LINE * uScrollLines) / WHEEL_DELTA)
                          : ((-zDelta * PERCENT_PER_PAGE * abs(uScrollLines)) / WHEEL_DELTA);

            if (ScrollByPercent(0, yPercent, MAX_SCROLLTIME))
            {
                hr = S_OK;
            }
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

#if DBG == 1
    CMarkup * pMarkup   = GetContentMarkup();
#endif

    // BUGBUG: (anandra) Most of these should be handled as commands
    // not keydowns.  Use ::TranslateAccelerator to perform translation.
    //

    if (_fVertical)
        pMessage->wParam = ConvVKey((WORD)pMessage->wParam);

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
            TestMarkupServices(ElementOwner());
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

#ifdef NEVER
    // BUGBUG (MohanB) If this code is required, use something other than
    // F10, that too in debug build only. Shift+F10 is used by Windows to
    // bring up the context menu.
    else if (   pMessage->wParam == VK_F10
             && !(pMessage->lParam & SYS_PREVKEYSTATE))
    {
        //if (GetKeyState(VK_SHIFT) & 0x8000)
            TestSelectionRenderServices( pMarkup , ElementContent() );
        //else
        //      pMarkup->DumpTree();

        hr = S_OK;
    }
#endif

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

    CHitTestInfo *  phti            = (CHitTestInfo *)pClientData;
    CDispNode    *  pDispElement    = GetElementDispNode();
    BOOL            fHitTestContent = !pDispElement->IsContainer() ||
                                        pDispElement != pDispNode;

    // Skip nested markups if asked to do so
    if (phti->_grfFlags & HT_SKIPSITES)
    {
        CElement *pElement = ElementContent();
        if (   pElement
            && pElement->HasSlaveMarkupPtr()
            && !SameScope(pElement, phti->_pNodeElement)
           )
        {
            phti->_htc = HTC_NO;
            goto Cleanup;
        }
    }
        
    if (fHitTestContent)
    {
        CPeerHolder  *  pPeerHolder  = ElementOwner()->GetRenderPeerHolder();

        if (pPeerHolder &&
            pPeerHolder->TestRenderFlags(BEHAVIORRENDERINFO_HITTESTING |
                                         BEHAVIORRENDERINFO_ABOVECONTENT))
        {
            //
            // delegate hit testing to peer
            //

            HRESULT hr;
            BOOL    fHit;

            // (treat hr error as no hit)

            hr = THR(pPeerHolder->HitTestPoint((POINT*)pptHit, &fHit));
            if (hr == S_OK && fHit)
            {
                phti->_htc = HTC_YES;
                phti->_pNodeElement = ElementContent()->GetFirstBranch();
                goto Finalize;
            }
        }

        //
        //  If allowed, see if a child element is hit
        //  NOTE: Only check content when the hit display node is a content node
        //

        phti->_htc = BranchFromPoint(phti->_grfFlags,
                                     *pptHit,
                                     &phti->_pNodeElement,
                                     phti->_phtr,
                                     TRUE,                  // ignore pseudo hit's
                                     pDispNode);

        // BUGBUG (donmarsh) - BranchFromPoint was written before the Display Tree
        // was introduced, so it might return a child element that already rejected
        // the hit when it was called directly from the Display Tree.  Therefore,
        // if BranchFromPoint returned an element that has its own layout, and if
        // that layout has its own display node, we reject the hit.
        if (    phti->_htc == HTC_YES
            &&  phti->_pNodeElement != NULL
            &&  phti->_pNodeElement->Element() != ElementOwner()
            &&  phti->_pNodeElement->NeedsLayout())
        {
            phti->_htc = HTC_NO;
            phti->_pNodeElement = NULL;
        }

        // If we pseudo-hit a flow-layer dispnode that isn't the "bottom-most"
        // (i.e. "first") flow-layer dispnode for this element, then we pretend
        // we really didn't hit it.  This allows hit testing to "drill through"
        // multiple flow-layer dispnodes in order to support hitting through
        // non-text areas of display nodes generated by -ve margins.
        // Bug #
        else if (   phti->_htc == HTC_YES 
            && phti->_phtr->_fPseudoHit == TRUE
            && pDispNode->GetLayerType() == DISPNODELAYER_FLOW
            && pDispNode->GetPreviousSiblingNode( TRUE ) )
        {
            phti->_htc = HTC_NO;
        }

        if (phti->_htc == HTC_YES)
        {
            goto Finalize;
        }
    }


    //
    // Do not call super if we are hit testing content and the current
    // element is a container. DisplayTree calls back with a HitTest
    // for the background after hittesting the -Z content.
    //
    if (!fHitTestContent || !pDispElement->IsContainer())
    {
        //
        //  If no child and no peer was hit, use default handling
        //

        super::HitTestContent(pptHit, pDispNode, pClientData);
    }
    goto Cleanup; // done

Finalize:

    //
    //  Save the point and CDispNode associated with the hit
    //

    phti->_ptContent    = *pptHit;
    phti->_pDispNode    = pDispNode;

Cleanup:

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
CFlowLayout::NotifyScrollEvent(
    RECT *  prcScroll,
    SIZE *  psizeScrollDelta)
{
// BUGBUG: Add 1st visible cp tracking here....(brendand)?
    super::NotifyScrollEvent(prcScroll, psizeScrollDelta);
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
    if (pDispNode->IsOwned())
    {
        super::DumpDebugInfo(hFile, level, childNumber, pDispNode, cookie);
    }
    else
    {
#if 1
        WriteString(hFile, _T("<br>\r\n"));
        WriteString(hFile, _T("<xmp>"));
        _dp.DumpLineText(hFile);
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
//
//  Returns:    Pointer to the element CDispNode if one exists, NULL otherwise
//
//-----------------------------------------------------------------------------
CDispNode *
CFlowLayout::GetElementDispNode(
    CElement *  pElement) const
{
    return (    !pElement
            ||  pElement == ElementOwner()
                    ? super::GetElementDispNode(pElement)
                    : pElement->IsRelative()
                        ? ((CFlowLayout *)this)->_dp.FindElementDispNode(pElement)
                        : NULL);
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
CFlowLayout::SetElementDispNode(
    CElement *  pElement,
    CDispNode * pDispNode)
{
    if (    !pElement
        ||  pElement == ElementOwner())
    {
        super::SetElementDispNode(pElement, pDispNode);
    }
    else
    {
        Assert(pElement->IsRelative());
        Assert(!pElement->HasLayout());
        Assert(!pElement->HasFilterPtr());

        _dp.SetElementDispNode(pElement, pDispNode);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     CFlowLayout::GetContentSize
//
//  Synopsis:   Return the width/height of the content
//
//  Arguments:  psize - Pointer to CSize
//
//-----------------------------------------------------------------------------

void
CFlowLayout::GetContentSize(
    CSize * psize,
    BOOL    fActualSize) const
{
    if (fActualSize)
    {
        psize->cx = _dp.GetWidth();
        psize->cy = _dp.GetHeight();
    }
    else
    {
        super::GetContentSize(psize, fActualSize);
    }
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
    return S_OK;
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
    // CHROME
    // If we are chrome hosted then we have no HWND.
    // Therefore simply ask our container whether we have focus.
    BOOL    fOnSetFocus = !Doc()->IsChromeHosted() ? (::GetFocus() == Doc()->_pInPlace->_hwnd) : Doc()->GetFocus();
    HRESULT hr          = S_OK;

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

//+----------------------------------------------------------------------------
//
// Function: GetNestedElementCch
//
// Synopsis: Returns the number of characters that correspond to the element
//           under the current layout context
//
//-----------------------------------------------------------------------------
LONG
CFlowLayout::GetNestedElementCch(CElement  *pElement,       // IN:  The nested element
                                 CTreePos **pptpLast,       // OUT: The pos beyond pElement 
                                 LONG       cpLayoutLast,   // IN:  This layout's last cp
                                 CTreePos  *ptpLayoutLast)  // IN:  This layout's last pos
{
    CTreePos * ptpStart;
    CTreePos * ptpLast;
    long       cpElemStart;
    long       cpElemLast;

    if (cpLayoutLast == -1)
        cpLayoutLast = GetContentLastCp();
    Assert(cpLayoutLast == GetContentLastCp());
    if (ptpLayoutLast == NULL)
    {
        ElementContent()->GetTreeExtent(NULL, &ptpLayoutLast);
    }
#if DBG==1
    {
        CTreePos *ptpLastDbg;
        ElementContent()->GetTreeExtent(NULL, &ptpLastDbg);
        Assert(ptpLayoutLast == ptpLastDbg);
    }
#endif
    
    pElement->GetTreeExtent(&ptpStart, &ptpLast);

    cpElemStart = ptpStart->GetCp();
    cpElemLast  = ptpLast->GetCp();

    if(cpElemLast > cpLayoutLast)
    {
        if(pptpLast)
        {
            ptpLast = ptpLayoutLast->PreviousTreePos();
            while(!ptpLast->IsNode())
            {
                Assert(ptpLast->GetCch() == 0);
                ptpLast = ptpLast->PreviousTreePos();
            }
        }

        // for overlapping layout limit the range to
        // parent layout's scope.
        cpElemLast = cpLayoutLast - 1;
    }

    if(pptpLast)
        *pptpLast = ptpLast;

    return(cpElemLast - cpElemStart + 1);
}

void
CFlowLayout::ShowSelected( CTreePos* ptpStart, CTreePos* ptpEnd, BOOL fSelected,  BOOL fLayoutCompletelyEnclosed, BOOL fFireOM )
{
    Assert(ptpStart && ptpEnd && ptpStart->GetMarkup() == ptpStart->GetMarkup());
    CElement* pElement = ElementOwner();


    // If this has a slave markup, but the selection is in the main markup, then
    // select the element (as opposed to part or all of its content)
    if  ( pElement->HasSlaveMarkupPtr()
           &&  ptpStart->GetMarkup() != ElementOwner()->GetSlaveMarkupPtr() )
    {
        SetSelected( fSelected, TRUE );
    }
    else
    {
        if(
#ifdef  NEVER
           ( pElement->_etag == ETAG_HTMLAREA ) ||
#endif
           ( pElement->_etag == ETAG_BUTTON ) ||
           ( pElement->_etag == ETAG_TEXTAREA ) )
        {
            if (( fSelected && fLayoutCompletelyEnclosed ) ||
                ( !fSelected && ! fLayoutCompletelyEnclosed ) )
                SetSelected( fSelected, TRUE );
            else
            {
                _dp.ShowSelected( ptpStart, ptpEnd, fSelected);
            }
        }
        else
            _dp.ShowSelected( ptpStart, ptpEnd, fSelected);
    }      
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
CFlowLayout::RegionFromElement( CElement       * pElement,
                                CDataAry<RECT> * paryRects,
                                RECT           * prcBound,
                                DWORD            dwFlags)
{
    Assert( pElement);
    Assert( paryRects );

    if ( !pElement || !paryRects )
        return;

    // Is the element passed the same element with the owner?
    if ( _pElementOwner == pElement )
    {
        // call CLayout implementation.
        super::RegionFromElement( pElement, paryRects, prcBound, dwFlags);
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
                               dwFlags,           // coord w/ respect to the client rc.
                               -1,                // Get the complete focus
                               -1,                //
                               prcBound);         // bounds of the element!
    }
}

void
CFlowLayout::SizeContentDispNode(
    const SIZE &    size,
    BOOL            fInvalidateAll)
{
    if (!_dp._fHasMultipleTextNodes)
    {
        super::SizeContentDispNode(size, fInvalidateAll);
    }
    else
    {
        CDispNode * pDispNode = GetFirstContentDispNode();

        // we better have a content dispnode
        Assert(pDispNode);

        while (pDispNode)
        {
            if (pDispNode->GetDispClient() == this)
                pDispNode->SetSize(CSize(size.cx, size.cy - (pDispNode->GetPosition()).y), fInvalidateAll);
            pDispNode = pDispNode->GetNextSiblingNode(TRUE);
        }
    }
}
