#include "w32knapi.h"

HINSTANCE g_hInstance;
HMODULE g_hModule = NULL;
PGDI_TABLE_ENTRY GdiHandleTable;

static
PGDI_TABLE_ENTRY
MyGdiQueryTable()
{
    PTEB pTeb = NtCurrentTeb();
    PPEB pPeb = pTeb->ProcessEnvironmentBlock;
    return pPeb->GdiSharedHandleTable;
}

BOOL
IsHandleValid(HGDIOBJ hobj)
{
    USHORT Index = (ULONG_PTR)hobj;
    PGDI_TABLE_ENTRY pentry = &GdiHandleTable[Index];

    if (pentry->KernelData == NULL ||
        pentry->KernelData < (PVOID)0x80000000 ||
        (USHORT)pentry->FullUnique != (USHORT)((ULONG_PTR)hobj >> 16))
    {
        return FALSE;
    }

    return TRUE;
}

PVOID
GetHandleUserData(HGDIOBJ hobj)
{
    USHORT Index = (ULONG_PTR)hobj;
    PGDI_TABLE_ENTRY pentry = &GdiHandleTable[Index];

    if (pentry->KernelData == NULL ||
        pentry->KernelData < (PVOID)0x80000000 ||
        (USHORT)pentry->FullUnique != (USHORT)((ULONG_PTR)hobj >> 16))
    {
        return NULL;
    }

    return pentry->UserData;
}


static DWORD WINAPI
IntSyscall(FARPROC proc, UINT cParams, PVOID pFirstParam)
{
    DWORD retval;

#ifdef __GNUC__
    asm volatile
    (
        "pushfl;"               // Save flags
        "movl %%ecx, %%eax;"
        "shl $2, %%eax;"        // Calculate param size
        "subl %%eax, %%esp;"    // Calculate new stack pos
        "movl %%esp, %%edi;"    // Destination is stackpointer
        "cld;"                  // Clear direction flag
        "rep movsd;"            // Copy params to the stack
        "call *%%edx;"          // Call function
        "popfl;"                // Restore flags
        : "=a" (retval)
        : "S" (pFirstParam), "c" (cParams), "d"(proc)
        : "%edi"
    );
#else
    __asm
    {
        pushf
        mov eax, cParams
        shl eax, 2
        sub esp, eax
        mov edi, esp
        cld
        rep movsd
        call proc
        mov retval, eax
        popf
    };
#endif

    return retval;
}

DWORD
Syscall(LPWSTR pszFunction, int cParams, void* pParams)
{
    char szFunctionName[MAX_PATH];
    FARPROC proc;

    sprintf(szFunctionName, "%ls", pszFunction);
    proc = (FARPROC)GetProcAddress(g_hModule, szFunctionName);
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

    printf("Win32k native API test\n");

    /* Convert to gui thread */
    // IsGUIThread(TRUE); <- does not exists on win2k

    InitOsVersion();
    printf("g_OsIdx = %d\n", g_OsIdx);

    g_hModule = LoadLibraryW(L"w32kdll.dll");
    if (!g_hModule)
    {
        printf("w32kdll.dll not found!\n");
        return -1;
    }

    GdiHandleTable = MyGdiQueryTable();
    if(!GdiHandleTable)
    {
        FreeLibrary(g_hModule);
        printf("GdiHandleTable not found!\n");
        return -1;
    }

    printf("\n");

    return TestMain(L"w32knapi", L"win32k.sys Nt-Api");
}
