//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       compress.h
//
//  Contents:   precompiled header file for compress directory
//
//  Classes:
//
//  Functions:
//
//  History:    07-14-97   DanpoZ (Danpo Zhang)   Created
//
//----------------------------------------------------------------------------
extern "C"
{
#ifdef unix
#include "../compress/gzip/api_int.h"
#else
#include "..\compress\gzip\api_int.h"
#endif /* !unix */
}

