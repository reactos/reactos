/*++

Copyright (c) 2000-2001 Goran Devic                                                   
Modified (c) 2001 Klaus P. Gerlicher

Module Name:

    disassembler.h

Abstract:

    header for disassembler.c

Environment:

    LINUX 2.2.X
    Kernel mode only

Author: 

    Goran Devic                                                   

Revision History:

  17-Mar-2000:  Original                                        (Goran Devic)                                             
  05-Jan-2001:  Modified for pICE                               (Klaus P. Gerlicher)

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

BUGS:
    There is no segment override for _Ap (look the code)
--*/


/******************************************************************************
*                                                                             *
*   Include Files                                                             *
*                                                                             *
******************************************************************************/

#include "disassemblerdefines.h"    // Include its own defines

/******************************************************************************
*                                                                             *
*   Define opcode names as ASCIIZ strings                                     *
*                                                                             *
******************************************************************************/
char* sNames[] = {
"",                 /* 0X000 */
"AAA",              /* 0X001 */
"AAD",              /* 0X002 */
"AAM",              /* 0X003 */
"AAS",              /* 0X004 */
"ADC",              /* 0X005 */
"ADD",              /* 0X006 */
"AND",              /* 0X007 */
"ARPL",             /* 0X008 */
"BOUND",            /* 0X009 */
"BSF",              /* 0X00A */
"BSR",              /* 0X00B */
"BT",               /* 0X00C */
"BTC",              /* 0X00D */
"BTR",              /* 0X00E */
"BTS",              /* 0X00F */
"CALL",             /* 0X010 */
"CBW",              /* 0X011 */
"CWDE",             /* 0X012 */
"CLC",              /* 0X013 */
"CLD",              /* 0X014 */
"CLI",              /* 0X015 */
"CLTS",             /* 0X016 */
"CMC",              /* 0X017 */
"CMP",              /* 0X018 */
"CMPS",             /* 0X019 */
"CMPSB",            /* 0X01A */
"CMPSW",            /* 0X01B */
"CMPSD",            /* 0X01C */
"CWD",              /* 0X01D */
"CDQ",              /* 0X01E */
"DAA",              /* 0X01F */
"DAS",              /* 0X020 */
"DEC",              /* 0X021 */
"DIV",              /* 0X022 */
"ENTER",            /* 0X023 */
"HLT",              /* 0X024 */
"IDIV",             /* 0X025 */
"IMUL",             /* 0X026 */
"IN",               /* 0X027 */
"INC",              /* 0X028 */
"INS",              /* 0X029 */
"INSB",             /* 0X02A */
"INSW",             /* 0X02B */
"INSD",             /* 0X02C */
"INT",              /* 0X02D */
"INTO",             /* 0X02E */
"IRET",             /* 0X02F */
"IRETD",            /* 0X030 */
"JO",               /* 0X031 */
"JNO",              /* 0X032 */
"JB",               /* 0X033 */
"JNB",              /* 0X034 */
"JZ",               /* 0X035 */
"JNZ",              /* 0X036 */
"JBE",              /* 0X037 */
"JNBE",             /* 0X038 */
"JS",               /* 0X039 */
"JNS",              /* 0X03A */
"JP",               /* 0X03B */
"JNP",              /* 0X03C */
"JL",               /* 0X03D */
"JNL",              /* 0X03E */
"JLE",              /* 0X03F */
"JNLE",             /* 0X040 */
"JMP",              /* 0X041 */
"LAHF",             /* 0X042 */
"LAR",              /* 0X043 */
"LEA",              /* 0X044 */
"LEAVE",            /* 0X045 */
"LGDT",             /* 0X046 */
"LIDT",             /* 0X047 */
"LGS",              /* 0X048 */
"LSS",              /* 0X049 */
"LDS",              /* 0X04A */
"LES",              /* 0X04B */
"LFS",              /* 0X04C */
"LLDT",             /* 0X04D */
"LMSW",             /* 0X04E */
"LOCK",             /* 0X04F */
"LODS",             /* 0X050 */
"LODSB",            /* 0X051 */
"LODSW",            /* 0X052 */
"LODSD",            /* 0X053 */
"LOOP",             /* 0X054 */
"LOOPE",            /* 0X055 */
"LOOPZ",            /* 0X056 */
"LOOPNE",           /* 0X057 */
"LOOPNZ",           /* 0X058 */
"LSL",              /* 0X059 */
"LTR",              /* 0X05A */
"MOV",              /* 0X05B */
"MOVS",             /* 0X05C */
"MOVSB",            /* 0X05D */
"MOVSW",            /* 0X05E */
"MOVSD",            /* 0X05F */
"MOVSX",            /* 0X060 */
"MOVZX",            /* 0X061 */
"MUL",              /* 0X062 */
"NEG",              /* 0X063 */
"NOP",              /* 0X064 */
"NOT",              /* 0X065 */
"OR",               /* 0X066 */
"OUT",              /* 0X067 */
"OUTS",             /* 0X068 */
"OUTSB",            /* 0X069 */
"OUTSW",            /* 0X06A */
"OUTSD",            /* 0X06B */
"POP",              /* 0X06C */
"POPA",             /* 0X06D */
"POPAD",            /* 0X06E */
"POPF",             /* 0X06F */
"POPFD",            /* 0X070 */
"PUSH",             /* 0X071 */
"PUSHA",            /* 0X072 */
"PUSHAD",           /* 0X073 */
"PUSHF",            /* 0X074 */
"PUSHFD",           /* 0X075 */
"RCL",              /* 0X076 */
"RCR",              /* 0X077 */
"ROL",              /* 0X078 */
"ROR",              /* 0X079 */
"REP",              /* 0X07A */
"REPE",             /* 0X07B */
"REPZ",             /* 0X07C */
"REPNE",            /* 0X07D */
"REPNZ",            /* 0X07E */
"RET",              /* 0X07F */
"SAHF",             /* 0X080 */
"SAL",              /* 0X081 */
"SAR",              /* 0X082 */
"SHL",              /* 0X083 */
"SHR",              /* 0X084 */
"SBB",              /* 0X085 */
"SCAS",             /* 0X086 */
"SCASB",            /* 0X087 */
"SCASW",            /* 0X088 */
"SCASD",            /* 0X089 */
"SET",              /* 0X08A */
"SGDT",             /* 0X08B */
"SIDT",             /* 0X08C */
"SHLD",             /* 0X08D */
"SHRD",             /* 0X08E */
"SLDT",             /* 0X08F */
"SMSW",             /* 0X090 */
"STC",              /* 0X091 */
"STD",              /* 0X092 */
"STI",              /* 0X093 */
"STOS",             /* 0X094 */
"STOSB",            /* 0X095 */
"STOSW",            /* 0X096 */
"STOSD",            /* 0X097 */
"STR",              /* 0X098 */
"SUB",              /* 0X099 */
"TEST",             /* 0X09A */
"VERR",             /* 0X09B */
"VERW",             /* 0X09C */
"WAIT",             /* 0X09D */
"XCHG",             /* 0X09E */
"XLAT",             /* 0X09F */
"XLATB",            /* 0X0A0 */
"XOR",              /* 0X0A1 */
"JCXZ",             /* 0X0A2 */
"LOADALL",          /* 0X0A3 */
"INVD",             /* 0X0A4 */
"WBINVD",           /* 0X0A5 */
"SETO",             /* 0X0A6 */
"SETNO",            /* 0X0A7 */
"SETB",             /* 0X0A8 */
"SETNB",            /* 0X0A9 */
"SETZ",             /* 0X0AA */
"SETNZ",            /* 0X0AB */
"SETBE",            /* 0X0AC */
"SETNBE",           /* 0X0AD */
"SETS",             /* 0X0AE */
"SETNS",            /* 0X0AF */
"SETP",             /* 0X0B0 */
"SETNP",            /* 0X0B1 */
"SETL",             /* 0X0B2 */
"SETNL",            /* 0X0B3 */
"SETLE",            /* 0X0B4 */
"SETNLE",           /* 0X0B5 */
"WRMSR",            /* 0X0B6 */
"RDTSC",            /* 0X0B7 */
"RDMSR",            /* 0X0B8 */
"CPUID",            /* 0X0B9 */
"RSM",              /* 0X0BA */
"CMPXCHG",          /* 0X0BB */
"XADD",             /* 0X0BC */
"BSWAP",            /* 0X0BD */
"INVLPG",           /* 0X0BE */
"CMPXCHG8B",        /* 0X0BF */
"JMP FAR",          /* 0X0C0 */
"RETF",             /* 0X0C1 */
"RDPMC"             /* 0X0C2 */
};


char* sCoprocNames[] = {
"",                 /* 0X000 */
"F2XM1",            /* 0X001 */
"FABS",             /* 0X002 */
"FADD",             /* 0X003 */
"FADDP",            /* 0X004 */
"FBLD",             /* 0X005 */
"FBSTP",            /* 0X006 */
"FCHS",             /* 0X007 */
"FCLEX",            /* 0X008 */
"FCOM",             /* 0X009 */
"FCOMP",            /* 0X00A */
"FCOMPP",           /* 0X00B */
"FCOS",             /* 0X00C */
"FDECSTP",          /* 0X00D */
"FDIV",             /* 0X00E */
"FDIVP",            /* 0X00F */
"FDIVR",            /* 0X010 */
"FDIVRP",           /* 0X011 */
"FFREE",            /* 0X012 */
"FIADD",            /* 0X013 */
"FICOM",            /* 0X014 */
"FICOMP",           /* 0X015 */
"FIDIV",            /* 0X016 */
"FIDIVR",           /* 0X017 */
"FILD",             /* 0X018 */
"FIMUL",            /* 0X019 */
"FINCSTP",          /* 0X01A */
"FINIT",            /* 0X01B */
"FIST",             /* 0X01C */
"FISTP",            /* 0X01D */
"FISUB",            /* 0X01E */
"FISUBR",           /* 0X01F */
"FLD",              /* 0X020 */
"FLD1",             /* 0X021 */
"FLDCW",            /* 0X022 */
"FLDENV",           /* 0X023 */
"FLDL2E",           /* 0X024 */
"FLDL2T",           /* 0X025 */
"FLDLG2",           /* 0X026 */
"FLDLN2",           /* 0X027 */
"FLDPI",            /* 0X028 */
"FLDZ",             /* 0X029 */
"FMUL",             /* 0X02A */
"FMULP",            /* 0X02B */
"FNOP",             /* 0X02C */
"FPATAN",           /* 0X02D */
"FPREM",            /* 0X02E */
"FPREM1",           /* 0X02F */
"FPTAN",            /* 0X030 */
"FRNDINT",          /* 0X031 */
"FRSTOR",           /* 0X032 */
"FSAVE",            /* 0X033 */
"FSCALE",           /* 0X034 */
"FSIN",             /* 0X035 */
"FSINCOS",          /* 0X036 */
"FSQRT",            /* 0X037 */
"FST",              /* 0X038 */
"FSTCW",            /* 0X039 */
"FSTENV",           /* 0X03A */
"FSTP",             /* 0X03B */
"FSTSW",            /* 0X03C */
"FSUB",             /* 0X03D */
"FSUBP",            /* 0X03E */
"FSUBR",            /* 0X03F */
"FSUBRP",           /* 0X040 */
"FTST",             /* 0X041 */
"FUCOM",            /* 0X042 */
"FUCOMP",           /* 0X043 */
"FUCOMPP",          /* 0X044 */
"FXAM",             /* 0X045 */
"FXCH",             /* 0X046 */
"FXTRACT",          /* 0X047 */
"FYL2X",            /* 0X048 */
"FYL2XP1"           /* 0X049 */
};


/******************************************************************************
*
*   Table of the first byte of an instruction
*
******************************************************************************/
TOpcodeData Op1[ 256 ] = {
{ /* 00 */        _add       ,2  ,_Eb ,_Gb ,0   ,0  ,0, DIS_MODRM   },
{ /* 01 */        _add       ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 02 */        _add       ,2  ,_Gb ,_Eb ,0   ,0  ,0, DIS_MODRM   },
{ /* 03 */        _add       ,2  ,_Gv ,_Ev ,0   ,0  ,0, DIS_MODRM   },
{ /* 04 */        _add       ,2  ,_AL ,_Ib ,0   ,0  ,0, 0   },
{ /* 05 */        _add       ,2  ,_eAX,_Iv ,0   ,0  ,0, 0   },
{ /* 06 */        _push      ,1  ,_ES ,0   ,0   ,0  ,0, 0   },
{ /* 07 */        _pop       ,1  ,_ES ,0   ,0   ,0  ,0, 0   },
{ /* 08 */        _or        ,2  ,_Eb ,_Gb ,0   ,0  ,0, DIS_MODRM   },
{ /* 09 */        _or        ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 0A */        _or        ,2  ,_Gb ,_Eb ,0   ,0  ,0, DIS_MODRM   },
{ /* 0B */        _or        ,2  ,_Gv ,_Ev ,0   ,0  ,0, DIS_MODRM   },
{ /* 0C */        _or        ,2  ,_AL ,_Ib ,0   ,0  ,0, 0   },
{ /* 0D */        _or        ,2  ,_eAX,_Iv ,0   ,0  ,0, 0   },
{ /* 0E */        _push      ,1  ,_CS ,0   ,0   ,0  ,0, 0   },
{ /* 0F */        _2BESC     ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },

{ /* 10 */        _adc       ,2  ,_Eb ,_Gb ,0   ,0  ,0, DIS_MODRM   },
{ /* 11 */        _adc       ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 12 */        _adc       ,2  ,_Gb ,_Eb ,0   ,0  ,0, DIS_MODRM   },
{ /* 13 */        _adc       ,2  ,_Gv ,_Ev ,0   ,0  ,0, DIS_MODRM   },
{ /* 14 */        _adc       ,2  ,_AL ,_Ib ,0   ,0  ,0, 0   },
{ /* 15 */        _adc       ,2  ,_eAX,_Iv ,0   ,0  ,0, 0   },
{ /* 16 */        _push      ,1  ,_SS ,0   ,0   ,0  ,0, 0   },
{ /* 17 */        _pop       ,1  ,_SS ,0   ,0   ,0  ,0, 0   },
{ /* 18 */        _sbb       ,2  ,_Eb ,_Gb ,0   ,0  ,0, DIS_MODRM   },
{ /* 19 */        _sbb       ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 1A */        _sbb       ,2  ,_Gb ,_Eb ,0   ,0  ,0, DIS_MODRM   },
{ /* 1B */        _sbb       ,2  ,_Gv ,_Ev ,0   ,0  ,0, DIS_MODRM   },
{ /* 1C */        _sbb       ,2  ,_AL ,_Ib ,0   ,0  ,0, 0   },
{ /* 1D */        _sbb       ,2  ,_eAX,_Iv ,0   ,0  ,0, 0   },
{ /* 1E */        _push      ,1  ,_DS ,0   ,0   ,0  ,0, 0   },
{ /* 1F */        _pop       ,1  ,_DS ,0   ,0   ,0  ,0, 0   },

{ /* 20 */        _and       ,2  ,_Eb ,_Gb ,0   ,0  ,0, DIS_MODRM   },
{ /* 21 */        _and       ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 22 */        _and       ,2  ,_Gb ,_Eb ,0   ,0  ,0, DIS_MODRM   },
{ /* 23 */        _and       ,2  ,_Gv ,_Ev ,0   ,0  ,0, DIS_MODRM   },
{ /* 24 */        _and       ,2  ,_AL ,_Ib ,0   ,0  ,0, 0   },
{ /* 25 */        _and       ,2  ,_eAX,_Iv ,0   ,0  ,0, 0   },
{ /* 26 */        _S_ES      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 27 */        _daa       ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* 28 */        _sub       ,2  ,_Eb ,_Gb ,0   ,0  ,0, DIS_MODRM   },
{ /* 29 */        _sub       ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 2A */        _sub       ,2  ,_Gb ,_Eb ,0   ,0  ,0, DIS_MODRM   },
{ /* 2B */        _sub       ,2  ,_Gv ,_Ev ,0   ,0  ,0, DIS_MODRM   },
{ /* 2C */        _sub       ,2  ,_AL ,_Ib ,0   ,0  ,0, 0   },
{ /* 2D */        _sub       ,2  ,_eAX,_Iv ,0   ,0  ,0, 0   },
{ /* 2E */        _S_CS      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 2F */        _das       ,0  ,0   ,0   ,0   ,0  ,0, 0   },

{ /* 30 */        _xor       ,2  ,_Eb ,_Gb ,0   ,0  ,0, DIS_MODRM   },
{ /* 31 */        _xor       ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 32 */        _xor       ,2  ,_Gb ,_Eb ,0   ,0  ,0, DIS_MODRM   },
{ /* 33 */        _xor       ,2  ,_Gv ,_Ev ,0   ,0  ,0, DIS_MODRM   },
{ /* 34 */        _xor       ,2  ,_AL ,_Ib ,0   ,0  ,0, 0   },
{ /* 35 */        _xor       ,2  ,_eAX,_Iv ,0   ,0  ,0, 0   },
{ /* 36 */        _S_SS      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 37 */        _aaa       ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* 38 */        _cmp       ,2  ,_Eb ,_Gb ,0   ,0  ,0, DIS_MODRM   },
{ /* 39 */        _cmp       ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 3A */        _cmp       ,2  ,_Gb ,_Eb ,0   ,0  ,0, DIS_MODRM   },
{ /* 3B */        _cmp       ,2  ,_Gv ,_Ev ,0   ,0  ,0, DIS_MODRM   },
{ /* 3C */        _cmp       ,2  ,_AL ,_Ib ,0   ,0  ,0, 0   },
{ /* 3D */        _cmp       ,2  ,_eAX,_Iv ,0   ,0  ,0, 0   },
{ /* 3E */        _S_DS      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 3F */        _aas       ,0  ,0   ,0   ,0   ,0  ,0, 0   },

{ /* 40 */        _inc       ,1  ,_eAX,0   ,0   ,0  ,0, 0   },
{ /* 41 */        _inc       ,1  ,_eCX,0   ,0   ,0  ,0, 0   },
{ /* 42 */        _inc       ,1  ,_eDX,0   ,0   ,0  ,0, 0   },
{ /* 43 */        _inc       ,1  ,_eBX,0   ,0   ,0  ,0, 0   },
{ /* 44 */        _inc       ,1  ,_eSP,0   ,0   ,0  ,0, 0   },
{ /* 45 */        _inc       ,1  ,_eBP,0   ,0   ,0  ,0, 0   },
{ /* 46 */        _inc       ,1  ,_eSI,0   ,0   ,0  ,0, 0   },
{ /* 47 */        _inc       ,1  ,_eDI,0   ,0   ,0  ,0, 0   },
{ /* 48 */        _dec       ,1  ,_eAX,0   ,0   ,0  ,0, 0   },
{ /* 49 */        _dec       ,1  ,_eCX,0   ,0   ,0  ,0, 0   },
{ /* 4A */        _dec       ,1  ,_eDX,0   ,0   ,0  ,0, 0   },
{ /* 4B */        _dec       ,1  ,_eBX,0   ,0   ,0  ,0, 0   },
{ /* 4C */        _dec       ,1  ,_eSP,0   ,0   ,0  ,0, 0   },
{ /* 4D */        _dec       ,1  ,_eBP,0   ,0   ,0  ,0, 0   },
{ /* 4E */        _dec       ,1  ,_eSI,0   ,0   ,0  ,0, 0   },
{ /* 4F */        _dec       ,1  ,_eDI,0   ,0   ,0  ,0, 0   },

{ /* 50 */        _push      ,1  ,_eAX,0   ,0   ,0  ,0, 0   },
{ /* 51 */        _push      ,1  ,_eCX,0   ,0   ,0  ,0, 0   },
{ /* 52 */        _push      ,1  ,_eDX,0   ,0   ,0  ,0, 0   },
{ /* 53 */        _push      ,1  ,_eBX,0   ,0   ,0  ,0, 0   },
{ /* 54 */        _push      ,1  ,_eSP,0   ,0   ,0  ,0, 0   },
{ /* 55 */        _push      ,1  ,_eBP,0   ,0   ,0  ,0, 0   },
{ /* 56 */        _push      ,1  ,_eSI,0   ,0   ,0  ,0, 0   },
{ /* 57 */        _push      ,1  ,_eDI,0   ,0   ,0  ,0, 0   },
{ /* 58 */        _pop       ,1  ,_eAX,0   ,0   ,0  ,0, 0   },
{ /* 59 */        _pop       ,1  ,_eCX,0   ,0   ,0  ,0, 0   },
{ /* 5A */        _pop       ,1  ,_eDX,0   ,0   ,0  ,0, 0   },
{ /* 5B */        _pop       ,1  ,_eBX,0   ,0   ,0  ,0, 0   },
{ /* 5C */        _pop       ,1  ,_eSP,0   ,0   ,0  ,0, 0   },
{ /* 5D */        _pop       ,1  ,_eBP,0   ,0   ,0  ,0, 0   },
{ /* 5E */        _pop       ,1  ,_eSI,0   ,0   ,0  ,0, 0   },
{ /* 5F */        _pop       ,1  ,_eDI,0   ,0   ,0  ,0, 0   },

{ /* 60 */        _pusha     ,0  ,0   ,0   ,0   ,0  ,0, DIS_NAME_FLAG   },
{ /* 61 */        _popa      ,0  ,0   ,0   ,0   ,0  ,0, DIS_NAME_FLAG   },
{ /* 62 */        _bound     ,2  ,_Gv ,_Ma ,0   ,0  ,0, DIS_MODRM   },
{ /* 63 */        _arpl      ,2  ,_Ew ,_Rw ,0   ,0  ,0, DIS_MODRM   },
{ /* 64 */        _S_FS      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 65 */        _S_GS      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 66 */        _OPSIZ     ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 67 */        _ADSIZ     ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 68 */        _push      ,1  ,_Iv ,0   ,0   ,0  ,0, 0   },
{ /* 69 */        _imul      ,2  ,_Gv ,_Ev ,_Iv ,0  ,0, DIS_MODRM   },
{ /* 6A */        _push      ,1  ,_Ib ,0   ,0   ,0  ,0, 0   },
{ /* 6B */        _imul      ,3  ,_Gv ,_Ev ,_Ib ,0  ,0, DIS_MODRM   },
{ /* 6C */        _insb      ,2  ,_Yb ,_DX ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* 6D */        _insw      ,2  ,_Yv ,_DX ,0   ,0  ,0, SCAN_TERMINATING | DIS_NAME_FLAG    },
{ /* 6E */        _outsb     ,2  ,_DX ,_Xb ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* 6F */        _outsw     ,2  ,_DX ,_Xv ,0   ,0  ,0, SCAN_TERMINATING | DIS_NAME_FLAG    },

{ /* 70 */        _jo        ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 71 */        _jno       ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 72 */        _jb        ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 73 */        _jnb       ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 74 */        _jz        ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 75 */        _jnz       ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 76 */        _jbe       ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 77 */        _jnbe      ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 78 */        _js        ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 79 */        _jns       ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 7A */        _jp        ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 7B */        _jnp       ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 7C */        _jl        ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 7D */        _jnl       ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 7E */        _jle       ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 7F */        _jnle      ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },

{ /* 80 */        _GRP1a     ,2  ,_Eb ,_Ib ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* 81 */        _GRP1b     ,2  ,_Ev ,_Iv ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* 82 */        _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 83 */        _GRP1c     ,2  ,_Ev ,_Ib ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* 84 */        _test      ,2  ,_Eb ,_Gb ,0   ,0  ,0, DIS_MODRM   },
{ /* 85 */        _test      ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 86 */        _xchg      ,2  ,_Eb ,_Gb ,0   ,0  ,0, DIS_MODRM   },
{ /* 87 */        _xchg      ,2  ,_Ev ,_Gv ,0   ,0  ,INSTR_READ_WRITE | INSTR_WORD_DWORD, DIS_MODRM     },
{ /* 88 */        _mov       ,2  ,_Eb ,_Gb ,0   ,0  ,INSTR_WRITE | INSTR_BYTE, DIS_MODRM    },
{ /* 89 */        _mov       ,2  ,_Ev ,_Gv ,0   ,0  ,INSTR_WRITE | INSTR_WORD, DIS_MODRM    },
{ /* 8A */        _mov       ,2  ,_Gb ,_Eb ,0   ,0  ,INSTR_READ | INSTR_BYTE, DIS_MODRM     },
{ /* 8B */        _mov       ,2  ,_Gv ,_Ev ,0   ,0  ,0, DIS_MODRM   },
{ /* 8C */        _mov       ,2  ,_Ew ,_Sw ,0   ,0  ,0, DIS_MODRM   },
{ /* 8D */        _lea       ,2  ,_Gv ,_M  ,0   ,0  ,0, DIS_MODRM   },
{ /* 8E */        _mov       ,2  ,_Sw ,_Ew ,0   ,0  ,0, SCAN_TERM_PMODE | DIS_MODRM     },
{ /* 8F */        _pop       ,1  ,_Ev ,0   ,0   ,0  ,0, DIS_MODRM   },

{ /* 90 */        _nop       ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* 91 */        _xchg      ,2  ,_eCX,_eAX,0   ,0  ,0, 0   },
{ /* 92 */        _xchg      ,2  ,_eDX,_eAX,0   ,0  ,0, 0   },
{ /* 93 */        _xchg      ,2  ,_eBX,_eAX,0   ,0  ,0, 0   },
{ /* 94 */        _xchg      ,2  ,_eSP,_eAX,0   ,0  ,0, 0   },
{ /* 95 */        _xchg      ,2  ,_eBP,_eAX,0   ,0  ,0, 0   },
{ /* 96 */        _xchg      ,2  ,_eSI,_eAX,0   ,0  ,0, 0   },
{ /* 97 */        _xchg      ,2  ,_eDI,_eAX,0   ,0  ,0, 0   },
{ /* 98 */        _cbw       ,0  ,0   ,0   ,0   ,0  ,0, DIS_NAME_FLAG   },
{ /* 99 */        _cwd       ,0  ,0   ,0   ,0   ,0  ,0, DIS_NAME_FLAG   },
{ /* 9A */        _call      ,1  ,_Ap ,0   ,0   ,0  ,0, SCAN_SINGLE_STEP    },
{ /* 9B */        _wait      ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* 9C */        _pushf     ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING | DIS_NAME_FLAG    },
{ /* 9D */        _popf      ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING | DIS_NAME_FLAG    },
{ /* 9E */        _sahf      ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* 9F */        _lahf      ,0  ,0   ,0   ,0   ,0  ,0, 0   },

{ /* A0 */        _mov       ,2  ,_AL ,_O  ,0   ,0  ,0, 0   },
{ /* A1 */        _mov       ,2  ,_eAX,_O  ,0   ,0  ,INSTR_READ | INSTR_WORD_DWORD, 0   },
{ /* A2 */        _mov       ,2  ,_O  ,_AL ,0   ,0  ,0, 0   },
{ /* A3 */        _mov       ,2  ,_O  ,_eAX,0   ,0  ,INSTR_WRITE | INSTR_WORD_DWORD, 0  },
{ /* A4 */        _movsb     ,2  ,_Yb ,_Xb ,0   ,0  ,0, 0   },
{ /* A5 */        _movsw     ,2  ,_Yv ,_Xv ,0   ,0  ,0, DIS_NAME_FLAG   },
{ /* A6 */        _cmpsb     ,2  ,_Xb ,_Yb ,0   ,0  ,0, 0   },
{ /* A7 */        _cmpsw     ,2  ,_Xv ,_Yv ,0   ,0  ,0, DIS_NAME_FLAG   },
{ /* A8 */        _test      ,2  ,_AL ,_Ib ,0   ,0  ,0, 0   },
{ /* A9 */        _test      ,2  ,_eAX,_Iv ,0   ,0  ,0, 0   },
{ /* AA */        _stosb     ,2  ,_Yb ,_AL ,0   ,0  ,0, 0   },
{ /* AB */        _stosw     ,2  ,_Yb ,_eAX,0   ,0  ,0, DIS_NAME_FLAG   },
{ /* AC */        _lodsb     ,2  ,_AL ,_Xb ,0   ,0  ,INSTR_READ | INSTR_BYTE, 0     },
{ /* AD */        _lodsw     ,2  ,_eAX,_Xv ,0   ,0  ,INSTR_READ | INSTR_WORD_DWORD, DIS_NAME_FLAG   },
{ /* AE */        _scasb     ,2  ,_AL ,_Xb ,0   ,0  ,0, 0   },
{ /* AF */        _scasw     ,2  ,_eAX,_Xv ,0   ,0  ,0, DIS_NAME_FLAG   },

{ /* B0 */        _mov       ,2  ,_AL ,_Ib ,0   ,0  ,0, 0   },
{ /* B1 */        _mov       ,2  ,_CL ,_Ib ,0   ,0  ,0, 0   },
{ /* B2 */        _mov       ,2  ,_DL ,_Ib ,0   ,0  ,0, 0   },
{ /* B3 */        _mov       ,2  ,_BL ,_Ib ,0   ,0  ,0, 0   },
{ /* B4 */        _mov       ,2  ,_AH ,_Ib ,0   ,0  ,0, 0   },
{ /* B5 */        _mov       ,2  ,_CH ,_Ib ,0   ,0  ,0, 0   },
{ /* B6 */        _mov       ,2  ,_DH ,_Ib ,0   ,0  ,0, 0   },
{ /* B7 */        _mov       ,2  ,_BH ,_Ib ,0   ,0  ,0, 0   },
{ /* B8 */        _mov       ,2  ,_eAX,_Iv ,0   ,0  ,0, 0   },
{ /* B9 */        _mov       ,2  ,_eCX,_Iv ,0   ,0  ,0, 0   },
{ /* BA */        _mov       ,2  ,_eDX,_Iv ,0   ,0  ,0, 0   },
{ /* BB */        _mov       ,2  ,_eBX,_Iv ,0   ,0  ,0, 0   },
{ /* BC */        _mov       ,2  ,_eSP,_Iv ,0   ,0  ,0, 0   },
{ /* BD */        _mov       ,2  ,_eBP,_Iv ,0   ,0  ,0, 0   },
{ /* BE */        _mov       ,2  ,_eSI,_Iv ,0   ,0  ,0, 0   },
{ /* BF */        _mov       ,2  ,_eDI,_Iv ,0   ,0  ,0, 0   },

{ /* C0 */        _GRP2a     ,2  ,_Eb ,_Ib ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* C1 */        _GRP2b     ,2  ,_Ev ,_Ib ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* C2 */        _ret       ,1  ,_Iw ,0   ,0   ,0  ,0, SCAN_SINGLE_STEP    },
{ /* C3 */        _ret       ,0  ,0   ,0   ,0   ,0  ,0, SCAN_SINGLE_STEP    },
{ /* C4 */        _les       ,2  ,_Gv ,_Mp ,0   ,0  ,0, SCAN_TERM_PMODE | DIS_MODRM     },
{ /* C5 */        _lds       ,2  ,_Gv ,_Mp ,0   ,0  ,0, SCAN_TERM_PMODE | DIS_MODRM     },
{ /* C6 */        _mov       ,2  ,_Eb ,_Ib ,0   ,0  ,0, DIS_MODRM   },
{ /* C7 */        _mov       ,2  ,_Ev ,_Iv ,0   ,0  ,0, DIS_MODRM   },
{ /* C8 */        _enter     ,2  ,_Iw ,_Ib ,0   ,0  ,0, 0   },
{ /* C9 */        _leave     ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* CA */        _retf      ,1  ,_Iw ,0   ,0   ,0  ,0, SCAN_SINGLE_STEP    },
{ /* CB */        _retf      ,0  ,0   ,0   ,0   ,0  ,0, SCAN_SINGLE_STEP    },
{ /* CC */        _int       ,1  ,_3  ,0   ,0   ,0  ,0, 0   },
{ /* CD */        _int       ,1  ,_Ib ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* CE */        _into      ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* CF */        _iret      ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },

{ /* D0 */        _GRP2c     ,2  ,_Eb ,_1  ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* D1 */        _GRP2d     ,2  ,_Ev ,_1  ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* D2 */        _GRP2e     ,2  ,_Eb ,_CL ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* D3 */        _GRP2f     ,2  ,_Ev ,_CL ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* D4 */        _aam       ,1  ,_Ib ,0   ,0   ,0  ,0, 0   },
{ /* D5 */        _aad       ,1  ,_Ib ,0   ,0   ,0  ,0, 0   },
{ /* D6 */        _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* D7 */        _xlat      ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* D8 */        _EscD8     ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* D9 */        _EscD9     ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* DA */        _EscDA     ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* DB */        _EscDB     ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* DC */        _EscDC     ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* DD */        _EscDD     ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* DE */        _EscDE     ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* DF */        _EscDF     ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },

{ /* E0 */        _loopne    ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* E1 */        _loope     ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* E2 */        _loop      ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* E3 */        _jcxz      ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* E4 */        _in        ,2  ,_AL ,_Ib ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* E5 */        _in        ,2  ,_eAX,_Ib ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* E6 */        _out       ,2  ,_Ib ,_AL ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* E7 */        _out       ,2  ,_Ib ,_eAX,0   ,0  ,0, SCAN_TERMINATING    },
{ /* E8 */        _call      ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* E9 */        _jmp       ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_JUMP   },
{ /* EA */        _jmpf      ,1  ,_Ap ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* EB */        _jmp       ,1  ,_Jb ,0   ,0   ,0  ,0, SCAN_JUMP   },
{ /* EC */        _in        ,2  ,_AL ,_DX ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* ED */        _in        ,2  ,_eAX,_DX ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* EE */        _out       ,2  ,_DX ,_AL ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* EF */        _out       ,2  ,_DX ,_eAX,0   ,0  ,0, SCAN_TERMINATING    },

{ /* F0 */        _lock      ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* F1 */        _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* F2 */        _REPNE     ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* F3 */        _REP       ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* F4 */        _hlt       ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* F5 */        _cmc       ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* F6 */        _GRP3a     ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* F7 */        _GRP3b     ,1  ,_Ev ,0   ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* F8 */        _clc       ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* F9 */        _stc       ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* FA */        _cli       ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* FB */        _sti       ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* FC */        _cld       ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* FD */        _std       ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* FE */        _GRP4      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* FF */        _GRP5      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     }
};


/******************************************************************************
*
*   Table of the second byte of an instruction where the first byte was FF,
*   the 2-byte escape code
*
******************************************************************************/
TOpcodeData Op2[ 256 ] = {
{ /* 0F 00 */     _GRP6      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* 0F 01 */     _GRP7      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* 0F 02 */     _lar       ,2  ,_Gv ,_Ew ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 03 */     _lsl       ,2  ,_Gv ,_Ew ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 04 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 05 */     _loadall   ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* 0F 06 */     _clts      ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* 0F 07 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 08 */     _invd      ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* 0F 09 */     _wbinv     ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* 0F 0A */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 0B */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 0C */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 0D */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 0E */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 0F */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },

{ /* 0F 10 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 11 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 12 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 13 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 14 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 15 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 16 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 17 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 18 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 19 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 1A */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 1B */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 1C */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 1D */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 1E */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 1F */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },

{ /* 0F 20 */     _mov       ,2  ,_Rd ,_Cd ,0   ,0  ,0, SCAN_TERMINATING | DIS_MODRM    },
{ /* 0F 21 */     _mov       ,2  ,_Rd ,_Dd ,0   ,0  ,0, SCAN_TERMINATING | DIS_MODRM    },
{ /* 0F 22 */     _mov       ,2  ,_Cd ,_Rd ,0   ,0  ,0, SCAN_TERMINATING | DIS_MODRM    },
{ /* 0F 23 */     _mov       ,2  ,_Dd ,_Rd ,0   ,0  ,0, SCAN_TERMINATING | DIS_MODRM    },
{ /* 0F 24 */     _mov       ,2  ,_Rd ,_Td ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 25 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 26 */     _mov       ,2  ,_Td ,_Rd ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 27 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 28 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 29 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 2A */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 2B */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 2C */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 2D */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 2E */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 2F */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },

{ /* 0F 30 */     _wrmsr     ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* 0F 31 */     _rdtsc     ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* 0F 32 */     _rdmsr     ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* 0F 33 */     _rdpmc     ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* 0F 34 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 35 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 36 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 37 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 38 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 39 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 3A */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 3B */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 3C */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 3D */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 3E */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 3F */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },

{ /* 0F 40 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 41 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 42 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 43 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 44 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 45 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 46 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 47 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 48 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 49 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 4A */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 4B */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 4C */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 4D */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 4E */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 4F */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },

{ /* 0F 50 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 51 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 52 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 53 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 54 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 55 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 56 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 57 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 58 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 59 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 5A */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 5B */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 5C */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 5D */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 5E */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 5F */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },

{ /* 0F 60 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 61 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 62 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 63 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 64 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 65 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 66 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 67 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 68 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 69 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 6A */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 6B */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 6C */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 6D */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 6E */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 6F */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },

{ /* 0F 70 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 71 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 72 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 73 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 74 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 75 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 76 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 77 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 78 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 79 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 7A */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 7B */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 7C */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 7D */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 7E */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F 7F */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },

{ /* 0F 80 */     _jo        ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 81 */     _jno       ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 82 */     _jb        ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 83 */     _jnb       ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 84 */     _jz        ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 85 */     _jnz       ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 86 */     _jbe       ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 87 */     _jnbe      ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 88 */     _js        ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 89 */     _jns       ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 8A */     _jp        ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 8B */     _jnp       ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 8C */     _jl        ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 8D */     _jnl       ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 8E */     _jle       ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },
{ /* 0F 8F */     _jnle      ,1  ,_Jv ,0   ,0   ,0  ,0, SCAN_COND_JUMP  },

{ /* 0F 90 */     _seto      ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 91 */     _setno     ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 92 */     _setb      ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 93 */     _setnb     ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 94 */     _setz      ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 95 */     _setnz     ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 96 */     _setbe     ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 97 */     _setnbe    ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 98 */     _sets      ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 99 */     _setns     ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 9A */     _setp      ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 9B */     _setnp     ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 9C */     _setl      ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 9D */     _setnl     ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 9E */     _setle     ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F 9F */     _setnle    ,1  ,_Eb ,0   ,0   ,0  ,0, DIS_MODRM   },

{ /* 0F A0 */     _push      ,1  ,_FS ,0   ,0   ,0  ,0, 0   },
{ /* 0F A1 */     _pop       ,1  ,_FS ,0   ,0   ,0  ,0, 0   },
{ /* 0F A2 */     _cpuid     ,0  ,0   ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
{ /* 0F A3 */     _bt        ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F A4 */     _shld      ,3  ,_Ev ,_Gv ,_Ib ,0  ,0, DIS_MODRM   },
{ /* 0F A5 */     _shld      ,3  ,_Ev ,_Gv ,_CL ,0  ,0, DIS_MODRM   },
{ /* 0F A6 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F A7 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F A8 */     _push      ,1  ,_GS ,0   ,0   ,0  ,0, 0   },
{ /* 0F A9 */     _pop       ,1  ,_GS ,0   ,0   ,0  ,0, 0   },
{ /* 0F AA */     _rsm       ,0  ,0   ,0   ,0   ,0  ,0, 0   },
{ /* 0F AB */     _bts       ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F AC */     _shrd      ,3  ,_Ev ,_Gv ,_Ib ,0  ,0, DIS_MODRM   },
{ /* 0F AD */     _shrd      ,3  ,_Ev ,_Gv ,_CL ,0  ,0, DIS_MODRM   },
{ /* 0F AE */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F AF */     _imul      ,2  ,_Gv ,_Ev ,0   ,0  ,0, DIS_MODRM   },

{ /* 0F B0 */     _cmpx      ,2  ,_Eb ,_Gb ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F B1 */     _cmpx      ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F B2 */     _lss       ,2  ,_Gv ,_Mp ,0   ,0  ,0, SCAN_TERM_PMODE | DIS_MODRM     },
{ /* 0F B3 */     _btr       ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F B4 */     _lfs       ,2  ,_Gv ,_Mp ,0   ,0  ,0, SCAN_TERM_PMODE | DIS_MODRM     },
{ /* 0F B5 */     _lgs       ,2  ,_Gv ,_Mp ,0   ,0  ,0, SCAN_TERM_PMODE | DIS_MODRM     },
{ /* 0F B6 */     _movzx     ,2  ,_Gv ,_Eb ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F B7 */     _movzx     ,2  ,_Gv ,_Ew ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F B8 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F B9 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F BA */     _GRP8      ,2  ,_Ev ,_Ib ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* 0F BB */     _btc       ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F BC */     _bsf       ,2  ,_Gv ,_Ev ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F BD */     _bsr       ,2  ,_Gv ,_Ev ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F BE */     _movsx     ,2  ,_Gv ,_Eb ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F BF */     _movsx     ,2  ,_Gv ,_Ew ,0   ,0  ,0, DIS_MODRM   },

{ /* 0F C0 */     _xadd      ,2  ,_Eb ,_Gb ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F C1 */     _xadd      ,2  ,_Ev ,_Gv ,0   ,0  ,0, DIS_MODRM   },
{ /* 0F C2 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F C3 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F C4 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F C5 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F C6 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F C7 */     _GRP9      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL | DIS_MODRM     },
{ /* 0F C8 */     _bswap     ,1  ,_eAX,0   ,0   ,0  ,0, 0   },
{ /* 0F C9 */     _bswap     ,1  ,_eCX,0   ,0   ,0  ,0, 0   },
{ /* 0F CA */     _bswap     ,1  ,_eDX,0   ,0   ,0  ,0, 0   },
{ /* 0F CB */     _bswap     ,1  ,_eBX,0   ,0   ,0  ,0, 0   },
{ /* 0F CC */     _bswap     ,1  ,_eSP,0   ,0   ,0  ,0, 0   },
{ /* 0F CD */     _bswap     ,1  ,_eBP,0   ,0   ,0  ,0, 0   },
{ /* 0F CE */     _bswap     ,1  ,_eSI,0   ,0   ,0  ,0, 0   },
{ /* 0F CF */     _bswap     ,1  ,_eDI,0   ,0   ,0  ,0, 0   },

{ /* 0F D0 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F D1 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F D2 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F D3 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F D4 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F D5 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F D6 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F D7 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F D8 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F D9 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F DA */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F DB */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F DC */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F DD */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F DE */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F DF */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },

{ /* 0F E0 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F E1 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F E2 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F E3 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F E4 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F E5 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F E6 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F E7 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F E8 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F E9 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F EA */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F EB */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F EC */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F ED */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F EE */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F EF */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },

{ /* 0F F0 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F F1 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F F2 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F F3 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F F4 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F F5 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F F6 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F F7 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F F8 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F F9 */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F FA */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F FB */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F FC */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F FD */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F FE */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
{ /* 0F FF */     _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     }
};


/******************************************************************************
*
*   Table for Groups codes; groups 1 - 9
*
*   (These records have DIS_MODRM implied)
*
******************************************************************************/
TOpcodeData Groups[ 17 ][ 8 ] = {
{{ /* Group 1a */  _add       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 001 */       _or        ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 010 */       _adc       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 011 */       _sbb       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 100 */       _and       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 101 */       _sub       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 110 */       _xor       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 111 */       _cmp       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   }},

{{ /* Group 1b */  _add       ,2  ,_Ev ,_Iv ,0   ,0  ,0, 0   },
 { /* 001 */       _or        ,2  ,_Ev ,_Iv ,0   ,0  ,0, 0   },
 { /* 010 */       _adc       ,2  ,_Ev ,_Iv ,0   ,0  ,0, 0   },
 { /* 011 */       _sbb       ,2  ,_Ev ,_Iv ,0   ,0  ,0, 0   },
 { /* 100 */       _and       ,2  ,_Ev ,_Iv ,0   ,0  ,0, 0   },
 { /* 101 */       _sub       ,2  ,_Ev ,_Iv ,0   ,0  ,0, 0   },
 { /* 110 */       _xor       ,2  ,_Ev ,_Iv ,0   ,0  ,0, 0   },
 { /* 111 */       _cmp       ,2  ,_Ev ,_Iv ,0   ,0  ,0, 0   }},

{{ /* Group 1c */  _add       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 001 */       _or        ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 010 */       _adc       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 011 */       _sbb       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 100 */       _and       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 101 */       _sub       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 110 */       _xor       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 111 */       _cmp       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   }},

{{ /* Group 2a */  _rol       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 001 */       _ror       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 010 */       _rcl       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 011 */       _rcr       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 100 */       _sal       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 101 */       _shr       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 110 */       _shl       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 111 */       _sar       ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   }},

{{ /* Group 2b */  _rol       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 001 */       _ror       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 010 */       _rcl       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 011 */       _rcr       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 100 */       _sal       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 101 */       _shr       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 110 */       _shl       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 111 */       _sar       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   }},

{{ /* Group 2c */  _rol       ,2  ,_Eb ,_1  ,0   ,0  ,0, 0   },
 { /* 001 */       _ror       ,2  ,_Eb ,_1  ,0   ,0  ,0, 0   },
 { /* 010 */       _rcl       ,2  ,_Eb ,_1  ,0   ,0  ,0, 0   },
 { /* 011 */       _rcr       ,2  ,_Eb ,_1  ,0   ,0  ,0, 0   },
 { /* 100 */       _sal       ,2  ,_Eb ,_1  ,0   ,0  ,0, 0   },
 { /* 101 */       _shr       ,2  ,_Eb ,_1  ,0   ,0  ,0, 0   },
 { /* 110 */       _shl       ,2  ,_Eb ,_1  ,0   ,0  ,0, 0   },
 { /* 111 */       _sar       ,2  ,_Eb ,_1  ,0   ,0  ,0, 0   }},

{{ /* Group 2d */  _rol       ,2  ,_Ev ,_1  ,0   ,0  ,0, 0   },
 { /* 001 */       _ror       ,2  ,_Ev ,_1  ,0   ,0  ,0, 0   },
 { /* 010 */       _rcl       ,2  ,_Ev ,_1  ,0   ,0  ,0, 0   },
 { /* 011 */       _rcr       ,2  ,_Ev ,_1  ,0   ,0  ,0, 0   },
 { /* 100 */       _sal       ,2  ,_Ev ,_1  ,0   ,0  ,0, 0   },
 { /* 101 */       _shr       ,2  ,_Ev ,_1  ,0   ,0  ,0, 0   },
 { /* 110 */       _shl       ,2  ,_Ev ,_1  ,0   ,0  ,0, 0   },
 { /* 111 */       _sar       ,2  ,_Ev ,_1  ,0   ,0  ,0, 0   }},

{{ /* Group 2e */  _rol       ,2  ,_Eb ,_CL ,0   ,0  ,0, 0   },
 { /* 001 */       _ror       ,2  ,_Eb ,_CL ,0   ,0  ,0, 0   },
 { /* 010 */       _rcl       ,2  ,_Eb ,_CL ,0   ,0  ,0, 0   },
 { /* 011 */       _rcr       ,2  ,_Eb ,_CL ,0   ,0  ,0, 0   },
 { /* 100 */       _sal       ,2  ,_Eb ,_CL ,0   ,0  ,0, 0   },
 { /* 101 */       _shr       ,2  ,_Eb ,_CL ,0   ,0  ,0, 0   },
 { /* 110 */       _shl       ,2  ,_Eb ,_CL ,0   ,0  ,0, 0   },
 { /* 111 */       _sar       ,2  ,_Eb ,_CL ,0   ,0  ,0, 0   }},

{{ /* Group 2f */  _rol       ,2  ,_Ev ,_CL ,0   ,0  ,0, 0   },
 { /* 001 */       _ror       ,2  ,_Ev ,_CL ,0   ,0  ,0, 0   },
 { /* 010 */       _rcl       ,2  ,_Ev ,_CL ,0   ,0  ,0, 0   },
 { /* 011 */       _rcr       ,2  ,_Ev ,_CL ,0   ,0  ,0, 0   },
 { /* 100 */       _sal       ,2  ,_Ev ,_CL ,0   ,0  ,0, 0   },
 { /* 101 */       _shr       ,2  ,_Ev ,_CL ,0   ,0  ,0, 0   },
 { /* 110 */       _shl       ,2  ,_Ev ,_CL ,0   ,0  ,0, 0   },
 { /* 111 */       _sar       ,2  ,_Ev ,_CL ,0   ,0  ,0, 0   }},

{{ /* Group 3a */  _test      ,2  ,_Eb ,_Ib ,0   ,0  ,0, 0   },
 { /* 001 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 010 */       _not       ,1  ,_Eb ,0   ,0   ,0  ,0, 0   },
 { /* 011 */       _neg       ,1  ,_Eb ,0   ,0   ,0  ,0, 0   },
 { /* 100 */       _mul       ,1  ,_Eb ,0   ,0   ,0  ,0, 0   },
 { /* 101 */       _imul      ,1  ,_Eb ,0   ,0   ,0  ,0, 0   },
 { /* 110 */       _div       ,1  ,_Eb ,0   ,0   ,0  ,0, 0   },
 { /* 111 */       _idiv      ,1  ,_Eb ,0   ,0   ,0  ,0, 0   }},

{{ /* Group 3b */  _test      ,2  ,_Ev ,_Iv ,0   ,0  ,0, 0   },
 { /* 001 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 010 */       _not       ,1  ,_Ev ,0   ,0   ,0  ,0, 0   },
 { /* 011 */       _neg       ,1  ,_Ev ,0   ,0   ,0  ,0, 0   },
 { /* 100 */       _mul       ,1  ,_Ev ,0   ,0   ,0  ,0, 0   },
 { /* 101 */       _imul      ,1  ,_Ev ,0   ,0   ,0  ,0, 0   },
 { /* 110 */       _div       ,1  ,_Ev ,0   ,0   ,0  ,0, 0   },
 { /* 111 */       _idiv      ,1  ,_Ev ,0   ,0   ,0  ,0, 0   }},

{{ /* Group 4  */  _inc       ,1  ,_Eb ,0   ,0   ,0  ,0, 0   },
 { /* 001 */       _dec       ,1  ,_Eb ,0   ,0   ,0  ,0, 0   },
 { /* 010 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 011 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 100 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 101 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 110 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 111 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     }},

{{ /* Group 5  */  _inc       ,1  ,_Ev ,0   ,0   ,0  ,0, 0   },
 { /* 001 */       _dec       ,1  ,_Ev ,0   ,0   ,0  ,0, 0   },
 { /* 010 */       _call      ,1  ,_Ev ,0   ,0   ,0  ,0, SCAN_SINGLE_STEP    },
 { /* 011 */       _call      ,1  ,_Ep ,0   ,0   ,0  ,0, SCAN_SINGLE_STEP    },
 { /* 100 */       _jmp       ,1  ,_Ev ,0   ,0   ,0  ,0, SCAN_SINGLE_STEP    },
 { /* 101 */       _jmp       ,1  ,_Ep ,0   ,0   ,0  ,0, SCAN_SINGLE_STEP    },
 { /* 110 */       _push      ,1  ,_Ev ,0   ,0   ,0  ,0, 0   },
 { /* 111 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     }},

{{ /* Group 6  */  _sldt      ,1  ,_Ew ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
 { /* 001 */       _str       ,1  ,_Ew ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
 { /* 010 */       _lldt      ,1  ,_Ew ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
 { /* 011 */       _ltr       ,1  ,_Ew ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
 { /* 100 */       _verr      ,1  ,_Ew ,0   ,0   ,0  ,0, 0   },
 { /* 101 */       _verw      ,1  ,_Ew ,0   ,0   ,0  ,0, 0   },
 { /* 110 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 111 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     }},

{{ /* Group 7  */  _sgdt      ,1  ,_Ms ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
 { /* 001 */       _sidt      ,1  ,_Ms ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
 { /* 010 */       _lgdt      ,1  ,_Ms ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
 { /* 011 */       _lidt      ,1  ,_Ms ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
 { /* 100 */       _smsw      ,1  ,_Ew ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
 { /* 101 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 110 */       _lmsw      ,1  ,_Ew ,0   ,0   ,0  ,0, SCAN_TERMINATING    },
 { /* 111 */       _invpg     ,1  ,_M  ,0   ,0   ,0  ,0, SCAN_TERMINATING    }},

{{ /* Group 8  */  _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 001 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 010 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 011 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 100 */       _bt        ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 101 */       _bts       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 110 */       _btr       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   },
 { /* 111 */       _btc       ,2  ,_Ev ,_Ib ,0   ,0  ,0, 0   }},

{{ /* Group 9  */  _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 001 */       _cmpx8     ,1  ,_Mq ,0   ,0   ,0  ,0, 0   },
 { /* 010 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 011 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 100 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 101 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 110 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL     },
 { /* 111 */       _NDEF      ,0  ,0   ,0   ,0   ,0  ,0, DIS_SPECIAL  }}
};


/******************************************************************************
*
*   Coprocessor instructions have the prefix byte of D8-DF.
*   The Coproc1 table defines instructions that have the second byte in the
*   range 00-BF
*
******************************************************************************/
TOpcodeData Coproc1[ 8 ][ 8 ] = {
{{ /* D8 000 */    _fadd    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D8 001 */    _fmul    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D8 010 */    _fcom    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D8 011 */    _fcomp   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D8 100 */    _fsub    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D8 101 */    _fsubr   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D8 110 */    _fdiv    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D8 111 */    _fdivr   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         }},

{{ /* D9 000 */    _fld     ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D9 001 */    _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL        },
{  /* D9 010 */    _fst     ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D9 011 */    _fstp    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D9 100 */    _fldenv  ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D9 101 */    _fldcw   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D9 110 */    _fstenv  ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* D9 111 */    _fstcw   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         }},

{{ /* DA 000 */    _fiadd   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DA 001 */    _fimul   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DA 010 */    _ficom   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DA 011 */    _ficomp  ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DA 100 */    _fisub   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DA 101 */    _fisubr  ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DA 110 */    _fidiv   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DA 111 */    _fidivr  ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         }},

{{ /* DB 000 */    _fild    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DB 001 */    _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL        },
{  /* DB 010 */    _fist    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DB 011 */    _fistp   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DB 100 */    _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL        },
{  /* DB 101 */    _fld     ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DB 110 */    _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL        },
{  /* DB 111 */    _fstp    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         }},

{{ /* DC 000 */    _fadd    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DC 001 */    _fmul    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DC 010 */    _fcom    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DC 011 */    _fcomp   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DC 100 */    _fsub    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DC 101 */    _fsubr   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DC 110 */    _fdiv    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DC 111 */    _fdivr   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         }},

{{ /* DD 000 */    _fld     ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DD 001 */    _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL        },
{  /* DD 010 */    _fst     ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DD 011 */    _fstp    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DD 100 */    _frstor  ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DD 101 */    _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL        },
{  /* DD 110 */    _fsave   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DD 111 */    _fstsw   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         }},

{{ /* DE 000 */    _fiadd   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DE 001 */    _fimul   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DE 010 */    _ficom   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DE 011 */    _ficomp  ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DE 100 */    _fisub   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DE 101 */    _fisubr  ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DE 110 */    _fidiv   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DE 111 */    _fidivr  ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         }},

{{ /* DF 000 */    _fild    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DF 001 */    _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL        },
{  /* DF 010 */    _fist    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DF 011 */    _fistp   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DF 100 */    _fbld    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DF 101 */    _fild    ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DF 110 */    _fbstp   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         },
{  /* DF 111 */    _fistp   ,1  ,_M  ,0   ,0   ,0 ,0, DIS_COPROC         }}
};


/******************************************************************************
*
*   The Coproc2 table defines coprocessor instructions that have the second
*   byte in the range C0-FF
*
******************************************************************************/
TOpcodeData Coproc2[ 8 ][ 16 * 4 ] = {
{{ /* D8 C0 */     _fadd    ,2  ,_ST ,_ST0,0   ,0 ,0, DIS_COPROC       },
{  /* D8 C1 */     _fadd    ,2  ,_ST ,_ST1,0   ,0 ,0, DIS_COPROC       },
{  /* D8 C2 */     _fadd    ,2  ,_ST ,_ST2,0   ,0 ,0, DIS_COPROC       },
{  /* D8 C3 */     _fadd    ,2  ,_ST ,_ST3,0   ,0 ,0, DIS_COPROC       },
{  /* D8 C4 */     _fadd    ,2  ,_ST ,_ST4,0   ,0 ,0, DIS_COPROC       },
{  /* D8 C5 */     _fadd    ,2  ,_ST ,_ST5,0   ,0 ,0, DIS_COPROC       },
{  /* D8 C6 */     _fadd    ,2  ,_ST ,_ST6,0   ,0 ,0, DIS_COPROC       },
{  /* D8 C7 */     _fadd    ,2  ,_ST ,_ST7,0   ,0 ,0, DIS_COPROC       },

{  /* D8 C8 */     _fmul    ,2  ,_ST ,_ST0,0   ,0 ,0, DIS_COPROC       },
{  /* D8 C9 */     _fmul    ,2  ,_ST ,_ST1,0   ,0 ,0, DIS_COPROC       },
{  /* D8 CA */     _fmul    ,2  ,_ST ,_ST2,0   ,0 ,0, DIS_COPROC       },
{  /* D8 CB */     _fmul    ,2  ,_ST ,_ST3,0   ,0 ,0, DIS_COPROC       },
{  /* D8 CC */     _fmul    ,2  ,_ST ,_ST4,0   ,0 ,0, DIS_COPROC       },
{  /* D8 CD */     _fmul    ,2  ,_ST ,_ST5,0   ,0 ,0, DIS_COPROC       },
{  /* D8 CE */     _fmul    ,2  ,_ST ,_ST6,0   ,0 ,0, DIS_COPROC       },
{  /* D8 CF */     _fmul    ,2  ,_ST ,_ST7,0   ,0 ,0, DIS_COPROC       },

{  /* D8 D0 */     _fcom    ,2  ,_ST ,_ST0,0   ,0 ,0, DIS_COPROC       },
{  /* D8 D1 */     _fcom    ,2  ,_ST ,_ST1,0   ,0 ,0, DIS_COPROC       },
{  /* D8 D2 */     _fcom    ,2  ,_ST ,_ST2,0   ,0 ,0, DIS_COPROC       },
{  /* D8 D3 */     _fcom    ,2  ,_ST ,_ST3,0   ,0 ,0, DIS_COPROC       },
{  /* D8 D4 */     _fcom    ,2  ,_ST ,_ST4,0   ,0 ,0, DIS_COPROC       },
{  /* D8 D5 */     _fcom    ,2  ,_ST ,_ST5,0   ,0 ,0, DIS_COPROC       },
{  /* D8 D6 */     _fcom    ,2  ,_ST ,_ST6,0   ,0 ,0, DIS_COPROC       },
{  /* D8 D7 */     _fcom    ,2  ,_ST ,_ST7,0   ,0 ,0, DIS_COPROC       },

{  /* D8 D8 */     _fcomp   ,2  ,_ST ,_ST0,0   ,0 ,0, DIS_COPROC       },
{  /* D8 D9 */     _fcomp   ,2  ,_ST ,_ST1,0   ,0 ,0, DIS_COPROC       },
{  /* D8 DA */     _fcomp   ,2  ,_ST ,_ST2,0   ,0 ,0, DIS_COPROC       },
{  /* D8 DB */     _fcomp   ,2  ,_ST ,_ST3,0   ,0 ,0, DIS_COPROC       },
{  /* D8 DC */     _fcomp   ,2  ,_ST ,_ST4,0   ,0 ,0, DIS_COPROC       },
{  /* D8 DD */     _fcomp   ,2  ,_ST ,_ST5,0   ,0 ,0, DIS_COPROC       },
{  /* D8 DE */     _fcomp   ,2  ,_ST ,_ST6,0   ,0 ,0, DIS_COPROC       },
{  /* D8 DF */     _fcomp   ,2  ,_ST ,_ST7,0   ,0 ,0, DIS_COPROC       },

{  /* D8 E0 */     _fsub    ,2  ,_ST ,_ST0,0   ,0 ,0, DIS_COPROC       },
{  /* D8 E1 */     _fsub    ,2  ,_ST ,_ST1,0   ,0 ,0, DIS_COPROC       },
{  /* D8 E2 */     _fsub    ,2  ,_ST ,_ST2,0   ,0 ,0, DIS_COPROC       },
{  /* D8 E3 */     _fsub    ,2  ,_ST ,_ST3,0   ,0 ,0, DIS_COPROC       },
{  /* D8 E4 */     _fsub    ,2  ,_ST ,_ST4,0   ,0 ,0, DIS_COPROC       },
{  /* D8 E5 */     _fsub    ,2  ,_ST ,_ST5,0   ,0 ,0, DIS_COPROC       },
{  /* D8 E6 */     _fsub    ,2  ,_ST ,_ST6,0   ,0 ,0, DIS_COPROC       },
{  /* D8 E7 */     _fsub    ,2  ,_ST ,_ST7,0   ,0 ,0, DIS_COPROC       },

{  /* D8 E8 */     _fsubr   ,2  ,_ST ,_ST0,0   ,0 ,0, DIS_COPROC       },
{  /* D8 E9 */     _fsubr   ,2  ,_ST ,_ST1,0   ,0 ,0, DIS_COPROC       },
{  /* D8 EA */     _fsubr   ,2  ,_ST ,_ST2,0   ,0 ,0, DIS_COPROC       },
{  /* D8 EB */     _fsubr   ,2  ,_ST ,_ST3,0   ,0 ,0, DIS_COPROC       },
{  /* D8 EC */     _fsubr   ,2  ,_ST ,_ST4,0   ,0 ,0, DIS_COPROC       },
{  /* D8 ED */     _fsubr   ,2  ,_ST ,_ST5,0   ,0 ,0, DIS_COPROC       },
{  /* D8 EE */     _fsubr   ,2  ,_ST ,_ST6,0   ,0 ,0, DIS_COPROC       },
{  /* D8 EF */     _fsubr   ,2  ,_ST ,_ST7,0   ,0 ,0, DIS_COPROC       },

{  /* D8 F0 */     _fdiv    ,2  ,_ST ,_ST0,0   ,0 ,0, DIS_COPROC       },
{  /* D8 F1 */     _fdiv    ,2  ,_ST ,_ST1,0   ,0 ,0, DIS_COPROC       },
{  /* D8 F2 */     _fdiv    ,2  ,_ST ,_ST2,0   ,0 ,0, DIS_COPROC       },
{  /* D8 F3 */     _fdiv    ,2  ,_ST ,_ST3,0   ,0 ,0, DIS_COPROC       },
{  /* D8 F4 */     _fdiv    ,2  ,_ST ,_ST4,0   ,0 ,0, DIS_COPROC       },
{  /* D8 F5 */     _fdiv    ,2  ,_ST ,_ST5,0   ,0 ,0, DIS_COPROC       },
{  /* D8 F6 */     _fdiv    ,2  ,_ST ,_ST6,0   ,0 ,0, DIS_COPROC       },
{  /* D8 F7 */     _fdiv    ,2  ,_ST ,_ST7,0   ,0 ,0, DIS_COPROC       },

{  /* D8 F8 */     _fdivr   ,2  ,_ST ,_ST0,0   ,0 ,0, DIS_COPROC       },
{  /* D8 F9 */     _fdivr   ,2  ,_ST ,_ST1,0   ,0 ,0, DIS_COPROC       },
{  /* D8 FA */     _fdivr   ,2  ,_ST ,_ST2,0   ,0 ,0, DIS_COPROC       },
{  /* D8 FB */     _fdivr   ,2  ,_ST ,_ST3,0   ,0 ,0, DIS_COPROC       },
{  /* D8 FC */     _fdivr   ,2  ,_ST ,_ST4,0   ,0 ,0, DIS_COPROC       },
{  /* D8 FD */     _fdivr   ,2  ,_ST ,_ST5,0   ,0 ,0, DIS_COPROC       },
{  /* D8 FE */     _fdivr   ,2  ,_ST ,_ST6,0   ,0 ,0, DIS_COPROC       },
{  /* D8 FF */     _fdivr   ,2  ,_ST ,_ST7,0   ,0 ,0, DIS_COPROC       }},
/*----------------------------------------------------*/
{{ /* D9 C0 */     _fld     ,2  ,_ST ,_ST0,0   ,0 ,0, DIS_COPROC       },
{  /* D9 C1 */     _fld     ,2  ,_ST ,_ST1,0   ,0 ,0, DIS_COPROC       },
{  /* D9 C2 */     _fld     ,2  ,_ST ,_ST2,0   ,0 ,0, DIS_COPROC       },
{  /* D9 C3 */     _fld     ,2  ,_ST ,_ST3,0   ,0 ,0, DIS_COPROC       },
{  /* D9 C4 */     _fld     ,2  ,_ST ,_ST4,0   ,0 ,0, DIS_COPROC       },
{  /* D9 C5 */     _fld     ,2  ,_ST ,_ST5,0   ,0 ,0, DIS_COPROC       },
{  /* D9 C6 */     _fld     ,2  ,_ST ,_ST6,0   ,0 ,0, DIS_COPROC       },
{  /* D9 C7 */     _fld     ,2  ,_ST ,_ST7,0   ,0 ,0, DIS_COPROC       },

{  /* D9 C8 */     _fxch    ,2  ,_ST ,_ST0,0   ,0 ,0, DIS_COPROC       },
{  /* D9 C9 */     _fxch    ,2  ,_ST ,_ST1,0   ,0 ,0, DIS_COPROC       },
{  /* D9 CA */     _fxch    ,2  ,_ST ,_ST2,0   ,0 ,0, DIS_COPROC       },
{  /* D9 CB */     _fxch    ,2  ,_ST ,_ST3,0   ,0 ,0, DIS_COPROC       },
{  /* D9 CC */     _fxch    ,2  ,_ST ,_ST4,0   ,0 ,0, DIS_COPROC       },
{  /* D9 CD */     _fxch    ,2  ,_ST ,_ST5,0   ,0 ,0, DIS_COPROC       },
{  /* D9 CE */     _fxch    ,2  ,_ST ,_ST6,0   ,0 ,0, DIS_COPROC       },
{  /* D9 CF */     _fxch    ,2  ,_ST ,_ST7,0   ,0 ,0, DIS_COPROC       },

{  /* D9 D0 */     _fnop    ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 D1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 D2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 D3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 D4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 D5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 D6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 D7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* D9 D8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 D9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 DA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 DB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 DC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 DD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 DE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 DF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* D9 E0 */     _fchs    ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 E1 */     _fabs    ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 E2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 E3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 E4 */     _ftst    ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 E5 */     _fxam    ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 E6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* D9 E7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* D9 E8 */     _fld1    ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 E9 */     _fldl2t  ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 EA */     _fldl2e  ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 EB */     _fldpi   ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 EC */     _fldlg2  ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 ED */     _fldln2  ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 EE */     _fldz    ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 EF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* D9 F0 */     _f2xm1   ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 F1 */     _fyl2x   ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 F2 */     _fptan   ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 F3 */     _fpatan  ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 F4 */     _fxtract ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 F5 */     _fprem1  ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 F6 */     _fdecstp ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 F7 */     _fincstp ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },

{  /* D9 F8 */     _fprem   ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 F9 */     _fyl2xp1 ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 FA */     _fsqrt   ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 FB */     _fsincos ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 FC */     _frndint ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 FD */     _fscale  ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 FE */     _fsin    ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* D9 FF */     _fcos    ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       }},
/*----------------------------------------------------*/
{{ /* DA C0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA C1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA C2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA C3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA C4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA C5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA C6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA C7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DA C8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA C9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA CA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA CB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA CC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA CD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA CE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA CF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DA D0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA D1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA D2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA D3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA D4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA D5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA D6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA D7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DA D8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA D9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA DA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA DB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA DC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA DD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA DE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA DF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DA E0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA E1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA E2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA E3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA E4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA E5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA E6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA E7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DA E8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA E9 */     _fucompp ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DA EA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA EB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA EC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA ED */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA EE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA EF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DA F0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA F1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA F2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA F3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA F4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA F5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA F6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA F7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DA F8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA F9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA FA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA FB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA FC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA FD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA FE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DA FF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      }},
/*----------------------------------------------------*/
{{ /* DB C0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB C1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB C2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB C3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB C4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB C5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB C6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB C7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DB C8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB C9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB CA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB CB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB CC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB CD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB CE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB CF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DB D0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB D1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB D2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB D3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB D4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB D5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB D6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB D7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DB D8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB D9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB DA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB DB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB DC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB DD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB DE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB DF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DB E0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB E1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB E2 */     _fclex   ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DB E3 */     _finit   ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DB E4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB E5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB E6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB E7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DB E8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB E9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB EA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB EB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB EC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB ED */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB EE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB EF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DB F0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB F1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB F2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB F3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB F4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB F5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB F6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB F7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DB F8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB F9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB FA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB FB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB FC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB FD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB FE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DB FF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      }},
/*----------------------------------------------------*/
{{ /* DC C0 */     _fadd    ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC C1 */     _fadd    ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC C2 */     _fadd    ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC C3 */     _fadd    ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC C4 */     _fadd    ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC C5 */     _fadd    ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC C6 */     _fadd    ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC C7 */     _fadd    ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       },

{  /* DC C8 */     _fmul    ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC C9 */     _fmul    ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC CA */     _fmul    ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC CB */     _fmul    ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC CC */     _fmul    ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC CD */     _fmul    ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC CE */     _fmul    ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC CF */     _fmul    ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       },

{  /* DC D0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC D1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC D2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC D3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC D4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC D5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC D6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC D7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DC D8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC D9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC DA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC DB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC DC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC DD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC DE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DC DF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DC E0 */     _fsubr   ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC E1 */     _fsubr   ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC E2 */     _fsubr   ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC E3 */     _fsubr   ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC E4 */     _fsubr   ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC E5 */     _fsubr   ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC E6 */     _fsubr   ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC E7 */     _fsubr   ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       },

{  /* DC E8 */     _fsub    ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC E9 */     _fsub    ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC EA */     _fsub    ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC EB */     _fsub    ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC EC */     _fsub    ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC ED */     _fsub    ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC EE */     _fsub    ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC EF */     _fsub    ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       },

{  /* DC F0 */     _fdivr   ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC F1 */     _fdivr   ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC F2 */     _fdivr   ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC F3 */     _fdivr   ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC F4 */     _fdivr   ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC F5 */     _fdivr   ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC F6 */     _fdivr   ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC F7 */     _fdivr   ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       },

{  /* DC F8 */     _fdiv    ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC F9 */     _fdiv    ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC FA */     _fdiv    ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC FB */     _fdiv    ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC FC */     _fdiv    ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC FD */     _fdiv    ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC FE */     _fdiv    ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DC FF */     _fdiv    ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       }},
/*----------------------------------------------------*/
{{ /* DD C0 */     _ffree   ,1  ,_ST0,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD C1 */     _ffree   ,1  ,_ST1,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD C2 */     _ffree   ,1  ,_ST2,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD C3 */     _ffree   ,1  ,_ST3,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD C4 */     _ffree   ,1  ,_ST4,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD C5 */     _ffree   ,1  ,_ST5,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD C6 */     _ffree   ,1  ,_ST6,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD C7 */     _ffree   ,1  ,_ST7,0   ,0   ,0 ,0, DIS_COPROC       },

{  /* DD C8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD C9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD CA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD CB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD CC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD CD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD CE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD CF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DD D0 */     _fst     ,1  ,_ST0,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD D1 */     _fst     ,1  ,_ST1,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD D2 */     _fst     ,1  ,_ST2,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD D3 */     _fst     ,1  ,_ST3,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD D4 */     _fst     ,1  ,_ST4,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD D5 */     _fst     ,1  ,_ST5,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD D6 */     _fst     ,1  ,_ST6,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD D7 */     _fst     ,1  ,_ST7,0   ,0   ,0 ,0, DIS_COPROC       },

{  /* DD D8 */     _fstp    ,1  ,_ST0,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD D9 */     _fstp    ,1  ,_ST1,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD DA */     _fstp    ,1  ,_ST2,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD DB */     _fstp    ,1  ,_ST3,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD DC */     _fstp    ,1  ,_ST4,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD DD */     _fstp    ,1  ,_ST5,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD DE */     _fstp    ,1  ,_ST6,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD DF */     _fstp    ,1  ,_ST7,0   ,0   ,0 ,0, DIS_COPROC       },

{  /* DD E0 */     _fucom   ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DD E1 */     _fucom   ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DD E2 */     _fucom   ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DD E3 */     _fucom   ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DD E4 */     _fucom   ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DD E5 */     _fucom   ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DD E6 */     _fucom   ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DD E7 */     _fucom   ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       },

{  /* DD E8 */     _fucomp  ,1  ,_ST0,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD E9 */     _fucomp  ,1  ,_ST1,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD EA */     _fucomp  ,1  ,_ST2,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD EB */     _fucomp  ,1  ,_ST3,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD EC */     _fucomp  ,1  ,_ST4,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD ED */     _fucomp  ,1  ,_ST5,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD EE */     _fucomp  ,1  ,_ST6,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DD EF */     _fucomp  ,1  ,_ST7,0   ,0   ,0 ,0, DIS_COPROC       },

{  /* DD F0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD F1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD F2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD F3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD F4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD F5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD F6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD F7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DD F8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD F9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD FA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD FB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD FC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD FD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD FE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DD FF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      }},
/*----------------------------------------------------*/
{{ /* DE C0 */     _faddp   ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE C1 */     _faddp   ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE C2 */     _faddp   ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE C3 */     _faddp   ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE C4 */     _faddp   ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE C5 */     _faddp   ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE C6 */     _faddp   ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE C7 */     _faddp   ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       },

{  /* DE C8 */     _fmulp   ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE C9 */     _fmulp   ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE CA */     _fmulp   ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE CB */     _fmulp   ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE CC */     _fmulp   ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE CD */     _fmulp   ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE CE */     _fmulp   ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE CF */     _fmulp   ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       },

{  /* DE D0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE D1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE D2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE D3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE D4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE D5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE D6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE D7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DE D8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE D9 */     _fcompp  ,0  ,0   ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DE DA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE DB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE DC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE DD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE DE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DE DF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DE E0 */     _fsubrp  ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE E1 */     _fsubrp  ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE E2 */     _fsubrp  ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE E3 */     _fsubrp  ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE E4 */     _fsubrp  ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE E5 */     _fsubrp  ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE E6 */     _fsubrp  ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE E7 */     _fsubrp  ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       },

{  /* DE E8 */     _fsubp   ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE E9 */     _fsubp   ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE EA */     _fsubp   ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE EB */     _fsubp   ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE EC */     _fsubp   ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE ED */     _fsubp   ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE EE */     _fsubp   ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE EF */     _fsubp   ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       },

{  /* DE F0 */     _fdivrp  ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE F1 */     _fdivrp  ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE F2 */     _fdivrp  ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE F3 */     _fdivrp  ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE F4 */     _fdivrp  ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE F5 */     _fdivrp  ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE F6 */     _fdivrp  ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE F7 */     _fdivrp  ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       },

{  /* DE F8 */     _fdivp   ,2  ,_ST0,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE F9 */     _fdivp   ,2  ,_ST1,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE FA */     _fdivp   ,2  ,_ST2,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE FB */     _fdivp   ,2  ,_ST3,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE FC */     _fdivp   ,2  ,_ST4,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE FD */     _fdivp   ,2  ,_ST5,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE FE */     _fdivp   ,2  ,_ST6,_ST ,0   ,0 ,0, DIS_COPROC       },
{  /* DE FF */     _fdivp   ,2  ,_ST7,_ST ,0   ,0 ,0, DIS_COPROC       }},
/*----------------------------------------------------*/
{{ /* DF C0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF C1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF C2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF C3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF C4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF C5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF C6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF C7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DF C8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF C9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF CA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF CB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF CC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF CD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF CE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF CF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DF D0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF D1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF D2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF D3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF D4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF D5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF D6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF D7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DF D8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF D9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF DA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF DB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF DC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF DD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF DE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF DF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DF E0 */     _fstsw   ,1  ,_AX ,0   ,0   ,0 ,0, DIS_COPROC       },
{  /* DF E1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF E2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF E3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF E4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF E5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF E6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF E7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DF E8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF E9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF EA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF EB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF EC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF ED */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF EE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF EF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DF F0 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF F1 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF F2 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF F3 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF F4 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF F5 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF F6 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF F7 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },

{  /* DF F8 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF F9 */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF FA */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF FB */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF FC */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF FD */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF FE */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      },
{  /* DF FF */     _NDEF    ,0  ,0   ,0   ,0   ,0 ,0, DIS_SPECIAL      }}
};

/******************************************************************************
*
*   Generic messages used by the disassembler
*
******************************************************************************/
char *sBytePtr  = "BYTE PTR ";
char *sWordPtr  = "WORD PTR ";
char *sDwordPtr = "DWORD PTR ";
char *sFwordPtr = "FWORD PTR ";
char *sQwordPtr = "QWORD PTR ";

/******************************************************************************
*
*   Different register messages used by the disassembler
*
******************************************************************************/
char *sGenReg16_32[ 2 ][ 8 ] = {
{ "AX","CX","DX","BX","SP","BP","SI","DI" },
{ "EAX","ECX","EDX","EBX","ESP","EBP","ESI","EDI" }
};

char *sSeg[ 8 ] = {
"ES","CS","SS","DS","FS","GS","?","?"
};

char *sSegOverride[ 8 ] = {
"", "ES:","CS:","SS:","DS:","FS:","GS:","?:"
};

char *sSegOverrideDefaultES[ 8 ] = {
"ES:", "ES:","CS:","SS:","DS:","FS:","GS:","?:"
};

char *sSegOverrideDefaultDS[ 8 ] = {
"DS:", "ES:","CS:","SS:","DS:","FS:","GS:","?:"
};

char *sScale[ 4 ] = {
"",  "2*",  "4*",  "8*"
};

char *sAdr1[ 2 ][ 8 ] = {
{ "BX+SI","BX+DI","BP+SI","BP+DI","SI","DI","BP","BX" },
{ "EAX","ECX","EDX","EBX","?","EBP","ESI","EDI" }
};

char *sRegs1[ 2 ][ 2 ][ 8 ] = {
{{ "AL","CL","DL","BL","AH","CH","DH","BH" },
 { "AX","CX","DX","BX","SP","BP","SI","DI" } },
{{ "AL","CL","DL","BL","AH","CH","DH","BH" },
 { "EAX","ECX","EDX","EBX","ESP","EBP","ESI","EDI" } }
};

char *sRegs2[] = {
"DX",  "AL",  "AH",  "BL",  "BH",  "CL",  "CH",  "DL",  "DH",  "CS",  "DS",  "ES",  "SS",  "FS",  "GS"
};

char *sControl[ 8 ] = {
"CR0","CR1","CR2","CR3","CR4","?","?","?"
};

char *sDebug[ 8 ] = {
"DR0","DR1","DR2","DR3","DR4","DR5","DR6","DR7"
};

char *sTest[ 8 ] = {
"?","?","?","?","?","?","TR6","TR7"
};

char *sYptr[ 2 ] = {
"[DI]",  "[EDI]"
};

char *sXptr[ 2 ] = {
"[SI]",  "[ESI]"
};

char *sRep[ 4 ] = {
"", "REP ", "REPNZ ", "?"
};

char *sST[ 9 ] = {
"ST(0)","ST(1)","ST(2)","ST(3)","ST(4)","ST(5)","ST(6)","ST(7)","ST"
};


