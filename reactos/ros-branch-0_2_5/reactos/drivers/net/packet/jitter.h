/*
 * Copyright (c) 2002
 *  Politecnico di Torino.  All rights reserved.
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

/** @ingroup NPF 
 *  @{
 */

/** @defgroup NPF_include NPF structures and definitions 
 *  @{
 */

//
// Registers
//
#define EAX 0
#define ECX 1
#define EDX 2
#define EBX 3
#define ESP 4
#define EBP 5
#define ESI 6
#define EDI 7

#define AX 0
#define CX 1
#define DX 2
#define BX 3
#define SP 4
#define BP 5
#define SI 6
#define DI 7

#define AL 0
#define CL 1
#define DL 2
#define BL 3

/*! \brief A stream of X86 binary code.*/
typedef struct binary_stream{
    INT cur_ip;     ///< Current X86 instruction pointer.
    INT bpf_pc;     ///< Current BPF instruction pointer, i.e. position in the BPF program reached by the jitter.
    PCHAR ibuf;     ///< Instruction buffer, contains the X86 generated code.
    PUINT refs;     ///< Jumps reference table.
}binary_stream;


/*! \brief Prototype of a filtering function created by the jitter. 

  The syntax and the meaning of the parameters is analogous to the one of bpf_filter(). Notice that the filter
  is not among the parameters, because it is hardwired in the function.
*/
typedef UINT (*BPF_filter_function)( binary_stream *, ULONG, UINT);

/*! \brief Prototype of the emit functions.

  Different emit functions are used to create the reference table and to generate the actual filtering code.
  This allows to have simpler instruction macros.
  The first parameter is the stream that will receive the data. The secon one is a variable containing
  the data, the third one is the length, that can be 1,2 or 4 since it is possible to emit a byte, a short
  or a work at a time.
*/
typedef void (*emit_func)(binary_stream *stream, ULONG value, UINT n);

/*! \brief Structure describing a x86 filtering program created by the jitter.*/
typedef struct JIT_BPF_Filter{
    BPF_filter_function Function;   ///< The x86 filtering binary, in the form of a BPF_filter_function.
    PINT mem;
}
JIT_BPF_Filter;




/**************************/
/* X86 INSTRUCTION MACROS */
/**************************/

/// mov r32,i32
#define MOVid(r32, i32) \
  emitm(&stream, 11 << 4 | 1 << 3 | r32 & 0x7, 1); emitm(&stream, i32, 4);

/// mov dr32,sr32
#define MOVrd(dr32, sr32) \
  emitm(&stream, 8 << 4 | 3 | 1 << 3, 1); emitm(&stream,  3 << 6 | (dr32 & 0x7) << 3 | sr32 & 0x7, 1);

/// mov dr32,sr32[off]
#define MOVodd(dr32, sr32, off) \
  emitm(&stream, 8 << 4 | 3 | 1 << 3, 1); \
  emitm(&stream,  1 << 6 | (dr32 & 0x7) << 3 | sr32 & 0x7, 1);\
  emitm(&stream,  off, 1);

/// mov dr32,sr32[or32]
#define MOVobd(dr32, sr32, or32) \
  emitm(&stream, 8 << 4 | 3 | 1 << 3, 1); \
  emitm(&stream,  (dr32 & 0x7) << 3 | 4 , 1);\
  emitm(&stream,  (or32 & 0x7) << 3 | (sr32 & 0x7) , 1);

/// mov dr16,sr32[or32]
#define MOVobw(dr32, sr32, or32) \
  emitm(&stream, 0x66, 1); \
  emitm(&stream, 8 << 4 | 3 | 1 << 3, 1); \
  emitm(&stream,  (dr32 & 0x7) << 3 | 4 , 1);\
  emitm(&stream,  (or32 & 0x7) << 3 | (sr32 & 0x7) , 1);

/// mov dr8,sr32[or32]
#define MOVobb(dr8, sr32, or32) \
  emitm(&stream, 0x8a, 1); \
  emitm(&stream,  (dr8 & 0x7) << 3 | 4 , 1);\
  emitm(&stream,  (or32 & 0x7) << 3 | (sr32 & 0x7) , 1);

/// mov [dr32][or32],sr32
#define MOVomd(dr32, or32, sr32) \
  emitm(&stream, 0x89, 1); \
  emitm(&stream,  (sr32 & 0x7) << 3 | 4 , 1);\
  emitm(&stream,  (or32 & 0x7) << 3 | (dr32 & 0x7) , 1);

/// bswap dr32
#define BSWAP(dr32) \
  emitm(&stream, 0xf, 1); \
  emitm(&stream,  0x19 << 3 | dr32 , 1);

/// xchg al,ah
#define SWAP_AX() \
  emitm(&stream, 0x86, 1); \
  emitm(&stream,  0xc4 , 1);

/// push r32
#define PUSH(r32) \
  emitm(&stream, 5 << 4 | 0 << 3 | r32 & 0x7, 1);

/// pop r32
#define POP(r32) \
  emitm(&stream, 5 << 4 | 1 << 3 | r32 & 0x7, 1);

/// ret
#define RET() \
  emitm(&stream, 12 << 4 | 0 << 3 | 3, 1);

/// add dr32,sr32
#define ADDrd(dr32, sr32) \
  emitm(&stream, 0x03, 1);\
  emitm(&stream, 3 << 6 | (dr32 & 0x7) << 3 | (sr32 & 0x7), 1);

/// add eax,i32
#define ADD_EAXi(i32) \
  emitm(&stream, 0x05, 1);\
  emitm(&stream, i32, 4);

/// add r32,i32
#define ADDid(r32, i32) \
  emitm(&stream, 0x81, 1);\
  emitm(&stream, 24 << 3 | r32, 1);\
  emitm(&stream, i32, 4);

/// add r32,i8
#define ADDib(r32, i8) \
  emitm(&stream, 0x83, 1);\
  emitm(&stream, 24 << 3 | r32, 1);\
  emitm(&stream, i8, 1);

/// sub dr32,sr32
#define SUBrd(dr32, sr32) \
  emitm(&stream, 0x2b, 1);\
  emitm(&stream, 3 << 6 | (dr32 & 0x7) << 3 | (sr32 & 0x7), 1);

/// sub eax,i32
#define SUB_EAXi(i32) \
  emitm(&stream, 0x2d, 1);\
  emitm(&stream, i32, 4);

/// mul r32
#define MULrd(r32) \
  emitm(&stream, 0xf7, 1);\
  emitm(&stream, 7 << 5 | (r32 & 0x7), 1);

/// div r32
#define DIVrd(r32) \
  emitm(&stream, 0xf7, 1);\
  emitm(&stream, 15 << 4 | (r32 & 0x7), 1);

/// and r8,i8
#define ANDib(r8, i8) \
  emitm(&stream, 0x80, 1);\
  emitm(&stream, 7 << 5 | r8, 1);\
  emitm(&stream, i8, 1);

/// and r32,i32
#define ANDid(r32, i32) \
  if (r32 == EAX){ \
  emitm(&stream, 0x25, 1);\
  emitm(&stream, i32, 4);}\
  else{ \
  emitm(&stream, 0x81, 1);\
  emitm(&stream, 7 << 5 | r32, 1);\
  emitm(&stream, i32, 4);}

/// and dr32,sr32
#define ANDrd(dr32, sr32) \
  emitm(&stream, 0x23, 1);\
  emitm(&stream,  3 << 6 | (dr32 & 0x7) << 3 | sr32 & 0x7, 1);

/// or dr32,sr32
#define ORrd(dr32, sr32) \
  emitm(&stream, 0x0b, 1);\
  emitm(&stream,  3 << 6 | (dr32 & 0x7) << 3 | sr32 & 0x7, 1);

/// or r32,i32
#define ORid(r32, i32) \
  if (r32 == EAX){ \
  emitm(&stream, 0x0d, 1);\
  emitm(&stream, i32, 4);}\
  else{ \
  emitm(&stream, 0x81, 1);\
  emitm(&stream, 25 << 3 | r32, 1);\
  emitm(&stream, i32, 4);}

/// shl r32,i8
#define SHLib(r32, i8) \
  emitm(&stream, 0xc1, 1);\
  emitm(&stream, 7 << 5 | r32 & 0x7, 1);\
  emitm(&stream, i8, 1);

/// shl dr32,cl
#define SHL_CLrb(dr32) \
  emitm(&stream, 0xd3, 1);\
  emitm(&stream,  7 << 5 | dr32 & 0x7, 1);

/// shr r32,i8
#define SHRib(r32, i8) \
  emitm(&stream, 0xc1, 1);\
  emitm(&stream, 29 << 3 | r32 & 0x7, 1);\
  emitm(&stream, i8, 1);

/// shr dr32,cl
#define SHR_CLrb(dr32) \
  emitm(&stream, 0xd3, 1);\
  emitm(&stream,  29 << 3 | dr32 & 0x7, 1);

/// neg r32
#define NEGd(r32) \
  emitm(&stream, 0xf7, 1);\
  emitm(&stream,  27 << 3 | r32 & 0x7, 1);

/// cmp dr32,sr32[off]
#define CMPodd(dr32, sr32, off) \
  emitm(&stream, 3 << 4 | 3 | 1 << 3, 1); \
  emitm(&stream,  1 << 6 | (dr32 & 0x7) << 3 | sr32 & 0x7, 1);\
  emitm(&stream,  off, 1);

/// cmp dr32,sr32
#define CMPrd(dr32, sr32) \
  emitm(&stream, 0x3b, 1); \
  emitm(&stream,  3 << 6 | (dr32 & 0x7) << 3 | sr32 & 0x7, 1);

/// cmp dr32,i32
#define CMPid(dr32, i32) \
  if (dr32 == EAX){ \
  emitm(&stream, 0x3d, 1); \
  emitm(&stream,  i32, 4);} \
  else{ \
  emitm(&stream, 0x81, 1); \
  emitm(&stream,  0x1f << 3 | (dr32 & 0x7), 1);\
  emitm(&stream,  i32, 4);}

/// jne off32
#define JNEb(off8) \
   emitm(&stream, 0x75, 1);\
   emitm(&stream, off8, 1);

/// je off32
#define JE(off32) \
   emitm(&stream, 0x0f, 1);\
   emitm(&stream, 0x84, 1);\
   emitm(&stream, off32, 4);

/// jle off32
#define JLE(off32) \
   emitm(&stream, 0x0f, 1);\
   emitm(&stream, 0x8e, 1);\
   emitm(&stream, off32, 4);

/// jle off8
#define JLEb(off8) \
   emitm(&stream, 0x7e, 1);\
   emitm(&stream, off8, 1);

/// ja off32
#define JA(off32) \
   emitm(&stream, 0x0f, 1);\
   emitm(&stream, 0x87, 1);\
   emitm(&stream, off32, 4);
   
/// jae off32
#define JAE(off32) \
   emitm(&stream, 0x0f, 1);\
   emitm(&stream, 0x83, 1);\
   emitm(&stream, off32, 4);

/// jg off32
#define JG(off32) \
   emitm(&stream, 0x0f, 1);\
   emitm(&stream, 0x8f, 1);\
   emitm(&stream, off32, 4);

/// jge off32
#define JGE(off32) \
   emitm(&stream, 0x0f, 1);\
   emitm(&stream, 0x8d, 1);\
   emitm(&stream, off32, 4);

/// jmp off32
#define JMP(off32) \
   emitm(&stream, 0xe9, 1);\
   emitm(&stream, off32, 4);

/**
 *  @}
 */

/**
 *  @}
 */

/**************************/
/* Prototypes             */
/**************************/

/** @ingroup NPF 
 *  @{
 */

/** @defgroup NPF_code NPF functions 
 *  @{
 */

/*!
  \brief BPF jitter, builds an x86 function from a BPF program.
  \param fp The BPF pseudo-assembly filter that will be translated into x86 code.
  \param nins Number of instructions of the input filter.
  \return The JIT_BPF_Filter structure containing the x86 filtering binary.

  BPF_jitter allocates the buffers for the new native filter and then translates the program pointed by fp
  calling BPFtoX86().
*/ 
JIT_BPF_Filter* BPF_jitter(struct bpf_insn *fp, INT nins);

/*!
  \brief Translates a set of BPF instructions in a set of x86 ones.
  \param ins Pointer to the BPF instructions that will be translated into x86 code.
  \param nins Number of instructions to translate.
  \param mem Memory used by the x86 function to emulate the RAM of the BPF pseudo processor.
  \return The x86 filtering function.

  This function does the hard work for the JIT compilation. It takes a group of BPF pseudo instructions and 
  through the instruction macros defined in jitter.h it is able to create an function directly executable
  by NPF.
*/ 
BPF_filter_function BPFtoX86(struct bpf_insn *ins, UINT nins, INT *mem);
/*!
  \brief Deletes a filtering function that was previously created by BPF_jitter().
  \param Filter The filter to destroy.

  This function frees the variuos buffers (code, memory, etc.) associated with a filtering function.
*/ 
void BPF_Destroy_JIT_Filter(JIT_BPF_Filter *Filter);

/**
 *  @}
 */

/**
 *  @}
 */
