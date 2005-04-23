/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386s/usercall.S
 * PURPOSE:         User-Mode callbacks and return.
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <roscfg.h>
#include <internal/i386/segment.h>
#include <internal/i386/ke.h>
#include <internal/i386/fpu.h>
#include <internal/ps.h>
#include <ntos/tss.h>
#include <internal/ntoskrnl.h>
.intel_syntax noprefix

/* GLOBALS ****************************************************************/
.extern PVOID _SystemDllCallbackDispatcher

#define CBSTACK_BUFFER_ADDRESS 0x20
#define CBSTACK_BUFFER_LENGTH  0x24
/* FUNCTIONS ****************************************************************/

/*++
 * KiSwitchToUserMode 
 *
 *     The KiSwitchToUserMode routine sets up a Trap Frame and a Callback stack
 *     for the purpose of switching to user mode. The actual final jump is done
 *     by KiServiceExit which will treat this as a syscall return.
 *
 * Params:
 *     OutputBuffer - Pointer to a caller-allocated buffer where to receive 
 *                    the return data from the user-mode function
 *
 *     OutputLength - Size of the Output Buffer described above.
 *
 * Returns:
 *     Jumps into KiServiceExit.
 *
 * Remarks:
 *     If there is not enough Kernel Stack space, the routine will increase the
 *     Kernel Stack.
 *
 *     User mode execution resumes at ntdll!KiUserCallbackDispatcher.
 *
 *     This call MUST be paired by interrupt 0x2B or NtCallbackReturn.
 *
 *--*/
.globl _KiSwitchToUserMode@8
.func KiSwitchToUserMode@8
_KiSwitchToUserMode@8:

.endfunc


