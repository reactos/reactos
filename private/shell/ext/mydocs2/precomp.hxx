#ifndef _pch_h
#define _pch_h

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <shlwapi.h>

#include <ccstock.h>
#include <shsemip.h>
#include <shlwapip.h>
#include <shlapip.h>
#include <shlobjp.h>    // for shellp.h
#include <shellp.h>     // SHFOLDERCUSTOMSETTINGS
#include <cfdefs.h>     // CClassFactory, LPOBJECTINFO
#include <comctrlp.h>

extern const CLSID CLSID_MyDocsDropTarget;

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);

#ifdef DBG
#define DEBUG 1
#endif

//
// Avoid bringing in C runtime code for NO reason
//
#if defined(__cplusplus)
inline void * __cdecl operator new(size_t size) { return (void *)LocalAlloc(LPTR, size); }
inline void __cdecl operator delete(void *ptr) { LocalFree(ptr); }
extern "C" inline __cdecl _purecall(void) { return 0; }
#endif  // __cplusplus

#if defined(DBG) || defined(DEBUG)

#ifndef DEBUG
#define DEBUG
#endif

#else

#undef  DEBUG

#endif

#ifdef DEBUG

#define MDTraceSetMask(dwMask)    MDDoTraceSetMask(dwMask)
#define MDTraceEnter(dwMask, fn)  MDDoTraceEnter(dwMask, TEXT(fn))
#define MDTraceLeave              MDDoTraceLeave

#define MDTrace                   MDDoTrace
#define MDTraceMsg(s)             MDDoTrace(TEXT(s))

void MDDoTraceSetMask(DWORD dwMask);
void MDDoTraceEnter(DWORD dwMask, LPCTSTR pName);
void MDDoTraceLeave(void);
void MDDoTrace(LPCTSTR pFormat, ...);
void MDDoTraceAssert(int iLine, LPTSTR pFilename);


#define MDTraceLeaveResult(hr) \
                { HRESULT __hr = hr; if (FAILED(__hr)) MDTrace(TEXT("Failed (%08x)"), hr); MDTraceLeave(); return __hr; }

#else

#define MDTraceSetMask(dwMask)
#define MDTraceEnter(dwMask, fn)
#define MDTraceLeave()

#define MDTrace if (FALSE)
#define MDTraceMsg(s)

#define MDTraceLeaveResult(hr)    { return hr; }
#endif

#define TRACE_COMMON_ASSERT    0x40000000


#define ExitGracefully(hr, result, text)            \
            { MDTraceMsg(text); hr = result; goto exit_gracefully; }

#define FailGracefully(hr, text)                    \
            { if ( FAILED(hr) ) { MDTraceMsg(text); goto exit_gracefully; } }

// Magic debug flags

#define TRACE_CORE          0x00000001
#define TRACE_FOLDER        0x00000002
#define TRACE_ENUM          0x00000004
#define TRACE_ICON          0x00000008
#define TRACE_DATAOBJ       0x00000010
#define TRACE_IDLIST        0x00000020
#define TRACE_CALLBACKS     0x00000040
#define TRACE_COMPAREIDS    0x00000080
#define TRACE_DETAILS       0x00000100
#define TRACE_QI            0x00000200
#define TRACE_EXT           0x00000400
#define TRACE_UTIL          0x00000800
#define TRACE_SETUP         0x00001000
#define TRACE_PROPS         0x00002000
#define TRACE_COPYHOOK      0x00004000
#define TRACE_FACTORY       0x00008000


#define TRACE_ALWAYS        0xffffffff          // use with caution


#endif
