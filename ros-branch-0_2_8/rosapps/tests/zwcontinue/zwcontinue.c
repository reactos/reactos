#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>

static unsigned int nRandBytes;

static int initrand(void)
{
 unsigned int nRandMax;
 unsigned int nRandMaxBits;
 time_t tLoc;

 nRandMax = RAND_MAX;

 for(nRandMaxBits = 0; nRandMax != 0; nRandMax >>= 1, ++ nRandMaxBits);

 nRandBytes = nRandMaxBits / CHAR_BIT;

 assert(nRandBytes != 0);

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

#ifdef _M_IX86
#define ZWC_SEGMENT_BITS (0xFFFF)
#define ZWC_EFLAGS_BITS  (0x3C0CD5)
#endif

static jmp_buf jmpbuf;
static CONTEXT continueContext;

extern void continuePoint(void);
extern void check(CONTEXT *);
extern LONG NTAPI ZwContinue(IN CONTEXT *, IN BOOLEAN);

void check(CONTEXT * actualContext)
{
#ifdef _M_IX86
 assert(actualContext->ContextFlags == CONTEXT_FULL);

 /* Random data segments */
 assert
 (
  (actualContext->SegGs & ZWC_SEGMENT_BITS) ==
  (continueContext.SegGs & ZWC_SEGMENT_BITS)
 );

 assert
 (
  (actualContext->SegFs & ZWC_SEGMENT_BITS) ==
  (continueContext.SegFs & ZWC_SEGMENT_BITS)
 );

 assert
 (
  (actualContext->SegEs & ZWC_SEGMENT_BITS) ==
  (continueContext.SegEs & ZWC_SEGMENT_BITS)
 );

 assert
 (
  (actualContext->SegDs & ZWC_SEGMENT_BITS) ==
  (continueContext.SegDs & ZWC_SEGMENT_BITS)
 );

 /* Integer registers */
 assert(actualContext->Edi == continueContext.Edi);
 assert(actualContext->Esi == continueContext.Esi);
 assert(actualContext->Ebx == continueContext.Ebx);
 printf("%s %lX : %lX\n", "Edx", actualContext->Edx, continueContext.Edx);
 //assert(actualContext->Edx == continueContext.Edx);
 assert(actualContext->Ecx == continueContext.Ecx);
 assert(actualContext->Eax == continueContext.Eax);

 /* Control registers and segments */
 assert(actualContext->Ebp == continueContext.Ebp);
 assert(actualContext->Eip == continueContext.Eip);

 assert
 (
  (actualContext->SegCs & ZWC_SEGMENT_BITS) ==
  (continueContext.SegCs & ZWC_SEGMENT_BITS)
 );

 assert
 (
  (actualContext->EFlags & ZWC_EFLAGS_BITS) ==
  (continueContext.EFlags & ZWC_EFLAGS_BITS)
 );

 assert(actualContext->Esp == continueContext.Esp);

 assert
 (
  (actualContext->SegSs & ZWC_SEGMENT_BITS) ==
  (continueContext.SegSs & ZWC_SEGMENT_BITS)
 );
#endif

 longjmp(jmpbuf, 1);
}

int main(void)
{
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
  continueContext.Esp =
   (ULONG)(((ULONG_PTR)&bogus) & 0xFFFFFFFF) +
   sizeof(bogus) -
   (randULONG() & 0xF) * 4;

  /* continuePoint() is implemented in assembler */
  continueContext.Eip = (ULONG)((ULONG_PTR)continuePoint & 0xFFFFFFF);

  /* Can't do a lot about segments */
#endif

  ZwContinue(&continueContext, FALSE);
 }
 /* Second time */
 else
  return 0;

 assert(0);
 return 1;
}
