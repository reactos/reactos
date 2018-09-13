//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       iapp.h
//
//  Contents:   precompiled header file for the trans directory
//
//  Classes:
//
//  Functions:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#include <urlmon.hxx>
#include <compress.h>
#ifdef unix
#include "../eapp/cmimeft.hxx"
#include "../eapp/ccodeft.hxx"
#include "../eapp/protbase.hxx"
#include "../eapp/cdlbsc.hxx"
#include "../eapp/cdlprot.hxx"
#include "../eapp/clshndlr.hxx"
#else
#include "..\eapp\cmimeft.hxx"
#include "..\eapp\ccodeft.hxx"
#include "..\eapp\protbase.hxx"
#include "..\eapp\cdlbsc.hxx"
#include "..\eapp\cdlprot.hxx"
#include "..\eapp\clshndlr.hxx"
#endif /* unix */

 

