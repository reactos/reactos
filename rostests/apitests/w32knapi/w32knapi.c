#include "w32knapi.h"

HINSTANCE g_hInstance;
HMODULE g_hModule = NULL;
PGDI_TABLE_ENTRY GdiHandleTable;

static DWORD STDCALL
IntSyscall(FARPROC proc, UINT cParams, PVOID pFirstParam)
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
	char szFunctionName[MAX_PATH];

	sprintf(szFunctionName, "%ls", pszFunction);
	FARPROC proc = (FARPROC)GetProcAddress(g_hModule, szFunctionName);
	if (!proc)
	{
		printf("Couldn't find proc: %s\n", szFunctionName);
		return FALSE;
	}

	return IntSyscall(proc, cParams, pParams);
}

BOOL
IsFunctionPresent(LPWSTR lpszFunction)
{
	char szFunctionName[MAX_PATH];
	sprintf(szFunctionName, "%ls", lpszFunction);
	return (GetProcAddress(g_hModule, szFunctionName) != NULL);
}

int APIENTRY
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR     lpCmdLine,
        int       nCmdShow)
{
	g_hInstance = hInstance;
	GDIQUERYPROC GdiQueryHandleTable;

	printf("Win32k native API test\n");

	/* Convert to gui thread */
	// IsGUIThread(TRUE); <- does not exists on win2k

	g_hModule = LoadLibraryW(L"w32kdll.dll");
	if (!g_hModule)
	{
		printf("w32kdll.dll not found!\n");
		return -1;
	}

	GdiQueryHandleTable = (GDIQUERYPROC)GetProcAddress(GetModuleHandleW(L"GDI32.DLL"), "GdiQueryTable");
	if(!GdiQueryHandleTable)
	{
		return -1;
	}
	GdiHandleTable = GdiQueryHandleTable();
	if(!GdiHandleTable)
	{
		return -1;
	}

	printf("\n");

	return TestMain(L"w32knapi", L"win32k.sys Nt-Api");
}
