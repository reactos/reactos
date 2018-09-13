//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pch.h
//
//--------------------------------------------------------------------------


#ifdef __cplusplus
extern "C" {
#endif

#ifndef NT_INCLUDED
#   include <nt.h>
#endif

#ifndef _NTRTL_
#   include <ntrtl.h>
#endif

#ifndef _NTURTL_
#   include <nturtl.h>
#endif

#ifdef __cplusplus
}  // end of extern "C"
#endif

#include <windows.h>
#include <windowsx.h>
#include <atlbase.h>
#include <atlconv.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlapip.h>
#include <shlobjp.h>
#include <shsemip.h>
#include <comctrlp.h>
#include <cscapi.h>
#include "strings.h"  // project-level string constants in dll directory.
#include "alloc.h"
#include "utils.h"
#include "debug.h"
#include "debugp.h"
#include "autoptr.h"
#include "strclass.h"
#include "config.h"
extern LONG g_cRefCount;               // defined in ..\dll\unknown.cpp
extern HINSTANCE g_hInstance;          // defined in ..\dll\dll.cpp


//
// Simple class representing an exception type.  I want to throw DWORDs
// so that all exception codes map to a system error message and so you 
// can throw the result of a GetLastError() call if needed.  It's not a great
// idea to throw a standard data type like DWORD so I've made this simple
// "exception" class to contain the DWORD code.
//
class CException
{
    public:
        explicit CException(DWORD code)
            : dwError(code) { }

        DWORD dwError;
};
       


const DWORD FLAG_CSC_COPY_STATUS_LOCALLY_MODIFIED = FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED |
                                                    FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED |
                                                    FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED;

const DWORD FLAG_CSC_COPY_STATUS_NEEDTOSYNC_FILE = FLAG_CSC_COPY_STATUS_SPARSE |
                                              FLAG_CSC_COPY_STATUS_STALE |
                                              FLAG_CSC_COPY_STATUS_ORPHAN |
                                              FLAG_CSC_COPY_STATUS_SUSPECT |
                                              FLAG_CSC_COPY_STATUS_LOCALLY_CREATED |
                                              FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED |
                                              FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED |
                                              FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED;
//
// Folders are always marked "sparse" so the sparse flag isn't
// included as it is with files.
//
const DWORD FLAG_CSC_COPY_STATUS_NEEDTOSYNC_FOLDER = FLAG_CSC_COPY_STATUS_STALE |
                                              FLAG_CSC_COPY_STATUS_ORPHAN |
                                              FLAG_CSC_COPY_STATUS_SUSPECT |
                                              FLAG_CSC_COPY_STATUS_LOCALLY_CREATED |
                                              FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED |
                                              FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED |
                                              FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED;

#define WC_NETCACHE_VIEWER   TEXT("MicrosoftNetCacheViewer")
