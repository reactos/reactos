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

	if (osv.dwMajorVersion == 5 && osv.dwMinorVersion == 0 && osv.dwBuildNumber == 2195)
	{
		printf("Running on Windows 2000, build 2195\n");
		g_nOs = OS_WINDOWS;
		g_SyscallTable = SyscallTable_2K_2195;
		return TRUE;
	}

	printf("Unsupported OS\n");

	return FALSE;
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

static DWORD STDCALL
WinSyscall(INT nSyscalNum, PVOID pFirstParam)
{
	DWORD ret;
	asm volatile ("int $0x2e\n" : "=a"(ret): "a" (nSyscalNum), "d" (pFirstParam));
	return ret;
}

static DWORD STDCALL
RosSyscall(FARPROC proc, UINT cParams, PVOID pFirstParam)
{
	DWORD ret;

	asm volatile 
	(
		"pushfl;"				// Save flags
		"movl %%ecx, %%eax;"
		"shl $2, %%eax;"		// Calculate param size
		"subl %%eax, %%esp;"	// Calculate new stack pos
		"movl %%esp, %%edi;"	// Destination is stackpointer
		"cld;"					// Clear direction flag
		"rep movsd;"			// Copy params to the stack
		"call *%%edx;"			// Call function
		"popfl;"				// Restore flags
		: "=a" (ret)
		: "S" (pFirstParam), "c" (cParams), "d"(proc)
		: "%edi"
	);

	return ret;
}

DWORD
Syscall(LPWSTR pszFunction, int cParams, void* pParams)
{
 	if (g_nOs == OS_REACTOS)
	{
		char szFunctionName[MAX_PATH];

		sprintf(szFunctionName, "%ls", pszFunction);
		FARPROC proc = (FARPROC)GetProcAddress(g_hModule, szFunctionName);
		if (!proc)
		{
			printf("Couldn't find proc: %s\n", szFunctionName);
			return FALSE;
		}

		return RosSyscall(proc, cParams, pParams);
	}
	else
	{
		PSYSCALL_ENTRY pEntry = GetSyscallEntry(pszFunction);
		return WinSyscall(pEntry->nSyscallNum, pParams);
	}
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
	// IsGUIThread(TRUE); <- does not exists on win2k

	if (!InitOsVersion())
	{
		return 0;
	}

	printf("\n");

	return TestMain(L"w32knapi", L"win32k.sys Nt-Api");
}
