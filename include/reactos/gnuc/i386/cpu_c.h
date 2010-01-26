#ifndef _CASM_INC
#define _CASM_INC

#define Ke386SetGlobalDescriptorTable(X) \
    __asm__("lgdt %0\n\t" \
    : /* no outputs */ \
    : "m" (*X));

#define Ke386GetGlobalDescriptorTable(X) \
    __asm__("sgdt %0\n\t" \
    : "=m" (*X) \
    : /* no input */ \
    : "memory");

FORCEINLINE
USHORT
Ke386GetLocalDescriptorTable()
{
    USHORT Ldt;
    __asm__("sldt %0\n\t"
    : "=m" (Ldt)
    : /* no input */
    : "memory");
    return Ldt;
}

#define Ke386SetLocalDescriptorTable(X) \
    __asm__("lldt %w0\n\t" \
    : /* no outputs */ \
    : "q" (X));

#define Ke386SetTr(X)                   __asm__ __volatile__("ltr %%ax" : :"a" (X));

FORCEINLINE
USHORT
Ke386GetTr(VOID)
{
    USHORT Tr;
    __asm__("str %0\n\t"
    : "=m" (Tr));
    return Tr;
}

#define _Ke386GetSeg(N)           ({ \
                                     unsigned int __d; \
                                     __asm__("movl %%" #N ",%0\n\t" :"=r" (__d)); \
                                     __d; \
                                  })

#define _Ke386SetSeg(N,X)         __asm__ __volatile__("movl %0,%%" #N : :"r" (X));

#define Ke386FnInit()               __asm__("fninit\n\t");

//
// CR Macros
//
#define Ke386SetCr2(X)              __asm__ __volatile__("movl %0,%%cr2" : :"r" (X));

//
// Segment Macros
//
#define Ke386GetSs()                _Ke386GetSeg(ss)
#define Ke386GetFs()                _Ke386GetSeg(fs)
#define Ke386SetFs(X)               _Ke386SetSeg(fs, X)
#define Ke386SetDs(X)               _Ke386SetSeg(ds, X)
#define Ke386SetEs(X)               _Ke386SetSeg(es, X)
#define Ke386SetSs(X)               _Ke386SetSeg(ss, X)
#define Ke386SetGs(X)               _Ke386SetSeg(gs, X)

#define FPU_DOUBLE(var) double var; \
	__asm__ __volatile__( "fstpl %0;fwait" : "=m" (var) : )

#define FPU_DOUBLES(var1,var2) double var1,var2; \
	__asm__ __volatile__( "fstpl %0;fwait" : "=m" (var2) : ); \
	__asm__ __volatile__( "fstpl %0;fwait" : "=m" (var1) : )

// gcc bug workaround
#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ < 40300) || \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ == 40303) 
#define DEFINE_WAIT_BLOCK(x)                                \
    struct _AlignHack                                       \
    {                                                       \
        UCHAR Hack[15];                                     \
        EX_PUSH_LOCK_WAIT_BLOCK UnalignedBlock;             \
    } WaitBlockBuffer;                                      \
    PEX_PUSH_LOCK_WAIT_BLOCK x = (PEX_PUSH_LOCK_WAIT_BLOCK) \
        ((ULONG_PTR)&WaitBlockBuffer.UnalignedBlock &~ 0xF);
#else
// This is only for compatibility; the compiler will optimize the extra
// local variable (the actual pointer) away, so we don't take any perf hit
// by doing this.
#define DEFINE_WAIT_BLOCK(x)                                \
    EX_PUSH_LOCK_WAIT_BLOCK WaitBlockBuffer;                \
    PEX_PUSH_LOCK_WAIT_BLOCK x = &WaitBlockBuffer;
#endif


#endif // #ifndef _CASM_INC
