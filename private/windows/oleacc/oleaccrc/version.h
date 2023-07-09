// Copyright (c) 1996-1999 Microsoft Corporation

// OLEACCRC.DLL version.h
#ifdef RC_INVOKED

// Only pull in WINVER.H if we need it.
#ifndef VER_H
#include <winver.h>
#endif/*VER_H*/

#include "..\oleacc\verdefs.h"

#define VER_IS_SET                  // Lets the RC know we're providing version strings
#define VER_FILEDESCRIPTION_STR     "Active Accessibility Resource DLL"
#define VER_INTERNALNAME_STR        "OLEACCRC"
#define VER_ORIGINALFILENAME_STR    "OLEACCRC.DLL"
#define VER_FILETYPE                VFT_DLL
#define VER_FILESUBTYPE             VFT2_UNKNOWN
#define VER_FILEVERSION             BUILD_VERSION_INT
#define VER_FILEVERSION_STR         BUILD_VERSION_STR

#endif/* RC_INVOKED */
