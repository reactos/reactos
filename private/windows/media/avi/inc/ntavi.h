/******************************************************************************

   Copyright (C) Microsoft Corporation 1992. All rights reserved.

   Title:   ntavi.h - Definitions for the portable win16/32 version of AVI

   This file should be included by ALL AVI components that are built for NT.
   It provides AVI specific portable definitions that either do not belong
   in PORT1632.H or are not yet included in that file.

*****************************************************************************/


#include <port1632.h>

#if 0

Windows 3.1, from which most of the AVI code comes, uses #ifdef DEBUG
to control debug and retail build.  Windows NT on the other hand uses
DBG (as in #if DBG - note, not ifDEF) to control debug stuff.

For NT, we need the following paragraph in this single common header.

#endif

#ifdef WIN32
#undef DEBUG
#if DBG
    #define DEBUG
    #define STATICFN
    #define STATICDT
#else
    #define STATICFN static
    #define STATICDT static
#endif

#else    // !WIN32
    #define STATICFN static
    #define STATICDT static
#endif

/*
 * mciavi\ntavi.h provides definitions that are specific to mciavi.
 *
 * this file provides general definitions that are of use throughout
 * avi.
 */

#ifdef WIN32


/* --- Win32 version --------------------------------------------------- */

// To separate 32 bit drivers from their Win 16 equivalent we use a defined
// constant to let the code know in which INI file section to look

#define MCIAVI_SECTION (TEXT("MCIAVI"))       // To be changed to MCIAVI32 shortly

#ifdef I386
// __inline provides speed improvements for x86 platforms.  Unfortunately
// the MIPS compiler does not have inline support.  Alpha is unknown, so
// we do not assume and play it safe.
#define INLINE __inline
#else
#define INLINE
#endif

#define AVI_HTASK	DWORD
#define NPTSTR		LPTSTR

#if !defined(_ALPHA_) && !defined(_PPC_)
typedef RGBQUAD *	LPRGBQUAD;
#endif

//typedef HANDLE      HDRVR;

#define _FASTCALL

#define hmemcpy		memcpy

#undef EXPORT
#define EXPORT

#define _huge
#define huge

#else

/* --- Win16 version --------------------------------------------------- */


// To separate 32 bit drivers from their Win 16 equivalent we use a defined
// constant to let the code know in which INI file section to look
// The WIN16 version of MMDDK.H does not have these constants defined

#define DRIVERS_SECTION "Drivers"
#define MCI_SECTION "MCI"
#define MCIAVI_SECTION "MCIAVI"

#define	WIN16
#define WIN31

#define TEXT(a)		a
#define AVI_HTASK	HANDLE
#define NPTSTR		NPSTR
#define LPTSTR		LPSTR
#define TCHAR		char

#define _FASTCALL	_fastcall
#define INLINE		__inline     /* Always OK for Win 16 */
#define UNALIGNED

/*
 * define these so we can explicitly use Ansi versions for debugging etc
 */
#define GetProfileStringA		GetProfileString
#define GetPrivateProfileStringA	GetPrivateProfileString
#define GetProfileIntA			GetProfileInt
#define wvsprintfA			wvsprintf
#define wsprintfA			wsprintf
#define lstrcmpiA			lstrcmpi
#define lstrcpyA			lstrcpy
#define lstrcatA			lstrcat
#define lstrlenA			lstrlen
#define LoadStringA			LoadString	
#define OutputDebugStringA		OutputDebugString
#define MessageBoxA			MessageBox


#endif
