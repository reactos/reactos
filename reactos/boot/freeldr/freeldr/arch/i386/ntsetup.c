/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            freeldr/windows/i386/ntsetup.c
 * PURPOSE:         i386-specific setup for Windows boot
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>
#include <debug.h>

// this is needed for new IDT filling
#if 0
extern ULONG_PTR i386DivideByZero;
extern ULONG_PTR i386DebugException;
extern ULONG_PTR i386NMIException;
extern ULONG_PTR i386Breakpoint;
extern ULONG_PTR i386Overflow;
extern ULONG_PTR i386BoundException;
extern ULONG_PTR i386InvalidOpcode;
extern ULONG_PTR i386FPUNotAvailable;
extern ULONG_PTR i386DoubleFault;
extern ULONG_PTR i386CoprocessorSegment;
extern ULONG_PTR i386InvalidTSS;
extern ULONG_PTR i386SegmentNotPresent;
extern ULONG_PTR i386StackException;
extern ULONG_PTR i386GeneralProtectionFault;
extern ULONG_PTR i386PageFault; // exc 14
extern ULONG_PTR i386CoprocessorError; // exc 16
extern ULONG_PTR i386AlignmentCheck; // exc 17
#endif

/* FUNCTIONS **************************************************************/
#if 0
// Last step before going virtual
void WinLdrSetupForNt(PLOADER_PARAMETER_BLOCK LoaderBlock,
                      PVOID *GdtIdt,
                      ULONG *PcrBasePage,
                      ULONG *TssBasePage)
{
	ULONG TssSize;
	//ULONG TssPages;
	ULONG_PTR Pcr = 0;
	ULONG_PTR Tss = 0;
	ULONG BlockSize, NumPages;

	LoaderBlock->u.I386.CommonDataArea = NULL; // Force No ABIOS support
	LoaderBlock->u.I386.MachineType = MACHINE_TYPE_ISA;

	/* Allocate 2 pages for PCR */
	Pcr = (ULONG_PTR)MmAllocateMemoryWithType(2 * MM_PAGE_SIZE, LoaderStartupPcrPage);
	*PcrBasePage = Pcr >> MM_PAGE_SHIFT;

	if (Pcr == 0)
	{
		UiMessageBox("Can't allocate PCR\n");
		return;
	}

	/* Allocate TSS */
	TssSize = (sizeof(KTSS) + MM_PAGE_SIZE) & ~(MM_PAGE_SIZE - 1);
	//TssPages = TssSize / MM_PAGE_SIZE;

	Tss = (ULONG_PTR)MmAllocateMemoryWithType(TssSize, LoaderMemoryData);

	*TssBasePage = Tss >> MM_PAGE_SHIFT;

	/* Allocate space for new GDT + IDT */
	BlockSize = NUM_GDT*sizeof(KGDTENTRY) + NUM_IDT*sizeof(KIDTENTRY);//FIXME: Use GDT/IDT limits here?
	NumPages = (BlockSize + MM_PAGE_SIZE - 1) >> MM_PAGE_SHIFT;
	*GdtIdt = (PKGDTENTRY)MmAllocateMemoryWithType(NumPages * MM_PAGE_SIZE, LoaderMemoryData);

	if (*GdtIdt == NULL)
	{
		UiMessageBox("Can't allocate pages for GDT+IDT!\n");
		return;
	}

	/* Zero newly prepared GDT+IDT */
	RtlZeroMemory(*GdtIdt, NumPages << MM_PAGE_SHIFT);
}
#endif
