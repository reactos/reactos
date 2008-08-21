/*
 * FILE:            ntoskrnl/ke/amd64/trap.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 * NOTE:            See asmmacro.S for the shared entry/exit code.
 */

/* INCLUDES ******************************************************************/

//#include <asm.h>
//#include <internal/amd64/asmmacro.S>
.intel_syntax noprefix
.code64

#define KERNEL_STACK_SIZE 0x6000

/* GLOBALS *******************************************************************/

.bss
.align 16

/* Kernel Boot Stack */
.globl _P0BootStack
.space KERNEL_STACK_SIZE
_P0BootStack:

/* Kernel Double-Fault and Temporary DPC Stack */
.globl _KiDoubleFaultStack
.space KERNEL_STACK_SIZE
_KiDoubleFaultStack:

.data

_MsgGeneralProtFault:
.ascii "General protection fault at %p!\n\0"

_MsgBreakpointTrap:
.ascii "BreakpointTrap at %p\n\0"

_MsgUnexpectedInterrupt:
.ascii "UnexpectedInterrupt\n\0"


/* SOFTWARE INTERRUPT SERVICES ***********************************************/
.text
.code64

.global _KiDivideErrorFault
_KiDivideErrorFault:

.global _KiDebugTrapOrFault
_KiDebugTrapOrFault:

.global _KiNmiInterrupt
_KiNmiInterrupt:

.global _KiBreakpointTrap
_KiBreakpointTrap:
//    mov rdx, [rsp]
    sub rsp, 0x10
//    movabs rcx, offset _MsgBreakpointTrap
//    movabs rax, offset _FrLdrDbgPrint
//    call [rax]
    add rsp, 0x10
    iret

.global _KiOverflowTrap
_KiOverflowTrap:

.global _KiBoundFault
_KiBoundFault:

.global _KiInvalidOpcodeFault
_KiInvalidOpcodeFault:

.global _KiNpxNotAvailableFault
_KiNpxNotAvailableFault:

.global _KiDoubleFaultAbort
_KiDoubleFaultAbort:

.global _KiNpxSegmentOverrunAbort
_KiNpxSegmentOverrunAbort:

.global _KiInvalidTssFault
_KiInvalidTssFault:

.global _KiSegmentNotPresentFault
_KiSegmentNotPresentFault:

.global _KiStackFault
_KiStackFault:

jmp $


.global _KiGeneralProtectionFault
_KiGeneralProtectionFault:
    mov rdx, 0
    mov dx, ss
    movabs rcx, offset _MsgGeneralProtFault
    movabs rax, offset _FrLdrDbgPrint
    call [rax]
    jmp $


.global _KiPageFault
_KiPageFault:

.global _KiFloatingErrorFault
_KiFloatingErrorFault:

.global _KiAlignmentFault
_KiAlignmentFault:

.global _KiMcheckAbort
_KiMcheckAbort:

.global _KiXmmException
_KiXmmException:

.global _KiApcInterrupt
_KiApcInterrupt:


.global _KiRaiseAssertion
_KiRaiseAssertion:

.global _KiDebugServiceTrap
_KiDebugServiceTrap:

.global _KiDpcInterrupt
_KiDpcInterrupt:


.global _KiIpiInterrupt
_KiIpiInterrupt:
    iret

.global _KiUnexpectedInterrupt
_KiUnexpectedInterrupt:
    movabs rcx, offset _MsgUnexpectedInterrupt
    movabs rax, offset _FrLdrDbgPrint
    call [rax]
    jmp $
