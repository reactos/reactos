/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/except.c
 * PURPOSE:         Exception functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  modified from WINE [ Onno Hovers, (onno@stack.urc.tue.nl) ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

LPTOP_LEVEL_EXCEPTION_FILTER GlobalTopLevelExceptionFilter = UnhandledExceptionFilter;

UINT
STDCALL
GetErrorMode(VOID)
{
  NTSTATUS Status;
  UINT ErrMode;
  
  Status = NtQueryInformationProcess(NtCurrentProcess(),
                                     ProcessDefaultHardErrorMode,
                                     (PVOID)&ErrMode,
                                     sizeof(ErrMode),
                                     NULL);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return 0;
  }
  
  return ErrMode;
}

/*
 * @implemented
 */
UINT 
STDCALL
SetErrorMode(UINT uMode)
{
   UINT PrevErrMode;
   NTSTATUS Status;
   
   PrevErrMode = GetErrorMode();
   
   Status = NtSetInformationProcess(NtCurrentProcess(),
                                    ProcessDefaultHardErrorMode,
                                    (PVOID)&uMode,
                                    sizeof(uMode));
   if(!NT_SUCCESS(Status))
   {
     SetLastErrorByStatus(Status);
     return 0;
   }

   return PrevErrMode;
}


/*
 * @implemented
 */
LPTOP_LEVEL_EXCEPTION_FILTER
STDCALL
SetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
    )
{
    return InterlockedExchangePointer(&GlobalTopLevelExceptionFilter,
                                      lpTopLevelExceptionFilter);
}


/*
 * Private helper function to lookup the module name from a given address.
 * The address can point to anywhere within the module.
 */
static const char*
_module_name_from_addr(const void* addr, void **module_start_addr, 
                       char* psz, size_t nChars)
{
   MEMORY_BASIC_INFORMATION mbi;
   if (VirtualQuery(addr, &mbi, sizeof(mbi)) != sizeof(mbi) ||
       !GetModuleFileNameA((HMODULE)mbi.AllocationBase, psz, nChars))
   {
      psz[0] = '\0';
      *module_start_addr = 0;
   }
   else
   {
      *module_start_addr = (void *)mbi.AllocationBase;
   }
   return psz;
}

static VOID
_dump_context(PCONTEXT pc)
{
   /*
    * Print out the CPU registers
    */
   DbgPrint("CS:EIP %x:%x\n", pc->SegCs&0xffff, pc->Eip );
   DbgPrint("DS %x ES %x FS %x GS %x\n", pc->SegDs&0xffff, pc->SegEs&0xffff,
	    pc->SegFs&0xffff, pc->SegGs&0xfff);
   DbgPrint("EAX: %.8x   EBX: %.8x   ECX: %.8x\n", pc->Eax, pc->Ebx, pc->Ecx);
   DbgPrint("EDX: %.8x   EBP: %.8x   ESI: %.8x   ESP: %.8x\n", pc->Edx,
	    pc->Ebp, pc->Esi, pc->Esp);
   DbgPrint("EDI: %.8x   EFLAGS: %.8x\n", pc->Edi, pc->EFlags);
}

/*
 * @unimplemented
 */
LONG STDCALL
UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
#if 0
   DWORD RetValue;
#endif
   HANDLE DebugPort = NULL;
   NTSTATUS ErrCode;

#if 0
   if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION &&
       ExceptionInfo->ExceptionRecord->ExceptionInformation[0])
   {
      RetValue = _BasepCheckForReadOnlyResource(
         ExceptionInfo->ExceptionRecord->ExceptionInformation[1]);
      if (RetValue == EXCEPTION_CONTINUE_EXECUTION)
         return EXCEPTION_CONTINUE_EXECUTION;
   }
#endif

   /* Is there a debugger running ? */
   ErrCode = NtQueryInformationProcess(NtCurrentProcess(), ProcessDebugPort,
                                       &DebugPort, sizeof(HANDLE), NULL);
   if (!NT_SUCCESS(ErrCode) && ErrCode != STATUS_NOT_IMPLEMENTED)
   {
      SetLastErrorByStatus(ErrCode);
      return EXCEPTION_EXECUTE_HANDLER;
   }

   if (DebugPort)
   {
      /* Pass the exception to debugger. */
      DPRINT("Passing exception to debugger\n");
      return EXCEPTION_CONTINUE_SEARCH;
   }

   if ((GetErrorMode() & SEM_NOGPFAULTERRORBOX) == 0)
   {
#ifdef _X86_
      PULONG Frame;
      PVOID StartAddr;
      CHAR szMod[128] = "";
#endif

      /* Print a stack trace. */
      DbgPrint("Unhandled exception\n");
      DbgPrint("Address:\n");
      DbgPrint("   %8x   %s\n",
         ExceptionInfo->ExceptionRecord->ExceptionAddress,
         _module_name_from_addr(ExceptionInfo->ExceptionRecord->ExceptionAddress, &StartAddr, szMod, sizeof(szMod)));
      _dump_context ( ExceptionInfo->ContextRecord );
#ifdef _X86_
      DbgPrint("Frames:\n");
      _SEH_TRY
      {
         Frame = (PULONG)ExceptionInfo->ContextRecord->Ebp;
         while (Frame[1] != 0 && Frame[1] != 0xdeadbeef)
         {
            if (IsBadReadPtr((PVOID)Frame[1], 4)) {
              DbgPrint("   %8x%9s   %s\n", Frame[1], "<invalid address>"," ");
            } else {
              _module_name_from_addr((const void*)Frame[1], &StartAddr,
                                     szMod, sizeof(szMod));
              DbgPrint("   %8x+%-8x   %s\n",
                      (PVOID)StartAddr,
                      (ULONG_PTR)Frame[1] - (ULONG_PTR)StartAddr, szMod);
            }
            if (IsBadReadPtr((PVOID)Frame[0], sizeof(*Frame) * 2)) {
              break;
            }
            Frame = (PULONG)Frame[0];
         }
      }
      _SEH_HANDLE
      {
         DbgPrint("<error dumping stack trace: 0x%x>\n", _SEH_GetExceptionCode());
      }
      _SEH_END;
#endif
   }

   /*
    * Returning EXCEPTION_EXECUTE_HANDLER means that the code in 
    * the __except block will be executed. Normally this will end up in a
    * Terminate process.
    */

   return EXCEPTION_EXECUTE_HANDLER;	
}


/*
 * @implemented
 */
VOID
STDCALL
RaiseException (
	DWORD		dwExceptionCode,
	DWORD		dwExceptionFlags,
	DWORD		nNumberOfArguments,
	CONST DWORD	* lpArguments		OPTIONAL
	)
{
	EXCEPTION_RECORD ExceptionRecord;

	/* Do NOT normalize dwExceptionCode: it will be done in
	 * NTDLL.RtlRaiseException().
	 */
	ExceptionRecord.ExceptionCode = dwExceptionCode;
	ExceptionRecord.ExceptionRecord = NULL;
	ExceptionRecord.ExceptionAddress = (PVOID) RaiseException;
	/*
	 * Normalize dwExceptionFlags.
	 */
	ExceptionRecord.ExceptionFlags = (dwExceptionFlags & EXCEPTION_NONCONTINUABLE);
	/*
	 * Normalize nNumberOfArguments.
	 */
	if (EXCEPTION_MAXIMUM_PARAMETERS < nNumberOfArguments)
	{
		nNumberOfArguments = EXCEPTION_MAXIMUM_PARAMETERS;
	}
	/*
	 * If the exception has no argument,
	 * ignore nNumberOfArguments and lpArguments.
	 */
	if (NULL == lpArguments)
	{
		ExceptionRecord.NumberParameters = 0;
	}
	else
	{
		ExceptionRecord.NumberParameters = nNumberOfArguments;
		for (	nNumberOfArguments = 0;
			(nNumberOfArguments < ExceptionRecord.NumberParameters); 
			nNumberOfArguments ++
			)
		{
			ExceptionRecord.ExceptionInformation [nNumberOfArguments]
				= *lpArguments ++;
		}
	}
	RtlRaiseException (& ExceptionRecord);
}

/* EOF */
