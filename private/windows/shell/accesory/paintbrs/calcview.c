/****************************Module*Header******************************\
* Copyright (c) 1987 - 1991  Microsoft Corporation                      *
\***********************************************************************/
/********************************************************
*                                                       *
*   file:   CalcView.c                                  *
*   system: PC Paintbrush for MS-Windows                *
*   descr:  calculates view variables                   *
*   date:   03/10/87 @ 10:45                            *
*                                                       *
********************************************************/

#include "onlypbr.h"
#undef NOMINMAX

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"

extern RECT imageView;
extern POINT viewExt, viewOrg;
extern int paintWid, paintHgt;
extern int imageWid, imageHgt;
extern int zoomAmount, zoomWid, zoomHgt;

void CalcView()
{
   zoomAmount = max(1, zoomAmount);
   zoomWid = (paintWid + zoomAmount - 1) / zoomAmount;
   zoomHgt = (paintHgt + zoomAmount - 1) / zoomAmount;
   viewExt.x = zoomWid + zoomAmount - (zoomWid % zoomAmount);
   viewExt.y = zoomHgt + zoomAmount - (zoomHgt % zoomAmount);
   viewOrg.x = (viewExt.x - zoomWid) >> 1;
   viewOrg.y = (viewExt.y - zoomHgt) >> 1;

   if((imageView.right = imageView.left + paintWid) > imageWid) {
      imageView.left = max(0, imageWid - paintWid);
      imageView.right = imageWid;
   }
   if((imageView.bottom = imageView.top + paintHgt) > imageHgt) {
      imageView.top = max(0, imageHgt - paintHgt);
      imageView.bottom = imageHgt;
   }

}
