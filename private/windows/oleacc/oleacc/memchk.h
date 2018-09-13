// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  MEMCHK.H
//
//  Simple new/delete counting error ehecking library
//
// -------------------------------------------------------------------------=


// Call this at DLL_PROCESS_ATTACH time...
void InitMemChk();

// ... and this at DLL_PROCESS_DETACH time - number of 
// outstanding delete's will be reported by DBPRINTF.
void UninitMemChk();
