/*
 * Copyright (c) 2001
 *	Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 *
 * Portions copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from the Stanford/CMU enet packet filter,
 * (net/enet.c) distributed as part of 4.3BSD, and code contributed
 * to Berkeley by Steven McCanne and Van Jacobson both of Lawrence 
 * Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "tme.h"
#include "win_bpf.h"

/*
 * Initialize the filter machine
 */
uint32 bpf_filter_init(register struct bpf_insn *pc, MEM_TYPE *mem_ex, TME_CORE *tme, struct time_conv *time_ref)
{
	register uint32 A, X;
	int32 mem[BPF_MEMWORDS];
	register int32 k;
	uint32 *tmp;
	uint16 *tmp2;
	uint32 j;
	if (pc == 0)
		/*
		 * No filter means accept all.
		 */
		 return (uint32)-1;
	A = 0;
	X = 0;
	--pc;
	while (1) {
		++pc;
		switch (pc->code) {

		default:
			return 0;

/* RET INSTRUCTIONS */
		case BPF_RET|BPF_K:
			return (uint32)pc->k;

		case BPF_RET|BPF_A:
			return (uint32)A;
/* END RET INSTRUCTIONS */

/* LD NO PACKET INSTRUCTIONS */
		case BPF_LD|BPF_IMM:
			A = pc->k;
			continue;

		case BPF_LDX|BPF_IMM:
			X = pc->k;
			continue;

		case BPF_LD|BPF_MEM:
			A = mem[pc->k];
			continue;
			
		case BPF_LDX|BPF_MEM:
			X = mem[pc->k];
			continue;

		case BPF_LD|BPF_MEM_EX_IMM|BPF_B:
			A= mem_ex->buffer[pc->k];
			continue;

		case BPF_LDX|BPF_MEM_EX_IMM|BPF_B:
			X= mem_ex->buffer[pc->k];
			continue;

		case BPF_LD|BPF_MEM_EX_IMM|BPF_H:
			tmp2=(uint16*)&mem_ex->buffer[pc->k];
#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				mov ebx,tmp2
				xor eax, eax
				mov ax, [ebx]
				bswap eax
				mov A, eax
				pop ebx
				pop eax
			}
#else
#endif
			continue;

		case BPF_LDX|BPF_MEM_EX_IMM|BPF_H:
			tmp2=(uint16*)&mem_ex->buffer[pc->k];
#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				mov ebx,tmp2
				xor eax, eax
				mov ax, [ebx]
				bswap eax
				mov X, eax
				pop ebx
				pop eax
			}
#else
#endif
			continue;

		case BPF_LD|BPF_MEM_EX_IMM|BPF_W:
			tmp=(uint32*)&mem_ex->buffer[pc->k];
#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				mov ebx,tmp
				mov eax, [ebx]
				bswap eax
				mov A, eax
				pop ebx
				pop eax
			}
#else
#endif
			continue;

		case BPF_LDX|BPF_MEM_EX_IMM|BPF_W:
			tmp=(uint32*)&mem_ex->buffer[pc->k];
#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				mov ebx,tmp
				mov eax, [ebx]
				bswap eax
				mov X, eax
				pop ebx
				pop eax
			}
#else
#endif
			continue;
			
		case BPF_LD|BPF_MEM_EX_IND|BPF_B:
			k = X + pc->k;
			if ((int32)k>= (int32)mem_ex->size) {
				return 0;
			}
			A= mem_ex->buffer[k];
			continue;

		case BPF_LD|BPF_MEM_EX_IND|BPF_H:
			k = X + pc->k;
			if ((int32)(k+1)>= (int32)mem_ex->size) {
				return 0;
			}
			tmp2=(uint16*)&mem_ex->buffer[k];
#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				mov ebx,tmp2
				xor eax, eax
				mov ax, [ebx]
				bswap eax
				mov A, eax
				pop ebx
				pop eax
			}
#else
#endif
			continue;

		case BPF_LD|BPF_MEM_EX_IND|BPF_W:
			k = X + pc->k;
			if ((int32)(k+3)>= (int32)mem_ex->size) {
				return 0;
			}
			tmp=(uint32*)&mem_ex->buffer[k];
#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				mov ebx,tmp
				mov eax, [ebx]
				bswap eax
				mov A, eax
				pop ebx
				pop eax
			}
#else
#endif
			continue;
/* END LD NO PACKET INSTRUCTIONS */

/* STORE INSTRUCTIONS */
		case BPF_ST:
			mem[pc->k] = A;
			continue;

		case BPF_STX:
			mem[pc->k] = X;
			continue;

		case BPF_ST|BPF_MEM_EX_IMM|BPF_B:
			mem_ex->buffer[pc->k]=(uint8)A;
			continue;

		case BPF_STX|BPF_MEM_EX_IMM|BPF_B:
			mem_ex->buffer[pc->k]=(uint8)X;
			continue;

		case BPF_ST|BPF_MEM_EX_IMM|BPF_W:
			tmp=(uint32*)&mem_ex->buffer[pc->k];
#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				mov ebx, tmp
				mov eax, A
				bswap eax
				mov [ebx], eax
				pop ebx
				pop eax
			}
#else
#endif
			continue;

		case BPF_STX|BPF_MEM_EX_IMM|BPF_W:
			tmp=(uint32*)&mem_ex->buffer[pc->k];
#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				mov ebx, tmp
				mov eax, X
				bswap eax
				mov [ebx], eax
				pop ebx
				pop eax
			}
#else
#endif
			continue;

		case BPF_ST|BPF_MEM_EX_IMM|BPF_H:
			tmp2=(uint16*)&mem_ex->buffer[pc->k];
#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				mov ebx, tmp2
				mov eax, A
				xchg ah, al
				mov [ebx], ax
				pop ebx
				pop eax
			}
#else
#endif
			continue;

		case BPF_STX|BPF_MEM_EX_IMM|BPF_H:
			tmp2=(uint16*)&mem_ex->buffer[pc->k];
#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				mov ebx, tmp2
				mov eax, X
				xchg ah, al
				mov [ebx], ax
				pop ebx
				pop eax
			}
#else
#endif
			continue;

		case BPF_ST|BPF_MEM_EX_IND|BPF_B:
			mem_ex->buffer[pc->k+X]=(uint8)A;

		case BPF_ST|BPF_MEM_EX_IND|BPF_W:
			tmp=(uint32*)&mem_ex->buffer[pc->k+X];
#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				mov ebx, tmp
				mov eax, A
				bswap eax
				mov [ebx], eax
				pop ebx
				pop eax
			}
#else
#endif
			continue;

		case BPF_ST|BPF_MEM_EX_IND|BPF_H:
			tmp2=(uint16*)&mem_ex->buffer[pc->k+X];
#ifndef __GNUC__
			__asm
			{
				push eax
				push ebx
				mov ebx, tmp2
				mov eax, A
				xchg ah, al
				mov [ebx], ax
				pop ebx
				pop eax
			}
#else
#endif
			continue;
/* END STORE INSTRUCTIONS */

/* JUMP INSTRUCTIONS */
		case BPF_JMP|BPF_JA:
			pc += pc->k;
			continue;

		case BPF_JMP|BPF_JGT|BPF_K:
			pc += ((int32)A > (int32)pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JGE|BPF_K:
			pc += ((int32)A >= (int32)pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JEQ|BPF_K:
			pc += ((int32)A == (int32)pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JSET|BPF_K:
			pc += (A & pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JGT|BPF_X:
			pc += (A > X) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JGE|BPF_X:
			pc += (A >= X) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JEQ|BPF_X:
			pc += (A == X) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JSET|BPF_X:
			pc += (A & X) ? pc->jt : pc->jf;
			continue;
/* END JUMP INSTRUCTIONS */

/* ARITHMETIC INSTRUCTIONS */
		case BPF_ALU|BPF_ADD|BPF_X:
			A += X;
			continue;
			
		case BPF_ALU|BPF_SUB|BPF_X:
			A -= X;
			continue;
			
		case BPF_ALU|BPF_MUL|BPF_X:
			A *= X;
			continue;
			
		case BPF_ALU|BPF_DIV|BPF_X:
			if (X == 0)
				return 0;
			A /= X;
			continue;
			
		case BPF_ALU|BPF_AND|BPF_X:
			A &= X;
			continue;
			
		case BPF_ALU|BPF_OR|BPF_X:
			A |= X;
			continue;

		case BPF_ALU|BPF_LSH|BPF_X:
			A <<= X;
			continue;

		case BPF_ALU|BPF_RSH|BPF_X:
			A >>= X;
			continue;

		case BPF_ALU|BPF_ADD|BPF_K:
			A += pc->k;
			continue;
			
		case BPF_ALU|BPF_SUB|BPF_K:
			A -= pc->k;
			continue;
			
		case BPF_ALU|BPF_MUL|BPF_K:
			A *= pc->k;
			continue;
			
		case BPF_ALU|BPF_DIV|BPF_K:
			A /= pc->k;
			continue;
			
		case BPF_ALU|BPF_AND|BPF_K:
			A &= pc->k;
			continue;
			
		case BPF_ALU|BPF_OR|BPF_K:
			A |= pc->k;
			continue;

		case BPF_ALU|BPF_LSH|BPF_K:
			A <<= pc->k;
			continue;

		case BPF_ALU|BPF_RSH|BPF_K:
			A >>= pc->k;
			continue;

		case BPF_ALU|BPF_NEG:
			(int32)A = -((int32)A);
			continue;
/* ARITHMETIC INSTRUCTIONS */

/* MISC INSTRUCTIONS */
		case BPF_MISC|BPF_TAX:
			X = A;
			continue;

		case BPF_MISC|BPF_TXA:
			A = X;
			continue;
/* END MISC INSTRUCTIONS */

/* TME INSTRUCTIONS */
		case BPF_MISC|BPF_TME|BPF_LOOKUP:
			j=lookup_frontend(mem_ex,tme,pc->k,time_ref);
			if (j==TME_ERROR)
				return 0;	
			pc += (j == TME_TRUE) ? pc->jt : pc->jf;
			continue;

		case BPF_MISC|BPF_TME|BPF_EXECUTE:
			if (execute_frontend(mem_ex,tme,0,pc->k)==TME_ERROR)
				return 0;
			continue;

		case BPF_MISC|BPF_TME|BPF_INIT:
			if (init_tme_block(tme,pc->k)==TME_ERROR)
				return 0;
			continue;
			
		case BPF_MISC|BPF_TME|BPF_VALIDATE:
			if (validate_tme_block(mem_ex,tme,A,pc->k)==TME_ERROR)
				return 0;
			continue;

		case BPF_MISC|BPF_TME|BPF_SET_MEMORY:
			if (init_extended_memory(pc->k,mem_ex)==TME_ERROR)
				return 0;
			continue;

		case BPF_MISC|BPF_TME|BPF_SET_ACTIVE:
			if (set_active_tme_block(tme,pc->k)==TME_ERROR)
				return 0;
			continue;

		case BPF_MISC|BPF_TME|BPF_SET_ACTIVE_READ:
			if (set_active_tme_block(tme,pc->k)==TME_ERROR)
				return 0;
			continue;
		case BPF_MISC|BPF_TME|BPF_SET_WORKING:
			if ((pc->k<0)||(pc->k>=MAX_TME_DATA_BLOCKS))
				return 0;
			tme->working=pc->k;
			continue;



		case BPF_MISC|BPF_TME|BPF_RESET:
			if (reset_tme(tme)==TME_ERROR)
				return 0;
			continue;

		case BPF_MISC|BPF_TME|BPF_GET_REGISTER_VALUE:
			if (get_tme_block_register(&tme->block_data[tme->working],mem_ex,pc->k,&j)==TME_ERROR)
				return 0;
			A=j;
			continue;

		case BPF_MISC|BPF_TME|BPF_SET_REGISTER_VALUE:
			if (set_tme_block_register(&tme->block_data[tme->working],mem_ex,pc->k,A,TRUE)==TME_ERROR)
				return 0;
			continue;

		case BPF_MISC|BPF_TME|BPF_SET_AUTODELETION:
			set_autodeletion(&tme->block_data[tme->working],pc->k);
			continue;
			
/* END TME INSTRUCTIONS */

		}
	}
}

