#if defined(__i386__) || defined(__386__)

#include "imports.h"
#include "x86sse.h"

#define DISASSEM 0
#define X86_TWOB 0x0f

static unsigned char *cptr( void (*label)() )
{
   return (unsigned char *)(unsigned long)label;
}


static void do_realloc( struct x86_function *p )
{
   if (p->size == 0) {
      p->size = 1024;
      p->store = _mesa_exec_malloc(p->size);
      p->csr = p->store;
   }
   else {
      unsigned used = p->csr - p->store;
      unsigned char *tmp = p->store;
      p->size *= 2;
      p->store = _mesa_exec_malloc(p->size);
      memcpy(p->store, tmp, used);
      p->csr = p->store + used;
      _mesa_exec_free(tmp);
   }
}

/* Emit bytes to the instruction stream:
 */
static unsigned char *reserve( struct x86_function *p, int bytes )
{
   if (p->csr + bytes - p->store > p->size)
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
 */
static void emit_modrm( struct x86_function *p, 
			struct x86_reg reg, 
			struct x86_reg regmem )
{
   unsigned char val = 0;
   
   assert(reg.mod == mod_REG);
   
   val |= regmem.mod << 6;     	/* mod field */
   val |= reg.idx << 3;		/* reg field */
   val |= regmem.idx;		/* r/m field */
   
   emit_1ub(p, val);

   /* Oh-oh we've stumbled into the SIB thing.
    */
   if (regmem.file == file_REG32 &&
       regmem.idx == reg_SP) {
      emit_1ub(p, 0x24);		/* simplistic! */
   }

   switch (regmem.mod) {
   case mod_REG:
   case mod_INDIRECT:
      break;
   case mod_DISP8:
      emit_1b(p, regmem.disp);
      break;
   case mod_DISP32:
      emit_1i(p, regmem.disp);
      break;
   default:
      assert(0);
      break;
   }
}


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

   if (reg.disp == 0)
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

unsigned char *x86_get_label( struct x86_function *p )
{
   return p->csr;
}



/***********************************************************************
 * x86 instructions
 */


void x86_jcc( struct x86_function *p,
	      enum x86_cc cc,
	      unsigned char *label )
{
   int offset = label - (x86_get_label(p) + 2);
   
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
unsigned char *x86_jcc_forward( struct x86_function *p,
			  enum x86_cc cc )
{
   emit_2ub(p, 0x0f, 0x80 + cc);
   emit_1i(p, 0);
   return x86_get_label(p);
}

unsigned char *x86_jmp_forward( struct x86_function *p)
{
   emit_1ub(p, 0xe9);
   emit_1i(p, 0);
   return x86_get_label(p);
}

unsigned char *x86_call_forward( struct x86_function *p)
{
   emit_1ub(p, 0xe8);
   emit_1i(p, 0);
   return x86_get_label(p);
}

/* Fixup offset from forward jump:
 */
void x86_fixup_fwd_jump( struct x86_function *p,
			 unsigned char *fixup )
{
   *(int *)(fixup - 4) = x86_get_label(p) - fixup;
}

void x86_jmp( struct x86_function *p, unsigned char *label)
{
   emit_1ub(p, 0xe9);
   emit_1i(p, label - x86_get_label(p) - 4);
}

#if 0
/* This doesn't work once we start reallocating & copying the
 * generated code on buffer fills, because the call is relative to the
 * current pc.
 */
void x86_call( struct x86_function *p, void (*label)())
{
   emit_1ub(p, 0xe8);
   emit_1i(p, cptr(label) - x86_get_label(p) - 4);
}
#else
void x86_call( struct x86_function *p, struct x86_reg reg)
{
   emit_1ub(p, 0xff);
   emit_modrm(p, reg, reg);
}
#endif


/* michal:
 * Temporary. As I need immediate operands, and dont want to mess with the codegen,
 * I load the immediate into general purpose register and use it.
 */
void x86_mov_reg_imm( struct x86_function *p, struct x86_reg dst, int imm )
{
   assert(dst.mod == mod_REG);
   emit_1ub(p, 0xb8 + dst.idx);
   emit_1i(p, imm);
}

void x86_push( struct x86_function *p,
	       struct x86_reg reg )
{
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x50 + reg.idx);
   p->stack_offset += 4;
}

void x86_pop( struct x86_function *p,
	      struct x86_reg reg )
{
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x58 + reg.idx);
   p->stack_offset -= 4;
}

void x86_inc( struct x86_function *p,
	      struct x86_reg reg )
{
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x40 + reg.idx);
}

void x86_dec( struct x86_function *p,
	      struct x86_reg reg )
{
   assert(reg.mod == mod_REG);
   emit_1ub(p, 0x48 + reg.idx);
}

void x86_ret( struct x86_function *p )
{
   emit_1ub(p, 0xc3);
}

void x86_sahf( struct x86_function *p )
{
   emit_1ub(p, 0x9e);
}

void x86_mov( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   emit_op_modrm( p, 0x8b, 0x89, dst, src );
}

void x86_xor( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   emit_op_modrm( p, 0x33, 0x31, dst, src );
}

void x86_cmp( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   emit_op_modrm( p, 0x3b, 0x39, dst, src );
}

void x86_lea( struct x86_function *p,
	      struct x86_reg dst,
	      struct x86_reg src )
{
   emit_1ub(p, 0x8d);
   emit_modrm( p, dst, src );
}

void x86_test( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   emit_1ub(p, 0x85);
   emit_modrm( p, dst, src );
}

void x86_add( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   emit_op_modrm(p, 0x03, 0x01, dst, src );
}

void x86_mul( struct x86_function *p,
	       struct x86_reg src )
{
   assert (src.file == file_REG32 && src.mod == mod_REG);
   emit_op_modrm(p, 0xf7, 0, x86_make_reg (file_REG32, reg_SP), src );
}

void x86_sub( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   emit_op_modrm(p, 0x2b, 0x29, dst, src );
}

void x86_or( struct x86_function *p,
             struct x86_reg dst,
             struct x86_reg src )
{
   emit_op_modrm( p, 0x0b, 0x09, dst, src );
}

void x86_and( struct x86_function *p,
              struct x86_reg dst,
              struct x86_reg src )
{
   emit_op_modrm( p, 0x23, 0x21, dst, src );
}



/***********************************************************************
 * SSE instructions
 */


void sse_movss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, 0xF3, X86_TWOB);
   emit_op_modrm( p, 0x10, 0x11, dst, src );
}

void sse_movaps( struct x86_function *p,
		 struct x86_reg dst,
		 struct x86_reg src )
{
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x28, 0x29, dst, src );
}

void sse_movups( struct x86_function *p,
		 struct x86_reg dst,
		 struct x86_reg src )
{
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x10, 0x11, dst, src );
}

void sse_movhps( struct x86_function *p,
		 struct x86_reg dst,
		 struct x86_reg src )
{
   assert(dst.mod != mod_REG || src.mod != mod_REG);
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x16, 0x17, dst, src ); /* cf movlhps */
}

void sse_movlps( struct x86_function *p,
		 struct x86_reg dst,
		 struct x86_reg src )
{
   assert(dst.mod != mod_REG || src.mod != mod_REG);
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x12, 0x13, dst, src ); /* cf movhlps */
}

void sse_maxps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x5F);
   emit_modrm( p, dst, src );
}

void sse_maxss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_3ub(p, 0xF3, X86_TWOB, 0x5F);
   emit_modrm( p, dst, src );
}

void sse_divss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_3ub(p, 0xF3, X86_TWOB, 0x5E);
   emit_modrm( p, dst, src );
}

void sse_minps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x5D);
   emit_modrm( p, dst, src );
}

void sse_subps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x5C);
   emit_modrm( p, dst, src );
}

void sse_mulps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x59);
   emit_modrm( p, dst, src );
}

void sse_mulss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_3ub(p, 0xF3, X86_TWOB, 0x59);
   emit_modrm( p, dst, src );
}

void sse_addps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x58);
   emit_modrm( p, dst, src );
}

void sse_addss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_3ub(p, 0xF3, X86_TWOB, 0x58);
   emit_modrm( p, dst, src );
}

void sse_andnps( struct x86_function *p,
                 struct x86_reg dst,
                 struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x55);
   emit_modrm( p, dst, src );
}

void sse_andps( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x54);
   emit_modrm( p, dst, src );
}

void sse_rsqrtps( struct x86_function *p,
                  struct x86_reg dst,
                  struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x52);
   emit_modrm( p, dst, src );
}

void sse_rsqrtss( struct x86_function *p,
		  struct x86_reg dst,
		  struct x86_reg src )
{
   emit_3ub(p, 0xF3, X86_TWOB, 0x52);
   emit_modrm( p, dst, src );

}

void sse_movhlps( struct x86_function *p,
		  struct x86_reg dst,
		  struct x86_reg src )
{
   assert(dst.mod == mod_REG && src.mod == mod_REG);
   emit_2ub(p, X86_TWOB, 0x12);
   emit_modrm( p, dst, src );
}

void sse_movlhps( struct x86_function *p,
		  struct x86_reg dst,
		  struct x86_reg src )
{
   assert(dst.mod == mod_REG && src.mod == mod_REG);
   emit_2ub(p, X86_TWOB, 0x16);
   emit_modrm( p, dst, src );
}

void sse_orps( struct x86_function *p,
               struct x86_reg dst,
               struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x56);
   emit_modrm( p, dst, src );
}

void sse_xorps( struct x86_function *p,
                struct x86_reg dst,
                struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x57);
   emit_modrm( p, dst, src );
}

void sse_cvtps2pi( struct x86_function *p,
		   struct x86_reg dst,
		   struct x86_reg src )
{
   assert(dst.file == file_MMX && 
	  (src.file == file_XMM || src.mod != mod_REG));

   p->need_emms = 1;

   emit_2ub(p, X86_TWOB, 0x2d);
   emit_modrm( p, dst, src );
}


/* Shufps can also be used to implement a reduced swizzle when dest ==
 * arg0.
 */
void sse_shufps( struct x86_function *p,
		 struct x86_reg dest,
		 struct x86_reg arg0,
		 unsigned char shuf) 
{
   emit_2ub(p, X86_TWOB, 0xC6);
   emit_modrm(p, dest, arg0);
   emit_1ub(p, shuf); 
}

void sse_cmpps( struct x86_function *p,
		struct x86_reg dest,
		struct x86_reg arg0,
		unsigned char cc) 
{
   emit_2ub(p, X86_TWOB, 0xC2);
   emit_modrm(p, dest, arg0);
   emit_1ub(p, cc); 
}

void sse_pmovmskb( struct x86_function *p,
                   struct x86_reg dest,
                   struct x86_reg src)
{
    emit_3ub(p, 0x66, X86_TWOB, 0xD7);
    emit_modrm(p, dest, src);
}

/***********************************************************************
 * SSE2 instructions
 */

/**
 * Perform a reduced swizzle:
 */
void sse2_pshufd( struct x86_function *p,
		  struct x86_reg dest,
		  struct x86_reg arg0,
		  unsigned char shuf) 
{
   emit_3ub(p, 0x66, X86_TWOB, 0x70);
   emit_modrm(p, dest, arg0);
   emit_1ub(p, shuf); 
}

void sse2_cvttps2dq( struct x86_function *p,
                     struct x86_reg dst,
                     struct x86_reg src )
{
   emit_3ub( p, 0xF3, X86_TWOB, 0x5B );
   emit_modrm( p, dst, src );
}

void sse2_cvtps2dq( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   emit_3ub(p, 0x66, X86_TWOB, 0x5B);
   emit_modrm( p, dst, src );
}

void sse2_packssdw( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   emit_3ub(p, 0x66, X86_TWOB, 0x6B);
   emit_modrm( p, dst, src );
}

void sse2_packsswb( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   emit_3ub(p, 0x66, X86_TWOB, 0x63);
   emit_modrm( p, dst, src );
}

void sse2_packuswb( struct x86_function *p,
		    struct x86_reg dst,
		    struct x86_reg src )
{
   emit_3ub(p, 0x66, X86_TWOB, 0x67);
   emit_modrm( p, dst, src );
}

void sse2_rcpps( struct x86_function *p,
                 struct x86_reg dst,
                 struct x86_reg src )
{
   emit_2ub(p, X86_TWOB, 0x53);
   emit_modrm( p, dst, src );
}

void sse2_rcpss( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_3ub(p, 0xF3, X86_TWOB, 0x53);
   emit_modrm( p, dst, src );
}

void sse2_movd( struct x86_function *p,
		struct x86_reg dst,
		struct x86_reg src )
{
   emit_2ub(p, 0x66, X86_TWOB);
   emit_op_modrm( p, 0x6e, 0x7e, dst, src );
}




/***********************************************************************
 * x87 instructions
 */
void x87_fist( struct x86_function *p, struct x86_reg dst )
{
   emit_1ub(p, 0xdb);
   emit_modrm_noreg(p, 2, dst);
}

void x87_fistp( struct x86_function *p, struct x86_reg dst )
{
   emit_1ub(p, 0xdb);
   emit_modrm_noreg(p, 3, dst);
}

void x87_fild( struct x86_function *p, struct x86_reg arg )
{
   emit_1ub(p, 0xdf);
   emit_modrm_noreg(p, 0, arg);
}

void x87_fldz( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xee);
}


void x87_fldcw( struct x86_function *p, struct x86_reg arg )
{
   assert(arg.file == file_REG32);
   assert(arg.mod != mod_REG);
   emit_1ub(p, 0xd9);
   emit_modrm_noreg(p, 5, arg);
}

void x87_fld1( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xe8);
}

void x87_fldl2e( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xea);
}

void x87_fldln2( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xed);
}

void x87_fwait( struct x86_function *p )
{
   emit_1ub(p, 0x9b);
}

void x87_fnclex( struct x86_function *p )
{
   emit_2ub(p, 0xdb, 0xe2);
}

void x87_fclex( struct x86_function *p )
{
   x87_fwait(p);
   x87_fnclex(p);
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

void x87_fmul( struct x86_function *p, struct x86_reg dst, struct x86_reg arg )
{
   x87_arith_op(p, dst, arg, 
		0xd8, 0xc8,
		0xdc, 0xc8,
		4);
}

void x87_fsub( struct x86_function *p, struct x86_reg dst, struct x86_reg arg )
{
   x87_arith_op(p, dst, arg, 
		0xd8, 0xe0,
		0xdc, 0xe8,
		4);
}

void x87_fsubr( struct x86_function *p, struct x86_reg dst, struct x86_reg arg )
{
   x87_arith_op(p, dst, arg, 
		0xd8, 0xe8,
		0xdc, 0xe0,
		5);
}

void x87_fadd( struct x86_function *p, struct x86_reg dst, struct x86_reg arg )
{
   x87_arith_op(p, dst, arg, 
		0xd8, 0xc0,
		0xdc, 0xc0,
		0);
}

void x87_fdiv( struct x86_function *p, struct x86_reg dst, struct x86_reg arg )
{
   x87_arith_op(p, dst, arg, 
		0xd8, 0xf0,
		0xdc, 0xf8,
		6);
}

void x87_fdivr( struct x86_function *p, struct x86_reg dst, struct x86_reg arg )
{
   x87_arith_op(p, dst, arg, 
		0xd8, 0xf8,
		0xdc, 0xf0,
		7);
}

void x87_fmulp( struct x86_function *p, struct x86_reg dst )
{
   assert(dst.file == file_x87);
   assert(dst.idx >= 1);
   emit_2ub(p, 0xde, 0xc8+dst.idx);
}

void x87_fsubp( struct x86_function *p, struct x86_reg dst )
{
   assert(dst.file == file_x87);
   assert(dst.idx >= 1);
   emit_2ub(p, 0xde, 0xe8+dst.idx);
}

void x87_fsubrp( struct x86_function *p, struct x86_reg dst )
{
   assert(dst.file == file_x87);
   assert(dst.idx >= 1);
   emit_2ub(p, 0xde, 0xe0+dst.idx);
}

void x87_faddp( struct x86_function *p, struct x86_reg dst )
{
   assert(dst.file == file_x87);
   assert(dst.idx >= 1);
   emit_2ub(p, 0xde, 0xc0+dst.idx);
}

void x87_fdivp( struct x86_function *p, struct x86_reg dst )
{
   assert(dst.file == file_x87);
   assert(dst.idx >= 1);
   emit_2ub(p, 0xde, 0xf8+dst.idx);
}

void x87_fdivrp( struct x86_function *p, struct x86_reg dst )
{
   assert(dst.file == file_x87);
   assert(dst.idx >= 1);
   emit_2ub(p, 0xde, 0xf0+dst.idx);
}

void x87_fucom( struct x86_function *p, struct x86_reg arg )
{
   assert(arg.file == file_x87);
   emit_2ub(p, 0xdd, 0xe0+arg.idx);
}

void x87_fucomp( struct x86_function *p, struct x86_reg arg )
{
   assert(arg.file == file_x87);
   emit_2ub(p, 0xdd, 0xe8+arg.idx);
}

void x87_fucompp( struct x86_function *p )
{
   emit_2ub(p, 0xda, 0xe9);
}

void x87_fxch( struct x86_function *p, struct x86_reg arg )
{
   assert(arg.file == file_x87);
   emit_2ub(p, 0xd9, 0xc8+arg.idx);
}

void x87_fabs( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xe1);
}

void x87_fchs( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xe0);
}

void x87_fcos( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xff);
}


void x87_fprndint( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xfc);
}

void x87_fscale( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xfd);
}

void x87_fsin( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xfe);
}

void x87_fsincos( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xfb);
}

void x87_fsqrt( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xfa);
}

void x87_fxtract( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xf4);
}

/* st0 = (2^st0)-1
 *
 * Restrictions: -1.0 <= st0 <= 1.0
 */
void x87_f2xm1( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xf0);
}

/* st1 = st1 * log2(st0);
 * pop_stack;
 */
void x87_fyl2x( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xf1);
}

/* st1 = st1 * log2(st0 + 1.0);
 * pop_stack;
 *
 * A fast operation, with restrictions: -.29 < st0 < .29 
 */
void x87_fyl2xp1( struct x86_function *p )
{
   emit_2ub(p, 0xd9, 0xf9);
}


void x87_fld( struct x86_function *p, struct x86_reg arg )
{
   if (arg.file == file_x87) 
      emit_2ub(p, 0xd9, 0xc0 + arg.idx);
   else {
      emit_1ub(p, 0xd9);
      emit_modrm_noreg(p, 0, arg);
   }
}

void x87_fst( struct x86_function *p, struct x86_reg dst )
{
   if (dst.file == file_x87) 
      emit_2ub(p, 0xdd, 0xd0 + dst.idx);
   else {
      emit_1ub(p, 0xd9);
      emit_modrm_noreg(p, 2, dst);
   }
}

void x87_fstp( struct x86_function *p, struct x86_reg dst )
{
   if (dst.file == file_x87) 
      emit_2ub(p, 0xdd, 0xd8 + dst.idx);
   else {
      emit_1ub(p, 0xd9);
      emit_modrm_noreg(p, 3, dst);
   }
}

void x87_fcom( struct x86_function *p, struct x86_reg dst )
{
   if (dst.file == file_x87) 
      emit_2ub(p, 0xd8, 0xd0 + dst.idx);
   else {
      emit_1ub(p, 0xd8);
      emit_modrm_noreg(p, 2, dst);
   }
}

void x87_fcomp( struct x86_function *p, struct x86_reg dst )
{
   if (dst.file == file_x87) 
      emit_2ub(p, 0xd8, 0xd8 + dst.idx);
   else {
      emit_1ub(p, 0xd8);
      emit_modrm_noreg(p, 3, dst);
   }
}


void x87_fnstsw( struct x86_function *p, struct x86_reg dst )
{
   assert(dst.file == file_REG32);

   if (dst.idx == reg_AX &&
       dst.mod == mod_REG) 
      emit_2ub(p, 0xdf, 0xe0);
   else {
      emit_1ub(p, 0xdd);
      emit_modrm_noreg(p, 7, dst);
   }
}




/***********************************************************************
 * MMX instructions
 */

void mmx_emms( struct x86_function *p )
{
   assert(p->need_emms);
   emit_2ub(p, 0x0f, 0x77);
   p->need_emms = 0;
}

void mmx_packssdw( struct x86_function *p,
		   struct x86_reg dst,
		   struct x86_reg src )
{
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
   p->need_emms = 1;
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x6e, 0x7e, dst, src );
}

void mmx_movq( struct x86_function *p,
	       struct x86_reg dst,
	       struct x86_reg src )
{
   p->need_emms = 1;
   emit_1ub(p, X86_TWOB);
   emit_op_modrm( p, 0x6f, 0x7f, dst, src );
}


/***********************************************************************
 * Helper functions
 */


/* Retreive a reference to one of the function arguments, taking into
 * account any push/pop activity:
 */
struct x86_reg x86_fn_arg( struct x86_function *p,
			   unsigned arg )
{
   return x86_make_disp(x86_make_reg(file_REG32, reg_SP), 
			p->stack_offset + arg * 4);	/* ??? */
}


void x86_init_func( struct x86_function *p )
{
   p->size = 0;
   p->store = NULL;
   p->csr = p->store;
}

int x86_init_func_size( struct x86_function *p, unsigned code_size )
{
   p->size = code_size;
   p->store = _mesa_exec_malloc(code_size);
   p->csr = p->store;
   return p->store != NULL;
}

void x86_release_func( struct x86_function *p )
{
   _mesa_exec_free(p->store);
   p->store = NULL;
   p->csr = NULL;
   p->size = 0;
}


void (*x86_get_func( struct x86_function *p ))(void)
{
   if (DISASSEM && p->store)
      _mesa_printf("disassemble %p %p\n", p->store, p->csr);
   return (void (*)(void)) (unsigned long) p->store;
}

#else

void x86sse_dummy( void )
{
}

#endif
