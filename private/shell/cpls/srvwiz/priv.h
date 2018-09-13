#ifndef _PRIV_H_
#define _PRIV_H_

#define WANT_SHLWAPI_POSTSPLIT
#include <shlwapi.h>
#include <shlwapip.h>

#include <shellapi.h>
#include <ole2ver.h>
#include <olectl.h>

#include <shellp.h>
#include <shguidp.h>
#include <isguids.h>
#include <shdguid.h>

#include <shlobj.h>
#include <commctrl.h>
#include <prsht.h>
#include <cpl.h>
#include <help.h>
#include <ccstock.h>

#ifndef ATOMICRELEASE
#ifdef __cplusplus
#define ATOMICRELEASET(p, type) { if(p) { type* punkT=p; p=NULL; punkT->Release();} }
#else
#define ATOMICRELEASET(p, type) { if(p) { type* punkT=p; p=NULL; punkT->lpVtbl->Release(punkT);} }
#endif // __cplusplus

// doing this as a function instead of inline seems to be a size win.
//
#ifdef NOATOMICRELESEFUNC
#define ATOMICRELEASE(p) ATOMICRELEASET(p, IUnknown)
#else
#define ATOMICRELEASE(p) IUnknown_AtomicRelease((LPVOID*)&p)
#endif // NOATOMICRELESEFUNC
#endif // ATOMICRELEASE

#ifdef SAFERELEASE
#undef SAFERELEASE
#endif
#define SAFERELEASE(p) ATOMICRELEASE(p)


#include <windows.h>
#include <windowsx.h>

#include <cfdefs.h>
#include <ntverp.h>
#include <advpub.h>         // For REGINSTALL

#ifdef UNICODE
#define SysAllocStringT(psz)    SysAllocString(psz)
#else
#define SysAllocStringT(psz)    SysAllocStringA(psz)
#endif

/*****************************************************************************
 *
 *      Global state management.
 *      DLL reference count, DLL critical section.
 *
 *****************************************************************************/

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);

/*****************************************************************************
 *      Local Includes
 *****************************************************************************/
#include "srvwiz.h"

class CSrvWizFactory;
class CSrvWiz;

/*****************************************************************************
 *      Object Constructors
 *****************************************************************************/
HRESULT CSrvWizFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);
HRESULT CSrvWiz_CreateInstance(REFIID riid, LPVOID * ppvObj);


// Global variables
//
extern LONG g_cRefDll;      // per-instance
extern HINSTANCE g_hinst;

#define HINST_THISDLL   g_hinst

#endif // _PRIV_H_
