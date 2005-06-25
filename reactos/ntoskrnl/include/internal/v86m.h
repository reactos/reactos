/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/internal/v86m.h
 * PURPOSE:         V86 Headers
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/
#ifndef __NTOSKRNL_INCLUDE_INTERNAL_V86M_H
#define __NTOSKRNL_INCLUDE_INTERNAL_V86M_H

/* Emulate cli/sti instructions */
#define KV86M_EMULATE_CLI_STI          (0x1)
/* Allow the v86 mode code to access i/o ports */
#define KV86M_ALLOW_IO_PORT_ACCESS      (0x2)

typedef struct _KV86M_REGISTERS
{
    /*
     * General purpose registers
     */
    ULONG Ebp;
    ULONG Edi;
    ULONG Esi;
    ULONG Edx;
    ULONG Ecx;
    ULONG Ebx;
    ULONG Eax;
    ULONG Ds;
    ULONG Es;
    ULONG Fs;
    ULONG Gs;

    /*
     * Control registers
     */
    ULONG Eip;
    ULONG Cs;
    ULONG Eflags;
    ULONG Esp;
    ULONG Ss;

    /*
     * Control structures
     */
    ULONG RecoveryAddress;
    UCHAR RecoveryInstruction[4];
    ULONG Vif;
    ULONG Flags;
    PNTSTATUS PStatus;
} KV86M_REGISTERS, *PKV86M_REGISTERS;

typedef struct _KV86M_TRAP_FRAME
{
    KTRAP_FRAME Tf;

    ULONG SavedExceptionStack;

    /*
     * These are put on the top of the stack by the routine that entered
     * v86 mode so the exception handlers can find the control information
     */
    struct _KV86M_REGISTERS* regs;
    ULONG orig_ebp;
} KV86M_TRAP_FRAME, *PKV86M_TRAP_FRAME;

NTSTATUS 
STDCALL
Ke386CallBios(
    UCHAR Int, 
    PKV86M_REGISTERS Regs
);

ULONG
KeV86Exception(
    ULONG ExceptionNr, 
    PKTRAP_FRAME Tf, 
    ULONG address
);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_V86M_H */
