/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/exp.c
 * PURPOSE:         Handling exceptions
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 *                  Skywing (skywing@valhallalegends.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#define FLAG_IF (1<<9)

#define _STR(x) #x
#define STR(x) _STR(x)

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(x) (sizeof (x) / sizeof (x[0]))
#endif

extern void KiSystemService(void);
extern void KiDebugService(void);

extern VOID KiTrap0(VOID);
extern VOID KiTrap1(VOID);
extern VOID KiTrap2(VOID);
extern VOID KiTrap3(VOID);
extern VOID KiTrap4(VOID);
extern VOID KiTrap5(VOID);
extern VOID KiTrap6(VOID);
extern VOID KiTrap7(VOID);
extern VOID KiTrap8(VOID);
extern VOID KiTrap9(VOID);
extern VOID KiTrap10(VOID);
extern VOID KiTrap11(VOID);
extern VOID KiTrap12(VOID);
extern VOID KiTrap13(VOID);
extern VOID KiTrap14(VOID);
extern VOID KiTrap15(VOID);
extern VOID KiTrap16(VOID);
extern VOID KiTrap17(VOID);
extern VOID KiTrap18(VOID);
extern VOID KiTrap19(VOID);
extern VOID KiTrapUnknown(VOID);

extern ULONG init_stack;
extern ULONG init_stack_top;

extern BOOLEAN Ke386NoExecute;

static char *ExceptionTypeStrings[] =
  {
    "Divide Error",
    "Debug Trap",
    "NMI",
    "Breakpoint",
    "Overflow",
    "BOUND range exceeded",
    "Invalid Opcode",
    "No Math Coprocessor",
    "Double Fault",
    "Unknown(9)",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection",
    "Page Fault",
    "Reserved(15)",
    "Math Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Fault"
  };

NTSTATUS ExceptionToNtStatus[] =
  {
    STATUS_INTEGER_DIVIDE_BY_ZERO,
    STATUS_SINGLE_STEP,
    STATUS_ACCESS_VIOLATION,
    STATUS_BREAKPOINT,
    STATUS_INTEGER_OVERFLOW,
    STATUS_ARRAY_BOUNDS_EXCEEDED,
    STATUS_ILLEGAL_INSTRUCTION,
    STATUS_FLOAT_INVALID_OPERATION,
    STATUS_ACCESS_VIOLATION,
    STATUS_ACCESS_VIOLATION,
    STATUS_ACCESS_VIOLATION,
    STATUS_ACCESS_VIOLATION,
    STATUS_STACK_OVERFLOW,
    STATUS_ACCESS_VIOLATION,
    STATUS_ACCESS_VIOLATION,
    STATUS_ACCESS_VIOLATION, /* RESERVED */
    STATUS_FLOAT_INVALID_OPERATION, /* Should not be used, the FPU can give more specific info */
    STATUS_DATATYPE_MISALIGNMENT,
    STATUS_ACCESS_VIOLATION,
    STATUS_FLOAT_MULTIPLE_TRAPS,
  };

/* FUNCTIONS ****************************************************************/

BOOLEAN STDCALL
KiRosPrintAddress(PVOID address)
{
   PLIST_ENTRY current_entry;
   MODULE_TEXT_SECTION* current;
   extern LIST_ENTRY ModuleTextListHead;
   ULONG_PTR RelativeAddress;
   ULONG i = 0;

   do
   {
     current_entry = ModuleTextListHead.Flink;

     while (current_entry != &ModuleTextListHead &&
            current_entry != NULL)
       {
          current =
            CONTAINING_RECORD(current_entry, MODULE_TEXT_SECTION, ListEntry);

          if (address >= (PVOID)current->Base &&
              address < (PVOID)(current->Base + current->Length))
            {
              RelativeAddress = (ULONG_PTR) address - current->Base;
              DbgPrint("<%ws: %x>", current->Name, RelativeAddress);
              return(TRUE);
            }
          current_entry = current_entry->Flink;
       }

     address = (PVOID)((ULONG_PTR)address & ~0xC0000000);
   } while(++i <= 1);

   return(FALSE);
}

ULONG
KiKernelTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr, PVOID Cr2)
{
  EXCEPTION_RECORD Er;

  Er.ExceptionFlags = 0;
  Er.ExceptionRecord = NULL;
  Er.ExceptionAddress = (PVOID)Tf->Eip;

  if (ExceptionNr == 14)
    {
      Er.ExceptionCode = STATUS_ACCESS_VIOLATION;
      Er.NumberParameters = 2;
      Er.ExceptionInformation[0] = Tf->ErrorCode & 0x1;
      Er.ExceptionInformation[1] = (ULONG)Cr2;
    }
  else
    {
      if (ExceptionNr < ARRAY_SIZE(ExceptionToNtStatus))
	{
	  Er.ExceptionCode = ExceptionToNtStatus[ExceptionNr];
	}
      else
	{
	  Er.ExceptionCode = STATUS_ACCESS_VIOLATION;
	}
      Er.NumberParameters = 0;
    }

  /* FIXME: Which exceptions are noncontinuable? */
  Er.ExceptionFlags = 0;

  KiDispatchException(&Er, 0, Tf, KernelMode, TRUE);

  return(0);
}

ULONG
KiDoubleFaultHandler(VOID)
{
  unsigned int cr2;
  ULONG StackLimit;
  ULONG StackBase;
  ULONG Esp0;
  ULONG ExceptionNr = 8;
  KTSS* OldTss;
  PULONG Frame;
  ULONG OldCr3;
#if 0
  ULONG i, j;
  static PVOID StackTrace[MM_STACK_SIZE / sizeof(PVOID)];
  static ULONG StackRepeatCount[MM_STACK_SIZE / sizeof(PVOID)];
  static ULONG StackRepeatLength[MM_STACK_SIZE / sizeof(PVOID)];
  ULONG TraceLength;
  BOOLEAN FoundRepeat;
#endif

  OldTss = KeGetCurrentKPCR()->TSS;
  Esp0 = OldTss->Esp;

  /* Get CR2 */
  cr2 = Ke386GetCr2();
  if (PsGetCurrentThread() != NULL &&
      PsGetCurrentThread()->ThreadsProcess != NULL)
    {
      OldCr3 = (ULONG)
	PsGetCurrentThread()->ThreadsProcess->Pcb.DirectoryTableBase.QuadPart;
    }
  else
    {
      OldCr3 = 0xBEADF0AL;
    }

   /*
    * Check for stack underflow
    */
   if (PsGetCurrentThread() != NULL &&
       Esp0 < (ULONG)PsGetCurrentThread()->Tcb.StackLimit)
     {
	DbgPrint("Stack underflow (tf->esp %x Limit %x)\n",
		 Esp0, (ULONG)PsGetCurrentThread()->Tcb.StackLimit);
	ExceptionNr = 12;
     }

   /*
    * Print out the CPU registers
    */
   if (ExceptionNr < ARRAY_SIZE(ExceptionTypeStrings))
     {
       DbgPrint("%s Exception: %d(%x)\n", ExceptionTypeStrings[ExceptionNr],
		ExceptionNr, 0);
     }
   else
     {
       DbgPrint("Exception: %d(%x)\n", ExceptionNr, 0);
     }
   DbgPrint("CS:EIP %x:%x ", OldTss->Cs, OldTss->Eip);
   KeRosPrintAddress((PVOID)OldTss->Eip);
   DbgPrint("\n");
   DbgPrint("cr2 %x cr3 %x ", cr2, OldCr3);
   DbgPrint("Proc: %x ",PsGetCurrentProcess());
   if (PsGetCurrentProcess() != NULL)
     {
	DbgPrint("Pid: %x <", PsGetCurrentProcess()->UniqueProcessId);
	DbgPrint("%.8s> ", PsGetCurrentProcess()->ImageFileName);
     }
   if (PsGetCurrentThread() != NULL)
     {
	DbgPrint("Thrd: %x Tid: %x",
		 PsGetCurrentThread(),
		 PsGetCurrentThread()->Cid.UniqueThread);
     }
   DbgPrint("\n");
   DbgPrint("DS %x ES %x FS %x GS %x\n", OldTss->Ds, OldTss->Es,
	    OldTss->Fs, OldTss->Gs);
   DbgPrint("EAX: %.8x   EBX: %.8x   ECX: %.8x\n", OldTss->Eax, OldTss->Ebx,
	    OldTss->Ecx);
   DbgPrint("EDX: %.8x   EBP: %.8x   ESI: %.8x\n   ESP: %.8x", OldTss->Edx,
	    OldTss->Ebp, OldTss->Esi, Esp0);
   DbgPrint("EDI: %.8x   EFLAGS: %.8x ", OldTss->Edi, OldTss->Eflags);
   if (OldTss->Cs == KERNEL_CS)
     {
	DbgPrint("kESP %.8x ", Esp0);
	if (PsGetCurrentThread() != NULL)
	  {
	     DbgPrint("kernel stack base %x\n",
		      PsGetCurrentThread()->Tcb.StackLimit);

	  }
     }
   else
     {
	DbgPrint("User ESP %.8x\n", OldTss->Esp);
     }
  if ((OldTss->Cs & 0xffff) == KERNEL_CS)
    {
      if (PsGetCurrentThread() != NULL)
	{
	  StackLimit = (ULONG)PsGetCurrentThread()->Tcb.StackBase;
	  StackBase = (ULONG)PsGetCurrentThread()->Tcb.StackLimit;
	}
      else
	{
	  StackLimit = (ULONG)init_stack_top;
	  StackBase = (ULONG)init_stack;
	}

      /*
	 Change to an #if 0 to reduce the amount of information printed on
	 a recursive stack trace.
      */
#if 1
      DbgPrint("Frames: ");
      Frame = (PULONG)OldTss->Ebp;
      while (Frame != NULL && (ULONG)Frame >= StackBase)
	{
	  KeRosPrintAddress((PVOID)Frame[1]);
	  Frame = (PULONG)Frame[0];
          DbgPrint("\n");
	}
#else
      DbgPrint("Frames: ");
      i = 0;
      Frame = (PULONG)OldTss->Ebp;
      while (Frame != NULL && (ULONG)Frame >= StackBase)
	{
	  StackTrace[i] = (PVOID)Frame[1];
	  Frame = (PULONG)Frame[0];
	  i++;
	}
      TraceLength = i;

      i = 0;
      while (i < TraceLength)
	{
	  StackRepeatCount[i] = 0;
	  j = i + 1;
	  FoundRepeat = FALSE;
	  while ((j - i) <= (TraceLength - j) && FoundRepeat == FALSE)
	    {
	      if (memcmp(&StackTrace[i], &StackTrace[j],
			 (j - i) * sizeof(PVOID)) == 0)
		{
		  StackRepeatCount[i] = 2;
		  StackRepeatLength[i] = j - i;
		  FoundRepeat = TRUE;
		}
	      else
		{
		  j++;
		}
	    }
	  if (FoundRepeat == FALSE)
	    {
	      i++;
	      continue;
	    }
	  j = j + StackRepeatLength[i];
	  while ((TraceLength - j) >= StackRepeatLength[i] &&
		 FoundRepeat == TRUE)
	    {
	      if (memcmp(&StackTrace[i], &StackTrace[j],
			 StackRepeatLength[i] * sizeof(PVOID)) == 0)
		{
		  StackRepeatCount[i]++;
		  j = j + StackRepeatLength[i];
		}
	      else
		{
		  FoundRepeat = FALSE;
		}
	    }
	  i = j;
	}

      i = 0;
      while (i < TraceLength)
	{
	  if (StackRepeatCount[i] == 0)
	    {
	      KeRosPrintAddress(StackTrace[i]);
	      i++;
	    }
	  else
	    {
	      DbgPrint("{");
	      if (StackRepeatLength[i] == 0)
		{
		  for(;;);
		}
	      for (j = 0; j < StackRepeatLength[i]; j++)
		{
		  KeRosPrintAddress(StackTrace[i + j]);
		}
	      DbgPrint("}*%d", StackRepeatCount[i]);
	      i = i + StackRepeatLength[i] * StackRepeatCount[i];
	    }
	}
#endif
    }

   DbgPrint("\n");
   for(;;);
   return 0;
}

VOID
KiDumpTrapFrame(PKTRAP_FRAME Tf, ULONG Parameter1, ULONG Parameter2)
{
  ULONG cr3_;
  ULONG StackLimit;
  ULONG Esp0;
  ULONG ExceptionNr = (ULONG)Tf->DebugArgMark;
  ULONG cr2 = (ULONG)Tf->DebugPointer;

  Esp0 = (ULONG)Tf;

   /*
    * Print out the CPU registers
    */
   if (ExceptionNr < ARRAY_SIZE(ExceptionTypeStrings))
     {
	DbgPrint("%s Exception: %d(%x)\n", ExceptionTypeStrings[ExceptionNr],
		 ExceptionNr, Tf->ErrorCode&0xffff);
     }
   else
     {
	DbgPrint("Exception: %d(%x)\n", ExceptionNr, Tf->ErrorCode&0xffff);
     }
   DbgPrint("Processor: %d CS:EIP %x:%x ", KeGetCurrentProcessorNumber(),
	    Tf->Cs&0xffff, Tf->Eip);
   KeRosPrintAddress((PVOID)Tf->Eip);
   DbgPrint("\n");
   Ke386GetPageTableDirectory(cr3_);
   DbgPrint("cr2 %x cr3 %x ", cr2, cr3_);
   DbgPrint("Proc: %x ",PsGetCurrentProcess());
   if (PsGetCurrentProcess() != NULL)
     {
	DbgPrint("Pid: %x <", PsGetCurrentProcess()->UniqueProcessId);
	DbgPrint("%.8s> ", PsGetCurrentProcess()->ImageFileName);
     }
   if (PsGetCurrentThread() != NULL)
     {
	DbgPrint("Thrd: %x Tid: %x",
		 PsGetCurrentThread(),
		 PsGetCurrentThread()->Cid.UniqueThread);
     }
   DbgPrint("\n");
   DbgPrint("DS %x ES %x FS %x GS %x\n", Tf->Ds&0xffff, Tf->Es&0xffff,
	    Tf->Fs&0xffff, Tf->Gs&0xfff);
   DbgPrint("EAX: %.8x   EBX: %.8x   ECX: %.8x\n", Tf->Eax, Tf->Ebx, Tf->Ecx);
   DbgPrint("EDX: %.8x   EBP: %.8x   ESI: %.8x   ESP: %.8x\n", Tf->Edx,
	    Tf->Ebp, Tf->Esi, Esp0);
   DbgPrint("EDI: %.8x   EFLAGS: %.8x ", Tf->Edi, Tf->Eflags);
   if ((Tf->Cs&0xffff) == KERNEL_CS)
     {
	DbgPrint("kESP %.8x ", Esp0);
	if (PsGetCurrentThread() != NULL)
	  {
	     DbgPrint("kernel stack base %x\n",
		      PsGetCurrentThread()->Tcb.StackLimit);

	  }
     }

   if (PsGetCurrentThread() != NULL)
     {
       StackLimit = (ULONG)PsGetCurrentThread()->Tcb.StackBase;
     }
   else
     {
       StackLimit = (ULONG)init_stack_top;
     }

   /*
    * Dump the stack frames
    */
   KeDumpStackFrames((PULONG)Tf->Ebp);
}

ULONG
KiTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr)
/*
 * FUNCTION: Called by the lowlevel execption handlers to print an amusing
 * message and halt the computer
 * ARGUMENTS:
 *        Complete CPU context
 */
{
   unsigned int cr2;
   NTSTATUS Status;
   ULONG Esp0;

   /* Store the exception number in an unused field in the trap frame. */
   Tf->DebugArgMark = (PVOID)ExceptionNr;

   /* Use the address of the trap frame as approximation to the ring0 esp */
   Esp0 = (ULONG)&Tf->Eip;

   /* Get CR2 */
   cr2 = Ke386GetCr2();
   Tf->DebugPointer = (PVOID)cr2;

   if (ExceptionNr == 14 && Tf->Eflags & FLAG_IF)
   {
     Ke386EnableInterrupts();
   }

   /*
    * If this was a V86 mode exception then handle it specially
    */
   if (Tf->Eflags & (1 << 17))
     {
       DPRINT("Tf->Eflags, %x, Tf->Eip %x, ExceptionNr: %d\n", Tf->Eflags, Tf->Eip, ExceptionNr);
       return(KeV86Exception(ExceptionNr, Tf, cr2));
     }

   /*
    * Check for stack underflow, this may be obsolete
    */
   if (PsGetCurrentThread() != NULL &&
       Esp0 < (ULONG)PsGetCurrentThread()->Tcb.StackLimit)
     {
	DPRINT1("Stack underflow (tf->esp %x Limit %x)\n",
		 Esp0, (ULONG)PsGetCurrentThread()->Tcb.StackLimit);
	ExceptionNr = 12;
     }

   if (ExceptionNr == 15)
     {
       /*
        * FIXME:
        *   This exception should never occur. The P6 has a bug, which does sometimes deliver
        *   the apic spurious interrupt as exception 15. On an athlon64, I get one exception
        *   in the early boot phase in apic mode (using the smp build). I've looked to the linux
        *   sources. Linux does ignore this exception.
        *
        *   Hartmut Birr
        */
       DPRINT1("Ignoring P6 Local APIC Spurious Interrupt Bug...\n");
       return(0);
     }

   /*
    * Maybe handle the page fault and return
    */
   if (ExceptionNr == 14)
     {
        if (Ke386NoExecute && Tf->ErrorCode & 0x10 && cr2 >= KERNEL_BASE)
	{
           KEBUGCHECKWITHTF(ATTEMPTED_EXECUTE_OF_NOEXECUTE_MEMORY, 0, 0, 0, 0, Tf);
	}
	Status = MmPageFault(Tf->Cs&0xffff,
			     &Tf->Eip,
			     &Tf->Eax,
			     cr2,
			     Tf->ErrorCode);
	if (NT_SUCCESS(Status))
	  {
	     return(0);
	  }
     }

   /*
    * Check for a breakpoint that was only for the attention of the debugger.
    */
   if (ExceptionNr == 3 && Tf->Eip == ((ULONG)DbgBreakPointNoBugCheck) + 1)
     {
       /*
	  EIP is already adjusted by the processor to point to the instruction
	  after the breakpoint.
       */
       return(0);
     }

   /*
    * Try to handle device-not-present, math-fault and xmm-fault exceptions.
    */
   if (ExceptionNr == 7 || ExceptionNr == 16 || ExceptionNr == 19)
     {
       Status = KiHandleFpuFault(Tf, ExceptionNr);
       if (NT_SUCCESS(Status))
         {
           return(0);
         }
     }

   /*
    * Handle user exceptions differently
    */
   if ((Tf->Cs & 0xFFFF) == USER_CS)
     {
       return(KiUserTrapHandler(Tf, ExceptionNr, (PVOID)cr2));
     }
   else
    {
      return(KiKernelTrapHandler(Tf, ExceptionNr, (PVOID)cr2));
    }
}

BOOLEAN
STDCALL
KeContextToTrapFrame(PCONTEXT Context,
                     PKTRAP_FRAME TrapFrame)
{
    /* Start with the basic Registers */
    if ((Context->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL)
    {
        TrapFrame->Esp = Context->Esp;
        TrapFrame->Ss = Context->SegSs;
        TrapFrame->Cs = Context->SegCs;
        TrapFrame->Eip = Context->Eip;
        TrapFrame->Eflags = Context->EFlags;
        TrapFrame->Ebp = Context->Ebp;
    }

    /* Process the Integer Registers */
    if ((Context->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
    {
        TrapFrame->Eax = Context->Eax;
        TrapFrame->Ebx = Context->Ebx;
        TrapFrame->Ecx = Context->Ecx;
        TrapFrame->Edx = Context->Edx;
        TrapFrame->Esi = Context->Esi;
        TrapFrame->Edi = Context->Edi;
    }

    /* Process the Context Segments */
    if ((Context->ContextFlags & CONTEXT_SEGMENTS) == CONTEXT_SEGMENTS)
    {
        TrapFrame->Ds = Context->SegDs;
        TrapFrame->Es = Context->SegEs;
        TrapFrame->Fs = Context->SegFs;
        TrapFrame->Gs = Context->SegGs;
    }

    /* Handle the Debug Registers */
    if ((Context->ContextFlags & CONTEXT_DEBUG_REGISTERS) == CONTEXT_DEBUG_REGISTERS)
    {
        TrapFrame->Dr0 = Context->Dr0;
        TrapFrame->Dr1 = Context->Dr1;
        TrapFrame->Dr2 = Context->Dr2;
        TrapFrame->Dr3 = Context->Dr3;
        TrapFrame->Dr6 = Context->Dr6;
        TrapFrame->Dr7 = Context->Dr7;
    }

    /* Handle FPU and Extended Registers */
    return KiContextToFxSaveArea((PFX_SAVE_AREA)(TrapFrame + 1), Context);
}

VOID
KeTrapFrameToContext(PKTRAP_FRAME TrapFrame,
		     PCONTEXT Context)
{
   if ((Context->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL)
     {
	Context->SegSs = TrapFrame->Ss;
	Context->Esp = TrapFrame->Esp;
	Context->SegCs = TrapFrame->Cs;
	Context->Eip = TrapFrame->Eip;
	Context->EFlags = TrapFrame->Eflags;
	Context->Ebp = TrapFrame->Ebp;
     }
   if ((Context->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
     {
	Context->Eax = TrapFrame->Eax;
	Context->Ebx = TrapFrame->Ebx;
	Context->Ecx = TrapFrame->Ecx;
	/*
	 * NOTE: In the trap frame which is built on entry to a system
	 * call TrapFrame->Edx will actually hold the address of the
	 * previous TrapFrame. I don't believe leaking this information
	 * has security implications. Also EDX holds the address of the
	 * arguments to the system call in progress so it isn't of much
	 * interest to the debugger.
	 */
	Context->Edx = TrapFrame->Edx;
	Context->Esi = TrapFrame->Esi;
	Context->Edi = TrapFrame->Edi;
     }
   if ((Context->ContextFlags & CONTEXT_SEGMENTS) == CONTEXT_SEGMENTS)
     {
	Context->SegDs = TrapFrame->Ds;
	Context->SegEs = TrapFrame->Es;
	Context->SegFs = TrapFrame->Fs;
	Context->SegGs = TrapFrame->Gs;
     }
   if ((Context->ContextFlags & CONTEXT_DEBUG_REGISTERS) == CONTEXT_DEBUG_REGISTERS)
     {
	/*
	 * FIXME: Implement this case
	 */
	Context->ContextFlags &= (~CONTEXT_DEBUG_REGISTERS) | CONTEXT_i386;
     }
   if ((Context->ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT)
     {
	/*
	 * FIXME: Implement this case
	 *
	 * I think this should only be filled for FPU exceptions, otherwise I
         * would not know where to get it from as it can be the current state
	 * of the FPU or already saved in the thread's FPU save area.
	 *  -blight
	 */
	Context->ContextFlags &= (~CONTEXT_FLOATING_POINT) | CONTEXT_i386;
     }
#if 0
   if ((Context->ContextFlags & CONTEXT_EXTENDED_REGISTERS) == CONTEXT_EXTENDED_REGISTERS)
     {
	/*
	 * FIXME: Investigate this
	 *
	 * This is the XMM state (first 512 bytes of FXSAVE_FORMAT/FX_SAVE_AREA)
	 * This should only be filled in case of a SIMD exception I think, so
	 * this is not the right place (like for FPU the state could already be
	 * saved in the thread's FX_SAVE_AREA or still be in the CPU)
	 *  -blight
	 */
        Context->ContextFlags &= ~CONTEXT_EXTENDED_REGISTERS;
     }
#endif
}

VOID
KeDumpStackFrames(PULONG Frame)
{
	PULONG StackBase, StackEnd;
	MEMORY_BASIC_INFORMATION mbi;
	ULONG ResultLength = sizeof(mbi);
	NTSTATUS Status;

	DbgPrint("Frames:\n");
	_SEH_TRY
	{
		Status = MiQueryVirtualMemory (
			(HANDLE)-1,
			Frame,
			MemoryBasicInformation,
			&mbi,
			sizeof(mbi),
			&ResultLength );
		if ( !NT_SUCCESS(Status) )
		{
			DPRINT1("Can't dump stack frames: NtQueryVirtualMemory() failed: %x\n", Status );
			return;
		}

		StackBase = Frame;
		StackEnd = mbi.BaseAddress + mbi.RegionSize;

		while ( Frame >= StackBase && Frame < StackEnd )
		{
			ULONG Addr = Frame[1];
			if (!KeRosPrintAddress((PVOID)Addr))
				DbgPrint("<%X>", Addr);
			if ( Addr == 0 || Addr == 0xDEADBEEF )
				break;
			StackBase = Frame;
			Frame = (PULONG)Frame[0];
			DbgPrint("\n");
		}
	}
	_SEH_HANDLE
	{
	}
	_SEH_END;
	DbgPrint("\n");
}

VOID STDCALL
KeRosDumpStackFrames ( PULONG Frame, ULONG FrameCount )
{
	ULONG i=0;
	PULONG StackBase, StackEnd;
	MEMORY_BASIC_INFORMATION mbi;
	ULONG ResultLength = sizeof(mbi);
	NTSTATUS Status;

	DbgPrint("Frames: ");
	_SEH_TRY
	{
		if ( !Frame )
		{
#if defined __GNUC__
			__asm__("mov %%ebp, %%ebx" : "=b" (Frame) : );
#elif defined(_MSC_VER)
			__asm mov [Frame], ebp
#endif
			//Frame = (PULONG)Frame[0]; // step out of KeRosDumpStackFrames
		}

		Status = MiQueryVirtualMemory (
			(HANDLE)-1,
			Frame,
			MemoryBasicInformation,
			&mbi,
			sizeof(mbi),
			&ResultLength );
		if ( !NT_SUCCESS(Status) )
		{
			DPRINT1("Can't dump stack frames: NtQueryVirtualMemory() failed: %x\n", Status );
			return;
		}

		StackBase = Frame;
		StackEnd = mbi.BaseAddress + mbi.RegionSize;

		while ( Frame >= StackBase && Frame < StackEnd && i++ < FrameCount )
		{
			ULONG Addr = Frame[1];
			if (!KeRosPrintAddress((PVOID)Addr))
				DbgPrint("<%X>", Addr);
			if ( Addr == 0 || Addr == 0xDEADBEEF )
				break;
			StackBase = Frame;
			Frame = (PULONG)Frame[0];
			DbgPrint(" ");
		}
	}
	_SEH_HANDLE
	{
	}
	_SEH_END;
	DbgPrint("\n");
}

ULONG STDCALL
KeRosGetStackFrames ( PULONG Frames, ULONG FrameCount )
{
	ULONG Count = 0;
	PULONG StackBase, StackEnd, Frame;
	MEMORY_BASIC_INFORMATION mbi;
	ULONG ResultLength = sizeof(mbi);
	NTSTATUS Status;

	_SEH_TRY
	{
#if defined __GNUC__
		__asm__("mov %%ebp, %%ebx" : "=b" (Frame) : );
#elif defined(_MSC_VER)
		__asm mov [Frame], ebp
#endif

		Status = MiQueryVirtualMemory (
			(HANDLE)-1,
			Frame,
			MemoryBasicInformation,
			&mbi,
			sizeof(mbi),
			&ResultLength );
		if ( !NT_SUCCESS(Status) )
		{
			DPRINT1("Can't get stack frames: NtQueryVirtualMemory() failed: %x\n", Status );
			return 0;
		}

		StackBase = Frame;
		StackEnd = mbi.BaseAddress + mbi.RegionSize;

		while ( Count < FrameCount && Frame >= StackBase && Frame < StackEnd )
		{
			Frames[Count++] = Frame[1];
			StackBase = Frame;
			Frame = (PULONG)Frame[0];
		}
	}
	_SEH_HANDLE
	{
	}
	_SEH_END;
	return Count;
}

static void
set_system_call_gate(unsigned int sel, unsigned int func)
{
   DPRINT("sel %x %d\n",sel,sel);
   KiIdt[sel].a = (((int)func)&0xffff) +
     (KERNEL_CS << 16);
   KiIdt[sel].b = 0xef00 + (((int)func)&0xffff0000);
   DPRINT("idt[sel].b %x\n",KiIdt[sel].b);
}

static void set_interrupt_gate(unsigned int sel, unsigned int func)
{
   DPRINT("set_interrupt_gate(sel %d, func %x)\n",sel,func);
   KiIdt[sel].a = (((int)func)&0xffff) +
     (KERNEL_CS << 16);
   KiIdt[sel].b = 0x8e00 + (((int)func)&0xffff0000);
}

static void set_trap_gate(unsigned int sel, unsigned int func, unsigned int dpl)
{
   DPRINT("set_trap_gate(sel %d, func %x, dpl %d)\n",sel, func, dpl);
   ASSERT(dpl <= 3);
   KiIdt[sel].a = (((int)func)&0xffff) +
     (KERNEL_CS << 16);
   KiIdt[sel].b = 0x8f00 + (dpl << 13) + (((int)func)&0xffff0000);
}

static void
set_task_gate(unsigned int sel, unsigned task_sel)
{
  KiIdt[sel].a = task_sel << 16;
  KiIdt[sel].b = 0x8500;
}

VOID INIT_FUNCTION
KeInitExceptions(VOID)
/*
 * FUNCTION: Initalize CPU exception handling
 */
{
   int i;

   DPRINT("KeInitExceptions()\n");

   /*
    * Set up the other gates
    */
   set_trap_gate(0, (ULONG)KiTrap0, 0);
   set_trap_gate(1, (ULONG)KiTrap1, 0);
   set_trap_gate(2, (ULONG)KiTrap2, 0);
   set_trap_gate(3, (ULONG)KiTrap3, 3);
   set_trap_gate(4, (ULONG)KiTrap4, 0);
   set_trap_gate(5, (ULONG)KiTrap5, 0);
   set_trap_gate(6, (ULONG)KiTrap6, 0);
   set_trap_gate(7, (ULONG)KiTrap7, 0);
   set_task_gate(8, TRAP_TSS_SELECTOR);
   set_trap_gate(9, (ULONG)KiTrap9, 0);
   set_trap_gate(10, (ULONG)KiTrap10, 0);
   set_trap_gate(11, (ULONG)KiTrap11, 0);
   set_trap_gate(12, (ULONG)KiTrap12, 0);
   set_trap_gate(13, (ULONG)KiTrap13, 0);
   set_interrupt_gate(14, (ULONG)KiTrap14);
   set_trap_gate(15, (ULONG)KiTrap15, 0);
   set_trap_gate(16, (ULONG)KiTrap16, 0);
   set_trap_gate(17, (ULONG)KiTrap17, 0);
   set_trap_gate(18, (ULONG)KiTrap18, 0);
   set_trap_gate(19, (ULONG)KiTrap19, 0);

   for (i = 20; i < 256; i++)
     {
        set_trap_gate(i,(int)KiTrapUnknown, 0);
     }

   set_system_call_gate(0x2d,(int)KiDebugService);
   set_system_call_gate(0x2e,(int)KiSystemService);
}

/*
 * @implemented
 */
NTSTATUS STDCALL
KeRaiseUserException(IN NTSTATUS ExceptionCode)
{
   ULONG OldEip;
   PKTHREAD Thread = KeGetCurrentThread();

    _SEH_TRY {
        Thread->Teb->ExceptionCode = ExceptionCode;
    } _SEH_HANDLE {
        return(ExceptionCode);
    } _SEH_END;

   OldEip = Thread->TrapFrame->Eip;
   Thread->TrapFrame->Eip = (ULONG_PTR)LdrpGetSystemDllRaiseExceptionDispatcher();
   return((NTSTATUS)OldEip);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN BOOLEAN SearchFrames)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKTRAP_FRAME TrapFrame = Thread->TrapFrame;
    PKTRAP_FRAME PrevTrapFrame = (PKTRAP_FRAME)TrapFrame->Edx;

    KeGetCurrentKPCR()->Tib.ExceptionList = TrapFrame->ExceptionList;

    KiDispatchException(ExceptionRecord,
                        Context,
                        TrapFrame,
                        KeGetPreviousMode(),
                        SearchFrames);

    /* Restore the user context */
    Thread->TrapFrame = PrevTrapFrame;
    __asm__("mov %%ebx, %%esp;\n" "jmp _KiServiceExit": : "b" (TrapFrame));

    /* We never get here */
    return(STATUS_SUCCESS);
}
