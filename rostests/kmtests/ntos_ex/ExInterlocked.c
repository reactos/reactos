/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Interlocked function test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <stddef.h>

/* missing prototypes >:| */
__declspec(dllimport)   long            __fastcall  InterlockedCompareExchange(volatile long *, long, long);
__declspec(dllimport)   __int64         __fastcall  ExInterlockedCompareExchange64(volatile __int64 *, __int64 *, __int64 *, void *);
__declspec(dllimport)   __int64         __fastcall  ExfInterlockedCompareExchange64(volatile __int64 *, __int64 *, __int64 *);
__declspec(dllimport)   long            __fastcall  InterlockedExchange(volatile long *, long);
__declspec(dllimport)   unsigned long   __stdcall   ExInterlockedExchangeUlong(unsigned long *, unsigned long, void *);
__declspec(dllimport)   long            __fastcall  InterlockedExchangeAdd(volatile long *, long);
#ifdef _X86_
__declspec(dllimport)   unsigned long   __stdcall   ExInterlockedAddUlong(unsigned long *, unsigned long, unsigned long *);
#endif
__declspec(dllimport)   unsigned long   __stdcall   Exi386InterlockedExchangeUlong(unsigned long *, unsigned long);
__declspec(dllimport)   long            __fastcall  InterlockedIncrement(long *);
__declspec(dllimport)   long            __fastcall  InterlockedDecrement(long *);
__declspec(dllimport)   int             __stdcall   ExInterlockedIncrementLong(long *, void *);
__declspec(dllimport)   int             __stdcall   ExInterlockedDecrementLong(long *, void *);
__declspec(dllimport)   int             __stdcall   Exi386InterlockedIncrementLong(long *);
__declspec(dllimport)   int             __stdcall   Exi386InterlockedDecrementLong(long *);

#include <kmt_test.h>

/* TODO: There are quite some changes needed for other architectures!
         ExInterlockedAddLargeInteger, ExInterlockedAddUlong are the only two
         functions actually exported by my win7/x64 kernel! */

/* TODO: stress-testing */

static KSPIN_LOCK SpinLock;

#ifdef _M_IX86
typedef struct
{
    unsigned long esi, edi, ebx, ebp, esp;
} PROCESSOR_STATE;
#elif defined(_M_AMD64)
typedef struct
{
    unsigned long long rsi, rdi, rbx, rbp, rsp, r12, r13, r14, r15;
} PROCESSOR_STATE;
#else
// dummy
typedef int PROCESSOR_STATE;
#endif

/* TODO: these need to be rewritten in proper assembly to account for registers
 *       saved by the caller */
#if defined(_MSC_VER) && defined(_M_IX86)
#define SaveState(State) do                                                 \
{                                                                           \
    __asm lea ecx,      [State]                                             \
    __asm mov [ecx],    esi                                                 \
    __asm mov [ecx+4],  edi                                                 \
    __asm mov [ecx+8],  ebx                                                 \
    __asm mov [ecx+12], ebp                                                 \
    __asm mov [ecx+16], esp                                                 \
} while (0)

#define CheckState(OldState, NewState) do                                   \
{                                                                           \
    /* TODO: MSVC uses esi and saves it before, so this is okay */          \
    /*ok_eq_hex((OldState)->esi, (NewState)->esi);*/                        \
    ok_eq_hex((OldState)->edi, (NewState)->edi);                            \
    ok_eq_hex((OldState)->ebx, (NewState)->ebx);                            \
    ok_eq_hex((OldState)->ebp, (NewState)->ebp);                            \
    ok_eq_hex((OldState)->esp, (NewState)->esp);                            \
} while (0)

#elif defined(__GNUC__) && defined(_M_IX86)
#define SaveState(State)                                                    \
    asm volatile(                                                           \
        "movl\t%%esi, (%%ecx)\n\t"                                          \
        "movl\t%%edi, 4(%%ecx)\n\t"                                         \
        "movl\t%%ebx, 8(%%ecx)\n\t"                                         \
        "movl\t%%ebp, 12(%%ecx)\n\t"                                        \
        "movl\t%%esp, 16(%%ecx)"                                            \
        : : "c" (&State) : "memory"                                         \
    );

#define CheckState(OldState, NewState) do                                   \
{                                                                           \
    /* TODO: GCC 4.7 uses esi and saves it before, so this is okay */       \
    /*ok_eq_hex((OldState)->esi, (NewState)->esi);*/                        \
    ok_eq_hex((OldState)->edi, (NewState)->edi);                            \
    /* TODO: GCC 4.4 uses ebx and saves it before, so this is okay */       \
    /*ok_eq_hex((OldState)->ebx, (NewState)->ebx);*/                        \
    ok_eq_hex((OldState)->ebp, (NewState)->ebp);                            \
    ok_eq_hex((OldState)->esp, (NewState)->esp);                            \
} while (0)
#elif defined(__GNUC__) && defined(_M_AMD64)
#define SaveState(State)                                                    \
    asm volatile(                                                           \
        "mov\t%%rsi, (%%rcx)\n\t"                                           \
        "mov\t%%rdi, 8(%%rcx)\n\t"                                          \
        "mov\t%%rbx, 16(%%rcx)\n\t"                                         \
        "mov\t%%rbp, 24(%%rcx)\n\t"                                         \
        "mov\t%%rsp, 32(%%rcx)\n\t"                                         \
        "mov\t%%r12, 40(%%rcx)\n\t"                                         \
        "mov\t%%r13, 48(%%rcx)\n\t"                                         \
        "mov\t%%r14, 56(%%rcx)\n\t"                                         \
        "mov\t%%r15, 64(%%rcx)"                                             \
        : : "c" (&State) : "memory"                                         \
    );

#define CheckState(OldState, NewState) do                                   \
{                                                                           \
    ok_eq_hex((OldState)->rsi, (NewState)->rsi);                            \
    ok_eq_hex((OldState)->rdi, (NewState)->rdi);                            \
    ok_eq_hex((OldState)->rbx, (NewState)->rbx);                            \
    ok_eq_hex((OldState)->rbp, (NewState)->rbp);                            \
    ok_eq_hex((OldState)->rsp, (NewState)->rsp);                            \
    ok_eq_hex((OldState)->r12, (NewState)->r12);                            \
    ok_eq_hex((OldState)->r13, (NewState)->r13);                            \
    ok_eq_hex((OldState)->r14, (NewState)->r14);                            \
    ok_eq_hex((OldState)->r15, (NewState)->r15);                            \
} while (0)
#else
#define SaveState(State)
#define CheckState(OldState, NewState) do                                   \
{                                                                           \
    (void)OldState;                                                         \
    (void)NewState;                                                         \
} while (0)
#endif

static
LARGE_INTEGER
Large(
    ULONGLONG Value)
{
    LARGE_INTEGER Ret;
    Ret.QuadPart = Value;
    return Ret;
}

#define CheckInterlockedCmpXchg(Function, Type, Print, Val, Cmp, Xchg,      \
                                ExpectedValue, ExpectedRet) do              \
{                                                                           \
    Type Ret##Type = 0;                                                     \
    Type Value##Type = Val;                                                 \
    Status = STATUS_SUCCESS;                                                \
    _SEH2_TRY {                                                             \
        SaveState(OldState);                                                \
        Ret##Type = Function(&Value##Type, Xchg, Cmp);                      \
        SaveState(NewState);                                                \
        CheckState(&OldState, &NewState);                                   \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {                             \
        Status = _SEH2_GetExceptionCode();                                  \
    } _SEH2_END;                                                            \
    ok_eq_hex(Status, STATUS_SUCCESS);                                      \
    ok_eq_print(Ret##Type, ExpectedRet, Print);                             \
    ok_eq_print(Value##Type, ExpectedValue, Print);                         \
} while (0)

#define CheckInterlockedCmpXchgI(Function, Type, Print, Val, Cmp, Xchg,     \
                                ExpectedValue, ExpectedRet, ...) do         \
{                                                                           \
    Type Ret##Type = 0;                                                     \
    Type Value##Type = Val;                                                 \
    Type Compare##Type = Cmp;                                               \
    Type Exchange##Type = Xchg;                                             \
    Status = STATUS_SUCCESS;                                                \
    _SEH2_TRY {                                                             \
        SaveState(OldState);                                                \
        Ret##Type = Function(&Value##Type, &Exchange##Type,                 \
                                &Compare##Type, ##__VA_ARGS__);             \
        SaveState(NewState);                                                \
        CheckState(&OldState, &NewState);                                   \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {                             \
        Status = _SEH2_GetExceptionCode();                                  \
    } _SEH2_END;                                                            \
    ok_eq_hex(Status, STATUS_SUCCESS);                                      \
    ok_eq_print(Ret##Type, ExpectedRet, Print);                             \
    ok_eq_print(Value##Type, ExpectedValue, Print);                         \
    ok_eq_print(Exchange##Type, Xchg, Print);                               \
    ok_eq_print(Compare##Type, Cmp, Print);                                 \
} while(0)

#define CheckInterlockedOp(Function, Type, Print, Val, Op,                  \
                                ExpectedValue, ExpectedRet, ...) do         \
{                                                                           \
    Type Ret##Type = 0;                                                     \
    Type Value##Type = Val;                                                 \
    Status = STATUS_SUCCESS;                                                \
    _SEH2_TRY {                                                             \
        SaveState(OldState);                                                \
        Ret##Type = Function(&Value##Type, Op, ##__VA_ARGS__);              \
        SaveState(NewState);                                                \
        CheckState(&OldState, &NewState);                                   \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {                             \
        Status = _SEH2_GetExceptionCode();                                  \
    } _SEH2_END;                                                            \
    ok_eq_hex(Status, STATUS_SUCCESS);                                      \
    ok_eq_print(Ret##Type, ExpectedRet, Print);                             \
    ok_eq_print(Value##Type, ExpectedValue, Print);                         \
} while (0)

#define CheckInterlockedOpNoArg(Function, Type, Print, Val,                 \
                                ExpectedValue, ExpectedRet, ...) do         \
{                                                                           \
    Type Ret##Type = 0;                                                     \
    Type Value##Type = Val;                                                 \
    Status = STATUS_SUCCESS;                                                \
    _SEH2_TRY {                                                             \
        SaveState(OldState);                                                \
        Ret##Type = Function(&Value##Type, ##__VA_ARGS__);                  \
        SaveState(NewState);                                                \
        CheckState(&OldState, &NewState);                                   \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {                             \
        Status = _SEH2_GetExceptionCode();                                  \
    } _SEH2_END;                                                            \
    ok_eq_hex(Status, STATUS_SUCCESS);                                      \
    ok_eq_print(Ret##Type, ExpectedRet, Print);                             \
    ok_eq_print(Value##Type, ExpectedValue, Print);                         \
} while (0)

#define CheckInterlockedOpLarge(Function, Type, Print, Val, Op,             \
                                ExpectedValue, ExpectedRet, ...) do         \
{                                                                           \
    Type Ret##Type = Large(0);                                              \
    Type Value##Type = Val;                                                 \
    Status = STATUS_SUCCESS;                                                \
    _SEH2_TRY {                                                             \
        SaveState(OldState);                                                \
        Ret##Type = Function(&Value##Type, Op, ##__VA_ARGS__);              \
        SaveState(NewState);                                                \
        CheckState(&OldState, &NewState);                                   \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {                             \
        Status = _SEH2_GetExceptionCode();                                  \
    } _SEH2_END;                                                            \
    ok_eq_hex(Status, STATUS_SUCCESS);                                      \
    ok_eq_print(Ret##Type.QuadPart, ExpectedRet, Print);                    \
    ok_eq_print(Value##Type.QuadPart, ExpectedValue, Print);                \
} while (0)

#define CheckInterlockedOpLargeNoRet(Function, Type, Print, Val, Op,        \
                                ExpectedValue) do                           \
{                                                                           \
    Type Value##Type = Val;                                                 \
    Status = STATUS_SUCCESS;                                                \
    _SEH2_TRY {                                                             \
        SaveState(OldState);                                                \
        Function(&Value##Type, Op);                                         \
        SaveState(NewState);                                                \
        CheckState(&OldState, &NewState);                                   \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {                             \
        Status = _SEH2_GetExceptionCode();                                  \
    } _SEH2_END;                                                            \
    ok_eq_hex(Status, STATUS_SUCCESS);                                      \
    ok_eq_print(Value##Type.QuadPart, ExpectedValue, Print);                \
} while (0)

static
VOID
TestInterlockedFunctional(VOID)
{
    NTSTATUS Status;
    PKSPIN_LOCK pSpinLock = &SpinLock;
    PROCESSOR_STATE OldState, NewState;

    /* on x86, most of these are supported intrinsically and don't need a spinlock! */
#if defined _M_IX86 || defined _M_AMD64
    pSpinLock = NULL;
#endif

    /* CompareExchange */
    /* macro version */
    CheckInterlockedCmpXchg(InterlockedCompareExchange, LONG, "%ld", 5, 6, 8, 5L, 5L);
    CheckInterlockedCmpXchg(InterlockedCompareExchange, LONG, "%ld", 5, 5, 9, 9L, 5L);
    /* these only exist as macros on x86 */
    CheckInterlockedCmpXchg(InterlockedCompareExchangeAcquire, LONG, "%ld", 16, 9, 12, 16L, 16L);
    CheckInterlockedCmpXchg(InterlockedCompareExchangeAcquire, LONG, "%ld", 16, 16, 4, 4L, 16L);
    CheckInterlockedCmpXchg(InterlockedCompareExchangeRelease, LONG, "%ld", 27, 123, 38, 27L, 27L);
    CheckInterlockedCmpXchg(InterlockedCompareExchangeRelease, LONG, "%ld", 27, 27, 39, 39L, 27L);
    /* exported function */
#undef InterlockedCompareExchange
#ifdef _M_IX86
    CheckInterlockedCmpXchg(InterlockedCompareExchange, LONG, "%ld", 5, 6, 8, 5L, 5L);
    CheckInterlockedCmpXchg(InterlockedCompareExchange, LONG, "%ld", 5, 5, 9, 9L, 5L);
#endif
    /* only exists as a macro */
    CheckInterlockedCmpXchg(InterlockedCompareExchangePointer, PVOID, "%p", (PVOID)117, (PVOID)711, (PVOID)12, (PVOID)117, (PVOID)117);
    CheckInterlockedCmpXchg(InterlockedCompareExchangePointer, PVOID, "%p", (PVOID)117, (PVOID)117, (PVOID)228, (PVOID)228, (PVOID)117);
    /* macro version */
    CheckInterlockedCmpXchgI(ExInterlockedCompareExchange64, LONGLONG, "%I64d", 17, 4LL, 20LL, 17LL, 17LL, pSpinLock);
    CheckInterlockedCmpXchgI(ExInterlockedCompareExchange64, LONGLONG, "%I64d", 17, 17LL, 21LL, 21LL, 17LL, pSpinLock);
#ifdef _M_IX86
    /* exported function */
    CheckInterlockedCmpXchgI((ExInterlockedCompareExchange64), LONGLONG, "%I64d", 17, 4LL, 20LL, 17LL, 17LL, pSpinLock);
    CheckInterlockedCmpXchgI((ExInterlockedCompareExchange64), LONGLONG, "%I64d", 17, 17LL, 21LL, 21LL, 17LL, pSpinLock);
    /* fastcall version */
    CheckInterlockedCmpXchgI(ExfInterlockedCompareExchange64, LONGLONG, "%I64d", 17, 4LL, 20LL, 17LL, 17LL);
    CheckInterlockedCmpXchgI(ExfInterlockedCompareExchange64, LONGLONG, "%I64d", 17, 17LL, 21LL, 21LL, 17LL);
#endif

    /* Exchange */
    CheckInterlockedOp(InterlockedExchange, LONG, "%ld", 5, 8, 8L, 5L);
    CheckInterlockedOpNoArg(InterlockedExchangePointer, PVOID, "%p", (PVOID)700, (PVOID)93, (PVOID)700, (PVOID)93);
#undef InterlockedExchange
#ifdef _M_IX86
    CheckInterlockedOp(InterlockedExchange, LONG, "%ld", 5, 8, 8L, 5L);
    CheckInterlockedOp(ExInterlockedExchangeUlong, ULONG, "%lu", 212, 121, 121LU, 212LU, pSpinLock);
    CheckInterlockedOp((ExInterlockedExchangeUlong), ULONG, "%lu", 212, 121, 121LU, 212LU, pSpinLock);
    CheckInterlockedOp(Exi386InterlockedExchangeUlong, ULONG, "%lu", 212, 121, 121LU, 212LU);
    CheckInterlockedOp(Exfi386InterlockedExchangeUlong, ULONG, "%lu", 212, 121, 121LU, 212LU);
#endif

    /* ExchangeAdd */
    /* TODO: ExInterlockedExchangeAddLargeInteger? */
    CheckInterlockedOp(InterlockedExchangeAdd, LONG, "%ld", 312, 7, 319L, 312L);
#undef InterlockedExchangeAdd
#ifdef _M_IX86
    CheckInterlockedOp(InterlockedExchangeAdd, LONG, "%ld", 312, 7, 319L, 312L);
#endif

    /* Add */
    /* these DO need a valid spinlock even on x86 */
    CheckInterlockedOpLarge(ExInterlockedAddLargeInteger, LARGE_INTEGER, "%I64d", Large(23), Large(7), 30LL, 23LL, &SpinLock);
    CheckInterlockedOpLargeNoRet(ExInterlockedAddLargeStatistic, LARGE_INTEGER, "%I64d", Large(15), 17LL, 32LL);
    CheckInterlockedOp(ExInterlockedAddUlong, ULONG, "%lu", 239, 44, 283LU, 239LU, &SpinLock);
#undef ExInterlockedAddUlong
    CheckInterlockedOp(ExInterlockedAddUlong, ULONG, "%lu", 239, 44, 283LU, 239LU, &SpinLock);

    /* Increment */
    CheckInterlockedOpNoArg(InterlockedIncrement, LONG, "%ld", 2341L, 2342L, 2342L);
    CheckInterlockedOpNoArg(InterlockedIncrement, LONG, "%ld", (LONG)MAXLONG, (LONG)MINLONG, (LONG)MINLONG);
    CheckInterlockedOpNoArg(InterlockedIncrementAcquire, LONG, "%ld", 2341L, 2342L, 2342L);
    CheckInterlockedOpNoArg(InterlockedIncrementRelease, LONG, "%ld", 2341L, 2342L, 2342L);
#undef InterlockedIncrement
#ifdef _M_IX86
    CheckInterlockedOpNoArg(InterlockedIncrement, LONG, "%ld", 2341L, 2342L, 2342L);
    CheckInterlockedOpNoArg(InterlockedIncrement, LONG, "%ld", (LONG)MAXLONG, (LONG)MINLONG, (LONG)MINLONG);
    CheckInterlockedOpNoArg(ExInterlockedIncrementLong, LONG, "%ld", -2L, -1L, (LONG)ResultNegative, pSpinLock);
    CheckInterlockedOpNoArg(ExInterlockedIncrementLong, LONG, "%ld", -1L, 0L, (LONG)ResultZero, pSpinLock);
    CheckInterlockedOpNoArg(ExInterlockedIncrementLong, LONG, "%ld", 0L, 1L, (LONG)ResultPositive, pSpinLock);
    CheckInterlockedOpNoArg(ExInterlockedIncrementLong, LONG, "%ld", (LONG)MAXLONG, (LONG)MINLONG, (LONG)ResultNegative, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedIncrementLong), LONG, "%ld", -2L, -1L, (LONG)ResultNegative, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedIncrementLong), LONG, "%ld", -1L, 0L, (LONG)ResultZero, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedIncrementLong), LONG, "%ld", 0L, 1L, (LONG)ResultPositive, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedIncrementLong), LONG, "%ld", (LONG)MAXLONG, (LONG)MINLONG, (LONG)ResultNegative, pSpinLock);
    CheckInterlockedOpNoArg(Exi386InterlockedIncrementLong, LONG, "%ld", -2L, -1L, (LONG)ResultNegative);
    CheckInterlockedOpNoArg(Exi386InterlockedIncrementLong, LONG, "%ld", -1L, 0L, (LONG)ResultZero);
    CheckInterlockedOpNoArg(Exi386InterlockedIncrementLong, LONG, "%ld", 0L, 1L, (LONG)ResultPositive);
    CheckInterlockedOpNoArg(Exi386InterlockedIncrementLong, LONG, "%ld", (LONG)MAXLONG, (LONG)MINLONG, (LONG)ResultNegative);
#endif

    /* Decrement */
    CheckInterlockedOpNoArg(InterlockedDecrement, LONG, "%ld", 1745L, 1744L, 1744L);
    CheckInterlockedOpNoArg(InterlockedDecrement, LONG, "%ld", (LONG)MINLONG, (LONG)MAXLONG, (LONG)MAXLONG);
    CheckInterlockedOpNoArg(InterlockedDecrementAcquire, LONG, "%ld", 1745L, 1744L, 1744L);
    CheckInterlockedOpNoArg(InterlockedDecrementRelease, LONG, "%ld", 1745L, 1744L, 1744L);
#undef InterlockedDecrement
#ifdef _M_IX86
    CheckInterlockedOpNoArg(InterlockedDecrement, LONG, "%ld", 1745L, 1744L, 1744L);
    CheckInterlockedOpNoArg(InterlockedDecrement, LONG, "%ld", (LONG)MINLONG, (LONG)MAXLONG, (LONG)MAXLONG);
    CheckInterlockedOpNoArg(ExInterlockedDecrementLong, LONG, "%ld", (LONG)MINLONG, (LONG)MAXLONG, (LONG)ResultPositive, pSpinLock);
    CheckInterlockedOpNoArg(ExInterlockedDecrementLong, LONG, "%ld", 0L, -1L, (LONG)ResultNegative, pSpinLock);
    CheckInterlockedOpNoArg(ExInterlockedDecrementLong, LONG, "%ld", 1L, 0L, (LONG)ResultZero, pSpinLock);
    CheckInterlockedOpNoArg(ExInterlockedDecrementLong, LONG, "%ld", 2L, 1L, (LONG)ResultPositive, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedDecrementLong), LONG, "%ld", (LONG)MINLONG, (LONG)MAXLONG, (LONG)ResultPositive, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedDecrementLong), LONG, "%ld", 0L, -1L, (LONG)ResultNegative, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedDecrementLong), LONG, "%ld", 1L, 0L, (LONG)ResultZero, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedDecrementLong), LONG, "%ld", 2L, 1L, (LONG)ResultPositive, pSpinLock);
    CheckInterlockedOpNoArg(Exi386InterlockedDecrementLong, LONG, "%ld", (LONG)MINLONG, (LONG)MAXLONG, (LONG)ResultPositive);
    CheckInterlockedOpNoArg(Exi386InterlockedDecrementLong, LONG, "%ld", 0L, -1L, (LONG)ResultNegative);
    CheckInterlockedOpNoArg(Exi386InterlockedDecrementLong, LONG, "%ld", 1L, 0L, (LONG)ResultZero);
    CheckInterlockedOpNoArg(Exi386InterlockedDecrementLong, LONG, "%ld", 2L, 1L, (LONG)ResultPositive);
#endif

    /* And, Or, Xor */
    CheckInterlockedOp(InterlockedAnd, LONG, "0x%lx", 0x1234L, 0x1111L, 0x1010L, 0x1234L);
    CheckInterlockedOp(InterlockedOr, LONG, "0x%lx", 0x1234L, 0x1111L, 0x1335L, 0x1234L);
    CheckInterlockedOp(InterlockedXor, LONG, "0x%lx", 0x1234L, 0x1111L, 0x0325L, 0x1234L);
#ifdef _WIN64
    CheckInterlockedOp(InterlockedXor64, LONGLONG, "0x%I64x", 0x200001234LL, 0x100001111LL, 0x300000325LL, 0x200001234LL);
#endif
}

START_TEST(ExInterlocked)
{
    KIRQL Irql;
    KeInitializeSpinLock(&SpinLock);

    /* functional testing */
    TestInterlockedFunctional();
    KeRaiseIrql(HIGH_LEVEL, &Irql);
    TestInterlockedFunctional();
    KeLowerIrql(Irql);
}
