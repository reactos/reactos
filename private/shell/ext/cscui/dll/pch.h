//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pch.h
//
//--------------------------------------------------------------------------

#ifndef _PCH_H_
#define _PCH_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef __cplusplus
} // end of extern "C"
#endif

#define INC_OLE2
#include <windows.h>
#include <windowsx.h>
#include <atlbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlobjp.h>    // SHFree
#include <shlwapi.h>
#include <shlwapip.h>   // QITAB, QISearch
#include <commctrl.h>
#include <comctrlp.h>
#include <cscapi.h>
#include <mobsync.h>    // OneStop/SyncMgr interfaces
#include <emptyvc.h>    // Memphis disk cleanup interface
#include <resource.h>   // resource IDs
#include <cscuiext.h>
#include <ras.h>
#include <rasdlg.h>
#include <raserror.h>

#ifndef FLAG_CSC_HINT_PIN_ADMIN
// BUGBUG This should be defined in cscapi.h, but since
// Remote Boot has been cancelled, use the system hint flag.
#define FLAG_CSC_HINT_PIN_ADMIN     FLAG_CSC_HINT_PIN_SYSTEM
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#define SIZEOF(a)       sizeof(a)
#endif

//
// Global function prototypes
//
STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);


#include "debug.h"
#include "cscentry.h"
#include "uuid.h"       // GUIDs
#include "util.h"
#include "filelist.h"   // CscFileNameList, PCSC_NAMELIST_HDR
#include "shellex.h"
#include "update.h"
#include "volclean.h"
#include "config.h"
#include "strings.h"


//
// Global variables
//
extern LONG             g_cRefCount;
extern HINSTANCE        g_hInstance;
extern CLIPFORMAT       g_cfShellIDList;
extern HANDLE           g_heventTerminate;
extern HANDLE           g_hmutexAdminPin;

//
// Magic debug flags
//
#define TRACE_UTIL          0x00000001
#define TRACE_SHELLEX       0x00000002
#define TRACE_UPDATE        0x00000004
#define TRACE_VOLFREE       0x00000008
#define TRACE_CSCST         0x00000080
#define TRACE_CSCENTRY      0x00000100
#define TRACE_ADMINPIN      0x00000200

#define TRACE_COMMON_ASSERT 0x40000000


/*-----------------------------------------------------------------------------
/ Exit macros
/   - these assume that a label "exit_gracefully:" prefixes the epilog
/     to your function
/----------------------------------------------------------------------------*/
#define ExitGracefully(hr, result, text)            \
            { TraceMsg(text); hr = result; goto exit_gracefully; }

#define FailGracefully(hr, text)                    \
            { if ( FAILED(hr) ) { TraceMsg(text); goto exit_gracefully; } }


/*-----------------------------------------------------------------------------
/ Interface helper macros
/----------------------------------------------------------------------------*/
#define DoRelease(pInterface)                       \
        { if ( pInterface ) { pInterface->Release(); pInterface = NULL; } }


/*-----------------------------------------------------------------------------
/ String helper macros
/----------------------------------------------------------------------------*/
#define StringByteCopy(pDest, iOffset, sz)          \
        { memcpy(&(((LPBYTE)pDest)[iOffset]), sz, StringByteSize(sz)); }

#define StringByteSize(sz)                          \
        ((lstrlen(sz)+1)*SIZEOF(TCHAR))


/*-----------------------------------------------------------------------------
/ Other helpful macros
/----------------------------------------------------------------------------*/
#define ByteOffset(base, offset)                    \
        (((LPBYTE)base)+offset)

#ifndef MAKEDWORDLONG
#define MAKEDWORDLONG(low,high) ((DWORDLONG)(((DWORD)(low)) | ((DWORDLONG)((DWORD)(high))) << 32))
#endif

#ifndef LODWORD
#define LODWORD(dwl)            ((DWORD)((dwl) & 0xFFFFFFFF))
#define HIDWORD(dwl)            ((DWORD)(((DWORDLONG)(dwl) >> 32) & 0xFFFFFFFF))
#endif

#ifndef ResultFromShort
#define ResultFromShort(i)      MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(i))
#endif


//
// This is from shell32. From the comments in browseui
// this value will not change.
//
#define FCIDM_REFRESH 0xA220

#define MAX_PATH_BYTES  (MAX_PATH * sizeof(TCHAR))


#endif  // _PCH_H_
