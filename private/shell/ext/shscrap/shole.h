//Prevent windows.h from pulling in OLE 1
#define _INC_OLE

#include <windows.h>
#include <stdlib.h>
#include <shlwapi.h>        // must be before commctrl.h and shlobj.h
#include <shlobj.h>         // ;Internal
#include <shellapi.h>       // ;Internal
#include <shsemip.h>

#include <ole2ver.h>
#include <shellp.h>     // in shell\inc
#include <debug.h>      // in shell\inc
#include <shguidp.h>    // in shell\inc
#include <shlwapip.h>   // for string helper functions

#ifndef WINNT
#include <w95wraps.h>
#define lstrcmpiW       StrCmpIW
#define lstrcpyW        StrCpyW
#else

#endif

#define SAVE_OBJECTDESCRIPTOR
#define FIX_ROUNDTRIP

#define CCF_CACHE_GLOBAL        32
#define CCF_CACHE_CLSID         32
#define CCF_RENDER_CLSID        32
#define CCFCACHE_TOTAL  (CC_FCACHE_GLOBAL+CCF_CACHE_CLSID+CCF_RENDER_CLSID)


HRESULT CScrapData_CreateInstance(LPUNKNOWN * ppunk);
HRESULT CTemplateFolder_CreateInstance(LPUNKNOWN * ppunk);
HRESULT CScrapExt_CreateInstance(LPUNKNOWN * ppunk);

//
// global variables
//
extern LONG g_cRefThisDll;              // per-instance
extern HINSTANCE g_hinst;
STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);

#ifdef __cplusplus
extern "C" {
#endif

extern const WCHAR c_wszContents[];
extern const WCHAR c_wszDescriptor[];

#ifdef __cplusplus
};

//
//  Don't pull in the bloated C runtime.
//
inline void *  __cdecl operator new(size_t size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }

#endif

#define HINST_THISDLL g_hinst



#define CFID_EMBEDDEDOBJECT     0
#define CFID_OBJECTDESCRIPTOR   1
#define CFID_LINKSRCDESCRIPTOR  2
#define CFID_RICHTEXT           3
#define CFID_SCRAPOBJECT        4
#define CFID_TARGETCLSID        5
#define CFID_RTF                6
#define CFID_MAX                7

#define CF_EMBEDDEDOBJECT       _GetClipboardFormat(CFID_EMBEDDEDOBJECT)
#define CF_OBJECTDESCRIPTOR     _GetClipboardFormat(CFID_OBJECTDESCRIPTOR)
#define CF_LINKSRCDESCRIPTOR    _GetClipboardFormat(CFID_LINKSRCDESCRIPTOR)
#define CF_RICHTEXT             _GetClipboardFormat(CFID_RICHTEXT)
#define CF_SCRAPOBJECT          _GetClipboardFormat(CFID_SCRAPOBJECT)
#define CF_TARGETCLSID          _GetClipboardFormat(CFID_TARGETCLSID)
#define CF_RTF                  _GetClipboardFormat(CFID_RTF)

CLIPFORMAT _GetClipboardFormat(UINT id);
void DisplayError(HWND hwndOwner, HRESULT hres, UINT idsMsg, LPCTSTR szFileName);

// From shole.c
void CShClientSite_RegisterClass();
IOleClientSite* CShClientSite_Create(HWND hwnd, LPCTSTR pszFileName);
void CShClientSite_Release(IOleClientSite* pcli);

// From template.cpp
HRESULT _KeyNameFromCLSID(REFCLSID rclsid, LPTSTR pszKey, UINT cchMax);
int _ParseIconLocation(LPTSTR pszIconFile);
