//+---------------------------------------------------------------------------
//  
//  Microsoft Windows
* Copyright (c) 1992 - 1999 Microsoft Corporation. All rights reserved.//  
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
//  Function:   FFail
//  
//  Synopsis:   Fails if count of fails is positive and evenly divides
//              interval count.
//  
//----------------------------------------------------------------------------


BOOL
FFail()
{
    Verify(!IsTagEnabled(tagValidate) || ValidateInternalHeap());

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
//  Function:   GetFailCount
//  
//  Synopsis:   Returns the number of failure points that have been
//              passed since the last failure count reset
//  
//  Returns:    int
//  
//-------------------------------------------------------------------------

int
GetFailCount( )
{
    // BUGBUG (garybu) Fail count should be per thread. Turn off crit sect for speed.
    // LOCK_GLOBALS;

    Assert(g_firstFailure >= 0);
    return g_cFFailCalled + ((g_firstFailure != 0) ? g_firstFailure : INT_MIN);
}


#ifdef BUILD_XMLDBG_AS_LIB
extern DWORD g_dwFALSE;
#else
DWORD g_dwFALSE = 0;
#endif
