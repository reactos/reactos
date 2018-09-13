//**********************************************************************
// File name: oleedit.cxx
//
// Implements the extensions required to enable drag and drop with edit
// controls
// History :
//       Dec 23, 1997   [v-nirnay]    wrote it.
//
//
// Copyright (c) 1997-1999 Microsoft Corporation.
//**********************************************************************

#include <windows.h>
#include <ole2.h>

#include "olecomon.h"
#include "cdataobj.h"
#include "cdropsrc.h"
#include "uce.h"
#include "oleedit.h"

WNDPROC fnEditProc = NULL;

//**********************************************************************
// SetEditProc
//
// Purpose:
//      Replaces current Edit window procedure with a new procedure
//      capable of handling drag and drop
//
// Parameters:
//      HWND  hWndEdit      -   Edit Control Handle
//
// Return Value:
//      TRUE                -   Success
//**********************************************************************
BOOL SetEditProc(HWND hWndEdit)
{
    fnEditProc = (WNDPROC)SetWindowLongPtr (hWndEdit, GWLP_WNDPROC,
        (LPARAM)OleEnabledEditControlProc);

    return TRUE;
}

//**********************************************************************
// OleEnabledEditControlProc
//
// Purpose:
//      Edit control procedure capable of handling drag and drop
//
//**********************************************************************
LRESULT CALLBACK OleEnabledEditControlProc(HWND    hWnd,
                                           UINT    uMessage,
                                           WPARAM  wParam,
                                           LPARAM  lParam)
{
    static POINT        ptDragStart;
    static BOOL         fPendingDrag = FALSE;
    static LPDROPSOURCE pDropSource = NULL;

    switch(uMessage) {
    case WM_LBUTTONDOWN:
        {
            // In case there is no drop source create one
            if (pDropSource == NULL) {
                // Create an instance of CDropSource for Drag and Drop
                pDropSource = new CDropSource;

                if (pDropSource == NULL) {
                    // Show error message and prevent window from being shown
                    return -1;
                }
            }

            // Store the point at which LBUTTON was down
            ptDragStart.x = (int)(short)LOWORD (lParam);
            ptDragStart.y = (int)(short)HIWORD (lParam);

            // Find it point is in current selection
            // If it is then go to drag mode
            if (PointInSel(hWnd, ptDragStart)) {
                fPendingDrag = TRUE;

                // Start timer
                SetTimer(hWnd, 1, nDragDelay, NULL);
                SetCapture(hWnd);

                // Do not pass control to edit window because it removes selection
                return TRUE;
            }

            break;
        }

    case WM_LBUTTONUP:
        {
            // Button came up before starting drag so clear flags and timer
            if (fPendingDrag)
            {
                ReleaseCapture();
                KillTimer(hWnd, 1);
                fPendingDrag = FALSE;
            }
            break;
        }

    case WM_MOUSEMOVE:
        {
            int x, y;

            // If a drag is pending and mouse moves beyond threshold
            // then start our drag and drop operation
            if (fPendingDrag)
            {
                x = (int)(short)LOWORD (lParam);
                y = (int)(short)HIWORD (lParam);

                // Find if the point at which the mouse is is beyond the
                // min rectangle enclosing the point at which LBUTTON
                // was down
                if (! (((ptDragStart.x - nDragMinDist) <= x)
                    && (x <= (ptDragStart.x + nDragMinDist))
                    && ((ptDragStart.y - nDragMinDist) <= y)
                    && (y <= (ptDragStart.y + nDragMinDist))) )
                {
                    // mouse moved beyond threshhold to start drag
                    ReleaseCapture();
                    KillTimer(hWnd, 1);
                    fPendingDrag = FALSE;

                    // perform the modal drag/drop operation.
                    EditCtrlDragAndDrop(GetParent(hWnd), hWnd, pDropSource);
                }
            }
            break;
        }

    case WM_TIMER:
        {
            // If the user has kept LBUTTON down for long then
            // start drag and drop operation
            ReleaseCapture();
            KillTimer(hWnd, 1);
            fPendingDrag = FALSE;

            // perform the modal drag/drop operation.
            EditCtrlDragAndDrop(GetParent(hWnd), hWnd, pDropSource);

            break;
        }

    case WM_DESTROY:
        {
            // Release pDropSource it will automatically destroy itself
            if (pDropSource)
                pDropSource->Release();

            break;
        }
    }

    return CallWindowProc(fnEditProc, hWnd, uMessage, wParam, lParam) ;
}

//**********************************************************************
// PointInSel
//
// Purpose:
//      Finds if the point provided is in current selection of edit control
//
// Parameters:
//      HWND  hWnd          -   Edit Control Handle
//      POINT ptDragStart   -   Point to be checked
//
// Return Value:
//      TRUE                -   Point in selection
//      FALSE               -   Point not in selection
//**********************************************************************
static BOOL PointInSel(HWND hWnd,
                       POINT ptDragStart)
{
    DWORD   dwStart, dwEnd;
    int     dwCharPos;
    TCHAR   szMessage[40];
    HGLOBAL hText;
    int     nChars, nRetChars;
    LPTSTR  lpszEditText;

    // Get current selection
    SendMessage(hWnd, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

    // IF there is not selection
    if (dwStart == dwEnd) {
        return FALSE;
    }

    // EM_CHARFROMPOS did not work, find why
    // dwCharPos = SendMessage(hWnd, EM_CHARFROMPOS, (WPARAM)0L, (LPARAM)&ptDragStart);

    // Get text from edit control
    nChars = GetWindowTextLength(hWnd);
    hText = GlobalAlloc(GMEM_SHARE | GMEM_ZEROINIT, CTOB((nChars + 1)));

    lpszEditText = (LPTSTR)GlobalLock(hText);

    nRetChars = GetWindowText(hWnd, lpszEditText, nChars+1);
    GlobalUnlock(hText);

    // Find char from click
    dwCharPos = XToCP(hWnd, lpszEditText, ptDragStart);
    GlobalFree(hText);

    wsprintf(szMessage, TEXT("%d %d %d\n"), dwStart, dwEnd, dwCharPos);
    TRACE(szMessage);

    // Find if it is within selection
    if (((DWORD)dwCharPos >= dwStart) && ((DWORD)dwCharPos < dwEnd)) {
        return TRUE; // Lbutton was down in selection
    }

    return FALSE;
}

//**********************************************************************
// XToCP
//
// Purpose:
//      Returns char index of the character nearest to specified point
//
// Parameters:
//      HWND    hWnd        -   Edit Control Handle
//      LPTSTR  lpszText    -   Edit control text
//      POINT   ptDragStart -   Point to be checked
//
// Return Value:
//      int                 -   Char nearest to specified point
//**********************************************************************
static int XToCP(HWND   hWnd,
                 LPTSTR lpszText,
                 POINT  ptDragStart)
{
    int     nLength, i, x = 0, xPos = ptDragStart.x;
    int     iWidth[2];
    BOOL    fSuccess, fCharFound=FALSE;
    HDC     hDC;
    HFONT   hFont, hOldFont;

    nLength = wcslen(lpszText);
    hDC = GetDC(hWnd);
    hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, (WPARAM)0L, (LPARAM)0L);
    hOldFont = (HFONT)SelectObject(hDC, hFont);

    for(i=0; i<nLength; i++) {
        fSuccess = GetCharWidth32(hDC,
            lpszText[i],
            lpszText[i],
            iWidth);

        if (fSuccess == FALSE) {
            ReleaseDC(hWnd, hDC);
            return -1;
        }

        x += iWidth[0];

        if (x > xPos) {
            fCharFound = TRUE;
            break;
        }
    }

    SelectObject(hDC, hOldFont);
    ReleaseDC(hWnd, hDC);

    return ((fCharFound==TRUE)? i: -1);
}

//**********************************************************************
// EditCtrlDragAndDrop
//
// Purpose:
//      Does drag and drop of text currently selected in edit control
//
// Parameters:
//      HWND          hWnd        -   Edit Control Handle
//      LPDROPSOURCE  pDropSource -   Pointer to drop source
//
// Return Value:
//      0                         -   Success
//      -1                        -   Failure
//**********************************************************************
static int EditCtrlDragAndDrop(HWND         hWndDlg,
                               HWND         hWndEdit,
                               LPDROPSOURCE pDropSource)
{
    PCImpIDataObject    pDataObject;
    DWORD               dwEffect, dwStart, dwEnd;
    LPTSTR              lpszEditText, lpszSelectedText;
    HGLOBAL             hText;
    int                 nChars, nRetChars;

    // Get current selection
    SendMessage(hWndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    // IF there is not selection
    if (dwStart == dwEnd) {
        return 0;
    }

    nChars = GetWindowTextLength(hWndEdit);
    hText = GlobalAlloc(0, CTOB((nChars + 1)));

    lpszEditText = (LPTSTR)GlobalLock(hText);

    nRetChars = GetWindowText(hWndEdit, lpszEditText, nChars+1);

    // The text in which we are interested is the selected text
    lpszSelectedText = lpszEditText + dwStart;
    lpszSelectedText[dwEnd-dwStart] = 0;

    GlobalUnlock(hText);

    // Create an instance of the DataObject
    pDataObject = new CImpIDataObject(hWndDlg);

    if (pDataObject == NULL) {
        return -1;
    }

    pDataObject->SetText(lpszSelectedText);

    // Do drag and drop with copy
    DoDragDrop(pDataObject, pDropSource, DROPEFFECT_COPY, &dwEffect);

    // Free instance of DataObject
    pDataObject->Release();

    GlobalFree(hText);

    return 0;
}

