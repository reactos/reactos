#include <ntddk.h>
#include <winddk.h>

NTKERNELAPI
VOID
DbgBreakPoint() { __asm__("ti 31,0,0"); }

NTKERNELAPI
VOID
DbgBreakPointWithStatus(ULONG Status) { __asm__("ti 31,0,0"); }

NTSTATUS
NTAPI
DebugService
(ULONG Service, const void *Buffer, ULONG Length, PVOID Arg1, PVOID Arg2)
{
    NTSTATUS Result;
    __asm__("mr 3,%2\n\t"
	    "mr 4,%3\n\t"
	    "mr 5,%4\n\t"
	    "mr 6,%5\n\t"
	    "mr 7,%6\n\t"
	    "mr 8,%1\n\t"
	    "sc\n\t"
	    "mr %0,3\n\t" :
	    "=r" (Result) :
	    "r" (0x10000),
	    "r" (Service),
	    "r" (Buffer),
	    "r" (Length),
	    "r" (Arg1),
	    "r" (Arg2) );
    return Result;
}

NTSTATUS
NTAPI
DebugService2
(PVOID Arg1, PVOID Arg2, ULONG Service)
{
    return STATUS_SUCCESS;
}
