//
//  This header file contains symbols and typedefs needed by any
//  files that exist outside the deskhtm sub-directory.
//

#ifndef _DESKHTM_H_
#define _DESKHTM_H_

// deskcls.cpp
STDAPI CDeskHtmlProp_RegUnReg(BOOL bReg);

// dutil.cpp
STDAPI_(BOOL) SetDesktopFlags(DWORD dwMask, DWORD dwNewFlags);
#define COMPONENTS_DIRTY        0x00000001

STDAPI_(void) RefreshWebViewDesktop(void);
BOOL PokeWebViewDesktop(DWORD dwFlags);
void RemoveDefaultWallpaper(void);
#define REFRESHACTIVEDESKTOP() (PokeWebViewDesktop(AD_APPLY_FORCE | AD_APPLY_HTMLGEN | AD_APPLY_REFRESH))
void OnDesktopSysColorChange(void);


void SetSafeMode(DWORD dwFlags);

// 
// Desk Mover and Sizer stuff
//

EXTERN_C const CLSID CLSID_DeskMovr;
EXTERN_C const IID IID_IDeskMovr;

STDAPI_(BOOL) DeskMovr_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/);


#define RETURN_ON_FAILURE(hr) if (FAILED(hr)) return hr
#define RETURN_ON_NULLALLOC(ptr) if (!(ptr)) return E_OUTOFMEMORY
#define CLEANUP_ON_FAILURE(hr) if (FAILED(hr)) goto CleanUp

// Reference counting help.
//
#define QUICK_RELEASE(ptr)     ATOMICRELEASE(ptr)
#define ADDREF_OBJECT(ptr)     if (ptr) (ptr)->AddRef()
#define RELEASE_OBJECT(ptr)    ATOMICRELEASE(ptr)

#endif // _DESKHTM_H_
