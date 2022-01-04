/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Classes for displaying progress dialog.
 * COPYRIGHT:   Copyright 2021 He Yang <1160386205@qq.com>
 */

#include "iernonce.h"
#include <process.h>

#define ITEM_VPADDING     3
#define ITEM_LEFTPADDING 22

HFONT CreateBoldFont(_In_ HFONT hOrigFont)
{
    LOGFONTW fontAttributes = { 0 };
    GetObjectW(hOrigFont, sizeof(fontAttributes), &fontAttributes);
    fontAttributes.lfWeight = FW_BOLD;

    return CreateFontIndirectW(&fontAttributes);
}

ProgressDlg::ProgressDlg(_In_ RunOnceExInstance &RunOnceExInst) :
    m_hListBox(NULL),
    m_hBoldFont(NULL),
    m_PointedItem(0),
    m_RunOnceExInst(RunOnceExInst)
{ ; }

BOOL ProgressDlg::RunDialogBox()
{
    // Show the dialog and run the items only when the list is not empty.
    if (m_RunOnceExInst.m_SectionList.GetSize() != 0)
    {
        return (DoModal() == 1);
    }
    return TRUE;
}

void ProgressDlg::CalcTextRect(
    _In_ LPCWSTR lpText,
    _Inout_ PRECT pRect)
{
    HDC hdc = ::GetDC(m_hListBox);
    ::GetClientRect(m_hListBox, pRect);

    pRect->bottom = pRect->top;
    pRect->left += ITEM_LEFTPADDING;

    HFONT OldFont = SelectFont(hdc, GetFont());
    DrawTextW(hdc, lpText, -1, pRect, DT_CALCRECT | DT_WORDBREAK);
    SelectFont(hdc, OldFont);
    ::ReleaseDC(m_hListBox, hdc);

    pRect->bottom -= pRect->top;
    pRect->bottom += ITEM_VPADDING * 2;
    pRect->top = 0;
    pRect->right -= pRect->left;
    pRect->left = 0;
}

void ProgressDlg::ResizeListBoxAndDialog(_In_ int NewHeight)
{
    RECT ListBoxRect;
    RECT DlgRect;
    ::GetWindowRect(m_hListBox, &ListBoxRect);
    GetWindowRect(&DlgRect);

    int HeightDiff = NewHeight - (ListBoxRect.bottom - ListBoxRect.top);

    ::SetWindowPos(m_hListBox, NULL, 0, 0,
                   ListBoxRect.right - ListBoxRect.left, NewHeight,
                   SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);

    SetWindowPos(HWND_TOP, 0, 0,
                 DlgRect.right - DlgRect.left,
                 DlgRect.bottom - DlgRect.top + HeightDiff,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
}

unsigned int __stdcall
RunOnceExExecThread(_In_ void *Param)
{
    ProgressDlg *pProgressDlg = (ProgressDlg *)Param;

    pProgressDlg->m_RunOnceExInst.Exec(pProgressDlg->m_hWnd);
    return 0;
}

BOOL
ProgressDlg::ProcessWindowMessage(
    _In_ HWND hwnd,
    _In_ UINT message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ LRESULT& lResult,
    _In_ DWORD dwMsgMapID)
{
    lResult = 0;
    switch (message)
    {
        case WM_INITDIALOG:
        {
            if (!m_RunOnceExInst.m_Title.IsEmpty())
            {
                SetWindowTextW(m_RunOnceExInst.m_Title);
            }

            m_hListBox = GetDlgItem(IDC_LB_ITEMS);

            m_hBoldFont = CreateBoldFont(GetFont());

            m_hArrowBmp = LoadBitmapW(NULL, MAKEINTRESOURCE(OBM_MNARROW));
            GetObjectW(m_hArrowBmp, sizeof(BITMAP), &m_ArrowBmp);

            // Add all sections with non-empty title into listbox
            int TotalHeight = 0;
            for (int i = 0; i < m_RunOnceExInst.m_SectionList.GetSize(); i++)
            {
                RunOnceExSection &Section = m_RunOnceExInst.m_SectionList[i];

                if (!Section.m_SectionTitle.IsEmpty())
                {
                    INT Index = ListBox_AddString(m_hListBox, Section.m_SectionTitle);
                    TotalHeight += ListBox_GetItemHeight(m_hListBox, Index);
                    ListBox_SetItemData(m_hListBox, Index, i);
                }
            }

            // Remove the sunken-edged border from the listbox.
            ::SetWindowLongPtr(m_hListBox, GWL_EXSTYLE, ::GetWindowLongPtr(m_hListBox, GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);

            ResizeListBoxAndDialog(TotalHeight);

            // Launch a thread to execute tasks.
            HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, RunOnceExExecThread, (void *)this, 0, NULL);
            if (hThread == INVALID_HANDLE_VALUE)
            {
                EndDialog(0);
                return TRUE;
            }
            CloseHandle(hThread);

            lResult = TRUE; // set keyboard focus to the dialog box control. 
            break;
        }

        case WM_MEASUREITEM:
        {
            PMEASUREITEMSTRUCT pMeasureItem = (PMEASUREITEMSTRUCT)lParam;
            RECT TextRect = { 0 };

            CStringW ItemText;
            ListBox_GetText(m_hListBox, pMeasureItem->itemID,
                            ItemText.GetBuffer(ListBox_GetTextLen(m_hListBox,
                            pMeasureItem->itemID) + 1));

            CalcTextRect(ItemText, &TextRect);

            ItemText.ReleaseBuffer();

            pMeasureItem->itemHeight = TextRect.bottom - TextRect.top;
            pMeasureItem->itemWidth  = TextRect.right - TextRect.left;

            break;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pDrawItem = (PDRAWITEMSTRUCT)lParam;
            CStringW ItemText;

            ListBox_GetText(m_hListBox, pDrawItem->itemID,
                            ItemText.GetBuffer(ListBox_GetTextLen(m_hListBox,
                            pDrawItem->itemID) + 1));

            SetBkMode(pDrawItem->hDC, TRANSPARENT);

            HFONT hOldFont = NULL;
            if (m_PointedItem == (INT)pDrawItem->itemData)
            {
                HDC hCompDC = CreateCompatibleDC(pDrawItem->hDC);

                SelectBitmap(hCompDC, m_hArrowBmp);

                int IconLeftPadding = (ITEM_LEFTPADDING - m_ArrowBmp.bmWidth) / 2;
                int IconTopPadding = (pDrawItem->rcItem.bottom - pDrawItem->rcItem.top - m_ArrowBmp.bmHeight) / 2;

                BitBlt(pDrawItem->hDC, IconLeftPadding, pDrawItem->rcItem.top + IconTopPadding,
                       m_ArrowBmp.bmWidth, m_ArrowBmp.bmHeight, hCompDC, 0, 0, SRCAND);

                DeleteDC(hCompDC);

                hOldFont = SelectFont(pDrawItem->hDC, m_hBoldFont);
            }

            pDrawItem->rcItem.left += ITEM_LEFTPADDING;
            pDrawItem->rcItem.top += ITEM_VPADDING;
            DrawTextW(pDrawItem->hDC, ItemText, -1,
                      &(pDrawItem->rcItem), DT_WORDBREAK);

            if (hOldFont)
            {
                SelectFont(pDrawItem->hDC, hOldFont);
            }
            ItemText.ReleaseBuffer();

            break;
        }

        case WM_SETINDEX:
        {
            if ((int)wParam == m_RunOnceExInst.m_SectionList.GetSize())
            {
                // All sections are handled, lParam is bSuccess.
                EndDialog(lParam);
            }
            m_PointedItem = wParam;
            InvalidateRect(NULL);
            break;
        }

        case WM_CTLCOLORLISTBOX:
        {
            lResult = (LRESULT)GetStockBrush(NULL_BRUSH);
            break;
        }

        case WM_DESTROY:
        {
            DeleteObject(m_hArrowBmp);
            DeleteFont(m_hBoldFont);
            break;
        }
    }
    return TRUE;
}
