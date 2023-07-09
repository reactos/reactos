#include <windows.h>

extern BOOL WINAPI (DllMainInternal)(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved);

EXTERN_C DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved);
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
    return ((DllMainInternal)(hInst, dwReason, lpvReserved));
}
