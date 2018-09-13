//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       rshx32.h
//
//  Remote administration shell extension.
//
//--------------------------------------------------------------------------

#ifndef _RSHX32_H_
#define _RSHX32_H_

#ifndef UNICODE
#error "No ANSI support yet"
#endif

extern "C"
{
    #include <nt.h>         // for SE_TAKE_OWNERSHIP_PRIVILEGE, etc
    #include <ntrtl.h>
    #include <nturtl.h>
    #include <seopaque.h>   // RtlObjectAceSid, etc.
    #include <sertlp.h>     // RtlpOwnerAddrSecurityDescriptor, etc.
}

#define INC_OLE2
#include <windows.h>
#include "resource.h"   // resource IDs

#ifndef RC_INVOKED

#include <winspool.h>
#include <shellapi.h>   // HDROP, ShellExecuteEx
#include <shlobj.h>     // CF_IDLIST
#include <shlwapi.h>    // StrChr
#include <commctrl.h>   // property page stuff
#include <comctrlp.h>   // DPA
#include <aclapi.h>
#include <aclui.h>
#include <common.h>
#include "cstrings.h"
#include "util.h"
#include "ntfssi.h"
#include "printsi.h"

#if(_WIN32_WINNT >= 0x0500)
#include <shlobjp.h>    // ILCombine
#else   // _WIN32_WINNT < 0x0500
#define ILIsEmpty(pidl)     ((pidl)->mkid.cb==0)
STDAPI_(void) ILFree(LPITEMIDLIST pidl);
STDAPI_(LPITEMIDLIST) ILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
#endif  // _WIN32_WINNT < 0x0500


#define ALL_SECURITY_ACCESS     (READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY)

// Magic debug flags
#define TRACE_RSHX32        0x00000001
#define TRACE_SI            0x00000002
#define TRACE_NTFSSI        0x00000004
#define TRACE_PRINTSI       0x00000008
#define TRACE_UTIL          0x00000010
#define TRACE_NTFSCOMPARE   0x00000020
#define TRACE_ALWAYS        0xffffffff          // use with caution

//
// Global variables
//
extern HINSTANCE        g_hInstance;
extern LONG             g_cRefThisDll;
extern CLIPFORMAT       g_cfShellIDList;
extern CLIPFORMAT       g_cfPrinterGroup;
extern CLIPFORMAT       g_cfMountedVolume;

#endif // RC_INVOKED
#endif // _RSHX32_H_
