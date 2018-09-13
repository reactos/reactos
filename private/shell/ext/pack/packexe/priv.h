#define STRICT
#define _INC_OLE        // REVIEW: don't include ole.h in windows.h

#define OEMRESOURCE


#ifdef WINNT

//
// NT uses DBG=1 for its debug builds, but the Win95 shell uses
// DEBUG.  Do the appropriate mapping here.
//
#if DBG
#define DEBUG 1
#endif
#endif

#include <windows.h>
#ifdef WINNT
#include <stddef.h>
#include <wingdip.h>
#endif
#include <commctrl.h>
#ifdef WINNT
#include <comctrlp.h>
#endif
#include <windowsx.h>
#include <ole2.h>
#include <shlobj.h>     // Includes <fcext.h>

#ifdef UNICODE
#define CP_WINNATURAL   CP_WINUNICODE
#else
#define CP_WINNATURAL   CP_WINANSI
#endif

#include <port32.h>
// #include <heapaloc.h>
#include <..\inc\debug.h>      // our version of Assert etc.
#include <shellp.h>

#ifndef _M_PPC
#pragma intrinsic(memcpy)
#pragma intrinsic(memcmp)
#endif

extern "C" BOOL WINAPI PackWizRunFromExe();