/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library
 * PURPOSE:         User-Mode Exception Support
 * FILE:            lib/rtl/exception.c
 * PROGRAMERS:      Alex Ionescu (alex@relsoft.net)
 *                  David Welch <welch@cwcom.net>
 *                  Skywing <skywing@valhallalegends.com>
 *                  KJK::Hyperion <noog@libero.it>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/

PRTLP_UNHANDLED_EXCEPTION_FILTER RtlpUnhandledExceptionFilter;

/* FUNCTIONS ***************************************************************/

#if !defined(_M_IX86) && !defined(_M_AMD64)

/*
 * @implemented
 */
VOID
NTAPI
RtlRaiseException(IN PEXCEPTION_RECORD ExceptionRecord)
{
    CONTEXT Context;
    NTSTATUS Status;

    /* Capture the context */
    RtlCaptureContext(&Context);

    /* Save the exception address */
    ExceptionRecord->ExceptionAddress = _ReturnAddress();

    /* Write the context flag */
    Context.ContextFlags = CONTEXT_FULL;

    /* Check if user mode debugger is active */
    if (RtlpCheckForActiveDebugger())
    {
        /* Raise an exception immediately */
        Status = ZwRaiseException(ExceptionRecord, &Context, TRUE);
    }
    else
    {
        /* Dispatch the exception and check if we should continue */
        if (!RtlDispatchException(ExceptionRecord, &Context))
        {
            /* Raise the exception */
            Status = ZwRaiseException(ExceptionRecord, &Context, FALSE);
        }
        else
        {
            /* Continue, go back to previous context */
            Status = ZwContinue(&Context, FALSE);
        }
    }

    /* If we returned, raise a status */
    RtlRaiseStatus(Status);
}

#endif

#if !defined(_M_IX86)

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4717) // RtlRaiseStatus is recursive by design
#endif

/*
 * @implemented
 */
VOID
NTAPI
RtlRaiseStatus(IN NTSTATUS Status)
{
    EXCEPTION_RECORD ExceptionRecord;
    CONTEXT Context;

     /* Capture the context */
    RtlCaptureContext(&Context);

    /* Create an exception record */
    ExceptionRecord.ExceptionAddress = _ReturnAddress();
    ExceptionRecord.ExceptionCode  = Status;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.NumberParameters = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

    /* Write the context flag */
    Context.ContextFlags = CONTEXT_FULL;

    /* Check if user mode debugger is active */
    if (RtlpCheckForActiveDebugger())
    {
        /* Raise an exception immediately */
        ZwRaiseException(&ExceptionRecord, &Context, TRUE);
    }
    else
    {
        /* Dispatch the exception */
        RtlDispatchException(&ExceptionRecord, &Context);

        /* Raise exception if we got here */
        Status = ZwRaiseException(&ExceptionRecord, &Context, FALSE);
    }

    /* If we returned, raise a status */
    RtlRaiseStatus(Status);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif

/*
 * @implemented
 */
USHORT
NTAPI
RtlCaptureStackBackTrace(IN ULONG FramesToSkip,
                         IN ULONG FramesToCapture,
                         OUT PVOID *BackTrace,
                         OUT PULONG BackTraceHash OPTIONAL)
{
    PVOID Frames[2 * 64];
    ULONG FrameCount;
    ULONG Hash = 0, i;

    /* Skip a frame for the caller */
    FramesToSkip++;

    /* Don't go past the limit */
    if ((FramesToCapture + FramesToSkip) >= 128) return 0;

    /* Do the back trace */
    FrameCount = RtlWalkFrameChain(Frames, FramesToCapture + FramesToSkip, 0);

    /* Make sure we're not skipping all of them */
    if (FrameCount <= FramesToSkip) return 0;

    /* Loop all the frames */
    for (i = 0; i < FramesToCapture; i++)
    {
        /* Don't go past the limit */
        if ((FramesToSkip + i) >= FrameCount) break;

        /* Save this entry and hash it */
        BackTrace[i] = Frames[FramesToSkip + i];
        Hash += PtrToUlong(BackTrace[i]);
    }

    /* Write the hash */
    if (BackTraceHash) *BackTraceHash = Hash;

    /* Clear the other entries and return count */
    RtlFillMemoryUlong(Frames, 128, 0);
    return (USHORT)i;
}

/*
* Private helper function to lookup the module name from a given address.
* The address can point to anywhere within the module.
*/
static const char*
    _module_name_from_addr(const void* addr, void **module_start_addr,
    char* psz, size_t nChars)
{
#if 0
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(addr, &mbi, sizeof(mbi)) != sizeof(mbi) ||
        !GetModuleFileNameA((HMODULE) mbi.AllocationBase, psz, nChars))
    {
        psz[0] = '\0';
        *module_start_addr = 0;
    }
    else
    {
        *module_start_addr = (void *) mbi.AllocationBase;
    }
    return psz;
#else
    psz[0] = '\0';
    *module_start_addr = 0;
    return psz;
#endif
}


static VOID
    _dump_context(PCONTEXT pc)
{
#ifdef _M_IX86
    /*
    * Print out the CPU registers
    */
    DbgPrint("CS:EIP %x:%x\n", pc->SegCs & 0xffff, pc->Eip);
    DbgPrint("DS %x ES %x FS %x GS %x\n", pc->SegDs & 0xffff, pc->SegEs & 0xffff,
        pc->SegFs & 0xffff, pc->SegGs & 0xfff);
    DbgPrint("EAX: %.8x   EBX: %.8x   ECX: %.8x\n", pc->Eax, pc->Ebx, pc->Ecx);
    DbgPrint("EDX: %.8x   EBP: %.8x   ESI: %.8x   ESP: %.8x\n", pc->Edx,
        pc->Ebp, pc->Esi, pc->Esp);
    DbgPrint("EDI: %.8x   EFLAGS: %.8x\n", pc->Edi, pc->EFlags);
#elif defined(_M_AMD64)
    DbgPrint("CS:RIP %x:%I64x\n", pc->SegCs & 0xffff, pc->Rip);
    DbgPrint("DS %x ES %x FS %x GS %x\n", pc->SegDs & 0xffff, pc->SegEs & 0xffff,
        pc->SegFs & 0xffff, pc->SegGs & 0xfff);
    DbgPrint("RAX: %I64x   RBX: %I64x   RCX: %I64x RDI: %I64x\n", pc->Rax, pc->Rbx, pc->Rcx, pc->Rdi);
    DbgPrint("RDX: %I64x   RBP: %I64x   RSI: %I64x   RSP: %I64x\n", pc->Rdx, pc->Rbp, pc->Rsi, pc->Rsp);
    DbgPrint("R8: %I64x   R9: %I64x   R10: %I64x   R11: %I64x\n", pc->R8, pc->R9, pc->R10, pc->R11);
    DbgPrint("R12: %I64x   R13: %I64x   R14: %I64x   R15: %I64x\n", pc->R12, pc->R13, pc->R14, pc->R15);
    DbgPrint("EFLAGS: %.8x\n", pc->EFlags);
#elif defined(_M_ARM)
    DbgPrint("Pc: %lx   Lr: %lx   Sp: %lx    Cpsr: %lx\n", pc->Pc, pc->Lr, pc->Sp, pc->Cpsr);
    DbgPrint("R0: %lx   R1: %lx   R2: %lx    R3: %lx\n", pc->R0, pc->R1, pc->R2, pc->R3);
    DbgPrint("R4: %lx   R5: %lx   R6: %lx    R7: %lx\n", pc->R4, pc->R5, pc->R6, pc->R7);
    DbgPrint("R8: %lx   R9: %lx  R10: %lx   R11: %lx\n", pc->R8, pc->R9, pc->R10, pc->R11);
    DbgPrint("R12: %lx\n", pc->R12);
#else
#pragma message ("Unknown architecture")
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

    if ((NTSTATUS) ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION &&
        ExceptionRecord->NumberParameters == 2)
    {
        DbgPrint("Faulting Address: %8x\n", ExceptionRecord->ExceptionInformation[1]);
    }

    /* Trace the wine special error and show the modulename and functionname */
    if (ExceptionRecord->ExceptionCode == 0x80000100 /* EXCEPTION_WINE_STUB */ &&
        ExceptionRecord->NumberParameters == 2)
    {
        DbgPrint("Missing function: %s!%s\n", (PSZ)ExceptionRecord->ExceptionInformation[0], (PSZ)ExceptionRecord->ExceptionInformation[1]);
    }

    _dump_context(ContextRecord);
    _module_name_from_addr(ExceptionRecord->ExceptionAddress, &StartAddr, szMod, sizeof(szMod));
    DbgPrint("Address:\n   %8x+%-8x   %s\n",
        (PVOID) StartAddr,
        (ULONG_PTR) ExceptionRecord->ExceptionAddress - (ULONG_PTR) StartAddr,
        szMod);
#ifdef _M_IX86
    DbgPrint("Frames:\n");

    _SEH2_TRY
    {
        UINT i;
        PULONG Frame = (PULONG) ContextRecord->Ebp;

        for (i = 0; Frame[1] != 0 && Frame[1] != 0xdeadbeef && i < 128; i++)
        {
            //if (IsBadReadPtr((PVOID) Frame[1], 4))
            if (Frame[1] == 0)
            {
                DbgPrint("   %8x%9s   %s\n", Frame[1], "<invalid address>", " ");
            }
            else
            {
                _module_name_from_addr((const void*) Frame[1], &StartAddr,
                    szMod, sizeof(szMod));
                DbgPrint("   %8x+%-8x   %s\n",
                    (PVOID) StartAddr,
                    (ULONG_PTR) Frame[1] - (ULONG_PTR) StartAddr,
                    szMod);
            }

            if (Frame[0] == 0) break;
            //if (IsBadReadPtr((PVOID) Frame[0], sizeof(*Frame) * 2))
                //break;

            Frame = (PULONG) Frame[0];
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DbgPrint("<error dumping stack trace: 0x%x>\n", _SEH2_GetExceptionCode());
    }
    _SEH2_END;
#endif
}


/*
 * @unimplemented
 */
LONG
NTAPI
RtlUnhandledExceptionFilter(IN struct _EXCEPTION_POINTERS* ExceptionInfo)
{
    /* This is used by the security cookie checks, and also called externally */
    UNIMPLEMENTED;
    PrintStackTrace(ExceptionInfo);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
LONG
NTAPI
RtlUnhandledExceptionFilter2(
    _In_ PEXCEPTION_POINTERS ExceptionInfo,
    _In_ ULONG Flags)
{
    /* This is used by the security cookie checks, and also called externally */
    UNIMPLEMENTED;
    PrintStackTrace(ExceptionInfo);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlSetUnhandledExceptionFilter(IN PRTLP_UNHANDLED_EXCEPTION_FILTER TopLevelExceptionFilter)
{
    /* Set the filter which is used by the CriticalSection package */
    RtlpUnhandledExceptionFilter = RtlEncodePointer(TopLevelExceptionFilter);
}
