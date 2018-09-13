;++
;
;   Copyright (c) 1989	Microsoft Corporation
;
;   Module Name:
;
;       MI.INC
;
;   Abstract:
;
;       This module contains equates for x86 machine instructions
;
;   Author:
;
;       Dave Hastings 2 May 1991
;
;   Notes:
;
;       This information used to reside in Trap.asm, but is now needed in
;       multiple source files.
;
;   Revision History:
;--


MAX_INSTRUCTION_LENGTH		EQU	15
MAX_INSTRUCTION_PREFIX_LENGTH	EQU	4
MI_LOCK_PREFIX			EQU	0F0H
MI_ADDR_PREFIX			EQU	067H
MI_TWO_BYTE			EQU	0FH
MI_HLT				EQU	0F4H
MI_LTR_LLDT			EQU	0
MI_LGDT_LIDT_LMSW		EQU	01H
MI_MODRM_MASK			EQU	38H
MI_LLDT_MASK			EQU	10h
MI_LTR_MASK			EQU	18H
MI_LGDT_MASK			EQU	10H
MI_LIDT_MASK			EQU	18H
MI_LMSW_MASK			EQU	30H
MI_SPECIAL_MOV_MASK		EQU	20H
MI_REP_INS_OUTS 		EQU	0F3H
MI_MIN_INS_OUTS 		EQU	06CH
MI_MAX_INS_OUTS 		EQU	06FH

MI_LMSW_OPCODE                  EQU     001H ; second byte of lmsw
MI_CLTS_OPCODE                  EQU     006H ; second byte of clts
MI_GET_CRx_OPCODE               EQU     020H ; mov r32,CRx
MI_SET_CRx_OPCODE               EQU     022h ; mov CRx,r32
MI_GET_TRx_OPCODE               EQU     024H ; mov r32,TRx
MI_SET_TRx_OPCODE               EQU     026H ; mov TRx,r32

MI_REGMASK                      EQU     038H ; REG field mask
MI_REGSHIFT                     EQU     3    ; REG field shift
MI_REGLMSW                      EQU     030h ; REG field for lmsw

MI_MODMASK                      EQU     0C0H ; MOD field mask
MI_MODSHIFT                     EQU     6    ; MOD field shift
MI_MODMOVSPEC                   EQU     0C0H ; MOD field for mov to/from special
MI_MODNONE                      EQU     0

MI_RMMASK                       EQU     007H ; RM field mask
MI_RMBP                         EQU     006H ; RM value for bp reg
MI_RMSIB                        EQU     004H ; RM value for sib

MI_SIB_BASEMASK                 EQU     007H ; SIB BASE field mask
MI_SIB_BASENONE                 EQU     005H
MI_SIB_BASESHIFT                EQU     0

MI_SIB_INDEXMASK                EQU     038H
MI_SIB_INDEXSHIFT               EQU     3
MI_SIB_INDEXNONE                EQU     020H

MI_SIB_SSMASK                   EQU     0c0H
MI_SIB_SSSHIFT                  EQU     6
