//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       trans.h
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
#ifndef unix
#include "..\trans\transact.hxx"
#include "..\trans\bindctx.hxx"
#else
#include "../trans/transact.hxx"
#include "../trans/bindctx.hxx"
#endif /* unix */
#include <tls.h>


 

