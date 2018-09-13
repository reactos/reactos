/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   PrintImg.c                                  *
*   system: Publisher's Paintbrush for MS-Windows       *
*   descr:  prints image from image buffer              *
*   date:   06/27/87    MSZ                             *
*                                                       *
********************************************************/

// #define DEBUG
#define STATIC static

#include <windows.h>
#include <port1632.h>

//#define NOEXTERN
#include "pbrush.h"
//#include "fixedpt.h"

extern TCHAR pgmTitle[];
extern TCHAR fileName[];
extern TCHAR noFile[];
extern TCHAR winIniAppName[];
extern HPALETTE hPalette;
extern HWND pbrushWnd[];
extern int imagePlanes, imagePixels;

#define SETDIBSCALING  32
#define SET_BOUNDS 4109

#define BUFSIZE 140
#define MEMFACTORNUM 1
#define MEMFACTORDEN 6
#define MEMMAX 63000

#define NOSTART 10
#define NOMESG  20
#define STARTOK 30
#define INITOK  40

#define BiToByAl(bi,al) (((bi)+(al)*8-1)/((al)*8)*(al))
#define PollPrint() (*lpfnAbortPrt)((HDC)NULL, 0)

      /*------------------------------------------------*/
      /* nSizeFactor is available for setting the size  */
      /* of the printed image.  It is the percent of    */
      /* scaling and can be set from another module.    */
      /*------------------------------------------------*/
int nSizeNum = 100, nSizeDen = 100;

HWND hDlgPrint = NULL;
BOOL bUserAbort;

extern int hResPrt, vResPrt;      /* HORZRES, VERTRES */
extern int xPelsPrt, yPelsPrt;    /* LOGPIXELSX, LOGPIXELSY */
extern TCHAR szHeader[], szFooter[]; /* Header/Footer strings */
extern BOOL fStretch;

STATIC BOOL bError;
STATIC BOOL fDIBScaling;
STATIC WNDPROC lpfnAbortDlg;
STATIC ABORTPROC lpfnAbortPrt;

STATIC long xLogPrint, yLogPrint, xLogImage, yLogImage; /* stretching factors */
STATIC POINT sFactor;
STATIC RECT rImage;
STATIC unsigned int nBitsPix, nScanLines;
STATIC unsigned long dwBitmapSize;
STATIC HDC hMemDC;
STATIC HANDLE hDIBInfo;
STATIC HPALETTE hPrtPalette, hMemPalette;

STATIC int CharPrintHeight;
STATIC unsigned long lIncArea, lTotArea;

STATIC int nPage, nPages;
STATIC int xImage, yImage;

STATIC LPTSTR szDay[] = { TEXT("Sun"), TEXT("Mon"), TEXT("Tue"), TEXT("Wed"), TEXT("Thu"), TEXT("Fri"), TEXT("Sat") };
STATIC LPTSTR szMonth[] = { TEXT(""), TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"), TEXT("May"), TEXT("Jun"),
                            TEXT("Jul"), TEXT("Aug"), TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec") };

typedef struct tagDIBSCALE {
   short ScaleMode;
   short dx;
   short dy;
} DIBSCALE;

DWORD TotalMemoryAvailable(void);

      /*------------------------------------------------*/
      /* Try to set the number of copies and the print  */
      /* quality; return the number of copies           */
      /*------------------------------------------------*/
STATIC int NEAR PASCAL SetPrintOptions(HDC hPrtDC, BOOL fDraft, int copies)
{
   LONG lHolder, lParam;

      /*------------------------------------------------*/
      /* Tell the driver how many copies we want        */
      /*------------------------------------------------*/
   lHolder = SETCOPYCOUNT;
   if (Escape(hPrtDC, QUERYESCSUPPORT, sizeof(lHolder), (LPSTR) &lHolder, NULL)) {
      Escape(hPrtDC, SETCOPYCOUNT, sizeof(copies), (LPSTR) &copies, (LPVOID) &copies);
   } else {
      copies = 1;
   }

      /*------------------------------------------------*/
      /* Set the print quality if the driver allows     */
      /*------------------------------------------------*/
   lHolder = DRAFTMODE;
   lParam = fDraft ? 1 : 0;
   if(Escape(hPrtDC, QUERYESCSUPPORT, sizeof(lHolder), (LPSTR) &lHolder, NULL))
      Escape(hPrtDC, DRAFTMODE, sizeof(lParam), (LPSTR) &lParam, NULL);

      /*------------------------------------------------*/
      /* Return the # of copies that will actually be   */
      /* printed                                        */
      /*------------------------------------------------*/
   return copies;
}

      /*------------------------------------------------*/
      /* Given a file name which may or may not include */
      /* a path specification, return a pointer to the  */
      /* file name without the path                     */
      /*------------------------------------------------*/
LPTSTR PFileInPath(LPTSTR sz)
{
   LPTSTR pch;

      /*------------------------------------------------*/
      /* Strip path/drive specification from file name  */
      /*------------------------------------------------*/
   pch = CharPrev(sz, sz + lstrlen(sz));
   while (pch > sz) {
      pch = CharPrev(sz,pch);
      if (*pch == TEXT('\\') || *pch == TEXT(':')) {
         pch = CharNext(pch);
         break;
      }
   }
   return(pch);
}

      /*------------------------------------------------*/
      /* Transform a rect in the image to a rect on the */
      /* printer.                                       */
      /*------------------------------------------------*/
STATIC void NEAR PASCAL ImageToPrint(NPRECT nprDest, NPRECT nprImage, NPRECT nprPrint)
{
   nprDest->left   = (int)(nprPrint->left +
         (nprDest->left   - nprImage->left)* xLogPrint/xLogImage);
   nprDest->top    = (int)(nprPrint->top  +
         (nprDest->top    - nprImage->top) * yLogPrint/yLogImage);
   nprDest->right  = (int)(nprPrint->left +
         (nprDest->right  - nprImage->left)* xLogPrint/xLogImage);
   nprDest->bottom = (int)(nprPrint->top  +
         (nprDest->bottom - nprImage->top) * yLogPrint/yLogImage);
}

      /*------------------------------------------------*/
      /* Transform a rect on the printer to a rect in   */
      /* the image.                                     */
      /*------------------------------------------------*/
STATIC void NEAR PASCAL PrintToImage(NPRECT nprDest, NPRECT nprImage, NPRECT nprPrint)
{
   nprDest->left   = (int)(nprImage->left +
         (nprDest->left   - nprPrint->left)*xLogImage/xLogPrint);
   nprDest->top    = (int)(nprImage->top  +
         (nprDest->top    - nprPrint->top )*yLogImage/yLogPrint);
   nprDest->right  = (int)(nprImage->left +
         ((nprDest->right  - nprPrint->left)*xLogImage+xLogPrint-1)/xLogPrint);
   nprDest->bottom = (int)(nprImage->top  +
         ((nprDest->bottom - nprPrint->top )*yLogImage+yLogPrint-1)/yLogPrint);
}

      /*------------------------------------------------*/
      /* Initialize variables for printing              */
      /*------------------------------------------------*/
STATIC BOOL NEAR PASCAL InitPrinting(HDC hPrintDC, NPRECT nprImage,
      NPRECT nprPrint, LPTSTR npFileName)
{
   HDC hImageDC;
   RECT rPrint;
   TEXTMETRIC Metrics;
   TCHAR msg[BUFSIZE], buf[BUFSIZE];
   LPBITMAPINFO lpDIBInfo;
   WORD nPaletteSize;
   HANDLE hLogPalette;
   LPLOGPALETTE lpLogPalette;
   int holder;
   int retVal;
   DOCINFO DocInfo;

      /*------------------------------------------------*/
      /* Set memory to NULL and print flags             */
      /*------------------------------------------------*/
   hMemDC = NULL;
   hPrtPalette = hMemPalette = NULL;
   hDIBInfo = NULL;

   bError = bUserAbort = FALSE;

      /*------------------------------------------------*/
      /* Get a DC for the image and a memory DC         */
      /*------------------------------------------------*/
   if(!(hImageDC = GetDisplayDC(pbrushWnd[PARENTid])))
      return(NOSTART);
   if(!(hMemDC = CreateCompatibleDC(hImageDC))) {
      ReleaseDC(pbrushWnd[PARENTid], hImageDC);
      return(NOSTART);
   }


      /*------------------------------------------------*/
      /* These are the scaling factors used in          */
      /* PrintToImage and ImageToPrint                  */
      /* nSizeNum and nSizeDen may be used to scale the */
      /* image on the printer                           */
      /*------------------------------------------------*/
   if(Escape(hPrintDC, GETSCALINGFACTOR, 0, NULL, (LPVOID)&sFactor) <= 0)
      sFactor.x = sFactor.y = 0;
   sFactor.x = 1 << sFactor.x;
   sFactor.y = 1 << sFactor.y;
   if (fStretch) {
      xLogPrint = (long)nSizeNum * xPelsPrt;
      yLogPrint = (long)nSizeNum * yPelsPrt;
      xLogImage = (long)nSizeDen * GetDeviceCaps(hImageDC, LOGPIXELSX);
      yLogImage = (long)nSizeDen * GetDeviceCaps(hImageDC, LOGPIXELSY);
   } else {
      xLogPrint = nSizeNum * sFactor.x;
      yLogPrint = nSizeNum * sFactor.y;
      xLogImage = nSizeDen;
      yLogImage = nSizeDen;
   }
   ReleaseDC(pbrushWnd[PARENTid], hImageDC);

      /*------------------------------------------------*/
      /* Compute the rect of the image that will        */
      /* actually be printed, the printing rectangle,   */
      /* and set the variables for calculating the      */
      /* percentage that has been printed               */
      /*------------------------------------------------*/
   rImage = *nprPrint;
   PrintToImage(&rImage, nprImage, nprPrint);
   IntersectRect(&rImage, &rImage, nprImage);

   xImage = rImage.right - rImage.left;
   yImage = rImage.bottom - rImage.top;
   nPages  = (nprImage->right - nprImage->left + xImage - 1)/xImage;
   nPages *= (nprImage->bottom - nprImage->top + yImage - 1)/yImage;

   rPrint = *nprImage;
   ImageToPrint(&rPrint, nprImage, nprPrint);

   lIncArea = 0;
   lTotArea = (long)(rPrint.right - rPrint.left)*(rPrint.bottom - rPrint.top);

   IntersectRect(&rPrint, &rPrint, nprPrint);

      /*------------------------------------------------*/
      /* Make string to send to spooler, set the abort  */
      /* procedure, set the bounding rectangle of the   */
      /* printed image, and start printing the document */
      /*------------------------------------------------*/
   LoadString(hInst, IDSPrintSpool, buf, CharSizeOf(buf));
   wsprintf(msg, buf, winIniAppName, npFileName);
   if(SetAbortProc(hPrintDC, lpfnAbortPrt) == SP_ERROR)
      return(NOSTART);

   holder = SET_BOUNDS;
   if(Escape(hPrintDC, QUERYESCSUPPORT, sizeof(holder), (LPSTR) &holder, NULL))
      Escape(hPrintDC, SET_BOUNDS, sizeof(rPrint), (LPSTR)&rPrint, NULL);

   DocInfo.cbSize = sizeof (DOCINFO);
   DocInfo.lpszDocName = msg;
   DocInfo.lpszOutput = NULL;
   DocInfo.lpszDatatype = NULL;
   DocInfo.fwType = 0;

   if ((retVal = StartDoc(hPrintDC, &DocInfo)) <= 0)
   {
      if(!bUserAbort)
         return(NOSTART);
      else
         return(NOMESG);
   }

      /*------------------------------------------------*/
      /* Get printer font height                        */
      /*------------------------------------------------*/
   if (GetTextMetrics(hPrintDC, (LPTEXTMETRIC)(&Metrics)))
      CharPrintHeight = Metrics.tmHeight + Metrics.tmExternalLeading;
   else
      CharPrintHeight = 12;

      /*------------------------------------------------*/
      /* Create two copies of the                       */
      /* palette and select them into hMemDC and        */
      /* hPrintDC                                       */
      /*------------------------------------------------*/
   if (hPalette) {
      if(!GetObject(hPalette, sizeof(nPaletteSize), (LPVOID)&nPaletteSize)
            || !(hLogPalette = GlobalAlloc(GMEM_MOVEABLE,
            (long)(sizeof(LOGPALETTE)+(nPaletteSize-1)*sizeof(PALETTEENTRY)))))
         return(STARTOK);

      if(!(lpLogPalette = (LPLOGPALETTE)GlobalLock(hLogPalette)))
         goto NoLock;
      lpLogPalette->palVersion    = 0x0300;
      lpLogPalette->palNumEntries = (WORD)nPaletteSize;
      if(!GetPaletteEntries(hPalette, 0, nPaletteSize,
            lpLogPalette->palPalEntry))
         goto NoEntries;

      hPrtPalette = CreatePalette(lpLogPalette);
      hMemPalette = CreatePalette(lpLogPalette);
NoEntries:
      GlobalUnlock(hLogPalette);
NoLock:
      GlobalFree(hLogPalette);

      if(!hPrtPalette || !hMemPalette
            || !SelectPalette(hPrintDC, hPrtPalette, FALSE)
            || !SelectPalette(hMemDC, hMemPalette, FALSE))
         return(STARTOK);
   }

   nBitsPix = fDIBScaling ? imagePixels * (imagePlanes==3 ? 4 : imagePlanes):8;

      /*------------------------------------------------*/
      /* Set up memory for the DIB                      */
      /*------------------------------------------------*/
   if(!(hDIBInfo = GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFO) +
         (long)((1<<nBitsPix) - 1) * sizeof(RGBQUAD)))
         || !(lpDIBInfo = (LPBITMAPINFO)GlobalLock(hDIBInfo)))
      return(STARTOK);
   lpDIBInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
   lpDIBInfo->bmiHeader.biWidth         = 1;
   lpDIBInfo->bmiHeader.biHeight        = 1;
   lpDIBInfo->bmiHeader.biPlanes        = 1;
   lpDIBInfo->bmiHeader.biBitCount      = (WORD)nBitsPix;
   lpDIBInfo->bmiHeader.biCompression   = BI_RGB;
   lpDIBInfo->bmiHeader.biSizeImage     = 0;
   lpDIBInfo->bmiHeader.biXPelsPerMeter = 0;
   lpDIBInfo->bmiHeader.biYPelsPerMeter = 0;
   lpDIBInfo->bmiHeader.biClrUsed       = 0;
   lpDIBInfo->bmiHeader.biClrImportant  = 0;
   GlobalUnlock(hDIBInfo);

   LoadString(hInst, IDSPrintPercent, buf, CharSizeOf(buf));
   wsprintf(msg, buf, 1, nPages, 0);
   SetDlgItemText(hDlgPrint, IDPERCENT, msg);

   return(INITOK);
}

      /*------------------------------------------------*/
      /* Main function for printing a band on the       */
      /* printer                                        */
      /*------------------------------------------------*/
STATIC void NEAR PASCAL PrintBand (HDC hPrintDC,
      NPRECT nprBandPrint, NPRECT nprImage, NPRECT nprPrint)
{
   int top;
   int xIntImage, yIntImage, xIntPrint, yIntPrint;
   RECT rBandImage, rBandPrint;
   RECT rIntPrint, rIntImage;
   HANDLE hDIBitmap;
   LPBYTE lpDIBitmap;
   LPBITMAPINFO lpDIBInfo;
   HBITMAP hBitmap, hOldBitmap;
   TCHAR szPercent[BUFSIZE], buf[BUFSIZE];
   unsigned int yMax;
   unsigned int xBitmap, yBitmap;
   int LowMem = -1;

   HBRUSH hBrush[256], hOldBrush;
   WORD crColor, nextColor;
   int i, j, dstI, endI, dstJ, dstWid, dstHgt;
   BYTE far *hpDIBitmap;

      /*------------------------------------------------*/
      /* Find the rect of the band on the image and     */
      /* return if nothing to print in this band        */
      /*------------------------------------------------*/
   rBandImage = *nprBandPrint;
   PrintToImage(&rBandImage, nprImage, nprPrint);

   if(!IntersectRect(&rBandImage, &rBandImage, nprImage))
      return;

      /*------------------------------------------------*/
      /* Find the inverse rect of the band on the image */
      /* and check if the current bitmap is large enough*/
      /* for our purposes;  xBitmap and yBitmap are the */
      /* dimensions of the bitmap, and are only allowed */
      /* to increase, except the total memory has a cap */
      /* of dwBitmapSize                                */
      /*------------------------------------------------*/
   rBandPrint = rBandImage;
   ImageToPrint(&rBandPrint, nprImage, nprPrint);
   if(IsRectEmpty(&rBandPrint))
      return;

/* For now, we are always going to create a new bitmap
   that is exactly the right width */
   xBitmap = rBandImage.right - rBandImage.left;

   bError = TRUE; /* this will be set to FALSE if we print OK */
TryAgain:
   ++LowMem;

      /*------------------------------------------------*/
      /* Use MEMFACTOR*available memory or MEMMAX bytes */
      /* for a bitmap that is compatible with the image */
      /*------------------------------------------------*/
   dwBitmapSize = ((DWORD)TotalMemoryAvailable() * MEMFACTORNUM / MEMFACTORDEN);
   if(dwBitmapSize > MEMMAX)
      dwBitmapSize = MEMMAX;

   yMax = (int)((dwBitmapSize*8)/(xBitmap*nBitsPix));
   yBitmap = rBandImage.bottom - rBandImage.top;
   if(yBitmap > yMax)
      yBitmap = yMax;
   yBitmap >>= LowMem;

      /*------------------------------------------------*/
      /* Create a new bitmap, a new DIB, and            */
      /* recompute nScanLines (the number of image scan */
      /* lines we may print at a time)                  */
      /*------------------------------------------------*/
   nScanLines = yBitmap;
   if(!nScanLines)
      return;

   if(!(hBitmap = CreateBitmap(xBitmap, yBitmap,
         (UINT)imagePlanes, (UINT)imagePixels, NULL))) {
      DB_OUTF((acDbgBfr, TEXT("LowMem = %d; CreateBitmap\n\r"), LowMem));
      goto Error1;
   }

   if(!(hOldBitmap = SelectObject(hMemDC, hBitmap))) {
      DB_OUTF((acDbgBfr, TEXT("LowMem = %d; SelectObject1\n\r"), LowMem));
      goto Error2;
   }

   if(!(hDIBitmap = GlobalAlloc(GMEM_MOVEABLE,
         yBitmap * BiToByAl((long)nBitsPix * xBitmap, 4)))) {
      DB_OUTF((acDbgBfr, TEXT("LowMem = %d; GlobalAlloc\n\r"), LowMem));
      goto Error3;
   }


      /*------------------------------------------------*/
      /* Repeat with successive hBitmap sized memory    */
      /* bands; we use PollPrint to get messages        */
      /*------------------------------------------------*/
   for(top = rBandImage.top;
         top < rBandImage.bottom && !bUserAbort;
         top += nScanLines) {
      bError = TRUE;

      PollPrint();

      /*------------------------------------------------*/
      /* Start with an hBitmap sized rect, then         */
      /* intersect with the printer band and the image  */
      /* and continue if nothing to print               */
      /*------------------------------------------------*/
      rIntImage.left   = nprImage->left;
      rIntImage.top    = top;
      rIntImage.right  = nprImage->right;
      rIntImage.bottom = top + nScanLines;

      IntersectRect(&rIntImage, &rIntImage, nprImage);
      if(!IntersectRect(&rIntImage, &rIntImage, &rBandImage)) {
         continue ;
      }

      PollPrint();

      /*------------------------------------------------*/
      /* Transform to a printer rect, get the dimensions*/
      /* of the image and printer rects, and page into  */
      /* memory                                         */
      /*------------------------------------------------*/
      rIntPrint = rIntImage;
      ImageToPrint(&rIntPrint, nprImage, nprPrint);
      if(IsRectEmpty(&rIntPrint))
         continue;

      xIntPrint = rIntPrint.right  - rIntPrint.left;
      yIntPrint = rIntPrint.bottom - rIntPrint.top;
      xIntImage = rIntImage.right  - rIntImage.left;
      yIntImage = rIntImage.bottom - rIntImage.top;

      if(hPalette)
         RealizePalette(hMemDC);

      if(!(SelectObject(hMemDC, hBitmap))) {
         DB_OUTF((acDbgBfr, TEXT("LowMem = %d; SelectObject2\n\r"), LowMem));
         goto Error4;
      }
      if(!(BitBlt(hMemDC, 0, 0, xIntImage, yIntImage,
            hdcWork, rIntImage.left, rIntImage.top, SRCCOPY))) {
         DB_OUTF((acDbgBfr, TEXT("LowMem = %d; BitBlt\n\r"), LowMem));
         goto Error4;
      }

      PollPrint();

      SelectObject(hMemDC, hOldBitmap);

      /*------------------------------------------------*/
      /* Lock the DIB memory for use                    */
      /*------------------------------------------------*/
      if(!(lpDIBitmap = GlobalLock(hDIBitmap)))
         goto Error4;
      if(!(lpDIBInfo = (LPBITMAPINFO)GlobalLock(hDIBInfo)))
         goto Error5;

      /*------------------------------------------------*/
      /* Set the size of the DIB to xBitmap,yIntImage   */
      /*------------------------------------------------*/
      lpDIBInfo->bmiHeader.biWidth  = xBitmap;
      lpDIBInfo->bmiHeader.biHeight = yIntImage;

      /*------------------------------------------------*/
      /* Transform a bitmap to a DIB                    */
      /*------------------------------------------------*/
      if(!(GetDIBits(hMemDC, hBitmap, 0, yIntImage,
            lpDIBitmap, lpDIBInfo, DIB_RGB_COLORS)))
         goto Error6;

      PollPrint();

      /*------------------------------------------------*/
      /* Send the portion of the image to the printer   */
      /*------------------------------------------------*/
      if(hPalette)
         RealizePalette(hPrintDC);

      if(fDIBScaling) {
         if(!StretchDIBits(hPrintDC, rIntPrint.left, rIntPrint.top,
               xIntPrint, yIntPrint, 0, 0, xIntImage, yIntImage,
               lpDIBitmap, lpDIBInfo, DIB_RGB_COLORS, SRCCOPY))
            goto Error6;

         bError = FALSE;
      } else {
         if(!(hOldBrush = SelectObject(hPrintDC, GetStockObject(WHITE_BRUSH))))
            goto Error6;
         SelectObject(hPrintDC, hOldBrush);

         for(j=0; j<256; ++j)
            hBrush[j] = NULL;

         for(j=0, dstJ=0; j<yIntImage && !bUserAbort; ++j, dstJ+=dstHgt) {
            PollPrint();

            if(!(dstHgt = (WORD)(((DWORD)(j+1)*yIntPrint + yIntImage/2)
                  /yIntImage - dstJ)))
               continue;

            hpDIBitmap = lpDIBitmap + BiToByAl(xBitmap*8, 4)*(yIntImage-j-1);
            nextColor = (WORD)*hpDIBitmap;
            for(i=0, dstI=0; i<xIntImage; i=endI, dstI+=dstWid) {
               crColor = nextColor;

               if(!hBrush[crColor]) {
                  nextColor = 0;
                  while(!(hBrush[crColor] = CreateSolidBrush(
                        RGB(lpDIBInfo->bmiColors[crColor].rgbRed,
                        lpDIBInfo->bmiColors[crColor].rgbGreen,
                        lpDIBInfo->bmiColors[crColor].rgbBlue)))) {
                     for( ; nextColor<256 && !hBrush[nextColor];
                           ++nextColor) ;
                     if(nextColor == 256)
                        goto Error7;
                     DeleteObject(hBrush[nextColor]);
                     hBrush[nextColor] = NULL;
                  }
               }

               for(endI=i+1; endI<xIntImage; ++endI) {
                  ++hpDIBitmap;
                  if((nextColor = (WORD)*hpDIBitmap) != crColor)
                     break;
               }

               if(!(dstWid = (WORD)(((DWORD)endI*xIntPrint + xIntImage/2)
                     /xIntImage - dstI)))
                  continue;

               if(!SelectObject(hPrintDC, hBrush[crColor]))
                  goto Error7;
               PatBlt(hPrintDC, dstI+rIntPrint.left, dstJ+rIntPrint.top,
                     dstWid, dstHgt, PATCOPY);
               SelectObject(hPrintDC, hOldBrush);
            }
         }

         bError = FALSE;

Error7:
         for(nextColor=0; nextColor<256; ++nextColor)
            if(hBrush[nextColor])
               DeleteObject(hBrush[nextColor]);
      }

      /*------------------------------------------------*/
      /* Unlock DIB memory until we need it again       */
      /*------------------------------------------------*/
Error6:
      GlobalUnlock(hDIBInfo);
Error5:
      GlobalUnlock(hDIBitmap);

      if(bError)
         goto Error4;

      PollPrint();

      /*------------------------------------------------*/
      /* Update the percentage display                  */
      /*------------------------------------------------*/
      if(hDlgPrint) {
         IntersectRect(&rIntPrint, &rIntPrint, nprBandPrint);
         lIncArea += (long)(rIntPrint.right - rIntPrint.left) *
               (rIntPrint.bottom - rIntPrint.top);
         if(lIncArea > lTotArea)
            lIncArea = lTotArea;
         LoadString(hInst, IDSPrintPercent, buf, CharSizeOf(buf));
         wsprintf(szPercent, buf, nPage, nPages, (int)(100*lIncArea/lTotArea));
         SetDlgItemText(hDlgPrint, IDPERCENT, szPercent);
      }

      PollPrint();
   }

   bError = FALSE;
Error4:
   GlobalFree(hDIBitmap);
Error3:
   SelectObject(hMemDC, hOldBitmap);
Error2:
   DeleteObject(hBitmap);
Error1:
   if(bError)
      goto TryAgain;
}

      /*------------------------------------------------*/
      /* Delete global memory objects that are still    */
      /* around                                         */
      /*------------------------------------------------*/
STATIC void NEAR PASCAL TermPrinting(HDC hPrintDC, int DelTo)
{
      /*------------------------------------------------*/
      /* If there were no printing errors               */
      /*------------------------------------------------*/
   if (DelTo >= STARTOK)
   {
      if (DelTo == INITOK && !bUserAbort && !bError)
         EndDoc(hPrintDC);
      else
         AbortDoc(hPrintDC);
   }

      /*------------------------------------------------*/
      /* Free up memory that is still being used        */
      /*------------------------------------------------*/
   if(hMemDC      ) DeleteDC(hMemDC);
   if(hPrtPalette ) DeleteObject(hPrtPalette);
   if(hMemPalette ) DeleteObject(hMemPalette);
   if(hDIBInfo    ) GlobalFree(hDIBInfo);
}

STATIC void NEAR PASCAL TranslateString(TCHAR chBuff[3][80], int nIndex[3],
      TCHAR * src, TCHAR * szFilename)
{
   TCHAR        buf[80];
   TCHAR        letters[15];
   TCHAR       *dst = buf, *save_src = src;
   int          temp;
   short        nAlign = 1;
   SYSTEMTIME   systime;

   nIndex[0] = nIndex[1] = nIndex[2] = 0;

   LoadString (hInst, IDSLetters, letters, CharSizeOf(letters));

      /*------------------------------------------------*/
      /* Get the date/time in case we need it           */
      /*------------------------------------------------*/
   GetSystemTime (&systime);

   while (*src) {
      /*------------------------------------------------*/
      /* Copy all "ordinary" characters to their aligned*/
      /* strings                                        */
      /*------------------------------------------------*/
      while(*src && *src != TEXT('&')) {
         chBuff[nAlign][nIndex[nAlign]] = *src++;
         nIndex[nAlign] += 1;
      }

      /*------------------------------------------------*/
      /* If we have come to the escape character        */
      /*------------------------------------------------*/
      if (*src == TEXT('&')) {
         src++;

      /*------------------------------------------------*/
      /* Copy the file name over                        */
      /*------------------------------------------------*/
         if (*src == letters[0] || *src == letters[1])
         {
            lstrcpy(chBuff[nAlign]+nIndex[nAlign], szFilename);
            nIndex[nAlign] += lstrlen(szFilename);
         }

      /*------------------------------------------------*/
      /* Print the page number (plus a constant)        */
      /*------------------------------------------------*/
         else if (*src == letters[2] || *src == letters[3])
         {
            temp = 0;

      /*------------------------------------------------*/
      /* If a constant, convert to int before printing  */
      /*------------------------------------------------*/
            if (*++src == TEXT('+')) {
               src++;
               while (_istdigit(*src)) {
                  temp = (10*temp) + *src - TEXT('0');
                  src++;
               }
            }

            wsprintf (buf, TEXT("%d"), nPage + temp);
            lstrcpy (chBuff[nAlign]+nIndex[nAlign], buf);
            nIndex[nAlign] += lstrlen(buf);
            src--;
         } else if (*src == letters[4] || *src == letters[5]) {
            /*------------------------------------------------*/
            /* Print the time                                 */
            /*------------------------------------------------*/
            WCHAR  szWide[2];

            GetLocaleInfoW (GetUserDefaultLCID (), LOCALE_STIME,
                            szWide, CharSizeOf(szWide));
#ifdef UNICODE
            lstrcpy (buf, szWide);
#else
            { BOOL   fDefCharUsed;
                WideCharToMultiByte (CP_OEMCP, 0, szWide, -1, buf,
                                 CharSizeOf(buf), NULL, &fDefCharUsed);
            }
#endif
            wsprintf (chBuff[nAlign]+nIndex[nAlign],
                      TEXT("%02d%c%02d%c%02d"), systime.wHour,
                      buf[0], systime.wMinute, buf[0], systime.wSecond);
            nIndex[nAlign] += 8;
         } else if (*src == letters[6] || *src == letters[7]) {
            /*------------------------------------------------*/
            /* Print the date                                 */
            /*------------------------------------------------*/
            wsprintf (chBuff[nAlign]+nIndex[nAlign],
                      TEXT("%3s %3s %02d %4d"),
                      szDay[systime.wDayOfWeek],
                      szMonth[systime.wMonth],
                      systime.wDay, systime.wYear);
            nIndex[nAlign] += 15;
         } else if (*src == TEXT('&')) {
            /*------------------------------------------------*/
            /* Print a single &                               */
            /*------------------------------------------------*/
            chBuff[nAlign][nIndex[nAlign]] = TEXT('&');
            nIndex[nAlign] += 1;
         } else if (*src == letters[8] || *src == letters[9])   /* left */ {
           /*------------------------------------------------*/
           /* Set the alignment for the following characters */
           /*------------------------------------------------*/
            nAlign = 0;

         } else if (*src == letters[10] || *src == letters[11]) /* center */
            nAlign = 1;

         else if (*src == letters[12] || *src == letters[13]) /* right */
            nAlign = 2;

         src++;

      }
   }

      /*------------------------------------------------*/
      /* Null-terminate all strings                     */
      /*------------------------------------------------*/
   for (nAlign = 0; nAlign < 3; nAlign++)
      chBuff[nAlign][nIndex[nAlign]] = (TCHAR) 0;
}

/*
 * print out the translated header/footer string in proper position.
 *
 * uses global stuff like CharPrintWidth, dyHeadFoot...
 *
 */

STATIC void NEAR PASCAL PrintHeaderFooter (
      HDC hPrintDC,
      NPRECT nprHeader,
      NPRECT nprFooter,
      LPTSTR szFileName)
{
   TCHAR   buf[3][80];
   int     len[3];

   TranslateString (buf, len, szHeader, szFileName);

   SetTextAlign (hPrintDC, TA_LEFT | TA_BOTTOM | TA_NOUPDATECP);
   if (len[0])
      TextOut(hPrintDC, nprHeader->left,
            nprHeader->bottom - CharPrintHeight, buf[0], len[0]);
   SetTextAlign(hPrintDC, TA_CENTER | TA_BOTTOM | TA_NOUPDATECP);
   if (len[1])
      TextOut(hPrintDC, (nprHeader->left + nprHeader->right)/2,
            nprHeader->bottom - CharPrintHeight, buf[1], len[1]);
   SetTextAlign(hPrintDC, TA_RIGHT | TA_BOTTOM | TA_NOUPDATECP);
   if (len[2])
      TextOut(hPrintDC, nprHeader->right,
            nprHeader->bottom - CharPrintHeight, buf[2], len[2]);

   TranslateString(buf, len, szFooter, szFileName);

   SetTextAlign(hPrintDC, TA_LEFT | TA_TOP | TA_NOUPDATECP);
   if (len[0])
      TextOut(hPrintDC, nprFooter->left,
            nprFooter->top + CharPrintHeight, buf[0], len[0]);
   SetTextAlign(hPrintDC, TA_CENTER | TA_TOP | TA_NOUPDATECP);
   if (len[1])
      TextOut(hPrintDC, (nprFooter->left + nprFooter->right)/2,
            nprFooter->top + CharPrintHeight, buf[1], len[1]);
   SetTextAlign(hPrintDC, TA_RIGHT | TA_TOP | TA_NOUPDATECP);
   if (len[2])
      TextOut(hPrintDC, nprFooter->right,
            nprFooter->top + CharPrintHeight, buf[2], len[2]);
}


STATIC void PRIVATE ActualPrintImg(HDC hPrtDC, NPRECT nprImage, BOOL draft)
{
   BOOL fBandingDevice;
   RECT bandRect, tempRect, fullPageRect;
   RECT rPrint, rHeader, rFooter;
#ifndef NT
   BANDINFOSTRUCT BandInfo;
   BOOL fBandInfo;
   int holder;
#endif
   int nInitReturn;
   TCHAR szMsg[BUFSIZE], buf[BUFSIZE];

   ComputePrintRect(nprImage, &rPrint, &rHeader, &rFooter);
   fullPageRect.left = fullPageRect.top = 0;
   fullPageRect.right = hResPrt;
   fullPageRect.bottom = vResPrt;

#ifndef NT
   holder = BANDINFO;
   fBandInfo = Escape(hPrtDC, QUERYESCSUPPORT, sizeof(holder), (LPSTR) &holder, NULL);
#endif
   fBandingDevice = GetDeviceCaps(hPrtDC, RASTERCAPS) & RC_BANDING;

   fDIBScaling = draft || (imagePlanes * imagePixels) == 1
      || (GetDeviceCaps(hPrtDC, RASTERCAPS) & RC_STRETCHDIB);

   if ((nInitReturn = InitPrinting(hPrtDC, nprImage, &rPrint,
                                   fileName[0] ? fileName : noFile)) != INITOK) {
      bUserAbort = TRUE;
      if(nInitReturn != NOMESG)
         SimpleMessage(IDSPrintInitErr, NULL, MB_OK | MB_ICONHAND);
      goto Error2;
   }

   for (nPage = 1, rImage.top = nprImage->top;
         rImage.top < nprImage->bottom;
         rImage.top += yImage) {
      for (rImage.left = nprImage->left;
            rImage.left < nprImage->right && !bUserAbort && !bError;
            rImage.left += xImage, ++nPage) {

         DB_OUT("NextPage\n\r");

         StartPage(hPrtDC);
         LoadString(hInst, IDSPrintPercent, buf, CharSizeOf(buf));
         wsprintf(szMsg, buf, nPage, nPages, (int)(100*lIncArea/lTotArea));
         SetDlgItemText(hDlgPrint, IDPERCENT, szMsg);

         rImage.right  = rImage.left + xImage;
         rImage.bottom = rImage.top + yImage;
         IntersectRect(&rImage, &rImage, nprImage);

         do {
            /* set fields to defaults */
            bandRect = fullPageRect;
#ifndef NT
            BandInfo.rcGraphics = rPrint;
            BandInfo.fText = BandInfo.fGraphics = TRUE;
#endif

            if (fBandingDevice) {
               /* tell the printer driver we're ready for the next band */
               Escape(hPrtDC, NEXTBAND, 0, NULL, (LPVOID) &bandRect);

               /* done page if band is empty */
               if (IsRectEmpty(&bandRect))
                  break;

#ifndef NT
                /* find out what type of band we are dealing with */
               if (fBandInfo) {
                  BandInfo.fText = FALSE;
#ifndef WIN32
                  Escape(hPrtDC, BANDINFO, sizeof(BandInfo), (LPCSTR) &BandInfo,
                         (LPVOID) &BandInfo);
#endif
               }
#endif
            }

            if(
#ifndef NT
                  BandInfo.fText &&
#endif
                  (IntersectRect(&tempRect, &rHeader, &bandRect)
                  || IntersectRect(&tempRect, &rFooter, &bandRect))) {
                  /* print header and footer */
                  PrintHeaderFooter (hPrtDC, &rHeader, &rFooter,
                                     fileName[0] ? fileName : noFile);
            }
#ifndef NT
            if (BandInfo.fGraphics) {
#endif
               /* do graphics stuff */
               PrintBand(hPrtDC, &bandRect, &rImage, &rPrint);
#ifndef NT
            }
#endif

         } while (fBandingDevice && !bUserAbort && !bError);

         if (!fBandingDevice)
            EndPage(hPrtDC);
      }
   }

   if(fBandingDevice)
      while(!IsRectEmpty(&bandRect))
         Escape(hPrtDC, NEXTBAND, 0, NULL, (LPVOID) &bandRect);

Error2:
   TermPrinting(hPrtDC, nInitReturn);
}

void PrintImg(NPRECT nprImage, BOOL draft, int copies)
{
   BOOL result = FALSE;
   HCURSOR oldcsr;
   TCHAR msg[80], buf[80];
   LPDEVNAMES lpDevNames;
   LPTSTR portName,deviceName;
   TCHAR nodefault[] = TEXT("NODEFAULT");
   extern HDC printDC;

   if (!printDC)
      goto Error0;

   oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));

   bUserAbort = bError = FALSE;
   lpfnAbortDlg = NULL;
   lpfnAbortPrt = NULL;
   hDlgPrint = NULL;

   if(!(lpfnAbortPrt = (ABORTPROC)MakeProcInstance((FARPROC)AbortPrt, hInst)))
      goto Error1;
   if(!(lpfnAbortDlg = (WNDPROC)MakeProcInstance((FARPROC)AbortDlg, hInst)))
      goto Error2;

#ifdef BOGUS
   SendMessage(pbrushWnd[PARENTid], WM_SYSCOMMAND, SC_MINIMIZE, 0L);
#endif

   if(!(hDlgPrint = CreateDialog(hInst, (LPTSTR) MAKEINTRESOURCE(ABORTBOX),
            pbrushWnd[PARENTid], (WNDPROC)lpfnAbortDlg)))
      goto Error3;

   EnableWindow(pbrushWnd[PARENTid], FALSE);

   LoadString(hInst, IDSPrintFile, buf, CharSizeOf(buf));
   wsprintf(msg, buf, PFileInPath(fileName[0] ? fileName : noFile));
   SetDlgItemText(hDlgPrint, IDFILENAME, msg);


   if (PD.hDevNames && (lpDevNames = (LPDEVNAMES)GlobalLock(PD.hDevNames))) {
        portName = (LPTSTR)lpDevNames+lpDevNames->wOutputOffset;
        deviceName = (LPTSTR)lpDevNames+lpDevNames->wDeviceOffset;
        GlobalUnlock(PD.hDevNames);
   }else{
        if(!GetDefaultPort())
                goto Error4;

        if(PD.hDevNames && (lpDevNames = (LPDEVNAMES)GlobalLock(PD.hDevNames))){
                portName = (LPTSTR)lpDevNames+lpDevNames->wOutputOffset;
                deviceName = (LPTSTR)lpDevNames+lpDevNames->wDeviceOffset;
                GlobalUnlock(PD.hDevNames);
        }else
                portName = deviceName =  nodefault;
   }

   LoadString(hInst, IDSPrintDevice, buf, CharSizeOf(buf));
   wsprintf(msg, buf, deviceName, portName);
   SetDlgItemText(hDlgPrint, IDDEVICEPORT, msg);

   for( ; copies>0 && !bUserAbort && !bError; )
   {
      copies -= SetPrintOptions(printDC, FALSE, copies);
      ActualPrintImg(printDC, nprImage, draft);
   }

   result = !bError;

Error4:
   if(hDlgPrint)
      SendMessage(hDlgPrint, WM_COMMAND, IDOK, 0L);
Error3:
#ifdef BOGUS
   SendMessage(pbrushWnd[PARENTid], WM_SYSCOMMAND, SC_RESTORE, 0L);
#endif
   UpdateWindow(pbrushWnd[PAINTid]);
   FreeProcInstance(lpfnAbortPrt);
Error2:
   FreeProcInstance(lpfnAbortDlg);
Error1:
   SetCursor(oldcsr);
   DeleteDC(printDC);
   printDC = NULL;
Error0:

   if(!result)
      SimpleMessage(IDSPrintInitErr, NULL, MB_OK);
}

