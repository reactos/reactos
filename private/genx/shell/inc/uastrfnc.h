//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994-1998.
//
//  File:       uastrfnc.h
//
//  Contents:   Unaligned UNICODE lstr functions for MIPS, PPC, ALPHA, ...
//
//  Classes:
//
//  Functions:
//
//;begin_internal
//  History:    1-11-95   davepl   Created
//;end_internal
//
//--------------------------------------------------------------------------

// NOTE: This file assumes it is included from shellprv.h

#ifndef _UASTRFNC_H_
#define _UASTRFNC_H_

#ifdef __cplusplus
extern "C" {
#endif

// If we are running on a platform that requires aligned data, we need
// to provide custom string functions that can deal with unaligned
// strings.  On other platforms, these call directly to the normal string
// functions.

#ifndef _X86_
#define ALIGNMENT_MACHINE
#endif

#ifdef ALIGNMENT_MACHINE

#define IS_ALIGNED(p)   (((ULONG_PTR)(p) & (sizeof(*(p))-1) )==0)

UNALIGNED WCHAR * ualstrcpynW(UNALIGNED WCHAR * lpString1,
    		  	      UNALIGNED const WCHAR * lpString2,
    			      int iMaxLength);

int 		  ualstrcmpiW (UNALIGNED const WCHAR * dst,
			       UNALIGNED const WCHAR * src);

int 		  ualstrcmpW  (UNALIGNED const WCHAR * src,
			       UNALIGNED const WCHAR * dst);

size_t 		  ualstrlenW  (UNALIGNED const WCHAR * wcs);

UNALIGNED WCHAR * ualstrcpyW  (UNALIGNED WCHAR * dst,
			       UNALIGNED const WCHAR * src);


#else

#define ualstrcpynW StrCpyNW     // lstrcpynW is stubbed out on Windows 95
#define ualstrcmpiW StrCmpIW     // lstrcmpiW is stubbed out on Windows 95
#define ualstrcmpW  StrCmpW      // lstrcmpW is stubbed out on Windows 95
#define ualstrlenW  lstrlenW
#define ualstrcpyW  StrCpyW      // lstrcpyW is stubbed out on Windows 95

#endif // ALIGNMENT_MACHINE

#define ualstrcpynA lstrcpynA
#define ualstrcmpiA lstrcmpiA
#define ualstrcmpA  lstrcmpA
#define ualstrlenA  lstrlenA
#define ualstrcpyA  lstrcpyA

#ifdef UNICODE
#define ualstrcpyn ualstrcpynW
#define ualstrcmpi ualstrcmpiW
#define ualstrcmp  ualstrcmpW
#define ualstrlen  ualstrlenW
#define ualstrcpy  ualstrcpyW
#else
#define ualstrcpyn ualstrcpynA
#define ualstrcmpi ualstrcmpiA
#define ualstrcmp  ualstrcmpA
#define ualstrlen  ualstrlenA
#define ualstrcpy  ualstrcpyA
#endif

#ifdef __cplusplus
}       // extern "C"
#endif

#endif // _UASTRFNC_H_
