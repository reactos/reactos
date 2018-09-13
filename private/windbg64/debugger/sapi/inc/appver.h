//-----------------------------------------------------------------------------
//	appver.h
//
//	Copyright (C) 1993, Microsoft Corporation
//
//  Purpose:
//		template for using version resources
//
//  Revision History:
//
//	[]		09-Jul-1993 [dans]		Created
//
//-----------------------------------------------------------------------------
#if !defined(_appver_h)
#define _appver_h 1

#ifdef RC_INVOKED

#include <winver.h>
#include "version.h"
#include "stdver.h"


#define VER_FILEVERSION_STR 		SZVER "\0"
#define VER_FILEVERSION				rmj,rmm,0,rup
#define VER_PRODUCTVERSION_STR      SZVER "\0"
#define VER_PRODUCTVERSION          rmj,rmm,0,rup
                                        
#define VER_FILETYPE				VFT_DLL
#define VER_FILESUBTYPE 			VFT2_UNKNOWN
#define VER_LEGALCOPYRIGHT_YEARS        "1993-1995"
#define VER_LEGALCOPYRIGHT_STR		\
"Copyright \251 Microsoft Corp " VER_LEGALCOPYRIGHT_YEARS ".\0"

#if defined(RETAIL) || (rup==0)
#define VER_DEBUG					0
#define VER_PRIVATEBUILD			0
#define VER_PRERELEASE				0
#else
#define VER_DEBUG					VS_FF_DEBUG
#define VER_PRIVATEBUILD			VS_FF_PRIVATEBUILD
#define VER_PRERELEASE				VS_FF_PRERELEASE
#endif

#if defined(WIN32) || defined(_WIN32)
#define VER_FILEOS                                      VOS__WINDOWS32
#else
#define VER_FILEOS					VOS_DOS_WINDOWS16
#endif

#define VER_FILEFLAGS				(VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)

#define VER_COMPANYNAME_STR 		"Microsoft Corporation"
#define VER_PRODUCTNAME_STR 		"Microsoft\256 Visual C++"
#define VER_LEGALTRADEMARKS_STR     \
"Microsoft\256 is a registered trademark of Microsoft Corporation."


#endif

#endif
