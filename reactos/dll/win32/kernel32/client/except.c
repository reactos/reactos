/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/except.c
 * PURPOSE:         Exception functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  modified from WINE [ Onno Hovers, (onno@stack.urc.tue.nl) ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

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
#ifdef _M_IX86
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
#elif defined(_M_AMD64)
   DbgPrint("CS:RIP %x:%I64x\n", pc->SegCs&0xffff, pc->Rip );
   DbgPrint("DS %x ES %x FS %x GS %x\n", pc->SegDs&0xffff, pc->SegEs&0xffff,
	    pc->SegFs&0xffff, pc->SegGs&0xfff);
   DbgPrint("RAX: %I64x   RBX: %I64x   RCX: %I64x RDI: %I64x\n", pc->Rax, pc->Rbx, pc->Rcx, pc->Rdi);
   DbgPrint("RDX: %I64x   RBP: %I64x   RSI: %I64x   RSP: %I64x\n", pc->Rdx, pc->Rbp, pc->Rsi, pc->Rsp);
   DbgPrint("R8: %I64x   R9: %I64x   R10: %I64x   R11: %I64x\n", pc->R8, pc->R9, pc->R10, pc->R11);
   DbgPrint("R12: %I64x   R13: %I64x   R14: %I64x   R15: %I64x\n", pc->R12, pc->R13, pc->R14, pc->R15);
   DbgPrint("EFLAGS: %.8x\n", pc->EFlags);
#elif defined(_M_ARM)
   DbgPrint("PC:  %08lx   LR:  %08lx   SP:  %08lx\n", pc->Pc);
   DbgPrint("R0:  %08lx   R1:  %08lx   R2:  %08lx   R3:  %08lx\n", pc->R0, pc->R1, pc->R2, pc->R3);
   DbgPrint("R4:  %08lx   R5:  %08lx   R6:  %08lx   R7:  %08lx\n", pc->R4, pc->R5, pc->R6, pc->R7);
   DbgPrint("R8:  %08lx   R9:  %08lx   R10: %08lx   R11: %08lx\n", pc->R8, pc->R9, pc->R10, pc->R11);
   DbgPrint("R12: %08lx   CPSR: %08lx  FPSCR: %08lx\n", pc->R12, pc->Cpsr, pc->R1, pc->Fpscr, pc->R3);
#else
#error "Unknown architecture"
#endif
}

static VOID
PrintStackTrace(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
    PVOID StartAddr;
    CHAR szMod[128] = "";
    PEXCEPTION_RECORD ExceptionRecord = ExceptionInfo->ExceptionRecord;
    PCONTEXT ContextRecord = ExceptionInfo->ContextRecord;

    /* Print a stack trace. */
    DbgPrint("Unhandled exception\n");
    DbgPrint("ExceptionCode:    %8x\n", ExceptionRecord->ExceptionCode);

    if ((NTSTATUS)ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION &&
        ExceptionRecord->NumberParameters == 2)
    {
        DbgPrint("Faulting Address: %8x\n", ExceptionRecord->ExceptionInformation[1]);
    }

    _dump_context (ContextRecord);
    _module_name_from_addr(ExceptionRecord->ExceptionAddress, &StartAddr, szMod, sizeof(szMod));
    DbgPrint("Address:\n   %8x+%-8x   %s\n",
             (PVOID)StartAddr,
             (ULONG_PTR)ExceptionRecord->ExceptionAddress - (ULONG_PTR)StartAddr,
             szMod);
#ifdef _M_IX86
    DbgPrint("Frames:\n");

    _SEH2_TRY
    {
        UINT i;
        PULONG Frame = (PULONG)ContextRecord->Ebp;

        for (i = 0; Frame[1] != 0 && Frame[1] != 0xdeadbeef && i < 128; i++)
        {
            if (IsBadReadPtr((PVOID)Frame[1], 4))
            {
                DbgPrint("   %8x%9s   %s\n", Frame[1], "<invalid address>"," ");
            }
            else
            {
                _module_name_from_addr((const void*)Frame[1], &StartAddr,
                                       szMod, sizeof(szMod));
                DbgPrint("   %8x+%-8x   %s\n",
                         (PVOID)StartAddr,
                         (ULONG_PTR)Frame[1] - (ULONG_PTR)StartAddr,
                         szMod);
            }

            if (IsBadReadPtr((PVOID)Frame[0], sizeof(*Frame) * 2))
                break;

            Frame = (PULONG)Frame[0];
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DbgPrint("<error dumping stack trace: 0x%x>\n", _SEH2_GetExceptionCode());
    }
    _SEH2_END;
#endif
}

/* GLOBALS ********************************************************************/

LPTOP_LEVEL_EXCEPTION_FILTER GlobalTopLevelExceptionFilter;
DWORD g_dwLastErrorToBreakOn;

/* FUNCTIONS ******************************************************************/

LONG
WINAPI
BasepCheckForReadOnlyResource(IN PVOID Ptr)
{
    PVOID Data;
    ULONG Size, OldProtect;
    MEMORY_BASIC_INFORMATION mbi;
    NTSTATUS Status;
    LONG Ret = EXCEPTION_CONTINUE_SEARCH;

    /* Check if it was an attempt to write to a read-only image section! */
    Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                  Ptr,
                                  MemoryBasicInformation,
                                  &mbi,
                                  sizeof(mbi),
                                  NULL);
    if (NT_SUCCESS(Status) &&
        mbi.Protect == PAGE_READONLY && mbi.Type == MEM_IMAGE)
    {
        /* Attempt to treat it as a resource section. We need to
           use SEH here because we don't know if it's actually a
           resource mapping */
        _SEH2_TRY
        {
            Data = RtlImageDirectoryEntryToData(mbi.AllocationBase,
                                                TRUE,
                                                IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                                &Size);

            if (Data != NULL &&
                (ULONG_PTR)Ptr >= (ULONG_PTR)Data &&
                (ULONG_PTR)Ptr < (ULONG_PTR)Data + Size)
            {
                /* The user tried to write into the resources. Make the page
                   writable... */
                Size = 1;
                Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                                &Ptr,
                                                &Size,
                                                PAGE_READWRITE,
                                                &OldProtect);
                if (NT_SUCCESS(Status))
                {
                    Ret = EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _SEH2_END;
    }

    return Ret;
}

UINT
WINAPI
GetErrorMode(VOID)
{
    NTSTATUS Status;
    UINT ErrMode;

    /* Query the current setting */
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDefaultHardErrorMode,
                                       (PVOID)&ErrMode,
                                       sizeof(ErrMode),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail if we couldn't query */
        BaseSetLastNTError(Status);
        return 0;
    }

    /* Check if NOT failing critical errors was requested */
    if (ErrMode & SEM_FAILCRITICALERRORS)
    {
        /* Mask it out, since the native API works differently */
        ErrMode &= ~SEM_FAILCRITICALERRORS;
    }
    else
    {
        /* OR it if the caller didn't, due to different native semantics */
        ErrMode |= SEM_FAILCRITICALERRORS;
    }

    /* Return the mode */
    return ErrMode;
}

/*
 * @implemented
 */
LONG WINAPI
UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
   LONG RetValue;
   HANDLE DebugPort = NULL;
   NTSTATUS ErrCode;
   ULONG_PTR ErrorParameters[4];
   ULONG ErrorResponse;
   PEXCEPTION_RECORD ExceptionRecord = ExceptionInfo->ExceptionRecord;
   LPTOP_LEVEL_EXCEPTION_FILTER RealFilter;

   if ((NTSTATUS)ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION &&
       ExceptionRecord->NumberParameters >= 2)
   {
      switch(ExceptionRecord->ExceptionInformation[0])
      {
      case EXCEPTION_WRITE_FAULT:
         /* Change the protection on some write attempts, some InstallShield setups
            have this bug */
         RetValue = BasepCheckForReadOnlyResource(
            (PVOID)ExceptionRecord->ExceptionInformation[1]);
         if (RetValue == EXCEPTION_CONTINUE_EXECUTION)
            return EXCEPTION_CONTINUE_EXECUTION;
         break;
      case EXCEPTION_EXECUTE_FAULT:
         /* FIXME */
         break;
      }
   }

   /* Is there a debugger running ? */
   ErrCode = NtQueryInformationProcess(NtCurrentProcess(), ProcessDebugPort,
                                       &DebugPort, sizeof(HANDLE), NULL);
   if (!NT_SUCCESS(ErrCode) && ErrCode != STATUS_NOT_IMPLEMENTED)
   {
      BaseSetLastNTError(ErrCode);
      return EXCEPTION_EXECUTE_HANDLER;
   }

   if (DebugPort)
   {
      /* Pass the exception to debugger. */
      DPRINT("Passing exception to debugger\n");
      return EXCEPTION_CONTINUE_SEARCH;
   }

   RealFilter = RtlDecodePointer(GlobalTopLevelExceptionFilter);
   if (RealFilter)
   {
      LONG ret = RealFilter(ExceptionInfo);
      if (ret != EXCEPTION_CONTINUE_SEARCH)
         return ret;
   }

   PrintStackTrace(ExceptionInfo);

   /* Save exception code and address */
   ErrorParameters[0] = (ULONG)ExceptionRecord->ExceptionCode;
   ErrorParameters[1] = (ULONG_PTR)ExceptionRecord->ExceptionAddress;

   if ((NTSTATUS)ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION)
   {
       /* get the type of operation that caused the access violation */
       ErrorParameters[2] = ExceptionRecord->ExceptionInformation[0];
   }
   else
   {
       ErrorParameters[2] = ExceptionRecord->ExceptionInformation[2];
   }

   /* Save faulting address */
   ErrorParameters[3] = ExceptionRecord->ExceptionInformation[1];

   /* Raise the harderror */
   ErrCode = NtRaiseHardError(STATUS_UNHANDLED_EXCEPTION,
       4, 0, ErrorParameters, OptionOkCancel, &ErrorResponse);

   if (NT_SUCCESS(ErrCode) && (ErrorResponse == ResponseCancel))
   {
       /* FIXME: Check the result, if the "Cancel" button was
                 clicked run a debugger */
       DPRINT1("Debugging is not implemented yet\n");
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
WINAPI
RaiseException(IN DWORD dwExceptionCode,
               IN DWORD dwExceptionFlags,
               IN DWORD nNumberOfArguments,
               IN CONST ULONG_PTR *lpArguments OPTIONAL)
{
    EXCEPTION_RECORD ExceptionRecord;

    /* Setup the exception record */
    ExceptionRecord.ExceptionCode = dwExceptionCode;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.ExceptionAddress = (PVOID)RaiseException;
    ExceptionRecord.ExceptionFlags = dwExceptionFlags & EXCEPTION_NONCONTINUABLE;

    /* Check if we have arguments */
    if (!lpArguments)
    {
        /* We don't */
        ExceptionRecord.NumberParameters = 0;
    }
    else
    {
        /* We do, normalize the count */
        if (nNumberOfArguments > EXCEPTION_MAXIMUM_PARAMETERS)
        {
            nNumberOfArguments = EXCEPTION_MAXIMUM_PARAMETERS;
        }

        /* Set the count of parameters and copy them */
        ExceptionRecord.NumberParameters = nNumberOfArguments;
        RtlCopyMemory(ExceptionRecord.ExceptionInformation,
                      lpArguments,
                      nNumberOfArguments * sizeof(ULONG));
    }

    /* Better handling of Delphi Exceptions... a ReactOS Hack */
    if (dwExceptionCode == 0xeedface || dwExceptionCode == 0xeedfade)
    {
        DPRINT1("Delphi Exception at address: %p\n", ExceptionRecord.ExceptionInformation[0]);
        DPRINT1("Exception-Object: %p\n", ExceptionRecord.ExceptionInformation[1]);
        DPRINT1("Exception text: %lx\n", ExceptionRecord.ExceptionInformation[2]);
    }

    /* Trace the wine special error and show the modulename and functionname */
    if (dwExceptionCode == 0x80000100 /*EXCEPTION_WINE_STUB*/)
    {
       /* Numbers of parameter must be equal to two */
       if (ExceptionRecord.NumberParameters == 2)
       {
          DPRINT1("Missing function in   : %s\n", ExceptionRecord.ExceptionInformation[0]);
          DPRINT1("with the functionname : %s\n", ExceptionRecord.ExceptionInformation[1]);
       }
    }

    /* Raise the exception */
    RtlRaiseException(&ExceptionRecord);
}

/*
 * @implemented
 */
UINT
WINAPI
SetErrorMode(IN UINT uMode)
{
    UINT PrevErrMode, NewMode;

    /* Get the previous mode */
    PrevErrMode = GetErrorMode();
    NewMode = uMode;

    /* Check if failing critical errors was requested */
    if (NewMode & SEM_FAILCRITICALERRORS)
    {
        /* Mask it out, since the native API works differently */
        NewMode &= ~SEM_FAILCRITICALERRORS;
    }
    else
    {
        /* OR it if the caller didn't, due to different native semantics */
        NewMode |= SEM_FAILCRITICALERRORS;
    }

    /* Always keep no alignment faults if they were set */
    NewMode |= (PrevErrMode & SEM_NOALIGNMENTFAULTEXCEPT);

    /* Set the new mode */
    NtSetInformationProcess(NtCurrentProcess(),
                            ProcessDefaultHardErrorMode,
                            (PVOID)&NewMode,
                            sizeof(NewMode));

    /* Return the previous mode */
    return PrevErrMode;
}

/*
 * @implemented
 */
LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI
DECLSPEC_HOTPATCH
SetUnhandledExceptionFilter(IN LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
    PVOID EncodedPointer, EncodedOldPointer;

    EncodedPointer = RtlEncodePointer(lpTopLevelExceptionFilter);
    EncodedOldPointer = InterlockedExchangePointer((PVOID*)&GlobalTopLevelExceptionFilter,
                                            EncodedPointer);
    return RtlDecodePointer(EncodedOldPointer);
}

/*
 * @implemented
 */
BOOL
WINAPI
IsBadReadPtr(IN LPCVOID lp,
             IN UINT_PTR ucb)
{
    ULONG PageSize;
    BOOLEAN Result = FALSE;
    volatile CHAR *Current;
    PCHAR Last;

    /* Quick cases */
    if (!ucb) return FALSE;
    if (!lp) return TRUE;

    /* Get the page size */
    PageSize = BaseStaticServerData->SysInfo.PageSize;

    /* Calculate start and end */
    Current = (volatile CHAR*)lp;
    Last = (PCHAR)((ULONG_PTR)lp + ucb - 1);

    /* Another quick failure case */
    if (Last < Current) return TRUE;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Do an initial probe */
        *Current;

        /* Align the addresses */
        Current = (volatile CHAR *)ROUND_DOWN(Current, PageSize);
        Last = (PCHAR)ROUND_DOWN(Last, PageSize);

        /* Probe the entire range */
        while (Current != Last)
        {
            Current += PageSize;
            *Current;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* We hit an exception, so return true */
        Result = TRUE;
    }
    _SEH2_END

    /* Return exception status */
    return Result;
}

/*
 * @implemented
 */
BOOL
NTAPI
IsBadHugeReadPtr(LPCVOID lp,
                 UINT_PTR ucb)
{
    /* Implementation is the same on 32-bit */
    return IsBadReadPtr(lp, ucb);
}

/*
 * @implemented
 */
BOOL
NTAPI
IsBadCodePtr(FARPROC lpfn)
{
    /* Executing has the same privileges as reading */
    return IsBadReadPtr((LPVOID)lpfn, 1);
}

/*
 * @implemented
 */
BOOL
NTAPI
IsBadWritePtr(IN LPVOID lp,
              IN UINT_PTR ucb)
{
    ULONG PageSize;
    BOOLEAN Result = FALSE;
    volatile CHAR *Current;
    PCHAR Last;

    /* Quick cases */
    if (!ucb) return FALSE;
    if (!lp) return TRUE;

    /* Get the page size */
    PageSize = BaseStaticServerData->SysInfo.PageSize;

    /* Calculate start and end */
    Current = (volatile CHAR*)lp;
    Last = (PCHAR)((ULONG_PTR)lp + ucb - 1);

    /* Another quick failure case */
    if (Last < Current) return TRUE;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Do an initial probe */
        *Current = *Current;

        /* Align the addresses */
        Current = (volatile CHAR *)ROUND_DOWN(Current, PageSize);
        Last = (PCHAR)ROUND_DOWN(Last, PageSize);

        /* Probe the entire range */
        while (Current != Last)
        {
            Current += PageSize;
            *Current = *Current;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* We hit an exception, so return true */
        Result = TRUE;
    }
    _SEH2_END

    /* Return exception status */
    return Result;
}

/*
 * @implemented
 */
BOOL
NTAPI
IsBadHugeWritePtr(IN LPVOID lp,
                  IN UINT_PTR ucb)
{
    /* Implementation is the same on 32-bit */
    return IsBadWritePtr(lp, ucb);
}

/*
 * @implemented
 */
BOOL
NTAPI
IsBadStringPtrW(IN LPCWSTR lpsz,
                IN UINT_PTR ucchMax)
{
    BOOLEAN Result = FALSE;
    volatile WCHAR *Current;
    PWCHAR Last;
    WCHAR Char;

    /* Quick cases */
    if (!ucchMax) return FALSE;
    if (!lpsz) return TRUE;

    /* Calculate start and end */
    Current = (volatile WCHAR*)lpsz;
    Last = (PWCHAR)((ULONG_PTR)lpsz + (ucchMax * 2) - 2);

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Probe the entire range */
        Char = *Current++;
        while ((Char) && (Current != Last)) Char = *Current++;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* We hit an exception, so return true */
        Result = TRUE;
    }
    _SEH2_END

    /* Return exception status */
    return Result;
}

/*
 * @implemented
 */
BOOL
NTAPI
IsBadStringPtrA(IN LPCSTR lpsz,
                IN UINT_PTR ucchMax)
{
    BOOLEAN Result = FALSE;
    volatile CHAR *Current;
    PCHAR Last;
    CHAR Char;

    /* Quick cases */
    if (!ucchMax) return FALSE;
    if (!lpsz) return TRUE;

    /* Calculate start and end */
    Current = (volatile CHAR*)lpsz;
    Last = (PCHAR)((ULONG_PTR)lpsz + ucchMax - 1);

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Probe the entire range */
        Char = *Current++;
        while ((Char) && (Current != Last)) Char = *Current++;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* We hit an exception, so return true */
        Result = TRUE;
    }
    _SEH2_END

    /* Return exception status */
    return Result;
}

/*
 * @implemented
 */
VOID
WINAPI
SetLastError(IN DWORD dwErrCode)
{
    /* Break if a debugger requested checking for this error code */
    if ((g_dwLastErrorToBreakOn) && (g_dwLastErrorToBreakOn == dwErrCode)) DbgBreakPoint();

    /* Set last error if it's a new error */
    if (NtCurrentTeb()->LastErrorValue != dwErrCode) NtCurrentTeb()->LastErrorValue = dwErrCode;
}

/*
 * @implemented
 */
DWORD
WINAPI
BaseSetLastNTError(IN NTSTATUS Status)
{
    DWORD dwErrCode;

    /* Convert from NT to Win32, then set */
    dwErrCode = RtlNtStatusToDosError(Status);
    SetLastError(dwErrCode);
    return dwErrCode;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetLastError(VOID)
{
    /* Return the current value */
    return NtCurrentTeb()->LastErrorValue;
}

/* EOF */
