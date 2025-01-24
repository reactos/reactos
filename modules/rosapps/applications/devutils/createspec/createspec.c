/*
- Info:
    - http://stackoverflow.com/questions/32251638/dbghelp-get-full-symbol-signature-function-name-parameters-types
    - https://www.debuginfo.com/articles/dbghelptypeinfo.html
- TODO:
    - Dump usage
    - Test for dbghelp + symsrv and warn if not working
    - Resolve forwarders

*/
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#ifdef __REACTOS__
#include <dbghelp.h>
#include <cvconst.h>

// dirty hacks!
#define sprintf_s(dst, size, format, ...) sprintf(dst, format, __VA_ARGS__)
#define vsprintf_s(dst, size, format, ap) vsprintf(dst, format, ap)
#define fopen_s(pfile, name, mode) ((*pfile = fopen(name, mode)), (*pfile != 0) ? 0 : -1)
#define strcpy_s(dst, size, src) strncpy(dst, src, size)
#define strcat_s(dst, size, src) strncat(dst, src, size)

#else
#ifdef _MSC_VER
#pragma warning(disable:4091)
#endif
#define _NO_CVCONST_H
#include <dbghelp.h>

// This is from cvconst.h, but win sdk lacks this file
enum BasicType {
   btNoType   = 0,
   btVoid     = 1,
   btChar     = 2,
   btWChar    = 3,
   btInt      = 6,
   btUInt     = 7,
   btFloat    = 8,
   btBCD      = 9,
   btBool     = 10,
   btLong     = 13,
   btULong    = 14,
   btCurrency = 25,
   btDate     = 26,
   btVariant  = 27,
   btComplex  = 28,
   btBit      = 29,
   btBSTR     = 30,
   btHresult  = 31
};

typedef enum CV_call_e {
   CV_CALL_NEAR_C    = 0x00,
   CV_CALL_NEAR_FAST = 0x04,
   CV_CALL_NEAR_STD  = 0x07,
   CV_CALL_NEAR_SYS  = 0x09,
   CV_CALL_THISCALL  = 0x0b,
   CV_CALL_CLRCALL   = 0x16
} CV_call_e;

#endif // __REACTOS__


typedef enum _PARAM_TYPES
{
    TYPE_NONE,
    TYPE_LONG,
    TYPE_DOUBLE,
    TYPE_PTR,
    TYPE_STR,
    TYPE_WSTR
} PARAM_TYPES, *PPARAM_TYPES;

const char*
gapszTypeStrings[] =
{
    "???",
    "long",
    "double",
    "ptr",
    "str",
    "wstr"
};

#define MAX_PARAMETERS 64
typedef struct _EXPORT
{
    PSTR pszName;
    PSTR pszSymbol;
    PSTR pszForwarder;
    ULONG ulRva;
    DWORD dwCallingConvention;
    ULONG fForwarder : 1;
    ULONG fNoName : 1;
    ULONG fData : 1;
    ULONG cParameters;
    PARAM_TYPES aeParameters[MAX_PARAMETERS];
} EXPORT, *PEXPORT;

typedef struct _EXPORT_DATA
{
    ULONG cNumberOfExports;
    EXPORT aExports[1];
} EXPORT_DATA, *PEXPORT_DATA;

HANDLE ghProcess;

void
error(
    _In_ const char* pszFormat,
    ...)
{
    CHAR szBuffer[512];
    SIZE_T cchBuffer;
    DWORD dwLastError;
    va_list argptr;

    /* Get last error */
    dwLastError = GetLastError();

    va_start(argptr, pszFormat);
    cchBuffer = vsprintf_s(szBuffer, sizeof(szBuffer), pszFormat, argptr);
    va_end(argptr);

    /* Strip trailing newlines */
    _Analysis_assume_(cchBuffer < sizeof(szBuffer));
    while ((cchBuffer >= 1) &&
           ((szBuffer[cchBuffer - 1] == '\r') ||
            (szBuffer[cchBuffer - 1] == '\n')))
    {
        szBuffer[cchBuffer - 1] = '\0';
        cchBuffer--;
    }

    /* Check if we have an error */
    if (dwLastError != ERROR_SUCCESS)
    {
        /* Append error code */
        cchBuffer += sprintf_s(szBuffer + cchBuffer,
                               sizeof(szBuffer) - cchBuffer,
                               " [error %lu: ", dwLastError);

        /* Format the last error code */
        cchBuffer += FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                                    NULL,
                                    dwLastError,
                                    0,
                                    szBuffer + cchBuffer,
                                    (DWORD)(sizeof(szBuffer) - cchBuffer),
                                    NULL);

        /* Strip trailing newlines */
        _Analysis_assume_(cchBuffer < sizeof(szBuffer));
        while ((cchBuffer >= 1) &&
               ((szBuffer[cchBuffer - 1] == '\r') ||
                (szBuffer[cchBuffer - 1] == '\n')))
        {
            szBuffer[cchBuffer - 1] = '\0';
            cchBuffer--;
        }

        fprintf(stderr, "%s]\n", szBuffer);
    }
    else
    {
        fprintf(stderr, "%s\n", szBuffer);
    }
}

BOOL
InitDbgHelp(
    VOID)
{
    static const char *pszMsSymbolServer = "srv**symbols*http://msdl.microsoft.com/download/symbols";
    DWORD Options;

    /* Save current process ;-) */
    ghProcess = GetCurrentProcess();

    /* Initialize dbghelp */
    if (!SymInitialize(ghProcess, 0, FALSE))
    {
        error("SymInitialize() failed.");
        return FALSE;
    }

    /* Set options */
    Options = SymGetOptions();
    Options |= SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_DEBUG;// | SYMOPT_NO_PROMPTS;
    Options &= ~SYMOPT_DEFERRED_LOADS;
    SymSetOptions(Options);

    /* Test if we can reach the MS symbol server */
    if (!SymSrvIsStore(ghProcess, pszMsSymbolServer))
    {
        error("Failed to connect to symbol server.");
        return FALSE;
    }

    /* Set MS symbol server as symbol search path */
    SymSetSearchPath(ghProcess, pszMsSymbolServer);

    return TRUE;
}

HMODULE
LoadModuleWithSymbolsFullPath(
    _In_ PSTR pszFullModuleFileName)
{
    HMODULE hmod;
    DWORD64 dwModuleBase;

    /* Load the DLL */
    hmod = LoadLibraryExA(pszFullModuleFileName,
                          NULL,
                          LOAD_IGNORE_CODE_AUTHZ_LEVEL |
                          DONT_RESOLVE_DLL_REFERENCES |
                          LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hmod == NULL)
    {
        return NULL;
    }

    /* Load symbols for this module */
    dwModuleBase = SymLoadModule64(ghProcess,
                                   NULL,
                                   pszFullModuleFileName,
                                   NULL,
                                   (DWORD_PTR)hmod,
                                   0);
    if (dwModuleBase == 0)
    {
        /* ERROR_SUCCESS means, we have symbols already */
        if (GetLastError() != ERROR_SUCCESS)
        {
            return NULL;
        }
    }
    else
    {
        printf("Successfully loaded symbols for '%s'\n",
               pszFullModuleFileName);
    }

    return hmod;
}

HMODULE
LoadModuleWithSymbols(
    _In_ PSTR pszModuleName)
{
    CHAR szFullFileName[MAX_PATH];
    HMODULE hmod;

    /* Check if the file name has a path */
    if (strchr(pszModuleName, '\\') != NULL)
    {
        /* Try as it is */
        hmod = LoadModuleWithSymbolsFullPath(pszModuleName);
        if (hmod != NULL)
        {
            return hmod;
        }
    }

    /* Try current directory */
    GetCurrentDirectoryA(MAX_PATH, szFullFileName);
    strcat_s(szFullFileName, sizeof(szFullFileName), "\\");
    strcat_s(szFullFileName, sizeof(szFullFileName), pszModuleName);
    hmod = LoadModuleWithSymbolsFullPath(szFullFileName);
    if (hmod != NULL)
    {
        return hmod;
    }

    /* Try system32 */
    strcpy_s(szFullFileName, sizeof(szFullFileName), "%systemroot%\\system32\\");
    strcat_s(szFullFileName, sizeof(szFullFileName), pszModuleName);
    hmod = LoadModuleWithSymbolsFullPath(szFullFileName);
    if (hmod != NULL)
    {
        return hmod;
    }

#ifdef _WIN64
    /* Try SysWOW64 */
    strcpy_s(szFullFileName, sizeof(szFullFileName), "%systemroot%\\SysWOW64\\");
    strcat_s(szFullFileName, sizeof(szFullFileName), pszModuleName);
    hmod = LoadModuleWithSymbolsFullPath(szFullFileName);
    if (hmod != NULL)
    {
        return hmod;
    }
#endif // _WIN64

    return NULL;
}

HRESULT
GetExportsFromFile(
    _In_ HMODULE hmod,
    _Out_ PEXPORT_DATA* ppExportData)
{
    PBYTE pjImageBase;
    PIMAGE_EXPORT_DIRECTORY pExportDir;
    ULONG i, cjExportSize, cFunctions, cjTableSize;
    PEXPORT_DATA pExportData;
    PULONG pulAddressTable, pulNameTable;
    PUSHORT pusOrdinalTable;

    pjImageBase = (PBYTE)hmod;

    /* Get the export directory */
    pExportDir = ImageDirectoryEntryToData(pjImageBase,
                                           TRUE,
                                           IMAGE_DIRECTORY_ENTRY_EXPORT,
                                           &cjExportSize);
    if (pExportDir == NULL)
    {
        fprintf(stderr, "Failed to get export directory\n");
        return E_FAIL;
    }

    cFunctions = pExportDir->NumberOfFunctions;
    cjTableSize = FIELD_OFFSET(EXPORT_DATA, aExports[cFunctions]);

    pExportData = malloc(cjTableSize);
    if (pExportData == NULL)
    {
        error("Failed to allocate %u bytes of memory for export table\n", cjTableSize);
        return E_OUTOFMEMORY;
    }

    RtlZeroMemory(pExportData, cjTableSize);

    pulAddressTable = (PULONG)(pjImageBase + pExportDir->AddressOfFunctions);

    pExportData->cNumberOfExports = cFunctions;

    /* Loop through the function table */
    for (i = 0; i < cFunctions; i++)
    {
        PVOID pvFunction = (pjImageBase + pulAddressTable[i]);

        /* Check if this is a forwarder */
        if ((ULONG_PTR)((PUCHAR)pvFunction - (PUCHAR)pExportDir) < cjExportSize)
        {
            pExportData->aExports[i].pszForwarder = _strdup(pvFunction);
        }
        else
        {
            pExportData->aExports[i].ulRva = pulAddressTable[i];
        }
    }

    pulNameTable = (PULONG)(pjImageBase + pExportDir->AddressOfNames);
    pusOrdinalTable = (PUSHORT)(pjImageBase + pExportDir->AddressOfNameOrdinals);

    /* Loop through the name table */
    for (i = 0; i < pExportDir->NumberOfNames; i++)
    {
        ULONG iIndex = pusOrdinalTable[i];
        PSTR pszName = (PSTR)(pjImageBase + pulNameTable[i]);

        pExportData->aExports[iIndex].pszName = _strdup(pszName);
    }

    *ppExportData = pExportData;
    return S_OK;
}

BOOL
CALLBACK
EnumParametersCallback(
    _In_ PSYMBOL_INFO pSymInfo,
    _In_ ULONG SymbolSize,
    _In_ PVOID UserContext)
{
    PEXPORT pExport = (PEXPORT)UserContext;
    enum SymTagEnum eSymTag;
    enum BasicType eBaseType;
    DWORD dwTypeIndex;
    ULONG64 ullLength;

    /* If it's not a parameter, skip it */
    if (!(pSymInfo->Flags & SYMFLAG_PARAMETER))
    {
        return TRUE;
    }

    /* Count this parameter */
    pExport->cParameters++;

    /* Get the type for the parameter */
    if (SymGetTypeInfo(ghProcess,
                       pSymInfo->ModBase,
                       pSymInfo->TypeIndex,
                       TI_GET_SYMTAG,
                       &eSymTag))
    {
        switch (eSymTag)
        {
        case SymTagUDT:
        case SymTagBaseType:

            /* Try to get the size */
            if (SymGetTypeInfo(ghProcess,
                                pSymInfo->ModBase,
                                pSymInfo->TypeIndex,
                                TI_GET_LENGTH,
                                &ullLength))
            {
                if (ullLength > 8)
                {
                    /* That is probably not possible */
                    __debugbreak();
                }

                if (ullLength > 4)
                {
                    /* 'double' type */
                    pExport->aeParameters[pExport->cParameters - 1] = TYPE_DOUBLE;
                    break;
                }
            }

            /* Default to 'long' type */
            pExport->aeParameters[pExport->cParameters - 1] = TYPE_LONG;
            break;

        case SymTagEnum:
            /* 'long' type */
            pExport->aeParameters[pExport->cParameters - 1] = TYPE_LONG;
            break;

        case SymTagPointerType:
            /* 'ptr' type */
            pExport->aeParameters[pExport->cParameters - 1] = TYPE_PTR;

            /* Try to get the underlying type */
            if (SymGetTypeInfo(ghProcess,
                                pSymInfo->ModBase,
                                pSymInfo->TypeIndex,
                                TI_GET_TYPEID,
                                &dwTypeIndex))
            {
                /* Try to get the base type */
                if (SymGetTypeInfo(ghProcess,
                                    pSymInfo->ModBase,
                                    dwTypeIndex,
                                    TI_GET_BASETYPE,
                                    &eBaseType))
                {
                    /* Check for string types */
                    if (eBaseType == btChar)
                    {
                        /* 'str' type */
                        pExport->aeParameters[pExport->cParameters - 1] = TYPE_STR;
                    }
                    else if (eBaseType == btWChar)
                    {
                        /* 'wstr' type */
                        pExport->aeParameters[pExport->cParameters - 1] = TYPE_WSTR;
                    }
                }
            }
            break;

        default:
            printf("Unhandled eSymTag: %u\n", eSymTag);
            return FALSE;
        }
    }
    else
    {
        printf("Could not get type info. Fallig back to ptr\n");
        pExport->aeParameters[pExport->cParameters - 1] = TYPE_PTR;
    }

    return TRUE;
}

ULONG64
GetFunctionFromForwarder(
    _In_ PCSTR pszForwarder)
{
    CHAR szDllName[MAX_SYM_NAME];
    PCH pchDot, pszName;
    ULONG64 ullFunction;
    HMODULE hmod;

    /* Copy the forwarder name */
    strcpy_s(szDllName, sizeof(szDllName), pszForwarder);

    /* Find the '.' */
    pchDot = strchr(szDllName, '.');
    if (pchDot == NULL)
    {
        error("Invalid name for forwarder '%s'!", pszForwarder);
        return 0;
    }

    /* Terminate DLL name */
    *pchDot = '\0';

    /* Load the DLL */
    hmod = LoadModuleWithSymbols(szDllName);
    if (hmod == NULL)
    {
        error("Failed to load module for forwarder '%s'!", pszForwarder);
        return 0;
    }

    /* Get the function name and check for ordinal */
    pszName = pchDot + 1;
    if (pszName[0] == '#')
    {
        ULONG iOrdinal = strtoul(pszName + 1, NULL, 10);
        if ((iOrdinal == 0) || (iOrdinal > 0xFFFF))
        {
            error("Got invalid ordinal %u for ''", iOrdinal, pszForwarder);
            return 0;
        }

        pszName = (PSTR)(ULONG_PTR)iOrdinal;
    }

    /* Get the function address */
    ullFunction = (ULONG_PTR)GetProcAddress(hmod, pszName);
    if (ullFunction == 0)
    {
        error("Failed to resolve '%s' in '%s'.", pchDot + 1, szDllName);
        return 0;
    }

    return ullFunction;
}

HRESULT
ParseImageSymbols(
    _In_ HMODULE hmod,
    _Inout_ PEXPORT_DATA pExportData)
{
    DWORD64 dwModuleBase;
    ULONG i;
    IMAGEHLP_STACK_FRAME StackFrame;
    SYMBOL_INFO_PACKAGE sym;

    dwModuleBase = (DWORD_PTR)hmod;

    /* Loop through all exports */
    for (i = 0; i < pExportData->cNumberOfExports; i++)
    {
        PEXPORT pExport = &pExportData->aExports[i];
        ULONG64 ullFunction = dwModuleBase + pExportData->aExports[i].ulRva;
        ULONG64 ullDisplacement;

        /* Check if this is a forwarder */
        if (pExport->pszForwarder != NULL)
        {
            /* Load the module and get the function address */
            ullFunction = GetFunctionFromForwarder(pExport->pszForwarder);
            if (ullFunction == 0)
            {
                printf("Failed to get function for forwarder '%s'. Skipping.\n", pExport->pszForwarder);
                continue;
            }
        }

        RtlZeroMemory(&sym, sizeof(sym));
        sym.si.SizeOfStruct = sizeof(SYMBOL_INFO);
        sym.si.MaxNameLen = MAX_SYM_NAME;

        /* Try to find the symbol */
        if (!SymFromAddr(ghProcess, ullFunction, &ullDisplacement, &sym.si))
        {
            error("Error: SymFromAddr() failed.");
            continue;
        }

        /* Get the symbol name */
        pExport->pszSymbol = _strdup(sym.si.Name);

        /* Check if it is a function */
        if (sym.si.Tag == SymTagFunction)
        {
            /* Get the calling convention */
            if (!SymGetTypeInfo(ghProcess,
                                dwModuleBase,
                                sym.si.TypeIndex,
                                TI_GET_CALLING_CONVENTION,
                                &pExport->dwCallingConvention))
            {
                /* Fall back to __stdcall */
                pExport->dwCallingConvention = CV_CALL_NEAR_STD;
            }

            /* Set the context to the function address */
            RtlZeroMemory(&StackFrame, sizeof(StackFrame));
            StackFrame.InstructionOffset = ullFunction;
            if (!SymSetContext(ghProcess, &StackFrame, NULL))
            {
                error("SymSetContext failed for i = %u.", i);
                continue;
            }

            /* Enumerate all symbols for this function */
            if (!SymEnumSymbols(ghProcess,
                                0, // use SymSetContext
                                NULL,
                                EnumParametersCallback,
                                pExport))
            {
                error("SymEnumSymbols failed for i = %u.", i);
                continue;
            }
        }
        else if (sym.si.Tag == SymTagPublicSymbol)
        {
            pExport->dwCallingConvention = CV_CALL_NEAR_STD;
        }
        else if (sym.si.Tag == SymTagData)
        {
            pExport->fData = TRUE;
        }
    }

    return S_OK;
}

const CHAR*
GetCallingConvention(
    _In_ PEXPORT pExport)
{
    if (pExport->fData)
    {
        return "extern";
    }

#ifndef _M_AMD64
    switch (pExport->dwCallingConvention)
    {
        case CV_CALL_NEAR_C:
            return "cdecl";
        case CV_CALL_NEAR_FAST:
            return "fastcall";
        case CV_CALL_NEAR_STD:
            return "stdcall";
        case CV_CALL_NEAR_SYS:
            return "syscall";
        case CV_CALL_THISCALL:
            return "thiscall";
        default:
            __debugbreak();
    }
#endif
    return "stdcall";
}

HRESULT
CreateSpecFile(
    _In_ PCSTR pszSpecFile,
    _In_ PEXPORT_DATA pExportData)
{
    FILE *file;
    ULONG i, p;
    PEXPORT pExport;

    /* Create the spec file */
    if (fopen_s(&file, pszSpecFile, "w") != 0)
    {
        error("Failed to open spec file: '%s'\n", pszSpecFile);
        return E_FAIL;
    }

    /* Loop all exports */
    for (i = 0; i < pExportData->cNumberOfExports; i++)
    {
        pExport = &pExportData->aExports[i];

        fprintf(file, "%lu %s ", i + 1, GetCallingConvention(pExport));
        //if (pExport->fNoName)
        if (pExport->pszName == NULL)
        {
            fprintf(file, "-noname ");
        }

        if (pExport->pszName != NULL)
        {
            fprintf(file, "%s", pExport->pszName);
        }
        else if (pExport->pszSymbol != NULL)
        {
            fprintf(file, "%s", pExport->pszSymbol);
        }
        else
        {
            fprintf(file, "NamelessExport_%lu", i);
        }

        if (!pExport->fData)
        {
            fprintf(file, "(");
            for (p = 0; p < pExport->cParameters; p++)
            {
                fprintf(file, "%s", gapszTypeStrings[pExport->aeParameters[p]]);
                if ((p + 1) < pExport->cParameters)
                {
                    fprintf(file, " ");
                }
            }
            fprintf(file, ")");
        }

        if (pExport->pszForwarder != NULL)
        {
            fprintf(file, " %s", pExport->pszForwarder);
        }

        fprintf(file, "\n");
    }

    fclose(file);

    return S_OK;
}

int main(int argc, char* argv[])
{
    HRESULT hr;
    CHAR szSpecFile[MAX_PATH];
    PSTR pszSpecFile;
    PEXPORT_DATA pExportData;
    HMODULE hmod;

    /* Check parameters */
    if ((argc < 2) || !strcmp(argv[1], "/?"))
    {
        printf("syntax: createspec <image file> [<spec file>]\n");
        return 0;
    }

    /* Check if we have a spec file name */
    if (argc > 2)
    {
        pszSpecFile = argv[2];
    }
    else
    {
        /* Create spec file name from image file name */
        PSTR pszStart = strrchr(argv[1], '\\');
        if (pszStart == 0)
            pszStart = argv[1];

        strcpy_s(szSpecFile, sizeof(szSpecFile), pszStart);
        strcat_s(szSpecFile, sizeof(szSpecFile), ".spec");
        pszSpecFile = szSpecFile;
    }

    /* Initialize dbghelp.dll */
    if (!InitDbgHelp())
    {
        error("Failed to init dbghelp!\n"
              "Make sure you have dbghelp.dll and symsrv.dll in the same folder.\n");
        return E_FAIL;
    }

    /* Load the file including symbols */
    printf("Loading symbols for '%s', please wait...\n", argv[1]);
    hmod = LoadModuleWithSymbols(argv[1]);
    if (hmod == NULL)
    {
        error("Failed to load module '%s'!", argv[1]);
        return E_FAIL;
    }

    /* Get the exports */
    hr = GetExportsFromFile(hmod, &pExportData);
    if (!SUCCEEDED(hr))
    {
        error("Failed to get exports: %lx\n", hr);
        return hr;
    }

    /* Get additional info from symbols */
    hr = ParseImageSymbols(hmod, pExportData);
    if (!SUCCEEDED(hr))
    {
        error("Failed to get symbol information: hr=%lx\n", hr);
    }

    /* Write the spec file */
    hr = CreateSpecFile(pszSpecFile, pExportData);

    printf("Spec file '%s' was successfully written.\n", szSpecFile);

    return hr;
}
