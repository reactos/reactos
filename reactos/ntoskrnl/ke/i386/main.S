#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/i386/ke.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>
#include <internal/i386/mm.h>
#include <internal/i386/fpu.h>

.globl _NtProcessStartup

_NtProcessStartup:

    /* FIXME: Application processors should have their own GDT/IDT */
    lgdt _KiGdtDescriptor
    lidt _KiIdtDescriptor

    /* Load the PCR selector */
    movl $PCR_SELECTOR, %eax
    movl %eax, %fs

    /* Load the initial kernel stack */
    lea _kernel_stack, %eax
    add $0x1000, %eax
    and $0xFFFFE000, %eax
    add $(0x3000 - SIZEOF_FX_SAVE_AREA), %eax
    movl %eax, %esp
    
    /* Call the main kernel initialization */
    pushl %edx
    pushl %ecx
    call __main
