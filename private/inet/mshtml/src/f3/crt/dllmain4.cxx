#include "headers.hxx"
extern "C" void InitDebugLib(HANDLE, BOOL (WINAPI *)(HANDLE, DWORD, LPVOID));
extern "C" void InitDebugChkStk(HANDLE, BOOL (WINAPI *)(HANDLE, DWORD, LPVOID));
extern "C" void TermDebugLib(HANDLE, BOOL);
extern "C" void InitChkStk(unsigned long dwFill);
#undef  DLL_MAIN_FUNCTION_NAME
#undef  DLL_MAIN_PRE_CINIT
#define DLL_MAIN_FUNCTION_NAME  _DllMainStartupDebugChkStk
#define DLL_MAIN_PRE_CINIT      InitDebugChkStk(hDllHandle, DLL_MAIN_FUNCTION_NAME);
#define DLL_MAIN_PRE_CEXIT      TermDebugLib(hDllHandle, FALSE);
#define DLL_MAIN_POST_CEXIT     TermDebugLib(hDllHandle, TRUE);
#include "dllmain.cxx"
#include <mshtmdbg.h>

extern "C"
void
InitDebugChkStk(HANDLE hDllHandle, BOOL (WINAPI * pfnDllMain)(HANDLE, DWORD, LPVOID))
{
    DWORD dwFill;

    InitDebugLib(hDllHandle, pfnDllMain);

    if (DbgExGetChkStkFill(&dwFill))
    {
        InitChkStk(dwFill);
    }
}
