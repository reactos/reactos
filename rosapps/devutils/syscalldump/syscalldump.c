#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#define _WINVER 0x501
#include <windows.h>
#include <shlwapi.h>
#include <dbghelp.h>

HANDLE hCurrentProcess;

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

DWORD64
GetOffsetFromAdress64(PBYTE pModule, DWORD64 dwAdress, PBOOL pbX64)
{
	PIMAGE_DOS_HEADER pDosHdr;
	PIMAGE_NT_HEADERS32 pNtHdr32;
	WORD NumberOfSections;
	INT i, nSection;
	DWORD64 dwOffset = 0;

	pDosHdr = (PIMAGE_DOS_HEADER)pModule;
	pNtHdr32 = (PIMAGE_NT_HEADERS32)((UINT_PTR)pModule + pDosHdr->e_lfanew);

	if (pNtHdr32->Signature != IMAGE_NT_SIGNATURE)
	{
		return 0;
	}

	if (pNtHdr32->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
	{
		PIMAGE_SECTION_HEADER pSectionHdr;

		*pbX64 = FALSE;
		NumberOfSections = pNtHdr32->FileHeader.NumberOfSections;
		pSectionHdr = (PIMAGE_SECTION_HEADER)(pNtHdr32 + 1);

		nSection = 0;
		for (i = 0; i < NumberOfSections; i++)
		{
			if (dwAdress >= pSectionHdr[i].VirtualAddress &&
			    pSectionHdr[i].PointerToRawData > pSectionHdr[nSection].PointerToRawData)
			{
				nSection = i;
			}
		}
		dwOffset = pSectionHdr[nSection].PointerToRawData + dwAdress - pSectionHdr[nSection].VirtualAddress;
		return dwOffset;
	}
	else
	{
		*pbX64 = TRUE;
		printf("x64 is unsupported atm\n");
		return 0;
	}
}

DWORD64
GetOffsetFromName(HANDLE hProcess, PSYMBOL_INFO pSym, PBYTE pModule, PCSTR Name, PBOOL pbX64)
{
	pSym->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSym->MaxNameLen = MAX_SYMBOL_NAME-1;

	if (!SymFromName(hProcess, Name, pSym))
	{
		printf("SymGetSymFromName64() failed: %ld\n", GetLastError());
		return 0;
	}
	printf("looking up adress for %s: 0x%llx\n", Name, pSym->Address);
	return GetOffsetFromAdress64(pModule, pSym->Address - pSym->ModBase, pbX64);
}

BOOL CALLBACK EnumSymbolsProc(
	PSYMBOL_INFO pSymInfo,
	ULONG SymbolSize,
	PVOID UserContext)
{
	printf("%s@%d ", pSymInfo->Name, (UINT)UserContext);
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
    BOOL bX64;
    DWORD64 dwW32pServiceTable, dwW32pServiceLimit, dwW32pArgumentTable;
    DWORD dwServiceLimit;
    BYTE *pdwArgs;

	struct
	{
		SYMBOL_INFO	Symbol;
		CHAR		Name[MAX_SYMBOL_NAME];
	} Sym;

	printf("Win32k Syscall dumper\n");
	printf("Copyright (c) Timo Kreuzer 2007\n");

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

	dwW32pServiceTable = GetOffsetFromName(hProcess, &Sym.Symbol, pModule, "W32pServiceTable", &bX64);
	dwW32pServiceLimit = GetOffsetFromName(hProcess, &Sym.Symbol, pModule, "W32pServiceLimit", &bX64);
	dwW32pArgumentTable = GetOffsetFromName(hProcess, &Sym.Symbol, pModule, "W32pArgumentTable", &bX64);
	printf("dwW32pServiceTable = %llx\n", dwW32pServiceTable);
	printf("dwW32pServiceLimit = %llx\n", dwW32pServiceLimit);
	printf("dwW32pArgumentTable = %llx\n", dwW32pArgumentTable);

	if (!dwW32pServiceTable || !dwW32pServiceLimit || !dwW32pArgumentTable)
	{
		printf("Couldn't find adress!\n");
		goto cleanup;
	}

	dwServiceLimit = *((DWORD*)(pModule + dwW32pServiceLimit));
	pdwArgs = (BYTE*)(pModule + dwW32pArgumentTable);

	if (!bX64)
	{
		DWORD *pdwEntries32 = (DWORD*)(pModule + dwW32pServiceTable);

		for (i = 0; i < dwServiceLimit; i++)
		{
			printf("0x%x:", i+0x1000);
			SymEnumSymbolsForAddr(hProcess, (DWORD64)pdwEntries32[i], EnumSymbolsProc, (PVOID)(DWORD)pdwArgs[i]);
			printf("\n");
		}
	}
	else
	{
		DWORD64 *pdwEntries64 = (DWORD64*)(pModule + dwW32pServiceTable);

		for (i = 0; i < dwServiceLimit; i++)
		{
			printf("0x%x:", i+0x1000);
			SymEnumSymbolsForAddr(hProcess, (DWORD64)pdwEntries64[i], EnumSymbolsProc, (PVOID)(i+0x1000));
			printf("\n");
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
