/*
 * ReactOS Access Control List Editor
 * Copyright (C) 2004 ReactOS Team
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
 *      08/10/2004  Created
 */
#include "acluilib.h"

typedef struct _CHECKITEM
{
    struct _CHECKITEM *Next;
    DWORD State;
    WCHAR Name[1];
} CHECKITEM, *PCHECKITEM;

typedef struct _CHECKLISTWND
{
   HWND hSelf;
   HWND hNotify;
   DWORD Style;
   HFONT hFont;
   
   PCHECKITEM CheckItemListHead;
   UINT CheckItemCount;
   
   INT ItemHeight;
   BOOL HasFocus;
   
   COLORREF TextColor[2];
   UINT CheckBoxLeft[2];
} CHECKLISTWND, *PCHECKLISTWND;

static PCHECKITEM
FindCheckItemByIndex(IN PCHECKLISTWND infoPtr,
                     IN UINT Index)
{
    PCHECKITEM Item, Found = NULL;
    
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
    
    return Found;
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
             IN DWORD State)
{
    PCHECKITEM CurItem;
    PCHECKITEM *PrevPtr = &infoPtr->CheckItemListHead;
    PCHECKITEM Item = HeapAlloc(GetProcessHeap(),
                                0,
                                sizeof(CHECKITEM) + (wcslen(Name) * sizeof(WCHAR)));
    if (Item != NULL)
    {
        for (CurItem = infoPtr->CheckItemListHead;
             CurItem != NULL;
             CurItem = CurItem->Next)
        {
            PrevPtr = &CurItem->Next;
        }
        
        Item->Next = NULL;
        Item->State = State & CIS_MASK;
        wcscpy(Item->Name,
               Name);
        
        *PrevPtr = Item;
        infoPtr->CheckItemCount++;
    }
    
    return Item;
}

static VOID
UpdateControl(IN PCHECKLISTWND infoPtr,
              IN BOOL AllowChangeStyle)
{
    RECT rcClient;
    SCROLLINFO ScrollInfo;
    
    GetClientRect(infoPtr->hSelf,
                  &rcClient);

    ScrollInfo.cbSize = sizeof(ScrollInfo);
    ScrollInfo.fMask = SIF_PAGE | SIF_RANGE;
    ScrollInfo.nMin = 0;
    ScrollInfo.nMax = infoPtr->CheckItemCount;
    ScrollInfo.nPage = ((rcClient.bottom - rcClient.top) + infoPtr->ItemHeight - 1) / infoPtr->ItemHeight;
    ScrollInfo.nPos = 0;
    ScrollInfo.nTrackPos = 0;

    if (AllowChangeStyle)
    {
        /* determine whether the vertical scrollbar has to be visible or not */
        if (ScrollInfo.nMax > ScrollInfo.nPage &&
            !(infoPtr->Style & WS_VSCROLL))
        {
            SetWindowLong(infoPtr->hSelf,
                          GWL_STYLE,
                          infoPtr->Style | WS_VSCROLL);
        }
        else if (ScrollInfo.nMax < ScrollInfo.nPage &&
                 infoPtr->Style & WS_VSCROLL)
        {
            SetWindowLong(infoPtr->hSelf,
                          GWL_STYLE,
                          infoPtr->Style & ~WS_VSCROLL);
        }
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
            height = 0;
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
        infoPtr->ItemHeight = 4 + GetIdealItemHeight(infoPtr);
    }

    UpdateControl(infoPtr,
                  TRUE);

    return hOldFont;
}

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
    
    GetClientRect(infoPtr->hSelf, &rcClient);
    
    if (infoPtr->Style & WS_VSCROLL)
    {
        ScrollPos = GetScrollPos(infoPtr->hSelf,
                                 SB_VERT);
    }
    else
    {
        ScrollPos = 0;
    }
    
    FirstItem = FindCheckItemByIndex(infoPtr,
                                     ScrollPos + VisibleFirstIndex);
    if (FirstItem != NULL)
    {
        RECT TextRect, ItemRect;
        HFONT hOldFont;
        DWORD CurrentIndex;
        COLORREF OldTextColor;
        BOOL Enabled, PrevEnabled;
        POINT ptOld;
        
        Enabled = IsWindowEnabled(infoPtr->hSelf);
        PrevEnabled = Enabled;
        
        ItemRect.left = 0;
        ItemRect.right = rcClient.right;
        ItemRect.top = VisibleFirstIndex * infoPtr->ItemHeight;
        
        TextRect.left = ItemRect.left + 6;
        TextRect.right = ItemRect.right - 6;
        TextRect.top = ItemRect.top + 2;
        
        MoveToEx(hDC,
                 infoPtr->CheckBoxLeft[CLB_ALLOW],
                 ItemRect.top,
                 &ptOld);
        
        OldTextColor = SetTextColor(hDC,
                                    infoPtr->TextColor[Enabled]);

        hOldFont = SelectObject(hDC,
                                infoPtr->hFont);
        
        for (Item = FirstItem, CurrentIndex = VisibleFirstIndex;
             Item != NULL && CurrentIndex <= LastTouchedIndex;
             Item = Item->Next, CurrentIndex++)
        {
            TextRect.bottom = TextRect.top + infoPtr->ItemHeight;
            
            if (Enabled && PrevEnabled != ((Item->State & CIS_DISABLED) == 0))
            {
                PrevEnabled = ((Item->State & CIS_DISABLED) == 0);
                
                SetTextColor(hDC,
                             infoPtr->TextColor[PrevEnabled]);
            }
            
            DrawText(hDC,
                     Item->Name,
                     -1,
                     &TextRect,
                     DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
            
            MoveToEx(hDC,
                     infoPtr->CheckBoxLeft[CLB_ALLOW],
                     TextRect.top - 6,
                     NULL);
            LineTo(hDC,
                   infoPtr->CheckBoxLeft[CLB_ALLOW],
                   TextRect.bottom - 6);
            MoveToEx(hDC,
                     infoPtr->CheckBoxLeft[CLB_DENY],
                     TextRect.top - 6,
                     NULL);
            LineTo(hDC,
                   infoPtr->CheckBoxLeft[CLB_DENY],
                   TextRect.bottom - 6);

            TextRect.top += infoPtr->ItemHeight;
        }

        SelectObject(hDC,
                     hOldFont);

        SetTextColor(hDC,
                     OldTextColor);

        MoveToEx(hDC,
                 ptOld.x,
                 ptOld.y,
                 NULL);
    }
}

LRESULT CALLBACK
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
        return DefWindowProc(hwnd,
                             uMsg,
                             wParam,
                             lParam);
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

                PaintControl(infoPtr,
                             hdc,
                             &rcUpdate);

                if (wParam == 0)
                {
                    EndPaint(hwnd,
                             &ps);
                }
            }
            break;
        }
        
        case WM_VSCROLL:
        {
            SCROLLINFO ScrollInfo;
            
            ScrollInfo.cbSize = sizeof(ScrollInfo);
            ScrollInfo.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_TRACKPOS;

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
                        if (ScrollInfo.nPos + ScrollInfo.nPage <= ScrollInfo.nMax)
                        {
                            ScrollInfo.nPos += ScrollInfo.nPage;
                        }
                        else
                        {
                            ScrollInfo.nPos = ScrollInfo.nMax;
                        }
                        break;

                    case SB_PAGEUP:
                        if (ScrollInfo.nPos >= ScrollInfo.nPage)
                        {
                            ScrollInfo.nPos -= ScrollInfo.nPage;
                        }
                        else
                        {
                            ScrollInfo.nPos = 0;
                        }
                        break;

                    case SB_THUMBPOSITION:
                    {
                        ScrollInfo.nPos = HIWORD(wParam);
                        break;
                    }
                    
                    case SB_THUMBTRACK:
                    {
                        ScrollInfo.nPos = ScrollInfo.nTrackPos;
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
                                       SW_INVALIDATE);

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
            Ret = (AddCheckItem(infoPtr,
                                (LPWSTR)lParam,
                                (DWORD)wParam) != NULL);
            if (Ret)
            {
                UpdateControl(infoPtr,
                              TRUE);
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
                    UpdateControl(infoPtr,
                                  TRUE);
                }
            }
            else
            {
                Ret = FALSE;
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
            UpdateControl(infoPtr,
                          TRUE);
            break;
        }
        
        case CLM_SETCHECKBOXCOLUMN:
        {
            infoPtr->CheckBoxLeft[wParam != CLB_DENY] = (UINT)lParam;
            Ret = 1;
            break;
        }
        
        case CLM_GETCHECKBOXCOLUMN:
        {
            Ret = (LRESULT)infoPtr->CheckBoxLeft[wParam != CLB_DENY];
            break;
        }
        
        case WM_SETFONT:
        {
            Ret = (LRESULT)RetChangeControlFont(infoPtr,
                                                (HFONT)wParam,
                                                (BOOL)lParam);
            break;
        }
        
        case WM_GETFONT:
        {
            Ret = (LRESULT)infoPtr->hFont;
            break;
        }
        
        case WM_STYLECHANGED:
        {
            LPSTYLESTRUCT Style = (LPSTYLESTRUCT)lParam;
            
            if (wParam == GWL_STYLE)
            {
                infoPtr->Style = Style->styleNew;
                UpdateControl(infoPtr,
                              FALSE);
            }
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
                                           SW_INVALIDATE);

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
            break;
        }
        
        case WM_KILLFOCUS:
        {
            infoPtr->HasFocus = FALSE;
            break;
        }
        
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
        {
            if (!infoPtr->HasFocus && IsWindowEnabled(hwnd))
            {
                SetFocus(hwnd);
            }
            break;
        }
        
        case WM_SYSCOLORCHANGE:
        {
            infoPtr->TextColor[0] = GetSysColor(COLOR_GRAYTEXT);
            infoPtr->TextColor[1] = GetSysColor(COLOR_WINDOWTEXT);
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
                infoPtr->Style = ((LPCREATESTRUCTW)lParam)->style;
                
                SetWindowLongPtr(hwnd,
                                 0,
                                 (DWORD_PTR)infoPtr);

                infoPtr->CheckItemListHead = NULL;
                infoPtr->CheckItemCount = 0;
                
                infoPtr->ItemHeight = 10;
                
                infoPtr->HasFocus = FALSE;
                
                infoPtr->TextColor[0] = GetSysColor(COLOR_GRAYTEXT);
                infoPtr->TextColor[1] = GetSysColor(COLOR_WINDOWTEXT);
                
                GetClientRect(hwnd, &rcClient);
                
                infoPtr->CheckBoxLeft[0] = rcClient.right - 30;
                infoPtr->CheckBoxLeft[1] = rcClient.right - 15;
            }
            else
            {
                Ret = -1;
            }
            break;
        }
        
        case WM_DESTROY:
        {
            ClearCheckItems(infoPtr);
            
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
RegisterCheckListControl(HINSTANCE hInstance)
{
    WNDCLASS wc;
    
    ZeroMemory(&wc, sizeof(WNDCLASS));
    
    wc.style = 0;
    wc.lpfnWndProc = CheckListWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(PCHECKLISTWND);
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(0, (LPWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"CHECKLIST_ACLUI";
    
    return RegisterClass(&wc) != 0;
}

VOID
UnregisterCheckListControl(VOID)
{
    UnregisterClass(L"CHECKLIST_ACLUI",
                    NULL);
}
