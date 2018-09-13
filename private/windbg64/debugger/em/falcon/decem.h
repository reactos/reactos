/***                                                                  ***/
/***   INTEL CORPORATION PROPRIETARY INFORMATION                      ***/
/***                                                                  ***/
/***   This software is supplied under the terms of a license         ***/
/***   agreement or nondisclosure agreement with Intel Corporation    ***/
/***   and may not be copied or disclosed except in accordance with   ***/
/***   the terms of that agreement.                                   ***/
/***   Copyright (c) 1992,1993,1994,1995,1996,1997 Intel Corporation. ***/
/***                                                                  ***/

#ifndef EM_DECODER_H
#define EM_DECODER_H

#include "inst_ids.h"
#include "emdb_types.h"
#include "EM.h"

#define EM_DECODER_INST_NONE  EM_INST_NONE

typedef Inst_id_t  EM_Decoder_Inst_Id;

typedef unsigned char  EM_Decoder_imp_oper_t;

#include "EM_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum em_cmp_rel_s
{
    EM_CMP_REL_NONE  = 0,
    EM_CMP_REL_GEU   = 1,
    EM_CMP_REL_LTU   = 2,
    EM_CMP_REL_EQ    = 3,
    EM_CMP_REL_NE    = 4,
    EM_CMP_REL_LT    = 5,
    EM_CMP_REL_GE    = 6,
    EM_CMP_REL_GT    = 7,
    EM_CMP_REL_LE    = 8,
    EM_CMP_REL_UNORD = 9,
    EM_CMP_REL_ORD   = 10,
    EM_CMP_REL_NEQ   = 11,
    EM_CMP_REL_NLT   = 12,
    EM_CMP_REL_NLE   = 13,
    EM_CMP_REL_LAST  = 14
} EM_cmp_rel_t;

typedef enum em_fp_precision_s
{
    EM_FP_PRECISION_NONE    = 0,
    EM_FP_PRECISION_SINGLE  = 1,
    EM_FP_PRECISION_DOUBLE  = 2,
    EM_FP_PRECISION_DYNAMIC = 3,
    EM_FP_PRECISION_LAST    = 4
} EM_fp_precision_t;    

typedef enum em_fp_status_s
{
    EM_FP_STATUS_NONE = 0,
    EM_FP_STATUS_S0   = 1,
    EM_FP_STATUS_S1   = 2,
    EM_FP_STATUS_S2   = 3,
    EM_FP_STATUS_S3   = 4,
    EM_FP_STATUS_LAST = 5
} EM_fp_status_t;

typedef enum EM_decoder_imp_operand
{
    EM_DECODER_IMP_OPERAND_NONE = 0,
    EM_DECODER_IMP_OPERAND_AR_LC,
    EM_DECODER_IMP_OPERAND_RR,
    EM_DECODER_IMP_OPERAND_AR_BSPSTORE,
    EM_DECODER_IMP_OPERAND_APP_REG_GRP_HIGH,
    EM_DECODER_IMP_OPERAND_DTR,
    EM_DECODER_IMP_OPERAND_AR_UNAT,
    EM_DECODER_IMP_OPERAND_CR_IIM,
    EM_DECODER_IMP_OPERAND_PSR,
    EM_DECODER_IMP_OPERAND_CFM,
    EM_DECODER_IMP_OPERAND_CR_IFS,
    EM_DECODER_IMP_OPERAND_AR_BSP,
    EM_DECODER_IMP_OPERAND_AR_RSC,
    EM_DECODER_IMP_OPERAND_AR_EC,
    EM_DECODER_IMP_OPERAND_AR_PFS,
    EM_DECODER_IMP_OPERAND_FPSR,
    EM_DECODER_IMP_OPERAND_APP_CCV,
    EM_DECODER_IMP_OPERAND_PR63,
    EM_DECODER_IMP_OPERAND_LAST
} EM_Decoder_Imp_Operand;

typedef enum EM_decoder_err
{ 
    EM_DECODER_NO_ERROR = 0,
    EM_DECODER_INVALID_SLOT_BRANCH_INST,
    EM_DECODER_MUST_BE_GROUP_LAST,
    EM_DECODER_BASE_EQUAL_DEST,
    EM_DECODER_EQUAL_DESTS,
    EM_DECODER_REGISTER_VALUE_OUT_OF_RANGE,
    EM_DECODER_REGISTER_RESERVED_VALUE,
    EM_DECODER_IMMEDIATE_VALUE_OUT_OF_RANGE,
    EM_DECODER_IMMEDIATE_INVALID_VALUE,
    EM_DECODER_STACK_FRAME_SIZE_OUT_OF_RANGE,
    EM_DECODER_LOCALS_SIZE_LARGER_STACK_FRAME,
    EM_DECODER_ROTATING_SIZE_LARGER_STACK_FRAME,
    EM_DECODER_FIRST_FATAL_INST_ERROR,
    EM_DECODER_INVALID_PRM_OPCODE = EM_DECODER_FIRST_FATAL_INST_ERROR,
    EM_DECODER_INVALID_INST_SLOT,
    EM_DECODER_FIRST_FATAL_ERROR,
    EM_DECODER_INVALID_TEMPLATE = EM_DECODER_FIRST_FATAL_ERROR,
    EM_DECODER_INVALID_CLIENT_ID,
    EM_DECODER_NULL_PTR,
    EM_DECODER_TOO_SHORT_ERR,
    EM_DECODER_ASSOCIATE_MISS,
    EM_DECODER_INVALID_INST_ID,
    EM_DECODER_INVALID_MACHINE_MODE,
    EM_DECODER_INVALID_MACHINE_TYPE,
    EM_DECODER_INTERNAL_ERROR,
    EM_DECODER_LAST_ERROR
} EM_Decoder_Err;

typedef EM_Decoder_Err DecErr();

typedef enum EM_decoder_machine_type
{ 
    EM_DECODER_CPU_NO_CHANGE=0,
    EM_DECODER_CPU_DEFAULT,
    EM_DECODER_CPU_P7 = 4,
    EM_DECODER_CPU_LAST = 7
} EM_Decoder_Machine_Type;

typedef enum EM_decoder_machine_mode
{
    EM_DECODER_MODE_NO_CHANGE = 0,
    EM_DECODER_MODE_DEFAULT,
    EM_DECODER_MODE_EM = 8,
    EM_DECODER_MODE_LAST = 9
} EM_Decoder_Machine_Mode;

typedef enum EM_decoder_operand_type
{
    EM_DECODER_NO_OPER = 0,
    EM_DECODER_REGISTER,
    EM_DECODER_MEMORY,
    EM_DECODER_IMMEDIATE,
    EM_DECODER_IP_RELATIVE,
    EM_DECODER_REGFILE,
    EM_DECODER_OPERAND_LAST
} EM_Decoder_Operand_Type;

typedef enum EM_decoder_reg_type
{
    EM_DECODER_NO_REG_TYPE = 0,
    EM_DECODER_INT_REG = 7,
    EM_DECODER_FP_REG,
    EM_DECODER_APP_REG,
    EM_DECODER_BR_REG,
    EM_DECODER_PRED_REG = 13,
    EM_DECODER_CR_REG,
    EM_DECODER_APP_CCV_REG,
    EM_DECODER_APP_PFS_REG,
    EM_DECODER_PR_REG,
    EM_DECODER_PR_ROT_REG,
    EM_DECODER_PSR_REG,
    EM_DECODER_PSR_L_REG,
    EM_DECODER_PSR_UM_REG = 20,
    EM_DECODER_IP_REG,            /* IP register type */
    EM_DECODER_REG_TYPE_LAST
} EM_Decoder_Reg_Type;

typedef enum EM_decoder_reg_name
{
    EM_DECODER_NO_REG=0,
    EM_DECODER_REG_R0 = 98,
    EM_DECODER_REG_R1,
    EM_DECODER_REG_R2,
    EM_DECODER_REG_R3,
    EM_DECODER_REG_R4,
    EM_DECODER_REG_R5,
    EM_DECODER_REG_R6,
    EM_DECODER_REG_R7,
    EM_DECODER_REG_R8,
    EM_DECODER_REG_R9,
    EM_DECODER_REG_R10,
    EM_DECODER_REG_R11,
    EM_DECODER_REG_R12,
    EM_DECODER_REG_R13,
    EM_DECODER_REG_R14,
    EM_DECODER_REG_R15,
    EM_DECODER_REG_R16,
    EM_DECODER_REG_R17,
    EM_DECODER_REG_R18,
    EM_DECODER_REG_R19,
    EM_DECODER_REG_R20,
    EM_DECODER_REG_R21,
    EM_DECODER_REG_R22,
    EM_DECODER_REG_R23,
    EM_DECODER_REG_R24,
    EM_DECODER_REG_R25,
    EM_DECODER_REG_R26,
    EM_DECODER_REG_R27,
    EM_DECODER_REG_R28,
    EM_DECODER_REG_R29,
    EM_DECODER_REG_R30,
    EM_DECODER_REG_R31,
    EM_DECODER_REG_R32,
    EM_DECODER_REG_R33,
    EM_DECODER_REG_R34,
    EM_DECODER_REG_R35,
    EM_DECODER_REG_R36,
    EM_DECODER_REG_R37,
    EM_DECODER_REG_R38,
    EM_DECODER_REG_R39,
    EM_DECODER_REG_R40,
    EM_DECODER_REG_R41,
    EM_DECODER_REG_R42,
    EM_DECODER_REG_R43,
    EM_DECODER_REG_R44,
    EM_DECODER_REG_R45,
    EM_DECODER_REG_R46,
    EM_DECODER_REG_R47,
    EM_DECODER_REG_R48,
    EM_DECODER_REG_R49,
    EM_DECODER_REG_R50,
    EM_DECODER_REG_R51,
    EM_DECODER_REG_R52,
    EM_DECODER_REG_R53,
    EM_DECODER_REG_R54,
    EM_DECODER_REG_R55,
    EM_DECODER_REG_R56,
    EM_DECODER_REG_R57,
    EM_DECODER_REG_R58,
    EM_DECODER_REG_R59,
    EM_DECODER_REG_R60,
    EM_DECODER_REG_R61,
    EM_DECODER_REG_R62,
    EM_DECODER_REG_R63,
    EM_DECODER_REG_R64,
    EM_DECODER_REG_R65,
    EM_DECODER_REG_R66,
    EM_DECODER_REG_R67,
    EM_DECODER_REG_R68,
    EM_DECODER_REG_R69,
    EM_DECODER_REG_R70,
    EM_DECODER_REG_R71,
    EM_DECODER_REG_R72,
    EM_DECODER_REG_R73,
    EM_DECODER_REG_R74,
    EM_DECODER_REG_R75,
    EM_DECODER_REG_R76,
    EM_DECODER_REG_R77,
    EM_DECODER_REG_R78,
    EM_DECODER_REG_R79,
    EM_DECODER_REG_R80,
    EM_DECODER_REG_R81,
    EM_DECODER_REG_R82,
    EM_DECODER_REG_R83,
    EM_DECODER_REG_R84,
    EM_DECODER_REG_R85,
    EM_DECODER_REG_R86,
    EM_DECODER_REG_R87,
    EM_DECODER_REG_R88,
    EM_DECODER_REG_R89,
    EM_DECODER_REG_R90,
    EM_DECODER_REG_R91,
    EM_DECODER_REG_R92,
    EM_DECODER_REG_R93,
    EM_DECODER_REG_R94,
    EM_DECODER_REG_R95,
    EM_DECODER_REG_R96,
    EM_DECODER_REG_R97,
    EM_DECODER_REG_R98,
    EM_DECODER_REG_R99,
    EM_DECODER_REG_R100,
    EM_DECODER_REG_R101,
    EM_DECODER_REG_R102,
    EM_DECODER_REG_R103,
    EM_DECODER_REG_R104,
    EM_DECODER_REG_R105,
    EM_DECODER_REG_R106,
    EM_DECODER_REG_R107,
    EM_DECODER_REG_R108,
    EM_DECODER_REG_R109,
    EM_DECODER_REG_R110,
    EM_DECODER_REG_R111,
    EM_DECODER_REG_R112,
    EM_DECODER_REG_R113,
    EM_DECODER_REG_R114,
    EM_DECODER_REG_R115,
    EM_DECODER_REG_R116,
    EM_DECODER_REG_R117,
    EM_DECODER_REG_R118,
    EM_DECODER_REG_R119,
    EM_DECODER_REG_R120,
    EM_DECODER_REG_R121,
    EM_DECODER_REG_R122,
    EM_DECODER_REG_R123,
    EM_DECODER_REG_R124,
    EM_DECODER_REG_R125,
    EM_DECODER_REG_R126,
    EM_DECODER_REG_R127,
    EM_DECODER_REG_F0,
    EM_DECODER_REG_F1,
    EM_DECODER_REG_F2,
    EM_DECODER_REG_F3,
    EM_DECODER_REG_F4,
    EM_DECODER_REG_F5,
    EM_DECODER_REG_F6,
    EM_DECODER_REG_F7,
    EM_DECODER_REG_F8,
    EM_DECODER_REG_F9,
    EM_DECODER_REG_F10,
    EM_DECODER_REG_F11,
    EM_DECODER_REG_F12,
    EM_DECODER_REG_F13,
    EM_DECODER_REG_F14,
    EM_DECODER_REG_F15,
    EM_DECODER_REG_F16,
    EM_DECODER_REG_F17,
    EM_DECODER_REG_F18,
    EM_DECODER_REG_F19,
    EM_DECODER_REG_F20,
    EM_DECODER_REG_F21,
    EM_DECODER_REG_F22,
    EM_DECODER_REG_F23,
    EM_DECODER_REG_F24,
    EM_DECODER_REG_F25,
    EM_DECODER_REG_F26,
    EM_DECODER_REG_F27,
    EM_DECODER_REG_F28,
    EM_DECODER_REG_F29,
    EM_DECODER_REG_F30,
    EM_DECODER_REG_F31,
    EM_DECODER_REG_F32,
    EM_DECODER_REG_F33,
    EM_DECODER_REG_F34,
    EM_DECODER_REG_F35,
    EM_DECODER_REG_F36,
    EM_DECODER_REG_F37,
    EM_DECODER_REG_F38,
    EM_DECODER_REG_F39,
    EM_DECODER_REG_F40,
    EM_DECODER_REG_F41,
    EM_DECODER_REG_F42,
    EM_DECODER_REG_F43,
    EM_DECODER_REG_F44,
    EM_DECODER_REG_F45,
    EM_DECODER_REG_F46,
    EM_DECODER_REG_F47,
    EM_DECODER_REG_F48,
    EM_DECODER_REG_F49,
    EM_DECODER_REG_F50,
    EM_DECODER_REG_F51,
    EM_DECODER_REG_F52,
    EM_DECODER_REG_F53,
    EM_DECODER_REG_F54,
    EM_DECODER_REG_F55,
    EM_DECODER_REG_F56,
    EM_DECODER_REG_F57,
    EM_DECODER_REG_F58,
    EM_DECODER_REG_F59,
    EM_DECODER_REG_F60,
    EM_DECODER_REG_F61,
    EM_DECODER_REG_F62,
    EM_DECODER_REG_F63,
    EM_DECODER_REG_F64,
    EM_DECODER_REG_F65,
    EM_DECODER_REG_F66,
    EM_DECODER_REG_F67,
    EM_DECODER_REG_F68,
    EM_DECODER_REG_F69,
    EM_DECODER_REG_F70,
    EM_DECODER_REG_F71,
    EM_DECODER_REG_F72,
    EM_DECODER_REG_F73,
    EM_DECODER_REG_F74,
    EM_DECODER_REG_F75,
    EM_DECODER_REG_F76,
    EM_DECODER_REG_F77,
    EM_DECODER_REG_F78,
    EM_DECODER_REG_F79,
    EM_DECODER_REG_F80,
    EM_DECODER_REG_F81,
    EM_DECODER_REG_F82,
    EM_DECODER_REG_F83,
    EM_DECODER_REG_F84,
    EM_DECODER_REG_F85,
    EM_DECODER_REG_F86,
    EM_DECODER_REG_F87,
    EM_DECODER_REG_F88,
    EM_DECODER_REG_F89,
    EM_DECODER_REG_F90,
    EM_DECODER_REG_F91,
    EM_DECODER_REG_F92,
    EM_DECODER_REG_F93,
    EM_DECODER_REG_F94,
    EM_DECODER_REG_F95,
    EM_DECODER_REG_F96,
    EM_DECODER_REG_F97,
    EM_DECODER_REG_F98,
    EM_DECODER_REG_F99,
    EM_DECODER_REG_F100,
    EM_DECODER_REG_F101,
    EM_DECODER_REG_F102,
    EM_DECODER_REG_F103,
    EM_DECODER_REG_F104,
    EM_DECODER_REG_F105,
    EM_DECODER_REG_F106,
    EM_DECODER_REG_F107,
    EM_DECODER_REG_F108,
    EM_DECODER_REG_F109,
    EM_DECODER_REG_F110,
    EM_DECODER_REG_F111,
    EM_DECODER_REG_F112,
    EM_DECODER_REG_F113,
    EM_DECODER_REG_F114,
    EM_DECODER_REG_F115,
    EM_DECODER_REG_F116,
    EM_DECODER_REG_F117,
    EM_DECODER_REG_F118,
    EM_DECODER_REG_F119,
    EM_DECODER_REG_F120,
    EM_DECODER_REG_F121,
    EM_DECODER_REG_F122,
    EM_DECODER_REG_F123,
    EM_DECODER_REG_F124,
    EM_DECODER_REG_F125,
    EM_DECODER_REG_F126,
    EM_DECODER_REG_F127,
    EM_DECODER_REG_AR0,
    EM_DECODER_REG_AR1,
    EM_DECODER_REG_AR2,
    EM_DECODER_REG_AR3,
    EM_DECODER_REG_AR4,
    EM_DECODER_REG_AR5,
    EM_DECODER_REG_AR6,
    EM_DECODER_REG_AR7,
    EM_DECODER_REG_AR8,
    EM_DECODER_REG_AR9,
    EM_DECODER_REG_AR10,
    EM_DECODER_REG_AR11,
    EM_DECODER_REG_AR12,
    EM_DECODER_REG_AR13,
    EM_DECODER_REG_AR14,
    EM_DECODER_REG_AR15,
    EM_DECODER_REG_AR16,
    EM_DECODER_REG_AR17,
    EM_DECODER_REG_AR18,
    EM_DECODER_REG_AR19,
    EM_DECODER_REG_AR20,
    EM_DECODER_REG_AR21,
    EM_DECODER_REG_AR22,
    EM_DECODER_REG_AR23,
    EM_DECODER_REG_AR24,
    EM_DECODER_REG_AR25,
    EM_DECODER_REG_AR26,
    EM_DECODER_REG_AR27,
    EM_DECODER_REG_AR28,
    EM_DECODER_REG_AR29,
    EM_DECODER_REG_AR30,
    EM_DECODER_REG_AR31,
    EM_DECODER_REG_AR32,
    EM_DECODER_REG_AR33,
    EM_DECODER_REG_AR34,
    EM_DECODER_REG_AR35,
    EM_DECODER_REG_AR36,
    EM_DECODER_REG_AR37,
    EM_DECODER_REG_AR38,
    EM_DECODER_REG_AR39,
    EM_DECODER_REG_AR40,
    EM_DECODER_REG_AR41,
    EM_DECODER_REG_AR42,
    EM_DECODER_REG_AR43,
    EM_DECODER_REG_AR44,
    EM_DECODER_REG_AR45,
    EM_DECODER_REG_AR46,
    EM_DECODER_REG_AR47,
    EM_DECODER_REG_AR48,
    EM_DECODER_REG_AR49,
    EM_DECODER_REG_AR50,
    EM_DECODER_REG_AR51,
    EM_DECODER_REG_AR52,
    EM_DECODER_REG_AR53,
    EM_DECODER_REG_AR54,
    EM_DECODER_REG_AR55,
    EM_DECODER_REG_AR56,
    EM_DECODER_REG_AR57,
    EM_DECODER_REG_AR58,
    EM_DECODER_REG_AR59,
    EM_DECODER_REG_AR60,
    EM_DECODER_REG_AR61,
    EM_DECODER_REG_AR62,
    EM_DECODER_REG_AR63,
    EM_DECODER_REG_AR64,
    EM_DECODER_REG_AR65,
    EM_DECODER_REG_AR66,
    EM_DECODER_REG_AR67,
    EM_DECODER_REG_AR68,
    EM_DECODER_REG_AR69,
    EM_DECODER_REG_AR70,
    EM_DECODER_REG_AR71,
    EM_DECODER_REG_AR72,
    EM_DECODER_REG_AR73,
    EM_DECODER_REG_AR74,
    EM_DECODER_REG_AR75,
    EM_DECODER_REG_AR76,
    EM_DECODER_REG_AR77,
    EM_DECODER_REG_AR78,
    EM_DECODER_REG_AR79,
    EM_DECODER_REG_AR80,
    EM_DECODER_REG_AR81,
    EM_DECODER_REG_AR82,
    EM_DECODER_REG_AR83,
    EM_DECODER_REG_AR84,
    EM_DECODER_REG_AR85,
    EM_DECODER_REG_AR86,
    EM_DECODER_REG_AR87,
    EM_DECODER_REG_AR88,
    EM_DECODER_REG_AR89,
    EM_DECODER_REG_AR90,
    EM_DECODER_REG_AR91,
    EM_DECODER_REG_AR92,
    EM_DECODER_REG_AR93,
    EM_DECODER_REG_AR94,
    EM_DECODER_REG_AR95,
    EM_DECODER_REG_AR96,
    EM_DECODER_REG_AR97,
    EM_DECODER_REG_AR98,
    EM_DECODER_REG_AR99,
    EM_DECODER_REG_AR100,
    EM_DECODER_REG_AR101,
    EM_DECODER_REG_AR102,
    EM_DECODER_REG_AR103,
    EM_DECODER_REG_AR104,
    EM_DECODER_REG_AR105,
    EM_DECODER_REG_AR106,
    EM_DECODER_REG_AR107,
    EM_DECODER_REG_AR108,
    EM_DECODER_REG_AR109,
    EM_DECODER_REG_AR110,
    EM_DECODER_REG_AR111,
    EM_DECODER_REG_AR112,
    EM_DECODER_REG_AR113,
    EM_DECODER_REG_AR114,
    EM_DECODER_REG_AR115,
    EM_DECODER_REG_AR116,
    EM_DECODER_REG_AR117,
    EM_DECODER_REG_AR118,
    EM_DECODER_REG_AR119,
    EM_DECODER_REG_AR120,
    EM_DECODER_REG_AR121,
    EM_DECODER_REG_AR122,
    EM_DECODER_REG_AR123,
    EM_DECODER_REG_AR124,
    EM_DECODER_REG_AR125,
    EM_DECODER_REG_AR126,
    EM_DECODER_REG_AR127,
    EM_DECODER_REG_P0,
    EM_DECODER_REG_P1,
    EM_DECODER_REG_P2,
    EM_DECODER_REG_P3,
    EM_DECODER_REG_P4,
    EM_DECODER_REG_P5,
    EM_DECODER_REG_P6,
    EM_DECODER_REG_P7,
    EM_DECODER_REG_P8,
    EM_DECODER_REG_P9,
    EM_DECODER_REG_P10,
    EM_DECODER_REG_P11,
    EM_DECODER_REG_P12,
    EM_DECODER_REG_P13,
    EM_DECODER_REG_P14,
    EM_DECODER_REG_P15,
    EM_DECODER_REG_P16,
    EM_DECODER_REG_P17,
    EM_DECODER_REG_P18,
    EM_DECODER_REG_P19,
    EM_DECODER_REG_P20,
    EM_DECODER_REG_P21,
    EM_DECODER_REG_P22,
    EM_DECODER_REG_P23,
    EM_DECODER_REG_P24,
    EM_DECODER_REG_P25,
    EM_DECODER_REG_P26,
    EM_DECODER_REG_P27,
    EM_DECODER_REG_P28,
    EM_DECODER_REG_P29,
    EM_DECODER_REG_P30,
    EM_DECODER_REG_P31,
    EM_DECODER_REG_P32,
    EM_DECODER_REG_P33,
    EM_DECODER_REG_P34,
    EM_DECODER_REG_P35,
    EM_DECODER_REG_P36,
    EM_DECODER_REG_P37,
    EM_DECODER_REG_P38,
    EM_DECODER_REG_P39,
    EM_DECODER_REG_P40,
    EM_DECODER_REG_P41,
    EM_DECODER_REG_P42,
    EM_DECODER_REG_P43,
    EM_DECODER_REG_P44,
    EM_DECODER_REG_P45,
    EM_DECODER_REG_P46,
    EM_DECODER_REG_P47,
    EM_DECODER_REG_P48,
    EM_DECODER_REG_P49,
    EM_DECODER_REG_P50,
    EM_DECODER_REG_P51,
    EM_DECODER_REG_P52,
    EM_DECODER_REG_P53,
    EM_DECODER_REG_P54,
    EM_DECODER_REG_P55,
    EM_DECODER_REG_P56,
    EM_DECODER_REG_P57,
    EM_DECODER_REG_P58,
    EM_DECODER_REG_P59,
    EM_DECODER_REG_P60,
    EM_DECODER_REG_P61,
    EM_DECODER_REG_P62,
    EM_DECODER_REG_P63,
    EM_DECODER_REG_BR0 ,
    EM_DECODER_REG_BR1,
    EM_DECODER_REG_BR2,
    EM_DECODER_REG_BR3,
    EM_DECODER_REG_BR4,
    EM_DECODER_REG_BR5,
    EM_DECODER_REG_BR6,
    EM_DECODER_REG_BR7,
    EM_DECODER_REG_PR,
    EM_DECODER_REG_PR_ROT,
    EM_DECODER_REG_CR0,
    EM_DECODER_REG_CR1,
    EM_DECODER_REG_CR2,
    EM_DECODER_REG_CR3,
    EM_DECODER_REG_CR4,
    EM_DECODER_REG_CR5,
    EM_DECODER_REG_CR6,
    EM_DECODER_REG_CR7,
    EM_DECODER_REG_CR8,
    EM_DECODER_REG_CR9,
    EM_DECODER_REG_CR10,
    EM_DECODER_REG_CR11,
    EM_DECODER_REG_CR12,
    EM_DECODER_REG_CR13,
    EM_DECODER_REG_CR14,
    EM_DECODER_REG_CR15,
    EM_DECODER_REG_CR16,
    EM_DECODER_REG_CR17,
    EM_DECODER_REG_CR18,
    EM_DECODER_REG_CR19,
    EM_DECODER_REG_CR20,
    EM_DECODER_REG_CR21,
    EM_DECODER_REG_CR22,
    EM_DECODER_REG_CR23,
    EM_DECODER_REG_CR24,
    EM_DECODER_REG_CR25,
    EM_DECODER_REG_CR26,
    EM_DECODER_REG_CR27,
    EM_DECODER_REG_CR28,
    EM_DECODER_REG_CR29,
    EM_DECODER_REG_CR30,
    EM_DECODER_REG_CR31,
    EM_DECODER_REG_CR32,
    EM_DECODER_REG_CR33,
    EM_DECODER_REG_CR34,
    EM_DECODER_REG_CR35,
    EM_DECODER_REG_CR36,
    EM_DECODER_REG_CR37,
    EM_DECODER_REG_CR38,
    EM_DECODER_REG_CR39,
    EM_DECODER_REG_CR40,
    EM_DECODER_REG_CR41,
    EM_DECODER_REG_CR42,
    EM_DECODER_REG_CR43,
    EM_DECODER_REG_CR44,
    EM_DECODER_REG_CR45,
    EM_DECODER_REG_CR46,
    EM_DECODER_REG_CR47,
    EM_DECODER_REG_CR48,
    EM_DECODER_REG_CR49,
    EM_DECODER_REG_CR50,
    EM_DECODER_REG_CR51,
    EM_DECODER_REG_CR52,
    EM_DECODER_REG_CR53,
    EM_DECODER_REG_CR54,
    EM_DECODER_REG_CR55,
    EM_DECODER_REG_CR56,
    EM_DECODER_REG_CR57,
    EM_DECODER_REG_CR58,
    EM_DECODER_REG_CR59,
    EM_DECODER_REG_CR60,
    EM_DECODER_REG_CR61,
    EM_DECODER_REG_CR62,
    EM_DECODER_REG_CR63,
    EM_DECODER_REG_CR64,
    EM_DECODER_REG_CR65,
    EM_DECODER_REG_CR66,
    EM_DECODER_REG_CR67,
    EM_DECODER_REG_CR68,
    EM_DECODER_REG_CR69,
    EM_DECODER_REG_CR70,
    EM_DECODER_REG_CR71,
    EM_DECODER_REG_CR72,
    EM_DECODER_REG_CR73,
    EM_DECODER_REG_CR74,
    EM_DECODER_REG_CR75,
    EM_DECODER_REG_CR76,
    EM_DECODER_REG_CR77,
    EM_DECODER_REG_CR78,
    EM_DECODER_REG_CR79,
    EM_DECODER_REG_CR80,
    EM_DECODER_REG_CR81,
    EM_DECODER_REG_CR82,
    EM_DECODER_REG_CR83,
    EM_DECODER_REG_CR84,
    EM_DECODER_REG_CR85,
    EM_DECODER_REG_CR86,
    EM_DECODER_REG_CR87,
    EM_DECODER_REG_CR88,
    EM_DECODER_REG_CR89,
    EM_DECODER_REG_CR90,
    EM_DECODER_REG_CR91,
    EM_DECODER_REG_CR92,
    EM_DECODER_REG_CR93,
    EM_DECODER_REG_CR94,
    EM_DECODER_REG_CR95,
    EM_DECODER_REG_CR96,
    EM_DECODER_REG_CR97,
    EM_DECODER_REG_CR98,
    EM_DECODER_REG_CR99,
    EM_DECODER_REG_CR100,
    EM_DECODER_REG_CR101,
    EM_DECODER_REG_CR102,
    EM_DECODER_REG_CR103,
    EM_DECODER_REG_CR104,
    EM_DECODER_REG_CR105,
    EM_DECODER_REG_CR106,
    EM_DECODER_REG_CR107,
    EM_DECODER_REG_CR108,
    EM_DECODER_REG_CR109,
    EM_DECODER_REG_CR110,
    EM_DECODER_REG_CR111,
    EM_DECODER_REG_CR112,
    EM_DECODER_REG_CR113,
    EM_DECODER_REG_CR114,
    EM_DECODER_REG_CR115,
    EM_DECODER_REG_CR116,
    EM_DECODER_REG_CR117,
    EM_DECODER_REG_CR118,
    EM_DECODER_REG_CR119,
    EM_DECODER_REG_CR120,
    EM_DECODER_REG_CR121,
    EM_DECODER_REG_CR122,
    EM_DECODER_REG_CR123,
    EM_DECODER_REG_CR124,
    EM_DECODER_REG_CR125,
    EM_DECODER_REG_CR126,
    EM_DECODER_REG_CR127,
    EM_DECODER_REG_PSR,
    EM_DECODER_REG_PSR_L,
    EM_DECODER_REG_PSR_UM,
    EM_DECODER_REG_IP,      /* register IP name */ 
    EM_DECODER_EM_REG_LAST,

    EM_DECODER_REG_AR_K0   = EM_DECODER_REG_AR0+EM_AR_KR0,
    EM_DECODER_REG_AR_K1   = EM_DECODER_REG_AR0+EM_AR_KR1,
    EM_DECODER_REG_AR_K2   = EM_DECODER_REG_AR0+EM_AR_KR2,
    EM_DECODER_REG_AR_K3   = EM_DECODER_REG_AR0+EM_AR_KR3,
    EM_DECODER_REG_AR_K4   = EM_DECODER_REG_AR0+EM_AR_KR4, /* added AR_K4-7 */
    EM_DECODER_REG_AR_K5   = EM_DECODER_REG_AR0+EM_AR_KR5,
    EM_DECODER_REG_AR_K6   = EM_DECODER_REG_AR0+EM_AR_KR6,
    EM_DECODER_REG_AR_K7   = EM_DECODER_REG_AR0+EM_AR_KR7,
    EM_DECODER_REG_AR_RSC  = EM_DECODER_REG_AR0+EM_AR_RSC,
    EM_DECODER_REG_AR_BSP  = EM_DECODER_REG_AR0+EM_AR_BSP,
    EM_DECODER_REG_AR_BSPSTORE = EM_DECODER_REG_AR0+EM_AR_BSPSTORE,
    EM_DECODER_REG_AR_RNAT = EM_DECODER_REG_AR0+EM_AR_RNAT,
    EM_DECODER_REG_AR_EFLAG= EM_DECODER_REG_AR0+EM_AR_EFLAG,
    EM_DECODER_REG_AR_CSD  = EM_DECODER_REG_AR0+EM_AR_CSD,
    EM_DECODER_REG_AR_SSD  = EM_DECODER_REG_AR0+EM_AR_SSD,
    EM_DECODER_REG_AR_CFLG = EM_DECODER_REG_AR0+EM_AR_CFLG,
    EM_DECODER_REG_AR_FSR  = EM_DECODER_REG_AR0+EM_AR_FSR,
    EM_DECODER_REG_AR_FIR  = EM_DECODER_REG_AR0+EM_AR_FIR,
    EM_DECODER_REG_AR_FDR  = EM_DECODER_REG_AR0+EM_AR_FDR,
    EM_DECODER_REG_AR_CCV  = EM_DECODER_REG_AR0+EM_AR_CCV,
    EM_DECODER_REG_AR_UNAT = EM_DECODER_REG_AR0+EM_AR_UNAT,
    EM_DECODER_REG_AR_FPSR = EM_DECODER_REG_AR0+EM_AR_FPSR,
    EM_DECODER_REG_AR_ITC  = EM_DECODER_REG_AR0+EM_AR_ITC,  
    EM_DECODER_REG_AR_PFS  = EM_DECODER_REG_AR0+EM_AR_PFS,
    EM_DECODER_REG_AR_LC   = EM_DECODER_REG_AR0+EM_AR_LC,
    EM_DECODER_REG_AR_EC   = EM_DECODER_REG_AR0+EM_AR_EC,

    EM_DECODER_REG_CR_DCR  = EM_DECODER_REG_CR0+EM_CR_DCR,
    EM_DECODER_REG_CR_ITM  = EM_DECODER_REG_CR0+EM_CR_ITM,
    EM_DECODER_REG_CR_IVA  = EM_DECODER_REG_CR0+EM_CR_IVA,
    EM_DECODER_REG_CR_PTA  = EM_DECODER_REG_CR0+EM_CR_PTA,
    EM_DECODER_REG_CR_GPTA = EM_DECODER_REG_CR0+EM_CR_GPTA,
    EM_DECODER_REG_CR_IPSR = EM_DECODER_REG_CR0+EM_CR_IPSR,
    EM_DECODER_REG_CR_ISR  = EM_DECODER_REG_CR0+EM_CR_ISR,
    EM_DECODER_REG_CR_IIP  = EM_DECODER_REG_CR0+EM_CR_IIP,
    EM_DECODER_REG_CR_IFA  = EM_DECODER_REG_CR0+EM_CR_IFA,
    EM_DECODER_REG_CR_ITIR = EM_DECODER_REG_CR0+EM_CR_ITIR,
    EM_DECODER_REG_CR_IIPA = EM_DECODER_REG_CR0+EM_CR_IIPA,
    EM_DECODER_REG_CR_IFS  = EM_DECODER_REG_CR0+EM_CR_IFS,
    EM_DECODER_REG_CR_IIM  = EM_DECODER_REG_CR0+EM_CR_IIM,
    EM_DECODER_REG_CR_IHA  = EM_DECODER_REG_CR0+EM_CR_IHA,

    EM_DECODER_REG_CR_LID  = EM_DECODER_REG_CR0+EM_CR_LID,
    EM_DECODER_REG_CR_IVR  = EM_DECODER_REG_CR0+EM_CR_IVR,
    EM_DECODER_REG_CR_TPR  = EM_DECODER_REG_CR0+EM_CR_TPR,
    EM_DECODER_REG_CR_EOI  = EM_DECODER_REG_CR0+EM_CR_EOI,
    EM_DECODER_REG_CR_IRR0 = EM_DECODER_REG_CR0+EM_CR_IRR0,
    EM_DECODER_REG_CR_IRR1 = EM_DECODER_REG_CR0+EM_CR_IRR1,
    EM_DECODER_REG_CR_IRR2 = EM_DECODER_REG_CR0+EM_CR_IRR2,
    EM_DECODER_REG_CR_IRR3 = EM_DECODER_REG_CR0+EM_CR_IRR3,
    EM_DECODER_REG_CR_ITV  = EM_DECODER_REG_CR0+EM_CR_ITV,
    EM_DECODER_REG_CR_PMV  = EM_DECODER_REG_CR0+EM_CR_PMV,
    EM_DECODER_REG_CR_LRR0 = EM_DECODER_REG_CR0+EM_CR_LRR0,
    EM_DECODER_REG_CR_LRR1 = EM_DECODER_REG_CR0+EM_CR_LRR1,
    EM_DECODER_REG_CR_CMCV = EM_DECODER_REG_CR0+EM_CR_CMCV,
        
/************************************************************/
    EM_DECODER_REG_LAST
} EM_Decoder_Reg_Name;

typedef struct EM_decoder_reg_info
{
    int                  valid;
    EM_Decoder_Reg_Type  type;
    EM_Decoder_Reg_Name  name;
    long                 value;
} EM_Decoder_Reg_Info;

typedef enum EM_DECODER_regfile_name
{
    EM_DECODER_NO_REGFILE = 0,
    EM_DECODER_REGFILE_PMC,
    EM_DECODER_REGFILE_PMD,
    EM_DECODER_REGFILE_PKR,
    EM_DECODER_REGFILE_RR,
    EM_DECODER_REGFILE_IBR,
    EM_DECODER_REGFILE_DBR,
    EM_DECODER_REGFILE_ITR,
    EM_DECODER_REGFILE_DTR,
    EM_DECODER_REGFILE_MSR,
    EM_DECODER_REGFILE_LAST
} EM_Decoder_Regfile_Name;

typedef enum EM_decoder_operand_2nd_role
{
    EM_DECODER_OPER_2ND_ROLE_NONE = 0,
    EM_DECODER_OPER_2ND_ROLE_SRC,
    EM_DECODER_OPER_2ND_ROLE_DST
} EM_Decoder_Operand_2nd_Role;

typedef enum EM_decoder_oper_size
{
    EM_DECODER_OPER_NO_SIZE =  0,
    EM_DECODER_OPER_SIZE_1 =   1,
    EM_DECODER_OPER_SIZE_2 =   2,
    EM_DECODER_OPER_SIZE_4 =   4,
    EM_DECODER_OPER_SIZE_8 =   8,
    EM_DECODER_OPER_SIZE_10 = 10,
    EM_DECODER_OPER_SIZE_16 = 16,
    EM_DECODER_OPER_SIZE_20 = 20,
    EM_DECODER_OPER_SIZE_22 = 22,
    EM_DECODER_OPER_SIZE_24 = 24,  
    EM_DECODER_OPER_SIZE_32 = 32,
    EM_DECODER_OPER_SIZE_64 = 64
} EM_Decoder_Oper_Size;

typedef enum EM_decoder_imm_type
{
    EM_DECODER_IMM_NONE,
    EM_DECODER_IMM_SIGNED,
    EM_DECODER_IMM_UNSIGNED,
    EM_DECODER_IMM_FCLASS,
    EM_DECODER_IMM_MUX1,
    EM_DECODER_IMM_LAST
} EM_Decoder_Imm_Type;

typedef enum EM_decoder_slot
{
    EM_DECODER_SLOT_0 = 0,
    EM_DECODER_SLOT_1 = 1,
    EM_DECODER_SLOT_2 = 2,
    EM_DECODER_SLOT_LAST = 2
} EM_Decoder_Slot;


/***** EM_decoder Structure Defenition ****/

typedef struct EM_decoder_modifiers_s
{
    EM_cmp_type_t           cmp_type;
    EM_cmp_rel_t            cmp_rel;
    EM_branch_type_t        branch_type;
    EM_branch_hint_t        branch_hint;
    EM_fp_precision_t       fp_precision;
    EM_fp_status_t          fp_status;
    EM_memory_access_hint_t mem_access_hint;
} EM_Decoder_modifiers_t;   


typedef struct EM_decoder_oper_static_s
{
    Operand_role_t role;
    Operand_type_t type;
    unsigned long  flags;
} EM_Decoder_oper_static_t;


typedef struct EM_decoder_static_info_s
{
    Mnemonic_t               mnemonic;
    Template_role_t          template_role;
    EM_Decoder_oper_static_t explicit_dst[2];
    EM_Decoder_oper_static_t explicit_src[5];
    EM_Decoder_imp_oper_t    implicit_dst[8];
    EM_Decoder_imp_oper_t    implicit_src[8];
    EM_Decoder_modifiers_t   modifiers;
    Flags_t                  flags;
} EM_Decoder_static_info_t;


extern const EM_Decoder_static_info_t em_decoder_static_info[];


typedef struct EM_decoder_regfile_info
{
    EM_Decoder_Regfile_Name  name;
    EM_Decoder_Reg_Info      index;
} EM_Decoder_Regfile_Info;


typedef struct EM_decoder_imm_info
{
    EM_Decoder_Imm_Type  imm_type;
    unsigned int         size;
    U64                  val64;
} EM_Decoder_Imm_Info;

typedef struct EM_decoder_mem_info
{
    EM_Decoder_Reg_Info    mem_base;
    EM_Decoder_Oper_Size   size;
} EM_Decoder_Mem_Info;

typedef struct em_decoder_em_bundle_info
{
    EM_template_t           b_template;
    unsigned long           flags;
} EM_Decoder_EM_Bundle_Info;

typedef struct em_decoder_em_info
{
    EM_Decoder_EM_Bundle_Info  em_bundle_info;
    EM_Decoder_Slot            slot_no;
    Template_role_t            eut;
    unsigned long              em_flags;
} EM_Decoder_EM_Info;

typedef struct EM_decoder_inst_static_info
{
    void *                client_info;
    const EM_Decoder_static_info_t *static_info;
    unsigned long         flags;
} EM_Decoder_Inst_Static_Info;


typedef struct EM_decoder_operand_info
{
    EM_Decoder_Operand_Type     type;
    EM_Decoder_Regfile_Info     regfile_info;
    EM_Decoder_Reg_Info         reg_info;
    EM_Decoder_Mem_Info         mem_info;
    EM_Decoder_Imm_Info         imm_info;
    long                        ip_relative_offset;
    unsigned long               oper_flags;
} EM_Decoder_Operand_Info;


typedef struct em_decoder_info
{
    EM_Decoder_Inst_Id       inst;
    EM_Decoder_Reg_Info      pred;
    EM_Decoder_Operand_Info  src1;
    EM_Decoder_Operand_Info  src2;
    EM_Decoder_Operand_Info  src3;
    EM_Decoder_Operand_Info  src4;
    EM_Decoder_Operand_Info  src5;
    EM_Decoder_Operand_Info  dst1;
    EM_Decoder_Operand_Info  dst2;
    EM_Decoder_EM_Info       EM_info;
    void *                   client_info;
    unsigned long            flags;
    unsigned char            size;
    const EM_Decoder_static_info_t *static_info;
} EM_Decoder_Info;


typedef struct em_decoder_bundle_info
{
    unsigned int                inst_num;
    EM_Decoder_EM_Bundle_Info   em_bundle_info;
    EM_Decoder_Err              error[3];
    EM_Decoder_Info             inst_info[3];
} EM_Decoder_Bundle_Info;
    

typedef int EM_Decoder_Id;


/***********************************************/
/***          Setup flags                    ***/
/***********************************************/
  
#define EM_DECODER_FLAG_NO_MEMSET       0X00000001


extern const U32 em_decoder_bundle_size;

EM_Decoder_Id  em_decoder_open(void);

EM_Decoder_Err em_decoder_associate_one(const EM_Decoder_Id      id,
                                   const EM_Decoder_Inst_Id      inst,
                                   const void                  * client_info);

EM_Decoder_Err em_decoder_associate_check(const EM_Decoder_Id    id,
                                           EM_Decoder_Inst_Id  * inst);

EM_Decoder_Err em_decoder_setenv(const EM_Decoder_Id,
                           const EM_Decoder_Machine_Type,
                           const EM_Decoder_Machine_Mode);

EM_Decoder_Err em_decoder_setup(const EM_Decoder_Id,
                           const EM_Decoder_Machine_Type,
                           const EM_Decoder_Machine_Mode,
                           unsigned long flags);

EM_Decoder_Err em_decoder_close(const EM_Decoder_Id);

EM_Decoder_Err em_decoder_decode(const EM_Decoder_Id    id,
                                 const unsigned char  * code,
                                 const int              max_code_size,
                                 const EM_IL            location,
                                 EM_Decoder_Info      * decoder_info);

 EM_Decoder_Err em_decoder_inst_static_info(const  EM_Decoder_Id,
                                            const  EM_Decoder_Inst_Id,
                                            EM_Decoder_Inst_Static_Info *);

 const char* em_decoder_ver_str(void);

 void  em_decoder_get_version(EM_library_version_t  * dec_version);

 const char* em_decoder_err_msg(EM_Decoder_Err error);

 EM_Decoder_Err em_decoder_decode_bundle(const EM_Decoder_Id        id,
                                         const unsigned char      * code,
                                         const int                  max_size,
                                         EM_Decoder_Bundle_Info   * bundle_info);

/**********************  GET next IL  *************************/
#define EM_DECODER_NEXT(IL, decoder_info)                       \
{                                                               \
    U32 rem_size;                                               \
    int slot_no = EM_IL_GET_SLOT_NO(IL),                        \
        size = (decoder_info)->size;                            \
    switch (slot_no)                                            \
    {                                                           \
      case 0:                                                   \
        break;                                                  \
      case 1:                                                   \
        if (size < 2)                                           \
            break;      /*** else fall-through ***/             \
      case 2:                                                   \
        size = EM_BUNDLE_SIZE - slot_no;                        \
    }                                                           \
    IEL_CONVERT1(rem_size, size);                               \
    IEL_ADDU(IL, IL, rem_size);                                 \
}

#define EM_DECODER_BUNDLE_NEXT(addr)                            \
{                                                               \
    IEL_ADDU(addr, addr, em_decoder_bundle_size);               \
}


#define EM_DECODER_ERROR_IS_FATAL(_Err)          \
     ((_Err) >= EM_DECODER_FIRST_FATAL_ERROR)

#define EM_DECODER_ERROR_IS_INST_FATAL(_Err)      \
     (((_Err) >= EM_DECODER_FIRST_FATAL_INST_ERROR) && ((_Err) < EM_DECODER_FIRST_FATAL_ERROR))


/*************   EM Instruction Flags Related Macros   ***************/

/*** EM_decoder and static infos flags ***/

/* Flags that copied directly from EMDB */

#define EM_DECODER_BIT_PREDICATE          EM_FLAG_PRED               /* The instruction can get pred  */
#define EM_DECODER_BIT_PRIVILEGED         EM_FLAG_PRIVILEGED         /* The instruction is privileged */
#define EM_DECODER_BIT_LMEM               EM_FLAG_LMEM               /* The instuction is a load inst */
#define EM_DECODER_BIT_SMEM               EM_FLAG_SMEM               /* The instruction is a store    */
#define EM_DECODER_BIT_CHECK_BASE_EQ_DST  EM_FLAG_CHECK_BASE_EQ_DST  /* Base value must differ from destination's */
#define EM_DECODER_BIT_GROUP_FIRST        EM_FLAG_FIRST_IN_INSTRUCTION_GROUP /* Instruction must be the first in instruction group */
#define EM_DECODER_BIT_GROUP_LAST         EM_FLAG_LAST_IN_INSTRUCTION_GROUP  /* Instruction must be the last in instruction group */
#define EM_DECODER_BIT_CHECK_SAME_DSTS    EM_FLAG_CHECK_SAME_DSTS    /* Two destinations should have different values */

/* Others */
#define EM_DECODER_BIT_SPECULATION        (EMDB_LAST_FLAG << 1)      /* Speculative form of instruction */
#define EM_DECODER_BIT_POSTINCREMENT      (EMDB_LAST_FLAG << 2)      /* Post increment form of instruction */
#define EM_DECODER_BIT_FALSE_PRED_EXEC    (EMDB_LAST_FLAG << 3)      /* Instruction executed when predicate is false */
#define EM_DECODER_BIT_BR_HINT            (EMDB_LAST_FLAG << 4)      /* Branch-hint form of instruction */ 
#define EM_DECODER_BIT_BR                 (EMDB_LAST_FLAG << 5)      /* Branch instruction              */
#define EM_DECODER_BIT_ADV_LOAD           (EMDB_LAST_FLAG << 6)      /* Instruction is an advanced or speculative advanced load */
#define EM_DECODER_BIT_CONTROL_TRANSFER   (EMDB_LAST_FLAG << 7)      /* Instruction violates sequential control flow */

/*** em_flags ***/

/* Flags that depend on the current bundle encoding */

/* in em_flags: */
#define EM_DECODER_BIT_CYCLE_BREAK 0x10000 /*Inst is last in its group */
#define EM_DECODER_BIT_LAST_INST   0x20000 /*Last instruction in bundle   */

/* Static flags (depend only on inst id) */
 
#define EM_DECODER_BIT_LONG_INST   0x40000 /* 2 slots Inst (movl)*/

/* in em_bundle_info flags */
#define EM_DECODER_BIT_BUNDLE_STOP 0x80000 /*Stop bit is set in bundle*/


#define EM_DECODER_PREDICATE(di)                (EM_DECODER_BIT_PREDICATE & ((di)->flags))
#define EM_DECODER_PRIVILEGED(di)               (EM_DECODER_BIT_PRIVILEGED & ((di)->flags))
#define EM_DECODER_LMEM(di)                     (EM_DECODER_BIT_LMEM & ((di)->flags))      
#define EM_DECODER_SMEM(di)                     (EM_DECODER_BIT_SMEM & ((di)->flags))
#define EM_DECODER_CHECK_BASE_EQ_DST(di)        (EM_DECODER_BIT_CHECK_BASE_EQ_DST & ((di)->flags))
#define EM_DECODER_CHECK_SPECULATION(di)        (EM_DECODER_BIT_SPECULATION & ((di)->flags))
#define EM_DECODER_CHECK_POSTINCREMENT(di)      (EM_DECODER_BIT_POSTINCREMENT & ((di)->flags))
#define EM_DECODER_CHECK_FALSE_PRED_EXEC(di)    (EM_DECODER_BIT_FALSE_PRED_EXEC & ((di)->flags))
#define EM_DECODER_CHECK_BR_HINT(di)            (EM_DECODER_BIT_BR_HINT & ((di)->flags))
#define EM_DECODER_CHECK_BR(di)                 (EM_DECODER_BIT_BR & ((di)->flags))
#define EM_DECODER_CHECK_GROUP_FIRST(di)        (EM_DECODER_BIT_GROUP_FIRST & ((di)->flags))
#define EM_DECODER_CHECK_GROUP_LAST(di)         (EM_DECODER_BIT_GROUP_LAST & ((di)->flags))
#define EM_DECODER_CHECK_SAME_DSTS(di)          (EM_DECODER_BIT_CHECK_SAME_DSTS & ((di)->flags))
#define EM_DECODER_CHECK_ADV_LOAD(di)           (EM_DECODER_BIT_ADV_LOAD & ((di)->flags))
#define EM_DECODER_CHECK_CONTROL_TRANSFER(di)   (EM_DECODER_BIT_CONTROL_TRANSFER & ((di)->flags))

#define EM_DECODER_LONG_INST(di)        \
        (EM_DECODER_BIT_LONG_INST & (((di)->EM_info).em_flags))

#define EM_DECODER_LAST_INST(di)                \
        (EM_DECODER_BIT_LAST_INST & (((di)->EM_info).em_flags))

#define EM_DECODER_CYCLE_BREAK(di)      \
        (EM_DECODER_BIT_CYCLE_BREAK & (((di)->EM_info).em_flags))

#define EM_DECODER_BUNDLE_STOP(di)      \
        (EM_DECODER_BIT_BUNDLE_STOP &   \
         (((di)->EM_info).em_bundle_info.flags))



/************** Operand Related macros ****************/
                                    
#define EM_DECODER_OPER_2ND_ROLE_SRC_BIT  0x00000001  /* Oper second role:  src */
#define EM_DECODER_OPER_2ND_ROLE_DST_BIT  0x00000002  /* Oper second role: dest */
#define EM_DECODER_OPER_IMM_IREG_BIT      0x00000040  /* Operand type is IREG_NUM */
#define EM_DECODER_OPER_IMM_FREG_BIT      0x00000080  /* Operand type is FREG_NUM */

#define EM_DECODER_OPER_2ND_ROLE_SRC(oi)                                         \
                            (((oi)->oper_flags) & EM_DECODER_OPER_2ND_ROLE_SRC_BIT)
#define EM_DECODER_OPER_2ND_ROLE_DST(oi)                                         \
                            (((oi)->oper_flags) & EM_DECODER_OPER_2ND_ROLE_DST_BIT)
#define EM_DECODER_OPER_NOT_TRUE_SRC(oi)                                         \
                            (((oi)->oper_flags) & EM_DECODER_OPER_NOT_TRUE_SRC_BIT)
#define EM_DECODER_OPER_IMP_ENCODED(oi)                                          \
                            (((oi)->oper_flags) & EM_DECODER_OPER_IMP_ENCODED_BIT)

#define EM_DECODER_OPER_IMM_REG(oi)                                             \
    (((oi)->oper_flags) & (EM_DECODER_OPER_IMM_IREG_BIT |                       \
                           EM_DECODER_OPER_IMM_FREG_BIT))

#define EM_DECODER_OPER_IMM_IREG(oi)                                     \
                            (((oi)->oper_flags) & EM_DECODER_OPER_IMM_IREG_BIT)
#define EM_DECODER_OPER_IMM_FREG(oi)                                     \
                            (((oi)->oper_flags) & EM_DECODER_OPER_IMM_FREG_BIT)



/************* EM_decoder Static Info Related macros ************/

/****** Macros receive pointer to modifiers ******/

#define EM_DECODER_MODIFIERS_CMP_TYPE(Mo) \
                            ((Mo)->cmp_type)

#define EM_DECODER_MODIFIERS_CMP_REL(Mo) \
                            ((Mo)->cmp_rel)

#define EM_DECODER_MODIFIERS_BRANCH_TYPE(Mo) \
                            ((Mo)->branch_type)

#define EM_DECODER_MODIFIERS_BRANCH_HINT(Mo) \
                            ((Mo)->branch_hint)
 
#define EM_DECODER_MODIFIERS_FP_PRECISION(Mo) \
                            ((Mo)->fp_precision)

#define EM_DECODER_MODIFIERS_FP_STATUS(Mo) \
                            ((Mo)->fp_status)

#define EM_DECODER_MODIFIERS_MEMORY_ACCESS_HINT(Mo) \
                            ((Mo)->mem_access_hint)


/****** Macros receive operand flags value ******/

#define EM_DECODER_OPER_FLAGS_2ND_ROLE_SRC(of) \
                            ((of) & EM_DECODER_OPER_2ND_ROLE_SRC_BIT)

#define EM_DECODER_OPER_FLAGS_2ND_ROLE_DST(of) \
                            ((of) & EM_DECODER_OPER_2ND_ROLE_DST_BIT)

#define EM_DECODER_OPER_FLAGS_IMM_REG(of) \
                            ((of) & (EM_DECODER_OPER_IMM_IREG_BIT | \
                            EM_DECODER_OPER_IMM_FREG_BIT))

#define EM_DECODER_OPER_FLAGS_IMM_IREG(of) \
                            ((of) & EM_DECODER_OPER_IMM_IREG_BIT)

#define EM_DECODER_OPER_FLAGS_IMM_FREG(of) \
                            ((of) & EM_DECODER_OPER_IMM_FREG_BIT)


/****** Macros receive pointer to operand ******/

#define EM_DECODER_OPER_STAT_2ND_ROLE_SRC(oi) \
                            EM_DECODER_OPER_FLAGS_2ND_ROLE_SRC((oi)->flags)

#define EM_DECODER_OPER_STAT_2ND_ROLE_DST(oi) \
                            EM_DECODER_OPER_FLAGS_2ND_ROLE_DST((oi)->flags)

#define EM_DECODER_OPER_STAT_IMM_REG(oi) \
                            EM_DECODER_OPER_FLAGS_IMM_REG((oi)->flags)

#define EM_DECODER_OPER_STAT_IMM_BREG(oi) \
                            EM_DECODER_OPER_FLAGS_IMM_BREG((oi)->flags)

#define EM_DECODER_OPER_STAT_IMM_IREG(oi) \
                            EM_DECODER_OPER_FLAGS_IMM_IREG((oi)->flags)
                                                  

#define EM_DECODER_OPER_STAT_IMM_FREG(oi) \
                            EM_DECODER_OPER_FLAGS_IMM_FREG((oi)->flags)



#define EM_DECODER_OPER_ROLE(oi) \
                            ((oi)->role)

#define EM_DECODER_OPER_TYPE(oi) \
                            ((oi)->type)

#define EM_DECODER_OPER_FLAGS(oi) \
                            ((oi)->flags)


/****** Macros receive instruction flags value ******/

#define EM_DECODER_FLAGS_FLAG_PRED(if) \
                            ((if) & EM_DECODER_BIT_PREDICATE)

#define EM_DECODER_FLAGS_FLAG_PRIVILEGED(if) \
                            ((if) & EM_DECODER_BIT_PRIVILEGED)

#define EM_DECODER_FLAGS_FLAG_LMEM(if) \
                            ((if) & EM_DECODER_BIT_LMEM)

#define EM_DECODER_FLAGS_FLAG_SMEM(if) \
                            ((if) & EM_DECODER_BIT_SMEM)

#define EM_DECODER_FLAGS_FLAG_CHECK_BASE_EQ_DST(if) \
                            ((if) & EM_DECODER_BIT_CHECK_BASE_EQ_DST)

#define EM_DECODER_FLAGS_FLAG_SPECULATION(if) \
                            ((if) & EM_DECODER_BIT_SPECULATION)

#define EM_DECODER_FLAGS_FLAG_POSTINCREMENT(if) \
                            ((if) & EM_DECODER_BIT_POSTINCREMENT)

#define EM_DECODER_FLAGS_FLAG_FALSE_PRED_EXEC(if) \
                            ((if) & EM_DECODER_BIT_FALSE_PRED_EXEC)

#define EM_DECODER_FLAGS_FLAG_BR_HINT(if) \
                            ((if) & EM_DECODER_BIT_BR_HINT)

#define EM_DECODER_FLAGS_FLAG_BR(if) \
                            ((if) & EM_DECODER_BIT_BR)

#define EM_DECODER_FLAGS_FLAG_ADV_LOAD(if) \
                            ((if) & EM_DECODER_CHECK_ADV_LOAD)

#define EM_DECODER_FLAGS_FLAG_GROUP_FIRST(if) \
                            ((if) & EM_DECODER_BIT_GROUP_FIRST)

#define EM_DECODER_FLAGS_FLAG_GROUP_LAST(if) \
                            ((if) & EM_DECODER_BIT_GROUP_LAST)

#define EM_DECODER_FLAGS_FLAG_CHECK_SAME_DSTS(if) \
                            ((if) & EM_DECODER_BIT_CHECK_SAME_DSTS)

#define EM_DECODER_FLAGS_FLAG_CONTROL_TRANSFER(if) \
                            ((if) & EM_DECODER_BIT_CONTROL_TRANSFER)


/****** Macros receive pointer to EM_decoder static info ******/

#define EM_DECODER_STATIC_MNEMONIC(si) \
                            ((si)->mnemonic)

#define EM_DECODER_STATIC_TEMPLATE_ROLE(si) \
                            ((si)->template_role)


#define EM_DECODER_STATIC_EXP_DST_ROLE(si, n) \
                            EM_DECODER_OPER_ROLE(((si)->explicit_dst) + (n))

#define EM_DECODER_STATIC_EXP_DST_TYPE(si, n) \
                            EM_DECODER_OPER_TYPE(((si)->explicit_dst) + (n))

#define EM_DECODER_STATIC_EXP_DST_FLAGS(si, n) \
                            EM_DECODER_OPER_FLAGS(((si)->explicit_dst) + (n))

#define EM_DECODER_STATIC_EXP_SRC_ROLE(si, n) \
                            EM_DECODER_OPER_ROLE(((si)->explicit_src) + (n))

#define EM_DECODER_STATIC_EXP_SRC_TYPE(si, n) \
                            EM_DECODER_OPER_TYPE(((si)->explicit_src) + (n))

#define EM_DECODER_STATIC_EXP_SRC_FLAGS(si, n) \
                            EM_DECODER_OPER_FLAGS(((si)->explicit_src) + (n))


#define EM_DECODER_STATIC_IMP_DST(si, n) \
                            ((si)->implicit_dst[(n)])

#define EM_DECODER_STATIC_IMP_SRC(si, n) \
                            ((si)->implicit_src[(n)])


#define EM_DECODER_STATIC_CMP_TYPE(si) \
                            EM_DECODER_MODIFIERS_CMP_TYPE(&((si)->modifiers))

#define EM_DECODER_STATIC_CMP_REL(si) \
                            EM_DECODER_MODIFIERS_CMP_REL(&((si)->modifiers))

#define EM_DECODER_STATIC_BRANCH_TYPE(si) \
                            EM_DECODER_MODIFIERS_BRANCH_TYPE(&((si)->modifiers))

#define EM_DECODER_STATIC_BRANCH_HINT(si) \
                            EM_DECODER_MODIFIERS_BRANCH_HINT(&((si)->modifiers))

#define EM_DECODER_STATIC_FP_PRECISION(si) \
                            EM_DECODER_MODIFIERS_FP_PRECISION(&((si)->modifiers))

#define EM_DECODER_STATIC_FP_STATUS(si) \
                            EM_DECODER_MODIFIERS_FP_STATUS(&((si)->modifiers))

#define EM_DECODER_STATIC_MEMORY_ACCESS_HINT(si) \
                            EM_DECODER_MODIFIERS_MEMORY_ACCESS_HINT(&((si)->modifiers))


#define EM_DECODER_STATIC_FLAGS(si) \
                            ((si)->flags)

#define EM_DECODER_STATIC_FLAG_PRED(si) \
                            EM_DECODER_FLAGS_FLAG_PRED((si)->flags)

#define EM_DECODER_STATIC_FLAG_PRIVILEGED(si) \
                            EM_DECODER_FLAGS_FLAG_PRIVILEGED((si)->flags)

#define EM_DECODER_STATIC_FLAG_LMEM(si) \
                            EM_DECODER_FLAGS_FLAG_LMEM((si)->flags)

#define EM_DECODER_STATIC_FLAG_SMEM(si) \
                            EM_DECODER_FLAGS_FLAG_SMEM((si)->flags)

#define EM_DECODER_STATIC_FLAG_CHECK_BASE_EQ_DST(si) \
                            EM_DECODER_FLAGS_FLAG_CHECK_BASE_EQ_DST((si)->flags)

#define EM_DECODER_STATIC_FLAG_SPECULATION(si) \
                            EM_DECODER_FLAGS_FLAG_SPECULATION((si)->flags)

#define EM_DECODER_STATIC_FLAG_POSTINCREMENT(si) \
                            EM_DECODER_FLAGS_FLAG_POSTINCREMENT((si)->flags)

#define EM_DECODER_STATIC_FLAG_FALSE_PRED_EXEC(si) \
                            EM_DECODER_FLAGS_FLAG_FALSE_PRED_EXEC((si)->flags)

#define EM_DECODER_STATIC_FLAG_BR_HINT(si) \
                            EM_DECODER_FLAGS_FLAG_BR_HINT((si)->flags)

#define EM_DECODER_STATIC_FLAG_BR(si) \
                            EM_DECODER_FLAGS_FLAG_BR((si)->flags)

#define EM_DECODER_STATIC_FLAG_GROUP_FIRST(si) \
                            EM_DECODER_FLAGS_FLAG_GROUP_FIRST((si)->flags)

#define EM_DECODER_STATIC_FLAG_GROUP_LAST(si) \
                            EM_DECODER_FLAGS_FLAG_GROUP_LAST((si)->flags)

#define EM_DECODER_STATIC_FLAG_CHECK_SAME_DSTS(si) \
                            EM_DECODER_FLAGS_FLAG_CHECK_SAME_DSTS((si)->flags)

#define EM_DECODER_STATIC_FLAG_CONTROL_TRANSFER(si) \
                            EM_DECODER_FLAGS_FLAG_CONTROL_TRANSFER((si)->flags)

#ifdef __cplusplus
}
#endif

#endif /*** EM_DECODER_H ***/









