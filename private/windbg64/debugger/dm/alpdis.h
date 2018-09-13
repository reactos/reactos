#ifndef _NTDIS_
#define _NTDIS_


// this stuff is all "#if 0"'ed in alphaops.h
//
// Bit manipulations for Alpha instructions
//

#define SHFT_OPCODE     26
#define SHFT_RA         21
#define SHFT_RB         16
#define SHFT_JMP_FNC    14
#define SHFT_OP_FNC     5
#define SHFT_RBV_TYPE   12
#define SHFT_LIT        13
#define SHFT_FP_FNC     5

#define WIDTH_OPCODE    6
#define WIDTH_REG       5
#define WIDTH_MEM_DISP  16
#define WIDTH_HINT      14
#define WIDTH_JMP_FNC   2
#define WIDTH_BR_DISP   21
#define WIDTH_OP_FNC    7
#define WIDTH_LIT       8
#define WIDTH_FP_FNC    11
#define WIDTH_PAL_FNC   26

#define BITS_OPCODE     ~(-1 << WIDTH_OPCODE)
#define BITS_REG        ~(-1 << WIDTH_REG)
#define BITS_MEM_DISP   ~(-1 << WIDTH_MEM_DISP)
#define BITS_HINT       ~(-1 << WIDTH_HINT)
#define BITS_JMP_FNC    ~(-1 << WIDTH_JMP_FNC)
#define BITS_BR_DISP    ~(-1 << WIDTH_BR_DISP)
#define BITS_OP_FNC     ~(-1 << WIDTH_OP_FNC)
#define BITS_LIT        ~(-1 << WIDTH_LIT)
#define BITS_FP_FNC     ~(-1 << WIDTH_FP_FNC)
#define BITS_PAL_FNC    ~(-1 << WIDTH_PAL_FNC)

#define OPCODE(a)       ((BITS_OPCODE & (a)) << SHFT_OPCODE)
#define REG_A(a)           ((BITS_REG & (a)) << SHFT_RA)
#define REG_B(a)           ((BITS_REG & (a)) << SHFT_RB)
#define REG_C(a)           (BITS_REG & (a))
#define MEM_DISP(a)     (BITS_MEM_DISP & (a))
#define MEM_FUNC(a)     MEM_DISP(a)
#define HINT(a)         (BITS_HINT & (a))
#define JMP_FNC(a)      ((BITS_JMP_FNC & (a)) << SHFT_JMP_FNC)
#define BR_DISP(a)      (BITS_BR_DISP & (a))
#define OP_FNC(a)       ((BITS_OP_FNC & (a)) << SHFT_OP_FNC)
#define RBV_TYPE(a)     ((1 & (a)) << SHFT_RBV_TYPE)
#define LIT(a)          ((BITS_LIT & (a)) << SHFT_LIT)
#define FP_FNC(a)       ((BITS_FP_FNC & (a)) << SHFT_FP_FNC)
#define PAL_FNC(a)      (BITS_PAL_FNC & (a))

#define MSK_OPCODE      OPCODE(BITS_OPCODE)
#define MSK_RA          GET_RA(BITS_REG)
#define MSK_RB          REG_B(BITS_REG)
#define MSK_RC          REG_C(BITS_REG)
#define MSK_MEM_DISP    DISP(BITS_MEM_DISP)
#define MSK_HINT        HINT(BITS_HINT)
#define MSK_JMP_FNC     JMP_FNC(BITS_JMP_FNC)
#define MSK_BR_DISP     BR_DISP(BITS_BR_DISP)
#define MSK_RBV_TYPE    RBV_TYPE(1)
#define MSK_LIT         LIT(BITS_LIT)
#define MSK_FP_FNC      FP_FNC(BITS_FP_FNC)
#define MSK_PAL_FNC     PAL_FNC(BITS_PAL_FNC)

#define EXTR_OPCODE(a)   (((a) & MSK_OPCODE) >> SHFT_OPCODE)
#define EXTR_RA(a)       (((a) & MSK_RA) >> SHFT_RA)
#define EXTR_RB(a)       (((a) & MSK_RB) >> SHFT_RA)
#define EXTR_RC(a)        ((a) & MSK_RC)
#define EXTR_MEM_DISP(a)  ((a) & MSK_MEM_DISP)
#define EXTR_HINT(a)      ((a) & MSK_HINT)
#define EXTR_JMP_FNC(a)  (((a) & MSK_JMP_FNC) >> SHFT_JMP_FNC)
#define EXTR_BR_DISP(a)   ((a) & MSK_BR_DISP)
#define EXTR_RBV_TYPE(a) (((a) & MSK_RBV_TYPE) >> SHFT_RBV_TYPE)
#define EXTR_LIT(a)      (((a) & MSK_LIT) >> SHFT_LIT)
#define EXTR_FP_FNC(a)   (((a) & MSK_FP_FNC) >> SHFT_FP_FNC)
#define EXTR_PAL_FNC(a)  ((a) & MSK_PAL_FNC)



//
// Bit manipulations for EV4 PAL mode instructions
//

#define SHFT_EV4_IBOX      5
#define SHFT_EV4_ABOX      6
#define SHFT_EV4_PALTEMP   7
#define SHFT_EV4_QWORD     12
#define SHFT_EV4_RWCHECK   13
#define SHFT_EV4_ALT       14
#define SHFT_EV4_PHYSICAL  15

#define WIDTH_EV4_INDEX    5
#define WIDTH_EV4_IBOX     1
#define WIDTH_EV4_ABOX     1
#define WIDTH_EV4_PALTEMP  1
#define WIDTH_EV4_DISP     12
#define WIDTH_EV4_QWORD    1
#define WIDTH_EV4_RWCHECK  1
#define WIDTH_EV4_ALT      1
#define WIDTH_EV4_PHYSICAL 1

#define BITS_EV4_INDEX     ~(-1 << WIDTH_EV4_INDEX)
#define BITS_EV4_IBOX      ~(-1 << WIDTH_EV4_IBOX)
#define BITS_EV4_ABOX      ~(-1 << WIDTH_EV4_ABOX)
#define BITS_EV4_PALTEMP   ~(-1 << WIDTH_EV4_PALTEMP)
#define BITS_EV4_DISP      ~(-1 << WIDTH_EV4_DISP)
#define BITS_EV4_QWORD     ~(-1 << WIDTH_EV4_QWORD)
#define BITS_EV4_RWCHECK   ~(-1 << WIDTH_EV4_RWCHECK)
#define BITS_EV4_ALT       ~(-1 << WIDTH_EV4_ALT)
#define BITS_EV4_PHYSICAL  ~(-1 << WIDTH_EV4_PHYSICAL)

#define EV4_INDEX(a)     (BITS_EV4_INDEX & (a))
#define EV4_IBOX(a)     ((BITS_EV4_IBOX & (a)) << SHFT_EV4_IBOX)
#define EV4_ABOX(a)     ((BITS_EV4_ABOX & (a)) << SHFT_EV4_ABOX)
#define EV4_PALTEMP(a)  ((BITS_EV4_PALTEMP & (a)) << SHFT_EV4_PALTEMP)
#define EV4_DISP(a)      (BITS_EV4_DISP & (a))
#define EV4_QWORD(a)    ((BITS_EV4_QWORD & (a)) << SHFT_EV4_QWORD)
#define EV4_RWCHECK(a)  ((BITS_EV4_RWCHECK & (a)) << SHFT_EV4_RWCHECK)
#define EV4_ALT(a)      ((BITS_EV4_ALT & (a)) << SHFT_EV4_ALT)
#define EV4_PHYSICAL(a) ((BITS_EV4_PHYSICAL & (a)) << SHFT_EV4_PHYSICAL)

#define MSK_EV4_INDEX       EV4_INDEX(BITS_EV4_INDEX)
#define MSK_EV4_IBOX        EV4_IBOX(BITS_EV4_IBOX)
#define MSK_EV4_ABOX        EV4_ABOX(BITS_EV4_ABOX)
#define MSK_EV4_PALTEMP     EV4_PALTEMP(BITS_EV4_PALTEMP)
#define MSK_EV4_PR          (MSK_EV4_INDEX | MSK_EV4_IBOX | MSK_EV4_ABOX | MSK_EV4_PALTEMP)
#define MSK_EV4_DISP        EV4_DISP(BITS_EV4_DISP)
#define MSK_EV4_QWORD       EV4_QWORD(BITS_EV4_QWORD)
#define MSK_EV4_RWCHECK     EV4_RWCHECK(BITS_EV4_RWCHECK)
#define MSK_EV4_ALT         EV4_ALT(BITS_EV4_ALT)
#define MSK_EV4_PHYSICAL    EV4_PHYSICAL(BITS_EV4_PHYSICAL)

#define EXTR_EV4_INDEX(a)        (MSK_EV4_INDEX & (a))
#define EXTR_EV4_IBOX(a)        ((MSK_EV4_IBOX & (a)) >> SHFT_EV4_IBOX)
#define EXTR_EV4_ABOX(a)        ((MSK_EV4_ABOX & (a)) >> SHFT_EV4_ABOX)
#define EXTR_EV4_PALTEMP(a)     ((MSK_EV4_PALTEMP & (a)) >> SHFT_EV4_PALTEMP)
#define EXTR_EV4_DISP(a)         (MSK_EV4_DISP & (a))
#define EXTR_EV4_QWORD(a)       ((MSK_EV4_QWORD & (a)) >> SHFT_EV4_QWORD)
#define EXTR_EV4_RWCHECK(a)     ((MSK_EV4_RWCHECK & (a)) >> SHFT_EV4_RWCHECK)
#define EXTR_EV4_ALT(a)         ((MSK_EV4_ALT & (a)) >> SHFT_EV4_ALT)
#define EXTR_EV4_PHYSICAL(a)    ((MSK_EV4_PHYSICAL & (a)) >> SHFT_EV4_PHYSICAL)

#define EV4_TB_TAG        (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(0))
#define EV4_ITB_PTE       (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(1))
#define EV4_ICCSR         (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(2))
#define EV4_ITM_PTE_TEMP  (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(3))
#define EV4_EXC_ADDR      (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(4))
#define EV4_SL_RCV        (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(5))
#define EV4_ITBZAP        (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(6))
#define EV4_ITBASM        (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(7))
#define EV4_ITBIS         (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(8))
#define EV4_PS            (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(9))
#define EV4_EXC_SUM       (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(10))
#define EV4_PAL_BASE      (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(11))
#define EV4_HIRR          (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(12))
#define EV4_SIRR          (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(13))
#define EV4_ASTRR         (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(14))
#define EV4_HIER          (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(16))
#define EV4_SIER          (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(17))
#define EV4_ASTER         (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(18))
#define EV4_SL_CLR        (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(19))
#define EV4_SL_XMIT       (EV4_PALTEMP(0) | EV4_ABOX(0) | EV4_IBOX(1) | EV4_INDEX(22))
#define EV4_DTB_CTL       (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(0))
#define EV4_DTB_PTE       (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(2))
#define EV4_DTB_PTE_TEMP  (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(3))
#define EV4_MMCSR         (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(4))
#define EV4_VA            (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(5))
#define EV4_DTBZAP        (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(6))
#define EV4_DTASM         (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(7))
#define EV4_DTBIS         (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(8))
#define EV4_BIU_ADDR      (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(9))
#define EV4_BIU_STAT      (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(10))
#define EV4_DC_ADDR       (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(11))
#define EV4_DC_STAT       (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(12))
#define EV4_FILL_ADDR     (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(13))
#define EV4_ABOX_CTL      (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(14))
#define EV4_ALT_MODE      (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(15))
#define EV4_CC            (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(16))
#define EV4_CC_CTL        (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(17))
#define EV4_BIU_CTL       (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(18))
#define EV4_FILL_SYNDROME (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(19))
#define EV4_BC_TAG        (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(20))
#define EV4_FLUSH_IC      (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(21))
#define EV4_FLUSH_IC_ASM  (EV4_PALTEMP(0) | EV4_ABOX(1) | EV4_IBOX(0) | EV4_INDEX(23))
#define EV4_PAL_TEMP(x)   (EV4_PALTEMP(1) | EV4_ABOX(0) | EV4_IBOX(0) | EV4_INDEX(x))

#endif
