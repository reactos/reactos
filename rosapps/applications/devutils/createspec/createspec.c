/*
- Info:
    - http://stackoverflow.com/questions/32251638/dbghelp-get-full-symbol-signature-function-name-parameters-types
    - http://www.debuginfo.com/articles/dbghelptypeinfo.html
- TODO:
    - Dump usage
    - Test for dbghelp + symsrv and warn if not working
    - Resolve forwarders

*/
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#ifdef _MSC_VER
#pragma warning(disable:4091)
#endif
#define _NO_CVCONST_H
#include <dbghelp.h>

// doesn't seem to be defined anywhere
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

#define MAX_SYMBOL_NAME		1024
typedef struct _SYMINFO_EX
{
    SYMBOL_INFO si;
    CHAR achName[MAX_SYMBOL_NAME];
} SYMINFO_EX;

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
CHAR gszModuleFileName[MAX_PATH+1];

HRESULT
OpenFileFromName(
    _In_ PCSTR pszDllName,
    _Out_ PHANDLE phFile)
{
	HANDLE hFile;

	/* Try current directory */
	GetCurrentDirectoryA(MAX_PATH, gszModuleFileName);
	strcat_s(gszModuleFileName, sizeof(gszModuleFileName), "\\");
	strcat_s(gszModuleFileName, sizeof(gszModuleFileName), pszDllName);
	hFile = CreateFileA(gszModuleFileName,
                        FILE_READ_DATA,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
	if (hFile != INVALID_HANDLE_VALUE)
    {
        *phFile = hFile;
        return S_OK;
    }

	/* Try system32 directory */
    strcat_s(gszModuleFileName, sizeof(gszModuleFileName), "%systemroot%\\system32\\");
    strcat_s(gszModuleFileName, sizeof(gszModuleFileName), pszDllName);
	hFile = CreateFileA(gszModuleFileName,
                        FILE_READ_DATA,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
	if (hFile != INVALID_HANDLE_VALUE)
    {
        *phFile = hFile;
        return S_OK;
    }

    return HRESULT_FROM_WIN32(GetLastError());
}

HRESULT
GetExportsFromFile(
    _In_ HANDLE hFile,
    _Out_ PEXPORT_DATA* ppExportData)
{
    HANDLE hMap;
    PBYTE pjImageBase;
    PIMAGE_EXPORT_DIRECTORY pExportDir;
    ULONG i, cjExportSize, cFunctions, cjTableSize;
    PEXPORT_DATA pExportData;
    PULONG pulAddressTable, pulNameTable;
    PUSHORT pusOrdinalTable;

    /* Create an image file mapping */
	hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL);
	if (!hMap)
	{
		fprintf(stderr, "CreateFileMapping() failed: %ld\n", GetLastError());
		return HRESULT_FROM_WIN32(GetLastError());
	}

    /* Map the file */
	pjImageBase = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if(pjImageBase == NULL)
	{
		fprintf(stderr, "MapViewOfFile() failed: %ld\n", GetLastError());
        CloseHandle(hMap);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	/* Get the export directory */
	pExportDir = ImageDirectoryEntryToData(pjImageBase,
                                           TRUE,
                                           IMAGE_DIRECTORY_ENTRY_EXPORT,
                                           &cjExportSize);

    cFunctions = pExportDir->NumberOfFunctions;
    cjTableSize = FIELD_OFFSET(EXPORT_DATA, aExports[cFunctions]);

    pExportData = malloc(cjTableSize);
    if (pExportData == NULL)
    {
        return E_OUTOFMEMORY;
    }

    RtlZeroMemory(pExportData, cjTableSize);

    pulAddressTable = (PULONG)(pjImageBase + pExportDir->AddressOfFunctions);

    pExportData->cNumberOfExports = cFunctions;

    /* Loop through the function table */
    for (i = 0; i < cFunctions; i++)
    {
        PVOID pvFunction = (pjImageBase + pulAddressTable[i]);
        
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
    UnmapViewOfFile(pjImageBase);
    CloseHandle(hMap);
    return S_OK;
}

BOOL
CALLBACK
EnumParametersCallback(
    _In_ PSYMBOL_INFO pSymInfo,
    _In_ ULONG SymbolSize,
    _In_opt_ PVOID UserContext)
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
            /* 'long' type */
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
                    if (eBaseType == btChar)
                    {
                        pExport->aeParameters[pExport->cParameters - 1] = TYPE_STR;
                    }
                    else if (eBaseType == btWChar)
                    {
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

HRESULT
ParseExportSymbols(
    _In_ HANDLE hFile,
    _Inout_ PEXPORT_DATA pExportData)
{
    DWORD Options;
    DWORD64 dwModuleBase;
    ULONG i;
    IMAGEHLP_STACK_FRAME StackFrame;
    SYMINFO_EX sym;


    /* Initialize dbghelp */
	if (!SymInitialize(ghProcess, 0, FALSE))
		return E_FAIL;

    Options = SymGetOptions();
    Options |= SYMOPT_ALLOW_ABSOLUTE_SYMBOLS | SYMOPT_DEBUG;// | SYMOPT_NO_PROMPTS;
    Options &= ~SYMOPT_DEFERRED_LOADS;
    SymSetOptions(Options);
    SymSetSearchPath(ghProcess, "srv**symbols*http://msdl.microsoft.com/download/symbols");

	printf("Loading symbols, please wait...\n");
	dwModuleBase = SymLoadModule64(ghProcess, 0, gszModuleFileName, 0, 0, 0);
	if (dwModuleBase == 0)
	{
		fprintf(stderr, "SymLoadModule64() failed: %ld\n", GetLastError());
		return E_FAIL;
	}


    for (i = 0; i < pExportData->cNumberOfExports; i++)
    {
        PEXPORT pExport = &pExportData->aExports[i];
        ULONG64 ullFunction = dwModuleBase + pExportData->aExports[i].ulRva;
        ULONG64 ullDisplacement;

        /* Skip forwarder */
        if (pExport->pszForwarder != NULL)
            continue;

        RtlZeroMemory(&sym, sizeof(sym));
        sym.si.SizeOfStruct = sizeof(SYMBOL_INFO);
        sym.si.MaxNameLen = MAX_SYMBOL_NAME;

        /* Try to find the symbol */
        if (!SymFromAddr(ghProcess, ullFunction, &ullDisplacement, &sym.si))
        {
            printf("Error: SymFromAddr() failed. Error code: %u \n", GetLastError());
            continue;
        }

        /* Symbol found. Check if it is a function */
        if (sym.si.Tag == SymTagFunction)
        {
            /* If we don't have a name yet, get one */
            pExport->pszSymbol = _strdup(sym.si.Name);

            /* Get the calling convention */
            if (!SymGetTypeInfo(ghProcess,
                                dwModuleBase,
                                sym.si.TypeIndex,
                                TI_GET_CALLING_CONVENTION,
                                &pExport->dwCallingConvention))
            {
                /* Fall back to __stdcall */
                pExport->dwCallingConvention = 0x07; // CV_CALL_NEAR_STD
            }

            /* Set the context to the function address */
            RtlZeroMemory(&StackFrame, sizeof(StackFrame));
            StackFrame.InstructionOffset = ullFunction;
            if (!SymSetContext(ghProcess, &StackFrame, NULL))
            {
                DWORD dwLastError = GetLastError();
                __debugbreak();
                continue;
            }

            /* Enumerate all symbols for this function */
            if (!SymEnumSymbols(ghProcess,
                                0, // use SymSetContext
                                NULL,
                                EnumParametersCallback,
                                pExport))
            {
                DWORD dwLastError = GetLastError();
                __debugbreak();
                continue;
            }
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
        case CV_CALL_CLRCALL:
            return "clrcall";
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
        fprintf(stderr, "Failed to open spec file: '%s'\n", pszSpecFile);
        return E_FAIL;
    }

    /* Loop all exports */
    for (i = 0; i < pExportData->cNumberOfExports; i++)
    {
        pExport = &pExportData->aExports[i];

        fprintf(file, "%u %s ", i + 1, GetCallingConvention(pExport));
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
            fprintf(file, "NamelessExport_%u", i);
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
    HANDLE hFile;
    PEXPORT_DATA pExportData;

    // check params
        // help

	ghProcess = GetCurrentProcess();

	/* Open the file */
	hr = OpenFileFromName(argv[1], &hFile);
    if (!SUCCEEDED(hr))
    {
        fprintf(stderr, "Failed to open file: %lx\n", hr);
        return hr;
    }

    /* Get the exports */
    hr = GetExportsFromFile(hFile, &pExportData);
    if (!SUCCEEDED(hr))
    {
        fprintf(stderr, "Failed to get exports: %lx\n", hr);
        return hr;
    }

    /* Get additional info from symbols */
    hr = ParseExportSymbols(hFile, pExportData);
    if (!SUCCEEDED(hr))
    {
        fprintf(stderr, "Failed to get symbol information: %lx\n", hr);
    }

    if (argc > 2)
    {
        pszSpecFile = argv[2];
    }
    else
    {
        PSTR pszStart = strrchr(argv[1], '\\');
        if (pszStart == 0)
            pszStart = argv[1];

        strcpy_s(szSpecFile, sizeof(szSpecFile), pszStart);
        strcat_s(szSpecFile, sizeof(szSpecFile), ".spec");
        pszSpecFile = szSpecFile;
    }

    hr = CreateSpecFile(pszSpecFile, pExportData);

    CloseHandle(hFile);

    printf("Spec file '%s' was successfully written.\n", szSpecFile);

    return hr;
}
