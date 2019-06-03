/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtContinue
 * PROGRAMMER:
 */

#include "precomp.h"

#include <setjmp.h>
#include <time.h>

#ifdef _M_IX86
#define NTC_SEGMENT_BITS (0xFFFF)
#define NTC_EFLAGS_BITS  (0x3C0CD5)
#endif

void continuePoint(void);

static jmp_buf jmpbuf;
static CONTEXT continueContext;
static unsigned int nRandBytes;

static int initrand(void)
{
    unsigned int nRandMax;
    unsigned int nRandMaxBits;
    time_t tLoc;

    nRandMax = RAND_MAX;
    for(nRandMaxBits = 0; nRandMax != 0; nRandMax >>= 1, ++ nRandMaxBits);
    nRandBytes = nRandMaxBits / CHAR_BIT;
    //assert(nRandBytes != 0);
    srand((unsigned)(time(&tLoc) & UINT_MAX));
    return 1;
}

static void randbytes(void * p, size_t n)
{
    unsigned char * b;
    size_t i;
    int r = rand();

    b = (unsigned char *)p;
    for(i = 0; i < n; ++ i)
    {
        if(i % nRandBytes == 0)
        r = rand();
        b[i] = (unsigned char)(r & UCHAR_MAX);
        r >>= CHAR_BIT;
    }
}

static ULONG randULONG(void)
{
    ULONG n;
    randbytes(&n, sizeof(n));
    return n;
}

void check(CONTEXT * pContext)
{
#ifdef _M_IX86
    ok(pContext->ContextFlags == CONTEXT_FULL,
       "ContextFlags=0x%lx\n", pContext->ContextFlags);

    /* Random data segments */
    ok((pContext->SegGs & NTC_SEGMENT_BITS) ==
       (continueContext.SegGs & NTC_SEGMENT_BITS),
       "SegGs=0x%lx / 0x%lx\n", pContext->SegGs, continueContext.SegGs);

    ok((pContext->SegFs & NTC_SEGMENT_BITS) ==
       (continueContext.SegFs & NTC_SEGMENT_BITS),
       "SegFs=0x%lx / 0x%lx\n", pContext->SegFs, continueContext.SegFs);

    ok((pContext->SegEs & NTC_SEGMENT_BITS) ==
       (continueContext.SegEs & NTC_SEGMENT_BITS),
       "SegEs=0x%lx / 0x%lx\n", pContext->SegEs, continueContext.SegEs);

    ok((pContext->SegDs & NTC_SEGMENT_BITS) ==
       (continueContext.SegDs & NTC_SEGMENT_BITS),
       "SegDs=0x%lx / 0x%lx\n", pContext->SegDs, continueContext.SegDs);

    /* Integer registers */
    ok(pContext->Edi == continueContext.Edi,
       "Edi: 0x%lx != 0x%lx\n", pContext->Edi, continueContext.Edi);
    ok(pContext->Esi == continueContext.Esi,
       "Esi: 0x%lx != 0x%lx\n", pContext->Esi, continueContext.Esi);
    ok(pContext->Ebx == continueContext.Ebx,
       "Ebx: 0x%lx != 0x%lx\n", pContext->Ebx, continueContext.Ebx);
    ok(pContext->Edx == continueContext.Edx,
       "Edx: 0x%lx != 0x%lx\n", pContext->Edx, continueContext.Edx);
    ok(pContext->Ecx == continueContext.Ecx,
       "Ecx: 0x%lx != 0x%lx\n", pContext->Ecx, continueContext.Ecx);
    ok(pContext->Eax == continueContext.Eax,
       "Eax: 0x%lx != 0x%lx\n", pContext->Eax, continueContext.Eax);

    /* Control registers and segments */
    ok(pContext->Ebp == continueContext.Ebp,
       "Ebp: 0x%lx != 0x%lx\n", pContext->Ebp, continueContext.Ebp);
    ok(pContext->Eip == continueContext.Eip,
       "Eip: 0x%lx != 0x%lx\n", pContext->Eip, continueContext.Eip);
    ok(pContext->Esp == continueContext.Esp,
       "Esp: 0x%lx != 0x%lx\n", pContext->Esp, continueContext.Esp);

    ok((pContext->SegCs & NTC_SEGMENT_BITS) ==
       (continueContext.SegCs & NTC_SEGMENT_BITS),
       "SegCs: 0x%lx != 0x%lx\n", pContext->SegCs, continueContext.SegCs);

    ok((pContext->EFlags & NTC_EFLAGS_BITS) ==
       (continueContext.EFlags & NTC_EFLAGS_BITS),
       "EFlags: 0x%lx != 0x%lx\n", pContext->EFlags, continueContext.EFlags);

    ok((pContext->SegSs & NTC_SEGMENT_BITS) ==
       (continueContext.SegSs & NTC_SEGMENT_BITS),
       "SegSs: 0x%lx != 0x%lx\n", pContext->SegSs, continueContext.SegSs);
#endif

    /* Return where we came from */
    longjmp(jmpbuf, 1);
}

START_TEST(NtContinue)
{
#ifdef __RUNTIME_CHECKS__
    skip("This test breaks MSVC runtime checks!\n");
    return;
#endif /* __RUNTIME_CHECKS__ */
    initrand();

    /* First time */
    if(setjmp(jmpbuf) == 0)
    {
        CONTEXT bogus;

        continueContext.ContextFlags = CONTEXT_FULL;
        GetThreadContext(GetCurrentThread(), &continueContext);

#ifdef _M_IX86
        continueContext.ContextFlags = CONTEXT_FULL;

        /* Fill the integer registers with random values */
        continueContext.Edi = randULONG();
        continueContext.Esi = randULONG();
        continueContext.Ebx = randULONG();
        continueContext.Edx = randULONG();
        continueContext.Ecx = randULONG();
        continueContext.Eax = randULONG();
        continueContext.Ebp = randULONG();

        /* Randomize all the allowed flags (determined experimentally with WinDbg) */
        continueContext.EFlags = randULONG() & 0x3C0CD5;

        /* Randomize the stack pointer as much as possible */
        continueContext.Esp = (ULONG)(((ULONG_PTR)&bogus) & 0xFFFFFFFF) +
                              sizeof(bogus) - (randULONG() & 0xF) * 4;

        /* continuePoint() is implemented in assembler */
        continueContext.Eip = (ULONG)((ULONG_PTR)continuePoint & 0xFFFFFFF);

        /* Can't do a lot about segments */
#endif

        NtContinue(&continueContext, FALSE);
        ok(0, "should never get here\n");
    }

    /* Second time */
    return;
}
