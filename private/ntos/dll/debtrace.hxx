//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       debtrace.hxx
//
//  Contents:   DEBTRACE compatibliity definitions
//
//  History:    20-Mar-95 VicH		Created
//
//--------------------------------------------------------------------------

#define DBGFLAG
#define DEBTRACE(args)	DebugTrace(0, ulLevel, args)
