/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    trace.c

Abstract:

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    19-Aug-1998:	created

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"

#include "precomp.h"

extern void NewInt31Handler(void);

void DeInstallTraceHook(void);

volatile ULONG OldInt1Handler=0;

BOOLEAN InstallTraceHook(void)
{
	ULONG LocalInt1Handler;

    DPRINT((0,"InstallTraceHook(OldInt1Handler=%0.8x)...\n",OldInt1Handler));

	MaskIrqs();
	if(!OldInt1Handler)
	{
		__asm__("mov $NewInt1Handler,%0"
			:"=r" (LocalInt1Handler)
			:
			:"eax");
		OldInt1Handler=SetGlobalInt(0x01,(ULONG)LocalInt1Handler);
	}
	UnmaskIrqs();
	return TRUE;
}

//this asm function must be at least second in the file. otherwise gcc does not
//generate correct code.
__asm__("\n\t \
NewInt1Handler:\n\t \
       pushl %eax\n\t \
	 	movl %dr6,%eax\n\t \
	 	testl $(1<<14),%eax\n\t \
	 	jz exceptionnotsinglestep\n\t \
\n\t \
        popl %eax\n\t \
        pushl $" STR(REASON_SINGLESTEP) "\n\t \
        jmp NewInt31Handler\n\t \
\n\t \
exceptionnotsinglestep:\n\t \
        popl %eax\n\t \
        pushl $" STR(REASON_HARDWARE_BP) "\n\t \
        jmp NewInt31Handler\n\t \
");

void DeInstallTraceHook(void)
{
	DPRINT((0,"DeInstallTraceHook(OldInt1Handler=%0.8x)...\n",OldInt1Handler));

	MaskIrqs();
	if(OldInt1Handler)
	{
		SetGlobalInt(0x01,(ULONG)OldInt1Handler);
        OldInt1Handler = 0;
	}
	UnmaskIrqs();
}
