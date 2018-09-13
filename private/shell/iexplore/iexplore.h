#undef  STRICT
#define STRICT
#define _INC_OLE        // REVIEW: don't include ole.h in windows.h

#define OEMRESOURCE

// For dllload: change function prototypes to not specify declspec import
#define _SHDOCVW_      

#include <windows.h>

#ifdef WINNT_ENV
#include <stddef.h>
#endif

#include <commctrl.h>
#include <windowsx.h>
#include <ole2.h>
#include <shlobj.h>     // Includes <fcext.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <regstr.h>     // for REGSTR_PATH_EXPLORE

#ifdef UNICODE
#define CP_WINNATURAL   CP_WINUNICODE
#else
#define CP_WINNATURAL   CP_WINANSI
#endif

#include <ccstock.h>
#include <crtfree.h>
#include <port32.h>
// #include <heapaloc.h>
#include <debug.h>      // our version of Assert etc.
#include <shellp.h>
#include <wininet.h>
#include <shdocvw.h>


// Debug and trace message values
#define DF_DELAYLOADDLL         0x00000001


