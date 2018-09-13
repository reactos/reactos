#ifndef STDAFX_H_INCLUDED
#define STDAFX_H_INCLUDED

#ifdef WINNT
#ifndef UNICODE
#error "NETPLWIZ is UNICODE only"
#endif // !UNICODE
#endif // WINNT

#ifndef MAX
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

#undef ATL_MIN_CRT

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntlsa.h"

#define SECURITY_WIN32
#define SECURITY_KERBEROS
#include <security.h>
#include <secint.h>

#include <windows.h>        // basic windows functionality
#include <windowsx.h>
#include <commctrl.h>       // ImageList, ListView
#include <comctrlp.h>
#include <string.h>         // string functions
#include <crtdbg.h>         // _ASSERT macro
#include <objbase.h>
#include <shconv.h>
#include <tchar.h>

#include <shlwapi.h>
#include <shlwapip.h>
#include <shellapi.h>
#include <shlapip.h>
#include <shlobj.h>         // Needed by dsshell.h
#include <shlobjp.h>
#include <shlguid.h>
#include <shellp.h>         
#include <ccstock.h>

#include <rasdlg.h>
#include <raserror.h>

#include <lm.h>             // LanMap APIs
#include <nddeapi.h>        // MAX_USERNAME, MAX_DOMAINNAME definitions
#include <dsgetdc.h>        // DsGetDCName and DS structures
#include <ntdsapi.h>

#include <activeds.h>       // ADsGetObject
#include <iadsp.h>

#include <dsclient.h>       // LPDSOBJECTNAMES structure
#include <cmnquery.h>
#include <dsquery.h>

#include <objsel.h>         // Object picker

#include <lm.h>
#include <winnls.h>
#include <dsrole.h>
#include <advpub.h>         // For REGINSTALL

#include <cryptui.h>        // for certificate mgr

#include <validc.h>         // specifies valid characters for user names, etc.
#include <wininet.h>
#include <Msdasc.h>         // Rosebud com stuff.

#include "netpldat.h"
#include "netplp.h"
#include "netwiz.h"
#include "dialog.h"
#include "debugp.h"
#include "helpids.h"
#include "misc.h"

// Global variables
extern HINSTANCE g_hInstance;
extern ULONG g_cLocks;

// Debug levels
#define TRACE_COMMON_ASSERT     0x10000000

#define TRACE_USR_CORE          0x00000001
#define TRACE_USR_COM           0x00000002

#define TRACE_PSW               0x00000100

#define TRACE_ANP_CORE          0x00001000

#define TRACE_MND_CORE          0x00100000

// Avoid bringing in C runtime code for NO reason
#if defined(__cplusplus)
inline void * __cdecl operator new(size_t size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }
#endif  // __cplusplus

// Useful stuff
#define ExitGracefully(hr, result, text)            \
            { TraceMsg(text); hr = result; goto exit_gracefully; }

#define FailGracefully(hr, text)                    \
	    { if ( FAILED(hr) ) { TraceMsg(text); goto exit_gracefully; } }


#define RECTWIDTH(rc)  ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

//
// our concept of what domain, password and user names should be
//

#define MAX_COMPUTERNAME    LM20_CNLEN
#define MAX_USER            LM20_UNLEN          // BUGBUG: upn or domain\user
#define MAX_UPN             UNLEN
#define MAX_PASSWORD        PWLEN
#define MAX_DOMAIN          MAX_PATH            // BUGBUG: dns domain names etc
#define MAX_WORKGROUP       LM20_DNLEN      
#define MAX_GROUP           GNLEN

#define MAX_DOMAINUSER      MAX(MAX_UPN, MAX_USER + MAX_DOMAIN + 1)
// MAX_DOMAINUSER can hold:
//  <domain>/<username> or <upn>

#endif // !STDAFX_H_INCLUDED
