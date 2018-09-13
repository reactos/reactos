//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       textxfrm.hxx
//
//  Contents:   CSS Text Transform
//
//----------------------------------------------------------------------------

#ifndef I_TEXTXFRM_HXX_
#define I_TEXTXFRM_HXX_
#pragma INCMSG("--- Beg 'textxfrm.hxx'")

const TCHAR *
TransformText( 
    CStr &strDst,
    const TCHAR * pchSrc,
    LONG cchIn,
    BYTE bTextTransform,
    TCHAR chPrev = 0 );

#pragma INCMSG("--- End 'textxfrm.hxx'")
#else
#pragma INCMSG("*** Dup 'textxfrm.hxx'")
#endif
