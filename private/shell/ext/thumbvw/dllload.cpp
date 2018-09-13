// You are expected to #include this file from your private dllload.c.
//

// Delay loading mechanism.  This allows you to write code as if you are
// calling implicitly linked APIs, and yet have these APIs really be
// explicitly linked.  You can reduce the initial number of DLLs that 
// are loaded (load on demand) using this technique.
//
// Use the following macros to indicate which APIs/DLLs are delay-linked
// and -loaded.
//
//      DELAY_LOAD
//      DELAY_LOAD_HRESULT
//      DELAY_LOAD_SAFEARRAY
//      DELAY_LOAD_UINT
//      DELAY_LOAD_INT
//      DELAY_LOAD_VOID
//
// Use these macros for APIs that are exported by ordinal only.
//
//      DELAY_LOAD_ORD
//      DELAY_LOAD_VOID_ORD
//
// Use these macros for APIs that only exist on the integrated-shell
// installations (i.e., a new shell32 is on the system).
//
//      DELAY_LOAD_SHELL
//      DELAY_LOAD_SHELL_HRESULT
//      DELAY_LOAD_SHELL_VOID
//
//
// Use DELAY_LOAD_IE_* for APIs that come from BrowseUI.  BrowseUI is
// special because it does not reside in the System directory.
//
// Use DELAY_LOAD_OCX_* for APIs that come from OCXs and not DLLs.
//

// a CLSID we know to be in browseui dll so that we can get the LFN path to the dll
#include "precomp.h"
#include "debug.h"
#include "dllload.h"
#include "port32.h"
#include "../lib/dllload.c"

/**********************************************************************/
/**********************************************************************/



// --------- SHELL32.DLL ---------------

//
// ----  delay load post win95 shell32 private functions
//
HINSTANCE g_hinstShell32 = NULL;

//
//  These functions are new for the NT5 shell and therefore must be
//  wrapped specially.
//
// 22 STDAPI_(BOOL) DAD_DragEnterEx2(HWND hwndTarget, const POINT ptStart, IDataObject *pdtObject)

DELAY_LOAD_SHELL(g_hinstShell32, shell32, BOOL, __DAD_DragEnterEx2, 22,
           (HWND hwndTarget, const POINT ptStart, IDataObject *pdtObject),
           (hwndTarget, ptStart, pdtObject));

STDAPI_(BOOL) DAD_DragEnterEx2(HWND hwndTarget, const POINT ptStart, IDataObject *pdtObject)
{
    if (g_uiShell32 >= 5) {
        return __DAD_DragEnterEx2(hwndTarget, ptStart, pdtObject);
    } else {
        return DAD_DragEnterEx(hwndTarget, ptStart);
    }
}


