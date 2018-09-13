//
// Local private header file

#include "deskhtm.h"
#include <regstr.h>
#include "resource.h"
#include "shdocvw.h"

#ifndef SHDOC401_DLL
#ifdef WINNT
#define g_fRunningOnNT TRUE
#else
#define g_fRunningOnNT FALSE
#endif
#endif

#define MAX_URL_STRING      INTERNET_MAX_URL_LENGTH


// Use our private version of the OLE task allocator so we don't pull
// in all of OLE32 just to do LocalAlloc and LocalFree!

#define CoTaskMemFree       SHFree
#define CoTaskMemAlloc      SHAlloc
