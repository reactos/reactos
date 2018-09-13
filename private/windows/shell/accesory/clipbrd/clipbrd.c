/****************************************************************************/
/*                                                                          */
/*  CLIPBRD.C -                                                             */
/*                                                                          */
/*  Copyright 1985-92, Microsoft Corporation                                */
/*                                                                          */
/*                                                                          */
/*      Window Clipboard Viewer                                             */
/*                                                                          */
/****************************************************************************/



/*
*
* Modified by Michael Gates (a-mgates), 9/9/92.
*
* Fixed bug #3576 ("Can't copy to clipboard from file manager")
*
* Touched by      :       Anas Jarrah
* On Date         :       May 11/1992.
* Revision remarks by Anas Jarrah ext #15201
* This file has been changed to comply with the Unicode standard
* Following is a quick overview of what I have done.
*
* Was             Changed it into        Remark
* ===             ===============        ======
* CHAR            TCHAR                  if it refers to a text elements
* LPCHAR & LPSTR  LPTSTR                 if it refers to text.
* LPCHAR & LPSTR  LPBYTE                 if it does not refer to text
* "..."           TEXT("...")            compile time macro resolves it.
* '...'           TEXT('...')            same
* strlen          CharStrLen             compile time macro resolves it.
* strcpy          CharStrCpy             compile time macro resolves it.
* strcmp          CharStrCmp             compile time macro resolves it.
* strcat          CharStrCat             compile time macro resolves it.
* RegisterWindow  RegisterWindowW        tell windows to send Unicode messages.
* LoadResource    LoadResource(A/W)      NT compiles resource strings into
*                                        Unicode binaries
* MOpenFile()     CreateFile             replaced to use the Win32 API [pierrej]
*
*
*
*   Modified by Pierre Jammes [pierrej] on 5/19/92
*
* The clipboard viewer must be able to display unicode string
* and ansi strings whether it is built as a unicode app or not.
* This is why the functions related to text display are able
* to handle both ansi and unicode strings and are specifically
* calling either the unicode or the ansi API.
* The three functions
*                 1) CchLineA(),
*                 2) CchLineW() and
*                 3) ShowText().
* are able to handle both ansi and unicode text, they contain some code
* that depend on ansi or unicode version of the system APIs and should
* stay the same whether UNICODE is defined or not (whether we are building
* a unicode app or not).
*
* There you will find some occurences of CHAR, WCHAR, LPSTR and LPWSTR.
*
* The Win32 API specifies clipboard ID as 32bit values and the BITMAP and
* METAFILEPICT structures contain fields that are twice as large as the
* same fields under Windows 3.1
* Saving the content of the clipboard into .CLP files that are readable by the
* Windows 3.1 clipboard viewer results in a loss of information.
* As the user may not want this loss of information unless (s)he is planning on
* using the .CLP file with a 16bit version of Windows (s)he is now given the
* oportunity to save the data in a new .CLP format that won't be recognized by
* the Windows 3.1 clipboard viewer (the FileID is different).
*
*/



#include "clipbrd.h"
#include "dib.h"
#include <shellapi.h>
#include <memory.h>

void NEAR PASCAL ShowString(HDC, WORD);

BOOL    fAnythingToRender = FALSE;
BOOL    fOwnerDisplay = FALSE;
BOOL    fDisplayFormatChanged = TRUE;

TCHAR    szAppName[] = TEXT("Clipboard");
TCHAR    szCaptionName[CAPTIONMAX];
TCHAR    szHelpFileName[20];

TCHAR    szMemErr[MSGMAX];

HWND    hwndNextViewer = NULL;
HWND    hwndMain;

HINSTANCE  hInst;
HANDLE  hAccel;
HANDLE  hfontSys;
HANDLE  hfontOem;
HANDLE  hfontUni;

HBRUSH  hbrWhite;
HBRUSH  hbrBackground;

HMENU   hMainMenu;
HMENU   hDispMenu;

/* The scroll information for OWNER display is to be preserved, whenever
 * the display changes between OWNER and NON-OWNER; The following globals
 * are used to save and restore the scroll info.
 */
INT     OwnVerMin, OwnVerMax, OwnHorMin, OwnHorMax;
INT     OwnVerPos, OwnHorPos;

LONG    cyScrollLast = -1;              /* Y-offset of display start when maximally */
                                        /*   scrolled down; -1==invalid             */
LONG    cyScrollNow  = 0;               /* Y-offset of current display start        */
                                        /*   (0=scrolled up all the way)            */
INT     cxScrollLast = -1;              /* Like cyScrollLast, but for horiz scroll  */
INT     cxScrollNow  = 0;               /* Like cyScrollNow, but for horiz scroll   */

RECT    rcWindow;                       /* Window in which to paint clipboard info  */

UINT      cyLine, cxChar, cxMaxCharWidth; /* Size of a standard text char             */
UINT      cxMargin, cyMargin;             /* White border size around clip data area  */

UINT    CurSelFormat = CBM_AUTO;

/* Defines priority order for show format */
UINT    rgfmt[] = {
                    CF_OWNERDISPLAY,
                    CF_DSPTEXT,
                    CF_DSPBITMAP,
                    CF_DSPENHMETAFILE,
                    CF_DSPMETAFILEPICT,
                    CF_TEXT,
                    CF_UNICODETEXT,
                    CF_OEMTEXT,
                    CF_ENHMETAFILE,
                    CF_METAFILEPICT,
                    CF_BITMAP,
                    CF_DIB,
                    CF_PALETTE,
                    CF_RIFF,
                    CF_WAVE,
                    CF_SYLK,
                    CF_DIF,
                    CF_TIFF,
                    CF_PENDATA
                  };
#define ifmtMax 19


/* variables for the new File Open,File SaveAs and Find Text dialogs */

OPENFILENAME OFN;
TCHAR szFileName[PATHMAX];              /*  Unicode  AnasJ May 92            */
TCHAR szLastDir[PATHMAX];
TCHAR szDefExt[CCH_szDefExt];           /* default extension for above       */
TCHAR szFilterSpec[FILTERMAX];          /* default filter spec. for above    */
TCHAR szCustFilterSpec[FILTERMAX];      /* buffer for custom filters created */
UINT  wHlpMsg;                          /* message used to invoke Help       */
TCHAR szOpenCaption [CAPTIONMAX];       /* File open dialog caption text     */
TCHAR szSaveCaption [CAPTIONMAX];       /* File Save as dialog caption text  */

/* MemErrorMessage is called when a failure occurs on either WinHelp or
 * ShellAbout.
 */
void FAR PASCAL MemErrorMessage()
{
        MessageBeep(0);
        MessageBox(hwndMain,szMemErr,NULL,MB_ICONHAND|MB_OK);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  MyOpenClipboard() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/
BOOL MyOpenClipboard(
HWND hWnd)
{
HDC   hDC;
RECT  Rect;

if(OpenClipboard(hWnd))
   {
   return(TRUE);
   }

/* Some stupid app forgot to close the clipboard */
hDC = GetDC(hWnd);
GetClientRect(hWnd, (LPRECT)&Rect);
FillRect(hDC, (LPRECT)&Rect, hbrBackground);
ShowString(hDC, IDS_ALREADYOPEN);
ReleaseDC(hWnd, hDC);
return(FALSE);
}

/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  ChangeCharDimensions() -                                                    */
/*                                                                            */
/*--------------------------------------------------------------------------*/
void ChangeCharDimensions(
HWND hwnd,
UINT wOldFormat,
UINT wNewFormat)
{
/* Check if the font has changed. */
if (wOldFormat != wNewFormat)
   {
   switch (wNewFormat)
      {
   case CF_OEMTEXT:
      SetCharDimensions(hwnd, hfontOem);
      break;
   case CF_UNICODETEXT:
      SetCharDimensions(hwnd, hfontUni);
      break;
   case CF_TEXT:
   default:
      SetCharDimensions(hwnd, hfontSys);
      break;
      }
   }
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  RenderFormat() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/
/* Read the data from fh and SetClipboardData() with it. */
BOOL RenderFormat(
FORMATHEADER *FormatHeader,
register INT fh)
{
  HANDLE            hBitmap;
  register HANDLE   hData;
  LPBYTE             lpData;
  DWORD             MetaOffset;     /* special case hack for metafiles */
  BITMAP            bitmap;
  HPALETTE          hPalette;
  LPLOGPALETTE      lpLogPalette;

  if (PRIVATE_FORMAT(FormatHeader->FormatID))
      FormatHeader->FormatID = (UINT)RegisterClipboardFormat(FormatHeader->Name);

  /* Special case hack for metafiles to get hData referencing
   * the metafile bits, not the METAFILEPICT structure.
   */

  switch (FormatHeader->FormatID)
    {
      case CF_METAFILEPICT:
          if (fNTReadFileFormat)
              MetaOffset = sizeof(METAFILEPICT);
          else
              MetaOffset = SIZE_OF_WIN31_METAFILEPICT_STRUCT;
          break;
      case CF_BITMAP:
          if (fNTReadFileFormat)
              MetaOffset = sizeof(BITMAP);
          else
              MetaOffset = SIZE_OF_WIN31_BITMAP_STRUCT;
          break;
      default:
          MetaOffset = 0;
          break;
    }


  if (!(hData = GlobalAlloc(GHND, FormatHeader->DataLen - MetaOffset)))
      return(FALSE);

  if (!(lpData = GlobalLock(hData)))
    {
      GlobalFree(hData);
      return(FALSE);
    }

  _llseek(fh, FormatHeader->DataOffset + MetaOffset, 0);
  if(!lread(fh, lpData, FormatHeader->DataLen - MetaOffset))
    {
      /* Error in reading the file */
      GlobalUnlock(hData);
      GlobalFree(hData);
      return(FALSE);
    }

  GlobalUnlock(hData);

  /* As when we write these we have to special case a few of
   * these guys.  This code and the write code should match in terms
   * of the sizes and positions of data blocks being written out.
   */

  switch (FormatHeader->FormatID)
    {
      case CF_ENHMETAFILE:
        {
          HENHMETAFILE     hEMF;

          hEMF = (HENHMETAFILE)SetEnhMetaFileBits(FormatHeader->DataLen, lpData);
          GlobalUnlock(hData);
          GlobalFree(hData);
          hData = hEMF;          /* Stuff this in the clipboard */
          break;
        }
      case CF_METAFILEPICT:
        {
          HANDLE            hMF;
          HANDLE            hMFP;
          LPMETAFILEPICT    lpMFP;

          /* Create the METAFILE with the bits we read in. */

          if (!(hMF = SetMetaFileBitsEx(FormatHeader->DataLen, lpData)))  /* portable code */
              return(FALSE);

          /* Alloc a METAFILEPICT header. */

          if (!(hMFP = GlobalAlloc(GHND, (DWORD)sizeof(METAFILEPICT))))
              return(FALSE);

          if (!(lpMFP = (LPMETAFILEPICT)GlobalLock(hMFP)))
            {
              GlobalFree(hMFP);
              return(FALSE);
            }

          /* Reposition to the start of the METAFILEPICT header. */
          _llseek(fh, FormatHeader->DataOffset, 0);

          /* Read it in. */
          if (fNTReadFileFormat)
              _lread(fh, (LPBYTE)lpMFP, sizeof(METAFILEPICT));
          else {
              /* If we read a win 3.1 metafile we have to read the fields
                 one after the other as they aren't of the same size as
                 the corresponding Win 3.1 METAFILEPICT structure fields.
                 We initialize the fields to zero their hight word.
                 [pierrej 5/27/92]                                        */
              lpMFP->mm = 0;
              lpMFP->xExt = 0;
              lpMFP->yExt = 0;
              _lread(fh, (LPBYTE)&(lpMFP->mm), sizeof(WORD));
              _lread(fh, (LPBYTE)&(lpMFP->xExt), sizeof(WORD));
              _lread(fh, (LPBYTE)&(lpMFP->yExt), sizeof(WORD));
                /* No, we don't need to read in the handle, we wouldn't
                   use it anyways.  [pierrej, 5/27/92]                   */
            }


          lpMFP->hMF = hMF;         /* Update the METAFILE handle  */
          GlobalUnlock(hMFP);       /* Unlock the header           */
          hData = hMFP;             /* Stuff this in the clipboard */
          break;
        }

      case CF_BITMAP:
          /* Reposition to the start of the METAFILEPICT header. */
          _llseek(fh, FormatHeader->DataOffset, 0);

          /* Read it in. */
          if (fNTReadFileFormat)
              _lread(fh, (LPBYTE)&bitmap, sizeof(BITMAP));
          else {
              /* If we read a win 3.1 bitmap we have to read the fields
                 one after the other as they aren't of the same size as
                 the corresponding Win 3.1 BITMAP structure fields.
                 We initialize the fields to zero their hight word or byte.
                 [pierrej 5/27/92]                                        */

              bitmap.bmType = 0;
              bitmap.bmWidth = 0;
              bitmap.bmHeight = 0;
              bitmap.bmWidthBytes = 0;
              bitmap.bmPlanes = 0;
              bitmap.bmBitsPixel = 0;
              bitmap.bmBits = NULL;
              _lread(fh, (LPBYTE)&(bitmap.bmType), sizeof(WORD));
              _lread(fh, (LPBYTE)&(bitmap.bmWidth), sizeof(WORD));
              _lread(fh, (LPBYTE)&(bitmap.bmHeight), sizeof(WORD));
              _lread(fh, (LPBYTE)&(bitmap.bmWidthBytes), sizeof(WORD));
              _lread(fh, (LPBYTE)&(bitmap.bmPlanes), sizeof(BYTE));
              _lread(fh, (LPBYTE)&(bitmap.bmBitsPixel), sizeof(BYTE));
              _lread(fh, (LPBYTE)&(bitmap.bmBits), sizeof(LPVOID));
            }

          if (!(lpData = GlobalLock(hData)))
             {
             GlobalFree(hData);
             return FALSE;
             }

          bitmap.bmBits = lpData;

          /* If this fails we should avoid doing the SetClipboardData()
           * below with the hData check.
           */

          hBitmap = CreateBitmapIndirect(&bitmap);

          GlobalUnlock(hData);
          GlobalFree(hData);
          hData = hBitmap;          /* Stuff this in the clipboard */
          break;

      case CF_DIB:
         break;

      case CF_PALETTE:
          if (!(lpLogPalette = (LPLOGPALETTE)GlobalLock(hData)))
            {
              GlobalFree(hData);
              return(FALSE);
            }

          /* Create a logical palette. */
          if (!(hPalette = CreatePalette(lpLogPalette)))
            {
              GlobalUnlock(hData);
              GlobalFree(hData);
              return(FALSE);
            }
          GlobalUnlock(hData);
          GlobalFree(hData);

          hData = hPalette;         /* Stuff this into clipboard */
          break;
    }

  if (!hData)
      return(FALSE);

  SetClipboardData(FormatHeader->FormatID, hData);
  return(TRUE);
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  ClipbrdVScroll() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

/* Scroll contents of window vertically, according to action code in wParam. */

void ClipbrdVScroll(
HWND hwnd,
WORD wParam,
WORD wThumb)
{
  INT           cyWindow;
  LONG          dyScroll;
  LONG          cyScrollT;
  LONG          dyScrollAbs;
  LONG          cyPartialChar;

  /* Ensure that all the bits are valid first, before scrolling them */
  UpdateWindow(hwnd);

  cyScrollT = cyScrollNow;
  cyWindow = rcWindow.bottom - rcWindow.top;

  /* Compute scroll results as an effect on cyScrollNow */
  switch (wParam)
    {
      case SB_LINEUP:
          cyScrollT -= cyLine;
          break;

      case SB_LINEDOWN:
          cyScrollT += cyLine;
          break;

      case SB_THUMBPOSITION:
          cyScrollT = (LONG)(((LONG)wThumb * (LONG)cyScrollLast) / VPOSLAST);
          break;

      case SB_PAGEUP:
      case SB_PAGEDOWN:
        {
          INT   cyPageScroll;

          cyPageScroll = cyWindow - cyLine;

          if (cyPageScroll < (INT)cyLine)
              cyPageScroll = cyLine;

          cyScrollT += (wParam == SB_PAGEUP) ? -cyPageScroll : cyPageScroll;
          break;
        }

      default:
          return;
    }

  if ((cyScrollT < 0) || (cyScrollLast <= 0))
      cyScrollT = 0;
  else if (cyScrollT > cyScrollLast)
      cyScrollT = cyScrollLast;
  else if ((cyPartialChar = cyScrollT % cyLine) != 0)
    {
      /* Round to the nearest character increment. */
      if (cyPartialChar > (LONG)(cyLine >> 1))
          cyScrollT += cyLine;
      cyScrollT -= cyPartialChar;
    }

  dyScroll = cyScrollNow - cyScrollT;
  if (dyScroll > 0)
      dyScrollAbs = dyScroll;
  else if (dyScroll < 0)
      dyScrollAbs = -dyScroll;
  else
      return;                       /* Scrolling has no effect here. */

  cyScrollNow = cyScrollT;

  if (dyScrollAbs >= rcWindow.bottom - rcWindow.top)
      /* ScrollWindow does not handle this case */
      InvalidateRect(hwnd, (LPRECT)&rcWindow, TRUE);
  else
      ScrollWindow(hwnd, 0, (INT)dyScroll, &rcWindow, &rcWindow);

  UpdateWindow(hwnd);

  SetScrollPos(hwnd,
               SB_VERT,
               (cyScrollLast <= 0) ? 0 : (INT)((cyScrollT * (DWORD)VPOSLAST) / cyScrollLast),
               TRUE);
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  ClipbrdHScroll() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

/* Scroll contents of window horizontally, according to op code in wParam. */

void NEAR PASCAL ClipbrdHScroll(HWND hwnd,WORD wParam,WORD wThumb)

{
  INT           cxWindow;
  register INT  dxScroll;
  register INT  cxScrollT;
  INT           dxScrollAbs;
  LONG          cxPartialChar;

  cxScrollT = cxScrollNow;
  cxWindow = rcWindow.right - rcWindow.left;

  /* Compute scroll results as an effect on cxScrollNow */
  switch (wParam)
    {
      case SB_LINEUP:
          cxScrollT -= cxChar;
          break;

      case SB_LINEDOWN:
          cxScrollT += cxChar;
          break;

      case SB_THUMBPOSITION:
          cxScrollT = (INT)(((LONG)wThumb * (LONG)cxScrollLast) / HPOSLAST);
          break;

      case SB_PAGEUP:
      case SB_PAGEDOWN:
        {
          INT   cxPageScroll;

          cxPageScroll = cxWindow - cxChar;
          if (cxPageScroll < (INT)cxChar)
              cxPageScroll = cxChar;

          cxScrollT += (wParam == SB_PAGEUP) ? -cxPageScroll : cxPageScroll;
          break;
        }

      default:
          return;
    }

  if ((cxScrollT < 0) || (cxScrollLast <= 0))
      cxScrollT = 0;
  else if (cxScrollT > cxScrollLast)
      cxScrollT = cxScrollLast;
  else if ((cxPartialChar = cxScrollT % cxChar) != 0)
    { /* Round to the nearest character increment */
      if (cxPartialChar > (LONG)(cxChar >> 1))
          cxScrollT += cxChar;
      cxScrollT -= cxPartialChar;
    }

  /* Now we have a good cxScrollT value */

  dxScroll = cxScrollNow - cxScrollT;
  if (dxScroll > 0)
      dxScrollAbs = dxScroll;
  else if (dxScroll < 0)
      dxScrollAbs = -dxScroll;
  else
      return;                       /* Scrolling has no effect here. */

  cxScrollNow = cxScrollT;

  if (dxScrollAbs >= rcWindow.right - rcWindow.left)
      /* ScrollWindow does not handle this case */
      InvalidateRect( hwnd, (LPRECT) & rcWindow, TRUE );
  else
      ScrollWindow(hwnd, dxScroll, 0, (LPRECT)&rcWindow, (LPRECT)&rcWindow);

  UpdateWindow(hwnd);

  SetScrollPos(hwnd,
               SB_HORZ,
               (cxScrollLast <= 0) ? 0 : (INT)(((DWORD)cxScrollT * (DWORD)HPOSLAST) / (DWORD)cxScrollLast),
               TRUE );
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  DibPaletteSize() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

INT NEAR PASCAL DibPaletteSize(LPBITMAPINFOHEADER  lpbi)

{
  register INT  bits;

  /* With the new format headers, the size of the palette is in biClrUsed
   * else is dependent on bits per pixel.
   */
  if (lpbi->biSize != sizeof(BITMAPCOREHEADER))
    {
      if (lpbi->biClrUsed != 0)
          return((WORD)lpbi->biClrUsed * sizeof(RGBQUAD));

      bits = lpbi->biBitCount;
      return((bits == 24) ? 0 : (1 << bits) * sizeof(RGBQUAD));
    }
  else
    {
      bits = ((LPBITMAPCOREHEADER)lpbi)->bcBitCount;
      return((bits == 24) ? 0 : (1 << bits) * sizeof(RGBTRIPLE));
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  DibGetInfo() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

void NEAR PASCAL DibGetInfo(HANDLE hdib,LPBITMAP pbm)

{
  LPBITMAPINFOHEADER lpbi;

  lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib);

  if (lpbi->biSize != sizeof(BITMAPCOREHEADER))
    {
      pbm->bmWidth  = (INT)lpbi->biWidth;
      pbm->bmHeight = (INT)lpbi->biHeight;
    }
  else
    {
      pbm->bmWidth  = (INT)((LPBITMAPCOREHEADER)lpbi)->bcWidth;
      pbm->bmHeight = (INT)((LPBITMAPCOREHEADER)lpbi)->bcHeight;
    }

  GlobalUnlock(hdib);
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  DrawDib() -                                                             */
/*                                                                            */
/*--------------------------------------------------------------------------*/

BOOL NEAR PASCAL DrawDib(HDC hdc,INT x0,INT y0,HANDLE hdib)

{
  BITMAP                bm;
  LPBYTE                 lpBits;
  LPBITMAPINFOHEADER    lpbi;

  if (!hdib)
      return(FALSE);

  lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib);

  if (!lpbi)
      return(FALSE);

  DibGetInfo(hdib, (LPBITMAP)&bm);

  lpBits = (LPBYTE)lpbi + (WORD)lpbi->biSize + DibPaletteSize(lpbi);

  SetDIBitsToDevice(hdc,
                    x0, y0, bm.bmWidth, bm.bmHeight,
                    0, 0,
                    0, bm.bmHeight,
                    lpBits, (LPBITMAPINFO)lpbi,
                    DIB_RGB_COLORS);

  GlobalUnlock(hdib);
  return(TRUE);
}


/*--------------------------------------------------------------------------
  FShowDIBitmap() -

  Parameters:
      hdc - DC to draw into.
      prc - Pointer to bounds rectangle within DC.
      hdib - DIB to draw.
      cxScroll, cyScroll - Position the window's scrolled to.
--------------------------------------------------------------------------*/

BOOL NEAR PASCAL FShowDIBitmap(register HDC hdc,PRECT prc,
            HANDLE hdib /* Bitmap in DIB format */,INT cxScroll,INT cyScroll)

{
  BITMAP    bm;

  DibGetInfo(hdib, (LPBITMAP)&bm);

  if (cyScrollLast == -1)
    {
      /* Compute last scroll offset into bitmap */
      cyScrollLast = bm.bmHeight - (rcWindow.bottom - rcWindow.top);
      if (cyScrollLast < 0)
          cyScrollLast = 0;
    }

  if (cxScrollLast == -1)
    {
      /* Compute last scroll offset into bitmap */
      cxScrollLast = bm.bmWidth - (rcWindow.right - rcWindow.left);
      if (cxScrollLast < 0)
          cxScrollLast = 0;
    }

  SaveDC(hdc);
  IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
  /* MSetViewportOrg(hdc, prc->left - cxScroll, prc->top - cyScroll); */
  DrawDib(hdc, -cxScroll, -cyScroll, hdib);
  RestoreDC(hdc, -1);
  return(TRUE);
}


/*--------------------------------------------------------------------------
  FShowBitmap() -

  Purpose: Draw a bitmap in the given HDC.

  Parameters:
      hdc - hDC to draw into.
      prc - Bounds rectangle in the DC.
      hbm - The bitmap to draw
      cxScroll, cyScroll - Where the DC's scrolled to

  Returns:
      TRUE on success, FALSE on failure.
--------------------------------------------------------------------------*/

BOOL NEAR PASCAL FShowBitmap(HDC hdc,register PRECT prc,
                    HBITMAP hbm,INT cxScroll,INT cyScroll)
{
  register HDC  hMemDC;
  BITMAP        bitmap;
  INT           cxBlt, cyBlt;
  INT           cxRect, cyRect;

  if ((hMemDC = CreateCompatibleDC(hdc)) == NULL)
      return(FALSE);

  SelectObject(hMemDC, (HBITMAP)hbm);
  GetObject((HBITMAP)hbm, sizeof(BITMAP), (LPBYTE)&bitmap);

  if (cyScrollLast == -1)
    {
      /* Compute last scroll offset into bitmap */
      cyScrollLast = bitmap.bmHeight - (rcWindow.bottom - rcWindow.top);
      if (cyScrollLast < 0)
          cyScrollLast = 0;
    }

  if (cxScrollLast == -1)
    {
      /* Compute last scroll offset into bitmap */
      cxScrollLast = bitmap.bmWidth - (rcWindow.right - rcWindow.left);
      if (cxScrollLast < 0)
          cxScrollLast = 0;
    }

  cxRect = prc->right - prc->left;
  cyRect = prc->bottom - prc->top;

/* Bug #10656:  Subtract 1 so we won't fall off the end of the bitmap
 *              (the bitmap is zero based).
 *   11 January 1992            Clark R. Cyr
 */
     cxBlt = min(cxRect, bitmap.bmWidth - cxScroll);
     cyBlt = min(cyRect, bitmap.bmHeight - cyScroll);

/* Bug #14131:  Instead of subtracting 1 from the amount to blt, subtract 1
 *              from the offset to blt from, thus allowing for a full picture
 *              when no scrolling is needed.
 *   06 February 1992           Clark R. Cyr
 */
     if ((cxBlt != cxRect) && (cxScroll > 0))
          cxScroll--;
     if ((cyBlt != cyRect) && (cyScroll > 0))
          cyScroll--;

  BitBlt(hdc, prc->left, prc->top,
         cxBlt, cyBlt,
         hMemDC,
         cxScroll, cyScroll,     /* X,Y offset into source DC */
         SRCCOPY);

  DeleteDC(hMemDC);
  return(TRUE);
}

#define DXPAL  (cyLine)
#define DYPAL  (cyLine)

/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  FShowPalette() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

BOOL NEAR PASCAL FShowPalette(register HDC hdc,register PRECT prc,
                    HPALETTE hpal,INT cxScroll,INT cyScroll)

{
  INT     n;
  INT     x, y;
  INT     nx, ny;
  INT     nNumEntries = 0;
  RECT    rc;
  HBRUSH  hbr;

  if (!hpal)
      return(FALSE);

  GetObject(hpal, sizeof(INT), (LPBYTE)&nNumEntries);

  nx = (rcWindow.right - rcWindow.left) / DXPAL;

  if (nx == 0)
      nx = 1;

  ny = (nNumEntries + nx - 1) / nx;

  if (cyScrollLast == -1)
    {
      /* Compute last scroll offset into bitmap */
      cyScrollLast = ny * DYPAL - (rcWindow.bottom - rcWindow.top);
      if (cyScrollLast < 0)
          cyScrollLast = 0;
    }

  if (cxScrollLast == -1)
    {
      /* Compute last scroll offset into bitmap */
      cxScrollLast = 0;
    }

  SaveDC(hdc);
  IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
  MSetViewportOrg(hdc, prc->left - cxScroll, prc->top - cyScroll);

  SelectPalette(hdc, hpal, FALSE);
  RealizePalette(hdc);

  x = 0;
  y = -((int)DYPAL);

  for (n=0; n < nNumEntries; n++, x += DXPAL)
    {
      if (n % nx == 0)
        {
          x = 0;
          y += DYPAL;
        }

      rc.left        = x;
      rc.top        = y;
      rc.right        = x + DXPAL;
      rc.bottom = y + DYPAL;

      if (RectVisible(hdc,&rc))
        {
          InflateRect(&rc, -1, -1);
          FrameRect(hdc, &rc, GetStockObject(BLACK_BRUSH));
          InflateRect(&rc, -1, -1);
          hbr = CreateSolidBrush(PALETTEINDEX(n));
          FillRect(hdc, &rc, hbr);
          DeleteObject(hbr);
        }
    }
  RestoreDC(hdc, -1);
  return(TRUE);
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  PxlConvert() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

/* Return the # of pixels spanned by 'val', a measurement in coordinates
 * appropriate to mapping mode mm.  'pxlDeviceRes' gives the resolution
 * of the device in pixels, along the axis of 'val'. 'milDeviceRes' gives
 * the same resolution measurement, but in millimeters.
 */

INT NEAR PASCAL PxlConvert(INT mm, INT val,INT pxlDeviceRes,INT milDeviceRes)
{
  register WORD wMult = 1;
  register WORD wDiv = 1;
  DWORD         ulPxl;
  DWORD         ulDenom;
  /* Should be a constant!  This works around a compiler bug as of 07/14/85. */
  DWORD         ulMaxInt = MAXSHORT;

  if (milDeviceRes == 0)
    {
      /* to make sure we don't get divide-by-0 */
      return(0);
    }

  switch (mm)
    {
      case MM_LOMETRIC:
          wDiv = 10;
          break;

      case MM_HIMETRIC:
          wDiv = 100;
          break;

      case MM_TWIPS:
          wMult = 254;
          wDiv = 14400;
          break;

      case MM_LOENGLISH:
          wMult = 2540;
          wDiv = 10000;
          break;

      case MM_HIENGLISH:
          wMult = 254;
          wDiv = 10000;
          break;

      case MM_TEXT:
          return(val);

      case MM_ISOTROPIC:
      case MM_ANISOTROPIC:
              /* These picture types have no original size */
      default:
              return(0);
    }

  /* Add denominator - 1 to numerator, to avoid roundoff */

  ulDenom = (DWORD)wDiv * (DWORD)milDeviceRes;
  ulPxl = (((DWORD)((DWORD)wMult * (DWORD)val * (DWORD)pxlDeviceRes)) + ulDenom - 1) / ulDenom;

  return((ulPxl > ulMaxInt) ? 0 : (INT)ulPxl);
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  FShowEnhMetaFile() -                                                    */
/*                                                                            */
/*--------------------------------------------------------------------------*/

/* Display an enhanced metafile in the specified rectangle. */

BOOL NEAR PASCAL FShowEnhMetaFile(
register HDC        hdc,
HANDLE hemf,
LPRECT prc)
{
  INT               level;
  INT               f = FALSE;
  ENHMETAHEADER     EnhHeader;

      if ((level = SaveDC( hdc )) != 0)
        {

          cyScrollLast = 0;
          cxScrollLast = 0;

          GetEnhMetaFileHeader(hemf, sizeof(ENHMETAHEADER), &EnhHeader);

          rcWindow.top--;
          rcWindow.left--;

          SetWindowOrgEx(hdc, -prc->left, -prc->top, NULL);
          f = PlayEnhMetaFile(hdc, hemf, &rcWindow);

          rcWindow.top++;
          rcWindow.left++;
          RestoreDC(hdc, level);
        }
  return(f);
}

/*--------------------------------------------------------------------------
  FShowMetaFilePict()

  Display a metafile in the specified rectangle.
--------------------------------------------------------------------------*/


BOOL NEAR PASCAL FShowMetaFilePict(
register HDC   hdc,
register PRECT prc,
HANDLE         hmfp,
INT            cxScroll,
INT            cyScroll)
{
  INT               level;
  INT               cxBitmap;
  INT               cyBitmap;
  INT               f = FALSE;
  LPMETAFILEPICT    lpmfp;

  if ((lpmfp = (LPMETAFILEPICT)GlobalLock( hmfp )) != NULL)
    {
      METAFILEPICT mfp;

      mfp = *lpmfp;
      GlobalUnlock( hmfp );

      if ((level = SaveDC( hdc )) != 0)
        {
          /* Compute size of picture to be displayed */
          switch (mfp.mm)
             {
          case MM_ISOTROPIC:
          case MM_ANISOTROPIC:
             /* Not scrollable.  Resize these into the given rect. */
             cyScrollLast = 0;
             cxScrollLast = 0;
             cxBitmap = rcWindow.right - rcWindow.left;
             cyBitmap = rcWindow.bottom - rcWindow.top;
             break;

          default:
             cxBitmap = PxlConvert(mfp.mm, mfp.xExt,
                  GetDeviceCaps(hdc, HORZRES),
                  GetDeviceCaps(hdc, HORZSIZE)
                  );
             cyBitmap = PxlConvert(mfp.mm, mfp.yExt,
                  GetDeviceCaps(hdc, VERTRES),
                  GetDeviceCaps(hdc, VERTSIZE)
                  );
             if (!cxBitmap || !cyBitmap)
                 goto NoDisplay;

             if (cxScrollLast == -1)
                {
                cxScrollLast = cxBitmap - (rcWindow.right - rcWindow.left);
                if (cxScrollLast < 0)
                   cxScrollLast = 0;
                }

             if (cyScrollLast == -1)
                {
                cyScrollLast = cyBitmap - (rcWindow.bottom - rcWindow.top);
                if (cyScrollLast < 0)
                   cyScrollLast = 0;
                }
             break;
             }

          /* We make the "viewport" to be an area the same size as the
           * clipboard object, and set the origin and clip region so as
           * to show the area we want. Note that the viewport may well be
           * bigger than the window.
           */
          // IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
          SetMapMode(hdc, mfp.mm);

          // MSetViewportOrg(hdc, cxScroll, cyScroll);
          switch (mfp.mm)
            {
              case MM_ANISOTROPIC:
                  if (mfp.xExt && mfp.yExt)
                    {
                      /* So we get the correct shape rectangle when
                       * SetViewportExtEx gets called.
                       */
                      MSetWindowExt(hdc, mfp.xExt, mfp.yExt);
                    }
                  /* FALL THRU */

              case MM_ISOTROPIC:
                  MSetViewportExt(hdc, cxBitmap, cyBitmap);
                  break;
            }

          /* Since we may have scrolled, force brushes to align */
          // MSetBrushOrg(hdc, cxScroll - prc->left, cyScroll - prc->top);
          f = PlayMetaFile(hdc, mfp.hMF);
NoDisplay:
          RestoreDC(hdc, level);
        }
    }
  return(f);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ShowString() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Blank rcWindow and show the string on the top line of the client area. */

void NEAR PASCAL ShowString(HDC        hdc,WORD id)

{
  TCHAR        szBuffer[BUFFERLEN];

  /* Cancel any scrolling effects. */
  cyScrollNow = 0;
  cxScrollNow = 0;

  LoadString(hInst, id, szBuffer, BUFFERLEN);
  FillRect(hdc, &rcWindow, hbrBackground);
  DrawText(hdc, szBuffer, -1, &rcWindow, DT_CENTER | DT_WORDBREAK | DT_NOCLIP | DT_TOP);
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  CchLineA() -                                                             */
/*                                                                            */
/*--------------------------------------------------------------------------*/

/* Determine the # of characters in one display line's worth of lpch.
 * lpch is assumed to be an ansi string.
 *
 * Return the following:
 *       HI WORD:    # of chars to display (excludes CR, LF; will not
 *                   exceed cchLine)
 *       LO WORD:    offset of start of next line in lpch; If the current line
 *                   is NULL terminated, this contains offset to the NULL char;
 *       In RgchBuf: characters to display
 *
 *   Expands Tabs
 *
 *   Accepts any of the following as valid end-of-line terminators:
 *       CR, LF, CR-LF, LF-CR, NULL
 *   Callers may test for having reached NULL by (lpch[LOWORD] == '\0')
 */

LONG NEAR PASCAL CchLineA(HDC hDC, CHAR rgchBuf[],
#ifdef WIN16
                    CHAR huge *lpch,
#else
                    CHAR FAR  *lpch,
#endif
                    INT cchLine,WORD wWidth)

{
  CHAR          ch;
  CHAR         *pch = rgchBuf;
  register INT  cchIn = 0;
  register INT  cchOut = 0;
  INT           iMinNoOfChars;
  SIZE          size;
  INT           iTextWidth = 0;

  iMinNoOfChars = wWidth / cxMaxCharWidth;

  while (cchOut < cchLine)
    {
      switch (ch = *(lpch + (DWORD)cchIn++))
        {
          case '\0':
               /* cchIn is already incremented; So, it is pointing to
               * a character beyond the NULL; So, decrement it.
               */
              cchIn--;
              goto DoubleBreak;

          case '\015':  /* CR */
          case '\012':  /* LF */
              if ((lpch[cchIn] == '\015') || (lpch[cchIn] == '\012'))
                  cchIn++;
              goto DoubleBreak;

          case '\011':  /* TAB */
            {
              INT   cchT = 8 - (cchOut % 8);

              /* Check if the width has exceeded or the total
               * number of characters has exceeded
               */
              if (((WORD)(iTextWidth + cchT * cxChar) > wWidth) || ((cchOut+cchT) >= cchLine))
                  /* Tab causes wrap to next line */
                  goto DoubleBreak;

              while (cchT--)
                  rgchBuf[cchOut++] = ' ';
              break;
            }

          default:
              rgchBuf[cchOut++] = ch;
#ifdef DBCS
              if( IsDBCSLeadByte(ch) )
                  rgchBuf[cchOut++] = *(lpch + (DWORD)cchIn++);
#endif
              break;
        }

      /* Check if the width has been exceeded. */
      if (cchOut >= iMinNoOfChars)
        {
          GetTextExtentPointA(hDC, rgchBuf, cchOut, (LPSIZE)&size);
          iTextWidth = size.cx;
          if ((WORD)iTextWidth == wWidth)
              break;
          else if((WORD)iTextWidth > wWidth)
              {
#ifdef DBCS
                if( IsDBCSLeadByte(ch) ){
                    cchOut--;
                    cchIn--;
                }
#endif
                cchOut--;
                cchIn--;
                break;
              }

          iMinNoOfChars += (wWidth - iTextWidth) / cxMaxCharWidth;
        }
    }
DoubleBreak:
  return(MAKELONG(cchIn, cchOut));
}

/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  CchLineW() -                                                             */
/*                                                                            */
/*--------------------------------------------------------------------------*/

/*Same as previous function but takes unicode strings.
 */

LONG NEAR PASCAL CchLineW(HDC hDC, WCHAR rgchBuf[],
                    WCHAR FAR  *lpch,
                    INT cchLine,WORD wWidth)

{
  WCHAR                ch;
  WCHAR                *pch = rgchBuf;
  register INT        cchIn = 0;
  register INT        cchOut = 0;
  INT                iMinNoOfChars;
  INT           iTextWidth = 0;
  SIZE          size;

  iMinNoOfChars = wWidth / cxMaxCharWidth;

  while (cchOut < cchLine)
    {
      switch (ch = *(lpch + (DWORD)cchIn++))
        {
          case L'\0':
               /* cchIn is already incremented; So, it is pointing to
               * a character beyond the NULL; So, decrement it.
               */
              cchIn--;
              goto DoubleBreak;

          case L'\015':  /* CR */
          case L'\012':  /* LF */
              if ((lpch[cchIn] == L'\015') || (lpch[cchIn] == L'\012'))
                  cchIn++;
              goto DoubleBreak;

          case L'\011':  /* TAB */
            {
              INT   cchT = 8 - (cchOut % 8);

              /* Check if the width has exceeded or the total
               * number of characters has exceeded
               */
              if (((WORD)(iTextWidth + cchT * cxChar) > wWidth) || ((cchOut+cchT) >= cchLine))
                  /* Tab causes wrap to next line */
                  goto DoubleBreak;

              while (cchT--)
                  rgchBuf[cchOut++] = L' ';
              break;
            }

          default:
              rgchBuf[cchOut++] = ch;
              break;
        }

      /* Check if the width has been exceeded. */
      if (cchOut >= iMinNoOfChars)
       {
          GetTextExtentPointW(hDC, rgchBuf, cchOut, &size);
          iTextWidth = size.cx;
          if ((WORD)iTextWidth == wWidth)
              break;
          else if((WORD)iTextWidth > wWidth)
              {
                cchOut--;
                cchIn--;
                break;
              }

          iMinNoOfChars += (wWidth - iTextWidth) / cxMaxCharWidth;
        }
    }
DoubleBreak:
  return(MAKELONG(cchIn, cchOut));
}


#define cchLineMax  200

/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  ShowText() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

void NEAR PASCAL ShowText(register HDC        hdc,PRECT prc,HANDLE h,INT cyScroll,
                         BOOL fUnicode)

{
#ifdef WIN16
  CHAR huge *lpch;
#else
  CHAR FAR  *lpch;
#endif
  INT       yT;
  INT       cLine;
  INT       cLineAllText = 0;
  RECT      rc;
  INT       yLine;
  INT       iLineFirstShow;
  WORD      wLen;
  WORD      wWidth;
  CHAR      rgch[cchLineMax*sizeof(WCHAR)];

  rc = *prc;

  /* Expand repaint rectangle as necessary to hold an exact number of
   * lines and start on an even line boundary. This is because we may
   * get arbitrarily weird repaint rectangles when popups are moved.
   * Scrolling repaint areas should require no adjustment.
   */
  rc.top -= (rc.top - rcWindow.top) % cyLine;

  /* If expanding the repaint rectangle to the next line expands it */
  /* beyond the bottom of my window, contract it one line.          */
  if ((yT = (rc.bottom - rc.top) % cyLine) != 0)
      if ((rc.bottom += cyLine - yT) > rcWindow.bottom)
          rc.bottom -= cyLine;

  if (rc.bottom <= rc.top)
      return;

  if (((wWidth = (WORD)(rcWindow.right - rcWindow.left)) <= 0) ||
      ((cLine = (rc.bottom - rc.top) / cyLine) <= 0)         ||
      (NULL == (lpch = (LPSTR)GlobalLock(h))) )
      {
        /* Bad Rectangle or Bad Text Handle */
        ShowString(hdc, IDS_ERROR);
        return;
      }

  /* Advance lpch to point at the text for the first line to show. */
  iLineFirstShow = cyScroll / cyLine;

  /* Advance lpch to point at text for that line. */
  if (!fUnicode)
      while ((*lpch) && (iLineFirstShow--))
        {
          lpch += LOWORD(CchLineA(hdc, rgch, lpch, cchLineMax, wWidth));
          cLineAllText++;
        }
  else
      while ((*((WCHAR *)lpch)) && (iLineFirstShow--))
        {
          lpch += ((LOWORD(CchLineW(hdc, (WCHAR *)rgch, (WCHAR FAR *)lpch, cchLineMax, wWidth)))*sizeof(WCHAR));
          cLineAllText++;
        }

  /* Display string, line by line */
  yLine = rc.top;
  while (cLine--)
    {
      LONG lT;

      if (!fUnicode)
         lT = CchLineA(hdc, rgch, lpch, cchLineMax, wWidth);
      else
         lT = CchLineW(hdc, (WCHAR *)rgch, (WCHAR FAR *)lpch, cchLineMax, wWidth);
      wLen = LOWORD(lT);
      if (!fUnicode) {
          TextOutA(hdc, rc.left, yLine, (LPSTR) rgch, HIWORD(lT));
          lpch += wLen;
      } else {
          if (!TextOutW(hdc, rc.left, yLine, (LPCWSTR) rgch, HIWORD(lT))) {
            GetLastError();
            }
          lpch += (wLen * sizeof(WCHAR));
      }
      yLine += cyLine;
      cLineAllText++;
      if ((!fUnicode && (*lpch == 0)) || (fUnicode && (*((WCHAR *)lpch) == L'\0')))
          break;
    }

  if (cxScrollLast == -1)
      /* We don't use horiz scroll for text */
      cxScrollLast = 0;

  if (cyScrollLast == -1)
    {
      INT   cLineInRcWindow;

      /* Validate y-size of text in clipboard. */
      /* Adjust rcWindow dimensions for text display */
      cLineInRcWindow = (rcWindow.bottom - rcWindow.top) / cyLine;

      do {
          if (!fUnicode)
              lpch += LOWORD(CchLineA(hdc, rgch, lpch, cchLineMax, wWidth));
          else
              lpch += ((LOWORD(CchLineW(hdc, (WCHAR *)rgch, (WCHAR FAR *)lpch, cchLineMax, wWidth)))*sizeof(WCHAR));
          cLineAllText++;
      } while ((!fUnicode && (*lpch != 0)) || (fUnicode && ((*lpch != 0) || (*(lpch+1) != 0))));

      cyScrollLast = (cLineAllText - cLineInRcWindow) * cyLine;
      if (cyScrollLast < 0)
         {
         cyScrollLast = 0;
         }

      /* Restrict rcWindow so that it holds an exact # of text lines */
      rcWindow.bottom = rcWindow.top + (cLineInRcWindow * cyLine);
    }
  GlobalUnlock(h);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SendOwnerMessage() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/
void SendOwnerMessage(
UINT message,
WPARAM wParam,
LPARAM lParam)
{
register HWND hwndOwner;

/* Send a message to the clipboard owner, if there is one */
hwndOwner = GetClipboardOwner();

if (hwndOwner != NULL)
   {
   SendMessage(hwndOwner, message, wParam, lParam);
   }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SendOwnerSizeMessage() -                                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Send WM_SIZECLIPBOARD message to clipboard owner.
 *    wParam is a handle to the clipboard window
 *    LOWORD(lParam) is a handle to the passed rect
 */
void SendOwnerSizeMessage(
HWND hwnd,
INT  left,
INT  top,
INT  right,
INT  bottom)
{
register HANDLE   hrc;
LPRECT            lprc;

if ((hrc = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                       (LONG)sizeof(RECT))) != NULL )
   {
   if ((lprc = (LPRECT)GlobalLock(hrc)) != NULL )
      {
      lprc->top = top;
      lprc->bottom = bottom;
      lprc->left = left;
      lprc->right = right;
      GlobalUnlock(hrc);
      SendOwnerMessage(WM_SIZECLIPBOARD, (WPARAM)hwnd, (LONG)hrc);
      }
   GlobalFree(hrc);
   }
}

//
// Purpose: Return the best clipboard format we have available, given a
//    'starting' clipboard format.
//
// Parameters:
//    wFormat - The format selected on the Display menu.
//
// Returns:
//    The number of the clipboard format to use, or NULL if no clipboard
//    format matching the requested one exists.
//
////////////////////////////////////////////////////////////////////////
UINT GetBestFormat(
UINT  wFormat)
{
register UINT cFmt;
register UINT *pfmt;

if (wFormat == CBM_AUTO)
   {
   wFormat = 0;

   for (cFmt=ifmtMax, pfmt=&rgfmt[0]; cFmt--; pfmt++)
      {
      if (IsClipboardFormatAvailable(*pfmt))
         {
         wFormat = *pfmt;
         break;
         }
      }
   }
else if (!IsClipboardFormatAvailable(wFormat))
   {
   wFormat = 0;
   }
return wFormat;
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  ClearClipboard() -                                                            */
/*                                                                            */
/*--------------------------------------------------------------------------*/

/* This is called to clear the clipboard.  If the clipboard is not
 *        empty the user is asked if it should be cleared.
 */

BOOL NEAR PASCAL ClearClipboard(register HWND        hwnd)

{
  CHAR          szBuffer[SMALLBUFFERLEN];
  CHAR          szLocBuffer[BUFFERLEN];

  if (CountClipboardFormats() <= 0)
      return(TRUE);

  /* Get the confirmation from the user. */
  LoadString(hInst, IDS_CLEARTITLE, szBuffer, SMALLBUFFERLEN);
  LoadString(hInst, IDS_CONFIRMCLEAR, szLocBuffer, BUFFERLEN);
  if (MessageBox(hwnd, szLocBuffer, szBuffer, MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
    {
    if (!OpenClipboard(hwnd) ||
        !EmptyClipboard() ||
        !CloseClipboard() )
       {
       LoadString(hInst, IDS_ERROR, szBuffer, SMALLBUFFERLEN);
       LoadString(hInst, IDS_CLEAR, szLocBuffer, BUFFERLEN);
       MessageBox(hwnd, szLocBuffer, szBuffer, MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);
       }
    InvalidateRect(hwnd, NULL, TRUE);

    return(TRUE);
    }
  return(FALSE);
}



/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  GetClipboardName() -                                                    */
/*                                                                            */
/*--------------------------------------------------------------------------*/

void NEAR PASCAL GetClipboardName(register UINT fmt,LPTSTR szName,
                                        register INT iSize)

{
  LPTSTR   lprgch;
  HANDLE  hrgch;

  *szName = 0;

  /* Get global memory that everyone can get to */
  if ((hrgch = GlobalAlloc(GPTR, (LONG)(iSize + 1))) == NULL)
      return;

  if (!(lprgch = (LPTSTR)GlobalLock(hrgch)))
      goto ExitPoint;

  switch (fmt)
    {
      case CF_RIFF:
      case CF_WAVE:
      case CF_PENDATA:
      case CF_SYLK:
      case CF_DIF:
      case CF_TIFF:
      case CF_TEXT:
      case CF_BITMAP:
      case CF_METAFILEPICT:
      case CF_OEMTEXT:
      case CF_DIB:
      case CF_PALETTE:
      case CF_DSPTEXT:
      case CF_DSPBITMAP:
      case CF_DSPMETAFILEPICT:
      case CF_UNICODETEXT:
      case CF_ENHMETAFILE:
      case CF_DSPENHMETAFILE:
          LoadString(hInst, fmt, lprgch, iSize);
          break;

      case CF_OWNERDISPLAY:           /* Clipbrd owner app supplies name */
	  *lprgch = 0;
          SendOwnerMessage(WM_ASKCBFORMATNAME, iSize, (LONG)(LPTSTR)lprgch);

          if (!*lprgch)
              LoadString(hInst, fmt, lprgch, iSize);
          break;

      default:
          GetClipboardFormatName(fmt, lprgch, iSize);
          break;
    }

  lstrcpy(szName, lprgch);

  GlobalUnlock(hrgch);
ExitPoint:
  GlobalFree(hrgch);
}


/*--------------------------------------------------------------------------

  DrawFormat() -

  Purpose: Draw the data on the clipboard, using the given format.

  Parameters:
      hdc - hDC to draw into.
      prc - Pointer to a rectangle showing bounds to paint into
      cxScroll - Horizontal position we're scrolled to (0=left)
      cyScroll - Vertical   position we're scrolled to (0=top)
      BestFormat - Format to use when drawing.

--------------------------------------------------------------------------*/
void NEAR PASCAL DrawFormat(
register HDC hdc,
PRECT prc,
INT cxScroll,
INT cyScroll,
UINT BestFormat)
{
register HANDLE   h;
HFONT             hFont;
INT               fOK = TRUE;
UINT		  wFormat = 0;

if ((BestFormat == 0))
   {
   if (CountClipboardFormats() )
      {
      ShowString(hdc, IDS_CANTDISPLAY);
      }
   }
else if ((h = GetClipboardData(/* wFormat ? wFormat :*/  BestFormat)) != NULL)
   {
   switch (BestFormat)
      {
   case CF_DSPTEXT:
   case CF_TEXT:
      ShowText(hdc, prc, h, cyScroll, FALSE);
      break;

   case CF_UNICODETEXT:
      hFont = SelectObject(hdc, hfontUni);
      ShowText(hdc, prc, h, cyScroll, TRUE);
      SelectObject(hdc, hFont);
      break;

   case CF_OEMTEXT:
      hFont = SelectObject(hdc, hfontOem);
      ShowText(hdc, prc, h, cyScroll, FALSE);
      SelectObject(hdc, hFont);
      break;

   case CF_DSPBITMAP:
   case CF_BITMAP:
      fOK = FShowBitmap(hdc, prc, h, cxScroll, cyScroll);
      break;

   case CF_DIB:
      fOK = FShowDIBitmap(hdc, prc, h, cxScroll, cyScroll);
      break;

   case CF_PALETTE:
      fOK = FShowPalette(hdc, prc, h, cxScroll, cyScroll);
      break;

   case CF_WAVE:
   case CF_RIFF:
   case CF_PENDATA:
   case CF_DIF:
   case CF_SYLK:
   case CF_TIFF:
      ShowString(hdc, IDS_BINARY);
      break;

   case CF_ENHMETAFILE:
   case CF_DSPENHMETAFILE:
      fOK = FShowEnhMetaFile(hdc, h, prc);
      break;

   case CF_DSPMETAFILEPICT:
   case CF_METAFILEPICT:
      fOK = FShowMetaFilePict(hdc, prc, h, cxScroll, cyScroll);
      break;

   /* If "Auto" is chosen and only data in unrecognised formats is
    * available, then display "Can't display data in this format".
   */
   default:
      ShowString(hdc, IDS_CANTDISPLAY);
      break;
      }
   }
else if (CountClipboardFormats()) // There's data, but we can't Get it..
   {
   ShowString(hdc, IDS_ERROR);
   }

/* If we are unable to display the data, display "<Error>" */
if (!fOK)
   {
   ShowString(hdc, IDS_NOTRENDERED);
   }
}


/*--------------------------------------------------------------------------

  DrawStuff() -

 Paint portion of current clipboard contents given by PAINT struct

 NOTE: If the paintstruct rectangle includes any part of the header, the
       whole header is redrawn.

 Parameters:
    hwnd - Window to draw in.
    pps  - Pointer to PAINTSTRUCT to use.

 Returns:
    Nothing.
----------------------------------------------------------------------------*/
void NEAR PASCAL DrawStuff(
HWND                  hwnd,
register PAINTSTRUCT *pps)
{
register HDC  hdc;
RECT          rcPaint;
RECT          rcClient;
UINT          BestFormat;

hdc = pps->hdc;

if (pps->fErase)
   FillRect(hdc, (LPRECT)&pps->rcPaint, hbrBackground);

GetClientRect(hwnd, (LPRECT)&rcClient);

BestFormat = GetBestFormat(CurSelFormat);

fOwnerDisplay = (BestFormat == CF_OWNERDISPLAY);

/* If the display format has changed, Set rcWindow,
 * the display area for clip info.
 */

if (fDisplayFormatChanged)
  {
    CopyRect((LPRECT)&rcWindow, (LPRECT)&rcClient);

    /* We have changed the size of the clipboard. Tell the owner,
     * if fOwnerDisplay is active.
     */

    if (fOwnerDisplay)
        SendOwnerSizeMessage(hwnd, rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom);
    else
        /* Give the window a small margin, for looks */
        InflateRect(&rcWindow, -((int)cxMargin), -((int)cyMargin));

    fDisplayFormatChanged = FALSE;
  }

if (fOwnerDisplay)
  {
    /* Clipboard Owner handles display */
    HANDLE hps;

    hps = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                      (LONG)sizeof(PAINTSTRUCT));

    if (hps != NULL)
      {
        LPPAINTSTRUCT lppsT;

        if ((lppsT = (LPPAINTSTRUCT)GlobalLock(hps)) != NULL)
           {
           _fmemcpy(lppsT,pps,sizeof(PAINTSTRUCT));
           IntersectRect(&lppsT->rcPaint, &pps->rcPaint, &rcWindow);
           GlobalUnlock(hps);
           SendOwnerMessage(WM_PAINTCLIPBOARD, (WPARAM)hwnd, (LONG)hps);
           GlobalFree(hps);
           }
      }

  }
else
   {
   /* We handle display */
   /* Redraw the portion of the paint rectangle that is in the clipbrd rect */
   IntersectRect(&rcPaint, &pps->rcPaint, &rcWindow);

   rcPaint.left = rcWindow.left;   /* Always draw from left edge of window */

   if ((rcPaint.bottom > rcPaint.top) && (rcPaint.right > rcPaint.left))
      {
      DrawFormat(hdc, &rcPaint,
                  /* rcPaint.left always == rcWindow.left | A-MGATES    */
                  /* (INT)(cxScrollNow + rcPaint.left - rcWindow.left), */
                  cxScrollNow,
                  (INT)(cyScrollNow + rcPaint.top - rcWindow.top),
                  BestFormat);
      }
   }
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  UpdateCBMenu() -                                                            */
/*                                                                            */
/* This routine is called once during initialisation and everytime
 * the contents of the clipboard change. This updates the entries
 * in the "Display" popup menu and the "grey" and "checked" status
 * based on the data formats available in the clipboard.
 */

void UpdateCBMenu(
HWND hwnd)
{
  register UINT fmt;
  WORD          cFmt;
  WORD          cCBCount;   /* Number of data items in CB */
  register WORD wFlags;     /* Used to store the status flags for menu items */
  INT           iIndex;
  INT           nPopupCount;
  BOOL          bAutoSelect;
  TCHAR          szName[40];

  /* Find out the number of data items present in clipboard. */
  if (!(cCBCount = (WORD)CountClipboardFormats()))
    {
      /* The Clipboard is empty;  So, disable both menu items */
      EnableMenuItem(hMainMenu, 2, MF_BYPOSITION | MF_GRAYED);
      EnableMenuItem(hMainMenu, CBM_CLEAR, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMainMenu, CBM_SAVEAS, MF_BYCOMMAND | MF_GRAYED);
      goto ExitPoint;
    }

  /* Now clipboard contains at least one item...
   * Find out the number entries in the popup menu at present.
   */
  if (!hDispMenu)
      /* Get the handle to the Display popup menu */
      hDispMenu = GetSubMenu(GetMenu(hwnd), 2);


  nPopupCount = GetMenuItemCount(hDispMenu);

  if (nPopupCount > 2)
    {
      /* Delete all the entries in the popup menu below menu break. */
      for (iIndex = 2; iIndex < nPopupCount; iIndex++)
        {
          /* NOTE: The second parameter must always be 2! (because we use
           * MF_BYPOSITION, when 2 is deleted, 3 becomes 2!).
           */
          DeleteMenu(hDispMenu, 2, MF_BYPOSITION);
        }
    }

  bAutoSelect = TRUE;

  /* EnumClipboard() requires an OpenClipboard(). */
  if (!OpenClipboard(hwnd))
      goto ExitPoint;

  for (fmt=0, cFmt=1; cFmt <= cCBCount; cFmt++)
    {
      wFlags = 0;
      fmt = EnumClipboardFormats(fmt);

      GetClipboardName(fmt, (LPTSTR)szName, sizeof(szName));

      switch (fmt)
        {
          case CF_TEXT:           /* can display all of these */
          case CF_BITMAP:
          case CF_METAFILEPICT:
          case CF_OEMTEXT:
          case CF_DIB:
          case CF_DSPTEXT:
          case CF_DSPBITMAP:
          case CF_DSPMETAFILEPICT:
          case CF_OWNERDISPLAY:
          case CF_PALETTE:
          case CF_UNICODETEXT:
          case CF_ENHMETAFILE:
          case CF_DSPENHMETAFILE:
              break;

          default:                /* all the rest... no */
              wFlags |= MF_GRAYED;
              break;
        }

      /* We have the name of the format in szName. */
      wFlags |= MF_STRING;

      /* Check if the current format is the one selected by the user */
      if (CurSelFormat == fmt)
        {
          bAutoSelect = FALSE;
          wFlags |= MF_CHECKED;
        }

      AppendMenu(hDispMenu, wFlags, fmt, (LPTSTR)szName);
    }

  CloseClipboard();

  if (bAutoSelect)
    {
      CurSelFormat = CBM_AUTO;
      CheckMenuItem(hDispMenu, CBM_AUTO, MF_BYCOMMAND | MF_CHECKED);
    }

  /* Enable the menu items in the top level menu. */
  EnableMenuItem(hMainMenu, 2, MF_BYPOSITION | MF_ENABLED);
  EnableMenuItem(hMainMenu, CBM_CLEAR, MF_BYCOMMAND | MF_ENABLED);
  EnableMenuItem(hMainMenu, CBM_SAVEAS, MF_BYCOMMAND | MF_ENABLED);

ExitPoint:
  DrawMenuBar(hwnd);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SaveOwnerScrollInfo() -                                                 */
/*                                                                          */
/* When the user switched the clipboard display from owner disp to
 *  a non-owner display, all the information about the scroll bar
 *  positions are to be saved. This routine does that.
 *  This is required because, when the user returns back to owner
 *  display, the scroll bar positions are to be restored.
 */

void SaveOwnerScrollInfo(
register HWND hwnd)
{
GetScrollRange(hwnd, SB_VERT, (LPINT) & OwnVerMin, (LPINT) & OwnVerMax);
GetScrollRange(hwnd, SB_HORZ, (LPINT) & OwnHorMin, (LPINT) & OwnHorMax);
OwnVerPos = GetScrollPos(hwnd, SB_VERT);
OwnHorPos = GetScrollPos(hwnd, SB_HORZ);
}


/*--------------------------------------------------------------------------*/
/*                                                                            */
/*  RestoreOwnerScrollInfo() -                                                    */
/*                                                                            */
/*--------------------------------------------------------------------------*/

/*  When the user sitches back to owner-display, the scroll bar
 *  positions are restored by this routine.
 */

void RestoreOwnerScrollInfo(
register HWND hwnd)
{
SetScrollRange(hwnd, SB_VERT, OwnVerMin, OwnVerMax, FALSE);
SetScrollRange(hwnd, SB_HORZ, OwnHorMin, OwnHorMax, FALSE);

SetScrollPos(hwnd, SB_VERT, OwnVerPos, TRUE);
SetScrollPos(hwnd, SB_HORZ, OwnHorPos, TRUE);
}
