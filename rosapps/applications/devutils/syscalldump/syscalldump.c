#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#define _WINVER 0x501
#define SYMOPT_ALLOW_ABSOLUTE_SYMBOLS 0x00000800
#include <windows.h>
#include <shlwapi.h>
#include <dbghelp.h>

HANDLE hCurrentProcess;
BOOL bX64;

#define MAX_SYMBOL_NAME		1024

BOOL InitDbgHelp(HANDLE hProcess)
{
	if (!SymInitialize(hProcess, 0, FALSE))
		return FALSE;

    SymSetOptions(SymGetOptions() | SYMOPT_ALLOW_ABSOLUTE_SYMBOLS);
	SymSetOptions(SymGetOptions() & (~SYMOPT_DEFERRED_LOADS));
	SymSetSearchPath(hProcess, "srv**symbols*http://msdl.microsoft.com/download/symbols");
	return TRUE;
}

PVOID
ImageSymToVa(HANDLE hProcess, PSYMBOL_INFO pSym, PBYTE pModule, PCSTR Name)
{
	PIMAGE_NT_HEADERS NtHeaders;
	PVOID p;

	pSym->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSym->MaxNameLen = MAX_SYMBOL_NAME-1;

	if (!SymFromName(hProcess, Name, pSym))
	{
		printf("SymGetSymFromName64() failed: %ld\n", GetLastError());
		return 0;
	}
#if defined(__GNUC__) && \
	(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ < 40400)
	printf("looking up adress for %s: 0x%llx\n", Name, pSym->Address);
#else
	printf("looking up adress for %s: 0x%I64x\n", Name, pSym->Address);
#endif

	NtHeaders = ImageNtHeader(pModule);
	p = ImageRvaToVa(NtHeaders, pModule, pSym->Address - pSym->ModBase, NULL);

	return p;
}

BOOL CALLBACK EnumSymbolsProc(
	PSYMBOL_INFO pSymInfo,
	ULONG SymbolSize,
	PVOID UserContext)
{
	if ((UINT)UserContext == -1)
	{
		printf("%s ", pSymInfo->Name);
	}
	else
	{
		if (!bX64)
		{
			printf("%s@%d ", pSymInfo->Name, (UINT)UserContext);
		}
		else
		{
			printf("%s <+ %d> ", pSymInfo->Name, (UINT)UserContext);
		}
	}
	return TRUE;
}

int main(int argc, char* argv[])
{
	HANDLE hProcess;
	CHAR szModuleFileName[MAX_PATH+1];
    DWORD64 dwModuleBase;
    HANDLE hFile = 0, hMap = 0;
    PBYTE pModule = NULL;
    UINT i;
    PVOID pW32pServiceTable, pW32pServiceLimit;
    PBYTE pW32pArgumentTable;
    PVOID pfnSimpleCall;
    DWORD dwServiceLimit;

	struct
	{
		SYMBOL_INFO	Symbol;
		CHAR		Name[MAX_SYMBOL_NAME];
	} Sym;

	printf("Win32k Syscall dumper\n");
	printf("Copyright (c) Timo Kreuzer 2007-08\n");

	hProcess = GetCurrentProcess();

	// try current dir
	GetCurrentDirectory(MAX_PATH, szModuleFileName);
	strcat(szModuleFileName, "\\win32k.sys");
	hFile = CreateFile(szModuleFileName, FILE_READ_DATA, FILE_SHARE_READ, NULL,
	          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,  NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		goto cont;
	}

	// try system dir
	GetSystemDirectory(szModuleFileName, MAX_PATH);
	strcat(szModuleFileName, "\\win32k.sys");
	hFile = CreateFile(szModuleFileName, FILE_READ_DATA, FILE_SHARE_READ, NULL,
	          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,  NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("CreateFile() failed: %ld!\n", GetLastError());
		goto cleanup;
	}

cont:
	printf("Trying to get syscalls from: %s\n", szModuleFileName);

	if (!InitDbgHelp(hProcess))
	{
		printf("SymInitialize() failed\n");
		goto cleanup;
	}

	printf("Loading symbols for %s, please wait...\n", szModuleFileName);
	dwModuleBase = SymLoadModule64(hProcess, 0, szModuleFileName, 0, 0, 0);
	if (dwModuleBase == 0)
	{
		printf("SymLoadModule64() failed: %ld\n", GetLastError());
		goto cleanup;
	}

	hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!hMap)
	{
		printf("CreateFileMapping() failed: %ld\n", GetLastError());
		goto cleanup;
	}

	pModule = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if(!pModule)
	{
		printf("MapViewOfFile() failed: %ld\n", GetLastError());
		goto cleanup;
	}

	bX64 = (ImageNtHeader(pModule)->FileHeader.Machine != IMAGE_FILE_MACHINE_I386);

	pW32pServiceTable = ImageSymToVa(hProcess, &Sym.Symbol, pModule, "W32pServiceTable");
	pW32pServiceLimit = ImageSymToVa(hProcess, &Sym.Symbol, pModule, "W32pServiceLimit");
	pW32pArgumentTable = ImageSymToVa(hProcess, &Sym.Symbol, pModule, "W32pArgumentTable");
//	printf("pW32pServiceTable = %p\n", pW32pServiceTable);
//	printf("pW32pServiceLimit = %p\n", pW32pServiceLimit);
//	printf("pW32pArgumentTable = %p\n", pW32pArgumentTable);

	if (!pW32pServiceTable || !pW32pServiceLimit || !pW32pArgumentTable)
	{
		printf("Couldn't find adress!\n");
		goto cleanup;
	}

	dwServiceLimit = *((DWORD*)pW32pServiceLimit);

	if (!bX64)
	{
		DWORD *pdwEntries32 = (DWORD*)pW32pServiceTable;

		for (i = 0; i < dwServiceLimit; i++)
		{
			printf("0x%x:", i+0x1000);
			SymEnumSymbolsForAddr(hProcess, (DWORD64)pdwEntries32[i], EnumSymbolsProc, (PVOID)(DWORD)pW32pArgumentTable[i]);
			printf("\n");
		}
	}
	else
	{
		DWORD64 *pdwEntries64 = (DWORD64*)pW32pServiceTable;

		for (i = 0; i < dwServiceLimit; i++)
		{
			printf("0x%x:", i+0x1000);
			SymEnumSymbolsForAddr(hProcess, (DWORD64)pdwEntries64[i], EnumSymbolsProc, (PVOID)(DWORD)pW32pArgumentTable[i]);
			printf("\n");
		}
	}

	/* Dump apfnSimpleCall */
	printf("\nDumping apfnSimpleCall:\n");
	pfnSimpleCall = (PVOID*)ImageSymToVa(hProcess, &Sym.Symbol, pModule, "apfnSimpleCall");
	i = 0;

	if (bX64)
	{
		DWORD64 *pfnSC64 = (DWORD64*)pfnSimpleCall;
		while (pfnSC64[i] != 0)
		{
			printf("0x%x:", i);
			SymEnumSymbolsForAddr(hProcess, (DWORD64)pfnSC64[i], EnumSymbolsProc, (PVOID)-1);
			printf("\n");
			i++;
		}
	}
	else
	{
		DWORD *pfnSC32 = (DWORD*)pfnSimpleCall;
		while (pfnSC32[i] != 0)
		{
			printf("0x%x:", i);
			SymEnumSymbolsForAddr(hProcess, (DWORD64)pfnSC32[i], EnumSymbolsProc, (PVOID)-1);
			printf("\n");
			i++;
		}
	}

cleanup:
	if (pModule)
	{
		UnmapViewOfFile(pModule);
	}
	if (hMap)
	{
		CloseHandle(hMap);
	}
	if (hFile)
	{
		CloseHandle(hFile);
	}

	return 0;
}
