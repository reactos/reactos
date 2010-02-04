
// create stubs
_NOWARN_PUSH
_NOWARN_MSC(4005)

// 00 DE divide error
#define TRAP_STUB_NAME KiTrap00
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 01 DB reserved
#define TRAP_STUB_NAME KiTrap01
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 02 NMI
#define TRAP_STUB_NAME KiTrap02
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 03 BP brealpoint
#define TRAP_STUB_NAME KiTrap03
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 04 OF overflow
#define TRAP_STUB_NAME KiTrap04
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 05 BR bound range exceeded
#define TRAP_STUB_NAME KiTrap05
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 06 UD undefined opcode
#define TRAP_STUB_NAME KiTrap06
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 07 NM no math coprocessor
#define TRAP_STUB_NAME KiTrap07
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 08 DF double fault
#define TRAP_STUB_NAME KiTrap08
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 09 math segment overrun
#define TRAP_STUB_NAME KiTrap09
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 0A TS invalid TSS 
#define TRAP_STUB_NAME KiTrap0A
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 0B NP segment not present
#define TRAP_STUB_NAME KiTrap0B
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 0C SS stack segment fault 
#define TRAP_STUB_NAME KiTrap0C
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 0D GP general protection 
#define TRAP_STUB_NAME KiTrap0D
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 0E PF page fault
#define TRAP_STUB_NAME KiTrap0E
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 0F reserved
#define TRAP_STUB_NAME KiTrap0F
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 10 MF math fault
#define TRAP_STUB_NAME KiTrap10
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 11 AC alignment check
#define TRAP_STUB_NAME KiTrap11
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 12 MC machine check
#define TRAP_STUB_NAME KiTrap12
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 13 XM simd exception
#define TRAP_STUB_NAME KiTrap13
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

#define TRAP_STUB_NAME KiGetTickCount
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

#define TRAP_STUB_NAME KiCallbackReturn
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

#define TRAP_STUB_NAME KiRaiseAssertion
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

#define TRAP_STUB_NAME KiDebugServiceVOID
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

#define TRAP_STUB_NAME KiSystemService
#define TRAP_STUB_FLAGS TRAPF_NOSAVESEG
#include <TrapStub.h>

// 
#define TRAP_STUB_NAME KiFastCallEntry
#define TRAP_STUB_FLAGS TRAPF_FASTSYSCALL
#include <TrapStub.h>

#define TRAP_STUB_NAME KiInterruptTemplate
#define TRAP_STUB_FLAGS TRAPF_VECTOR
#include <TrapStub.h>

_NOWARN_POP

/* TRAP EXIT CODE *************************************************************/
//
// Generic Exit Routine
//

VOID
FASTCALL
KiExitTrap(IN PKTRAP_FRAME TrapFrame, IN UCHAR Skip)
{
    KTRAP_EXIT_SKIP_BITS SkipBits;
    PULONG ReturnStack;
    
	SkipBits.Bits = Skip;

    /* Debugging checks */
	// DPRINTT("DebugChecks\n");
	// KiExitTrapDebugChecks(TrapFrame, SkipBits);

    /* Restore the SEH handler chain */
    KeGetPcr()->Tib.ExceptionList = TrapFrame->ExceptionList;
    
    /* Check if the previous mode must be restored */
    if (__builtin_expect(!SkipBits.SkipPreviousMode, 0)) /* More INTS than SYSCALLs */
    {
        /* Restore it */
        KeGetCurrentThread()->PreviousMode = TrapFrame->PreviousPreviousMode;
    }

    /* Check if there are active debug registers */
    if (__builtin_expect(TrapFrame->Dr7 & ~DR7_RESERVED_MASK, 0))
    {
        /* Not handled yet */
        DbgPrint("Need Hardware Breakpoint Support!\n");
        DbgBreakPoint();
        while (TRUE);
    }
    
    /* Check if this was a V8086 trap */
	// DPRINTT("V8086\n");
	if (__builtin_expect(TrapFrame->EFlags & EFLAGS_V86_MASK, 0))
	{
		// DPRINTT("V8086 r\n");
		// KiTrapReturn(TrapFrame);
		return;
	}

    /* Check if the trap frame was edited */
    if (__builtin_expect(!(TrapFrame->SegCs & FRAME_EDITED), 0))
    {   
        /*
         * An edited trap frame happens when we need to modify CS and/or ESP but
         * don't actually have a ring transition. This happens when a kernelmode
         * caller wants to perform an NtContinue to another kernel address, such
         * as in the case of SEH (basically, a longjmp), or to a user address.
         *
         * Therefore, the CPU never saved CS/ESP on the stack because we did not
         * get a trap frame due to a ring transition (there was no interrupt).
         * Even if we didn't want to restore CS to a new value, a problem occurs
         * due to the fact a normal RET would not work if we restored ESP since
         * RET would then try to read the result off the stack.
         *
         * The NT kernel solves this by adding 12 bytes of stack to the exiting
         * trap frame, in which EFLAGS, CS, and EIP are stored, and then saving
         * the ESP that's being requested into the ErrorCode field. It will then
         * exit with an IRET. This fixes both issues, because it gives the stack
         * some space where to hold the return address and then end up with the
         * wanted stack, and it uses IRET which allows a new CS to be inputted.
         *
         */
        // DPRINTT("edited\n");
        /* Set CS that is requested */
        TrapFrame->SegCs = TrapFrame->TempSegCs;
         
        /* First make space on requested stack */
        ReturnStack = (PULONG)(TrapFrame->TempEsp - 12);
        TrapFrame->ErrCode = (ULONG_PTR)ReturnStack;
         
        /* Now copy IRET frame */
        ReturnStack[0] = TrapFrame->Eip;
        ReturnStack[1] = TrapFrame->SegCs;
        ReturnStack[2] = TrapFrame->EFlags;
        
        /* Do special edited return */
		// DPRINTT("KiEditedTrapReturn\n");
        // KiEditedTrapReturn(TrapFrame);
		return;
    }
    
    /* Check if this is a user trap */
    if (__builtin_expect(KiUserTrap(TrapFrame), 1)) /* Ring 3 is where we spend time */
    {
        // DPRINTT("user\n");
		/* Check if segments should be restored */
        if (!SkipBits.SkipSegments)
        {
            /* Restore segments */
            CpuSetGs(TrapFrame->SegGs);
            CpuSetEs(TrapFrame->SegEs);
            CpuSetDs(TrapFrame->SegDs);
            CpuSetFs(TrapFrame->SegFs);
        }
        
        /* Always restore FS since it goes from KPCR to TEB */
        CpuSetFs(TrapFrame->SegFs);
    }
    
    /* Check for system call -- a system call skips volatiles! */
    if (__builtin_expect(SkipBits.SkipVolatiles, 0)) /* More INTs than SYSCALLs */
    {
        // DPRINTT("syscall\n");
		/* Kernel call or user call? */
        if (__builtin_expect(KiUserTrap(TrapFrame), 1)) /* More Ring 3 than 0 */
        {
            /* Is SYSENTER supported and/or enabled, or are we stepping code? */
            if (__builtin_expect((KiFastSystemCallDisable) ||
                                 (TrapFrame->EFlags & EFLAGS_TF), 0))
            {
                /* Exit normally */
				// DPRINTT("normally KiSystemCallTrapReturn\n");
                KiSystemCallTrapReturn(TrapFrame);
            }
            else
            {
                /* Restore user FS */
                CpuSetFs(KGDT_R3_TEB | RPL_MASK);
                
                /* Remove interrupt flag */
                TrapFrame->EFlags &= ~EFLAGS_INTERRUPT_MASK;
                __writeeflags(TrapFrame->EFlags);
                
                /* Exit through SYSEXIT */
				// DPRINTT("sysexit KiSystemCallSysExitReturn\n");
                // KiSystemCallSysExitReturn(TrapFrame);
            }
        }
        else
        {
            /* Restore EFLags */
            __writeeflags(TrapFrame->EFlags);
            
            /* Call is kernel, so do a jump back since this wasn't a real INT */
			// DPRINTT("kernel KiSystemCallReturn\n");
            // KiSystemCallReturn(TrapFrame);
        }  
    }
    else
    {
        // DPRINTT("int KiTrapReturn\n"); 
		/* Return from interrupt */
        // KiTrapReturn(TrapFrame);
    }
}



VOID _FASTCALL
KiEoiHelper(IN PKTRAP_FRAME TrapFrame)
{
	// DPRINTT("\n");
	/* Disable interrupts until we return */
    CpuIntDisable();
    
    /* Check for APC delivery */
    KiCheckForApcDelivery(TrapFrame);
    
    /* Now exit the trap for real */
	// DPRINTT("KiExitTrap\n");
    KiExitTrap(TrapFrame, KTE_SKIP_PM_BIT);
	UNREACHABLE;
}



