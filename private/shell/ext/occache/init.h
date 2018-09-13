#ifndef __INIT__
#define __INIT__

#define CONST_VTABLE
#ifndef STRICT
#define STRICT
#endif
#ifndef WINVER
#define WINVER 0x0400
#define _WIN32_WINDOWS 0x0400
#endif

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <shlwapi.h>
#include <shlobj.h>         // in \sdk\inc
#include <shellapi.h>

#include <crtfree.h>        // don't use CRT libs
#include <ccstock.h>        // in ccshell\inc
#include <shsemip.h>        // in ccshell\inc
#include <shellp.h>             // in ccshell\inc
#include <debug.h>              // in ccshell\inc
#include <shguidp.h>        // in ccshell\inc
#include <advpub.h>

#pragma intrinsic(memcpy, memset, memcmp)

#ifdef __cplusplus
extern "C" {
#endif
    
extern HINSTANCE g_hInst;
extern BOOL      g_fAllAccess;
extern const CLSID CLSID_ControlFolder;
extern const CLSID CLSID_EmptyControlVolumeCache;
extern TCHAR g_szUnknownData[64];

#ifdef __cplusplus
};
#endif


STDAPI_(void) DllAddRef();
STDAPI_(void) DllRelease();

STDAPI ControlFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv); 
STDAPI EmptyControl_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv);


#endif
