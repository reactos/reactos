/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/amd64/cpu.c
 * PURPOSE:         Routines for CPU-level support
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* The Boot TSS */
KTSS64 KiBootTss;

/* The TSS to use for Double Fault Traps (INT 0x9) */
UCHAR KiDoubleFaultTSS[KTSS_IO_MAPS];

/* The TSS to use for NMI Fault Traps (INT 0x2) */
UCHAR KiNMITSS[KTSS_IO_MAPS];

/* CPU Features and Flags */
ULONG KeI386MachineType;

CHAR KeNumberProcessors = 0;

/* FUNCTIONS *****************************************************************/


VOID
FASTCALL
Ki386InitializeTss(IN PKTSS Tss,
                   IN PKIDTENTRY Idt,
                   IN PKGDTENTRY Gdt)
{
 //   UNIMPLEMENTED;
}

VOID
NTAPI
KeFlushCurrentTb(VOID)
{
    /* Flush the TLB by resetting CR3 */
    __writecr3(__readcr3());
}

VOID
NTAPI
KiInitializeMachineType(VOID)
{
    /* Set the Machine Type we got from NTLDR */
    KeI386MachineType = KeLoaderBlock->u.I386.MachineType & 0x000FF;
}


