#include <windows.h>
#include <ddk/ntddk.h>
#include <stdarg.h>

VOID WINAPI __HeapInit(LPVOID base, ULONG minsize, ULONG maxsize);

VOID KERNEL32_Init(PWSTR Args)
{
   InitializePeb(Args);
   __HeapInit(0, 4*1024*1024, 4*1024*1024);
}
