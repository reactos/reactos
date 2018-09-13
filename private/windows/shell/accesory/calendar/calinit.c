/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
 ****** calinit.c
 *
 */

/* Get rid of more stuff from windows.h */
#define NOSYSCOMMANDS
#define NOSCROLL
#define NODRAWTEXT
#define NOVIRTUALKEYCODES

#include "cal.h"
#include "string.h"

#define DOTIMER

//- OBM_RGARROW & OBM_LFARROW: Use the new arrows instead of old ones.
#define OBM_RGARROW         32751
#define OBM_LFARROW         32750


extern INT cchLongDateMax;
extern INT cchTimeMax;
extern BOOL f24Time;

#define GSM(SM) GetSystemMetrics(SM)

BOOL APIENTRY   ProcessShellOptions(
        LPSTR lpszCmdLine);

/**** CalInit ****/

BOOL APIENTRY CalInit (
     HANDLE hInstance,
     HANDLE hPrevInstance,
     LPSTR  lpszCmdLine,
     INT    cmdShow)
     {
     BITMAP         bmBell;
     WNDCLASS   WndClass;
     HDC            hDC;
     TEXTMETRIC Metrics;
     INT i;
     INT cyUseable;
     CHAR   *pch;
     INT    cchRemaining, cch, cxWnd2C, cyWnd2C;
     HANDLE hStrings;
     INT    tm, TimeSetting;
     INT    BlankWidth;   /* width (in pixels) of a blank char */
     CHAR * pszFilterSpec = vszFilterSpec;  /* temp. var. for creating filter text */

     CHAR   sz[10];
     CHAR   *psz=sz;
     INT        iHeight;
     INT    iWidth;


     /* Remember our instance handle. */
     vhInstance = hInstance;

     /* Assume this string is longer for initialization. */
     vfHour24    = FALSE;
     TimeSetting = GTS_12HOUR;

     /* determine time setting from "International" section of win.ini */
     tm = 1;
     if (tm=GetProfileInt((LPSTR)"intl", "iTime", tm))
        if (tm = 1)
            {
            TimeSetting = GTS_24HOUR;
            vfHour24 = TRUE;
            }

     InitTimeDate(vhInstance, TimeSetting);
     InitLongTimeDate(TimeSetting);

     /* Load strings from resource file. */
     hStrings=LocalAlloc(LPTR, cchRemaining=CCHSTRINGSMAX);

     pch=(CHAR *)(hStrings);
     if (!pch)
          return (FALSE);


     for (i=0; i<CSTRINGS; i++)
          {
          cch=1+LoadString(hInstance, i, (LPSTR)pch, cchRemaining);

          /* If LoadString failed, not enough memory. */
          if (cch < 2)
              {
              MessageBeep(0);
              return FALSE;
              }
          vrgsz [i] = pch;
          pch += cch;

          /* If we run out of space it means that CCHSTRINGSMAX is too small.
             This should only happen when someone changes the strings in the
             .RC file, and returning FALSE should prevent them from shipping
             the new version without increasing CCHSTRINGSMAX (and possibly
             the initial heap size in the .DEF file).
             Note - we fail on the boundary condition that cchRemaining == 0
             because last loadstring will trim the size of the string loaded
             to be <= cchRemaining.
          */
          if ((cchRemaining -= cch) <= 0)
               return (FALSE);
          }


     //- CalInit: Eliminate Realloc because it seems to trash string pointers.
         //- Only lose 800 bytes anyway.
     //- LocalReAlloc(hStrings, CCHSTRINGSMAX-cchRemaining, LPTR);

     //- MergeStr: Changed to string to avoid crossing word boundries.
     strncpy (vszMergeStr, vrgsz [IDS_MERGE1], 2);


     /* Get default Page Setup stuff. */
     for (i=IDS_HEADER; i<=IDS_BOTTOM; i++)
         LoadString(hInstance, i, (LPSTR)chPageText[i-IDS_HEADER], PT_LEN);

     /* Allocate the DRs. */
     if (!AllocDr ())
        return FALSE;

     /* Create the brushes. */
     if (!CreateBrushes ())
        /* Destroy any brushes that were created on failure. */
        return CalTerminate(0);

     /* construct default filter string in the required format for
      * the new FileOpen and FileSaveAs dialogs
      */
     lstrcpy((LPSTR)vszFilterSpec, (LPSTR)vszFilterText);
     pszFilterSpec += lstrlen ((LPSTR)vszFilterSpec) + 1;
     lstrcpy((LPSTR)pszFilterSpec++, (LPSTR)"*");
     lstrcpy((LPSTR)pszFilterSpec, (LPSTR)vszFileExtension);
     pszFilterSpec += lstrlen((LPSTR)pszFilterSpec) + 1;
     lstrcpy((LPSTR)pszFilterSpec, (LPSTR)vszAllFiles);
     pszFilterSpec += lstrlen((LPSTR)pszFilterSpec) + 1;
     lstrcpy((LPSTR)pszFilterSpec, (LPSTR)"*.*");
     pszFilterSpec += lstrlen((LPSTR)pszFilterSpec) + 1;
     *pszFilterSpec = '\0';
     *vszCustFilterSpec = '\0';

     /* Get cursors. */
     if (!(vhcsrArrow = LoadCursor (NULL, IDC_ARROW)))
        return CalTerminate(0);

     if (!(vhcsrIbeam = LoadCursor (NULL, IDC_IBEAM)))
        return CalTerminate(0);

     if (!(vhcsrWait = LoadCursor (NULL, IDC_WAIT)))
        return CalTerminate(0);



     if (hPrevInstance == (HANDLE)NULL)
          {
          /* There is no previous instance, so we must register our
           * window classes.
           */

          FillBuf ((BYTE *)&WndClass, sizeof (WNDCLASS), 0);

          if (!(WndClass.hIcon = LoadIcon (hInstance, MAKEINTRESOURCE(1))))
            return CalTerminate(0);

          WndClass.lpszMenuName  = MAKEINTRESOURCE(1);
          WndClass.lpszClassName = (LPSTR)"CalWndMain",
          WndClass.hInstance    = hInstance;
          WndClass.style         = CS_VREDRAW | CS_HREDRAW |
                                   CS_DBLCLKS | CS_BYTEALIGNCLIENT;
          WndClass.lpfnWndProc   = (WNDPROC)(CalWndProc);

          /* Register CalWndMain. */
          if (!RegisterClass ((LPWNDCLASS)&WndClass))
               return CalTerminate(0);

          WndClass.lpszMenuName  = (LPSTR)NULL;
          WndClass.lpszClassName = (LPSTR)"CalWndSub";

          /* Register CalWndSub. */
          if (!RegisterClass ((LPWNDCLASS)&WndClass))
               return CalTerminate(0);
          }

     /* Bind the data segment of this instance to the dialog functions. */
     for (i=0; i < CIDD; i++)
          {
          if (vrglpfnDialog[i] == NULL)
                continue;
          vrglpfnDialog[i]=MakeProcInstance(vrglpfnDialog [i], hInstance);
          if (!vrglpfnDialog[i])
            return CalTerminate(0);
          }

     /* Load in the accelerators. */
     vhAccel=LoadAccelerators(hInstance, MAKEINTRESOURCE(1));
     if (!vhAccel)
        return CalTerminate(0);

     /*   Get bitmaps.
      *   If LoadBitmaps returns false, must delete those that were loaded.
      */
     if (!LoadBitmaps(hInstance))
        return CalTerminate(1);


     /* Get a screen DC. */
     hDC=GetDC(NULL);

     /* Fetch the text metrics of the system font. */
     GetTextMetrics (hDC, (LPTEXTMETRIC)(&Metrics));

     /* Create a memory DC for BitBlts. */
     vhDCMemory=CreateCompatibleDC(hDC);

     ReleaseDC (NULL, hDC);

     if (!vhDCMemory)
        return CalTerminate(1);

     /* Remember the text metrics we care about. */
     vcyFont    = Metrics.tmHeight;
     vcxFont    = Metrics.tmAveCharWidth;
     vcxFontMax = Metrics.tmMaxCharWidth;
     vcyDescent = Metrics.tmDescent;
     vcyExtLead = Metrics.tmExternalLeading;

     if (vcyExtLead == 0)
          vcyExtLead=max(1, vcyFont/8);

     vcyLineToLine=vcyExtLead+vcyFont;

     /* Fetch some system metrics. */
     vcxBorder     = GSM(SM_CXBORDER);
     vcyBorder     = GSM(SM_CYBORDER);
     vcxVScrollBar = GSM(SM_CXVSCROLL);
     vcxHScrollBar = GSM(SM_CXHSCROLL);
     vcyHScrollBar = GSM(SM_CYHSCROLL);

     /* Find out how big the the bell bitmap is. */
     GetObject(vhbmBell, sizeof(BITMAP), (LPSTR)(CHAR *)&bmBell);
     vcxBell = bmBell.bmWidth;
     vcyBell = bmBell.bmHeight;

     /* Calculate the window heights.  All heights are client rectangle
        heights, except for vcyWnd1, which includes the top and bottom
        borders.
     */
     vcyWnd2A=max(vcyExtLead + vcyLineToLine,
                   vcyHScrollBar + 2 * vcyBorder);

     vcyWnd2BTop=vcyBorder+vcyExtLead+2*vcyLineToLine;

     /* Note - the assumption is that numeric digits will not have
        descenders.  Therefore, we subtract out the descent when
        calculating the space below the date digits in the monthly
        calendar display.
     */
     /* changed from 6 to 9   */
     vcyWnd2BBot = 9*(6*vcyBorder+max(vcyBorder-vcyDescent, 0)
                   +vcyFont)+vcyBorder;

#ifdef DISABLE
     cyT = 11*vcyLineToLine+vcyBorder+vcyBorder;
     if (vcyWnd2BBot < cyT)
        vcyWnd2BBot = cyT;
#endif

     /* The idea is to make the boxes not look very different in size
        on the last week of the month for the 4, 5, and 6 week cases.
     */
     vcyWnd2BBot++;
     vcyWnd2B = vcyWnd2BTop + vcyWnd2BBot + vcyHScrollBar; /* last item added lsr */

     cyWnd2C = CLNNOTES * vcyLineToLine;
     vcyWnd1 = vcyBorder + (vycoNotesBox = vcyWnd2A + vcyWnd2B)
               + vcyBorder + vcyExtLead + cyWnd2C + vcyBorder;

     /* Calculate the window widths.  All widths are client rectangle
        widths, except for vcxWnd1, which includes the left and right
        borders.
        The width is determined by:
        Needed for Wnd2A in day mode:
        "<border> <time> <left arrow><border><right arrow>
            Wednesday, September 25, 1985 <border>"
        Needed for Wnd2B in day mode:
        "<border> <Bell> <time> <40 chars appointment description> \
            <scroll bar>"
        Note that the right border of the scroll bar aligns with the right
        border of Wnd1, so it shouldn't be added in twice.
     */
     vcxWnd1 = max(
                    /* width of header */
                    (cchLongDateMax + cchTimeMax + 4) * vcxFont
                                    + 2 * vcxHScrollBar + 3 * vcxBorder,
                    /* width of line of display */

                    vcxBorder + vcxBell + (40 + 4 + cchTimeMax)
                                    * vcxFont +vcxVScrollBar);


     vcxWnd2A = vcxWnd1 - 2 * vcxBorder ;


     /* Note that by adding in vcxBorder here we force the right
        border of the scroll bar to align with the border of the
        enclosing window Wnd1.
     */

     vcxWnd2B = vcxWnd2A - vcxVScrollBar + vcxBorder;

     /* Note - we know there are at least 40+4+1+cchTimeMax characters (from the width of
        wnd1) and we assume that the rest of the stuff (like the scroll
        bar and alarm bell bitmap totals at least one extra character
        (a very safe assumption).
        Leave room to the right of the last character for showing the
        caret when the edit control is full (the caret is vcxBorder
        pixels wide).
     */
     cxWnd2C = (40 + 4 + 1 + cchTimeMax) * vcxFont + vcxBorder;

     /* Calculate some x coordinates within Wnd2A.  */
#ifdef BUG_8560
     vxcoLeftArrowMax  = (vxcoLeftArrowFirst  = (7 + cchTimeMax) * vcxFont)  + vcxHScrollBar;
     vxcoRightArrowMax = (vxcoRightArrowFirst = vxcoLeftArrowMax + vcxBorder)+ vcxHScrollBar;
#else
     vxcoLeftArrowMax  = (vxcoLeftArrowFirst  = (7 + cchTimeMax) * vcxFont)  + vcxHScrollBar + vcxBorder;
     vxcoRightArrowMax = (vxcoRightArrowFirst = vxcoLeftArrowMax)+ vcxHScrollBar + vcxBorder;
#endif

     hDC=GetDC(NULL);

     vxcoDate = vxcoRightArrowMax + vcxFont;

     /* Calculate some coordinates within wnd2B for day mode. */
     GetTimeSz ( SAMPLETIME, psz);
     MGetTextExtent(hDC, vszBlank, 1, &iHeight, &iWidth);
     BlankWidth = iWidth;

     MGetTextExtent(hDC, psz, 1, &iHeight, &iWidth);
         //- KLUDGE: Divide the width of a character by two because it seems to
         //- be too large.
     //- TEMP: vxcoBell     = iWidth;
     //- TEMP: vxcoApptTime = vxcoBell + vcxBell + iWidth;
     vxcoBell     = iWidth / 2;
     vxcoApptTime = vxcoBell + vcxBell + iWidth / 2;

     MGetTextExtent(hDC, psz, 6, &iHeight, &iWidth);
     vxcoAmPm     = vxcoApptTime + iWidth -1;
     cchTimeMax   = 2*vcxFontMax / BlankWidth;

     /* Release the the DC.  */
     ReleaseDC (NULL, hDC);

     /* Calculate the x boundaries of the appointment descriptions.
        Leave room to the right of the last character for showing the
        caret when the edit control is full (the caret is vcxBorder
        pixels wide).
     */
    /*
     vxcoQdMax = (vxcoQdFirst = vxcoAmPm + 3 * vcxFont)
      + CCHQDMAX * vcxFont + vcxBorder;
    */

     //- KLUDGE: vxcoQdFirst = vxcoAmPm + 3 * vcxFontMax;
     vxcoQdFirst = vxcoAmPm + 6 * vcxFontMax;

     /* This formula limits the Moving edit control to what is visible
        and not to a number of characters.  The -2 at the end is simply
        so the caret is not flush with the scrollbar.  c-kraigb */

     vxcoQdMax=vcxWnd1 - vcxBorder - GSM(SM_CXVSCROLL)-2;

     /* Calculate the number of ln and the y boundaries of the appointment
        descriptions.  Subtract out the top and bottom borders, and subtract
        out the external leading before the first ln to see how much useable
        space there is.  Then divide to see how many ln will fit, and
        split the extra pixels between the top and the bottom.
     */
     cyUseable = vcyWnd2B - 2 * vcyBorder - vcyExtLead;
     vlnLast = (vcln = cyUseable / vcyLineToLine) - 1;
     vycoQdMax = (vycoQdFirst = 2 + (cyUseable % vcyLineToLine) / 2)
                  + vcln * vcyLineToLine;

     /* Create window CalWnd0. */
     if(!(vhwnd0 = CreateWindow((LPSTR)"CalWndMain",
                               (LPSTR)NULL,
                               WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                               min(vcxWnd1+GSM(SM_CXFRAME)*2+vcxFont*3,
                                                        GSM(SM_CXSCREEN)),
                               min(vcyWnd1+GSM(SM_CYCAPTION)+ GSM(SM_CYFRAME)*2+
                                           GSM(SM_CYMENU)+(vcyFont*2),
                                                              GSM(SM_CYSCREEN)),
                               NULL, NULL,
                               hInstance, NULL)))

        return CalTerminate(2);


     /* Create window CalWnd1. */
     if (!(vhwnd1 = CreateWindow ((LPSTR) "CalWndSub",
                                  (LPSTR) NULL,
                                  WS_VISIBLE | WS_CHILD | WS_BORDER,
                                  XcoWnd1 (), YcoWnd1 (), vcxWnd1, vcyWnd1,
                                  vhwnd0, NULL, hInstance, NULL)))
        {
        DestroyWindow(vhwnd0);
        return FALSE;
        }


     /* Create window CalWnd2A. */
     if (!(vhwnd2A = CreateWindow ((LPSTR) "CalWndSub",
                                   (LPSTR) NULL,
                                   WS_VISIBLE | WS_CHILD,
                                   0, 0, vcxWnd2A, vcyWnd2A,
                                   vhwnd1, NULL, hInstance, NULL)))
        {
        DestroyWindow(vhwnd0);
        return FALSE;
        }

#ifndef BUG_8560
     /* Create the scrollbar control as a child of hwnd2A */
     if (!(vhScrollWnd = CreateWindow ((LPSTR) "scrollbar",
                                   (LPSTR) NULL,
                                   WS_VISIBLE | WS_CHILD | SBS_HORZ,
                                   vxcoLeftArrowFirst, 0, (vcxHScrollBar + vcxBorder)<<1, vcyWnd2A,
                                   vhwnd2A, (HMENU)IDHORZSCROLL, hInstance, NULL)))
        {
        DestroyWindow(vhwnd0);
        return FALSE;
        }
#endif



     /* Create window CalWnd2B.  Note - use width of vcxWnd1 - vcxBorder
        in order to overlay the right border of the scroll bar with the
        right border of Wnd1.
     */
     if (!(vhwnd2B = CreateWindow ((LPSTR) "CalWndSub",
                                   (LPSTR)NULL,
                                   WS_VISIBLE | WS_CHILD | WS_VSCROLL| WS_HSCROLL,
                                   0, vcyWnd2A, vcxWnd1 - vcxBorder, vcyWnd2B,
                                   vhwnd1, NULL, hInstance, NULL)))
        {
        DestroyWindow(vhwnd0);
        return FALSE;
        }

     hmScrollMax = 0;
     vmScrollPos = 0;
     vmScrollMax = 0;
     hmScrollPos = 0;

     /* Create window CalWnd2C. */
     if (!(vhwnd2C = CreateWindow ((LPSTR) "Edit",
                                   (LPSTR)NULL,
                                   WS_VISIBLE | WS_CHILD | ES_MULTILINE,
                                   vxcoWnd2C = (vcxWnd2A - cxWnd2C) / 2,
                                   vycoWnd2C = vcyWnd1 - 2 * vcyBorder - cyWnd2C,
                                   cxWnd2C, cyWnd2C,
                                   vhwnd1, (HMENU)IDECNOTES, hInstance, NULL)))
        {
        DestroyWindow(vhwnd0);
        return FALSE;
        }



     /* limit text in notes area to number of chars that fit.
      * this is done to get around a bug in edit controls when
      * you paste in text which has tabs.
      * 05-Oct-1987. davidhab.
      */
#if VARIABLENOTELENGTH
     SendMessage(vhwnd2C, EM_LIMITTEXT, (cyWnd2C / vcyFont) * (cxWnd2C / vcxFont)*2, 0L);
#else
     SendMessage(vhwnd2C, EM_LIMITTEXT, CBNOTESMAX, 0L);
#endif


     /* Create window CalWnd3. c-kraigb : ES_AUTOHSCROLL added to allow
        more text in the appointment area without having to alter the
        size of the entire layout.*/

     if (!(vhwnd3 = CreateWindow ((LPSTR) "Edit",
                                  (LPSTR)NULL,
                                  WS_CHILD | ES_AUTOHSCROLL,
                                  0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                  vhwnd2B, (HMENU)IDECQD, hInstance, NULL)))

        {
        DestroyWindow(vhwnd0);
        return FALSE;
        }



     /* Limit the Appoinment Edit to it's maximum.  80 chars should be
        sufficient for all intensive purposes , c-kraigb
      */
     SendMessage(vhwnd3, EM_LIMITTEXT, CCHQDMAX, 0L);

     /* Make all the files look closed. */
     for (i=0; i < CFILE; i++)
         hFile[i]=-1;

     /* Get a handle for the tdd.  Note - the tdd will get initialized
            when we call CleanSlate or LoadCal below.
      */
         //- Calinit: Changed Allocation because of bug in LocalReAlloc.
     //- vhlmTdd = LocalAlloc (LMEM_MOVEABLE, 0);

     /* Fetch the current date and time. */
     ReadClock(&vd3Cur, &vftCur.tm);
     vftCur.dt = DtFromPd3 (&vd3Cur);

     /* Set up the timer event for updating the time and date. */

#ifdef DOTIMER
     if (!SetTimer (vhwnd0, 0, 1000, (TIMERPROC)NULL))
        {
        AlertBox (vrgsz[IDS_NOTIMER], (CHAR *)NULL,
                  MB_SYSTEMMODAL | MB_OK | MB_ICONHAND);
        DestroyWindow(vhwnd0);
        return FALSE;
        }
#endif /* #ifdef DOTIMER */

     /* init. some fields of the OPENFILENAME struct used by fileopen and
      * filesaveas
      */
/* changed to sizeof instead of constant.  18 Jan 1991  clarkc */
     vOFN.lStructSize       = sizeof(OPENFILENAME);
     vOFN.hwndOwner         = vhwnd0;
     vOFN.lpstrFileTitle    = 0;
     vOFN.nMaxCustFilter    = CCHFILTERMAX;
     vOFN.nFilterIndex      = vFilterIndex;
     vOFN.nMaxFile          = CCHFILESPECMAX;
     vOFN.lpfnHook          = NULL;

     /* init fields of the PRINTDLG structure */
/* changed to sizeof instead of constant.  18 Jan 1991  clarkc */
     vPD.lStructSize        = sizeof(PRINTDLG);
     vPD.hwndOwner          = vhwnd0;
     vPD.hDevMode           = NULL;
     vPD.hDevNames          = NULL;
     vPD.hDC                = NULL;
     vPD.Flags              = PD_NOSELECTION | PD_NOPAGENUMS; /* disable "pages" and "Selection" radiobuttons */
     vPD.nFromPage          = 0;
     vPD.nToPage            = 0;
     vPD.nMinPage           = 0;
     vPD.nMaxPage           = 0;
     vPD.nCopies            = 1;

     /* determine the message number to be used for communication with
      * Find dialog
      */
     if (!(vHlpMsg = RegisterWindowMessage ((LPSTR)HELPMSGSTRING)))
          return FALSE;

     /* Prevent grabbing of focus if we are coming up iconic. */
     vfNoGrabFocus = (cmdShow == SW_SHOWMINNOACTIVE);


     /* Initialize according to values from win.ini */
     CalWinIniChange();

    if (ProcessShellOptions(lpszCmdLine))
    {
        PostMessage(vhwnd0, WM_CLOSE, 0, 0L);
        return (TRUE);
    }
    else if (*lpszCmdLine == '\0')      /* Not invoked with a file. */
    {
        CleanSlate (TRUE);
    }
    else
    {
            /* Try to load the file. */
            AddDefExt(lpszCmdLine);

            hFile [IDFILEORIGINAL] = MOpenFile (lpszCmdLine,
                    (OFSTRUCT FAR *)&OFStruct [IDFILEORIGINAL], OF_READ);
            LoadCal ();
    }

    /* If we didn't grab the focus we need to set up vhwndFocus so
                we correctly set the focus when we get activated later on.
                Initially we are in day mode with the focus on the appointment
                edit control.
    */
    if (vfNoGrabFocus)
    {
        vhwndFocus = vhwnd3;
        vfNoGrabFocus = FALSE;
    }


    /* Make the windows visible. */
    ShowWindow (vhwnd0, cmdShow);
    return (TRUE);
}


BOOL APIENTRY   ProcessShellOptions(
        LPSTR lpszCmdLine)
{
        CHAR szFileName[80];

        AnsiUpper(lpszCmdLine);

        if (*lpszCmdLine != '/' || *lpszCmdLine != 'P')
                return FALSE;

        lpszCmdLine += 2;

        // skip blanks
    while (*lpszCmdLine == ' ' || *lpszCmdLine == '\t')
        lpszCmdLine++;

        if (!*lpszCmdLine)
                return FALSE;

    /* Get the filename. */
        lstrcpy((LPSTR)szFileName, lpszCmdLine);
        AddDefExt(szFileName);

        hFile[IDFILEORIGINAL] = MOpenFile (szFileName, &OFStruct [IDFILEORIGINAL],
                        OF_READ);
        if (hFile[IDFILEORIGINAL] == -1)
        {
        AlertBox (vszCannotReadFile, szFileName, MB_SYSTEMMODAL | MB_OK | MB_ICONEXCLAMATION);
                return TRUE;
        }

        LoadCal();

        // need to set the first and last dates for which appts have to be
        // printed.
        // Since the data structures are utterly incomprehensible,
        // the following method appeared to provide the easiest solution
        vdtFrom = DTFIRST;              // from 1/1/1980
        vdtTo = DTLAST;                 // to December 31, 2099.

    FSearchTdd (vdtFrom, &vitddFirst);  // first day in the file
    FSearchTdd (vdtTo, &vitddMax);              // last day in the file
        Print ();

        return TRUE;
}

/**** AllocDr ****/

BOOL APIENTRY AllocDr ()
     {
     register INT i;

     for (i=0; i < CDR; i++)
          {
          /* Note - the DRs get marked free by New. */
          if (!(vrghlmDr [i] = LocalAlloc (LMEM_MOVEABLE | LMEM_ZEROINIT, CBDRMAX)))
                return FALSE;
          }
          return TRUE;
     }




/**** Return TRUE iff all bitmaps loaded.  If we fail to load any bitmap,
      DeleteBitmaps will deleted the ones already loaded. */
BOOL APIENTRY LoadBitmaps(HANDLE  hInstance)
    {
    vhbmLeftArrow = LoadBitmap (NULL, MAKEINTRESOURCE (OBM_LFARROW));
    vhbmRightArrow = LoadBitmap (NULL, MAKEINTRESOURCE (OBM_RGARROW));
    vhbmBell = LoadBitmap (hInstance, MAKEINTRESOURCE(1));
    return (vhbmBell && vhbmLeftArrow && vhbmRightArrow);
    }



/**** Delete bitmaps that have been loaded.  Depends on handles having been
      initialized to 0. */
VOID APIENTRY DeleteBitmaps()
    {
    if (vhbmBell)
        DeleteObject(vhbmBell);

    if (vhbmLeftArrow)
        DeleteObject(vhbmLeftArrow);

    if (vhbmRightArrow)
        DeleteObject(vhbmRightArrow);
    }

/**** Destroy Global Objects.
      iLevel determines which objects will be deleted.
      Always returns FALSE */
BOOL APIENTRY CalTerminate(INT iLevel)
    {
    /* Note case statement falls thru every arm. */
    switch (iLevel)
        {
        case 2:
            DeleteDC (vhDCMemory);

        case 1:
            DeleteBitmaps();

        case 0:
            DestroyBrushes();
        }

    /* Leave handy value in ax for calinit to return. */
    return FALSE;
    }
