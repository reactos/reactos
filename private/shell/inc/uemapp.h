#ifndef _UEMAPP_H_ // {
#define _UEMAPP_H_

//***   uemapp.h -- application (client) side of event monitor
//

#ifdef __cplusplus
extern "C" {
#endif

// {
//***   UEME_* -- events
#include "uemevt.h"         // separate #include so rulc.exe can use it

#define UEME_FBROWSER   0x80000000  // 0:shell 1:browser

//***   UEMF_* -- Flags used when calling FireEvent
#define UEMF_EVENTMON   0x00000001       // Traditional Event Monitor use of FireEvent
#define UEMF_INSTRUMENT 0x00000002       // These events are being instrumented
#define UEMF_MASK       (UEMF_EVENTMON | UEMF_INSTRUMENT)

//****  UEMF_ Meta Categories
#define UEMF_XEVENT     (UEMF_EVENTMON | UEMF_INSTRUMENT)

//***   UIG_* -- UI 'groups'
// NOTES
//  BUGBUG not sure if this is the right partitioning
#define UIG_NIL         (-1)
#define UIG_COMMON      1       // common UI elements (e.g. back/stop/refresh)
#define UIG_INET        2       // inet (html) elements (e.g. search/favs)
#define UIG_FILE        3       // file (defview) elements (e.g. up)
#define UIG_OTHER       4       // custom (isf, isv, docobj) elements

//***   UIM_* -- modules
// NOTES
//  used to separate namespaces.  e.g. IDMs for UEME_RUNWMCMD.
#define UIM_NIL         (-1)    // none (global)
#define UIM_EXPLORER    1       // explorer.exe
#define UIM_BROWSEUI    2       // browseui.dll
#define UIM_SHDOCVW     3       // shdocvw.dll
#define UIM_SHELL32     4       // shell32.dll

// Instrumented Browser wparams 
#define UIBW_ADDTOFAV   1
#define UIBW_404ERROR   2
#define UIBW_NAVIGATE   3       // navigation lP=how
    #define UIBL_NAVOTHER   0   // via other
    #define UIBL_NAVADDRESS 1   // via address bar
    #define UIBL_NAVGO      2   // (NYI) via 'go' button on address bar
    #define UIBL_NAVHIST    3   // via history pane
    #define UIBL_NAVFAVS    4   // via favorites pane
    #define UIBL_NAVFOLDERS 5   // (NYI) via all-folders pane
    #define UIBL_NAVSEARCH  6   // (NYI) via search pane
#define UIBW_RUNASSOC   4       // run lP=assoc
    #define UIBL_DOTOTHER   0   // other
    #define UIBL_DOTEXE     1   // .exe
    #define UIBL_DOTASSOC   2   // associated w/ some .exe
    #define UIBL_DOTNOASSOC 3   // not associated w/ some .exe (OpenWith)
    #define UIBL_DOTFOLDER  4   // folder
    #define UIBL_DOTLNK     5   // .lnk
#define UIBW_UICONTEXT  5       // context menu lP=where
    #define UIBL_CTXTOTHER      0   // (NYI) other
    #define UIBL_CTXTDEFBKGND   1   // defview background
    #define UIBL_CTXTDEFITEM    2   // defview item
    #define UIBL_CTXTDESKBKGND  3   // desktop background
    #define UIBL_CTXTDESKITEM   4   // desktop item
//  #define UIBL_CTXTQCUTBKGND  5   // (n/a) qlaunch background
    #define UIBL_CTXTQCUTITEM   6   // qlaunch/qlinks item
//  #define UIBL_CTXTISFBKGND   7   // (n/a) arb. isf background
    #define UIBL_CTXTISFITEM    8   // arb. isf item
    #define UIBL_CTXTITBBKGND   9   // (n/a) itbar background
    #define UIBL_CTXTITBITEM    10  // itbar item
// for input, however the menu is *1st* invoked is assumed to be representative
// of the *entire* menu action
#define UIBW_UIINPUT    6       // input method lP=source
    // n.b. no desktop/browser distinction
    #define UIBL_INPOTHER   0       // (NYI) other
    #define UIBL_INPMOUSE   1       // mouse
    #define UIBL_INPMENU    2       // menu key (alt or alt+letter)
    #define UIBL_INPACCEL   3       // (NYI) accelerator
    #define UIBL_INPWIN     4       // (NYI) 'windows' key

// Instrumented Browser lparams
#define UIBL_KEYBOARD   1
#define UIBL_MENU       2
#define UIBL_PANE       3


//***   UEM*_* -- app 'groups'
//
#define UEMIID_NIL      CLSID_NULL              // nil (office uses 0...)
#define UEMIID_SHELL    CLSID_ActiveDesktop     // BUGBUG need better one
#define UEMIID_BROWSER  CLSID_InternetToolbar   // BUGBUG need better one

#define UEMIND_NIL      (-1)
#define UEMIND_SHELL    0
#define UEMIND_BROWSER  1

#define UEMIND_NSTANDARD    2   // cardinality(UEMIND_*)

//***   UEM*Event -- helpers from ../lib/uassist.cpp
// NOTES
//  BUGBUG rename to UA* (from UEM*)
BOOL UEMIsLoaded();
HRESULT UEMFireEvent(const GUID *pguidGrp, int eCmd, DWORD dwFlags, WPARAM wParam, LPARAM lParam);
HRESULT UEMQueryEvent(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, LPUEMINFO pui);
HRESULT UEMSetEvent(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, LPUEMINFO pui);


#if 1 // {
//***   obsolete -- old exports, nuke after all callers fixed
//

STDAPI_(void) UEMEvalMsg(const GUID *pguidGrp, int uemCmd, WPARAM wParam, LPARAM lParam);

// obsolete! use UEMEvalMsg
//STDAPI_(void) UEMTrace(int uemCmd, LPARAM lParam);
#define UEMTrace(uemCmd, lParam)    UEMEvalMsg(&UEMIID_NIL, uemCmd, -1, lParam)

#define UEIA_RARE       0x01    // rare (demotion candidate)

STDAPI_(BOOL) UEMGetInfo(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, LPUEMINFO pui);
#endif // }

// }

// {
//***   UEMC_* -- commands
//
#define TABDAT(uemc)   uemc,
enum {
    #include "uemcdat.h"
};
#undef  TABDAT
// }


// {
//***   misc helpers
//

//***   XMB_ICONERROR -- guys that look like an error (vs. idle chit-chat)
// error stop (question) exclamation hand (info) (aster) (warn)
// BUGBUG is this the right set?
#define XMB_ICONERROR   (MB_ICONERROR|MB_ICONSTOP|MB_ICONEXCLAMATION|MB_ICONHAND)
// }

#ifdef __cplusplus
}
#endif

#endif // } _UEMAPP_H_
