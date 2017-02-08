

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _tag_ARMINTR_SHIFT_T
{
    _ARM_LSR = 0,
    _ARM_LSL = 1,
    _ARM_ASR = 2,
    _ARM_ROR = 3
} _ARMINTR_SHIFT_T;

typedef enum _tag_ARMINTR_CPS_OP
{
    _ARM_CPS_ENABLE_INTERRUPTS  = 1,
    _ARM_CPS_DISABLE_INTERRUPTS = 2,
    _ARM_CPS_CHANGE_MODE        = 4
} _ARMINTR_CPS_OP;

typedef enum _tag_ARMINTR_CPS_FLAG
{
    _ARM_CPS_INTERRUPT_FLAG_F  = 1,
    _ARM_CPS_INTERRUPT_FLAG_I  = 2,
    _ARM_CPS_INTERRUPT_FLAG_A  = 4
} _ARMINTR_CPS_FLAG;

typedef enum _tag_ARMINTR_BARRIER_TYPE
{
    _ARM_BARRIER_SY    = 0xF,
    _ARM_BARRIER_ST    = 0xE,
    _ARM_BARRIER_ISH   = 0xB,
    _ARM_BARRIER_ISHST = 0xA,
    _ARM_BARRIER_NSH   = 0x7,
    _ARM_BARRIER_NSHST = 0x6,
    _ARM_BARRIER_OSH   = 0x3,
    _ARM_BARRIER_OSHST = 0x2
} _ARMINTR_BARRIER_TYPE;

typedef enum _tag_ARMINTR_BANKED_REG
{
    _ARM_BANKED_R8_USR   = 0x0,
    _ARM_BANKED_R9_USR   = 0x1,
    _ARM_BANKED_R10_USR  = 0x2,
    _ARM_BANKED_R11_USR  = 0x3,
    _ARM_BANKED_R12_USR  = 0x4,
    _ARM_BANKED_R13_USR  = 0x5,
    _ARM_BANKED_SP_USR   = 0x5,
    _ARM_BANKED_R14_USR  = 0x6,
    _ARM_BANKED_LR_USR   = 0x6,
    _ARM_BANKED_R8_FIQ   = 0x8,
    _ARM_BANKED_R9_FIQ   = 0x9,
    _ARM_BANKED_R10_FIQ  = 0xA,
    _ARM_BANKED_R11_FIQ  = 0xB,
    _ARM_BANKED_R12_FIQ  = 0xC,
    _ARM_BANKED_R13_FIQ  = 0xD,
    _ARM_BANKED_SP_FIQ   = 0xD,
    _ARM_BANKED_R14_FIQ  = 0xE,
    _ARM_BANKED_LR_FIQ   = 0xE,
    _ARM_BANKED_R14_IRQ  = 0x10,
    _ARM_BANKED_LR_IRQ   = 0x10,
    _ARM_BANKED_R13_IRQ  = 0x11,
    _ARM_BANKED_SP_IRQ   = 0x11,
    _ARM_BANKED_R14_SVC  = 0x12,
    _ARM_BANKED_LR_SVC   = 0x12,
    _ARM_BANKED_R13_SVC  = 0x13,
    _ARM_BANKED_SP_SVC   = 0x13,
    _ARM_BANKED_R14_ABT  = 0x14,
    _ARM_BANKED_LR_ABT   = 0x14,
    _ARM_BANKED_R13_ABT  = 0x15,
    _ARM_BANKED_SP_ABT   = 0x15,
    _ARM_BANKED_R14_UND  = 0x16,
    _ARM_BANKED_LR_UND   = 0x16,
    _ARM_BANKED_R13_UND  = 0x17,
    _ARM_BANKED_SP_UND   = 0x17,
    _ARM_BANKED_R14_MON  = 0x1C,
    _ARM_BANKED_LR_MON   = 0x1C,
    _ARM_BANKED_R13_MON  = 0x1D,
    _ARM_BANKED_SP_MON   = 0x1D,
    _ARM_BANKED_ELR_HYP  = 0x1E,
    _ARM_BANKED_R13_HYP  = 0x1F,
    _ARM_BANKED_SP_HYP   = 0x1F,
    _ARM_BANKED_SPSR_FIQ = 0x2E,
    _ARM_BANKED_SPSR_IRQ = 0x30,
    _ARM_BANKED_SPSR_SVC = 0x32,
    _ARM_BANKED_SPSR_ABT = 0x34,
    _ARM_BANKED_SPSR_UND = 0x36,
    _ARM_BANKED_SPSR_MON = 0x3C,
    _ARM_BANKED_SPSR_HYP = 0x3E
} _ARMINTR_BANKED_REG;

void __dmb(unsigned int Type);
void __dsb(unsigned int Type);
void __isb(unsigned int Type);

#pragma intrinsic(__dmb)
#pragma intrinsic(__dsb)
#pragma intrinsic(__isb)


#if defined(__cplusplus)
} // extern "C"
#endif
