//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: shitemid.c
//
// History:
//  12-06-93 SatoNa     Created.
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop

//
// Returns:
//  The resource index (of SHELL232.DLL) of the appropriate icon.
//
UINT SILGetIconIndex(LPCITEMIDLIST pidl, const ICONMAP aicmp[], UINT cmax)
{
    UINT i=0;
    UINT uType = (pidl->mkid.abID[0] & SHID_TYPEMASK);
    for (i=0; i<cmax; i++) {
	if (aicmp[i].uType == uType) {
	    return aicmp[i].indexResource;
	}
    }

    return II_DOCUMENT;   // default
}
