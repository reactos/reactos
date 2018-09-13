#include "onlypbr.h"
#undef NOATOM
#undef NOKERNEL
#undef NOMEMMGR
#undef NORASTEROPS
#undef NOMENUS

#include <windows.h>
#include <port1632.h>

//#define NOEXTERN
#include "pbrush.h"
#include "pbserver.h"

extern BOOL inMagnify;
extern DWORD *rgbColor;
extern HWND pbrushWnd[];
extern DPPROC DrawProc;
extern int imageWid, imageHgt, imageByteWid, imagePlanes, imagePixels;
extern int theBackg;
extern int zoomWid, zoomHgt;
extern POINT viewOrg;
extern RECT zoomView;

#ifdef DEBUG
WORD wForceBands = MF_UNCHECKED;
#endif

HDC hdcWork=0,hdcImage=0;
HBITMAP hbmWork=0,hbmImage=0;
RECT theBounds = {0, 0, 0, 0};
RECT rDirty = {0, 0, 0, 0};
static int bDontCopy = 0;
BOOL bExchanged = FALSE;

void UnionWithRect(LPRECT lprDst, LPRECT lprSrc)
{
   if(IsRectEmpty(lprDst))
      *lprDst = *lprSrc;
   else {
      if(lprDst->left > lprSrc->left)
         lprDst->left = lprSrc->left;
      if(lprDst->left > lprSrc->right)
         lprDst->left = lprSrc->right;
      if(lprDst->right < lprSrc->right)
         lprDst->right = lprSrc->right;
      if(lprDst->right < lprSrc->left)
         lprDst->right = lprSrc->left;
      if(lprDst->top > lprSrc->top)
         lprDst->top = lprSrc->top;
      if(lprDst->top > lprSrc->bottom)
         lprDst->top = lprSrc->bottom;
      if(lprDst->bottom < lprSrc->bottom)
         lprDst->bottom = lprSrc->bottom;
      if(lprDst->bottom < lprSrc->top)
         lprDst->bottom = lprSrc->top;
   }
}

void UnionWithPt(LPRECT lprDst, POINT thePt)
{
   if(lprDst->left > thePt.x)
      lprDst->left = thePt.x;
   if(lprDst->right < thePt.x)
      lprDst->right = thePt.x;

   if(lprDst->top > thePt.y)
      lprDst->top = thePt.y;
   if(lprDst->bottom < thePt.y)
      lprDst->bottom = thePt.y;
}

DWORD TotalMemoryAvailable(void)
{
   DWORD dwWinFlags, cbFree, cbAboveLine;
   BOOL bEMS;

   dwWinFlags = MGetWinFlags();

   /* Are we running on an EMS System? */
   bEMS = (BOOL) (dwWinFlags & (WF_LARGEFRAME | WF_SMALLFRAME));

   /* Get the number of free bytes below the line. */
   cbFree = GetFreeSpace(GMEM_NOT_BANKED);

   if (bEMS) {
      /* Get the number of free bytes above the line. */
      cbAboveLine = GetFreeSpace(0);
      cbFree += cbAboveLine;
   }

   return cbFree;
}

int MyCreateDCBitmap(HDC FAR *pDC,HBITMAP FAR *pBM, int wid, int hgt, int planes,
	int pixelBits,DWORD dwBkgnd) {

    HBRUSH hBrush,hOldBrush;

    *pDC=CreateCompatibleDC(NULL);
    *pBM = CreateBitmap(wid, hgt,planes, pixelBits, NULL);

    SelectObject(*pDC,*pBM);

    if (dwBkgnd != (DWORD)(-1L)) {
       hBrush = CreateSolidBrush(dwBkgnd);
       if (hBrush) {
	   hOldBrush = SelectObject(*pDC, hBrush);
	   PatBlt(*pDC, 0, 0, wid, hgt, PATCOPY);
           if (hOldBrush)
	       SelectObject(*pDC, hOldBrush);
           DeleteObject(hBrush);
       }
   }

   return TRUE;

}

WORD AllocImg(int wid, int hgt, int planes, int pixelBits, BOOL erase)
{
   HDC hDC;
   DWORD dwBkgd;
// WORD err;


   /* if an image is already allocated, delete it */
   if (hdcWork)
      FreeImg();

   /* set parameter default values if necessary */
   if(hDC = GetDisplayDC(pbrushWnd[PARENTid])) {
      if(!planes)
         planes = GetDeviceCaps(hDC, PLANES);
      if(!pixelBits)
         pixelBits = GetDeviceCaps(hDC, BITSPIXEL);
      ReleaseDC(pbrushWnd[PARENTid], hDC);
   } else {
      if(!planes)
         planes = 1;
      if(!pixelBits)
         pixelBits = 1;
   }

   dwBkgd = erase ? rgbColor[theBackg] : -1L;

   MyCreateDCBitmap(&hdcWork,&hbmWork,wid, hgt, planes, pixelBits, dwBkgd);
   MyCreateDCBitmap(&hdcImage,&hbmImage,wid, hgt, planes, pixelBits, dwBkgd);

   /* set image globals */
   imageWid = wid;
   imageHgt = hgt;
   imageByteWid = (wid * pixelBits) >> 3;
   imagePixels = pixelBits;
   imagePlanes = planes;
   rDirty.left = rDirty.right = rDirty.top = rDirty.bottom = 0;

   return 0;

//Error3:
//   VDeleteObject(hWorkVBM);
//   hWorkVBM = NULL;
//
//Error2:
//   /* set image globals */
//   imageWid = wid;
//   imageHgt = hgt;
//   imagePixels = pixelBits;
//   imagePlanes = planes;

//   switch(err) {
//   case VBM_NODISKSPACE:
//	return(IDSNotDiskAvail);
//	break;

//   case VBM_NOMEMSPACE:
//   default:
//	return(IDSNotMemAvail);
//	break;
//   }
}

void FreeImg(void)
{
    if (hdcImage) {
	DeleteDC(hdcImage);
	DeleteObject(hbmImage);
	hdcImage=0;
	hbmImage=0;
    }
    if (hdcWork) {
	DeleteDC(hdcWork);
	DeleteObject(hbmWork);
	hdcWork=0;
	hbmWork=0;
    }
}

int ClearImg(void)
{
   HBRUSH hBrush, hOldBrush;

   if(!hdcWork || !hdcImage
         || !(hBrush = CreateSolidBrush(rgbColor[theBackg])))
      return(FALSE);

   MSetBrushOrg(hdcWork, 0, 0);
   hOldBrush = SelectObject(hdcWork, hBrush);
   PatBlt(hdcWork, 0, 0, imageWid, imageHgt, PATCOPY);
   if (hOldBrush)
      SelectObject(hdcWork, hOldBrush);

   MSetBrushOrg(hdcImage, 0, 0);
   hOldBrush = SelectObject(hdcImage, hBrush);
   PatBlt(hdcImage, 0, 0, imageWid, imageHgt, PATCOPY);
   if (hOldBrush)
      SelectObject(hdcImage, hOldBrush);

   DeleteObject(hBrush);

   rDirty.left = rDirty.right = rDirty.top = rDirty.bottom = 0;

   return(TRUE);
}

void CopyFromWork(int left, int top, int width, int height)
{
   HDC hDC;
   RECT rTemp;

   if(!hdcWork)
      return;

   GetClientRect(pbrushWnd[PAINTid], &rTemp);
   if((left | top | width | height) == 0) {
      if(inMagnify) {
         left = viewOrg.x;
         top = viewOrg.y;
         width = zoomWid;
         height = zoomHgt;
      } else {
         width = rTemp.right;
         height = rTemp.bottom;
      }
   }

   if(!(hDC = GetDisplayDC(pbrushWnd[PAINTid])))
      return;
   if(inMagnify)
      BitBlt(hDC, left, top, width, height,
	    hdcWork, imageView.left+zoomView.left,
	    imageView.top+zoomView.top, SRCCOPY);
   else
      BitBlt(hDC, left, top, width, height,
	    hdcWork, left + imageView.left, top + imageView.top, SRCCOPY);
   ReleaseDC(pbrushWnd[PAINTid], hDC);
}

void PasteDownRect(int left, int top, int width, int height)
{
   register int temp;

   if(!hdcWork || !hdcImage)
      return;

   if((left | top | width | height) == 0) {
      left = rDirty.left;
      top = rDirty.top;
      width = rDirty.right - rDirty.left;
      height = rDirty.bottom - rDirty.top;
      rDirty.left = rDirty.top = rDirty.right = rDirty.bottom = 0;
   } else {
      if(left < (temp=rDirty.left))
         left = temp;
      if(top < (temp=rDirty.top))
         top = temp;

      if(width > (temp=rDirty.right-left)) {
         width = temp;
         if(width < 0)
            width = 0;
      }
      if(height > (temp=rDirty.bottom-top)) {
         height = temp;
         if(height < 0)
            height = 0;
      }
   }

   BitBlt(hdcImage, left, top, width, height,
	 hdcWork, left, top, SRCCOPY);

   bExchanged = FALSE;
}

void WorkImageExchange()
{
    int left, top;
    int width, height;
    HDC hDC;
    HBITMAP hbm;

    if(!hdcWork || !hdcImage)
       return;
    hbm=CreateCompatibleBitmap(hdcWork,1,1);

    hbm=SelectObject(hdcWork,hbm);
    hbm=SelectObject(hdcImage,hbm);
    hbm=SelectObject(hdcWork,hbm);

    DeleteObject(hbm);

    if(!(hDC = GetDisplayDC(pbrushWnd[PAINTid])))
       return;

    left = rDirty.left;
    top = rDirty.top;
    width = rDirty.right - rDirty.left;
    height = rDirty.bottom - rDirty.top;

    BitBlt(hDC, left - imageView.left, top - imageView.top, width, height,
	 hdcWork, left, top, SRCCOPY);

    ReleaseDC(pbrushWnd[PAINTid], hDC);

   bExchanged = !bExchanged;
}

void UndoRect(int left, int top, int width, int height)
{
   register int temp;
   HDC hDC;

   if(!hdcWork || !hdcImage)
      return;

   if((left | top | width | height) == 0) {
      left = rDirty.left;
      top = rDirty.top;
      width = rDirty.right - rDirty.left;
      height = rDirty.bottom - rDirty.top;
      rDirty.left = rDirty.top = rDirty.right = rDirty.bottom = 0;
   } else {
      if(left < (temp=rDirty.left))
         left = temp;
      if(top < (temp=rDirty.top))
         top = temp;

      if(width > (temp=rDirty.right-left)) {
         width = temp;
         if(width < 0)
            width = 0;
      }
      if(height > (temp=rDirty.bottom-top)) {
         height = temp;
         if(height < 0)
            height = 0;
      }
   }

   if(!(hDC = GetDisplayDC(pbrushWnd[PAINTid])))
      return;

//   wsprintf(acDbgBfr,"UndoRect l,t,w,h = %d,%d,%d,%d\n",
//	       left, top, width, height);
//   OutputDebugString(acDbgBfr);

   BitBlt(hDC, left - imageView.left, top - imageView.top, width, height,
	 hdcImage, left, top, SRCCOPY);
   BitBlt(hdcWork, left, top, width, height,
	 hdcImage, left, top, SRCCOPY);

   ReleaseDC(pbrushWnd[PAINTid], hDC);
}

void SuspendCopy()
{
   ++bDontCopy;
}

void ResumeCopy()
{
   --bDontCopy;
}
