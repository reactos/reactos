/*
 * ComboBoxEx control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 2000, 2001, 2002 Guy Albertelli <galberte@neo.lrun.com>
 * Copyright 2002 Dimitrie O. Paun
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(comboex);

/* Item structure */
typedef struct _CBE_ITEMDATA
{
    struct _CBE_ITEMDATA *next;
    UINT         mask;
    LPWSTR       pszText;
    LPWSTR       pszTemp;
    int          cchTextMax;
    int          iImage;
    int          iSelectedImage;
    int          iOverlay;
    int          iIndent;
    LPARAM       lParam;
} CBE_ITEMDATA;

/* ComboBoxEx structure */
typedef struct
{
    HIMAGELIST   himl;
    HWND         hwndSelf;         /* my own hwnd */
    HWND         hwndNotify;       /* my parent hwnd */
    HWND         hwndCombo;
    HWND         hwndEdit;
    DWORD        dwExtStyle;
    INT          selected;         /* index of selected item */
    DWORD        flags;            /* WINE internal flags */
    HFONT        defaultFont;
    HFONT        font;
    INT          nb_items;         /* Number of items */
    BOOL         unicode;          /* TRUE if this window is Unicode   */
    BOOL         NtfUnicode;       /* TRUE if parent wants notify in Unicode */
    CBE_ITEMDATA edit;             /* item data for edit item */
    CBE_ITEMDATA *items;           /* Array of items */
} COMBOEX_INFO;

/* internal flags in the COMBOEX_INFO structure */
#define  WCBE_ACTEDIT		0x00000001  /* Edit active i.e.
                                             * CBEN_BEGINEDIT issued
                                             * but CBEN_ENDEDIT{A|W}
                                             * not yet issued. */
#define  WCBE_EDITCHG		0x00000002  /* Edit issued EN_CHANGE */
#define  WCBE_EDITHASCHANGED	(WCBE_ACTEDIT | WCBE_EDITCHG)
#define  WCBE_EDITFOCUSED	0x00000004  /* Edit control has focus */
#define  WCBE_MOUSECAPTURED	0x00000008  /* Combo has captured mouse */
#define  WCBE_MOUSEDRAGGED      0x00000010  /* User has dragged in combo */

#define ID_CB_EDIT		1001


/*
 * Special flag set in DRAWITEMSTRUCT itemState field. It is set by
 * the ComboEx version of the Combo Window Proc so that when the
 * WM_DRAWITEM message is then passed to ComboEx, we know that this
 * particular WM_DRAWITEM message is for listbox only items. Any message
 * without this flag is then for the Edit control field.
 *
 * We really cannot use the ODS_COMBOBOXEDIT flag because MSDN states that
 * only version 4.0 applications will have ODS_COMBOBOXEDIT set.
 */
#define ODS_COMBOEXLBOX		0x4000



/* Height in pixels of control over the amount of the selected font */
#define CBE_EXTRA		3

/* Indent amount per MS documentation */
#define CBE_INDENT		10

/* Offset in pixels from left side for start of image or text */
#define CBE_STARTOFFSET		6

/* Offset between image and text */
#define CBE_SEP			4

#define COMBO_SUBCLASSID  1
#define EDIT_SUBCLASSID   2

#define COMBOEX_GetInfoPtr(hwnd) ((COMBOEX_INFO *)GetWindowLongPtrW (hwnd, 0))

static LRESULT CALLBACK COMBOEX_EditWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                             UINT_PTR uId, DWORD_PTR ref_data);
static LRESULT CALLBACK COMBOEX_ComboWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                              UINT_PTR uId, DWORD_PTR ref_data);
static LRESULT COMBOEX_Destroy (COMBOEX_INFO *infoPtr);
typedef INT (WINAPI *cmp_func_t)(LPCWSTR, LPCWSTR);

static inline BOOL is_textW(LPCWSTR str)
{
    return str && str != LPSTR_TEXTCALLBACKW;
}

static inline BOOL is_textA(LPCSTR str)
{
    return str && str != LPSTR_TEXTCALLBACKA;
}

static inline LPCSTR debugstr_txt(LPCWSTR str)
{
    if (str == LPSTR_TEXTCALLBACKW) return "(callback)";
    return debugstr_w(str);
}

static void COMBOEX_DumpItem (CBE_ITEMDATA const *item)
{
    TRACE("item %p - mask=%08x, pszText=%p, cchTM=%d, iImage=%d\n",
          item, item->mask, item->pszText, item->cchTextMax, item->iImage);
    TRACE("item %p - iSelectedImage=%d, iOverlay=%d, iIndent=%d, lParam=%Ix\n",
          item, item->iSelectedImage, item->iOverlay, item->iIndent, item->lParam);
    if (item->mask & CBEIF_TEXT)
        TRACE("item %p - pszText=%s\n", item, debugstr_txt(item->pszText));
}


static void COMBOEX_DumpInput (COMBOBOXEXITEMW const *input)
{
    TRACE("input - mask=%08x, iItem=%Id, pszText=%p, cchTM=%d, iImage=%d\n",
          input->mask, input->iItem, input->pszText, input->cchTextMax,
          input->iImage);
    if (input->mask & CBEIF_TEXT)
        TRACE("input - pszText=<%s>\n", debugstr_txt(input->pszText));
    TRACE("input - iSelectedImage=%d, iOverlay=%d, iIndent=%d, lParam=%Ix\n",
          input->iSelectedImage, input->iOverlay, input->iIndent, input->lParam);
}


static inline CBE_ITEMDATA *get_item_data(const COMBOEX_INFO *infoPtr, INT index)
{
    return (CBE_ITEMDATA *)SendMessageW (infoPtr->hwndCombo, CB_GETITEMDATA,
                                         index, 0);
}

static inline cmp_func_t get_cmp_func(COMBOEX_INFO const *infoPtr)
{
    return infoPtr->dwExtStyle & CBES_EX_CASESENSITIVE ? lstrcmpW : lstrcmpiW;
}

static INT COMBOEX_Notify (const COMBOEX_INFO *infoPtr, INT code, NMHDR *hdr)
{
    hdr->idFrom = GetDlgCtrlID (infoPtr->hwndSelf);
    hdr->hwndFrom = infoPtr->hwndSelf;
    hdr->code = code;
    if (infoPtr->NtfUnicode)
	return SendMessageW (infoPtr->hwndNotify, WM_NOTIFY, 0, (LPARAM)hdr);
    else
	return SendMessageA (infoPtr->hwndNotify, WM_NOTIFY, 0, (LPARAM)hdr);
}


static INT
COMBOEX_NotifyItem (const COMBOEX_INFO *infoPtr, UINT code, NMCOMBOBOXEXW *hdr)
{
    /* Change the Text item from Unicode to ANSI if necessary for NOTIFY */
    if (infoPtr->NtfUnicode)
	return COMBOEX_Notify (infoPtr, code, &hdr->hdr);
    else {
	LPWSTR wstr = hdr->ceItem.pszText;
	LPSTR astr = 0;
	INT ret, len = 0;

	if ((hdr->ceItem.mask & CBEIF_TEXT) && is_textW(wstr)) {
	    len = WideCharToMultiByte (CP_ACP, 0, wstr, -1, 0, 0, NULL, NULL);
	    if (len > 0) {
                astr = Alloc ((len + 1)*sizeof(CHAR));
		if (!astr) return 0;
		WideCharToMultiByte (CP_ACP, 0, wstr, -1, astr, len, 0, 0);
		hdr->ceItem.pszText = (LPWSTR)astr;
	    }
	}

	if (code == CBEN_ENDEDITW) code = CBEN_ENDEDITA;
	else if (code == CBEN_GETDISPINFOW) code = CBEN_GETDISPINFOA;
	else if (code == CBEN_DRAGBEGINW) code = CBEN_DRAGBEGINA;

	ret = COMBOEX_Notify (infoPtr, code, (NMHDR *)hdr);

	if (astr && hdr->ceItem.pszText == (LPWSTR)astr)
	    hdr->ceItem.pszText = wstr;

	Free(astr);

	return ret;
    }
}


static INT COMBOEX_NotifyEndEdit (const COMBOEX_INFO *infoPtr, NMCBEENDEDITW *neew, LPCWSTR wstr)
{
    /* Change the Text item from Unicode to ANSI if necessary for NOTIFY */
    if (infoPtr->NtfUnicode) {
	lstrcpynW(neew->szText, wstr, CBEMAXSTRLEN);
	return COMBOEX_Notify (infoPtr, CBEN_ENDEDITW, &neew->hdr);
    } else {
	NMCBEENDEDITA neea;

        neea.hdr = neew->hdr;
        neea.fChanged = neew->fChanged;
        neea.iNewSelection = neew->iNewSelection;
        WideCharToMultiByte (CP_ACP, 0, wstr, -1, neea.szText, CBEMAXSTRLEN, 0, 0);
        neea.iWhy = neew->iWhy;

        return COMBOEX_Notify (infoPtr, CBEN_ENDEDITA, &neea.hdr);
    }
}


static void COMBOEX_NotifyDragBegin(const COMBOEX_INFO *infoPtr, LPCWSTR wstr)
{
    /* Change the Text item from Unicode to ANSI if necessary for NOTIFY */
    if (infoPtr->NtfUnicode) {
        NMCBEDRAGBEGINW ndbw;

	ndbw.iItemid = -1;
	lstrcpynW(ndbw.szText, wstr, CBEMAXSTRLEN);
	COMBOEX_Notify (infoPtr, CBEN_DRAGBEGINW, &ndbw.hdr);
    } else {
	NMCBEDRAGBEGINA ndba;

	ndba.iItemid = -1;
	WideCharToMultiByte (CP_ACP, 0, wstr, -1, ndba.szText, CBEMAXSTRLEN, 0, 0);

	COMBOEX_Notify (infoPtr, CBEN_DRAGBEGINA, &ndba.hdr);
    }
}


static void COMBOEX_FreeText (CBE_ITEMDATA *item)
{
    if (is_textW(item->pszText)) Free(item->pszText);
    item->pszText = NULL;
    Free(item->pszTemp);
    item->pszTemp = NULL;
}


static INT COMBOEX_GetIndex(COMBOEX_INFO const *infoPtr, CBE_ITEMDATA const *item)
{
    CBE_ITEMDATA const *moving;
    INT index;

    moving = infoPtr->items;
    index = infoPtr->nb_items - 1;

    while (moving && (moving != item)) {
        moving = moving->next;
        index--;
    }
    if (!moving || (index < 0)) {
        ERR("COMBOBOXEX item structures broken. Please report!\n");
        return -1;
    }
    return index;
}


static LPCWSTR COMBOEX_GetText(const COMBOEX_INFO *infoPtr, CBE_ITEMDATA *item)
{
    NMCOMBOBOXEXW nmce;
    LPWSTR text, buf;
    INT len;

    if (item->pszText != LPSTR_TEXTCALLBACKW)
	return item->pszText;

    ZeroMemory(&nmce, sizeof(nmce));
    nmce.ceItem.mask = CBEIF_TEXT;
    nmce.ceItem.lParam = item->lParam;
    nmce.ceItem.iItem = COMBOEX_GetIndex(infoPtr, item);
    COMBOEX_NotifyItem(infoPtr, CBEN_GETDISPINFOW, &nmce);

    if (is_textW(nmce.ceItem.pszText)) {
	len = MultiByteToWideChar (CP_ACP, 0, (LPSTR)nmce.ceItem.pszText, -1, NULL, 0);
        buf = Alloc ((len + 1)*sizeof(WCHAR));
	if (buf)
	    MultiByteToWideChar (CP_ACP, 0, (LPSTR)nmce.ceItem.pszText, -1, buf, len);
	if (nmce.ceItem.mask & CBEIF_DI_SETITEM) {
	    COMBOEX_FreeText(item);
	    item->pszText = buf;
	} else {
	    Free(item->pszTemp);
	    item->pszTemp = buf;
	}
	text = buf;
    } else
	text = nmce.ceItem.pszText;

    if (nmce.ceItem.mask & CBEIF_DI_SETITEM)
	item->pszText = text;
    return text;
}


static void COMBOEX_GetComboFontSize (const COMBOEX_INFO *infoPtr, SIZE *size)
{
    HFONT nfont, ofont;
    HDC mydc;

    mydc = GetDC (0); /* why the entire screen???? */
    nfont = (HFONT)SendMessageW (infoPtr->hwndCombo, WM_GETFONT, 0, 0);
    ofont = SelectObject (mydc, nfont);
    GetTextExtentPointW (mydc, L"A", 1, size);
    SelectObject (mydc, ofont);
    ReleaseDC (0, mydc);
    TRACE("selected font hwnd %p, height %ld\n", nfont, size->cy);
}


static void COMBOEX_CopyItem (const CBE_ITEMDATA *item, COMBOBOXEXITEMW *cit)
{
    if (cit->mask & CBEIF_TEXT) {
        /*
         * when given a text buffer actually use that buffer
         */
        if (cit->pszText) {
	    if (is_textW(item->pszText))
                lstrcpynW(cit->pszText, item->pszText, cit->cchTextMax);
	    else
		cit->pszText[0] = 0;
        } else {
            cit->pszText        = item->pszText;
            cit->cchTextMax     = item->cchTextMax;
        }
    }
    if (cit->mask & CBEIF_IMAGE)
	cit->iImage         = item->iImage;
    if (cit->mask & CBEIF_SELECTEDIMAGE)
	cit->iSelectedImage = item->iSelectedImage;
    if (cit->mask & CBEIF_OVERLAY)
	cit->iOverlay       = item->iOverlay;
    if (cit->mask & CBEIF_INDENT)
	cit->iIndent        = item->iIndent;
    if (cit->mask & CBEIF_LPARAM)
	cit->lParam         = item->lParam;
}


static void COMBOEX_AdjustEditPos (const COMBOEX_INFO *infoPtr)
{
    SIZE mysize;
    INT x, y, w, h, xioff;
    RECT rect;

    if (!infoPtr->hwndEdit) return;

    if (infoPtr->himl && !(infoPtr->dwExtStyle & CBES_EX_NOEDITIMAGEINDENT)) {
    	IMAGEINFO iinfo;
        iinfo.rcImage.left = iinfo.rcImage.right = 0;
	ImageList_GetImageInfo(infoPtr->himl, 0, &iinfo);
	xioff = iinfo.rcImage.right - iinfo.rcImage.left + CBE_SEP;
    }  else xioff = 0;

    GetClientRect (infoPtr->hwndCombo, &rect);
    InflateRect (&rect, -2, -2);
    InvalidateRect (infoPtr->hwndCombo, &rect, TRUE);

    /* reposition the Edit control based on whether icon exists */
    COMBOEX_GetComboFontSize (infoPtr, &mysize);
    TRACE("Combo font x %ld, y %ld\n", mysize.cx, mysize.cy);
    x = xioff + CBE_STARTOFFSET + 1;
    w = rect.right-rect.left - x - GetSystemMetrics(SM_CXVSCROLL) - 1;
    h = mysize.cy + 1;
    y = rect.bottom - h - 1;

    TRACE("Combo client (%s), setting Edit to (%d,%d)-(%d,%d)\n",
          wine_dbgstr_rect(&rect), x, y, x + w, y + h);
    SetWindowPos(infoPtr->hwndEdit, HWND_TOP, x, y, w, h,
		 SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
}


static void COMBOEX_ReSize (const COMBOEX_INFO *infoPtr)
{
    SIZE mysize;
    LONG cy;
    IMAGEINFO iinfo;

    COMBOEX_GetComboFontSize (infoPtr, &mysize);
    cy = mysize.cy + CBE_EXTRA;
    if (infoPtr->himl && ImageList_GetImageInfo(infoPtr->himl, 0, &iinfo)) {
	cy = max (iinfo.rcImage.bottom - iinfo.rcImage.top, cy);
	TRACE("upgraded height due to image: height %ld\n", cy);
    }
    SendMessageW (infoPtr->hwndSelf, CB_SETITEMHEIGHT, -1, cy);
    if (infoPtr->hwndCombo) {
        SendMessageW (infoPtr->hwndCombo, CB_SETITEMHEIGHT, 0, cy);
	if ( !(infoPtr->flags & CBES_EX_NOSIZELIMIT)) {
	    RECT comboRect, ourRect;
	    GetWindowRect(infoPtr->hwndCombo, &comboRect);
            GetWindowRect(infoPtr->hwndSelf, &ourRect);
            if (comboRect.bottom > ourRect.bottom)
                SetWindowPos( infoPtr->hwndSelf, 0, 0, 0, ourRect.right - ourRect.left,
                              comboRect.bottom - comboRect.top,
                              SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW );
	}
    }
}


static void COMBOEX_SetEditText (const COMBOEX_INFO *infoPtr, CBE_ITEMDATA *item)
{
    if (!infoPtr->hwndEdit) return;

    if (item->mask & CBEIF_TEXT) {
	SendMessageW (infoPtr->hwndEdit, WM_SETTEXT, 0, (LPARAM)COMBOEX_GetText(infoPtr, item));
	SendMessageW (infoPtr->hwndEdit, EM_SETSEL, 0, 0);
	SendMessageW (infoPtr->hwndEdit, EM_SETSEL, 0, -1);
    }
}


static CBE_ITEMDATA *COMBOEX_FindItem(COMBOEX_INFO *infoPtr, INT_PTR index)
{
    CBE_ITEMDATA *item;
    INT i;

    if ((index >= infoPtr->nb_items) || (index < -1))
	return NULL;
    if (index == -1)
	return &infoPtr->edit;
    item = infoPtr->items;
    i = infoPtr->nb_items - 1;

    /* find the item in the list */
    while (item && (i > index)) {
	item = item->next;
	i--;
    }
    if (!item || (i != index)) {
	ERR("COMBOBOXEX item structures broken. Please report!\n");
	return 0;
    }
    return item;
}

/* ***  CBEM_xxx message support  *** */

static UINT COMBOEX_GetListboxText(COMBOEX_INFO *infoPtr, INT_PTR n, LPWSTR buf)
{
    CBE_ITEMDATA *item;
    LPCWSTR str;

    item = COMBOEX_FindItem(infoPtr, n);
    if (!item)
        return 0;

    str = COMBOEX_GetText(infoPtr, item);
    if (!str)
    {
        if (buf)
        {
            if (infoPtr->unicode)
                buf[0] = 0;
            else
                *((LPSTR)buf) = 0;
        }
        return 0;
    }

    if (infoPtr->unicode)
    {
        if (buf)
            lstrcpyW(buf, str);
        return lstrlenW(str);
    }
    else
    {
        UINT r;
        r = WideCharToMultiByte(CP_ACP, 0, str, -1, (LPSTR)buf, 0x40000000, NULL, NULL);
        if (r) r--;
        return r;
    }
}


static INT COMBOEX_DeleteItem (COMBOEX_INFO *infoPtr, INT_PTR index)
{
    TRACE("index %Id\n", index);

    /* if item number requested does not exist then return failure */
    if ((index >= infoPtr->nb_items) || (index < 0)) return CB_ERR;
    if (!COMBOEX_FindItem(infoPtr, index)) return CB_ERR;

    /* doing this will result in WM_DELETEITEM being issued */
    SendMessageW (infoPtr->hwndCombo, CB_DELETESTRING, index, 0);

    return infoPtr->nb_items;
}


static BOOL COMBOEX_GetItemW (COMBOEX_INFO *infoPtr, COMBOBOXEXITEMW *cit)
{
    INT_PTR index = cit->iItem;
    CBE_ITEMDATA *item;

    TRACE("\n");

    /* if item number requested does not exist then return failure */
    if ((index >= infoPtr->nb_items) || (index < -1)) return FALSE;

    /* if the item is the edit control and there is no edit control, skip */
    if ((index == -1) && !infoPtr->hwndEdit) return FALSE;

    if (!(item = COMBOEX_FindItem(infoPtr, index))) return FALSE;

    COMBOEX_CopyItem (item, cit);

    return TRUE;
}


static BOOL COMBOEX_GetItemA (COMBOEX_INFO *infoPtr, COMBOBOXEXITEMA *cit)
{
    COMBOBOXEXITEMW tmpcit;

    TRACE("\n");

    tmpcit.mask = cit->mask;
    tmpcit.iItem = cit->iItem;
    tmpcit.pszText = 0;
    if(!COMBOEX_GetItemW (infoPtr, &tmpcit)) return FALSE;

    if (cit->mask & CBEIF_TEXT)
    {
        if (is_textW(tmpcit.pszText) && cit->pszText)
            WideCharToMultiByte(CP_ACP, 0, tmpcit.pszText, -1,
                                cit->pszText, cit->cchTextMax, NULL, NULL);
        else if (cit->pszText) cit->pszText[0] = 0;
        else cit->pszText = (LPSTR)tmpcit.pszText;
    }

    if (cit->mask & CBEIF_IMAGE)
        cit->iImage = tmpcit.iImage;
    if (cit->mask & CBEIF_SELECTEDIMAGE)
        cit->iSelectedImage = tmpcit.iSelectedImage;
    if (cit->mask & CBEIF_OVERLAY)
        cit->iOverlay = tmpcit.iOverlay;
    if (cit->mask & CBEIF_INDENT)
        cit->iIndent = tmpcit.iIndent;
    if (cit->mask & CBEIF_LPARAM)
        cit->lParam = tmpcit.lParam;

    return TRUE;
}


static inline BOOL COMBOEX_HasEditChanged (COMBOEX_INFO const *infoPtr)
{
    return infoPtr->hwndEdit && (infoPtr->flags & WCBE_EDITHASCHANGED) == WCBE_EDITHASCHANGED;
}


static INT COMBOEX_InsertItemW (COMBOEX_INFO *infoPtr, COMBOBOXEXITEMW const *cit)
{
    INT_PTR index;
    CBE_ITEMDATA *item;
    NMCOMBOBOXEXW nmcit;

    TRACE("\n");

    if (TRACE_ON(comboex)) COMBOEX_DumpInput (cit);

    /* get real index of item to insert */
    index = cit->iItem;
    if (index == -1) index = infoPtr->nb_items;
    if (index > infoPtr->nb_items) return -1;

    /* get zero-filled space and chain it in */
    if(!(item = Alloc (sizeof(*item)))) return -1;

    /* locate position to insert new item in */
    if (index == infoPtr->nb_items) {
        /* fast path for iItem = -1 */
        item->next = infoPtr->items;
	infoPtr->items = item;
    }
    else {
        INT i = infoPtr->nb_items-1;
	CBE_ITEMDATA *moving = infoPtr->items;

	while ((i > index) && moving) {
	    moving = moving->next;
	    i--;
	}
	if (!moving) {
	    ERR("COMBOBOXEX item structures broken. Please report!\n");
	    Free(item);
	    return -1;
	}
	item->next = moving->next;
	moving->next = item;
    }

    /* fill in our hidden item structure */
    item->mask = cit->mask;
    if (item->mask & CBEIF_TEXT) {
	INT len = 0;

        if (is_textW(cit->pszText)) len = lstrlenW (cit->pszText);
	if (len > 0) {
            item->pszText = Alloc ((len + 1)*sizeof(WCHAR));
	    if (!item->pszText) {
		Free(item);
		return -1;
	    }
	    lstrcpyW (item->pszText, cit->pszText);
	}
	else if (cit->pszText == LPSTR_TEXTCALLBACKW)
	    item->pszText = LPSTR_TEXTCALLBACKW;
        item->cchTextMax = cit->cchTextMax;
    }
    if (item->mask & CBEIF_IMAGE)
        item->iImage = cit->iImage;
    if (item->mask & CBEIF_SELECTEDIMAGE)
        item->iSelectedImage = cit->iSelectedImage;
    if (item->mask & CBEIF_OVERLAY)
        item->iOverlay = cit->iOverlay;
    if (item->mask & CBEIF_INDENT)
        item->iIndent = cit->iIndent;
    if (item->mask & CBEIF_LPARAM)
        item->lParam = cit->lParam;
    infoPtr->nb_items++;

    if (TRACE_ON(comboex)) COMBOEX_DumpItem (item);

    SendMessageW (infoPtr->hwndCombo, CB_INSERTSTRING, cit->iItem, (LPARAM)item);

    memset (&nmcit.ceItem, 0, sizeof(nmcit.ceItem));
    nmcit.ceItem.mask=~0;
    COMBOEX_CopyItem (item, &nmcit.ceItem);
    COMBOEX_NotifyItem (infoPtr, CBEN_INSERTITEM, &nmcit);

    return index;

}


static INT COMBOEX_InsertItemA (COMBOEX_INFO *infoPtr, COMBOBOXEXITEMA const *cit)
{
    COMBOBOXEXITEMW citW;
    LPWSTR wstr = NULL;
    INT	ret;

    memcpy(&citW,cit,sizeof(COMBOBOXEXITEMA));
    if (cit->mask & CBEIF_TEXT && is_textA(cit->pszText)) {
	INT len = MultiByteToWideChar (CP_ACP, 0, cit->pszText, -1, NULL, 0);
        wstr = Alloc ((len + 1)*sizeof(WCHAR));
	if (!wstr) return -1;
	MultiByteToWideChar (CP_ACP, 0, cit->pszText, -1, wstr, len);
	citW.pszText = wstr;
    }
    ret = COMBOEX_InsertItemW(infoPtr, &citW);

    Free(wstr);

    return ret;
}


static DWORD
COMBOEX_SetExtendedStyle (COMBOEX_INFO *infoPtr, DWORD mask, DWORD style)
{
    DWORD dwTemp;

    TRACE("mask %#lx, style %#lx\n", mask, style);

    dwTemp = infoPtr->dwExtStyle;

    if (mask)
	infoPtr->dwExtStyle = (infoPtr->dwExtStyle & ~mask) | style;
    else
	infoPtr->dwExtStyle = style;

    /* see if we need to change the word break proc on the edit */
    if ((infoPtr->dwExtStyle ^ dwTemp) & CBES_EX_PATHWORDBREAKPROC)
        SetPathWordBreakProc(infoPtr->hwndEdit, 
            (infoPtr->dwExtStyle & CBES_EX_PATHWORDBREAKPROC) != 0);

    /* test if the control's appearance has changed */
    mask = CBES_EX_NOEDITIMAGE | CBES_EX_NOEDITIMAGEINDENT;
    if ((infoPtr->dwExtStyle & mask) != (dwTemp & mask)) {
	/* if state of EX_NOEDITIMAGE changes, invalidate all */
	TRACE("EX_NOEDITIMAGE state changed to %#lx\n", infoPtr->dwExtStyle & CBES_EX_NOEDITIMAGE);
	InvalidateRect (infoPtr->hwndSelf, NULL, TRUE);
	COMBOEX_AdjustEditPos (infoPtr);
	if (infoPtr->hwndEdit)
	    InvalidateRect (infoPtr->hwndEdit, NULL, TRUE);
    }

    return dwTemp;
}


static HIMAGELIST COMBOEX_SetImageList (COMBOEX_INFO *infoPtr, HIMAGELIST himl)
{
    HIMAGELIST himlTemp = infoPtr->himl;

    TRACE("\n");

    infoPtr->himl = himl;

    COMBOEX_ReSize (infoPtr);
    InvalidateRect (infoPtr->hwndCombo, NULL, TRUE);

    /* reposition the Edit control based on whether icon exists */
    COMBOEX_AdjustEditPos (infoPtr);
    return himlTemp;
}

static BOOL COMBOEX_SetItemW (COMBOEX_INFO *infoPtr, const COMBOBOXEXITEMW *cit)
{
    INT_PTR index = cit->iItem;
    CBE_ITEMDATA *item;

    if (TRACE_ON(comboex)) COMBOEX_DumpInput (cit);

    /* if item number requested does not exist then return failure */
    if ((index >= infoPtr->nb_items) || (index < -1)) return FALSE;

    /* if the item is the edit control and there is no edit control, skip */
    if ((index == -1) && !infoPtr->hwndEdit) return FALSE;

    if (!(item = COMBOEX_FindItem(infoPtr, index))) return FALSE;

    /* add/change stuff to the internal item structure */
    item->mask |= cit->mask;
    if (cit->mask & CBEIF_TEXT) {
	INT len = 0;

	COMBOEX_FreeText(item);
        if (is_textW(cit->pszText)) len = lstrlenW(cit->pszText);
	if (len > 0) {
            item->pszText = Alloc ((len + 1)*sizeof(WCHAR));
	    if (!item->pszText) return FALSE;
	    lstrcpyW(item->pszText, cit->pszText);
	} else if (cit->pszText == LPSTR_TEXTCALLBACKW)
	    item->pszText = LPSTR_TEXTCALLBACKW;
        item->cchTextMax = cit->cchTextMax;
    }
    if (cit->mask & CBEIF_IMAGE)
        item->iImage = cit->iImage;
    if (cit->mask & CBEIF_SELECTEDIMAGE)
        item->iSelectedImage = cit->iSelectedImage;
    if (cit->mask & CBEIF_OVERLAY)
        item->iOverlay = cit->iOverlay;
    if (cit->mask & CBEIF_INDENT)
        item->iIndent = cit->iIndent;
    if (cit->mask & CBEIF_LPARAM)
        item->lParam = cit->lParam;

    if (TRACE_ON(comboex)) COMBOEX_DumpItem (item);

    /* if original request was to update edit control, do some fast foot work */
    if (cit->iItem == -1 && cit->mask & CBEIF_TEXT) {
	COMBOEX_SetEditText (infoPtr, item);
	RedrawWindow (infoPtr->hwndCombo, 0, 0, RDW_ERASE | RDW_INVALIDATE);
    }
    return TRUE;
}

static BOOL COMBOEX_SetItemA (COMBOEX_INFO *infoPtr, COMBOBOXEXITEMA const *cit)
{
    COMBOBOXEXITEMW citW;
    LPWSTR wstr = NULL;
    BOOL ret;

    memcpy(&citW, cit, sizeof(COMBOBOXEXITEMA));
    if ((cit->mask & CBEIF_TEXT) && is_textA(cit->pszText)) {
	INT len = MultiByteToWideChar (CP_ACP, 0, cit->pszText, -1, NULL, 0);
        wstr = Alloc ((len + 1)*sizeof(WCHAR));
	if (!wstr) return FALSE;
	MultiByteToWideChar (CP_ACP, 0, cit->pszText, -1, wstr, len);
	citW.pszText = wstr;
    }
    ret = COMBOEX_SetItemW(infoPtr, &citW);

    Free(wstr);

    return ret;
}


static BOOL COMBOEX_SetUnicodeFormat (COMBOEX_INFO *infoPtr, BOOL value)
{
    BOOL bTemp = infoPtr->unicode;

    TRACE("to %s, was %s\n", value ? "TRUE":"FALSE", bTemp ? "TRUE":"FALSE");

    infoPtr->unicode = value;

    return bTemp;
}


/* ***  CB_xxx message support  *** */

static INT
COMBOEX_FindStringExact (const COMBOEX_INFO *infoPtr, INT start, LPCWSTR str)
{
    INT i;
    cmp_func_t cmptext = get_cmp_func(infoPtr);
    INT count = SendMessageW (infoPtr->hwndCombo, CB_GETCOUNT, 0, 0);

    /* now search from after starting loc and wrapping back to start */
    for(i=start+1; i<count; i++) {
	CBE_ITEMDATA *item = get_item_data(infoPtr, i);
	if ((LRESULT)item == CB_ERR) continue;
	if (cmptext(COMBOEX_GetText(infoPtr, item), str) == 0) return i;
    }
    for(i=0; i<=start; i++) {
	CBE_ITEMDATA *item = get_item_data(infoPtr, i);
	if ((LRESULT)item == CB_ERR) continue;
	if (cmptext(COMBOEX_GetText(infoPtr, item), str) == 0) return i;
    }
    return CB_ERR;
}


static DWORD_PTR COMBOEX_GetItemData (COMBOEX_INFO *infoPtr, INT_PTR index)
{
    CBE_ITEMDATA const *item1;
    CBE_ITEMDATA const *item2;
    DWORD_PTR ret = 0;

    item1 = get_item_data(infoPtr, index);
    if ((item1 != NULL) && ((LRESULT)item1 != CB_ERR)) {
	item2 = COMBOEX_FindItem (infoPtr, index);
	if (item2 != item1) {
	    ERR("data structures damaged!\n");
	    return CB_ERR;
	}
	if (item1->mask & CBEIF_LPARAM) ret = item1->lParam;
	TRACE("returning %#Ix\n", ret);
    } else {
        ret = (DWORD_PTR)item1;
        TRACE("non-valid result from combo, returning %#Ix\n", ret);
    }
    return ret;
}


static INT COMBOEX_SetCursel (COMBOEX_INFO *infoPtr, INT_PTR index)
{
    CBE_ITEMDATA *item;
    INT sel;

    if (!(item = COMBOEX_FindItem(infoPtr, index)))
	return SendMessageW (infoPtr->hwndCombo, CB_SETCURSEL, index, 0);

    TRACE("selecting item %Id text=%s\n", index, debugstr_txt(item->pszText));
    infoPtr->selected = index;

    sel = (INT)SendMessageW (infoPtr->hwndCombo, CB_SETCURSEL, index, 0);
    COMBOEX_SetEditText (infoPtr, item);
    return sel;
}


static DWORD_PTR COMBOEX_SetItemData (COMBOEX_INFO *infoPtr, INT_PTR index, DWORD_PTR data)
{
    CBE_ITEMDATA *item1;
    CBE_ITEMDATA const *item2;

    item1 = get_item_data(infoPtr, index);
    if ((item1 != NULL) && ((LRESULT)item1 != CB_ERR)) {
	item2 = COMBOEX_FindItem (infoPtr, index);
	if (item2 != item1) {
	    ERR("data structures damaged!\n");
	    return CB_ERR;
	}
	item1->mask |= CBEIF_LPARAM;
	item1->lParam = data;
	TRACE("setting lparam to %#Ix\n", data);
	return 0;
    }
    TRACE("non-valid result from combo %p\n", item1);
    return (DWORD_PTR)item1;
}


static INT COMBOEX_SetItemHeight (COMBOEX_INFO const *infoPtr, INT index, UINT height)
{
    RECT cb_wrect, cbx_wrect, cbx_crect;

    /* First, lets forward the message to the normal combo control
       just like Windows.     */
    if (infoPtr->hwndCombo)
       if (SendMessageW (infoPtr->hwndCombo, CB_SETITEMHEIGHT,
			 index, height) == CB_ERR) return CB_ERR;

    GetWindowRect (infoPtr->hwndCombo, &cb_wrect);
    GetWindowRect (infoPtr->hwndSelf, &cbx_wrect);
    GetClientRect (infoPtr->hwndSelf, &cbx_crect);
    /* the height of comboex as height of the combo + comboex border */
    height = cb_wrect.bottom-cb_wrect.top
             + cbx_wrect.bottom-cbx_wrect.top
             - (cbx_crect.bottom-cbx_crect.top);
    TRACE("EX window=(%s), client=(%s)\n",
          wine_dbgstr_rect(&cbx_wrect), wine_dbgstr_rect(&cbx_crect));
    TRACE("CB window=(%s), EX setting=(0,0)-(%ld,%d)\n",
          wine_dbgstr_rect(&cbx_wrect), cbx_wrect.right-cbx_wrect.left, height);
    SetWindowPos (infoPtr->hwndSelf, HWND_TOP, 0, 0,
		  cbx_wrect.right-cbx_wrect.left, height,
		  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);

    return 0;
}


/* ***  WM_xxx message support  *** */


static LRESULT COMBOEX_Create (HWND hwnd, CREATESTRUCTA const *cs)
{
    COMBOEX_INFO *infoPtr;
    LOGFONTW mylogfont;
    RECT win_rect;
    INT i;

    /* allocate memory for info structure */
    infoPtr = Alloc (sizeof(COMBOEX_INFO));
    if (!infoPtr) return -1;

    /* initialize info structure */
    /* note that infoPtr is allocated zero-filled */

    infoPtr->hwndSelf = hwnd;
    infoPtr->selected = -1;

    infoPtr->unicode = IsWindowUnicode (hwnd);
    infoPtr->hwndNotify = cs->hwndParent;

    i = SendMessageW(infoPtr->hwndNotify, WM_NOTIFYFORMAT, (WPARAM)hwnd, NF_QUERY);
    if ((i != NFR_ANSI) && (i != NFR_UNICODE)) {
	WARN("wrong response to WM_NOTIFYFORMAT (%d), assuming ANSI\n", i);
	i = NFR_ANSI;
    }
    infoPtr->NtfUnicode = (i == NFR_UNICODE);

    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    if (TRACE_ON(comboex)) {
	RECT client, rect;
	GetWindowRect(hwnd, &rect);
	GetClientRect(hwnd, &client);
	TRACE("EX window=(%s), client=(%s)\n",
		wine_dbgstr_rect(&rect), wine_dbgstr_rect(&client));
    }

    /* Native version of ComboEx creates the ComboBox with DROPDOWNLIST */
    /* specified. It then creates its own version of the EDIT control   */
    /* and makes the ComboBox the parent. This is because a normal      */
    /* DROPDOWNLIST does not have an EDIT control, but we need one.     */
    /* We also need to place the edit control at the proper location    */
    /* (allow space for the icons).                                     */

    infoPtr->hwndCombo = CreateWindowW (WC_COMBOBOXW, L"",
                         WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL |
                         CBS_NOINTEGRALHEIGHT | CBS_DROPDOWNLIST |
			 WS_CHILD | WS_VISIBLE | CBS_OWNERDRAWFIXED |
			 GetWindowLongW (hwnd, GWL_STYLE),
			 cs->y, cs->x, cs->cx, cs->cy, hwnd,
			 (HMENU) GetWindowLongPtrW (hwnd, GWLP_ID),
			 (HINSTANCE)GetWindowLongPtrW (hwnd, GWLP_HINSTANCE), NULL);

    SetWindowSubclass(infoPtr->hwndCombo, COMBOEX_ComboWndProc, COMBO_SUBCLASSID,
                      (DWORD_PTR)hwnd);
    infoPtr->font = (HFONT)SendMessageW (infoPtr->hwndCombo, WM_GETFONT, 0, 0);

    /*
     * Now create our own EDIT control so we can position it.
     * It is created only for CBS_DROPDOWN style
     */
    if ((cs->style & CBS_DROPDOWNLIST) == CBS_DROPDOWN) {
	infoPtr->hwndEdit = CreateWindowExW (0, WC_EDITW, L"",
		    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | ES_AUTOHSCROLL,
		    0, 0, 0, 0,  /* will set later */
		    infoPtr->hwndCombo,
		    (HMENU) GetWindowLongPtrW (hwnd, GWLP_ID),
		    (HINSTANCE)GetWindowLongPtrW (hwnd, GWLP_HINSTANCE), NULL);

	SetWindowSubclass(infoPtr->hwndEdit, COMBOEX_EditWndProc, EDIT_SUBCLASSID,
	                  (DWORD_PTR)hwnd);

	infoPtr->font = (HFONT)SendMessageW(infoPtr->hwndCombo, WM_GETFONT, 0, 0);
    }

    /*
     * Locate the default font if necessary and then set it in
     * all associated controls
     */
    if (!infoPtr->font) {
	SystemParametersInfoW (SPI_GETICONTITLELOGFONT, sizeof(mylogfont),
			       &mylogfont, 0);
	infoPtr->font = infoPtr->defaultFont = CreateFontIndirectW (&mylogfont);
    }
    SendMessageW (infoPtr->hwndCombo, WM_SETFONT, (WPARAM)infoPtr->font, 0);
    if (infoPtr->hwndEdit) {
	SendMessageW (infoPtr->hwndEdit, WM_SETFONT, (WPARAM)infoPtr->font, 0);
       SendMessageW (infoPtr->hwndEdit, EM_SETMARGINS, EC_USEFONTINFO, 0);
    }

    COMBOEX_ReSize (infoPtr);

    /* Above is fairly certain, below is much less certain. */

    GetWindowRect(hwnd, &win_rect);

    if (TRACE_ON(comboex)) {
	RECT client, rect;
	GetClientRect(hwnd, &client);
	GetWindowRect(infoPtr->hwndCombo, &rect);
	TRACE("EX window=(%s) client=(%s) CB wnd=(%s)\n",
		wine_dbgstr_rect(&win_rect), wine_dbgstr_rect(&client),
		wine_dbgstr_rect(&rect));
    }
    SetWindowPos(infoPtr->hwndCombo, HWND_TOP, 0, 0,
		 win_rect.right - win_rect.left, win_rect.bottom - win_rect.top,
		 SWP_NOACTIVATE | SWP_NOREDRAW);

    GetWindowRect(infoPtr->hwndCombo, &win_rect);
    TRACE("CB window=(%s)\n", wine_dbgstr_rect(&win_rect));
    SetWindowPos(hwnd, HWND_TOP, 0, 0,
		 win_rect.right - win_rect.left, win_rect.bottom - win_rect.top,
		 SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);

    COMBOEX_AdjustEditPos (infoPtr);

    return 0;
}


static LRESULT COMBOEX_Command (COMBOEX_INFO *infoPtr, WPARAM wParam)
{
    LRESULT lret;
    INT command = HIWORD(wParam);
    CBE_ITEMDATA *item = 0;
    WCHAR wintext[520];
    INT cursel, n;
    INT_PTR oldItem;
    NMCBEENDEDITW cbeend;
    DWORD oldflags;
    HWND parent = infoPtr->hwndNotify;

    TRACE("for command %d\n", command);

    switch (command)
    {
    case CBN_DROPDOWN:
        SetFocus (infoPtr->hwndCombo);
        ShowWindow (infoPtr->hwndEdit, SW_HIDE);
        infoPtr->flags |= WCBE_ACTEDIT;
        return SendMessageW (parent, WM_COMMAND, wParam, (LPARAM)infoPtr->hwndSelf);
    case CBN_CLOSEUP:
	SendMessageW (parent, WM_COMMAND, wParam, (LPARAM)infoPtr->hwndSelf);
	ShowWindow (infoPtr->hwndEdit, SW_SHOW);
	InvalidateRect (infoPtr->hwndCombo, 0, TRUE);
	if (infoPtr->hwndEdit) InvalidateRect (infoPtr->hwndEdit, 0, TRUE);
	cursel = SendMessageW (infoPtr->hwndCombo, CB_GETCURSEL, 0, 0);
	if (cursel == -1) {
            cmp_func_t cmptext = get_cmp_func(infoPtr);
	    /* find match from edit against those in Combobox */
	    GetWindowTextW (infoPtr->hwndEdit, wintext, 520);
	    n = SendMessageW (infoPtr->hwndCombo, CB_GETCOUNT, 0, 0);
	    for (cursel = 0; cursel < n; cursel++){
                item = get_item_data(infoPtr, cursel);
		if ((INT_PTR)item == CB_ERR) break;
		if (!cmptext(COMBOEX_GetText(infoPtr, item), wintext)) break;
	    }
	    if ((cursel == n) || ((INT_PTR)item == CB_ERR)) {
		TRACE("failed to find match??? item=%p cursel=%d\n",
		      item, cursel);
		if (infoPtr->hwndEdit) SetFocus(infoPtr->hwndEdit);
		return 0;
	    }
	}
	else {
            item = get_item_data(infoPtr, cursel);
	    if ((INT_PTR)item == CB_ERR) {
		TRACE("failed to find match??? item=%p cursel=%d\n",
		      item, cursel);
		if (infoPtr->hwndEdit) SetFocus(infoPtr->hwndEdit);
		return 0;
	    }
	}

	/* Save flags for testing and reset them */
	oldflags = infoPtr->flags;
	infoPtr->flags &= ~(WCBE_ACTEDIT | WCBE_EDITCHG);

	if (oldflags & WCBE_ACTEDIT) {
	    cbeend.fChanged = (oldflags & WCBE_EDITCHG);
	    cbeend.iNewSelection = SendMessageW (infoPtr->hwndCombo,
						 CB_GETCURSEL, 0, 0);
	    cbeend.iWhy = CBENF_DROPDOWN;

	    if (COMBOEX_NotifyEndEdit (infoPtr, &cbeend, COMBOEX_GetText(infoPtr, item))) return 0;
	}

	/* if selection has changed the set the new current selection */
	cursel = SendMessageW (infoPtr->hwndCombo, CB_GETCURSEL, 0, 0);
	if ((oldflags & WCBE_EDITCHG) || (cursel != infoPtr->selected)) {
	    infoPtr->selected = cursel;
	    SendMessageW (infoPtr->hwndSelf, CB_SETCURSEL, cursel, 0);
	    SetFocus(infoPtr->hwndCombo);
	}
	return 0;

    case CBN_SELCHANGE:
	/*
	 * CB_GETCURSEL(Combo)
	 * CB_GETITEMDATA(Combo)   < simulated by COMBOEX_FindItem
	 * lstrlenA
	 * WM_SETTEXT(Edit)
	 * WM_GETTEXTLENGTH(Edit)
	 * WM_GETTEXT(Edit)
	 * EM_SETSEL(Edit, 0,0)
	 * WM_GETTEXTLENGTH(Edit)
	 * WM_GETTEXT(Edit)
	 * EM_SETSEL(Edit, 0,len)
	 * return WM_COMMAND to parent
	 */
	oldItem = SendMessageW (infoPtr->hwndCombo, CB_GETCURSEL, 0, 0);
	if (!(item = COMBOEX_FindItem(infoPtr, oldItem))) {
	    ERR("item %Id not found. Problem!\n", oldItem);
	    break;
	}
	infoPtr->selected = oldItem;
	COMBOEX_SetEditText (infoPtr, item);
	return SendMessageW (parent, WM_COMMAND, wParam, (LPARAM)infoPtr->hwndSelf);

    case CBN_SELENDOK:
    case CBN_SELENDCANCEL:
	/*
	 * We have to change the handle since we are the control
	 * issuing the message. IE4 depends on this.
	 */
	return SendMessageW (parent, WM_COMMAND, wParam, (LPARAM)infoPtr->hwndSelf);

    case CBN_KILLFOCUS:
	SendMessageW (parent, WM_COMMAND, wParam, (LPARAM)infoPtr->hwndSelf);
	if (infoPtr->flags & WCBE_ACTEDIT) {
	    GetWindowTextW (infoPtr->hwndEdit, wintext, 260);
	    cbeend.fChanged = (infoPtr->flags & WCBE_EDITCHG);
	    cbeend.iNewSelection = SendMessageW (infoPtr->hwndCombo,
						 CB_GETCURSEL, 0, 0);
	    cbeend.iWhy = CBENF_KILLFOCUS;

	    infoPtr->flags &= ~(WCBE_ACTEDIT | WCBE_EDITCHG);
	    if (COMBOEX_NotifyEndEdit (infoPtr, &cbeend, wintext)) return 0;
	}
	/* possible CB_GETCURSEL */
	InvalidateRect (infoPtr->hwndCombo, 0, 0);
	return 0;

    case CBN_SETFOCUS:
        return SendMessageW (parent, WM_COMMAND, wParam, (LPARAM)infoPtr->hwndSelf);

    default:
	/*
	 * We have to change the handle since we are the control
	 * issuing the message. IE4 depends on this.
	 * We also need to set the focus back to the Edit control
	 * after passing the command to the parent of the ComboEx.
	 */
	lret = SendMessageW (parent, WM_COMMAND, wParam, (LPARAM)infoPtr->hwndSelf);
	if (infoPtr->hwndEdit) SetFocus(infoPtr->hwndEdit);
	return lret;
    }
    return 0;
}


static BOOL COMBOEX_WM_DeleteItem (COMBOEX_INFO *infoPtr, DELETEITEMSTRUCT const *dis)
{
    CBE_ITEMDATA *item, *olditem;
    NMCOMBOBOXEXW nmcit;
    UINT i;

    TRACE("CtlType=%08x, CtlID=%08x, itemID=%08x, hwnd=%p, data=%Ix\n",
	  dis->CtlType, dis->CtlID, dis->itemID, dis->hwndItem, dis->itemData);

    if (dis->itemID >= infoPtr->nb_items) return FALSE;

    olditem = infoPtr->items;
    i = infoPtr->nb_items - 1;

    if (i == dis->itemID) {
	infoPtr->items = infoPtr->items->next;
    }
    else {
	item = olditem;
	i--;

	/* find the prior item in the list */
	while (item->next && (i > dis->itemID)) {
	    item = item->next;
	    i--;
	}
	if (!item->next || (i != dis->itemID)) {
	    ERR("COMBOBOXEX item structures broken. Please report!\n");
	    return FALSE;
	}
	olditem = item->next;
	item->next = item->next->next;
    }
    infoPtr->nb_items--;

    memset (&nmcit.ceItem, 0, sizeof(nmcit.ceItem));
    nmcit.ceItem.mask=~0;
    COMBOEX_CopyItem (olditem, &nmcit.ceItem);
    COMBOEX_NotifyItem (infoPtr, CBEN_DELETEITEM, &nmcit);

    COMBOEX_FreeText(olditem);
    Free(olditem);

    return TRUE;
}


static LRESULT COMBOEX_DrawItem (COMBOEX_INFO *infoPtr, DRAWITEMSTRUCT const *dis)
{
    CBE_ITEMDATA *item = NULL;
    SIZE txtsize;
    RECT rect;
    LPCWSTR str = L"";
    UINT xbase, x, y;
    INT len;
    COLORREF nbkc, ntxc, bkc, txc;
    int drawimage, drawstate, xioff, selected;

    TRACE("DRAWITEMSTRUCT: CtlType=0x%08x CtlID=0x%08x\n",
	  dis->CtlType, dis->CtlID);
    TRACE("itemID=0x%08x itemAction=0x%08x itemState=0x%08x\n",
	  dis->itemID, dis->itemAction, dis->itemState);
    TRACE("hWnd=%p hDC=%p (%s) itemData=%#Ix\n",
          dis->hwndItem, dis->hDC, wine_dbgstr_rect(&dis->rcItem), dis->itemData);

    /* MSDN says:                                                       */
    /*     "itemID - Specifies the menu item identifier for a menu      */
    /*      item or the index of the item in a list box or combo box.   */
    /*      For an empty list box or combo box, this member can be -1.  */
    /*      This allows the application to draw only the focus          */
    /*      rectangle at the coordinates specified by the rcItem        */
    /*      member even though there are no items in the control.       */
    /*      This indicates to the user whether the list box or combo    */
    /*      box has the focus. How the bits are set in the itemAction   */
    /*      member determines whether the rectangle is to be drawn as   */
    /*      though the list box or combo box has the focus.             */
    if (dis->itemID == 0xffffffff) {
	if ( ( (dis->itemAction & ODA_FOCUS) && (dis->itemState & ODS_SELECTED)) ||
	     ( (dis->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)) && (dis->itemState & ODS_FOCUS) ) ) {

            TRACE("drawing item -1 special focus, rect=(%s)\n",
                  wine_dbgstr_rect(&dis->rcItem));
	}
	else if ((dis->CtlType == ODT_COMBOBOX) &&
		 (dis->itemAction == ODA_DRAWENTIRE)) {
	    /* draw of edit control data */

	    if (TRACE_ON(comboex)) {
		RECT exrc, cbrc, edrc;
		GetWindowRect (infoPtr->hwndSelf, &exrc);
		GetWindowRect (infoPtr->hwndCombo, &cbrc);
                SetRect(&edrc, -1, -1, -1, -1);
		if (infoPtr->hwndEdit) GetWindowRect (infoPtr->hwndEdit, &edrc);
                TRACE("window rects ex=(%s), cb=(%s), ed=(%s)\n",
                      wine_dbgstr_rect(&exrc), wine_dbgstr_rect(&cbrc),
                      wine_dbgstr_rect(&edrc));
	    }
	}
	else {
            ERR("NOT drawing item  -1 special focus, rect=(%s), action=%08x, state=%08x\n",
                wine_dbgstr_rect(&dis->rcItem),
                dis->itemAction, dis->itemState);
	    return 0;
	}
    }

    /* If draw item is -1 (edit control) setup the item pointer */
    if (dis->itemID == 0xffffffff) {
        item = &infoPtr->edit;

	if (infoPtr->hwndEdit) {
	    /* free previous text of edit item */
	    COMBOEX_FreeText(item);
	    item->mask &= ~CBEIF_TEXT;
	    if( (len = GetWindowTextLengthW(infoPtr->hwndEdit)) ) {
		item->mask |= CBEIF_TEXT;
                item->pszText = Alloc ((len + 1)*sizeof(WCHAR));
		if (item->pszText)
		    GetWindowTextW(infoPtr->hwndEdit, item->pszText, len+1);

	       TRACE("edit control hwndEdit=%p, text len=%d str=%s\n",
		     infoPtr->hwndEdit, len, debugstr_txt(item->pszText));
	    }
	}
    }


    /* if the item pointer is not set, then get the data and locate it */
    if (!item) {
        item = get_item_data(infoPtr, dis->itemID);
	if (item == (CBE_ITEMDATA *)CB_ERR) {
	    ERR("invalid item for id %d\n", dis->itemID);
	    return 0;
	}
    }

    if (TRACE_ON(comboex)) COMBOEX_DumpItem (item);

    xbase = CBE_STARTOFFSET;
    if ((item->mask & CBEIF_INDENT) && (dis->itemState & ODS_COMBOEXLBOX)) {
	INT indent = item->iIndent;
	if (indent == I_INDENTCALLBACK) {
	    NMCOMBOBOXEXW nmce;
	    ZeroMemory(&nmce, sizeof(nmce));
	    nmce.ceItem.mask = CBEIF_INDENT;
	    nmce.ceItem.lParam = item->lParam;
	    nmce.ceItem.iItem = dis->itemID;
	    COMBOEX_NotifyItem(infoPtr, CBEN_GETDISPINFOW, &nmce);
	    if (nmce.ceItem.mask & CBEIF_DI_SETITEM)
		item->iIndent = nmce.ceItem.iIndent;
	    indent = nmce.ceItem.iIndent;
	}
        xbase += (indent * CBE_INDENT);
    }

    drawimage = -2;
    drawstate = ILD_NORMAL;
    selected = infoPtr->selected == dis->itemID;

    if (item->mask & CBEIF_IMAGE)
	drawimage = item->iImage;
    if (item->mask & CBEIF_SELECTEDIMAGE && selected)
        drawimage = item->iSelectedImage;
    if (dis->itemState & ODS_COMBOEXLBOX) {
	/* drawing listbox entry */
	if (dis->itemState & ODS_SELECTED)
	    drawstate = ILD_SELECTED;
    } else {
	/* drawing combo/edit entry */
	if (IsWindowVisible(infoPtr->hwndEdit)) {
	    /* if we have an edit control, set the selection state from the edit focus state */
	    if (infoPtr->flags & WCBE_EDITFOCUSED)
		drawstate = ILD_SELECTED;
	} else
	    /* if we don't have an edit control, use
	     * the requested state.
	     */
	    if (dis->itemState & ODS_SELECTED)
		drawstate = ILD_SELECTED;
    }

    if (infoPtr->himl && !(infoPtr->dwExtStyle & CBES_EX_NOEDITIMAGEINDENT)) {
    	IMAGEINFO iinfo;
        iinfo.rcImage.left = iinfo.rcImage.right = 0;
	ImageList_GetImageInfo(infoPtr->himl, 0, &iinfo);
	xioff = iinfo.rcImage.right - iinfo.rcImage.left + CBE_SEP;
    }  else xioff = 0;

    /* setup pointer to text to be drawn */
    str = COMBOEX_GetText(infoPtr, item);
    if (!str) str = L"";

    len = lstrlenW (str);
    GetTextExtentPoint32W (dis->hDC, str, len, &txtsize);

    if (dis->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)) {
	int overlay = item->iOverlay;

    	if (drawimage == I_IMAGECALLBACK) {
	    NMCOMBOBOXEXW nmce;
	    ZeroMemory(&nmce, sizeof(nmce));
	    nmce.ceItem.mask = selected ? CBEIF_SELECTEDIMAGE : CBEIF_IMAGE;
	    nmce.ceItem.lParam = item->lParam;
	    nmce.ceItem.iItem = dis->itemID;
	    COMBOEX_NotifyItem(infoPtr, CBEN_GETDISPINFOW, &nmce);
	    if (!selected) {
	    	if (nmce.ceItem.mask & CBEIF_DI_SETITEM) item->iImage = nmce.ceItem.iImage;
	    	drawimage = nmce.ceItem.iImage;
	    } else {
	        if (nmce.ceItem.mask & CBEIF_DI_SETITEM) item->iSelectedImage = nmce.ceItem.iSelectedImage;
                drawimage = nmce.ceItem.iSelectedImage;
	    }
        }

	if (overlay == I_IMAGECALLBACK) {
	    NMCOMBOBOXEXW nmce;
	    ZeroMemory(&nmce, sizeof(nmce));
	    nmce.ceItem.mask = CBEIF_OVERLAY;
	    nmce.ceItem.lParam = item->lParam;
	    nmce.ceItem.iItem = dis->itemID;
	    COMBOEX_NotifyItem(infoPtr, CBEN_GETDISPINFOW, &nmce);
	    if (nmce.ceItem.mask & CBEIF_DI_SETITEM)
		item->iOverlay = nmce.ceItem.iOverlay;
	    overlay = nmce.ceItem.iOverlay;
	}

	if (drawimage >= 0 &&
	    !(infoPtr->dwExtStyle & (CBES_EX_NOEDITIMAGE | CBES_EX_NOEDITIMAGEINDENT))) {
	    if (overlay > 0) ImageList_SetOverlayImage (infoPtr->himl, overlay, 1);
	    ImageList_Draw (infoPtr->himl, drawimage, dis->hDC, xbase, dis->rcItem.top,
			    drawstate | (overlay > 0 ? INDEXTOOVERLAYMASK(1) : 0));
	}

	/* now draw the text */
	if (!IsWindowVisible (infoPtr->hwndEdit)) {
	    nbkc = (dis->itemState & ODS_SELECTED) ?
	            comctl32_color.clrHighlight : comctl32_color.clrWindow;
	    bkc = SetBkColor (dis->hDC, nbkc);
	    ntxc = (dis->itemState & ODS_SELECTED) ?
	            comctl32_color.clrHighlightText : comctl32_color.clrWindowText;
	    txc = SetTextColor (dis->hDC, ntxc);
	    x = xbase + xioff;
	    y = dis->rcItem.top +
	        (dis->rcItem.bottom - dis->rcItem.top - txtsize.cy) / 2;
            SetRect(&rect, x, dis->rcItem.top + 1, x + txtsize.cx, dis->rcItem.bottom - 1);
            TRACE("drawing item %d text, rect=(%s)\n",
                  dis->itemID, wine_dbgstr_rect(&rect));
	    ExtTextOutW (dis->hDC, x, y, ETO_OPAQUE | ETO_CLIPPED,
		         &rect, str, len, 0);
	    SetBkColor (dis->hDC, bkc);
	    SetTextColor (dis->hDC, txc);
	}
    }

    if (dis->itemAction & ODA_FOCUS) {
	rect.left = xbase + xioff - 1;
	rect.right = rect.left + txtsize.cx + 2;
	rect.top = dis->rcItem.top;
	rect.bottom = dis->rcItem.bottom;
	DrawFocusRect(dis->hDC, &rect);
    }

    return 0;
}


static void COMBOEX_ResetContent (COMBOEX_INFO *infoPtr)
{
    if (infoPtr->items)
    {
        CBE_ITEMDATA *item, *next;

        item = infoPtr->items;
        while (item) {
            next = item->next;
            COMBOEX_FreeText (item);
            Free (item);
            item = next;
        }
        infoPtr->items = 0;
    }

    infoPtr->selected = -1;
    infoPtr->nb_items = 0;
}


static LRESULT COMBOEX_Destroy (COMBOEX_INFO *infoPtr)
{
    if (infoPtr->hwndCombo)
        SetWindowSubclass(infoPtr->hwndCombo, COMBOEX_ComboWndProc, COMBO_SUBCLASSID, 0);

    if (infoPtr->hwndEdit)
        SetWindowSubclass(infoPtr->hwndEdit, COMBOEX_EditWndProc, EDIT_SUBCLASSID, 0);

    COMBOEX_FreeText (&infoPtr->edit);
    COMBOEX_ResetContent (infoPtr);

    if (infoPtr->defaultFont)
	DeleteObject (infoPtr->defaultFont);

    SetWindowLongPtrW (infoPtr->hwndSelf, 0, 0);

    /* free comboex info data */
    Free (infoPtr);

    return 0;
}


static LRESULT COMBOEX_Enable (COMBOEX_INFO *infoPtr, BOOL enable)
{
    TRACE("hwnd=%p, enable=%s\n", infoPtr->hwndSelf, enable ? "TRUE":"FALSE");

    if (infoPtr->hwndEdit)
       EnableWindow(infoPtr->hwndEdit, enable);

    EnableWindow(infoPtr->hwndCombo, enable);

    /* Force the control to repaint when the enabled state changes. */
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return 1;
}


static LRESULT COMBOEX_MeasureItem (COMBOEX_INFO const *infoPtr, MEASUREITEMSTRUCT *mis)
{
    SIZE mysize;
    HDC hdc;

    hdc = GetDC (0);
    GetTextExtentPointW (hdc, L"W", 1, &mysize);
    ReleaseDC (0, hdc);
    mis->itemHeight = mysize.cy + CBE_EXTRA;

    TRACE("adjusted height hwnd=%p, height=%d\n",
	  infoPtr->hwndSelf, mis->itemHeight);

    return 0;
}


static LRESULT COMBOEX_NCCreate (HWND hwnd)
{
    /* WARNING: The COMBOEX_INFO structure is not yet created */
    DWORD oldstyle, newstyle;

    oldstyle = (DWORD)GetWindowLongW (hwnd, GWL_STYLE);
    newstyle = oldstyle & ~(WS_VSCROLL | WS_HSCROLL | WS_BORDER);
    if (newstyle != oldstyle)
    {
        TRACE("req style %#lx, resetting style %#lx\n", oldstyle, newstyle);
        SetWindowLongW (hwnd, GWL_STYLE, newstyle);
    }
    return 1;
}


static LRESULT COMBOEX_NotifyFormat (COMBOEX_INFO *infoPtr, LPARAM lParam)
{
    if (lParam == NF_REQUERY) {
	INT i = SendMessageW(infoPtr->hwndNotify,
			 WM_NOTIFYFORMAT, (WPARAM)infoPtr->hwndSelf, NF_QUERY);
        infoPtr->NtfUnicode = (i == NFR_UNICODE);
    }
    return infoPtr->NtfUnicode ? NFR_UNICODE : NFR_ANSI;
}


static LRESULT COMBOEX_Size (COMBOEX_INFO *infoPtr, INT width, INT height)
{
    TRACE("(width=%d, height=%d)\n", width, height);

    MoveWindow (infoPtr->hwndCombo, 0, 0, width, height, TRUE);

    COMBOEX_AdjustEditPos (infoPtr);

    return 0;
}

static LRESULT COMBOEX_SetFont( COMBOEX_INFO *infoPtr, HFONT font, BOOL redraw )
{
    infoPtr->font = font;
    SendMessageW( infoPtr->hwndCombo, WM_SETFONT, (WPARAM)font, 0 );
    if (infoPtr->hwndEdit) SendMessageW( infoPtr->hwndEdit, WM_SETFONT, (WPARAM)font, 0 );
    COMBOEX_ReSize( infoPtr );
    if (redraw) InvalidateRect( infoPtr->hwndCombo, NULL, TRUE );
    return 0;
}

static LRESULT COMBOEX_SetRedraw(const COMBOEX_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret = DefWindowProcW( infoPtr->hwndSelf, WM_SETREDRAW, wParam, lParam );
    if (wParam) RedrawWindow( infoPtr->hwndSelf, NULL, 0, RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN );
    return ret;
}


static LRESULT COMBOEX_WindowPosChanging (const COMBOEX_INFO *infoPtr, WINDOWPOS *wp)
{
    RECT cbx_wrect, cbx_crect, cb_wrect;
    INT width, height;

    GetWindowRect (infoPtr->hwndSelf, &cbx_wrect);
    GetClientRect (infoPtr->hwndSelf, &cbx_crect);
    GetWindowRect (infoPtr->hwndCombo, &cb_wrect);

    /* width is winpos value + border width of comboex */
    width = wp->cx
	    + (cbx_wrect.right-cbx_wrect.left)
            - (cbx_crect.right-cbx_crect.left);

    TRACE("winpos=(%d,%d %dx%d) flags=0x%08x\n",
	  wp->x, wp->y, wp->cx, wp->cy, wp->flags);
    TRACE("EX window=(%s), client=(%s)\n",
          wine_dbgstr_rect(&cbx_wrect), wine_dbgstr_rect(&cbx_crect));
    TRACE("CB window=(%s), EX setting=(0,0)-(%d,%ld)\n",
          wine_dbgstr_rect(&cbx_wrect), width, cb_wrect.bottom-cb_wrect.top);

    if (width) SetWindowPos (infoPtr->hwndCombo, HWND_TOP, 0, 0,
			     width,
			     cb_wrect.bottom-cb_wrect.top,
			     SWP_NOACTIVATE);

    GetWindowRect (infoPtr->hwndCombo, &cb_wrect);

    /* height is combo window height plus border width of comboex */
    height =   (cb_wrect.bottom-cb_wrect.top)
	     + (cbx_wrect.bottom-cbx_wrect.top)
             - (cbx_crect.bottom-cbx_crect.top);
    wp->cy = height;
    if (infoPtr->hwndEdit) {
	COMBOEX_AdjustEditPos (infoPtr);
	InvalidateRect (infoPtr->hwndCombo, 0, TRUE);
    }

    return 0;
}

static LRESULT CALLBACK
COMBOEX_EditWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                     UINT_PTR uId, DWORD_PTR ref_data)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr ((HWND)ref_data);
    NMCBEENDEDITW cbeend;
    WCHAR edit_text[260];
    COLORREF obkc;
    HDC hDC;
    RECT rect;
    LRESULT lret;

    TRACE("hwnd %p, msg %x, wparam %Ix, lParam %Ix, info_ptr=%p\n",
            hwnd, uMsg, wParam, lParam, infoPtr);

    if (uMsg == WM_NCDESTROY)
        RemoveWindowSubclass(hwnd, COMBOEX_EditWndProc, EDIT_SUBCLASSID);

    if (!infoPtr)
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {

	case WM_CHAR:
	    /* handle (ignore) the return character */
	    if (wParam == VK_RETURN) return 0;
	    /* all other characters pass into the real Edit */
	    return DefSubclassProc(hwnd, uMsg, wParam, lParam);

	case WM_ERASEBKGND:
            hDC = (HDC) wParam;
	    obkc = SetBkColor (hDC, comctl32_color.clrWindow);
            GetClientRect (hwnd, &rect);
            TRACE("erasing (%s)\n", wine_dbgstr_rect(&rect));
	    ExtTextOutW (hDC, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);
            SetBkColor (hDC, obkc);
	    return DefSubclassProc(hwnd, uMsg, wParam, lParam);

	case WM_KEYDOWN: {
	    INT_PTR oldItem, selected;
	    CBE_ITEMDATA *item;

	    switch ((INT)wParam)
	    {
	    case VK_ESCAPE:
		TRACE("special code for VK_ESCAPE\n");

		GetWindowTextW (infoPtr->hwndEdit, edit_text, 260);

		infoPtr->flags &= ~(WCBE_ACTEDIT | WCBE_EDITCHG);
		cbeend.fChanged = FALSE;
		cbeend.iNewSelection = SendMessageW (infoPtr->hwndCombo,
						     CB_GETCURSEL, 0, 0);
		cbeend.iWhy = CBENF_ESCAPE;

		if (COMBOEX_NotifyEndEdit (infoPtr, &cbeend, edit_text)) return 0;
		oldItem = SendMessageW (infoPtr->hwndCombo, CB_GETCURSEL, 0, 0);
		InvalidateRect (infoPtr->hwndCombo, 0, 0);
		if (!(item = COMBOEX_FindItem(infoPtr, oldItem))) {
		    ERR("item %Id not found. Problem!\n", oldItem);
		    break;
		}
		infoPtr->selected = oldItem;
		COMBOEX_SetEditText (infoPtr, item);
		RedrawWindow (infoPtr->hwndCombo, 0, 0, RDW_ERASE |
			      RDW_INVALIDATE);
		break;

	    case VK_RETURN:
		TRACE("special code for VK_RETURN\n");

		GetWindowTextW (infoPtr->hwndEdit, edit_text, 260);

		infoPtr->flags &= ~(WCBE_ACTEDIT | WCBE_EDITCHG);
		selected = SendMessageW (infoPtr->hwndCombo,
					 CB_GETCURSEL, 0, 0);

		if (selected != -1) {
                    cmp_func_t cmptext = get_cmp_func(infoPtr);
		    item = COMBOEX_FindItem (infoPtr, selected);
		    TRACE("handling VK_RETURN, selected = %Id, selected_text=%s\n", selected, debugstr_txt(item->pszText));
		    TRACE("handling VK_RETURN, edittext=%s\n", debugstr_w(edit_text));
		    if (cmptext (COMBOEX_GetText(infoPtr, item), edit_text)) {
			/* strings not equal -- indicate edit has changed */
			selected = -1;
		    }
		}

		cbeend.iNewSelection = selected;
		cbeend.fChanged = TRUE;
		cbeend.iWhy = CBENF_RETURN;
		if (COMBOEX_NotifyEndEdit (infoPtr, &cbeend, edit_text)) {
		    /* abort the change, restore previous */
		    TRACE("Notify requested abort of change\n");
                    COMBOEX_SetEditText (infoPtr, &infoPtr->edit);
		    RedrawWindow (infoPtr->hwndCombo, 0, 0, RDW_ERASE |
				  RDW_INVALIDATE);
		    return 0;
		}
		oldItem = SendMessageW (infoPtr->hwndCombo,CB_GETCURSEL, 0, 0);
		if (oldItem != -1) {
		    /* if something is selected, then deselect it */
                    SendMessageW (infoPtr->hwndCombo, CB_SETCURSEL, -1, 0);
		}
		InvalidateRect (infoPtr->hwndCombo, 0, 0);
		SetFocus(infoPtr->hwndEdit);
		break;

	    case VK_UP:
	    case VK_DOWN:
	    {
		INT step = wParam == VK_DOWN ? 1 : -1;

		oldItem = SendMessageW (infoPtr->hwndSelf, CB_GETCURSEL, 0, 0);
		if (oldItem >= 0 && oldItem + step >= 0)
		    SendMessageW (infoPtr->hwndSelf, CB_SETCURSEL, oldItem + step, 0);
	    	return 0;
	    }
	    default:
		return DefSubclassProc(hwnd, uMsg, wParam, lParam);
	    }
	    return 0;
            }

	case WM_SETFOCUS:
	    /* remember the focus to set state of icon */
	    lret = DefSubclassProc(hwnd, uMsg, wParam, lParam);
	    infoPtr->flags |= WCBE_EDITFOCUSED;
	    return lret;

	case WM_KILLFOCUS:
	    /*
	     * do NOTIFY CBEN_ENDEDIT with CBENF_KILLFOCUS
	     */
	    infoPtr->flags &= ~WCBE_EDITFOCUSED;
	    if (infoPtr->flags & WCBE_ACTEDIT) {
		infoPtr->flags &= ~(WCBE_ACTEDIT | WCBE_EDITCHG);

		GetWindowTextW (infoPtr->hwndEdit, edit_text, 260);
		cbeend.fChanged = FALSE;
		cbeend.iNewSelection = SendMessageW (infoPtr->hwndCombo,
						     CB_GETCURSEL, 0, 0);
		cbeend.iWhy = CBENF_KILLFOCUS;

		COMBOEX_NotifyEndEdit (infoPtr, &cbeend, edit_text);
	    }
	    /* fall through */

	default:
	    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
    }
}


static LRESULT CALLBACK
COMBOEX_ComboWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                      UINT_PTR uId, DWORD_PTR ref_data)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr ((HWND)ref_data);
    NMCBEENDEDITW cbeend;
    NMMOUSE nmmse;
    COLORREF obkc;
    HDC hDC;
    HWND focusedhwnd;
    RECT rect;
    POINT pt;
    WCHAR edit_text[260];

    TRACE("hwnd %p, msg %x, wparam %Ix, lParam %Ix, info_ptr %p\n",
            hwnd, uMsg, wParam, lParam, infoPtr);

    if (uMsg == WM_NCDESTROY)
        RemoveWindowSubclass(hwnd, COMBOEX_ComboWndProc, COMBO_SUBCLASSID);

    if (!infoPtr)
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_DRAWITEM:
	    /*
	     * The only way this message should come is from the
	     * child Listbox issuing the message. Flag this so
	     * that ComboEx knows this is listbox.
	     */
	    ((DRAWITEMSTRUCT *)lParam)->itemState |= ODS_COMBOEXLBOX;
	    break;

    case WM_ERASEBKGND:
            hDC = (HDC) wParam;
	    obkc = SetBkColor (hDC, comctl32_color.clrWindow);
            GetClientRect (hwnd, &rect);
            TRACE("erasing (%s)\n", wine_dbgstr_rect(&rect));
	    ExtTextOutW (hDC, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);
            SetBkColor (hDC, obkc);
	    break;

    case WM_SETCURSOR:
	    /*
	     *  WM_NOTIFY to comboex parent (rebar)
	     *   with NM_SETCURSOR with extra words of 0,0,0,0,0x02010001
	     *  CallWindowProc (previous)
	     */
	    nmmse.dwItemSpec = 0;
	    nmmse.dwItemData = 0;
	    nmmse.pt.x = 0;
	    nmmse.pt.y = 0;
	    nmmse.dwHitInfo = lParam;
	    COMBOEX_Notify (infoPtr, NM_SETCURSOR, (NMHDR *)&nmmse);
	    break;

    case WM_LBUTTONDOWN:
	    GetClientRect (hwnd, &rect);
	    rect.bottom = rect.top + SendMessageW(infoPtr->hwndSelf,
			                          CB_GETITEMHEIGHT, -1, 0);
	    rect.left = rect.right - GetSystemMetrics(SM_CXVSCROLL);
	    pt.x = (short)LOWORD(lParam);
	    pt.y = (short)HIWORD(lParam);
	    if (PtInRect(&rect, pt))
		break;

	    infoPtr->flags |= WCBE_MOUSECAPTURED;
	    SetCapture(hwnd);
	    return 0;

    case WM_LBUTTONUP:
	    if (!(infoPtr->flags & WCBE_MOUSECAPTURED))
		break;

	    ReleaseCapture();
	    infoPtr->flags &= ~WCBE_MOUSECAPTURED;
	    if (infoPtr->flags & WCBE_MOUSEDRAGGED) {
		infoPtr->flags &= ~WCBE_MOUSEDRAGGED;
	    } else {
		SendMessageW(hwnd, CB_SHOWDROPDOWN, TRUE, 0);
	    }
	    return 0;

    case WM_MOUSEMOVE:
	    if ( (infoPtr->flags & WCBE_MOUSECAPTURED) &&
		!(infoPtr->flags & WCBE_MOUSEDRAGGED)) {
		GetWindowTextW (infoPtr->hwndEdit, edit_text, 260);
		COMBOEX_NotifyDragBegin(infoPtr, edit_text);
		infoPtr->flags |= WCBE_MOUSEDRAGGED;
	    }
	    break;

    case WM_COMMAND:
	    switch (HIWORD(wParam)) {

	    case EN_UPDATE:
		/* traces show that COMBOEX does not issue CBN_EDITUPDATE
		 * on the EN_UPDATE
		 */
		return 0;

	    case EN_KILLFOCUS:
		focusedhwnd = GetFocus();
		if (infoPtr->flags & WCBE_ACTEDIT) {
		    GetWindowTextW (infoPtr->hwndEdit, edit_text, 260);
		    cbeend.fChanged = (infoPtr->flags & WCBE_EDITCHG);
		    cbeend.iNewSelection = SendMessageW (infoPtr->hwndCombo,
							 CB_GETCURSEL, 0, 0);
		    cbeend.iWhy = CBENF_KILLFOCUS;

		    infoPtr->flags &= ~(WCBE_ACTEDIT | WCBE_EDITCHG);
		    if (COMBOEX_NotifyEndEdit (infoPtr, &cbeend, edit_text)) return 0;
		}
		/* possible CB_GETCURSEL */
		InvalidateRect (infoPtr->hwndCombo, 0, 0);
		if (focusedhwnd)
		    SendMessageW (infoPtr->hwndCombo, WM_KILLFOCUS,
				  (WPARAM)focusedhwnd, 0);
		return 0;

	    case EN_SETFOCUS: {
		NMHDR hdr;

		SendMessageW (infoPtr->hwndEdit, EM_SETSEL, 0, 0);
		SendMessageW (infoPtr->hwndEdit, EM_SETSEL, 0, -1);
		COMBOEX_Notify (infoPtr, CBEN_BEGINEDIT, &hdr);
		infoPtr->flags |= WCBE_ACTEDIT;
		infoPtr->flags &= ~WCBE_EDITCHG; /* no change yet */
		return 0;
	        }

	    case EN_CHANGE: {
		LPCWSTR lastwrk;
                cmp_func_t cmptext = get_cmp_func(infoPtr);

		INT_PTR selected = SendMessageW (infoPtr->hwndCombo,
                                                 CB_GETCURSEL, 0, 0);

		/* lstrlenW( lastworkingURL ) */

		GetWindowTextW (infoPtr->hwndEdit, edit_text, 260);
		if (selected == -1) {
                    lastwrk = infoPtr->edit.pszText;
		}
		else {
		    CBE_ITEMDATA *item = COMBOEX_FindItem (infoPtr, selected);
		    lastwrk = COMBOEX_GetText(infoPtr, item);
		}

		TRACE("handling EN_CHANGE, selected = %Id, selected_text=%s\n", selected, debugstr_w(lastwrk));
		TRACE("handling EN_CHANGE, edittext=%s\n",
		      debugstr_w(edit_text));

		/* cmptext is between lastworkingURL and GetWindowText */
		if (cmptext (lastwrk, edit_text)) {
		    /* strings not equal -- indicate edit has changed */
		    infoPtr->flags |= WCBE_EDITCHG;
		}
		SendMessageW ( infoPtr->hwndNotify, WM_COMMAND,
			       MAKEWPARAM(GetDlgCtrlID (infoPtr->hwndSelf),
					  CBN_EDITCHANGE),
			       (LPARAM)infoPtr->hwndSelf);
		return 0;
	        }

	    case LBN_SELCHANGE:
	    default:
		break;
	    }/* fall through */
    default:
        ;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}


static LRESULT WINAPI
COMBOEX_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    TRACE("hwnd %p, msg %x, wparam %Ix, lParam %Ix\n", hwnd, uMsg, wParam, lParam);

    if (!infoPtr) {
	if (uMsg == WM_CREATE)
	    return COMBOEX_Create (hwnd, (LPCREATESTRUCTA)lParam);
	if (uMsg == WM_NCCREATE)
	    COMBOEX_NCCreate (hwnd);
        return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }

    switch (uMsg)
    {
        case CBEM_DELETEITEM:
	    return COMBOEX_DeleteItem (infoPtr, wParam);

	case CBEM_GETCOMBOCONTROL:
	    return (LRESULT)infoPtr->hwndCombo;

	case CBEM_GETEDITCONTROL:
	    return (LRESULT)infoPtr->hwndEdit;

	case CBEM_GETEXTENDEDSTYLE:
	    return infoPtr->dwExtStyle;

	case CBEM_GETIMAGELIST:
	    return (LRESULT)infoPtr->himl;

	case CBEM_GETITEMA:
	    return (LRESULT)COMBOEX_GetItemA (infoPtr, (COMBOBOXEXITEMA *)lParam);

	case CBEM_GETITEMW:
	    return (LRESULT)COMBOEX_GetItemW (infoPtr, (COMBOBOXEXITEMW *)lParam);

	case CBEM_GETUNICODEFORMAT:
	    return infoPtr->unicode;

	case CBEM_HASEDITCHANGED:
	    return COMBOEX_HasEditChanged (infoPtr);

	case CBEM_INSERTITEMA:
	    return COMBOEX_InsertItemA (infoPtr, (COMBOBOXEXITEMA *)lParam);

	case CBEM_INSERTITEMW:
	    return COMBOEX_InsertItemW (infoPtr, (COMBOBOXEXITEMW *)lParam);

	case CBEM_SETEXSTYLE:
	case CBEM_SETEXTENDEDSTYLE:
	    return COMBOEX_SetExtendedStyle (infoPtr, (DWORD)wParam, (DWORD)lParam);

	case CBEM_SETIMAGELIST:
	    return (LRESULT)COMBOEX_SetImageList (infoPtr, (HIMAGELIST)lParam);

	case CBEM_SETITEMA:
	    return COMBOEX_SetItemA (infoPtr, (COMBOBOXEXITEMA *)lParam);

	case CBEM_SETITEMW:
	    return COMBOEX_SetItemW (infoPtr, (COMBOBOXEXITEMW *)lParam);

	case CBEM_SETUNICODEFORMAT:
	    return COMBOEX_SetUnicodeFormat (infoPtr, wParam);

	/*case CBEM_SETWINDOWTHEME:
	    FIXME("CBEM_SETWINDOWTHEME: stub\n");*/

	case WM_SETTEXT:
	case WM_GETTEXT:
	case WM_GETTEXTLENGTH:
            return SendMessageW(infoPtr->hwndEdit, uMsg, wParam, lParam);

	case CB_GETLBTEXT:
            return COMBOEX_GetListboxText(infoPtr, wParam, (LPWSTR)lParam);

	case CB_GETLBTEXTLEN:
            return COMBOEX_GetListboxText(infoPtr, wParam, NULL);

	case CB_RESETCONTENT:
            COMBOEX_ResetContent(infoPtr);
            /* fall through */

/*   Combo messages we are not sure if we need to process or just forward */
	case CB_GETDROPPEDCONTROLRECT:
	case CB_GETITEMHEIGHT:
	case CB_GETEXTENDEDUI:
	case CB_LIMITTEXT:
	case CB_SELECTSTRING:

/*   Combo messages OK to just forward to the regular COMBO */
	case CB_GETCOUNT:
	case CB_GETCURSEL:
	case CB_GETDROPPEDSTATE:
        case CB_SETDROPPEDWIDTH:
        case CB_SETEXTENDEDUI:
        case CB_SHOWDROPDOWN:
	    return SendMessageW (infoPtr->hwndCombo, uMsg, wParam, lParam);

/*   Combo messages we need to process specially */
        case CB_FINDSTRINGEXACT:
	    return COMBOEX_FindStringExact (infoPtr, (INT)wParam, (LPCWSTR)lParam);

	case CB_GETITEMDATA:
	    return COMBOEX_GetItemData (infoPtr, (INT)wParam);

	case CB_SETCURSEL:
	    return COMBOEX_SetCursel (infoPtr, (INT)wParam);

	case CB_SETITEMDATA:
	    return COMBOEX_SetItemData (infoPtr, (INT)wParam, (DWORD_PTR)lParam);

	case CB_SETITEMHEIGHT:
	    return COMBOEX_SetItemHeight (infoPtr, (INT)wParam, (UINT)lParam);



/*   Window messages passed to parent */
	case WM_COMMAND:
	    return COMBOEX_Command (infoPtr, wParam);

	case WM_NOTIFY:
	    if (infoPtr->NtfUnicode)
		return SendMessageW (infoPtr->hwndNotify, uMsg, wParam, lParam);
	    else
		return SendMessageA (infoPtr->hwndNotify, uMsg, wParam, lParam);


/*   Window messages we need to process */
        case WM_DELETEITEM:
	    return COMBOEX_WM_DeleteItem (infoPtr, (DELETEITEMSTRUCT *)lParam);

        case WM_DRAWITEM:
            return COMBOEX_DrawItem (infoPtr, (DRAWITEMSTRUCT *)lParam);

	case WM_DESTROY:
	    return COMBOEX_Destroy (infoPtr);

	case WM_ENABLE:
	    return COMBOEX_Enable (infoPtr, (BOOL)wParam);

        case WM_MEASUREITEM:
            return COMBOEX_MeasureItem (infoPtr, (MEASUREITEMSTRUCT *)lParam);

        case WM_NOTIFYFORMAT:
	    return COMBOEX_NotifyFormat (infoPtr, lParam);

	case WM_SIZE:
	    return COMBOEX_Size (infoPtr, LOWORD(lParam), HIWORD(lParam));

        case WM_GETFONT:
	    return (LRESULT)infoPtr->font;

	case WM_SETFONT:
	    return COMBOEX_SetFont( infoPtr, (HFONT)wParam, LOWORD(lParam) != 0 );

        case WM_SETREDRAW:
            return COMBOEX_SetRedraw(infoPtr, wParam, lParam);

        case WM_WINDOWPOSCHANGING:
	    return COMBOEX_WindowPosChanging (infoPtr, (WINDOWPOS *)lParam);

        case WM_SETFOCUS:
            if (infoPtr->hwndEdit) SetFocus( infoPtr->hwndEdit );
            else SetFocus( infoPtr->hwndCombo );
            return 0;

        case WM_SYSCOLORCHANGE:
            COMCTL32_RefreshSysColors();
            return 0;

        default:
            if ((uMsg >= WM_USER) && (uMsg < WM_APP) && !COMCTL32_IsReflectedMessage(uMsg))
                ERR("unknown msg %04x, wp %#Ix, lp %#Ix\n",uMsg,wParam,lParam);
            return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }
}


void COMBOEX_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = COMBOEX_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(COMBOEX_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = WC_COMBOBOXEXW;

    RegisterClassW (&wndClass);
}


void COMBOEX_Unregister (void)
{
    UnregisterClassW (WC_COMBOBOXEXW, NULL);
}
