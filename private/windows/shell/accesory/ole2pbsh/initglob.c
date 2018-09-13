/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   INITGLOB.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  windows main global init routine            *
*   date:   04/07/87 @ 15:30                            *
*                                                       *
********************************************************/

#include <string.h>
#include <windows.h>
#include <port1632.h>
#include <direct.h>
#include <shellapi.h>

#include "oleglue.h"
#include "pbrush.h"

#ifdef PENWIN
#include <penwin.h>
#endif

#if defined(DBCS_IME) && defined(KOREA)
#include <ime.h>
#endif

//
// CG: the following is THE hmenu for this App.
//     (added in process of rationalizing menu access in order to
//      make the spliced InPlace menu work correctly)
//
HMENU ghMenuFrame = NULL;
//
// CG: the following array indicates whether the coresponding menu
//     is actually present (owned by us) in the ghMenuFrame
//
BOOL gafMenuPresent[MAXmenus];

extern BOOL drawing, moving;
extern BOOL inMagnify, bZoomedOut;
extern WORD wFileType;
extern HPALETTE hPalette;
extern int theTool, theSize, theForeg, theBackg, theBrush;
extern int cursTool;
extern POINT aspect;
extern struct csstat CursorStat;
extern DWORD colorColor[], bwColor[];
extern DWORD defltColor[], defltBW[];
extern HWND pbrushWnd[MAXwnds];
extern TCHAR pgmTitle[], *pbrushWndClass[];
extern TCHAR NotEnoughMem[];
extern WNDPROC lpMouseDlg;
extern WNDPROC lpColorDlg;
extern WNDPROC lpNullWP;
extern int FontHeight;
extern LOGFONT theFont;
extern RECT pbrushRct[];
extern TCHAR *cuArray[];
extern BOOL bIsPrinterDefault;
extern int horzDotsMM, vertDotsMM;
extern DWORD *rgbColor;
extern DPPROC dpArray[];
extern int nNewImageWidth, nNewImageHeight, nNewImagePlanes, nNewImagePixels;
extern TCHAR *namePtr, *pathPtr, noFile[];
extern TCHAR fileName[], tempName[];
extern TCHAR filePath[], tempPath[];
extern TCHAR winIniAppName[], winIniHelpName[];
extern TCHAR winIniOmitPictureFormat[];
extern TCHAR winIniHeightName[], winIniWidthName[], winIniClrName[];
extern DPPROC DrawProc;
extern int nSizeNum,nSizeDen;
extern RECT imageRect;
extern TCHAR deviceStr[];
extern int imageWid, imageHgt;
extern TCHAR szHeader[], szFooter[];
extern BOOL bFileLoaded;
extern int fOmitPictureFormat;

#ifdef JAPAN
extern BOOL bWritable; // This flag identifies whether the Font is possible
                       // to write vertically or not.
extern BOOL bVertical; // This flag identifies whether user chose
                       // the Vertical-Writing option or not.
#endif

/* pen windows */
VOID (FAR PASCAL *lpfnRegisterPenApp)(WORD, BOOL) = NULL;
#ifdef PENWIN
int (FAR PASCAL *lpfnProcessWriting)(HWND, LPRC) = NULL;
BOOL (FAR PASCAL *lpfnCorrectWriting)(HWND, LPTSTR, int, LPRC, DWORD, DWORD) = NULL;
BOOL (FAR PASCAL *lpfnTPtoDP)(LPPOINT, int) = NULL;
BOOL (FAR PASCAL *lpfnIsPenEvent)( WORD, LONG) = NULL;

#endif

PRINTDLG PD;

/* Function to determine if the command line passed to PBRUSH contains the
   print file command, i.e. "pbrush /p <filename>".
 */
BOOL NEAR PASCAL IsCommandPrint(LPTSTR lpszCmdLine)
{
        LPTSTR tmpCmd = lpszCmdLine;

        while(*tmpCmd != TEXT('/') && *tmpCmd != TEXT('\0'))
                tmpCmd++;
        if(!*tmpCmd) {
            return FALSE;
        } else {
            if(tmpCmd[1] == TEXT('p') || tmpCmd[1] == TEXT('P')) {
                LPTSTR p;

                /* get rid of /p  */
                p = tmpCmd;
                tmpCmd += 2;

                while( *tmpCmd == TEXT(' ') || *tmpCmd == TEXT('\t') )
                    tmpCmd++;

                while(*tmpCmd)
                        *p++ = *tmpCmd++;
                *p = TEXT('\0');
                return TRUE;
            } else
                return FALSE;
        }
}

#define SBYTE(l,b) ((int)((l>>(b*8))&0xffL))

static int cntr;

short FAR PASCAL EnumPens(LOGPEN FAR *lpLogPen, COLORREF FAR *lpData)
{
   if(lpLogPen->lopnStyle == PS_SOLID
         && !(lpLogPen->lopnColor&0xff000000L)) {
      if(cntr < MAXcolors)
         lpData[cntr++] = lpLogPen->lopnColor;
      else
         return(FALSE);
   }

   return(TRUE);
}

static void NEAR PASCAL FindBestColors(HDC hDC, HANDLE hInstApp)
{
   long nPens;
   FARPROC lpfnEnumPens;
   int i, j, indexi, indexj, nDiff, maxDiff;
   COLORREF dCol, rCol;
   COLORREF FAR *devColors;
   BOOL FAR *devUsed, reqUsed[MAXcolors];
   HANDLE hDevColors, hDevUsed;

   nPens = GetDeviceCaps(hDC, NUMPENS);
   if (nPens == -1)    /* too many? use the default color table */
       return;
   if (!(hDevColors = GlobalAlloc(GMEM_MOVEABLE, nPens * sizeof(COLORREF))))
       return;
   if (!(hDevUsed = GlobalAlloc(GMEM_MOVEABLE, nPens * sizeof(BOOL))))
       return;

   devColors = (COLORREF FAR *)GlobalLock(hDevColors);
   devUsed = (BOOL FAR *)GlobalLock(hDevUsed);

   cntr = 0;

   lpfnEnumPens = MakeProcInstance((FARPROC)EnumPens, hInstApp);
   EnumObjects(hDC, OBJ_PEN, (GOBJENUMPROC)lpfnEnumPens, (LPARAM)devColors);
   FreeProcInstance(lpfnEnumPens);

   for (i = 0; i < MAXcolors; ++i)
      reqUsed[i] = FALSE;
   for (i = 0; i < (int)nPens; ++i)
      devUsed[i] = FALSE;

   for (i = 0; i < MAXcolors; ++i) {
      maxDiff = 255 + 255 + 255 + 1;

   /* find the color in our requested table that matches the closest */
   for(j = 0; j < cntr; ++j) {
         if(devUsed[j])
            continue;

         dCol = devColors[j];
         rCol = defltColor[i];
         nDiff = abs(SBYTE(dCol, 0) - SBYTE(rCol, 0))
               + abs(SBYTE(dCol, 1) - SBYTE(rCol, 1))
               + abs(SBYTE(dCol, 2) - SBYTE(rCol, 2));
         if(maxDiff > nDiff) {
            maxDiff = nDiff;
            indexi = i;
            indexj = j;
         }
      }

      if(maxDiff < 64) {
         defltColor[indexi] = devColors[indexj];
         devUsed[indexj] = TRUE;
         reqUsed[indexi] = TRUE;
      }
   }

   while(1) {
      maxDiff = 255 + 255 + 255 + 1;

      for(j = 0; j < cntr; ++j) {
         if(devUsed[j])
            continue;

         /* find the color in our requested table that matches the closest */
         for(i = 0; i < MAXcolors; ++i) {
            if(reqUsed[i])
               continue;

            dCol = devColors[j];
            rCol = defltColor[i];
            nDiff = abs(SBYTE(dCol, 0) - SBYTE(rCol, 0))
                  + abs(SBYTE(dCol, 1) - SBYTE(rCol, 1))
                  + abs(SBYTE(dCol, 2) - SBYTE(rCol, 2));
            if(maxDiff > nDiff) {
               maxDiff = nDiff;
               indexi = i;
               indexj = j;
            }
         }
      }

      if(maxDiff >= 255 + 255 + 255 + 1)
         break;

      defltColor[indexi] = devColors[indexj];
      devUsed[indexj] = TRUE;
      reqUsed[indexi] = TRUE;
   }

   GlobalUnlock(hDevColors);
   GlobalUnlock(hDevUsed);
   GlobalFree(hDevColors);
   GlobalFree(hDevUsed);
}

WndInitGlob(HINSTANCE hInst, LPTSTR lpCmdLine, int cmdShow)
{
   HDC parentDC, hdcPrint;
   int wid, hgt;
   int i;
   UINT numcolors;

   TCHAR szIniColor[10];
   TCHAR ach[40];
   BOOL bColor;
   RECT rWind;
   BOOL fCommandLinePrint;

   /* Let Commdlg retrieve default printer so we can modify printer
    * settings with hDevMode.
    */
    // Initialize the PRINTDLG structure
   PD.lStructSize       = sizeof(PRINTDLG);
   PD.hDevMode          = NULL;
   PD.hDevNames         = NULL;
   PD.hInstance         = hInst;
   PD.hwndOwner         = NULL;
   PD.nCopies           = 1;


   /* Able to print a file from command line if of form "pbrush /p <file>" */
   fCommandLinePrint = IsCommandPrint(lpCmdLine);
   if(fCommandLinePrint)
      bFileLoaded = FALSE;      /* Ensure a file is loaded before printing. */

   /* Initialize the new dialogs */
   InitNewDialogs(hInst);

   LoadString(hInst, IDSNotEnufMem, NotEnoughMem, CharSizeOf(NotEnoughMem));
   LoadString(hInst, IDSuntitled, noFile, CharSizeOf(noFile));

    /* miscellaney */
   drawing = FALSE;
   moving = FALSE;
   inMagnify = FALSE;
   bZoomedOut = FALSE;
   wFileType = BITMAPFILE4;
   hPalette = NULL;

#ifdef JAPAN
   /* intialize flags for the Vertical-Writing option */
   bWritable = bVertical = FALSE;
#endif

   /* initialize command variables */
   theTool = BRUSHtool;
   cursTool = BRUSHtool;
   DrawProc = BrushDP;
   theForeg = 1;
   theBackg = 0;
   theSize = 2;
   aspect.x = theSize + 1;
   aspect.y = theSize + 1;
   theBrush = RECTbrush;
   CursorStat.allowed = TRUE;
   CursorStat.noted = FALSE;
   /* create parent window */
   pbrushWnd[PARENTid] = CreateWindow(pbrushWndClass[PARENTid],
                                     pgmTitle,
                                     WS_TILEDWINDOW,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     (HWND)NULL, (HMENU)NULL,
                                     hInst, (LPVOID)NULL);

   if (!pbrushWnd[PARENTid])
   {
      DOUT(L"PBrush: WndInitGlob [-1]\r\n");

      FreeImg();
      PbrushOkError(IDSNotMemAvail, MB_OK | MB_ICONASTERISK);
      return(FALSE);
   }

    //
    // Initialize our global menu handle and it's associated array
    //
    ghMenuFrame = GetMenu(pbrushWnd[PARENTid]);
    {
        int i;
        for(i = 0; i < MAXmenus; i++)
            gafMenuPresent[i] = TRUE;
    }

   /* This would be a way low memory situation
    */
#ifndef WIN32
   if (!(lpMouseDlg = MakeProcInstance(MouseDlg,hInst))
         || !(lpColorDlg = MakeProcInstance(ColorDlg,hInst))
         || !(lpNullWP = MakeProcInstance(NullWP, hInst)))
      goto NoMemory;
#endif


   /* All processing of drag/drop files is done the under WM_DROPFILES*/
   DragAcceptFiles(pbrushWnd[PARENTid],TRUE); /* Proccess dragged and dropped files. */
   PD.hwndOwner = pbrushWnd[PARENTid];

#ifdef PENWIN
        /* these are needed over a wide span of time */
        if (lpfnRegisterPenApp = GetProcAddress((HANDLE)GetSystemMetrics(SM_PENWINDOWS),
            "RegisterPenApp"))
        (*lpfnRegisterPenApp)(1, TRUE);
        {
           lpfnProcessWriting = GetProcAddress(GetSystemMetrics(SM_PENWINDOWS), "ProcessWriting");
           lpfnCorrectWriting = GetProcAddress(GetSystemMetrics(SM_PENWINDOWS), "CorrectWriting");
           lpfnTPtoDP = GetProcAddress(GetSystemMetrics(SM_PENWINDOWS), "TPtoDP");
           lpfnIsPenEvent = GetProcAddress(GetSystemMetrics(SM_PENWINDOWS), "IsPenEvent");
        }
#endif


   /* initialize the logical font */
   GetObject (GetStockObject (SYSTEM_FONT), sizeof(LOGFONT), &theFont);
   theFont.lfCharSet = ANSI_CHARSET;
   lstrcpy (theFont.lfFaceName, LOCAL_FONT_NAME);
   FontHeight = theFont.lfHeight;

   /* calculate max size of paint window (zoomed) */
   /* 09-Jan-1993 - Jonma - change this to SM_C<n>FULLSCREEN so that
       we can paste in full-screen bitmaps */
#ifdef OLDCODE
   wid = GetSystemMetrics(SM_CXFULLSCREEN);
   hgt = GetSystemMetrics(SM_CYFULLSCREEN) - GetSystemMetrics(SM_CYMENU);
#endif
   wid = GetSystemMetrics(SM_CXSCREEN);
   hgt = GetSystemMetrics(SM_CYSCREEN);
   pbrushRct[PARENTid].right = wid;
   pbrushRct[PARENTid].bottom = hgt;

   for (i = 0; i < MAXtools; ++i)
      cuArray[i] = (TCHAR *) NULL;
   cuArray[ROLLERtool] = TEXT("flood");
   cuArray[SCISSORStool] = TEXT("crossh");
   cuArray[PICKtool] = TEXT("crossh");
   cuArray[AIRBRUSHtool] = TEXT("crossh");
   cuArray[CURVEtool] = TEXT("crossh");
   cuArray[LINEtool] = TEXT("crossh");
   cuArray[RECTFRAMEtool] = TEXT("crossh");
   cuArray[RECTFILLtool] = TEXT("crossh");
   cuArray[RNDRECTFRAMEtool] = TEXT("crossh");
   cuArray[RNDRECTFILLtool] = TEXT("crossh");
   cuArray[OVALFRAMEtool] = TEXT("crossh");
   cuArray[OVALFILLtool] = TEXT("crossh");
   cuArray[POLYFRAMEtool] = TEXT("crossh");
   cuArray[POLYFILLtool] = TEXT("crossh");
#ifdef JAPAN
   cuArray[TEXTtool] = TEXT("ibeam2");        // cursor for vertical writing.
#else
   cuArray[TEXTtool] = TEXT("text");
#endif


   /* font and printer initializers */
   if( hdcPrint = GetPrtDC() )
        DeleteDC( hdcPrint );

   bIsPrinterDefault = TRUE;

   if (parentDC = GetDisplayDC (pbrushWnd[PARENTid]))
   {
      /* calculate screen dimension ratios by getting twips mode extents */
      horzDotsMM = (int) ((GetDeviceCaps(parentDC, HORZRES) * 100L)
                          / GetDeviceCaps(parentDC, HORZSIZE));
      vertDotsMM = (int) ((GetDeviceCaps(parentDC, VERTRES) * 100L)
                          / GetDeviceCaps(parentDC, VERTSIZE));

      numcolors = (UINT)GetDeviceCaps(parentDC, NUMCOLORS);
      FindBestColors(parentDC, hInst);

      ReleaseDC(pbrushWnd[PARENTid], parentDC);
   }
   else
   {
      horzDotsMM = 100;
      vertDotsMM = 100;

      numcolors = 2;

      PbrushOkError(IDSNoDC, MB_ICONHAND);
   }

   /* initialize color pattern vectors */
   for(i = 0; i < MAXcolors; ++i) {
      colorColor[i] = defltColor[i];
      bwColor[i] = defltBW[i];
   }

   pbrushWnd[PAINTid] = (HWND)NULL;   /* turn off rewrite for a bit */


    //
    // CG: Manage OLE2 Startup
    //
    if(!InitializePBS(hInst, lpCmdLine))
    {
        DOUT(L"PBrush: InitializePBS Failed!\r\n");
        return(FALSE);
    }
    if(gfStandalone)
    {
        // Start file loading if one was specified on command line
        SetupFileVars(lpCmdLine);
        lstrcpy(gachLinkFilename, fileName);
        CreateStandaloneObject();
    }
    else if(gfLinked)
    {
        lstrcpy(fileName, gachLinkFilename);
    }


   // calculate child window dimensions
   CalcWnds(SHOWWINDOW, SHOWWINDOW, SHOWWINDOW, SHOWWINDOW);
   for(i = 1; i < MAXwnds; ++i)
   {
      //
      // create and show child windows
      //
      pbrushWnd[i] = CreateWindow(pbrushWndClass[i],
                        (LPTSTR)NULL,
                        WS_CHILD | ((TOOLid == i) ? 0L : WS_BORDER) |
                        ((PAINTid == i) ? (WS_VSCROLL | WS_HSCROLL) : 0L),
                        pbrushRct[i].left, pbrushRct[i].top,
                        pbrushRct[i].right - pbrushRct[i].left,
                        pbrushRct[i].bottom - pbrushRct[i].top,
                        pbrushWnd[PARENTid], (HMENU)i, hInst, (LPVOID)NULL);
      if(pbrushWnd[i] == NULL) {
         FreeImg();
         PbrushOkError(IDSNotMemAvail, MB_OK | MB_ICONASTERISK);
         goto DestroyParent;
      }
      ShowWindow(pbrushWnd[i], SHOW_OPENWINDOW);
   }

   GetProfileString(winIniAppName, winIniClrName, TEXT("COLOR"), szIniColor, CharSizeOf(szIniColor));

   bColor = (numcolors > 2) ? (!lstrcmpi(TEXT("COLOR"), szIniColor)) : FALSE;
   if(bColor)
      rgbColor = colorColor;
   else {
      dpArray[COLORERASERtool] = EraserDP;
      rgbColor = bwColor;
   }

   /* Copy metafile format during Edit.Cut/Copy ? */
   fOmitPictureFormat = GetProfileInt(winIniAppName, winIniOmitPictureFormat, FALSE);

   nNewImageWidth = GetProfileInt(winIniAppName, winIniWidthName, wid);
   nNewImageHeight = GetProfileInt(winIniAppName, winIniHeightName, hgt);
   nNewImagePlanes = (bColor ? 0 : 1);
   nNewImagePixels = (bColor ? 0 : 1);

   /* display program title */
   SetTitle(noFile);

   /* Set default extensions for .pcx and .bmp files */
   if (!GetProfileString(TEXT("extensions"), TEXT("bmp"), TEXT(""), ach, CharSizeOf(ach)))
      WriteProfileString(TEXT("extensions"), TEXT("bmp"), TEXT("pbrush.exe ^.bmp"));
   if (!GetProfileString(TEXT("extensions"), TEXT("pcx"), TEXT(""), ach, CharSizeOf(ach)))
      WriteProfileString(TEXT("extensions"), TEXT("pcx"), TEXT("pbrush.exe ^.pcx"));

   InitDecimal(NULL);

   /* the SIZE message puts the windows in the right spot */
   GetClientRect(pbrushWnd[PARENTid], &rWind);
   SendMessage(pbrushWnd[PARENTid], WM_SIZE,
                   SIZENORMAL, MAKELONG(rWind.right, rWind.bottom));

   /* the WININICHANGE message initializes the font menu and the printers */
   SendMessage(pbrushWnd[PARENTid], WM_WININICHANGE, 0, 0L);


    /* the FILEnew message creates a new image */
    if (!(*fileName))
        SendMessage(pbrushWnd[PARENTid], WM_COMMAND, FILEnew, 0L);
    else
        SendMessage(pbrushWnd[PARENTid], WM_COMMAND, FILEload, 0L);

#if defined(DBCS_IME) && defined(KOREA)
    /* REVIEW: Correct this position for setting level 90.12.17 by sangl */
    {
        HANDLE  hKs;
        LPIMESTRUCT lpKs;

        hKs = GlobalAlloc (GMEM_MOVEABLE|GMEM_DDESHARE,(LONG)sizeof(IMESTRUCT));
        lpKs = (LPIMESTRUCT)GlobalLock(hKs);
        lpKs->fnc = IME_SETLEVEL;
        lpKs->wParam = 3;
        GlobalUnlock(hKs);
        SendIMEMessage (pbrushWnd[PARENTid], MAKELONG(hKs,0));
        GlobalFree(hKs);
    }
#endif

#ifdef JAPAN        // added by Hiraisi
    {
        extern int xScreen, yScreen;

        xScreen = GetSystemMetrics(SM_CXSCREEN);
        yScreen = GetSystemMetrics(SM_CYSCREEN);
    }
#endif

#ifdef JAPAN         // added by Hiraisi  07 Apr. 1992 : jinwoo 11/9/92
    {
        extern void CheckVerticalWriting( HDC, LPSTR );
        extern LOGFONT vertFont;    // for vertical writing.
        extern LOGFONT horiFont;    // for horizontal writing.
        extern int nonBoldWeight;
        HDC hDC;

         hDC = GetDC(pbrushWnd[PARENTid]);
         CheckVerticalWriting( hDC, theFont.lfFaceName );
         if( bVertical )
             theFont = vertFont;
         ReleaseDC(pbrushWnd[PARENTid], hDC);
         if( theFont.lfWeight != FW_BOLD )        //  added  06 Jul. 1992
             nonBoldWeight = theFont.lfWeight;
    }
#endif


    if (fCommandLinePrint && bFileLoaded)
    {
        SendMessage(pbrushWnd[PARENTid], WM_COMMAND, GET_WM_COMMAND_MPS(FILEprint,0,0));
//CloseParent:
        SendMessage(pbrushWnd[PARENTid], WM_CLOSE, 0, 0L);
        return(FALSE);
    } else if(gfStandalone) {
        /* activate and display parent window */
        RegisterAsDropTarget(gpahwndApp[iPaint]);
        ShowWindow(pbrushWnd[PARENTid], cmdShow);
        UpdateWindow(pbrushWnd[PARENTid]);
        return TRUE;
    } else
        return TRUE;

DestroyParent:
   DestroyWindow(pbrushWnd[PARENTid]);
   return(FALSE);
}

void SetupFileVars(
    LPTSTR szPath)
{
    LPTSTR lpStr, pPtr;
    int cch;

    fileName[0] = TEXT('\0');
    filePath[0] = TEXT('\0');
    if (*szPath)
    {
        DB_OUTF((acDbgBfr, TEXT("SetupFileVars:szPath: %s\n"), szPath));

        /* add .BMP extension if one doesn't exist */
        MakeValidFilename(szPath, TEXT(".BMP"));

        /* search backwards for path seperator of beginning of string */
        for (lpStr = szPath + lstrlen(szPath);
             lpStr > szPath && !IsPathSep(*lpStr);
             lpStr = CharPrev(szPath, lpStr))
             /* do nothing */ ;

        /* if we are sitting on path character move forward */
        if (IsPathSep(*lpStr))
            lpStr = CharNext(lpStr);

        /* save the filename */
        namePtr = fileName;
        lstrcpy (namePtr, lpStr);

        /* save the path */
        pPtr = pathPtr = filePath;

        // if it's the root
        if (IsPathSep(szPath[0]) && !IsPathSep(szPath[1]))
        {
            *pPtr++ = _getdrive() + TEXT('a') - 1;
            *pPtr++ = TEXT(':');
        }

        // copy up to the name
        cch = lpStr - szPath;
        CopyMemory (pPtr, szPath, ByteCountOf(cch));
        pPtr += cch;
        *pPtr = TEXT('\0');

        if (pPtr != pathPtr) {
            pPtr -= 1;

            // wipe out the trailing '\' if we are not on the root dir
            if ((*pPtr != TEXT('\\') && *pPtr != TEXT('/')) ||
                    (pPtr > pathPtr && pPtr[-1] == TEXT(':'))) {
                pPtr++;
            }
        }

        *pPtr = TEXT('\0');

        DB_OUTF((acDbgBfr,TEXT("SetupFileVars:namePtr: %s\n"), namePtr ));
        DB_OUTF((acDbgBfr, TEXT("SetupFileVars:pathPtr: %s\n"), pathPtr));

        /* if there was no path get the current path */
        if (!*pathPtr)
              GetCurrentDirectory (PATHlen, pathPtr);

        /* load the file */
        *tempName = *tempPath = TEXT('\0');
    }
}
