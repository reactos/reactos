/*
 * npprint.c -- Code for printing from notepad.
 * Copyright (C) 1984-1995 Microsoft Inc.
 */

#define NOMINMAX
#include "precomp.h"

//#define DBGPRINT

/* indices into chBuff */
#define LEFT   0
#define CENTER 1
#define RIGHT  2

INT     tabSize;                    /* Size of a tab for print device in device units*/
HWND    hAbortDlgWnd;
INT     fAbort;                     /* true if abort in progress      */
INT     yPrintChar;                 /* height of a character          */


RECT rtMargin;

/* left,center and right string for header or trailer */
#define MAXTITLE MAX_PATH
TCHAR chBuff[RIGHT+1][MAXTITLE];

/* date and time stuff for headers */
#define MAXDATE MAX_PATH
#define MAXTIME MAX_PATH
TCHAR szFormattedDate[MAXDATE]=TEXT("Y");   // formatted date (may be internationalized)
TCHAR szFormattedTime[MAXTIME]=TEXT("Y");   // formatted time (may be internaltionalized)
SYSTEMTIME PrintTime;                       // time we started printing


INT xPrintRes;          // printer resolution in x direction
INT yPrintRes;          // printer resolution in y direction
INT yPixInch;           // pixels/inch
INT xPhysRes;           // physical resolution x of paper
INT yPhysRes;           // physical resolution y of paper

INT xPhysOff;           // physical offset x
INT yPhysOff;           // physical offset y

INT dyTop;              // width of top border (pixels)
INT dyBottom;           // width of bottom border
INT dxLeft;             // width of left border
INT dxRight;            // width of right border

INT iPageNum;           // global page number currently being printed
HMENU hSysMenu;

/* define a type for NUM and the base */
typedef long NUM;
#define BASE 100L

/* converting in/out of fixed point */
#define  NumToShort(x,s)   (LOWORD(((x) + (s)) / BASE))
#define  NumRemToShort(x)  (LOWORD((x) % BASE))

/* rounding options for NumToShort */
#define  NUMFLOOR      0
#define  NUMROUND      (BASE/2)
#define  NUMCEILING    (BASE-1)

#define  ROUND(x)  NumToShort(x,NUMROUND)
#define  FLOOR(x)  NumToShort(x,NUMFLOOR)

/* Unit conversion */
#define  InchesToCM(x)  (((x) * 254L + 50) / 100)
#define  CMToInches(x)  (((x) * 100L + 127) / 254)

void     DestroyAbortWnd(void) ;
VOID     TranslateString(TCHAR *);

BOOL CALLBACK AbortProc(HDC hPrintDC, INT reserved)
{
    MSG msg;

    while (!fAbort && PeekMessage((LPMSG)&msg, NULL, 0, 0, TRUE))
       if (!hAbortDlgWnd || !IsDialogMessage (hAbortDlgWnd, (LPMSG)&msg))
       {
          TranslateMessage((LPMSG)&msg);
          DispatchMessage((LPMSG)&msg);
       }
    return (!fAbort);

    UNREFERENCED_PARAMETER(hPrintDC);
    UNREFERENCED_PARAMETER(reserved);
}


INT_PTR CALLBACK AbortDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch( msg )
    {
       case WM_COMMAND:
          fAbort= TRUE;
          DestroyAbortWnd();
          return( TRUE );

       case WM_INITDIALOG:
          hSysMenu= GetSystemMenu( hwnd, FALSE );
          SetDlgItemText( hwnd, ID_FILENAME,
             fUntitled ? szUntitled : PFileInPath (szFileName) );
          SetFocus( hwnd );
          return( TRUE );

       case WM_INITMENU:
          EnableMenuItem( hSysMenu, (WORD)SC_CLOSE, (DWORD)MF_GRAYED );
          return( TRUE );
    }
    return( FALSE );

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
}


/*
 * print out the translated header/footer string in proper position.
 * uses globals xPrintWidth, ...
 *
 * returns 1 if line was printed, otherwise 0.
 */

INT PrintHeaderFooter (HDC hDC, INT nHF)
{
    SIZE    Size;    // to compute the width of each string
    INT     yPos;    // y position to print
    INT     xPos;    // x position to print

    if( *chPageText[nHF] == 0 )   // see if anything to do
        return 0;                // we didn't print

    TranslateString( chPageText[nHF] );

    // figure out the y position we are printing

    if( nHF == HEADER )
        yPos= dyTop;
    else
        yPos= yPrintRes - dyBottom - yPrintChar;

    // print out the various strings
    // N.B. could overprint which seems ok for now

    if( *chBuff[LEFT] )     // left string
    {
        TextOut( hDC, dxLeft, yPos, chBuff[LEFT], lstrlen(chBuff[LEFT]) );
    }

    if( *chBuff[CENTER] )   // center string
    {
        GetTextExtentPoint32( hDC, chBuff[CENTER], lstrlen(chBuff[CENTER]), &Size );
        xPos= (xPrintRes-dxRight+dxLeft)/2 - Size.cx/2;
        TextOut( hDC, xPos, yPos, chBuff[CENTER], lstrlen(chBuff[CENTER]) );
    }

    if( *chBuff[RIGHT] )    // right string
    {
        GetTextExtentPoint32( hDC, chBuff[RIGHT], lstrlen(chBuff[RIGHT]), &Size );
        xPos= xPrintRes - dxRight - Size.cx;
        TextOut( hDC, xPos, yPos, chBuff[RIGHT], lstrlen(chBuff[RIGHT]) );
    }
    return 1;              // we did print something
}
/*
 * GetResolutions
 *
 * Gets printer resolutions.
 * sets globals: xPrintRes, yPrintRes, yPixInch
 *
 */

VOID GetResolutions(HDC hPrintDC)
{
    xPrintRes = GetDeviceCaps( hPrintDC, HORZRES );
    yPrintRes = GetDeviceCaps( hPrintDC, VERTRES );
    yPixInch  = GetDeviceCaps( hPrintDC, LOGPIXELSY );

    xPhysRes  = GetDeviceCaps( hPrintDC, PHYSICALWIDTH );
    yPhysRes  = GetDeviceCaps( hPrintDC, PHYSICALHEIGHT );

    xPhysOff  = GetDeviceCaps( hPrintDC, PHYSICALOFFSETX );
    yPhysOff  = GetDeviceCaps( hPrintDC, PHYSICALOFFSETY );
}

/* GetMoreText
 *
 * Gets the next line of text from the MLE, returning a pointer
 * to the beginning and just past the end.
 *
 * linenum    - index into MLE                                   (IN)
 * pStartText - start of MLE                                     (IN)
 * ppsStr     - pointer to where to put pointer to start of text (OUT)
 * ppEOL      - pointer to where to put pointer to just past EOL (OUT)
 *
 */

VOID GetMoreText( INT linenum, PTCHAR pStartText, PTCHAR* ppsStr, PTCHAR* ppEOL )
{
    INT Offset;        // offset in 'chars' into edit buffer
    INT nChars;        // number of chars in line

    Offset= (INT)SendMessage( hwndEdit, EM_LINEINDEX, linenum, 0 );

    nChars= (INT)SendMessage( hwndEdit, EM_LINELENGTH, Offset, 0 );

    *ppsStr= pStartText + Offset;

    *ppEOL= (pStartText+Offset) + nChars;
}

#ifdef DBGPRINT
TCHAR dbuf[100];
VOID ShowMargins( HDC hPrintDC )
{
    INT xPrintRes, yPrintRes;
    RECT rct;
    HBRUSH hBrush;

    xPrintRes= GetDeviceCaps( hPrintDC, HORZRES );
    yPrintRes= GetDeviceCaps( hPrintDC, VERTRES );
    hBrush= GetStockObject( BLACK_BRUSH );
    SetRect( &rct, 0,0,xPrintRes-1, yPrintRes-1 );
    FrameRect( hPrintDC, &rct, hBrush );
    SetRect( &rct, dxLeft, dyTop, xPrintRes-dxRight, yPrintRes-dyBottom );
    FrameRect( hPrintDC, &rct, hBrush );
}

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

// GetPrinterDCviaDialog
//
// Use the common dialog PrintDlgEx() function to get a printer DC to print to.
//
// Returns: valid HDC or INVALID_HANDLE_VALUE if error.
//

HDC GetPrinterDCviaDialog( VOID )
{
    PRINTDLGEX pdTemp;
    HDC hDC;
    HRESULT hRes;

    //
    // Get the page setup information
    //

    if( !g_PageSetupDlg.hDevNames )   /* Retrieve default printer if none selected. */
    {
        g_PageSetupDlg.Flags |= (PSD_RETURNDEFAULT|PSD_NOWARNING );
        PageSetupDlg(&g_PageSetupDlg);
        g_PageSetupDlg.Flags &= ~(PSD_RETURNDEFAULT|PSD_NOWARNING);
    }

    //
    // Initialize the dialog structure
    //

    ZeroMemory( &pdTemp, sizeof(pdTemp) );

    pdTemp.lStructSize= sizeof(pdTemp);

    pdTemp.hwndOwner= hwndNP;
    pdTemp.nStartPage= START_PAGE_GENERAL;
    pdTemp.Flags= PD_NOPAGENUMS  | PD_RETURNDC | PD_NOCURRENTPAGE |
                  PD_NOSELECTION | 0;

    // if use set printer in PageSetup, use it here too.

    if( g_PageSetupDlg.hDevMode )
    {
        pdTemp.hDevMode= g_PageSetupDlg.hDevMode;
    }

    if( g_PageSetupDlg.hDevNames )
    {
        pdTemp.hDevNames= g_PageSetupDlg.hDevNames;
    }


    //
    // let user select printer
    //

    hRes= PrintDlgEx( &pdTemp );

    //
    // get DC if valid return
    //

    hDC= INVALID_HANDLE_VALUE;

    if( hRes == S_OK )
    {
        if( (pdTemp.dwResultAction == PD_RESULT_PRINT) || (pdTemp.dwResultAction == PD_RESULT_APPLY) )
        {
            if( pdTemp.dwResultAction == PD_RESULT_PRINT )
            {
                hDC= pdTemp.hDC;
            }
            
            //
            // Get the page setup information for the printer selected in case it was
            // the first printer added by the user through notepad.
            //
            if( !g_PageSetupDlg.hDevMode ) 
            {
                g_PageSetupDlg.Flags |= (PSD_RETURNDEFAULT|PSD_NOWARNING );
                PageSetupDlg(&g_PageSetupDlg);
                g_PageSetupDlg.Flags &= ~(PSD_RETURNDEFAULT|PSD_NOWARNING);
            }

            // change devmode if user pressed print or apply
            g_PageSetupDlg.hDevMode= pdTemp.hDevMode;
            g_PageSetupDlg.hDevNames= pdTemp.hDevNames;
        }       
    }

    // bugbug: free hDevNames

    return( hDC );
}

INT NpPrint( PRINT_DIALOG_TYPE type)
{
    HDC hPrintDC;

    SetCursor( hWaitCursor );

    switch( type )
    {
        case UseDialog:
            hPrintDC= GetPrinterDCviaDialog();
            break;
        case NoDialogNonDefault:
            hPrintDC= GetNonDefPrinterDC();
            break;
        case DoNotUseDialog:
        default:
            hPrintDC= GetPrinterDC();
            break;
    }

    if( hPrintDC == INVALID_HANDLE_VALUE )
    {
        SetCursor( hStdCursor );
        return( 0 );   // message already given
    }

    return( NpPrintGivenDC( hPrintDC ) );

}

INT NpPrintGivenDC( HDC hPrintDC )
{
    HANDLE     hText= NULL;          // handle to MLE text
    HFONT      hPrintFont= NULL;     // font to print with
    HANDLE     hPrevFont= NULL;      // previous font in hPrintDC

    BOOL       fPageStarted= FALSE;  // true if StartPage called for this page
    BOOL       fDocStarted=  FALSE;  // true if StartDoc called
    PTCHAR     pStartText= NULL;     // start of edit text (locked hText)
    TEXTMETRIC Metrics;
    TCHAR      msgbuf[MAX_PATH];
    INT        nLinesPerPage;        // not inc. header and footer
    DWORD      Offset;               // line offset into MLE buffer
    INT        nPrintedLines;        // number of lines on this page
    // iErr will contain the first error discovered ie it is sticky
    // This will be the value returned by this function.
    // It does not need to translate SP_* errors except for SP_ERROR which should be
    // GetLastError() right after it is first detected.
    INT        iErr=0;               // error return
    INT        wpNumLines;           // number of lines to print
    DOCINFO    DocInfo;
    INT        yCurpos;              // current y-position rel. to top margin
    PTCHAR     pNextLine;            // next bit of line to try printing; leftovers
    INT        nPixelsLeft;          // number of pixels left to print in
    SIZE       Size;                 // to see if text will fit in space left
    INT        guess;                // number of chars that can print
    PTCHAR     pLineEOL;             // pointer to end of string from MLE
    PTCHAR     lpLine;               // current line
    INT        LineNum;              // current line number
    LOGFONT    lfPrintFont;          // local version of FontStruct
    LCID       lcid;                 // locale id
#ifdef LEGACY_PRINT
    BOOL       bRight2Left;          // The direction of the edit control
#endif

    fAbort = FALSE;
    hAbortDlgWnd= NULL;

    SetCursor( hWaitCursor );

    GetResolutions( hPrintDC );

    // Get the time and date for use in the header or trailer.
    // We use the GetDateFormat and GetTimeFormat to get the
    // internationalized versions.

    GetLocalTime( &PrintTime );       // use local, not gmt

    lcid= GetUserDefaultLCID();

    GetDateFormat( lcid, DATE_LONGDATE, &PrintTime, NULL, szFormattedDate, MAXDATE );

    GetTimeFormat( lcid, 0,             &PrintTime, NULL, szFormattedTime, MAXTIME );


   /*
    * This part is to select the current font to the printer device.
    * We have to change the height because FontStruct was created
    * assuming the display.  Using the remembered pointsize, calculate
    * the new height.
    */

    lfPrintFont= FontStruct;                          // make local copy
    lfPrintFont.lfHeight= -(iPointSize*yPixInch)/(72*10);
    lfPrintFont.lfWidth= 0;

    //
    // convert margins to pixels
    // ptPaperSize is the physical paper size, not the printable area.
    // do the mapping in physical units
    //

    SetMapMode( hPrintDC, MM_ANISOTROPIC );

    SetViewportExtEx( hPrintDC,
                      xPhysRes,
                      yPhysRes,
                      NULL );

    SetWindowExtEx( hPrintDC,
                    g_PageSetupDlg.ptPaperSize.x,
                    g_PageSetupDlg.ptPaperSize.y,
                    NULL );

    rtMargin = g_PageSetupDlg.rtMargin;

    LPtoDP( hPrintDC, (LPPOINT) &rtMargin, 2 );

    SetMapMode( hPrintDC,MM_TEXT );    // restore to mm_text mode

    hPrintFont= CreateFontIndirect(&lfPrintFont);

    if( !hPrintFont )
    {
        goto ErrorExit;
    }

    hPrevFont= SelectObject( hPrintDC, hPrintFont );
    if( !hPrevFont )
    {
        goto ErrorExit;
    }

    SetBkMode( hPrintDC, TRANSPARENT );
    if( !GetTextMetrics( hPrintDC, (LPTEXTMETRIC) &Metrics ) )
    {
        goto ErrorExit;
    }

    // The font may not a scalable (say on a bubblejet printer)
    // In this case, just pick some font
    // For example, FixedSys 9 pt would be non-scalable

    if( !(Metrics.tmPitchAndFamily & (TMPF_VECTOR | TMPF_TRUETYPE )) )
    {
        // remove just created font

        hPrintFont= SelectObject( hPrintDC, hPrevFont );  // get old font
        DeleteObject( hPrintFont );

        memset( lfPrintFont.lfFaceName, 0, LF_FACESIZE*sizeof(TCHAR) );

        hPrintFont= CreateFontIndirect( &lfPrintFont );
        if( !hPrintFont )
        {
            goto ErrorExit;
        }

        hPrevFont= SelectObject( hPrintDC, hPrintFont );
        if( !hPrevFont )
        {
            goto ErrorExit;
        }

        if( !GetTextMetrics( hPrintDC, (LPTEXTMETRIC) &Metrics ) )
        {
            goto ErrorExit;
        }
    }
    yPrintChar= Metrics.tmHeight+Metrics.tmExternalLeading;  /* the height */

    tabSize = Metrics.tmAveCharWidth * 8; /* 8 ave char width pixels for tabs */

    // compute margins in pixels

    dxLeft=   max(rtMargin.left - xPhysOff,0);
    dxRight=  max(rtMargin.right  - (xPhysRes - xPrintRes - xPhysOff), 0 );
    dyTop=    max(rtMargin.top  - yPhysOff,0);
    dyBottom= max(rtMargin.bottom - (yPhysRes - yPrintRes - yPhysOff), 0 );

#ifdef DBGPRINT
    {
        TCHAR dbuf[100];
        RECT rt= g_PageSetupDlg.rtMargin;
        POINT pt;

        wsprintf(dbuf,TEXT("Print pOffx %d  pOffy %d\n"),
                 GetDeviceCaps(hPrintDC, PHYSICALOFFSETX),
                 GetDeviceCaps(hPrintDC, PHYSICALOFFSETY));
        ODS(dbuf);
        wsprintf(dbuf,TEXT("PHYSICALWIDTH: %d\n"), xPhysRes);
        ODS(dbuf);
        wsprintf(dbuf,TEXT("HORZRES: %d\n"),xPrintRes);
        ODS(dbuf);
        wsprintf(dbuf,TEXT("PHYSICALOFFSETX: %d\n"),xPhysOff);
        ODS(dbuf);
        wsprintf(dbuf,TEXT("LOGPIXELSX: %d\n"),
                 GetDeviceCaps(hPrintDC,LOGPIXELSX));
        ODS(dbuf);

        GetViewportOrgEx( hPrintDC, (LPPOINT) &pt );
        wsprintf(dbuf,TEXT("Viewport org:  %d %d\n"), pt.x, pt.y );
        ODS(dbuf);
        GetWindowOrgEx( hPrintDC, (LPPOINT) &pt );
        wsprintf(dbuf,TEXT("Window org:  %d %d\n"), pt.x, pt.y );
        ODS(dbuf);
        wsprintf(dbuf,TEXT("PrintRes x: %d  y: %d\n"),xPrintRes, yPrintRes);
        ODS(dbuf);
        wsprintf(dbuf,TEXT("PaperSize  x: %d  y: %d\n"),
                 g_PageSetupDlg.ptPaperSize.x,
                 g_PageSetupDlg.ptPaperSize.y );
        ODS(dbuf);
        wsprintf(dbuf,TEXT("unit margins:  l: %d  r: %d  t: %d  b: %d\n"),
                 rt.left, rt.right, rt.top, rt.bottom);
        ODS(dbuf);
        wsprintf(dbuf,TEXT("pixel margins: l: %d  r: %d  t: %d  b: %d\n"),
                 rtMargin.left, rtMargin.right, rtMargin.top, rtMargin.bottom);
        ODS(dbuf);

        wsprintf(dbuf,TEXT("dxLeft %d  dxRight %d\n"),dxLeft,dxRight);
        ODS(dbuf);
        wsprintf(dbuf,TEXT("dyTop %d  dyBot %d\n"),dyTop,dyBottom);
        ODS(dbuf);
    }
#endif


    /* Number of lines on a page with margins  */
    /* two lines are used by header and footer */
    nLinesPerPage = ((yPrintRes - dyTop - dyBottom) / yPrintChar);

    if( *chPageText[HEADER] )
        nLinesPerPage--;
    if( *chPageText[FOOTER] )
        nLinesPerPage--;


    /*
    ** There was a bug in NT once where a printer driver would
    ** return a font that was larger than the page size which
    ** would then cause Notepad to constantly print blank pages
    ** To keep from doing this we check to see if we can fit ANYTHING
    ** on a page, if not then there is a problem so quit.  MarkRi 8/92
    */
    if( nLinesPerPage <= 0 )
    {
FontTooBig:
        MessageBox( hwndNP, szFontTooBig, szNN, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION );

        SetLastError(0);          // no error

ErrorExit:
        iErr= GetLastError();     // remember the first error

ExitWithThisError:                // preserve iErr (return SP_* errors)

        if (hPrevFont)
        {
            SelectObject( hPrintDC, hPrevFont );
            DeleteObject( hPrintFont );
        }

        if( pStartText )          // were able to lock hText
            LocalUnlock( hText );

        if( fPageStarted )
        {
            if( EndPage( hPrintDC ) <= 0 )
            {
                // if iErr not already set then set it to the new error code.
                if (iErr == 0)
                {
                    iErr = GetLastError();
                }
       
            }
        }    

        if( fDocStarted )
        {
            if( EndDoc( hPrintDC ) <= 0 )
            {
                // if iErr not already set then set it to the new error code.
                if (iErr == 0)
                {
                    iErr = GetLastError();
                }
       
            }
        }    

        DeleteDC( hPrintDC );

        DestroyAbortWnd();

        SetCursor( hStdCursor );

        if (!fAbort)
        {
            return( iErr );
        }
        else
        {
            return( SP_USERABORT );
        }
    }



    if( (iErr= SetAbortProc (hPrintDC, AbortProc)) < 0 )
    {
        goto ExitWithThisError;
    }

    // get printer to MLE text
    hText= (HANDLE) SendMessage( hwndEdit, EM_GETHANDLE, 0, 0 );
    if( !hText )
    {
        goto ErrorExit;
    }
    pStartText= LocalLock( hText );
    if( !pStartText )
    {
        goto ErrorExit;
    }

    GetWindowText( hwndNP, msgbuf, CharSizeOf(msgbuf) );

    EnableWindow( hwndNP, FALSE );    // Disable window to prevent reentrancy

    hAbortDlgWnd= CreateDialog(         hInstanceNP,
                              (LPTSTR)  MAKEINTRESOURCE(IDD_ABORTPRINT),
                                        hwndNP,
                                        AbortDlgProc);

    if( !hAbortDlgWnd )
    {
        goto ErrorExit;
    }

    DocInfo.cbSize= sizeof(DOCINFO);
    DocInfo.lpszDocName= msgbuf;
    DocInfo.lpszOutput= NULL;
    DocInfo.lpszDatatype= NULL; // Type of data used to record print job
    DocInfo.fwType= 0; // not DI_APPBANDING

    SetLastError(0);      // clear error so it reflects errors in the future

    if( StartDoc( hPrintDC, &DocInfo ) <= 0 )
    {
        iErr = GetLastError();
        goto ExitWithThisError;
    }
    fDocStarted= TRUE;

#ifdef LEGACY_PRINT
    yCurpos = 0;
    Offset  = 0;
    LineNum = 0;
    nPrintedLines= 0;
    iPageNum= 1;

    //Get the edit control direction.
    if(bRight2Left = GetWindowLongPtr(hwndEdit, GWL_EXSTYLE) & WS_EX_RTLREADING)
       SetTextAlign(hPrintDC, TA_RTLREADING | GetTextAlign(hPrintDC));

    wpNumLines= SendMessage( hwndEdit, EM_GETLINECOUNT, 0, 0 );

    // if last line is empty, don't print it
    GetMoreText( wpNumLines-1, pStartText, &lpLine, &pLineEOL );
    if( *lpLine == 0 )
        wpNumLines--;

    nPrintedLines= 0;  // number of lines printed on this page
    fPageStarted= FALSE;

    while (!fAbort && LineNum < wpNumLines)
    {
      GetMoreText( LineNum++, pStartText, &lpLine, &pLineEOL );

      do                          // till lpLine == pLineEOL
      {
         // Print out header if we are about to print
         // the first line on the page
         if( nPrintedLines == 0 && !fPageStarted )
         {
            if( StartPage( hPrintDC ) <= 0 )
            {
               iErr = GetLastError();
               goto ExitWithThisError;
            }
            fPageStarted= TRUE;    // prevent StartPage on next partial line

            // print header if one exists
            yCurpos= 0;
            if( PrintHeaderFooter( hPrintDC, HEADER ) )
               yCurpos= yPrintChar;

            xCurpos= 0;
            #ifdef DBGPRINT
            ShowMargins(hPrintDC);
            #endif
         }

         // print and move print head in x-direction
         // handle tabs characters as a special case

         if( *lpLine == TEXT('\t') )   // tab?
         {
            // round up to the next tab stop
            // if the current position is on the tabstop, goto next one
            xCurpos= ( (xCurpos+tabSize)/tabSize ) * tabSize;
            lpLine++;
         }
         else                          // first character not a tab
         {
            // find what to print - up to EOL or tab
            pNextLine= lpLine;     // find end of line or tab
            while( (pNextLine!=pLineEOL) && *pNextLine != TEXT('\t') )
               pNextLine++;

            // find out how many characters will fit on line
            nPixelsLeft= xPrintRes - dxRight - dxLeft - xCurpos;
            GetTextExtentExPoint( hPrintDC, lpLine, pNextLine-lpLine,
                                  nPixelsLeft, &guess, NULL, &Size );

            if( guess )
            {
               // at least one character fits - print

                if (bRight2Left)
                   TextOut( hPrintDC, xPrintRes - (dxRight+xCurpos+Size.cx), yCurpos+dyTop, lpLine, guess);
                else
                   TextOut( hPrintDC, dxLeft+xCurpos, yCurpos+dyTop, lpLine, guess);

               xCurpos += Size.cx;   // account for printing
               lpLine  += guess;
            }
            else      // no characters fit what's left
            {
               // no characters will fit in space left
               // if none ever will, just print one
               // character to keep progressing through
               // input file.
               if( xCurpos == 0 )
               {
                  if( lpLine != pNextLine ) //print something if not null line
                  {
                     // could use exttextout here to clip
                     TextOut(hPrintDC,dxLeft+xCurpos,yCurpos+dyTop,lpLine,1);
                     lpLine++;
                  }
               }
               else  // perhaps the next line will get it
               {
                   xCurpos= xPrintRes;  // force to next line
               }
            }

         }  // not a tab

         // move printhead in y-direction

         if( (xCurpos >= (xPrintRes - dxRight - dxLeft) ) || (lpLine==pLineEOL ) )
         {
            yCurpos += yPrintChar;
            nPrintedLines++;
            xCurpos= 0;
         }

         if( nPrintedLines >= nLinesPerPage )
         {
            PrintHeaderFooter( hPrintDC, FOOTER );

            if( EndPage( hPrintDC ) <= 0 )
            {
               iErr = GetLastError();
               goto ExitWithThisError;
            }
            fPageStarted= FALSE;

            // reset info
            nPrintedLines= 0;
            xCurpos= 0;
            iPageNum++;
         }
      }
      while( lpLine != pLineEOL  && !fAbort );  // continue if more to do with line


    } // continue if more lines

    if( !fAbort && nPrintedLines )
    {
        PrintHeaderFooter( hPrintDC, FOOTER );
    }

    iErr=0;        // no errors
    goto ExitWithThisError;

#else  // LEGACY_PRINT

    // Basicly, this is just a loop surrounding the DrawTextEx API.
    // We have to calculate the printable area which will not include
    // the header and footer area.
    {
    INT iTextLeft;        // amount of text left to print
    INT iSta;              // status
    UINT dwDTFormat;       // drawtext flags
    DRAWTEXTPARAMS dtParm; // drawtext control
    RECT rect;             // rectangle to draw in
    UINT dwDTRigh = 0;     // drawtext flags (RTL)

    iPageNum= 1;
    fPageStarted= FALSE;

    // calculate the size of the printable area for the text
    // not including the header and footer

    ZeroMemory( &rect, sizeof(rect) );

    rect.left= dxLeft; rect.right= xPrintRes-dxRight;
    rect.top=  dyTop;  rect.bottom= yPrintRes-dyBottom;

    if( *chPageText[HEADER] != 0 )
    {
        rect.top += yPrintChar;
    }

    if( *chPageText[FOOTER] != 0 )
    {
        rect.bottom -= yPrintChar;
    }

    iTextLeft= lstrlen(pStartText);

    //Get the edit control direction.
    if (GetWindowLong(hwndEdit, GWL_EXSTYLE) & WS_EX_RTLREADING)
        dwDTRigh = DT_RIGHT | DT_RTLREADING;


    while(  !fAbort && (iTextLeft>0) )
    {

        PrintHeaderFooter( hPrintDC, HEADER );

        ZeroMemory( &dtParm, sizeof(dtParm) );

        dtParm.cbSize= sizeof(dtParm);
        dtParm.iTabLength= tabSize;

        dwDTFormat= DT_EDITCONTROL | DT_LEFT | DT_EXPANDTABS | DT_NOPREFIX |
                    DT_WORDBREAK | dwDTRigh | 0;

        if( StartPage( hPrintDC ) <= 0 )
        {
            iErr= GetLastError();            
            goto ExitWithThisError;
        }
        fPageStarted= TRUE;

        #ifdef DBGPRINT
        ShowMargins(hPrintDC);
        #endif

        /* Ignore errors in printing.  EndPage or StartPage will find them */
        iSta= DrawTextEx( hPrintDC,
                          pStartText,
                          iTextLeft,
                          &rect,
                          dwDTFormat,
                          &dtParm);

        PrintHeaderFooter( hPrintDC, FOOTER );

        if( EndPage( hPrintDC ) <= 0 )
        {
            iErr= GetLastError();            
            goto ExitWithThisError;
        }
        fPageStarted= FALSE;

        iPageNum++;

        // if we can't print a single character (too big perhaps)
        // just bail now.
        if( dtParm.uiLengthDrawn == 0 )
        {
            goto FontTooBig;
        }

        pStartText += dtParm.uiLengthDrawn;
        iTextLeft  -= dtParm.uiLengthDrawn;

    }


    }

    iErr=0;        // no errors
    goto ExitWithThisError;
#endif  // LEGACY_PRINT

}


VOID DestroyAbortWnd (void)
{
    EnableWindow(hwndNP, TRUE);
    DestroyWindow(hAbortDlgWnd);
    hAbortDlgWnd = NULL;
}



const DWORD s_PageSetupHelpIDs[] = {
    ID_HEADER_LABEL,       IDH_PAGE_HEADER,
    ID_HEADER,             IDH_PAGE_HEADER,
    ID_FOOTER_LABEL,       IDH_PAGE_FOOTER,
    ID_FOOTER,             IDH_PAGE_FOOTER,
    0, 0
};

/*******************************************************************************
*
*  PageSetupHookProc
*
*  DESCRIPTION:
*     Callback procedure for the PageSetup common dialog box.
*
*  PARAMETERS:
*     hWnd, handle of PageSetup window.
*     Message,
*     wParam,
*     lParam,
*     (returns),
*
*******************************************************************************/

UINT_PTR CALLBACK PageSetupHookProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    INT   id;    /* ID of dialog edit controls */
    POINT pt;

    switch (Message)
    {

        case WM_INITDIALOG:
            for (id = ID_HEADER; id <= ID_FOOTER; id++)
            {
                SendDlgItemMessage(hWnd, id, EM_LIMITTEXT, PT_LEN-1, 0L);
                SetDlgItemText(hWnd, id, chPageText[id - ID_HEADER]);
            }

            SendDlgItemMessage(hWnd, ID_HEADER, EM_SETSEL, 0,
                               MAKELONG(0, PT_LEN-1));
            return TRUE;

        case WM_DESTROY:
            //  We don't know if the user hit OK or Cancel, so we don't
            //  want to replace our real copies until we know!  We _should_ get
            //  a notification from the common dialog code!
            for( id = ID_HEADER; id <= ID_FOOTER; id++ )
            {
                GetDlgItemText(hWnd, id, chPageTextTemp[id - ID_HEADER],PT_LEN);
            }
            break;

        case WM_HELP:
            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            id = GetDlgCtrlID(((LPHELPINFO) lParam)-> hItemHandle);

            if (id < ID_HEADER || id > ID_FOOTER_LABEL)
                break;

            WinHelp(((LPHELPINFO) lParam)-> hItemHandle, szHelpFile,
                HELP_WM_HELP, (UINT_PTR) (LPVOID) s_PageSetupHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            //
            //  If the user clicks on any of our labels, then the wParam will
            //  be the hwnd of the dialog, not the static control.  WinHelp()
            //  handles this, but because we hook the dialog, we must catch it
            //  first.
            //
            if( hWnd == (HWND) wParam )
            {

                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                wParam = (WPARAM) ChildWindowFromPoint(hWnd, pt);

            }

            //
            //  We only want to intercept help messages for controls that we are
            //  responsible for.
            //

            id = GetDlgCtrlID((HWND) wParam);

            if (id < ID_HEADER || id > ID_FOOTER_LABEL)
                break;

            WinHelp((HWND) wParam, szHelpFile, HELP_CONTEXTMENU,
                (UINT_PTR) (LPVOID) s_PageSetupHelpIDs);
            return TRUE;

    }

    return FALSE;

}

/***************************************************************************
 * VOID TranslateString(TCHAR *src)
 *
 * purpose:
 *    translate a header/footer strings
 *
 * supports the following:
 *
 *    &&    insert a & char
 *    &f    current file name or (untitled)
 *    &d    date in Day Month Year
 *    &t    time
 *    &p    page number
 *    &p+num  set first page number to num
 *
 * Alignment:
 *    &l, &c, &r for left, center, right
 *
 * params:
 *    IN/OUT  src     this is the string to translate
 *
 *
 * used by:
 *    Header Footer stuff
 *
 * uses:
 *    lots of c lib stuff
 *
 ***************************************************************************/


VOID TranslateString (TCHAR * src)
{
    TCHAR        letters[15];
    TCHAR        buf[MAX_PATH];
    TCHAR       *ptr;
    INT          page;
    INT          nAlign=CENTER;    // current string to add chars to
    INT          foo;
    INT          nIndex[RIGHT+1];  // current lengths of (left,center,right)
    struct tm   *newtime;
    time_t       long_time;
    INT          iLen;             // length of strings

    nIndex[LEFT]   = 0;
    nIndex[CENTER] = 0;
    nIndex[RIGHT]  = 0;

    /* Get the time we need in case we use &t. */
    time (&long_time);
    newtime = localtime (&long_time);

    LoadString (hInstanceNP, IDS_LETTERS, letters, CharSizeOf(letters));

    while (*src)   /* look at all of source */
    {
        while (*src && *src != TEXT('&'))
        {
            chBuff[nAlign][nIndex[nAlign]] = *src++;
            nIndex[nAlign] += 1;
        }

        if (*src == TEXT('&'))   /* is it the escape char? */
        {
            src++;

            if (*src == letters[0] || *src == letters[1])
            {                      /* &f file name (no path) */
                if (!fUntitled)
                {
                    GetFileTitle(szFileName, buf, CharSizeOf(buf));
                }
                else
                {
                    lstrcpy(buf, szUntitled);
                }

                /* Copy to the currently aligned string. */
                if( nIndex[nAlign] + lstrlen(buf) < MAXTITLE )
                {
                    lstrcpy( chBuff[nAlign] + nIndex[nAlign], buf );

                    /* Update insertion position. */
                    nIndex[nAlign] += lstrlen (buf);
                }

            }
            else if (*src == letters[2] || *src == letters[3])  /* &P or &P+num page */
            {
                src++;
                page = 0;
                if (*src == TEXT('+'))       /* &p+num case */
                {
                    src++;
                    while (_istdigit(*src))
                    {
                        /* Convert to int on-the-fly*/
                        page = (10*page) + (*src) - TEXT('0');
                        src++;
                    }
                }

                wsprintf( buf, TEXT("%d"), iPageNum+page );  // convert to chars

                if( nIndex[nAlign] + lstrlen(buf) < MAXTITLE )
                {
                    lstrcpy( chBuff[nAlign] + nIndex[nAlign], buf );
                    nIndex[nAlign] += lstrlen (buf);
                }
                src--;
            }
            else if (*src == letters[4] || *src == letters[5])   /* &t time */
            {
                iLen= lstrlen( szFormattedTime );

                /* extract time */
                if( nIndex[nAlign] + iLen < MAXTITLE )
                {
                    _tcsncpy (chBuff[nAlign] + nIndex[nAlign], szFormattedTime, iLen);
                    nIndex[nAlign] += iLen;
                }
            }
            else if (*src == letters[6] || *src == letters[7])   /* &d date */
            {
                iLen= lstrlen( szFormattedDate );

                /* extract day month day */
                if( nIndex[nAlign] + iLen < MAXTITLE )
                {
                    _tcsncpy (chBuff[nAlign] + nIndex[nAlign], szFormattedDate, iLen);
                    nIndex[nAlign] += iLen;
                }
            }
            else if (*src == TEXT('&'))       /* quote a single & */
            {
                if( nIndex[nAlign] + 1 < MAXTITLE )
                {
                    chBuff[nAlign][nIndex[nAlign]] = TEXT('&');
                    nIndex[nAlign] += 1;
                }
            }
            /* Set the alignment for whichever has last occured. */
            else if (*src == letters[8] || *src == letters[9])   /* &c center */
                nAlign=CENTER;
            else if (*src == letters[10] || *src == letters[11]) /* &r right */
                nAlign=RIGHT;
            else if (*src == letters[12] || *src == letters[13]) /* &d date */
                nAlign=LEFT;

            src++;
        }
     }
     /* Make sure all strings are null-terminated. */
     for (nAlign= LEFT; nAlign <= RIGHT ; nAlign++)
        chBuff[nAlign][nIndex[nAlign]] = (TCHAR) 0;

}

/* GetPrinterDC() - returns printer DC or INVALID_HANDLE_VALUE if none. */

HANDLE GetPrinterDC (VOID)
{
    LPDEVMODE lpDevMode;
    LPDEVNAMES lpDevNames;
    HDC hDC;


    if( !g_PageSetupDlg.hDevNames )   /* Retrieve default printer if none selected. */
    {
        g_PageSetupDlg.Flags |= PSD_RETURNDEFAULT;
        PageSetupDlg(&g_PageSetupDlg);
        g_PageSetupDlg.Flags &= ~PSD_RETURNDEFAULT;
    }

    if( !g_PageSetupDlg.hDevNames )
    {
        MessageBox( hwndNP, szLoadDrvFail, szNN, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
        return INVALID_HANDLE_VALUE;
    }

    lpDevNames= (LPDEVNAMES) GlobalLock (g_PageSetupDlg.hDevNames);


    lpDevMode= NULL;

    if( g_PageSetupDlg.hDevMode )
       lpDevMode= (LPDEVMODE) GlobalLock( g_PageSetupDlg.hDevMode );

    /*  For pre 3.0 Drivers,hDevMode will be null  from Commdlg so lpDevMode
     *  will be NULL after GlobalLock()
     */

    /* The lpszOutput name is null so CreateDC will use the current setting
     * from PrintMan.
     */

    hDC= CreateDC (((LPTSTR)lpDevNames)+lpDevNames->wDriverOffset,
                      ((LPTSTR)lpDevNames)+lpDevNames->wDeviceOffset,
                      NULL,
                      lpDevMode);

    GlobalUnlock( g_PageSetupDlg.hDevNames );

    if( g_PageSetupDlg.hDevMode )
        GlobalUnlock( g_PageSetupDlg.hDevMode );


    if( hDC == NULL )
    {
        MessageBox( hwndNP, szLoadDrvFail, szNN, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
        return INVALID_HANDLE_VALUE;
    }

    return hDC;
}


/* GetNonDefPrinterDC() - returns printer DC or INVALID_HANDLE_VALUE if none. */
/*                        using the name of the Printer server */

HANDLE GetNonDefPrinterDC (VOID)
{
    HDC     hDC;
    HANDLE  hPrinter;
    DWORD   dwBuf;
    DRIVER_INFO_1  *di1;



    // open the printer and retrieve the driver name.
    if (!OpenPrinter(szPrinterName, &hPrinter, NULL))
    {
        return INVALID_HANDLE_VALUE;
    }

    // get the buffer size.
    GetPrinterDriver(hPrinter, NULL, 1, NULL, 0, &dwBuf);
    di1 = (DRIVER_INFO_1  *) LocalAlloc(LPTR, dwBuf);
    if (!di1)
    {
        ClosePrinter(hPrinter);
        return INVALID_HANDLE_VALUE;
    }

    if (!GetPrinterDriver(hPrinter, NULL, 1, (LPBYTE) di1, dwBuf, &dwBuf))
    {
        LocalFree(di1);
        ClosePrinter(hPrinter);
        return INVALID_HANDLE_VALUE;
    }

    // Initialize the PageSetup dlg to default values.
    // BUGBUG: using default printer's value for another printer !!
    g_PageSetupDlg.Flags |= PSD_RETURNDEFAULT;
    PageSetupDlg(&g_PageSetupDlg);
    g_PageSetupDlg.Flags &= ~PSD_RETURNDEFAULT;

    // create printer dc with default initialization.
    hDC= CreateDC (di1->pName, szPrinterName, NULL, NULL);

    // cleanup.
    LocalFree(di1);
    ClosePrinter(hPrinter);

    if( hDC == NULL )
    {
        MessageBox( hwndNP, szLoadDrvFail, szNN, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
        return INVALID_HANDLE_VALUE;
    }

    return hDC;
}


/* PrintIt() - print the file, giving popup if some error */

void PrintIt(PRINT_DIALOG_TYPE type)
{
    INT iError;
    TCHAR* szMsg= NULL;
    TCHAR  msg[400];       // message info on error

    /* print the file */

    iError= NpPrint( type );

    if(( iError != 0) &&
       ( iError != SP_APPABORT )     &&
       ( iError != SP_USERABORT ) )
    {
        // translate any known spooler errors
        if( iError == SP_OUTOFDISK   ) iError= ERROR_DISK_FULL;
        if( iError == SP_OUTOFMEMORY ) iError= ERROR_OUTOFMEMORY;
        if( iError == SP_ERROR       ) iError= GetLastError();
        /* SP_NOTREPORTED not handled.  Does it happen? */


        //
        // iError may be 0 because the user aborted the printing.
        // Just ignore.
        //

        if( iError == 0 ) return;

        // Get system to give reasonable error message
        // These will also be internationalized.

        if(!FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS |
                           FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           iError,
                           GetUserDefaultLangID(),
                           msg,  // where message will end up
                           CharSizeOf(msg), NULL ) )
        {
            szMsg= szCP;   // couldn't get system to say; give generic msg
        }
        else
        {
            szMsg= msg;
        }

        AlertBox( hwndNP, szNN, szMsg, fUntitled ? szUntitled : szFileName,
                  MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
    }
}
