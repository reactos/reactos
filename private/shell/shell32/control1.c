#include "shellprv.h"
#pragma  hdrstop

#include "control.h"


//
// Should be put in header...
//

#ifdef WINNT
STDAPI_(HANDLE) ShellGetNextDriverName(HDRVR hdrv, LPTSTR pszName, int cbName);
LANGID DelayGetUserDefaultUILanguage( void );
#else
STDAPI_(HDRVR) ShellGetNextDriverName(HDRVR hdrv, LPTSTR pszName, int cbName);
#endif

#ifdef WX86

#include <wx86dll.h>

LONG g_cRefWx86;
HMODULE g_hmodWx86;

WX86LOADX86DLL_ROUTINE g_pfnWx86LoadX86Dll;
WX86FREEX86Dll_ROUTINE g_pfnWx86FreeX86Dll;
WX86THUNKPROC_ROUTINE g_pfnWx86ThunkProc;
WX86COMPACT_ROUTINE g_pfnWx86Compact;

#define Wx86LoadX86Dll(libname) (*g_pfnWx86LoadX86Dll)(libname, 0)
#define Wx86FreeX86Dll(hmod) (*g_pfnWx86FreeX86Dll)(hmod)
#define Wx86ThunkProc(pvAddress, pvCBDispatch, fNativeToX86) (*g_pfnWx86ThunkProc)(pvAddress, pvCBDispatch, fNativeToX86)
#define Wx86Compact() (*g_pfnWx86Compact)()

/*++
    This routine will increment Shell32's usage count of Wx86. When the use
    count transitions from 0 to 1 then Wx86 is loaded. The companion function
    Wx86Disable will decrement Shell32's usage count of Wx86 and unload Wx86
    when the count transitions from 1 to 0.

    If we have trouble loading wx86 then we will decide that wx86 is not
    installed and never try again.
--*/
BOOL Wx86Enable(void)
{
#ifdef WX86
    ENTERCRITICAL;
    if (++g_cRefWx86 == 1)
    {
        g_hmodWx86 = LoadLibrary(TEXT("wx86.dll"));
        if (g_hmodWx86)
        {
            g_pfnWx86LoadX86Dll = (PVOID)GetProcAddress(g_hmodWx86, "Wx86LoadX86Dll");
            g_pfnWx86FreeX86Dll = (PVOID)GetProcAddress(g_hmodWx86, "Wx86FreeX86Dll");
            g_pfnWx86ThunkProc = (PVOID)GetProcAddress(g_hmodWx86, "Wx86ThunkProc");
            g_pfnWx86Compact  = (PVOID)GetProcAddress(g_hmodWx86, "Wx86Compact");


            if (!g_pfnWx86LoadX86Dll || !g_pfnWx86FreeX86Dll ||
                !g_pfnWx86ThunkProc || !g_pfnWx86Compact)
            {
                FreeLibrary(g_hmodWx86);
                g_hmodWx86 = NULL;
            }
        }
    }

    LEAVECRITICAL;
    
    return g_hmodWx86 != NULL;
#else
    return FALSE;
#endif
}


void Wx86Disable(void)
{
    ENTERCRITICAL;

    if (--g_cRefWx86 == 0)
    {    
        FreeLibrary(g_hmodWx86);
        g_hmodWx86 = NULL;
        g_pfnWx86LoadX86Dll = NULL;
        g_pfnWx86FreeX86Dll = NULL;
        g_pfnWx86ThunkProc = NULL;
        g_pfnWx86Compact = NULL;
    }

    LEAVECRITICAL;
}

#endif


//
// BUGBUG? we may need to serialize access to this
//

HDSA g_hacplmLoaded = NULL;

//==========================================================================
//                                Functions
//==========================================================================

void ConvertCplInfo(LPVOID lpv)
{
#ifdef UNICODE
   NEWCPLINFOA   CplInfoA;
   LPNEWCPLINFOW lpCplInfoW = (LPNEWCPLINFOW)lpv;

   memcpy ((LPBYTE) &CplInfoA, lpv, SIZEOF(NEWCPLINFOA));

   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED,
                        CplInfoA.szName, ARRAYSIZE(CplInfoA.szName),
                        lpCplInfoW->szName, ARRAYSIZE(lpCplInfoW->szName));
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED,
                        CplInfoA.szInfo, ARRAYSIZE(CplInfoA.szInfo),
                        lpCplInfoW->szInfo, ARRAYSIZE(lpCplInfoW->szInfo));
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED,
                        CplInfoA.szHelpFile, ARRAYSIZE(CplInfoA.szHelpFile),
                        lpCplInfoW->szHelpFile, ARRAYSIZE(lpCplInfoW->szHelpFile));
   lpCplInfoW->dwSize = sizeof(NEWCPLINFOW);
#else
   NEWCPLINFOW   CplInfoW;
   LPNEWCPLINFOA lpCplInfoA = (LPNEWCPLINFOA)lpv;

   memcpy((LPBYTE) &CplInfoW, lpv, SIZEOF(NEWCPLINFOW));

   WideCharToMultiByte (CP_ACP, 0,
                        CplInfoW.szName, ARRAYSIZE(CplInfoW.szName),
                        lpCplInfoA->szName, ARRAYSIZE(lpCplInfoA->szName),
                        NULL, NULL);

   WideCharToMultiByte (CP_ACP, 0,
                        CplInfoW.szInfo, ARRAYSIZE(CplInfoW.szInfo),
                        lpCplInfoA->szInfo, ARRAYSIZE(lpCplInfoA->szInfo),
                        NULL, NULL);

   WideCharToMultiByte (CP_ACP, 0,
                        CplInfoW.szHelpFile, ARRAYSIZE(CplInfoW.szHelpFile),
                        lpCplInfoA->szHelpFile, ARRAYSIZE(lpCplInfoA->szHelpFile),
                        NULL, NULL);
   lpCplInfoA->dwSize = sizeof(NEWCPLINFOA);

#endif
}

//  See if pszShort is a truncated version of the string referred to by
//  hinst/id.  If so, then use the long string.  This is to work around
//  a "bad design feature" of CPL_NEWINQUIRE where the app returns a buffer
//  (which is only 32 or 64 chars long) rather than a resource id
//  like CPL_INQUIRE.  So if the app responds to both messages, and the
//  NEWINQUIRE string is a truncated version of the INQUIRE string, then
//  switch to the INQUIRE string.

LPTSTR _RestoreTruncatedCplString(
        HINSTANCE hinst,
        int id,
        LPTSTR pszShort,
        LPTSTR pszBuf,
        int cchBufMax)
{
    int cchLenShort, cchLen;

    cchLenShort = lstrlen(pszShort);
    cchLen = LoadString(hinst, id, pszBuf, cchBufMax);

    // Don't use SHTruncateString since KERNEL32 doesn't either
    if (StrCmpNC(pszShort, pszBuf, cchLenShort) == 0) {
        pszShort = pszBuf;
    }
    return pszShort;
}

//
//  Initializes *pcpli.
//
// Requires:
//  *pcpli is filled with 0 & NULL's.
//
BOOL _InitializeControl(LPCPLMODULE pcplm, LPCPLITEM pcpli)
{
    BOOL fSucceed = TRUE;
    union {
        NEWCPLINFO  Native;
        NEWCPLINFOA NewCplInfoA;
        NEWCPLINFOW NewCplInfoW;
    } Newcpl;
    CPLINFO cpl;
    TCHAR szName[MAX_CCH_CPLNAME];
    TCHAR szInfo[MAX_CCH_CPLINFO];
    LPTSTR pszName = Newcpl.Native.szName, pszInfo = Newcpl.Native.szInfo;
    HICON hIconTemp = NULL;

    //
    // always do the old method to get the icon ID
    //
    cpl.idIcon = 0;

    CPL_CallEntry(pcplm, NULL, CPL_INQUIRE, (LONG)pcpli->idControl, (LONG_PTR)(LPCPLINFO)&cpl);

    //
    // if this is a 32bit CPL and it gave us an ID then validate it
    // this fixes ODBC32 which gives back a bogus ID but a correct HICON
    // note that the next load of the same icon should be very fast
    //
    if (cpl.idIcon && !pcplm->minst.fIs16bit)
    {
        hIconTemp = LoadImage(pcplm->minst.hinst, MAKEINTRESOURCE(cpl.idIcon),
            IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

        if (!hIconTemp)
        {
            // the id was bogus, make it a negative number (invalid resource)...
            cpl.idIcon = -1;
            TraceMsg(TF_GENERAL, "_InitializaControl: %s returned an invalid icon id, ignoring", pcplm->szModule);
        }
    }

    pcpli->idIcon = cpl.idIcon;

    //
    //  Try the new method first and call it with the largest structure
    //  so it doesn't overwrite anything on the stack.  If you put a
    //  Unicode applet on Windows '95 it will kill the Explorer because
    //  it trashes memory by overwriting the stack.
    //
    memset(&Newcpl,0,SIZEOF(Newcpl));

    CPL_CallEntry (pcplm, NULL, CPL_NEWINQUIRE, (LONG)pcpli->idControl,
                    (LONG_PTR)(LPCPLINFO)&Newcpl);

    //
    //  If the call is to an ANSI applet, convert strings to Unicode
    //
#ifdef UNICODE
#define UNNATIVE_SIZE   SIZEOF(NEWCPLINFOA)
#else
#define UNNATIVE_SIZE   SIZEOF(NEWCPLINFOW)
#endif

    if (Newcpl.Native.dwSize == UNNATIVE_SIZE)
    {
        ConvertCplInfo(&Newcpl);        // This will set Newcpl.Native.dwSize
    }

    if (Newcpl.Native.dwSize == SIZEOF(NEWCPLINFO))
    {
        pszName = _RestoreTruncatedCplString(pcplm->minst.hinst, cpl.idName, pszName, szName, ARRAYSIZE(szName));
        pszInfo = _RestoreTruncatedCplString(pcplm->minst.hinst, cpl.idInfo, pszInfo, szInfo, ARRAYSIZE(szInfo));
    }
    else
    {
        Newcpl.Native.hIcon = LoadImage(pcplm->minst.hinst, MAKEINTRESOURCE(cpl.idIcon), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
        pszName = szName;
        LoadString(pcplm->minst.hinst, cpl.idName, szName, ARRAYSIZE(szName));
        pszInfo = szInfo;
        LoadString(pcplm->minst.hinst, cpl.idInfo, szInfo, ARRAYSIZE(szInfo));
        Newcpl.Native.szHelpFile[0] = TEXT('\0');
        Newcpl.Native.lData = cpl.lData;
        Newcpl.Native.dwHelpContext = 0L;
    }

    pcpli->hIcon = Newcpl.Native.hIcon;

    if (hIconTemp)
        DestroyIcon(hIconTemp);

    fSucceed = Str_SetPtr(&pcpli->pszName, pszName)
            && Str_SetPtr(&pcpli->pszInfo, pszInfo)
            && Str_SetPtr(&pcpli->pszHelpFile, Newcpl.Native.szHelpFile);

    pcpli->lData = Newcpl.Native.lData;
    pcpli->dwContext = Newcpl.Native.dwHelpContext;

#ifdef DEBUG
    if (!pcpli->idIcon)
        TraceMsg(TF_GENERAL, "PERFORMANCE: cannot cache %s because no icon ID for <%s>", pcplm->szModule, pcpli->pszName);
#endif

    //DebugMsg(DM_TRACE,"** Initialized %s (%s)", );

    return fSucceed;
}


//
// Terminate the control
//
void _TerminateControl(LPCPLMODULE pcplm, LPCPLITEM pcpli)
{
    //DebugMsg(DM_TRACE,"** Terminating %s (%s)", pcpli->pszName, pcplm->szModule);

    if( pcpli->hIcon )
    {
        DestroyIcon( pcpli->hIcon );
        pcpli->hIcon = NULL;
    }

    Str_SetPtr(&pcpli->pszName, NULL);
    Str_SetPtr(&pcpli->pszInfo, NULL);
    Str_SetPtr(&pcpli->pszHelpFile, NULL);
    CPL_CallEntry(pcplm, NULL, CPL_STOP, pcpli->idControl, pcpli->lData);
}


//
//  For each control of the specified CPL module, call the control entry
//  with CPL_STOP. Then, call it with CPL_EXIT.
//
void _TerminateCPLModule(LPCPLMODULE pcplm)
{
    if (pcplm->minst.hinst)
    {
        //DebugMsg(DM_TRACE,"** Terminating CPL %s", pcplm->szModule);

        if (pcplm->lpfnCPL)
        {
            if (pcplm->hacpli)
            {
                int cControls, i;

                for (i=0, cControls=DSA_GetItemCount(pcplm->hacpli); i<cControls; ++i)
                {
                    LPCPLITEM pcpli;
                    pcpli = DSA_GetItemPtr(pcplm->hacpli, i);
                    _TerminateControl(pcplm, pcpli);
                }

                DSA_DeleteAllItems(pcplm->hacpli);
                DSA_Destroy(pcplm->hacpli);
                pcplm->hacpli=NULL;
            }

            CPL_CallEntry(pcplm, NULL, CPL_EXIT, 0L, 0L);
            pcplm->lpfnCPL=NULL;
        }

        if (pcplm->minst.fIs16bit)
            FreeLibrary16(pcplm->minst.hinst);
        else
#ifdef WX86
            if (pcplm->minst.fIsX86Dll)
            {
                Wx86FreeX86Dll(pcplm->minst.hinst);
                Wx86Disable();
            } else {          
#endif
                FreeLibrary(pcplm->minst.hinst);
#ifdef WX86
            }
#endif

        pcplm->minst.hinst = NULL;
    }

    pcplm->minst.idOwner = (DWORD)-1;

    if(pcplm->minst.hOwner)
    {
        CloseHandle(pcplm->minst.hOwner);
        pcplm->minst.hOwner = NULL;
    }
}


//
// Initializes the CPL Module.
//
// Requires:
//  *pcplm should be initialized appropriately.
//
//
BOOL _InitializeCPLModule(LPCPLMODULE pcplm)
{
    BOOL fSuccess = FALSE;

    if (pcplm->minst.fIs16bit)
    {
        pcplm->lpfnCPL16 = (FARPROC16)GetProcAddress16(pcplm->minst.hinst, "CPlApplet");
    }
    else
    {
        pcplm->lpfnCPL32 = (APPLET_PROC)GetProcAddress(pcplm->minst.hinst, "CPlApplet");
#ifdef WX86
        if (pcplm->minst.fIsX86Dll)
        {
            pcplm->lpfnCPL32 = (APPLET_PROC)Wx86ThunkProc(pcplm->lpfnCPL32,
                                                          (PVOID)4,
                                                          TRUE);
            if (pcplm->lpfnCPL32 == (APPLET_PROC)-1)
            {
                pcplm->lpfnCPL32 = NULL;
            }
        }
#endif
    }

    //DebugMsg(DM_TRACE,"** Loading/Initializing CPL %s", pcplm->szModule);

    //
    // Initialize the CPL
    if (pcplm->lpfnCPL &&
        CPL_CallEntry(pcplm, NULL, CPL_INIT, 0L, 0L))
    {
        int cControls = (int)CPL_CallEntry(pcplm, NULL, CPL_GETCOUNT, 0L, 0L);

        if (cControls>0)
        {
            //
            // By passing in the number of applets, we should speed up allocation
            // of this array.
            //

            pcplm->hacpli = DSA_Create(SIZEOF(CPLITEM), cControls);

            if (pcplm->hacpli)
            {
                int i;

                fSuccess=TRUE; // succeded, so far.

                //
                // Go through the applets and load the information about them
                //

                for (i=0; i<cControls; ++i)
                {
                    CPLITEM control = {
                    (int)i,
                    (HICON)NULL,
                    (int)0,
                    (LPTSTR)NULL,
                    (LPTSTR)NULL,
                    (LPTSTR)NULL,
                    (LONG)0L,
                    (DWORD)0L };

                    if (_InitializeControl(pcplm, &control))
                    {
                        //
                        // removing this now saves us from doing it later
                        //

                        CPL_StripAmpersand(control.pszName);

                        if (DSA_AppendItem(pcplm->hacpli, &control) >=
                            0)
                        {
                            continue;
                        }
                    }

                    _TerminateControl(pcplm, &control);
                    fSuccess=FALSE;
                    break;
                }
            }
        }
    }
    else
    {
        // don't ever call it again if we couldn't CPL_INIT
        pcplm->lpfnCPL = NULL;
    }

    return fSuccess;
}


//
// Returns:
//   The index to the g_hacplmLoaded, if the specified DLL is already
//  loaded; -1 otherwise.
//
int _FindCPLModule(const MINST * pminst)
{
    int i = -1; // Assumes error

    ENTERCRITICAL;
    if (g_hacplmLoaded)
    {
        for (i=DSA_GetItemCount(g_hacplmLoaded)-1; i>=0; --i)
        {
            LPCPLMODULE pcplm = DSA_GetItemPtr(g_hacplmLoaded, i);

            //
            // owner id tested last since hinst is more varied
            //

            if ((pcplm->minst.hinst == pminst->hinst) &&
                (pcplm->minst.fIs16bit == pminst->fIs16bit) &&
                (pcplm->minst.idOwner == pminst->idOwner))
            {
                break;
            }
        }
    }
    LEAVECRITICAL;
    return i;
}

LPCPLMODULE FindCPLModule(const MINST * pminst)
{
    return (LPCPLMODULE) DSA_GetItemPtr(g_hacplmLoaded, _FindCPLModule(pminst));
}


//
// Returns:
//   The index to the g_hacplmLoaded, if the specified DLL is already
//  loaded; -1 otherwise.
//
int _FindCPLModuleByName(LPCTSTR pszModule)
{
    int i = -1; // Assumes error

    ENTERCRITICAL;
    if (g_hacplmLoaded)
    {
        for (i=DSA_GetItemCount(g_hacplmLoaded)-1; i>=0; --i)
        {
            LPCPLMODULE pcplm = DSA_GetItemPtr(g_hacplmLoaded, i);

            if (!lstrcmpi(pcplm->szModule, pszModule))
            {
                break;
            }
        }
    }
    LEAVECRITICAL;
    return i;
}

//
// Adds the specified CPL module to g_hacplmLoaded.
//
// Requires:
//  The specified CPL module is not in g_hacplmLoaded yet.
//
// Returns:
//  The index to the CPL module if succeeded; -1 otherwise.
//
int _AddModule(LPCPLMODULE pcplm)
{
    int     result;

    //
    // Create the Loaded Modules guy if necessary
    //
    ENTERCRITICAL;
    if (g_hacplmLoaded == NULL)
        g_hacplmLoaded = DSA_Create(SIZEOF(CPLMODULE), 4);
    //
    // Add this CPL to our list
    //
    if (g_hacplmLoaded == NULL)
        result = -1;
    else
        result = DSA_AppendItem(g_hacplmLoaded, pcplm);
    LEAVECRITICAL;
    return(result);
}

LRESULT CPL_CallEntry(LPCPLMODULE pcplm, HWND hwnd, UINT msg, LPARAM lParam1, LPARAM lParam2)
{
    LRESULT lres;

    _try
    {
        if (pcplm->minst.fIs16bit)
        {
            lres = CallCPLEntry16(pcplm->minst.hinst, pcplm->lpfnCPL16, hwnd, msg, lParam1, lParam2);
        }
        else
        {
            lres = pcplm->lpfnCPL32(hwnd, msg, lParam1, lParam2);
#ifdef WX86
            if (pcplm->minst.fIsX86Dll)
            {
                Wx86Compact();
            }
#endif
        }
    }
    _except(SetErrorMode(SEM_NOGPFAULTERRORBOX),UnhandledExceptionFilter(GetExceptionInformation()))
    {
        TraceMsg(TF_ERROR, "CPL: Exception calling CPL module: %s", pcplm->szModule);
        ShellMessageBox(HINST_THISDLL, NULL, MAKEINTRESOURCE(IDS_CPL_EXCEPTION),
                MAKEINTRESOURCE(IDS_CONTROLPANEL),
                MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL,
                (LPTSTR)pcplm->szModule);
        lres = 0;
    }

    return lres;
}

//
// Loads the specified CPL module and returns the index to g_hacplmLoaded.
//
int _LoadCPLModule(LPCTSTR pszModule)
{
    MINST minst;
    int iModule;

    minst.fIs16bit = TRUE;      // assume this at first

    // Warning: LoadLibrary16() now contains a specific hack to
    // work around a Multimedia Environments CPL bug.
    // (details in win95c:10306.) If you switch to your
    // own thunk to bypass LoadLibrary16, you'll need to copy that bug fix
    // from core\kernel\krn32.asm: LoadLibrary16().

#ifdef WINNT
    minst.hinst = (HINSTANCE) 21;       // Force it to try Win32
#else
    minst.hinst = LoadLibrary16(pszModule);
#endif  //  WINNT

    minst.idOwner = GetCurrentProcessId();
    minst.hOwner = OpenProcess(SYNCHRONIZE,FALSE,minst.idOwner);

    if (!ISVALIDHINST16(minst.hinst)) {

        // might be a 32 bit CPL
        if ((UINT_PTR)minst.hinst == 21) {   // Win32 DLL?
            minst.hinst = LoadLibrary(pszModule);
#ifdef WX86
            minst.fIsX86Dll = FALSE;
            if ((minst.hinst == NULL) &&
                (GetLastError() == ERROR_BAD_EXE_FORMAT) &&
                (Wx86Enable()))
            {
                // If we got a DLL type mismatch loading the dll then it
                // may be an x86 dll. Lets try to load it as an x86 dll.
                minst.hinst = (HINSTANCE)Wx86LoadX86Dll(pszModule);
                minst.fIsX86Dll = TRUE;             
            }
#endif
            if (!ISVALIDHINSTANCE(minst.hinst))
            {
                CloseHandle(minst.hOwner);
#ifdef WX86
                if (minst.fIsX86Dll)
                {
                    Wx86Disable();
                }
#endif
                return -1;
            }
            minst.fIs16bit = FALSE;
        }
        else
            return -1;
    }

    //
    // Check if this module is already in the list.
    //

    iModule = _FindCPLModule(&minst);

    if (iModule >= 0)
    {
        //
        // Yes. Increment the reference count and return the ID.
        //
        LPCPLMODULE pcplm;

        ENTERCRITICAL;
        pcplm = DSA_GetItemPtr(g_hacplmLoaded, iModule);
        ++pcplm->cRef;
        LEAVECRITICAL;

        //
        // Decrement KERNELs reference count
        //

        if (minst.fIs16bit)
            FreeLibrary16(minst.hinst);
        else
#ifdef WX86
        {
            if (minst.fIsX86Dll)
            {
                Wx86FreeX86Dll(minst.hinst);
                Wx86Disable();
            } else {
                FreeLibrary(minst.hinst);
            }
        }
#else
            FreeLibrary(minst.hinst);
#endif    

        CloseHandle(minst.hOwner);
    }
    else
    {
        CPLMODULE sModule;

        //
        // No. Append it.
        //

        sModule.cRef = 1;
        sModule.minst = minst;
        sModule.lpfnCPL = NULL;
        sModule.hacpli = NULL;

        if (minst.fIs16bit)
        {
            GetModuleFileName16(minst.hinst, sModule.szModule, ARRAYSIZE(sModule.szModule));
        }
        else
        {
            GetModuleFileName(minst.hinst, sModule.szModule, ARRAYSIZE(sModule.szModule));
        }

        if (_InitializeCPLModule(&sModule))
        {
            iModule = _AddModule(&sModule);
        }

        if (iModule < 0)
        {
            _TerminateCPLModule(&sModule);
        }
    }
    return iModule;
}


int _FreeCPLModuleIndex(int iModule)
{
    LPCPLMODULE pcplm;

    ENTERCRITICAL;
    pcplm = DSA_GetItemPtr(g_hacplmLoaded, iModule);

    if (!pcplm)
    {
        //
        // BUGBUG: It would be very bad if pcplm is 0; Perhaps there
        // should be an assert?
        //
        LEAVECRITICAL;
        return(-1);
    }

    //
    // Dec the ref count; return if not 0
    //

    --pcplm->cRef;

    if (pcplm->cRef)
    {
        LEAVECRITICAL;
        return(pcplm->cRef);
    }

    //
    // Free up the whole thing and return 0
    //

    _TerminateCPLModule(pcplm);

    DSA_DeleteItem(g_hacplmLoaded, iModule);

    //
    // Destroy this when all CPLs have been removed
    //

    if (DSA_GetItemCount(g_hacplmLoaded) == 0)
    {
        DSA_Destroy(g_hacplmLoaded);
        g_hacplmLoaded = NULL;
    }
    LEAVECRITICAL;
    return(0);
}


int _FreeCPLModuleHandle(const MINST * pminst)
{
    int iModule;

    //
    // Check if the module is actually loaded (major error if not)
    //

    iModule = _FindCPLModule(pminst);

    if (iModule < 0)
    {
        return(-1);
    }

    return _FreeCPLModuleIndex(iModule);
}

int CPL_FreeCPLModule(LPCPLMODULE pcplm)
{
    return _FreeCPLModuleHandle(&pcplm->minst);
}


void CPL_StripAmpersand(LPTSTR szBuffer)
{
    LPTSTR pIn, pOut;

    //
    // copy the name sans '&' chars
    //

    pIn = pOut = szBuffer;
    do
    {
        //
        // strip FE accelerators with parentheses.  e.g. "foo(&F)" -> "foo"
        //
        if (*pIn == TEXT('(') && *(pIn+1) == TEXT('&') &&
            *(pIn+2) && *(pIn+3) == TEXT(')')) {
            pIn += 4;
        }

#ifdef DBCS
        // Also strip FE accelerators in old win31 cpl, i.e, 01EH/01FH.
        if (*pIn == 0x1e && *++pIn) {


            // Assumes a character right before the mnemonic
            // is a parenthesis or something to be removed as well.
            //
            pOut=CharPrev(szBuffer, pOut);

            // Skip Alphabet accelerator.
            pIn = CharNext(pIn);

            if (*pIn) {
                if (*pIn == 0x1f && *++pIn) {

                    // Skip FE accelelator
                    //
                    pIn = CharNext(pIn);
                }
                // Skip second parenthesis.
                //
                pIn = CharNext(pIn);
            }
        }
#endif
        if (*pIn != TEXT('&')) {
            *pOut++ = *pIn;
        }
        if (IsDBCSLeadByte(*pIn)) {
            *pOut++ = *++pIn;
        }
    } while (*pIn++) ;
}


//
// filter out bogus old ini keys... we may be able to blow this off
BOOL IsValidCplKey(LPTSTR pStr)
{
    return lstrcmpi(pStr, TEXT("NumApps")) &&
        !((*(pStr+1) == 0) &&
        ((*pStr == TEXT('X')) || (*pStr == TEXT('Y')) || (*pStr == TEXT('W')) || (*pStr == TEXT('H'))));
}


LPCPLMODULE CPL_LoadCPLModule(LPCTSTR szModule)
{
    LPCPLMODULE     result;

    int iModule = _LoadCPLModule(szModule);

    if (iModule < 0)
        result = NULL;
    else
    {
        ENTERCRITICAL;
        result = DSA_GetItemPtr(g_hacplmLoaded, iModule);
        LEAVECRITICAL;
    }
    return(result);
}


//=========================================================================
// CPLD_ functions
//=========================================================================

//
// Called for each CPL module file which we may want to load on startup.
void _InsertModuleName(ControlData *lpData, LPCTSTR szPath, PMODULEINFO pmi)
{
    //DebugMsg(DM_TRACE, "_InsertModuleName inserts \"%s\"", szPath);

    pmi->pszModule = NULL;
    Str_SetPtr(&pmi->pszModule, szPath);

    if (pmi->pszModule)
    {
        TCHAR szTemp[10];
        int i;

        pmi->pszModuleName = PathFindFileName(pmi->pszModule);

        //
        // don't insert the module if it's in the [don't load] section!
        //

        GetPrivateProfileString(TEXT("don't load"), pmi->pszModuleName, TEXT(""),
            szTemp, ARRAYSIZE(szTemp), TEXT("control.ini"));

        if (szTemp[0])  // yep, don't put this in the list
        {
            Str_SetPtr(&pmi->pszModule, NULL);
            goto skip;
        }

        //
        // don't insert the module if it's already in the list!
        //

        for (i = DSA_GetItemCount(lpData->hamiModule)-1 ; i >= 0 ; i--)
        {
            PMODULEINFO pmi1 = DSA_GetItemPtr(lpData->hamiModule, i);

            if (!lstrcmpi(pmi1->pszModuleName, pmi->pszModuleName))
            {
                Str_SetPtr(&pmi->pszModule, NULL);
                goto skip;
            }
        }

        DSA_AppendItem(lpData->hamiModule, pmi);
skip:
        ;
    }
}

#define GETMODULE(haminst,i)     ((MINST *)DSA_GetItemPtr(haminst, i))
#define ADDMODULE(haminst,pminst) DSA_AppendItem(haminst, (void *)pminst)

int _LoadCPLModuleAndAdd(ControlData *lpData, LPCTSTR szModule)
{
    int iModule, i;
    LPCPLMODULE pcplm;

    //
    // Load the module and controls (or get the previous one if already
    // loaded).
    //

    iModule = _LoadCPLModule(szModule);

    if (iModule < 0)
    {
        TraceMsg(TF_WARNING, "_LoadCPLModuleAndAdd: _LoadControls refused %s", szModule);
        return -1;
    }

    pcplm = DSA_GetItemPtr(g_hacplmLoaded, iModule);

    //
    // Check if this guy has already loaded this module
    //

    for (i = DSA_GetItemCount(lpData->haminst) - 1; i >= 0; --i)
    {
        const MINST * pminst = GETMODULE(lpData->haminst,i);

        //
        // note: owner id tested last since hinst is more varied
        //

        if ((pminst->hinst == pcplm->minst.hinst) &&
            (pminst->fIs16bit == pcplm->minst.fIs16bit) &&
            (pminst->idOwner == pcplm->minst.idOwner))
        {
FreeThisModule:

            //
            // This guy already loaded this module, so dec
            // the reference and return failure
            //

            _FreeCPLModuleIndex(iModule);
            return(-1);
        }
    }

    //
    // this is a new module, so add it to the list
    //

    if (ADDMODULE(lpData->haminst, &pcplm->minst) < 0)
    {
        goto FreeThisModule;
    }

    return iModule;
}


void _AddItemsFromKey(ControlData *pcd, HKEY hkRoot)
{
    HKEY hkey;
    MODULEINFO mi = {0};

    mi.flags = MI_FIND_FILE;
    
    if (ERROR_SUCCESS == RegOpenKey(hkRoot, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\CPLs"), &hkey))
    {    
        TCHAR szData[MAX_PATH], szValue[128];
        DWORD dwSizeData, dwValue, dwIndex;

        for (dwIndex = 0; 
            ERROR_SUCCESS == (dwValue = ARRAYSIZE(szValue), dwSizeData = sizeof(szData),
                RegEnumValue(hkey, dwIndex, szValue, &dwValue, NULL, NULL, (BYTE *)szData, &dwSizeData));
            dwIndex++)
        {
            TCHAR szPath[MAX_PATH];
            if (SHExpandEnvironmentStrings(szData, szPath, ARRAYSIZE(szPath)))
            {
                WIN32_FIND_DATA fd;
                HANDLE hfind = FindFirstFile(szPath, &fd);
                if (hfind != INVALID_HANDLE_VALUE)
                {
                    mi.ftCreationTime = fd.ftCreationTime;
                    mi.nFileSizeHigh = fd.nFileSizeHigh;
                    mi.nFileSizeLow = fd.nFileSizeLow;
                    FindClose(hfind);

                    _InsertModuleName(pcd, szPath, &mi);
                }
            }
        }
        RegCloseKey(hkey);
    }
}


void _AddItemsFromReg(ControlData *pcd)
{
    _AddItemsFromKey(pcd, HKEY_CURRENT_USER);
    _AddItemsFromKey(pcd, HKEY_LOCAL_MACHINE);
}


/* Get the keynames under [MMCPL] in CONTROL.INI and cycle
   through all such keys to load their applets into our
   list box.  Also allocate the array of CPLMODULE structs.
   Returns early if can't load old WIN3 applets.
*/
BOOL CPLD_GetModules(ControlData *lpData)
{
    LPTSTR       pStr;
    HANDLE   hFindFile;
    WIN32_FIND_DATA findData;
    MODULEINFO  mi;
    TCHAR        szKeys[512];    // from section of extra cpls to load
    TCHAR        szPath[MAX_PATH], szSysDir[MAX_PATH];
    TCHAR        szName[64];
    HANDLE      hdrv;

    ASSERT(lpData->hamiModule == NULL);

    lpData->hamiModule = DSA_Create(SIZEOF(mi), 4);

    if (!lpData->hamiModule)
    {
        return FALSE;
    }

    lpData->haminst = DSA_Create(SIZEOF(MINST), 4);

    if (!lpData->haminst)
    {
        DSA_Destroy(lpData->hamiModule);
        lpData->hamiModule = NULL; // no one is freeing hamiModule in the caller if this fails, but just to make sure ...
        return FALSE;
    }

    //
    // So here's the deal:
    // We have this global list of all modules that have been loaded, along
    // with a reference count for each.  This is so that we don't need to
    // load a CPL file again when the user double clicks on it.
    // We still need to keep a list for each window that is open, so that
    // we will not load the same CPL twice in a single window.  Therefore,
    // we need to keep a list of all modules that are loaded (note that
    // we cannot just keep indexes, since the global list can move around).
    //
    // hamiModule contains the module name, the instance info if loaded,
    // and some other information for comparing with cached information
    //

    ZeroMemory(&mi, SIZEOF(mi));

    //
    // don't special case main, instead sort the data by title
    //

    GetSystemDirectory(szSysDir, ARRAYSIZE(szSysDir));

    //
    // Use our own internal thunks to iterate through the list of
    // drivers.
    //

    hdrv = NULL;

    while (NULL != (hdrv = ShellGetNextDriverName(hdrv, szPath, ARRAYSIZE(szPath))))
    {
        _InsertModuleName(lpData, szPath, &mi);
    }

    //
    // load the modules specified in CONTROL.INI under [MMCPL]
    //

    GetPrivateProfileString(TEXT("MMCPL"), NULL, c_szNULL, szKeys, ARRAYSIZE(szKeys), TEXT("control.ini"));

    for (pStr = szKeys; *pStr; pStr += lstrlen(pStr) + 1)
    {
        GetPrivateProfileString(TEXT("MMCPL"), pStr, c_szNULL, szName, ARRAYSIZE(szName), TEXT("control.ini"));
        if (IsValidCplKey(pStr))
        {
            _InsertModuleName(lpData, szName, &mi);
        }
    }

    //
    // load applets from the system directory
    //

    PathCombine(szPath, szSysDir, TEXT("*.CPL"));

    mi.flags |= MI_FIND_FILE;

    hFindFile = FindFirstFile(szPath, &findData);

    if (hFindFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                PathCombine(szPath, szSysDir, findData.cFileName);

                mi.ftCreationTime = findData.ftCreationTime;
                mi.nFileSizeHigh = findData.nFileSizeHigh;
                mi.nFileSizeLow = findData.nFileSizeLow;

                _InsertModuleName(lpData, szPath, &mi);
            }
        } while (FindNextFile(hFindFile, &findData));

        FindClose(hFindFile);
    }

    _AddItemsFromReg(lpData);
#ifdef WX86

    //
    // For Wx86 we need to also search the wx86 system directory for any
    // x86 CPL files

    NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = TRUE;
    GetSystemDirectory(szSysDir, ARRAYSIZE(szSysDir));
    NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = FALSE;

    PathCombine(szPath, szSysDir, TEXT("*.CPL"));

    mi.flags |= MI_FIND_FILE;

    hFindFile = FindFirstFile(szPath, &findData);

    if (hFindFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                PathCombine(szPath, szSysDir, findData.cFileName);

                mi.ftCreationTime = findData.ftCreationTime;
                mi.nFileSizeHigh = findData.nFileSizeHigh;
                mi.nFileSizeLow = findData.nFileSizeLow;

                _InsertModuleName(lpData, szPath, &mi);
            }
        } while (FindNextFile(hFindFile, &findData));

        FindClose(hFindFile);
    }

#endif

    lpData->cModules = DPA_GetPtrCount(lpData->hamiModule);

    return TRUE;
}


//
//  Read the registry for cached CPL info.
//  If this info is up-to-date with current modules (from CPLD_GetModules),
//  then we can enumerate these without loading the CPLs.
//

// these are extern from control.h
TCHAR const c_szCPLCache[] = REGSTR_PATH_CONTROLSFOLDER;
TCHAR const c_szCPLData[] = TEXT("Presentation Cache");

void CPLD_GetRegModules(ControlData *lpData)
{
    HKEY hkey;

    //
    // default to nothing
    //

    ASSERT(lpData->cRegCPLs == 0);

    //
    // dont cache any-thing in clean boot.
    //

    if (GetSystemMetrics(SM_CLEANBOOT))
        return;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, c_szCPLCache, &hkey))
    {
        DWORD cbSize;
#ifdef WINNT    
        DWORD dwLCID = 0;
        cbSize = sizeof( dwLCID);

        // fail if this cache is not tagged with the UI language ID, or we are not in
        // the language ID that the cache was saved in.
        if (ERROR_SUCCESS != SHQueryValueEx(hkey, TEXT("Presentation LCID"),
                NULL, NULL, (LPBYTE) &dwLCID, &cbSize ) || dwLCID != DelayGetUserDefaultUILanguage())
        {
            RegCloseKey( hkey );
            return;
        }
#endif
        if (ERROR_SUCCESS == SHQueryValueEx(hkey, c_szCPLData,
                NULL, NULL, NULL, &cbSize))
        {
            lpData->pRegCPLBuffer = LocalAlloc(LPTR, cbSize);

            if (lpData->pRegCPLBuffer)
            {
                if (ERROR_SUCCESS == SHQueryValueEx(hkey, c_szCPLData,
                        NULL, NULL, lpData->pRegCPLBuffer, &cbSize))
                {
                    lpData->hRegCPLs = DPA_Create(4);

                    if (lpData->hRegCPLs)
                    {
                        RegCPLInfo * p;
                        DWORD cbOffset;

                        for ( cbOffset = 0          ;
                              cbOffset < cbSize     ;
                              cbOffset += p->cbSize )
                        {
                            p = (PRegCPLInfo)&(lpData->pRegCPLBuffer[cbOffset]);
                            p->flags |= REGCPL_FROMREG;
                            DPA_AppendPtr(lpData->hRegCPLs, p);

                            //DebugMsg(DM_TRACE,"sh CPLD_GetRegModules: %s (%s)", REGCPL_FILENAME(p), REGCPL_CPLNAME(p));
                        }

                        lpData->cRegCPLs = DPA_GetPtrCount(lpData->hRegCPLs);
                    }
                }
                else
                {
                    TraceMsg(TF_WARNING, "CPLD_GetRegModules: failed read!");
                }
            } // Alloc
        } // SHQueryValueEx for size

        RegCloseKey(hkey);

    } // RegOpenKey
}


//
// On a typical system, we will successfully cache all the CPLs.  So this
// function will write out the data only once.
//

void CPLD_FlushRegModules(ControlData *lpData)
{
  if (lpData->fRegCPLChanged)
  {
    int         num = DPA_GetPtrCount(lpData->hRegCPLs);
    DWORD       cbSize = num * SIZEOF(RegCPLInfo);
    PRegCPLInfo prcpli = LocalAlloc(LPTR, cbSize);

    if (prcpli)
    {
        RegCPLInfo * pDest;
        HKEY hkey;
        int i;

        //
        // 0<=i<=num && CPLs 0..i-1 have been copied to prcpli or skipped
        //

        for (i = 0 , pDest = prcpli ; i < num ; )
        {
            PRegCPLInfo p = DPA_GetPtr(lpData->hRegCPLs, i);
            int j;

            //
            // if any CPL in this module has a dynamic icon, we cannot cache
            // any of this module's CPLs.
            //
            // i<=j<=num && CPLs i..j-1 are in same module
            //

            for (j = i ; j < num ; j++)
            {
                PRegCPLInfo q = DPA_GetPtr(lpData->hRegCPLs, j);

                if (lstrcmp(REGCPL_FILENAME(p), REGCPL_FILENAME(q)))
                {
                    //
                    // all CPLs in this module are okay, save 'em
                    //

                    break;
                }

                if (q->idIcon == 0)
                {
                    TraceMsg(TF_GENERAL, "CPLD_FlushRegModules: SKIPPING %s (%s) [dynamic icon]",REGCPL_FILENAME(p),REGCPL_CPLNAME(p));

                    //
                    // this module has a dynamic icon, skip it
                    //

                    for (j++ ; j < num ; j++)
                    {
                        q = DPA_GetPtr(lpData->hRegCPLs, j);
                        if (lstrcmp(REGCPL_FILENAME(p), REGCPL_FILENAME(q)))
                            break;
                    }
                    i = j;
                    break;
                }
            }

            //
            // CPLs i..j-1 are in the same module and need to be saved
            // (if j<num, CPL j is in the next module)
            //
            for ( ; i < j ; i++)
            {
                p = DPA_GetPtr(lpData->hRegCPLs, i);

                hmemcpy(pDest, p, p->cbSize);
                pDest = (RegCPLInfo *)(((LPBYTE)pDest) + pDest->cbSize);
                //DebugMsg(DM_TRACE,"CPLD_FlushRegModules: %s (%s)",REGCPL_FILENAME(p),REGCPL_CPLNAME(p));
            }
        } // for (i=0,pDest=prcpli


        //
        // prcpli contains packed RegCPLInfo structures to save to the registry
        //

        if (ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_USER, c_szCPLCache, &hkey))
        {
#ifdef WINNT
            DWORD dwLCID;
            DWORD dwSize = sizeof( dwLCID );
            dwLCID = DelayGetUserDefaultUILanguage();
            
            if (ERROR_SUCCESS != RegSetValueEx(hkey, TEXT("Presentation LCID"), 0, REG_DWORD, (LPBYTE) &dwLCID, dwSize ))
            {
                TraceMsg(TF_WARNING, "CPLD_FLushRegModules: failed to write the LCID!");
            }
#endif
            if (ERROR_SUCCESS != RegSetValueEx(hkey, c_szCPLData, 0, REG_BINARY, (LPBYTE)prcpli, (DWORD) ((LPBYTE)pDest-(LPBYTE)prcpli)))
            {
                TraceMsg(TF_WARNING, "CPLD_FLushRegModules: failed write!");
            }
            RegCloseKey(hkey);
        }

        LocalFree((HLOCAL)prcpli);

        lpData->fRegCPLChanged = FALSE; // no longer dirty
    } // if (prcpli)
  } // if dirty
}


//---------------------------------------------------------------------------
void CPLD_Destroy(ControlData *lpData)
{
    int i;

    if (lpData->haminst)
    {
        for (i=DSA_GetItemCount(lpData->haminst)-1 ; i>=0 ; --i)
            _FreeCPLModuleHandle(DSA_GetItemPtr(lpData->haminst, i));

        DSA_Destroy(lpData->haminst);
    }

    if (lpData->hamiModule)
    {
        for (i=DSA_GetItemCount(lpData->hamiModule)-1 ; i>=0 ; --i)
        {
            PMODULEINFO pmi = DSA_GetItemPtr(lpData->hamiModule, i);

            Str_SetPtr(&pmi->pszModule, NULL);
        }

        DSA_Destroy(lpData->hamiModule);
    }

    if (lpData->hRegCPLs)
    {
        CPLD_FlushRegModules(lpData);

        for (i = DPA_GetPtrCount(lpData->hRegCPLs)-1 ; i >= 0 ; i--)
        {
            PRegCPLInfo p = DPA_GetPtr(lpData->hRegCPLs, i);
            if (!(p->flags & REGCPL_FROMREG))
                LocalFree((HLOCAL)p);
        }
        DPA_Destroy(lpData->hRegCPLs);
    }
    if (lpData->pRegCPLBuffer)
        LocalFree((HLOCAL)lpData->pRegCPLBuffer);
}


//
// Loads module lpData->hamiModule[nModule] and returns # cpls in module
int CPLD_InitModule(ControlData *lpData, int nModule, MINST *pminst)
{
    PMODULEINFO pmi;
    LPCPLMODULE pcplm;
    int iModule;

    pmi = DSA_GetItemPtr(lpData->hamiModule, nModule);

    iModule = _LoadCPLModuleAndAdd(lpData, pmi->pszModule);

    if (iModule < 0)
    {
        return(0);
    }

    pcplm = DSA_GetItemPtr(g_hacplmLoaded, iModule);
    *pminst = pcplm->minst;

    return DSA_GetItemCount(pcplm->hacpli);
}

void CPLD_AddControlToReg(ControlData *lpData, const MINST * pminst, int nControl)
{
    int iModule;
    LPCPLMODULE pcplm;
    LPCPLITEM  pcpli;

    TCHAR buf[MAX_PATH];
    HANDLE hFindFile;
    WIN32_FIND_DATA findData;


    iModule = _FindCPLModule(pminst);
    pcplm  = DSA_GetItemPtr(g_hacplmLoaded, iModule);
    pcpli = DSA_GetItemPtr(pcplm->hacpli, nControl);

    //
    // BUGBUG: Why are we using GetModuleFileName instead of the name
    // of the file we used to load this module?  (We have the name both
    // in the calling function and in lpData.)
    //

    if (pminst->fIs16bit)
    {
        GetModuleFileName16(pcplm->minst.hinst, buf, MAX_PATH);
    }
    else
    {
        GetModuleFileName(pcplm->minst.hinst, buf, MAX_PATH);
    }

    hFindFile = FindFirstFile(buf, &findData);

    if (hFindFile != INVALID_HANDLE_VALUE)
    {
        PRegCPLInfo prcpli = LocalAlloc(LPTR, SIZEOF(RegCPLInfo));

        FindClose(hFindFile);

        if (prcpli)
        {
            lstrcpy(REGCPL_FILENAME(prcpli), buf);

            prcpli->flags = FALSE;
            prcpli->ftCreationTime = findData.ftCreationTime;
            prcpli->nFileSizeHigh = findData.nFileSizeHigh;
            prcpli->nFileSizeLow = findData.nFileSizeLow;

            prcpli->idIcon = pcpli->idIcon;

            prcpli->oName = lstrlen(REGCPL_FILENAME(prcpli)) + 1;

            lstrcpy(REGCPL_CPLNAME(prcpli), pcpli->pszName);

            prcpli->oInfo = prcpli->oName + lstrlen(REGCPL_CPLNAME(prcpli)) + 1;

            lstrcpy(REGCPL_CPLINFO(prcpli), pcpli->pszInfo);

            prcpli->cbSize = FIELD_OFFSET(RegCPLInfo, buf) + (prcpli->oInfo
                                            + lstrlen(REGCPL_CPLINFO(prcpli))
                                            + 1) * SIZEOF(TCHAR);

            //
            // Force struct size to be DWORD aligned since these are packed
            // together in registry, then read and accessed after reading
            // cache from registry.
            //

            if (prcpli->cbSize & 3)
                prcpli->cbSize += SIZEOF(DWORD) - (prcpli->cbSize & 3);

            if (!lpData->hRegCPLs)
            {
                lpData->hRegCPLs = DPA_Create(4);
            }
            if (lpData->hRegCPLs)
            {
                DPA_AppendPtr(lpData->hRegCPLs, prcpli);

                //
                // don't update cRegCPLs.  We don't need it any more, and
                // it is also the upper-end counter for ESF_Next registry enum.
                //lpData->cRegCPLs++;
                //

                lpData->fRegCPLChanged = TRUE;
            }
            else
                LocalFree((HLOCAL)prcpli);
        }
    }
}
