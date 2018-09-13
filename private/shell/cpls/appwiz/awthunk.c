#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM


//============================================================================
// This file contains a bunch of Unicode/Ansi thunks to handle calling
// some internal functions that on Windows 95 the strings are Ansi,
// whereas the string on NT are unicode
//============================================================================

#define PFN_FIRSTTIME   ((void *)0xFFFFFFFF)

// First undefine everything that we are intercepting as to not forward back to us...
#undef SHGetSpecialFolderPath



// Explicit prototype because only the A/W prototypes exist in the headers
STDAPI_(BOOL) SHGetSpecialFolderPath(HWND hwndOwner, LPTSTR lpszPath, int nFolder, BOOL fCreate);

typedef BOOL (WINAPI * PFNGETSPECIALFOLDERPATH)(HWND hwndOwner, LPTSTR pwszPath, int nFolder, BOOL fCreate); 

BOOL _AorW_SHGetSpecialFolderPath(HWND hwndOwner, /*OUT*/ LPTSTR pszPath, int nFolder, BOOL fCreate)
{
    // The classic shell exported a non-decorated SHGetSpecialFolderPath
    // that took TCHAR parameters.  The IE4 shell exported additional
    // decorated versions.  Try to use the explicit decorated versions if
    // we can.
    
    static PFNGETSPECIALFOLDERPATH s_pfn = PFN_FIRSTTIME;

    *pszPath = 0;
    
    if (PFN_FIRSTTIME == s_pfn)
    {
        HINSTANCE hinst = LoadLibraryA("SHELL32.DLL");
        if (hinst)
        {
#ifdef UNICODE 
            s_pfn = (PFNGETSPECIALFOLDERPATH)GetProcAddress(hinst, "SHGetSpecialFolderPathW");
#else
            s_pfn = (PFNGETSPECIALFOLDERPATH)GetProcAddress(hinst, "SHGetSpecialFolderPathA");
#endif
        }
        else
            s_pfn = NULL;
    }

    if (s_pfn)
    {
        return s_pfn(hwndOwner, pszPath, nFolder, fCreate);
    }
    else
    {
        return SHGetSpecialFolderPath(hwndOwner, pszPath, nFolder, fCreate);
    }
}

#endif //DOWNLEVEL_PLATFORM
