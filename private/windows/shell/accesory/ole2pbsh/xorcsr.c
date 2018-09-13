/****************************Module*Header******************************\
* Module Name: xorcsr.c                                                 *
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

#include <windows.h>
#include "port1632.h"
#include "pbrush.h"

extern struct csstat CursorStat;
extern int theSize;
extern int zoomWid, zoomHgt;
extern int imagePlanes, imagePixels;

void XorCsr(HDC dc, POINT pt, int type)
{
   int halfWid, halfHgt, quarterWid, quarterHgt, wid, hgt, oldDC;

   if (CursorStat.allowed)
       return;

   if (pt.x >= 0 && pt.y >= 0) {
       oldDC = SaveDC(dc);
       SetROP2(dc, R2_XORPEN);
       GetAspct(theSize, &wid, &hgt);
       if (wid < 2)
           wid = 2;
       if (hgt < 2)
           hgt = 2;

       switch (type) {
           case RECTcsr:
           case OVALcsr:
               SelectObject(dc, GetStockObject(NULL_PEN));
               SelectObject(dc, GetStockObject(WHITE_BRUSH));
               if (type == RECTcsr)
                   Rectangle(dc, pt.x, pt.y, pt.x + wid + 1, pt.y + hgt + 1);
               else
                   Ellipse(dc, pt.x, pt.y - 1, pt.x + wid, pt.y + hgt + 1);
               break;

           case HORZcsr:
               SelectObject(dc, GetStockObject(WHITE_PEN));
	       MMoveTo(dc, pt.x, pt.y);
               LineTo(dc, pt.x + wid, pt.y);
               break;

           case VERTcsr:
               SelectObject(dc, GetStockObject(WHITE_PEN));
	       MMoveTo(dc, pt.x, pt.y);
               LineTo(dc, pt.x, pt.y + hgt);
               break;

           case SLANTLcsr:
               SelectObject(dc, GetStockObject(WHITE_PEN));
	       MMoveTo(dc, pt.x + 1, pt.y + hgt - 1);
               LineTo(dc, pt.x + hgt + 1, pt.y - 1);
               break;

           case SLANTRcsr:
               SelectObject(dc, GetStockObject(WHITE_PEN));
	       MMoveTo(dc, pt.x, pt.y);
               LineTo(dc, pt.x + hgt, pt.y + hgt);
               break;

           case CROSScsr:
               SelectObject(dc, GetStockObject(WHITE_PEN));
               halfWid = GetSystemMetrics(SM_CXCURSOR) >> 1;
               halfHgt = GetSystemMetrics(SM_CYCURSOR) >> 1;
	       MMoveTo(dc, pt.x - halfWid, pt.y);
               LineTo(dc, pt.x + halfWid, pt.y);
	       MMoveTo(dc, pt.x, pt.y - halfHgt);
               LineTo(dc, pt.x, pt.y + halfHgt);
               break;

           case BOXCROSScsr:
           case BOXXcsr:
               SelectObject(dc, GetStockObject(WHITE_PEN));
               halfWid = GetSystemMetrics(SM_CXCURSOR) >> 2;
               halfHgt = GetSystemMetrics(SM_CYCURSOR) >> 2;
               if (type == BOXXcsr) {
                   quarterWid = halfWid >> 1;
                   quarterHgt = halfHgt >> 1;
		   MMoveTo(dc, pt.x - quarterWid, pt.y - quarterHgt);
                   LineTo(dc, pt.x + quarterWid, pt.y + quarterHgt);
		   MMoveTo(dc, pt.x + quarterWid, pt.y - quarterHgt);
                   LineTo(dc, pt.x - quarterWid, pt.y + quarterHgt);
               } else {
		   MMoveTo(dc, pt.x - halfWid, pt.y);
                   LineTo(dc, pt.x + halfWid, pt.y);
		   MMoveTo(dc, pt.x, pt.y - halfHgt);
                   LineTo(dc, pt.x, pt.y + halfHgt);
               }   

               if (type == CROSScsr)
                   break;

               /* FALL THROUGH to draw box outline */

           case BOXcsr:
               SelectObject(dc, GetStockObject(WHITE_PEN));
               SelectObject(dc, GetStockObject(NULL_BRUSH));
               GetAspct(-((theSize + 1) << 2), &wid, &hgt);
               halfWid = wid >> 1;
               halfHgt = hgt >> 1;
               Rectangle(dc, pt.x - halfWid, pt.y - halfHgt,
                             pt.x - halfWid + wid, pt.y - halfHgt + hgt);
               break;

           case ROLLERcsr:
               SelectObject(dc, GetStockObject(WHITE_PEN));
               halfWid = GetSystemMetrics(SM_CXCURSOR) >> 2;
               halfHgt = GetSystemMetrics(SM_CYCURSOR) >> 2;
	       MMoveTo(dc, pt.x, pt.y);
               LineTo(dc, pt.x + halfWid, pt.y - halfHgt);
               SelectObject(dc, GetStockObject(WHITE_BRUSH));
               Rectangle(dc, pt.x + halfWid, pt.y - (halfHgt << 1), 
                             pt.x + (halfWid << 1), pt.y - halfHgt);
               break;

           case IBEAMcsr:
               SelectObject(dc, GetStockObject(WHITE_PEN));
               halfWid = GetSystemMetrics(SM_CXCURSOR) >> 1;
               halfHgt = GetSystemMetrics(SM_CYCURSOR) >> 1;
               quarterWid = halfWid >> 2;
               quarterHgt = halfHgt >> 2;

	       MMoveTo(dc, pt.x, pt.y - halfHgt + quarterHgt);
               LineTo(dc, pt.x, pt.y - 1       + quarterHgt);

	       MMoveTo(dc, pt.x - quarterWid, pt.y - halfHgt - 1 + quarterHgt);
               LineTo(dc, pt.x             , pt.y - halfHgt - 1 + quarterHgt);
	       MMoveTo(dc, pt.x + quarterWid, pt.y - halfHgt - 1 + quarterHgt);
               LineTo(dc, pt.x             , pt.y - halfHgt - 1 + quarterHgt);
	       MMoveTo(dc, pt.x - quarterWid, pt.y	     - 1 + quarterHgt);
               LineTo(dc, pt.x             , pt.y           - 1 + quarterHgt);
	       MMoveTo(dc, pt.x + quarterWid, pt.y	     - 1 + quarterHgt);
               LineTo(dc, pt.x             , pt.y           - 1 + quarterHgt);
               break;

           case ZOOMINcsr:
               SelectObject(dc, GetStockObject(WHITE_PEN));
               SelectObject(dc, GetStockObject(NULL_BRUSH));
               Rectangle(dc, pt.x, pt.y, pt.x + zoomWid, pt.y + zoomHgt);
               break;
       }
       RestoreDC(dc, oldDC);
   }
}

int SizeCsr(int wtool, int wbrush)
{
   int maxx, maxy;
   int hgt, wid;

   maxx = GetSystemMetrics(SM_CXCURSOR);
   maxy = GetSystemMetrics(SM_CYCURSOR);

   GetAspct(theSize << 1, &wid, &hgt);

   if (wid < 2)
       wid = 2;
   if (hgt < 2)
       hgt = 2;

   switch (wtool) {
       case SCISSORStool:
       case PICKtool:
       case AIRBRUSHtool:
       case TEXTtool:
       case ROLLERtool:
       case CURVEtool:
       case LINEtool:
       case RECTFRAMEtool:
       case RECTFILLtool:
       case RNDRECTFRAMEtool:
       case RNDRECTFILLtool:
       case OVALFRAMEtool:
       case OVALFILLtool:
       case POLYFRAMEtool:
       case POLYFILLtool:
           return TRUE;   /* always fits  */

       case ZOOMINtool:
            // There is no code to correctly create a rectangular cursor
            // on the fly, so always draw it by hand.
            //  26-Jan-1994 JonPa
            return FALSE;

       case BRUSHtool:
           switch (wbrush) {
               case OVALcsr:
               case SLANTLcsr:
               case SLANTRcsr:
               case RECTcsr:
                   return ((hgt < maxy) && (wid < maxx));

               case HORZcsr:
                   return (wid < maxx);

               case VERTcsr:
                   return (hgt < maxy);
           }
           return TRUE;

       case ERASERtool:
       case COLORERASERtool:
       case LCUNDOtool:
	   GetAspct(-((theSize + 1) << 2), &wid, &hgt);
           return ((hgt < maxy) && (wid < maxx));

       default: 
           break;
   }

   return FALSE;
}
