/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#define NOLSTRING    TRUE
#include <windows.h>
#include "port1632.h"
#include "dynacomm.h"


#define ID_FLASHEVENT    1
#define FLASH_ELAPSE     450

WNDPROC lpNextFlash;

/*****************************************************************************/
/* nextFlash() -                                                             */
/*****************************************************************************/
LONG  APIENTRY nextFlash(HWND  hWnd, UINT  message, DWORD nIDEvent, LONG  sysTime)
{
   RECT  rect;
   HDC   hDC;

   rect.left   = 0;
   rect.top    = 0;
   rect.right  = 16*icon.dx;
   rect.bottom = 16*icon.dy;
   InvertRect(hDC = GetDC(hWnd), (LPRECT) &rect);
   ReleaseDC(hWnd, hDC);
}

/*****************************************************************************/
/* myDrawIcon() -                                                            */
/*****************************************************************************/

myDrawIcon(HWND hDC, BOOL bErase)
{
   INT          i, x, y;
   RECT         rect;

   DrawIcon(hDC, 0, 0, icon.hIcon);

   if((progress > 0) && !icon.flash)
   {
      i = progress-1;
      y = i/16;
      x = i%16;

      rect.left   = 0;
      rect.top    = 0;
      rect.right  = 16*icon.dx;
      if(x < 16-1)
         rect.bottom = y*icon.dy;
      else
         rect.bottom = (y+1)*icon.dy;
      InvertRect(hDC, (LPRECT) &rect);

      if(x < 16-1)
      {
         rect.left   = 0;
         rect.top    = y*icon.dy;
         rect.right  = (x+1)*icon.dx;
         rect.bottom = (y+1)*icon.dy;
         InvertRect(hDC, (LPRECT) &rect);
      }

      icon.last = progress;
   }
}


/*****************************************************************************/
/* updateIcon()                                                              */
/*****************************************************************************/

updateIcon()
{
   INT          i, x, y;
   HDC          hDC;
   RECT         rect;

   UpdateWindow(hItWnd); /* jtf 3.20 */
   hDC = GetDC(hItWnd);

   i = icon.last;
   y = i / 16;
   x = i % 16;

   while(i < progress)
   {
      rect.left   = x*icon.dx;
      rect.top    = y*icon.dy;
      rect.right  = (x+1)*icon.dx;
      rect.bottom = (y+1)*icon.dy;
      InvertRect(hDC, (LPRECT) &rect);

      i++;
      if(x < 16-1)
         x++;
      else
      {
         x = 0;
         if(y < 16-1)
            y++;
         else
            y = 0;
      }
   }

   icon.last = progress;

   ReleaseDC(hItWnd, hDC);
}


/*****************************************************************************/
/* flashIcon() -                                                             */
/*****************************************************************************/

flashIcon(BOOL  bInitFlash, BOOL  bEndProc)
{
   HDC   hDC;

   if(bEndProc)
   {
      progress = 0;
      icon.last = 0;
   }
   if (IsIconic (hItWnd))
   {
      if(icon.flash = bInitFlash)
         SetTimer(hItWnd, ID_FLASHEVENT, FLASH_ELAPSE, lpNextFlash);
      else
         KillTimer(hItWnd, ID_FLASHEVENT);
      
      myDrawIcon(hDC = GetDC(hItWnd), TRUE);
      ReleaseDC(hItWnd, hDC);
   }
}
