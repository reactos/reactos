#include "headers.hxx"
extern "C" void InitDebugLib(HANDLE, BOOL (WINAPI *)(HANDLE, DWORD, LPVOID));
extern "C" void TermDebugLib(HANDLE, BOOL);
#define DLL_MAIN_FUNCTION_NAME  _DllMainStartupDebug
#define DLL_MAIN_PRE_CINIT      InitDebugLib(hDllHandle, DLL_MAIN_FUNCTION_NAME);
#define DLL_MAIN_PRE_CEXIT      TermDebugLib(hDllHandle, FALSE);
#define DLL_MAIN_POST_CEXIT     TermDebugLib(hDllHandle, TRUE);
#include "dllmain.cxx"
