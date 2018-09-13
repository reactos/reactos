#include "precomp.h"

#define MAXPROFILELEN   60

// leading - non-zero looks a lot better
#define LEADING (max(ExtPrintLeading,yPrintChar/4))

// offset to start of body of card
#define BODYOFFSET (yPrintChar+LEADING+2*LINEWIDTH +10*LINEWIDTH)

// offset from left of card to print title or body of text
#define XTEXTOFFSET (xPrintChar/2)

// width of lines in pixels
#define LINEWIDTH 1

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

#if DBG
TCHAR dbuf[100];
VOID PrintLogFont( LOGFONT lf )
{
    wsprintf(dbuf,TEXT("lfHeight          %d\n"), lf.lfHeight        ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfWidth           %d\n"), lf.lfWidth         ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfEscapement      %d\n"), lf. lfEscapement   ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfOrientation     %d\n"), lf.lfOrientation   ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfWeight          %d\n"), lf.lfWeight        ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfItalic          %d\n"), lf.lfItalic        ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfUnderline       %d\n"), lf.lfUnderline     ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfStrikeOut       %d\n"), lf.lfStrikeOut     ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfCharSet         %d\n"), lf.lfCharSet       ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfOutPrecision    %d\n"), lf.lfOutPrecision  ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfClipPrecison    %d\n"), lf.lfClipPrecision ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfQuality         %d\n"), lf.lfQuality       ); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfPitchAndFamily  %d\n"), lf.lfPitchAndFamily); ODS(dbuf);
    wsprintf(dbuf,TEXT("lfFaceName        %s\n"), lf.lfFaceName      ); ODS(dbuf);
}
#endif

NOEXPORT void NEAR PrintHeaderFooter (HDC hDC, SHORT i);

NOEXPORT void GetDateTime(TCHAR *szTime, TCHAR *szDate);

TCHAR DefaultNullStr[] = TEXT("");

HWND hAbortDlgWnd;
INT fAbort;
INT bError;
INT iTabSize;

INT xPrintChar;         /* width of char on printer */
INT yPrintChar;         /* height of char on printer */
INT xPrintCard;         /* width of card on printer */
INT yPrintCard;         /* height of card on printer */
INT yCardSpace;         /* Space between card titles */

INT ExtPrintLeading;
INT xHeadFoot;
BOOL bCenter, bRight;
INT xPrintRes;
INT yPrintRes;
INT yPixelsPerInch;        /* pixels/inch */
INT xPixelsPerInch;        /* pixels/inch */


/* Flag if printer setup was done. */
BOOL bPrinterSetupDone=FALSE;
/*
 * all of these are in device units (for printer) and are calculated
 * by SetupPrinting()
 */

/* Had to use HEAD since HEADER already has another def.  */
#define HEAD   0
#define FOOTER 1

int dyTop;        /* width of top border */
int dyBottom;        /* width of bottom border */
int dxLeft;        /* width of left border */
int dxRight;        /* width of right border (this doesn't get used) */
int dyHeadFoot;        /* height from top/bottom of headers and footers */

int iPageNum;        /* global page number currently being printed */
int xLeftSpace, xRightSpace; /* Space of margins */
INT xCharPage;

FARPROC lpfnAbortProc;
FARPROC lpfnAbortDlgProc;
FARPROC lpfnPageDlgProc;

/* We'll dynamically allocate this */
HANDLE hHeadFoot=NULL;
LPTSTR  szHeadFoot;

void NEAR FreePrintHandles()
{
    if(PD.hDevMode)
        GlobalFree(PD.hDevMode);
    if(PD.hDevNames)
        GlobalFree(PD.hDevNames);
    PD.hDevMode = PD.hDevNames = NULL;
    bPrinterSetupDone = FALSE;
}

/* Get dafault printer data using the commdlg code for printer setup. */
BOOL NEAR GetDefaultPrinter()
{
    FreePrintHandles();

    PD.lStructSize     = sizeof(PRINTDLG);
    PD.Flags         = PD_PRINTSETUP|PD_RETURNDEFAULT;
    if (PrintDlg((LPPRINTDLG)&PD))
    {
        bPrinterSetupDone = TRUE;
        return TRUE;
    }
    else
    {
        FreePrintHandles();
        return FALSE;
    }
}

/* Call the commdlg code for printer setup. */
void PrinterSetupDlg(HWND hwnd)
{
    BOOL bTryAgain = (PD.hDevMode || PD.hDevNames);
    DWORD dwErr;

    LockData(0);
    PD.Flags = PD_PRINTSETUP;    /* invoke only the Setup dialog */

TryPrintSetupAgain:
    bPrinterSetupDone |= PrintDlg(&PD);

    /* set szPrinter and free hDevMode and hDevNames */
    /* PrintDlg error. */
    if(dwErr = CommDlgExtendedError()) /* Re-initialize the PD structure. */
    {
        PD.lStructSize    = sizeof(PRINTDLG);
        PD.hwndOwner      = hwnd;
        PD.hDC            = NULL;
        PD.nCopies        = 1;
        FreePrintHandles();
        if (bTryAgain)
        {
            bTryAgain = FALSE;
            goto TryPrintSetupAgain;
        }
        else if (dwErr != PDERR_NODEFAULTPRN)
        {
            TCHAR szError[256];

            LoadString(hIndexInstance, E_PRINT_SETUP_ERROR, szError, CharSizeOf(szError));
            MessageBox(hwnd, szError, szCardfile, MB_OK | MB_ICONHAND);
        }
    }
    UnlockData(0);
}

/*
 * convert floating point strings (like 2.75 1.5 2) into number of pixels
 * given the number of pixels per inch
 */
INT atopix(
    TCHAR *ptr,
    INT pix_per_in)
{
    TCHAR *dot_ptr;
    TCHAR sz[20];
    INT decimal;

    lstrcpy(sz, ptr);
    dot_ptr = _tcschr(sz, szDec[0]);
    if (dot_ptr)
    {
        *dot_ptr++ = 0;        /* terminate the inches */
        if (*(dot_ptr + 1) == (TCHAR) 0)
        {
            *(dot_ptr + 1) = TEXT('0');   /* convert decimal part to hundredths */
            *(dot_ptr + 2) = (TCHAR) 0;
        }
        decimal = ((int)MyAtol(dot_ptr) * pix_per_in) / 100;    /* first part */
    }
    else
        decimal = 0;        /* there is not fraction part */

    return ((INT)MyAtol(sz) * pix_per_in) + decimal;     /* second part */
}

HDC GetPrinterDC(
    void)
{
    LPDEVMODE lpDevMode;
    LPDEVNAMES lpDevNames;

    if (!bPrinterSetupDone) /* Retrieve default printer if none selected. */
    {
        if (!GetDefaultPrinter())
            return NULL;
    }

    lpDevNames  = (LPDEVNAMES)GlobalLock(PD.hDevNames);
    if (PD.hDevMode)
        lpDevMode = (LPDEVMODE)GlobalLock(PD.hDevMode);
    else
        lpDevMode = NULL;

    /* For pre 3.0 drivers lpDevMode will be null as these drivers don't
     * use this structure.
     */
    PD.hDC = CreateDC(((LPTSTR)lpDevNames)+lpDevNames->wDriverOffset,
                      ((LPTSTR)lpDevNames)+lpDevNames->wDeviceOffset,
                      ((LPTSTR)lpDevNames)+lpDevNames->wOutputOffset,
                      lpDevMode);
    GlobalUnlock(PD.hDevNames);
    if (PD.hDevMode)
        GlobalUnlock(PD.hDevMode);

    return PD.hDC;
}

HDC SetupPrintingFailed(
    HDC hPrintDC)
{
    if (hPrintDC)
        DeleteDC(hPrintDC);
    IndexOkError(ECANTPRINT);
    return NULL;
}

/*
 * setup the printer, return it's DC and create printing (abort)
 * dialog box.
 */
HDC SetupPrinting( BOOL bUseFont )
{
    TCHAR      buf[80];
    DWORD      nSpace;
    HDC        hPrintDC;
    TEXTMETRIC Metrics;
    DOCINFO    DocInfo;
    INT        iErr;
    HFONT      hPrintFont;       // font in print DC
    HFONT      hPrevFont;        // previous font in print DC

    /*
     * On second print job, this was getting used
     * before the dialog was created for the second time.
     */
    hAbortDlgWnd = (HWND)NULL;

    if (!(hPrintDC = GetPrinterDC()))
        return SetupPrintingFailed(hPrintDC);

    xPrintRes       = GetDeviceCaps(hPrintDC, HORZRES);
    yPrintRes       = GetDeviceCaps(hPrintDC, VERTRES);
    xPixelsPerInch  = GetDeviceCaps(hPrintDC, LOGPIXELSX);
    yPixelsPerInch  = GetDeviceCaps(hPrintDC, LOGPIXELSY);


    // if need be, use user defined font in printer


    if( bUseFont )
    {
        LOGFONT lf;            // temp version of FontStruct

        lf= FontStruct; 
        lf.lfHeight= - (iPointSize * yPixelsPerInch ) / (72*10);
        lf.lfWidth= 0;   // let font mapper figure it out
        lf.lfQuality= DEFAULT_QUALITY;
        hPrintFont = CreateFontIndirect(&lf);

        if( hPrintFont )
        {
            //PrintLogFont( lf );
            hPrevFont= SelectObject( hPrintDC, hPrintFont );
            DeleteObject( hPrevFont );
        }
        // if not successful, just use default font
    }

    GetTextMetrics(hPrintDC, &Metrics);
    yPrintChar = Metrics.tmHeight + Metrics.tmExternalLeading;
    ExtPrintLeading = Metrics.tmExternalLeading;
    xPrintChar = Metrics.tmAveCharWidth;    /* character width */
    iTabSize = xPrintChar*8;

    dyHeadFoot = yPixelsPerInch / 2;                  /* 1/2 an inch */
    dyTop      = atopix(chPageText[4], yPixelsPerInch);
    dyBottom   = atopix(chPageText[5], yPixelsPerInch);
    dxLeft     = atopix(chPageText[2], xPixelsPerInch);
    dxRight    = atopix(chPageText[3], xPixelsPerInch);

#if !defined(WIN32)
    nSpace = GetTextExtent(hPrintDC, TEXT(" "), 1 )
#else
    {
      SIZE  sz ;

      GetTextExtentPoint(hPrintDC , TEXT(" "), 1, &sz);

      nSpace = sz.cx;
    }
#endif
    xLeftSpace  = dxLeft / nSpace;
    xRightSpace = dxRight / nSpace;

    /* Number of characters between margins */
    xCharPage = (xPrintRes / xPrintChar) - xLeftSpace - xRightSpace;

    /* Allocate memory for the header.footer string.  Will allow any size
     * of paper and still have enough for the string.
     */
    hHeadFoot = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, ByteCountOf(xCharPage+2));

    if (!hHeadFoot)
        return SetupPrintingFailed (hPrintDC);

    SetAbortProc (hPrintDC, fnAbortProc);

    BuildCaption (buf, CharSizeOf(buf));

    /* Gotta disable the window before doing the start doc so that the user
     * can't quickly do multiple prints.
     */
    EnableWindow (hIndexWnd, FALSE);

    DocInfo.cbSize = sizeof (DOCINFO);
    DocInfo.lpszDocName  = buf;
    DocInfo.lpszOutput   = NULL;
    DocInfo.lpszDatatype = NULL;
    DocInfo.fwType       = 0;

    if ((iErr = StartDoc (hPrintDC, &DocInfo)) < 0)
    {
        DeleteDC (hPrintDC);
        EnableWindow (hIndexWnd, TRUE);
        if (iErr == SP_USERABORT)
            SendMessage (hIndexWnd, WM_SETFOCUS, 0, 0);
        else
            IndexOkError (ECANTPRINT);
        return NULL;
    }

    bError = FALSE;
    fAbort = FALSE;
    hAbortDlgWnd = CreateDialog(hIndexInstance, (LPTSTR) DTABORTDLG,
                                hIndexWnd, (DLGPROC)fnAbortDlgProc);
    if (!hAbortDlgWnd)
    {
        EnableWindow(hIndexWnd, TRUE);
        return SetupPrintingFailed(hPrintDC);
    }
    return hPrintDC;
}

void FinishPrinting (HDC hPrintDC)
{
    if (!fAbort)
    {
        if (!bError)
            EndDoc(hPrintDC);
        EnableWindow (hIndexWnd, TRUE);
        DestroyWindow (hAbortDlgWnd);
    }
    DeleteDC (hPrintDC);
    if (hHeadFoot)
        GlobalFree (hHeadFoot);
    hHeadFoot = NULL;
}


/*
 * print out the translated header/footer string in proper position.
 *
 * uses global stuff like xPrintChar, dyHeadFoot...
 */
NOEXPORT void NEAR PrintHeaderFooter (HDC hDC,SHORT nHF)
{
    TCHAR    buf[80];
    SHORT   len;

    lstrcpy (buf, chPageText[nHF]);

    szHeadFoot = GlobalLock (hHeadFoot);
    len = TranslateString (buf);

    if (*szHeadFoot)
    {
        if (nHF == HEAD)
        {
            TabbedTextOut (hDC, dxLeft, dyHeadFoot - yPrintChar, szHeadFoot,
                           len, 1, &iTabSize, dxLeft);
        }
        else
        {
            TabbedTextOut (hDC, dxLeft, yPrintRes-yPrintChar-dyHeadFoot,
                           szHeadFoot, len, 1, &iTabSize, dxLeft);
        }
    }
    GlobalUnlock (hHeadFoot);
}

void PrintList (void)
{
    HDC hPrintDC;
    INT curcard;
    INT i, y, Start, End;
    INT cCardsPerPage;
    LPCARDHEADER Cards;
    INT iError;


    hPrintDC = SetupPrinting( TRUE );
    if (!hPrintDC)
    {
        if (hHeadFoot)
        {
            GlobalFree (hHeadFoot);
            hHeadFoot = NULL;
        }
        return;
    }

    cCardsPerPage = max (1, (yPrintRes - dyTop - dyBottom) / yPrintChar);

    iPageNum = 1;

    Cards = (LPCARDHEADER) GlobalLock (hCards);

    y = dyTop - yPrintChar;

    for (curcard = 0; curcard < cCards; )
    {
        iError = StartPage(hPrintDC);
        if (iError < 0)
        {
            PrintError (iError);
            break;
        }

        PrintHeaderFooter (hPrintDC, HEAD);

        Start = curcard;
        End = curcard + cCardsPerPage;
        if (End > cCards)
            End = cCards;
        for (i = Start; i < End; ++i, curcard++)
        {
            TabbedTextOut (hPrintDC, dxLeft, y, Cards[i].line,
                           lstrlen(Cards[i].line), 1, &iTabSize, dxLeft);
            y += yPrintChar;
        }

        PrintHeaderFooter (hPrintDC, FOOTER);
        iPageNum++;

        /* Reset y so printing starts at top again. */
        y = dyTop - yPrintChar;

        iError = EndPage(hPrintDC);
        if (iError < 0)
        {
            PrintError (iError);
            break;
        }
        if (fAbort)
            break;
    }
    GlobalUnlock (hCards);
    FinishPrinting (hPrintDC);
}

void PrintCards (INT count)
{
    HDC          hPrintDC;
    INT          curcard;
    INT          i;
    INT          y;
    INT          cCardsPerPage;
    CARDHEADER   CardHead;
    LPCARDHEADER Cards;
    CARD         Card;
    HDC          hMemoryDC;
    HWND         hPrintWnd;
    HANDLE       hOldObject;
    INT          fPictureWarning;
    INT          iError;
    TEXTMETRIC   Metrics;

    hPrintWnd = CreateWindow(TEXT("Edit"), NULL,
        WS_CHILD | ES_MULTILINE,
        0, 0, (LINELENGTH * CharFixWidth) + 1, CARDLINES * CharFixHeight,
        hIndexWnd, NULL, hIndexInstance, NULL);

    /* Set fixed pitched font to this edit control, so that the text does
     * not get chopped off at the right when we print, because we use
     * fixed pitched font;
     * Fix for Bug #3760 --SANKAR-- 02-22-90
     */
    if (hPrintWnd)
        SendMessage(hPrintWnd, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));

    if (!hPrintWnd)
    {
        IndexOkError(EINSMEMORY);
        if (hHeadFoot)
        {
            GlobalFree(hHeadFoot);
            hHeadFoot=NULL;
        }
        return;
    }

    if (!(hPrintDC = SetupPrinting( TRUE )))
    {
        if (hHeadFoot)
        {
            GlobalFree(hHeadFoot);
            hHeadFoot = NULL;
        }
        return;
    }

    //            length of text + offset on each end + line width
    xPrintCard = (LINELENGTH * xPrintChar) + XTEXTOFFSET*2 + 2*LINEWIDTH;

    //           start of body + size of body
    yPrintCard = BODYOFFSET + (CARDLINES * yPrintChar);
    yCardSpace = yPixelsPerInch/4;

    hOldObject = SelectObject(hPrintDC, GetStockObject(HOLLOW_BRUSH));

    hMemoryDC = CreateCompatibleDC(hPrintDC);
    fPictureWarning = FALSE;

    if (count == 1)
    {
        iPageNum = 1;
        if (!hMemoryDC && CurCard.lpObject)
            IndexOkError (ENOPICTURES);

        if ((iError = StartPage (hPrintDC)) < 0)
            PrintError (iError);

        PrintHeaderFooter (hPrintDC, HEAD);
        PrintCurCard (hPrintDC, hMemoryDC, dxLeft, dyTop, &CurCardHead, &CurCard, hEditWnd);
        PrintHeaderFooter (hPrintDC, FOOTER);

        if ((iError = EndPage (hPrintDC)) < 0)
            PrintError (iError);
    }
    else
    {
        iPageNum = 1;

        // extra yCardSpace in case it spills to next page during card spacing

        cCardsPerPage = max(1, (yPrintRes - dyTop - dyBottom + yCardSpace) / (yPrintCard + yCardSpace));

        for (curcard = 0; curcard < count; )
        {
            if ((iError = StartPage (hPrintDC)) < 0)
                PrintError (iError);

            PrintHeaderFooter (hPrintDC, HEAD);
            y = dyTop;
            for (i = 0; i < cCardsPerPage && curcard < count; ++i)
            {
                if (curcard != iFirstCard)
                {
                    Cards = (LPCARDHEADER) GlobalLock(hCards);
                    CardHead = Cards[curcard];
                    GlobalUnlock(hCards);
                    if (!ReadCurCardData(&CardHead, &Card, szText))
                        IndexOkError(ECANTPRINTPICT);
                }
                else
                {
                    CardHead = CurCardHead;
                    Card = CurCard;
                    GetWindowText(hEditWnd, szText, CARDTEXTSIZE);
                }
                SetWindowText(hPrintWnd, szText);
                if (!hMemoryDC && Card.lpObject && !fPictureWarning)
                {
                    fPictureWarning++;
                    IndexOkError(ENOPICTURES);
                }
                PrintCurCard(hPrintDC, hMemoryDC, dxLeft, y, &CardHead, &Card, hPrintWnd);

                if (curcard != iFirstCard && Card.lpObject)
                    PicDelete(&Card);

                y += yPrintCard + yCardSpace;
                curcard++;
            }

            PrintHeaderFooter(hPrintDC, FOOTER);

            iPageNum++;

            if ((iError = EndPage (hPrintDC)) < 0)
            {
                PrintError (iError);
                break;
            }
            if (fAbort)
                break;

        }
    }

    DestroyWindow (hPrintWnd);
    SelectObject (hPrintDC, hOldObject);
    FinishPrinting (hPrintDC);
    if (hMemoryDC)
        DeleteDC (hMemoryDC);
}

void PrintCurCard(
    HDC hPrintDC,
    HDC hMemoryDC,
    INT xPos,                    // x offset of upper left
    INT yPos,                    // y offset of upper left
    PCARDHEADER pCardHead,
    PCARD   pCard,
    HWND    hWnd)
{
    INT y;
    HANDLE hOldObject;
    INT level;
    INT i;
    INT cLines;
    TCHAR buf[LINELENGTH];
    INT cch;

    // Draw the card outline

    Rectangle(hPrintDC,
              max( 0, xPos ),
              max( 0, yPos ),
              xPos + xPrintCard,
              yPos + yPrintCard);

    // draw a narrow (LINEWIDTH) box separating title from body of card

    Rectangle(hPrintDC,
              max( 0, xPos ),
              yPos + yPrintChar + LEADING,
              xPos + xPrintCard,
              yPos + BODYOFFSET - LINEWIDTH );

    // Draw the card title

    SetBkMode(hPrintDC, TRANSPARENT);
    TabbedTextOut(hPrintDC,
                  xPos + XTEXTOFFSET,
                  yPos + (LEADING/2),  // center top to bottom
                  pCardHead->line,
                  lstrlen(pCardHead->line),
                  1,
                  &iTabSize,
                  xPos+XTEXTOFFSET);

    // Print any object embedded in card! (wonder what AVI objects do?)

    if (pCard->lpObject && hMemoryDC)
    {
        HBITMAP hBitmap;
        BITMAP  bm;

        level = SaveDC(hPrintDC);

        IntersectClipRect(hPrintDC,
                          xPos + 1,
                          yPos + BODYOFFSET,
                          xPos + xPrintCard-1,
                          yPos + yPrintCard-1);

        /* Get a bitmap to print */
        if (GetObjectType( pCard->lpObject) != OBJ_BITMAP)
        {
            hBitmap = MakeObjectCopy(pCard, hPrintDC );
        }
        else
        {
            hBitmap = (HANDLE)pCard->lpObject ;
        }

        GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bm);
        hOldObject = SelectObject(hMemoryDC, hBitmap);


        if (!StretchBlt(hPrintDC,
            xPos + (pCard->rcObject.left * xPrintChar) / CharFixWidth,
            yPos + BODYOFFSET + (pCard->rcObject.top * yPrintChar) / CharFixHeight,
            (bm.bmWidth * xPrintChar) / CharFixWidth,
            (bm.bmHeight * yPrintChar) / CharFixHeight,
            hMemoryDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY))
        {
            IndexOkError(ECANTPRINTPICT);
        }

        SelectObject(hMemoryDC, hOldObject);
        RestoreDC(hPrintDC, level);

        if (GetObjectType( pCard->lpObject) != OBJ_BITMAP)
             DeleteObject(hBitmap);
    }

    /* draw the text */
    /* we should really be using DrawText and EM_GETHANDLE */

    y = yPos + BODYOFFSET;
    cLines = SendMessage(hWnd, EM_GETLINECOUNT, 0, 0L);
    for (i = 0; i < cLines; ++i)
    {
        buf[0] = LINELENGTH;
        buf[1] = 0;
        cch = (INT)SendMessage(hWnd, EM_GETLINE, i, (LPARAM)buf);
        TabbedTextOut(hPrintDC, xPos + 1, y, buf, cch,
                1, (LPINT)&iTabSize, xPos + 1);
        y += yPrintChar;
    }
}

int fnAbortProc(
    HDC hPrintDC,       /* what is this useless parameter? */
    int iReserved)      /* and this one? Good question! */
{
    MSG msg;

    while (!fAbort && PeekMessage (&msg, NULL, 0, 0, TRUE))
    if (!hAbortDlgWnd || !IsDialogMessage (hAbortDlgWnd, (LPMSG)&msg))
    {
        TranslateMessage ((LPMSG)&msg);
        DispatchMessage ((LPMSG)&msg);
    }
    return(!fAbort);

    hPrintDC;
    iReserved;
}

HANDLE hSysMenu;

/*
 * dialog procedure for the print dialog
 *
 * this allows the user to cancel printing
 */
int fnAbortDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    LPTSTR pchTmp;

    switch(msg)
    {
        case WM_COMMAND:
            fAbort = TRUE;
            EnableWindow(hIndexWnd, TRUE);
            DestroyWindow(hwnd);
            hAbortDlgWnd = NULL;
            return(TRUE);

        case WM_INITDIALOG:
            hSysMenu = GetSystemMenu(hwnd, FALSE);
            if (CurIFile[0])
                pchTmp = FileFromPath(CurIFile);
            else
                pchTmp = szUntitled;

            SetDlgItemText(hwnd, DTNAME, pchTmp);
            SetFocus(hwnd);
            return(TRUE);

        case WM_INITMENU:
            EnableMenuItem(hSysMenu, SC_CLOSE, MF_GRAYED);
            return(TRUE);
    }
    return(FALSE);

    wParam;
    lParam;
}

void PrintError (int iError)
{
    bError = TRUE;
    if (iError & SP_NOTREPORTED)
    {
        switch(iError)
        {
            case -5:
                IndexOkError(EMEMPRINT);
                break;
            case -4:
                IndexOkError(EDISKPRINT);
                break;
            case -3:
            case -2:
                break;
            default:
                IndexOkError(ECANTPRINT);
                break;
        }
    }
}

/***************************************************************************
 * short TranslateString(char *src)
 *
 * purpose:
 *    translate a header/footer strings
 *
 *     supports the following:
 *
 *    &&    insert a & char
 *    &f    current file name or (untitiled)
 *    &d    date in Day Month Year
 *    &t    time
 *    &p    page number
 *    &p+num    set first page number to num
 *
 * params:
 *    IN/OUT    src    this is the string to translate, gets filled with
 *            translate string.  limited by len chars
 *    IN    len    # chars src pts to
 *
 * used by:
 *    Header Footer stuff
 *
 * uses:
 *    lots of c lib stuff
 *
 * restrictions:
 *     this function uses the following global data
 *
 *    iPageNum
 *    text from main window caption
 * NOTE : Resides in _TEXT segment so that it can call C runtimes
 * most of the print code is in _PRINT segment
 *
 ***************************************************************************/

short TranslateString(
    TCHAR *src)
{
    extern int iPageNum;
    extern INT xCharPage;
    extern LPTSTR  szHeadFoot;

    TCHAR         letters[15];
    TCHAR         chBuff[3][80], buf[80];
    TCHAR         *ptr, *dst=buf, *save_src=src;
    int          page;
    short        nAlign=1, foo, nx,
                 nIndex[3];

    nIndex[0]=0;
    nIndex[1]=0;
    nIndex[2]=0;

    LoadString(hIndexInstance, IDS_LETTERS, letters, CharSizeOf(letters));

    while (*src)   /* look at all of source */
    {
        while (*src && *src != TEXT('&'))
        {
            if( IsDBCSLeadByte(*src))
            {
                chBuff[nAlign][nIndex[nAlign]]=*src++;
                nIndex[nAlign] += 1;
            }
            chBuff[nAlign][nIndex[nAlign]]=*src++;
            nIndex[nAlign] += 1;
        }

        if (*src == TEXT('&'))   /* is it the escape char? */
        {
            src++;

            if (*src == letters[0] || *src == letters[1])
            {                      /* &f file name (no path) */

                /* a bit of sleaze... get the caption from
                 * the main window.  search for the '-' and
                 * look two chars beyond, there is the
                 * file name or (untitled) (cute huh?)
                 */

                GetWindowText(hIndexWnd, buf, 80);
                ptr=_tcschr(buf, TEXT('-')) + 2;

                /* Copy to the currently aligned string. */
                lstrcpy((chBuff[nAlign]+nIndex[nAlign]), ptr);

                /* Update insertion position. */
                nIndex[nAlign] += lstrlen(ptr);
            }
            else if (*src == letters[2] || *src == letters[3])
            {                      /* &P or &P+num page */
                src++;
                page = 0;
                if (*src == TEXT('+'))       /* &p+num case */
                {
                    src++;
                    while (_istdigit(*src))
                        {
                        /* Convert to int on-the-fly*/
                        page = (10*page)+(TCHAR)(*src)-48;
                        src++;
                        }
                }

                wsprintf(buf, TEXT("%d"), iPageNum+page);
                lstrcpy((chBuff[nAlign]+nIndex[nAlign]), buf);
                nIndex[nAlign] += lstrlen(buf);
                src--;
            }
            else if (*src == letters[4] || *src == letters[5])
            {                      /* &t time */

                GetDateTime(buf, NULL);

                /* extract time */
                _tcsncpy(chBuff[nAlign]+nIndex[nAlign], buf, lstrlen(buf));
                nIndex[nAlign] += lstrlen(buf);
            }
            else if (*src == letters[6] || *src == letters[7])
            {                      /* &d date */
                GetDateTime(NULL, buf);
                /* extract day month day */
                _tcsncpy(chBuff[nAlign]+nIndex[nAlign], buf, lstrlen(buf));
                nIndex[nAlign] += lstrlen(buf);
            }
            else if (*src == TEXT('&'))
            {               /* quote a single & */
                chBuff[nAlign][nIndex[nAlign]]=TEXT('&');
                nIndex[nAlign] += 1;
            }
            /* Set the alignment for whichever has last occured. */
            else if (*src == letters[8] || *src == letters[9])
                nAlign=1;                    /* &c center */
            else if (*src == letters[10] || *src == letters[11])
                nAlign=2;                    /* &r right */
            else if (*src == letters[12] || *src == letters[13])
                nAlign=0;                   /* &d date */
            src++;
        }
    }
    /* Make sure all strings are null-terminated. */
    for (nAlign=0; nAlign<3; nAlign++)
        chBuff[nAlign][nIndex[nAlign]]=0;

    /* Initialize Header/Footer string */
    for (nx=0; nx<xCharPage; nx++)
        *(szHeadFoot+nx)=32;

    /* Copy Left aligned text. */
    for (nx=0; nx < nIndex[0]; nx++)
        *(szHeadFoot+nx)=chBuff[0][nx];

    /* Calculate where the centered text should go. */
    foo=(xCharPage-nIndex[1])/2;
    for (nx=0; nx<nIndex[1]; nx++)
        *(szHeadFoot+foo+nx)=(TCHAR)chBuff[1][nx];

    /* Calculate where the right aligned text should go. */
    foo=xCharPage-nIndex[2];
    for (nx=0; nx<nIndex[2]; nx++)
        *(szHeadFoot+foo+nx)=(TCHAR)chBuff[2][nx];


    return lstrlen(szHeadFoot);
}

TIME Time;
DATE Date;

/* ** Get current date and time from dos, and build string showing same.
      String must be formatted according to country info obtained from
      win.ini. */
static void GetDateTime(TCHAR *szTime, TCHAR *szDate)
{
  register int i = 0, j = 0;
  int          isAM = TRUE;
  BOOL         bLead;
  TCHAR        cSep;
  SYSTEMTIME   st;


    GetLocalTime (&st);

    if (!szTime)
       goto GetDate;

    if (Time.iTime)
       wsprintf (szTime, Time.iTLZero ? TEXT("%02d%c%02d") : TEXT("%d%c%02d"),
                 st.wHour, Time.szSep[0], st.wMinute);
    else
    {
       if (st.wHour > 12)
       {
          st.wHour -= 12;
          isAM = FALSE;
       }
       wsprintf (szTime, Time.iTLZero ? TEXT("%02d%c%02d%s") : TEXT("%d%c%02d%s"),
                 st.wHour, Time.szSep[0], st.wMinute, isAM ? Time.sz1159 : Time.sz2359);
    }

GetDate:
    if (!szDate)
       return;

    while (Date.szFormat[i] && (j < MAX_FORMAT - 1))
    {
        bLead = FALSE;
        switch (cSep = Date.szFormat[i++])
        {
            case TEXT('d'):
                if (Date.szFormat[i] == TEXT('d'))
                {
                    bLead = TRUE;
                    i++;
                }
                if (bLead || (st.wDay / 10))
                    szDate[j++] = TEXT('0') + st.wDay / 10;
                szDate[j++] = TEXT('0') + st.wDay % 10;
                break;

            case TEXT('M'):
                if (Date.szFormat[i] == TEXT('M'))
                {
                    bLead = TRUE;
                    i++;
                }
                if (bLead || (st.wMonth / 10))
                    szDate[j++] = TEXT('0') + st.wMonth / 10;
                szDate[j++] = TEXT('0') + st.wMonth % 10;
                break;

            case TEXT('y'):
                i++;
                if (Date.szFormat[i] == TEXT('y'))
                {
                    bLead = TRUE;
                    i+=2;
                }
                if (bLead)
                {
                    szDate[j++] = (st.wYear < 2000 ? TEXT('1') : TEXT('2'));
                    szDate[j++] = (st.wYear < 2000 ? TEXT('9') : TEXT('0'));
                }
                szDate[j++] = TEXT('0') + (st.wYear % 100) / 10;
                szDate[j++] = TEXT('0') + (st.wYear % 100) % 10;
                break;

            default:
                /* copy the current character into the formatted string - it
                 * is a separator. BUT: don't copy a separator into the
                 * very first position (could happen if the year comes first,
                 * but we're not using the year)
                 */
                if (j)
                    szDate[j++] = cSep;
                break;
        }
    }
    while ((szDate[j-1] < TEXT('0')) || (szDate[j-1] > TEXT('9')))
        j--;
    szDate[j] = TEXT('\0');
}
