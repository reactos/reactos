#pragma once

/* ARM64 Compiler Intrinsics for ReactOS Kernel */

#ifdef __cplusplus  
extern "C" {
#endif

/* ARM64 System Register Access */
#define __readcr0()         ARM64_READ_SYSREG(sctlr_el1)
#define __writecr0(val)     ARM64_WRITE_SYSREG(sctlr_el1, val)

/* ARM64 Memory Barriers */
#define __dsb(option)       __asm__ volatile("dsb " #option ::: "memory")
#define __dmb(option)       __asm__ volatile("dmb " #option ::: "memory") 
#define __isb()             __asm__ volatile("isb" ::: "memory")

/* ARM64 Cache Operations */
#define __dc_civac(addr)    __asm__ volatile("dc civac, %0" :: "r"(addr) : "memory")
#define __dc_cvau(addr)     __asm__ volatile("dc cvau, %0" :: "r"(addr) : "memory")
#define __dc_cvac(addr)     __asm__ volatile("dc cvac, %0" :: "r"(addr) : "memory")
#define __ic_ivau(addr)     __asm__ volatile("ic ivau, %0" :: "r"(addr) : "memory")

/* ARM64 TLB Operations */
#define __tlbi_vmalle1()    __asm__ volatile("tlbi vmalle1" ::: "memory")
#define __tlbi_vae1(addr)   __asm__ volatile("tlbi vae1, %0" :: "r"((addr) >> 12) : "memory")

/* ARM64 Interrupt Control */
#define __disable_irq()     __asm__ volatile("msr daifset, #0x2" ::: "memory")
#define __enable_irq()      __asm__ volatile("msr daifclr, #0x2" ::: "memory")
#define __disable_fiq()     __asm__ volatile("msr daifset, #0x1" ::: "memory")
#define __enable_fiq()      __asm__ volatile("msr daifclr, #0x1" ::: "memory")

/* ARM64 Wait Instructions */
#define __wfi()             __asm__ volatile("wfi" ::: "memory")
#define __wfe()             __asm__ volatile("wfe" ::: "memory")
#define __sev()             __asm__ volatile("sev" ::: "memory")
#define __sevl()            __asm__ volatile("sevl" ::: "memory")
#define __yield()           __asm__ volatile("yield" ::: "memory")

/* ARM64 Debug Instructions */
#define __brk(val)          __asm__ volatile("brk %0" :: "i"(val))

/* ARM64 Atomic Operations using GCC built-ins */
#define _InterlockedIncrement(ptr) \
    __sync_add_and_fetch(ptr, 1)

#define _InterlockedDecrement(ptr) \
    __sync_sub_and_fetch(ptr, 1)

#define _InterlockedExchange(ptr, val) \
    __sync_lock_test_and_set(ptr, val)

#define _InterlockedCompareExchange(ptr, new_val, old_val) \
    __sync_val_compare_and_swap(ptr, old_val, new_val)

#define _InterlockedExchangeAdd(ptr, val) \
    __sync_fetch_and_add(ptr, val)

#define _InterlockedOr(ptr, val) \
    __sync_fetch_and_or(ptr, val)

#define _InterlockedAnd(ptr, val) \
    __sync_fetch_and_and(ptr, val)

#define _InterlockedXor(ptr, val) \
    __sync_fetch_and_xor(ptr, val)

/* 64-bit versions */
#define _InterlockedIncrement64(ptr) \
    __sync_add_and_fetch(ptr, 1LL)

#define _InterlockedDecrement64(ptr) \
    __sync_sub_and_fetch(ptr, 1LL)

#define _InterlockedExchange64(ptr, val) \
    __sync_lock_test_and_set(ptr, val)

#define _InterlockedCompareExchange64(ptr, new_val, old_val) \
    __sync_val_compare_and_swap(ptr, old_val, new_val)

#define _InterlockedExchangeAdd64(ptr, val) \
    __sync_fetch_and_add(ptr, val)

/* ARM64 Bit Manipulation */
static __inline__ unsigned char _BitScanForward(unsigned long *Index, unsigned long Mask)
{
    if (Mask == 0) return 0;
    *Index = __builtin_ctzl(Mask);
    return 1;
}

static __inline__ unsigned char _BitScanReverse(unsigned long *Index, unsigned long Mask) 
{
    if (Mask == 0) return 0;
    *Index = 31 - __builtin_clzl(Mask);
    return 1;
}

static __inline__ unsigned char _BitScanForward64(unsigned long *Index, unsigned long long Mask)
{
    if (Mask == 0) return 0;
    *Index = __builtin_ctzll(Mask);
    return 1;
}

static __inline__ unsigned char _BitScanReverse64(unsigned long *Index, unsigned long long Mask)
{
    if (Mask == 0) return 0;
    *Index = 63 - __builtin_clzll(Mask);
    return 1;
}

/* ARM64 Read/Write System Registers */
static __inline__ unsigned long long __readcntvct(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

static __inline__ unsigned long long __readcntfrq(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(val));
    return val;
}

static __inline__ unsigned long long __readmidr(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, midr_el1" : "=r"(val));
    return val;
}

static __inline__ unsigned long long __readmpidr(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, mpidr_el1" : "=r"(val));
    return val;
}

static __inline__ unsigned long long __readcurrentel(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, currentel" : "=r"(val));
    return val;
}

static __inline__ void __writevbar_el1(unsigned long long val)
{
    __asm__ volatile("msr vbar_el1, %0" :: "r"(val));
    __isb();
}

static __inline__ unsigned long long __readvbar_el1(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, vbar_el1" : "=r"(val));
    return val;
}

/* ARM64 Stack Pointer Manipulation */
static __inline__ unsigned long long __readsp(void)
{
    unsigned long long val;
    __asm__ volatile("mov %0, sp" : "=r"(val));
    return val;
}

static __inline__ void __writesp(unsigned long long val)
{
    __asm__ volatile("mov sp, %0" :: "r"(val) : "memory");
}

/* ARM64 Exception State */
static __inline__ unsigned long long __readdaif(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, daif" : "=r"(val));
    return val;
}

static __inline__ void __writedaif(unsigned long long val)
{
    __asm__ volatile("msr daif, %0" :: "r"(val) : "memory");
}

/* ARM64 Memory Management */
static __inline__ void __writettbr0_el1(unsigned long long val)
{
    __asm__ volatile("msr ttbr0_el1, %0" :: "r"(val));
    __isb();
}

static __inline__ unsigned long long __readttbr0_el1(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, ttbr0_el1" : "=r"(val));
    return val;
}

static __inline__ void __writettbr1_el1(unsigned long long val)
{
    __asm__ volatile("msr ttbr1_el1, %0" :: "r"(val));
    __isb();
}

static __inline__ unsigned long long __readttbr1_el1(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, ttbr1_el1" : "=r"(val));
    return val;
}

static __inline__ void __writetcr_el1(unsigned long long val)
{
    __asm__ volatile("msr tcr_el1, %0" :: "r"(val));
    __isb();
}

static __inline__ unsigned long long __readtcr_el1(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, tcr_el1" : "=r"(val));
    return val;
}

static __inline__ void __writemair_el1(unsigned long long val)
{
    __asm__ volatile("msr mair_el1, %0" :: "r"(val));
    __isb();
}

static __inline__ unsigned long long __readmair_el1(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, mair_el1" : "=r"(val));
    return val;
}

static __inline__ void __writesctlr_el1(unsigned long long val)
{
    __asm__ volatile("msr sctlr_el1, %0" :: "r"(val));
    __isb();
}

static __inline__ unsigned long long __readsctlr_el1(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, sctlr_el1" : "=r"(val));
    return val;
}

/* ARM64 Cache Maintenance by Set/Way */
static __inline__ void __dc_cisw(unsigned long long val)
{
    __asm__ volatile("dc cisw, %0" :: "r"(val) : "memory");
}

static __inline__ void __dc_csw(unsigned long long val)
{
    __asm__ volatile("dc csw, %0" :: "r"(val) : "memory");
}

static __inline__ void __dc_isw(unsigned long long val)
{
    __asm__ volatile("dc isw, %0" :: "r"(val) : "memory");
}

/* ARM64 Instruction Cache */
static __inline__ void __ic_ialluis(void)
{
    __asm__ volatile("ic ialluis" ::: "memory");
}

static __inline__ void __ic_iallu(void)
{
    __asm__ volatile("ic iallu" ::: "memory");
}

/* ARM64 Branch Prediction */
static __inline__ void __br_i_pred_inval_all(void)
{
    /* ARM64 doesn't have a direct equivalent - use ISB */
    __isb();
}

/* ARM64 Performance Monitoring */
static __inline__ unsigned long long __readpmcr_el0(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, pmcr_el0" : "=r"(val));
    return val;
}

static __inline__ void __writepmcr_el0(unsigned long long val)
{
    __asm__ volatile("msr pmcr_el0, %0" :: "r"(val));
    __isb();
}

/* ARM64 Generic Timers */
static __inline__ void __writecntp_tval_el0(unsigned int val)
{
    __asm__ volatile("msr cntp_tval_el0, %0" :: "r"((unsigned long long)val));
    __isb();
}

static __inline__ unsigned int __readcntp_tval_el0(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, cntp_tval_el0" : "=r"(val));
    return (unsigned int)val;
}

static __inline__ void __writecntp_ctl_el0(unsigned int val)
{
    __asm__ volatile("msr cntp_ctl_el0, %0" :: "r"((unsigned long long)val));
    __isb();
}

static __inline__ unsigned int __readcntp_ctl_el0(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, cntp_ctl_el0" : "=r"(val));
    return (unsigned int)val;
}

/* ARM64 Random Number Generation */
static __inline__ unsigned long long __rndr(void)
{
    unsigned long long val;
    __asm__ volatile("mrs %0, s3_3_c2_c4_0" : "=r"(val)); /* RNDR */
    return val;
}

#ifdef __cplusplus
}
#endif