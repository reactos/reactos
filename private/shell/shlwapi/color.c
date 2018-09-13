#include "priv.h"

// Copied mostly from Desk.cpl.
#define  RANGE   240            // range of values for HLS scrollbars 
                                // HLS-RGB conversions work best when
                                // RANGE is divisible by 6 
#define  HLSMAX   RANGE
#define  RGBMAX   255
#define UNDEFINED (HLSMAX*2/3)  //Hue is undefined if Saturation is 0 (grey-scale)



//-------------------------------------------------------------------------
// ColorRGBToHLS
//
// Purpose: Convert RGB to HLS
//
// A point of reference for the algorithms is Foley and Van Dam, pp. 618-19.
// Their algorithm is in floating point.  CHART implements a less general
// (hardwired ranges) integral algorithm.


// There are potential roundoff errors lurking throughout here.
//   (0.5 + x/y) without floating point,
//      (x/y) phrased ((x + (y/2))/y)
//   yields very small roundoff error.
//   This makes many of the following divisions look funny.

// H,L, and S vary over 0-HLSMAX
// R,G, and B vary over 0-RGBMAX
// HLSMAX BEST IF DIVISIBLE BY 6
// RGBMAX, HLSMAX must each fit in a byte. 

STDAPI_(void) ColorRGBToHLS(COLORREF clrRGB, WORD* pwHue, WORD* pwLuminance, WORD* pwSaturation)
{
    int R,G,B;                /* input RGB values */
    WORD cMax,cMin;        /* max and min RGB values */
    WORD cSum,cDif;
    int  Rdelta,Gdelta,Bdelta;  /* intermediate value: % of spread from max */
    int H, L, S;

    /* get R, G, and B out of DWORD */
    R = GetRValue(clrRGB);
    G = GetGValue(clrRGB);
    B = GetBValue(clrRGB);

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

   ASSERT( pwHue && pwLuminance && pwSaturation );
   *pwHue = (WORD) H;
   *pwLuminance = (WORD) L;
   *pwSaturation = (WORD) S;
}


/* utility routine for HLStoRGB */
WORD HueToRGB(WORD n1, WORD n2, WORD hue)
{

   /* range check: note values passed add/subtract thirds of range */

   /* The following is redundant for WORD (unsigned int) */

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


//-------------------------------------------------------------------------
// ColorHLSToRGB
//
// Purpose: Convert HLS to RGB

STDAPI_(COLORREF) ColorHLSToRGB(WORD wHue, WORD wLuminance, WORD wSaturation)
{
    WORD R,G,B;                      /* RGB component values */
    WORD  Magic1,Magic2;       /* calculated magic numbers (really!) */

    if (wSaturation == 0)                /* achromatic case */
    {
        R = G = B = (wLuminance * RGBMAX) / HLSMAX;
        if (wHue != UNDEFINED)
        {
            R = G = B = 0;
        }
    }
    else                         /* chromatic case */
    {
        /* set up magic numbers */
        if (wLuminance <= (HLSMAX/2))
          Magic2 = (WORD)((wLuminance * ((DWORD)HLSMAX + wSaturation) + (HLSMAX/2))/HLSMAX);
        else
          Magic2 = wLuminance + wSaturation - (WORD)(((wLuminance*wSaturation) + (DWORD)(HLSMAX/2))/HLSMAX);
        Magic1 = 2*wLuminance-Magic2;

        /* get RGB, change units from HLSMAX to RGBMAX */
        R = (WORD)((HueToRGB(Magic1,Magic2,(WORD)(wHue+(WORD)(HLSMAX/3)))*(DWORD)RGBMAX + (HLSMAX/2))) / (WORD)HLSMAX;
        G = (WORD)((HueToRGB(Magic1,Magic2,wHue)*(DWORD)RGBMAX + (HLSMAX/2))) / HLSMAX;
        B = (WORD)((HueToRGB(Magic1,Magic2,(WORD)(wHue-(WORD)(HLSMAX/3)))*(DWORD)RGBMAX + (HLSMAX/2))) / (WORD)HLSMAX;
    }

    return(RGB(R,G,B));
}


//-------------------------------------------------------------------------
// ColorAdjustLuma
//
// Purpose: Adjusts the luma of an RGB value

STDAPI_(COLORREF) ColorAdjustLuma(COLORREF clrRGB, int n, BOOL fScale)
{
    WORD H, L, S;

    if (n == 0)
        return clrRGB;

    ColorRGBToHLS(clrRGB, &H, &L, &S);

    if (fScale)
    {
        if (n > 0)
        {
            return ColorHLSToRGB((WORD)H, (WORD)(((long)L * (1000 - n) + (RANGE + 1l) * n) / 1000), (WORD)S);
        }
        else
        {
            return ColorHLSToRGB((WORD)H, (WORD)(((long)L * (n + 1000)) / 1000), (WORD)S);
        }
    }

    L += (int)((long)n * RANGE / 1000);

    if (L < 0)
        L = 0;
    if (L > HLSMAX)
        L = HLSMAX;

    return ColorHLSToRGB((WORD)H, (WORD)L, (WORD)S);
}
