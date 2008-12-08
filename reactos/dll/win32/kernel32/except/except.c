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
#include <debug.h>

LPTOP_LEVEL_EXCEPTION_FILTER GlobalTopLevelExceptionFilter = NULL;

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
        SetLastErrorByStatus(Status);
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
UINT
WINAPI
SetErrorMode(IN UINT uMode)
{
    UINT PrevErrMode, NewMode;
    NTSTATUS Status;

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

    /* Set the new mode */
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessDefaultHardErrorMode,
                                     (PVOID)&NewMode,
                                     sizeof(NewMode));
    if(!NT_SUCCESS(Status)) SetLastErrorByStatus(Status);

    /* Return the previous mode */
    return PrevErrMode;
}

/*
 * @implemented
 */
LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI
SetUnhandledExceptionFilter(
    IN LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
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
   DbgPrint("CS:EIP %x:%I64x\n", pc->SegCs&0xffff, pc->Rip );
   DbgPrint("DS %x ES %x FS %x GS %x\n", pc->SegDs&0xffff, pc->SegEs&0xffff,
	    pc->SegFs&0xffff, pc->SegGs&0xfff);
   DbgPrint("RAX: %I64x   RBX: %I64x   RCX: %I64x RDI: %I64x\n", pc->Rax, pc->Rbx, pc->Rcx, pc->Rdi);
   DbgPrint("RDX: %I64x   RBP: %I64x   RSI: %I64x   RSP: %I64x\n", pc->Rdx, pc->Rbp, pc->Rsi, pc->Rsp);
   DbgPrint("R8: %I64x   R9: %I64x   R10: %I64x   R11: %I64x\n", pc->R8, pc->R9, pc->R10, pc->R11);
   DbgPrint("R12: %I64x   R13: %I64x   R14: %I64x   R15: %I64x\n", pc->R12, pc->R13, pc->R14, pc->R15);
   DbgPrint("EFLAGS: %.8x\n", pc->EFlags);
#else
#warning Unknown architecture
#endif
}

static LONG
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

/*
 * @unimplemented
 */
LONG WINAPI
UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
   LONG RetValue;
   HANDLE DebugPort = NULL;
   NTSTATUS ErrCode;
   ULONG_PTR ErrorParameters[4];
   ULONG ErrorResponse;

   if ((NTSTATUS)ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION &&
       ExceptionInfo->ExceptionRecord->NumberParameters >= 2)
   {
      switch(ExceptionInfo->ExceptionRecord->ExceptionInformation[0])
      {
      case EXCEPTION_WRITE_FAULT:
         /* Change the protection on some write attempts, some InstallShield setups
            have this bug */
         RetValue = BasepCheckForReadOnlyResource(
            (PVOID)ExceptionInfo->ExceptionRecord->ExceptionInformation[1]);
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
      SetLastErrorByStatus(ErrCode);
      return EXCEPTION_EXECUTE_HANDLER;
   }

   if (DebugPort)
   {
      /* Pass the exception to debugger. */
      DPRINT("Passing exception to debugger\n");
      return EXCEPTION_CONTINUE_SEARCH;
   }

   if (GlobalTopLevelExceptionFilter)
   {
      LONG ret = GlobalTopLevelExceptionFilter( ExceptionInfo );
      if (ret != EXCEPTION_CONTINUE_SEARCH)
         return ret;
   }

   if ((GetErrorMode() & SEM_NOGPFAULTERRORBOX) == 0)
   {
#ifdef __i386__
      PULONG Frame;
#endif
      PVOID StartAddr;
      CHAR szMod[128] = "";

      /* Print a stack trace. */
      DbgPrint("Unhandled exception\n");
      DbgPrint("ExceptionCode:    %8x\n", ExceptionInfo->ExceptionRecord->ExceptionCode);
      if ((NTSTATUS)ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION &&
          ExceptionInfo->ExceptionRecord->NumberParameters == 2)
      {
         DbgPrint("Faulting Address: %8x\n", ExceptionInfo->ExceptionRecord->ExceptionInformation[1]);
      }
      DbgPrint("Address:          %8x   %s\n",
         ExceptionInfo->ExceptionRecord->ExceptionAddress,
         _module_name_from_addr(ExceptionInfo->ExceptionRecord->ExceptionAddress, &StartAddr, szMod, sizeof(szMod)));
      _dump_context ( ExceptionInfo->ContextRecord );
#ifdef __i386__
      DbgPrint("Frames:\n");
      _SEH2_TRY
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
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         DbgPrint("<error dumping stack trace: 0x%x>\n", _SEH2_GetExceptionCode());
      }
      _SEH2_END;
#endif
   }

   /* Save exception code and address */
   ErrorParameters[0] = (ULONG)ExceptionInfo->ExceptionRecord->ExceptionCode;
   ErrorParameters[1] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionAddress;

   if ((NTSTATUS)ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION)
   {
       /* get the type of operation that caused the access violation */
       ErrorParameters[2] = ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
   }
   else
   {
       ErrorParameters[2] = ExceptionInfo->ExceptionRecord->ExceptionInformation[2];
   }

   /* Save faulting address */
   ErrorParameters[3] = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];

   /* Raise the harderror */
   ErrCode = NtRaiseHardError(STATUS_UNHANDLED_EXCEPTION | 0x10000000,
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

        /* Set the count of parameters */
        ExceptionRecord.NumberParameters = nNumberOfArguments;

        /* Loop each parameter */
        for (nNumberOfArguments = 0;
            (nNumberOfArguments < ExceptionRecord.NumberParameters);
            nNumberOfArguments ++)
        {
            /* Copy the exception information */
            ExceptionRecord.ExceptionInformation[nNumberOfArguments] =
                *lpArguments++;
        }
    }

    if (dwExceptionCode == 0xeedface)
    {
        DPRINT1("Delphi Exception at address: %p\n", ExceptionRecord.ExceptionInformation[0]);
        DPRINT1("Exception-Object: %p\n", ExceptionRecord.ExceptionInformation[1]);
        DPRINT1("Exception text: %s\n", ExceptionRecord.ExceptionInformation[2]);        
    }

    /* Raise the exception */
    RtlRaiseException(&ExceptionRecord);
}

/* EOF */
