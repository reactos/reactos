/*
 * PROJECT:    ReactOS Notepad
 * LICENSE:    LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:    Providing a Windows-compatible simple text editor for ReactOS
 * COPYRIGHT:  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 *             Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *             Copyright 2002 Andriy Palamarchuk
 *             Copyright 2020-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "notepad.h"

#include <strsafe.h>
#include <assert.h>

static VOID AlertPrintError(VOID)
{
    TCHAR szUntitled[MAX_STRING_LEN];

    LoadString(Globals.hInstance, STRING_UNTITLED, szUntitled, _countof(szUntitled));

    DIALOG_StringMsgBox(Globals.hMainWnd, STRING_PRINTERROR,
                        Globals.szFileName[0] ? Globals.szFileName : szUntitled,
                        MB_ICONEXCLAMATION | MB_OK);
}

static RECT
GetPrintingRect(IN HDC hdc, IN LPCRECT pMargins)
{
    INT iLogPixelsX = GetDeviceCaps(hdc, LOGPIXELSX);
    INT iLogPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
    INT iHorzRes = GetDeviceCaps(hdc, HORZRES); /* in pixels */
    INT iVertRes = GetDeviceCaps(hdc, VERTRES); /* in pixels */
    RECT rcPrintRect, rcPhysical;

#define CONVERT_X(x) MulDiv((x), iLogPixelsX, 2540) /* 100th millimeters to pixels */
#define CONVERT_Y(y) MulDiv((y), iLogPixelsY, 2540) /* 100th millimeters to pixels */
    SetRect(&rcPrintRect,
            CONVERT_X(pMargins->left), CONVERT_Y(pMargins->top),
            iHorzRes - CONVERT_X(pMargins->right),
            iVertRes - CONVERT_Y(pMargins->bottom));

    rcPhysical.left = GetDeviceCaps(hdc, PHYSICALOFFSETX);
    rcPhysical.right = rcPhysical.left + GetDeviceCaps(hdc, PHYSICALWIDTH);
    rcPhysical.top = GetDeviceCaps(hdc, PHYSICALOFFSETY);
    rcPhysical.bottom = rcPhysical.top + GetDeviceCaps(hdc, PHYSICALHEIGHT);

    /* Adjust the margin */
    rcPrintRect.left = max(rcPrintRect.left, rcPhysical.left);
    rcPrintRect.top = max(rcPrintRect.top, rcPhysical.top);
    rcPrintRect.right = min(rcPrintRect.right, rcPhysical.right);
    rcPrintRect.bottom = min(rcPrintRect.bottom, rcPhysical.bottom);

    return rcPrintRect;
}

static INT GetSelectionTextLength(HWND hWnd)
{
    DWORD dwStart = 0, dwEnd = 0;
    SendMessage(hWnd, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    return dwEnd - dwStart;
}

static INT GetSelectionText(HWND hWnd, LPTSTR lpString, INT nMaxCount)
{
    DWORD dwStart = 0, dwEnd = 0;
    INT cchText = GetWindowTextLength(hWnd);
    LPTSTR pszText;
    HLOCAL hLocal;
    HRESULT hr;

    SendMessage(hWnd, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    if (!lpString || dwStart == dwEnd || cchText == 0)
        return 0;

    hLocal = (HLOCAL)SendMessage(hWnd, EM_GETHANDLE, 0, 0);
    pszText = (LPTSTR)LocalLock(hLocal);
    if (!pszText)
        return 0;

    hr = StringCchCopyN(lpString, nMaxCount, pszText + dwStart, dwEnd - dwStart);
    LocalUnlock(hLocal);

    switch (hr)
    {
        case S_OK:
            return dwEnd - dwStart;

        case STRSAFE_E_INSUFFICIENT_BUFFER:
            return nMaxCount - 1;

        default:
            return 0;
    }
}

typedef struct
{
    PRINTDLG printer;
    HWND hwndDlg;
    INT status;
    INT currentPage;
    RECT printRect;
    SYSTEMTIME stNow;
    HFONT hHeaderFont;
    HFONT hBodyFont;
    LPTSTR pszText;
    DWORD ich;
    DWORD cchText;
    INT cyHeader;
    INT cySpacing;
    INT cyFooter;
} PRINT_DATA, *PPRINT_DATA;

/* Convert the points into pixels */
#define X_POINTS_TO_PIXELS(hDC, points) MulDiv((points), GetDeviceCaps((hDC), LOGPIXELSX), 72)
#define Y_POINTS_TO_PIXELS(hDC, points) MulDiv((points), GetDeviceCaps((hDC), LOGPIXELSY), 72)

/*
 * See also:
 * https://support.microsoft.com/en-us/windows/help-in-notepad-4d68c388-2ff2-0e7f-b706-35fb2ab88a8c
 */
static VOID
DrawHeaderOrFooter(HDC hDC, LPRECT pRect, LPCTSTR pszFormat, INT nPageNo, const SYSTEMTIME *pstNow)
{
    TCHAR szText[256], szField[128];
    const TCHAR *pchFormat;
    UINT uAlign = DT_CENTER, uFlags = DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX;
    HGDIOBJ hOldPen, hOldBrush;

    /* Draw a rectangle */
    hOldPen = SelectObject(hDC, GetStockObject(BLACK_PEN));
    hOldBrush = SelectObject(hDC, GetStockObject(NULL_BRUSH));
    Rectangle(hDC, pRect->left, pRect->top, pRect->right, pRect->bottom);
    SelectObject(hDC, hOldBrush);
    SelectObject(hDC, hOldPen);

    InflateRect(pRect, -X_POINTS_TO_PIXELS(hDC, 3), 0); /* Shrink 3pt */

    szText[0] = 0;

    for (pchFormat = pszFormat; *pchFormat; ++pchFormat)
    {
        if (*pchFormat != _T('&'))
        {
            StringCchCatN(szText, _countof(szText), pchFormat, 1);
            continue;
        }

        ++pchFormat;
        if (*pchFormat == 0)
            break;

        switch (_totupper(*pchFormat)) /* Make it uppercase */
        {
            case _T('&'): /* Found double ampersand */
                StringCchCat(szText, _countof(szText), TEXT("&"));
                break;

            case _T('L'): /* Left */
                DrawText(hDC, szText, -1, pRect, uAlign | uFlags);
                szText[0] = 0;
                uAlign = DT_LEFT;
                break;

            case _T('C'): /* Center */
                DrawText(hDC, szText, -1, pRect, uAlign | uFlags);
                szText[0] = 0;
                uAlign = DT_CENTER;
                break;

            case _T('R'): /* Right */
                DrawText(hDC, szText, -1, pRect, uAlign | uFlags);
                szText[0] = 0;
                uAlign = DT_RIGHT;
                break;

            case _T('D'): /* Date */
                GetDateFormat(LOCALE_USER_DEFAULT, 0, pstNow, NULL,
                              szField, (INT)_countof(szField));
                StringCchCat(szText, _countof(szText), szField);
                break;

            case _T('T'): /* Time */
                GetTimeFormat(LOCALE_USER_DEFAULT, 0, pstNow, NULL,
                              szField, (INT)_countof(szField));
                StringCchCat(szText, _countof(szText), szField);
                break;

            case _T('F'): /* Filename */
                StringCchCat(szText, _countof(szText), Globals.szFileTitle);
                break;

            case _T('P'): /* Page number */
                StringCchPrintf(szField, _countof(szField), TEXT("%u"), nPageNo);
                StringCchCat(szText, _countof(szText), szField);
                break;

            default: /* Otherwise */
                szField[0] = _T('&');
                szField[1] = *pchFormat;
                szField[2] = 0;
                StringCchCat(szText, _countof(szText), szField);
                break;
        }
    }

    DrawText(hDC, szText, -1, pRect, uAlign | uFlags);
}

static BOOL DoPrintBody(PPRINT_DATA pData, DWORD PageCount, BOOL bSkipPage)
{
    LPPRINTDLG pPrinter = &pData->printer;
    RECT printRect = pData->printRect;
    INT xLeft = printRect.left, yTop = printRect.top + pData->cyHeader + pData->cySpacing;
    INT xStart, tabWidth;
    DWORD ichStart;
    SIZE charMetrics;
    TEXTMETRIC tmText;

    /* Calculate a tab width */
#define TAB_STOP 8
    GetTextMetrics(pPrinter->hDC, &tmText);
    tabWidth = TAB_STOP * tmText.tmAveCharWidth;

#define DO_FLUSH() do { \
    if (ichStart < pData->ich && !bSkipPage) { \
        TextOut(pPrinter->hDC, xStart, yTop, &pData->pszText[ichStart], pData->ich - ichStart); \
    } \
    ichStart = pData->ich; \
    xStart = xLeft; \
    if (pData->status == STRING_PRINTCANCELING) return FALSE; \
} while (0)

    /* The drawing-body loop */
    for (ichStart = pData->ich, xStart = xLeft; pData->ich < pData->cchText; )
    {
        TCHAR ch = pData->pszText[pData->ich];

        if (ch == _T('\r'))
        {
            DO_FLUSH();

            pData->ich++; /* Next char */
            ichStart = pData->ich;
            continue;
        }

        if (ch == _T('\n'))
        {
            DO_FLUSH();

            /* Next line */
            yTop += tmText.tmHeight;
            xLeft = xStart = printRect.left;
        }
        else
        {
            if (ch == _T('\t'))
            {
                INT nStepWidth = tabWidth - ((xLeft - printRect.left) % tabWidth);

                DO_FLUSH();

                /* Go to the next tab stop */
                xLeft += nStepWidth;
                xStart = xLeft;
            }
            else /* Normal char */
            {
                GetTextExtentPoint32(pPrinter->hDC, &ch, 1, &charMetrics);
                xLeft += charMetrics.cx;
            }

            /* Insert a line break if the next position reached the right edge */
            if (xLeft + charMetrics.cx >= printRect.right)
            {
                if (ch != _T('\t'))
                    DO_FLUSH();

                /* Next line */
                yTop += tmText.tmHeight;
                xLeft = xStart = printRect.left;
            }
        }

        pData->ich++; /* Next char */
        if (ch == _T('\t') || ch == _T('\n'))
            ichStart = pData->ich;

        if (yTop + tmText.tmHeight >= printRect.bottom - pData->cyFooter)
            break; /* The next line reached the body bottom */
    }

    DO_FLUSH();
    return TRUE;
}

static BOOL DoPrintPage(PPRINT_DATA pData, DWORD PageCount)
{
    LPPRINTDLG pPrinter = &pData->printer;
    BOOL bSkipPage, ret;
    HFONT hOldFont;

    /* Should we skip this page? */
    bSkipPage = !(pPrinter->Flags & PD_SELECTION) &&
                (pPrinter->Flags & PD_PAGENUMS) &&
                !(pPrinter->nFromPage <= PageCount && PageCount <= pPrinter->nToPage);

    /* The prologue of a page */
    if (!bSkipPage)
    {
        if (StartPage(pPrinter->hDC) <= 0)
        {
            pData->status = STRING_PRINTFAILED;
            return FALSE;
        }

        if (pData->cyHeader > 0)
        {
            /* Draw the page header */
            RECT rc = pData->printRect;
            rc.bottom = rc.top + pData->cyHeader;

            hOldFont = SelectObject(pPrinter->hDC, pData->hHeaderFont);
            DrawHeaderOrFooter(pPrinter->hDC, &rc, Globals.szHeader, PageCount, &pData->stNow);
            SelectObject(pPrinter->hDC, hOldFont); /* De-select the font */
        }
    }

    hOldFont = SelectObject(pPrinter->hDC, pData->hBodyFont);
    ret = DoPrintBody(pData, PageCount, bSkipPage);
    SelectObject(pPrinter->hDC, hOldFont);
    if (!ret)
        return FALSE; /* Canceled */

    /* The epilogue of a page */
    if (!bSkipPage)
    {
        if (pData->cyFooter > 0)
        {
            /* Draw the page footer */
            RECT rc = pData->printRect;
            rc.top = rc.bottom - pData->cyFooter;

            hOldFont = SelectObject(pPrinter->hDC, pData->hHeaderFont);
            DrawHeaderOrFooter(pPrinter->hDC, &rc, Globals.szFooter, PageCount, &pData->stNow);
            SelectObject(pPrinter->hDC, hOldFont);
        }

        if (EndPage(pPrinter->hDC) <= 0)
        {
            pData->status = STRING_PRINTFAILED;
            return FALSE;
        }
    }

    return TRUE;
}

#define BODY_FONT_SIZE      10 /* 10pt */
#define HEADER_FONT_SIZE    9  /* 9pt */
#define SPACING_HEIGHT      4  /* 4pt */
#define PRINTING_MESSAGE (WM_USER + 100)

static BOOL DoCreatePrintFonts(LPPRINTDLG pPrinter, PPRINT_DATA pPrintData)
{
    LOGFONT lfBody, lfHeader;

    /* Create the main text font for printing */
    lfBody = Globals.lfFont;
    lfBody.lfHeight = -Y_POINTS_TO_PIXELS(pPrinter->hDC, BODY_FONT_SIZE);
    pPrintData->hBodyFont = CreateFontIndirect(&lfBody);
    if (pPrintData->hBodyFont == NULL)
        return FALSE;

    /* Create the header/footer font */
    lfHeader = Globals.lfFont;
    lfHeader.lfHeight = -Y_POINTS_TO_PIXELS(pPrinter->hDC, HEADER_FONT_SIZE);
    lfHeader.lfWeight = FW_BOLD;
    pPrintData->hHeaderFont = CreateFontIndirect(&lfHeader);
    if (pPrintData->hHeaderFont == NULL)
        return FALSE;

    return TRUE;
}

static BOOL DoPrintDocument(PPRINT_DATA printData)
{
    DOCINFO docInfo;
    LPPRINTDLG pPrinter = &printData->printer;
    DWORD CopyCount, PageCount;
    TEXTMETRIC tmHeader;
    BOOL ret = FALSE;
    HFONT hOldFont;

    GetLocalTime(&printData->stNow);

    printData->printRect = GetPrintingRect(pPrinter->hDC, &Globals.lMargins);

    if (!DoCreatePrintFonts(pPrinter, printData))
    {
        printData->status = STRING_PRINTFAILED;
        goto Quit;
    }

    if (pPrinter->Flags & PD_SELECTION)
        printData->cchText = GetSelectionTextLength(Globals.hEdit);
    else
        printData->cchText = GetWindowTextLength(Globals.hEdit);

    /* Allocate a buffer for the text */
    printData->pszText = HeapAlloc(GetProcessHeap(), 0, (printData->cchText + 1) * sizeof(TCHAR));
    if (!printData->pszText)
    {
        printData->status = STRING_PRINTFAILED;
        goto Quit;
    }

    if (pPrinter->Flags & PD_SELECTION)
        GetSelectionText(Globals.hEdit, printData->pszText, printData->cchText + 1);
    else
        GetWindowText(Globals.hEdit, printData->pszText, printData->cchText + 1);

    /* Start a document */
    ZeroMemory(&docInfo, sizeof(docInfo));
    docInfo.cbSize = sizeof(DOCINFO);
    docInfo.lpszDocName = Globals.szFileTitle;
    if (StartDoc(pPrinter->hDC, &docInfo) <= 0)
    {
        printData->status = STRING_PRINTFAILED;
        goto Quit;
    }

    /* Calculate the header and footer heights */
    hOldFont = SelectObject(pPrinter->hDC, printData->hHeaderFont);
    GetTextMetrics(pPrinter->hDC, &tmHeader);
    printData->cyHeader = printData->cyFooter = 2 * tmHeader.tmHeight;
    printData->cySpacing = Y_POINTS_TO_PIXELS(pPrinter->hDC, SPACING_HEIGHT);
    SelectObject(pPrinter->hDC, hOldFont); /* De-select the font */
    if (!Globals.szHeader[0])
        printData->cyHeader = printData->cySpacing = 0;
    if (!Globals.szFooter[0])
        printData->cyFooter = 0;

    /* The printing-copies loop */
    for (CopyCount = 1; CopyCount <= pPrinter->nCopies; ++CopyCount)
    {
        /* The printing-pages loop */
        for (PageCount = 1, printData->ich = 0; printData->ich < printData->cchText; ++PageCount)
        {
            printData->currentPage = PageCount;
            PostMessage(printData->hwndDlg, PRINTING_MESSAGE, 0, 0);

            if (!DoPrintPage(printData, PageCount))
            {
                AbortDoc(pPrinter->hDC); /* Cancel printing */
                goto Quit;
            }
        }
    }

    if (EndDoc(pPrinter->hDC) <= 0)
    {
        printData->status = STRING_PRINTFAILED;
        goto Quit;
    }

    ret = TRUE;
    printData->status = STRING_PRINTCOMPLETE;

Quit:
    DeleteObject(printData->hHeaderFont);
    DeleteObject(printData->hBodyFont);
    if (printData->pszText)
        HeapFree(GetProcessHeap(), 0, printData->pszText);
    if (printData->status == STRING_PRINTCANCELING)
        printData->status = STRING_PRINTCANCELED;
    PostMessage(printData->hwndDlg, PRINTING_MESSAGE, 0, 0);
    return ret;
}

static DWORD WINAPI PrintThreadFunc(LPVOID arg)
{
    PPRINT_DATA pData = arg;
    pData->currentPage = 1;
    pData->status = STRING_NOWPRINTING;
    PostMessage(pData->hwndDlg, PRINTING_MESSAGE, 0, 0);
    return DoPrintDocument(pData);
}

static INT_PTR CALLBACK
DIALOG_Printing_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szText[MAX_STRING_LEN];
    static TCHAR s_szPage[64];
    static PPRINT_DATA s_pData = NULL;
    static HANDLE s_hThread = NULL;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            s_pData = (PPRINT_DATA)lParam;
            s_pData->hwndDlg = hwnd;
            SetDlgItemText(hwnd, IDC_PRINTING_FILENAME, Globals.szFileTitle);
            GetDlgItemText(hwnd, IDC_PRINTING_PAGE, s_szPage, _countof(s_szPage));
            SetDlgItemText(hwnd, IDC_PRINTING_PAGE, NULL);

            s_hThread = CreateThread(NULL, 0, PrintThreadFunc, s_pData, 0, NULL);
            if (!s_hThread)
            {
                s_pData->status = STRING_PRINTFAILED;
                EndDialog(hwnd, IDABORT);
            }
            return TRUE;

        case PRINTING_MESSAGE:
            switch (s_pData->status)
            {
                case STRING_NOWPRINTING:
                case STRING_PRINTCANCELING:
                    StringCchPrintf(szText, _countof(szText), s_szPage, s_pData->currentPage);
                    SetDlgItemText(hwnd, IDC_PRINTING_PAGE, szText);

                    LoadString(Globals.hInstance, s_pData->status, szText, _countof(szText));
                    SetDlgItemText(hwnd, IDC_PRINTING_STATUS, szText);
                    break;

                case STRING_PRINTCOMPLETE:
                case STRING_PRINTCANCELED:
                case STRING_PRINTFAILED:
                    LoadString(Globals.hInstance, s_pData->status, szText, _countof(szText));
                    SetDlgItemText(hwnd, IDC_PRINTING_STATUS, szText);

                    if (s_pData->status == STRING_PRINTCOMPLETE)
                        EndDialog(hwnd, IDOK);
                    else if (s_pData->status == STRING_PRINTFAILED)
                        EndDialog(hwnd, IDABORT);
                    else
                        EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL && s_pData->status == STRING_NOWPRINTING)
            {
                EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
                s_pData->status = STRING_PRINTCANCELING;
                PostMessage(hwnd, PRINTING_MESSAGE, 0, 0);
            }
            break;

        case WM_DESTROY:
            if (s_hThread)
                CloseHandle(s_hThread);
            DeleteDC(s_pData->printer.hDC);
            s_pData = LocalFree(s_pData);
            break;
    }

    return 0;
}

VOID DIALOG_FilePrint(VOID)
{
    BOOL ret;
    LPPRINTDLG printer;
    PPRINT_DATA printData = LocalAlloc(LPTR, sizeof(PRINT_DATA));
    if (!printData)
    {
        ShowLastError();
        return;
    }

    printer = &printData->printer;
    printer->lStructSize = sizeof(PRINTDLG);
    printer->hwndOwner = Globals.hMainWnd;
    printer->Flags = PD_RETURNDC | PD_SELECTION;

    /* Disable the selection radio button if there is no text selected */
    if (!GetSelectionTextLength(Globals.hEdit))
        printer->Flags |= PD_NOSELECTION;

    printer->nFromPage = 1;
    printer->nToPage = MAXWORD;
    printer->nMinPage = 1;
    printer->nMaxPage = MAXWORD;

    printer->hDevMode = Globals.hDevMode;
    printer->hDevNames = Globals.hDevNames;

    ret = PrintDlg(printer);
    /* NOTE: Even if PrintDlg returns FALSE, hDevMode and hDevNames may have changed. */
    Globals.hDevMode = printer->hDevMode;
    Globals.hDevNames = printer->hDevNames;

    if (!ret)
    {
        LocalFree(printData);
        return; /* The user canceled printing */
    }
    assert(printer->hDC != NULL);

    /* Ensure that each logical unit maps to one pixel */
    SetMapMode(printer->hDC, MM_TEXT);

    if (DialogBoxParam(Globals.hInstance,
                       MAKEINTRESOURCE(DIALOG_PRINTING),
                       Globals.hMainWnd,
                       DIALOG_Printing_DialogProc,
                       (LPARAM)printer) == IDABORT)
    {
        AlertPrintError();
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *           DIALOG_PAGESETUP_Hook
 */
static UINT_PTR CALLBACK DIALOG_PAGESETUP_Hook(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            /* fetch last user input prior to display dialog */
            SetDlgItemText(hDlg, 0x141, Globals.szHeader);
            SetDlgItemText(hDlg, 0x143, Globals.szFooter);
            break;

        case WM_COMMAND:
        {
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                case IDOK:
                    /* save user input and close dialog */
                    GetDlgItemText(hDlg, 0x141, Globals.szHeader, _countof(Globals.szHeader));
                    GetDlgItemText(hDlg, 0x143, Globals.szFooter, _countof(Globals.szFooter));
                    return FALSE;

                case IDCANCEL:
                    /* discard user input and close dialog */
                    return FALSE;

                case IDHELP:
                    {
                        /* FIXME: Bring this to work */
                        static const TCHAR sorry[] = _T("Sorry, no help available");
                        static const TCHAR help[] = _T("Help");
                        MessageBox(Globals.hMainWnd, sorry, help, MB_ICONEXCLAMATION);
                        return TRUE;
                    }

                default:
                    break;
                }
            }
            break;
        }
    }

    return FALSE;
}

/***********************************************************************
 *           DIALOG_FilePageSetup
 */
VOID DIALOG_FilePageSetup(VOID)
{
    PAGESETUPDLG page;

    ZeroMemory(&page, sizeof(page));
    page.lStructSize = sizeof(page);
    page.hwndOwner = Globals.hMainWnd;
    page.Flags = PSD_ENABLEPAGESETUPTEMPLATE | PSD_ENABLEPAGESETUPHOOK | PSD_MARGINS;
    page.hInstance = Globals.hInstance;
    page.rtMargin = Globals.lMargins;
    page.hDevMode = Globals.hDevMode;
    page.hDevNames = Globals.hDevNames;
    page.lpPageSetupTemplateName = MAKEINTRESOURCE(DIALOG_PAGESETUP);
    page.lpfnPageSetupHook = DIALOG_PAGESETUP_Hook;

    PageSetupDlg(&page);

    /* NOTE: Even if PageSetupDlg returns FALSE, the following members may have changed */
    Globals.hDevMode = page.hDevMode;
    Globals.hDevNames = page.hDevNames;
    Globals.lMargins = page.rtMargin;
}
