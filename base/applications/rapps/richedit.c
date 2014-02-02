/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/richedit.c
 * PURPOSE:         RichEdit functions
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"

#include <shlwapi.h>

HWND hRichEdit;
PWSTR pLink = NULL;

VOID
RichEditOnLink(HWND hwnd, ENLINK *Link)
{
    switch (Link->msg)
    {
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        {
            if (pLink) HeapFree(GetProcessHeap(), 0, pLink);

            pLink = HeapAlloc(GetProcessHeap(),
                              0,
                              (max(Link->chrg.cpMin, Link->chrg.cpMax) -
                               min(Link->chrg.cpMin, Link->chrg.cpMax) + 1) * sizeof(WCHAR));
            if (!pLink)
            {
                /* TODO: Error message */
                return;
            }

            SendMessageW(hRichEdit, EM_SETSEL, Link->chrg.cpMin, Link->chrg.cpMax);
            SendMessageW(hRichEdit, EM_GETSELTEXT, 0, (LPARAM)pLink);

            ShowPopupMenu(hwnd, IDR_LINKMENU, -1);
        }
        break;
    }
}

static VOID
SetRangeFormatting(LONG Start, LONG End, DWORD dwEffects)
{
    CHARFORMAT2 CharFormat;

    SendMessageW(hRichEdit, EM_SETSEL, Start, End);

    ZeroMemory(&CharFormat, sizeof(CHARFORMAT2));

    CharFormat.cbSize = sizeof(CHARFORMAT2);
    CharFormat.dwMask = dwEffects;
    CharFormat.dwEffects = dwEffects;

    SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_WORD | SCF_SELECTION, (LPARAM)&CharFormat);

    SendMessageW(hRichEdit, EM_SETSEL, End, End + 1);
}

static LONG
GetRichEditTextLen(VOID)
{
    GETTEXTLENGTHEX TxtLenStruct;

    TxtLenStruct.flags = GTL_NUMCHARS;
    TxtLenStruct.codepage = 1200;

    return (LONG) SendMessageW(hRichEdit, EM_GETTEXTLENGTHEX, (WPARAM)&TxtLenStruct, 0);
}

/*
 * Insert text (without cleaning old text)
 * Supported effects:
 *   - CFM_BOLD
 *   - CFM_ITALIC
 *   - CFM_UNDERLINE
 *   - CFM_LINK
 */
VOID
InsertRichEditText(LPCWSTR lpszText, DWORD dwEffects)
{
    SETTEXTEX SetText;
    LONG Len = GetRichEditTextLen();

    /* Insert new text */
    SetText.flags = ST_SELECTION;
    SetText.codepage = 1200;

    SendMessageW(hRichEdit, EM_SETTEXTEX, (WPARAM)&SetText, (LPARAM)lpszText);

    SetRangeFormatting(Len, Len + wcslen(lpszText),
                       (dwEffects == CFM_LINK) ? (PathIsURLW(lpszText) ? dwEffects : 0) : dwEffects);
}

/*
 * Clear old text and add new
 */
VOID
NewRichEditText(LPCWSTR lpszText, DWORD dwEffects)
{
    SetWindowTextW(hRichEdit, L"");
    InsertRichEditText(lpszText, dwEffects);
}

BOOL
CreateRichEdit(HWND hwnd)
{
    LoadLibraryW(L"riched20.dll");

    hRichEdit = CreateWindowExW(0,
                                L"RichEdit20W",
                                NULL,
                                WS_CHILD | WS_VISIBLE | ES_MULTILINE |
                                ES_LEFT | ES_READONLY,
                                205, 28, 465, 100,
                                hwnd,
                                NULL,
                                hInst,
                                NULL);

    if (!hRichEdit)
    {
        /* TODO: Show error message */
        return FALSE;
    }

    SendMessageW(hRichEdit, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
    SendMessageW(hRichEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);
    SendMessageW(hRichEdit, EM_SETEVENTMASK, 0, ENM_LINK | ENM_MOUSEEVENTS);
    SendMessageW(hRichEdit, EM_SHOWSCROLLBAR, SB_VERT, TRUE);

    return TRUE;
}
