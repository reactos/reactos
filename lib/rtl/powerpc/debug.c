#include <ntddk.h>
#include <winddk.h>

NTKERNELAPI
VOID
DbgBreakPoint() { __asm__("ti 31,0,0"); }

NTKERNELAPI
VOID
DbgBreakPointWithStatus(ULONG Status) { __asm__("ti 31,0,0"); }

ULONG
NTAPI
DebugService
(ULONG Service, PVOID Argument1, PVOID Argument1, PVOID Argument3, PVOID Argument4)
{
    ULONG Result;
    __asm__("mr 0,%1\n\t"
            "mr 3,%2\n\t"
	    "mr 4,%3\n\t"
	    "mr 5,%4\n\t"
	    "mr 6,%5\n\t"
	    "mr 7,%6\n\t"
	    "sc\n\t"
	    "mr %0,3\n\t" :
	    "=r" (Result) :
	    "r" (0x10000),
	    "r" (Service),
	    "r" (Argument1),
	    "r" (Argument2),
	    "r" (Argument3),
	    "r" (Argument4) );
    return Result;
}

VOID
NTAPI
DebugService2
(PVOID Arg1, PVOID Arg2, ULONG Service)
{
}
