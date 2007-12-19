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
	INT i;
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

		for (i = 0; i < NumberOfSections; i++)
		{
			if (dwAdress >= pSectionHdr[i].VirtualAddress &&
			    pSectionHdr[i].PointerToRawData > dwOffset)
			{
				dwOffset = pSectionHdr[i].PointerToRawData;
			}
		}
		return dwOffset;
	}
	else
	{
		*pbX64 = TRUE;
		printf("x64 is unsupported atm\n");
		return 0;
	}
}

int main(int argc, char* argv[])
{
	HANDLE hProcess;
	CHAR szModuleFileName[MAX_PATH+1];
    DWORD64 dwModuleBase;
    DWORD64 dwFileOffset;
    HANDLE hFile = 0, hMap = 0;
    PBYTE pModule = NULL;
    UINT i;
    BOOL bX64;

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

	Sym.Symbol.SizeOfStruct = sizeof(SYMBOL_INFO);
	Sym.Symbol.MaxNameLen = MAX_SYMBOL_NAME-1;

	if (!SymFromName(hProcess, "W32pServiceTable", &Sym.Symbol))
	{
		printf("SymGetSymFromName64() failed: %ld\n", GetLastError());
		goto cleanup;
	}

	printf("Address for W32pServiceTable = %llx\n", Sym.Symbol.Address);
	printf("Module base = %llx\n", dwModuleBase);

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

	dwFileOffset = GetOffsetFromAdress64(pModule, Sym.Symbol.Address - dwModuleBase, &bX64);
	if (!dwFileOffset)
	{
		printf("PE file is invalid!\n");
		goto cleanup;
	}

	if (!bX64)
	{
		DWORD *pdwEntries32 = (DWORD*)(pModule + dwFileOffset);

		for (i = 0; pdwEntries32[i] > dwModuleBase; i++)
		{
			SymFromAddr(hProcess, (DWORD64)pdwEntries32[i], 0, &Sym.Symbol);
			printf("0x%x:%s\n", i+0x1000, Sym.Symbol.Name);
		}
	}
	else
	{
		DWORD64 *pdwEntries64 = (DWORD64*)(pModule + dwFileOffset);

		for (i = 0; pdwEntries64[i] > dwModuleBase; i++)
		{
			SymFromAddr(hProcess, (DWORD64)pdwEntries64[i], 0, &Sym.Symbol);
			printf("0x%x:%s\n", i+0x1000, Sym.Symbol.Name);
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
