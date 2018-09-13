//
// The floating point registers
//

#define   szF0    "f0"
#define   szF1    "f1"
#define   szF2    "f2"
#define   szF3    "f3"
#define   szF4    "f4"
#define   szF5    "f5"
#define   szF6    "f6"
#define   szF7    "f7"
#define   szF8    "f8"
#define   szF9    "f9"
#define   szF10   "f10"
#define   szF11   "f11"
#define   szF12   "f12"
#define   szF13   "f13"
#define   szF14   "f14"
#define   szF15   "f15"
#define   szF16   "f16"
#define   szF17   "f17"
#define   szF18   "f18"
#define   szF19   "f19"
#define   szF20   "f20"
#define   szF21   "f21"
#define   szF22   "f22"
#define   szF23   "f23"
#define   szF24   "f24"
#define   szF25   "f25"
#define   szF26   "f26"
#define   szF27   "f27"
#define   szF28   "f28"
#define   szF29   "f29"
#define   szF30   "f30"
#define   szF31   "f31"

//
// The integer registers
//


#define   szR0    "v0"
#define   szR1    "t0"
#define   szR2    "t1"
#define   szR3    "t2"
#define   szR4    "t3"
#define   szR5    "t4"
#define   szR6    "t5"
#define   szR7    "t6"
#define   szR8    "t7"
#define   szR9    "s0"
#define   szR10   "s1"
#define   szR11   "s2"
#define   szR12   "s3"
#define   szR13   "s4"
#define   szR14   "s5"
#define   szR15   "fp"
#define   szR16   "a0"
#define   szR17   "a1"
#define   szR18   "a2"
#define   szR19   "a3"
#define   szR20   "a4"
#define   szR21   "a5"
#define   szR22   "t8"
#define   szR23   "t9"
#define   szR24   "t10"
#define   szR25   "t11"
#define   szR26   "ra"
#define   szR27   "t12"
#define   szR28   "at"
#define   szR29   "gp"
#define   szR30   "sp"
#define   szR31   "zero"

//
// ALPHA other accessible registers
//

#define   szFpcr   "fpcr"       // floating point control register
#define   szSoftFpcr   "softFpcr"   // floating point control register
#define   szFir    "fir"        // fetched/faulting instruction: nextPC
#define   szPsr    "psr"        // processor status register: see flags

//
// these flags are associated with the psr
// defined in ntalpha.h.
#define   szFlagMode    "mode"         // mode: 1? user : system
#define   szFlagIe      "ie"           // interrupt enable
#define   szFlagIrql    "irql"         // IRQL level: 3 bits
#define   szFlagInt5    "int5"
#define   szFlagInt4    "int4"
#define   szFlagInt3    "int3"
#define   szFlagInt2    "int2"
#define   szFlagInt1    "int1"
#define   szFlagInt0    "int0"

#define    szEaPReg     "$ea"
#define    szExpPReg    "$exp"
#define    szRaPReg     "$ra"
#define    szPPReg      "$p"

#define    szGPReg      "$gp"

#define    szU0Preg     "$u0"
#define    szU1Preg     "$u1"
#define    szU2Preg     "$u2"
#define    szU3Preg     "$u3"
#define    szU4Preg     "$u4"
#define    szU5Preg     "$u5"
#define    szU6Preg     "$u6"
#define    szU7Preg     "$u7"
#define    szU8Preg     "$u8"
#define    szU9Preg     "$u9"


