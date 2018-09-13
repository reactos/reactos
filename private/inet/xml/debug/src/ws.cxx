//+------------------------------------------------------------------------
//  
//  Microsoft Forms
// Copyright (c) 1996 - 1999 Microsoft Corporation. All rights reserved.*///  
//  File:       Working set test infrastructure
//  
//-------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_PSAPI_H_
#define X_PSAPI_H_
#include "psapi.h"
#endif

#define MAX_PAGE_ENTRIES 256
#define MAXMODULES 64
#define MAX_WS_DELTA_PAGES 1024

struct WSPAGEINFO
{
   DWORD fAttr      :  8;
   DWORD fShared    :  1;
   DWORD filler     :  3;
   DWORD addrPage   : 20;
};

struct WSPAGEENTRY
{
    DWORD_PTR addrStart;
    DWORD_PTR addrEnd;
    TCHAR szModule[16];
    TCHAR szSection[8];
    DWORD_PTR dwSize;
};

static WSPAGEENTRY s_aPE[MAX_PAGE_ENTRIES];
static PSAPI_WS_WATCH_INFORMATION s_aWsChanges[MAX_WS_DELTA_PAGES];
static DWORD_PTR s_cPageEntries = 0;
static HINSTANCE s_hmod = NULL;
static BOOL s_bWsDeltaStarted = FALSE;

static BOOL (WINAPI *s_pfnGetProcessMemoryInfo)(HANDLE Process, PPROCESS_MEMORY_COUNTERS ppmemctrs, DWORD cb);
static BOOL (WINAPI *s_pfnEmptyWorkingSet)(HANDLE hProcess);
static BOOL (WINAPI *s_pfnQueryWorkingSet)(HANDLE hProcess, PVOID pv, DWORD cb);
static BOOL (WINAPI *s_pfnEnumProcessModules)(HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded);
static DWORD (WINAPI *s_pfnGetModuleBaseName)(HANDLE hProcess, HMODULE hModule, LPTSTR lpBaseName, DWORD nSize);
static BOOL (WINAPI *s_pfnGetWsChanges)(HANDLE hProcess, PPSAPI_WS_WATCH_INFORMATION lpWatchInfo, DWORD cb);
static BOOL (WINAPI *s_pfnInitializeProcessForWsWatch)(HANDLE hProcess);

HRESULT LoadPSAPI()
{
    if (s_hmod)
	{
        return S_OK;
	}

    s_hmod = LoadLibrary(_T("PSAPI"));
    if (!s_hmod)
	{
        RRETURN(E_FAIL);
	}

    *(void **)&s_pfnGetProcessMemoryInfo = GetProcAddress(s_hmod, "GetProcessMemoryInfo");
    *(void **)&s_pfnQueryWorkingSet = GetProcAddress(s_hmod, "QueryWorkingSet");
    *(void **)&s_pfnEmptyWorkingSet = GetProcAddress(s_hmod, "EmptyWorkingSet");
    *(void **)&s_pfnEnumProcessModules = GetProcAddress(s_hmod, "EnumProcessModules");
    *(void **)&s_pfnGetModuleBaseName = GetProcAddress(s_hmod, "GetModuleBaseNameW");
    *(void **)&s_pfnGetWsChanges = GetProcAddress(s_hmod, "GetWsChanges");
    *(void **)&s_pfnInitializeProcessForWsWatch = GetProcAddress(s_hmod, "InitializeProcessForWsWatch");

    return S_OK;
}

HRESULT UnLoadPSAPI()
{
    if (!s_hmod)
	{
        return S_OK;
	}

    if (!FreeLibrary(s_hmod))
	{
		RRETURN(E_FAIL);
	}

    s_hmod = NULL;
	s_pfnGetProcessMemoryInfo = NULL;
    s_pfnQueryWorkingSet = NULL;
    s_pfnEmptyWorkingSet = NULL;
    s_pfnEnumProcessModules = NULL;
    s_pfnGetModuleBaseName = NULL;
    s_pfnGetWsChanges = NULL;
    s_pfnInitializeProcessForWsWatch = NULL;

    return S_OK;
}

HRESULT WsClear(HANDLE hProcess)
{
	HRESULT hr;

    hr = LoadPSAPI();
    if (S_OK != hr)
	{
		// No PSAPI for Win95, so return doing nothing if
		// LoadLibrary fails!
		hr = S_OK;
        goto Cleanup;
	}

    if (!(*s_pfnEmptyWorkingSet)(hProcess))
    {
		hr = E_FAIL;
        goto Cleanup;
    }

    s_cPageEntries = 0;

Cleanup:
	RRETURN(hr);
}

DWORD AddModuleSections(HANDLE hProcess, PVOID pBase, WSPAGEENTRY **ppPE)
{
    BYTE *pifh;
    PIMAGE_SECTION_HEADER pish;
    ULONG i;
    ULONG cSections = 0;

    pifh = (BYTE *)pBase + ((PIMAGE_DOS_HEADER)pBase)->e_lfanew + sizeof(DWORD);
    
    cSections = ((PIMAGE_FILE_HEADER)pifh)->NumberOfSections;
    if (cSections + s_cPageEntries > MAX_PAGE_ENTRIES)
    {
        return cSections;
    }

    pish = (PIMAGE_SECTION_HEADER)(pifh + sizeof(IMAGE_FILE_HEADER) + ((PIMAGE_FILE_HEADER)pifh)->SizeOfOptionalHeader);

    for (i=0; i<cSections; i++, pish++, (*ppPE)++)
    {
        (*ppPE)->addrStart = (DWORD_PTR)pBase + (DWORD_PTR)pish->VirtualAddress;
        (*ppPE)->addrEnd = (DWORD_PTR)pBase + pish->VirtualAddress + pish->SizeOfRawData - 1;
        (*s_pfnGetModuleBaseName)(hProcess, (HMODULE)pBase, (*ppPE)->szModule, sizeof((*ppPE)->szModule));
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)pish->Name, -1, (LPWSTR)(*ppPE)->szSection, sizeof((*ppPE)->szSection));
        (*ppPE)->dwSize = 0;
    }

    return cSections;
}

HRESULT FillPETable(HANDLE hProcess)
{
	HRESULT hr = S_OK;
    HMODULE ahModule[MAXMODULES];
    ULONG i;
    WSPAGEENTRY *pPE;
    MEMORY_BASIC_INFORMATION mbi;
    DWORD cb;

    if (!(*s_pfnEnumProcessModules)(hProcess, ahModule, sizeof(ahModule), &cb))
    {
	    hr = E_FAIL;
		goto Cleanup;
    }

    for (i = 0, pPE = s_aPE; i < cb / sizeof(HMODULE); i++)
    {
        s_cPageEntries += AddModuleSections(hProcess, (PVOID)ahModule[i], &pPE);

        if (s_cPageEntries > MAX_PAGE_ENTRIES)
        {
		    hr = E_OUTOFMEMORY;
			goto Cleanup;
        }
    }

    if (!VirtualQuery(GetProcessHeap(), &mbi, sizeof(mbi)))
    {
	    hr = E_FAIL;
		goto Cleanup;
    }

    pPE->addrStart = (DWORD_PTR)mbi.BaseAddress;
    pPE->addrEnd = (DWORD_PTR)mbi.BaseAddress + mbi.RegionSize;
    lstrcpy (pPE->szModule, _T("Process Heap"));
    pPE->szSection[0] = 0;
    pPE++->dwSize = 0;
    
    s_cPageEntries++;
    if (s_cPageEntries > MAX_PAGE_ENTRIES)
    {
        hr = E_OUTOFMEMORY;
	    goto Cleanup;
    }

    if (!VirtualQuery(&cb, &mbi, sizeof(mbi)))
    {
	    hr = E_FAIL;
		goto Cleanup;
    }

    pPE->addrStart = (DWORD_PTR)mbi.BaseAddress;
    pPE->addrEnd = (DWORD_PTR)mbi.BaseAddress + mbi.RegionSize;
    lstrcpy (pPE->szModule, _T("Thread Stack"));
    pPE->szSection[0] = 0;
    pPE++->dwSize = 0;

    s_cPageEntries++;
    if (s_cPageEntries > MAX_PAGE_ENTRIES)
    {
        hr = E_OUTOFMEMORY;
	    goto Cleanup;
    }

    pPE->addrStart = 0;
    pPE->addrEnd = 0xFFFFFFFF;
    lstrcpy (pPE->szModule, _T("(Unknown)"));
    pPE->szSection[0] = 0;
    pPE->dwSize = 0;

    s_cPageEntries++;
    if (s_cPageEntries > MAX_PAGE_ENTRIES)
    {
        hr = E_OUTOFMEMORY;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT WsTakeSnapshot(HANDLE hProcess)
{
	HRESULT hr = S_OK;
    DWORD_PTR i,j;
    WSPAGEINFO *pWSPages = NULL, *pBufHead = NULL;
    PROCESS_MEMORY_COUNTERS psvmctr;
    SYSTEM_INFO si;
    DWORD_PTR cPages = 0;

    memset(s_aPE, 0, sizeof(s_aPE));
    s_cPageEntries = 0;

    hr = LoadPSAPI();
    if (S_OK != hr)
	{
		// No PSAPI for Win95, so return doing nothing if
		// LoadLibrary fails!
		hr = S_OK;
        goto Cleanup;
	}

    if (!(*s_pfnGetProcessMemoryInfo)(hProcess, &psvmctr, sizeof(psvmctr)))
    {
	    hr = E_FAIL;
		goto Cleanup;
    }

    GetSystemInfo(&si);
    // Double the buffer size to make sure that QueryWorkingSet always succeeds!
    cPages = ((DWORD_PTR)psvmctr.WorkingSetSize / si.dwPageSize)*2 + 1;

    pWSPages = pBufHead = new WSPAGEINFO[(ULONG)cPages];
	if (!pWSPages)
    {
        hr = E_OUTOFMEMORY;
	    goto Cleanup;
    }

    if (!(*s_pfnQueryWorkingSet)(hProcess, pWSPages, (ULONG)cPages * sizeof(WSPAGEINFO)))
    {
	    hr = E_FAIL;
		goto Cleanup;
    }

    hr = FillPETable(hProcess);
	if (S_OK != hr)
    {
	    goto Cleanup;
    }
    
    cPages = *((DWORD *)pWSPages++);
    for (i=0; i<cPages; i++, pWSPages++)
    {
        for (j=0; j<s_cPageEntries; j++)
        {
            if ((*(DWORD *)pWSPages & 0xFFFFF000) >= s_aPE[j].addrStart && (*(DWORD*)pWSPages & 0xFFFFF000) <= s_aPE[j].addrEnd)
            {
                s_aPE[j].dwSize += si.dwPageSize;
                break;
            }
        }
    }

Cleanup:
    delete pBufHead;
    RRETURN(hr);
}

BSTR WsGetModule(LONG_PTR row)
{
    if (row >= 0 && (DWORD_PTR)row < s_cPageEntries)
        return SysAllocString(s_aPE[row].szModule);
    else
        return NULL;
}

BSTR WsGetSection(LONG_PTR row)
{
    if (row >= 0 && (DWORD_PTR)row < s_cPageEntries)
        return SysAllocString(s_aPE[row].szSection);
    else
        return NULL;
}

HRESULT WsStartDelta(HANDLE hProcess)
{
	HRESULT hr;

    hr = LoadPSAPI();
    if (S_OK != hr)
	{
		// No PSAPI for Win95, so return doing nothing if
		// LoadLibrary fails!
		hr = S_OK;
        goto Cleanup;
	}

    if (!(*s_pfnInitializeProcessForWsWatch)(hProcess))
    {
		hr = E_FAIL;
        goto Cleanup;
    }

    // clear all WS changes till before the call to WsStartDelta
    (*s_pfnGetWsChanges)(hProcess, s_aWsChanges, sizeof(s_aWsChanges));
    s_bWsDeltaStarted = TRUE;

Cleanup:
	RRETURN(hr);
}

LONG_PTR WsEndDelta(HANDLE hProcess)
{
    LONG_PTR nPageFaults = -1;
    PPSAPI_WS_WATCH_INFORMATION pWsChanges = s_aWsChanges;

    if (s_bWsDeltaStarted)
    {
        (*s_pfnGetWsChanges)(hProcess, s_aWsChanges, sizeof(s_aWsChanges));
        if (s_aWsChanges[MAX_WS_DELTA_PAGES-1].FaultingPc)
        {
            nPageFaults = MAX_WS_DELTA_PAGES;
        }
        else
        {
            while (pWsChanges->FaultingPc || pWsChanges->FaultingVa)
            {
                pWsChanges++;
            }
            
            nPageFaults = (LONG_PTR)pWsChanges - (LONG_PTR)s_aWsChanges;
        }

        s_bWsDeltaStarted = FALSE;
    }

    return nPageFaults;
}

LONG_PTR WsSize(LONG_PTR row)
{
    if (row >= 0 && (DWORD_PTR)row < s_cPageEntries)
        return s_aPE[row].dwSize;
    else
        return 0;
}

LONG_PTR WsCount()
{
    return s_cPageEntries;
}

LONG_PTR WsTotal()
{
    DWORD_PTR i;
    LONG_PTR total = 0;
    
    for (i=0; i<s_cPageEntries; i++)
    {
        total += (long)s_aPE[i].dwSize;
    }

    return total;
}
