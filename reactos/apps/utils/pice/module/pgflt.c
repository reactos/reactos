/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    pgflt.c

Abstract:

    page fault handling on x86

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    25-Nov-1999:	created
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

char tempPageFault[1024];
extern void NewInt31Handler(void);

ULONG OldIntEHandler=0;
ULONG error_code;
BOOLEAN bInPageFaultHandler = FALSE;

////////////////////////////////////////////////////
// FUNCTIONS
////

//*************************************************************************
// HandleInDebuggerFault()
//
//*************************************************************************
ULONG HandleInDebuggerFault(FRAME* ptr,ULONG address)
{
	PEPROCESS tsk;

    ENTER_FUNC();

	DPRINT((0,"HandleInDebuggerFault(): ###### page fault @ %.8X while inside debugger\n",address));

	// fault in this page fault handler
	if(bInPageFaultHandler)
	{
    	DPRINT((0,"HandleInDebuggerFault(): ###### page fault @ %.8X while in page fault handler\n",address));

        DPRINT((0,"!!! machine is halted !!!\n"));

        __asm__ __volatile__ ("hlt");

        LEAVE_FUNC();
		return 0;
	}

	bInPageFaultHandler = TRUE;

    // when we come here from DebuggerShell() we live on a different stack
    // so the current task is different as well
    tsk = IoGetCurrentProcess();

    DPRINT((0,"%.8X (%.4X:%.8X %.8X %s %s %s task=%.8X )\n",
        address,
        ptr->cs,
        ptr->eip,
        ptr->eflags,
        (ptr->error_code&1)?"PLP":"NP",
        (ptr->error_code&2)?"WRITE":"READ",
        (ptr->error_code&4)?"USER-MODE":"KERNEL-MODE",
        (ULONG)tsk));

	if(!bInPrintk)
    {
    	DPRINT((0,"HandleInDebuggerFault(): unexpected pagefault in command handler!\n",address));
    }
	else
    {
    	DPRINT((0,"HandleInDebuggerFault(): unexpected pagefault in command handler while in PrintkCallback()!\n",address));
    }

    if(tsk)
    {
	    PULONG pPGD;
	    PULONG pPTE;

        pPGD = ADDR_TO_PDE(address);

        DPRINT((0,"PGD for %.8X @ %.8X = %.8X\n",address,(ULONG)pPGD,(ULONG)(*pPGD) ));

        if(pPGD && (*pPGD)&_PAGE_PRESENT)
        {
            // not large page
            if(!((*pPGD)&_PAGE_4M))
            {
                pPTE = ADDR_TO_PTE(address);
                if(pPTE)
                {
                    DPRINT((0,"PTE for %.8X @ %.8X = %.8X\n",address,(ULONG)pPTE,(ULONG)(*pPTE) ));
                }
            }
        }
    }

    IntelStackWalk(ptr->eip,CurrentEBP,ulRealStackPtr);

    DPRINT((0,"!!! machine is halted !!!\n"));

    __asm__ __volatile__ ("hlt");

    LEAVE_FUNC();

	return 2;
}

//*************************************************************************
// HandlePageFault()
//
// returns:
// 0    =       let the system handle it
// 1    =       call DebuggerShell()
// 2    =       FATAL error inside debugger
//*************************************************************************
ULONG HandlePageFault(FRAME* ptr)
{
    PVOID address;
	PEPROCESS tsk;
	PMADDRESS_SPACE vma;
    PLIST_ENTRY current_entry;
	MEMORY_AREA* current;

    // get linear address of page fault
	__asm__("movl %%cr2,%0":"=r" (address));

    // current process
    tsk = IoGetCurrentProcess();

    // there's something terribly wrong if we get a fault in our command handler
    if(bInDebuggerShell)
    {
		return HandleInDebuggerFault(ptr,(ULONG)address);
    }

    // remember error code so we can push it back on the stack
    error_code = ptr->error_code;

    // interrupt handlers can't have page faults
/*
    if(in_interrupt())
    {
        Print(OUTPUT_WINDOW,"pICE: system is currently processing an interrupt!\n");
        return 1;
    }
*/
    // lookup VMA for this address
	vma = &(tsk->AddressSpace);
	current_entry = vma->MAreaListHead.Flink;
	while(current_entry != &vma->MAreaListHead)
	{
		current = CONTAINING_RECORD(current_entry,
						MEMORY_AREA,
						Entry);
		if( (address >= current->BaseAddress) && (address <= current->BaseAddress + current->Length ))
		{
	        if(error_code & 2)
	        {
	            // area was not writable
	            if(!(current->Attributes & PAGE_READONLY))
	            {
	                Print(OUTPUT_WINDOW,"pICE: virtual memory arena is not writeable!\n");
	                return 1;
	            }
	        }
	        // READ ACCESS
	        else
	        {
	            // test EXT bit in error code
			    if (error_code & 1)
	            {
	                Print(OUTPUT_WINDOW,"pICE: page-level protection fault!\n");
	                return 1;
	            }
	            //
			    if (!(current->Attributes & PAGE_EXECUTE_READ))
	            {
	                Print(OUTPUT_WINDOW,"pICE: VMA is not readable!\n");
	                return 1;
	            }
	        }
	        // let the system handle it
	        return 0;
		}
		current_entry = current_entry->Flink;
	}

    Print(OUTPUT_WINDOW,"pICE: no virtual memory arena at this address!\n");
    return 0;

    // let the system handle it
//    return 0;
}

//*************************************************************************
// NewIntEHandler()
//
//*************************************************************************
__asm__ ("
NewIntEHandler:
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
        call _HandlePageFault
        addl $4,%esp

        cmpl $0,%eax
        je call_old_inte_handler

        cmpl $2,%eax
        je call_handler_unknown_reason

	    popl %ds
        popal
		popfl
        // remove error code. will be restored later when we call
        // original handler again.
        addl $4,%esp
		// call debugger loop
        pushl $" STR(REASON_PAGEFAULT) "
		jmp NewInt31Handler

call_old_inte_handler:
	    popl %ds
        popal
		popfl
		// chain to old handler
		.byte 0x2e
		jmp *_OldIntEHandler

call_handler_unknown_reason:
	    popl %ds
        popal
		popfl
        // remove error code. will be restored later when we call
        // original handler again.
        addl $4,%esp
		// call debugger loop
        pushl $" STR(REASON_INTERNAL_ERROR) "
		jmp NewInt31Handler
        ");


//*************************************************************************
// InstallIntEHook()
//
//*************************************************************************
void InstallIntEHook(void)
{
	ULONG LocalIntEHandler;

	ENTER_FUNC();

	MaskIrqs();
	if(!OldIntEHandler)
	{
		__asm__("mov $NewIntEHandler,%0"
			:"=r" (LocalIntEHandler)
			:
			:"eax");
		OldIntEHandler=SetGlobalInt(0x0E,(ULONG)LocalIntEHandler);
	}
	UnmaskIrqs();

    LEAVE_FUNC();
}

//*************************************************************************
// DeInstallIntEHook()
//
//*************************************************************************
void DeInstallIntEHook(void)
{
	ENTER_FUNC();

	MaskIrqs();
	if(OldIntEHandler)
	{
		SetGlobalInt(0x0E,(ULONG)OldIntEHandler);
        OldIntEHandler=0;
	}
	UnmaskIrqs();

    LEAVE_FUNC();
}
