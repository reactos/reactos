/*
 * Copyright (c) 2002
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
 */

#ifdef _MSC_VER
#include "stdarg.h"
#include "ntddk.h"
#include "ntiologc.h"
#include "ndis.h"
#else
#include <ddk/ntddk.h>
#include <net/ndis.h>
#endif

#include "packet.h"
#include "win_bpf.h"

emit_func emitm;

//
// emit routine to update the jump table
//
void emit_lenght(binary_stream *stream, ULONG value, UINT len)
{
	(stream->refs)[stream->bpf_pc]+=len;
	stream->cur_ip+=len;
}

//
// emit routine to output the actual binary code
//
void emit_code(binary_stream *stream, ULONG value, UINT len)
{
	
	switch (len){

	case 1:
		stream->ibuf[stream->cur_ip]=(UCHAR)value;
		stream->cur_ip++;
		break;

	case 2:
		*((USHORT*)(stream->ibuf+stream->cur_ip))=(USHORT)value;
		stream->cur_ip+=2;
		break;

	case 4:
		*((ULONG*)(stream->ibuf+stream->cur_ip))=value;
		stream->cur_ip+=4;
		break;

	default:;
	
	}

	return;

}

//
// Function that does the real stuff
//
BPF_filter_function BPFtoX86(struct bpf_insn *prog, UINT nins, INT *mem)
{
	struct bpf_insn *ins;
	UINT i, pass;
	binary_stream stream;


	// Allocate the reference table for the jumps
#ifdef NTKERNEL
	stream.refs=(UINT *)ExAllocatePoolWithTag(NonPagedPool, (nins + 1)*sizeof(UINT), '0JWA');
#else
	stream.refs=(UINT *)malloc((nins + 1)*sizeof(UINT));
#endif
	if(stream.refs==NULL) 
	{
		return NULL;
	}

	// Reset the reference table
	for(i=0; i< nins + 1; i++)
		stream.refs[i]=0;

	stream.cur_ip=0;
	stream.bpf_pc=0;

	// the first pass will emit the lengths of the instructions 
	// to create the reference table
	emitm=emit_lenght;
	
	for(pass=0;;){

		ins = prog;

		/* create the procedure header */
		PUSH(EBP)
		MOVrd(EBP,ESP)
		PUSH(EBX)
		PUSH(ECX)
		PUSH(EDX)
		PUSH(ESI)
		PUSH(EDI)
		MOVodd(EBX, EBP, 8)

		for(i=0;i<nins;i++){
			
			stream.bpf_pc++;
			
			switch (ins->code) {
				
			default:
				
				return NULL;
				
			case BPF_RET|BPF_K:
				
				MOVid(EAX,ins->k)
				POP(EDI)
				POP(ESI)
				POP(EDX)
				POP(ECX)
				POP(EBX)
				POP(EBP)
				RET()
				
				break;
				

			case BPF_RET|BPF_A:
				
				POP(EDI)
				POP(ESI)
				POP(EDX)
				POP(ECX)
				POP(EBX)
				POP(EBP)
				RET()
				
				break;

				
			case BPF_LD|BPF_W|BPF_ABS:
				
				MOVid(ECX,ins->k)
				MOVrd(ESI,ECX)
				ADDib(ECX,sizeof(INT))
				CMPodd(ECX, EBP, 0x10)
				JLEb(12)
				POP(EDI)
				POP(ESI)
				POP(EDX)
				POP(ECX)
				POP(EBX)
				POP(EBP)
				MOVid(EAX,0)  //this can be optimized with xor eax,eax
				RET()
				MOVobd(EAX, EBX, ESI)
				BSWAP(EAX)

				break;

			case BPF_LD|BPF_H|BPF_ABS:

				MOVid(ECX,ins->k)
				MOVrd(ESI,ECX)
				ADDib(ECX,sizeof(SHORT))
				CMPodd(ECX, EBP, 0x10)
				JLEb(12)
				POP(EDI)
				POP(ESI)
				POP(EDX)
				POP(ECX)
				POP(EBX)
				POP(EBP)
				MOVid(EAX,0)  
				RET()
				MOVid(EAX,0)  
				MOVobw(AX, EBX, ESI)
				SWAP_AX()

				break;
				
			case BPF_LD|BPF_B|BPF_ABS:
			
				MOVid(ECX,ins->k)
				CMPodd(ECX, EBP, 0x10)
				JLEb(12)
				POP(EDI)
				POP(ESI)
				POP(EDX)
				POP(ECX)
				POP(EBX)
				POP(EBP)
				MOVid(EAX,0)  
				RET()
				MOVid(EAX,0)  
				MOVobb(AL,EBX,ECX)

				break;

			case BPF_LD|BPF_W|BPF_LEN:

				MOVodd(EAX, EBP, 0xc)

				break;

			case BPF_LDX|BPF_W|BPF_LEN:

				MOVodd(EDX, EBP, 0xc)

				break;
			
			case BPF_LD|BPF_W|BPF_IND:
			
				MOVid(ECX,ins->k)
				ADDrd(ECX,EDX)
				MOVrd(ESI,ECX)
				ADDib(ECX,sizeof(INT))
				CMPodd(ECX, EBP, 0x10)
				JLEb(12)
				POP(EDI)
				POP(ESI)
				POP(EDX)
				POP(ECX)
				POP(EBX)
				POP(EBP)
				MOVid(EAX,0)  
				RET()
				MOVobd(EAX, EBX, ESI)
				BSWAP(EAX)

				break;

			case BPF_LD|BPF_H|BPF_IND:

				MOVid(ECX,ins->k)
				ADDrd(ECX,EDX)
				MOVrd(ESI,ECX)
				ADDib(ECX,sizeof(SHORT))
				CMPodd(ECX, EBP, 0x10)
				JLEb(12)
				POP(EDI)
				POP(ESI)
				POP(EDX)
				POP(ECX)
				POP(EBX)
				POP(EBP)
				MOVid(EAX,0)  
				RET()
				MOVid(EAX,0)  
				MOVobw(AX, EBX, ESI)
				SWAP_AX()

				break;

			case BPF_LD|BPF_B|BPF_IND:

				MOVid(ECX,ins->k)
				ADDrd(ECX,EDX)
				CMPodd(ECX, EBP, 0x10)
				JLEb(12)
				POP(EDI)
				POP(ESI)
				POP(EDX)
				POP(ECX)
				POP(EBX)
				POP(EBP)
				MOVid(EAX,0)  
				RET()
				MOVid(EAX,0)  
				MOVobb(AL,EBX,ECX)

				break;

			case BPF_LDX|BPF_MSH|BPF_B:

				MOVid(ECX,ins->k)
				CMPodd(ECX, EBP, 0x10)
				JLEb(12)
				POP(EDI)
				POP(ESI)
				POP(EDX)
				POP(ECX)
				POP(EBX)
				POP(EBP)
				MOVid(EAX,0)  
				RET()
				MOVid(EDX,0)
				MOVobb(DL,EBX,ECX)
				ANDib(DL, 0xf)
				SHLib(EDX, 2)
								
				break;

			case BPF_LD|BPF_IMM:

				MOVid(EAX,ins->k)

				break;

			case BPF_LDX|BPF_IMM:
			
				MOVid(EDX,ins->k)

				break;

			case BPF_LD|BPF_MEM:

				MOVid(ECX,(INT)mem)
				MOVid(ESI,ins->k*4)
				MOVobd(EAX, ECX, ESI)

				break;

			case BPF_LDX|BPF_MEM:

				MOVid(ECX,(INT)mem)
				MOVid(ESI,ins->k*4)
				MOVobd(EDX, ECX, ESI)

				break;

			case BPF_ST:

				// XXX: this command and the following could be optimized if the previous
				// instruction was already of this type
				MOVid(ECX,(INT)mem)
				MOVid(ESI,ins->k*4)
				MOVomd(ECX, ESI, EAX)

				break;

			case BPF_STX:

				MOVid(ECX,(INT)mem)
				MOVid(ESI,ins->k*4)
				MOVomd(ECX, ESI, EDX)
				break;

			case BPF_JMP|BPF_JA:

				JMP(stream.refs[stream.bpf_pc+ins->k]-stream.refs[stream.bpf_pc])

				break;

			case BPF_JMP|BPF_JGT|BPF_K:

				CMPid(EAX, ins->k)
				JG(stream.refs[stream.bpf_pc+ins->jt]-stream.refs[stream.bpf_pc]+5) // 5 is the size of the following JMP
				JMP(stream.refs[stream.bpf_pc+ins->jf]-stream.refs[stream.bpf_pc])				
				break;

			case BPF_JMP|BPF_JGE|BPF_K:

				CMPid(EAX, ins->k)
				JGE(stream.refs[stream.bpf_pc+ins->jt]-stream.refs[stream.bpf_pc]+5)
				JMP(stream.refs[stream.bpf_pc+ins->jf]-stream.refs[stream.bpf_pc])				

				break;

			case BPF_JMP|BPF_JEQ|BPF_K:

				CMPid(EAX, ins->k)
				JE(stream.refs[stream.bpf_pc+ins->jt]-stream.refs[stream.bpf_pc]+5) 
				JMP(stream.refs[stream.bpf_pc+ins->jf]-stream.refs[stream.bpf_pc])				

				break;

			case BPF_JMP|BPF_JSET|BPF_K:

				MOVrd(ECX,EAX)
				ANDid(ECX,ins->k)
				JE(stream.refs[stream.bpf_pc+ins->jf]-stream.refs[stream.bpf_pc]+5)
				JMP(stream.refs[stream.bpf_pc+ins->jt]-stream.refs[stream.bpf_pc])				

				break;

			case BPF_JMP|BPF_JGT|BPF_X:

				CMPrd(EAX, EDX)
				JA(stream.refs[stream.bpf_pc+ins->jt]-stream.refs[stream.bpf_pc]+5)
				JMP(stream.refs[stream.bpf_pc+ins->jf]-stream.refs[stream.bpf_pc])				
				break;

			case BPF_JMP|BPF_JGE|BPF_X:

				CMPrd(EAX, EDX)
				JAE(stream.refs[stream.bpf_pc+ins->jt]-stream.refs[stream.bpf_pc]+5)
				JMP(stream.refs[stream.bpf_pc+ins->jf]-stream.refs[stream.bpf_pc])				

				break;

			case BPF_JMP|BPF_JEQ|BPF_X:

				CMPrd(EAX, EDX)
				JE(stream.refs[stream.bpf_pc+ins->jt]-stream.refs[stream.bpf_pc]+5)
				JMP(stream.refs[stream.bpf_pc+ins->jf]-stream.refs[stream.bpf_pc])				

				break;

			case BPF_JMP|BPF_JSET|BPF_X:

				MOVrd(ECX,EAX)
				ANDrd(ECX,EDX)
				JE(stream.refs[stream.bpf_pc+ins->jf]-stream.refs[stream.bpf_pc]+5)
				JMP(stream.refs[stream.bpf_pc+ins->jt]-stream.refs[stream.bpf_pc])				
				
				break;

			case BPF_ALU|BPF_ADD|BPF_X:

				ADDrd(EAX,EDX)
				
				break;

			case BPF_ALU|BPF_SUB|BPF_X:

				SUBrd(EAX,EDX)

				break;

			case BPF_ALU|BPF_MUL|BPF_X:

				MOVrd(ECX,EDX)
				MULrd(EDX)
				MOVrd(EDX,ECX)
				break;

			case BPF_ALU|BPF_DIV|BPF_X:

				CMPid(EDX, 0)
				JNEb(12)
				POP(EDI)
				POP(ESI)
				POP(EDX)
				POP(ECX)
				POP(EBX)
				POP(EBP)
				MOVid(EAX,0)  
				RET()
				MOVrd(ECX,EDX)
				MOVid(EDX,0)  
				DIVrd(ECX)
				MOVrd(EDX,ECX)

				break;

			case BPF_ALU|BPF_AND|BPF_X:

				ANDrd(EAX,EDX)
				
				break;

			case BPF_ALU|BPF_OR|BPF_X:

				ORrd(EAX,EDX)

				break;

			case BPF_ALU|BPF_LSH|BPF_X:

				MOVrd(ECX,EDX)
				SHL_CLrb(EAX)

				break;

			case BPF_ALU|BPF_RSH|BPF_X:

				MOVrd(ECX,EDX)
				SHR_CLrb(EAX)

				break;

			case BPF_ALU|BPF_ADD|BPF_K:

				ADD_EAXi(ins->k)

				break;

			case BPF_ALU|BPF_SUB|BPF_K:

				SUB_EAXi(ins->k)
				
				break;

			case BPF_ALU|BPF_MUL|BPF_K:

				MOVrd(ECX,EDX)
				MOVid(EDX,ins->k)  
				MULrd(EDX)
				MOVrd(EDX,ECX)

				break;

			case BPF_ALU|BPF_DIV|BPF_K:

				MOVrd(ECX,EDX)
				MOVid(EDX,0)  
				MOVid(ESI,ins->k)
				DIVrd(ESI)
				MOVrd(EDX,ECX)

				break;

			case BPF_ALU|BPF_AND|BPF_K:

				ANDid(EAX, ins->k)

				break;

			case BPF_ALU|BPF_OR|BPF_K:

				ORid(EAX, ins->k)
				
				break;

			case BPF_ALU|BPF_LSH|BPF_K:

				SHLib(EAX, (ins->k) & 255)

				break;

			case BPF_ALU|BPF_RSH|BPF_K:

				SHRib(EAX, (ins->k) & 255)

				break;

			case BPF_ALU|BPF_NEG:

				NEGd(EAX)

				break;

			case BPF_MISC|BPF_TAX:

				MOVrd(EDX,EAX)

				break;

			case BPF_MISC|BPF_TXA:

				MOVrd(EAX,EDX)

				break;



			}
		
			ins++;	
		}

		pass++;
		if(pass == 2) break;
		
#ifdef NTKERNEL
		stream.ibuf=(CHAR*)ExAllocatePoolWithTag(NonPagedPool, stream.cur_ip, '1JWA');
#else
		stream.ibuf=(CHAR*)malloc(stream.cur_ip);
#endif
		if(stream.ibuf==NULL) 
		{
#ifdef NTKERNEL
			ExFreePool(stream.refs);
#else
			free(stream.refs);
#endif
			return NULL;
		}
		
		// modify the reference table to contain the offsets and not the lengths of the instructions
		for(i=1; i< nins + 1; i++)
			stream.refs[i]+=stream.refs[i-1];

		// Reset the counters
		stream.cur_ip=0;
		stream.bpf_pc=0;
		// the second pass creates the actual code
		emitm=emit_code;

	}

	// the reference table is needed only during compilation, now we can free it
#ifdef NTKERNEL
	ExFreePool(stream.refs);
#else
	free(stream.refs);
#endif
	return (BPF_filter_function)stream.ibuf;

}


JIT_BPF_Filter* BPF_jitter(struct bpf_insn *fp, INT nins)
{
	JIT_BPF_Filter *Filter;


	// Allocate the filter structure
#ifdef NTKERNEL
	Filter=(struct JIT_BPF_Filter*)ExAllocatePoolWithTag(NonPagedPool, sizeof(struct JIT_BPF_Filter), '2JWA');
#else
	Filter=(struct JIT_BPF_Filter*)malloc(sizeof(struct JIT_BPF_Filter));
#endif
	if(Filter==NULL)
	{
		return NULL;
	}

	// Allocate the filter's memory
#ifdef NTKERNEL
	Filter->mem=(INT*)ExAllocatePoolWithTag(NonPagedPool, BPF_MEMWORDS*sizeof(INT), '3JWA');
#else
	Filter->mem=(INT*)malloc(BPF_MEMWORDS*sizeof(INT));
#endif
	if(Filter->mem==NULL)
	{
#ifdef NTKERNEL
		ExFreePool(Filter);
#else
		free(Filter);
#endif
		return NULL;
	}

	// Create the binary
	if((Filter->Function = BPFtoX86(fp, nins, Filter->mem))==NULL)
	{
#ifdef NTKERNEL
		ExFreePool(Filter->mem);
		ExFreePool(Filter);
#else
		free(Filter->mem);
		free(Filter);

		return NULL;
#endif
	}

	return Filter;

}

//////////////////////////////////////////////////////////////

void BPF_Destroy_JIT_Filter(JIT_BPF_Filter *Filter){
	
#ifdef NTKERNEL
	ExFreePool(Filter->mem);
	ExFreePool(Filter->Function);
	ExFreePool(Filter);
#else
	free(Filter->mem);
	free(Filter->Function);
	free(Filter);
#endif

}
