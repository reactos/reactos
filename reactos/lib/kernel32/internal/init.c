#include <windows.h>
#include <ddk/ntddk.h>
#include <stdarg.h>

#include <kernel32/kernel32.h>


VOID KERNEL32_Init()
{
   DPRINT("KERNEL32_Init()\n");
   __HeapInit(0, 4*1024*1024, 4*1024*1024);
}
