// small icon view (positional view, not list)

#include "ctlspriv.h"
#include "listview.h"

int NEAR ListView_SItemHitTest(LV* plv, int x, int y, UINT FAR* pflags, int *piSubItem)
{

    int iHit;
    UINT flags;
    POINT pt;
    RECT rcState;
    RECT rcLabel;
    RECT rcIcon;

    if (piSubItem)
        *piSubItem = 0;

    // Map window-relative coordinates to view-relative coords...
    //
    pt.x = x + plv->ptOrigin.x;
    pt.y = y + plv->ptOrigin.y;

    // If we find an uncomputed item, recompute them all now...
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
        ListView_SGetRectsOwnerData( plv, iHit, &rcIcon, &rcLabel, &item, FALSE );
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
    else
    {
        for (iHit = 0; iHit < ListView_Count(plv); iHit++)
        {
            LISTITEM FAR* pitem = ListView_FastGetZItemPtr(plv, iHit);
            POINT ptItem;

            ptItem.x = pitem->pt.x;
            ptItem.y = pitem->pt.y;

            rcIcon.top    = ptItem.y;
            rcIcon.bottom = ptItem.y + plv->cyItem;

            rcLabel.top    = rcIcon.top;
            rcLabel.bottom = rcIcon.bottom;

            // Quick, easy rejection test...
            //
            if (pt.y < rcIcon.top || pt.y >= rcIcon.bottom)
                continue;

            rcIcon.left   = ptItem.x;
            rcIcon.right  = ptItem.x + plv->cxSmIcon;
            
            rcState.bottom = rcIcon.bottom;
            rcState.right = rcIcon.left;
            rcState.left = rcState.right - plv->cxState;
            rcState.top = rcState.bottom - plv->cyState;

            rcLabel.left   = rcIcon.right;
            rcLabel.right  = rcLabel.left + pitem->cxSingleLabel;

            if (PtInRect(&rcIcon, pt))
            {
                flags = LVHT_ONITEMICON;
            } else if (PtInRect(&rcLabel, pt))
            {
                flags = LVHT_ONITEMLABEL;
            } else if (PtInRect(&rcState, pt)) 
            {
                flags = LVHT_ONITEMSTATEICON;
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
          iHit = DPA_GetPtrIndex(plv->hdpa, (void FAR*)ListView_FastGetZItemPtr(plv, iHit));
    }

    *pflags = flags;
    return iHit;
}


void NEAR ListView_SGetRectsOwnerData( LV* plv,
        int iItem,
        RECT FAR* prcIcon,
        RECT FAR* prcLabel,
        LISTITEM* pitem,
        BOOL fUsepitem )
{
    RECT rcIcon;
    RECT rcLabel;
    int cSlots;

    // calculate itemx, itemy, itemsSingleLabel from iItem
    cSlots = ListView_GetSlotCount( plv, TRUE );
    pitem->iWorkArea = 0;               // OwnerData doesn't support workareas
    ListView_SetIconPos( plv, pitem, iItem, cSlots );

    // calculate lable sizes
    // Note the rect we return should be the min of the size returned and the slot size...
    ListView_RecomputeLabelSize( plv, pitem, iItem, NULL, fUsepitem );

    rcIcon.left   = pitem->pt.x - plv->ptOrigin.x;
    rcIcon.right  = rcIcon.left + plv->cxSmIcon;
    rcIcon.top    = pitem->pt.y - plv->ptOrigin.y;
    rcIcon.bottom = rcIcon.top + plv->cyItem;
    *prcIcon = rcIcon;

    rcLabel.left   = rcIcon.right;
    if (pitem->cxSingleLabel < (plv->cxItem - plv->cxSmIcon))
        rcLabel.right  = rcLabel.left + pitem->cxSingleLabel;
    else
        rcLabel.right  = rcLabel.left + plv->cxItem - plv->cxSmIcon;
    rcLabel.top    = rcIcon.top;
    rcLabel.bottom = rcIcon.bottom;
    *prcLabel = rcLabel;
}


void NEAR ListView_SGetRects(LV* plv, LISTITEM FAR* pitem, RECT FAR* prcIcon, RECT FAR* prcLabel, LPRECT prcBounds)
{

    ASSERT( !ListView_IsOwnerData( plv ));

    if (pitem->pt.x == RECOMPUTE) {
        ListView_Recompute(plv);
    }

    prcIcon->left   = pitem->pt.x - plv->ptOrigin.x;
    prcIcon->right  = prcIcon->left + plv->cxSmIcon;
    prcIcon->top    = pitem->pt.y - plv->ptOrigin.y;
    prcIcon->bottom = prcIcon->top + plv->cyItem;

    prcLabel->left   = prcIcon->right;
    prcLabel->right  = prcLabel->left + pitem->cxSingleLabel;
    prcLabel->top    = prcIcon->top;
    prcLabel->bottom = prcIcon->bottom;
}

// Return the index of the first item >= *pszLookup.
//
int NEAR ListView_DoLookupString(LV* plv, LPCTSTR pszLookup, UINT flags, int iStart, int j)
{
    int i;
    BOOL fExact;
    int k;
    LISTITEM FAR* pitem;
    LISTITEM FAR* pitemLast = NULL;

    ASSERT( !ListView_IsOwnerData( plv ));

    fExact = FALSE;
    i = iStart;
    while ((i >= iStart) && (i < j))
    {
        int result;
        k = (i + j) / 2;
        pitem = ListView_FastGetItemPtr(plv, k);
        
        if (pitem == pitemLast)
            break;
        pitemLast = pitem;
        
        result = ListView_CompareString(plv, 
                k, pszLookup, flags, 0);

#ifdef MAINWIN
        // IEUNIX - Mainwin's lstrcmp is not compatable with WIN32.
        if(result < 0)
            result = -1;
        else if(result > 0)
            result = 1;
#endif

        if (plv->ci.style & LVS_SORTDESCENDING)
            result = -result;

        switch (result)
        {
        case 0:
            fExact = TRUE;
            // fall through
        case 1:
            j = k;
            break;
        case -1:
            i = k + 1;
            break;
        }
    }
    // For substrings, return index only if exact match was found.
    //
    if (!(flags & (LVFI_SUBSTRING | LVFI_PARTIAL)) && 
        !fExact)
        return -1;

    if (i < 0)
        i = 0;
    
    if ((!(flags & LVFI_NEARESTXY)) &&
        ListView_CompareString(plv, i, pszLookup, flags, 1)) {
        i = -1;
    }
    return i;
}

int NEAR ListView_LookupString(LV* plv, LPCTSTR pszLookup, UINT flags, int iStart)
{
    int iret;
    
    if (!pszLookup)
        return 0;
    
    iret = ListView_DoLookupString(plv, pszLookup, flags, iStart, ListView_Count(plv));
    if (iret == -1 && (flags & LVFI_WRAP)) {
        iret = ListView_DoLookupString(plv, pszLookup, flags, 0, iStart);
    }
    
    return iret;
}
