/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    optable.c

Abstract:

    Declaration for -
    Table of operations, their names and charactersitics
    Used by ntsd, windbg and acc's dissassembler

Author:

    Miche Baker-Harvey (mbh) 10-Jan-1993

Revision History:

    Nigel Haslock (haslock) 5-Sept-1995
        Added EV56 instructions for Byte and Word aligned memory access

--*/
#include "precomp.h"
#pragma hdrstop

#include "alphaops.h"
#include "alphaopt.h"
#include "alphaasm.h"

#define DEFINE_STRINGS
#include "alphastr.h"

// for strcmp
#include <string.h>

// for fVerboseOutput
BOOLEAN fVerboseOutput = FALSE;

#define NOFNCTBL (PVOID) 0
#define NOSIZE (ULONG)0


//
// These fields are used to find the beginning of the sections
// containing different "ENTRY_TYPE"s
//

POPTBLENTRY InvalidTab;
POPTBLENTRY NonTerminalTab;
POPTBLENTRY TerminalTab;
POPTBLENTRY FunctionTab;

ULONG        InvalidTabSize;
ULONG        NonTerminalTabSize;
ULONG        TerminalTabSize;
ULONG        FunctionTabSize;



//
// THE OPCODE TABLE ITSELF
//
// The opcode table "opTable" describes each opcode and function.
// There is an entry for each opcode, and for each function.
//
// The table is organized as follows:
//                invalid-ops,
//                non-terminal-ops,
//                terminal-ops,
//                functions,
//
//    This organization is NOT required:
//           no assumptions are made on it.
//
//    Searches based on opcode must search
//             INVALID, TERMINAL and NON_TERMINAL
//
//    Searches based on instruction name must search
//             TERMINAL and FUNCTION
//
//

OPTBLENTRY opTable[] = {


      //
      // First, the INVALID_ETYPE section.
      // (opcode searches begin here)
      //


{ "?Opc01", ParseUnknown, _01_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},
{ "?Opc02", ParseUnknown, _02_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},
{ "?Opc03", ParseUnknown, _03_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},
{ "?Opc04", ParseUnknown, _04_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},
{ "?Opc05", ParseUnknown, _05_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},
{ "?Opc06", ParseUnknown, _06_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},
{ "?Opc07", ParseUnknown, _07_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},
//{ "?Opc0A", ParseUnknown, _0A_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},
//{ "?Opc0C", ParseUnknown, _0C_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},
//{ "?Opc0D", ParseUnknown, _0D_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},
//{ "?Opc0E", ParseUnknown, _0E_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},
{ "?Opc14", ParseUnknown, _14_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},
//{ "?Opc1C", ParseUnknown, _1C_OP, NO_FUNC, ALPHA_UNKNOWN, INVALID_ETYPE},


//
//         This is what hasn't been done yet -
//  the EV4 stuff - there are no names for it
//  in the alphaops.h header file.  Should we
//  put them there?  Should they be elsewhere?
//  Do we want to assemble them?
//
// For the moment, just pretend they are invalid.  They never
// come up for all practical purposes, anyway.
//


{ "MFPR", ParseUnknown, MFPR_OP, NO_FUNC,
                                      ALPHA_EV4_PR, INVALID_ETYPE },
{ "MTPR", ParseUnknown, MTPR_OP, NO_FUNC,
                                      ALPHA_EV4_PR, INVALID_ETYPE },



      //
      // Secondly, the NON_TERMINAL_ETYPE section
      //



{  NOFNCTBL, NOSIZE, CALLPAL_OP, NO_FUNC,
                                   ALPHA_CALLPAL, NON_TERMINAL_ETYPE },
{  NOFNCTBL, NOSIZE, ARITH_OP, NO_FUNC,
                                   ALPHA_OPERATE, NON_TERMINAL_ETYPE },
{  NOFNCTBL, NOSIZE, BIT_OP, NO_FUNC,
                                   ALPHA_OPERATE, NON_TERMINAL_ETYPE },
{  NOFNCTBL, NOSIZE, BYTE_OP, NO_FUNC,
                                   ALPHA_OPERATE, NON_TERMINAL_ETYPE },
{  NOFNCTBL, NOSIZE, SEXT_OP, NO_FUNC,
                                   ALPHA_OPERATE, NON_TERMINAL_ETYPE },
{  NOFNCTBL, NOSIZE, MUL_OP, NO_FUNC,
                                   ALPHA_OPERATE, NON_TERMINAL_ETYPE },
{  NOFNCTBL, NOSIZE, MEMSPC_OP, NO_FUNC,
                                   ALPHA_MEMSPC,  NON_TERMINAL_ETYPE },
{  NOFNCTBL, NOSIZE, JMP_OP, NO_FUNC,
                                   ALPHA_JUMP,    NON_TERMINAL_ETYPE },
{  NOFNCTBL, NOSIZE, VAXFP_OP, NO_FUNC,
                                   ALPHA_FP_OPERATE, NON_TERMINAL_ETYPE },
{  NOFNCTBL, NOSIZE, IEEEFP_OP, NO_FUNC,
                                   ALPHA_FP_OPERATE, NON_TERMINAL_ETYPE },
{  NOFNCTBL, NOSIZE, FPOP_OP,    NO_FUNC,
                                   ALPHA_FP_OPERATE, NON_TERMINAL_ETYPE },



      //
      // Thirdly, the TERMINAL_ETYPE section
      // (everything from here on has an instruction name)
      //



{ szLda,   ParseIntMemory, LDA_OP,  NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szLdah,  ParseIntMemory, LDAH_OP, NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szLdl,   ParseIntMemory, LDL_OP,  NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szLdq,   ParseIntMemory, LDQ_OP,  NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szLdf,   ParseFltMemory, LDF_OP,  NO_FUNC, ALPHA_FP_MEMORY, TERMINAL_ETYPE },
{ szLdg,   ParseFltMemory, LDG_OP,  NO_FUNC, ALPHA_FP_MEMORY, TERMINAL_ETYPE },
{ szLds,   ParseFltMemory, LDS_OP,  NO_FUNC, ALPHA_FP_MEMORY, TERMINAL_ETYPE },
{ szLdt,   ParseFltMemory, LDT_OP,  NO_FUNC, ALPHA_FP_MEMORY, TERMINAL_ETYPE },
{ szLdq_u, ParseIntMemory, LDQ_U_OP,NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szLdl_l, ParseIntMemory, LDL_L_OP,NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szLdq_l, ParseIntMemory, LDQ_L_OP,NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szLdbu,  ParseIntMemory, LDBU_OP, NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szLdbu,  ParseIntMemory, LDWU_OP, NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },

{ szStl,   ParseIntMemory, STL_OP,  NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szStq,   ParseIntMemory, STQ_OP,  NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szStf,   ParseFltMemory, STF_OP,  NO_FUNC, ALPHA_FP_MEMORY, TERMINAL_ETYPE },
{ szStg,   ParseFltMemory, STG_OP,  NO_FUNC, ALPHA_FP_MEMORY, TERMINAL_ETYPE },
{ szSts,   ParseFltMemory, STS_OP,  NO_FUNC, ALPHA_FP_MEMORY, TERMINAL_ETYPE },
{ szStt,   ParseFltMemory, STT_OP,  NO_FUNC, ALPHA_FP_MEMORY, TERMINAL_ETYPE },
{ szStq_u, ParseIntMemory, STQ_U_OP,NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szStl_c, ParseIntMemory, STL_C_OP,NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szStq_c, ParseIntMemory, STQ_C_OP,NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szStb,   ParseIntMemory, STB_OP,  NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },
{ szStw,   ParseIntMemory, STW_OP,  NO_FUNC, ALPHA_MEMORY, TERMINAL_ETYPE },

{ szBeq,   ParseIntBranch, BEQ_OP,  NO_FUNC, ALPHA_BRANCH, TERMINAL_ETYPE },
{ szBne,   ParseIntBranch, BNE_OP,  NO_FUNC, ALPHA_BRANCH, TERMINAL_ETYPE },
{ szBlt,   ParseIntBranch, BLT_OP,  NO_FUNC, ALPHA_BRANCH, TERMINAL_ETYPE },
{ szBle,   ParseIntBranch, BLE_OP,  NO_FUNC, ALPHA_BRANCH, TERMINAL_ETYPE },
{ szBgt,   ParseIntBranch, BGT_OP,  NO_FUNC, ALPHA_BRANCH, TERMINAL_ETYPE },
{ szBge,   ParseIntBranch, BGE_OP,  NO_FUNC, ALPHA_BRANCH, TERMINAL_ETYPE },
{ szBlbc,  ParseIntBranch, BLBC_OP, NO_FUNC, ALPHA_BRANCH, TERMINAL_ETYPE },
{ szBlbs,  ParseIntBranch, BLBS_OP, NO_FUNC, ALPHA_BRANCH, TERMINAL_ETYPE },
{ szBr,    ParseIntBranch, BR_OP,   NO_FUNC, ALPHA_BRANCH, TERMINAL_ETYPE },
{ szBsr,   ParseIntBranch, BSR_OP,  NO_FUNC, ALPHA_BRANCH, TERMINAL_ETYPE },

{ szFbeq,  ParseFltBranch, FBEQ_OP, NO_FUNC, ALPHA_FP_BRANCH, TERMINAL_ETYPE },
{ szFbne,  ParseFltBranch, FBNE_OP, NO_FUNC, ALPHA_FP_BRANCH, TERMINAL_ETYPE },
{ szFblt,  ParseFltBranch, FBLT_OP, NO_FUNC, ALPHA_FP_BRANCH, TERMINAL_ETYPE },
{ szFble,  ParseFltBranch, FBLE_OP, NO_FUNC, ALPHA_FP_BRANCH, TERMINAL_ETYPE },
{ szFbgt,  ParseFltBranch, FBGT_OP, NO_FUNC, ALPHA_FP_BRANCH, TERMINAL_ETYPE },
{ szFbge,  ParseFltBranch, FBGE_OP, NO_FUNC, ALPHA_FP_BRANCH, TERMINAL_ETYPE },


{ "REI",   ParseUnknown, PAL1B_OP, NO_FUNC, ALPHA_EV4_REI, TERMINAL_ETYPE},
{ "HW_LD", ParseUnknown, PAL1E_OP, NO_FUNC, ALPHA_EV4_MEM, TERMINAL_ETYPE},
{ "HW_ST", ParseUnknown, PAL1F_OP, NO_FUNC, ALPHA_EV4_MEM, TERMINAL_ETYPE},


      //
      // Fourthly, (and finally) the FUNCTION_ETYPE section
      // (opcode searches needn't include this section)
      //

           //
           // The memory-special functions
           //

{ szMb,   ParseMemSpec, MEMSPC_OP, MB_FUNC,    ALPHA_MEMSPC, FUNCTION_ETYPE },
{ szWmb,  ParseMemSpec, MEMSPC_OP, WMB_FUNC,   ALPHA_MEMSPC, FUNCTION_ETYPE },
{ szMb2,  ParseMemSpec, MEMSPC_OP, MB2_FUNC,   ALPHA_MEMSPC, FUNCTION_ETYPE },
{ szMb3,  ParseMemSpec, MEMSPC_OP, MB3_FUNC,   ALPHA_MEMSPC, FUNCTION_ETYPE },
{ szFetch,ParseMemSpec, MEMSPC_OP, FETCH_FUNC, ALPHA_MEMSPC, FUNCTION_ETYPE },
{ szFetch_m,ParseMemSpec,MEMSPC_OP,FETCH_M_FUNC,ALPHA_MEMSPC,FUNCTION_ETYPE },
{ szRs,   ParseMemSpec, MEMSPC_OP, RS_FUNC,    ALPHA_MEMSPC, FUNCTION_ETYPE },
{ szTrapb,ParseMemSpec, MEMSPC_OP, TRAPB_FUNC, ALPHA_MEMSPC, FUNCTION_ETYPE },
{ szExcb, ParseMemSpec, MEMSPC_OP, EXCB_FUNC,  ALPHA_MEMSPC, FUNCTION_ETYPE },
{ szRpcc, ParseMemSpec, MEMSPC_OP, RPCC_FUNC,  ALPHA_MEMSPC, FUNCTION_ETYPE },
{ szRc,   ParseMemSpec, MEMSPC_OP, RC_FUNC,    ALPHA_MEMSPC, FUNCTION_ETYPE },

           //
           // The jump functions
           //

{ szJmp,  ParseJump,  JMP_OP, JMP_FUNC, ALPHA_JUMP, FUNCTION_ETYPE },
{ szJsr,  ParseJump,  JMP_OP, JSR_FUNC, ALPHA_JUMP, FUNCTION_ETYPE },
{ szRet,  ParseJump,  JMP_OP, RET_FUNC, ALPHA_JUMP, FUNCTION_ETYPE },
{ szJsr_co,  ParseJump,  JMP_OP, JSR_CO_FUNC, ALPHA_JUMP, FUNCTION_ETYPE },

           //
           // The arithmetic ops, which are ALPHA_OPERATE
           //

{ szAddl,   ParseIntOp, ARITH_OP, ADDL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szAddlv,  ParseIntOp, ARITH_OP, ADDLV_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szAddq,   ParseIntOp, ARITH_OP, ADDQ_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szAddqv,  ParseIntOp, ARITH_OP, ADDQV_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szSubl,   ParseIntOp, ARITH_OP, SUBL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szSublv,  ParseIntOp, ARITH_OP, SUBLV_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szSubq,   ParseIntOp, ARITH_OP, SUBQ_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szSubqv,  ParseIntOp, ARITH_OP, SUBQV_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },


{ szCmpeq,  ParseIntOp, ARITH_OP, CMPEQ_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szCmplt,  ParseIntOp, ARITH_OP, CMPLT_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szCmple,  ParseIntOp, ARITH_OP, CMPLE_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szCmpult, ParseIntOp, ARITH_OP, CMPULT_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szCmpule, ParseIntOp, ARITH_OP, CMPULE_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szCmpbge, ParseIntOp, ARITH_OP, CMPBGE_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },


{ szS4addl, ParseIntOp, ARITH_OP, S4ADDL_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szS4addq, ParseIntOp, ARITH_OP, S4ADDQ_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szS4subl, ParseIntOp, ARITH_OP, S4SUBL_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szS4subq, ParseIntOp, ARITH_OP, S4SUBQ_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szS8addl, ParseIntOp, ARITH_OP, S8ADDL_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szS8addq, ParseIntOp, ARITH_OP, S8ADDQ_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szS8subl, ParseIntOp, ARITH_OP, S8SUBL_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szS8subq, ParseIntOp, ARITH_OP, S8SUBQ_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },

           //
           // The bit ops, which are ALPHA_OPERATE
           //

{ szAnd,   ParseIntOp, BIT_OP, AND_FUNC,   ALPHA_OPERATE, FUNCTION_ETYPE },
{ szBic,   ParseIntOp, BIT_OP, BIC_FUNC,   ALPHA_OPERATE, FUNCTION_ETYPE },
{ szBis,   ParseIntOp, BIT_OP, BIS_FUNC,   ALPHA_OPERATE, FUNCTION_ETYPE },
{ szOrnot, ParseIntOp, BIT_OP, ORNOT_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szXor,   ParseIntOp, BIT_OP, XOR_FUNC,   ALPHA_OPERATE, FUNCTION_ETYPE },
{ szEqv,   ParseIntOp, BIT_OP, EQV_FUNC,   ALPHA_OPERATE, FUNCTION_ETYPE },

{ szCmoveq,  ParseIntOp, BIT_OP, CMOVEQ_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szCmovne,  ParseIntOp, BIT_OP, CMOVNE_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szCmovlbs, ParseIntOp, BIT_OP, CMOVLBS_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szCmovlt,  ParseIntOp, BIT_OP, CMOVLT_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szCmovge,  ParseIntOp, BIT_OP, CMOVGE_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szCmovlbc, ParseIntOp, BIT_OP, CMOVLBC_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szCmovle,  ParseIntOp, BIT_OP, CMOVLE_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szCmovgt,  ParseIntOp, BIT_OP, CMOVGT_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },

           //
           // The byte ops, which are ALPHA_OPERATE
           //

{ szSll,    ParseIntOp, BYTE_OP, SLL_FUNC,    ALPHA_OPERATE, FUNCTION_ETYPE },
{ szSra,    ParseIntOp, BYTE_OP, SRA_FUNC,    ALPHA_OPERATE, FUNCTION_ETYPE },
{ szSrl,    ParseIntOp, BYTE_OP, SRL_FUNC,    ALPHA_OPERATE, FUNCTION_ETYPE },
{ szExtbl,  ParseIntOp, BYTE_OP, EXTBL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szExtwl,  ParseIntOp, BYTE_OP, EXTWL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szExtll,  ParseIntOp, BYTE_OP, EXTLL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szExtql,  ParseIntOp, BYTE_OP, EXTQL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szExtwh,  ParseIntOp, BYTE_OP, EXTWH_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szExtlh,  ParseIntOp, BYTE_OP, EXTLH_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szExtqh,  ParseIntOp, BYTE_OP, EXTQH_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szInsbl,  ParseIntOp, BYTE_OP, INSBL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szInswl,  ParseIntOp, BYTE_OP, INSWL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szInsll,  ParseIntOp, BYTE_OP, INSLL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szInsql,  ParseIntOp, BYTE_OP, INSQL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szInswh,  ParseIntOp, BYTE_OP, INSWH_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szInslh,  ParseIntOp, BYTE_OP, INSLH_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szInsqh,  ParseIntOp, BYTE_OP, INSQH_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szMskbl,  ParseIntOp, BYTE_OP, MSKBL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szMskwl,  ParseIntOp, BYTE_OP, MSKWL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szMskll,  ParseIntOp, BYTE_OP, MSKLL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szMskql,  ParseIntOp, BYTE_OP, MSKQL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szMskwh,  ParseIntOp, BYTE_OP, MSKWH_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szMsklh,  ParseIntOp, BYTE_OP, MSKLH_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szMskqh,  ParseIntOp, BYTE_OP, MSKQH_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szZap,    ParseIntOp, BYTE_OP, ZAP_FUNC,    ALPHA_OPERATE, FUNCTION_ETYPE },
{ szZapnot, ParseIntOp, BYTE_OP, ZAPNOT_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },

           //
           // The multiply ops, which are ALPHA_OPERATE
           //

{ szMull,   ParseIntOp,  MUL_OP, MULL_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szMulqv,  ParseIntOp,  MUL_OP, MULQV_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szMullv,  ParseIntOp,  MUL_OP, MULLV_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szUmulh,  ParseIntOp,  MUL_OP, UMULH_FUNC, ALPHA_OPERATE, FUNCTION_ETYPE },
{ szMulq,   ParseIntOp,  MUL_OP, MULQ_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },

           //
           // The SEXT ops, which are ALPHA_OPERATE
           //

{ szSextb,  ParseIntOp, SEXT_OP, SEXTB_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },
{ szSextw,  ParseIntOp, SEXT_OP, SEXTW_FUNC,  ALPHA_OPERATE, FUNCTION_ETYPE },

           //
           // The call pal functions
           //


{ szBpt,       ParsePal, CALLPAL_OP,  BPT_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szCallsys,   ParsePal, CALLPAL_OP, CALLSYS_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szImb,       ParsePal, CALLPAL_OP, IMB_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRdteb,     ParsePal, CALLPAL_OP, RDTEB_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szGentrap,   ParsePal, CALLPAL_OP, GENTRAP_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szKbpt,      ParsePal, CALLPAL_OP, KBPT_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szCallKD,    ParsePal, CALLPAL_OP, CALLKD_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szHalt, ParsePal, CALLPAL_OP, HALT_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRestart, ParsePal, CALLPAL_OP, RESTART_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szDraina, ParsePal, CALLPAL_OP, DRAINA_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szInitpal, ParsePal, CALLPAL_OP, INITPAL_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szWrentry, ParsePal, CALLPAL_OP, WRENTRY_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szSwpirql, ParsePal, CALLPAL_OP, SWPIRQL_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRdirql, ParsePal, CALLPAL_OP, RDIRQL_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szDi, ParsePal, CALLPAL_OP, DI_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szEi, ParsePal, CALLPAL_OP, EI_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szSwppal, ParsePal, CALLPAL_OP, SWPPAL_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szSsir, ParsePal, CALLPAL_OP, SSIR_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szCsir, ParsePal, CALLPAL_OP, CSIR_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRfe, ParsePal, CALLPAL_OP, RFE_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRetsys, ParsePal, CALLPAL_OP, RETSYS_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szSwpctx, ParsePal, CALLPAL_OP, SWPCTX_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szSwpprocess, ParsePal, CALLPAL_OP, SWPPROCESS_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRdmces, ParsePal, CALLPAL_OP, RDMCES_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szWrmces, ParsePal, CALLPAL_OP, WRMCES_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szTbia, ParsePal, CALLPAL_OP, TBIA_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szTbis, ParsePal, CALLPAL_OP, TBIS_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szDtbis, ParsePal, CALLPAL_OP, DTBIS_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRdksp, ParsePal, CALLPAL_OP, RDKSP_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szSwpksp, ParsePal, CALLPAL_OP, SWPKSP_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRdpsr, ParsePal, CALLPAL_OP, RDPSR_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRdpcr, ParsePal, CALLPAL_OP, RDPCR_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRdthread, ParsePal, CALLPAL_OP, RDTHREAD_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRdcounters, ParsePal, CALLPAL_OP, RDCOUNTERS_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRdstate, ParsePal, CALLPAL_OP, RDSTATE_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szInitpcr, ParsePal, CALLPAL_OP, INITPCR_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szWrperfmon, ParsePal, CALLPAL_OP, WRPERFMON_FUNC,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szMt, ParsePal, CALLPAL_OP, MTPR_OP,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szMf, ParsePal, CALLPAL_OP, MFPR_OP,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szHwld, ParsePal, CALLPAL_OP, HWLD_OP,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szHwst, ParsePal, CALLPAL_OP, HWST_OP,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },
{ szRei, ParsePal, CALLPAL_OP, REI_OP,
                                  ALPHA_CALLPAL, FUNCTION_ETYPE },


           //
           // The VAX Floating point functions
           //


{ szAddf,   ParseUnknown, VAXFP_OP, ADDF_FUNC,
                                    ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCvtdg,  ParseUnknown, VAXFP_OP, CVTDG_FUNC,
                                    ALPHA_FP_CONVERT, FUNCTION_ETYPE },
{ szAddg,   ParseUnknown, VAXFP_OP, ADDG_FUNC,
                                    ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCmpgeq, ParseUnknown, VAXFP_OP, CMPGEQ_FUNC,
                                    ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCmpglt, ParseUnknown, VAXFP_OP, CMPGLT_FUNC,
                                    ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCmpgle, ParseUnknown, VAXFP_OP, CMPGLE_FUNC,
                                    ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCvtgf,  ParseUnknown, VAXFP_OP, CVTGF_FUNC,
                                    ALPHA_FP_CONVERT, FUNCTION_ETYPE },
{ szCvtgd,  ParseUnknown, VAXFP_OP, CVTGD_FUNC,
                                    ALPHA_FP_CONVERT, FUNCTION_ETYPE },
{ szCvtqf,  ParseUnknown, VAXFP_OP, CVTQF_FUNC,
                                    ALPHA_FP_CONVERT, FUNCTION_ETYPE },
{ szCvtqg,  ParseUnknown, VAXFP_OP, CVTQG_FUNC,
                                    ALPHA_FP_CONVERT, FUNCTION_ETYPE },
{ szDivf,   ParseUnknown, VAXFP_OP, DIVF_FUNC,
                                    ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szDivg,   ParseUnknown, VAXFP_OP, DIVG_FUNC,
                                    ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szMulf,   ParseUnknown, VAXFP_OP, MULF_FUNC,
                                    ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szMulg,   ParseUnknown, VAXFP_OP, MULG_FUNC,
                                    ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szSubf,   ParseUnknown, VAXFP_OP, SUBF_FUNC,
                                    ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szSubg,   ParseUnknown, VAXFP_OP, SUBG_FUNC,
                                    ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCvtgq,  ParseUnknown, VAXFP_OP, CVTGQ_FUNC,
                                    ALPHA_FP_CONVERT, FUNCTION_ETYPE },
           //
           // The IEEE Floating point functions
           //

{ szAdds,   ParseUnknown, IEEEFP_OP, ADDS_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szSubs,   ParseUnknown, IEEEFP_OP, SUBS_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szMuls,   ParseUnknown, IEEEFP_OP, MULS_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szDivs,   ParseUnknown, IEEEFP_OP, DIVS_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szAddt,   ParseUnknown, IEEEFP_OP, ADDT_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szSubt,   ParseUnknown, IEEEFP_OP, SUBT_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szMult,   ParseUnknown, IEEEFP_OP, MULT_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szDivt,   ParseUnknown, IEEEFP_OP, DIVT_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCmptun, ParseUnknown, IEEEFP_OP, CMPTUN_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCmpteq, ParseUnknown, IEEEFP_OP, CMPTEQ_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCmptlt, ParseUnknown, IEEEFP_OP, CMPTLT_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCmptle, ParseUnknown, IEEEFP_OP, CMPTLE_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCvtts,  ParseUnknown, IEEEFP_OP, CVTTS_FUNC,
                                     ALPHA_FP_CONVERT, FUNCTION_ETYPE },
{ szCvttq,  ParseUnknown, IEEEFP_OP, CVTTQ_FUNC,
                                     ALPHA_FP_CONVERT, FUNCTION_ETYPE },
{ szCvtqs,  ParseUnknown, IEEEFP_OP, CVTQS_FUNC,
                                     ALPHA_FP_CONVERT, FUNCTION_ETYPE },
{ szCvtqt,  ParseUnknown, IEEEFP_OP, CVTQT_FUNC,
                                     ALPHA_FP_CONVERT, FUNCTION_ETYPE },

           //
           // The Common Floating point functions
           //


{ szCvtlq,    ParseUnknown,  FPOP_OP, CVTLQ_FUNC,
                                     ALPHA_FP_CONVERT, FUNCTION_ETYPE },
{ szCpys,     ParseUnknown,  FPOP_OP, CPYS_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCpysn,    ParseUnknown,  FPOP_OP, CPYSN_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCpyse,    ParseUnknown,  FPOP_OP, CPYSE_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szMt_fpcr,  ParseUnknown,  FPOP_OP, MT_FPCR_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szMf_fpcr,  ParseUnknown,  FPOP_OP, MF_FPCR_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szFcmoveq,  ParseUnknown,  FPOP_OP, FCMOVEQ_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szFcmovne,  ParseUnknown,  FPOP_OP, FCMOVNE_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szFcmovlt,  ParseUnknown,  FPOP_OP, FCMOVLT_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szFcmovge,  ParseUnknown,  FPOP_OP, FCMOVGE_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szFcmovle,  ParseUnknown,  FPOP_OP, FCMOVLE_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szFcmovgt,  ParseUnknown,  FPOP_OP, FCMOVGT_FUNC,
                                     ALPHA_FP_OPERATE, FUNCTION_ETYPE },
{ szCvtql,    ParseUnknown,  FPOP_OP, CVTQL_FUNC,
                                     ALPHA_FP_CONVERT, FUNCTION_ETYPE },
{ szCvtqlv,   ParseUnknown,  FPOP_OP, CVTQLV_FUNC,
                                     ALPHA_FP_CONVERT, FUNCTION_ETYPE },
{ szCvtqlsv,  ParseUnknown,  FPOP_OP, CVTQLSV_FUNC,
                                     ALPHA_FP_CONVERT, FUNCTION_ETYPE },

};       // end of opTable


#define SEARCHNUM  sizeof(opTable) / sizeof(OPTBLENTRY)



//
// Here are the tables of Floating Point flags.
//

FPFLAGS ConvertFlags[] = {
    { NONE_FLAGS, NONE_FLAGS_STR },
    { C_FLAGS, C_FLAGS_STR },
    { V_FLAGS, V_FLAGS_STR },
    { VC_FLAGS, VC_FLAGS_STR },
    { SV_FLAGS, SV_FLAGS_STR },
    { SVC_FLAGS, SVC_FLAGS_STR },
    { SVI_FLAGS, SVI_FLAGS_STR },
    { SVIC_FLAGS, SVIC_FLAGS_STR },

    { D_FLAGS, D_FLAGS_STR },
    { VD_FLAGS, VD_FLAGS_STR },
    { SVD_FLAGS, SVD_FLAGS_STR },
    { SVID_FLAGS, SVID_FLAGS_STR },
    { M_FLAGS, M_FLAGS_STR },
    { VM_FLAGS, VM_FLAGS_STR },
    { SVM_FLAGS, SVM_FLAGS_STR },
    { SVIM_FLAGS, SVIM_FLAGS_STR },

    { S_FLAGS, S_FLAGS_STR },
    { SC_FLAGS, SC_FLAGS_STR },
    { FPFLAGS_NOT_AN_ENTRY, "" }
};

FPFLAGS FloatOpFlags[] = {
    { C_FLAGS, C_FLAGS_STR },
    { M_FLAGS, M_FLAGS_STR },
    { NONE_FLAGS, NONE_FLAGS_STR },
    { D_FLAGS, D_FLAGS_STR },
    { UC_FLAGS, UC_FLAGS_STR },
    { UM_FLAGS, UM_FLAGS_STR },
    { U_FLAGS, U_FLAGS_STR },
    { UD_FLAGS, UD_FLAGS_STR },
    { SC_FLAGS, SC_FLAGS_STR },
    { S_FLAGS, S_FLAGS_STR },
    { SUC_FLAGS, SUC_FLAGS_STR },
    { SUM_FLAGS, SUM_FLAGS_STR },
    { SU_FLAGS, SU_FLAGS_STR },
    { SUD_FLAGS, SUD_FLAGS_STR },
    { SUIC_FLAGS, SUIC_FLAGS_STR },
    { SUIM_FLAGS, SUIM_FLAGS_STR },
    { SUI_FLAGS, SUI_FLAGS_STR },
    { SUID_FLAGS, SUID_FLAGS_STR },
    { FPFLAGS_NOT_AN_ENTRY, "" }
};


/*** findNameEntry - find POPTBLENTRY based on name
*
*   Purpose:
*       This routine is used to determine if the opTable has
*       been initialized.  If a routine fails, it calls this
*       initialization check.  If the table is not initialized,
*       we initialize it, and try again.
*
*   Input:
*       VOID
*
*   Returns:
*       TRUE  - means table was already initialized; real failure
*       FALSE - just now initialized table; try again
*
*************************************************************************/
BOOLEAN
opTableInitialized()
{

    static BOOLEAN opTableInitialized = FALSE;

    if (opTableInitialized == TRUE) {
         return TRUE;
    }

    //
    // it wasn't initialized yet; do so now.
    //

    opTableInit();
    opTableInitialized = TRUE;
    return FALSE;
}


/*** findNameEntry - find POPTBLENTRY based on name
*
*   Purpose:
*       Search the opTable for a match with the token
*       pointed by *pszOp.  Must search through the
*       TERMINAL and the FUNCTION tables
*
*   Input:
*       *pszOp - string to search as mnemonic
*
*   Returns:
*       Pointer to entry in the opTable
*
*************************************************************************/

POPTBLENTRY
findStringEntry (PUCHAR pszOp)
{

    POPTBLENTRY pEntry;

    for (pEntry = TerminalTab;
         pEntry < &TerminalTab[TerminalTabSize];
         pEntry++) {

        if (!strcmp(pszOp, pEntry->pszAlphaName))
                return(pEntry);
    }

    for (pEntry = FunctionTab;
         pEntry < &FunctionTab[FunctionTabSize];
         pEntry++) {

        if (!strcmp(pszOp, pEntry->pszAlphaName))
                return(pEntry);
    }

    if (opTableInitialized() == FALSE) {
        return findStringEntry(pszOp);
    }
    return((POPTBLENTRY)-1);
}


/* findOpCodeEntry - find POPTBLENTRY based on opcode
*
*   Purpose:
*       Search the opTable for a match with the opcode
*       Must search through the
*       INVALID, TERMINAL and NON_TERMINAL tables
*
*   Input:
*       pOpEntry - pointer to NON_TERMINAL_ETYPE in opTable
*       function - the function value to be looked up
*
*   Output:
*       pointer to string mnemonic for the function
*
***********************************************************************/

POPTBLENTRY
findOpCodeEntry(ULONG opcode)
{
    POPTBLENTRY pEntry;

    for (pEntry = TerminalTab;
         pEntry < &TerminalTab[TerminalTabSize];
         pEntry++) {

        if (pEntry->opCode == opcode)
                return(pEntry);
    }

    for (pEntry = NonTerminalTab;
         pEntry < &NonTerminalTab[NonTerminalTabSize];
         pEntry++) {

        if (pEntry->opCode == opcode)
                return(pEntry);
    }

    for (pEntry = InvalidTab;
         pEntry < &InvalidTab[InvalidTabSize];
         pEntry++) {

        if (pEntry->opCode == opcode)
                return(pEntry);
    }

    if (opTableInitialized() == FALSE) {
        return findOpCodeEntry(opcode);
    }
    return((POPTBLENTRY)-1);
}


/*** findTypeFromOpcode - find the instruction type
*
*   Purpose:
*       This routine finds the type of the instruction (ALPHA_*)
*       from the opcode.  It returns a single value so that it
*       can be called from other dll's without passing storage.
*
*   Input:
*       the opcode number.
*
*   Output:
*       the type of the opcode (ALPHA_* in alphaops.h)
*
*   Errors:
*       returns ALPHA_UNKNOWN for an invalid opcode
*
*   Exceptions:
*       None.
*
*
*************************************************************************/

ULONG
findTypeFromOpcode(ULONG opcode)
{
    POPTBLENTRY pEntry;

    pEntry = findOpCodeEntry(opcode);

    if (pEntry != (POPTBLENTRY)(-1)) {
        return(pEntry->iType);
    } else {
        return(ALPHA_UNKNOWN);
    }
}


/*** findNonTerminalEntry - find pointer to set of functions
*
*   Purpose:
*       This routine finds the entry in the table which the is
*       nonterminal entry for an opcode.
*
*   Input:
*       The type of function that is interesting
*
*   Output:
*       Pointer to the nonterminal entry in opTable
*
*   Errors:
*       If the entry is not found, a message is printed, and the
*       routine exits.
*
*   Exceptions:
*       None.
*
*   Note:
*       This routine is called BEFORE NonTerminalTable is established!
*       (it's used to set up these tables, in fact).
*
*************************************************************************/

POPTBLENTRY
findNonTerminalEntry(ULONG opCode)
{
    ULONG index;

    for ( index = 0 ; index < SEARCHNUM; index++ ) {

        if ( ( opTable[index].eType == NON_TERMINAL_ETYPE ) &&
             ( opTable[index].opCode == opCode ) ) {

                 return(&opTable[index]);
        }
    }

    // Should return before here
    assert( 0 );
    return NULL;
}


/* findFuncName - get string name for a function
*
*   Purpose:
*       to get function name, given the function number, and a
*       pointer to the opTable entry for the NON_TERMINAL_ETYPE
*       opcode associated with the function
*
*   Input:
*       pOpEntry - pointer to NON_TERMINAL_ETYPE in opTable
*       function - the function value to be looked up
*
*   Output:
*       pointer to string mnemonic for the function
*
***********************************************************************/

char *
findFuncName(POPTBLENTRY pEntry, ULONG function)
{
    int cIndex;
    POPTBLENTRY pFncEntry;

    pFncEntry = pEntry->funcTable;
    cIndex = (int)pEntry->funcTableSize;

    //
    // make sure that this entry pts to a function table
    //

    if (pEntry->eType != NON_TERMINAL_ETYPE) {
        return("???");
    }

    while(cIndex-- > 0) {
        if (function == pFncEntry->funcCode)
            return(pFncEntry->pszAlphaName);
        pFncEntry++;
    };

    return("???");
}


/** findFlagName - get the string associated with a flag
*
*    Purpose - return a string associated with the flags for a
*              floating point instruction
*
*    Input:
*      flag    - the flags on the opcode
*      opcode  - the opcode; if it's Cvt*, we use different flags
*
*    Output:
*      pointer to string describing flags, or "/???"
*
***************/

char *
findFlagName(ULONG flag, ULONG function)
{

     PFPFLAGS table;

     if ((function == CVTTQ_FUNC) || (function == CVTGQ_FUNC)) {
         table = ConvertFlags;
     } else {
         table = FloatOpFlags;
     }

     while (table->flags != FPFLAGS_NOT_AN_ENTRY) {

         if (table->flags == flag) {
              return(table->flagname);
         }
         table++;
    }

    // no match found
    //

    return("/???");
}


/*** opTableInit - initialize fields used in and with the opTable
*
*   Purpose:
*       This routine is called once, and sets up pointers to the
*       subtables embedded in the opTable, such as AddOpTab, and
*       sizes for these subtables.  It also checks that all like
*       instructions are grouped together in the table, which is
*       the only requirement on it.
*
*   Input:
*       None.
*
*   Output:
*       None.
*
*   Errors:
*       If the table is not properly organized (four types separated,
*       and the functions for a single opcode grouped), this prints a
*       messages and fails
*
*   Exceptions:
*       None.
*
*************************************************************************/

void
opTableInit()
{

    ULONG typesDone[4] = {0,0,0,0};

    ULONG palDone, arithDone, bitDone, byteDone, jmpDone;
    ULONG fpopDone, vaxDone, IEEEDone, mulDone, memSpcDone;

    ULONG        index;

    POPTBLENTRY  entry;

    ENTRY_TYPE   curType = NOT_AN_ETYPE;
    ULONG        curFunc = NO_FUNC;    // OPCODE field in func entry

    //
    // To set the end of the table, and its size, without having
    // nested case statements, maintain pointers to the entry and
    // function tables we are currently walking through
    //

    PULONG        curTypeSize,       curFuncSize;
    POPTBLENTRY * curTypeTable,    * curFuncTable;


    //
    // these will be reset before they are needed, but not before
    // they are used.
    //

    curTypeTable = (POPTBLENTRY *)&curTypeTable;
    curTypeSize  = (PULONG)&curTypeSize;
    curFuncTable = (POPTBLENTRY *)&curFuncTable;
    curFuncSize  = (PULONG)&curFuncSize;

    palDone = arithDone = bitDone = byteDone = jmpDone = 0;
    fpopDone = vaxDone = IEEEDone = mulDone = memSpcDone = 0;

    for (index = 0 ; index < SEARCHNUM; index++) {

        entry = &opTable[index];

        switch(entry->eType) {

         case INVALID_ETYPE:

             if (curType == entry->eType)
                 continue;

             //
             // The entries must be together; if this is a
             // new type, we must never have seen it before
             //

             //
             // Finish off the old tables
             //

             *curTypeSize = (ULONG)(entry - *curTypeTable);
             if (curType == FUNCTION_ETYPE) {
                 *curFuncSize = (ULONG)(entry - *curFuncTable);
             }

             //
             // Set up the new table
             //

             InvalidTab = entry;
             curTypeSize = &InvalidTabSize;
             curTypeTable = &InvalidTab;
             curType = INVALID_ETYPE;
             typesDone[INVALID_ETYPE] = 1;
             break;

         case NON_TERMINAL_ETYPE:

             if (curType == entry->eType)
                 continue;

             *curTypeSize = (ULONG)(entry - *curTypeTable);
             if (curType == FUNCTION_ETYPE) {
                 *curFuncSize = (ULONG)(entry - *curFuncTable);
             }

             NonTerminalTab = entry;
             curTypeSize = &NonTerminalTabSize;
             curTypeTable = &NonTerminalTab;
             curType = NON_TERMINAL_ETYPE;
             typesDone[NON_TERMINAL_ETYPE] = 1;
             break;

         case TERMINAL_ETYPE:

             if (curType == entry->eType)
                 continue;

             *curTypeSize = (ULONG)(entry - *curTypeTable);
             if (curType == FUNCTION_ETYPE) {
                 *curFuncSize = (ULONG)(entry - *curFuncTable);
             }

             TerminalTab = entry;
             curTypeSize = &TerminalTabSize;
             curTypeTable = &TerminalTab;
             curType = TERMINAL_ETYPE;
             typesDone[TERMINAL_ETYPE] = 1;
             break;


         case FUNCTION_ETYPE:

             if (entry->opCode == curFunc)
                      continue;

             //
             // Take care of a new eType table; this exactly
             // parallels the three cases above (*_ETYPE)
             //

             if (curType != FUNCTION_ETYPE) {

                 *curTypeSize = (ULONG)(entry - *curTypeTable);

                 FunctionTab = entry;
                 curTypeSize = &FunctionTabSize;
                 curTypeTable = &FunctionTab;
                 curType = FUNCTION_ETYPE;
                 typesDone[FUNCTION_ETYPE] = 1;

             }

             //
             // Next, handle a new function table when this is a new
             // function (==> when this is the first entry in the
             // FunctionTab)
             //


             switch(entry->opCode) {

             POPTBLENTRY e;

             case CALLPAL_OP:

                 *curFuncSize = (ULONG)(entry - *curFuncTable);

                 e = findNonTerminalEntry(CALLPAL_OP);
                 e->funcTable = entry;
                 curFuncSize = &(e->funcTableSize);
                 curFuncTable = &(e->funcTable);

                 curFunc = CALLPAL_OP;
                 palDone = 1;

                 break;

             case ARITH_OP:

                 *curFuncSize = (ULONG)(entry - *curFuncTable);

                 e = findNonTerminalEntry(ARITH_OP);
                 e->funcTable = entry;
                 curFuncSize = &(e->funcTableSize);
                 curFuncTable = &(e->funcTable);

                 curFunc = ARITH_OP;
                 arithDone = 1;

                 break;

             case BIT_OP:

                 *curFuncSize = (ULONG)(entry - *curFuncTable);

                 e = findNonTerminalEntry(BIT_OP);
                 e->funcTable = entry;
                 curFuncSize = &(e->funcTableSize);
                 curFuncTable = &(e->funcTable);

                 curFunc = BIT_OP;
                 bitDone = 1;

                 break;

             case BYTE_OP:

                 *curFuncSize = (ULONG)(entry - *curFuncTable);

                 e = findNonTerminalEntry(BYTE_OP);
                 e->funcTable = entry;
                 curFuncSize = &(e->funcTableSize);
                 curFuncTable = &(e->funcTable);

                 curFunc = BYTE_OP;
                 byteDone = 1;

                 break;

             case SEXT_OP:

                 *curFuncSize = (ULONG)(entry - *curFuncTable);

                 e = findNonTerminalEntry(SEXT_OP);
                 e->funcTable = entry;
                 curFuncSize = &(e->funcTableSize);
                 curFuncTable = &(e->funcTable);

                 curFunc = SEXT_OP;
                 byteDone = 1;

                 break;

             case MUL_OP:

                 *curFuncSize = (ULONG)(entry - *curFuncTable);

                 e = findNonTerminalEntry(MUL_OP);
                 e->funcTable = entry;
                 curFuncSize = &(e->funcTableSize);
                 curFuncTable = &(e->funcTable);

                 curFunc = MUL_OP;
                 mulDone = 1;

                 break;

             case MEMSPC_OP:

                 *curFuncSize = (ULONG)(entry - *curFuncTable);

                 e = findNonTerminalEntry(MEMSPC_OP);
                 e->funcTable = entry;
                 curFuncSize = &(e->funcTableSize);
                 curFuncTable = &(e->funcTable);

                 curFunc = MEMSPC_OP;
                 memSpcDone = 1;

                 break;

             case JMP_OP:

                 *curFuncSize = (ULONG)(entry - *curFuncTable);

                 e = findNonTerminalEntry(JMP_OP);
                 e->funcTable = entry;
                 curFuncSize = &(e->funcTableSize);
                 curFuncTable = &(e->funcTable);

                 curFunc = JMP_OP;
                 jmpDone = 1;

                 break;

             case VAXFP_OP:

                 *curFuncSize = (ULONG)(entry - *curFuncTable);

                 e = findNonTerminalEntry(VAXFP_OP);
                 e->funcTable = entry;
                 curFuncSize = &(e->funcTableSize);
                 curFuncTable = &(e->funcTable);

                 curFunc = VAXFP_OP;
                 vaxDone = 1;

                 break;

             case IEEEFP_OP:

                 *curFuncSize = (ULONG)(entry - *curFuncTable);

                 e = findNonTerminalEntry(IEEEFP_OP);
                 e->funcTable = entry;
                 curFuncSize = &(e->funcTableSize);
                 curFuncTable = &(e->funcTable);

                 curFunc = IEEEFP_OP;
                 IEEEDone = 1;

                 break;

             case FPOP_OP:

                 *curFuncSize = (ULONG)(entry - *curFuncTable);

                 e = findNonTerminalEntry(FPOP_OP);
                 e->funcTable = entry;
                 curFuncSize = &(e->funcTableSize);
                 curFuncTable = &(e->funcTable);

                 curFunc = FPOP_OP;
                 fpopDone = 1;

                 break;

             default:
                 break;

            }  // end of Function table switch

            break;

        default:
            break;

        }      // end of etype table switch
    }          // end of For switch

    //
    // close out the size of the last tables
    //

    if (curType == FUNCTION_ETYPE) {
        *curFuncSize = (ULONG)(&opTable[SEARCHNUM] - *curFuncTable);
    }
    *curTypeSize = (ULONG)(&opTable[SEARCHNUM] - *curTypeTable);
}              // end of opTableInit

/*******************************************
//
// These routines are part of the assembler, which is not
// yet referenced in windbg, so these are place holders till then
//


ULONG ParseIntMemory (PUCHAR a, PUCHAR * b, POPTBLENTRY c, PULONG d)
{
   return(0);
}
ULONG ParseFltMemory (PUCHAR a, PUCHAR * b, POPTBLENTRY c, PULONG d)
{
   return(0);
}
ULONG ParseMemSpec   (PUCHAR a, PUCHAR * b, POPTBLENTRY c, PULONG d)
{
   return(0);
}
ULONG ParseJump      (PUCHAR a, PUCHAR * b, POPTBLENTRY c, PULONG d)
{
   return(0);
}
ULONG ParseIntBranch (PUCHAR a, PUCHAR * b, POPTBLENTRY c, PULONG d)
{
   return(0);
}
ULONG ParseFltBranch (PUCHAR a, PUCHAR * b, POPTBLENTRY c, PULONG d)
{
   return(0);
}
ULONG ParseIntOp     (PUCHAR a, PUCHAR * b, POPTBLENTRY c, PULONG d)
{
   return(0);
}
ULONG ParsePal       (PUCHAR a, PUCHAR * b, POPTBLENTRY c, PULONG d)
{
   return(0);
}
ULONG ParseUnknown   (PUCHAR a, PUCHAR * b, POPTBLENTRY c, PULONG d)
{
   return(0);
}

*******************************************/
