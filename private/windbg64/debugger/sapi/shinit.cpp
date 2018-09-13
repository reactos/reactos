//  shinit
//
//  Copyright <C> 1990-94, Microsoft Corporation
//
//      [00] 31-dec-91 DavidGra
//          Add SHFindSymbol API for assembler symbol handling.
//
//      10-Nov-94   BryanT
//          Merge in NT changes.
//          Remove SHF ifdef's, remove non-Win32 hosting, remove FAR/NEAR/PASCAL.
//          Replace SHCritxxx calls with native CritSec calls.
//          Initialize the SymCvt pointers.
//          Use the DBG version macros if NT_BUILD
//          Add the background symbol support, but leave it under if 0 for now.

#include "shinc.hpp"
#pragma hdrstop
#include "version.h"

CRITICAL_SECTION    csSh;   // Global CritSec used for MT safe.
KNF                 knf;    // Kernel functions (init to zero by the loader)
HMODULE             hLib;   // Handle returned from LoadLibrary on symcvt
CONVERTPROC         pfConvertSymbolsForImage; // Symcvt ptr.

static SHF shf = {
    sizeof(SHF),
    SHCreateProcess,
    SHSetHpid,
    SHDeleteProcess,
    SHChangeProcess,
    SHAddDll,
    SHAddDllsToProcess,
    SHLoadDll,
    SHUnloadDll,
    SHGetDebugStart,
    SHGetSymName,
    SHAddrFromHsym,
    SHHmodGetNextGlobal,
    SHModelFromAddr,
    SHPublicNameToAddr,
    SHGetSymbol,
    PHGetAddr,
    SHIsLabel,

    SHSetDebuggeeDir,
    SHAddrToLabel,

    SHGetSymLoc,
    SHFIsAddrNonVirtual,
    SHIsFarProc,

    SHGetNextExe,
    SHHexeFromHmod,
    SHGetNextMod,
    SHGetCxtFromHmod,
    SHSetCxt,
    SHSetCxtMod,
    SHFindNameInGlobal,
    SHFindNameInContext,
    SHGoToParent,
    SHHsymFromPcxt,
    SHNextHsym,
    NULL,                       // SHGetFuncCXF
    SHGetModName,
    SHGetExeName,
    SHGethExeFromName,
    SHGetNearestHsym,
    SHIsInProlog,
    SHIsAddrInCxt,
    SHCompareRE,                // SHCompareRE
    SHFindSymbol,
    PHGetNearestHsym,
    PHFindNameInPublics,
    THGetTypeFromIndex,
    THGetNextType,
    SHLpGSNGetTable,
    SHCanDisplay,

    // Source Line Handler API

    SLLineFromAddr,
    SLFLineToAddr,
    SLNameFromHsf,
    SLNameFromHmod,
    SLFQueryModSrc,
    NULL,
    SLHsfFromPcxt,
    SLHsfFromFile,
    SLCAddrFromLine,
    SHFree,
    SHUnloadSymbolHandler,
    SHGetExeTimeStamp,
    SHPdbNameFromExe,
    SHGetDebugData,
    SHIsThunk,
    SHFindSymInExe,
    SHFindSLink32,
    SHIsEmiLoaded,

// Entries added for NT work.

    SHGetModule,
    SHGetCxtFromHexe,
    SHGetModNameFromHexe,
    SHGetSymFName,
    SHGethExeFromModuleName,
    SHLszGetErrorText,
    SHWantSymbols,
    SHFindNameInTypes,
#if CC_LAZYTYPES
    THAreTypesEqual,
#endif
    SHSymbolsLoaded,
    SHSymbolsLoadError,
    SHUnloadSymbols,
};

VOID
SHFree(
    LPV lpv
    )
{
    MHFree (lpv);
}

BOOL
SHInit(
    LPSHF  *lplpshf,
    LPKNF   lpknf
    )
{
    //
    //  Make a copy of the comming support routines and give back the
    //  symbol handler routines we are supplying
    //

    knf = *lpknf;
    *lplpshf = &shf;

    //
    // Initialize symbol convert pointers.
    //

    hLib = (HMODULE) LoadLibrary( "symcvt.dll" );
    if (hLib != NULL) {
        pfConvertSymbolsForImage = (CONVERTPROC) GetProcAddress( hLib, "ConvertSymbolsForImage" );
    }

    InitializeCriticalSection(&csSh);

    //

    if (!StartLazyLoader()) {
        return FALSE;
    }

    return FInitLists();
}

#if defined(DEBUGVER)
DEBUG_VERSION('S','H',"Debug Symbolics handler")
#else
RELEASE_VERSION('S','H',"Debug Symbolics handler")
#endif

DBGVERSIONCHECK();
