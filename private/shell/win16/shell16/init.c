#include "shprv.h"

HINSTANCE g_hinst = NULL;
int g_cxIcon, g_cyIcon;
int g_cxSmIcon, g_cySmIcon;

BOOL CALLBACK LibMain(HINSTANCE hinst, UINT wDS, DWORD unused)
{
    g_hinst = hinst;
    g_cxIcon = GetSystemMetrics(SM_CXICON);
    g_cyIcon = GetSystemMetrics(SM_CYICON);
    g_cxSmIcon = GetSystemMetrics(SM_CXSMICON);
    g_cySmIcon = GetSystemMetrics(SM_CYSMICON);
    return TRUE;
}

BOOL CALLBACK WEP(BOOL fSystemExit)
{
    return TRUE;
}

#pragma data_seg("_TEXT")
char const c_szShell16Dll[] = "shell.dll";
char const c_szShell32Dll[] = "shell32.dll";
#pragma data_seg()

// created by the thunk asm file
extern BOOL FAR PASCAL Shl1632_ThunkConnect16(LPCSTR pszDll16, LPCSTR pszDll32, WORD hInst, DWORD dwReason);
extern BOOL FAR PASCAL Shl3216_ThunkConnect16(LPCSTR pszDll16, LPCSTR pszDll32, WORD hInst, DWORD dwReason);

#define DLL_PROCESS_ATTACH 1    
#define DLL_THREAD_ATTACH  2    
#define DLL_THREAD_DETACH  3    
#define DLL_PROCESS_DETACH 0

// in pifmgr.c
extern void PifMgr_Free(void);

BOOL FAR PASCAL DllEntryPoint(DWORD dwReason, WORD  hInst, WORD  wDS, WORD wHeapSize, DWORD dwReserved1, WORD wReserved2)
{
    // The "win31compat" line in the thunk script now prevents premature
    // unloading of shell32.dll.
    if (!(Shl1632_ThunkConnect16(c_szShell16Dll, c_szShell32Dll, hInst, dwReason)))
    {
            return FALSE;
    }
    if (!(Shl3216_ThunkConnect16(c_szShell16Dll, c_szShell32Dll, hInst, dwReason)))
    {
            return FALSE;
    }


    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DebugMsg(DM_TRACE, "shell: ProcessAttach");
            break;

        case DLL_PROCESS_DETACH:
            DebugMsg(DM_TRACE, "shell: ProcessDettach");
            break;

        default:
            break;
    }

    return TRUE;
} 

#define ORD_GLOBALDEFECT        543

#pragma data_seg("_TEXT")
char const c_szKrnl386Exe[] = "krnl386.exe";
#pragma data_seg()

// NB Magic call to kernel just for WinFax Pro DDE. See the comment in cabinet\desktop.c
// for an explanation.
void SHGlobalDefect(DWORD dwHnd32)
{
    HANDLE hModKernel;
    void (WINAPI *GlobalDefect)(DWORD dwHnd32);
    
    hModKernel = LoadLibrary(c_szKrnl386Exe);
    if (hModKernel)
    {
        (FARPROC)GlobalDefect = GetProcAddress(hModKernel, MAKEINTATOM(ORD_GLOBALDEFECT));
        if (GlobalDefect)
             GlobalDefect(dwHnd32);
        FreeLibrary(hModKernel);
    }
}
