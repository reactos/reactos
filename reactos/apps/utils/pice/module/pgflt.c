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
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/interrupt.h>

////////////////////////////////////////////////////
// GLOBALS
////

char tempPageFault[1024];

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
	struct task_struct *tsk;
	struct mm_struct *mm;
	struct mm_struct *p = NULL;

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
    tsk = (struct task_struct *)(0xFFFFE000 & ulRealStackPtr);
    mm = tsk->mm;

    DPRINT((0,"%.8X (%.4X:%.8X %.8X %s %s %s task=%.8X mm=%.8X)\n",
        address,
        ptr->cs,
        ptr->eip,
        ptr->eflags,
        (ptr->error_code&1)?"PLP":"NP",
        (ptr->error_code&2)?"WRITE":"READ",
        (ptr->error_code&4)?"USER-MODE":"KERNEL-MODE",
        (ULONG)tsk,
        (ULONG)mm));

	if(!bInPrintk)
    {
    	DPRINT((0,"HandleInDebuggerFault(): unexpected pagefault in command handler!\n",address));
    }
	else
    {
    	DPRINT((0,"HandleInDebuggerFault(): unexpected pagefault in command handler while in PrintkCallback()!\n",address));
    }


    if(address < TASK_SIZE)
    {
        p = mm;
    }
    else
    {
        p = my_init_mm;
    }
    
    if(p)
    {
	    pgd_t * pPGD;
	    pmd_t * pPMD;
	    pte_t * pPTE;

        pPGD = pgd_offset(p,address);

        DPRINT((0,"PGD for %.8X @ %.8X = %.8X\n",address,(ULONG)pPGD,(ULONG)pgd_val(*pPGD) ));

        if(pPGD && pgd_val(*pPGD)&_PAGE_PRESENT)
        {
            // not large page
            if(!(pgd_val(*pPGD)&_PAGE_4M))
            {
                pPMD = pmd_offset(pPGD,address);
                if(pPMD)
                {
                    pPTE = pte_offset(pPMD,address);
                    if(pPTE)
                    {
                        DPRINT((0,"PTE for %.8X @ %.8X = %.8X\n",address,(ULONG)pPTE,(ULONG)pte_val(*pPTE) ));
                    }
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
    ULONG address;
	struct task_struct *tsk;
	struct mm_struct *mm;
	struct vm_area_struct * vma;

    // get linear address of page fault
	__asm__("movl %%cr2,%0":"=r" (address));

    // current process
    tsk = current;

    // there's something terribly wrong if we get a fault in our command handler
    if(bInDebuggerShell)
    {
		return HandleInDebuggerFault(ptr,address);
    }

    // remember error code so we can push it back on the stack
    error_code = ptr->error_code;

    //////////////////////////////////////
    // kernel page fault 

    // since LINUX kernel is not pageable this is death
    // so call handler
    if(address >= TASK_SIZE)
    {
        // 
        if(error_code & 4)
        {
            PICE_sprintf(tempPageFault,"pICE: kernel page fault from user-mode code (error code %x)!\n",error_code);
            Print(OUTPUT_WINDOW,tempPageFault);
        }
        else
        {
            PICE_sprintf(tempPageFault,"pICE: kernel page fault from kernel-mode code (error code %x)!\n",error_code);
            Print(OUTPUT_WINDOW,tempPageFault);
        }
        return 1;
    }

    // and it's memory environment
	mm = tsk->mm;

    //////////////////////////////////////
    // user page fault
    // fault address is below TASK_SIZE

    // no user context, i.e. no pages below TASK_SIZE are mapped
    if(mm == my_init_mm)
    {
        Print(OUTPUT_WINDOW,"pICE: there's no user context!\n");
        return 1;
    }

    // interrupt handlers can't have page faults
    if(in_interrupt())
    {
        Print(OUTPUT_WINDOW,"pICE: system is currently processing an interrupt!\n");
        return 1;
    }

    // lookup VMA for this address
    vma = find_vma(mm, address);
    if(!vma)
    {
        Print(OUTPUT_WINDOW,"pICE: no virtual memory arena at this address!\n");
        return 1;
    }

    // address is greater than the start of this VMA
	if (address >= vma->vm_start)
    {
        // WRITE ACCESS
        // write bit set in error_code
        if(error_code & 2)
        {
            // area was not writable
            if(!(vma->vm_flags & VM_WRITE))
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
		    if (!(vma->vm_flags & (VM_READ | VM_EXEC)))
            {
                Print(OUTPUT_WINDOW,"pICE: VMA is not readable!\n");
                return 1;
            }
        }
        // let the system handle it
        return 0;
    }

    // 
	if (!(vma->vm_flags & VM_GROWSDOWN))
    {
        Print(OUTPUT_WINDOW,"pICE: virtual memory arena doesn't grow down!\n");
        return 1;
    }

    // let the system handle it
    return 0;
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
        call HandlePageFault
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
		jmp *OldIntEHandler

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