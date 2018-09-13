// BUGBUG: this code is not used anymore!

#include "shellprv.h"
#include "listview.h"

// Internal STREAM entry points

BOOL Stream_WriteString(STREAM FAR* pstm, LPCTSTR psz);
LPTSTR Stream_ReadString(STREAM FAR* pstm);
UINT Stream_ReadStringBuffer(STREAM FAR* pstm, LPTSTR psz, UINT cb);

// Read or write a ListView to a stream.  flags indicate what aspects     /* ;Internal */
// of the listview to write out.  If aspects of a ListView state are      /* ;Internal */
// not written, default values will be used when read back in.            /* ;Internal */
//                                                                        /* ;Internal */
#define LVRW_ICONS          0x0001                                        /* ;Internal */
#define LVRW_SMALLICONS     0x0002                                        /* ;Internal */
#define LVRW_FONT           0x0004                                        /* ;Internal */
#define LVRW_LPARAMS        0x0008                                        /* ;Internal */
#define LVRW_COLINFO        0x0010                                        /* ;Internal */
#define LVRW_ENUMORDER      0x0020                                        /* ;Internal */
                                                                          /* ;Internal */
    // BOOL ListView_Write(HWND hwndLV, STREAM FAR* pstm, UINT flags);    /* ;Internal */
#define LVM_WRITE           (LVM_FIRST + 31)                              /* ;Internal */
#define ListView_Write(hwndLV, pstm, flags)     /* ;Internal */ \
    (BOOL)SendMessage((hwndLV), LVM_WRITE,      /* ;Internal */ \
    (WPARAM)(BOOL)(flags),                      /* ;Internal */  \
            (LPARAM)(STREAM FAR*)(pstm))                                  /* ;Internal */
                                                                          /* ;Internal */
typedef struct _LV_READINFO                                               /* ;Internal */
{                                                                         /* ;Internal */
    UINT flags;                                                           /* ;Internal */
    HINSTANCE hinst;                                                      /* ;Internal */
    HWND hwndParent;                                                      /* ;Internal */
} LV_READINFO;                                                            /* ;Internal */
                                                                          /* ;Internal */
    // HWND ListView_Read(STREAM FAR* pstm, LV_READINFO FAR* pinfo);      /* ;Internal */
    // BUGBUG This can't be a message!  How do we want this to work?      /* ;Internal */
#define LVM_READ            (LVM_FIRST + 32)                              /* ;Internal */
#define ListView_Read(plv, pinfo)                                         /* ;Internal */
                                                                          /* ;Internal */


#define LV_MAGIC    (TEXT('L') | (TEXT('V') << (8 * sizeof(TCHAR))))

typedef struct _LV_STREAMHDR
{
    UINT magic;
    UINT flags;
    UINT style;
    UINT id;
    POINT ptOrigin;
    COLORREF clrBk;
    int cItem;
} LV_STREAMHDR;

typedef struct _LV_ITEMHDR
{
    POINT pt;
    UINT state;
    int iImage;
    int iZOrder;
} LV_ITEMHDR;

BOOL NEAR ListView_OnWrite(LV* plv, STREAM FAR* pstm, UINT flags)
{
    int i;
    LV_STREAMHDR hdr;

    hdr.magic = LV_MAGIC;
    hdr.flags = flags;
    hdr.style = plv->style;
    hdr.id = GetWindowID(plv->hwnd);
    hdr.ptOrigin = plv->ptOrigin;
    hdr.clrBk = plv->clrBk;
    hdr.cItem = ListView_Count(plv);

    if (!Stream_Write(pstm, &hdr, sizeof(hdr)))
        return FALSE;

    for (i = 0; i < hdr.cItem; i++)
    {
        LV_ITEMHDR ihdr;
        LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);

        ihdr.pt.x = pitem->pt.x;
        ihdr.pt.y = pitem->pt.y;
        ihdr.state = pitem->state;
        ihdr.iImage = pitem->iImage;
        ihdr.iZOrder = ListView_ZOrderIndex(plv, i);

        if (!Stream_Write(pstm, &ihdr, sizeof(ihdr)))
            return FALSE;

        if (flags & LVRW_LPARAMS)
        {
            if (!Stream_Write(pstm, &pitem->lParam, sizeof(pitem->lParam)))
                return FALSE;
        }

        if (!Stream_WriteString(pstm, pitem->pszText))
            return FALSE;
    }

    if (flags & LVRW_FONT)
    {
    // REVIEW: Need to optionally write out log font...
    }

    if (flags & LVRW_ICONS)
    {
        if (!ImageList_Write(plv->himl, pstm))
            return FALSE;
    }

    if (flags & LVRW_SMALLICONS)
    {
        if (!ImageList_Write(plv->himlSmall, pstm))
            return FALSE;
    }

    if (!Stream_Flush(pstm))
        return FALSE;

    return TRUE;
}

HWND NEAR ListView_OnRead(STREAM FAR* pstm, LV_READINFO FAR* pinfo)
{
    HWND hwndLV;
    int i;
    LV* plv;
    LV_STREAMHDR hdr;
    BOOL fSuccess;
    UINT flags = pinfo->flags;

    fSuccess = FALSE;
    hwndLV = NULL;

    if (!Stream_Read(pstm, &hdr, sizeof(hdr)))
        return FALSE;

    if (hdr.magic != LV_MAGIC || hdr.flags != flags)
        return FALSE;

    // REVIEW: Could create window always with LVS_SHAREIMAGELISTS
    // so we don't have to destroy and recreate the imagelists
    // later.  Probably only a negligible speed savings, though.
    //
    hwndLV = CreateWindowEx(
            0L,                         // extendedStyle
            c_szListViewClass,          // class name
            NULL,                       // text
            WS_CHILD | (DWORD)hdr.style,
            0, 0, 0, 0,                 // x, y, cx, cy
            pinfo->hwndParent,                 // hwndParent
            (HMENU)hdr.id,              // child window id
            pinfo->hinst,                      // hInstance
            NULL);

    if (!hwndLV)
        return FALSE;

    plv = ListView_GetPtr(hwndLV);
    if (!plv)
        goto Error;

    plv->ptOrigin = hdr.ptOrigin;
    plv->clrBk = hdr.clrBk;

    // Grow the Z-order array to cItem items...
    //
    for (i = 0; i < hdr.cItem; i++)
    {
        // Add a non-NULL item so we can test return value
        // of ReplaceItem() later...
        //
        if (DPA_InsertPtr(plv->hdpaZOrder, i, (void FAR*)1) == -1)
            goto Error;
    }

    for (i = 0; i < hdr.cItem; i++)
    {
        int i2;
        LV_ITEMHDR ihdr;
        LV_ITEM item;
        LISTITEM FAR* pitem;
        LPTSTR pszText;

        if (!Stream_Read(pstm, &ihdr, sizeof(ihdr)))
            goto Error;

        item.mask = LVIF_ALL;
        item.pszText = NULL;
        item.state = 0;
        item.iImage = ihdr.iImage;
        item.lParam = 0L;

        pitem = ListView_CreateItem(plv, &item);
        if (!pitem)
            goto Error;

        if (flags & LVRW_LPARAMS)
        {
            if (!Stream_Read(pstm, &pitem->lParam, sizeof(pitem->lParam)))
                goto Error;
        }

        pszText = Stream_ReadString(pstm);
        if (!pszText)
        {
            ListView_FreeItem(plv, pitem);
            goto Error;
        }

        pitem->pt.y = (short)ihdr.pt.y;
        pitem->pt.x = (short)ihdr.pt.x;
        pitem->state = ihdr.state;
        pitem->pszText = pszText;

        // If sorted, then insert sorted.
        //
        i2 = i;
        if (plv->style & (LVS_SORTASCENDING | LVS_SORTDESCENDING))
            i2 = ListView_LookupString(plv, pszText, LVFI_SUBSTRING | LVFI_NEARESTXY, 0);

        if (DPA_InsertPtr(plv->hdpa, i2, (void FAR*)pitem) == -1)
        {
            ListView_FreeItem(plv, pitem);
            goto Error;
        }

        // Now set the Z order.
        //
        if (!DPA_SetPtr(plv->hdpaZOrder, ihdr.iZOrder, (void FAR*)i2))
            goto Error;
    }

    if (flags & LVRW_FONT)
    {
        // REVIEW: Need to read & setfont
    }

    if (flags & LVRW_ICONS)
    {
        ImageList_Destroy(plv->himl);

        plv->himl = ImageList_Read(pstm);
        if (!plv->himl)
            goto Error;
    }

    if (flags & LVRW_SMALLICONS)
    {
        ImageList_Destroy(plv->himlSmall);

        plv->himlSmall = ImageList_Read(pstm);
        if (!plv->himlSmall)
            goto Error;
    }

    plv->rcView.left = RECOMPUTE;

    // Instead of sending out a zillion creates (one for each item we just
    // created), just destroy and re-create ourselves.
    ListView_NotifyRecreate(plv);

    fSuccess = TRUE;

Error:
    if (!fSuccess && hwndLV)
    {
        DestroyWindow(hwndLV);
        hwndLV = NULL;
    }
    return hwndLV;
}
