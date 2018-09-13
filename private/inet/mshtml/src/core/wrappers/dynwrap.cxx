//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dynwrap.cxx
//
//  Contents:   Utility for dynamically loaded procedures.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_CDERR_H_
#define X_CDERR_H_
#include <cderr.h>
#endif

// List of loaded libraries.

DeclareTag(tagLoadDll, "!Perf", "Trace callers on DLL loads");

static DYNLIB * s_pdynlibHead;

#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Function:   DeinitDynamicLibrary
//
//  Synopsis:   Unloads library
//
//----------------------------------------------------------------------------
void
DeinitDynamicLibrary(LPCTSTR pchLib)
{
    DYNLIB * pdynlib, ** ppdynlibPrev;
    char buf[200];
    int l;

    l = WideCharToMultiByte(CP_ACP, 0, pchLib, _tcslen(pchLib), buf, ARRAY_SIZE(buf), NULL, NULL);

#ifndef NO_IME
    if (!StrCmpNICA(buf, "mlang.dll", l))
    {
        extern DWORD g_dwTls;
        extern void DeinitMultiLanguage();
        if (TlsGetValue(g_dwTls))
            DeinitMultiLanguage();
    }
    else
#endif // NO_IME
    {
        ppdynlibPrev = &s_pdynlibHead;
        for (pdynlib = s_pdynlibHead; pdynlib; )
        {
            if (!StrCmpNICA(buf, pdynlib->achName, l))
            {
                if (pdynlib->hinst)
                {
                    FreeLibrary(pdynlib->hinst);
                    pdynlib->hinst = NULL;
                    *ppdynlibPrev = pdynlib = pdynlib->pdynlibNext;
                }
            }
            else
            {
                ppdynlibPrev = &pdynlib->pdynlibNext;
                pdynlib = pdynlib->pdynlibNext;
            }
        }
    }
}

#ifndef WIN16
//+---------------------------------------------------------------------------
//
//  Function:   IsDynamicLibraryLoaded
//
//  Synopsis:   Check if library loaded
//
//----------------------------------------------------------------------------
#if defined(UNIX) && defined(_HPUX_SOURCE)
extern void *g_pMultiLanguage;
#else
extern interface IMultiLanguage *g_pMultiLanguage;
#endif 

BOOL
IsDynamicLibraryLoaded(LPCTSTR pchLib)
{
    DYNLIB * pdynlib;
    char buf[200];
    int l;

    l = WideCharToMultiByte(CP_ACP, 0, pchLib, _tcslen(pchLib), buf, ARRAY_SIZE(buf), NULL, NULL);

    if (!StrCmpNICA(buf, "mlang.dll", l))
    {
        return g_pMultiLanguage != NULL;
    }
    else
    {
        for (pdynlib = s_pdynlibHead; pdynlib; pdynlib = pdynlib->pdynlibNext)
        {
            if (!StrCmpNICA(buf, pdynlib->achName, l))
            {
                if (pdynlib->hinst)
                {
                    return TRUE;
                }
                break;
            }
        }
    }

    return FALSE;
}
#endif // ndef WIN16

#endif

//+---------------------------------------------------------------------------
//
//  Function:   DeinitDynamicLibraries
//
//  Synopsis:   Undoes the work of LoadProcedure.
//
//----------------------------------------------------------------------------
void
DeinitDynamicLibraries()
{
    DYNLIB * pdynlib;

    for (pdynlib = s_pdynlibHead; pdynlib; pdynlib = pdynlib->pdynlibNext)
    {
        Assert(pdynlib->hinst);
        FreeLibrary(pdynlib->hinst);
        pdynlib->hinst = NULL;
    }
    s_pdynlibHead = NULL;
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadProcedure
//
//  Synopsis:   Load library and get address of procedure.
//
//              Declare DYNLIB and DYNPROC globals describing the procedure.
//              Note that several DYNPROC structures can point to a single
//              DYNLIB structure.
//
//                  DYNLIB g_dynlibOLEDLG = { NULL, "OLEDLG.DLL" };
//                  DYNPROC g_dynprocOleUIInsertObjectA =
//                          { NULL, &g_dynlibOLEDLG, "OleUIInsertObjectA" };
//                  DYNPROC g_dynprocOleUIPasteSpecialA =
//                          { NULL, &g_dynlibOLEDLG, "OleUIPasteSpecialA" };
//
//              Call LoadProcedure to load the library and get the procedure
//              address.  LoadProcedure returns immediatly if the procedure
//              has already been loaded.
//
//                  hr = LoadProcedure(&g_dynprocOLEUIInsertObjectA);
//                  if (hr)
//                      goto Error;
//
//                  uiResult = (*(UINT (__stdcall *)(LPOLEUIINSERTOBJECTA))
//                      g_dynprocOLEUIInsertObjectA.pfn)(&ouiio);
//
//              Release the library at shutdown.
//
//                  void DllProcessDetach()
//                  {
//                      DeinitDynamicLibraries();
//                  }
//
//  Arguments:  pdynproc  Descrition of library and procedure to load.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
LoadProcedure(DYNPROC *pdynproc)
{
    HINSTANCE   hinst;
    DYNLIB *    pdynlib = pdynproc->pdynlib;
    DWORD       dwError;

    if (pdynproc->pfn && pdynlib->hinst)
        return S_OK;

    if (!pdynlib->hinst)
    {
        TraceTag((tagPerf, "Loading library %s", pdynlib->achName));
        TraceTag((tagLoadDll, "%s initially loaded by:", pdynlib->achName));
        TraceCallers(tagLoadDll, 0, 10);
        
        // Try to load the library using the normal mechanism.

        hinst = LoadLibraryA(pdynlib->achName);

#ifdef WINCE
        if (!hinst)
        {
            goto Error;
        }
#endif // WINCE
#ifdef WIN16
        if ( (UINT) hinst < 32 )
        {
            // jumping to error won't work,
            // since GetLastError is currently always 0.
            //goto Error;
            // instead, return a bogus (but non-zero) error code.
            // (What should we return? I got 0x7e on one test.)
            // --mblain27feb97
            RRETURN(hinst ? (DWORD) hinst : (DWORD) ~0);
        }
#endif // WIN16
#if !defined(WIN16) && !defined(WINCE)
        // If that failed because the module was not be found,
        // then try to find the module in the directory we were
        // loaded from.

        if (!hinst)
        {
            dwError = GetLastError();

            if (   dwError == ERROR_MOD_NOT_FOUND
                || dwError == ERROR_DLL_NOT_FOUND)
            {
                char achBuf1[MAX_PATH];
                char achBuf2[MAX_PATH];
                char *pch;

                // Get path name of this module.
                if (GetModuleFileNameA(g_hInstCore, achBuf1, ARRAY_SIZE(achBuf1)) == 0)
                    goto Error;

                // Find where the file name starts in the module path.
                if (GetFullPathNameA(achBuf1, ARRAY_SIZE(achBuf2), achBuf2, &pch) == 0)
                    goto Error;

                // Chop off the file name to get a directory name.
                *pch = 0;

                // See if there's a dll with the given name in the directory.
                if (SearchPathA(
                        achBuf2,
                        pdynlib->achName,
                        NULL,
                        ARRAY_SIZE(achBuf1),
                        achBuf1,
                        NULL) != 0)
                {
                    // Yes, there's a dll. Load it.
                    hinst = LoadLibraryExA(
                                achBuf1,
                                NULL,
                                LOAD_WITH_ALTERED_SEARCH_PATH);
                }
            }
        }
        if (!hinst)
        {
            goto Error;
        }
#endif // !defined(WIN16) && !defined(WINCE)

        // Link into list for DeinitDynamicLibraries

        {
            LOCK_GLOBALS;

            if (pdynlib->hinst)
                FreeLibrary(hinst);
            else
            {
                pdynlib->hinst = hinst;
                pdynlib->pdynlibNext = s_pdynlibHead;
                s_pdynlibHead = pdynlib;
            }
        }
    }

    pdynproc->pfn = GetProcAddress(pdynlib->hinst, pdynproc->achName);
    if (!pdynproc->pfn)
    {
        goto Error;
    }

    return S_OK;

Error:
    RRETURN(GetLastWin32Error());
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeDynlib
//
//  Synopsis:   Free a solitary dynlib entry from the link list of dynlibs
//
//  Arguments:  Pointer to DYNLIB to be freed
//
//  Returns:    S_OK
//
//----------------------------------------------------------------------------

HRESULT
FreeDynlib(DYNLIB *pdynlib)
{
    DYNLIB ** ppdynlibPrev;

    LOCK_GLOBALS;

    if (pdynlib == s_pdynlibHead)
    {
        ppdynlibPrev = &s_pdynlibHead;
    }
    else
    {
        DYNLIB * pdynlibPrev;

        for (pdynlibPrev = s_pdynlibHead;
             pdynlibPrev && pdynlibPrev->pdynlibNext != pdynlib;
             pdynlibPrev = pdynlibPrev->pdynlibNext);

        ppdynlibPrev = pdynlibPrev ? &pdynlibPrev->pdynlibNext : NULL;
    }

    if (ppdynlibPrev)
    {
        if (pdynlib->hinst)
        {
            FreeLibrary(pdynlib->hinst);
            pdynlib->hinst = NULL;
        }
        
        *ppdynlibPrev = pdynlib->pdynlibNext;
    }
    
    return S_OK;
}

