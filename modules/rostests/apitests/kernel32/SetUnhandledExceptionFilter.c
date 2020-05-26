/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for SetUnhandledExceptionFilter
 * PROGRAMMER:      Mike "tamlin" Nordell
 */

#include "precomp.h"

#include <xmmintrin.h>

/*
 * Keep these returning different values, to prevent compiler folding
 * them into a single function, thereby voiding the test
 */
LONG WINAPI Filter1(LPEXCEPTION_POINTERS p) { return 0; }
LONG WINAPI Filter2(LPEXCEPTION_POINTERS p) { return 1; }


/*
 * Verify that SetUnhandledExceptionFilter actually returns the
 * _previous_ handler.
 */
static
VOID
TestSetUnhandledExceptionFilter(VOID)
{
	LPTOP_LEVEL_EXCEPTION_FILTER p1, p2;
	p1 = SetUnhandledExceptionFilter(Filter1);
	p2 = SetUnhandledExceptionFilter(Filter2);
	ok(p1 != Filter1, "SetUnhandledExceptionFilter returned what was set, not prev\n");
	ok(p2 != Filter2, "SetUnhandledExceptionFilter returned what was set, not prev\n");
	ok(p2 == Filter1, "SetUnhandledExceptionFilter didn't return previous filter\n");
	ok(p1 != p2, "SetUnhandledExceptionFilter seems to return random stuff\n");
    
    p1 = SetUnhandledExceptionFilter(NULL);
    ok(p1 == Filter2, "SetUnhandledExceptionFilter didn't return previous filter\n");
}

static LONG WINAPI ExceptionFilterSSESupport(LPEXCEPTION_POINTERS exp)
{
    PEXCEPTION_RECORD rec = exp->ExceptionRecord;
    PCONTEXT ctx = exp->ContextRecord;
    
    trace("Exception raised while using SSE instructions.\n");

    ok(rec->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION, "Exception code is 0x%08x.\n", (unsigned int)rec->ExceptionCode);
    
    if(rec->ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION)
    {
        trace("Unexpected exception code, terminating!\n");
        return EXCEPTION_EXECUTE_HANDLER;
    }
    
    ok((ctx->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL, "Context does not contain control register.\n");
    
#ifdef _M_IX86
    ctx->Eip += 3;
#elif defined(_M_AMD64)
    ctx->Rip += 3;
#else
#error Architecture not handled
#endif

    return EXCEPTION_CONTINUE_EXECUTION;
}

static BOOL ExceptionCaught = FALSE;

static LONG WINAPI ExceptionFilterSSEException(LPEXCEPTION_POINTERS exp)
{
    PEXCEPTION_RECORD rec = exp->ExceptionRecord;
    PCONTEXT ctx = exp->ContextRecord;
#ifdef _M_AMD64
    ULONG ExpectedExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
#else
    ULONG ExpectedExceptionCode = STATUS_FLOAT_MULTIPLE_TRAPS;
#endif
    
    trace("Exception raised while dividing by 0.\n");
    
    ok(rec->ExceptionCode == ExpectedExceptionCode, "Exception code is 0x%08x.\n", (unsigned int)rec->ExceptionCode);
    
    if(rec->ExceptionCode != ExpectedExceptionCode)
    {
        trace("Unexpected exception code, terminating!\n");
        return EXCEPTION_EXECUTE_HANDLER;
    }
    
    ok((ctx->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL, "Context does not contain control register.\n");
    
    ExceptionCaught = TRUE;
    
#ifdef _M_IX86
    ctx->Eip += 3;
#elif defined(_M_AMD64)
    ctx->Rip += 3;
#else
#error Architecture not handled
#endif

    return EXCEPTION_CONTINUE_EXECUTION;
}

static
VOID TestSSEExceptions(VOID)
{
    LPTOP_LEVEL_EXCEPTION_FILTER p;
    BOOL supportsSSE = FALSE;
    unsigned int csr;
    
    /* Test SSE support for the CPU */
    p = SetUnhandledExceptionFilter(ExceptionFilterSSESupport);
    ok(p == NULL, "Previous filter should be NULL\n");
#ifdef _MSC_VER
#if defined(_M_AMD64)
    {
        __m128 xmm = { { 0 } };
        xmm = _mm_xor_ps(xmm, xmm);
        if (!ExceptionCaught) supportsSSE = TRUE;
    }
#else
    __asm
    {
        xorps xmm0, xmm0
        mov supportsSSE, 0x1
    }
#endif
#else
    __asm__(
        "xorps %%xmm0, %%xmm0\n"
        "movl $1, %0\n"
        : "=r"(supportsSSE)
    );
#endif /* _MSC_VER */
    if(!supportsSSE)
    {
        skip("CPU doesn't support SSE instructions.\n");
        SetUnhandledExceptionFilter(NULL);
        return;
    }
    /* Deliberately throw a divide by 0 exception */
    p = SetUnhandledExceptionFilter(ExceptionFilterSSEException);
    ok(p == ExceptionFilterSSESupport, "Unexpected old filter : 0x%p", p);
    
    /* Unmask divide by 0 exception */
    csr = _mm_getcsr();
    _mm_setcsr(csr & 0xFFFFFDFF);

    /* We can't use _mm_div_ps, as it masks the exception before performing anything*/
#if defined(_MSC_VER)
#if defined(_M_AMD64)
    {
        __m128 xmm1 = { { 1., 1. } }, xmm2 = { { 0 } };
        /* Wait, aren't exceptions masked? Yes, but actually no. */
        xmm1 = _mm_div_ps(xmm1, xmm2);
        if (!ExceptionCaught) supportsSSE = TRUE;
    }
#else
    __asm
    {
        xorps xmm0, xmm0
        push 0x3f800000
        push 0x3f800000
        push 0x3f800000
        push 0x3f800000

        movups xmm1, [esp]

        /* Divide by 0 */
        divps xmm1, xmm0

        /* Clean up */
        add esp, 16
    }
#endif
#else
    __asm__ (
        "xorps %%xmm0, %%xmm0\n"
        "pushl $0x3f800000\n"
        "pushl $0x3f800000\n"
        "pushl $0x3f800000\n"
        "pushl $0x3f800000\n"
        "movups (%%esp), %%xmm1\n"

        /* Divide by 0 */
        "divps %%xmm0, %%xmm1\n"

        /* Clean up */
        "addl $16, %%esp\n"
        :
    );
#endif /* _MSC_VER */

    /* Restore mxcsr */
    _mm_setcsr(csr);

    ok(ExceptionCaught, "The exception was not caught.\n");

    p = SetUnhandledExceptionFilter(NULL);
    ok(p == ExceptionFilterSSEException, "Unexpected old filter : 0x%p", p);
}

START_TEST(SetUnhandledExceptionFilter)
{
    TestSetUnhandledExceptionFilter();
    TestSSEExceptions();
}
