// small icon view (positional view, not list)

#include "ctlspriv.h"
#include "listview.h"

void NEAR PASCAL ListView_SDrawItem(LV* plv, int i, HDC hdc, LPPOINT lpptOrg, RECT FAR* prcClip, UINT fDraw)
{
    RECT rcIcon;
    RECT rcLabel;
    RECT rcBounds;
    RECT rcT;

    ListView_GetRects(plv, i, &rcIcon, &rcLabel, &rcBounds, NULL);

    if (!prcClip || IntersectRect(&rcT, &rcBounds, prcClip))
    {
        char ach[CCHLABELMAX];
        LV_ITEM item;
        UINT fText;

	if (lpptOrg)
	{
	    OffsetRect(&rcIcon, lpptOrg->x - rcBounds.left,
	    			lpptOrg->y - rcBounds.top);
	    OffsetRect(&rcLabel, lpptOrg->x - rcBounds.left,
	    			lpptOrg->y - rcBounds.top);
	}

        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = ach;
        item.cchTextMax = sizeof(ach);
        item.stateMask = LVIS_ALL;
        ListView_OnGetItem(plv, &item);

        fText = ListView_DrawImage(plv, &item, hdc,
            rcIcon.left, rcIcon.top, fDraw);

        // Don't draw label if it's being edited...
        //
        if (plv->iEdit != i)
        {
	    if (fDraw & LVDI_TRANSTEXT)
                fText |= SHDT_TRANSPARENT;

            if (item.pszText)
            {
                SHDrawText(hdc, item.pszText, &rcLabel, LVCFMT_LEFT, fText,
                           plv->cyLabelChar, plv->cxEllipses, plv->clrText, 
                           plv->style & WS_DISABLED ? plv->clrBk : plv->clrTextBk);
            }

            if ((fDraw & LVDI_FOCUS) && (item.state & LVIS_FOCUSED))
                DrawFocusRect(hdc, &rcLabel);
        }
    }
}

int NEAR ListView_SItemHitTest(LV* plv, int x, int y, UINT FAR* pflags)
{

    int iHit;
    UINT flags;
    POINT pt;

    // Map window-relative coordinates to view-relative coords...
    //
    pt.x = x + plv->ptOrigin.x;
    pt.y = y + plv->ptOrigin.y;

    // If we find an uncomputed item, recompute them all now...
    //
    if (plv->rcView.left == RECOMPUTE)
        ListView_Recompute(plv);

    flags = 0;
    for (iHit = 0; iHit < ListView_Count(plv); iHit++)
    {
        LISTITEM FAR* pitem = ListView_FastGetZItemPtr(plv, iHit);
        POINT ptItem;
        RECT rcLabel;
        RECT rcIcon;

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

        rcLabel.left   = rcIcon.right;
        rcLabel.right  = rcLabel.left + pitem->cxSingleLabel;

        if (PtInRect(&rcIcon, pt))
        {
            flags = LVHT_ONITEMICON;
            break;
        }

        if (PtInRect(&rcLabel, pt))
        {
            flags = LVHT_ONITEMLABEL;
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
        iHit = DPA_GetPtrIndex(plv->hdpa, (void FAR*)ListView_FastGetZItemPtr(plv, iHit));
    }

    *pflags = flags;
    return iHit;
}

void NEAR ListView_SGetRects(LV* plv, LISTITEM FAR* pitem, RECT FAR* prcIcon, RECT FAR* prcLabel, LPRECT prcBounds)
{
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
int NEAR ListView_DoLookupString(LV* plv, LPCSTR pszLookup, UINT flags, int iStart, int j)
{
    int i;
    BOOL fExact;
    int k;
    LISTITEM FAR* pitem;
    LISTITEM FAR* pitemLast = NULL;
    
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

        if (plv->style & LVS_SORTDESCENDING)
            result = -result;

        switch (result)
        {
        case 0:
            fExact = TRUE;
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

int NEAR ListView_LookupString(LV* plv, LPCSTR pszLookup, UINT flags, int iStart)
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
