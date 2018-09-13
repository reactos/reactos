//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: stream.c
//
//  This file contains some of the stream support code that is used by
// the shell.  It also contains the shells implementation of a memory
// stream that is used by the cabinet to allow views to be serialized.
//
// History:
//  08-20-93 KurtE      Added header block and memory stream.
//
//---------------------------------------------------------------------------
#include "shellprv.h"


STDAPI_(IStream *) 
OpenRegStream(
    HKEY hkey, 
    LPCTSTR pszSubkey, 
    LPCTSTR pszValue, 
    DWORD grfMode)
{
    return SHOpenRegStream(hkey, pszSubkey, pszValue, grfMode);
}


STDAPI_(IStream *)
CreateMemStream(
    LPBYTE pInit, 
    UINT cbInit)
{
    return SHCreateMemStream(pInit, cbInit);
}


