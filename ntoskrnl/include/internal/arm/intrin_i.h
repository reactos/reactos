#ifndef _INTRIN_INTERNAL_
#define _INTRIN_INTERNAL_

FORCEINLINE
VOID
KeArmHaltProcessor(void)
{
    //
    // Enter Wait-For-Interrupt Mode
    //
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c0, 4" : : "r"(0) : "cc");
}

FORCEINLINE
ARM_CONTROL_REGISTER
KeArmControlRegisterGet(VOID)
{
    ARM_CONTROL_REGISTER Value;
    __asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0" : "=r"(Value.AsUlong) : : "cc");
    return Value;
}

FORCEINLINE
ARM_ID_CODE_REGISTER
KeArmIdCodeRegisterGet(VOID)
{
    ARM_ID_CODE_REGISTER Value;
    __asm__ __volatile__ ("mrc p15, 0, %0, c0, c0, 0" : "=r"(Value.AsUlong) : : "cc");
    return Value;
}

FORCEINLINE
ULONG
KeArmFaultStatusRegisterGet(VOID)
{
    ULONG Value;
    __asm__ __volatile__ ("mrc p15, 0, %0, c5, c0, 0" : "=r"(Value) : : "cc");
    return Value;
}

FORCEINLINE
ULONG
KeArmInstructionFaultStatusRegisterGet(VOID)
{
    ULONG Value;
    __asm__ __volatile__ ("mrc p15, 0, %0, c5, c0, 1" : "=r"(Value) : : "cc");
    return Value;
}

FORCEINLINE
ULONG
KeArmFaultAddressRegisterGet(VOID)
{
    ULONG Value;
    __asm__ __volatile__ ("mrc p15, 0, %0, c6, c0, 0" : "=r"(Value) : : "cc");
    return Value;
}

FORCEINLINE
ARM_LOCKDOWN_REGISTER
KeArmLockdownRegisterGet(VOID)
{
    ARM_LOCKDOWN_REGISTER Value;
    __asm__ __volatile__ ("mrc p15, 0, %0, c10, c0, 0" : "=r"(Value.AsUlong) : : "cc");
    return Value;
}

FORCEINLINE
ARM_TTB_REGISTER
KeArmTranslationTableRegisterGet(VOID)
{
    ARM_TTB_REGISTER Value;
    __asm__ __volatile__ ("mrc p15, 0, %0, c2, c0, 0" : "=r"(Value.AsUlong) : : "cc");
    return Value;
}

FORCEINLINE
ARM_CACHE_REGISTER
KeArmCacheRegisterGet(VOID)
{
    ARM_CACHE_REGISTER Value;
    __asm__ __volatile__ ("mrc p15, 0, %0, c0, c0, 1" : "=r"(Value.AsUlong) : : "cc");
    return Value;
}

FORCEINLINE
ARM_STATUS_REGISTER
KeArmStatusRegisterGet(VOID)
{
    ARM_STATUS_REGISTER Value;
    __asm__ __volatile__ ("mrs %0, cpsr" : "=r"(Value.AsUlong) : : "cc");
    return Value;    
}

FORCEINLINE
VOID
KeArmControlRegisterSet(IN ARM_CONTROL_REGISTER ControlRegister)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0" : : "r"(ControlRegister.AsUlong) : "cc");    
}

FORCEINLINE
VOID
KeArmTranslationTableRegisterSet(IN ARM_TTB_REGISTER Ttb)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c2, c0, 0" : : "r"(Ttb.AsUlong) : "cc");
}

FORCEINLINE
VOID
KeArmDomainRegisterSet(IN ARM_DOMAIN_REGISTER DomainRegister)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c3, c0, 0" : : "r"(DomainRegister.AsUlong) : "cc");
}

FORCEINLINE
VOID
KeArmLockdownRegisterSet(IN ARM_LOCKDOWN_REGISTER LockdownRegister)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c10, c0, 0" : : "r"(LockdownRegister.AsUlong) : "cc");
}

FORCEINLINE
VOID
KeArmFlushTlb(VOID)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c8, c7, 0" : : "r"(0) : "cc");
}

FORCEINLINE
VOID
KeArmInvalidateTlbEntry(IN PVOID Address)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c8, c7, 1" : : "r"(Address) : "cc");
}

FORCEINLINE
VOID
KeArmInvalidateAllCaches(VOID)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c7, 0" : : "r"(0) : "cc");
}

FORCEINLINE
VOID
KeArmFlushIcache(VOID)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 0" : : "r"(0) : "cc");
}

FORCEINLINE
VOID
KeArmWaitForInterrupt(VOID)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c0, 4" : : "r"(0) : "cc");
}

#endif
