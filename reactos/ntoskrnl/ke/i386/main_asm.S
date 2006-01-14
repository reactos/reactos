#include <roscfg.h>
#include <ndk/asm.h>

#define AP_MAGIC (0x12481020)

.globl _NtProcessStartup

_NtProcessStartup:

    /* FIXME: Application processors should have their own GDT/IDT */
    lgdt _KiGdtDescriptor
    lidt _KiIdtDescriptor

    /* Load the PCR selector */
    movl $KGDT_R0_PCR, %eax
    movl %eax, %fs
    
    cmpl $AP_MAGIC, %ecx
    jne  .m1
     
    pushl $0
    popfl
	
    /*
     *  Reserve space for the floating point save area.
     */
    subl  $SIZEOF_FX_SAVE_AREA, %esp

    /*
     * Call the application processor initialization code
     */
    pushl $0
    call  _KiSystemStartup
    
.m1:     
    /* Load the initial kernel stack */
    lea _kernel_stack, %eax
    add $0x2000, %eax
    and $0xFFFFE000, %eax
    add $(0x3000 - SIZEOF_FX_SAVE_AREA), %eax
    movl %eax, %esp
    
    /* Call the main kernel initialization */
    pushl %edx
    pushl %ecx
    call __main
