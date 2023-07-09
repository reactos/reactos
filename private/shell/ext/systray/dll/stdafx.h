// Global Headers
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <regstr.h>

#define NOPOWERSTATUSDEFINES

#include <mmsystem.h>
#include <shellapi.h>
#include <shlapip.h>
#include <commctrl.h>
#include <winuserp.h>
#include <pccrdapi.h>     // Bugbug: Shoud be in NT include path (included locally)
#include <systrayp.h>
#include <help.h>         // Bugbug: Should be in NT include path (included locally)
#include <dbt.h>
#include <ntpoapi.h>
#include <poclass.h>
#include <cscuiext.h>

#include <objbase.h>
#include <docobj.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shlobjp.h>

#include <dbt.h>

// Global vars
extern long g_cLocks;
extern long g_cComponents;
extern HINSTANCE g_hinstDll;

// Macros
#ifndef ARRAYSIZE
#define ARRAYSIZE(x)   (sizeof((x))/sizeof((x)[0]))
#endif

#if defined(__cplusplus)
inline void* __cdecl operator new(size_t size)
{
    return (void*) LocalAlloc(LPTR, size);
}

inline void __cdecl operator delete(void* pv)
{
    LocalFree(pv);
}
#endif