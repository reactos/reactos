/*
 * Unit test suite for ntdll exceptions
 *
 * Copyright 2005 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <setjmp.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "winuser.h"
#include "winternl.h"
#include "rtlsupportapi.h"
#include "ddk/wdm.h"
#include "excpt.h"
#include "wine/test.h"
#include "intrin.h"
#ifdef __REACTOS__
#include <wine/exception.h>
#ifdef _M_AMD64
USHORT __readsegds(void);
USHORT __readseges(void);
USHORT __readsegfs(void);
USHORT __readseggs(void);
USHORT __readsegss(void);
void __cld(void);
void Call_NtRaiseException(PEXCEPTION_RECORD ExceptionRecord, PCONTEXT Context, BOOLEAN FirstChance, PVOID pNtRaiseException);
#endif // _M_AMD64
#endif // __REACTOS__

static void *code_mem;
static HMODULE hntdll;
static BOOL is_arm64ec;

static NTSTATUS  (WINAPI *pNtGetContextThread)(HANDLE,CONTEXT*);
static NTSTATUS  (WINAPI *pNtSetContextThread)(HANDLE,CONTEXT*);
static NTSTATUS  (WINAPI *pNtQueueApcThread)(HANDLE handle, PNTAPCFUNC func,
        ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3);
static NTSTATUS  (WINAPI *pNtContinueEx)(CONTEXT*,KCONTINUE_ARGUMENT*);
static NTSTATUS  (WINAPI *pRtlRaiseException)(EXCEPTION_RECORD *rec);
static PVOID     (WINAPI *pRtlUnwind)(PVOID, PVOID, PEXCEPTION_RECORD, PVOID);
static VOID      (WINAPI *pRtlCaptureContext)(CONTEXT*);
static PVOID     (WINAPI *pRtlAddVectoredExceptionHandler)(ULONG first, PVECTORED_EXCEPTION_HANDLER func);
static ULONG     (WINAPI *pRtlRemoveVectoredExceptionHandler)(PVOID handler);
static PVOID     (WINAPI *pRtlAddVectoredContinueHandler)(ULONG first, PVECTORED_EXCEPTION_HANDLER func);
static ULONG     (WINAPI *pRtlRemoveVectoredContinueHandler)(PVOID handler);
static void      (WINAPI *pRtlSetUnhandledExceptionFilter)(PRTL_EXCEPTION_FILTER filter);
static ULONG64   (WINAPI *pRtlGetEnabledExtendedFeatures)(ULONG64);
static NTSTATUS  (WINAPI *pRtlGetExtendedContextLength)(ULONG context_flags, ULONG *length);
static NTSTATUS  (WINAPI *pRtlGetExtendedContextLength2)(ULONG context_flags, ULONG *length, ULONG64 compaction_mask);
static NTSTATUS  (WINAPI *pRtlInitializeExtendedContext)(void *context, ULONG context_flags, CONTEXT_EX **context_ex);
static NTSTATUS  (WINAPI *pRtlInitializeExtendedContext2)(void *context, ULONG context_flags, CONTEXT_EX **context_ex,
        ULONG64 compaction_mask);
static NTSTATUS  (WINAPI *pRtlCopyContext)(CONTEXT *dst, DWORD context_flags, CONTEXT *src);
static NTSTATUS  (WINAPI *pRtlCopyExtendedContext)(CONTEXT_EX *dst, ULONG context_flags, CONTEXT_EX *src);
static void *    (WINAPI *pRtlLocateExtendedFeature)(CONTEXT_EX *context_ex, ULONG feature_id, ULONG *length);
static void *    (WINAPI *pRtlLocateLegacyContext)(CONTEXT_EX *context_ex, ULONG *length);
static void      (WINAPI *pRtlSetExtendedFeaturesMask)(CONTEXT_EX *context_ex, ULONG64 feature_mask);
static ULONG64   (WINAPI *pRtlGetExtendedFeaturesMask)(CONTEXT_EX *context_ex);
static void *    (WINAPI *pRtlPcToFileHeader)(PVOID pc, PVOID *address);
static void      (WINAPI *pRtlGetCallersAddress)(void**,void**);
static NTSTATUS  (WINAPI *pNtRaiseException)(EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance);
static NTSTATUS  (WINAPI *pNtReadVirtualMemory)(HANDLE, const void*, void*, SIZE_T, SIZE_T*);
static NTSTATUS  (WINAPI *pNtTerminateProcess)(HANDLE handle, LONG exit_code);
static NTSTATUS  (WINAPI *pNtQueryInformationThread)(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
static NTSTATUS  (WINAPI *pNtSetInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG);
static BOOL      (WINAPI *pIsWow64Process)(HANDLE, PBOOL);
static NTSTATUS  (WINAPI *pNtClose)(HANDLE);
static NTSTATUS  (WINAPI *pNtSuspendProcess)(HANDLE process);
static NTSTATUS  (WINAPI *pNtResumeProcess)(HANDLE process);
static BOOL      (WINAPI *pInitializeContext)(void *buffer, DWORD context_flags, CONTEXT **context,
        DWORD *length);
static BOOL      (WINAPI *pInitializeContext2)(void *buffer, DWORD context_flags, CONTEXT **context,
        DWORD *length, ULONG64 compaction_mask);
static void *    (WINAPI *pLocateXStateFeature)(CONTEXT *context, DWORD feature_id, DWORD *length);
static BOOL      (WINAPI *pSetXStateFeaturesMask)(CONTEXT *context, DWORD64 feature_mask);
static BOOL      (WINAPI *pGetXStateFeaturesMask)(CONTEXT *context, DWORD64 *feature_mask);
static BOOL      (WINAPI *pWaitForDebugEventEx)(DEBUG_EVENT *, DWORD);
#ifndef __i386__
static VOID      (WINAPI *pRtlUnwindEx)(VOID*, VOID*, EXCEPTION_RECORD*, VOID*, CONTEXT*, UNWIND_HISTORY_TABLE*);
static BOOLEAN   (CDECL  *pRtlAddFunctionTable)(RUNTIME_FUNCTION*, DWORD, DWORD64);
static BOOLEAN   (CDECL  *pRtlDeleteFunctionTable)(RUNTIME_FUNCTION*);
static VOID      (CDECL  *pRtlRestoreContext)(CONTEXT*, EXCEPTION_RECORD*);
static NTSTATUS  (WINAPI *pRtlGetNativeSystemInformation)(SYSTEM_INFORMATION_CLASS,void*,ULONG,ULONG*);
#endif

static void *pKiUserApcDispatcher;
static void *pKiUserCallbackDispatcher;
static void *pKiUserExceptionDispatcher;

#define RTL_UNLOAD_EVENT_TRACE_NUMBER 64

typedef struct _RTL_UNLOAD_EVENT_TRACE
{
    void *BaseAddress;
    SIZE_T SizeOfImage;
    ULONG Sequence;
    ULONG TimeDateStamp;
    ULONG CheckSum;
    WCHAR ImageName[32];
} RTL_UNLOAD_EVENT_TRACE, *PRTL_UNLOAD_EVENT_TRACE;

static RTL_UNLOAD_EVENT_TRACE *(WINAPI *pRtlGetUnloadEventTrace)(void);
static void (WINAPI *pRtlGetUnloadEventTraceEx)(ULONG **element_size, ULONG **element_count, void **event_trace);

#if defined(__x86_64__)

typedef union _UNWIND_CODE
{
    struct
    {
        BYTE CodeOffset;
        BYTE UnwindOp : 4;
        BYTE OpInfo   : 4;
    } s;
    USHORT FrameOffset;
} UNWIND_CODE;

typedef struct _UNWIND_INFO
{
    BYTE Version       : 3;
    BYTE Flags         : 5;
    BYTE SizeOfProlog;
    BYTE CountOfCodes;
    BYTE FrameRegister : 4;
    BYTE FrameOffset   : 4;
    UNWIND_CODE UnwindCode[1]; /* actually CountOfCodes (aligned) */
/*
 *  union
 *  {
 *      OPTIONAL ULONG ExceptionHandler;
 *      OPTIONAL ULONG FunctionEntry;
 *  };
 *  OPTIONAL ULONG ExceptionData[];
 */
} UNWIND_INFO;

static EXCEPTION_DISPOSITION (WINAPI *p__C_specific_handler)(EXCEPTION_RECORD*, ULONG64, CONTEXT*, DISPATCHER_CONTEXT*);
static NTSTATUS  (WINAPI *pRtlWow64GetThreadContext)(HANDLE, WOW64_CONTEXT *);
static NTSTATUS  (WINAPI *pRtlWow64SetThreadContext)(HANDLE, const WOW64_CONTEXT *);
static NTSTATUS  (WINAPI *pRtlWow64GetCpuAreaInfo)(WOW64_CPURESERVED*,ULONG,WOW64_CPU_AREA_INFO*);
#endif

enum debugger_stages
{
    STAGE_RTLRAISE_NOT_HANDLED = 1,
    STAGE_RTLRAISE_HANDLE_LAST_CHANCE,
    STAGE_OUTPUTDEBUGSTRINGA_CONTINUE,
    STAGE_OUTPUTDEBUGSTRINGA_NOT_HANDLED,
    STAGE_OUTPUTDEBUGSTRINGW_CONTINUE,
    STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED,
    STAGE_RIPEVENT_CONTINUE,
    STAGE_RIPEVENT_NOT_HANDLED,
    STAGE_SERVICE_CONTINUE,
    STAGE_SERVICE_NOT_HANDLED,
    STAGE_BREAKPOINT_CONTINUE,
    STAGE_BREAKPOINT_NOT_HANDLED,
    STAGE_EXCEPTION_INVHANDLE_CONTINUE,
    STAGE_EXCEPTION_INVHANDLE_NOT_HANDLED,
    STAGE_NO_EXCEPTION_INVHANDLE_NOT_HANDLED,
    STAGE_XSTATE,
    STAGE_XSTATE_LEGACY_SSE,
    STAGE_SEGMENTS,
};

static int      my_argc;
static char**   my_argv;
static BOOL     is_wow64;
static BOOL old_wow64;  /* Wine old-style wow64 */
static UINT apc_count;
static BOOL have_vectored_api;
static enum debugger_stages test_stage;

static void CALLBACK apc_func( ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3 )
{
    ok( arg1 == 0x1234 + apc_count, "wrong arg1 %Ix\n", arg1 );
    ok( arg2 == 0x5678, "wrong arg2 %Ix\n", arg2 );
    ok( arg3 == 0xdeadbeef, "wrong arg3 %Ix\n", arg3 );
    apc_count++;
}

#if defined(__i386__) || defined(__x86_64__)
static void test_debugger_xstate(HANDLE thread, CONTEXT *ctx, enum debugger_stages stage)
{
    char context_buffer[sizeof(CONTEXT) + sizeof(CONTEXT_EX) + sizeof(XSTATE) + 3072];
    CONTEXT_EX *c_ex;
    NTSTATUS status;
    YMMCONTEXT *ymm;
    CONTEXT *xctx;
    DWORD length;
    XSTATE *xs;
    M128A *xmm;
    BOOL bret;

    if (!pRtlGetEnabledExtendedFeatures || !pRtlGetEnabledExtendedFeatures(1 << XSTATE_AVX))
        return;

    if (stage == STAGE_XSTATE)
        return;

    length = sizeof(context_buffer);
    bret = pInitializeContext(context_buffer, ctx->ContextFlags | CONTEXT_XSTATE, &xctx, &length);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

    ymm = pLocateXStateFeature(xctx, XSTATE_AVX, &length);
    ok(!!ymm, "Got zero ymm.\n");
    memset(ymm, 0xcc, sizeof(*ymm));

    xmm = pLocateXStateFeature(xctx, XSTATE_LEGACY_SSE, &length);
    ok(length == sizeof(*xmm) * (sizeof(void *) == 8 ? 16 : 8), "Got unexpected length %#lx.\n", length);
    ok(!!xmm, "Got zero xmm.\n");
    memset(xmm, 0xcc, length);

    status = pNtGetContextThread(thread, xctx);
    ok(!status, "NtSetContextThread failed with 0x%lx\n", status);

    c_ex = (CONTEXT_EX *)(xctx + 1);
    xs = (XSTATE *)((char *)c_ex + c_ex->XState.Offset);
    ok((xs->Mask & 7) == 4 || broken(!xs->Mask) /* Win7 */,
            "Got unexpected xs->Mask %s.\n", wine_dbgstr_longlong(xs->Mask));

    ok(xmm[0].Low == 0x200000001, "Got unexpected data %s.\n", wine_dbgstr_longlong(xmm[0].Low));
    ok(xmm[0].High == 0x400000003, "Got unexpected data %s.\n", wine_dbgstr_longlong(xmm[0].High));

    ok(ymm->Ymm0.Low == 0x600000005 || broken(!xs->Mask && ymm->Ymm0.Low == 0xcccccccccccccccc) /* Win7 */,
            "Got unexpected data %s.\n", wine_dbgstr_longlong(ymm->Ymm0.Low));
    ok(ymm->Ymm0.High == 0x800000007 || broken(!xs->Mask && ymm->Ymm0.High == 0xcccccccccccccccc) /* Win7 */,
            "Got unexpected data %s.\n", wine_dbgstr_longlong(ymm->Ymm0.High));

    xmm = pLocateXStateFeature(ctx, XSTATE_LEGACY_SSE, &length);
    ok(!!xmm, "Got zero xmm.\n");

    xmm[0].Low = 0x2828282828282828;
    xmm[0].High = xmm[0].Low;
    ymm->Ymm0.Low = 0x4848484848484848;
    ymm->Ymm0.High = ymm->Ymm0.Low;

    status = pNtSetContextThread(thread, xctx);
    ok(!status, "NtSetContextThread failed with 0x%lx\n", status);
}

#define check_context_exception_request( a, b ) check_context_exception_request_( a, b, __LINE__ )
static void check_context_exception_request_( DWORD flags, BOOL hardware_exception, unsigned int line )
{
    static const DWORD exception_reporting_flags = CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING
                                                   | CONTEXT_EXCEPTION_ACTIVE | CONTEXT_SERVICE_ACTIVE;
    DWORD expected_flags = CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING;

    if (!(flags & CONTEXT_EXCEPTION_REPORTING)) return;
    expected_flags |= hardware_exception ? CONTEXT_EXCEPTION_ACTIVE : CONTEXT_SERVICE_ACTIVE;
    ok_(__FILE__, line)( (flags & exception_reporting_flags) == expected_flags, "got %#lx, expected %#lx.\n",
                         flags, expected_flags );
}

static BOOL test_hwbpt_in_syscall_trap;

static LONG WINAPI test_hwbpt_in_syscall_handler( EXCEPTION_POINTERS *eptr )
{
    EXCEPTION_RECORD *rec = eptr->ExceptionRecord;

    test_hwbpt_in_syscall_trap = TRUE;
    ok(rec->ExceptionCode == EXCEPTION_SINGLE_STEP, "got %#lx.\n", rec->ExceptionCode);
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void test_hwbpt_in_syscall(void)
{
    TEB *teb = NtCurrentTeb();
    NTSTATUS status;
    void *handler;
    CONTEXT c;
    DWORD ind;
    BOOL bret;

    ind = TlsAlloc();
    ok(ind < ARRAY_SIZE(teb->TlsSlots), "got %lu.\n", ind);
    handler = AddVectoredExceptionHandler(TRUE, test_hwbpt_in_syscall_handler);
    memset(&c, 0, sizeof(c));
    c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    c.Dr0 = (ULONG_PTR)&teb->TlsSlots[ind];
    c.Dr7 = 3 | (3 << 16) | (3 << 18); /* read / write 4 byte breakpoint. */
    bret = SetThreadContext(GetCurrentThread(), &c);
    ok(bret, "got error %lu.\n", GetLastError());
    test_hwbpt_in_syscall_trap = FALSE;
    teb->TlsSlots[ind] = (void *)0xdeadbeef;
    ok(test_hwbpt_in_syscall_trap, "expected trap.\n");

    test_hwbpt_in_syscall_trap = FALSE;
    status = NtSetInformationThread(GetCurrentThread(), ThreadZeroTlsCell, &ind, sizeof(ind));
    ok(!status, "got %#lx.\n", status);
    ok(!test_hwbpt_in_syscall_trap, "got trap.\n");
    c.Dr7 = 0;
    bret = SetThreadContext(GetCurrentThread(), &c);
    ok(bret, "got error %lu.\n", GetLastError());
    ok(!teb->TlsSlots[ind], "got %p.\n", teb->TlsSlots[ind]);
    RemoveVectoredExceptionHandler(handler);
    TlsFree(ind);
}
#endif

#ifdef __i386__

#ifndef __WINE_WINTRNL_H
#define ProcessExecuteFlags 0x22
#define MEM_EXECUTE_OPTION_DISABLE   0x01
#define MEM_EXECUTE_OPTION_ENABLE    0x02
#define MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION 0x04
#define MEM_EXECUTE_OPTION_PERMANENT 0x08
#endif

/* Test various instruction combinations that cause a protection fault on the i386,
 * and check what the resulting exception looks like.
 */

static const struct exception
{
    BYTE     code[18];      /* asm code */
    BYTE     offset;        /* offset of faulting instruction */
    BYTE     length;        /* length of faulting instruction */
    BOOL     wow64_broken;  /* broken on Wow64, should be skipped */
    NTSTATUS status;        /* expected status code */
    DWORD    nb_params;     /* expected number of parameters */
    DWORD    params[4];     /* expected parameters */
    NTSTATUS alt_status;    /* alternative status code */
    DWORD    alt_nb_params; /* alternative number of parameters */
    DWORD    alt_params[4]; /* alternative parameters */
} exceptions[] =
{
/* 0 */
    /* test some privileged instructions */
    { { 0xfb, 0xc3 },  /* 0: sti; ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6c, 0xc3 },  /* 1: insb (%dx); ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6d, 0xc3 },  /* 2: insl (%dx); ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6e, 0xc3 },  /* 3: outsb (%dx); ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6f, 0xc3 },  /* 4: outsl (%dx); ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 5 */
    { { 0xe4, 0x11, 0xc3 },  /* 5: inb $0x11,%al; ret */
      0, 2, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe5, 0x11, 0xc3 },  /* 6: inl $0x11,%eax; ret */
      0, 2, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe6, 0x11, 0xc3 },  /* 7: outb %al,$0x11; ret */
      0, 2, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe7, 0x11, 0xc3 },  /* 8: outl %eax,$0x11; ret */
      0, 2, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xed, 0xc3 },  /* 9: inl (%dx),%eax; ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 10 */
    { { 0xee, 0xc3 },  /* 10: outb %al,(%dx); ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xef, 0xc3 },  /* 11: outl %eax,(%dx); ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xf4, 0xc3 },  /* 12: hlt; ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xfa, 0xc3 },  /* 13: cli; ret */
      0, 1, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test long jump to invalid selector */
    { { 0xea, 0, 0, 0, 0, 0, 0, 0xc3 },  /* 14: ljmp $0,$0; ret */
      0, 7, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

/* 15 */
    /* test iret to invalid selector */
    { { 0x6a, 0x00, 0x6a, 0x00, 0x6a, 0x00, 0xcf, 0x83, 0xc4, 0x0c, 0xc3 },
      /* 15: pushl $0; pushl $0; pushl $0; iret; addl $12,%esp; ret */
      6, 1, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test loading an invalid selector */
    { { 0xb8, 0xef, 0xbe, 0x00, 0x00, 0x8e, 0xe8, 0xc3 },  /* 16: mov $beef,%ax; mov %ax,%gs; ret */
      5, 2, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xbee8 } }, /* 0xbee8 or 0xffffffff */

    /* test accessing a zero selector (%es broken on Wow64) */
    { { 0x06, 0x31, 0xc0, 0x8e, 0xc0, 0x26, 0xa1, 0, 0, 0, 0, 0x07, 0xc3 },
       /* push %es; xor %eax,%eax; mov %ax,%es; mov %es:(0),%ax; pop %es; ret */
      5, 6, TRUE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },
    { { 0x0f, 0xa8, 0x31, 0xc0, 0x8e, 0xe8, 0x65, 0xa1, 0, 0, 0, 0, 0x0f, 0xa9, 0xc3 },
      /* push %gs; xor %eax,%eax; mov %ax,%gs; mov %gs:(0),%ax; pop %gs; ret */
      6, 6, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test moving %cs -> %ss */
    { { 0x0e, 0x17, 0x58, 0xc3 },  /* pushl %cs; popl %ss; popl %eax; ret */
      1, 1, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

/* 20 */
    /* test overlong instruction (limit is 15 bytes, 5 on Win7) */
    { { 0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0xfa,0xc3 },
      0, 16, TRUE, STATUS_ILLEGAL_INSTRUCTION, 0, { 0 },
      STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },
    { { 0x64,0x64,0x64,0x64,0xfa,0xc3 },
      0, 5, TRUE, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test invalid interrupt */
    { { 0xcd, 0xff, 0xc3 },   /* int $0xff; ret */
      0, 2, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test moves to/from Crx */
    { { 0x0f, 0x20, 0xc0, 0xc3 },  /* movl %cr0,%eax; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x20, 0xe0, 0xc3 },  /* movl %cr4,%eax; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 25 */
    { { 0x0f, 0x22, 0xc0, 0xc3 },  /* movl %eax,%cr0; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x22, 0xe0, 0xc3 },  /* movl %eax,%cr4; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test moves to/from Drx */
    { { 0x0f, 0x21, 0xc0, 0xc3 },  /* movl %dr0,%eax; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x21, 0xc8, 0xc3 },  /* movl %dr1,%eax; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x21, 0xf8, 0xc3 },  /* movl %dr7,%eax; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 30 */
    { { 0x0f, 0x23, 0xc0, 0xc3 },  /* movl %eax,%dr0; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x23, 0xc8, 0xc3 },  /* movl %eax,%dr1; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x23, 0xf8, 0xc3 },  /* movl %eax,%dr7; ret */
      0, 3, FALSE, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test memory reads */
    { { 0xa1, 0xfc, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffc,%eax; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffc } },
    { { 0xa1, 0xfd, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffd,%eax; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffd } },
/* 35 */
    { { 0xa1, 0xfe, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffe,%eax; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffe } },
    { { 0xa1, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xffffffff,%eax; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test memory writes */
    { { 0xa3, 0xfc, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffc; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffc } },
    { { 0xa3, 0xfd, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffd; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffd } },
    { { 0xa3, 0xfe, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffe; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffe } },
/* 40 */
    { { 0xa3, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xffffffff; ret */
      0, 5, FALSE, STATUS_ACCESS_VIOLATION, 2, { 1, 0xffffffff } },

    /* test exception with cleared %ds and %es (broken on Wow64) */
    { { 0x1e, 0x06, 0x31, 0xc0, 0x8e, 0xd8, 0x8e, 0xc0, 0xfa, 0x07, 0x1f, 0xc3 },
          /* push %ds; push %es; xorl %eax,%eax; mov %ax,%ds; mov %ax,%es; cli; pop %es; pop %ds; ret */
      8, 1, TRUE, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    { { 0xf1, 0x90, 0xc3 },  /* icebp; nop; ret */
      1, 1, FALSE, STATUS_SINGLE_STEP, 0 },
    { { 0xb8, 0xb8, 0xb8, 0xb8, 0xb8,          /* mov $0xb8b8b8b8, %eax */
        0xb9, 0xb9, 0xb9, 0xb9, 0xb9,          /* mov $0xb9b9b9b9, %ecx */
        0xba, 0xba, 0xba, 0xba, 0xba,          /* mov $0xbabababa, %edx */
        0xcd, 0x2d, 0xc3 },                    /* int $0x2d; ret */
      17, 0, FALSE, STATUS_BREAKPOINT, 3, { 0xb8b8b8b8, 0xb9b9b9b9, 0xbabababa } },
};

static int got_exception;

static void run_exception_test(void *handler, const void* context,
                               const void *code, unsigned int code_size,
                               DWORD access)
{
    struct {
        EXCEPTION_REGISTRATION_RECORD frame;
        const void *context;
    } exc_frame;
    void (*func)(void) = code_mem;
    DWORD oldaccess, oldaccess2;

    exc_frame.frame.Handler = handler;
    exc_frame.frame.Prev = NtCurrentTeb()->Tib.ExceptionList;
    exc_frame.context = context;

    memcpy(code_mem, code, code_size);
    if(access)
        VirtualProtect(code_mem, code_size, access, &oldaccess);

    NtCurrentTeb()->Tib.ExceptionList = &exc_frame.frame;
    func();
    NtCurrentTeb()->Tib.ExceptionList = exc_frame.frame.Prev;

    if(access)
        VirtualProtect(code_mem, code_size, oldaccess, &oldaccess2);
}

static LONG CALLBACK rtlraiseexception_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT context = ExceptionInfo->ContextRecord;
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;

    ok(rec->ExceptionAddress == (char *)code_mem + 0xb
            || broken(rec->ExceptionAddress == code_mem || !rec->ExceptionAddress) /* 2008 */,
            "ExceptionAddress at %p instead of %p\n", rec->ExceptionAddress, (char *)code_mem + 0xb);

    if (NtCurrentTeb()->Peb->BeingDebugged)
        ok((void *)context->Eax == pRtlRaiseException ||
           broken( is_wow64 && context->Eax == 0xf00f00f1 ), /* broken on vista */
           "debugger managed to modify Eax to %lx should be %p\n",
           context->Eax, pRtlRaiseException);

    /* check that context.Eip is fixed up only for EXCEPTION_BREAKPOINT
     * even if raised by RtlRaiseException
     */
    if(rec->ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        ok(context->Eip == (DWORD)code_mem + 0xa ||
           (is_wow64 && context->Eip == (DWORD)code_mem + 0xb) ||
           broken(context->Eip == (DWORD)code_mem + 0xd) /* w2008 */,
           "Eip at %lx instead of %lx or %lx\n", context->Eip,
           (DWORD)code_mem + 0xa, (DWORD)code_mem + 0xb);
    }
    else
    {
        ok(context->Eip == (DWORD)code_mem + 0xb ||
           broken(context->Eip == (DWORD)code_mem + 0xd) /* w2008 */,
           "Eip at %lx instead of %lx\n", context->Eip, (DWORD)code_mem + 0xb);
    }

    /* test if context change is preserved from vectored handler to stack handlers */
    context->Eax = 0xf00f00f0;
    return EXCEPTION_CONTINUE_SEARCH;
}

static DWORD rtlraiseexception_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                      CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    trace( "exception: %08lx flags:%lx addr:%p context: Eip:%lx\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, context->Eip );

    ok(rec->ExceptionAddress == (char *)code_mem + 0xb
            || broken(rec->ExceptionAddress == code_mem || !rec->ExceptionAddress) /* 2008 */,
            "ExceptionAddress at %p instead of %p\n", rec->ExceptionAddress, (char *)code_mem + 0xb);

    ok( context->ContextFlags == CONTEXT_ALL || context->ContextFlags == (CONTEXT_ALL | CONTEXT_XSTATE) ||
        broken(context->ContextFlags == CONTEXT_FULL),  /* win2003 */
        "wrong context flags %lx\n", context->ContextFlags );

    /* check that context.Eip is fixed up only for EXCEPTION_BREAKPOINT
     * even if raised by RtlRaiseException
     */
    if(rec->ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        ok(context->Eip == (DWORD)code_mem + 0xa ||
           (is_wow64 && context->Eip == (DWORD)code_mem + 0xb) ||
           broken(context->Eip == (DWORD)code_mem + 0xd) /* w2008 */,
           "Eip at %lx instead of %lx or %lx\n", context->Eip,
           (DWORD)code_mem + 0xa, (DWORD)code_mem + 0xb);
    }
    else
    {
        ok(context->Eip == (DWORD)code_mem + 0xb ||
           broken(context->Eip == (DWORD)code_mem + 0xd) /* w2008 */,
           "Eip at %lx instead of %lx\n", context->Eip, (DWORD)code_mem + 0xb);
    }

    if(have_vectored_api)
        ok(context->Eax == 0xf00f00f0, "Eax is %lx, should have been set to 0xf00f00f0 in vectored handler\n",
           context->Eax);

    /* give the debugger a chance to examine the state a second time */
    /* without the exception handler changing Eip */
    if (test_stage == STAGE_RTLRAISE_HANDLE_LAST_CHANCE)
        return ExceptionContinueSearch;

    /* Eip in context is decreased by 1
     * Increase it again, else execution will continue in the middle of an instruction */
    if(rec->ExceptionCode == EXCEPTION_BREAKPOINT && (context->Eip == (DWORD)code_mem + 0xa))
        context->Eip += 1;
    return ExceptionContinueExecution;
}


static const BYTE call_one_arg_code[] = {
        0x8b, 0x44, 0x24, 0x08, /* mov 0x8(%esp),%eax */
        0x50,                   /* push %eax */
        0x8b, 0x44, 0x24, 0x08, /* mov 0x8(%esp),%eax */
        0xff, 0xd0,             /* call *%eax */
        0x90,                   /* nop */
        0x90,                   /* nop */
        0x90,                   /* nop */
        0x90,                   /* nop */
        0xc3,                   /* ret */
};


static void run_rtlraiseexception_test(DWORD exceptioncode)
{
    EXCEPTION_REGISTRATION_RECORD frame;
    EXCEPTION_RECORD record;
    PVOID vectored_handler = NULL;

    void (*func)(void* function, EXCEPTION_RECORD* record) = code_mem;

    record.ExceptionCode = exceptioncode;
    record.ExceptionFlags = 0;
    record.ExceptionRecord = NULL;
    record.ExceptionAddress = NULL; /* does not matter, copied return address */
    record.NumberParameters = 0;

    frame.Handler = rtlraiseexception_handler;
    frame.Prev = NtCurrentTeb()->Tib.ExceptionList;

    memcpy(code_mem, call_one_arg_code, sizeof(call_one_arg_code));

    NtCurrentTeb()->Tib.ExceptionList = &frame;
    if (have_vectored_api)
    {
        vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &rtlraiseexception_vectored_handler);
        ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");
    }

    func(pRtlRaiseException, &record);
    ok( record.ExceptionAddress == (char *)code_mem + 0x0b,
        "address set to %p instead of %p\n", record.ExceptionAddress, (char *)code_mem + 0x0b );

    if (have_vectored_api)
        pRtlRemoveVectoredExceptionHandler(vectored_handler);
    NtCurrentTeb()->Tib.ExceptionList = frame.Prev;
}

static void test_rtlraiseexception(void)
{
    if (!pRtlRaiseException)
    {
        skip("RtlRaiseException not found\n");
        return;
    }

    /* test without debugger */
    run_rtlraiseexception_test(0x12345);
    run_rtlraiseexception_test(EXCEPTION_BREAKPOINT);
    run_rtlraiseexception_test(EXCEPTION_INVALID_HANDLE);
}

static DWORD unwind_expected_eax;

static DWORD unwind_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                             CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    trace("exception: %08lx flags:%lx addr:%p context: Eip:%lx\n",
          rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, context->Eip);

    ok(rec->ExceptionCode == STATUS_UNWIND, "ExceptionCode is %08lx instead of %08lx\n",
       rec->ExceptionCode, STATUS_UNWIND);
    ok(rec->ExceptionAddress == (char *)code_mem + 0x22 || broken(TRUE) /* Win10 1709 */,
       "ExceptionAddress at %p instead of %p\n", rec->ExceptionAddress, (char *)code_mem + 0x22);
    ok(context->Eip == (DWORD)code_mem + 0x22, "context->Eip is %08lx instead of %08lx\n",
       context->Eip, (DWORD)code_mem + 0x22);
    ok(context->Eax == unwind_expected_eax, "context->Eax is %08lx instead of %08lx\n",
       context->Eax, unwind_expected_eax);

    context->Eax += 1;
    return ExceptionContinueSearch;
}

static const BYTE call_unwind_code[] = {
    0x55,                           /* push %ebp */
    0x53,                           /* push %ebx */
    0x56,                           /* push %esi */
    0x57,                           /* push %edi */
    0xe8, 0x00, 0x00, 0x00, 0x00,   /* call 0 */
    0x58,                           /* 0: pop %eax */
    0x05, 0x1e, 0x00, 0x00, 0x00,   /* add $0x1e,%eax */
    0xff, 0x74, 0x24, 0x20,         /* push 0x20(%esp) */
    0xff, 0x74, 0x24, 0x20,         /* push 0x20(%esp) */
    0x50,                           /* push %eax */
    0xff, 0x74, 0x24, 0x24,         /* push 0x24(%esp) */
    0x8B, 0x44, 0x24, 0x24,         /* mov 0x24(%esp),%eax */
    0xff, 0xd0,                     /* call *%eax */
    0x5f,                           /* pop %edi */
    0x5e,                           /* pop %esi */
    0x5b,                           /* pop %ebx */
    0x5d,                           /* pop %ebp */
    0xc3,                           /* ret */
    0xcc,                           /* int $3 */
};

static void test_unwind(void)
{
    EXCEPTION_REGISTRATION_RECORD frames[2], *frame2 = &frames[0], *frame1 = &frames[1];
    DWORD (*func)(void* function, EXCEPTION_REGISTRATION_RECORD *pEndFrame, EXCEPTION_RECORD* record, DWORD retval) = code_mem;
    DWORD retval;

    memcpy(code_mem, call_unwind_code, sizeof(call_unwind_code));

    /* add first unwind handler */
    frame1->Handler = unwind_handler;
    frame1->Prev = NtCurrentTeb()->Tib.ExceptionList;
    NtCurrentTeb()->Tib.ExceptionList = frame1;

    /* add second unwind handler */
    frame2->Handler = unwind_handler;
    frame2->Prev = NtCurrentTeb()->Tib.ExceptionList;
    NtCurrentTeb()->Tib.ExceptionList = frame2;

    /* test unwind to current frame */
    unwind_expected_eax = 0xDEAD0000;
    retval = func(pRtlUnwind, frame2, NULL, 0xDEAD0000);
    ok(retval == 0xDEAD0000, "RtlUnwind returned eax %08lx instead of %08x\n", retval, 0xDEAD0000);
    ok(NtCurrentTeb()->Tib.ExceptionList == frame2, "Exception record points to %p instead of %p\n",
       NtCurrentTeb()->Tib.ExceptionList, frame2);

    /* unwind to frame1 */
    unwind_expected_eax = 0xDEAD0000;
    retval = func(pRtlUnwind, frame1, NULL, 0xDEAD0000);
    ok(retval == 0xDEAD0001, "RtlUnwind returned eax %08lx instead of %08x\n", retval, 0xDEAD0001);
    ok(NtCurrentTeb()->Tib.ExceptionList == frame1, "Exception record points to %p instead of %p\n",
       NtCurrentTeb()->Tib.ExceptionList, frame1);

    /* restore original handler */
    NtCurrentTeb()->Tib.ExceptionList = frame1->Prev;
}

static DWORD prot_fault_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                 CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    const struct exception *except = *(const struct exception **)(frame + 1);
    unsigned int i, parameter_count, entry = except - exceptions;

    got_exception++;
    trace( "exception %u: %lx flags:%lx addr:%p\n",
           entry, rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );

    ok( rec->ExceptionCode == except->status ||
        (except->alt_status != 0 && rec->ExceptionCode == except->alt_status),
        "%u: Wrong exception code %lx/%lx\n", entry, rec->ExceptionCode, except->status );
    ok( context->Eip == (DWORD)code_mem + except->offset,
        "%u: Unexpected eip %#lx/%#lx\n", entry,
        context->Eip, (DWORD)code_mem + except->offset );
    ok( rec->ExceptionAddress == (char*)context->Eip ||
        (rec->ExceptionCode == STATUS_BREAKPOINT && rec->ExceptionAddress == (char*)context->Eip + 1),
        "%u: Unexpected exception address %p/%p\n", entry,
        rec->ExceptionAddress, (char*)context->Eip );

    if (except->status == STATUS_BREAKPOINT && is_wow64)
        parameter_count = 1;
    else if (except->alt_status == 0 || rec->ExceptionCode != except->alt_status)
        parameter_count = except->nb_params;
    else
        parameter_count = except->alt_nb_params;

    ok( rec->NumberParameters == parameter_count,
        "%u: Unexpected parameter count %lu/%u\n", entry, rec->NumberParameters, parameter_count );

    /* Most CPUs (except Intel Core apparently) report a segment limit violation */
    /* instead of page faults for accesses beyond 0xffffffff */
    if (except->nb_params == 2 && except->params[1] >= 0xfffffffd)
    {
        if (rec->ExceptionInformation[0] == 0 && rec->ExceptionInformation[1] == 0xffffffff)
            goto skip_params;
    }

    /* Seems that both 0xbee8 and 0xfffffffff can be returned in windows */
    if (except->nb_params == 2 && rec->NumberParameters == 2
        && except->params[1] == 0xbee8 && rec->ExceptionInformation[1] == 0xffffffff
        && except->params[0] == rec->ExceptionInformation[0])
    {
        goto skip_params;
    }

    if (except->alt_status == 0 || rec->ExceptionCode != except->alt_status)
    {
        for (i = 0; i < rec->NumberParameters; i++)
            ok( rec->ExceptionInformation[i] == except->params[i],
                "%u: Wrong parameter %d: %Ix/%lx\n",
                entry, i, rec->ExceptionInformation[i], except->params[i] );
    }
    else
    {
        for (i = 0; i < rec->NumberParameters; i++)
            ok( rec->ExceptionInformation[i] == except->alt_params[i],
                "%u: Wrong parameter %d: %Ix/%lx\n",
                entry, i, rec->ExceptionInformation[i], except->alt_params[i] );
    }

skip_params:
    context->Eip = (DWORD_PTR)code_mem + except->offset + except->length;
    return ExceptionContinueExecution;
}

static void test_prot_fault(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(exceptions); i++)
    {
        if (is_wow64 && exceptions[i].wow64_broken && !strcmp( winetest_platform, "windows" ))
        {
            skip( "Exception %u broken on Wow64\n", i );
            continue;
        }
        got_exception = 0;
        run_exception_test(prot_fault_handler, &exceptions[i], &exceptions[i].code,
                           sizeof(exceptions[i].code), 0);
        if (!i && !got_exception)
        {
            trace( "No exception, assuming win9x, no point in testing further\n" );
            break;
        }
        ok( got_exception == (exceptions[i].status != 0),
            "%u: bad exception count %d\n", i, got_exception );
    }
}

struct dbgreg_test {
    DWORD dr0, dr1, dr2, dr3, dr6, dr7;
};

/* test handling of debug registers */
static DWORD dreg_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                      CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    const struct dbgreg_test *test = *(const struct dbgreg_test **)(frame + 1);

    context->Eip += 2;	/* Skips the popl (%eax) */
    context->Dr0 = test->dr0;
    context->Dr1 = test->dr1;
    context->Dr2 = test->dr2;
    context->Dr3 = test->dr3;
    context->Dr6 = test->dr6;
    context->Dr7 = test->dr7;
    return ExceptionContinueExecution;
}

#define CHECK_DEBUG_REG(n, m) \
    ok((ctx.Dr##n & m) == test->dr##n, "(%d) failed to set debug register " #n " to %lx, got %lx\n", \
       test_num, test->dr##n, ctx.Dr##n)

static void check_debug_registers(int test_num, const struct dbgreg_test *test)
{
    CONTEXT ctx;
    NTSTATUS status;

    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    status = pNtGetContextThread(GetCurrentThread(), &ctx);
    ok(status == STATUS_SUCCESS, "NtGetContextThread failed with %lx\n", status);

    CHECK_DEBUG_REG(0, ~0);
    CHECK_DEBUG_REG(1, ~0);
    CHECK_DEBUG_REG(2, ~0);
    CHECK_DEBUG_REG(3, ~0);
    CHECK_DEBUG_REG(6, 0x0f);
    CHECK_DEBUG_REG(7, ~0xdc00);
}

static const BYTE segfault_code[5] = {
	0x31, 0xc0, /* xor    %eax,%eax */
	0x8f, 0x00, /* popl   (%eax) - cause exception */
        0xc3        /* ret */
};

/* test the single step exception behaviour */
static DWORD single_step_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                  CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    got_exception++;
    ok (!(context->EFlags & 0x100), "eflags has single stepping bit set\n");

    if( got_exception < 3)
        context->EFlags |= 0x100;  /* single step until popf instruction */
    else {
        /* show that the last single step exception on the popf instruction
         * (which removed the TF bit), still is a EXCEPTION_SINGLE_STEP exception */
        ok( rec->ExceptionCode == EXCEPTION_SINGLE_STEP,
            "exception is not EXCEPTION_SINGLE_STEP: %lx\n", rec->ExceptionCode);
    }

    return ExceptionContinueExecution;
}

static const BYTE single_stepcode[] = {
    0x9c,		/* pushf */
    0x58,		/* pop   %eax */
    0x0d,0,1,0,0,	/* or    $0x100,%eax */
    0x50,		/* push   %eax */
    0x9d,		/* popf    */
    0x35,0,1,0,0,	/* xor    $0x100,%eax */
    0x50,		/* push   %eax */
    0x9d,		/* popf    */
    0xc3
};

/* Test the alignment check (AC) flag handling. */
static const BYTE align_check_code[] = {
    0x55,                  	/* push   %ebp */
    0x89,0xe5,             	/* mov    %esp,%ebp */
    0x9c,                  	/* pushf   */
    0x9c,                  	/* pushf   */
    0x58,                  	/* pop    %eax */
    0x0d,0,0,4,0,       	/* or     $0x40000,%eax */
    0x50,                  	/* push   %eax */
    0x9d,                  	/* popf    */
    0x89,0xe0,                  /* mov %esp, %eax */
    0x8b,0x40,0x1,              /* mov 0x1(%eax), %eax - cause exception */
    0x9d,                  	/* popf    */
    0x5d,                  	/* pop    %ebp */
    0xc3,                  	/* ret     */
};

static DWORD align_check_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                  CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    ok (!(context->EFlags & 0x40000), "eflags has AC bit set\n");
    got_exception++;
    return ExceptionContinueExecution;
}

/* Test the direction flag handling. */
static const BYTE direction_flag_code[] = {
    0x55,                  	/* push   %ebp */
    0x89,0xe5,             	/* mov    %esp,%ebp */
    0xfd,                  	/* std */
    0xfa,                  	/* cli - cause exception */
    0x5d,                  	/* pop    %ebp */
    0xc3,                  	/* ret     */
};

static DWORD direction_flag_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                     CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
#ifdef __GNUC__
    unsigned int flags;
    __asm__("pushfl; popl %0; cld" : "=r" (flags) );
    /* older windows versions don't clear DF properly so don't test */
    if (flags & 0x400) trace( "eflags has DF bit set\n" );
#endif
    ok( context->EFlags & 0x400, "context eflags has DF bit cleared\n" );
    got_exception++;
    context->Eip++;  /* skip cli */
    context->EFlags &= ~0x400;  /* make sure it is cleared on return */
    return ExceptionContinueExecution;
}

/* test single stepping over hardware breakpoint */
static DWORD bpx_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                          CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    got_exception++;
    ok( rec->ExceptionCode == EXCEPTION_SINGLE_STEP,
        "wrong exception code: %lx\n", rec->ExceptionCode);

    if(got_exception == 1) {
        /* hw bp exception on first nop */
        ok( context->Eip == (DWORD)code_mem, "eip is wrong:  %lx instead of %lx\n",
                                             context->Eip, (DWORD)code_mem);
        ok( (context->Dr6 & 0xf) == 1, "B0 flag is not set in Dr6\n");
        ok( !(context->Dr6 & 0x4000), "BS flag is set in Dr6\n");
        context->Dr0 = context->Dr0 + 1;  /* set hw bp again on next instruction */
        context->EFlags |= 0x100;       /* enable single stepping */
    } else if(  got_exception == 2) {
        /* single step exception on second nop */
        ok( context->Eip == (DWORD)code_mem + 1, "eip is wrong: %lx instead of %lx\n",
                                                 context->Eip, (DWORD)code_mem + 1);
        ok( (context->Dr6 & 0x4000), "BS flag is not set in Dr6\n");
       /* depending on the win version the B0 bit is already set here as well
        ok( (context->Dr6 & 0xf) == 0, "B0...3 flags in Dr6 shouldn't be set\n"); */
        context->EFlags |= 0x100;
    } else if( got_exception == 3) {
        /* hw bp exception on second nop */
        ok( context->Eip == (DWORD)code_mem + 1, "eip is wrong: %lx instead of %lx\n",
                                                 context->Eip, (DWORD)code_mem + 1);
        ok( (context->Dr6 & 0xf) == 1, "B0 flag is not set in Dr6\n");
        ok( !(context->Dr6 & 0x4000), "BS flag is set in Dr6\n");
        context->Dr0 = 0;       /* clear breakpoint */
        context->EFlags |= 0x100;
    } else {
        /* single step exception on ret */
        ok( context->Eip == (DWORD)code_mem + 2, "eip is wrong: %lx instead of %lx\n",
                                                 context->Eip, (DWORD)code_mem + 2);
        ok( (context->Dr6 & 0xf) == 0, "B0...3 flags in Dr6 shouldn't be set\n");
        ok( (context->Dr6 & 0x4000), "BS flag is not set in Dr6\n");
    }

    context->Dr6 = 0;  /* clear status register */
    return ExceptionContinueExecution;
}

static const BYTE dummy_code[] = { 0x90, 0x90, 0xc3 };  /* nop, nop, ret */

/* test int3 handling */
static DWORD int3_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                           CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    ok( rec->ExceptionAddress == code_mem, "exception address not at: %p, but at %p\n",
                                           code_mem,  rec->ExceptionAddress);
    ok( context->Eip == (DWORD)code_mem, "eip not at: %p, but at %#lx\n", code_mem, context->Eip);
    if(context->Eip == (DWORD)code_mem) context->Eip++; /* skip breakpoint */

    return ExceptionContinueExecution;
}

static const BYTE int3_code[] = { 0xCC, 0xc3 };  /* int 3, ret */

static DWORD WINAPI hw_reg_exception_thread( void *arg )
{
    int expect = (ULONG_PTR)arg;
    got_exception = 0;
    run_exception_test( bpx_handler, NULL, dummy_code, sizeof(dummy_code), 0 );
    ok( got_exception == expect, "expected %u exceptions, got %d\n", expect, got_exception );
    return 0;
}

static void test_exceptions(void)
{
    CONTEXT ctx;
    NTSTATUS res;
    struct dbgreg_test dreg_test;
    HANDLE h;

    if (!pNtGetContextThread || !pNtSetContextThread)
    {
        skip( "NtGetContextThread/NtSetContextThread not found\n" );
        return;
    }

    /* test handling of debug registers */
    memset(&dreg_test, 0, sizeof(dreg_test));

    dreg_test.dr0 = 0x42424240;
    dreg_test.dr2 = 0x126bb070;
    dreg_test.dr3 = 0x0badbad0;
    dreg_test.dr7 = 0xffff0115;
    run_exception_test(dreg_handler, &dreg_test, &segfault_code, sizeof(segfault_code), 0);
    check_debug_registers(1, &dreg_test);

    dreg_test.dr0 = 0x42424242;
    dreg_test.dr2 = 0x100f0fe7;
    dreg_test.dr3 = 0x0abebabe;
    dreg_test.dr7 = 0x115;
    run_exception_test(dreg_handler, &dreg_test, &segfault_code, sizeof(segfault_code), 0);
    check_debug_registers(2, &dreg_test);

#if defined(__REACTOS__)
    if (is_reactos())
    {
        skip("Skipping test of single stepping behavior and int3 handling on ReactOS due to crashes\n");
        return;
    }
#endif

    /* test single stepping behavior */
    got_exception = 0;
    run_exception_test(single_step_handler, NULL, &single_stepcode, sizeof(single_stepcode), 0);
    ok(got_exception == 3, "expected 3 single step exceptions, got %d\n", got_exception);

    /* test alignment exceptions */
    got_exception = 0;
    run_exception_test(align_check_handler, NULL, align_check_code, sizeof(align_check_code), 0);
    ok(got_exception == 0, "got %d alignment faults, expected 0\n", got_exception);

    /* test direction flag */
    got_exception = 0;
    run_exception_test(direction_flag_handler, NULL, direction_flag_code, sizeof(direction_flag_code), 0);
    ok(got_exception == 1, "got %d exceptions, expected 1\n", got_exception);

    /* test single stepping over hardware breakpoint */
    memset(&ctx, 0, sizeof(ctx));
    ctx.Dr0 = (DWORD) code_mem;  /* set hw bp on first nop */
    ctx.Dr7 = 3;
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    res = pNtSetContextThread( GetCurrentThread(), &ctx);
    ok( res == STATUS_SUCCESS, "NtSetContextThread failed with %lx\n", res);

    got_exception = 0;
    run_exception_test(bpx_handler, NULL, dummy_code, sizeof(dummy_code), 0);
    ok( got_exception == 4,"expected 4 exceptions, got %d\n", got_exception);

    /* test int3 handling */
    run_exception_test(int3_handler, NULL, int3_code, sizeof(int3_code), 0);

    /* test that hardware breakpoints are not inherited by created threads */
    res = pNtSetContextThread( GetCurrentThread(), &ctx );
    ok( res == STATUS_SUCCESS, "NtSetContextThread failed with %lx\n", res );

    h = CreateThread( NULL, 0, hw_reg_exception_thread, 0, 0, NULL );
    WaitForSingleObject( h, 10000 );
    CloseHandle( h );

    h = CreateThread( NULL, 0, hw_reg_exception_thread, (void *)4, CREATE_SUSPENDED, NULL );
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    res = pNtGetContextThread( h, &ctx );
    ok( res == STATUS_SUCCESS, "NtGetContextThread failed with %lx\n", res );
    ok( ctx.Dr0 == 0, "dr0 %lx\n", ctx.Dr0 );
    ok( ctx.Dr7 == 0, "dr7 %lx\n", ctx.Dr7 );
    ctx.Dr0 = (DWORD)code_mem;
    ctx.Dr7 = 3;
    res = pNtSetContextThread( h, &ctx );
    ok( res == STATUS_SUCCESS, "NtSetContextThread failed with %lx\n", res );
    ResumeThread( h );
    WaitForSingleObject( h, 10000 );
    CloseHandle( h );

    ctx.Dr0 = 0;
    ctx.Dr7 = 0;
    res = pNtSetContextThread( GetCurrentThread(), &ctx );
    ok( res == STATUS_SUCCESS, "NtSetContextThread failed with %lx\n", res );
}

static void test_debugger(DWORD cont_status, BOOL with_WaitForDebugEventEx)
{
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    DEBUG_EVENT de;
    DWORD continuestatus;
    PVOID code_mem_address = NULL;
    NTSTATUS status;
    SIZE_T size_read;
    BOOL ret;
    int counter = 0;
    si.cb = sizeof(si);

    if(!pNtGetContextThread || !pNtSetContextThread || !pNtReadVirtualMemory || !pNtTerminateProcess)
    {
        skip("NtGetContextThread, NtSetContextThread, NtReadVirtualMemory or NtTerminateProcess not found\n");
        return;
    }

    if (with_WaitForDebugEventEx && !pWaitForDebugEventEx)
    {
        skip("WaitForDebugEventEx not found, skipping unicode strings in OutputDebugStringW\n");
        return;
    }

    sprintf(cmdline, "%s %s %s %p", my_argv[0], my_argv[1], "debuggee", &test_stage);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "could not create child process error: %lu\n", GetLastError());
    if (!ret)
        return;

    do
    {
        continuestatus = cont_status;
        ret = with_WaitForDebugEventEx ? pWaitForDebugEventEx(&de, INFINITE) : WaitForDebugEvent(&de, INFINITE);
        ok(ret, "reading debug event\n");

        ret = ContinueDebugEvent(de.dwProcessId, de.dwThreadId, 0xdeadbeef);
        ok(!ret, "ContinueDebugEvent unexpectedly succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "Unexpected last error: %lu\n", GetLastError());

        if (de.dwThreadId != pi.dwThreadId)
        {
            trace("event %ld not coming from main thread, ignoring\n", de.dwDebugEventCode);
            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, cont_status);
            continue;
        }

        if (de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            if(de.u.CreateProcessInfo.lpBaseOfImage != NtCurrentTeb()->Peb->ImageBaseAddress)
            {
                skip("child process loaded at different address, terminating it\n");
                pNtTerminateProcess(pi.hProcess, 0);
            }
        }
        else if (de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            CONTEXT ctx;
            enum debugger_stages stage;

            counter++;
            status = pNtReadVirtualMemory(pi.hProcess, &code_mem, &code_mem_address,
                                          sizeof(code_mem_address), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);
            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            ctx.ContextFlags = CONTEXT_FULL | CONTEXT_EXTENDED_REGISTERS | CONTEXT_EXCEPTION_REQUEST;
            status = pNtGetContextThread(pi.hThread, &ctx);
            ok(!status, "NtGetContextThread failed with 0x%lx\n", status);
            ok(ctx.ContextFlags & CONTEXT_EXCEPTION_REPORTING
                    || broken( !(ctx.ContextFlags & CONTEXT_EXCEPTION_REPORTING) ) /* Win7 WoW64 */,
                    "got %#lx.\n", ctx.ContextFlags);

            trace("exception 0x%lx at %p firstchance=%ld Eip=0x%lx, Eax=0x%lx ctx.ContextFlags %#lx\n",
                  de.u.Exception.ExceptionRecord.ExceptionCode,
                  de.u.Exception.ExceptionRecord.ExceptionAddress, de.u.Exception.dwFirstChance, ctx.Eip, ctx.Eax, ctx.ContextFlags);

            if (counter > 100)
            {
                ok(FALSE, "got way too many exceptions, probably caught in an infinite loop, terminating child\n");
                pNtTerminateProcess(pi.hProcess, 1);
            }
            else if (counter < 2) /* startup breakpoint */
            {
                /* breakpoint is inside ntdll */
                void *ntdll = GetModuleHandleA( "ntdll.dll" );
                IMAGE_NT_HEADERS *nt = RtlImageNtHeader( ntdll );

                ok( (char *)ctx.Eip >= (char *)ntdll &&
                    (char *)ctx.Eip < (char *)ntdll + nt->OptionalHeader.SizeOfImage,
                    "wrong eip %p ntdll %p-%p\n", (void *)ctx.Eip, ntdll,
                    (char *)ntdll + nt->OptionalHeader.SizeOfImage );
                check_context_exception_request( ctx.ContextFlags, TRUE );
            }
            else
            {
                if (stage == STAGE_RTLRAISE_NOT_HANDLED)
                {
                    ok((char *)ctx.Eip == (char *)code_mem_address + 0xb, "Eip at %lx instead of %p\n",
                       ctx.Eip, (char *)code_mem_address + 0xb);
                    /* setting the context from debugger does not affect the context that the
                     * exception handler gets, except on w2008 */
                    ctx.Eip = (UINT_PTR)code_mem_address + 0xd;
                    ctx.Eax = 0xf00f00f1;
                    /* let the debuggee handle the exception */
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    check_context_exception_request( ctx.ContextFlags, !is_wow64 );
                }
                else if (stage == STAGE_RTLRAISE_HANDLE_LAST_CHANCE)
                {
                    if (de.u.Exception.dwFirstChance)
                    {
                        /* debugger gets first chance exception with unmodified ctx.Eip */
                        ok((char *)ctx.Eip == (char *)code_mem_address + 0xb, "Eip at 0x%lx instead of %p\n",
                           ctx.Eip, (char *)code_mem_address + 0xb);
                        ctx.Eip = (UINT_PTR)code_mem_address + 0xd;
                        ctx.Eax = 0xf00f00f1;
                        /* pass exception to debuggee
                         * exception will not be handled and a second chance exception will be raised */
                        continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    }
                    else
                    {
                        /* debugger gets context after exception handler has played with it */
                        /* ctx.Eip is the same value the exception handler got */
                        if (de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                        {
                            ok((char *)ctx.Eip == (char *)code_mem_address + 0xa ||
                               (is_wow64 && (char *)ctx.Eip == (char *)code_mem_address + 0xb) ||
                               broken((char *)ctx.Eip == (char *)code_mem_address + 0xd) /* w2008 */,
                               "Eip at 0x%lx instead of %p\n",
                                ctx.Eip, (char *)code_mem_address + 0xa);
                            /* need to fixup Eip for debuggee */
                            if ((char *)ctx.Eip == (char *)code_mem_address + 0xa)
                                ctx.Eip += 1;
                        }
                        else
                            ok((char *)ctx.Eip == (char *)code_mem_address + 0xb ||
                               broken((char *)ctx.Eip == (char *)code_mem_address + 0xd) /* w2008 */,
                               "Eip at 0x%lx instead of %p\n",
                               ctx.Eip, (char *)code_mem_address + 0xb);
                        /* here we handle exception */
                    }
                    check_context_exception_request( ctx.ContextFlags, !is_wow64 );
                }
                else if (stage == STAGE_SERVICE_CONTINUE || stage == STAGE_SERVICE_NOT_HANDLED)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08lx\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Eip == (char *)code_mem_address + 0x1d,
                       "expected Eip = %p, got 0x%lx\n", (char *)code_mem_address + 0x1d, ctx.Eip);

                    if (stage == STAGE_SERVICE_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    check_context_exception_request( ctx.ContextFlags, TRUE );
                }
                else if (stage == STAGE_BREAKPOINT_CONTINUE || stage == STAGE_BREAKPOINT_NOT_HANDLED)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08lx\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Eip == (char *)code_mem_address + 2,
                       "expected Eip = %p, got 0x%lx\n", (char *)code_mem_address + 2, ctx.Eip);

                    if (stage == STAGE_BREAKPOINT_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    check_context_exception_request( ctx.ContextFlags, TRUE );
                }
                else if (stage == STAGE_EXCEPTION_INVHANDLE_CONTINUE || stage == STAGE_EXCEPTION_INVHANDLE_NOT_HANDLED)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_INVALID_HANDLE,
                       "unexpected exception code %08lx, expected %08lx\n", de.u.Exception.ExceptionRecord.ExceptionCode,
                       EXCEPTION_INVALID_HANDLE);
                    ok(de.u.Exception.ExceptionRecord.NumberParameters == 0,
                       "unexpected number of parameters %ld, expected 0\n", de.u.Exception.ExceptionRecord.NumberParameters);

                    if (stage == STAGE_EXCEPTION_INVHANDLE_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    check_context_exception_request( ctx.ContextFlags, !is_wow64 );
                }
                else if (stage == STAGE_NO_EXCEPTION_INVHANDLE_NOT_HANDLED)
                {
                    ok(FALSE || broken(TRUE) /* < Win10 */, "should not throw exception\n");
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    check_context_exception_request( ctx.ContextFlags, !is_wow64 );
                }
                else if (stage == STAGE_XSTATE || stage == STAGE_XSTATE_LEGACY_SSE)
                {
                    check_context_exception_request( ctx.ContextFlags, TRUE );
                    test_debugger_xstate(pi.hThread, &ctx, stage);
                }
                else if (stage == STAGE_SEGMENTS)
                {
                    USHORT ss;
#if defined(__REACTOS__) && defined(_MSC_VER)
                    USHORT segss;
                    __asm  mov [segss], ss
                    ss = segss;
#else
                    __asm__( "movw %%ss,%0" : "=r" (ss) );
#endif
                    ok( ctx.SegSs == ss, "wrong ss %04lx / %04x\n", ctx.SegSs, ss );
                    ok( ctx.SegFs != ctx.SegSs, "wrong fs %04lx / %04lx\n", ctx.SegFs, ctx.SegSs );
                    if (is_wow64) todo_wine_if( !ctx.SegDs ) /* old wow64 */
                    {
                        ok( ctx.SegDs == ctx.SegSs, "wrong ds %04lx / %04lx\n", ctx.SegDs, ctx.SegSs );
                        ok( ctx.SegEs == ctx.SegSs, "wrong es %04lx / %04lx\n", ctx.SegEs, ctx.SegSs );
                        ok( ctx.SegGs == ctx.SegSs, "wrong gs %04lx / %04lx\n", ctx.SegGs, ctx.SegSs );
                    }
                    else
                    {
                        ok( !ctx.SegDs, "wrong ds %04lx / %04lx\n", ctx.SegDs, ctx.SegSs );
                        ok( !ctx.SegEs, "wrong es %04lx / %04lx\n", ctx.SegEs, ctx.SegSs );
                        ok( !ctx.SegGs, "wrong gs %04lx / %04lx\n", ctx.SegGs, ctx.SegSs );
                    }
                    check_context_exception_request( ctx.ContextFlags, TRUE );
                }
                else
                    ok(FALSE, "unexpected stage %u\n", stage);

                status = pNtSetContextThread(pi.hThread, &ctx);
                ok(!status, "NtSetContextThread failed with 0x%lx\n", status);
            }
        }
        else if (de.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
        {
            enum debugger_stages stage;
            char buffer[64 * sizeof(WCHAR)];
            unsigned char_size = de.u.DebugString.fUnicode ? sizeof(WCHAR) : sizeof(char);

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            if (de.u.DebugString.fUnicode)
                ok(with_WaitForDebugEventEx &&
                   (stage == STAGE_OUTPUTDEBUGSTRINGW_CONTINUE || stage == STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED),
                   "unexpected unicode debug string event\n");
            else
                ok(!with_WaitForDebugEventEx || stage != STAGE_OUTPUTDEBUGSTRINGW_CONTINUE || cont_status != DBG_CONTINUE,
                   "unexpected ansi debug string event %u %s %lx\n",
                   stage, with_WaitForDebugEventEx ? "with" : "without", cont_status);

            ok(de.u.DebugString.nDebugStringLength < sizeof(buffer) / char_size - 1,
               "buffer not large enough to hold %d bytes\n", de.u.DebugString.nDebugStringLength);

            memset(buffer, 0, sizeof(buffer));
            status = pNtReadVirtualMemory(pi.hProcess, de.u.DebugString.lpDebugStringData, buffer,
                                          de.u.DebugString.nDebugStringLength * char_size, &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            if (stage == STAGE_OUTPUTDEBUGSTRINGA_CONTINUE || stage == STAGE_OUTPUTDEBUGSTRINGA_NOT_HANDLED ||
                stage == STAGE_OUTPUTDEBUGSTRINGW_CONTINUE || stage == STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED)
            {
                if (de.u.DebugString.fUnicode)
                    ok(!wcscmp((WCHAR*)buffer, L"Hello World"), "got unexpected debug string '%ls'\n", (WCHAR*)buffer);
                else
                    ok(!strcmp(buffer, "Hello World"), "got unexpected debug string '%s'\n", buffer);
            }
            else /* ignore unrelated debug strings like 'SHIMVIEW: ShimInfo(Complete)' */
                ok(strstr(buffer, "SHIMVIEW") != NULL, "unexpected stage %x, got debug string event '%s'\n", stage, buffer);

            if (stage == STAGE_OUTPUTDEBUGSTRINGA_NOT_HANDLED || stage == STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED)
                continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }
        else if (de.dwDebugEventCode == RIP_EVENT)
        {
            enum debugger_stages stage;

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            if (stage == STAGE_RIPEVENT_CONTINUE || stage == STAGE_RIPEVENT_NOT_HANDLED)
            {
                ok(de.u.RipInfo.dwError == 0x11223344, "got unexpected rip error code %08lx, expected %08x\n",
                   de.u.RipInfo.dwError, 0x11223344);
                ok(de.u.RipInfo.dwType  == 0x55667788, "got unexpected rip type %08lx, expected %08x\n",
                   de.u.RipInfo.dwType, 0x55667788);
            }
            else
                ok(FALSE, "unexpected stage %x\n", stage);

            if (stage == STAGE_RIPEVENT_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continuestatus);

    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    wait_child_process( pi.hProcess );
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %lu\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "error %lu\n", GetLastError());

    return;
}

static DWORD simd_fault_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                 CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    int *stage = *(int **)(frame + 1);

    got_exception++;

    if( *stage == 1) {
        /* fault while executing sse instruction */
        context->Eip += 3; /* skip addps */
        return ExceptionContinueExecution;
    }
    else if ( *stage == 2 || *stage == 3 ) {
        /* stage 2 - divide by zero fault */
        /* stage 3 - invalid operation fault */
        if( rec->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION)
            skip("system doesn't support SIMD exceptions\n");
        else {
            ok( rec->ExceptionCode ==  STATUS_FLOAT_MULTIPLE_TRAPS,
                "exception code: %#lx, should be %#lx\n",
                rec->ExceptionCode,  STATUS_FLOAT_MULTIPLE_TRAPS);
            ok( rec->NumberParameters == is_wow64 ? 2 : 1, "# of params: %li\n", rec->NumberParameters);
            ok( rec->ExceptionInformation[0] == 0, "param #1: %Ix, should be 0\n", rec->ExceptionInformation[0]);
            if (rec->NumberParameters == 2)
                ok( rec->ExceptionInformation[1] == ((XSAVE_FORMAT *)context->ExtendedRegisters)->MxCsr,
                    "param #1: %Ix / %lx\n", rec->ExceptionInformation[1],
                    ((XSAVE_FORMAT *)context->ExtendedRegisters)->MxCsr);
        }
        context->Eip += 3; /* skip divps */
    }
    else
        ok(FALSE, "unexpected stage %x\n", *stage);

    return ExceptionContinueExecution;
}

static const BYTE simd_exception_test[] = {
    0x83, 0xec, 0x4,                     /* sub $0x4, %esp       */
    0x0f, 0xae, 0x1c, 0x24,              /* stmxcsr (%esp)       */
    0x8b, 0x04, 0x24,                    /* mov    (%esp),%eax   * store mxcsr */
    0x66, 0x81, 0x24, 0x24, 0xff, 0xfd,  /* andw $0xfdff,(%esp)  * enable divide by */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%esp)       * zero exceptions  */
    0x6a, 0x01,                          /* push   $0x1          */
    0x6a, 0x01,                          /* push   $0x1          */
    0x6a, 0x01,                          /* push   $0x1          */
    0x6a, 0x01,                          /* push   $0x1          */
    0x0f, 0x10, 0x0c, 0x24,              /* movups (%esp),%xmm1  * fill dividend  */
    0x0f, 0x57, 0xc0,                    /* xorps  %xmm0,%xmm0   * clear divisor  */
    0x0f, 0x5e, 0xc8,                    /* divps  %xmm0,%xmm1   * generate fault */
    0x83, 0xc4, 0x10,                    /* add    $0x10,%esp    */
    0x89, 0x04, 0x24,                    /* mov    %eax,(%esp)   * restore to old mxcsr */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%esp)       */
    0x83, 0xc4, 0x04,                    /* add    $0x4,%esp     */
    0xc3,                                /* ret */
};

static const BYTE simd_exception_test2[] = {
    0x83, 0xec, 0x4,                     /* sub $0x4, %esp       */
    0x0f, 0xae, 0x1c, 0x24,              /* stmxcsr (%esp)       */
    0x8b, 0x04, 0x24,                    /* mov    (%esp),%eax   * store mxcsr */
    0x66, 0x81, 0x24, 0x24, 0x7f, 0xff,  /* andw $0xff7f,(%esp)  * enable invalid       */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%esp)       * operation exceptions */
    0x0f, 0x57, 0xc9,                    /* xorps  %xmm1,%xmm1   * clear dividend */
    0x0f, 0x57, 0xc0,                    /* xorps  %xmm0,%xmm0   * clear divisor  */
    0x0f, 0x5e, 0xc8,                    /* divps  %xmm0,%xmm1   * generate fault */
    0x89, 0x04, 0x24,                    /* mov    %eax,(%esp)   * restore to old mxcsr */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%esp)       */
    0x83, 0xc4, 0x04,                    /* add    $0x4,%esp     */
    0xc3,                                /* ret */
};

static const BYTE sse_check[] = {
    0x0f, 0x58, 0xc8,                    /* addps  %xmm0,%xmm1 */
    0xc3,                                /* ret */
};

static void test_simd_exceptions(void)
{
    int stage;

    /* test if CPU & OS can do sse */
    stage = 1;
    got_exception = 0;
    run_exception_test(simd_fault_handler, &stage, sse_check, sizeof(sse_check), 0);
    if(got_exception) {
        skip("system doesn't support SSE\n");
        return;
    }

    /* generate a SIMD exception */
    stage = 2;
    got_exception = 0;
    run_exception_test(simd_fault_handler, &stage, simd_exception_test,
                       sizeof(simd_exception_test), 0);
    ok(got_exception == 1, "got exception: %i, should be 1\n", got_exception);

    /* generate a SIMD exception, test FPE_FLTINV */
    stage = 3;
    got_exception = 0;
    run_exception_test(simd_fault_handler, &stage, simd_exception_test2,
                       sizeof(simd_exception_test2), 0);
    ok(got_exception == 1, "got exception: %i, should be 1\n", got_exception);
}

struct fpu_exception_info
{
    DWORD exception_code;
    DWORD exception_offset;
    DWORD eip_offset;
};

static DWORD fpu_exception_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    struct fpu_exception_info *info = *(struct fpu_exception_info **)(frame + 1);

    info->exception_code = rec->ExceptionCode;
    info->exception_offset = (BYTE *)rec->ExceptionAddress - (BYTE *)code_mem;
    info->eip_offset = context->Eip - (DWORD)code_mem;

    ++context->Eip;
    return ExceptionContinueExecution;
}

static void test_fpu_exceptions(void)
{
    static const BYTE fpu_exception_test_ie[] =
    {
        0x83, 0xec, 0x04,                   /* sub $0x4,%esp        */
        0x66, 0xc7, 0x04, 0x24, 0xfe, 0x03, /* movw $0x3fe,(%esp)   */
        0x9b, 0xd9, 0x7c, 0x24, 0x02,       /* fstcw 0x2(%esp)      */
        0xd9, 0x2c, 0x24,                   /* fldcw (%esp)         */
        0xd9, 0xee,                         /* fldz                 */
        0xd9, 0xe8,                         /* fld1                 */
        0xde, 0xf1,                         /* fdivp                */
        0xdd, 0xd8,                         /* fstp %st(0)          */
        0xdd, 0xd8,                         /* fstp %st(0)          */
        0x9b,                               /* fwait                */
        0xdb, 0xe2,                         /* fnclex               */
        0xd9, 0x6c, 0x24, 0x02,             /* fldcw 0x2(%esp)      */
        0x83, 0xc4, 0x04,                   /* add $0x4,%esp        */
        0xc3,                               /* ret                  */
    };

    static const BYTE fpu_exception_test_de[] =
    {
        0x83, 0xec, 0x04,                   /* sub $0x4,%esp        */
        0x66, 0xc7, 0x04, 0x24, 0xfb, 0x03, /* movw $0x3fb,(%esp)   */
        0x9b, 0xd9, 0x7c, 0x24, 0x02,       /* fstcw 0x2(%esp)      */
        0xd9, 0x2c, 0x24,                   /* fldcw (%esp)         */
        0xdd, 0xd8,                         /* fstp %st(0)          */
        0xd9, 0xee,                         /* fldz                 */
        0xd9, 0xe8,                         /* fld1                 */
        0xde, 0xf1,                         /* fdivp                */
        0x9b,                               /* fwait                */
        0xdb, 0xe2,                         /* fnclex               */
        0xdd, 0xd8,                         /* fstp %st(0)          */
        0xdd, 0xd8,                         /* fstp %st(0)          */
        0xd9, 0x6c, 0x24, 0x02,             /* fldcw 0x2(%esp)      */
        0x83, 0xc4, 0x04,                   /* add $0x4,%esp        */
        0xc3,                               /* ret                  */
    };

    struct fpu_exception_info info;

    memset(&info, 0, sizeof(info));
    run_exception_test(fpu_exception_handler, &info, fpu_exception_test_ie, sizeof(fpu_exception_test_ie), 0);
    ok(info.exception_code == EXCEPTION_FLT_STACK_CHECK,
            "Got exception code %#lx, expected EXCEPTION_FLT_STACK_CHECK\n", info.exception_code);
    ok(info.exception_offset == 0x19 || info.exception_offset == info.eip_offset,
       "Got exception offset %#lx, expected 0x19\n", info.exception_offset);
    ok(info.eip_offset == 0x1b, "Got EIP offset %#lx, expected 0x1b\n", info.eip_offset);

    memset(&info, 0, sizeof(info));
    run_exception_test(fpu_exception_handler, &info, fpu_exception_test_de, sizeof(fpu_exception_test_de), 0);
    ok(info.exception_code == EXCEPTION_FLT_DIVIDE_BY_ZERO,
            "Got exception code %#lx, expected EXCEPTION_FLT_DIVIDE_BY_ZERO\n", info.exception_code);
    ok(info.exception_offset == 0x17 || info.exception_offset == info.eip_offset,
       "Got exception offset %#lx, expected 0x17\n", info.exception_offset);
    ok(info.eip_offset == 0x19, "Got EIP offset %#lx, expected 0x19\n", info.eip_offset);
}

struct dpe_exception_info {
    BOOL exception_caught;
    DWORD exception_info;
};

static DWORD dpe_exception_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    DWORD old_prot;
    struct dpe_exception_info *info = *(struct dpe_exception_info **)(frame + 1);

    ok(rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION,
       "Exception code %08lx\n", rec->ExceptionCode);
    ok(rec->NumberParameters == 2,
       "Parameter count: %ld\n", rec->NumberParameters);
    ok((LPVOID)rec->ExceptionInformation[1] == code_mem,
       "Exception address: %p, expected %p\n",
       (LPVOID)rec->ExceptionInformation[1], code_mem);

    info->exception_info = rec->ExceptionInformation[0];
    info->exception_caught = TRUE;

    VirtualProtect(code_mem, 1, PAGE_EXECUTE_READWRITE, &old_prot);
    return ExceptionContinueExecution;
}

static void test_dpe_exceptions(void)
{
    static const BYTE single_ret[] = {0xC3};
    struct dpe_exception_info info;
    NTSTATUS stat;
    BOOL has_hw_support;
    BOOL is_permanent = FALSE, can_test_without = TRUE, can_test_with = TRUE;
    DWORD val;
    ULONG len;

    /* Query DEP with len too small */
    stat = NtQueryInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val - 1, &len);
    if(stat == STATUS_INVALID_INFO_CLASS)
    {
        skip("This software platform does not support DEP\n");
        return;
    }
    ok(stat == STATUS_INFO_LENGTH_MISMATCH, "buffer too small: %08lx\n", stat);

    /* Query DEP */
    stat = NtQueryInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val, &len);
    ok(stat == STATUS_SUCCESS, "querying DEP: status %08lx\n", stat);
    if(stat == STATUS_SUCCESS)
    {
        ok(len == sizeof val, "returned length: %ld\n", len);
        if(val & MEM_EXECUTE_OPTION_PERMANENT)
        {
            skip("toggling DEP impossible - status locked\n");
            is_permanent = TRUE;
            if(val & MEM_EXECUTE_OPTION_DISABLE)
                can_test_without = FALSE;
            else
                can_test_with = FALSE;
        }
    }

    if(!is_permanent)
    {
        /* Enable DEP */
        val = MEM_EXECUTE_OPTION_DISABLE;
        stat = pNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val);
        ok(stat == STATUS_SUCCESS, "enabling DEP: status %08lx\n", stat);
    }

    if(can_test_with)
    {
        /* Try access to locked page with DEP on*/
        info.exception_caught = FALSE;
        run_exception_test(dpe_exception_handler, &info, single_ret, sizeof(single_ret), PAGE_NOACCESS);
        ok(info.exception_caught == TRUE, "Execution of disabled memory succeeded\n");
        ok(info.exception_info == EXCEPTION_READ_FAULT ||
           info.exception_info == EXCEPTION_EXECUTE_FAULT,
              "Access violation type: %08x\n", (unsigned)info.exception_info);
        has_hw_support = info.exception_info == EXCEPTION_EXECUTE_FAULT;
        trace("DEP hardware support: %s\n", has_hw_support?"Yes":"No");

        /* Try execution of data with DEP on*/
        info.exception_caught = FALSE;
        run_exception_test(dpe_exception_handler, &info, single_ret, sizeof(single_ret), PAGE_READWRITE);
        if(has_hw_support)
        {
            ok(info.exception_caught == TRUE, "Execution of data memory succeeded\n");
            ok(info.exception_info == EXCEPTION_EXECUTE_FAULT,
                  "Access violation type: %08x\n", (unsigned)info.exception_info);
        }
        else
            ok(info.exception_caught == FALSE, "Execution trapped without hardware support\n");
    }
    else
        skip("DEP is in AlwaysOff state\n");

    if(!is_permanent)
    {
        /* Disable DEP */
        val = MEM_EXECUTE_OPTION_ENABLE;
        stat = pNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val);
        ok(stat == STATUS_SUCCESS, "disabling DEP: status %08lx\n", stat);
    }

    /* page is read without exec here */
    if(can_test_without)
    {
        /* Try execution of data with DEP off */
        info.exception_caught = FALSE;
        run_exception_test(dpe_exception_handler, &info, single_ret, sizeof(single_ret), PAGE_READWRITE);
        ok(info.exception_caught == FALSE, "Execution trapped with DEP turned off\n");

        /* Try access to locked page with DEP off - error code is different than
           with hardware DEP on */
        info.exception_caught = FALSE;
        run_exception_test(dpe_exception_handler, &info, single_ret, sizeof(single_ret), PAGE_NOACCESS);
        ok(info.exception_caught == TRUE, "Execution of disabled memory succeeded\n");
        ok(info.exception_info == EXCEPTION_READ_FAULT,
              "Access violation type: %08x\n", (unsigned)info.exception_info);
    }
    else
        skip("DEP is in AlwaysOn state\n");

    if(!is_permanent)
    {
        /* Turn off DEP permanently */
        val = MEM_EXECUTE_OPTION_ENABLE | MEM_EXECUTE_OPTION_PERMANENT;
        stat = pNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val);
        ok(stat == STATUS_SUCCESS, "disabling DEP permanently: status %08lx\n", stat);
    }

    /* Try to turn off DEP */
    val = MEM_EXECUTE_OPTION_ENABLE;
    stat = pNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val);
    ok(stat == STATUS_ACCESS_DENIED, "disabling DEP while permanent: status %08lx\n", stat);

    /* Try to turn on DEP */
    val = MEM_EXECUTE_OPTION_DISABLE;
    stat = pNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val);
    ok(stat == STATUS_ACCESS_DENIED, "enabling DEP while permanent: status %08lx\n", stat);
}

static void test_thread_context(void)
{
    CONTEXT context;
    NTSTATUS status;
    struct expected
    {
        DWORD Eax, Ebx, Ecx, Edx, Esi, Edi, Ebp, Esp, Eip,
            SegCs, SegDs, SegEs, SegFs, SegGs, SegSs, EFlags, prev_frame,
            x87_control;
    } expect;
    NTSTATUS (*func_ptr)( struct expected *res, void *func, void *arg1, void *arg2,
            DWORD *new_x87_control ) = code_mem;
    DWORD new_x87_control;

    static const BYTE call_func[] =
    {
        0x55,             /* pushl  %ebp */
        0x89, 0xe5,       /* mov    %esp,%ebp */
        0x50,             /* pushl  %eax ; add a bit of offset to the stack */
        0x50,             /* pushl  %eax */
        0x50,             /* pushl  %eax */
        0x50,             /* pushl  %eax */
        0x8b, 0x45, 0x08, /* mov    0x8(%ebp),%eax */
        0x8f, 0x00,       /* popl   (%eax) */
        0x89, 0x58, 0x04, /* mov    %ebx,0x4(%eax) */
        0x89, 0x48, 0x08, /* mov    %ecx,0x8(%eax) */
        0x89, 0x50, 0x0c, /* mov    %edx,0xc(%eax) */
        0x89, 0x70, 0x10, /* mov    %esi,0x10(%eax) */
        0x89, 0x78, 0x14, /* mov    %edi,0x14(%eax) */
        0x89, 0x68, 0x18, /* mov    %ebp,0x18(%eax) */
        0x89, 0x60, 0x1c, /* mov    %esp,0x1c(%eax) */
        0xff, 0x75, 0x04, /* pushl  0x4(%ebp) */
        0x8f, 0x40, 0x20, /* popl   0x20(%eax) */
        0x8c, 0x48, 0x24, /* mov    %cs,0x24(%eax) */
        0x8c, 0x58, 0x28, /* mov    %ds,0x28(%eax) */
        0x8c, 0x40, 0x2c, /* mov    %es,0x2c(%eax) */
        0x8c, 0x60, 0x30, /* mov    %fs,0x30(%eax) */
        0x8c, 0x68, 0x34, /* mov    %gs,0x34(%eax) */
        0x8c, 0x50, 0x38, /* mov    %ss,0x38(%eax) */
        0x9c,             /* pushf */
        0x8f, 0x40, 0x3c, /* popl   0x3c(%eax) */
        0xff, 0x75, 0x00, /* pushl  0x0(%ebp) ; previous stack frame */
        0x8f, 0x40, 0x40, /* popl   0x40(%eax) */
                          /* pushl $0x47f */
        0x68, 0x7f, 0x04, 0x00, 0x00,
        0x8f, 0x40, 0x44, /* popl  0x44(%eax) */
        0xd9, 0x68, 0x44, /* fldcw 0x44(%eax) */

        0x8b, 0x00,       /* mov    (%eax),%eax */
        0xff, 0x75, 0x14, /* pushl  0x14(%ebp) */
        0xff, 0x75, 0x10, /* pushl  0x10(%ebp) */
        0xff, 0x55, 0x0c, /* call   *0xc(%ebp) */

        0x8b, 0x55, 0x18, /* mov   0x18(%ebp),%edx */
        0x9b, 0xd9, 0x3a, /* fstcw (%edx) */
        0xdb, 0xe3,       /* fninit */

        0xc9,             /* leave */
        0xc3,             /* ret */
    };

    memcpy( func_ptr, call_func, sizeof(call_func) );

#define COMPARE(reg) \
    ok( context.reg == expect.reg, "wrong " #reg " %08lx/%08lx\n", context.reg, expect.reg )

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    func_ptr( &expect, pRtlCaptureContext, &context, 0, &new_x87_control );
    trace( "expect: eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx "
           "eip=%08lx cs=%04lx ds=%04lx es=%04lx fs=%04lx gs=%04lx ss=%04lx flags=%08lx prev=%08lx\n",
           expect.Eax, expect.Ebx, expect.Ecx, expect.Edx, expect.Esi, expect.Edi,
           expect.Ebp, expect.Esp, expect.Eip, expect.SegCs, expect.SegDs, expect.SegEs,
           expect.SegFs, expect.SegGs, expect.SegSs, expect.EFlags, expect.prev_frame );
    trace( "actual: eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx "
           "eip=%08lx cs=%04lx ds=%04lx es=%04lx fs=%04lx gs=%04lx ss=%04lx flags=%08lx\n",
           context.Eax, context.Ebx, context.Ecx, context.Edx, context.Esi, context.Edi,
           context.Ebp, context.Esp, context.Eip, context.SegCs, context.SegDs, context.SegEs,
           context.SegFs, context.SegGs, context.SegSs, context.EFlags );

    ok( context.ContextFlags == (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS) ||
        broken( context.ContextFlags == 0xcccccccc ),  /* <= vista */
        "wrong flags %08lx\n", context.ContextFlags );
    COMPARE( Eax );
    COMPARE( Ebx );
    COMPARE( Ecx );
    COMPARE( Edx );
    COMPARE( Esi );
    COMPARE( Edi );
    COMPARE( Eip );
    COMPARE( SegCs );
    COMPARE( SegDs );
    COMPARE( SegEs );
    COMPARE( SegFs );
    COMPARE( SegGs );
    COMPARE( SegSs );
    COMPARE( EFlags );
    /* Ebp is from the previous stackframe */
    ok( context.Ebp == expect.prev_frame, "wrong Ebp %08lx/%08lx\n", context.Ebp, expect.prev_frame );
    /* Esp is the value on entry to the previous stackframe */
    ok( context.Esp == expect.Ebp + 8, "wrong Esp %08lx/%08lx\n", context.Esp, expect.Ebp + 8 );

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT;

    status = func_ptr( &expect, pNtGetContextThread, (void *)GetCurrentThread(), &context, &new_x87_control );
    ok( status == STATUS_SUCCESS, "NtGetContextThread failed %08lx\n", status );
    trace( "expect: eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx "
           "eip=%08lx cs=%04lx ds=%04lx es=%04lx fs=%04lx gs=%04lx ss=%04lx flags=%08lx prev=%08lx\n",
           expect.Eax, expect.Ebx, expect.Ecx, expect.Edx, expect.Esi, expect.Edi,
           expect.Ebp, expect.Esp, expect.Eip, expect.SegCs, expect.SegDs, expect.SegEs,
           expect.SegFs, expect.SegGs, expect.SegSs, expect.EFlags, expect.prev_frame );
    trace( "actual: eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx "
           "eip=%08lx cs=%04lx ds=%04lx es=%04lx fs=%04lx gs=%04lx ss=%04lx flags=%08lx\n",
           context.Eax, context.Ebx, context.Ecx, context.Edx, context.Esi, context.Edi,
           context.Ebp, context.Esp, context.Eip, context.SegCs, context.SegDs, context.SegEs,
           context.SegFs, context.SegGs, context.SegSs, context.EFlags );
    /* Eax, Ecx, Edx, EFlags are not preserved */
    COMPARE( Ebx );
    COMPARE( Esi );
    COMPARE( Edi );
    COMPARE( Ebp );
    /* Esp is the stack upon entry to NtGetContextThread */
    ok( context.Esp == expect.Esp - 12 || context.Esp == expect.Esp - 16,
        "wrong Esp %08lx/%08lx\n", context.Esp, expect.Esp );
    /* Eip is somewhere close to the NtGetContextThread implementation */
    ok( (char *)context.Eip >= (char *)pNtGetContextThread - 0x40000 &&
        (char *)context.Eip <= (char *)pNtGetContextThread + 0x40000,
        "wrong Eip %08lx/%08lx\n", context.Eip, (DWORD)pNtGetContextThread );
    /* segment registers clear the high word */
    ok( context.SegCs == LOWORD(expect.SegCs), "wrong SegCs %08lx/%08lx\n", context.SegCs, expect.SegCs );
    ok( context.SegDs == LOWORD(expect.SegDs), "wrong SegDs %08lx/%08lx\n", context.SegDs, expect.SegDs );
    ok( context.SegEs == LOWORD(expect.SegEs), "wrong SegEs %08lx/%08lx\n", context.SegEs, expect.SegEs );
    ok( context.SegFs == LOWORD(expect.SegFs), "wrong SegFs %08lx/%08lx\n", context.SegFs, expect.SegFs );
    if (LOWORD(expect.SegGs)) ok( context.SegGs == LOWORD(expect.SegGs), "wrong SegGs %08lx/%08lx\n", context.SegGs, expect.SegGs );
    ok( context.SegSs == LOWORD(expect.SegSs), "wrong SegSs %08lx/%08lx\n", context.SegSs, expect.SegSs );

    ok( LOWORD(context.FloatSave.ControlWord) == LOWORD(expect.x87_control),
            "wrong x87 control word %#lx/%#lx.\n", context.FloatSave.ControlWord, expect.x87_control );
    ok( LOWORD(expect.x87_control) == LOWORD(new_x87_control),
            "x87 control word changed in NtGetContextThread() %#x/%#x.\n",
            LOWORD(expect.x87_control), LOWORD(new_x87_control) );

#undef COMPARE
}

static BYTE saved_KiUserExceptionDispatcher_bytes[7];
static BOOL hook_called;
static void *hook_KiUserExceptionDispatcher_eip;
static void *dbg_except_continue_handler_eip;
static void *hook_exception_address;

static struct
{
    DWORD old_eax;
    DWORD old_edx;
    DWORD old_esi;
    DWORD old_edi;
    DWORD old_ebp;
    DWORD old_esp;
    DWORD new_eax;
    DWORD new_edx;
    DWORD new_esi;
    DWORD new_edi;
    DWORD new_ebp;
    DWORD new_esp;
}
test_kiuserexceptiondispatcher_regs;

static DWORD dbg_except_continue_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    ok(hook_called, "Hook was not called.\n");

    ok(rec->ExceptionCode == 0x80000003, "Got unexpected ExceptionCode %#lx.\n", rec->ExceptionCode);

    got_exception = 1;
    dbg_except_continue_handler_eip = (void *)context->Eip;
    ++context->Eip;

    context->Eip = (DWORD)code_mem + 0x1c;
    context->Eax = 0xdeadbeef;
    context->Esi = 0xdeadbeef;
    pRtlUnwind(NtCurrentTeb()->Tib.ExceptionList, (void *)context->Eip, rec, (void *)0xdeadbeef);
    return ExceptionContinueExecution;
}

static LONG WINAPI dbg_except_continue_vectored_handler(struct _EXCEPTION_POINTERS *e)
{
    EXCEPTION_RECORD *rec = e->ExceptionRecord;
    CONTEXT *context = e->ContextRecord;

    trace("dbg_except_continue_vectored_handler, code %#lx, eip %#lx, ExceptionAddress %p.\n",
            rec->ExceptionCode, context->Eip, rec->ExceptionAddress);

    ok(rec->ExceptionCode == 0x80000003, "Got unexpected ExceptionCode %#lx.\n", rec->ExceptionCode);

    got_exception = 1;

    if ((ULONG_PTR)rec->ExceptionAddress == context->Eip + 1)
    {
        /* XP and Vista+ have ExceptionAddress == Eip + 1, Eip is adjusted even
         * for software raised breakpoint exception.
         * Win2003 has Eip not adjusted and matching ExceptionAddress.
         * Win2008 has Eip not adjusted and ExceptionAddress not filled for
         * software raised exception. */
        context->Eip = (ULONG_PTR)rec->ExceptionAddress;
    }

    return EXCEPTION_CONTINUE_EXECUTION;
}

/* Use CDECL to leave arguments on stack. */
static void * CDECL hook_KiUserExceptionDispatcher(EXCEPTION_RECORD *rec, CONTEXT *context)
{
    CONTEXT_EX *xctx = (CONTEXT_EX *)(context + 1);

    trace( "rec %p context %p context->Eip %#lx, context->Esp %#lx (%x), ContextFlags %#lx.\n",
           rec, context, context->Eip, context->Esp,
           (char *)context->Esp - (char *)&rec, context->ContextFlags);
    trace( "xstate %lx = %p (%x) %lx\n", xctx->XState.Offset, (char *)xctx + xctx->XState.Offset,
           (char *)xctx + xctx->XState.Offset - (char *)&rec, xctx->XState.Length );

    ok( (char *)rec->ExceptionInformation <= (char *)context &&
        (char *)(rec + 1) >= (char *)context, "wrong ptrs %p / %p\n", rec, context );
    ok( xctx->All.Offset == -sizeof(CONTEXT), "wrong All.Offset %lx\n", xctx->All.Offset );
    ok( xctx->All.Length >= sizeof(CONTEXT) + sizeof(CONTEXT_EX), "wrong All.Length %lx\n", xctx->All.Length );
    ok( xctx->Legacy.Offset == -sizeof(CONTEXT), "wrong Legacy.Offset %lx\n", xctx->All.Offset );
    ok( xctx->Legacy.Length == sizeof(CONTEXT), "wrong Legacy.Length %lx\n", xctx->All.Length );

    hook_called = TRUE;
    hook_KiUserExceptionDispatcher_eip = (void *)context->Eip;
    hook_exception_address = rec->ExceptionAddress;
    memcpy(pKiUserExceptionDispatcher, saved_KiUserExceptionDispatcher_bytes,
            sizeof(saved_KiUserExceptionDispatcher_bytes));
    return pKiUserExceptionDispatcher;
}

static void test_KiUserExceptionDispatcher(void)
{
    PVOID vectored_handler;
    static BYTE except_code[] =
    {
        0xb9, /* mov imm32, %ecx */
        /* offset: 0x1 */
        0x00, 0x00, 0x00, 0x00,

        0x89, 0x01,       /* mov %eax, (%ecx) */
        0x89, 0x51, 0x04, /* mov %edx, 0x4(%ecx) */
        0x89, 0x71, 0x08, /* mov %esi, 0x8(%ecx) */
        0x89, 0x79, 0x0c, /* mov %edi, 0xc(%ecx) */
        0x89, 0x69, 0x10, /* mov %ebp, 0x10(%ecx) */
        0x89, 0x61, 0x14, /* mov %esp, 0x14(%ecx) */
        0x83, 0xc1, 0x18, /* add $0x18, %ecx */

        /* offset: 0x19 */
        0xcc,  /* int3 */

        0x0f, 0x0b, /* ud2, illegal instruction */

        /* offset: 0x1c */
        0xb9, /* mov imm32, %ecx */
        /* offset: 0x1d */
        0x00, 0x00, 0x00, 0x00,

        0x89, 0x01,       /* mov %eax, (%ecx) */
        0x89, 0x51, 0x04, /* mov %edx, 0x4(%ecx) */
        0x89, 0x71, 0x08, /* mov %esi, 0x8(%ecx) */
        0x89, 0x79, 0x0c, /* mov %edi, 0xc(%ecx) */
        0x89, 0x69, 0x10, /* mov %ebp, 0x10(%ecx) */
        0x89, 0x61, 0x14, /* mov %esp, 0x14(%ecx) */
        0x8b, 0x71, 0xf0, /* mov -0x10(%ecx),%esi */

        0xc3,  /* ret  */
    };
    static BYTE hook_trampoline[] =
    {
        0xff, 0x15,
        /* offset: 2 bytes */
        0x00, 0x00, 0x00, 0x00,     /* call *addr */ /* call hook implementation. */
        0xff, 0xe0,                 /* jmp *%eax */
    };
    void *phook_KiUserExceptionDispatcher = hook_KiUserExceptionDispatcher;
    BYTE patched_KiUserExceptionDispatcher_bytes[7];
    DWORD old_protect1, old_protect2;
    EXCEPTION_RECORD record;
    void *bpt_address;
    BYTE *ptr;
    BOOL ret;

    if (!pRtlUnwind)
    {
        win_skip("RtlUnwind is not available.\n");
        return;
    }

#ifdef __REACTOS__ // This hangs on Windows 10 WOW64
    if (is_wow64)
    {
        win_skip("Skipping test that hangs on WOW64\n");
        return;
    }
#endif

    *(DWORD *)(except_code + 1) = (DWORD)&test_kiuserexceptiondispatcher_regs;
    *(DWORD *)(except_code + 0x1d) = (DWORD)&test_kiuserexceptiondispatcher_regs.new_eax;

    *(unsigned int *)(hook_trampoline + 2) = (ULONG_PTR)&phook_KiUserExceptionDispatcher;

    ret = VirtualProtect(hook_trampoline, ARRAY_SIZE(hook_trampoline), PAGE_EXECUTE_READWRITE, &old_protect1);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    ret = VirtualProtect(pKiUserExceptionDispatcher, sizeof(saved_KiUserExceptionDispatcher_bytes),
            PAGE_EXECUTE_READWRITE, &old_protect2);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    memcpy(saved_KiUserExceptionDispatcher_bytes, pKiUserExceptionDispatcher,
            sizeof(saved_KiUserExceptionDispatcher_bytes));

    ptr = patched_KiUserExceptionDispatcher_bytes;
    /* mov $hook_trampoline, %eax */
    *ptr++ = 0xb8;
    *(void **)ptr = hook_trampoline;
    ptr += sizeof(void *);
    /* jmp *eax */
    *ptr++ = 0xff;
    *ptr++ = 0xe0;

    memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
            sizeof(patched_KiUserExceptionDispatcher_bytes));
    got_exception = 0;
    run_exception_test(dbg_except_continue_handler, NULL, except_code, sizeof(except_code),
            PAGE_EXECUTE_READ);

    ok(got_exception, "Handler was not called.\n");
    ok(hook_called, "Hook was not called.\n");

    ok(test_kiuserexceptiondispatcher_regs.new_eax == 0xdeadbeef, "Got unexpected eax %#lx.\n",
            test_kiuserexceptiondispatcher_regs.new_eax);
    ok(test_kiuserexceptiondispatcher_regs.new_esi == 0xdeadbeef, "Got unexpected esi %#lx.\n",
            test_kiuserexceptiondispatcher_regs.new_esi);
    ok(test_kiuserexceptiondispatcher_regs.old_edi
            == test_kiuserexceptiondispatcher_regs.new_edi, "edi does not match.\n");
    ok(test_kiuserexceptiondispatcher_regs.old_ebp
            == test_kiuserexceptiondispatcher_regs.new_ebp, "ebp does not match.\n");

    bpt_address = (BYTE *)code_mem + 0x19;

    ok(hook_exception_address == bpt_address || broken(!hook_exception_address) /* Win2008 */,
            "Got unexpected exception address %p, expected %p.\n",
            hook_exception_address, bpt_address);
    ok(hook_KiUserExceptionDispatcher_eip == bpt_address, "Got unexpected exception address %p, expected %p.\n",
            hook_KiUserExceptionDispatcher_eip, bpt_address);
    ok(dbg_except_continue_handler_eip == bpt_address, "Got unexpected exception address %p, expected %p.\n",
            dbg_except_continue_handler_eip, bpt_address);

    record.ExceptionCode = 0x80000003;
    record.ExceptionFlags = 0;
    record.ExceptionRecord = NULL;
    record.ExceptionAddress = NULL; /* does not matter, copied return address */
    record.NumberParameters = 0;

    vectored_handler = AddVectoredExceptionHandler(TRUE, dbg_except_continue_vectored_handler);

    memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
            sizeof(patched_KiUserExceptionDispatcher_bytes));
    got_exception = 0;
    hook_called = FALSE;

    pRtlRaiseException(&record);

    ok(got_exception, "Handler was not called.\n");
    ok(hook_called || broken(!hook_called) /* 2003 */, "Hook was not called.\n");

    memcpy(pKiUserExceptionDispatcher, saved_KiUserExceptionDispatcher_bytes,
            sizeof(saved_KiUserExceptionDispatcher_bytes));

    RemoveVectoredExceptionHandler(vectored_handler);
    ret = VirtualProtect(pKiUserExceptionDispatcher, sizeof(saved_KiUserExceptionDispatcher_bytes),
            old_protect2, &old_protect2);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = VirtualProtect(hook_trampoline, ARRAY_SIZE(hook_trampoline), old_protect1, &old_protect1);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
}

static BYTE saved_KiUserApcDispatcher[7];

static void * CDECL hook_KiUserApcDispatcher( void *func, ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3 )
{
    CONTEXT *context = (CONTEXT *)((ULONG_PTR)&arg3 + sizeof(ULONG));

    ok( func == apc_func, "wrong function %p / %p\n", func, apc_func );
    ok( arg1 == 0x1234 + apc_count, "wrong arg1 %Ix\n", arg1 );
    ok( arg2 == 0x5678, "wrong arg2 %Ix\n", arg2 );
    ok( arg3 == 0xdeadbeef, "wrong arg3 %Ix\n", arg3 );

    if (context->ContextFlags != 1)
    {
        trace( "context %p eip %lx ebp %lx esp %lx (%x)\n",
               context, context->Eip, context->Ebp, context->Esp, (char *)context->Esp - (char *)&func );
    }
    else  /* new style with alertable arg and CONTEXT_EX */
    {
        CONTEXT_EX *xctx;
        ULONG *alertable = (ULONG *)context;

        context = (CONTEXT *)(alertable + 1);
        xctx = (CONTEXT_EX *)(context + 1);

        trace( "alertable %lx context %p eip %lx ebp %lx esp %lx (%x)\n", *alertable,
               context, context->Eip, context->Ebp, context->Esp, (char *)context->Esp - (char *)&func );
        if ((void *)(xctx + 1) < (void *)context->Esp)
        {
            ok( xctx->All.Offset == -sizeof(CONTEXT), "wrong All.Offset %lx\n", xctx->All.Offset );
            ok( xctx->All.Length >= sizeof(CONTEXT) + sizeof(CONTEXT_EX), "wrong All.Length %lx\n", xctx->All.Length );
            ok( xctx->Legacy.Offset == -sizeof(CONTEXT), "wrong Legacy.Offset %lx\n", xctx->All.Offset );
            ok( xctx->Legacy.Length == sizeof(CONTEXT), "wrong Legacy.Length %lx\n", xctx->All.Length );
        }

        if (apc_count) *alertable = 0;
        pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234 + apc_count + 1, 0x5678, 0xdeadbeef );
    }

    hook_called = TRUE;
    memcpy( pKiUserApcDispatcher, saved_KiUserApcDispatcher, sizeof(saved_KiUserApcDispatcher));
    return pKiUserApcDispatcher;
}

static void test_KiUserApcDispatcher(void)
{
    BYTE hook_trampoline[] =
    {
        0xff, 0x15,
        /* offset: 2 bytes */
        0x00, 0x00, 0x00, 0x00,     /* call *addr */ /* call hook implementation. */
        0xff, 0xe0,                 /* jmp *%eax */
    };

    BYTE patched_KiUserApcDispatcher[7];
    void *phook_KiUserApcDispatcher = hook_KiUserApcDispatcher;
    DWORD old_protect;
    BYTE *ptr;
    BOOL ret;

    *(ULONG_PTR *)(hook_trampoline + 2) = (ULONG_PTR)&phook_KiUserApcDispatcher;
    memcpy(code_mem, hook_trampoline, sizeof(hook_trampoline));

    ret = VirtualProtect( pKiUserApcDispatcher, sizeof(saved_KiUserApcDispatcher),
                          PAGE_EXECUTE_READWRITE, &old_protect );
    ok( ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError() );

    memcpy( saved_KiUserApcDispatcher, pKiUserApcDispatcher, sizeof(saved_KiUserApcDispatcher) );
    ptr = patched_KiUserApcDispatcher;
    /* mov $hook_trampoline, %eax */
    *ptr++ = 0xb8;
    *(void **)ptr = code_mem;
    ptr += sizeof(void *);
    /* jmp *eax */
    *ptr++ = 0xff;
    *ptr++ = 0xe0;
    memcpy( pKiUserApcDispatcher, patched_KiUserApcDispatcher, sizeof(patched_KiUserApcDispatcher) );

    apc_count = 0;
    hook_called = FALSE;
    pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234, 0x5678, 0xdeadbeef );
    SleepEx( 0, TRUE );
    ok( apc_count == 1 || apc_count == 2, "APC count %u\n", apc_count );
    ok( hook_called, "hook was not called\n" );

    if (apc_count == 2)
    {
        memcpy( pKiUserApcDispatcher, patched_KiUserApcDispatcher, sizeof(patched_KiUserApcDispatcher) );
        pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234 + apc_count, 0x5678, 0xdeadbeef );
        SleepEx( 0, TRUE );
        ok( apc_count == 3, "APC count %u\n", apc_count );
        SleepEx( 0, TRUE );
        ok( apc_count == 4, "APC count %u\n", apc_count );
    }
    VirtualProtect( pKiUserApcDispatcher, sizeof(saved_KiUserApcDispatcher), old_protect, &old_protect );
}

static void CDECL hook_KiUserCallbackDispatcher( void *eip, ULONG id, ULONG *args, ULONG len,
                                                 ULONG unk1, ULONG unk2, ULONG arg0, ULONG arg1 )
{
    KERNEL_CALLBACK_PROC func = NtCurrentTeb()->Peb->KernelCallbackTable[id];

    trace( "eip %p id %lx args %p (%x) len %lx unk1 %lx unk2 %lx args %lx,%lx\n",
           eip, id, args, (char *)args - (char *)&eip, len, unk1, unk2, arg0, arg1 );

    if (args[0] != arg0)  /* new style with extra esp */
    {
        void *esp = (void *)arg0;

        ok( args[0] == arg1, "wrong arg1 %lx / %lx\n", args[0], arg1 );
        ok( (char *)esp - ((char *)args + len) < 0x10, "wrong esp offset %p / %p\n", esp, args );
    }

    if (eip && pRtlPcToFileHeader)
    {
        void *mod, *win32u = GetModuleHandleA("win32u.dll");

        pRtlPcToFileHeader( eip, &mod );
        if (win32u) ok( mod == win32u, "ret address %p not in win32u %p\n", eip, win32u );
        else trace( "ret address %p in %p\n", eip, mod );
    }
    NtCallbackReturn( NULL, 0, func( args, len ));
}

static void test_KiUserCallbackDispatcher(void)
{
    BYTE saved_code[7], patched_code[7];
    DWORD old_protect;
    BYTE *ptr;
    BOOL ret;

    ret = VirtualProtect( pKiUserCallbackDispatcher, sizeof(saved_code),
                          PAGE_EXECUTE_READWRITE, &old_protect );
    ok( ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError() );

    memcpy( saved_code, pKiUserCallbackDispatcher, sizeof(saved_code) );
    ptr = patched_code;
    /* mov $hook_trampoline, %eax */
    *ptr++ = 0xb8;
    *(void **)ptr = hook_KiUserCallbackDispatcher;
    ptr += sizeof(void *);
    /* call *eax */
    *ptr++ = 0xff;
    *ptr++ = 0xd0;
    memcpy( pKiUserCallbackDispatcher, patched_code, sizeof(patched_code) );

    DestroyWindow( CreateWindowA( "Static", "test", 0, 0, 0, 0, 0, 0, 0, 0, 0 ));

    memcpy( pKiUserCallbackDispatcher, saved_code, sizeof(saved_code));
    VirtualProtect( pKiUserCallbackDispatcher, sizeof(saved_code), old_protect, &old_protect );
}

static void test_instrumentation_callback(void)
{
    static const BYTE instrumentation_callback[] =
    {
        0xff, 0x05, /* inc instrumentation_call_count */
                /* &instrumentation_call_count, offset 2 */ 0x00, 0x00, 0x00, 0x00,
        0xff, 0xe1, /* jmp *ecx */
    };

    unsigned int instrumentation_call_count;
    NTSTATUS status;

    PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION info;

    memcpy( code_mem, instrumentation_callback, sizeof(instrumentation_callback) );
    *(volatile void **)((char *)code_mem + 2) = &instrumentation_call_count;

    memset(&info, 0, sizeof(info));
    /* On 32 bit the structure is never used and just a callback pointer is expected. */
    info.Version = (ULONG_PTR)code_mem;
    instrumentation_call_count = 0;
    status = NtSetInformationProcess( GetCurrentProcess(), ProcessInstrumentationCallback, &info, sizeof(info) );
    ok( status == STATUS_SUCCESS || status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_NOT_SUPPORTED
            || broken( status == STATUS_PRIVILEGE_NOT_HELD ) /* some versions and machines before Win10 */,
            "got %#lx.\n", status );
    if (status)
    {
        win_skip( "Failed setting instrumenation callback.\n" );
        return;
    }
    DestroyWindow( CreateWindowA( "Static", "test", 0, 0, 0, 0, 0, 0, 0, 0, 0 ));
    todo_wine ok( instrumentation_call_count, "got %u.\n", instrumentation_call_count );

    memset(&info, 0, sizeof(info));
    instrumentation_call_count = 0;
    status = NtSetInformationProcess( GetCurrentProcess(), ProcessInstrumentationCallback, &info, sizeof(info) );
    ok( status == STATUS_SUCCESS, "got %#lx.\n", status );
    ok( !instrumentation_call_count, "got %u.\n", instrumentation_call_count );
}

#elif defined(__x86_64__)

static LONG consolidate_dummy_called;
static PVOID CALLBACK test_consolidate_dummy(EXCEPTION_RECORD *rec)
{
    CONTEXT *ctx = (CONTEXT *)rec->ExceptionInformation[1];

    switch (InterlockedIncrement(&consolidate_dummy_called))
    {
    case 1:  /* RtlRestoreContext */
        ok(ctx->Rip == 0xdeadbeef, "RtlRestoreContext wrong Rip, expected: 0xdeadbeef, got: %Ix\n", ctx->Rip);
        ok( rec->ExceptionInformation[10] == -1, "wrong info %Ix\n", rec->ExceptionInformation[10] );
        break;
    case 2: /* RtlUnwindEx */
        ok(ctx->Rip != 0xdeadbeef, "RtlUnwindEx wrong Rip, got: %Ix\n", ctx->Rip );
        if (is_arm64ec)
        {
            DISPATCHER_CONTEXT_NONVOLREG_ARM64 *regs = (void *)rec->ExceptionInformation[10];
            _JUMP_BUFFER *buf = (void *)rec->ExceptionInformation[3];
            ARM64EC_NT_CONTEXT *ec_ctx = (ARM64EC_NT_CONTEXT *)ctx;
            int i;

            ok( rec->ExceptionInformation[10] != -1, "wrong info %Ix\n", rec->ExceptionInformation[10] );
            ok( regs->GpNvRegs[0] == buf->R12, "wrong reg X19, %Ix / %Ix\n", regs->GpNvRegs[0], buf->R12 );
            ok( regs->GpNvRegs[1] == buf->R13, "wrong reg X20, %Ix / %Ix\n", regs->GpNvRegs[1], buf->R13 );
            ok( regs->GpNvRegs[2] == buf->R14, "wrong reg X21, %Ix / %Ix\n", regs->GpNvRegs[2], buf->R14 );
            ok( regs->GpNvRegs[3] == buf->R15, "wrong reg X22, %Ix / %Ix\n", regs->GpNvRegs[3], buf->R15 );
            ok( regs->GpNvRegs[4] == 0,        "wrong reg X23, %Ix / 0\n",   regs->GpNvRegs[4] );
            ok( regs->GpNvRegs[5] == 0,        "wrong reg X24, %Ix / 0\n",   regs->GpNvRegs[5] );
            ok( regs->GpNvRegs[6] == buf->Rsi, "wrong reg X25, %Ix / %Ix\n", regs->GpNvRegs[6], buf->Rsi );
            ok( regs->GpNvRegs[7] == buf->Rdi, "wrong reg X26, %Ix / %Ix\n", regs->GpNvRegs[7], buf->Rdi );
            ok( regs->GpNvRegs[8] == buf->Rbx, "wrong reg X27, %Ix / %Ix\n", regs->GpNvRegs[8], buf->Rbx );
            ok( regs->GpNvRegs[9] == 0,        "wrong reg X28, %Ix / 0\n",   regs->GpNvRegs[9] );
            ok( regs->GpNvRegs[10] == buf->Rbp,"wrong reg X29, %Ix / %Ix\n", regs->GpNvRegs[10], buf->Rbp );
            for (i = 0; i < 8; i++)
                ok(regs->FpNvRegs[i] == ec_ctx->V[i + 8].D[0], "wrong reg D%u, expected: %g, got: %g\n",
                   i + 8, regs->FpNvRegs[i], ec_ctx->V[i + 8].D[0] );
        }
        else ok( rec->ExceptionInformation[10] == -1, "wrong info %Ix\n", rec->ExceptionInformation[10] );
        break;
    }
    return (PVOID)rec->ExceptionInformation[2];
}

static void test_restore_context(void)
{
    SETJMP_FLOAT128 *fltsave;
    EXCEPTION_RECORD rec;
    _JUMP_BUFFER buf;
    CONTEXT ctx;
    int i;
    LONG pass;

    if (!pRtlUnwindEx || !pRtlRestoreContext || !pRtlCaptureContext)
    {
        skip("RtlUnwindEx/RtlCaptureContext/RtlRestoreContext not found\n");
        return;
    }

    /* RtlRestoreContext(NULL, NULL); crashes on Windows */

    /* test simple case of capture and restore context */
    pass = 0;
    InterlockedIncrement(&pass); /* interlocked to prevent compiler from moving after capture */
    pRtlCaptureContext(&ctx);
    if (InterlockedIncrement(&pass) == 2) /* interlocked to prevent compiler from moving before capture */
    {
        pRtlRestoreContext(&ctx, NULL);
        ok(0, "shouldn't be reached\n");
    }
    else
        ok(pass < 4, "unexpected pass %ld\n", pass);

    /* test with jmp using RtlRestoreContext */
    pass = 0;
    InterlockedIncrement(&pass);
    RtlCaptureContext(&ctx);
    InterlockedIncrement(&pass); /* only called once */
    setjmp((_JBTYPE *)&buf);
    InterlockedIncrement(&pass);
    if (pass == 3)
    {
        rec.ExceptionCode = STATUS_LONGJUMP;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD64)&buf;

        ok(buf.FpCsr == 0x27f, "Got unexpected FpCsr %#x.\n", buf.FpCsr);
        buf.FpCsr = 0x7f;
        buf.MxCsr = 0x3f80;
        /* uses buf.Rip instead of ctx.Rip */
        pRtlRestoreContext(&ctx, &rec);
        ok(0, "shouldn't be reached\n");
    }
    else if (pass == 4)
    {
        ok(buf.Rbx == ctx.Rbx, "longjmp failed for Rbx, expected: %Ix, got: %Ix\n", buf.Rbx, ctx.Rbx);
        ok(buf.Rsp == ctx.Rsp, "longjmp failed for Rsp, expected: %Ix, got: %Ix\n", buf.Rsp, ctx.Rsp);
        ok(buf.Rbp == ctx.Rbp, "longjmp failed for Rbp, expected: %Ix, got: %Ix\n", buf.Rbp, ctx.Rbp);
        ok(buf.Rsi == ctx.Rsi, "longjmp failed for Rsi, expected: %Ix, got: %Ix\n", buf.Rsi, ctx.Rsi);
        ok(buf.Rdi == ctx.Rdi, "longjmp failed for Rdi, expected: %Ix, got: %Ix\n", buf.Rdi, ctx.Rdi);
        ok(buf.R12 == ctx.R12, "longjmp failed for R12, expected: %Ix, got: %Ix\n", buf.R12, ctx.R12);
        ok(buf.R13 == ctx.R13, "longjmp failed for R13, expected: %Ix, got: %Ix\n", buf.R13, ctx.R13);
        ok(buf.R14 == ctx.R14, "longjmp failed for R14, expected: %Ix, got: %Ix\n", buf.R14, ctx.R14);
        ok(buf.R15 == ctx.R15, "longjmp failed for R15, expected: %Ix, got: %Ix\n", buf.R15, ctx.R15);

        fltsave = &buf.Xmm6;
        for (i = 0; i < 10; i++)
        {
            ok(fltsave[i].Part[0] == ctx.FltSave.XmmRegisters[i + 6].Low,
                "longjmp failed for Xmm%d, expected %Ix, got %Ix\n", i + 6,
                fltsave[i].Part[0], ctx.FltSave.XmmRegisters[i + 6].Low);

            ok(fltsave[i].Part[1] == ctx.FltSave.XmmRegisters[i + 6].High,
                "longjmp failed for Xmm%d, expected %Ix, got %Ix\n", i + 6,
                fltsave[i].Part[1], ctx.FltSave.XmmRegisters[i + 6].High);
        }
        ok(ctx.FltSave.ControlWord == 0x7f, "Got unexpected float control word %#x.\n", ctx.FltSave.ControlWord);
        ok(ctx.MxCsr == 0x3f80, "Got unexpected MxCsr %#lx.\n", ctx.MxCsr);
        ok(ctx.FltSave.MxCsr == 0x3f80, "Got unexpected MxCsr %#lx.\n", ctx.FltSave.MxCsr);
        buf.FpCsr = 0x27f;
        buf.MxCsr = 0x1f80;
        pRtlRestoreContext(&ctx, &rec);
        ok(0, "shouldn't be reached\n");
    }
    else if (pass == 5)
    {
        ok(ctx.FltSave.ControlWord == 0x27f, "Got unexpected float control word %#x.\n", ctx.FltSave.ControlWord);
        ok(ctx.FltSave.MxCsr == 0x1f80, "Got unexpected MxCsr %#lx.\n", ctx.MxCsr);
    }
    else
        ok(0, "unexpected pass %ld\n", pass);

    /* test with jmp through RtlUnwindEx */
    pass = 0;
    InterlockedIncrement(&pass);
    pRtlCaptureContext(&ctx);
    InterlockedIncrement(&pass); /* only called once */
    setjmp((_JBTYPE *)&buf);
    InterlockedIncrement(&pass);
    if (pass == 3)
    {
        rec.ExceptionCode = STATUS_LONGJUMP;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD64)&buf;

        /* uses buf.Rip instead of bogus 0xdeadbeef */
        pRtlUnwindEx((void*)buf.Frame, (void*)0xdeadbeef, &rec, NULL, &ctx, NULL);
        ok(0, "shouldn't be reached\n");
    }
    else
        ok(pass == 4, "unexpected pass %ld\n", pass);


    /* test with consolidate */
    pass = 0;
    InterlockedIncrement(&pass);
    RtlCaptureContext(&ctx);
    InterlockedIncrement(&pass);
    if (pass == 2)
    {
        rec.ExceptionCode = STATUS_UNWIND_CONSOLIDATE;
        rec.NumberParameters = 3;
        rec.ExceptionInformation[0] = (DWORD64)test_consolidate_dummy;
        rec.ExceptionInformation[1] = (DWORD64)&ctx;
        rec.ExceptionInformation[2] = ctx.Rip;
        rec.ExceptionInformation[10] = -1;
        ctx.Rip = 0xdeadbeef;

        pRtlRestoreContext(&ctx, &rec);
        ok(0, "shouldn't be reached\n");
    }
    else if (pass == 3)
        ok(consolidate_dummy_called == 1, "test_consolidate_dummy not called\n");
    else
        ok(0, "unexpected pass %ld\n", pass);

    /* test with consolidate through RtlUnwindEx */
    pass = 0;
    InterlockedIncrement(&pass);
    pRtlCaptureContext(&ctx);
    InterlockedIncrement(&pass);
    if (pass == 2)
    {
        rec.ExceptionCode = STATUS_UNWIND_CONSOLIDATE;
        rec.NumberParameters = 4;
        rec.ExceptionInformation[0] = (DWORD64)test_consolidate_dummy;
        rec.ExceptionInformation[1] = (DWORD64)&ctx;
        rec.ExceptionInformation[2] = ctx.Rip;
        rec.ExceptionInformation[3] = (DWORD64)&buf;
        rec.ExceptionInformation[10] = -1;  /* otherwise it doesn't get set */
        ctx.Rip = 0xdeadbeef;
        /* uses consolidate callback Rip instead of bogus 0xdeadbeef */
        setjmp((_JBTYPE *)&buf);
        pRtlUnwindEx((void*)buf.Frame, (void*)0xdeadbeef, &rec, NULL, &ctx, NULL);
        ok(0, "shouldn't be reached\n");
    }
    else if (pass == 3)
        ok(consolidate_dummy_called == 2, "test_consolidate_dummy not called\n");
    else
        ok(0, "unexpected pass %ld\n", pass);
}

static int termination_handler_called;
static void WINAPI termination_handler(ULONG flags, ULONG64 frame)
{
    termination_handler_called++;

    ok(flags == 1 || broken(flags == 0x401), "flags = %lx\n", flags);
    ok(frame == 0x1234, "frame = %p\n", (void*)frame);
}

static void test___C_specific_handler(void)
{
    DISPATCHER_CONTEXT dispatch;
    EXCEPTION_RECORD rec;
    CONTEXT context;
    ULONG64 frame;
    EXCEPTION_DISPOSITION ret;
    SCOPE_TABLE scope_table;

    if (!p__C_specific_handler)
    {
        win_skip("__C_specific_handler not available\n");
        return;
    }

    memset(&rec, 0, sizeof(rec));
    rec.ExceptionFlags = EXCEPTION_UNWINDING;
    frame = 0x1234;
    memset(&dispatch, 0, sizeof(dispatch));
    dispatch.ImageBase = (ULONG_PTR)GetModuleHandleA(NULL);
    dispatch.ControlPc = dispatch.ImageBase + 0x200;
    dispatch.HandlerData = &scope_table;
    dispatch.ContextRecord = &context;
    scope_table.Count = 1;
    scope_table.ScopeRecord[0].BeginAddress = 0x200;
    scope_table.ScopeRecord[0].EndAddress = 0x400;
    scope_table.ScopeRecord[0].HandlerAddress = (ULONG_PTR)termination_handler-dispatch.ImageBase;
    scope_table.ScopeRecord[0].JumpTarget = 0;
    memset(&context, 0, sizeof(context));

    termination_handler_called = 0;
    ret = p__C_specific_handler(&rec, frame, &context, &dispatch);
    ok(ret == ExceptionContinueSearch, "__C_specific_handler returned %x\n", ret);
    ok(termination_handler_called == 1, "termination_handler_called = %d\n",
            termination_handler_called);
    ok(dispatch.ScopeIndex == 1, "dispatch.ScopeIndex = %ld\n", dispatch.ScopeIndex);

    ret = p__C_specific_handler(&rec, frame, &context, &dispatch);
    ok(ret == ExceptionContinueSearch, "__C_specific_handler returned %x\n", ret);
    ok(termination_handler_called == 1, "termination_handler_called = %d\n",
            termination_handler_called);
    ok(dispatch.ScopeIndex == 1, "dispatch.ScopeIndex = %ld\n", dispatch.ScopeIndex);
}

/* This is heavily based on the i386 exception tests. */
static const struct exception
{
    BYTE     code[40];      /* asm code */
    BYTE     offset;        /* offset of faulting instruction */
    BYTE     length;        /* length of faulting instruction */
    NTSTATUS status;        /* expected status code */
    DWORD    nb_params;     /* expected number of parameters */
    ULONG64  params[4];     /* expected parameters */
    NTSTATUS alt_status;    /* alternative status code */
    DWORD    alt_nb_params; /* alternative number of parameters */
    ULONG64  alt_params[4]; /* alternative parameters */
} exceptions[] =
{
/* 0 */
    /* test some privileged instructions */
    { { 0xfb, 0xc3 },  /* 0: sti; ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6c, 0xc3 },  /* 1: insb (%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6d, 0xc3 },  /* 2: insl (%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6e, 0xc3 },  /* 3: outsb (%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6f, 0xc3 },  /* 4: outsl (%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 5 */
    { { 0xe4, 0x11, 0xc3 },  /* 5: inb $0x11,%al; ret */
      0, 2, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe5, 0x11, 0xc3 },  /* 6: inl $0x11,%eax; ret */
      0, 2, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe6, 0x11, 0xc3 },  /* 7: outb %al,$0x11; ret */
      0, 2, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe7, 0x11, 0xc3 },  /* 8: outl %eax,$0x11; ret */
      0, 2, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xed, 0xc3 },  /* 9: inl (%dx),%eax; ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 10 */
    { { 0xee, 0xc3 },  /* 10: outb %al,(%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xef, 0xc3 },  /* 11: outl %eax,(%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xf4, 0xc3 },  /* 12: hlt; ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xfa, 0xc3 },  /* 13: cli; ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test iret to invalid selector */
    { { 0x6a, 0x00, 0x6a, 0x00, 0x6a, 0x00, 0xcf, 0x48, 0x83, 0xc4, 0x18, 0xc3 },
      /* 15: pushq $0; pushq $0; pushq $0; iret; addq $24,%rsp; ret */
      6, 1, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffffffffffff } },
/* 15 */
    /* test loading an invalid selector */
    { { 0xb8, 0xef, 0xbe, 0x00, 0x00, 0x8e, 0xe8, 0xc3 },  /* 16: mov $beef,%ax; mov %ax,%gs; ret */
      5, 2, STATUS_ACCESS_VIOLATION, 2, { 0, 0xbee8 } }, /* 0xbee8 or 0xffffffff */

    /* test overlong instruction (limit is 15 bytes) */
    { { 0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0xfa,0xc3 },
      0, 16, STATUS_ILLEGAL_INSTRUCTION, 0, { 0 },
      STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffffffffffff } },

    /* test invalid interrupt */
    { { 0xcd, 0xff, 0xc3 },   /* int $0xff; ret */
      0, 2, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffffffffffff } },

    /* test moves to/from Crx */
    { { 0x0f, 0x20, 0xc0, 0xc3 },  /* movl %cr0,%eax; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x20, 0xe0, 0xc3 },  /* movl %cr4,%eax; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 20 */
    { { 0x0f, 0x22, 0xc0, 0xc3 },  /* movl %eax,%cr0; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x22, 0xe0, 0xc3 },  /* movl %eax,%cr4; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test moves to/from Drx */
    { { 0x0f, 0x21, 0xc0, 0xc3 },  /* movl %dr0,%eax; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x21, 0xc8, 0xc3 },  /* movl %dr1,%eax; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x21, 0xf8, 0xc3 },  /* movl %dr7,%eax; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
/* 25 */
    { { 0x0f, 0x23, 0xc0, 0xc3 },  /* movl %eax,%dr0; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x23, 0xc8, 0xc3 },  /* movl %eax,%dr1; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x23, 0xf8, 0xc3 },  /* movl %eax,%dr7; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test memory reads */
    { { 0xa1, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffffffffffc,%eax; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffffffffffc } },
    { { 0xa1, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffffffffffd,%eax; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffffffffffd } },
/* 30 */
    { { 0xa1, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffffffffffe,%eax; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffffffffffe } },
    { { 0xa1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xffffffffffffffff,%eax; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffffffffffff } },

    /* test memory writes */
    { { 0xa3, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffffffffffc; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffffffffffc } },
    { { 0xa3, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffffffffffd; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffffffffffd } },
    { { 0xa3, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffffffffffe; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffffffffffe } },
/* 35 */
    { { 0xa3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xffffffffffffffff; ret */
      0, 9, STATUS_ACCESS_VIOLATION, 2, { 1, 0xffffffffffffffff } },

    /* test exception with cleared segment registers */
    { {
        0x8c, 0xc0, /* mov %es,%eax */
        0x50,       /* push %rax */
        0x8c, 0xd8, /* mov %ds,%eax */
        0x50,       /* push %rax */
        0x8c, 0xe0, /* mov %fs,%eax */
        0x50,       /* push %rax */
        0x8c, 0xe8, /* mov %gs,%eax */
        0x50,       /* push %rax */
        0x31, 0xc0, /* xor %eax,%eax */
        0x8e, 0xc0, /* mov %eax,%es */
        0x8e, 0xd8, /* mov %eax,%ds */
#if 0
        /* It is observed that fs/gs base is reset
           on some CPUs when setting the segment value
           even to 0 (regardless of CPU spec
           saying otherwise) and it is not currently
           handled in Wine.
           Disable this part to avoid crashing the test. */
        0x8e, 0xe0, /* mov %eax,%fs */
        0x8e, 0xe8, /* mov %eax,%gs */
#else
        0x90, 0x90, /* nop */
        0x90, 0x90, /* nop */
#endif
        0xfa,       /* cli */
        0x58,       /* pop %rax */
#if 0
        0x8e, 0xe8, /* mov %eax,%gs */
        0x58,       /* pop %rax */
        0x8e, 0xe0, /* mov %eax,%fs */
#else
        0x58,       /* pop %rax */
        0x90, 0x90, /* nop */
        0x90, 0x90, /* nop */
#endif
        0x58,       /* pop %rax */
        0x8e, 0xd8, /* mov %eax,%ds */
        0x58,       /* pop %rax */
        0x8e, 0xc0, /* mov %eax,%es */
        0xc3,       /* retq */
      }, 22, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    { { 0xf1, 0x90, 0xc3 },  /* icebp; nop; ret */
      1, 1, STATUS_SINGLE_STEP, 0 },
    { { 0xcd, 0x2c, 0xc3 },
      0, 2, STATUS_ASSERTION_FAILURE, 0 },
    { { 0xb8, 0xb8, 0xb8, 0xb8, 0xb8,          /* mov $0xb8b8b8b8, %eax */
        0xcd, 0x2d, 0xfa, 0xc3 },              /* int $0x2d; cli; ret */
      7, 1, STATUS_BREAKPOINT, 1, { 0xb8b8b8b8 } },
/* 40 */
    { { 0xb8, 0x01, 0x00, 0x00, 0x00,          /* mov $0x01, %eax */
        0xcd, 0x2d, 0xfa, 0xc3 },              /* int $0x2d; cli; ret */
      8, 0, STATUS_SUCCESS, 0 },
};

static int got_exception;

static void run_exception_test_flags(void *handler, const void* context,
                               const void *code, unsigned int code_size,
                               DWORD access, DWORD handler_flags)
{
    unsigned char buf[2 + 8 + 2 + 8 + 8];
    RUNTIME_FUNCTION runtime_func;
    UNWIND_INFO *unwind = (UNWIND_INFO *)buf;
    void (*func)(void) = code_mem;
    DWORD oldaccess, oldaccess2;

    runtime_func.BeginAddress = 0;
    runtime_func.EndAddress = code_size;
    runtime_func.UnwindData = 0x1000;

    unwind->Version = 1;
    unwind->Flags = handler_flags;
    unwind->SizeOfProlog = 0;
    unwind->CountOfCodes = 0;
    unwind->FrameRegister = 0;
    unwind->FrameOffset = 0;
    *(ULONG *)&buf[4] = 0x1010;
    *(const void **)&buf[8] = context;

    /* movabs $<handler>, %rax */
    buf[16] = 0x48;
    buf[17] = 0xb8;
    *(void **)&buf[18] = handler;
    /* jmp *%rax */
    buf[26] = 0xff;
    buf[27] = 0xe0;

    memcpy((unsigned char *)code_mem + 0x1000, buf, sizeof(buf));
    memcpy(code_mem, code, code_size);
    if(access)
        VirtualProtect(code_mem, code_size, access, &oldaccess);

    pRtlAddFunctionTable(&runtime_func, 1, (ULONG_PTR)code_mem);
    func();
    pRtlDeleteFunctionTable(&runtime_func);

    if(access)
        VirtualProtect(code_mem, code_size, oldaccess, &oldaccess2);
}

static void run_exception_test(void *handler, const void* context,
                               const void *code, unsigned int code_size,
                               DWORD access)
{
    run_exception_test_flags(handler, context, code, code_size, access, UNW_FLAG_EHANDLER);
}

static DWORD WINAPI prot_fault_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                        CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    const struct exception *except = *(const struct exception **)(dispatcher->HandlerData);
    unsigned int i, parameter_count, entry = except - exceptions;

    got_exception++;
    winetest_push_context( "%u: %lx", entry, rec->ExceptionCode );

    ok( rec->ExceptionCode == except->status ||
        (except->alt_status != 0 && rec->ExceptionCode == except->alt_status),
        "Wrong exception code %lx/%lx\n", rec->ExceptionCode, except->status );
    ok( context->Rip == (DWORD_PTR)code_mem + except->offset,
        "Unexpected eip %#Ix/%#Ix\n", context->Rip, (DWORD_PTR)code_mem + except->offset );
    ok( rec->ExceptionAddress == (char*)context->Rip ||
        (rec->ExceptionCode == STATUS_BREAKPOINT && rec->ExceptionAddress == (char*)context->Rip + 1),
        "Unexpected exception address %p/%p\n", rec->ExceptionAddress, (char*)context->Rip );

#ifndef __arm64ec__
    if (!is_arm64ec)
    {
        USHORT ds, es, fs, gs, ss;
#if defined(__REACTOS__) && defined(_MSC_VER)
        ds = __readsegds();
        es = __readseges();
        fs = __readsegfs();
        gs = __readseggs();
        ss = __readsegss();
#else
        __asm__ volatile( "movw %%ds,%0" : "=g" (ds) );
        __asm__ volatile( "movw %%es,%0" : "=g" (es) );
        __asm__ volatile( "movw %%fs,%0" : "=g" (fs) );
        __asm__ volatile( "movw %%gs,%0" : "=g" (gs) );
        __asm__ volatile( "movw %%ss,%0" : "=g" (ss) );
#endif
        ok( context->SegDs == ds || !ds, "ds %#x does not match %#x\n", context->SegDs, ds );
        ok( context->SegEs == es || !es, "es %#x does not match %#x\n", context->SegEs, es );
        ok( context->SegFs == fs || !fs, "fs %#x does not match %#x\n", context->SegFs, fs );
        ok( context->SegGs == gs || !gs, "gs %#x does not match %#x\n", context->SegGs, gs );
        ok( context->SegSs == ss, "ss %#x does not match %#x\n", context->SegSs, ss );
        ok( context->SegDs == context->SegSs,
            "ds %#x does not match ss %#x\n", context->SegDs, context->SegSs );
        ok( context->SegEs == context->SegSs,
            "es %#x does not match ss %#x\n", context->SegEs, context->SegSs );
        ok( context->SegGs == context->SegSs,
            "gs %#x does not match ss %#x\n", context->SegGs, context->SegSs );
        todo_wine ok( context->SegFs && context->SegFs != context->SegSs,
                      "got fs %#x\n", context->SegFs );
    }
#endif

    if (except->status == STATUS_BREAKPOINT && is_wow64)
        parameter_count = 1;
    else if (except->alt_status == 0 || rec->ExceptionCode != except->alt_status)
        parameter_count = except->nb_params;
    else
        parameter_count = except->alt_nb_params;

    ok( rec->NumberParameters == parameter_count,
        "Unexpected parameter count %lu/%u\n", rec->NumberParameters, parameter_count );

    /* Most CPUs (except Intel Core apparently) report a segment limit violation */
    /* instead of page faults for accesses beyond 0xffffffffffffffff */
    if (except->nb_params == 2 && except->params[1] >= 0xfffffffffffffffd)
    {
        if (rec->ExceptionInformation[0] == 0 && rec->ExceptionInformation[1] == 0xffffffffffffffff)
            goto skip_params;
    }

    /* Seems that both 0xbee8 and 0xfffffffffffffffff can be returned in windows */
    if (except->nb_params == 2 && rec->NumberParameters == 2
        && except->params[1] == 0xbee8 && rec->ExceptionInformation[1] == 0xffffffffffffffff
        && except->params[0] == rec->ExceptionInformation[0])
    {
        goto skip_params;
    }

    if (except->alt_status == 0 || rec->ExceptionCode != except->alt_status)
    {
        for (i = 0; i < rec->NumberParameters; i++)
            ok( rec->ExceptionInformation[i] == except->params[i],
                "Wrong parameter %d: %Ix/%Ix\n",
                i, rec->ExceptionInformation[i], except->params[i] );
    }
    else
    {
        for (i = 0; i < rec->NumberParameters; i++)
            ok( rec->ExceptionInformation[i] == except->alt_params[i],
                "Wrong parameter %d: %Ix/%Ix\n",
                i, rec->ExceptionInformation[i], except->alt_params[i] );
    }

skip_params:
    winetest_pop_context();

    context->Rip = (DWORD_PTR)code_mem + except->offset + except->length;
    return ExceptionContinueExecution;
}

static const BYTE segfault_code[5] =
{
    0x31, 0xc0, /* xor    %eax,%eax */
    0x8f, 0x00, /* popq   (%rax) - cause exception */
    0xc3        /* ret */
};

struct dbgreg_test
{
    ULONG_PTR dr0, dr1, dr2, dr3, dr6, dr7;
};
/* test handling of debug registers */
static DWORD WINAPI dreg_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                  CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    const struct dbgreg_test *test = *(const struct dbgreg_test **)dispatcher->HandlerData;

    context->Rip += 2;	/* Skips the popq (%rax) */
    context->Dr0 = test->dr0;
    context->Dr1 = test->dr1;
    context->Dr2 = test->dr2;
    context->Dr3 = test->dr3;
    context->Dr6 = test->dr6;
    context->Dr7 = test->dr7;
    return ExceptionContinueExecution;
}

#define CHECK_DEBUG_REG(n, m) \
    ok((ctx.Dr##n & m) == test->dr##n, "(%d) failed to set debug register " #n " to %p, got %p\n", \
       test_num, (void *)test->dr##n, (void *)ctx.Dr##n)

static int check_debug_registers(int test_num, const struct dbgreg_test *test)
{
    CONTEXT ctx;
    NTSTATUS status;

    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    status = pNtGetContextThread(GetCurrentThread(), &ctx);
    ok(status == STATUS_SUCCESS, "NtGetContextThread failed with %lx\n", status);

    if (!ctx.Dr0 && !ctx.Dr1 && !ctx.Dr2 && !ctx.Dr3 && !ctx.Dr6 && !ctx.Dr7)
    {
        win_skip( "debug registers broken\n" );
        return 0;
    }
    CHECK_DEBUG_REG(0, ~0);
    CHECK_DEBUG_REG(1, ~0);
    CHECK_DEBUG_REG(2, ~0);
    CHECK_DEBUG_REG(3, ~0);
    CHECK_DEBUG_REG(6, 0x0f);
    CHECK_DEBUG_REG(7, ~0xdc00);
    return 1;
}

static const BYTE single_stepcode[] =
{
    0x9c,               /* pushf */
    0x58,               /* pop   %rax */
    0x0d,0,1,0,0,       /* or    $0x100,%eax */
    0x50,               /* push   %rax */
    0x9d,               /* popf    */
    0x35,0,1,0,0,       /* xor    $0x100,%eax */
    0x50,               /* push   %rax */
    0x9d,               /* popf    */
    0x90,
    0xc3
};

/* test the single step exception behaviour */
static DWORD WINAPI single_step_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                         CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    got_exception++;
    ok (!(context->EFlags & 0x100), "eflags has single stepping bit set\n");

    if( got_exception < 3)
        context->EFlags |= 0x100;  /* single step until popf instruction */
    else {
        /* show that the last single step exception on the popf instruction
         * (which removed the TF bit), still is a EXCEPTION_SINGLE_STEP exception */
        ok( rec->ExceptionCode == EXCEPTION_SINGLE_STEP,
            "exception is not EXCEPTION_SINGLE_STEP: %lx\n", rec->ExceptionCode);
    }
    return ExceptionContinueExecution;
}

/* Test the alignment check (AC) flag handling. */
static const BYTE align_check_code[] =
{
    0x55,               /* push   %rbp */
    0x48,0x89,0xe5,     /* mov    %rsp,%rbp */
    0x9c,               /* pushf   */
    0x9c,               /* pushf   */
    0x58,               /* pop    %rax */
    0x0d,0,0,4,0,       /* or     $0x40000,%eax */
    0x50,               /* push   %rax */
    0x9d,               /* popf    */
    0x48,0x89,0xe0,     /* mov %rsp, %rax */
    0x8b,0x40,0x1,      /* mov 0x1(%rax), %eax - cause exception */
    0x9d,               /* popf    */
    0x5d,               /* pop    %rbp */
    0xc3,               /* ret     */
};

static DWORD WINAPI align_check_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                         CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
#ifdef __GNUC__
    __asm__ volatile( "pushfq; andl $~0x40000,(%rsp); popfq" );
#endif
    ok (context->EFlags & 0x40000, "eflags has AC bit unset\n");
    got_exception++;
    if (got_exception != 1)
    {
        ok(broken(1) /* win7 */, "exception should occur only once");
        context->EFlags &= ~0x40000;
    }
    return ExceptionContinueExecution;
}

/* Test the direction flag handling. */
static const BYTE direction_flag_code[] =
{
    0xfd,               /* std */
    0xfa,               /* cli - cause exception */
    0xc3,               /* ret     */
};

static DWORD WINAPI direction_flag_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                            CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
#ifdef __GNUC__
    ULONG_PTR flags;
    __asm__("pushfq; popq %0; cld" : "=r" (flags) );
    /* older windows versions don't clear DF properly so don't test */
    if (flags & 0x400) trace( "eflags has DF bit set\n" );
#endif
    ok( context->EFlags & 0x400, "context eflags has DF bit cleared\n" );
    got_exception++;
    context->Rip++;  /* skip cli */
    context->EFlags &= ~0x400;  /* make sure it is cleared on return */
    return ExceptionContinueExecution;
}

/* test single stepping over hardware breakpoint */
static const BYTE dummy_code[] = { 0x90, 0x90, 0x90, 0xc3 };  /* nop, nop, nop, ret */

static DWORD WINAPI bpx_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                 CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    got_exception++;
    ok( rec->ExceptionCode == EXCEPTION_SINGLE_STEP,
        "wrong exception code: %lx\n", rec->ExceptionCode);

    if(got_exception == 1) {
        /* hw bp exception on first nop */
        ok( (void *)context->Rip == code_mem, "rip is wrong: %p instead of %p\n",
            (void *)context->Rip, code_mem );
        ok( (context->Dr6 & 0xf) == 1, "B0 flag is not set in Dr6\n");
        ok( !(context->Dr6 & 0x4000), "BS flag is set in Dr6\n");
        context->Dr0 = context->Dr0 + 1;  /* set hw bp again on next instruction */
        context->EFlags |= 0x100;       /* enable single stepping */
    } else if (got_exception == 2) {
        /* single step exception on second nop */
        ok( (char *)context->Rip == (char *)code_mem + 1, "rip is wrong: %p instead of %p\n",
            (void *)context->Rip, (char *)code_mem + 1);
        ok( (context->Dr6 & 0x4000), "BS flag is not set in Dr6\n");
        context->EFlags |= 0x100;
    } else if( got_exception == 3) {
        /* hw bp exception on second nop */
        ok( (void *)context->Rip == (char *)code_mem + 1, "rip is wrong: %p instead of %p\n",
            (void *)context->Rip, (char *)code_mem + 1);
        ok( (context->Dr6 & 0xf) == 1, "B0 flag is not set in Dr6\n");
        ok( !(context->Dr6 & 0x4000), "BS flag is set in Dr6\n");
        context->Dr0 = 0;       /* clear breakpoint */
        context->EFlags |= 0x100;
    } else {
        /* single step exception on third nop */
        ok( (void *)context->Rip == (char *)code_mem + 2, "rip is wrong: %p instead of %p\n",
            (void *)context->Rip, (char *)code_mem + 2);
        ok( (context->Dr6 & 0xf) == 0, "B0...3 flags in Dr6 shouldn't be set\n");
        ok( (context->Dr6 & 0x4000), "BS flag is not set in Dr6\n");
    }

    context->Dr6 = 0;  /* clear status register */
    return ExceptionContinueExecution;
}

/* test int3 handling */
static const BYTE int3_code[] = { 0xcc, 0xc3 };  /* int 3, ret */

static DWORD WINAPI int3_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                  CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    ok( rec->ExceptionAddress == code_mem, "exception address not at: %p, but at %p\n",
                                           code_mem,  rec->ExceptionAddress);
    ok( (void *)context->Rip == code_mem, "rip not at: %p, but at %p\n", code_mem, (void *)context->Rip);
    if ((void *)context->Rip == code_mem) context->Rip++; /* skip breakpoint */

    return ExceptionContinueExecution;
}

/* trap unhandled exceptions */
static LONG CALLBACK exc_filter( EXCEPTION_POINTERS *ptrs )
{
    printf( "%04lx unhandled exception %08lx at %p rip %p eflags %lx\n",
            GetCurrentProcessId(),
            ptrs->ExceptionRecord->ExceptionCode, ptrs->ExceptionRecord->ExceptionAddress,
            (void *)ptrs->ContextRecord->Rip, ptrs->ContextRecord->EFlags );
    fflush( stdout );
    return EXCEPTION_EXECUTE_HANDLER;
}

static void test_exceptions(void)
{
    CONTEXT ctx;
    NTSTATUS res;
    struct dbgreg_test dreg_test;

    /* test handling of debug registers */
    memset(&dreg_test, 0, sizeof(dreg_test));

    dreg_test.dr0 = 0x42424240;
    dreg_test.dr2 = 0x126bb070;
    dreg_test.dr3 = 0x0badbad0;
    dreg_test.dr7 = 0xffff0115;
    run_exception_test(dreg_handler, &dreg_test, &segfault_code, sizeof(segfault_code), 0);
    if (check_debug_registers(1, &dreg_test))
    {
        dreg_test.dr0 = 0x42424242;
        dreg_test.dr2 = 0x100f0fe7;
        dreg_test.dr3 = 0x0abebabe;
        dreg_test.dr7 = 0x115;
        run_exception_test(dreg_handler, &dreg_test, &segfault_code, sizeof(segfault_code), 0);
        check_debug_registers(2, &dreg_test);

        /* test single stepping over hardware breakpoint */
        memset(&ctx, 0, sizeof(ctx));
        ctx.Dr0 = (ULONG_PTR)code_mem;  /* set hw bp on first nop */
        ctx.Dr7 = 1;
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        res = pNtSetContextThread( GetCurrentThread(), &ctx );
        ok( res == STATUS_SUCCESS, "NtSetContextThread failed with %lx\n", res);

        got_exception = 0;
        run_exception_test(bpx_handler, NULL, dummy_code, sizeof(dummy_code), 0);
        ok( got_exception == 4,"expected 4 exceptions, got %d\n", got_exception);
    }

#if defined(__REACTOS__)
    if (is_reactos())
    {
        skip("Skipping tests that crash\n");
        return;
    }
#endif

    /* test single stepping behavior */
    SetUnhandledExceptionFilter( exc_filter );
    got_exception = 0;
    run_exception_test(single_step_handler, NULL, &single_stepcode, sizeof(single_stepcode), 0);
    ok(got_exception == 3, "expected 3 single step exceptions, got %d\n", got_exception);

    /* test alignment exceptions */
    got_exception = 0;
    run_exception_test(align_check_handler, NULL, align_check_code, sizeof(align_check_code), 0);
    todo_wine
    ok(got_exception == 1 || broken(got_exception == 2) /* win7 */, "got %d alignment faults, expected 1\n", got_exception);

    /* test direction flag */
    got_exception = 0;
    run_exception_test(direction_flag_handler, NULL, direction_flag_code, sizeof(direction_flag_code), 0);
    ok(got_exception == 1, "got %d exceptions, expected 1\n", got_exception);
#ifndef __arm64ec__
#if defined(__REACTOS__ ) && defined(_MSC_VER)
    if (is_arm64ec) __cld();
#else
    if (is_arm64ec) __asm__ volatile( "cld" ); /* needed on Windows */
#endif
#endif

    /* test int3 handling */
    run_exception_test(int3_handler, NULL, int3_code, sizeof(int3_code), 0);

#ifndef __arm64ec__
    if (!is_arm64ec)
    {
        USHORT ds, es, fs, gs, ss;
        /* test segment registers */
        ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_SEGMENTS;
        res = pNtGetContextThread( GetCurrentThread(), &ctx );
        ok( res == STATUS_SUCCESS, "NtGetContextThread failed with %lx\n", res );
#if defined(__REACTOS__) && defined(_MSC_VER)
        ds = __readsegds();
        es = __readseges();
        fs = __readsegfs();
        gs = __readseggs();
        ss = __readsegss();
#else
        __asm__ volatile( "movw %%ds,%0" : "=g" (ds) );
        __asm__ volatile( "movw %%es,%0" : "=g" (es) );
        __asm__ volatile( "movw %%fs,%0" : "=g" (fs) );
        __asm__ volatile( "movw %%gs,%0" : "=g" (gs) );
        __asm__ volatile( "movw %%ss,%0" : "=g" (ss) );
#endif
        ok( ctx.SegDs == ds, "wrong ds %04x / %04x\n", ctx.SegDs, ds );
        ok( ctx.SegEs == es, "wrong es %04x / %04x\n", ctx.SegEs, es );
        ok( ctx.SegFs == fs, "wrong fs %04x / %04x\n", ctx.SegFs, fs );
        ok( ctx.SegGs == gs || !gs, "wrong gs %04x / %04x\n", ctx.SegGs, gs );
        ok( ctx.SegSs == ss, "wrong ss %04x / %04x\n", ctx.SegSs, ss );
        ok( ctx.SegDs == ctx.SegSs, "wrong ds %04x / %04x\n", ctx.SegDs, ctx.SegSs );
        ok( ctx.SegEs == ctx.SegSs, "wrong es %04x / %04x\n", ctx.SegEs, ctx.SegSs );
        ok( ctx.SegFs != ctx.SegSs, "wrong fs %04x / %04x\n", ctx.SegFs, ctx.SegSs );
        ok( ctx.SegGs == ctx.SegSs, "wrong gs %04x / %04x\n", ctx.SegGs, ctx.SegSs );
        ctx.SegDs = 0;
        ctx.SegEs = ctx.SegFs;
        ctx.SegFs = ctx.SegSs;
        res = pNtSetContextThread( GetCurrentThread(), &ctx );
        ok( res == STATUS_SUCCESS, "NtGetContextThread failed with %lx\n", res );
#if defined(__REACTOS__) && defined(_MSC_VER)
        ds = __readsegds();
        es = __readseges();
        fs = __readsegfs();
        gs = __readseggs();
        ss = __readsegss();
#else
        __asm__ volatile( "movw %%ds,%0" : "=g" (ds) );
        __asm__ volatile( "movw %%es,%0" : "=g" (es) );
        __asm__ volatile( "movw %%fs,%0" : "=g" (fs) );
        __asm__ volatile( "movw %%gs,%0" : "=g" (gs) );
        __asm__ volatile( "movw %%ss,%0" : "=g" (ss) );
#endif
        res = pNtGetContextThread( GetCurrentThread(), &ctx );
        ok( res == STATUS_SUCCESS, "NtGetContextThread failed with %lx\n", res );
        ok( ctx.SegDs == ds, "wrong ds %04x / %04x\n", ctx.SegDs, ds );
        ok( ctx.SegEs == es, "wrong es %04x / %04x\n", ctx.SegEs, es );
        ok( ctx.SegFs == fs, "wrong fs %04x / %04x\n", ctx.SegFs, fs );
        ok( ctx.SegGs == gs || !gs, "wrong gs %04x / %04x\n", ctx.SegGs, gs );
        ok( ctx.SegSs == ss, "wrong ss %04x / %04x\n", ctx.SegSs, ss );
        ok( ctx.SegDs == ctx.SegSs, "wrong ds %04x / %04x\n", ctx.SegDs, ctx.SegSs );
        ok( ctx.SegEs == ctx.SegSs, "wrong es %04x / %04x\n", ctx.SegEs, ctx.SegSs );
        ok( ctx.SegFs != ctx.SegSs, "wrong fs %04x / %04x\n", ctx.SegFs, ctx.SegSs );
        ok( ctx.SegGs == ctx.SegSs, "wrong gs %04x / %04x\n", ctx.SegGs, ctx.SegSs );
    }
#endif
}

static DWORD WINAPI simd_fault_handler( EXCEPTION_RECORD *rec, ULONG64 frame,
                                        CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    int *stage = *(int **)dispatcher->HandlerData;

    got_exception++;

    if (*stage == 1)
    {
        /* fault while executing sse instruction */
        context->Rip += 3; /* skip addps */
        return ExceptionContinueExecution;
    }
    else if (*stage == 2 || *stage == 3 )
    {
        /* stage 2 - divide by zero fault */
        /* stage 3 - invalid operation fault */
        if( rec->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION)
            skip("system doesn't support SIMD exceptions\n");
        else
        {
            ULONG expect = *stage == 2 ? EXCEPTION_FLT_DIVIDE_BY_ZERO : EXCEPTION_FLT_INVALID_OPERATION;
            ok( rec->ExceptionCode == expect, "exception code: %#lx, should be %#lx\n",
                rec->ExceptionCode, expect );
            ok( rec->NumberParameters == 2, "# of params: %li, should be 2\n", rec->NumberParameters);
            ok( rec->ExceptionInformation[0] == 0, "param #0: %Ix\n", rec->ExceptionInformation[0]);
            ok( rec->ExceptionInformation[1] == context->MxCsr, "param #1: %Ix / %lx\n",
                rec->ExceptionInformation[1], context->MxCsr);
        }
        context->Rip += 3; /* skip divps */
    }
    else
        ok(FALSE, "unexpected stage %x\n", *stage);

    return ExceptionContinueExecution;
}

static const BYTE simd_exception_test[] =
{
    0x48, 0x83, 0xec, 0x8,               /* sub $0x8, %rsp       */
    0x0f, 0xae, 0x1c, 0x24,              /* stmxcsr (%rsp)       */
    0x8b, 0x04, 0x24,                    /* mov    (%rsp),%eax   * store mxcsr */
    0x66, 0x81, 0x24, 0x24, 0xff, 0xfd,  /* andw $0xfdff,(%rsp)  * enable divide by */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%rsp)       * zero exceptions  */
    0xb9, 0x01, 0x00, 0x00, 0x00,        /* movl   $0x1,%ecx     */
    0x66, 0x48, 0x0f, 0x6e, 0xc9,        /* movq   %rcx,%xmm1    * fill dividend  */
    0x0f, 0x57, 0xc0,                    /* xorps  %xmm0,%xmm0   * clear divisor  */
    0x0f, 0x5e, 0xc8,                    /* divps  %xmm0,%xmm1   * generate fault */
    0x89, 0x04, 0x24,                    /* mov    %eax,(%rsp)   * restore to old mxcsr */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%rsp)       */
    0x48, 0x83, 0xc4, 0x08,              /* add    $0x8,%rsp     */
    0xc3,                                /* ret */
};

static const BYTE simd_exception_test2[] =
{
    0x48, 0x83, 0xec, 0x8,               /* sub $0x8, %rsp       */
    0x0f, 0xae, 0x1c, 0x24,              /* stmxcsr (%rsp)       */
    0x8b, 0x04, 0x24,                    /* mov    (%rsp),%eax   * store mxcsr */
    0x66, 0x81, 0x24, 0x24, 0x7f, 0xff,  /* andw $0xff7f,(%rsp)  * enable invalid       */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%rsp)       * operation exceptions */
    0x0f, 0x57, 0xc9,                    /* xorps  %xmm1,%xmm1   * clear dividend */
    0x0f, 0x57, 0xc0,                    /* xorps  %xmm0,%xmm0   * clear divisor  */
    0x0f, 0x5e, 0xc8,                    /* divps  %xmm0,%xmm1   * generate fault */
    0x89, 0x04, 0x24,                    /* mov    %eax,(%rsp)   * restore to old mxcsr */
    0x0f, 0xae, 0x14, 0x24,              /* ldmxcsr (%rsp)       */
    0x48, 0x83, 0xc4, 0x08,              /* add    $0x8,%rsp     */
    0xc3,                                /* ret */
};

static const BYTE sse_check[] =
{
    0x0f, 0x58, 0xc8,                    /* addps  %xmm0,%xmm1 */
    0xc3,                                /* ret */
};

static void test_simd_exceptions(void)
{
    int stage;

    /* test if CPU & OS can do sse */
    stage = 1;
    got_exception = 0;
    run_exception_test(simd_fault_handler, &stage, sse_check, sizeof(sse_check), 0);
    if(got_exception) {
        skip("system doesn't support SSE\n");
        return;
    }

    /* generate a SIMD exception */
    stage = 2;
    got_exception = 0;
    run_exception_test(simd_fault_handler, &stage, simd_exception_test,
                       sizeof(simd_exception_test), 0);
    ok(got_exception == 1, "got exception: %i, should be 1\n", got_exception);

    /* generate a SIMD exception, test FPE_FLTINV */
    stage = 3;
    got_exception = 0;
    run_exception_test(simd_fault_handler, &stage, simd_exception_test2,
                       sizeof(simd_exception_test2), 0);
    ok(got_exception == 1, "got exception: %i, should be 1\n", got_exception);
}

static void test_prot_fault(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(exceptions); i++)
    {
        got_exception = 0;
        run_exception_test(prot_fault_handler, &exceptions[i], &exceptions[i].code,
                           sizeof(exceptions[i].code), 0);
        ok( got_exception == (exceptions[i].status != 0),
            "%u: bad exception count %d\n", i, got_exception );
    }
}

static LONG CALLBACK dpe_handler(EXCEPTION_POINTERS *info)
{
    EXCEPTION_RECORD *rec = info->ExceptionRecord;
    DWORD old_prot;

    got_exception++;

    ok(rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION,
        "got %#lx\n", rec->ExceptionCode);
    ok(rec->NumberParameters == 2, "got %lu params\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT,
        "got %#Ix\n", rec->ExceptionInformation[0]);
    ok((void *)rec->ExceptionInformation[1] == code_mem,
        "got %p\n", (void *)rec->ExceptionInformation[1]);

    VirtualProtect(code_mem, 1, PAGE_EXECUTE_READWRITE, &old_prot);

    return EXCEPTION_CONTINUE_EXECUTION;
}

static void test_dpe_exceptions(void)
{
    static const BYTE ret[] = {0xc3};
    DWORD (CDECL *func)(void) = code_mem;
    DWORD old_prot, val = 0, len = 0xdeadbeef;
    NTSTATUS status;
    void *handler;

    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val, &len );
    ok( status == STATUS_SUCCESS || status == STATUS_INVALID_PARAMETER, "got status %08lx\n", status );
    if (!status)
    {
        ok( len == sizeof(val), "wrong len %lu\n", len );
        ok( val == (MEM_EXECUTE_OPTION_DISABLE | MEM_EXECUTE_OPTION_PERMANENT |
                    MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION),
            "wrong val %08lx\n", val );
    }
    else ok( len == 0xdeadbeef, "wrong len %lu\n", len );

    val = MEM_EXECUTE_OPTION_DISABLE;
    status = pNtSetInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val );
    ok( status == STATUS_INVALID_PARAMETER, "got status %08lx\n", status );

    memcpy(code_mem, ret, sizeof(ret));

    handler = pRtlAddVectoredExceptionHandler(TRUE, &dpe_handler);
    ok(!!handler, "RtlAddVectoredExceptionHandler failed\n");

    VirtualProtect(code_mem, 1, PAGE_NOACCESS, &old_prot);

    got_exception = 0;
    func();
    ok(got_exception == 1, "got %u exceptions\n", got_exception);

    VirtualProtect(code_mem, 1, old_prot, &old_prot);

    VirtualProtect(code_mem, 1, PAGE_READWRITE, &old_prot);

    got_exception = 0;
    func();
    ok(got_exception == 1, "got %u exceptions\n", got_exception);

    VirtualProtect(code_mem, 1, old_prot, &old_prot);

    pRtlRemoveVectoredExceptionHandler(handler);
}

static const BYTE call_one_arg_code[] = {
    0x48, 0x83, 0xec, 0x28, /* sub $0x28,%rsp */
    0x48, 0x89, 0xc8, /* mov %rcx,%rax */
    0x48, 0x89, 0xd1, /* mov %rdx,%rcx */
    0xff, 0xd0, /* callq *%rax */
    0x90, /* nop */
    0x90, /* nop */
    0x90, /* nop */
    0x90, /* nop */
    0x48, 0x83, 0xc4, 0x28, /* add $0x28,%rsp */
    0xc3, /* retq  */
};

static int rtlraiseexception_unhandled_handler_called;
static int rtlraiseexception_teb_handler_called;
static int rtlraiseexception_handler_called;

static void rtlraiseexception_handler_( EXCEPTION_RECORD *rec, void *frame, CONTEXT *context,
                                        void *dispatcher, BOOL unhandled_handler )
{
    void *addr = rec->ExceptionAddress;

    trace( "exception: %08lx flags:%lx addr:%p context: Rip:%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, (void *)context->Rip );

    if (is_arm64ec) /* addr points to RtlRaiseException entry thunk */
    {
        ok( ((ULONG *)addr)[-1] == 0xd63f0120 /* blr x9 */,
            "ExceptionAddress not in entry thunk %p (ntdll+%Ix)\n",
            addr, (char *)addr - (char *)hntdll );
        ok( context->ContextFlags == (CONTEXT_FULL | CONTEXT_UNWOUND_TO_CALL),
            "wrong context flags %lx\n", context->ContextFlags );
    }
    else
    {
        ok( addr == (char *)code_mem + 0x0c || broken( addr == code_mem || !addr ) /* 2008 */,
            "ExceptionAddress at %p instead of %p\n", addr, (char *)code_mem + 0x0c );
        ok( context->ContextFlags == CONTEXT_ALL || context->ContextFlags == (CONTEXT_ALL | CONTEXT_XSTATE)
            || context->ContextFlags == (CONTEXT_FULL | CONTEXT_SEGMENTS)
            || context->ContextFlags == (CONTEXT_FULL | CONTEXT_SEGMENTS | CONTEXT_XSTATE),
            "wrong context flags %lx\n", context->ContextFlags );
    }

    /* check that pc is fixed up only for EXCEPTION_BREAKPOINT
     * even if raised by RtlRaiseException
     */
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT && test_stage && !is_arm64ec)
        ok( context->Rip == (UINT_PTR)addr - 1,
            "%d: Rip at %Ix instead of %Ix\n", test_stage, context->Rip, (UINT_PTR)addr - 1 );
    else
        ok( context->Rip == (UINT_PTR)addr,
            "%d: Rip at %Ix instead of %Ix\n", test_stage, context->Rip, (UINT_PTR)addr );

    if (have_vectored_api) ok( context->Rax == 0xf00f00f0, "context->Rax is %Ix, should have been set to 0xf00f00f0 in vectored handler\n", context->Rax );
}

static LONG CALLBACK rtlraiseexception_unhandled_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT context = ExceptionInfo->ContextRecord;
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    rtlraiseexception_unhandled_handler_called = 1;
    rtlraiseexception_handler_(rec, NULL, context, NULL, TRUE);
    if (test_stage == STAGE_RTLRAISE_HANDLE_LAST_CHANCE) return EXCEPTION_CONTINUE_SEARCH;

    /* pc in context is decreased by 1
     * Increase it again, else execution will continue in the middle of an instruction */
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT && (context->Rip == (UINT_PTR)rec->ExceptionAddress - 1))
        context->Rip++;
    return EXCEPTION_CONTINUE_EXECUTION;
}

static DWORD WINAPI rtlraiseexception_teb_handler( EXCEPTION_RECORD *rec,
                                                   EXCEPTION_REGISTRATION_RECORD *frame,
                                                   CONTEXT *context,
                                                   EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    rtlraiseexception_teb_handler_called = 1;
    rtlraiseexception_handler_(rec, frame, context, dispatcher, FALSE);
    return ExceptionContinueSearch;
}

static DWORD WINAPI rtlraiseexception_handler( EXCEPTION_RECORD *rec, void *frame,
                                               CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    if (is_arm64ec)
    {
        ARM64EC_NT_CONTEXT *ec_ctx = (ARM64EC_NT_CONTEXT *)context;
        DISPATCHER_CONTEXT_NONVOLREG_ARM64 *nonvol_regs;
        int i;

        nonvol_regs = (void *)((DISPATCHER_CONTEXT_ARM64 *)dispatcher)->NonVolatileRegisters;
        ok( nonvol_regs->GpNvRegs[0] == ec_ctx->X19,
            "wrong non volatile reg x19 %I64x / %I64x\n", nonvol_regs->GpNvRegs[0], ec_ctx->X19 );
        ok( nonvol_regs->GpNvRegs[1] == ec_ctx->X20,
            "wrong non volatile reg x20 %I64x / %I64x\n", nonvol_regs->GpNvRegs[1], ec_ctx->X20 );
        ok( nonvol_regs->GpNvRegs[2] == ec_ctx->X21,
            "wrong non volatile reg x21 %I64x / %I64x\n", nonvol_regs->GpNvRegs[2], ec_ctx->X21 );
        ok( nonvol_regs->GpNvRegs[3] == ec_ctx->X22,
            "wrong non volatile reg x22 %I64x / %I64x\n", nonvol_regs->GpNvRegs[3], ec_ctx->X22 );
        ok( nonvol_regs->GpNvRegs[4] == 0, "wrong non volatile reg x23 %I64x\n", nonvol_regs->GpNvRegs[4] );
        ok( nonvol_regs->GpNvRegs[5] == 0, "wrong non volatile reg x24 %I64x\n", nonvol_regs->GpNvRegs[5] );
        ok( nonvol_regs->GpNvRegs[6] == ec_ctx->X25,
            "wrong non volatile reg x25 %I64x / %I64x\n", nonvol_regs->GpNvRegs[6], ec_ctx->X25 );
        ok( nonvol_regs->GpNvRegs[7] == ec_ctx->X26,
            "wrong non volatile reg x26 %I64x / %I64x\n", nonvol_regs->GpNvRegs[7], ec_ctx->X26 );
        ok( nonvol_regs->GpNvRegs[8] == ec_ctx->X27,
            "wrong non volatile reg x27 %I64x / %I64x\n", nonvol_regs->GpNvRegs[8], ec_ctx->X27 );
        ok( nonvol_regs->GpNvRegs[9] == 0, "wrong non volatile reg x28 %I64x\n", nonvol_regs->GpNvRegs[9] );
        ok( nonvol_regs->GpNvRegs[10] > ec_ctx->Fp, /* previous frame */
            "wrong non volatile reg x29 %I64x / %I64x\n", nonvol_regs->GpNvRegs[10], ec_ctx->Fp );

        for (i = 0; i < NONVOL_FP_NUMREG_ARM64; i++)
            ok( nonvol_regs->FpNvRegs[i] == ec_ctx->V[i + 8].D[0],
                "wrong non volatile reg d%u %g / %g\n", i + 8,
                nonvol_regs->FpNvRegs[i] , ec_ctx->V[i + 8].D[0] );
    }
    rtlraiseexception_handler_called = 1;
    rtlraiseexception_handler_(rec, frame, context, dispatcher, FALSE);
    return ExceptionContinueSearch;
}

static LONG CALLBACK rtlraiseexception_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT context = ExceptionInfo->ContextRecord;
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    void *addr = rec->ExceptionAddress;

    if (is_arm64ec) /* addr points to RtlRaiseException entry thunk */
        ok( ((ULONG *)addr)[-1] == 0xd63f0120 /* blr x9 */,
            "ExceptionAddress not in entry thunk %p (ntdll+%Ix)\n",
            addr, (char *)addr - (char *)hntdll );
    else
        ok( addr == (char *)code_mem + 0xc || broken(addr == code_mem || !addr ) /* 2008 */,
            "ExceptionAddress at %p instead of %p\n", addr, (char *)code_mem + 0xc );

    /* check that Rip is fixed up only for EXCEPTION_BREAKPOINT
     * even if raised by RtlRaiseException
     */
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT && test_stage && !is_arm64ec)
        ok( context->Rip == (UINT_PTR)addr - 1,
            "%d: Rip at %Ix instead of %Ix\n", test_stage, context->Rip, (UINT_PTR)addr - 1 );
    else
        ok( context->Rip == (UINT_PTR)addr,
            "%d: Rip at %Ix instead of %Ix\n", test_stage, context->Rip, (UINT_PTR)addr );

    /* test if context change is preserved from vectored handler to stack handlers */
    context->Rax = 0xf00f00f0;

    return EXCEPTION_CONTINUE_SEARCH;
}

static void run_rtlraiseexception_test(DWORD exceptioncode)
{
    unsigned char buf[4 + 4 + 4 + 8 + 2 + 8 + 2];
    RUNTIME_FUNCTION runtime_func;
    UNWIND_INFO *unwind = (UNWIND_INFO *)buf;
    EXCEPTION_REGISTRATION_RECORD frame;
    EXCEPTION_RECORD record;
    PVOID vectored_handler = NULL;

    void (CDECL *func)(void* function, EXCEPTION_RECORD* record) = code_mem;

    record.ExceptionCode = exceptioncode;
    record.ExceptionFlags = 0;
    record.ExceptionRecord = NULL;
    record.ExceptionAddress = NULL; /* does not matter, copied return address */
    record.NumberParameters = 0;

    runtime_func.BeginAddress = 0;
    runtime_func.EndAddress = sizeof(call_one_arg_code);
    runtime_func.UnwindData = 0x1000;

    unwind->Version = 1;
    unwind->Flags = UNW_FLAG_EHANDLER;
    unwind->SizeOfProlog = 4;
    unwind->CountOfCodes = 1;
    unwind->FrameRegister = 0;
    unwind->FrameOffset = 0;
    *(WORD *)&buf[4] = 0x4204; /* sub $0x28,%rsp */
    *(ULONG *)&buf[8] = 0x1014;
    *(const void **)&buf[12] = NULL;
    /* movabs $<handler>, %rax */
    buf[20] = 0x48;
    buf[21] = 0xb8;
    *(void **)&buf[22] = rtlraiseexception_handler;
    /* jmp *%rax */
    buf[30] = 0xff;
    buf[31] = 0xe0;

    memcpy((unsigned char *)code_mem + 0x1000, buf, sizeof(buf));
    pRtlAddFunctionTable( &runtime_func, 1, (ULONG_PTR)code_mem );

    frame.Handler = rtlraiseexception_teb_handler;
    frame.Prev = NtCurrentTeb()->Tib.ExceptionList;

    memcpy(code_mem, call_one_arg_code, sizeof(call_one_arg_code));

    NtCurrentTeb()->Tib.ExceptionList = &frame;
    if (have_vectored_api)
    {
        vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, rtlraiseexception_vectored_handler);
        ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");
    }
    if (pRtlSetUnhandledExceptionFilter) pRtlSetUnhandledExceptionFilter(rtlraiseexception_unhandled_handler);

    rtlraiseexception_handler_called = 0;
    rtlraiseexception_teb_handler_called = 0;
    rtlraiseexception_unhandled_handler_called = 0;
    func(pRtlRaiseException, &record);
    if (is_arm64ec) /* addr points to RtlRaiseException entry thunk */
        ok( ((ULONG *)record.ExceptionAddress)[-1] == 0xd63f0120 /* blr x9 */,
            "ExceptionAddress not in entry thunk %p (ntdll+%Ix)\n",
            record.ExceptionAddress, (char *)record.ExceptionAddress - (char *)hntdll );
    else
        ok( record.ExceptionAddress == (char *)code_mem + 0x0c,
            "address set to %p instead of %p\n", record.ExceptionAddress, (char *)code_mem + 0x0c );

    todo_wine
    ok( !rtlraiseexception_teb_handler_called, "Frame TEB handler called\n" );
    ok( rtlraiseexception_handler_called, "Frame handler called\n" );
    ok( rtlraiseexception_unhandled_handler_called, "UnhandledExceptionFilter wasn't called\n" );

    if (have_vectored_api)
        pRtlRemoveVectoredExceptionHandler(vectored_handler);

    if (pRtlSetUnhandledExceptionFilter) pRtlSetUnhandledExceptionFilter(NULL);
    pRtlDeleteFunctionTable( &runtime_func );
    NtCurrentTeb()->Tib.ExceptionList = frame.Prev;
}

static void test_rtlraiseexception(void)
{
#if defined(__REACTOS__)
    if (is_reactos())
    {
        skip("Skipping tests that crash\n");
        return;
    }
#endif
    if (!pRtlRaiseException)
    {
        skip("RtlRaiseException not found\n");
        return;
    }

    /* test without debugger */
    run_rtlraiseexception_test(0x12345);
    run_rtlraiseexception_test(EXCEPTION_BREAKPOINT);
    run_rtlraiseexception_test(EXCEPTION_INVALID_HANDLE);
}

static void test_debugger(DWORD cont_status, BOOL with_WaitForDebugEventEx)
{
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    DEBUG_EVENT de;
    DWORD continuestatus;
    PVOID code_mem_address = NULL;
    NTSTATUS status;
    SIZE_T size_read;
    BOOL ret;
    int counter = 0;
    si.cb = sizeof(si);

    if(!pNtGetContextThread || !pNtSetContextThread || !pNtReadVirtualMemory || !pNtTerminateProcess)
    {
        skip("NtGetContextThread, NtSetContextThread, NtReadVirtualMemory or NtTerminateProcess not found\n");
        return;
    }

    if (with_WaitForDebugEventEx && !pWaitForDebugEventEx)
    {
        skip("WaitForDebugEventEx not found, skipping unicode strings in OutputDebugStringW\n");
        return;
    }

    sprintf(cmdline, "%s %s %s %p", my_argv[0], my_argv[1], "debuggee", &test_stage);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "could not create child process error: %lu\n", GetLastError());
    if (!ret)
        return;

    do
    {
        continuestatus = cont_status;
        ret = with_WaitForDebugEventEx ? pWaitForDebugEventEx(&de, INFINITE) : WaitForDebugEvent(&de, INFINITE);
        ok(ret, "reading debug event\n");

        ret = ContinueDebugEvent(de.dwProcessId, de.dwThreadId, 0xdeadbeef);
        ok(!ret, "ContinueDebugEvent unexpectedly succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "Unexpected last error: %lu\n", GetLastError());

        if (de.dwThreadId != pi.dwThreadId)
        {
            trace("event %ld not coming from main thread, ignoring\n", de.dwDebugEventCode);
            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, cont_status);
            continue;
        }

        if (de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            if(de.u.CreateProcessInfo.lpBaseOfImage != NtCurrentTeb()->Peb->ImageBaseAddress)
            {
                skip("child process loaded at different address, terminating it\n");
                pNtTerminateProcess(pi.hProcess, 0);
            }
        }
        else if (de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            CONTEXT ctx;
            enum debugger_stages stage;

            counter++;
            status = pNtReadVirtualMemory(pi.hProcess, &code_mem, &code_mem_address,
                                          sizeof(code_mem_address), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);
            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            ctx.ContextFlags = CONTEXT_FULL | CONTEXT_SEGMENTS | CONTEXT_EXCEPTION_REQUEST;
            status = pNtGetContextThread(pi.hThread, &ctx);
            ok(!status, "NtGetContextThread failed with 0x%lx\n", status);
            ok(ctx.ContextFlags & CONTEXT_EXCEPTION_REPORTING, "got %#lx.\n", ctx.ContextFlags);

            trace("exception 0x%lx at %p firstchance=%ld Rip=%p, Rax=%p\n",
                  de.u.Exception.ExceptionRecord.ExceptionCode,
                  de.u.Exception.ExceptionRecord.ExceptionAddress,
                  de.u.Exception.dwFirstChance, (char *)ctx.Rip, (char *)ctx.Rax);

            if (counter > 100)
            {
                ok(FALSE, "got way too many exceptions, probably caught in an infinite loop, terminating child\n");
                pNtTerminateProcess(pi.hProcess, 1);
            }
            else if (counter < 2) /* startup breakpoint */
            {
                /* breakpoint is inside ntdll */
                IMAGE_NT_HEADERS *nt = RtlImageNtHeader( hntdll );

                ok( (char *)ctx.Rip >= (char *)hntdll &&
                    (char *)ctx.Rip < (char *)hntdll + nt->OptionalHeader.SizeOfImage,
                    "wrong rip %p ntdll %p-%p\n", (void *)ctx.Rip, hntdll,
                    (char *)hntdll + nt->OptionalHeader.SizeOfImage );
                check_context_exception_request( ctx.ContextFlags, TRUE );
            }
            else
            {
                if (stage == STAGE_RTLRAISE_NOT_HANDLED)
                {
                    if (is_arm64ec) /* addr points to RtlRaiseException entry thunk */
                        ok( ((ULONG *)ctx.Rip)[-1] == 0xd63f0120 /* blr x9 */,
                            "Rip not in entry thunk %p (ntdll+%Ix)\n",
                            (char *)ctx.Rip, (char *)ctx.Rip - (char *)hntdll );
                    else
                        ok((char *)ctx.Rip == (char *)code_mem_address + 0x0c, "Rip at %p instead of %p\n",
                           (char *)ctx.Rip, (char *)code_mem_address + 0x0c);
                    /* setting the context from debugger does not affect the context that the
                     * exception handler gets, except on w2008 */
                    ctx.Rip = (UINT_PTR)code_mem_address + 0x0e;
                    ctx.Rax = 0xf00f00f1;
                    /* let the debuggee handle the exception */
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    check_context_exception_request( ctx.ContextFlags, FALSE );
                }
                else if (stage == STAGE_RTLRAISE_HANDLE_LAST_CHANCE)
                {
                    if (de.u.Exception.dwFirstChance)
                    {
                        if (is_arm64ec)
                            ok( ((ULONG *)ctx.Rip)[-1] == 0xd63f0120 /* blr x9 */,
                                "Rip not in entry thunk %p (ntdll+%Ix)\n",
                                (char *)ctx.Rip, (char *)ctx.Rip - (char *)hntdll );
                        else
                            ok((char *)ctx.Rip == (char *)code_mem_address + 0x0c,
                               "Rip at %p instead of %p\n",
                               (char *)ctx.Rip, (char *)code_mem_address + 0x0c);
                        /* setting the context from debugger does not affect the context that the
                         * exception handler gets, except on w2008 */
                        ctx.Rip = (UINT_PTR)code_mem_address + 0x0e;
                        ctx.Rax = 0xf00f00f1;
                        /* pass exception to debuggee
                         * exception will not be handled and a second chance exception will be raised */
                        continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    }
                    else
                    {
                        /* debugger gets context after exception handler has played with it */
                        if (is_arm64ec)
                            ok( ((ULONG *)ctx.Rip)[-1] == 0xd63f0120 /* blr x9 */,
                                "Rip not in entry thunk %p (ntdll+%Ix)\n",
                                (char *)ctx.Rip, (char *)ctx.Rip - (char *)hntdll );
                        else if (de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                        {
                            ok((char *)ctx.Rip == (char *)code_mem_address + 0xb, "Rip at %p instead of %p\n",
                               (char *)ctx.Rip, (char *)code_mem_address + 0xb);
                            ctx.Rip += 1;
                        }
                        else ok((char *)ctx.Rip == (char *)code_mem_address + 0x0c, "Rip at 0x%I64x instead of %p\n",
                                ctx.Rip, (char *)code_mem_address + 0x0c);
                        /* here we handle exception */
                    }
                    check_context_exception_request( ctx.ContextFlags, FALSE );
                }
                else if (stage == STAGE_SERVICE_CONTINUE || stage == STAGE_SERVICE_NOT_HANDLED)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08lx\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Rip == (char *)code_mem_address + 0x30,
                       "expected Rip = %p, got %p\n", (char *)code_mem_address + 0x30, (char *)ctx.Rip);
                    if (stage == STAGE_SERVICE_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    check_context_exception_request( ctx.ContextFlags, TRUE );
                }
                else if (stage == STAGE_BREAKPOINT_CONTINUE || stage == STAGE_BREAKPOINT_NOT_HANDLED)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08lx\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Rip == (char *)code_mem_address + 2,
                       "expected Rip = %p, got %p\n", (char *)code_mem_address + 2, (char *)ctx.Rip);
                    if (stage == STAGE_BREAKPOINT_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    check_context_exception_request( ctx.ContextFlags, TRUE );
                }
                else if (stage == STAGE_EXCEPTION_INVHANDLE_CONTINUE || stage == STAGE_EXCEPTION_INVHANDLE_NOT_HANDLED)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_INVALID_HANDLE,
                       "unexpected exception code %08lx, expected %08lx\n", de.u.Exception.ExceptionRecord.ExceptionCode,
                       EXCEPTION_INVALID_HANDLE);
                    ok(de.u.Exception.ExceptionRecord.NumberParameters == 0,
                       "unexpected number of parameters %ld, expected 0\n", de.u.Exception.ExceptionRecord.NumberParameters);

                    if (stage == STAGE_EXCEPTION_INVHANDLE_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    check_context_exception_request( ctx.ContextFlags, FALSE );
                }
                else if (stage == STAGE_NO_EXCEPTION_INVHANDLE_NOT_HANDLED)
                {
                    ok(FALSE || broken(TRUE) /* < Win10 */, "should not throw exception\n");
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    check_context_exception_request( ctx.ContextFlags, FALSE );
                }
                else if (stage == STAGE_XSTATE || stage == STAGE_XSTATE_LEGACY_SSE)
                {
                    check_context_exception_request( ctx.ContextFlags, TRUE );
                    test_debugger_xstate(pi.hThread, &ctx, stage);
                }
                else if (stage == STAGE_SEGMENTS)
                {
#ifdef __arm64ec__
                    USHORT ss = 0x2b;
#elif defined(__REACTOS__) && defined(_MSC_VER)
                    USHORT ss = __readsegss();
#else
                    USHORT ss;
                    __asm__( "movw %%ss,%0" : "=r" (ss) );
#endif
                    ok( ctx.SegSs == ss, "wrong ss %04x / %04x\n", ctx.SegSs, ss );
                    ok( ctx.SegDs == ctx.SegSs, "wrong ds %04x / %04x\n", ctx.SegDs, ctx.SegSs );
                    ok( ctx.SegEs == ctx.SegSs, "wrong es %04x / %04x\n", ctx.SegEs, ctx.SegSs );
                    ok( ctx.SegFs != ctx.SegSs, "wrong fs %04x / %04x\n", ctx.SegFs, ctx.SegSs );
                    ok( ctx.SegGs == ctx.SegSs, "wrong gs %04x / %04x\n", ctx.SegGs, ctx.SegSs );
                    ctx.SegSs = 0;
                    ctx.SegDs = 0;
                    ctx.SegEs = ctx.SegFs;
                    ctx.SegGs = 0;
                    status = pNtSetContextThread( pi.hThread, &ctx );
                    ok( status == STATUS_SUCCESS, "NtSetContextThread failed with %lx\n", status );
                    status = pNtGetContextThread( pi.hThread, &ctx );
                    ok( status == STATUS_SUCCESS, "NtGetContextThread failed with %lx\n", status );
                    todo_wine ok( ctx.SegSs == ss, "wrong ss %04x / %04x\n", ctx.SegSs, ss );
                    ok( ctx.SegDs == ctx.SegSs, "wrong ds %04x / %04x\n", ctx.SegDs, ctx.SegSs );
                    ok( ctx.SegEs == ctx.SegSs, "wrong es %04x / %04x\n", ctx.SegEs, ctx.SegSs );
                    todo_wine ok( ctx.SegFs != ctx.SegSs, "wrong fs %04x / %04x\n", ctx.SegFs, ctx.SegSs );
                    ok( ctx.SegGs == ctx.SegSs, "wrong gs %04x / %04x\n", ctx.SegGs, ctx.SegSs );
                    check_context_exception_request( ctx.ContextFlags, TRUE );
                }
                else
                    ok(FALSE, "unexpected stage %u\n", stage);

                status = pNtSetContextThread(pi.hThread, &ctx);
                ok(!status, "NtSetContextThread failed with 0x%lx\n", status);
            }
        }
        else if (de.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
        {
            enum debugger_stages stage;
            char buffer[64 * sizeof(WCHAR)];
            unsigned char_size = de.u.DebugString.fUnicode ? sizeof(WCHAR) : sizeof(char);

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            if (de.u.DebugString.fUnicode)
                ok(with_WaitForDebugEventEx &&
                   (stage == STAGE_OUTPUTDEBUGSTRINGW_CONTINUE || stage == STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED),
                   "unexpected unicode debug string event\n");
            else
                ok(!with_WaitForDebugEventEx || stage != STAGE_OUTPUTDEBUGSTRINGW_CONTINUE || cont_status != DBG_CONTINUE,
                   "unexpected ansi debug string event %u %s %lx\n",
                   stage, with_WaitForDebugEventEx ? "with" : "without", cont_status);

            ok(de.u.DebugString.nDebugStringLength < sizeof(buffer) / char_size - 1,
               "buffer not large enough to hold %d bytes\n", de.u.DebugString.nDebugStringLength);

            memset(buffer, 0, sizeof(buffer));
            status = pNtReadVirtualMemory(pi.hProcess, de.u.DebugString.lpDebugStringData, buffer,
                                          de.u.DebugString.nDebugStringLength * char_size, &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            if (stage == STAGE_OUTPUTDEBUGSTRINGA_CONTINUE || stage == STAGE_OUTPUTDEBUGSTRINGA_NOT_HANDLED ||
                stage == STAGE_OUTPUTDEBUGSTRINGW_CONTINUE || stage == STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED)
            {
                if (de.u.DebugString.fUnicode)
                    ok(!wcscmp((WCHAR*)buffer, L"Hello World"), "got unexpected debug string '%ls'\n", (WCHAR*)buffer);
                else
                    ok(!strcmp(buffer, "Hello World"), "got unexpected debug string '%s'\n", buffer);
            }
            else /* ignore unrelated debug strings like 'SHIMVIEW: ShimInfo(Complete)' */
                ok(strstr(buffer, "SHIMVIEW") != NULL, "unexpected stage %x, got debug string event '%s'\n", stage, buffer);

            if (stage == STAGE_OUTPUTDEBUGSTRINGA_NOT_HANDLED || stage == STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED)
                continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }
        else if (de.dwDebugEventCode == RIP_EVENT)
        {
            enum debugger_stages stage;

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            if (stage == STAGE_RIPEVENT_CONTINUE || stage == STAGE_RIPEVENT_NOT_HANDLED)
            {
                ok(de.u.RipInfo.dwError == 0x11223344, "got unexpected rip error code %08lx, expected %08x\n",
                   de.u.RipInfo.dwError, 0x11223344);
                ok(de.u.RipInfo.dwType  == 0x55667788, "got unexpected rip type %08lx, expected %08x\n",
                   de.u.RipInfo.dwType, 0x55667788);
            }
            else
                ok(FALSE, "unexpected stage %x\n", stage);

            if (stage == STAGE_RIPEVENT_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continuestatus);

    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    wait_child_process( pi.hProcess );
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %lu\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "error %lu\n", GetLastError());
}

static void test_thread_context(void)
{
    CONTEXT context;
    NTSTATUS status;
    int i;
    struct expected
    {
        ULONG64 Rax, Rbx, Rcx, Rdx, Rsi, Rdi, R8, R9, R10, R11,
            R12, R13, R14, R15, Rbp, Rsp, Rip, prev_frame, EFlags;
        ULONG MxCsr;
        XMM_SAVE_AREA32 FltSave;
        WORD SegCs, SegDs, SegEs, SegFs, SegGs, SegSs;
    } expect;
    NTSTATUS (*func_ptr)( void *arg1, void *arg2, struct expected *res, void *func ) = code_mem;

    static const BYTE call_func[] =
    {
        0x55,                                                 /* push   %rbp */
        0x48, 0x89, 0xe5,                                     /* mov    %rsp,%rbp */
        0x48, 0x8d, 0x64, 0x24, 0xd0,                         /* lea    -0x30(%rsp),%rsp */
        0x49, 0x89, 0x00,                                     /* mov    %rax,(%r8) */
        0x49, 0x89, 0x58, 0x08,                               /* mov    %rbx,0x8(%r8) */
        0x49, 0x89, 0x48, 0x10,                               /* mov    %rcx,0x10(%r8) */
        0x49, 0x89, 0x50, 0x18,                               /* mov    %rdx,0x18(%r8) */
        0x49, 0x89, 0x70, 0x20,                               /* mov    %rsi,0x20(%r8) */
        0x49, 0x89, 0x78, 0x28,                               /* mov    %rdi,0x28(%r8) */
        0x4d, 0x89, 0x40, 0x30,                               /* mov    %r8,0x30(%r8) */
        0x4d, 0x89, 0x48, 0x38,                               /* mov    %r9,0x38(%r8) */
        0x4d, 0x89, 0x50, 0x40,                               /* mov    %r10,0x40(%r8) */
        0x4d, 0x89, 0x58, 0x48,                               /* mov    %r11,0x48(%r8) */
        0x4d, 0x89, 0x60, 0x50,                               /* mov    %r12,0x50(%r8) */
        0x4d, 0x89, 0x68, 0x58,                               /* mov    %r13,0x58(%r8) */
        0x4d, 0x89, 0x70, 0x60,                               /* mov    %r14,0x60(%r8) */
        0x4d, 0x89, 0x78, 0x68,                               /* mov    %r15,0x68(%r8) */
        0x49, 0x89, 0x68, 0x70,                               /* mov    %rbp,0x70(%r8) */
        0x49, 0x89, 0x60, 0x78,                               /* mov    %rsp,0x78(%r8) */
        0xff, 0x75, 0x08,                                     /* pushq  0x8(%rbp) */
        0x41, 0x8f, 0x80, 0x80, 0x00, 0x00, 0x00,             /* popq   0x80(%r8) */
        0xff, 0x75, 0x00,                                     /* pushq  0x0(%rbp) */
        0x41, 0x8f, 0x80, 0x88, 0x00, 0x00, 0x00,             /* popq   0x88(%r8) */
        0x9c,                                                 /* pushfq */
        0x41, 0x8f, 0x80, 0x90, 0x00, 0x00, 0x00,             /* popq   0x90(%r8) */
        0x41, 0x0f, 0xae, 0x98, 0x98, 0x00, 0x00, 0x00,       /* stmxcsr 0x98(%r8) */
        0x41, 0x0f, 0xae, 0x80, 0xa0, 0x00, 0x00, 0x00,       /* fxsave 0xa0(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0x80, 0x40, 0x01, 0x00, 0x00, /* movdqa %xmm0,0x140(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0x88, 0x50, 0x01, 0x00, 0x00, /* movdqa %xmm1,0x150(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0x90, 0x60, 0x01, 0x00, 0x00, /* movdqa %xmm2,0x160(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0x98, 0x70, 0x01, 0x00, 0x00, /* movdqa %xmm3,0x170(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0xa0, 0x80, 0x01, 0x00, 0x00, /* movdqa %xmm4,0x180(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0xa8, 0x90, 0x01, 0x00, 0x00, /* movdqa %xmm5,0x190(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0xb0, 0xa0, 0x01, 0x00, 0x00, /* movdqa %xmm6,0x1a0(%r8) */
        0x66, 0x41, 0x0f, 0x7f, 0xb8, 0xb0, 0x01, 0x00, 0x00, /* movdqa %xmm7,0x1b0(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0x80, 0xc0, 0x01, 0x00, 0x00, /* movdqa %xmm8,0x1c0(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0x88, 0xd0, 0x01, 0x00, 0x00, /* movdqa %xmm9,0x1d0(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0x90, 0xe0, 0x01, 0x00, 0x00, /* movdqa %xmm10,0x1e0(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0x98, 0xf0, 0x01, 0x00, 0x00, /* movdqa %xmm11,0x1f0(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0xa0, 0x00, 0x02, 0x00, 0x00, /* movdqa %xmm12,0x200(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0xa8, 0x10, 0x02, 0x00, 0x00, /* movdqa %xmm13,0x210(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0xb0, 0x20, 0x02, 0x00, 0x00, /* movdqa %xmm14,0x220(%r8) */
        0x66, 0x45, 0x0f, 0x7f, 0xb8, 0x30, 0x02, 0x00, 0x00, /* movdqa %xmm15,0x230(%r8) */
        0x41, 0x8c, 0x88, 0xa0, 0x02, 0x00, 0x00,             /* mov    %cs,0x2a0(%r8) */
        0x41, 0x8c, 0x98, 0xa2, 0x02, 0x00, 0x00,             /* mov    %ds,0x2a2(%r8) */
        0x41, 0x8c, 0x80, 0xa4, 0x02, 0x00, 0x00,             /* mov    %es,0x2a4(%r8) */
        0x41, 0x8c, 0xa0, 0xa6, 0x02, 0x00, 0x00,             /* mov    %fs,0x2a6(%r8) */
        0x41, 0x8c, 0xa8, 0xa8, 0x02, 0x00, 0x00,             /* mov    %gs,0x2a8(%r8) */
        0x41, 0x8c, 0x90, 0xaa, 0x02, 0x00, 0x00,             /* mov    %ss,0x2aa(%r8) */
        0x41, 0xff, 0xd1,                                     /* callq  *%r9 */
        0xc9,                                                 /* leaveq */
        0xc3,                                                 /* retq */
    };

    memcpy( func_ptr, call_func, sizeof(call_func) );

#define COMPARE(reg) \
    ok( context.reg == expect.reg, "wrong " #reg " %p/%p\n", (void *)(ULONG64)context.reg, (void *)(ULONG64)expect.reg )

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    func_ptr( &context, 0, &expect, pRtlCaptureContext );

    if (is_arm64ec)
    {
        ok( context.ContextFlags == (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT),
            "wrong flags %08lx\n", context.ContextFlags );
        ok( (context.EFlags & ~0xd5) == (expect.EFlags & ~0xd5), "wrong EFlags %lx / %I64x\n",
            context.EFlags, expect.EFlags );
        ok( context.SegCs == 0xcccc, "wrong SegCs %x\n", context.SegCs);
        ok( context.SegDs == 0xcccc, "wrong SegDs %x\n", context.SegDs);
        ok( context.SegEs == 0xcccc, "wrong SegEs %x\n", context.SegEs);
        ok( context.SegFs == 0xcccc, "wrong SegFs %x\n", context.SegFs);
        ok( context.SegGs == 0xcccc, "wrong SegGs %x\n", context.SegGs);
        ok( context.SegSs == 0xcccc, "wrong SegSs %x\n", context.SegSs);
    }
    else
    {
        ok( context.ContextFlags == (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT),
            "wrong flags %08lx\n", context.ContextFlags );
        COMPARE( Rax );
        COMPARE( Rcx );
        COMPARE( Rdx );
        COMPARE( R8 );
        COMPARE( R9 );
        COMPARE( R10 );
        COMPARE( R11 );
        COMPARE( EFlags );
        COMPARE( SegCs );
        COMPARE( SegDs );
        COMPARE( SegEs );
        COMPARE( SegFs );
        COMPARE( SegGs );
        COMPARE( SegSs );
        ok( !memcmp( &context.FltSave, &expect.FltSave, offsetof( XMM_SAVE_AREA32, XmmRegisters )),
            "wrong FltSave\n" );
    }
    COMPARE( Rbx );
    COMPARE( Rsi );
    COMPARE( Rdi );
    COMPARE( R12 );
    COMPARE( R13 );
    COMPARE( R14 );
    COMPARE( R15 );
    COMPARE( Rbp );
    COMPARE( Rsp );
    COMPARE( MxCsr );
    COMPARE( FltSave.MxCsr );
    COMPARE( FltSave.ControlWord );
    COMPARE( FltSave.StatusWord );
    for (i = 0; i < 16; i++)
        ok( !memcmp( &context.Xmm0 + i, &expect.FltSave.XmmRegisters[i], sizeof(context.Xmm0) ),
            "wrong xmm%u\n", i );
    /* Rip is return address from RtlCaptureContext */
    ok( context.Rip == (ULONG64)func_ptr + sizeof(call_func) - 2,
        "wrong Rip %p/%p\n", (void *)context.Rip, (char *)func_ptr + sizeof(call_func) - 2 );

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT;

    status = func_ptr( GetCurrentThread(), &context, &expect, pNtGetContextThread );
    ok( status == STATUS_SUCCESS, "NtGetContextThread failed %08lx\n", status );

    if (is_arm64ec)
    {
        /* Rsp is the stack upon entry to the ARM64 NtGetContextThread syscall */
        ok( context.Rsp <= expect.Rsp - sizeof(ARM64_NT_CONTEXT) && context.Rsp >= expect.Rsp - 0x1000,
            "wrong Rsp %p/%p\n", (void *)context.Rsp, (void *)expect.Rsp );
    }
    else
    {
        /* other registers are not preserved */
        COMPARE( Rbx );
        COMPARE( Rsi );
        COMPARE( Rdi );
        COMPARE( R12 );
        COMPARE( R13 );
        COMPARE( R14 );
        COMPARE( R15 );
        COMPARE( Rbp );
        /* Rsp is the stack upon entry to NtGetContextThread */
        ok( context.Rsp == expect.Rsp - 8,
            "wrong Rsp %p/%p\n", (void *)context.Rsp, (void *)expect.Rsp );
        /* Rip is somewhere close to the NtGetContextThread implementation */
        ok( (char *)context.Rip >= (char *)pNtGetContextThread - 0x40000 &&
            (char *)context.Rip <= (char *)pNtGetContextThread + 0x40000,
            "wrong Rip %p/%p\n", (void *)context.Rip, (void *)pNtGetContextThread );
    }
    COMPARE( MxCsr );
    COMPARE( SegCs );
    COMPARE( SegDs );
    COMPARE( SegEs );
    COMPARE( SegFs );
    if (expect.SegGs) COMPARE( SegGs );
    COMPARE( SegSs );

    /* AMD CPUs don't save the opcode or data pointer if no exception is
     * pending; see the AMD64 Architecture Programmer's Manual Volume 5 s.v.
     * FXSAVE */
    memcpy( &expect.FltSave, &context.FltSave, 0x12 );

    ok( !memcmp( &context.FltSave, &expect.FltSave, offsetof( XMM_SAVE_AREA32, ErrorOffset )), "wrong FltSave\n" );
    for (i = 6; i < 16; i++)
        ok( !memcmp( &context.Xmm0 + i, &expect.FltSave.XmmRegisters[i], sizeof(context.Xmm0) ),
            "wrong xmm%u\n", i );
#undef COMPARE
}

static void test_continue(void)
{
    struct context_pair {
        CONTEXT before;
        CONTEXT after;
    } contexts;
    NTSTATUS (*func_ptr)( struct context_pair *, void *arg, void *continue_func, void *capture_func ) = code_mem;
    KCONTINUE_ARGUMENT args = { .ContinueType = KCONTINUE_UNWIND };
    int i;

    static const BYTE call_func[] =
    {
        /* ret at 8*13(rsp) */

        /* need to preserve these */
        0x53,             /* push %rbx; 8*12(rsp) */
        0x55,             /* push %rbp; 8*11(rsp) */
        0x56,             /* push %rsi; 8*10(rsp) */
        0x57,             /* push %rdi; 8*9(rsp) */
        0x41, 0x54,       /* push %r12; 8*8(rsp) */
        0x41, 0x55,       /* push %r13; 8*7(rsp) */
        0x41, 0x56,       /* push %r14; 8*6(rsp) */
        0x41, 0x57,       /* push %r15; 8*5(rsp) */

        0x48, 0x83, 0xec, 0x28, /* sub $0x28, %rsp; reserve space for rsp and outgoing reg params */
        0x48, 0x89, 0x64, 0x24, 0x20, /* mov %rsp, 8*4(%rsp); for stack validation */

        /* save args */
        0x48, 0x89, 0x4c, 0x24, 0x70,                   /* mov %rcx, 8*14(%rsp) */
        0x48, 0x89, 0x54, 0x24, 0x78,                   /* mov %rdx, 8*15(%rsp) */
        0x4c, 0x89, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00, /* mov %r8,  8*16(%rsp) */
        0x4c, 0x89, 0x8c, 0x24, 0x88, 0x00, 0x00, 0x00, /* mov %r9,  8*17(%rsp) */

        /* invoke capture context */
        0x41, 0xff, 0xd1,       /* call *%r9 */

        /* overwrite general registers */
        0x48, 0xb8, 0xef, 0xbe, 0xad, 0xde, 0xef, 0xbe, 0xad, 0xde,  /* movabs $0xdeadbeefdeadbeef, %rax */
        0x48, 0x89, 0xc1, /* mov %rax, %rcx */
        0x48, 0x89, 0xc2, /* mov %rax, %rdx */
        0x48, 0x89, 0xc3, /* mov %rax, %rbx */
        0x48, 0x89, 0xc5, /* mov %rax, %rbp */
        0x48, 0x89, 0xc6, /* mov %rax, %rsi */
        0x48, 0x89, 0xc7, /* mov %rax, %rdi */
        0x49, 0x89, 0xc0, /* mov %rax, %r8 */
        0x49, 0x89, 0xc1, /* mov %rax, %r9 */
        0x49, 0x89, 0xc2, /* mov %rax, %r10 */
        0x49, 0x89, 0xc3, /* mov %rax, %r11 */
        0x49, 0x89, 0xc4, /* mov %rax, %r12 */
        0x49, 0x89, 0xc5, /* mov %rax, %r13 */
        0x49, 0x89, 0xc6, /* mov %rax, %r14 */
        0x49, 0x89, 0xc7, /* mov %rax, %r15 */

        /* overwrite SSE registers */
        0x66, 0x48, 0x0f, 0x6e, 0xc0, /* movq %rax, %xmm0 */
        0x66, 0x0f, 0x6c, 0xc0,       /* punpcklqdq %xmm0, %xmm0; extend to high quadword */
        0x0f, 0x28, 0xc8,             /* movaps %xmm0, %xmm1 */
        0x0f, 0x28, 0xd0,             /* movaps %xmm0, %xmm2 */
        0x0f, 0x28, 0xd8,             /* movaps %xmm0, %xmm3 */
        0x0f, 0x28, 0xe0,             /* movaps %xmm0, %xmm4 */
        0x0f, 0x28, 0xe8,             /* movaps %xmm0, %xmm5 */
        0x0f, 0x28, 0xf0,             /* movaps %xmm0, %xmm6 */
        0x0f, 0x28, 0xf8,             /* movaps %xmm0, %xmm7 */
        0x44, 0x0f, 0x28, 0xc0,       /* movaps %xmm0, %xmm8 */
        0x44, 0x0f, 0x28, 0xc8,       /* movaps %xmm0, %xmm9 */
        0x44, 0x0f, 0x28, 0xd0,       /* movaps %xmm0, %xmm10 */
        0x44, 0x0f, 0x28, 0xd8,       /* movaps %xmm0, %xmm11 */
        0x44, 0x0f, 0x28, 0xe0,       /* movaps %xmm0, %xmm12 */
        0x44, 0x0f, 0x28, 0xe8,       /* movaps %xmm0, %xmm13 */
        0x44, 0x0f, 0x28, 0xf0,       /* movaps %xmm0, %xmm14 */
        0x44, 0x0f, 0x28, 0xf8,       /* movaps %xmm0, %xmm15 */

        /* FIXME: overwrite debug, x87 FPU and AVX registers to test those */

        /* load args */
        0x48, 0x8b, 0x4c, 0x24, 0x70, /* mov 8*14(%rsp), %rcx; context   */
        0x48, 0x8b, 0x54, 0x24, 0x78, /* mov 8*15(%rsp), %rdx; arg */
        0x48, 0x83, 0xec, 0x70,       /* sub $0x70, %rsp; change stack   */

        /* setup context to return to label 1 */
        0x48, 0x8d, 0x05, 0x18, 0x00, 0x00, 0x00, /* lea 1f(%rip), %rax */
        0x48, 0x89, 0x81, 0xf8, 0x00, 0x00, 0x00, /* mov %rax, 0xf8(%rcx); context.Rip */

        /* flip some EFLAGS */
        0x9c,                                           /* pushf */
        /*
           0x0001 Carry flag
           0x0004 Parity flag
           0x0010 Auxiliary Carry flag
           0x0040 Zero flag
           0x0080 Sign flag
           FIXME: 0x0400 Direction flag - not changing as it breaks Wine
           0x0800 Overflow flag
           ~0x4000~ Nested task flag - not changing - breaks Wine
           = 0x8d5
        */
        0x48, 0x81, 0x34, 0x24, 0xd5, 0x08, 0x00, 0x00, /* xorq $0x8d5, (%rsp) */
        0x9d,                                           /* popf */

        /* invoke NtContinue... */
        0xff, 0x94, 0x24, 0xf0, 0x00, 0x00, 0x00, /* call *8*16+0x70(%rsp) */

        /* validate stack pointer */
        0x48, 0x3b, 0x64, 0x24, 0x20, /* 1: cmp 0x20(%rsp), %rsp */
        0x74, 0x02,                   /* je 2f; jump over ud2 */
        0x0f, 0x0b,                   /* ud2; stack pointer invalid, let's crash */

        /* invoke capture context */
        0x48, 0x8b, 0x4c, 0x24, 0x70,             /* 2: mov 8*14(%rsp), %rcx; context */
        0x48, 0x81, 0xc1, 0xd0, 0x04, 0x00, 0x00, /* add $0x4d0, %rcx; +sizeof(CONTEXT) to get context->after */
        0xff, 0x94, 0x24, 0x88, 0x00, 0x00, 0x00, /* call *8*17(%rsp) */

        /* free stack */
        0x48, 0x83, 0xc4, 0x28, /* add $0x28, %rsp */

        /* restore back */
        0x41, 0x5f,       /* pop %r15 */
        0x41, 0x5e,       /* pop %r14 */
        0x41, 0x5d,       /* pop %r13 */
        0x41, 0x5c,       /* pop %r12 */
        0x5f,             /* pop %rdi */
        0x5e,             /* pop %rsi */
        0x5d,             /* pop %rbp */
        0x5b,             /* pop %rbx */
        0xc3              /* ret      */
    };

    if (!pRtlCaptureContext)
    {
        win_skip("RtlCaptureContext is not available.\n");
        return;
    }

    memcpy( func_ptr, call_func, sizeof(call_func) );
    FlushInstructionCache( GetCurrentProcess(), func_ptr, sizeof(call_func) );

    func_ptr( &contexts, 0, NtContinue, pRtlCaptureContext );

#define COMPARE(reg) \
    ok( contexts.before.reg == contexts.after.reg, "wrong " #reg " %p/%p\n", (void *)(ULONG64)contexts.before.reg, (void *)(ULONG64)contexts.after.reg )

    COMPARE( Rax );
    COMPARE( Rdx );
    COMPARE( Rbx );
    COMPARE( Rbp );
    COMPARE( Rsi );
    COMPARE( Rdi );
    COMPARE( R8 );
    COMPARE( R9 );
    COMPARE( R10 );
    COMPARE( R11 );
    COMPARE( R12 );
    COMPARE( R13 );
    COMPARE( R14 );
    COMPARE( R15 );

    for (i = 0; i < 16; i++)
        ok( !memcmp( &contexts.before.Xmm0 + i, &contexts.after.Xmm0 + i, sizeof(contexts.before.Xmm0) ),
            "wrong xmm%u %08I64x%08I64x/%08I64x%08I64x\n", i, *(&contexts.before.Xmm0.High + i*2), *(&contexts.before.Xmm0.Low + i*2),
            *(&contexts.after.Xmm0.High + i*2), *(&contexts.after.Xmm0.Low + i*2) );

    apc_count = 0;
    pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234, 0x5678, 0xdeadbeef );
    func_ptr( &contexts, 0, NtContinue, pRtlCaptureContext );
    ok( apc_count == 0, "apc called\n" );
    func_ptr( &contexts, (void *)1, NtContinue, pRtlCaptureContext );
    ok( apc_count == 1, "apc not called\n" );

    if (!pNtContinueEx)
    {
        win_skip( "NtContinueEx not supported\n" );
        return;
    }

    func_ptr( &contexts, &args, pNtContinueEx, pRtlCaptureContext );

#define COMPARE(reg) \
    ok( contexts.before.reg == contexts.after.reg, "wrong " #reg " %p/%p\n", (void *)(ULONG64)contexts.before.reg, (void *)(ULONG64)contexts.after.reg )

    COMPARE( Rax );
    COMPARE( Rdx );
    COMPARE( Rbx );
    COMPARE( Rbp );
    COMPARE( Rsi );
    COMPARE( Rdi );
    COMPARE( R8 );
    COMPARE( R9 );
    COMPARE( R10 );
    COMPARE( R11 );
    COMPARE( R12 );
    COMPARE( R13 );
    COMPARE( R14 );
    COMPARE( R15 );

    for (i = 0; i < 16; i++)
        ok( !memcmp( &contexts.before.Xmm0 + i, &contexts.after.Xmm0 + i, sizeof(contexts.before.Xmm0) ),
            "wrong xmm%u %08I64x%08I64x/%08I64x%08I64x\n", i, *(&contexts.before.Xmm0.High + i*2), *(&contexts.before.Xmm0.Low + i*2),
            *(&contexts.after.Xmm0.High + i*2), *(&contexts.after.Xmm0.Low + i*2) );

    pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234 + apc_count, 0x5678, 0xdeadbeef );
    func_ptr( &contexts, &args, pNtContinueEx, pRtlCaptureContext );
    ok( apc_count == 1, "apc called\n" );
    args.ContinueFlags = KCONTINUE_FLAG_TEST_ALERT;
    func_ptr( &contexts, &args, pNtContinueEx, pRtlCaptureContext );
    ok( apc_count == 2, "apc not called\n" );

#undef COMPARE
}

static void test_wow64_context(void)
{
#ifdef __REACTOS__
    char appname[MAX_PATH];
#else
    const char appname[] = "C:\\windows\\syswow64\\cmd.exe";
#endif
    char cmdline[256];
    THREAD_BASIC_INFORMATION info;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {0};
    WOW64_CONTEXT ctx, *ctx_ptr = NULL;
    CONTEXT context;
    NTSTATUS ret;
    TEB teb;
    TEB32 teb32;
    SIZE_T res, cpu_size = 0;
    WOW64_CPURESERVED *cpu = NULL;
    WOW64_CPU_AREA_INFO cpu_info;
    BOOL r, got32, got64;
    unsigned int i, cs32, cs64;
    ULONG_PTR ecx, rcx;

#ifdef __REACTOS__
    if ((pRtlWow64GetThreadContext == NULL) ||
        (pRtlWow64SetThreadContext == NULL))
    {
        skip("RtlWow64Get/SetThreadContext not found\n");
        return;
    }

    GetWindowsDirectoryA(appname, sizeof(appname));
    strcat(appname, "\\syswow64\\cmd.exe");
#endif

    memset(&ctx, 0x55, sizeof(ctx));
    ctx.ContextFlags = WOW64_CONTEXT_ALL;
    ret = pRtlWow64GetThreadContext( GetCurrentThread(), &ctx );
    ok(ret == STATUS_INVALID_PARAMETER || broken(ret == STATUS_PARTIAL_COPY), "got %#lx\n", ret);

    sprintf( cmdline, "\"%s\" /c for /l %%n in () do @echo >nul", appname );
    r = CreateProcessA( appname, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    ok( r, "failed to start %s err %lu\n", appname, GetLastError() );

    ret = pRtlWow64GetThreadContext( pi.hThread, &ctx );
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(ctx.ContextFlags == WOW64_CONTEXT_ALL, "got context flags %#lx\n", ctx.ContextFlags);
    ok(!ctx.Ebp, "got ebp %08lx\n", ctx.Ebp);
    ok(!ctx.Ecx, "got ecx %08lx\n", ctx.Ecx);
    ok(!ctx.Edx, "got edx %08lx\n", ctx.Edx);
    ok(!ctx.Esi, "got esi %08lx\n", ctx.Esi);
    ok(!ctx.Edi, "got edi %08lx\n", ctx.Edi);
    ok((ctx.EFlags & ~2) == 0x200, "got eflags %08lx\n", ctx.EFlags);
    ok((WORD) ctx.FloatSave.ControlWord == 0x27f, "got control word %08lx\n",
        ctx.FloatSave.ControlWord);
    ok(*(WORD *)ctx.ExtendedRegisters == 0x27f, "got SSE control word %04x\n",
       *(WORD *)ctx.ExtendedRegisters);

    ret = pRtlWow64SetThreadContext( pi.hThread, &ctx );
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    pNtQueryInformationThread( pi.hThread, ThreadBasicInformation, &info, sizeof(info), NULL );
    if (!ReadProcessMemory( pi.hProcess, info.TebBaseAddress, &teb, sizeof(teb), &res )) res = 0;
    ok( res == sizeof(teb), "wrong len %Ix\n", res );

    memset( &teb32, 0, sizeof(teb32) );
    if (teb.WowTebOffset > 1)
    {
        if (!ReadProcessMemory( pi.hProcess, (char *)info.TebBaseAddress + teb.WowTebOffset,
                                &teb32, sizeof(teb32), &res )) res = 0;
        ok( res == sizeof(teb32), "wrong len %Ix\n", res );

        ok( ((ctx.Esp + 0xfff) & ~0xfff) == teb32.Tib.StackBase,
            "esp is not at top of stack: %08lx / %08lx\n", ctx.Esp, teb32.Tib.StackBase );
        ok( ULongToPtr( teb32.Tib.StackBase ) <= teb.DeallocationStack ||
            ULongToPtr( teb32.DeallocationStack ) >= teb.Tib.StackBase,
            "stacks overlap %08lx-%08lx / %p-%p\n", teb32.DeallocationStack, teb32.Tib.StackBase,
            teb.DeallocationStack, teb.Tib.StackBase );
    }

    if (pRtlWow64GetCpuAreaInfo)
    {
        ok( teb.TlsSlots[WOW64_TLS_CPURESERVED] == teb.Tib.StackBase, "wrong cpu reserved %p / %p\n",
            teb.TlsSlots[WOW64_TLS_CPURESERVED], teb.Tib.StackBase );
        cpu_size = 0x1000 - ((ULONG_PTR)teb.TlsSlots[WOW64_TLS_CPURESERVED] & 0xfff);
        cpu = malloc( cpu_size );
        if (!ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED], cpu, cpu_size, &res )) res = 0;
        ok( res == cpu_size, "wrong len %Ix\n", res );
        ok( cpu->Machine == IMAGE_FILE_MACHINE_I386, "wrong machine %04x\n", cpu->Machine );
        ret = pRtlWow64GetCpuAreaInfo( cpu, 0, &cpu_info );
        ok( !ret, "RtlWow64GetCpuAreaInfo failed %lx\n", ret );
        /* work around pointer truncation bug on win10 <= 1709 */
        if (!((ULONG_PTR)cpu_info.Context >> 32))
        {
            cpu_info.Context = (char *)cpu + (ULONG)((char *)cpu_info.Context - (char *)cpu);
            cpu_info.ContextEx = (char *)cpu + (ULONG)((char *)cpu_info.ContextEx - (char *)cpu);
        }
        ctx_ptr = (WOW64_CONTEXT *)cpu_info.Context;
        ok(!*(void **)cpu_info.ContextEx, "got context_ex %p\n", *(void **)cpu_info.ContextEx);
        ok(ctx_ptr->ContextFlags == WOW64_CONTEXT_ALL, "got context flags %#lx\n", ctx_ptr->ContextFlags);
        ok(ctx_ptr->Eax == ctx.Eax, "got eax %08lx / %08lx\n", ctx_ptr->Eax, ctx.Eax);
        ok(ctx_ptr->Ebx == ctx.Ebx, "got ebx %08lx / %08lx\n", ctx_ptr->Ebx, ctx.Ebx);
        ok(ctx_ptr->Ecx == ctx.Ecx, "got ecx %08lx / %08lx\n", ctx_ptr->Ecx, ctx.Ecx);
        ok(ctx_ptr->Edx == ctx.Edx, "got edx %08lx / %08lx\n", ctx_ptr->Edx, ctx.Edx);
        ok(ctx_ptr->Ebp == ctx.Ebp, "got ebp %08lx / %08lx\n", ctx_ptr->Ebp, ctx.Ebp);
        ok(ctx_ptr->Esi == ctx.Esi, "got esi %08lx / %08lx\n", ctx_ptr->Esi, ctx.Esi);
        ok(ctx_ptr->Edi == ctx.Edi, "got edi %08lx / %08lx\n", ctx_ptr->Edi, ctx.Edi);
        ok(ctx_ptr->SegCs == ctx.SegCs, "got cs %04lx / %04lx\n", ctx_ptr->SegCs, ctx.SegCs);
        ok(ctx_ptr->SegDs == ctx.SegDs, "got ds %04lx / %04lx\n", ctx_ptr->SegDs, ctx.SegDs);
        ok(ctx_ptr->SegEs == ctx.SegEs, "got es %04lx / %04lx\n", ctx_ptr->SegEs, ctx.SegEs);
        ok(ctx_ptr->SegFs == ctx.SegFs, "got fs %04lx / %04lx\n", ctx_ptr->SegFs, ctx.SegFs);
        ok(ctx_ptr->SegGs == ctx.SegGs, "got gs %04lx / %04lx\n", ctx_ptr->SegGs, ctx.SegGs);
        ok(ctx_ptr->SegSs == ctx.SegSs, "got ss %04lx / %04lx\n", ctx_ptr->SegSs, ctx.SegSs);
        ok(ctx_ptr->EFlags == ctx.EFlags, "got eflags %08lx / %08lx\n", ctx_ptr->EFlags, ctx.EFlags);
        ok((WORD)ctx_ptr->FloatSave.ControlWord == ctx.FloatSave.ControlWord,
           "got control word %08lx / %08lx\n", ctx_ptr->FloatSave.ControlWord, ctx.FloatSave.ControlWord);
        ok(*(WORD *)ctx_ptr->ExtendedRegisters == *(WORD *)ctx.ExtendedRegisters,
           "got SSE control word %04x / %04x\n", *(WORD *)ctx_ptr->ExtendedRegisters,
           *(WORD *)ctx.ExtendedRegisters);

        ecx = ctx.Ecx;
        ctx.Ecx = 0x12345678;
        ret = pRtlWow64SetThreadContext( pi.hThread, &ctx );
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
        if (!ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED], cpu, cpu_size, &res )) res = 0;
        ok( res == cpu_size, "wrong len %Ix\n", res );
        todo_wine
        ok( ctx_ptr->Ecx == 0x12345678, "got ecx %08lx\n", ctx_ptr->Ecx );
        ctx.Ecx = ecx;
        pRtlWow64SetThreadContext( pi.hThread, &ctx );
    }
    else win_skip( "RtlWow64GetCpuAreaInfo not supported\n" );

    memset( &context, 0x55, sizeof(context) );
    context.ContextFlags = CONTEXT_ALL;
    ret = pNtGetContextThread( pi.hThread, &context );
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok( context.ContextFlags == is_arm64ec ? CONTEXT_FULL : CONTEXT_ALL,
        "got context flags %#lx\n", context.ContextFlags );
    ok( !context.Rsi, "rsi is not zero %Ix\n", context.Rsi );
    ok( !context.Rdi, "rdi is not zero %Ix\n", context.Rdi );
    ok( !context.Rbp, "rbp is not zero %Ix\n", context.Rbp );
    ok( !context.R8, "r8 is not zero %Ix\n", context.R8 );
    ok( !context.R9, "r9 is not zero %Ix\n", context.R9 );
    ok( !context.R10, "r10 is not zero %Ix\n", context.R10 );
    ok( !context.R11, "r11 is not zero %Ix\n", context.R11 );
    ok( !context.R12, "r12 is not zero %Ix\n", context.R12 );
    ok( !context.R13, "r13 is not zero %Ix\n", context.R13 );
    ok( !context.R14, "r14 is not zero %Ix\n", context.R14 );
    ok( !context.R15, "r15 is not zero %Ix\n", context.R15 );
    ok( context.MxCsr == 0x1f80, "wrong mxcsr %08lx\n", context.MxCsr );
    ok( context.FltSave.ControlWord == 0x27f, "wrong control %08x\n", context.FltSave.ControlWord );
    if (LOWORD(context.ContextFlags) & CONTEXT_SEGMENTS)
    {
        ok( context.SegDs == ctx.SegDs, "wrong ds %04x / %04lx\n", context.SegDs, ctx.SegDs );
        ok( context.SegEs == ctx.SegEs, "wrong es %04x / %04lx\n", context.SegEs, ctx.SegEs );
        ok( context.SegFs == ctx.SegFs, "wrong fs %04x / %04lx\n", context.SegFs, ctx.SegFs );
        ok( context.SegGs == ctx.SegGs, "wrong gs %04x / %04lx\n", context.SegGs, ctx.SegGs );
        ok( context.SegSs == ctx.SegSs, "wrong ss %04x / %04lx\n", context.SegSs, ctx.SegSs );
    }
    cs32 = ctx.SegCs;
    cs64 = context.SegCs;
    if (cs32 == cs64)
    {
        todo_wine win_skip( "no wow64 support\n" );
        goto done;
    }

    ok( !context.Rax, "rax is not zero %Ix\n", context.Rax );
    ok( !context.Rbx, "rbx is not zero %Ix\n", context.Rbx );
    ok( ((ULONG_PTR)context.Rsp & ~0xfff) == ((ULONG_PTR)teb.Tib.StackBase & ~0xfff),
        "rsp is not at top of stack %p / %p\n", (void *)context.Rsp, teb.Tib.StackBase );
    ok( context.EFlags == 0x200 || context.EFlags == 0x202, "wrong flags %08lx\n", context.EFlags );

    for (i = 0, got32 = got64 = FALSE; i < 10000 && !(got32 && got64); i++)
    {
        ResumeThread( pi.hThread );
        Sleep( 1 );
        SuspendThread( pi.hThread );
        memset( &context, 0x55, sizeof(context) );
        context.ContextFlags = CONTEXT_ALL;
        ret = pNtGetContextThread( pi.hThread, &context );
        ok( ret == STATUS_SUCCESS, "got %#lx\n", ret );
        if (ret) break;
        if (context.SegCs == cs32 && got32) continue;
        if (context.SegCs == cs64 && got64) continue;
        if (context.SegCs != cs32 && context.SegCs != cs64)
        {
            ok( 0, "unexpected cs %04x\n", context.SegCs );
            break;
        }

        memset( &ctx, 0x55, sizeof(ctx) );
        ctx.ContextFlags = WOW64_CONTEXT_ALL;
        ret = pRtlWow64GetThreadContext( pi.hThread, &ctx );
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
        ok( ctx.ContextFlags == WOW64_CONTEXT_ALL, "got context flags %#lx\n", ctx.ContextFlags );

        ctx.ContextFlags = WOW64_CONTEXT_ALL | CONTEXT_EXCEPTION_REQUEST;
        ret = pRtlWow64GetThreadContext( pi.hThread, &ctx );
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
        ok( (ctx.ContextFlags & CONTEXT_EXCEPTION_REPORTING) || broken( ctx.ContextFlags == WOW64_CONTEXT_ALL ) /*Win 7*/,
            "got context flags %#lx\n", ctx.ContextFlags );

        if (context.SegCs == cs32)
        {
            trace( "in 32-bit mode %04x\n", context.SegCs );
            if (ctx.ContextFlags & CONTEXT_EXCEPTION_REPORTING)
                ok( ctx.ContextFlags == (WOW64_CONTEXT_ALL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING)
                    || ctx.ContextFlags == (WOW64_CONTEXT_ALL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING
                       | CONTEXT_EXCEPTION_ACTIVE),
                    "got %#lx.\n", ctx.ContextFlags );
            ok( ctx.Eip == context.Rip, "cs32: eip %08lx / %p\n", ctx.Eip, (void *)context.Rip );
            ok( ctx.Ebp == context.Rbp, "cs32: ebp %08lx / %p\n", ctx.Ebp, (void *)context.Rbp );
            ok( ctx.Esp == context.Rsp, "cs32: esp %08lx / %p\n", ctx.Esp, (void *)context.Rsp );
            ok( ctx.Eax == context.Rax, "cs32: eax %08lx / %p\n", ctx.Eax, (void *)context.Rax );
            ok( ctx.Ebx == context.Rbx, "cs32: ebx %08lx / %p\n", ctx.Ebx, (void *)context.Rbx );
            ok( ctx.Ecx == context.Rcx || broken(ctx.Ecx == (ULONG)context.Rcx),
                "cs32: ecx %08lx / %p\n", ctx.Ecx, (void *)context.Rcx );
            ok( ctx.Edx == context.Rdx, "cs32: edx %08lx / %p\n", ctx.Edx, (void *)context.Rdx );
            ok( ctx.Esi == context.Rsi, "cs32: esi %08lx / %p\n", ctx.Esi, (void *)context.Rsi );
            ok( ctx.Edi == context.Rdi, "cs32: edi %08lx / %p\n", ctx.Edi, (void *)context.Rdi );
            ok( ctx.SegCs == cs32, "cs32: wrong cs %04lx / %04x\n", ctx.SegCs, cs32 );
            ok( ctx.SegDs == context.SegDs, "cs32: wrong ds %04lx / %04x\n", ctx.SegDs, context.SegDs );
            ok( ctx.SegEs == context.SegEs, "cs32: wrong es %04lx / %04x\n", ctx.SegEs, context.SegEs );
            ok( ctx.SegFs == context.SegFs, "cs32: wrong fs %04lx / %04x\n", ctx.SegFs, context.SegFs );
            ok( ctx.SegGs == context.SegGs, "cs32: wrong gs %04lx / %04x\n", ctx.SegGs, context.SegGs );
            ok( ctx.SegSs == context.SegSs, "cs32: wrong ss %04lx / %04x\n", ctx.SegSs, context.SegSs );
            if (teb32.DeallocationStack)
                ok( ctx.Esp >= teb32.DeallocationStack && ctx.Esp <= teb32.Tib.StackBase,
                    "cs32: esp not inside 32-bit stack %08lx / %08lx-%08lx\n", ctx.Esp,
                    teb32.DeallocationStack, teb32.Tib.StackBase );
            /* r12 points to the TEB */
            ok( (void *)context.R12 == info.TebBaseAddress,
                "cs32: r12 not pointing to the TEB %p / %p\n", (void *)context.R12, info.TebBaseAddress );
            /* r13 points inside the cpu area */
            ok( (void *)context.R13 >= teb.TlsSlots[WOW64_TLS_CPURESERVED] &&
                context.R13 <= ((ULONG_PTR)teb.TlsSlots[WOW64_TLS_CPURESERVED] | 0xfff),
                "cs32: r13 not pointing into cpu area %p / %p\n", (void *)context.R13,
                teb.TlsSlots[WOW64_TLS_CPURESERVED] );
            /* r14 stores the 64-bit stack pointer */
            ok( (void *)context.R14 >= teb.DeallocationStack && (void *)context.R14 <= teb.Tib.StackBase,
                "cs32: r14 not inside 32-bit stack %p / %p-%p\n", (void *)context.R14,
                (void *)teb.DeallocationStack, (void *)teb.Tib.StackBase );

            if (pRtlWow64GetCpuAreaInfo)
            {
                /* in 32-bit mode, the 32-bit context is the current cpu context, not the stored one */
                if (!ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED],
                                        cpu, cpu_size, &res )) res = 0;
                ok( res == cpu_size, "wrong len %Ix\n", res );
                ok(ctx_ptr->ContextFlags == WOW64_CONTEXT_ALL,
                   "cs32: got context flags %#lx\n", ctx_ptr->ContextFlags);

                /* changing either context changes the actual cpu context */
                rcx = context.Rcx;
                ecx = ctx_ptr->Ecx;
                context.Rcx = 0xfedcba987654321ull;
                pNtSetContextThread( pi.hThread, &context );
                memset( &ctx, 0x55, sizeof(ctx) );
                ctx.ContextFlags = WOW64_CONTEXT_ALL;
                pRtlWow64GetThreadContext( pi.hThread, &ctx );
                todo_wine
                ok( ctx.Ecx == 0x87654321, "cs32: ecx set to %08lx\n", ctx.Ecx );
                ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED], cpu, cpu_size, &res );
                ok( ctx_ptr->Ecx == ecx, "cs32: ecx set to %08lx\n", ctx_ptr->Ecx );
                ctx.Ecx = 0x33334444;
                pRtlWow64SetThreadContext( pi.hThread, &ctx );
                memset( &ctx, 0x55, sizeof(ctx) );
                ctx.ContextFlags = WOW64_CONTEXT_ALL;
                pRtlWow64GetThreadContext( pi.hThread, &ctx );
                ok( ctx.Ecx == 0x33334444, "cs32: ecx set to %08lx\n", ctx.Ecx );
                ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED], cpu, cpu_size, &res );
                ok( ctx_ptr->Ecx == ecx, "cs32: ecx set to %08lx\n", ctx_ptr->Ecx );
                memset( &context, 0x55, sizeof(context) );
                context.ContextFlags = CONTEXT_ALL;
                pNtGetContextThread( pi.hThread, &context );
                todo_wine
                ok( context.Rcx == 0x33334444, "cs32: rcx set to %p\n", (void *)context.Rcx );
                /* restore everything */
                context.Rcx = rcx;
                pNtSetContextThread( pi.hThread, &context );
            }
            got32 = TRUE;
        }
        else
        {
            trace( "in 64-bit mode %04x\n", context.SegCs );
            if (ctx.ContextFlags & CONTEXT_EXCEPTION_REPORTING)
                ok( ctx.ContextFlags == (WOW64_CONTEXT_ALL | CONTEXT_EXCEPTION_REQUEST
                    | CONTEXT_EXCEPTION_REPORTING | CONTEXT_SERVICE_ACTIVE)
                    || ctx.ContextFlags == (WOW64_CONTEXT_ALL | CONTEXT_EXCEPTION_REQUEST
                    | CONTEXT_EXCEPTION_REPORTING | CONTEXT_EXCEPTION_ACTIVE),
                    "got %#lx.\n", ctx.ContextFlags );
            ok( ctx.Eip != context.Rip, "cs64: eip %08lx / %p\n", ctx.Eip, (void *)context.Rip);
            ok( ctx.SegCs == cs32, "cs64: wrong cs %04lx / %04x\n", ctx.SegCs, cs32 );
            if (!is_arm64ec)
            {
                ok( ctx.SegDs == context.SegDs, "cs64: wrong ds %04lx / %04x\n", ctx.SegDs, context.SegDs );
                ok( ctx.SegEs == context.SegEs, "cs64: wrong es %04lx / %04x\n", ctx.SegEs, context.SegEs );
                ok( ctx.SegFs == context.SegFs, "cs64: wrong fs %04lx / %04x\n", ctx.SegFs, context.SegFs );
                ok( ctx.SegGs == context.SegGs, "cs64: wrong gs %04lx / %04x\n", ctx.SegGs, context.SegGs );
                ok( ctx.SegSs == context.SegSs, "cs64: wrong ss %04lx / %04x\n", ctx.SegSs, context.SegSs );
            }
            if (teb32.DeallocationStack)
                ok( ctx.Esp >= teb32.DeallocationStack && ctx.Esp <= teb32.Tib.StackBase,
                    "cs64: esp not inside 32-bit stack %08lx / %08lx-%08lx\n", ctx.Esp,
                    teb32.DeallocationStack, teb32.Tib.StackBase );
            ok( ((void *)context.Rsp >= teb.DeallocationStack && (void *)context.Rsp <= teb.Tib.StackBase) ||
                (context.Rsp >= teb32.DeallocationStack && context.Rsp <= teb32.Tib.StackBase),
                "cs64: rsp not inside stack %p / 64-bit %p-%p 32-bit %p-%p\n", (void *)context.Rsp,
                teb.DeallocationStack, teb.Tib.StackBase,
                ULongToPtr(teb32.DeallocationStack), ULongToPtr(teb32.Tib.StackBase) );

            if (pRtlWow64GetCpuAreaInfo)
            {
                /* in 64-bit mode, the 32-bit context is stored in the cpu area */
                if (!ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED],
                                        cpu, cpu_size, &res )) res = 0;
                ok( res == cpu_size, "wrong len %Ix\n", res );
                ok(ctx_ptr->ContextFlags == WOW64_CONTEXT_ALL,
                   "cs64: got context flags %#lx\n", ctx_ptr->ContextFlags);
                ok(ctx_ptr->Eip == ctx.Eip, "cs64: got eip %08lx / %08lx\n", ctx_ptr->Eip, ctx.Eip);
                ok(ctx_ptr->Eax == ctx.Eax, "cs64: got eax %08lx / %08lx\n", ctx_ptr->Eax, ctx.Eax);
                ok(ctx_ptr->Ebx == ctx.Ebx, "cs64: got ebx %08lx / %08lx\n", ctx_ptr->Ebx, ctx.Ebx);
                ok(ctx_ptr->Ecx == ctx.Ecx, "cs64: got ecx %08lx / %08lx\n", ctx_ptr->Ecx, ctx.Ecx);
                ok(ctx_ptr->Edx == ctx.Edx, "cs64: got edx %08lx / %08lx\n", ctx_ptr->Edx, ctx.Edx);
                ok(ctx_ptr->Ebp == ctx.Ebp, "cs64: got ebp %08lx / %08lx\n", ctx_ptr->Ebp, ctx.Ebp);
                ok(ctx_ptr->Esi == ctx.Esi, "cs64: got esi %08lx / %08lx\n", ctx_ptr->Esi, ctx.Esi);
                ok(ctx_ptr->Edi == ctx.Edi, "cs64: got edi %08lx / %08lx\n", ctx_ptr->Edi, ctx.Edi);
                ok(ctx_ptr->EFlags == ctx.EFlags, "cs64: got eflags %08lx / %08lx\n", ctx_ptr->EFlags, ctx.EFlags);

                /* changing one context doesn't change the other one */
                rcx = context.Rcx;
                ecx = ctx.Ecx;
                context.Rcx = 0xfedcba987654321ull;
                pNtSetContextThread( pi.hThread, &context );
                memset( &ctx, 0x55, sizeof(ctx) );
                ctx.ContextFlags = WOW64_CONTEXT_ALL;
                pRtlWow64GetThreadContext( pi.hThread, &ctx );
                ok( ctx.Ecx == ecx, "cs64: ecx set to %08lx\n", ctx.Ecx );
                ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED], cpu, cpu_size, &res );
                ok( ctx_ptr->Ecx == ecx, "cs64: ecx set to %08lx\n", ctx_ptr->Ecx );
                ctx.Ecx = 0x22223333;
                pRtlWow64SetThreadContext( pi.hThread, &ctx );
                memset( &ctx, 0x55, sizeof(ctx) );
                ctx.ContextFlags = WOW64_CONTEXT_ALL;
                pRtlWow64GetThreadContext( pi.hThread, &ctx );
                ok( ctx.Ecx == 0x22223333, "cs64: ecx set to %08lx\n", ctx.Ecx );
                ReadProcessMemory( pi.hProcess, teb.TlsSlots[WOW64_TLS_CPURESERVED], cpu, cpu_size, &res );
                todo_wine
                ok( ctx_ptr->Ecx == 0x22223333, "cs64: ecx set to %08lx\n", ctx_ptr->Ecx );
                memset( &context, 0x55, sizeof(context) );
                context.ContextFlags = CONTEXT_ALL;
                pNtGetContextThread( pi.hThread, &context );
                ok( context.Rcx == 0xfedcba987654321ull, "cs64: rcx set to %p\n", (void *)context.Rcx );
                /* restore everything */
                context.Rcx = rcx;
                pNtSetContextThread( pi.hThread, &context );
                ctx.Ecx = ecx;
                pRtlWow64SetThreadContext( pi.hThread, &ctx );
            }
            got64 = TRUE;
            if (is_arm64ec) break; /* no 32-bit %cs on arm64ec */
        }
    }
    if (!got32) skip( "failed to test 32-bit context\n" );
    if (!got64) skip( "failed to test 64-bit context\n" );

done:
    pNtTerminateProcess(pi.hProcess, 0);
    free( cpu );
}

static BYTE saved_KiUserExceptionDispatcher_bytes[12];
static BOOL hook_called;
static void *hook_KiUserExceptionDispatcher_rip;
static void *dbg_except_continue_handler_rip;
static void *hook_exception_address;
static struct
{
    ULONG64 old_rax;
    ULONG64 old_rdx;
    ULONG64 old_rsi;
    ULONG64 old_rdi;
    ULONG64 old_rbp;
    ULONG64 old_rsp;
    ULONG64 new_rax;
    ULONG64 new_rdx;
    ULONG64 new_rsi;
    ULONG64 new_rdi;
    ULONG64 new_rbp;
    ULONG64 new_rsp;
}
test_kiuserexceptiondispatcher_regs;

static ULONG64 test_kiuserexceptiondispatcher_saved_r12;

struct machine_frame
{
    ULONG64 rip;
    ULONG64 cs;
    ULONG64 eflags;
    ULONG64 rsp;
    ULONG64 ss;
};

static DWORD dbg_except_continue_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    trace("handler context->Rip %#Ix, codemem %p.\n", context->Rip, code_mem);
    got_exception = 1;
    dbg_except_continue_handler_rip = (void *)context->Rip;
    ++context->Rip;
    memcpy(pKiUserExceptionDispatcher, saved_KiUserExceptionDispatcher_bytes,
            sizeof(saved_KiUserExceptionDispatcher_bytes));

    RtlUnwind((void *)test_kiuserexceptiondispatcher_regs.old_rsp,
            (BYTE *)code_mem + 0x28, rec, (void *)0xdeadbeef);
    return ExceptionContinueExecution;
}

static LONG WINAPI dbg_except_continue_vectored_handler(struct _EXCEPTION_POINTERS *e)
{
    EXCEPTION_RECORD *rec = e->ExceptionRecord;
    CONTEXT *context = e->ContextRecord;

    trace("dbg_except_continue_vectored_handler, code %#lx, Rip %#Ix.\n", rec->ExceptionCode, context->Rip);

    if (rec->ExceptionCode == 0xceadbeef)
    {
        ok(context->P1Home == (ULONG64)0xdeadbeeffeedcafe, "Got unexpected context->P1Home %#Ix.\n", context->P1Home);
        context->R12 = test_kiuserexceptiondispatcher_saved_r12;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    ok(rec->ExceptionCode == 0x80000003, "Got unexpected exception code %#lx.\n", rec->ExceptionCode);

    got_exception = 1;
    dbg_except_continue_handler_rip = (void *)context->Rip;
    if (NtCurrentTeb()->Peb->BeingDebugged && !is_arm64ec)
        ++context->Rip;

    if (context->Rip >= (ULONG64)code_mem && context->Rip < (ULONG64)code_mem + 0x100)
        RtlUnwind((void *)test_kiuserexceptiondispatcher_regs.old_rsp,
                (BYTE *)code_mem + 0x28, rec, (void *)0xdeadbeef);

    return EXCEPTION_CONTINUE_EXECUTION;
}

static void * WINAPI hook_KiUserExceptionDispatcher(EXCEPTION_RECORD *rec, CONTEXT *context)
{
    struct machine_frame *frame = (struct machine_frame *)(((ULONG_PTR)(rec + 1) + 0x0f) & ~0x0f);
    CONTEXT_EX *xctx = (CONTEXT_EX *)(context + 1);

    trace("rec %p context %p context->Rip %#Ix, context->Rsp %#Ix, ContextFlags %#lx.\n",
          rec, context, context->Rip, context->Rsp, context->ContextFlags);

    hook_called = TRUE;
    /* Broken on Win2008, probably rec offset in stack is different. */
    ok(rec->ExceptionCode == 0x80000003 || rec->ExceptionCode == 0xceadbeef || broken(!rec->ExceptionCode),
            "Got unexpected ExceptionCode %#lx.\n", rec->ExceptionCode);

    ok( !((ULONG_PTR)context & 15), "unaligned context %p\n", context );
    ok( xctx->All.Offset == -sizeof(CONTEXT), "wrong All.Offset %lx\n", xctx->All.Offset );
    ok( xctx->All.Length >= sizeof(CONTEXT) + offsetof(CONTEXT_EX, align), "wrong All.Length %lx\n", xctx->All.Length );
    ok( xctx->Legacy.Offset == -sizeof(CONTEXT), "wrong Legacy.Offset %lx\n", xctx->All.Offset );
    ok( xctx->Legacy.Length == sizeof(CONTEXT), "wrong Legacy.Length %lx\n", xctx->All.Length );
    ok( (void *)(xctx + 1) == (void *)rec, "wrong ptrs %p / %p\n", xctx, rec );
    ok( frame->rip == context->Rip, "wrong rip %Ix / %Ix\n", frame->rip, context->Rip );
    ok( frame->rsp == context->Rsp, "wrong rsp %Ix / %Ix\n", frame->rsp, context->Rsp );

    hook_KiUserExceptionDispatcher_rip = (void *)context->Rip;
    hook_exception_address = rec->ExceptionAddress;
    memcpy(pKiUserExceptionDispatcher, saved_KiUserExceptionDispatcher_bytes,
            sizeof(saved_KiUserExceptionDispatcher_bytes));
    return pKiUserExceptionDispatcher;
}

static void * WINAPI hook_KiUserExceptionDispatcher_arm64ec(EXCEPTION_RECORD *rec, CONTEXT *context)
{
    ARM64_NT_CONTEXT *arm64_context = (ARM64_NT_CONTEXT *)(context + 1);

    trace("rec %p context %p context->Rip %#Ix, context->Rsp %#Ix, ContextFlags %#lx.\n",
          rec, context, context->Rip, context->Rsp, context->ContextFlags);
    hook_called = TRUE;
    ok(rec->ExceptionCode == 0x80000003 || rec->ExceptionCode == 0xceadbeef,
       "Got unexpected ExceptionCode %#lx.\n", rec->ExceptionCode);

    ok( !((ULONG_PTR)context & 15), "unaligned context %p\n", context );
    ok( arm64_context->Pc == context->Rip, "wrong rip %Ix / %Ix\n", arm64_context->Pc, context->Rip );
    ok( arm64_context->Sp == context->Rsp, "wrong rsp %Ix / %Ix\n", arm64_context->Sp, context->Rsp );

    hook_KiUserExceptionDispatcher_rip = (void *)context->Rip;
    hook_exception_address = rec->ExceptionAddress;
    memcpy(pKiUserExceptionDispatcher, saved_KiUserExceptionDispatcher_bytes,
            sizeof(saved_KiUserExceptionDispatcher_bytes));
    return pKiUserExceptionDispatcher;
}

static void test_KiUserExceptionDispatcher(void)
{
    LPVOID vectored_handler;
    static BYTE except_code[] =
    {
        0x48, 0xb9, /* mov imm64, %rcx */
        /* offset: 0x2 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        0x48, 0x89, 0x01,       /* mov %rax, (%rcx) */
        0x48, 0x89, 0x51, 0x08, /* mov %rdx, 0x8(%rcx) */
        0x48, 0x89, 0x71, 0x10, /* mov %rsi, 0x10(%rcx) */
        0x48, 0x89, 0x79, 0x18, /* mov %rdi, 0x18(%rcx) */
        0x48, 0x89, 0x69, 0x20, /* mov %rbp, 0x20(%rcx) */
        0x48, 0x89, 0x61, 0x28, /* mov %rsp, 0x28(%rcx) */
        0x48, 0x83, 0xc1, 0x30, /* add $0x30, %rcx */

        /* offset: 0x25 */
        0xcc,  /* int3 */

        0x0f, 0x0b, /* ud2, illegal instruction */

        /* offset: 0x28 */
        0x48, 0xb9, /* mov imm64, %rcx */
        /* offset: 0x2a */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        0x48, 0x89, 0x01,       /* mov %rax, (%rcx) */
        0x48, 0x89, 0x51, 0x08, /* mov %rdx, 0x8(%rcx) */
        0x48, 0x89, 0x71, 0x10, /* mov %rsi, 0x10(%rcx) */
        0x48, 0x89, 0x79, 0x18, /* mov %rdi, 0x18(%rcx) */
        0x48, 0x89, 0x69, 0x20, /* mov %rbp, 0x20(%rcx) */
        0x48, 0x89, 0x61, 0x28, /* mov %rsp, 0x28(%rcx) */
        0xc3,  /* ret  */
    };
    static BYTE hook_trampoline[] =
    {
        0x48, 0x89, 0xe2,           /* mov %rsp,%rdx */
        0x48, 0x8d, 0x8c, 0x24, 0xf0, 0x04, 0x00, 0x00,
                                    /* lea 0x4f0(%rsp),%rcx */
        0x4c, 0x89, 0x22,           /* mov %r12,(%rdx) */
        0x48, 0xb8,                 /* movabs hook_KiUserExceptionDispatcher,%rax */
        0,0,0,0,0,0,0,0,            /* offset 16 */
        0xff, 0xd0,                 /* callq *rax */
        0x48, 0x31, 0xc9,           /* xor %rcx, %rcx */
        0x48, 0x31, 0xd2,           /* xor %rdx, %rdx */
        0xff, 0xe0,                 /* jmpq *rax */
    };
    static BYTE hook_trampoline_arm64ec[] =
    {
        0x48, 0x8d, 0x54, 0x24, 0x08, /* lea 0x8(%rsp),%rdx */
        0x48, 0x8d, 0x8a, 0x60, 0x08, 0x00, 0x00,
 	                            /* lea 0x860(%rdx),%rcx */
        0x4c, 0x89, 0x22,           /* mov %r12,(%rdx) */
        0x48, 0xb8,                 /* movabs hook_KiUserExceptionDispatcher_arm64ec,%rax */
        0,0,0,0,0,0,0,0,            /* offset 16 */
        0xff, 0xd0,                 /* callq *rax */
        0x48, 0x31, 0xc9,           /* xor %rcx, %rcx */
        0x48, 0x31, 0xd2,           /* xor %rdx, %rdx */
        0xff, 0xe0,                 /* jmpq *rax */
    };

    BYTE patched_KiUserExceptionDispatcher_bytes[12];
    void *bpt_address, *trampoline_ptr;
    EXCEPTION_RECORD record;
    DWORD old_protect;
    CONTEXT ctx;
    LONG pass;
    BYTE *ptr;
    BOOL ret;

    *(ULONG64 *)(except_code + 2) = (ULONG64)&test_kiuserexceptiondispatcher_regs;
    *(ULONG64 *)(except_code + 0x2a) = (ULONG64)&test_kiuserexceptiondispatcher_regs.new_rax;

    *(ULONG_PTR *)(hook_trampoline_arm64ec + 17) = (ULONG_PTR)hook_KiUserExceptionDispatcher_arm64ec;
    *(ULONG_PTR *)(hook_trampoline + 16) = (ULONG_PTR)hook_KiUserExceptionDispatcher;
    trampoline_ptr = (char *)code_mem + 1024;
    if (is_arm64ec)
        memcpy(trampoline_ptr, hook_trampoline_arm64ec, sizeof(hook_trampoline_arm64ec));
    else
        memcpy(trampoline_ptr, hook_trampoline, sizeof(hook_trampoline));

    ret = VirtualProtect(pKiUserExceptionDispatcher, sizeof(saved_KiUserExceptionDispatcher_bytes),
            PAGE_EXECUTE_READWRITE, &old_protect);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    memcpy(saved_KiUserExceptionDispatcher_bytes, pKiUserExceptionDispatcher,
            sizeof(saved_KiUserExceptionDispatcher_bytes));
    ptr = (BYTE *)patched_KiUserExceptionDispatcher_bytes;
    /* mov hook_trampoline, %rax */
    *ptr++ = 0x48;
    *ptr++ = 0xb8;
    *(void **)ptr = trampoline_ptr;
    ptr += sizeof(ULONG64);
    /* jmp *rax */
    *ptr++ = 0xff;
    *ptr++ = 0xe0;

    memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
            sizeof(patched_KiUserExceptionDispatcher_bytes));
    got_exception = 0;
    run_exception_test(dbg_except_continue_handler, NULL, except_code, sizeof(except_code), PAGE_EXECUTE_READ);
    ok(got_exception, "Handler was not called.\n");
    todo_wine_if(is_arm64ec)
    ok(hook_called, "Hook was not called.\n");
    if (!hook_called) return;

    ok(test_kiuserexceptiondispatcher_regs.new_rax == 0xdeadbeef, "Got unexpected rax %#Ix.\n",
            test_kiuserexceptiondispatcher_regs.new_rax);
    ok(test_kiuserexceptiondispatcher_regs.old_rsi
            == test_kiuserexceptiondispatcher_regs.new_rsi, "rsi does not match.\n");
    ok(test_kiuserexceptiondispatcher_regs.old_rdi
            == test_kiuserexceptiondispatcher_regs.new_rdi, "rdi does not match.\n");
    ok(test_kiuserexceptiondispatcher_regs.old_rbp
            == test_kiuserexceptiondispatcher_regs.new_rbp, "rbp does not match.\n");

    bpt_address = (BYTE *)code_mem + 0x25;

    ok(hook_exception_address == bpt_address || broken(!hook_exception_address) /* Win2008 */,
            "Got unexpected exception address %p, expected %p.\n",
            hook_exception_address, bpt_address);
    ok(hook_KiUserExceptionDispatcher_rip == bpt_address, "Got unexpected exception address %p, expected %p.\n",
            hook_KiUserExceptionDispatcher_rip, bpt_address);
    ok(dbg_except_continue_handler_rip == bpt_address, "Got unexpected exception address %p, expected %p.\n",
            dbg_except_continue_handler_rip, bpt_address);

    memset(&record, 0, sizeof(record));
    record.ExceptionCode = 0x80000003;
    record.ExceptionFlags = 0;
    record.ExceptionRecord = NULL;
    record.ExceptionAddress = NULL;
    record.NumberParameters = 0;

    vectored_handler = AddVectoredExceptionHandler(TRUE, dbg_except_continue_vectored_handler);

    memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
            sizeof(patched_KiUserExceptionDispatcher_bytes));
    got_exception = 0;
    hook_called = FALSE;

    pRtlRaiseException(&record);

    ok(got_exception, "Handler was not called.\n");
    ok(!hook_called, "Hook was called.\n");

    memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
            sizeof(patched_KiUserExceptionDispatcher_bytes));
    got_exception = 0;
    hook_called = FALSE;
    NtCurrentTeb()->Peb->BeingDebugged = 1;

    pRtlRaiseException(&record);

    ok(got_exception, "Handler was not called.\n");
    ok(hook_called, "Hook was not called.\n");

    ok(hook_exception_address == (BYTE *)hook_KiUserExceptionDispatcher_rip + !is_arm64ec
            || broken(!hook_exception_address) /* 2008 */, "Got unexpected addresses %p, %p.\n",
            hook_KiUserExceptionDispatcher_rip, hook_exception_address);

    RemoveVectoredExceptionHandler(vectored_handler);

    memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
            sizeof(patched_KiUserExceptionDispatcher_bytes));
    got_exception = 0;
    hook_called = FALSE;

    run_exception_test(dbg_except_continue_handler, NULL, except_code, sizeof(except_code), PAGE_EXECUTE_READ);

    ok(got_exception, "Handler was not called.\n");
    ok(hook_called, "Hook was not called.\n");
    ok(hook_KiUserExceptionDispatcher_rip == bpt_address, "Got unexpected exception address %p, expected %p.\n",
            hook_KiUserExceptionDispatcher_rip, bpt_address);
    ok(dbg_except_continue_handler_rip == bpt_address, "Got unexpected exception address %p, expected %p.\n",
            dbg_except_continue_handler_rip, bpt_address);

    ok(test_kiuserexceptiondispatcher_regs.new_rax == 0xdeadbeef, "Got unexpected rax %#Ix.\n",
            test_kiuserexceptiondispatcher_regs.new_rax);
    ok(test_kiuserexceptiondispatcher_regs.old_rsi
            == test_kiuserexceptiondispatcher_regs.new_rsi, "rsi does not match.\n");
    ok(test_kiuserexceptiondispatcher_regs.old_rdi
            == test_kiuserexceptiondispatcher_regs.new_rdi, "rdi does not match.\n");
    ok(test_kiuserexceptiondispatcher_regs.old_rbp
            == test_kiuserexceptiondispatcher_regs.new_rbp, "rbp does not match.\n");

    NtCurrentTeb()->Peb->BeingDebugged = 0;

    vectored_handler = AddVectoredExceptionHandler(TRUE, dbg_except_continue_vectored_handler);
    pass = 0;
    InterlockedIncrement(&pass);
    pRtlCaptureContext(&ctx);
    if (InterlockedIncrement(&pass) == 2) /* interlocked to prevent compiler from moving before capture */
    {
        memcpy(pKiUserExceptionDispatcher, patched_KiUserExceptionDispatcher_bytes,
                sizeof(patched_KiUserExceptionDispatcher_bytes));
        got_exception = 0;
        hook_called = FALSE;

        record.ExceptionCode = 0xceadbeef;
        test_kiuserexceptiondispatcher_saved_r12 = ctx.R12;
        ctx.R12 = (ULONG64)0xdeadbeeffeedcafe;

#if defined(__REACTOS__) && defined(_MSC_VER)
        Call_NtRaiseException(&record, &ctx, TRUE, pNtRaiseException);
#else
#ifdef __GNUC__
        /* Spoil r12 value to make sure it doesn't come from the current userspace registers. */
        __asm__ volatile("movq $0xdeadcafe, %%r12" : : : "%r12");
#endif
        pNtRaiseException(&record, &ctx, TRUE);
#endif
        ok(0, "Shouldn't be reached.\n");
    }
    else
    {
        ok(pass == 3, "Got unexpected pass %ld.\n", pass);
    }
    ok(hook_called, "Hook was not called.\n");
    RemoveVectoredExceptionHandler(vectored_handler);

    ret = VirtualProtect(pKiUserExceptionDispatcher, sizeof(saved_KiUserExceptionDispatcher_bytes),
            old_protect, &old_protect);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
}


static BYTE saved_KiUserApcDispatcher[12];

static void * WINAPI hook_KiUserApcDispatcher(CONTEXT *context)
{
    struct machine_frame *frame = (struct machine_frame *)(context + 1);
    UINT i;

    trace( "context %p, context->Rip %#Ix, context->Rsp %#Ix (%#Ix), ContextFlags %#lx.\n",
           context, context->Rip, context->Rsp,
           (char *)context->Rsp - (char *)context, context->ContextFlags );

    ok( context->P1Home == 0x1234, "wrong p1 %#Ix\n", context->P1Home );
    ok( context->P2Home == 0x5678, "wrong p2 %#Ix\n", context->P2Home );
    ok( context->P3Home == 0xdeadbeef, "wrong p3 %#Ix\n", context->P3Home );
    ok( context->P4Home == (ULONG_PTR)apc_func, "wrong p4 %#Ix / %p\n", context->P4Home, apc_func );

    /* machine frame offset varies between Windows versions */
    for (i = 0; i < 16; i++)
    {
        if (frame->rip == context->Rip) break;
        frame = (struct machine_frame *)((ULONG64 *)frame + 2);
    }
    ok( frame->rip == context->Rip, "wrong rip %#Ix / %#Ix\n", frame->rip, context->Rip );
    ok( frame->rsp == context->Rsp, "wrong rsp %#Ix / %#Ix\n", frame->rsp, context->Rsp );

    hook_called = TRUE;
    memcpy( pKiUserApcDispatcher, saved_KiUserApcDispatcher, sizeof(saved_KiUserApcDispatcher));
    return pKiUserApcDispatcher;
}

static void test_KiUserApcDispatcher(void)
{
    BYTE hook_trampoline[] =
    {
        0x48, 0x89, 0xe1,           /* mov %rsp,%rcx */
        0x48, 0xb8,                 /* movabs hook_KiUserApcDispatcher,%rax */
        0,0,0,0,0,0,0,0,            /* offset 5 */
        0xff, 0xd0,                 /* callq *rax */
        0xff, 0xe0,                 /* jmpq *rax */
    };

    BYTE patched_KiUserApcDispatcher[12];
    DWORD old_protect;
    BYTE *ptr;
    BOOL ret;

    *(ULONG_PTR *)(hook_trampoline + 5) = (ULONG_PTR)hook_KiUserApcDispatcher;
    memcpy(code_mem, hook_trampoline, sizeof(hook_trampoline));

    ret = VirtualProtect( pKiUserApcDispatcher, sizeof(saved_KiUserApcDispatcher),
                          PAGE_EXECUTE_READWRITE, &old_protect );
    ok( ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError() );

    memcpy( saved_KiUserApcDispatcher, pKiUserApcDispatcher, sizeof(saved_KiUserApcDispatcher) );
    ptr = patched_KiUserApcDispatcher;
    /* mov $code_mem, %rax */
    *ptr++ = 0x48;
    *ptr++ = 0xb8;
    *(void **)ptr = code_mem;
    ptr += sizeof(ULONG64);
    /* jmp *rax */
    *ptr++ = 0xff;
    *ptr++ = 0xe0;
    memcpy( pKiUserApcDispatcher, patched_KiUserApcDispatcher, sizeof(patched_KiUserApcDispatcher) );

    hook_called = FALSE;
    apc_count = 0;
    pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234, 0x5678, 0xdeadbeef );
    SleepEx( 0, TRUE );
    ok( apc_count == 1, "APC was not called\n" );
    /* hooking is bypassed on arm64ec */
    ok( is_arm64ec ? !hook_called : hook_called, "hook was not called\n" );

    VirtualProtect( pKiUserApcDispatcher, sizeof(saved_KiUserApcDispatcher), old_protect, &old_protect );
}

static void WINAPI hook_KiUserCallbackDispatcher(void *rsp)
{
    struct
    {
        ULONG64              padding[4];
        void                *args;
        ULONG                len;
        ULONG                id;
        struct machine_frame frame;
        BYTE                 args_data[0];
    } *stack = rsp;

    KERNEL_CALLBACK_PROC func = NtCurrentTeb()->Peb->KernelCallbackTable[stack->id];

    trace( "rsp %p args %p (%#Ix) len %lu id %lu\n", stack, stack->args,
           (char *)stack->args - (char *)stack, stack->len, stack->id );

    ok( !((ULONG_PTR)stack & 15), "unaligned stack %p\n", stack );
    ok( stack->args == stack->args_data, "wrong args %p / %p\n", stack->args, stack->args_data );
    ok( (BYTE *)stack->frame.rsp - &stack->args_data[stack->len] <= 16, "wrong rsp %p / %p\n",
        (void *)stack->frame.rsp, &stack->args_data[stack->len] );

    if (stack->frame.rip && pRtlPcToFileHeader)
    {
        void *mod, *win32u = GetModuleHandleA("win32u.dll");

        pRtlPcToFileHeader( (void *)stack->frame.rip, &mod );
        if (win32u) ok( mod == win32u, "ret address %Ix not in win32u %p\n", stack->frame.rip, win32u );
        else trace( "ret address %Ix in %p\n", stack->frame.rip, mod );
    }
    NtCallbackReturn( NULL, 0, func( stack->args, stack->len ));
}

static void test_KiUserCallbackDispatcher(void)
{
    BYTE hook_trampoline[] =
    {
        0x48, 0x89, 0xe1,           /* mov %rsp,%rcx */
        0x48, 0xb8,                 /* movabs hook_KiUserCallbackDispatcher,%rax */
        0,0,0,0,0,0,0,0,            /* offset 5 */
        0xff, 0xd0,                 /* callq *rax */
    };

    BYTE saved_KiUserCallbackDispatcher[12];
    BYTE patched_KiUserCallbackDispatcher[12];
    DWORD old_protect;
    BYTE *ptr;
    BOOL ret;

    *(ULONG_PTR *)(hook_trampoline + 5) = (ULONG_PTR)hook_KiUserCallbackDispatcher;
    memcpy(code_mem, hook_trampoline, sizeof(hook_trampoline));

    ret = VirtualProtect( pKiUserCallbackDispatcher, sizeof(saved_KiUserCallbackDispatcher),
                          PAGE_EXECUTE_READWRITE, &old_protect );
    ok( ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError() );

    memcpy( saved_KiUserCallbackDispatcher, pKiUserCallbackDispatcher, sizeof(saved_KiUserCallbackDispatcher) );
    ptr = patched_KiUserCallbackDispatcher;
    /* mov $code_mem, %rax */
    *ptr++ = 0x48;
    *ptr++ = 0xb8;
    *(void **)ptr = code_mem;
    ptr += sizeof(ULONG64);
    /* jmp *rax */
    *ptr++ = 0xff;
    *ptr++ = 0xe0;
    memcpy( pKiUserCallbackDispatcher, patched_KiUserCallbackDispatcher, sizeof(patched_KiUserCallbackDispatcher) );

    DestroyWindow( CreateWindowA( "Static", "test", 0, 0, 0, 0, 0, 0, 0, 0, 0 ));

    memcpy( pKiUserCallbackDispatcher, saved_KiUserCallbackDispatcher, sizeof(saved_KiUserCallbackDispatcher));
    VirtualProtect( pKiUserCallbackDispatcher, sizeof(saved_KiUserCallbackDispatcher), old_protect, &old_protect );
}

static BOOL got_nested_exception, got_prev_frame_exception;
static void *nested_exception_initial_frame;

static DWORD nested_exception_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    trace("nested_exception_handler Rip %p, Rsp %p, code %#lx, flags %#lx, ExceptionAddress %p.\n",
            (void *)context->Rip, (void *)context->Rsp, rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress);

    if (rec->ExceptionCode == 0x80000003
            && !(rec->ExceptionFlags & EXCEPTION_NESTED_CALL))
    {
        ok(rec->NumberParameters == 1, "Got unexpected rec->NumberParameters %lu.\n", rec->NumberParameters);
        ok((void *)context->Rsp == frame, "Got unexpected frame %p.\n", frame);
        ok(*(void **)frame == (char *)code_mem + 5, "Got unexpected *frame %p.\n", *(void **)frame);
        ok(context->Rip == (ULONG_PTR)((char *)code_mem + 7), "Got unexpected Rip %#Ix.\n", context->Rip);

        nested_exception_initial_frame = frame;
        RaiseException(0xdeadbeef, 0, 0, 0);
        ++context->Rip;
        return ExceptionContinueExecution;
    }

    if (rec->ExceptionCode == 0xdeadbeef && (rec->ExceptionFlags == EXCEPTION_NESTED_CALL
            || rec->ExceptionFlags == (EXCEPTION_NESTED_CALL | EXCEPTION_SOFTWARE_ORIGINATE)))
    {
        ok(!rec->NumberParameters, "Got unexpected rec->NumberParameters %lu.\n", rec->NumberParameters);
        got_nested_exception = TRUE;
        ok(frame == nested_exception_initial_frame, "Got unexpected frame %p.\n", frame);
        return ExceptionContinueSearch;
    }

    ok(rec->ExceptionCode == 0xdeadbeef && (!rec->ExceptionFlags || rec->ExceptionFlags == EXCEPTION_SOFTWARE_ORIGINATE),
            "Got unexpected exception code %#lx, flags %#lx.\n", rec->ExceptionCode, rec->ExceptionFlags);
    ok(!rec->NumberParameters, "Got unexpected rec->NumberParameters %lu.\n", rec->NumberParameters);
    ok(frame == (void *)((BYTE *)nested_exception_initial_frame + 8),
            "Got unexpected frame %p.\n", frame);
    got_prev_frame_exception = TRUE;
    return ExceptionContinueExecution;
}

static const BYTE nested_except_code[] =
{
    0xe8, 0x02, 0x00, 0x00, 0x00, /* call nest */
    0x90,                         /* nop */
    0xc3,                         /* ret */
    /* nest: */
    0xcc,                         /* int3 */
    0x90,                         /* nop */
    0xc3,                         /* ret  */
};

static void test_nested_exception(void)
{
    got_nested_exception = got_prev_frame_exception = FALSE;
    run_exception_test(nested_exception_handler, NULL, nested_except_code, sizeof(nested_except_code), PAGE_EXECUTE_READ);
    ok(got_nested_exception, "Did not get nested exception.\n");
    ok(got_prev_frame_exception, "Did not get nested exception in the previous frame.\n");
}

static unsigned int collided_unwind_exception_count;

static DWORD collided_exception_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    CONTEXT ctx;

    trace("collided_exception_handler Rip %p, Rsp %p, code %#lx, flags %#lx, ExceptionAddress %p, frame %p.\n",
            (void *)context->Rip, (void *)context->Rsp, rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, frame);

    switch(collided_unwind_exception_count++)
    {
        case 0:
            /* Initial exception from nested_except_code. */
            ok(rec->ExceptionCode == STATUS_BREAKPOINT, "got %#lx.\n", rec->ExceptionCode);
            nested_exception_initial_frame = frame;
            /* Start unwind. */
            pRtlUnwindEx((char *)nested_exception_initial_frame + 8, (char *)code_mem + 5, NULL, NULL, &ctx, NULL);
            ok(0, "shouldn't be reached\n");
            break;
        case 1:
            ok(rec->ExceptionCode == STATUS_UNWIND, "got %#lx.\n", rec->ExceptionCode);
            ok(rec->ExceptionFlags == EXCEPTION_UNWINDING, "got %#lx.\n", rec->ExceptionFlags);
            ok((char *)context->Rip == (char *)code_mem + 7, "got %p.\n", rec->ExceptionAddress);
            /* generate exception in unwind handler. */
            RaiseException(0xdeadbeef, 0, 0, 0);
            ok(0, "shouldn't be reached\n");
            break;
        case 2:
            /* Inner call frame, continue search. */
            ok(rec->ExceptionCode == 0xdeadbeef, "got %#lx.\n", rec->ExceptionCode);
            ok(!rec->ExceptionFlags || rec->ExceptionFlags == EXCEPTION_SOFTWARE_ORIGINATE, "got %#lx.\n", rec->ExceptionFlags);
            ok(frame == nested_exception_initial_frame, "got %p, expected %p.\n", frame, nested_exception_initial_frame);
            break;
        case 3:
            /* Top level call frame, handle exception by unwinding. */
            ok(rec->ExceptionCode == 0xdeadbeef, "got %#lx.\n", rec->ExceptionCode);
            ok(!rec->ExceptionFlags || rec->ExceptionFlags == EXCEPTION_SOFTWARE_ORIGINATE, "got %#lx.\n", rec->ExceptionFlags);
            ok((char *)frame == (char *)nested_exception_initial_frame + 8, "got %p, expected %p.\n", frame, nested_exception_initial_frame);
            pRtlUnwindEx((char *)nested_exception_initial_frame + 8, (char *)code_mem + 5, NULL, NULL, &ctx, NULL);
            ok(0, "shouldn't be reached\n");
            break;
        case 4:
            /* Collided unwind. */
            ok(rec->ExceptionCode == STATUS_UNWIND, "got %#lx.\n", rec->ExceptionCode);
            ok(rec->ExceptionFlags == (EXCEPTION_UNWINDING | EXCEPTION_COLLIDED_UNWIND), "got %#lx.\n", rec->ExceptionFlags);
            ok(frame == nested_exception_initial_frame, "got %p, expected %p.\n", frame, nested_exception_initial_frame);
            break;
        case 5:
            /* EXCEPTION_COLLIDED_UNWIND cleared for the following frames. */
            ok(rec->ExceptionCode == STATUS_UNWIND, "got %#lx.\n", rec->ExceptionCode);
            ok(rec->ExceptionFlags == (EXCEPTION_UNWINDING | EXCEPTION_TARGET_UNWIND), "got %#lx.\n", rec->ExceptionFlags);
            ok((char *)frame == (char *)nested_exception_initial_frame + 8, "got %p, expected %p.\n", frame,
                    (char *)nested_exception_initial_frame + 8);
            break;
    }
    return ExceptionContinueSearch;
}

static void test_collided_unwind(void)
{
    got_nested_exception = got_prev_frame_exception = FALSE;
    collided_unwind_exception_count = 0;
    run_exception_test_flags(collided_exception_handler, NULL, nested_except_code, sizeof(nested_except_code),
            PAGE_EXECUTE_READ, UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER);
    ok(collided_unwind_exception_count == 6, "got %u.\n", collided_unwind_exception_count);
}

static CONTEXT test_unwind_apc_context;
static BOOL test_unwind_apc_called;

static void CALLBACK test_unwind_apc(ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3)
{
    EXCEPTION_RECORD rec;

    test_unwind_apc_called = TRUE;
    memset(&rec, 0, sizeof(rec));
    pRtlUnwind((void *)test_unwind_apc_context.Rsp, (void *)test_unwind_apc_context.Rip, &rec, (void *)0xdeadbeef);
    ok(0, "Should not get here.\n");
}

static void test_unwind_from_apc(void)
{
    NTSTATUS status;
    LONG pass;

    if (!pNtQueueApcThread)
    {
        win_skip("NtQueueApcThread is not available.\n");
        return;
    }

    pass = 0;
    InterlockedIncrement(&pass);
    RtlCaptureContext(&test_unwind_apc_context);
    InterlockedIncrement(&pass);

    if (pass == 2)
    {
        test_unwind_apc_called = FALSE;
        status = pNtQueueApcThread(GetCurrentThread(), test_unwind_apc, 0, 0, 0);
        ok(!status, "Got unexpected status %#lx.\n", status);
        SleepEx(0, TRUE);
        ok(0, "Should not get here.\n");
    }
    if (pass == 3)
    {
        ok(test_unwind_apc_called, "Test user APC was not called.\n");
        test_unwind_apc_called = FALSE;
        status = pNtQueueApcThread(GetCurrentThread(), test_unwind_apc, 0, 0, 0);
        ok(!status, "Got unexpected status %#lx.\n", status);
        NtContinue(&test_unwind_apc_context, TRUE );
        ok(0, "Should not get here.\n");
    }
    ok(pass == 4, "Got unexpected pass %ld.\n", pass);
    ok(test_unwind_apc_called, "Test user APC was not called.\n");
}

static void test_syscall_clobbered_regs(void)
{
    struct regs
    {
        UINT64 rcx;
        UINT64 r10;
        UINT64 r11;
        UINT32 eflags;
    };
    static const BYTE code[] =
    {
        0x48, 0x8d, 0x05, 0x00, 0x10, 0x00, 0x00,
                                    /* leaq 0x1000(%rip),%rax */
        0x48, 0x25, 0x00, 0xf0, 0xff, 0xff,
                                    /* andq $~0xfff,%rax */
        0x48, 0x83, 0xe8, 0x08,     /* subq $8,%rax */
        0x48, 0x89, 0x20,           /* movq %rsp,0(%rax) */
        0x48, 0x89, 0xc4,           /* movq %rax,%rsp */
        0xfd,                       /* std */
        0x45, 0x31, 0xdb,           /* xorl %r11d,%r11d */
        0x41, 0x50,                 /* push %r8 */
        0x53, 0x55, 0x57, 0x56, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57,
                                    /* push %rbx, %rbp, %rdi, %rsi, %r12, %r13, %r14, %r15 */
        0x49, 0xba, 0xef, 0xbe, 0xad, 0xde, 0x00, 0x00, 0x00, 0x00,
                                    /* movabs $0xdeadbeef,%r10 */
        0x41, 0xff, 0xd1,           /* callq *r9 */
        0x41, 0x5f, 0x41, 0x5e, 0x41, 0x5d, 0x41, 0x5c, 0x5e, 0x5f, 0x5d, 0x5b,
                                    /* pop %r15, %r14, %r13, %r12, %rsi, %rdi, %rbp, %rbx */
        0x41, 0x58,                 /* pop %r8 */
        0x49, 0x89, 0x48, 0x00,     /* mov %rcx,(%r8) */
        0x4d, 0x89, 0x50, 0x08,     /* mov %r10,0x8(%r8) */
        0x4d, 0x89, 0x58, 0x10,     /* mov %r11,0x10(%r8) */
        0x9c,                       /* pushfq */
        0x59,                       /* pop %rcx */
        0xfc,                       /* cld */
        0x41, 0x89, 0x48, 0x18,     /* mov %ecx,0x18(%r8) */
        0x5c,                       /* pop %rsp */
        0xc3,                       /* ret */
    };

    NTSTATUS (WINAPI *func)(void *arg1, void *arg2, struct regs *, void *call_addr);
    NTSTATUS (WINAPI *pNtCancelTimer)(HANDLE, BOOLEAN *);
    struct regs regs;
    CONTEXT context;
    NTSTATUS status;

    if (is_arm64ec) return;  /* arm64ec register handling is different */

    pNtCancelTimer = (void *)GetProcAddress(hntdll, "NtCancelTimer");
    ok(!!pNtCancelTimer, "NtCancelTimer not found.\n");
    memcpy(code_mem, code, sizeof(code));
    func = code_mem;
    memset(&regs, 0, sizeof(regs));
    status = func((HANDLE)0xdeadbeef, NULL, &regs, pNtCancelTimer);
    ok(status == STATUS_INVALID_HANDLE, "Got unexpected status %#lx.\n", status);
    ok(regs.r11 == regs.eflags, "Expected r11 (%#I64x) to equal EFLAGS (%#x).\n", regs.r11, regs.eflags);
    ok(regs.r10 != regs.rcx, "got %#I64x.\n", regs.r10);

    /* After the syscall instruction rcx contains the address of the instruction next after syscall. */
    ok((BYTE *)regs.rcx > (BYTE *)pNtCancelTimer && (BYTE *)regs.rcx < (BYTE *)pNtCancelTimer + 0x20,
            "Got unexpected rcx %s, pNtCancelTimer %p.\n", wine_dbgstr_longlong(regs.rcx), pNtCancelTimer);

    status = func((HANDLE)0xdeadbeef, (BOOLEAN *)0xdeadbeef, &regs, pNtCancelTimer);
    ok(status == STATUS_ACCESS_VIOLATION, "Got unexpected status %#lx.\n", status);
    ok((BYTE *)regs.rcx > (BYTE *)pNtCancelTimer && (BYTE *)regs.rcx < (BYTE *)pNtCancelTimer + 0x20,
            "Got unexpected rcx %s, pNtCancelTimer %p.\n", wine_dbgstr_longlong(regs.rcx), pNtCancelTimer);
    ok(regs.r11 == regs.eflags, "Expected r11 (%#I64x) to equal EFLAGS (%#x).\n", regs.r11, regs.eflags);
    ok(regs.r10 != regs.rcx, "got %#I64x.\n", regs.r10);

    context.ContextFlags = CONTEXT_CONTROL;
    status = func(GetCurrentThread(), &context, &regs, pNtGetContextThread);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok((BYTE *)regs.rcx > (BYTE *)pNtGetContextThread && (BYTE *)regs.rcx < (BYTE *)pNtGetContextThread + 0x20,
            "Got unexpected rcx %s, pNtGetContextThread %p.\n", wine_dbgstr_longlong(regs.rcx), pNtGetContextThread);
    ok(regs.r11 == regs.eflags, "Expected r11 (%#I64x) to equal EFLAGS (%#x).\n", regs.r11, regs.eflags);
    ok(regs.r10 != regs.rcx, "got %#I64x.\n", regs.r10);

    status = func(GetCurrentThread(), &context, &regs, pNtSetContextThread);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok((BYTE *)regs.rcx > (BYTE *)pNtGetContextThread && (BYTE *)regs.rcx < (BYTE *)pNtGetContextThread + 0x20,
            "Got unexpected rcx %s, pNtGetContextThread %p.\n", wine_dbgstr_longlong(regs.rcx), pNtGetContextThread);
    ok((regs.r11 | 0x2) == regs.eflags, "Expected r11 (%#I64x) | 0x2 to equal EFLAGS (%#x).\n", regs.r11, regs.eflags);
    ok(regs.r10 != regs.rcx, "got %#I64x.\n", regs.r10);

    context.ContextFlags = CONTEXT_INTEGER;
    status = func(GetCurrentThread(), &context, &regs, pNtGetContextThread);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok((BYTE *)regs.rcx > (BYTE *)pNtGetContextThread && (BYTE *)regs.rcx < (BYTE *)pNtGetContextThread + 0x20,
            "Got unexpected rcx %s, pNtGetContextThread %p.\n", wine_dbgstr_longlong(regs.rcx), pNtGetContextThread);
    ok(regs.r11 == regs.eflags, "Expected r11 (%#I64x) to equal EFLAGS (%#x).\n", regs.r11, regs.eflags);
    ok(regs.r10 != regs.rcx, "got %#I64x.\n", regs.r10);

    status = func(GetCurrentThread(), &context, &regs, pNtSetContextThread);
    ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok((BYTE *)regs.rcx > (BYTE *)pNtSetContextThread && (BYTE *)regs.rcx < (BYTE *)pNtSetContextThread + 0x20,
            "Got unexpected rcx %s, pNtSetContextThread %p.\n", wine_dbgstr_longlong(regs.rcx), pNtSetContextThread);
    ok(regs.r11 == regs.eflags, "Expected r11 (%#I64x) to equal EFLAGS (%#x).\n", regs.r11, regs.eflags);
    ok(regs.r10 != regs.rcx, "got %#I64x.\n", regs.r10);
}

static CONTEXT test_raiseexception_regs_context;
static LONG CALLBACK test_raiseexception_regs_handle(EXCEPTION_POINTERS *exception_info)
{
    EXCEPTION_RECORD *rec = exception_info->ExceptionRecord;
    unsigned int i;

    test_raiseexception_regs_context = *exception_info->ContextRecord;
    ok(rec->NumberParameters == EXCEPTION_MAXIMUM_PARAMETERS, "got %lu.\n", rec->NumberParameters);
    ok(rec->ExceptionCode == 0xdeadbeaf, "got %#lx.\n", rec->ExceptionCode);
    ok(!rec->ExceptionRecord, "got %p.\n", rec->ExceptionRecord);
    ok(!rec->ExceptionFlags || rec->ExceptionFlags == EXCEPTION_SOFTWARE_ORIGINATE, "got %#lx.\n", rec->ExceptionFlags);
    for (i = 0; i < rec->NumberParameters; ++i)
        ok(rec->ExceptionInformation[i] == i, "got %Iu, i %u.\n", rec->ExceptionInformation[i], i);
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void test_raiseexception_regs(void)
{
    static const BYTE code[] =
    {
        0xb8, 0x00, 0xb0, 0xad, 0xde,       /* mov $0xdeadb000,%eax */
        0x53,                               /* push %rbx */
        0x48, 0x89, 0xc3,                   /* mov %rax,%rbx */
        0x56,                               /* push %rsi */
        0x48, 0xff, 0xc0,                   /* inc %rax */
        0x48, 0x89, 0xc6,                   /* mov %rax,%rsi */
        0x57,                               /* push %rdi */
        0x48, 0xff, 0xc0,                   /* inc %rax */
        0x48, 0x89, 0xc7,                   /* mov %rax,%rdi */
        0x55,                               /* push %rbp */
        0x48, 0xff, 0xc0,                   /* inc %rax */
        0x48, 0x89, 0xc5,                   /* mov %rax,%rbp */
        0x41, 0x54,                         /* push %r12 */
        0x48, 0xff, 0xc0,                   /* inc %rax */
        0x49, 0x89, 0xc4,                   /* mov %rax,%r12 */
        0x41, 0x55,                         /* push %r13 */
        0x48, 0xff, 0xc0,                   /* inc %rax */
        0x49, 0x89, 0xc5,                   /* mov %rax,%r13 */
        0x41, 0x56,                         /* push %r14 */
        0x48, 0xff, 0xc0,                   /* inc %rax */
        0x49, 0x89, 0xc6,                   /* mov %rax,%r14 */
        0x41, 0x57,                         /* push %r15 */
        0x48, 0xff, 0xc0,                   /* inc %rax */
        0x49, 0x89, 0xc7,                   /* mov %rax,%r15 */

        0x50,                               /* push %rax */ /* align stack */
        0x48, 0x89, 0xc8,                   /* mov %rcx,%rax */
        0xb9, 0xaf, 0xbe, 0xad, 0xde,       /* mov $0xdeadbeaf,%ecx */
        0xff, 0xd0,                         /* call *%rax */
        0x58,                               /* pop %rax */

        0x41, 0x5f,                         /* pop %r15 */
        0x41, 0x5e,                         /* pop %r14 */
        0x41, 0x5d,                         /* pop %r13 */
        0x41, 0x5c,                         /* pop %r12 */
        0x5d,                               /* pop %rbp */
        0x5f,                               /* pop %rdi */
        0x5e,                               /* pop %rsi */
        0x5b,                               /* pop %rbx */
        0xc3,                               /* ret */
    };
    void (WINAPI *pRaiseException)( DWORD code, DWORD flags, DWORD count, const ULONG_PTR *args ) = RaiseException;
    void (WINAPI *func)(void *raise_exception, DWORD flags, DWORD count, const ULONG_PTR *args);
    void *vectored_handler;
    ULONG_PTR args[20];
    ULONG64 expected;
    unsigned int i;

    vectored_handler = AddVectoredExceptionHandler(TRUE, test_raiseexception_regs_handle);
    ok(!!vectored_handler, "failed.\n");

    memcpy(code_mem, code, sizeof(code));
    func = code_mem;

    for (i = 0; i < ARRAY_SIZE(args); ++i)
        args[i] = i;

    func(pRaiseException, 0, ARRAY_SIZE(args), args);
    expected = 0xdeadb000;
    ok(test_raiseexception_regs_context.Rbx == expected, "got %#I64x.\n", test_raiseexception_regs_context.Rbx);
    ++expected;
    ok(test_raiseexception_regs_context.Rsi == expected, "got %#I64x.\n", test_raiseexception_regs_context.Rsi);
    ++expected;
    ok(test_raiseexception_regs_context.Rdi == expected, "got %#I64x.\n", test_raiseexception_regs_context.Rdi);
    ++expected;
    ok(test_raiseexception_regs_context.Rbp == expected || is_arm64ec /* x29 modified by entry thunk */,
       "got %#I64x.\n", test_raiseexception_regs_context.Rbp);
    ++expected;
    ok(test_raiseexception_regs_context.R12 == expected, "got %#I64x.\n", test_raiseexception_regs_context.R12);
    ++expected;
    ok(test_raiseexception_regs_context.R13 == expected, "got %#I64x.\n", test_raiseexception_regs_context.R13);
    ++expected;
    ok(test_raiseexception_regs_context.R14 == expected, "got %#I64x.\n", test_raiseexception_regs_context.R14);
    ++expected;
    ok(test_raiseexception_regs_context.R15 == expected, "got %#I64x.\n", test_raiseexception_regs_context.R15);

    RemoveVectoredExceptionHandler(vectored_handler);
}

static LONG CALLBACK test_instrumentation_callback_handler( EXCEPTION_POINTERS *exception_info )
{
    EXCEPTION_RECORD *rec = exception_info->ExceptionRecord;
    CONTEXT *c = exception_info->ContextRecord;

    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) ++c->Rip;
    return EXCEPTION_CONTINUE_EXECUTION;
}

static HANDLE instrumentation_callback_thread_ready, instrumentation_callback_thread_wait;

static DWORD WINAPI test_instrumentation_callback_thread( void *arg )
{
    SetEvent( instrumentation_callback_thread_ready );
    NtWaitForSingleObject( instrumentation_callback_thread_wait, FALSE, NULL );

    SetEvent( instrumentation_callback_thread_ready );
    NtWaitForSingleObject( instrumentation_callback_thread_wait, FALSE, NULL );
    return 0;
}

struct instrumentation_callback_data
{
    unsigned int call_count;
    struct
    {
        char *r10;
        char *rcx;
    }
    call_data[256];
};

static void init_instrumentation_data(struct instrumentation_callback_data *d)
{
    memset( d, 0xcc, sizeof(*d) );
    d->call_count = 0;
}

static void test_instrumentation_callback(void)
{
    static const BYTE instrumentation_callback[] =
    {
        0x50, 0x52,                         /* push %rax, %rdx */

        0x48, 0xba,                         /* movabs instrumentation_call_count, %rdx */
                /* &instrumentation_call_count, offset 4 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xb8, 0x01, 0x00, 0x00, 0x00,       /* mov $0x1,%eax */
        0xf0, 0x0f, 0xc1, 0x02,             /* lock xadd %eax,(%rdx) */
        0x0f, 0xb6, 0xc0,                   /* movzx %al,%eax */
        0x48, 0xba,                         /* movabs instrumentation_call_data, %rdx */
                /* instrumentation_call_data, offset 26 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x48, 0x01, 0xc0,                   /* add %rax,%rax */
        0x48, 0x8d, 0x14, 0xc2,             /* lea (%rdx,%rax,8),%rdx */
        0x4c, 0x89, 0x12,                   /* mov %r10,(%rdx) */
        0x48, 0x89, 0x4a, 0x08,             /* mov %rcx,0x8(%rdx) */

        0x5a, 0x58,                         /* pop %rdx, %rax */
        0x41, 0xff, 0xe2,                   /* jmp *r10 */
    };

    struct instrumentation_callback_data curr_data, data;
    PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION info;
    HMODULE ntdll = GetModuleHandleA( "ntdll.dll" );
    void *pLdrInitializeThunk;
    EXCEPTION_RECORD record;
    void *vectored_handler;
    unsigned int i, count;
    NTSTATUS status;
    HANDLE thread;
    CONTEXT ctx;
    HWND hwnd;
    LONG pass;

    if (is_arm64ec) return;

    memcpy( code_mem, instrumentation_callback, sizeof(instrumentation_callback) );
    *(void **)((char *)code_mem + 4) = &curr_data.call_count;
    *(void **)((char *)code_mem + 26) = curr_data.call_data;

    memset(&info, 0, sizeof(info));
    info.Callback = code_mem;
    init_instrumentation_data( &curr_data );
    status = NtSetInformationProcess( GetCurrentProcess(), ProcessInstrumentationCallback, &info, sizeof(info) );
    data = curr_data;
    ok( status == STATUS_SUCCESS || status == STATUS_INFO_LENGTH_MISMATCH
            || broken( status == STATUS_PRIVILEGE_NOT_HELD ) /* some versions and machines before Win10 */,
            "got %#lx.\n", status );
    /* If instrumentation callback is not yet set during syscall entry it won't be called on exit. */
    ok( !data.call_count, "got %u.\n", data.call_count );
    if (status)
    {
        win_skip( "Failed setting instrumenation callback.\n" );
        return;
    }

    init_instrumentation_data( &curr_data );
    memset( &info, 0xcc, sizeof(info) );
    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessInstrumentationCallback, &info, sizeof(info), NULL );
    data = curr_data;
    ok( status == STATUS_INVALID_INFO_CLASS, "got %#lx.\n", status );
    ok( data.call_count == 1, "got %u.\n", data.call_count );
    ok( data.call_data[0].r10 >= (char *)NtQueryInformationProcess
        && data.call_data[0].r10 < (char *)NtQueryInformationProcess + 0x20,
        "got %p, NtQueryInformationProcess %p.\n", data.call_data[0].r10, NtQueryInformationProcess );
    ok( data.call_data[0].rcx != data.call_data[0].r10, "got %p.\n", data.call_data[0].rcx );

    memset(&info, 0, sizeof(info));
    info.Callback = code_mem;
    init_instrumentation_data( &curr_data );
    status = NtSetInformationProcess( GetCurrentProcess(), ProcessInstrumentationCallback, &info, sizeof(info) );
    data = curr_data;
    ok( status == STATUS_SUCCESS, "got %#lx.\n", status );
    ok( data.call_count == 1, "got %u.\n", data.call_count );

    vectored_handler = AddVectoredExceptionHandler( TRUE, test_instrumentation_callback_handler );
    ok( !!vectored_handler, "failed.\n" );
    init_instrumentation_data( &curr_data );
    DbgBreakPoint();
    data = curr_data;
    ok( data.call_count == 1 || broken( data.call_count == 2 ) /* before Win10 1809 */, "got %u.\n", data.call_count );
    ok( data.call_data[0].r10 == pKiUserExceptionDispatcher, "got %p, expected %p.\n", data.call_data[0].r10,
        pKiUserExceptionDispatcher );

    pass = 0;
    InterlockedIncrement( &pass );
    pRtlCaptureContext( &ctx );
    if (InterlockedIncrement( &pass ) == 2) /* interlocked to prevent compiler from moving before capture */
    {
        record.ExceptionCode = 0xceadbeef;
        record.NumberParameters = 0;
        init_instrumentation_data( &curr_data );
        status = pNtRaiseException( &record, &ctx, TRUE );
        ok( 0, "Shouldn't be reached.\n" );
    }
    else if (pass == 3)
    {
        data = curr_data;
        ok( data.call_count == 1 || broken( data.call_count == 2 ) /* before Win10 1809 */, "got %u.\n", data.call_count );
        ok( data.call_data[0].r10 == pKiUserExceptionDispatcher, "got %p, expected %p.\n", data.call_data[0].r10,
            pKiUserExceptionDispatcher );
        init_instrumentation_data( &curr_data );
        NtContinue( &ctx, FALSE );
        ok( 0, "Shouldn't be reached.\n" );
    }
    else if (pass == 4)
    {
        data = curr_data;
        /* Not called for NtContinue. */
        ok( !data.call_count, "got %u.\n", data.call_count );
        init_instrumentation_data( &curr_data );
        NtSetContextThread( GetCurrentThread(), &ctx );
        ok( 0, "Shouldn't be reached.\n" );
    }
    else if (pass == 5)
    {
        data = curr_data;
        ok( data.call_count == 1, "got %u.\n", data.call_count );
        ok( data.call_data[0].r10 == (void *)ctx.Rip, "got %p, expected %p.\n", data.call_data[0].r10, (void *)ctx.Rip );
        init_instrumentation_data( &curr_data );
    }
    ok( pass == 5, "got %ld.\n", pass );
    RemoveVectoredExceptionHandler( vectored_handler );

    apc_count = 0;
    status = pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234, 0x5678, 0xdeadbeef );
    ok( !status, "got %#lx.\n", status );
    init_instrumentation_data( &curr_data );
    SleepEx( 0, TRUE );
    data = curr_data;
    ok( apc_count == 1, "APC was not called.\n" );
    ok( data.call_count == 1, "got %u.\n", data.call_count );
    ok( data.call_data[0].r10 == pKiUserApcDispatcher, "got %p, expected %p.\n", data.call_data[0].r10, pKiUserApcDispatcher );

    instrumentation_callback_thread_ready = CreateEventW( NULL, FALSE, FALSE, NULL );
    instrumentation_callback_thread_wait = CreateEventW( NULL, FALSE, FALSE, NULL );
    init_instrumentation_data( &curr_data );
    thread = CreateThread( NULL, 0, test_instrumentation_callback_thread, 0, 0, NULL );
    NtWaitForSingleObject( instrumentation_callback_thread_ready, FALSE, NULL );
    data = curr_data;
    ok( data.call_count && data.call_count <= 256, "got %u.\n", data.call_count );
    pLdrInitializeThunk = GetProcAddress( ntdll, "LdrInitializeThunk" );
    for (i = 0; i < data.call_count; ++i)
    {
        if (data.call_data[i].r10 == pLdrInitializeThunk) break;
    }
    ok( i < data.call_count, "LdrInitializeThunk not found.\n" );

    init_instrumentation_data( &curr_data );
    SetEvent( instrumentation_callback_thread_wait );
    NtWaitForSingleObject( instrumentation_callback_thread_ready, FALSE, NULL );
    data = curr_data;
    ok( data.call_count && data.call_count <= 256, "got %u.\n", data.call_count );
    count = 0;
    for (i = 0; i < data.call_count; ++i)
    {
        if (data.call_data[i].r10 >= (char *)NtWaitForSingleObject && data.call_data[i].r10 < (char *)NtWaitForSingleObject + 0x20)
            ++count;
    }
    ok( count == 2, "got %u.\n", count );

    SetEvent( instrumentation_callback_thread_wait );
    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );
    CloseHandle( instrumentation_callback_thread_ready );
    CloseHandle( instrumentation_callback_thread_wait );

    hwnd = CreateWindowA( "Static", "test", 0, 0, 0, 0, 0, 0, 0, 0, 0 );
    init_instrumentation_data( &curr_data );
    DestroyWindow( hwnd );
    data = curr_data;
    ok( data.call_count && data.call_count <= 256, "got %u.\n", data.call_count );
    for (i = 0; i < data.call_count; ++i)
    {
        if (data.call_data[i].r10 == pKiUserCallbackDispatcher)
            break;
    }
    ok( i < data.call_count, "KiUserCallbackDispatcher not found.\n" );

    init_instrumentation_data( &curr_data );
    memset(&info, 0, sizeof(info));
    status = NtSetInformationProcess( GetCurrentProcess(), ProcessInstrumentationCallback, &info, sizeof(info) );
    data = curr_data;
    ok( !status, "got %#lx.\n", status );
    ok( !data.call_count, "got %u.\n", data.call_count );
}

static UINT32 find_syscall_nr(const char *function)
{
    UINT32 syscall_nr;

    char *code = (char *)GetProcAddress(hntdll, function);

    /* This assumes that Nt* syscall thunks are all formatted as:
     *
     * 4c 8b d1                      movq    %rcx, %r10
     * b8 ?? ?? ?? ??                movl    $(syscall number), %eax
     */
    memcpy(&syscall_nr, code + 4, sizeof(UINT32));
    return syscall_nr;
}

static void test_direct_syscalls(void)
{
    static const BYTE code[] =
    {
        0x49, 0x89, 0xd2,           /* movq %rdx, %r10 */
        0x4c, 0x89, 0xc2,           /* movq %r8,  %rdx */
        0x89, 0xc8,                 /* movl %ecx, %eax */
        0x0f, 0x05,                 /* syscall */
        0xc3,                       /* ret */
    };

    HANDLE event;
    NTSTATUS (WINAPI *func)(UINT32 syscall_nr, HANDLE h, LONG *prev_state);

    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    memcpy(code_mem, code, sizeof(code));
    func = code_mem;
    func(find_syscall_nr("NtSetEvent"), event, NULL);

    todo_wine
    ok(WaitForSingleObject(event, 0) == WAIT_OBJECT_0, "Event not signaled.\n");
    CloseHandle(event);
}

#elif defined(__arm__)

static void test_thread_context(void)
{
    CONTEXT context;
    NTSTATUS status;
    struct expected
    {
        DWORD R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, Sp, Lr, Pc, Cpsr;
    } expect;
    NTSTATUS (*func_ptr)( void *arg1, void *arg2, struct expected *res, void *func );

    static const WORD call_func[] =
    {
        0xb502,            /* push    {r1, lr} */
        0xe882, 0x1fff,    /* stmia.w r2, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl, fp, ip} */
        0xf8c2, 0xd034,    /* str.w   sp, [r2, #52] */
        0xf8c2, 0xe038,    /* str.w   lr, [r2, #56] */
        0xf3ef, 0x8100,    /* mrs     r1, CPSR */
        0xf041, 0x0120,    /* orr.w   r1, r1, #32 */
        0x6411,            /* str     r1, [r2, #64] */
        0x9900,            /* ldr     r1, [sp, #0] */
        0x4679,            /* mov     r1, pc */
        0xf101, 0x0109,    /* add.w   r1, r1, #9 */
        0x63d1,            /* str     r1, [r2, #60] */
        0x9900,            /* ldr     r1, [sp, #0] */
        0x4798,            /* blx     r3 */
        0xbd02,            /* pop     {r1, pc} */
    };

    memcpy( code_mem, call_func, sizeof(call_func) );
    func_ptr = (void *)((char *)code_mem + 1);  /* thumb */

#define COMPARE(reg) \
    ok( context.reg == expect.reg, "wrong " #reg " %08lx/%08lx\n", context.reg, expect.reg )

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    func_ptr( &context, 0, &expect, pRtlCaptureContext );
    trace( "expect: r0=%08lx r1=%08lx r2=%08lx r3=%08lx r4=%08lx r5=%08lx r6=%08lx r7=%08lx r8=%08lx r9=%08lx "
           "r10=%08lx r11=%08lx r12=%08lx sp=%08lx lr=%08lx pc=%08lx cpsr=%08lx\n",
           expect.R0, expect.R1, expect.R2, expect.R3, expect.R4, expect.R5, expect.R6, expect.R7,
           expect.R8, expect.R9, expect.R10, expect.R11, expect.R12, expect.Sp, expect.Lr, expect.Pc, expect.Cpsr );
    trace( "actual: r0=%08lx r1=%08lx r2=%08lx r3=%08lx r4=%08lx r5=%08lx r6=%08lx r7=%08lx r8=%08lx r9=%08lx "
           "r10=%08lx r11=%08lx r12=%08lx sp=%08lx lr=%08lx pc=%08lx cpsr=%08lx\n",
           context.R0, context.R1, context.R2, context.R3, context.R4, context.R5, context.R6, context.R7,
           context.R8, context.R9, context.R10, context.R11, context.R12, context.Sp, context.Lr, context.Pc, context.Cpsr );

    ok( context.ContextFlags == (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT),
        "wrong flags %08lx\n", context.ContextFlags );
    ok( !context.R0, "wrong R0 %08lx\n", context.R0 );
    COMPARE( R1 );
    COMPARE( R2 );
    COMPARE( R3 );
    COMPARE( R4 );
    COMPARE( R5 );
    COMPARE( R6 );
    COMPARE( R7 );
    COMPARE( R8 );
    COMPARE( R9 );
    COMPARE( R10 );
    COMPARE( R11 );
    COMPARE( R12 );
    COMPARE( Sp );
    COMPARE( Pc );
    COMPARE( Cpsr );
    ok( !context.Lr, "wrong Lr %08lx\n", context.Lr );

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    context.ContextFlags = CONTEXT_FULL;

    status = func_ptr( GetCurrentThread(), &context, &expect, pNtGetContextThread );
    ok( status == STATUS_SUCCESS, "NtGetContextThread failed %08lx\n", status );
    trace( "expect: r0=%08lx r1=%08lx r2=%08lx r3=%08lx r4=%08lx r5=%08lx r6=%08lx r7=%08lx r8=%08lx r9=%08lx "
           "r10=%08lx r11=%08lx r12=%08lx sp=%08lx lr=%08lx pc=%08lx cpsr=%08lx\n",
           expect.R0, expect.R1, expect.R2, expect.R3, expect.R4, expect.R5, expect.R6, expect.R7,
           expect.R8, expect.R9, expect.R10, expect.R11, expect.R12, expect.Sp, expect.Lr, expect.Pc, expect.Cpsr );
    trace( "actual: r0=%08lx r1=%08lx r2=%08lx r3=%08lx r4=%08lx r5=%08lx r6=%08lx r7=%08lx r8=%08lx r9=%08lx "
           "r10=%08lx r11=%08lx r12=%08lx sp=%08lx lr=%08lx pc=%08lx cpsr=%08lx\n",
           context.R0, context.R1, context.R2, context.R3, context.R4, context.R5, context.R6, context.R7,
           context.R8, context.R9, context.R10, context.R11, context.R12, context.Sp, context.Lr, context.Pc, context.Cpsr );
    /* other registers are not preserved */
    COMPARE( R4 );
    COMPARE( R5 );
    COMPARE( R6 );
    COMPARE( R7 );
    COMPARE( R8 );
    COMPARE( R9 );
    COMPARE( R10 );
    COMPARE( R11 );
    ok( (context.Cpsr & 0xff0f0030) == (expect.Cpsr & 0xff0f0030),
        "wrong Cpsr %08lx/%08lx\n", context.Cpsr, expect.Cpsr );
    ok( context.Sp == expect.Sp - 16,
        "wrong Sp %08lx/%08lx\n", context.Sp, expect.Sp - 16 );
    /* Pc is somewhere close to the NtGetContextThread implementation */
    ok( (char *)context.Pc >= (char *)pNtGetContextThread &&
        (char *)context.Pc <= (char *)pNtGetContextThread + 0x10,
        "wrong Pc %08lx/%08lx\n", context.Pc, (DWORD)pNtGetContextThread );
#undef COMPARE
}

static void test_debugger(DWORD cont_status, BOOL with_WaitForDebugEventEx)
{
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    DEBUG_EVENT de;
    DWORD continuestatus;
    PVOID code_mem_address = NULL;
    NTSTATUS status;
    SIZE_T size_read;
    BOOL ret;
    int counter = 0;
    si.cb = sizeof(si);

    if(!pNtGetContextThread || !pNtSetContextThread || !pNtReadVirtualMemory || !pNtTerminateProcess)
    {
        skip("NtGetContextThread, NtSetContextThread, NtReadVirtualMemory or NtTerminateProcess not found\n");
        return;
    }

    if (with_WaitForDebugEventEx && !pWaitForDebugEventEx)
    {
        skip("WaitForDebugEventEx not found, skipping unicode strings in OutputDebugStringW\n");
        return;
    }

    sprintf(cmdline, "%s %s %s %p", my_argv[0], my_argv[1], "debuggee", &test_stage);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "could not create child process error: %lu\n", GetLastError());
    if (!ret)
        return;

    do
    {
        continuestatus = cont_status;
        ret = with_WaitForDebugEventEx ? pWaitForDebugEventEx(&de, INFINITE) : WaitForDebugEvent(&de, INFINITE);
        ok(ret, "reading debug event\n");

        ret = ContinueDebugEvent(de.dwProcessId, de.dwThreadId, 0xdeadbeef);
        ok(!ret, "ContinueDebugEvent unexpectedly succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "Unexpected last error: %lu\n", GetLastError());

        if (de.dwThreadId != pi.dwThreadId)
        {
            trace("event %ld not coming from main thread, ignoring\n", de.dwDebugEventCode);
            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, cont_status);
            continue;
        }

        if (de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            if(de.u.CreateProcessInfo.lpBaseOfImage != NtCurrentTeb()->Peb->ImageBaseAddress)
            {
                skip("child process loaded at different address, terminating it\n");
                pNtTerminateProcess(pi.hProcess, 0);
            }
        }
        else if (de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            CONTEXT ctx;
            enum debugger_stages stage;

            counter++;
            status = pNtReadVirtualMemory(pi.hProcess, &code_mem, &code_mem_address,
                                          sizeof(code_mem_address), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);
            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            ctx.ContextFlags = CONTEXT_FULL;
            status = pNtGetContextThread(pi.hThread, &ctx);
            ok(!status, "NtGetContextThread failed with 0x%lx\n", status);

            trace("exception 0x%lx at %p firstchance=%ld pc=%08lx, r0=%08lx\n",
                  de.u.Exception.ExceptionRecord.ExceptionCode,
                  de.u.Exception.ExceptionRecord.ExceptionAddress,
                  de.u.Exception.dwFirstChance, ctx.Pc, ctx.R0);

            if (counter > 100)
            {
                ok(FALSE, "got way too many exceptions, probably caught in an infinite loop, terminating child\n");
                pNtTerminateProcess(pi.hProcess, 1);
            }
            else if (counter < 2) /* startup breakpoint */
            {
                /* breakpoint is inside ntdll */
                void *ntdll = GetModuleHandleA( "ntdll.dll" );
                IMAGE_NT_HEADERS *nt = RtlImageNtHeader( ntdll );

                ok( (char *)ctx.Pc >= (char *)ntdll &&
                    (char *)ctx.Pc < (char *)ntdll + nt->OptionalHeader.SizeOfImage,
                    "wrong pc %p ntdll %p-%p\n", (void *)ctx.Pc, ntdll,
                    (char *)ntdll + nt->OptionalHeader.SizeOfImage );
            }
            else
            {
                if (stage == STAGE_RTLRAISE_NOT_HANDLED)
                {
                    ok((char *)ctx.Pc == (char *)code_mem_address + 7, "Pc at %lx instead of %p\n",
                       ctx.Pc, (char *)code_mem_address + 7);
                    /* setting the context from debugger does not affect the context that the
                     * exception handler gets, except on w2008 */
                    ctx.Pc = (UINT_PTR)code_mem_address + 9;
                    ctx.R0 = 0xf00f00f1;
                    /* let the debuggee handle the exception */
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == STAGE_RTLRAISE_HANDLE_LAST_CHANCE)
                {
                    if (de.u.Exception.dwFirstChance)
                    {
                        /* debugger gets first chance exception with unmodified ctx.Pc */
                        ok((char *)ctx.Pc == (char *)code_mem_address + 7, "Pc at 0x%lx instead of %p\n",
                           ctx.Pc, (char *)code_mem_address + 7);
                        ctx.Pc = (UINT_PTR)code_mem_address + 9;
                        ctx.R0 = 0xf00f00f1;
                        /* pass exception to debuggee
                         * exception will not be handled and a second chance exception will be raised */
                        continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    }
                    else
                    {
                        /* debugger gets context after exception handler has played with it */
                        /* ctx.Pc is the same value the exception handler got */
                        if (de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                        {
                            ok((char *)ctx.Pc == (char *)code_mem_address + 7,
                               "Pc at 0x%lx instead of %p\n", ctx.Pc, (char *)code_mem_address + 7);
                            /* need to fixup Pc for debuggee */
                            ctx.Pc += 2;
                        }
                        else ok((char *)ctx.Pc == (char *)code_mem_address + 7,
                                "Pc at 0x%lx instead of %p\n", ctx.Pc, (char *)code_mem_address + 7);
                        /* here we handle exception */
                    }
                }
                else if (stage == STAGE_SERVICE_CONTINUE || stage == STAGE_SERVICE_NOT_HANDLED)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08lx\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Pc == (char *)code_mem_address + 0x1d,
                       "expected Pc = %p, got 0x%lx\n", (char *)code_mem_address + 0x1d, ctx.Pc);
                    if (stage == STAGE_SERVICE_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == STAGE_BREAKPOINT_CONTINUE || stage == STAGE_BREAKPOINT_NOT_HANDLED)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08lx\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Pc == (char *)code_mem_address + 3,
                       "expected Pc = %p, got 0x%lx\n", (char *)code_mem_address + 3, ctx.Pc);
                    if (stage == STAGE_BREAKPOINT_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == STAGE_EXCEPTION_INVHANDLE_CONTINUE || stage == STAGE_EXCEPTION_INVHANDLE_NOT_HANDLED)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_INVALID_HANDLE,
                       "unexpected exception code %08lx, expected %08lx\n", de.u.Exception.ExceptionRecord.ExceptionCode,
                       EXCEPTION_INVALID_HANDLE);
                    ok(de.u.Exception.ExceptionRecord.NumberParameters == 0,
                       "unexpected number of parameters %ld, expected 0\n", de.u.Exception.ExceptionRecord.NumberParameters);

                    if (stage == STAGE_EXCEPTION_INVHANDLE_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == STAGE_NO_EXCEPTION_INVHANDLE_NOT_HANDLED)
                {
                    ok(FALSE || broken(TRUE) /* < Win10 */, "should not throw exception\n");
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else
                    ok(FALSE, "unexpected stage %x\n", stage);

                status = pNtSetContextThread(pi.hThread, &ctx);
                ok(!status, "NtSetContextThread failed with 0x%lx\n", status);
            }
        }
        else if (de.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
        {
            enum debugger_stages stage;
            char buffer[64 * sizeof(WCHAR)];
            unsigned char_size = de.u.DebugString.fUnicode ? sizeof(WCHAR) : sizeof(char);

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

           if (de.u.DebugString.fUnicode)
                ok(with_WaitForDebugEventEx &&
                   (stage == STAGE_OUTPUTDEBUGSTRINGW_CONTINUE || stage == STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED),
                   "unexpected unicode debug string event\n");
            else
                ok(!with_WaitForDebugEventEx || stage != STAGE_OUTPUTDEBUGSTRINGW_CONTINUE || cont_status != DBG_CONTINUE,
                   "unexpected ansi debug string event %u %s %lx\n",
                   stage, with_WaitForDebugEventEx ? "with" : "without", cont_status);

            ok(de.u.DebugString.nDebugStringLength < sizeof(buffer) / char_size - 1,
               "buffer not large enough to hold %d bytes\n", de.u.DebugString.nDebugStringLength);

            memset(buffer, 0, sizeof(buffer));
            status = pNtReadVirtualMemory(pi.hProcess, de.u.DebugString.lpDebugStringData, buffer,
                                          de.u.DebugString.nDebugStringLength * char_size, &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            if (stage == STAGE_OUTPUTDEBUGSTRINGA_CONTINUE || stage == STAGE_OUTPUTDEBUGSTRINGA_NOT_HANDLED ||
                stage == STAGE_OUTPUTDEBUGSTRINGW_CONTINUE || stage == STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED)
            {
                if (de.u.DebugString.fUnicode)
                    ok(!wcscmp((WCHAR*)buffer, L"Hello World"), "got unexpected debug string '%ls'\n", (WCHAR*)buffer);
                else
                    ok(!strcmp(buffer, "Hello World"), "got unexpected debug string '%s'\n", buffer);
            }
            else /* ignore unrelated debug strings like 'SHIMVIEW: ShimInfo(Complete)' */
                ok(strstr(buffer, "SHIMVIEW") != NULL, "unexpected stage %x, got debug string event '%s'\n", stage, buffer);

            if (stage == STAGE_OUTPUTDEBUGSTRINGA_NOT_HANDLED || stage == STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED)
                continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }
        else if (de.dwDebugEventCode == RIP_EVENT)
        {
            enum debugger_stages stage;

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            if (stage == STAGE_RIPEVENT_CONTINUE || stage == STAGE_RIPEVENT_NOT_HANDLED)
            {
                ok(de.u.RipInfo.dwError == 0x11223344, "got unexpected rip error code %08lx, expected %08x\n",
                   de.u.RipInfo.dwError, 0x11223344);
                ok(de.u.RipInfo.dwType  == 0x55667788, "got unexpected rip type %08lx, expected %08x\n",
                   de.u.RipInfo.dwType, 0x55667788);
            }
            else
                ok(FALSE, "unexpected stage %x\n", stage);

            if (stage == STAGE_RIPEVENT_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continuestatus);

    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    wait_child_process( pi.hProcess );
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %lu\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "error %lu\n", GetLastError());
}

static void test_debug_service(DWORD numexc)
{
    /* not supported */
}


static BOOL hook_called;
static BOOL got_exception;
static void *code_ptr;

static WORD patched_code[] =
{
    0x4668,         /* mov r0, sp */
    0xf8df, 0xc004, /* ldr.w ip, [pc, #0x4] */
    0x4760,         /* bx ip */
    0, 0,           /* 1: hook_trampoline */
};
static WORD saved_code[ARRAY_SIZE(patched_code)];

static LONG WINAPI dbg_except_continue_vectored_handler(struct _EXCEPTION_POINTERS *ptrs)
{
    EXCEPTION_RECORD *rec = ptrs->ExceptionRecord;
    CONTEXT *context = ptrs->ContextRecord;

    trace("dbg_except_continue_vectored_handler, code %#lx, pc %#lx.\n", rec->ExceptionCode, context->Pc);
    got_exception = TRUE;

    ok(rec->ExceptionCode == 0x80000003, "Got unexpected exception code %#lx.\n", rec->ExceptionCode);
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void * WINAPI hook_KiUserExceptionDispatcher(void *stack)
{
    CONTEXT *context = stack;
    EXCEPTION_RECORD *rec = (EXCEPTION_RECORD *)(context + 1);

    trace( "rec %p context %p pc %#lx sp %#lx flags %#lx\n",
           rec, context, context->Pc, context->Sp, context->ContextFlags );

    ok( !((ULONG_PTR)stack & 7), "unaligned stack %p\n", stack );
    ok( rec->ExceptionCode == 0x80000003, "Got unexpected ExceptionCode %#lx.\n", rec->ExceptionCode );

    hook_called = TRUE;
    memcpy(code_ptr, saved_code, sizeof(saved_code));
    FlushInstructionCache( GetCurrentProcess(), code_ptr, sizeof(saved_code));
    return pKiUserExceptionDispatcher;
}

static void test_KiUserExceptionDispatcher(void)
{
    WORD hook_trampoline[] =
    {
        0x4668,         /* mov r0, sp */
        0xf8df, 0xc006, /* ldr.w r12, [pc, #0x6] */
        0x47e0,         /* blx r12 */
        0x4700,         /* bx r0 */
        0, 0,           /* 1: hook_KiUserExceptionDispatcher */
    };

    EXCEPTION_RECORD record = { EXCEPTION_BREAKPOINT };
    void *trampoline_ptr, *vectored_handler;
    DWORD old_protect;
    BOOL ret;

    code_ptr = (void *)(((ULONG_PTR)pKiUserExceptionDispatcher) & ~1); /* mask thumb bit */
    *(void **)&hook_trampoline[5] = hook_KiUserExceptionDispatcher;
    trampoline_ptr = (char *)code_mem + 1024;
    memcpy( trampoline_ptr, hook_trampoline, sizeof(hook_trampoline));

    ret = VirtualProtect( code_ptr, sizeof(saved_code),
                          PAGE_EXECUTE_READWRITE, &old_protect );
    ok( ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError() );

    memcpy( saved_code, code_ptr, sizeof(saved_code) );
    *(void **)&patched_code[4] = (char *)trampoline_ptr + 1;  /* thumb */

    vectored_handler = AddVectoredExceptionHandler(TRUE, dbg_except_continue_vectored_handler);

    memcpy( code_ptr, patched_code, sizeof(patched_code) );
    FlushInstructionCache( GetCurrentProcess(), code_ptr, sizeof(patched_code));

    got_exception = FALSE;
    hook_called = FALSE;

    pRtlRaiseException(&record);

    ok(got_exception, "Handler was not called.\n");
    ok(!hook_called, "Hook was called.\n");

    memcpy( code_ptr, patched_code, sizeof(patched_code) );
    FlushInstructionCache( GetCurrentProcess(), code_ptr, sizeof(patched_code));

    got_exception = 0;
    hook_called = FALSE;
    NtCurrentTeb()->Peb->BeingDebugged = 1;

    pRtlRaiseException(&record);

    ok(got_exception, "Handler was not called.\n");
    ok(hook_called, "Hook was not called.\n");
    NtCurrentTeb()->Peb->BeingDebugged = 0;

    RemoveVectoredExceptionHandler(vectored_handler);
    VirtualProtect(code_ptr, sizeof(saved_code), old_protect, &old_protect);
}

static UINT alertable_supported;

static void * WINAPI hook_KiUserApcDispatcher(void *stack)
{
    struct
    {
        void *func;
        ULONG args[3];
        ULONG alertable;
        ULONG align;
        CONTEXT context;
    } *args = stack;
    CONTEXT *context = &args->context;

    if (args->alertable == 1) alertable_supported = TRUE;
    else context = (CONTEXT *)&args->alertable;

    trace( "stack=%p func=%p args=%lx,%lx,%lx alertable=%lx context=%p pc=%lx sp=%lx (%lx)\n",
           args, args->func, args->args[0], args->args[1], args->args[2],
           args->alertable, context, context->Pc, context->Sp,
           context->Sp - (ULONG_PTR)stack );

    ok( args->func == apc_func, "wrong func %p / %p\n", args->func, apc_func );
    ok( args->args[0] == 0x1234 + apc_count, "wrong arg1 %lx\n", args->args[0] );
    ok( args->args[1] == 0x5678, "wrong arg2 %lx\n", args->args[1] );
    ok( args->args[2] == 0xdeadbeef, "wrong arg3 %lx\n", args->args[2] );

    if (apc_count && alertable_supported) args->alertable = FALSE;
    pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234 + apc_count + 1, 0x5678, 0xdeadbeef );

    hook_called = TRUE;
    memcpy( code_ptr, saved_code, sizeof(saved_code));
    FlushInstructionCache( GetCurrentProcess(), code_ptr, sizeof(saved_code));
    return pKiUserApcDispatcher;
}

static void test_KiUserApcDispatcher(void)
{
    WORD hook_trampoline[] =
    {
        0x4668,         /* mov r0, sp */
        0xf8df, 0xc006, /* ldr.w r12, [pc, #0x6] */
        0x47e0,         /* blx r12 */
        0x4700,         /* bx r0 */
        0, 0,           /* 1: hook_KiUserApcDispatcher */
    };
    DWORD old_protect;
    BOOL ret;

    code_ptr = (void *)(((ULONG_PTR)pKiUserApcDispatcher) & ~1); /* mask thumb bit */
    *(void **)&hook_trampoline[5] = hook_KiUserApcDispatcher;
    memcpy(code_mem, hook_trampoline, sizeof(hook_trampoline));

    ret = VirtualProtect( code_ptr, sizeof(saved_code),
                          PAGE_EXECUTE_READWRITE, &old_protect );
    ok( ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError() );

    memcpy( saved_code, code_ptr, sizeof(saved_code) );
    *(void **)&patched_code[4] = (char *)code_mem + 1; /* thumb */
    memcpy( code_ptr, patched_code, sizeof(patched_code) );
    FlushInstructionCache( GetCurrentProcess(), code_ptr, sizeof(patched_code));

    hook_called = FALSE;
    apc_count = 0;
    pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234, 0x5678, 0xdeadbeef );
    SleepEx( 0, TRUE );
    ok( apc_count == 2, "APC count %u\n", apc_count );
    ok( hook_called, "hook was not called\n" );

    memcpy( code_ptr, patched_code, sizeof(patched_code) );
    FlushInstructionCache( GetCurrentProcess(), code_ptr, sizeof(patched_code));
    pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234 + apc_count, 0x5678, 0xdeadbeef );
    SleepEx( 0, TRUE );
    if (alertable_supported)
    {
        ok( apc_count == 3, "APC count %u\n", apc_count );
        SleepEx( 0, TRUE );
    }
    ok( apc_count == 4, "APC count %u\n", apc_count );

    VirtualProtect( code_ptr, sizeof(saved_code), old_protect, &old_protect );
}

static void WINAPI hook_KiUserCallbackDispatcher(void *sp)
{
    struct
    {
        void *args;
        ULONG len;
        ULONG id;
        ULONG lr;
        ULONG sp;
        ULONG pc;
        BYTE args_data[0];
    } *stack = sp;
    ULONG_PTR redzone = (BYTE *)stack->sp - &stack->args_data[stack->len];
    KERNEL_CALLBACK_PROC func = NtCurrentTeb()->Peb->KernelCallbackTable[stack->id];

    trace( "stack=%p len=%lx id=%lx lr=%lx sp=%lx pc=%lx\n",
           stack, stack->len, stack->id, stack->lr, stack->sp, stack->pc );
    NtCallbackReturn( NULL, 0, 0 );

    ok( stack->args == stack->args_data, "wrong args %p / %p\n", stack->args, stack->args_data );
    ok( redzone >= 8 && redzone <= 16, "wrong sp %p / %p (%Iu)\n",
        (void *)stack->sp, stack->args_data, redzone );

    if (pRtlPcToFileHeader)
    {
        void *mod, *win32u = GetModuleHandleA("win32u.dll");

        pRtlPcToFileHeader( (void *)stack->pc, &mod );
        ok( mod == win32u, "pc %lx not in win32u %p\n", stack->pc, win32u );
    }
    NtCallbackReturn( NULL, 0, func( stack->args, stack->len ));
}

static void test_KiUserCallbackDispatcher(void)
{
    DWORD old_protect;
    BOOL ret;

    code_ptr = (void *)(((ULONG_PTR)pKiUserCallbackDispatcher) & ~1); /* mask thumb bit */
    ret = VirtualProtect( code_ptr, sizeof(saved_code),
                          PAGE_EXECUTE_READWRITE, &old_protect );
    ok( ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError() );

    memcpy( saved_code, code_ptr, sizeof(saved_code));
    *(void **)&patched_code[4] = hook_KiUserCallbackDispatcher;
    memcpy( code_ptr, patched_code, sizeof(patched_code));
    FlushInstructionCache(GetCurrentProcess(), code_ptr, sizeof(patched_code));

    DestroyWindow( CreateWindowA( "Static", "test", 0, 0, 0, 0, 0, 0, 0, 0, 0 ));

    memcpy( code_ptr, saved_code, sizeof(saved_code));
    FlushInstructionCache(GetCurrentProcess(), code_ptr, sizeof(saved_code));
    VirtualProtect( code_ptr, sizeof(saved_code), old_protect, &old_protect );
}

struct unwind_info
{
    DWORD function_length : 18;
    DWORD version : 2;
    DWORD x : 1;
    DWORD e : 1;
    DWORD f : 1;
    DWORD epilog : 5;
    DWORD codes : 4;
};

static void run_exception_test(void *handler, const void* context,
                               const void *code, unsigned int code_size,
                               unsigned int func2_offset, DWORD access, DWORD handler_flags,
                               void *arg1, void *arg2)
{
    DWORD buf[11];
    RUNTIME_FUNCTION runtime_func[2];
    struct unwind_info unwind;
    void (*func)(void*, void*) = (void *)((char *)code_mem + 1); /* thumb */
    DWORD oldaccess, oldaccess2;

    runtime_func[0].BeginAddress = 0;
    runtime_func[0].UnwindData = 0x1000;
    runtime_func[1].BeginAddress = func2_offset;
    runtime_func[1].UnwindData = 0x1010;

    unwind.function_length = func2_offset / 2;
    unwind.version = 0;
    unwind.x = 1;
    unwind.e = 1;
    unwind.f = 0;
    unwind.epilog = 1;
    unwind.codes = 1;
    buf[0] = *(DWORD *)&unwind;
    buf[1] = 0xfbfbffd4; /* push {r4, lr}; end; nop; nop */
    buf[2] = 0x1021;
    *(const void **)&buf[3] = context;
    unwind.function_length = (code_size - func2_offset) / 2;
    buf[4] = *(DWORD *)&unwind;
    buf[5] = 0xfbfbffd4; /* push {r4, lr}; end; nop; nop */
    buf[6] = 0x1021;
    *(const void **)&buf[7] = context;
    buf[8] = 0xc004f8df; /* ldr ip, 1f */
    buf[9] = 0xbf004760; /* bx ip; nop */
    *(const void **)&buf[10] = handler;

    memcpy((unsigned char *)code_mem + 0x1000, buf, sizeof(buf));
    memcpy(code_mem, code, code_size);
    if (access) VirtualProtect(code_mem, code_size, access, &oldaccess);
    FlushInstructionCache( GetCurrentProcess(), code_mem, 0x2000 );

    pRtlAddFunctionTable(runtime_func, ARRAY_SIZE(runtime_func), (ULONG_PTR)code_mem);
    func( arg1, arg2 );
    pRtlDeleteFunctionTable(runtime_func);

    if (access) VirtualProtect(code_mem, code_size, oldaccess, &oldaccess2);
}

static BOOL got_nested_exception, got_prev_frame_exception;
static void *nested_exception_initial_frame;

static DWORD nested_exception_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                      CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    trace("nested_exception_handler pc %p, sp %p, code %#lx, flags %#lx, ExceptionAddress %p.\n",
          (void *)context->Pc, (void *)context->Sp, rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress);

    if (rec->ExceptionCode == 0x80000003 && !(rec->ExceptionFlags & EXCEPTION_NESTED_CALL))
    {
        ok(rec->NumberParameters == 1, "Got unexpected rec->NumberParameters %lu.\n", rec->NumberParameters);
        ok((char *)context->Sp == (char *)frame - 8, "Got unexpected frame %p / %p.\n", frame, (void *)context->Sp);
        ok((char *)context->Lr == (char *)code_mem + 0x07, "Got unexpected lr %p.\n", (void *)context->Lr);
        ok((char *)context->Pc == (char *)code_mem + 0x0b, "Got unexpected pc %p.\n", (void *)context->Pc);

        nested_exception_initial_frame = frame;
        RaiseException(0xdeadbeef, 0, 0, 0);
        context->Pc += 2;
        return ExceptionContinueExecution;
    }

    if (rec->ExceptionCode == 0xdeadbeef &&
        (rec->ExceptionFlags == EXCEPTION_NESTED_CALL ||
         rec->ExceptionFlags == (EXCEPTION_NESTED_CALL | EXCEPTION_SOFTWARE_ORIGINATE)))
    {
        ok(!rec->NumberParameters, "Got unexpected rec->NumberParameters %lu.\n", rec->NumberParameters);
        got_nested_exception = TRUE;
        ok(frame == nested_exception_initial_frame, "Got unexpected frame %p / %p.\n",
           frame, nested_exception_initial_frame);
        return ExceptionContinueSearch;
    }

    ok(rec->ExceptionCode == 0xdeadbeef && (!rec->ExceptionFlags || rec->ExceptionFlags == EXCEPTION_SOFTWARE_ORIGINATE),
       "Got unexpected exception code %#lx, flags %#lx.\n", rec->ExceptionCode, rec->ExceptionFlags);
    ok(!rec->NumberParameters, "Got unexpected rec->NumberParameters %lu.\n", rec->NumberParameters);
    ok((char *)frame == (char *)nested_exception_initial_frame + 8, "Got unexpected frame %p / %p.\n",
        frame, nested_exception_initial_frame);
    got_prev_frame_exception = TRUE;
    return ExceptionContinueExecution;
}

static const WORD nested_except_code[] =
{
    0xb510,         /* 00: push {r4, lr} */
    0xf000, 0xf801, /* 02: bl 1f */
    0xbd10,         /* 06: pop {r4, pc} */
    0xb510,         /* 08: 1: push {r4, lr} */
    0xdefe,         /* 0a: trap */
    0xbf00,         /* 0c: nop */
    0xbd10,         /* 0e: pop {r4, pc} */
};

static void test_nested_exception(void)
{
    got_nested_exception = got_prev_frame_exception = FALSE;
    run_exception_test(nested_exception_handler, NULL, nested_except_code, sizeof(nested_except_code),
                       4 * sizeof(WORD), PAGE_EXECUTE_READ, UNW_FLAG_EHANDLER, 0, 0);
    ok(got_nested_exception, "Did not get nested exception.\n");
    ok(got_prev_frame_exception, "Did not get nested exception in the previous frame.\n");
}

static unsigned int collided_unwind_exception_count;

static DWORD collided_exception_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    CONTEXT ctx;

    trace("collided_exception_handler pc %p, sp %p, code %#lx, flags %#lx, ExceptionAddress %p, frame %p.\n",
          (void *)context->Pc, (void *)context->Sp, rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, frame);

    switch(collided_unwind_exception_count++)
    {
        case 0:
            /* Initial exception from nested_except_code. */
            ok(rec->ExceptionCode == STATUS_BREAKPOINT, "got %#lx.\n", rec->ExceptionCode);
            nested_exception_initial_frame = frame;
            /* Start unwind. */
            pRtlUnwindEx((char *)frame + 8, (char *)code_mem + 0x07, NULL, NULL, &ctx, NULL);
            ok(0, "shouldn't be reached\n");
            break;
        case 1:
            ok(rec->ExceptionCode == STATUS_UNWIND, "got %#lx.\n", rec->ExceptionCode);
            ok(rec->ExceptionFlags == EXCEPTION_UNWINDING, "got %#lx.\n", rec->ExceptionFlags);
            ok((char *)context->Pc == (char *)code_mem + 0x0b, "got %p.\n", (void *)context->Pc);
            /* generate exception in unwind handler. */
            RaiseException(0xdeadbeef, 0, 0, 0);
            ok(0, "shouldn't be reached\n");
            break;
        case 2:
            /* Inner call frame, continue search. */
            ok(rec->ExceptionCode == 0xdeadbeef, "got %#lx.\n", rec->ExceptionCode);
            ok(!rec->ExceptionFlags || rec->ExceptionFlags == EXCEPTION_SOFTWARE_ORIGINATE, "got %#lx.\n", rec->ExceptionFlags);
            ok(frame == nested_exception_initial_frame, "got %p, expected %p.\n", frame, nested_exception_initial_frame);
            break;
        case 3:
            /* Top level call frame, handle exception by unwinding. */
            ok(rec->ExceptionCode == 0xdeadbeef, "got %#lx.\n", rec->ExceptionCode);
            ok(!rec->ExceptionFlags || rec->ExceptionFlags == EXCEPTION_SOFTWARE_ORIGINATE, "got %#lx.\n", rec->ExceptionFlags);
            ok((char *)frame == (char *)nested_exception_initial_frame + 8, "got %p, expected %p.\n", frame, nested_exception_initial_frame);
            pRtlUnwindEx((char *)nested_exception_initial_frame + 8, (char *)code_mem + 0x07, NULL, NULL, &ctx, NULL);
            ok(0, "shouldn't be reached\n");
            break;
        case 4:
            /* Collided unwind. */
            ok(rec->ExceptionCode == STATUS_UNWIND, "got %#lx.\n", rec->ExceptionCode);
            ok(rec->ExceptionFlags == (EXCEPTION_UNWINDING | EXCEPTION_COLLIDED_UNWIND), "got %#lx.\n", rec->ExceptionFlags);
            ok(frame == nested_exception_initial_frame, "got %p, expected %p.\n", frame, nested_exception_initial_frame);
            break;
        case 5:
            /* EXCEPTION_COLLIDED_UNWIND cleared for the following frames. */
            ok(rec->ExceptionCode == STATUS_UNWIND, "got %#lx.\n", rec->ExceptionCode);
            ok(rec->ExceptionFlags == (EXCEPTION_UNWINDING | EXCEPTION_TARGET_UNWIND), "got %#lx.\n", rec->ExceptionFlags);
            ok((char *)frame == (char *)nested_exception_initial_frame + 8, "got %p, expected %p.\n", frame, nested_exception_initial_frame);
            break;
    }
    return ExceptionContinueSearch;
}

static void test_collided_unwind(void)
{
    got_nested_exception = got_prev_frame_exception = FALSE;
    collided_unwind_exception_count = 0;
    run_exception_test(collided_exception_handler, NULL, nested_except_code, sizeof(nested_except_code),
                       4 * sizeof(WORD), PAGE_EXECUTE_READ, UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER, 0, 0);
    ok(collided_unwind_exception_count == 6, "got %u.\n", collided_unwind_exception_count);
}


static int rtlraiseexception_unhandled_handler_called;
static int rtlraiseexception_teb_handler_called;
static int rtlraiseexception_handler_called;

static void rtlraiseexception_handler_( EXCEPTION_RECORD *rec, void *frame, CONTEXT *context,
                                        void *dispatcher, BOOL unhandled_handler )
{
    void *addr = rec->ExceptionAddress;

    trace( "exception: %08lx flags:%lx addr:%p context: Pc:%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, (void *)context->Pc );

    ok( addr == (char *)code_mem + 7,
        "ExceptionAddress at %p instead of %p\n", addr, (char *)code_mem + 7 );
    ok( context->ContextFlags == (CONTEXT_FULL | CONTEXT_ARM_FLOATING_POINT | CONTEXT_UNWOUND_TO_CALL) ||
        context->ContextFlags == CONTEXT_ALL,
        "wrong context flags %lx\n", context->ContextFlags );
    ok( context->Pc == (UINT_PTR)addr,
        "%d: Pc at %lx instead of %Ix\n", test_stage, context->Pc, (UINT_PTR)addr );

    ok( context->R0 == 0xf00f00f0, "context->X0 is %lx, should have been set to 0xf00f00f0 in vectored handler\n", context->R0 );
}

static LONG CALLBACK rtlraiseexception_unhandled_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT context = ExceptionInfo->ContextRecord;
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;

    trace( "exception: %08lx flags:%lx addr:%p context: Pc:%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, (void *)context->Pc );
    rtlraiseexception_unhandled_handler_called = 1;
    rtlraiseexception_handler_(rec, NULL, context, NULL, TRUE);
    if (test_stage == STAGE_RTLRAISE_HANDLE_LAST_CHANCE) return EXCEPTION_CONTINUE_SEARCH;

    return EXCEPTION_CONTINUE_EXECUTION;
}

static DWORD WINAPI rtlraiseexception_teb_handler( EXCEPTION_RECORD *rec,
                                                   EXCEPTION_REGISTRATION_RECORD *frame,
                                                   CONTEXT *context,
                                                   EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    rtlraiseexception_teb_handler_called = 1;
    rtlraiseexception_handler_(rec, frame, context, dispatcher, FALSE);
    return ExceptionContinueSearch;
}

static DWORD WINAPI rtlraiseexception_handler( EXCEPTION_RECORD *rec, void *frame,
                                               CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    ULONG *nonvol_regs = (void *)dispatcher->NonVolatileRegisters;
    int i;

    for (i = 0; i < 8; i++)
        ok( nonvol_regs[i] == ((ULONG *)&context->R4)[i],
            "wrong non volatile reg r%u %lx / %lx\n", i + 4,
            nonvol_regs[i], ((ULONG *)&context->R4)[i] );
    for (i = 0; i < 8; i++)
        ok( ((ULONGLONG *)(nonvol_regs + 8))[i] == context->D[i + 8],
            "wrong non volatile reg d%u %I64x / %I64x\n", i + 8,
            ((ULONGLONG *)(nonvol_regs + 8))[i], context->D[i + 8] );

    rtlraiseexception_handler_called = 1;
    rtlraiseexception_handler_(rec, frame, context, dispatcher, FALSE);
    return ExceptionContinueSearch;
}

static LONG CALLBACK rtlraiseexception_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT context = ExceptionInfo->ContextRecord;
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    void *addr = rec->ExceptionAddress;

    trace( "exception: %08lx flags:%lx addr:%p context: Pc:%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, (void *)context->Pc );
    ok( addr == (char *)code_mem + 7,
        "ExceptionAddress at %p instead of %p\n", addr, (char *)code_mem + 7 );
    ok( context->Pc == (UINT_PTR)addr,
        "%d: Pc at %lx instead of %Ix\n", test_stage, context->Pc, (UINT_PTR)addr );

    context->R0 = 0xf00f00f0;
    return EXCEPTION_CONTINUE_SEARCH;
}

static const DWORD call_one_arg_code[] =
{
    0xb510,  /* 00: push {r4, lr} */
    0x4788,  /* 02: blx r1 */
    0xbf00,  /* 04: nop */
    0xbd10,  /* 06: pop {r4, pc} */
};

static void run_rtlraiseexception_test(DWORD exceptioncode)
{
    EXCEPTION_REGISTRATION_RECORD frame;
    EXCEPTION_RECORD record;
    PVOID vectored_handler = NULL;

    record.ExceptionCode = exceptioncode;
    record.ExceptionFlags = 0;
    record.ExceptionRecord = NULL;
    record.ExceptionAddress = NULL; /* does not matter, copied return address */
    record.NumberParameters = 0;

    frame.Handler = rtlraiseexception_teb_handler;
    frame.Prev = NtCurrentTeb()->Tib.ExceptionList;

    NtCurrentTeb()->Tib.ExceptionList = &frame;
    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, rtlraiseexception_vectored_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");
    if (pRtlSetUnhandledExceptionFilter) pRtlSetUnhandledExceptionFilter(rtlraiseexception_unhandled_handler);

    rtlraiseexception_handler_called = 0;
    rtlraiseexception_teb_handler_called = 0;
    rtlraiseexception_unhandled_handler_called = 0;

    run_exception_test( rtlraiseexception_handler, NULL, call_one_arg_code,
                        sizeof(call_one_arg_code), sizeof(call_one_arg_code),
                        PAGE_EXECUTE_READ, UNW_FLAG_EHANDLER,
                        &record, pRtlRaiseException);

    ok( record.ExceptionAddress == (char *)code_mem + 7,
        "address set to %p instead of %p\n", record.ExceptionAddress, (char *)code_mem + 7 );

    todo_wine
    ok( !rtlraiseexception_teb_handler_called, "Frame TEB handler called\n" );
    ok( rtlraiseexception_handler_called, "Frame handler called\n" );
    ok( rtlraiseexception_unhandled_handler_called, "UnhandledExceptionFilter wasn't called\n" );

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
    if (pRtlSetUnhandledExceptionFilter) pRtlSetUnhandledExceptionFilter(NULL);
    NtCurrentTeb()->Tib.ExceptionList = frame.Prev;
}

static void test_rtlraiseexception(void)
{
    run_rtlraiseexception_test(0x12345);
    run_rtlraiseexception_test(EXCEPTION_BREAKPOINT);
    run_rtlraiseexception_test(EXCEPTION_INVALID_HANDLE);
}


static LONG consolidate_dummy_called;
static LONG pass;

static const WORD call_rtlunwind[] =
{
    0xf8dd, 0xc00c,  /* ldr r12, [sp, #0xc] */
    0xe8ac, 0x0ff0,  /* stm r12!, {r4-r11} */
    0xec8c, 0x8b10,  /* vstm r12, {d8-d15} */
    0xf8dd, 0xc008,  /* ldr r12, [sp, #0x8] */
    0x4760,          /* bx r12 */
};

static PVOID CALLBACK test_consolidate_dummy(EXCEPTION_RECORD *rec)
{
    CONTEXT *ctx = (CONTEXT *)rec->ExceptionInformation[1];
    DWORD *saved_regs = (DWORD *)rec->ExceptionInformation[3];
    DWORD *regs = (DWORD *)rec->ExceptionInformation[10];
    int i;

    switch (InterlockedIncrement(&consolidate_dummy_called))
    {
    case 1:  /* RtlRestoreContext */
        ok(ctx->Pc == 0xdeadbeef, "RtlRestoreContext wrong Pc, expected: 0xdeadbeef, got: %lx\n", ctx->Pc);
        ok( rec->ExceptionInformation[10] == -1, "wrong info %Ix\n", rec->ExceptionInformation[10] );
        break;
    case 2: /* RtlUnwindEx */
        ok(ctx->Pc != 0xdeadbeef, "RtlUnwindEx wrong Pc, got: %lx\n", ctx->Pc );
        ok( rec->ExceptionInformation[10] != -1, "wrong info %Ix\n", rec->ExceptionInformation[10] );
        for (i = 0; i < 8; i++)
            ok( saved_regs[i] == regs[i], "wrong reg R%u, expected: %lx, got: %lx\n",
                i + 4, saved_regs[i], regs[i] );
        regs += 8;
        saved_regs += 8;
        for (i = 0; i < 8; i++)
            ok( ((DWORD64 *)saved_regs)[i] == ((DWORD64 *)regs)[i],
                "wrong reg D%u, expected: %I64x, got: %I64x\n",
                i + 8, ((DWORD64 *)saved_regs)[i], ((DWORD64 *)regs)[i] );
        break;
    }
    return (PVOID)rec->ExceptionInformation[2];
}

static void test_restore_context(void)
{
    EXCEPTION_RECORD rec;
    _JUMP_BUFFER buf;
    CONTEXT ctx;
    int i;

    if (!pRtlUnwindEx || !pRtlRestoreContext || !pRtlCaptureContext)
    {
        skip("RtlUnwindEx/RtlCaptureContext/RtlRestoreContext not found\n");
        return;
    }

    /* test simple case of capture and restore context */
    pass = 0;
    InterlockedIncrement(&pass); /* interlocked to prevent compiler from moving after capture */
    pRtlCaptureContext(&ctx);
    if (InterlockedIncrement(&pass) == 2) /* interlocked to prevent compiler from moving before capture */
    {
        pRtlRestoreContext(&ctx, NULL);
        ok(0, "shouldn't be reached\n");
    }
    else
        ok(pass < 4, "unexpected pass %ld\n", pass);

    /* test with jmp using RtlRestoreContext */
    pass = 0;
    InterlockedIncrement(&pass);
    RtlCaptureContext(&ctx);
    InterlockedIncrement(&pass); /* only called once */
    setjmp((_JBTYPE *)&buf);
    InterlockedIncrement(&pass);
    if (pass == 3)
    {
        rec.ExceptionCode = STATUS_LONGJUMP;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD)&buf;
        /* uses buf.Pc instead of ctx.Pc */
        pRtlRestoreContext(&ctx, &rec);
        ok(0, "shouldn't be reached\n");
    }
    else if (pass == 4)
    {
        ok(buf.R4  == ctx.R4 , "longjmp failed for R4, expected: %lx, got: %lx\n",  buf.R4,  ctx.R4 );
        ok(buf.R5  == ctx.R5 , "longjmp failed for R5, expected: %lx, got: %lx\n",  buf.R5,  ctx.R5 );
        ok(buf.R6  == ctx.R6 , "longjmp failed for R6, expected: %lx, got: %lx\n",  buf.R6,  ctx.R6 );
        ok(buf.R7  == ctx.R7 , "longjmp failed for R7, expected: %lx, got: %lx\n",  buf.R7,  ctx.R7 );
        ok(buf.R8  == ctx.R8 , "longjmp failed for R8, expected: %lx, got: %lx\n",  buf.R8,  ctx.R8 );
        ok(buf.R9  == ctx.R9 , "longjmp failed for R9, expected: %lx, got: %lx\n",  buf.R9,  ctx.R9 );
        ok(buf.R10 == ctx.R10, "longjmp failed for R10, expected: %lx, got: %lx\n", buf.R10, ctx.R10 );
        ok(buf.R11 == ctx.R11, "longjmp failed for R11, expected: %lx, got: %lx\n", buf.R11, ctx.R11 );
        for (i = 0; i < 8; i++)
            ok(buf.D[i] == ctx.D[i + 8], "longjmp failed for D%u, expected: %I64x, got: %I64x\n",
               i + 8, buf.D[i], ctx.D[i + 8]);
        pRtlRestoreContext(&ctx, &rec);
        ok(0, "shouldn't be reached\n");
    }
    else
        ok(pass == 5, "unexpected pass %ld\n", pass);

    /* test with jmp through RtlUnwindEx */
    pass = 0;
    InterlockedIncrement(&pass);
    pRtlCaptureContext(&ctx);
    InterlockedIncrement(&pass); /* only called once */
    setjmp((_JBTYPE *)&buf);
    InterlockedIncrement(&pass);
    if (pass == 3)
    {
        rec.ExceptionCode = STATUS_LONGJUMP;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD)&buf;

        /* uses buf.Pc instead of bogus 0xdeadbeef */
        pRtlUnwindEx((void*)buf.Sp, (void*)0xdeadbeef, &rec, NULL, &ctx, NULL);
        ok(0, "shouldn't be reached\n");
    }
    else
        ok(pass == 4, "unexpected pass %ld\n", pass);


    /* test with consolidate */
    pass = 0;
    InterlockedIncrement(&pass);
    RtlCaptureContext(&ctx);
    InterlockedIncrement(&pass);
    if (pass == 2)
    {
        rec.ExceptionCode = STATUS_UNWIND_CONSOLIDATE;
        rec.NumberParameters = 3;
        rec.ExceptionInformation[0] = (DWORD)test_consolidate_dummy;
        rec.ExceptionInformation[1] = (DWORD)&ctx;
        rec.ExceptionInformation[2] = ctx.Pc;
        rec.ExceptionInformation[10] = -1;
        ctx.Pc = 0xdeadbeef;

        pRtlRestoreContext(&ctx, &rec);
        ok(0, "shouldn't be reached\n");
    }
    else if (pass == 3)
        ok(consolidate_dummy_called == 1, "test_consolidate_dummy not called\n");
    else
        ok(0, "unexpected pass %ld\n", pass);

    /* test with consolidate through RtlUnwindEx */
    pass = 0;
    InterlockedIncrement(&pass);
    pRtlCaptureContext(&ctx);
    InterlockedIncrement(&pass);
    if (pass == 2)
    {
        void (*func)(DWORD,DWORD,EXCEPTION_RECORD*,DWORD,CONTEXT*,void*,void*,void*);
        DWORD64 nonvol_regs[12];

        func = (void *)((ULONG_PTR)code_mem | 1); /* thumb */
        rec.ExceptionCode = STATUS_UNWIND_CONSOLIDATE;
        rec.NumberParameters = 4;
        rec.ExceptionInformation[0] = (DWORD)test_consolidate_dummy;
        rec.ExceptionInformation[1] = (DWORD)&ctx;
        rec.ExceptionInformation[2] = ctx.Pc;
        rec.ExceptionInformation[3] = (DWORD)&nonvol_regs;
        rec.ExceptionInformation[10] = -1;  /* otherwise it doesn't get set */
        ctx.Pc = 0xdeadbeef;
        /* uses consolidate callback Pc instead of bogus 0xdeadbeef */
        memcpy( code_mem, call_rtlunwind, sizeof(call_rtlunwind) );
        FlushInstructionCache( GetCurrentProcess(), code_mem, sizeof(call_rtlunwind) );
        func( buf.Frame, 0xdeadbeef, &rec, 0, &ctx, NULL, pRtlUnwindEx, nonvol_regs );
        ok(0, "shouldn't be reached\n");
    }
    else if (pass == 3)
        ok(consolidate_dummy_called == 2, "test_consolidate_dummy not called\n");
    else
        ok(0, "unexpected pass %ld\n", pass);
}

#elif defined(__aarch64__)

static void test_thread_context(void)
{
    CONTEXT context;
    NTSTATUS status;
    struct expected
    {
        ULONG64 X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11, X12, X13, X14, X15, X16,
            X17, X18, X19, X20, X21, X22, X23, X24, X25, X26, X27, X28, Fp, Lr, Sp, Pc;
        ULONG Cpsr, Fpcr, Fpsr;
    } expect;
    NTSTATUS (*func_ptr)( void *arg1, void *arg2, struct expected *res, void *func ) = code_mem;

    static const DWORD call_func[] =
    {
        0xa9bf7bfd,  /* stp     x29, x30, [sp, #-16]! */
        0xa9000440,  /* stp     x0, x1, [x2] */
        0xa9010c42,  /* stp     x2, x3, [x2, #16] */
        0xa9021444,  /* stp     x4, x5, [x2, #32] */
        0xa9031c46,  /* stp     x6, x7, [x2, #48] */
        0xa9042448,  /* stp     x8, x9, [x2, #64] */
        0xa9052c4a,  /* stp     x10, x11, [x2, #80] */
        0xa906344c,  /* stp     x12, x13, [x2, #96] */
        0xa9073c4e,  /* stp     x14, x15, [x2, #112] */
        0xa9084450,  /* stp     x16, x17, [x2, #128] */
        0xa9094c52,  /* stp     x18, x19, [x2, #144] */
        0xa90a5454,  /* stp     x20, x21, [x2, #160] */
        0xa90b5c56,  /* stp     x22, x23, [x2, #176] */
        0xa90c6458,  /* stp     x24, x25, [x2, #192] */
        0xa90d6c5a,  /* stp     x26, x27, [x2, #208] */
        0xa90e745c,  /* stp     x28, x29, [x2, #224] */
        0xf900785e,  /* str     x30, [x2, #240] */
        0x910003e1,  /* mov     x1, sp */
        0xf9007c41,  /* str     x1, [x2, #248] */
        0x90000001,  /* adrp    x1, 1f */
        0x9101e021,  /* add     x1, x1, #:lo12:1f */
        0xf9008041,  /* str     x1, [x2, #256] */
        0xd53b4201,  /* mrs     x1, nzcv */
        0xb9010841,  /* str     w1, [x2, #264] */
        0xd53b4401,  /* mrs     x1, fpcr */
        0xb9010c41,  /* str     w1, [x2, #268] */
        0xd53b4421,  /* mrs     x1, fpsr */
        0xb9011041,  /* str     w1, [x2, #272] */
        0xf9400441,  /* ldr     x1, [x2, #8] */
        0xd63f0060,  /* blr     x3 */
        0xa8c17bfd,  /* 1: ldp     x29, x30, [sp], #16 */
        0xd65f03c0,  /* ret */
     };

    memcpy( func_ptr, call_func, sizeof(call_func) );

#define COMPARE(reg) \
    ok( context.reg == expect.reg, "wrong " #reg " %p/%p\n", (void *)(ULONG64)context.reg, (void *)(ULONG64)expect.reg )

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    func_ptr( &context, 0, &expect, pRtlCaptureContext );
    trace( "expect: x0=%p x1=%p x2=%p x3=%p x4=%p x5=%p x6=%p x7=%p x8=%p x9=%p x10=%p x11=%p x12=%p x13=%p x14=%p x15=%p x16=%p x17=%p x18=%p x19=%p x20=%p x21=%p x22=%p x23=%p x24=%p x25=%p x26=%p x27=%p x28=%p fp=%p lr=%p sp=%p pc=%p cpsr=%08lx\n",
           (void *)expect.X0, (void *)expect.X1, (void *)expect.X2, (void *)expect.X3,
           (void *)expect.X4, (void *)expect.X5, (void *)expect.X6, (void *)expect.X7,
           (void *)expect.X8, (void *)expect.X9, (void *)expect.X10, (void *)expect.X11,
           (void *)expect.X12, (void *)expect.X13, (void *)expect.X14, (void *)expect.X15,
           (void *)expect.X16, (void *)expect.X17, (void *)expect.X18, (void *)expect.X19,
           (void *)expect.X20, (void *)expect.X21, (void *)expect.X22, (void *)expect.X23,
           (void *)expect.X24, (void *)expect.X25, (void *)expect.X26, (void *)expect.X27,
           (void *)expect.X28, (void *)expect.Fp, (void *)expect.Lr, (void *)expect.Sp,
           (void *)expect.Pc, expect.Cpsr );
    trace( "actual: x0=%p x1=%p x2=%p x3=%p x4=%p x5=%p x6=%p x7=%p x8=%p x9=%p x10=%p x11=%p x12=%p x13=%p x14=%p x15=%p x16=%p x17=%p x18=%p x19=%p x20=%p x21=%p x22=%p x23=%p x24=%p x25=%p x26=%p x27=%p x28=%p fp=%p lr=%p sp=%p pc=%p cpsr=%08lx\n",
           (void *)context.X0, (void *)context.X1, (void *)context.X2, (void *)context.X3,
           (void *)context.X4, (void *)context.X5, (void *)context.X6, (void *)context.X7,
           (void *)context.X8, (void *)context.X9, (void *)context.X10, (void *)context.X11,
           (void *)context.X12, (void *)context.X13, (void *)context.X14, (void *)context.X15,
           (void *)context.X16, (void *)context.X17, (void *)context.X18, (void *)context.X19,
           (void *)context.X20, (void *)context.X21, (void *)context.X22, (void *)context.X23,
           (void *)context.X24, (void *)context.X25, (void *)context.X26, (void *)context.X27,
           (void *)context.X28, (void *)context.Fp, (void *)context.Lr, (void *)context.Sp,
           (void *)context.Pc, context.Cpsr );

    ok( context.ContextFlags == CONTEXT_FULL,
        "wrong flags %08lx\n", context.ContextFlags );
    ok( !context.X0, "wrong X0 %p\n", (void *)context.X0 );
    COMPARE( X1 );
    COMPARE( X2 );
    COMPARE( X3 );
    COMPARE( X4 );
    COMPARE( X5 );
    COMPARE( X6 );
    COMPARE( X7 );
    COMPARE( X8 );
    COMPARE( X9 );
    COMPARE( X10 );
    COMPARE( X11 );
    COMPARE( X12 );
    COMPARE( X13 );
    COMPARE( X14 );
    COMPARE( X15 );
    COMPARE( X16 );
    COMPARE( X17 );
    COMPARE( X18 );
    COMPARE( X19 );
    COMPARE( X20 );
    COMPARE( X21 );
    COMPARE( X22 );
    COMPARE( X23 );
    COMPARE( X24 );
    COMPARE( X25 );
    COMPARE( X26 );
    COMPARE( X27 );
    COMPARE( X28 );
    COMPARE( Fp );
    COMPARE( Sp );
    COMPARE( Pc );
    COMPARE( Cpsr );
    COMPARE( Fpcr );
    COMPARE( Fpsr );
    ok( !context.Lr, "wrong Lr %p\n", (void *)context.Lr );

    memset( &context, 0xcc, sizeof(context) );
    memset( &expect, 0xcc, sizeof(expect) );
    context.ContextFlags = CONTEXT_FULL;

    status = func_ptr( GetCurrentThread(), &context, &expect, pNtGetContextThread );
    ok( status == STATUS_SUCCESS, "NtGetContextThread failed %08lx\n", status );
    trace( "expect: x0=%p x1=%p x2=%p x3=%p x4=%p x5=%p x6=%p x7=%p x8=%p x9=%p x10=%p x11=%p x12=%p x13=%p x14=%p x15=%p x16=%p x17=%p x18=%p x19=%p x20=%p x21=%p x22=%p x23=%p x24=%p x25=%p x26=%p x27=%p x28=%p fp=%p lr=%p sp=%p pc=%p cpsr=%08lx\n",
           (void *)expect.X0, (void *)expect.X1, (void *)expect.X2, (void *)expect.X3,
           (void *)expect.X4, (void *)expect.X5, (void *)expect.X6, (void *)expect.X7,
           (void *)expect.X8, (void *)expect.X9, (void *)expect.X10, (void *)expect.X11,
           (void *)expect.X12, (void *)expect.X13, (void *)expect.X14, (void *)expect.X15,
           (void *)expect.X16, (void *)expect.X17, (void *)expect.X18, (void *)expect.X19,
           (void *)expect.X20, (void *)expect.X21, (void *)expect.X22, (void *)expect.X23,
           (void *)expect.X24, (void *)expect.X25, (void *)expect.X26, (void *)expect.X27,
           (void *)expect.X28, (void *)expect.Fp, (void *)expect.Lr, (void *)expect.Sp,
           (void *)expect.Pc, expect.Cpsr );
    trace( "actual: x0=%p x1=%p x2=%p x3=%p x4=%p x5=%p x6=%p x7=%p x8=%p x9=%p x10=%p x11=%p x12=%p x13=%p x14=%p x15=%p x16=%p x17=%p x18=%p x19=%p x20=%p x21=%p x22=%p x23=%p x24=%p x25=%p x26=%p x27=%p x28=%p fp=%p lr=%p sp=%p pc=%p cpsr=%08lx\n",
           (void *)context.X0, (void *)context.X1, (void *)context.X2, (void *)context.X3,
           (void *)context.X4, (void *)context.X5, (void *)context.X6, (void *)context.X7,
           (void *)context.X8, (void *)context.X9, (void *)context.X10, (void *)context.X11,
           (void *)context.X12, (void *)context.X13, (void *)context.X14, (void *)context.X15,
           (void *)context.X16, (void *)context.X17, (void *)context.X18, (void *)context.X19,
           (void *)context.X20, (void *)context.X21, (void *)context.X22, (void *)context.X23,
           (void *)context.X24, (void *)context.X25, (void *)context.X26, (void *)context.X27,
           (void *)context.X28, (void *)context.Fp, (void *)context.Lr, (void *)context.Sp,
           (void *)context.Pc, context.Cpsr );
    /* other registers are not preserved */
    COMPARE( X19 );
    COMPARE( X20 );
    COMPARE( X21 );
    COMPARE( X22 );
    COMPARE( X23 );
    COMPARE( X24 );
    COMPARE( X25 );
    COMPARE( X26 );
    COMPARE( X27 );
    COMPARE( X28 );
    COMPARE( Fp );
    COMPARE( Fpcr );
    COMPARE( Fpsr );
    ok( context.Lr == expect.Pc, "wrong Lr %p/%p\n", (void *)context.Lr, (void *)expect.Pc );
    ok( context.Sp == expect.Sp, "wrong Sp %p/%p\n", (void *)context.Sp, (void *)expect.Sp );
    ok( (char *)context.Pc >= (char *)pNtGetContextThread &&
        (char *)context.Pc <= (char *)pNtGetContextThread + 32,
        "wrong Pc %p/%p\n", (void *)context.Pc, pNtGetContextThread );
#undef COMPARE
}

static void test_debugger(DWORD cont_status, BOOL with_WaitForDebugEventEx)
{
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    DEBUG_EVENT de;
    DWORD continuestatus;
    PVOID code_mem_address = NULL;
    NTSTATUS status;
    SIZE_T size_read;
    BOOL ret;
    int counter = 0;
    si.cb = sizeof(si);

    if(!pNtGetContextThread || !pNtSetContextThread || !pNtReadVirtualMemory || !pNtTerminateProcess)
    {
        skip("NtGetContextThread, NtSetContextThread, NtReadVirtualMemory or NtTerminateProcess not found\n");
        return;
    }

    if (with_WaitForDebugEventEx && !pWaitForDebugEventEx)
    {
        skip("WaitForDebugEventEx not found, skipping unicode strings in OutputDebugStringW\n");
        return;
    }

    sprintf(cmdline, "%s %s %s %p", my_argv[0], my_argv[1], "debuggee", &test_stage);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "could not create child process error: %lu\n", GetLastError());
    if (!ret)
        return;

    do
    {
        continuestatus = cont_status;
        ret = with_WaitForDebugEventEx ? pWaitForDebugEventEx(&de, INFINITE) : WaitForDebugEvent(&de, INFINITE);
        ok(ret, "reading debug event\n");

        ret = ContinueDebugEvent(de.dwProcessId, de.dwThreadId, 0xdeadbeef);
        ok(!ret, "ContinueDebugEvent unexpectedly succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "Unexpected last error: %lu\n", GetLastError());

        if (de.dwThreadId != pi.dwThreadId)
        {
            trace("event %ld not coming from main thread, ignoring\n", de.dwDebugEventCode);
            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, cont_status);
            continue;
        }

        if (de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            if(de.u.CreateProcessInfo.lpBaseOfImage != NtCurrentTeb()->Peb->ImageBaseAddress)
            {
                skip("child process loaded at different address, terminating it\n");
                pNtTerminateProcess(pi.hProcess, 0);
            }
        }
        else if (de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            CONTEXT ctx;
            enum debugger_stages stage;

            counter++;
            status = pNtReadVirtualMemory(pi.hProcess, &code_mem, &code_mem_address,
                                          sizeof(code_mem_address), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);
            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            ctx.ContextFlags = CONTEXT_FULL;
            status = pNtGetContextThread(pi.hThread, &ctx);
            ok(!status, "NtGetContextThread failed with 0x%lx\n", status);

            trace("exception 0x%lx at %p firstchance=%ld pc=%p, x0=%p\n",
                  de.u.Exception.ExceptionRecord.ExceptionCode,
                  de.u.Exception.ExceptionRecord.ExceptionAddress,
                  de.u.Exception.dwFirstChance, (char *)ctx.Pc, (char *)ctx.X0);

            if (counter > 100)
            {
                ok(FALSE, "got way too many exceptions, probably caught in an infinite loop, terminating child\n");
                pNtTerminateProcess(pi.hProcess, 1);
            }
            else if (counter < 2) /* startup breakpoint */
            {
                /* breakpoint is inside ntdll */
                void *ntdll = GetModuleHandleA( "ntdll.dll" );
                IMAGE_NT_HEADERS *nt = RtlImageNtHeader( ntdll );

                ok( (char *)ctx.Pc >= (char *)ntdll &&
                    (char *)ctx.Pc < (char *)ntdll + nt->OptionalHeader.SizeOfImage,
                    "wrong pc %p ntdll %p-%p\n", (void *)ctx.Pc, ntdll,
                    (char *)ntdll + nt->OptionalHeader.SizeOfImage );
            }
            else
            {
                if (stage == STAGE_RTLRAISE_NOT_HANDLED)
                {
                    ok((char *)ctx.Pc == (char *)code_mem_address + 0xc, "Pc at %p instead of %p\n",
                       (char *)ctx.Pc, (char *)code_mem_address + 0xc);
                    /* setting the context from debugger does not affect the context that the
                     * exception handler gets, except on w2008 */
                    ctx.Pc = (UINT_PTR)code_mem_address + 0x10;
                    ctx.X0 = 0xf00f00f1;
                    /* let the debuggee handle the exception */
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == STAGE_RTLRAISE_HANDLE_LAST_CHANCE)
                {
                    if (de.u.Exception.dwFirstChance)
                    {
                        /* debugger gets first chance exception with unmodified ctx.Pc */
                        ok((char *)ctx.Pc == (char *)code_mem_address + 0xc, "Pc at %p instead of %p\n",
                           (char *)ctx.Pc, (char *)code_mem_address + 0xc);
                        ctx.Pc = (UINT_PTR)code_mem_address + 0x10;
                        ctx.X0 = 0xf00f00f1;
                        /* pass exception to debuggee
                         * exception will not be handled and a second chance exception will be raised */
                        continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    }
                    else
                    {
                        /* debugger gets context after exception handler has played with it */
                        /* ctx.Pc is the same value the exception handler got */
                        if (de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                        {
                            ok((char *)ctx.Pc == (char *)code_mem_address + 0xc,
                               "Pc at %p instead of %p\n", (char *)ctx.Pc, (char *)code_mem_address + 0xc);
                            /* need to fixup Pc for debuggee */
                            ctx.Pc += 4;
                        }
                        else ok((char *)ctx.Pc == (char *)code_mem_address + 0xc,
                                "Pc at %p instead of %p\n", (void *)ctx.Pc, (char *)code_mem_address + 0xc);
                        /* here we handle exception */
                    }
                }
                else if (stage == STAGE_SERVICE_CONTINUE || stage == STAGE_SERVICE_NOT_HANDLED)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08lx\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Pc == (char *)code_mem_address + 0x1d,
                       "expected Pc = %p, got %p\n", (char *)code_mem_address + 0x1d, (char *)ctx.Pc);
                    if (stage == STAGE_SERVICE_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == STAGE_BREAKPOINT_CONTINUE || stage == STAGE_BREAKPOINT_NOT_HANDLED)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT,
                       "expected EXCEPTION_BREAKPOINT, got %08lx\n", de.u.Exception.ExceptionRecord.ExceptionCode);
                    ok((char *)ctx.Pc == (char *)code_mem_address + 4,
                       "expected Pc = %p, got %p\n", (char *)code_mem_address + 4, (char *)ctx.Pc);
                    if (stage == STAGE_BREAKPOINT_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == STAGE_EXCEPTION_INVHANDLE_CONTINUE || stage == STAGE_EXCEPTION_INVHANDLE_NOT_HANDLED)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_INVALID_HANDLE,
                       "unexpected exception code %08lx, expected %08lx\n", de.u.Exception.ExceptionRecord.ExceptionCode,
                       EXCEPTION_INVALID_HANDLE);
                    ok(de.u.Exception.ExceptionRecord.NumberParameters == 0,
                       "unexpected number of parameters %ld, expected 0\n", de.u.Exception.ExceptionRecord.NumberParameters);

                    if (stage == STAGE_EXCEPTION_INVHANDLE_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == STAGE_NO_EXCEPTION_INVHANDLE_NOT_HANDLED)
                {
                    ok(FALSE || broken(TRUE) /* < Win10 */, "should not throw exception\n");
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else
                    ok(FALSE, "unexpected stage %x\n", stage);

                status = pNtSetContextThread(pi.hThread, &ctx);
                ok(!status, "NtSetContextThread failed with 0x%lx\n", status);
            }
        }
        else if (de.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
        {
            enum debugger_stages stage;
            char buffer[128 * sizeof(WCHAR)];
            unsigned char_size = de.u.DebugString.fUnicode ? sizeof(WCHAR) : sizeof(char);

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            if (de.u.DebugString.fUnicode)
                ok(with_WaitForDebugEventEx &&
                   (stage == STAGE_OUTPUTDEBUGSTRINGW_CONTINUE || stage == STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED),
                   "unexpected unicode debug string event\n");
            else
                ok(!with_WaitForDebugEventEx || stage != STAGE_OUTPUTDEBUGSTRINGW_CONTINUE || cont_status != DBG_CONTINUE,
                   "unexpected ansi debug string event %u %s %lx\n",
                   stage, with_WaitForDebugEventEx ? "with" : "without", cont_status);

            ok(de.u.DebugString.nDebugStringLength < sizeof(buffer) / char_size - 1,
               "buffer not large enough to hold %d bytes\n", de.u.DebugString.nDebugStringLength);

            memset(buffer, 0, sizeof(buffer));
            status = pNtReadVirtualMemory(pi.hProcess, de.u.DebugString.lpDebugStringData, buffer,
                                          de.u.DebugString.nDebugStringLength * char_size, &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            if (stage == STAGE_OUTPUTDEBUGSTRINGA_CONTINUE || stage == STAGE_OUTPUTDEBUGSTRINGA_NOT_HANDLED ||
                stage == STAGE_OUTPUTDEBUGSTRINGW_CONTINUE || stage == STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED)
            {
                if (de.u.DebugString.fUnicode)
                    ok(!wcscmp((WCHAR*)buffer, L"Hello World"), "got unexpected debug string '%ls'\n", (WCHAR*)buffer);
                else
                    ok(!strcmp(buffer, "Hello World"), "got unexpected debug string '%s'\n", buffer);
            }
            else /* ignore unrelated debug strings like 'SHIMVIEW: ShimInfo(Complete)' */
                ok(strstr(buffer, "SHIMVIEW") || !strncmp(buffer, "RTL:", 4),
                     "unexpected stage %x, got debug string event '%s'\n", stage, buffer);

            if (stage == STAGE_OUTPUTDEBUGSTRINGA_NOT_HANDLED || stage == STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED)
                continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }
        else if (de.dwDebugEventCode == RIP_EVENT)
        {
            enum debugger_stages stage;

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%lx\n", status);

            if (stage == STAGE_RIPEVENT_CONTINUE || stage == STAGE_RIPEVENT_NOT_HANDLED)
            {
                ok(de.u.RipInfo.dwError == 0x11223344, "got unexpected rip error code %08lx, expected %08x\n",
                   de.u.RipInfo.dwError, 0x11223344);
                ok(de.u.RipInfo.dwType  == 0x55667788, "got unexpected rip type %08lx, expected %08x\n",
                   de.u.RipInfo.dwType, 0x55667788);
            }
            else
                ok(FALSE, "unexpected stage %x\n", stage);

            if (stage == STAGE_RIPEVENT_NOT_HANDLED) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continuestatus);

    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    wait_child_process( pi.hProcess );
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %lu\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "error %lu\n", GetLastError());
}

static void test_debug_service(DWORD numexc)
{
    /* not supported */
}

static void test_continue(void)
{
    struct context_pair {
        CONTEXT before;
        CONTEXT after;
    } contexts;
    KCONTINUE_ARGUMENT args = { .ContinueType = KCONTINUE_UNWIND };
    unsigned int i;
    NTSTATUS (*func_ptr)( struct context_pair *, void *arg, void *continue_func, void *capture_func ) = code_mem;

    static const DWORD call_func[] =
    {
        0xa9bd7bfd, /* stp x29, x30, [sp, #-0x30]! */
                    /* stash volatile registers before calling capture */
        0xa90107e0, /* stp x0, x1, [sp, #0x10] */
        0xa9020fe2, /* stp x2, x3, [sp, #0x20] */
        0xd63f0060, /* blr x3 * capture context from before NtContinue to contexts->before */
        0xa9420fe2, /* ldp x2, x3, [sp, #0x20] */
        0xa94107e0, /* ldp x0, x1, [sp, #0x10] */
                    /* overwrite the contents of x4...k28 with a dummy value */
        0xd297dde4, /* mov x4, #0xbeef */
        0xf2bbd5a4, /* movk x4, #0xdead, lsl #16 */
        0xaa048084, /* orr x4, x4, x4, lsl #32 */
        0xaa0403e5, /* mov x5,  x4 */
        0xaa0403e6, /* mov x6,  x4 */
        0xaa0403e7, /* mov x7,  x4 */
        0xaa0403e8, /* mov x8,  x4 */
        0xaa0403e9, /* mov x9,  x4 */
        0xaa0403ea, /* mov x10, x4 */
        0xaa0403eb, /* mov x11, x4 */
        0xaa0403ec, /* mov x12, x4 */
        0xaa0403ed, /* mov x13, x4 */
        0xaa0403ee, /* mov x14, x4 */
        0xaa0403ef, /* mov x15, x4 */
        0xaa0403f0, /* mov x16, x4 */
        0xaa0403f1, /* mov x17, x4 */
                    /* avoid overwriting the TEB in x18 */
        0xaa0403f3, /* mov x19, x4 */
        0xaa0403f4, /* mov x20, x4 */
        0xaa0403f5, /* mov x21, x4 */
        0xaa0403f6, /* mov x22, x4 */
        0xaa0403f7, /* mov x23, x4 */
        0xaa0403f8, /* mov x24, x4 */
        0xaa0403f9, /* mov x25, x4 */
        0xaa0403fa, /* mov x26, x4 */
        0xaa0403fb, /* mov x27, x4 */
        0xaa0403fc, /* mov x28, x4 */
                    /* overwrite the contents all vector registers a dummy value */
        0x4e080c80, /* dup v0.2d,  x4 */
        0x4ea01c01, /* mov v1.2d,  v0.2d */
        0x4ea01c02, /* mov v2.2d,  v0.2d */
        0x4ea01c03, /* mov v3.2d,  v0.2d */
        0x4ea01c04, /* mov v4.2d,  v0.2d */
        0x4ea01c05, /* mov v5.2d,  v0.2d */
        0x4ea01c06, /* mov v6.2d,  v0.2d */
        0x4ea01c07, /* mov v7.2d,  v0.2d */
        0x4ea01c08, /* mov v8.2d,  v0.2d */
        0x4ea01c09, /* mov v9.2d,  v0.2d */
        0x4ea01c0a, /* mov v10.2d, v0.2d */
        0x4ea01c0b, /* mov v11.2d, v0.2d */
        0x4ea01c0c, /* mov v12.2d, v0.2d */
        0x4ea01c0d, /* mov v13.2d, v0.2d */
        0x4ea01c0e, /* mov v14.2d, v0.2d */
        0x4ea01c0f, /* mov v15.2d, v0.2d */
        0x4ea01c10, /* mov v16.2d, v0.2d */
        0x4ea01c11, /* mov v17.2d, v0.2d */
        0x4ea01c12, /* mov v18.2d, v0.2d */
        0x4ea01c13, /* mov v19.2d, v0.2d */
        0x4ea01c14, /* mov v20.2d, v0.2d */
        0x4ea01c15, /* mov v21.2d, v0.2d */
        0x4ea01c16, /* mov v22.2d, v0.2d */
        0x4ea01c17, /* mov v23.2d, v0.2d */
        0x4ea01c18, /* mov v24.2d, v0.2d */
        0x4ea01c19, /* mov v25.2d, v0.2d */
        0x4ea01c1a, /* mov v26.2d, v0.2d */
        0x4ea01c1b, /* mov v27.2d, v0.2d */
        0x4ea01c1c, /* mov v28.2d, v0.2d */
        0x4ea01c1d, /* mov v29.2d, v0.2d */
        0x4ea01c1e, /* mov v30.2d, v0.2d */
        0x4ea01c1f, /* mov v31.2d, v0.2d */
        0xd51b441f, /* msr fpcr, xzr */
        0xd51b443f, /* msr fpsr, xzr */
                    /* setup the control context so execution continues from label 1 */
        0x10000064, /* adr x4, #0xc */
        0xf9008404, /* str x4, [x0, #0x108] */
        0xd63f0040, /* blr x2 * restore the captured integer and floating point context */
        0xf94017e3, /* 1: ldr x3, [sp, #0x28] */
        0xf9400be0, /* ldr x0, [sp, #0x10] */
        0x910e4000, /* add x0, x0, #0x390 * adjust contexts to point to contexts->after */
        0xd63f0060, /* blr x3 * capture context from after NtContinue to contexts->after */
        0xa8c37bfd, /* ldp x29, x30, [sp], #0x30 */
        0xd65f03c0, /* ret */
     };

    if (!pRtlCaptureContext)
    {
        win_skip("RtlCaptureContext is not available.\n");
        return;
    }

    memcpy( func_ptr, call_func, sizeof(call_func) );
    FlushInstructionCache( GetCurrentProcess(), func_ptr, sizeof(call_func) );

#define COMPARE(reg) \
    ok( contexts.before.reg == contexts.after.reg, "wrong " #reg " %p/%p\n", (void *)(ULONG64)contexts.before.reg, (void *)(ULONG64)contexts.after.reg )
#define COMPARE_INDEXED(reg) \
    ok( contexts.before.reg == contexts.after.reg, "wrong " #reg " i: %u, %p/%p\n", i, (void *)(ULONG64)contexts.before.reg, (void *)(ULONG64)contexts.after.reg )

    func_ptr( &contexts, 0, NtContinue, pRtlCaptureContext );

    for (i = 1; i < 29; i++) COMPARE_INDEXED( X[i] );

    COMPARE( Fpcr );
    COMPARE( Fpsr );

    for (i = 0; i < 32; i++)
    {
        COMPARE_INDEXED( V[i].Low );
        COMPARE_INDEXED( V[i].High );
    }

    apc_count = 0;
    pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234, 0x5678, 0xdeadbeef );
    func_ptr( &contexts, 0, NtContinue, pRtlCaptureContext );
    ok( apc_count == 0, "apc called\n" );
    func_ptr( &contexts, (void *)1, NtContinue, pRtlCaptureContext );
    ok( apc_count == 1, "apc not called\n" );

    if (!pNtContinueEx)
    {
        win_skip( "NtContinueEx not supported\n" );
        return;
    }

    func_ptr( &contexts, &args, pNtContinueEx, pRtlCaptureContext );

    for (i = 1; i < 29; i++) COMPARE_INDEXED( X[i] );

    COMPARE( Fpcr );
    COMPARE( Fpsr );

    for (i = 0; i < 32; i++)
    {
        COMPARE_INDEXED( V[i].Low );
        COMPARE_INDEXED( V[i].High );
    }

    pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234 + apc_count, 0x5678, 0xdeadbeef );
    func_ptr( &contexts, &args, pNtContinueEx, pRtlCaptureContext );
    ok( apc_count == 1, "apc called\n" );
    args.ContinueFlags = KCONTINUE_FLAG_TEST_ALERT;
    func_ptr( &contexts, &args, pNtContinueEx, pRtlCaptureContext );
    ok( apc_count == 2, "apc not called\n" );

#undef COMPARE
}

static BOOL hook_called;
static BOOL got_exception;

static ULONG patched_code[] =
{
    0x910003e0, /* mov x0, sp */
    0x5800004f, /* ldr x15, 1f */
    0xd61f01e0, /* br x15 */
    0, 0,       /* 1: hook_trampoline */
};
static ULONG saved_code[ARRAY_SIZE(patched_code)];

static LONG WINAPI dbg_except_continue_vectored_handler(struct _EXCEPTION_POINTERS *ptrs)
{
    EXCEPTION_RECORD *rec = ptrs->ExceptionRecord;
    CONTEXT *context = ptrs->ContextRecord;

    trace("dbg_except_continue_vectored_handler, code %#lx, pc %#Ix.\n", rec->ExceptionCode, context->Pc);
    got_exception = TRUE;

    ok(rec->ExceptionCode == 0x80000003, "Got unexpected exception code %#lx.\n", rec->ExceptionCode);
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void * WINAPI hook_KiUserExceptionDispatcher(void *stack)
{
    struct
    {
        CONTEXT              context;    /* 000 */
        CONTEXT_EX           context_ex; /* 390 */
        EXCEPTION_RECORD     rec;        /* 3b0 */
        ULONG64              align;      /* 448 */
        ULONG64              sp;         /* 450 */
        ULONG64              pc;         /* 458 */
    } *args = stack;
    EXCEPTION_RECORD *old_rec = (EXCEPTION_RECORD *)&args->context_ex;

    trace("stack %p context->Pc %#Ix, context->Sp %#Ix, ContextFlags %#lx.\n",
          stack, args->context.Pc, args->context.Sp, args->context.ContextFlags);

    hook_called = TRUE;
    ok( !((ULONG_PTR)stack & 15), "unaligned stack %p\n", stack );

    if (!broken( old_rec->ExceptionCode == 0x80000003 )) /* Windows 11 versions prior to 27686 */
    {
        ok( args->rec.ExceptionCode == 0x80000003, "Got unexpected ExceptionCode %#lx.\n", args->rec.ExceptionCode );

        ok( args->context_ex.All.Offset == -sizeof(CONTEXT), "wrong All.Offset %lx\n", args->context_ex.All.Offset );
        ok( args->context_ex.All.Length >= sizeof(CONTEXT) + offsetof(CONTEXT_EX, align), "wrong All.Length %lx\n", args->context_ex.All.Length );
        ok( args->context_ex.Legacy.Offset == -sizeof(CONTEXT), "wrong Legacy.Offset %lx\n", args->context_ex.All.Offset );
        ok( args->context_ex.Legacy.Length == sizeof(CONTEXT), "wrong Legacy.Length %lx\n", args->context_ex.All.Length );
        ok( args->sp == args->context.Sp, "wrong sp %Ix / %Ix\n", args->sp, args->context.Sp );
        ok( args->pc == args->context.Pc, "wrong pc %Ix / %Ix\n", args->pc, args->context.Pc );
    }

    memcpy(pKiUserExceptionDispatcher, saved_code, sizeof(saved_code));
    FlushInstructionCache( GetCurrentProcess(), pKiUserExceptionDispatcher, sizeof(saved_code));
    return pKiUserExceptionDispatcher;
}

static void test_KiUserExceptionDispatcher(void)
{
    ULONG hook_trampoline[] =
    {
        0x910003e0, /* mov x0, sp */
        0x5800006f, /* ldr x15, 1f */
        0xd63f01e0, /* blr x15 */
        0xd61f0000, /* br x0 */
        0, 0,       /* 1: hook_KiUserExceptionDispatcher */
    };

    EXCEPTION_RECORD record = { EXCEPTION_BREAKPOINT };
    void *trampoline_ptr, *vectored_handler;
    DWORD old_protect;
    BOOL ret;

    *(void **)&hook_trampoline[4] = hook_KiUserExceptionDispatcher;
    trampoline_ptr = (char *)code_mem + 1024;
    memcpy( trampoline_ptr, hook_trampoline, sizeof(hook_trampoline));

    ret = VirtualProtect( pKiUserExceptionDispatcher, sizeof(saved_code),
                          PAGE_EXECUTE_READWRITE, &old_protect );
    ok( ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError() );

    memcpy( saved_code, pKiUserExceptionDispatcher, sizeof(saved_code) );
    *(void **)&patched_code[3] = trampoline_ptr;

    vectored_handler = AddVectoredExceptionHandler(TRUE, dbg_except_continue_vectored_handler);

    memcpy( pKiUserExceptionDispatcher, patched_code, sizeof(patched_code) );
    FlushInstructionCache( GetCurrentProcess(), pKiUserExceptionDispatcher, sizeof(patched_code));

    got_exception = FALSE;
    hook_called = FALSE;

    pRtlRaiseException(&record);

    ok(got_exception, "Handler was not called.\n");
    ok(!hook_called, "Hook was called.\n");

    memcpy( pKiUserExceptionDispatcher, patched_code, sizeof(patched_code) );
    FlushInstructionCache( GetCurrentProcess(), pKiUserExceptionDispatcher, sizeof(patched_code));

    got_exception = 0;
    hook_called = FALSE;
    NtCurrentTeb()->Peb->BeingDebugged = 1;

    pRtlRaiseException(&record);

    ok(got_exception, "Handler was not called.\n");
    ok(hook_called, "Hook was not called.\n");
    NtCurrentTeb()->Peb->BeingDebugged = 0;

    RemoveVectoredExceptionHandler(vectored_handler);
    VirtualProtect(pKiUserExceptionDispatcher, sizeof(saved_code), old_protect, &old_protect);
}

static void * WINAPI hook_KiUserApcDispatcher(void *stack)
{
    struct
    {
        void *func;
        ULONG64 args[3];
        ULONG64 alertable;
        ULONG64 align;
        CONTEXT context;
    } *args = stack;

    trace( "stack=%p func=%p args=%Ix,%Ix,%Ix alertable=%Ix context=%p pc=%Ix sp=%Ix (%Ix)\n",
           args, args->func, args->args[0], args->args[1], args->args[2],
           args->alertable, &args->context, args->context.Pc, args->context.Sp,
           args->context.Sp - (ULONG_PTR)stack );

    ok( args->func == apc_func, "wrong func %p / %p\n", args->func, apc_func );
    ok( args->args[0] == 0x1234 + apc_count, "wrong arg1 %Ix\n", args->args[0] );
    ok( args->args[1] == 0x5678, "wrong arg2 %Ix\n", args->args[1] );
    ok( args->args[2] == 0xdeadbeef, "wrong arg3 %Ix\n", args->args[2] );

    if (apc_count) args->alertable = FALSE;
    pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234 + apc_count + 1, 0x5678, 0xdeadbeef );

    hook_called = TRUE;
    memcpy( pKiUserApcDispatcher, saved_code, sizeof(saved_code));
    FlushInstructionCache( GetCurrentProcess(), pKiUserApcDispatcher, sizeof(saved_code));
    return pKiUserApcDispatcher;
}

static void test_KiUserApcDispatcher(void)
{
    ULONG hook_trampoline[] =
    {
        0x910003e0, /* mov x0, sp */
        0x5800006f, /* ldr x15, 1f */
        0xd63f01e0, /* blr x15 */
        0xd61f0000, /* br x0 */
        0, 0,       /* 1: hook_KiUserApcDispatcher */
    };
    DWORD old_protect;
    BOOL ret;

    *(void **)&hook_trampoline[4] = hook_KiUserApcDispatcher;
    memcpy(code_mem, hook_trampoline, sizeof(hook_trampoline));

    ret = VirtualProtect( pKiUserApcDispatcher, sizeof(saved_code),
                          PAGE_EXECUTE_READWRITE, &old_protect );
    ok( ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError() );

    memcpy( saved_code, pKiUserApcDispatcher, sizeof(saved_code) );
    *(void **)&patched_code[3] = code_mem;
    memcpy( pKiUserApcDispatcher, patched_code, sizeof(patched_code) );
    FlushInstructionCache( GetCurrentProcess(), pKiUserApcDispatcher, sizeof(patched_code));

    hook_called = FALSE;
    apc_count = 0;
    pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234, 0x5678, 0xdeadbeef );
    SleepEx( 0, TRUE );
    ok( apc_count == 2, "APC count %u\n", apc_count );
    ok( hook_called, "hook was not called\n" );

    memcpy( pKiUserApcDispatcher, patched_code, sizeof(patched_code) );
    FlushInstructionCache( GetCurrentProcess(), pKiUserApcDispatcher, sizeof(patched_code));
    pNtQueueApcThread( GetCurrentThread(), apc_func, 0x1234 + apc_count, 0x5678, 0xdeadbeef );
    SleepEx( 0, TRUE );
    ok( apc_count == 3, "APC count %u\n", apc_count );
    SleepEx( 0, TRUE );
    ok( apc_count == 4, "APC count %u\n", apc_count );

    VirtualProtect( pKiUserApcDispatcher, sizeof(saved_code), old_protect, &old_protect );
}

static void WINAPI hook_KiUserCallbackDispatcher(void *sp)
{
    struct
    {
        void *args;
        ULONG len;
        ULONG id;
        ULONG64 unknown;
        ULONG64 lr;
        ULONG64 sp;
        ULONG64 pc;
        BYTE args_data[0];
    } *stack = sp;
    ULONG_PTR redzone = (BYTE *)stack->sp - &stack->args_data[stack->len];
    KERNEL_CALLBACK_PROC func = NtCurrentTeb()->Peb->KernelCallbackTable[stack->id];

    trace( "stack=%p len=%lx id=%lx unk=%Ix lr=%Ix sp=%Ix pc=%Ix\n",
           stack, stack->len, stack->id, stack->unknown, stack->lr, stack->sp, stack->pc );

    ok( stack->args == stack->args_data, "wrong args %p / %p\n", stack->args, stack->args_data );
    ok( redzone >= 16 && redzone <= 32, "wrong sp %p / %p (%Iu)\n",
        (void *)stack->sp, stack->args_data, redzone );

    if (pRtlPcToFileHeader)
    {
        void *mod, *win32u = GetModuleHandleA("win32u.dll");

        pRtlPcToFileHeader( (void *)stack->pc, &mod );
        ok( mod == win32u, "pc %Ix not in win32u %p\n", stack->pc, win32u );
    }
    NtCallbackReturn( NULL, 0, func( stack->args, stack->len ));
}

static void test_KiUserCallbackDispatcher(void)
{
    DWORD old_protect;
    BOOL ret;

    ret = VirtualProtect( pKiUserCallbackDispatcher, sizeof(saved_code),
                          PAGE_EXECUTE_READWRITE, &old_protect );
    ok( ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError() );

    memcpy( saved_code, pKiUserCallbackDispatcher, sizeof(saved_code));
    *(void **)&patched_code[3] = hook_KiUserCallbackDispatcher;
    memcpy( pKiUserCallbackDispatcher, patched_code, sizeof(patched_code));
    FlushInstructionCache(GetCurrentProcess(), pKiUserCallbackDispatcher, sizeof(patched_code));

    DestroyWindow( CreateWindowA( "Static", "test", 0, 0, 0, 0, 0, 0, 0, 0, 0 ));

    memcpy( pKiUserCallbackDispatcher, saved_code, sizeof(saved_code));
    FlushInstructionCache(GetCurrentProcess(), pKiUserCallbackDispatcher, sizeof(saved_code));
    VirtualProtect( pKiUserCallbackDispatcher, sizeof(saved_code), old_protect, &old_protect );
}

static void run_exception_test(void *handler, const void* context,
                               const void *code, unsigned int code_size,
                               unsigned int func2_offset, DWORD access, DWORD handler_flags,
                               void *arg1, void *arg2)
{
    DWORD buf[14];
    RUNTIME_FUNCTION runtime_func[2];
    IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY_XDATA unwind;
    void (*func)(void*,void*) = code_mem;
    DWORD oldaccess, oldaccess2;

    runtime_func[0].BeginAddress = 0;
    runtime_func[0].UnwindData = 0x1000;
    runtime_func[1].BeginAddress = func2_offset;
    runtime_func[1].UnwindData = 0x1014;

    unwind.FunctionLength = func2_offset / 4;
    unwind.Version = 0;
    unwind.ExceptionDataPresent = 1;
    unwind.EpilogInHeader = 1;
    unwind.EpilogCount = 1;
    unwind.CodeWords = 1;
    buf[0] = unwind.HeaderData;
    buf[1] = 0xe3e481e1; /* mov x29,sp; stp r29,lr,[sp,-#0x10]!; end; nop */
    buf[2] = 0x1028;
    *(const void **)&buf[3] = context;

    unwind.FunctionLength = (code_size - func2_offset) / 4;
    buf[5] = unwind.HeaderData;
    buf[6] = 0xe3e481e1; /* mov x29,sp; stp r29,lr,[sp,-#0x10]!; end; nop */
    buf[7] = 0x1028;
    *(const void **)&buf[8] = context;

    buf[10] = 0x5800004f; /* ldr x15, 1f */
    buf[11] = 0xd61f01e0; /* br x15 */
    *(const void **)&buf[12] = handler;

    memcpy((unsigned char *)code_mem + 0x1000, buf, sizeof(buf));
    memcpy(code_mem, code, code_size);
    if (access) VirtualProtect(code_mem, code_size, access, &oldaccess);
    FlushInstructionCache( GetCurrentProcess(), code_mem, 0x2000 );

    pRtlAddFunctionTable(runtime_func, ARRAY_SIZE(runtime_func), (ULONG_PTR)code_mem);
    func( arg1, arg2 );
    pRtlDeleteFunctionTable(runtime_func);

    if (access) VirtualProtect(code_mem, code_size, oldaccess, &oldaccess2);
}

static BOOL got_nested_exception, got_prev_frame_exception;
static void *nested_exception_initial_frame;

static DWORD nested_exception_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                      CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    trace("nested_exception_handler pc %p, sp %p, code %#lx, flags %#lx, ExceptionAddress %p.\n",
          (void *)context->Pc, (void *)context->Sp, rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress);

    if (rec->ExceptionCode == 0x80000003 && !(rec->ExceptionFlags & EXCEPTION_NESTED_CALL))
    {
        ok(rec->NumberParameters == 1, "Got unexpected rec->NumberParameters %lu.\n", rec->NumberParameters);
        ok((char *)context->Sp == (char *)frame - 0x10, "Got unexpected frame %p / %p.\n", frame, (void *)context->Sp);
        ok((char *)context->Fp == (char *)frame - 0x10, "Got unexpected frame %p / %p.\n", frame, (void *)context->Fp);
        ok((char *)context->Lr == (char *)code_mem + 0x0c, "Got unexpected lr %p.\n", (void *)context->Lr);
        ok((char *)context->Pc == (char *)code_mem + 0x1c, "Got unexpected pc %p.\n", (void *)context->Pc);

        nested_exception_initial_frame = frame;
        RaiseException(0xdeadbeef, 0, 0, 0);
        context->Pc += 4;
        return ExceptionContinueExecution;
    }

    if (rec->ExceptionCode == 0xdeadbeef &&
        (rec->ExceptionFlags == EXCEPTION_NESTED_CALL ||
         rec->ExceptionFlags == (EXCEPTION_NESTED_CALL | EXCEPTION_SOFTWARE_ORIGINATE)))
    {
        ok(!rec->NumberParameters, "Got unexpected rec->NumberParameters %lu.\n", rec->NumberParameters);
        got_nested_exception = TRUE;
        ok(frame == nested_exception_initial_frame, "Got unexpected frame %p / %p.\n",
           frame, nested_exception_initial_frame);
        return ExceptionContinueSearch;
    }

    ok(rec->ExceptionCode == 0xdeadbeef && (!rec->ExceptionFlags || rec->ExceptionFlags == EXCEPTION_SOFTWARE_ORIGINATE),
       "Got unexpected exception code %#lx, flags %#lx.\n", rec->ExceptionCode, rec->ExceptionFlags);
    ok(!rec->NumberParameters, "Got unexpected rec->NumberParameters %lu.\n", rec->NumberParameters);
    ok((char *)frame == (char *)nested_exception_initial_frame + 0x10, "Got unexpected frame %p / %p.\n",
        frame, nested_exception_initial_frame);
    got_prev_frame_exception = TRUE;
    return ExceptionContinueExecution;
}

static const DWORD nested_except_code[] =
{
    0xa9bf7bfd, /* 00: stp x29, x30, [sp, #-16]! */
    0x910003fd, /* 04: mov x29, sp */
    0x94000003, /* 08: bl 1f */
    0xa8c17bfd, /* 0c: ldp x29, x30, [sp], #16 */
    0xd65f03c0, /* 10: ret */

    0xa9bf7bfd, /* 14: stp x29, x30, [sp, #-16]! */
    0x910003fd, /* 18: mov x29, sp */
    0xd43e0000, /* 1c: brk #0xf000 */
    0xd503201f, /* 20: nop */
    0xa8c17bfd, /* 24: ldp x29, x30, [sp], #16 */
    0xd65f03c0, /* 28: ret */
};

static void test_nested_exception(void)
{
    got_nested_exception = got_prev_frame_exception = FALSE;
    run_exception_test(nested_exception_handler, NULL, nested_except_code, sizeof(nested_except_code),
                       5 * sizeof(DWORD), PAGE_EXECUTE_READ, UNW_FLAG_EHANDLER, 0, 0);
    ok(got_nested_exception, "Did not get nested exception.\n");
    ok(got_prev_frame_exception, "Did not get nested exception in the previous frame.\n");
}

static unsigned int collided_unwind_exception_count;

static DWORD collided_exception_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    CONTEXT ctx;

    trace("collided_exception_handler pc %p, sp %p, code %#lx, flags %#lx, ExceptionAddress %p, frame %p.\n",
          (void *)context->Pc, (void *)context->Sp, rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, frame);

    switch(collided_unwind_exception_count++)
    {
        case 0:
            /* Initial exception from nested_except_code. */
            ok(rec->ExceptionCode == STATUS_BREAKPOINT, "got %#lx.\n", rec->ExceptionCode);
            nested_exception_initial_frame = frame;
            /* Start unwind. */
            pRtlUnwindEx((char *)frame + 0x10, (char *)code_mem + 0x0c, NULL, NULL, &ctx, NULL);
            ok(0, "shouldn't be reached\n");
            break;
        case 1:
            ok(rec->ExceptionCode == STATUS_UNWIND, "got %#lx.\n", rec->ExceptionCode);
            ok(rec->ExceptionFlags == EXCEPTION_UNWINDING, "got %#lx.\n", rec->ExceptionFlags);
            ok((char *)context->Pc == (char *)code_mem + 0x1c, "got %p.\n", (void *)context->Pc);
            /* generate exception in unwind handler. */
            RaiseException(0xdeadbeef, 0, 0, 0);
            ok(0, "shouldn't be reached\n");
            break;
        case 2:
            /* Inner call frame, continue search. */
            ok(rec->ExceptionCode == 0xdeadbeef, "got %#lx.\n", rec->ExceptionCode);
            ok(!rec->ExceptionFlags || rec->ExceptionFlags == EXCEPTION_SOFTWARE_ORIGINATE, "got %#lx.\n", rec->ExceptionFlags);
            ok(frame == nested_exception_initial_frame, "got %p, expected %p.\n", frame, nested_exception_initial_frame);
            break;
        case 3:
            /* Top level call frame, handle exception by unwinding. */
            ok(rec->ExceptionCode == 0xdeadbeef, "got %#lx.\n", rec->ExceptionCode);
            ok(!rec->ExceptionFlags || rec->ExceptionFlags == EXCEPTION_SOFTWARE_ORIGINATE, "got %#lx.\n", rec->ExceptionFlags);
            ok((char *)frame == (char *)nested_exception_initial_frame + 0x10, "got %p, expected %p.\n", frame, nested_exception_initial_frame);
            pRtlUnwindEx((char *)nested_exception_initial_frame + 0x10, (char *)code_mem + 0x0c, NULL, NULL, &ctx, NULL);
            ok(0, "shouldn't be reached\n");
            break;
        case 4:
            /* Collided unwind. */
            ok(rec->ExceptionCode == STATUS_UNWIND, "got %#lx.\n", rec->ExceptionCode);
            ok(rec->ExceptionFlags == (EXCEPTION_UNWINDING | EXCEPTION_COLLIDED_UNWIND), "got %#lx.\n", rec->ExceptionFlags);
            ok(frame == nested_exception_initial_frame, "got %p, expected %p.\n", frame, nested_exception_initial_frame);
            break;
        case 5:
            /* EXCEPTION_COLLIDED_UNWIND cleared for the following frames. */
            ok(rec->ExceptionCode == STATUS_UNWIND, "got %#lx.\n", rec->ExceptionCode);
            ok(rec->ExceptionFlags == (EXCEPTION_UNWINDING | EXCEPTION_TARGET_UNWIND), "got %#lx.\n", rec->ExceptionFlags);
            ok((char *)frame == (char *)nested_exception_initial_frame + 0x10, "got %p, expected %p.\n", frame, nested_exception_initial_frame);
            break;
    }
    return ExceptionContinueSearch;
}

static void test_collided_unwind(void)
{
    got_nested_exception = got_prev_frame_exception = FALSE;
    collided_unwind_exception_count = 0;
    run_exception_test(collided_exception_handler, NULL, nested_except_code, sizeof(nested_except_code),
                       5 * sizeof(DWORD), PAGE_EXECUTE_READ, UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER, 0, 0);
    ok(collided_unwind_exception_count == 6, "got %u.\n", collided_unwind_exception_count);
}


static int rtlraiseexception_unhandled_handler_called;
static int rtlraiseexception_teb_handler_called;
static int rtlraiseexception_handler_called;

static void rtlraiseexception_handler_( EXCEPTION_RECORD *rec, void *frame, CONTEXT *context,
                                        void *dispatcher, BOOL unhandled_handler )
{
    void *addr = rec->ExceptionAddress;

    trace( "exception: %08lx flags:%lx addr:%p context: Pc:%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, (void *)context->Pc );

    ok( addr == (char *)code_mem + 0x0c,
        "ExceptionAddress at %p instead of %p\n", addr, (char *)code_mem + 0x0c );
    ok( context->ContextFlags == (CONTEXT_FULL | CONTEXT_UNWOUND_TO_CALL) ||
        context->ContextFlags == (CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS | CONTEXT_UNWOUND_TO_CALL),
        "wrong context flags %lx\n", context->ContextFlags );
    ok( context->Pc == (UINT_PTR)addr,
        "%d: Pc at %Ix instead of %Ix\n", test_stage, context->Pc, (UINT_PTR)addr );

    ok( context->X0 == 0xf00f00f0, "context->X0 is %Ix, should have been set to 0xf00f00f0 in vectored handler\n", context->X0 );
}

static LONG CALLBACK rtlraiseexception_unhandled_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT context = ExceptionInfo->ContextRecord;
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;

    rtlraiseexception_unhandled_handler_called = 1;
    rtlraiseexception_handler_(rec, NULL, context, NULL, TRUE);
    if (test_stage == STAGE_RTLRAISE_HANDLE_LAST_CHANCE) return EXCEPTION_CONTINUE_SEARCH;

    return EXCEPTION_CONTINUE_EXECUTION;
}

static DWORD WINAPI rtlraiseexception_teb_handler( EXCEPTION_RECORD *rec,
                                                   EXCEPTION_REGISTRATION_RECORD *frame,
                                                   CONTEXT *context,
                                                   EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    rtlraiseexception_teb_handler_called = 1;
    rtlraiseexception_handler_(rec, frame, context, dispatcher, FALSE);
    return ExceptionContinueSearch;
}

static DWORD WINAPI rtlraiseexception_handler( EXCEPTION_RECORD *rec, void *frame,
                                               CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    DISPATCHER_CONTEXT_NONVOLREG_ARM64 *nonvol_regs = (void *)dispatcher->NonVolatileRegisters;
    int i;

    for (i = 0; i < NONVOL_INT_NUMREG_ARM64; i++)
        ok( nonvol_regs->GpNvRegs[i] == ((DWORD64 *)&context->X19)[i],
            "wrong non volatile reg x%u %I64x / %I64x\n", i + 19,
            nonvol_regs->GpNvRegs[i] , ((DWORD64 *)&context->X19)[i] );
    for (i = 0; i < NONVOL_FP_NUMREG_ARM64; i++)
        ok( nonvol_regs->FpNvRegs[i] == context->V[i + 8].D[0],
            "wrong non volatile reg d%u %g / %g\n", i + 8,
            nonvol_regs->FpNvRegs[i] , context->V[i + 8].D[0] );

    rtlraiseexception_handler_called = 1;
    rtlraiseexception_handler_(rec, frame, context, dispatcher, FALSE);
    return ExceptionContinueSearch;
}

static LONG CALLBACK rtlraiseexception_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT context = ExceptionInfo->ContextRecord;
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    void *addr = rec->ExceptionAddress;

    ok( addr == (char *)code_mem + 0xc,
        "ExceptionAddress at %p instead of %p\n", addr, (char *)code_mem + 0xc );
    ok( context->Pc == (UINT_PTR)addr,
        "%d: Pc at %Ix instead of %Ix\n", test_stage, context->Pc, (UINT_PTR)addr );

    context->X0 = 0xf00f00f0;

    return EXCEPTION_CONTINUE_SEARCH;
}

static const DWORD call_one_arg_code[] =
{
    0xa9bf7bfd, /* 00: stp x29, x30, [sp, #-16]! */
    0x910003fd, /* 04: mov x29, sp */
    0xd63f0020, /* 08: blr x1 */
    0xd503201f, /* 0c: nop */
    0xa8c17bfd, /* 10: ldp x29, x30, [sp], #16 */
    0xd65f03c0, /* 14: ret */
};

static void run_rtlraiseexception_test(DWORD exceptioncode)
{
    EXCEPTION_REGISTRATION_RECORD frame;
    EXCEPTION_RECORD record;
    PVOID vectored_handler = NULL;

    record.ExceptionCode = exceptioncode;
    record.ExceptionFlags = 0;
    record.ExceptionRecord = NULL;
    record.ExceptionAddress = NULL; /* does not matter, copied return address */
    record.NumberParameters = 0;

    frame.Handler = rtlraiseexception_teb_handler;
    frame.Prev = NtCurrentTeb()->Tib.ExceptionList;

    NtCurrentTeb()->Tib.ExceptionList = &frame;
    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, rtlraiseexception_vectored_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");
    if (pRtlSetUnhandledExceptionFilter) pRtlSetUnhandledExceptionFilter(rtlraiseexception_unhandled_handler);

    rtlraiseexception_handler_called = 0;
    rtlraiseexception_teb_handler_called = 0;
    rtlraiseexception_unhandled_handler_called = 0;

    run_exception_test( rtlraiseexception_handler, NULL, call_one_arg_code,
                        sizeof(call_one_arg_code), sizeof(call_one_arg_code),
                        PAGE_EXECUTE_READ, UNW_FLAG_EHANDLER,
                        &record, pRtlRaiseException);

    ok( record.ExceptionAddress == (char *)code_mem + 0x0c,
        "address set to %p instead of %p\n", record.ExceptionAddress, (char *)code_mem + 0x0c );

    todo_wine
    ok( !rtlraiseexception_teb_handler_called, "Frame TEB handler called\n" );
    ok( rtlraiseexception_handler_called, "Frame handler called\n" );
    ok( rtlraiseexception_unhandled_handler_called, "UnhandledExceptionFilter wasn't called\n" );

    pRtlRemoveVectoredExceptionHandler(vectored_handler);

    if (pRtlSetUnhandledExceptionFilter) pRtlSetUnhandledExceptionFilter(NULL);
    NtCurrentTeb()->Tib.ExceptionList = frame.Prev;
}

static void test_rtlraiseexception(void)
{
    run_rtlraiseexception_test(0x12345);
    run_rtlraiseexception_test(EXCEPTION_BREAKPOINT);
    run_rtlraiseexception_test(EXCEPTION_INVALID_HANDLE);
}

static DWORD brk_exception_handler_code;

static DWORD WINAPI brk_exception_handler( EXCEPTION_RECORD *rec, void *frame,
                                           CONTEXT *context, DISPATCHER_CONTEXT *dispatcher )
{
    ok( rec->ExceptionCode == brk_exception_handler_code, "got: %08lx\n", rec->ExceptionCode );
    ok( rec->NumberParameters == 0, "got: %ld\n", rec->NumberParameters );
    ok( rec->ExceptionAddress == (void *)context->Pc, "got addr: %p, pc: %p\n", rec->ExceptionAddress, (void *)context->Pc );
    context->Pc += 4;
    return ExceptionContinueExecution;
}


static void test_brk(void)
{
    DWORD call_brk[] =
    {
        0xa9bf7bfd, /* 00: stp x29, x30, [sp, #-16]! */
        0x910003fd, /* 04: mov x29, sp */
        0x00000000, /* 08: <filled in later> */
        0xd503201f, /* 0c: nop */
        0xa8c17bfd, /* 10: ldp x29, x30, [sp], #16 */
        0xd65f03c0, /* 14: ret */
    };

    /* brk #0xf000 is tested as part of breakpoint tests */

    brk_exception_handler_code = STATUS_ASSERTION_FAILURE;
    call_brk[2] = 0xd43e0020; /* 08: brk #0xf001 */
    run_exception_test( brk_exception_handler, NULL, call_brk,
                        sizeof(call_brk), sizeof(call_brk),
                        PAGE_EXECUTE_READ, UNW_FLAG_EHANDLER,
                        0, 0 );

    /* FIXME: brk #0xf002 needs debug service tests */

    /* brk #0xf003 is tested as part of fastfail tests*/

    brk_exception_handler_code = EXCEPTION_INT_DIVIDE_BY_ZERO;
    call_brk[2] = 0xd43e0080; /* 08: brk #0xf004 */
    run_exception_test( brk_exception_handler, NULL, call_brk,
                        sizeof(call_brk), sizeof(call_brk),
                        PAGE_EXECUTE_READ, UNW_FLAG_EHANDLER,
                        0, 0 );

    /* Any unknown immediate raises EXCEPTION_ILLEGAL_INSTRUCTION */

    brk_exception_handler_code = EXCEPTION_ILLEGAL_INSTRUCTION;
    call_brk[2] = 0xd43e00a0; /* 08: brk #0xf005 */
    run_exception_test( brk_exception_handler, NULL, call_brk,
                        sizeof(call_brk), sizeof(call_brk),
                        PAGE_EXECUTE_READ, UNW_FLAG_EHANDLER,
                        0, 0 );

    brk_exception_handler_code = EXCEPTION_ILLEGAL_INSTRUCTION;
    call_brk[2] = 0xd4200000; /* 08: brk #0x0 */
    run_exception_test( brk_exception_handler, NULL, call_brk,
                        sizeof(call_brk), sizeof(call_brk),
                        PAGE_EXECUTE_READ, UNW_FLAG_EHANDLER,
                        0, 0 );
}

static LONG consolidate_dummy_called;
static LONG pass;

static const DWORD call_rtlunwind[] =
{
    0xa88150f3, /* stp x19, x20, [x7], #0x10 */
    0xa88158f5, /* stp x21, x22, [x7], #0x10 */
    0xa88160f7, /* stp x23, x24, [x7], #0x10 */
    0xa88168f9, /* stp x25, x26, [x7], #0x10 */
    0xa88170fb, /* stp x27, x28, [x7], #0x10 */
    0xf80084fd, /* str x29,      [x7], #0x8 */
    0x6c8124e8, /* stp d8,  d9,  [x7], #0x10 */
    0x6c812cea, /* stp d10, d11, [x7], #0x10 */
    0x6c8134ec, /* stp d12, d13, [x7], #0x10 */
    0x6c813cee, /* stp d14, d15, [x7], #0x10 */
    0xd61f00c0, /* br x6 */
};

static PVOID CALLBACK test_consolidate_dummy(EXCEPTION_RECORD *rec)
{
    CONTEXT *ctx = (CONTEXT *)rec->ExceptionInformation[1];
    DWORD64 *saved_regs = (DWORD64 *)rec->ExceptionInformation[3];
    DISPATCHER_CONTEXT_NONVOLREG_ARM64 *regs;
    int i;

    switch (InterlockedIncrement(&consolidate_dummy_called))
    {
    case 1:  /* RtlRestoreContext */
        ok(ctx->Pc == 0xdeadbeef, "RtlRestoreContext wrong Pc, expected: 0xdeadbeef, got: %Ix\n", ctx->Pc);
        ok( rec->ExceptionInformation[10] == -1, "wrong info %Ix\n", rec->ExceptionInformation[10] );
        break;
    case 2: /* RtlUnwindEx */
        ok(ctx->Pc != 0xdeadbeef, "RtlUnwindEx wrong Pc, got: %Ix\n", ctx->Pc );
        ok( rec->ExceptionInformation[10] != -1, "wrong info %Ix\n", rec->ExceptionInformation[10] );
        regs = (DISPATCHER_CONTEXT_NONVOLREG_ARM64 *)rec->ExceptionInformation[10];
        for (i = 0; i < 11; i++)
            ok( saved_regs[i] == regs->GpNvRegs[i], "wrong reg X%u, expected: %Ix, got: %Ix\n",
                19 + i, saved_regs[i], regs->GpNvRegs[i] );
        for (i = 0; i < 8; i++)
            ok( saved_regs[i + 11] == *(DWORD64 *)&regs->FpNvRegs[i],
                "wrong reg D%u, expected: %Ix, got: %Ix\n",
                i + 8, saved_regs[i + 11], *(DWORD64 *)&regs->FpNvRegs[i] );
        break;
    }
    return (PVOID)rec->ExceptionInformation[2];
}

static void test_restore_context(void)
{
    EXCEPTION_RECORD rec;
    _JUMP_BUFFER buf;
    CONTEXT ctx;
    int i;

    if (!pRtlUnwindEx || !pRtlRestoreContext || !pRtlCaptureContext)
    {
        skip("RtlUnwindEx/RtlCaptureContext/RtlRestoreContext not found\n");
        return;
    }

    /* test simple case of capture and restore context */
    pass = 0;
    InterlockedIncrement(&pass); /* interlocked to prevent compiler from moving after capture */
    pRtlCaptureContext(&ctx);
    if (InterlockedIncrement(&pass) == 2) /* interlocked to prevent compiler from moving before capture */
    {
        pRtlRestoreContext(&ctx, NULL);
        ok(0, "shouldn't be reached\n");
    }
    else
        ok(pass < 4, "unexpected pass %ld\n", pass);

    /* test with jmp using RtlRestoreContext */
    pass = 0;
    InterlockedIncrement(&pass);
    RtlCaptureContext(&ctx);
    InterlockedIncrement(&pass); /* only called once */
    setjmp((_JBTYPE *)&buf);
    InterlockedIncrement(&pass);
    if (pass == 3)
    {
        rec.ExceptionCode = STATUS_LONGJUMP;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD64)&buf;
        /* uses buf.Pc instead of ctx.Pc */
        pRtlRestoreContext(&ctx, &rec);
        ok(0, "shouldn't be reached\n");
    }
    else if (pass == 4)
    {
        ok(buf.X19 == ctx.X19, "longjmp failed for X19, expected: %Ix, got: %Ix\n", buf.X19, ctx.X19);
        ok(buf.X20 == ctx.X20, "longjmp failed for X20, expected: %Ix, got: %Ix\n", buf.X20, ctx.X20);
        ok(buf.X21 == ctx.X21, "longjmp failed for X21, expected: %Ix, got: %Ix\n", buf.X21, ctx.X21);
        ok(buf.X22 == ctx.X22, "longjmp failed for X22, expected: %Ix, got: %Ix\n", buf.X22, ctx.X22);
        ok(buf.X23 == ctx.X23, "longjmp failed for X23, expected: %Ix, got: %Ix\n", buf.X23, ctx.X23);
        ok(buf.X24 == ctx.X24, "longjmp failed for X24, expected: %Ix, got: %Ix\n", buf.X24, ctx.X24);
        ok(buf.X25 == ctx.X25, "longjmp failed for X25, expected: %Ix, got: %Ix\n", buf.X25, ctx.X25);
        ok(buf.X26 == ctx.X26, "longjmp failed for X26, expected: %Ix, got: %Ix\n", buf.X26, ctx.X26);
        ok(buf.X27 == ctx.X27, "longjmp failed for X27, expected: %Ix, got: %Ix\n", buf.X27, ctx.X27);
        ok(buf.X28 == ctx.X28, "longjmp failed for X28, expected: %Ix, got: %Ix\n", buf.X28, ctx.X28);
        ok(buf.Fp  == ctx.Fp,  "longjmp failed for Fp, expected: %Ix, got: %Ix\n",  buf.Fp,  ctx.Fp);
        for (i = 0; i < 8; i++)
            ok(buf.D[i] == ctx.V[i + 8].D[0], "longjmp failed for D%u, expected: %g, got: %g\n",
               i + 8, buf.D[i], ctx.V[i + 8].D[0]);
        pRtlRestoreContext(&ctx, &rec);
        ok(0, "shouldn't be reached\n");
    }
    else
        ok(pass == 5, "unexpected pass %ld\n", pass);

    /* test with jmp through RtlUnwindEx */
    pass = 0;
    InterlockedIncrement(&pass);
    pRtlCaptureContext(&ctx);
    InterlockedIncrement(&pass); /* only called once */
    setjmp((_JBTYPE *)&buf);
    InterlockedIncrement(&pass);
    if (pass == 3)
    {
        rec.ExceptionCode = STATUS_LONGJUMP;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD64)&buf;

        /* uses buf.Pc instead of bogus 0xdeadbeef */
        pRtlUnwindEx((void*)buf.Sp, (void*)0xdeadbeef, &rec, NULL, &ctx, NULL);
        ok(0, "shouldn't be reached\n");
    }
    else
        ok(pass == 4, "unexpected pass %ld\n", pass);


    /* test with consolidate */
    pass = 0;
    InterlockedIncrement(&pass);
    RtlCaptureContext(&ctx);
    InterlockedIncrement(&pass);
    if (pass == 2)
    {
        rec.ExceptionCode = STATUS_UNWIND_CONSOLIDATE;
        rec.NumberParameters = 3;
        rec.ExceptionInformation[0] = (DWORD64)test_consolidate_dummy;
        rec.ExceptionInformation[1] = (DWORD64)&ctx;
        rec.ExceptionInformation[2] = ctx.Pc;
        rec.ExceptionInformation[10] = -1;
        ctx.Pc = 0xdeadbeef;

        pRtlRestoreContext(&ctx, &rec);
        ok(0, "shouldn't be reached\n");
    }
    else if (pass == 3)
        ok(consolidate_dummy_called == 1, "test_consolidate_dummy not called\n");
    else
        ok(0, "unexpected pass %ld\n", pass);

    /* test with consolidate through RtlUnwindEx */
    pass = 0;
    InterlockedIncrement(&pass);
    pRtlCaptureContext(&ctx);
    InterlockedIncrement(&pass);
    setjmp((_JBTYPE *)&buf);
    if (pass == 2)
    {
        void (*func)(DWORD64,DWORD64,EXCEPTION_RECORD*,DWORD64,CONTEXT*,void*,void*,void*) = code_mem;
        DWORD64 nonvol_regs[19];

        rec.ExceptionCode = STATUS_UNWIND_CONSOLIDATE;
        rec.NumberParameters = 4;
        rec.ExceptionInformation[0] = (DWORD64)test_consolidate_dummy;
        rec.ExceptionInformation[1] = (DWORD64)&ctx;
        rec.ExceptionInformation[2] = ctx.Pc;
        rec.ExceptionInformation[3] = (DWORD64)nonvol_regs;
        rec.ExceptionInformation[10] = -1;  /* otherwise it doesn't get set */
        ctx.Pc = 0xdeadbeef;
        /* uses consolidate callback Pc instead of bogus 0xdeadbeef */
        memcpy( code_mem, call_rtlunwind, sizeof(call_rtlunwind) );
        FlushInstructionCache( GetCurrentProcess(), code_mem, sizeof(call_rtlunwind) );
        func( buf.Frame, 0xdeadbeef, &rec, 0, &ctx, NULL, pRtlUnwindEx, nonvol_regs );
        ok(0, "shouldn't be reached\n");
    }
    else if (pass == 3)
        ok(consolidate_dummy_called == 2, "test_consolidate_dummy not called\n");
    else
        ok(0, "unexpected pass %ld\n", pass);
}

static void test_mrs_currentel(void)
{
    DWORD64 (*func_ptr)(void) = code_mem;
    DWORD64 result;

    static const DWORD call_func[] =
    {
        0xd5384240, /* mrs x0, CurrentEL */
        0xd538425f, /* mrs xzr, CurrentEL */
        0xd65f03c0, /* ret */
    };

    memcpy( func_ptr, call_func, sizeof(call_func) );
    FlushInstructionCache( GetCurrentProcess(), func_ptr, sizeof(call_func) );
    result = func_ptr();
    ok( result == 0, "expected 0, got %llx\n", result );
}


#endif  /* __aarch64__ */

#if defined(__i386__) || defined(__x86_64__)

static DWORD WINAPI register_check_thread(void *arg)
{
    NTSTATUS status;
    CONTEXT ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    status = pNtGetContextThread(GetCurrentThread(), &ctx);
    ok(status == STATUS_SUCCESS, "NtGetContextThread failed with %lx\n", status);
    ok(!ctx.Dr0, "expected 0, got %Ix\n", (DWORD_PTR)ctx.Dr0);
    ok(!ctx.Dr1, "expected 0, got %Ix\n", (DWORD_PTR)ctx.Dr1);
    ok(!ctx.Dr2, "expected 0, got %Ix\n", (DWORD_PTR)ctx.Dr2);
    ok(!ctx.Dr3, "expected 0, got %Ix\n", (DWORD_PTR)ctx.Dr3);
    ok(!ctx.Dr6, "expected 0, got %Ix\n", (DWORD_PTR)ctx.Dr6);
    ok(!ctx.Dr7, "expected 0, got %Ix\n", (DWORD_PTR)ctx.Dr7);

    return 0;
}

static void test_debug_registers(void)
{
    static const struct
    {
        ULONG_PTR dr0, dr1, dr2, dr3, dr6, dr7;
    }
    tests[] =
    {
        { 0x42424240, 0, 0x126bb070, 0x0badbad0, 0, 0xffff0115 },
        { 0x42424242, 0, 0x100f0fe7, 0x0abebabe, 0, 0x115 },
    };
    NTSTATUS status;
    CONTEXT ctx;
    HANDLE thread;
    int i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        memset(&ctx, 0, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        ctx.Dr0 = tests[i].dr0;
        ctx.Dr1 = tests[i].dr1;
        ctx.Dr2 = tests[i].dr2;
        ctx.Dr3 = tests[i].dr3;
        ctx.Dr6 = tests[i].dr6;
        ctx.Dr7 = tests[i].dr7;

        status = pNtSetContextThread(GetCurrentThread(), &ctx);
        ok(status == STATUS_SUCCESS, "NtSetContextThread failed with %08lx\n", status);

        memset(&ctx, 0xcc, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

        status = pNtGetContextThread(GetCurrentThread(), &ctx);
        ok(status == STATUS_SUCCESS, "NtGetContextThread failed with %08lx\n", status);
        if (is_arm64ec)  /* setting debug registers is silently ignored */
        {
            ok(!ctx.Dr0, "test %d: expected 0, got %Ix\n", i, (DWORD_PTR)ctx.Dr0);
            ok(!ctx.Dr1, "test %d: expected 0, got %Ix\n", i, (DWORD_PTR)ctx.Dr1);
            ok(!ctx.Dr2, "test %d: expected 0, got %Ix\n", i, (DWORD_PTR)ctx.Dr2);
            ok(!ctx.Dr3, "test %d: expected 0, got %Ix\n", i, (DWORD_PTR)ctx.Dr3);
            ok(!ctx.Dr6, "test %d: expected 0, got %Ix\n", i, (DWORD_PTR)ctx.Dr6);
            ok(!ctx.Dr7, "test %d: expected 0, got %Ix\n", i, (DWORD_PTR)ctx.Dr7);
        }
        else
        {
            ok(ctx.Dr0 == tests[i].dr0, "test %d: expected %Ix, got %Ix\n", i, tests[i].dr0, (DWORD_PTR)ctx.Dr0);
            ok(ctx.Dr1 == tests[i].dr1, "test %d: expected %Ix, got %Ix\n", i, tests[i].dr1, (DWORD_PTR)ctx.Dr1);
            ok(ctx.Dr2 == tests[i].dr2, "test %d: expected %Ix, got %Ix\n", i, tests[i].dr2, (DWORD_PTR)ctx.Dr2);
            ok(ctx.Dr3 == tests[i].dr3, "test %d: expected %Ix, got %Ix\n", i, tests[i].dr3, (DWORD_PTR)ctx.Dr3);
            ok((ctx.Dr6 &  0xf00f) == tests[i].dr6, "test %d: expected %Ix, got %Ix\n", i, tests[i].dr6, (DWORD_PTR)ctx.Dr6);
            ok((ctx.Dr7 & ~0xdc00) == tests[i].dr7, "test %d: expected %Ix, got %Ix\n", i, tests[i].dr7, (DWORD_PTR)ctx.Dr7);
        }
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    ctx.Dr0 = 0xffffffff;
    ctx.Dr1 = 0xffffffff;
    ctx.Dr2 = 0xffffffff;
    ctx.Dr3 = 0xffffffff;
    ctx.Dr6 = 0xffffffff;
    ctx.Dr7 = 0x00000400;
    status = pNtSetContextThread(GetCurrentThread(), &ctx);
    ok(status == STATUS_SUCCESS, "NtSetContextThread failed with %lx\n", status);

    thread = CreateThread(NULL, 0, register_check_thread, NULL, CREATE_SUSPENDED, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "CreateThread failed with %ld\n", GetLastError());

    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    status = pNtGetContextThread(thread, &ctx);
    ok(status == STATUS_SUCCESS, "NtGetContextThread failed with %lx\n", status);
    ok(!ctx.Dr0, "expected 0, got %Ix\n", (DWORD_PTR)ctx.Dr0);
    ok(!ctx.Dr1, "expected 0, got %Ix\n", (DWORD_PTR)ctx.Dr1);
    ok(!ctx.Dr2, "expected 0, got %Ix\n", (DWORD_PTR)ctx.Dr2);
    ok(!ctx.Dr3, "expected 0, got %Ix\n", (DWORD_PTR)ctx.Dr3);
    ok(!ctx.Dr6, "expected 0, got %Ix\n", (DWORD_PTR)ctx.Dr6);
    ok(!ctx.Dr7, "expected 0, got %Ix\n", (DWORD_PTR)ctx.Dr7);

    ResumeThread(thread);
    WaitForSingleObject(thread, 10000);
    CloseHandle(thread);
}

#if defined(__x86_64__)

static void test_debug_registers_wow64(void)
{
#ifdef __REACTOS__
    char cmdline[MAX_PATH];
#else
    char cmdline[] = "C:\\windows\\syswow64\\msinfo32.exe";
#endif
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {0};
    WOW64_CONTEXT wow64_ctx;
    CONTEXT ctx;
    BOOL is_wow64;
    NTSTATUS ret;
    BOOL bret;

#ifdef __REACTOS__
    if ((pRtlWow64GetThreadContext == NULL) ||
        (pRtlWow64SetThreadContext == NULL))
    {
        skip("RtlWow64Get/SetThreadContext not found\n");
        return;
    }

    GetWindowsDirectoryA(cmdline, sizeof(cmdline));
    strcat(cmdline, "\\syswow64\\msinfo32.exe");
#endif

    si.cb = sizeof(si);
    bret = CreateProcessA(cmdline, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(bret, "CreateProcessA failed\n");

    bret = pIsWow64Process(pi.hProcess, &is_wow64);
    ok(bret && is_wow64, "expected Wow64 process\n");

    SuspendThread(pi.hThread);

    ZeroMemory(&ctx, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_ALL;
    bret = GetThreadContext(pi.hThread, &ctx);
    ok(bret, "GetThreadContext failed\n");

    ctx.Dr0 = 0x12340000;
    ctx.Dr1 = 0x12340001;
    ctx.Dr2 = 0x12340002;
    ctx.Dr3 = 0x12340003;
    ctx.Dr7 = 0x155; /* enable all breakpoints (local) */
    bret = SetThreadContext(pi.hThread, &ctx);
    ok(bret, "SetThreadContext failed\n");

    if (bret) {
        memset(&ctx, 0xcc, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_ALL;
        bret = GetThreadContext(pi.hThread, &ctx);
        ok(bret, "GetThreadContext failed\n");
        if (bret)
        {
            if (is_arm64ec)
            {
                ok(!ctx.Dr0, "expected 0, got %Ix\n", ctx.Dr0);
                ok(!ctx.Dr1, "expected 0, got %Ix\n", ctx.Dr1);
                ok(!ctx.Dr2, "expected 0, got %Ix\n", ctx.Dr2);
                ok(!ctx.Dr3, "expected 0, got %Ix\n", ctx.Dr3);
                ok(!ctx.Dr7, "expected 0, got %Ix\n", ctx.Dr7);
            }
            else
            {
                ok(ctx.Dr0 == 0x12340000, "expected 0x12340000, got %Ix\n", ctx.Dr0);
                ok(ctx.Dr1 == 0x12340001, "expected 0x12340001, got %Ix\n", ctx.Dr1);
                ok(ctx.Dr2 == 0x12340002, "expected 0x12340002, got %Ix\n", ctx.Dr2);
                ok(ctx.Dr3 == 0x12340003, "expected 0x12340003, got %Ix\n", ctx.Dr3);
                ok(ctx.Dr7 == 0x155, "expected 0x155, got %Ix\n", ctx.Dr7);
            }
        }

        memset(&wow64_ctx, 0xcc, sizeof(wow64_ctx));
        wow64_ctx.ContextFlags = WOW64_CONTEXT_ALL;
        ret = pRtlWow64GetThreadContext(pi.hThread, &wow64_ctx);
        ok(ret == STATUS_SUCCESS, "Wow64GetThreadContext failed with %lx\n", ret);
        if (ret == STATUS_SUCCESS)
        {
            if (is_arm64ec)
            {
                ok(!wow64_ctx.Dr0, "expected 0, got %lx\n", wow64_ctx.Dr0);
                ok(!wow64_ctx.Dr1, "expected 0, got %lx\n", wow64_ctx.Dr1);
                ok(!wow64_ctx.Dr2, "expected 0, got %lx\n", wow64_ctx.Dr2);
                ok(!wow64_ctx.Dr3, "expected 0, got %lx\n", wow64_ctx.Dr3);
                ok(!wow64_ctx.Dr7, "expected 0, got %lx\n", wow64_ctx.Dr7);
            }
            else
            {
                ok(wow64_ctx.Dr0 == 0x12340000, "expected 0x12340000, got %lx\n", wow64_ctx.Dr0);
                ok(wow64_ctx.Dr1 == 0x12340001, "expected 0x12340001, got %lx\n", wow64_ctx.Dr1);
                ok(wow64_ctx.Dr2 == 0x12340002, "expected 0x12340002, got %lx\n", wow64_ctx.Dr2);
                ok(wow64_ctx.Dr3 == 0x12340003, "expected 0x12340003, got %lx\n", wow64_ctx.Dr3);
                ok(wow64_ctx.Dr7 == 0x155, "expected 0x155, got %lx\n", wow64_ctx.Dr7);
            }
        }
    }

    wow64_ctx.Dr0 = 0x56780000;
    wow64_ctx.Dr1 = 0x56780001;
    wow64_ctx.Dr2 = 0x56780002;
    wow64_ctx.Dr3 = 0x56780003;
    wow64_ctx.Dr7 = 0x101; /* enable only the first breakpoint */
    ret = pRtlWow64SetThreadContext(pi.hThread, &wow64_ctx);
    ok(ret == STATUS_SUCCESS, "Wow64SetThreadContext failed with %lx\n", ret);

    memset(&wow64_ctx, 0xcc, sizeof(wow64_ctx));
    wow64_ctx.ContextFlags = WOW64_CONTEXT_ALL;
    ret = pRtlWow64GetThreadContext(pi.hThread, &wow64_ctx);
    ok(ret == STATUS_SUCCESS, "Wow64GetThreadContext failed with %lx\n", ret);
    if (ret == STATUS_SUCCESS)
    {
        ok(wow64_ctx.Dr0 == 0x56780000, "expected 0x56780000, got %lx\n", wow64_ctx.Dr0);
        ok(wow64_ctx.Dr1 == 0x56780001, "expected 0x56780001, got %lx\n", wow64_ctx.Dr1);
        ok(wow64_ctx.Dr2 == 0x56780002, "expected 0x56780002, got %lx\n", wow64_ctx.Dr2);
        ok(wow64_ctx.Dr3 == 0x56780003, "expected 0x56780003, got %lx\n", wow64_ctx.Dr3);
        ok(wow64_ctx.Dr7 == 0x101, "expected 0x101, got %lx\n", wow64_ctx.Dr7);
    }

    memset(&ctx, 0xcc, sizeof(ctx));
    ctx.ContextFlags = CONTEXT_ALL;
    bret = GetThreadContext(pi.hThread, &ctx);
    ok(bret, "GetThreadContext failed\n");
    if (bret)
    {
        if (is_arm64ec)
        {
            ok(!ctx.Dr0, "expected 0, got %Ix\n", ctx.Dr0);
            ok(!ctx.Dr1, "expected 0, got %Ix\n", ctx.Dr1);
            ok(!ctx.Dr2, "expected 0, got %Ix\n", ctx.Dr2);
            ok(!ctx.Dr3, "expected 0, got %Ix\n", ctx.Dr3);
            ok(!ctx.Dr7, "expected 0, got %Ix\n", ctx.Dr7);
        }
        else
        {
            ok(ctx.Dr0 == 0x56780000, "expected 0x56780000, got %Ix\n", ctx.Dr0);
            ok(ctx.Dr1 == 0x56780001, "expected 0x56780001, got %Ix\n", ctx.Dr1);
            ok(ctx.Dr2 == 0x56780002, "expected 0x56780002, got %Ix\n", ctx.Dr2);
            ok(ctx.Dr3 == 0x56780003, "expected 0x56780003, got %Ix\n", ctx.Dr3);
            ok(ctx.Dr7 == 0x101, "expected 0x101, got %Ix\n", ctx.Dr7);
        }
    }

    ResumeThread(pi.hThread);
    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

#endif

static DWORD debug_service_exceptions;

static LONG CALLBACK debug_service_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    EXCEPTION_RECORD *rec = ExceptionInfo->ExceptionRecord;

    ok(rec->ExceptionCode == EXCEPTION_BREAKPOINT, "ExceptionCode is %08lx instead of %08lx\n",
       rec->ExceptionCode, EXCEPTION_BREAKPOINT);

#ifdef __i386__
    ok(ExceptionInfo->ContextRecord->Eip == (DWORD)code_mem + 0x1c,
       "expected Eip = %lx, got %lx\n", (DWORD)code_mem + 0x1c, ExceptionInfo->ContextRecord->Eip);
    ok(rec->NumberParameters == (is_wow64 ? 1 : 3),
       "ExceptionParameters is %ld instead of %d\n", rec->NumberParameters, is_wow64 ? 1 : 3);
    ok(rec->ExceptionInformation[0] == ExceptionInfo->ContextRecord->Eax,
       "expected ExceptionInformation[0] = %lx, got %Ix\n",
       ExceptionInfo->ContextRecord->Eax, rec->ExceptionInformation[0]);
    if (!is_wow64)
    {
        ok(rec->ExceptionInformation[1] == 0x11111111,
           "got ExceptionInformation[1] = %Ix\n", rec->ExceptionInformation[1]);
        ok(rec->ExceptionInformation[2] == 0x22222222,
           "got ExceptionInformation[2] = %Ix\n", rec->ExceptionInformation[2]);
    }
#else
    ok(ExceptionInfo->ContextRecord->Rip == (DWORD_PTR)code_mem + 0x2f,
       "expected Rip = %Ix, got %Ix\n", (DWORD_PTR)code_mem + 0x2f, ExceptionInfo->ContextRecord->Rip);
    ok(rec->NumberParameters == 1,
       "ExceptionParameters is %ld instead of 1\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == ExceptionInfo->ContextRecord->Rax,
       "expected ExceptionInformation[0] = %Ix, got %Ix\n",
       ExceptionInfo->ContextRecord->Rax, rec->ExceptionInformation[0]);
#endif

    debug_service_exceptions++;
    return (rec->ExceptionCode == EXCEPTION_BREAKPOINT) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

#ifdef __i386__

static const BYTE call_debug_service_code[] = {
    0x53,                         /* pushl %ebx */
    0x57,                         /* pushl %edi */
    0x8b, 0x44, 0x24, 0x0c,       /* movl 12(%esp),%eax */
    0xb9, 0x11, 0x11, 0x11, 0x11, /* movl $0x11111111,%ecx */
    0xba, 0x22, 0x22, 0x22, 0x22, /* movl $0x22222222,%edx */
    0xbb, 0x33, 0x33, 0x33, 0x33, /* movl $0x33333333,%ebx */
    0xbf, 0x44, 0x44, 0x44, 0x44, /* movl $0x44444444,%edi */
    0xcd, 0x2d,                   /* int $0x2d */
    0xeb,                         /* jmp $+17 */
    0x0f, 0x1f, 0x00,             /* nop */
    0x31, 0xc0,                   /* xorl %eax,%eax */
    0xeb, 0x0c,                   /* jmp $+14 */
    0x90, 0x90, 0x90, 0x90,       /* nop */
    0x90, 0x90, 0x90, 0x90,
    0x90,
    0x31, 0xc0,                   /* xorl %eax,%eax */
    0x40,                         /* incl %eax */
    0x5f,                         /* popl %edi */
    0x5b,                         /* popl %ebx */
    0xc3,                         /* ret */
};

#else

static const BYTE call_debug_service_code[] = {
    0x53,                         /* push %rbx */
    0x57,                         /* push %rdi */
    0x48, 0x89, 0xc8,             /* movl %rcx,%rax */
    0x48, 0xb9, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, /* movabs $0x1111111111111111,%rcx */
    0x48, 0xba, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, /* movabs $0x2222222222222222,%rdx */
    0x48, 0xbb, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, /* movabs $0x3333333333333333,%rbx */
    0x48, 0xbf, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, /* movabs $0x4444444444444444,%rdi */
    0xcd, 0x2d,                   /* int $0x2d */
    0xeb,                         /* jmp $+17 */
    0x0f, 0x1f, 0x00,             /* nop */
    0x48, 0x31, 0xc0,             /* xor %rax,%rax */
    0xeb, 0x0e,                   /* jmp $+16 */
    0x90, 0x90, 0x90, 0x90,       /* nop */
    0x90, 0x90, 0x90, 0x90,
    0x48, 0x31, 0xc0,             /* xor %rax,%rax */
    0x48, 0xff, 0xc0,             /* inc %rax */
    0x5f,                         /* pop %rdi */
    0x5b,                         /* pop %rbx */
    0xc3,                         /* ret */
};

#endif

static void test_debug_service(DWORD numexc)
{
    DWORD (CDECL *func)(DWORD_PTR) = code_mem;
    DWORD expected_exc, expected_ret;
    void *vectored_handler;
    DWORD ret;

    /* code will return 0 if execution resumes immediately after "int $0x2d", otherwise 1 */
    memcpy(code_mem, call_debug_service_code, sizeof(call_debug_service_code));

    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &debug_service_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    expected_exc = numexc;
    expected_ret = (numexc != 0);

    /* BREAKPOINT_BREAK */
    debug_service_exceptions = 0;
    ret = func(0);
    ok(debug_service_exceptions == expected_exc,
       "BREAKPOINT_BREAK generated %lu exceptions, expected %lu\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret,
       "BREAKPOINT_BREAK returned %lu, expected %lu\n", ret, expected_ret);

    /* BREAKPOINT_PROMPT */
    debug_service_exceptions = 0;
    ret = func(2);
    ok(debug_service_exceptions == expected_exc,
       "BREAKPOINT_PROMPT generated %lu exceptions, expected %lu\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret,
       "BREAKPOINT_PROMPT returned %lu, expected %lu\n", ret, expected_ret);

    /* invalid debug service */
    debug_service_exceptions = 0;
    ret = func(6);
    ok(debug_service_exceptions == expected_exc,
       "invalid debug service generated %lu exceptions, expected %lu\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret,
      "invalid debug service returned %lu, expected %lu\n", ret, expected_ret);

    expected_exc = (is_wow64 ? numexc : 0);
    expected_ret = (is_wow64 && numexc);

    /* BREAKPOINT_PRINT */
    debug_service_exceptions = 0;
    ret = func(1);
    ok(debug_service_exceptions == expected_exc,
       "BREAKPOINT_PRINT generated %lu exceptions, expected %lu\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret,
       "BREAKPOINT_PRINT returned %lu, expected %lu\n", ret, expected_ret);

    /* BREAKPOINT_LOAD_SYMBOLS */
    debug_service_exceptions = 0;
    ret = func(3);
    ok(debug_service_exceptions == expected_exc,
       "BREAKPOINT_LOAD_SYMBOLS generated %lu exceptions, expected %lu\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret,
       "BREAKPOINT_LOAD_SYMBOLS returned %lu, expected %lu\n", ret, expected_ret);

    /* BREAKPOINT_UNLOAD_SYMBOLS */
    debug_service_exceptions = 0;
    ret = func(4);
    ok(debug_service_exceptions == expected_exc,
       "BREAKPOINT_UNLOAD_SYMBOLS generated %lu exceptions, expected %lu\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret,
       "BREAKPOINT_UNLOAD_SYMBOLS returned %lu, expected %lu\n", ret, expected_ret);

    /* BREAKPOINT_COMMAND_STRING */
    debug_service_exceptions = 0;
    ret = func(5);
    ok(debug_service_exceptions == expected_exc || broken(debug_service_exceptions == numexc),
       "BREAKPOINT_COMMAND_STRING generated %lu exceptions, expected %lu\n",
       debug_service_exceptions, expected_exc);
    ok(ret == expected_ret || broken(ret == (numexc != 0)),
       "BREAKPOINT_COMMAND_STRING returned %lu, expected %lu\n", ret, expected_ret);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}
#endif /* defined(__i386__) || defined(__x86_64__) */

static DWORD outputdebugstring_exceptions_ansi;
static DWORD outputdebugstring_exceptions_unicode;

static LONG CALLBACK outputdebugstring_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;

    switch (rec->ExceptionCode)
    {
    case DBG_PRINTEXCEPTION_C:
        ok(rec->NumberParameters == 2, "ExceptionParameters is %ld instead of 2\n", rec->NumberParameters);
        ok(rec->ExceptionInformation[0] == 12, "ExceptionInformation[0] = %ld instead of 12\n", (DWORD)rec->ExceptionInformation[0]);
        ok(!strcmp((char *)rec->ExceptionInformation[1], "Hello World"),
           "ExceptionInformation[1] = '%s' instead of 'Hello World'\n", (char *)rec->ExceptionInformation[1]);
        outputdebugstring_exceptions_ansi++;
        break;
    case DBG_PRINTEXCEPTION_WIDE_C:
        ok(outputdebugstring_exceptions_ansi == 0, "Unicode exception should come first\n");
        ok(rec->NumberParameters == 4, "ExceptionParameters is %ld instead of 4\n", rec->NumberParameters);
        ok(rec->ExceptionInformation[0] == 12, "ExceptionInformation[0] = %ld instead of 12\n", (DWORD)rec->ExceptionInformation[0]);
        ok(!wcscmp((WCHAR *)rec->ExceptionInformation[1], L"Hello World"),
           "ExceptionInformation[1] = '%s' instead of 'Hello World'\n", (char *)rec->ExceptionInformation[1]);
        ok(rec->ExceptionInformation[2] == 12, "ExceptionInformation[2] = %ld instead of 12\n", (DWORD)rec->ExceptionInformation[2]);
        ok(!strcmp((char *)rec->ExceptionInformation[3], "Hello World"),
           "ExceptionInformation[3] = '%s' instead of 'Hello World'\n", (char *)rec->ExceptionInformation[3]);
        outputdebugstring_exceptions_unicode++;
        break;
    default:
        ok(0, "ExceptionCode is %08lx unexpected\n", rec->ExceptionCode);
        break;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

static void test_outputdebugstring(BOOL unicode, DWORD numexc_ansi, BOOL todo_ansi,
                                   DWORD numexc_unicode_low, DWORD numexc_unicode_high)
{
    PVOID vectored_handler;

    if (!pRtlAddVectoredExceptionHandler || !pRtlRemoveVectoredExceptionHandler)
    {
        skip("RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler not found\n");
        return;
    }

    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &outputdebugstring_vectored_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    outputdebugstring_exceptions_ansi = outputdebugstring_exceptions_unicode = 0;

    if (unicode)
        OutputDebugStringW(L"Hello World");
    else
        OutputDebugStringA("Hello World");

    todo_wine_if(todo_ansi)
    ok(outputdebugstring_exceptions_ansi == numexc_ansi,
       "OutputDebugString%c generated %ld ansi exceptions, expected %ld\n",
       unicode ? 'W' : 'A', outputdebugstring_exceptions_ansi, numexc_ansi);
    ok(outputdebugstring_exceptions_unicode >= numexc_unicode_low &&
       outputdebugstring_exceptions_unicode <= numexc_unicode_high,
       "OutputDebugString%c generated %lu unicode exceptions, expected %ld-%ld\n",
       unicode ? 'W' : 'A', outputdebugstring_exceptions_unicode, numexc_unicode_low, numexc_unicode_high);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}

static DWORD outputdebugstring_exceptions_newmodel_order;
static DWORD outputdebugstring_newmodel_return;

static LONG CALLBACK outputdebugstring_new_model_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;

    switch (rec->ExceptionCode)
    {
    case DBG_PRINTEXCEPTION_C:
        ok(rec->NumberParameters == 2, "ExceptionParameters is %ld instead of 2\n", rec->NumberParameters);
        ok(rec->ExceptionInformation[0] == 12, "ExceptionInformation[0] = %ld instead of 12\n", (DWORD)rec->ExceptionInformation[0]);
        ok(!strcmp((char *)rec->ExceptionInformation[1], "Hello World"),
           "ExceptionInformation[1] = '%s' instead of 'Hello World'\n", (char *)rec->ExceptionInformation[1]);
        outputdebugstring_exceptions_newmodel_order =
            (outputdebugstring_exceptions_newmodel_order << 8) | 'A';
        break;
    case DBG_PRINTEXCEPTION_WIDE_C:
        ok(rec->NumberParameters == 4, "ExceptionParameters is %ld instead of 4\n", rec->NumberParameters);
        ok(rec->ExceptionInformation[0] == 12, "ExceptionInformation[0] = %ld instead of 12\n", (DWORD)rec->ExceptionInformation[0]);
        ok(!wcscmp((WCHAR *)rec->ExceptionInformation[1], L"Hello World"),
           "ExceptionInformation[1] = '%s' instead of 'Hello World'\n", (char *)rec->ExceptionInformation[1]);
        ok(rec->ExceptionInformation[2] == 12, "ExceptionInformation[2] = %ld instead of 12\n", (DWORD)rec->ExceptionInformation[2]);
        ok(!strcmp((char *)rec->ExceptionInformation[3], "Hello World"),
           "ExceptionInformation[3] = '%s' instead of 'Hello World'\n", (char *)rec->ExceptionInformation[3]);
        outputdebugstring_exceptions_newmodel_order =
            (outputdebugstring_exceptions_newmodel_order << 8) | 'W';
        break;
    default:
        ok(0, "ExceptionCode is %08lx unexpected\n", rec->ExceptionCode);
        break;
    }

    return outputdebugstring_newmodel_return;
}

static void test_outputdebugstring_newmodel(void)
{
    PVOID vectored_handler;
    struct
    {
        /* input */
        BOOL unicode;
        DWORD ret_code;
        /* expected output */
        DWORD exceptions_order;
    }
    tests[] =
    {
        {FALSE, EXCEPTION_CONTINUE_EXECUTION, 'A'},
        {FALSE, EXCEPTION_CONTINUE_SEARCH,    'A'},
        {TRUE,  EXCEPTION_CONTINUE_EXECUTION, 'W'},
        {TRUE,  EXCEPTION_CONTINUE_SEARCH,    ('W' << 8) | 'A'},
    };
    int i;

    if (!pRtlAddVectoredExceptionHandler || !pRtlRemoveVectoredExceptionHandler)
    {
        skip("RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler not found\n");
        return;
    }

    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &outputdebugstring_new_model_vectored_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        outputdebugstring_exceptions_newmodel_order = 0;
        outputdebugstring_newmodel_return = tests[i].ret_code;

        if (tests[i].unicode)
            OutputDebugStringW(L"Hello World");
        else
            OutputDebugStringA("Hello World");

        ok(outputdebugstring_exceptions_newmodel_order == tests[i].exceptions_order,
           "OutputDebugString%c/%u generated exceptions %04lxs, expected %04lx\n",
           tests[i].unicode ? 'W' : 'A', i,
           outputdebugstring_exceptions_newmodel_order, tests[i].exceptions_order);
    }

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}

static DWORD ripevent_exceptions;

static LONG CALLBACK ripevent_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;

    ok(rec->ExceptionCode == DBG_RIPEXCEPTION, "ExceptionCode is %08lx instead of %08lx\n",
       rec->ExceptionCode, DBG_RIPEXCEPTION);
    ok(rec->NumberParameters == 2, "ExceptionParameters is %ld instead of 2\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == 0x11223344, "ExceptionInformation[0] = %08lx instead of %08x\n",
       (NTSTATUS)rec->ExceptionInformation[0], 0x11223344);
    ok(rec->ExceptionInformation[1] == 0x55667788, "ExceptionInformation[1] = %08lx instead of %08x\n",
       (NTSTATUS)rec->ExceptionInformation[1], 0x55667788);

    ripevent_exceptions++;
    return (rec->ExceptionCode == DBG_RIPEXCEPTION) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

static void test_ripevent(DWORD numexc)
{
    EXCEPTION_RECORD record;
    PVOID vectored_handler;

    if (!pRtlAddVectoredExceptionHandler || !pRtlRemoveVectoredExceptionHandler || !pRtlRaiseException)
    {
        skip("RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler or RtlRaiseException not found\n");
        return;
    }

    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &ripevent_vectored_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    record.ExceptionCode = DBG_RIPEXCEPTION;
    record.ExceptionFlags = 0;
    record.ExceptionRecord = NULL;
    record.ExceptionAddress = NULL;
    record.NumberParameters = 2;
    record.ExceptionInformation[0] = 0x11223344;
    record.ExceptionInformation[1] = 0x55667788;

    ripevent_exceptions = 0;
    pRtlRaiseException(&record);
    ok(ripevent_exceptions == numexc, "RtlRaiseException generated %ld exceptions, expected %ld\n",
       ripevent_exceptions, numexc);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}

static void subtest_fastfail(unsigned int code)
{
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    DEBUG_EVENT de;
    DWORD continuestatus;
    BOOL ret;
    BOOL had_ff = FALSE, had_se = FALSE;

    sprintf(cmdline, "%s %s %s %u", my_argv[0], my_argv[1], "fastfail", code);
    si.cb = sizeof(si);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "could not create child process error: %lu\n", GetLastError());
    if (!ret)
        return;

    do
    {
        continuestatus = DBG_CONTINUE;
        ok(WaitForDebugEvent(&de, INFINITE), "reading debug event\n");

        if (de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            if (de.u.Exception.ExceptionRecord.ExceptionCode == STATUS_STACK_BUFFER_OVERRUN)
            {
                ok(!de.u.Exception.dwFirstChance, "must be a second chance exception\n");
                ok(de.u.Exception.ExceptionRecord.NumberParameters == 1 || broken(is_arm64ec),
                   "expected exactly one parameter, got %lu\n",
                   de.u.Exception.ExceptionRecord.NumberParameters);
                if (de.u.Exception.ExceptionRecord.NumberParameters >= 1)
                    ok(de.u.Exception.ExceptionRecord.ExceptionInformation[0] == code,
                       "expected %u for code, got %Iu\n",
                       code, de.u.Exception.ExceptionRecord.ExceptionInformation[0]);
                had_ff = TRUE;
            }

            if (de.u.Exception.dwFirstChance)
            {
                continuestatus = DBG_EXCEPTION_NOT_HANDLED;
            }
            else
            {
                had_se = TRUE;
                pNtTerminateProcess(pi.hProcess, 0);
            }
        }

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continuestatus);

    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    ok(had_ff || broken(had_se) /* Win7 */, "fast fail did not occur\n");

    wait_child_process( pi.hProcess );
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %lu\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "error %lu\n", GetLastError());

    return;
}

static void test_fastfail(void)
{
    unsigned int codes[] = {
        FAST_FAIL_LEGACY_GS_VIOLATION,
        FAST_FAIL_VTGUARD_CHECK_FAILURE,
        FAST_FAIL_STACK_COOKIE_CHECK_FAILURE,
        FAST_FAIL_CORRUPT_LIST_ENTRY,
        FAST_FAIL_INCORRECT_STACK,
        FAST_FAIL_INVALID_ARG,
        FAST_FAIL_GS_COOKIE_INIT,
        FAST_FAIL_FATAL_APP_EXIT,
        FAST_FAIL_INVALID_FAST_FAIL_CODE,
        0xdeadbeefUL,
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(codes); i++)
    {
        winetest_push_context("__fastfail(%#x)", codes[i]);
        subtest_fastfail(codes[i]);
        winetest_pop_context();
    }
}

static DWORD breakpoint_exceptions;

static LONG CALLBACK breakpoint_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    EXCEPTION_RECORD *rec = ExceptionInfo->ExceptionRecord;

    ok(rec->ExceptionCode == EXCEPTION_BREAKPOINT, "ExceptionCode is %08lx instead of %08lx\n",
       rec->ExceptionCode, EXCEPTION_BREAKPOINT);

#ifdef __i386__
    ok(ExceptionInfo->ContextRecord->Eip == (DWORD)code_mem + 1,
       "expected Eip = %lx, got %lx\n", (DWORD)code_mem + 1, ExceptionInfo->ContextRecord->Eip);
    ok(rec->NumberParameters == (is_wow64 ? 1 : 3),
       "ExceptionParameters is %ld instead of %d\n", rec->NumberParameters, is_wow64 ? 1 : 3);
    ok(rec->ExceptionInformation[0] == 0,
       "got ExceptionInformation[0] = %Ix\n", rec->ExceptionInformation[0]);
    ExceptionInfo->ContextRecord->Eip = (DWORD)code_mem + 2;
#elif defined(__x86_64__)
    ok(ExceptionInfo->ContextRecord->Rip == (DWORD_PTR)code_mem + 1,
       "expected Rip = %Ix, got %Ix\n", (DWORD_PTR)code_mem + 1, ExceptionInfo->ContextRecord->Rip);
    ok(rec->NumberParameters == 1,
       "ExceptionParameters is %ld instead of 1\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == 0,
       "got ExceptionInformation[0] = %Ix\n", rec->ExceptionInformation[0]);
    ExceptionInfo->ContextRecord->Rip = (DWORD_PTR)code_mem + 2;
#elif defined(__arm__)
    ok(ExceptionInfo->ContextRecord->Pc == (DWORD)code_mem + 1,
       "expected pc = %lx, got %lx\n", (DWORD)code_mem + 1, ExceptionInfo->ContextRecord->Pc);
    ok(rec->NumberParameters == 1,
       "ExceptionParameters is %ld instead of 1\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == 0,
       "got ExceptionInformation[0] = %Ix\n", rec->ExceptionInformation[0]);
    ExceptionInfo->ContextRecord->Pc += 2;
#elif defined(__aarch64__)
    ok(ExceptionInfo->ContextRecord->Pc == (DWORD_PTR)code_mem,
       "expected pc = %p, got %p\n", code_mem, (void *)ExceptionInfo->ContextRecord->Pc);
    ok(rec->NumberParameters == 1,
       "ExceptionParameters is %ld instead of 1\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == 0,
       "got ExceptionInformation[0] = %p\n", (void *)rec->ExceptionInformation[0]);
    ExceptionInfo->ContextRecord->Pc += 4;
#endif

    breakpoint_exceptions++;
    return (rec->ExceptionCode == EXCEPTION_BREAKPOINT) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

#if defined(__i386__) || defined(__x86_64__)
static const BYTE breakpoint_code[] = { 0xcd, 0x03, 0xc3 };   /* int $0x3; ret */
#elif defined(__arm__)
static const DWORD breakpoint_code[] = { 0xdefe, 0x4770 };  /* udf #0xfe; bx lr */
#elif defined(__aarch64__)
static const DWORD breakpoint_code[] = { 0xd43e0000, 0xd65f03c0 };  /* brk #0xf000; ret */
#endif

static void test_breakpoint(DWORD numexc)
{
    DWORD (CDECL *func)(void) = code_mem;
    void *vectored_handler;

#if defined(__REACTOS__)
    if (is_reactos())
    {
        skip("Skipping tests that crash\n");
        return;
    }
#endif

    memcpy(code_mem, breakpoint_code, sizeof(breakpoint_code));
#ifdef __arm__
    func = (void *)((char *)code_mem + 1);  /* thumb */
#endif
    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &breakpoint_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    breakpoint_exceptions = 0;
    func();
    ok(breakpoint_exceptions == numexc, "int $0x3 generated %lu exceptions, expected %lu\n",
       breakpoint_exceptions, numexc);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}

#if defined(__i386__) || defined(__x86_64__)
static BYTE except_code_set_ymm0[] =
{
#ifdef __x86_64__
    0x48,
#endif
    0xb8,                         /* mov imm,%ax */
    0x00, 0x00, 0x00, 0x00,
#ifdef __x86_64__
    0x00, 0x00, 0x00, 0x00,
#endif

    0xc5, 0xfc, 0x10, 0x00,       /* vmovups (%ax),%ymm0 */
    0xcc,                         /* int3 */
    0xc5, 0xfc, 0x11, 0x00,       /* vmovups %ymm0,(%ax) */
    0xc3,                         /* ret  */
};

static void test_debuggee_xstate(void)
{
    void (CDECL *func)(void) = code_mem;
    unsigned int address_offset, i;
    unsigned int data[8];

    if (!pRtlGetEnabledExtendedFeatures || !pRtlGetEnabledExtendedFeatures(1 << XSTATE_AVX))
    {
        memcpy(code_mem, breakpoint_code, sizeof(breakpoint_code));
        func();
        return;
    }

    memcpy(code_mem, except_code_set_ymm0, sizeof(except_code_set_ymm0));
    address_offset = sizeof(void *) == 8 ? 2 : 1;
    *(void **)((BYTE *)code_mem + address_offset) = data;

    for (i = 0; i < ARRAY_SIZE(data); ++i)
        data[i] = i + 1;

    func();

    for (i = 0; i < 4; ++i)
        ok(data[i] == (test_stage == STAGE_XSTATE ? i + 1 : 0x28282828),
                "Got unexpected data %#x, test_stage %u, i %u.\n", data[i], test_stage, i);

    for (     ; i < ARRAY_SIZE(data); ++i)
        ok(data[i] == (test_stage == STAGE_XSTATE ? i + 1 : 0x48484848)
                || broken(test_stage == STAGE_XSTATE_LEGACY_SSE && data[i] == i + 1) /* Win7 */,
                "Got unexpected data %#x, test_stage %u, i %u.\n", data[i], test_stage, i);
}

static BYTE except_code_segments[] =
{
    0x8c, 0xc0, /* mov %es,%eax */
    0x50,       /* push %rax */
    0x8c, 0xd8, /* mov %ds,%eax */
    0x50,       /* push %rax */
    0x8c, 0xe0, /* mov %fs,%eax */
    0x50,       /* push %rax */
    0x8c, 0xe8, /* mov %gs,%eax */
    0x50,       /* push %rax */
    0x31, 0xc0, /* xor %eax,%eax */
    0x8e, 0xc0, /* mov %eax,%es */
    0x8e, 0xd8, /* mov %eax,%ds */
    0x8e, 0xe0, /* mov %eax,%fs */
    0x8e, 0xe8, /* mov %eax,%gs */
    0xcc,       /* int3 */
    0x58,       /* pop %rax */
    0x8e, 0xe8, /* mov %eax,%gs */
    0x58,       /* pop %rax */
    0x8e, 0xe0, /* mov %eax,%fs */
    0x58,       /* pop %rax */
    0x8e, 0xd8, /* mov %eax,%ds */
    0x58,       /* pop %rax */
    0x8e, 0xc0, /* mov %eax,%es */
    0xc3,       /* retq */
};

static void test_debuggee_segments(void)
{
    void (CDECL *func)(void) = code_mem;

    memcpy( code_mem, except_code_segments, sizeof(except_code_segments));
    func();
}
#endif

static DWORD invalid_handle_exceptions;

static LONG CALLBACK invalid_handle_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;

    ok(rec->ExceptionCode == EXCEPTION_INVALID_HANDLE, "ExceptionCode is %08lx instead of %08lx\n",
       rec->ExceptionCode, EXCEPTION_INVALID_HANDLE);
    ok(rec->NumberParameters == 0, "ExceptionParameters is %ld instead of 0\n", rec->NumberParameters);

    invalid_handle_exceptions++;
    return (rec->ExceptionCode == EXCEPTION_INVALID_HANDLE) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

static inline BOOL is_magic_handle(HANDLE handle)
{
    return HandleToLong(handle) >= ~5 && HandleToLong(handle) <= ~0;
}

static void test_closehandle(DWORD numexc, HANDLE handle)
{
    NTSTATUS status, expect;
    PVOID vectored_handler;
    BOOL ret, expectret;

    if (!pRtlAddVectoredExceptionHandler || !pRtlRemoveVectoredExceptionHandler || !pRtlRaiseException)
    {
        skip("RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler or RtlRaiseException not found\n");
        return;
    }

    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &invalid_handle_vectored_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    invalid_handle_exceptions = 0;
    expectret = is_magic_handle(handle) || broken(numexc && sizeof(handle) == 4); /* < Win10 */
    ret = CloseHandle(handle);
    ok(ret || (GetLastError() == ERROR_INVALID_HANDLE),
        "CloseHandle had wrong GetLastError(), got %lu for %p\n", GetLastError(), handle);
    ok(ret == expectret || broken(HandleToLong(handle) < 0) /* < Win10 */,
        "CloseHandle expected %d, got %d for %p\n", expectret, ret, handle);
    ok(invalid_handle_exceptions == numexc || broken(!numexc && is_magic_handle(handle)), /* < Win10 */
        "CloseHandle generated %ld exceptions, expected %ld for %p\n",
       invalid_handle_exceptions, numexc, handle);

    invalid_handle_exceptions = 0;
    expect = expectret ? STATUS_SUCCESS : STATUS_INVALID_HANDLE;
    status = pNtClose(handle);
    ok(status == expect || broken(HandleToLong(handle) < 0), /* < Win10 */
        "NtClose returned unexpected status %#lx, expected %#lx for %p\n", status, expect, handle);
    ok(invalid_handle_exceptions == numexc || broken(!numexc && is_magic_handle(handle)), /* < Win10 */
        "CloseHandle generated %ld exceptions, expected %ld for %p\n",
       invalid_handle_exceptions, numexc, handle);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}

static void test_vectored_continue_handler(void)
{
    PVOID handler1, handler2;
    ULONG ret;

    if (!pRtlAddVectoredContinueHandler || !pRtlRemoveVectoredContinueHandler)
    {
        skip("RtlAddVectoredContinueHandler or RtlRemoveVectoredContinueHandler not found\n");
        return;
    }

    handler1 = pRtlAddVectoredContinueHandler(TRUE, (void *)0xdeadbeef);
    ok(handler1 != 0, "RtlAddVectoredContinueHandler failed\n");

    handler2 = pRtlAddVectoredContinueHandler(TRUE, (void *)0xdeadbeef);
    ok(handler2 != 0, "RtlAddVectoredContinueHandler failed\n");
    ok(handler1 != handler2, "RtlAddVectoredContinueHandler returned same handler\n");

    if (pRtlRemoveVectoredExceptionHandler)
    {
        ret = pRtlRemoveVectoredExceptionHandler(handler1);
        ok(!ret, "RtlRemoveVectoredExceptionHandler succeeded\n");
    }

    ret = pRtlRemoveVectoredContinueHandler(handler1);
    ok(ret, "RtlRemoveVectoredContinueHandler failed\n");

    ret = pRtlRemoveVectoredContinueHandler(handler2);
    ok(ret, "RtlRemoveVectoredContinueHandler failed\n");

    ret = pRtlRemoveVectoredContinueHandler(handler1);
    ok(!ret, "RtlRemoveVectoredContinueHandler succeeded\n");

    ret = pRtlRemoveVectoredContinueHandler((void *)0x11223344);
    ok(!ret, "RtlRemoveVectoredContinueHandler succeeded\n");
}

static void test_user_apc(void)
{
    NTSTATUS status;
    CONTEXT context;
    LONG pass;
    int ret;

    if (!pNtQueueApcThread)
    {
        win_skip("NtQueueApcThread is not available.\n");
        return;
    }

    pass = 0;
    InterlockedIncrement(&pass);
#ifdef __i386__
    {
        /* RtlCaptureContext puts the return address of the caller's stack
         * frame into %eip, so we need a thunk to get it to return here */
        static const BYTE code[] =
        {
            0x55,               /* pushl %ebp */
            0x89, 0xe5,         /* movl %esp, %ebp */
            0xff, 0x75, 0x0c,   /* pushl 0xc(%ebp) */
            0xff, 0x55, 0x08,   /* call *0x8(%ebp) */
            0xc9,               /* leave */
            0xc3,               /* ret */
        };
        int (__cdecl *func)(void *capture, CONTEXT *context) = code_mem;

        memcpy(code_mem, code, sizeof(code));
        ret = func(RtlCaptureContext, &context);
        /* work around broken RtlCaptureContext on Windows < 7 which doesn't set
         * ContextFlags */
        context.ContextFlags = CONTEXT_FULL;
    }
#else
    {
        int (WINAPI *func)(CONTEXT *context) = (void *)RtlCaptureContext;

        ret = func(&context);
    }
#endif
    InterlockedIncrement(&pass);

    if (pass == 2)
    {
        /* Try to make sure context data is far enough below context.Esp. */
        CONTEXT c[4];

#ifdef __i386__
        context.Eax = 0xabacab;
#elif defined(__x86_64__)
        context.Rax = 0xabacab;
#elif defined(__arm__)
        context.R0 = 0xabacab;
#elif defined(__aarch64__)
        context.X0 = 0xabacab;
#endif

        c[0] = context;

        apc_count = 0;
        status = pNtQueueApcThread(GetCurrentThread(), apc_func, 0x1234, 0x5678, 0xdeadbeef);
        ok(!status, "Got unexpected status %#lx.\n", status);
        SleepEx(0, TRUE);
        ok(apc_count == 1, "Test user APC was not called.\n");
        apc_count = 0;
        status = pNtQueueApcThread(GetCurrentThread(), apc_func, 0x1234, 0x5678, 0xdeadbeef);
        ok(!status, "Got unexpected status %#lx.\n", status);
        status = NtContinue(&c[0], TRUE );

        ok(0, "Should not get here, status %#lx.\n", status);
        return;
    }
    ok(ret == 0xabacab, "Got return value %#x.\n", ret);
    ok(pass == 3, "Got unexpected pass %ld.\n", pass);
    ok(apc_count > 0, "Test user APC was not called.\n");
}

static void test_user_callback(void)
{
    NTSTATUS status = NtCallbackReturn( NULL, 0, STATUS_SUCCESS );
    ok( status == STATUS_NO_CALLBACK_ACTIVE, "failed %lx\n", status );
}

static DWORD WINAPI suspend_thread_test( void *arg )
{
    HANDLE event = arg;
    WaitForSingleObject(event, INFINITE);
    return 0;
}

static void test_suspend_count(HANDLE hthread, ULONG expected_count, int line)
{
    static BOOL supported = TRUE;
    NTSTATUS status;
    ULONG count;

    if (!supported)
        return;

    count = ~0u;
    status = pNtQueryInformationThread(hthread, ThreadSuspendCount, &count, sizeof(count), NULL);
    if (status)
    {
        win_skip("ThreadSuspendCount is not supported.\n");
        supported = FALSE;
        return;
    }

    ok_(__FILE__, line)(!status, "Failed to get suspend count, status %#lx.\n", status);
    ok_(__FILE__, line)(count == expected_count, "Unexpected suspend count %lu.\n", count);
}

static void test_suspend_thread(void)
{
#define TEST_SUSPEND_COUNT(thread, count) test_suspend_count((thread), (count), __LINE__)
    HANDLE thread, event;
    ULONG count, len;
    NTSTATUS status;
    DWORD ret;

    status = NtSuspendThread(0, NULL);
    ok(status == STATUS_INVALID_HANDLE, "Unexpected return value %#lx.\n", status);

    status = NtResumeThread(0, NULL);
    ok(status == STATUS_INVALID_HANDLE, "Unexpected return value %#lx.\n", status);

    event = CreateEventW(NULL, FALSE, FALSE, NULL);

    thread = CreateThread(NULL, 0, suspend_thread_test, event, 0, NULL);
    ok(thread != NULL, "Failed to create a thread.\n");

    ret = WaitForSingleObject(thread, 0);
    ok(ret == WAIT_TIMEOUT, "Unexpected status %ld.\n", ret);

    status = pNtQueryInformationThread(thread, ThreadSuspendCount, &count, sizeof(count), NULL);
    if (!status)
    {
        status = pNtQueryInformationThread(thread, ThreadSuspendCount, NULL, sizeof(count), NULL);
        ok(status == STATUS_ACCESS_VIOLATION, "Unexpected status %#lx.\n", status);

        status = pNtQueryInformationThread(thread, ThreadSuspendCount, &count, sizeof(count) / 2, NULL);
        ok(status == STATUS_INFO_LENGTH_MISMATCH, "Unexpected status %#lx.\n", status);

        len = 123;
        status = pNtQueryInformationThread(thread, ThreadSuspendCount, &count, sizeof(count) / 2, &len);
        ok(status == STATUS_INFO_LENGTH_MISMATCH, "Unexpected status %#lx.\n", status);
        ok(len == 123, "Unexpected info length %lu.\n", len);

        len = 123;
        status = pNtQueryInformationThread(thread, ThreadSuspendCount, NULL, 0, &len);
        ok(status == STATUS_INFO_LENGTH_MISMATCH, "Unexpected status %#lx.\n", status);
        ok(len == 123, "Unexpected info length %lu.\n", len);

        count = 10;
        status = pNtQueryInformationThread(0, ThreadSuspendCount, &count, sizeof(count), NULL);
        ok(status, "Unexpected status %#lx.\n", status);
        ok(count == 10, "Unexpected suspend count %lu.\n", count);
    }

    status = NtResumeThread(thread, NULL);
    ok(!status, "Unexpected status %#lx.\n", status);

    status = NtResumeThread(thread, &count);
    ok(!status, "Unexpected status %#lx.\n", status);
    ok(count == 0, "Unexpected suspended count %lu.\n", count);

    TEST_SUSPEND_COUNT(thread, 0);

    status = NtSuspendThread(thread, NULL);
    ok(!status, "Failed to suspend a thread, status %#lx.\n", status);

    TEST_SUSPEND_COUNT(thread, 1);

    status = NtSuspendThread(thread, &count);
    ok(!status, "Failed to suspend a thread, status %#lx.\n", status);
    ok(count == 1, "Unexpected suspended count %lu.\n", count);

    TEST_SUSPEND_COUNT(thread, 2);

    status = NtResumeThread(thread, &count);
    ok(!status, "Failed to resume a thread, status %#lx.\n", status);
    ok(count == 2, "Unexpected suspended count %lu.\n", count);

    TEST_SUSPEND_COUNT(thread, 1);

    status = NtResumeThread(thread, NULL);
    ok(!status, "Failed to resume a thread, status %#lx.\n", status);

    TEST_SUSPEND_COUNT(thread, 0);

    SetEvent(event);
    WaitForSingleObject(thread, INFINITE);

    CloseHandle(thread);
#undef TEST_SUSPEND_COUNT
}

static const char *suspend_process_event_name = "suspend_process_event";
static const char *suspend_process_event2_name = "suspend_process_event2";

static DWORD WINAPI dummy_thread_proc( void *arg )
{
    return 0;
}

static void suspend_process_proc(void)
{
    HANDLE event = OpenEventA(SYNCHRONIZE, FALSE, suspend_process_event_name);
    HANDLE event2 = OpenEventA(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, suspend_process_event2_name);
    DWORD count;
    NTSTATUS status;
    HANDLE thread;

    ok(event != NULL, "Failed to open event handle.\n");
    ok(event2 != NULL, "Failed to open event handle.\n");

    thread = CreateThread(NULL, 0, dummy_thread_proc, 0, CREATE_SUSPENDED, NULL);
    ok(thread != NULL, "Failed to create auxiliary thread.\n");

    /* Suspend up to limit. */
    while (!(status = NtSuspendThread(thread, NULL)))
        ;
    ok(status == STATUS_SUSPEND_COUNT_EXCEEDED, "Unexpected status %#lx.\n", status);

    for (;;)
    {
        SetEvent(event2);
        if (WaitForSingleObject(event, 100) == WAIT_OBJECT_0)
            break;
    }

    status = NtSuspendThread(thread, &count);
    ok(!status, "Failed to suspend a thread, status %#lx.\n", status);
    ok(count == 125, "Unexpected suspend count %lu.\n", count);

    status = NtResumeThread(thread, NULL);
    ok(!status, "Failed to resume a thread, status %#lx.\n", status);

    CloseHandle(event);
    CloseHandle(event2);
}

static void test_suspend_process(void)
{
    PROCESS_INFORMATION info;
    char path_name[MAX_PATH];
    STARTUPINFOA startup;
    HANDLE event, event2;
    NTSTATUS status;
    char **argv;
    DWORD ret;

    event = CreateEventA(NULL, FALSE, FALSE, suspend_process_event_name);
    ok(event != NULL, "Failed to create event.\n");

    event2 = CreateEventA(NULL, FALSE, FALSE, suspend_process_event2_name);
    ok(event2 != NULL, "Failed to create event.\n");

    winetest_get_mainargs(&argv);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    sprintf(path_name, "%s exception suspend_process", argv[0]);

    ret = CreateProcessA(NULL, path_name, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info);
    ok(ret, "Failed to create target process.\n");

    /* New process signals this event. */
    ResetEvent(event2);
    ret = WaitForSingleObject(event2, INFINITE);
    ok(ret == WAIT_OBJECT_0, "Wait failed, %#lx.\n", ret);

    /* Suspend main thread */
    status = NtSuspendThread(info.hThread, &ret);
    ok(!status && !ret, "Failed to suspend main thread, status %#lx.\n", status);

    /* Process wasn't suspended yet. */
    status = pNtResumeProcess(info.hProcess);
    ok(!status, "Failed to resume a process, status %#lx.\n", status);

    status = pNtSuspendProcess(0);
    ok(status == STATUS_INVALID_HANDLE, "Unexpected status %#lx.\n", status);

    status = pNtResumeProcess(info.hProcess);
    ok(!status, "Failed to resume a process, status %#lx.\n", status);

    ResetEvent(event2);
    ret = WaitForSingleObject(event2, 200);
    ok(ret == WAIT_OBJECT_0, "Wait failed.\n");

    status = pNtSuspendProcess(info.hProcess);
    ok(!status, "Failed to suspend a process, status %#lx.\n", status);

    status = NtSuspendThread(info.hThread, &ret);
    ok(!status && ret == 1, "Failed to suspend main thread, status %#lx.\n", status);
    status = NtResumeThread(info.hThread, &ret);
    ok(!status && ret == 2, "Failed to resume main thread, status %#lx.\n", status);

    ResetEvent(event2);
    ret = WaitForSingleObject(event2, 200);
    ok(ret == WAIT_TIMEOUT, "Wait failed.\n");

    status = pNtSuspendProcess(info.hProcess);
    ok(!status, "Failed to suspend a process, status %#lx.\n", status);

    status = pNtResumeProcess(info.hProcess);
    ok(!status, "Failed to resume a process, status %#lx.\n", status);

    ResetEvent(event2);
    ret = WaitForSingleObject(event2, 200);
    ok(ret == WAIT_TIMEOUT, "Wait failed.\n");

    status = pNtResumeProcess(info.hProcess);
    ok(!status, "Failed to resume a process, status %#lx.\n", status);

    ResetEvent(event2);
    ret = WaitForSingleObject(event2, 1000);
    ok(ret == WAIT_OBJECT_0, "Wait failed.\n");

    SetEvent(event);

    wait_child_process(info.hProcess);

    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);

    CloseHandle(event);
    CloseHandle(event2);
}

static void test_unload_trace(void)
{
    static const WCHAR imageW[] = {'m','s','x','m','l','3','.','d','l','l',0};
    RTL_UNLOAD_EVENT_TRACE *unload_trace, **unload_trace_ex = NULL, *ptr;
    ULONG *element_size, *element_count, size;
    HMODULE hmod;
    BOOL found;

    unload_trace = pRtlGetUnloadEventTrace();
    ok(unload_trace != NULL, "Failed to get unload events pointer.\n");

    if (pRtlGetUnloadEventTraceEx)
    {
        pRtlGetUnloadEventTraceEx(&element_size, &element_count, (void **)&unload_trace_ex);
        ok(*element_size >= sizeof(*ptr), "Unexpected element size.\n");
        ok(*element_count == RTL_UNLOAD_EVENT_TRACE_NUMBER, "Unexpected trace element count %lu.\n", *element_count);
        ok(unload_trace_ex != NULL, "Unexpected pointer %p.\n", unload_trace_ex);
        size = *element_size;
    }
    else
        size = sizeof(*unload_trace);

    hmod = LoadLibraryA("msxml3.dll");
    ok(hmod != NULL, "Failed to load library.\n");
    FreeLibrary(hmod);

    found = FALSE;
    ptr = unload_trace;
    while (ptr->BaseAddress != NULL)
    {
        if (!lstrcmpW(imageW, ptr->ImageName))
        {
            found = TRUE;
            break;
        }
        ptr = (RTL_UNLOAD_EVENT_TRACE *)((char *)ptr + size);
    }
    ok(found, "Unloaded module wasn't found.\n");

    if (unload_trace_ex)
    {
        found = FALSE;
        ptr = *unload_trace_ex;
        while (ptr->BaseAddress != NULL)
        {
            if (!lstrcmpW(imageW, ptr->ImageName))
            {
                found = TRUE;
                break;
            }
            ptr = (RTL_UNLOAD_EVENT_TRACE *)((char *)ptr + size);
        }
        ok(found, "Unloaded module wasn't found.\n");
    }
}

#if defined(__i386__) || defined(__x86_64__)

static const unsigned int test_extended_context_data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
static const unsigned test_extended_context_spoil_data1[8] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
static const unsigned test_extended_context_spoil_data2[8] = {0x15, 0x25, 0x35, 0x45, 0x55, 0x65, 0x75, 0x85};

static BOOL test_extended_context_modified_state;
static BOOL xsaveopt_enabled, compaction_enabled;
static ULONG64 xstate_supported_features;

static DWORD test_extended_context_handler(EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
        CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher)
{
    const ULONG64 expected_compaction_mask = (0x8000000000000000 | xstate_supported_features) & ~(ULONG64)3;
    CONTEXT_EX *xctx = (CONTEXT_EX *)(context + 1);
    unsigned int *context_ymm_data;
    DWORD expected_min_offset;
    XSTATE *xs;

    ok((context->ContextFlags & (CONTEXT_FULL | CONTEXT_XSTATE)) == (CONTEXT_FULL | CONTEXT_XSTATE),
            "Got unexpected ContextFlags %#lx.\n", context->ContextFlags);

    if ((context->ContextFlags & (CONTEXT_FULL | CONTEXT_XSTATE)) != (CONTEXT_FULL | CONTEXT_XSTATE))
        goto done;

#ifdef __x86_64__
    {
        /* Unwind contexts do not inherit xstate information. */
        DISPATCHER_CONTEXT *dispatch = (DISPATCHER_CONTEXT *)dispatcher;

        ok(!(dispatch->ContextRecord->ContextFlags & 0x40), "Got unexpected ContextRecord->ContextFlags %#lx.\n",
                dispatch->ContextRecord->ContextFlags);
    }
#endif

    ok(xctx->Legacy.Offset == -(int)(sizeof(CONTEXT)), "Got unexpected Legacy.Offset %ld.\n", xctx->Legacy.Offset);
    ok(xctx->Legacy.Length == sizeof(CONTEXT), "Got unexpected Legacy.Length %ld.\n", xctx->Legacy.Length);
    ok(xctx->All.Offset == -(int)sizeof(CONTEXT), "Got unexpected All.Offset %ld.\n", xctx->All.Offset);
    ok(xctx->All.Length == sizeof(CONTEXT) + xctx->XState.Offset + xctx->XState.Length,
            "Got unexpected All.Offset %ld.\n", xctx->All.Offset);
    expected_min_offset = sizeof(void *) == 8 ? sizeof(CONTEXT_EX) + sizeof(EXCEPTION_RECORD) : sizeof(CONTEXT_EX);
    ok(xctx->XState.Offset >= expected_min_offset,
            "Got unexpected XState.Offset %ld.\n", xctx->XState.Offset);
    ok(xctx->XState.Length >= sizeof(XSTATE), "Got unexpected XState.Length %ld.\n", xctx->XState.Length);

    xs = (XSTATE *)((char *)xctx + xctx->XState.Offset);
    context_ymm_data = (unsigned int *)&xs->YmmContext;
    ok(!((ULONG_PTR)xs % 64), "Got unexpected xs %p.\n", xs);

    if (compaction_enabled)
        ok((xs->CompactionMask & (expected_compaction_mask | 3)) == expected_compaction_mask,
                "Got compaction mask %#I64x, expected %#I64x.\n", xs->CompactionMask, expected_compaction_mask);
    else
        ok(!xs->CompactionMask, "Got compaction mask %#I64x.\n", xs->CompactionMask);

    if (test_extended_context_modified_state)
    {
        ok((xs->Mask & 7) == 4, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
        ok(!memcmp(context_ymm_data, test_extended_context_data + 4, sizeof(M128A)),
                "Got unexpected context data.\n");
    }
    else
    {
        ok((xs->Mask & 7) == (xsaveopt_enabled ? 0 : 4), "Got unexpected Mask %#I64x.\n", xs->Mask);
        /* The save area has garbage if xsaveopt is available, so we can't test
         * its contents. */

        /* Clear the mask; the state should be restored to INIT_STATE without
         * using this data. */
        xs->Mask = 0;
        memcpy(context_ymm_data, test_extended_context_spoil_data1 + 4, sizeof(M128A));
    }

done:
#ifdef __GNUC__
    __asm__ volatile("vmovups (%0),%%ymm0" : : "r"(test_extended_context_spoil_data2));
#endif
#ifdef __x86_64__
    ++context->Rip;
#else
    if (*(BYTE *)context->Eip == 0xcc)
        ++context->Eip;
#endif
    return ExceptionContinueExecution;
}

struct call_func_offsets
{
    unsigned int func_addr;
    unsigned int func_param1;
    unsigned int func_param2;
    unsigned int ymm0_save;
};
#ifdef __x86_64__
static BYTE call_func_code_set_ymm0[] =
{
    0x55,                         /* pushq %rbp */
    0x48, 0xb8,                   /* mov imm,%rax */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x48, 0xb9,                   /* mov imm,%rcx */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x48, 0xba,                   /* mov imm,%rdx */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x48, 0xbd,                   /* mov imm,%rbp */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0xc5, 0xfc, 0x10, 0x45, 0x00, /* vmovups (%rbp),%ymm0 */
    0x48, 0x83, 0xec, 0x20,       /* sub $0x20,%rsp */
    0xff, 0xd0,                   /* call *rax */
    0x48, 0x83, 0xc4, 0x20,       /* add $0x20,%rsp */
    0xc5, 0xfc, 0x11, 0x45, 0x00, /* vmovups %ymm0,(%rbp) */
    0x5d,                         /* popq %rbp */
    0xc3,                         /* ret  */
};
static BYTE call_func_code_reset_ymm_state[] =
{
    0x55,                         /* pushq %rbp */
    0x48, 0xb8,                   /* mov imm,%rax */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x48, 0xb9,                   /* mov imm,%rcx */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x48, 0xba,                   /* mov imm,%rdx */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x48, 0xbd,                   /* mov imm,%rbp */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0xc5, 0xf8, 0x77,             /* vzeroupper */
    0x0f, 0x57, 0xc0,             /* xorps  %xmm0,%xmm0 */
    0x48, 0x83, 0xec, 0x20,       /* sub $0x20,%rsp */
    0xff, 0xd0,                   /* call *rax */
    0x48, 0x83, 0xc4, 0x20,       /* add $0x20,%rsp */
    0xc5, 0xfc, 0x11, 0x45, 0x00, /* vmovups %ymm0,(%rbp) */
    0x5d,                         /* popq %rbp */
    0xc3,                         /* ret  */
};
static const struct call_func_offsets call_func_offsets = {3, 13, 23, 33};
#else
static BYTE call_func_code_set_ymm0[] =
{
    0x55,                         /* pushl %ebp */
    0xb8,                         /* mov imm,%eax */
    0x00, 0x00, 0x00, 0x00,

    0xb9,                         /* mov imm,%ecx */
    0x00, 0x00, 0x00, 0x00,

    0xba,                         /* mov imm,%edx */
    0x00, 0x00, 0x00, 0x00,

    0xbd,                         /* mov imm,%ebp */
    0x00, 0x00, 0x00, 0x00,

    0x81, 0xfa, 0xef, 0xbe, 0xad, 0xde,
                                  /* cmpl $0xdeadbeef, %edx */
    0x74, 0x01,                   /* je 1f */
    0x52,                         /* pushl %edx */
    0x51,                         /* 1: pushl %ecx */
    0xc5, 0xfc, 0x10, 0x45, 0x00, /* vmovups (%ebp),%ymm0 */
    0xff, 0xd0,                   /* call *eax */
    0xc5, 0xfc, 0x11, 0x45, 0x00, /* vmovups %ymm0,(%ebp) */
    0x5d,                         /* popl %ebp */
    0xc3,                         /* ret  */
};
static BYTE call_func_code_reset_ymm_state[] =
{
    0x55,                         /* pushl %ebp */
    0xb8,                         /* mov imm,%eax */
    0x00, 0x00, 0x00, 0x00,

    0xb9,                         /* mov imm,%ecx */
    0x00, 0x00, 0x00, 0x00,

    0xba,                         /* mov imm,%edx */
    0x00, 0x00, 0x00, 0x00,

    0xbd,                         /* mov imm,%ebp */
    0x00, 0x00, 0x00, 0x00,

    0x81, 0xfa, 0xef, 0xbe, 0xad, 0xde,
                                  /* cmpl $0xdeadbeef, %edx */
    0x74, 0x01,                   /* je 1f */
    0x52,                         /* pushl %edx */
    0x51,                         /* 1: pushl %ecx */
    0xc5, 0xf8, 0x77,             /* vzeroupper */
    0x0f, 0x57, 0xc0,             /* xorps  %xmm0,%xmm0 */
    0xff, 0xd0,                   /* call *eax */
    0xc5, 0xfc, 0x11, 0x45, 0x00, /* vmovups %ymm0,(%ebp) */
    0x5d,                         /* popl %ebp */
    0xc3,                         /* ret  */
};
static const struct call_func_offsets call_func_offsets = {2, 7, 12, 17};
#endif

static DWORD WINAPI test_extended_context_thread(void *arg)
{
    ULONG (WINAPI* func)(void) = code_mem;
    static unsigned int data[8];
    unsigned int i;

    memcpy(code_mem, call_func_code_reset_ymm_state, sizeof(call_func_code_reset_ymm_state));
    *(void **)((BYTE *)code_mem + call_func_offsets.func_addr) = SuspendThread;
    *(void **)((BYTE *)code_mem + call_func_offsets.func_param1) = (void *)GetCurrentThread();
    *(void **)((BYTE *)code_mem + call_func_offsets.func_param2) = (void *)0xdeadbeef;
    *(void **)((BYTE *)code_mem + call_func_offsets.ymm0_save) = data;
    func();

    for (i = 0; i < 4; ++i)
        ok(!data[i], "Got unexpected data %#x, i %u.\n", data[i], i);
    for (; i < 8; ++i)
        ok(data[i] == 0x48484848, "Got unexpected data %#x, i %u.\n", data[i], i);
    memset(data, 0x68, sizeof(data));

    memcpy(code_mem, call_func_code_set_ymm0, sizeof(call_func_code_set_ymm0));
    *(void **)((BYTE *)code_mem + call_func_offsets.func_addr) = SuspendThread;
    *(void **)((BYTE *)code_mem + call_func_offsets.func_param1) = (void *)GetCurrentThread();
    *(void **)((BYTE *)code_mem + call_func_offsets.func_param2) = (void *)0xdeadbeef;
    *(void **)((BYTE *)code_mem + call_func_offsets.ymm0_save) = data;
    func();

    memcpy(code_mem, call_func_code_reset_ymm_state, sizeof(call_func_code_reset_ymm_state));
    *(void **)((BYTE *)code_mem + call_func_offsets.func_addr) = SuspendThread;
    *(void **)((BYTE *)code_mem + call_func_offsets.func_param1) = (void *)GetCurrentThread();
    *(void **)((BYTE *)code_mem + call_func_offsets.func_param2) = (void *)0xdeadbeef;
    *(void **)((BYTE *)code_mem + call_func_offsets.ymm0_save) = data;
    func();
    return 0;
}

static void wait_for_thread_next_suspend(HANDLE thread)
{
    DWORD result;

    result = ResumeThread(thread);
    ok(result == 1, "Got unexpected suspend count %lu.\n", result);

    /* NtQueryInformationThread(ThreadSuspendCount, ...) is not supported on older Windows. */
    while (!(result = SuspendThread(thread)))
    {
        ResumeThread(thread);
        Sleep(1);
    }
    ok(result == 1, "Got unexpected suspend count %lu.\n", result);
    result = ResumeThread(thread);
    ok(result == 2, "Got unexpected suspend count %lu.\n", result);
}

#define CONTEXT_NATIVE (CONTEXT_XSTATE & CONTEXT_CONTROL)

struct context_parameters
{
    ULONG flag;
    ULONG supported_flags;
    ULONG broken_flags;
    ULONG context_length;
    ULONG legacy_length;
    ULONG context_ex_length;
    ULONG align;
    ULONG flags_offset;
    ULONG xsavearea_offset;
    ULONG vector_reg_count;
};

static void test_extended_context(void)
{
    static BYTE except_code_reset_ymm_state[] =
    {
#ifdef __x86_64__
        0x48,
#endif
        0xb8,                         /* mov imm,%ax */
        0x00, 0x00, 0x00, 0x00,
#ifdef __x86_64__
        0x00, 0x00, 0x00, 0x00,
#endif
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        0xc5, 0xf8, 0x77,             /* vzeroupper */
        0x0f, 0x57, 0xc0,             /* xorps  %xmm0,%xmm0 */
        0xcc,                         /* int3 */
        0xc5, 0xfc, 0x11, 0x00,       /* vmovups %ymm0,(%ax) */
        0xc3,                         /* ret  */
    };

    static const struct context_parameters context_arch_old[] =
    {
        {
            0x00100000,  /* CONTEXT_AMD64 */
            0xd800005f,
            0xd8000000,
            0x4d0,       /* sizeof(CONTEXT) */
            0x4d0,       /* sizeof(CONTEXT) */
            0x20,        /* sizeof(CONTEXT_EX) */
            7,
            0x30,
            0x100,       /* offsetof(CONTEXT, FltSave) */
            16,
        },
        {
            0x00010000,  /* CONTEXT_X86  */
            0xd800007f,
            0xd8000000,
            0x2cc,       /* sizeof(CONTEXT) */
            0xcc,        /* offsetof(CONTEXT, ExtendedRegisters) */
            0x18,        /* sizeof(CONTEXT_EX) */
            3,
            0,
            0xcc,        /* offsetof(CONTEXT, ExtendedRegisters) */
            8,
        },
    };

    static const struct context_parameters context_arch_new[] =
    {
        {
            0x00100000,  /* CONTEXT_AMD64 */
            0xf800005f,
            0xf8000000,
            0x4d0,       /* sizeof(CONTEXT) */
            0x4d0,       /* sizeof(CONTEXT) */
            0x20,        /* sizeof(CONTEXT_EX) */
            15,
            0x30,
            0x100,       /* offsetof(CONTEXT, FltSave) */
            16,
        },
        {
            0x00010000,  /* CONTEXT_X86  */
            0xf800007f,
            0xf8000000,
            0x2cc,       /* sizeof(CONTEXT) */
            0xcc,        /* offsetof(CONTEXT, ExtendedRegisters) */
            0x20,        /* sizeof(CONTEXT_EX) */
            3,
            0,
            0xcc,        /* offsetof(CONTEXT, ExtendedRegisters) */
            8,
        },
    };
    const struct context_parameters *context_arch;

    const ULONG64 supported_features = 0xff;
    const ULONG64 supported_compaction_mask = supported_features | ((ULONG64)1 << 63);
    ULONG expected_length, expected_length_xstate, context_flags, expected_offset, max_xstate_length;
    ULONG64 enabled_features, expected_compaction;
    DECLSPEC_ALIGN(64) BYTE context_buffer2[4096];
    DECLSPEC_ALIGN(64) BYTE context_buffer[4096];
    unsigned int i, j, address_offset, test;
    ULONG ret, ret2, length, length2, align;
    ULONG flags, flags_fpx, expected_flags;
    ULONG (WINAPI* func)(void) = code_mem;
    CONTEXT_EX *context_ex;
    CONTEXT *context;
    unsigned data[8];
    HANDLE thread;
    ULONG64 mask;
    XSTATE *xs;
    BOOL bret;
    void *p;

    address_offset = sizeof(void *) == 8 ? 2 : 1;
    *(void **)(except_code_set_ymm0 + address_offset) = data;
    *(void **)(except_code_reset_ymm_state + address_offset) = data;

    if (!pRtlGetEnabledExtendedFeatures)
    {
        win_skip("RtlGetEnabledExtendedFeatures is not available.\n");
        return;
    }

    enabled_features = pRtlGetEnabledExtendedFeatures(~(ULONG64)0);

#ifndef __arm64ec__
    if (enabled_features)
    {
        int regs[4];

        __cpuidex(regs, 0xd, 1);
        xsaveopt_enabled = regs[0] & 1;
        compaction_enabled = regs[0] & 2;
    }
#endif
    xstate_supported_features = enabled_features & supported_features;

    /* Test context manipulation functions. */
    length = 0xdeadbeef;
    ret = pRtlGetExtendedContextLength(0, &length);
    ok(ret == STATUS_INVALID_PARAMETER && length == 0xdeadbeef, "Got unexpected result ret %#lx, length %#lx.\n",
            ret, length);

    ret = pRtlGetExtendedContextLength(context_arch_new[0].flag, &length);
    ok(!ret, "Got %#lx.\n", ret);
    if (length == context_arch_new[0].context_length + context_arch_new[0].context_ex_length
                + context_arch_new[0].align)
        context_arch = context_arch_new;
    else
        context_arch = context_arch_old;

    for (test = 0; test < 2; ++test)
    {
        expected_length = context_arch[test].context_length + context_arch[test].context_ex_length
                + context_arch[test].align;
        expected_length_xstate = context_arch[test].context_length + context_arch[test].context_ex_length
                + sizeof(XSTATE) + 63;

        length = 0xdeadbeef;
        ret = pRtlGetExtendedContextLength(context_arch[test].flag, &length);
        ok(!ret && length == expected_length, "Got unexpected result ret %#lx, length %#lx.\n",
                ret, length);

        for (i = 0; i < 32; ++i)
        {
            if (i == 6) /* CONTEXT_XSTATE */
                continue;

            flags = context_arch[test].flag | (1 << i);
            length = length2 = 0xdeadbeef;
            ret = pRtlGetExtendedContextLength(flags, &length);

            if ((context_arch[test].supported_flags & flags) || flags == context_arch[test].flag)
            {
                ok((!ret && length == expected_length)
                        || broken((context_arch[test].broken_flags & (1 << i))
                        && ret == STATUS_INVALID_PARAMETER && length == 0xdeadbeef),
                        "Got unexpected result ret %#lx, length %#lx, flags 0x%08lx.\n",
                        ret, length, flags);
            }
            else
            {
                ok((ret == STATUS_INVALID_PARAMETER || ret == STATUS_NOT_SUPPORTED) && length == 0xdeadbeef,
                        "Got unexpected result ret %#lx, length %#lx, flags 0x%08lx.\n", ret, length, flags);
            }

            SetLastError(0xdeadbeef);
            bret = pInitializeContext(NULL, flags, NULL, &length2);
            ok(!bret && length2 == length && GetLastError()
                    == (!ret ? ERROR_INSUFFICIENT_BUFFER
                    : (ret == STATUS_INVALID_PARAMETER ? ERROR_INVALID_PARAMETER : ERROR_NOT_SUPPORTED)),
                    "Got unexpected bret %#x, length2 %#lx, GetLastError() %lu, flags %#lx.\n",
                    bret, length2, GetLastError(), flags);

            if (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_NOT_SUPPORTED)
                continue;

            SetLastError(0xdeadbeef);
            context = (void *)0xdeadbeef;
            length2 = expected_length - 1;
            bret = pInitializeContext(context_buffer, flags, &context, &length2);
            ok(!bret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
                    "Got unexpected bret %#x, GetLastError() %lu, flags %#lx.\n", bret, GetLastError(), flags);
            ok(context == (void *)0xdeadbeef, "Got unexpected context %p.\n", context);

            SetLastError(0xdeadbeef);
            memset(context_buffer, 0xcc, sizeof(context_buffer));
            length2 = expected_length;
            bret = pInitializeContext(context_buffer, flags, &context, &length2);
            ok(bret && GetLastError() == 0xdeadbeef,
                    "Got unexpected bret %#x, GetLastError() %lu, flags %#lx.\n", bret, GetLastError(), flags);
            ok(length2 == expected_length, "Got unexpected length %#lx.\n", length);
            ok((BYTE *)context == context_buffer, "Got unexpected context %p, flags %#lx.\n", context, flags);

            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == flags, "Got unexpected ContextFlags %#lx, flags %#lx.\n", context_flags, flags);

            context_ex = (CONTEXT_EX *)(context_buffer + context_arch[test].context_length);
            ok(context_ex->Legacy.Offset == -(int)context_arch[test].context_length,
                    "Got unexpected Offset %ld, flags %#lx.\n", context_ex->Legacy.Offset, flags);
            ok(context_ex->Legacy.Length == ((flags & 0x20) ? context_arch[test].context_length
                    : context_arch[test].legacy_length),
                    "Got unexpected Length %#lx, flags %#lx.\n", context_ex->Legacy.Length, flags);
            ok(context_ex->All.Offset == -(int)context_arch[test].context_length,
                    "Got unexpected Offset %ld, flags %#lx.\n", context_ex->All.Offset, flags);

            /* No extra 8 bytes in x64 CONTEXT_EX here (before Win11). */
            ok(context_ex->All.Length == context_arch[test].context_length + context_arch[1].context_ex_length,
                    "Got unexpected Length %#lx, flags %#lx.\n", context_ex->All.Length, flags);

            ok(context_ex->XState.Offset == context_arch[1].context_ex_length + 1,
                    "Got unexpected Offset %ld, flags %#lx.\n", context_ex->XState.Offset, flags);
            ok(!context_ex->XState.Length,
                    "Got unexpected Length %#lx, flags %#lx.\n", context_ex->XState.Length, flags);

            if (0)
            {
                /* Crashes on Windows. */
                pRtlLocateLegacyContext(NULL, NULL);
            }
            p = pRtlLocateLegacyContext(context_ex, NULL);
            ok(p == context, "Got unexpected p %p, flags %#lx.\n", p, flags);
            length2 = 0xdeadbeef;
            p = pRtlLocateLegacyContext(context_ex, &length2);
            ok(p == context && length2 == context_ex->Legacy.Length,
                    "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);
            length2 = expected_length;

            if (0)
            {
                /* Crashes on Windows. */
                pGetXStateFeaturesMask(context, NULL);
                pRtlGetExtendedFeaturesMask(context_ex);
                pRtlSetExtendedFeaturesMask(context_ex, 0);
            }

            flags_fpx = flags & 0x10000 ? flags | 0x20 : flags | 0x8;

            mask = 0xdeadbeef;
            bret = pGetXStateFeaturesMask(context, &mask);
            SetLastError(0xdeadbeef);
            if (flags & CONTEXT_NATIVE)
                ok(bret && mask == ((flags & flags_fpx) == flags_fpx ? 0x3 : 0),
                        "Got unexpected bret %#x, mask %s, flags %#lx.\n", bret, wine_dbgstr_longlong(mask), flags);
            else
                ok(!bret && mask == 0xdeadbeef && GetLastError() == 0xdeadbeef,
                        "Got unexpected bret %#x, mask %s, GetLastError() %#lx, flags %#lx.\n",
                        bret, wine_dbgstr_longlong(mask), GetLastError(), flags);

            bret = pSetXStateFeaturesMask(context, 0);
            ok(bret == !!(flags & CONTEXT_NATIVE), "Got unexpected bret %#x, flags %#lx.\n", bret, flags);
            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == flags, "Got unexpected ContextFlags %#lx, flags %#lx.\n", context_flags, flags);

            bret = pSetXStateFeaturesMask(context, 1);
            ok(bret == !!(flags & CONTEXT_NATIVE), "Got unexpected bret %#x, flags %#lx.\n", bret, flags);
            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == (bret ? flags_fpx : flags),
                    "Got unexpected ContextFlags %#lx, flags %#lx.\n", context_flags, flags);

            bret = pSetXStateFeaturesMask(context, 2);
            ok(bret == !!(flags & CONTEXT_NATIVE), "Got unexpected bret %#x, flags %#lx.\n", bret, flags);
            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == (bret ? flags_fpx : flags),
                    "Got unexpected ContextFlags %#lx, flags %#lx.\n", context_flags, flags);

            bret = pSetXStateFeaturesMask(context, 4);
            ok(!bret, "Got unexpected bret %#x.\n", bret);
            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == (flags & CONTEXT_NATIVE ? flags_fpx : flags),
                    "Got unexpected ContextFlags %#lx, flags %#lx.\n", context_flags, flags);
            *(DWORD *)(context_buffer + context_arch[test].flags_offset) = flags;

            for (j = 0; j < context_arch[test].flags_offset; ++j)
            {
                if (context_buffer[j] != 0xcc)
                {
                    ok(0, "Buffer data changed at offset %#x.\n", j);
                    break;
                }
            }
            for (j = context_arch[test].flags_offset + sizeof(context_flags);
                    j < context_arch[test].context_length; ++j)
            {
                if (context_buffer[j] != 0xcc)
                {
                    ok(0, "Buffer data changed at offset %#x.\n", j);
                    break;
                }
            }
            for (j = context_arch[test].context_length + context_arch[test].context_ex_length;
                    j < sizeof(context_buffer); ++j)
            {
                if (context_buffer[j] != 0xcc)
                {
                    ok(0, "Buffer data changed at offset %#x.\n", j);
                    break;
                }
            }

            memset(context_buffer2, 0xcc, sizeof(context_buffer2));
            ret2 = pRtlInitializeExtendedContext(context_buffer2, flags, &context_ex);
            ok(!ret2, "Got unexpected ret2 %#lx, flags %#lx.\n", ret2, flags);
            ok(!memcmp(context_buffer2, context_buffer, sizeof(context_buffer2)),
                    "Context data do not match, flags %#lx.\n", flags);

            memset(context_buffer2, 0xcc, sizeof(context_buffer2));
            ret2 = pRtlInitializeExtendedContext(context_buffer2 + 2, flags, &context_ex);
            ok(!ret2, "Got unexpected ret2 %#lx, flags %#lx.\n", ret2, flags);

            /* Buffer gets aligned to 16 bytes on x64, while returned context length suggests it should be 8. */
            align = test ? 4 : 16;
            ok(!memcmp(context_buffer2 + align, context_buffer,
                    sizeof(context_buffer2) - align),
                    "Context data do not match, flags %#lx.\n", flags);

            SetLastError(0xdeadbeef);
            memset(context_buffer2, 0xcc, sizeof(context_buffer2));
            bret = pInitializeContext(context_buffer2 + 2, flags, &context, &length2);
            ok(bret && GetLastError() == 0xdeadbeef,
                    "Got unexpected bret %#x, GetLastError() %lu, flags %#lx.\n", bret, GetLastError(), flags);
            ok(length2 == expected_length, "Got unexpected length %#lx.\n", length);
            ok(!memcmp(context_buffer2 + align, context_buffer,
                    sizeof(context_buffer2) - align),
                    "Context data do not match, flags %#lx.\n", flags);

            length2 = 0xdeadbeef;
            p = pLocateXStateFeature(context, 0, &length2);
            if (flags & CONTEXT_NATIVE)
                ok(p == (BYTE *)context + context_arch[test].xsavearea_offset
                        && length2 == offsetof(XSAVE_FORMAT, XmmRegisters),
                        "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);
            else
                ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);
            length2 = 0xdeadbeef;
            p = pLocateXStateFeature(context, 1, &length2);
            if (flags & CONTEXT_NATIVE)
                ok(p == (BYTE *)context + context_arch[test].xsavearea_offset + offsetof(XSAVE_FORMAT, XmmRegisters)
                        && length2 == sizeof(M128A) * context_arch[test].vector_reg_count,
                         "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);
            else
                ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);
            length2 = 0xdeadbeef;
            p = pLocateXStateFeature(context, 2, &length2);
            ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);

            if (!pRtlInitializeExtendedContext2 || !pInitializeContext2)
            {
                static int once;

                if (!once++)
                    win_skip("InitializeContext2 is not available.\n");
                continue;
            }

            length2 = expected_length;
            memset(context_buffer2, 0xcc, sizeof(context_buffer2));
            ret2 = pRtlInitializeExtendedContext2(context_buffer2 + 2, flags, &context_ex, ~(ULONG64)0);
            ok(!ret2, "Got unexpected ret2 %#lx, flags %#lx.\n", ret2, flags);
            ok(!memcmp(context_buffer2 + align, context_buffer,
                    sizeof(context_buffer2) - align),
                    "Context data do not match, flags %#lx.\n", flags);

            memset(context_buffer2, 0xcc, sizeof(context_buffer2));
            bret = pInitializeContext2(context_buffer2 + 2, flags, &context, &length2, 0);
            ok(bret && GetLastError() == 0xdeadbeef,
                    "Got unexpected bret %#x, GetLastError() %lu, flags %#lx.\n", bret, GetLastError(), flags);
            ok(length2 == expected_length, "Got unexpected length %#lx.\n", length);
            ok(!memcmp(context_buffer2 + align, context_buffer,
                    sizeof(context_buffer2) - align),
                    "Context data do not match, flags %#lx.\n", flags);

            length2 = 0xdeadbeef;
            p = pLocateXStateFeature(context, 0, &length2);
            if (flags & CONTEXT_NATIVE)
                ok(p == (BYTE *)context + context_arch[test].xsavearea_offset
                        && length2 == offsetof(XSAVE_FORMAT, XmmRegisters),
                        "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);
            else
                ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);
        }

        flags = context_arch[test].flag | 0x40;
        flags_fpx = flags & 0x10000 ? flags | 0x20 : flags | 0x8;

        length = 0xdeadbeef;
        ret = pRtlGetExtendedContextLength(flags, &length);

        if (!enabled_features)
        {
            ok(ret == STATUS_NOT_SUPPORTED && length == 0xdeadbeef,
                    "Got unexpected result ret %#lx, length %#lx.\n", ret, length);

            context_ex = (void *)0xdeadbeef;
            ret2 = pRtlInitializeExtendedContext(context_buffer, flags, &context_ex);
            ok(ret2 == STATUS_NOT_SUPPORTED, "Got unexpected result ret %#lx, test %u.\n", ret2, test);

            SetLastError(0xdeadbeef);
            length2 = sizeof(context_buffer);
            bret = pInitializeContext(context_buffer, flags, &context, &length2);
            ok(bret && GetLastError() == 0xdeadbeef,
                    "Got unexpected bret %#x, GetLastError() %lu, flags %#lx.\n", bret, GetLastError(), flags);
            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == (flags & ~0x40), "Got unexpected ContextFlags %#lx, flags %#lx.\n",
                    context_flags, flags);

            if (pInitializeContext2)
            {
                SetLastError(0xdeadbeef);
                length2 = sizeof(context_buffer);
                bret = pInitializeContext2(context_buffer, flags, &context, &length2, ~(ULONG64)0);
                ok(bret && GetLastError() == 0xdeadbeef,
                        "Got unexpected bret %#x, GetLastError() %lu, flags %#lx.\n", bret, GetLastError(), flags);
                context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
                ok(context_flags == (flags & ~0x40), "Got unexpected ContextFlags %#lx, flags %#lx.\n",
                        context_flags, flags);
            }
            continue;
        }

        ok(!ret && length >= expected_length_xstate,
                "Got unexpected result ret %#lx, length %#lx, test %u.\n", ret, length, test);

        if (!pRtlGetExtendedContextLength2)
        {
            win_skip("RtlGetExtendedContextLength2 is not available.\n");
        }
        else
        {
            length = 0xdeadbeef;
            ret = pRtlGetExtendedContextLength2(flags, &length, 7);
            ok(!ret && length == expected_length_xstate,
                    "Got unexpected result ret %#lx, length %#lx, test %u.\n", ret, length, test);

            length = 0xdeadbeef;
            ret = pRtlGetExtendedContextLength2(flags, &length, ~0);
            ok(!ret && length >= expected_length_xstate,
                    "Got unexpected result ret %#lx, length %#lx, test %u.\n", ret, length, test);

            length = 0xdeadbeef;
            ret = pRtlGetExtendedContextLength2(flags, &length, 0);
            ok((!ret && length == expected_length_xstate - sizeof(YMMCONTEXT))
                    || broken(!ret && length == expected_length_xstate) /* win10pro */,
                    "Got unexpected result ret %#lx, length %#lx, test %u.\n", ret, length, test);

            length = 0xdeadbeef;
            ret = pRtlGetExtendedContextLength2(flags, &length, 3);
            ok((!ret && length == expected_length_xstate - sizeof(YMMCONTEXT))
                    || broken(!ret && length == expected_length_xstate) /* win10pro */,
                    "Got unexpected result ret %#lx, length %#lx, test %u.\n", ret, length, test);

            length = 0xdeadbeef;
            ret = pRtlGetExtendedContextLength2(flags, &length, 4);
            ok(!ret && length == expected_length_xstate,
                    "Got unexpected result ret %#lx, length %#lx, test %u.\n", ret, length, test);
        }

        pRtlGetExtendedContextLength(flags, &length);
        SetLastError(0xdeadbeef);
        bret = pInitializeContext(NULL, flags, NULL, &length2);
        ok(!bret && length2 == length && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
                "Got unexpected bret %#x, length2 %#lx, GetLastError() %lu, flags %#lx.\n",
                bret, length2, GetLastError(), flags);

        SetLastError(0xdeadbeef);
        context = (void *)0xdeadbeef;
        length2 = length - 1;
        bret = pInitializeContext(context_buffer, flags, &context, &length2);
        ok(!bret && GetLastError() == ERROR_INSUFFICIENT_BUFFER && length2 == length && context == (void *)0xdeadbeef,
                "Got unexpected bret %#x, GetLastError() %lu, length2 %#lx, flags %#lx.\n",
                bret, GetLastError(), length2, flags);

        SetLastError(0xdeadbeef);
        memset(context_buffer, 0xcc, sizeof(context_buffer));
        length2 = length + 1;
        bret = pInitializeContext(context_buffer, flags, &context, &length2);
        ok(bret && GetLastError() == 0xdeadbeef,
                "Got unexpected bret %#x, GetLastError() %lu, flags %#lx.\n", bret, GetLastError(), flags);
        ok(length2 == length, "Got unexpected length %#lx.\n", length);
        ok((BYTE *)context == context_buffer, "Got unexpected context %p.\n", context);

        context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
        ok(context_flags == flags, "Got unexpected ContextFlags %#lx, flags %#lx.\n", context_flags, flags);

        context_ex = (CONTEXT_EX *)(context_buffer + context_arch[test].context_length);
        ok(context_ex->Legacy.Offset == -(int)context_arch[test].context_length,
                "Got unexpected Offset %ld, flags %#lx.\n", context_ex->Legacy.Offset, flags);
        ok(context_ex->Legacy.Length == ((flags & 0x20) ? context_arch[test].context_length
                : context_arch[test].legacy_length),
                "Got unexpected Length %#lx, flags %#lx.\n", context_ex->Legacy.Length, flags);

        expected_offset = (((ULONG_PTR)context + context_arch[test].context_length
                + context_arch[test].context_ex_length + 63) & ~(ULONG64)63) - (ULONG_PTR)context
                - context_arch[test].context_length;
        ok(context_ex->XState.Offset == expected_offset,
                "Got unexpected Offset %ld, flags %#lx.\n", context_ex->XState.Offset, flags);
        ok(context_ex->XState.Length >= sizeof(XSTATE),
                "Got unexpected Length %#lx, flags %#lx.\n", context_ex->XState.Length, flags);

        ok(context_ex->All.Offset == -(int)context_arch[test].context_length,
                "Got unexpected Offset %ld, flags %#lx.\n", context_ex->All.Offset, flags);
        /* No extra 8 bytes in x64 CONTEXT_EX here. */
        ok(context_ex->All.Length == context_arch[test].context_length
                + context_ex->XState.Offset + context_ex->XState.Length,
                "Got unexpected Length %#lx, flags %#lx.\n", context_ex->All.Length, flags);

        xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);
        length2 = 0xdeadbeef;
        for (i = 0; i < 2; ++i)
        {
            p = pRtlLocateExtendedFeature(context_ex, i, &length2);
            ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#lx.\n", p, length2);
        }

        p = pRtlLocateExtendedFeature(context_ex, XSTATE_AVX, &length2);
        ok(length2 == sizeof(YMMCONTEXT), "Got unexpected length %#lx.\n", length2);
        ok(p == &xs->YmmContext, "Got unexpected p %p.\n", p);
        p = pRtlLocateExtendedFeature(context_ex, XSTATE_AVX, NULL);
        ok(p == &xs->YmmContext, "Got unexpected p %p.\n", p);

        length2 = 0xdeadbeef;
        p = pLocateXStateFeature(context, 0, &length2);
        if (flags & CONTEXT_NATIVE)
            ok(p == (BYTE *)context + context_arch[test].xsavearea_offset
                    && length2 == offsetof(XSAVE_FORMAT, XmmRegisters),
                    "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);
        else
            ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);

        length2 = 0xdeadbeef;
        p = pLocateXStateFeature(context, 1, &length2);
        if (flags & CONTEXT_NATIVE)
            ok(p == (BYTE *)context + context_arch[test].xsavearea_offset + offsetof(XSAVE_FORMAT, XmmRegisters)
                    && length2 == sizeof(M128A) * context_arch[test].vector_reg_count,
                    "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);
        else
            ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);

        length2 = 0xdeadbeef;
        p = pLocateXStateFeature(context, 2, &length2);
        if (flags & CONTEXT_NATIVE)
            ok(p == &xs->YmmContext && length2 == sizeof(YMMCONTEXT),
                    "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);
        else
            ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);

        mask = 0xdeadbeef;
        bret = pGetXStateFeaturesMask(context, &mask);
        if (flags & CONTEXT_NATIVE)
            ok(bret && !mask,
                    "Got unexpected bret %#x, mask %s, flags %#lx.\n", bret, wine_dbgstr_longlong(mask), flags);
        else
            ok(!bret && mask == 0xdeadbeef,
                    "Got unexpected bret %#x, mask %s, flags %#lx.\n", bret, wine_dbgstr_longlong(mask), flags);

        expected_compaction = compaction_enabled ? ((ULONG64)1 << 63) | enabled_features : 0;
        ok(!xs->Mask, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
        mask = pRtlGetExtendedFeaturesMask(context_ex);
        ok(mask == (xs->Mask & ~(ULONG64)3), "Got unexpected mask %s.\n", wine_dbgstr_longlong(mask));
        ok(xs->CompactionMask == expected_compaction,
                "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(xs->CompactionMask));
        ok(!xs->Reserved[0], "Got unexpected Reserved[0]  %s.\n", wine_dbgstr_longlong(xs->Reserved[0]));

        xs->Mask = 0xdeadbeef;
        xs->CompactionMask = 0xdeadbeef;
        pRtlSetExtendedFeaturesMask(context_ex, 0);
        ok(!xs->Mask, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
        mask = pRtlGetExtendedFeaturesMask(context_ex);
        ok(mask == (xs->Mask & ~(ULONG64)3), "Got unexpected mask %s.\n", wine_dbgstr_longlong(mask));
        ok(xs->CompactionMask == 0xdeadbeef, "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(xs->CompactionMask));
        context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
        ok(context_flags == flags, "Got unexpected ContextFlags %#lx, flags %#lx.\n", context->ContextFlags, flags);

        xs->Mask = 0xdeadbeef;
        xs->CompactionMask = 0;
        pRtlSetExtendedFeaturesMask(context_ex, ~(ULONG64)0);
        ok(xs->Mask == (enabled_features & ~(ULONG64)3), "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
        mask = pRtlGetExtendedFeaturesMask(context_ex);
        ok(mask == (xs->Mask & ~(ULONG64)3), "Got unexpected mask %s.\n", wine_dbgstr_longlong(mask));
        ok(!xs->CompactionMask, "Got unexpected CompactionMask %s.\n",
                wine_dbgstr_longlong(xs->CompactionMask));
        context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
        ok(context_flags == flags, "Got unexpected ContextFlags %#lx, flags %#lx.\n", context->ContextFlags, flags);

        xs->Mask = 0xdeadbeef;
        xs->CompactionMask = 0xdeadbeef;
        bret = pSetXStateFeaturesMask(context, xstate_supported_features);
        ok(bret == !!(flags & CONTEXT_NATIVE), "Got unexpected bret %#x.\n", bret);
        context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
        ok(context_flags == (bret ? flags_fpx : flags),
                "Got unexpected ContextFlags %#lx, flags %#lx.\n", context_flags, flags);
        ok(xs->Mask == bret ? 4 : 0xdeadbeef, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
        mask = pRtlGetExtendedFeaturesMask(context_ex);
        ok(mask == (xs->Mask & ~(ULONG64)3), "Got unexpected mask %s.\n", wine_dbgstr_longlong(mask));
        ok(xs->CompactionMask == bret ? expected_compaction : 0xdeadbeef, "Got unexpected CompactionMask %s.\n",
                wine_dbgstr_longlong(xs->CompactionMask));

        mask = 0xdeadbeef;
        bret = pGetXStateFeaturesMask(context, &mask);
        if (flags & CONTEXT_NATIVE)
            ok(bret && mask == xstate_supported_features,
                    "Got unexpected bret %#x, mask %s, flags %#lx (enabled_features & supported_features %#I64x).\n", bret, wine_dbgstr_longlong(mask), flags, xstate_supported_features);
        else
            ok(!bret && mask == 0xdeadbeef,
                    "Got unexpected bret %#x, mask %s, flags %#lx.\n", bret, wine_dbgstr_longlong(mask), flags);

        if (pRtlGetExtendedContextLength2)
        {
            memset(context_buffer, 0xcc, sizeof(context_buffer));
            pRtlGetExtendedContextLength2(flags, &length, 0);
            SetLastError(0xdeadbeef);
            memset(context_buffer, 0xcc, sizeof(context_buffer));
            length2 = length;
            bret = pInitializeContext2(context_buffer, flags, &context, &length2, 0);
            ok(bret && GetLastError() == 0xdeadbeef,
                    "Got unexpected bret %#x, GetLastError() %lu, flags %#lx.\n", bret, GetLastError(), flags);
            ok(length2 == length, "Got unexpected length %#lx.\n", length);
            ok((BYTE *)context == context_buffer, "Got unexpected context %p.\n", context);

            length2 = 0xdeadbeef;
            p = pLocateXStateFeature(context, 0, &length2);
            if (flags & CONTEXT_NATIVE)
                ok(p == (BYTE *)context + context_arch[test].xsavearea_offset
                    && length2 == offsetof(XSAVE_FORMAT, XmmRegisters),
                    "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);
            else
                ok(!p && length2 == 0xdeadbeef, "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);

            length2 = 0xdeadbeef;
            p = pRtlLocateExtendedFeature(context_ex, 2, &length2);
            ok((!p && length2 == sizeof(YMMCONTEXT))
                    || broken(p && length2 == sizeof(YMMCONTEXT)) /* win10pro */,
                    "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);

            length2 = 0xdeadbeef;
            p = pLocateXStateFeature(context, 2, &length2);
            ok(!p && length2 == (flags & CONTEXT_NATIVE) ? sizeof(YMMCONTEXT) : 0xdeadbeef,
                    "Got unexpected p %p, length %#lx, flags %#lx.\n", p, length2, flags);

            context_flags = *(DWORD *)(context_buffer + context_arch[test].flags_offset);
            ok(context_flags == flags, "Got unexpected ContextFlags %#lx, flags %#lx.\n", context_flags, flags);

            context_ex = (CONTEXT_EX *)(context_buffer + context_arch[test].context_length);
            ok(context_ex->Legacy.Offset == -(int)context_arch[test].context_length,
                    "Got unexpected Offset %ld, flags %#lx.\n", context_ex->Legacy.Offset, flags);
            ok(context_ex->Legacy.Length == ((flags & 0x20) ? context_arch[test].context_length
                    : context_arch[test].legacy_length),
                    "Got unexpected Length %#lx, flags %#lx.\n", context_ex->Legacy.Length, flags);

            expected_offset = (((ULONG_PTR)context + context_arch[test].context_length
                    + context_arch[test].context_ex_length + 63) & ~(ULONG64)63) - (ULONG_PTR)context
                    - context_arch[test].context_length;
            ok(context_ex->XState.Offset == expected_offset,
                    "Got unexpected Offset %ld, flags %#lx.\n", context_ex->XState.Offset, flags);
            ok(context_ex->XState.Length == sizeof(XSTATE) - sizeof(YMMCONTEXT)
                    || broken(context_ex->XState.Length == sizeof(XSTATE)) /* win10pro */,
                    "Got unexpected Length %#lx, flags %#lx.\n", context_ex->XState.Length, flags);

            ok(context_ex->All.Offset == -(int)context_arch[test].context_length,
                    "Got unexpected Offset %ld, flags %#lx.\n", context_ex->All.Offset, flags);
            /* No extra 8 bytes in x64 CONTEXT_EX here. */
            ok(context_ex->All.Length == context_arch[test].context_length
                    + context_ex->XState.Offset + context_ex->XState.Length,
                    "Got unexpected Length %#lx, flags %#lx.\n", context_ex->All.Length, flags);

            expected_compaction = compaction_enabled ? (ULONG64)1 << 63 : 0;
            xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);
            ok(!xs->Mask, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
            ok(xs->CompactionMask == expected_compaction,
                    "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(xs->CompactionMask));
            ok(!xs->Reserved[0], "Got unexpected Reserved[0]  %s.\n", wine_dbgstr_longlong(xs->Reserved[0]));

            pRtlSetExtendedFeaturesMask(context_ex, ~(ULONG64)0);
            ok(xs->Mask == (enabled_features & ~(ULONG64)3), "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
            ok(xs->CompactionMask == expected_compaction, "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(xs->CompactionMask));
        }
    }

    length = 0xdeadbeef;
    ret = pRtlGetExtendedContextLength(context_arch[0].flag | context_arch[1].flag, &length);
    ok(ret == STATUS_INVALID_PARAMETER && length == 0xdeadbeef, "Got unexpected result ret %#lx, length %#lx.\n",
            ret, length);

    if (0)
    {
        /* Crashes on Windows. */
        pRtlGetExtendedContextLength(CONTEXT_FULL, NULL);
        length = sizeof(context_buffer);
        pInitializeContext(context_buffer, CONTEXT_FULL, NULL, &length);
        pInitializeContext(context_buffer, CONTEXT_FULL, &context, NULL);
    }

    if (!(enabled_features & (1 << XSTATE_AVX)))
    {
        skip("AVX is not supported.\n");
        return;
    }

    /* Test RtlCaptureContext (doesn't support xstates). */
    length = sizeof(context_buffer);
    memset(context_buffer, 0xcc, sizeof(context_buffer));
    bret = pInitializeContext(context_buffer, CONTEXT_XSTATE, &context, &length);
    ok(bret, "Got unexpected bret %#x.\n", bret);
    context_ex = (CONTEXT_EX *)(context + 1);
    xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);

    max_xstate_length = context_ex->XState.Length;
    ok(max_xstate_length >= sizeof(XSTATE), "XSTATE size: %#lx; min: %#Ix.\n", max_xstate_length, sizeof(XSTATE));

    *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_addr) = RtlCaptureContext;
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_param1) = context;
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_param2) = (void *)0xdeadbeef;
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.ymm0_save) = data;
    memcpy(code_mem, call_func_code_set_ymm0, sizeof(call_func_code_set_ymm0));

    memcpy(data, test_extended_context_data, sizeof(data));
    func();
    ok(context->ContextFlags == (CONTEXT_FULL | CONTEXT_SEGMENTS), "Got unexpected ContextFlags %#lx.\n",
            context->ContextFlags);
    for (i = 0; i < 8; ++i)
        ok(data[i] == test_extended_context_data[i], "Got unexpected data %#x, i %u.\n", data[i], i);

    /* Test GetThreadContext (current thread, ymm0 set). */
    length = sizeof(context_buffer);
    memset(context_buffer, 0xcc, sizeof(context_buffer));
    bret = pInitializeContext(context_buffer, CONTEXT_FULL | CONTEXT_XSTATE | CONTEXT_FLOATING_POINT,
            &context, &length);
    ok(bret, "Got unexpected bret %#x.\n", bret);
    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));

    expected_flags = CONTEXT_FULL | CONTEXT_XSTATE | CONTEXT_FLOATING_POINT;
#ifdef __i386__
    expected_flags |= CONTEXT_EXTENDED_REGISTERS;
#endif
    pSetXStateFeaturesMask(context, ~(ULONG64)0);
    ok(context->ContextFlags == expected_flags, "Got unexpected ContextFlags %#lx.\n",
            context->ContextFlags);
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_addr) = GetThreadContext;
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_param1) = (void *)GetCurrentThread();
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_param2) = context;
    *(void **)(call_func_code_set_ymm0 + call_func_offsets.ymm0_save) = data;
    memcpy(code_mem, call_func_code_set_ymm0, sizeof(call_func_code_set_ymm0));
    xs->CompactionMask = 2;
    xs->Mask = compaction_enabled ? 2 : 0;
    context_ex->XState.Length = sizeof(XSTATE);

    bret = func();
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

    ok(context->ContextFlags == expected_flags, "Got unexpected ContextFlags %#lx.\n",
            context->ContextFlags);
    expected_compaction = compaction_enabled ? (ULONG64)1 << 63 : 0;

    ok(!xs->Mask || broken(xs->Mask == 4) /* win10pro */,
            "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
    ok(xs->CompactionMask == expected_compaction, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));

    for (i = 4; i < 8; ++i)
        ok(data[i] == test_extended_context_data[i], "Got unexpected data %#x, i %u.\n", data[i], i);

    for (i = 0; i < 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == (xs->Mask == 4 ? test_extended_context_data[i + 4] : 0xcccccccc),
                "Got unexpected data %#lx, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    expected_compaction = compaction_enabled ? ((ULONG64)1 << 63) | 4 : 0;

    xs->CompactionMask = 4;
    xs->Mask = compaction_enabled ? 0 : 4;
    context_ex->XState.Length = max_xstate_length + 64;
    bret = func();
    ok(!bret && GetLastError() == ERROR_INVALID_PARAMETER,
            "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    ok(context->ContextFlags == expected_flags, "Got unexpected ContextFlags %#lx.\n",
            context->ContextFlags);
    ok(xs->Mask == (compaction_enabled ? 0 : 4), "Got unexpected Mask %#I64x.\n", xs->Mask);
    ok(xs->CompactionMask == 4, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));
    for (i = 0; i < 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == 0xcccccccc
                || broken(((ULONG *)&xs->YmmContext)[i] == test_extended_context_data[i + 4]) /* win10pro */,
                "Got unexpected data %#lx, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    xs->CompactionMask = 4;
    xs->Mask = compaction_enabled ? 0 : 4;
    context_ex->XState.Length = offsetof(XSTATE, YmmContext);
    bret = func();
    ok(context->ContextFlags == expected_flags, "Got unexpected ContextFlags %#lx.\n",
            context->ContextFlags);
    ok(!bret && GetLastError() == ERROR_MORE_DATA,
            "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    ok(xs->Mask == 4, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
    ok(xs->CompactionMask == expected_compaction, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));
    for (i = 0; i < 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == 0xcccccccc
                || broken(((ULONG *)&xs->YmmContext)[i] == test_extended_context_data[i + 4]) /* win10pro */,
                "Got unexpected data %#lx, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    context_ex->XState.Length = sizeof(XSTATE);
    xs->CompactionMask = 4;
    xs->Mask = compaction_enabled ? 0 : 4;
    bret = func();
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

    ok(context->ContextFlags == expected_flags, "Got unexpected ContextFlags %#lx.\n",
            context->ContextFlags);

    ok(xs->Mask == 4, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
    ok(xs->CompactionMask == expected_compaction, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));

    for (i = 4; i < 8; ++i)
        ok(data[i] == test_extended_context_data[i], "Got unexpected data %#x, i %u.\n", data[i], i);

    for (i = 0; i < 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == test_extended_context_data[i + 4],
                "Got unexpected data %#lx, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    /* Test GetThreadContext (current thread, ymm state cleared). */
    length = sizeof(context_buffer);
    memset(context_buffer, 0xcc, sizeof(context_buffer));
    bret = pInitializeContext(context_buffer, CONTEXT_FULL | CONTEXT_XSTATE | CONTEXT_FLOATING_POINT,
            &context, &length);
    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));
    ok(bret, "Got unexpected bret %#x.\n", bret);

    /* clear potentially leftover xstate */
    pSetXStateFeaturesMask(context, 0);
    context->ContextFlags = CONTEXT_XSTATE;
    SetThreadContext(GetCurrentThread(), context);

    context->ContextFlags = CONTEXT_FULL | CONTEXT_XSTATE | CONTEXT_FLOATING_POINT;

    pSetXStateFeaturesMask(context, ~(ULONG64)0);
    *(void **)(call_func_code_reset_ymm_state + call_func_offsets.func_addr) = GetThreadContext;
    *(void **)(call_func_code_reset_ymm_state + call_func_offsets.func_param1) = (void *)GetCurrentThread();
    *(void **)(call_func_code_reset_ymm_state + call_func_offsets.func_param2) = context;
    *(void **)(call_func_code_reset_ymm_state + call_func_offsets.ymm0_save) = data;
    memcpy(code_mem, call_func_code_reset_ymm_state, sizeof(call_func_code_reset_ymm_state));

    bret = func();
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

    expected_flags = CONTEXT_FULL | CONTEXT_XSTATE | CONTEXT_FLOATING_POINT;
#ifdef __i386__
    expected_flags |= CONTEXT_EXTENDED_REGISTERS;
#endif
    ok(context->ContextFlags == expected_flags, "Got unexpected ContextFlags %#lx.\n",
            context->ContextFlags);

    expected_compaction = compaction_enabled ? ((ULONG64)1 << 63) | (xstate_supported_features & ~(UINT64)3) : 0;

    xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);
    ok((xs->Mask & supported_features) == (xsaveopt_enabled ? 0 : 4), "Got unexpected Mask %#I64x.\n", xs->Mask);
    ok((xs->CompactionMask & (supported_features | ((ULONG64)1 << 63))) == expected_compaction,
            "Got unexpected CompactionMask %s (expected %#I64x).\n", wine_dbgstr_longlong(xs->CompactionMask), expected_compaction);

    for (i = 4; i < 8; ++i)
        ok(!data[i], "Got unexpected data %#x, i %u.\n", data[i], i);

    for (i = 0; i < 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == ((xs->Mask & 4) ? 0 : 0xcccccccc)
                || broken(((ULONG *)&xs->YmmContext)[i] == test_extended_context_data[i + 4]),
                "Got unexpected data %#lx, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    /* Test setting context which has only part of xstate in CompactionMask. */
    if (compaction_enabled && enabled_features & ((ULONG64)1 << XSTATE_AVX512_KMASK))
    {
        *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_addr) = SetThreadContext;
        *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_param1) = (void *)GetCurrentThread();
        *(void **)(call_func_code_set_ymm0 + call_func_offsets.func_param2) = context;
        *(void **)(call_func_code_set_ymm0 + call_func_offsets.ymm0_save) = data;
        memcpy(code_mem, call_func_code_set_ymm0, sizeof(call_func_code_set_ymm0));
        context->ContextFlags = CONTEXT_XSTATE;
        xs->CompactionMask = 0x8000000000000000 | ((ULONG64)1 << XSTATE_AVX512_KMASK);
        xs->Mask = 0;
        memcpy(data, test_extended_context_data, sizeof(data));
        bret = func();
        ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
        /* Setting a context with only part of xstate in CompactionMask doesn't change missing parts. */
        for (i = 4; i < 8; ++i)
            ok(data[i] == test_extended_context_data[i], "Got unexpected data %#x, i %u.\n", data[i], i);

        memcpy(data, test_extended_context_data, sizeof(data));
        xs->CompactionMask |= XSTATE_MASK_GSSE;
        bret = func();
        ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
        for (i = 4; i < 8; ++i)
            ok(!data[i], "Got unexpected data %#x, i %u.\n", data[i], i);
    }
    else
    {
        skip("avx512 is not available, skipping test.\n");
    }

    /* Test fault exception context. */
    memset(data, 0xff, sizeof(data));
    xs->Mask = 0;
    test_extended_context_modified_state = FALSE;
    run_exception_test(test_extended_context_handler, NULL, except_code_reset_ymm_state,
            ARRAY_SIZE(except_code_reset_ymm_state), PAGE_EXECUTE_READ);
    for (i = 0; i < 8; ++i)
    {
        /* Older Windows version do not reset AVX context to INIT_STATE on x86. */
        ok(!data[i] || broken(i >= 4 && sizeof(void *) == 4 && data[i] == test_extended_context_spoil_data2[i]),
                "Got unexpected data %#x, i %u.\n", data[i], i);
    }

    memcpy(data, test_extended_context_data, sizeof(data));
    test_extended_context_modified_state = TRUE;
    run_exception_test(test_extended_context_handler, NULL, except_code_set_ymm0,
            ARRAY_SIZE(except_code_set_ymm0), PAGE_EXECUTE_READ);

    for (i = 0; i < 8; ++i)
        ok(data[i] == test_extended_context_data[i], "Got unexpected data %#x, i %u.\n", data[i], i);

    /* Test GetThreadContext for the other thread. */
    thread = CreateThread(NULL, 0, test_extended_context_thread, 0, CREATE_SUSPENDED, NULL);
    ok(!!thread, "Failed to create thread.\n");

    bret = pInitializeContext(context_buffer, CONTEXT_FULL | CONTEXT_XSTATE | CONTEXT_FLOATING_POINT,
            &context, &length);
    ok(bret, "Got unexpected bret %#x.\n", bret);
    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));
    context_ex = (CONTEXT_EX *)(context + 1);
    xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);
    pSetXStateFeaturesMask(context, 4);

    bret = GetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    todo_wine_if (!xsaveopt_enabled)
    ok((xs->Mask & supported_features) == (xsaveopt_enabled ? 0 : 4), "Got unexpected Mask %#I64x.\n", xs->Mask);
    ok((xs->CompactionMask & supported_compaction_mask) == expected_compaction,
            "Got unexpected CompactionMask %I64x, expected %I64x.\n", xs->CompactionMask,
            expected_compaction);

    for (i = 0; i < 16 * 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == ((xs->Mask & 4) ? 0 : 0xcccccccc),
                "Got unexpected value %#lx, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    pSetXStateFeaturesMask(context, 4);
    memset(&xs->YmmContext, 0, sizeof(xs->YmmContext));
    bret = SetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));
    bret = GetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    ok(!(xs->Mask & supported_features) || broken((xs->Mask & supported_features) == 4), "Got unexpected Mask %s.\n",
            wine_dbgstr_longlong(xs->Mask));
    ok((xs->CompactionMask & supported_compaction_mask) == expected_compaction, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));
    for (i = 0; i < 16 * 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == 0xcccccccc || broken(xs->Mask == 4 && !((ULONG *)&xs->YmmContext)[i]),
                "Got unexpected value %#lx, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);

    pSetXStateFeaturesMask(context, 4);
    memset(&xs->YmmContext, 0x28, sizeof(xs->YmmContext));
    bret = SetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));
    bret = GetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    ok((xs->Mask & supported_features) == 4, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));
    ok((xs->CompactionMask & supported_compaction_mask) == expected_compaction, "Got unexpected CompactionMask %s.\n",
            wine_dbgstr_longlong(xs->CompactionMask));
    for (i = 0; i < 16 * 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == 0x28282828, "Got unexpected value %#lx, i %u.\n",
                ((ULONG *)&xs->YmmContext)[i], i);

    wait_for_thread_next_suspend(thread);

    bret = GetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    pSetXStateFeaturesMask(context, 4);
    memset(&xs->YmmContext, 0x48, sizeof(xs->YmmContext));
    bret = SetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

    wait_for_thread_next_suspend(thread);

    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));
    bret = GetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    ok((xs->Mask & supported_features) == 4, "Got unexpected Mask %s.\n", wine_dbgstr_longlong(xs->Mask));

    for (i = 0; i < 4; ++i)
        ok(((ULONG *)&xs->YmmContext)[i] == 0x68686868, "Got unexpected value %#lx, i %u.\n",
                ((ULONG *)&xs->YmmContext)[i], i);

    wait_for_thread_next_suspend(thread);

    memset(&xs->YmmContext, 0xcc, sizeof(xs->YmmContext));
    bret = GetThreadContext(thread, context);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    todo_wine_if (!xsaveopt_enabled && sizeof(void *) != 4)
        ok((xs->Mask & supported_features) == (xsaveopt_enabled ? 0 : 4)
                || (sizeof(void *) == 4 && (xs->Mask & supported_features) == 4),
                "Got unexpected Mask %#I64x, supported_features.\n", xs->Mask);
    if ((xs->Mask & supported_features) == 4)
    {
        for (i = 0; i < 8 * sizeof(void *); ++i)
            ok(((ULONG *)&xs->YmmContext)[i] == 0,
                    "Got unexpected value %#lx, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);
        for (; i < 16 * 4; ++i)
            ok(((ULONG *)&xs->YmmContext)[i] == 0x48484848,
                    "Got unexpected value %#lx, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);
    }
    else
    {
        for (i = 0; i < 16 * 4; ++i)
            ok(((ULONG *)&xs->YmmContext)[i] == 0xcccccccc,
                    "Got unexpected value %#lx, i %u.\n", ((ULONG *)&xs->YmmContext)[i], i);
    }

    if (compaction_enabled && enabled_features & ((ULONG64)1 << XSTATE_AVX512_KMASK))
    {
        ULONG64 saved_mask;
        ULONG *d;

        saved_mask = xs->CompactionMask;
        xs->Mask = XSTATE_MASK_GSSE;
        xs->CompactionMask = 0x8000000000000000 | xs->Mask;
        *(ULONG *)&xs->YmmContext = 0x11111111;
        bret = SetThreadContext(thread, context);
        ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

        xs->Mask = (ULONG64)1 << XSTATE_AVX512_KMASK;
        xs->CompactionMask = 0x8000000000000000 | xs->Mask;
        *(ULONG *)&xs->YmmContext = 0x22222222;
        bret = SetThreadContext(thread, context);
        ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

        xs->CompactionMask = saved_mask;
        bret = GetThreadContext(thread, context);
        ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

        todo_wine_if(xs->Mask == XSTATE_MASK_GSSE)
        ok((xs->Mask & (XSTATE_MASK_GSSE | ((ULONG64)1 << XSTATE_AVX512_KMASK)))
                == (XSTATE_MASK_GSSE | ((ULONG64)1 << XSTATE_AVX512_KMASK)), "got Mask %#I64x.\n", xs->Mask);
        d = pLocateXStateFeature(context, XSTATE_AVX, NULL);
        ok(!!d, "Got NULL.\n");
        ok(*d == 0x11111111, "got %#lx.\n", *d);

        d = pLocateXStateFeature(context, XSTATE_AVX512_KMASK, NULL);
        ok(!!d, "Got NULL.\n");
        todo_wine ok(*d == 0x22222222, "got %#lx.\n", *d);
    }
    else
    {
        skip("avx512 is not available, skipping test.\n");
    }

    bret = ResumeThread(thread);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

struct modified_range
{
    ULONG start;
    ULONG flag;
};

#define check_changes_in_range(a, b, c, d) check_changes_in_range_(__FILE__, __LINE__, a, b, c, d)
static void check_changes_in_range_(const char *file, unsigned int line, const BYTE *p,
        const struct modified_range *range, ULONG flags, unsigned int length)
{
    ULONG range_flag, flag;
    unsigned int once = 0;
    unsigned int i;

    range_flag = 0;
    for (i = 0; i < length; i++)
    {
        if (i == range->start)
        {
            range_flag = range->flag;
            ++range;
        }

        if ((flag = range_flag) == ~0)
            continue;

        if (flag & 0x80000000)
        {
            if (flag & flags && p[i] == 0xcc)
            {
                if (!once++)
                    ok(broken(1), "Matched broken result at %#x, flags %#lx.\n", i, flags);
                continue;
            }
            flag = 0;
        }

        if (flag & flags && p[i] != 0xcc)
        {
            ok_(file, line)(0, "Got unexpected byte %#x at %#x, flags %#lx.\n", p[i], i, flags);
            return;
        }
        else if (!(flag & flags) && p[i] != 0xdd)
        {
            ok_(file, line)(0, "Got unexpected byte %#x at %#x, flags %#lx.\n", p[i], i, flags);
            return;
        }
    }
    ok_(file, line)(1, "Range matches.\n");
}

static void test_copy_context(void)
{
    static struct modified_range ranges_amd64[] =
    {
        {0x30, ~0}, {0x38, 0x1}, {0x3a, 0x4}, {0x42, 0x1}, {0x48, 0x10}, {0x78, 0x2}, {0x98, 0x1},
        {0xa0, 0x2}, {0xf8, 0x1}, {0x100, 0x8}, {0x2a0, 0x80000008}, {0x4b0, 0x10}, {0x4d0, ~0},
        {0x4e8, 0}, {0x500, ~0}, {0x640, 0}, {0x1000, 0},
    };
    static struct modified_range ranges_x86[] =
    {
        {0x0, ~0}, {0x4, 0x10}, {0x1c, 0x8}, {0x8c, 0x4}, {0x9c, 0x2}, {0xb4, 0x1}, {0xcc, 0x20}, {0x1ec, 0x80000020},
        {0x2cc, ~0}, {0x440, 0}, {0x1000, 0},
    };
    static const struct modified_range single_range[] =
    {
        {0x0, 0x1}, {0x1000, 0},
    };

    static const ULONG tests[] =
    {
        /* AMD64 */
        CONTEXT_AMD64_CONTROL,
        CONTEXT_AMD64_INTEGER,
        CONTEXT_AMD64_SEGMENTS,
        CONTEXT_AMD64_FLOATING_POINT,
        CONTEXT_AMD64_DEBUG_REGISTERS,
        CONTEXT_AMD64_FULL,
        CONTEXT_AMD64_XSTATE,
        CONTEXT_AMD64_ALL,
        /* X86 */
        CONTEXT_I386_CONTROL,
        CONTEXT_I386_INTEGER,
        CONTEXT_I386_SEGMENTS,
        CONTEXT_I386_FLOATING_POINT,
        CONTEXT_I386_DEBUG_REGISTERS,
        CONTEXT_I386_EXTENDED_REGISTERS,
        CONTEXT_I386_XSTATE,
        CONTEXT_I386_ALL
    };
    static const ULONG arch_flags[] = {CONTEXT_AMD64, CONTEXT_i386};

    DECLSPEC_ALIGN(64) BYTE src_context_buffer[4096];
    DECLSPEC_ALIGN(64) BYTE dst_context_buffer[4096];
    ULONG64 enabled_features, expected_compaction;
    unsigned int context_length, flags_offset, i;
    CONTEXT_EX *src_ex, *dst_ex;
    XSTATE *dst_xs, *src_xs;
    BOOL compaction, bret;
    CONTEXT *src, *dst;
    NTSTATUS status;
    DWORD length;
    ULONG flags;

    if (!pRtlCopyExtendedContext)
    {
        win_skip("RtlCopyExtendedContext is not available.\n");
        return;
    }

    if (!pRtlGetEnabledExtendedFeatures)
    {
        win_skip("RtlGetEnabledExtendedFeatures is not available.\n");
        return;
    }

    enabled_features = pRtlGetEnabledExtendedFeatures(~(ULONG64)0);

    memset(dst_context_buffer, 0xdd, sizeof(dst_context_buffer));
    memset(src_context_buffer, 0xcc, sizeof(src_context_buffer));

    status = pRtlInitializeExtendedContext(src_context_buffer, CONTEXT_ALL | CONTEXT_XSTATE, &src_ex);
    if (!status)
    {
        src = pRtlLocateLegacyContext(src_ex, NULL);
        dst = (CONTEXT *)dst_context_buffer;
        dst->ContextFlags = CONTEXT_ALL;
        status = pRtlCopyContext(dst, dst->ContextFlags, src);
        ok(!status, "Got status %#lx.\n", status);
        check_changes_in_range((BYTE *)dst, CONTEXT_ALL & CONTEXT_AMD64 ? &ranges_amd64[0] : &ranges_x86[0],
                CONTEXT_ALL, sizeof(CONTEXT));
    }
    else
    {
        ok(status == STATUS_NOT_SUPPORTED, "Got status %#lx.\n", status);
        skip("Extended context is not supported.\n");
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        flags = tests[i];
        flags_offset = (flags & CONTEXT_AMD64) ? offsetof(AMD64_CONTEXT,ContextFlags)
                                               : offsetof(I386_CONTEXT,ContextFlags);

        memset(dst_context_buffer, 0xdd, sizeof(dst_context_buffer));
        memset(src_context_buffer, 0xcc, sizeof(src_context_buffer));

        status = pRtlInitializeExtendedContext(src_context_buffer, flags, &src_ex);
        if (enabled_features || !(flags & 0x40))
        {
            ok(!status, "Got unexpected status %#lx, flags %#lx.\n", status, flags);
        }
        else
        {
            ok(status == STATUS_NOT_SUPPORTED, "Got unexpected status %#lx, flags %#lx.\n", status, flags);
            continue;
        }
        status = pRtlInitializeExtendedContext(dst_context_buffer, flags, &dst_ex);
        ok(!status, "Got unexpected status %#lx, flags %#lx.\n", status, flags);

        src = pRtlLocateLegacyContext(src_ex, NULL);
        dst = pRtlLocateLegacyContext(dst_ex, NULL);

        *(DWORD *)((BYTE *)dst + flags_offset) = 0;
        *(DWORD *)((BYTE *)src + flags_offset) = 0;

        context_length = dst_ex->All.Length;

        if (flags & 0x40)
        {
            src_xs = (XSTATE *)((BYTE *)src_ex + src_ex->XState.Offset);
            memset(src_xs, 0xcc, src_ex->XState.Length);
            src_xs->Mask = enabled_features & ~(ULONG64)4;
            src_xs->CompactionMask = ~(ULONG64)0;
            if (flags & CONTEXT_AMD64)
                ranges_amd64[ARRAY_SIZE(ranges_amd64) - 2].start = 0x640 + src_ex->XState.Length - sizeof(XSTATE);
            else
                ranges_x86[ARRAY_SIZE(ranges_x86) - 2].start = 0x440 + src_ex->XState.Length - sizeof(XSTATE);
        }

        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(!status, "Got unexpected status %#lx, flags %#lx.\n", status, flags);

        check_changes_in_range((BYTE *)dst, flags & CONTEXT_AMD64 ? &ranges_amd64[0] : &ranges_x86[0],
                flags, context_length);

        ok(*(DWORD *)((BYTE *)dst + flags_offset) == flags, "Got unexpected ContextFlags %#lx, flags %#lx.\n",
                *(DWORD *)((BYTE *)dst + flags_offset), flags);

        memset(dst_context_buffer, 0xdd, sizeof(dst_context_buffer));
        status = pRtlInitializeExtendedContext(dst_context_buffer, flags, &dst_ex);
        ok(!status, "Got unexpected status %#lx, flags %#lx.\n", status, flags);
        *(DWORD *)((BYTE *)src + flags_offset) = 0;
        *(DWORD *)((BYTE *)dst + flags_offset) = 0;
        SetLastError(0xdeadbeef);
        status = pRtlCopyContext(dst, flags | 0x40, src);
        ok(status == (enabled_features ? STATUS_INVALID_PARAMETER : STATUS_NOT_SUPPORTED)
           || broken(status == STATUS_INVALID_PARAMETER),
           "Got unexpected status %#lx, flags %#lx.\n", status, flags);
        ok(*(DWORD *)((BYTE *)dst + flags_offset) == 0, "Got unexpected ContextFlags %#lx, flags %#lx.\n",
                *(DWORD *)((BYTE *)dst + flags_offset), flags);
        check_changes_in_range((BYTE *)dst, flags & CONTEXT_AMD64 ? &ranges_amd64[0] : &ranges_x86[0],
                0, context_length);

        *(DWORD *)((BYTE *)dst + flags_offset) = flags & (CONTEXT_AMD64 | CONTEXT_i386);
        *(DWORD *)((BYTE *)src + flags_offset) = flags;
        status = pRtlCopyContext(dst, flags, src);
        if (flags & 0x40)
            ok((status == STATUS_BUFFER_OVERFLOW)
               || broken(!(flags & CONTEXT_NATIVE) && status == STATUS_INVALID_PARAMETER),
               "Got unexpected status %#lx, flags %#lx.\n", status, flags);
        else
            ok(!status || broken(!(flags & CONTEXT_NATIVE) && status == STATUS_INVALID_PARAMETER),
               "Got unexpected status %#lx, flags %#lx.\n", status, flags);
        if (!status)
        {
            ok(*(DWORD *)((BYTE *)dst + flags_offset) == flags, "Got unexpected ContextFlags %#lx, flags %#lx.\n",
                    *(DWORD *)((BYTE *)dst + flags_offset), flags);
            check_changes_in_range((BYTE *)dst, flags & CONTEXT_AMD64 ? &ranges_amd64[0] : &ranges_x86[0],
                    flags, context_length);
        }
        else
        {
            ok(*(DWORD *)((BYTE *)dst + flags_offset) == (flags & 0x110000),
                    "Got unexpected ContextFlags %#lx, flags %#lx.\n",
                    *(DWORD *)((BYTE *)dst + flags_offset), flags);
            check_changes_in_range((BYTE *)dst, flags & CONTEXT_AMD64 ? &ranges_amd64[0] : &ranges_x86[0],
                    0, context_length);
        }
    }

    for (i = 0; i < ARRAY_SIZE(arch_flags); ++i)
    {
        flags = arch_flags[i] | 0x42;
        flags_offset = (flags & CONTEXT_AMD64) ? offsetof(AMD64_CONTEXT,ContextFlags)
                                               : offsetof(I386_CONTEXT,ContextFlags);
        context_length = (flags & CONTEXT_AMD64) ? sizeof(AMD64_CONTEXT) : sizeof(I386_CONTEXT);

        memset(dst_context_buffer, 0xdd, sizeof(dst_context_buffer));
        memset(src_context_buffer, 0xcc, sizeof(src_context_buffer));
        length = sizeof(src_context_buffer);
        bret = pInitializeContext(src_context_buffer, flags, &src, &length);
        ok(bret, "Got unexpected bret %#x, flags %#lx.\n", bret, flags);

        length = sizeof(dst_context_buffer);
        bret = pInitializeContext(dst_context_buffer, flags, &dst, &length);
        ok(bret, "Got unexpected bret %#x, flags %#lx.\n", bret, flags);

        dst_ex = (CONTEXT_EX *)((BYTE *)dst + context_length);
        src_ex = (CONTEXT_EX *)((BYTE *)src + context_length);

        dst_xs = (XSTATE *)((BYTE *)dst_ex + dst_ex->XState.Offset);
        src_xs = (XSTATE *)((BYTE *)src_ex + src_ex->XState.Offset);

        *(DWORD *)((BYTE *)dst + flags_offset) = 0;
        *(DWORD *)((BYTE *)src + flags_offset) = 0;

        compaction = !!(src_xs->CompactionMask & ((ULONG64)1 << 63));
        expected_compaction = (compaction ? ((ULONG64)1 << (ULONG64)63) | enabled_features : 0);

        memset(&src_xs->YmmContext, 0xcc, sizeof(src_xs->YmmContext));
        src_xs->CompactionMask = ~(ULONG64)0;

        src_xs->Mask = 0;
        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        dst_ex->XState.Length = 0;
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(status == (enabled_features ? STATUS_BUFFER_OVERFLOW : STATUS_NOT_SUPPORTED),
                "Got unexpected status %#lx, flags %#lx.\n", status, flags);

        if (!enabled_features)
            continue;

        ok(*(DWORD *)((BYTE *)dst + flags_offset) == flags, "Got unexpected ContextFlags %#lx, flags %#lx.\n",
                *(DWORD *)((BYTE *)dst + flags_offset), flags);

        src_xs->Mask = ~(ULONG64)0;

        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        dst_ex->XState.Length = 0;
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(status == STATUS_BUFFER_OVERFLOW, "Got unexpected status %#lx, flags %#lx.\n", status, flags);
        ok(*(DWORD *)((BYTE *)dst + flags_offset) == flags, "Got unexpected ContextFlags %#lx, flags %#lx.\n",
                *(DWORD *)((BYTE *)dst + flags_offset), flags);

        ok(dst_xs->Mask == 0xdddddddddddddddd, "Got unexpected Mask %s.\n",
                wine_dbgstr_longlong(dst_xs->Mask));
        ok(dst_xs->CompactionMask == 0xdddddddddddddddd, "Got unexpected CompactionMask %s.\n",
                wine_dbgstr_longlong(dst_xs->CompactionMask));
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range, 0, sizeof(dst_xs->YmmContext));

        src_xs->Mask = 3;
        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        dst_ex->XState.Length = offsetof(XSTATE, YmmContext);
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(!status, "Got unexpected status %#lx, flags %#lx.\n", status, flags);
        ok(*(DWORD *)((BYTE *)dst + flags_offset) == flags, "Got unexpected ContextFlags %#lx, flags %#lx.\n",
                *(DWORD *)((BYTE *)dst + flags_offset), flags);
        ok(dst_xs->Mask == 0, "Got unexpected Mask %s.\n",
                wine_dbgstr_longlong(dst_xs->Mask));
        ok(dst_xs->CompactionMask == expected_compaction,
                "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(dst_xs->CompactionMask));
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range, 0, sizeof(dst_xs->YmmContext));

        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        dst_ex->XState.Length = sizeof(XSTATE);
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(!status, "Got unexpected status %#lx, flags %#lx.\n", status, flags);
        ok(dst_xs->Mask == 0, "Got unexpected Mask %s.\n",
                wine_dbgstr_longlong(dst_xs->Mask));
        ok(dst_xs->CompactionMask == expected_compaction,
                "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(dst_xs->CompactionMask));
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range, 0, sizeof(dst_xs->YmmContext));

        src_xs->Mask = 4;
        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(!status, "Got unexpected status %#lx, flags %#lx.\n", status, flags);
        ok(dst_xs->Mask == 4, "Got unexpected Mask %s.\n",
                wine_dbgstr_longlong(dst_xs->Mask));
        ok(dst_xs->CompactionMask == expected_compaction,
                "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(dst_xs->CompactionMask));
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range, 1, sizeof(dst_xs->YmmContext));

        src_xs->Mask = 3;
        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(!status, "Got unexpected status %#lx, flags %#lx.\n", status, flags);
        ok(dst_xs->Mask == 0, "Got unexpected Mask %s.\n",
                wine_dbgstr_longlong(dst_xs->Mask));
        ok(dst_xs->CompactionMask == expected_compaction,
                "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(dst_xs->CompactionMask));
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range, 0, sizeof(dst_xs->YmmContext));


        *(DWORD *)((BYTE *)src + flags_offset) = arch_flags[i];

        src_xs->Mask = 7;
        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        status = pRtlCopyExtendedContext(dst_ex, flags, src_ex);
        ok(!status, "Got unexpected status %#lx, flags %#lx.\n", status, flags);
        ok(dst_xs->Mask == 4, "Got unexpected Mask %s.\n",
                wine_dbgstr_longlong(dst_xs->Mask));
        ok(dst_xs->CompactionMask == expected_compaction,
                "Got unexpected CompactionMask %s.\n", wine_dbgstr_longlong(dst_xs->CompactionMask));
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range, 1, sizeof(dst_xs->YmmContext));

        src_xs->Mask = 7;
        memset(&dst_xs->YmmContext, 0xdd, sizeof(dst_xs->YmmContext));
        dst_xs->CompactionMask = 0xdddddddddddddddd;
        dst_xs->Mask = 0xdddddddddddddddd;
        status = pRtlCopyContext(dst, flags, src);
        ok(!status || broken(!(flags & CONTEXT_NATIVE) && status == STATUS_INVALID_PARAMETER),
           "Got unexpected status %#lx, flags %#lx.\n", status, flags);
        ok(dst_xs->Mask == 0xdddddddddddddddd || broken(dst_xs->Mask == 4), "Got unexpected Mask %s, flags %#lx.\n",
                wine_dbgstr_longlong(dst_xs->Mask), flags);
        ok(dst_xs->CompactionMask == 0xdddddddddddddddd || broken(dst_xs->CompactionMask == expected_compaction),
                "Got unexpected CompactionMask %s, flags %#lx.\n", wine_dbgstr_longlong(dst_xs->CompactionMask), flags);
        check_changes_in_range((BYTE *)&dst_xs->YmmContext, single_range,
                dst_xs->Mask == 4, sizeof(dst_xs->YmmContext));
    }
}

#if defined(__i386__)
# define IP_REG(ctx) ctx.Eip
#else
# define IP_REG(ctx) ctx.Rip
#endif

static volatile int exit_ip_test;
static DWORD WINAPI ip_test_thread_proc( void *param )
{
    SetEvent( param );
    while (!exit_ip_test);
    return ERROR_SUCCESS;
}

static void test_set_live_context(void)
{
    UINT_PTR old_ip, target;
    HANDLE thread, event;
    char *target_ptr;
    CONTEXT ctx;
    DWORD res;
    int i;

    /* jmp to self at offset 0 and 4 */
    static const char target_code[] = {0xeb, 0xfe, 0x90, 0x90, 0xeb, 0xfe};

    target_ptr = VirtualAlloc( NULL, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    memcpy( target_ptr, target_code, sizeof(target_code) );
    target = (UINT_PTR)target_ptr;

    event = CreateEventW( NULL, TRUE, FALSE, NULL );
    thread = CreateThread( NULL, 65536, ip_test_thread_proc, event, 0, NULL );
    ok( thread != NULL, "Failed to create thread: %lx\n", GetLastError() );
    res = WaitForSingleObject( event, 1000 );
    ok( !res, "wait returned: %ld\n", res );
    CloseHandle( event );

    memset( &ctx, 0, sizeof(ctx) );
    ctx.ContextFlags = CONTEXT_ALL;
    res = GetThreadContext( thread, &ctx );
    ok( res, "Failed to get thread context: %lx\n", GetLastError() );
    old_ip = IP_REG(ctx);

    IP_REG(ctx) = target;
    res = SetThreadContext( thread, &ctx );
    ok( res, "Failed to set thread context: %lx\n", GetLastError() );

    for (i = 0; i < 10; i++)
    {
        IP_REG(ctx) = target;
        res = SetThreadContext( thread, &ctx );
        ok( res, "Failed to set thread context: %lx\n", GetLastError() );

        ctx.ContextFlags = CONTEXT_ALL;
        res = GetThreadContext( thread, &ctx );
        ok( res, "Failed to get thread context: %lx\n", GetLastError() );
        ok( IP_REG(ctx) == target, "IP = %p, expected %p\n", (void *)IP_REG(ctx), target_ptr );

        IP_REG(ctx) = target + 4;
        res = SetThreadContext( thread, &ctx );
        ok( res, "Failed to set thread context: %lx\n", GetLastError()) ;

        ctx.ContextFlags = CONTEXT_ALL;
        res = GetThreadContext( thread, &ctx );
        ok( res, "Failed to get thread context: %lx\n", GetLastError() );
        ok( IP_REG(ctx) == target + 4, "IP = %p, expected %p\n", (void *)IP_REG(ctx), target_ptr + 4 );
    }

    exit_ip_test = 1;
    ctx.ContextFlags = CONTEXT_ALL;
    IP_REG(ctx) = old_ip;
    res = SetThreadContext( thread, &ctx );
    ok( res, "Failed to restore thread context: %lx\n", GetLastError() );

    res = WaitForSingleObject( thread, 1000 );
    ok( !res, "wait returned: %ld\n", res );

    VirtualFree( target_ptr, 0, MEM_RELEASE );
}
#endif

static void test_backtrace(void)
{
    void *buffer[1024];
    WCHAR name[MAX_PATH];
    void *module;
    ULONG hash, hash_expect;
    int i, count = RtlCaptureStackBackTrace( 0, 1024, buffer, &hash );

    ok( count > 0, "got %u entries\n", count );
    for (i = hash_expect = 0; i < count; i++) hash_expect += (ULONG_PTR)buffer[i];
    ok( hash == hash_expect, "hash mismatch %lx / %lx\n", hash, hash_expect );
    pRtlPcToFileHeader( buffer[0], &module );
    if (is_arm64ec && module == hntdll)  /* Windows arm64ec has an extra frame for the entry thunk */
    {
        ok( count > 1, "wrong count %u\n", count );
        pRtlPcToFileHeader( buffer[1], &module );
    }
    GetModuleFileNameW( module, name, ARRAY_SIZE(name) );
    ok( module == GetModuleHandleA(0), "wrong module %p %s / %p for %p\n",
        module, debugstr_w(name), GetModuleHandleA(0), buffer[0]);

    if (pRtlGetCallersAddress)
    {
        void *caller, *parent;

        caller = parent = (void *)0xdeadbeef;
        pRtlGetCallersAddress( &caller, &parent );
        ok( caller == (count > 1 ? buffer[1] : NULL) || broken(is_arm64ec), /* caller is entry thunk */
            "wrong caller %p / %p\n", caller, buffer[1] );
        ok( parent == (count > 2 ? buffer[2] : NULL), "wrong parent %p / %p\n", parent, buffer[2] );
    }
    else win_skip( "RtlGetCallersAddress not supported\n" );

    if (count && !buffer[count - 1]) count--;  /* win11 32-bit */
    if (count <= 1) return;
    pRtlPcToFileHeader( buffer[count - 1], &module );
    GetModuleFileNameW( module, name, ARRAY_SIZE(name) );
    ok( module == hntdll, "wrong module %p %s for frame %u %p\n",
        module, debugstr_w(name), count - 1, buffer[count - 1] );
}

struct context_exception_request_thread_param
{
    LONG volatile sync;
    HANDLE event;
};
static volatile int *p_context_exception_request_value;
struct context_exception_request_thread_param *context_exception_request_param;

static LONG CALLBACK test_context_exception_request_handler( EXCEPTION_POINTERS *info )
{
    PEXCEPTION_RECORD rec = info->ExceptionRecord;
    CONTEXT *c = info->ContextRecord;
    DWORD old_prot;

    ok( !(c->ContextFlags & (CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING | CONTEXT_SERVICE_ACTIVE
                             | CONTEXT_EXCEPTION_ACTIVE)), "got %#lx.\n", c->ContextFlags );

    ok( rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION, "got %#lx.\n", rec->ExceptionCode );
    VirtualProtect( (void *)p_context_exception_request_value, sizeof(*p_context_exception_request_value),
                    PAGE_READWRITE, &old_prot );

    WriteRelease( &context_exception_request_param->sync, 5 );
    while (ReadAcquire( &context_exception_request_param->sync ) != 6)
        ;

    return EXCEPTION_CONTINUE_EXECUTION;
}

#ifdef __i386__
static const BYTE call_func64_code[] =
{
    0x58,                               /* pop %eax */
    0x0e,                               /* push %cs */
    0x50,                               /* push %eax */
    0x6a, 0x33,                         /* push $0x33 */
    0xe8, 0x00, 0x00, 0x00, 0x00,       /* call 1f */
    0x83, 0x04, 0x24, 0x05,             /* 1: addl $0x5,(%esp) */
    0xcb,                               /* lret */
    /* in 64-bit mode: */
    0x4c, 0x87, 0xf4,                   /* xchg %r14,%rsp */
    0x55,                               /* push %rbp */
    0x48, 0x89, 0xe5,                   /* mov %rsp,%rbp */
    0x56,                               /* push %rsi */
    0x57,                               /* push %rdi */
    0x41, 0x8b, 0x4e, 0x10,             /* mov 0x10(%r14),%ecx */
    0x41, 0x8b, 0x76, 0x14,             /* mov 0x14(%r14),%esi */
    0x67, 0x8d, 0x04, 0xcd, 0, 0, 0, 0, /* lea 0x0(,%ecx,8),%eax */
    0x83, 0xf8, 0x20,                   /* cmp $0x20,%eax */
    0x7d, 0x05,                         /* jge 1f */
    0xb8, 0x20, 0x00, 0x00, 0x00,       /* mov $0x20,%eax */
    0x48, 0x29, 0xc4,                   /* 1: sub %rax,%rsp */
    0x48, 0x83, 0xe4, 0xf0,             /* and $~15,%rsp */
    0x48, 0x89, 0xe7,                   /* mov %rsp,%rdi */
    0xf3, 0x48, 0xa5,                   /* rep movsq */
    0x48, 0x8b, 0x0c, 0x24,             /* mov (%rsp),%rcx */
    0x48, 0x8b, 0x54, 0x24, 0x08,       /* mov 0x8(%rsp),%rdx */
    0x4c, 0x8b, 0x44, 0x24, 0x10,       /* mov 0x10(%rsp),%r8 */
    0x4c, 0x8b, 0x4c, 0x24, 0x18,       /* mov 0x18(%rsp),%r9 */
    0x41, 0xff, 0x56, 0x08,             /* callq *0x8(%r14) */
    0x48, 0x8d, 0x65, 0xf0,             /* lea -0x10(%rbp),%rsp */
    0x5f,                               /* pop %rdi */
    0x5e,                               /* pop %rsi */
    0x5d,                               /* pop %rbp */
    0x4c, 0x87, 0xf4,                   /* xchg %r14,%rsp */
    0xcb,                               /* lret */
};

static NTSTATUS call_func64( ULONG64 func64, int nb_args, ULONG64 *args, void *code_mem )
{
    NTSTATUS (WINAPI *func)( ULONG64 func64, int nb_args, ULONG64 *args ) = code_mem;

    memcpy( code_mem, call_func64_code, sizeof(call_func64_code) );
    return func( func64, nb_args, args );
}
#endif

static DWORD WINAPI test_context_exception_request_thread( void *arg )
{
#ifdef __i386__
    static BYTE wait_sync_x64_code[] =
    {
        0x89, 0x11,       /* mov %edx,(%rcx) */
        0x83, 0xc2, 0x01, /* add $0x1,%edx */
        0x0f, 0x1f, 0x00, /* 1: nopl   (%rax) */
        0x8b, 0x01,       /* mov (%rcx),%eax */
        0x39, 0xd0,       /* cmp %edx,%eax */
        0x75, 0xfa,       /* jne 1b */
        0xc3,             /* ret */
    };
    ULONG64 args[2];
#endif
    struct context_exception_request_thread_param *p = arg;
    void *vectored_handler;

    context_exception_request_param = p;
    vectored_handler = pRtlAddVectoredExceptionHandler( TRUE, test_context_exception_request_handler );
    ok( !!vectored_handler, "failed.\n" );

    WriteRelease( &p->sync, 1 );
    while (ReadAcquire( &p->sync ) != 2)
        ;

    WaitForSingleObject( p->event, INFINITE );

#ifdef __i386__
    memcpy( (char *)code_mem + 1024, wait_sync_x64_code, sizeof(wait_sync_x64_code) );
    args[0] = (ULONG_PTR)&p->sync;
    args[1] = 3;
    if (is_wow64 && !old_wow64) call_func64( (ULONG64)(ULONG_PTR)code_mem + 1024, ARRAY_SIZE(args), args, code_mem );
#endif

    p_context_exception_request_value = VirtualAlloc( NULL, sizeof(*p_context_exception_request_value),
                                                      MEM_RESERVE | MEM_COMMIT, PAGE_READONLY );
    ok( !!p_context_exception_request_value, "got NULL.\n" );
    *p_context_exception_request_value = 1;
    ok( *p_context_exception_request_value == 1, "got %d.\n", *p_context_exception_request_value );
    VirtualFree( (void *)p_context_exception_request_value, 0, MEM_RELEASE );
    pRtlRemoveVectoredExceptionHandler( vectored_handler );

#ifdef __i386__
    args[1] = 7;
    if (is_wow64 && !old_wow64) call_func64( (ULONG64)(ULONG_PTR)code_mem + 1024, ARRAY_SIZE(args), args, code_mem );
#endif

    return 0;
}

static void test_context_exception_request(void)
{
    struct context_exception_request_thread_param p;
    DWORD expected_flags;
    HANDLE thread;
    CONTEXT c;
    BOOL ret;

    if (!pRtlAddVectoredExceptionHandler || !pRtlRemoveVectoredExceptionHandler)
    {
        skip( "RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler not found.\n" );
        return;
    }

    c.ContextFlags = CONTEXT_CONTROL;
    ret = GetThreadContext( GetCurrentThread(), &c );
    ok( ret, "got error %lu.\n", GetLastError() );
    ok( c.ContextFlags == CONTEXT_CONTROL, "got %#lx.\n", c.ContextFlags );

    expected_flags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING | CONTEXT_SERVICE_ACTIVE;

    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST;
    ret = GetThreadContext( GetCurrentThread(), &c );
    ok( ret, "got error %lu.\n", GetLastError() );
    ok( c.ContextFlags == expected_flags || broken( c.ContextFlags == 0x10001 ) /* Win7 WoW64 */,
        "got %#lx.\n", c.ContextFlags );
    if (c.ContextFlags == 0x10001)
    {
        win_skip( "Old WoW64 behaviour, skipping tests.\n" );
        return;
    }

    ret = DuplicateHandle( GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &thread, 0, TRUE, DUPLICATE_SAME_ACCESS );
    ok( ret, "got error %lu.\n", GetLastError() );
    c.ContextFlags = expected_flags | CONTEXT_EXCEPTION_REQUEST;

    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST;
    ret = GetThreadContext( thread, &c );
    ok( ret, "got error %lu.\n", GetLastError() );
    ok( c.ContextFlags == expected_flags, "got %#lx.\n", c.ContextFlags );
    CloseHandle( thread );

    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING | CONTEXT_SERVICE_ACTIVE
                     | CONTEXT_EXCEPTION_ACTIVE;
    ret = GetThreadContext( GetCurrentThread(), &c );
    ok( ret, "got error %lu.\n", GetLastError() );
    ok( c.ContextFlags == expected_flags, "got %#lx.\n", c.ContextFlags );

    p.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    thread = CreateThread( NULL, 0, test_context_exception_request_thread, &p, CREATE_SUSPENDED, NULL );
    ok( !!thread, "got error %lu.\n", GetLastError() );

    expected_flags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING | CONTEXT_EXCEPTION_ACTIVE;

    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST;
    ret = GetThreadContext( thread, &c );
    ok( ret, "got error %lu.\n", GetLastError() );
    ok( c.ContextFlags == expected_flags || broken( c.ContextFlags == (CONTEXT_CONTROL
        | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING)) /* Win7 64 */, "got %#lx.\n", c.ContextFlags );

    p.sync = 0;
    ResumeThread(thread);

    while (ReadAcquire( &p.sync ) != 1)
        SwitchToThread();
    /* thread is in user code. */
    SuspendThread( thread );

    c.ContextFlags = CONTEXT_CONTROL;
    ret = GetThreadContext( thread, &c );
    ok( ret, "got error %lu.\n", GetLastError() );
    ok( c.ContextFlags == CONTEXT_CONTROL, "got %#lx.\n", c.ContextFlags );

    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING | CONTEXT_SERVICE_ACTIVE;
    ret = SetThreadContext( thread, &c );
    ok( ret, "got error %lu.\n", GetLastError() );

    expected_flags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING;

    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST;
    ret = GetThreadContext( thread, &c );
    ok( ret, "got error %lu.\n", GetLastError() );
    ok( c.ContextFlags == expected_flags, "got %#lx.\n", c.ContextFlags );

    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING | CONTEXT_SERVICE_ACTIVE
                     | CONTEXT_EXCEPTION_ACTIVE;
    ret = GetThreadContext( thread, &c );
    ok( ret, "got error %lu.\n", GetLastError() );
    ok( c.ContextFlags == expected_flags, "got %#lx.\n", c.ContextFlags );

    ResumeThread(thread);
    WriteRelease( &p.sync, 2 );
    /* Try to make sure the thread entered WaitForSingleObject(). */
    Sleep(30);

    expected_flags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING | CONTEXT_SERVICE_ACTIVE;

    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST;
    ret = GetThreadContext( thread, &c );
    ok( ret, "got error %lu.\n", GetLastError() );
    ok( c.ContextFlags == expected_flags, "got %#lx.\n", c.ContextFlags );

    c.ContextFlags = CONTEXT_CONTROL;
    ret = SetThreadContext( thread, &c );
    ok( ret, "got error %lu.\n", GetLastError() );

    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING | CONTEXT_SERVICE_ACTIVE
                     | CONTEXT_EXCEPTION_ACTIVE;
    ret = GetThreadContext( thread, &c );
    ok( ret, "got error %lu.\n", GetLastError() );
    ok( c.ContextFlags == expected_flags, "got %#lx.\n", c.ContextFlags );

    SetEvent( p.event );

    if (is_wow64 && !old_wow64)
    {
        while (ReadAcquire( &p.sync ) != 3)
            SwitchToThread();
        /* thread is in x64 code. */

        expected_flags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING | CONTEXT_EXCEPTION_ACTIVE;

        c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST;
        ret = GetThreadContext( thread, &c );
        ok( ret, "got error %lu.\n", GetLastError() );
        ok( c.ContextFlags == expected_flags, "got %#lx, expected %#lx.\n", c.ContextFlags, expected_flags );

        WriteRelease( &p.sync, 4 );
    }

    while (ReadAcquire( &p.sync ) != 5)
        SwitchToThread();

#if defined(__REACTOS__) && defined(__i386__) && !defined(__GNUC__)
    if (is_wow64)
    {
        win_skip("Skipping on WOW64 with MSVC builds, because it makes the test crash\n");
        CloseHandle( thread );
        CloseHandle( p.event );
        return;
    }
#endif

    expected_flags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING;

    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST;
    ret = GetThreadContext( thread, &c );
    ok( ret, "got error %lu.\n", GetLastError() );
    ok( c.ContextFlags == expected_flags, "got %#lx.\n", c.ContextFlags );

    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING | CONTEXT_SERVICE_ACTIVE
                     | CONTEXT_EXCEPTION_ACTIVE;
    ret = GetThreadContext( thread, &c );
    ok( ret, "got error %lu.\n", GetLastError() );
    ok( c.ContextFlags == expected_flags, "got %#lx.\n", c.ContextFlags );

    WriteRelease( &p.sync, 6 );

    if (is_wow64 && !old_wow64)
    {
        while (ReadAcquire( &p.sync ) != 7)
            SwitchToThread();
        /* thread is in x64 code. */

        expected_flags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING | CONTEXT_EXCEPTION_ACTIVE;

        c.ContextFlags = CONTEXT_CONTROL | CONTEXT_EXCEPTION_REQUEST;
        ret = GetThreadContext( thread, &c );
        ok( ret, "got error %lu.\n", GetLastError() );
        ok( c.ContextFlags == expected_flags, "got %#lx, expected %#lx.\n", c.ContextFlags, expected_flags );

        WriteRelease( &p.sync, 8 );
    }

    WaitForSingleObject( thread, INFINITE );
    CloseHandle( thread );
    CloseHandle( p.event );
}

START_TEST(exception)
{
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");
    hntdll = GetModuleHandleA("ntdll.dll");

    my_argc = winetest_get_mainargs( &my_argv );

    if (my_argc >= 3 && !strcmp(my_argv[2], "suspend_process"))
    {
        suspend_process_proc();
        return;
    }

    code_mem = VirtualAlloc(NULL, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if(!code_mem) {
        trace("VirtualAlloc failed\n");
        return;
    }

#define X(f) p##f = (void*)GetProcAddress(hntdll, #f)
    X(NtGetContextThread);
    X(NtSetContextThread);
    X(NtQueueApcThread);
    X(NtContinueEx);
    X(NtReadVirtualMemory);
    X(NtClose);
    X(RtlUnwind);
    X(RtlRaiseException);
    X(RtlCaptureContext);
    X(NtTerminateProcess);
    X(RtlAddVectoredExceptionHandler);
    X(RtlRemoveVectoredExceptionHandler);
    X(RtlAddVectoredContinueHandler);
    X(RtlRemoveVectoredContinueHandler);
    X(RtlSetUnhandledExceptionFilter);
    X(NtQueryInformationThread);
    X(NtSetInformationProcess);
    X(NtSuspendProcess);
    X(NtRaiseException);
    X(NtResumeProcess);
    X(RtlGetUnloadEventTrace);
    X(RtlGetUnloadEventTraceEx);
    X(RtlGetEnabledExtendedFeatures);
    X(RtlGetExtendedContextLength);
    X(RtlGetExtendedContextLength2);
    X(RtlInitializeExtendedContext);
    X(RtlInitializeExtendedContext2);
    X(RtlLocateExtendedFeature);
    X(RtlLocateLegacyContext);
    X(RtlSetExtendedFeaturesMask);
    X(RtlGetExtendedFeaturesMask);
    X(RtlPcToFileHeader);
    X(RtlGetCallersAddress);
    X(RtlCopyContext);
    X(RtlCopyExtendedContext);
    X(KiUserApcDispatcher);
    X(KiUserCallbackDispatcher);
    X(KiUserExceptionDispatcher);
#ifndef __i386__
    X(RtlRestoreContext);
    X(RtlUnwindEx);
    X(RtlAddFunctionTable);
    X(RtlDeleteFunctionTable);
    X(RtlGetNativeSystemInformation);
#endif

#ifdef __x86_64__
    if (pRtlGetNativeSystemInformation)
    {
        SYSTEM_CPU_INFORMATION info;
        ULONG len;
        if (!pRtlGetNativeSystemInformation( SystemCpuInformation, &info, sizeof(info), &len ))
            is_arm64ec = (info.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64);
    }
#endif
#undef X

#define X(f) p##f = (void*)GetProcAddress(hkernel32, #f)
    X(IsWow64Process);
    if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;
    if (is_wow64)
    {
        TEB64 *teb64 = ULongToPtr( NtCurrentTeb()->GdiBatchCount );

        if (teb64)
        {
            PEB64 *peb64 = ULongToPtr(teb64->Peb);
            old_wow64 = !peb64->LdrData;
        }
    }

    X(InitializeContext);
    X(InitializeContext2);
    X(LocateXStateFeature);
    X(SetXStateFeaturesMask);
    X(GetXStateFeaturesMask);
    X(WaitForDebugEventEx);
#undef X

    if (pRtlAddVectoredExceptionHandler && pRtlRemoveVectoredExceptionHandler)
        have_vectored_api = TRUE;
    else
        skip("RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler not found\n");

    my_argc = winetest_get_mainargs( &my_argv );
    if (my_argc >= 4)
    {
        void *addr;

        if (strcmp(my_argv[2], "fastfail") == 0)
        {
            __fastfail(strtoul(my_argv[3], NULL, 0));
            return;
        }

        sscanf( my_argv[3], "%p", &addr );

        if (addr != &test_stage)
        {
            skip( "child process not mapped at same address (%p/%p)\n", &test_stage, addr);
            return;
        }

        /* child must be run under a debugger */
        if (!NtCurrentTeb()->Peb->BeingDebugged)
        {
            ok(FALSE, "child process not being debugged?\n");
            return;
        }

        if (pRtlRaiseException)
        {
            test_stage = STAGE_RTLRAISE_NOT_HANDLED;
            run_rtlraiseexception_test(0x12345);
            run_rtlraiseexception_test(EXCEPTION_BREAKPOINT);
            run_rtlraiseexception_test(EXCEPTION_INVALID_HANDLE);
            test_stage = STAGE_RTLRAISE_HANDLE_LAST_CHANCE;
            run_rtlraiseexception_test(0x12345);
            run_rtlraiseexception_test(EXCEPTION_BREAKPOINT);
            run_rtlraiseexception_test(EXCEPTION_INVALID_HANDLE);
        }
        else skip( "RtlRaiseException not found\n" );

        test_stage = STAGE_OUTPUTDEBUGSTRINGA_CONTINUE;

        test_outputdebugstring(FALSE, 0, FALSE, 0, 0);
        test_stage = STAGE_OUTPUTDEBUGSTRINGA_NOT_HANDLED;
        test_outputdebugstring(FALSE, 2, TRUE,  0, 0); /* is 2 a Windows bug? */
        test_stage = STAGE_OUTPUTDEBUGSTRINGW_CONTINUE;
        /* depending on value passed DebugContinue we can get the unicode exception or not */
        test_outputdebugstring(TRUE, 0, FALSE, 0, 1);
        test_stage = STAGE_OUTPUTDEBUGSTRINGW_NOT_HANDLED;
        /* depending on value passed DebugContinue we can get the unicode exception or not */
        test_outputdebugstring(TRUE, 2, TRUE, 0, 1); /* is 2 a Windows bug? */
        test_stage = STAGE_RIPEVENT_CONTINUE;
        test_ripevent(0);
        test_stage = STAGE_RIPEVENT_NOT_HANDLED;
        test_ripevent(1);
        test_stage = STAGE_SERVICE_CONTINUE;
        test_debug_service(0);
        test_stage = STAGE_SERVICE_NOT_HANDLED;
        test_debug_service(1);
        test_stage = STAGE_BREAKPOINT_CONTINUE;
        test_breakpoint(0);
        test_stage = STAGE_BREAKPOINT_NOT_HANDLED;
        test_breakpoint(1);
        test_stage = STAGE_EXCEPTION_INVHANDLE_CONTINUE;
        test_closehandle(0, (HANDLE)0xdeadbeef);
        test_closehandle(0, (HANDLE)0x7fffffff);
        test_stage = STAGE_EXCEPTION_INVHANDLE_NOT_HANDLED;
        test_closehandle(1, (HANDLE)0xdeadbeef);
        test_closehandle(1, (HANDLE)~(ULONG_PTR)6);
        test_stage = STAGE_NO_EXCEPTION_INVHANDLE_NOT_HANDLED; /* special cases */
        test_closehandle(0, 0);
        test_closehandle(0, INVALID_HANDLE_VALUE);
        test_closehandle(0, GetCurrentProcess());
        test_closehandle(0, GetCurrentThread());
        test_closehandle(0, (HANDLE)~(ULONG_PTR)2);
        test_closehandle(0, GetCurrentProcessToken());
        test_closehandle(0, GetCurrentThreadToken());
        test_closehandle(0, GetCurrentThreadEffectiveToken());
#if defined(__i386__) || defined(__x86_64__)
        test_stage = STAGE_XSTATE;
        test_debuggee_xstate();
        test_stage = STAGE_XSTATE_LEGACY_SSE;
        test_debuggee_xstate();
        test_stage = STAGE_SEGMENTS;
        test_debuggee_segments();
#endif

        /* rest of tests only run in parent */
        return;
    }

#ifdef __i386__

    test_unwind();
    test_exceptions();
    test_debug_registers();
    test_debug_service(1);
    test_simd_exceptions();
    test_fpu_exceptions();
    test_dpe_exceptions();
    test_prot_fault();
    test_extended_context();
    test_copy_context();
    test_set_live_context();
    test_hwbpt_in_syscall();
    test_instrumentation_callback();

#elif defined(__x86_64__)

#define X(f) p##f = (void*)GetProcAddress(hntdll, #f)
    X(__C_specific_handler);
    X(RtlWow64GetThreadContext);
    X(RtlWow64SetThreadContext);
    X(RtlWow64GetCpuAreaInfo);
#undef X

    test_exceptions();
    test_debug_registers();
    test_debug_registers_wow64();
    test_debug_service(1);
    test_simd_exceptions();
    test_continue();
    test___C_specific_handler();
    test_restore_context();
    test_prot_fault();
    test_dpe_exceptions();
    test_wow64_context();
    test_nested_exception();
    test_collided_unwind();
    test_extended_context();
    test_copy_context();
    test_set_live_context();
    test_unwind_from_apc();
    test_syscall_clobbered_regs();
    test_raiseexception_regs();
    test_hwbpt_in_syscall();
    test_instrumentation_callback();
    test_direct_syscalls();

#elif defined(__aarch64__)

    test_continue();
    test_brk();
    test_nested_exception();
    test_collided_unwind();
    test_restore_context();
    test_mrs_currentel();

#elif defined(__arm__)

    test_nested_exception();
    test_collided_unwind();
    test_restore_context();

#endif

    test_KiUserExceptionDispatcher();
    test_KiUserApcDispatcher();
    test_KiUserCallbackDispatcher();
    test_rtlraiseexception();
    test_debugger(DBG_EXCEPTION_HANDLED, FALSE);
    test_debugger(DBG_CONTINUE, FALSE);
    test_debugger(DBG_EXCEPTION_HANDLED, TRUE);
    test_debugger(DBG_CONTINUE, TRUE);
    test_thread_context();
    test_outputdebugstring(FALSE, 1, FALSE, 0, 0);
    if (pWaitForDebugEventEx)
    {
        test_outputdebugstring(TRUE, 1, FALSE, 1, 1);
        test_outputdebugstring_newmodel();
    }
    else
        skip("Unsupported new unicode debug string model\n");

    test_ripevent(1);
    test_fastfail();
    test_breakpoint(1);
    test_closehandle(0, (HANDLE)0xdeadbeef);
    /* Call of Duty WWII writes to BeingDebugged then closes an invalid handle,
     * crashing the game if an exception is raised. */
    NtCurrentTeb()->Peb->BeingDebugged = 0x98;
    test_closehandle(0, (HANDLE)0xdeadbeef);
    NtCurrentTeb()->Peb->BeingDebugged = 0;

    test_user_apc();
    test_user_callback();
    test_vectored_continue_handler();
    test_suspend_thread();
    test_suspend_process();
    test_unload_trace();
    test_backtrace();
    test_context_exception_request();
    VirtualFree(code_mem, 0, MEM_RELEASE);
}
