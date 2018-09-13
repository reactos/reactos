//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       magic.cxx
//
//  Contents:   Stack backtracing functions
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_IMAGEHLP_H_
#define X_IMAGEHLP_H_
#include <imagehlp.h>
#endif

//$ WIN64: Needs to use 64-bit APIs for ImageHelp

BOOL       g_fLoadedImageHlp = FALSE;
HINSTANCE  g_hinstImageHlp   = NULL;
BOOL       g_fOSIsNT         = FALSE;

ExternTag(tagMagic);

//
// Critical Section used to serialize access to imagehlp.dll
//
void TestStackTrace(void);

// Function Pointers to APIs in IMAGEHLP.DLL. Loaded dynamically.
//
typedef LPAPI_VERSION (__stdcall *pfnImgHlp_ImagehlpApiVersionEx)(
    LPAPI_VERSION AppVersion
    );

typedef BOOL (__stdcall *pfnImgHlp_StackWalk)(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME                      StackFrame,
    LPVOID                            ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE          GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress
    );

typedef BOOL (__stdcall *pfnImgHlp_SymGetModuleInfo)(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PIMAGEHLP_MODULE    ModuleInfo
    );

typedef LPVOID (__stdcall *pfnImgHlp_SymFunctionTableAccess)(
    HANDLE  hProcess,
    DWORD   AddrBase
    );

typedef BOOL (__stdcall *pfnImgHlp_SymGetSymFromAddr)(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PDWORD              pdwDisplacement,
    OUT PIMAGEHLP_SYMBOL    Symbol
    );

typedef BOOL (__stdcall *pfnImgHlp_SymInitialize)(
    IN HANDLE   hProcess,
    IN LPSTR    UserSearchPath,
    IN BOOL     fInvadeProcess
    );

typedef BOOL (__stdcall *pfnImgHlp_SymLoadModule)(
    IN  HANDLE          hProcess,
    IN  HANDLE          hFile,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD           BaseOfDll,
    IN  DWORD           SizeOfDll
    );

typedef DWORD (__stdcall *pfnImgHlp_SymSetOptions)(
    IN  DWORD           SymOptions
    );

pfnImgHlp_ImagehlpApiVersionEx    _ImagehlpApiVersionEx;
pfnImgHlp_StackWalk               _StackWalk;
pfnImgHlp_SymGetModuleInfo        _SymGetModuleInfo;
pfnImgHlp_SymFunctionTableAccess  _SymFunctionTableAccess;
pfnImgHlp_SymGetSymFromAddr       _SymGetSymFromAddr;
pfnImgHlp_SymInitialize           _SymInitialize;
pfnImgHlp_SymLoadModule           _SymLoadModule;
pfnImgHlp_SymSetOptions           _SymSetOptions;


struct IMGHLPFN_LOAD
{
    LPSTR   pszFnName;
    LPVOID * ppvfn;
};

IMGHLPFN_LOAD ailFuncList[] =
{
    { "ImagehlpApiVersionEx",   (LPVOID*)&_ImagehlpApiVersionEx },
    { "StackWalk",              (LPVOID*)&_StackWalk },
    { "SymGetModuleInfo",       (LPVOID*)&_SymGetModuleInfo },
    { "SymFunctionTableAccess", (LPVOID*)&_SymFunctionTableAccess },
    { "SymGetSymFromAddr",      (LPVOID*)&_SymGetSymFromAddr },
    { "SymInitialize",          (LPVOID*)&_SymInitialize },
    { "SymLoadModule",          (LPVOID*)&_SymLoadModule },
    { "SymSetOptions",          (LPVOID*)&_SymSetOptions },
};


//+---------------------------------------------------------------------------
//
//  Function:   MagicInit
//
//  Synopsis:   Initializes the symbol loading code.
//
//----------------------------------------------------------------------------

void
MagicInit(void)
{
    OSVERSIONINFOA ovi;
    API_VERSION    AppVersion = { 4, 0, API_VERSION_NUMBER, 0 };
    PSTR           pszModPath;
    char           achBuf[MAX_PATH*3];
    char           achBuf2[MAX_PATH];
    char        *  pch;

    int i;

    TraceTag((tagMagic, "MagicInit: Starting."));

    //
    // Get what operating system we're on.
    //
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    Verify(GetVersionExA(&ovi));

    g_fOSIsNT = (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT);

    //
    // Try to load imagehlp.dll
    //
    g_hinstImageHlp = LoadLibraryA("imagehlp.dll");

    if (!g_hinstImageHlp)
    {
        TraceTag((tagWarning, "IMAGEHLP.DLL could not be loaded."));

        return;
    }

    //
    // Try to get the API entrypoints in imagehlp.dll
    //
    for (i=0; i < ARRAY_SIZE(ailFuncList); i++)
    {
        *(ailFuncList[i].ppvfn) = GetProcAddress(g_hinstImageHlp,
                                                 ailFuncList[i].pszFnName);
        if (!*(ailFuncList[i].ppvfn))
        {
            return;
        }
    }

    LPAPI_VERSION papiver = _ImagehlpApiVersionEx(&AppVersion);

    //
    // Verify that imagehlp.dll is a version we're compatible with.
    //
    if ((papiver->Revision < 6) || (papiver->Revision > 8))
    {
        TraceTag((tagError, "IMAGEHLP.DLL version mismatch (%ld "
            "vs 6-8 (Header version: %d).", papiver->Revision, API_VERSION_NUMBER));
        return;
    }

    g_fLoadedImageHlp = TRUE;


    //
    // ImageHlp doesn't look it very good places for the symbols by default,
    // so here we construct two paths based on where this DLL is located. We
    // set the symbol search path to:
    //
    //       "<dllpath>;<dllpath>\symbols\retail\dll;<windir>\symbols\dll"
    //
    pszModPath = NULL;
    achBuf[0] = 0;

    if (GetModuleFileNameA(g_hinstMain, achBuf, sizeof(achBuf)))
    {
        pch = strrchr(achBuf, '\\');
        if (pch)
        {
            *pch = '\0';
        }

        strcat(achBuf, ";");

        strncat(achBuf, achBuf, strlen(achBuf)-1);

        strcat(achBuf, "\\symbols\\retail\\dll");

        pszModPath = achBuf;
    }

    if (GetWindowsDirectoryA(achBuf2, MAX_PATH))
    {
        strcat(achBuf, ";");
        strcat(achBuf, achBuf2);
        strcat(achBuf, "\\symbols\\dll");
        pszModPath = achBuf;
    }

    _SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

    //
    // Initialize imagehlp.dll
    //
    TraceTag((tagMagic, "Symbol search path is %s", pszModPath));
    if (!_SymInitialize(g_hProcess, pszModPath, FALSE))
    {
        g_fLoadedImageHlp = FALSE;
    }

    // TestStackTrace();

    TraceTag((tagMagic, "MagicInit: Returning."));

    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   MagicDeinit
//
//  Synopsis:   Cleans up for the symbol loading code.
//
//----------------------------------------------------------------------------

void
MagicDeinit(void)
{
    if (g_hinstImageHlp)
    {
        FreeLibrary(g_hinstImageHlp);

        g_hinstImageHlp   = NULL;
        g_fLoadedImageHlp = FALSE;
    }
}

//
// WARNING -- All the functions between here and GetStackBactrace should
// not be called from outside this file! Otherwise this code will not be
// reentrant!
//

//+---------------------------------------------------------------------------
//
//  Function:   AreSymbolsEnabled
//
//  Synopsis:   Returns TRUE if symbols are enabled.
//
//----------------------------------------------------------------------------

BOOL
AreSymbolsEnabled(void)
{
    return (    g_fLoadedImageHlp
            &&  (   DbgExIsTagEnabled(tagSymbols)
                ||  (   DbgExIsTagEnabled(tagSpySymbols)
                    &&  DbgGetThreadState()
                    &&  TLS(fSpyAlloc))));
}

DWORD
LoadModuleFromVMemData(HANDLE hProcess, DWORD dwAddr)
{
    MEMORY_BASIC_INFORMATION mbi;
    char                     achFile[MAX_PATH] = {0};
    DWORD                    cch = 0;

    if (VirtualQueryEx(hProcess, (LPVOID)(DWORD_PTR)dwAddr, &mbi, sizeof(mbi)))
    {
        if (!g_fOSIsNT || (mbi.Type & MEM_IMAGE))
        {
            cch = GetModuleFileNameA((HINSTANCE)mbi.AllocationBase,
                                     achFile,
                                     MAX_PATH);

            TraceTag((tagMagic, "Module Filename is %s", achFile));

            // Ignore the return code since we can't do anything with it.
            if (!_SymLoadModule(hProcess,
                           NULL,
                           ((cch) ? achFile : NULL),
                           NULL,
                           (DWORD)(DWORD_PTR)mbi.AllocationBase,
                           0))
            {
                TraceTag((tagMagic, "SymLoadModule failed with %d", GetLastError()));
            }
            else
            {
                TraceTag((tagMagic, "SymLoadModule succeeded"));
            }

            return (DWORD)(DWORD_PTR)mbi.AllocationBase;
        }
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   FillSymbolInfo
//
//  Synopsis:   Fills in a SYMBOL_INFO struct
//
//  Arguments:  [psi]    -- SYMBOL_INFO to fill
//              [dwAddr] -- Address to get symbols of.
//
//----------------------------------------------------------------------------

void
FillSymbolInfo(SYMBOL_INFO *psi, DWORD dwAddr)
{
    union {
        CHAR rgchSymbol[sizeof(IMAGEHLP_SYMBOL) + 255];
        IMAGEHLP_SYMBOL  sym;
    };
    CHAR * pszSymbol = NULL;
    IMAGEHLP_MODULE  mi;
    BOOL             fFoundModule = TRUE;

    Assert(psi);

    memset(psi, 0, sizeof(SYMBOL_INFO));

    if (!g_fLoadedImageHlp)
    {
        TraceTag((tagMagic, "ImageHlp not loaded! Bailing out of FillSymbolInfo"));
        return;
    }

    memset(&mi, 0, sizeof(mi));
    mi.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

    if (!_SymGetModuleInfo(g_hProcess, dwAddr, &mi))
    {
        TraceTag((tagMagic, "SymGetModuleInfo failed with %d", GetLastError()));
        //
        // Attempt to load the module here if it hasn't been loaded yet.
        //
        // We do it twice because SymGetModuleInfo seems to not always get it
        // the first time! - WHY? (lylec)
        //
        LoadModuleFromVMemData(g_hProcess, dwAddr);
        LoadModuleFromVMemData(g_hProcess, dwAddr);

        if (!_SymGetModuleInfo(g_hProcess, dwAddr, &mi))
        {
            TraceTag((tagMagic, "SymGetModuleInfo failed AGAIN with %d", GetLastError()));

            strncpy(psi->achModule, "<no module>", sizeof(psi->achModule)-1);

            fFoundModule = FALSE;
        }
    }

    if (fFoundModule)
    {
        psi->achModule[0] = '[';
        strncpy(&psi->achModule[1], mi.ModuleName, sizeof(psi->achModule)-3);
        strcat(psi->achModule, "]");

        TraceTag((tagMagic, "SymGetModuleInfo returned module %s", psi->achModule));
    }

    sym.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    sym.MaxNameLength = 255;

    if (_SymGetSymFromAddr(g_hProcess, dwAddr, &psi->dwOffset, &sym))
    {
        pszSymbol = sym.Name;
    }
    else
    {
        TraceTag((tagMagic, "SymGetSymFromAddr failed with %d", GetLastError()));
        pszSymbol = "<no symbol>";
    }

    strncpy(psi->achSymbol, pszSymbol, ARRAY_SIZE(psi->achSymbol)-1);
}

//+---------------------------------------------------------------------------
//
//  Helpers for imagehlp's StackWalk API.
//
//----------------------------------------------------------------------------

LPVOID
FormsFunctionTableAccess(HANDLE hProcess, DWORD_PTR dwPCAddr)
{
    return _SymFunctionTableAccess( hProcess, dwPCAddr );
}


//+---------------------------------------------------------------------------
//
//  Function:   FormsGetModuleBase
//
//  Synopsis:   Retrieves the base address of the module containing the given
//              virtual address.
//
//  Arguments:  [hProcess]      -- Process
//              [ReturnAddress] -- Virtual address to get base of.
//
//  Notes:      If the module information for the given module has not yet
//              been loaded, then it is loaded.
//
//----------------------------------------------------------------------------

DWORD_PTR
FormsGetModuleBase(HANDLE hProcess, DWORD_PTR ReturnAddress)
{
    IMAGEHLP_MODULE ModuleInfo;
    DWORD           dwBaseAddr;

    memset(&ModuleInfo, 0, sizeof(IMAGEHLP_MODULE));
    ModuleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

    if (_SymGetModuleInfo(hProcess, ReturnAddress, &ModuleInfo))
    {
        dwBaseAddr = ModuleInfo.BaseOfImage;
    }
    else
    {
        dwBaseAddr = LoadModuleFromVMemData(hProcess, ReturnAddress);
    }

    return dwBaseAddr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetStackBacktrace
//
//  Synopsis:   Gets a stacktrace of the current stack, including symbols.
//
//  Arguments:  [ifrStart] -- How many stack elements to skip before starting.
//              [cfrTotal] -- How many elements to trace after starting
//              [pdwEip]   -- Array to be filled with stack addresses.
//              [psiSym]   -- This array is filled with symbol information.
//                             It should be big enough to hold cfrTotal elts.
//                             If NULL, no symbol information is stored.
//
//  Returns:    Number of elements actually retrieved.
//
//----------------------------------------------------------------------------

int
GetStackBacktrace(int             ifrStart,
                  int             cfrTotal,
                  DWORD *         pdwEip,
                  SYMBOL_INFO *   psiSymbols)
{
    HANDLE        hThread;
    CONTEXT       context;
    STACKFRAME    stkfrm;
    DWORD         dwMachType;
    int           i;
    DWORD       * pdw        = pdwEip;
    SYMBOL_INFO * psi        = psiSymbols;
    BOOL          fNeedLeave = TRUE;

    EnsureThreadState();

    TraceTag((tagMagic, "Entering GetStackBacktrace"));

    memset(pdwEip, 0, cfrTotal * sizeof(DWORD));

    if (psiSymbols)
    {
        memset(psiSymbols, 0, cfrTotal * sizeof(SYMBOL_INFO));
    }

    if (!g_fLoadedImageHlp)
    {
        TraceTag((tagMagic, "ImageHlp not loaded! Bailing out of GetStackBacktrace!"));

        return 0;
    }

    hThread  = GetCurrentThread();

#if !defined(UNIX) && !defined(_M_IA64)                    // IEUNIX BUGBUG will something similar be needed ?
    context.ContextFlags = CONTEXT_FULL;
#endif

    // Officially, GetThreadContext can't be called on the running thread.
    if (GetThreadContext(hThread, &context))
    {
        memset(&stkfrm, 0, sizeof(STACKFRAME));

#if defined(_M_IX86)
        dwMachType = IMAGE_FILE_MACHINE_I386;

#if 0
        DWORD         dwEIP;
        DWORD         dwESP;
        DWORD         dwEBP;

        _asm
        {
                // Hack getting EIP into a register
                call label
        label:
                // Pop EIP into EAX
                pop eax
                // Store EIP into dwEIP
                mov [dwEIP], eax    ;
                mov [dwEBP], ebp        ; Move EBP into dwEBP
                mov [dwESP], esp        ; Move ESP into dwESP
        }

        // Initialize the STACKFRAME structure
        stkfrm.AddrPC.Offset    = dwEIP;
        stkfrm.AddrPC.Mode      = AddrModeFlat;
        stkfrm.AddrStack.Offset = dwESP;
        stkfrm.AddrStack.Mode   = AddrModeFlat;
        stkfrm.AddrFrame.Offset = dwEBP;
        stkfrm.AddrFrame.Mode   = AddrModeFlat;
#else
        stkfrm.AddrPC.Offset    = context.Eip;  // Program Counter
        stkfrm.AddrPC.Mode      = AddrModeFlat;
        stkfrm.AddrStack.Offset = context.Esp;  // Stack Pointer
        stkfrm.AddrStack.Mode   = AddrModeFlat;
        stkfrm.AddrFrame.Offset = context.Ebp;  // Frame Pointer
        stkfrm.AddrFrame.Mode   = AddrModeFlat;
#endif
#elif defined(_M_MRX000)
        dwMachType              = IMAGE_FILE_MACHINE_R4000;
        stkfrm.AddrPC.Offset    = context.Fir;  // Program Counter
        stkfrm.AddrPC.Mode      = AddrModeFlat;
#elif defined(_M_AXP64)
        dwMachType              = IMAGE_FILE_MACHINE_ALPHA64;
        stkfrm.AddrPC.Offset    = context.Fir;  // Program Counter
        stkfrm.AddrPC.Mode      = AddrModeFlat;
#elif defined(_M_ALPHA)
        dwMachType              = IMAGE_FILE_MACHINE_ALPHA;
        stkfrm.AddrPC.Offset    = (unsigned long) context.Fir;  // Program Counter
        stkfrm.AddrPC.Mode      = AddrModeFlat;
#elif defined(_M_PPC)
        dwMachType              = IMAGE_FILE_MACHINE_POWERPC;
        stkfrm.AddrPC.Offset    = context.Iar;  // Program Counter
        stkfrm.AddrPC.Mode      = AddrModeFlat;
#elif defined(UNIX)
        dwMachType              = (DWORD) -1; // IEUNIX fill in correctly
        stkfrm.AddrPC.Offset    = 0; // IEUNIX fill in correctly
        stkfrm.AddrPC.Mode      = AddrModeFlat;
#elif defined(_M_IA64)
        dwMachType              = IMAGE_FILE_MACHINE_IA64;
        stkfrm.AddrPC.Offset    = 0; //$WIN64 (IA64) Which is the program counter register? 
        stkfrm.AddrPC.Mode      = AddrModeFlat;
#else
#error("Unknown Target Machine");
#endif

        //
        // We have to use a critical section because MSPDB50.DLL (and
        // maybe imagehlp.dll) is not reentrant and simultaneous calls
        // to StackWalk cause it to tromp on its own memory.
        //
        EnterCriticalSection(&g_csHeapHack);

        // Check this again just in case another thread failed and caused
        // imagehlp.dll to be unloaded.

        if (!g_fLoadedImageHlp)
        {
            return 0;
        }

        __try
        {
            for (i = 0; i < ifrStart + cfrTotal; i++)
            {
                if (!_StackWalk(dwMachType,
                                g_hProcess,
                                hThread,
                                &stkfrm,
                                &context,
                                NULL,
                                FormsFunctionTableAccess,
                                FormsGetModuleBase,
                                NULL))
                {
                    break;
                }

                if (i >= ifrStart)
                {
                    *pdw++ = stkfrm.AddrPC.Offset;

                    if (psi)
                    {
                        FillSymbolInfo(psi++, stkfrm.AddrPC.Offset);
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Something blew up. This can happen if someone asserts during
            // process cleanup or just because things are hosed. In this case
            // we empty our return data, unload imagehlp.dll so we don't try
            // any of this again, and return 0.
            //
            memset(pdwEip, 0, cfrTotal * sizeof(DWORD));

            if (psiSymbols)
            {
                memset(psiSymbols, 0, cfrTotal * sizeof(SYMBOL_INFO));
            }

            // Block anyone else from using imagehlp.

            g_fLoadedImageHlp = FALSE;

            LeaveCriticalSection(&g_csHeapHack);

            fNeedLeave = FALSE;

            // This deletes the critical section
            MagicDeinit();

            pdw = pdwEip; // Will cause us to return 0
        }
        __endexcept
    }

    if (fNeedLeave)
    {
        LeaveCriticalSection(&g_csHeapHack);
    }

    return pdw - pdwEip;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetStringFromSymbolInfo
//
//  Synopsis:   Returns a string from an SYMBOL_INFO
//
//  Arguments:  [dwAddr]    -- Memory Address corresponding to [psi]
//              [psi]       -- Pointer to SYMBOL_INFO. Can be NULL.
//              [pszString] -- Place to put string
//
//----------------------------------------------------------------------------

void
GetStringFromSymbolInfo(DWORD dwAddr, SYMBOL_INFO *psi, CHAR *pszString)
{
    if (psi)
    {

        wsprintfA(pszString,
                 "  %08x  %-11hs %hs + 0x%x",
                 dwAddr,
                 (psi->achModule[0]) ? psi->achModule : "<no module>",
                 (psi->achSymbol[0]) ? psi->achSymbol : "<no symbol>",
                 psi->dwOffset);
    }
    else
    {
        wsprintfA(pszString, "  %08x <symbols not available>", dwAddr);
    }
}

void WINAPI
DbgExGetSymbolFromAddress(void * pvAddr, char * pszBuf, DWORD cchBuf)
{
    SYMBOL_INFO     si;
    char            ach[512];

    ach[0] = 0;

    FillSymbolInfo(&si, (DWORD)(DWORD_PTR)pvAddr);
    GetStringFromSymbolInfo((DWORD)(DWORD_PTR)pvAddr, &si, ach);
    strncpy(pszBuf, ach, cchBuf - 1);
    pszBuf[cchBuf - 1] = 0;
}

// -----------------
//
// Stacktrace Testing Code
//
// -----------------

int
Alpha(int n, int cdw, DWORD * pdw, SYMBOL_INFO *psi)
{
    int     c;

    if (n > 0)
    {
        c = Alpha(n - 1, cdw, pdw, psi);
    }
    else
    {
        c = GetStackBacktrace(1, cdw, pdw, psi);
    }

    return c;
}


class CBeta
{
public:
    void        Foo(void);
    STDMETHOD(Bar) (int x, int y);
};

void
CBeta::Foo( )
{
    Bar(1, 2);
}

STDMETHODIMP
CBeta::Bar(int x, int y)
{
    int              i;
    int              c;
    DWORD            dwEip[8];
    SYMBOL_INFO      asiSym[8];
    CHAR             achSymbol[256];

    TraceTag((0, "CBeta::Bar"));

    c = GetStackBacktrace(1, ARRAY_SIZE(dwEip), dwEip, asiSym);
    for (i = 0; i < c; i++)
    {
        GetStringFromSymbolInfo(dwEip[i], &asiSym[i], achSymbol);

        TraceTag((0, "%s", achSymbol));
    }

    return S_OK;
}


void
TestStackTrace( )
{
    int              i;
    int              c;
    DWORD            dwEip[8];
    SYMBOL_INFO      asiSym[8];
    CBeta            Beta;
    CHAR             achSymbol[256];

    static BOOL  fRepeat = FALSE;

    if (fRepeat)
    {
        return;
    }

    fRepeat = TRUE;

    TraceTag((0, "GetStackBacktrace"));

    c = GetStackBacktrace(1, ARRAY_SIZE(dwEip), dwEip, asiSym);
    for (i = 0; i < c; i++)
    {
        GetStringFromSymbolInfo(dwEip[i], &asiSym[i], achSymbol);

        TraceTag((0, "%s", achSymbol));
    }

    TraceTag((0, "Alpha(3)"));

    c = Alpha(3, ARRAY_SIZE(dwEip), dwEip, asiSym);
    for (i = 0; i < c; i++)
    {
        GetStringFromSymbolInfo(dwEip[i], &asiSym[i], achSymbol);

        TraceTag((0, "%s", achSymbol));
    }

    Beta.Foo();
}

