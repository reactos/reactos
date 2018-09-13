//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// Stdinc.h 
//
//   Contains the standard include files used by.cpp files in ineticon.  Most
//   of the ineticon files will include this file.
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _STDINC_H_
#define _STDINC_H_

#ifdef UNICODE
// This stuff must run on Win95
#define _WIN32_WINDOWS      0x0400

#ifndef WINVER
#define WINVER              0x0400
#endif

#define _OLEAUT32_       // get DECLSPEC_IMPORT stuff right, we are defing these
#define _BROWSEUI_       // define bruiapi as functions exported instead of imported
#define _WINX32_         // get DECLSPEC_IMPORT stuff right for WININET API
#define _URLCACHEAPI     // get DECLSPEC_IMPORT stuff right for WININET CACHE API
#ifndef STRICT
#define STRICT
#endif

//
// Globaly defined includes.
//
//  
//  Include <w95wraps.h> before anything else that messes with names.
//  Although everybody gets the wrong name, at least it's *consistently*
//  the wrong name, so everything links.
//
//  NOTE:  This means that while debugging you will see functions like
//  CWindowImplBase__DefWindowProcWrapW when you expected to see
//  CWindowImplBase__DefWindowProc.
//
#define POST_IE5_BETA // turn on post-split iedev stuff
#endif

#define _SHDOCVW_

#ifdef UNICODE
#include <w95wraps.h>
#endif

#include <windows.h>
#include <ole2.h>
#include <debug.h>     // From shell\inc.
#ifdef UNICODE
#define _FIX_ENABLEMODELESS_CONFLICT  // for shlobj.h
#endif
#include <wininet.h>   // INTERNET_MAX_URL_LENGTH.  Must be before shlobjp.h!
#include <shlobj.h>    // IShellFolder
#include <shlobjp.h>   // IActiveDesktop
#include <shsemip.h>   // ILClone, ILFree etc.
#include <shellp.h>    // PDETAILSINFO
#include <ccstock.h>   // From shell\inc.
#ifdef UNICODE
#include <port32.h>
#endif

#include <urlmon.h>    // IPersistMoniker, IBindStatusCallback
#ifdef UNICODE
#include <winineti.h>    // Cache APIs & structures
#endif

#include <intshcut.h>  // IUniformResourceLocator
#include <msxml.h>
#include <iimgctx.h>   // IImgCtx interface.

#ifdef UNICODE
#define WANT_SHLWAPI_POSTSPLIT
#endif

#include "shlwapi.h"

#include <webcheck.h>  // ISubscriptionMgr
#include <mstask.h>    // TASK_TRIGGER
#include <chanmgr.h>   // Channel Mgr interface
#include <shdocvw.h>   // WhichPlatform


//
// Localy defined includes.
//

#include "debug.h"
#include "cache.h"
#include "runonnt.h"
#include "globals.h"
#include "strutil.h"
#include "utils.h"

#ifdef UNIX

extern "C" void unixEnsureFileScheme(TCHAR *lpszFileScheme);

#undef DebugMsg
#undef TraceMsg
#undef ASSERT

#ifdef DEBUG
extern "C" void _DebugMsgUnix(int i, const char *s, ...);
#define DebugMsg _DebugMsgUnix
#define TraceMsg _DebugMsgUnix
extern "C" void _DebugAssertMsgUnix(char *msg, char *fileName, int line);
#define ASSERT(x) { if(!(x)) _DebugAssertMsgUnix(#x, __FILE__, __LINE__);}
#else
#define DebugMsg
#define TraceMsg
#define ASSERT(x)
#endif /* DEBUG */
#endif /* UNIX */

#endif // _STDINC_H_
