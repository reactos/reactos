// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

//	VK 3/13/98:  Changed from StdAfx.cpp to Inc.cpp.  This naming convention wasn't compatible with NTBuild.


#include "stdafx.h"

#pragma warning(disable: 4100 4189)	// Necessary for ia64 build

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>

#pragma warning(default: 4100 4189)	// Necessary for ia64 build
