//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       dutil.hxx
//
//  Contents:   Utility functions and macros for data objects
//
//  History:    14-Dec-95    BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __DUTIL_HXX__
#define __DUTIL_HXX__

LPIDA
DataObj_GetHIDA(
    LPDATAOBJECT pdtobj,
    STGMEDIUM* pmedium
    );

//===========================================================================
// HIDA -- IDList Array handle
//===========================================================================

#define HIDA_GetPIDLFolder(pida)        (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida,i)        (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

typedef HGLOBAL HIDA;

VOID
HIDA_ReleaseStgMedium(
    LPIDA pida,
    STGMEDIUM* pmedium
    );

//===========================================================================
// ID list functions
//===========================================================================

LPITEMIDLIST*
ILA_Clone(
    UINT cidl,
    LPCITEMIDLIST* apidl
    );

VOID
ILA_Free(
    UINT cidl,
    LPITEMIDLIST* apidl
    );

#endif // __DUTIL_HXX__
