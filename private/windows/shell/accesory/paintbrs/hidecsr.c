/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   HideCsr.c                                   *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  hides paint window's cursor                 *
*   date:   03/06/87 @ 15:45                            *
*                                                       *
********************************************************/

#include "onlypbr.h"

#include <windows.h>
#include "port1632.h"
//#define NOEXTERN
#include "pbrush.h"

void HideCsr(HDC dc, HWND hWnd, int csr)
{
   if(CursorStat.allowed)
      return;

   if(!drawing)
      dc = GetDisplayDC(hWnd);
   XorCsr(dc, csrPt, csr);
   csrPt.x = csrPt.y = -1;
   if(!drawing)
      ReleaseDC(hWnd, dc);
}
