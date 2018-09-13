//+---------------------------------------------------------------------------
//  
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//  
//  File:       ffail.cxx
//  
//  Contents:   Debug functions that you don't want to step into in
//              the debugger.  This module is compiled without the /Zi flag.
//  
//----------------------------------------------------------------------------

#include "headers.hxx"

BOOL g_fJustFailed;

//+---------------------------------------------------------------------------
//  
//  Function:   DbgExFFail
//  
//  Synopsis:   Fails if count of fails is positive and evenly divides
//              interval count.
//  
//----------------------------------------------------------------------------


BOOL WINAPI
DbgExFFail()
{
    Verify(!DbgExIsTagEnabled(tagValidate) || DbgExValidateInternalHeap());

    // BUGBUG (garybu) Fail count should be per thread. Turn off crit sect for speed.
    // LOCK_GLOBALS;

    g_fJustFailed = (++g_cFFailCalled < 0) ? FALSE : ! (g_cFFailCalled % g_cInterval);
    return g_fJustFailed;
}



//+---------------------------------------------------------------------------
//  
//  Function:   JustFailed
//  
//  Synopsis:   Returns result of last call to FFail
//  
//----------------------------------------------------------------------------

BOOL
JustFailed()
{
    return g_fJustFailed;
}



//+------------------------------------------------------------------------
//  
//  Function:   DbgExGetFailCount
//  
//  Synopsis:   Returns the number of failure points that have been
//              passed since the last failure count reset
//  
//  Returns:    int
//  
//-------------------------------------------------------------------------

int WINAPI
DbgExGetFailCount()
{
    // BUGBUG (garybu) Fail count should be per thread. Turn off crit sect for speed.
    // LOCK_GLOBALS;

    Assert(g_firstFailure >= 0);
    return g_cFFailCalled + ((g_firstFailure != 0) ? g_firstFailure : INT_MIN);
}

// used for assert to fool the compiler

DWORD g_dwFALSE = 0;

