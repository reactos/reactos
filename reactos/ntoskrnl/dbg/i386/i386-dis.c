#include <stdarg.h>

/* ReactOS compatibility stuff. */
#define PARAMS(X) X
#define PTR void*
typedef enum bfd_flavour
{
  bfd_target_unknown_flavour,
} bfd_flavour;
typedef enum bfd_architecture
{
  bfd_arch_i386,  
} bfd_arch;
typedef unsigned int bfd_vma;
typedef unsigned char bfd_byte;
enum bfd_endian { BFD_ENDIAN_BIG, BIG_ENDIAN_LITTLE, BFD_ENDIAN_UNKNOWN };
typedef void* bfd;
typedef void* FILE;
typedef signed int bfd_signed_vma;
#define NULL 0
#define bfd_mach_x86_64_intel_syntax 0
#define bfd_mach_x86_64 1
#define bfd_mach_i386_i386_intel_syntax 2
#define bfd_mach_i386_i386 3
#define bfd_mach_i386_i8086 4
#define abort() __asm__("int $3\n\t")
#define _(X) X
#define ATTRIBUTE_UNUSED
extern char* strcpy(char *dest, const char *src);
extern unsigned int strlen(const char *s);
extern int sprintf(char *str, const char *format, ...);
extern int vsprintf(char *buf, const char *format, va_list ap);
void* memcpy(void *dest, const void *src, unsigned int length);
extern void DbgPrint(const char *format, ...);
#define sprintf_vma(BUF, VMA) sprintf(BUF, "0x%X", VMA)
#define _setjmp setjmp
unsigned int
KdbPrintAddress(void* address);

struct disassemble_info;

int
print_insn_i386_att (bfd_vma pc, struct disassemble_info *info);

int
KdbPrintDisasm(void* Ignored, const char* fmt, ...)
{
  va_list ap;
  static char buffer[256];
  int ret;

  va_start(ap, fmt);
  ret = vsprintf(buffer, fmt, ap);
  DbgPrint(buffer);
  va_end(ap);
  return(ret);
}

int
KdbNopPrintDisasm(void* Ignored, const char* fmt, ...)
{
  return(0);
}

int static
KdbReadMemory(unsigned int Addr, unsigned char* Data, unsigned int Length, 
	      struct disassemble_info * Ignored)
{
  memcpy(Data, (unsigned char*)Addr, Length);
  return(0);
}

void static
KdbMemoryError(int Status, unsigned int Addr, 
	       struct disassemble_info * Ignored)
{
}

void static
KdbPrintAddressInCode(unsigned int Addr, struct disassemble_info * Ignored)
{
  if (!KdbPrintAddress((void*)Addr))
    {
      DbgPrint("<0x%X>", Addr);
    }
}

void static
KdbNopPrintAddress(unsigned int Addr, struct disassemble_info * Ignored)
{
}

#include "dis-asm.h"

long
KdbGetInstLength(unsigned int Address)
{
  disassemble_info info;

  info.fprintf_func = KdbNopPrintDisasm;
  info.stream = NULL;
  info.application_data = NULL;
  info.flavour = bfd_target_unknown_flavour;
  info.arch = bfd_arch_i386;
  info.mach = bfd_mach_i386_i386;
  info.insn_sets = 0;
  info.flags = 0;
  info.read_memory_func = KdbReadMemory;
  info.memory_error_func = KdbMemoryError;
  info.print_address_func = KdbNopPrintAddress;
  info.symbol_at_address_func = NULL;
  info.buffer = NULL;
  info.buffer_vma = info.buffer_length = 0;
  info.bytes_per_chunk = 0;
  info.display_endian = BIG_ENDIAN_LITTLE;
  info.disassembler_options = NULL;

  return(print_insn_i386_att(Address, &info));
}

long
KdbDisassemble(unsigned int Address)
{
  disassemble_info info;

  info.fprintf_func = KdbPrintDisasm;
  info.stream = NULL;
  info.application_data = NULL;
  info.flavour = bfd_target_unknown_flavour;
  info.arch = bfd_arch_i386;
  info.mach = bfd_mach_i386_i386;
  info.insn_sets = 0;
  info.flags = 0;
  info.read_memory_func = KdbReadMemory;
  info.memory_error_func = KdbMemoryError;
  info.print_address_func = KdbPrintAddressInCode;
  info.symbol_at_address_func = NULL;
  info.buffer = NULL;
  info.buffer_vma = info.buffer_length = 0;
  info.bytes_per_chunk = 0;
  info.display_endian = BIG_ENDIAN_LITTLE;
  info.disassembler_options = NULL;

  return(print_insn_i386_att(Address, &info));
}

/* Print i386 instructions for GDB, the GNU debugger.
   Copyright 1988, 1989, 1991, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2001
   Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/*
 * 80386 instruction printer by Pace Willisson (pace@prep.ai.mit.edu)
 * July 1988
 *  modified by John Hassey (hassey@dg-rtp.dg.com)
 *  x86-64 support added by Jan Hubicka (jh@suse.cz)
 */

/*
 * The main tables describing the instructions is essentially a copy
 * of the "Opcode Map" chapter (Appendix A) of the Intel 80386
 * Programmers Manual.  Usually, there is a capital letter, followed
 * by a small letter.  The capital letter tell the addressing mode,
 * and the small letter tells about the operand size.  Refer to
 * the Intel manual for details.
 */

#include "dis-asm.h"
#if 0
#include "sysdep.h"
#include "opintl.h"
#endif

#define MAXLEN 20

#include <setjmp.h>

#ifndef UNIXWARE_COMPAT
/* Set non-zero for broken, compatible instructions.  Set to zero for
   non-broken opcodes.  */
#define UNIXWARE_COMPAT 1
#endif

static int fetch_data PARAMS ((struct disassemble_info *, bfd_byte *));
static void ckprefix PARAMS ((void));
static const char *prefix_name PARAMS ((int, int));
static int print_insn PARAMS ((bfd_vma, disassemble_info *));
static void dofloat PARAMS ((int));
static void OP_ST PARAMS ((int, int));
static void OP_STi  PARAMS ((int, int));
static int putop PARAMS ((const char *, int));
static void oappend PARAMS ((const char *));
static void append_seg PARAMS ((void));
static void OP_indirE PARAMS ((int, int));
static void print_operand_value PARAMS ((char *, int, bfd_vma));
static void OP_E PARAMS ((int, int));
static void OP_G PARAMS ((int, int));
static bfd_vma get64 PARAMS ((void));
static bfd_signed_vma get32 PARAMS ((void));
static bfd_signed_vma get32s PARAMS ((void));
static int get16 PARAMS ((void));
static void set_op PARAMS ((bfd_vma, int));
static void OP_REG PARAMS ((int, int));
static void OP_IMREG PARAMS ((int, int));
static void OP_I PARAMS ((int, int));
static void OP_I64 PARAMS ((int, int));
static void OP_sI PARAMS ((int, int));
static void OP_J PARAMS ((int, int));
static void OP_SEG PARAMS ((int, int));
static void OP_DIR PARAMS ((int, int));
static void OP_OFF PARAMS ((int, int));
static void OP_OFF64 PARAMS ((int, int));
static void ptr_reg PARAMS ((int, int));
static void OP_ESreg PARAMS ((int, int));
static void OP_DSreg PARAMS ((int, int));
static void OP_C PARAMS ((int, int));
static void OP_D PARAMS ((int, int));
static void OP_T PARAMS ((int, int));
static void OP_Rd PARAMS ((int, int));
static void OP_MMX PARAMS ((int, int));
static void OP_XMM PARAMS ((int, int));
static void OP_EM PARAMS ((int, int));
static void OP_EX PARAMS ((int, int));
static void OP_MS PARAMS ((int, int));
static void OP_XS PARAMS ((int, int));
static void OP_3DNowSuffix PARAMS ((int, int));
static void OP_SIMD_Suffix PARAMS ((int, int));
static void SIMD_Fixup PARAMS ((int, int));
static void BadOp PARAMS ((void));

struct dis_private {
  /* Points to first byte not fetched.  */
  bfd_byte *max_fetched;
  bfd_byte the_buffer[MAXLEN];
  bfd_vma insn_start;
  int orig_sizeflag;
  jmp_buf bailout;
};

/* The opcode for the fwait instruction, which we treat as a prefix
   when we can.  */
#define FWAIT_OPCODE (0x9b)

/* Set to 1 for 64bit mode disassembly.  */
static int mode_64bit;

/* Flags for the prefixes for the current instruction.  See below.  */
static int prefixes;

/* REX prefix the current instruction.  See below.  */
static int rex;
/* Bits of REX we've already used.  */
static int rex_used;
#define REX_MODE64	8
#define REX_EXTX	4
#define REX_EXTY	2
#define REX_EXTZ	1
/* Mark parts used in the REX prefix.  When we are testing for
   empty prefix (for 8bit register REX extension), just mask it
   out.  Otherwise test for REX bit is excuse for existence of REX
   only in case value is nonzero.  */
#define USED_REX(value)					\
  {							\
    if (value)						\
      rex_used |= (rex & value) ? (value) | 0x40 : 0;	\
    else						\
      rex_used |= 0x40;					\
  }

/* Flags for prefixes which we somehow handled when printing the
   current instruction.  */
static int used_prefixes;

/* Flags stored in PREFIXES.  */
#define PREFIX_REPZ 1
#define PREFIX_REPNZ 2
#define PREFIX_LOCK 4
#define PREFIX_CS 8
#define PREFIX_SS 0x10
#define PREFIX_DS 0x20
#define PREFIX_ES 0x40
#define PREFIX_FS 0x80
#define PREFIX_GS 0x100
#define PREFIX_DATA 0x200
#define PREFIX_ADDR 0x400
#define PREFIX_FWAIT 0x800

/* Make sure that bytes from INFO->PRIVATE_DATA->BUFFER (inclusive)
   to ADDR (exclusive) are valid.  Returns 1 for success, longjmps
   on error.  */
#define FETCH_DATA(info, addr) \
  ((addr) <= ((struct dis_private *) (info->private_data))->max_fetched \
   ? 1 : fetch_data ((info), (addr)))

static int
fetch_data (info, addr)
     struct disassemble_info *info;
     bfd_byte *addr;
{
  int status;
  struct dis_private *priv = (struct dis_private *) info->private_data;
  bfd_vma start = priv->insn_start + (priv->max_fetched - priv->the_buffer);

  status = (*info->read_memory_func) (start,
				      priv->max_fetched,
				      addr - priv->max_fetched,
				      info);
  if (status != 0)
    {
      /* If we did manage to read at least one byte, then
         print_insn_i386 will do something sensible.  Otherwise, print
         an error.  We do that here because this is where we know
         STATUS.  */
      if (priv->max_fetched == priv->the_buffer)
	(*info->memory_error_func) (status, start, info);
      longjmp (priv->bailout, 1);
    }
  else
    priv->max_fetched = addr;
  return 1;
}

#define XX NULL, 0

#define Eb OP_E, b_mode
#define Ev OP_E, v_mode
#define Ed OP_E, d_mode
#define indirEb OP_indirE, b_mode
#define indirEv OP_indirE, v_mode
#define Ew OP_E, w_mode
#define Ma OP_E, v_mode
#define M OP_E, 0		/* lea, lgdt, etc. */
#define Mp OP_E, 0		/* 32 or 48 bit memory operand for LDS, LES etc */
#define Gb OP_G, b_mode
#define Gv OP_G, v_mode
#define Gd OP_G, d_mode
#define Gw OP_G, w_mode
#define Rd OP_Rd, d_mode
#define Rm OP_Rd, m_mode
#define Ib OP_I, b_mode
#define sIb OP_sI, b_mode	/* sign extened byte */
#define Iv OP_I, v_mode
#define Iq OP_I, q_mode
#define Iv64 OP_I64, v_mode
#define Iw OP_I, w_mode
#define Jb OP_J, b_mode
#define Jv OP_J, v_mode
#define Cm OP_C, m_mode
#define Dm OP_D, m_mode
#define Td OP_T, d_mode

#define RMeAX OP_REG, eAX_reg
#define RMeBX OP_REG, eBX_reg
#define RMeCX OP_REG, eCX_reg
#define RMeDX OP_REG, eDX_reg
#define RMeSP OP_REG, eSP_reg
#define RMeBP OP_REG, eBP_reg
#define RMeSI OP_REG, eSI_reg
#define RMeDI OP_REG, eDI_reg
#define RMrAX OP_REG, rAX_reg
#define RMrBX OP_REG, rBX_reg
#define RMrCX OP_REG, rCX_reg
#define RMrDX OP_REG, rDX_reg
#define RMrSP OP_REG, rSP_reg
#define RMrBP OP_REG, rBP_reg
#define RMrSI OP_REG, rSI_reg
#define RMrDI OP_REG, rDI_reg
#define RMAL OP_REG, al_reg
#define RMAL OP_REG, al_reg
#define RMCL OP_REG, cl_reg
#define RMDL OP_REG, dl_reg
#define RMBL OP_REG, bl_reg
#define RMAH OP_REG, ah_reg
#define RMCH OP_REG, ch_reg
#define RMDH OP_REG, dh_reg
#define RMBH OP_REG, bh_reg
#define RMAX OP_REG, ax_reg
#define RMDX OP_REG, dx_reg

#define eAX OP_IMREG, eAX_reg
#define eBX OP_IMREG, eBX_reg
#define eCX OP_IMREG, eCX_reg
#define eDX OP_IMREG, eDX_reg
#define eSP OP_IMREG, eSP_reg
#define eBP OP_IMREG, eBP_reg
#define eSI OP_IMREG, eSI_reg
#define eDI OP_IMREG, eDI_reg
#define AL OP_IMREG, al_reg
#define AL OP_IMREG, al_reg
#define CL OP_IMREG, cl_reg
#define DL OP_IMREG, dl_reg
#define BL OP_IMREG, bl_reg
#define AH OP_IMREG, ah_reg
#define CH OP_IMREG, ch_reg
#define DH OP_IMREG, dh_reg
#define BH OP_IMREG, bh_reg
#define AX OP_IMREG, ax_reg
#define DX OP_IMREG, dx_reg
#define indirDX OP_IMREG, indir_dx_reg

#define Sw OP_SEG, w_mode
#define Ap OP_DIR, 0
#define Ob OP_OFF, b_mode
#define Ob64 OP_OFF64, b_mode
#define Ov OP_OFF, v_mode
#define Ov64 OP_OFF64, v_mode
#define Xb OP_DSreg, eSI_reg
#define Xv OP_DSreg, eSI_reg
#define Yb OP_ESreg, eDI_reg
#define Yv OP_ESreg, eDI_reg
#define DSBX OP_DSreg, eBX_reg

#define es OP_REG, es_reg
#define ss OP_REG, ss_reg
#define cs OP_REG, cs_reg
#define ds OP_REG, ds_reg
#define fs OP_REG, fs_reg
#define gs OP_REG, gs_reg

#define MX OP_MMX, 0
#define XM OP_XMM, 0
#define EM OP_EM, v_mode
#define EX OP_EX, v_mode
#define MS OP_MS, v_mode
#define XS OP_XS, v_mode
#define None OP_E, 0
#define OPSUF OP_3DNowSuffix, 0
#define OPSIMD OP_SIMD_Suffix, 0

#define cond_jump_flag NULL, cond_jump_mode
#define loop_jcxz_flag NULL, loop_jcxz_mode

/* bits in sizeflag */
#define SUFFIX_ALWAYS 4
#define AFLAG 2
#define DFLAG 1

#define b_mode 1  /* byte operand */
#define v_mode 2  /* operand size depends on prefixes */
#define w_mode 3  /* word operand */
#define d_mode 4  /* double word operand  */
#define q_mode 5  /* quad word operand */
#define x_mode 6
#define m_mode 7  /* d_mode in 32bit, q_mode in 64bit mode.  */
#define cond_jump_mode 8
#define loop_jcxz_mode 9

#define es_reg 100
#define cs_reg 101
#define ss_reg 102
#define ds_reg 103
#define fs_reg 104
#define gs_reg 105

#define eAX_reg 108
#define eCX_reg 109
#define eDX_reg 110
#define eBX_reg 111
#define eSP_reg 112
#define eBP_reg 113
#define eSI_reg 114
#define eDI_reg 115

#define al_reg 116
#define cl_reg 117
#define dl_reg 118
#define bl_reg 119
#define ah_reg 120
#define ch_reg 121
#define dh_reg 122
#define bh_reg 123

#define ax_reg 124
#define cx_reg 125
#define dx_reg 126
#define bx_reg 127
#define sp_reg 128
#define bp_reg 129
#define si_reg 130
#define di_reg 131

#define rAX_reg 132
#define rCX_reg 133
#define rDX_reg 134
#define rBX_reg 135
#define rSP_reg 136
#define rBP_reg 137
#define rSI_reg 138
#define rDI_reg 139

#define indir_dx_reg 150

#define FLOATCODE 1
#define USE_GROUPS 2
#define USE_PREFIX_USER_TABLE 3
#define X86_64_SPECIAL 4

#define FLOAT	  NULL, NULL, FLOATCODE, NULL, 0, NULL, 0

#define GRP1b	  NULL, NULL, USE_GROUPS, NULL,  0, NULL, 0
#define GRP1S	  NULL, NULL, USE_GROUPS, NULL,  1, NULL, 0
#define GRP1Ss	  NULL, NULL, USE_GROUPS, NULL,  2, NULL, 0
#define GRP2b	  NULL, NULL, USE_GROUPS, NULL,  3, NULL, 0
#define GRP2S	  NULL, NULL, USE_GROUPS, NULL,  4, NULL, 0
#define GRP2b_one NULL, NULL, USE_GROUPS, NULL,  5, NULL, 0
#define GRP2S_one NULL, NULL, USE_GROUPS, NULL,  6, NULL, 0
#define GRP2b_cl  NULL, NULL, USE_GROUPS, NULL,  7, NULL, 0
#define GRP2S_cl  NULL, NULL, USE_GROUPS, NULL,  8, NULL, 0
#define GRP3b	  NULL, NULL, USE_GROUPS, NULL,  9, NULL, 0
#define GRP3S	  NULL, NULL, USE_GROUPS, NULL, 10, NULL, 0
#define GRP4	  NULL, NULL, USE_GROUPS, NULL, 11, NULL, 0
#define GRP5	  NULL, NULL, USE_GROUPS, NULL, 12, NULL, 0
#define GRP6	  NULL, NULL, USE_GROUPS, NULL, 13, NULL, 0
#define GRP7	  NULL, NULL, USE_GROUPS, NULL, 14, NULL, 0
#define GRP8	  NULL, NULL, USE_GROUPS, NULL, 15, NULL, 0
#define GRP9	  NULL, NULL, USE_GROUPS, NULL, 16, NULL, 0
#define GRP10	  NULL, NULL, USE_GROUPS, NULL, 17, NULL, 0
#define GRP11	  NULL, NULL, USE_GROUPS, NULL, 18, NULL, 0
#define GRP12	  NULL, NULL, USE_GROUPS, NULL, 19, NULL, 0
#define GRP13	  NULL, NULL, USE_GROUPS, NULL, 20, NULL, 0
#define GRP14	  NULL, NULL, USE_GROUPS, NULL, 21, NULL, 0
#define GRPAMD	  NULL, NULL, USE_GROUPS, NULL, 22, NULL, 0

#define PREGRP0   NULL, NULL, USE_PREFIX_USER_TABLE, NULL,  0, NULL, 0
#define PREGRP1   NULL, NULL, USE_PREFIX_USER_TABLE, NULL,  1, NULL, 0
#define PREGRP2   NULL, NULL, USE_PREFIX_USER_TABLE, NULL,  2, NULL, 0
#define PREGRP3   NULL, NULL, USE_PREFIX_USER_TABLE, NULL,  3, NULL, 0
#define PREGRP4   NULL, NULL, USE_PREFIX_USER_TABLE, NULL,  4, NULL, 0
#define PREGRP5   NULL, NULL, USE_PREFIX_USER_TABLE, NULL,  5, NULL, 0
#define PREGRP6   NULL, NULL, USE_PREFIX_USER_TABLE, NULL,  6, NULL, 0
#define PREGRP7   NULL, NULL, USE_PREFIX_USER_TABLE, NULL,  7, NULL, 0
#define PREGRP8   NULL, NULL, USE_PREFIX_USER_TABLE, NULL,  8, NULL, 0
#define PREGRP9   NULL, NULL, USE_PREFIX_USER_TABLE, NULL,  9, NULL, 0
#define PREGRP10  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 10, NULL, 0
#define PREGRP11  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 11, NULL, 0
#define PREGRP12  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 12, NULL, 0
#define PREGRP13  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 13, NULL, 0
#define PREGRP14  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 14, NULL, 0
#define PREGRP15  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 15, NULL, 0
#define PREGRP16  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 16, NULL, 0
#define PREGRP17  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 17, NULL, 0
#define PREGRP18  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 18, NULL, 0
#define PREGRP19  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 19, NULL, 0
#define PREGRP20  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 20, NULL, 0
#define PREGRP21  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 21, NULL, 0
#define PREGRP22  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 22, NULL, 0
#define PREGRP23  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 23, NULL, 0
#define PREGRP24  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 24, NULL, 0
#define PREGRP25  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 25, NULL, 0
#define PREGRP26  NULL, NULL, USE_PREFIX_USER_TABLE, NULL, 26, NULL, 0

#define X86_64_0  NULL, NULL, X86_64_SPECIAL, NULL,  0, NULL, 0

typedef void (*op_rtn) PARAMS ((int bytemode, int sizeflag));

struct dis386 {
  const char *name;
  op_rtn op1;
  int bytemode1;
  op_rtn op2;
  int bytemode2;
  op_rtn op3;
  int bytemode3;
};

/* Upper case letters in the instruction names here are macros.
   'A' => print 'b' if no register operands or suffix_always is true
   'B' => print 'b' if suffix_always is true
   'E' => print 'e' if 32-bit form of jcxz
   'F' => print 'w' or 'l' depending on address size prefix (loop insns)
   'H' => print ",pt" or ",pn" branch hint
   'L' => print 'l' if suffix_always is true
   'N' => print 'n' if instruction has no wait "prefix"
   'O' => print 'd', or 'o'
   'P' => print 'w', 'l' or 'q' if instruction has an operand size prefix,
   .      or suffix_always is true.  print 'q' if rex prefix is present.
   'Q' => print 'w', 'l' or 'q' if no register operands or suffix_always
   .      is true
   'R' => print 'w', 'l' or 'q' ("wd" or "dq" in intel mode)
   'S' => print 'w', 'l' or 'q' if suffix_always is true
   'T' => print 'q' in 64bit mode and behave as 'P' otherwise
   'U' => print 'q' in 64bit mode and behave as 'Q' otherwise
   'X' => print 's', 'd' depending on data16 prefix (for XMM)
   'W' => print 'b' or 'w' ("w" or "de" in intel mode)
   'Y' => 'q' if instruction has an REX 64bit overwrite prefix

   Many of the above letters print nothing in Intel mode.  See "putop"
   for the details.

   Braces '{' and '}', and vertical bars '|', indicate alternative
   mnemonic strings for AT&T, Intel, X86_64 AT&T, and X86_64 Intel
   modes.  In cases where there are only two alternatives, the X86_64
   instruction is reserved, and "(bad)" is printed.
*/

static const struct dis386 dis386[] = {
  /* 00 */
  { "addB",		Eb, Gb, XX },
  { "addS",		Ev, Gv, XX },
  { "addB",		Gb, Eb, XX },
  { "addS",		Gv, Ev, XX },
  { "addB",		AL, Ib, XX },
  { "addS",		eAX, Iv, XX },
  { "push{T|}",		es, XX, XX },
  { "pop{T|}",		es, XX, XX },
  /* 08 */
  { "orB",		Eb, Gb, XX },
  { "orS",		Ev, Gv, XX },
  { "orB",		Gb, Eb, XX },
  { "orS",		Gv, Ev, XX },
  { "orB",		AL, Ib, XX },
  { "orS",		eAX, Iv, XX },
  { "push{T|}",		cs, XX, XX },
  { "(bad)",		XX, XX, XX },	/* 0x0f extended opcode escape */
  /* 10 */
  { "adcB",		Eb, Gb, XX },
  { "adcS",		Ev, Gv, XX },
  { "adcB",		Gb, Eb, XX },
  { "adcS",		Gv, Ev, XX },
  { "adcB",		AL, Ib, XX },
  { "adcS",		eAX, Iv, XX },
  { "push{T|}",		ss, XX, XX },
  { "popT|}",		ss, XX, XX },
  /* 18 */
  { "sbbB",		Eb, Gb, XX },
  { "sbbS",		Ev, Gv, XX },
  { "sbbB",		Gb, Eb, XX },
  { "sbbS",		Gv, Ev, XX },
  { "sbbB",		AL, Ib, XX },
  { "sbbS",		eAX, Iv, XX },
  { "push{T|}",		ds, XX, XX },
  { "pop{T|}",		ds, XX, XX },
  /* 20 */
  { "andB",		Eb, Gb, XX },
  { "andS",		Ev, Gv, XX },
  { "andB",		Gb, Eb, XX },
  { "andS",		Gv, Ev, XX },
  { "andB",		AL, Ib, XX },
  { "andS",		eAX, Iv, XX },
  { "(bad)",		XX, XX, XX },	/* SEG ES prefix */
  { "daa{|}",		XX, XX, XX },
  /* 28 */
  { "subB",		Eb, Gb, XX },
  { "subS",		Ev, Gv, XX },
  { "subB",		Gb, Eb, XX },
  { "subS",		Gv, Ev, XX },
  { "subB",		AL, Ib, XX },
  { "subS",		eAX, Iv, XX },
  { "(bad)",		XX, XX, XX },	/* SEG CS prefix */
  { "das{|}",		XX, XX, XX },
  /* 30 */
  { "xorB",		Eb, Gb, XX },
  { "xorS",		Ev, Gv, XX },
  { "xorB",		Gb, Eb, XX },
  { "xorS",		Gv, Ev, XX },
  { "xorB",		AL, Ib, XX },
  { "xorS",		eAX, Iv, XX },
  { "(bad)",		XX, XX, XX },	/* SEG SS prefix */
  { "aaa{|}",		XX, XX, XX },
  /* 38 */
  { "cmpB",		Eb, Gb, XX },
  { "cmpS",		Ev, Gv, XX },
  { "cmpB",		Gb, Eb, XX },
  { "cmpS",		Gv, Ev, XX },
  { "cmpB",		AL, Ib, XX },
  { "cmpS",		eAX, Iv, XX },
  { "(bad)",		XX, XX, XX },	/* SEG DS prefix */
  { "aas{|}",		XX, XX, XX },
  /* 40 */
  { "inc{S|}",		RMeAX, XX, XX },
  { "inc{S|}",		RMeCX, XX, XX },
  { "inc{S|}",		RMeDX, XX, XX },
  { "inc{S|}",		RMeBX, XX, XX },
  { "inc{S|}",		RMeSP, XX, XX },
  { "inc{S|}",		RMeBP, XX, XX },
  { "inc{S|}",		RMeSI, XX, XX },
  { "inc{S|}",		RMeDI, XX, XX },
  /* 48 */
  { "dec{S|}",		RMeAX, XX, XX },
  { "dec{S|}",		RMeCX, XX, XX },
  { "dec{S|}",		RMeDX, XX, XX },
  { "dec{S|}",		RMeBX, XX, XX },
  { "dec{S|}",		RMeSP, XX, XX },
  { "dec{S|}",		RMeBP, XX, XX },
  { "dec{S|}",		RMeSI, XX, XX },
  { "dec{S|}",		RMeDI, XX, XX },
  /* 50 */
  { "pushS",		RMrAX, XX, XX },
  { "pushS",		RMrCX, XX, XX },
  { "pushS",		RMrDX, XX, XX },
  { "pushS",		RMrBX, XX, XX },
  { "pushS",		RMrSP, XX, XX },
  { "pushS",		RMrBP, XX, XX },
  { "pushS",		RMrSI, XX, XX },
  { "pushS",		RMrDI, XX, XX },
  /* 58 */
  { "popS",		RMrAX, XX, XX },
  { "popS",		RMrCX, XX, XX },
  { "popS",		RMrDX, XX, XX },
  { "popS",		RMrBX, XX, XX },
  { "popS",		RMrSP, XX, XX },
  { "popS",		RMrBP, XX, XX },
  { "popS",		RMrSI, XX, XX },
  { "popS",		RMrDI, XX, XX },
  /* 60 */
  { "pusha{P|}",	XX, XX, XX },
  { "popa{P|}",		XX, XX, XX },
  { "bound{S|}",	Gv, Ma, XX },
  { X86_64_0 },
  { "(bad)",		XX, XX, XX },	/* seg fs */
  { "(bad)",		XX, XX, XX },	/* seg gs */
  { "(bad)",		XX, XX, XX },	/* op size prefix */
  { "(bad)",		XX, XX, XX },	/* adr size prefix */
  /* 68 */
  { "pushT",		Iq, XX, XX },
  { "imulS",		Gv, Ev, Iv },
  { "pushT",		sIb, XX, XX },
  { "imulS",		Gv, Ev, sIb },
  { "ins{b||b|}",	Yb, indirDX, XX },
  { "ins{R||R|}",	Yv, indirDX, XX },
  { "outs{b||b|}",	indirDX, Xb, XX },
  { "outs{R||R|}",	indirDX, Xv, XX },
  /* 70 */
  { "joH",		Jb, XX, cond_jump_flag },
  { "jnoH",		Jb, XX, cond_jump_flag },
  { "jbH",		Jb, XX, cond_jump_flag },
  { "jaeH",		Jb, XX, cond_jump_flag },
  { "jeH",		Jb, XX, cond_jump_flag },
  { "jneH",		Jb, XX, cond_jump_flag },
  { "jbeH",		Jb, XX, cond_jump_flag },
  { "jaH",		Jb, XX, cond_jump_flag },
  /* 78 */
  { "jsH",		Jb, XX, cond_jump_flag },
  { "jnsH",		Jb, XX, cond_jump_flag },
  { "jpH",		Jb, XX, cond_jump_flag },
  { "jnpH",		Jb, XX, cond_jump_flag },
  { "jlH",		Jb, XX, cond_jump_flag },
  { "jgeH",		Jb, XX, cond_jump_flag },
  { "jleH",		Jb, XX, cond_jump_flag },
  { "jgH",		Jb, XX, cond_jump_flag },
  /* 80 */
  { GRP1b },
  { GRP1S },
  { "(bad)",		XX, XX, XX },
  { GRP1Ss },
  { "testB",		Eb, Gb, XX },
  { "testS",		Ev, Gv, XX },
  { "xchgB",		Eb, Gb, XX },
  { "xchgS",		Ev, Gv, XX },
  /* 88 */
  { "movB",		Eb, Gb, XX },
  { "movS",		Ev, Gv, XX },
  { "movB",		Gb, Eb, XX },
  { "movS",		Gv, Ev, XX },
  { "movQ",		Ev, Sw, XX },
  { "leaS",		Gv, M, XX },
  { "movQ",		Sw, Ev, XX },
  { "popU",		Ev, XX, XX },
  /* 90 */
  { "nop",		XX, XX, XX },
  /* FIXME: NOP with REPz prefix is called PAUSE.  */
  { "xchgS",		RMeCX, eAX, XX },
  { "xchgS",		RMeDX, eAX, XX },
  { "xchgS",		RMeBX, eAX, XX },
  { "xchgS",		RMeSP, eAX, XX },
  { "xchgS",		RMeBP, eAX, XX },
  { "xchgS",		RMeSI, eAX, XX },
  { "xchgS",		RMeDI, eAX, XX },
  /* 98 */
  { "cW{tR||tR|}",	XX, XX, XX },
  { "cR{tO||tO|}",	XX, XX, XX },
  { "lcall{T|}",	Ap, XX, XX },
  { "(bad)",		XX, XX, XX },	/* fwait */
  { "pushfT",		XX, XX, XX },
  { "popfT",		XX, XX, XX },
  { "sahf{|}",		XX, XX, XX },
  { "lahf{|}",		XX, XX, XX },
  /* a0 */
  { "movB",		AL, Ob64, XX },
  { "movS",		eAX, Ov64, XX },
  { "movB",		Ob64, AL, XX },
  { "movS",		Ov64, eAX, XX },
  { "movs{b||b|}",	Yb, Xb, XX },
  { "movs{R||R|}",	Yv, Xv, XX },
  { "cmps{b||b|}",	Xb, Yb, XX },
  { "cmps{R||R|}",	Xv, Yv, XX },
  /* a8 */
  { "testB",		AL, Ib, XX },
  { "testS",		eAX, Iv, XX },
  { "stosB",		Yb, AL, XX },
  { "stosS",		Yv, eAX, XX },
  { "lodsB",		AL, Xb, XX },
  { "lodsS",		eAX, Xv, XX },
  { "scasB",		AL, Yb, XX },
  { "scasS",		eAX, Yv, XX },
  /* b0 */
  { "movB",		RMAL, Ib, XX },
  { "movB",		RMCL, Ib, XX },
  { "movB",		RMDL, Ib, XX },
  { "movB",		RMBL, Ib, XX },
  { "movB",		RMAH, Ib, XX },
  { "movB",		RMCH, Ib, XX },
  { "movB",		RMDH, Ib, XX },
  { "movB",		RMBH, Ib, XX },
  /* b8 */
  { "movS",		RMeAX, Iv64, XX },
  { "movS",		RMeCX, Iv64, XX },
  { "movS",		RMeDX, Iv64, XX },
  { "movS",		RMeBX, Iv64, XX },
  { "movS",		RMeSP, Iv64, XX },
  { "movS",		RMeBP, Iv64, XX },
  { "movS",		RMeSI, Iv64, XX },
  { "movS",		RMeDI, Iv64, XX },
  /* c0 */
  { GRP2b },
  { GRP2S },
  { "retT",		Iw, XX, XX },
  { "retT",		XX, XX, XX },
  { "les{S|}",		Gv, Mp, XX },
  { "ldsS",		Gv, Mp, XX },
  { "movA",		Eb, Ib, XX },
  { "movQ",		Ev, Iv, XX },
  /* c8 */
  { "enterT",		Iw, Ib, XX },
  { "leaveT",		XX, XX, XX },
  { "lretP",		Iw, XX, XX },
  { "lretP",		XX, XX, XX },
  { "int3",		XX, XX, XX },
  { "int",		Ib, XX, XX },
  { "into{|}",		XX, XX, XX },
  { "iretP",		XX, XX, XX },
  /* d0 */
  { GRP2b_one },
  { GRP2S_one },
  { GRP2b_cl },
  { GRP2S_cl },
  { "aam{|}",		sIb, XX, XX },
  { "aad{|}",		sIb, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "xlat",		DSBX, XX, XX },
  /* d8 */
  { FLOAT },
  { FLOAT },
  { FLOAT },
  { FLOAT },
  { FLOAT },
  { FLOAT },
  { FLOAT },
  { FLOAT },
  /* e0 */
  { "loopneFH",		Jb, XX, loop_jcxz_flag },
  { "loopeFH",		Jb, XX, loop_jcxz_flag },
  { "loopFH",		Jb, XX, loop_jcxz_flag },
  { "jEcxzH",		Jb, XX, loop_jcxz_flag },
  { "inB",		AL, Ib, XX },
  { "inS",		eAX, Ib, XX },
  { "outB",		Ib, AL, XX },
  { "outS",		Ib, eAX, XX },
  /* e8 */
  { "callT",		Jv, XX, XX },
  { "jmpT",		Jv, XX, XX },
  { "ljmp{T|}",		Ap, XX, XX },
  { "jmp",		Jb, XX, XX },
  { "inB",		AL, indirDX, XX },
  { "inS",		eAX, indirDX, XX },
  { "outB",		indirDX, AL, XX },
  { "outS",		indirDX, eAX, XX },
  /* f0 */
  { "(bad)",		XX, XX, XX },	/* lock prefix */
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },	/* repne */
  { "(bad)",		XX, XX, XX },	/* repz */
  { "hlt",		XX, XX, XX },
  { "cmc",		XX, XX, XX },
  { GRP3b },
  { GRP3S },
  /* f8 */
  { "clc",		XX, XX, XX },
  { "stc",		XX, XX, XX },
  { "cli",		XX, XX, XX },
  { "sti",		XX, XX, XX },
  { "cld",		XX, XX, XX },
  { "std",		XX, XX, XX },
  { GRP4 },
  { GRP5 },
};

static const struct dis386 dis386_twobyte[] = {
  /* 00 */
  { GRP6 },
  { GRP7 },
  { "larS",		Gv, Ew, XX },
  { "lslS",		Gv, Ew, XX },
  { "(bad)",		XX, XX, XX },
  { "syscall",		XX, XX, XX },
  { "clts",		XX, XX, XX },
  { "sysretP",		XX, XX, XX },
  /* 08 */
  { "invd",		XX, XX, XX },
  { "wbinvd",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "ud2a",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { GRPAMD },
  { "femms",		XX, XX, XX },
  { "",			MX, EM, OPSUF }, /* See OP_3DNowSuffix.  */
  /* 10 */
  { PREGRP8 },
  { PREGRP9 },
  { "movlpX",		XM, EX, SIMD_Fixup, 'h' }, /* really only 2 operands */
  { "movlpX",		EX, XM, SIMD_Fixup, 'h' },
  { "unpcklpX",		XM, EX, XX },
  { "unpckhpX",		XM, EX, XX },
  { "movhpX",		XM, EX, SIMD_Fixup, 'l' },
  { "movhpX",		EX, XM, SIMD_Fixup, 'l' },
  /* 18 */
  { GRP14 },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  /* 20 */
  { "movL",		Rm, Cm, XX },
  { "movL",		Rm, Dm, XX },
  { "movL",		Cm, Rm, XX },
  { "movL",		Dm, Rm, XX },
  { "movL",		Rd, Td, XX },
  { "(bad)",		XX, XX, XX },
  { "movL",		Td, Rd, XX },
  { "(bad)",		XX, XX, XX },
  /* 28 */
  { "movapX",		XM, EX, XX },
  { "movapX",		EX, XM, XX },
  { PREGRP2 },
  { "movntpX",		Ev, XM, XX },
  { PREGRP4 },
  { PREGRP3 },
  { "ucomisX",		XM,EX, XX },
  { "comisX",		XM,EX, XX },
  /* 30 */
  { "wrmsr",		XX, XX, XX },
  { "rdtsc",		XX, XX, XX },
  { "rdmsr",		XX, XX, XX },
  { "rdpmc",		XX, XX, XX },
  { "sysenter",		XX, XX, XX },
  { "sysexit",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  /* 38 */
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  /* 40 */
  { "cmovo",		Gv, Ev, XX },
  { "cmovno",		Gv, Ev, XX },
  { "cmovb",		Gv, Ev, XX },
  { "cmovae",		Gv, Ev, XX },
  { "cmove",		Gv, Ev, XX },
  { "cmovne",		Gv, Ev, XX },
  { "cmovbe",		Gv, Ev, XX },
  { "cmova",		Gv, Ev, XX },
  /* 48 */
  { "cmovs",		Gv, Ev, XX },
  { "cmovns",		Gv, Ev, XX },
  { "cmovp",		Gv, Ev, XX },
  { "cmovnp",		Gv, Ev, XX },
  { "cmovl",		Gv, Ev, XX },
  { "cmovge",		Gv, Ev, XX },
  { "cmovle",		Gv, Ev, XX },
  { "cmovg",		Gv, Ev, XX },
  /* 50 */
  { "movmskpX",		Gd, XS, XX },
  { PREGRP13 },
  { PREGRP12 },
  { PREGRP11 },
  { "andpX",		XM, EX, XX },
  { "andnpX",		XM, EX, XX },
  { "orpX",		XM, EX, XX },
  { "xorpX",		XM, EX, XX },
  /* 58 */
  { PREGRP0 },
  { PREGRP10 },
  { PREGRP17 },
  { PREGRP16 },
  { PREGRP14 },
  { PREGRP7 },
  { PREGRP5 },
  { PREGRP6 },
  /* 60 */
  { "punpcklbw",	MX, EM, XX },
  { "punpcklwd",	MX, EM, XX },
  { "punpckldq",	MX, EM, XX },
  { "packsswb",		MX, EM, XX },
  { "pcmpgtb",		MX, EM, XX },
  { "pcmpgtw",		MX, EM, XX },
  { "pcmpgtd",		MX, EM, XX },
  { "packuswb",		MX, EM, XX },
  /* 68 */
  { "punpckhbw",	MX, EM, XX },
  { "punpckhwd",	MX, EM, XX },
  { "punpckhdq",	MX, EM, XX },
  { "packssdw",		MX, EM, XX },
  { PREGRP26 },
  { PREGRP24 },
  { "movd",		MX, Ed, XX },
  { PREGRP19 },
  /* 70 */
  { PREGRP22 },
  { GRP10 },
  { GRP11 },
  { GRP12 },
  { "pcmpeqb",		MX, EM, XX },
  { "pcmpeqw",		MX, EM, XX },
  { "pcmpeqd",		MX, EM, XX },
  { "emms",		XX, XX, XX },
  /* 78 */
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  { PREGRP23 },
  { PREGRP20 },
  /* 80 */
  { "joH",		Jv, XX, cond_jump_flag },
  { "jnoH",		Jv, XX, cond_jump_flag },
  { "jbH",		Jv, XX, cond_jump_flag },
  { "jaeH",		Jv, XX, cond_jump_flag },
  { "jeH",		Jv, XX, cond_jump_flag },
  { "jneH",		Jv, XX, cond_jump_flag },
  { "jbeH",		Jv, XX, cond_jump_flag },
  { "jaH",		Jv, XX, cond_jump_flag },
  /* 88 */
  { "jsH",		Jv, XX, cond_jump_flag },
  { "jnsH",		Jv, XX, cond_jump_flag },
  { "jpH",		Jv, XX, cond_jump_flag },
  { "jnpH",		Jv, XX, cond_jump_flag },
  { "jlH",		Jv, XX, cond_jump_flag },
  { "jgeH",		Jv, XX, cond_jump_flag },
  { "jleH",		Jv, XX, cond_jump_flag },
  { "jgH",		Jv, XX, cond_jump_flag },
  /* 90 */
  { "seto",		Eb, XX, XX },
  { "setno",		Eb, XX, XX },
  { "setb",		Eb, XX, XX },
  { "setae",		Eb, XX, XX },
  { "sete",		Eb, XX, XX },
  { "setne",		Eb, XX, XX },
  { "setbe",		Eb, XX, XX },
  { "seta",		Eb, XX, XX },
  /* 98 */
  { "sets",		Eb, XX, XX },
  { "setns",		Eb, XX, XX },
  { "setp",		Eb, XX, XX },
  { "setnp",		Eb, XX, XX },
  { "setl",		Eb, XX, XX },
  { "setge",		Eb, XX, XX },
  { "setle",		Eb, XX, XX },
  { "setg",		Eb, XX, XX },
  /* a0 */
  { "pushT",		fs, XX, XX },
  { "popT",		fs, XX, XX },
  { "cpuid",		XX, XX, XX },
  { "btS",		Ev, Gv, XX },
  { "shldS",		Ev, Gv, Ib },
  { "shldS",		Ev, Gv, CL },
  { "(bad)",		XX, XX, XX },
  { "(bad)",		XX, XX, XX },
  /* a8 */
  { "pushT",		gs, XX, XX },
  { "popT",		gs, XX, XX },
  { "rsm",		XX, XX, XX },
  { "btsS",		Ev, Gv, XX },
  { "shrdS",		Ev, Gv, Ib },
  { "shrdS",		Ev, Gv, CL },
  { GRP13 },
  { "imulS",		Gv, Ev, XX },
  /* b0 */
  { "cmpxchgB",		Eb, Gb, XX },
  { "cmpxchgS",		Ev, Gv, XX },
  { "lssS",		Gv, Mp, XX },
  { "btrS",		Ev, Gv, XX },
  { "lfsS",		Gv, Mp, XX },
  { "lgsS",		Gv, Mp, XX },
  { "movz{bR|x|bR|x}",	Gv, Eb, XX },
  { "movz{wR|x|wR|x}",	Gv, Ew, XX }, /* yes, there really is movzww ! */
  /* b8 */
  { "(bad)",		XX, XX, XX },
  { "ud2b",		XX, XX, XX },
  { GRP8 },
  { "btcS",		Ev, Gv, XX },
  { "bsfS",		Gv, Ev, XX },
  { "bsrS",		Gv, Ev, XX },
  { "movs{bR|x|bR|x}",	Gv, Eb, XX },
  { "movs{wR|x|wR|x}",	Gv, Ew, XX }, /* yes, there really is movsww ! */
  /* c0 */
  { "xaddB",		Eb, Gb, XX },
  { "xaddS",		Ev, Gv, XX },
  { PREGRP1 },
  { "movntiS",		Ev, Gv, XX },
  { "pinsrw",		MX, Ed, Ib },
  { "pextrw",		Gd, MS, Ib },
  { "shufpX",		XM, EX, Ib },
  { GRP9 },
  /* c8 */
  { "bswap",		RMeAX, XX, XX },
  { "bswap",		RMeCX, XX, XX },
  { "bswap",		RMeDX, XX, XX },
  { "bswap",		RMeBX, XX, XX },
  { "bswap",		RMeSP, XX, XX },
  { "bswap",		RMeBP, XX, XX },
  { "bswap",		RMeSI, XX, XX },
  { "bswap",		RMeDI, XX, XX },
  /* d0 */
  { "(bad)",		XX, XX, XX },
  { "psrlw",		MX, EM, XX },
  { "psrld",		MX, EM, XX },
  { "psrlq",		MX, EM, XX },
  { "paddq",		MX, EM, XX },
  { "pmullw",		MX, EM, XX },
  { PREGRP21 },
  { "pmovmskb",		Gd, MS, XX },
  /* d8 */
  { "psubusb",		MX, EM, XX },
  { "psubusw",		MX, EM, XX },
  { "pminub",		MX, EM, XX },
  { "pand",		MX, EM, XX },
  { "paddusb",		MX, EM, XX },
  { "paddusw",		MX, EM, XX },
  { "pmaxub",		MX, EM, XX },
  { "pandn",		MX, EM, XX },
  /* e0 */
  { "pavgb",		MX, EM, XX },
  { "psraw",		MX, EM, XX },
  { "psrad",		MX, EM, XX },
  { "pavgw",		MX, EM, XX },
  { "pmulhuw",		MX, EM, XX },
  { "pmulhw",		MX, EM, XX },
  { PREGRP15 },
  { PREGRP25 },
  /* e8 */
  { "psubsb",		MX, EM, XX },
  { "psubsw",		MX, EM, XX },
  { "pminsw",		MX, EM, XX },
  { "por",		MX, EM, XX },
  { "paddsb",		MX, EM, XX },
  { "paddsw",		MX, EM, XX },
  { "pmaxsw",		MX, EM, XX },
  { "pxor",		MX, EM, XX },
  /* f0 */
  { "(bad)",		XX, XX, XX },
  { "psllw",		MX, EM, XX },
  { "pslld",		MX, EM, XX },
  { "psllq",		MX, EM, XX },
  { "pmuludq",		MX, EM, XX },
  { "pmaddwd",		MX, EM, XX },
  { "psadbw",		MX, EM, XX },
  { PREGRP18 },
  /* f8 */
  { "psubb",		MX, EM, XX },
  { "psubw",		MX, EM, XX },
  { "psubd",		MX, EM, XX },
  { "psubq",		MX, EM, XX },
  { "paddb",		MX, EM, XX },
  { "paddw",		MX, EM, XX },
  { "paddd",		MX, EM, XX },
  { "(bad)",		XX, XX, XX }
};

static const unsigned char onebyte_has_modrm[256] = {
  /*       0 1 2 3 4 5 6 7 8 9 a b c d e f        */
  /*       -------------------------------        */
  /* 00 */ 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0, /* 00 */
  /* 10 */ 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0, /* 10 */
  /* 20 */ 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0, /* 20 */
  /* 30 */ 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0, /* 30 */
  /* 40 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 40 */
  /* 50 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 50 */
  /* 60 */ 0,0,1,1,0,0,0,0,0,1,0,1,0,0,0,0, /* 60 */
  /* 70 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 70 */
  /* 80 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 80 */
  /* 90 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 90 */
  /* a0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* a0 */
  /* b0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* b0 */
  /* c0 */ 1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0, /* c0 */
  /* d0 */ 1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1, /* d0 */
  /* e0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* e0 */
  /* f0 */ 0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1  /* f0 */
  /*       -------------------------------        */
  /*       0 1 2 3 4 5 6 7 8 9 a b c d e f        */
};

static const unsigned char twobyte_has_modrm[256] = {
  /*       0 1 2 3 4 5 6 7 8 9 a b c d e f        */
  /*       -------------------------------        */
  /* 00 */ 1,1,1,1,0,0,0,0,0,0,0,0,0,1,0,1, /* 0f */
  /* 10 */ 1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0, /* 1f */
  /* 20 */ 1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1, /* 2f */
  /* 30 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 3f */
  /* 40 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 4f */
  /* 50 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 5f */
  /* 60 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 6f */
  /* 70 */ 1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1, /* 7f */
  /* 80 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 8f */
  /* 90 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 9f */
  /* a0 */ 0,0,0,1,1,1,0,0,0,0,0,1,1,1,1,1, /* af */
  /* b0 */ 1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1, /* bf */
  /* c0 */ 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0, /* cf */
  /* d0 */ 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* df */
  /* e0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* ef */
  /* f0 */ 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0  /* ff */
  /*       -------------------------------        */
  /*       0 1 2 3 4 5 6 7 8 9 a b c d e f        */
};

static const unsigned char twobyte_uses_SSE_prefix[256] = {
  /*       0 1 2 3 4 5 6 7 8 9 a b c d e f        */
  /*       -------------------------------        */
  /* 00 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0f */
  /* 10 */ 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 1f */
  /* 20 */ 0,0,0,0,0,0,0,0,0,0,1,0,1,1,0,0, /* 2f */
  /* 30 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 3f */
  /* 40 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 4f */
  /* 50 */ 0,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1, /* 5f */
  /* 60 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1, /* 6f */
  /* 70 */ 1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1, /* 7f */
  /* 80 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 8f */
  /* 90 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 9f */
  /* a0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* af */
  /* b0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* bf */
  /* c0 */ 0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0, /* cf */
  /* d0 */ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0, /* df */
  /* e0 */ 0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0, /* ef */
  /* f0 */ 0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0  /* ff */
  /*       -------------------------------        */
  /*       0 1 2 3 4 5 6 7 8 9 a b c d e f        */
};

static char obuf[100];
static char *obufp;
static char scratchbuf[100];
static unsigned char *start_codep;
static unsigned char *insn_codep;
static unsigned char *codep;
static disassemble_info *the_info;
static int mod;
static int rm;
static int reg;
static unsigned char need_modrm;

/* If we are accessing mod/rm/reg without need_modrm set, then the
   values are stale.  Hitting this abort likely indicates that you
   need to update onebyte_has_modrm or twobyte_has_modrm.  */
#define MODRM_CHECK  if (!need_modrm) abort ()

static const char **names64;
static const char **names32;
static const char **names16;
static const char **names8;
static const char **names8rex;
static const char **names_seg;
static const char **index16;

static const char *intel_names64[] = {
  "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};
static const char *intel_names32[] = {
  "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
  "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"
};
static const char *intel_names16[] = {
  "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
  "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"
};
static const char *intel_names8[] = {
  "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
};
static const char *intel_names8rex[] = {
  "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil",
  "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"
};
static const char *intel_names_seg[] = {
  "es", "cs", "ss", "ds", "fs", "gs", "?", "?",
};
static const char *intel_index16[] = {
  "bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx"
};

static const char *att_names64[] = {
  "%rax", "%rcx", "%rdx", "%rbx", "%rsp", "%rbp", "%rsi", "%rdi",
  "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"
};
static const char *att_names32[] = {
  "%eax", "%ecx", "%edx", "%ebx", "%esp", "%ebp", "%esi", "%edi",
  "%r8d", "%r9d", "%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d"
};
static const char *att_names16[] = {
  "%ax", "%cx", "%dx", "%bx", "%sp", "%bp", "%si", "%di",
  "%r8w", "%r9w", "%r10w", "%r11w", "%r12w", "%r13w", "%r14w", "%r15w"
};
static const char *att_names8[] = {
  "%al", "%cl", "%dl", "%bl", "%ah", "%ch", "%dh", "%bh",
};
static const char *att_names8rex[] = {
  "%al", "%cl", "%dl", "%bl", "%spl", "%bpl", "%sil", "%dil",
  "%r8b", "%r9b", "%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b"
};
static const char *att_names_seg[] = {
  "%es", "%cs", "%ss", "%ds", "%fs", "%gs", "%?", "%?",
};
static const char *att_index16[] = {
  "%bx,%si", "%bx,%di", "%bp,%si", "%bp,%di", "%si", "%di", "%bp", "%bx"
};

static const struct dis386 grps[][8] = {
  /* GRP1b */
  {
    { "addA",	Eb, Ib, XX },
    { "orA",	Eb, Ib, XX },
    { "adcA",	Eb, Ib, XX },
    { "sbbA",	Eb, Ib, XX },
    { "andA",	Eb, Ib, XX },
    { "subA",	Eb, Ib, XX },
    { "xorA",	Eb, Ib, XX },
    { "cmpA",	Eb, Ib, XX }
  },
  /* GRP1S */
  {
    { "addQ",	Ev, Iv, XX },
    { "orQ",	Ev, Iv, XX },
    { "adcQ",	Ev, Iv, XX },
    { "sbbQ",	Ev, Iv, XX },
    { "andQ",	Ev, Iv, XX },
    { "subQ",	Ev, Iv, XX },
    { "xorQ",	Ev, Iv, XX },
    { "cmpQ",	Ev, Iv, XX }
  },
  /* GRP1Ss */
  {
    { "addQ",	Ev, sIb, XX },
    { "orQ",	Ev, sIb, XX },
    { "adcQ",	Ev, sIb, XX },
    { "sbbQ",	Ev, sIb, XX },
    { "andQ",	Ev, sIb, XX },
    { "subQ",	Ev, sIb, XX },
    { "xorQ",	Ev, sIb, XX },
    { "cmpQ",	Ev, sIb, XX }
  },
  /* GRP2b */
  {
    { "rolA",	Eb, Ib, XX },
    { "rorA",	Eb, Ib, XX },
    { "rclA",	Eb, Ib, XX },
    { "rcrA",	Eb, Ib, XX },
    { "shlA",	Eb, Ib, XX },
    { "shrA",	Eb, Ib, XX },
    { "(bad)",	XX, XX, XX },
    { "sarA",	Eb, Ib, XX },
  },
  /* GRP2S */
  {
    { "rolQ",	Ev, Ib, XX },
    { "rorQ",	Ev, Ib, XX },
    { "rclQ",	Ev, Ib, XX },
    { "rcrQ",	Ev, Ib, XX },
    { "shlQ",	Ev, Ib, XX },
    { "shrQ",	Ev, Ib, XX },
    { "(bad)",	XX, XX, XX },
    { "sarQ",	Ev, Ib, XX },
  },
  /* GRP2b_one */
  {
    { "rolA",	Eb, XX, XX },
    { "rorA",	Eb, XX, XX },
    { "rclA",	Eb, XX, XX },
    { "rcrA",	Eb, XX, XX },
    { "shlA",	Eb, XX, XX },
    { "shrA",	Eb, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "sarA",	Eb, XX, XX },
  },
  /* GRP2S_one */
  {
    { "rolQ",	Ev, XX, XX },
    { "rorQ",	Ev, XX, XX },
    { "rclQ",	Ev, XX, XX },
    { "rcrQ",	Ev, XX, XX },
    { "shlQ",	Ev, XX, XX },
    { "shrQ",	Ev, XX, XX },
    { "(bad)",	XX, XX, XX},
    { "sarQ",	Ev, XX, XX },
  },
  /* GRP2b_cl */
  {
    { "rolA",	Eb, CL, XX },
    { "rorA",	Eb, CL, XX },
    { "rclA",	Eb, CL, XX },
    { "rcrA",	Eb, CL, XX },
    { "shlA",	Eb, CL, XX },
    { "shrA",	Eb, CL, XX },
    { "(bad)",	XX, XX, XX },
    { "sarA",	Eb, CL, XX },
  },
  /* GRP2S_cl */
  {
    { "rolQ",	Ev, CL, XX },
    { "rorQ",	Ev, CL, XX },
    { "rclQ",	Ev, CL, XX },
    { "rcrQ",	Ev, CL, XX },
    { "shlQ",	Ev, CL, XX },
    { "shrQ",	Ev, CL, XX },
    { "(bad)",	XX, XX, XX },
    { "sarQ",	Ev, CL, XX }
  },
  /* GRP3b */
  {
    { "testA",	Eb, Ib, XX },
    { "(bad)",	Eb, XX, XX },
    { "notA",	Eb, XX, XX },
    { "negA",	Eb, XX, XX },
    { "mulA",	Eb, XX, XX },	/* Don't print the implicit %al register,  */
    { "imulA",	Eb, XX, XX },	/* to distinguish these opcodes from other */
    { "divA",	Eb, XX, XX },	/* mul/imul opcodes.  Do the same for div  */
    { "idivA",	Eb, XX, XX }	/* and idiv for consistency.		   */
  },
  /* GRP3S */
  {
    { "testQ",	Ev, Iv, XX },
    { "(bad)",	XX, XX, XX },
    { "notQ",	Ev, XX, XX },
    { "negQ",	Ev, XX, XX },
    { "mulQ",	Ev, XX, XX },	/* Don't print the implicit register.  */
    { "imulQ",	Ev, XX, XX },
    { "divQ",	Ev, XX, XX },
    { "idivQ",	Ev, XX, XX },
  },
  /* GRP4 */
  {
    { "incA",	Eb, XX, XX },
    { "decA",	Eb, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
  },
  /* GRP5 */
  {
    { "incQ",	Ev, XX, XX },
    { "decQ",	Ev, XX, XX },
    { "callT",	indirEv, XX, XX },
    { "lcallT",	indirEv, XX, XX },
    { "jmpT",	indirEv, XX, XX },
    { "ljmpT",	indirEv, XX, XX },
    { "pushU",	Ev, XX, XX },
    { "(bad)",	XX, XX, XX },
  },
  /* GRP6 */
  {
    { "sldtQ",	Ev, XX, XX },
    { "strQ",	Ev, XX, XX },
    { "lldt",	Ew, XX, XX },
    { "ltr",	Ew, XX, XX },
    { "verr",	Ew, XX, XX },
    { "verw",	Ew, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX }
  },
  /* GRP7 */
  {
    { "sgdtQ",	 M, XX, XX },
    { "sidtQ",	 M, XX, XX },
    { "lgdtQ",	 M, XX, XX },
    { "lidtQ",	 M, XX, XX },
    { "smswQ",	Ev, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "lmsw",	Ew, XX, XX },
    { "invlpg",	Ew, XX, XX },
  },
  /* GRP8 */
  {
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "btQ",	Ev, Ib, XX },
    { "btsQ",	Ev, Ib, XX },
    { "btrQ",	Ev, Ib, XX },
    { "btcQ",	Ev, Ib, XX },
  },
  /* GRP9 */
  {
    { "(bad)",	XX, XX, XX },
    { "cmpxchg8b", Ev, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
  },
  /* GRP10 */
  {
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "psrlw",	MS, Ib, XX },
    { "(bad)",	XX, XX, XX },
    { "psraw",	MS, Ib, XX },
    { "(bad)",	XX, XX, XX },
    { "psllw",	MS, Ib, XX },
    { "(bad)",	XX, XX, XX },
  },
  /* GRP11 */
  {
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "psrld",	MS, Ib, XX },
    { "(bad)",	XX, XX, XX },
    { "psrad",	MS, Ib, XX },
    { "(bad)",	XX, XX, XX },
    { "pslld",	MS, Ib, XX },
    { "(bad)",	XX, XX, XX },
  },
  /* GRP12 */
  {
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "psrlq",	MS, Ib, XX },
    { "psrldq",	MS, Ib, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "psllq",	MS, Ib, XX },
    { "pslldq",	MS, Ib, XX },
  },
  /* GRP13 */
  {
    { "fxsave", Ev, XX, XX },
    { "fxrstor", Ev, XX, XX },
    { "ldmxcsr", Ev, XX, XX },
    { "stmxcsr", Ev, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "lfence", None, XX, XX },
    { "mfence", None, XX, XX },
    { "sfence", None, XX, XX },
    /* FIXME: the sfence with memory operand is clflush!  */
  },
  /* GRP14 */
  {
    { "prefetchnta", Ev, XX, XX },
    { "prefetcht0", Ev, XX, XX },
    { "prefetcht1", Ev, XX, XX },
    { "prefetcht2", Ev, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
  },
  /* GRPAMD */
  {
    { "prefetch", Eb, XX, XX },
    { "prefetchw", Eb, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
  }
};

static const struct dis386 prefix_user_table[][4] = {
  /* PREGRP0 */
  {
    { "addps", XM, EX, XX },
    { "addss", XM, EX, XX },
    { "addpd", XM, EX, XX },
    { "addsd", XM, EX, XX },
  },
  /* PREGRP1 */
  {
    { "", XM, EX, OPSIMD },	/* See OP_SIMD_SUFFIX.  */
    { "", XM, EX, OPSIMD },
    { "", XM, EX, OPSIMD },
    { "", XM, EX, OPSIMD },
  },
  /* PREGRP2 */
  {
    { "cvtpi2ps", XM, EM, XX },
    { "cvtsi2ssY", XM, Ev, XX },
    { "cvtpi2pd", XM, EM, XX },
    { "cvtsi2sdY", XM, Ev, XX },
  },
  /* PREGRP3 */
  {
    { "cvtps2pi", MX, EX, XX },
    { "cvtss2siY", Gv, EX, XX },
    { "cvtpd2pi", MX, EX, XX },
    { "cvtsd2siY", Gv, EX, XX },
  },
  /* PREGRP4 */
  {
    { "cvttps2pi", MX, EX, XX },
    { "cvttss2siY", Gv, EX, XX },
    { "cvttpd2pi", MX, EX, XX },
    { "cvttsd2siY", Gv, EX, XX },
  },
  /* PREGRP5 */
  {
    { "divps", XM, EX, XX },
    { "divss", XM, EX, XX },
    { "divpd", XM, EX, XX },
    { "divsd", XM, EX, XX },
  },
  /* PREGRP6 */
  {
    { "maxps", XM, EX, XX },
    { "maxss", XM, EX, XX },
    { "maxpd", XM, EX, XX },
    { "maxsd", XM, EX, XX },
  },
  /* PREGRP7 */
  {
    { "minps", XM, EX, XX },
    { "minss", XM, EX, XX },
    { "minpd", XM, EX, XX },
    { "minsd", XM, EX, XX },
  },
  /* PREGRP8 */
  {
    { "movups", XM, EX, XX },
    { "movss", XM, EX, XX },
    { "movupd", XM, EX, XX },
    { "movsd", XM, EX, XX },
  },
  /* PREGRP9 */
  {
    { "movups", EX, XM, XX },
    { "movss", EX, XM, XX },
    { "movupd", EX, XM, XX },
    { "movsd", EX, XM, XX },
  },
  /* PREGRP10 */
  {
    { "mulps", XM, EX, XX },
    { "mulss", XM, EX, XX },
    { "mulpd", XM, EX, XX },
    { "mulsd", XM, EX, XX },
  },
  /* PREGRP11 */
  {
    { "rcpps", XM, EX, XX },
    { "rcpss", XM, EX, XX },
    { "(bad)", XM, EX, XX },
    { "(bad)", XM, EX, XX },
  },
  /* PREGRP12 */
  {
    { "rsqrtps", XM, EX, XX },
    { "rsqrtss", XM, EX, XX },
    { "(bad)", XM, EX, XX },
    { "(bad)", XM, EX, XX },
  },
  /* PREGRP13 */
  {
    { "sqrtps", XM, EX, XX },
    { "sqrtss", XM, EX, XX },
    { "sqrtpd", XM, EX, XX },
    { "sqrtsd", XM, EX, XX },
  },
  /* PREGRP14 */
  {
    { "subps", XM, EX, XX },
    { "subss", XM, EX, XX },
    { "subpd", XM, EX, XX },
    { "subsd", XM, EX, XX },
  },
  /* PREGRP15 */
  {
    { "(bad)", XM, EX, XX },
    { "cvtdq2pd", XM, EX, XX },
    { "cvttpd2dq", XM, EX, XX },
    { "cvtpd2dq", XM, EX, XX },
  },
  /* PREGRP16 */
  {
    { "cvtdq2ps", XM, EX, XX },
    { "cvttps2dq",XM, EX, XX },
    { "cvtps2dq",XM, EX, XX },
    { "(bad)", XM, EX, XX },
  },
  /* PREGRP17 */
  {
    { "cvtps2pd", XM, EX, XX },
    { "cvtss2sd", XM, EX, XX },
    { "cvtpd2ps", XM, EX, XX },
    { "cvtsd2ss", XM, EX, XX },
  },
  /* PREGRP18 */
  {
    { "maskmovq", MX, MS, XX },
    { "(bad)", XM, EX, XX },
    { "maskmovdqu", XM, EX, XX },
    { "(bad)", XM, EX, XX },
  },
  /* PREGRP19 */
  {
    { "movq", MX, EM, XX },
    { "movdqu", XM, EX, XX },
    { "movdqa", XM, EX, XX },
    { "(bad)", XM, EX, XX },
  },
  /* PREGRP20 */
  {
    { "movq", EM, MX, XX },
    { "movdqu", EX, XM, XX },
    { "movdqa", EX, XM, XX },
    { "(bad)", EX, XM, XX },
  },
  /* PREGRP21 */
  {
    { "(bad)", EX, XM, XX },
    { "movq2dq", XM, MS, XX },
    { "movq", EX, XM, XX },
    { "movdq2q", MX, XS, XX },
  },
  /* PREGRP22 */
  {
    { "pshufw", MX, EM, Ib },
    { "pshufhw", XM, EX, Ib },
    { "pshufd", XM, EX, Ib },
    { "pshuflw", XM, EX, Ib },
  },
  /* PREGRP23 */
  {
    { "movd", Ed, MX, XX },
    { "movq", XM, EX, XX },
    { "movd", Ed, XM, XX },
    { "(bad)", Ed, XM, XX },
  },
  /* PREGRP24 */
  {
    { "(bad)", MX, EX, XX },
    { "(bad)", XM, EX, XX },
    { "punpckhqdq", XM, EX, XX },
    { "(bad)", XM, EX, XX },
  },
  /* PREGRP25 */
  {
  { "movntq", Ev, MX, XX },
  { "(bad)", Ev, XM, XX },
  { "movntdq", Ev, XM, XX },
  { "(bad)", Ev, XM, XX },
  },
  /* PREGRP26 */
  {
    { "(bad)", MX, EX, XX },
    { "(bad)", XM, EX, XX },
    { "punpcklqdq", XM, EX, XX },
    { "(bad)", XM, EX, XX },
  },
};

static const struct dis386 x86_64_table[][2] = {
  {
    { "arpl", Ew, Gw, XX },
    { "movs{||lq|xd}", Gv, Ed, XX },
  },
};

#define INTERNAL_DISASSEMBLER_ERROR _("<internal disassembler error>")

static void
ckprefix ()
{
  int newrex;
  rex = 0;
  prefixes = 0;
  used_prefixes = 0;
  rex_used = 0;
  while (1)
    {
      FETCH_DATA (the_info, codep + 1);
      newrex = 0;
      switch (*codep)
	{
	/* REX prefixes family.  */
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47:
	case 0x48:
	case 0x49:
	case 0x4a:
	case 0x4b:
	case 0x4c:
	case 0x4d:
	case 0x4e:
	case 0x4f:
	    if (mode_64bit)
	      newrex = *codep;
	    else
	      return;
	  break;
	case 0xf3:
	  prefixes |= PREFIX_REPZ;
	  break;
	case 0xf2:
	  prefixes |= PREFIX_REPNZ;
	  break;
	case 0xf0:
	  prefixes |= PREFIX_LOCK;
	  break;
	case 0x2e:
	  prefixes |= PREFIX_CS;
	  break;
	case 0x36:
	  prefixes |= PREFIX_SS;
	  break;
	case 0x3e:
	  prefixes |= PREFIX_DS;
	  break;
	case 0x26:
	  prefixes |= PREFIX_ES;
	  break;
	case 0x64:
	  prefixes |= PREFIX_FS;
	  break;
	case 0x65:
	  prefixes |= PREFIX_GS;
	  break;
	case 0x66:
	  prefixes |= PREFIX_DATA;
	  break;
	case 0x67:
	  prefixes |= PREFIX_ADDR;
	  break;
	case FWAIT_OPCODE:
	  /* fwait is really an instruction.  If there are prefixes
	     before the fwait, they belong to the fwait, *not* to the
	     following instruction.  */
	  if (prefixes)
	    {
	      prefixes |= PREFIX_FWAIT;
	      codep++;
	      return;
	    }
	  prefixes = PREFIX_FWAIT;
	  break;
	default:
	  return;
	}
      /* Rex is ignored when followed by another prefix.  */
      if (rex)
	{
	  oappend (prefix_name (rex, 0));
	  oappend (" ");
	}
      rex = newrex;
      codep++;
    }
}

/* Return the name of the prefix byte PREF, or NULL if PREF is not a
   prefix byte.  */

static const char *
prefix_name (pref, sizeflag)
     int pref;
     int sizeflag;
{
  switch (pref)
    {
    /* REX prefixes family.  */
    case 0x40:
      return "rex";
    case 0x41:
      return "rexZ";
    case 0x42:
      return "rexY";
    case 0x43:
      return "rexYZ";
    case 0x44:
      return "rexX";
    case 0x45:
      return "rexXZ";
    case 0x46:
      return "rexXY";
    case 0x47:
      return "rexXYZ";
    case 0x48:
      return "rex64";
    case 0x49:
      return "rex64Z";
    case 0x4a:
      return "rex64Y";
    case 0x4b:
      return "rex64YZ";
    case 0x4c:
      return "rex64X";
    case 0x4d:
      return "rex64XZ";
    case 0x4e:
      return "rex64XY";
    case 0x4f:
      return "rex64XYZ";
    case 0xf3:
      return "repz";
    case 0xf2:
      return "repnz";
    case 0xf0:
      return "lock";
    case 0x2e:
      return "cs";
    case 0x36:
      return "ss";
    case 0x3e:
      return "ds";
    case 0x26:
      return "es";
    case 0x64:
      return "fs";
    case 0x65:
      return "gs";
    case 0x66:
      return (sizeflag & DFLAG) ? "data16" : "data32";
    case 0x67:
      if (mode_64bit)
        return (sizeflag & AFLAG) ? "addr32" : "addr64";
      else
        return ((sizeflag & AFLAG) && !mode_64bit) ? "addr16" : "addr32";
    case FWAIT_OPCODE:
      return "fwait";
    default:
      return NULL;
    }
}

static char op1out[100], op2out[100], op3out[100];
static int op_ad, op_index[3];
static bfd_vma op_address[3];
static bfd_vma op_riprel[3];
static bfd_vma start_pc;

/*
 *   On the 386's of 1988, the maximum length of an instruction is 15 bytes.
 *   (see topic "Redundant prefixes" in the "Differences from 8086"
 *   section of the "Virtual 8086 Mode" chapter.)
 * 'pc' should be the address of this instruction, it will
 *   be used to print the target address if this is a relative jump or call
 * The function returns the length of this instruction in bytes.
 */

static char intel_syntax;
static char open_char;
static char close_char;
static char separator_char;
static char scale_char;

/* Here for backwards compatibility.  When gdb stops using
   print_insn_i386_att and print_insn_i386_intel these functions can
   disappear, and print_insn_i386 be merged into print_insn.  */
int
print_insn_i386_att (pc, info)
     bfd_vma pc;
     disassemble_info *info;
{
  intel_syntax = 0;

  return print_insn (pc, info);
}

int
print_insn_i386_intel (pc, info)
     bfd_vma pc;
     disassemble_info *info;
{
  intel_syntax = 1;

  return print_insn (pc, info);
}

int
print_insn_i386 (pc, info)
     bfd_vma pc;
     disassemble_info *info;
{
  intel_syntax = -1;

  return print_insn (pc, info);
}

static int
print_insn (pc, info)
     bfd_vma pc;
     disassemble_info *info;
{
  const struct dis386 *dp;
  int i;
  int two_source_ops;
  char *first, *second, *third;
  int needcomma;
  unsigned char uses_SSE_prefix;
  int sizeflag;
  /*const char *p;*/
  struct dis_private priv;

  mode_64bit = (info->mach == bfd_mach_x86_64_intel_syntax
		|| info->mach == bfd_mach_x86_64);

  if (intel_syntax == -1)
    intel_syntax = (info->mach == bfd_mach_i386_i386_intel_syntax
		    || info->mach == bfd_mach_x86_64_intel_syntax);

  if (info->mach == bfd_mach_i386_i386
      || info->mach == bfd_mach_x86_64
      || info->mach == bfd_mach_i386_i386_intel_syntax
      || info->mach == bfd_mach_x86_64_intel_syntax)
    priv.orig_sizeflag = AFLAG | DFLAG;
  else if (info->mach == bfd_mach_i386_i8086)
    priv.orig_sizeflag = 0;
  else
    abort ();

#if 0
  for (p = info->disassembler_options; p != NULL; )
    {
      if (strncmp (p, "x86-64", 6) == 0)
	{
	  mode_64bit = 1;
	  priv.orig_sizeflag = AFLAG | DFLAG;
	}
      else if (strncmp (p, "i386", 4) == 0)
	{
	  mode_64bit = 0;
	  priv.orig_sizeflag = AFLAG | DFLAG;
	}
      else if (strncmp (p, "i8086", 5) == 0)
	{
	  mode_64bit = 0;
	  priv.orig_sizeflag = 0;
	}
      else if (strncmp (p, "intel", 5) == 0)
	{
	  intel_syntax = 1;
	}
      else if (strncmp (p, "att", 3) == 0)
	{
	  intel_syntax = 0;
	}
      else if (strncmp (p, "addr", 4) == 0)
	{
	  if (p[4] == '1' && p[5] == '6')
	    priv.orig_sizeflag &= ~AFLAG;
	  else if (p[4] == '3' && p[5] == '2')
	    priv.orig_sizeflag |= AFLAG;
	}
      else if (strncmp (p, "data", 4) == 0)
	{
	  if (p[4] == '1' && p[5] == '6')
	    priv.orig_sizeflag &= ~DFLAG;
	  else if (p[4] == '3' && p[5] == '2')
	    priv.orig_sizeflag |= DFLAG;
	}
      else if (strncmp (p, "suffix", 6) == 0)
	priv.orig_sizeflag |= SUFFIX_ALWAYS;

      p = strchr (p, ',');
      if (p != NULL)
	p++;
    }
#else
  mode_64bit = 0;
  priv.orig_sizeflag = AFLAG | DFLAG;
  intel_syntax = 0;
#endif

  if (intel_syntax)
    {
      names64 = intel_names64;
      names32 = intel_names32;
      names16 = intel_names16;
      names8 = intel_names8;
      names8rex = intel_names8rex;
      names_seg = intel_names_seg;
      index16 = intel_index16;
      open_char = '[';
      close_char = ']';
      separator_char = '+';
      scale_char = '*';
    }
  else
    {
      names64 = att_names64;
      names32 = att_names32;
      names16 = att_names16;
      names8 = att_names8;
      names8rex = att_names8rex;
      names_seg = att_names_seg;
      index16 = att_index16;
      open_char = '(';
      close_char =  ')';
      separator_char = ',';
      scale_char = ',';
    }

  /* The output looks better if we put 7 bytes on a line, since that
     puts most long word instructions on a single line.  */
  info->bytes_per_line = 7;

  info->private_data = (PTR) &priv;
  priv.max_fetched = priv.the_buffer;
  priv.insn_start = pc;

  obuf[0] = 0;
  op1out[0] = 0;
  op2out[0] = 0;
  op3out[0] = 0;

  op_index[0] = op_index[1] = op_index[2] = -1;

  the_info = info;
  start_pc = pc;
  start_codep = priv.the_buffer;
  codep = priv.the_buffer;

  if (setjmp (priv.bailout) != 0)
    {
      const char *name;

      /* Getting here means we tried for data but didn't get it.  That
	 means we have an incomplete instruction of some sort.  Just
	 print the first byte as a prefix or a .byte pseudo-op.  */
      if (codep > priv.the_buffer)
	{
	  name = prefix_name (priv.the_buffer[0], priv.orig_sizeflag);
	  if (name != NULL)
	    (*info->fprintf_func) (info->stream, "%s", name);
	  else
	    {
	      /* Just print the first byte as a .byte instruction.  */
	      (*info->fprintf_func) (info->stream, ".byte 0x%x",
				     (unsigned int) priv.the_buffer[0]);
	    }

	  return 1;
	}

      return -1;
    }

  obufp = obuf;
  ckprefix ();

  insn_codep = codep;
  sizeflag = priv.orig_sizeflag;

  FETCH_DATA (info, codep + 1);
  two_source_ops = (*codep == 0x62) || (*codep == 0xc8);

  if ((prefixes & PREFIX_FWAIT)
      && ((*codep < 0xd8) || (*codep > 0xdf)))
    {
      const char *name;

      /* fwait not followed by floating point instruction.  Print the
         first prefix, which is probably fwait itself.  */
      name = prefix_name (priv.the_buffer[0], priv.orig_sizeflag);
      if (name == NULL)
	name = INTERNAL_DISASSEMBLER_ERROR;
      (*info->fprintf_func) (info->stream, "%s", name);
      return 1;
    }

  if (*codep == 0x0f)
    {
      FETCH_DATA (info, codep + 2);
      dp = &dis386_twobyte[*++codep];
      need_modrm = twobyte_has_modrm[*codep];
      uses_SSE_prefix = twobyte_uses_SSE_prefix[*codep];
    }
  else
    {
      dp = &dis386[*codep];
      need_modrm = onebyte_has_modrm[*codep];
      uses_SSE_prefix = 0;
    }
  codep++;

  if (!uses_SSE_prefix && (prefixes & PREFIX_REPZ))
    {
      oappend ("repz ");
      used_prefixes |= PREFIX_REPZ;
    }
  if (!uses_SSE_prefix && (prefixes & PREFIX_REPNZ))
    {
      oappend ("repnz ");
      used_prefixes |= PREFIX_REPNZ;
    }
  if (prefixes & PREFIX_LOCK)
    {
      oappend ("lock ");
      used_prefixes |= PREFIX_LOCK;
    }

  if (prefixes & PREFIX_ADDR)
    {
      sizeflag ^= AFLAG;
      if (dp->bytemode3 != loop_jcxz_mode || intel_syntax)
	{
	  if ((sizeflag & AFLAG) || mode_64bit)
	    oappend ("addr32 ");
	  else
	    oappend ("addr16 ");
	  used_prefixes |= PREFIX_ADDR;
	}
    }

  if (!uses_SSE_prefix && (prefixes & PREFIX_DATA))
    {
      sizeflag ^= DFLAG;
      if (dp->bytemode3 == cond_jump_mode
	  && dp->bytemode1 == v_mode
	  && !intel_syntax)
	{
	  if (sizeflag & DFLAG)
	    oappend ("data32 ");
	  else
	    oappend ("data16 ");
	  used_prefixes |= PREFIX_DATA;
	}
    }

  if (need_modrm)
    {
      FETCH_DATA (info, codep + 1);
      mod = (*codep >> 6) & 3;
      reg = (*codep >> 3) & 7;
      rm = *codep & 7;
    }

  if (dp->name == NULL && dp->bytemode1 == FLOATCODE)
    {
      dofloat (sizeflag);
    }
  else
    {
      int index;
      if (dp->name == NULL)
	{
	  switch (dp->bytemode1)
	    {
	    case USE_GROUPS:
	      dp = &grps[dp->bytemode2][reg];
	      break;

	    case USE_PREFIX_USER_TABLE:
	      index = 0;
	      used_prefixes |= (prefixes & PREFIX_REPZ);
	      if (prefixes & PREFIX_REPZ)
		index = 1;
	      else
		{
		  used_prefixes |= (prefixes & PREFIX_DATA);
		  if (prefixes & PREFIX_DATA)
		    index = 2;
		  else
		    {
		      used_prefixes |= (prefixes & PREFIX_REPNZ);
		      if (prefixes & PREFIX_REPNZ)
			index = 3;
		    }
		}
	      dp = &prefix_user_table[dp->bytemode2][index];
	      break;

	    case X86_64_SPECIAL:
	      dp = &x86_64_table[dp->bytemode2][mode_64bit];
	      break;

	    default:
	      oappend (INTERNAL_DISASSEMBLER_ERROR);
	      break;
	    }
	}

      if (putop (dp->name, sizeflag) == 0)
	{
	  obufp = op1out;
	  op_ad = 2;
	  if (dp->op1)
	    (*dp->op1) (dp->bytemode1, sizeflag);

	  obufp = op2out;
	  op_ad = 1;
	  if (dp->op2)
	    (*dp->op2) (dp->bytemode2, sizeflag);

	  obufp = op3out;
	  op_ad = 0;
	  if (dp->op3)
	    (*dp->op3) (dp->bytemode3, sizeflag);
	}
    }

  /* See if any prefixes were not used.  If so, print the first one
     separately.  If we don't do this, we'll wind up printing an
     instruction stream which does not precisely correspond to the
     bytes we are disassembling.  */
  if ((prefixes & ~used_prefixes) != 0)
    {
      const char *name;

      name = prefix_name (priv.the_buffer[0], priv.orig_sizeflag);
      if (name == NULL)
	name = INTERNAL_DISASSEMBLER_ERROR;
      (*info->fprintf_func) (info->stream, "%s", name);
      return 1;
    }
  if (rex & ~rex_used)
    {
      const char *name;
      name = prefix_name (rex | 0x40, priv.orig_sizeflag);
      if (name == NULL)
	name = INTERNAL_DISASSEMBLER_ERROR;
      (*info->fprintf_func) (info->stream, "%s ", name);
    }

  obufp = obuf + strlen (obuf);
  for (i = strlen (obuf); i < 6; i++)
    oappend (" ");
  oappend (" ");
  (*info->fprintf_func) (info->stream, "%s", obuf);

  /* The enter and bound instructions are printed with operands in the same
     order as the intel book; everything else is printed in reverse order.  */
  if (intel_syntax || two_source_ops)
    {
      first = op1out;
      second = op2out;
      third = op3out;
      op_ad = op_index[0];
      op_index[0] = op_index[2];
      op_index[2] = op_ad;
    }
  else
    {
      first = op3out;
      second = op2out;
      third = op1out;
    }
  needcomma = 0;
  if (*first)
    {
      if (op_index[0] != -1 && !op_riprel[0])
	(*info->print_address_func) ((bfd_vma) op_address[op_index[0]], info);
      else
	(*info->fprintf_func) (info->stream, "%s", first);
      needcomma = 1;
    }
  if (*second)
    {
      if (needcomma)
	(*info->fprintf_func) (info->stream, ",");
      if (op_index[1] != -1 && !op_riprel[1])
	(*info->print_address_func) ((bfd_vma) op_address[op_index[1]], info);
      else
	(*info->fprintf_func) (info->stream, "%s", second);
      needcomma = 1;
    }
  if (*third)
    {
      if (needcomma)
	(*info->fprintf_func) (info->stream, ",");
      if (op_index[2] != -1 && !op_riprel[2])
	(*info->print_address_func) ((bfd_vma) op_address[op_index[2]], info);
      else
	(*info->fprintf_func) (info->stream, "%s", third);
    }
  for (i = 0; i < 3; i++)
    if (op_index[i] != -1 && op_riprel[i])
      {
	(*info->fprintf_func) (info->stream, "        # ");
	(*info->print_address_func) ((bfd_vma) (start_pc + codep - start_codep
						+ op_address[op_index[i]]), info);
      }
  return codep - priv.the_buffer;
}

static const char *float_mem[] = {
  /* d8 */
  "fadd{s||s|}",
  "fmul{s||s|}",
  "fcom{s||s|}",
  "fcomp{s||s|}",
  "fsub{s||s|}",
  "fsubr{s||s|}",
  "fdiv{s||s|}",
  "fdivr{s||s|}",
  /*  d9 */
  "fld{s||s|}",
  "(bad)",
  "fst{s||s|}",
  "fstp{s||s|}",
  "fldenv",
  "fldcw",
  "fNstenv",
  "fNstcw",
  /* da */
  "fiadd{l||l|}",
  "fimul{l||l|}",
  "ficom{l||l|}",
  "ficomp{l||l|}",
  "fisub{l||l|}",
  "fisubr{l||l|}",
  "fidiv{l||l|}",
  "fidivr{l||l|}",
  /* db */
  "fild{l||l|}",
  "(bad)",
  "fist{l||l|}",
  "fistp{l||l|}",
  "(bad)",
  "fld{t||t|}",
  "(bad)",
  "fstp{t||t|}",
  /* dc */
  "fadd{l||l|}",
  "fmul{l||l|}",
  "fcom{l||l|}",
  "fcomp{l||l|}",
  "fsub{l||l|}",
  "fsubr{l||l|}",
  "fdiv{l||l|}",
  "fdivr{l||l|}",
  /* dd */
  "fld{l||l|}",
  "(bad)",
  "fst{l||l|}",
  "fstp{l||l|}",
  "frstor",
  "(bad)",
  "fNsave",
  "fNstsw",
  /* de */
  "fiadd",
  "fimul",
  "ficom",
  "ficomp",
  "fisub",
  "fisubr",
  "fidiv",
  "fidivr",
  /* df */
  "fild",
  "(bad)",
  "fist",
  "fistp",
  "fbld",
  "fild{ll||ll|}",
  "fbstp",
  "fistpll",
};

#define ST OP_ST, 0
#define STi OP_STi, 0

#define FGRPd9_2 NULL, NULL, 0, NULL, 0, NULL, 0
#define FGRPd9_4 NULL, NULL, 1, NULL, 0, NULL, 0
#define FGRPd9_5 NULL, NULL, 2, NULL, 0, NULL, 0
#define FGRPd9_6 NULL, NULL, 3, NULL, 0, NULL, 0
#define FGRPd9_7 NULL, NULL, 4, NULL, 0, NULL, 0
#define FGRPda_5 NULL, NULL, 5, NULL, 0, NULL, 0
#define FGRPdb_4 NULL, NULL, 6, NULL, 0, NULL, 0
#define FGRPde_3 NULL, NULL, 7, NULL, 0, NULL, 0
#define FGRPdf_4 NULL, NULL, 8, NULL, 0, NULL, 0

static const struct dis386 float_reg[][8] = {
  /* d8 */
  {
    { "fadd",	ST, STi, XX },
    { "fmul",	ST, STi, XX },
    { "fcom",	STi, XX, XX },
    { "fcomp",	STi, XX, XX },
    { "fsub",	ST, STi, XX },
    { "fsubr",	ST, STi, XX },
    { "fdiv",	ST, STi, XX },
    { "fdivr",	ST, STi, XX },
  },
  /* d9 */
  {
    { "fld",	STi, XX, XX },
    { "fxch",	STi, XX, XX },
    { FGRPd9_2 },
    { "(bad)",	XX, XX, XX },
    { FGRPd9_4 },
    { FGRPd9_5 },
    { FGRPd9_6 },
    { FGRPd9_7 },
  },
  /* da */
  {
    { "fcmovb",	ST, STi, XX },
    { "fcmove",	ST, STi, XX },
    { "fcmovbe",ST, STi, XX },
    { "fcmovu",	ST, STi, XX },
    { "(bad)",	XX, XX, XX },
    { FGRPda_5 },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
  },
  /* db */
  {
    { "fcmovnb",ST, STi, XX },
    { "fcmovne",ST, STi, XX },
    { "fcmovnbe",ST, STi, XX },
    { "fcmovnu",ST, STi, XX },
    { FGRPdb_4 },
    { "fucomi",	ST, STi, XX },
    { "fcomi",	ST, STi, XX },
    { "(bad)",	XX, XX, XX },
  },
  /* dc */
  {
    { "fadd",	STi, ST, XX },
    { "fmul",	STi, ST, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
#if UNIXWARE_COMPAT
    { "fsub",	STi, ST, XX },
    { "fsubr",	STi, ST, XX },
    { "fdiv",	STi, ST, XX },
    { "fdivr",	STi, ST, XX },
#else
    { "fsubr",	STi, ST, XX },
    { "fsub",	STi, ST, XX },
    { "fdivr",	STi, ST, XX },
    { "fdiv",	STi, ST, XX },
#endif
  },
  /* dd */
  {
    { "ffree",	STi, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "fst",	STi, XX, XX },
    { "fstp",	STi, XX, XX },
    { "fucom",	STi, XX, XX },
    { "fucomp",	STi, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
  },
  /* de */
  {
    { "faddp",	STi, ST, XX },
    { "fmulp",	STi, ST, XX },
    { "(bad)",	XX, XX, XX },
    { FGRPde_3 },
#if UNIXWARE_COMPAT
    { "fsubp",	STi, ST, XX },
    { "fsubrp",	STi, ST, XX },
    { "fdivp",	STi, ST, XX },
    { "fdivrp",	STi, ST, XX },
#else
    { "fsubrp",	STi, ST, XX },
    { "fsubp",	STi, ST, XX },
    { "fdivrp",	STi, ST, XX },
    { "fdivp",	STi, ST, XX },
#endif
  },
  /* df */
  {
    { "ffreep",	STi, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { "(bad)",	XX, XX, XX },
    { FGRPdf_4 },
    { "fucomip",ST, STi, XX },
    { "fcomip", ST, STi, XX },
    { "(bad)",	XX, XX, XX },
  },
};

static char *fgrps[][8] = {
  /* d9_2  0 */
  {
    "fnop","(bad)","(bad)","(bad)","(bad)","(bad)","(bad)","(bad)",
  },

  /* d9_4  1 */
  {
    "fchs","fabs","(bad)","(bad)","ftst","fxam","(bad)","(bad)",
  },

  /* d9_5  2 */
  {
    "fld1","fldl2t","fldl2e","fldpi","fldlg2","fldln2","fldz","(bad)",
  },

  /* d9_6  3 */
  {
    "f2xm1","fyl2x","fptan","fpatan","fxtract","fprem1","fdecstp","fincstp",
  },

  /* d9_7  4 */
  {
    "fprem","fyl2xp1","fsqrt","fsincos","frndint","fscale","fsin","fcos",
  },

  /* da_5  5 */
  {
    "(bad)","fucompp","(bad)","(bad)","(bad)","(bad)","(bad)","(bad)",
  },

  /* db_4  6 */
  {
    "feni(287 only)","fdisi(287 only)","fNclex","fNinit",
    "fNsetpm(287 only)","(bad)","(bad)","(bad)",
  },

  /* de_3  7 */
  {
    "(bad)","fcompp","(bad)","(bad)","(bad)","(bad)","(bad)","(bad)",
  },

  /* df_4  8 */
  {
    "fNstsw","(bad)","(bad)","(bad)","(bad)","(bad)","(bad)","(bad)",
  },
};

static void
dofloat (sizeflag)
     int sizeflag;
{
  const struct dis386 *dp;
  unsigned char floatop;

  floatop = codep[-1];

  if (mod != 3)
    {
      putop (float_mem[(floatop - 0xd8) * 8 + reg], sizeflag);
      obufp = op1out;
      if (floatop == 0xdb)
        OP_E (x_mode, sizeflag);
      else if (floatop == 0xdd)
        OP_E (d_mode, sizeflag);
      else
        OP_E (v_mode, sizeflag);
      return;
    }
  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;

  dp = &float_reg[floatop - 0xd8][reg];
  if (dp->name == NULL)
    {
      putop (fgrps[dp->bytemode1][rm], sizeflag);

      /* Instruction fnstsw is only one with strange arg.  */
      if (floatop == 0xdf && codep[-1] == 0xe0)
	strcpy (op1out, names16[0]);
    }
  else
    {
      putop (dp->name, sizeflag);

      obufp = op1out;
      if (dp->op1)
	(*dp->op1) (dp->bytemode1, sizeflag);
      obufp = op2out;
      if (dp->op2)
	(*dp->op2) (dp->bytemode2, sizeflag);
    }
}

static void
OP_ST (bytemode, sizeflag)
     int bytemode ATTRIBUTE_UNUSED;
     int sizeflag ATTRIBUTE_UNUSED;
{
  oappend ("%st");
}

static void
OP_STi (bytemode, sizeflag)
     int bytemode ATTRIBUTE_UNUSED;
     int sizeflag ATTRIBUTE_UNUSED;
{
  sprintf (scratchbuf, "%%st(%d)", rm);
  oappend (scratchbuf + intel_syntax);
}

/* Capital letters in template are macros.  */
static int
putop (template, sizeflag)
     const char *template;
     int sizeflag;
{
  const char *p;
  int alt;

  for (p = template; *p; p++)
    {
      switch (*p)
	{
	default:
	  *obufp++ = *p;
	  break;
	case '{':
	  alt = 0;
	  if (intel_syntax)
	    alt += 1;
	  if (mode_64bit)
	    alt += 2;
	  while (alt != 0)
	    {
	      while (*++p != '|')
		{
		  if (*p == '}')
		    {
		      /* Alternative not valid.  */
		      strcpy (obuf, "(bad)");
		      obufp = obuf + 5;
		      return 1;
		    }
		  else if (*p == '\0')
		    abort ();
		}
	      alt--;
	    }
	  break;
	case '|':
	  while (*++p != '}')
	    {
	      if (*p == '\0')
		abort ();
	    }
	  break;
	case '}':
	  break;
	case 'A':
          if (intel_syntax)
            break;
	  if (mod != 3 || (sizeflag & SUFFIX_ALWAYS))
	    *obufp++ = 'b';
	  break;
	case 'B':
          if (intel_syntax)
            break;
	  if (sizeflag & SUFFIX_ALWAYS)
	    *obufp++ = 'b';
	  break;
	case 'E':		/* For jcxz/jecxz */
	  if (mode_64bit)
	    {
	      if (sizeflag & AFLAG)
		*obufp++ = 'r';
	      else
		*obufp++ = 'e';
	    }
	  else
	    if (sizeflag & AFLAG)
	      *obufp++ = 'e';
	  used_prefixes |= (prefixes & PREFIX_ADDR);
	  break;
	case 'F':
          if (intel_syntax)
            break;
	  if ((prefixes & PREFIX_ADDR) || (sizeflag & SUFFIX_ALWAYS))
	    {
	      if (sizeflag & AFLAG)
		*obufp++ = mode_64bit ? 'q' : 'l';
	      else
		*obufp++ = mode_64bit ? 'l' : 'w';
	      used_prefixes |= (prefixes & PREFIX_ADDR);
	    }
	  break;
	case 'H':
          if (intel_syntax)
            break;
	  if ((prefixes & (PREFIX_CS | PREFIX_DS)) == PREFIX_CS
	      || (prefixes & (PREFIX_CS | PREFIX_DS)) == PREFIX_DS)
	    {
	      used_prefixes |= prefixes & (PREFIX_CS | PREFIX_DS);
	      *obufp++ = ',';
	      *obufp++ = 'p';
	      if (prefixes & PREFIX_DS)
		*obufp++ = 't';
	      else
		*obufp++ = 'n';
	    }
	  break;
	case 'L':
          if (intel_syntax)
            break;
	  if (sizeflag & SUFFIX_ALWAYS)
	    *obufp++ = 'l';
	  break;
	case 'N':
	  if ((prefixes & PREFIX_FWAIT) == 0)
	    *obufp++ = 'n';
	  else
	    used_prefixes |= PREFIX_FWAIT;
	  break;
	case 'O':
	  USED_REX (REX_MODE64);
	  if (rex & REX_MODE64)
	    *obufp++ = 'o';
	  else
	    *obufp++ = 'd';
	  break;
	case 'T':
          if (intel_syntax)
            break;
	  if (mode_64bit)
	    {
	      *obufp++ = 'q';
	      break;
	    }
	  /* Fall through.  */
	case 'P':
          if (intel_syntax)
            break;
	  if ((prefixes & PREFIX_DATA)
	      || (rex & REX_MODE64)
	      || (sizeflag & SUFFIX_ALWAYS))
	    {
	      USED_REX (REX_MODE64);
	      if (rex & REX_MODE64)
		*obufp++ = 'q';
	      else
		{
		   if (sizeflag & DFLAG)
		      *obufp++ = 'l';
		   else
		     *obufp++ = 'w';
		   used_prefixes |= (prefixes & PREFIX_DATA);
		}
	    }
	  break;
	case 'U':
          if (intel_syntax)
            break;
	  if (mode_64bit)
	    {
	      *obufp++ = 'q';
	      break;
	    }
	  /* Fall through.  */
	case 'Q':
          if (intel_syntax)
            break;
	  USED_REX (REX_MODE64);
	  if (mod != 3 || (sizeflag & SUFFIX_ALWAYS))
	    {
	      if (rex & REX_MODE64)
		*obufp++ = 'q';
	      else
		{
		  if (sizeflag & DFLAG)
		    *obufp++ = 'l';
		  else
		    *obufp++ = 'w';
		  used_prefixes |= (prefixes & PREFIX_DATA);
		}
	    }
	  break;
	case 'R':
	  USED_REX (REX_MODE64);
          if (intel_syntax)
	    {
	      if (rex & REX_MODE64)
		{
		  *obufp++ = 'q';
		  *obufp++ = 't';
		}
	      else if (sizeflag & DFLAG)
		{
		  *obufp++ = 'd';
		  *obufp++ = 'q';
		}
	      else
		{
		  *obufp++ = 'w';
		  *obufp++ = 'd';
		}
	    }
	  else
	    {
	      if (rex & REX_MODE64)
		*obufp++ = 'q';
	      else if (sizeflag & DFLAG)
		*obufp++ = 'l';
	      else
		*obufp++ = 'w';
	    }
	  if (!(rex & REX_MODE64))
	    used_prefixes |= (prefixes & PREFIX_DATA);
	  break;
	case 'S':
          if (intel_syntax)
            break;
	  if (sizeflag & SUFFIX_ALWAYS)
	    {
	      if (rex & REX_MODE64)
		*obufp++ = 'q';
	      else
		{
		  if (sizeflag & DFLAG)
		    *obufp++ = 'l';
		  else
		    *obufp++ = 'w';
		  used_prefixes |= (prefixes & PREFIX_DATA);
		}
	    }
	  break;
	case 'X':
	  if (prefixes & PREFIX_DATA)
	    *obufp++ = 'd';
	  else
	    *obufp++ = 's';
          used_prefixes |= (prefixes & PREFIX_DATA);
	  break;
	case 'Y':
          if (intel_syntax)
            break;
	  if (rex & REX_MODE64)
	    {
	      USED_REX (REX_MODE64);
	      *obufp++ = 'q';
	    }
	  break;
	  /* implicit operand size 'l' for i386 or 'q' for x86-64 */
	case 'W':
	  /* operand size flag for cwtl, cbtw */
	  USED_REX (0);
	  if (rex)
	    *obufp++ = 'l';
	  else if (sizeflag & DFLAG)
	    *obufp++ = 'w';
	  else
	    *obufp++ = 'b';
          if (intel_syntax)
	    {
	      if (rex)
		{
		  *obufp++ = 'q';
		  *obufp++ = 'e';
		}
	      if (sizeflag & DFLAG)
		{
		  *obufp++ = 'd';
		  *obufp++ = 'e';
		}
	      else
		{
		  *obufp++ = 'w';
		}
	    }
	  if (!rex)
	    used_prefixes |= (prefixes & PREFIX_DATA);
	  break;
	}
    }
  *obufp = 0;
  return 0;
}

static void
oappend (s)
     const char *s;
{
  strcpy (obufp, s);
  obufp += strlen (s);
}

static void
append_seg ()
{
  if (prefixes & PREFIX_CS)
    {
      used_prefixes |= PREFIX_CS;
      oappend ("%cs:" + intel_syntax);
    }
  if (prefixes & PREFIX_DS)
    {
      used_prefixes |= PREFIX_DS;
      oappend ("%ds:" + intel_syntax);
    }
  if (prefixes & PREFIX_SS)
    {
      used_prefixes |= PREFIX_SS;
      oappend ("%ss:" + intel_syntax);
    }
  if (prefixes & PREFIX_ES)
    {
      used_prefixes |= PREFIX_ES;
      oappend ("%es:" + intel_syntax);
    }
  if (prefixes & PREFIX_FS)
    {
      used_prefixes |= PREFIX_FS;
      oappend ("%fs:" + intel_syntax);
    }
  if (prefixes & PREFIX_GS)
    {
      used_prefixes |= PREFIX_GS;
      oappend ("%gs:" + intel_syntax);
    }
}

static void
OP_indirE (bytemode, sizeflag)
     int bytemode;
     int sizeflag;
{
  if (!intel_syntax)
    oappend ("*");
  OP_E (bytemode, sizeflag);
}

static void
print_operand_value (buf, hex, disp)
  char *buf;
  int hex;
  bfd_vma disp;
{
  if (mode_64bit)
    {
      if (hex)
	{
	  char tmp[30];
	  int i;
	  buf[0] = '0';
	  buf[1] = 'x';
	  sprintf_vma (tmp, disp);
	  for (i = 0; tmp[i] == '0' && tmp[i + 1]; i++);
	  strcpy (buf + 2, tmp + i);
	}
      else
	{
	  bfd_signed_vma v = disp;
	  char tmp[30];
	  int i;
	  if (v < 0)
	    {
	      *(buf++) = '-';
	      v = -disp;
	      /* Check for possible overflow on 0x8000000000000000.  */
	      if (v < 0)
		{
		  strcpy (buf, "9223372036854775808");
		  return;
		}
	    }
	  if (!v)
	    {
	      strcpy (buf, "0");
	      return;
	    }

	  i = 0;
	  tmp[29] = 0;
	  while (v)
	    {
	      tmp[28 - i] = (v % 10) + '0';
	      v /= 10;
	      i++;
	    }
	  strcpy (buf, tmp + 29 - i);
	}
    }
  else
    {
      if (hex)
	sprintf (buf, "0x%x", (unsigned int) disp);
      else
	sprintf (buf, "%d", (int) disp);
    }
}

static void
OP_E (bytemode, sizeflag)
     int bytemode;
     int sizeflag;
{
  bfd_vma disp;
  int add = 0;
  int riprel = 0;
  USED_REX (REX_EXTZ);
  if (rex & REX_EXTZ)
    add += 8;

  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;

  if (mod == 3)
    {
      switch (bytemode)
	{
	case b_mode:
	  USED_REX (0);
	  if (rex)
	    oappend (names8rex[rm + add]);
	  else
	    oappend (names8[rm + add]);
	  break;
	case w_mode:
	  oappend (names16[rm + add]);
	  break;
	case d_mode:
	  oappend (names32[rm + add]);
	  break;
	case q_mode:
	  oappend (names64[rm + add]);
	  break;
	case m_mode:
	  if (mode_64bit)
	    oappend (names64[rm + add]);
	  else
	    oappend (names32[rm + add]);
	  break;
	case v_mode:
	  USED_REX (REX_MODE64);
	  if (rex & REX_MODE64)
	    oappend (names64[rm + add]);
	  else if (sizeflag & DFLAG)
	    oappend (names32[rm + add]);
	  else
	    oappend (names16[rm + add]);
	  used_prefixes |= (prefixes & PREFIX_DATA);
	  break;
	case 0:
	  if (!(codep[-2] == 0xAE && codep[-1] == 0xF8 /* sfence */)
	      && !(codep[-2] == 0xAE && codep[-1] == 0xF0 /* mfence */)
	      && !(codep[-2] == 0xAE && codep[-1] == 0xe8 /* lfence */))
	    BadOp ();	/* bad sfence,lea,lds,les,lfs,lgs,lss modrm */
	  break;
	default:
	  oappend (INTERNAL_DISASSEMBLER_ERROR);
	  break;
	}
      return;
    }

  disp = 0;
  append_seg ();

  if ((sizeflag & AFLAG) || mode_64bit) /* 32 bit address mode */
    {
      int havesib;
      int havebase;
      int base;
      int index = 0;
      int scale = 0;

      havesib = 0;
      havebase = 1;
      base = rm;

      if (base == 4)
	{
	  havesib = 1;
	  FETCH_DATA (the_info, codep + 1);
	  scale = (*codep >> 6) & 3;
	  index = (*codep >> 3) & 7;
	  base = *codep & 7;
	  USED_REX (REX_EXTY);
	  USED_REX (REX_EXTZ);
	  if (rex & REX_EXTY)
	    index += 8;
	  if (rex & REX_EXTZ)
	    base += 8;
	  codep++;
	}

      switch (mod)
	{
	case 0:
	  if ((base & 7) == 5)
	    {
	      havebase = 0;
	      if (mode_64bit && !havesib && (sizeflag & AFLAG))
		riprel = 1;
	      disp = get32s ();
	    }
	  break;
	case 1:
	  FETCH_DATA (the_info, codep + 1);
	  disp = *codep++;
	  if ((disp & 0x80) != 0)
	    disp -= 0x100;
	  break;
	case 2:
	  disp = get32s ();
	  break;
	}

      if (!intel_syntax)
        if (mod != 0 || (base & 7) == 5)
          {
	    print_operand_value (scratchbuf, !riprel, disp);
            oappend (scratchbuf);
	    if (riprel)
	      {
		set_op (disp, 1);
		oappend ("(%rip)");
	      }
          }

      if (havebase || (havesib && (index != 4 || scale != 0)))
	{
          if (intel_syntax)
            {
              switch (bytemode)
                {
                case b_mode:
                  oappend ("BYTE PTR ");
                  break;
                case w_mode:
                  oappend ("WORD PTR ");
                  break;
                case v_mode:
                  oappend ("DWORD PTR ");
                  break;
                case d_mode:
                  oappend ("QWORD PTR ");
                  break;
                case m_mode:
		  if (mode_64bit)
		    oappend ("DWORD PTR ");
		  else
		    oappend ("QWORD PTR ");
		  break;
                case x_mode:
                  oappend ("XWORD PTR ");
                  break;
                default:
                  break;
                }
             }
	  *obufp++ = open_char;
	  if (intel_syntax && riprel)
	    oappend ("rip + ");
          *obufp = '\0';
	  USED_REX (REX_EXTZ);
	  if (!havesib && (rex & REX_EXTZ))
	    base += 8;
	  if (havebase)
	    oappend (mode_64bit && (sizeflag & AFLAG)
		     ? names64[base] : names32[base]);
	  if (havesib)
	    {
	      if (index != 4)
		{
                  if (intel_syntax)
                    {
                      if (havebase)
                        {
                          *obufp++ = separator_char;
                          *obufp = '\0';
                        }
                      sprintf (scratchbuf, "%s",
			       mode_64bit && (sizeflag & AFLAG)
			       ? names64[index] : names32[index]);
                    }
                  else
		    sprintf (scratchbuf, ",%s",
			     mode_64bit && (sizeflag & AFLAG)
			     ? names64[index] : names32[index]);
		  oappend (scratchbuf);
		}
              if (!intel_syntax
                  || (intel_syntax
                      && bytemode != b_mode
                      && bytemode != w_mode
                      && bytemode != v_mode))
                {
                  *obufp++ = scale_char;
                  *obufp = '\0';
	          sprintf (scratchbuf, "%d", 1 << scale);
	          oappend (scratchbuf);
                }
	    }
          if (intel_syntax)
            if (mod != 0 || (base & 7) == 5)
              {
		/* Don't print zero displacements.  */
                if (disp != 0)
                  {
		    if ((bfd_signed_vma) disp > 0)
		      {
			*obufp++ = '+';
			*obufp = '\0';
		      }

		    print_operand_value (scratchbuf, 0, disp);
                    oappend (scratchbuf);
                  }
              }

	  *obufp++ = close_char;
          *obufp = '\0';
	}
      else if (intel_syntax)
        {
          if (mod != 0 || (base & 7) == 5)
            {
	      if (prefixes & (PREFIX_CS | PREFIX_SS | PREFIX_DS
			      | PREFIX_ES | PREFIX_FS | PREFIX_GS))
		;
	      else
		{
		  oappend (names_seg[ds_reg - es_reg]);
		  oappend (":");
		}
	      print_operand_value (scratchbuf, 1, disp);
              oappend (scratchbuf);
            }
        }
    }
  else
    { /* 16 bit address mode */
      switch (mod)
	{
	case 0:
	  if ((rm & 7) == 6)
	    {
	      disp = get16 ();
	      if ((disp & 0x8000) != 0)
		disp -= 0x10000;
	    }
	  break;
	case 1:
	  FETCH_DATA (the_info, codep + 1);
	  disp = *codep++;
	  if ((disp & 0x80) != 0)
	    disp -= 0x100;
	  break;
	case 2:
	  disp = get16 ();
	  if ((disp & 0x8000) != 0)
	    disp -= 0x10000;
	  break;
	}

      if (!intel_syntax)
        if (mod != 0 || (rm & 7) == 6)
          {
	    print_operand_value (scratchbuf, 0, disp);
            oappend (scratchbuf);
          }

      if (mod != 0 || (rm & 7) != 6)
	{
	  *obufp++ = open_char;
          *obufp = '\0';
	  oappend (index16[rm + add]);
          *obufp++ = close_char;
          *obufp = '\0';
	}
    }
}

static void
OP_G (bytemode, sizeflag)
     int bytemode;
     int sizeflag;
{
  int add = 0;
  USED_REX (REX_EXTX);
  if (rex & REX_EXTX)
    add += 8;
  switch (bytemode)
    {
    case b_mode:
      USED_REX (0);
      if (rex)
	oappend (names8rex[reg + add]);
      else
	oappend (names8[reg + add]);
      break;
    case w_mode:
      oappend (names16[reg + add]);
      break;
    case d_mode:
      oappend (names32[reg + add]);
      break;
    case q_mode:
      oappend (names64[reg + add]);
      break;
    case v_mode:
      USED_REX (REX_MODE64);
      if (rex & REX_MODE64)
	oappend (names64[reg + add]);
      else if (sizeflag & DFLAG)
	oappend (names32[reg + add]);
      else
	oappend (names16[reg + add]);
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      break;
    }
}

static bfd_vma
get64 ()
{
  bfd_vma x;
#ifdef BFD64
  unsigned int a;
  unsigned int b;

  FETCH_DATA (the_info, codep + 8);
  a = *codep++ & 0xff;
  a |= (*codep++ & 0xff) << 8;
  a |= (*codep++ & 0xff) << 16;
  a |= (*codep++ & 0xff) << 24;
  b = *codep++ & 0xff;
  b |= (*codep++ & 0xff) << 8;
  b |= (*codep++ & 0xff) << 16;
  b |= (*codep++ & 0xff) << 24;
  x = a + ((bfd_vma) b << 32);
#else
  abort ();
  x = 0;
#endif
  return x;
}

static bfd_signed_vma
get32 ()
{
  bfd_signed_vma x = 0;

  FETCH_DATA (the_info, codep + 4);
  x = *codep++ & (bfd_signed_vma) 0xff;
  x |= (*codep++ & (bfd_signed_vma) 0xff) << 8;
  x |= (*codep++ & (bfd_signed_vma) 0xff) << 16;
  x |= (*codep++ & (bfd_signed_vma) 0xff) << 24;
  return x;
}

static bfd_signed_vma
get32s ()
{
  bfd_signed_vma x = 0;

  FETCH_DATA (the_info, codep + 4);
  x = *codep++ & (bfd_signed_vma) 0xff;
  x |= (*codep++ & (bfd_signed_vma) 0xff) << 8;
  x |= (*codep++ & (bfd_signed_vma) 0xff) << 16;
  x |= (*codep++ & (bfd_signed_vma) 0xff) << 24;

  x = (x ^ ((bfd_signed_vma) 1 << 31)) - ((bfd_signed_vma) 1 << 31);

  return x;
}

static int
get16 ()
{
  int x = 0;

  FETCH_DATA (the_info, codep + 2);
  x = *codep++ & 0xff;
  x |= (*codep++ & 0xff) << 8;
  return x;
}

static void
set_op (op, riprel)
     bfd_vma op;
     int riprel;
{
  op_index[op_ad] = op_ad;
  if (mode_64bit)
    {
      op_address[op_ad] = op;
      op_riprel[op_ad] = riprel;
    }
  else
    {
      /* Mask to get a 32-bit address.  */
      op_address[op_ad] = op & 0xffffffff;
      op_riprel[op_ad] = riprel & 0xffffffff;
    }
}

static void
OP_REG (code, sizeflag)
     int code;
     int sizeflag;
{
  const char *s;
  int add = 0;
  USED_REX (REX_EXTZ);
  if (rex & REX_EXTZ)
    add = 8;

  switch (code)
    {
    case indir_dx_reg:
      if (intel_syntax)
        s = "[dx]";
      else
        s = "(%dx)";
      break;
    case ax_reg: case cx_reg: case dx_reg: case bx_reg:
    case sp_reg: case bp_reg: case si_reg: case di_reg:
      s = names16[code - ax_reg + add];
      break;
    case es_reg: case ss_reg: case cs_reg:
    case ds_reg: case fs_reg: case gs_reg:
      s = names_seg[code - es_reg + add];
      break;
    case al_reg: case ah_reg: case cl_reg: case ch_reg:
    case dl_reg: case dh_reg: case bl_reg: case bh_reg:
      USED_REX (0);
      if (rex)
	s = names8rex[code - al_reg + add];
      else
	s = names8[code - al_reg];
      break;
    case rAX_reg: case rCX_reg: case rDX_reg: case rBX_reg:
    case rSP_reg: case rBP_reg: case rSI_reg: case rDI_reg:
      if (mode_64bit)
	{
	  s = names64[code - rAX_reg + add];
	  break;
	}
      code += eAX_reg - rAX_reg;
      /* Fall through.  */
    case eAX_reg: case eCX_reg: case eDX_reg: case eBX_reg:
    case eSP_reg: case eBP_reg: case eSI_reg: case eDI_reg:
      USED_REX (REX_MODE64);
      if (rex & REX_MODE64)
	s = names64[code - eAX_reg + add];
      else if (sizeflag & DFLAG)
	s = names32[code - eAX_reg + add];
      else
	s = names16[code - eAX_reg + add];
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    default:
      s = INTERNAL_DISASSEMBLER_ERROR;
      break;
    }
  oappend (s);
}

static void
OP_IMREG (code, sizeflag)
     int code;
     int sizeflag;
{
  const char *s;

  switch (code)
    {
    case indir_dx_reg:
      if (intel_syntax)
        s = "[dx]";
      else
        s = "(%dx)";
      break;
    case ax_reg: case cx_reg: case dx_reg: case bx_reg:
    case sp_reg: case bp_reg: case si_reg: case di_reg:
      s = names16[code - ax_reg];
      break;
    case es_reg: case ss_reg: case cs_reg:
    case ds_reg: case fs_reg: case gs_reg:
      s = names_seg[code - es_reg];
      break;
    case al_reg: case ah_reg: case cl_reg: case ch_reg:
    case dl_reg: case dh_reg: case bl_reg: case bh_reg:
      USED_REX (0);
      if (rex)
	s = names8rex[code - al_reg];
      else
	s = names8[code - al_reg];
      break;
    case eAX_reg: case eCX_reg: case eDX_reg: case eBX_reg:
    case eSP_reg: case eBP_reg: case eSI_reg: case eDI_reg:
      USED_REX (REX_MODE64);
      if (rex & REX_MODE64)
	s = names64[code - eAX_reg];
      else if (sizeflag & DFLAG)
	s = names32[code - eAX_reg];
      else
	s = names16[code - eAX_reg];
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    default:
      s = INTERNAL_DISASSEMBLER_ERROR;
      break;
    }
  oappend (s);
}

static void
OP_I (bytemode, sizeflag)
     int bytemode;
     int sizeflag;
{
  bfd_signed_vma op;
  bfd_signed_vma mask = -1;

  switch (bytemode)
    {
    case b_mode:
      FETCH_DATA (the_info, codep + 1);
      op = *codep++;
      mask = 0xff;
      break;
    case q_mode:
      if (mode_64bit)
	{
	  op = get32s ();
	  break;
	}
      /* Fall through.  */
    case v_mode:
      USED_REX (REX_MODE64);
      if (rex & REX_MODE64)
	op = get32s ();
      else if (sizeflag & DFLAG)
	{
	  op = get32 ();
	  mask = 0xffffffff;
	}
      else
	{
	  op = get16 ();
	  mask = 0xfffff;
	}
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    case w_mode:
      mask = 0xfffff;
      op = get16 ();
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      return;
    }

  op &= mask;
  scratchbuf[0] = '$';
  print_operand_value (scratchbuf + 1, 1, op);
  oappend (scratchbuf + intel_syntax);
  scratchbuf[0] = '\0';
}

static void
OP_I64 (bytemode, sizeflag)
     int bytemode;
     int sizeflag;
{
  bfd_signed_vma op;
  bfd_signed_vma mask = -1;

  if (!mode_64bit)
    {
      OP_I (bytemode, sizeflag);
      return;
    }

  switch (bytemode)
    {
    case b_mode:
      FETCH_DATA (the_info, codep + 1);
      op = *codep++;
      mask = 0xff;
      break;
    case v_mode:
      USED_REX (REX_MODE64);
      if (rex & REX_MODE64)
	op = get64 ();
      else if (sizeflag & DFLAG)
	{
	  op = get32 ();
	  mask = 0xffffffff;
	}
      else
	{
	  op = get16 ();
	  mask = 0xfffff;
	}
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    case w_mode:
      mask = 0xfffff;
      op = get16 ();
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      return;
    }

  op &= mask;
  scratchbuf[0] = '$';
  print_operand_value (scratchbuf + 1, 1, op);
  oappend (scratchbuf + intel_syntax);
  scratchbuf[0] = '\0';
}

static void
OP_sI (bytemode, sizeflag)
     int bytemode;
     int sizeflag;
{
  bfd_signed_vma op;
  bfd_signed_vma mask = -1;

  switch (bytemode)
    {
    case b_mode:
      FETCH_DATA (the_info, codep + 1);
      op = *codep++;
      if ((op & 0x80) != 0)
	op -= 0x100;
      mask = 0xffffffff;
      break;
    case v_mode:
      USED_REX (REX_MODE64);
      if (rex & REX_MODE64)
	op = get32s ();
      else if (sizeflag & DFLAG)
	{
	  op = get32s ();
	  mask = 0xffffffff;
	}
      else
	{
	  mask = 0xffffffff;
	  op = get16 ();
	  if ((op & 0x8000) != 0)
	    op -= 0x10000;
	}
      used_prefixes |= (prefixes & PREFIX_DATA);
      break;
    case w_mode:
      op = get16 ();
      mask = 0xffffffff;
      if ((op & 0x8000) != 0)
	op -= 0x10000;
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      return;
    }

  scratchbuf[0] = '$';
  print_operand_value (scratchbuf + 1, 1, op);
  oappend (scratchbuf + intel_syntax);
}

static void
OP_J (bytemode, sizeflag)
     int bytemode;
     int sizeflag;
{
  bfd_vma disp;
  bfd_vma mask = -1;

  switch (bytemode)
    {
    case b_mode:
      FETCH_DATA (the_info, codep + 1);
      disp = *codep++;
      if ((disp & 0x80) != 0)
	disp -= 0x100;
      break;
    case v_mode:
      if (sizeflag & DFLAG)
	disp = get32s ();
      else
	{
	  disp = get16 ();
	  /* For some reason, a data16 prefix on a jump instruction
	     means that the pc is masked to 16 bits after the
	     displacement is added!  */
	  mask = 0xffff;
	}
      break;
    default:
      oappend (INTERNAL_DISASSEMBLER_ERROR);
      return;
    }
  disp = (start_pc + codep - start_codep + disp) & mask;
  set_op (disp, 0);
  print_operand_value (scratchbuf, 1, disp);
  oappend (scratchbuf);
}

static void
OP_SEG (dummy, sizeflag)
     int dummy ATTRIBUTE_UNUSED;
     int sizeflag ATTRIBUTE_UNUSED;
{
  oappend (names_seg[reg]);
}

static void
OP_DIR (dummy, sizeflag)
     int dummy ATTRIBUTE_UNUSED;
     int sizeflag;
{
  int seg, offset;

  if (sizeflag & DFLAG)
    {
      offset = get32 ();
      seg = get16 ();
    }
  else
    {
      offset = get16 ();
      seg = get16 ();
    }
  used_prefixes |= (prefixes & PREFIX_DATA);
  if (intel_syntax)
    sprintf (scratchbuf, "0x%x,0x%x", seg, offset);
  else
    sprintf (scratchbuf, "$0x%x,$0x%x", seg, offset);
  oappend (scratchbuf);
}

static void
OP_OFF (bytemode, sizeflag)
     int bytemode ATTRIBUTE_UNUSED;
     int sizeflag;
{
  bfd_vma off;

  append_seg ();

  if ((sizeflag & AFLAG) || mode_64bit)
    off = get32 ();
  else
    off = get16 ();

  if (intel_syntax)
    {
      if (!(prefixes & (PREFIX_CS | PREFIX_SS | PREFIX_DS
		        | PREFIX_ES | PREFIX_FS | PREFIX_GS)))
	{
	  oappend (names_seg[ds_reg - es_reg]);
	  oappend (":");
	}
    }
  print_operand_value (scratchbuf, 1, off);
  oappend (scratchbuf);
}

static void
OP_OFF64 (bytemode, sizeflag)
     int bytemode ATTRIBUTE_UNUSED;
     int sizeflag ATTRIBUTE_UNUSED;
{
  bfd_vma off;

  if (!mode_64bit)
    {
      OP_OFF (bytemode, sizeflag);
      return;
    }

  append_seg ();

  off = get64 ();

  if (intel_syntax)
    {
      if (!(prefixes & (PREFIX_CS | PREFIX_SS | PREFIX_DS
		        | PREFIX_ES | PREFIX_FS | PREFIX_GS)))
	{
	  oappend (names_seg[ds_reg - es_reg]);
	  oappend (":");
	}
    }
  print_operand_value (scratchbuf, 1, off);
  oappend (scratchbuf);
}

static void
ptr_reg (code, sizeflag)
     int code;
     int sizeflag;
{
  const char *s;
  if (intel_syntax)
    oappend ("[");
  else
    oappend ("(");

  USED_REX (REX_MODE64);
  if (rex & REX_MODE64)
    {
      if (!(sizeflag & AFLAG))
        s = names32[code - eAX_reg];
      else
        s = names64[code - eAX_reg];
    }
  else if (sizeflag & AFLAG)
    s = names32[code - eAX_reg];
  else
    s = names16[code - eAX_reg];
  oappend (s);
  if (intel_syntax)
    oappend ("]");
  else
    oappend (")");
}

static void
OP_ESreg (code, sizeflag)
     int code;
     int sizeflag;
{
  oappend ("%es:" + intel_syntax);
  ptr_reg (code, sizeflag);
}

static void
OP_DSreg (code, sizeflag)
     int code;
     int sizeflag;
{
  if ((prefixes
       & (PREFIX_CS
	  | PREFIX_DS
	  | PREFIX_SS
	  | PREFIX_ES
	  | PREFIX_FS
	  | PREFIX_GS)) == 0)
    prefixes |= PREFIX_DS;
  append_seg ();
  ptr_reg (code, sizeflag);
}

static void
OP_C (dummy, sizeflag)
     int dummy ATTRIBUTE_UNUSED;
     int sizeflag ATTRIBUTE_UNUSED;
{
  int add = 0;
  USED_REX (REX_EXTX);
  if (rex & REX_EXTX)
    add = 8;
  sprintf (scratchbuf, "%%cr%d", reg + add);
  oappend (scratchbuf + intel_syntax);
}

static void
OP_D (dummy, sizeflag)
     int dummy ATTRIBUTE_UNUSED;
     int sizeflag ATTRIBUTE_UNUSED;
{
  int add = 0;
  USED_REX (REX_EXTX);
  if (rex & REX_EXTX)
    add = 8;
  if (intel_syntax)
    sprintf (scratchbuf, "db%d", reg + add);
  else
    sprintf (scratchbuf, "%%db%d", reg + add);
  oappend (scratchbuf);
}

static void
OP_T (dummy, sizeflag)
     int dummy ATTRIBUTE_UNUSED;
     int sizeflag ATTRIBUTE_UNUSED;
{
  sprintf (scratchbuf, "%%tr%d", reg);
  oappend (scratchbuf + intel_syntax);
}

static void
OP_Rd (bytemode, sizeflag)
     int bytemode;
     int sizeflag;
{
  if (mod == 3)
    OP_E (bytemode, sizeflag);
  else
    BadOp ();
}

static void
OP_MMX (bytemode, sizeflag)
     int bytemode ATTRIBUTE_UNUSED;
     int sizeflag ATTRIBUTE_UNUSED;
{
  int add = 0;
  USED_REX (REX_EXTX);
  if (rex & REX_EXTX)
    add = 8;
  used_prefixes |= (prefixes & PREFIX_DATA);
  if (prefixes & PREFIX_DATA)
    sprintf (scratchbuf, "%%xmm%d", reg + add);
  else
    sprintf (scratchbuf, "%%mm%d", reg + add);
  oappend (scratchbuf + intel_syntax);
}

static void
OP_XMM (bytemode, sizeflag)
     int bytemode ATTRIBUTE_UNUSED;
     int sizeflag ATTRIBUTE_UNUSED;
{
  int add = 0;
  USED_REX (REX_EXTX);
  if (rex & REX_EXTX)
    add = 8;
  sprintf (scratchbuf, "%%xmm%d", reg + add);
  oappend (scratchbuf + intel_syntax);
}

static void
OP_EM (bytemode, sizeflag)
     int bytemode;
     int sizeflag;
{
  int add = 0;
  if (mod != 3)
    {
      OP_E (bytemode, sizeflag);
      return;
    }
  USED_REX (REX_EXTZ);
  if (rex & REX_EXTZ)
    add = 8;

  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;
  used_prefixes |= (prefixes & PREFIX_DATA);
  if (prefixes & PREFIX_DATA)
    sprintf (scratchbuf, "%%xmm%d", rm + add);
  else
    sprintf (scratchbuf, "%%mm%d", rm + add);
  oappend (scratchbuf + intel_syntax);
}

static void
OP_EX (bytemode, sizeflag)
     int bytemode;
     int sizeflag;
{
  int add = 0;
  if (mod != 3)
    {
      OP_E (bytemode, sizeflag);
      return;
    }
  USED_REX (REX_EXTZ);
  if (rex & REX_EXTZ)
    add = 8;

  /* Skip mod/rm byte.  */
  MODRM_CHECK;
  codep++;
  sprintf (scratchbuf, "%%xmm%d", rm + add);
  oappend (scratchbuf + intel_syntax);
}

static void
OP_MS (bytemode, sizeflag)
     int bytemode;
     int sizeflag;
{
  if (mod == 3)
    OP_EM (bytemode, sizeflag);
  else
    BadOp ();
}

static void
OP_XS (bytemode, sizeflag)
     int bytemode;
     int sizeflag;
{
  if (mod == 3)
    OP_EX (bytemode, sizeflag);
  else
    BadOp ();
}

static const char *Suffix3DNow[] = {
/* 00 */	NULL,		NULL,		NULL,		NULL,
/* 04 */	NULL,		NULL,		NULL,		NULL,
/* 08 */	NULL,		NULL,		NULL,		NULL,
/* 0C */	"pi2fw",	"pi2fd",	NULL,		NULL,
/* 10 */	NULL,		NULL,		NULL,		NULL,
/* 14 */	NULL,		NULL,		NULL,		NULL,
/* 18 */	NULL,		NULL,		NULL,		NULL,
/* 1C */	"pf2iw",	"pf2id",	NULL,		NULL,
/* 20 */	NULL,		NULL,		NULL,		NULL,
/* 24 */	NULL,		NULL,		NULL,		NULL,
/* 28 */	NULL,		NULL,		NULL,		NULL,
/* 2C */	NULL,		NULL,		NULL,		NULL,
/* 30 */	NULL,		NULL,		NULL,		NULL,
/* 34 */	NULL,		NULL,		NULL,		NULL,
/* 38 */	NULL,		NULL,		NULL,		NULL,
/* 3C */	NULL,		NULL,		NULL,		NULL,
/* 40 */	NULL,		NULL,		NULL,		NULL,
/* 44 */	NULL,		NULL,		NULL,		NULL,
/* 48 */	NULL,		NULL,		NULL,		NULL,
/* 4C */	NULL,		NULL,		NULL,		NULL,
/* 50 */	NULL,		NULL,		NULL,		NULL,
/* 54 */	NULL,		NULL,		NULL,		NULL,
/* 58 */	NULL,		NULL,		NULL,		NULL,
/* 5C */	NULL,		NULL,		NULL,		NULL,
/* 60 */	NULL,		NULL,		NULL,		NULL,
/* 64 */	NULL,		NULL,		NULL,		NULL,
/* 68 */	NULL,		NULL,		NULL,		NULL,
/* 6C */	NULL,		NULL,		NULL,		NULL,
/* 70 */	NULL,		NULL,		NULL,		NULL,
/* 74 */	NULL,		NULL,		NULL,		NULL,
/* 78 */	NULL,		NULL,		NULL,		NULL,
/* 7C */	NULL,		NULL,		NULL,		NULL,
/* 80 */	NULL,		NULL,		NULL,		NULL,
/* 84 */	NULL,		NULL,		NULL,		NULL,
/* 88 */	NULL,		NULL,		"pfnacc",	NULL,
/* 8C */	NULL,		NULL,		"pfpnacc",	NULL,
/* 90 */	"pfcmpge",	NULL,		NULL,		NULL,
/* 94 */	"pfmin",	NULL,		"pfrcp",	"pfrsqrt",
/* 98 */	NULL,		NULL,		"pfsub",	NULL,
/* 9C */	NULL,		NULL,		"pfadd",	NULL,
/* A0 */	"pfcmpgt",	NULL,		NULL,		NULL,
/* A4 */	"pfmax",	NULL,		"pfrcpit1",	"pfrsqit1",
/* A8 */	NULL,		NULL,		"pfsubr",	NULL,
/* AC */	NULL,		NULL,		"pfacc",	NULL,
/* B0 */	"pfcmpeq",	NULL,		NULL,		NULL,
/* B4 */	"pfmul",	NULL,		"pfrcpit2",	"pfmulhrw",
/* B8 */	NULL,		NULL,		NULL,		"pswapd",
/* BC */	NULL,		NULL,		NULL,		"pavgusb",
/* C0 */	NULL,		NULL,		NULL,		NULL,
/* C4 */	NULL,		NULL,		NULL,		NULL,
/* C8 */	NULL,		NULL,		NULL,		NULL,
/* CC */	NULL,		NULL,		NULL,		NULL,
/* D0 */	NULL,		NULL,		NULL,		NULL,
/* D4 */	NULL,		NULL,		NULL,		NULL,
/* D8 */	NULL,		NULL,		NULL,		NULL,
/* DC */	NULL,		NULL,		NULL,		NULL,
/* E0 */	NULL,		NULL,		NULL,		NULL,
/* E4 */	NULL,		NULL,		NULL,		NULL,
/* E8 */	NULL,		NULL,		NULL,		NULL,
/* EC */	NULL,		NULL,		NULL,		NULL,
/* F0 */	NULL,		NULL,		NULL,		NULL,
/* F4 */	NULL,		NULL,		NULL,		NULL,
/* F8 */	NULL,		NULL,		NULL,		NULL,
/* FC */	NULL,		NULL,		NULL,		NULL,
};

static void
OP_3DNowSuffix (bytemode, sizeflag)
     int bytemode ATTRIBUTE_UNUSED;
     int sizeflag ATTRIBUTE_UNUSED;
{
  const char *mnemonic;

  FETCH_DATA (the_info, codep + 1);
  /* AMD 3DNow! instructions are specified by an opcode suffix in the
     place where an 8-bit immediate would normally go.  ie. the last
     byte of the instruction.  */
  obufp = obuf + strlen (obuf);
  mnemonic = Suffix3DNow[*codep++ & 0xff];
  if (mnemonic)
    oappend (mnemonic);
  else
    {
      /* Since a variable sized modrm/sib chunk is between the start
	 of the opcode (0x0f0f) and the opcode suffix, we need to do
	 all the modrm processing first, and don't know until now that
	 we have a bad opcode.  This necessitates some cleaning up.  */
      op1out[0] = '\0';
      op2out[0] = '\0';
      BadOp ();
    }
}

static const char *simd_cmp_op[] = {
  "eq",
  "lt",
  "le",
  "unord",
  "neq",
  "nlt",
  "nle",
  "ord"
};

static void
OP_SIMD_Suffix (bytemode, sizeflag)
     int bytemode ATTRIBUTE_UNUSED;
     int sizeflag ATTRIBUTE_UNUSED;
{
  unsigned int cmp_type;

  FETCH_DATA (the_info, codep + 1);
  obufp = obuf + strlen (obuf);
  cmp_type = *codep++ & 0xff;
  if (cmp_type < 8)
    {
      char suffix1 = 'p', suffix2 = 's';
      used_prefixes |= (prefixes & PREFIX_REPZ);
      if (prefixes & PREFIX_REPZ)
	suffix1 = 's';
      else
	{
	  used_prefixes |= (prefixes & PREFIX_DATA);
	  if (prefixes & PREFIX_DATA)
	    suffix2 = 'd';
	  else
	    {
	      used_prefixes |= (prefixes & PREFIX_REPNZ);
	      if (prefixes & PREFIX_REPNZ)
		suffix1 = 's', suffix2 = 'd';
	    }
	}
      sprintf (scratchbuf, "cmp%s%c%c",
	       simd_cmp_op[cmp_type], suffix1, suffix2);
      used_prefixes |= (prefixes & PREFIX_REPZ);
      oappend (scratchbuf);
    }
  else
    {
      /* We have a bad extension byte.  Clean up.  */
      op1out[0] = '\0';
      op2out[0] = '\0';
      BadOp ();
    }
}

static void
SIMD_Fixup (extrachar, sizeflag)
     int extrachar;
     int sizeflag ATTRIBUTE_UNUSED;
{
  /* Change movlps/movhps to movhlps/movlhps for 2 register operand
     forms of these instructions.  */
  if (mod == 3)
    {
      char *p = obuf + strlen (obuf);
      *(p + 1) = '\0';
      *p       = *(p - 1);
      *(p - 1) = *(p - 2);
      *(p - 2) = *(p - 3);
      *(p - 3) = extrachar;
    }
}

static void
BadOp (void)
{
  /* Throw away prefixes and 1st. opcode byte.  */
  codep = insn_codep + 1;
  oappend ("(bad)");
}
