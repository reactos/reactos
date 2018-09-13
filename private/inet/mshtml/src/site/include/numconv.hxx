//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       numconv.hxx
//
//  Contents:   Numeral String Conversions
//
//----------------------------------------------------------------------------

#ifndef I_NUMCONV_HXX_
#define I_NUMCONV_HXX_
#pragma INCMSG("--- Beg 'numconv.hxx'")

#define NUMCONV_STRLEN 18

void NumberToRomanLower(LONG n, TCHAR achBuffer[NUMCONV_STRLEN]);
void NumberToRomanUpper(LONG n, TCHAR achBuffer[NUMCONV_STRLEN]);
void NumberToAlphaLower(LONG n, TCHAR achBuffer[NUMCONV_STRLEN]);
void NumberToAlphaUpper(LONG n, TCHAR achBuffer[NUMCONV_STRLEN]);
void NumberToNumeral(LONG n, TCHAR achBuffer[NUMCONV_STRLEN]);

#pragma INCMSG("--- End 'numconv.hxx'")
#else
#pragma INCMSG("*** Dup 'numconv.hxx'")
#endif
