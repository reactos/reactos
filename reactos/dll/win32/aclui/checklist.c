/*
 * ReactOS Access Control List Editor
 * Copyright (C) 2004-2005 ReactOS Team
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
 */
/* $Id$
 *
 * PROJECT:         ReactOS Access Control List Editor
 * FILE:            lib/aclui/checklist.c
 * PURPOSE:         Access Control List Editor
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      07/01/2005  Created
 */
#include <precomp.h>

#define NDEBUG
#include <debug.h>

static const WCHAR szCheckListWndClass[] = L"CHECKLIST_ACLUI";

#define CI_TEXT_MARGIN_WIDTH    (8)
#define CI_TEXT_MARGIN_HEIGHT   (3)
#define CI_TEXT_SELECTIONMARGIN (1)

#define TIMER_ID_SETHITFOCUS    (1)
#define TIMER_ID_RESETQUICKSEARCH       (2)

#define DEFAULT_QUICKSEARCH_SETFOCUS_DELAY      (2000)
#define DEFAULT_QUICKSEARCH_RESET_DELAY (3000)

typedef struct _CHECKITEM
{
    struct _CHECKITEM *Next;
    ACCESS_MASK AccessMask;
    DWORD State;
    WCHAR Name[1];
} CHECKITEM, *PCHECKITEM;

typedef struct _CHECKLISTWND
{
    HWND hSelf;
    HWND hNotify;
    HFONT hFont;

    PCHECKITEM CheckItemListHead;
    UINT CheckItemCount;

    INT ItemHeight;

    PCHECKITEM FocusedCheckItem;
    UINT FocusedCheckItemBox;

    COLORREF TextColor[2];
    INT CheckBoxLeft[2];

    PCHECKITEM QuickSearchHitItem;
    WCHAR QuickSearchText[65];
    UINT QuickSearchSetFocusDelay;
    UINT QuickSearchResetDelay;

    DWORD CaretWidth;

    DWORD UIState;

#if SUPPORT_UXTHEME
    PCHECKITEM HoveredCheckItem;
    UINT HoveredCheckItemBox;
    UINT HoverTime;

    HTHEME ThemeHandle;
#endif

    UINT HasFocus : 1;
    UINT FocusedPushed : 1;
    UINT QuickSearchEnabled : 1;
    UINT ShowingCaret : 1;
} CHECKLISTWND, *PCHECKLISTWND;

static VOID EscapeQuickSearch(IN PCHECKLISTWND infoPtr);
#if SUPPORT_UXTHEME
static VOID ChangeCheckItemHotTrack(IN PCHECKLISTWND infoPtr,
                                    IN PCHECKITEM NewHotTrack,
                                    IN UINT NewHotTrackBox);
#endif
static VOID ChangeCheckItemFocus(IN PCHECKLISTWND infoPtr,
                                 IN PCHECKITEM NewFocus,
                                 IN UINT NewFocusBox);

/******************************************************************************/

static LRESULT
NotifyControlParent(IN PCHECKLISTWND infoPtr,
                    IN UINT code,
                    IN OUT PVOID data)
{
    LRESULT Ret = 0;
    
    if (infoPtr->hNotify != NULL)
    {
        LPNMHDR pnmh = (LPNMHDR)data;
        
        pnmh->hwndFrom = infoPtr->hSelf;
        pnmh->idFrom = GetWindowLongPtr(infoPtr->hSelf,
                                        GWLP_ID);
        pnmh->code = code;
        
        Ret = SendMessage(infoPtr->hNotify,
                          WM_NOTIFY,
                          (WPARAM)pnmh->idFrom,
                          (LPARAM)pnmh);
    }
    
    return Ret;
}

static PCHECKITEM
FindCheckItemByIndex(IN PCHECKLISTWND infoPtr,
                     IN INT Index)
{
    PCHECKITEM Item, Found = NULL;
    
    if (Index >= 0)
    {
        for (Item = infoPtr->CheckItemListHead;
             Item != NULL;
             Item = Item->Next)
        {
            if (Index == 0)
            {
                Found = Item;
                break;
            }

            Index--;
        }
    }
    
    return Found;
}

static UINT
FindCheckItemIndexByAccessMask(IN PCHECKLISTWND infoPtr,
                               IN ACCESS_MASK AccessMask)
{
    PCHECKITEM Item;
    UINT Index = 0, Found = -1;

    for (Item = infoPtr->CheckItemListHead;
         Item != NULL;
         Item = Item->Next)
    {
        if (Item->AccessMask == AccessMask)
        {
            Found = Index;
            break;
        }

        Index++;
    }

    return Found;
}

static INT
CheckItemToIndex(IN PCHECKLISTWND infoPtr,
                 IN PCHECKITEM Item)
{
    PCHECKITEM CurItem;
    INT Index;
    
    for (CurItem = infoPtr->CheckItemListHead, Index = 0;
         CurItem != NULL;
         CurItem = CurItem->Next, Index++)
    {
        if (CurItem == Item)
        {
            return Index;
        }
    }
    
    return -1;
}

static PCHECKITEM
FindCheckItem(IN PCHECKLISTWND infoPtr,
              IN LPWSTR SearchText)
{
    PCHECKITEM CurItem;
    SIZE_T Count = wcslen(SearchText);
    
    for (CurItem = infoPtr->CheckItemListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        if ((CurItem->State & CIS_DISABLED) != CIS_DISABLED &&
            !wcsnicmp(CurItem->Name,
                      SearchText, Count))
        {
            break;
        }
    }
    
    return CurItem;
}

static PCHECKITEM
FindFirstEnabledCheckBox(IN PCHECKLISTWND infoPtr,
                         OUT UINT *CheckBox)
{
    PCHECKITEM CurItem;

    for (CurItem = infoPtr->CheckItemListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        if ((CurItem->State & CIS_DISABLED) != CIS_DISABLED)
        {
            /* return the Allow checkbox in case both check boxes are enabled! */
            *CheckBox = ((!(CurItem->State & CIS_ALLOWDISABLED)) ? CLB_ALLOW : CLB_DENY);
            break;
        }
    }
    
    return CurItem;
}

static PCHECKITEM
FindLastEnabledCheckBox(IN PCHECKLISTWND infoPtr,
                        OUT UINT *CheckBox)
{
    PCHECKITEM CurItem;
    PCHECKITEM LastEnabledItem = NULL;

    for (CurItem = infoPtr->CheckItemListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        if ((CurItem->State & CIS_DISABLED) != CIS_DISABLED)
        {
            LastEnabledItem = CurItem;
        }
    }
    
    if (LastEnabledItem != NULL)
    {
        /* return the Deny checkbox in case both check boxes are enabled! */
        *CheckBox = ((!(LastEnabledItem->State & CIS_DENYDISABLED)) ? CLB_DENY : CLB_ALLOW);
    }

    return LastEnabledItem;
}

static PCHECKITEM
FindPreviousEnabledCheckBox(IN PCHECKLISTWND infoPtr,
                            OUT UINT *CheckBox)
{
    PCHECKITEM Item;

    if (infoPtr->FocusedCheckItem != NULL)
    {
        Item = infoPtr->FocusedCheckItem;

        if (infoPtr->FocusedCheckItemBox == CLB_DENY &&
            !(Item->State & CIS_ALLOWDISABLED))
        {
            /* currently an Deny checkbox is focused. return the Allow checkbox
               if it's enabled */
            *CheckBox = CLB_ALLOW;
        }
        else
        {
            PCHECKITEM CurItem;

            Item = NULL;

            for (CurItem = infoPtr->CheckItemListHead;
                 CurItem != infoPtr->FocusedCheckItem;
                 CurItem = CurItem->Next)
            {
                if ((CurItem->State & CIS_DISABLED) != CIS_DISABLED)
                {
                    Item = CurItem;
                }
            }
            
            if (Item != NULL)
            {
                /* return the Deny checkbox in case both check boxes are enabled! */
                *CheckBox = ((!(Item->State & CIS_DENYDISABLED)) ? CLB_DENY : CLB_ALLOW);
            }
        }
    }
    else
    {
        Item = FindLastEnabledCheckBox(infoPtr,
                                       CheckBox);
    }

    return Item;
}

static PCHECKITEM
FindNextEnabledCheckBox(IN PCHECKLISTWND infoPtr,
                        OUT UINT *CheckBox)
{
    PCHECKITEM Item;
    
    if (infoPtr->FocusedCheckItem != NULL)
    {
        Item = infoPtr->FocusedCheckItem;
        
        if (infoPtr->FocusedCheckItemBox != CLB_DENY &&
            !(Item->State & CIS_DENYDISABLED))
        {
            /* currently an Allow checkbox is focused. return the Deny checkbox
               if it's enabled */
            *CheckBox = CLB_DENY;
        }
        else
        {
            Item = Item->Next;
            
            while (Item != NULL)
            {
                if ((Item->State & CIS_DISABLED) != CIS_DISABLED)
                {
                    /* return the Allow checkbox in case both check boxes are enabled! */
                    *CheckBox = ((!(Item->State & CIS_ALLOWDISABLED)) ? CLB_ALLOW : CLB_DENY);
                    break;
                }

                Item = Item->Next;
            }
        }
    }
    else
    {
        Item = FindFirstEnabledCheckBox(infoPtr,
                                        CheckBox);
    }
    
    return Item;
}

static PCHECKITEM
FindEnabledCheckBox(IN PCHECKLISTWND infoPtr,
                    IN BOOL ReverseSearch,
                    OUT UINT *CheckBox)
{
    PCHECKITEM Item;
    
    if (ReverseSearch)
    {
        Item = FindPreviousEnabledCheckBox(infoPtr,
                                           CheckBox);
    }
    else
    {
        Item = FindNextEnabledCheckBox(infoPtr,
                                       CheckBox);
    }
    
    return Item;
}

static PCHECKITEM
PtToCheckItemBox(IN PCHECKLISTWND infoPtr,
                 IN PPOINT ppt,
                 OUT UINT *CheckBox,
                 OUT BOOL *DirectlyInCheckBox)
{
    INT FirstVisible, Index;
    PCHECKITEM Item;
    
    FirstVisible = GetScrollPos(infoPtr->hSelf,
                                SB_VERT);
    
    Index = FirstVisible + (ppt->y / infoPtr->ItemHeight);
    
    Item = FindCheckItemByIndex(infoPtr,
                                Index);
    if (Item != NULL)
    {
        INT cx;
        
        cx = infoPtr->CheckBoxLeft[CLB_ALLOW] +
             ((infoPtr->CheckBoxLeft[CLB_DENY] - infoPtr->CheckBoxLeft[CLB_ALLOW]) / 2);

        *CheckBox = ((ppt->x <= cx) ? CLB_ALLOW : CLB_DENY);
        
        if (DirectlyInCheckBox != NULL)
        {
            INT y = ppt->y % infoPtr->ItemHeight;
            INT cxBox = infoPtr->ItemHeight - (2 * CI_TEXT_MARGIN_HEIGHT);
            
            if ((y >= CI_TEXT_MARGIN_HEIGHT &&
                 y < infoPtr->ItemHeight - CI_TEXT_MARGIN_HEIGHT) &&

                (((ppt->x >= (infoPtr->CheckBoxLeft[CLB_ALLOW] - (cxBox / 2))) &&
                  (ppt->x < (infoPtr->CheckBoxLeft[CLB_ALLOW] - (cxBox / 2) + cxBox)))
                 ||
                 ((ppt->x >= (infoPtr->CheckBoxLeft[CLB_DENY] - (cxBox / 2))) &&
                  (ppt->x < (infoPtr->CheckBoxLeft[CLB_DENY] - (cxBox / 2) + cxBox)))))
            {
                *DirectlyInCheckBox = TRUE;
            }
            else
            {
                *DirectlyInCheckBox = FALSE;
            }
        }
    }
    
    return Item;
}

static VOID
ClearCheckItems(IN PCHECKLISTWND infoPtr)
{
    PCHECKITEM CurItem, NextItem;

    CurItem = infoPtr->CheckItemListHead;
    while (CurItem != NULL)
    {
        NextItem = CurItem->Next;
        HeapFree(GetProcessHeap(),
                 0,
                 CurItem);
        CurItem = NextItem;
    }

    infoPtr->CheckItemListHead = NULL;
    infoPtr->CheckItemCount = 0;
}

static BOOL
DeleteCheckItem(IN PCHECKLISTWND infoPtr,
                IN PCHECKITEM Item)
{
    PCHECKITEM CurItem;
    PCHECKITEM *PrevPtr = &infoPtr->CheckItemListHead;
    
    for (CurItem = infoPtr->CheckItemListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        if (CurItem == Item)
        {
            if (Item == infoPtr->QuickSearchHitItem && infoPtr->QuickSearchEnabled)
            {
                EscapeQuickSearch(infoPtr);
            }
            
#if SUPPORT_UXTHEME
            if (Item == infoPtr->HoveredCheckItem)
            {
                ChangeCheckItemHotTrack(infoPtr,
                                        NULL,
                                        0);
            }
#endif
            
            if (Item == infoPtr->FocusedCheckItem)
            {
                ChangeCheckItemFocus(infoPtr,
                                     NULL,
                                     0);
            }
            
            *PrevPtr = CurItem->Next;
            HeapFree(GetProcessHeap(),
                     0,
                     CurItem);
            infoPtr->CheckItemCount--;
            return TRUE;
        }
        
        PrevPtr = &CurItem->Next;
    }
    
    return FALSE;
}

static PCHECKITEM
AddCheckItem(IN PCHECKLISTWND infoPtr,
             IN LPWSTR Name,
             IN DWORD State,
             IN ACCESS_MASK AccessMask,
             OUT INT *Index)
{
    PCHECKITEM CurItem;
    INT i;
    PCHECKITEM *PrevPtr = &infoPtr->CheckItemListHead;
    PCHECKITEM Item = HeapAlloc(GetProcessHeap(),
                                0,
                                sizeof(CHECKITEM) + (wcslen(Name) * sizeof(WCHAR)));
    if (Item != NULL)
    {
        for (CurItem = infoPtr->CheckItemListHead, i = 0;
             CurItem != NULL;
             CurItem = CurItem->Next)
        {
            PrevPtr = &CurItem->Next;
            i++;
        }
        
        Item->Next = NULL;
        Item->AccessMask = AccessMask;
        Item->State = State & CIS_MASK;
        wcscpy(Item->Name,
               Name);
        
        *PrevPtr = Item;
        infoPtr->CheckItemCount++;
        
        if (Index != NULL)
        {
            *Index = i;
        }
    }
    
    return Item;
}

static UINT
ClearCheckBoxes(IN PCHECKLISTWND infoPtr)
{
    PCHECKITEM CurItem;
    UINT nUpdated = 0;
    
    for (CurItem = infoPtr->CheckItemListHead;
         CurItem != NULL;
         CurItem = CurItem->Next)
    {
        if (CurItem->State & (CIS_ALLOW | CIS_DENY))
        {
            CurItem->State &= ~(CIS_ALLOW | CIS_DENY);
            nUpdated++;
        }
    }
    
    return nUpdated;
}

static VOID
UpdateControl(IN PCHECKLISTWND infoPtr)
{
    RECT rcClient;
    SCROLLINFO ScrollInfo;
    INT VisibleItems;
    
    GetClientRect(infoPtr->hSelf,
                  &rcClient);

    ScrollInfo.cbSize = sizeof(ScrollInfo);
    ScrollInfo.fMask = SIF_PAGE | SIF_RANGE;
    ScrollInfo.nMin = 0;
    ScrollInfo.nMax = infoPtr->CheckItemCount;
    ScrollInfo.nPage = ((rcClient.bottom - rcClient.top) + infoPtr->ItemHeight - 1) / infoPtr->ItemHeight;
    ScrollInfo.nPos = 0;
    ScrollInfo.nTrackPos = 0;
    
    VisibleItems = (rcClient.bottom - rcClient.top) / infoPtr->ItemHeight;
    
    if (ScrollInfo.nPage == (UINT)VisibleItems && ScrollInfo.nMax > 0)
    {
        ScrollInfo.nMax--;
    }

    SetScrollInfo(infoPtr->hSelf,
                  SB_VERT,
                  &ScrollInfo,
                  TRUE);

    RedrawWindow(infoPtr->hSelf,
                 NULL,
                 NULL,
                 RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN);
}

static VOID
UpdateCheckItem(IN PCHECKLISTWND infoPtr,
                IN PCHECKITEM Item)
{
    RECT rcClient;
    INT VisibleFirst, VisibleItems;
    INT Index = CheckItemToIndex(infoPtr,
                                 Item);
    if (Index != -1)
    {
        VisibleFirst = GetScrollPos(infoPtr->hSelf,
                                    SB_VERT);
        
        if (Index >= VisibleFirst)
        {
            GetClientRect(infoPtr->hSelf,
                          &rcClient);

            VisibleItems = ((rcClient.bottom - rcClient.top) + infoPtr->ItemHeight - 1) / infoPtr->ItemHeight;
            
            if (Index <= VisibleFirst + VisibleItems)
            {
                RECT rcUpdate;
                
                rcUpdate.left = rcClient.left;
                rcUpdate.right = rcClient.right;
                rcUpdate.top = (Index - VisibleFirst) * infoPtr->ItemHeight;
                rcUpdate.bottom = rcUpdate.top + infoPtr->ItemHeight;

                RedrawWindow(infoPtr->hSelf,
                             &rcUpdate,
                             NULL,
                             RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN);
            }
        }
    }
}

static VOID
MakeCheckItemVisible(IN PCHECKLISTWND infoPtr,
                     IN PCHECKITEM Item)
{
    RECT rcClient;
    INT VisibleFirst, VisibleItems, NewPos;
    INT Index = CheckItemToIndex(infoPtr,
                                 Item);
    if (Index != -1)
    {
        VisibleFirst = GetScrollPos(infoPtr->hSelf,
                                    SB_VERT);
    
        if (Index <= VisibleFirst)
        {
            NewPos = Index;
        }
        else
        {
            GetClientRect(infoPtr->hSelf,
                          &rcClient);

            VisibleItems = (rcClient.bottom - rcClient.top) / infoPtr->ItemHeight;
            if (Index - VisibleItems + 1 > VisibleFirst)
            {
                NewPos = Index - VisibleItems + 1;
            }
            else
            {
                NewPos = VisibleFirst;
            }
        }
        
        if (VisibleFirst != NewPos)
        {
            SCROLLINFO ScrollInfo;
            
            ScrollInfo.cbSize = sizeof(ScrollInfo);
            ScrollInfo.fMask = SIF_POS;
            ScrollInfo.nPos = NewPos;
            NewPos = SetScrollInfo(infoPtr->hSelf,
                                   SB_VERT,
                                   &ScrollInfo,
                                   TRUE);

            if (VisibleFirst != NewPos)
            {
                ScrollWindowEx(infoPtr->hSelf,
                               0,
                               (NewPos - VisibleFirst) * infoPtr->ItemHeight,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               SW_INVALIDATE | SW_SCROLLCHILDREN);

                RedrawWindow(infoPtr->hSelf,
                             NULL,
                             NULL,
                             RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN);
            }
        }
    }
}

static UINT
GetIdealItemHeight(IN PCHECKLISTWND infoPtr)
{
    HDC hdc = GetDC(infoPtr->hSelf);
    if(hdc != NULL)
    {
        UINT height;
        TEXTMETRIC tm;
        HGDIOBJ hOldFont = SelectObject(hdc,
                                        infoPtr->hFont);

        if(GetTextMetrics(hdc,
                          &tm))
        {
            height = tm.tmHeight;
        }
        else
        {
            height = 2;
        }

        SelectObject(hdc,
                     hOldFont);

        ReleaseDC(infoPtr->hSelf,
                  hdc);

        return height;
    }
    return 0;
}

static HFONT
RetChangeControlFont(IN PCHECKLISTWND infoPtr,
                     IN HFONT hFont,
                     IN BOOL Redraw)
{
    HFONT hOldFont = infoPtr->hFont;
    infoPtr->hFont = hFont;
    
    if (hOldFont != hFont)
    {
        infoPtr->ItemHeight = (2 * CI_TEXT_MARGIN_HEIGHT) + GetIdealItemHeight(infoPtr);
    }
    
    if (infoPtr->ShowingCaret)
    {
        DestroyCaret();
        CreateCaret(infoPtr->hSelf,
                    NULL,
                    0,
                    infoPtr->ItemHeight - (2 * CI_TEXT_MARGIN_HEIGHT));
    }
    
    UpdateControl(infoPtr);

    return hOldFont;
}

#if SUPPORT_UXTHEME
static INT
CalculateCheckBoxStyle(IN BOOL Checked,
                       IN BOOL Enabled,
                       IN BOOL HotTrack,
                       IN BOOL Pushed)
{
    INT BtnState;

    if (Checked)
    {
        BtnState = (Enabled ?
                    (Pushed ? CBS_CHECKEDPRESSED : (HotTrack ? CBS_CHECKEDHOT : CBS_CHECKEDNORMAL)) :
                    CBS_CHECKEDDISABLED);
    }
    else
    {
        BtnState = (Enabled ?
                    (Pushed ? CBS_UNCHECKEDPRESSED : (HotTrack ? CBS_UNCHECKEDHOT : CBS_UNCHECKEDNORMAL)) :
                    CBS_UNCHECKEDDISABLED);
    }

    return BtnState;
}
#endif

static VOID
PaintControl(IN PCHECKLISTWND infoPtr,
             IN HDC hDC,
             IN PRECT rcUpdate)
{
    INT ScrollPos;
    PCHECKITEM FirstItem, Item;
    RECT rcClient;
    UINT VisibleFirstIndex = rcUpdate->top / infoPtr->ItemHeight;
    UINT LastTouchedIndex = rcUpdate->bottom / infoPtr->ItemHeight;
    
    FillRect(hDC,
             rcUpdate,
             (HBRUSH)(COLOR_WINDOW + 1));
    
    GetClientRect(infoPtr->hSelf,
                  &rcClient);
    
    ScrollPos = GetScrollPos(infoPtr->hSelf,
                             SB_VERT);
    
    FirstItem = FindCheckItemByIndex(infoPtr,
                                     ScrollPos + VisibleFirstIndex);
    if (FirstItem != NULL)
    {
        RECT TextRect, ItemRect, CheckBox;
        HFONT hOldFont;
        DWORD CurrentIndex;
        COLORREF OldTextColor;
        BOOL Enabled, PrevEnabled, IsPushed;
        POINT hOldBrushOrg;
#if SUPPORT_UXTHEME
        HRESULT hDrawResult;
        BOOL ItemHovered;
#endif
        
        Enabled = IsWindowEnabled(infoPtr->hSelf);
        PrevEnabled = Enabled;
        
        ItemRect.left = 0;
        ItemRect.right = rcClient.right;
        ItemRect.top = VisibleFirstIndex * infoPtr->ItemHeight;
        
        TextRect.left = ItemRect.left + CI_TEXT_MARGIN_WIDTH;
        TextRect.right = ItemRect.right - CI_TEXT_MARGIN_WIDTH;
        TextRect.top = ItemRect.top + CI_TEXT_MARGIN_HEIGHT;
        
        SetBrushOrgEx(hDC,
                      ItemRect.left,
                      ItemRect.top,
                      &hOldBrushOrg);
        
        OldTextColor = SetTextColor(hDC,
                                    infoPtr->TextColor[Enabled]);

        hOldFont = SelectObject(hDC,
                                infoPtr->hFont);
        
        for (Item = FirstItem, CurrentIndex = VisibleFirstIndex;
             Item != NULL && CurrentIndex <= LastTouchedIndex;
             Item = Item->Next, CurrentIndex++)
        {
            TextRect.bottom = TextRect.top + infoPtr->ItemHeight - (2 * CI_TEXT_MARGIN_HEIGHT);
            ItemRect.bottom = ItemRect.top + infoPtr->ItemHeight;
            
            SetBrushOrgEx(hDC,
                          ItemRect.left,
                          ItemRect.top,
                          NULL);
            
            if (Enabled && PrevEnabled != ((Item->State & CIS_DISABLED) != CIS_DISABLED))
            {
                PrevEnabled = ((Item->State & CIS_DISABLED) != CIS_DISABLED);
                
                SetTextColor(hDC,
                             infoPtr->TextColor[PrevEnabled]);
            }

#if SUPPORT_UXTHEME
            ItemHovered = (Enabled && infoPtr->HoveredCheckItem == Item);
#endif

            if (infoPtr->QuickSearchHitItem == Item)
            {
                COLORREF OldBkColor, OldFgColor;
                SIZE TextSize;
                SIZE_T TextLen, HighlightLen = wcslen(infoPtr->QuickSearchText);

                /* highlight the quicksearch text */
                if (GetTextExtentPoint32(hDC,
                                         Item->Name,
                                         HighlightLen,
                                         &TextSize))
                {
                    COLORREF HighlightTextColor, HighlightBackground;
                    RECT rcHighlight = TextRect;
                    
                    HighlightTextColor = GetSysColor(COLOR_HIGHLIGHTTEXT);
                    HighlightBackground = GetSysColor(COLOR_HIGHLIGHT);

                    rcHighlight.right = rcHighlight.left + TextSize.cx;
                    
                    InflateRect(&rcHighlight,
                                0,
                                CI_TEXT_SELECTIONMARGIN);
                    
                    OldBkColor = SetBkColor(hDC,
                                            HighlightBackground);
                    OldFgColor = SetTextColor(hDC,
                                              HighlightTextColor);

                    /* draw the highlighted text */
                    DrawText(hDC,
                             Item->Name,
                             HighlightLen,
                             &rcHighlight,
                             DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

                    SetBkColor(hDC,
                               OldBkColor);
                    SetTextColor(hDC,
                                 OldFgColor);

                    /* draw the remaining part of the text */
                    TextLen = wcslen(Item->Name);
                    if (HighlightLen < TextLen)
                    {
                        rcHighlight.left = rcHighlight.right;
                        rcHighlight.right = TextRect.right;
                        
                        DrawText(hDC,
                                 Item->Name + HighlightLen,
                                 -1,
                                 &rcHighlight,
                                 DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
                    }
                }
            }
            else
            {
                /* draw the text */
                DrawText(hDC,
                         Item->Name,
                         -1,
                         &TextRect,
                         DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
            }
            
            CheckBox.top = TextRect.top;
            CheckBox.bottom = TextRect.bottom;

            /* draw the Allow checkbox */
            IsPushed = (Enabled && Item == infoPtr->FocusedCheckItem && infoPtr->HasFocus &&
                        !(Item->State & CIS_ALLOWDISABLED) && infoPtr->FocusedCheckItemBox != CLB_DENY &&
                        infoPtr->FocusedPushed);

            CheckBox.left = infoPtr->CheckBoxLeft[CLB_ALLOW] - ((TextRect.bottom - TextRect.top) / 2);
            CheckBox.right = CheckBox.left + (TextRect.bottom - TextRect.top);
#if SUPPORT_UXTHEME
            if (infoPtr->ThemeHandle != NULL)
            {
                INT BtnState = CalculateCheckBoxStyle(Item->State & CIS_ALLOW,
                                                      Enabled && !(Item->State & CIS_ALLOWDISABLED),
                                                      (ItemHovered && infoPtr->HoveredCheckItemBox != CLB_DENY),
                                                      IsPushed);
                

                hDrawResult = DrawThemeBackground(infoPtr->ThemeHandle,
                                                  hDC,
                                                  BP_CHECKBOX,
                                                  BtnState,
                                                  &CheckBox,
                                                  NULL);
                                                  
            }
            else
            {
                hDrawResult = E_FAIL;
            }

            /* draw the standard checkbox if no themes are enabled or drawing the
               themeed control failed */
            if (FAILED(hDrawResult))
#endif
            {
                DrawFrameControl(hDC,
                                 &CheckBox,
                                 DFC_BUTTON,
                                 DFCS_BUTTONCHECK | DFCS_FLAT |
                                 ((Item->State & CIS_ALLOWDISABLED) || !Enabled ? DFCS_INACTIVE : 0) |
                                 ((Item->State & CIS_ALLOW) ? DFCS_CHECKED : 0) |
                                 (IsPushed ? DFCS_PUSHED : 0));
            }
            if (Item == infoPtr->FocusedCheckItem && !(infoPtr->UIState & UISF_HIDEFOCUS) &&
                infoPtr->HasFocus &&
                infoPtr->FocusedCheckItemBox != CLB_DENY)
            {
                RECT rcFocus = CheckBox;
                
                InflateRect (&rcFocus,
                             CI_TEXT_MARGIN_HEIGHT,
                             CI_TEXT_MARGIN_HEIGHT);

                DrawFocusRect(hDC,
                              &rcFocus);
            }

            /* draw the Deny checkbox */
            IsPushed = (Enabled && Item == infoPtr->FocusedCheckItem && infoPtr->HasFocus &&
                        !(Item->State & CIS_DENYDISABLED) && infoPtr->FocusedCheckItemBox == CLB_DENY &&
                        infoPtr->FocusedPushed);

            CheckBox.left = infoPtr->CheckBoxLeft[CLB_DENY] - ((TextRect.bottom - TextRect.top) / 2);
            CheckBox.right = CheckBox.left + (TextRect.bottom - TextRect.top);
#if SUPPORT_UXTHEME
            if (infoPtr->ThemeHandle != NULL)
            {
                INT BtnState = CalculateCheckBoxStyle(Item->State & CIS_DENY,
                                                      Enabled && !(Item->State & CIS_DENYDISABLED),
                                                      (ItemHovered && infoPtr->HoveredCheckItemBox == CLB_DENY),
                                                      IsPushed);

                hDrawResult = DrawThemeBackground(infoPtr->ThemeHandle,
                                                  hDC,
                                                  BP_CHECKBOX,
                                                  BtnState,
                                                  &CheckBox,
                                                  NULL);

            }
            else
            {
                hDrawResult = E_FAIL;
            }

            /* draw the standard checkbox if no themes are enabled or drawing the
               themeed control failed */
            if (FAILED(hDrawResult))
#endif
            {
                DrawFrameControl(hDC,
                                 &CheckBox,
                                 DFC_BUTTON,
                                 DFCS_BUTTONCHECK | DFCS_FLAT |
                                 ((Item->State & CIS_DENYDISABLED) || !Enabled ? DFCS_INACTIVE : 0) |
                                 ((Item->State & CIS_DENY) ? DFCS_CHECKED : 0) |
                                 (IsPushed ? DFCS_PUSHED : 0));
            }
            if (infoPtr->HasFocus && !(infoPtr->UIState & UISF_HIDEFOCUS) &&
                Item == infoPtr->FocusedCheckItem &&
                infoPtr->FocusedCheckItemBox == CLB_DENY)
            {
                RECT rcFocus = CheckBox;

                InflateRect (&rcFocus,
                             CI_TEXT_MARGIN_HEIGHT,
                             CI_TEXT_MARGIN_HEIGHT);

                DrawFocusRect(hDC,
                              &rcFocus);
            }

            TextRect.top += infoPtr->ItemHeight;
            ItemRect.top += infoPtr->ItemHeight;
        }

        SelectObject(hDC,
                     hOldFont);

        SetTextColor(hDC,
                     OldTextColor);

        SetBrushOrgEx(hDC,
                      hOldBrushOrg.x,
                      hOldBrushOrg.y,
                      NULL);
    }
}

static VOID
ChangeCheckItemFocus(IN PCHECKLISTWND infoPtr,
                     IN PCHECKITEM NewFocus,
                     IN UINT NewFocusBox)
{
    if (NewFocus != infoPtr->FocusedCheckItem)
    {
        PCHECKITEM OldFocus = infoPtr->FocusedCheckItem;
        infoPtr->FocusedCheckItem = NewFocus;
        infoPtr->FocusedCheckItemBox = NewFocusBox;

        if (OldFocus != NULL)
        {
            UpdateCheckItem(infoPtr,
                            OldFocus);
        }
    }
    else
    {
        infoPtr->FocusedCheckItemBox = NewFocusBox;
    }

    if (NewFocus != NULL)
    {
        MakeCheckItemVisible(infoPtr,
                             NewFocus);
        UpdateCheckItem(infoPtr,
                        NewFocus);
    }
}

static VOID
UpdateCheckItemBox(IN PCHECKLISTWND infoPtr,
                   IN PCHECKITEM Item,
                   IN UINT ItemBox)
{
    RECT rcClient;
    INT VisibleFirst, VisibleItems;
    INT Index = CheckItemToIndex(infoPtr,
                                 Item);
    if (Index != -1)
    {
        VisibleFirst = GetScrollPos(infoPtr->hSelf,
                                    SB_VERT);

        if (Index >= VisibleFirst)
        {
            GetClientRect(infoPtr->hSelf,
                          &rcClient);

            VisibleItems = ((rcClient.bottom - rcClient.top) + infoPtr->ItemHeight - 1) / infoPtr->ItemHeight;

            if (Index <= VisibleFirst + VisibleItems)
            {
                RECT rcUpdate;

                rcUpdate.left = rcClient.left + infoPtr->CheckBoxLeft[ItemBox] - (infoPtr->ItemHeight / 2);
                rcUpdate.right = rcUpdate.left + infoPtr->ItemHeight;
                rcUpdate.top = ((Index - VisibleFirst) * infoPtr->ItemHeight);
                rcUpdate.bottom = rcUpdate.top + infoPtr->ItemHeight;

                RedrawWindow(infoPtr->hSelf,
                             &rcUpdate,
                             NULL,
                             RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN);
            }
        }
    }
}

#if SUPPORT_UXTHEME
static VOID
ChangeCheckItemHotTrack(IN PCHECKLISTWND infoPtr,
                        IN PCHECKITEM NewHotTrack,
                        IN UINT NewHotTrackBox)
{
    if (NewHotTrack != infoPtr->HoveredCheckItem)
    {
        PCHECKITEM OldHotTrack = infoPtr->HoveredCheckItem;
        UINT OldHotTrackBox = infoPtr->HoveredCheckItemBox;
        
        infoPtr->HoveredCheckItem = NewHotTrack;
        infoPtr->HoveredCheckItemBox = NewHotTrackBox;

        if (OldHotTrack != NULL)
        {
            UpdateCheckItemBox(infoPtr,
                               OldHotTrack,
                               OldHotTrackBox);
        }
    }
    else
    {
        infoPtr->HoveredCheckItemBox = NewHotTrackBox;
    }

    if (NewHotTrack != NULL)
    {
        UpdateCheckItemBox(infoPtr,
                           NewHotTrack,
                           NewHotTrackBox);
    }
}
#endif

static BOOL
ChangeCheckBox(IN PCHECKLISTWND infoPtr,
               IN PCHECKITEM CheckItem,
               IN UINT CheckItemBox)
{
    NMCHANGEITEMCHECKBOX CheckData;
    DWORD OldState = CheckItem->State;
    DWORD CheckedBit = ((infoPtr->FocusedCheckItemBox == CLB_DENY) ? CIS_DENY : CIS_ALLOW);
    BOOL Checked = (CheckItem->State & CheckedBit) != 0;

    CheckData.OldState = OldState;
    CheckData.NewState = (Checked ? OldState & ~CheckedBit : OldState | CheckedBit);
    CheckData.CheckBox = infoPtr->FocusedCheckItemBox;
    CheckData.Checked = !Checked;

    if (NotifyControlParent(infoPtr,
                            CLN_CHANGINGITEMCHECKBOX,
                            &CheckData) != (LRESULT)-1)
    {
        CheckItem->State = CheckData.NewState;
    }
    
    return (CheckItem->State != OldState);
}

static VOID
DisplayCaret(IN PCHECKLISTWND infoPtr)
{
    if (IsWindowEnabled(infoPtr->hSelf) && !infoPtr->ShowingCaret)
    {
        infoPtr->ShowingCaret = TRUE;
        
        CreateCaret(infoPtr->hSelf,
                    NULL,
                    infoPtr->CaretWidth,
                    infoPtr->ItemHeight - (2 * CI_TEXT_MARGIN_HEIGHT));

        ShowCaret(infoPtr->hSelf);
    }
}

static VOID
RemoveCaret(IN PCHECKLISTWND infoPtr)
{
    if (IsWindowEnabled(infoPtr->hSelf) && infoPtr->ShowingCaret)
    {
        infoPtr->ShowingCaret = FALSE;
        
        HideCaret(infoPtr->hSelf);
        DestroyCaret();
    }
}

static VOID
KillQuickSearchTimers(IN PCHECKLISTWND infoPtr)
{
    KillTimer(infoPtr->hSelf,
              TIMER_ID_SETHITFOCUS);
    KillTimer(infoPtr->hSelf,
              TIMER_ID_RESETQUICKSEARCH);
}

static VOID
MapItemToRect(IN PCHECKLISTWND infoPtr,
              IN PCHECKITEM CheckItem,
              OUT RECT *prcItem)
{
    INT Index = CheckItemToIndex(infoPtr,
                                 CheckItem);
    if (Index != -1)
    {
        RECT rcClient;
        INT VisibleFirst;
        
        GetClientRect(infoPtr->hSelf,
                      &rcClient);
        
        VisibleFirst = GetScrollPos(infoPtr->hSelf,
                                    SB_VERT);
        
        prcItem->left = rcClient.left;
        prcItem->right = rcClient.right;
        prcItem->top = (Index - VisibleFirst) * infoPtr->ItemHeight;
        prcItem->bottom = prcItem->top + infoPtr->ItemHeight;
    }
    else
    {
        prcItem->left = 0;
        prcItem->top = 0;
        prcItem->right = 0;
        prcItem->bottom = 0;
    }
}

static VOID
UpdateCaretPos(IN PCHECKLISTWND infoPtr)
{
    if (infoPtr->ShowingCaret && infoPtr->QuickSearchHitItem != NULL)
    {
        HDC hDC = GetDC(infoPtr->hSelf);
        if (hDC != NULL)
        {
            SIZE TextSize;
            HGDIOBJ hOldFont = SelectObject(hDC,
                                            infoPtr->hFont);

            TextSize.cx = 0;
            TextSize.cy = 0;
            
            if (infoPtr->QuickSearchText[0] == L'\0' ||
                GetTextExtentPoint32(hDC,
                                     infoPtr->QuickSearchHitItem->Name,
                                     wcslen(infoPtr->QuickSearchText),
                                     &TextSize))
            {
                RECT rcItem;
                
                MapItemToRect(infoPtr,
                              infoPtr->QuickSearchHitItem,
                              &rcItem);

                /* actually change the caret position */
                SetCaretPos(rcItem.left + CI_TEXT_MARGIN_WIDTH + TextSize.cx,
                            rcItem.top + CI_TEXT_MARGIN_HEIGHT);
            }

            SelectObject(hDC,
                         hOldFont);

            ReleaseDC(infoPtr->hSelf,
                      hDC);
        }
    }
}

static VOID
EscapeQuickSearch(IN PCHECKLISTWND infoPtr)
{
    if (infoPtr->QuickSearchEnabled && infoPtr->QuickSearchHitItem != NULL)
    {
        PCHECKITEM OldHit = infoPtr->QuickSearchHitItem;
        
        infoPtr->QuickSearchHitItem = NULL;
        infoPtr->QuickSearchText[0] = L'\0';
        
        /* scroll back to the focused item */
        if (infoPtr->FocusedCheckItem != NULL)
        {
            MakeCheckItemVisible(infoPtr,
                                 infoPtr->FocusedCheckItem);
        }
        
        /* repaint the old search hit item if it's still visible */
        UpdateCheckItem(infoPtr,
                        OldHit);

        KillQuickSearchTimers(infoPtr);
        
        RemoveCaret(infoPtr);
    }
}

static VOID
ChangeSearchHit(IN PCHECKLISTWND infoPtr,
                IN PCHECKITEM NewHit)
{
    PCHECKITEM OldHit = infoPtr->QuickSearchHitItem;
    
    infoPtr->QuickSearchHitItem = NewHit;
    
    if (OldHit != NewHit)
    {
        /* scroll to the new search hit */
        MakeCheckItemVisible(infoPtr,
                             NewHit);
        
        /* repaint the old hit if present and visible */
        if (OldHit != NULL)
        {
            UpdateCheckItem(infoPtr,
                            OldHit);
        }
        else
        {
            /* show the caret the first time we find an item */
             DisplayCaret(infoPtr);
        }
    }
    
    UpdateCaretPos(infoPtr);
    
    UpdateCheckItem(infoPtr,
                    NewHit);
    
    /* kill the reset timer and restart the set hit focus timer */
    KillTimer(infoPtr->hSelf,
              TIMER_ID_RESETQUICKSEARCH);
    if (infoPtr->QuickSearchSetFocusDelay != 0)
    {
        SetTimer(infoPtr->hSelf,
                 TIMER_ID_SETHITFOCUS,
                 infoPtr->QuickSearchSetFocusDelay,
                 NULL);
    }
}

static BOOL
QuickSearchFindHit(IN PCHECKLISTWND infoPtr,
                   IN WCHAR c)
{
    if (infoPtr->QuickSearchEnabled)
    {
        BOOL Ret = FALSE;
        PCHECKITEM NewHit;
        
        switch (c)
        {
            case '\r':
            case '\n':
            {
                Ret = infoPtr->QuickSearchHitItem != NULL;
                if (Ret)
                {
                    /* NOTE: QuickSearchHitItem definitely has at least one
                             enabled check box, the user can't search for disabled
                             check items */

                    ChangeCheckItemFocus(infoPtr,
                                         infoPtr->QuickSearchHitItem,
                                         ((!(infoPtr->QuickSearchHitItem->State & CIS_ALLOWDISABLED)) ? CLB_ALLOW : CLB_DENY));

                    EscapeQuickSearch(infoPtr);
                }
                break;
            }
            
            case VK_BACK:
            {
                if (infoPtr->QuickSearchHitItem != NULL)
                {
                    INT SearchLen = wcslen(infoPtr->QuickSearchText);
                    if (SearchLen > 0)
                    {
                        /* delete the last character */
                        infoPtr->QuickSearchText[--SearchLen] = L'\0';
                        
                        if (SearchLen > 0)
                        {
                            /* search again */
                            NewHit = FindCheckItem(infoPtr,
                                                   infoPtr->QuickSearchText);

                            if (NewHit != NULL)
                            {
                                /* change the search hit */
                                ChangeSearchHit(infoPtr,
                                                NewHit);

                                Ret = TRUE;
                            }
                        }
                    }

                    if (!Ret)
                    {
                        EscapeQuickSearch(infoPtr);
                    }
                }
                break;
            }
            
            default:
            {
                INT SearchLen = wcslen(infoPtr->QuickSearchText);
                if (SearchLen < (INT)(sizeof(infoPtr->QuickSearchText) / sizeof(infoPtr->QuickSearchText[0])) - 1)
                {
                    infoPtr->QuickSearchText[SearchLen++] = c;
                    infoPtr->QuickSearchText[SearchLen] = L'\0';
                    
                    NewHit = FindCheckItem(infoPtr,
                                           infoPtr->QuickSearchText);
                    if (NewHit != NULL)
                    {
                        /* change the search hit */
                        ChangeSearchHit(infoPtr,
                                        NewHit);
                        
                        Ret = TRUE;
                    }
                    else
                    {
                        /* reset the input */
                        infoPtr->QuickSearchText[--SearchLen] = L'\0';
                    }
                }
                break;
            }
        }
        return Ret;
    }
    
    return FALSE;
}

static LRESULT CALLBACK
CheckListWndProc(IN HWND hwnd,
                 IN UINT uMsg,
                 IN WPARAM wParam,
                 IN LPARAM lParam)
{
    PCHECKLISTWND infoPtr;
    LRESULT Ret;
    
    infoPtr = (PCHECKLISTWND)GetWindowLongPtr(hwnd,
                                              0);
    
    if (infoPtr == NULL && uMsg != WM_CREATE)
    {
        goto HandleDefaultMessage;
    }
    
    Ret = 0;
    
    switch (uMsg)
    {
        case WM_PAINT:
        {
            HDC hdc;
            RECT rcUpdate;
            PAINTSTRUCT ps;
            
            if (GetUpdateRect(hwnd,
                              &rcUpdate,
                              FALSE))
            {
                hdc = (wParam != 0 ? (HDC)wParam : BeginPaint(hwnd, &ps));

                if (hdc != NULL)
                {
                    PaintControl(infoPtr,
                                 hdc,
                                 &rcUpdate);

                    if (wParam == 0)
                    {
                        EndPaint(hwnd,
                                 &ps);
                    }
                }
            }
            break;
        }
        
        case WM_MOUSEMOVE:
        {
            POINT pt;
            BOOL InCheckBox;
            HWND hWndCapture = GetCapture();
            
            pt.x = (LONG)LOWORD(lParam);
            pt.y = (LONG)HIWORD(lParam);
            
#if SUPPORT_UXTHEME
            /* handle hovering checkboxes */
            if (hWndCapture == NULL && infoPtr->ThemeHandle != NULL)
            {
                TRACKMOUSEEVENT tme;
                PCHECKITEM HotTrackItem;
                UINT HotTrackItemBox;

                HotTrackItem = PtToCheckItemBox(infoPtr,
                                                &pt,
                                                &HotTrackItemBox,
                                                &InCheckBox);
                if (HotTrackItem != NULL && InCheckBox)
                {
                    if (infoPtr->HoveredCheckItem != HotTrackItem ||
                        infoPtr->HoveredCheckItemBox != HotTrackItemBox)
                    {
                        ChangeCheckItemHotTrack(infoPtr,
                                                HotTrackItem,
                                                HotTrackItemBox);
                    }
                }
                else
                {
                    ChangeCheckItemHotTrack(infoPtr,
                                            NULL,
                                            0);
                }
                
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                tme.dwHoverTime = infoPtr->HoverTime;
                
                TrackMouseEvent(&tme);
            }
#endif

            if (hWndCapture == hwnd && infoPtr->FocusedCheckItem != NULL)
            {
                PCHECKITEM PtItem;
                UINT PtItemBox;
                BOOL OldPushed;
                
                PtItem = PtToCheckItemBox(infoPtr,
                                          &pt,
                                          &PtItemBox,
                                          &InCheckBox);

                OldPushed = infoPtr->FocusedPushed;
                infoPtr->FocusedPushed = InCheckBox && infoPtr->FocusedCheckItem == PtItem &&
                                         infoPtr->FocusedCheckItemBox == PtItemBox;

                if (OldPushed != infoPtr->FocusedPushed)
                {
                    UpdateCheckItemBox(infoPtr,
                                       infoPtr->FocusedCheckItem,
                                       infoPtr->FocusedCheckItemBox);
                }
            }
            
            break;
        }
        
        case WM_VSCROLL:
        {
            SCROLLINFO ScrollInfo;
            
            ScrollInfo.cbSize = sizeof(ScrollInfo);
            ScrollInfo.fMask = SIF_RANGE | SIF_POS;

            if (GetScrollInfo(hwnd,
                              SB_VERT,
                              &ScrollInfo))
            {
                INT OldPos = ScrollInfo.nPos;
                
                switch (LOWORD(wParam))
                {
                    case SB_BOTTOM:
                        ScrollInfo.nPos = ScrollInfo.nMax;
                        break;

                    case SB_LINEDOWN:
                        if (ScrollInfo.nPos < ScrollInfo.nMax)
                        {
                            ScrollInfo.nPos++;
                        }
                        break;

                    case SB_LINEUP:
                        if (ScrollInfo.nPos > 0)
                        {
                            ScrollInfo.nPos--;
                        }
                        break;

                    case SB_PAGEDOWN:
                    {
                        RECT rcClient;
                        INT ScrollLines;
                        
                        /* don't use ScrollInfo.nPage because we should only scroll
                           down by the number of completely visible list entries.
                           nPage however also includes the partly cropped list
                           item at the bottom of the control */

                        GetClientRect(hwnd,
                                      &rcClient);

                        ScrollLines = max(1,
                                          (rcClient.bottom - rcClient.top) / infoPtr->ItemHeight);
                        
                        if (ScrollInfo.nPos + ScrollLines <= ScrollInfo.nMax)
                        {
                            ScrollInfo.nPos += ScrollLines;
                        }
                        else
                        {
                            ScrollInfo.nPos = ScrollInfo.nMax;
                        }
                        break;
                    }

                    case SB_PAGEUP:
                    {
                        RECT rcClient;
                        INT ScrollLines;

                        /* don't use ScrollInfo.nPage because we should only scroll
                           down by the number of completely visible list entries.
                           nPage however also includes the partly cropped list
                           item at the bottom of the control */

                        GetClientRect(hwnd,
                                      &rcClient);

                        ScrollLines = max(1,
                                          (rcClient.bottom - rcClient.top) / infoPtr->ItemHeight);
                        
                        if (ScrollInfo.nPos >= ScrollLines)
                        {
                            ScrollInfo.nPos -= ScrollLines;
                        }
                        else
                        {
                            ScrollInfo.nPos = 0;
                        }
                        break;
                    }

                    case SB_THUMBPOSITION:
                    case SB_THUMBTRACK:
                    {
                        ScrollInfo.nPos = HIWORD(wParam);
                        break;
                    }

                    case SB_TOP:
                        ScrollInfo.nPos = 0;
                        break;
                }
                
                if (OldPos != ScrollInfo.nPos)
                {
                    ScrollInfo.fMask = SIF_POS;
                    
                    ScrollInfo.nPos = SetScrollInfo(hwnd,
                                                    SB_VERT,
                                                    &ScrollInfo,
                                                    TRUE);
                    
                    if (OldPos != ScrollInfo.nPos)
                    {
                        ScrollWindowEx(hwnd,
                                       0,
                                       (OldPos - ScrollInfo.nPos) * infoPtr->ItemHeight,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       SW_INVALIDATE | SW_SCROLLCHILDREN);

                        RedrawWindow(hwnd,
                                     NULL,
                                     NULL,
                                     RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN);
                    }
                }
            }
            break;
        }
        
        case CLM_ADDITEM:
        {
            INT Index = -1;
            PCHECKITEM Item = AddCheckItem(infoPtr,
                                           (LPWSTR)lParam,
                                           CIS_NONE,
                                           (ACCESS_MASK)wParam,
                                           &Index);
            if (Item != NULL)
            {
                UpdateControl(infoPtr);
                Ret = (LRESULT)Index;
            }
            else
            {
                Ret = (LRESULT)-1;
            }
            break;
        }
        
        case CLM_DELITEM:
        {
            PCHECKITEM Item = FindCheckItemByIndex(infoPtr,
                                                   wParam);
            if (Item != NULL)
            {
                Ret = DeleteCheckItem(infoPtr,
                                      Item);
                if (Ret)
                {
                    UpdateControl(infoPtr);
                }
            }
            else
            {
                Ret = FALSE;
            }
            break;
        }
        
        case CLM_SETITEMSTATE:
        {
            PCHECKITEM Item = FindCheckItemByIndex(infoPtr,
                                                   wParam);
            if (Item != NULL)
            {
                DWORD OldState = Item->State;
                Item->State = (DWORD)lParam & CIS_MASK;
                
                if (Item->State != OldState)
                {
                    /* revert the focus if the currently focused item is about
                       to be disabled */
                    if (Item == infoPtr->FocusedCheckItem &&
                        (Item->State & CIS_DISABLED))
                    {
                        if (infoPtr->FocusedCheckItemBox == CLB_DENY)
                        {
                            if (Item->State & CIS_DENYDISABLED)
                            {
                                infoPtr->FocusedCheckItem = NULL;
                            }
                        }
                        else
                        {
                            if (Item->State & CIS_ALLOWDISABLED)
                            {
                                infoPtr->FocusedCheckItem = NULL;
                            }
                        }
                    }

                    UpdateControl(infoPtr);
                }
                Ret = TRUE;
            }
            break;
        }
        
        case CLM_GETITEMCOUNT:
        {
            Ret = infoPtr->CheckItemCount;
            break;
        }
        
        case CLM_CLEAR:
        {
            ClearCheckItems(infoPtr);
            UpdateControl(infoPtr);
            break;
        }
        
        case CLM_SETCHECKBOXCOLUMN:
        {
            infoPtr->CheckBoxLeft[wParam != CLB_DENY] = (INT)lParam;
            UpdateControl(infoPtr);
            Ret = 1;
            break;
        }
        
        case CLM_GETCHECKBOXCOLUMN:
        {
            Ret = (LRESULT)infoPtr->CheckBoxLeft[wParam != CLB_DENY];
            break;
        }
        
        case CLM_CLEARCHECKBOXES:
        {
            Ret = (LRESULT)ClearCheckBoxes(infoPtr);
            if (Ret)
            {
                UpdateControl(infoPtr);
            }
            break;
        }
        
        case CLM_ENABLEQUICKSEARCH:
        {
            if (wParam == 0)
            {
                EscapeQuickSearch(infoPtr);
            }
            infoPtr->QuickSearchEnabled = (wParam != 0);
            break;
        }
        
        case CLM_SETQUICKSEARCH_TIMEOUT_RESET:
        {
            infoPtr->QuickSearchResetDelay = (UINT)wParam;
            break;
        }
        
        case CLM_SETQUICKSEARCH_TIMEOUT_SETFOCUS:
        {
            infoPtr->QuickSearchSetFocusDelay = (UINT)wParam;
            break;
        }
        
        case CLM_FINDITEMBYACCESSMASK:
        {
            Ret = (LRESULT)FindCheckItemIndexByAccessMask(infoPtr,
                                                          (ACCESS_MASK)wParam);
            break;
        }
        
        case WM_SETFONT:
        {
            Ret = (LRESULT)RetChangeControlFont(infoPtr,
                                                (HFONT)wParam,
                                                (BOOL)LOWORD(lParam));
            break;
        }
        
        case WM_GETFONT:
        {
            Ret = (LRESULT)infoPtr->hFont;
            break;
        }
        
        case WM_STYLECHANGED:
        {
            if (wParam == (WPARAM)GWL_STYLE)
            {
                UpdateControl(infoPtr);
            }
            break;
        }
        
        case WM_ENABLE:
        {
            EscapeQuickSearch(infoPtr);
            
            UpdateControl(infoPtr);
            break;
        }
        
        case WM_MOUSEWHEEL:
        {
            SHORT ScrollDelta;
            UINT ScrollLines = 3;
            
            SystemParametersInfo(SPI_GETWHEELSCROLLLINES,
                                 0,
                                 &ScrollLines,
                                 0);
            ScrollDelta = 0 - (SHORT)HIWORD(wParam);
            
            if (ScrollLines != 0 &&
                abs(ScrollDelta) >= WHEEL_DELTA)
            {
                SCROLLINFO ScrollInfo;

                ScrollInfo.cbSize = sizeof(ScrollInfo);
                ScrollInfo.fMask = SIF_RANGE | SIF_POS;
                
                if (GetScrollInfo(hwnd,
                                  SB_VERT,
                                  &ScrollInfo))
                {
                    INT OldPos = ScrollInfo.nPos;
                    
                    ScrollInfo.nPos += (ScrollDelta / WHEEL_DELTA) * ScrollLines;
                    if (ScrollInfo.nPos < 0)
                        ScrollInfo.nPos = 0;
                    else if (ScrollInfo.nPos > ScrollInfo.nMax)
                        ScrollInfo.nPos = ScrollInfo.nMax;

                    if (OldPos != ScrollInfo.nPos)
                    {
                        ScrollInfo.fMask = SIF_POS;
                        
                        ScrollInfo.nPos = SetScrollInfo(hwnd,
                                                        SB_VERT,
                                                        &ScrollInfo,
                                                        TRUE);
                        
                        if (OldPos != ScrollInfo.nPos)
                        {
                            ScrollWindowEx(hwnd,
                                           0,
                                           (OldPos - ScrollInfo.nPos) * infoPtr->ItemHeight,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL,
                                           SW_INVALIDATE | SW_SCROLLCHILDREN);

                            RedrawWindow(hwnd,
                                         NULL,
                                         NULL,
                                         RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN);
                        }
                    }
                }
            }
            break;
        }
        
        case WM_SETFOCUS:
        {
            infoPtr->HasFocus = TRUE;

            if (infoPtr->FocusedCheckItem == NULL)
            {
                BOOL Shift = GetKeyState(VK_SHIFT) & 0x8000;
                infoPtr->FocusedCheckItem = FindEnabledCheckBox(infoPtr,
                                                                Shift,
                                                                &infoPtr->FocusedCheckItemBox);
            }
            if (infoPtr->FocusedCheckItem != NULL)
            {
                MakeCheckItemVisible(infoPtr,
                                     infoPtr->FocusedCheckItem);

                UpdateCheckItem(infoPtr,
                                infoPtr->FocusedCheckItem);
            }
            break;
        }
        
        case WM_KILLFOCUS:
        {
            EscapeQuickSearch(infoPtr);
            
            infoPtr->HasFocus = FALSE;
            if (infoPtr->FocusedCheckItem != NULL)
            {
                infoPtr->FocusedPushed = FALSE;
                
                UpdateCheckItem(infoPtr,
                                infoPtr->FocusedCheckItem);
            }
            break;
        }
        
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        {
            if (IsWindowEnabled(hwnd))
            {
                PCHECKITEM NewFocus;
                UINT NewFocusBox = 0;
                BOOL InCheckBox;
                POINT pt;
                BOOL ChangeFocus, Capture = FALSE;
                
                pt.x = (LONG)LOWORD(lParam);
                pt.y = (LONG)HIWORD(lParam);
                
                NewFocus = PtToCheckItemBox(infoPtr,
                                            &pt,
                                            &NewFocusBox,
                                            &InCheckBox);
                if (NewFocus != NULL)
                {
                    if (NewFocus->State & ((NewFocusBox != CLB_DENY) ? CIS_ALLOWDISABLED : CIS_DENYDISABLED))
                    {
                        /* the user clicked on a disabled checkbox, try to set
                           the focus to the other one or not change it at all */

                        InCheckBox = FALSE;
                        
                        ChangeFocus = ((NewFocus->State & CIS_DISABLED) != CIS_DISABLED);
                        if (ChangeFocus)
                        {
                            NewFocusBox = ((NewFocusBox != CLB_DENY) ? CLB_DENY : CLB_ALLOW);
                        }
                    }
                    else
                    {
                        ChangeFocus = TRUE;
                    }
                    
                    if (InCheckBox && ChangeFocus && GetCapture() == NULL &&
                        (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK))
                    {
                        infoPtr->FocusedPushed = TRUE;
                        Capture = TRUE;
                    }
                }
                else
                {
                    ChangeFocus = TRUE;
                }
                
                if (ChangeFocus)
                {
                    if (infoPtr->QuickSearchEnabled && infoPtr->QuickSearchHitItem != NULL &&
                        infoPtr->QuickSearchHitItem != NewFocus)
                    {
                        EscapeQuickSearch(infoPtr);
                    }
                    
                    ChangeCheckItemFocus(infoPtr,
                                         NewFocus,
                                         NewFocusBox);
                }
                
                if (!infoPtr->HasFocus)
                {
                    SetFocus(hwnd);
                }
                
                if (Capture)
                {
                    SetCapture(hwnd);
                }
            }
            break;
        }
        
        case WM_LBUTTONUP:
        {
            if (GetCapture() == hwnd)
            {
                if (infoPtr->FocusedCheckItem != NULL && infoPtr->FocusedPushed)
                {
                    PCHECKITEM PtItem;
                    UINT PtItemBox;
                    BOOL InCheckBox;
                    POINT pt;
                    
                    pt.x = (LONG)LOWORD(lParam);
                    pt.y = (LONG)HIWORD(lParam);
                    
                    infoPtr->FocusedPushed = FALSE;

                    PtItem = PtToCheckItemBox(infoPtr,
                                              &pt,
                                              &PtItemBox,
                                              &InCheckBox);

                    if (PtItem == infoPtr->FocusedCheckItem && InCheckBox &&
                        PtItemBox == infoPtr->FocusedCheckItemBox)
                    {
                        UINT OtherBox = ((PtItemBox == CLB_ALLOW) ? CLB_DENY : CLB_ALLOW);
                        DWORD OtherStateMask = ((OtherBox == CLB_ALLOW) ?
                                                (CIS_ALLOW | CIS_ALLOWDISABLED) :
                                                (CIS_DENY | CIS_DENYDISABLED));
                        DWORD OtherStateOld = PtItem->State & OtherStateMask;
                        if (ChangeCheckBox(infoPtr,
                                           PtItem,
                                           PtItemBox) &&
                            ((PtItem->State & OtherStateMask) != OtherStateOld))
                        {
                            UpdateCheckItemBox(infoPtr,
                                               infoPtr->FocusedCheckItem,
                                               OtherBox);
                        }
                    }
                    
                    UpdateCheckItemBox(infoPtr,
                                       infoPtr->FocusedCheckItem,
                                       infoPtr->FocusedCheckItemBox);
                }
                
                ReleaseCapture();
            }
            break;
        }
        
        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_SPACE:
                {
                    if (GetCapture() == NULL &&
                        !QuickSearchFindHit(infoPtr,
                                            L' '))
                    {
                        if (infoPtr->FocusedCheckItem != NULL &&
                            (infoPtr->QuickSearchHitItem == NULL ||
                             infoPtr->QuickSearchHitItem == infoPtr->FocusedCheckItem))
                        {
                            BOOL OldPushed = infoPtr->FocusedPushed;
                            infoPtr->FocusedPushed = TRUE;

                            if (infoPtr->FocusedPushed != OldPushed)
                            {
                                MakeCheckItemVisible(infoPtr,
                                                     infoPtr->FocusedCheckItem);

                                UpdateCheckItemBox(infoPtr,
                                                   infoPtr->FocusedCheckItem,
                                                   infoPtr->FocusedCheckItemBox);
                            }
                        }
                    }
                    break;
                }
                
                case VK_RETURN:
                {
                    if (GetCapture() == NULL &&
                        !QuickSearchFindHit(infoPtr,
                                            L'\n'))
                    {
                        if (infoPtr->FocusedCheckItem != NULL &&
                            infoPtr->QuickSearchHitItem == NULL)
                        {
                            UINT OtherBox;
                            DWORD OtherStateMask;
                            DWORD OtherStateOld;

                            MakeCheckItemVisible(infoPtr,
                                                 infoPtr->FocusedCheckItem);

                            OtherBox = ((infoPtr->FocusedCheckItemBox == CLB_ALLOW) ? CLB_DENY : CLB_ALLOW);
                            OtherStateMask = ((OtherBox == CLB_ALLOW) ?
                                              (CIS_ALLOW | CIS_ALLOWDISABLED) :
                                              (CIS_DENY | CIS_DENYDISABLED));
                            OtherStateOld = infoPtr->FocusedCheckItem->State & OtherStateMask;
                            if (ChangeCheckBox(infoPtr,
                                               infoPtr->FocusedCheckItem,
                                               infoPtr->FocusedCheckItemBox))
                            {
                                UpdateCheckItemBox(infoPtr,
                                                   infoPtr->FocusedCheckItem,
                                                   infoPtr->FocusedCheckItemBox);
                                if ((infoPtr->FocusedCheckItem->State & OtherStateMask) != OtherStateOld)
                                {
                                    UpdateCheckItemBox(infoPtr,
                                                       infoPtr->FocusedCheckItem,
                                                       OtherBox);
                                }
                            }
                        }
                    }
                    break;
                }
                
                case VK_TAB:
                {
                    if (GetCapture() == NULL)
                    {
                        PCHECKITEM NewFocus;
                        UINT NewFocusBox = 0;
                        BOOL Shift = GetKeyState(VK_SHIFT) & 0x8000;
                        
                        EscapeQuickSearch(infoPtr);

                        NewFocus = FindEnabledCheckBox(infoPtr,
                                                       Shift,
                                                       &NewFocusBox);

                        /* update the UI status */
                        SendMessage(GetAncestor(hwnd,
                                                GA_PARENT),
                                    WM_CHANGEUISTATE,
                                    MAKEWPARAM(UIS_INITIALIZE,
                                               0),
                                    0);

                        ChangeCheckItemFocus(infoPtr,
                                             NewFocus,
                                             NewFocusBox);
                    }
                    break;
                }
                
                default:
                {
                    goto HandleDefaultMessage;
                }
            }
            break;
        }
        
        case WM_KEYUP:
        {
            if (wParam == VK_SPACE && IsWindowEnabled(hwnd) &&
                infoPtr->FocusedCheckItem != NULL &&
                infoPtr->FocusedPushed)
            {
                UINT OtherBox = ((infoPtr->FocusedCheckItemBox == CLB_ALLOW) ? CLB_DENY : CLB_ALLOW);
                DWORD OtherStateMask = ((OtherBox == CLB_ALLOW) ?
                                        (CIS_ALLOW | CIS_ALLOWDISABLED) :
                                        (CIS_DENY | CIS_DENYDISABLED));
                DWORD OtherStateOld = infoPtr->FocusedCheckItem->State & OtherStateMask;

                infoPtr->FocusedPushed = FALSE;

                if (ChangeCheckBox(infoPtr,
                                   infoPtr->FocusedCheckItem,
                                   infoPtr->FocusedCheckItemBox))
                {
                    UpdateCheckItemBox(infoPtr,
                                       infoPtr->FocusedCheckItem,
                                       infoPtr->FocusedCheckItemBox);

                    if ((infoPtr->FocusedCheckItem->State & OtherStateMask) != OtherStateOld)
                    {
                        UpdateCheckItemBox(infoPtr,
                                           infoPtr->FocusedCheckItem,
                                           OtherBox);
                    }
                }
            }
            break;
        }
        
        case WM_GETDLGCODE:
        {
            INT virtKey;
            
            Ret = 0;
            virtKey = (lParam != 0 ? (INT)((LPMSG)lParam)->wParam : 0);
            switch (virtKey)
            {
                case VK_RETURN:
                {
                    if (infoPtr->QuickSearchEnabled && infoPtr->QuickSearchHitItem != NULL)
                    {
                        Ret |= DLGC_WANTCHARS | DLGC_WANTMESSAGE;
                    }
                    else
                    {
                        Ret |= DLGC_WANTMESSAGE;
                    }
                    break;
                }

                case VK_TAB:
                {
                    UINT CheckBox;
                    BOOL EnabledBox;
                    BOOL Shift = GetKeyState(VK_SHIFT) & 0x8000;
                    
                    EnabledBox = FindEnabledCheckBox(infoPtr,
                                                     Shift,
                                                     &CheckBox) != NULL;
                    Ret |= (EnabledBox ? DLGC_WANTTAB : DLGC_WANTCHARS);
                    break;
                }
                
                default:
                {
                    if (infoPtr->QuickSearchEnabled)
                    {
                        Ret |= DLGC_WANTCHARS;
                    }
                    break;
                }
            }
            break;
        }
        
        case WM_CHAR:
        {
            QuickSearchFindHit(infoPtr,
                               (WCHAR)wParam);
            break;
        }
        
        case WM_SYSCOLORCHANGE:
        {
            infoPtr->TextColor[0] = GetSysColor(COLOR_GRAYTEXT);
            infoPtr->TextColor[1] = GetSysColor(COLOR_WINDOWTEXT);
            break;
        }
        
#if SUPPORT_UXTHEME
        case WM_MOUSELEAVE:
        {
            if (infoPtr->HoveredCheckItem != NULL)
            {
                /* reset and repaint the hovered check item box */
                ChangeCheckItemHotTrack(infoPtr,
                                        NULL,
                                        0);
            }
            break;
        }
        
        case WM_THEMECHANGED:
        {
            if (infoPtr->ThemeHandle != NULL)
            {
                CloseThemeData(infoPtr->ThemeHandle);
                infoPtr->ThemeHandle = NULL;
            }
            if (IsAppThemed())
            {
                infoPtr->ThemeHandle = OpenThemeData(infoPtr->hSelf,
                                                     L"BUTTON");
            }
            break;
        }
#endif
        
        case WM_SETTINGCHANGE:
        {
            DWORD OldCaretWidth = infoPtr->CaretWidth;
            
#if SUPPORT_UXTHEME
            /* update the hover time */
            if (!SystemParametersInfo(SPI_GETMOUSEHOVERTIME,
                                      0,
                                      &infoPtr->HoverTime,
                                      0))
            {
                infoPtr->HoverTime = HOVER_DEFAULT;
            }
#endif

            /* update the caret */
            if (!SystemParametersInfo(SPI_GETCARETWIDTH,
                                      0,
                                      &infoPtr->CaretWidth,
                                      0))
            {
                infoPtr->CaretWidth = 2;
            }
            if (OldCaretWidth != infoPtr->CaretWidth && infoPtr->ShowingCaret)
            {
                DestroyCaret();
                CreateCaret(hwnd,
                            NULL,
                            infoPtr->CaretWidth,
                            infoPtr->ItemHeight - (2 * CI_TEXT_MARGIN_HEIGHT));
            }
            break;
        }
        
        case WM_SIZE:
        {
            UpdateControl(infoPtr);
            break;
        }
        
        case WM_UPDATEUISTATE:
        {
            DWORD OldUIState = infoPtr->UIState;

            switch (LOWORD(wParam))
            {
                case UIS_SET:
                    infoPtr->UIState |= HIWORD(wParam);
                    break;

                case UIS_CLEAR:
                    infoPtr->UIState &= ~(HIWORD(wParam));
                    break;
            }

            if (OldUIState != infoPtr->UIState)
            {
                if (infoPtr->FocusedCheckItem != NULL)
                {
                    UpdateCheckItemBox(infoPtr,
                                       infoPtr->FocusedCheckItem,
                                       infoPtr->FocusedCheckItemBox);
                }
            }
            break;
        }
        
        case WM_TIMER:
        {
            switch (wParam)
            {
                case TIMER_ID_SETHITFOCUS:
                {
                    /* kill the timer */
                    KillTimer(hwnd,
                              wParam);

                    if (infoPtr->QuickSearchEnabled && infoPtr->QuickSearchHitItem != NULL)
                    {
                        /* change the focus to the hit item, this item has to have
                           at least one enabled checkbox! */
                        ChangeCheckItemFocus(infoPtr,
                                             infoPtr->QuickSearchHitItem,
                                             ((!(infoPtr->QuickSearchHitItem->State & CIS_ALLOWDISABLED)) ? CLB_ALLOW : CLB_DENY));

                        /* start the timer to reset quicksearch */
                        if (infoPtr->QuickSearchResetDelay != 0)
                        {
                            SetTimer(hwnd,
                                     TIMER_ID_RESETQUICKSEARCH,
                                     infoPtr->QuickSearchResetDelay,
                                     NULL);
                        }
                    }
                    break;
                }
                case TIMER_ID_RESETQUICKSEARCH:
                {
                    /* kill the timer */
                    KillTimer(hwnd,
                              wParam);

                    /* escape quick search */
                    EscapeQuickSearch(infoPtr);
                    break;
                }
            }
            break;
        }

        case WM_CREATE:
        {
            infoPtr = HeapAlloc(GetProcessHeap(),
                                0,
                                sizeof(CHECKLISTWND));
            if (infoPtr != NULL)
            {
                RECT rcClient;
                
                infoPtr->hSelf = hwnd;
                infoPtr->hNotify = ((LPCREATESTRUCTW)lParam)->hwndParent;
                
                SetWindowLongPtr(hwnd,
                                 0,
                                 (DWORD_PTR)infoPtr);

                infoPtr->CheckItemListHead = NULL;
                infoPtr->CheckItemCount = 0;
                
                if (!SystemParametersInfo(SPI_GETCARETWIDTH,
                                          0,
                                          &infoPtr->CaretWidth,
                                          0))
                {
                    infoPtr->CaretWidth = 2;
                }
                infoPtr->ItemHeight = 10;
                infoPtr->ShowingCaret = FALSE;
                
                infoPtr->HasFocus = FALSE;
                infoPtr->FocusedCheckItem = NULL;
                infoPtr->FocusedCheckItemBox = 0;
                infoPtr->FocusedPushed = FALSE;
                
                infoPtr->TextColor[0] = GetSysColor(COLOR_GRAYTEXT);
                infoPtr->TextColor[1] = GetSysColor(COLOR_WINDOWTEXT);
                
                GetClientRect(hwnd,
                              &rcClient);
                
                infoPtr->CheckBoxLeft[0] = rcClient.right - 30;
                infoPtr->CheckBoxLeft[1] = rcClient.right - 15;
                
                infoPtr->QuickSearchEnabled = FALSE;
                infoPtr->QuickSearchText[0] = L'\0';

                infoPtr->QuickSearchSetFocusDelay = DEFAULT_QUICKSEARCH_SETFOCUS_DELAY;
                infoPtr->QuickSearchResetDelay = DEFAULT_QUICKSEARCH_RESET_DELAY;

#if SUPPORT_UXTHEME
                infoPtr->HoveredCheckItem = NULL;
                infoPtr->HoveredCheckItemBox = 0;
                if (!SystemParametersInfo(SPI_GETMOUSEHOVERTIME,
                                          0,
                                          &infoPtr->HoverTime,
                                          0))
                {
                    infoPtr->HoverTime = HOVER_DEFAULT;
                }

                if (IsAppThemed())
                {
                    infoPtr->ThemeHandle = OpenThemeData(infoPtr->hSelf,
                                                         L"BUTTON");
                }
                else
                {
                    infoPtr->ThemeHandle = NULL;
                }
#endif

                infoPtr->UIState = SendMessage(hwnd,
                                               WM_QUERYUISTATE,
                                               0,
                                               0);
            }
            else
            {
                Ret = -1;
            }
            break;
        }
        
        case WM_DESTROY:
        {
            if (infoPtr->ShowingCaret)
            {
                DestroyCaret();
            }

            ClearCheckItems(infoPtr);
            
#if SUPPORT_UXTHEME
            if (infoPtr->ThemeHandle != NULL)
            {
                CloseThemeData(infoPtr->ThemeHandle);
            }
#endif
            
            HeapFree(GetProcessHeap(),
                     0,
                     infoPtr);
            SetWindowLongPtr(hwnd,
                             0,
                             (DWORD_PTR)NULL);
            break;
        }
        
        default:
        {
HandleDefaultMessage:
            Ret = DefWindowProc(hwnd,
                                uMsg,
                                wParam,
                                lParam);
            break;
        }
    }
    
    return Ret;
}

BOOL
RegisterCheckListControl(IN HINSTANCE hInstance)
{
    WNDCLASS wc;
    
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = CheckListWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(PCHECKLISTWND);
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL,
                            (LPWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szCheckListWndClass;
    
    return RegisterClass(&wc) != 0;
}

VOID
UnregisterCheckListControl(HINSTANCE hInstance)
{
    UnregisterClass(szCheckListWndClass,
                    hInstance);
}
