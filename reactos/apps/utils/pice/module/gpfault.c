/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    GPFault.c

Abstract:

    handle general protection faults on x86

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    13-Nov-1999:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"
#include "precomp.h"

////////////////////////////////////////////////////
// GLOBALS
////
ULONG OldGPFaultHandler = 0;

char tempGP[1024];

////////////////////////////////////////////////////
// FUNCTIONS
////

//*************************************************************************
// NewGPFaultHandler()
//
//*************************************************************************
void HandleGPFault(FRAME* ptr)
{
	DPRINT((0,"HandleGPFault(): ptr = %x at eip: %x\n",ptr, ptr->eip));
}

//*************************************************************************
// NewGPFaultHandler()
//
//*************************************************************************
__asm__ ("\n\t \
NewGPFaultHandler:\n\t \
		pushfl\n\t \
        cli\n\t \
        cld\n\t \
        pushal\n\t \
	    pushl %ds\n\t \
\n\t \
		// test for v86 mode.
 		testl $0x20000,40(%esp)\n\t \
		jnz notv86\n\t \
		popl %ds\n\t \
        popal\n\t \
		popfl\n\t \
		.byte 0x2e\n\t \
		jmp *_OldGPFaultHandler\n\t \
notv86:\n\t \
		// setup default data selectors\n\t \
	    movw %ss,%ax\n\t \
	    movw %ax,%ds\n\t \
\n\t \
        // get frame ptr\n\t \
        lea 40(%esp),%eax\n\t \
		pushl %eax\n\t \
        call _HandleGPFault\n\t \
        addl $4,%esp\n\t \
\n \t \
		popl %ds\n\t \
        popal\n\t \
		popfl\n\t \
		// remove error code from stack and replace with reason code\n\t \
        movl $" STR(REASON_GP_FAULT) ",(%esp)\n\t \
		// call debugger loop\n\t \
		jmp NewInt31Handler\n\t \
		");

//*************************************************************************
// InstallGPFaultHook()
//
//*************************************************************************
void InstallGPFaultHook(void)
{
	ULONG LocalGPFaultHandler;

	ENTER_FUNC();

	MaskIrqs();
	if(!OldGPFaultHandler)
	{
		__asm__("mov $NewGPFaultHandler,%0"
			:"=r" (LocalGPFaultHandler)
			:
			:"eax");
		OldGPFaultHandler=SetGlobalInt(0x0D,(ULONG)LocalGPFaultHandler);
	}
	UnmaskIrqs();

    LEAVE_FUNC();
}

//*************************************************************************
// DeInstallGPFaultHook()
//
//*************************************************************************
void DeInstallGPFaultHook(void)
{
	ENTER_FUNC();

	MaskIrqs();
	if(OldGPFaultHandler)
	{
        RemoveAllSWBreakpoints(TRUE);
		SetGlobalInt(0x0D,(ULONG)OldGPFaultHandler);
        OldGPFaultHandler=0;
	}
	UnmaskIrqs();

    LEAVE_FUNC();
}

// EOF
