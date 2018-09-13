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
