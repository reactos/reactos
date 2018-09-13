#ifndef _pch_h_
#define _pch_h_

#include <windows.h>

// Map KERNEL32 unicode string functions to SHLWAPI
#define lstrcmpW    StrCmpW
#define lstrcmpiW   StrCmpIW
#define lstrcpyW    StrCpyW
#define lstrcpynW   StrCpyNW
#define lstrcatW    StrCatW

// Win9x implements lstrlenW and MessageBoxW so no mapping needed.

#include <windowsx.h>
#include <prsht.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shsemip.h>
#include <stdlib.h>
#include <shlobjp.h>
#include <shellp.h>
#include <comctrlp.h>
#include <ccstock.h>
#include <shlwapip.h>
#include <w95wraps.h>

#define FCIDM_REFRESH 0xA220

#include "..\common\propsext.h"
#include "addon.h"
#include "rc.h"
#include "regutils.h"

//
// Avoid bringing in C runtime code for NO reason
//
#if defined(__cplusplus)
inline void * __cdecl operator new(size_t size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }
#endif  // __cplusplus


#endif
