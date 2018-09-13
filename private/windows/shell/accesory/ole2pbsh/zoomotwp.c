/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   ZoomOtWP.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:   window proc for zoom out                   *
*   date:   04/02/87 @ 10:40                            *
*   date:   08/08/87 @ 10:40pm  MSZ                     *
********************************************************/


#include <windows.h>
#include <port1632.h>

#include "oleglue.h"
#include "pbrush.h"
#include "fixedpt.h"

extern BITMAPFILEHEADER_VER1 BitmapHeader;
extern BOOL drawing;
extern BOOL gfDirty;
extern BOOL mouseFlag, bZoomedOut;
extern BOOL TerminateKill;
extern TCHAR fileName[], clipName[], tempName[];       
extern DHDR imageHdr;
extern DWORD *rgbColor;
extern HDC pickDC;
extern HBITMAP pickBM;
extern HWND pbrushWnd[], mouseWnd;
extern HWND zoomOutWnd;
extern DPPROC DrawProc;
extern int cursTool;
extern int DlgCaptionNo;
extern int imageWid, imageHgt, imagePlanes, imagePixels;
extern int pickWid, pickHgt;
extern int theTool, theBackg;
extern int theSize;
extern LPTSTR DrawCursor;      
extern DPPROC dpArray[];
extern WORD wFileType;
extern HPALETTE hPalette;
extern RECT imageRect;
extern HDC printDC;
extern HWND hDlgPrint;

#define NOSOURCE 0
#define CLIPBOARD 1
#define PICKDC 2
#define PASTEFILE 3
#define SCREEN 4

extern int copies, quality;

BOOL MyPtInRect(LPRECT lprect,POINT  pt)
{
   return(pt.x >= lprect->left && pt.x <= lprect->right &&
         pt.y >= lprect->top && pt.y <= lprect->bottom);
}

void ClipPointToRect(LPPOINT lppt,LPRECT lprect)
{
   lppt->x = min(lppt->x, lprect->right);
   lppt->x = max(lppt->x, lprect->left);
   lppt->y = min(lppt->y, lprect->bottom);
   lppt->y = max(lppt->y, lprect->top);
}

static int zoomW, zoomH;

void ComputeZoomRect(LPRECT lprWind, LPRECT lprZoom)
{
   if (NGTN(NDivI(ToN(lprWind->right-lprWind->left), imageWid),
             NDivI(ToN(lprWind->bottom-lprWind->top), imageHgt))) {
      zoomH = lprWind->bottom - lprWind->top;
      zoomW = ROUND(NDivI(NMulI(ToN(zoomH), imageWid), imageHgt));
      lprZoom->top = 0;
      lprZoom->left = ((lprWind->right - lprWind->left) - zoomW) >> 1;
   } else {
      zoomW = lprWind->right -lprWind->left;
      zoomH = ROUND(NDivI(NMulI(ToN(zoomW), imageHgt), imageWid));
      lprZoom->left = 0;
      lprZoom->top = ((lprWind->bottom-lprWind->top) - zoomH) >> 1;
   }

   lprZoom->right = lprZoom->left + zoomW;
   lprZoom->bottom = lprZoom->top + zoomH;
}

static POINT startpt, endpt, pImgStart, pImgEnd;
extern BOOL IsConstrained;
RECT zoomRect;

LONG APIENTRY DrawPaste(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   REGISTER int dx, dy;
   RECT rcTemp, rcZoom;
   HBRUSH hBrush, hOldBrush;

   if(!IsConstrained) {
      GetClientRect(zoomOutWnd, &rcTemp);
      ComputeZoomRect(&rcTemp, &zoomRect);
      rcZoom = zoomRect;

      ClipPointToRect((LPPOINT)lprBounds  , &rcZoom);
      ClipPointToRect((LPPOINT)lprBounds+1, &rcZoom);

      ClientToScreen(zoomOutWnd, (LPPOINT) &rcZoom);
      ClientToScreen(zoomOutWnd, ((LPPOINT) &rcZoom) + 1);
      ClipCursor(&rcZoom);

      IsConstrained = TRUE;
   }

   dx = lprBounds->right - lprBounds->left;
   dy = lprBounds->bottom - lprBounds->top;

   startpt.x += dx;
   startpt.y += dy;
   endpt.x += dx;
   endpt.y += dy;

   lprBounds->left = lprBounds->right;
   lprBounds->top = lprBounds->bottom;

   Rectangle(dstDC, startpt.x, startpt.y, endpt.x, endpt.y);

   SetBkColor(dstDC, RGB(255, 255, 255));
   if(!(hBrush = CreateHatchBrush(HS_DIAGCROSS, RGB(0, 0, 0))))
      goto Error1;
   if(!(hOldBrush = SelectObject(dstDC, hBrush)))
      goto Error2;
   PatBlt(dstDC, startpt.x, startpt.y,
         endpt.x-startpt.x, endpt.y-startpt.y, ROP_DPnx);
   SelectObject(dstDC, hOldBrush);
Error2:
   DeleteObject(hBrush);
Error1:

   return(TRUE);
}

LONG FAR PASCAL ZoomOtWP(HWND hWnd,UINT message,WPARAM wParam,LONG lParam)
{
   static BOOL cutmade;
   static int nClipSource, nPickWidth, nPickHeight;
   static HBITMAP ZpickBM;
   static HDC ZpickDC;
   static NUM widRatio, hgtRatio;
   static WORD twFileType = BITMAPFILE;
   static BOOL IsArrow;
   POINT pt;

   int error;
   int i, width, height;
   PAINTSTRUCT ps;
   HCURSOR oldCsr;
   RECT r, bitmaprect;
   HMENU hMenu;
   HDC hdc, toolDC, clipbDC=NULL, hParentDC;
   HBITMAP clipbBM, oldBM;
   HANDLE clipbMF, clipbText;
   HBRUSH hcutbrush, oldBrush;
   BITMAP bitmap;
   HPALETTE hOldPalette;
   RECT rWind;

   if(IsWindow(pbrushWnd[PARENTid]))
      hMenu = GetMenu(pbrushWnd[PARENTid]);
   else {
      if(cutmade && nClipSource == PICKDC) {
         DeleteDC(ZpickDC);
         DeleteObject(ZpickBM);
      }
      nClipSource = NOSOURCE;

      return(DefWindowProc(hWnd,message,wParam,lParam));
   }

   switch(message) {
   case WM_KEYDOWN:
      if(wParam == VK_ESCAPE) {
         SendMessage(hWnd, WM_COMMAND, GET_WM_COMMAND_MPS(EDITundo, NULL, 0));
         bZoomedOut = FALSE;
         DrawProc = dpArray[theTool];
         DrawCursor = szPbCursor(theTool);
         cursTool = theTool;
         drawing = FALSE;
         DestroyWindow(hWnd);

         if (printDC && !hDlgPrint)
           {
             DeleteDC(printDC);
             printDC = NULL;
           }
         break;
      }
/* fall through if not ESCAPE key */
   case WM_KEYUP:
      SendMessage(pbrushWnd[PARENTid], message, wParam, lParam);
      break;

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam,lParam)) {
      case ZOOMaccept:
         gfDirty = TRUE;

         r.right = max(startpt.x, endpt.x);
         r.left = min(startpt.x, endpt.x);
         r.top = min(startpt.y, endpt.y);
         r.bottom = max(startpt.y, endpt.y);

         bitmaprect.top  = (int) ( ((long) (r.top - zoomRect.top) *
               imageHgt) /   zoomH);
         bitmaprect.right  = (int) ( ((long) (r.right - zoomRect.left) *
               imageHgt) /   zoomH);
         bitmaprect.left  = (int) ( ((long) (r.left - zoomRect.left) *
               imageHgt) /   zoomH);
         bitmaprect.bottom  = (int) ( ((long) (r.bottom - zoomRect.top) *
               imageHgt) /   zoomH);

         width = bitmaprect.right - bitmaprect.left;
         height = bitmaprect.bottom - bitmaprect.top;

         if(cutmade) {
            switch(nClipSource) {
            case PICKDC:
               BitBlt(hdcWork, bitmaprect.left, bitmaprect.top,
                     nPickWidth, nPickHeight, ZpickDC, 0, 0, SRCCOPY);
               BitBlt(hdcImage, bitmaprect.left, bitmaprect.top,
                     nPickWidth, nPickHeight, ZpickDC, 0, 0, SRCCOPY);
               InvalidateRect(hWnd, &r, TRUE);
               break;

            case CLIPBOARD:
               if(!OpenClipboard(hWnd)) {
                  PbrushOkError(IDSNoClipboard, MB_ICONEXCLAMATION);
                  break;
               }
               if (!(clipbBM = GetClipboardData(CF_BITMAP)) &&
                  !(clipbMF = GetClipboardData(CF_METAFILEPICT))) {
                  CloseClipboard();
                  PbrushOkError(IDSNoClipboardFormat, MB_ICONEXCLAMATION);
                  break;
               }

               if (hParentDC = GetDisplayDC(pbrushWnd[PARENTid]))
               {
                   clipbDC = CreateCompatibleDC(hParentDC);
                   ReleaseDC(pbrushWnd[PARENTid], hParentDC);
               }

               if (!clipbDC) {
                  CloseClipboard();
                  PbrushOkError(IDSNoDC, MB_ICONEXCLAMATION);
                  break;
               }

               if (hPalette) {
                  SelectPalette(clipbDC, hPalette, 0);
                  RealizePalette(clipbDC);
               }
               if (clipbBM)
                   oldBM = SelectObject(clipbDC, clipbBM);
               else /* metafile */
               {
                  RECT Rect;

                  Rect.left = Rect.top = 0;
                  Rect.right = nPickWidth; Rect.bottom = nPickHeight;
                  clipbBM = CreateBitmap(nPickWidth, nPickHeight, imagePlanes, imagePixels, NULL);
                  oldBM = SelectObject(clipbDC, clipbBM);
                  PlayMetafileIntoDC(clipbMF, &Rect, clipbDC);
               }
               BitBlt(hdcWork, bitmaprect.left, bitmaprect.top,
                              nPickWidth, nPickHeight, clipbDC, 0, 0, SRCCOPY);
               BitBlt(hdcImage, bitmaprect.left, bitmaprect.top,
                              nPickWidth, nPickHeight, clipbDC, 0, 0, SRCCOPY);
               SelectObject(clipbDC, oldBM);
               if (clipbMF)
                   DeleteObject(clipbBM);
               DeleteDC(clipbDC);
               InvalidateRect(hWnd, &r, TRUE);

               CloseClipboard();
               break;

            default:
               break;
            }
         }

         EnableMenuItem(hMenu, ZOOMundo, MF_GRAYED | MF_BYCOMMAND);
         ChangeCutCopy(hMenu, MF_GRAYED | MF_BYCOMMAND);
         EnableMenuItem(hMenu, EDITpasteFrom, MF_ENABLED | MF_BYCOMMAND);

         break;

      case EDITundo:
         if(cutmade) {
            hdc = GetDisplayDC(hWnd);
            SetROP2(hdc, R2_XORPEN);
            SelectObject(hdc, GetStockObject(WHITE_PEN));
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            IsConstrained = TRUE;
            r.left = r.top = r.right = r.bottom = 0;
            if(nClipSource == NOSOURCE)
               Rectangle(hdc, startpt.x, startpt.y, endpt.x, endpt.y);
            else
               DrawPaste(hdc, &r, 0);
            ReleaseDC(hWnd, hdc);
         }

         r.right = max(startpt.x, endpt.x);
         r.left = min(startpt.x, endpt.x);
         r.top = min(startpt.y, endpt.y);
         r.bottom = max(startpt.y, endpt.y);

         startpt.x = startpt.y = endpt.x = endpt.y = 0;
         pImgStart.x = pImgStart.y = pImgEnd.x = pImgEnd.y = 0;
         if(cutmade && nClipSource == PICKDC) {
            DeleteDC(ZpickDC);
            DeleteObject(ZpickBM);
         }

         cutmade = FALSE;
         nClipSource = NOSOURCE;

         EnableMenuItem(hMenu, ZOOMundo, MF_GRAYED | MF_BYCOMMAND);
         ChangeCutCopy(hMenu, MF_GRAYED | MF_BYCOMMAND);
         EnableMenuItem(hMenu, EDITpasteFrom, MF_ENABLED | MF_BYCOMMAND);

         UpdateWindow(hWnd);
         break;

      default:
         break;
      }
      break;

   case WM_PASTEFROM:
      SendMessage(hWnd, WM_COMMAND, EDITundo, 0l);

      ZpickDC = pickDC;
      ZpickBM = pickBM;
      nPickWidth = pickWid;
      nPickHeight = pickHgt;
      nClipSource = PICKDC;

      pickDC = NULL; pickBM = NULL;
      FreePick();

      startpt.x = zoomRect.left;
      startpt.y = zoomRect.top;
      endpt.x =ROUND(IDivN(pickWid, widRatio)) + zoomRect.left;
      endpt.y =ROUND(IDivN(pickHgt, hgtRatio)) + zoomRect.top;
      pImgStart.x = pImgStart.y = 0;
      pImgEnd.x = pickWid;
      pImgEnd.y = pickHgt;
      cutmade = TRUE;

      hdc = GetDisplayDC(hWnd);
      SetROP2(hdc, R2_XORPEN);
      SelectObject(hdc, GetStockObject(WHITE_PEN));
      SelectObject(hdc, GetStockObject(NULL_BRUSH));
      IsConstrained = TRUE;
      r.left = r.top = r.right = r.bottom = 0;
      DrawPaste(hdc, &r, 0);
      ReleaseDC(hWnd, hdc);

      EnableMenuItem(hMenu, ZOOMundo, MF_ENABLED | MF_BYCOMMAND);
      ChangeCutCopy(hMenu, MF_GRAYED | MF_BYCOMMAND);
      EnableMenuItem(hMenu, EDITpasteFrom, MF_GRAYED | MF_BYCOMMAND);
      break;

   case WM_COPYTO:
      r.right = max(startpt.x, endpt.x);
      r.left = min(startpt.x, endpt.x);
      r.top = min(startpt.y, endpt.y);
      r.bottom = max(startpt.y, endpt.y);

      bitmaprect.top  = (int)  ( ((long) (r.top - zoomRect.top) *
            imageHgt) /  zoomH);
      bitmaprect.right  =(int) ( ((long) (r.right - zoomRect.left) *
            imageHgt) /  zoomH);
      bitmaprect.left  =(int) ( ((long) (r.left - zoomRect.left) *
            imageHgt) /  zoomH);
      bitmaprect.bottom  = (int) ( ((long) (r.bottom - zoomRect.top) *
            imageHgt) /  zoomH);

      width = bitmaprect.right - bitmaprect.left;
      height = bitmaprect.bottom - bitmaprect.top;

      if(!IsRectEmpty(&r)) {
            /* remove the pick area and save undoish
               so that a repaint succeeds   */
            /* this does the dlg box, and then
               makes sure no terminate took place in between */
         TerminateKill = FALSE;
         if(DoFileDialog(SAVEBOX, pbrushWnd[PARENTid])) {
            lstrcpy(tempName,fileName);
            lstrcpy(fileName, clipName);
            if (wFileType == BITMAPFILE ||
                  wFileType == BITMAPFILE4 ||
                  wFileType == BITMAPFILE8 ||
                  wFileType == BITMAPFILE24)
               SaveBitmapFile(hWnd, bitmaprect.left,bitmaprect.top,
                     width, height, NULL);
            else
               SaveImg(hWnd,bitmaprect.left,bitmaprect.top,
                     width, height, (width /8 ) + (width %8 ? 1: 0),
                     NULL);
            lstrcpy(fileName, tempName);
         }
         TerminateKill = TRUE;
      }

      SendMessage(hWnd, WM_COMMAND, EDITundo, 0l);

      EnableMenuItem(hMenu, ZOOMundo, MF_GRAYED | MF_BYCOMMAND);
      ChangeCutCopy(hMenu, MF_GRAYED | MF_BYCOMMAND);
      EnableMenuItem(hMenu, EDITpasteFrom, MF_ENABLED | MF_BYCOMMAND);
      break;

   case WM_PASTE:
      SendMessage(hWnd, WM_COMMAND, EDITundo, 0l);

      if(!OpenClipboard(hWnd)) {
         PbrushOkError(IDSNoClipboardFormat, MB_ICONEXCLAMATION);
         goto Error2_1;
      }
      if((!(clipbBM = GetClipboardData(CF_BITMAP)) &&
          !(clipbMF = GetClipboardData(CF_METAFILEPICT)) &&
          !(clipbText = GetClipboardData(CF_TEXT))) ||
         !(hdc = GetDisplayDC(hWnd))){
         PbrushOkError(IDSNoClipboard, MB_ICONEXCLAMATION);
         goto Error2_2;
      }

      if (!clipbBM && !clipbMF && clipbText)
      {
         ReleaseDC(hWnd, hdc);
         SimpleMessage(IDSTextPasteMsgZoomed, TEXT(""), MB_OK);
         goto Error2_2;
      }

      cutmade = TRUE;

      if (clipbBM)
      {
          GetObject(clipbBM, sizeof(BITMAP),(LPVOID)&bitmap);  
          nPickHeight = bitmap.bmHeight;
          nPickWidth = bitmap.bmWidth;
      }
      else /* clipbMF */
      {
         if (!GetMFDimensions(clipbMF, hdc, &nPickWidth, &nPickHeight))
         {
             ReleaseDC(hWnd, hdc);
             goto Error2_2;
         }
      }

      startpt.x = zoomRect.left;
      startpt.y = zoomRect.top;
      endpt.x = (int) ( ((long) (nPickWidth) *
            zoomH) /    imageHgt) + startpt.x;
      endpt.y = (int) ( ((long)(nPickHeight) *
            zoomH) /    imageHgt) + startpt.y;
      pImgStart.x = pImgStart.y = 0;
      pImgEnd.x = nPickWidth;
      pImgEnd.y = nPickHeight;

      SetROP2(hdc, R2_XORPEN);
      SelectObject(hdc, GetStockObject(WHITE_PEN));
      SelectObject(hdc, GetStockObject(NULL_BRUSH));
      IsConstrained = TRUE;
      r.left = r.top = r.right = r.bottom = 0;
      DrawPaste(hdc, &r, 0);
      ReleaseDC(hWnd, hdc);

      nClipSource = CLIPBOARD;
/* end of test */
      /* Check if this is a metafile paste; note that the bitmap struct
       * will be uninitialized at this point. BUG #12386
       */
      if (!clipbBM)
         goto Error2_3;

      if(!(ZpickBM = CreateBitmap(bitmap.bmWidth, bitmap.bmHeight,
            bitmap.bmPlanes, bitmap.bmBitsPixel, NULL )))
         goto Error2_3;
      if(!(clipbDC = CreateCompatibleDC(NULL)))
         goto Error2_4;
      if(!(ZpickDC = CreateCompatibleDC(NULL)))
         goto Error2_5;

      if(!SelectObject(clipbDC, clipbBM))
         goto Error2_6;
      if(!SelectObject(ZpickDC, ZpickBM))
         goto Error2_6;

      if (hPalette) {
         SelectPalette(clipbDC, hPalette, 0);
         RealizePalette(clipbDC);
         SelectPalette(ZpickDC, hPalette, 0);
         RealizePalette(ZpickDC);
      }
      BitBlt(ZpickDC, 0,0, bitmap.bmWidth, bitmap.bmHeight,
            clipbDC, 0,0, SRCCOPY);

      nClipSource = PICKDC;

Error2_6:
      if(nClipSource != PICKDC)
         DeleteDC(ZpickDC);
Error2_5:
      DeleteDC(clipbDC);
Error2_4:
      if(nClipSource != PICKDC)
         DeleteObject(ZpickBM);
Error2_3:
Error2_2:
      CloseClipboard();
Error2_1:

      EnableMenuItem(hMenu, ZOOMundo, MF_ENABLED | MF_BYCOMMAND);
      ChangeCutCopy(hMenu, MF_GRAYED | MF_BYCOMMAND);
      EnableMenuItem(hMenu, EDITpasteFrom, MF_GRAYED | MF_BYCOMMAND);
      break;

   case WM_CUT:
   case WM_COPY:
      DOUTR(L"zoomotwp: WM_CUT/COPY" );

      oldCsr = SetCursor(LoadCursor(NULL,IDC_WAIT));
      error = TRUE;

      r.right = max(startpt.x, endpt.x);
      r.left = min(startpt.x, endpt.x);
      r.top = min(startpt.y, endpt.y);
      r.bottom = max(startpt.y, endpt.y);
      if(IsRectEmpty(&r))
         goto Error3_1;

      bitmaprect.top  = (int) ( ((long) (r.top - zoomRect.top) *
            imageHgt) /   zoomH);
      /* if the rect is as wide as it can be, to avoid rounding errors in the
       * above calcs, set the rect to its maximum size. */
      if (r.right == (zoomRect.right - 1))
          bitmaprect.right = imageWid-1;
      else
          bitmaprect.right  = (int) ( ((long) (r.right - zoomRect.left) *
                imageHgt) /   zoomH);
      bitmaprect.left  =(int) ( ((long) (r.left - zoomRect.left) *
            imageHgt) /   zoomH);
      if (r.bottom == (zoomRect.bottom - 1))
          bitmaprect.bottom = imageHgt-1;
      else
          bitmaprect.bottom  = (int)( ((long)(r.bottom - zoomRect.top) *
                imageHgt) /   zoomH);

      if(!(clipbBM = CreateBitmap(bitmaprect.right - bitmaprect.left+1,
            bitmaprect.bottom - bitmaprect.top+1,
            (BYTE)imagePlanes ,(BYTE )imagePixels, NULL)))
         goto Error3_1;
      if(!(clipbDC = CreateCompatibleDC(NULL)))
         goto Error3_2;
      if(!(oldBM = SelectObject(clipbDC, clipbBM)))
         goto Error3_3;

      if (hPalette) {
         SelectPalette(clipbDC, hPalette, 0);
         RealizePalette(clipbDC);
      }

      if (!BitBlt(clipbDC, 0, 0, bitmaprect.right-bitmaprect.left + 1,
                   bitmaprect.bottom-bitmaprect.top + 1,
                   hdcWork, bitmaprect.left, bitmaprect.top, SRCCOPY))
         goto Error3_4;

      error = FALSE;

      DOUTR( L"zoomotwp: Snapshotting bitmap" );
      if(SnapshotBitmap(clipbDC, bitmaprect) && message == WM_CUT) {
         if(!(hcutbrush = CreateSolidBrush(rgbColor[theBackg]))) {
            PbrushOkError(IDSNoCut, MB_ICONEXCLAMATION);
            goto Error3_5;
         }

         oldBrush = SelectObject(hdcWork, hcutbrush);
         PatBlt(hdcWork, bitmaprect.left, bitmaprect.top,
               bitmaprect.right-bitmaprect.left+1,
               bitmaprect.bottom-bitmaprect.top+1, PATCOPY);
         PatBlt(hdcImage, bitmaprect.left, bitmaprect.top,
               bitmaprect.right-bitmaprect.left+1,
               bitmaprect.bottom-bitmaprect.top+1, PATCOPY);

         InvalidateRect(hWnd, &r, TRUE);
         if (oldBrush)
            SelectObject(hdcWork, oldBrush);
         DeleteObject(hcutbrush);
Error3_5:
         ;
      }

      //
      // Offer our OLE data-transfer object...
      //
      DOUTR( L"zoomotwp: Transfering to clipboard" );
      TransferToClipboard();

Error3_4:
      SelectObject(clipbDC, oldBM);
Error3_3:
      DeleteDC(clipbDC);
Error3_2:
      DeleteObject(clipbBM);
Error3_1:
      if(error)
         PbrushOkError(IDSNoCopy, MB_ICONEXCLAMATION);

      SendMessage(hWnd, WM_COMMAND, EDITundo, 0l);

      EnableMenuItem(hMenu, ZOOMundo, MF_GRAYED | MF_BYCOMMAND);
      ChangeCutCopy(hMenu, MF_GRAYED | MF_BYCOMMAND);
      EnableMenuItem(hMenu, EDITpasteFrom, MF_ENABLED | MF_BYCOMMAND);

      SetCursor(oldCsr);
      break;

   case WM_CREATE:
      if(DrawProc == PrintDP) {
         EnableMenuItem(hMenu, 1, MF_GRAYED | MF_BYPOSITION);
         EnableMenuItem(hMenu, 2, MF_GRAYED | MF_BYPOSITION);

         hMenu = GetSystemMenu(pbrushWnd[PARENTid], FALSE);
         EnableMenuItem(hMenu, SC_CLOSE, MF_GRAYED | MF_BYCOMMAND);

         EnableWindow(pbrushWnd[TOOLid], FALSE);
      } else {
         nClipSource = NOSOURCE;
         toolDC = GetDisplayDC(pbrushWnd[TOOLid]);
         InvertButton(toolDC, theTool);
         InvertButton(toolDC, theTool = PICKtool);
         ReleaseDC(pbrushWnd[TOOLid], toolDC);
         DrawProc = dpArray[theTool];
         SetFocus(pbrushWnd[PAINTid]);

         startpt.x = startpt.y = endpt.x = endpt.y = 0;
         pImgStart.x = pImgStart.y = pImgEnd.x = pImgEnd.y = 0;
         SetActiveWindow(pbrushWnd[PARENTid]);
      }

      cursTool = PICKtool;
      IsArrow = TRUE;
      cutmade = FALSE;

      zoomW = zoomH = 1;
      break;

   case WM_SIZE:
      GetClientRect(hWnd, &r);

      ComputeZoomRect(&r, &zoomRect);

      widRatio = NDivI(ToN(imageWid), zoomW);
      hgtRatio = NDivI(ToN(imageHgt), zoomH);

      if(cutmade) {
         startpt.x = (int)((long)pImgStart.x*zoomW/imageWid) + zoomRect.left;
         startpt.y = (int)((long)pImgStart.y*zoomH/imageHgt) + zoomRect.top;
         endpt.x = (int)((long)pImgEnd.x*zoomW/imageWid) + zoomRect.left;
         endpt.y = (int)((long)pImgEnd.y*zoomH/imageHgt) + zoomRect.top;
      }
      break;

   case WM_PAINT:
      oldCsr = SetCursor(LoadCursor(NULL,IDC_WAIT));
      BeginPaint(hWnd,(LPPAINTSTRUCT)&ps);
      if(hPalette) {
         hOldPalette = SelectPalette(ps.hdc, hPalette, 0);
         RealizePalette(ps.hdc);
      }

      ps.rcPaint.right ++;
      ps.rcPaint.bottom ++;
      IntersectRect(&r, &ps.rcPaint, &zoomRect);

      if (!IsRectEmpty(&r)) {
         HDC hdcTmp = NULL;
         HBITMAP hbmTmp = NULL;
         HBITMAP hbmOld;
         int cxDst, cyDst, xDst, yDst;
         int cxSrc, cySrc, xSrc, ySrc;

         cxDst = r.right - r.left;
         cyDst = r.bottom - r.top;
         xDst = r.left;
         yDst = r.top;

         xSrc = ROUND(NMulI(r.left - zoomRect.left, widRatio));
         ySrc = ROUND(NMulI(r.top - zoomRect.top  , hgtRatio));
         cxSrc= ROUND(NMulI(r.right - r.left      , widRatio));
         cySrc= ROUND(NMulI(r.bottom - r.top      , hgtRatio));

         /*
          * Stretch blitting to the screen can take a *LONG* time to finish,
          * and all the while GDI will have the screen semaphore locked,
          * makeing the system appear to hang.  So to keep the system alive,
          * we will attempt to create a screen compatable temporary bitmap
          * and stretch blit to that, and then blt that bmp directly to the
          * screen.
          */
         if ((hdcTmp = CreateCompatibleDC(ps.hdc)) != NULL &&
              (hbmTmp = CreateCompatibleBitmap(ps.hdc, cxDst, cyDst)) != NULL) {

            hbmOld = SelectObject(hdcTmp, hbmTmp);
            xDst = 0;
            yDst = 0;

         } else {

            if (hdcTmp != NULL)
                DeleteDC(hdcTmp);

            hdcTmp = ps.hdc;
         }

         StretchBlt(hdcTmp, xDst, yDst, cxDst, cyDst,
               hdcWork,
               xSrc,
               ySrc,
               cxSrc,
               cySrc,
               SRCCOPY);

         if (hdcTmp != ps.hdc) {
            BitBlt( ps.hdc, r.left, r.top, cxDst, cyDst,
                    hdcTmp, 0, 0, SRCCOPY);

            SelectObject(hdcTmp, hbmOld);
            DeleteObject(hbmTmp);
            DeleteDC(hdc);
         }
      }
      SelectObject(ps.hdc,GetStockObject(NULL_BRUSH));
      Rectangle(ps.hdc,zoomRect.left,zoomRect.top,zoomRect.right,
            zoomRect.bottom);
      if(cutmade) {
         SetROP2(ps.hdc, R2_XORPEN);
         SelectObject(ps.hdc, GetStockObject(WHITE_PEN));
         SelectObject(ps.hdc, GetStockObject(NULL_BRUSH));
         IsConstrained = TRUE;
         r.left = r.top = r.right = r.bottom = 0;
         if(nClipSource == NOSOURCE)
            Rectangle(ps.hdc, startpt.x, startpt.y, endpt.x, endpt.y);
         else
            DrawPaste(ps.hdc, &r, 0);
      }
      if(hPalette && hOldPalette)
         SelectPalette(ps.hdc, hOldPalette, 0);
      EndPaint(hWnd,(LPPAINTSTRUCT)&ps);
      SetCursor(oldCsr);
      break;


   case WM_LBUTTONDOWN:
      if(DrawProc == PrintDP) {
         PrintDP(hWnd, message, wParam, lParam);

         if(!IsRectEmpty(&imageRect) &&
               IntersectRect(&imageRect, &imageRect, &zoomRect)) {
            OffsetRect(&imageRect, -zoomRect.left, -zoomRect.top);
            zoomW = zoomRect.right - zoomRect.left;
            widRatio = NDivI(ToN(imageWid), zoomW);
            zoomH = zoomRect.bottom - zoomRect.top;
            hgtRatio = NDivI(ToN(imageHgt), zoomH);

            imageRect.left = ROUND(NMulI(widRatio, imageRect.left));
            imageRect.right = ROUND(NMulI(widRatio, imageRect.right));
            imageRect.top = ROUND(NMulI(hgtRatio, imageRect.top));
            imageRect.bottom = ROUND(NMulI(hgtRatio, imageRect.bottom));

            SendMessage(pbrushWnd[PARENTid], WM_COMMAND, MISCzoomIn, 0L);

            PrintImg(&imageRect, quality == IDDRAFT, copies);
            SetRectEmpty((LPRECT)&imageRect);
         }
         break;
      }

      r.right = max(startpt.x, endpt.x);
      r.left = min(startpt.x, endpt.x);
      r.top = min(startpt.y, endpt.y);
      r.bottom = max(startpt.y, endpt.y);

      hdc = GetDisplayDC(hWnd);
      SetROP2(hdc, R2_XORPEN);
      SelectObject(hdc, GetStockObject(WHITE_PEN));
      SelectObject(hdc, GetStockObject(NULL_BRUSH));
      LONG2POINT(lParam,pt);
      if(!cutmade || (cutmade && !MyPtInRect(&r, pt))) {
         if(cutmade) {
            SendMessage(hWnd, WM_COMMAND, ZOOMaccept, 0L);
            SendMessage(hWnd, WM_COMMAND, EDITundo, 0l);
            Terminatewnd();

            cutmade = FALSE;
         }

         r.left = r.right = LOWORD(lParam);
         r.top = r.bottom = HIWORD(lParam);

         IsConstrained = FALSE;
         if(TrackTool(hWnd, DrawPrint, &r, &wParam, hdc)
               != WM_LBUTTONUP)
            goto Error1;

         ConstrainRect(&r, &zoomRect, wParam);

         startpt.x = r.left;
         startpt.y = r.top;
         endpt.x = r.right;
         endpt.y = r.bottom;
         pImgStart.x = (int)((long)(startpt.x-zoomRect.left)*imageWid/zoomW);
         pImgStart.y = (int)((long)(startpt.y-zoomRect.top )*imageHgt/zoomH);
         pImgEnd.x = (int)((long)(endpt.x-zoomRect.left)*imageWid/zoomW);
         pImgEnd.y = (int)((long)(endpt.y-zoomRect.top )*imageHgt/zoomH);

         r.right = max(startpt.x, endpt.x);
         r.left = min(startpt.x, endpt.x);
         r.top = min(startpt.y, endpt.y);
         r.bottom = max(startpt.y, endpt.y);

         if(IsRectEmpty(&r))
            goto Error1;

         Rectangle(hdc, startpt.x, startpt.y, endpt.x, endpt.y);

         EnableMenuItem(hMenu, ZOOMundo, MF_GRAYED | MF_BYCOMMAND);
         ChangeCutCopy(hMenu, MF_ENABLED | MF_BYCOMMAND);
         EnableMenuItem(hMenu, EDITpasteFrom, MF_ENABLED | MF_BYCOMMAND);

         cutmade = TRUE;
         nClipSource = NOSOURCE;
      } else if(nClipSource != NOSOURCE) {
         r.left = r.right = LOWORD(lParam);
         r.top = r.bottom = HIWORD(lParam);

         IsConstrained = TRUE;
         DrawPaste(hdc, &r, 0);
         IsConstrained = FALSE;
         TrackTool(hWnd, DrawPaste, &r, &wParam, hdc);
         DrawPaste(hdc, &r, 0);

         pImgStart.x = (int)((long)(startpt.x-zoomRect.left)*imageWid/zoomW);
         pImgStart.y = (int)((long)(startpt.y-zoomRect.top )*imageHgt/zoomH);
         pImgEnd.x = (int)((long)(endpt.x-zoomRect.left)*imageWid/zoomW);
         pImgEnd.y = (int)((long)(endpt.y-zoomRect.top )*imageHgt/zoomH);
      }

Error1:
      ReleaseDC(hWnd, hdc);
      break;

   case WM_MOUSEMOVE:
      r.right = max(startpt.x, endpt.x);
      r.left = min(startpt.x, endpt.x);
      r.top = min(startpt.y, endpt.y);
      r.bottom = max(startpt.y, endpt.y);
      LONG2POINT(lParam,pt);
      if(cutmade && MyPtInRect(&r, pt)
            && nClipSource != NOSOURCE) {
         if(!IsArrow) {
            SETCLASSCURSOR(hWnd, LoadCursor(NULL, IDC_ARROW));
            IsArrow = TRUE;
         }
      } else {
         if(IsArrow) {
            PbSetCursor(DrawCursor = szPbCursor(cursTool));
            IsArrow = FALSE;
         }
      }

      if(mouseFlag && MyPtInRect(&zoomRect, pt)) {
         PostMessage(mouseWnd, WM_MOUSEPOS, 0,
               MAKELONG((long)(pt.x - zoomRect.left)*imageWid/zoomW,
               (long)(pt.y - zoomRect.top)*imageHgt/zoomH));
      }
      break;

   case WM_DESTROY:
      DrawProc = dpArray[theTool];
      DrawCursor = szPbCursor(theTool);
      cursTool = theTool;
      PbSetCursor(DrawCursor);

      ShowWindow(pbrushWnd[PAINTid], SW_SHOW);
      GetClientRect(pbrushWnd[PARENTid], &rWind);
      SendMessage(pbrushWnd[PARENTid], WM_SIZE, 0,
            MAKELONG(rWind.right-rWind.left, rWind.bottom-rWind.top));

      EnableMenuItem(hMenu, ZOOMundo, MF_GRAYED | MF_BYCOMMAND);
      EnableMenuItem(hMenu, MISCzoomOut, MF_ENABLED | MF_BYCOMMAND);
      ChangeCutCopy(hMenu, MF_GRAYED | MF_BYCOMMAND);
      EnableMenuItem(hMenu, EDITpasteFrom, MF_ENABLED | MF_BYCOMMAND);
      for (i = 0; i < MAXmenus; i++)
         EnableMenuItem(hMenu, (UINT)i, MF_ENABLED | MF_BYPOSITION);
      EnableMenuItem(hMenu, MENUPOS_PICK, MF_GRAYED | MF_BYPOSITION);

      hMenu = GetSystemMenu(pbrushWnd[PARENTid], FALSE);
      EnableMenuItem(hMenu, SC_CLOSE, MF_ENABLED | MF_BYCOMMAND);

      DrawMenuBar(pbrushWnd[PARENTid]);

      EnableWindow(pbrushWnd[TOOLid], TRUE);
      break;

   default:
      return(DefWindowProc(hWnd,message,wParam,lParam));
      break;
   }

   return(0L);
}
