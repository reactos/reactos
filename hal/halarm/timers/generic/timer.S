
#include <ksarm.h>

    TEXTAREA

    NESTED_ENTRY pArmTranslationTableRegisterSet
    PROLOG_END pArmTranslationTableRegisterSet

    mcr p15, 0, r0, c2, c0, 0
    bx lr

    NESTED_END pArmTranslationTableRegisterSet

    NESTED_ENTRY ArmReadCntFrq
    PROLOG_END ArmReadCntFrq
    mrc    p15, 0, r0, c14, c0, 0    ; Read CNTFRQ
    bx     lr
    NESTED_END ArmReadCntFrq

    NESTED_ENTRY ArmWriteCntFrq
    PROLOG_END ArmWriteCntFrq
    mcr    p15, 0, r0, c14, c0, 0    ; Write to CNTFRQ
    bx     lr
    NESTED_END ArmWriteCntFrq

    NESTED_ENTRY ArmReadCntPct
    PROLOG_END ArmReadCntPct
    mrrc   p15, 0, r0, r1, c14       ; Read CNTPT (Physical counter register)
    bx     lr
    NESTED_END ArmReadCntPct

    NESTED_ENTRY ArmReadCntkCtl
    PROLOG_END ArmReadCntkCtl
    mrc    p15, 0, r0, c14, c1, 0    ; Read CNTK_CTL (Timer PL1 Control Register)
    bx     lr
    NESTED_END ArmReadCntkCtl

    NESTED_ENTRY ArmWriteCntkCtl
    PROLOG_END ArmWriteCntkCtl
    mcr    p15, 0, r0, c14, c1, 0    ; Write to CNTK_CTL (Timer PL1 Control Register)
    bx     lr
    NESTED_END ArmWriteCntkCtl

    NESTED_ENTRY ArmReadCntpTval
    PROLOG_END ArmReadCntpTval
    mrc    p15, 0, r0, c14, c2, 0    ; Read CNTP_TVAL (PL1 physical timer value register)
    bx     lr
    NESTED_END ArmReadCntpTval

    NESTED_ENTRY ArmWriteCntpTval
    PROLOG_END ArmWriteCntpTval
    mcr    p15, 0, r0, c14, c2, 0    ; Write to CNTP_TVAL (PL1 physical timer value register)
    bx     lr
    NESTED_END ArmWriteCntpTval

    NESTED_ENTRY ArmReadCntpCtl
    PROLOG_END ArmReadCntpCtl
    mrc    p15, 0, r0, c14, c2, 1    ; Read CNTP_CTL (PL1 Physical Timer Control Register)
    bx     lr
    NESTED_END ArmReadCntpCtl

    NESTED_ENTRY ArmWriteCntpCtl
    PROLOG_END ArmWriteCntpCtl
    mcr    p15, 0, r0, c14, c2, 1    ; Write to  CNTP_CTL (PL1 Physical Timer Control Register)
    bx     lr
    NESTED_END ArmWriteCntpCtl

    NESTED_ENTRY ArmReadCntvTval
    PROLOG_END ArmReadCntvTval
    mrc    p15, 0, r0, c14, c3, 0    ; Read CNTV_TVAL (Virtual Timer Value register)
    bx     lr
    NESTED_END ArmReadCntvTval

    NESTED_ENTRY ArmWriteCntvTval
    PROLOG_END ArmWriteCntvTval
    mcr    p15, 0, r0, c14, c3, 0    ; Write to CNTV_TVAL (Virtual Timer Value register)
    bx     lr
    NESTED_END ArmWriteCntvTval

    NESTED_ENTRY ArmReadCntvCtl
    PROLOG_END ArmReadCntvCtl
    mrc    p15, 0, r0, c14, c3, 1    ; Read CNTV_CTL (Virtual Timer Control Register)
    bx     lr
    NESTED_END ArmReadCntvCtl

    NESTED_ENTRY ArmWriteCntvCtl
    PROLOG_END ArmWriteCntvCtl
    mcr    p15, 0, r0, c14, c3, 1    ; Write to CNTV_CTL (Virtual Timer Control Register)
    bx     lr
    NESTED_END ArmWriteCntvCtl

    NESTED_ENTRY ArmReadCntvCt
    PROLOG_END ArmReadCntvCt
    mrrc   p15, 1, r0, r1, c14       ; Read CNTVCT  (Virtual Count Register)
    bx     lr
    NESTED_END ArmReadCntvCt

    NESTED_ENTRY ArmReadCntpCval
    PROLOG_END ArmReadCntpCval
    mrrc   p15, 2, r0, r1, c14       ; Read CNTP_CTVAL (Physical Timer Compare Value Register)
    bx     lr
    NESTED_END ArmReadCntpCval

    NESTED_ENTRY ArmWriteCntpCval
    PROLOG_END ArmWriteCntpCval
    mcrr   p15, 2, r0, r1, c14       ; Write to CNTP_CTVAL (Physical Timer Compare Value Register)
    bx     lr
    NESTED_END ArmWriteCntpCval

    NESTED_ENTRY ArmReadCntvCval
    PROLOG_END ArmReadCntvCval
    mrrc   p15, 3, r0, r1, c14       ; Read CNTV_CTVAL (Virtual Timer Compare Value Register)
    bx     lr
    NESTED_END ArmReadCntvCval

    NESTED_ENTRY ArmWriteCntvCval
    PROLOG_END ArmWriteCntvCval
    mcrr   p15, 3, r0, r1, c14       ; write to  CNTV_CTVAL (Virtual Timer Compare Value Register)
    bx     lr
    NESTED_END ArmWriteCntvCval

    NESTED_ENTRY ArmReadCntvOff
    PROLOG_END ArmReadCntvOff
    mrrc   p15, 4, r0, r1, c14       ; Read CNTVOFF (virtual Offset register)
    bx     lr
    NESTED_END ArmReadCntvOff

    NESTED_ENTRY ArmWriteCntvOff
    PROLOG_END ArmWriteCntvOff
    mcrr   p15, 4, r0, r1, c14       ; Write to CNTVOFF (Virtual Offset register)
    bx     lr
    NESTED_END ArmWriteCntvOff


 	END
