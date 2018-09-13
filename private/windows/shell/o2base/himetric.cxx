//+---------------------------------------------------------------------
//
//  File:       himetric.cxx
//
//  Contents:   Routines to convert Pixels to Himetric and vice versa
//
//----------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define HIMETRIC_PER_INCH 2540L

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
   HDC hdc = GetDC(NULL);
   int iPPLI = GetDeviceCaps(hdc, LOGPIXELSX);
   ReleaseDC(NULL, hdc);

   return (HIMETRIC_PER_INCH * (long)iPix)/(long)iPPLI;
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
   HDC hdc = GetDC(NULL);
   int iPPLI = GetDeviceCaps(hdc, LOGPIXELSY);
   ReleaseDC(NULL, hdc);
   return (HIMETRIC_PER_INCH * (long)iPix)/(long)iPPLI;
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
   HDC hdc = GetDC(NULL);
   int iPPLI = GetDeviceCaps(hdc, LOGPIXELSX);
   ReleaseDC(NULL, hdc);
   return (int)(( (iPPLI * lHi) + (HIMETRIC_PER_INCH / 2) )/HIMETRIC_PER_INCH);
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
   HDC hdc = GetDC(NULL);
   int iPPLI = GetDeviceCaps(hdc, LOGPIXELSY);
   ReleaseDC(NULL, hdc);
   return (int)(( (iPPLI * lHi) + (HIMETRIC_PER_INCH / 2) )/HIMETRIC_PER_INCH);
}

