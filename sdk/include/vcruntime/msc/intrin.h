
#ifdef __cplusplus
extern "C" {
#endif

/*** Stack frame juggling ***/
#pragma intrinsic(_ReturnAddress)
#pragma intrinsic(_AddressOfReturnAddress)
#if defined(_M_IX86) || defined(_M_AMD64)
#pragma intrinsic(__getcallerseflags)
#endif

/*** Memory barriers ***/
#pragma intrinsic(_ReadWriteBarrier)
#pragma intrinsic(_ReadBarrier)
#pragma intrinsic(_WriteBarrier)
#if defined(_M_IX86) || defined(_M_AMD64)
#pragma intrinsic(_mm_mfence)
#pragma intrinsic(_mm_lfence)
#pragma intrinsic(_mm_sfence)
#endif
#if defined(_M_AMD64)
#pragma intrinsic(__faststorefence)
#elif defined(_M_ARM)
#pragma intrinsic(__iso_volatile_load16)
#pragma intrinsic(__iso_volatile_load32)
#pragma intrinsic(__iso_volatile_load64)
#pragma intrinsic(__iso_volatile_load8)
#pragma intrinsic(__iso_volatile_store16)
#pragma intrinsic(__iso_volatile_store32)
#pragma intrinsic(__iso_volatile_store64)
#pragma intrinsic(__iso_volatile_store8)
#endif

/*** Atomic operations ***/
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedCompareExchange8)
#pragma intrinsic(_InterlockedCompareExchange16)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchange8)
#pragma intrinsic(_InterlockedExchange16)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedExchangeAdd8)
#pragma intrinsic(_InterlockedExchangeAdd16)
#pragma intrinsic(_InterlockedAnd8)
#pragma intrinsic(_InterlockedAnd16)
#pragma intrinsic(_InterlockedAnd)
#pragma intrinsic(_InterlockedOr8)
#pragma intrinsic(_InterlockedOr16)
#pragma intrinsic(_InterlockedOr)
#pragma intrinsic(_InterlockedXor8)
#pragma intrinsic(_InterlockedXor16)
#pragma intrinsic(_InterlockedXor)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement16)
#pragma intrinsic(_InterlockedIncrement16)
#pragma intrinsic(_interlockedbittestandreset)
#pragma intrinsic(_interlockedbittestandset)
#if defined(_M_IX86)
#pragma intrinsic(_InterlockedAddLargeStatistic)
#elif defined(_M_AMD64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedCompareExchangePointer)
#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedCompareExchange128)
#pragma intrinsic(_InterlockedAnd64)
#pragma intrinsic(_InterlockedOr64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_interlockedbittestandreset64)
#pragma intrinsic(_interlockedbittestandset64)
#pragma intrinsic(_InterlockedAnd_np)
#pragma intrinsic(_InterlockedAnd8_np)
#pragma intrinsic(_InterlockedAnd16_np)
#pragma intrinsic(_InterlockedAnd64_np)
#pragma intrinsic(_InterlockedCompareExchange16_np)
#pragma intrinsic(_InterlockedCompareExchange64_np)
#pragma intrinsic(_InterlockedCompareExchange128_np)
#pragma intrinsic(_InterlockedCompareExchangePointer_np)
#pragma intrinsic(_InterlockedCompareExchange_np)
#pragma intrinsic(_InterlockedOr16_np)
#pragma intrinsic(_InterlockedOr8_np)
#pragma intrinsic(_InterlockedOr_np)
#pragma intrinsic(_InterlockedXor16_np)
#pragma intrinsic(_InterlockedXor64_np)
#pragma intrinsic(_InterlockedXor8_np)
#pragma intrinsic(_InterlockedXor_np)
#pragma intrinsic(_InterlockedOr64_np)
#elif defined(_M_ARM)

#endif

#if defined(_M_AMD64) || defined(_M_ARM)
#endif

/*** String operations ***/
#if defined(_M_IX86) || defined(_M_AMD64)
#pragma intrinsic(__stosb)
#pragma intrinsic(__stosw)
#pragma intrinsic(__stosd)
#pragma intrinsic(__movsb)
#pragma intrinsic(__movsw)
#pragma intrinsic(__movsd)
#endif
#ifdef _M_AMD64
#pragma intrinsic(__stosq)
#pragma intrinsic(__movsq)
#endif

/*** GS segment addressing ***/
#if defined(_M_AMD64)
#pragma intrinsic(__writegsbyte)
#pragma intrinsic(__writegsword)
#pragma intrinsic(__writegsdword)
#pragma intrinsic(__writegsqword)
#pragma intrinsic(__readgsbyte)
#pragma intrinsic(__readgsword)
#pragma intrinsic(__readgsdword)
#pragma intrinsic(__readgsqword)
#pragma intrinsic(__incgsbyte)
#pragma intrinsic(__incgsword)
#pragma intrinsic(__incgsdword)
#pragma intrinsic(__incgsqword)
#pragma intrinsic(__addgsbyte)
#pragma intrinsic(__addgsword)
#pragma intrinsic(__addgsdword)
#pragma intrinsic(__addgsqword)
#endif

/*** FS segment addressing ***/
#if defined(_M_IX86)
#pragma intrinsic(__writefsbyte)
#pragma intrinsic(__writefsword)
#pragma intrinsic(__writefsdword)
#pragma intrinsic(__writefsdword)
#pragma intrinsic(__readfsbyte)
#pragma intrinsic(__readfsword)
#pragma intrinsic(__readfsdword)
#pragma intrinsic(__incfsbyte)
#pragma intrinsic(__incfsword)
#pragma intrinsic(__incfsdword)
#pragma intrinsic(__addfsbyte)
#pragma intrinsic(__addfsword)
#pragma intrinsic(__addfsdword)
#endif

/*** Bit manipulation ***/
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#ifdef _WIN64
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)
#endif
#pragma intrinsic(_bittest)
#pragma intrinsic(_bittestandcomplement)
#pragma intrinsic(_bittestandreset)
#pragma intrinsic(_bittestandset)
#pragma intrinsic(_rotl8)
#pragma intrinsic(_rotl16)
#pragma intrinsic(_rotl)
#pragma intrinsic(_rotl64)
#pragma intrinsic(_lrotl)
#pragma intrinsic(_rotr8)
#pragma intrinsic(_rotr16)
#pragma intrinsic(_rotr)
#pragma intrinsic(_rotr64)
#pragma intrinsic(_lrotr)
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)
#if defined(_M_IX86) || defined(_M_AMD64)
#pragma intrinsic(__ll_lshift)
#pragma intrinsic(__ll_rshift)
#pragma intrinsic(__ull_rshift)
#pragma intrinsic(__lzcnt)
#pragma intrinsic(__lzcnt16)
#pragma intrinsic(__popcnt)
#pragma intrinsic(__popcnt16)
#endif
#ifdef _M_AMD64
#pragma intrinsic(__shiftleft128)
#pragma intrinsic(__shiftright128)
#pragma intrinsic(_bittest64)
#pragma intrinsic(_bittestandcomplement64)
#pragma intrinsic(_bittestandreset64)
#pragma intrinsic(_bittestandset64)
#pragma intrinsic(__lzcnt64)
#pragma intrinsic(__popcnt64)
#elif defined(_M_ARM)

#endif

/*** 64/128-bit math ***/
#pragma intrinsic(_abs64)
#if defined(_M_IX86) || defined(_M_AMD64)
#pragma intrinsic(__emul)
#pragma intrinsic(__emulu)
#endif
#ifdef _M_AMD64
#pragma intrinsic(__mulh)
#pragma intrinsic(__umulh)
#pragma intrinsic(_mul128)
#pragma intrinsic(_umul128)
#elif defined(_M_ARM)
#pragma intrinsic(_MulHigh)
#pragma intrinsic(_MulUnsignedHigh)
#endif

/** Floating point stuff **/
#if defined(_M_ARM)
#pragma intrinsic(_isunordered)
#pragma intrinsic(_isunorderedf)
#pragma intrinsic(_CopyDoubleFromInt64)
#pragma intrinsic(_CopyFloatFromInt32)
#pragma intrinsic(_CopyInt32FromFloat)
#pragma intrinsic(_CopyInt64FromDouble)
#endif

/*** Port I/O ***/
#if defined(_M_IX86) || defined(_M_AMD64)
#pragma intrinsic(__inbyte)
#pragma intrinsic(__inword)
#pragma intrinsic(__indword)
#pragma intrinsic(__inbytestring)
#pragma intrinsic(__inwordstring)
#pragma intrinsic(__indwordstring)
#pragma intrinsic(__outbyte)
#pragma intrinsic(__outword)
#pragma intrinsic(__outdword)
#pragma intrinsic(__outbytestring)
#pragma intrinsic(__outwordstring)
#pragma intrinsic(__outdwordstring)
#pragma intrinsic(_inp)
#pragma intrinsic(_inpd)
#pragma intrinsic(_inpw)
#pragma intrinsic(inp)
#pragma intrinsic(inpd)
#pragma intrinsic(inpw)
#pragma intrinsic(_outp)
#pragma intrinsic(_outpd)
#pragma intrinsic(_outpw)
#pragma intrinsic(outp)
#pragma intrinsic(outpd)
#pragma intrinsic(outpw)
#endif

/*** System information ***/
#if defined(_M_IX86) || defined(_M_AMD64)
#pragma intrinsic(__cpuid)
#pragma intrinsic(__cpuidex)
#pragma intrinsic(__rdtsc)
#pragma intrinsic(__rdtscp)
#pragma intrinsic(__writeeflags)
#pragma intrinsic(__readeflags)
#endif

/*** Interrupts and traps ***/
#pragma intrinsic(__debugbreak)
#pragma intrinsic(_disable)
#pragma intrinsic(_enable)
#if defined(_M_IX86) || defined(_M_AMD64)
#pragma intrinsic(__int2c)
#pragma intrinsic(__halt)
#pragma intrinsic(__ud2)
#if (_MSC_VER >= 1700)
#pragma intrinsic(__fastfail)
#else
#if defined(_M_IX86)
__declspec(noreturn) __forceinline
void __fastfail(unsigned int Code)
{
    __asm
    {
        mov ecx, Code
        int 29h
    }
}
#else
void __fastfail(unsigned int Code);
#endif // defined(_M_IX86)
#endif
#endif
#if defined(_M_ARM)
#endif

/*** Protected memory management ***/
#if defined(_M_IX86) || defined(_M_AMD64)
#pragma intrinsic(__writecr0)
#pragma intrinsic(__writecr3)
#pragma intrinsic(__writecr4)
#pragma intrinsic(__writecr8)
#endif
#if defined(_M_IX86)
#pragma intrinsic(__readcr0)
#pragma intrinsic(__readcr2)
#pragma intrinsic(__readcr3)
//#pragma intrinsic(__readcr4)
// HACK: MSVC is broken
unsigned long __cdecl  ___readcr4(void);
#define __readcr4 ___readcr4
#pragma intrinsic(__readcr8)
#pragma intrinsic(__readdr)
#pragma intrinsic(__writedr)
// This intrinsic is broken and generates wrong opcodes,
// when optimization is enabled!
#pragma warning(push)
#pragma warning(disable:4711)
void  __forceinline __invlpg_fixed(void * Address)
{
    _ReadWriteBarrier();
   __asm
   {
       mov eax, Address
       invlpg [eax]
   }
    _ReadWriteBarrier();
}
#pragma warning(pop)
#define __invlpg __invlpg_fixed
#elif defined(_M_AMD64)
#pragma intrinsic(__invlpg)
#pragma intrinsic(__readcr0)
#pragma intrinsic(__readcr2)
#pragma intrinsic(__readcr3)
#pragma intrinsic(__readcr4)
#pragma intrinsic(__readcr8)
#pragma intrinsic(__readdr)
#pragma intrinsic(__writedr)
#elif defined(_M_ARM)
#pragma intrinsic(__prefetch)
#endif

/*** System operations ***/
#if defined(_M_IX86) || defined(_M_AMD64)
#pragma intrinsic(__readmsr)
#pragma intrinsic(__writemsr)
#pragma intrinsic(__readpmc)
#pragma intrinsic(__segmentlimit)
#pragma intrinsic(__wbinvd)
#pragma intrinsic(__lidt)
#pragma intrinsic(__sidt)
#if (_MSC_VER >= 1800)
#pragma intrinsic(_sgdt)
#else
#if defined(_M_IX86)
__forceinline
void _sgdt(void *Destination)
{
    __asm
    {
        mov eax, Destination
        sgdt [eax]
    }
}
#else
void _sgdt(void *Destination);
#endif // defined(_M_IX86)
#endif
#pragma intrinsic(_mm_pause)
#endif
#if defined(_M_ARM)
#pragma intrinsic(_MoveFromCoprocessor)
#pragma intrinsic(_MoveFromCoprocessor2)
#pragma intrinsic(_MoveFromCoprocessor64)
#pragma intrinsic(_MoveToCoprocessor)
#pragma intrinsic(_MoveToCoprocessor2)
#pragma intrinsic(_MoveToCoprocessor64)
#pragma intrinsic(_ReadStatusReg)
#pragma intrinsic(_WriteStatusReg)
#pragma intrinsic(__yield)
#pragma intrinsic(__wfe)
#pragma intrinsic(__wfi)
#pragma intrinsic(__swi)
#pragma intrinsic(__hvc)
#pragma intrinsic(__ldrexd)
#pragma intrinsic(__rdpmccntr64)
#pragma intrinsic(__sev)
#endif

/** Secure virtual machine **/
#if defined(_M_IX86) || defined(_M_AMD64)
#pragma intrinsic(__svm_clgi)
#pragma intrinsic(__svm_invlpga)
#pragma intrinsic(__svm_skinit)
#pragma intrinsic(__svm_stgi)
#pragma intrinsic(__svm_vmload)
#pragma intrinsic(__svm_vmrun)
#pragma intrinsic(__svm_vmsave)
#endif

/** Virtual machine extension **/
#if defined(_M_IX86) || defined(_M_AMD64)

#endif
#if defined(_M_AMD64)

#endif

/** Misc **/
#pragma intrinsic(__nop)
#if (_MSC_VER >= 1700)
#pragma intrinsic(__code_seg)
#endif
#ifdef _M_ARM
#pragma intrinsic(_AddSatInt)
#pragma intrinsic(_DAddSatInt)
#pragma intrinsic(_DSubSatInt)
#pragma intrinsic(_SubSatInt)
#pragma intrinsic(__emit)
#pragma intrinsic(__static_assert)
#endif

#ifdef __cplusplus
}
#endif

/* EOF */
