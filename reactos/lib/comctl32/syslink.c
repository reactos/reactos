/*
 * SysLink control
 *
 * Copyright 2004 Thomas Weidenmueller <w3seek@reactos.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * TODO:
 * - Fix SHIFT+TAB and TAB issue (wrong link is selected when control gets the focus)
 * - Better string parsing
 * - Improve word wrapping
 * - Control styles?!
 *
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(progress);

INT WINAPI StrCmpNIW(LPCWSTR,LPCWSTR,INT);

#define SYSLINK_Alloc(size)        HeapAlloc(GetProcessHeap(), 0, (size))
#define SYSLINK_Free(ptr)          HeapFree(GetProcessHeap(), 0, (ptr))
#define SYSLINK_ReAlloc(ptr, size) HeapReAlloc(GetProcessHeap(), 0, ptr, (size))

typedef struct
{
    int nChars;
    RECT rc;
} DOC_TEXTBLOCK, *PDOC_TEXTBLOCK;

#define LIF_FLAGSMASK   (LIF_STATE | LIF_ITEMID | LIF_URL)
#define LIS_MASK        (LIS_FOCUSED | LIS_ENABLED | LIS_VISITED)

typedef enum
{
    slText = 0,
    slLink
} SL_ITEM_TYPE;

typedef struct _DOC_ITEM
{
    struct _DOC_ITEM *Next; /* Address to the next item */
    LPWSTR Text;            /* Text of the document item */
    UINT nText;             /* Number of characters of the text */
    SL_ITEM_TYPE Type;      /* type of the item */
    PDOC_TEXTBLOCK Blocks;  /* Array of text blocks */
    union
    {
        struct
        {
            UINT state;     /* Link state */
            WCHAR *szID;    /* Link ID string */
            WCHAR *szUrl;   /* Link URL string */
            HRGN hRgn;      /* Region of the link */
        } Link;
        struct
        {
            UINT Dummy;
        } Text;
    } u;
} DOC_ITEM, *PDOC_ITEM;

typedef struct
{
    HWND      Self;         /* The window handle for this control */
    PDOC_ITEM Items;        /* Address to the first document item */
    BOOL      HasFocus;     /* Whether the control has the input focus */
    int       MouseDownID;  /* ID of the link that the mouse button first selected */
    HFONT     Font;         /* Handle to the font for text */
    HFONT     LinkFont;     /* Handle to the font for links */
    COLORREF  TextColor;    /* Color of the text */
    COLORREF  LinkColor;    /* Color of links */
    COLORREF  VisitedColor; /* Color of visited links */
} SYSLINK_INFO;

static const WCHAR SL_LINKOPEN[] =  { '<','a', 0 };
static const WCHAR SL_HREF[] =      { 'h','r','e','f','=','\"',0 };
static const WCHAR SL_ID[] =        { 'i','d','=','\"',0 };
static const WCHAR SL_LINKCLOSE[] = { '<','/','a','>',0 };

/* Control configuration constants */

#define SL_LEFTMARGIN   (0)
#define SL_TOPMARGIN    (0)
#define SL_RIGHTMARGIN  (0)
#define SL_BOTTOMMARGIN (0)

/***********************************************************************
 * SYSLINK_FreeDocItem
 * Frees all data and gdi objects associated with a document item
 */
static VOID SYSLINK_FreeDocItem (PDOC_ITEM DocItem)
{
    if(DocItem->Type == slLink)
    {
        SYSLINK_Free(DocItem->u.Link.szID);
        SYSLINK_Free(DocItem->u.Link.szUrl);
    }

    if(DocItem->Type == slLink && DocItem->u.Link.hRgn != NULL)
    {
        DeleteObject(DocItem->u.Link.hRgn);
    }

    /* we don't free Text because it's just a pointer to a character in the
       entire window text string */

    SYSLINK_Free(DocItem);
}

/***********************************************************************
 * SYSLINK_AppendDocItem
 * Create and append a new document item.
 */
static PDOC_ITEM SYSLINK_AppendDocItem (SYSLINK_INFO *infoPtr, LPWSTR Text, UINT textlen,
                                        SL_ITEM_TYPE type, PDOC_ITEM LastItem)
{
    PDOC_ITEM Item;
    Item = SYSLINK_Alloc(sizeof(DOC_ITEM) + ((textlen + 1) * sizeof(WCHAR)));
    if(Item == NULL)
    {
        ERR("Failed to alloc DOC_ITEM structure!\n");
        return NULL;
    }
    textlen = min(textlen, lstrlenW(Text));

    Item->Next = NULL;
    Item->Text = (LPWSTR)(Item + 1);
    Item->nText = textlen;
    Item->Type = type;
    Item->Blocks = NULL;
    
    if(LastItem != NULL)
    {
        LastItem->Next = Item;
    }
    else
    {
        infoPtr->Items = Item;
    }
    
    lstrcpynW(Item->Text, Text, textlen + 1);
    Item->Text[textlen] = 0;
    
    return Item;
}

/***********************************************************************
 * SYSLINK_ClearDoc
 * Clears the document tree
 */
static VOID SYSLINK_ClearDoc (SYSLINK_INFO *infoPtr)
{
    PDOC_ITEM Item, Next;
    
    Item = infoPtr->Items;
    while(Item != NULL)
    {
        Next = Item->Next;
        SYSLINK_FreeDocItem(Item);
        Item = Next;
    }
    
    infoPtr->Items = NULL;
}

/***********************************************************************
 * SYSLINK_ParseText
 * Parses the window text string and creates a document. Returns the
 * number of document items created.
 */
static UINT SYSLINK_ParseText (SYSLINK_INFO *infoPtr, LPWSTR Text)
{
    WCHAR *current, *textstart, *linktext, *firsttag;
    int taglen = 0, textlen, linklen, docitems = 0;
    PDOC_ITEM Last = NULL;
    SL_ITEM_TYPE CurrentType = slText;
    DWORD Style;
    LPWSTR lpID, lpUrl;
    UINT lenId, lenUrl;

    Style = GetWindowLongW(infoPtr->Self, GWL_STYLE);

    firsttag = NULL;
    textstart = NULL;
    linktext = NULL;
    textlen = 0;
    linklen = 0;
    
    for(current = (WCHAR*)Text; *current != 0;)
    {
        if(*current == '<')
        {
            if(!StrCmpNIW(current, SL_LINKOPEN, 2) && (CurrentType == slText))
            {
                BOOL ValidParam = FALSE, ValidLink = FALSE;

                switch (*(current + 2))
                {
                case '>':
                    /* we just have to deal with a <a> tag */
                    taglen = 3;
                    ValidLink = TRUE;
                    ValidParam = TRUE;
                    firsttag = current;
                    linklen = 0;
                    lpID = NULL;
                    lpUrl = NULL;
                    break;
                case ' ':
                {
                    /* we expect parameters, parse them */
                    LPWSTR *CurrentParameter = NULL;
                    UINT *CurrentParameterLen = NULL;
                    WCHAR *tmp;

                    taglen = 3;
                    tmp = current + taglen;
                    lpID = NULL;
                    lpUrl = NULL;
                    
CheckParameter:
                    /* compare the current position with all known parameters */
                    if(!StrCmpNIW(tmp, SL_HREF, 6))
                    {
                        taglen += 6;
                        ValidParam = TRUE;
                        CurrentParameter = &lpUrl;
                        CurrentParameterLen = &lenUrl;
                    }
                    else if(!StrCmpNIW(tmp, SL_ID, 4))
                    {
                        taglen += 4;
                        ValidParam = TRUE;
                        CurrentParameter = &lpID;
                        CurrentParameterLen = &lenId;
                    }
                    else
                    {
                        ValidParam = FALSE;
                    }
                    
                    if(ValidParam)
                    {
                        /* we got a known parameter, now search until the next " character.
                           If we can't find a " character, there's a syntax error and we just assume it's text */
                        ValidParam = FALSE;
                        *CurrentParameter = current + taglen;
                        *CurrentParameterLen = 0;

                        for(tmp = *CurrentParameter; *tmp != 0; tmp++)
                        {
                            taglen++;
                            if(*tmp == '\"')
                            {
                                ValidParam = TRUE;
                                tmp++;
                                break;
                            }
                            (*CurrentParameterLen)++;
                        }
                    }
                    if(ValidParam)
                    {
                        /* we're done with this parameter, now there are only 2 possibilities:
                         * 1. another parameter is coming, so expect a ' ' (space) character
                         * 2. the tag is being closed, so expect a '<' character
                         */
                        switch(*tmp)
                        {
                        case ' ':
                            /* we expect another parameter, do the whole thing again */
                            taglen++;
                            tmp++;
                            goto CheckParameter;

                        case '>':
                            /* the tag is being closed, we're done */
                            ValidLink = TRUE;
                            taglen++;
                            break;
                        default:
                            tmp++;
                            break;
                        }
                    }
                    
                    break;
                }
                }
                
                if(ValidLink && ValidParam)
                {
                    /* the <a ...> tag appears to be valid. save all information
                       so we can add the link if we find a valid </a> tag later */
                    CurrentType = slLink;
                    linktext = current + taglen;
                    linklen = 0;
                    firsttag = current;
                }
                else
                {
                    taglen = 1;
                    lpID = NULL;
                    lpUrl = NULL;
                    if(textstart == NULL)
                    {
                        textstart = current;
                    }
                }
            }
            else if(!StrCmpNIW(current, SL_LINKCLOSE, 4) && (CurrentType == slLink) && firsttag)
            {
                /* there's a <a...> tag opened, first add the previous text, if present */
                if(textstart != NULL && textlen > 0 && firsttag > textstart)
                {
                    Last = SYSLINK_AppendDocItem(infoPtr, textstart, firsttag - textstart, slText, Last);
                    if(Last == NULL)
                    {
                        ERR("Unable to create new document item!\n");
                        return docitems;
                    }
                    docitems++;
                    textstart = NULL;
                    textlen = 0;
                }
                
                /* now it's time to add the link to the document */
                current += 4;
                if(linktext != NULL && linklen > 0)
                {
                    Last = SYSLINK_AppendDocItem(infoPtr, linktext, linklen, slLink, Last);
                    if(Last == NULL)
                    {
                        ERR("Unable to create new document item!\n");
                        return docitems;
                    }
                    docitems++;
                    if(CurrentType == slLink)
                    {
                        int nc;

                        if(!(Style & WS_DISABLED))
                        {
                            Last->u.Link.state |= LIS_ENABLED;
                        }
                        /* Copy the tag parameters */
                        if(lpID != NULL)
                        {
                            nc = min(lenId, strlenW(lpID));
                            nc = min(nc, MAX_LINKID_TEXT);
                            Last->u.Link.szID = SYSLINK_Alloc((MAX_LINKID_TEXT + 1) * sizeof(WCHAR));
                            if(Last->u.Link.szID != NULL)
                            {
                                lstrcpynW(Last->u.Link.szID, lpID, nc + 1);
                                Last->u.Link.szID[nc] = 0;
                            }
                        }
                        else
                            Last->u.Link.szID = NULL;
                        if(lpUrl != NULL)
                        {
                            nc = min(lenUrl, strlenW(lpUrl));
                            nc = min(nc, L_MAX_URL_LENGTH);
                            Last->u.Link.szUrl = SYSLINK_Alloc((L_MAX_URL_LENGTH + 1) * sizeof(WCHAR));
                            if(Last->u.Link.szUrl != NULL)
                            {
                                lstrcpynW(Last->u.Link.szUrl, lpUrl, nc + 1);
                                Last->u.Link.szUrl[nc] = 0;
                            }
                        }
                        else
                            Last->u.Link.szUrl = NULL;
                    }
                    linktext = NULL;
                }
                CurrentType = slText;
                firsttag = NULL;
                textstart = NULL;
                continue;
            }
            else
            {
                /* we don't know what tag it is, so just continue */
                taglen = 1;
                linklen++;
                if(CurrentType == slText && textstart == NULL)
                {
                    textstart = current;
                }
            }
            
            textlen += taglen;
            current += taglen;
        }
        else
        {
            textlen++;
            linklen++;

            /* save the pointer of the current text item if we couldn't find a tag */
            if(textstart == NULL && CurrentType == slText)
            {
                textstart = current;
            }
            
            current++;
        }
    }
    
    if(textstart != NULL && textlen > 0)
    {
        Last = SYSLINK_AppendDocItem(infoPtr, textstart, textlen, CurrentType, Last);
        if(Last == NULL)
        {
            ERR("Unable to create new document item!\n");
            return docitems;
        }
        if(CurrentType == slLink)
        {
            int nc;

            if(!(Style & WS_DISABLED))
            {
                Last->u.Link.state |= LIS_ENABLED;
            }
            /* Copy the tag parameters */
            if(lpID != NULL)
            {
                nc = min(lenId, strlenW(lpID));
                nc = min(nc, MAX_LINKID_TEXT);
                Last->u.Link.szID = SYSLINK_Alloc((MAX_LINKID_TEXT + 1) * sizeof(WCHAR));
                if(Last->u.Link.szID != NULL)
                {
                    lstrcpynW(Last->u.Link.szID, lpID, nc + 1);
                    Last->u.Link.szID[nc] = 0;
                }
            }
            else
                Last->u.Link.szID = NULL;
            if(lpUrl != NULL)
            {
                nc = min(lenUrl, strlenW(lpUrl));
                nc = min(nc, L_MAX_URL_LENGTH);
                Last->u.Link.szUrl = SYSLINK_Alloc((L_MAX_URL_LENGTH + 1) * sizeof(WCHAR));
                if(Last->u.Link.szUrl != NULL)
                {
                    lstrcpynW(Last->u.Link.szUrl, lpUrl, nc + 1);
                    Last->u.Link.szUrl[nc] = 0;
                }
            }
            else
                Last->u.Link.szUrl = NULL;
        }
        docitems++;
    }

    if(linktext != NULL && linklen > 0)
    {
        /* we got a unclosed link, just display the text */
        Last = SYSLINK_AppendDocItem(infoPtr, linktext, linklen, slText, Last);
        if(Last == NULL)
        {
            ERR("Unable to create new document item!\n");
            return docitems;
        }
        docitems++;
    }

    return docitems;
}

/***********************************************************************
 * SYSLINK_RepaintLink
 * Repaints a link.
 */
static VOID SYSLINK_RepaintLink (SYSLINK_INFO *infoPtr, PDOC_ITEM DocItem)
{
    if(DocItem->Type != slLink)
    {
        ERR("DocItem not a link!\n");
        return;
    }
    
    if(DocItem->u.Link.hRgn != NULL)
    {
        /* repaint the region */
        RedrawWindow(infoPtr->Self, NULL, DocItem->u.Link.hRgn, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

/***********************************************************************
 * SYSLINK_GetLinkItemByIndex
 * Retreives a document link by it's index
 */
static PDOC_ITEM SYSLINK_GetLinkItemByIndex (SYSLINK_INFO *infoPtr, int iLink)
{
    PDOC_ITEM Current = infoPtr->Items;

    while(Current != NULL)
    {
        if((Current->Type == slLink) && (iLink-- <= 0))
        {
            return Current;
        }
        Current = Current->Next;
    }
    return NULL;
}

/***********************************************************************
 * SYSLINK_GetFocusLink
 * Retreives the link that has the LIS_FOCUSED bit
 */
static PDOC_ITEM SYSLINK_GetFocusLink (SYSLINK_INFO *infoPtr, int *LinkId)
{
    PDOC_ITEM Current = infoPtr->Items;
    int id = 0;

    while(Current != NULL)
    {
        if((Current->Type == slLink))
        {
            if(Current->u.Link.state & LIS_FOCUSED)
            {
                if(LinkId != NULL)
                    *LinkId = id;
                return Current;
            }
            id++;
        }
        Current = Current->Next;
    }
    return NULL;
}

/***********************************************************************
 * SYSLINK_GetNextLink
 * Gets the next link
 */
static PDOC_ITEM SYSLINK_GetNextLink (SYSLINK_INFO *infoPtr, PDOC_ITEM Current)
{
    for(Current = (Current != NULL ? Current->Next : infoPtr->Items);
        Current != NULL;
        Current = Current->Next)
    {
        if(Current->Type == slLink)
        {
            return Current;
        }
    }
    return NULL;
}

/***********************************************************************
 * SYSLINK_GetPrevLink
 * Gets the previous link
 */
static PDOC_ITEM SYSLINK_GetPrevLink (SYSLINK_INFO *infoPtr, PDOC_ITEM Current)
{
    if(Current == NULL)
    {
        /* returns the last link */
        PDOC_ITEM Last = NULL;
        
        for(Current = infoPtr->Items; Current != NULL; Current = Current->Next)
        {
            if(Current->Type == slLink)
            {
                Last = Current;
            }
        }
        return Last;
    }
    else
    {
        /* returns the previous link */
        PDOC_ITEM Cur, Prev = NULL;
        
        for(Cur = infoPtr->Items; Cur != NULL; Cur = Cur->Next)
        {
            if(Cur == Current)
            {
                break;
            }
            if(Cur->Type == slLink)
            {
                Prev = Cur;
            }
        }
        return Prev;
    }
}

/***********************************************************************
 * SYSLINK_WrapLine
 * Tries to wrap a line.
 */
static BOOL SYSLINK_WrapLine (HDC hdc, LPWSTR Text, WCHAR BreakChar, int *LineLen, int nFit, LPSIZE Extent, int Width)
{
    WCHAR *Current;

    if(nFit == *LineLen)
    {
        return FALSE;
    }

    *LineLen = nFit;

    Current = Text + nFit;
    
    /* check if we're in the middle of a word */
    if((*Current) != BreakChar)
    {
        /* search for the beginning of the word */
        while(Current > Text && (*(Current - 1)) != BreakChar)
        {
            Current--;
            (*LineLen)--;
        }
        
        if((*LineLen) == 0)
        {
            Extent->cx = 0;
            Extent->cy = 0;
        }
        return TRUE;
    }

    return TRUE;
}

/***********************************************************************
 * SYSLINK_Render
 * Renders the document in memory
 */
static VOID SYSLINK_Render (SYSLINK_INFO *infoPtr, HDC hdc)
{
    RECT rc;
    PDOC_ITEM Current;
    HGDIOBJ hOldFont;
    int x, y, LineHeight;
    TEXTMETRICW tm;
    
    GetClientRect(infoPtr->Self, &rc);
    rc.right -= SL_RIGHTMARGIN;
    rc.bottom -= SL_BOTTOMMARGIN;
    
    if(rc.right - SL_LEFTMARGIN < 0 || rc.bottom - SL_TOPMARGIN < 0) return;
    
    hOldFont = SelectObject(hdc, infoPtr->Font);
    GetTextMetricsW(hdc, &tm);
    
    x = SL_LEFTMARGIN;
    y = SL_TOPMARGIN;
    LineHeight = 0;
    
    for(Current = infoPtr->Items; Current != NULL; Current = Current->Next)
    {
        int n, nBlocks;
        LPWSTR tx;
        PDOC_TEXTBLOCK bl, cbl;
        INT nFit;
        SIZE szDim;
        
        if(Current->nText == 0)
        {
            ERR("DOC_ITEM with no text?!\n");
            continue;
        }
        
        tx = Current->Text;
        n = Current->nText;
        bl = Current->Blocks;
        nBlocks = 0;
        
        if(Current->Type == slText)
        {
            SelectObject(hdc, infoPtr->Font);
        }
        else if(Current->Type == slLink)
        {
            SelectObject(hdc, infoPtr->LinkFont);
        }
        
        while(n > 0)
        {
            if(GetTextExtentExPointW(hdc, tx, n, rc.right - x, &nFit, NULL, &szDim))
            {
                int LineLen = n;
                BOOL Wrap = SYSLINK_WrapLine(hdc, tx, tm.tmBreakChar, &LineLen, nFit, &szDim, rc.right - x);
                
                if(LineLen == 0)
                {
                    if(x > SL_LEFTMARGIN)
                    {
                        /* move one line down, the word didn't fit into the line */
                        x = SL_LEFTMARGIN;
                        y += LineHeight;
                        LineHeight = 0;
                        continue;
                    }
                    else
                    {
                        /* the word starts at the beginning of the line and doesn't
                           fit into the line, so break it at the last character that fits */
                        LineLen = max(nFit, 1);
                    }
                }
                
                if(LineLen != n)
                {
                    GetTextExtentExPointW(hdc, tx, LineLen, rc.right - x, NULL, NULL, &szDim);
                }
                
                if(bl != NULL)
                {
                    bl = SYSLINK_ReAlloc(bl, ++nBlocks * sizeof(DOC_TEXTBLOCK));
                }
                else
                {
                    bl = SYSLINK_Alloc(++nBlocks * sizeof(DOC_TEXTBLOCK));
                }
                
                if(bl != NULL)
                {
                    cbl = bl + nBlocks - 1;
                    
                    cbl->nChars = LineLen;
                    cbl->rc.left = x;
                    cbl->rc.top = y;
                    cbl->rc.right = x + szDim.cx;
                    cbl->rc.bottom = y + szDim.cy;
                    
                    x += szDim.cx;
                    LineHeight = max(LineHeight, szDim.cy);
                    
                    /* (re)calculate the link's region */
                    if(Current->Type == slLink)
                    {
                        if(nBlocks <= 1)
                        {
                            if(Current->u.Link.hRgn != NULL)
                            {
                                DeleteObject(Current->u.Link.hRgn);
                            }
                            /* initialize the link's hRgn */
                            Current->u.Link.hRgn = CreateRectRgnIndirect(&cbl->rc);
                        }
                        else if(Current->u.Link.hRgn != NULL)
                        {
                            HRGN hrgn;
                            hrgn = CreateRectRgnIndirect(&cbl->rc);
                            /* add the rectangle */
                            CombineRgn(Current->u.Link.hRgn, Current->u.Link.hRgn, hrgn, RGN_OR);
                            DeleteObject(hrgn);
                        }
                    }
                    
                    if(Wrap)
                    {
                        x = SL_LEFTMARGIN;
                        y += LineHeight;
                        LineHeight = 0;
                    }
                }
                else
                {
                    ERR("Failed to alloc DOC_TEXTBLOCK structure!\n");
                    break;
                }
                n -= LineLen;
                tx += LineLen;
            }
            else
            {
                ERR("GetTextExtentExPoint() failed?!\n");
                n--;
            }
        }
        Current->Blocks = bl;
    }
    
    SelectObject(hdc, hOldFont);
}

/***********************************************************************
 * SYSLINK_Draw
 * Draws the SysLink control.
 */
static LRESULT SYSLINK_Draw (SYSLINK_INFO *infoPtr, HDC hdc)
{
    RECT rc;
    PDOC_ITEM Current;
    HFONT hOldFont;
    COLORREF OldTextColor, OldBkColor;

    hOldFont = SelectObject(hdc, infoPtr->Font);
    OldTextColor = SetTextColor(hdc, infoPtr->TextColor);
    OldBkColor = SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
    
    GetClientRect(infoPtr->Self, &rc);
    rc.right -= SL_RIGHTMARGIN + SL_LEFTMARGIN;
    rc.bottom -= SL_BOTTOMMARGIN + SL_TOPMARGIN;

    if(rc.right < 0 || rc.bottom < 0) return 0;

    for(Current = infoPtr->Items; Current != NULL; Current = Current->Next)
    {
        int n;
        LPWSTR tx;
        PDOC_TEXTBLOCK bl;
        
        bl = Current->Blocks;
        if(bl != NULL)
        {
            tx = Current->Text;
            n = Current->nText;

            if(Current->Type == slText)
            {
                 SelectObject(hdc, infoPtr->Font);
                 SetTextColor(hdc, infoPtr->TextColor);
            }
            else
            {
                 SelectObject(hdc, infoPtr->LinkFont);
                 SetTextColor(hdc, (!(Current->u.Link.state & LIS_VISITED) ? infoPtr->LinkColor : infoPtr->VisitedColor));
            }

            while(n > 0)
            {
                ExtTextOutW(hdc, bl->rc.left, bl->rc.top, ETO_OPAQUE | ETO_CLIPPED, &bl->rc, tx, bl->nChars, NULL);
                if((Current->Type == slLink) && (Current->u.Link.state & LIS_FOCUSED) && infoPtr->HasFocus)
                {
                    COLORREF PrevColor;
                    PrevColor = SetBkColor(hdc, OldBkColor);
                    DrawFocusRect(hdc, &bl->rc);
                    SetBkColor(hdc, PrevColor);
                }
                tx += bl->nChars;
                n -= bl->nChars;
                bl++;
            }
        }
    }

    SetBkColor(hdc, OldBkColor);
    SetTextColor(hdc, OldTextColor);
    SelectObject(hdc, hOldFont);
    
    return 0;
}


/***********************************************************************
 * SYSLINK_Paint
 * Handles the WM_PAINT message.
 */
static LRESULT SYSLINK_Paint (SYSLINK_INFO *infoPtr)
{
    HDC hdc;
    PAINTSTRUCT ps;
    hdc = BeginPaint (infoPtr->Self, &ps);
    SYSLINK_Draw (infoPtr, hdc);
    EndPaint (infoPtr->Self, &ps);
    return 0;
}


/***********************************************************************
 *           SYSLINK_SetFont
 * Set new Font for the SysLink control.
 */
static HFONT SYSLINK_SetFont (SYSLINK_INFO *infoPtr, HFONT hFont, BOOL bRedraw)
{
    HDC hdc;
    LOGFONTW lf;
    HFONT hOldFont = infoPtr->Font;
    infoPtr->Font = hFont;
    
    /* free the underline font */
    if(infoPtr->LinkFont != NULL)
    {
        DeleteObject(infoPtr->LinkFont);
        infoPtr->LinkFont = NULL;
    }

    /* Render text position and word wrapping in memory */
    hdc = GetDC(infoPtr->Self);
    if(hdc != NULL)
    {
        /* create a new underline font */
        if(GetObjectW(infoPtr->Font, sizeof(LOGFONTW), &lf))
        {
            lf.lfUnderline = TRUE;
            infoPtr->LinkFont = CreateFontIndirectW(&lf);
        }
        else
        {
            ERR("Failed to create link font!\n");
        }

        SYSLINK_Render(infoPtr, hdc);
        ReleaseDC(infoPtr->Self, hdc);
    }
    
    if(bRedraw)
    {
        RedrawWindow(infoPtr->Self, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    
    return hOldFont;
}

/***********************************************************************
 *           SYSLINK_SetText
 * Set new text for the SysLink control.
 */
static LRESULT SYSLINK_SetText (SYSLINK_INFO *infoPtr, LPWSTR Text)
{
    int textlen;

    /* clear the document */
    SYSLINK_ClearDoc(infoPtr);
    
    textlen = lstrlenW(Text);
    if(Text == NULL || textlen == 0)
    {
        return TRUE;
    }
    
    /* let's parse the string and create a document */
    if(SYSLINK_ParseText(infoPtr, Text) > 0)
    {
        /* Render text position and word wrapping in memory */
        HDC hdc = GetDC(infoPtr->Self);
        SYSLINK_Render(infoPtr, hdc);
        SYSLINK_Draw(infoPtr, hdc);
        ReleaseDC(infoPtr->Self, hdc);
    }
    
    return TRUE;
}

/***********************************************************************
 *           SYSLINK_SetFocusLink
 * Updates the focus status bits and focusses the specified link.
 * If no document item is specified, the focus bit will be removed from all links.
 * Returns the previous focused item.
 */
static PDOC_ITEM SYSLINK_SetFocusLink (SYSLINK_INFO *infoPtr, PDOC_ITEM DocItem)
{
    PDOC_ITEM Current, PrevFocus = NULL;
    
    for(Current = infoPtr->Items; Current != NULL; Current = Current->Next)
    {
        if(Current->Type == slLink)
        {
            if((PrevFocus == NULL) && (Current->u.Link.state & LIS_FOCUSED))
            {
                PrevFocus = Current;
            }
            
            if(Current == DocItem)
            {
                Current->u.Link.state |= LIS_FOCUSED;
            }
            else
            {
                Current->u.Link.state &= ~LIS_FOCUSED;
            }
        }
    }
    
    return PrevFocus;
}

/***********************************************************************
 *           SYSLINK_SetItem
 * Sets the states and attributes of a link item.
 */
static LRESULT SYSLINK_SetItem (SYSLINK_INFO *infoPtr, PLITEM Item)
{
    PDOC_ITEM di;
    BOOL Repaint = FALSE;
    BOOL Ret = TRUE;

    if(!(Item->mask & LIF_ITEMINDEX) || !(Item->mask & (LIF_FLAGSMASK)))
    {
        ERR("Invalid Flags!\n");
        return FALSE;
    }

    di = SYSLINK_GetLinkItemByIndex(infoPtr, Item->iLink);
    if(di == NULL)
    {
        ERR("Link %d couldn't be found\n", Item->iLink);
        return FALSE;
    }
    
    if(Item->mask & LIF_STATE)
    {
        UINT oldstate = di->u.Link.state;
        /* clear the masked bits */
        di->u.Link.state &= ~(Item->stateMask & LIS_MASK);
        /* copy the bits */
        di->u.Link.state |= (Item->state & Item->stateMask) & LIS_MASK;
        Repaint = (oldstate != di->u.Link.state);
        
        /* update the focus */
        SYSLINK_SetFocusLink(infoPtr, ((di->u.Link.state & LIS_FOCUSED) ? di : NULL));
    }

    if(Item->mask & LIF_ITEMID)
    {
        if(!di->u.Link.szID)
        {
            di->u.Link.szID = SYSLINK_Alloc((MAX_LINKID_TEXT + 1) * sizeof(WCHAR));
            if(!Item->szID)
            {
                ERR("Unable to allocate memory for link id\n");
                Ret = FALSE;
            }
        }
        if(di->u.Link.szID)
        {
            lstrcpynW(di->u.Link.szID, Item->szID, MAX_LINKID_TEXT + 1);
        }
    }

    if(Item->mask & LIF_URL)
    {
        if(!di->u.Link.szUrl)
        {
            di->u.Link.szUrl = SYSLINK_Alloc((MAX_LINKID_TEXT + 1) * sizeof(WCHAR));
            if(!Item->szUrl)
            {
                ERR("Unable to allocate memory for link url\n");
                Ret = FALSE;
            }
        }
        if(di->u.Link.szUrl)
        {
            lstrcpynW(di->u.Link.szUrl, Item->szUrl, MAX_LINKID_TEXT + 1);
        }
    }
    
    if(Repaint)
    {
        SYSLINK_RepaintLink(infoPtr, di);
    }
    
    return Ret;
}

/***********************************************************************
 *           SYSLINK_GetItem
 * Retrieves the states and attributes of a link item.
 */
static LRESULT SYSLINK_GetItem (SYSLINK_INFO *infoPtr, PLITEM Item)
{
    PDOC_ITEM di;
    
    if(!(Item->mask & LIF_ITEMINDEX) || !(Item->mask & (LIF_FLAGSMASK)))
    {
        ERR("Invalid Flags!\n");
        return FALSE;
    }
    
    di = SYSLINK_GetLinkItemByIndex(infoPtr, Item->iLink);
    if(di == NULL)
    {
        ERR("Link %d couldn't be found\n", Item->iLink);
        return FALSE;
    }
    
    if(Item->mask & LIF_STATE)
    {
        Item->state = (di->u.Link.state & Item->stateMask);
        if(!infoPtr->HasFocus)
        {
            /* remove the LIS_FOCUSED bit if the control doesn't have focus */
            Item->state &= ~LIS_FOCUSED;
        }
    }
    
    if(Item->mask & LIF_ITEMID)
    {
        if(di->u.Link.szID)
        {
            lstrcpynW(Item->szID, di->u.Link.szID, MAX_LINKID_TEXT + 1);
        }
        else
        {
            Item->szID[0] = 0;
        }
    }
    
    if(Item->mask & LIF_URL)
    {
        if(di->u.Link.szUrl)
        {
            lstrcpynW(Item->szUrl, di->u.Link.szUrl, L_MAX_URL_LENGTH + 1);
        }
        else
        {
            Item->szUrl[0] = 0;
        }
    }
    
    return TRUE;
}

/***********************************************************************
 *           SYSLINK_HitTest
 * Determines the link the user clicked on.
 */
static LRESULT SYSLINK_HitTest (SYSLINK_INFO *infoPtr, PLHITTESTINFO HitTest)
{
    PDOC_ITEM Current;
    int id = 0;

    for(Current = infoPtr->Items; Current != NULL; Current = Current->Next)
    {
        if(Current->Type == slLink)
        {
            if((Current->u.Link.hRgn != NULL) &&
               PtInRegion(Current->u.Link.hRgn, HitTest->pt.x, HitTest->pt.y))
            {
                HitTest->item.mask = 0;
                HitTest->item.iLink = id;
                HitTest->item.state = 0;
                HitTest->item.stateMask = 0;
                if(Current->u.Link.szID)
                {
                    lstrcpynW(HitTest->item.szID, Current->u.Link.szID, MAX_LINKID_TEXT + 1);
                }
                else
                {
                    HitTest->item.szID[0] = 0;
                }
                if(Current->u.Link.szUrl)
                {
                    lstrcpynW(HitTest->item.szUrl, Current->u.Link.szUrl, L_MAX_URL_LENGTH + 1);
                }
                else
                {
                    HitTest->item.szUrl[0] = 0;
                }
                return TRUE;
            }
            id++;
        }
    }
    
    return FALSE;
}

/***********************************************************************
 *           SYSLINK_GetIdealHeight
 * Returns the preferred height of a link at the current control's width.
 */
static LRESULT SYSLINK_GetIdealHeight (SYSLINK_INFO *infoPtr)
{
    HDC hdc = GetDC(infoPtr->Self);
    if(hdc != NULL)
    {
        LRESULT height;
        TEXTMETRICW tm;
        HGDIOBJ hOldFont = SelectObject(hdc, infoPtr->Font);
        
        if(GetTextMetricsW(hdc, &tm))
        {
            height = tm.tmHeight;
        }
        else
        {
            height = 0;
        }
        SelectObject(hdc, hOldFont);
        ReleaseDC(infoPtr->Self, hdc);
        
        return height;
    }
    return 0;
}

/***********************************************************************
 *           SYSLINK_SendParentNotify
 * Sends a WM_NOTIFY message to the parent window.
 */
static LRESULT SYSLINK_SendParentNotify (SYSLINK_INFO *infoPtr, UINT code, PDOC_ITEM Link, int iLink)
{
    NMLINK nml;

    nml.hdr.hwndFrom = infoPtr->Self;
    nml.hdr.idFrom = GetWindowLongPtrW(infoPtr->Self, GWLP_ID);
    nml.hdr.code = code;

    nml.item.mask = 0;
    nml.item.iLink = iLink;
    nml.item.state = 0;
    nml.item.stateMask = 0;
    if(Link->u.Link.szID)
    {
        lstrcpynW(nml.item.szID, Link->u.Link.szID, MAX_LINKID_TEXT + 1);
    }
    else
    {
        nml.item.szID[0] = 0;
    }
    if(Link->u.Link.szUrl)
    {
        lstrcpynW(nml.item.szUrl, Link->u.Link.szUrl, L_MAX_URL_LENGTH + 1);
    }
    else
    {
        nml.item.szUrl[0] = 0;
    }

    return SendMessageW(GetParent(infoPtr->Self), WM_NOTIFY, (WPARAM)nml.hdr.idFrom, (LPARAM)&nml);
}

/***********************************************************************
 *           SYSLINK_SetFocus
 * Handles receiving the input focus.
 */
static LRESULT SYSLINK_SetFocus (SYSLINK_INFO *infoPtr, HWND PrevFocusWindow)
{
    PDOC_ITEM Focus;
    
    infoPtr->HasFocus = TRUE;

#if 1
    /* FIXME - How to detect whether SHIFT+TAB or just TAB has been pressed?
     *         The problem is we could get this message without keyboard input, too
     */
    Focus = SYSLINK_GetFocusLink(infoPtr, NULL);
    
    if(Focus == NULL && (Focus = SYSLINK_GetNextLink(infoPtr, NULL)))
    {
        SYSLINK_SetFocusLink(infoPtr, Focus);
    }
#else
    /* This is a temporary hack since I'm not really sure how to detect which link to select.
       See message above! */
    Focus = SYSLINK_GetNextLink(infoPtr, NULL);
    if(Focus != NULL)
    {
        SYSLINK_SetFocusLink(infoPtr, Focus);
    }
#endif
    
    SYSLINK_RepaintLink(infoPtr, Focus);
    
    return 0;
}

/***********************************************************************
 *           SYSLINK_KillFocus
 * Handles losing the input focus.
 */
static LRESULT SYSLINK_KillFocus (SYSLINK_INFO *infoPtr, HWND NewFocusWindow)
{
    PDOC_ITEM Focus;
    
    infoPtr->HasFocus = FALSE;
    Focus = SYSLINK_GetFocusLink(infoPtr, NULL);
    
    if(Focus != NULL)
    {
        SYSLINK_RepaintLink(infoPtr, Focus);
    }

    return 0;
}

/***********************************************************************
 *           SYSLINK_LinkAtPt
 * Returns a link at the specified position
 */
static PDOC_ITEM SYSLINK_LinkAtPt (SYSLINK_INFO *infoPtr, POINT *pt, int *LinkId, BOOL MustBeEnabled)
{
    PDOC_ITEM Current;
    int id = 0;

    for(Current = infoPtr->Items; Current != NULL; Current = Current->Next)
    {
        if((Current->Type == slLink) && (Current->u.Link.hRgn != NULL) &&
           PtInRegion(Current->u.Link.hRgn, pt->x, pt->y) &&
           (!MustBeEnabled || (MustBeEnabled && (Current->u.Link.state & LIS_ENABLED))))
        {
            if(LinkId != NULL)
            {
                *LinkId = id;
            }
            return Current;
        }
        id++;
    }

    return NULL;
}

/***********************************************************************
 *           SYSLINK_LButtonDown
 * Handles mouse clicks
 */
static LRESULT SYSLINK_LButtonDown (SYSLINK_INFO *infoPtr, DWORD Buttons, POINT *pt)
{
    PDOC_ITEM Current, Old;
    int id;

    Current = SYSLINK_LinkAtPt(infoPtr, pt, &id, TRUE);
    if(Current != NULL)
    {
      Old = SYSLINK_SetFocusLink(infoPtr, Current);
      if(Old != NULL && Old != Current)
      {
          SYSLINK_RepaintLink(infoPtr, Old);
      }
      infoPtr->MouseDownID = id;
      SYSLINK_RepaintLink(infoPtr, Current);
      SetFocus(infoPtr->Self);
    }

    return 0;
}

/***********************************************************************
 *           SYSLINK_LButtonUp
 * Handles mouse clicks
 */
static LRESULT SYSLINK_LButtonUp (SYSLINK_INFO *infoPtr, DWORD Buttons, POINT *pt)
{
    if(infoPtr->MouseDownID > -1)
    {
        PDOC_ITEM Current;
        int id;
        
        Current = SYSLINK_LinkAtPt(infoPtr, pt, &id, TRUE);
        if((Current != NULL) && (Current->u.Link.state & LIS_FOCUSED) && (infoPtr->MouseDownID == id))
        {
            SYSLINK_SendParentNotify(infoPtr, NM_CLICK, Current, id);
        }
    }

    infoPtr->MouseDownID = -1;

    return 0;
}

/***********************************************************************
 *           SYSLINK_OnEnter
 * Handles ENTER key events
 */
static BOOL SYSLINK_OnEnter (SYSLINK_INFO *infoPtr)
{
    if(infoPtr->HasFocus)
    {
        PDOC_ITEM Focus;
        int id;
        
        Focus = SYSLINK_GetFocusLink(infoPtr, &id);
        if(Focus != NULL)
        {
            SYSLINK_SendParentNotify(infoPtr, NM_RETURN, Focus, id);
            return TRUE;
        }
    }
    return FALSE;
}

/***********************************************************************
 *           SYSKEY_SelectNextPrevLink
 * Changes the currently focused link
 */
static BOOL SYSKEY_SelectNextPrevLink (SYSLINK_INFO *infoPtr, BOOL Prev)
{
    if(infoPtr->HasFocus)
    {
        PDOC_ITEM Focus;
        int id;

        Focus = SYSLINK_GetFocusLink(infoPtr, &id);
        if(Focus != NULL)
        {
            PDOC_ITEM NewFocus, OldFocus;

            if(Prev)
                NewFocus = SYSLINK_GetPrevLink(infoPtr, Focus);
            else
                NewFocus = SYSLINK_GetNextLink(infoPtr, Focus);

            if(NewFocus != NULL)
            {
                OldFocus = SYSLINK_SetFocusLink(infoPtr, NewFocus);

                if(OldFocus != NewFocus)
                {
                    SYSLINK_RepaintLink(infoPtr, OldFocus);
                }
                SYSLINK_RepaintLink(infoPtr, NewFocus);
                return TRUE;
            }
        }
    }
    return FALSE;
}

/***********************************************************************
 *           SYSKEY_SelectNextPrevLink
 * Determines if there's a next or previous link to decide whether the control
 * should capture the tab key message
 */
static BOOL SYSLINK_NoNextLink (SYSLINK_INFO *infoPtr, BOOL Prev)
{
    PDOC_ITEM Focus, NewFocus;

    Focus = SYSLINK_GetFocusLink(infoPtr, NULL);
    if(Prev)
        NewFocus = SYSLINK_GetPrevLink(infoPtr, Focus);
    else
        NewFocus = SYSLINK_GetNextLink(infoPtr, Focus);

    return NewFocus == NULL;
}

/***********************************************************************
 *           SysLinkWindowProc
 */
static LRESULT WINAPI SysLinkWindowProc(HWND hwnd, UINT message,
                                        WPARAM wParam, LPARAM lParam)
{
    SYSLINK_INFO *infoPtr;

    TRACE("hwnd=%p msg=%04x wparam=%x lParam=%lx\n", hwnd, message, wParam, lParam);

    infoPtr = (SYSLINK_INFO *)GetWindowLongPtrW(hwnd, 0);

    if (!infoPtr && message != WM_CREATE)
        return DefWindowProcW( hwnd, message, wParam, lParam );

    switch(message) {
    case WM_PAINT:
        return SYSLINK_Paint (infoPtr);

    case WM_SETCURSOR:
    {
        LHITTESTINFO ht;
        DWORD mp = GetMessagePos();
        
        ht.pt.x = (short)LOWORD(mp);
        ht.pt.y = (short)HIWORD(mp);
        
        ScreenToClient(infoPtr->Self, &ht.pt);
        if(SYSLINK_HitTest (infoPtr, &ht))
        {
            SetCursor(LoadCursorW(0, (LPCWSTR)IDC_HAND));
            return TRUE;
        }
        /* let the default window proc handle this message */
        return DefWindowProcW(hwnd, message, wParam, lParam);

    }

    case WM_SIZE:
    {
        HDC hdc = GetDC(infoPtr->Self);
        if(hdc != NULL)
        {
          SYSLINK_Render(infoPtr, hdc);
          ReleaseDC(infoPtr->Self, hdc);
        }
        return 0;
    }

    case WM_GETFONT:
        return (LRESULT)infoPtr->Font;

    case WM_SETFONT:
        return (LRESULT)SYSLINK_SetFont(infoPtr, (HFONT)wParam, (BOOL)lParam);

    case WM_SETTEXT:
        SYSLINK_SetText(infoPtr, (LPWSTR)lParam);
        return DefWindowProcW(hwnd, message, wParam, lParam);

    case WM_LBUTTONDOWN:
    {
        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        return SYSLINK_LButtonDown(infoPtr, wParam, &pt);
    }
    case WM_LBUTTONUP:
    {
        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        return SYSLINK_LButtonUp(infoPtr, wParam, &pt);
    }
    
    case WM_KEYDOWN:
    {
        switch(wParam)
        {
        case VK_RETURN:
            SYSLINK_OnEnter(infoPtr);
            return 0;
        case VK_TAB:
        {
            BOOL shift = GetKeyState(VK_SHIFT) & 0x8000;
            SYSKEY_SelectNextPrevLink(infoPtr, shift);
            return 0;
        }
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    
    case WM_GETDLGCODE:
    {
        LRESULT Ret = DLGC_HASSETSEL;
        int vk = (lParam != 0 ? (int)((LPMSG)lParam)->wParam : 0);
        switch(vk)
        {
        case VK_RETURN:
            Ret |= DLGC_WANTMESSAGE;
            break;
        case VK_TAB:
        {
            BOOL shift = GetKeyState(VK_SHIFT) & 0x8000;
            if(!SYSLINK_NoNextLink(infoPtr, shift))
            {
                Ret |= DLGC_WANTTAB;
            }
            else
            {
                Ret |= DLGC_WANTCHARS;
            }
            break;
        }
        }
        return Ret;
    }
    
    case WM_NCHITTEST:
    {
        POINT pt;
        RECT rc;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        
        GetClientRect(infoPtr->Self, &rc);
        ScreenToClient(infoPtr->Self, &pt);
        if(pt.x < 0 || pt.y < 0 || pt.x > rc.right || pt.y > rc.bottom)
        {
            return HTNOWHERE;
        }

        if(SYSLINK_LinkAtPt(infoPtr, &pt, NULL, FALSE))
        {
            return HTCLIENT;
        }
        
        return HTTRANSPARENT;
    }

    case LM_HITTEST:
        return SYSLINK_HitTest(infoPtr, (PLHITTESTINFO)lParam);

    case LM_SETITEM:
        return SYSLINK_SetItem(infoPtr, (PLITEM)lParam);

    case LM_GETITEM:
        return SYSLINK_GetItem(infoPtr, (PLITEM)lParam);

    case LM_GETIDEALHEIGHT:
        return SYSLINK_GetIdealHeight(infoPtr);

    case WM_SETFOCUS:
        return SYSLINK_SetFocus(infoPtr, (HWND)wParam);

    case WM_KILLFOCUS:
        return SYSLINK_KillFocus(infoPtr, (HWND)wParam);

    case WM_CREATE:
        /* allocate memory for info struct */
        infoPtr = (SYSLINK_INFO *)SYSLINK_Alloc (sizeof(SYSLINK_INFO));
        if (!infoPtr) return -1;
        SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

        /* initialize the info struct */
        infoPtr->Self = hwnd;
        infoPtr->Font = 0;
        infoPtr->LinkFont = 0;
        infoPtr->Items = NULL;
        infoPtr->HasFocus = FALSE;
        infoPtr->MouseDownID = -1;
        infoPtr->TextColor = GetSysColor(COLOR_WINDOWTEXT);
        infoPtr->LinkColor = GetSysColor(COLOR_HIGHLIGHT);
        infoPtr->VisitedColor = GetSysColor(COLOR_HIGHLIGHT);
        TRACE("SysLink Ctrl creation, hwnd=%p\n", hwnd);
        lParam = (LPARAM)(((LPCREATESTRUCTW)lParam)->lpszName);
        SYSLINK_SetText(infoPtr, (LPWSTR)lParam);
        return 0;

    case WM_DESTROY:
        TRACE("SysLink Ctrl destruction, hwnd=%p\n", hwnd);
        SYSLINK_ClearDoc(infoPtr);
        if(infoPtr->Font != 0) DeleteObject(infoPtr->Font);
        if(infoPtr->LinkFont != 0) DeleteObject(infoPtr->LinkFont);
        SYSLINK_Free (infoPtr);
        SetWindowLongPtrW(hwnd, 0, 0);
        return 0;

    default:
        if ((message >= WM_USER) && (message < WM_APP))
	    ERR("unknown msg %04x wp=%04x lp=%08lx\n", message, wParam, lParam );
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}


/***********************************************************************
 * SYSLINK_Register [Internal]
 *
 * Registers the SysLink window class.
 */
VOID SYSLINK_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(wndClass));
    wndClass.style         = CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW;
    wndClass.lpfnWndProc   = SysLinkWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof (SYSLINK_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.lpszClassName = WC_LINK;

    RegisterClassW (&wndClass);
}


/***********************************************************************
 * SYSLINK_Unregister [Internal]
 *
 * Unregisters the SysLink window class.
 */
VOID SYSLINK_Unregister (void)
{
    UnregisterClassW (WC_LINK, NULL);
}
