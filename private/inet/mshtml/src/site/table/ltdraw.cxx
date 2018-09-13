//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltdraw.cxx
//
//  Contents:   CTableLayout drawing methods.
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif


MtDefine(CTableLayoutDrawSiteList_aryElements_pv, Locals, "CTableLayout::DrawSiteList aryElements::_pv")
MtDefine(CTableLayout_pBorderInfoCellDefault, CTableLayout, "CTableLayout::_pBorderInfoCellDefault")
MtDefine(CTableLayout_pTableBorderRenderer, CTableLayout, "CTableLayout::_pTableBorderRenderer")

extern WORD s_awEdgesFromTableFrame[htmlFrameborder+1];
extern WORD s_awEdgesFromTableRules[htmlRulesall+1];

ExternTag(tagTableRecalc);
DeclareTag(tagNoExcludeClip, "Tables", "Don't exclude cliprects in GetCellBorderInfo")
DeclareTag(tagNoExcludeClipCorners, "Tables", "Don't exclude corner cliprects in GetCellBorderInfo")
DeclareTag(tagNoInflateRect, "Tables", "Don't inflate invalid rect in collapsed tables")
DeclareTag(tagNoBorderInfoCache, "Tables", "Don't use the tablewide borderinfo cache")
DeclareTag(tagDontComeOnIn, "Tables", "Rule out no-border cells for corner rendering")
DeclareTag(tagClipInsetRect, "Tables", "Clip corner out from inset rect")
DeclareTag(tagNoCollapsedBorders, "Tables", "Disable rendering of collapsed borders")
DeclareTag(tagRedCollapsedBorders, "Tables", "Render collapsed borders in red")


//+--------------------------------------------------------------------------------------
//
// Drawing methods overriding CLayout
//
//---------------------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CTableCell::GetCellBorderInfoDefault
//
//  Synopsis:   Retrieves a single table cell's borderinfo.
//
//  Arguments:  pdci [in]         Docinfo
//              pborderinfo [out] Pointer to borderinfo structure to be filled
//              fRender [in]      Is this borderinfo needed for rendering or
//                                layout? (render retrieves more info, e.g. colors)
//
//  Returns:    TRUE if the cell has at least one border.  FALSE otherwise.
//
//  Note:       This routine makes use of a tablewide cell-border default cache
//              to retrieve normal cell borderinfos faster (memcpy).  If the cache
//              doesn't exist, it creates it as a side-effect once we encounter
//              the first cell with default border settings.
//
//              We retrieve border settings by first inheriting defaults from the
//              table, and then relying on CElement::GetBorderInfo() to override
//              the defaults.
//
//-------------------------------------------------------------------------

DWORD
CTableCellLayout::GetCellBorderInfoDefault(
    CDocInfo *      pdci,
    CBorderInfo *   pborderinfo,
    BOOL            fRender,
    CTable *        pTable,
    CTableLayout *  pTableLayout)
{
    CTableCell *    pCell        = TableCell();
    BOOL            fNotCaption  = !IsCaption(pCell->Tag());
    CTreeNode *     pCellNode    = pCell->GetFirstBranch();
    BOOL            fOverrideTablewideBorderSettings = pCellNode->GetCascadedborderOverride();

    BOOL            fUseBorderCache = !fOverrideTablewideBorderSettings   WHEN_DBG( && !IsTagEnabled(tagNoBorderInfoCache) )
                                    && pTableLayout 
                                    && (pTableLayout->CanRecalc() && pTableLayout->_sizeMax.cx != -1)
                                    && !pTableLayout->_fRuleOrFrameAffectBorders
                                    && fNotCaption;


    if (!pTableLayout || (!pTableLayout->_fBorder && fUseBorderCache) || pCellNode->IsDisplayNone())
    {
        memset(pborderinfo, 0, sizeof(CBorderInfo));
        goto Done;
    }

    if (pTableLayout->_fBorderInfoCellDefaultCached && fUseBorderCache)
    {
        Assert(pTableLayout->_pBorderInfoCellDefault && "CTableLayout::_fBorderInfoCellDefaultCached should imply CTableLayout::_pBorderInfoCellDefault");
        memcpy(pborderinfo, pTableLayout->_pBorderInfoCellDefault, sizeof(CBorderInfo));
        goto Done;
    }

    // Make sure we are retrieving everything everything if we are going to cache this guy.
    fRender |= fUseBorderCache;

    // Initialize border info.
    pborderinfo->Init();

    // Inherit default settings from table.
    if (pTableLayout->_fBorder && fNotCaption)
    {
        int                  xBorder = pTableLayout->BorderX() ? 1 : 0;
        int                  yBorder = pTableLayout->BorderY() ? 1 : 0;
        WORD                 wEdges;
        WORD                 wFrameEdges;
        htmlRules            trRules = htmlRulesNotSet;
        htmlFrame            hf = htmlFrameNotSet;

        wFrameEdges = s_awEdgesFromTableFrame[hf];
        wEdges      = s_awEdgesFromTableRules[trRules];

        if ( pTableLayout->EnsureTableLayoutCache() )
            return FALSE;

        if (pTableLayout->_fRuleOrFrameAffectBorders)
        {

            htmlRules trRules = pTable->GetAArules();
            htmlFrame hf = pTable->GetAAframe();

            Assert(htmlFrameNotSet == 0);
            Assert(hf < ARRAY_SIZE(s_awEdgesFromTableFrame));
            Assert(htmlRulesNotSet == 0);
            Assert(trRules < ARRAY_SIZE(s_awEdgesFromTableRules));

            wFrameEdges = s_awEdgesFromTableFrame[hf];
            wEdges      = s_awEdgesFromTableRules[trRules];


            if (trRules == htmlRulesgroups)
            {
                CTableCol * pColGroup;
                CTableSection * pSection;

                pColGroup = pTableLayout->GetColGroup(_iCol);

                if (pColGroup)
                {
                    if (pColGroup->_iCol == _iCol)
                    {
                        wEdges |= BF_LEFT;
                    }
                    if (pColGroup->_iCol + pColGroup->_cCols == (_iCol + pCell->ColSpan()))
                    {
                        wEdges |= BF_RIGHT;
                    }
                }

                pSection = Row()->Section();
                if (pSection->_iRow == pCell->RowIndex())
                {
                    wEdges |= BF_TOP;
                }
                if (pSection->_iRow + pSection->_cRows == (pCell->RowIndex() + pCell->RowSpan()))
                {
                    wEdges |= BF_BOTTOM;
                }
            }
        }

        //
        // Adjust edges of perimeter cells to match the FRAME/BORDER setting
        //

        if ( _iCol == 0 )
        {
            if ( wFrameEdges & BF_LEFT )
                wEdges |= BF_LEFT;
            else
                wEdges &= ~BF_LEFT;
        }
        if ( _iCol+pCell->ColSpan() == pTableLayout->GetCols() )
        {
            if ( wFrameEdges & BF_RIGHT )
                wEdges |= BF_RIGHT;
            else
                wEdges &= ~BF_RIGHT;
        }
        if ( pCell->RowIndex() == pTableLayout->GetFirstRow() )
        {
            if ( wFrameEdges & BF_TOP )
                wEdges |= BF_TOP;
            else
                wEdges &= ~BF_TOP;
        }
        if ( pCell->RowIndex() + pCell->RowSpan() - 1 == pTableLayout->GetLastRow() )   // BUGBUG (alexa): potential problem going across the section
        {
            if ( wFrameEdges & BF_BOTTOM )
                wEdges |= BF_BOTTOM;
            else
                wEdges &= ~BF_BOTTOM;
        }

        if (wEdges & BF_TOP)
        {
            pborderinfo->aiWidths[BORDER_TOP]    = yBorder;
            pborderinfo->abStyles[BORDER_TOP]    = fmBorderStyleSunkenMono;
        }

        if (wEdges & BF_BOTTOM)
        {
            pborderinfo->aiWidths[BORDER_BOTTOM] = yBorder;
            pborderinfo->abStyles[BORDER_BOTTOM] = fmBorderStyleSunkenMono;
        }

        if (wEdges & BF_LEFT)
        {
            pborderinfo->aiWidths[BORDER_LEFT]   = xBorder;
            pborderinfo->abStyles[BORDER_LEFT]   = fmBorderStyleSunkenMono;
        }

        if (wEdges & BF_RIGHT)
        {
            pborderinfo->aiWidths[BORDER_RIGHT]  = xBorder;
            pborderinfo->abStyles[BORDER_RIGHT]  = fmBorderStyleSunkenMono;
        }
    }

    pCell->CElement::GetBorderInfo( pdci, pborderinfo, fRender );

    // If this is a default cell, it means we have no cache since we got here, so
    // create the cache.
    if (fUseBorderCache)
    {
        Assert(fRender && "Caching an incomplete borderinfo");
        Assert(!pTableLayout->_fBorderInfoCellDefaultCached);
        Assert(pTableLayout->_fBorder);

        if (!pTableLayout->_pBorderInfoCellDefault)
            pTableLayout->_pBorderInfoCellDefault = (CBorderInfo *)MemAlloc(Mt(CTableLayout_pBorderInfoCellDefault), sizeof(CBorderInfo));

        if (pTableLayout->_pBorderInfoCellDefault)
        {
            memcpy(pTableLayout->_pBorderInfoCellDefault, pborderinfo, sizeof(CBorderInfo));
            pTableLayout->_fBorderInfoCellDefaultCached = TRUE;
        }
    }

Done:
    if (    pborderinfo->wEdges
        ||  pborderinfo->rcSpace.top    > 0
        ||  pborderinfo->rcSpace.bottom > 0
        ||  pborderinfo->rcSpace.left   > 0
        ||  pborderinfo->rcSpace.right  > 0)
    {
        return (    pborderinfo->wEdges & (BF_TOP | BF_RIGHT | BF_BOTTOM | BF_LEFT)
                &&  pborderinfo->aiWidths[BORDER_TOP]  == pborderinfo->aiWidths[BORDER_BOTTOM]
                &&  pborderinfo->aiWidths[BORDER_LEFT] == pborderinfo->aiWidths[BORDER_RIGHT]
                &&  pborderinfo->aiWidths[BORDER_TOP]  == pborderinfo->aiWidths[BORDER_LEFT]
                        ? DISPNODEBORDER_SIMPLE
                        : DISPNODEBORDER_COMPLEX);
    }
    return DISPNODEBORDER_NONE;
}


//+------------------------------------------------------------------------
//
//  Macros:     Some useful macros for accessing table cell neighbors.
//
//  Synopsis:   Retrieve adjacent borders.  These macros are useful
//              because they facilitate keeping the code border-independent,
//              i.e. avoiding the replication of common code four times.
//
//              Used by CTableCell::GetCellBorderInfo() below to resolve
//              collapsed border precendence.
//
//  Description:  Opposite:       Retrieves the border on the other side, e.g. left -> right
//                NextBorder:     Next border in clockwise direction, e.g. top -> right
//                PreviousBorder: Previous, counterclockwise border, e.g.: top -> left
//                TopLeft:        Is border top or left as opposed to bottom or right?
//                TopBottom:      Does border have vertical neighbor?
//                LeftRight:      Does border have horizontal neighbor?
//                TopRight:       Is border top or right?
//                LeftBottom:     Is border left or bottom?
//                RightBottom:    Is border right or bottom?
//                BorderFlag:     Retrieves borderflag corresponding to border.
//
//+------------------------------------------------------------------------

#define Opposite(border) ((border + 2) % 4)
#define NextBorder(border) ((border + 1) % 4)
#define PreviousBorder(border) ((border + 3) % 4)
#define TopLeft(border) (border == BORDER_LEFT || border == BORDER_TOP)
#define TopBottom(border) (border == BORDER_TOP || border == BORDER_BOTTOM)
#define LeftRight(border) (border == BORDER_LEFT || border == BORDER_RIGHT)
#define TopRight(border) (border == BORDER_TOP || border == BORDER_RIGHT)
#define LeftBottom(border) (border == BORDER_LEFT || border == BORDER_BOTTOM)
#define RightBottom(border) (border == BORDER_RIGHT || border == BORDER_BOTTOM)
#define BorderFlag(border) \
   ((border == BORDER_LEFT) ? BF_LEFT : \
   ((border == BORDER_TOP) ? BF_TOP : \
   ((border == BORDER_RIGHT) ? BF_RIGHT : BF_BOTTOM)))


//+------------------------------------------------------------------------
//
//  Member:     CTableCellLayout::GetCellBorderInfo
//
//  Synopsis:   Retrieves a table cell's borderinfo.  For normal (non-collapsed)
//              tables, simply calls GetCellBorderInfoDefault.  For collapsed
//              borders, also calls GetCellBorderInfoDefault on cell neighbors
//              to resolve collapse border precedence.
//
//  Arguments:  pdci [in]         Docinfo
//              pborderinfo [out] Pointer to borderinfo structure to be filled
//              fRender [in]      Is this borderinfo needed for rendering or
//                                layout? (render retrieves more info, e.g. colors)
//              hdc [in]          When rendering, can provide device context for
//                                clipping out spanned neighbors and border corners.
//              pfShrunkDCClipRegion [out] Set if the clipregion of hdc is shrunk.
//
//  Returns:    TRUE if this cell is responsible for at least one border.
//              FALSE otherwise.
//
//  Note:       For normal borders (non-collapsed), this routine simply calls
//              GetCellBorderInfoDefault() and returns.
//
//              Collapsed borders: During layout (fRender==FALSE), we allocate
//              space for half the cell borders, retrieving borderinfos from
//              neighbors and resolving border precedence.  During rendering,
//              we also indicate in the borderinfo which borders this cell
//              is responsible for drawing and clip out cellspan-neighbor borders
//              and border corners where necessary.
//
//-------------------------------------------------------------------------

DWORD
CTableCellLayout::GetCellBorderInfo(
    CDocInfo *      pdci,
    CBorderInfo *   pborderinfo,
    BOOL            fRender,
    HDC             hdc,
    CTable *        pTable,
    CTableLayout *  pTableLayout,
    BOOL *          pfShrunkDCClipRegion)
{
    DWORD           dnbDefault;
    CDocInfo        DI;
    CTableCell    * pCell;
    CTreeNode     * pNode;
    const CFancyFormat * pFF;

    if (!pTable)       pTable = Table();
    if (!pTableLayout) pTableLayout = pTable? pTable->Layout() : NULL;
    BOOL               fRTLTable = pTable? pTable->GetFirstBranch()->GetParaFormat()->HasRTL(TRUE) : FALSE;
    BOOL            fPrinting;

    if (!pdci)
    {
        DI.Init(ElementOwner());
        pdci = &DI;
    }

    fPrinting = pdci->_pDoc && pdci->_pDoc->IsPrintDoc();

    dnbDefault = GetCellBorderInfoDefault(pdci, pborderinfo, fRender, pTable, pTableLayout);

    if (!pTableLayout || !pTableLayout->_fCollapse)
        goto Done;

    pCell = TableCell();
    pNode = pCell->GetFirstBranch();
    if (!pNode)
        goto Done;

    pFF = pNode->GetFancyFormat();
    if (   pFF->_bPositionType == stylePositionabsolute
        || pFF->_bPositionType == stylePositionrelative)
        goto Done;

    Assert(IsCaption(Tag()) || !fRender || pfShrunkDCClipRegion);

    // Deal with collapsed borders.
    if (!IsCaption(Tag()))
    {
        CTableRow * pRow = Row();
        CTableRowLayout * pRowLayout = pRow->Layout();
        int         colspan=pCell->ColSpan(), rowspan=pCell->RowSpan();
        int         border;
        int         aryCornerBorderWidths[/*8*/] = {0, 0, 0, 0, 0, 0, 0, 0};

        if ( pTableLayout->EnsureTableLayoutCache() )
            return FALSE;

        for (border = BORDER_TOP ; border <= BORDER_LEFT ; border++)
        {
            int     iCol;
            if(!fRTLTable)
                iCol = _iCol + ((border == BORDER_RIGHT) ? colspan : ((border == BORDER_LEFT) ? -1 : 0));
            else
                iCol = _iCol + ((border == BORDER_LEFT) ? colspan : ((border == BORDER_RIGHT) ? -1 : 0));
            int     iRow = pRow->Layout()->_iRow;
            BOOL    fOwnBorder = TRUE, fCellAtTableBorder = FALSE, fOwnBorderPartially = FALSE, fFirstSweep = TRUE;
            BOOL    fTopLeft = TopLeft(border);
            BOOL    fTopRight = TopRight(border);
            int     widthMax = pborderinfo->aiWidths[border];
            long    widthMaxMin = MAXLONG, widthSegment;

            // Compute visually next or previous row.
            if (border == BORDER_BOTTOM)
            {
                //do
                {
                    iRow = pTableLayout->GetNextRowSafe(iRow+rowspan-1);
                }
                // Skip over incomplete rows.
                //while (iRow < pTableLayout->GetRows() && !pTableLayout->_aryRows[iRow]->_fCompleted);
            }
            else if (border == BORDER_TOP)
                iRow = pTableLayout->GetPreviousRow(iRow);

            do
            {
                if (iCol < 0 || iCol >= pTableLayout->_cCols || iRow < 0 || iRow >= pTableLayout->GetRows())
                {
                    fCellAtTableBorder = TRUE;
                    break;
                }

                CTableCell       *pNeighborCell   = Cell(pTableLayout->_aryRows[iRow]->Layout()->_aryCells[iCol]);
                CTableCellLayout *pNeighborLayout = (pNeighborCell) ? pNeighborCell->Layout() : NULL;
                CBorderInfo biNeighbor(FALSE);  // no init
                BOOL fNeighborHasBorders = pNeighborLayout && pNeighborLayout->GetCellBorderInfoDefault(pdci, &biNeighbor, FALSE, pTable, pTableLayout);
                BOOL fNeighborHasOppositeBorder = fNeighborHasBorders && (biNeighbor.aiWidths[Opposite(border)]);

                if (fNeighborHasOppositeBorder)
                {
                    pNode = pNeighborCell->GetFirstBranch();
                    Assert(pNode);
                    pFF = pNode->GetFancyFormat();
                    if (pFF->_bPositionType != stylePositionrelative && pFF->_bPositionType != stylePositionabsolute)
                    {
                        // Make sure that this cell and neighbor each have a border.
                        if (widthMax < biNeighbor.aiWidths[Opposite(border)] + (fOwnBorder && (!fRTLTable ? fTopLeft?1:0 : fTopRight?1:0)))
                        {
                            widthMax = biNeighbor.aiWidths[Opposite(border)];
                            fOwnBorder = FALSE;
                        }
                        else if (pborderinfo->aiWidths[border] >= biNeighbor.aiWidths[Opposite(border)] + (!fRTLTable ? fTopLeft?1:0 : fTopRight?1:0))
                        {
                            fOwnBorderPartially = TRUE;
                        }
                    }
                }

                widthSegment = (!fNeighborHasOppositeBorder || pborderinfo->aiWidths[border] >= biNeighbor.aiWidths[Opposite(border)])
                            ? pborderinfo->aiWidths[border] : biNeighbor.aiWidths[Opposite(border)];

                if (widthSegment < widthMaxMin)
                    widthMaxMin = widthSegment;

                // If rendering, set up cliprect (and information for cliprect).
                if (fRender)
                {
                    if ( fNeighborHasOppositeBorder && (colspan>1 || rowspan>1) && WHEN_DBG( !IsTagEnabled(tagNoExcludeClip) && )
                         (pborderinfo->aiWidths[border] < biNeighbor.aiWidths[Opposite(border)] + (!fRTLTable ? fTopLeft?1:0 : fTopRight?1:0)) )
                    {
                        CRect   rcNeighbor, rcBorder;

                        // Exclude clip rect (ignore if fails).
                        Assert(GetElementDispNode());

                        // Get Rect without subtracting inset.
                        pNeighborLayout->GetRect(&rcNeighbor, COORDSYS_PARENT);

                        // If we are RTLTable we need to offset for off by one error
                        if(fRTLTable)
                            rcNeighbor.OffsetX(-1);

                        rcBorder.left   = ((biNeighbor.aiWidths[BORDER_LEFT]+1)>>1);
                        rcBorder.top    = ((biNeighbor.aiWidths[BORDER_TOP]+1)>>1);
                        rcBorder.right  = (biNeighbor.aiWidths[BORDER_RIGHT]>>1);
                        rcBorder.bottom = (biNeighbor.aiWidths[BORDER_BOTTOM]>>1);

                        if (fPrinting)
                        {
                            // Scale borders to device.
                            rcBorder.left   = pdci->DocPixelsFromWindowX(rcBorder.left);
                            rcBorder.top    = pdci->DocPixelsFromWindowY(rcBorder.top);
                            rcBorder.right  = pdci->DocPixelsFromWindowX(rcBorder.right);
                            rcBorder.bottom = pdci->DocPixelsFromWindowY(rcBorder.bottom);
                        }

                        Verify(ExcludeClipRect(hdc,
                                               rcNeighbor.left   - rcBorder.left,
                                               rcNeighbor.top    - rcBorder.top,
                                               rcNeighbor.right  + rcBorder.right,
                                               rcNeighbor.bottom + rcBorder.bottom));


                        Assert(pfShrunkDCClipRegion);
                        *pfShrunkDCClipRegion = TRUE;
                    }

                    if (fFirstSweep || !fRTLTable ? LeftBottom(border) : RightBottom(border))
                        aryCornerBorderWidths[(2*border+7)%8] = biNeighbor.aiWidths[PreviousBorder(border)];
                    if (fFirstSweep || !fRTLTable ? TopRight(border) : TopLeft(border))
                        aryCornerBorderWidths[2*border] = biNeighbor.aiWidths[NextBorder(border)];
                }

                if (TopBottom(border))
                    iCol++;
                else
                    iRow++;

                fFirstSweep = FALSE;
            } while ((TopBottom(border) && iCol-_iCol<colspan) || (LeftRight(border) && iRow-pRowLayout->_iRow<rowspan));

            if (fCellAtTableBorder)
            {
                CBorderInfo biTable;

                // If the table has borders, don't reserve any space for collapsed cell borders.
                if (pTableLayout->GetTableBorderInfo(pdci, &biTable, FALSE)
                    && (biTable.wEdges & BorderFlag(border)))
                {
                    pborderinfo->wEdges &= ~BorderFlag(border);
                    pborderinfo->aiWidths[border] = 0;
                }
                // else change nothing: Cell borders are laid out and rendered in full
                // and uncollapsed.
            }
            else if (fOwnBorder || (fOwnBorderPartially && fRender))
            {
                // This cell is responsible for drawing borders along the entire edge.
                // If we are in the rendering mode (fRender), we return the full
                // width (no change necessary).
                Assert(widthMax == pborderinfo->aiWidths[border] || (fOwnBorderPartially && fRender));

                if (!fRender)
                {
                    // Because neighboring cells each provide half the space for
                    // collapsed borders, we divide the border width by 2 during
                    // layout (!fRender).  We round up for bottom and right borders.
                    pborderinfo->aiWidths[border] = (pborderinfo->aiWidths[border]+(!fRTLTable ? fTopLeft?1:0 : fTopRight?1:0))>>1;
                }
            }
            else
            {
                // One of the neighbors is responsible for drawing borders along at least
                // part of the edge because its border is wider.
                Assert(widthMax + (!fRTLTable ? fTopLeft?1:0 : fTopRight?1:0) > pborderinfo->aiWidths[border]);

                // If we are in render mode, clear the border edge since our neighbor
                // is drawing the edge.  During layout, we budget for half the neighbor's
                // space. We round up for bottom and right borders.
                if (fRender)
                {
                    pborderinfo->wEdges &= ~BorderFlag(border);

                    // Return border width, even though it is not drawn so that caller
                    // can set up cliprects correctly.
                    pborderinfo->aiWidths[border] = widthMaxMin;

                    // If space is needed for neighbor's border, mark wEdges for adjustment.
                    if (pborderinfo->aiWidths[border])
                        pborderinfo->wEdges |= BF_ADJUST;
                }
                else
                {
                    pborderinfo->aiWidths[border] = (widthMaxMin+(!fRTLTable ? fTopLeft?1:0 : fTopRight?1:0))>>1;

                    // During layout, make sure the bit in wEdges is set when we need to make
                    // space for a neighbor's edge.
                    if (pborderinfo->aiWidths[border])
                    {
                        Assert(!fRender && "Only set wEdges for neighbor's borders when we are not rendering");
                        pborderinfo->wEdges |= BorderFlag(border);
                    }
                }
            }

            // Cache maximal bottom and trail width encountered.
            if (border == BORDER_BOTTOM && pRowLayout->_yWidestCollapsedBorder < widthMax)
                pRowLayout->_yWidestCollapsedBorder = widthMax;
            else if (!fRTLTable ? border == BORDER_RIGHT && pTableLayout->_xWidestCollapsedBorder < widthMax
                                : border == BORDER_LEFT && pTableLayout->_xWidestCollapsedBorder < widthMax)
                pTableLayout->_xWidestCollapsedBorder = widthMax;
        }

        // Border corners: If we are in rendering mode, make sure the border corners
        // have no overlap.  For corners we apply the same precedence rules as for edges.
        if (fRender)
        {
            for (border = BORDER_TOP ; border <= BORDER_LEFT ; border++)
            {
                int     iCol;
                if(!fRTLTable)
                    iCol = _iCol + ((NextBorder(border) == BORDER_RIGHT) ? colspan : ((NextBorder(border) == BORDER_LEFT) ? -1 : 0))
                                  + ((border == BORDER_RIGHT) ? colspan : ((border == BORDER_LEFT) ? -1 : 0));
                else
                    iCol = _iCol + ((NextBorder(border) == BORDER_LEFT) ? colspan : ((NextBorder(border) == BORDER_RIGHT) ? -1 : 0))
                                  + ((border == BORDER_LEFT) ? colspan : ((border == BORDER_RIGHT) ? -1 : 0));

                int     iRow = pRowLayout->_iRow;

                // If we don't render any edge touching this corner, we don't need to clip.
                if (!(pborderinfo->wEdges & BorderFlag(border)) && !(pborderinfo->wEdges & BorderFlag(NextBorder(border))))
                    continue;

                // Compute visually next or previous row.
                if (border == BORDER_BOTTOM || NextBorder(border) == BORDER_BOTTOM)
                    iRow = pTableLayout->GetNextRowSafe(iRow+rowspan-1);
                else if (border == BORDER_TOP || NextBorder(border) == BORDER_TOP)
                    iRow = pTableLayout->GetPreviousRow(iRow);


                // Cell at table border?
                if (iCol < 0 || iCol >= pTableLayout->_cCols || iRow < 0 || iRow >= pTableLayout->GetRows())
                    continue;

                CTableCell *pNeighborCell = pTableLayout->_aryRows[iRow]->Layout()->_aryCells[iCol];
                BOOL fReject = !IsReal(pNeighborCell);
                pNeighborCell = Cell(pNeighborCell);

                if (!pNeighborCell)
                    continue;

                CTableCellLayout * pNeighborLayout = pNeighborCell->Layout();
                int colspanN = pNeighborCell->ColSpan(),
                    rowspanN = pNeighborCell->RowSpan();


                int borderRTLSensitive = !fRTLTable ? border : (3 - border);

                // Reject certain col/rowspans.
                switch(borderRTLSensitive)
                {
                case BORDER_TOP:
                    // Reject rowspans in top-right corner because there is no corner problem.
                    if (!fReject && rowspanN > 1)
                    {
                        fReject = TRUE;
                        break;
                    }

                    // Reject only if not last cell of rowspan (using assumption
                    // that rowspans don't cross sections).
                    if (fReject && colspanN == 1)
                        fReject = (iRow - pNeighborCell->Row()->Layout()->_iRow + 1 != rowspanN);
                    break;
                case BORDER_RIGHT:
                    // In bottom-right corner, no colspans or rowspans cause a corner problem.
                    Assert(!fReject || colspanN>1 || rowspanN>1);
                    break;
                case BORDER_BOTTOM:
                    // Reject colspans in bottom-left corner because there is no corner problem.
                    if (!fReject && colspanN > 1)
                    {
                        fReject = TRUE;
                        break;
                    }

                    // Reject only if not last cell of colspan.
                    if (fReject && rowspanN == 1)
                        fReject = (iCol - pNeighborLayout->_iCol + 1 != colspanN);
                    break;
                case BORDER_LEFT:
                    // In top-left corner, no colspans or rowspans cause a corner problem.
                    if (!fReject && (colspanN > 1 || rowspanN > 1))
                    {
                        fReject = TRUE;
                        break;
                    }

                    // Reject only if not last cell of rowspan (using assumption
                    // that rowspans don't cross sections).
                    if (fReject)
                        fReject = (iRow - pNeighborCell->Row()->Layout()->_iRow + 1 != rowspanN)
                               || (iCol - pNeighborLayout->_iCol + 1 != colspanN);
                    break;
                }

                if (fReject)
                    continue;

                Assert(pNeighborCell);

                CBorderInfo biNeighbor(FALSE); // No init
                pNeighborLayout->GetCellBorderInfoDefault(pdci, &biNeighbor, FALSE, pTable, pTableLayout);

                // 1. Find the competing opposite candidate width.
                int widthWinnerX = 0,
                    widthWinnerY = 0;
                CRect rcNeighbor;

                // When table have fixed layout, some cells might not be calc'ed yet, and thus might
                // not have a display node yet.  In that case, don't address corner rendering problem.
                if (!pNeighborLayout->_pDispNode)
                {
                    Assert(pNeighborLayout->_fSizeThis && pTableLayout->IsFixedBehaviour());
                    continue;
                }

                pNeighborLayout->GetRect(&rcNeighbor, COORDSYS_PARENT);

                // If we are RTLTable we need to offset for off by one error
                if(fRTLTable)
                    rcNeighbor.OffsetX(-1);

                WHEN_DBG( if (!IsTagEnabled(tagDontComeOnIn)) )
                {
                    int xOppositeWidth = aryCornerBorderWidths[2*border];
                    int yOppositeWidth = aryCornerBorderWidths[2*border+1];
                    BOOL fWinnerX, fWinnerY;
                    BOOL fTopLeft = TopLeft(border), fTopLeftNext = TopLeft(NextBorder(border));
                    BOOL fTopRight = TopRight(border), fTopRightNext = TopRight(NextBorder(border));

                    // Round 1: Have the two borders of the corner neighbor compete against the
                    // borders of the two direct neighbors.
                    if (xOppositeWidth < biNeighbor.aiWidths[Opposite(NextBorder(border))])
                        xOppositeWidth = biNeighbor.aiWidths[Opposite(NextBorder(border))];
                    if (yOppositeWidth < biNeighbor.aiWidths[Opposite(border)])
                        yOppositeWidth = biNeighbor.aiWidths[Opposite(border)];

                    // Round 2: Have the borders of this cell compete against the winner of
                    // round 1.
                    fWinnerX = pborderinfo->aiWidths[NextBorder(border)] >= xOppositeWidth;
                    fWinnerY = pborderinfo->aiWidths[border] >= yOppositeWidth;

                    // Set the width to the winners of round 2.
                    widthWinnerX = fWinnerX ? (-((pborderinfo->aiWidths[NextBorder(border)]+(!fRTLTable ? (fTopLeftNext?0:1) : (fTopRightNext?0:1)))>>1))
                                            : ((xOppositeWidth+(!fRTLTable ? (fTopLeftNext?0:1) : (fTopRightNext?0:1)))>>1);
                    widthWinnerY = fWinnerY ? (-((pborderinfo->aiWidths[border]+(!fRTLTable ? (fTopLeft?0:1) : (fTopRight?0:1)))>>1))
                                            : ((yOppositeWidth+(!fRTLTable ? (fTopLeft?0:1) : (fTopRight?0:1)))>>1);

                    if (fWinnerX || fWinnerY)
                    {
#if DBG == 1
                    // If both edges are winning, avoid the inset rect.
                    // Note: This condition assumes that wider borders win.  This will no longer be the case
                    // when we implement borders on other table elements such as rows which always win over
                    // table cell border in IE5, BETA2.  Then two winners doesn't necessarily mean we are
                    // cutting into our neighbor's inset rect.
                        if (IsTagEnabled(tagClipInsetRect) && fWinnerX && fWinnerY)
                        {
                            widthWinnerX = -((xOppositeWidth+(!fRTLTable ? (fTopLeftNext?0:1) : (fTopRightNext?0:1)))>>1);
                            widthWinnerY = -((yOppositeWidth+(!fRTLTable ? (fTopLeft?0:1) : (fTopRight?0:1)))>>1);
                        }
                        else
#endif // DBG==1
                            continue;
                    }
                }

                // Scale border widths for printing.
                if (fPrinting)
                {
                    widthWinnerX = pdci->DocPixelsFromWindowX(widthWinnerX);
                    widthWinnerY = pdci->DocPixelsFromWindowY(widthWinnerY);
                }

                // 2. Make room for winning edge or inset.

                switch (border)
                {
                case BORDER_TOP:
                    rcNeighbor.left   -= widthWinnerX;
                    rcNeighbor.bottom += widthWinnerY;
                    break;
                case BORDER_RIGHT:
                    rcNeighbor.top    -= widthWinnerX;
                    rcNeighbor.left   -= widthWinnerY;
                    break;
                case BORDER_BOTTOM:
                    rcNeighbor.right  += widthWinnerX;
                    rcNeighbor.top    -= widthWinnerY;
                    break;
                case BORDER_LEFT:
                    rcNeighbor.bottom += widthWinnerX;
                    rcNeighbor.right  += widthWinnerY;
                    break;
                }

                // Actually exclude winning or inset rect of neighbor.
                if ( WHEN_DBG(!IsTagEnabled(tagNoExcludeClipCorners) &&) !IsRectEmpty(&rcNeighbor))
                {
                    // Exclude clip rect (ignore if fails).
                    Verify(ExcludeClipRect(hdc, rcNeighbor.left, rcNeighbor.top, rcNeighbor.right, rcNeighbor.bottom));
                    Assert(pfShrunkDCClipRegion);
                    *pfShrunkDCClipRegion = TRUE;
                }

            }
        }
    }
    else if (dnbDefault != DISPNODEBORDER_NONE)
    {
        // Collapse caption.
        Assert(IsCaption(Tag()));
        CBorderInfo biTable;

        // If the table has borders, we need to zero out the touching caption border.
        if (pTableLayout->GetTableBorderInfo(pdci, &biTable, FALSE))
        {
            int border = (DYNCAST(CTableCaption, pCell)->IsCaptionOnBottom()) ? BORDER_TOP : BORDER_BOTTOM;

            // If the table has a border on the corresponding side, zero out caption border.
            if (biTable.wEdges & BorderFlag(Opposite(border)))
            {
                pborderinfo->wEdges &= ~BorderFlag(border);
                pborderinfo->aiWidths[border] = 0;
            }
        }

    }

    // We should only finish this way, if we had to collapse borders.
    Assert(pTableLayout->_fCollapse);

Done:
    if (    pborderinfo->wEdges
        ||  pborderinfo->rcSpace.top    > 0
        ||  pborderinfo->rcSpace.bottom > 0
        ||  pborderinfo->rcSpace.left   > 0
        ||  pborderinfo->rcSpace.right  > 0)
    {
        return (    pborderinfo->wEdges & (BF_TOP | BF_RIGHT | BF_BOTTOM | BF_LEFT)
                &&  pborderinfo->aiWidths[BORDER_TOP]  == pborderinfo->aiWidths[BORDER_BOTTOM]
                &&  pborderinfo->aiWidths[BORDER_LEFT] == pborderinfo->aiWidths[BORDER_RIGHT]
                &&  pborderinfo->aiWidths[BORDER_TOP]  == pborderinfo->aiWidths[BORDER_LEFT]
                        ? DISPNODEBORDER_SIMPLE
                        : DISPNODEBORDER_COMPLEX);
    }
    return DISPNODEBORDER_NONE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableLayout::GetTableBorderInfo
//
//  Synopsis:   fill out border information
//
//----------------------------------------------------------------------------

DWORD
CTableLayout::GetTableBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fRender)
{
    htmlFrame   hf = Table()->GetAAframe();
    WORD        wEdges;

    Assert(pdci);

    if ( _xBorder || _yBorder )
    {
        Assert(htmlFrameNotSet == 0);
        Assert(hf < ARRAY_SIZE(s_awEdgesFromTableFrame));
        wEdges = s_awEdgesFromTableFrame[hf];

        if (wEdges & BF_TOP)
        {
            pborderinfo->aiWidths[BORDER_TOP]    = _yBorder;
            pborderinfo->abStyles[BORDER_TOP]    = fmBorderStyleRaisedMono;
        }

        if (wEdges & BF_BOTTOM)
        {
            pborderinfo->aiWidths[BORDER_BOTTOM] = _yBorder;
            pborderinfo->abStyles[BORDER_BOTTOM] = fmBorderStyleRaisedMono;
        }

        if (wEdges & BF_LEFT)
        {
            pborderinfo->aiWidths[BORDER_LEFT]   = _xBorder;
            pborderinfo->abStyles[BORDER_LEFT]   = fmBorderStyleRaisedMono;
        }

        if (wEdges & BF_RIGHT)
        {
            pborderinfo->aiWidths[BORDER_RIGHT]  = _xBorder;
            pborderinfo->abStyles[BORDER_RIGHT]  = fmBorderStyleRaisedMono;
        }

        if (pdci && pdci->_pDoc && pdci->_pDoc->IsPrintDoc())
        {
            // Scale borders from device to pixels temporarily.
            pborderinfo->aiWidths[BORDER_TOP]    = pdci->WindowYFromDocPixels( pborderinfo->aiWidths[BORDER_TOP]    );
            pborderinfo->aiWidths[BORDER_RIGHT]  = pdci->WindowXFromDocPixels( pborderinfo->aiWidths[BORDER_RIGHT]  );
            pborderinfo->aiWidths[BORDER_BOTTOM] = pdci->WindowYFromDocPixels( pborderinfo->aiWidths[BORDER_BOTTOM] );
            pborderinfo->aiWidths[BORDER_LEFT]   = pdci->WindowXFromDocPixels( pborderinfo->aiWidths[BORDER_LEFT]   );
        }
    }

    Table()->CElement::GetBorderInfo( pdci, pborderinfo, fRender );

    if (    pborderinfo->wEdges
        ||  hf != htmlFrameNotSet
        ||  pborderinfo->rcSpace.top    > 0
        ||  pborderinfo->rcSpace.bottom > 0
        ||  pborderinfo->rcSpace.left   > 0
        ||  pborderinfo->rcSpace.right  > 0)
    {
        return (    pborderinfo->wEdges & (BF_TOP | BF_RIGHT | BF_BOTTOM | BF_LEFT)
                &&  pborderinfo->aiWidths[BORDER_TOP]  == pborderinfo->aiWidths[BORDER_BOTTOM]
                &&  pborderinfo->aiWidths[BORDER_LEFT] == pborderinfo->aiWidths[BORDER_RIGHT]
                &&  pborderinfo->aiWidths[BORDER_TOP]  == pborderinfo->aiWidths[BORDER_LEFT]
                        ? DISPNODEBORDER_SIMPLE
                        : DISPNODEBORDER_COMPLEX);
    }
    return DISPNODEBORDER_NONE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CalculateBorderAndSpacing
//
//  Synopsis:   Calculate and cache border, cellspacing, cellpadding
//
//--------------------------------------------------------------------------

void
CTableLayout::CalculateBorderAndSpacing(CDocInfo * pdci)
{
    CTable    * pTable = Table();
    CUnitValue  cuv;
    CUnitValue  uvDefaultborder;
    CBorderInfo borderinfo;
    CTreeNode * pNodeSelf = GetFirstBranch();
    htmlRules   trRules = pTable->GetAArules();
    htmlFrame   hf  = pTable->GetAAframe();

    _fRuleOrFrameAffectBorders =
        (trRules != htmlRulesNotSet && trRules != htmlRulesall) ||  // only when there are no groups/rows/cols rules
        (hf != htmlFrameNotSet);                                    // only when FRAME attribute is not set on table

    cuv = pTable->GetAAborder();

    if (cuv.IsNull())
    {
        _xBorder = 0;
        _yBorder = 0;

        Assert(hf < ARRAY_SIZE(s_awEdgesFromTableFrame));
        WORD wFrameEdges = s_awEdgesFromTableFrame[hf];

        if (trRules != htmlRulesNotSet || hf != htmlFrameNotSet)
        {
            uvDefaultborder.SetValue(1, CUnitValue::UNIT_PIXELS);
            switch(trRules)
            {
                case htmlRulesrows:
                    _yBorder = uvDefaultborder.YGetPixelValue(pdci, 0, 1);
                    break;

                case htmlRulesgroups:
                case htmlRulesall:
                    _yBorder = uvDefaultborder.YGetPixelValue(pdci, 0, 1);
                    // fall through
                case htmlRulescols:
                    _xBorder = uvDefaultborder.XGetPixelValue(pdci, 0, 1);
                    break;
            }

            if ( _xBorder == 0 && ((wFrameEdges & BF_LEFT) || (wFrameEdges & BF_RIGHT)) )
                _xBorder = uvDefaultborder.XGetPixelValue(pdci, 0, 1);

            if ( _yBorder == 0 && ((wFrameEdges & BF_TOP) || (wFrameEdges & BF_BOTTOM)) )
                _yBorder = uvDefaultborder.YGetPixelValue(pdci, 0, 1);
        }
    }
    else
    {
        // get border space
        long lFontHeight = pNodeSelf->GetFontHeightInTwips(&cuv);

        _xBorder = cuv.XGetPixelValue(pdci, 0, lFontHeight);
        _yBorder = cuv.YGetPixelValue(pdci, 0, lFontHeight);

        // use 1 if negative and restrict it .. use TagNotAssignedDefault for 1
        if (_xBorder < 0)
        {
            uvDefaultborder.SetValue ( 1,CUnitValue::UNIT_PIXELS );

            _xBorder = uvDefaultborder.XGetPixelValue(pdci, 0, 1);
            _yBorder = uvDefaultborder.YGetPixelValue(pdci, 0, 1);
        }
        // BUGBUG: MAX_BORDER_SPACE is always in pixels while _x/_yBorder are in device units.
        if (_xBorder > MAX_BORDER_SPACE)
        {
            _xBorder = MAX_BORDER_SPACE;
        }
        if (_yBorder > MAX_BORDER_SPACE)
        {
            _yBorder = MAX_BORDER_SPACE;
        }

    }


    _fBorder = ( _yBorder != 0 ) || ( _xBorder != 0 );


    GetTableBorderInfo(pdci, &borderinfo, FALSE);
    if (borderinfo.wEdges)
    {
        // Scale borders to device.
        borderinfo.aiWidths[BORDER_TOP]    = pdci->DocPixelsFromWindowY( borderinfo.aiWidths[BORDER_TOP]    );
        borderinfo.aiWidths[BORDER_RIGHT]  = pdci->DocPixelsFromWindowX( borderinfo.aiWidths[BORDER_RIGHT]  );
        borderinfo.aiWidths[BORDER_BOTTOM] = pdci->DocPixelsFromWindowY( borderinfo.aiWidths[BORDER_BOTTOM] );
        borderinfo.aiWidths[BORDER_LEFT]   = pdci->DocPixelsFromWindowX( borderinfo.aiWidths[BORDER_LEFT]   );
    }
    memcpy(_aiBorderWidths, borderinfo.aiWidths, 4*sizeof(int));

    cuv = pTable->GetAAcellSpacing();

    if (cuv.IsNull())
    {
        CUnitValue uvDefaultCellSpacing;
        uvDefaultCellSpacing.SetValue (_fCollapse?0:2, CUnitValue::UNIT_PIXELS);
        _xCellSpacing = uvDefaultCellSpacing.XGetPixelValue(pdci, 0, 1);
        _yCellSpacing = uvDefaultCellSpacing.YGetPixelValue(pdci, 0, 1);
    }
    else
    {
        long SpaceFontHeight = pNodeSelf->GetFontHeightInTwips(&cuv);

        _xCellSpacing = max(0L,cuv.XGetPixelValue(pdci, 0, SpaceFontHeight));
        _yCellSpacing = max(0L,cuv.YGetPixelValue(pdci, 0, SpaceFontHeight));
        if (_xCellSpacing > MAX_CELL_SPACING)
        {
            _xCellSpacing = MAX_CELL_SPACING;
        }
        if (_yCellSpacing > MAX_CELL_SPACING)
        {
            _yCellSpacing = MAX_CELL_SPACING;
        }
    }

    _xCellPadding =
    _yCellPadding = 0;
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
//  Returns:    pSite (if found), NULL otherwise
//
//  NOTE:       This routine and GetNextLayout walk through the captions
//              and rows on the table. Negative cookies represent
//              CAPTIONs rendered at the top. Cookies in the range 0 to
//              one less than the number of rows represent rows. Positive
//              cookies greater than or equal to the number of rows
//              represent CAPTIONs rendered at the bottom. Since all the
//              CAPTIONs are kept in a single array, regardless where they
//              are rendered, the cookie may skip by more then one when
//              walking through the CAPTIONs. The cookie end-points are:
//
//                  Top-most CAPTION    - (-1 - _aryCaptions.Size())
//                  Top-most ROW        - (-1)
//                  Bottom-most ROW     - (GetRows())
//                  Bottom-most CAPTION - (GetRows() + _aryCaptions.Size())
//
//              If you change the way the cookie is implemented, then
//              please also change the GetCookieForSite funciton.
//
//-------------------------------------------------------------------------

CLayout *
CTableLayout::GetFirstLayout ( DWORD_PTR * pdw, BOOL fBack, BOOL fRaw )
{
    // NOTE: This routine always walks the actual array (after ensuring
    //       it is in-sync with the current state of the tree)

    Assert(!fRaw);

    {
        if ( EnsureTableLayoutCache() )
            return NULL;
    }

    *pdw = fBack
            ? DWORD( GetRows() ) + DWORD( _aryCaptions.Size() )
            : DWORD( -1 ) - DWORD( _aryCaptions.Size() );

    return CTableLayout::GetNextLayout( pdw, fBack, fRaw );
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
//  Returns:    site
//
//  Note:       See comment on GetFirstLayout
//
//-------------------------------------------------------------------------

CLayout *
CTableLayout::GetNextLayout ( DWORD_PTR * pdw, BOOL fBack, BOOL fRaw )
{
    int i, cRows, cCaptions;

    // NOTE: This routine always walks the actual array (after ensuring
    //       it is in-sync with the current state of the tree)

    Assert(!fRaw);

    {
        if ( EnsureTableLayoutCache() )
            return NULL;
    }

    i         = *pdw;
    cRows     = GetRows();
    cCaptions = _aryCaptions.Size();

    Assert(i >= (-1 - _aryCaptions.Size()));
    Assert(i <= (GetRows() + _aryCaptions.Size()));

    if (fBack)
    {
        i--;

        // While the cookie is past the end of the row array,
        // look for a caption which renders at the bottom
        for ( ; i >= cRows; i--)
        {
            if (_aryCaptions[i-cRows]->_uLocation == CTableCaption::CAPTION_BOTTOM)
            {
                *pdw = (DWORD)i;
                return _aryCaptions[i-cRows]->GetCurLayout();
            }
        }

        // While the cookie is before the rows,
        // look for a caption which renders at the top
        if (i < 0)
        {
            for ( ; (cCaptions+i) >= 0; i--)
            {
                if (_aryCaptions[cCaptions+i]->_uLocation == CTableCaption::CAPTION_TOP)
                {
                    *pdw = (DWORD)i;
                    return _aryCaptions[cCaptions+i]->GetCurLayout();
                }
            }

            return NULL;
        }

        // Otherwise, fall through and return the row
    }

    else
    {
        i++;

        // While the cookie is before the rows,
        // look for a caption which renders at the top
        for ( ; i < 0; i++)
        {
            if (_aryCaptions[cCaptions+i]->_uLocation == CTableCaption::CAPTION_TOP)
            {
                *pdw = (DWORD)i;
                return _aryCaptions[cCaptions+i]->GetCurLayout();
            }
        }

        // While the cookie is past the end of the row array,
        // look for a caption which renders at the bottom
        if (i >= cRows)
        {
            for ( ; i < (cRows+cCaptions); i++)
            {
                if (_aryCaptions[i-cRows]->_uLocation == CTableCaption::CAPTION_BOTTOM)
                {
                    *pdw = (DWORD)i;
                    return _aryCaptions[i-cRows]->GetCurLayout();
                }
            }

            return NULL;
        }

        // Otherwise, fall through and return the row
    }

    Assert( i >= 0 && i < GetRows());
    *pdw = (DWORD)i;
    return _aryRows[i]->GetCurLayout();
}

//+---------------------------------------------------------------------------
//
// Member:      ContainsChildLayout
//
//----------------------------------------------------------------------------
BOOL
CTableLayout::ContainsChildLayout(BOOL fRaw)
{
    DWORD_PTR dw;
    Assert(!fRaw);
    CLayout * pLayout = GetFirstLayout(&dw, FALSE, fRaw);
    ClearLayoutIterator(dw, fRaw);
    return pLayout ? TRUE : FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableLayout::Draw
//
//  Synopsis:   Paint the table
//
//----------------------------------------------------------------------------

void
CTableLayout::Draw(CFormDrawInfo *pDI, CDispNode * pDispNode)
{
    if ( EnsureTableLayoutCache() )
        return;

#if DBG == 1
    if (!IsTagEnabled(tagTableRecalc))
#endif
    if (!IsCalced())    // return if table is not calculated
        return;

    super::Draw(pDI, pDispNode);
}

HRESULT
CTableLayout::GetElementsInZOrder(CPtrAry<CElement *> * paryElements,
                            CElement            * pElementThis,
                            RECT                * prc,
                            HRGN                  hrgn,
                            BOOL                  fIncludeNotVisible /* ==FALSE*/)
{
    CTableCaption **    ppCaption;
    CTableRow *         pRow;
    CTableRowLayout *   pRowLayout;
    CTableRow **        ppRow;
    CTableCell *        pCell;
    CTableCellLayout *  pCellLayout;
    CTableCell **       ppCell;
    BOOL                fCaption     = FALSE;
    BOOL                fRegionFound = FALSE;
    int                 cRBeg = -1;
    int                 cREnd = -1;
    int                 cR, cC;
    int                 xAdd, yAdd;
    htmlRules           trRules;
    HRESULT             hr;
    CDoc *              pDoc;

    hr = EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    //
    // set up xAdd and yAdd to cell spacing if we are drawing rules
    // so the cells will get included...
    //

    trRules = Table()->GetAArules();
    if (trRules != htmlRulesNotSet && trRules != htmlRulesall)
    {
        xAdd = _xCellSpacing;
        yAdd = _yCellSpacing;
    }
    else
    {
        xAdd =
        yAdd = 0;
    }

    #if DBG==1
    ClearCellMark(0, GetRows(), TRUE);
    #endif

    //
    // First, add all our direct children that intersect the draw rect
    // (This includes in-flow sites and those with relative positioning)
    //

    if(pElementThis == Table())
    {
        for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
             cC > 0;
             cC--, ppCaption++)
        {
            (*ppCaption)->_fMark1 = FALSE;

            if (!(*ppCaption)->Layout()->NoContent())
            {
                if ((*ppCaption)->_uLocation == CTableCaption::CAPTION_BOTTOM)
                    fCaption = TRUE;
                else
                {
                    Assert(!(*ppCaption)->IsAbsolute());

                    fRegionFound = fRegionFound || !(*ppCaption)->IsPositionStatic();

                    hr = paryElements->Append(*ppCaption);
                    if (hr)
                        goto Cleanup;
                }
            }
        }

        for (ppRow = _aryRows, cR = GetRows(); cR > 0; ppRow++, cR--)
        {
            pRow       = *ppRow;
            if (!pRow->_fCompleted)
            {
                continue;
            }
            pRowLayout = pRow->Layout();

            pRow->_fMark1 = FALSE;

            if (cRBeg == -1)
                cRBeg = cR;
            cREnd = cR;

            Assert(!pRow->IsAbsolute());

            for (ppCell = pRowLayout->_aryCells, cC = pRowLayout->_aryCells.Size();
                cC > 0;
                ppCell++, cC--)
            {
                pCell = Cell(*ppCell);
                pCellLayout = pCell ? pCell->Layout() : NULL;
                Assert(!pCell || pCellLayout);

                if (pCell && !pCellLayout->_fClearedSiteMark)
                {
                    pCellLayout->_fClearedSiteMark = TRUE;
                    pCell->_fMark1                 = FALSE;  // _fMark1 flag resides on CElement (thus not part of CTableCellLayout)

                    Assert(!pCell->IsAbsolute());

                    fRegionFound = fRegionFound || !pCell->IsPositionStatic();

                    hr = paryElements->Append(pCell);
                    if (hr)
                        goto Cleanup;
                }
            }
        }

        if (fCaption)
        {
            for (cC = _aryCaptions.Size(), ppCaption = _aryCaptions;
                 cC > 0;
                 cC--, ppCaption++)
            {
                Assert((*ppCaption)->_fMark1 == FALSE);

                if (!(*ppCaption)->Layout()->NoContent() &&
                     (*ppCaption)->_uLocation == CTableCaption::CAPTION_BOTTOM)
                {
                    Assert(!(*ppCaption)->IsAbsolute());

                    fRegionFound = fRegionFound || !(*ppCaption)->IsPositionStatic();

                    hr = paryElements->Append(*ppCaption);
                    if (hr)
                        goto Cleanup;
                }
            }
        }

    }

    //
    // Next, add absolutely positioned sites for which this site is the region parent
    //

    pDoc = Doc();
    if (pDoc->_fRegionCollection && Table()->IsZParent())
    {
        long       lIndex;
        long       lArySize;
        CElement * pElement;
        CLayout *  pLayout;
        CCollectionCache *pCollectionCache;

        hr = THR(pDoc->PrimaryMarkup()->EnsureCollectionCache(CMarkup::REGION_COLLECTION));
        if (!hr)
        {
            pCollectionCache = pDoc->PrimaryMarkup()->CollectionCache();

            lArySize = pCollectionCache->SizeAry(CMarkup::REGION_COLLECTION);

            for (lIndex = 0; lIndex < lArySize; lIndex++)
            {
                CTreeNode * pNodeTemp;

                hr = THR(pCollectionCache->GetIntoAry(CMarkup::REGION_COLLECTION,
                                                              lIndex,
                                                              &pElement));
                Assert(pElement);

                pNodeTemp = pElement->GetFirstBranch();

                if (   !pElement->IsPositionStatic()
                    && pNodeTemp->ZParent() == pElementThis)
                {
                    pLayout = pElement->GetCurLayout();

                    if(pLayout)
                    {
                        if (    !fIncludeNotVisible
                            &&  !pElement->IsVisible(FALSE))
                        {
                            continue;
                        }
                    }
                    else
                    {
                        const CCharFormat *pCF = pNodeTemp->GetCharFormat();

                        // BUGBUG -- will this mess up visibility:visible
                        // for child elements of pElement? (lylec)
                        //
                        if (pCF->IsVisibilityHidden() ||
                            pCF->IsDisplayNone())
                        {
                            continue;
                        }
                    }

                    fRegionFound      = TRUE;

                    hr = paryElements->Append(pElement);
                    if (hr)
                        goto Cleanup;
                }
            }
        }
    }

    // BUGBUG: Since ROWs are never in the site list, how should we ensure the
    //         proper z-ordering of cells within relatively positioned rows?
    //         THIS MUST BE FIXED WHEN WE SUPPORT POSITIONING OF TABLE ELEMENTS.
    //         (brendand)
    if (fRegionFound)
    {
        qsort(*paryElements,
              paryElements->Size(),
              sizeof(CElement*),
              CompareElementsByZIndex);
    }

    if (cRBeg >= 0)
    {
        cR = GetRows();
        ClearCellMark(cR - cRBeg, cRBeg - cREnd + 1, FALSE);
    }

Cleanup:
    RRETURN(hr);
}


void CTableLayout::ClearCellMark(int i, int n, BOOL fAssert)
{
    CTableRow **    ppRow;
    CTableRowLayout * pRowLayout;
    CTableCell **   ppCell;
    CTableCell *    pCell;
    int             cR, cC;

    Assert(IsTableLayoutCacheCurrent());
    Assert(i >= 0 && n >= 0 && i + n <= GetRows());

    for (ppRow = &_aryRows[i], cR = n; cR > 0; ppRow++, cR--)
    {
        if (!(*ppRow)->_fCompleted)
        {
            continue;
        }
        pRowLayout = (*ppRow)->Layout();
        for (ppCell = pRowLayout->_aryCells, cC = pRowLayout->_aryCells.Size();
            cC > 0;
            ppCell++, cC--)
        {
            pCell = Cell(*ppCell);
            if (pCell)
            {
                Assert(!fAssert || !pCell->Layout()->_fClearedSiteMark);
                pCell->Layout()->_fClearedSiteMark = FALSE;
            }
        }
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CTableBorderRenderer::QueryInterface, IUnknown
//
//-------------------------------------------------------------------------

HRESULT
CTableBorderRenderer::QueryInterface(REFIID riid, LPVOID * ppv)
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
//  Member:     CTableBorderRenderer::DrawClient
//
//  Synopsis:   Draws collapsed, ruled, or framed table cell borders
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
CTableBorderRenderer::DrawClient(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;

    Assert(pDI);
    Assert(pDispNode->GetLayerType() == DISPNODELAYER_FLOW);
    Assert(_pTableLayout);

    _pTableLayout->DrawCellBorders(
        pDI,
        prcBounds,
        prcRedraw,
        pDispSurface,
        pDispNode,
        cookie,
        pClientData,
        dwFlags);
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CTableBorderRenderer::DumpDebugInfo
//
//  Synopsis:   Dump display tree debug information
//
//----------------------------------------------------------------------------

void
CTableBorderRenderer::DumpDebugInfo(
    HANDLE         hFile,
    long           level,
    long           childNumber,
    CDispNode *    pDispNode,
    void *         cookie)
{
    WriteHelp(
            hFile,
            _T("<<br>\r\n<<font class=tag>&lt;<0s>&gt;<</font><<br>\r\n"),
            _T("Table Border Renderer"));
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CTableLayout::DrawCellBorders
//
//  Synopsis:   Draws collapsed, ruled, or framed table cell borders
//
//  Arguments:  prcBounds       bounding rect of display leaf node
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
// NOTE (paulnel) - 'lead' refers to left in left to right (LTR) and right in 
//                         right to left (RTL)
//                  'trail' refers to right in LTR and left in RTL
//----------------------------------------------------------------------------

WHEN_DBG(COLORREF g_crRotate = 255;)

void
CTableLayout::DrawCellBorders(
    CFormDrawInfo * pDI,
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
    HDC             hdc = pDI->GetDC(TRUE);
    CTableRowLayout * pRowLayout;
    int iRow, iRowTop, iRowBottom, iRowPrevious, iRowNext;
    int iCol, iColLead, iColTrail;
    int yRowTop, yRowBottom, xColLead, xColTrail;

    BOOL fRightToLeft = pDispNode->IsRightToLeft();
    CRect rcDraw((CRect)*prcRedraw);

    HRGN hrgnClipOriginal;

    if (!IsTableLayoutCacheCurrent())
        return;

    // If the table has no rows, we are done.
    if (!GetRows())
        return;

    // if there is no calculated columns. (the right fix would be to remove all the display nodes from the tree on FlushGrid, bug #64931)
    if (!_aryColCalcs.Size())
        return;

    // Remember the original clip rect
    hrgnClipOriginal = CreateRectRgn(0,0,1,1);
    if (hrgnClipOriginal)
        GetClipRgn(hdc, hrgnClipOriginal);

#if DBG==1
    COLORREF crOld = 0x0;

    if (IsTagEnabled(tagNoCollapsedBorders))
        return;

    if (_pBorderInfoCellDefault && IsTagEnabled(tagRedCollapsedBorders))
    {
        crOld = _pBorderInfoCellDefault->acrColors[0][0];

        _pBorderInfoCellDefault->acrColors[0][0] = _pBorderInfoCellDefault->acrColors[0][1] = _pBorderInfoCellDefault->acrColors[0][2] =
        _pBorderInfoCellDefault->acrColors[1][0] = _pBorderInfoCellDefault->acrColors[1][1] = _pBorderInfoCellDefault->acrColors[1][2] =
        _pBorderInfoCellDefault->acrColors[2][0] = _pBorderInfoCellDefault->acrColors[2][1] = _pBorderInfoCellDefault->acrColors[2][2] =
        _pBorderInfoCellDefault->acrColors[3][0] = _pBorderInfoCellDefault->acrColors[3][1] = _pBorderInfoCellDefault->acrColors[3][2] = g_crRotate;

        g_crRotate = (g_crRotate << 8);
        if (!g_crRotate)
            g_crRotate = 255;
    }
#endif // DBG==1

    //
    // Obtain subgrid of table borders to be rendered.
    // (If we are collapsing borders, make sure the invalid rect is
    // large enough to include neighboring borders so that collapsed
    // neighbors get a chance to draw their borders.)
    //

    //
    // iRowTop.
    //

    yRowTop    =
    yRowBottom = 0;
    pRowLayout = GetRowLayoutFromPos(rcDraw.top, &yRowTop, &yRowBottom);
    iRowTop = pRowLayout ? pRowLayout->_iRow : GetFirstRow();
    if (iRowTop != GetFirstRow())
    {
        iRowPrevious = GetPreviousRowSafe(iRowTop);
        Assert(iRowPrevious >= 0);

        // Only expand when redraw rect infringes on maximum border area.
        if (_fCollapse && rcDraw.top < yRowTop + ((_aryRows[iRowPrevious]->Layout()->_yWidestCollapsedBorder+1)>>1)
            WHEN_DBG( && !IsTagEnabled(tagNoInflateRect) ) )
        {
            iRowTop = iRowPrevious;
        }
    }

    //
    // iRowBottom.
    //

    // Note that the bottom, right coordinates are outside the clip-rect (subtract 1).
    yRowTop    =
    yRowBottom = 0;
    pRowLayout = GetRowLayoutFromPos(rcDraw.bottom-1, &yRowTop, &yRowBottom);
    iRowBottom = pRowLayout ? pRowLayout->_iRow : GetLastRow();
    if (iRowBottom != GetLastRow())
    {
        iRowNext = GetNextRowSafe(iRowBottom);
        Assert(iRowNext < GetRows());

        // Only expand when cliprect infringes on maximum border area.
        if (_fCollapse && rcDraw.bottom >= yRowBottom - ((pRowLayout->_yWidestCollapsedBorder)>>1)
            WHEN_DBG( && !IsTagEnabled(tagNoInflateRect) ) )
        {
            iRowBottom = iRowNext;
        }
    }

    //
    // iColLead.
    //

    iColLead = GetColExtentFromPos(!fRightToLeft ? rcDraw.left: rcDraw.right, &xColLead, &xColTrail, fRightToLeft);
    if (iColLead == -1)
        iColLead = 0;

    // Only expand when cliprect infringes on maximum border area.
    if (   _fCollapse
        && iColLead > 0
        && (!fRightToLeft 
            ? rcDraw.left < xColLead + ((_xWidestCollapsedBorder+1)>>1)
            : rcDraw.right > xColLead - (_xWidestCollapsedBorder>>1))
        WHEN_DBG( && !IsTagEnabled(tagNoInflateRect) ) )
    {
        iColLead--;
    }

    //
    // iColTrail.
    //

    // Note that the bottom, right coordinates are outside the clip-rect (subtract 1).
    iColTrail = GetColExtentFromPos(!fRightToLeft ? rcDraw.right-1 : rcDraw.left+1, &xColLead, &xColTrail, fRightToLeft);
    if (iColTrail == -1)
        iColTrail = GetCols()-1;

    // Only expand when cliprect infringes on maximum border area.
    if (   _fCollapse
        && iColTrail < GetCols()-1
        && (!fRightToLeft
            ? rcDraw.right >= xColTrail - (_xWidestCollapsedBorder>>1)
            : rcDraw.left <= xColTrail + ((_xWidestCollapsedBorder+1)>>1))
        WHEN_DBG( && !IsTagEnabled(tagNoInflateRect) ) )
    {
        iColTrail++;
    }

    Assert(iRowTop    >= 0 && iRowTop    < GetRows());
    Assert(iRowBottom >= 0 && iRowBottom < GetRows());
    Assert(iColLead   >= 0 && iColLead   < GetCols());
    Assert(iColTrail  >= 0 && iColTrail  < GetCols());

    for ( iRow = iRowTop ; ; iRow = GetNextRowSafe(iRow) )
    {
        CTableRowLayout * pRowLayout = GetRow(iRow)->Layout();
        int iColSpan;
        Assert(pRowLayout);

        for ( iCol = iColLead ; iCol <= iColTrail ; iCol += iColSpan )
        {
            CTableCell * pCell = Cell(pRowLayout->_aryCells[iCol]);
            if (pCell)
            {
                CTableCellLayout * pCellLayout = pCell->Layout();
                const CFancyFormat * pFF = pCell->GetFirstBranch()->GetFancyFormat();
                if (!pFF->_fPositioned)
                {
                    BOOL fShrunkDCClipRegion = FALSE;

                    pCellLayout->DrawBorderHelper(pDI, &fShrunkDCClipRegion);

                    // If the clip region was modified, restore the original one.
                    if (fShrunkDCClipRegion)
                        SelectClipRgn(hdc, hrgnClipOriginal);

                }
                iColSpan = pCell->ColSpan() - ( iCol - pCellLayout->_iCol );
            }
            else
            {
                iColSpan = 1;
            }
        }

        // Finished last row?
        if (iRow == iRowBottom)
            break;
    }

#if DBG==1
    if (_pBorderInfoCellDefault && IsTagEnabled(tagRedCollapsedBorders) )
    {
        _pBorderInfoCellDefault->acrColors[0][0] =
        _pBorderInfoCellDefault->acrColors[0][1] =
        _pBorderInfoCellDefault->acrColors[0][2] =
        _pBorderInfoCellDefault->acrColors[1][0] =
        _pBorderInfoCellDefault->acrColors[1][1] =
        _pBorderInfoCellDefault->acrColors[1][2] =
        _pBorderInfoCellDefault->acrColors[2][0] =
        _pBorderInfoCellDefault->acrColors[2][1] =
        _pBorderInfoCellDefault->acrColors[2][2] =
        _pBorderInfoCellDefault->acrColors[3][0] =
        _pBorderInfoCellDefault->acrColors[3][1] =
        _pBorderInfoCellDefault->acrColors[3][2] = crOld;
    }
#endif // DBG==1

    // Cleanup:
    if (hrgnClipOriginal)
        DeleteObject(hrgnClipOriginal);
}

//+----------------------------------------------------------------------------
//
//  Member:     CLayout::GetClientLayersInfo
//
//-----------------------------------------------------------------------------

DWORD
CTableLayout::GetClientLayersInfo(CDispNode *pDispNodeFor)
{
    if (GetTableOuterDispNode() != pDispNodeFor)    // if draw request is for dispNode other then primary
        return 0;                                   // then no drawing at all the dispNode

    return GetPeerLayersInfo();
}

