#pragma once

FORCEINLINE
VOID
KeArmHaltProcessor(VOID)
{
    //
    // Enter Wait-For-Interrupt Mode
    //
#ifdef _MSC_VER
#else
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c0, 4" : : "r"(0) : "cc");
#endif
}

FORCEINLINE
ARM_CONTROL_REGISTER
KeArmControlRegisterGet(VOID)
{
    ARM_CONTROL_REGISTER Value;
#ifdef _MSC_VER
    Value.AsUlong = 0;
#else
    __asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0" : "=r"(Value.AsUlong) : : "cc");
#endif
    return Value;
}

FORCEINLINE
ARM_ID_CODE_REGISTER
KeArmIdCodeRegisterGet(VOID)
{
    ARM_ID_CODE_REGISTER Value;
#ifdef _MSC_VER
    Value.AsUlong = 0;
#else
    __asm__ __volatile__ ("mrc p15, 0, %0, c0, c0, 0" : "=r"(Value.AsUlong) : : "cc");
#endif
    return Value;
}

FORCEINLINE
ULONG
KeArmFaultStatusRegisterGet(VOID)
{
    ULONG Value;
#ifdef _MSC_VER
    Value = 0;
#else
    __asm__ __volatile__ ("mrc p15, 0, %0, c5, c0, 0" : "=r"(Value) : : "cc");
#endif
    return Value;
}

FORCEINLINE
ULONG
KeArmInstructionFaultStatusRegisterGet(VOID)
{
    ULONG Value;
#ifdef _MSC_VER
    Value = 0;
#else
    __asm__ __volatile__ ("mrc p15, 0, %0, c5, c0, 1" : "=r"(Value) : : "cc");
#endif
    return Value;
}

FORCEINLINE
ULONG
KeArmFaultAddressRegisterGet(VOID)
{
    ULONG Value;
#ifdef _MSC_VER
    Value = 0;
#else
    __asm__ __volatile__ ("mrc p15, 0, %0, c6, c0, 0" : "=r"(Value) : : "cc");
#endif
    return Value;
}

FORCEINLINE
ARM_LOCKDOWN_REGISTER
KeArmLockdownRegisterGet(VOID)
{
    ARM_LOCKDOWN_REGISTER Value;
#ifdef _MSC_VER
    Value.AsUlong = 0;
#else
    __asm__ __volatile__ ("mrc p15, 0, %0, c10, c0, 0" : "=r"(Value.AsUlong) : : "cc");
#endif
    return Value;
}

FORCEINLINE
ARM_TTB_REGISTER
KeArmTranslationTableRegisterGet(VOID)
{
    ARM_TTB_REGISTER Value;
#ifdef _MSC_VER
    Value.AsUlong = 0;
#else
    __asm__ __volatile__ ("mrc p15, 0, %0, c2, c0, 0" : "=r"(Value.AsUlong) : : "cc");
#endif
    return Value;
}

FORCEINLINE
ARM_CACHE_REGISTER
KeArmCacheRegisterGet(VOID)
{
    ARM_CACHE_REGISTER Value;
#ifdef _MSC_VER
    Value.AsUlong = 0;
#else
    __asm__ __volatile__ ("mrc p15, 0, %0, c0, c0, 1" : "=r"(Value.AsUlong) : : "cc");
#endif
    return Value;
}

FORCEINLINE
ARM_STATUS_REGISTER
KeArmStatusRegisterGet(VOID)
{
    ARM_STATUS_REGISTER Value;
#ifdef _MSC_VER
    Value.AsUlong = _ReadStatusReg(0);
#else
    __asm__ __volatile__ ("mrs %0, cpsr" : "=r"(Value.AsUlong) : : "cc");
#endif
    return Value;
}

FORCEINLINE
VOID
KeArmControlRegisterSet(IN ARM_CONTROL_REGISTER ControlRegister)
{
#ifdef _MSC_VER
#else
    __asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0" : : "r"(ControlRegister.AsUlong) : "cc");
#endif
}

FORCEINLINE
VOID
KeArmTranslationTableRegisterSet(IN ARM_TTB_REGISTER Ttb)
{
#ifdef _MSC_VER
#else
    __asm__ __volatile__ ("mcr p15, 0, %0, c2, c0, 0" : : "r"(Ttb.AsUlong) : "cc");
#endif
}

FORCEINLINE
VOID
KeArmDomainRegisterSet(IN ARM_DOMAIN_REGISTER DomainRegister)
{
#ifdef _MSC_VER
#else
    __asm__ __volatile__ ("mcr p15, 0, %0, c3, c0, 0" : : "r"(DomainRegister.AsUlong) : "cc");
#endif
}

FORCEINLINE
VOID
KeArmLockdownRegisterSet(IN ARM_LOCKDOWN_REGISTER LockdownRegister)
{
#ifdef _MSC_VER
#else
    __asm__ __volatile__ ("mcr p15, 0, %0, c10, c0, 0" : : "r"(LockdownRegister.AsUlong) : "cc");
#endif
}

FORCEINLINE
VOID
KeArmFlushTlb(VOID)
{
#ifdef _MSC_VER
#else
    __asm__ __volatile__ ("mcr p15, 0, %0, c8, c7, 0" : : "r"(0) : "cc");
#endif
}

FORCEINLINE
VOID
KeArmInvalidateTlbEntry(IN PVOID Address)
{
#ifdef _MSC_VER
#else
    __asm__ __volatile__ ("mcr p15, 0, %0, c8, c7, 1" : : "r"(Address) : "cc");
#endif
}

FORCEINLINE
VOID
KeArmInvalidateAllCaches(VOID)
{
#ifdef _MSC_VER
#else
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c7, 0" : : "r"(0) : "cc");
#endif
}

FORCEINLINE
VOID
KeArmFlushIcache(VOID)
{
#ifdef _MSC_VER
#else
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 0" : : "r"(0) : "cc");
#endif
}

FORCEINLINE
VOID
KeArmWaitForInterrupt(VOID)
{
#ifdef _MSC_VER
#else
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c0, 4" : : "r"(0) : "cc");
#endif
}
