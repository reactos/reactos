#include "w32knapi.h"

HINSTANCE g_hInstance;
HMODULE g_hModule = NULL;
INT g_nOs;
PSYSCALL_ENTRY g_SyscallTable;

BOOL
InitOsVersion()
{
	OSVERSIONINFOW osv;
	LPWSTR pszRos;

	osv.dwOSVersionInfoSize = sizeof(osv);
	GetVersionExW(&osv);
	pszRos =  osv.szCSDVersion + wcslen(osv.szCSDVersion) + 1;
	/* make sure the string is zero terminated */
	osv.szCSDVersion[127] = 0;
	/* Is ReactOS? */
	if (wcsstr(pszRos, L"ReactOS") != NULL)
	{
		printf("Running on %ls\n", pszRos);
		g_hModule = LoadLibraryW(L"w32kdll.dll");
		if (!g_hModule)
		{
			printf("w32kdll.dll not found!\n");
			return FALSE;
		}
		g_nOs = OS_REACTOS;
		return TRUE;
	}

	if (osv.dwPlatformId != VER_PLATFORM_WIN32_NT)
	{
		printf("Unsupported OS\n");
		return FALSE;
	}

	if (osv.dwMajorVersion == 5 && osv.dwMinorVersion == 1 && osv.dwBuildNumber == 2600)
	{
		printf("Running on Windows XP, build 2600\n");
		g_nOs = OS_WINDOWS;
		g_SyscallTable = SyscallTable_XP_2600;
		return TRUE;
	}

	printf("Unsupported OS\n");

	return FALSE;
}


static BOOL
RosSyscall(LPWSTR lpszFunction, int cParams, void* pParams, DWORD* pResult)
{
	DWORD ret;
	char szFunctionName[MAX_PATH];
	int ParamSize = cParams * 4;

	sprintf(szFunctionName, "%ls", lpszFunction);
	FARPROC proc = (FARPROC)GetProcAddress(g_hModule, szFunctionName);
	if (!proc)
	{
		printf("Couldn't find proc: %s\n", szFunctionName);
		return FALSE;
	}

	asm volatile
	(
		"subl %%eax, %%esp;"	// calculate new stack pos
		"movl %%esp, %%edi;"	// destination is stackpointer
		"cld;"					// clear direction flag
		"rep movsd;"			// copy params to the stack
		"call *%%edx"			// call function
		: "=a" (ret)
		: "c" (cParams), "a" (ParamSize), "S"(pParams), "d"(proc)
	);

	*pResult = ret;
	return TRUE;
}

static PSYSCALL_ENTRY
GetSyscallEntry(LPWSTR lpszFunction)
{
	INT i;

	for (i = 0; g_SyscallTable[i].lpszFunction != NULL; i++)
	{
		if (wcscmp(g_SyscallTable[i].lpszFunction, lpszFunction) == 0)
		{
			return &g_SyscallTable[i];
		}
	}
	return NULL;
}

static BOOL
WinSyscall(LPWSTR pszFunction, void* pParams, void* pResult)
{
	PSYSCALL_ENTRY pEntry = GetSyscallEntry(pszFunction);
	DWORD ret;

	asm volatile ("int $0x2e\n" : "=a"(ret): "a" (pEntry->nSyscallNum), "d" (pParams));\
	*((DWORD*)pResult) = ret;
	return FALSE;
}

DWORD
Syscall(LPWSTR pszFunction, int cParams, void* pParams)
{
    DWORD dwRet = 0;

	if (g_nOs == OS_REACTOS)
	{
		RosSyscall(pszFunction, cParams, pParams, &dwRet);
	}
	else
	{
		WinSyscall(pszFunction, pParams, &dwRet);
	}
	return dwRet;
}

BOOL
IsFunctionPresent(LPWSTR lpszFunction)
{
	if (g_nOs == OS_REACTOS)
	{
		char szFunctionName[MAX_PATH];
		sprintf(szFunctionName, "%ls", lpszFunction);
		return (GetProcAddress(g_hModule, szFunctionName) != NULL);
	}

	return (GetSyscallEntry(lpszFunction) != NULL);
}

int APIENTRY
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR     lpCmdLine,
        int       nCmdShow)
{
	g_hInstance = hInstance;

	printf("Win32k native API test\n");

	/* Convert to gui thread */
	IsGUIThread(TRUE);

	if (!InitOsVersion())
	{
		return 0;
	}

	printf("\n");

	TestMain(L"w32knapi", L"win32k.sys Nt-Api");

	return 0;
}
