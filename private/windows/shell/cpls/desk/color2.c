/*  COLOR2.C
**
**  Copyright (C) Microsoft, 1993, All Rights Reserved.
**
**
**  History:
**
*/
#include <windows.h>
#include "desk.h"
#include "look.h"

int H,L,S;                         /* Hue, Lightness, Saturation */
#define  RANGE   240                 /* range of values for HLS scrollbars */
                                     /* HLS-RGB conversions work best when
                                        RANGE is divisible by 6 */
#define  HLSMAX   RANGE
#define  RGBMAX   255
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* Color conversion routines --

   RGBtoHLS() takes a DWORD RGB value, translates it to HLS, and stores the
   results in the global vars H, L, and S.  HLStoRGB takes the current values
   of H, L, and S and returns the equivalent value in an RGB DWORD.  The vars
   H, L and S are written to only by 1) RGBtoHLS (initialization) or 2) the
   scrollbar handlers.

   A point of reference for the algorithms is Foley and Van Dam, pp. 618-19.
   Their algorithm is in floating point.  CHART implements a less general
   (hardwired ranges) integral algorithm.

*/

/* There are potential roundoff errors lurking throughout here.
   (0.5 + x/y) without floating point,
      (x/y) phrased ((x + (y/2))/y)
   yields very small roundoff error.
   This makes many of the following divisions look funny.
*/

                        /* H,L, and S vary over 0-HLSMAX */
                        /* R,G, and B vary over 0-RGBMAX */
                        /* HLSMAX BEST IF DIVISIBLE BY 6 */
                        /* RGBMAX, HLSMAX must each fit in a byte. */

#define UNDEFINED (HLSMAX*2/3)/* Hue is undefined if Saturation is 0 (grey-scale) */
                           /* This value determines where the Hue scrollbar is */
                           /* initially set for achromatic colors */

void   RGBtoHLS(DWORD lRGBColor)
{
   int R,G,B;                /* input RGB values */
   WORD cMax,cMin;        /* max and min RGB values */
   WORD cSum,cDif;
   int  Rdelta,Gdelta,Bdelta;  /* intermediate value: % of spread from max */

   /* get R, G, and B out of DWORD */
   R = GetRValue(lRGBColor);
   G = GetGValue(lRGBColor);
   B = GetBValue(lRGBColor);

   /* calculate lightness */
   cMax = max( max(R,G), B);
   cMin = min( min(R,G), B);
   cSum = cMax + cMin;
   L = (WORD)(((cSum * (DWORD)HLSMAX) + RGBMAX )/(2*RGBMAX));

   cDif = cMax - cMin;
   if (!cDif)   	/* r=g=b --> achromatic case */
     {
       S = 0;                         /* saturation */
       H = UNDEFINED;                 /* hue */
     }
   else                           /* chromatic case */
     {
       /* saturation */
       if (L <= (HLSMAX/2))
           S = (WORD) (((cDif * (DWORD) HLSMAX) + (cSum / 2) ) / cSum);
       else
           S = (WORD) ((DWORD) ((cDif * (DWORD) HLSMAX) + (DWORD)((2*RGBMAX-cSum)/2) )
                                                 / (2*RGBMAX-cSum));
      /* hue */
      Rdelta = (int) (( ((cMax-R)*(DWORD)(HLSMAX/6)) + (cDif / 2) ) / cDif);
      Gdelta = (int) (( ((cMax-G)*(DWORD)(HLSMAX/6)) + (cDif / 2) ) / cDif);
      Bdelta = (int) (( ((cMax-B)*(DWORD)(HLSMAX/6)) + (cDif / 2) ) / cDif);

      if ((WORD) R == cMax)
         H = Bdelta - Gdelta;
      else if ((WORD) G == cMax)
         H = (HLSMAX/3) + Rdelta - Bdelta;
      else /* B == cMax */
         H = ((2*HLSMAX)/3) + Gdelta - Rdelta;

      if (H < 0)
         H += HLSMAX;
      if (H > HLSMAX)
         H -= HLSMAX;
   }
}

/* utility routine for HLStoRGB */
WORD NEAR PASCAL HueToRGB(WORD n1, WORD n2, WORD hue)
{

   /* range check: note values passed add/subtract thirds of range */

   /* The following is redundant for WORD (unsigned int) */

#if 0
   if (hue < 0)
      hue += HLSMAX;
#endif

   if (hue > HLSMAX)
      hue -= HLSMAX;

   /* return r,g, or b value from this tridrant */
   if (hue < (HLSMAX/6))
      return ( n1 + (((n2-n1)*hue+(HLSMAX/12))/(HLSMAX/6)) );
   if (hue < (HLSMAX/2))
      return ( n2 );
   if (hue < ((HLSMAX*2)/3))
      return ( n1 + (((n2-n1)*(((HLSMAX*2)/3)-hue)+(HLSMAX/12)) / (HLSMAX/6)) );
   else
      return ( n1 );
}


DWORD NEAR PASCAL HLStoRGB(WORD hue, WORD lum, WORD sat)
{
  WORD R,G,B;                      /* RGB component values */
  WORD  Magic1,Magic2;       /* calculated magic numbers (really!) */

  if (sat == 0)                /* achromatic case */
    {
      R = G = B = (lum * RGBMAX) / HLSMAX;
      if (hue != UNDEFINED)
        {
         /* ERROR */
        }
    }
  else                         /* chromatic case */
    {
      /* set up magic numbers */
      if (lum <= (HLSMAX/2))
          Magic2 = (WORD)((lum * ((DWORD)HLSMAX + sat) + (HLSMAX/2))/HLSMAX);
      else
          Magic2 = lum + sat - (WORD)(((lum*sat) + (DWORD)(HLSMAX/2))/HLSMAX);
      Magic1 = 2*lum-Magic2;

      /* get RGB, change units from HLSMAX to RGBMAX */
      R = (WORD)((HueToRGB(Magic1,Magic2,(WORD)(hue+(WORD)(HLSMAX/3)))*(DWORD)RGBMAX + (HLSMAX/2))) / (WORD)HLSMAX;
      G = (WORD)((HueToRGB(Magic1,Magic2,hue)*(DWORD)RGBMAX + (HLSMAX/2))) / HLSMAX;
      B = (WORD)((HueToRGB(Magic1,Magic2,(WORD)(hue-(WORD)(HLSMAX/3)))*(DWORD)RGBMAX + (HLSMAX/2))) / (WORD)HLSMAX;
    }
  return(RGB(R,G,B));
}


DWORD  DarkenColor(DWORD rgb, int n)
{
    RGBtoHLS(rgb);
    return HLStoRGB((WORD)H, (WORD)((long)L * n / 1000), (WORD)S);
}

DWORD  BrightenColor(DWORD rgb, int n)
{
    RGBtoHLS(rgb);
    return HLStoRGB((WORD)H, (WORD)(((long)L * (1000-n) + (RANGE+1l)*n) / 1000), (WORD)S);
}

#ifdef OLDCODE
void  Get3DColors(DWORD rgbFace, LPDWORD lprgbShadow, LPDWORD lprgbHilight)
{
    RGBtoHLS(rgbFace);

    // colors are half way between the face and the min/max values

    *lprgbShadow  = HLStoRGB(H, L * 2 / 3, S);
    *lprgbHilight = HLStoRGB(H, (L + RANGE + 1) / 2, S);
    *lprgbHilight = HLStoRGB(H, (L + RANGE + 1) / 2, S);

//    *lprgbHilight = HLStoRGB(H, MIN(RANGE, L * 7 / 6), S);
}
#endif
