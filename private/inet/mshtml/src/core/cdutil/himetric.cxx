//+---------------------------------------------------------------------
//
//  File:       himetric.cxx
//
//  Contents:   Routines to convert Pixels to Himetric and vice versa
//
//----------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

//+---------------------------------------------------------------
//
//  Function:   HimetricFromHPix
//
//  Synopsis:   Converts horizontal pixel units to himetric units.
//
//----------------------------------------------------------------

long
HimetricFromHPix(int iPix)
{
    Assert(g_sizePixelsPerInch.cx);

    return MulDivQuick(iPix, HIMETRIC_PER_INCH, g_sizePixelsPerInch.cx);
}

//+---------------------------------------------------------------
//
//  Function:   HimetricFromVPix
//
//  Synopsis:   Converts vertical pixel units to himetric units.
//
//----------------------------------------------------------------

long
HimetricFromVPix(int iPix)
{
    Assert(g_sizePixelsPerInch.cy);

    return MulDivQuick(iPix, HIMETRIC_PER_INCH, g_sizePixelsPerInch.cy);
}

//+---------------------------------------------------------------
//
//  Function:   HPixFromHimetric
//
//  Synopsis:   Converts himetric units to horizontal pixel units.
//
//----------------------------------------------------------------

int
HPixFromHimetric(long lHi)
{
    Assert(g_sizePixelsPerInch.cx);

    return MulDivQuick(g_sizePixelsPerInch.cx, lHi, HIMETRIC_PER_INCH);
}

//+---------------------------------------------------------------
//
//  Function:   VPixFromHimetric
//
//  Synopsis:   Converts himetric units to vertical pixel units.
//
//----------------------------------------------------------------

int
VPixFromHimetric(long lHi)
{
    Assert(g_sizePixelsPerInch.cy);

    return MulDivQuick(g_sizePixelsPerInch.cy, lHi, HIMETRIC_PER_INCH);
}

#ifdef PRODUCT_96
void
PixelFromHMRect(RECT *prcDest, RECTL *prcSrc)
{
    prcDest->left = HPixFromHimetric(prcSrc->left);
    prcDest->top = VPixFromHimetric(prcSrc->top);
    prcDest->right = HPixFromHimetric(prcSrc->right);
    prcDest->bottom = VPixFromHimetric(prcSrc->bottom);
}
#endif

#ifdef PRODUCT_96
void
HMFromPixelRect(RECTL *prcDest, RECT *prcSrc)
{
    prcDest->left = HimetricFromHPix(prcSrc->left);
    prcDest->top = HimetricFromVPix(prcSrc->top);
    prcDest->right = HimetricFromHPix(prcSrc->right);
    prcDest->bottom = HimetricFromVPix(prcSrc->bottom);
}
#endif


//+-------------------------------------------------------------------------
//
//  Function:   StringToHimetric
//
//  Synopsis:   Converts a numeric string with units to a himetric value.
//              Expects a NULL-terminated string.  The contents of the
//              string may be altered.
//
//              Example: "72 pt" returns 2540 and UNITS_POINT
//
//  Arguments:  [szString]  String to convert
//              [pUnits]    Returns the original units found.  NULL ok.
//              [plValue]   Resulting himetric value
//
//  BUGBUG - Is atof the right thing to use here?
//
//--------------------------------------------------------------------------

HRESULT
StringToHimetric(TCHAR * pstr, UNITS * punits, long * plValue)
{
    HRESULT hr;
    int     units;
    TCHAR * pstrT;
    float   flt;
    TCHAR   achUnits[UNITS_BUFLEN];

    *plValue = 0;

    // Convert all trailing spaces to nulls so they don't confuse
    // the units.

    for (pstrT = pstr; *pstrT; pstrT++);
    do { *pstrT-- = 0; } while (*pstrT == ' ');

    //  First, see if the user specified units in the string

    for (units = UNITS_MIN; units < UNITS_UNKNOWN; units++)
    {


        Verify(LoadString(
                    GetResourceHInst(),
                    IDS_UNITS_BASE + units,
                    achUnits,
                    ARRAY_SIZE(achUnits)));

        for (pstrT = pstr; *pstrT; pstrT++)
        {
            if (!_tcsicmp(pstrT, achUnits))
            {
                *pstrT = 0;
                goto FoundMatch;
            }
        }
    }

    //  If no units are specified, use the global default

    Assert(units == UNITS_UNKNOWN);

#if NEVER // we should use UNITS_POINT in Forms3 96.
    units = g_unitsMeasure;
#else // ! NEVER
    units = UNITS_POINT;
#endif // ! NEVER

FoundMatch:

    //  Use OleAuto to convert the string to a float; this assumes
    //    that the conversion will ignore "noise" like the units
    hr = THR(VarR4FromStr(pstr, g_lcidUserDefault, 0, &flt));
    if (hr)
        goto Cleanup;

    switch (units)
    {
    case UNITS_CM:
        *plValue = (long) (flt * 1000);
        break;

    case UNITS_UNKNOWN:
    case UNITS_POINT:
        *plValue = (long) ((flt * 2540) / 72);
        break;

    case UNITS_INCH:
        *plValue = (long) (flt * 2540);
        break;

    default:
        Assert(FALSE);
        break;
    }

Cleanup:

    *punits = (UNITS) units;

    RRETURN(hr);
}



//+-------------------------------------------------------------------------
//
//  Function:   HimetricToString
//
//  Synopsis:   Converts a himetric value to a numeric string of the
//              specified units.
//
//              Example:  2540 and UNITS_POINT returns "72 pt"
//
//  Arguments:  [lVal]      Value to convert
//              [units]     Units to convert to
//              [szRet]     Buffer for returned string
//              [cch]       Size of buffer
//
//--------------------------------------------------------------------------

HRESULT
HimetricToString(long lVal, UNITS units, TCHAR * szRet, int cch)
{
    HRESULT     hr;
    float       flt;
    BSTR        bstr;

    Assert(units == UNITS_POINT);

    flt = UserFromHimetric(lVal);

    hr = THR(VarBstrFromR4(flt, g_lcidUserDefault, 0, &bstr));
    if (hr)
        goto Cleanup;



    hr = Format(
            0,
            szRet,
            cch,
            _T("<0s> <1i>"),
            bstr,
            GetResourceHInst(), IDS_UNITS_BASE + units);

Cleanup:
    FormsFreeString(bstr);

    RRETURN(hr);
}



#if DBG == 1
BOOL
CheckFPConversion( )
{
    long    i;  // for win16 basically.
    float   xf;
    float   xf2;

    for (i = 0; i < 72 * 20; i++)
    {
        xf = i / 20.0f;
        xf2 = UserFromHimetric(HimetricFromUser(xf));

        Assert(xf == xf2);
    }

    return TRUE;
}
#endif

#ifndef WIN16
// BUGWIN16 this really asserts in WIN16 - need to figure that out.
StartupAssert(CheckFPConversion());
#endif

//+-------------------------------------------------------------------------
//
//  Function:   UserFromHimetric
//
//  Synopsis:   Converts a himetric long value to point size float.
//
//--------------------------------------------------------------------------

float
UserFromHimetric(long lValue)
{
    //  Rounds to the nearest .05pt.  This is about the maximum
    //    precision we can get keeping things in himetric internally

#if NEVER // Should not change the default unit in Forms3 96. Use UNITS_POINT.
          // Leave this code to roll back in 97.
    switch (g_unitsMeasure)
    {
    case UNITS_CM:
        return (float)lValue / (float)1000;

    case UNITS_POINT:
    default:
        return ((float) MulDivQuick(lValue, 72 * 20, 2540)) / 20;
    }

#else // ! NEVER

    return ((float) MulDivQuick(lValue, 72 * 20, 2540)) / 20;

#endif // ! NEVER

}


//+-------------------------------------------------------------------------
//
//  Function:   HimetricFromUser
//
//  Synopsis:   Converts a point size double to himetric long.  Rounds
//              to the nearest himetric value.
//
//--------------------------------------------------------------------------

long
HimetricFromUser(float xf)
{
    long lResult;

#if NEVER // Should not change the default unit in Forms3 96. Use UNITS_POINT.
          // Leave this code to roll back in 97.
    switch (g_unitsMeasure)
    {
    case UNITS_CM:
        xf = xf * (float)1000;
        break;
    case UNITS_POINT:
    default:
        xf = xf * ( ((float)2540) / 72 );
        break;
    }

#else // ! NEVER

    xf = xf * ( ((float)2540) / 72 );

#endif // ! NEVER

    if (xf > LONG_MAX)
        lResult = LONG_MAX;
    else if (xf > .0)
        lResult = long(xf + .5);
    else if (xf < LONG_MIN)
        lResult = LONG_MIN;
    else
        lResult = long(xf - .5);

    return lResult;
}
