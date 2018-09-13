/****************************Module*Header******************************\
* Module Name: newpick.c                                                *
*                                                                       *
*                                                                       *
*                                                                       *
* Created: 1989                                                         *
*                                                                       *
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
*                                                                       *
* A general description of how the module is used goes here.            *
*                                                                       *
* Additional information such as restrictions, limitations, or special  *
* algorithms used if they are externally visible or effect proper use   *
* of the module.                                                        *
\***********************************************************************/

#include   <windows.h>
#include <port1632.h>
#include   "pbrush.h"
#include "pbserver.h"

#ifndef CBM_CREATEDIB
#   define CBM_CREATEDIB   0x02L   /* create DIB bitmap */
#endif

extern HWND pbrushWnd[];
extern int pickWid, pickHgt;
extern int imagePlanes, imagePixels;
extern HDC pickDC, saveDC, monoDC, tempDC;
extern HBITMAP pickBM, saveBM, monoBM;
extern HPALETTE hPalette;
extern DWORD *rgbColor;
extern int theBackg;
extern RECT pickRect;
extern POINT polyPts[];
extern int numPts;
extern int theTool;
extern DPPROC DrawProc;
extern LPTSTR DrawCursor;
extern BOOL TerminateKill;
extern RECT imageView;

extern BOOL bExchanged;
extern RECT rDirty;

/* define raster ops needed for MyMaskBlt */
#define    DSa  0x008800C6L
#define    DSna 0x00220326L

static POINT OldPos, CurPos;
static BOOL  bPickSwept;

short nPickMode;

HBITMAP MapBitmapColors( HDC hdcMem, HPALETTE hpalDst, HBITMAP hbmSrc,
        HPALETTE hpalSrc );

void EnablePickMenu(BOOL bEnable)
{
   static BOOL bStatus = FALSE;

   HMENU hMenu;

   /* don't change if it is already set */
   if (bStatus == bEnable)
       return;

   hMenu = GetMenu(pbrushWnd[PARENTid]);
   EnableMenuItem(hMenu, MENUPOS_PICK, (bEnable ? MF_ENABLED : MF_GRAYED) |
                            MF_BYPOSITION);
   DrawMenuBar(pbrushWnd[PARENTid]);
   bStatus = bEnable;
}

int AlocPick(HDC hDC)
{
   if (pickBM = CreateBitmap(pickWid, pickHgt, (BYTE) imagePlanes,
                             (BYTE) imagePixels, 0L))
       if (pickDC = CreateCompatibleDC(hDC))
           SelectObject(pickDC, pickBM);

   if (saveBM = CreateBitmap(pickWid, pickHgt, (BYTE) imagePlanes,
                             (BYTE) imagePixels, 0L))
       if (saveDC = CreateCompatibleDC(hDC))
           SelectObject(saveDC, saveBM);

   if (monoBM = CreateBitmap(pickWid, pickHgt, 1, 1, 0L))
       if (monoDC = CreateCompatibleDC(hDC))
           SelectObject(monoDC, monoBM);

   if(pickBM && pickDC && saveBM && saveDC && monoBM && monoDC) {
       if (hPalette) {
           SelectPalette(pickDC, hPalette, 0);
           SelectPalette(saveDC, hPalette, 0);
           RealizePalette(pickDC);
           RealizePalette(saveDC);
       }

       return TRUE;
   }

   /* something was not built */
   FreePick();
   return FALSE;
}


/*  Build an opaque/transparent mask for bitmap contained in pickDC
**
**  The mask is 0 when the destination should remain unchanged (transparent)
**  The mask is 1 when the source should be copied to destination (opaque)
*/
void BuildMask(int mode)
{
   DWORD  rgbOldBk;
   DWORD  dwROP;

   /* Since we change lots of things just do global save of monoDC */
   SaveDC(monoDC);

   /* Start with everything transparent */
   PatBlt(monoDC, 0, 0, pickWid, pickHgt, BLACKNESS);

   /* Set window origin of monoDC so we don't have to transform points */
   MSetWindowOrg(monoDC, pickRect.left, pickRect.top);

   /* make our interior/exterior white */
   SelectObject(monoDC, GetStockObject(WHITE_PEN));
   SelectObject(monoDC, GetStockObject(WHITE_BRUSH));

   /* fill the interior */
   SetPolyFillMode(monoDC, ALTERNATE);
   Polygon(monoDC, polyPts, numPts);

   if (mode == TRANSPARENT) {
       /* set background to transparent color */

       DB_OUTF((acDbgBfr, TEXT("Bkgd = %lx\r\n"), rgbColor[theBackg]));

       rgbOldBk = SetBkColor(pickDC, rgbColor[theBackg]);

       dwROP = DSna;
       if (imagePlanes == 1 && imagePixels == 1 &&
           !PBGetNearestColor(monoDC, rgbColor[theBackg]))
           dwROP = DSa;

       BitBlt(monoDC, pickRect.left, pickRect.top, pickWid, pickHgt,
              pickDC, 0, 0,
              dwROP);

       /* restore pickDC's background color */
       SetBkColor(pickDC, rgbOldBk);
   }

   /* throw out the clipping region, reset window origin to (0,0) */
   RestoreDC(monoDC, -1);

}

static RECT rcPaint;

LONG APIENTRY DrawDotRect(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   RECT rcTemp;

   rcTemp = *lprBounds;
   ConstrainRect(&rcTemp, &rcPaint, wParam);

   DotRect(dstDC, rcTemp.left, rcTemp.top, rcTemp.right, rcTemp.bottom);

   return(TRUE);
}

LONG APIENTRY DrawShrGroRect(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   int     dx, dy;
   BOOL    bFinal;
   RECT    r;

   bFinal = (GetROP2(dstDC) == R2_COPYPEN);

   if (wParam & MK_SHIFT) {
       dx = abs(lprBounds->right - lprBounds->left);
       dy = abs(lprBounds->bottom - lprBounds->top);

       if (pickHgt < pickWid) {
           dy = (int) (((long) dx * pickHgt) / pickWid);
       } else {
           dx = (int) (((long) dy * pickWid) / pickHgt);
       }
   } else {
       dx = lprBounds->right - lprBounds->left;
       dy = lprBounds->bottom - lprBounds->top;
   }

       if (bFinal) {
           r.left = lprBounds->left;
           r.top = lprBounds->top;
           r.right = lprBounds->left + dx;
           r.bottom = lprBounds->top + dy;
           *lprBounds = r;
       } else {
         DotRect(dstDC, lprBounds->left, lprBounds->top,
                 lprBounds->left + dx, lprBounds->top + dy);
       }

   return TRUE;
}

LONG APIENTRY DrawDotPoly(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   static int xLast = -1, yLast = -1;

   /* do nothing if we already have all of our points */
   if (numPts == MAXpts - 1 || (GetROP2(dstDC) == R2_COPYPEN))
       return TRUE;

   if (xLast != lprBounds->right || yLast != lprBounds->bottom) {
       polyPts[numPts].x = xLast = lprBounds->right;
       polyPts[numPts].y = yLast = lprBounds->bottom;
       ++numPts;

       if (numPts > 1)
          LineTo(dstDC, xLast, yLast);
       else
          MMoveTo(dstDC, xLast, yLast);
   }

   /* if we are terminating reset last points back to -1 */
   if (GetROP2(dstDC) == R2_COPYPEN)
       xLast = yLast = -1;

   return TRUE;
}


/*static*/ LONG PRIVATE TrackPick(HWND hWnd, LPRECT rcReturn, WPARAM *wParam)
{
   WORD w;

   if (theTool == PICKtool || DrawProc == ShrGroDP) {
       GetClientRect(hWnd, &rcPaint);
       w=TrackTool(hWnd, DrawDotRect, rcReturn, wParam, NULL);
   } else
       w=TrackTool(hWnd, DrawDotPoly, rcReturn, wParam, NULL);
   return w;
}

extern int cntr;

LONG APIENTRY DragProc(HDC dstDC, LPRECT lprBounds, WPARAM wParam)
{
   static int dx = -32768, dy = -32768;

   UINT    OldROP;
   RECT    rcOld, rcNew, rcTemp;
   int     unionWid, unionHgt;
   BOOL    bUseRadMouse;

   if (GetROP2(dstDC) == R2_COPYPEN) {
       dx = dy = -32768;
   } else {
       if (dx == -32768) {
           cntr = 0;
           dx = lprBounds->right - CurPos.x;
           dy = lprBounds->bottom - CurPos.y;
       }
       if(01&cntr++)
           return(FALSE);

       OldROP = SetROP2(dstDC, R2_COPYPEN);

       /* compute old/new rectangles */
       rcOld.left = CurPos.x;
       rcOld.top = CurPos.y;
       rcOld.right = rcOld.left + pickWid - 1;
       rcOld.bottom = rcOld.top + pickHgt - 1;
       rcNew.left = lprBounds->right - dx;
       rcNew.top = lprBounds->bottom - dy;
       rcNew.right = rcNew.left + pickWid - 1;
       rcNew.bottom = rcNew.top + pickHgt - 1;

       bUseRadMouse = IntersectRect(&rcTemp, &rcOld, &rcNew) ? TRUE
                                                             : FALSE;
       /* compute union rect */
       UnionRect(&rcTemp, &rcOld, &rcNew);
       rcTemp.right++;
       rcTemp.bottom++;
       unionWid = rcTemp.right - rcTemp.left;
       unionHgt = rcTemp.bottom - rcTemp.top;


       if (bUseRadMouse) {
           /* restore old area if we aren't sweeping */
           if (!(wParam & MK_SHIFT)) // EDH: change to SHIFT for sweep
               BitBlt(hdcWork, rcOld.left+imageView.left,
                               rcOld.top+imageView.top,
                               pickWid, pickHgt,
                      saveDC, 0, 0, SRCCOPY);
           else
               bPickSwept = TRUE;

           /* Save area under new pick */
           BitBlt(saveDC, 0, 0, pickWid, pickHgt,
                  hdcWork, rcNew.left+imageView.left,
                           rcNew.top+imageView.top,
                  SRCCOPY);

           /* draw pick */
           MaskBlt(hdcWork, rcNew.left+imageView.left,
                            rcNew.top+imageView.top,
                            pickWid, pickHgt,
                   pickDC, 0, 0,monoBM, 0, 0,MASKROP(SRCCOPY,0x00aa0000));

           /* transfer result to screen */

           /* Reset window origion */
       } else {
           if (!(wParam & MK_SHIFT)) // EDH: change to SHIFT for sweep
               BitBlt(hdcWork, rcOld.left + imageView.left,
                               rcOld.top + imageView.top,
                               pickWid, pickHgt,
                      saveDC, 0, 0, SRCCOPY);
           else
               bPickSwept = TRUE;

           BitBlt(saveDC, 0, 0, pickWid, pickHgt,
                  hdcWork, rcNew.left + imageView.left,
                           rcNew.top + imageView.top,
                  SRCCOPY);
           MaskBlt(hdcWork, rcNew.left + imageView.left,
                              rcNew.top + imageView.top,
                              pickWid, pickHgt,
                   pickDC, 0, 0,monoBM, 0, 0,MASKROP(SRCCOPY,0x00aa0000));
       }

       rcTemp.left += imageView.left;
       rcTemp.right += imageView.left;
       rcTemp.top += imageView.top;
       rcTemp.bottom += imageView.top;

       BitBlt(dstDC, rcTemp.left - imageView.left,
                     rcTemp.top - imageView.top,
                     unionWid, unionHgt,
                  hdcWork, rcTemp.left, rcTemp.top,
                  SRCCOPY);

       UnionWithRect(&rDirty,&rcTemp);

       if (fOLE)
           SendDocChangeMsg(vpdoc, OLE_CHANGED);

       SetROP2(dstDC, OldROP);

       CurPos.x = rcNew.left;
       CurPos.y = rcNew.top;
   }

   return TRUE;
}

void EnableOutline(BOOL bEnable)
{
   static BOOL bPickStatus = FALSE;
   static BOOL bScissorStatus = FALSE;

   HDC hDC;

   if (!(hDC = GetDisplayDC(pbrushWnd[PAINTid])))
       return;

   switch (theTool) {
       case PICKtool:
           if (bPickStatus == bEnable)
               break;

           DotRect(hDC, pickRect.left, pickRect.top,
                        pickRect.right, pickRect.bottom);
           bPickStatus = bEnable;
           break;

       case SCISSORStool:
           if (bScissorStatus == bEnable)
               break;

           DotPoly(hDC, polyPts, numPts);
           bScissorStatus = bEnable;
           break;
   }

   ReleaseDC(pbrushWnd[PAINTid], hDC);
}

BOOL IsInSelection(POINT pt)
{
   int x = pt.x - CurPos.x;
   int y = pt.y - CurPos.y;

   if (!monoDC ||
       !PtInRect(&pickRect, pt) ||
       GetPixel(monoDC, x, y) == RGB(0,0,0))
       return FALSE;

   return TRUE;
}

void FillBkgd(HDC hDC, int x, int y, int dx, int dy)
{
   HBRUSH hBrush, hOldBrush;

   if (hBrush = CreateSolidBrush(rgbColor[theBackg]))
      hOldBrush = SelectObject(hDC, hBrush);

   PatBlt(hDC, x, y, dx, dy, PATCOPY);

   if (hBrush) {
       if (hOldBrush)
           SelectObject(hDC, hOldBrush);
       DeleteObject(hBrush);
   }
}

void FillInCutoutOrigin(void)
{
   HDC     dstDC;
   HBRUSH  hBrush, hOldBrush;
   RECT    tempRect;

   tempRect.left   = pickRect.left + imageView.left;
   tempRect.right  = pickRect.right + imageView.left;
   tempRect.top    = pickRect.top + imageView.top;
   tempRect.bottom = pickRect.bottom + imageView.top;

   dstDC = GetDisplayDC(pbrushWnd[PAINTid]);

   BitBlt(saveDC, 0, 0, pickWid, pickHgt,
          pickDC, 0, 0, SRCCOPY);

   MSetBrushOrg(saveDC, -imageView.left-pickRect.left,
         -imageView.top-pickRect.top);
   if (hBrush = CreateSolidBrush(rgbColor[theBackg]))
      hOldBrush = SelectObject(saveDC, hBrush);

   BitBlt(saveDC, 0, 0, pickWid, pickHgt,
           monoDC, 0, 0, ROP_DSPDxax);

   if (hBrush) {
      if(hOldBrush)
         SelectObject(saveDC, hOldBrush);
      DeleteObject(hBrush);
   }

   /* copy save area to work image */
   BitBlt(hdcWork, tempRect.left, tempRect.top, pickWid, pickHgt,
          saveDC, 0, 0, SRCCOPY);

   UnionWithRect(&rDirty,&tempRect);

   if (fOLE)
           SendDocChangeMsg(vpdoc, OLE_CHANGED);

   BitBlt(dstDC, pickRect.left, pickRect.top, pickWid, pickHgt,
          pickDC, 0, 0, SRCCOPY);

   ReleaseDC(pbrushWnd[PAINTid], dstDC);
}


void ClearPickArea(void)
{
   HDC  dstDC;
   RECT rect1;

   dstDC = GetDisplayDC(pbrushWnd[PAINTid]);

   if (!dstDC) return;

   EnableOutline(FALSE);

   BuildMask(OPAQUE);

   BitBlt(hdcWork, CurPos.x + imageView.left,
                   CurPos.y + imageView.top, pickWid, pickHgt,
          saveDC, 0, 0,
          SRCCOPY);
   FillBkgd(hdcWork, CurPos.x + imageView.left,
                     CurPos.y + imageView.top,  pickWid, pickHgt);
   BitBlt(dstDC, CurPos.x, CurPos.y, pickWid, pickHgt,
          hdcWork, CurPos.x + imageView.left,
                   CurPos.y + imageView.top,
          SRCCOPY);

   rect1.left = CurPos.x + imageView.left;
   rect1.right = rect1.left + pickWid;
   rect1.top = CurPos.y + imageView.top;
   rect1.bottom = rect1.top + pickHgt;

   UnionWithRect(&rDirty,&rect1);

   if (fOLE)
       SendDocChangeMsg(vpdoc, OLE_CHANGED);

   ReleaseDC(pbrushWnd[PAINTid], dstDC);
}

BOOL PickDP(HWND hWnd, UINT code, WPARAM wParam, LONG lParam)
{
   static BOOL bFirstDrag = FALSE, bDragging = FALSE;
   static BOOL bCrossh = TRUE;

   POINT       newPt;
   RECT        rcBounds;
   BOOL        bCancel = FALSE;
   HDC         dstDC, hClipDC;
   HMENU       hMenu;
   HBITMAP     hClipBM;
   BITMAP      bm;
   POINT       OldPos;
   LPPAINTSTRUCT lpps;
   HCURSOR     oldcsr;
   RECT        rc1;

   LONG2POINT(lParam,newPt);
   dstDC = NULL;
   switch (code) {
       case WM_PAINT:
           lpps = (LPPAINTSTRUCT) lParam;
           if (IsRectEmpty(&pickRect))
               break;

           BitBlt(lpps->hdc, CurPos.x, CurPos.y, pickWid, pickHgt,
                  saveDC, 0, 0,
                  SRCCOPY);
           MaskBlt(lpps->hdc, CurPos.x, CurPos.y, pickWid, pickHgt,
                   pickDC, 0, 0,monoBM, 0, 0,MASKROP(SRCCOPY,0x00aa0000));
           break;

       case WM_LBUTTONDOWN:
           if(bExchanged)
               PasteDownRect(rDirty.left, rDirty.top,
               rDirty.right-rDirty.left, rDirty.bottom-rDirty.top);

           if (!IsInSelection(newPt)) {
               SendMessage(hWnd, WM_TERMINATE, 0, 0L);
               UpdatImg();

               rcBounds.left = rcBounds.right = newPt.x;
               rcBounds.top = rcBounds.bottom = newPt.y;
               bCancel = TrackPick(hWnd, &rcBounds, &wParam) == WM_RBUTTONDOWN;

       case WM_OUTLINE:
               if (code == WM_OUTLINE) {
                   GetClientRect(hWnd, &rcPaint);
                   rcBounds = pickRect;
               }

               dstDC = GetDisplayDC(hWnd);
               if (!dstDC)
                   bCancel = TRUE;
               else {
                   /* Get final coordinates */
                   ConstrainRect(&rcBounds, &rcPaint, wParam);

                   if (theTool == PICKtool) {
                       numPts = 4;
                       polyPts[0].x = polyPts[3].x = rcBounds.left;
                       polyPts[0].y = polyPts[1].y = rcBounds.top;
                       polyPts[1].x = polyPts[2].x = rcBounds.right;
                       polyPts[2].y = polyPts[3].y = rcBounds.bottom;
                   } else {
                       SetROP2(dstDC, R2_XORPEN);
                       SelectObject(dstDC, GetStockObject(WHITE_PEN));
                       Polyline(dstDC, polyPts, numPts);
                       SetROP2(dstDC, R2_COPYPEN);
                       PolyRect(polyPts, numPts, &rcBounds);
                   }
               }

               /* if bounding rect is empty then just cancel operation */
               if (IsRectEmpty(&rcBounds))
                   bCancel = TRUE;

               if (!bCancel) {

                   /* get bounding rect and its width/height */
                   pickRect = rcBounds;
                   pickWid = pickRect.right - pickRect.left + 1;
                   pickHgt = pickRect.bottom - pickRect.top + 1;

                   /* Allocate offscreen bitmaps for pick operations */
                   if (!AlocPick(hdcWork)) {
                       SimpleMessage(IDSNoMemAvail, NULL,
                             MB_OK | MB_ICONEXCLAMATION);
                       bCancel = TRUE;
                   } else {
                       /* copy pick into pickDC */
                       BitBlt(pickDC, 0, 0, pickWid, pickHgt,
                              hdcWork, pickRect.left + imageView.left,
                                       pickRect.top + imageView.top,
                              SRCCOPY);


                       /* Initialize saveDC to screen contents */
                       BitBlt(saveDC, 0, 0, pickWid, pickHgt,
                              hdcWork, pickRect.left + imageView.left,
                              pickRect.top + imageView.top,
                              SRCCOPY);


                       /* make an opaque mask to use for hit testing */
                       BuildMask(nPickMode = OPAQUE);


                       /* draw the dotted outline */
                       EnableOutline(TRUE);

                       /* Save current/old position */
                       OldPos.x = CurPos.x = pickRect.left;
                       OldPos.y = CurPos.y = pickRect.top;

                       /* Enable Pick Menu */
                       EnablePickMenu(TRUE);

                       /* Enable Copy options in edit menu */
                       ChangeCutCopy(GetMenu(pbrushWnd[PARENTid]), MF_ENABLED);
                   }
               }

               if (bCancel)
                   SendMessage(hWnd, WM_TERMINATE, 0, 0L);

               if (dstDC)
                   ReleaseDC(hWnd, dstDC);

               break;
           }

           /* Note: If left button down pressed inside selection then
           ** FALL THROUGH to the right button down message to begin
           ** movement processing.
           */

       case WM_RBUTTONDOWN:
           if (IsRectEmpty(&pickRect) || !IsInSelection(newPt))
               break;

           /* remove outline */
           EnableOutline(FALSE);

           /* if left button down then make transparent mask */
           if (code == WM_LBUTTONDOWN)
               BuildMask(nPickMode = TRANSPARENT);

           dstDC = GetDisplayDC(hWnd);

           /* if shift key down make save area same as pick */
           if ((wParam & MK_CONTROL) && dstDC) { // EDH: change to CTRL for copy
               rc1.left = pickRect.left + imageView.left;
               rc1.right = rc1.left + pickWid;
               rc1.top = pickRect.top + imageView.top;
               rc1.bottom = rc1.top + pickHgt;

               BitBlt(saveDC, 0, 0, pickWid, pickHgt,
                      hdcWork, rc1.left, rc1.top,
                      SRCCOPY);

               UnionWithRect(&rDirty, &rc1);

               if (fOLE)
                   SendDocChangeMsg(vpdoc, OLE_CHANGED);

               UpdFlag(TRUE);

           } else if (bFirstDrag)
               FillInCutoutOrigin();

           bFirstDrag = FALSE;
           bPickSwept = FALSE;

           /***** Allocate work memory for dragging *****/

           OldPos = CurPos;

           rcBounds.left = rcBounds.right = newPt.x;
           rcBounds.top = rcBounds.bottom = newPt.y;

           bDragging = TRUE;
           TrackTool(hWnd, DragProc, &rcBounds, &wParam, NULL);
           bDragging = FALSE;

           /* reset track tool */
           if (dstDC) {
              DragProc(dstDC, &rcBounds, wParam);
              ReleaseDC(hWnd, dstDC);
           }

           /* get rid of temporary storage */

           /* user swept, so just copy entire area to work buffer */
           if (bPickSwept) {
//              CopyToWork(0, 0, 0, 0);
                rDirty.left = 0;
                rDirty.top  = 0;
                rDirty.right = nNewImageWidth;
                rDirty.bottom = nNewImageHeight;

                if (fOLE)
                    SendDocChangeMsg(vpdoc, OLE_CHANGED);

               UpdFlag(TRUE);
           }

           /* move pickRect and polyPts to new position */
           OffsetRect(&pickRect, CurPos.x - OldPos.x, CurPos.y - OldPos.y);
           OffsetPoly(polyPts, numPts, CurPos.x - OldPos.x,
                      CurPos.y - OldPos.y);

           BuildMask(nPickMode = OPAQUE);
           EnableOutline(TRUE);
           break;

       case WM_MOUSEMOVE:
           if (bDragging || IsInSelection(newPt)) {
               if (bCrossh) {
                   PbSetCursor(szPbCursor(HANDtool));
                   bCrossh = FALSE;
               }
           } else {
               if (!bCrossh) {
                  PbSetCursor(DrawCursor);
                  bCrossh = TRUE;
               }
           }
           break;

       case WM_TERMINATE:
           EnableOutline(FALSE);

           if (!IsRectEmpty(&pickRect) && imageFlag) {
//             CopyToWork(pickRect.left, pickRect.top, pickWid, pickHgt);
               rc1.left = pickRect.left + imageView.left;
               rc1.right = rc1.left + pickWid;
               rc1.top = pickRect.top + imageView.top;
               rc1.bottom = rc1.top + pickHgt;

               UnionWithRect(&rDirty, &rc1);

               if (fOLE)
                   SendDocChangeMsg(vpdoc, OLE_CHANGED);

               UpdFlag(TRUE);
           }

           numPts = 0;
           SetRectEmpty(&pickRect);
           bFirstDrag = TRUE;

           ChangeCutCopy(GetMenu(pbrushWnd[PARENTid]), MF_GRAYED);
           EnablePickMenu(FALSE);

           /* tell DotRect to delete its local objects */
           DotRect(NULL, 0, 0, 0, 0);
           FreePick();
           break;

       case WM_PICKFLIPH:
           oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
           if (dstDC = GetDisplayDC(hWnd)) {
               EnableOutline(FALSE);


               if (bFirstDrag) {
                   FillInCutoutOrigin();
                   bFirstDrag = FALSE;
               }

               StretchBlt(pickDC, pickWid-1, 0, -pickWid, pickHgt,
                          pickDC, 0, 0, pickWid, pickHgt,
                          SRCCOPY);


               StretchBlt(monoDC, pickWid-1, 0, -pickWid, pickHgt,
                          monoDC, 0, 0, pickWid, pickHgt,
                          SRCCOPY);


               BitBlt(hdcWork, CurPos.x + imageView.left,
                               CurPos.y + imageView.top,
                      pickWid, pickHgt,
                      saveDC, 0, 0,
                      SRCCOPY);

               MaskBlt(hdcWork, CurPos.x + imageView.left,
                               CurPos.y + imageView.top,
                       pickWid, pickHgt,
                       pickDC, 0, 0,monoBM, 0, 0,MASKROP(SRCCOPY,0x00aa0000));


               BitBlt(dstDC, CurPos.x, CurPos.y, pickWid, pickHgt,
                      hdcWork, CurPos.x + imageView.left,
                               CurPos.y + imageView.top,
                      SRCCOPY);

               rc1.left = CurPos.x + imageView.left;
               rc1.top = CurPos.y + imageView.top;
               rc1.right = rc1.left + pickWid;
               rc1.bottom = rc1.top + pickHgt;

               UnionWithRect(&rDirty,&rc1);
               if (fOLE)
                   SendDocChangeMsg(vpdoc, OLE_CHANGED);

               FlipPoly(polyPts, numPts, FLIPh, &pickRect);
               EnableOutline(TRUE);

               ReleaseDC(hWnd, dstDC);
           }
           SetCursor(oldcsr);
           break;

       case WM_PICKFLIPV:
           oldcsr = SetCursor(LoadCursor(NULL, IDC_WAIT));
           if (dstDC = GetDisplayDC(hWnd)) {
               EnableOutline(FALSE);


               if (bFirstDrag) {
                   FillInCutoutOrigin();
                   bFirstDrag = FALSE;
               }

               StretchBlt(pickDC, 0, pickHgt-1, pickWid, -pickHgt,
                          pickDC, 0, 0, pickWid, pickHgt,
                          SRCCOPY);


               StretchBlt(monoDC, 0, pickHgt-1, pickWid, -pickHgt,
                          monoDC, 0, 0, pickWid, pickHgt,
                          SRCCOPY);


               BitBlt(hdcWork, CurPos.x + imageView.left,
                               CurPos.y + imageView.top,
                      pickWid, pickHgt,
                      saveDC, 0, 0,
                      SRCCOPY);

               MaskBlt(hdcWork, CurPos.x + imageView.left,
                               CurPos.y + imageView.top,
                       pickWid, pickHgt,
                       pickDC, 0, 0,monoBM, 0, 0,MASKROP(SRCCOPY,0x00aa0000));

               BitBlt(dstDC, CurPos.x, CurPos.y, pickWid, pickHgt,
                      hdcWork, CurPos.x + imageView.left,
                               CurPos.y + imageView.top,
                      SRCCOPY);

               rc1.left = CurPos.x + imageView.left;
               rc1.top = CurPos.y + imageView.top;
               rc1.right = rc1.left + pickWid;
               rc1.bottom = rc1.top + pickHgt;

               UnionWithRect(&rDirty,&rc1);
               if (fOLE)
                   SendDocChangeMsg(vpdoc, OLE_CHANGED);

               FlipPoly(polyPts, numPts, FLIPv, &pickRect);
               EnableOutline(TRUE);

               ReleaseDC(hWnd, dstDC);
           }
           SetCursor(oldcsr);
           break;

       case WM_PICKINVERT:
           if (dstDC = GetDisplayDC(hWnd)) {
               EnableOutline(FALSE);

               PatBlt(pickDC, 0, 0, pickWid, pickHgt, DSTINVERT);


               BuildMask(OPAQUE);


               BitBlt(hdcWork, CurPos.x + imageView.left,
                               CurPos.y + imageView.top,
                      pickWid, pickHgt,
                      saveDC, 0, 0,
                      SRCCOPY);

               MaskBlt(hdcWork, CurPos.x + imageView.left,
                                CurPos.y + imageView.top,
                       pickWid, pickHgt,
                       pickDC,0, 0, monoBM, 0, 0,MASKROP(SRCCOPY,0x00aa0000));


               BitBlt(dstDC, CurPos.x, CurPos.y, pickWid, pickHgt,
                      hdcWork, CurPos.x + imageView.left,
                               CurPos.y + imageView.top,
                      SRCCOPY);

               rc1.left = CurPos.x + imageView.left;
               rc1.top = CurPos.y + imageView.top;
               rc1.right = rc1.left + pickWid;
               rc1.bottom = rc1.top + pickHgt;

               UnionWithRect(&rDirty,&rc1);
               if (fOLE)
                   SendDocChangeMsg(vpdoc, OLE_CHANGED);

               EnableOutline(TRUE);

               ReleaseDC(hWnd, dstDC);
           }
           break;

       case WM_PICKSG:
           if (IsRectEmpty((LPRECT) &pickRect))
               break;

           EnableOutline(FALSE);
           DrawProc = ShrGroDP;
           hMenu = GetMenu(pbrushWnd[PARENTid]);
           CheckMenuItem(hMenu, PICKsg, MF_CHECKED);
           ChangeCutCopy(hMenu, MF_GRAYED);
           break;

       case WM_PICKTILT:
           if (IsRectEmpty((LPRECT)&pickRect))
               break;

           EnableOutline(FALSE);
           DrawProc = TiltDP;
           hMenu = GetMenu(pbrushWnd[PARENTid]);
           CheckMenuItem(hMenu, PICKtilt, MF_CHECKED);
           break;

       case WM_COPYTO:
           if (!IsRectEmpty(&pickRect)) {
               int result;

               EnableOutline(FALSE);
               TerminateKill = FALSE;
               result = DoFileDialog(SAVEBOX, pbrushWnd[PARENTid]);
               if (result)
                   SaveClip(pickDC, pickBM, pickWid, pickHgt);
               TerminateKill = TRUE;
               UpdateWindow(pbrushWnd[PARENTid]);
               EnableOutline(TRUE);
           }
           break;

       case WM_CUT:
       case WM_COPY:
           if (!IsRectEmpty(&pickRect)) {
               HDC tempDC=NULL;
               HBITMAP tempBM=NULL;

               EnableOutline(FALSE);

               tempDC = CreateCompatibleDC(NULL);
               tempBM = CreateBitmap(pickWid, pickHgt, (BYTE) imagePlanes,
                                     (BYTE) imagePixels, 0L);
               if (tempDC && tempBM) {
                   if (SelectObject(tempDC, tempBM)) {
                       RECT rcFix = pickRect;

                       ++rcFix.right;
                       ++rcFix.bottom;

                       FillBkgd(tempDC, 0, 0, pickWid, pickHgt);
                       MaskBlt(tempDC, 0, 0, pickWid, pickHgt,
                               pickDC, 0, 0,monoBM, 0, 0,MASKROP(SRCCOPY,0x00aa0000));
                       bCancel = DumpBitmapToClipboard(tempDC, code, rcFix);
                       UpdateWindow(hWnd);
                   }
               }

               if (tempDC)
                   DeleteDC(tempDC);
               tempDC = NULL;
               if (tempBM)
                   DeleteObject(tempBM);
               tempBM = NULL;

               if (bCancel && code == WM_CUT) {
                   if(bFirstDrag) {
                      /* Set the image changed flag
                       */
                      imageFlag = TRUE;
                      ClearPickArea();
                   } else {
                      dstDC = GetDisplayDC(pbrushWnd[PAINTid]);
                      BitBlt(hdcWork, CurPos.x + imageView.left,
                                   CurPos.y + imageView.top,
                                   pickWid, pickHgt,
                             saveDC, 0, 0,
                             SRCCOPY);
                      BitBlt(dstDC, CurPos.x, CurPos.y, pickWid, pickHgt,
                             hdcWork, CurPos.x + imageView.left,
                                      CurPos.y + imageView.top,
                             SRCCOPY);

                      ReleaseDC(pbrushWnd[PAINTid], dstDC);
                   }
                   SendMessage(hWnd, WM_TERMINATE, 0, 0L);
               } else
                   EnableOutline(TRUE);
           }
           break;

        case WM_PASTE:
        {
            HANDLE      hMF = NULL, hDIB;
            HBITMAP     hOldBM = NULL;
            LPBITMAPINFO        lpDIB;
            LPBITMAPINFOHEADER  bmHdr;
            UINT                nOffset;
            HPALETTE            hpalClip = NULL;

            if (!OpenClipboard(hWnd))   /* can't open the clipboard */
                break;
            if ((!((hClipBM = GetClipboardData(CF_BITMAP)) ||
                  (hClipBM = GetClipboardData(CF_DIB))) && /* if no bitmap/metafile */
                 !(hMF = GetClipboardData(CF_METAFILEPICT))) ||
                 !(dstDC = GetDisplayDC(hWnd)))  /* failed to get a DC */
            {
                CloseClipboard();
                break;
            }

            /* If this is a DIB, we need to convert it */
            if (hDIB = GetClipboardData(CF_DIB))
            {
                lpDIB = GlobalLock (hDIB);
                bmHdr = (LPBITMAPINFOHEADER) lpDIB;

                if (bmHdr->biClrUsed)
                    nOffset = bmHdr->biClrUsed * sizeof( RGBQUAD );
                else {
                    /*
                     * No clr count specified.  Color count is either none,
                     * or else the palette is as big as needed.
                     */
                    if( bmHdr->biBitCount > 8 )
                        nOffset = 0;
                    else {
                        nOffset = ( sizeof( RGBQUAD ) << bmHdr->biBitCount );
                    }
                }

                /* Skip any bitfield masks if we have them */
                if ( bmHdr->biBitCount > 8 && bmHdr->biCompression == BI_BITFIELDS ) {
                    nOffset += (sizeof(DWORD) * 3);
                }

                if ((hClipBM = CreateDIBitmap (dstDC,
                        (LPBITMAPINFOHEADER) lpDIB,
                        CBM_INIT, ( BYTE * )( lpDIB ) + bmHdr->biSize + nOffset,
                        lpDIB, DIB_RGB_COLORS)) == NULL)
                {
                    CloseClipboard ();
                    GlobalUnlock (hDIB);
                    break;
                }

                GlobalUnlock( hDIB );
            } else if (IsClipboardFormatAvailable(CF_PALETTE)) {
                hpalClip = GetClipboardData(CF_PALETTE);
            }

            if (!IsRectEmpty(&pickRect))
                SendMessage(hWnd, WM_TERMINATE, 0, 0L);

            /* get dimensions of the bitmap to be pasted */
            if (hClipBM)
            {
                GetObject(hClipBM, sizeof(BITMAP), (LPVOID) &bm);
                pickWid = bm.bmWidth;
                pickHgt = bm.bmHeight;
            }
            else /* metafile on clipboard, compute its dimensions */
            {
                if (!GetMFDimensions(hMF, hdcWork, &pickWid, &pickHgt))
                {
                    CloseClipboard();
                    ReleaseDC(hWnd, dstDC);
                    break;
                }
            }

            /* fill polyPts with rect coords */
            numPts = 4;
            polyPts[0].x = polyPts[3].x =
            polyPts[0].y = polyPts[1].y =
            pickRect.left = pickRect.top = 0;
            polyPts[1].x = polyPts[2].x = pickRect.right = pickWid - 1;
            polyPts[2].y = polyPts[3].y = pickRect.bottom = pickHgt - 1;

            /* Allocate offscreen bitmaps for pick operations */
            if (AlocPick(hdcWork)) {
                /* copy bitmap into pickDC */
                if (hClipBM && (hClipDC = CreateCompatibleDC(hdcWork)))
                {
                    HBITMAP hbmTmp = NULL;
                    if (hPalette != NULL) {
                        hbmTmp = MapBitmapColors( hClipDC, hPalette, hClipBM,
                            hpalClip );
                    } else if (hpalClip != NULL) {
                        HPALETTE hpalNew;
                        /*
                         * We're pasting a paletized bitmap and we don't already
                         * have a palette selected.  Adopt the one from the
                         * clipboard
                         */
                        hpalNew = CopyPalette( hpalClip );
                        SelectPalette(hdcWork, hpalNew, 0);
                        RealizePalette(hdcWork);
                        SelectPalette(hdcImage, hpalNew, 0);
                        RealizePalette(hdcImage);

                        hPalette = hpalNew;
                    }

                    hOldBM = SelectObject(hClipDC, hbmTmp != NULL ?
                            hbmTmp : hClipBM);

                    BitBlt(pickDC, 0, 0, pickWid, pickHgt, hClipDC, 0, 0, SRCCOPY);
                    SelectObject(hClipDC, hOldBM);
                    DeleteDC(hClipDC);

                    if (hbmTmp != NULL) {
                        DeleteObject(hbmTmp);
                    }
                }
                else if (hMF)   /* play metafile into the pickDC */
                {
                    RECT Rect;

                    Rect.left = Rect.top = 0;
                    Rect.right = pickWid; Rect.bottom = pickHgt;
                    PlayMetafileIntoDC(hMF, &Rect, pickDC);
                }

                /* Initialize saveDC to ULC of workDC */
                BitBlt(saveDC, 0, 0, pickWid, pickHgt,
                       hdcWork, pickRect.left + imageView.left,
                       pickRect.top + imageView.top,
                       SRCCOPY);

                /* make an opaque mask to use for hit testing */
                BuildMask(nPickMode = OPAQUE);

                /* draw pick on screen */
                rc1.left = pickRect.left + imageView.left;
                rc1.top = pickRect.top + imageView.top;
                rc1.right = rc1.left + pickWid;
                rc1.bottom = rc1.top + pickHgt;

                MaskBlt(hdcWork, rc1.left, rc1.top, pickWid, pickHgt,
                        pickDC, 0, 0, monoBM, 0, 0,MASKROP(SRCCOPY,0x00aa0000));

                UnionWithRect(&rDirty,&rc1);
                if (fOLE)
                   SendDocChangeMsg(vpdoc, OLE_CHANGED);
                BitBlt(dstDC, pickRect.left,
                                pickRect.top,
                      pickWid, pickHgt,
                      hdcWork, rc1.left,
                               rc1.top,
                      SRCCOPY);

                /* draw the dotted outline */
                EnableOutline(TRUE);

                /* Save current/old position */
                OldPos.x = CurPos.x = pickRect.left;
                OldPos.y = CurPos.y = pickRect.top;

                /* Enable Pick Menu */
                EnablePickMenu(TRUE);

                /* Enable Copy options in edit menu */
                ChangeCutCopy(GetMenu(pbrushWnd[PARENTid]), MF_ENABLED);

                /* make sure screen preserved on drag */
                bFirstDrag = FALSE;
            } else
                SimpleMessage(IDSNoMemAvail, NULL, MB_OK | MB_ICONEXCLAMATION);

            ReleaseDC(hWnd, dstDC);
            CloseClipboard();
        }
            break;

       case WM_PASTEFROM:
           if (!pickDC)
               break;

           /* fill polyPts with rect coords */
           numPts = 4;
           polyPts[0].x = polyPts[3].x =
           polyPts[0].y = polyPts[1].y =
           pickRect.left = pickRect.top = 0;
           polyPts[1].x = polyPts[2].x = pickRect.right = pickWid - 1;
           polyPts[2].y = polyPts[3].y = pickRect.bottom = pickHgt - 1;

           dstDC = GetDisplayDC(hWnd);
           if (dstDC) {
               /* Initialize saveDC to ULC of screen */
               BitBlt(saveDC, 0, 0, pickWid, pickHgt,
                      hdcWork, pickRect.left + imageView.left,
                               pickRect.top + imageView.top,
                      SRCCOPY);

               /* make an opaque mask to use for hit testing */
               BuildMask(nPickMode = OPAQUE);

               /* draw pick on screen */
               rc1.left = pickRect.left + imageView.left;
               rc1.top = pickRect.top + imageView.top;
               rc1.right = rc1.left + pickWid;
               rc1.bottom = rc1.top + pickHgt;

               MaskBlt(hdcWork, rc1.left, rc1.top, pickWid, pickHgt,
                       pickDC, 0, 0,monoBM, 0, 0,MASKROP(SRCCOPY,0x00aa0000));

               UnionWithRect(&rDirty,&rc1);
               if (fOLE)
                  SendDocChangeMsg(vpdoc, OLE_CHANGED);
               BitBlt(dstDC, pickRect.left,
                             pickRect.top,
                      pickWid, pickHgt,
                      hdcWork, rc1.left,
                               rc1.top,
                      SRCCOPY);

               /* draw the dotted outline */
               EnableOutline(TRUE);

               /* Save current/old position */
               OldPos.x = CurPos.x = pickRect.left;
               OldPos.y = CurPos.y = pickRect.top;

               /* Enable Pick Menu */
               EnablePickMenu(TRUE);

               /* Enable Copy options in edit menu */
               ChangeCutCopy(GetMenu(pbrushWnd[PARENTid]), MF_ENABLED);
               ReleaseDC(hWnd, dstDC);

               bFirstDrag = FALSE; /* make sure screen preserved on drag */
               UpdFlag(TRUE);
           } else {
               SendMessage(hWnd, WM_TERMINATE, 0, 0L);
           }
           break;

       default:
           break;
   }

   return(0);
}

void FAR ChangeCutCopy(HMENU hMenu, WORD wEnable) {
    EnableMenuItem(hMenu, EDITcutpict, wEnable);
    EnableMenuItem(hMenu, EDITcopypict, wEnable);
    EnableMenuItem(hMenu, EDITcopyTo, wEnable);
}

#if 0

#pragma message( __FILE__ "(1342) : warning @@@@ : remove this debug code" )

#define JONP    0x504e4f4aL
#define NCSU    0x5543534eL

#define MsgBox( szTxt, szTitle )    \
    MessageBox( GetDesktopWindow(), szTxt, szTitle, MB_OK )

HLOCAL AllocLPTR(UINT cbBytes ) {
    LPDWORD pdw;
    UINT cb;

    cb = ((cbBytes + 3) & ~3);

    pdw = LocalAlloc( LPTR, cb + sizeof(DWORD) * 3 );

    if (pdw != NULL ) {
        *pdw++ = (DWORD)cbBytes;
        *pdw++ = JONP;
        pdw[cb/4] = NCSU;
    }

    return (LPBYTE)pdw;
}

HLOCAL FreeLPTR( HLOCAL hpb, LPTSTR szName ) {
    LPDWORD pdw;
    DWORD cb;
    LPBYTE pb = (LPBYTE)hpb;

    pdw = (LPDWORD)pb;

    if ( *--pdw != JONP )
        MsgBox( szName, TEXT("Under Write on pointer") );

    pdw--;
    cb = *pdw;

    cb = ((cb + 3) & ~3);
    if ( ((LPDWORD)pb)[cb/4] != NCSU )
        MsgBox( szName, TEXT("OverWrite on pointer") );

    return LocalFree( pdw );
}

#else
#   define AllocLPTR( cb )          LocalAlloc( LPTR, cb )
#   define FreeLPTR(pb, sz)         LocalFree( pb )
#   define MsgBox( s1, s2 )         /* nothing */
#endif


HBITMAP MapBitmapColors( HDC hdcMem, HPALETTE hpalDst, HBITMAP hbmSrc,
        HPALETTE hpalSrc ) {

    WORD cColors;
    LPBITMAPINFO lpbmi = NULL;
    BITMAPINFO bmi;
    HPALETTE hpalOld;
    LPVOID lpBits = NULL;
    HBITMAP hbmNew = NULL;

    if (hpalSrc == NULL)
        return NULL;

    hpalOld = SelectPalette(hdcMem, hpalSrc, FALSE);
    RealizePalette(hdcMem);


    ZeroMemory( &(bmi.bmiHeader), sizeof(bmi.bmiHeader) );
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);

    if (GetDIBits( hdcMem, hbmSrc, 0, 0, NULL, &bmi, DIB_RGB_COLORS ) == 0) {
        MsgBox( L"GetDIBits1 failed", NULL );
        goto MBCExit;
    }

    cColors = 1 << (bmi.bmiHeader.biPlanes * bmi.bmiHeader.biBitCount);

    lpbmi = AllocLPTR( sizeof(BITMAPINFO) + (sizeof(RGBQUAD) * cColors) );

    if (lpbmi == NULL)
        goto MBCExit;

    CopyMemory( lpbmi, &bmi, sizeof(bmi) );

    lpbmi->bmiHeader.biSize = sizeof(lpbmi->bmiHeader);

    lpBits = AllocLPTR( lpbmi->bmiHeader.biHeight *
            ((lpbmi->bmiHeader.biWidth * lpbmi->bmiHeader.biPlanes *
            lpbmi->bmiHeader.biBitCount + 31) & ~31) / 8);


    if (lpBits == NULL) {
        MsgBox(L"LocalAllocs failed", NULL);
        goto MBCExit;
    }

    if (GetDIBits( hdcMem, hbmSrc, 0, lpbmi->bmiHeader.biHeight, lpBits, lpbmi,
            DIB_RGB_COLORS ) == 0) {
        MsgBox( L"GetDIBits2 failed", NULL );
        goto MBCExit;
    }

    SelectPalette( hdcMem, hpalDst, FALSE );
    RealizePalette(hdcMem);

    if ( (hbmNew = CreateDIBitmap( hdcMem, NULL, CBM_INIT | CBM_CREATEDIB,
            lpBits, lpbmi, DIB_RGB_COLORS )) == NULL) {
        MsgBox(L"SetDIBits failed", NULL);
    }

MBCExit:
    if (lpbmi != NULL)
        FreeLPTR(lpbmi, TEXT("lpbmi"));

    if (lpBits != NULL)
        FreeLPTR(lpBits, TEXT("lpBits") );

    SelectPalette(hdcMem, hpalOld, FALSE);

    return hbmNew;
}
