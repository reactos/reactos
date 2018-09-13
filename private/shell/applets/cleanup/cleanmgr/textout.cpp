/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Applet
** File:    textout.cpp
**
** Purpose: Print functions
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#include "common.h"
#include "textout.h"


/*
 * DEFINITIONS ________________________________________________________________
 *
 */

         typedef struct
            {
            HWND     hWnd;
            LONG     style;
            TCHAR    *pszText;	   // Buffer for text
            size_t   cbMaxText;	// Length of text buffer
            HFONT    hf;	      // Current font
            HBITMAP  bmp;	      // Off-screen bitmap for NO FLICKER
            RECT     rBmp;	      // Size of 'bmp'
            } TextOutInfo;


/*
 * VARIABLES __________________________________________________________________
 *
 */

         static HINSTANCE     l_hInst = NULL;


/*
 * PROTOTYPES _________________________________________________________________
 *
 */

         LRESULT APIENTRY TextOutProc         (HWND, UINT, WPARAM, LPARAM);

         void             TextOutPaint        (TextOutInfo *, HDC, RECT *);

         BOOL             TextOutMakeBitmap   (TextOutInfo *);
         BOOL             TextOutSetText      (TextOutInfo *, LPCTSTR);
         void             TextOutRedraw       (TextOutInfo *, BOOL = TRUE);


/*
 * ROUTINES ___________________________________________________________________
 *
 */
/*
void RegisterTextOutClass (HINSTANCE hInst)
{
   WNDCLASS  wc;

   if (hInst == NULL)
      return;

   l_hInst = hInst;
   

   wc.style = 0;
   wc.lpfnWndProc    = TextOutProc;
   wc.cbClsExtra     = 0;
   wc.cbWndExtra     = sizeof (void *);
   wc.hInstance      = hInst;
   wc.hIcon          = NULL;
   wc.hCursor        = LoadCursor (NULL, IDC_ARROW);
   wc.hbrBackground  = NULL;
   wc.lpszMenuName   = NULL;
   wc.lpszClassName  = szTextOutCLASS;

   RegisterClass (&wc);
}


void UnregisterTextOutClass (void)
{
	   //	Unregister from windows
   if (l_hInst)
      UnregisterClass (szTextOutCLASS, l_hInst);
   l_hInst = NULL;
}


LRESULT APIENTRY TextOutProc (HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
   TextOutInfo   *ptoi;
   PAINTSTRUCT    ps;
   HDC            hdc;

   if (msg == WM_CREATE)
      {
      if ((ptoi = (TextOutInfo *)GlobalAlloc (GMEM_FIXED, sizeof(*ptoi))) == 0)
         {
         //OutOfMemory();
         return -1;
         }
      SetWindowLongPtr (hWnd, 0, (LPARAM)ptoi);

      ptoi->hWnd        = hWnd;
      ptoi->style       = ((LPCREATESTRUCT)lp)->style;
      ptoi->pszText     = NULL;
      ptoi->cbMaxText   = 0;
      ptoi->hf          = (HFONT) GetStockObject (DEFAULT_GUI_FONT);
      ptoi->bmp         = NULL;

      if (!TextOutMakeBitmap (ptoi))
         return -1;

      if (!TextOutSetText (ptoi, ((LPCREATESTRUCT)lp)->lpszName ))
         return -1;

      TextOutRedraw (ptoi);
      }
   else if ((ptoi = (TextOutInfo *)GetWindowLongPtr (hWnd, 0)) == NULL)
      {
      return DefWindowProc (hWnd, msg, wp, lp);
      }


   switch (msg)
      {
      case WM_DESTROY:  if (ptoi->pszText != NULL)
                           GlobalFree (ptoi->pszText);
                        if (ptoi->bmp != NULL)
                           DeleteObject (ptoi->bmp);
                        GlobalFree (ptoi);
                        SetWindowLongPtr (hWnd, 0, 0L);
                       break;

      case WM_USER:     TextOutRedraw (ptoi, FALSE);
                       break;

      case WM_PAINT:    hdc = BeginPaint (hWnd, &ps);
                        TextOutPaint (ptoi, hdc, &ps.rcPaint);
                        EndPaint (hWnd, &ps);
                        return 0;
                       break;

      case WM_SETTEXT:  TextOutSetText (ptoi, (LPCTSTR)lp);
                        TextOutRedraw (ptoi);
                       break;

      case WM_GETFONT:  return (LRESULT)ptoi->hf;
                       break;

      case WM_SETFONT:  if ((ptoi->hf = (HFONT)wp) == NULL)
                           {
                           ptoi->hf = (HFONT) GetStockObject (DEFAULT_GUI_FONT);
                           }
                        if (LOWORD(lp) != 0)
                           {
                           TextOutRedraw (ptoi);
                           }
                       break;

      case WM_SIZE:     TextOutMakeBitmap (ptoi);
                       break;

      case WM_ERASEBKGND:
                        return (LRESULT)1;
                       break;
      }

   return DefWindowProc (hWnd, msg, wp, lp);
}


void TextOutPaint (TextOutInfo *ptoi, HDC hdcTrg, RECT *pr)
{
   RECT     r;
   HDC      hdcSrc;
   HBITMAP  bmpSrc;


   if (ptoi->bmp == NULL)	// No bitmap?
      return;	// No paint.

   if (pr == NULL)
      {
      GetClientRect (ptoi->hWnd, pr = &r);
      }

   hdcSrc = CreateCompatibleDC (hdcTrg);
   bmpSrc = (HBITMAP) SelectObject (hdcSrc, (HGDIOBJ)ptoi->bmp);

   BitBlt (hdcTrg, pr->left, pr->top, pr->right -pr->left, pr->bottom -pr->top,
           hdcSrc, pr->left, pr->top, SRCCOPY);

   SelectObject (hdcSrc, (HGDIOBJ)bmpSrc);
   DeleteDC (hdcSrc);
}


BOOL TextOutMakeBitmap (TextOutInfo *ptoi)
{
   HDC      hdc, hdcMem;
   RECT     r;

   GetClientRect (ptoi->hWnd, &r);

   if (ptoi->bmp != NULL)
      {
      if ( ((ptoi->rBmp.right - ptoi->rBmp.left) > (r.right - r.left)) ||
           ((ptoi->rBmp.bottom - ptoi->rBmp.top) > (r.bottom - r.top)) )
         {
         DeleteObject (ptoi->bmp);
         ptoi->bmp = NULL;
         }
      }

   if (ptoi->bmp == NULL)
      {
      hdc = GetDC (ptoi->hWnd);
      hdcMem = CreateCompatibleDC (hdc);

      ptoi->bmp = CreateCompatibleBitmap (hdc, r.right, r.bottom);
      ptoi->rBmp = r;

      DeleteDC (hdcMem);
      ReleaseDC (ptoi->hWnd, hdc);
      }

   if (ptoi->bmp == NULL)
      {
      //OutOfMemory();
      return FALSE;
      }

   return TRUE;
}


BOOL TextOutSetText (TextOutInfo *ptoi, LPCTSTR psz)
{
   size_t  cb;

   if (psz == NULL)
      {
      if (ptoi->pszText != NULL)
         ptoi->pszText[0] = 0;
      return TRUE;
      }

   cb = 1+ lstrlen(psz);
   if (cb > (ptoi->cbMaxText))
      {
      if (ptoi->pszText != NULL)
         {
         GlobalFree (ptoi->pszText);
         ptoi->pszText = NULL;
         }

      if ((ptoi->pszText = (TCHAR *)GlobalAlloc (GMEM_FIXED, cb * sizeof( TCHAR ))) == NULL)
         {
         ptoi->cbMaxText = 0;
         //OutOfMemory();
         return FALSE;
         }

      ptoi->cbMaxText = cb;
      }

   StrCpy(ptoi->pszText, psz);
   return TRUE;
}


void TextOutRedraw (TextOutInfo *ptoi, BOOL fRepaint)
{
   HBRUSH   hbr;
   HDC      hdc, hdcMem;
   HBITMAP  bmpOld;
   RECT     r;
   WPARAM   wp;
   LPARAM   lp;
   HFONT    hfOld = NULL;

   if (ptoi->bmp == NULL)
      return;

   hdc = GetDC (ptoi->hWnd);
   hdcMem = CreateCompatibleDC (hdc);
   bmpOld = (HBITMAP) SelectObject (hdcMem, (HGDIOBJ)ptoi->bmp);

   wp = (WPARAM)hdcMem;
   lp = (LPARAM)ptoi->hWnd;
   hbr = (HBRUSH)SendMessage (GetParent(ptoi->hWnd), WM_CTLCOLORSTATIC, wp,lp);

   if (hbr == NULL)
      {
      SetTextColor (hdc, GetSysColor (COLOR_BTNTEXT));
      SetBkColor (hdc, GetSysColor (COLOR_BTNFACE));
      hbr = CreateSolidBrush (GetSysColor (COLOR_BTNFACE));
      }


   GetClientRect (ptoi->hWnd, &r);
   FillRect (hdcMem, &r, hbr);

   if (ptoi->pszText != NULL)
      {
      if (ptoi->hf != NULL)
         hfOld = (HFONT) SelectObject (hdcMem, ptoi->hf);

      UINT fDrawFlags = DT_EXPANDTABS |  DT_WORDBREAK;
      if (ptoi->style & SS_RIGHT)
         fDrawFlags |= DT_RIGHT;
      else if (ptoi->style & SS_CENTER)
         fDrawFlags |= DT_CENTER;
      else
         fDrawFlags |= DT_LEFT;

      DrawText (  hdcMem,
                  ptoi->pszText,
                  lstrlen(ptoi->pszText),
                  &r,
                  fDrawFlags ); 
       
      if (hfOld != NULL)
         SelectObject (hdcMem, hfOld);
      }


   DeleteObject (hbr);

   SelectObject (hdcMem, bmpOld);
   DeleteDC (hdcMem);
   ReleaseDC (ptoi->hWnd, hdc);

   if (fRepaint)
      {
      InvalidateRect (ptoi->hWnd, NULL, TRUE);
      UpdateWindow (ptoi->hWnd);
      }
}
*/