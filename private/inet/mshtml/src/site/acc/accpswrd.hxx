//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccPswrd.hxx
//
//  Contents:   Accessible password object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCPSWRD_HXX_
#define I_ACCPSWRD_HXX_
#pragma INCMSG("--- Beg 'accpswrd.hxx'")

#ifndef X_ACCEDIT_HXX_
#define X_ACCEDIT_HXX_
#include "accedit.hxx"
#endif

class CAccPassword : public CAccEdit
{

public:
    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccPassword( CElement* pElementParent );
};

#pragma INCMSG("--- End 'accpswrd.hxx'")
#else
#pragma INCMSG("*** Dup 'accpswrd.hxx'")
#endif



