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

#define KeArchHaltProcessor KeArmHaltProcessor
#endif
