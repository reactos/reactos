//----------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       versstr.h
//
//----------------------------------------------------------------------------

#include "version.h"

#if	(rmm < 10)
#define rmmpad "0"
#else
#define rmmpad
#endif

#if defined(_RELEASE_BUILD) && DBG == 0
#define VER_STR1(a,b,c)	    #a ".0"
#else
#define VER_STR1(a,b,c)	    #a ".00." rmmpad #b "." #c
#endif
#define VER_STR2(a,b,c)     VER_STR1(a,b,c)

#define VER_VERSION_STR     VER_STR2(rmj,rmm,rup)
#define VER_VERSION         rmj, 0, rmm, rup
#define VER_COMMENT         szVerName

#ifdef VER_PRIVATE_BUILD_STR
#define VER_PRIVATE_BUILD_FLG   VS_FF_PRIVATEBUILD | VS_FF_SPECIALBUILD
#else
#define VER_PRIVATE_BUILD_FLG   0
#endif

#if DBG==1
#define VER_DEBUG_BUILD_FLG     VS_FF_DEBUG
#else
#define VER_DEBUG_BUILD_FLG     0
#endif
