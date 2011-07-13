/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Interlocked function test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

/* missing prototypes >:| */
#ifndef _MSC_VER
typedef long long __int64;
#endif
struct _KSPIN_LOCK;
__declspec(dllimport)   long            __fastcall  InterlockedCompareExchange(volatile long *, long, long);
__declspec(dllimport)   __int64         __fastcall  ExInterlockedCompareExchange64(volatile __int64 *, __int64 *, __int64 *, void *);
__declspec(dllimport)   __int64         __fastcall  ExfInterlockedCompareExchange64(volatile __int64 *, __int64 *, __int64 *);
__declspec(dllimport)   long            __fastcall  InterlockedExchange(volatile long *, long);
__declspec(dllimport)   unsigned long   __stdcall   ExInterlockedExchangeUlong(unsigned long *, unsigned long, void *);
__declspec(dllimport)   long            __fastcall  InterlockedExchangeAdd(volatile long *, long);
__declspec(dllimport)   unsigned long   __stdcall   ExInterlockedAddUlong(unsigned long *, unsigned long, void *);
__declspec(dllimport)   unsigned long   __stdcall   Exi386InterlockedExchangeUlong(unsigned long *, unsigned long);
__declspec(dllimport)   long            __fastcall  InterlockedIncrement(long *);
__declspec(dllimport)   long            __fastcall  InterlockedDecrement(long *);
__declspec(dllimport)   int             __stdcall   ExInterlockedIncrementLong(long *, void *);
__declspec(dllimport)   int             __stdcall   ExInterlockedDecrementLong(long *, void *);
__declspec(dllimport)   int             __stdcall   Exi386InterlockedIncrementLong(long *);
__declspec(dllimport)   int             __stdcall   Exi386InterlockedDecrementLong(long *);

#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WS03SP1
#include <ntddk.h>
#include <pseh/pseh2.h>

#include <kmt_test.h>

/* TODO: There are quite some changes needed for other architectures!
         ExInterlockedAddLargeInteger, ExInterlockedAddUlong are the only two
         functions actually exported by my win7/x64 kernel! */

/* TODO: stress-testing */

static KSPIN_LOCK SpinLock;

typedef struct
{
    int esi, edi, ebx, ebp, esp;
} PROCESSOR_STATE, *PPROCESSOR_STATE;

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
    ok_eq_hex((OldState)->esi, (NewState)->esi);                            \
    ok_eq_hex((OldState)->edi, (NewState)->edi);                            \
    ok_eq_hex((OldState)->ebx, (NewState)->ebx);                            \
    ok_eq_hex((OldState)->ebp, (NewState)->ebp);                            \
    ok_eq_hex((OldState)->esp, (NewState)->esp);                            \
} while (0)

#else
#define SaveState(State)
#define CheckState(OldState, NewState)
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
                                ExpectedValue, ExpectedRet, ...) do         \
{                                                                           \
    Type Ret##Type = 0;                                                     \
    Type Value##Type = Val;                                                 \
    Status = STATUS_SUCCESS;                                                \
    _SEH2_TRY {                                                             \
        SaveState(OldState);                                                \
        Ret##Type = Function(&Value##Type, Xchg, Cmp, ##__VA_ARGS__);       \
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
                                ExpectedValue, ...) do                      \
{                                                                           \
    Type Value##Type = Val;                                                 \
    Status = STATUS_SUCCESS;                                                \
    _SEH2_TRY {                                                             \
        SaveState(OldState);                                                \
        Function(&Value##Type, Op, ##__VA_ARGS__);                          \
        SaveState(NewState);                                                \
        CheckState(&OldState, &NewState);                                   \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {                             \
        Status = _SEH2_GetExceptionCode();                                  \
    } _SEH2_END;                                                            \
    ok_eq_hex(Status, STATUS_SUCCESS);                                      \
    ok_eq_print(Value##Type.QuadPart, ExpectedValue, Print);                \
} while (0)

/* TODO: missing in wdm.h! */
#define InterlockedCompareExchangeAcquire InterlockedCompareExchange
#define InterlockedCompareExchangeRelease InterlockedCompareExchange
#define InterlockedIncrementAcquire InterlockedIncrement
#define InterlockedIncrementRelease InterlockedIncrement
#define InterlockedDecrementAcquire InterlockedDecrement
#define InterlockedDecrementRelease InterlockedDecrement

static
VOID
TestInterlockedFunctional(VOID)
{
    NTSTATUS Status;
    PKSPIN_LOCK pSpinLock = &SpinLock;
    PROCESSOR_STATE OldState, NewState;

    /* on x86, most of these are supported intrinsicly and don't need a spinlock! */
#if defined _M_IX86 || defined _M_AMD64
    pSpinLock = NULL;
#endif

    /* CompareExchange */
    /* macro version */
    CheckInterlockedCmpXchg(InterlockedCompareExchange, LONG, "%ld", 5, 6, 8, 5L, 5L);
    CheckInterlockedCmpXchg(InterlockedCompareExchange, LONG, "%ld", 5, 5, 9, 9L, 5L);
    /* exported function */
#undef InterlockedCompareExchange
    CheckInterlockedCmpXchg(InterlockedCompareExchange, LONG, "%ld", 5, 6, 8, 5L, 5L);
    CheckInterlockedCmpXchg(InterlockedCompareExchange, LONG, "%ld", 5, 5, 9, 9L, 5L);
    /* these only exist as macros on x86 */
    CheckInterlockedCmpXchg(InterlockedCompareExchangeAcquire, LONG, "%ld", 16, 9, 12, 16L, 16L);
    CheckInterlockedCmpXchg(InterlockedCompareExchangeAcquire, LONG, "%ld", 16, 16, 4, 4L, 16L);
    CheckInterlockedCmpXchg(InterlockedCompareExchangeRelease, LONG, "%ld", 27, 123, 38, 27L, 27L);
    CheckInterlockedCmpXchg(InterlockedCompareExchangeRelease, LONG, "%ld", 27, 27, 39, 39L, 27L);
    /* only exists as a macro */
    CheckInterlockedCmpXchg(InterlockedCompareExchangePointer, PVOID, "%p", (PVOID)117, (PVOID)711, (PVOID)12, (PVOID)117, (PVOID)117);
    CheckInterlockedCmpXchg(InterlockedCompareExchangePointer, PVOID, "%p", (PVOID)117, (PVOID)117, (PVOID)228, (PVOID)228, (PVOID)117);
    /* macro version */
    CheckInterlockedCmpXchgI(ExInterlockedCompareExchange64, LONGLONG, "%I64d", 17, 4LL, 20LL, 17LL, 17LL, pSpinLock);
    CheckInterlockedCmpXchgI(ExInterlockedCompareExchange64, LONGLONG, "%I64d", 17, 17LL, 21LL, 21LL, 17LL, pSpinLock);
    /* exported function */
    CheckInterlockedCmpXchgI((ExInterlockedCompareExchange64), LONGLONG, "%I64d", 17, 4LL, 20LL, 17LL, 17LL, pSpinLock);
    CheckInterlockedCmpXchgI((ExInterlockedCompareExchange64), LONGLONG, "%I64d", 17, 17LL, 21LL, 21LL, 17LL, pSpinLock);
    /* fastcall version */
    CheckInterlockedCmpXchgI(ExfInterlockedCompareExchange64, LONGLONG, "%I64d", 17, 4LL, 20LL, 17LL, 17LL);
    CheckInterlockedCmpXchgI(ExfInterlockedCompareExchange64, LONGLONG, "%I64d", 17, 17LL, 21LL, 21LL, 17LL);

    /* Exchange */
    CheckInterlockedOp(InterlockedExchange, LONG, "%ld", 5, 8, 8L, 5L);
#undef InterlockedExchange
    CheckInterlockedOp(InterlockedExchange, LONG, "%ld", 5, 8, 8L, 5L);
    CheckInterlockedOp(InterlockedExchangePointer, PVOID, "%p", (PVOID)700, (PVOID)93, (PVOID)93, (PVOID)700);
    CheckInterlockedOp(ExInterlockedExchangeUlong, ULONG, "%lu", 212, 121, 121LU, 212LU, pSpinLock);
    CheckInterlockedOp((ExInterlockedExchangeUlong), ULONG, "%lu", 212, 121, 121LU, 212LU, pSpinLock);
#ifdef _M_IX86
    CheckInterlockedOp(Exi386InterlockedExchangeUlong, ULONG, "%lu", 212, 121, 121LU, 212LU);
    CheckInterlockedOp(Exfi386InterlockedExchangeUlong, ULONG, "%lu", 212, 121, 121LU, 212LU);
#endif

    /* ExchangeAdd */
    /* TODO: ExInterlockedExchangeAddLargeInteger? */
    CheckInterlockedOp(InterlockedExchangeAdd, LONG, "%ld", 312, 7, 319L, 312L);
#undef InterlockedExchangeAdd
    CheckInterlockedOp(InterlockedExchangeAdd, LONG, "%ld", 312, 7, 319L, 312L);

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
#undef InterlockedIncrement
    CheckInterlockedOpNoArg(InterlockedIncrement, LONG, "%ld", 2341L, 2342L, 2342L);
    CheckInterlockedOpNoArg(InterlockedIncrement, LONG, "%ld", (LONG)MAXLONG, (LONG)MINLONG, (LONG)MINLONG);
    CheckInterlockedOpNoArg(InterlockedIncrementAcquire, LONG, "%ld", 2341L, 2342L, 2342L);
    CheckInterlockedOpNoArg(InterlockedIncrementRelease, LONG, "%ld", 2341L, 2342L, 2342L);
    CheckInterlockedOpNoArg(ExInterlockedIncrementLong, LONG, "%ld", -2L, -1L, (LONG)ResultNegative, pSpinLock);
    CheckInterlockedOpNoArg(ExInterlockedIncrementLong, LONG, "%ld", -1L, 0L, (LONG)ResultZero, pSpinLock);
    CheckInterlockedOpNoArg(ExInterlockedIncrementLong, LONG, "%ld", 0L, 1L, (LONG)ResultPositive, pSpinLock);
    CheckInterlockedOpNoArg(ExInterlockedIncrementLong, LONG, "%ld", (LONG)MAXLONG, (LONG)MINLONG, (LONG)ResultNegative, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedIncrementLong), LONG, "%ld", -2L, -1L, (LONG)ResultNegative, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedIncrementLong), LONG, "%ld", -1L, 0L, (LONG)ResultZero, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedIncrementLong), LONG, "%ld", 0L, 1L, (LONG)ResultPositive, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedIncrementLong), LONG, "%ld", (LONG)MAXLONG, (LONG)MINLONG, (LONG)ResultNegative, pSpinLock);
#ifdef _M_IX86
    CheckInterlockedOpNoArg(Exi386InterlockedIncrementLong, LONG, "%ld", -2L, -1L, (LONG)ResultNegative);
    CheckInterlockedOpNoArg(Exi386InterlockedIncrementLong, LONG, "%ld", -1L, 0L, (LONG)ResultZero);
    CheckInterlockedOpNoArg(Exi386InterlockedIncrementLong, LONG, "%ld", 0L, 1L, (LONG)ResultPositive);
    CheckInterlockedOpNoArg(Exi386InterlockedIncrementLong, LONG, "%ld", (LONG)MAXLONG, (LONG)MINLONG, (LONG)ResultNegative);
#endif

    /* Decrement */
    CheckInterlockedOpNoArg(InterlockedDecrement, LONG, "%ld", 1745L, 1744L, 1744L);
    CheckInterlockedOpNoArg(InterlockedDecrement, LONG, "%ld", (LONG)MINLONG, (LONG)MAXLONG, (LONG)MAXLONG);
#undef InterlockedDecrement
    CheckInterlockedOpNoArg(InterlockedDecrement, LONG, "%ld", 1745L, 1744L, 1744L);
    CheckInterlockedOpNoArg(InterlockedDecrement, LONG, "%ld", (LONG)MINLONG, (LONG)MAXLONG, (LONG)MAXLONG);
    CheckInterlockedOpNoArg(InterlockedDecrementAcquire, LONG, "%ld", 1745L, 1744L, 1744L);
    CheckInterlockedOpNoArg(InterlockedDecrementRelease, LONG, "%ld", 1745L, 1744L, 1744L);
    CheckInterlockedOpNoArg(ExInterlockedDecrementLong, LONG, "%ld", (LONG)MINLONG, (LONG)MAXLONG, (LONG)ResultPositive, pSpinLock);
    CheckInterlockedOpNoArg(ExInterlockedDecrementLong, LONG, "%ld", 0L, -1L, (LONG)ResultNegative, pSpinLock);
    CheckInterlockedOpNoArg(ExInterlockedDecrementLong, LONG, "%ld", 1L, 0L, (LONG)ResultZero, pSpinLock);
    CheckInterlockedOpNoArg(ExInterlockedDecrementLong, LONG, "%ld", 2L, 1L, (LONG)ResultPositive, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedDecrementLong), LONG, "%ld", (LONG)MINLONG, (LONG)MAXLONG, (LONG)ResultPositive, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedDecrementLong), LONG, "%ld", 0L, -1L, (LONG)ResultNegative, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedDecrementLong), LONG, "%ld", 1L, 0L, (LONG)ResultZero, pSpinLock);
    CheckInterlockedOpNoArg((ExInterlockedDecrementLong), LONG, "%ld", 2L, 1L, (LONG)ResultPositive, pSpinLock);
#ifdef _M_IX86
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
