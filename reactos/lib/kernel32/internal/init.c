#include <windows.h>
#include <ddk/ntddk.h>
#include <stdarg.h>

VOID KERNEL32_Init(VOID)
{
   __HeapInit(0, 4*1024*1024, 4*1024*1024);
}
