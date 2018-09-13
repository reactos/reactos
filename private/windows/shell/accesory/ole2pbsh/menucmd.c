/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   MenuCmd.c                                   *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  processes menu commands                     *
*   date:   04/01/87 @ 18:15                            *
********************************************************/

#include <windows.h>
#include <port1632.h>
#include <shellapi.h>

#include "oleglue.h"
#include "pbrush.h"

#ifdef WIN32
#define strcpy lstrcpy
#endif


extern HWND pbrushWnd[], mouseWnd, colorWnd, hDlgModeless;
extern WNDPROC lpNullWP;
extern RECT pbrushRct[];
extern int paintWid, paintHgt, zoomAmount;
extern WORD wFileType;
extern DPPROC dpArray[];
extern DWORD *rgbColor;
extern DWORD colorColor[], bwColor[];
extern TCHAR fileName[], clipName[], tempName[];
extern TCHAR *wildCard, *namePtr, *pathPtr, noFile[];
extern int defaultWid, defaultHgt;
extern BOOL gfDirty;
extern TCHAR filePath[], clipPath[], colorPath[];
extern int DlgCaptionNo;
extern HPALETTE hPalette;
extern int nNewImageWidth, nNewImageHeight, nNewImagePlanes, nNewImagePixels;
extern int fileMode;
extern int imageWid, imageHgt, imageByteWid, imagePlanes, imagePixels;
extern HWND zoomOutWnd;
extern int theTool;
extern LOGFONT theFont;
extern BOOL inMagnify, clearFlag, mouseFlag, outline, shadow, bZoomedOut;
extern DPPROC DrawProc;
extern int cursTool;
extern LPTSTR DrawCursor;
extern WNDPROC lpColorDlg;
extern WNDPROC lpMouseDlg;
extern PRINTDLG PD;
extern BOOL bFileLoaded;

#ifdef JAPAN
#include <dlgs.h>           // added 20 May. 1992
void CheckVerticalWriting( HDC, LPSTR );
//int FAR PASCAL _export DeleteFacename( HWND , UINT , WPARAM , LPARAM );
int FAR PASCAL DeleteFacename( HWND , UINT , WPARAM , LPARAM );
BOOL bWritable; // This flag identifies whether the Font is possible to write
                // vertically or not.
BOOL bVertical; // This flag identifies whether user chose the Vertical-Writing
                // option or not.
extern LOGFONT vertFont;    // for vertical writing.
extern LOGFONT horiFont;    // for horizontal writing.

int nonBoldWeight; //  This weight is used instead of FW_MEDIUM, because some
                   // TT fonts have FW_LIGHT weight.      06 Jul. 1992
#endif

HDC printDC;
static HWND fullWnd;
BOOL IsCanceled;
static TCHAR    szTemplate[WARNMSGMAX*3];
static TCHAR    szMessage[WARNMSGMAX*4];

void PUBLIC Help(HWND hWnd, UINT wCommand, LONG lParam)
{
   TCHAR szHelpPath[100], *pPath;
   BOOL result;

   SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);

   pPath = szHelpPath
         + GetModuleFileName(hInst, szHelpPath, CharSizeOf(szHelpPath));
#ifdef DBCS
   while (*pPath != TEXT('.'))                  /* DBCS */
       pPath = CharPrev(szHelpPath,pPath);
#else
   while (*pPath-- != TEXT('.'))
      ;
   ++pPath;
#endif
   *++pPath = TEXT('H');
   *++pPath = TEXT('L');
   *++pPath = TEXT('P');
   *++pPath = TEXT('\0');

   result = WinHelp(hWnd, szHelpPath, wCommand, lParam);
   if(!result)
        SimpleMessage(IDSNotEnufMem, NULL, MB_OK|MB_ICONHAND|MB_SYSTEMMODAL);
}

/* they all do this pretty much to clean up the screen */
void PUBLIC Terminatewnd(
    void)
{
   SendMessage(pbrushWnd[PAINTid], WM_MOUSEMOVE, 0, -1L);
   SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);
}

/* dummy window function.  Used to shield input if virtual allocations fail.
*/
BOOL FAR PASCAL NullWP(HWND hWnd, UINT message, WPARAM wParam, LONG lParam)
{
   switch(message) {
   case WM_SHOWWINDOW:
      break;

   default:
      return((BOOL)DefWindowProc(hWnd,message,wParam,lParam));
   }

   return(FALSE);
}

static INT EnPaMeIt1[] = {FILEsave, FILEsaveas, FILEpage,
                          FILEprinter, MISCbrush, MISCeditColor, MISCgetColor, MISCsaveColor, -1};
static INT EnPaMeIt2[] = {1, 2, 3, -1};
static INT MiZoInEn1[] = {EDITcopypict, EDITcutpict,
                          EDITcopyTo, EDITpasteFrom, MISCzoomIn, FILEshow, WINDOWtool,
                          WINDOWpalette, MISCmousePos, -1};
static INT MiZoInEn2[] = {0, 3, 4, 5, -1};
static INT MiZoOtEn1[] = {ZOOMundo, EDITcopypict, EDITcutpict,
                          EDITcopyTo, MISCzoomOut, -1};
static INT MiEdCoEn[]  = {0, 1, 2, 3, 4, 5, -1};


static void PRIVATE EnablePaintWindow(BOOL bEnable)
{
   REGISTER HWND hWnd = pbrushWnd[PARENTid];

  //
  // NOP when InPlace
  //
  if(gfInPlace)
      return;

   if(bEnable) {
       EnableMenuItems(ghMenuFrame, EnPaMeIt1,
                   (UINT)(MF_BYCOMMAND | (bEnable ? MF_ENABLED : MF_GRAYED)));
       EnableMenuItems(ghMenuFrame, EnPaMeIt2,
                   (UINT)(MF_BYPOSITION | (bEnable ? MF_ENABLED : MF_GRAYED)));
       DrawFrameMenuBar();
   } else {
      PostMessage(hWnd, WM_CLOSE, 0, 0L);
   }
}

/* this is necessary whenever the paint window size changes */
/* such as loading a small picture */
void ResetPaintWindow(void)
{
   int x, y;

   /* in case it's smaller or something like that */

   EnablePaintWindow(TRUE);

   CalcWnds(NOCHANGEWINDOW, NOCHANGEWINDOW, NOCHANGEWINDOW, NOCHANGEWINDOW);
   x = pbrushRct[PAINTid].right - pbrushRct[PAINTid].left;
   y = pbrushRct[PAINTid].bottom - pbrushRct[PAINTid].top;
   MoveWindow(pbrushWnd[PAINTid],
         pbrushRct[PAINTid].left, pbrushRct[PAINTid].top,
         x, y, TRUE);
   SendMessage(pbrushWnd[PAINTid], WM_SIZE, 0, MAKELONG(paintWid, paintHgt));
   SetScrollPos(pbrushWnd[PAINTid], SB_HORZ, imageView.left, TRUE);
   SetScrollPos(pbrushWnd[PAINTid], SB_VERT, imageView.top, TRUE);
   InvalidateRect(pbrushWnd[PAINTid], NULL, TRUE);
   UpdateWindow(pbrushWnd[PAINTid]);
}

static WORD PRIVATE RealFileType(TCHAR *Filename)
{
   HANDLE fh;
   BYTE   buf[4];

   if((fh = MyOpenFile(Filename, NULL, OF_READ | OF_SHARE_DENY_WRITE)) == INVALID_HANDLE_VALUE)
   {
      SimpleMessage(IDSNotExist, Filename, MB_OK | MB_ICONEXCLAMATION);
      return UNKNOWN_TYPE;
   }

   MyByteReadFile(fh, buf, 4);
   MyCloseFile(fh);

   if(*buf == 0x42 && buf[1] == 0x4D)  /* 'B' and 'M' */
      return BITMAPFILE;
   else if (buf[0] == 10)
      return PCXFILE;
   else if ((*((WORD *) buf) == 0x694c && *((WORD *) (buf+2)) == 0x536e) ||
            (*((WORD *) buf) == 0x6144 && *((WORD *) (buf+2)) == 0x4d6e))
      return MSPFILE;
   else if (buf[0] == 2)
      SimpleMessage(IDS3PlaneFile, Filename, MB_OK | MB_ICONEXCLAMATION);
   else
      SimpleMessage(IDSBadHeader, Filename, MB_OK | MB_ICONEXCLAMATION);

   return UNKNOWN_TYPE;
}

static void PRIVATE GetWildcard(LPTSTR lpFilename, LPTSTR lpWildcard)
{
   LPTSTR lpStr;

   /* start with a star ... */
   *lpWildcard++ = TEXT('*');

   /* scan backwards until . or beginning of string */
   for(lpStr = lpFilename + lstrlen(lpFilename);
         lpStr > lpFilename && *lpStr != TEXT('.');
         lpStr = CharPrev(lpFilename, lpStr))
      ;

   /* if there is no extension use wFiletype to determine extension */
   if(*lpStr != TEXT('.'))
      lstrcpy(lpWildcard, (wFileType == PCXFILE) ? TEXT(".PCX") : TEXT(".BMP"));
   else
      lstrcpy(lpWildcard, lpStr);
}

static int PRIVATE MCDoDialog(int id, HWND hwnd, WNDPROC theProc)
{
   REGISTER int result;

   gfInDialog = TRUE;
   Terminatewnd();
   UpdatImg();
   if((result = DoDialog(id, hwnd, theProc)) == -1)
      SimpleMessage(IDSNotEnufMem, NULL, MB_OK  | MB_ICONHAND);

   gfInDialog = FALSE;
   return(result);
}

static BOOL PRIVATE MCDoFileDialog(int id, HWND hwnd) {
    Terminatewnd();
    UpdatImg();
    return DoFileDialog(id, hwnd);
}

/* Changed from:
static void PRIVATE NewImage(int result)
so that we can call this from pbserver.c
*/
void PUBLIC NewImage(WORD result)
{
   TCHAR str[2];
   LPTSTR lpFilePathName;
   TCHAR sz[FILENAMElen + PATHlen];


   /* For a more comprehensive file load message. */
   lstrcpy(sz, filePath);

   /* concatenate a backslash on the path only if this isn't the root */
   if ((*sz) && (sz[lstrlen(sz) - 1] != TEXT('\\')))
       lstrcat(sz, TEXT("\\"));

   lstrcat(sz, fileName);
   lpFilePathName = sz;

   if (result || (imagePlanes == 1 && imagePixels == 1)) {
      dpArray[COLORERASERtool] = EraserDP;
      rgbColor = bwColor;
   } else {
      dpArray[COLORERASERtool] = ColEraDP;
      rgbColor = colorColor;
   }

   InvalidateRect (pbrushWnd[COLORid], NULL, FALSE);

   if (!result)
   {
      bFileLoaded = TRUE;       /* For command line print to determine if
                                 * a file has been successfully loaded.
                                 */
      SetTitle(fileName[0] ? fileName : noFile);
      ResetPaintWindow(); /* see below */
      AdviseDataChange();
   }
   else
   {
      /* If it failed, create a monochrome by default */
      if (!AllocImg(defaultWid, defaultHgt, 1, 1, TRUE))
      {
         ResetPaintWindow();

         if (result == IDSNotDiskAvail)
         {
            /* Raid #2275 */
            TCHAR pszString[144];

            GetTempFileName (0, TEXT(""), 0, pszString);
            pszString[1] = (TCHAR) 0;

            /* 11-Jan-1993 - Jonma - OemToAnsi is obsolete and not
                needed here as per bug # */
#ifdef CONVOEM
            OemToAnsi (pszString, str);
#else
            lstrcpy (str, pszString);
#endif
            SimpleMessage (result, str, (WORD)(MB_OK | MB_ICONEXCLAMATION));
         }
         else
         {
            SimpleMessage (result, lpFilePathName, (WORD)(MB_OK | MB_ICONEXCLAMATION));
         }
      }
      else
      {
         SimpleMessage (IDSNotEnufMem, lpFilePathName, (WORD)(MB_OK | MB_ICONHAND));
         EnablePaintWindow (FALSE);
         return;
      }

      wFileType = BITMAPFILE;
      fileName[0] = TEXT('\0');
      SetTitle (noFile);
   }

   InitShapeLibrary();
   gfDirty = FALSE;  /* does not need saving anymore */

   PostMessage(pbrushWnd[TOOLid], WM_SELECTTOOL, 0, 0L);
}

TCHAR tempPath[PATHlen];


void PUBLIC MenuCmd(HWND hWnd, UINT item)
{
   int i, result = 0;
   int wftOld, wftTemp;
   HCURSOR oldcsr;
   RECT rWind;
   HPALETTE hOldPalette;
   static BOOL inZoom = FALSE;
   LPTSTR lpFileUNCName;
   TCHAR sz[FILENAMElen + PATHlen];
#ifdef JAPAN                  // added by Hiraisi  02 Sep. 1992
   static int tempType;
#endif



   SendMessage(pbrushWnd[PAINTid], WM_HIDECURSOR, 0, 0L);
   SetCursorOn();   /* not any more */
   oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

   switch(item) {
   case FILEload:      /* just supply a name */

      /* Register the document */
      { LPTSTR psz;
        TCHAR szPath[MAX_PATH];
        int i;

         DOUTR(L"BUGBUG - TEST THIS NEW CODE WITH OLE2" );
#pragma message("menucmd.c(370): warning !!!!! : Test this code with OLE2!")

         /* Register the document */
         i = lstrlen(filePath);

         CopyMemory(szPath, filePath, ByteCountOf(i));

         if (i > 0 && !IsPathSep(szPath[i-1])) {
            szPath[i++] = TEXT('\\');
         }

         lstrcpy( &szPath[i], fileName );

         GetFullPathName( szPath, CharSizeOf(sz), sz, &psz);
      }
      lpFileUNCName = sz;

      AdviseRename(lpFileUNCName);

      gfLoading = TRUE;
      Terminatewnd();
      if(filePath[0])
         changeDiskDir(filePath);

      hOldPalette = hPalette;
      hPalette = NULL;  /* no more palette */

      imageView.top = imageView.left = 0;  /* reset view */

      DlgCaptionNo = -1;
      switch(RealFileType(fileName)) {
      case BITMAPFILE:
         result = LoadBitmapFile(hWnd, lpFileUNCName, NULL);
         break;

      case PCXFILE:
         if(!(result = LoadImg(hWnd, lpFileUNCName)))
            wFileType = PCXFILE;
         break;

      case MSPFILE:
         if(SimpleMessage(IDSXlat, lpFileUNCName, MB_OKCANCEL | MB_ICONEXCLAMATION)
                 == IDCANCEL)
            goto Error1;
         if(!(result = LoadMSPImg(hWnd)))
            wFileType = BITMAPFILE;
         break;

      default:
         goto Error1;
         break;
      }

      if (hOldPalette)
          DeleteObject(hOldPalette);

      NewImage((WORD)result);
      gfLoading = FALSE;
      break;

Error1:
      hPalette = hOldPalette;
      gfLoading = FALSE;
         lstrcpy(fileName, tempName);
         lstrcpy(filePath, tempPath);
#ifdef JAPAN                  // added by Hiraisi  02 Sep. 1992
         wFileType = tempType;
#endif

         /* Rename the document back */
         {
             TCHAR sz[FILENAMElen + PATHlen];
             lstrcpy(sz, tempPath);
             if (*sz)
                   lstrcat(sz, TEXT("\\"));
             lstrcat(sz, tempName);
             AdviseRename(sz);
         }
      if (gfInvisible)
          PostMessage(pbrushWnd[PARENTid], WM_SYSCOMMAND, SC_CLOSE, 0L);
      break;

   //
   // The following is the guts of file new.
   // We process this stuff here because we need to give OLE a chance to
   // get our clipboard formats rendered prior to blowing our current
   // document away...
   //
   case FILEasync:
    GetWildcard(fileName, wildCard);
    namePtr = fileName;
    pathPtr = filePath;

    Terminatewnd();
    UpdatImg();
    if(filePath[0])
    changeDiskDir(filePath);

    fileName[0] = TEXT('\0');
    if(hPalette)
    DeleteObject(hPalette);
    hPalette = NULL;     /* Palette goes bye bye */

    SetTitle(noFile);
    AdviseRename(fileName);

    gfLoading = TRUE;
    result = AllocImg(nNewImageWidth, nNewImageHeight,
                   nNewImagePlanes, nNewImagePixels, TRUE);
    wFileType = GetImageFileType(imagePixels * imagePlanes);
    gfLoading = FALSE;

    NewImage((WORD)result);

    //
    // Ensure a unique filename in gachLinkFilename so we can create valid
    // FileMonikers...
    //
    if(gachLinkFilename[0] == L'\0')
        BuildUniqueLinkName();
     break;

   case FILEnew:
   case FILEopen:
      if (!SaveAsNeeded())
          break;

      if(item == FILEnew)
      {
        //
        // force rendering of any currently posted stuff...
        //
        FlushOleClipboard();
        PostMessage(hWnd, WM_COMMAND, FILEasync, 0L);
      }
      else
      {
         fileMode = LOADfile;
         DlgCaptionNo = FILELOAD;
         lstrcpy(tempName, fileName);
         lstrcpy(tempPath, filePath);
#ifdef JAPAN                  // added by Hiraisi  02 Sep. 1992
         tempType = wFileType;
#endif

         wftOld = wFileType;
         result = DoFileDialog(LOADBOX, hWnd);
         wftTemp = wFileType;

         if(!pbrushWnd[PARENTid] || !result) {
            lstrcpy(fileName, tempName);
            lstrcpy(filePath, tempPath);
            break;
         }
         else
         {
            lstrcpy(fileName, namePtr);
            lstrcpy(filePath, pathPtr);
            lstrcpy(gachLinkFilename, pathPtr);
            lstrcat(gachLinkFilename, L"\\");
            lstrcat(gachLinkFilename, namePtr);
         }

         GetWildcard(fileName, wildCard);
         namePtr = fileName;
         pathPtr = filePath;

         Terminatewnd();
         UpdatImg();
         if(filePath[0])
            changeDiskDir(filePath);

         SendMessage(hWnd, WM_COMMAND, FILEload, 0L);
      }

#ifdef JAPAN //KKBUGFIX added by Hiraisi 11 Nov. 1992 (BUG#457/WIN31 in Japan)
       // The mouse window sometimes overlaps on the paint window.
      if( mouseFlag )
          SendMessage(mouseWnd, WM_MOUSEWINDOW, 0, 0L);
#endif
      break;

   case FILEsave:
      if (!gfStandalone && !gfLinked)     /* It's really update! */
      {
          DoOleSave();
          break;
      }
      else
      {
          int SaveIds[] = { IDS_PCXFILTER, IDS_MONOBMPFILTER, IDS_16COLORFILTER,
                            IDS_256COLORFILTER, IDS_24BITFILTER, 0 };
          WORD twFileType;
          HDC hDC;

          if (hDC = GetWindowDC(pbrushWnd[PAINTid])) {
              twFileType = GetImageFileType(GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES));
              ReleaseDC(pbrushWnd[PAINTid], hDC);
          } else
              twFileType = 1;

          if (twFileType < wFileType
              && BITMAPFILE <= twFileType
              && twFileType <= BITMAPFILE24)
          {
              TCHAR    szOldRes[FILTERMAX];
              TCHAR    szNewRes[FILTERMAX];

              LoadString(hInst, (WORD)SaveIds[twFileType], szOldRes, CharSizeOf(szOldRes));
              LoadString(hInst, (WORD)SaveIds[wFileType], szNewRes, CharSizeOf(szNewRes));
              LoadString(hInst, W_SAVING_LOWER_RESOLUTION1, szTemplate, WARNMSGMAX);
              LoadString(hInst, W_SAVING_LOWER_RESOLUTION2, szTemplate + lstrlen(szTemplate), WARNMSGMAX);
              LoadString(hInst, W_SAVING_LOWER_RESOLUTION3, szTemplate + lstrlen(szTemplate), WARNMSGMAX);
              wsprintf(szMessage, szTemplate, szNewRes, szOldRes);
              switch (MessageBox(GetActiveWindow(), szMessage, pgmTitle, MB_OKCANCEL | MB_ICONHAND))
              {
                  case IDCANCEL:
                      SetCursor(oldcsr);
                      return;
                  case IDOK:
                      /* Default to saving the lower resolution */
                      wFileType = twFileType;
                      break;
              }
           }
        }
      changeDiskDir(filePath);
      //
      // Fall through
      //

   case FILEsaveas:
      if (!gfStandalone && !gfLinked)
      {
          if (!SaveAsNeeded())
            break;
      }

      IsCanceled = FALSE;

      Terminatewnd();
      UpdatImg();

      if (!(*fileName))
          item = FILEsaveas;

      if(item == FILEsaveas) {
         GetWildcard(namePtr, wildCard);
         lstrcpy(tempName, fileName);
         namePtr = tempName;
         pathPtr = filePath;
         fileMode = SAVEfile;
         DlgCaptionNo = FILESAVE;

         result = DoFileDialog(SAVEBOX, hWnd);
         /* if a client closes us in the midst of the SaveAs dlg, abort */
         if(!pbrushWnd[PARENTid] || !result) {
            IsCanceled = TRUE;
            break;
         }

         lstrcpy(fileName, tempName);
         SetTitle(fileName);
      }

      if(wFileType == BITMAPFILE ||
            wFileType == BITMAPFILE4 ||
            wFileType == BITMAPFILE8 ||
            wFileType == BITMAPFILE24 ||
            wFileType == MSPFILE)
         IsCanceled = !SaveBitmapFile(hWnd, 0, 0, imageWid, imageHgt, NULL);
      else
         IsCanceled = !SaveImg(hWnd, 0, 0, imageWid, imageHgt,
               imageByteWid, NULL);

      if (!IsCanceled) {
          if (item == FILEsaveas) {
              TCHAR sz[FILENAMElen + PATHlen];

              lstrcpy(sz, filePath);
              if (*sz)
                  lstrcat(sz, TEXT("\\"));
              lstrcat(sz, fileName);

              AdviseRename(sz);
          }
      }
      break;

   case FILEupdate:

      if (theTool == PICKtool && TerminateKill)
          SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0);

      DoOleSave();
      break;

   case FILEprinter:
        // Commmon printer setup dialog added 02/28/91
        PD.Flags = PD_PRINTSETUP;
        if(PrintDlg((LPPRINTDLG)&PD))
          bIsPrinterDefault = FALSE;
        if(CommDlgExtendedError()){ /* If error re-initialize PD. */
                FreePrintHandles();
                PD.lStructSize  = sizeof(PRINTDLG);
                PD.hwndOwner    = hWnd;
                PD.nCopies      = 1;
        }

      break;

   case FILEpage:
      MCDoDialog(PAGESETUP, hWnd, (WNDPROC)PageSetDlg);
      break;

   case FILEprint:
   {
        BOOL fGraphicsPrinter = FALSE;
        int  iResult = 0;

        if (printDC = GetPrtDC())
        {
            fGraphicsPrinter = (GetDeviceCaps(printDC, RASTERCAPS) & RC_BITBLT);
            if (!fGraphicsPrinter)
            {
                DeleteDC(printDC);
                printDC = NULL;
                SimpleMessage(IDSCANTPRINTGRAPHICS, NULL, MB_OK | MB_ICONEXCLAMATION);
            }
            else
            {
                iResult = MCDoDialog(PRINTBOX, hWnd, (WNDPROC)PrintFileDlg);
                /* If the dialog is cancelled, then delete the DC.
                 */
                if (iResult == 0)
                  {
                    DeleteDC(printDC);
                    printDC = NULL;
                  }
            }
        }
    }
        break;

   case FILEshow:
      Terminatewnd();
      UpdatImg();
      fullWnd = CreateWindow(TEXT("pbFull"), NULL, WS_POPUP,
                  0, 0,
                  GetSystemMetrics(SM_CXSCREEN),
                  GetSystemMetrics(SM_CYSCREEN),
                  pbrushWnd[PARENTid], NULL, hInst, NULL);
      if(!fullWnd)
         break;

      ShowWindow(fullWnd, SHOW_OPENWINDOW);
      UpdateWindow(fullWnd);
      SetFocus(fullWnd);
      break;

   case FILEclear:
      MCDoDialog(CLEARBOX, hWnd, (WNDPROC)ClearDlg);
      break;

   case FILEexit:
      PostMessage(pbrushWnd[PARENTid], WM_SYSCOMMAND, SC_CLOSE, 0L);
      break;

   case EDITundo:
      if(bZoomedOut) {
         SendMessage(zoomOutWnd, WM_COMMAND, EDITundo, 0l);
      } else if(inMagnify) {
         SendMessage(pbrushWnd[PAINTid], WM_ZOOMUNDO, 0, 0L);
         Terminatewnd();
         /* set this if we closed the box during zoomin mode  */
         if(!mouseFlag)
            CheckMenuItem(ghMenuFrame, MISCmousePos, MF_UNCHECKED);
      } else {
         Terminatewnd();
         UndoImg();
      }
      break;

   case EDITpaste:
       {
        CLIPFORMAT cf;
        if(OleClipboardContainsAcceptableFormats(&cf))
        {
            HGLOBAL hGlobal;
            if(GetTypedHGlobalFromOleClipboard(cf, &hGlobal))
            {
                PasteTypedHGlobal(cf, hGlobal);
                //FreeGlobalToPaste();
                //SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);
            }
        }
        else
        {
 DOUT(L"###PBrush MenuCmd No Acceptable Formats on OLE clipboard!\r\n");
        }
        break;
       }

   case EDITcutpict:
        gfWholeHog = FALSE;
        SendMessage(bZoomedOut ? zoomOutWnd : pbrushWnd[PAINTid], WM_CUT, 0, 0L);
        break;

   case EDITcopydoc:
        DOUT(L"\nmenucmd: EDITcopydoc\n" );
        SendMessage(pbrushWnd[PAINTid], WM_TERMINATE, 0, 0L);
        if(theTool != PICKtool) {
            ToolWP(pbrushWnd[TOOLid], WM_SELECTTOOL, PICKtool, 0L);
        }
        gfWholeHog = TRUE;
        //SavePickState();
        SelectWholePicture();
        SendMessage(bZoomedOut ? zoomOutWnd : pbrushWnd[PAINTid], WM_COPY, 0, 0L);
        //RestorePickState();
        break;

   case EDITcopypict:
        //
        // If the current pick rect is empty, NOP
        //
        if (!bZoomedOut && IsRectEmpty(&pickRect))
                break;

        gfWholeHog = FALSE;
        SendMessage(bZoomedOut ? zoomOutWnd : pbrushWnd[PAINTid], WM_COPY, 0, 0L);
        break;

   case EDITcopyTo:
      GetWildcard(namePtr, wildCard);
      clipName[0] = TEXT('\0');
      namePtr = clipName;
      pathPtr = clipPath;
      fileMode = SAVEfile;
      DlgCaptionNo = COPYTO;
      if(clipPath[0])
         changeDiskDir(clipPath);

      i = wFileType;

      SendMessage(bZoomedOut ? zoomOutWnd : pbrushWnd[PAINTid],
            WM_COPYTO, 0, 0L);

      wFileType = (WORD)i;
      namePtr = fileName;
      pathPtr = filePath;
      break;

   case EDITpasteFrom:
      GetWildcard(namePtr, wildCard);
      namePtr = clipName;
      pathPtr = clipPath;
      fileMode = LOADfile;
      DlgCaptionNo = PASTEFROM;
      if(clipPath[0])
         changeDiskDir(clipPath);

      i = wFileType;
      if(wFileType == MSPFILE)
         wFileType = BITMAPFILE;

      SendMessage(pbrushWnd[TOOLid], WM_SELECTTOOL, PICKtool, 0L);

      if (!(result = MCDoFileDialog(LOADBOX, hWnd)))
         goto EPFErr;

      switch(RealFileType(namePtr)) {
      case BITMAPFILE:
         result = LoadBitmapFile(hWnd, namePtr, NULL);
         break;

      case PCXFILE:
         result = LoadImg(hWnd, namePtr);
         break;

      case MSPFILE:
         result = IDSNoMSPPaste;
         break;

      default: /* invalid format (user already notified) */
         goto EPFErr;
         break;
      }

      if(result)
         SimpleMessage((WORD)result, namePtr, (WORD)(MB_OK | MB_ICONEXCLAMATION));
      else {
         UpdateWindow(hWnd);
         SendMessage(bZoomedOut ? zoomOutWnd : pbrushWnd[PAINTid],
               WM_PASTEFROM, 0, 0L);
         gfDirty = TRUE;
      }

EPFErr:
      wFileType = (WORD)i;
      break;

   case STYLEplain:
      CheckMenuItem(ghMenuFrame, STYLEplain, MF_CHECKED);
#ifdef JAPAN        // added by Hiraisi  08 Apr. 1992 : jinwoo 11/9/92
      if( bWritable ){
         for (i = STYLEbold; i <= STYLEvertical; ++i)
            CheckMenuItem(ghMenuFrame, i, MF_UNCHECKED);
         if( bVertical ){
            bVertical = FALSE;
            theFont = horiFont;
            if( theTool == TEXTtool )
                DrawCursor = szPbCursor(TEXTtool);
         }
      }
      else{
         for (i = STYLEbold; i <= STYLEshadow; ++i)
            CheckMenuItem(ghMenuFrame, (WORD)i, MF_UNCHECKED);
      }
      vertFont.lfWeight = nonBoldWeight;
      vertFont.lfItalic = vertFont.lfUnderline = FALSE;
      horiFont.lfWeight = nonBoldWeight;
      horiFont.lfItalic = horiFont.lfUnderline = FALSE;
      theFont.lfWeight = nonBoldWeight;
#else
      for (i = STYLEbold; i <= STYLEshadow; ++i)
         CheckMenuItem(ghMenuFrame, (WORD)i, MF_UNCHECKED);
      theFont.lfWeight = FW_MEDIUM;
#endif
      theFont.lfItalic = theFont.lfUnderline = FALSE;
      outline = shadow = FALSE;
      SendMessage(pbrushWnd[PAINTid], WM_CHANGEFONT, 0, 0L);
      break;

   case STYLEbold:
      if(theFont.lfWeight != FW_BOLD) {
         theFont.lfWeight = FW_BOLD;
         CheckMenuItem(ghMenuFrame, STYLEbold, MF_CHECKED);
      } else {
#ifdef  JAPAN            // added by Hiraisi  06 Jul. 1992 : jinwoo 11/9/92
         theFont.lfWeight = nonBoldWeight;
#else
         theFont.lfWeight = FW_MEDIUM;
#endif
         CheckMenuItem(ghMenuFrame, STYLEbold, MF_UNCHECKED);
      }
      goto AllStyles;

   case STYLEitalic:
      if(!theFont.lfItalic) {
         theFont.lfItalic = TRUE;
         CheckMenuItem(ghMenuFrame, STYLEitalic, MF_CHECKED);
      } else {
         theFont.lfItalic = FALSE;
         CheckMenuItem(ghMenuFrame, STYLEitalic, MF_UNCHECKED);
      }
      goto AllStyles;

   case STYLEunderline:
      if(!theFont.lfUnderline) {
         theFont.lfUnderline = TRUE;
         CheckMenuItem(ghMenuFrame, STYLEunderline, MF_CHECKED);
      } else {
         theFont.lfUnderline = FALSE;
         CheckMenuItem(ghMenuFrame, STYLEunderline, MF_UNCHECKED);
      }
      goto AllStyles;

   case STYLEoutline:
      if(!outline) {
         outline = TRUE;
         shadow = FALSE;
         CheckMenuItem(ghMenuFrame, STYLEoutline, MF_CHECKED);
         CheckMenuItem(ghMenuFrame, STYLEshadow, MF_UNCHECKED);
      } else {
         outline = FALSE;
         CheckMenuItem(ghMenuFrame, STYLEoutline, MF_UNCHECKED);
      }
      goto AllStyles;

   case STYLEshadow:
      if(!shadow) {
         shadow = TRUE;
         outline = FALSE;
         CheckMenuItem(ghMenuFrame, STYLEshadow, MF_CHECKED);
         CheckMenuItem(ghMenuFrame, STYLEoutline, MF_UNCHECKED);
      } else {
         shadow = FALSE;
         CheckMenuItem(ghMenuFrame, STYLEshadow, MF_UNCHECKED);
      }
      goto AllStyles;

#ifdef JAPAN   // added by Hiraisi  07 Apr. 1992 : jinwoo 11/9/92
   case STYLEvertical:      /* Vertical-Writing option */
      if( !bVertical ) {
         bVertical = TRUE;
         theFont = vertFont;
         CheckMenuItem(ghMenuFrame, STYLEvertical, MF_CHECKED);
      } else {
         bVertical = FALSE;
         theFont = horiFont;
         CheckMenuItem(ghMenuFrame, STYLEvertical, MF_UNCHECKED);
      }
      if( theTool == TEXTtool )
          DrawCursor = szPbCursor(TEXTtool);
      goto AllStyles;
#endif

AllStyles:
#ifdef JAPAN         // added by Hiraisi  06 Jul. 1992 : jinwoo 11/9/92
      if( (theFont.lfWeight==FW_MEDIUM   ||
          theFont.lfWeight==nonBoldWeight)
            && !theFont.lfItalic
            && !theFont.lfUnderline
            && !outline && !shadow && !bVertical)
#else
      if(theFont.lfWeight==FW_MEDIUM
            && !theFont.lfItalic
            && !theFont.lfUnderline
            && !outline && !shadow)
#endif
         CheckMenuItem(ghMenuFrame, STYLEplain, MF_CHECKED);
      else
         CheckMenuItem(ghMenuFrame, STYLEplain, MF_UNCHECKED);

#ifdef JAPAN         // added by Hiraisi  17 Apr. 1992 : jinwoo 11/9/92
      vertFont.lfWeight = theFont.lfWeight;
      vertFont.lfItalic = theFont.lfItalic;
      vertFont.lfUnderline = theFont.lfUnderline;
      horiFont.lfWeight = theFont.lfWeight;
      horiFont.lfItalic = theFont.lfItalic;
      horiFont.lfUnderline = theFont.lfUnderline;
#endif

      SendMessage(pbrushWnd[PAINTid], WM_CHANGEFONT, 0, 0L);
      break;

   case FONTS:
   {
      LOGFONT LogFont;
      HDC hDC;
      CHOOSEFONT cfStruct;
      int Result;
#ifdef JAPAN        // added by Hiraisi  20 May. 1992 : jinwoo 11/9/92
      FARPROC lpfnDeleteFacename;
#endif

      TerminateKill = FALSE;
      hDC = GetDC(hWnd);
      LogFont = theFont;
      cfStruct.lStructSize = sizeof(CHOOSEFONT);
      cfStruct.hwndOwner = hWnd;
      cfStruct.hDC = hDC;
      cfStruct.lpLogFont = &LogFont;
      cfStruct.Flags = CF_SCREENFONTS | CF_EFFECTS |
#ifdef JAPAN        // added by Hiraisi  20 May. 1992 : jinwoo 11/9/92
                       CF_ENABLEHOOK |
#endif
                       CF_INITTOLOGFONTSTRUCT | CF_ENABLETEMPLATE;
      cfStruct.rgbColors = rgbColor[theForeg];
      cfStruct.lCustData = 0L;
#ifdef JAPAN         // added by Hiraisi  20 May. 1992 : jinwoo 11/9/92
      lpfnDeleteFacename = MakeProcInstance( (FARPROC)DeleteFacename, hInst );
      cfStruct.lpfnHook = (FARPROC)lpfnDeleteFacename;
#else
      cfStruct.lpfnHook = (LPCFHOOKPROC)NULL;
#endif
      cfStruct.lpTemplateName = (LPTSTR) MAKEINTRESOURCE(DLG_FONT);
      cfStruct.hInstance = hInst;
      cfStruct.lpszStyle = NULL;
      cfStruct.nFontType = SCREEN_FONTTYPE;
      cfStruct.nSizeMin = 0;
      cfStruct.nSizeMax = 0;

#ifdef JAPAN          // added by Hiraisi  20 Apr. 1992 : jinwoo 11/9/92
      if( bVertical )
         lstrcpy( LogFont.lfFaceName, horiFont.lfFaceName );
#endif

      Result = ChooseFont(&cfStruct);

#ifdef JAPAN           // added by Hiraisi  20 May. 1992 : jinwoo 11/9/92
      FreeProcInstance( lpfnDeleteFacename );
#endif

      ReleaseDC(hWnd, hDC);
      TerminateKill = TRUE;
      if (Result)
      {
         theFont = LogFont;
         for (i = STYLEplain; i <= STYLEunderline; ++i)  /* FGS fix 1/21/92 */
             CheckMenuItem(ghMenuFrame, i, MF_UNCHECKED);

         if (theFont.lfWeight == FW_BOLD)
             CheckMenuItem(ghMenuFrame, STYLEbold, MF_CHECKED);
         if (theFont.lfItalic)
             CheckMenuItem(ghMenuFrame, STYLEitalic, MF_CHECKED);
         if (theFont.lfUnderline)
             CheckMenuItem(ghMenuFrame, STYLEunderline, MF_CHECKED);
#ifdef JAPAN         // added by Hiraisi  06 Jul. 1992 : jinwoo 11/9/92
         if( (theFont.lfWeight==FW_MEDIUM    ||
              theFont.lfWeight==nonBoldWeight)
             && !theFont.lfItalic
             && !theFont.lfUnderline
             && !outline && !shadow && !bVertical )
#else
         if (theFont.lfWeight==FW_MEDIUM
             && !theFont.lfItalic
             && !theFont.lfUnderline
             && !outline && !shadow)
#endif
             CheckMenuItem(ghMenuFrame, STYLEplain, MF_CHECKED);

#ifdef JAPAN         // added by Hiraisi  07 Apr. 1992 : jinwoo 11/9/92
         hDC = GetDC(hWnd);
         CheckVerticalWriting( hDC, LogFont.lfFaceName );
         if( bVertical )
             theFont = vertFont;
         ReleaseDC(hWnd, hDC);

         if( theFont.lfWeight != FW_BOLD )        //  added  06 Jul. 1992
             nonBoldWeight = theFont.lfWeight;

         //KKBUGFIX     // added by Hiraisi  15 May. 1992
         /*
            The reason is as follows.
               A part of bk color of texts becomes color of the parent window
              when font size is changed larger than before,
              if color of the parent window is not white.
               This occurs when CHOOSEFONT dialog box was moved.
         */
         UpdateWindow(pbrushWnd[PAINTid]);
#endif

         SendMessage(pbrushWnd[PAINTid], WM_CHANGEFONT, 0, 0);
      }
#ifdef JAPAN //KKBUGFIX     // added by Hiraisi  04 Sep. 1992 (in Japan)
         /*
            The reason is as follows.
               We need to create our own caret again because it had already
              been destroyed.
               And no caret appears though we can input chars,
              if we don't do this.
         */
      else
         SendMessage(pbrushWnd[PAINTid], WM_RESETCARET, 0, 0);
#endif
   }
      break;

#if defined (JAPAN) & defined (IME_HIDDEN) // added by Hiraisi
   case EDITimeHidden:
   {
      extern BOOL bIMEhidden, bHidden;
      extern TCHAR winIniImeHidden[];
      TCHAR szBuf[2];

      Terminatewnd();
      bIMEhidden = !bIMEhidden;
      if( bIMEhidden && bHidden )
         CheckMenuItem(ghMenuFrame, EDITimeHidden, MF_CHECKED);
      else
         CheckMenuItem(ghMenuFrame, EDITimeHidden, MF_UNCHECKED);

      wsprintf(szBuf, TEXT("%d"), bIMEhidden);
      WriteProfileString(winIniAppName, winIniImeHidden, szBuf);
      break;
   }
#endif

   case PICKflipH:
   case PICKflipV:
   case PICKinverse:
   case PICKsg:
   case PICKtilt:
      SendMessage(pbrushWnd[PAINTid], (WORD)(WM_PICKFLIPH + item - PICKflipH), 0, 0L);
      break;

   case PICKclear:
      CheckMenuItem(ghMenuFrame, PICKclear,
            (WORD)((clearFlag ^= TRUE) ? MF_CHECKED : MF_UNCHECKED));
      break;

   case MISCzoomIn:
      //
      // NOP when InPlace
      //
      if(gfInPlace)
          break;

      if(bZoomedOut) {
         PostMessage(zoomOutWnd, WM_KEYDOWN, VK_ESCAPE, 0l);
      } else {
         zoomAmount = 8;
         CalcView();
         Terminatewnd();
         UpdatImg();
         inZoom = TRUE;

         EnableMenuItem(ghMenuFrame, ZOOMundo, MF_ENABLED | MF_BYCOMMAND);
         EnableMenuItems(ghMenuFrame, MiZoInEn1, MF_GRAYED | MF_BYCOMMAND);

         EnableMenuItems(ghMenuFrame, MiZoInEn2, MF_GRAYED | MF_BYPOSITION);
         DrawFrameMenuBar();

         if(theTool != ROLLERtool)
            SendMessage(pbrushWnd[TOOLid], WM_SELECTTOOL, BRUSHtool, 0L);
         EnableWindow(pbrushWnd[TOOLid], FALSE);

         DrawProc = ZoomInDP;
         cursTool = ZOOMINtool;
         DrawCursor = NULL;
         PbSetCursor(DrawCursor = NULL);
      }
      break;

   case MISCzoomOut:
      //
      // NOP when InPlace
      //
      if(gfInPlace)
          break;

      if(inMagnify || inZoom) {
         SendMessage(hWnd, WM_COMMAND, ZOOMaccept, 0L);
         break;
      }
      Terminatewnd();
      UpdatImg();

      zoomOutWnd = CreateWindow(TEXT("pbZoomOut"), NULL,
                  WS_CHILD | WS_BORDER | WS_OVERLAPPED,
                  pbrushRct[PAINTid].left, pbrushRct[PAINTid].top,
                  pbrushRct[PAINTid].right - pbrushRct[PAINTid].left,
                  pbrushRct[PAINTid].bottom - pbrushRct[PAINTid].top,
                  pbrushWnd[PARENTid], NULL, hInst, NULL);
      if(!zoomOutWnd)
         break;

      ShowWindow(pbrushWnd[PAINTid], SW_HIDE);
      ShowWindow(zoomOutWnd, SW_SHOWNA);
      UpdateWindow(zoomOutWnd);
      SetFocus(zoomOutWnd);

      bZoomedOut = TRUE;

      EnableMenuItem(ghMenuFrame, ZOOMundo, MF_ENABLED | MF_BYCOMMAND);
      EnableMenuItems(ghMenuFrame, MiZoOtEn1, MF_GRAYED | MF_BYCOMMAND);

      EnableMenuItems(ghMenuFrame, MiZoInEn2, MF_GRAYED | MF_BYPOSITION);
      DrawFrameMenuBar();
      break;

   case ZOOMaccept:
      //
      // NOP when InPlace
      //
      if(gfInPlace)
          break;

      if(inMagnify || inZoom)
         SendMessage(pbrushWnd[PAINTid], WM_ZOOMACCEPT, 0, 0L);
      else if(bZoomedOut) {
                /* accept the paste*/
         SendMessage(zoomOutWnd, WM_COMMAND, ZOOMaccept, 0l);
                /* clean up any DC's or other junk */
         SendMessage(zoomOutWnd, WM_COMMAND, EDITundo, 0l);
      }
      Terminatewnd();
      inMagnify = inZoom = FALSE;
      /* set this if we closed the box during zoomin mode  */
      if(!mouseFlag)
         CheckMenuItem(ghMenuFrame, MISCmousePos, MF_UNCHECKED);
      break;

   case MISCbrush:
      MCDoDialog(BRUSHBOX, hWnd, (WNDPROC)BrushDlg);
      break;

   case MISCeditColor:
      Terminatewnd();
      UpdatImg();
      if(hDlgModeless = colorWnd =
            CreateDialog(hInst, (LPTSTR) MAKEINTRESOURCE(COLORBOX),
            pbrushWnd[PARENTid], (WNDPROC)lpColorDlg)) {
         EnableWindow(pbrushWnd[PAINTid], FALSE);
         EnableWindow(pbrushWnd[TOOLid], FALSE);
         EnableWindow(pbrushWnd[SIZEid], FALSE);

         EnableMenuItems(ghMenuFrame, MiEdCoEn, MF_GRAYED | MF_BYPOSITION);
         DrawFrameMenuBar();
      } else {
         SimpleMessage(IDSNotEnufMem, NULL, MB_OK  | MB_ICONHAND);
      }
      break;

   case MISCgetColor:
      lstrcpy(wildCard, TEXT("*.PAL"));
      namePtr = tempName;
      pathPtr = colorPath;
      fileMode = LOADfile;
      DlgCaptionNo = COLORLOAD;

      if (!(result = MCDoFileDialog(CLRLDBOX, hWnd)))
         break;

      LoadColr(hWnd, tempName);
      break;

   case MISCsaveColor:
      lstrcpy(wildCard, TEXT("*.PAL"));
      tempName[0] = TEXT('\0');
      namePtr = tempName;
      pathPtr = colorPath;
      fileMode = SAVEfile;
      DlgCaptionNo = COLORSAVE;

      if (!(result = MCDoFileDialog(CLRSVBOX, hWnd)))
         break;

      SaveColr(hWnd, tempName);
      break;

   case MISCmousePos:
      if(mouseFlag) {
         if(DestroyWindow(mouseWnd)) {
            CheckMenuItem(ghMenuFrame, MISCmousePos, MF_UNCHECKED);
            mouseFlag = FALSE;
         }
      } else {
         if(mouseWnd = CreateDialog(hInst, (LPTSTR) MAKEINTRESOURCE(MOUSEBOX),
               pbrushWnd[PARENTid], (WNDPROC)lpMouseDlg)) {
            mouseFlag = TRUE;
            CheckMenuItem(ghMenuFrame, MISCmousePos, MF_CHECKED);
            SetActiveWindow(pbrushWnd[PARENTid]);
         } else {
            SimpleMessage(IDSNotEnufMem, NULL, MB_OK  | MB_ICONHAND);
         }
      }
      break;

   case WINDOWtool:
      /* Must update the image before showing the new window
       */
      Terminatewnd();
      UpdatImg();

      if(GetMenuState(ghMenuFrame, WINDOWtool, MF_BYCOMMAND) & MF_CHECKED) {
         CheckMenuItem(ghMenuFrame, WINDOWtool, MF_BYCOMMAND | MF_UNCHECKED);
         ShowWindow(pbrushWnd[SIZEid], SW_HIDE);
         ShowWindow(pbrushWnd[TOOLid], SW_HIDE);
      } else {
         CheckMenuItem(ghMenuFrame, WINDOWtool, MF_BYCOMMAND | MF_CHECKED);
         ShowWindow(pbrushWnd[TOOLid], SW_SHOWNA);
         ShowWindow(pbrushWnd[SIZEid], SW_SHOWNA);
      }
      goto AllWindows;

   case WINDOWpalette:
      /* Must update the image before showing the new window
       */
      Terminatewnd();
      UpdatImg();

      if(GetMenuState(ghMenuFrame, WINDOWpalette, MF_BYCOMMAND) & MF_CHECKED) {
         CheckMenuItem(ghMenuFrame, WINDOWpalette, MF_BYCOMMAND | MF_UNCHECKED);
         ShowWindow(pbrushWnd[COLORid], SW_HIDE);
      } else {
         CheckMenuItem(ghMenuFrame, WINDOWpalette, MF_BYCOMMAND | MF_CHECKED);
         ShowWindow(pbrushWnd[COLORid], SW_SHOWNA);
      }
      goto AllWindows;

AllWindows:
      CalcWnds(NOCHANGEWINDOW, NOCHANGEWINDOW, NOCHANGEWINDOW, NOCHANGEWINDOW);
      /* the SIZE message puts the windows in the right spot */
      GetClientRect(pbrushWnd[PARENTid], &rWind);
      SendMessage(pbrushWnd[PARENTid], WM_SIZE,
            SIZENORMAL, MAKELONG(rWind.right, rWind.bottom));
      InvalidateRect(pbrushWnd[PAINTid], NULL, FALSE);
      BringWindowToTop(pbrushWnd[PAINTid]);
      break;

   case MENU_INDEX:
      Help(hWnd, HELP_INDEX, 0L);
      break;

   case MENU_SEARCH:
      Help(hWnd, HELP_PARTIALKEY, (DWORD)TEXT(""));
      break;

   case MENU_USINGHELP:
      Help(hWnd, HELP_HELPONHELP, 0L);
      break;

   case MENUabout:
#ifdef WIN32
      if (ShellAbout(hWnd, pgmTitle, TEXT(""), LoadIcon(hInst, pgmName)) == -1)
        SimpleMessage(IDSNotEnufMem, NULL, MB_OK|MB_ICONHAND|MB_SYSTEMMODAL);
#endif
      break;

#ifdef DEBUG

   case DEBUG_GLOBALCOMPACT:
      GlobalCompact(-1L);
      break;

#endif

   default:
      break;
   }

   SetCursor(oldcsr);
}

WORD PUBLIC GetImageFileType(int Pixels) {
    if (!Pixels)
        Pixels = imagePixels * imagePlanes;

    /* Sweet success, set the default file type */
    if (Pixels > 8)        return BITMAPFILE24;
    else if (Pixels > 4)   return BITMAPFILE8;
    else if (Pixels > 1)   return BITMAPFILE4;
    else                   return BITMAPFILE;
}

BOOL GetPrintParms(
    HDC hDC)
{
   HDC hPrintDC, dc;
   extern int hSizePrt;
   extern int vSizePrt;
   extern int hResPrt;
   extern int vResPrt;
   extern int xPelsPrt;
   extern int yPelsPrt;

   /* check for printer driver */
   if (!hDC)
      hPrintDC = GetPrtDC();
   else
      hPrintDC = hDC;

   dc = (hPrintDC ? hPrintDC : GetDisplayDC(pbrushWnd[PARENTid]));

   if(!dc)
      return(FALSE);

   hSizePrt = GetDeviceCaps(dc, HORZSIZE);
   vSizePrt = GetDeviceCaps(dc, VERTSIZE);
   hResPrt = GetDeviceCaps(dc, HORZRES);
   vResPrt = GetDeviceCaps(dc, VERTRES);
   xPelsPrt = GetDeviceCaps(dc, LOGPIXELSX);
   yPelsPrt = GetDeviceCaps(dc, LOGPIXELSY);

   if (!hDC && hPrintDC) {
      DeleteDC(hPrintDC);
      return TRUE;
   } else if (!hPrintDC) {
      ReleaseDC(pbrushWnd[PARENTid], dc);
      return FALSE;
   }
}

BOOL SaveFileNameOK(
    TCHAR szPath[],
    TCHAR szFile[])
{
/* EDH 9 Oct 91
 * Check if ".\file" exists instead of "file". The Windows dir gets searched
 * if the dot isn't specified (if no dir is specified).
 */

    if (*szPath)
        changeDiskDir(szPath);
    if (IsReadOnly(szFile)) {
       SimpleMessage(IDSReadOnly, szFile, MB_OK | MB_ICONEXCLAMATION);
       return FALSE;
    }
    return TRUE;
}


BOOL FAR PASCAL ObjectUpdateDlgProc(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{

   switch (message) {
       case WM_COMMAND:
           switch (GET_WM_COMMAND_ID(wParam,lParam)) {
               case IDCANCEL:
               case IDD_CONTINUEEDIT:
                   EndDialog(hDlg, TRUE);
                   break;

               case IDD_UPDATEEXIT:
                   EndDialog(hDlg, FALSE);
                   break;

               default:
                   break;
           }
           break;

       case WM_INITDIALOG:
       {
           TCHAR szMsg[200];
           TCHAR szStr[100];

           LoadString(hInst, IDS_OBJECTUPDATE, szStr, CharSizeOf(szStr));
           wsprintf(szMsg, szStr, GetClientObjName(), pgmTitle);
           SetDlgItemText(hDlg, IDD_TEXT, szMsg);
           return TRUE; /* default Push button gets the focus */
       }
           break;
       default:
           break;
   }

   return FALSE;
}

#ifdef JAPAN          // added by Hiraisi  07 Apr. 1992 : jinwoo 11/9/92

/*
 *  This function is called when the selected font has facename(s) with @.
*/
int FAR PASCAL CheckEscapement( LPLOGFONT lf, LPTEXTMETRIC tm, short nType, LPTSTR lpData )
{
    /*
     *  We save LOGFONT structure for vertical writing,
     * if it is possible to write vertically.
    */
    if( lf->lfEscapement == 2700 ||
        (tm->tmPitchAndFamily & TMPF_TRUETYPE) == TMPF_TRUETYPE )
    {
        vertFont.lfHeight = theFont.lfHeight;
        vertFont.lfWidth = theFont.lfWidth;
        vertFont.lfEscapement = 2700;
        vertFont.lfOrientation = 2700;
        vertFont.lfWeight = theFont.lfWeight;
        vertFont.lfItalic = theFont.lfItalic;
        vertFont.lfUnderline = theFont.lfUnderline;
        vertFont.lfStrikeOut = theFont.lfStrikeOut;
        vertFont.lfCharSet = lf->lfCharSet;
        vertFont.lfOutPrecision = lf->lfOutPrecision;
        vertFont.lfClipPrecision = lf->lfClipPrecision;
        vertFont.lfQuality = lf->lfQuality;
        vertFont.lfPitchAndFamily = lf->lfPitchAndFamily;
        lstrcpy( vertFont.lfFaceName, lf->lfFaceName );
        return( FALSE );
    }

    return( TRUE );
}

void CheckVerticalWriting( HDC hDC, LPTSTR lpFaceName )
{
    FARPROC lpfnCheckEscapement;
    HMENU   hTextMenu;
    TCHAR   cFaceName[LF_FACESIZE+1] = TEXT("@");

    lstrcat( cFaceName, lpFaceName );
    lpfnCheckEscapement = MakeProcInstance( (FARPROC)CheckEscapement, hInst );
    if( !EnumFonts( hDC, cFaceName, lpfnCheckEscapement, 0 ) )
        bWritable = TRUE;
    else
        bWritable = FALSE;
    FreeProcInstance( lpfnCheckEscapement );

    horiFont = theFont;     // Save LOGFONT structure for horizontal writing.

    //
    // HACK WARNING: do the follwing constants actually match the
    //               current menu bar?
    //
    hTextMenu = GetSubMenu( ghMenuFrame, gfInPlace ? 2 : 3 );
    if( bWritable ){        /* Is it possible to write vertically ? */
        EnableMenuItem( hTextMenu, STYLEvertical, MF_BYCOMMAND | MF_ENABLED );
    }
    else{
        CheckMenuItem(ghMenuFrame, STYLEvertical, MF_UNCHECKED);
        EnableMenuItem( hTextMenu, STYLEvertical, MF_BYCOMMAND | MF_GRAYED );
        if( bVertical ){
            bVertical = FALSE;
            theFont = horiFont;
            if( theTool == TEXTtool )
                DrawCursor = szPbCursor(TEXTtool);
        }
    }
}

/*
 *  This function deletes facename(s) with @-prefix
 * from FONT combobox(cmb1) of the CHOOSEFONT dialog.       20 May. 1992
*/
int FAR PASCAL DeleteFacename( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    TCHAR str[50], sel[50];
    int ix;
    int cnt;

    if( uMsg != WM_INITDIALOG )
        return FALSE;

    cnt = (int)SendDlgItemMessage( hDlg, cmb1, CB_GETCOUNT, 0, 0L );
    ix = (int)SendDlgItemMessage( hDlg, cmb1, CB_GETCURSEL, 0, 0L );
    SendDlgItemMessage( hDlg, cmb1, CB_GETLBTEXT, ix, (DWORD)sel );
    for( ix = 0 ; ix < cnt ; ){
        SendDlgItemMessage( hDlg, cmb1, CB_GETLBTEXT, ix, (DWORD)str );
        if( str[0] == TEXT('@') )
            cnt = (int)SendDlgItemMessage( hDlg,cmb1,CB_DELETESTRING,ix,NULL );
        else
            ix++;
    }
    ix = (int)SendDlgItemMessage( hDlg, cmb1, CB_FINDSTRING, -1, (DWORD)sel );
    SendDlgItemMessage( hDlg, cmb1, CB_SETCURSEL, ix, 0L );

    return  TRUE;
}

#endif
