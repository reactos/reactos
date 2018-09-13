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
STDAPI_(DWORD) GetDesktopFlags(void);
#define COMPONENTS_DIRTY        0x00000001
#define COMPONENTS_LOCKED       0x00000002
#define COMPONENTS_ZOOMDIRTY    0x00000004

STDAPI_(void) RefreshWebViewDesktop(void);
BOOL PokeWebViewDesktop(DWORD dwFlags);
void RemoveDefaultWallpaper(void);
#define REFRESHACTIVEDESKTOP() (PokeWebViewDesktop(AD_APPLY_FORCE | AD_APPLY_HTMLGEN | AD_APPLY_REFRESH | AD_APPLY_DYNAMICREFRESH))
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

#endif // _DESKHTM_H_
