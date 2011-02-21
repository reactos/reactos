/******************************************************************************
 *                           INTERLOCKED Functions                            *
 ******************************************************************************/
//
// Intrinsics (note: taken from our winnt.h)
// FIXME: 64-bit
//
#if defined(__GNUC__)

static __inline__ BOOLEAN
InterlockedBitTestAndSet(
  IN LONG volatile *Base,
  IN LONG Bit)
{
#if defined(_M_IX86)
  LONG OldBit;
  __asm__ __volatile__("lock "
                       "btsl %2,%1\n\t"
                       "sbbl %0,%0\n\t"
                       :"=r" (OldBit),"+m" (*Base)
                       :"Ir" (Bit)
                       : "memory");
  return OldBit;
#else
  return (_InterlockedOr(Base, 1 << Bit) >> Bit) & 1;
#endif
}

static __inline__ BOOLEAN
InterlockedBitTestAndReset(
  IN LONG volatile *Base,
  IN LONG Bit)
{
#if defined(_M_IX86)
  LONG OldBit;
  __asm__ __volatile__("lock "
                       "btrl %2,%1\n\t"
                       "sbbl %0,%0\n\t"
                       :"=r" (OldBit),"+m" (*Base)
                       :"Ir" (Bit)
                       : "memory");
  return OldBit;
#else
  return (_InterlockedAnd(Base, ~(1 << Bit)) >> Bit) & 1;
#endif
}

#endif /* defined(__GNUC__) */

#define BitScanForward _BitScanForward
#define BitScanReverse _BitScanReverse
#define BitTest _bittest
#define BitTestAndComplement _bittestandcomplement
#define BitTestAndSet _bittestandset
#define BitTestAndReset _bittestandreset
#define InterlockedBitTestAndSet _interlockedbittestandset
#define InterlockedBitTestAndReset _interlockedbittestandreset

#ifdef _M_AMD64
#define BitTest64 _bittest64
#define BitTestAndComplement64 _bittestandcomplement64
#define BitTestAndSet64 _bittestandset64
#define BitTestAndReset64 _bittestandreset64
#define InterlockedBitTestAndSet64 _interlockedbittestandset64
#define InterlockedBitTestAndReset64 _interlockedbittestandreset64
#endif

#if !defined(__INTERLOCKED_DECLARED)
#define __INTERLOCKED_DECLARED

#if defined (_X86_)
#if defined(NO_INTERLOCKED_INTRINSICS)
NTKERNELAPI
LONG
FASTCALL
InterlockedIncrement(
  IN OUT LONG volatile *Addend);

NTKERNELAPI
LONG
FASTCALL
InterlockedDecrement(
  IN OUT LONG volatile *Addend);

NTKERNELAPI
LONG
FASTCALL
InterlockedCompareExchange(
  IN OUT LONG volatile *Destination,
  IN LONG Exchange,
  IN LONG Comparand);

NTKERNELAPI
LONG
FASTCALL
InterlockedExchange(
  IN OUT LONG volatile *Destination,
  IN LONG Value);

NTKERNELAPI
LONG
FASTCALL
InterlockedExchangeAdd(
  IN OUT LONG volatile *Addend,
  IN LONG  Value);

#else /* !defined(NO_INTERLOCKED_INTRINSICS) */

#define InterlockedExchange _InterlockedExchange
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedOr _InterlockedOr
#define InterlockedAnd _InterlockedAnd
#define InterlockedXor _InterlockedXor

#endif /* !defined(NO_INTERLOCKED_INTRINSICS) */

#endif /* defined (_X86_) */

#if !defined (_WIN64)
/*
 * PVOID
 * InterlockedExchangePointer(
 *   IN OUT PVOID volatile  *Target,
 *   IN PVOID  Value)
 */
#define InterlockedExchangePointer(Target, Value) \
  ((PVOID) InterlockedExchange((PLONG) Target, (LONG) Value))

/*
 * PVOID
 * InterlockedCompareExchangePointer(
 *   IN OUT PVOID  *Destination,
 *   IN PVOID  Exchange,
 *   IN PVOID  Comparand)
 */
#define InterlockedCompareExchangePointer(Destination, Exchange, Comparand) \
  ((PVOID) InterlockedCompareExchange((PLONG) Destination, (LONG) Exchange, (LONG) Comparand))

#define InterlockedExchangeAddSizeT(a, b) InterlockedExchangeAdd((LONG *)a, b)
#define InterlockedIncrementSizeT(a) InterlockedIncrement((LONG *)a)
#define InterlockedDecrementSizeT(a) InterlockedDecrement((LONG *)a)

#endif // !defined (_WIN64)

#if defined (_M_AMD64)

#define InterlockedExchangeAddSizeT(a, b) InterlockedExchangeAdd64((LONGLONG *)a, (LONGLONG)b)
#define InterlockedIncrementSizeT(a) InterlockedIncrement64((LONGLONG *)a)
#define InterlockedDecrementSizeT(a) InterlockedDecrement64((LONGLONG *)a)
#define InterlockedAnd _InterlockedAnd
#define InterlockedOr _InterlockedOr
#define InterlockedXor _InterlockedXor
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedAdd _InterlockedAdd
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedAnd64 _InterlockedAnd64
#define InterlockedOr64 _InterlockedOr64
#define InterlockedXor64 _InterlockedXor64
#define InterlockedIncrement64 _InterlockedIncrement64
#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedAdd64 _InterlockedAdd64
#define InterlockedExchange64 _InterlockedExchange64
#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#define InterlockedCompareExchange64 _InterlockedCompareExchange64
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer
#define InterlockedExchangePointer _InterlockedExchangePointer
#define InterlockedBitTestAndSet64 _interlockedbittestandset64
#define InterlockedBitTestAndReset64 _interlockedbittestandreset64

#endif // _M_AMD64

#if defined(_M_AMD64) && !defined(RC_INVOKED) && !defined(MIDL_PASS)
//#if !defined(_X86AMD64_) // FIXME: what's _X86AMD64_ used for?
FORCEINLINE
LONG64
InterlockedAdd64(
  IN OUT LONG64 volatile *Addend,
  IN LONG64 Value)
{
  return InterlockedExchangeAdd64(Addend, Value) + Value;
}
//#endif
#endif

#endif /* !__INTERLOCKED_DECLARED */


