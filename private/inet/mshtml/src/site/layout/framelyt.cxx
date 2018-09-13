//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       frameset.cxx
//
//  Contents:   Implementation of CFrameSetLayout
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_CGUID_H_
#define X_CGUID_H_
#include <cguid.h>
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_FRAMELYT_HXX_
#define X_FRAMELYT_HXX_
#include "framelyt.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_OPTSHOLD_HXX_
#define X_OPTSHOLD_HXX_
#include "optshold.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx" // needed for EVENTPARAM
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif


#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

enum XorYLoop { XDirection = 0, YDirection = 1, Terminate = 2 };

MtDefine(CFrameSetLayout, Layout, "CFrameSetLayout")
MtDefine(CFrameSetLayout_aryLayouts_pv, CFrameSetLayout, "CFrameSetLayout::__aryLayouts::_pv")
MtDefine(CFrameSetLayout_aryRows_pv, CFrameSetLayout, "CFrameSetLayout::_aryRows::_pv")
MtDefine(CFrameSetLayout_aryCols_pv, CFrameSetLayout, "CFrameSetLayout::_aryCols::_pv")
MtDefine(CFrameSetLayout_aryResizeRecords_pv, CFrameSetLayout, "CFrameSetLayout::_aryResizeRecords::_pv")
MtDefine(CFrameSetLayoutCalcPositions_aryPosX_pv, Locals, "CFrameSetLayout::CalcPositions aryPosX::_pv")
MtDefine(CFrameSetLayoutCalcPositions_aryPosY_pv, Locals, "CFrameSetLayout::CalcPositions aryPosY::_pv")

const CLayout::LAYOUTDESC CFrameSetLayout::s_layoutdesc =
{
    0,          // _dwFlags
};

//+---------------------------------------------------------------------------
//
//  Member:     CFrameSetLayout::CFrameSetLayout, public
//
//  Synopsis:   CFrameSetLayout ctor
//
//----------------------------------------------------------------------------
CFrameSetLayout::CFrameSetLayout (CElement * pElement) :
    CLayout(pElement),
    __aryLayouts(Mt(CFrameSetLayout_aryLayouts_pv)),
    _aryRows(Mt(CFrameSetLayout_aryRows_pv)),
    _aryCols(Mt(CFrameSetLayout_aryCols_pv)),
    _aryResizeRecords(Mt(CFrameSetLayout_aryResizeRecords_pv))
{
};



//+---------------------------------------------------------------------------
//
// Member  : CFrameSetLayout::SetRowsCols
//
// Synopsis: Set _aryRows and _aryCols
//
//----------------------------------------------------------------------------

void
CFrameSetLayout::Reset(BOOL fForce)
{
    super::Reset(fForce);
}



//+---------------------------------------------------------------------------
//
// Member  : CFrameSetLayout::SetRowsCols
//
// Synopsis: Set _aryRows and _aryCols
//
//----------------------------------------------------------------------------
struct MYPROP
{
    PROPERTYDESC  pdesc;
    NUMPROPPARAMS numprop;
};

// Value the unitvalue is given if the string is empty
#define NOT_SET_DEFAULT  0

LPTSTR
FindRowColSeparator(LPTSTR pchStart, const TCHAR * pchSep)
{
    // IE50 Raid 3475
    // re-implement _tcstok to not skipping over leading separators
    //
    LPTSTR  pchNext;
    TCHAR * pchControl;

    for (pchNext = pchStart; * pchNext; pchNext ++)
    {
        for (pchControl = (TCHAR * ) pchSep;
             * pchControl && * pchNext != * pchControl;
             pchControl ++);
        if (* pchControl)
            break;
    }

    if (* pchNext)
    {
        * pchNext = _T('\0');
        return pchNext + 1;
    }
    else
        return NULL;
}

void
CFrameSetLayout::SetRowsCols()
{
    HRESULT                hr;
    CDataAry<CUnitValue> * paryValues;
    CUnitValue             uvValue;
    XorYLoop               dir;
    CStr                   cstr;
    LPCTSTR                pch;
    LPTSTR                 pchThis, pchNext;

    static const TCHAR * s_pszSep = _T(",;"); // Be liberal and allow semicolons
    static MYPROP  s_pdesc = {
                                      { NULL, NULL, 0, NOT_SET_DEFAULT },
                                      {
                                        {
                                           PP_UV_LENGTH_OR_PERCENT |
                                           PROPPARAM_TIMESRELATIVE, 0, 0
                                        },
                                        VT_EMPTY, 0, 0, 0
                                      }
                                    };

    for (dir  = XDirection;
         dir != Terminate;
         dir  = ((dir==XDirection) ? YDirection : Terminate))
    {
        if (dir == XDirection)
        {
            pch = FrameSetElement()->GetAAcols();
            paryValues = &_aryCols;
        }
        else
        {
            pch = FrameSetElement()->GetAArows();
            paryValues = &_aryRows;
        }

        if (!pch)
            continue;

        cstr.Set(pch); // Copy the string because _tcstok modifies it

        pchThis = cstr;

        while (pchThis && * pchThis)
        {
            pchNext = FindRowColSeparator(pchThis, s_pszSep);

            if (!(*pchThis))
            {
                // IE50 Raid 3475 - Treat empty string as "*"
                //
                hr = THR(uvValue.SetValue(100, CUnitValue::UNIT_TIMESRELATIVE));
            }
            else
            {
                hr = THR(uvValue.FromString(pchThis, &s_pdesc.pdesc));

                if (hr)
                {
                    uvValue.SetValue(0, CUnitValue::UNIT_PIXELS);
                }
                else if (uvValue.GetUnitValue() < 0)
                {
                    if (uvValue.GetUnitType() == CUnitValue::UNIT_PERCENT)
                    {
                        // treat negative percentage rows/cols values as "*"
                        //
                        hr = THR(uvValue.SetValue(100,
                                CUnitValue::UNIT_TIMESRELATIVE));
                    }
                    else
                    {
                        uvValue.SetValue(0, CUnitValue::UNIT_PIXELS);
                    }
                }
            }

            paryValues->AppendIndirect(&uvValue);
            pchThis = pchNext;
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CFrameSetLayout::CalcSize
//
//  Synopsis:   Calculate the size of the object
//              (See CSite::CalcSize for details)
//
//--------------------------------------------------------------------------

DWORD
CFrameSetLayout::CalcSize( CCalcInfo * pci,
                           SIZE *      psize,
                           SIZE *      psizeDefault)
{
    CScopeFlag      csfCalcing(this);
    CElement::CLock LockS(FrameSetElement(), CElement::ELEMENTLOCK_SIZING);
    CSaveCalcInfo   sci(pci, this);
    DWORD           grfReturn;

    Assert(pci);
    Assert(psize);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    grfReturn = super::CalcSize(pci, psize);

    if (grfReturn & LAYOUT_THIS)
    {
        Assert(pci->_smMode == SIZEMODE_NATURAL || pci->_smMode == SIZEMODE_SET);
        CalcPositions(pci, *psize);
    }

    return grfReturn;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFrameSetLayout::GetManualResizeTravel
//
//  Synopsis:   Search the manual resize records for one
//              that matches a particular edge
//
//--------------------------------------------------------------------------

int CFrameSetLayout::GetManualResizeTravel(BOOL fVerticalTravel, int iEdge)
{
    int i;
    ManualResizeRecord *pmrr = _aryResizeRecords;
    for (i = _aryResizeRecords.Size();
         i > 0;
         i--, pmrr++)
    {
        if (pmrr->fVerticalTravel == fVerticalTravel &&
            pmrr->iEdge == iEdge)
            return pmrr->iTravel;
    }
    return 0;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFrameSetLayout::CancelManualResizeTravel
//
//  Synopsis:   Eliminate the manual resize record for
//              a particular edge
//
//--------------------------------------------------------------------------

BOOL CFrameSetLayout::CancelManualResize(BOOL fVerticalTravel)
{
    BOOL retVal = FALSE;
    int i;
    ManualResizeRecord *pmrr = _aryResizeRecords;
    for (i = _aryResizeRecords.Size();
         i > 0;
         i--, pmrr++)
    {
        if (pmrr->fVerticalTravel == fVerticalTravel)
        {
            if (pmrr->iTravel != 0)
            {
                retVal = TRUE;
                pmrr->iTravel = 0;
            }
        }
    }
    return retVal;
}

#define MIN_RESIZE_SIZE 10

//+---------------------------------------------------------------------------
//
//  Member:     CFrameSetLayout::CalcPositions, public
//
//  Synopsis:   Calculates the positions of all the embedded frames.
//
//  Arguments:  [pci]       -- Pointer to CCalcInfo
//              [sizeSet]   -- Size of the FRAMESET
//
//----------------------------------------------------------------------------

void
CFrameSetLayout::CalcPositions(
    CCalcInfo * pci,
    SIZE        sizeSet)
{
    CSaveCalcInfo   sci(pci);
    int             iFixedWidth; // Pixels used up by percentage or pixel values
    int             iSizeRel;    // Number of pixels for "relative-sized" values
                                 //  This value is the size of "1*". Also used
                                 //  if the sum of percentages and pixels is greater
                                 //  than the total available width.
    int             iSumPercents = 0;   // sum of pixel values from percent unit values

    XorYLoop        dir;
    int             i=0, j=0;

    CDataAry<CUnitValue> *paryValues; // Ptr to array of values parsed in Init2
    CUnitValue *          puv;        // Ptr to current value
    CDataAry<DWORD>       aryPosX(Mt(CFrameSetLayoutCalcPositions_aryPosX_pv));    // Array of X coordinates for frames
    CDataAry<DWORD>       aryPosY(Mt(CFrameSetLayoutCalcPositions_aryPosY_pv));    // Array of Y coordinates for frames
    CDataAry<DWORD>      *paryPos;    // Pointer to current coordinate array
    DWORD                 pos;        // Current position to be stored in array
    int                   cRel;       // Count of "relative-sized" values
    int                   cStar;      // Count of "*"'s (e.g. "2*" is 2)
    int                   iTotal;     // # of pixels representing 100%
    BOOL                  fAdjust;    // If TRUE, the sum of values is not
                                      //  equal to the total width and we must
                                      //  proportionately size-down or up the
                                      //  values
    int                   iFrameSpacing;  // value of the frameSpacing attribute
    int                   iTopDelta=0;    // amount of travel for the top edge
    int                   iLeftDelta=0;   // amount of travel for the left edge

    SIZE                  origSizeSet = sizeSet;     // original sizeSet
    DWORD_PTR             dw = 0;

    //
    // No need to do anything if we don't have any sites yet.
    //
    if (!ContainsChildLayout())
        return;

    {
        // Reduce the amount of space available to dole out (sizeSet) by our own
        // border size.
        CBorderInfo borderInfo;

        if (FrameSetElement()->GetBorderInfo(pci, &borderInfo, FALSE))
        {
            sizeSet.cx -= borderInfo.aiWidths[BORDER_RIGHT] + borderInfo.aiWidths[BORDER_LEFT];
            sizeSet.cy -= borderInfo.aiWidths[BORDER_TOP] + borderInfo.aiWidths[BORDER_BOTTOM];
        }

        // Now reduce the sizeSet by the amount of space to be taken up by
        // the framespacing.  This will cause the contained sites to be
        // positioned incorrectly (but with the right size!) -- this will get
        // fixed up when we actually tell the sites where they're positioned.
        iFrameSpacing = FrameSetElement()->GetFrameSpacing();
        if (iFrameSpacing > min(sizeSet.cx, sizeSet.cy))
           iFrameSpacing = min(sizeSet.cx, sizeSet.cy);

        if (_aryCols.Size() > 1)
            sizeSet.cx -= iFrameSpacing * (_aryCols.Size() - 1);
        if (_aryRows.Size() > 1)
            sizeSet.cy -= iFrameSpacing * (_aryRows.Size() - 1);
    }

    {
        // Now adjust the sizeSet by the amount of travel of the
        // top/left/bottom/right edges.  This amount will be reversed
        // later, after the distribution of space to the contained sites.
        // This can only happen when the frameset is nested within another
        // frameset which has moved one of our edges.

        int j;
        ManualResizeRecord *pmrr = _aryResizeRecords;
        for (j = _aryResizeRecords.Size();
             j > 0;
             j--, pmrr++)
        {
            if (pmrr->iEdge == 0 )
            {
                if (pmrr->fVerticalTravel)
                {
                    iTopDelta = pmrr->iTravel;
                    sizeSet.cy += iTopDelta;
                }
                else
                {
                    iLeftDelta = pmrr->iTravel;
                    sizeSet.cx += iLeftDelta;
                }
            }
            else if (pmrr->iEdge == _iNumActualRows && pmrr->fVerticalTravel)
                sizeSet.cy -= pmrr->iTravel;
            else if (pmrr->iEdge == _iNumActualCols && !pmrr->fVerticalTravel)
                sizeSet.cx -= pmrr->iTravel;
        }
    }

    for (dir  = XDirection;
         dir != Terminate;
         dir  = ((dir==XDirection) ? YDirection : Terminate))
    {
        if (dir == XDirection)
        {
            paryValues = &_aryCols;
            paryPos    = &aryPosX;
            iTotal     = sizeSet.cx;
            pos        = 0;
        }
        else
        {
            paryValues = &_aryRows;
            paryPos    = &aryPosY;
            iTotal     = sizeSet.cy;
            pos        = 0;
        }

        iFixedWidth = 0;
        cRel        = 0;
        cStar       = 0;

        // Store the starting x or y position as the first element in the
        //  coordinate array.
        paryPos->AppendIndirect(&pos);

        // No need to do a bunch of calculations if there's 0 or 1 values
        if (paryValues->Size() <= 1)
        {
            iTotal += pos;
            paryPos->AppendIndirect((DWORD*)&iTotal);
            continue;
        }

        //
        // Compute the amount of space taken up by "fixed-sized" values
        // (percent or pixel values), and also the number of "relative-sized"
        // values that we must divide the remaining space between.
        //
        for (i=paryValues->Size(), puv = *paryValues;
             i > 0;
             i--, puv++)
        {
            if (puv->GetUnitType() == CUnitValue::UNIT_TIMESRELATIVE)
            {
                cStar += puv->GetTimesRelative();
                cRel++;
            }
            else
            {
                long iPixel = puv->GetPixelValue(
                                        pci,
                                        ((dir == XDirection)
                                            ? CUnitValue::DIRECTION_CX
                                            : CUnitValue::DIRECTION_CY),
                                        iTotal);
                iFixedWidth += iPixel;
                if (puv->GetUnitType() == CUnitValue::UNIT_PERCENT)
                {
                    iSumPercents += iPixel;
                }
            }
        }

        fAdjust = FALSE;

        if (cStar && (iFixedWidth < iTotal))
        {
            // We have relative-sized values, plus some space left over to give
            // them. Compute the size of "1*".
            //
            iSizeRel = (iTotal - iFixedWidth) / cStar;
        }
        else if (iFixedWidth != iTotal && iTotal >= 0)
        {
            // One of the following is true:
            //    - The sum of all "fixed-width" (percents and pixels) values
            //      is greater than our total space. Any relative-sized values
            //      in this case are given a size of zero.
            //    - The sum of all "fixed-width" values is less than our
            //      total available space and there are no "relative-sized"
            //      values.
            //
            // For both these cases, we must shrink or enlarge all regions by
            // a fixed amount to make them fit. iSizeRel is negative if
            // we're shrinking, and positive if we're enlarging.
            //
            Assert(cRel < paryValues->Size());

            iSizeRel = (iTotal - iFixedWidth) / (paryValues->Size() - cRel);
            fAdjust  = TRUE;
        }
        else
        {
            // The total of "fixed-width" values is exactly equal to the
            // window width. Any "relative-sized" values are given a size of
            // zero.
            //
            iSizeRel = 0;
        }

        //
        // Compute actual pixel positions and store them into the aryPos array
        //
        for (j=paryValues->Size(), puv = *paryValues;
             j > 0;
             j--, puv++)
        {
            if (puv->GetUnitType() == CUnitValue::UNIT_TIMESRELATIVE)
            {
                if (!fAdjust)
                {
                    pos += puv->GetPixelValue(
                                     pci,
                                     ((dir == XDirection)
                                       ? CUnitValue::DIRECTION_CX
                                       : CUnitValue::DIRECTION_CY),
                                     iSizeRel);
                }
                // else preserve the previous value of "pos", giving this
                // relative-sized frame a dimension of zero.
            }
            else
            {
                int iPixelValue = puv->GetPixelValue(pci,
                                          ((dir == XDirection)
                                            ? CUnitValue::DIRECTION_CX
                                            : CUnitValue::DIRECTION_CY),
                                          iTotal);
                pos += iPixelValue;
                if (fAdjust)
                {
                    if (puv->GetUnitType() == CUnitValue::UNIT_PERCENT ||
                        iSumPercents == 0)
                    {
                        // iRemainder is the total amount that needs
                        // to be adjusted (added or deleted)
                        long iRemainder = iTotal - iFixedWidth;

                        // our percentage is computed as follows:
                        // if there are > 0 "percentage" frames
                        //     our size divided by the sum of all percentage frames
                        // else
                        //     our size divided by the sum of all fixed width frames

                        long iAdjustment;

                        if (iSumPercents > 0)
                            iAdjustment = (iRemainder * iPixelValue) / iSumPercents;
                        else if (iFixedWidth > 0)
                            iAdjustment = (iRemainder * iPixelValue) / iFixedWidth;
                        else
                        {
                            // We haven't dolled anything out because there
                            // aren't any times-relative or fixed width.
                            // So, let's pretend that each of the "zeros"
                            // are divided evenly.
                            iAdjustment = iTotal / paryValues->Size();
                        }

                        pos += iAdjustment;
                    }
                }
            }

            paryPos->AppendIndirect(&pos);
        }
    }

    i = j = 0;

    //
    // Tweak the far-right and bottom pixel values to account for rounding
    // error by just setting them to the right and bottom of our rect.
    aryPosX[aryPosX.Size()-1] = sizeSet.cx;
    aryPosY[aryPosY.Size()-1] = sizeSet.cy;

    CDispNode * pDispNodeSibling = NULL;
    CDispNode * pDispNode;
    CPoint      pt;
    SIZE        size;

    i = -1;

    // Iterate through all contained sites and re-size all (if this site re-sized)
    // or those which requested re-sizing (themselves or a descendent)

    for (CLayout * pLayout = GetFirstLayout(&dw, FALSE), *pPrevLayout = NULL;
        pLayout;
        pPrevLayout = pLayout, pLayout = GetNextLayout(&dw, FALSE))
    {
        pLayout->ElementOwner()->DirtyLayout(pci->_grfLayout);

        if (pLayout->ElementOwner()->_etag != ETAG_FRAME &&
            pLayout->ElementOwner()->_etag != ETAG_FRAMESET)
        {
            pLayout->SetYProposed(0);
            pLayout->SetXProposed(0);
            continue;
        }

        i++;
        if (i >= aryPosX.Size() - 1)
        {
            j++;
            i = 0;
        }

        if (j >= aryPosY.Size()-1)
        {
            // There's more <FRAME> tags than they gave values for.
            // Put it at (0,0) with zero width and height.
            pLayout->SetYProposed(0);
            pLayout->SetXProposed(0);
            size = g_Zero.size;
            pt   = g_Zero.pt;
        }
        else
        {
            Assert(i < aryPosX.Size()-1);

            pLayout->SetXProposed(aryPosX[i]);
            pLayout->SetYProposed(aryPosY[j]);
            size.cx = aryPosX[i+1] - pLayout->GetXProposed();
            size.cy = aryPosY[j+1] - pLayout->GetYProposed();


            // adjusted the origin for pLayout based on the amount
            // we reduced sizeSet above to compensate for frameSpacing
            pLayout->SetXProposed(pLayout->GetXProposed() + (i * iFrameSpacing));
            pLayout->SetYProposed(pLayout->GetYProposed() + (j * iFrameSpacing));
            pt.x = aryPosX[i] + (i * iFrameSpacing);
            pt.y = aryPosY[j] + (j * iFrameSpacing);
            {
                // Adjust the proposed pt and size for the current
                // size based on the manual resize records.  This
                // adjustment corresponds to the adjustment to the
                // sizeSet made up top.

                int iTravel;
                iTravel = GetManualResizeTravel(FALSE, i);
                if (iTravel != 0)
                {
                    // moving the left edge
                    pLayout->SetXProposed(pLayout->GetXProposed() + iTravel);
                    pt.x    += iTravel;
                    size.cx -= iTravel;
                }

                iTravel = GetManualResizeTravel(FALSE, i+1);
                if (iTravel != 0)
                {
                    // moving the right edge
                    size.cx += iTravel;
                }

                iTravel = GetManualResizeTravel(TRUE, j);
                if (iTravel != 0)
                {
                    // moving the top edge
                    pLayout->SetYProposed(pLayout->GetYProposed() + iTravel);
                    pt.y    += iTravel;
                    size.cy -= iTravel;
                }

                iTravel = GetManualResizeTravel(TRUE, j+1);
                if (iTravel != 0)
                {
                    // moving the bottom edge
                    size.cy += iTravel;
                }
                // if the top/left edge moved then we need
                // to shift things over by the amount they
                // moved.  this isn't a problem if the right/bottom
                // edges moved
                pLayout->SetYProposed(pLayout->GetYProposed() - iTopDelta);
                pLayout->SetXProposed(pLayout->GetXProposed() - iLeftDelta);
                pt.x -= iLeftDelta;
                pt.y -= iTopDelta;
                if (size.cx < 0 || size.cy < 0)
                {
                    // Check to see if we went negative.  This can
                    // happen if the user resized a frame to be small
                    // and then made the window smaller.
                    // For now just toss the resize info and recurse.
                    // It's possible we could do better.

                    if ( CancelManualResize(FALSE) || // cancelling horizontal
                         CancelManualResize(TRUE) )   // cancelling vertical
                    {
                        CalcPositions(pci, origSizeSet);
                        goto Cleanup;
                    }
#if !defined(WIN16) && !defined(WINCE) && DBG == 1
                    else
                    {
                        TCHAR buffer[100];
                        if (GetEnvironmentVariable(_T("NegativeFrameSizeDebug"),
                                                   buffer,
                                                   sizeof(buffer)/sizeof(buffer[0])))
                            DebugBreak();
                        TraceTag((tagWarning,
                                 "computed negative size for frame"));
                    }
#endif // !defined(WIN16) && !defined(WINCE)
                }
            }
        }

        pci->_smMode = (pLayout->_fSizeThis
                            ? SIZEMODE_SET
                            : SIZEMODE_NATURAL);
        pLayout->CalcSize(pci, &size);
        pLayout->SetPosition(pt);

        pDispNode = pLayout->GetElementDispNode();

        if (pDispNode)
        {
            pDispNode->ExtractFromTree();

            if (pDispNodeSibling)
            {
                pDispNodeSibling->InsertNextSiblingNode(pDispNode);
            }
            else
            {
                DYNCAST(CDispContainer, GetElementDispNode())->InsertFirstChildInFlow(pDispNode);
            }

            pDispNodeSibling = pDispNode;
        }
    }

    _iNumActualRows = j+1;
    _iNumActualCols = i+1;

Cleanup:
    ClearLayoutIterator(dw, FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFrameSetLayout::GetElementsInZOrder, public
//
//  Synopsis:   Gets a complete list of elements which we are responsible for
//              (for things like painting and hit-testing), with the list
//              sorted by ZOrder (index 0 = bottom-most element).
//
//  Arguments:  [paryElements] -- Array to fill with elements.
//              [prcDraw]      -- Rectangle which sites must intersect with to
//                                be included in this list. Can be NULL.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CFrameSetLayout::GetElementsInZOrder(CPtrAry<CElement *> *paryElements,
                                   CElement            *pElementThis,
                                   RECT                *prcDraw,
                                   HRGN                 hrgn,
                                   BOOL                 fIncludeNotVisible)
{
    Assert(pElementThis == FrameSetElement());

    DWORD_PTR dw;
    CLayout *pLayout;

    for (pLayout = GetFirstLayout(&dw); pLayout; pLayout = GetNextLayout(&dw))
    {
        paryElements->Append(pLayout->ElementOwner());
    }
    ClearLayoutIterator(dw, FALSE);

    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSetLayout::MaxTravelForEdge
//
//  Synopsis:   Determine the maximum travel for a resize of a particular
//              edge.  This requires us to look at the contained sites and,
//              if a contained site is a frameset we must recurse.
//
//-------------------------------------------------------------------------

BOOL CFrameSetLayout::MaxTravelForEdge(int iEdge, BOOL fVerticalTravel,
                                     int *pMinTravel, int *pMaxTravel)
{
    int iMaxTravel = INT_MAX;
    int iMinTravel = INT_MIN;

    int iHorz    = 0;
    int iVert    = 0;

    DWORD_PTR dw;

    for (CLayout *pLayout = GetFirstLayout(&dw, FALSE);
         pLayout;
         pLayout = GetNextLayout(&dw, FALSE))
    {
        CFrameSetLayout *pfss;
        CSize           size;

        pLayout->GetSize(&size);

        switch (pLayout->ElementOwner()->_etag)
        {
        case ETAG_FRAMESET:
            pfss = DYNCAST(CFrameSetLayout, pLayout);
            break;

        case ETAG_FRAME:
            pfss = NULL;
            break;

        default:
            continue;
        }

        if (fVerticalTravel)
        {
            if (iVert == iEdge-1)
            {
                // motion limited upward by pLayout
                if (pfss)
                {
                    int iMin, iMax;
                    if (!pfss->MaxTravelForEdge(pfss->_iNumActualRows,
                                                fVerticalTravel,
                                                &iMin,
                                                &iMax))
                        goto Retfalse;
                    iMinTravel = max(iMinTravel, iMin);
                }
                else
                {
                    if (DYNCAST(CFrameSite, pLayout->ElementOwner())->NoResize())
                        goto Retfalse;
                    iMinTravel = max(iMinTravel,
                                     min(0, (INT) -size.cy + MIN_RESIZE_SIZE));
                }
            }
            else if (iVert == iEdge)
            {
                // motion limited downward by pLayout
                if (pfss)
                {
                    int iMin, iMax;
                    if (!pfss->MaxTravelForEdge(0, fVerticalTravel,
                                           &iMin, &iMax))
                        goto Retfalse;
                    iMaxTravel = min(iMaxTravel, iMax);
                }
                else
                {
                    if (DYNCAST(CFrameSite, pLayout->ElementOwner())->NoResize())
                        goto Retfalse;
                    iMaxTravel = min(iMaxTravel,
                                     max(0, (INT) size.cy - MIN_RESIZE_SIZE));
                }
            }
        }
        else
        {
            if (iHorz == iEdge-1)
            {
                // motion limited to the left by pLayout
                if (pfss)
                {
                    int iMin, iMax;
                    if (!pfss->MaxTravelForEdge(pfss->_iNumActualCols,
                                           fVerticalTravel,
                                           &iMin,
                                           &iMax))
                        goto Retfalse;
                    iMinTravel = max(iMinTravel, iMin);
                }
                else
                {
                    if (DYNCAST(CFrameSite, pLayout->ElementOwner())->NoResize())
                        goto Retfalse;
                    iMinTravel = max(iMinTravel,
                                     min(0, (INT) -size.cx + MIN_RESIZE_SIZE));
                }
            }
            else if (iHorz == iEdge)
            {
                // motion limited to the right by pLayout
                if (pfss)
                {
                    int iMin, iMax;
                    if (!pfss->MaxTravelForEdge(0, fVerticalTravel,
                                           &iMin, &iMax))
                        goto Retfalse;
                    iMaxTravel = min(iMaxTravel, iMax);
                }
                else
                {
                    if (DYNCAST(CFrameSite, pLayout->ElementOwner())->NoResize())
                        goto Retfalse;
                    iMaxTravel = min(iMaxTravel,
                                     max(0, (INT) size.cx - MIN_RESIZE_SIZE));
                }
            }
        }
        if (++iHorz == _iNumActualCols)
        {
            iHorz = 0;
            iVert++;
        }
    }
    ClearLayoutIterator(dw, FALSE);
    if (pMaxTravel)
        *pMaxTravel = iMaxTravel;
    if (pMinTravel)
        *pMinTravel = iMinTravel;
    return TRUE;
    
Retfalse:
    ClearLayoutIterator(dw, FALSE);
    return FALSE;
}


//+------------------------------------------------------------------------
//
//  Member:     CFrameSetLayout::GetResizeInfo
//
//  Synopsis:   Record information about where the user clicked so that
//              subsequent mouse moves can do the appropriate resizing
//
//-------------------------------------------------------------------------

BOOL CFrameSetLayout::GetResizeInfo(POINT pt, FrameResizeInfo *pFRI)
{
    DWORD_PTR   dw;
    BOOL        retVal   = FALSE;
    CLayout    *pPrev    = NULL;
    CLayout    *pLayout  = GetFirstLayout(&dw, FALSE);
    int         iHorz    = 0;
    int         iVert    = 0;

    pFRI->vertical.iEdge = pFRI->horizontal.iEdge = -1;

    while (pLayout) {

        if (pLayout->ElementOwner()->_etag != ETAG_FRAMESET &&
            pLayout->ElementOwner()->_etag != ETAG_FRAME)
            goto next;

        if (pPrev)
        {
            CRect   rcPrev;
            CRect   rcCurr;

            pPrev->GetRect(&rcPrev, COORDSYS_GLOBAL);
            pLayout->GetRect(&rcCurr, COORDSYS_GLOBAL);
            InflateRect(&rcPrev, -2, -2);
            InflateRect(&rcCurr, -2, -2);

            //BUGBUG (lmollico): Is -2 the size of the border?

            if (rcPrev.top == rcCurr.top)
            {
                // checking horizinally at this point
                iHorz++;
                if (pFRI->horizontal.iEdge == -1) // once is enough
                {
                    if (pt.x >= rcPrev.right && pt.x <= rcCurr.left)
                    {
                        if (MaxTravelForEdge(iHorz,
                                             FALSE,
                                             &pFRI->horizontal.iMinTravel,
                                             &pFRI->horizontal.iMaxTravel))
                        {
                            pFRI->horizontal.iEdge = iHorz;
                            pFRI->horizontal.rcEdge.top    = rcPrev.top;
                            pFRI->horizontal.rcEdge.left   = rcPrev.right;
                            pFRI->horizontal.rcEdge.right  = rcCurr.left;
                            pFRI->horizontal.rcEdge.bottom = rcCurr.bottom;
                          //  InflateRect(&pFRI->horizontal.rcEdge, -2, 0);
                            retVal = TRUE;
                        }
                    }
                }
            }
            else
            {
                // checking vertically at this point
                iHorz = 0;
                iVert++;
                if (pFRI->vertical.iEdge == -1) // once is enough
                {
                    if (pt.y >= rcPrev.bottom && pt.y <= rcCurr.top)
                    {
                        if (MaxTravelForEdge(iVert,
                                             TRUE,
                                             &pFRI->vertical.iMinTravel,
                                             &pFRI->vertical.iMaxTravel))
                        {
                            pFRI->vertical.iEdge = iVert;
                            pFRI->vertical.rcEdge.top    = rcPrev.bottom;
                            pFRI->vertical.rcEdge.left   = rcPrev.left;
                            pFRI->vertical.rcEdge.right  = rcCurr.right;
                            pFRI->vertical.rcEdge.bottom = rcCurr.top;
                            //InflateRect(&pFRI->vertical.rcEdge, 0, -2);
                            retVal = TRUE;
                        }
                    }
                }
            }
//#define MULTI_DIRECTIONAL_RESIZING
#ifndef MULTI_DIRECTIONAL_RESIZING
            if (retVal)
                goto Cleanup;
#endif
        }
        pPrev = pLayout;

    next:

        pLayout = GetNextLayout(&dw, FALSE);

    }
Cleanup:
    ClearLayoutIterator(dw, FALSE);
    return retVal;
}

//+--------------------------------------------------------------
//
//  Member:     CFrameSetLayout::Resize
//
//  Synopsis:   Resizes the contained sites based on a
//              ManualResizeRecord
//
//---------------------------------------------------------------

void CFrameSetLayout::Resize(const ManualResizeRecord *pmr)
{
    // Process, as follows:
    // (1) if we've moved this edge before, then merge this into the previous
    // (2) record this ManualResize object to be used in CalcPositions
    // (3) determine whether the edge in question borders another FrameSet
    //     and if so, propagate this change down to it
    // (4) in CalcPositions()
    //    a. take the SizeSet and adjust it according to those edge
    //       MR's which effect the left/right/top/bottom edges only
    //    b. proceed as normal, allocating the space to the children
    //    c. process the recorded MR's as follows:
    //         abuts the edge being processed.  note that this may not
    //       - adjust the size/position of all children whose rect
    //         be totally possible if things have shrunk due to the window
    //         having been made smaller (what to do?)

    ManualResizeRecord mr = *pmr;
    if (mr.iEdge == -1)
        mr.iEdge = mr.fVerticalTravel ? _iNumActualRows : _iNumActualCols;

    int i = _aryResizeRecords.Size();
    ManualResizeRecord *pMRList = _aryResizeRecords;
    while (--i >= 0)
    {
        if (pMRList->iEdge == mr.iEdge && pMRList->fVerticalTravel == mr.fVerticalTravel)
        {
            pMRList->iTravel += mr.iTravel;
            break;
        }
        pMRList++;
    }
    if (i == -1)
    {
        _aryResizeRecords.AppendIndirect(&mr);
    }

    DWORD_PTR dw;
    CLayout *pLayout = GetFirstLayout(&dw, FALSE);
    int iHorz = 0;
    int iVert = 0;

    while (pLayout)
    {
        CFrameSetLayout *pfss;

        switch (pLayout->ElementOwner()->_etag)
        {
        case ETAG_FRAMESET:
            pfss = DYNCAST(CFrameSetLayout, pLayout);
            break;

        case ETAG_FRAME:
            pfss = NULL;
            break;

        default:
            goto next;
        }

        if (pfss)
        {
            BOOL fDoit = FALSE;
            ManualResizeRecord mrTmp;
            if ( (mr.iEdge == iVert &&  mr.fVerticalTravel) ||
                 (mr.iEdge == iHorz && !mr.fVerticalTravel) )
            {
                mrTmp = mr;
                mrTmp.iEdge = 0;
                fDoit = TRUE;
            }
            else if ( (mr.iEdge-1 == iVert &&  mr.fVerticalTravel) ||
                      (mr.iEdge-1 == iHorz && !mr.fVerticalTravel) )
            {
                mrTmp = mr;
                mrTmp.iEdge = -1;
                fDoit = TRUE;
            }
            if (fDoit)
            {
                pfss->Resize(&mrTmp);  // recurse
            }
        }
        if (++iHorz == _iNumActualCols)
        {
            iHorz = 0;
            iVert++;
        }

    next:

        pLayout = GetNextLayout(&dw, FALSE);
    }

    ClearLayoutIterator(dw, FALSE);

    ElementOwner()->RemeasureElement();
}


//+--------------------------------------------------------------
//
//  Function:   ResizeWndProc
//
//  Synopsis:   Window proc for the window which is displayed
//              during frame resizing
//
//---------------------------------------------------------------

LRESULT CALLBACK
ResizeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_PAINT:
        {
            HBRUSH hbr = (HBRUSH)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            FillRect(ps.hdc, &ps.rcPaint, hbr);
            EndPaint(hwnd, &ps);
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//+--------------------------------------------------------------
//
//  Function:   CreateDitherBrush
//
//  Synopsis:   Used for drawing the resize bar
//
//  Notes:      stolen from ie3 which stole it from comctl132
//
//+--------------------------------------------------------------

static HBRUSH CreateDitherBrush(void)
{
    HBRUSH  hbrDitherBrush = NULL;
    HBITMAP hbmTemp;
    static WORD graybits[] = {0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555};

    // build the dither brush.  this is a fixed 8x8 bitmap
    hbmTemp = CreateBitmap(8, 8, 1, 1, graybits);
    if (hbmTemp)
    {
        // now use the bitmap for what it was really intended...
        hbrDitherBrush = CreatePatternBrush(hbmTemp);
        DeleteObject(hbmTemp);
    }

    return( hbrDitherBrush );
}

//+--------------------------------------------------------------
//
//  Member:     CFrameSetLayout::HandleMessage
//
//  Synopsis:   Handle the resizing of frames.  Otherwise, pass
//              it to super.
//
//  Notes:      This method uses a number of static variables
//              making it non-thread-safe.  Presumably this won't
//              be an issue since there's only one mouse.
//
//---------------------------------------------------------------

HRESULT BUGCALL
CFrameSetLayout::HandleMessage(CMessage * pMessage)
{
    static FrameResizeInfo s_fri;
    static BOOL s_fResizing = FALSE;
    static POINT s_ptMouseDown;
    static POINT s_ptPrevious;
    static POINT s_ptOrigin;
    static SIZE  s_size;
    static HBRUSH s_hbrOld;
    static HDC s_hdcScreen;
    HRESULT  hr = S_FALSE;

    BOOL fDynamicResize = FALSE;
#ifndef WINCE
    SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &fDynamicResize, 0);
#endif // WINCE

    switch (pMessage->message)
    {
        case WM_LBUTTONDOWN:
            if (GetResizeInfo(pMessage->pt, &s_fri))
            {
                FrameSetElement()->TakeCapture(TRUE);
                s_fResizing = TRUE;
                // hang on to this so we know
                // how far we've moved
                s_ptPrevious = s_ptMouseDown = pMessage->pt;

                if (!fDynamicResize)
                {
                    // compute the position, in screen coordinates, of the
                    // resize bar.  then draw it.  subsequent 'move' events
                    // will cause the previous location to be inverted
                    // and the new location to be redrawn (inverted)

                    RECT rc = s_fri.vertical.iEdge == -1 ? s_fri.horizontal.rcEdge
                                                     : s_fri.vertical  .rcEdge;
                    s_ptOrigin.x = rc.left;
                    s_ptOrigin.y = rc.top;
                    s_size.cx  = rc.right - rc.left;
                    s_size.cy  = rc.bottom - rc.top;

                    ClientToScreen(Doc()->_pInPlace->_hwnd, &s_ptOrigin);
                    // BUGBUG (mwagner) remove this call to GetDC(NULL) and
                    // replace it with a call to GetDCEx() with the appropriate
                    // parameters.
                    s_hdcScreen = ::GetDC(NULL);
                    s_hbrOld = (HBRUSH)SelectObject(s_hdcScreen, CreateDitherBrush());
                    PatBlt(s_hdcScreen, s_ptOrigin.x,
                                    s_ptOrigin.y,
                                    s_size.cx,
                                    s_size.cy,
                                    PATINVERT);
                }
            }
            hr = S_OK;
            break;

        case WM_LBUTTONUP:
            if (s_fResizing)
            {
                if (!fDynamicResize)
                {
                    // undo the last inversion
                    PatBlt(s_hdcScreen, s_ptOrigin.x,
                           s_ptOrigin.y,
                           s_size.cx,
                           s_size.cy,
                           PATINVERT);

                    // put back the old brush and delete the current,
                    // temporary, one
                    DeleteObject(SelectObject(s_hdcScreen, s_hbrOld));

                    if (s_fri.vertical.iEdge != -1)
                    {
                        FrameResizeInfo1 *frr1 = &s_fri.vertical;

                        int iTravel = pMessage->pt.y - s_ptMouseDown.y;
                        iTravel = min(iTravel, frr1->iMaxTravel);
                        iTravel = max(iTravel, frr1->iMinTravel);

                        ManualResizeRecord mrr = { (SHORT)iTravel, (SHORT)frr1->iEdge, TRUE };
                        Resize(&mrr);
                    }
                    if (s_fri.horizontal.iEdge != -1)
                    {
                        FrameResizeInfo1 *frr1 = &s_fri.horizontal;

                        int iTravel = pMessage->pt.x - s_ptMouseDown.x;
                        iTravel = min(iTravel, frr1->iMaxTravel);
                        iTravel = max(iTravel, frr1->iMinTravel);

                        ManualResizeRecord mrr = { (SHORT)iTravel, (SHORT)frr1->iEdge, FALSE };
                        Resize(&mrr);
                    }
                }
                FrameSetElement()->TakeCapture(FALSE);
                s_fResizing = FALSE;
            }
            hr = S_OK;
            break;

        case WM_SETCURSOR:
            if (!s_fResizing)
            {
                if (GetResizeInfo(pMessage->pt, &s_fri))
                {
                    LPCTSTR idcResize = NULL;
                    if (   s_fri.vertical.iEdge != -1
                        && s_fri.horizontal.iEdge != -1)
                        idcResize = IDC_SIZEALL;
                    else if ( s_fri.vertical.iEdge != -1 )
                        idcResize = IDC_SIZENS;
                    else if ( s_fri.horizontal.iEdge != -1 )
                        idcResize = IDC_SIZEWE;

                    FrameSetElement()->SetCursorStyle(idcResize);
                }
                else
                {
                    FrameSetElement()->SetCursorStyle(IDC_ARROW);
                }
            }
            hr = S_OK;
            break;

#ifndef WIN16
        case WM_MOUSEWHEEL:
            {
                CLayout *   pLayout;
                DWORD_PTR   dw;
                CDoc *      pDoc    = Doc();

                // NEWTREE  MonsterWalk should build these site arrays
                //          with branches instead of sites

                for (pLayout = GetFirstLayout(&dw, FALSE); pLayout;
                                pLayout = GetNextLayout(&dw, FALSE))
                {
                    // Two cases:
                    //
                    // 1) _pElemCurrent is _pSiteRoot or the current frameset site,
                    //    in this case, need to check all child sites, be it
                    //    CFrameSetSite or CFrameElement.
                    // 2) _pElemCurrent is one of the child site, HandleMessage is
                    //    called up through the parent chain from _pElemCurrent, no
                    //    need to pass down to _pElemCurrent again to avoid
                    //    potential infinite call chain.
                    //
                    if (pLayout->ElementOwner() != pDoc->_pElemCurrent)
                    {
                        // Call HandleMessage directly because we do not bubbling here
                        hr = THR(pLayout->ElementOwner()->HandleMessage(pMessage));
                        if (hr != S_FALSE)
                            break;
                    }
                }
                ClearLayoutIterator(dw, FALSE);

                // BUGBUG (MohanB) Need to stop bubbling?
            }
            break;
#endif // ndef WIN16

        case WM_MOUSEMOVE:
            if (s_fResizing)
            {
                // the following code causes the screen resize/update to
                // occur during the mouse drag operation rather than at
                // the mouse up.

                if (!fDynamicResize)
                    PatBlt(s_hdcScreen, s_ptOrigin.x,
                                        s_ptOrigin.y,
                                        s_size.cx,
                                        s_size.cy,
                                        PATINVERT);

                POINT pt = pMessage->pt;
                if (s_fri.vertical.iEdge != -1)
                {
                    FrameResizeInfo1 *frr1 = &s_fri.vertical;

                    pt.y = min((LONG)pt.y, (LONG)(s_ptMouseDown.y + frr1->iMaxTravel));
                    pt.y = max((LONG)pt.y, (LONG)(s_ptMouseDown.y + frr1->iMinTravel));

                    s_ptOrigin.y += pt.y - s_ptPrevious.y;

                    if (fDynamicResize)
                    {
                        ManualResizeRecord mrr = { (SHORT)(pt.y - s_ptPrevious.y),
                                                   (SHORT)(frr1->iEdge),
                                                   TRUE };
                        Resize(&mrr);
                    }
                }
                if (s_fri.horizontal.iEdge != -1)
                {
                    FrameResizeInfo1 *frr1 = &s_fri.horizontal;

                    pt.x = min((LONG)pt.x, (LONG)(s_ptMouseDown.x + frr1->iMaxTravel));
                    pt.x = max((LONG)pt.x, (LONG)(s_ptMouseDown.x + frr1->iMinTravel));

                    s_ptOrigin.x += pt.x - s_ptPrevious.x;

                    if (fDynamicResize)
                    {
                        ManualResizeRecord mrr = { (SHORT)(pt.x - s_ptPrevious.x),
                                                   (SHORT)(frr1->iEdge),
                                                   FALSE };
                        Resize(&mrr);
                    }
                }
                s_ptPrevious = pt;

                if (!fDynamicResize)
                {
                    PatBlt(s_hdcScreen, s_ptOrigin.x,
                                        s_ptOrigin.y,
                                        s_size.cx,
                                        s_size.cy,
                                        PATINVERT);
                }
                hr = S_OK;
            }
            break;

    }

    if (hr == S_FALSE)
    {
        hr = THR(super::HandleMessage(pMessage));
    }

    RRETURN1(hr, S_FALSE);
}

int CFrameSetLayout::iPixelFrameHighlightWidth = 0;
int CFrameSetLayout::iPixelFrameHighlightBuffer = 0;

//+----------------------------------------------------------------------------
//
// Member: CheckFrame3DBorder
//
// Synopsis: Based on current 3D border setting b3DBorder, determine which
//           extra 3D border edges are needed for pDoc, which can be a CDoc
//           inside one of the CFrameElement in this CFrameSetLayout
//
//-----------------------------------------------------------------------------
BOOL
CFrameSetLayout::CheckFrame3DBorder(CDoc * pDoc, BYTE b3DBorder)
{
    int  cRows   = max(_aryRows.Size(), 1);
    int  cCols   = max(_aryCols.Size(), 1);
    int  iRow    = 0;
    int  iCol    = 0;
    BOOL fResult = FALSE;

    DWORD_PTR         dw;
    BYTE              b3DBorderCurrent;
    CLayout       *   pLayout;
    CFrameElement *   pFrame;
    CDoc          *   pDocCurrent;

    CDocInfo          DCIFrameSet(FrameSetElement());
    CBorderInfo       borderinfo;
    int               iBorderWidthMin;

    // need to exclude those 3D border drawn by outmost CFrameSetLayout
    //
    iBorderWidthMin = ((Doc() == Doc()->GetRootDoc())
        && (FrameSetElement() == Doc()->GetPrimaryElementClient()))
                    ? (2) : (0);

    // if there is extra border space (top/left/bottom/right) needed for
    // CFrameSetLayout, need to draw 3D border edge for them
    //
    FrameSetElement()->GetBorderInfo(&DCIFrameSet, &borderinfo, FALSE);
    if ((borderinfo.rcSpace.top + borderinfo.aiWidths[BORDER_TOP])
            > iBorderWidthMin)
    {
        b3DBorder |= NEED3DBORDER_TOP;
    }
    if ((borderinfo.rcSpace.left + borderinfo.aiWidths[BORDER_LEFT])
            > iBorderWidthMin)
    {
        b3DBorder |= NEED3DBORDER_LEFT;
    }
    if ((borderinfo.rcSpace.bottom + borderinfo.aiWidths[BORDER_BOTTOM])
            > iBorderWidthMin)
    {
        b3DBorder |= NEED3DBORDER_BOTTOM;
    }
    if ((borderinfo.rcSpace.right + borderinfo.aiWidths[BORDER_RIGHT])
            > iBorderWidthMin)
    {
        b3DBorder |= NEED3DBORDER_RIGHT;
    }

    // loop through all CFrameElement and CFrameSetLayout inside current
    // CFrameSetLayout until we find pDoc
    //
    for (pLayout = GetFirstLayout(&dw, FALSE);
            pLayout && !fResult;
            pLayout = GetNextLayout(&dw, FALSE))
    {
        ELEMENT_TAG eTag = (ELEMENT_TAG) pLayout->ElementOwner()->_etag;

        pDocCurrent = NULL;

        switch (eTag)
        {
        case ETAG_FRAME:
            pFrame = DYNCAST(CFrameElement, pLayout->ElementOwner());
            break;
        case ETAG_FRAMESET:
            pFrame = NULL;
            break;
        default:
            continue;
        }

        if (pFrame)
        {
            pFrame->GetCDoc(&pDocCurrent);
        }

        if (eTag == ETAG_FRAMESET || pDoc == pDocCurrent)
        {
            b3DBorderCurrent = b3DBorder;

            // If we are not the first row and there are more than one rows,
            // we should need to draw top 3D border edge
            //
            if ((iRow != 0) && (cRows > 1))
                b3DBorderCurrent |= NEED3DBORDER_TOP;

            // If we are not the last row and there are more than one rows,
            // we should need to draw bottom 3D border edge
            //
            if ((iRow != cRows - 1) && (cRows > 1))
                b3DBorderCurrent |= NEED3DBORDER_BOTTOM;

            // If we are not the first column and there are more than one
            // columns, we should need to draw left 3D border edge
            //
            if ((iCol != 0) && (cCols > 1))
                b3DBorderCurrent |= NEED3DBORDER_LEFT;

            // If we are not the first column and there are more than one
            // columns, we should need to draw right 3D border edge
            //
            if ((iCol != cCols - 1) && (cCols > 1))
                b3DBorderCurrent |= NEED3DBORDER_RIGHT;

            if (eTag == ETAG_FRAMESET)
            {
                // propogate down CFrameSetLayout to see if pDoc is inside there
                //
                fResult = DYNCAST(CFrameSetLayout, pLayout)->CheckFrame3DBorder(pDoc, b3DBorderCurrent);
            }
            else
            {
                CDocInfo DCIFrame(pFrame);

                // if there is extra border space (top/left/bottom/right)
                // needed for CFrameElement, need to draw 3D border edge then
                //
                memset(&borderinfo, 0, sizeof(borderinfo));
                pFrame->GetBorderInfo(&DCIFrame, &borderinfo, FALSE);
                if ((borderinfo.rcSpace.top +
                        borderinfo.aiWidths[BORDER_TOP]) > 0)
                {
                    b3DBorderCurrent |= NEED3DBORDER_TOP;
                }
                if ((borderinfo.rcSpace.left +
                        borderinfo.aiWidths[BORDER_LEFT]) > 0)
                {
                    b3DBorderCurrent |= NEED3DBORDER_LEFT;
                }
                if ((borderinfo.rcSpace.bottom +
                        borderinfo.aiWidths[BORDER_BOTTOM]) > 0)
                {
                    b3DBorderCurrent |= NEED3DBORDER_BOTTOM;
                }
                if ((borderinfo.rcSpace.right +
                        borderinfo.aiWidths[BORDER_RIGHT]) > 0)
                {
                    b3DBorderCurrent |= NEED3DBORDER_RIGHT;
                }

                pDoc->_b3DBorder = b3DBorderCurrent;
                fResult          = TRUE;
            }
        }

        iCol ++;
        if (iCol == cCols)
        {
            iCol = 0;
            iRow ++;
        }
    }
    ClearLayoutIterator(dw, FALSE);

    return fResult;
}

//+------------------------------------------------------------------------
//
//  Member:     CFrameSetSite::GetBackgroundInfo
//
//  Synopsis:   Generate the background information for a framesite
//
//-------------------------------------------------------------------------
BOOL
CFrameSetLayout::GetBackgroundInfo(
    CFormDrawInfo *     pDI,
    BACKGROUNDINFO *    pbginfo,
    BOOL                fAll,
    BOOL                fRightToLeft )
{
    Assert(pDI || !fAll);

    CColorValue ccv = FrameSetElement()->BorderColorAttribute();
    COLORREF    cr;
    CLayout   * pLayout;
    DWORD_PTR   dw;
    CColorValue ccvFrame;
    CDoc      * pDoc = Doc();

    // BUGBUG (t-johnh): This isn't going to give us the exact behavior we
    // want.  There are some issues with merging frame border colors, that
    // Netscape does and we don't.  Additionally, checks need to be done
    // for existence/color on a border-by-border basis for existence/color,
    // because we can have a case where one frameset has several non-
    // intersecting borders of different colors, widths, and existence.
    //
    // First, do we have a border or not?  If we don't have a border,
    // we shouldn't look for bordercolor attrs on <FRAME>s.
    //
    if (!pDoc->_fFrameBorderCacheValid)
    {
        CElement * pElemClient = pDoc->GetPrimaryElementClient();
        Assert(pElemClient->Tag() == ETAG_FRAMESET);
        DYNCAST(CFrameSetSite, pElemClient)->FrameBorderAttribute(TRUE, FALSE);
        pDoc->_fFrameBorderCacheValid = TRUE;
    }

    if(!FrameSetElement()->_fFrameBorder)
    {
        cr = pDoc->PrimaryRoot()->GetBackgroundColor();
    }
    else
    {
        if(ccv.IsDefined())
        {
            // First, if they gave us a border color, let's use it.
            //
            cr = ccv.GetColorRef();
        }
        else
        {
            // Otherwise, use the 3d button face color so we
            // match the window frame.
            //
            cr = GetSysColorQuick(COLOR_3DFACE);
        }

        // test if one of the frames has borderColor defined. If it is the case,
        // set borderColor to the first defined and use for all borders.
        //
        for (pLayout = GetFirstLayout(&dw, FALSE);
             pLayout;
             pLayout = GetNextLayout(&dw, FALSE))
        {
            if (pLayout->ElementOwner()->_etag == ETAG_FRAME)
            {
                ccvFrame = (DYNCAST(CFrameElement, pLayout->ElementOwner()))->GetAAborderColor();
                if (ccvFrame.IsDefined())
                {
                    cr = ccvFrame.GetColorRef();
                    break;
                }
            }
        }
        ClearLayoutIterator(dw, FALSE);
    }

    super::GetBackgroundInfo(pDI, pbginfo, fAll, fRightToLeft);
    pbginfo->crBack = cr;

    return TRUE;
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
CFrameSetLayout::GetFirstLayout(DWORD_PTR * pdw, BOOL fBack, BOOL fRaw)
{
    if (ElementOwner()->GetFirstBranch())
    {
        CChildIterator *    pLayoutIterator;
        static ELEMENT_TAG   atagStop = ETAG_FRAMESET;
        static ELEMENT_TAG   atagChild[2] = { ETAG_FRAMESET, ETAG_FRAME };
        
        pLayoutIterator = new CChildIterator(
            ElementOwner(),
            NULL,
            CHILDITERATOR_USETAGS,
            &atagStop, 1,
            atagChild, ARRAY_SIZE(atagChild));
        
        *pdw = (DWORD_PTR)pLayoutIterator;
        
        return *pdw == NULL ? NULL : CFrameSetLayout::GetNextLayout(pdw, fBack, fRaw);
    }
    
    // if CFrameSetSite is not in the tree, no need to walk through
    // CChildIterator
    //
    * pdw = 0;
    return NULL;
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
CFrameSetLayout::GetNextLayout ( DWORD_PTR * pdw, BOOL fBack, BOOL fRaw )
{
    CLayout *           pLayout = NULL;
    CChildIterator *    pLayoutWalker;

    pLayoutWalker = (CChildIterator *) (*pdw);
    if (pLayoutWalker)
    {
        CTreeNode * pNode = fBack ? pLayoutWalker->PreviousChild()
                                : pLayoutWalker->NextChild();
        pLayout = pNode ? pNode->GetUpdatedLayout() : NULL;
    }
    return pLayout;
}

//+---------------------------------------------------------------------------
//
// Member:      ClearLayoutIterator
//
//----------------------------------------------------------------------------

void
CFrameSetLayout::ClearLayoutIterator(DWORD_PTR dw, BOOL fRaw)
{
    Assert(!fRaw);

    CChildIterator * pLayoutWalker = (CChildIterator *) dw;
    if (pLayoutWalker)
        delete pLayoutWalker;
}

//+---------------------------------------------------------------------------
//
// Member:      ContainsChildLayout
//
//----------------------------------------------------------------------------
BOOL
CFrameSetLayout::ContainsChildLayout(BOOL fRaw)
{
    DWORD_PTR dw;
    CLayout * pLayout = GetFirstLayout(&dw, FALSE, fRaw);
    ClearLayoutIterator(dw, fRaw);
    return pLayout ? TRUE : FALSE;
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
CFrameSetLayout::SetZOrder(CLayout * pLayout, LAYOUT_ZORDER zorder, BOOL fUpdate)
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

//+------------------------------------------------------------------------
//
//  Member:     CFrameSetLayout::SelectSite
//
//  Synopsis:   Selects the site based on the flags passed in
//
//
//  Arguments:  [pLayout]       -- The layout to select (for parent layouts)
//              [dwFlags]       -- Action flags:
//                  SS_ADDTOSELECTION       add it to the selection
//                  SS_REMOVEFROMSELECTION  remove it from selection
//                  SS_KEEPOLDSELECTION     keep old selection
//                  SS_SETSELECTIONSTATE    set flag according to state
//
//  Returns:    HRESULT
//
//  Notes:      This method will call parent objects or children objects
//              depending on the action and passes the child/parent along
//
//-------------------------------------------------------------------------
HRESULT
CFrameSetLayout::SelectSite(CLayout * pLayout, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    AssertSz(0,"This method doesn't do anything anymore - and will be deleted soon");
    
#ifdef NEVER
    if (pLayout == this && (dwFlags & SS_SETSELECTIONSTATE))
    {
        if (dwFlags & SS_CLEARSELECTION)
        {
            SetSelected(FALSE);
        }
        else if (!ElementOwner()->IsEditable() && GetParentLayout()
                                          && ElementOwner()->IsInMarkup())
        {
            Verify(!GetParentLayout()->SelectSite(this, dwFlags));
        }

        // call state on all children
        {
            DWORD_PTR dw;
            CLayout * pLayout;

            for (pLayout = GetFirstLayout(&dw); pLayout; pLayout = GetNextLayout(&dw))
            {
                Verify(!pLayout->SelectSite(pLayout, dwFlags));
            }
            ClearLayoutIterator(dw, FALSE);
        }
    }
    else
    {
        hr = super::SelectSite(pLayout, dwFlags);
    }
#endif    
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     DoLayout
//
//  Synopsis:   Initiate a re-layout of the table
//
//  Arguments:  grfFlags - LAYOUT_xxxx flags
//
//-----------------------------------------------------------------------------
void
CFrameSetLayout::DoLayout(
    DWORD   grfLayout)
{
    Assert(grfLayout & (LAYOUT_MEASURE | LAYOUT_POSITION | LAYOUT_ADORNERS));

    //
    //  If the element is not hidden, layout its content
    //

    if (!IsDisplayNone())
    {
        CElement::CLock Lock(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
        CCalcInfo       CI(this);
        CSize           size;

        GetSize(&size);

        CI._grfLayout |= grfLayout;

        //
        //  Layout child FRAMESETs and FRAMEs
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

            CalcPositions(&CI, size);

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
CFrameSetLayout::Notify(
    CNotification * pnf)
{
    Assert(!pnf->IsReceived(_snLast));

    //
    //  Ignore already handled notifications
    //

    if (pnf->IsHandled())
        goto Cleanup;

    Assert(!pnf->Handler());

    super::Notify(pnf);
    switch (pnf->Type())
    {
    case NTYPE_ELEMENT_RESIZE:
    case NTYPE_ELEMENT_REMEASURE:
        //
        //  Always "dirty" the layout associated with the element
        //  if the element is not itself.
        //
        if (pnf->Element() != ElementOwner())
        {
            pnf->Element()->DirtyLayout(pnf->LayoutFlags());
        }

        //
        //  Ignore the notification if already "dirty"
        //  Otherwise, post a layout request
        //

        if (    !_fSizeThis
            &&  !TestLock(CElement::ELEMENTLOCK_SIZING))
        {
            PostLayoutRequest(pnf->LayoutFlags() | LAYOUT_MEASURE);
        }
        break;

    case NTYPE_SELECT_CHANGE:
        // Fire this onto the form
        Doc()->OnSelectChange();
        break;
    }
    
Cleanup:
    //
    //  Handle the notification
    //

    pnf->SetHandler(ElementOwner());

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


