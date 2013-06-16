/*
 * softx86.c
 *
 * Copyright (C) 2003, 2004 Jonathan Campbell <jcampbell@mdjk.com>
 * 
 * Softx86 library API.
 *
 * Initialization, de-initialization, register modification, and default
 * routines for callbacks (in case they weren't set by the host app).
 *
 * Internal to this library, there are also functions to take care of
 * fetching the current opcode (as referred to by CS:IP), fetching
 * the current opcode for the decompiler (as referred to by a separate
 * pair CS:IP), and pushing/popping data to/from the stack (SS:SP).
 * 
 ***********************************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 ************************************************************************************/

#include <softx86.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include "optable.h"

/* softx86_setsegment(context,segid,val)
   Sets a segment register and all hidden register information */
int softx86_setsegval(softx86_ctx* ctx,int sreg_id,sx86_udword x)
{
	if (sreg_id > 7 || sreg_id < 0) return 0;

/* TODO: In 286/386 protected mode fetch limit from GDT/LDT */
	ctx->state->segment_reg[sreg_id].val =			x;
	ctx->state->segment_reg[sreg_id].cached_linear =	SEGMENT_TO_LINEAR(x);
	ctx->state->segment_reg[sreg_id].cached_limit =		0xFFFF;

	return 1;		/* success */
}

/* softx86_reset(context)
   Resets a CPU

   return value:
   0 = failed
   1 = success */
int softx86_reset(softx86_ctx* ctx)
{
	if (!ctx) return 0;

	if (ctx->__private->level >= SX86_CPULEVEL_80286)
		ctx->__private->addr_mask = 0xFFFFFF;	/* 80286 24-bit addressing */
	else
		ctx->__private->addr_mask = 0xFFFFF;	/* 8086/8088/80186 20-bit addressing */

	ctx->state->general_reg[SX86_REG_AX].val	= 0;
	ctx->state->general_reg[SX86_REG_BX].val	= 0;
	ctx->state->general_reg[SX86_REG_CX].val	= 0;
	ctx->state->general_reg[SX86_REG_DX].val	= 0;
	ctx->state->general_reg[SX86_REG_SI].val	= 0;
	ctx->state->general_reg[SX86_REG_DI].val	= 0;
	ctx->state->general_reg[SX86_REG_SP].val	= 0;
	ctx->state->general_reg[SX86_REG_BP].val	= 0;
	ctx->state->reg_flags.val			= SX86_CPUFLAG_RESERVED_01;
	softx86_setsegval(ctx,SX86_SREG_DS,0xF000);
	softx86_setsegval(ctx,SX86_SREG_ES,0xF000);
	softx86_setsegval(ctx,SX86_SREG_SS,0xF000);

	ctx->__private->ptr_regs_8reg[0] =	(sx86_ubyte*)(&ctx->state->general_reg[SX86_REG_AX].b.lo);	// AL
	ctx->__private->ptr_regs_8reg[1] =	(sx86_ubyte*)(&ctx->state->general_reg[SX86_REG_CX].b.lo);	// CL
	ctx->__private->ptr_regs_8reg[2] =	(sx86_ubyte*)(&ctx->state->general_reg[SX86_REG_DX].b.lo);	// DL
	ctx->__private->ptr_regs_8reg[3] =	(sx86_ubyte*)(&ctx->state->general_reg[SX86_REG_BX].b.lo);	// BL
	ctx->__private->ptr_regs_8reg[4] =	(sx86_ubyte*)(&ctx->state->general_reg[SX86_REG_AX].b.hi);	// AH
	ctx->__private->ptr_regs_8reg[5] =	(sx86_ubyte*)(&ctx->state->general_reg[SX86_REG_CX].b.hi);	// CH
	ctx->__private->ptr_regs_8reg[6] =	(sx86_ubyte*)(&ctx->state->general_reg[SX86_REG_DX].b.hi);	// DH
	ctx->__private->ptr_regs_8reg[7] =	(sx86_ubyte*)(&ctx->state->general_reg[SX86_REG_BX].b.hi);	// BH

/* initial CPU instruction pointer in BIOS area on older machines: 0xF000:0xFFF0 (as far as I know) */
	softx86_setsegval(ctx,SX86_SREG_CS,0xF000);
	ctx->state->reg_ip =			0xFFF0;
	ctx->state->int_hw_flag =		0;
	ctx->state->int_nmi =			0;

/* switch on/off bugs */
	ctx->bugs->preemptible_after_prefix = 	(ctx->__private->level <= SX86_CPULEVEL_8086)?1:0;
	ctx->bugs->decrement_sp_before_store =	(ctx->__private->level <= SX86_CPULEVEL_80186)?1:0;
	ctx->bugs->mask_5bit_shiftcount = 	(ctx->__private->level >= SX86_CPULEVEL_80286)?1:0;

/* CPU mode flags */
	ctx->state->reg_cr0 =			0;

/* opcode table */
	ctx->__private->opcode_table =		(void*)(&optab8086);
	ctx->__private->opcode_table_sub_0Fh =	(void*)(&optab8086_0Fh);

/* we were just reset */
	ctx->callbacks->on_reset(ctx);

	return 1; /* success */
}

int softx86_getversion(int *major,int *minor,int *subminor)
{
	if (!minor || !major || !subminor) return 0;

	*major =	SOFTX86_VERSION_HI;
	*minor =	SOFTX86_VERSION_LO;
	*subminor =	SOFTX86_VERSION_SUBLO;
	return 1;
}

/* softx86_init(context)
   Initialize a CPU context structure.

   return value:
   0 = failed
   1 = success
   2 = beta development advisory (CPU level emulation not quite stable) */
int softx86_init(softx86_ctx* ctx,int level)
{
	int ret;

	ret=1;
	if (!ctx) return 0;
	if (level > SX86_CPULEVEL_80286) return 0;	/* we currently support up to the 80286 */
	if (level < 0) return 0;			/* apparently the host wants an 80(-1)86? :) */
	if (level > SX86_CPULEVEL_8086) ret=2;		/* 80186 or higher emulation is not stable yet */

/* set up pointers */
	ctx->__private =	(softx86_internal*)malloc(sizeof(softx86_internal));
	ctx->bugs =		(softx86_bugs*)malloc(sizeof(softx86_bugs));
	ctx->callbacks =	(softx86_callbacks*)malloc(sizeof(softx86_callbacks));
	ctx->state =		(softx86_cpustate*)malloc(sizeof(softx86_cpustate));
	if ((!ctx->__private) || (!ctx->bugs) || (!ctx->callbacks) || (!ctx->state)) {
		softx86_free(ctx);
		return 0;
	}

/* store settings */
	ctx->__private->level =			level;

/* store version in the structure */
	ctx->version_hi =			SOFTX86_VERSION_HI;
	ctx->version_lo =			SOFTX86_VERSION_LO;
	ctx->version_sublo =			SOFTX86_VERSION_SUBLO;

/* default callbacks */
	ctx->callbacks->on_read_io		= softx86_step_def_on_read_io;
	ctx->callbacks->on_read_memory		= softx86_step_def_on_read_memory;
	ctx->callbacks->on_write_io		= softx86_step_def_on_write_io;
	ctx->callbacks->on_write_memory		= softx86_step_def_on_write_memory;
	ctx->callbacks->on_hw_int		= softx86_step_def_on_hw_int;
	ctx->callbacks->on_hw_int_ack		= softx86_step_def_on_hw_int_ack;
	ctx->callbacks->on_sw_int		= softx86_step_def_on_sw_int;
	ctx->callbacks->on_idle_cycle		= softx86_step_def_on_idle_cycle;
	ctx->callbacks->on_nmi_int		= softx86_step_def_on_nmi_int;
	ctx->callbacks->on_nmi_int_ack		= softx86_step_def_on_nmi_int_ack;
	ctx->callbacks->on_fpu_opcode_dec	= softx86_step_def_on_fpu_opcode_dec;
	ctx->callbacks->on_fpu_opcode_exec	= softx86_step_def_on_fpu_opcode_exec;
	ctx->callbacks->on_reset		= softx86_step_def_on_reset;

/* default pointers */
	ctx->ref_softx87_ctx			= NULL;

/* now reset */
	if (!softx86_reset(ctx)) return 0;

	return ret; /* success */
}

/* softx86_free(context)
   Free a CPU context structure */
int softx86_free(softx86_ctx* ctx)
{
	if (!ctx) return 0;
	if (ctx->__private)	free(ctx->__private);
	if (ctx->bugs)		free(ctx->bugs);
	if (ctx->callbacks)	free(ctx->callbacks);
	if (ctx->state)		free(ctx->state);
	ctx->__private =	NULL;
	ctx->bugs =		NULL;
	ctx->callbacks =	NULL;
	ctx->state =		NULL;
	return 1; /* success */
}

/* fetch byte at executioneer CS:IP, increment pointer */
sx86_ubyte softx86_fetch_exec_byte(softx86_ctx* ctx)
{
	sx86_ubyte opcode;

	if (!ctx) return 0;

	softx86_fetch(ctx,NULL,ctx->state->segment_reg[SX86_SREG_CS].cached_linear+ctx->state->reg_ip,&opcode,1);
	ctx->state->reg_ip++;
/* TODO: If IP passes 0xFFFF generate an exception */

	return opcode;
}

/* fetch byte at decompiler CS:IP, increment pointer */
sx86_ubyte softx86_fetch_dec_byte(softx86_ctx* ctx)
{
	sx86_ubyte opcode;

	if (!ctx) return 0;

	softx86_fetch(ctx,NULL,SEGMENT_TO_LINEAR(ctx->state->reg_cs_decompiler)+(ctx->state->reg_ip_decompiler),&opcode,1);
	ctx->state->reg_ip_decompiler++;
	return opcode;
}

/* retrieve a word value from the stack pointer SS:SP */
sx86_uword softx86_stack_popw(softx86_ctx* ctx)
{
	sx86_uword data;

	if (!ctx) return 0;

/* TODO: If SP == 0xFFFF generate an exception */
	softx86_fetch(ctx,NULL,ctx->state->segment_reg[SX86_SREG_SS].cached_linear+ctx->state->general_reg[SX86_REG_SP].w.lo,&data,2);
	SWAP_WORD_FROM_LE(data);
	ctx->state->general_reg[SX86_REG_SP].w.lo += 2;
/* TODO: If SP passes 0xFFFF generate an exception */

	return data;
}

/* discard n bytes from stack pointer SS:SP */
void softx86_stack_discard_n(softx86_ctx* ctx,int bytez)
{
	if (!ctx) return;

	ctx->state->general_reg[SX86_REG_SP].w.lo += bytez;
/* TODO: If SP passes 0xFFFF generate an exception */
}

/* adds n bytes to stack pointer SS:SP */
void softx86_stack_add_n(softx86_ctx* ctx,int bytez)
{
	if (!ctx) return;

	ctx->state->general_reg[SX86_REG_SP].w.lo -= bytez;
/* TODO: If SP passes 0xFFFF generate an exception */
}

/* store a word value to the stack pointer SS:SP */
void softx86_stack_pushw(softx86_ctx* ctx,sx86_uword data)
{
	SWAP_WORD_TO_LE(data);
/* TODO: If SP == 0x0001 generate an exception */
	ctx->state->general_reg[SX86_REG_SP].w.lo -= 2;
	softx86_write(ctx,NULL,ctx->state->segment_reg[SX86_SREG_SS].cached_linear+ctx->state->general_reg[SX86_REG_SP].w.lo,&data,2);
}

/* set stack pointer SS:SP */
int softx86_set_stack_ptr(softx86_ctx* ctx,sx86_udword ss,sx86_udword sp)
{
	if (!ctx) return 0;

/* TODO: Check SS:SP for validity */
	ctx->state->general_reg[SX86_REG_SP].w.lo = sp;
	softx86_setsegval(ctx,SX86_SREG_SS,ss);

	return 1;
}

/* set executioneer CS:IP */
int softx86_set_instruction_ptr(softx86_ctx* ctx,sx86_udword cs,sx86_udword ip)
{
	if (!ctx) return 0;

/* TODO: Check CS:IP for validity */
	ctx->state->reg_ip = ip;
	softx86_setsegval(ctx,SX86_SREG_CS,cs);

	return 1;
}

/* set executioneer IP (not CS). used for emulating near jumps. */
int softx86_set_near_instruction_ptr(softx86_ctx* ctx,sx86_udword ip)
{
	if (!ctx) return 0;

/* TODO: Check IP for validity */
	ctx->state->reg_ip = ip;

	return 1;
}

/* set decompiler CS:IP */
int softx86_set_instruction_dec_ptr(softx86_ctx* ctx,sx86_udword cs,sx86_udword ip)
{
	if (!ctx) return 0;

/* TODO: Check CS:IP for validity */
	ctx->state->reg_ip_decompiler = ip;
	ctx->state->reg_cs_decompiler = cs;

	return 1;
}

/* create a stack frame for simple near procedural calls and set the instruction pointer (if ip != NULL) */
int softx86_make_simple_near_call(softx86_ctx* ctx,sx86_udword* ip)
{
	if (!ctx) return 0;

	softx86_stack_pushw(ctx,ctx->state->reg_ip);
	if (ip != NULL) return softx86_set_near_instruction_ptr(ctx,*ip);
	return 1;
}

/* create a stack frame for simple far procedural calls and set the instruction pointer (if ip != NULL) */
int softx86_make_simple_far_call(softx86_ctx* ctx,sx86_udword* cs,sx86_udword* ip)
{
	if (!ctx) return 0;

	softx86_stack_pushw(ctx,ctx->state->segment_reg[SX86_SREG_CS].val);
	softx86_stack_pushw(ctx,ctx->state->reg_ip);
	if (cs != NULL && ip != NULL) return softx86_set_instruction_ptr(ctx,*cs,*ip);
	return 1;
}

/* create a stack frame for simple interrupt procedural calls and set the instruction pointer (if ip != NULL) */
int softx86_make_simple_interrupt_call(softx86_ctx* ctx,sx86_udword* cs,sx86_udword* ip)
{
	if (!ctx) return 0;

	softx86_stack_pushw(ctx,ctx->state->reg_flags.w.lo);
	softx86_stack_pushw(ctx,ctx->state->segment_reg[SX86_SREG_CS].val);
	softx86_stack_pushw(ctx,ctx->state->reg_ip);
	if (cs != NULL && ip != NULL) return softx86_set_instruction_ptr(ctx,*cs,*ip);
	return 1;
}

/* retrieves an interrupt vector */
int softx86_get_intvect(softx86_ctx* ctx,sx86_ubyte i,sx86_uword *seg,sx86_uword *ofs)
{
	sx86_uword _seg,_ofs;
	sx86_udword o;

	if (!ctx) return 0;

/* obtain the interrupt vector */
	o = (i<<2);
	softx86_fetch(ctx,NULL,o,(void*)(&_ofs),2);
	SWAP_WORD_FROM_LE(_ofs);
	*ofs = _ofs;

	o += 2;
	softx86_fetch(ctx,NULL,o,(void*)(&_seg),2);
	SWAP_WORD_FROM_LE(_seg);
	*seg = _seg;

	return 1;
}

/* sets an interrupt vector */
int softx86_set_intvect(softx86_ctx* ctx,sx86_ubyte i,sx86_uword seg,sx86_uword ofs)
{
	sx86_udword o;

	if (!ctx) return 0;

/* obtain the interrupt vector */
	o = (i<<2);
	SWAP_WORD_TO_LE(ofs);
	softx86_write(ctx,NULL,o,(void*)(&ofs),2);

	o += 2;
	SWAP_WORD_TO_LE(seg);
	softx86_write(ctx,NULL,o,(void*)(&seg),2);

	return 1;
}

/* create interrupt frame and manipulate registers */
void softx86_go_int_frame(softx86_ctx* ctx,sx86_ubyte i)
{
	sx86_uword seg,ofs;

/* get interrupt vector */
	if (!softx86_get_intvect(ctx,i,&seg,&ofs)) return;

/* save old address */
	softx86_stack_pushw(ctx,ctx->state->reg_flags.val);
	softx86_stack_pushw(ctx,ctx->state->segment_reg[SX86_SREG_CS].val);
	softx86_stack_pushw(ctx,ctx->state->reg_ip);

/* disable interrupts */
	ctx->state->reg_flags.val &= ~SX86_CPUFLAG_INTENABLE;

/* go! */
	if (!softx86_set_instruction_ptr(ctx,seg,ofs)) return;
}

/* enable/disable emulation of specific bugs. */
int softx86_setbug(softx86_ctx* ctx,sx86_udword bug_id,sx86_ubyte on_off)
{
	if (bug_id == SX86_BUG_PREEMPTIBLE_AFTER_PREFIX) {
		ctx->bugs->preemptible_after_prefix = on_off;
		return 1;
	}
	else if (bug_id == SX86_BUG_SP_DECREMENT_BEFORE_STORE) {
		ctx->bugs->decrement_sp_before_store = on_off;
	}

	return 0;
}

/* simulate external hardware interrupt from host app. */
int softx86_ext_hw_signal(softx86_ctx* ctx,sx86_ubyte i)
{
	if (!ctx) return 0;

/* make sure the poor CPU isn't getting deluged by interrupts.
   as far as I know, every system using the x86 with an interrupt
   system has some sort of moderator for the #int line. The x86 PC
   platform for instance, has a "programmable interrupt controller"
   that is responsible for handling 16 interrupt signals (IRQ 0
   thru 15) and presenting them one at a time to the CPU. */
	if (ctx->state->int_hw_flag) return 0;

	ctx->callbacks->on_hw_int(ctx,i);
	ctx->state->int_hw_flag = 1;
	ctx->state->int_hw      = i;

	return 1;
}

/* simulate external hardware NMI interrupt from host app. */
int softx86_ext_hw_nmi_signal(softx86_ctx* ctx)
{
	if (!ctx) return 0;

/* only one #NMI at a time! */
/* TODO: Is the #NMI interrupt number fixed or variable? */
	if (ctx->state->int_nmi) return 0;

	ctx->state->int_nmi = 1;
	ctx->callbacks->on_nmi_int(ctx);

	return 1;
}

/* simulate internal software interrupt. */
int softx86_int_sw_signal(softx86_ctx* ctx,sx86_ubyte i)
{
	if (!ctx) return 0;

	ctx->callbacks->on_sw_int(ctx,i);
	softx86_go_int_frame(ctx,i);

	return 1;
}

/* INTERNAL USE: used by this library to send acknowledgement back
                 to host application for the given interrupt signal. */
int softx86_ext_hw_ack(softx86_ctx* ctx)
{
	if (!ctx) return 0;
	if (!ctx->state->int_hw_flag) return 0;

	ctx->callbacks->on_hw_int_ack(ctx,ctx->state->int_hw);
	ctx->state->int_hw_flag = 0;
	ctx->state->int_hw      = 0;

	return 1;
}

/* INTERNAL USE: used by this library to send acknowledgement back
                 to host application for the given interrupt signal. */
int softx86_ext_hw_nmi_ack(softx86_ctx* ctx)
{
	if (!ctx) return 0;
	if (!ctx->state->int_nmi) return 0;

	ctx->callbacks->on_nmi_int_ack(ctx);
	ctx->state->int_nmi = 0;

	return 1;
}

/* softx86_step(context)
   Executes ONE instruction at CS:IP

   return value:
   0 = failed
   1 = success */
int softx86_step(softx86_ctx* ctx)
{
	Sfx86OpcodeTable* sop;
	sx86_ubyte opcode;
	int lp,x,restart,err,count;
	int can_hwint;

	if (!ctx) return 0;
	sop = (Sfx86OpcodeTable*)(ctx->__private->opcode_table);
	if (!sop) return 0;

	ctx->state->is_segment_override = 0;
	ctx->state->segment_override    = 0;
	ctx->state->rep_flag            = 0;
	ctx->state->reg_cs_exec_initial = ctx->state->segment_reg[SX86_SREG_CS].val;
	ctx->state->reg_ip_exec_initial = ctx->state->reg_ip;

	lp = 1;
	err = 0;
	count = 16;
	restart = 0;
	can_hwint = 1;
	while (lp && count-- > 0) {
/* if now is a good time to allow an interrupt and interrupts enabled, so be it */
		if (can_hwint || ctx->bugs->preemptible_after_prefix) {
			if (ctx->state->int_nmi) {	/* #NMI pin has priority */
				softx86_go_int_frame(ctx,2);
				softx86_ext_hw_nmi_ack(ctx);
			}
			else if (ctx->state->reg_flags.val & SX86_CPUFLAG_INTENABLE) {
				if (ctx->state->int_hw_flag) {
					softx86_go_int_frame(ctx,ctx->state->int_hw);
					softx86_ext_hw_ack(ctx);
				}
			}

			can_hwint=0;
		}

/* fetch opcode */
		opcode = softx86_fetch_exec_byte(ctx);
/* execute opcode */
		x      = sop->table[opcode].exec(opcode,ctx);
/* what happened? */
		if (x == 0) {			/* opcode not recognized? stop! */
			lp = 0;
			restart = 1;
			err = 1;
		}
		else if (x == 1) {		/* opcode recognized? */
			lp = 0;
		}
		else if (x == 2) {		/* opcode recognized but more parsing needed (i.e. instruction prefix)? */
		}
		else if (x == 3) {		/* opcode recognized, looping action in progress do not advance (i.e. REP MOVSB or HLT/WAIT)? */
			restart = 1;
			lp = 0;
		}
		else {				/* this indicates a bug in this library */
			lp = 0;
			restart = 1;		/* huh? */
			err = 1;
		}
	}

	if (count <= 0) {
		err = 1;
		restart = 1;
	}

	if (restart) {
		ctx->state->segment_reg[SX86_SREG_CS].val = ctx->state->reg_cs_exec_initial;
		ctx->state->reg_ip = ctx->state->reg_ip_exec_initial;
	}

	return !err;		/* success */
}

/* softx86_decompile(context)
   Decompiles ONE instruction at decoder CS:IP

   return value:
   0 = failed
   1 = success */
int softx86_decompile(softx86_ctx* ctx,char asmbuf[256])
{
	Sfx86OpcodeTable* sop;
	sx86_ubyte opcode;
	int lp,x,err,count,asmx;
	char buf[256];

	if (!ctx) return 0;
	sop = (Sfx86OpcodeTable*)(ctx->__private->opcode_table);
	if (!sop) return 0;

	lp = 1;
	err = 0;
	asmx = 0;
	asmbuf[0] = 0;
	count = 16;
	while (lp && count-- > 0) {
/* fetch opcode */
		opcode = softx86_fetch_dec_byte(ctx);
/* execute opcode */
		x      = sop->table[opcode].dec(opcode,ctx,buf);
/* what happened? */
		if (x == 0) {			/* opcode not recognized? stop! */
			lp = 0;
			err = 1;
		}
		else if (x == 1) {		/* opcode recognized? */
			strcpy(asmbuf+asmx,buf);
			lp = 0;
		}
		else if (x == 2) {		/* opcode recognized but more parsing needed? */
			strcpy(asmbuf+asmx,buf); /* this is why asmbuf[] is 256 bytes :) */
			asmbuf += strlen(asmbuf+asmx);
		}
		else {				/* this indicates a bug in this library */
			lp = 0;
			err = 1;
		}
	}

	if (count <= 0) {
		err = 1;
	}

	return !err;		/* success */
}

/* softx86_decompile_exec_cs_ip(context)
   Point decompiler instructor ptr to where the executioneer CS:IP is */
int softx86_decompile_exec_cs_ip(softx86_ctx* ctx)
{
	if (!ctx) return 0;

/* set decompiler CS:IP to executioneer CS:IP */
	ctx->state->reg_cs_decompiler = ctx->state->segment_reg[SX86_SREG_CS].val;
	ctx->state->reg_ip_decompiler = ctx->state->reg_ip;

	return 1;		/* success */
}

/*--------------------------------------------------------------------------------
  default callbacks
  --------------------------------------------------------------------------------*/

void softx86_step_def_on_read_memory(void* _ctx,sx86_udword address,sx86_ubyte *buf,int size)
{
	softx86_ctx *ctx = ((softx86_ctx*)_ctx);
	if (!ctx || !buf || size < 1) return;
	memset(buf,0xFF,size);
}

void softx86_step_def_on_read_io(void* _ctx,sx86_udword address,sx86_ubyte *buf,int size)
{
	softx86_ctx *ctx = ((softx86_ctx*)_ctx);
	if (!ctx || !buf || size < 1) return;
	memset(buf,0xFF,size);
}

void softx86_step_def_on_write_memory(void* _ctx,sx86_udword address,sx86_ubyte *buf,int size)
{
	softx86_ctx *ctx = ((softx86_ctx*)_ctx);
	if (!ctx || !buf || size < 1) return;
}

void softx86_step_def_on_write_io(void* _ctx,sx86_udword address,sx86_ubyte *buf,int size)
{
	softx86_ctx *ctx = ((softx86_ctx*)_ctx);
	if (!ctx || !buf || size < 1) return;
}

void softx86_step_def_on_hw_int(void* _ctx,sx86_ubyte i)
{
}

void softx86_step_def_on_sw_int(void* _ctx,sx86_ubyte i)
{
}

void softx86_step_def_on_hw_int_ack(void* _ctx,sx86_ubyte i)
{
}

void softx86_step_def_on_idle_cycle(void* _ctx)
{
}

void softx86_step_def_on_nmi_int(void* _ctx)
{
}

void softx86_step_def_on_nmi_int_ack(void* _ctx)
{
}

int softx86_step_def_on_fpu_opcode_exec(void* _ctx86,void* _ctx87,sx86_ubyte opcode)
{
	return 0;
}

int softx86_step_def_on_fpu_opcode_dec(void* _ctx86,void* _ctx87,sx86_ubyte opcode,char buf[128])
{
	return 0;
}

void softx86_step_def_on_reset(/* softx86_ctx */ void* _ctx)
{
}

/*--------------------------------------------------------------------------------
  API for CPU itself to fetch data through a cache.
  --------------------------------------------------------------------------------*/
int softx86_fetch(softx86_ctx* ctx,void *reserved,sx86_udword addr,void *_buf,int sz)
{
	sx86_ubyte *buf = (sx86_ubyte*)(_buf);
	int rsz;
	sx86_udword raddr;

	if (!ctx || !buf || sz < 1) return 0;

	rsz = sz;
	raddr = addr;

/* TODO: fetching from cache can be implemented at this point, filling in by reassigning ratter, buf and rsz. */

/* save host application the aggrivation of dealing with 32-bit address wrap-arounds */
	if ((raddr+rsz) > 0 && (raddr+rsz) < rsz) {
		ctx->callbacks->on_read_memory(ctx,raddr,buf,(sx86_udword)((0x100000000L-raddr)&0xFFFFFFFF));
		raddr += (sx86_udword)((0x100000000L-raddr)&0xFFFFFFFF);
		rsz   -= (sx86_udword)((0x100000000L-raddr)&0xFFFFFFFF);
		buf   += (sx86_udword)((0x100000000L-raddr)&0xFFFFFFFF);
		ctx->callbacks->on_read_memory(ctx,raddr,buf,rsz);
		return 1;
	}

	ctx->callbacks->on_read_memory(ctx,raddr,buf,rsz);

	return 1;
}

int softx86_write(softx86_ctx* ctx,void *reserved,sx86_udword addr,void *_buf,int sz)
{
	sx86_ubyte *buf = (sx86_ubyte*)(_buf);
	int rsz;
	sx86_udword raddr;

	if (!ctx || !buf || sz < 1) return 0;

	rsz = sz;
	raddr = addr;

/* TODO: writing to/through the cache can be implemented at this point, filling in by reassigning ratter, buf and rsz. */

/* save host application the aggrivation of dealing with 32-bit address wrap-arounds */
	if ((raddr+rsz) > 0 && (raddr+rsz) < rsz) {
		ctx->callbacks->on_write_memory(ctx,raddr,buf,(sx86_udword)((0x100000000L-raddr)&0xFFFFFFFF));
		raddr += (sx86_udword)((0x100000000L-raddr)&0xFFFFFFFF);
		rsz   -= (sx86_udword)((0x100000000L-raddr)&0xFFFFFFFF);
		buf   += (sx86_udword)((0x100000000L-raddr)&0xFFFFFFFF);
		ctx->callbacks->on_write_memory(ctx,raddr,buf,rsz);
		return 1;
	}

	ctx->callbacks->on_write_memory(ctx,raddr,buf,rsz);

	return 1;
}

void softx86_bswap2(sx86_ubyte *x)
{
	sx86_ubyte t;

	t=x[0]; x[0]=x[1]; x[1]=t;
}

void softx86_bswap4(sx86_ubyte *x)
{
	sx86_ubyte t;

	t=x[0]; x[0]=x[3]; x[3]=t;
	t=x[1]; x[1]=x[2]; x[2]=t;
}

/* general purpose mod/reg/rm executioneer helper for LEA.

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   d32 = 386+ 32-bit data size override
   mod/reg/rm = mod/reg/rm unpacked byte

   Example of instructions that fit this format:

   LEA AX,[AX+2]	(apparently a substitute for add ax,2?)
   LEA BX,[SP]          (seen sometimes as an alternative to mov bx,sp)
   LEA EBX,[ESI+ECX]    (surprisingly, often seen in Win32 code)
*/
void sx86_exec_full_modregrm_lea(softx86_ctx* ctx,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm)
{
	if (mod == 3) {		/* register <- register */
		/* TODO: Invalid opcode exception. */
	}
	else {			/* register <- memory */
		sx86_uword ofs;

/* don't calculate segment value because LEA does not fetch memory, it only stores
   the offset into the destination register */

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		ctx->state->general_reg[reg].w.lo = ofs;
	}
}

/* general purpose mod/reg/rm executioneer helper for
   instructions that modify the destination operand.

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   w16 = 16-bit operand flag
   d32 = 386+ 32-bit data size override (ignored if w16 == 0)
   mod/reg/rm = mod/reg/rm unpacked byte
   op8 = function to call for emulation of instruction, given 8-bit operands "src" and "dst"
   op16 = function to call for emulation of instruction, given 16-bit operands "src" and "dst"
   op32 = function to call for emulation of instruction, given 32-bit operands "src" and "dst"

   Example of instructions that fit this format:

   MOV AX,CX
   MOV WORD PTR [3456h],DX
   MOV SI,DI
   MOV BX,WORD PTR [1246h]
*/
void sx86_exec_full_modregrm_rw(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,sx86_ubyte opswap,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val))
{
	if (mod == 3) {		/* register <-> register */
		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword regv,rmv;

			regv = ctx->state->general_reg[reg].w.lo;
			rmv = ctx->state->general_reg[rm].w.lo;

			if (opswap)
				 ctx->state->general_reg[reg].w.lo =
					op16(ctx,regv,rmv);
			else
				 ctx->state->general_reg[rm].w.lo =
					op16(ctx,rmv,regv);
		}
		else {
			sx86_ubyte regv,rmv;

			regv = *(ctx->__private->ptr_regs_8reg[reg]);
			rmv = *(ctx->__private->ptr_regs_8reg[rm]);

			if (opswap)
				*(ctx->__private->ptr_regs_8reg[reg]) =
					op8(ctx,regv,rmv);
			else
				*(ctx->__private->ptr_regs_8reg[rm]) =
					op8(ctx,rmv,regv);
		}
	}
	else {			/* register <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword regv,rmv;

			regv = ctx->state->general_reg[reg].w.lo;
			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,2);
			SWAP_WORD_FROM_LE(rmv);

			if (opswap) {
				ctx->state->general_reg[reg].w.lo =
					op16(ctx,regv,rmv);
			}
			else {
				rmv = op16(ctx,rmv,regv);
				SWAP_WORD_TO_LE(rmv);
				softx86_write(ctx,NULL,lo,&rmv,2);
			}
		}
		else {
			sx86_ubyte regv,rmv;

			regv = *(ctx->__private->ptr_regs_8reg[reg]);
			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,1);

			if (opswap) {
				*(ctx->__private->ptr_regs_8reg[reg]) =
					op8(ctx,regv,rmv);
			}
			else {
				rmv = op8(ctx,rmv,regv);
				softx86_write(ctx,NULL,lo,&rmv,1);
			}
		}
	}
}

/* general purpose mod/reg/rm executioneer helper for
   instructions that do not modify the destination operand.

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   w16 = 16-bit operand flag
   d32 = 386+ 32-bit data size override (ignored if w16 == 0)
   mod/reg/rm = mod/reg/rm unpacked byte
   op8 = function to call for emulation of instruction, given 8-bit operands "src" and "dst"
   op16 = function to call for emulation of instruction, given 16-bit operands "src" and "dst"
   op32 = function to call for emulation of instruction, given 32-bit operands "src" and "dst"

   Example of instructions that fit this format:

   TEST AX,CX
   TEST WORD PTR [3456h],DX
   TEST SI,DI
   TEST BX,WORD PTR [1246h]
*/
void sx86_exec_full_modregrm_ro(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,sx86_ubyte opswap,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val))
{
	if (mod == 3) {		/* register <-> register */
		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword regv,rmv;

			regv = ctx->state->general_reg[reg].w.lo;
			rmv = ctx->state->general_reg[rm].w.lo;

			if (opswap)
				op16(ctx,regv,rmv);
			else
				op16(ctx,rmv,regv);
		}
		else {
			sx86_ubyte regv,rmv;

			regv = *(ctx->__private->ptr_regs_8reg[reg]);
			rmv = *(ctx->__private->ptr_regs_8reg[rm]);

			if (opswap)
				op8(ctx,regv,rmv);
			else
				op8(ctx,rmv,regv);
		}
	}
	else {			/* register <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword regv,rmv;

			regv = ctx->state->general_reg[reg].w.lo;
			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,2);
			SWAP_WORD_FROM_LE(rmv);

			if (opswap) {
				op16(ctx,regv,rmv);
			}
			else {
				op16(ctx,rmv,regv);
			}
		}
		else {
			sx86_ubyte regv,rmv;

			regv = *(ctx->__private->ptr_regs_8reg[reg]);
			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,1);

			if (opswap) {
				op8(ctx,regv,rmv);
			}
			else {
				op8(ctx,rmv,regv);
			}
		}
	}
}

/* general purpose mod/reg/rm executioneer helper for
   instructions that do modify the destination operand
   and need a FAR pointer

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   d32 = 386+ 32-bit data size override (ignored if w16 == 0)
   mod/reg/rm = mod/reg/rm unpacked byte
   op16 = function to call for emulation of instruction, given 16-bit operands "src" and "dst"
   op32 = function to call for emulation of instruction, given 32-bit operands "src" and "dst"

   Example of instructions that fit this format:

   LES BX,WORD PTR [5256h]
*/
void sx86_exec_full_modregrm_far(softx86_ctx* ctx,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword seg,sx86_uword ofs),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword seg,sx86_udword ofs))
{
	if (mod == 3) {		/* register <-> register */
		/* illegal */
	}
	else {			/* register <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		{
			sx86_uword rmv,rmv2;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,2);
			lo  += 2;
			softx86_fetch(ctx,NULL,lo,&rmv2,2);
			SWAP_WORD_FROM_LE(rmv);
			SWAP_WORD_FROM_LE(rmv2);
			ctx->state->general_reg[reg].w.lo = op16(ctx,rmv2,rmv);
		}
	}
}

/* general purpose mod/reg/rm executioneer helper for
   instructions that do not modify the destination operand
   but need a FAR pointer and the register operand

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   d32 = 386+ 32-bit data size override (ignored if w16 == 0)
   mod/reg/rm = mod/reg/rm unpacked byte
   op16 = function to call for emulation of instruction, given 16-bit operands "src" and "dst"
   op32 = function to call for emulation of instruction, given 32-bit operands "src" and "dst"

   Example of instructions that fit this format:

   BOUND BX,WORD PTR [9874h]
*/
void sx86_exec_full_modregrm_far_ro3(softx86_ctx* ctx,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,sx86_sword (*op16)(softx86_ctx* ctx,sx86_sword idx,sx86_sword upper,sx86_sword lower),sx86_sdword (*op32)(softx86_ctx* ctx,sx86_sdword idx,sx86_sdword upper,sx86_sdword lower))
{
	if (mod == 3) {		/* register <-> register */
		/* illegal */
	}
	else {			/* register <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		{
			sx86_sword rmv,rmv2;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,2);
			lo  += 2;
			softx86_fetch(ctx,NULL,lo,&rmv2,2);
			SWAP_WORD_FROM_LE(rmv);
			SWAP_WORD_FROM_LE(rmv2);
			op16(ctx,ctx->state->general_reg[reg].w.lo,rmv2,rmv);
		}
	}
}

/* general purpose mod/rm executioneer helper for
   instructions that modify the destination operand
   and have an immediate operand of specified size.

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   w16 = 16-bit operand flag
   d32 = 386+ 32-bit data size override (ignored if w16 == 0)
   mod/rm = mod/reg/rm unpacked byte
   op8 = function to call for emulation of instruction, given 8-bit operands "src" and "dst"
   op16 = function to call for emulation of instruction, given 16-bit operands "src" and "dst"
   op32 = function to call for emulation of instruction, given 32-bit operands "src" and "dst"

   Example of instructions that fit this format:

   MOV AH,2456h
   MOV WORD PTR [1334h],2222h
*/
void sx86_exec_full_modrmonly_rw_imm(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val),int sx)
{
	sx86_uword imm;

	if (mod == 3) {		/* immediate <-> register */
		imm = softx86_fetch_exec_byte(ctx);
		if (w16)
			if (sx)	imm |= (imm&0x80) ? 0xFF80 : 0;
			else	imm |= softx86_fetch_exec_byte(ctx)<<8;

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword rmv;

			rmv = ctx->state->general_reg[rm].w.lo;

			ctx->state->general_reg[rm].w.lo =
				op16(ctx,rmv,(sx86_uword)imm);
		}
		else {
			sx86_ubyte rmv;

			rmv = *(ctx->__private->ptr_regs_8reg[rm]);

			*(ctx->__private->ptr_regs_8reg[rm]) =
				op8(ctx,rmv,(sx86_ubyte)imm);
		}
	}
	else {			/* immediate <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		imm = softx86_fetch_exec_byte(ctx);
		if (w16)
			if (sx)	imm |= (imm&0x80) ? 0xFF80 : 0;
			else	imm |= softx86_fetch_exec_byte(ctx)<<8;

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword rmv;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,2);
			SWAP_WORD_FROM_LE(rmv);

			rmv = op16(ctx,rmv,(sx86_uword)imm);
			SWAP_WORD_TO_LE(rmv);
			softx86_write(ctx,NULL,lo,&rmv,2);
		}
		else {
			sx86_ubyte rmv;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,1);

			rmv = op8(ctx,rmv,(sx86_ubyte)imm);
			softx86_write(ctx,NULL,lo,&rmv,1);
		}
	}
}

/* general purpose mod/rm executioneer helper for
   instructions that modify the destination operand
   and have an immediate operand of specified size.

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   w16 = 16-bit operand flag
   d32 = 386+ 32-bit data size override (ignored if w16 == 0)
   mod/rm = mod/reg/rm unpacked byte
   op8 = function to call for emulation of instruction, given 8-bit operands "src" and "dst"
   op16 = function to call for emulation of instruction, given 16-bit operands "src" and "dst"
   op32 = function to call for emulation of instruction, given 32-bit operands "src" and "dst"

   Example of instructions that fit this format:

   MOV AH,2456h
   MOV WORD PTR [1334h],2222h
*/
void sx86_exec_full_modrmonly_rw_imm8(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val))
{
	sx86_uword imm;

	if (mod == 3) {		/* immediate <-> register */
		imm = softx86_fetch_exec_byte(ctx);

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword rmv;

			rmv = ctx->state->general_reg[rm].w.lo;

			ctx->state->general_reg[rm].w.lo =
				op16(ctx,rmv,(sx86_uword)imm);
		}
		else {
			sx86_ubyte rmv;

			rmv = *(ctx->__private->ptr_regs_8reg[rm]);

			*(ctx->__private->ptr_regs_8reg[rm]) =
				op8(ctx,rmv,(sx86_ubyte)imm);
		}
	}
	else {			/* immediate <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		imm = softx86_fetch_exec_byte(ctx);
		if (w16) imm |= softx86_fetch_exec_byte(ctx)<<8;

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword rmv;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,2);
			SWAP_WORD_FROM_LE(rmv);

			rmv = op16(ctx,rmv,(sx86_uword)imm);
			SWAP_WORD_TO_LE(rmv);
			softx86_write(ctx,NULL,lo,&rmv,2);
		}
		else {
			sx86_ubyte rmv;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,1);

			rmv = op8(ctx,rmv,(sx86_ubyte)imm);
			softx86_write(ctx,NULL,lo,&rmv,1);
		}
	}
}

/* general purpose mod/rm executioneer helper for
   instructions that DO NOT modify the destination operand
   and have an immediate operand of specified size.

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   w16 = 16-bit operand flag
   d32 = 386+ 32-bit data size override (ignored if w16 == 0)
   mod/rm = mod/reg/rm unpacked byte
   op8 = function to call for emulation of instruction, given 8-bit operands "src" and "dst"
   op16 = function to call for emulation of instruction, given 16-bit operands "src" and "dst"
   op32 = function to call for emulation of instruction, given 32-bit operands "src" and "dst"

   Example of instructions that fit this format:

   MOV AH,2456h
   MOV WORD PTR [1334h],2222h
*/
void sx86_exec_full_modrmonly_ro_imm(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val),int sx)
{
	sx86_uword imm;

	if (mod == 3) {		/* immediate <-> register */
		imm = softx86_fetch_exec_byte(ctx);
		if (w16)
			if (sx)	imm |= (imm&0x80) ? 0xFF80 : 0;
			else	imm |= softx86_fetch_exec_byte(ctx)<<8;

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword rmv;

			rmv = ctx->state->general_reg[rm].w.lo;
			op16(ctx,rmv,(sx86_uword)imm);
		}
		else {
			sx86_ubyte rmv;

			rmv = *(ctx->__private->ptr_regs_8reg[rm]);
			op8(ctx,rmv,(sx86_ubyte)imm);
		}
	}
	else {			/* immediate <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		imm = softx86_fetch_exec_byte(ctx);
		if (w16)
			if (sx)	imm |= (imm&0x80) ? 0xFF80 : 0;
			else	imm |= softx86_fetch_exec_byte(ctx)<<8;

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword rmv;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,2);
			SWAP_WORD_FROM_LE(rmv);

			op16(ctx,rmv,(sx86_uword)imm);
		}
		else {
			sx86_ubyte rmv;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,1);

			op8(ctx,rmv,(sx86_ubyte)imm);
		}
	}
}

/* general purpose mod/rm executioneer helper for
   instructions that modify the destination operand
   and do not have an immediate operand of specified size.

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   w16 = 16-bit operand flag
   d32 = 386+ 32-bit data size override (ignored if w16 == 0)
   mod/rm = mod/reg/rm unpacked byte
   op8 = function to call for emulation of instruction, given 8-bit operand "src"
   op16 = function to call for emulation of instruction, given 16-bit operand "src"
   op32 = function to call for emulation of instruction, given 32-bit operand "src"

   INC BX
   INC WORD PTR [2045h]
*/
void sx86_exec_full_modrmonly_rw(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src))
{
	if (mod == 3) {		/* immediate <-> register */
		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword rmv;

			rmv = ctx->state->general_reg[rm].w.lo;

			ctx->state->general_reg[rm].w.lo =
				op16(ctx,rmv);
		}
		else {
			sx86_ubyte rmv;

			rmv = *(ctx->__private->ptr_regs_8reg[rm]);

			*(ctx->__private->ptr_regs_8reg[rm]) =
				op8(ctx,rmv);
		}
	}
	else {			/* immediate <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword rmv;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,2);
			SWAP_WORD_FROM_LE(rmv);

			rmv = op16(ctx,rmv);
			SWAP_WORD_TO_LE(rmv);
			softx86_write(ctx,NULL,lo,&rmv,2);
		}
		else {
			sx86_ubyte rmv;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,1);

			rmv = op8(ctx,rmv);
			softx86_write(ctx,NULL,lo,&rmv,1);
		}
	}
}

/* general purpose mod/rm executioneer helper for
   instructions that do not modify the destination operand
   and do not have an immediate operand of specified size.

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   w16 = 16-bit operand flag
   d32 = 386+ 32-bit data size override (ignored if w16 == 0)
   mod/rm = mod/reg/rm unpacked byte
   op8 = function to call for emulation of instruction, given 8-bit operand "src"
   op16 = function to call for emulation of instruction, given 16-bit operand "src"
   op32 = function to call for emulation of instruction, given 32-bit operand "src"

   CALL WORD PTR [2045h]
*/
void sx86_exec_full_modrmonly_ro(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src))
{
	if (mod == 3) {		/* immediate <-> register */
		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			op16(ctx,ctx->state->general_reg[rm].w.lo);
		}
		else {
			op8(ctx,*(ctx->__private->ptr_regs_8reg[rm]));
		}
	}
	else {			/* immediate <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword rmv;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,2);
			SWAP_WORD_FROM_LE(rmv);
			op16(ctx,rmv);
		}
		else {
			sx86_ubyte rmv;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,1);
			op8(ctx,rmv);
		}
	}
}

/* general purpose mod/rm executioneer helper for
   instructions that modify the destination operand
   and do not have an immediate operand of specified size.
   these functions do not consider the previous value.

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   w16 = 16-bit operand flag
   d32 = 386+ 32-bit data size override (ignored if w16 == 0)
   mod/rm = mod/reg/rm unpacked byte
   op8 = function to call for emulation of instruction, given 8-bit operand "src"
   op16 = function to call for emulation of instruction, given 16-bit operand "src"
   op32 = function to call for emulation of instruction, given 32-bit operand "src"

   POP WORD PTR [2045h]
*/
void sx86_exec_full_modrmonly_wo(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,sx86_ubyte (*op8)(softx86_ctx* ctx),sx86_uword (*op16)(softx86_ctx* ctx),sx86_udword (*op32)(softx86_ctx* ctx))
{
	if (mod == 3) {		/* immediate <-> register */
		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			ctx->state->general_reg[rm].w.lo = op16(ctx);
		}
		else {
			*(ctx->__private->ptr_regs_8reg[rm]) = op8(ctx);
		}
	}
	else {			/* immediate <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword rmv;

			lo = sx86_far_to_linear(ctx,seg,ofs);
			rmv = op16(ctx);
			SWAP_WORD_TO_LE(rmv);
			softx86_write(ctx,NULL,lo,&rmv,2);
		}
		else {
			sx86_ubyte rmv;

			lo = sx86_far_to_linear(ctx,seg,ofs);
			rmv = op8(ctx);
			softx86_write(ctx,NULL,lo,&rmv,1);
		}
	}
}

/* general purpose mod/rm executioneer helper for
   instructions that need a segment:offset pair
   and do not have an immediate operand of specified size.

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   d32 = 386+ 32-bit data size override (ignored if w16 == 0)
   mod/rm = mod/reg/rm unpacked byte
   op8 = function to call for emulation of instruction, given 8-bit operand "src"
   op16 = function to call for emulation of instruction, given 16-bit operand "src"
   op32 = function to call for emulation of instruction, given 32-bit operand "src"

   CALL FAR WORD PTR [2045h]
*/
void sx86_exec_full_modrmonly_callfar(softx86_ctx* ctx,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,void (*op16)(softx86_ctx* ctx,sx86_uword seg,sx86_uword ofs),void (*op32)(softx86_ctx* ctx,sx86_udword seg,sx86_udword ofs))
{
	if (mod == 3) {		/* immediate <-> register */
		// invalid, since a seg:off pair does not fit in a register
	}
	else {			/* immediate <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		{
			sx86_uword rmv,rmv2;

			lo   = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&rmv,2); lo += 2;
			softx86_fetch(ctx,NULL,lo,&rmv2,2);
			SWAP_WORD_FROM_LE(rmv);
			SWAP_WORD_FROM_LE(rmv2);
			op16(ctx,rmv2,rmv);
		}
	}
}

/* general purpose mod/reg/rm executioneer helper for
   the XCHG instruction.

   this is provided to avoid copy and pasting this source code into the emulation
   code for *EVERY* instruction.

   ctx = CPU context
   w16 = 16-bit operand flag
   d32 = 386+ 32-bit data size override (ignored if w16 == 0)
   mod/reg/rm = mod/reg/rm unpacked byte
*/
void sx86_exec_full_modregrm_xchg(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm)
{
	if (mod == 3) {		/* register <-> register */
		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword tmp;

			tmp =					ctx->state->general_reg[reg].w.lo;
			ctx->state->general_reg[reg].w.lo =	ctx->state->general_reg[rm].w.lo;
			ctx->state->general_reg[rm].w.lo =	tmp;
		}
		else {
			sx86_ubyte tmp;

			tmp =					*(ctx->__private->ptr_regs_8reg[reg]);
			*(ctx->__private->ptr_regs_8reg[reg]) =	*(ctx->__private->ptr_regs_8reg[rm]);
			*(ctx->__private->ptr_regs_8reg[rm]) =	tmp;
		}
	}
	else {			/* register <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		if (w16) {
			sx86_uword tmp,mem;

			lo = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&mem,2);
			SWAP_WORD_FROM_LE(mem);
			tmp = ctx->state->general_reg[reg].w.lo;
			SWAP_WORD_TO_LE(tmp);
			softx86_write(ctx,NULL,lo,&tmp,2);
			ctx->state->general_reg[reg].w.lo = mem;
		}
		else {
			sx86_ubyte tmp,mem;

			lo = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,&mem,1);
			tmp = *(ctx->__private->ptr_regs_8reg[reg]);
			softx86_write(ctx,NULL,lo,&tmp,1);
			*(ctx->__private->ptr_regs_8reg[reg]) = mem;
		}
	}
}

int softx86_parity8(sx86_ubyte ret)
{
	int b,p;

	p=1;
	for (b=0;b < 8;b++) {
		p    ^= (ret&1);
		ret >>= 1;
	}

	return p;
}

int softx86_parity16(sx86_uword ret)
{
	int b,p;

	p=1;
	for (b=0;b < 16;b++) {
		p    ^= (ret&1);
		ret >>= 1;
	}

	return p;
}

int softx86_parity32(sx86_udword ret)
{
	int b,p;

	p=1;
	for (b=0;b < 32;b++) {
		p    ^= (ret&1);
		ret >>= 1;
	}

	return p;
}

int softx86_parity64(sx86_uldword ret)
{
	int b,p;

	p=1;
	for (b=0;b < 64;b++) {
		p    ^= (ret&1);
		ret >>= 1;
	}

	return p;
}

void sx86_exec_full_modrmonly_memx(softx86_ctx* ctx,sx86_ubyte mod,sx86_ubyte rm,int sz,void (*op64)(softx86_ctx* ctx,char *datz,int sz))
{
	if (sz > 16)
		return;

	if (mod == 3) {		/* immediate <-> register */
		// invalid
	}
	else {			/* immediate <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		{
			sx86_ubyte tmp[16];

			lo = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,tmp,sz);
			op64(ctx,tmp,sz);
		}
	}
}

void sx86_exec_full_modrw_memx(softx86_ctx* ctx,sx86_ubyte mod,sx86_ubyte rm,int sz,void (*op64)(softx86_ctx* ctx,char *datz,int sz))
{
	if (sz > 16)
		return;

	if (mod == 3) {		/* immediate <-> register */
		// invalid
	}
	else {			/* immediate <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		// TODO: For 386+ 32-bit instructions... if (d32) {...}
		{
			sx86_ubyte tmp[16];

			lo = sx86_far_to_linear(ctx,seg,ofs);
			softx86_fetch(ctx,NULL,lo,tmp,sz);
			op64(ctx,tmp,sz);
			softx86_write(ctx,NULL,lo,tmp,sz);
		}
	}
}

/* general purpose mod/segreg/rm executioneer helper for
   instructions that modify the destination operand.

   this is provided to avoid copy and pasting this source code into the emulation
   of *EVERY* instruction.

   ctx = CPU context
   mod/reg/rm = mod/reg/rm unpacked byte
   op16 = function to call for emulation of instruction, given 16-bit operands "src" and "dst"

   MOV AX,ES
   MOV CS,BX
   MOV ES,WORD PTR [3939h]
   MOV WORD PTR [9393h],DS
*/
void sx86_exec_full_modsregrm_rw(softx86_ctx* ctx,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,sx86_ubyte opswap,sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val))
{
	sx86_uword regv,rmv;

/* because not all segreg values are valid, check validity now! */
	if (reg >= 4) {		/* the 8086 does not have FS and GS */
/* TODO: Invalid opcode exception */
		return;
	}

	if (mod == 3) {		/* segment register <-> register */
		regv = ctx->state->segment_reg[reg].val;
		rmv = ctx->state->general_reg[rm].w.lo;

		if (opswap) {
/* REMEMBER: WE NEVER DIRECTLY MODIFY THE SEGMENT REGISTER VALUES! */
/*			*(ctx->ptr_segregs[reg]) =
				op16(ctx,rmv,regv); */
			softx86_setsegval(ctx,reg,rmv);
		}
		else {
			ctx->state->general_reg[rm].w.lo =
				op16(ctx,rmv,regv);
		}
	}
	else {			/* segment register <-> memory */
		sx86_uword seg,ofs;
		sx86_udword lo;

		if (!ctx->state->is_segment_override)
/* apparently if BP is used (i.e. [BP+BX]) the stack segment is assumed. */
			if ((rm == 2 || rm == 3) || (rm == 6 && (mod == 1 || mod == 2)))
				seg = ctx->state->segment_reg[SX86_SREG_SS].val;
			else
				seg = ctx->state->segment_reg[SX86_SREG_DS].val;
		else
			seg = ctx->state->segment_override;

/* figure out the memory address */
		if (mod == 0) {
			if (rm == 6) {
				ofs  =  softx86_fetch_exec_byte(ctx);
				ofs |=  softx86_fetch_exec_byte(ctx)<<8;
			}
			else if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;
		}
		else {
			sx86_uword xx;

			if (rm == 0)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 1)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 2)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 3)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo +
					ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 4)
				ofs =	ctx->state->general_reg[SX86_REG_SI].w.lo;
			else if (rm == 5)
				ofs =	ctx->state->general_reg[SX86_REG_DI].w.lo;
			else if (rm == 6)
				ofs =	ctx->state->general_reg[SX86_REG_BP].w.lo;
			else if (rm == 7)
				ofs =	ctx->state->general_reg[SX86_REG_BX].w.lo;

			if (mod == 1) {
				xx = softx86_fetch_exec_byte(ctx);
				if (xx & 0x80) xx |= 0xFF00;
			}
			else {
				xx  = softx86_fetch_exec_byte(ctx);
				xx |= softx86_fetch_exec_byte(ctx)<<8;
			}

			ofs = FORCE_WORD_SIZE(ofs + xx);
		}

		regv = ctx->state->segment_reg[reg].val;
		lo   = sx86_far_to_linear(ctx,seg,ofs);
		softx86_fetch(ctx,NULL,lo,&rmv,2);
		SWAP_WORD_FROM_LE(rmv);

		if (opswap) {
/* REMEMBER: WE NEVER DIRECTLY MODIFY THE SEGMENT REGISTER VALUES! */
/*			*(ctx->ptr_segregs[reg]) =
				op16(ctx,rmv,regv); */
			softx86_setsegval(ctx,reg,rmv);
		}
		else {
			rmv = op16(ctx,rmv,regv);
			SWAP_WORD_TO_LE(rmv);
			softx86_write(ctx,NULL,lo,&rmv,2);
		}
	}
}

/* basic mod/reg/rm full-use decompiling.
   ctx     = CPU context struct
   is_word = word value
   dat32   = 32-bit 386+ data override
   mod/reg/rm
   op1     = buffer for r/m 
   op2     = buffer for reg */
void sx86_dec_full_modregrm(softx86_ctx* ctx,sx86_ubyte is_word,sx86_ubyte dat32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,char* op1,char* op2)
{
	sx86_uword o;

	if (is_word)
		if (dat32)
			sprintf(op2,"%s",sx86_regs32[reg]);
		else
			sprintf(op2,"%s",sx86_regs16[reg]);
	else
		sprintf(op2,"%s",sx86_regs8[reg]);

	if (mod == 3) {		/* destination: register */
		if (is_word)
			if (dat32)
				sprintf(op1,"%s",sx86_regs32[rm]);
			else
				sprintf(op1,"%s",sx86_regs16[rm]);
		else
			sprintf(op1,"%s",sx86_regs8[rm]);
	}
	else if (mod == 0) {	/* destination: memory address */
		if (rm == 6) {
			o  = softx86_fetch_dec_byte(ctx);
			o |= softx86_fetch_dec_byte(ctx)<<8;
			sprintf(op1,"[%04Xh]",o);
		}
		else {
			sprintf(op1,"[%s]",sx86_regsaddr16_16[rm]);
		}
	}
	else {			/* destination: memory address + offset */
		if (mod == 1) {
			o  = softx86_fetch_dec_byte(ctx);
			if (o & 0x80) o |= 0xFF00;
		}
		else if (mod == 2) {
			o  = softx86_fetch_dec_byte(ctx);
			o |= softx86_fetch_dec_byte(ctx)<<8;
		}

		sprintf(op1,"[%s+%04Xh]",sx86_regsaddr16_16[rm],o);
	}
}

/* basic mod/rm full-use decompiling (reg used for another purpose)
   ctx     = CPU context struct
   is_word = word value
   dat32   = 32-bit 386+ data override
   mod/rm
   op1     = buffer for r/m */
void sx86_dec_full_modrmonly(softx86_ctx* ctx,sx86_ubyte is_word,sx86_ubyte dat32,sx86_ubyte mod,sx86_ubyte rm,char* op1)
{
	sx86_uword o;

	if (mod == 3) {		/* destination: register */
		if (is_word)
			if (dat32)
				sprintf(op1,"%s",sx86_regs32[rm]);
			else
				sprintf(op1,"%s",sx86_regs16[rm]);
		else
			sprintf(op1,"%s",sx86_regs8[rm]);
	}
	else if (mod == 0) {	/* destination: memory address */
		if (rm == 6) {
			o  = softx86_fetch_dec_byte(ctx);
			o |= softx86_fetch_dec_byte(ctx)<<8;
			sprintf(op1,"[%04Xh]",o);
		}
		else {
			sprintf(op1,"[%s]",sx86_regsaddr16_16[rm]);
		}
	}
	else {			/* destination: memory address + offset */
		if (mod == 1) {
			o  = softx86_fetch_dec_byte(ctx);
			if (o & 0x80) o |= 0xFF00;
		}
		else if (mod == 2) {
			o  = softx86_fetch_dec_byte(ctx);
			o |= softx86_fetch_dec_byte(ctx)<<8;
		}

		sprintf(op1,"[%s+%04Xh]",sx86_regsaddr16_16[rm],o);
	}
}

/* basic mod/segreg/rm full-use decompiling.
   ctx     = CPU context struct
   is_word = word value
   dat32   = 32-bit 386+ data override
   mod/reg/rm where "reg" refers to a segment register
   op1     = buffer for r/m 
   op2     = buffer for reg */
void sx86_dec_full_modsregrm(softx86_ctx* ctx,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,char* op1,char* op2)
{
	sx86_uword o;

	sprintf(op2,"%s",sx86_segregs[reg]);

	if (mod == 3) {		/* destination: register */
		sprintf(op1,"%s",sx86_regs16[rm]);
	}
	else if (mod == 0) {	/* destination: memory address */
		if (rm == 6) {
			o  = softx86_fetch_dec_byte(ctx);
			o |= softx86_fetch_dec_byte(ctx)<<8;
			sprintf(op1,"[%04Xh]",o);
		}
		else {
			sprintf(op1,"[%s]",sx86_regsaddr16_16[rm]);
		}
	}
	else {			/* destination: memory address + offset */
		if (mod == 1) {
			o  = softx86_fetch_dec_byte(ctx);
			if (o & 0x80) o |= 0xFF00;
		}
		else if (mod == 2) {
			o  = softx86_fetch_dec_byte(ctx);
			o |= softx86_fetch_dec_byte(ctx)<<8;
		}

		sprintf(op1,"[%s+%04Xh]",sx86_regsaddr16_16[rm],o);
	}
}

