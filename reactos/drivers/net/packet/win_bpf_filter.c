/*
 * Copyright (c) 1999, 2000
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
 
	@(#)bpf.c	7.5 (Berkeley) 7/15/91
*/

#if !(defined(lint) || defined(KERNEL))
static const char rcsid[] =
    "@(#) $Header: /cygdrive/c/RCVS/CVS/ReactOS/reactos/drivers/net/packet/win_bpf_filter.c,v 1.1 2002/06/19 15:43:15 robd Exp $ (LBL)";
#endif

#ifndef WIN32
#include <sys/param.h>
#include <sys/time.h>
#else 
#include <winsock2.h>
#endif
#include <sys/types.h>
#include "win_bpf.h"

#define int32 bpf_int32
#define u_int32 bpf_u_int32


#ifndef LBL_ALIGN
#if defined(sparc) || defined(mips) || defined(ibm032) || \
    defined(__alpha) || defined(__hpux)
#define LBL_ALIGN
#endif
#endif

#ifndef LBL_ALIGN
#ifndef WIN32
#include <netinet/in.h>
#endif

#define EXTRACT_SHORT(p)\
	((u_short)\
		((u_short)*((u_char *)p+0)<<8|\
		 (u_short)*((u_char *)p+1)<<0))
#define EXTRACT_LONG(p)\
		((u_int32)*((u_char *)p+0)<<24|\
		 (u_int32)*((u_char *)p+1)<<16|\
		 (u_int32)*((u_char *)p+2)<<8|\
		 (u_int32)*((u_char *)p+3)<<0)
#endif

/** @ingroup NPF 
 *  @{
 */

/** @defgroup NPF_code NPF functions 
 *  @{
 */

/*
 * Execute the filter program starting at pc on the packet p
 * wirelen is the length of the original packet
 * buflen is the amount of data present
 */
u_int bpf_filter(pc, p, wirelen, buflen)
	register struct bpf_insn *pc;
	register u_char *p;
	u_int wirelen;
	register u_int buflen;
{
	register u_int32 A, X;
	register int k;
	int32 mem[BPF_MEMWORDS];

	if (pc == 0)
		/*
		 * No filter means accept all.
		 */
		return (u_int)-1;
	A = 0;
	X = 0;
	--pc;
	while (1) {
		++pc;
		switch (pc->code) {

		default:
		
			return 0;

		case BPF_RET|BPF_K:
			return (u_int)pc->k;

		case BPF_RET|BPF_A:
			return (u_int)A;

		case BPF_LD|BPF_W|BPF_ABS:
			k = pc->k;
			if (k + sizeof(int32) > buflen) {
				return 0;
			}
			A = EXTRACT_LONG(&p[k]);
			continue;

		case BPF_LD|BPF_H|BPF_ABS:
			k = pc->k;
			if (k + sizeof(short) > buflen) {
				return 0;
			}
			A = EXTRACT_SHORT(&p[k]);
			continue;

		case BPF_LD|BPF_B|BPF_ABS:
			k = pc->k;
			if ((int)k >= (int)buflen) {
				return 0;
			}
			A = p[k];
			continue;

		case BPF_LD|BPF_W|BPF_LEN:
			A = wirelen;
			continue;

		case BPF_LDX|BPF_W|BPF_LEN:
			X = wirelen;
			continue;

		case BPF_LD|BPF_W|BPF_IND:
			k = X + pc->k;
			if (k + sizeof(int32) > buflen) {
				return 0;
			}
			A = EXTRACT_LONG(&p[k]);
			continue;

		case BPF_LD|BPF_H|BPF_IND:
			k = X + pc->k;
			if (k + sizeof(short) > buflen) {
				return 0;
			}
			A = EXTRACT_SHORT(&p[k]);
			continue;

		case BPF_LD|BPF_B|BPF_IND:
			k = X + pc->k;
			if ((int)k >= (int)buflen) {
				return 0;
			}
			A = p[k];
			continue;

		case BPF_LDX|BPF_MSH|BPF_B:
			k = pc->k;
			if ((int)k >= (int)buflen) {
				return 0;
			}
			X = (p[pc->k] & 0xf) << 2;
			continue;

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

		case BPF_ST:
			mem[pc->k] = A;
			continue;

		case BPF_STX:
			mem[pc->k] = X;
			continue;

		case BPF_JMP|BPF_JA:
			pc += pc->k;
			continue;

		case BPF_JMP|BPF_JGT|BPF_K:
			pc += ((int)A > (int)pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JGE|BPF_K:
			pc += ((int)A >= (int)pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JEQ|BPF_K:
			pc += ((int)A == (int)pc->k) ? pc->jt : pc->jf;
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
			(int)A = -((int)A);
			continue;

		case BPF_MISC|BPF_TAX:
			X = A;
			continue;

		case BPF_MISC|BPF_TXA:
			A = X;
			continue;
		}
	}
}


/*
 * Execute the filter program starting at pc on the packet whose header is 
 * pointed by p and whose data is pointed by pd.
 * headersize is the size of the the header
 * wirelen is the length of the original packet
 * buflen is the amount of data present
 */

u_int bpf_filter_with_2_buffers(pc, p, pd, headersize, wirelen, buflen)
	register struct bpf_insn *pc;
	register u_char *p;
	register u_char *pd;
	register int headersize; 
	u_int wirelen;
	register u_int buflen;
{
	register u_int32 A, X;
	register int k;
	int32 mem[BPF_MEMWORDS];

	if (pc == 0)
		/*
		 * No filter means accept all.
		 */
		return (u_int)-1;
	A = 0;
	X = 0;
	--pc;
	while (1) {
		++pc;
		switch (pc->code) {

		default:
		
			return 0;

		case BPF_RET|BPF_K:
			return (u_int)pc->k;

		case BPF_RET|BPF_A:
			return (u_int)A;

		case BPF_LD|BPF_W|BPF_ABS:
			k = pc->k;
			if (k + sizeof(int32) > buflen) {
				return 0;
			}
			
			if(k + (int)sizeof(int32) < headersize) A = EXTRACT_LONG(&p[k]);
			else if(k + 2 == headersize){
				A=(u_int32)*((u_char *)p+k)<<24|
					(u_int32)*((u_char *)p+k+1)<<16|
					(u_int32)*((u_char *)p+k+2)<<8|
					(u_int32)*((u_char *)pd+k-headersize);
			}
			else if(k == headersize-1){
				A=(u_int32)*((u_char *)p+k)<<24|
					(u_int32)*((u_char *)p+k+1)<<16|
					(u_int32)*((u_char *)pd+k-headersize)<<8|
					(u_int32)*((u_char *)pd+k-headersize+1);
			}
			else if(k == headersize){
				A=(u_int32)*((u_char *)p+k)<<24|
					(u_int32)*((u_char *)pd+k-headersize+1)<<16|
					(u_int32)*((u_char *)pd+k-headersize+2)<<8|
					(u_int32)*((u_char *)pd+k-headersize+3);
			}
			A = EXTRACT_LONG(&pd[k-headersize]);
			
			continue;
			
		case BPF_LD|BPF_H|BPF_ABS:
			k = pc->k;
			if (k + sizeof(short) > buflen) {
				return 0;
			}
			
			if(k + (int)sizeof(short) < headersize) A = EXTRACT_SHORT(&p[k]);
			else if(k == headersize){
				A=(u_short)*((u_char *)p+k)<<8|
					(u_short)*((u_char *)pd+k-headersize);
			}
			A = EXTRACT_SHORT(&pd[k-headersize]);
			
			continue;

		case BPF_LD|BPF_B|BPF_ABS:
			k = pc->k;
			if ((int)k >= (int)buflen) {
				return 0;
			}

			if(k<headersize) A = p[k];
			 else A = pd[k-headersize];

			continue;

		case BPF_LD|BPF_W|BPF_LEN:
			A = wirelen;
			continue;

		case BPF_LDX|BPF_W|BPF_LEN:
			X = wirelen;
			continue;

		case BPF_LD|BPF_W|BPF_IND:
			k = X + pc->k;
			if (k + sizeof(int32) > buflen) {
				return 0;
			}

			if(k + (int)sizeof(int32) < headersize) A = EXTRACT_LONG(&p[k]);
			else if(k + (int)sizeof(int32) == headersize+2){
				A=(u_int32)*((u_char *)p+k)<<24|
					(u_int32)*((u_char *)p+k+1)<<16|
					(u_int32)*((u_char *)p+k+2)<<8|
					(u_int32)*((u_char *)pd+k-headersize);
			}
			else if(k + (int)sizeof(int32) == headersize+3){
				A=(u_int32)*((u_char *)p+k)<<24|
					(u_int32)*((u_char *)p+k+1)<<16|
					(u_int32)*((u_char *)pd+k-headersize)<<8|
					(u_int32)*((u_char *)pd+k-headersize+1);
			}
			else if(k + (int)sizeof(int32) == headersize+4){
				A=(u_int32)*((u_char *)p+k)<<24|
					(u_int32)*((u_char *)pd+k-headersize+1)<<16|
					(u_int32)*((u_char *)pd+k-headersize+2)<<8|
					(u_int32)*((u_char *)pd+k-headersize+3);
			}
			A = EXTRACT_LONG(&pd[k-headersize]);
			
			continue;
			
		case BPF_LD|BPF_H|BPF_IND:
			k = X + pc->k;
			if (k + sizeof(short) > buflen) {
				return 0;
			}
			
			if(k + (int)sizeof(short) < headersize) A = EXTRACT_SHORT(&p[k]);
			else if(k == headersize){
				A=(u_short)*((u_char *)p+k)<<8|
					(u_short)*((u_char *)pd+k-headersize);
			}
			A = EXTRACT_SHORT(&pd[k-headersize]);

			continue;

		case BPF_LD|BPF_B|BPF_IND:
			k = X + pc->k;
			if ((int)k >= (int)buflen) {
				return 0;
			}

			if(k<headersize) A = p[k];
			 else A = pd[k-headersize];

			continue;

		case BPF_LDX|BPF_MSH|BPF_B:
			k = pc->k;
			if ((int)k >= (int)buflen) {
				return 0;
			}
			
			if((pc->k)<headersize) X = (p[pc->k] & 0xf) << 2;
			 else X = (pd[(pc->k)-headersize] & 0xf) << 2;

			continue;

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

		case BPF_ST:
			mem[pc->k] = A;
			continue;

		case BPF_STX:
			mem[pc->k] = X;
			continue;

		case BPF_JMP|BPF_JA:
			pc += pc->k;
			continue;

		case BPF_JMP|BPF_JGT|BPF_K:
			pc += ((int)A > (int)pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JGE|BPF_K:
			pc += ((int)A >= (int)pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JEQ|BPF_K:
			pc += ((int)A == (int)pc->k) ? pc->jt : pc->jf;
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
			(int)A = -((int)A);
			continue;

		case BPF_MISC|BPF_TAX:
			X = A;
			continue;

		case BPF_MISC|BPF_TXA:
			A = X;
			continue;
		}
	}
}


/*
 * Return true if the 'fcode' is a valid filter program.
 * The constraints are that each jump be forward and to a valid
 * code.  The code must terminate with either an accept or reject. 
 * 'valid' is an array for use by the routine (it must be at least
 * 'len' bytes long).  
 *
 * The kernel needs to be able to verify an application's filter code.
 * Otherwise, a bogus program could easily crash the system.
 */
int
bpf_validate(f, len)
	struct bpf_insn *f;
	int len;
{
	register int i;
	register struct bpf_insn *p;

	for (i = 0; i < len; ++i) {
		/*
		 * Check that that jumps are forward, and within 
		 * the code block.
		 */
		p = &f[i];
		if (BPF_CLASS(p->code) == BPF_JMP) {
			register int from = i + 1;

			if (BPF_OP(p->code) == BPF_JA) {
				if (from + p->k >= len)
					return 0;
			}
			else if (from + p->jt >= len || from + p->jf >= len)
				return 0;
		}
		/*
		 * Check that memory operations use valid addresses.
		 */
		if ((BPF_CLASS(p->code) == BPF_ST ||
		     (BPF_CLASS(p->code) == BPF_LD && 
		      (p->code & 0xe0) == BPF_MEM)) &&
		    (p->k >= BPF_MEMWORDS || p->k < 0))
			return 0;
		/*
		 * Check for constant division by 0.
		 */
		if (p->code == (BPF_ALU|BPF_DIV|BPF_K) && p->k == 0)
			return 0;
	}
	return BPF_CLASS(f[len - 1].code) == BPF_RET;
}
