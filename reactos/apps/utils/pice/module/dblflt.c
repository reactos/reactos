/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    dblflt.c

Abstract:

    handle double faults on x86

Environment:

    LINUX 2.2.X
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
#include <asm/delay.h>

#include "precomp.h"

////////////////////////////////////////////////////
// GLOBALS
////
ULONG OldDblFltHandler = 0;

////////////////////////////////////////////////////
// FUNCTIONS
////

//************************************************************************* 
// HandleDoubleFault() 
// 
//************************************************************************* 
void HandleDoubleFault(FRAME* ptr)
{
    DPRINT((0,"HandleDoubleFault(): ptr = %x\n",ptr));
}


//************************************************************************* 
// NewDblFltHandler() 
// 
//************************************************************************* 
__asm__ (" 
NewDblFltHandler:
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
        call HandleDoubleFault
        addl $4,%esp

	    popl %ds 
        popal
		popfl
		// remove error code from stack and replace with reason code
        movl $" STR(REASON_DOUBLE_FAULT) ",(%esp)
		// call debugger loop
		jmp NewInt31Handler");


//************************************************************************* 
// InstallDblFltHook() 
// 
//************************************************************************* 
void InstallDblFltHook(void)
{
	ULONG LocalDblFltHandler;

	ENTER_FUNC();

	MaskIrqs();
	if(!OldDblFltHandler)
	{
		__asm__("mov $NewDblFltHandler,%0"
			:"=r" (LocalDblFltHandler)
			:
			:"eax");
		OldDblFltHandler=SetGlobalInt(0x08,(ULONG)LocalDblFltHandler);
	}
	UnmaskIrqs();

    LEAVE_FUNC();
}

//************************************************************************* 
// DeInstallDblFltHook() 
// 
//************************************************************************* 
void DeInstallDblFltHook(void)
{
	ENTER_FUNC();

	MaskIrqs();
	if(OldDblFltHandler)
	{
        RemoveAllSWBreakpoints(TRUE);
		SetGlobalInt(0x08,(ULONG)OldDblFltHandler);
        OldDblFltHandler=0;
	}
	UnmaskIrqs();

    LEAVE_FUNC();
}

// EOF