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
__asm__ (" \
NewDblFltHandler:\n\t \
		pushfl\n\t \
        cli;\n\t \
        cld;\n\t \
        pushal;\n\t \
	    pushl %ds;\n\t \
\n\t \
	    // setup default data selectors\n\t \
	    movw %ss,%ax\n\t \
	    movw %ax,%ds\n\t \
\n\t \
        // get frame ptr\n\t \
        lea 40(%esp),%eax\n\t \
        pushl %eax\n\t \
        call _HandleDoubleFault\n\t \
        addl $4,%esp\n\t \
\n\t \
	    popl %ds\n\t \
        popal\n\t \
		popfl\n\t \
		// remove error code from stack and replace with reason code\n\t \
        movl $" STR(REASON_DOUBLE_FAULT) ",(%esp)\n\t \
		// call debugger loop\n\t \
		jmp NewInt31Handler\n\t");


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
