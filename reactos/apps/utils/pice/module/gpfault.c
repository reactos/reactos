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
    DPRINT((0,"HandleGPFault(): ptr = %x\n",ptr));
}

//*************************************************************************
// NewGPFaultHandler()
//
//*************************************************************************
__asm__ ("
NewGPFaultHandler:
		pushfl
        cli
        cld
        pushal
	    pushl %ds

	    // setup default data selectors
	    movw %ss,%ax
	    movw %ax,%ds

        // get frame ptr
        lea 40(%esp),%eax
        pushl %eax
        call _HandleGPFault
        addl $4,%esp

	    popl %ds
        popal
		popfl
		// remove error code from stack and replace with reason code
        movl $" STR(REASON_GP_FAULT) ",(%esp)
		// call debugger loop
		jmp NewInt31Handler");


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
