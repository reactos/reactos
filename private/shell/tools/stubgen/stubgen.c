#include "windows.h"
#include "windowsx.h"
#include "shlwapi.h"
#include "commctrl.h"
#include "comctrlp.h"
#include <stdlib.h>
#include <stdio.h>

#define VERSION TEXT("0.00")
#define SIZEOF(x) sizeof(x)
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

typedef struct
{
    WORD wOrdinal;
    LPTSTR pFunction;
} EXPORTENTRY, * LPEXPORTENTRY;


//
// read a line, skipping leading and trailing white space and placing the output
// into the specified buffer.
//

LPTSTR _ReadLine(LPTSTR pSource, LPTSTR pBuffer, INT cchBuffer)
{
    //
    // skip leading white space
    //
    
    *pBuffer = TEXT('\0');

    while ( (*pSource == TEXT(' ')) ||
               (*pSource == TEXT('\t')) )
    {
        pSource++;
    }

    if ( !*pSource )
        return NULL;

    while ( (*pSource != TEXT('\r')) && 
              (*pSource != TEXT('\n')) && 
                (*pSource != TEXT('\0')) && 
                  (cchBuffer >= 1) )
    {   
        *pBuffer++ = *pSource++;
        cchBuffer--;
    }

    *pBuffer++ = TEXT('\0');

    while ( (*pSource == TEXT('\r')) ||
              (*pSource == TEXT('\n')) )
    {
        pSource++;
    }

    return pSource;
}


//
// Get string element, given an index into the buffer copy out the element
// that we want.
//

BOOL _GetStringElement(LPTSTR pString, INT index, BOOL fEntireLine, LPTSTR pBuffer, INT cchBuffer)
{
    for ( ; *pString && (index > 0) ; index-- )
    {
        while ( *pString != TEXT(',') && *pString != TEXT('\0') )
            pString++;

        if ( *pString == TEXT(',') )
            pString++;
    }

    if ( index )
        return FALSE;        

    while ( *pString == TEXT(' ') )
        pString++;

    while ( *pString && (cchBuffer > 1) )
    {
        if ( !fEntireLine && (*pString == TEXT(',')) )
            break;
            
        *pBuffer++ = *pString++;
        cchBuffer--;
    }

    *pBuffer = TEXT('\0');

    return TRUE;
}


//
// Get a stub function name given its module and function name.
//

static TCHAR szStubFunction[MAX_PATH];

LPTSTR _GetStubFunction(LPTSTR pModule, LPTSTR pFunction)
{
    wnsprintf(szStubFunction, ARRAYSIZE(szStubFunction), TEXT("_%s_%s"), pModule, pFunction);
    return szStubFunction;
}


//
// Generate stub
//
// This takes a line from the file and get the information we need from it.
//

BOOL _GenerateStub(LPTSTR pModule, LPTSTR pBuffer, HDPA hdpaFunctions, HDPA hdpaOrdinals)
{
    TCHAR szResultType[MAX_PATH];
    TCHAR szResult[MAX_PATH];
    TCHAR szFunction[MAX_PATH];
    TCHAR szArguments[MAX_PATH*2];
    LPTSTR pFunction;
    LPTSTR pOrdinal;
    INT iByName, iByOrdinal;
    LPEXPORTENTRY pExport;

    // get the fields, all are required

    if ( !_GetStringElement(pBuffer, 0, FALSE, szResultType, ARRAYSIZE(szResultType)) )
        return FALSE;

    if ( !_GetStringElement(pBuffer, 1, FALSE, szResult, ARRAYSIZE(szResult)) )
        return FALSE;

    if ( !_GetStringElement(pBuffer, 2, FALSE, szFunction, ARRAYSIZE(szFunction)) )
        return FALSE;

    if ( !_GetStringElement(pBuffer, 3, TRUE, szArguments, ARRAYSIZE(szArguments)) )
        return FALSE;

    // if the function name is bla@4 then it has an ordinal therefore we must attempt
    // to get the ordinal number.

    pOrdinal = StrChr(szFunction, TEXT('@'));
    if ( pOrdinal )
        *pOrdinal++ = TEXT('\0');

    // allocate an export, adding both the ordinals and the functions as required.
    // if pOrdinal != NULL then we assume that we should parse the int.

    pExport = LocalAlloc(LPTR, SIZEOF(EXPORTENTRY));
    if ( !pExport )
        return FALSE;    

    Str_SetPtr(&pFunction, szFunction);
    if ( !pFunction )
    {
        LocalFree(pExport);
        return FALSE;
    }

    pExport->wOrdinal = StrToInt(pOrdinal ? pOrdinal:TEXT(""));
    pExport->pFunction = pFunction;

    iByOrdinal = iByName = DPA_AppendPtr(hdpaFunctions, pExport);
    
    if ( pOrdinal )
        iByOrdinal = DPA_AppendPtr(hdpaOrdinals, pExport);

    if ( (iByName == -1) || (iByOrdinal == -1) )
    {
        if ( iByName != -1 )
            DPA_DeletePtr(hdpaFunctions, iByName);

        LocalFree(pExport);
        Str_SetPtr(&pFunction, NULL);
        return FALSE;
    }

    // spew out the function name

    printf(TEXT("\n"));
    printf(TEXT("%s %s%s\n"), szResultType, _GetStubFunction(pModule, pFunction), szArguments);
    printf(TEXT("{\n"));

    if ( szResult[0] )
        printf(TEXT("   return %s;\n"), szResult);

    printf(TEXT("}\n"));
    
    return TRUE;
}


//
// "stubgen <stub list> <module>"
//
// The stub list is a text file that lists all the exports you want to generate
// stubs for, each stub is a simple function which returns a specified result.
//
// The format of the file is:
//
//    <result type>,<result>,<function>,<arguments>
//
// eg:
//
//    BOOL, FALSE, SHBrowseForContainer, (bla, bla, bla)
//
// Which generates a stub:
//
//    BOOL SHBrowseForContainer(bla, bla, bla)
//    {
//      return FALSE;
//    }
//

INT _SortNameCB(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    LPEXPORTENTRY pExport1 = (LPEXPORTENTRY)p1;
    LPEXPORTENTRY pExport2 = (LPEXPORTENTRY)p2;
    return StrCmpI(pExport1->pFunction, pExport2->pFunction);
}

INT _SortOrdinalCB(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    LPEXPORTENTRY pExport1 = (LPEXPORTENTRY)p1;
    LPEXPORTENTRY pExport2 = (LPEXPORTENTRY)p2;
    return pExport1->wOrdinal - pExport2->wOrdinal;
}

INT __cdecl main(INT cArgs, LPTSTR pArgs[])
{
    TCHAR szSource[MAX_PATH];
    TCHAR szModule[MAX_PATH];
    HANDLE hFile;
    LPTSTR pStubFile;
    DWORD dwSize, dwRead;
    HDPA hdpaFunctions;
    HDPA hdpaOrdinals;
    INT i;

    if ( cArgs < 2 )
    {
        printf(TEXT("stubgen: <src> <module>\n"));
        return -1;
    }

    StrCpy(szSource, pArgs[1]);
    StrCpy(szModule, pArgs[2]);


    //
    // load the source file into memory and then lets generate the stub table,
    // add a TCHAR to the file size to get it null terminated
    //

    hFile = CreateFile(szSource,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL, 
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if ( hFile == INVALID_HANDLE_VALUE )
        return -1;

    dwSize = GetFileSize(hFile, NULL);
    pStubFile = LocalAlloc(LPTR, dwSize+SIZEOF(TCHAR));

    if ( !pStubFile ||
           !ReadFile(hFile, pStubFile, dwSize, &dwRead, NULL) ||
             dwRead != dwSize )
    {
        CloseHandle(hFile);
        return -1;
    }

    CloseHandle(hFile);    


    //
    // Create the DPA we will use for storing the function names
    //

    hdpaFunctions = DPA_Create(16);
    hdpaOrdinals = DPA_Create(16);

    if ( !hdpaFunctions || ! hdpaOrdinals )
        return -1;


    //
    // output header information
    // 

    for ( i = 3 ; i < cArgs ; i++ )
        printf(TEXT("#include \"%s\"\n"), pArgs[i]);

    printf(TEXT("#pragma hdrstop\n"));

    printf(TEXT("\n"));
    printf(TEXT("// Generate from %s by stubgen.exe\n"), szSource);
    printf(TEXT("// *** DO NOT EDIT THIS FILE ***\n\n"));


    //
    // now lets parse the file, trying to the function prototypes from it,
    // we skip all lines that start with a ';', '#' or '/' (as in //)
    //      

    while ( pStubFile )
    {
        TCHAR szBuffer[1024];

        pStubFile = _ReadLine(pStubFile, szBuffer, ARRAYSIZE(szBuffer));
        
        if ( pStubFile )
        {
            switch ( szBuffer[0] )
            {
                case TEXT('#'):
                case TEXT(';'):
                case TEXT('/'):
                    // comments are stripped
                    break;

                default:
                    _GenerateStub(szModule, szBuffer, hdpaFunctions, hdpaOrdinals);
                    break;
            }
        }
    }   

    
    //
    // if hdpaFunctions contains anything then we have generated a set of
    // stubs, so lets sort it and output that.
    //

    if ( DPA_GetPtrCount(hdpaFunctions) )
    {
        DPA_Sort(hdpaFunctions, _SortNameCB, 0);

        printf(TEXT("\n"));
        printf(TEXT("const INT g_c%sExportTable = %d;\n"), szModule, DPA_GetPtrCount(hdpaFunctions));
        printf(TEXT("const EXPORTTABLE g_%sExportTable[] =\n"), szModule);
        printf(TEXT("{\n"));

        for ( i = 0 ; i < DPA_GetPtrCount(hdpaFunctions); i++ )
        {
            LPEXPORTENTRY pExport = (LPEXPORTENTRY)DPA_GetPtr(hdpaFunctions, i);
            TCHAR szBuffer[MAX_PATH];

            StrCpy(szBuffer, pExport->pFunction);
#if UNICODE
            _wcslwr(szBuffer);
#else
            _strlwr(szBuffer);
#endif

            printf(TEXT("    \"%s\", (FARPROC)%s,\n"), szBuffer, _GetStubFunction(szModule, pExport->pFunction));
        }

        printf(TEXT("};\n"));
    }

    if ( DPA_GetPtrCount(hdpaOrdinals) )
    {
        DPA_Sort(hdpaFunctions, _SortOrdinalCB, 0);

        printf(TEXT("\n"));
        printf(TEXT("const INT g_c%sOrdinalTable = %d;\n"), szModule, DPA_GetPtrCount(hdpaOrdinals));
        printf(TEXT("const ORDINALTABLE g_%sOrdinalTable[] =\n"), szModule);
        printf(TEXT("{\n"));

        for ( i = 0 ; i < DPA_GetPtrCount(hdpaOrdinals); i++ )
        {
            LPEXPORTENTRY pExport = (LPEXPORTENTRY)DPA_GetPtr(hdpaOrdinals, i);
            printf(TEXT("    %d, (FARPROC)%s,\n"), pExport->wOrdinal, _GetStubFunction(szModule, pExport->pFunction));
        }

        printf(TEXT("};\n"));

    }

    return 0;
}
