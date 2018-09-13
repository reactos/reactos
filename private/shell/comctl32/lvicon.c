// large icon view stuff

#include "ctlspriv.h"
#include "listview.h"

#if defined(FE_IME) || !defined(WINNT)
static TCHAR const szIMECompPos[]=TEXT("IMECompPos");
#endif

__inline int ICONCXLABEL(LV *plv, LISTITEM *pitem)
{
    if (plv->ci.style & LVS_NOLABELWRAP) {
        ASSERT(pitem->cxSingleLabel == pitem->cxMultiLabel);
    }
    return pitem->cxMultiLabel;
}

int LV_GetNewColWidth(LV* plv, int iFirst, int iLast);
void LV_AdjustViewRectOnMove(LV* plv, LISTITEM *pitem, int x, int y);
UINT LV_IsItemOnViewEdge(LV* plv, LISTITEM *pitem);
void ListView_RecalcRegion(LV *plv, BOOL fForce, BOOL fRedraw);

extern BOOL g_fSlowMachine;

BOOL ListView_IDrawItem(PLVDRAWITEM plvdi)
{
    RECT rcIcon;
    RECT rcLabel;
    RECT rcBounds;
    RECT rcT;
    TCHAR ach[CCHLABELMAX];
    LV_ITEM item;
    int i = (int) plvdi->nmcd.nmcd.dwItemSpec;
    LV* plv = plvdi->plv;
    LISTITEM FAR* pitem;
    BOOL fUnfolded;

    if (ListView_IsOwnerData(plv))
    {
        LISTITEM litem;

        // moved here to reduce call backs in OWNERDATA case
        item.iItem = i;
        item.iSubItem = 0;
        item.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
        item.stateMask = LVIS_ALL;
        item.pszText = ach;
        item.cchTextMax = ARRAYSIZE(ach);
        ListView_OnGetItem(plv, &item);

        litem.pszText = item.pszText;
        ListView_GetRectsOwnerData(plv, i, &rcIcon, &rcLabel, &rcBounds, NULL, &litem);
        pitem = NULL;
    }
    else
    {
        pitem = ListView_GetItemPtr(plv, i);
        // NOTE this will do a GetItem LVIF_TEXT iff needed
        ListView_GetRects(plv, i, &rcIcon, &rcLabel, &rcBounds, NULL);
    }

    fUnfolded = FALSE;
    if ( (plvdi->flags & LVDI_UNFOLDED) || ListView_IsItemUnfolded(plv, i))
    {
        ListView_UnfoldRects(plv, i, &rcIcon, &rcLabel, &rcBounds, NULL );
        fUnfolded = TRUE;
    }

    if (!plvdi->prcClip || IntersectRect(&rcT, &rcBounds, plvdi->prcClip))
    {
        UINT fText;

        if (!ListView_IsOwnerData(plv))
        {
            item.iItem = i;
            item.iSubItem = 0;
            item.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
            item.stateMask = LVIS_ALL;
            item.pszText = ach;
            item.cchTextMax = ARRAYSIZE(ach);
            ListView_OnGetItem(plv, &item);
            
            // Make sure the listview hasn't been altered during
            // the callback to get the item info

            if (pitem != ListView_GetItemPtr(plv, i))
                return FALSE;
        }

        if (plvdi->lpptOrg)
        {
            OffsetRect(&rcIcon, plvdi->lpptOrg->x - rcBounds.left,
                                plvdi->lpptOrg->y - rcBounds.top);
            OffsetRect(&rcLabel, plvdi->lpptOrg->x - rcBounds.left,
                                plvdi->lpptOrg->y - rcBounds.top);
        }

        if (ListView_IsIconView(plv))
        {
            fText = ListView_DrawImage(plv, &item, plvdi->nmcd.nmcd.hdc,
                                       rcIcon.left + g_cxIconMargin, rcIcon.top + g_cyIconMargin, plvdi->flags);

            // If linebreaking needs to happen, then use SHDT_DRAWTEXT.
            // Otherwise, use our (hopefully faster) internal SHDT_ELLIPSES
            if (rcLabel.bottom - rcLabel.top > plv->cyLabelChar)
                fText |= SHDT_DRAWTEXT;
            else
                fText |= SHDT_ELLIPSES;

            // We only use DT_NOFULLWIDTHCHARBREAK on Korean(949) Memphis and NT5
            if (949 == g_uiACP && (g_bRunOnNT5 || g_bRunOnMemphis))
                fText |= SHDT_NODBCSBREAK;

        }
        else
        {
            fText = ListView_DrawImage(plv, &item, plvdi->nmcd.nmcd.hdc,
                                       rcIcon.left, rcIcon.top, plvdi->flags);


        }

        // Don't draw label if it's being edited...
        //
        if (plv->iEdit != i)
        {
            // If multiline label, then we need to use DrawText
            if (rcLabel.bottom - rcLabel.top > plv->cyLabelChar)
            {
                fText |= SHDT_DRAWTEXT;

                // If the text is folded, we need to clip and add ellipses

                if (!fUnfolded)
                    fText |= SHDT_CLIPPED | SHDT_DTELLIPSIS;

                if ( ListView_IsOwnerData(plv) )
                {
                    // If owner data, we have no z-order and if long names they will over lap each
                    // other, better to truncate for now...
                    if (ListView_IsSmallView(plv))
                        fText |= SHDT_ELLIPSES;
                }

            }
            else
                fText |= SHDT_ELLIPSES;

            if (plvdi->flags & LVDI_TRANSTEXT)
                fText |= SHDT_TRANSPARENT;

            if ((fText & SHDT_SELECTED) && (plvdi->flags & LVDI_HOTSELECTED))
                fText |= SHDT_HOTSELECTED;

            if (item.pszText && (*item.pszText))
            {
#ifdef WINDOWS_ME
                if(plv->dwExStyle & WS_EX_RTLREADING)
                    fText |= SHDT_RTLREADING;
#endif
                SHDrawText(plvdi->nmcd.nmcd.hdc, item.pszText, &rcLabel, LVCFMT_LEFT, fText,
                           plv->cyLabelChar, plv->cxEllipses,
                           plvdi->nmcd.clrText, plvdi->nmcd.clrTextBk);

                if ((plvdi->flags & LVDI_FOCUS) && (item.state & LVIS_FOCUSED)
#ifdef KEYBOARDCUES
                    && !(CCGetUIState(&(plvdi->plv->ci)) & UISF_HIDEFOCUS)
#endif
					)
                {
                    DrawFocusRect(plvdi->nmcd.nmcd.hdc, &rcLabel);
                }
            }
        }
    }
    return TRUE;
}

void ListView_RefoldLabelRect(LV* plv, RECT *prcLabel, LISTITEM *pitem)
{
    int bottom = pitem->cyUnfoldedLabel;
    bottom = min(bottom, pitem->cyFoldedLabel);
    bottom = min(bottom, CLIP_HEIGHT);
    prcLabel->bottom = prcLabel->top + bottom;
}

int NEAR ListView_IItemHitTest(LV* plv, int x, int y, UINT FAR* pflags, int *piSubItem)
{
    int iHit;
    UINT flags;
    POINT pt;
    RECT rcLabel;
    RECT rcIcon;
    RECT rcState;

    if (piSubItem)
        *piSubItem = 0;

    // Map window-relative coordinates to view-relative coords...
    //
    pt.x = x + plv->ptOrigin.x;
    pt.y = y + plv->ptOrigin.y;

    // If there are any uncomputed items, recompute them now.
    //
    if (plv->rcView.left == RECOMPUTE)
        ListView_Recompute(plv);

    flags = 0;

    if (ListView_IsOwnerData( plv ))
    {
        int cSlots;
        POINT ptWnd;
        LISTITEM item;

        cSlots = ListView_GetSlotCount( plv, TRUE );
        iHit = ListView_CalcHitSlot( plv, pt, cSlots );
        if (iHit < ListView_Count(plv))
        {
            ListView_IGetRectsOwnerData( plv, iHit, &rcIcon, &rcLabel, &item, FALSE );
            ptWnd.x = x;
            ptWnd.y = y;
            if (PtInRect(&rcIcon, ptWnd))
            {
                flags = LVHT_ONITEMICON;
            }
            else if (PtInRect(&rcLabel, ptWnd))
            {
                flags = LVHT_ONITEMLABEL;
            }
        }
    }
    else
    {
        for (iHit = 0; (iHit < ListView_Count(plv)); iHit++)
        {
            LISTITEM FAR* pitem = ListView_FastGetZItemPtr(plv, iHit);
            POINT ptItem;

            ptItem.x = pitem->pt.x;
            ptItem.y = pitem->pt.y;

            rcIcon.top    = ptItem.y - g_cyIconMargin;

            rcLabel.top    = ptItem.y + plv->cyIcon + g_cyLabelSpace;
            rcLabel.bottom = rcLabel.top + pitem->cyUnfoldedLabel;

            if ( !ListView_IsItemUnfoldedPtr(plv, pitem) )
                ListView_RefoldLabelRect(plv, &rcLabel, pitem);

            // Quick, easy rejection test...
            //
            if (pt.y < rcIcon.top || pt.y >= rcLabel.bottom)
                continue;

            rcIcon.left   = ptItem.x - g_cxIconMargin;
            rcIcon.right  = ptItem.x + plv->cxIcon + g_cxIconMargin;
            // We need to make sure there is no gap between the icon and label
            rcIcon.bottom = rcLabel.top;

            rcState.bottom = ptItem.y + plv->cyIcon;
            rcState.right = ptItem.x;
            rcState.top = rcState.bottom - plv->cyState;
            rcState.left = rcState.right - plv->cxState;

            rcLabel.left   = ptItem.x  + (plv->cxIcon / 2) - (ICONCXLABEL(plv, pitem) / 2);
            rcLabel.right  = rcLabel.left + ICONCXLABEL(plv, pitem);

            if (plv->cxState && PtInRect(&rcState, pt))
            {
                flags = LVHT_ONITEMSTATEICON;
            }
            else if (PtInRect(&rcIcon, pt))
            {
                flags = LVHT_ONITEMICON;
            }
            else if (PtInRect(&rcLabel, pt))
            {
                flags = LVHT_ONITEMLABEL;
            }
            
            if (flags)
                break;
        }
    }

    if (flags == 0)
    {
        flags = LVHT_NOWHERE;
        iHit = -1;
    }
    else
    {
      if (!ListView_IsOwnerData( plv ))
        {
        iHit = DPA_GetPtrIndex(plv->hdpa, ListView_FastGetZItemPtr(plv, iHit));
        }
    }

    *pflags = flags;
    return iHit;
}

// BUGBUG raymondc
// need to pass HDC here isnce it's sometimes called from the paint loop

void NEAR ListView_IGetRectsOwnerData( LV* plv,
        int iItem,
        RECT FAR* prcIcon,
        RECT FAR* prcLabel,
        LISTITEM* pitem,
        BOOL fUsepitem )
{
   int itemIconXLabel;
   int cSlots;

   // calculate x, y from iItem
   cSlots = ListView_GetSlotCount( plv, TRUE );
   pitem->iWorkArea = 0;               // OwnerData doesn't support workareas
   ListView_SetIconPos( plv, pitem, iItem, cSlots );

   // calculate lable sizes from iItem
   ListView_RecomputeLabelSize( plv, pitem, iItem, NULL, fUsepitem);

   if (plv->ci.style & LVS_NOLABELWRAP)
   {
      // use single label
      itemIconXLabel = pitem->cxSingleLabel;
   }
   else
   {
      // use multilabel
      itemIconXLabel = pitem->cxMultiLabel;
   }

    prcIcon->left   = pitem->pt.x - g_cxIconMargin - plv->ptOrigin.x;
    prcIcon->right  = prcIcon->left + plv->cxIcon + 2 * g_cxIconMargin;
    prcIcon->top    = pitem->pt.y - g_cyIconMargin - plv->ptOrigin.y;
    prcIcon->bottom = prcIcon->top + plv->cyIcon + 2 * g_cyIconMargin;

    prcLabel->left   = pitem->pt.x  + (plv->cxIcon / 2) - (itemIconXLabel / 2) - plv->ptOrigin.x;
    prcLabel->right  = prcLabel->left + itemIconXLabel;
    prcLabel->top    = pitem->pt.y  + plv->cyIcon + g_cyLabelSpace - plv->ptOrigin.y;
    prcLabel->bottom = prcLabel->top  + pitem->cyUnfoldedLabel;

    if ( !ListView_IsItemUnfolded(plv, iItem) )
        ListView_RefoldLabelRect(plv, prcLabel, pitem);
}


// out:
//      prcIcon         icon bounds including icon margin area

void NEAR ListView_IGetRects(LV* plv, LISTITEM FAR* pitem, RECT FAR* prcIcon, RECT FAR* prcLabel, LPRECT prcBounds)
{
    int cxIconMargin;

    ASSERT( !ListView_IsOwnerData( plv ) );

    if (pitem->pt.x == RECOMPUTE) {
        ListView_Recompute(plv);
    }

    if (ListView_IsIconView(plv) && ((plv->cxIconSpacing - plv->cxIcon) < (2 * g_cxIconMargin)))
        cxIconMargin = (plv->cxIconSpacing - plv->cxIcon) / 2;
    else
        cxIconMargin = g_cxIconMargin;

    prcIcon->left   = pitem->pt.x - cxIconMargin - plv->ptOrigin.x;
    prcIcon->right  = prcIcon->left + plv->cxIcon + 2 * cxIconMargin;
    prcIcon->top    = pitem->pt.y - g_cyIconMargin - plv->ptOrigin.y;
    prcIcon->bottom = prcIcon->top + plv->cyIcon + 2 * g_cyIconMargin;

    prcLabel->left   = pitem->pt.x  + (plv->cxIcon / 2) - (ICONCXLABEL(plv, pitem) / 2) - plv->ptOrigin.x;
    prcLabel->right  = prcLabel->left + ICONCXLABEL(plv, pitem);
    prcLabel->top    = pitem->pt.y  + plv->cyIcon + g_cyLabelSpace - plv->ptOrigin.y;
    prcLabel->bottom = prcLabel->top  + pitem->cyUnfoldedLabel;

    if ( !ListView_IsItemUnfoldedPtr(plv, pitem) )
        ListView_RefoldLabelRect(plv, prcLabel, pitem);

}

int NEAR ListView_GetSlotCountEx(LV* plv, BOOL fWithoutScrollbars, int iWorkArea)
{
    int cxScreen;
    int cyScreen;
    int dxItem;
    int dyItem;
    int iSlots = 1;
    BOOL fCheckWithScroll = FALSE;
    DWORD style = 0;

    // Always use the current client window size to determine
    //
    // REVIEW: Should we exclude any vertical scroll bar that may
    // exist when computing this?  progman.exe does not.
    //
    if ((iWorkArea >= 0 ) && (plv->nWorkAreas > 0))
    {
        ASSERT(iWorkArea < plv->nWorkAreas);
        cxScreen = RECTWIDTH(plv->prcWorkAreas[iWorkArea]);
        cyScreen = RECTHEIGHT(plv->prcWorkAreas[iWorkArea]);
    }
    else
    {
        cxScreen = plv->sizeClient.cx;
        cyScreen = plv->sizeClient.cy;
    }

    if (fWithoutScrollbars)
    {
        style = ListView_GetWindowStyle(plv);

        if (style & WS_VSCROLL)
        {
            cxScreen += ListView_GetCxScrollbar(plv);
        }
        if (style & WS_HSCROLL)
        {
            cyScreen += ListView_GetCyScrollbar(plv);
        }
    }

    if (ListView_IsSmallView(plv))
        dxItem = plv->cxItem;
    else
        dxItem = lv_cxIconSpacing;

    if (ListView_IsSmallView(plv))
        dyItem = plv->cyItem;
    else
        dyItem = lv_cyIconSpacing;

    if (!dxItem)
        dxItem = 1;
    if (!dyItem)
        dyItem = 1;

    // Lets see which direction the view states
    switch (plv->ci.style & LVS_ALIGNMASK)
    {
    case LVS_ALIGNBOTTOM:
    case LVS_ALIGNTOP:
#ifndef UNIX
        iSlots = max(1, (cxScreen) / dxItem);
#else
        iSlots = max(1, (cxScreen) /  max(1,dxItem));
#endif
        fCheckWithScroll = (BOOL)(style & WS_VSCROLL);
        break;

    case LVS_ALIGNRIGHT:
    case LVS_ALIGNLEFT:
#ifndef UNIX
        iSlots = max(1, (cyScreen) / dyItem);
#else
        iSlots = max(1, (cyScreen) / max(1,dyItem));
#endif
        fCheckWithScroll = (BOOL)(style & WS_HSCROLL);
        break;

    default:
        ASSERT(0);
        return 1;
    }

    // if we don't have enough slots total on the screen, we're going to have
    // a scrollbar, so recompute with the scrollbars on
    if (fWithoutScrollbars && fCheckWithScroll) {
        int iTotalSlots = (dxItem * dyItem);
        if (iTotalSlots < ListView_Count(plv)) {
            iSlots = ListView_GetSlotCountEx(plv, FALSE, iWorkArea);
        }

    }

    return iSlots;
}


int NEAR ListView_GetSlotCount(LV* plv, BOOL fWithoutScrollbars)
{
    // Make sure this function does exactly the same thing as when
    // we had no workareas
    return ListView_GetSlotCountEx(plv, fWithoutScrollbars, -1);
}

// get the pixel row (or col in left align) of pitem
int LV_GetItemPixelRow(LV* plv, LISTITEM* pitem)
{
    if ((plv->ci.style & LVS_ALIGNMASK) == LVS_ALIGNLEFT) {
        return pitem->pt.x;
    } else {
        return pitem->pt.y;
    }
}

// get the pixel row (or col in left align) of the lowest item
int LV_GetMaxPlacedItem(LV* plv)
{
    int i;
    int iMaxPlacedItem = 0;
    
    for (i = 0; i < ListView_Count(plv); i++) {
        LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);
        if (pitem->pt.y != RECOMPUTE) {
            int iRow = LV_GetItemPixelRow(plv, pitem);
            // if the current item is "below" (on right if it's left aligned)
            // the lowest placed item, we can start appending
            if (!i || iRow > iMaxPlacedItem)
                iMaxPlacedItem = iRow;
        }
    }
    
    return iMaxPlacedItem;;
}

// Go through and recompute any icon positions and optionally
// icon label dimensions.
//
// This function also recomputes the view bounds rectangle.
//
// The algorithm is to simply search the list for any items needing
// recomputation.  For icon positions, we scan possible icon slots
// and check to see if any already-positioned icon intersects the slot.
// If not, the slot is free.  As an optimization, we start scanning
// icon slots from the previous slot we found.
//
void NEAR ListView_Recompute(LV* plv)
{
    int i;
    int cSlots;
    int cWorkAreaSlots[LV_MAX_WORKAREAS];
    BOOL fUpdateSB;
    // if all the items are unplaced, we can just keep appending
    BOOL fAppendAtEnd = (((UINT)ListView_Count(plv)) == plv->uUnplaced);
    int iFree;

    plv->uUnplaced = 0;

    if (!(ListView_IsIconView(plv) || ListView_IsSmallView(plv)))
        return;

    if (plv->flags & LVF_INRECOMPUTE)
    {
        return;
    }
    plv->flags |= LVF_INRECOMPUTE;

    cSlots = ListView_GetSlotCount(plv, FALSE);

    if (plv->nWorkAreas > 0)
        for (i = 0; i < plv->nWorkAreas; i++)
            cWorkAreaSlots[i] = ListView_GetSlotCountEx(plv, FALSE, i);

    // Scan all items for RECOMPUTE, and recompute slot if needed.
    //
    fUpdateSB = (plv->rcView.left == RECOMPUTE);

    if (!ListView_IsOwnerData( plv ))
    {
        LVFAKEDRAW lvfd;                    // in case client uses customdraw
        LV_ITEM item;                       // in case client uses customdraw
        int iMaxPlacedItem = RECOMPUTE;

        item.mask = LVIF_PARAM;
        item.iSubItem = 0;

        ListView_BeginFakeCustomDraw(plv, &lvfd, &item);

        if (!fAppendAtEnd)
            iMaxPlacedItem = LV_GetMaxPlacedItem(plv);

        // Must keep in local variable because ListView_SetIconPos will keep
        // invalidating the iFreeSlot cache while we're looping
        iFree = plv->iFreeSlot;
        for (i = 0; i < ListView_Count(plv); i++)
        {
            int cRealSlots;
            LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);
            BOOL fRedraw = FALSE;
            cRealSlots = (plv->nWorkAreas > 0) ? cWorkAreaSlots[pitem->iWorkArea] : cSlots;
            if (pitem->pt.y == RECOMPUTE)
            {
                if (pitem->cyFoldedLabel == SRECOMPUTE)
                {
                    // Get the item lParam only if we need it for customdraw
                    item.iItem = i;
                    item.lParam = pitem->lParam;

                    ListView_BeginFakeItemDraw(&lvfd);
                    ListView_RecomputeLabelSize(plv, pitem, i, lvfd.nmcd.nmcd.hdc, FALSE);
                    ListView_EndFakeItemDraw(&lvfd);
                }
                // BUGBUG: (dli) This function gets a new icon postion and then goes 
                // through the whole set of items to see if that position is occupied
                // should let it know in the multi-workarea case, it only needs to go
                // through those who are in the same workarea.
                // This is okay for now because we cannot have too many items on the
                // desktop. 
                iFree = ListView_FindFreeSlot(plv, i, iFree + 1, cRealSlots, &fUpdateSB, &fAppendAtEnd, lvfd.nmcd.nmcd.hdc);
                ASSERT(iFree != -1);

                ListView_SetIconPos(plv, pitem, iFree, cRealSlots);

                if (!fAppendAtEnd) {
                    //// optimization.  each time we calc a new free slot, we iterate through all the items to see
                    // if any of the freely placed items collide with this.
                    // fAppendAtEnd indicates that iFree is beyond any freely placed item
                    // 
                    // if the current item is "below" (on right if it's left aligned)
                    // the lowest placed item, we can start appending
                    if (LV_GetItemPixelRow(plv, pitem) > iMaxPlacedItem)
                        fAppendAtEnd = TRUE;
                }
                
                if (!fUpdateSB && LV_IsItemOnViewEdge(plv, pitem))
                    fUpdateSB = TRUE;

                fRedraw = TRUE;
            }

            if (fRedraw)
            {
                ListView_InvalidateItem(plv, i, FALSE, RDW_INVALIDATE | RDW_ERASE);
            }
        }
        plv->iFreeSlot = iFree;
        ListView_EndFakeCustomDraw(&lvfd);

    }
    // If we changed something, recompute the view rectangle
    // and then update the scroll bars.
    //
    if (fUpdateSB || plv->rcView.left == RECOMPUTE )
    {

        TraceMsg(TF_GENERAL, "************ LV: Expensive update! ******* ");

        // NOTE: No infinite recursion results because we're setting
        // plv->rcView.left != RECOMPUTE
        //
        SetRectEmpty(&plv->rcView);

        if (ListView_IsOwnerData( plv ))
        {
           if (ListView_Count( plv ) > 0)
           {
              RECT  rcLast;
              RECT  rcItem;
              int iSlots;
              int   iItem = ListView_Count( plv ) - 1;

              ListView_GetRects( plv, 0, NULL, NULL, &plv->rcView, NULL );
              ListView_GetRects( plv, iItem, NULL, NULL, &rcLast, NULL );
              plv->rcView.right = rcLast.right;
              plv->rcView.bottom = rcLast.bottom;

              //
              // calc how far back in the list to check
              //
              iSlots = cSlots + 2;
               
              // REVIEW:  This cache hint notification causes a spurious
              //  hint, since this happens often but is always the last items
              //  available.  Should this hint be done at all and this information
              //  be cached local to the control?
              ListView_NotifyCacheHint( plv, max( 0, iItem - iSlots), iItem );
               
              // move backwards from last item until either rc.right or
              // rc.left is greater than the last, then use that value.
              // Note: This code makes very little assumptions about the ordering
              // done.  We should be careful as multiple line text fields could
              // screw us up.
              for( iItem--;
                  (iSlots > 0) && (iItem >= 0);
                  iSlots--, iItem--)
              {
                  RECT rcIcon;
                  RECT rcLabel;
                  
                  ListView_GetRects( plv, iItem, &rcIcon, &rcLabel, &rcItem, NULL );
                  ListView_UnfoldRects( plv, iItem, &rcIcon, &rcLabel, &rcItem, NULL );
                  if (rcItem.right > rcLast.right)
                  {
                      plv->rcView.right =  rcItem.right;
                  }
                  if (rcItem.bottom > rcLast.bottom)
                  {
                      plv->rcView.bottom = rcItem.bottom;
                  }
              }
           }
        }
        else
        {
            for (i = 0; i < ListView_Count(plv); i++)
            {
                RECT rcIcon;
                RECT rcLabel;
                RECT rcItem;

                ListView_GetRects(plv, i, &rcIcon, &rcLabel, &rcItem, NULL);
                ListView_UnfoldRects(plv, i, &rcIcon, &rcLabel, &rcItem, NULL);
                UnionRect(&plv->rcView, &plv->rcView, &rcItem);
            }
        }
        // add a little space at the edges so that we don't bump text
        // completely to the end of the window
        plv->rcView.bottom += g_cyEdge;
        plv->rcView.right += g_cxEdge;

        OffsetRect(&plv->rcView, plv->ptOrigin.x, plv->ptOrigin.y);
        //TraceMsg(DM_TRACE, "RECOMPUTE: rcView %x %x %x %x", plv->rcView.left, plv->rcView.top, plv->rcView.right, plv->rcView.bottom);
        //TraceMsg(DM_TRACE, "Origin %x %x", plv->ptOrigin.x, plv->ptOrigin.y);

        ListView_UpdateScrollBars(plv);
    }
    ListView_RecalcRegion(plv, FALSE, TRUE);
    // Now state we are out of the recompute...
    plv->flags &= ~LVF_INRECOMPUTE;
}

void NEAR PASCAL NearestSlot(int FAR *x, int FAR *y, int cxItem, int cyItem, LPRECT prcWork)
{
    if (prcWork != NULL)
    {
        *x = *x - prcWork->left;
        *y = *y - prcWork->top;
    }
    
    if (*x < 0)
        *x -= cxItem/2;
    else
        *x += cxItem/2;

    if (*y < 0)
        *y -= cyItem/2;
    else
        *y += cyItem/2;

    *x = *x - (*x % cxItem);
    *y = *y - (*y % cyItem);

    if (prcWork != NULL)
    {
        *x = *x + prcWork->left;
        *y = *y + prcWork->top;
    }
}


void NEAR PASCAL NextSlot(LV* plv, LPRECT lprc)
{
    int cxItem;

    if (ListView_IsSmallView(plv))
    {
        cxItem = plv->cxItem;
    }
    else
    {
        cxItem = lv_cxIconSpacing;
    }
    lprc->left += cxItem;
    lprc->right += cxItem;
}


//-------------------------------------------------------------------
//
//-------------------------------------------------------------------

void ListView_CalcMinMaxIndex( LV* plv, PRECT prcBounding, int* iMin, int* iMax )
{
   POINT pt;
   int cSlots;

   cSlots = ListView_GetSlotCount( plv, TRUE );

   pt.x = prcBounding->left + plv->ptOrigin.x;
   pt.y = prcBounding->top + plv->ptOrigin.y;
   *iMin = ListView_CalcHitSlot( plv, pt, cSlots );

   pt.x = prcBounding->right + plv->ptOrigin.x;
   pt.y = prcBounding->bottom + plv->ptOrigin.y;
   *iMax = ListView_CalcHitSlot( plv, pt, cSlots ) + 1;
}
//-------------------------------------------------------------------
//
// Function: ListView_CalcHitSlot
//
// Summary: Given a point (relative to complete icon view), calculate
//    which slot that point is closest to.
//
// Arguments:
//    plv [in] - List view to work with
//    pt [in]  - location to check with
//    cslot [in]  - number of slots wide the current view is
//
// Notes: This does not guarentee that the point is hitting the item
//    located at that slot.  That should be checked by comparing rects.
//
// History:
//    Nov-1-1994  MikeMi   Added to improve Ownerdata hit testing
//
//-------------------------------------------------------------------

int ListView_CalcHitSlot( LV* plv, POINT pt, int cSlot )
{
    int cxItem;
    int cyItem;
   int iSlot = 0;

    ASSERT(plv);

    if (cSlot < 1)
        cSlot = 1;

    if (ListView_IsSmallView(plv))
    {
        cxItem = plv->cxItem;
        cyItem = plv->cyItem;
    }
    else
    {
        cxItem = lv_cxIconSpacing;
        cyItem = lv_cyIconSpacing;
    }

    // Lets see which direction the view states
    switch (plv->ci.style & LVS_ALIGNMASK)
    {
    case LVS_ALIGNBOTTOM:
        // Assert False (Change default in shell2d.. to ALIGN_TOP)

    case LVS_ALIGNTOP:
      iSlot = (pt.x / cxItem) + (pt.y / cyItem) * cSlot;
      break;

    case LVS_ALIGNLEFT:
      iSlot = (pt.x / cxItem) * cSlot + (pt.y / cyItem);
      break;

    case LVS_ALIGNRIGHT:
        ASSERT(FALSE);      // Not implemented yet...
        break;
    }

    return( iSlot );
}

void _GetCurrentItemSize(LV* plv, int * pcx, int *pcy)
{
    if (ListView_IsSmallView(plv))
    {
        *pcx = plv->cxItem;
        *pcy = plv->cyItem;
    }
    else
    {
        *pcx = lv_cxIconSpacing;
        *pcy = lv_cyIconSpacing;
    }
}

DWORD ListView_IApproximateViewRect(LV* plv, int iCount, int iWidth, int iHeight)
{
    int cxSave = plv->sizeClient.cx;
    int cySave = plv->sizeClient.cy;
    int cxItem;
    int cyItem;
    int cCols;
    int cRows;

    plv->sizeClient.cx = iWidth;
    plv->sizeClient.cy = iHeight;
    cCols = ListView_GetSlotCount(plv, TRUE);

    plv->sizeClient.cx = cxSave;
    plv->sizeClient.cy = cySave;

    cCols = min(cCols, iCount);
    if (cCols == 0)
        cCols = 1;
    cRows = (iCount + cCols - 1) / cCols;

    if (plv->ci.style & (LVS_ALIGNLEFT | LVS_ALIGNRIGHT)) {
        int c;

        c = cCols;
        cCols = cRows;
        cRows = c;
    }

    _GetCurrentItemSize(plv, &cxItem, &cyItem);

    iWidth = cCols * cxItem;
    iHeight = cRows * cyItem;

    return MAKELONG(iWidth + g_cxEdge, iHeight + g_cyEdge);
}


void NEAR _CalcSlotRect(LV* plv, LISTITEM *pItem, int iSlot, int cSlot, BOOL fBias, LPRECT lprc)
{
    int cxItem, cyItem;

    ASSERT(plv);

    if (cSlot < 1)
        cSlot = 1;

    _GetCurrentItemSize(plv, &cxItem, &cyItem);

    // Lets see which direction the view states
    switch (plv->ci.style & LVS_ALIGNMASK)
    {
    case LVS_ALIGNBOTTOM:
        // Assert False (Change default in shell2d.. to ALIGN_TOP)

    case LVS_ALIGNTOP:
        lprc->left = (iSlot % cSlot) * cxItem;
        lprc->top = (iSlot / cSlot) * cyItem;
        break;

    case LVS_ALIGNRIGHT:
        RIPMSG(0, "LVM_ARRANGE: Invalid listview icon arrangement style");
        // ASSERT(FALSE);    // Not implemented yet...
        // fall through, use LVS_ALIGNLEFT instead

    case LVS_ALIGNLEFT:
        lprc->top = (iSlot % cSlot) * cyItem;
        lprc->left = (iSlot / cSlot) * cxItem;
        break;

    }

    if (fBias)
    {
        lprc->left -= plv->ptOrigin.x;
        lprc->top -= plv->ptOrigin.y;
    }
    lprc->bottom = lprc->top + cyItem;
    lprc->right = lprc->left + cxItem;
    
    // Multi-Workarea case offset from the workarea coordinate to the whole
    // listview coordinate. 
    if (plv->nWorkAreas > 0)
    {
        ASSERT(pItem);
        ASSERT(pItem->iWorkArea < plv->nWorkAreas);
        OffsetRect(lprc, plv->prcWorkAreas[pItem->iWorkArea].left, plv->prcWorkAreas[pItem->iWorkArea].top);
    }
}

// Intersect this rectangle with all items in this listview except myself,
// this will determine if this rectangle overlays any icons. 
BOOL NEAR ListView_IsCleanRect(LV * plv, RECT * prc, int iExcept, BOOL * pfUpdate, HDC hdc)
{
    int j;
    RECT rc;
    int cItems = ListView_Count(plv);
    for (j = cItems; j-- > 0; )
    {
        if (j == iExcept)
            continue;
        else
        {
            LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, j);
            if (pitem->pt.y != RECOMPUTE)
            {
                // If the dimensions aren't computed, then do it now.
                //
                if (pitem->cyFoldedLabel == SRECOMPUTE)
                {
                    ListView_RecomputeLabelSize(plv, pitem, j, hdc, FALSE);
                    
                    // Ensure that the item gets redrawn...
                    //
                    ListView_InvalidateItem(plv, j, FALSE, RDW_INVALIDATE | RDW_ERASE);
                    
                    // Set flag indicating that scroll bars need to be
                    // adjusted.
                    //
                    if (LV_IsItemOnViewEdge(plv, pitem))
                        *pfUpdate = TRUE;
                }
                
                
                ListView_GetRects(plv, j, NULL, NULL, &rc, NULL);
                
                if (IntersectRect(&rc, &rc, prc))
                    return FALSE;
            }
        }
    }
    
    return TRUE;
}       

// Find an icon slot that doesn't intersect an icon.
// Start search for free slot from slot i.
//
int NEAR ListView_FindFreeSlot(LV* plv, int iItem, int i, int cSlot, BOOL FAR* pfUpdate,
        BOOL FAR *pfAppend, HDC hdc)
{
    RECT rcSlot;
    RECT rcItem;
    RECT rc;
    LISTITEM FAR * pItemLooking = ListView_FastGetItemPtr(plv, iItem);

    ASSERT(!ListView_IsOwnerData( plv ));

    // Horrible N-squared algorithm:
    // enumerate each slot and see if any items intersect it.
    //
    // REVIEW: This is really slow with long lists (e.g., 1000)
    //

    //
    // If the Append at end is set, we should be able to simply get the
    // rectangle of the i-1 element and check against it instead of
    // looking at every other item...
    //
    if (*pfAppend)
    {
        int iPrev = iItem - 1;
        // Be carefull about going of the end of the list. (i is a slot
        // number not an item index).
        
        if (plv->nWorkAreas > 0)
        {
            while (iPrev >= 0)
            {
                LISTITEM FAR * pPrev = ListView_FastGetItemPtr(plv, iPrev);
                if (pPrev->iWorkArea == pItemLooking->iWorkArea)
                    break;	
                iPrev--;
            }
        }
        
        if (iPrev >= 0)
            ListView_GetRects(plv, iPrev, NULL, NULL, &rcItem, NULL);
        else
            SetRect(&rcItem, 0, 0, 0, 0);
    }

    for ( ; ; i++)
    {
        // Compute view-relative slot rectangle...
        //
        _CalcSlotRect(plv, pItemLooking, i, cSlot, TRUE, &rcSlot);

        if (*pfAppend)
        {
            if (!IntersectRect(&rc, &rcItem, &rcSlot)) {
                return i;       // Found a free slot...
            }
        }
        
        if (ListView_IsCleanRect(plv, &rcSlot, iItem, pfUpdate, hdc))
            break;
    }

    return i;
}

// Recompute an item's label size (cxLabel/cyLabel).  For speed, this function
// is passed a DC to use for text measurement.
//
// If hdc is NULL, then this function will create and initialize a temporary
// DC, then destroy it.  If hdc is non-NULL, then it is assumed to have
// the correct font already selected into it.
//
// fUsepitem means not to use the text of the item.  Instead, use the text
// pointed to by the pitem structure.  This is used in two cases.
//
//  -   Ownerdata, because we don't have a real pitem.
//  -   Regulardata, where we already found the pitem text (as an optimizatin)
//
void NEAR ListView_RecomputeLabelSize(LV* plv, LISTITEM FAR* pitem, int i, HDC hdc, BOOL fUsepitem)
{
    TCHAR szLabel[CCHLABELMAX + 4];
    TCHAR szLabelFolded[ARRAYSIZE(szLabel) + CCHELLIPSES + CCHELLIPSES];
    int cchLabel;
    RECT rcSingle, rcFolded, rcUnfolded;
    LVFAKEDRAW lvfd;
    LV_ITEM item;

    ASSERT(plv);

    // the following will use the passed in pitem text instead of calling
    // GetItem.  This would be two consecutive calls otherwise, in some cases.
    //
    if (fUsepitem && (pitem->pszText != LPSTR_TEXTCALLBACK))
    {
        Str_GetPtr0(pitem->pszText, szLabel, ARRAYSIZE(szLabel));
        item.lParam = pitem->lParam;
    }
    else
    {
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = szLabel;
        item.cchTextMax = ARRAYSIZE(szLabel);
        item.stateMask = 0;
        szLabel[0] = TEXT('\0');    // In case the OnGetItem fails
        ListView_OnGetItem(plv, &item);

        if (!item.pszText)
        {
            SetRectEmpty(&rcSingle);
            rcFolded = rcSingle;
            rcUnfolded = rcSingle;
            goto Exit;
        }

        if (item.pszText != szLabel)
            lstrcpyn(szLabel, item.pszText, ARRAYSIZE(szLabel));
    }

    cchLabel = lstrlen(szLabel);

    rcUnfolded.left = rcUnfolded.top = rcUnfolded.bottom = 0;
    rcUnfolded.right = lv_cxIconSpacing - g_cxLabelMargin * 2;
    rcSingle = rcUnfolded;
    rcFolded = rcUnfolded;

    if (cchLabel > 0)
    {
        UINT flags;

        if (!hdc) {                             // Set up fake customdraw
            ListView_BeginFakeCustomDraw(plv, &lvfd, &item);
            ListView_BeginFakeItemDraw(&lvfd);
        } else
            lvfd.nmcd.nmcd.hdc = hdc;           // Use the one the app gave us

#if 0   // Don't do this - Stripping trailing spaces results in unwanted ellipses
        // Strip off spaces so they're not included in format
        // REVIEW: Is this is a DrawText bug?
        //
        while (cchLabel > 1 && szLabel[cchLabel - 1] == TEXT(' '))
            szLabel[--cchLabel] = 0;
#endif

        DrawText(lvfd.nmcd.nmcd.hdc, szLabel, cchLabel, &rcSingle, (DT_LV | DT_CALCRECT));

        if (plv->ci.style & LVS_NOLABELWRAP) {
            flags = DT_LV | DT_CALCRECT;
        } else {
            flags = DT_LVWRAP | DT_CALCRECT;
            // We only use DT_NOFULLWIDTHCHARBREAK on Korean(949) Memphis and NT5
            if (949 == g_uiACP && (g_bRunOnNT5 || g_bRunOnMemphis))
                flags |= DT_NOFULLWIDTHCHARBREAK;
        }

        DrawText(lvfd.nmcd.nmcd.hdc, szLabel, cchLabel, &rcUnfolded, flags);

        //
        //  DrawText with DT_MODIFYSTRING is quirky when you enable
        //  word ellipses.  Once it finds anything that requires ellipses,
        //  it stops and doesn't return anything else (even if those other
        //  things got displayed).
        //
        lstrcpy(szLabelFolded, szLabel);
        DrawText(lvfd.nmcd.nmcd.hdc, szLabelFolded, cchLabel, &rcFolded, flags | DT_WORD_ELLIPSIS | DT_MODIFYSTRING);

        //  If we had to ellipsify, but you can't tell from looking at the
        //  rcFolded.bottom and rcUnfolded.bottom, then tweak rcFolded.bottom
        //  so the unfoldifier knows that unfolding is worthwhile.
        if (rcFolded.bottom == rcUnfolded.bottom &&
            lstrcmp(szLabel, szLabelFolded))
        {
            // The actual value isn't important, as long as it's greater
            // than rcUnfolded.bottom and CLIP_HEIGHT.  We take advantage
            // of the fact that CLIP_HEIGHT is only two lines, so the only
            // problem case is where you have a two-line item and only the
            // first line is ellipsified.
            rcFolded.bottom++;
        }

        if (!hdc) {                             // Clean up fake customdraw
            ListView_EndFakeItemDraw(&lvfd);
            ListView_EndFakeCustomDraw(&lvfd);
        }

    }
    else
    {
        rcFolded.bottom = rcUnfolded.bottom = rcUnfolded.top + plv->cyLabelChar;
    }

Exit:

    if (pitem) {
        int cyEdge;
        pitem->cxSingleLabel = (short)((rcSingle.right - rcSingle.left) + 2 * g_cxLabelMargin);
        pitem->cxMultiLabel = (short)((rcUnfolded.right - rcUnfolded.left) + 2 * g_cxLabelMargin);

        cyEdge = (plv->ci.style & LVS_NOLABELWRAP) ? 0 : g_cyEdge;

        pitem->cyFoldedLabel = (short)((rcFolded.bottom - rcFolded.top) + cyEdge);
        pitem->cyUnfoldedLabel = (short)((rcUnfolded.bottom - rcUnfolded.top) + cyEdge);
    }

}

// Set up an icon slot position.  Returns FALSE if position didn't change.
//
BOOL NEAR ListView_SetIconPos(LV* plv, LISTITEM FAR* pitem, int iSlot, int cSlot)
{
    RECT rc;

    ASSERT(plv);

    //
    // Sort of a hack, this internal function return TRUE if small icon.

    _CalcSlotRect(plv, pitem, iSlot, cSlot, FALSE, &rc);

    if (ListView_IsIconView(plv))
    {
        rc.left += ((lv_cxIconSpacing - plv->cxIcon) / 2);
        rc.top += g_cyIconOffset;
    }
    
    if (rc.left != pitem->pt.x || rc.top != pitem->pt.y)
    {
        LV_AdjustViewRectOnMove(plv, pitem, rc.left, rc.top);

        return TRUE;
    }
    return FALSE;
}

void NEAR ListView_GetViewRect2(LV* plv, RECT FAR* prcView, int cx, int cy)
{

    if (plv->rcView.left == RECOMPUTE)
        ListView_Recompute(plv);

    *prcView = plv->rcView;

    //
    // Offsets for scrolling.
    //
    OffsetRect(prcView, -plv->ptOrigin.x, -plv->ptOrigin.y);

    if (ListView_IsIconView(plv) || ListView_IsSmallView(plv))
    {
        //  don't do that funky half-re-origining thing.

        RECT rc;

        rc.left = 0;
        rc.top = 0;
        rc.right = cx;
        rc.bottom = cy;
        UnionRect(prcView, prcView, &rc);

#if 0
        // if we're scrolled way in the positive area (plv->ptOrigin > 0), make sure we
        // include our true origin
        if ((prcView->left > -plv->ptOrigin.x))
            prcView->left = -plv->ptOrigin.x;
        if ((prcView->top > -plv->ptOrigin.y))
            prcView->top = -plv->ptOrigin.y;

        // if we're scrolled way in the positive area (plv->ptOrigin > 0),
        // make sure our scrollbars include our current position
        if ((prcView->right < (plv->sizeClient.cx)))
            prcView->right = plv->sizeClient.cx;
        if ((prcView->bottom < (plv->sizeClient.cy)))
            prcView->bottom = plv->sizeClient.cy;
#endif
    }
}

// prcViewRect used only if fSubScroll is TRUE
DWORD NEAR ListView_GetClientRect(LV* plv, RECT FAR* prcClient, BOOL fSubScroll, RECT FAR *prcViewRect)
{
    RECT rcClient;
    RECT rcView;
    DWORD style;

#if 1
    // do this instead of the #else below because
    // in new versus old apps, you may need to add in g_c?Border because of
    // the one pixel overlap...
    GetWindowRect(plv->ci.hwnd, &rcClient);
    if (GetWindowLong(plv->ci.hwnd, GWL_EXSTYLE) & (WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE)) {
        rcClient.right -= 2 * g_cxEdge;
        rcClient.bottom -= 2 * g_cyEdge;
    }
    rcClient.right -= rcClient.left;
    rcClient.bottom -= rcClient.top;
    if (rcClient.right < 0)
        rcClient.right = 0;
    if (rcClient.bottom < 0)
        rcClient.bottom = 0;
    rcClient.top = rcClient.left = 0;
#else
    style = ListView_GetWindowStyle(plv);
    GetClientRect(plv->ci.hwnd, &rcClient);
    if (style & WS_VSCROLL)
        rcClient.right += ListView_GetCxScrollbar(plv);
    if (style & WS_HSCROLL)
        rcClient.bottom += ListView_GetCyScrollbar(plv);
#endif

    style = 0L;
    if (fSubScroll)
    {
        ListView_GetViewRect2(plv, &rcView, 0, 0);
        if ((rcClient.left < rcClient.right) && (rcClient.top < rcClient.bottom))
        {
            do
            {
                if (!(style & WS_HSCROLL) &&
                    (rcView.left < rcClient.left || rcView.right > rcClient.right))
                {
                    style |= WS_HSCROLL;
                    rcClient.bottom -= ListView_GetCyScrollbar(plv); // BUGBUG what if not SB yet?
                }
                if (!(style & WS_VSCROLL) &&
                    (rcView.top < rcClient.top || rcView.bottom > rcClient.bottom))
                {
                    style |= WS_VSCROLL;
                    rcClient.right -= ListView_GetCxScrollbar(plv);
                }
            }
            while (!(style & WS_HSCROLL) && rcView.right > rcClient.right);
        }
        if (prcViewRect)
            *prcViewRect = rcView;
    }
    *prcClient = rcClient;
    return style;
}

int CALLBACK ArrangeIconCompare(LISTITEM FAR* pitem1, LISTITEM FAR* pitem2, LPARAM lParam)
{
    int v1, v2;

    // REVIEW: lParam can be 0 and we fault ... bug in caller, but we might want to be robust here.

    if (HIWORD(lParam))
    {
        // Vertical arrange
        v1 = pitem1->pt.x / GET_X_LPARAM(lParam);
        v2 = pitem2->pt.x / GET_X_LPARAM(lParam);

        if (v1 > v2)
            return 1;
        else if (v1 < v2)
            return -1;
        else
        {
            int y1 = pitem1->pt.y;
            int y2 = pitem2->pt.y;

            if (y1 > y2)
                return 1;
            else if (y1 < y2)
                return -1;
        }

    }
    else
    {
        v1 = pitem1->pt.y / (int)lParam;
        v2 = pitem2->pt.y / (int)lParam;

        if (v1 > v2)
            return 1;
        else if (v1 < v2)
            return -1;
        else
        {
            int x1 = pitem1->pt.x;
            int x2 = pitem2->pt.x;

            if (x1 > x2)
                return 1;
            else if (x1 < x2)
                return -1;
        }
    }
    return 0;
}

void NEAR PASCAL _ListView_GetRectsFromItem(LV* plv, BOOL bSmallIconView,
                                            LISTITEM FAR *pitem,
                                            LPRECT prcIcon, LPRECT prcLabel, LPRECT prcBounds, LPRECT prcSelectBounds)
{
    RECT rcIcon;
    RECT rcLabel;

    if (!prcIcon)
        prcIcon = &rcIcon;
    if (!prcLabel)
        prcLabel = &rcLabel;

    // Test for NULL item passed in
    if (pitem)
    {
        // This routine is called during ListView_Recompute(), while
        // plv->rcView.left may still be == RECOMPUTE.  So, we can't
        // test that to see if recomputation is needed.
        //
        if (pitem->pt.y == RECOMPUTE || pitem->cyFoldedLabel == SRECOMPUTE)
            ListView_Recompute(plv);

        if (bSmallIconView)
            ListView_SGetRects(plv, pitem, prcIcon, prcLabel, prcBounds);
        else
            // ListView_IGetRects already refolds as necessary
            ListView_IGetRects(plv, pitem, prcIcon, prcLabel, prcBounds);

        if (prcBounds)
        {
            UnionRect(prcBounds, prcIcon, prcLabel);
            if (plv->himlState && (LV_StateImageValue(pitem)))
            {
                prcBounds->left -= plv->cxState;
            }
        }

    } else {
        SetRectEmpty(prcIcon);
        *prcLabel = *prcIcon;
        if (prcBounds)
            *prcBounds = *prcIcon;
    }

    if (prcSelectBounds)
    {
        UnionRect(prcSelectBounds, prcIcon, prcLabel);
    }
}

void NEAR _ListView_InvalidateItemPtr(LV* plv, BOOL bSmallIcon, LISTITEM FAR *pitem, UINT fRedraw)
{
    RECT rcBounds;

    ASSERT( !ListView_IsOwnerData( plv ));

    _ListView_GetRectsFromItem(plv, bSmallIcon, pitem, NULL, NULL, &rcBounds, NULL);
    RedrawWindow(plv->ci.hwnd, &rcBounds, NULL, fRedraw);
}

// return TRUE if things still overlap
// this only happens if we tried to unstack things, and there was NOSCROLL set and
// items tried to go off the deep end
BOOL NEAR PASCAL ListView_IUnstackOverlaps(LV* plv, HDPA hdpaSort, int iDirection)
{
    BOOL fRet = FALSE;
    int i;
    int iCount;
    BOOL bSmallIconView;
    RECT rcItem, rcItem2, rcTemp;
    int cxItem, cyItem;
    LISTITEM FAR* pitem;
    LISTITEM FAR* pitem2;

    ASSERT( !ListView_IsOwnerData( plv ) );

    if (bSmallIconView = ListView_IsSmallView(plv))
    {
        cxItem = plv->cxItem;
        cyItem = plv->cyItem;
    }
    else
    {
        cxItem = lv_cxIconSpacing;
        cyItem = lv_cyIconSpacing;
    }
    iCount = ListView_Count(plv);

    // finally, unstack any overlaps
    for (i = 0 ; i < iCount ; i++) {
        int j;
        pitem = DPA_GetPtr(hdpaSort, i);

        if (bSmallIconView) {
            _ListView_GetRectsFromItem(plv, bSmallIconView, pitem, NULL, NULL, &rcItem, NULL);
        }

        // move all the items that overlap with us
        for (j = i+1 ; j < iCount; j++) {
            POINT ptOldPos;

            pitem2 = DPA_GetPtr(hdpaSort, j);
            ptOldPos = pitem2->pt;

            if (bSmallIconView) {

                // for small icons, we need to do an intersect rect
                _ListView_GetRectsFromItem(plv, bSmallIconView, pitem2, NULL, NULL, &rcItem2, NULL);

                if (IntersectRect(&rcTemp, &rcItem, &rcItem2)) {
                    // yes, it intersects.  move it out
                    _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem2, RDW_INVALIDATE| RDW_ERASE);
                    do {
                        pitem2->pt.x += (cxItem * iDirection);
                    } while (PtInRect(&rcItem, pitem2->pt));
                } else {
                    // no more intersect!
                    break;
                }

            } else {
                // for large icons, just find the ones that share the x,y;
                if (pitem2->pt.x == pitem->pt.x && pitem2->pt.y == pitem->pt.y) {

                    _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem2, RDW_INVALIDATE| RDW_ERASE);
                    pitem2->pt.x += (cxItem * iDirection);
                } else {
                    // no more intersect!
                    break;
                }
            }

            if (plv->ci.style & LVS_NOSCROLL) {
                if (pitem2->pt.x < 0 || pitem2->pt.y < 0 ||
                    pitem2->pt.x > (plv->sizeClient.cx - (cxItem/2))||
                    pitem2->pt.y > (plv->sizeClient.cy - (cyItem/2))) {
                    pitem2->pt = ptOldPos;
                    fRet = TRUE;
                }
            }

            // invalidate the new position as well
            _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem2, RDW_INVALIDATE| RDW_ERASE);
        }
    }
    return fRet;
}


BOOL NEAR PASCAL ListView_SnapToGrid(LV* plv, HDPA hdpaSort)
{
    // this algorithm can't fit in the structure of the other
    // arrange loop without becoming n^2 or worse.
    // this algorithm is order n.

    // iterate through and snap to the nearest grid.
    // iterate through and push aside overlaps.

    int i;
    int iCount;
    LPARAM  xySpacing;
    int x,y;
    LISTITEM FAR* pitem;
    BOOL bSmallIconView;
    int cxItem, cyItem;

    ASSERT( !ListView_IsOwnerData( plv ) );

    if (bSmallIconView = ListView_IsSmallView(plv))
    {
        cxItem = plv->cxItem;
        cyItem = plv->cyItem;
    }
    else
    {
        cxItem = lv_cxIconSpacing;
        cyItem = lv_cyIconSpacing;
    }


    iCount = ListView_Count(plv);

    // first snap to nearest grid
    for (i = 0; i < iCount; i++) {
        pitem = DPA_GetPtr(hdpaSort, i);

        x = pitem->pt.x;
        y = pitem->pt.y;

        if (!bSmallIconView) {
            x -= ((lv_cxIconSpacing - plv->cxIcon) / 2);
            y -= g_cyIconOffset;
        }

        NearestSlot(&x,&y, cxItem, cyItem, (plv->nWorkAreas > 0) ? &(plv->prcWorkAreas[pitem->iWorkArea]) : NULL);
        if (!bSmallIconView) {
            x += ((lv_cxIconSpacing - plv->cxIcon) / 2);
            y += g_cyIconOffset;
        }

        if (x != pitem->pt.x || y != pitem->pt.y) {
            _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem, RDW_INVALIDATE| RDW_ERASE);
            if ((plv->ci.style & LVS_NOSCROLL) && (plv->nWorkAreas == 0)) {

                // if it's marked noscroll, make sure it's still on the client region
                while (x >= (plv->sizeClient.cx - (cxItem/2)))
                    x -= cxItem;

                while (x < 0)
                    x += cxItem;

                while (y >= (plv->sizeClient.cy - (cyItem/2)))
                    y -= cyItem;

                while (y < 0)
                    y += cyItem;
            }
            pitem->pt.x = x;
            pitem->pt.y = y;
            plv->iFreeSlot = -1; // The "free slot" cache is no good once an item moves

            _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem, RDW_INVALIDATE| RDW_ERASE);
        }
    }

    // now resort the dpa
    switch (plv->ci.style & LVS_ALIGNMASK)
    {
        case LVS_ALIGNLEFT:
        case LVS_ALIGNRIGHT:
            xySpacing = MAKELONG(bSmallIconView ? plv->cxItem : lv_cxIconSpacing, TRUE);
            break;
        default:
            xySpacing = MAKELONG(bSmallIconView ? plv->cyItem : lv_cyIconSpacing, FALSE);
    }

    if (!DPA_Sort(hdpaSort, ArrangeIconCompare, xySpacing))
        return FALSE;


    // go in one direction, if there are still overlaps, go in the other
    // direction as well
    if (ListView_IUnstackOverlaps(plv, hdpaSort, 1))
        ListView_IUnstackOverlaps(plv, hdpaSort, -1);
    return FALSE;
}


BOOL NEAR ListView_OnArrange(LV* plv, UINT style)
{
    BOOL bSmallIconView;
    LPARAM  xySpacing;
    HDPA hdpaSort = NULL;

    bSmallIconView = ListView_IsSmallView(plv);

    if (!bSmallIconView && !ListView_IsIconView(plv)) {
        return FALSE;
    }

    if (ListView_IsOwnerData( plv ))
    {
        if ( style & (LVA_SNAPTOGRID | LVA_SORTASCENDING | LVA_SORTDESCENDING) )
        {
            RIPMSG(0, "LVM_ARRANGE: Cannot combine LVA_SNAPTOGRID or LVA_SORTxxx with owner-data");
            return( FALSE );
        }
    }

    // Make sure our items have positions and their text rectangles
    // caluculated
    if (plv->rcView.left == RECOMPUTE)
        ListView_Recompute(plv);

    if (!ListView_IsOwnerData( plv ))
    {
        // we clone plv->hdpa so we don't blow away indices that
        // apps have saved away.
        // we sort here to make the nested for loop below more bearable.
        hdpaSort = DPA_Clone(plv->hdpa, NULL);

        if (!hdpaSort)
            return FALSE;
    }
    switch (plv->ci.style & LVS_ALIGNMASK)
    {
        case LVS_ALIGNLEFT:
        case LVS_ALIGNRIGHT:
            xySpacing = MAKELONG(bSmallIconView ? plv->cxItem : lv_cxIconSpacing, TRUE);
            break;
        default:
            xySpacing = MAKELONG(bSmallIconView ? plv->cyItem : lv_cyIconSpacing, FALSE);
    }

    if (ListView_IsOwnerData( plv ))
    {
        ListView_CommonArrange(plv, style, NULL);
    }
    else
    {
        if (!DPA_Sort(hdpaSort, ArrangeIconCompare, xySpacing))
            return FALSE;

        ListView_CommonArrange(plv, style, hdpaSort);

        DPA_Destroy(hdpaSort);
    }

    MyNotifyWinEvent(EVENT_OBJECT_REORDER, plv->ci.hwnd, OBJID_CLIENT, 0);

    return TRUE;
}

// Arrange the icons given a sorted hdpa, and arrange them in the sub workareas
BOOL NEAR ListView_CommonArrangeEx(LV* plv, UINT style, HDPA hdpaSort, int iWorkArea)
{
    int iSlot;
    int iItem;
    int cSlots;
    int cWorkAreaSlots[LV_MAX_WORKAREAS];
    BOOL fItemMoved;
    RECT rcLastItem;
    RECT rcSlot;
    RECT rcT;
    BOOL bSmallIconView;
    BOOL bIconView;
    int  xMin = 0;

    bSmallIconView = ListView_IsSmallView(plv);
    bIconView      = ListView_IsIconView(plv);

    // 
    //  when this is an autoarrange, then we dont need to worry about 
    //  scrolling the origin, because we are going to arrange everything 
    //  around the positive side of the origin
    //
    if (LVA_DEFAULT == style && (plv->ci.style & LVS_AUTOARRANGE))
    {
        if (plv->ptOrigin.x < 0)
            plv->ptOrigin.x = 0;
        if (plv->ptOrigin.y < 0)
            plv->ptOrigin.y = 0;
    }

    // REVIEW, this causes a repaint if we are scrollled
    // we can probably avoid this some how
    
    fItemMoved = (plv->ptOrigin.x != 0) || (plv->ptOrigin.y != 0);

    if (!ListView_IsOwnerData( plv ))
    {
        if (style == LVA_SNAPTOGRID) {
            // (dli) This function is fitting all the icons into just one rectangle, 
            // namely sizeClient. We need to make it multi-workarea aware if we want 
            // multi-workarea for the general case (i.e. other than just the desktop)
            // This is never called in the desktop case. 
            fItemMoved |= ListView_SnapToGrid(plv, hdpaSort);

        } else {
            if (plv->nWorkAreas > 0)
            {
                int i;
                for (i = 0; i < plv->nWorkAreas; i++)
                    cWorkAreaSlots[i] = ListView_GetSlotCountEx(plv, TRUE, i);
            }
            else
                cSlots = ListView_GetSlotCount(plv, TRUE);

            
            SetRectEmpty(&rcLastItem);

            // manipulate only the sorted version of the item list below!

            iSlot = 0;
            for (iItem = 0; iItem < ListView_Count(plv); iItem++)
            {
                int cRealSlots; 
                RECT rcIcon, rcLabel;
                LISTITEM FAR* pitem = DPA_GetPtr(hdpaSort, iItem);
                // (dli) In the multi-workarea case, if this item is not in our 
                // workarea, skip it. 
                if (pitem->iWorkArea != iWorkArea)
                    continue;

                cRealSlots = (plv->nWorkAreas > 0) ? cWorkAreaSlots[pitem->iWorkArea] : cSlots;

                if (bSmallIconView || bIconView)
                {
                    for ( ; ; )
                    {
                        _CalcSlotRect(plv, pitem, iSlot, cRealSlots, FALSE, &rcSlot);
                        if (!IntersectRect(&rcT, &rcSlot, &rcLastItem))
                            break;
                        iSlot++;
                    }
                }

                fItemMoved |= ListView_SetIconPos(plv, pitem, iSlot++, cRealSlots);

                // do this instead of ListView_GetRects() because we need
                // to use the pitem from the sorted hdpa, not the ones in *plv
                _ListView_GetRectsFromItem(plv, bSmallIconView, pitem, &rcIcon, &rcLabel, &rcLastItem, NULL);
                // f-n above will return unfolded rects if there are any, we must make sure
                // we use folded ones for slot allocations
                if (bIconView)
                {
                    if (ListView_IsItemUnfoldedPtr(plv, pitem))
                    {
                        ListView_RefoldLabelRect(plv, &rcLabel, pitem);
                        UnionRect(&rcLastItem, &rcIcon, &rcLabel);
                        if (plv->himlState && (LV_StateImageValue(pitem)))
                            rcLastItem.left -= plv->cxState;
                    }
                }
                //
                // Keep track of the minimum x as we don't want negative values
                // when we finish.
                if (rcLastItem.left < xMin)
                    xMin = rcLastItem.left;
            }

            //
            // See if we need to scroll the items over to make sure that all of the
            // no items are hanging off the left hand side.
            //
            if (xMin < 0)
            {
                for (iItem = 0; iItem < ListView_Count(plv); iItem++)
                {
                    LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, iItem);
                    pitem->pt.x -= xMin;        // scroll them over
                }
                plv->rcView.left = RECOMPUTE;   // need to recompute.
                fItemMoved = TRUE;
            }
        }
    }
    //
    // We might as well invalidate the entire window to make sure...
    if (fItemMoved) {
        if (ListView_RedrawEnabled(plv))
            RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
        else {
            ListView_DeleteHrgnInval(plv);
            plv->hrgnInval = (HRGN)ENTIRE_REGION;
            plv->flags |= LVF_ERASE;
        }

        // ensure important items are visible
        iItem = (plv->iFocus >= 0) ? plv->iFocus : ListView_OnGetNextItem(plv, -1, LVNI_SELECTED);

        if (iItem >= 0)
            ListView_OnEnsureVisible(plv, iItem, FALSE);

        if (ListView_RedrawEnabled(plv))
            ListView_UpdateScrollBars(plv);
    }
    return TRUE;
}


// this arranges the icon given a sorted hdpa.
// Arrange the workareas one by one in the multi-workarea case. 
BOOL NEAR ListView_CommonArrange(LV* plv, UINT style, HDPA hdpaSort)
{
    if (plv->nWorkAreas < 1)
    {
        if (plv->exStyle & LVS_EX_MULTIWORKAREAS)
            return TRUE;
        else
            return ListView_CommonArrangeEx(plv, style, hdpaSort, 0);
    }
    else
    {
        int i;
        for (i = 0; i < plv->nWorkAreas; i++)
            ListView_CommonArrangeEx(plv, style, hdpaSort, i);
        return TRUE;
    }
}

void NEAR ListView_IUpdateScrollBars(LV* plv)
{
    RECT rcClient;
    RECT rcView;
    DWORD style;
    DWORD styleOld;
    SCROLLINFO si;
    int ixDelta = 0, iyDelta = 0;
    int iNewPos;
    BOOL fReupdate = FALSE;

    styleOld = ListView_GetWindowStyle(plv);
    style = ListView_GetClientRect(plv, &rcClient, TRUE, &rcView);

    // Grow scrolling rect to origin if necessary.
    if (rcView.left > 0)
    {
        rcView.left = 0;
    }
    if (rcView.top > 0)
    {
        rcView.top = 0;
    }

    //TraceMsg(TF_LISTVIEW, "ListView_GetClientRect %x %x %x %x", rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
    //TraceMsg(TF_LISTVIEW, "ListView_GetViewRect2 %x %x %x %x", rcView.left, rcView.top, rcView.right, rcView.bottom);
    //TraceMsg(TF_LISTVIEW, "rcView %x %x %x %x", plv->rcView.left, plv->rcView.top, plv->rcView.right, plv->rcView.bottom);
    //TraceMsg(TF_LISTVIEW, "Origin %x %x", plv->ptOrigin.x, plv->ptOrigin.y);

    si.cbSize = sizeof(SCROLLINFO);

    if (style & WS_HSCROLL)
    {
        si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
        si.nMin = 0;
        si.nMax = rcView.right - rcView.left - 1;
        //TraceMsg(TF_LISTVIEW, "si.nMax rcView.right - rcView.left - 1 %x", si.nMax);

        si.nPage = rcClient.right - rcClient.left;
        //TraceMsg(TF_LISTVIEW, "si.nPage %x", si.nPage);

        si.nPos = rcClient.left - rcView.left;
        if (si.nPos < 0)
        {
            // with the new rcView calculations, I don't think
            // rcView.left is ever larger than rcClient.left.  msq
            ASSERT(0);
            si.nPos = 0;
        }
        //TraceMsg(TF_LISTVIEW, "si.nPos %x", si.nPos);

        ListView_SetScrollInfo(plv, SB_HORZ, &si, TRUE);

        // make sure our position and page doesn't hang over max
        if ((si.nPos + (LONG)si.nPage - 1 > si.nMax) && si.nPos > 0) {
            iNewPos = (int)si.nMax - (int)si.nPage + 1;
            if (iNewPos < 0) iNewPos = 0;
            if (iNewPos != si.nPos) {
                ixDelta = iNewPos - (int)si.nPos;
                fReupdate = TRUE;
            }
        }
        
    }
    else if (styleOld & WS_HSCROLL)
    {
        ListView_SetScrollRange(plv, SB_HORZ, 0, 0, TRUE);
    }

    if (style & WS_VSCROLL)
    {
        si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
        si.nMin = 0;
        si.nMax = rcView.bottom - rcView.top - 1;

        si.nPage = rcClient.bottom - rcClient.top;

        si.nPos = rcClient.top - rcView.top;
        if (si.nPos < 0)
        {
            // with the new rcView calculations, I don't think
            // rcView.top is ever larger than rcClient.top.  msq
            ASSERT(0);
            si.nPos = 0;
        }

        ListView_SetScrollInfo(plv, SB_VERT, &si, TRUE);

        // make sure our position and page doesn't hang over max
        if ((si.nPos + (LONG)si.nPage - 1 > si.nMax) && si.nPos > 0) {
            iNewPos = (int)si.nMax - (int)si.nPage + 1;
            if (iNewPos < 0) iNewPos = 0;
            if (iNewPos != si.nPos) {
                iyDelta = iNewPos - (int)si.nPos;
                fReupdate = TRUE;
            }
        }
    }
    else if (styleOld & WS_VSCROLL)
    {
        ListView_SetScrollRange(plv, SB_VERT, 0, 0, TRUE);
    }

    if (fReupdate) {
        // we shouldn't recurse because the second time through, si.nPos >0
        ListView_IScroll2(plv, ixDelta, iyDelta, 0);
        ListView_IUpdateScrollBars(plv);
        TraceMsg(TF_WARNING, "LISTVIEW: ERROR: We had to recurse!");
    }

#if 0
    if ((styleOld ^ style) & (WS_HSCROLL | WS_VSCROLL)) {
        SetWindowPos(plv->ci.hwnd, NULL, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE| SWP_FRAMECHANGED);
    }
#endif
        
}

void FAR PASCAL ListView_ComOnScroll(LV* plv, UINT code, int posNew, int sb,
                                     int cLine, int cPage)
{
    int pos;
    SCROLLINFO si;
    BOOL fVert = (sb == SB_VERT);
    UINT uSmooth = 0;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;

    if (!ListView_GetScrollInfo(plv, sb, &si)) {
        return;
    }

    if (cPage != -1)
        si.nPage = cPage;

    si.nMax -= (si.nPage - 1);

    if (si.nMax < si.nMin)
        si.nMax = si.nMin;

    pos = (int)si.nPos; // current position

    switch (code)
    {
    case SB_LEFT:
        si.nPos = si.nMin;
        break;
    case SB_RIGHT:
        si.nPos = si.nMax;
        break;
    case SB_PAGELEFT:
         si.nPos -= si.nPage;
        break;
    case SB_LINELEFT:
        si.nPos -= cLine;
        break;
    case SB_PAGERIGHT:
        si.nPos += si.nPage;
        break;
    case SB_LINERIGHT:
        si.nPos += cLine;
        break;

    case SB_THUMBTRACK:
        si.nPos = posNew;
        uSmooth = SSW_EX_IMMEDIATE;
        break;

    case SB_ENDSCROLL:
        // When scroll bar tracking is over, ensure scroll bars
        // are properly updated...
        //
        ListView_UpdateScrollBars(plv);
        return;

    default:
        return;
    }

    if (plv->iScrollCount >= SMOOTHSCROLLLIMIT)
        uSmooth = SSW_EX_IMMEDIATE;

    si.fMask = SIF_POS;
    si.nPos = ListView_SetScrollInfo(plv, sb, &si, TRUE);

    if (pos != si.nPos)
    {
        int delta = (int)si.nPos - pos;
        int dx = 0, dy = 0;
        if (fVert)
            dy = delta;
        else
            dx = delta;
        _ListView_Scroll2(plv, dx, dy, uSmooth);
        UpdateWindow(plv->ci.hwnd);
    }
}

//
//  We need a smoothscroll callback so our background image draws
//  at the correct origin.  If we don't have a background image,
//  then this work is superfluous but not harmful either.
//
int CALLBACK ListView_IScroll2_SmoothScroll(
    HWND hwnd,
    int dx,
    int dy,
    CONST RECT *prcScroll,
    CONST RECT *prcClip ,
    HRGN hrgnUpdate,
    LPRECT prcUpdate,
    UINT flags)
{
    LV* plv = ListView_GetPtr(hwnd);
    if (plv)
    {
        plv->ptOrigin.x -= dx;
        plv->ptOrigin.y -= dy;
    }

    // Now do what SmoothScrollWindow would've done if we weren't
    // a callback

    return ScrollWindowEx(hwnd, dx, dy, prcScroll, prcClip, hrgnUpdate, prcUpdate, flags);
}



void FAR PASCAL ListView_IScroll2(LV* plv, int dx, int dy, UINT uSmooth)
{
    if (dx | dy)
    {
        if ((plv->clrBk == CLR_NONE) && (plv->pImgCtx == NULL))
        {
            plv->ptOrigin.x += dx;
            plv->ptOrigin.y += dy;
            LVSeeThruScroll(plv, NULL);
        }
        else
        {
            SMOOTHSCROLLINFO si;
            si.cbSize =  sizeof(si);
            si.fMask = SSIF_SCROLLPROC;
            si.hwnd = plv->ci.hwnd;
            si.dx = -dx;
            si.dy = -dy;
            si.lprcSrc = NULL;
            si.lprcClip = NULL;
            si.hrgnUpdate = NULL;
            si.lprcUpdate = NULL;
            si.fuScroll = uSmooth | SW_INVALIDATE | SW_ERASE;
            si.pfnScrollProc = ListView_IScroll2_SmoothScroll;
            SmoothScrollWindow(&si);
        }
    }
}

void NEAR ListView_IOnScroll(LV* plv, UINT code, int posNew, UINT sb)
{
    int cLine;

    if (sb == SB_VERT)
    {
        cLine = lv_cyIconSpacing / 2;
    }
    else
    {
        cLine = lv_cxIconSpacing / 2;
    }

    ListView_ComOnScroll(plv, code,  posNew,  sb,
                         cLine, -1);

}

int NEAR ListView_IGetScrollUnitsPerLine(LV* plv, UINT sb)
{
    int cLine;

    if (sb == SB_VERT)
    {
        cLine = lv_cyIconSpacing / 2;
    }
    else
    {
        cLine = lv_cxIconSpacing / 2;
    }

    return cLine;
}

// NOTE: there is very similar code in the treeview
//
// Totally disgusting hack in order to catch VK_RETURN
// before edit control gets it.
//
LRESULT CALLBACK ListView_EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LV* plv = ListView_GetPtr(GetParent(hwnd));
    LRESULT lret;

    ASSERT(plv);

#if defined(FE_IME) || !defined(WINNT)
    if ( (g_fDBCSInputEnabled) && LOWORD(GetKeyboardLayout(0L)) == 0x0411 )
    {
        // The following code adds IME awareness to the
        // listview's label editing. Currently just for Japanese.
        //
        DWORD dwGcs;
    
        if (msg==WM_SIZE)
        {
            // If it's given the size, tell it to an IME.

             ListView_SizeIME(hwnd);
        }
        else if (msg == EM_SETLIMITTEXT )
        {
           if (wParam < 13)
               plv->flags |= LVF_DONTDRAWCOMP;
           else
               plv->flags &= ~LVF_DONTDRAWCOMP;
        }
        // Give up to draw IME composition by ourselves in case
        // we're working on SFN. Win95d-5709
        else if (!(plv->flags & LVF_DONTDRAWCOMP ))
        {
            switch (msg)
            {

             case WM_IME_STARTCOMPOSITION:
             case WM_IME_ENDCOMPOSITION:
                 return 0L;


             case WM_IME_COMPOSITION:

             // If lParam has no data available bit, it implies
             // canceling composition.
             // ListView_InsertComposition() tries to get composition
             // string w/ GCS_COMPSTR then remove it from edit control if
             // nothing is available.
             //
                 if ( !lParam )
                     dwGcs = GCS_COMPSTR;
                 else
                     dwGcs = (DWORD) lParam;

                 ListView_InsertComposition(hwnd, wParam, dwGcs, plv);
                 return 0L;
                 
             case WM_PAINT:
                 lret=CallWindowProc(plv->pfnEditWndProc, hwnd, msg, wParam, lParam);
                 ListView_PaintComposition(hwnd,plv);
                 return lret;
                 
             case WM_IME_SETCONTEXT:

             // We draw composition string.
             //
                 lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
                 break;

             default:
                 // the other messages should simply be processed
                 // in this subclass procedure.
                 break;
            }
        }
    }
#endif FE_IME

    switch (msg)
    {
    case WM_SETTEXT:
        SetWindowID(hwnd, 1);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            ListView_DismissEdit(plv, FALSE);
            return 0L;

        case VK_ESCAPE:
            ListView_DismissEdit(plv, TRUE);
            return 0L;
        }
        break;

    case WM_CHAR:
        switch (wParam)
        {
        case VK_RETURN:
            // Eat the character, so edit control wont beep!
            return 0L;
        }
                break;

        case WM_GETDLGCODE:
                return DLGC_WANTALLKEYS | DLGC_HASSETSEL;        /* editing name, no dialog handling right now */
    }

    return CallWindowProc(plv->pfnEditWndProc, hwnd, msg, wParam, lParam);
}

//  Helper routine for SetEditSize
void ListView_ChangeEditRectForRegion(LV* plv, LPRECT lprc)
{
    LISTITEM FAR* pitem = ListView_GetItemPtr(plv, plv->iEdit);

    ASSERT(!ListView_IsOwnerData(plv));
    ASSERT(ListView_IsIconView(plv));

    if (!EqualRect((CONST RECT *)&pitem->rcTextRgn, (CONST RECT *)lprc)) {
        // RecalcRegion knows to use rcTextRgn in the case where iEdit != -1,
        // so set it up before calling through.
        CopyRect(&pitem->rcTextRgn, (CONST RECT *)lprc);
        ListView_RecalcRegion(plv, TRUE, TRUE);

        // Invalidate the entire Edit and force a repaint from the listview
        // on down to make sure we don't leave turds...
        InvalidateRect(plv->hwndEdit, NULL, TRUE);
        UpdateWindow(plv->ci.hwnd);
    }
}

// BUGBUG: very similar routine in treeview

void NEAR ListView_SetEditSize(LV* plv)
{
    RECT rcLabel;
    UINT seips;

    if (!((plv->iEdit >= 0) && (plv->iEdit < ListView_Count(plv))))
    {
       ListView_DismissEdit(plv, TRUE);    // cancel edits
       return;
    }

    ListView_GetRects(plv, plv->iEdit, NULL, &rcLabel, NULL, NULL);

    // OffsetRect(&rc, rcLabel.left + g_cxLabelMargin + g_cxBorder,
    //         (rcLabel.bottom + rcLabel.top - rc.bottom) / 2 + g_cyBorder);
    // OffsetRect(&rc, rcLabel.left + g_cxLabelMargin , rcLabel.top);

    // get the text bounding rect

    if (ListView_IsIconView(plv))
    {
        // We should not adjust y-positoin in case of the icon view.
        InflateRect(&rcLabel, -g_cxLabelMargin, -g_cyBorder);
    }
    else
    {
        // Special case for single-line & centered
        InflateRect(&rcLabel, -g_cxLabelMargin - g_cxBorder, (-(rcLabel.bottom - rcLabel.top - plv->cyLabelChar) / 2) - g_cyBorder);
    }

    seips = 0;
    if (ListView_IsIconView(plv) && !(plv->ci.style & LVS_NOLABELWRAP))
        seips |= SEIPS_WRAP;
#ifdef DEBUG
    if (plv->ci.style & LVS_NOSCROLL)
        seips |= SEIPS_NOSCROLL;
#endif

    SetEditInPlaceSize(plv->hwndEdit, &rcLabel, plv->hfontLabel, seips);

    if (plv->exStyle & LVS_EX_REGIONAL)
        ListView_ChangeEditRectForRegion(plv, &rcLabel);
}

// to avoid eating too much stack
void NEAR ListView_DoOnEditLabel(LV *plv, int i, LPTSTR pszInitial)
{
    TCHAR szLabel[CCHLABELMAX];
    LV_ITEM item;

    item.mask = LVIF_TEXT;
    item.iItem = i;
    item.iSubItem = 0;
    item.pszText = szLabel;
    item.cchTextMax = ARRAYSIZE(szLabel);
    ListView_OnGetItem(plv, &item);

    if (!item.pszText)
        return;

    // Make sure the edited item has the focus.
    if (plv->iFocus != i)
        ListView_SetFocusSel(plv, i, TRUE, TRUE, FALSE);

    // Make sure the item is fully visible
    ListView_OnEnsureVisible(plv, i, FALSE);        // fPartialOK == FALSE

    // Must subtract one from ARRAYSIZE(szLabel) because Edit_LimitText doesn't include
    // the terminating NULL

    plv->hwndEdit = CreateEditInPlaceWindow(plv->ci.hwnd,
            pszInitial? pszInitial : item.pszText, ARRAYSIZE(szLabel) - 1,
        ListView_IsIconView(plv) ?
            (WS_BORDER | WS_CLIPSIBLINGS | WS_CHILD | ES_CENTER | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL) :
            (WS_BORDER | WS_CLIPSIBLINGS | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL), plv->hfontLabel);
    if (plv->hwndEdit)
    {
        LISTITEM FAR* pitem;
        LV_DISPINFO nm;

        // We create the edit window but have not shown it.  Ask the owner
        // if they are interested or not.
        // If we passed in initial text set the ID to be dirty...
        if (pszInitial)
            SetWindowID(plv->hwndEdit, 1);

        nm.item.mask = LVIF_PARAM;
        nm.item.iItem = i;
        nm.item.iSubItem = 0;

        if (!ListView_IsOwnerData( plv ))
        {
            if (!(pitem = ListView_GetItemPtr(plv, i)))
            {
                DestroyWindow(plv->hwndEdit);
                plv->hwndEdit = NULL;
                return;
            }
            nm.item.lParam = pitem->lParam;
        }
        else
            nm.item.lParam = (LPARAM)0;


        plv->iEdit = i;

        // if they have LVS_EDITLABELS but return non-FALSE here, stop!
        if ((BOOL)CCSendNotify(&plv->ci, LVN_BEGINLABELEDIT, &nm.hdr))
        {
            plv->iEdit = -1;
            DestroyWindow(plv->hwndEdit);
            plv->hwndEdit = NULL;
        }
    }
}


void FAR PASCAL RescrollEditWindow(HWND hwndEdit)
{
    Edit_SetSel(hwndEdit, -1, -1);      // move to the end
    Edit_SetSel(hwndEdit, 0, -1);       // select all text
}
// BUGBUG: very similar code in treeview.c

HWND NEAR ListView_OnEditLabel(LV* plv, int i, LPTSTR pszInitialText)
{

    // this eats stack
    ListView_DismissEdit(plv, FALSE);

    if (!(plv->ci.style & LVS_EDITLABELS) || (GetFocus() != plv->ci.hwnd) ||
        (i == -1))
        return(NULL);   // Does not support this.

    ListView_DoOnEditLabel(plv, i, pszInitialText);

    if (plv->hwndEdit) {

        plv->pfnEditWndProc = SubclassWindow(plv->hwndEdit, ListView_EditWndProc);

#if defined(FE_IME) || !defined(WINNT)
        if (g_fDBCSInputEnabled) {
            if (SendMessage(plv->hwndEdit, EM_GETLIMITTEXT, (WPARAM)0, (LPARAM)0)<13)
            {
                plv->flags |= LVF_DONTDRAWCOMP;
            }

        }
#endif

        ListView_SetEditSize(plv);

        // Show the window and set focus to it.  Do this after setting the
        // size so we don't get flicker.
        SetFocus(plv->hwndEdit);
        ShowWindow(plv->hwndEdit, SW_SHOW);
        ListView_InvalidateItem(plv, i, TRUE, RDW_INVALIDATE | RDW_ERASE);

        RescrollEditWindow(plv->hwndEdit);

        /* Due to a bizzare twist of fate, a certain mix of resolution / font size / icon
        /  spacing results in being able to see the previous label behind the edit control
        /  we have just created.  Therefore to overcome this problem we ensure that this
        /  label is erased.
        /
        /  As the label is not painted when we have an edit control we just invalidate the
        /  area and the background will be painted.  As the window is a child of the list view
        /  we should not see any flicker within it. */

        if ( ListView_IsIconView( plv ) )
        {
            RECT rcLabel;
            
            ListView_GetRects( plv, i, NULL, &rcLabel, NULL, NULL );
            ListView_UnfoldRects( plv, i, NULL, &rcLabel, NULL, NULL );

            InvalidateRect( plv->ci.hwnd, &rcLabel, TRUE );
            UpdateWindow( plv->ci.hwnd );
        }
    }

    return plv->hwndEdit;
}


// BUGBUG: very similar code in treeview.c

BOOL NEAR ListView_DismissEdit(LV* plv, BOOL fCancel)
{
    LISTITEM FAR* pitem = NULL;
    BOOL fOkToContinue = TRUE;
    HWND hwndEdit = plv->hwndEdit;
    HWND hwnd = plv->ci.hwnd;
    int iEdit;
    LV_DISPINFO nm;
    TCHAR szLabel[CCHLABELMAX];
#if defined(FE_IME) || !defined(WINNT)
    HIMC himc;
#endif


    if (plv->fNoDismissEdit)
        return FALSE;

    if (!hwndEdit) {
        // Also make sure there are no pending edits...
        ListView_CancelPendingEdit(plv);
        return TRUE;    // It is OK to process as normal...
    }

    // If the window is not visible, we are probably in the process
    // of being destroyed, so assume that we are being destroyed
    if (!IsWindowVisible(plv->ci.hwnd))
        fCancel = TRUE;

    //
    // We are using the Window ID of the control as a BOOL to
    // state if it is dirty or not.
    switch (GetWindowID(hwndEdit)) {
    case 0:
        // The edit control is not dirty so act like cancel.
        fCancel = TRUE;
        // Fall through to set window so we will not recurse!
    case 1:
        // The edit control is dirty so continue.
        SetWindowID(hwndEdit, 2);    // Don't recurse
        break;
    case 2:
        // We are in the process of processing an update now, bail out
        return TRUE;
    }

    // BUGBUG: this will fail if the program deleted the items out
    // from underneath us (while we are waiting for the edit timer).
    // make delete item invalidate our edit item
    // We uncouple the edit control and hwnd out from under this as
    // to allow code that process the LVN_ENDLABELEDIT to reenter
    // editing mode if an error happens.
    iEdit = plv->iEdit;

    do
    {
        if (ListView_IsOwnerData( plv ))
        {
            if (!((iEdit >= 0) && (iEdit < plv->cTotalItems)))
            {
                break;
            }
            nm.item.lParam = 0;
        }
        else
        {

            pitem = ListView_GetItemPtr(plv, iEdit);
            ASSERT(pitem);
            if (pitem == NULL)
            {
                break;
            }
            nm.item.lParam = pitem->lParam;
        }

        nm.item.iItem = iEdit;
        nm.item.iSubItem = 0;
        nm.item.cchTextMax = 0;
        nm.item.mask = 0;

        if (fCancel)
            nm.item.pszText = NULL;
        else
        {
            Edit_GetText(hwndEdit, szLabel, ARRAYSIZE(szLabel));
            nm.item.pszText = szLabel;
            nm.item.mask |= LVIF_TEXT;
            nm.item.cchTextMax = ARRAYSIZE(szLabel);
        }

        //
        // Notify the parent that we the label editing has completed.
        // We will use the LV_DISPINFO structure to return the new
        // label in.  The parent still has the old text available by
        // calling the GetItemText function.
        //

        fOkToContinue = (BOOL)CCSendNotify(&plv->ci, LVN_ENDLABELEDIT, &nm.hdr);
        if (!IsWindow(hwnd)) {
            return FALSE;
        }
        if (fOkToContinue && !fCancel)
        {
            //
            // If the item has the text set as CALLBACK, we will let the
            // ower know that they are supposed to set the item text in
            // their own data structures.  Else we will simply update the
            // text in the actual view.
            //
            if (!ListView_IsOwnerData( plv ) &&
                (pitem->pszText != LPSTR_TEXTCALLBACK))
            {
                // Set the item text (everything's set up in nm.item)
                //
                nm.item.mask = LVIF_TEXT;
                ListView_OnSetItem(plv, &nm.item);
            }
            else
            {
                CCSendNotify(&plv->ci, LVN_SETDISPINFO, &nm.hdr);

                // Also we will assume that our cached size is invalid...
                plv->rcView.left = RECOMPUTE;
                if (!ListView_IsOwnerData( plv ))
                {
                    ListView_SetSRecompute(pitem);
                }
            }
        }

#if defined(FE_IME) || !defined(WINNT)
        if (g_fDBCSInputEnabled) {
            if (LOWORD(GetKeyboardLayout(0L)) == 0x0411 && (himc = ImmGetContext(hwndEdit)))
            {
                ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0L);
                ImmReleaseContext(hwndEdit, himc);
            }
        }
#endif

        // redraw
        ListView_InvalidateItem(plv, iEdit, FALSE, RDW_INVALIDATE | RDW_ERASE);
    } while (FALSE);

    // If the hwnedit is still us clear out the variables
    if (hwndEdit == plv->hwndEdit)
    {
        plv->iEdit = -1;
        plv->hwndEdit = NULL;   // avoid being reentered
    }
    DestroyWindow(hwndEdit);

    // We've to recalc the region because the edit in place window has
    // added stuff to the region that we don't know how to remove
    // safely.
    ListView_RecalcRegion(plv, TRUE, TRUE);

    return fOkToContinue;
}

//
// This function will scall the icon positions that are stored in the
// item structures between large and small icon view.
//
void NEAR ListView_ScaleIconPositions(LV* plv, BOOL fSmallIconView)
{
    int cxItem, cyItem;
    HWND hwnd;
    int i;

    if (fSmallIconView)
    {
        if (plv->flags & LVF_ICONPOSSML)
            return;     // Already done
    }
    else
    {
        if ((plv->flags & LVF_ICONPOSSML) == 0)
            return;     // dito
    }

    // Last but not least update our bit!
    plv->flags ^= LVF_ICONPOSSML;

    cxItem = plv->cxItem;
    cyItem = plv->cyItem;
    hwnd = plv->ci.hwnd;

    // We will now loop through all of the items and update their coordinats
    // We will update th position directly into the view instead of calling
    // SetItemPosition as to not do 5000 invalidates and messages...
    if (!ListView_IsOwnerData( plv ))
    {
        for (i = 0; i < ListView_Count(plv); i++)
        {
            LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);

            if (pitem->pt.y != RECOMPUTE) {
                if (fSmallIconView)
                {
                    pitem->pt.x = MulDiv(pitem->pt.x - g_cxIconOffset, cxItem, lv_cxIconSpacing);
                    pitem->pt.y = MulDiv(pitem->pt.y - g_cyIconOffset, cyItem, lv_cyIconSpacing);
                }
                else
                {
                    pitem->pt.x = MulDiv(pitem->pt.x, lv_cxIconSpacing, cxItem) + g_cxIconOffset;
                    pitem->pt.y = MulDiv(pitem->pt.y, lv_cyIconSpacing, cyItem) + g_cyIconOffset;
                }
            }
        }

        plv->iFreeSlot = -1; // The "free slot" cache is no good once an item moves

        if (plv->ci.style & LVS_AUTOARRANGE)
        {
            ListView_ISetColumnWidth(plv, 0,
                                     LV_GetNewColWidth(plv, 0, ListView_Count(plv)-1), FALSE);
            // If autoarrange is turned on, the arrange function will do
            // everything that is needed.
            ListView_OnArrange(plv, LVA_DEFAULT);
            return;
        }
    }
    plv->rcView.left = RECOMPUTE;

    //
    // Also scale the origin
    //
    if (fSmallIconView)
    {
        plv->ptOrigin.x = MulDiv(plv->ptOrigin.x, cxItem, lv_cxIconSpacing);
        plv->ptOrigin.y = MulDiv(plv->ptOrigin.y, cyItem, lv_cyIconSpacing);
    }
    else
    {
        plv->ptOrigin.x = MulDiv(plv->ptOrigin.x, lv_cxIconSpacing, cxItem);
        plv->ptOrigin.y = MulDiv(plv->ptOrigin.y, lv_cyIconSpacing, cyItem);
    }

    // Make sure it fully redraws correctly
    RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}




HWND FAR PASCAL CreateEditInPlaceWindow(HWND hwnd, LPCTSTR lpText, int cbText, LONG style, HFONT hFont)
{
    HWND hwndEdit;

    // Create the window with some nonzero size so margins work properly
    // The caller will do a SetEditInPlaceSize to set the real size
    // But make sure the width is huge so when an app calls SetWindowText,
    // USER won't try to scroll the window.
    hwndEdit = CreateWindowEx(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_RTLREADING, 
                              TEXT("EDIT"), lpText, style,
            0, 0, 16384, 20, hwnd, NULL, HINST_THISDLL, NULL);

    if (hwndEdit) {

        Edit_LimitText(hwndEdit, cbText);

        Edit_SetSel(hwndEdit, 0, 0);    // move to the beginning

        FORWARD_WM_SETFONT(hwndEdit, hFont, FALSE, SendMessage);

    }

    return hwndEdit;
}


// BUGBUG: very similar routine in treeview

// in:
//      hwndEdit        edit control to position in client coords of parent window
//      prc             bonding rect of the text, used to position everthing
//      hFont           font being used
//      flags
//          SEIPS_WRAP      if this is a wrapped type (multiline) edit
//          SEIPS_NOSCROLL  if the parent control does not have scrollbars
//
//      The SEIPS_NOSCROLL flag is used only in DEBUG.  Normally, the item
//      being edited should have been scrolled into view, but if the parent
//      doesn't have scrollbars, then clearly that's not possible, so we
//      shouldn't ASSERT in that case.
//
// Notes:
//       The top-left corner of the bouding rectangle must be the position
//      the client uses to draw text. We adjust the edit field rectangle
//      appropriately.
//

void FAR PASCAL SetEditInPlaceSize(HWND hwndEdit, RECT FAR *prc, HFONT hFont, UINT seips)
{
    RECT rc, rcClient, rcFormat;
    TCHAR szLabel[CCHLABELMAX + 1];
    int cchLabel, cxIconTextWidth;
    HDC hdc;
    HWND hwndParent = GetParent(hwndEdit);
    UINT flags;

    // was #ifdef DBCS
    // short wRightMgn;
    // #endif

    cchLabel = Edit_GetText(hwndEdit, szLabel, ARRAYSIZE(szLabel));
    if (szLabel[0] == 0)
    {
        lstrcpy(szLabel, c_szSpace);
        cchLabel = 1;
    }

    hdc = GetDC(hwndParent);

#ifdef DEBUG
    //DrawFocusRect(hdc, prc);       // this is the rect they are passing in
#endif

    SelectFont(hdc, hFont);

    cxIconTextWidth = g_cxIconSpacing - g_cxLabelMargin * 2;
    rc.left = rc.top = rc.bottom = 0;
    rc.right = cxIconTextWidth;      // for DT_LVWRAP

    // REVIEW: we might want to include DT_EDITCONTROL in our DT_LVWRAP

    if (seips & SEIPS_WRAP)
    {
        flags = DT_LVWRAP | DT_CALCRECT;
        // We only use DT_NOFULLWIDTHCHARBREAK on Korean(949) Memphis and NT5
        if (949 == g_uiACP && (g_bRunOnNT5 || g_bRunOnMemphis))
            flags |= DT_NOFULLWIDTHCHARBREAK;
    }
    else
        flags = DT_LV | DT_CALCRECT;
    // If the string is NULL display a rectangle that is visible.
    DrawText(hdc, szLabel, cchLabel, &rc, flags);

    // Minimum text box size is 1/4 icon spacing size
    if (rc.right < g_cxIconSpacing / 4)
        rc.right = g_cxIconSpacing / 4;

    // position the text rect based on the text rect passed in
    // if wrapping, center the edit control around the text mid point

    OffsetRect(&rc,
        (seips & SEIPS_WRAP) ? prc->left + ((prc->right - prc->left) - (rc.right - rc.left)) / 2 : prc->left,
        (seips & SEIPS_WRAP) ? prc->top : prc->top +  ((prc->bottom - prc->top) - (rc.bottom - rc.top)) / 2 );

    // give a little space to ease the editing of this thing
    if (!(seips & SEIPS_WRAP))
        rc.right += g_cxLabelMargin * 4;
    rc.right += g_cyEdge;   // try to leave a little more for dual blanks

#ifdef DEBUG
    //DrawFocusRect(hdc, &rc);
#endif

    ReleaseDC(hwndParent, hdc);

    //
    // #5688: We need to make it sure that the whole edit window is
    //  always visible. We should not extend it to the outside of
    //  the parent window.
    //
    {
        BOOL fSuccess;
        GetClientRect(hwndParent, &rcClient);
        fSuccess = IntersectRect(&rc, &rc, &rcClient);
        ASSERT(fSuccess || IsRectEmpty(&rcClient) || (seips & SEIPS_NOSCROLL));
    }

    //
    // Inflate it after the clipping, because it's ok to hide border.
    //
    // EM_GETRECT already takes EM_GETMARGINS into account, so don't use both.

    SendMessage(hwndEdit, EM_GETRECT, 0, (LPARAM)(LPRECT)&rcFormat);

    // Turn the margins inside-out so we can AdjustWindowRect on them.
    rcFormat.top = -rcFormat.top;
    rcFormat.left = -rcFormat.left;
    AdjustWindowRectEx(&rcFormat, GetWindowStyle(hwndEdit), FALSE,
                                  GetWindowExStyle(hwndEdit));

    InflateRect(&rc, -rcFormat.left, -rcFormat.top);

    HideCaret(hwndEdit);

    SetWindowPos(hwndEdit, NULL, rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);

    CopyRect(prc, (CONST RECT *)&rc);

#ifndef UNIX
    InvalidateRect(hwndEdit, NULL, TRUE);
#endif

    ShowCaret(hwndEdit);
}

// draw three pixel wide border for border selection.
void NEAR PASCAL ListView_DrawBorderSel(HIMAGELIST himl, HWND hwnd, HDC hdc, int x,int y, COLORREF clr)
{
    int dx, dy;
    RECT rc;
    COLORREF clrSave = SetBkColor(hdc, clr);

    ImageList_GetIconSize(himl, &dx, &dy);
    //left
    rc.left = x - 4;    // 1 pixel seperation + 3 pixel width.
    rc.top = y - 4;
    rc.right = x - 1;
    rc.bottom = y + dy + 4;
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
    //top
    rc.left = rc.right;
    rc.right = rc.left + dx + 2;
    rc.bottom = rc.top + 3;
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
    //right
    rc.left = rc.right;
    rc.right = rc.left + 3;
    rc.bottom = rc.top + dy + 8; // 2*3 pixel borders + 2*1 pixel seperation = 8
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
    // bottom
    rc.top = rc.bottom - 3;
    rc.right = rc.left;
    rc.left = rc.right - dx - 2;
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);

    SetBkColor(hdc, clrSave);
    return;
}

//
//  If xMax >= 0, then the image will not be drawn past the x-coordinate
//  specified by xMax.  This is used only during report view drawing, where
//  we have to clip against our column width.
//
UINT NEAR PASCAL ListView_DrawImageEx(LV* plv, LV_ITEM FAR* pitem, HDC hdc, int x, int y, UINT fDraw, int xMax)
{
    UINT fText = SHDT_DESELECTED;
    UINT fImage = ILD_NORMAL;
    COLORREF clr = 0;
    HIMAGELIST himl;
    BOOL fBorderSel = (plv->exStyle & LVS_EX_BORDERSELECT);
    int cxIcon;

    fImage = (pitem->state & LVIS_OVERLAYMASK);
    fText = SHDT_DESELECTED;

    if (ListView_IsIconView(plv)) {
        himl = plv->himl;
        cxIcon = plv->cxIcon;
    } else {
        himl = plv->himlSmall;
        cxIcon = plv->cxSmIcon;
    }

    // the item can have one of 4 states, for 3 looks:
    //    normal                    simple drawing
    //    selected, no focus        light image highlight, no text hi
    //    selected w/ focus         highlight image & text
    //    drop highlighting         highlight image & text

    if ((pitem->state & LVIS_DROPHILITED) ||
        ((fDraw & LVDI_SELECTED) && (pitem->state & LVIS_SELECTED)))
    {
        fText = SHDT_SELECTED;
        if (!fBorderSel)    // do not effect color of icon on borderselect.
        {
            fImage |= ILD_BLEND50;
            clr = CLR_HILIGHT;
        }
    }

    if ((fDraw & LVDI_SELECTNOFOCUS) && (pitem->state & LVIS_SELECTED)) {
        fText = SHDT_SELECTNOFOCUS;
        //fImage |= ILD_BLEND50;
        //clr = GetSysColor(COLOR_3DFACE);
    }

    if (pitem->state & LVIS_CUT)
    {
        fImage |= ILD_BLEND50;
        clr = plv->clrBk;
    }

#if 0   // dont do a selected but dont have the focus vis.
    else if (item.state & LVIS_SELECTED)
    {
        fImage |= ILD_BLEND25;
        clr = CLR_HILIGHT;
    }
#endif

    if (!(fDraw & LVDI_NOIMAGE))
    {
        if (himl) {
            COLORREF clrBk;

            if (plv->pImgCtx || ((plv->exStyle & LVS_EX_REGIONAL) && !g_fSlowMachine))
                clrBk = CLR_NONE;
            else
                clrBk = plv->clrBk;

            if (xMax >= 0)
                cxIcon = min(cxIcon, xMax - x);

            if (cxIcon > 0)
                ImageList_DrawEx(himl, pitem->iImage, hdc, x, y, cxIcon, 0, clrBk, clr, fImage);
        }

        if (plv->himlState) {
            if (LV_StateImageValue(pitem) &&
                (pitem->iSubItem == 0 ||
                 plv->exStyle & LVS_EX_SUBITEMIMAGES)
                ) {
                int iState = LV_StateImageIndex(pitem);
                int dyImage =
                    (himl) ?
                        ( (ListView_IsIconView(plv) ? plv->cyIcon : plv->cySmIcon) - plv->cyState)
                            : 0;
                int xDraw = x-plv->cxState;
                cxIcon = plv->cxState;
                if (xMax >= 0)
                    cxIcon = min(cxIcon, xMax - xDraw);
                if (cxIcon > 0)
                    ImageList_DrawEx(plv->himlState, iState, hdc,
                               xDraw,
                               y + dyImage,
                               cxIcon,
                               0,
                               CLR_DEFAULT,
                               CLR_DEFAULT,
                               ILD_NORMAL);
            }
        }
        // draw the border selection if appropriate.
        if (fBorderSel && !(fText & SHDT_DESELECTED))       // selected, draw the selection rect.
        {
            COLORREF clrBorder = (fDraw & LVDI_HOTSELECTED) 
                        ? GetSysColor(COLOR_HOTLIGHT) : g_clrHighlight;
            ListView_DrawBorderSel(himl, plv->ci.hwnd, hdc, x, y, clrBorder);
        }
        else if (fBorderSel && (fText & SHDT_DESELECTED))   // erase possible selection rect.
            ListView_DrawBorderSel(himl, plv->ci.hwnd, hdc, x, y, plv->clrBk);

    }

    return fText;
}

#if defined(FE_IME) || !defined(WINNT)
void NEAR PASCAL ListView_SizeIME(HWND hwnd)
{
    HIMC himc;
#ifdef _WIN32
    CANDIDATEFORM   candf;
#else
    CANDIDATEFORM16   candf;
#endif
    RECT rc;

    // If this subclass procedure is being called with WM_SIZE,
    // This routine sets the rectangle to an IME.

    GetClientRect(hwnd, &rc);


    // Candidate stuff
    candf.dwIndex = 0; // Bogus assumption for Japanese IME.
    candf.dwStyle = CFS_EXCLUDE;
    candf.ptCurrentPos.x = rc.left;
    candf.ptCurrentPos.y = rc.bottom;
    candf.rcArea = rc;

    if (himc=ImmGetContext(hwnd))
    {
        ImmSetCandidateWindow(himc, &candf);
        ImmReleaseContext(hwnd, himc);
    }
}

#ifndef UNICODE
LPSTR NEAR PASCAL DoDBCSBoundary(LPTSTR lpsz, int FAR *lpcchMax)
{
    int i = 0;

    while (i < *lpcchMax && *lpsz)
    {
        i++;

        if (IsDBCSLeadByte(*lpsz))
        {

            if (i >= *lpcchMax)
            {
                --i; // Wrap up without the last leadbyte.
                break;
            }

            i++;
            lpsz+= 2;
        }
        else
            lpsz++;
   }

   *lpcchMax = i;

   return lpsz;
}
#endif

void NEAR PASCAL DrawCompositionLine(HWND hwnd, HDC hdc, HFONT hfont, LPTSTR lpszComp, LPBYTE lpszAttr, int ichCompStart, int ichCompEnd, int ichStart)
{
    PTSTR pszCompStr;
    int ichSt,ichEnd;
    DWORD dwPos;
    BYTE bAttr;
    HFONT hfontOld;

    int  fnPen;
    HPEN hPen;
    COLORREF crDrawText;
    COLORREF crDrawBack;
    COLORREF crOldText;
    COLORREF crOldBk;


    while (ichCompStart < ichCompEnd)
    {

        // Get the fragment to draw
        //
        // ichCompStart,ichCompEnd -- index at Edit Control
        // ichSt,ichEnd            -- index at lpszComp

        ichEnd = ichSt  = ichCompStart - ichStart;
        bAttr = lpszAttr[ichSt];

        while (ichEnd < ichCompEnd - ichStart)
        {
            if (bAttr == lpszAttr[ichEnd])
                ichEnd++;
            else
                break;
        }

        pszCompStr = (PTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(ichEnd - ichSt + 1 + 1) ); // 1 for NULL.

        if (pszCompStr)
        {
            lstrcpyn(pszCompStr, &lpszComp[ichSt], ichEnd-ichSt+1);
            pszCompStr[ichEnd-ichSt] = '\0';
        }


        // Attribute stuff
        switch (bAttr)
        {
            case ATTR_INPUT:
                fnPen = PS_DOT;
                crDrawText = g_clrWindowText;
                crDrawBack = g_clrWindow;
                break;
            case ATTR_TARGET_CONVERTED:
            case ATTR_TARGET_NOTCONVERTED:
                fnPen = PS_DOT;
                crDrawText = g_clrHighlightText;
                crDrawBack = g_clrHighlight;
                break;
            case ATTR_CONVERTED:
                fnPen = PS_SOLID;
                crDrawText = g_clrWindowText;
                crDrawBack = g_clrWindow;
                break;
        }
        crOldText = SetTextColor(hdc, crDrawText);
        crOldBk = SetBkColor(hdc, crDrawBack);

        hfontOld= SelectObject(hdc, hfont);

        // Get the start position of composition
        //
        dwPos = (DWORD) SendMessage(hwnd, EM_POSFROMCHAR, ichCompStart, 0);

        // Draw it.
        TextOut(hdc, GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos), pszCompStr, ichEnd-ichSt);
#ifndef DONT_UNDERLINE
        // Underline
        hPen = CreatePen(fnPen, 1, crDrawText);
        if( hPen ) {

            HPEN hpenOld = SelectObject( hdc, hPen );
            int iOldBk = SetBkMode( hdc, TRANSPARENT );
            SIZE size;

            GetTextExtentPoint(hdc, pszCompStr, ichEnd-ichSt, &size);

            MoveToEx( hdc, GET_X_LPARAM(dwPos), size.cy + GET_Y_LPARAM(dwPos)-1, NULL);

            LineTo( hdc, size.cx + GET_X_LPARAM(dwPos),  size.cy + GET_Y_LPARAM(dwPos)-1 );

            SetBkMode( hdc, iOldBk );

            if( hpenOld ) SelectObject( hdc, hpenOld );

            DeleteObject( hPen );
        }
#endif

        if (hfontOld)
            SelectObject(hdc, hfontOld);

        SetTextColor(hdc, crOldText);
        SetBkColor(hdc, crOldBk);

        LocalFree((HLOCAL)pszCompStr);

        //Next fragment
        //
        ichCompStart += ichEnd-ichSt;
    }
}

void NEAR PASCAL ListView_InsertComposition(HWND hwnd, WPARAM wParam, LPARAM lParam, LV *plv)
{
    PSTR pszCompStr;

    int  cbComp = 0;
    int  cbCompNew;
    int  cchMax;
    int  cchText;
    DWORD dwSel;
    HIMC himc = (HIMC)0;


    // To prevent recursion..

    if (plv->flags & LVF_INSERTINGCOMP)
    {
        return;
    }
    plv->flags |= LVF_INSERTINGCOMP;

    // Don't want to redraw edit during inserting.
    //
    SendMessage(hwnd, WM_SETREDRAW, (WPARAM)FALSE, 0);

    // If we have RESULT STR, put it to EC first.

    if (himc = ImmGetContext(hwnd))
    {
#ifdef WIN32
        if (!(dwSel = PtrToUlong(GetProp(hwnd, szIMECompPos))))
            dwSel = Edit_GetSel(hwnd);

        // Becaues we don't setsel after inserting composition
        // in win32 case.
        Edit_SetSel(hwnd, GET_X_LPARAM(dwSel), GET_Y_LPARAM(dwSel));
#endif
        if (lParam&GCS_RESULTSTR)
        {
            // ImmGetCompositionString() returns length of buffer in bytes,
            // not in # of character
            cbComp = (int)ImmGetCompositionString(himc, GCS_RESULTSTR, NULL, 0);
            
            pszCompStr = (PSTR)LocalAlloc(LPTR, cbComp + sizeof(TCHAR));
            if (pszCompStr)
            {
                ImmGetCompositionString(himc, GCS_RESULTSTR, (PSTR)pszCompStr, cbComp+sizeof(TCHAR));
                
                // With ImmGetCompositionStringW, cbComp is # of bytes copied
                // character position must be calculated by cbComp / sizeof(TCHAR)
                //
                *(TCHAR *)(&pszCompStr[cbComp]) = TEXT('\0');
                Edit_ReplaceSel(hwnd, (LPTSTR)pszCompStr);
                LocalFree((HLOCAL)pszCompStr);
            }
#ifdef WIN32
            // There's no longer selection
            //
            RemoveProp(hwnd, szIMECompPos);

            // Get current cursor pos so that the subsequent composition
            // handling will do the right thing.
            //
            dwSel = Edit_GetSel(hwnd);
#endif
        }

        if (lParam & GCS_COMPSTR)
        {
            // ImmGetCompositionString() returns length of buffer in bytes,
            // not in # of character
            //
            cbComp = (int)ImmGetCompositionString(himc, GCS_COMPSTR, NULL, 0);
            pszCompStr = (PSTR)LocalAlloc(LPTR, cbComp + sizeof(TCHAR));
            if (pszCompStr)
            {
                ImmGetCompositionString(himc, GCS_COMPSTR, pszCompStr, cbComp+sizeof(TCHAR));

                // Get position of the current selection
                //
#ifndef WIN32
                dwSel  = Edit_GetSel(hwnd);
#endif
                cchMax = (int)SendMessage(hwnd, EM_GETLIMITTEXT, 0, 0);
                cchText = Edit_GetTextLength(hwnd);

                // Cut the composition string if it exceeds limit.
                //
                cbCompNew = min((UINT)cbComp,
                              sizeof(TCHAR)*(cchMax-(cchText-(HIWORD(dwSel)-LOWORD(dwSel)))));

                // wrap up the DBCS at the end of string
                //
                if (cbCompNew < cbComp)
                {
#ifndef UNICODE
                    DoDBCSBoundary((LPSTR)pszCompStr, (int FAR *)&cbCompNew);
#endif

                    *(TCHAR *)(&pszCompStr[cbCompNew]) = TEXT('\0');

                    // Reset composition string if we cut it.
                    ImmSetCompositionString(himc, SCS_SETSTR, pszCompStr, cbCompNew, NULL, 0);
                    cbComp = cbCompNew;
                }
                
               *(TCHAR *)(&pszCompStr[cbComp]) = TEXT('\0');

               // Replace the current selection with composition string.
               //
               Edit_ReplaceSel(hwnd, (LPTSTR)pszCompStr);

               LocalFree((HLOCAL)pszCompStr);
           }

           // Mark the composition string so that we can replace it again
           // for the next time.
           //

#ifdef WIN32
           // Don't setsel to avoid flicking
           if (cbComp)
           {
               dwSel = MAKELONG(LOWORD(dwSel),LOWORD(dwSel)+cbComp/sizeof(TCHAR));
               SetProp(hwnd, szIMECompPos, (HANDLE)dwSel);
           }
           else
               RemoveProp(hwnd, szIMECompPos);
#else
           // Still use SETSEL for 16bit.
           if (cbComp)
               Edit_SetSel(hwnd, LOWORD(dwSel), LOWORD(dwSel)+cbComp);
#endif

        }

        ImmReleaseContext(hwnd, himc);
    }

    SendMessage(hwnd, WM_SETREDRAW, (WPARAM)TRUE, 0);
    //
    // We want to update the size of label edit just once at
    // each WM_IME_COMPOSITION processing. ReplaceSel causes several EN_UPDATE
    // and it causes ugly flicking too.
    //
    RedrawWindow(hwnd, NULL, NULL, RDW_INTERNALPAINT|RDW_INVALIDATE);
    SetWindowID(plv->hwndEdit, 1);
    ListView_SetEditSize(plv);

    plv->flags &= ~LVF_INSERTINGCOMP;
}

void NEAR PASCAL ListView_PaintComposition(HWND hwnd, LV * plv)
{
    BYTE szCompStr[CCHLABELMAX + 1];
    BYTE szCompAttr[CCHLABELMAX + 1];

    int  cchLine, ichLineStart;
    int  cbComp = 0;
    int  cchComp;
    int  nLine;
    int  ichCompStart, ichCompEnd;
    DWORD dwSel;
    int  cchMax, cchText;
    HIMC himc = (HIMC)0;
    HDC  hdc;


    if (plv->flags & LVF_INSERTINGCOMP)
    {
        // This is the case that ImmSetCompositionString() generates
        // WM_IME_COMPOSITION. We're not ready to paint composition here.
        return;
    }

    if (himc = ImmGetContext(hwnd))
    {

        cbComp=(UINT)ImmGetCompositionString(himc, GCS_COMPSTR, szCompStr, sizeof(szCompStr));

        ImmGetCompositionString(himc, GCS_COMPATTR, szCompAttr, sizeof(szCompStr));
        ImmReleaseContext(hwnd, himc);
    }

    if (cbComp)
    {

        // Get the position of current selection
        //
#ifdef WIN32

        if (!(dwSel = PtrToUlong(GetProp(hwnd, szIMECompPos))))
            dwSel = 0L;
#else
        dwSel  = Edit_GetSel(hwnd);
#endif
        cchMax = (int)SendMessage(hwnd, EM_GETLIMITTEXT, 0, 0);
        cchText = Edit_GetTextLength(hwnd);
        cbComp = min((UINT)cbComp, sizeof(TCHAR)*(cchMax-(cchText-(HIWORD(dwSel)-LOWORD(dwSel)))));
#ifndef UNICODE
        DoDBCSBoundary((LPTSTR)szCompStr, (int FAR *)&cbComp);
#endif
        *(TCHAR *)(&szCompStr[cbComp]) = TEXT('\0');



        /////////////////////////////////////////////////
        //                                             //
        // Draw composition string over the sel string.//
        //                                             //
        /////////////////////////////////////////////////


        hdc = GetDC(hwnd);


        ichCompStart = LOWORD(dwSel);

        cchComp = cbComp/sizeof(TCHAR);
        while (ichCompStart < (int)LOWORD(dwSel) + cchComp)
        {
            // Get line from each start pos.
            //
            nLine = Edit_LineFromChar(hwnd, ichCompStart);
            ichLineStart = Edit_LineIndex(hwnd, nLine);
            cchLine= Edit_LineLength(hwnd, ichLineStart);

            // See if composition string is longer than this line.
            //
            if(ichLineStart+cchLine > (int)LOWORD(dwSel)+cchComp)
                ichCompEnd = LOWORD(dwSel)+cchComp;
            else
            {
                // Yes, the composition string is longer.
                // Take the begining of the next line as next start.
                //
                if (ichLineStart+cchLine > ichCompStart)
                    ichCompEnd = ichLineStart+cchLine;
                else
                {
                    // If the starting position is not proceeding,
                    // let's get out of here.
                    break;
                }
            }

            // Draw the line
            //
            DrawCompositionLine(hwnd, hdc, plv->hfontLabel, (LPTSTR)szCompStr, szCompAttr, ichCompStart, ichCompEnd, LOWORD(dwSel));

            ichCompStart = ichCompEnd;
        }

        ReleaseDC(hwnd, hdc);
        // We don't want to repaint the window.
        ValidateRect(hwnd, NULL);
    }
}

#endif FE_IME
