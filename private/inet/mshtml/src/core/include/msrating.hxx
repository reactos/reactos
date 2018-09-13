//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       msrating.hxx
//
//  Contents:   Dynamic wrappers for msrating functions.
//
//----------------------------------------------------------------------------

#ifndef I_MSRATING_HXX_
#define I_MSRATING_HXX_
#pragma INCMSG("--- Beg 'msrating.hxx'")

#ifdef UNIX
#define AreRatingsEnabled() S_FALSE
#else
STDAPI AreRatingsEnabled();
#endif

#pragma INCMSG("--- End 'msrating.hxx'")
#else
#pragma INCMSG("*** Dup 'msrating.hxx'")
#endif
