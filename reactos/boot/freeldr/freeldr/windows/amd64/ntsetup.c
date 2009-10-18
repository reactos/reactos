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

// Last step before going virtual
void WinLdrSetupForNt(PLOADER_PARAMETER_BLOCK LoaderBlock,
                      PVOID *GdtIdt,
                      ULONG *PcrBasePage,
                      ULONG *TssBasePage)
{

}
