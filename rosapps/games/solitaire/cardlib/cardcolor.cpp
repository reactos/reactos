//
//    Colour support
//
#include <windows.h>

#define MakeRGB RGB

#define MIN3(a,b,c) ( (a)<=(b) ? (a)<=(c)?(a):(c) : (b)<=(c)?(b):(c) )
#define MAX3(a,b,c) ( (a)>=(b) ? (a)>=(c)?(a):(c) : (b)>=(c)?(b):(c) )

inline double fMax(double a, double b)
{
    return a < b ? b : a;
}

inline double fMin(double a, double b)
{
    return a < b ? a : b;
}
/******************************************************************************
  FUNCTION: RGBtoHLS
  PURPOSE:     Convert from RGB to HLS
  IN: RGB color (0xBBGGRR)
  OUT: Hue, Saturation, Luminance from 0 to 1
  COPYRIGHT:1995-1997 Robert Mashlan
            Modified for LabWindows/CVI, 1999 Guillaume Dargaud
******************************************************************************/
void RGBtoHLS(const COLORREF rgb, double *H, double *L, double *S ) 
{
    double delta;
    double r = (double)((rgb    )&0xFF)/255;
    double g = (double)((rgb>> 8)&0xFF)/255;
    double b = (double)((rgb>>16)&0xFF)/255;
    double cmax = MAX3(r,g,b);
    double cmin = MIN3(r,g,b);
    *L=(cmax+cmin)/2.0;
    
    if(cmax == cmin) 
    {
        *S = *H = 0; // it's really undefined
    }
    else 
    {
        if(*L < 0.5)    *S = (cmax-cmin)/(cmax+cmin);
        else            *S = (cmax-cmin)/(2.0-cmax-cmin);
        
        delta = cmax - cmin;
        
        if(r == cmax) 
        {
            *H = (g - b) / delta;
        }
        else
        {
            if(g == cmax) *H = 2.0 + (b-r) / delta;
              else          *H = 4.0 + (r-g) / delta;
        }
          *H /= 6.0;
          if (*H < 0.0) *H += 1;
    }
}

/******************************************************************************
  FUNCTION: HueToRGB
  PURPOSE:     Convert a hue (color) to RGB
  COPYRIGHT:1995-1997 Robert Mashlan
            Modified for LabWindows/CVI, 1999 Guillaume Dargaud
******************************************************************************/
double HueToRGB(const double m1, const double m2, double h ) 
{
    if (h<0) h+=1.0;
    if (h>1) h-=1.0;
    if (6.0*h < 1  ) return (m1+(m2-m1)*h*6.0);
    if (2.0*h < 1  ) return m2;
    if (3.0*h < 2.0) return (m1+(m2-m1)*((2.0/3.0)-h)*6.0);
    return m1;
}


/******************************************************************************
  FUNCTION: HLStoRGB
  PURPOSE:     Convert from HSL to RGB
  IN:         Hue, Saturation, Luminance from 0 to 1
  RETURN:     RGB color (0xBBGGRR)
  COPYRIGHT:1995-1997 Robert Mashlan
            Modified for LabWindows/CVI, 1999 Guillaume Dargaud
******************************************************************************/

COLORREF HLStoRGB(const double H, const double L, const double S ) 
{
    double r,g,b;
    double m1, m2;

    if(S == 0) 
    {
        r = g = b = L;
    }
    else 
    {
        if (L <= 0.5) 
            m2 = L * (1.0 + S);
        else         
            m2 = L + S - L * S;

        m1 = 2.0 * L - m2;

        r = HueToRGB(m1,m2,H+1.0/3.0);
        g = HueToRGB(m1,m2,H);
        b = HueToRGB(m1,m2,H-1.0/3.0);
    }
  
    return RGB(r*255, g*255, b*255);
}



/******************************************************************************
  FUNCTION: ColorScaleHSL
  PURPOSE:     Returns the HSL linear interpolated color between 2 colors
            (more natural looking than RGB interpolation)
               For instance if the luminance is the same in Col1 and Col2, 
                   then the luminance of the result will be the same
               If Ratio=0, you get Col1,
             If Ratio=1, you get Col2
  IN: Col1: low color in hex 0xBBGGRR format
        Col2: high color in hex 0xBBGGRR format
        Ratio: 0 for low color, 1 for high color, or in between
  EXAMPLE: Col1=0, Col2=0xFF00FF, Ratio=0.5 returns 0x1F5F3F
******************************************************************************/
COLORREF ColorScaleHSL(    const COLORREF Col1, const COLORREF Col2, const double Ratio) 
{
    static double H1, H2, S1, S2, L1, L2;
    
    if (Ratio<=0) return Col1;    // Ratio parameter must be between 0 and 1
    else if (Ratio>=1) return Col2;

    RGBtoHLS( Col1, &H1, &L1, &S1);
    RGBtoHLS( Col2, &H2, &L2, &S2);
    return HLStoRGB( H1+(H2-H1)*Ratio, L1+(L2-L1)*Ratio, S1+(S2-S1)*Ratio );
}


/******************************************************************************
  FUNCTION: ColorScaleRGB
  PURPOSE:     Returns the RGB linear interpolated color between 2 colors
               If Ratio=0, you get Col1,
             If Ratio=1, you get Col2
  IN: Col1: low color in hex 0xBBGGRR format
        Col2: high color in hex 0xBBGGRR format
        Ratio: 0 for low color, 1 for high color, or in between
  EXAMPLE: Col1=0, Col2=0xFF00FF, Ratio=0.5 returns 0x800080
******************************************************************************/
COLORREF ColorScaleRGB(    const COLORREF Col1, 
                        const COLORREF Col2, 
                        const double Ratio) {
    int R1=(Col1)&0xFF, G1=(Col1>>8)&0xFF, B1=(Col1>>16)&0xFF;
    int R2=(Col2)&0xFF, G2=(Col2>>8)&0xFF, B2=(Col2>>16)&0xFF;

    if (Ratio<=0) return Col1;    // Ratio parameter must be between 0 and 1
    else if (Ratio>=1) return Col2;

/*    return RGB(
               (R1 + (R2 - R1) * (Ratio + 0.02) + 0.5),        // rounding
               (G1 + (G2 - G1) * (Ratio - 0.00) + 0.5),
               (B1 + (B2 - B1) * (Ratio + 0.05) + 0.5)
               );*/

    /*double r = Ratio;
    if(Col2 == 0)
        r = 1-Ratio;
    else
        r = 1+Ratio;
    R1 = (int)(double(R1) * r + 0.5);
    G1 = (int)(double(G1) * r + 0.5);
    B1 = (int)(double(B1) * r + 0.5);
    return RGB(R1,G1,B1);*/

    return RGB(
               (R1 + (R2 - R1) * (Ratio + 0.02) + 0.5),        // rounding
               (G1 + (G2 - G1) * (Ratio - 0.00) + 0.5),
               (B1 + (B2 - B1) * (Ratio + 0.05) + 0.5)
               );
}



COLORREF ColorDarker(COLORREF col, double ratio)
{
    return ColorScaleHSL(col, RGB(0,0,0), ratio);
}

COLORREF ColorLighter(COLORREF col, double ratio)
{
    return ColorScaleHSL(col, RGB(255,255,255), ratio);
}

//
//    Experimental!!!
//
#if 0

typedef enum { Red, Green, Blue };

void RGBtoHLS(COLORREF rgb, double *Hue, double *Lum, double *Sat)
{
    double mn, mx;
    int major;

    BYTE red, green, blue;

    red    = GetRValue(rgb);
    green  = GetGValue(rgb);
    blue   = GetBValue(rgb);

    if(red < green)
    {
        mn = red; mx = green; major = Green;
    }
    else
    {
        mn = green; mx = red; major = Red;
    }

    if(blue < mn)
    {
        mn = blue;
    }
    else if(blue > mx)
    {
        mx = blue; major = Blue;
    }

    if(mn == mx)
    {
        *Lum = mn / 255;
        *Sat = 0;
        *Hue = 0;
    }
    else
    {
        *Lum = (mx + mn) / 510;

        if(*Lum <= 0.5)
            *Sat = (mx-mn) / (mn+mx);
        else
            *Sat = (mx-mn) / (510-mn-mx);

        switch(major)
        {
        case Red:   *Hue = (green-blue) * 60.0 / (mx-mn) + 360.0;
                    break;

        case Green: *Hue = (blue-red) * 60.0 / (mx-mn) + 120.0;
                    break;

        case Blue:  *Hue = (red-green) * 60.0 / (mx-mn) + 240.0;
                    break;

        }

        if(*Hue > 360.0)
            *Hue -= 360.0;
    }
}

static BYTE Value(double m1, double m2, double hue)
{

    if(hue > 360)        hue -= 360;
    else if(hue < 0)    hue += 360;

    if(hue < 60)
        m1 = m1 + (m2 - m1) * hue / 60;
    else if(hue < 180)
        m1 = m2;
    else if(hue < 240)
        m1 = m1 + (m2 - m1) * (240 - hue) / 60;

    return (BYTE)(m1 * 255);
}

COLORREF HLStoRGB(const double Hue, const double Lum, const double Sat)
{
    BYTE red, green, blue;

    if(Sat == 0)
    {
        red = green = blue = (BYTE)(Lum * 255);        
    }
    else
    {
        double m1, m2;

        if(Lum <= 0.5)
            m2 = Lum + Lum * Sat;
        else
            m2 = Lum + Sat - Lum * Sat;

        m1 = 2 * Lum - m2;

        red   = Value(m1, m2, Hue + 120);
        green = Value(m1, m2, Hue);
        blue  = Value(m1, m2, Hue - 120);
    }

    return RGB(red, green, blue);
}

COLORREF ScaleLumRGB(COLORREF col1, double ratio)
{
    double H1, L1, S1;

    RGBtoHLS(col1, &H1, &L1, &S1);

    L1 += L1 * ratio;

    return HLStoRGB(H1, L1, S1);
}

COLORREF ColorScaleHSL(const COLORREF Col1, const COLORREF Col2, const double Ratio) 
{
    static double H1, H2, S1, S2, L1, L2;
    
    if(Ratio <= 0)        return Col1;    // Ratio parameter must be between 0 and 1
    else if(Ratio >= 1)    return Col2;

    RGBtoHLS( Col1, &H1, &L1, &S1);
    RGBtoHLS( Col2, &H2, &L2, &S2);

    return HLStoRGB( H1 /*+ (H2 - H1 ) * Ratio*/, L1 + (L2 - L1) * Ratio, S1 + (S2 - S1) * Ratio * 2);
}

COLORREF ColorScaleRGB(const COLORREF Col1, const COLORREF Col2, const double Ratio)
{
    return ColorScaleHSL(Col1, Col2, Ratio);
}
#endif
