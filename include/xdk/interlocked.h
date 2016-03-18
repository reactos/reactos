/******************************************************************************
 *                           INTERLOCKED Functions                            *
 ******************************************************************************/

#define BitScanForward _BitScanForward
#define BitScanReverse _BitScanReverse
#define BitTest _bittest
#define BitTestAndComplement _bittestandcomplement
#define BitTestAndSet _bittestandset
#define BitTestAndReset _bittestandreset
#ifdef _WIN64
#define BitScanForward64 _BitScanForward64
#define BitScanReverse64 _BitScanReverse64
#define BitTest64 _bittest64
#define BitTestAndComplement64 _bittestandcomplement64
#define BitTestAndSet64 _bittestandset64
#define BitTestAndReset64 _bittestandreset64
#endif /* _WIN64 */

#if defined(_M_ARM) || defined(_M_IA64)
#define __ACQ_(x) x##_acq
#define __REL_(x) x##_rel
#define __NF_(x) x##_nf
#else
#define __ACQ_(x) x
#define __REL_(x) x
#define __NF_(x) x
#endif

#define InterlockedBitTestAndSet _interlockedbittestandset
#define InterlockedBitTestAndSetAcquire __ACQ_(_interlockedbittestandset)
#define InterlockedBitTestAndSetRelease __REL_(_interlockedbittestandset)
#define InterlockedBitTestAndSetNoFence __NF_(_interlockedbittestandset)

#define InterlockedBitTestAndReset _interlockedbittestandreset
#define InterlockedBitTestAndResetAcquire __ACQ_(_interlockedbittestandreset)
#define InterlockedBitTestAndResetRelease __REL_(_interlockedbittestandreset)
#define InterlockedBitTestAndResetNoFence __NF_(_interlockedbittestandreset)

#ifdef _WIN64
#define InterlockedBitTestAndSet64 _interlockedbittestandset64
#define InterlockedBitTestAndSet64Acquire __ACQ_(_interlockedbittestandset64)
#define InterlockedBitTestAndSet64Release __REL_(_interlockedbittestandset64)
#define InterlockedBitTestAndSet64NoFence __NF_(_interlockedbittestandset64)

#define InterlockedBitTestAndReset64 _interlockedbittestandreset64
#define InterlockedBitTestAndReset64Acquire __ACQ_(_interlockedbittestandreset64)
#define InterlockedBitTestAndReset64Release __REL_(_interlockedbittestandreset64)
#define InterlockedBitTestAndReset64NoFence __NF_(_interlockedbittestandreset64)
#endif /* _WIN64 */

#define InterlockedAdd _InterlockedAdd
#define InterlockedAddAcquire __ACQ_(_InterlockedAdd)
#define InterlockedAddRelease __REL_(_InterlockedAdd)
#define InterlockedAddNoFence __NF_(_InterlockedAdd)

#define InterlockedAdd64 _InterlockedAdd64
#define InterlockedAddAcquire64 __ACQ_(_InterlockedAdd64)
#define InterlockedAddRelease64 __REL_(_InterlockedAdd64)
#define InterlockedAddNoFence64 __NF_(_InterlockedAdd64)

#define InterlockedAnd _InterlockedAnd
#define InterlockedAndAcquire __ACQ_(_InterlockedAnd)
#define InterlockedAndRelease __REL_(_InterlockedAnd)
#define InterlockedAndNoFence __NF_(_InterlockedAnd)

#define InterlockedAnd8 _InterlockedAnd8
#ifdef _M_ARM
#define InterlockedAndAcquire8 _InterlockedAnd8_acq
#define InterlockedAndRelease8 _InterlockedAnd8_rel
#define InterlockedAndNoFence8 _InterlockedAnd8_nf
#elif defined(_M_IA64)
#define InterlockedAnd8Acquire  _InterlockedAnd8_acq
#define InterlockedAnd8Release  _InterlockedAnd8_rel
#endif // _M_ARM

#define InterlockedAnd16 _InterlockedAnd16
#ifdef _M_ARM
#define InterlockedAndAcquire16 _InterlockedAnd16_acq
#define InterlockedAndRelease16 _InterlockedAnd16_rel
#define InterlockedAndNoFence16 _InterlockedAnd16_nf
#elif defined(_M_IA64)
#define InterlockedAnd16Acquire _InterlockedAnd16_acq
#define InterlockedAnd16Release _InterlockedAnd16_rel
#endif // _M_ARM

#define InterlockedAnd64 _InterlockedAnd64
#ifdef _M_ARM
#define InterlockedAndAcquire64 __ACQ_(_InterlockedAnd64)
#define InterlockedAndRelease64 __REL_(_InterlockedAnd64)
#define InterlockedAndNoFence64 __NF_(_InterlockedAnd64)
#else // _M_ARM
#define InterlockedAnd64Acquire __ACQ_(_InterlockedAnd64)
#define InterlockedAnd64Release __REL_(_InterlockedAnd64)
#define InterlockedAnd64NoFence __NF_(_InterlockedAnd64)
#endif // _M_ARM

#ifdef _WIN64
#define InterlockedAndAffinity InterlockedAnd64
#else
#define InterlockedAndAffinity InterlockedAnd
#endif // _WIN64

#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedCompareExchangeAcquire __ACQ_(_InterlockedCompareExchange)
#define InterlockedCompareExchangeRelease __REL_(_InterlockedCompareExchange)
#define InterlockedCompareExchangeNoFence __NF_(_InterlockedCompareExchange)

#define InterlockedCompareExchange16 _InterlockedCompareExchange16
#define InterlockedCompareExchangeAcquire16 __ACQ_(_InterlockedCompareExchange16)
#define InterlockedCompareExchangeRelease16 __REL_(_InterlockedCompareExchange16)
#define InterlockedCompareExchangeNoFence16 __NF_(_InterlockedCompareExchange16)

#define InterlockedCompareExchange64 _InterlockedCompareExchange64
#define InterlockedCompareExchangeAcquire64 __ACQ_(_InterlockedCompareExchange64)
#define InterlockedCompareExchangeRelease64 __REL_(_InterlockedCompareExchange64)
#define InterlockedCompareExchangeNoFence64 __NF_(_InterlockedCompareExchange64)

#ifdef _WIN64
#define InterlockedCompareExchange128 _InterlockedCompareExchange128
#endif // _WIN64

#ifdef _M_IA64
#define InterlockedCompare64Exchange128         _InterlockedCompare64Exchange128
#define InterlockedCompare64ExchangeAcquire128  _InterlockedCompare64Exchange128_acq
#define InterlockedCompare64ExchangeRelease128  _InterlockedCompare64Exchange128_rel
#endif // _M_IA64

#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer
#define InterlockedCompareExchangePointerAcquire __ACQ_(_InterlockedCompareExchangePointer)
#define InterlockedCompareExchangePointerRelease __REL_(_InterlockedCompareExchangePointer)
#define InterlockedCompareExchangePointerNoFence __NF_(_InterlockedCompareExchangePointer)

#define InterlockedDecrement _InterlockedDecrement
#define InterlockedDecrementAcquire __ACQ_(_InterlockedDecrement)
#define InterlockedDecrementRelease __REL_(_InterlockedDecrement)
#define InterlockedDecrementNoFence __NF_(_InterlockedDecrement)

#define InterlockedDecrement16 _InterlockedDecrement16
#define InterlockedDecrementAcquire16 __ACQ_(_InterlockedDecrement16)
#define InterlockedDecrementRelease16 __REL_(_InterlockedDecrement16)
#define InterlockedDecrementNoFence16 __NF_(_InterlockedDecrement16)

#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedDecrementAcquire64 __ACQ_(_InterlockedDecrement64)
#define InterlockedDecrementRelease64 __REL_(_InterlockedDecrement64)
#define InterlockedDecrementNoFence64 __NF_(_InterlockedDecrement64)

#ifdef _WIN64
#define InterlockedDecrementSizeT(a) InterlockedDecrement64((LONG64 *)a)
#define InterlockedDecrementSizeTNoFence(a) InterlockedDecrementNoFence64((LONG64 *)a)
#else
#define InterlockedDecrementSizeT(a) InterlockedDecrement((LONG *)a)
#define InterlockedDecrementSizeTNoFence(a) InterlockedDecrementNoFence((LONG *)a)
#endif // _WIN64

#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAcquire __ACQ_(_InterlockedExchange)
/* No release here */
#define InterlockedExchangeNoFence __NF_(_InterlockedExchange)

#if (_MSC_VER >= 1600)
#define InterlockedExchange8 _InterlockedExchange8
#endif // (_MSC_VER >= 1600)

#define InterlockedExchange16 _InterlockedExchange16
/* No release here */
#define InterlockedExchangeAcquire16 __ACQ_(_InterlockedExchange16)
#define InterlockedExchangeNoFence16 __NF_(_InterlockedExchange16)

#define InterlockedExchange64 _InterlockedExchange64
#define InterlockedExchangeAcquire64 __ACQ_(_InterlockedExchange64)
/* No release here */
#define InterlockedExchangeNoFence64 __NF_(_InterlockedExchange64)

#define InterlockedExchangePointer _InterlockedExchangePointer
#define InterlockedExchangePointerAcquire __ACQ_(_InterlockedExchangePointer)
/* No release here */
#define InterlockedExchangePointerNoFence __NF_(_InterlockedExchangePointer)

#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedExchangeAddAcquire __ACQ_(_InterlockedExchangeAdd)
#define InterlockedExchangeAddRelease __REL_(_InterlockedExchangeAdd)
#define InterlockedExchangeAddNoFence __NF_(_InterlockedExchangeAdd)

#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#define InterlockedExchangeAddAcquire64 __ACQ_(_InterlockedExchangeAdd64)
#define InterlockedExchangeAddRelease64 __REL_(_InterlockedExchangeAdd64)
#define InterlockedExchangeAddNoFence64 __NF_(_InterlockedExchangeAdd64)

#ifdef _WIN64
#define InterlockedExchangeAddSizeT(a, b) InterlockedExchangeAdd64((LONG64 *)a, b)
#define InterlockedExchangeAddSizeTAcquire(a, b) InterlockedExchangeAddAcquire64((LONG64 *)a, b)
#define InterlockedExchangeAddSizeTNoFence(a, b) InterlockedExchangeAddNoFence64((LONG64 *)a, b)
#else
#define InterlockedExchangeAddSizeT(a, b) InterlockedExchangeAdd((LONG *)a, b)
#define InterlockedExchangeAddSizeTAcquire(a, b) InterlockedExchangeAddAcquire((LONG *)a, b)
#define InterlockedExchangeAddSizeTNoFence(a, b) InterlockedExchangeAddNoFence((LONG *)a, b)
#endif // _WIN64

#define InterlockedIncrement _InterlockedIncrement
#define InterlockedIncrementAcquire __ACQ_(_InterlockedIncrement)
#define InterlockedIncrementRelease __REL_(_InterlockedIncrement)
#define InterlockedIncrementNoFence __NF_(_InterlockedIncrement)

#define InterlockedIncrement16 _InterlockedIncrement16
#define InterlockedIncrementAcquire16 __ACQ_(_InterlockedIncrement16)
#define InterlockedIncrementRelease16 __REL_(_InterlockedIncrement16)
#define InterlockedIncrementNoFence16 __NF_(_InterlockedIncrement16)

#define InterlockedIncrement64 _InterlockedIncrement64
#define InterlockedIncrementAcquire64 __ACQ_(_InterlockedIncrement64)
#define InterlockedIncrementRelease64 __REL_(_InterlockedIncrement64)
#define InterlockedIncrementNoFence64 __NF_(_InterlockedIncrement64)

#ifdef _WIN64
#define InterlockedIncrementSizeT(a) InterlockedIncrement64((LONG64 *)a)
#define InterlockedIncrementSizeTNoFence(a) InterlockedIncrementNoFence64((LONG64 *)a)
#else
#define InterlockedIncrementSizeT(a) InterlockedIncrement((LONG *)a)
#define InterlockedIncrementSizeTNoFence(a) InterlockedIncrementNoFence((LONG *)a)
#endif // _WIN64

#define InterlockedOr _InterlockedOr
#define InterlockedOrAcquire __ACQ_(_InterlockedOr)
#define InterlockedOrRelease __REL_(_InterlockedOr)
#define InterlockedOrNoFence __NF_(_InterlockedOr)

#define InterlockedOr8 _InterlockedOr8
#ifdef _M_ARM
#define InterlockedOrAcquire8 _InterlockedOr8_acq
#define InterlockedOrRelease8 _InterlockedOr8_rel
#define InterlockedOrNoFence8 _InterlockedOr8_nf
#elif defined(_M_IA64)
#define InterlockedOr8Acquire  _InterlockedOr8_acq
#define InterlockedOr8Release  _InterlockedOr8_rel
#endif // _M_ARM

#define InterlockedOr16 _InterlockedOr16
#ifdef _M_ARM
#define InterlockedOrAcquire16 _InterlockedOr16_acq
#define InterlockedOrRelease16 _InterlockedOr16_rel
#define InterlockedOrNoFence16 _InterlockedOr16_nf
#elif defined(_M_IA64)
#define InterlockedOr16Acquire _InterlockedOr16_acq
#define InterlockedOr16Release _InterlockedOr16_rel
#endif // _M_ARM

#define InterlockedOr64 _InterlockedOr64
#ifdef _M_ARM
#define InterlockedOrAcquire64 _InterlockedOr64_acq
#define InterlockedOrRelease64 _InterlockedOr64_rel
#define InterlockedOrNoFence64 _InterlockedOr64_nf
#elif defined(_M_IA64) || defined(_M_AMD64)
#define InterlockedOr64Acquire __ACQ_(_InterlockedOr64)
#define InterlockedOr64Release __REL_(_InterlockedOr64)
#define InterlockedOr64NoFence __NF_(_InterlockedOr64)
#endif // _M_ARM

#ifdef _WIN64
#define InterlockedOrAffinity InterlockedOr64
#else
#define InterlockedOrAffinity InterlockedOr
#endif // _WIN64

#define InterlockedXor _InterlockedXor
#define InterlockedXorAcquire __ACQ_(_InterlockedXor)
#define InterlockedXorRelease __REL_(_InterlockedXor)
#define InterlockedXorNoFence __NF_(_InterlockedXor)

#define InterlockedXor8 _InterlockedXor8
#ifdef _M_ARM
#define InterlockedXorAcquire8 _InterlockedXor8_acq
#define InterlockedXorRelease8 _InterlockedXor8_rel
#define InterlockedXorNoFence8 _InterlockedXor8_nf
#elif defined(_M_IA64)
#define InterlockedXor8Acquire _InterlockedXor8_acq
#define InterlockedXor8Release _InterlockedXor8_rel
#endif /* _M_ARM */

#define InterlockedXor16 _InterlockedXor16
#ifdef _M_ARM
#define InterlockedXorAcquire16 _InterlockedXor16_acq
#define InterlockedXorRelease16 _InterlockedXor16_rel
#define InterlockedXorNoFence16 _InterlockedXor16_nf
#elif defined(_M_IA64)
#define InterlockedXor16Acquire _InterlockedXor16_acq
#define InterlockedXor16Release _InterlockedXor16_rel
#endif /* _M_ARM */

#define InterlockedXor64 _InterlockedXor64
#ifdef _M_ARM
#define InterlockedXorAcquire64 _InterlockedXor64_acq
#define InterlockedXorRelease64 _InterlockedXor64_rel
#define InterlockedXorNoFence64 _InterlockedXor64_nf
#elif defined(_M_IA64) || defined(_M_AMD64)
#define InterlockedXor64Acquire __ACQ_(_InterlockedXor64)
#define InterlockedXor64Release __REL_(_InterlockedXor64)
#define InterlockedXor64NoFence __NF_(_InterlockedXor64)
#endif /* _M_ARM */

#ifdef _M_IX86

FORCEINLINE
LONG64
_InterlockedExchange64(
    _Inout_ _Interlocked_operand_ volatile LONG64 *Target,
    _In_ LONG64 Value)
{
    LONG64 Old, Prev;
    for (Old = *Target; ; Old = Prev)
    {
        Prev = _InterlockedCompareExchange64(Target, Value, Old);
        if (Prev == Old)
            return Prev;
    }
}

FORCEINLINE
LONG64
_InterlockedAdd64(
    _Inout_ _Interlocked_operand_ volatile LONG64 *Target,
    _In_ LONG64 Value)
{
    LONG64 Old, Prev, New;
    for (Old = *Target; ; Old = Prev)
    {
        New = Old + Value;
        Prev = _InterlockedCompareExchange64(Target, New, Old);
        if (Prev == Old)
            return New;
    }
}

FORCEINLINE
LONG64
_InterlockedExchangeAdd64 (
    _Inout_ _Interlocked_operand_ volatile LONG64 *Target,
    _In_ LONG64 Value
    )
{
    LONG64 Old, Prev, New;
    for (Old = *Target; ; Old = Prev)
    {
        New = Old + Value;
        Prev = _InterlockedCompareExchange64(Target, New, Old);
        if (Prev == Old)
            return Prev;
    }
}

FORCEINLINE
LONG64
_InterlockedAnd64(
    _Inout_ _Interlocked_operand_ volatile LONG64 *Target,
    _In_ LONG64 Value)
{
    LONG64 Old, Prev, New;
    for (Old = *Target; ; Old = Prev)
    {
        New = Old & Value;
        Prev = _InterlockedCompareExchange64(Target, New, Old);
        if (Prev == Old)
            return New;
    }
}

FORCEINLINE
LONG64
_InterlockedOr64(
    _Inout_ _Interlocked_operand_ volatile LONG64 *Target,
    _In_ LONG64 Value)
{
    LONG64 Old, Prev, New;
    for (Old = *Target; ; Old = Prev)
    {
        New = Old | Value;
        Prev = _InterlockedCompareExchange64(Target, New, Old);
        if (Prev == Old)
            return New;
    }
}

FORCEINLINE
LONG64
_InterlockedXor64(
    _Inout_ _Interlocked_operand_ volatile LONG64 *Target,
    _In_ LONG64 Value)
{
    LONG64 Old, Prev, New;
    for (Old = *Target; ; Old = Prev)
    {
        New = Old ^ Value;
        Prev = _InterlockedCompareExchange64(Target, New, Old);
        if (Prev == Old)
            return New;
    }
}

FORCEINLINE
LONG64
_InterlockedIncrement64(
    _Inout_ _Interlocked_operand_ volatile LONG64 *Target)
{
    return _InterlockedAdd64(Target, 1);
}

FORCEINLINE
LONG64
_InterlockedDecrement64(
    _Inout_ _Interlocked_operand_ volatile LONG64 *Target)
{
    return _InterlockedAdd64(Target, -1);
}

#undef _InterlockedExchangePointer
#define _InterlockedExchangePointer _InlineInterlockedExchangePointer
FORCEINLINE
_Ret_writes_(_Inexpressible_(Unknown))
PVOID
_InterlockedExchangePointer(
    _Inout_ _At_(*Destination, _Pre_writable_byte_size_(_Inexpressible_(Unknown))
        _Post_writable_byte_size_(_Inexpressible_(Unknown)))
        _Interlocked_operand_ volatile PVOID *Destination,
    _In_opt_ PVOID Value)
{
    return (PVOID)InterlockedExchange((volatile long *)Destination, (long)Value);
}

#undef _InterlockedCompareExchangePointer
#define _InterlockedCompareExchangePointer _InlineInterlockedCompareExchangePointer
FORCEINLINE
_Ret_writes_(_Inexpressible_(Unknown))
PVOID
_InterlockedCompareExchangePointer(
    _Inout_ _At_(*Destination, _Pre_writable_byte_size_(_Inexpressible_(Unknown))
        _Post_writable_byte_size_(_Inexpressible_(Unknown)))
        _Interlocked_operand_ volatile PVOID *Destination,
    _In_opt_ PVOID ExChange,
    _In_opt_ PVOID Comperand)
{
    return (PVOID)InterlockedCompareExchange((volatile long *)Destination,
                                             (long)ExChange,
                                             (long)Comperand);
}

#endif /* _M_IX86 */

#ifdef _M_AMD64

FORCEINLINE
LONG64
_InterlockedAdd64(
    _Inout_ _Interlocked_operand_ volatile LONG64 *Target,
    _In_ LONG64 Value)

{
    return _InterlockedExchangeAdd64(Target, Value) + Value;
}

#endif /* _M_AMD64 */

#ifdef _M_IA64

#undef _InterlockedBitTestAndSet
#define _InterlockedBitTestAndSet InterlockedBitTestAndSet_Inline
FORCEINLINE
BOOLEAN
_InterlockedBitTestAndSet(
    _Inout_ _Interlocked_operand_ volatile LONG *Target,
    _In_ LONG Bit)
{
    ULONG Mask = 1 << (Bit & 31);
    return (BOOLEAN)((InterlockedOr(&Target[Bit / 32], Mask) & Mask) != 0);
}

#undef _InterlockedBitTestAndReset
#define _InterlockedBitTestAndReset InterlockedBitTestAndReset_Inline
FORCEINLINE
BOOLEAN
_InterlockedBitTestAndReset(
    _Inout_ _Interlocked_operand_ volatile LONG *Target,
    _In_ LONG Bit)
{
    ULONG Mask = 1 << (Bit & 31);
    return (BOOLEAN)((InterlockedAnd(&Target[Bit / 32], ~Mask) & Mask) != 0);
}

#undef _InterlockedBitTestAndSet64
#define _InterlockedBitTestAndSet64 InterlockedBitTestAndSet64_Inline
FORCEINLINE
BOOLEAN
_InterlockedBitTestAndSet64(
    _Inout_ _Interlocked_operand_ volatile LONG64 *Target,
    _In_ LONG64 Bit)
{
    ULONG64 Mask = 1LL << (Bit & 63);
    return (BOOLEAN)((InterlockedOr64(&Target[Bit / 64], Mask) & Mask) != 0);
}

#undef _InterlockedBitTestAndReset64
#define _InterlockedBitTestAndReset64 InterlockedBitTestAndReset64_Inline
FORCEINLINE
BOOLEAN
_InterlockedBitTestAndReset64(
    _Inout_ _Interlocked_operand_ volatile LONG64 *Target,
    _In_ LONG64 Bit)
{
    ULONG64 Mask = 1LL << (Bit & 63);
    return (BOOLEAN)((InterlockedAnd64(&Target[Bit / 64], ~Mask) & Mask) != 0);
}

#undef _InterlockedBitTestAndComplement
#define _InterlockedBitTestAndComplement InterlockedBitTestAndComplement_Inline
FORCEINLINE
BOOLEAN
_InterlockedBitTestAndComplement(
    _Inout_ _Interlocked_operand_ volatile LONG *Target,
    _In_ LONG Bit)
{
    ULONG Mask = 1 << (Bit & 31);
    return (BOOLEAN)((InterlockedXor(&Target[Bit / 32], Mask) & Mask) != 0);
}

#undef _InterlockedBitTestAndComplement64
#define _InterlockedBitTestAndComplement64 InterlockedBitTestAndComplement64_Inline
FORCEINLINE
BOOLEAN
_InterlockedBitTestAndComplement64(
    _Inout_ _Interlocked_operand_ volatile LONG64 *Target,
    _In_ LONG64 Bit)
{
    ULONG64 Mask = 1LL << (Bit & 63);
    return (BOOLEAN)((InterlockedXor64(&Target[Bit / 64], Mask) & Mask) != 0);
}

#endif /* M_IA64 */

