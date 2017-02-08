/**************************************************************************
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "pipe/p_config.h"
#include "util/u_cpu_detect.h"

#if defined(PIPE_ARCH_X86) || (defined(PIPE_ARCH_X86_64) && !defined(__MINGW32__))

#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "util/u_pointer.h"

#include "rtasm_execmem.h"
#include "rtasm_x86sse.h"

#define DISASSEM 0
#define X86_TWOB 0x0f


#define DUMP_SSE  0


void x86_print_reg( struct x86_reg reg )
{
   if (reg.mod != mod_REG) 
      debug_printf( "[" );
      
   switch( reg.file ) {
   case file_REG32:
      switch( reg.idx ) {
      case reg_AX: debug_printf( "EAX" ); break;
      case reg_CX: debug_printf( "ECX" ); break;
      case reg_DX: debug_printf( "EDX" ); break;
      case reg_BX: debug_printf( "EBX" ); break;
      case reg_SP: debug_printf( "ESP" ); break;
      case reg_BP: debug_printf( "EBP" ); break;
      case reg_SI: debug_printf( "ESI" ); break;
      case reg_DI: debug_printf( "EDI" ); break;
      }
      break;
   case file_MMX:
      debug_printf( "MMX%u", reg.idx );
      break;
   case file_XMM:
      debug_printf( "XMM%u", reg.idx );
      break;
   case file_x87:
      debug_printf( "fp%u", reg.idx );
      break;
   }

   if (reg.mod == mod_DISP8 ||
       reg.mod == mod_DISP32)
      debug_printf("+%d", reg.disp);

   if (reg.mod != mod_REG) 
      debug_printf( "]" );
}

#if DUMP_SSE

#define DUMP_START() debug_printf( "\n" )
#define DUMP_END() debug_printf( "\n" )

#define DUMP() do {                             \
   const char *foo = __FUNCTION__;              \
   while (*foo && *foo != '_')                  \
      foo++;                                    \
   if  (*foo)                                   \
      foo++;                                    \
   debug_printf( "\n%4x %14s ", p->csr - p->store, foo );             \
} while (0)

#define DUMP_I( I ) do {                        \
   DUMP();                                      \
   debug_printf( "%u", I );                     \
} while( 0 )

#define DUMP_R( R0 ) do {                       \
   DUMP();                                      \
   x86_print_reg( R0 );                            \
} while( 0 )

#define DUMP_RR( R0, R1 ) do {                  \
   DUMP();                                      \
   x86_print_reg( R0 );                            \
   debug_printf( ", " );                        \
   x86_print_reg( R1 );                            \
} while( 0 )

#define DUMP_RI( R0, I ) do {                   \
   DUMP();                                      \
   x86_print_reg( R0 );                            \
   debug_printf( ", %u", I );                   \
} while( 0 )

#define DUMP_RRI( R0, R1, I ) do {              \
   DUMP();                                      \
   x86_print_reg( R0 );                            \
   debug_printf( ", " );                        \
   x86_print_reg( R1 );                            \
   debug_printf( ", %u", I );                   \
} while( 0 )

#else

#define DUMP_START()
#define DUMP_END()
#define DUMP( )
#define DUMP_I( I )
#define DUMP_R( R0 )
#define DUMP_RR( R0, R1 )
#define DUMP_RI( R0, I )
#define DUMP_RRI( R0, R1, I )

#endif


static void do_realloc( struct x86_function *p )
{
   if (p->store == p->error_overflow) {
      p->csr = p->store;
   }
   else if (p->size == 0) {
      p->size = 1024;
      p->store = rtasm_exec_malloc(p->size);
      p->csr = p->store;
   }
   else {
      uintptr_t used = pointer_to_uintptr( p->csr ) - pointer_to_uintptr( p->store );
      unsigned char *tmp = p->store;
      p->size *= 2;
      p->store = rtasm_exec_malloc(p->size);

      if (p->store) {
         memcpy(p->store, tmp, used);
         p->csr = p->store + used;
      }
      else {
         p->csr = p->store;
      }

      rtasm_exec_free(tmp);
   }

   if (p->store == NULL) {
      p->store = p->csr = p->error_overflow;
      p->size = sizeof(p->error_overflow);
   }
}

/* Emit bytes to the instruction stream:
 */
static unsigned char *reserve( struct x86_function *p, int bytes )
{
   if (p->csr + bytes - p->store > (int) p->size)
      do_realloc(p);

   {
      unsigned char *csr = p->csr;
      p->csr += bytes;
      return csr;
   }
}



static void emit_1b( struct x86_function *p, char b0 )
{
   char *csr = (char *)reserve(p, 1);
   *csr = b0;
}

static void emit_1i( struct x86_function *p, int i0 )
{
   int *icsr = (int *)reserve(p, sizeof(i0));
   *icsr = i0;
}

static void emit_1ub( struct x86_function *p, unsigned char b0 )
{
   unsigned char *csr = reserve(p, 1);
   *csr++ = b0;
}

static void emit_2ub( struct x86_function *p, unsigned char b0, unsigned char b1 )
{
   unsigned char *csr = reserve(p, 2);
   *csr++ = b0;
   *csr++ = b1;
}

static void emit_3ub( struct x86_function *p, unsigned char b0, unsigned char b1, unsigned char b2 )
{
   unsigned char *csr = reserve(p, 3);
   *csr++ = b0;
   *csr++ = b1;
   *csr++ = b2;
}


/* Build a modRM byte + possible displacement.  No treatment of SIB
 * indexing.  BZZT - no way to encode an absolute address.
 *
 * This is the "/r" field in the x86 manuals...
 */
static void emit_modrm( struct x86_function *p, 
			struct x86_reg reg, 
			struct x86_reg regmem )
{
   unsigned char val = 0;
   
   assert(reg.mod == mod_REG);
   
   /* TODO: support extended x86-64 registers */
   assert(reg.idx < 8);
   assert(regmem.idx < 8);

   val |= regmem.mod << 6;     	/* mod field */
   val |= reg.idx << 3;		/* reg field */
   val |= regmem.idx;		/* r/m field */
   
   emit_1ub(p, val);

   /* Oh-oh we've stumbled into the SIB thing.
    */
   if (regmem.file == file_REG32 &&
       regmem.idx == reg_SP &&
       regmem.mod != mod_REG) {
      emit_1ub(p, 0x24);		/* simplistic! */
   }

   switch (regmem.mod) {
   case mod_REG:
   case mod_INDIRECT:
      break;
   case mod_DISP8:
      emit_1b(p, (char) regmem.disp);
      break;
   case mod_DISP32:
      emit_1i(p, regmem.disp);
      break;
   default:
      assert(0);
      break;
   }
}

/* Emits the "/0".."/7" specialized versions of the modrm ("/r") bytes.
 */
static void emit_modrm_noreg( struct x86_function *p,
			      unsigned op,
			      struct x86_reg regmem )
{
   struct x86_reg dummy = x86_make_reg(file_REG32, op);
   emit_modrm(p, dummy, regmem);
}

/* Many x86 instructions have two opcodes to cope with the situations
 * where the destination is a register or memory reference
 * respectively.  This function selects the correct opcode based on
 * the arguments presented.
 */
static void emit_op_modrm( struct x86_function *p,
			   unsigned char op_dst_is_reg, 
			   unsigned char op_dst_is_mem,
			   struct x86_reg dst,
			   struct x86_reg src )
{  
   switch (dst.mod) {
   case mod_REG:
      emit_1ub(p, op_dst_is_reg);
      emit_modrm(p, dst, src);
      break;
   case mod_INDIRECT:
   case mod_DISP32:
   case mod_DISP8:
      assert(src.mod == mod_REG);
      emit_1ub(p, op_dst_is_mem);
      emit_modrm(p, src, dst);
      break;
   default:
      assert(0);
      break;
   }
}







/* Create and manipulate registers and regmem values:
 */
struct x86_reg x86_make_reg( enum x86_reg_file file,
			     enum x86_reg_name idx )
{
   struct x86_reg reg;

   reg.file = file;
   reg.idx = idx;
   reg.mod = mod_REG;
   reg.disp = 0;

   return reg;
}

struct x86_reg x86_make_disp( struct x86_reg reg,
			      int disp )
{
   assert(reg.file == file_REG32);

   if (reg.mod == mod_REG)
      reg.disp = disp;
   else
      reg.disp += disp;

   if (reg.disp == 0 && reg.idx != reg_BP)
      reg.mod = mod_INDIRECT;
   else if (reg.disp <= 127 && reg.disp >= -128)
      reg.mod = mod_DISP8;
   else
      reg.mod = mod_DISP32;

   return reg;
}

struct x86_reg x86_deref( struct x86_reg reg )
{
   return x86_make_disp(reg, 0);
}

struct x86_reg x86_get_base_reg( struct x86_reg reg )
{
   return x86_make_reg( reg.file, reg.idx );
}

int x86_get_label( struct x86_function *p )
{
   return p->csr - p->store;
}



/***********************************************************************
 * x86 instructions
 */


void x64_rexw(struct x86_function *p)
{
   if(x86_target(p) != X86_32)
      emit_1ub(p, 0x48);
}

void x86_jcc( struct x86_function *p,
	      enum x86_cc cc,
	      int label )
{
   int offset = label - (x86_get_label(p) + 2);
   DUMP_I(cc);
   
   if (offset < 0) {
      /*assert(p->csr - p->store > -offset);*/
      if (p->csr - p->store <= -offset) {
         /* probably out of memory (using the error_overflow buffer) */
         return;
      }
   }

   if (offset <= 127 && offset >= -128) {
      emit_1ub(p, 0x70 + cc);
      emit_1b(p, (char) offset);
   }
   else {
      offset = label - (x86_get_label(p) + 6);
      emit_2ub(p, 0x0f, 0x80 + cc);
      emit_1i(p, offset);
   }
}

/* Always use a 32bit offset for forward jumps:
 */
int x86_jcc_forward( struct x86_function *p,
                     enum x86_cc cc )
{
   DUMP_I(cc);
   emit_2ub(p, 0x0f, 0x80 + cc);
   emit_1i(p, 0);
   return x86_get_label(p);
}

int x86_jmp_forward( struct x86_function *p)
{
   DUMP();
   emit_1ub(p, 0xe9);
   emit_1i(p, 0);
   return x86_get_label(p);
}

int x86_call_forward( struct x86_function *p)
{
   DUMP();

   emit_1ub(p, 0xe8);
   emit_1i(p, 0);
   return x86_get_label(p);
}

/* Fixup offset from forward jump:
 */
void x86_fixup_fwd_jump( struct x86_function *p,
			 int fixup )
{
   *(int *)(p->store + fixup - 4) = x86_get_label(p) - fixup;
}

void x86_jmp( struct x86_function *p, int label)
{
   DUMP_I( label );
   emit_1ub(p, 0xe9);
   emit_1i(p, label - x86_get_label(p) - 4);
}

void x86_call( struct x86_function *p, struct x86_reg reg)
{
   DUMP_R( reg );
   emit_1ub(p, 0xff);
   emit_modrm_noreg(p, 2, reg);
}


void x86_mov_reg_imm( struct x86_function *p, struct x86_reg dst, int imm )
{
   DUMP_RI( dst, imm );
   assert(dst.file == file_REG32);
   assert(dst.mod == mod_REG);
   emit_1ub(p, 0xb8 + dst.idx);
   emit_1i(p, imm);
}

void x86_mov_imm( struct x86_function *p, struct x86_reg dst, int imm )
{
   DUMP_RI( dst, imm );
   if(dst.mod == mod_REG)
      x86_mov_reg_imm(p, dst, imm);
   else
   {
      emit_1ub(p, 0xc7);
      emit_modrm_noreg(p, 0, dst);
      emit_1i(p, imm);
   }
}

void x86_mov16_imm( struct x86_function *p, struct x86_reg dst, uint16_t imm )
{
   DUMP_RI( dst, imm );
   emit_1ub(p, 0x66);
   if(dst.mod == mod_REG)
   {
      emit_1ub(p, 0xb8 + dst.idx);
      emit_2ub(p, imm & 0xff, imm >> 8);
   }
   else
   {
      emit_1ub(p, 0xc7);
      emit_modrm_noreg(p, 0, dst);
      emit_2ub(p, imm & 0xff, imm >> 8);
   }
}

void x86_mov8_imm( struct x86_function *p, struct x86_reg dst, uint8_t imm )
{
   DUMP_RI( dst, imm );
   if(dst.mod == mod_REG)
   {
      emit_1ub(p, 0xb0 + dst.idx);
      emit_1ub(p, imm);
   }
   else
   {
      emit_1ub(p, 0xc6);
      emit_modrm_noreg(p, 0, dst);
      emit_1ub(p, imm);
   }
}

/**
 * Immediate group 1 instructions.
 */
static INLINE void 
x86_group1_imm( struct x86_function *p, 
                unsigned op, struct x86_reg dst, int imm )
{
   assert(dst.file == file_REG32);
   assert(dst.mod == mod_REG);
   if(-0x80 <= imm && imm < 0x80) {
      emit_1ub(p, 0x83);
      emit_modrm_noreg(p, op, dst);
      emit_1b(p, (char)imm);
   }
   else {
      emit_1ub(p, 0x81);
      emit_modrm_noreg(p, op, dst);
      emit_1i(p, imm);
   }
}

void x86_add_imm( struct x86_function *p, struct x86_reg dst, int imm )
{
   DUMP_RI( dst, imm );
   x86_group1_imm(p, 0, dst, imm);
}

void x86_or_imm( struct x86_function *p, struct x86_reg dst, int imm )
{
   DUMP_RI( dst, imm );
   x86_group1_imm(p, 1, dst, imm);
}

void x86_and_imm( struct x86_function *p, struct x86_reg dst, int imm )
{
   DUMP_RI( dst, imm );
   x86_group1_imm(p, 4, dst, imm);
}

void x86_sub_imm( struct x86_function *p, struct x86_reg dst, int imm )
{
   DUMP_RI( dst, imm );
   x86_group1_imm(p, 5, dst, imm);
}

void x86_xor_imm( struct x86_function *p, struct x86_reg dst, int imm )
{
   DUMP_RI( dst, imm );
   x86_group1_imm(p, 6, dst, imm);
}

void x86_cmp_imm( struct x86_function *p, struct x86_reg dst, int imm )
{
   DUMP_RI( dst, imm );
   x86_group1_imm(p, 7, dst, imm);
}


void x86_push( struct x86_function *p,
	       struct x86_reg reg )
{
   DUMP_R( reg );
   if (reg.mod == mod_REG)
      emit_1ub(p, 0x50 + reg.idx);
   else 
   {
      emit_1ub(p, 0xff);
      emit_modrm_noreg(p, 6, reg);
   }


   p->stack_offset += sizeof(void*);
}

void x86_push_imm32( struct x86_function *p,
                     int imm32 )
{
   DUMP_I( imm32 );
   emit_1ub(p, 0x68);
   emit_1i(p,  imm32);

   p->stack_offset += sizeof(void*);
}


void x86_pop( struct x86_function *p,
	      struct x86_reg reg )
{
   DUMP_R( reg );
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x58 + reg.idx);
   p->stack_offset -= sizeof(void*);
}

void x86_inc( struct x86_function *p,
	      struct x86_reg reg )
{
   DUMP_R( reg );
   if(x86_target(p) == X86_32 && reg.mod == mod_REG)
   {
      emit_1ub(p, 0x40 + reg.idx);
      return;
   }
   emit_1ub(p, 0xff);
   emit_modrm_noreg(p, 0, reg);
}

void x86_dec( struct x86_function *p,
	      struct x86_reg reg )
{
   DUMP_R( reg );
   if(x86_target(p) == X86_32 && reg.mod == mod_REG)
   {
      emit_1ub(p, 0x48 + reg.idx);
      return;
   }
   emit_1ub(p, 0xff);
   emit_modrm_noreg(p, 1, reg);
}

void x86_ret( struct x86_function *p )
{
   DUMP();
   assert(p->stack_offset == 0);
   emit_1ub(p, 0xc3);
}

void x86_retw( struct x86_function *p, unsigned short imm )
{
   DUMP();
   emit_3ub(p, 0xc2, imm & 0xff, (imm >> 8) & 0xff);
}

void x86_sahf( struct x86_function *p )
{
   DUMP();
   emit_1ub(p, 0x9e);
}

void x86_mov( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   DUMP_RR( dst, src );
   /* special hack for reading arguments until we support x86-64 registers everywhere */
   if(src.mod == mod_REG && dst.mod == mod_REG && (src.idx >= 8 || dst.idx >= 8))
   {
      uint8_t rex = 0x40;
      if(dst.idx >= 8)
      {
         rex |= 4;
         dst.idx -= 8;
      }
      if(src.idx >= 8)
      {
         rex |= 1;
         src.idx -= 8;
      }
      emit_1ub(p, rex);
   }
   emit_op_modrm( p, 0x8b, 0x89, dst, src );
}

void x86_mov16( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_1ub(p, 0x66);
   emit_op_modrm( p, 0x8b, 0x89, dst, src );
}

void x86_mov8( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_op_modrm( p, 0x8a, 0x88, dst, src );
}

void x64_mov64( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   uint8_t rex = 0x48;
   DUMP_RR( dst, src );
   assert(x86_target(p) != X86_32);

   /* special hack for reading arguments until we support x86-64 registers everywhere */
   if(src.mod == mod_REG && dst.mod == mod_REG && (src.idx >= 8 || dst.idx >= 8))
   {
      if(dst.idx >= 8)
      {
         rex |= 4;
         dst.idx -= 8;
      }
      if(src.idx >= 8)
      {
         rex |= 1;
         src.idx -= 8;
      }
   }
   emit_1ub(p, rex);
   emit_op_modrm( p, 0x8b, 0x89, dst, src );
}

void x86_movzx8(struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, 0x0f, 0xb6);
   emit_modrm(p, dst, src);
}

void x86_movzx16(struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, 0x0f, 0xb7);
   emit_modrm(p, dst, src);
}

void x86_cmovcc( struct x86_function *p,
                 struct x86_reg dst,
                 struct x86_reg src,
                 enum x86_cc cc)
{
   DUMP_RRI( dst, src, cc );
   emit_2ub( p, 0x0f, 0x40 + cc );
   emit_modrm( p, dst, src );
}

void x86_xor( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_op_modrm( p, 0x33, 0x31, dst, src );
}

void x86_cmp( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_op_modrm( p, 0x3b, 0x39, dst, src );
}

void x86_lea( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_1ub(p, 0x8d);
   emit_modrm( p, dst, src );
}

void x86_test( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_1ub(p, 0x85);
   emit_modrm( p, dst, src );
}

void x86_add( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_op_modrm(p, 0x03, 0x01, dst, src );
}

/* Calculate EAX * src, results in EDX:EAX.
 */
void x86_mul( struct x86_function *p,
	       struct x86_reg src )
{
   DUMP_R(  src );
   emit_1ub(p, 0xf7);
   emit_modrm_noreg(p, 4, src );
}


void x86_imul( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0xAF);
   emit_modrm(p, dst, src);
}


void x86_sub( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_op_modrm(p, 0x2b, 0x29, dst, src );
}

void x86_or( struct x86_function *p,
             struct x86_reg dst,
             struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_op_modrm( p, 0x0b, 0x09, dst, src );
}

void x86_and( struct x86_function *p,
              struct x86_reg dst,
              struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_op_modrm( p, 0x23, 0x21, dst, src );
}

void x86_div( struct x86_function *p,
              struct x86_reg src )
{
   assert(src.file == file_REG32 && src.mod == mod_REG);
   emit_op_modrm(p, 0xf7, 0, x86_make_reg(file_REG32, 6), src);
}

void x86_bswap( struct x86_function *p, struct x86_reg reg )
{
   DUMP_R(reg);
   assert(reg.file == file_REG32);
   assert(reg.mod == mod_REG);
   emit_2ub(p, 0x0f, 0xc8 + reg.idx);
}

void x86_shr_imm( struct x86_function *p, struct x86_reg reg, unsigned imm )
{
   DUMP_RI(reg, imm);
   if(imm == 1)
   {
      emit_1ub(p, 0xd1);
      emit_modrm_noreg(p, 5, reg);
   }
   else
   {
      emit_1ub(p, 0xc1);
      emit_modrm_noreg(p, 5, reg);
      emit_1ub(p, imm);
   }
}

void x86_sar_imm( struct x86_function *p, struct x86_reg reg, unsigned imm )
{
   DUMP_RI(reg, imm);
   if(imm == 1)
   {
      emit_1ub(p, 0xd1);
      emit_modrm_noreg(p, 7, reg);
   }
   else
   {
      emit_1ub(p, 0xc1);
      emit_modrm_noreg(p, 7, reg);
      emit_1ub(p, imm);
   }
}

void x86_shl_imm( struct x86_function *p, struct x86_reg reg, unsigned imm  )
{
   DUMP_RI(reg, imm);
   if(imm == 1)
   {
      emit_1ub(p, 0xd1);
      emit_modrm_noreg(p, 4, reg);
   }
   else
   {
      emit_1ub(p, 0xc1);
      emit_modrm_noreg(p, 4, reg);
      emit_1ub(p, imm);
   }
}


/***********************************************************************
 * SSE instructions
 */

void sse_prefetchnta( struct x86_function *p, struct x86_reg ptr)
{
   DUMP_R( ptr );
   assert(ptr.mod != mod_REG);
   emit_2ub(p, 0x0f, 0x18);
   emit_modrm_noreg(p, 0, ptr);
}

void sse_prefetch0( struct x86_function *p, struct x86_reg ptr)
{
   DUMP_R( ptr );
   assert(ptr.mod != mod_REG);
   emit_2ub(p, 0x0f, 0x18);
   emit_modrm_noreg(p, 1, ptr);
}

void sse_prefetch1( struct x86_function *p, struct x86_reg ptr)
{
   DUMP_R( ptr );
   assert(ptr.mod != mod_REG);
   emit_2ub(p, 0x0f, 0x18);
   emit_modrm_noreg(p, 2, ptr);
}

void sse_movntps( struct x86_function *p, 
                  struct x86_reg dst,
                  struct x86_reg src)
{
   DUMP_RR( dst, src );

   assert(dst.mod != mod_REG);
   assert(src.mod == mod_REG);
   emit_2ub(p, 0x0f, 0x2b);
   emit_modrm(p, src, dst);
}




void sse_movss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, 0xF3, X86_TWOB);
   emit_op_modrm( p, 0x10, 0x11, dst, src );
}

void sse_movaps( struct x86_function *p,
		 struct x86_reg dst,
		 struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x28, 0x29, dst, src );
}

void sse_movups( struct x86_function *p,
		 struct x86_reg dst,
		 struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x10, 0x11, dst, src );
}

void sse_movhps( struct x86_function *p,
		 struct x86_reg dst,
		 struct x86_reg src )
{
   DUMP_RR( dst, src );
   assert(dst.mod != mod_REG || src.mod != mod_REG);
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x16, 0x17, dst, src ); /* cf movlhps */
}

void sse_movlps( struct x86_function *p,
		 struct x86_reg dst,
		 struct x86_reg src )
{
   DUMP_RR( dst, src );
   assert(dst.mod != mod_REG || src.mod != mod_REG);
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x12, 0x13, dst, src ); /* cf movhlps */
}

void sse_maxps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x5F);
   emit_modrm( p, dst, src );
}

void sse_maxss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0xF3, X86_TWOB, 0x5F);
   emit_modrm( p, dst, src );
}

void sse_divss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0xF3, X86_TWOB, 0x5E);
   emit_modrm( p, dst, src );
}

void sse_minps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x5D);
   emit_modrm( p, dst, src );
}

void sse_subps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x5C);
   emit_modrm( p, dst, src );
}

void sse_mulps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x59);
   emit_modrm( p, dst, src );
}

void sse_mulss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0xF3, X86_TWOB, 0x59);
   emit_modrm( p, dst, src );
}

void sse_addps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x58);
   emit_modrm( p, dst, src );
}

void sse_addss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0xF3, X86_TWOB, 0x58);
   emit_modrm( p, dst, src );
}

void sse_andnps( struct x86_function *p,
                 struct x86_reg dst,
                 struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x55);
   emit_modrm( p, dst, src );
}

void sse_andps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x54);
   emit_modrm( p, dst, src );
}

void sse_rsqrtps( struct x86_function *p,
                  struct x86_reg dst,
                  struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x52);
   emit_modrm( p, dst, src );
}

void sse_rsqrtss( struct x86_function *p,
		  struct x86_reg dst,
		  struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0xF3, X86_TWOB, 0x52);
   emit_modrm( p, dst, src );

}

void sse_movhlps( struct x86_function *p,
		  struct x86_reg dst,
		  struct x86_reg src )
{
   DUMP_RR( dst, src );
   assert(dst.mod == mod_REG && src.mod == mod_REG);
   emit_2ub(p, X86_TWOB, 0x12);
   emit_modrm( p, dst, src );
}

void sse_movlhps( struct x86_function *p,
		  struct x86_reg dst,
		  struct x86_reg src )
{
   DUMP_RR( dst, src );
   assert(dst.mod == mod_REG && src.mod == mod_REG);
   emit_2ub(p, X86_TWOB, 0x16);
   emit_modrm( p, dst, src );
}

void sse_orps( struct x86_function *p,
               struct x86_reg dst,
               struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x56);
   emit_modrm( p, dst, src );
}

void sse_xorps( struct x86_function *p,
                struct x86_reg dst,
                struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x57);
   emit_modrm( p, dst, src );
}

void sse_cvtps2pi( struct x86_function *p,
		   struct x86_reg dst,
		   struct x86_reg src )
{
   DUMP_RR( dst, src );
   assert(dst.file == file_MMX && 
	  (src.file == file_XMM || src.mod != mod_REG));

   p->need_emms = 1;

   emit_2ub(p, X86_TWOB, 0x2d);
   emit_modrm( p, dst, src );
}

void sse2_cvtdq2ps( struct x86_function *p,
		   struct x86_reg dst,
		   struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x5b);
   emit_modrm( p, dst, src );
}


/* Shufps can also be used to implement a reduced swizzle when dest ==
 * arg0.
 */
void sse_shufps( struct x86_function *p,
		 struct x86_reg dst,
		 struct x86_reg src,
		 unsigned char shuf) 
{
   DUMP_RRI( dst, src, shuf );
   emit_2ub(p, X86_TWOB, 0xC6);
   emit_modrm(p, dst, src);
   emit_1ub(p, shuf); 
}

void sse_unpckhps( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub( p, X86_TWOB, 0x15 );
   emit_modrm( p, dst, src );
}

void sse_unpcklps( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub( p, X86_TWOB, 0x14 );
   emit_modrm( p, dst, src );
}

void sse_cmpps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src,
		enum sse_cc cc) 
{
   DUMP_RRI( dst, src, cc );
   emit_2ub(p, X86_TWOB, 0xC2);
   emit_modrm(p, dst, src);
   emit_1ub(p, cc); 
}

void sse_pmovmskb( struct x86_function *p,
                   struct x86_reg dst,
                   struct x86_reg src)
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0x66, X86_TWOB, 0xD7);
   emit_modrm(p, dst, src);
}

void sse_movmskps( struct x86_function *p,
                   struct x86_reg dst,
                   struct x86_reg src)
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x50);
   emit_modrm(p, dst, src);
}

/***********************************************************************
 * SSE2 instructions
 */

void sse2_movd( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR(dst, src);
   emit_2ub(p, 0x66, 0x0f);
   if(dst.mod == mod_REG && dst.file == file_REG32)
   {
      emit_1ub(p, 0x7e);
      emit_modrm(p, src, dst);
   }
   else
   {
      emit_op_modrm(p, 0x6e, 0x7e, dst, src);
   }
}

void sse2_movq( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR(dst, src);
   switch (dst.mod) {
   case mod_REG:
      emit_3ub(p, 0xf3, 0x0f, 0x7e);
      emit_modrm(p, dst, src);
      break;
   case mod_INDIRECT:
   case mod_DISP32:
   case mod_DISP8:
      assert(src.mod == mod_REG);
      emit_3ub(p, 0x66, 0x0f, 0xd6);
      emit_modrm(p, src, dst);
      break;
   default:
      assert(0);
      break;
   }
}

void sse2_movdqu( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR(dst, src);
   emit_2ub(p, 0xf3, 0x0f);
   emit_op_modrm(p, 0x6f, 0x7f, dst, src);
}

void sse2_movdqa( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR(dst, src);
   emit_2ub(p, 0x66, 0x0f);
   emit_op_modrm(p, 0x6f, 0x7f, dst, src);
}

void sse2_movsd( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR(dst, src);
   emit_2ub(p, 0xf2, 0x0f);
   emit_op_modrm(p, 0x10, 0x11, dst, src);
}

void sse2_movupd( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR(dst, src);
   emit_2ub(p, 0x66, 0x0f);
   emit_op_modrm(p, 0x10, 0x11, dst, src);
}

void sse2_movapd( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR(dst, src);
   emit_2ub(p, 0x66, 0x0f);
   emit_op_modrm(p, 0x28, 0x29, dst, src);
}

/**
 * Perform a reduced swizzle:
 */
void sse2_pshufd( struct x86_function *p,
		  struct x86_reg dst,
		  struct x86_reg src,
		  unsigned char shuf) 
{
   DUMP_RRI( dst, src, shuf );
   emit_3ub(p, 0x66, X86_TWOB, 0x70);
   emit_modrm(p, dst, src);
   emit_1ub(p, shuf); 
}

void sse2_pshuflw( struct x86_function *p,
                  struct x86_reg dst,
                  struct x86_reg src,
                  unsigned char shuf)
{
   DUMP_RRI( dst, src, shuf );
   emit_3ub(p, 0xf2, X86_TWOB, 0x70);
   emit_modrm(p, dst, src);
   emit_1ub(p, shuf);
}

void sse2_pshufhw( struct x86_function *p,
                  struct x86_reg dst,
                  struct x86_reg src,
                  unsigned char shuf)
{
   DUMP_RRI( dst, src, shuf );
   emit_3ub(p, 0xf3, X86_TWOB, 0x70);
   emit_modrm(p, dst, src);
   emit_1ub(p, shuf);
}

void sse2_cvttps2dq( struct x86_function *p,
                     struct x86_reg dst,
                     struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub( p, 0xF3, X86_TWOB, 0x5B );
   emit_modrm( p, dst, src );
}

void sse2_cvtps2dq( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0x66, X86_TWOB, 0x5B);
   emit_modrm( p, dst, src );
}

void sse2_cvtsd2ss( struct x86_function *p,
                    struct x86_reg dst,
                    struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0xf2, 0x0f, 0x5a);
   emit_modrm( p, dst, src );
}

void sse2_cvtpd2ps( struct x86_function *p,
                    struct x86_reg dst,
                    struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0x66, 0x0f, 0x5a);
   emit_modrm( p, dst, src );
}

void sse2_packssdw( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0x66, X86_TWOB, 0x6B);
   emit_modrm( p, dst, src );
}

void sse2_packsswb( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0x66, X86_TWOB, 0x63);
   emit_modrm( p, dst, src );
}

void sse2_packuswb( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0x66, X86_TWOB, 0x67);
   emit_modrm( p, dst, src );
}

void sse2_punpcklbw( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0x66, X86_TWOB, 0x60);
   emit_modrm( p, dst, src );
}

void sse2_punpcklwd( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0x66, 0x0f, 0x61);
   emit_modrm( p, dst, src );
}

void sse2_punpckldq( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0x66, 0x0f, 0x62);
   emit_modrm( p, dst, src );
}

void sse2_punpcklqdq( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0x66, 0x0f, 0x6c);
   emit_modrm( p, dst, src );
}

void sse2_psllw_imm( struct x86_function *p, struct x86_reg dst, unsigned imm )
{
   DUMP_RI(dst, imm);
   emit_3ub(p, 0x66, 0x0f, 0x71);
   emit_modrm_noreg(p, 6, dst);
   emit_1ub(p, imm);
}

void sse2_pslld_imm( struct x86_function *p, struct x86_reg dst, unsigned imm )
{
   DUMP_RI(dst, imm);
   emit_3ub(p, 0x66, 0x0f, 0x72);
   emit_modrm_noreg(p, 6, dst);
   emit_1ub(p, imm);
}

void sse2_psllq_imm( struct x86_function *p, struct x86_reg dst, unsigned imm )
{
   DUMP_RI(dst, imm);
   emit_3ub(p, 0x66, 0x0f, 0x73);
   emit_modrm_noreg(p, 6, dst);
   emit_1ub(p, imm);
}

void sse2_psrlw_imm( struct x86_function *p, struct x86_reg dst, unsigned imm )
{
   DUMP_RI(dst, imm);
   emit_3ub(p, 0x66, 0x0f, 0x71);
   emit_modrm_noreg(p, 2, dst);
   emit_1ub(p, imm);
}

void sse2_psrld_imm( struct x86_function *p, struct x86_reg dst, unsigned imm )
{
   DUMP_RI(dst, imm);
   emit_3ub(p, 0x66, 0x0f, 0x72);
   emit_modrm_noreg(p, 2, dst);
   emit_1ub(p, imm);
}

void sse2_psrlq_imm( struct x86_function *p, struct x86_reg dst, unsigned imm )
{
   DUMP_RI(dst, imm);
   emit_3ub(p, 0x66, 0x0f, 0x73);
   emit_modrm_noreg(p, 2, dst);
   emit_1ub(p, imm);
}

void sse2_psraw_imm( struct x86_function *p, struct x86_reg dst, unsigned imm )
{
   DUMP_RI(dst, imm);
   emit_3ub(p, 0x66, 0x0f, 0x71);
   emit_modrm_noreg(p, 4, dst);
   emit_1ub(p, imm);
}

void sse2_psrad_imm( struct x86_function *p, struct x86_reg dst, unsigned imm )
{
   DUMP_RI(dst, imm);
   emit_3ub(p, 0x66, 0x0f, 0x72);
   emit_modrm_noreg(p, 4, dst);
   emit_1ub(p, imm);
}

void sse2_por( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR(dst, src);
   emit_3ub(p, 0x66, 0x0f, 0xeb);
   emit_modrm(p, dst, src);
}

void sse2_rcpps( struct x86_function *p,
                 struct x86_reg dst,
                 struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_2ub(p, X86_TWOB, 0x53);
   emit_modrm( p, dst, src );
}

void sse2_rcpss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   DUMP_RR( dst, src );
   emit_3ub(p, 0xF3, X86_TWOB, 0x53);
   emit_modrm( p, dst, src );
}

/***********************************************************************
 * x87 instructions
 */
static void note_x87_pop( struct x86_function *p )
{
   p->x87_stack--;
   assert(p->x87_stack >= 0);
}

static void note_x87_push( struct x86_function *p )
{
   p->x87_stack++;
   assert(p->x87_stack <= 7);
}

void x87_assert_stack_empty( struct x86_function *p )
{
   assert (p->x87_stack == 0);
}


void x87_fist( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   emit_1ub(p, 0xdb);
   emit_modrm_noreg(p, 2, dst);
}

void x87_fistp( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   emit_1ub(p, 0xdb);
   emit_modrm_noreg(p, 3, dst);
   note_x87_pop(p);
}

void x87_fild( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   emit_1ub(p, 0xdf);
   emit_modrm_noreg(p, 0, arg);
   note_x87_push(p);
}

void x87_fldz( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xee);
   note_x87_push(p);
}


void x87_fldcw( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   assert(arg.file == file_REG32);
   assert(arg.mod != mod_REG);
   emit_1ub(p, 0xd9);
   emit_modrm_noreg(p, 5, arg);
}

void x87_fld1( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xe8);
   note_x87_push(p);
}

void x87_fldl2e( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xea);
   note_x87_push(p);
}

void x87_fldln2( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xed);
   note_x87_push(p);
}

void x87_fwait( struct x86_function *p )
{
   DUMP();
   emit_1ub(p, 0x9b);
}

void x87_fnclex( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xdb, 0xe2);
}

void x87_fclex( struct x86_function *p )
{
   x87_fwait(p);
   x87_fnclex(p);
}

void x87_fcmovb( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   assert(arg.file == file_x87);
   emit_2ub(p, 0xda, 0xc0+arg.idx);
}

void x87_fcmove( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   assert(arg.file == file_x87);
   emit_2ub(p, 0xda, 0xc8+arg.idx);
}

void x87_fcmovbe( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   assert(arg.file == file_x87);
   emit_2ub(p, 0xda, 0xd0+arg.idx);
}

void x87_fcmovnb( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   assert(arg.file == file_x87);
   emit_2ub(p, 0xdb, 0xc0+arg.idx);
}

void x87_fcmovne( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   assert(arg.file == file_x87);
   emit_2ub(p, 0xdb, 0xc8+arg.idx);
}

void x87_fcmovnbe( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   assert(arg.file == file_x87);
   emit_2ub(p, 0xdb, 0xd0+arg.idx);
}



static void x87_arith_op( struct x86_function *p, struct x86_reg dst, struct x86_reg arg,
			  unsigned char dst0ub0,
			  unsigned char dst0ub1,
			  unsigned char arg0ub0,
			  unsigned char arg0ub1,
			  unsigned char argmem_noreg)
{
   assert(dst.file == file_x87);

   if (arg.file == file_x87) {
      if (dst.idx == 0) 
	 emit_2ub(p, dst0ub0, dst0ub1+arg.idx);
      else if (arg.idx == 0) 
	 emit_2ub(p, arg0ub0, arg0ub1+arg.idx);
      else
	 assert(0);
   }
   else if (dst.idx == 0) {
      assert(arg.file == file_REG32);
      emit_1ub(p, 0xd8);
      emit_modrm_noreg(p, argmem_noreg, arg);
   }
   else
      assert(0);
}

void x87_fmul( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   x87_arith_op(p, dst, src, 
		0xd8, 0xc8,
		0xdc, 0xc8,
		4);
}

void x87_fsub( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   x87_arith_op(p, dst, src, 
		0xd8, 0xe0,
		0xdc, 0xe8,
		4);
}

void x87_fsubr( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   x87_arith_op(p, dst, src, 
		0xd8, 0xe8,
		0xdc, 0xe0,
		5);
}

void x87_fadd( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   x87_arith_op(p, dst, src, 
		0xd8, 0xc0,
		0xdc, 0xc0,
		0);
}

void x87_fdiv( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   x87_arith_op(p, dst, src, 
		0xd8, 0xf0,
		0xdc, 0xf8,
		6);
}

void x87_fdivr( struct x86_function *p, struct x86_reg dst, struct x86_reg src )
{
   DUMP_RR( dst, src );
   x87_arith_op(p, dst, src, 
		0xd8, 0xf8,
		0xdc, 0xf0,
		7);
}

void x87_fmulp( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   assert(dst.file == file_x87);
   assert(dst.idx >= 1);
   emit_2ub(p, 0xde, 0xc8+dst.idx);
   note_x87_pop(p);
}

void x87_fsubp( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   assert(dst.file == file_x87);
   assert(dst.idx >= 1);
   emit_2ub(p, 0xde, 0xe8+dst.idx);
   note_x87_pop(p);
}

void x87_fsubrp( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   assert(dst.file == file_x87);
   assert(dst.idx >= 1);
   emit_2ub(p, 0xde, 0xe0+dst.idx);
   note_x87_pop(p);
}

void x87_faddp( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   assert(dst.file == file_x87);
   assert(dst.idx >= 1);
   emit_2ub(p, 0xde, 0xc0+dst.idx);
   note_x87_pop(p);
}

void x87_fdivp( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   assert(dst.file == file_x87);
   assert(dst.idx >= 1);
   emit_2ub(p, 0xde, 0xf8+dst.idx);
   note_x87_pop(p);
}

void x87_fdivrp( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   assert(dst.file == file_x87);
   assert(dst.idx >= 1);
   emit_2ub(p, 0xde, 0xf0+dst.idx);
   note_x87_pop(p);
}

void x87_ftst( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xe4);
}

void x87_fucom( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   assert(arg.file == file_x87);
   emit_2ub(p, 0xdd, 0xe0+arg.idx);
}

void x87_fucomp( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   assert(arg.file == file_x87);
   emit_2ub(p, 0xdd, 0xe8+arg.idx);
   note_x87_pop(p);
}

void x87_fucompp( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xda, 0xe9);
   note_x87_pop(p);             /* pop twice */
   note_x87_pop(p);             /* pop twice */
}

void x87_fxch( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   assert(arg.file == file_x87);
   emit_2ub(p, 0xd9, 0xc8+arg.idx);
}

void x87_fabs( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xe1);
}

void x87_fchs( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xe0);
}

void x87_fcos( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xff);
}


void x87_fprndint( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xfc);
}

void x87_fscale( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xfd);
}

void x87_fsin( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xfe);
}

void x87_fsincos( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xfb);
}

void x87_fsqrt( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xfa);
}

void x87_fxtract( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xf4);
}

/* st0 = (2^st0)-1
 *
 * Restrictions: -1.0 <= st0 <= 1.0
 */
void x87_f2xm1( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xf0);
}

/* st1 = st1 * log2(st0);
 * pop_stack;
 */
void x87_fyl2x( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xf1);
   note_x87_pop(p);
}

/* st1 = st1 * log2(st0 + 1.0);
 * pop_stack;
 *
 * A fast operation, with restrictions: -.29 < st0 < .29 
 */
void x87_fyl2xp1( struct x86_function *p )
{
   DUMP();
   emit_2ub(p, 0xd9, 0xf9);
   note_x87_pop(p);
}


void x87_fld( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   if (arg.file == file_x87) 
      emit_2ub(p, 0xd9, 0xc0 + arg.idx);
   else {
      emit_1ub(p, 0xd9);
      emit_modrm_noreg(p, 0, arg);
   }
   note_x87_push(p);
}

void x87_fst( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   if (dst.file == file_x87) 
      emit_2ub(p, 0xdd, 0xd0 + dst.idx);
   else {
      emit_1ub(p, 0xd9);
      emit_modrm_noreg(p, 2, dst);
   }
}

void x87_fstp( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   if (dst.file == file_x87) 
      emit_2ub(p, 0xdd, 0xd8 + dst.idx);
   else {
      emit_1ub(p, 0xd9);
      emit_modrm_noreg(p, 3, dst);
   }
   note_x87_pop(p);
}

void x87_fpop( struct x86_function *p )
{
   x87_fstp( p, x86_make_reg( file_x87, 0 ));
}


void x87_fcom( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   if (dst.file == file_x87) 
      emit_2ub(p, 0xd8, 0xd0 + dst.idx);
   else {
      emit_1ub(p, 0xd8);
      emit_modrm_noreg(p, 2, dst);
   }
}


void x87_fcomp( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   if (dst.file == file_x87) 
      emit_2ub(p, 0xd8, 0xd8 + dst.idx);
   else {
      emit_1ub(p, 0xd8);
      emit_modrm_noreg(p, 3, dst);
   }
   note_x87_pop(p);
}

void x87_fcomi( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   emit_2ub(p, 0xdb, 0xf0+arg.idx);
}

void x87_fcomip( struct x86_function *p, struct x86_reg arg )
{
   DUMP_R( arg );
   emit_2ub(p, 0xdb, 0xf0+arg.idx);
   note_x87_pop(p);
}


void x87_fnstsw( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   assert(dst.file == file_REG32);

   if (dst.idx == reg_AX &&
       dst.mod == mod_REG) 
      emit_2ub(p, 0xdf, 0xe0);
   else {
      emit_1ub(p, 0xdd);
      emit_modrm_noreg(p, 7, dst);
   }
}


void x87_fnstcw( struct x86_function *p, struct x86_reg dst )
{
   DUMP_R( dst );
   assert(dst.file == file_REG32);

   emit_1ub(p, 0x9b);           /* WAIT -- needed? */
   emit_1ub(p, 0xd9);
   emit_modrm_noreg(p, 7, dst);
}




/***********************************************************************
 * MMX instructions
 */

void mmx_emms( struct x86_function *p )
{
   DUMP();
   assert(p->need_emms);
   emit_2ub(p, 0x0f, 0x77);
   p->need_emms = 0;
}

void mmx_packssdw( struct x86_function *p,
		   struct x86_reg dst,
		   struct x86_reg src )
{
   DUMP_RR( dst, src );
   assert(dst.file == file_MMX && 
	  (src.file == file_MMX || src.mod != mod_REG));

   p->need_emms = 1;

   emit_2ub(p, X86_TWOB, 0x6b);
   emit_modrm( p, dst, src );
}

void mmx_packuswb( struct x86_function *p,
		   struct x86_reg dst,
		   struct x86_reg src )
{
   DUMP_RR( dst, src );
   assert(dst.file == file_MMX && 
	  (src.file == file_MMX || src.mod != mod_REG));

   p->need_emms = 1;

   emit_2ub(p, X86_TWOB, 0x67);
   emit_modrm( p, dst, src );
}

void mmx_movd( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   DUMP_RR( dst, src );
   p->need_emms = 1;
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x6e, 0x7e, dst, src );
}

void mmx_movq( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   DUMP_RR( dst, src );
   p->need_emms = 1;
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x6f, 0x7f, dst, src );
}


/***********************************************************************
 * Helper functions
 */


void x86_cdecl_caller_push_regs( struct x86_function *p )
{
   x86_push(p, x86_make_reg(file_REG32, reg_AX));
   x86_push(p, x86_make_reg(file_REG32, reg_CX));
   x86_push(p, x86_make_reg(file_REG32, reg_DX));
}

void x86_cdecl_caller_pop_regs( struct x86_function *p )
{
   x86_pop(p, x86_make_reg(file_REG32, reg_DX));
   x86_pop(p, x86_make_reg(file_REG32, reg_CX));
   x86_pop(p, x86_make_reg(file_REG32, reg_AX));
}


struct x86_reg x86_fn_arg( struct x86_function *p,
                           unsigned arg )
{
   switch(x86_target(p))
   {
   case X86_64_WIN64_ABI:
      /* Microsoft uses a different calling convention than the rest of the world */
      switch(arg)
      {
      case 1:
         return x86_make_reg(file_REG32, reg_CX);
      case 2:
         return x86_make_reg(file_REG32, reg_DX);
      case 3:
         return x86_make_reg(file_REG32, reg_R8);
      case 4:
         return x86_make_reg(file_REG32, reg_R9);
      default:
	 /* Win64 allocates stack slots as if it pushed the first 4 arguments too */
         return x86_make_disp(x86_make_reg(file_REG32, reg_SP),
               p->stack_offset + arg * 8);
      }
   case X86_64_STD_ABI:
      switch(arg)
      {
      case 1:
         return x86_make_reg(file_REG32, reg_DI);
      case 2:
         return x86_make_reg(file_REG32, reg_SI);
      case 3:
         return x86_make_reg(file_REG32, reg_DX);
      case 4:
         return x86_make_reg(file_REG32, reg_CX);
      case 5:
         return x86_make_reg(file_REG32, reg_R8);
      case 6:
         return x86_make_reg(file_REG32, reg_R9);
      default:
         return x86_make_disp(x86_make_reg(file_REG32, reg_SP),
               p->stack_offset + (arg - 6) * 8);     /* ??? */
      }
   case X86_32:
      return x86_make_disp(x86_make_reg(file_REG32, reg_SP),
			p->stack_offset + arg * 4);	/* ??? */
   default:
      assert(0 && "Unexpected x86 target ABI in x86_fn_arg");
      return x86_make_reg(file_REG32, reg_CX); /* not used / silence warning */
   }
}

static void x86_init_func_common( struct x86_function *p )
{
   util_cpu_detect();
   p->caps = 0;
   if(util_cpu_caps.has_mmx)
      p->caps |= X86_MMX;
   if(util_cpu_caps.has_mmx2)
      p->caps |= X86_MMX2;
   if(util_cpu_caps.has_sse)
      p->caps |= X86_SSE;
   if(util_cpu_caps.has_sse2)
      p->caps |= X86_SSE2;
   if(util_cpu_caps.has_sse3)
      p->caps |= X86_SSE3;
   if(util_cpu_caps.has_sse4_1)
      p->caps |= X86_SSE4_1;
   p->csr = p->store;
   DUMP_START();
}

void x86_init_func( struct x86_function *p )
{
   p->size = 0;
   p->store = NULL;
   x86_init_func_common(p);
}

void x86_init_func_size( struct x86_function *p, unsigned code_size )
{
   p->size = code_size;
   p->store = rtasm_exec_malloc(code_size);
   if (p->store == NULL) {
      p->store = p->error_overflow;
   }
   x86_init_func_common(p);
}

void x86_release_func( struct x86_function *p )
{
   if (p->store && p->store != p->error_overflow)
      rtasm_exec_free(p->store);

   p->store = NULL;
   p->csr = NULL;
   p->size = 0;
}


static INLINE x86_func
voidptr_to_x86_func(void *v)
{
   union {
      void *v;
      x86_func f;
   } u;
   assert(sizeof(u.v) == sizeof(u.f));
   u.v = v;
   return u.f;
}


x86_func x86_get_func( struct x86_function *p )
{
   DUMP_END();
   if (DISASSEM && p->store)
      debug_printf("disassemble %p %p\n", p->store, p->csr);

   if (p->store == p->error_overflow)
      return voidptr_to_x86_func(NULL);
   else
      return voidptr_to_x86_func(p->store);
}

#else

void x86sse_dummy( void );

void x86sse_dummy( void )
{
}

#endif
