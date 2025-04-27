/*
 * PROJECT:     ReactOS Event Log Viewer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Event Details Control.
 * COPYRIGHT:   Copyright 2007 Marc Piulachs <marc.piulachs@codexchange.net>
 *              Copyright 2008-2016 Eric Kohl <eric.kohl@reactos.org>
 *              Copyright 2016-2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "eventvwr.h"
#include "evtdetctl.h"

#include <shellapi.h>

// FIXME:
#define EVENT_MESSAGE_EVENTTEXT_BUFFER  (1024*10)
extern WCHAR szTitle[];
extern HWND hwndListView;

extern BOOL
GetEventMessage(IN LPCWSTR KeyName,
                IN LPCWSTR SourceName,
                IN PEVENTLOGRECORD pevlr,
                OUT PWCHAR EventText);


typedef struct _DETAILDATA
{
    /* The event record being displayed */
    PEVENTLOGRECORD pevlr;

    /* Data initialized from EVENTDETAIL_INFO */
    PEVENTLOGFILTER EventLogFilter;
    INT iEventItem;

    BOOL bDisplayWords;
    HFONT hMonospaceFont;

    INT cxMin, cyMin;
    INT cxOld, cyOld;
    POINT scPos;
} DETAILDATA, *PDETAILDATA;


static
VOID
DisplayEventData(
    _In_ HWND hDlg,
    _In_ PDETAILDATA pDetailData);

static
VOID
DisplayEvent(
    _In_ HWND hDlg,
    _In_ PDETAILDATA pDetailData)
{
    PEVENTLOGRECORD pevlr;
    PEVENTLOGFILTER EventLogFilter = pDetailData->EventLogFilter;
    INT iItem = pDetailData->iEventItem;
    BOOL bEventData;

    WCHAR szEventType[MAX_PATH];
    WCHAR szTime[MAX_PATH];
    WCHAR szDate[MAX_PATH];
    WCHAR szUser[MAX_PATH];
    WCHAR szComputer[MAX_PATH];
    WCHAR szSource[MAX_PATH];
    WCHAR szCategory[MAX_PATH];
    WCHAR szEventID[MAX_PATH];
    WCHAR szEventText[EVENT_MESSAGE_EVENTTEXT_BUFFER];

    /* Retrieve and cache the pointer to the selected event item */
    LVITEMW li;
    li.mask = LVIF_PARAM;
    li.iItem = iItem;
    li.iSubItem = 0;
    ListView_GetItem(hwndListView, &li);
    pevlr = pDetailData->pevlr = (PEVENTLOGRECORD)li.lParam;

    ListView_GetItemText(hwndListView, iItem, 0, szEventType, ARRAYSIZE(szEventType));
    ListView_GetItemText(hwndListView, iItem, 1, szDate, ARRAYSIZE(szDate));
    ListView_GetItemText(hwndListView, iItem, 2, szTime, ARRAYSIZE(szTime));
    ListView_GetItemText(hwndListView, iItem, 3, szSource, ARRAYSIZE(szSource));
    ListView_GetItemText(hwndListView, iItem, 4, szCategory, ARRAYSIZE(szCategory));
    ListView_GetItemText(hwndListView, iItem, 5, szEventID, ARRAYSIZE(szEventID));
    ListView_GetItemText(hwndListView, iItem, 6, szUser, ARRAYSIZE(szUser));
    ListView_GetItemText(hwndListView, iItem, 7, szComputer, ARRAYSIZE(szComputer));

    SetDlgItemTextW(hDlg, IDC_EVENTDATESTATIC, szDate);
    SetDlgItemTextW(hDlg, IDC_EVENTTIMESTATIC, szTime);
    SetDlgItemTextW(hDlg, IDC_EVENTUSERSTATIC, szUser);
    SetDlgItemTextW(hDlg, IDC_EVENTSOURCESTATIC, szSource);
    SetDlgItemTextW(hDlg, IDC_EVENTCOMPUTERSTATIC, szComputer);
    SetDlgItemTextW(hDlg, IDC_EVENTCATEGORYSTATIC, szCategory);
    SetDlgItemTextW(hDlg, IDC_EVENTIDSTATIC, szEventID);
    SetDlgItemTextW(hDlg, IDC_EVENTTYPESTATIC, szEventType);

    bEventData = (pevlr->DataLength > 0);
    EnableDlgItem(hDlg, IDC_BYTESRADIO, bEventData);
    EnableDlgItem(hDlg, IDC_WORDSRADIO, bEventData);

    // FIXME: At the moment we support only one event log in the filter
    GetEventMessage(EventLogFilter->EventLogs[0]->LogName, szSource, pevlr, szEventText);
    SetDlgItemTextW(hDlg, IDC_EVENTTEXTEDIT, szEventText);

    DisplayEventData(hDlg, pDetailData);
}

static
UINT
PrintByteDataLine(PWCHAR pBuffer, UINT uOffset, PBYTE pData, UINT uLength)
{
    PWCHAR p = pBuffer;
    UINT n, i, r = 0;

    if (uOffset != 0)
    {
        n = swprintf(p, L"\r\n");
        p += n;
        r += n;
    }

    n = swprintf(p, L"%04lx:", uOffset);
    p += n;
    r += n;

    for (i = 0; i < uLength; i++)
    {
        n = swprintf(p, L" %02x", pData[i]);
        p += n;
        r += n;
    }

    for (i = 0; i < 9 - uLength; i++)
    {
        n = swprintf(p, L"   ");
        p += n;
        r += n;
    }

    for (i = 0; i < uLength; i++)
    {
        // NOTE: Normally iswprint should return FALSE for tabs...
        n = swprintf(p, L"%c", (iswprint(pData[i]) && (pData[i] != L'\t')) ? pData[i] : L'.');
        p += n;
        r += n;
    }

    return r;
}

static
UINT
PrintWordDataLine(PWCHAR pBuffer, UINT uOffset, PULONG pData, UINT uLength)
{
    PWCHAR p = pBuffer;
    UINT n, i, r = 0;

    if (uOffset != 0)
    {
        n = swprintf(p, L"\r\n");
        p += n;
        r += n;
    }

    n = swprintf(p, L"%04lx:", uOffset);
    p += n;
    r += n;

    for (i = 0; i < uLength / sizeof(ULONG); i++)
    {
        n = swprintf(p, L" %08lx", pData[i]);
        p += n;
        r += n;
    }

    /* Display the remaining bytes if uLength was not a multiple of sizeof(ULONG) */
    for (i = (uLength / sizeof(ULONG)) * sizeof(ULONG); i < uLength; i++)
    {
        n = swprintf(p, L" %02x", ((PBYTE)pData)[i]);
        p += n;
        r += n;
    }

    return r;
}

static
VOID
DisplayEventData(
    _In_ HWND hDlg,
    _In_ PDETAILDATA pDetailData)
{
    PEVENTLOGRECORD pevlr = pDetailData->pevlr;
    BOOL bDisplayWords = pDetailData->bDisplayWords;

    LPBYTE pData;
    UINT i, uOffset;
    UINT uBufferSize, uLineLength;
    PWCHAR pTextBuffer, pLine;

    if (!pevlr)
        return;

    if (pevlr->DataLength == 0)
    {
        SetDlgItemTextW(hDlg, IDC_EVENTDATAEDIT, L"");
        return;
    }

    if (bDisplayWords)
        uBufferSize = ((pevlr->DataLength / 8) + 1) * 26 * sizeof(WCHAR);
    else
        uBufferSize = ((pevlr->DataLength / 8) + 1) * 43 * sizeof(WCHAR);

    pTextBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, uBufferSize);
    if (!pTextBuffer)
        return;

    pLine = pTextBuffer;
    uOffset = 0;

    for (i = 0; i < pevlr->DataLength / 8; i++)
    {
        pData = (LPBYTE)((LPBYTE)pevlr + pevlr->DataOffset + uOffset);

        if (bDisplayWords)
            uLineLength = PrintWordDataLine(pLine, uOffset, (PULONG)pData, 8);
        else
            uLineLength = PrintByteDataLine(pLine, uOffset, pData, 8);
        pLine = pLine + uLineLength;

        uOffset += 8;
    }

    if (pevlr->DataLength % 8 != 0)
    {
        pData = (LPBYTE)((LPBYTE)pevlr + pevlr->DataOffset + uOffset);

        if (bDisplayWords)
            PrintWordDataLine(pLine, uOffset, (PULONG)pData, pevlr->DataLength % 8);
        else
            PrintByteDataLine(pLine, uOffset, pData, pevlr->DataLength % 8);
    }

    SetDlgItemTextW(hDlg, IDC_EVENTDATAEDIT, pTextBuffer);

    HeapFree(GetProcessHeap(), 0, pTextBuffer);
}

static
HFONT
CreateMonospaceFont(VOID)
{
    LOGFONTW tmpFont = {0};
    HFONT hFont;
    HDC hDC;

    hDC = GetDC(NULL);

    tmpFont.lfHeight = -MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    tmpFont.lfWeight = FW_NORMAL;
    wcscpy(tmpFont.lfFaceName, L"Courier New");

    hFont = CreateFontIndirectW(&tmpFont);

    ReleaseDC(NULL, hDC);

    return hFont;
}

static
VOID
CopyEventEntry(HWND hWnd)
{
    WCHAR tmpHeader[512];
    WCHAR szEventType[MAX_PATH];
    WCHAR szSource[MAX_PATH];
    WCHAR szCategory[MAX_PATH];
    WCHAR szEventID[MAX_PATH];
    WCHAR szDate[MAX_PATH];
    WCHAR szTime[MAX_PATH];
    WCHAR szUser[MAX_PATH];
    WCHAR szComputer[MAX_PATH];
    WCHAR evtDesc[EVENT_MESSAGE_EVENTTEXT_BUFFER];
    ULONG size = 0;
    LPWSTR output;
    HGLOBAL hMem;

    /* Try to open the clipboard */
    if (!OpenClipboard(hWnd))
        return;

    /* Get the formatted text needed to place the content into */
    size += LoadStringW(hInst, IDS_COPY, tmpHeader, ARRAYSIZE(tmpHeader));

    /* Grab all the information and get it ready for the clipboard */
    size += GetDlgItemTextW(hWnd, IDC_EVENTTYPESTATIC, szEventType, ARRAYSIZE(szEventType));
    size += GetDlgItemTextW(hWnd, IDC_EVENTSOURCESTATIC, szSource, ARRAYSIZE(szSource));
    size += GetDlgItemTextW(hWnd, IDC_EVENTCATEGORYSTATIC, szCategory, ARRAYSIZE(szCategory));
    size += GetDlgItemTextW(hWnd, IDC_EVENTIDSTATIC, szEventID, ARRAYSIZE(szEventID));
    size += GetDlgItemTextW(hWnd, IDC_EVENTDATESTATIC, szDate, ARRAYSIZE(szDate));
    size += GetDlgItemTextW(hWnd, IDC_EVENTTIMESTATIC, szTime, ARRAYSIZE(szTime));
    size += GetDlgItemTextW(hWnd, IDC_EVENTUSERSTATIC, szUser, ARRAYSIZE(szUser));
    size += GetDlgItemTextW(hWnd, IDC_EVENTCOMPUTERSTATIC, szComputer, ARRAYSIZE(szComputer));
    size += GetDlgItemTextW(hWnd, IDC_EVENTTEXTEDIT, evtDesc, ARRAYSIZE(evtDesc));

    size++; /* Null-termination */
    size *= sizeof(WCHAR);

    /*
     * Consolidate the information into one big piece and
     * sort out the memory needed to write to the clipboard.
     */
    hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (hMem == NULL) goto Quit;

    output = GlobalLock(hMem);
    if (output == NULL)
    {
        GlobalFree(hMem);
        goto Quit;
    }

    StringCbPrintfW(output, size,
                    tmpHeader, szEventType, szSource, szCategory, szEventID,
                    szDate, szTime, szUser, szComputer, evtDesc);

    GlobalUnlock(hMem);

    /* We succeeded, empty the clipboard and write the data in it */
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hMem);

Quit:
    /* Close the clipboard once we are done with it */
    CloseClipboard();
}

static
VOID
OnLink(HWND hDlg, ENLINK* penLink)
{
    LPWSTR pLink;
    TEXTRANGE txtRange;

    ASSERT(penLink->nmhdr.idFrom == IDC_EVENTTEXTEDIT);

    /* Only act on left button up events */
    if (penLink->msg != WM_LBUTTONUP)
        return;

    /* If the range is empty, do nothing */
    if (penLink->chrg.cpMin == penLink->chrg.cpMax)
        return;

    /* Allocate memory for the text link */
    pLink = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                      (max(penLink->chrg.cpMin, penLink->chrg.cpMax) -
                       min(penLink->chrg.cpMin, penLink->chrg.cpMax) + 1) * sizeof(WCHAR));
    if (!pLink)
    {
        /* Not enough memory, bail out */
        return;
    }

    txtRange.chrg = penLink->chrg;
    txtRange.lpstrText = pLink;
    SendDlgItemMessageW(hDlg, IDC_EVENTTEXTEDIT, EM_GETTEXTRANGE, 0, (LPARAM)&txtRange);

    /* Open the link */
    ShellExecuteW(hDlg, L"open", pLink, NULL, NULL, SW_SHOWNOACTIVATE);

    /* Free the buffer */
    HeapFree(GetProcessHeap(), 0, pLink);
}

static
VOID
OnScroll(HWND hDlg, PDETAILDATA pData, INT nBar, WORD sbCode)
{
    RECT rect;

    SCROLLINFO sInfo;
    INT oldPos, Maximum;
    PLONG pOriginXY;

    ASSERT(nBar == SB_HORZ || nBar == SB_VERT);

    GetClientRect(hDlg, &rect);

    if (nBar == SB_HORZ)
    {
        Maximum = pData->cxMin - (rect.right-rect.left) /* pData->cxOld */;
        pOriginXY = &pData->scPos.x;
    }
    else // if (nBar == SB_VERT)
    {
        Maximum = pData->cyMin - (rect.bottom-rect.top) /* pData->cyOld */;
        pOriginXY = &pData->scPos.y;
    }

    /* Set scrollbar sizes */
    sInfo.cbSize = sizeof(sInfo);
    sInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE | SIF_TRACKPOS;

    if (!GetScrollInfo(hDlg, nBar, &sInfo))
        return;

    oldPos = sInfo.nPos;

    switch (sbCode)
    {
        case SB_LINEUP:   // SB_LINELEFT:
            sInfo.nPos--;
            break;

        case SB_LINEDOWN: // SB_LINERIGHT:
            sInfo.nPos++;
            break;

        case SB_PAGEUP:   // SB_PAGELEFT:
            sInfo.nPos -= sInfo.nPage;
            break;

        case SB_PAGEDOWN: // SB_PAGERIGHT:
            sInfo.nPos += sInfo.nPage;
            break;

        case SB_THUMBTRACK:
            sInfo.nPos = sInfo.nTrackPos;
            break;

        case SB_THUMBPOSITION:
            sInfo.nPos = sInfo.nTrackPos;
            break;

        case SB_TOP:    // SB_LEFT:
            sInfo.nPos = sInfo.nMin;
            break;

        case SB_BOTTOM: // SB_RIGHT:
            sInfo.nPos = sInfo.nMax;
            break;

        default:
            break;
    }

    sInfo.nPos = min(max(sInfo.nPos, 0), Maximum);

    if (oldPos != sInfo.nPos)
    {
        POINT scOldPos = pData->scPos;

        /* We now modify pData->scPos */
        *pOriginXY = sInfo.nPos;

        ScrollWindowEx(hDlg,
                       (scOldPos.x - pData->scPos.x),
                       (scOldPos.y - pData->scPos.y),
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN);

        sInfo.fMask = SIF_POS;
        SetScrollInfo(hDlg, nBar, &sInfo, TRUE);

        // UpdateWindow(hDlg);
    }
}

static
VOID
OnSize(HWND hDlg, PDETAILDATA pData, INT cx, INT cy)
{
    LONG_PTR dwStyle;
    INT sbVXSize, sbHYSize;
    SCROLLINFO sInfo;
    POINT scOldPos;
    HDWP hdwp;
    HWND hItemWnd;
    RECT rect;
    INT  y = 0;

    if (!pData)
        return;

    dwStyle  = GetWindowLongPtrW(hDlg, GWL_STYLE);
    sbVXSize = GetSystemMetrics(SM_CXVSCROLL);
    sbHYSize = GetSystemMetrics(SM_CYHSCROLL);

    /* Compensate for existing scroll bars (because lParam values do not accommodate scroll bar) */
    if (dwStyle & WS_HSCROLL) cy += sbHYSize; // Window currently has a horizontal scrollbar
    if (dwStyle & WS_VSCROLL) cx += sbVXSize; // Window currently has a vertical scrollbar

    /* Compensate for added scroll bars in window */
    if (cx < pData->cxMin) cy -= sbHYSize; // Window will have a horizontal scroll bar
    if (cy < pData->cyMin) cx -= sbVXSize; // Window will have a vertical scroll bar

    /* Set scrollbar sizes */
    sInfo.cbSize = sizeof(sInfo);

    sInfo.fMask = SIF_POS;
    if (GetScrollInfo(hDlg, SB_VERT, &sInfo))
        scOldPos.y = sInfo.nPos;
    else
        scOldPos.y = pData->scPos.y;

    sInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    sInfo.nMin = 0;
    if (pData->cyMin > cy)
    {
        sInfo.nMax  = pData->cyMin - 1;
        sInfo.nPage = cy;
        sInfo.nPos  = pData->scPos.y;
        SetScrollInfo(hDlg, SB_VERT, &sInfo, TRUE);

        /* Display the scrollbar if needed */
        if (!(dwStyle & WS_VSCROLL))
            ShowScrollBar(hDlg, SB_VERT, TRUE);
    }
    else
    {
        scOldPos.y = 0;

        sInfo.nMax  = pData->cyMin - 1;
        sInfo.nPage = cy;
        sInfo.nPos  = pData->scPos.y;
        sInfo.nPos  = scOldPos.y;
        SetScrollInfo(hDlg, SB_VERT, &sInfo, TRUE);

        ShowScrollBar(hDlg, SB_VERT, FALSE);

        rect.left   = cx - sbVXSize;
        rect.right  = cx;
        rect.top    = 0;
        rect.bottom = cy;
        InvalidateRect(hDlg, &rect, TRUE);
    }

    sInfo.fMask = SIF_POS;
    if (GetScrollInfo(hDlg, SB_HORZ, &sInfo))
        scOldPos.x = sInfo.nPos;
    else
        scOldPos.x = pData->scPos.x;

    sInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    sInfo.nMin = 0;
    if (pData->cxMin > cx)
    {
        sInfo.nMax  = pData->cxMin - 1;
        sInfo.nPage = cx;
        sInfo.nPos  = pData->scPos.x;
        SetScrollInfo(hDlg, SB_HORZ, &sInfo, TRUE);

        /* Display the scrollbar if needed */
        if (!(dwStyle & WS_HSCROLL))
            ShowScrollBar(hDlg, SB_HORZ, TRUE);
    }
    else
    {
        scOldPos.x = 0;

        sInfo.nMax  = pData->cxMin - 1;
        sInfo.nPage = cx;
        sInfo.nPos  = pData->scPos.x;
        sInfo.nPos  = scOldPos.x;
        SetScrollInfo(hDlg, SB_HORZ, &sInfo, TRUE);

        ShowScrollBar(hDlg, SB_HORZ, FALSE);

        rect.left   = 0;
        rect.right  = cx;
        rect.top    = cy - sbHYSize;
        rect.bottom = cy;
        InvalidateRect(hDlg, &rect, TRUE);
    }

    if ((scOldPos.x != pData->scPos.x) || (scOldPos.y != pData->scPos.y))
    {
        ScrollWindowEx(hDlg,
                       // (scOldPos.x - pData->scPos.x),
                       (pData->scPos.x - scOldPos.x),
                       // (scOldPos.y - pData->scPos.y),
                       (pData->scPos.y - scOldPos.y),
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN);

        pData->scPos = scOldPos;
    }

    // /* Adjust the start of the visible area if we are attempting to show nonexistent areas */
    // if ((pData->cxMin - pData->scPos.x) < cx) pData->scPos.x = pData->cxMin - cx;
    // if ((pData->cyMin - pData->scPos.y) < cy) pData->scPos.y = pData->cyMin - cy;
    // // InvalidateRect(GuiData->hWindow, NULL, TRUE);

    /* Forbid resizing the control smaller than its minimal size */
    if (cx < pData->cxMin) cx = pData->cxMin;
    if (cy < pData->cyMin) cy = pData->cyMin;

    if ((cx != pData->cxOld) || (cy != pData->cyOld))
    {
        hdwp = BeginDeferWindowPos(8);

        /* Move the edit boxes */

        GetWindowRect(hDlg, &rect);

        hItemWnd = GetDlgItem(hDlg, IDC_EVENTTEXTEDIT);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));
        // OffsetRect(&rect, 0, y);
        // y += (cy - pData->cyOld) / 2 ; // + (cy - pData->cyOld) % 2;
        /** y += (cy - pData->cyOld) / 2 ; // + (cy - pData->cyOld) % 2; **/
        if (cy >= pData->cyOld)
            y += (cy - pData->cyOld) / 2 + (cy - pData->cyOld) % 2;
        else
            y -= (pData->cyOld - cy) / 2 + (pData->cyOld - cy) % 2;

        if (hdwp)
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  0,
                                  rect.left, rect.top,
                                  (rect.right - rect.left) + (cx - pData->cxOld),
                                  (rect.bottom - rect.top) + y,
                                  /** SWP_NOMOVE | **/ SWP_NOZORDER | SWP_NOACTIVATE);

        hItemWnd = GetDlgItem(hDlg, IDC_DETAILS_STATIC);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));
        // OffsetRect(&rect, 0, y);

        if (hdwp)
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  0,
                                  rect.left, rect.top + y,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        hItemWnd = GetDlgItem(hDlg, IDC_BYTESRADIO);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));
        // OffsetRect(&rect, 0, y);

        if (hdwp)
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  0,
                                  rect.left, rect.top + y,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        hItemWnd = GetDlgItem(hDlg, IDC_WORDSRADIO);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));
        // OffsetRect(&rect, 0, y);

        if (hdwp)
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  0,
                                  rect.left, rect.top + y,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        hItemWnd = GetDlgItem(hDlg, IDC_EVENTDATAEDIT);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));
        // OffsetRect(&rect, 0, y);
        // // y -= (cy - pData->cyOld) % 2;

        if (hdwp)
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  0,
                                  rect.left, rect.top + y,
                                  (rect.right - rect.left) + (cx - pData->cxOld),
                                  (rect.bottom - rect.top) + y,
                                  SWP_NOZORDER | SWP_NOACTIVATE);

        /* Move the buttons */

        hItemWnd = GetDlgItem(hDlg, IDC_PREVIOUS);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  0,
                                  rect.left + (cx - pData->cxOld),
                                  rect.top,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        hItemWnd = GetDlgItem(hDlg, IDC_NEXT);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  0,
                                  rect.left + (cx - pData->cxOld),
                                  rect.top,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        hItemWnd = GetDlgItem(hDlg, IDC_COPY);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP /*NULL*/, hDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  0,
                                  rect.left + (cx - pData->cxOld),
                                  rect.top,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        if (hdwp)
            EndDeferWindowPos(hdwp);

        pData->cxOld = cx;
        pData->cyOld = cy;
    }
}

static
VOID
EnableNavigationArrows(
    _In_ HWND hDlg,
    _In_ BOOL bEnable)
{
    /* Enable Previous/Next only if there is more than one item in the list */
    if (bEnable)
        bEnable &= (ListView_GetItemCount(hwndListView) > 1);
    EnableDlgItem(hDlg, IDC_PREVIOUS, bEnable);
    EnableDlgItem(hDlg, IDC_NEXT, bEnable);
}

static
VOID
ClearContents(
    _In_ HWND hDlg)
{
    /* Disable the Previous/Next and Copy buttons */
    EnableNavigationArrows(hDlg, FALSE);
    EnableDlgItem(hDlg, IDC_COPY, FALSE);

    /* Disable the Bytes/Words mode buttons */
    EnableDlgItem(hDlg, IDC_BYTESRADIO, FALSE);
    EnableDlgItem(hDlg, IDC_WORDSRADIO, FALSE);

    /* Clear the data fields */
    SetDlgItemTextW(hDlg, IDC_EVENTDATESTATIC, L"");
    SetDlgItemTextW(hDlg, IDC_EVENTTIMESTATIC, L"");
    SetDlgItemTextW(hDlg, IDC_EVENTUSERSTATIC, L"");
    SetDlgItemTextW(hDlg, IDC_EVENTSOURCESTATIC, L"");
    SetDlgItemTextW(hDlg, IDC_EVENTCOMPUTERSTATIC, L"");
    SetDlgItemTextW(hDlg, IDC_EVENTCATEGORYSTATIC, L"");
    SetDlgItemTextW(hDlg, IDC_EVENTIDSTATIC, L"");
    SetDlgItemTextW(hDlg, IDC_EVENTTYPESTATIC, L"");

    SetDlgItemTextW(hDlg, IDC_EVENTTEXTEDIT, L"");
    SetDlgItemTextW(hDlg, IDC_EVENTDATAEDIT, L"");
}

static
VOID
InitDetailsDlgCtrl(HWND hDlg, PDETAILDATA pData)
{
    DWORD dwMask;

    HANDLE nextIcon = LoadImageW(hInst, MAKEINTRESOURCEW(IDI_NEXT), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    HANDLE prevIcon = LoadImageW(hInst, MAKEINTRESOURCEW(IDI_PREV), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    HANDLE copyIcon = LoadImageW(hInst, MAKEINTRESOURCEW(IDI_COPY), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    SendDlgItemMessageW(hDlg, IDC_NEXT, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)nextIcon);
    SendDlgItemMessageW(hDlg, IDC_PREVIOUS, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)prevIcon);
    SendDlgItemMessageW(hDlg, IDC_COPY, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)copyIcon);

    /* Set the default read-only RichEdit color */
    SendDlgItemMessageW(hDlg, IDC_EVENTTEXTEDIT, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_3DFACE));

    /* Enable RichEdit coloured and underlined links */
    dwMask = SendDlgItemMessageW(hDlg, IDC_EVENTTEXTEDIT, EM_GETEVENTMASK, 0, 0);
    SendDlgItemMessageW(hDlg, IDC_EVENTTEXTEDIT, EM_SETEVENTMASK, 0, dwMask | ENM_LINK | ENM_MOUSEEVENTS);

    /*
     * Activate automatic URL recognition by the RichEdit control. For more information, see:
     * https://learn.microsoft.com/en-us/archive/blogs/murrays/automatic-richedit-hyperlinks
     * https://learn.microsoft.com/en-us/archive/blogs/murrays/richedit-friendly-name-hyperlinks
     * https://learn.microsoft.com/en-us/windows/win32/controls/em-autourldetect
     */
    SendDlgItemMessageW(hDlg, IDC_EVENTTEXTEDIT, EM_AUTOURLDETECT, AURL_ENABLEURL /* | AURL_ENABLEEAURLS */, 0);

    /* Note that the RichEdit control never gets themed under WinXP+; one would have to write code to simulate Edit-control theming */

    SendDlgItemMessageW(hDlg, pData->bDisplayWords ? IDC_WORDSRADIO : IDC_BYTESRADIO, BM_SETCHECK, BST_CHECKED, 0);
    SendDlgItemMessageW(hDlg, IDC_EVENTDATAEDIT, WM_SETFONT, (WPARAM)pData->hMonospaceFont, (LPARAM)TRUE);

    //ClearContents(hDlg);
    if (pData->iEventItem != -1)
    {
        EnableNavigationArrows(hDlg, TRUE);
        EnableDlgItem(hDlg, IDC_COPY, TRUE);
    }
    else
    {
        EnableNavigationArrows(hDlg, FALSE);
        EnableDlgItem(hDlg, IDC_COPY, FALSE);
    }
    EnableDlgItem(hDlg, IDC_BYTESRADIO, FALSE);
    EnableDlgItem(hDlg, IDC_WORDSRADIO, FALSE);
}

/* Message handler for Event Details control */
static
INT_PTR CALLBACK
EventDetailsCtrl(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PDETAILDATA pData;

    pData = (PDETAILDATA)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            RECT rect;

            pData = (PDETAILDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pData));
            if (!pData)
            {
                EndDialog(hDlg, 0);
                return (INT_PTR)TRUE;
            }
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)pData);

            if (lParam != 0)
            {
                PEVENTDETAIL_INFO DetailInfo = (PEVENTDETAIL_INFO)lParam;
                pData->EventLogFilter = DetailInfo->EventLogFilter;
                pData->iEventItem     = DetailInfo->iEventItem;
            }
            else
            {
                pData->iEventItem = -1;
            }

            pData->bDisplayWords  = FALSE;
            pData->hMonospaceFont = CreateMonospaceFont();

            GetClientRect(hDlg, &rect);
            pData->cxOld = pData->cxMin = rect.right - rect.left;
            pData->cyOld = pData->cyMin = rect.bottom - rect.top;
            pData->scPos.x = pData->scPos.y = 0;

            InitDetailsDlgCtrl(hDlg, pData);
            /* NOTE: Showing the event (if any) is currently delayed to later */
            // SendMessageW(hDlg, EVT_DISPLAY, TRUE, (LPARAM)pData->iEventItem);

            // OnSize(hDlg, pData, pData->cxOld, pData->cyOld);
            return (INT_PTR)TRUE;
        }

        case WM_DESTROY:
            if (pData)
            {
                if (pData->hMonospaceFont)
                    DeleteObject(pData->hMonospaceFont);
                HeapFree(GetProcessHeap(), 0, pData);
            }
            return (INT_PTR)TRUE;

        case EVT_SETFILTER:
        {
            /* Disable the display first, before changing the current filter */
            pData->pevlr = NULL;
            pData->iEventItem = -1;
            ClearContents(hDlg);
            pData->EventLogFilter = (PEVENTLOGFILTER)lParam;
            return (INT_PTR)TRUE;
        }

        case EVT_DISPLAY:
        {
            if (wParam)
            {
                INT iEventItem = (INT)lParam;

                /* If no filter is set, don't change anything */
                if (!pData->EventLogFilter)
                    return (INT_PTR)TRUE;

                /* If we re-enable display from previously disabled state, re-enable the buttons */
                if ((pData->iEventItem == -1) && (iEventItem != -1))
                {
                    EnableNavigationArrows(hDlg, TRUE);
                    EnableDlgItem(hDlg, IDC_COPY, TRUE);
                }
                else
                /* If we disable display from previously enabled state, clear and disable it */
                if ((pData->iEventItem != -1) && (iEventItem == -1))
                {
                    ClearContents(hDlg);
                }

                /* Set the new item */
                pData->pevlr = NULL;
                pData->iEventItem = iEventItem;

                /* Display the event info, if there is one */
                if (pData->iEventItem != -1)
                    DisplayEvent(hDlg, pData);
            }
            else
            {
                /* Enable or disable the Previous/Next buttons,
                 * but keep the existing contents, if any */
                EnableNavigationArrows(hDlg, (lParam != -1));

                // HACK: Disable the Bytes/Words mode buttons
                // because we won't have access anymore to the
                // event data. (This will be fixed in the future.)
                pData->pevlr = NULL; // Invalidate also the cache.
                EnableDlgItem(hDlg, IDC_BYTESRADIO, FALSE);
                EnableDlgItem(hDlg, IDC_WORDSRADIO, FALSE);
            }
            return (INT_PTR)TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_PREVIOUS:
                case IDC_NEXT:
                {
                    BOOL bPrev = (LOWORD(wParam) == IDC_PREVIOUS);
                    INT nItems = ListView_GetItemCount(hwndListView);
                    INT iItem, iSel;

                    if (nItems <= 0) /* No items? */
                        break;

                    /* Get the index of the first selected item */
                    iSel = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);

                    /* Select the previous/next item from our current one */
                    iItem = ListView_GetNextItem(hwndListView, iSel,
                                                 (bPrev ? LVNI_ABOVE : LVNI_BELOW));
                    if (iItem < 0 || iItem >= nItems)
                    {
                        /* Confirm selection restart only if an item was previously
                         * selected. If not, just proceed with default selection. */
                        if (iSel != -1)
                        {
                            WCHAR szText[200];
                            LoadStringW(hInst,
                                        (bPrev ? IDS_CONTFROMEND : IDS_CONTFROMBEGINNING),
                                        szText, _countof(szText));
                            if (MessageBoxW(hDlg, szText, szTitle, MB_YESNO | MB_ICONQUESTION) != IDYES)
                                break;
                        }

                        /* Determine where to restart from */
                        iItem = (bPrev ? (nItems - 1) : 0);
                    }

                    /*
                     * Deselect the currently selected items in the list view.
                     * (They may be different from our current one, if multiple
                     * event details are being displayed concurrently!)
                     */
                    iSel = -1;
                    while ((iSel = ListView_GetNextItem(hwndListView, iSel, LVNI_SELECTED)) != -1)
                    {
                        ListView_SetItemState(hwndListView, iSel,
                                              0, LVIS_FOCUSED | LVIS_SELECTED);
                    }

                    /* Select the new item */
                    ListView_SetItemState(hwndListView, iItem,
                                          LVIS_FOCUSED | LVIS_SELECTED,
                                          LVIS_FOCUSED | LVIS_SELECTED);
                    ListView_EnsureVisible(hwndListView, iItem, FALSE);

                    pData->pevlr = NULL;
                    pData->iEventItem = iItem;

                    /* Display the event info */
                    if (pData->EventLogFilter)
                        DisplayEvent(hDlg, pData);
                    return (INT_PTR)TRUE;
                }

                case IDC_COPY:
                    if (pData->EventLogFilter)
                        CopyEventEntry(hDlg);
                    return (INT_PTR)TRUE;

                case IDC_BYTESRADIO:
                case IDC_WORDSRADIO:
                    pData->bDisplayWords = (LOWORD(wParam) == IDC_WORDSRADIO);
                    if (pData->EventLogFilter)
                        DisplayEventData(hDlg, pData);
                    return (INT_PTR)TRUE;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            if (hdr->idFrom == IDC_EVENTTEXTEDIT)
            {
                switch (hdr->code)
                {
                    case EN_LINK:
                        OnLink(hDlg, (ENLINK*)lParam);
                        break;
                }
            }
            break;
        }

        case WM_HSCROLL:
        case WM_VSCROLL:
        {
            OnScroll(hDlg, pData,
                     (uMsg == WM_HSCROLL) ? SB_HORZ : SB_VERT,
                     LOWORD(wParam));
            SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
            return (INT_PTR)TRUE;
        }

        case WM_SIZE:
            OnSize(hDlg, pData, LOWORD(lParam), HIWORD(lParam));
            SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, 0);
            return (INT_PTR)TRUE;
    }

    return (INT_PTR)FALSE;
}

HWND
CreateEventDetailsCtrl(HINSTANCE hInstance,
                       HWND hParentWnd,
                       LPARAM lParam)
{
    return CreateDialogParamW(hInstance,
                              MAKEINTRESOURCEW(IDD_EVENTDETAILS_CTRL),
                              hParentWnd, EventDetailsCtrl, lParam);
}
