#include "priv.h"
#include "linkload.h"
#include "delayimp.h"
#pragma hdrstop

//
// linkload.c
//
// This code handles the failure case when using the linker DELAYLOAD.  When linking with the
// delay load switch the linker places stubs for the functions in that dll, when that function
// is first called the linker then attempts to fix up that export. If that fixup fails, then the
// linker by default will throw an exception.
//
// Having exceptions thrown is great for new code, however for existing code and to make our lives
// easier we need a simplier way to handle this, so here is one answer.
//
// When an API fails to load (normally because the required DLL is not present), override the linkers
// default handler for failure and attempt to provide a fall back.  Our fallback is to have a series
// of stub APIs that return failure's for the APIs being called, eg: calling CoCreateInstance and OLE32
// is not present, could result in E_FAIL, therefore the calling app can just take that error
// and process it as normal.
//
// The stubs are generated from the delayload directory, it contains a series of .txt files that
// are munged into .c files containing the failure cases, see the readme in that directory for
// more details of how to add functions.
//
// daviddv jul98.


//
// array containing the list of modules (from the delay load directory)
//

INT g_cLinkLoadModules;
MODULETABLE g_aLinkLoadModules[];


//
// Given a module name lets find it in the sorted table of modules.  We assume that
// all names are lower case and then we can just binary search through.
//

LPMODULETABLE _LookupModule(LPCSTR pName)
{
    LPMODULETABLE pResult = NULL;
    CHAR szNameLC[MAX_PATH+1];
    INT nResult;
    INT iLow;                                 // these must be signed for the search to work
    INT iMiddle;
    INT iHigh;

    StrCpy(szNameLC, pName);
    CharLowerBuff(szNameLC, lstrlenA(szNameLC));

    iLow = 0;
    iHigh = g_cLinkLoadModules-1;

    while (iHigh >= iLow)
    {
        iMiddle = (iLow + iHigh) / 2;
        nResult = StrCmp(szNameLC, g_aLinkLoadModules[iMiddle].pName);

        if (nResult < 0)
        {
            iHigh = iMiddle - 1;
        }
        else if (nResult > 0)
        {
            iLow = iMiddle + 1;
        }
        else
        {
            pResult = &g_aLinkLoadModules[iMiddle];
            break;
        }
    }

    return pResult;
}


//
// Given a module and function name/ordinal lets locate it in that export
// list, returning NULL if nothing is found.
//

FARPROC _LookupExportName(LPMODULETABLE pModule, LPCSTR pName)
{
    FARPROC pResult = NULL;
    CHAR szNameLC[MAX_PATH+1];
    INT nResult;
    INT iLow;                                 // these must be signed for the search to work
    INT iMiddle;
    INT iHigh;

    StrCpy(szNameLC, pName);
    CharLowerBuff(szNameLC, lstrlenA(szNameLC));

    iLow = 0;
    iHigh = (*pModule->pcExportTable)-1;

    while (iHigh >= iLow)
    {
        iMiddle = (iLow + iHigh) / 2;
        nResult = StrCmp(szNameLC, pModule->pExportTable[iMiddle].pName);

        if (nResult < 0)
        {
            iHigh = iMiddle - 1;
        }
        else if (nResult > 0)
        {
            iLow = iMiddle + 1;
        }
        else
        {
            pResult = pModule->pExportTable[iMiddle].pEntry;
            break;
        }
    }

    return pResult;
}

FARPROC _LookupOrdinal(LPMODULETABLE pModule, DWORD dwOrdinal)
{
    FARPROC pResult = NULL;
    INT nResult;
    INT iLow;                                 // these must be signed for the search to work
    INT iMiddle;
    INT iHigh;

    iLow = 0;
    iHigh = (*pModule->pcOrdinalTable)-1;

    while (iHigh >= iLow)
    {
        iMiddle = (iLow + iHigh) / 2;
        nResult = dwOrdinal - pModule->pOrdinalTable[iMiddle].dwOrdinal;

        if (nResult < 0)
        {
            iHigh = iMiddle - 1;
        }
        else if (nResult > 0)
        {
            iLow = iMiddle + 1;
        }
        else
        {
            pResult = pModule->pOrdinalTable[iMiddle].pEntry;
            break;
        }
    }

    return pResult;
}


//
// Delay load failure, this is called when the linker fails to load
// a DLL find an entry point.  We must attempt to fix up the function
// pointers accordingly.
//

FARPROC _HandleGetProcAddress(PDelayLoadInfo pInfo)
{
    FARPROC fpResult = NULL;
    LPMODULETABLE pModule;

    // simulate a GetProcAddress using the information in the PDelayLoadInfo 
    // structure.  

    pModule = _LookupModule(pInfo->szDll);                           
    if ( pModule )
    {
        if ( pInfo->dlp.fImportByName )
            fpResult = _LookupExportName(pModule, pInfo->dlp.szProcName);
        else
            fpResult = _LookupOrdinal(pModule, pInfo->dlp.dwOrdinal);
    }

    if ( !fpResult )
        SetLastError(ERROR_MOD_NOT_FOUND);

    return fpResult;
}

FARPROC WINAPI ShellDelayLoadHelper(UINT unReason, PDelayLoadInfo pInfo)
{
    FARPROC fpResult = NULL;
 
    switch ( unReason )
    {
        case dliFailLoadLib:
        {   
            // The linker failed to load the library, so return -1 as the HINSTNACE
            // to use when calling GetProcAddress.  This allows us to determine if
            // this is an export we should be fixing up.  

            if ( _LookupModule(pInfo->szDll) )
                fpResult = (FARPROC)-1;

            break;
        }

        case dliNotePreGetProcAddress:
        {
            // If the module handle is -1 then try and fix up the API now (rather than
            // the linker calling GetProcAddress). 

            if ( pInfo->hmodCur == (HINSTANCE)-1 ) 
                fpResult = _HandleGetProcAddress(pInfo);

            break;
        }

        case dliFailGetProc:
        {
            // if the module is not -1 then look up the export, it maybe that the DLL
            // exists, but that API doesn't (eg: an older DLL is present on the machine).

            if ( pInfo->hmodCur != (HINSTANCE)-1 )                 
                fpResult = _HandleGetProcAddress(pInfo);

            break;
        }

        default:
            break;
    }    

    return fpResult;
}
