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

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x500 /* For NTSTATUS */
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "wine/winternl.h"
#include "wine/exception.h"
#include "wine/test.h"

static void *code_mem;

static struct _TEB * (WINAPI *pNtCurrentTeb)(void);
static NTSTATUS  (WINAPI *pNtGetContextThread)(HANDLE,CONTEXT*);
static NTSTATUS  (WINAPI *pNtSetContextThread)(HANDLE,CONTEXT*);
static NTSTATUS  (WINAPI *pRtlRaiseException)(EXCEPTION_RECORD *rec);
static PVOID     (WINAPI *pRtlUnwind)(PVOID, PVOID, PEXCEPTION_RECORD, PVOID);
static PVOID     (WINAPI *pRtlAddVectoredExceptionHandler)(ULONG first, PVECTORED_EXCEPTION_HANDLER func);
static ULONG     (WINAPI *pRtlRemoveVectoredExceptionHandler)(PVOID handler);
static PVOID     (WINAPI *pRtlAddVectoredContinueHandler)(ULONG first, PVECTORED_EXCEPTION_HANDLER func);
static ULONG     (WINAPI *pRtlRemoveVectoredContinueHandler)(PVOID handler);
static NTSTATUS  (WINAPI *pNtReadVirtualMemory)(HANDLE, const void*, void*, SIZE_T, SIZE_T*);
static NTSTATUS  (WINAPI *pNtTerminateProcess)(HANDLE handle, LONG exit_code);
static NTSTATUS  (WINAPI *pNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
static NTSTATUS  (WINAPI *pNtSetInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG);
static BOOL      (WINAPI *pIsWow64Process)(HANDLE, PBOOL);
static NTSTATUS  (WINAPI *pNtClose)(HANDLE);

#if defined(__x86_64__)
static BOOLEAN   (CDECL *pRtlAddFunctionTable)(RUNTIME_FUNCTION*, DWORD, DWORD64);
static BOOLEAN   (CDECL *pRtlDeleteFunctionTable)(RUNTIME_FUNCTION*);
static BOOLEAN   (CDECL *pRtlInstallFunctionTableCallback)(DWORD64, DWORD64, DWORD, PGET_RUNTIME_FUNCTION_CALLBACK, PVOID, PCWSTR);
static PRUNTIME_FUNCTION (WINAPI *pRtlLookupFunctionEntry)(ULONG64, ULONG64*, UNWIND_HISTORY_TABLE*);
#endif

#ifdef __i386__

#ifndef __WINE_WINTRNL_H
#define ProcessExecuteFlags 0x22
#define MEM_EXECUTE_OPTION_DISABLE   0x01
#define MEM_EXECUTE_OPTION_ENABLE    0x02
#define MEM_EXECUTE_OPTION_PERMANENT 0x08
#endif

static int      my_argc;
static char**   my_argv;
static int      test_stage;

static BOOL is_wow64;

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
};

static int got_exception;
static BOOL have_vectored_api;

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
    exc_frame.frame.Prev = pNtCurrentTeb()->Tib.ExceptionList;
    exc_frame.context = context;

    memcpy(code_mem, code, code_size);
    if(access)
        VirtualProtect(code_mem, code_size, access, &oldaccess);

    pNtCurrentTeb()->Tib.ExceptionList = &exc_frame.frame;
    func();
    pNtCurrentTeb()->Tib.ExceptionList = exc_frame.frame.Prev;

    if(access)
        VirtualProtect(code_mem, code_size, oldaccess, &oldaccess2);
}

static LONG CALLBACK rtlraiseexception_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT context = ExceptionInfo->ContextRecord;
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    trace("vect. handler %08x addr:%p context.Eip:%x\n", rec->ExceptionCode,
          rec->ExceptionAddress, context->Eip);

    ok(rec->ExceptionAddress == (char *)code_mem + 0xb, "ExceptionAddress at %p instead of %p\n",
       rec->ExceptionAddress, (char *)code_mem + 0xb);

    if (pNtCurrentTeb()->Peb->BeingDebugged)
        ok((void *)context->Eax == pRtlRaiseException ||
           broken( is_wow64 && context->Eax == 0xf00f00f1 ), /* broken on vista */
           "debugger managed to modify Eax to %x should be %p\n",
           context->Eax, pRtlRaiseException);

    /* check that context.Eip is fixed up only for EXCEPTION_BREAKPOINT
     * even if raised by RtlRaiseException
     */
    if(rec->ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        ok(context->Eip == (DWORD)code_mem + 0xa ||
           broken(context->Eip == (DWORD)code_mem + 0xb), /* win2k3 */
           "Eip at %x instead of %x or %x\n", context->Eip,
           (DWORD)code_mem + 0xa, (DWORD)code_mem + 0xb);
    }
    else
    {
        ok(context->Eip == (DWORD)code_mem + 0xb, "Eip at %x instead of %x\n",
           context->Eip, (DWORD)code_mem + 0xb);
    }

    /* test if context change is preserved from vectored handler to stack handlers */
    context->Eax = 0xf00f00f0;
    return EXCEPTION_CONTINUE_SEARCH;
}

static DWORD rtlraiseexception_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                      CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    trace( "exception: %08x flags:%x addr:%p context: Eip:%x\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, context->Eip );

    ok(rec->ExceptionAddress == (char *)code_mem + 0xb, "ExceptionAddress at %p instead of %p\n",
       rec->ExceptionAddress, (char *)code_mem + 0xb);

    /* check that context.Eip is fixed up only for EXCEPTION_BREAKPOINT
     * even if raised by RtlRaiseException
     */
    if(rec->ExceptionCode == EXCEPTION_BREAKPOINT)
    {
        ok(context->Eip == (DWORD)code_mem + 0xa ||
           broken(context->Eip == (DWORD)code_mem + 0xb), /* win2k3 */
           "Eip at %x instead of %x or %x\n", context->Eip,
           (DWORD)code_mem + 0xa, (DWORD)code_mem + 0xb);
    }
    else
    {
        ok(context->Eip == (DWORD)code_mem + 0xb, "Eip at %x instead of %x\n",
           context->Eip, (DWORD)code_mem + 0xb);
    }

    if(have_vectored_api)
        ok(context->Eax == 0xf00f00f0, "Eax is %x, should have been set to 0xf00f00f0 in vectored handler\n",
           context->Eax);

    /* give the debugger a chance to examine the state a second time */
    /* without the exception handler changing Eip */
    if (test_stage == 2)
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
    frame.Prev = pNtCurrentTeb()->Tib.ExceptionList;

    memcpy(code_mem, call_one_arg_code, sizeof(call_one_arg_code));

    pNtCurrentTeb()->Tib.ExceptionList = &frame;
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
    pNtCurrentTeb()->Tib.ExceptionList = frame.Prev;
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
    trace("exception: %08x flags:%x addr:%p context: Eip:%x\n",
          rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, context->Eip);

    ok(rec->ExceptionCode == STATUS_UNWIND, "ExceptionCode is %08x instead of %08x\n",
       rec->ExceptionCode, STATUS_UNWIND);
    ok(rec->ExceptionAddress == (char *)code_mem + 0x22, "ExceptionAddress at %p instead of %p\n",
       rec->ExceptionAddress, (char *)code_mem + 0x22);
    ok(context->Eax == unwind_expected_eax, "context->Eax is %08x instead of %08x\n",
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
    frame1->Prev = pNtCurrentTeb()->Tib.ExceptionList;
    pNtCurrentTeb()->Tib.ExceptionList = frame1;

    /* add second unwind handler */
    frame2->Handler = unwind_handler;
    frame2->Prev = pNtCurrentTeb()->Tib.ExceptionList;
    pNtCurrentTeb()->Tib.ExceptionList = frame2;

    /* test unwind to current frame */
    unwind_expected_eax = 0xDEAD0000;
    retval = func(pRtlUnwind, frame2, NULL, 0xDEAD0000);
    ok(retval == 0xDEAD0000, "RtlUnwind returned eax %08x instead of %08x\n", retval, 0xDEAD0000);
    ok(pNtCurrentTeb()->Tib.ExceptionList == frame2, "Exception record points to %p instead of %p\n",
       pNtCurrentTeb()->Tib.ExceptionList, frame2);

    /* unwind to frame1 */
    unwind_expected_eax = 0xDEAD0000;
    retval = func(pRtlUnwind, frame1, NULL, 0xDEAD0000);
    ok(retval == 0xDEAD0001, "RtlUnwind returned eax %08x instead of %08x\n", retval, 0xDEAD0001);
    ok(pNtCurrentTeb()->Tib.ExceptionList == frame1, "Exception record points to %p instead of %p\n",
       pNtCurrentTeb()->Tib.ExceptionList, frame1);

    /* restore original handler */
    pNtCurrentTeb()->Tib.ExceptionList = frame1->Prev;
}

static DWORD handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                      CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    const struct exception *except = *(const struct exception **)(frame + 1);
    unsigned int i, entry = except - exceptions;

    got_exception++;
    trace( "exception %u: %x flags:%x addr:%p\n",
           entry, rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );

    ok( rec->ExceptionCode == except->status ||
        (except->alt_status != 0 && rec->ExceptionCode == except->alt_status),
        "%u: Wrong exception code %x/%x\n", entry, rec->ExceptionCode, except->status );
    ok( rec->ExceptionAddress == (char*)code_mem + except->offset,
        "%u: Wrong exception address %p/%p\n", entry,
        rec->ExceptionAddress, (char*)code_mem + except->offset );

    if (except->alt_status == 0 || rec->ExceptionCode != except->alt_status)
    {
        ok( rec->NumberParameters == except->nb_params,
            "%u: Wrong number of parameters %u/%u\n", entry, rec->NumberParameters, except->nb_params );
    }
    else
    {
        ok( rec->NumberParameters == except->alt_nb_params,
            "%u: Wrong number of parameters %u/%u\n", entry, rec->NumberParameters, except->nb_params );
    }

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
                "%u: Wrong parameter %d: %lx/%x\n",
                entry, i, rec->ExceptionInformation[i], except->params[i] );
    }
    else
    {
        for (i = 0; i < rec->NumberParameters; i++)
            ok( rec->ExceptionInformation[i] == except->alt_params[i],
                "%u: Wrong parameter %d: %lx/%x\n",
                entry, i, rec->ExceptionInformation[i], except->alt_params[i] );
    }

skip_params:
    /* don't handle exception if it's not the address we expected */
    if (rec->ExceptionAddress != (char*)code_mem + except->offset) return ExceptionContinueSearch;

    context->Eip += except->length;
    return ExceptionContinueExecution;
}

static void test_prot_fault(void)
{
    unsigned int i;

    for (i = 0; i < sizeof(exceptions)/sizeof(exceptions[0]); i++)
    {
        if (is_wow64 && exceptions[i].wow64_broken && !strcmp( winetest_platform, "windows" ))
        {
            skip( "Exception %u broken on Wow64\n", i );
            continue;
        }
        got_exception = 0;
        run_exception_test(handler, &exceptions[i], &exceptions[i].code,
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
    ok((ctx.Dr##n & m) == test->dr##n, "(%d) failed to set debug register " #n " to %x, got %x\n", \
       test_num, test->dr##n, ctx.Dr##n)

static void check_debug_registers(int test_num, const struct dbgreg_test *test)
{
    CONTEXT ctx;
    NTSTATUS status;

    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    status = pNtGetContextThread(GetCurrentThread(), &ctx);
    ok(status == STATUS_SUCCESS, "NtGetContextThread failed with %x\n", status);

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
            "exception is not EXCEPTION_SINGLE_STEP: %x\n", rec->ExceptionCode);
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
    0x58,                  	/* pop    %eax */
    0x0d,0,0,4,0,       	/* or     $0x40000,%eax */
    0x50,                  	/* push   %eax */
    0x9d,                  	/* popf    */
    0x89,0xe0,                  /* mov %esp, %eax */
    0x8b,0x40,0x1,              /* mov 0x1(%eax), %eax - cause exception */
    0x9c,                  	/* pushf   */
    0x58,                  	/* pop    %eax */
    0x35,0,0,4,0,       	/* xor    $0x40000,%eax */
    0x50,                  	/* push   %eax */
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
        "wrong exception code: %x\n", rec->ExceptionCode);

    if(got_exception == 1) {
        /* hw bp exception on first nop */
        ok( context->Eip == (DWORD)code_mem, "eip is wrong:  %x instead of %x\n",
                                             context->Eip, (DWORD)code_mem);
        ok( (context->Dr6 & 0xf) == 1, "B0 flag is not set in Dr6\n");
        ok( !(context->Dr6 & 0x4000), "BS flag is set in Dr6\n");
        context->Dr0 = context->Dr0 + 1;  /* set hw bp again on next instruction */
        context->EFlags |= 0x100;       /* enable single stepping */
    } else if(  got_exception == 2) {
        /* single step exception on second nop */
        ok( context->Eip == (DWORD)code_mem + 1, "eip is wrong: %x instead of %x\n",
                                                 context->Eip, (DWORD)code_mem + 1);
        ok( (context->Dr6 & 0x4000), "BS flag is not set in Dr6\n");
       /* depending on the win version the B0 bit is already set here as well
        ok( (context->Dr6 & 0xf) == 0, "B0...3 flags in Dr6 shouldn't be set\n"); */
        context->EFlags |= 0x100;
    } else if( got_exception == 3) {
        /* hw bp exception on second nop */
        ok( context->Eip == (DWORD)code_mem + 1, "eip is wrong: %x instead of %x\n",
                                                 context->Eip, (DWORD)code_mem + 1);
        ok( (context->Dr6 & 0xf) == 1, "B0 flag is not set in Dr6\n");
        ok( !(context->Dr6 & 0x4000), "BS flag is set in Dr6\n");
        context->Dr0 = 0;       /* clear breakpoint */
        context->EFlags |= 0x100;
    } else {
        /* single step exception on ret */
        ok( context->Eip == (DWORD)code_mem + 2, "eip is wrong: %x instead of %x\n",
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
    ok( context->Eip == (DWORD)code_mem, "eip not at: %p, but at %#x\n", code_mem, context->Eip);
    if(context->Eip == (DWORD)code_mem) context->Eip++; /* skip breakpoint */

    return ExceptionContinueExecution;
}

static const BYTE int3_code[] = { 0xCC, 0xc3 };  /* int 3, ret */


static void test_exceptions(void)
{
    CONTEXT ctx;
    NTSTATUS res;
    struct dbgreg_test dreg_test;

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
    ok( res == STATUS_SUCCESS, "NtSetContextThread failed with %x\n", res);

    got_exception = 0;
    run_exception_test(bpx_handler, NULL, dummy_code, sizeof(dummy_code), 0);
    ok( got_exception == 4,"expected 4 exceptions, got %d\n", got_exception);

    /* test int3 handling */
    run_exception_test(int3_handler, NULL, int3_code, sizeof(int3_code), 0);
}

static void test_debugger(void)
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

    sprintf(cmdline, "%s %s %s %p", my_argv[0], my_argv[1], "debuggee", &test_stage);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "could not create child process error: %u\n", GetLastError());
    if (!ret)
        return;

    do
    {
        continuestatus = DBG_CONTINUE;
        ok(WaitForDebugEvent(&de, INFINITE), "reading debug event\n");

        if (de.dwThreadId != pi.dwThreadId)
        {
            trace("event %d not coming from main thread, ignoring\n", de.dwDebugEventCode);
            ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
            continue;
        }

        if (de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            if(de.u.CreateProcessInfo.lpBaseOfImage != pNtCurrentTeb()->Peb->ImageBaseAddress)
            {
                skip("child process loaded at different address, terminating it\n");
                pNtTerminateProcess(pi.hProcess, 0);
            }
        }
        else if (de.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            CONTEXT ctx;
            int stage;

            counter++;
            status = pNtReadVirtualMemory(pi.hProcess, &code_mem, &code_mem_address,
                                          sizeof(code_mem_address), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);
            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            ctx.ContextFlags = CONTEXT_FULL;
            status = pNtGetContextThread(pi.hThread, &ctx);
            ok(!status, "NtGetContextThread failed with 0x%x\n", status);

            trace("exception 0x%x at %p firstchance=%d Eip=0x%x, Eax=0x%x\n",
                  de.u.Exception.ExceptionRecord.ExceptionCode,
                  de.u.Exception.ExceptionRecord.ExceptionAddress, de.u.Exception.dwFirstChance, ctx.Eip, ctx.Eax);

            if (counter > 100)
            {
                ok(FALSE, "got way too many exceptions, probably caught in an infinite loop, terminating child\n");
                pNtTerminateProcess(pi.hProcess, 1);
            }
            else if (counter >= 2) /* skip startup breakpoint */
            {
                if (stage == 1)
                {
                    ok((char *)ctx.Eip == (char *)code_mem_address + 0xb, "Eip at %x instead of %p\n",
                       ctx.Eip, (char *)code_mem_address + 0xb);
                    /* setting the context from debugger does not affect the context, the exception handlers gets */
                    /* uncomment once wine is fixed */
                    /* ctx.Eip = 0x12345; */
                    ctx.Eax = 0xf00f00f1;

                    /* let the debuggee handle the exception */
                    continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else if (stage == 2)
                {
                    if (de.u.Exception.dwFirstChance)
                    {
                        /* debugger gets first chance exception with unmodified ctx.Eip */
                        ok((char *)ctx.Eip == (char *)code_mem_address + 0xb, "Eip at 0x%x instead of %p\n",
                            ctx.Eip, (char *)code_mem_address + 0xb);

                        /* setting the context from debugger does not affect the context, the exception handlers gets */
                        /* uncomment once wine is fixed */
                        /* ctx.Eip = 0x12345; */
                        ctx.Eax = 0xf00f00f1;

                        /* pass exception to debuggee
                         * exception will not be handled and
                         * a second chance exception will be raised */
                        continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                    }
                    else
                    {
                        /* debugger gets context after exception handler has played with it */
                        /* ctx.Eip is the same value the exception handler got */
                        if (de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                        {
                            ok((char *)ctx.Eip == (char *)code_mem_address + 0xa ||
                               broken(is_wow64 && (char *)ctx.Eip == (char *)code_mem_address + 0xb),
                               "Eip at 0x%x instead of %p\n",
                                ctx.Eip, (char *)code_mem_address + 0xa);
                            /* need to fixup Eip for debuggee */
                            if ((char *)ctx.Eip == (char *)code_mem_address + 0xa)
                                ctx.Eip += 1;
                        }
                        else
                            ok((char *)ctx.Eip == (char *)code_mem_address + 0xb, "Eip at 0x%x instead of %p\n",
                                ctx.Eip, (char *)code_mem_address + 0xb);
                        /* here we handle exception */
                    }
                }
                else if (stage == 7 || stage == 8)
                {
                    ok(de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_INVALID_HANDLE,
                       "unexpected exception code %08x, expected %08x\n", de.u.Exception.ExceptionRecord.ExceptionCode,
                       EXCEPTION_INVALID_HANDLE);
                    ok(de.u.Exception.ExceptionRecord.NumberParameters == 0,
                       "unexpected number of parameters %d, expected 0\n", de.u.Exception.ExceptionRecord.NumberParameters);

                    if (stage == 8) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                else
                    ok(FALSE, "unexpected stage %x\n", stage);

                status = pNtSetContextThread(pi.hThread, &ctx);
                ok(!status, "NtSetContextThread failed with 0x%x\n", status);
            }
        }
        else if (de.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
        {
            int stage;
#ifdef __REACTOS__
            /* This will catch our DPRINTs, such as
             * "WARNING:  RtlpDphTargetDllsLogicInitialize at ..\..\lib\rtl\heappage.c:1283 is UNIMPLEMENTED!"
             * so we need a full-size buffer to avoid a stack overflow
             */
            char buffer[513];
#else
            char buffer[64];
#endif

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            ok(!de.u.DebugString.fUnicode, "unexpected unicode debug string event\n");
            ok(de.u.DebugString.nDebugStringLength < sizeof(buffer) - 1, "buffer not large enough to hold %d bytes\n",
               de.u.DebugString.nDebugStringLength);

            memset(buffer, 0, sizeof(buffer));
            status = pNtReadVirtualMemory(pi.hProcess, de.u.DebugString.lpDebugStringData, buffer,
                                          de.u.DebugString.nDebugStringLength, &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            if (stage == 3 || stage == 4)
                ok(!strcmp(buffer, "Hello World"), "got unexpected debug string '%s'\n", buffer);
            else /* ignore unrelated debug strings like 'SHIMVIEW: ShimInfo(Complete)' */
                ok(strstr(buffer, "SHIMVIEW") != NULL, "unexpected stage %x, got debug string event '%s'\n", stage, buffer);

            if (stage == 4) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }
        else if (de.dwDebugEventCode == RIP_EVENT)
        {
            int stage;

            status = pNtReadVirtualMemory(pi.hProcess, &test_stage, &stage,
                                          sizeof(stage), &size_read);
            ok(!status,"NtReadVirtualMemory failed with 0x%x\n", status);

            if (stage == 5 || stage == 6)
            {
                ok(de.u.RipInfo.dwError == 0x11223344, "got unexpected rip error code %08x, expected %08x\n",
                   de.u.RipInfo.dwError, 0x11223344);
                ok(de.u.RipInfo.dwType  == 0x55667788, "got unexpected rip type %08x, expected %08x\n",
                   de.u.RipInfo.dwType, 0x55667788);
            }
            else
                ok(FALSE, "unexpected stage %x\n", stage);

            if (stage == 6) continuestatus = DBG_EXCEPTION_NOT_HANDLED;
        }

        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, continuestatus);

    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    winetest_wait_child_process( pi.hProcess );
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %u\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "error %u\n", GetLastError());

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

    /* stage 2 - divide by zero fault */
    if( rec->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION)
        skip("system doesn't support SIMD exceptions\n");
    else {
        ok( rec->ExceptionCode ==  STATUS_FLOAT_MULTIPLE_TRAPS,
            "exception code: %#x, should be %#x\n",
            rec->ExceptionCode,  STATUS_FLOAT_MULTIPLE_TRAPS);
        ok( rec->NumberParameters == 1 || broken(is_wow64 && rec->NumberParameters == 2),
            "# of params: %i, should be 1\n",
            rec->NumberParameters);
        if( rec->NumberParameters == 1 )
            ok( rec->ExceptionInformation[0] == 0, "param #1: %lx, should be 0\n", rec->ExceptionInformation[0]);
    }

    context->Eip += 3; /* skip divps */

    return ExceptionContinueExecution;
}

static const BYTE simd_exception_test[] = {
    0x83, 0xec, 0x4,                     /* sub $0x4, %esp       */
    0x0f, 0xae, 0x1c, 0x24,              /* stmxcsr (%esp)       */
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
    0x66, 0x81, 0x0c, 0x24, 0x00, 0x02,  /* orw    $0x200,(%esp) * disable exceptions */
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
    ok( got_exception == 1, "got exception: %i, should be 1\n", got_exception);
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
            "Got exception code %#x, expected EXCEPTION_FLT_STACK_CHECK\n", info.exception_code);
    ok(info.exception_offset == 0x19 ||
       broken( is_wow64 && info.exception_offset == info.eip_offset ),
       "Got exception offset %#x, expected 0x19\n", info.exception_offset);
    ok(info.eip_offset == 0x1b, "Got EIP offset %#x, expected 0x1b\n", info.eip_offset);

    memset(&info, 0, sizeof(info));
    run_exception_test(fpu_exception_handler, &info, fpu_exception_test_de, sizeof(fpu_exception_test_de), 0);
    ok(info.exception_code == EXCEPTION_FLT_DIVIDE_BY_ZERO,
            "Got exception code %#x, expected EXCEPTION_FLT_DIVIDE_BY_ZERO\n", info.exception_code);
    ok(info.exception_offset == 0x17 ||
       broken( is_wow64 && info.exception_offset == info.eip_offset ),
       "Got exception offset %#x, expected 0x17\n", info.exception_offset);
    ok(info.eip_offset == 0x19, "Got EIP offset %#x, expected 0x19\n", info.eip_offset);
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
       "Exception code %08x\n", rec->ExceptionCode);
    ok(rec->NumberParameters == 2,
       "Parameter count: %d\n", rec->NumberParameters);
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
    stat = pNtQueryInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val - 1, &len);
    if(stat == STATUS_INVALID_INFO_CLASS)
    {
        skip("This software platform does not support DEP\n");
        return;
    }
    ok(stat == STATUS_INFO_LENGTH_MISMATCH, "buffer too small: %08x\n", stat);

    /* Query DEP */
    stat = pNtQueryInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val, &len);
    ok(stat == STATUS_SUCCESS, "querying DEP: status %08x\n", stat);
    if(stat == STATUS_SUCCESS)
    {
        ok(len == sizeof val, "returned length: %d\n", len);
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
        ok(stat == STATUS_SUCCESS, "enabling DEP: status %08x\n", stat);
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
        ok(stat == STATUS_SUCCESS, "disabling DEP: status %08x\n", stat);
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
        ok(stat == STATUS_SUCCESS, "disabling DEP permanently: status %08x\n", stat);
    }

    /* Try to turn off DEP */
    val = MEM_EXECUTE_OPTION_ENABLE;
    stat = pNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val);
    ok(stat == STATUS_ACCESS_DENIED, "disabling DEP while permanent: status %08x\n", stat);

    /* Try to turn on DEP */
    val = MEM_EXECUTE_OPTION_DISABLE;
    stat = pNtSetInformationProcess(GetCurrentProcess(), ProcessExecuteFlags, &val, sizeof val);
    ok(stat == STATUS_ACCESS_DENIED, "enabling DEP while permanent: status %08x\n", stat);
}

#elif defined(__x86_64__)

#define UNW_FLAG_NHANDLER  0
#define UNW_FLAG_EHANDLER  1
#define UNW_FLAG_UHANDLER  2
#define UNW_FLAG_CHAININFO 4

#define UWOP_PUSH_NONVOL     0
#define UWOP_ALLOC_LARGE     1
#define UWOP_ALLOC_SMALL     2
#define UWOP_SET_FPREG       3
#define UWOP_SAVE_NONVOL     4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_SAVE_XMM128     8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME  10

struct results
{
    int rip_offset;   /* rip offset from code start */
    int rbp_offset;   /* rbp offset from stack pointer */
    int handler;      /* expect handler to be set? */
    int rip;          /* expected final rip value */
    int frame;        /* expected frame return value */
    int regs[8][2];   /* expected values for registers */
};

struct unwind_test
{
    const BYTE *function;
    size_t function_size;
    const BYTE *unwind_info;
    const struct results *results;
    unsigned int nb_results;
};

enum regs
{
    rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi,
    r8,  r9,  r10, r11, r12, r13, r14, r15
};

static const char * const reg_names[16] =
{
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"
};

#define UWOP(code,info) (UWOP_##code | ((info) << 4))

static void call_virtual_unwind( int testnum, const struct unwind_test *test )
{
    static const int code_offset = 1024;
    static const int unwind_offset = 2048;
    void *handler, *data;
    CONTEXT context;
    RUNTIME_FUNCTION runtime_func;
    KNONVOLATILE_CONTEXT_POINTERS ctx_ptr;
    UINT i, j, k;
    ULONG64 fake_stack[256];
    ULONG64 frame, orig_rip, orig_rbp, unset_reg;
    UINT unwind_size = 4 + 2 * test->unwind_info[2] + 8;

    memcpy( (char *)code_mem + code_offset, test->function, test->function_size );
    memcpy( (char *)code_mem + unwind_offset, test->unwind_info, unwind_size );

    runtime_func.BeginAddress = code_offset;
    runtime_func.EndAddress = code_offset + test->function_size;
    runtime_func.UnwindData = unwind_offset;

    trace( "code: %p stack: %p\n", code_mem, fake_stack );

    for (i = 0; i < test->nb_results; i++)
    {
        memset( &ctx_ptr, 0, sizeof(ctx_ptr) );
        memset( &context, 0x55, sizeof(context) );
        memset( &unset_reg, 0x55, sizeof(unset_reg) );
        for (j = 0; j < 256; j++) fake_stack[j] = j * 8;

        context.Rsp = (ULONG_PTR)fake_stack;
        context.Rbp = (ULONG_PTR)fake_stack + test->results[i].rbp_offset;
        orig_rbp = context.Rbp;
        orig_rip = (ULONG64)code_mem + code_offset + test->results[i].rip_offset;

        trace( "%u/%u: rip=%p (%02x) rbp=%p rsp=%p\n", testnum, i,
               (void *)orig_rip, *(BYTE *)orig_rip, (void *)orig_rbp, (void *)context.Rsp );

        data = (void *)0xdeadbeef;
        handler = RtlVirtualUnwind( UNW_FLAG_EHANDLER, (ULONG64)code_mem, orig_rip,
                                    &runtime_func, &context, &data, &frame, &ctx_ptr );
        if (test->results[i].handler)
        {
            ok( (char *)handler == (char *)code_mem + 0x200,
                "%u/%u: wrong handler %p/%p\n", testnum, i, handler, (char *)code_mem + 0x200 );
            if (handler) ok( *(DWORD *)data == 0x08070605,
                             "%u/%u: wrong handler data %p\n", testnum, i, data );
        }
        else
        {
            ok( handler == NULL, "%u/%u: handler %p instead of NULL\n", testnum, i, handler );
            ok( data == (void *)0xdeadbeef, "%u/%u: handler data set to %p\n", testnum, i, data );
        }

        ok( context.Rip == test->results[i].rip, "%u/%u: wrong rip %p/%x\n",
            testnum, i, (void *)context.Rip, test->results[i].rip );
        ok( frame == (ULONG64)fake_stack + test->results[i].frame, "%u/%u: wrong frame %p/%p\n",
            testnum, i, (void *)frame, (char *)fake_stack + test->results[i].frame );

        for (j = 0; j < 16; j++)
        {
            static const UINT nb_regs = sizeof(test->results[i].regs) / sizeof(test->results[i].regs[0]);

            for (k = 0; k < nb_regs; k++)
            {
                if (test->results[i].regs[k][0] == -1)
                {
                    k = nb_regs;
                    break;
                }
                if (test->results[i].regs[k][0] == j) break;
            }

            if (j == rsp)  /* rsp is special */
            {
                ok( !ctx_ptr.u2.IntegerContext[j],
                    "%u/%u: rsp should not be set in ctx_ptr\n", testnum, i );
                ok( context.Rsp == (ULONG64)fake_stack + test->results[i].regs[k][1],
                    "%u/%u: register rsp wrong %p/%p\n",
                    testnum, i, (void *)context.Rsp, (char *)fake_stack + test->results[i].regs[k][1] );
                continue;
            }

            if (ctx_ptr.u2.IntegerContext[j])
            {
                ok( k < nb_regs, "%u/%u: register %s should not be set to %lx\n",
                    testnum, i, reg_names[j], *(&context.Rax + j) );
                if (k < nb_regs)
                    ok( *(&context.Rax + j) == test->results[i].regs[k][1],
                        "%u/%u: register %s wrong %p/%x\n",
                        testnum, i, reg_names[j], (void *)*(&context.Rax + j), test->results[i].regs[k][1] );
            }
            else
            {
                ok( k == nb_regs, "%u/%u: register %s should be set\n", testnum, i, reg_names[j] );
                if (j == rbp)
                    ok( context.Rbp == orig_rbp, "%u/%u: register rbp wrong %p/unset\n",
                        testnum, i, (void *)context.Rbp );
                else
                    ok( *(&context.Rax + j) == unset_reg,
                        "%u/%u: register %s wrong %p/unset\n",
                        testnum, i, reg_names[j], (void *)*(&context.Rax + j));
            }
        }
    }
}

static void test_virtual_unwind(void)
{
    static const BYTE function_0[] =
    {
        0xff, 0xf5,                                  /* 00: push %rbp */
        0x48, 0x81, 0xec, 0x10, 0x01, 0x00, 0x00,    /* 02: sub $0x110,%rsp */
        0x48, 0x8d, 0x6c, 0x24, 0x30,                /* 09: lea 0x30(%rsp),%rbp */
        0x48, 0x89, 0x9d, 0xf0, 0x00, 0x00, 0x00,    /* 0e: mov %rbx,0xf0(%rbp) */
        0x48, 0x89, 0xb5, 0xf8, 0x00, 0x00, 0x00,    /* 15: mov %rsi,0xf8(%rbp) */
        0x90,                                        /* 1c: nop */
        0x48, 0x8b, 0x9d, 0xf0, 0x00, 0x00, 0x00,    /* 1d: mov 0xf0(%rbp),%rbx */
        0x48, 0x8b, 0xb5, 0xf8, 0x00, 0x00, 0x00,    /* 24: mov 0xf8(%rbp),%rsi */
        0x48, 0x8d, 0xa5, 0xe0, 0x00, 0x00, 0x00,    /* 2b: lea 0xe0(%rbp),%rsp */
        0x5d,                                        /* 32: pop %rbp */
        0xc3                                         /* 33: ret */
    };

    static const BYTE unwind_info_0[] =
    {
        1 | (UNW_FLAG_EHANDLER << 3),  /* version + flags */
        0x1c,                          /* prolog size */
        8,                             /* opcode count */
        (0x03 << 4) | rbp,             /* frame reg rbp offset 0x30 */

        0x1c, UWOP(SAVE_NONVOL, rsi), 0x25, 0, /* 1c: mov %rsi,0x128(%rsp) */
        0x15, UWOP(SAVE_NONVOL, rbx), 0x24, 0, /* 15: mov %rbx,0x120(%rsp) */
        0x0e, UWOP(SET_FPREG, rbp),            /* 0e: lea 0x30(%rsp),rbp */
        0x09, UWOP(ALLOC_LARGE, 0), 0x22, 0,   /* 09: sub $0x110,%rsp */
        0x02, UWOP(PUSH_NONVOL, rbp),          /* 02: push %rbp */

        0x00, 0x02, 0x00, 0x00,  /* handler */
        0x05, 0x06, 0x07, 0x08,  /* data */
    };

    static const struct results results_0[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x00,  0x40,  FALSE, 0x000, 0x000, { {rsp,0x008}, {-1,-1} }},
        { 0x02,  0x40,  FALSE, 0x008, 0x000, { {rsp,0x010}, {rbp,0x000}, {-1,-1} }},
        { 0x09,  0x40,  FALSE, 0x118, 0x000, { {rsp,0x120}, {rbp,0x110}, {-1,-1} }},
        { 0x0e,  0x40,  FALSE, 0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {-1,-1} }},
        { 0x15,  0x40,  FALSE, 0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {-1,-1} }},
        { 0x1c,  0x40,  TRUE,  0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {rsi,0x138}, {-1,-1}}},
        { 0x1d,  0x40,  TRUE,  0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {rsi,0x138}, {-1,-1}}},
        { 0x24,  0x40,  TRUE,  0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {rbx,0x130}, {rsi,0x138}, {-1,-1}}},
        { 0x2b,  0x40,  FALSE, 0x128, 0x010, { {rsp,0x130}, {rbp,0x120}, {-1,-1}}},
        { 0x32,  0x40,  FALSE, 0x008, 0x010, { {rsp,0x010}, {rbp,0x000}, {-1,-1}}},
        { 0x33,  0x40,  FALSE, 0x000, 0x010, { {rsp,0x008}, {-1,-1}}},
    };


    static const BYTE function_1[] =
    {
        0x53,                     /* 00: push %rbx */
        0x55,                     /* 01: push %rbp */
        0x56,                     /* 02: push %rsi */
        0x57,                     /* 03: push %rdi */
        0x41, 0x54,               /* 04: push %r12 */
        0x48, 0x83, 0xec, 0x30,   /* 06: sub $0x30,%rsp */
        0x90, 0x90,               /* 0a: nop; nop */
        0x48, 0x83, 0xc4, 0x30,   /* 0c: add $0x30,%rsp */
        0x41, 0x5c,               /* 10: pop %r12 */
        0x5f,                     /* 12: pop %rdi */
        0x5e,                     /* 13: pop %rsi */
        0x5d,                     /* 14: pop %rbp */
        0x5b,                     /* 15: pop %rbx */
        0xc3                      /* 16: ret */
     };

    static const BYTE unwind_info_1[] =
    {
        1 | (UNW_FLAG_EHANDLER << 3),  /* version + flags */
        0x0a,                          /* prolog size */
        6,                             /* opcode count */
        0,                             /* frame reg */

        0x0a, UWOP(ALLOC_SMALL, 5),   /* 0a: sub $0x30,%rsp */
        0x06, UWOP(PUSH_NONVOL, r12), /* 06: push %r12 */
        0x04, UWOP(PUSH_NONVOL, rdi), /* 04: push %rdi */
        0x03, UWOP(PUSH_NONVOL, rsi), /* 03: push %rsi */
        0x02, UWOP(PUSH_NONVOL, rbp), /* 02: push %rbp */
        0x01, UWOP(PUSH_NONVOL, rbx), /* 01: push %rbx */

        0x00, 0x02, 0x00, 0x00,  /* handler */
        0x05, 0x06, 0x07, 0x08,  /* data */
    };

    static const struct results results_1[] =
    {
      /* offset  rbp   handler  rip   frame   registers */
        { 0x00,  0x50,  FALSE, 0x000, 0x000, { {rsp,0x008}, {-1,-1} }},
        { 0x01,  0x50,  FALSE, 0x008, 0x000, { {rsp,0x010}, {rbx,0x000}, {-1,-1} }},
        { 0x02,  0x50,  FALSE, 0x010, 0x000, { {rsp,0x018}, {rbx,0x008}, {rbp,0x000}, {-1,-1} }},
        { 0x03,  0x50,  FALSE, 0x018, 0x000, { {rsp,0x020}, {rbx,0x010}, {rbp,0x008}, {rsi,0x000}, {-1,-1} }},
        { 0x04,  0x50,  FALSE, 0x020, 0x000, { {rsp,0x028}, {rbx,0x018}, {rbp,0x010}, {rsi,0x008}, {rdi,0x000}, {-1,-1} }},
        { 0x06,  0x50,  FALSE, 0x028, 0x000, { {rsp,0x030}, {rbx,0x020}, {rbp,0x018}, {rsi,0x010}, {rdi,0x008}, {r12,0x000}, {-1,-1} }},
        { 0x0a,  0x50,  TRUE,  0x058, 0x000, { {rsp,0x060}, {rbx,0x050}, {rbp,0x048}, {rsi,0x040}, {rdi,0x038}, {r12,0x030}, {-1,-1} }},
        { 0x0c,  0x50,  FALSE, 0x058, 0x000, { {rsp,0x060}, {rbx,0x050}, {rbp,0x048}, {rsi,0x040}, {rdi,0x038}, {r12,0x030}, {-1,-1} }},
        { 0x10,  0x50,  FALSE, 0x028, 0x000, { {rsp,0x030}, {rbx,0x020}, {rbp,0x018}, {rsi,0x010}, {rdi,0x008}, {r12,0x000}, {-1,-1} }},
        { 0x12,  0x50,  FALSE, 0x020, 0x000, { {rsp,0x028}, {rbx,0x018}, {rbp,0x010}, {rsi,0x008}, {rdi,0x000}, {-1,-1} }},
        { 0x13,  0x50,  FALSE, 0x018, 0x000, { {rsp,0x020}, {rbx,0x010}, {rbp,0x008}, {rsi,0x000}, {-1,-1} }},
        { 0x14,  0x50,  FALSE, 0x010, 0x000, { {rsp,0x018}, {rbx,0x008}, {rbp,0x000}, {-1,-1} }},
        { 0x15,  0x50,  FALSE, 0x008, 0x000, { {rsp,0x010}, {rbx,0x000}, {-1,-1} }},
        { 0x16,  0x50,  FALSE, 0x000, 0x000, { {rsp,0x008}, {-1,-1} }},
    };

    static const struct unwind_test tests[] =
    {
        { function_0, sizeof(function_0), unwind_info_0,
          results_0, sizeof(results_0)/sizeof(results_0[0]) },
        { function_1, sizeof(function_1), unwind_info_1,
          results_1, sizeof(results_1)/sizeof(results_1[0]) }
    };
    unsigned int i;

    for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
        call_virtual_unwind( i, &tests[i] );
}

static RUNTIME_FUNCTION* CALLBACK dynamic_unwind_callback( DWORD64 pc, PVOID context )
{
    static const int code_offset = 1024;
    static RUNTIME_FUNCTION runtime_func;
    (*(DWORD *)context)++;

    runtime_func.BeginAddress = code_offset + 16;
    runtime_func.EndAddress   = code_offset + 32;
    runtime_func.UnwindData   = 0;
    return &runtime_func;
}

static void test_dynamic_unwind(void)
{
    static const int code_offset = 1024;
    char buf[sizeof(RUNTIME_FUNCTION) + 4];
    RUNTIME_FUNCTION *runtime_func, *func;
    ULONG_PTR table, base;
    DWORD count;

    /* Test RtlAddFunctionTable with aligned RUNTIME_FUNCTION pointer */
    runtime_func = (RUNTIME_FUNCTION *)buf;
    runtime_func->BeginAddress = code_offset;
    runtime_func->EndAddress   = code_offset + 16;
    runtime_func->UnwindData   = 0;
    ok( pRtlAddFunctionTable( runtime_func, 1, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for runtime_func = %p (aligned)\n", runtime_func );

    /* Lookup function outside of any function table */
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 16, &base, NULL );
    ok( func == NULL,
        "RtlLookupFunctionEntry returned unexpected function, expected: NULL, got: %p\n", func );
    ok( !base || broken(base == 0xdeadbeef),
        "RtlLookupFunctionEntry modified base address, expected: 0, got: %lx\n", base );

    /* Test with pointer inside of our function */
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 8, &base, NULL );
    ok( func == runtime_func,
        "RtlLookupFunctionEntry didn't return expected function, expected: %p, got: %p\n", runtime_func, func );
    ok( base == (ULONG_PTR)code_mem,
        "RtlLookupFunctionEntry returned invalid base, expected: %lx, got: %lx\n", (ULONG_PTR)code_mem, base );

    /* Test RtlDeleteFunctionTable */
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for runtime_func = %p (aligned)\n", runtime_func );
    ok( !pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable returned success for nonexistent table runtime_func = %p\n", runtime_func );

    /* Unaligned RUNTIME_FUNCTION pointer */
    runtime_func = (RUNTIME_FUNCTION *)((ULONG_PTR)buf | 0x3);
    runtime_func->BeginAddress = code_offset;
    runtime_func->EndAddress   = code_offset + 16;
    runtime_func->UnwindData   = 0;
    ok( pRtlAddFunctionTable( runtime_func, 1, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for runtime_func = %p (unaligned)\n", runtime_func );
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for runtime_func = %p (unaligned)\n", runtime_func );

    /* Attempt to insert the same entry twice */
    runtime_func = (RUNTIME_FUNCTION *)buf;
    runtime_func->BeginAddress = code_offset;
    runtime_func->EndAddress   = code_offset + 16;
    runtime_func->UnwindData   = 0;
    ok( pRtlAddFunctionTable( runtime_func, 1, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for runtime_func = %p (first attempt)\n", runtime_func );
    ok( pRtlAddFunctionTable( runtime_func, 1, (ULONG_PTR)code_mem ),
        "RtlAddFunctionTable failed for runtime_func = %p (second attempt)\n", runtime_func );
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for runtime_func = %p (first attempt)\n", runtime_func );
    ok( pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable failed for runtime_func = %p (second attempt)\n", runtime_func );
    ok( !pRtlDeleteFunctionTable( runtime_func ),
        "RtlDeleteFunctionTable returned success for nonexistent table runtime_func = %p\n", runtime_func );

    /* Test RtlInstallFunctionTableCallback with both low bits unset */
    table = (ULONG_PTR)code_mem;
    ok( !pRtlInstallFunctionTableCallback( table, (ULONG_PTR)code_mem, code_offset + 32, &dynamic_unwind_callback, (PVOID*)&count, NULL ),
        "RtlInstallFunctionTableCallback returned success for table = %lx\n", table );

    /* Test RtlInstallFunctionTableCallback with both low bits set */
    table = (ULONG_PTR)code_mem | 0x3;
    ok( pRtlInstallFunctionTableCallback( table, (ULONG_PTR)code_mem, code_offset + 32, &dynamic_unwind_callback, (PVOID*)&count, NULL ),
        "RtlInstallFunctionTableCallback failed for table = %lx\n", table );

    /* Lookup function outside of any function table */
    count = 0;
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 32, &base, NULL );
    ok( func == NULL,
        "RtlLookupFunctionEntry returned unexpected function, expected: NULL, got: %p\n", func );
    ok( !base || broken(base == 0xdeadbeef),
        "RtlLookupFunctionEntry modified base address, expected: 0, got: %lx\n", base );
    ok( !count,
        "RtlLookupFunctionEntry issued %d unexpected calls to dynamic_unwind_callback\n", count );

    /* Test with pointer inside of our function */
    count = 0;
    base = 0xdeadbeef;
    func = pRtlLookupFunctionEntry( (ULONG_PTR)code_mem + code_offset + 24, &base, NULL );
    ok( func != NULL && func->BeginAddress == code_offset + 16 && func->EndAddress == code_offset + 32,
        "RtlLookupFunctionEntry didn't return expected function, got: %p\n", func );
    ok( base == (ULONG_PTR)code_mem,
        "RtlLookupFunctionEntry returned invalid base, expected: %lx, got: %lx\n", (ULONG_PTR)code_mem, base );
    ok( count == 1,
        "RtlLookupFunctionEntry issued %d calls to dynamic_unwind_callback, expected: 1\n", count );

    /* Clean up again */
    ok( pRtlDeleteFunctionTable( (PRUNTIME_FUNCTION)table ),
        "RtlDeleteFunctionTable failed for table = %p\n", (PVOID)table );
    ok( !pRtlDeleteFunctionTable( (PRUNTIME_FUNCTION)table ),
        "RtlDeleteFunctionTable returned success for nonexistent table = %p\n", (PVOID)table );

}

#endif  /* __x86_64__ */

#if defined(__i386__) || defined(__x86_64__)
static DWORD outputdebugstring_exceptions;

static LONG CALLBACK outputdebugstring_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    trace("vect. handler %08x addr:%p\n", rec->ExceptionCode, rec->ExceptionAddress);

    ok(rec->ExceptionCode == DBG_PRINTEXCEPTION_C, "ExceptionCode is %08x instead of %08x\n",
        rec->ExceptionCode, DBG_PRINTEXCEPTION_C);
    ok(rec->NumberParameters == 2, "ExceptionParameters is %d instead of 2\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == 12, "ExceptionInformation[0] = %d instead of 12\n", (DWORD)rec->ExceptionInformation[0]);
    ok(!strcmp((char *)rec->ExceptionInformation[1], "Hello World"),
        "ExceptionInformation[1] = '%s' instead of 'Hello World'\n", (char *)rec->ExceptionInformation[1]);

    outputdebugstring_exceptions++;
    return EXCEPTION_CONTINUE_SEARCH;
}

static void test_outputdebugstring(DWORD numexc)
{
    PVOID vectored_handler;

    if (!pRtlAddVectoredExceptionHandler || !pRtlRemoveVectoredExceptionHandler)
    {
        skip("RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler not found\n");
        return;
    }

    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &outputdebugstring_vectored_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    outputdebugstring_exceptions = 0;
    OutputDebugStringA("Hello World");
    ok(outputdebugstring_exceptions == numexc, "OutputDebugStringA generated %d exceptions, expected %d\n",
       outputdebugstring_exceptions, numexc);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}

static DWORD ripevent_exceptions;

static LONG CALLBACK ripevent_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    trace("vect. handler %08x addr:%p\n", rec->ExceptionCode, rec->ExceptionAddress);

    ok(rec->ExceptionCode == DBG_RIPEXCEPTION, "ExceptionCode is %08x instead of %08x\n",
       rec->ExceptionCode, DBG_RIPEXCEPTION);
    ok(rec->NumberParameters == 2, "ExceptionParameters is %d instead of 2\n", rec->NumberParameters);
    ok(rec->ExceptionInformation[0] == 0x11223344, "ExceptionInformation[0] = %08x instead of %08x\n",
       (NTSTATUS)rec->ExceptionInformation[0], 0x11223344);
    ok(rec->ExceptionInformation[1] == 0x55667788, "ExceptionInformation[1] = %08x instead of %08x\n",
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
    ok(ripevent_exceptions == numexc, "RtlRaiseException generated %d exceptions, expected %d\n",
       ripevent_exceptions, numexc);

    pRtlRemoveVectoredExceptionHandler(vectored_handler);
}

static DWORD invalid_handle_exceptions;

static LONG CALLBACK invalid_handle_vectored_handler(EXCEPTION_POINTERS *ExceptionInfo)
{
    PEXCEPTION_RECORD rec = ExceptionInfo->ExceptionRecord;
    trace("vect. handler %08x addr:%p\n", rec->ExceptionCode, rec->ExceptionAddress);

    ok(rec->ExceptionCode == EXCEPTION_INVALID_HANDLE, "ExceptionCode is %08x instead of %08x\n",
       rec->ExceptionCode, EXCEPTION_INVALID_HANDLE);
    ok(rec->NumberParameters == 0, "ExceptionParameters is %d instead of 0\n", rec->NumberParameters);

    invalid_handle_exceptions++;
    return (rec->ExceptionCode == EXCEPTION_INVALID_HANDLE) ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

static void test_closehandle(DWORD numexc)
{
    PVOID vectored_handler;
    NTSTATUS status;
    DWORD res;

    if (!pRtlAddVectoredExceptionHandler || !pRtlRemoveVectoredExceptionHandler || !pRtlRaiseException)
    {
        skip("RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler or RtlRaiseException not found\n");
        return;
    }

    vectored_handler = pRtlAddVectoredExceptionHandler(TRUE, &invalid_handle_vectored_handler);
    ok(vectored_handler != 0, "RtlAddVectoredExceptionHandler failed\n");

    invalid_handle_exceptions = 0;
    res = CloseHandle((HANDLE)0xdeadbeef);
    ok(!res, "CloseHandle(0xdeadbeef) unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "wrong error code %d instead of %d\n",
       GetLastError(), ERROR_INVALID_HANDLE);
    ok(invalid_handle_exceptions == numexc, "CloseHandle generated %d exceptions, expected %d\n",
       invalid_handle_exceptions, numexc);

    invalid_handle_exceptions = 0;
    status = pNtClose((HANDLE)0xdeadbeef);
    ok(status == STATUS_INVALID_HANDLE, "NtClose(0xdeadbeef) returned status %08x\n", status);
    ok(invalid_handle_exceptions == numexc, "NtClose generated %d exceptions, expected %d\n",
       invalid_handle_exceptions, numexc);

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
#endif /* defined(__i386__) || defined(__x86_64__) */

START_TEST(exception)
{
    HMODULE hntdll = GetModuleHandleA("ntdll.dll");

    code_mem = VirtualAlloc(NULL, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if(!code_mem) {
        trace("VirtualAlloc failed\n");
        return;
    }

    pNtCurrentTeb        = (void *)GetProcAddress( hntdll, "NtCurrentTeb" );
    pNtGetContextThread  = (void *)GetProcAddress( hntdll, "NtGetContextThread" );
    pNtSetContextThread  = (void *)GetProcAddress( hntdll, "NtSetContextThread" );
    pNtReadVirtualMemory = (void *)GetProcAddress( hntdll, "NtReadVirtualMemory" );
    pNtClose             = (void *)GetProcAddress( hntdll, "NtClose" );
    pRtlUnwind           = (void *)GetProcAddress( hntdll, "RtlUnwind" );
    pRtlRaiseException   = (void *)GetProcAddress( hntdll, "RtlRaiseException" );
    pNtTerminateProcess  = (void *)GetProcAddress( hntdll, "NtTerminateProcess" );
    pRtlAddVectoredExceptionHandler    = (void *)GetProcAddress( hntdll,
                                                                 "RtlAddVectoredExceptionHandler" );
    pRtlRemoveVectoredExceptionHandler = (void *)GetProcAddress( hntdll,
                                                                 "RtlRemoveVectoredExceptionHandler" );
    pRtlAddVectoredContinueHandler     = (void *)GetProcAddress( hntdll,
                                                                 "RtlAddVectoredContinueHandler" );
    pRtlRemoveVectoredContinueHandler  = (void *)GetProcAddress( hntdll,
                                                                 "RtlRemoveVectoredContinueHandler" );
    pNtQueryInformationProcess         = (void*)GetProcAddress( hntdll,
                                                                 "NtQueryInformationProcess" );
    pNtSetInformationProcess           = (void*)GetProcAddress( hntdll,
                                                                 "NtSetInformationProcess" );
    pIsWow64Process = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process");

#ifdef __i386__
    if (!pNtCurrentTeb)
    {
        skip( "NtCurrentTeb not found\n" );
        return;
    }
    if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

    if (pRtlAddVectoredExceptionHandler && pRtlRemoveVectoredExceptionHandler)
        have_vectored_api = TRUE;
    else
        skip("RtlAddVectoredExceptionHandler or RtlRemoveVectoredExceptionHandler not found\n");

    my_argc = winetest_get_mainargs( &my_argv );
    if (my_argc >= 4)
    {
        void *addr;
        sscanf( my_argv[3], "%p", &addr );

        if (addr != &test_stage)
        {
            skip( "child process not mapped at same address (%p/%p)\n", &test_stage, addr);
            return;
        }

        /* child must be run under a debugger */
        if (!pNtCurrentTeb()->Peb->BeingDebugged)
        {
            ok(FALSE, "child process not being debugged?\n");
            return;
        }

        if (pRtlRaiseException)
        {
            test_stage = 1;
            run_rtlraiseexception_test(0x12345);
            run_rtlraiseexception_test(EXCEPTION_BREAKPOINT);
            run_rtlraiseexception_test(EXCEPTION_INVALID_HANDLE);
            test_stage = 2;
            run_rtlraiseexception_test(0x12345);
            run_rtlraiseexception_test(EXCEPTION_BREAKPOINT);
            run_rtlraiseexception_test(EXCEPTION_INVALID_HANDLE);
            test_stage = 3;
            test_outputdebugstring(0);
            test_stage = 4;
            test_outputdebugstring(2);
            test_stage = 5;
            test_ripevent(0);
            test_stage = 6;
            test_ripevent(1);
            test_stage = 7;
            test_closehandle(0);
            test_stage = 8;
            test_closehandle(1);
        }
        else
            skip( "RtlRaiseException not found\n" );

        /* rest of tests only run in parent */
        return;
    }

    test_unwind();
    test_exceptions();
    test_rtlraiseexception();
    test_outputdebugstring(1);
    test_ripevent(1);
    test_closehandle(0);
    test_vectored_continue_handler();
    test_debugger();
    test_simd_exceptions();
    test_fpu_exceptions();
    test_dpe_exceptions();
    test_prot_fault();

#elif defined(__x86_64__)
    pRtlAddFunctionTable               = (void *)GetProcAddress( hntdll,
                                                                 "RtlAddFunctionTable" );
    pRtlDeleteFunctionTable            = (void *)GetProcAddress( hntdll,
                                                                 "RtlDeleteFunctionTable" );
    pRtlInstallFunctionTableCallback   = (void *)GetProcAddress( hntdll,
                                                                 "RtlInstallFunctionTableCallback" );
    pRtlLookupFunctionEntry            = (void *)GetProcAddress( hntdll,
                                                                 "RtlLookupFunctionEntry" );

    test_outputdebugstring(1);
    test_ripevent(1);
    test_closehandle(0);
    test_vectored_continue_handler();
    test_virtual_unwind();

    if (pRtlAddFunctionTable && pRtlDeleteFunctionTable && pRtlInstallFunctionTableCallback && pRtlLookupFunctionEntry)
      test_dynamic_unwind();
    else
      skip( "Dynamic unwind functions not found\n" );

#endif

    VirtualFree(code_mem, 0, MEM_FREE);
}
