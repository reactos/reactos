/* Definitions of target machine for GNU compiler for IA-32.
   Copyright (C) 1988, 1992, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
   2001, 2002 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* The purpose of this file is to define the characteristics of the i386,
   independent of assembler syntax or operating system.

   Three other files build on this one to describe a specific assembler syntax:
   bsd386.h, att386.h, and sun386.h.

   The actual tm.h file for a particular system should include
   this file, and then the file for the appropriate assembler syntax.

   Many macros that specify assembler syntax are omitted entirely from
   this file because they really belong in the files for particular
   assemblers.  These include RP, IP, LPREFIX, PUT_OP_SIZE, USE_STAR,
   ADDR_BEG, ADDR_END, PRINT_IREG, PRINT_SCALE, PRINT_B_I_S, and many
   that start with ASM_ or end in ASM_OP.  */

/* Stubs for half-pic support if not OSF/1 reference platform.  */

#ifndef HALF_PIC_P
#define HALF_PIC_P() 0
#define HALF_PIC_NUMBER_PTRS 0
#define HALF_PIC_NUMBER_REFS 0
#define HALF_PIC_ENCODE(DECL)
#define HALF_PIC_DECLARE(NAME)
#define HALF_PIC_INIT()	error ("half-pic init called on systems that don't support it")
#define HALF_PIC_ADDRESS_P(X) 0
#define HALF_PIC_PTR(X) (X)
#define HALF_PIC_FINISH(STREAM)
#endif

/* Define the specific costs for a given cpu */

struct processor_costs {
  const int add;		/* cost of an add instruction */
  const int lea;		/* cost of a lea instruction */
  const int shift_var;		/* variable shift costs */
  const int shift_const;	/* constant shift costs */
  const int mult_init;		/* cost of starting a multiply */
  const int mult_bit;		/* cost of multiply per each bit set */
  const int divide;		/* cost of a divide/mod */
  int movsx;			/* The cost of movsx operation.  */
  int movzx;			/* The cost of movzx operation.  */
  const int large_insn;		/* insns larger than this cost more */
  const int move_ratio;		/* The threshold of number of scalar
				   memory-to-memory move insns.  */
  const int movzbl_load;	/* cost of loading using movzbl */
  const int int_load[3];	/* cost of loading integer registers
				   in QImode, HImode and SImode relative
				   to reg-reg move (2).  */
  const int int_store[3];	/* cost of storing integer register
				   in QImode, HImode and SImode */
  const int fp_move;		/* cost of reg,reg fld/fst */
  const int fp_load[3];		/* cost of loading FP register
				   in SFmode, DFmode and XFmode */
  const int fp_store[3];	/* cost of storing FP register
				   in SFmode, DFmode and XFmode */
  const int mmx_move;		/* cost of moving MMX register.  */
  const int mmx_load[2];	/* cost of loading MMX register
				   in SImode and DImode */
  const int mmx_store[2];	/* cost of storing MMX register
				   in SImode and DImode */
  const int sse_move;		/* cost of moving SSE register.  */
  const int sse_load[3];	/* cost of loading SSE register
				   in SImode, DImode and TImode*/
  const int sse_store[3];	/* cost of storing SSE register
				   in SImode, DImode and TImode*/
  const int mmxsse_to_integer;	/* cost of moving mmxsse register to
				   integer and vice versa.  */
  const int prefetch_block;	/* bytes moved to cache for prefetch.  */
  const int simultaneous_prefetches; /* number of parallel prefetch
				   operations.  */
};

extern const struct processor_costs *ix86_cost;

/* Run-time compilation parameters selecting different hardware subsets.  */

extern int target_flags;

/* Macros used in the machine description to test the flags.  */

/* configure can arrange to make this 2, to force a 486.  */

#ifndef TARGET_CPU_DEFAULT
#define TARGET_CPU_DEFAULT 0
#endif

/* Masks for the -m switches */
#define MASK_80387		0x00000001	/* Hardware floating point */
#define MASK_RTD		0x00000002	/* Use ret that pops args */
#define MASK_ALIGN_DOUBLE	0x00000004	/* align doubles to 2 word boundary */
#define MASK_SVR3_SHLIB		0x00000008	/* Uninit locals into bss */
#define MASK_IEEE_FP		0x00000010	/* IEEE fp comparisons */
#define MASK_FLOAT_RETURNS	0x00000020	/* Return float in st(0) */
#define MASK_NO_FANCY_MATH_387	0x00000040	/* Disable sin, cos, sqrt */
#define MASK_OMIT_LEAF_FRAME_POINTER 0x080      /* omit leaf frame pointers */
#define MASK_STACK_PROBE	0x00000100	/* Enable stack probing */
#define MASK_NO_ALIGN_STROPS	0x00000200	/* Enable aligning of string ops.  */
#define MASK_INLINE_ALL_STROPS	0x00000400	/* Inline stringops in all cases */
#define MASK_NO_PUSH_ARGS	0x00000800	/* Use push instructions */
#define MASK_ACCUMULATE_OUTGOING_ARGS 0x00001000/* Accumulate outgoing args */
#define MASK_ACCUMULATE_OUTGOING_ARGS_SET 0x00002000
#define MASK_MMX		0x00004000	/* Support MMX regs/builtins */
#define MASK_MMX_SET		0x00008000
#define MASK_SSE		0x00010000	/* Support SSE regs/builtins */
#define MASK_SSE_SET		0x00020000
#define MASK_SSE2		0x00040000	/* Support SSE2 regs/builtins */
#define MASK_SSE2_SET		0x00080000
#define MASK_3DNOW		0x00100000	/* Support 3Dnow builtins */
#define MASK_3DNOW_SET		0x00200000
#define MASK_3DNOW_A		0x00400000	/* Support Athlon 3Dnow builtins */
#define MASK_3DNOW_A_SET	0x00800000
#define MASK_128BIT_LONG_DOUBLE 0x01000000	/* long double size is 128bit */
#define MASK_64BIT		0x02000000	/* Produce 64bit code */
/* ... overlap with subtarget options starts by 0x04000000.  */
#define MASK_NO_RED_ZONE	0x04000000	/* Do not use red zone */

/* Use the floating point instructions */
#define TARGET_80387 (target_flags & MASK_80387)

/* Compile using ret insn that pops args.
   This will not work unless you use prototypes at least
   for all functions that can take varying numbers of args.  */  
#define TARGET_RTD (target_flags & MASK_RTD)

/* Align doubles to a two word boundary.  This breaks compatibility with
   the published ABI's for structures containing doubles, but produces
   faster code on the pentium.  */
#define TARGET_ALIGN_DOUBLE (target_flags & MASK_ALIGN_DOUBLE)

/* Use push instructions to save outgoing args.  */
#define TARGET_PUSH_ARGS (!(target_flags & MASK_NO_PUSH_ARGS))

/* Accumulate stack adjustments to prologue/epilogue.  */
#define TARGET_ACCUMULATE_OUTGOING_ARGS \
 (target_flags & MASK_ACCUMULATE_OUTGOING_ARGS)

/* Put uninitialized locals into bss, not data.
   Meaningful only on svr3.  */
#define TARGET_SVR3_SHLIB (target_flags & MASK_SVR3_SHLIB)

/* Use IEEE floating point comparisons.  These handle correctly the cases
   where the result of a comparison is unordered.  Normally SIGFPE is
   generated in such cases, in which case this isn't needed.  */
#define TARGET_IEEE_FP (target_flags & MASK_IEEE_FP)

/* Functions that return a floating point value may return that value
   in the 387 FPU or in 386 integer registers.  If set, this flag causes
   the 387 to be used, which is compatible with most calling conventions.  */
#define TARGET_FLOAT_RETURNS_IN_80387 (target_flags & MASK_FLOAT_RETURNS)

/* Long double is 128bit instead of 96bit, even when only 80bits are used.
   This mode wastes cache, but avoid misaligned data accesses and simplifies
   address calculations.  */
#define TARGET_128BIT_LONG_DOUBLE (target_flags & MASK_128BIT_LONG_DOUBLE)

/* Disable generation of FP sin, cos and sqrt operations for 387.
   This is because FreeBSD lacks these in the math-emulator-code */
#define TARGET_NO_FANCY_MATH_387 (target_flags & MASK_NO_FANCY_MATH_387)

/* Don't create frame pointers for leaf functions */
#define TARGET_OMIT_LEAF_FRAME_POINTER \
  (target_flags & MASK_OMIT_LEAF_FRAME_POINTER)

/* Debug GO_IF_LEGITIMATE_ADDRESS */
#define TARGET_DEBUG_ADDR (ix86_debug_addr_string != 0)

/* Debug FUNCTION_ARG macros */
#define TARGET_DEBUG_ARG (ix86_debug_arg_string != 0)

/* 64bit Sledgehammer mode */
#ifdef TARGET_BI_ARCH
#define TARGET_64BIT (target_flags & MASK_64BIT)
#else
#ifdef TARGET_64BIT_DEFAULT
#define TARGET_64BIT 1
#else
#define TARGET_64BIT 0
#endif
#endif

#define TARGET_386 (ix86_cpu == PROCESSOR_I386)
#define TARGET_486 (ix86_cpu == PROCESSOR_I486)
#define TARGET_PENTIUM (ix86_cpu == PROCESSOR_PENTIUM)
#define TARGET_PENTIUMPRO (ix86_cpu == PROCESSOR_PENTIUMPRO)
#define TARGET_K6 (ix86_cpu == PROCESSOR_K6)
#define TARGET_ATHLON (ix86_cpu == PROCESSOR_ATHLON)
#define TARGET_PENTIUM4 (ix86_cpu == PROCESSOR_PENTIUM4)

#define CPUMASK (1 << ix86_cpu)
extern const int x86_use_leave, x86_push_memory, x86_zero_extend_with_and;
extern const int x86_use_bit_test, x86_cmove, x86_deep_branch;
extern const int x86_branch_hints, x86_unroll_strlen;
extern const int x86_double_with_add, x86_partial_reg_stall, x86_movx;
extern const int x86_use_loop, x86_use_fiop, x86_use_mov0;
extern const int x86_use_cltd, x86_read_modify_write;
extern const int x86_read_modify, x86_split_long_moves;
extern const int x86_promote_QImode, x86_single_stringop;
extern const int x86_himode_math, x86_qimode_math, x86_promote_qi_regs;
extern const int x86_promote_hi_regs, x86_integer_DFmode_moves;
extern const int x86_add_esp_4, x86_add_esp_8, x86_sub_esp_4, x86_sub_esp_8;
extern const int x86_partial_reg_dependency, x86_memory_mismatch_stall;
extern const int x86_accumulate_outgoing_args, x86_prologue_using_move;
extern const int x86_epilogue_using_move, x86_decompose_lea;
extern const int x86_arch_always_fancy_math_387;
extern int x86_prefetch_sse;

#define TARGET_USE_LEAVE (x86_use_leave & CPUMASK)
#define TARGET_PUSH_MEMORY (x86_push_memory & CPUMASK)
#define TARGET_ZERO_EXTEND_WITH_AND (x86_zero_extend_with_and & CPUMASK)
#define TARGET_USE_BIT_TEST (x86_use_bit_test & CPUMASK)
#define TARGET_UNROLL_STRLEN (x86_unroll_strlen & CPUMASK)
/* For sane SSE instruction set generation we need fcomi instruction.  It is
   safe to enable all CMOVE instructions.  */
#define TARGET_CMOVE ((x86_cmove & (1 << ix86_arch)) || TARGET_SSE)
#define TARGET_DEEP_BRANCH_PREDICTION (x86_deep_branch & CPUMASK)
#define TARGET_BRANCH_PREDICTION_HINTS (x86_branch_hints & CPUMASK)
#define TARGET_DOUBLE_WITH_ADD (x86_double_with_add & CPUMASK)
#define TARGET_USE_SAHF ((x86_use_sahf & CPUMASK) && !TARGET_64BIT)
#define TARGET_MOVX (x86_movx & CPUMASK)
#define TARGET_PARTIAL_REG_STALL (x86_partial_reg_stall & CPUMASK)
#define TARGET_USE_LOOP (x86_use_loop & CPUMASK)
#define TARGET_USE_FIOP (x86_use_fiop & CPUMASK)
#define TARGET_USE_MOV0 (x86_use_mov0 & CPUMASK)
#define TARGET_USE_CLTD (x86_use_cltd & CPUMASK)
#define TARGET_SPLIT_LONG_MOVES (x86_split_long_moves & CPUMASK)
#define TARGET_READ_MODIFY_WRITE (x86_read_modify_write & CPUMASK)
#define TARGET_READ_MODIFY (x86_read_modify & CPUMASK)
#define TARGET_PROMOTE_QImode (x86_promote_QImode & CPUMASK)
#define TARGET_SINGLE_STRINGOP (x86_single_stringop & CPUMASK)
#define TARGET_QIMODE_MATH (x86_qimode_math & CPUMASK)
#define TARGET_HIMODE_MATH (x86_himode_math & CPUMASK)
#define TARGET_PROMOTE_QI_REGS (x86_promote_qi_regs & CPUMASK)
#define TARGET_PROMOTE_HI_REGS (x86_promote_hi_regs & CPUMASK)
#define TARGET_ADD_ESP_4 (x86_add_esp_4 & CPUMASK)
#define TARGET_ADD_ESP_8 (x86_add_esp_8 & CPUMASK)
#define TARGET_SUB_ESP_4 (x86_sub_esp_4 & CPUMASK)
#define TARGET_SUB_ESP_8 (x86_sub_esp_8 & CPUMASK)
#define TARGET_INTEGER_DFMODE_MOVES (x86_integer_DFmode_moves & CPUMASK)
#define TARGET_PARTIAL_REG_DEPENDENCY (x86_partial_reg_dependency & CPUMASK)
#define TARGET_MEMORY_MISMATCH_STALL (x86_memory_mismatch_stall & CPUMASK)
#define TARGET_PROLOGUE_USING_MOVE (x86_prologue_using_move & CPUMASK)
#define TARGET_EPILOGUE_USING_MOVE (x86_epilogue_using_move & CPUMASK)
#define TARGET_DECOMPOSE_LEA (x86_decompose_lea & CPUMASK)
#define TARGET_PREFETCH_SSE (x86_prefetch_sse)

#define TARGET_STACK_PROBE (target_flags & MASK_STACK_PROBE)

#define TARGET_ALIGN_STRINGOPS (!(target_flags & MASK_NO_ALIGN_STROPS))
#define TARGET_INLINE_ALL_STRINGOPS (target_flags & MASK_INLINE_ALL_STROPS)

#define ASSEMBLER_DIALECT (ix86_asm_dialect)

#define TARGET_SSE ((target_flags & (MASK_SSE | MASK_SSE2)) != 0)
#define TARGET_SSE2 ((target_flags & MASK_SSE2) != 0)
#define TARGET_SSE_MATH ((ix86_fpmath & FPMATH_SSE) != 0)
#define TARGET_MIX_SSE_I387 ((ix86_fpmath & FPMATH_SSE) \
			     && (ix86_fpmath & FPMATH_387))
#define TARGET_MMX ((target_flags & MASK_MMX) != 0)
#define TARGET_3DNOW ((target_flags & MASK_3DNOW) != 0)
#define TARGET_3DNOW_A ((target_flags & MASK_3DNOW_A) != 0)

#define TARGET_RED_ZONE (!(target_flags & MASK_NO_RED_ZONE))

/* WARNING: Do not mark empty strings for translation, as calling
            gettext on an empty string does NOT return an empty
            string. */


#define TARGET_SWITCHES							      \
{ { "80387",			 MASK_80387, N_("Use hardware fp") },	      \
  { "no-80387",			-MASK_80387, N_("Do not use hardware fp") },  \
  { "hard-float",		 MASK_80387, N_("Use hardware fp") },	      \
  { "soft-float",		-MASK_80387, N_("Do not use hardware fp") },  \
  { "no-soft-float",		 MASK_80387, N_("Use hardware fp") },	      \
  { "386",			 0, "" /*Deprecated.*/},		      \
  { "486",			 0, "" /*Deprecated.*/},		      \
  { "pentium",			 0, "" /*Deprecated.*/},		      \
  { "pentiumpro",		 0, "" /*Deprecated.*/},		      \
  { "intel-syntax",		 0, "" /*Deprecated.*/},	 	      \
  { "no-intel-syntax",		 0, "" /*Deprecated.*/},	 	      \
  { "rtd",			 MASK_RTD,				      \
    N_("Alternate calling convention") },				      \
  { "no-rtd",			-MASK_RTD,				      \
    N_("Use normal calling convention") },				      \
  { "align-double",		 MASK_ALIGN_DOUBLE,			      \
    N_("Align some doubles on dword boundary") },			      \
  { "no-align-double",		-MASK_ALIGN_DOUBLE,			      \
    N_("Align doubles on word boundary") },				      \
  { "svr3-shlib",		 MASK_SVR3_SHLIB,			      \
    N_("Uninitialized locals in .bss")  },				      \
  { "no-svr3-shlib",		-MASK_SVR3_SHLIB,			      \
    N_("Uninitialized locals in .data") },				      \
  { "ieee-fp",			 MASK_IEEE_FP,				      \
    N_("Use IEEE math for fp comparisons") },				      \
  { "no-ieee-fp",		-MASK_IEEE_FP,				      \
    N_("Do not use IEEE math for fp comparisons") },			      \
  { "fp-ret-in-387",		 MASK_FLOAT_RETURNS,			      \
    N_("Return values of functions in FPU registers") },		      \
  { "no-fp-ret-in-387",		-MASK_FLOAT_RETURNS ,			      \
    N_("Do not return values of functions in FPU registers")},		      \
  { "no-fancy-math-387",	 MASK_NO_FANCY_MATH_387,		      \
    N_("Do not generate sin, cos, sqrt for FPU") },			      \
  { "fancy-math-387",		-MASK_NO_FANCY_MATH_387,		      \
     N_("Generate sin, cos, sqrt for FPU")},				      \
  { "omit-leaf-frame-pointer",	 MASK_OMIT_LEAF_FRAME_POINTER,		      \
    N_("Omit the frame pointer in leaf functions") },			      \
  { "no-omit-leaf-frame-pointer",-MASK_OMIT_LEAF_FRAME_POINTER, "" },	      \
  { "stack-arg-probe",		 MASK_STACK_PROBE,			      \
    N_("Enable stack probing") },					      \
  { "no-stack-arg-probe",	-MASK_STACK_PROBE, "" },		      \
  { "windows",			0, 0 /* undocumented */ },		      \
  { "dll",			0,  0 /* undocumented */ },		      \
  { "align-stringops",		-MASK_NO_ALIGN_STROPS,			      \
    N_("Align destination of the string operations") },			      \
  { "no-align-stringops",	 MASK_NO_ALIGN_STROPS,			      \
    N_("Do not align destination of the string operations") },		      \
  { "inline-all-stringops",	 MASK_INLINE_ALL_STROPS,		      \
    N_("Inline all known string operations") },				      \
  { "no-inline-all-stringops",	-MASK_INLINE_ALL_STROPS,		      \
    N_("Do not inline all known string operations") },			      \
  { "push-args",		-MASK_NO_PUSH_ARGS,			      \
    N_("Use push instructions to save outgoing arguments") },		      \
  { "no-push-args",		MASK_NO_PUSH_ARGS,			      \
    N_("Do not use push instructions to save outgoing arguments") },	      \
  { "accumulate-outgoing-args",	(MASK_ACCUMULATE_OUTGOING_ARGS		      \
				 | MASK_ACCUMULATE_OUTGOING_ARGS_SET),	      \
    N_("Use push instructions to save outgoing arguments") },		      \
  { "no-accumulate-outgoing-args",MASK_ACCUMULATE_OUTGOING_ARGS_SET,	      \
    N_("Do not use push instructions to save outgoing arguments") },	      \
  { "mmx",			 MASK_MMX | MASK_MMX_SET,		      \
    N_("Support MMX built-in functions") },				      \
  { "no-mmx",			 -MASK_MMX,				      \
    N_("Do not support MMX built-in functions") },			      \
  { "no-mmx",			 MASK_MMX_SET, "" },			      \
  { "3dnow",                     MASK_3DNOW | MASK_3DNOW_SET,		      \
    N_("Support 3DNow! built-in functions") },				      \
  { "no-3dnow",                  -MASK_3DNOW, "" },			      \
  { "no-3dnow",                  MASK_3DNOW_SET,			      \
    N_("Do not support 3DNow! built-in functions") },			      \
  { "sse",			 MASK_SSE | MASK_SSE_SET,		      \
    N_("Support MMX and SSE built-in functions and code generation") },	      \
  { "no-sse",			 -MASK_SSE, "" },	 		      \
  { "no-sse",			 MASK_SSE_SET,				      \
    N_("Do not support MMX and SSE built-in functions and code generation") },\
  { "sse2",			 MASK_SSE2 | MASK_SSE2_SET,		      \
    N_("Support MMX, SSE and SSE2 built-in functions and code generation") }, \
  { "no-sse2",			 -MASK_SSE2, "" },			      \
  { "no-sse2",			 MASK_SSE2_SET,				      \
    N_("Do not support MMX, SSE and SSE2 built-in functions and code generation") },    \
  { "128bit-long-double",	 MASK_128BIT_LONG_DOUBLE,		      \
    N_("sizeof(long double) is 16") },					      \
  { "96bit-long-double",	-MASK_128BIT_LONG_DOUBLE,		      \
    N_("sizeof(long double) is 12") },					      \
  { "64",			MASK_64BIT,				      \
    N_("Generate 64bit x86-64 code") },					      \
  { "32",			-MASK_64BIT,				      \
    N_("Generate 32bit i386 code") },					      \
  { "red-zone",			-MASK_NO_RED_ZONE,			      \
    N_("Use red-zone in the x86-64 code") },				      \
  { "no-red-zone",		MASK_NO_RED_ZONE,			      \
    N_("Do not use red-zone in the x86-64 code") },			      \
  SUBTARGET_SWITCHES							      \
  { "", TARGET_DEFAULT, 0 }}

#ifdef TARGET_64BIT_DEFAULT
#define TARGET_DEFAULT (MASK_64BIT | TARGET_SUBTARGET_DEFAULT)
#else
#define TARGET_DEFAULT TARGET_SUBTARGET_DEFAULT
#endif

/* Which processor to schedule for. The cpu attribute defines a list that
   mirrors this list, so changes to i386.md must be made at the same time.  */

enum processor_type
{
  PROCESSOR_I386,			/* 80386 */
  PROCESSOR_I486,			/* 80486DX, 80486SX, 80486DX[24] */
  PROCESSOR_PENTIUM,
  PROCESSOR_PENTIUMPRO,
  PROCESSOR_K6,
  PROCESSOR_ATHLON,
  PROCESSOR_PENTIUM4,
  PROCESSOR_max
};
enum fpmath_unit
{
  FPMATH_387 = 1,
  FPMATH_SSE = 2
};

extern enum processor_type ix86_cpu;
extern enum fpmath_unit ix86_fpmath;

extern int ix86_arch;

/* This macro is similar to `TARGET_SWITCHES' but defines names of
   command options that have values.  Its definition is an
   initializer with a subgrouping for each command option.

   Each subgrouping contains a string constant, that defines the
   fixed part of the option name, and the address of a variable.  The
   variable, type `char *', is set to the variable part of the given
   option if the fixed part matches.  The actual option name is made
   by appending `-m' to the specified name.  */
#define TARGET_OPTIONS						\
{ { "cpu=",		&ix86_cpu_string,			\
    N_("Schedule code for given CPU")},				\
  { "fpmath=",		&ix86_fpmath_string,			\
    N_("Generate floating point mathematics using given instruction set")},\
  { "arch=",		&ix86_arch_string,			\
    N_("Generate code for given CPU")},				\
  { "regparm=",		&ix86_regparm_string,			\
    N_("Number of registers used to pass integer arguments") },	\
  { "align-loops=",	&ix86_align_loops_string,		\
    N_("Loop code aligned to this power of 2") },		\
  { "align-jumps=",	&ix86_align_jumps_string,		\
    N_("Jump targets are aligned to this power of 2") },	\
  { "align-functions=",	&ix86_align_funcs_string,		\
    N_("Function starts are aligned to this power of 2") },	\
  { "preferred-stack-boundary=",				\
    &ix86_preferred_stack_boundary_string,			\
    N_("Attempt to keep stack aligned to this power of 2") },	\
  { "branch-cost=",	&ix86_branch_cost_string,		\
    N_("Branches are this expensive (1-5, arbitrary units)") },	\
  { "cmodel=", &ix86_cmodel_string,				\
    N_("Use given x86-64 code model") },			\
  { "debug-arg", &ix86_debug_arg_string,			\
    "" /* Undocumented. */ },					\
  { "debug-addr", &ix86_debug_addr_string,			\
    "" /* Undocumented. */ },					\
  { "asm=", &ix86_asm_string,					\
    N_("Use given assembler dialect") },			\
  SUBTARGET_OPTIONS						\
}

/* Sometimes certain combinations of command options do not make
   sense on a particular target machine.  You can define a macro
   `OVERRIDE_OPTIONS' to take account of this.  This macro, if
   defined, is executed once just after all the command options have
   been parsed.

   Don't use this macro to turn on various extra optimizations for
   `-O'.  That is what `OPTIMIZATION_OPTIONS' is for.  */

#define OVERRIDE_OPTIONS override_options ()

/* These are meant to be redefined in the host dependent files */
#define SUBTARGET_SWITCHES
#define SUBTARGET_OPTIONS

/* Define this to change the optimizations performed by default.  */
#define OPTIMIZATION_OPTIONS(LEVEL, SIZE) \
  optimization_options ((LEVEL), (SIZE))

/* Specs for the compiler proper */

#ifndef CC1_CPU_SPEC
#define CC1_CPU_SPEC "\
%{!mcpu*: \
%{m386:-mcpu=i386 \
%n`-m386' is deprecated. Use `-march=i386' or `-mcpu=i386' instead.\n} \
%{m486:-mcpu=i486 \
%n`-m486' is deprecated. Use `-march=i486' or `-mcpu=i486' instead.\n} \
%{mpentium:-mcpu=pentium \
%n`-mpentium' is deprecated. Use `-march=pentium' or `-mcpu=pentium' instead.\n} \
%{mpentiumpro:-mcpu=pentiumpro \
%n`-mpentiumpro' is deprecated. Use `-march=pentiumpro' or `-mcpu=pentiumpro' instead.\n}} \
%{mintel-syntax:-masm=intel \
%n`-mintel-syntax' is deprecated. Use `-masm=intel' instead.\n} \
%{mno-intel-syntax:-masm=att \
%n`-mno-intel-syntax' is deprecated. Use `-masm=att' instead.\n}"
#endif

#define TARGET_CPU_DEFAULT_i386 0
#define TARGET_CPU_DEFAULT_i486 1
#define TARGET_CPU_DEFAULT_pentium 2
#define TARGET_CPU_DEFAULT_pentium_mmx 3
#define TARGET_CPU_DEFAULT_pentiumpro 4
#define TARGET_CPU_DEFAULT_pentium2 5
#define TARGET_CPU_DEFAULT_pentium3 6
#define TARGET_CPU_DEFAULT_pentium4 7
#define TARGET_CPU_DEFAULT_k6 8
#define TARGET_CPU_DEFAULT_k6_2 9
#define TARGET_CPU_DEFAULT_k6_3 10
#define TARGET_CPU_DEFAULT_athlon 11
#define TARGET_CPU_DEFAULT_athlon_sse 12

#define TARGET_CPU_DEFAULT_NAMES {"i386", "i486", "pentium", "pentium-mmx",\
				  "pentiumpro", "pentium2", "pentium3", \
				  "pentium4", "k6", "k6-2", "k6-3",\
				  "athlon", "athlon-4"}
#ifndef CPP_CPU_DEFAULT_SPEC
#if TARGET_CPU_DEFAULT == TARGET_CPU_DEFAULT_i486
#define CPP_CPU_DEFAULT_SPEC "-D__tune_i486__"
#endif
#if TARGET_CPU_DEFAULT == TARGET_CPU_DEFAULT_pentium
#define CPP_CPU_DEFAULT_SPEC "-D__tune_i586__ -D__tune_pentium__"
#endif
#if TARGET_CPU_DEFAULT == TARGET_CPU_DEFAULT_pentium_mmx
#define CPP_CPU_DEFAULT_SPEC "-D__tune_i586__ -D__tune_pentium__ -D__tune_pentium_mmx__"
#endif
#if TARGET_CPU_DEFAULT == TARGET_CPU_DEFAULT_pentiumpro
#define CPP_CPU_DEFAULT_SPEC "-D__tune_i686__ -D__tune_pentiumpro__"
#endif
#if TARGET_CPU_DEFAULT == TARGET_CPU_DEFAULT_pentium2
#define CPP_CPU_DEFAULT_SPEC "-D__tune_i686__ -D__tune_pentiumpro__\
-D__tune_pentium2__"
#endif
#if TARGET_CPU_DEFAULT == TARGET_CPU_DEFAULT_pentium3
#define CPP_CPU_DEFAULT_SPEC "-D__tune_i686__ -D__tune_pentiumpro__\
-D__tune_pentium2__ -D__tune_pentium3__"
#endif
#if TARGET_CPU_DEFAULT == TARGET_CPU_DEFAULT_pentium4
#define CPP_CPU_DEFAULT_SPEC "-D__tune_pentium4__"
#endif
#if TARGET_CPU_DEFAULT == TARGET_CPU_DEFAULT_k6
#define CPP_CPU_DEFAULT_SPEC "-D__tune_k6__"
#endif
#if TARGET_CPU_DEFAULT == TARGET_CPU_DEFAULT_k6_2
#define CPP_CPU_DEFAULT_SPEC "-D__tune_k6__ -D__tune_k6_2__"
#endif
#if TARGET_CPU_DEFAULT == TARGET_CPU_DEFAULT_k6_3
#define CPP_CPU_DEFAULT_SPEC "-D__tune_k6__ -D__tune_k6_3__"
#endif
#if TARGET_CPU_DEFAULT == TARGET_CPU_DEFAULT_athlon
#define CPP_CPU_DEFAULT_SPEC "-D__tune_athlon__"
#endif
#if TARGET_CPU_DEFAULT == TARGET_CPU_DEFAULT_athlon_sse
#define CPP_CPU_DEFAULT_SPEC "-D__tune_athlon__ -D__tune_athlon_sse__"
#endif
#ifndef CPP_CPU_DEFAULT_SPEC
#define CPP_CPU_DEFAULT_SPEC "-D__tune_i386__"
#endif
#endif /* CPP_CPU_DEFAULT_SPEC */

#ifdef TARGET_BI_ARCH
#define NO_BUILTIN_SIZE_TYPE
#define NO_BUILTIN_PTRDIFF_TYPE
#endif

#ifdef NO_BUILTIN_SIZE_TYPE
#define CPP_CPU32_SIZE_TYPE_SPEC \
  " -D__SIZE_TYPE__=unsigned\\ int -D__PTRDIFF_TYPE__=int"
#define CPP_CPU64_SIZE_TYPE_SPEC \
  " -D__SIZE_TYPE__=unsigned\\ long\\ int -D__PTRDIFF_TYPE__=long\\ int"
#else
#define CPP_CPU32_SIZE_TYPE_SPEC ""
#define CPP_CPU64_SIZE_TYPE_SPEC ""
#endif

#define CPP_CPU32_SPEC \
  "-Acpu=i386 -Amachine=i386 %{!ansi:%{!std=c*:%{!std=i*:-Di386}}} -D__i386 \
-D__i386__ %(cpp_cpu32sizet)"

#define CPP_CPU64_SPEC \
  "-Acpu=x86_64 -Amachine=x86_64 -D__x86_64 -D__x86_64__ %(cpp_cpu64sizet)"

#define CPP_CPUCOMMON_SPEC "\
%{march=i386:%{!mcpu*:-D__tune_i386__ }}\
%{march=i486:-D__i486 -D__i486__ %{!mcpu*:-D__tune_i486__ }}\
%{march=pentium|march=i586:-D__i586 -D__i586__ -D__pentium -D__pentium__ \
  %{!mcpu*:-D__tune_i586__ -D__tune_pentium__ }}\
%{march=pentium-mmx:-D__i586 -D__i586__ -D__pentium -D__pentium__ \
  -D__pentium__mmx__ \
  %{!mcpu*:-D__tune_i586__ -D__tune_pentium__ -D__tune_pentium_mmx__}}\
%{march=pentiumpro|march=i686:-D__i686 -D__i686__ \
  -D__pentiumpro -D__pentiumpro__ \
  %{!mcpu*:-D__tune_i686__ -D__tune_pentiumpro__ }}\
%{march=k6:-D__k6 -D__k6__ %{!mcpu*:-D__tune_k6__ }}\
%{march=k6-2:-D__k6 -D__k6__ -D__k6_2__ \
  %{!mcpu*:-D__tune_k6__ -D__tune_k6_2__ }}\
%{march=k6-3:-D__k6 -D__k6__ -D__k6_3__ \
  %{!mcpu*:-D__tune_k6__ -D__tune_k6_3__ }}\
%{march=athlon|march=athlon-tbird:-D__athlon -D__athlon__ \
  %{!mcpu*:-D__tune_athlon__ }}\
%{march=athlon-4|march=athlon-xp|march=athlon-mp:-D__athlon -D__athlon__ \
  -D__athlon_sse__ \
  %{!mcpu*:-D__tune_athlon__ -D__tune_athlon_sse__ }}\
%{march=pentium4:-D__pentium4 -D__pentium4__ %{!mcpu*:-D__tune_pentium4__ }}\
%{m386|mcpu=i386:-D__tune_i386__ }\
%{m486|mcpu=i486:-D__tune_i486__ }\
%{mpentium|mcpu=pentium|mcpu=i586|mcpu=pentium-mmx:-D__tune_i586__ -D__tune_pentium__ }\
%{mpentiumpro|mcpu=pentiumpro|mcpu=i686|cpu=pentium2|cpu=pentium3:-D__tune_i686__ \
-D__tune_pentiumpro__ }\
%{mcpu=k6|mcpu=k6-2|mcpu=k6-3:-D__tune_k6__ }\
%{mcpu=athlon|mcpu=athlon-tbird|mcpu=athlon-4|mcpu=athlon-xp|mcpu=athlon-mp:\
-D__tune_athlon__ }\
%{mcpu=athlon-4|mcpu=athlon-xp|mcpu=athlon-mp:\
-D__tune_athlon_sse__ }\
%{mcpu=pentium4:-D__tune_pentium4__ }\
%{march=athlon-tbird|march=athlon-xp|march=athlon-mp|march=pentium3|march=pentium4:\
-D__SSE__ }\
%{march=pentium-mmx|march=k6|march=k6-2|march=k6-3\
|march=athlon|march=athlon-tbird|march=athlon-4|march=athlon-xp\
|march=athlon-mp|march=pentium2|march=pentium3|march=pentium4: -D__MMX__ }\
%{march=k6-2|march=k6-3\
|march=athlon|march=athlon-tbird|march=athlon-4|march=athlon-xp\
|march=athlon-mp: -D__3dNOW__ }\
%{march=athlon|march=athlon-tbird|march=athlon-4|march=athlon-xp\
|march=athlon-mp: -D__3dNOW_A__ }\
%{march=pentium4: -D__SSE2__ }\
%{!march*:%{!mcpu*:%{!m386:%{!m486:%{!mpentium*:%(cpp_cpu_default)}}}}}"

#ifndef CPP_CPU_SPEC
#ifdef TARGET_BI_ARCH
#ifdef TARGET_64BIT_DEFAULT
#define CPP_CPU_SPEC "%{m32:%(cpp_cpu32)}%{!m32:%(cpp_cpu64)} %(cpp_cpucommon)"
#else
#define CPP_CPU_SPEC "%{m64:%(cpp_cpu64)}%{!m64:%(cpp_cpu32)} %(cpp_cpucommon)"
#endif
#else
#ifdef TARGET_64BIT_DEFAULT
#define CPP_CPU_SPEC "%(cpp_cpu64) %(cpp_cpucommon)"
#else
#define CPP_CPU_SPEC "%(cpp_cpu32) %(cpp_cpucommon)"
#endif
#endif
#endif

#ifndef CC1_SPEC
#define CC1_SPEC "%(cc1_cpu) "
#endif

/* This macro defines names of additional specifications to put in the
   specs that can be used in various specifications like CC1_SPEC.  Its
   definition is an initializer with a subgrouping for each command option.

   Each subgrouping contains a string constant, that defines the
   specification name, and a string constant that used by the GNU CC driver
   program.

   Do not define this macro if it does not need to do anything.  */

#ifndef SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS
#endif

#define EXTRA_SPECS							\
  { "cpp_cpu_default",	CPP_CPU_DEFAULT_SPEC },				\
  { "cpp_cpu",	CPP_CPU_SPEC },						\
  { "cpp_cpu32", CPP_CPU32_SPEC },					\
  { "cpp_cpu64", CPP_CPU64_SPEC },					\
  { "cpp_cpu32sizet", CPP_CPU32_SIZE_TYPE_SPEC },			\
  { "cpp_cpu64sizet", CPP_CPU64_SIZE_TYPE_SPEC },			\
  { "cpp_cpucommon", CPP_CPUCOMMON_SPEC },				\
  { "cc1_cpu",  CC1_CPU_SPEC },						\
  SUBTARGET_EXTRA_SPECS

/* target machine storage layout */

/* Define for XFmode or TFmode extended real floating point support.
   This will automatically cause REAL_ARITHMETIC to be defined.
 
   The XFmode is specified by i386 ABI, while TFmode may be faster
   due to alignment and simplifications in the address calculations.
 */
#define LONG_DOUBLE_TYPE_SIZE (TARGET_128BIT_LONG_DOUBLE ? 128 : 96)
#define MAX_LONG_DOUBLE_TYPE_SIZE 128
#ifdef __x86_64__
#define LIBGCC2_LONG_DOUBLE_TYPE_SIZE 128
#else
#define LIBGCC2_LONG_DOUBLE_TYPE_SIZE 96
#endif
/* Tell real.c that this is the 80-bit Intel extended float format
   packaged in a 128-bit or 96bit entity.  */
#define INTEL_EXTENDED_IEEE_FORMAT 1


#define SHORT_TYPE_SIZE 16
#define INT_TYPE_SIZE 32
#define FLOAT_TYPE_SIZE 32
#define LONG_TYPE_SIZE BITS_PER_WORD
#define MAX_WCHAR_TYPE_SIZE 32
#define DOUBLE_TYPE_SIZE 64
#define LONG_LONG_TYPE_SIZE 64

#if defined (TARGET_BI_ARCH) || defined (TARGET_64BIT_DEFAULT)
#define MAX_BITS_PER_WORD 64
#define MAX_LONG_TYPE_SIZE 64
#else
#define MAX_BITS_PER_WORD 32
#define MAX_LONG_TYPE_SIZE 32
#endif

/* Define if you don't want extended real, but do want to use the
   software floating point emulator for REAL_ARITHMETIC and
   decimal <-> binary conversion.  */
/* #define REAL_ARITHMETIC */

/* Define this if most significant byte of a word is the lowest numbered.  */
/* That is true on the 80386.  */

#define BITS_BIG_ENDIAN 0

/* Define this if most significant byte of a word is the lowest numbered.  */
/* That is not true on the 80386.  */
#define BYTES_BIG_ENDIAN 0

/* Define this if most significant word of a multiword number is the lowest
   numbered.  */
/* Not true for 80386 */
#define WORDS_BIG_ENDIAN 0

/* number of bits in an addressable storage unit */
#define BITS_PER_UNIT 8

/* Width in bits of a "word", which is the contents of a machine register.
   Note that this is not necessarily the width of data type `int';
   if using 16-bit ints on a 80386, this would still be 32.
   But on a machine with 16-bit registers, this would be 16.  */
#define BITS_PER_WORD (TARGET_64BIT ? 64 : 32)

/* Width of a word, in units (bytes).  */
#define UNITS_PER_WORD (TARGET_64BIT ? 8 : 4)
#define MIN_UNITS_PER_WORD 4

/* Width in bits of a pointer.
   See also the macro `Pmode' defined below.  */
#define POINTER_SIZE BITS_PER_WORD

/* Allocation boundary (in *bits*) for storing arguments in argument list.  */
#define PARM_BOUNDARY BITS_PER_WORD

/* Boundary (in *bits*) on which stack pointer should be aligned.  */
#define STACK_BOUNDARY BITS_PER_WORD

/* Boundary (in *bits*) on which the stack pointer preferrs to be
   aligned; the compiler cannot rely on having this alignment.  */
#define PREFERRED_STACK_BOUNDARY ix86_preferred_stack_boundary

/* As of July 2001, many runtimes to not align the stack properly when
   entering main.  This causes expand_main_function to forcably align
   the stack, which results in aligned frames for functions called from
   main, though it does nothing for the alignment of main itself.  */
#define FORCE_PREFERRED_STACK_BOUNDARY_IN_MAIN \
  (ix86_preferred_stack_boundary > STACK_BOUNDARY && !TARGET_64BIT)

/* Allocation boundary for the code of a function.  */
#define FUNCTION_BOUNDARY 16

/* Alignment of field after `int : 0' in a structure.  */

#define EMPTY_FIELD_BOUNDARY BITS_PER_WORD

/* Minimum size in bits of the largest boundary to which any
   and all fundamental data types supported by the hardware
   might need to be aligned. No data type wants to be aligned
   rounder than this.
   
   Pentium+ preferrs DFmode values to be aligned to 64 bit boundary
   and Pentium Pro XFmode values at 128 bit boundaries.  */

#define BIGGEST_ALIGNMENT 128

/* Decide whether a variable of mode MODE must be 128 bit aligned.  */
#define ALIGN_MODE_128(MODE) \
 ((MODE) == XFmode || (MODE) == TFmode || ((MODE) == TImode) \
  || (MODE) == V4SFmode	|| (MODE) == V4SImode)

/* The published ABIs say that doubles should be aligned on word
   boundaries, so lower the aligment for structure fields unless
   -malign-double is set.  */
/* BIGGEST_FIELD_ALIGNMENT is also used in libobjc, where it must be
   constant.  Use the smaller value in that context.  */
#ifndef IN_TARGET_LIBS
#define BIGGEST_FIELD_ALIGNMENT (TARGET_64BIT ? 128 : (TARGET_ALIGN_DOUBLE ? 64 : 32))
#else
#define BIGGEST_FIELD_ALIGNMENT 32
#endif

/* If defined, a C expression to compute the alignment given to a
   constant that is being placed in memory.  EXP is the constant
   and ALIGN is the alignment that the object would ordinarily have.
   The value of this macro is used instead of that alignment to align
   the object.

   If this macro is not defined, then ALIGN is used.

   The typical use of this macro is to increase alignment for string
   constants to be word aligned so that `strcpy' calls that copy
   constants can be done inline.  */

#define CONSTANT_ALIGNMENT(EXP, ALIGN) ix86_constant_alignment ((EXP), (ALIGN))

/* If defined, a C expression to compute the alignment for a static
   variable.  TYPE is the data type, and ALIGN is the alignment that
   the object would ordinarily have.  The value of this macro is used
   instead of that alignment to align the object.

   If this macro is not defined, then ALIGN is used.

   One use of this macro is to increase alignment of medium-size
   data to make it all fit in fewer cache lines.  Another is to
   cause character arrays to be word-aligned so that `strcpy' calls
   that copy constants to character arrays can be done inline.  */

#define DATA_ALIGNMENT(TYPE, ALIGN) ix86_data_alignment ((TYPE), (ALIGN))

/* If defined, a C expression to compute the alignment for a local
   variable.  TYPE is the data type, and ALIGN is the alignment that
   the object would ordinarily have.  The value of this macro is used
   instead of that alignment to align the object.

   If this macro is not defined, then ALIGN is used.

   One use of this macro is to increase alignment of medium-size
   data to make it all fit in fewer cache lines.  */

#define LOCAL_ALIGNMENT(TYPE, ALIGN) ix86_local_alignment ((TYPE), (ALIGN))

/* If defined, a C expression that gives the alignment boundary, in
   bits, of an argument with the specified mode and type.  If it is
   not defined, `PARM_BOUNDARY' is used for all arguments.  */

#define FUNCTION_ARG_BOUNDARY(MODE, TYPE) \
  ix86_function_arg_boundary ((MODE), (TYPE))

/* Set this non-zero if move instructions will actually fail to work
   when given unaligned data.  */
#define STRICT_ALIGNMENT 0

/* If bit field type is int, don't let it cross an int,
   and give entire struct the alignment of an int.  */
/* Required on the 386 since it doesn't have bitfield insns.  */
#define PCC_BITFIELD_TYPE_MATTERS 1

/* Standard register usage.  */

/* This processor has special stack-like registers.  See reg-stack.c
   for details.  */

#define STACK_REGS
#define IS_STACK_MODE(MODE)					\
  ((MODE) == DFmode || (MODE) == SFmode || (MODE) == XFmode	\
   || (MODE) == TFmode)

/* Number of actual hardware registers.
   The hardware registers are assigned numbers for the compiler
   from 0 to just below FIRST_PSEUDO_REGISTER.
   All registers that the compiler knows about must be given numbers,
   even those that are not normally considered general registers.

   In the 80386 we give the 8 general purpose registers the numbers 0-7.
   We number the floating point registers 8-15.
   Note that registers 0-7 can be accessed as a  short or int,
   while only 0-3 may be used with byte `mov' instructions.

   Reg 16 does not correspond to any hardware register, but instead
   appears in the RTL as an argument pointer prior to reload, and is
   eliminated during reloading in favor of either the stack or frame
   pointer.  */

#define FIRST_PSEUDO_REGISTER 53

/* Number of hardware registers that go into the DWARF-2 unwind info.
   If not defined, equals FIRST_PSEUDO_REGISTER.  */

#define DWARF_FRAME_REGISTERS 17

/* 1 for registers that have pervasive standard uses
   and are not available for the register allocator.
   On the 80386, the stack pointer is such, as is the arg pointer.
 
   The value is an mask - bit 1 is set for fixed registers
   for 32bit target, while 2 is set for fixed registers for 64bit.
   Proper value is computed in the CONDITIONAL_REGISTER_USAGE.
 */
#define FIXED_REGISTERS						\
/*ax,dx,cx,bx,si,di,bp,sp,st,st1,st2,st3,st4,st5,st6,st7*/	\
{  0, 0, 0, 0, 0, 0, 0, 3, 0,  0,  0,  0,  0,  0,  0,  0,	\
/*arg,flags,fpsr,dir,frame*/					\
    3,    3,   3,  3,    3,					\
/*xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7*/			\
     0,   0,   0,   0,   0,   0,   0,   0,			\
/*mmx0,mmx1,mmx2,mmx3,mmx4,mmx5,mmx6,mmx7*/			\
     0,   0,   0,   0,   0,   0,   0,   0,			\
/*  r8,  r9, r10, r11, r12, r13, r14, r15*/			\
     1,   1,   1,   1,   1,   1,   1,   1,			\
/*xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15*/		\
     1,   1,    1,    1,    1,    1,    1,    1}
 

/* 1 for registers not available across function calls.
   These must include the FIXED_REGISTERS and also any
   registers that can be used without being saved.
   The latter must include the registers where values are returned
   and the register where structure-value addresses are passed.
   Aside from that, you can include as many other registers as you like. 
 
   The value is an mask - bit 1 is set for call used
   for 32bit target, while 2 is set for call used for 64bit.
   Proper value is computed in the CONDITIONAL_REGISTER_USAGE.
*/
#define CALL_USED_REGISTERS					\
/*ax,dx,cx,bx,si,di,bp,sp,st,st1,st2,st3,st4,st5,st6,st7*/	\
{  3, 3, 3, 0, 2, 2, 0, 3, 3,  3,  3,  3,  3,  3,  3,  3,	\
/*arg,flags,fpsr,dir,frame*/					\
     3,   3,   3,  3,    3,					\
/*xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7*/			\
     3,   3,   3,   3,   3,  3,    3,   3,			\
/*mmx0,mmx1,mmx2,mmx3,mmx4,mmx5,mmx6,mmx7*/			\
     3,   3,   3,   3,   3,   3,   3,   3,			\
/*  r8,  r9, r10, r11, r12, r13, r14, r15*/			\
     3,   3,   3,   3,   1,   1,   1,   1,			\
/*xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15*/		\
     3,   3,    3,    3,    3,    3,    3,    3}		\

/* Order in which to allocate registers.  Each register must be
   listed once, even those in FIXED_REGISTERS.  List frame pointer
   late and fixed registers last.  Note that, in general, we prefer
   registers listed in CALL_USED_REGISTERS, keeping the others
   available for storage of persistent values.

   The ORDER_REGS_FOR_LOCAL_ALLOC actually overwrite the order,
   so this is just empty initializer for array.  */

#define REG_ALLOC_ORDER 					\
{  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,\
   18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,	\
   33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,  \
   48, 49, 50, 51, 52 }

/* ORDER_REGS_FOR_LOCAL_ALLOC is a macro which permits reg_alloc_order
   to be rearranged based on a particular function.  When using sse math,
   we want to allocase SSE before x87 registers and vice vera.  */

#define ORDER_REGS_FOR_LOCAL_ALLOC x86_order_regs_for_local_alloc ()


/* Macro to conditionally modify fixed_regs/call_used_regs.  */
#define CONDITIONAL_REGISTER_USAGE					\
do {									\
    int i;								\
    for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)				\
      {									\
        fixed_regs[i] = (fixed_regs[i] & (TARGET_64BIT ? 2 : 1)) != 0;	\
        call_used_regs[i] = (call_used_regs[i]				\
			     & (TARGET_64BIT ? 2 : 1)) != 0;		\
      }									\
    if (PIC_OFFSET_TABLE_REGNUM != INVALID_REGNUM)			\
      {									\
	fixed_regs[PIC_OFFSET_TABLE_REGNUM] = 1;			\
	call_used_regs[PIC_OFFSET_TABLE_REGNUM] = 1;			\
      }									\
    if (! TARGET_MMX)							\
      {									\
	int i;								\
        for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)			\
          if (TEST_HARD_REG_BIT (reg_class_contents[(int)MMX_REGS], i))	\
	    fixed_regs[i] = call_used_regs[i] = 1;		 	\
      }									\
    if (! TARGET_SSE)							\
      {									\
	int i;								\
        for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)			\
          if (TEST_HARD_REG_BIT (reg_class_contents[(int)SSE_REGS], i))	\
	    fixed_regs[i] = call_used_regs[i] = 1;		 	\
      }									\
    if (! TARGET_80387 && ! TARGET_FLOAT_RETURNS_IN_80387)		\
      {									\
	int i;								\
	HARD_REG_SET x;							\
        COPY_HARD_REG_SET (x, reg_class_contents[(int)FLOAT_REGS]);	\
        for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)			\
          if (TEST_HARD_REG_BIT (x, i)) 				\
	    fixed_regs[i] = call_used_regs[i] = 1;			\
      }									\
  } while (0)

/* Return number of consecutive hard regs needed starting at reg REGNO
   to hold something of mode MODE.
   This is ordinarily the length in words of a value of mode MODE
   but can be less for certain modes in special long registers.

   Actually there are no two word move instructions for consecutive 
   registers.  And only registers 0-3 may have mov byte instructions
   applied to them.
   */

#define HARD_REGNO_NREGS(REGNO, MODE)   \
  (FP_REGNO_P (REGNO) || SSE_REGNO_P (REGNO) || MMX_REGNO_P (REGNO)	\
   ? (COMPLEX_MODE_P (MODE) ? 2 : 1)					\
   : ((MODE) == TFmode							\
      ? (TARGET_64BIT ? 2 : 3)						\
      : (MODE) == TCmode						\
      ? (TARGET_64BIT ? 4 : 6)						\
      : ((GET_MODE_SIZE (MODE) + UNITS_PER_WORD - 1) / UNITS_PER_WORD)))

#define VALID_SSE_REG_MODE(MODE)					\
    ((MODE) == TImode || (MODE) == V4SFmode || (MODE) == V4SImode	\
     || (MODE) == SFmode						\
     || (TARGET_SSE2 && ((MODE) == DFmode || VALID_MMX_REG_MODE (MODE))))

#define VALID_MMX_REG_MODE_3DNOW(MODE) \
    ((MODE) == V2SFmode || (MODE) == SFmode)

#define VALID_MMX_REG_MODE(MODE)					\
    ((MODE) == DImode || (MODE) == V8QImode || (MODE) == V4HImode	\
     || (MODE) == V2SImode || (MODE) == SImode)

#define VECTOR_MODE_SUPPORTED_P(MODE)					\
    (VALID_SSE_REG_MODE (MODE) && TARGET_SSE ? 1			\
     : VALID_MMX_REG_MODE (MODE) && TARGET_MMX ? 1			\
     : VALID_MMX_REG_MODE_3DNOW (MODE) && TARGET_3DNOW ? 1 : 0)

#define VALID_FP_MODE_P(MODE)						\
    ((MODE) == SFmode || (MODE) == DFmode || (MODE) == TFmode		\
     || (!TARGET_64BIT && (MODE) == XFmode)				\
     || (MODE) == SCmode || (MODE) == DCmode || (MODE) == TCmode	\
     || (!TARGET_64BIT && (MODE) == XCmode))

#define VALID_INT_MODE_P(MODE)						\
    ((MODE) == QImode || (MODE) == HImode || (MODE) == SImode		\
     || (MODE) == DImode						\
     || (MODE) == CQImode || (MODE) == CHImode || (MODE) == CSImode	\
     || (MODE) == CDImode						\
     || (TARGET_64BIT && ((MODE) == TImode || (MODE) == CTImode)))

/* Value is 1 if hard register REGNO can hold a value of machine-mode MODE.  */

#define HARD_REGNO_MODE_OK(REGNO, MODE)	\
   ix86_hard_regno_mode_ok ((REGNO), (MODE))

/* Value is 1 if it is a good idea to tie two pseudo registers
   when one has mode MODE1 and one has mode MODE2.
   If HARD_REGNO_MODE_OK could produce different values for MODE1 and MODE2,
   for any hard reg, then this must be 0 for correct output.  */

#define MODES_TIEABLE_P(MODE1, MODE2)				\
  ((MODE1) == (MODE2)						\
   || (((MODE1) == HImode || (MODE1) == SImode			\
	|| ((MODE1) == QImode					\
	    && (TARGET_64BIT || !TARGET_PARTIAL_REG_STALL))	\
        || ((MODE1) == DImode && TARGET_64BIT))			\
       && ((MODE2) == HImode || (MODE2) == SImode		\
	   || ((MODE1) == QImode				\
	       && (TARGET_64BIT || !TARGET_PARTIAL_REG_STALL))	\
	   || ((MODE2) == DImode && TARGET_64BIT))))


/* Specify the modes required to caller save a given hard regno.
   We do this on i386 to prevent flags from being saved at all.

   Kill any attempts to combine saving of modes.  */

#define HARD_REGNO_CALLER_SAVE_MODE(REGNO, NREGS, MODE)			\
  (CC_REGNO_P (REGNO) ? VOIDmode					\
   : (MODE) == VOIDmode && (NREGS) != 1 ? VOIDmode			\
   : (MODE) == VOIDmode ? choose_hard_reg_mode ((REGNO), (NREGS))	\
   : (MODE) == HImode && !TARGET_PARTIAL_REG_STALL ? SImode		\
   : (MODE) == QImode && (REGNO) >= 4 && !TARGET_64BIT ? SImode 	\
   : (MODE))
/* Specify the registers used for certain standard purposes.
   The values of these macros are register numbers.  */

/* on the 386 the pc register is %eip, and is not usable as a general
   register.  The ordinary mov instructions won't work */
/* #define PC_REGNUM  */

/* Register to use for pushing function arguments.  */
#define STACK_POINTER_REGNUM 7

/* Base register for access to local variables of the function.  */
#define HARD_FRAME_POINTER_REGNUM 6

/* Base register for access to local variables of the function.  */
#define FRAME_POINTER_REGNUM 20

/* First floating point reg */
#define FIRST_FLOAT_REG 8

/* First & last stack-like regs */
#define FIRST_STACK_REG FIRST_FLOAT_REG
#define LAST_STACK_REG (FIRST_FLOAT_REG + 7)

#define FLAGS_REG 17
#define FPSR_REG 18
#define DIRFLAG_REG 19

#define FIRST_SSE_REG (FRAME_POINTER_REGNUM + 1)
#define LAST_SSE_REG  (FIRST_SSE_REG + 7)
 
#define FIRST_MMX_REG  (LAST_SSE_REG + 1)
#define LAST_MMX_REG   (FIRST_MMX_REG + 7)

#define FIRST_REX_INT_REG  (LAST_MMX_REG + 1)
#define LAST_REX_INT_REG   (FIRST_REX_INT_REG + 7)

#define FIRST_REX_SSE_REG  (LAST_REX_INT_REG + 1)
#define LAST_REX_SSE_REG   (FIRST_REX_SSE_REG + 7)

/* Value should be nonzero if functions must have frame pointers.
   Zero means the frame pointer need not be set up (and parms
   may be accessed via the stack pointer) in functions that seem suitable.
   This is computed in `reload', in reload1.c.  */
#define FRAME_POINTER_REQUIRED  ix86_frame_pointer_required ()

/* Override this in other tm.h files to cope with various OS losage
   requiring a frame pointer.  */
#ifndef SUBTARGET_FRAME_POINTER_REQUIRED
#define SUBTARGET_FRAME_POINTER_REQUIRED 0
#endif

/* Make sure we can access arbitrary call frames.  */
#define SETUP_FRAME_ADDRESSES()  ix86_setup_frame_addresses ()

/* Base register for access to arguments of the function.  */
#define ARG_POINTER_REGNUM 16

/* Register in which static-chain is passed to a function.
   We do use ECX as static chain register for 32 bit ABI.  On the
   64bit ABI, ECX is an argument register, so we use R10 instead.  */
#define STATIC_CHAIN_REGNUM (TARGET_64BIT ? FIRST_REX_INT_REG + 10 - 8 : 2)

/* Register to hold the addressing base for position independent
   code access to data items.  We don't use PIC pointer for 64bit
   mode.  Define the regnum to dummy value to prevent gcc from
   pessimizing code dealing with EBX.  */
#define PIC_OFFSET_TABLE_REGNUM \
  (TARGET_64BIT || !flag_pic ? INVALID_REGNUM : 3)

/* Register in which address to store a structure value
   arrives in the function.  On the 386, the prologue
   copies this from the stack to register %eax.  */
#define STRUCT_VALUE_INCOMING 0

/* Place in which caller passes the structure value address.
   0 means push the value on the stack like an argument.  */
#define STRUCT_VALUE 0

/* A C expression which can inhibit the returning of certain function
   values in registers, based on the type of value.  A nonzero value
   says to return the function value in memory, just as large
   structures are always returned.  Here TYPE will be a C expression
   of type `tree', representing the data type of the value.

   Note that values of mode `BLKmode' must be explicitly handled by
   this macro.  Also, the option `-fpcc-struct-return' takes effect
   regardless of this macro.  On most systems, it is possible to
   leave the macro undefined; this causes a default definition to be
   used, whose value is the constant 1 for `BLKmode' values, and 0
   otherwise.

   Do not use this macro to indicate that structures and unions
   should always be returned in memory.  You should instead use
   `DEFAULT_PCC_STRUCT_RETURN' to indicate this.  */

#define RETURN_IN_MEMORY(TYPE) \
  ix86_return_in_memory (TYPE)


/* Define the classes of registers for register constraints in the
   machine description.  Also define ranges of constants.

   One of the classes must always be named ALL_REGS and include all hard regs.
   If there is more than one class, another class must be named NO_REGS
   and contain no registers.

   The name GENERAL_REGS must be the name of a class (or an alias for
   another name such as ALL_REGS).  This is the class of registers
   that is allowed by "g" or "r" in a register constraint.
   Also, registers outside this class are allocated only when
   instructions express preferences for them.

   The classes must be numbered in nondecreasing order; that is,
   a larger-numbered class must never be contained completely
   in a smaller-numbered class.

   For any two classes, it is very desirable that there be another
   class that represents their union.

   It might seem that class BREG is unnecessary, since no useful 386
   opcode needs reg %ebx.  But some systems pass args to the OS in ebx,
   and the "b" register constraint is useful in asms for syscalls.

   The flags and fpsr registers are in no class.  */

enum reg_class
{
  NO_REGS,
  AREG, DREG, CREG, BREG, SIREG, DIREG,
  AD_REGS,			/* %eax/%edx for DImode */
  Q_REGS,			/* %eax %ebx %ecx %edx */
  NON_Q_REGS,			/* %esi %edi %ebp %esp */
  INDEX_REGS,			/* %eax %ebx %ecx %edx %esi %edi %ebp */
  LEGACY_REGS,			/* %eax %ebx %ecx %edx %esi %edi %ebp %esp */
  GENERAL_REGS,			/* %eax %ebx %ecx %edx %esi %edi %ebp %esp %r8 - %r15*/
  FP_TOP_REG, FP_SECOND_REG,	/* %st(0) %st(1) */
  FLOAT_REGS,
  SSE_REGS,
  MMX_REGS,
  FP_TOP_SSE_REGS,
  FP_SECOND_SSE_REGS,
  FLOAT_SSE_REGS,
  FLOAT_INT_REGS,
  INT_SSE_REGS,
  FLOAT_INT_SSE_REGS,
  ALL_REGS, LIM_REG_CLASSES
};

#define N_REG_CLASSES ((int) LIM_REG_CLASSES)

#define INTEGER_CLASS_P(CLASS) \
  reg_class_subset_p ((CLASS), GENERAL_REGS)
#define FLOAT_CLASS_P(CLASS) \
  reg_class_subset_p ((CLASS), FLOAT_REGS)
#define SSE_CLASS_P(CLASS) \
  reg_class_subset_p ((CLASS), SSE_REGS)
#define MMX_CLASS_P(CLASS) \
  reg_class_subset_p ((CLASS), MMX_REGS)
#define MAYBE_INTEGER_CLASS_P(CLASS) \
  reg_classes_intersect_p ((CLASS), GENERAL_REGS)
#define MAYBE_FLOAT_CLASS_P(CLASS) \
  reg_classes_intersect_p ((CLASS), FLOAT_REGS)
#define MAYBE_SSE_CLASS_P(CLASS) \
  reg_classes_intersect_p (SSE_REGS, (CLASS))
#define MAYBE_MMX_CLASS_P(CLASS) \
  reg_classes_intersect_p (MMX_REGS, (CLASS))

#define Q_CLASS_P(CLASS) \
  reg_class_subset_p ((CLASS), Q_REGS)

/* Give names of register classes as strings for dump file.   */

#define REG_CLASS_NAMES \
{  "NO_REGS",				\
   "AREG", "DREG", "CREG", "BREG",	\
   "SIREG", "DIREG",			\
   "AD_REGS",				\
   "Q_REGS", "NON_Q_REGS",		\
   "INDEX_REGS",			\
   "LEGACY_REGS",			\
   "GENERAL_REGS",			\
   "FP_TOP_REG", "FP_SECOND_REG",	\
   "FLOAT_REGS",			\
   "SSE_REGS",				\
   "MMX_REGS",				\
   "FP_TOP_SSE_REGS",			\
   "FP_SECOND_SSE_REGS",		\
   "FLOAT_SSE_REGS",			\
   "FLOAT_INT_REGS",			\
   "INT_SSE_REGS",			\
   "FLOAT_INT_SSE_REGS",		\
   "ALL_REGS" }

/* Define which registers fit in which classes.
   This is an initializer for a vector of HARD_REG_SET
   of length N_REG_CLASSES.  */

#define REG_CLASS_CONTENTS						\
{     { 0x00,     0x0 },						\
      { 0x01,     0x0 }, { 0x02, 0x0 },	/* AREG, DREG */		\
      { 0x04,     0x0 }, { 0x08, 0x0 },	/* CREG, BREG */		\
      { 0x10,     0x0 }, { 0x20, 0x0 },	/* SIREG, DIREG */		\
      { 0x03,     0x0 },		/* AD_REGS */			\
      { 0x0f,     0x0 },		/* Q_REGS */			\
  { 0x1100f0,  0x1fe0 },		/* NON_Q_REGS */		\
      { 0x7f,  0x1fe0 },		/* INDEX_REGS */		\
  { 0x1100ff,  0x0 },			/* LEGACY_REGS */		\
  { 0x1100ff,  0x1fe0 },		/* GENERAL_REGS */		\
     { 0x100,     0x0 }, { 0x0200, 0x0 },/* FP_TOP_REG, FP_SECOND_REG */\
    { 0xff00,     0x0 },		/* FLOAT_REGS */		\
{ 0x1fe00000,0x1fe000 },		/* SSE_REGS */			\
{ 0xe0000000,    0x1f },		/* MMX_REGS */			\
{ 0x1fe00100,0x1fe000 },		/* FP_TOP_SSE_REG */		\
{ 0x1fe00200,0x1fe000 },		/* FP_SECOND_SSE_REG */		\
{ 0x1fe0ff00,0x1fe000 },		/* FLOAT_SSE_REGS */		\
   { 0x1ffff,  0x1fe0 },		/* FLOAT_INT_REGS */		\
{ 0x1fe100ff,0x1fffe0 },		/* INT_SSE_REGS */		\
{ 0x1fe1ffff,0x1fffe0 },		/* FLOAT_INT_SSE_REGS */	\
{ 0xffffffff,0x1fffff }							\
}

/* The same information, inverted:
   Return the class number of the smallest class containing
   reg number REGNO.  This could be a conditional expression
   or could index an array.  */

#define REGNO_REG_CLASS(REGNO) (regclass_map[REGNO])

/* When defined, the compiler allows registers explicitly used in the
   rtl to be used as spill registers but prevents the compiler from
   extending the lifetime of these registers.  */

#define SMALL_REGISTER_CLASSES 1

#define QI_REG_P(X) \
  (REG_P (X) && REGNO (X) < 4)

#define GENERAL_REGNO_P(N) \
  ((N) < 8 || REX_INT_REGNO_P (N))

#define GENERAL_REG_P(X) \
  (REG_P (X) && GENERAL_REGNO_P (REGNO (X)))

#define ANY_QI_REG_P(X) (TARGET_64BIT ? GENERAL_REG_P(X) : QI_REG_P (X))

#define NON_QI_REG_P(X) \
  (REG_P (X) && REGNO (X) >= 4 && REGNO (X) < FIRST_PSEUDO_REGISTER)

#define REX_INT_REGNO_P(N) ((N) >= FIRST_REX_INT_REG && (N) <= LAST_REX_INT_REG)
#define REX_INT_REG_P(X) (REG_P (X) && REX_INT_REGNO_P (REGNO (X)))

#define FP_REG_P(X) (REG_P (X) && FP_REGNO_P (REGNO (X)))
#define FP_REGNO_P(N) ((N) >= FIRST_STACK_REG && (N) <= LAST_STACK_REG)
#define ANY_FP_REG_P(X) (REG_P (X) && ANY_FP_REGNO_P (REGNO (X)))
#define ANY_FP_REGNO_P(N) (FP_REGNO_P (N) || SSE_REGNO_P (N))

#define SSE_REGNO_P(N) \
  (((N) >= FIRST_SSE_REG && (N) <= LAST_SSE_REG) \
   || ((N) >= FIRST_REX_SSE_REG && (N) <= LAST_REX_SSE_REG))

#define SSE_REGNO(N) \
  ((N) < 8 ? FIRST_SSE_REG + (N) : FIRST_REX_SSE_REG + (N) - 8)
#define SSE_REG_P(N) (REG_P (N) && SSE_REGNO_P (REGNO (N)))

#define SSE_FLOAT_MODE_P(MODE) \
  ((TARGET_SSE && (MODE) == SFmode) || (TARGET_SSE2 && (MODE) == DFmode))

#define MMX_REGNO_P(N) ((N) >= FIRST_MMX_REG && (N) <= LAST_MMX_REG)
#define MMX_REG_P(XOP) (REG_P (XOP) && MMX_REGNO_P (REGNO (XOP)))
  
#define STACK_REG_P(XOP)		\
  (REG_P (XOP) &&		       	\
   REGNO (XOP) >= FIRST_STACK_REG &&	\
   REGNO (XOP) <= LAST_STACK_REG)

#define NON_STACK_REG_P(XOP) (REG_P (XOP) && ! STACK_REG_P (XOP))

#define STACK_TOP_P(XOP) (REG_P (XOP) && REGNO (XOP) == FIRST_STACK_REG)

#define CC_REG_P(X) (REG_P (X) && CC_REGNO_P (REGNO (X)))
#define CC_REGNO_P(X) ((X) == FLAGS_REG || (X) == FPSR_REG)

/* Indicate whether hard register numbered REG_NO should be converted
   to SSA form.  */
#define CONVERT_HARD_REGISTER_TO_SSA_P(REG_NO) \
  ((REG_NO) == FLAGS_REG || (REG_NO) == ARG_POINTER_REGNUM)

/* The class value for index registers, and the one for base regs.  */

#define INDEX_REG_CLASS INDEX_REGS
#define BASE_REG_CLASS GENERAL_REGS

/* Get reg_class from a letter such as appears in the machine description.  */

#define REG_CLASS_FROM_LETTER(C)	\
  ((C) == 'r' ? GENERAL_REGS :					\
   (C) == 'R' ? LEGACY_REGS :					\
   (C) == 'q' ? TARGET_64BIT ? GENERAL_REGS : Q_REGS :		\
   (C) == 'Q' ? Q_REGS :					\
   (C) == 'f' ? (TARGET_80387 || TARGET_FLOAT_RETURNS_IN_80387	\
		 ? FLOAT_REGS					\
		 : NO_REGS) :					\
   (C) == 't' ? (TARGET_80387 || TARGET_FLOAT_RETURNS_IN_80387	\
		 ? FP_TOP_REG					\
		 : NO_REGS) :					\
   (C) == 'u' ? (TARGET_80387 || TARGET_FLOAT_RETURNS_IN_80387	\
		 ? FP_SECOND_REG				\
		 : NO_REGS) :					\
   (C) == 'a' ? AREG :						\
   (C) == 'b' ? BREG :						\
   (C) == 'c' ? CREG :						\
   (C) == 'd' ? DREG :						\
   (C) == 'x' ? TARGET_SSE ? SSE_REGS : NO_REGS :		\
   (C) == 'Y' ? TARGET_SSE2? SSE_REGS : NO_REGS :		\
   (C) == 'y' ? TARGET_MMX ? MMX_REGS : NO_REGS :		\
   (C) == 'A' ? AD_REGS :					\
   (C) == 'D' ? DIREG :						\
   (C) == 'S' ? SIREG : NO_REGS)

/* The letters I, J, K, L and M in a register constraint string
   can be used to stand for particular ranges of immediate operands.
   This macro defines what the ranges are.
   C is the letter, and VALUE is a constant value.
   Return 1 if VALUE is in the range specified by C.

   I is for non-DImode shifts.
   J is for DImode shifts.
   K is for signed imm8 operands.
   L is for andsi as zero-extending move.
   M is for shifts that can be executed by the "lea" opcode.
   N is for immedaite operands for out/in instructions (0-255)
   */

#define CONST_OK_FOR_LETTER_P(VALUE, C)				\
  ((C) == 'I' ? (VALUE) >= 0 && (VALUE) <= 31			\
   : (C) == 'J' ? (VALUE) >= 0 && (VALUE) <= 63			\
   : (C) == 'K' ? (VALUE) >= -128 && (VALUE) <= 127		\
   : (C) == 'L' ? (VALUE) == 0xff || (VALUE) == 0xffff		\
   : (C) == 'M' ? (VALUE) >= 0 && (VALUE) <= 3			\
   : (C) == 'N' ? (VALUE) >= 0 && (VALUE) <= 255		\
   : 0)

/* Similar, but for floating constants, and defining letters G and H.
   Here VALUE is the CONST_DOUBLE rtx itself.  We allow constants even if
   TARGET_387 isn't set, because the stack register converter may need to
   load 0.0 into the function value register.  */

#define CONST_DOUBLE_OK_FOR_LETTER_P(VALUE, C)  \
  ((C) == 'G' ? standard_80387_constant_p (VALUE) \
   : ((C) == 'H' ? standard_sse_constant_p (VALUE) : 0))

/* A C expression that defines the optional machine-dependent
   constraint letters that can be used to segregate specific types of
   operands, usually memory references, for the target machine.  Any
   letter that is not elsewhere defined and not matched by
   `REG_CLASS_FROM_LETTER' may be used.  Normally this macro will not
   be defined.

   If it is required for a particular target machine, it should
   return 1 if VALUE corresponds to the operand type represented by
   the constraint letter C.  If C is not defined as an extra
   constraint, the value returned should be 0 regardless of VALUE.  */

#define EXTRA_CONSTRAINT(VALUE, C)				\
  ((C) == 'e' ? x86_64_sign_extended_value (VALUE)		\
   : (C) == 'Z' ? x86_64_zero_extended_value (VALUE)		\
   : 0)

/* Place additional restrictions on the register class to use when it
   is necessary to be able to hold a value of mode MODE in a reload
   register for which class CLASS would ordinarily be used.  */

#define LIMIT_RELOAD_CLASS(MODE, CLASS) 			\
  ((MODE) == QImode && !TARGET_64BIT				\
   && ((CLASS) == ALL_REGS || (CLASS) == GENERAL_REGS		\
       || (CLASS) == LEGACY_REGS || (CLASS) == INDEX_REGS)	\
   ? Q_REGS : (CLASS))

/* Given an rtx X being reloaded into a reg required to be
   in class CLASS, return the class of reg to actually use.
   In general this is just CLASS; but on some machines
   in some cases it is preferable to use a more restrictive class.
   On the 80386 series, we prevent floating constants from being
   reloaded into floating registers (since no move-insn can do that)
   and we ensure that QImodes aren't reloaded into the esi or edi reg.  */

/* Put float CONST_DOUBLE in the constant pool instead of fp regs.
   QImode must go into class Q_REGS.
   Narrow ALL_REGS to GENERAL_REGS.  This supports allowing movsf and
   movdf to do mem-to-mem moves through integer regs.  */

#define PREFERRED_RELOAD_CLASS(X, CLASS) \
   ix86_preferred_reload_class ((X), (CLASS))

/* If we are copying between general and FP registers, we need a memory
   location. The same is true for SSE and MMX registers.  */
#define SECONDARY_MEMORY_NEEDED(CLASS1, CLASS2, MODE) \
  ix86_secondary_memory_needed ((CLASS1), (CLASS2), (MODE), 1)

/* QImode spills from non-QI registers need a scratch.  This does not
   happen often -- the only example so far requires an uninitialized 
   pseudo.  */

#define SECONDARY_OUTPUT_RELOAD_CLASS(CLASS, MODE, OUT)			\
  (((CLASS) == GENERAL_REGS || (CLASS) == LEGACY_REGS			\
    || (CLASS) == INDEX_REGS) && !TARGET_64BIT && (MODE) == QImode	\
   ? Q_REGS : NO_REGS)

/* Return the maximum number of consecutive registers
   needed to represent mode MODE in a register of class CLASS.  */
/* On the 80386, this is the size of MODE in words,
   except in the FP regs, where a single reg is always enough.
   The TFmodes are really just 80bit values, so we use only 3 registers
   to hold them, instead of 4, as the size would suggest.
 */
#define CLASS_MAX_NREGS(CLASS, MODE)					\
 (!MAYBE_INTEGER_CLASS_P (CLASS)					\
  ? (COMPLEX_MODE_P (MODE) ? 2 : 1)					\
  : ((GET_MODE_SIZE ((MODE) == TFmode ? XFmode : (MODE))		\
     + UNITS_PER_WORD - 1) / UNITS_PER_WORD))

/* A C expression whose value is nonzero if pseudos that have been
   assigned to registers of class CLASS would likely be spilled
   because registers of CLASS are needed for spill registers.

   The default value of this macro returns 1 if CLASS has exactly one
   register and zero otherwise.  On most machines, this default
   should be used.  Only define this macro to some other expression
   if pseudo allocated by `local-alloc.c' end up in memory because
   their hard registers were needed for spill registers.  If this
   macro returns nonzero for those classes, those pseudos will only
   be allocated by `global.c', which knows how to reallocate the
   pseudo to another register.  If there would not be another
   register available for reallocation, you should not change the
   definition of this macro since the only effect of such a
   definition would be to slow down register allocation.  */

#define CLASS_LIKELY_SPILLED_P(CLASS)					\
  (((CLASS) == AREG)							\
   || ((CLASS) == DREG)							\
   || ((CLASS) == CREG)							\
   || ((CLASS) == BREG)							\
   || ((CLASS) == AD_REGS)						\
   || ((CLASS) == SIREG)						\
   || ((CLASS) == DIREG))

/* A C statement that adds to CLOBBERS any hard regs the port wishes
   to automatically clobber for all asms. 

   We do this in the new i386 backend to maintain source compatibility
   with the old cc0-based compiler.  */

#define MD_ASM_CLOBBERS(CLOBBERS)					\
  do {									\
    (CLOBBERS) = tree_cons (NULL_TREE, build_string (5, "flags"),	\
			    (CLOBBERS));				\
    (CLOBBERS) = tree_cons (NULL_TREE, build_string (4, "fpsr"),	\
			    (CLOBBERS));				\
    (CLOBBERS) = tree_cons (NULL_TREE, build_string (7, "dirflag"),	\
			    (CLOBBERS));				\
  } while (0)

/* Stack layout; function entry, exit and calling.  */

/* Define this if pushing a word on the stack
   makes the stack pointer a smaller address.  */
#define STACK_GROWS_DOWNWARD

/* Define this if the nominal address of the stack frame
   is at the high-address end of the local variables;
   that is, each additional local variable allocated
   goes at a more negative offset in the frame.  */
#define FRAME_GROWS_DOWNWARD

/* Offset within stack frame to start allocating local variables at.
   If FRAME_GROWS_DOWNWARD, this is the offset to the END of the
   first local allocated.  Otherwise, it is the offset to the BEGINNING
   of the first local allocated.  */
#define STARTING_FRAME_OFFSET 0

/* If we generate an insn to push BYTES bytes,
   this says how many the stack pointer really advances by.
   On 386 pushw decrements by exactly 2 no matter what the position was.
   On the 386 there is no pushb; we use pushw instead, and this
   has the effect of rounding up to 2.
 
   For 64bit ABI we round up to 8 bytes.
 */

#define PUSH_ROUNDING(BYTES) \
  (TARGET_64BIT		     \
   ? (((BYTES) + 7) & (-8))  \
   : (((BYTES) + 1) & (-2)))

/* If defined, the maximum amount of space required for outgoing arguments will
   be computed and placed into the variable
   `current_function_outgoing_args_size'.  No space will be pushed onto the
   stack for each call; instead, the function prologue should increase the stack
   frame size by this amount.  */

#define ACCUMULATE_OUTGOING_ARGS TARGET_ACCUMULATE_OUTGOING_ARGS

/* If defined, a C expression whose value is nonzero when we want to use PUSH
   instructions to pass outgoing arguments.  */

#define PUSH_ARGS (TARGET_PUSH_ARGS && !ACCUMULATE_OUTGOING_ARGS)

/* Offset of first parameter from the argument pointer register value.  */
#define FIRST_PARM_OFFSET(FNDECL) 0

/* Define this macro if functions should assume that stack space has been
   allocated for arguments even when their values are passed in registers.

   The value of this macro is the size, in bytes, of the area reserved for
   arguments passed in registers for the function represented by FNDECL.

   This space can be allocated by the caller, or be a part of the
   machine-dependent stack frame: `OUTGOING_REG_PARM_STACK_SPACE' says
   which.  */
#define REG_PARM_STACK_SPACE(FNDECL) 0

/* Define as a C expression that evaluates to nonzero if we do not know how
   to pass TYPE solely in registers.  The file expr.h defines a
   definition that is usually appropriate, refer to expr.h for additional
   documentation. If `REG_PARM_STACK_SPACE' is defined, the argument will be
   computed in the stack and then loaded into a register.  */
#define MUST_PASS_IN_STACK(MODE, TYPE)				\
  ((TYPE) != 0							\
   && (TREE_CODE (TYPE_SIZE (TYPE)) != INTEGER_CST		\
       || TREE_ADDRESSABLE (TYPE)				\
       || ((MODE) == TImode)					\
       || ((MODE) == BLKmode 					\
	   && ! ((TYPE) != 0					\
		 && TREE_CODE (TYPE_SIZE (TYPE)) == INTEGER_CST \
		 && 0 == (int_size_in_bytes (TYPE)		\
			  % (PARM_BOUNDARY / BITS_PER_UNIT)))	\
	   && (FUNCTION_ARG_PADDING (MODE, TYPE)		\
	       == (BYTES_BIG_ENDIAN ? upward : downward)))))

/* Value is the number of bytes of arguments automatically
   popped when returning from a subroutine call.
   FUNDECL is the declaration node of the function (as a tree),
   FUNTYPE is the data type of the function (as a tree),
   or for a library call it is an identifier node for the subroutine name.
   SIZE is the number of bytes of arguments passed on the stack.

   On the 80386, the RTD insn may be used to pop them if the number
     of args is fixed, but if the number is variable then the caller
     must pop them all.  RTD can't be used for library calls now
     because the library is compiled with the Unix compiler.
   Use of RTD is a selectable option, since it is incompatible with
   standard Unix calling sequences.  If the option is not selected,
   the caller must always pop the args.

   The attribute stdcall is equivalent to RTD on a per module basis.  */

#define RETURN_POPS_ARGS(FUNDECL, FUNTYPE, SIZE) \
  ix86_return_pops_args ((FUNDECL), (FUNTYPE), (SIZE))

/* Define how to find the value returned by a function.
   VALTYPE is the data type of the value (as a tree).
   If the precise function being called is known, FUNC is its FUNCTION_DECL;
   otherwise, FUNC is 0.  */
#define FUNCTION_VALUE(VALTYPE, FUNC)  \
   ix86_function_value (VALTYPE)

#define FUNCTION_VALUE_REGNO_P(N) \
  ix86_function_value_regno_p (N)

/* Define how to find the value returned by a library function
   assuming the value has mode MODE.  */

#define LIBCALL_VALUE(MODE) \
  ix86_libcall_value (MODE)

/* Define the size of the result block used for communication between
   untyped_call and untyped_return.  The block contains a DImode value
   followed by the block used by fnsave and frstor.  */

#define APPLY_RESULT_SIZE (8+108)

/* 1 if N is a possible register number for function argument passing.  */
#define FUNCTION_ARG_REGNO_P(N) ix86_function_arg_regno_p (N)

/* Define a data type for recording info about an argument list
   during the scan of that argument list.  This data type should
   hold all necessary information about the function itself
   and about the args processed so far, enough to enable macros
   such as FUNCTION_ARG to determine where the next arg should go.  */

typedef struct ix86_args {
  int words;			/* # words passed so far */
  int nregs;			/* # registers available for passing */
  int regno;			/* next available register number */
  int sse_words;		/* # sse words passed so far */
  int sse_nregs;		/* # sse registers available for passing */
  int sse_regno;		/* next available sse register number */
  int maybe_vaarg;		/* true for calls to possibly vardic fncts.  */
} CUMULATIVE_ARGS;

/* Initialize a variable CUM of type CUMULATIVE_ARGS
   for a call to a function whose data type is FNTYPE.
   For a library call, FNTYPE is 0.  */

#define INIT_CUMULATIVE_ARGS(CUM, FNTYPE, LIBNAME, INDIRECT) \
  init_cumulative_args (&(CUM), (FNTYPE), (LIBNAME))

/* Update the data in CUM to advance over an argument
   of mode MODE and data type TYPE.
   (TYPE is null for libcalls where that information may not be available.)  */

#define FUNCTION_ARG_ADVANCE(CUM, MODE, TYPE, NAMED) \
  function_arg_advance (&(CUM), (MODE), (TYPE), (NAMED))

/* Define where to put the arguments to a function.
   Value is zero to push the argument on the stack,
   or a hard register in which to store the argument.

   MODE is the argument's machine mode.
   TYPE is the data type of the argument (as a tree).
    This is null for libcalls where that information may
    not be available.
   CUM is a variable of type CUMULATIVE_ARGS which gives info about
    the preceding args and about the function being called.
   NAMED is nonzero if this argument is a named parameter
    (otherwise it is an extra parameter matching an ellipsis).  */

#define FUNCTION_ARG(CUM, MODE, TYPE, NAMED) \
  function_arg (&(CUM), (MODE), (TYPE), (NAMED))

/* For an arg passed partly in registers and partly in memory,
   this is the number of registers used.
   For args passed entirely in registers or entirely in memory, zero.  */

#define FUNCTION_ARG_PARTIAL_NREGS(CUM, MODE, TYPE, NAMED) 0

/* If PIC, we cannot make sibling calls to global functions
   because the PLT requires %ebx live.
   If we are returning floats on the register stack, we cannot make
   sibling calls to functions that return floats.  (The stack adjust
   instruction will wind up after the sibcall jump, and not be executed.) */
#define FUNCTION_OK_FOR_SIBCALL(DECL)					\
  ((DECL)								\
   && (! flag_pic || ! TREE_PUBLIC (DECL))				\
   && (! TARGET_FLOAT_RETURNS_IN_80387					\
       || ! FLOAT_MODE_P (TYPE_MODE (TREE_TYPE (TREE_TYPE (DECL))))	\
       || FLOAT_MODE_P (TYPE_MODE (TREE_TYPE (TREE_TYPE (cfun->decl))))))

/* Perform any needed actions needed for a function that is receiving a
   variable number of arguments.

   CUM is as above.

   MODE and TYPE are the mode and type of the current parameter.

   PRETEND_SIZE is a variable that should be set to the amount of stack
   that must be pushed by the prolog to pretend that our caller pushed
   it.

   Normally, this macro will push all remaining incoming registers on the
   stack and set PRETEND_SIZE to the length of the registers pushed.  */

#define SETUP_INCOMING_VARARGS(CUM, MODE, TYPE, PRETEND_SIZE, NO_RTL)	\
  ix86_setup_incoming_varargs (&(CUM), (MODE), (TYPE), &(PRETEND_SIZE), \
			       (NO_RTL))

/* Define the `__builtin_va_list' type for the ABI.  */
#define BUILD_VA_LIST_TYPE(VALIST) \
  ((VALIST) = ix86_build_va_list ())

/* Implement `va_start' for varargs and stdarg.  */
#define EXPAND_BUILTIN_VA_START(STDARG, VALIST, NEXTARG) \
  ix86_va_start ((STDARG), (VALIST), (NEXTARG))

/* Implement `va_arg'.  */
#define EXPAND_BUILTIN_VA_ARG(VALIST, TYPE) \
  ix86_va_arg ((VALIST), (TYPE))

/* This macro is invoked at the end of compilation.  It is used here to
   output code for -fpic that will load the return address into %ebx.  */

#undef ASM_FILE_END
#define ASM_FILE_END(FILE)  ix86_asm_file_end (FILE)

/* Output assembler code to FILE to increment profiler label # LABELNO
   for profiling a function entry.  */

#define FUNCTION_PROFILER(FILE, LABELNO)				\
do {									\
  if (flag_pic)								\
    {									\
      fprintf ((FILE), "\tleal\t%sP%d@GOTOFF(%%ebx),%%edx\n",		\
	       LPREFIX, (LABELNO));					\
      fprintf ((FILE), "\tcall\t*_mcount@GOT(%%ebx)\n");		\
    }									\
  else									\
    {									\
      fprintf ((FILE), "\tmovl\t$%sP%d,%%edx\n", LPREFIX, (LABELNO));	\
      fprintf ((FILE), "\tcall\t_mcount\n");				\
    }									\
} while (0)

/* EXIT_IGNORE_STACK should be nonzero if, when returning from a function,
   the stack pointer does not matter.  The value is tested only in
   functions that have frame pointers.
   No definition is equivalent to always zero.  */
/* Note on the 386 it might be more efficient not to define this since 
   we have to restore it ourselves from the frame pointer, in order to
   use pop */

#define EXIT_IGNORE_STACK 1

/* Output assembler code for a block containing the constant parts
   of a trampoline, leaving space for the variable parts.  */

/* On the 386, the trampoline contains two instructions:
     mov #STATIC,ecx
     jmp FUNCTION
   The trampoline is generated entirely at runtime.  The operand of JMP
   is the address of FUNCTION relative to the instruction following the
   JMP (which is 5 bytes long).  */

/* Length in units of the trampoline for entering a nested function.  */

#define TRAMPOLINE_SIZE (TARGET_64BIT ? 23 : 10)

/* Emit RTL insns to initialize the variable parts of a trampoline.
   FNADDR is an RTX for the address of the function's pure code.
   CXT is an RTX for the static chain value for the function.  */

#define INITIALIZE_TRAMPOLINE(TRAMP, FNADDR, CXT) \
  x86_initialize_trampoline ((TRAMP), (FNADDR), (CXT))

/* Definitions for register eliminations.

   This is an array of structures.  Each structure initializes one pair
   of eliminable registers.  The "from" register number is given first,
   followed by "to".  Eliminations of the same "from" register are listed
   in order of preference.

   There are two registers that can always be eliminated on the i386.
   The frame pointer and the arg pointer can be replaced by either the
   hard frame pointer or to the stack pointer, depending upon the
   circumstances.  The hard frame pointer is not used before reload and
   so it is not eligible for elimination.  */

#define ELIMINABLE_REGS					\
{{ ARG_POINTER_REGNUM, STACK_POINTER_REGNUM},		\
 { ARG_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM},	\
 { FRAME_POINTER_REGNUM, STACK_POINTER_REGNUM},		\
 { FRAME_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM}}	\

/* Given FROM and TO register numbers, say whether this elimination is
   allowed.  Frame pointer elimination is automatically handled.

   All other eliminations are valid.  */

#define CAN_ELIMINATE(FROM, TO) \
  ((TO) == STACK_POINTER_REGNUM ? ! frame_pointer_needed : 1)

/* Define the offset between two registers, one to be eliminated, and the other
   its replacement, at the start of a routine.  */

#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET) \
  ((OFFSET) = ix86_initial_elimination_offset ((FROM), (TO)))

/* Addressing modes, and classification of registers for them.  */

/* #define HAVE_POST_INCREMENT 0 */
/* #define HAVE_POST_DECREMENT 0 */

/* #define HAVE_PRE_DECREMENT 0 */
/* #define HAVE_PRE_INCREMENT 0 */

/* Macros to check register numbers against specific register classes.  */

/* These assume that REGNO is a hard or pseudo reg number.
   They give nonzero only if REGNO is a hard reg of the suitable class
   or a pseudo reg currently allocated to a suitable hard reg.
   Since they use reg_renumber, they are safe only once reg_renumber
   has been allocated, which happens in local-alloc.c.  */

#define REGNO_OK_FOR_INDEX_P(REGNO) 					\
  ((REGNO) < STACK_POINTER_REGNUM 					\
   || (REGNO >= FIRST_REX_INT_REG					\
       && (REGNO) <= LAST_REX_INT_REG)					\
   || ((unsigned) reg_renumber[(REGNO)] >= FIRST_REX_INT_REG		\
       && (unsigned) reg_renumber[(REGNO)] <= LAST_REX_INT_REG)		\
   || (unsigned) reg_renumber[(REGNO)] < STACK_POINTER_REGNUM)

#define REGNO_OK_FOR_BASE_P(REGNO) 					\
  ((REGNO) <= STACK_POINTER_REGNUM 					\
   || (REGNO) == ARG_POINTER_REGNUM 					\
   || (REGNO) == FRAME_POINTER_REGNUM 					\
   || (REGNO >= FIRST_REX_INT_REG					\
       && (REGNO) <= LAST_REX_INT_REG)					\
   || ((unsigned) reg_renumber[(REGNO)] >= FIRST_REX_INT_REG		\
       && (unsigned) reg_renumber[(REGNO)] <= LAST_REX_INT_REG)		\
   || (unsigned) reg_renumber[(REGNO)] <= STACK_POINTER_REGNUM)

#define REGNO_OK_FOR_SIREG_P(REGNO) \
  ((REGNO) == 4 || reg_renumber[(REGNO)] == 4)
#define REGNO_OK_FOR_DIREG_P(REGNO) \
  ((REGNO) == 5 || reg_renumber[(REGNO)] == 5)

/* The macros REG_OK_FOR..._P assume that the arg is a REG rtx
   and check its validity for a certain class.
   We have two alternate definitions for each of them.
   The usual definition accepts all pseudo regs; the other rejects
   them unless they have been allocated suitable hard regs.
   The symbol REG_OK_STRICT causes the latter definition to be used.

   Most source files want to accept pseudo regs in the hope that
   they will get allocated to the class that the insn wants them to be in.
   Source files for reload pass need to be strict.
   After reload, it makes no difference, since pseudo regs have
   been eliminated by then.  */


/* Non strict versions, pseudos are ok */
#define REG_OK_FOR_INDEX_NONSTRICT_P(X)					\
  (REGNO (X) < STACK_POINTER_REGNUM					\
   || (REGNO (X) >= FIRST_REX_INT_REG					\
       && REGNO (X) <= LAST_REX_INT_REG)				\
   || REGNO (X) >= FIRST_PSEUDO_REGISTER)

#define REG_OK_FOR_BASE_NONSTRICT_P(X)					\
  (REGNO (X) <= STACK_POINTER_REGNUM					\
   || REGNO (X) == ARG_POINTER_REGNUM					\
   || REGNO (X) == FRAME_POINTER_REGNUM 				\
   || (REGNO (X) >= FIRST_REX_INT_REG					\
       && REGNO (X) <= LAST_REX_INT_REG)				\
   || REGNO (X) >= FIRST_PSEUDO_REGISTER)

/* Strict versions, hard registers only */
#define REG_OK_FOR_INDEX_STRICT_P(X) REGNO_OK_FOR_INDEX_P (REGNO (X))
#define REG_OK_FOR_BASE_STRICT_P(X)  REGNO_OK_FOR_BASE_P (REGNO (X))

#ifndef REG_OK_STRICT
#define REG_OK_FOR_INDEX_P(X)  REG_OK_FOR_INDEX_NONSTRICT_P (X)
#define REG_OK_FOR_BASE_P(X)   REG_OK_FOR_BASE_NONSTRICT_P (X)

#else
#define REG_OK_FOR_INDEX_P(X)  REG_OK_FOR_INDEX_STRICT_P (X)
#define REG_OK_FOR_BASE_P(X)   REG_OK_FOR_BASE_STRICT_P (X)
#endif

/* GO_IF_LEGITIMATE_ADDRESS recognizes an RTL expression
   that is a valid memory address for an instruction.
   The MODE argument is the machine mode for the MEM expression
   that wants to use this address.

   The other macros defined here are used only in GO_IF_LEGITIMATE_ADDRESS,
   except for CONSTANT_ADDRESS_P which is usually machine-independent.

   See legitimize_pic_address in i386.c for details as to what
   constitutes a legitimate address when -fpic is used.  */

#define MAX_REGS_PER_ADDRESS 2

#define CONSTANT_ADDRESS_P(X)					\
  (GET_CODE (X) == LABEL_REF || GET_CODE (X) == SYMBOL_REF	\
   || GET_CODE (X) == CONST_INT || GET_CODE (X) == CONST	\
   || GET_CODE (X) == CONST_DOUBLE)

/* Nonzero if the constant value X is a legitimate general operand.
   It is given that X satisfies CONSTANT_P or is a CONST_DOUBLE.  */

#define LEGITIMATE_CONSTANT_P(X) 1

#ifdef REG_OK_STRICT
#define GO_IF_LEGITIMATE_ADDRESS(MODE, X, ADDR)				\
do {									\
  if (legitimate_address_p ((MODE), (X), 1))				\
    goto ADDR;								\
} while (0)

#else
#define GO_IF_LEGITIMATE_ADDRESS(MODE, X, ADDR)				\
do {									\
  if (legitimate_address_p ((MODE), (X), 0))				\
    goto ADDR;								\
} while (0)

#endif

/* If defined, a C expression to determine the base term of address X.
   This macro is used in only one place: `find_base_term' in alias.c.

   It is always safe for this macro to not be defined.  It exists so
   that alias analysis can understand machine-dependent addresses.

   The typical use of this macro is to handle addresses containing
   a label_ref or symbol_ref within an UNSPEC.  */

#define FIND_BASE_TERM(X) ix86_find_base_term (X)

/* Try machine-dependent ways of modifying an illegitimate address
   to be legitimate.  If we find one, return the new, valid address.
   This macro is used in only one place: `memory_address' in explow.c.

   OLDX is the address as it was before break_out_memory_refs was called.
   In some cases it is useful to look at this to decide what needs to be done.

   MODE and WIN are passed so that this macro can use
   GO_IF_LEGITIMATE_ADDRESS.

   It is always safe for this macro to do nothing.  It exists to recognize
   opportunities to optimize the output.

   For the 80386, we handle X+REG by loading X into a register R and
   using R+REG.  R will go in a general reg and indexing will be used.
   However, if REG is a broken-out memory address or multiplication,
   nothing needs to be done because REG can certainly go in a general reg.

   When -fpic is used, special handling is needed for symbolic references.
   See comments by legitimize_pic_address in i386.c for details.  */

#define LEGITIMIZE_ADDRESS(X, OLDX, MODE, WIN)				\
do {									\
  (X) = legitimize_address ((X), (OLDX), (MODE));			\
  if (memory_address_p ((MODE), (X)))					\
    goto WIN;								\
} while (0)

#define REWRITE_ADDRESS(X) rewrite_address (X)

/* Nonzero if the constant value X is a legitimate general operand
   when generating PIC code.  It is given that flag_pic is on and 
   that X satisfies CONSTANT_P or is a CONST_DOUBLE.  */

#define LEGITIMATE_PIC_OPERAND_P(X)		\
  (! SYMBOLIC_CONST (X)				\
   || legitimate_pic_address_disp_p (X))

#define SYMBOLIC_CONST(X)	\
  (GET_CODE (X) == SYMBOL_REF						\
   || GET_CODE (X) == LABEL_REF						\
   || (GET_CODE (X) == CONST && symbolic_reference_mentioned_p (X)))

/* Go to LABEL if ADDR (a legitimate address expression)
   has an effect that depends on the machine mode it is used for.
   On the 80386, only postdecrement and postincrement address depend thus
   (the amount of decrement or increment being the length of the operand).  */
#define GO_IF_MODE_DEPENDENT_ADDRESS(ADDR, LABEL)	\
do {							\
 if (GET_CODE (ADDR) == POST_INC			\
     || GET_CODE (ADDR) == POST_DEC)			\
   goto LABEL;						\
} while (0)

/* Codes for all the SSE/MMX builtins.  */
enum ix86_builtins
{
  IX86_BUILTIN_ADDPS,
  IX86_BUILTIN_ADDSS,
  IX86_BUILTIN_DIVPS,
  IX86_BUILTIN_DIVSS,
  IX86_BUILTIN_MULPS,
  IX86_BUILTIN_MULSS,
  IX86_BUILTIN_SUBPS,
  IX86_BUILTIN_SUBSS,

  IX86_BUILTIN_CMPEQPS,
  IX86_BUILTIN_CMPLTPS,
  IX86_BUILTIN_CMPLEPS,
  IX86_BUILTIN_CMPGTPS,
  IX86_BUILTIN_CMPGEPS,
  IX86_BUILTIN_CMPNEQPS,
  IX86_BUILTIN_CMPNLTPS,
  IX86_BUILTIN_CMPNLEPS,
  IX86_BUILTIN_CMPNGTPS,
  IX86_BUILTIN_CMPNGEPS,
  IX86_BUILTIN_CMPORDPS,
  IX86_BUILTIN_CMPUNORDPS,
  IX86_BUILTIN_CMPNEPS,
  IX86_BUILTIN_CMPEQSS,
  IX86_BUILTIN_CMPLTSS,
  IX86_BUILTIN_CMPLESS,
  IX86_BUILTIN_CMPGTSS,
  IX86_BUILTIN_CMPGESS,
  IX86_BUILTIN_CMPNEQSS,
  IX86_BUILTIN_CMPNLTSS,
  IX86_BUILTIN_CMPNLESS,
  IX86_BUILTIN_CMPNGTSS,
  IX86_BUILTIN_CMPNGESS,
  IX86_BUILTIN_CMPORDSS,
  IX86_BUILTIN_CMPUNORDSS,
  IX86_BUILTIN_CMPNESS,

  IX86_BUILTIN_COMIEQSS,
  IX86_BUILTIN_COMILTSS,
  IX86_BUILTIN_COMILESS,
  IX86_BUILTIN_COMIGTSS,
  IX86_BUILTIN_COMIGESS,
  IX86_BUILTIN_COMINEQSS,
  IX86_BUILTIN_UCOMIEQSS,
  IX86_BUILTIN_UCOMILTSS,
  IX86_BUILTIN_UCOMILESS,
  IX86_BUILTIN_UCOMIGTSS,
  IX86_BUILTIN_UCOMIGESS,
  IX86_BUILTIN_UCOMINEQSS,

  IX86_BUILTIN_CVTPI2PS,
  IX86_BUILTIN_CVTPS2PI,
  IX86_BUILTIN_CVTSI2SS,
  IX86_BUILTIN_CVTSS2SI,
  IX86_BUILTIN_CVTTPS2PI,
  IX86_BUILTIN_CVTTSS2SI,

  IX86_BUILTIN_MAXPS,
  IX86_BUILTIN_MAXSS,
  IX86_BUILTIN_MINPS,
  IX86_BUILTIN_MINSS,

  IX86_BUILTIN_LOADAPS,
  IX86_BUILTIN_LOADUPS,
  IX86_BUILTIN_STOREAPS,
  IX86_BUILTIN_STOREUPS,
  IX86_BUILTIN_LOADSS,
  IX86_BUILTIN_STORESS,
  IX86_BUILTIN_MOVSS,

  IX86_BUILTIN_MOVHLPS,
  IX86_BUILTIN_MOVLHPS,
  IX86_BUILTIN_LOADHPS,
  IX86_BUILTIN_LOADLPS,
  IX86_BUILTIN_STOREHPS,
  IX86_BUILTIN_STORELPS,

  IX86_BUILTIN_MASKMOVQ,
  IX86_BUILTIN_MOVMSKPS,
  IX86_BUILTIN_PMOVMSKB,

  IX86_BUILTIN_MOVNTPS,
  IX86_BUILTIN_MOVNTQ,

  IX86_BUILTIN_PACKSSWB,
  IX86_BUILTIN_PACKSSDW,
  IX86_BUILTIN_PACKUSWB,

  IX86_BUILTIN_PADDB,
  IX86_BUILTIN_PADDW,
  IX86_BUILTIN_PADDD,
  IX86_BUILTIN_PADDSB,
  IX86_BUILTIN_PADDSW,
  IX86_BUILTIN_PADDUSB,
  IX86_BUILTIN_PADDUSW,
  IX86_BUILTIN_PSUBB,
  IX86_BUILTIN_PSUBW,
  IX86_BUILTIN_PSUBD,
  IX86_BUILTIN_PSUBSB,
  IX86_BUILTIN_PSUBSW,
  IX86_BUILTIN_PSUBUSB,
  IX86_BUILTIN_PSUBUSW,

  IX86_BUILTIN_PAND,
  IX86_BUILTIN_PANDN,
  IX86_BUILTIN_POR,
  IX86_BUILTIN_PXOR,

  IX86_BUILTIN_PAVGB,
  IX86_BUILTIN_PAVGW,

  IX86_BUILTIN_PCMPEQB,
  IX86_BUILTIN_PCMPEQW,
  IX86_BUILTIN_PCMPEQD,
  IX86_BUILTIN_PCMPGTB,
  IX86_BUILTIN_PCMPGTW,
  IX86_BUILTIN_PCMPGTD,

  IX86_BUILTIN_PEXTRW,
  IX86_BUILTIN_PINSRW,

  IX86_BUILTIN_PMADDWD,

  IX86_BUILTIN_PMAXSW,
  IX86_BUILTIN_PMAXUB,
  IX86_BUILTIN_PMINSW,
  IX86_BUILTIN_PMINUB,

  IX86_BUILTIN_PMULHUW,
  IX86_BUILTIN_PMULHW,
  IX86_BUILTIN_PMULLW,

  IX86_BUILTIN_PSADBW,
  IX86_BUILTIN_PSHUFW,

  IX86_BUILTIN_PSLLW,
  IX86_BUILTIN_PSLLD,
  IX86_BUILTIN_PSLLQ,
  IX86_BUILTIN_PSRAW,
  IX86_BUILTIN_PSRAD,
  IX86_BUILTIN_PSRLW,
  IX86_BUILTIN_PSRLD,
  IX86_BUILTIN_PSRLQ,
  IX86_BUILTIN_PSLLWI,
  IX86_BUILTIN_PSLLDI,
  IX86_BUILTIN_PSLLQI,
  IX86_BUILTIN_PSRAWI,
  IX86_BUILTIN_PSRADI,
  IX86_BUILTIN_PSRLWI,
  IX86_BUILTIN_PSRLDI,
  IX86_BUILTIN_PSRLQI,

  IX86_BUILTIN_PUNPCKHBW,
  IX86_BUILTIN_PUNPCKHWD,
  IX86_BUILTIN_PUNPCKHDQ,
  IX86_BUILTIN_PUNPCKLBW,
  IX86_BUILTIN_PUNPCKLWD,
  IX86_BUILTIN_PUNPCKLDQ,

  IX86_BUILTIN_SHUFPS,

  IX86_BUILTIN_RCPPS,
  IX86_BUILTIN_RCPSS,
  IX86_BUILTIN_RSQRTPS,
  IX86_BUILTIN_RSQRTSS,
  IX86_BUILTIN_SQRTPS,
  IX86_BUILTIN_SQRTSS,
  
  IX86_BUILTIN_UNPCKHPS,
  IX86_BUILTIN_UNPCKLPS,

  IX86_BUILTIN_ANDPS,
  IX86_BUILTIN_ANDNPS,
  IX86_BUILTIN_ORPS,
  IX86_BUILTIN_XORPS,

  IX86_BUILTIN_EMMS,
  IX86_BUILTIN_LDMXCSR,
  IX86_BUILTIN_STMXCSR,
  IX86_BUILTIN_SFENCE,

  /* 3DNow! Original */
  IX86_BUILTIN_FEMMS,
  IX86_BUILTIN_PAVGUSB,
  IX86_BUILTIN_PF2ID,
  IX86_BUILTIN_PFACC,
  IX86_BUILTIN_PFADD,
  IX86_BUILTIN_PFCMPEQ,
  IX86_BUILTIN_PFCMPGE,
  IX86_BUILTIN_PFCMPGT,
  IX86_BUILTIN_PFMAX,
  IX86_BUILTIN_PFMIN,
  IX86_BUILTIN_PFMUL,
  IX86_BUILTIN_PFRCP,
  IX86_BUILTIN_PFRCPIT1,
  IX86_BUILTIN_PFRCPIT2,
  IX86_BUILTIN_PFRSQIT1,
  IX86_BUILTIN_PFRSQRT,
  IX86_BUILTIN_PFSUB,
  IX86_BUILTIN_PFSUBR,
  IX86_BUILTIN_PI2FD,
  IX86_BUILTIN_PMULHRW,

  /* 3DNow! Athlon Extensions */
  IX86_BUILTIN_PF2IW,
  IX86_BUILTIN_PFNACC,
  IX86_BUILTIN_PFPNACC,
  IX86_BUILTIN_PI2FW,
  IX86_BUILTIN_PSWAPDSI,
  IX86_BUILTIN_PSWAPDSF,

  IX86_BUILTIN_SSE_ZERO,
  IX86_BUILTIN_MMX_ZERO,

  IX86_BUILTIN_MAX
};

/* Define this macro if references to a symbol must be treated
   differently depending on something about the variable or
   function named by the symbol (such as what section it is in).

   On i386, if using PIC, mark a SYMBOL_REF for a non-global symbol
   so that we may access it directly in the GOT.  */

#define ENCODE_SECTION_INFO(DECL)				\
do {								\
    if (flag_pic)						\
      {								\
	rtx rtl = (TREE_CODE_CLASS (TREE_CODE (DECL)) != 'd'	\
		   ? TREE_CST_RTL (DECL) : DECL_RTL (DECL));	\
								\
	if (GET_CODE (rtl) == MEM)				\
	  {							\
	    if (TARGET_DEBUG_ADDR				\
		&& TREE_CODE_CLASS (TREE_CODE (DECL)) == 'd')	\
	      {							\
		fprintf (stderr, "Encode %s, public = %d\n",	\
			 IDENTIFIER_POINTER (DECL_NAME (DECL)),	\
			 TREE_PUBLIC (DECL));			\
	      }							\
	    							\
	    SYMBOL_REF_FLAG (XEXP (rtl, 0))			\
	      = (TREE_CODE_CLASS (TREE_CODE (DECL)) != 'd'	\
		 || ! TREE_PUBLIC (DECL));			\
	  }							\
      }								\
} while (0)

/* The `FINALIZE_PIC' macro serves as a hook to emit these special
   codes once the function is being compiled into assembly code, but
   not before.  (It is not done before, because in the case of
   compiling an inline function, it would lead to multiple PIC
   prologues being included in functions which used inline functions
   and were compiled to assembly language.)  */

#define FINALIZE_PIC \
  (current_function_uses_pic_offset_table |= current_function_profile)


/* Max number of args passed in registers.  If this is more than 3, we will
   have problems with ebx (register #4), since it is a caller save register and
   is also used as the pic register in ELF.  So for now, don't allow more than
   3 registers to be passed in registers.  */

#define REGPARM_MAX (TARGET_64BIT ? 6 : 3)

#define SSE_REGPARM_MAX (TARGET_64BIT ? 8 : 0)


/* Specify the machine mode that this machine uses
   for the index in the tablejump instruction.  */
#define CASE_VECTOR_MODE (!TARGET_64BIT || flag_pic ? SImode : DImode)

/* Define as C expression which evaluates to nonzero if the tablejump
   instruction expects the table to contain offsets from the address of the
   table.
   Do not define this if the table should contain absolute addresses.  */
/* #define CASE_VECTOR_PC_RELATIVE 1 */

/* Define this as 1 if `char' should by default be signed; else as 0.  */
#define DEFAULT_SIGNED_CHAR 1

/* Number of bytes moved into a data cache for a single prefetch operation.  */
#define PREFETCH_BLOCK ix86_cost->prefetch_block

/* Number of prefetch operations that can be done in parallel.  */
#define SIMULTANEOUS_PREFETCHES ix86_cost->simultaneous_prefetches

/* Max number of bytes we can move from memory to memory
   in one reasonably fast instruction.  */
#define MOVE_MAX 16

/* MOVE_MAX_PIECES is the number of bytes at a time which we can
   move efficiently, as opposed to  MOVE_MAX which is the maximum
   number of bytes we can move with a single instruction.  */
#define MOVE_MAX_PIECES (TARGET_64BIT ? 8 : 4)

/* If a memory-to-memory move would take MOVE_RATIO or more simple
   move-instruction pairs, we will do a movstr or libcall instead.
   Increasing the value will always make code faster, but eventually
   incurs high cost in increased code size.

   If you don't define this, a reasonable default is used.  */

#define MOVE_RATIO (optimize_size ? 3 : ix86_cost->move_ratio)

/* Define if shifts truncate the shift count
   which implies one can omit a sign-extension or zero-extension
   of a shift count.  */
/* On i386, shifts do truncate the count.  But bit opcodes don't.  */

/* #define SHIFT_COUNT_TRUNCATED */

/* Value is 1 if truncating an integer of INPREC bits to OUTPREC bits
   is done just by pretending it is already truncated.  */
#define TRULY_NOOP_TRUNCATION(OUTPREC, INPREC) 1

/* We assume that the store-condition-codes instructions store 0 for false
   and some other value for true.  This is the value stored for true.  */

#define STORE_FLAG_VALUE 1

/* When a prototype says `char' or `short', really pass an `int'.
   (The 386 can't easily push less than an int.)  */

#define PROMOTE_PROTOTYPES (!TARGET_64BIT)

/* A macro to update M and UNSIGNEDP when an object whose type is
   TYPE and which has the specified mode and signedness is to be
   stored in a register.  This macro is only called when TYPE is a
   scalar type.

   On i386 it is sometimes useful to promote HImode and QImode
   quantities to SImode.  The choice depends on target type.  */

#define PROMOTE_MODE(MODE, UNSIGNEDP, TYPE) 		\
do {							\
  if (((MODE) == HImode && TARGET_PROMOTE_HI_REGS)	\
      || ((MODE) == QImode && TARGET_PROMOTE_QI_REGS))	\
    (MODE) = SImode;					\
} while (0)

/* Specify the machine mode that pointers have.
   After generation of rtl, the compiler makes no further distinction
   between pointers and any other objects of this machine mode.  */
#define Pmode (TARGET_64BIT ? DImode : SImode)

/* A function address in a call instruction
   is a byte address (for indexing purposes)
   so give the MEM rtx a byte's mode.  */
#define FUNCTION_MODE QImode

/* A part of a C `switch' statement that describes the relative costs
   of constant RTL expressions.  It must contain `case' labels for
   expression codes `const_int', `const', `symbol_ref', `label_ref'
   and `const_double'.  Each case must ultimately reach a `return'
   statement to return the relative cost of the use of that kind of
   constant value in an expression.  The cost may depend on the
   precise value of the constant, which is available for examination
   in X, and the rtx code of the expression in which it is contained,
   found in OUTER_CODE.
  
   CODE is the expression code--redundant, since it can be obtained
   with `GET_CODE (X)'.  */

#define CONST_COSTS(RTX, CODE, OUTER_CODE)			\
  case CONST_INT:						\
  case CONST:							\
  case LABEL_REF:						\
  case SYMBOL_REF:						\
    if (TARGET_64BIT && !x86_64_sign_extended_value (RTX))	\
      return 3;							\
    if (TARGET_64BIT && !x86_64_zero_extended_value (RTX))	\
      return 2;							\
    return flag_pic && SYMBOLIC_CONST (RTX) ? 1 : 0;		\
								\
  case CONST_DOUBLE:						\
    {								\
      int code;							\
      if (GET_MODE (RTX) == VOIDmode)				\
	return 0;						\
								\
      code = standard_80387_constant_p (RTX);			\
      return code == 1 ? 1 :					\
	     code == 2 ? 2 :					\
			 3;					\
    }

/* Delete the definition here when TOPLEVEL_COSTS_N_INSNS gets added to cse.c */
#define TOPLEVEL_COSTS_N_INSNS(N) \
  do { total = COSTS_N_INSNS (N); goto egress_rtx_costs; } while (0)

/* Like `CONST_COSTS' but applies to nonconstant RTL expressions.
   This can be used, for example, to indicate how costly a multiply
   instruction is.  In writing this macro, you can use the construct
   `COSTS_N_INSNS (N)' to specify a cost equal to N fast
   instructions.  OUTER_CODE is the code of the expression in which X
   is contained.

   This macro is optional; do not define it if the default cost
   assumptions are adequate for the target machine.  */

#define RTX_COSTS(X, CODE, OUTER_CODE)					\
  case ZERO_EXTEND:							\
    /* The zero extensions is often completely free on x86_64, so make	\
       it as cheap as possible.  */					\
    if (TARGET_64BIT && GET_MODE (X) == DImode				\
	&& GET_MODE (XEXP (X, 0)) == SImode)				\
      {									\
	total = 1; goto egress_rtx_costs;				\
      } 								\
    else								\
      TOPLEVEL_COSTS_N_INSNS (TARGET_ZERO_EXTEND_WITH_AND ?		\
			      ix86_cost->add : ix86_cost->movzx);	\
    break;								\
  case SIGN_EXTEND:							\
    TOPLEVEL_COSTS_N_INSNS (ix86_cost->movsx);				\
    break;								\
  case ASHIFT:								\
    if (GET_CODE (XEXP (X, 1)) == CONST_INT				\
	&& (GET_MODE (XEXP (X, 0)) != DImode || TARGET_64BIT))		\
      {									\
	HOST_WIDE_INT value = INTVAL (XEXP (X, 1));			\
	if (value == 1)							\
	  TOPLEVEL_COSTS_N_INSNS (ix86_cost->add);			\
	if ((value == 2 || value == 3)					\
	    && !TARGET_DECOMPOSE_LEA					\
	    && ix86_cost->lea <= ix86_cost->shift_const)		\
	  TOPLEVEL_COSTS_N_INSNS (ix86_cost->lea);			\
      }									\
    /* fall through */							\
		  							\
  case ROTATE:								\
  case ASHIFTRT:							\
  case LSHIFTRT:							\
  case ROTATERT:							\
    if (!TARGET_64BIT && GET_MODE (XEXP (X, 0)) == DImode)		\
      {									\
	if (GET_CODE (XEXP (X, 1)) == CONST_INT)			\
	  {								\
	    if (INTVAL (XEXP (X, 1)) > 32)				\
	      TOPLEVEL_COSTS_N_INSNS(ix86_cost->shift_const + 2);	\
	    else							\
	      TOPLEVEL_COSTS_N_INSNS(ix86_cost->shift_const * 2);	\
	  }								\
	else								\
	  {								\
	    if (GET_CODE (XEXP (X, 1)) == AND)				\
	      TOPLEVEL_COSTS_N_INSNS(ix86_cost->shift_var * 2);		\
	    else							\
	      TOPLEVEL_COSTS_N_INSNS(ix86_cost->shift_var * 6 + 2);	\
	  }								\
      }									\
    else								\
      {									\
	if (GET_CODE (XEXP (X, 1)) == CONST_INT)			\
	  TOPLEVEL_COSTS_N_INSNS (ix86_cost->shift_const);		\
	else								\
	  TOPLEVEL_COSTS_N_INSNS (ix86_cost->shift_var);		\
      }									\
    break;								\
									\
  case MULT:								\
    if (GET_CODE (XEXP (X, 1)) == CONST_INT)				\
      {									\
	unsigned HOST_WIDE_INT value = INTVAL (XEXP (X, 1));		\
	int nbits = 0;							\
									\
	while (value != 0)						\
	  {								\
	    nbits++;							\
	    value >>= 1;						\
	  } 								\
									\
	TOPLEVEL_COSTS_N_INSNS (ix86_cost->mult_init			\
			        + nbits * ix86_cost->mult_bit);		\
      }									\
    else			/* This is arbitrary */			\
      TOPLEVEL_COSTS_N_INSNS (ix86_cost->mult_init			\
			      + 7 * ix86_cost->mult_bit);		\
									\
  case DIV:								\
  case UDIV:								\
  case MOD:								\
  case UMOD:								\
    TOPLEVEL_COSTS_N_INSNS (ix86_cost->divide);				\
									\
  case PLUS:								\
    if (!TARGET_DECOMPOSE_LEA						\
	&& INTEGRAL_MODE_P (GET_MODE (X))				\
	&& GET_MODE_BITSIZE (GET_MODE (X)) <= GET_MODE_BITSIZE (Pmode))	\
      {									\
        if (GET_CODE (XEXP (X, 0)) == PLUS				\
	    && GET_CODE (XEXP (XEXP (X, 0), 0)) == MULT			\
	    && GET_CODE (XEXP (XEXP (XEXP (X, 0), 0), 1)) == CONST_INT	\
	    && CONSTANT_P (XEXP (X, 1)))				\
	  {								\
	    HOST_WIDE_INT val = INTVAL (XEXP (XEXP (XEXP (X, 0), 0), 1));\
	    if (val == 2 || val == 4 || val == 8)			\
	      {								\
		return (COSTS_N_INSNS (ix86_cost->lea)			\
			+ rtx_cost (XEXP (XEXP (X, 0), 1),		\
				    (OUTER_CODE))			\
			+ rtx_cost (XEXP (XEXP (XEXP (X, 0), 0), 0),	\
				    (OUTER_CODE))			\
			+ rtx_cost (XEXP (X, 1), (OUTER_CODE)));	\
	      }								\
	  }								\
	else if (GET_CODE (XEXP (X, 0)) == MULT				\
		 && GET_CODE (XEXP (XEXP (X, 0), 1)) == CONST_INT)	\
	  {								\
	    HOST_WIDE_INT val = INTVAL (XEXP (XEXP (X, 0), 1));		\
	    if (val == 2 || val == 4 || val == 8)			\
	      {								\
		return (COSTS_N_INSNS (ix86_cost->lea)			\
			+ rtx_cost (XEXP (XEXP (X, 0), 0),		\
				    (OUTER_CODE))			\
			+ rtx_cost (XEXP (X, 1), (OUTER_CODE)));	\
	      }								\
	  }								\
	else if (GET_CODE (XEXP (X, 0)) == PLUS)			\
	  {								\
	    return (COSTS_N_INSNS (ix86_cost->lea)			\
		    + rtx_cost (XEXP (XEXP (X, 0), 0), (OUTER_CODE))	\
		    + rtx_cost (XEXP (XEXP (X, 0), 1), (OUTER_CODE))	\
		    + rtx_cost (XEXP (X, 1), (OUTER_CODE)));		\
	  }								\
      }									\
									\
    /* fall through */							\
  case AND:								\
  case IOR:								\
  case XOR:								\
  case MINUS:								\
    if (!TARGET_64BIT && GET_MODE (X) == DImode)			\
      return (COSTS_N_INSNS (ix86_cost->add) * 2			\
	      + (rtx_cost (XEXP (X, 0), (OUTER_CODE))			\
	         << (GET_MODE (XEXP (X, 0)) != DImode))			\
	      + (rtx_cost (XEXP (X, 1), (OUTER_CODE))			\
 	         << (GET_MODE (XEXP (X, 1)) != DImode)));		\
									\
    /* fall through */							\
  case NEG:								\
  case NOT:								\
    if (!TARGET_64BIT && GET_MODE (X) == DImode)			\
      TOPLEVEL_COSTS_N_INSNS (ix86_cost->add * 2);			\
    TOPLEVEL_COSTS_N_INSNS (ix86_cost->add);				\
									\
  egress_rtx_costs:							\
    break;


/* An expression giving the cost of an addressing mode that contains
   ADDRESS.  If not defined, the cost is computed from the ADDRESS
   expression and the `CONST_COSTS' values.

   For most CISC machines, the default cost is a good approximation
   of the true cost of the addressing mode.  However, on RISC
   machines, all instructions normally have the same length and
   execution time.  Hence all addresses will have equal costs.

   In cases where more than one form of an address is known, the form
   with the lowest cost will be used.  If multiple forms have the
   same, lowest, cost, the one that is the most complex will be used.

   For example, suppose an address that is equal to the sum of a
   register and a constant is used twice in the same basic block.
   When this macro is not defined, the address will be computed in a
   register and memory references will be indirect through that
   register.  On machines where the cost of the addressing mode
   containing the sum is no higher than that of a simple indirect
   reference, this will produce an additional instruction and
   possibly require an additional register.  Proper specification of
   this macro eliminates this overhead for such machines.

   Similar use of this macro is made in strength reduction of loops.

   ADDRESS need not be valid as an address.  In such a case, the cost
   is not relevant and can be any value; invalid addresses need not be
   assigned a different cost.

   On machines where an address involving more than one register is as
   cheap as an address computation involving only one register,
   defining `ADDRESS_COST' to reflect this can cause two registers to
   be live over a region of code where only one would have been if
   `ADDRESS_COST' were not defined in that manner.  This effect should
   be considered in the definition of this macro.  Equivalent costs
   should probably only be given to addresses with different numbers
   of registers on machines with lots of registers.

   This macro will normally either not be defined or be defined as a
   constant.

   For i386, it is better to use a complex address than let gcc copy
   the address into a reg and make a new pseudo.  But not if the address
   requires to two regs - that would mean more pseudos with longer
   lifetimes.  */

#define ADDRESS_COST(RTX) \
  ix86_address_cost (RTX)

/* A C expression for the cost of moving data from a register in class FROM to
   one in class TO.  The classes are expressed using the enumeration values
   such as `GENERAL_REGS'.  A value of 2 is the default; other values are
   interpreted relative to that.

   It is not required that the cost always equal 2 when FROM is the same as TO;
   on some machines it is expensive to move between registers if they are not
   general registers.  */

#define REGISTER_MOVE_COST(MODE, CLASS1, CLASS2) \
   ix86_register_move_cost ((MODE), (CLASS1), (CLASS2))

/* A C expression for the cost of moving data of mode M between a
   register and memory.  A value of 2 is the default; this cost is
   relative to those in `REGISTER_MOVE_COST'.

   If moving between registers and memory is more expensive than
   between two registers, you should define this macro to express the
   relative cost.  */

#define MEMORY_MOVE_COST(MODE, CLASS, IN)	\
  ix86_memory_move_cost ((MODE), (CLASS), (IN))

/* A C expression for the cost of a branch instruction.  A value of 1
   is the default; other values are interpreted relative to that.  */

#define BRANCH_COST ix86_branch_cost

/* Define this macro as a C expression which is nonzero if accessing
   less than a word of memory (i.e. a `char' or a `short') is no
   faster than accessing a word of memory, i.e., if such access
   require more than one instruction or if there is no difference in
   cost between byte and (aligned) word loads.

   When this macro is not defined, the compiler will access a field by
   finding the smallest containing object; when it is defined, a
   fullword load will be used if alignment permits.  Unless bytes
   accesses are faster than word accesses, using word accesses is
   preferable since it may eliminate subsequent memory access if
   subsequent accesses occur to other fields in the same word of the
   structure, but to different bytes.  */

#define SLOW_BYTE_ACCESS 0

/* Nonzero if access to memory by shorts is slow and undesirable.  */
#define SLOW_SHORT_ACCESS 0

/* Define this macro to be the value 1 if unaligned accesses have a
   cost many times greater than aligned accesses, for example if they
   are emulated in a trap handler.

   When this macro is non-zero, the compiler will act as if
   `STRICT_ALIGNMENT' were non-zero when generating code for block
   moves.  This can cause significantly more instructions to be
   produced.  Therefore, do not set this macro non-zero if unaligned
   accesses only add a cycle or two to the time for a memory access.

   If the value of this macro is always zero, it need not be defined.  */

/* #define SLOW_UNALIGNED_ACCESS(MODE, ALIGN) 0 */

/* Define this macro to inhibit strength reduction of memory
   addresses.  (On some machines, such strength reduction seems to do
   harm rather than good.)  */

/* #define DONT_REDUCE_ADDR */

/* Define this macro if it is as good or better to call a constant
   function address than to call an address kept in a register.

   Desirable on the 386 because a CALL with a constant address is
   faster than one with a register address.  */

#define NO_FUNCTION_CSE

/* Define this macro if it is as good or better for a function to call
   itself with an explicit address than to call an address kept in a
   register.  */

#define NO_RECURSIVE_FUNCTION_CSE

/* Add any extra modes needed to represent the condition code.

   For the i386, we need separate modes when floating-point
   equality comparisons are being done. 
   
   Add CCNO to indicate comparisons against zero that requires
   Overflow flag to be unset.  Sign bit test is used instead and
   thus can be used to form "a&b>0" type of tests.

   Add CCGC to indicate comparisons agains zero that allows
   unspecified garbage in the Carry flag.  This mode is used
   by inc/dec instructions.

   Add CCGOC to indicate comparisons agains zero that allows
   unspecified garbage in the Carry and Overflow flag. This
   mode is used to simulate comparisons of (a-b) and (a+b)
   against zero using sub/cmp/add operations.

   Add CCZ to indicate that only the Zero flag is valid.  */

#define EXTRA_CC_MODES		\
	CC (CCGCmode, "CCGC")	\
	CC (CCGOCmode, "CCGOC")	\
	CC (CCNOmode, "CCNO")	\
	CC (CCZmode, "CCZ")	\
	CC (CCFPmode, "CCFP")	\
	CC (CCFPUmode, "CCFPU")

/* Given a comparison code (EQ, NE, etc.) and the first operand of a COMPARE,
   return the mode to be used for the comparison.

   For floating-point equality comparisons, CCFPEQmode should be used.
   VOIDmode should be used in all other cases.

   For integer comparisons against zero, reduce to CCNOmode or CCZmode if
   possible, to allow for more combinations.  */

#define SELECT_CC_MODE(OP, X, Y) ix86_cc_mode ((OP), (X), (Y))

/* Return non-zero if MODE implies a floating point inequality can be
   reversed.  */

#define REVERSIBLE_CC_MODE(MODE) 1

/* A C expression whose value is reversed condition code of the CODE for
   comparison done in CC_MODE mode.  */
#define REVERSE_CONDITION(CODE, MODE) \
  ((MODE) != CCFPmode && (MODE) != CCFPUmode ? reverse_condition (CODE) \
   : reverse_condition_maybe_unordered (CODE))


/* Control the assembler format that we output, to the extent
   this does not vary between assemblers.  */

/* How to refer to registers in assembler output.
   This sequence is indexed by compiler's hard-register-number (see above).  */

/* In order to refer to the first 8 regs as 32 bit regs prefix an "e"
   For non floating point regs, the following are the HImode names.

   For float regs, the stack top is sometimes referred to as "%st(0)"
   instead of just "%st".  PRINT_REG handles this with the "y" code.  */

#undef  HI_REGISTER_NAMES						
#define HI_REGISTER_NAMES						\
{"ax","dx","cx","bx","si","di","bp","sp",				\
 "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)","",	\
 "flags","fpsr", "dirflag", "frame",					\
 "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7",		\
 "mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7"	,		\
 "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",			\
 "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"}

#define REGISTER_NAMES HI_REGISTER_NAMES

/* Table of additional register names to use in user input.  */

#define ADDITIONAL_REGISTER_NAMES \
{ { "eax", 0 }, { "edx", 1 }, { "ecx", 2 }, { "ebx", 3 },	\
  { "esi", 4 }, { "edi", 5 }, { "ebp", 6 }, { "esp", 7 },	\
  { "rax", 0 }, { "rdx", 1 }, { "rcx", 2 }, { "rbx", 3 },	\
  { "rsi", 4 }, { "rdi", 5 }, { "rbp", 6 }, { "rsp", 7 },	\
  { "al", 0 }, { "dl", 1 }, { "cl", 2 }, { "bl", 3 },		\
  { "ah", 0 }, { "dh", 1 }, { "ch", 2 }, { "bh", 3 },		\
  { "mm0", 8},  { "mm1", 9},  { "mm2", 10}, { "mm3", 11},	\
  { "mm4", 12}, { "mm5", 13}, { "mm6", 14}, { "mm7", 15} }

/* Note we are omitting these since currently I don't know how
to get gcc to use these, since they want the same but different
number as al, and ax.
*/

#define QI_REGISTER_NAMES \
{"al", "dl", "cl", "bl", "sil", "dil", "bpl", "spl",}

/* These parallel the array above, and can be used to access bits 8:15
   of regs 0 through 3.  */

#define QI_HIGH_REGISTER_NAMES \
{"ah", "dh", "ch", "bh", }

/* How to renumber registers for dbx and gdb.  */

#define DBX_REGISTER_NUMBER(N) \
  (TARGET_64BIT ? dbx64_register_map[(N)] : dbx_register_map[(N)])

extern int const dbx_register_map[FIRST_PSEUDO_REGISTER];
extern int const dbx64_register_map[FIRST_PSEUDO_REGISTER];
extern int const svr4_dbx_register_map[FIRST_PSEUDO_REGISTER];

/* Before the prologue, RA is at 0(%esp).  */
#define INCOMING_RETURN_ADDR_RTX \
  gen_rtx_MEM (VOIDmode, gen_rtx_REG (VOIDmode, STACK_POINTER_REGNUM))
 
/* After the prologue, RA is at -4(AP) in the current frame.  */
#define RETURN_ADDR_RTX(COUNT, FRAME)					   \
  ((COUNT) == 0								   \
   ? gen_rtx_MEM (Pmode, plus_constant (arg_pointer_rtx, -UNITS_PER_WORD)) \
   : gen_rtx_MEM (Pmode, plus_constant (FRAME, UNITS_PER_WORD)))

/* PC is dbx register 8; let's use that column for RA.  */
#define DWARF_FRAME_RETURN_COLUMN 	(TARGET_64BIT ? 16 : 8)

/* Before the prologue, the top of the frame is at 4(%esp).  */
#define INCOMING_FRAME_SP_OFFSET UNITS_PER_WORD

/* Describe how we implement __builtin_eh_return.  */
#define EH_RETURN_DATA_REGNO(N)	((N) < 2 ? (N) : INVALID_REGNUM)
#define EH_RETURN_STACKADJ_RTX	gen_rtx_REG (Pmode, 2)


/* Select a format to encode pointers in exception handling data.  CODE
   is 0 for data, 1 for code labels, 2 for function pointers.  GLOBAL is
   true if the symbol may be affected by dynamic relocations.

   ??? All x86 object file formats are capable of representing this.
   After all, the relocation needed is the same as for the call insn.
   Whether or not a particular assembler allows us to enter such, I
   guess we'll have to see.  */
#define ASM_PREFERRED_EH_DATA_FORMAT(CODE, GLOBAL)       		\
  (flag_pic								\
    ? ((GLOBAL) ? DW_EH_PE_indirect : 0) | DW_EH_PE_pcrel | DW_EH_PE_sdata4\
   : DW_EH_PE_absptr)

/* This is how to output the definition of a user-level label named NAME,
   such as the label on a static function or variable NAME.  */

#define ASM_OUTPUT_LABEL(FILE, NAME)	\
  (assemble_name ((FILE), (NAME)), fputs (":\n", (FILE)))

/* Store in OUTPUT a string (made with alloca) containing
   an assembler-name for a local static variable named NAME.
   LABELNO is an integer which is different for each call.  */

#define ASM_FORMAT_PRIVATE_NAME(OUTPUT, NAME, LABELNO)	\
( (OUTPUT) = (char *) alloca (strlen ((NAME)) + 10),	\
  sprintf ((OUTPUT), "%s.%d", (NAME), (LABELNO)))

/* This is how to output an insn to push a register on the stack.
   It need not be very fast code.  */

#define ASM_OUTPUT_REG_PUSH(FILE, REGNO)  \
  asm_fprintf ((FILE), "\tpush{l}\t%%e%s\n", reg_names[(REGNO)])

/* This is how to output an insn to pop a register from the stack.
   It need not be very fast code.  */

#define ASM_OUTPUT_REG_POP(FILE, REGNO)  \
  asm_fprintf ((FILE), "\tpop{l}\t%%e%s\n", reg_names[(REGNO)])

/* This is how to output an element of a case-vector that is absolute.  */

#define ASM_OUTPUT_ADDR_VEC_ELT(FILE, VALUE)  \
  ix86_output_addr_vec_elt ((FILE), (VALUE))

/* This is how to output an element of a case-vector that is relative.  */

#define ASM_OUTPUT_ADDR_DIFF_ELT(FILE, BODY, VALUE, REL) \
  ix86_output_addr_diff_elt ((FILE), (VALUE), (REL))

/* Under some conditions we need jump tables in the text section, because
   the assembler cannot handle label differences between sections.  */

#define JUMP_TABLES_IN_TEXT_SECTION \
  (!TARGET_64BIT && flag_pic && !HAVE_AS_GOTOFF_IN_DATA)

/* A C statement that outputs an address constant appropriate to 
   for DWARF debugging.  */

#define ASM_OUTPUT_DWARF_ADDR_CONST(FILE, X) \
  i386_dwarf_output_addr_const ((FILE), (X))

/* Either simplify a location expression, or return the original.  */

#define ASM_SIMPLIFY_DWARF_ADDR(X) \
  i386_simplify_dwarf_addr (X)

/* Switch to init or fini section via SECTION_OP, emit a call to FUNC,
   and switch back.  For x86 we do this only to save a few bytes that
   would otherwise be unused in the text section.  */
#define CRT_CALL_STATIC_FUNCTION(SECTION_OP, FUNC)	\
   asm (SECTION_OP "\n\t"				\
	"call " USER_LABEL_PREFIX #FUNC "\n"		\
	TEXT_SECTION_ASM_OP);

/* Print operand X (an rtx) in assembler syntax to file FILE.
   CODE is a letter or dot (`z' in `%z0') or 0 if no letter was specified.
   Effect of various CODE letters is described in i386.c near
   print_operand function.  */

#define PRINT_OPERAND_PUNCT_VALID_P(CODE) \
  ((CODE) == '*' || (CODE) == '+')

/* Print the name of a register based on its machine mode and number.
   If CODE is 'w', pretend the mode is HImode.
   If CODE is 'b', pretend the mode is QImode.
   If CODE is 'k', pretend the mode is SImode.
   If CODE is 'q', pretend the mode is DImode.
   If CODE is 'h', pretend the reg is the `high' byte register.
   If CODE is 'y', print "st(0)" instead of "st", if the reg is stack op.  */

#define PRINT_REG(X, CODE, FILE)  \
  print_reg ((X), (CODE), (FILE))

#define PRINT_OPERAND(FILE, X, CODE)  \
  print_operand ((FILE), (X), (CODE))

#define PRINT_OPERAND_ADDRESS(FILE, ADDR)  \
  print_operand_address ((FILE), (ADDR))

/* Print the name of a register for based on its machine mode and number.
   This macro is used to print debugging output.
   This macro is different from PRINT_REG in that it may be used in
   programs that are not linked with aux-output.o.  */

#define DEBUG_PRINT_REG(X, CODE, FILE)			\
  do { static const char * const hi_name[] = HI_REGISTER_NAMES;	\
       static const char * const qi_name[] = QI_REGISTER_NAMES;	\
       fprintf ((FILE), "%d ", REGNO (X));		\
       if (REGNO (X) == FLAGS_REG)			\
	 { fputs ("flags", (FILE)); break; }		\
       if (REGNO (X) == DIRFLAG_REG)			\
	 { fputs ("dirflag", (FILE)); break; }		\
       if (REGNO (X) == FPSR_REG)			\
	 { fputs ("fpsr", (FILE)); break; }		\
       if (REGNO (X) == ARG_POINTER_REGNUM)		\
	 { fputs ("argp", (FILE)); break; }		\
       if (REGNO (X) == FRAME_POINTER_REGNUM)		\
	 { fputs ("frame", (FILE)); break; }		\
       if (STACK_TOP_P (X))				\
	 { fputs ("st(0)", (FILE)); break; }		\
       if (FP_REG_P (X))				\
	 { fputs (hi_name[REGNO(X)], (FILE)); break; }	\
       if (REX_INT_REG_P (X))				\
	 {						\
	   switch (GET_MODE_SIZE (GET_MODE (X)))	\
	     {						\
	     default:					\
	     case 8:					\
	       fprintf ((FILE), "r%i", REGNO (X)	\
			- FIRST_REX_INT_REG + 8);	\
	       break;					\
	     case 4:					\
	       fprintf ((FILE), "r%id", REGNO (X)	\
			- FIRST_REX_INT_REG + 8);	\
	       break;					\
	     case 2:					\
	       fprintf ((FILE), "r%iw", REGNO (X)	\
			- FIRST_REX_INT_REG + 8);	\
	       break;					\
	     case 1:					\
	       fprintf ((FILE), "r%ib", REGNO (X)	\
			- FIRST_REX_INT_REG + 8);	\
	       break;					\
	     }						\
	   break;					\
	 }						\
       switch (GET_MODE_SIZE (GET_MODE (X)))		\
	 {						\
	 case 8:					\
	   fputs ("r", (FILE));				\
	   fputs (hi_name[REGNO (X)], (FILE));		\
	   break;					\
	 default:					\
	   fputs ("e", (FILE));				\
	 case 2:					\
	   fputs (hi_name[REGNO (X)], (FILE));		\
	   break;					\
	 case 1:					\
	   fputs (qi_name[REGNO (X)], (FILE));		\
	   break;					\
	 }						\
     } while (0)

/* a letter which is not needed by the normal asm syntax, which
   we can use for operand syntax in the extended asm */

#define ASM_OPERAND_LETTER '#'
#define RET return ""
#define AT_SP(MODE) (gen_rtx_MEM ((MODE), stack_pointer_rtx))

/* Define the codes that are matched by predicates in i386.c.  */

#define PREDICATE_CODES							\
  {"x86_64_immediate_operand", {CONST_INT, SUBREG, REG,			\
				SYMBOL_REF, LABEL_REF, CONST}},		\
  {"x86_64_nonmemory_operand", {CONST_INT, SUBREG, REG,			\
				SYMBOL_REF, LABEL_REF, CONST}},		\
  {"x86_64_movabs_operand", {CONST_INT, SUBREG, REG,			\
				SYMBOL_REF, LABEL_REF, CONST}},		\
  {"x86_64_szext_nonmemory_operand", {CONST_INT, SUBREG, REG,		\
				     SYMBOL_REF, LABEL_REF, CONST}},	\
  {"x86_64_general_operand", {CONST_INT, SUBREG, REG, MEM,		\
			      SYMBOL_REF, LABEL_REF, CONST}},		\
  {"x86_64_szext_general_operand", {CONST_INT, SUBREG, REG, MEM,	\
				   SYMBOL_REF, LABEL_REF, CONST}},	\
  {"x86_64_zext_immediate_operand", {CONST_INT, CONST_DOUBLE, CONST,	\
				       SYMBOL_REF, LABEL_REF}},		\
  {"shiftdi_operand", {SUBREG, REG, MEM}},				\
  {"const_int_1_operand", {CONST_INT}},					\
  {"symbolic_operand", {SYMBOL_REF, LABEL_REF, CONST}},			\
  {"aligned_operand", {CONST_INT, CONST_DOUBLE, CONST, SYMBOL_REF,	\
		       LABEL_REF, SUBREG, REG, MEM}},			\
  {"pic_symbolic_operand", {CONST}},					\
  {"call_insn_operand", {REG, SUBREG, MEM, SYMBOL_REF}},		\
  {"constant_call_address_operand", {SYMBOL_REF, CONST}},		\
  {"const0_operand", {CONST_INT, CONST_DOUBLE}},			\
  {"const1_operand", {CONST_INT}},					\
  {"const248_operand", {CONST_INT}},					\
  {"incdec_operand", {CONST_INT}},					\
  {"mmx_reg_operand", {REG}},						\
  {"reg_no_sp_operand", {SUBREG, REG}},					\
  {"general_no_elim_operand", {CONST_INT, CONST_DOUBLE, CONST,		\
			SYMBOL_REF, LABEL_REF, SUBREG, REG, MEM}},	\
  {"nonmemory_no_elim_operand", {CONST_INT, REG, SUBREG}},		\
  {"q_regs_operand", {SUBREG, REG}},					\
  {"non_q_regs_operand", {SUBREG, REG}},				\
  {"fcmov_comparison_operator", {EQ, NE, LTU, GTU, LEU, GEU, UNORDERED, \
				 ORDERED, LT, UNLT, GT, UNGT, LE, UNLE,	\
				 GE, UNGE, LTGT, UNEQ}},		\
  {"sse_comparison_operator", {EQ, LT, LE, UNORDERED, NE, UNGE, UNGT,	\
			       ORDERED, UNEQ, UNLT, UNLE, LTGT, GE, GT	\
			       }},					\
  {"ix86_comparison_operator", {EQ, NE, LE, LT, GE, GT, LEU, LTU, GEU,	\
			       GTU, UNORDERED, ORDERED, UNLE, UNLT,	\
			       UNGE, UNGT, LTGT, UNEQ }},		\
  {"cmp_fp_expander_operand", {CONST_DOUBLE, SUBREG, REG, MEM}},	\
  {"ext_register_operand", {SUBREG, REG}},				\
  {"binary_fp_operator", {PLUS, MINUS, MULT, DIV}},			\
  {"mult_operator", {MULT}},						\
  {"div_operator", {DIV}},						\
  {"arith_or_logical_operator", {PLUS, MULT, AND, IOR, XOR, SMIN, SMAX, \
				 UMIN, UMAX, COMPARE, MINUS, DIV, MOD,	\
				 UDIV, UMOD, ASHIFT, ROTATE, ASHIFTRT,	\
				 LSHIFTRT, ROTATERT}},			\
  {"promotable_binary_operator", {PLUS, MULT, AND, IOR, XOR, ASHIFT}},	\
  {"memory_displacement_operand", {MEM}},				\
  {"cmpsi_operand", {CONST_INT, CONST_DOUBLE, CONST, SYMBOL_REF,	\
		     LABEL_REF, SUBREG, REG, MEM, AND}},		\
  {"long_memory_operand", {MEM}},

/* A list of predicates that do special things with modes, and so
   should not elicit warnings for VOIDmode match_operand.  */

#define SPECIAL_MODE_PREDICATES \
  "ext_register_operand",

/* CM_32 is used by 32bit ABI
   CM_SMALL is small model assuming that all code and data fits in the first
   31bits of address space.
   CM_KERNEL is model assuming that all code and data fits in the negative
   31bits of address space.
   CM_MEDIUM is model assuming that code fits in the first 31bits of address
   space.  Size of data is unlimited.
   CM_LARGE is model making no assumptions about size of particular sections.
  
   CM_SMALL_PIC is model for PIC libraries assuming that code+data+got/plt
   tables first in 31bits of address space.
 */
enum cmodel {
  CM_32,
  CM_SMALL,
  CM_KERNEL,
  CM_MEDIUM,
  CM_LARGE,
  CM_SMALL_PIC
};

/* Size of the RED_ZONE area.  */
#define RED_ZONE_SIZE 128
/* Reserved area of the red zone for temporaries.  */
#define RED_ZONE_RESERVE 8
extern const char *ix86_debug_arg_string, *ix86_debug_addr_string;

enum asm_dialect {
  ASM_ATT,
  ASM_INTEL
};
extern const char *ix86_asm_string;
extern enum asm_dialect ix86_asm_dialect;
/* Value of -mcmodel specified by user.  */
extern const char *ix86_cmodel_string;
extern enum cmodel ix86_cmodel;

/* Variables in i386.c */
extern const char *ix86_cpu_string;		/* for -mcpu=<xxx> */
extern const char *ix86_arch_string;		/* for -march=<xxx> */
extern const char *ix86_fpmath_string;		/* for -mfpmath=<xxx> */
extern const char *ix86_regparm_string;		/* # registers to use to pass args */
extern const char *ix86_align_loops_string;	/* power of two alignment for loops */
extern const char *ix86_align_jumps_string;	/* power of two alignment for non-loop jumps */
extern const char *ix86_align_funcs_string;	/* power of two alignment for functions */
extern const char *ix86_preferred_stack_boundary_string;/* power of two alignment for stack boundary */
extern const char *ix86_branch_cost_string;	/* values 1-5: see jump.c */
extern int ix86_regparm;			/* ix86_regparm_string as a number */
extern int ix86_preferred_stack_boundary;	/* preferred stack boundary alignment in bits */
extern int ix86_branch_cost;			/* values 1-5: see jump.c */
extern enum reg_class const regclass_map[FIRST_PSEUDO_REGISTER]; /* smalled class containing REGNO */
// Commented out the following two lines due to lack of definition for "rtx" - Brian
//extern rtx ix86_compare_op0;	/* operand 0 for comparisons */
//extern rtx ix86_compare_op1;	/* operand 1 for comparisons */

/* To properly truncate FP values into integers, we need to set i387 control
   word.  We can't emit proper mode switching code before reload, as spills
   generated by reload may truncate values incorrectly, but we still can avoid
   redundant computation of new control word by the mode switching pass.
   The fldcw instructions are still emitted redundantly, but this is probably
   not going to be noticeable problem, as most CPUs do have fast path for
   the sequence.  

   The machinery is to emit simple truncation instructions and split them
   before reload to instructions having USEs of two memory locations that
   are filled by this code to old and new control word.
 
   Post-reload pass may be later used to eliminate the redundant fildcw if
   needed.  */

enum fp_cw_mode {FP_CW_STORED, FP_CW_UNINITIALIZED, FP_CW_ANY};

/* Define this macro if the port needs extra instructions inserted
   for mode switching in an optimizing compilation.  */

#define OPTIMIZE_MODE_SWITCHING(ENTITY) 1

/* If you define `OPTIMIZE_MODE_SWITCHING', you have to define this as
   initializer for an array of integers.  Each initializer element N
   refers to an entity that needs mode switching, and specifies the
   number of different modes that might need to be set for this
   entity.  The position of the initializer in the initializer -
   starting counting at zero - determines the integer that is used to
   refer to the mode-switched entity in question.  */

#define NUM_MODES_FOR_MODE_SWITCHING { FP_CW_ANY }

/* ENTITY is an integer specifying a mode-switched entity.  If
   `OPTIMIZE_MODE_SWITCHING' is defined, you must define this macro to
   return an integer value not larger than the corresponding element
   in `NUM_MODES_FOR_MODE_SWITCHING', to denote the mode that ENTITY
   must be switched into prior to the execution of INSN.  */

#define MODE_NEEDED(ENTITY, I)						\
  (GET_CODE (I) == CALL_INSN						\
   || (GET_CODE (I) == INSN && (asm_noperands (PATTERN (I)) >= 0 	\
				|| GET_CODE (PATTERN (I)) == ASM_INPUT))\
   ? FP_CW_UNINITIALIZED						\
   : recog_memoized (I) < 0 || get_attr_type (I) != TYPE_FISTP		\
   ? FP_CW_ANY								\
   : FP_CW_STORED)

/* This macro specifies the order in which modes for ENTITY are
   processed.  0 is the highest priority.  */

#define MODE_PRIORITY_TO_MODE(ENTITY, N) (N)

/* Generate one or more insns to set ENTITY to MODE.  HARD_REG_LIVE
   is the set of hard registers live at the point where the insn(s)
   are to be inserted.  */

#define EMIT_MODE_SET(ENTITY, MODE, HARD_REGS_LIVE) 			\
  ((MODE) == FP_CW_STORED						\
   ? emit_i387_cw_initialization (assign_386_stack_local (HImode, 1),	\
				  assign_386_stack_local (HImode, 2)), 0\
   : 0)

/* Avoid renaming of stack registers, as doing so in combination with
   scheduling just increases amount of live registers at time and in
   the turn amount of fxch instructions needed.

   ??? Maybe Pentium chips benefits from renaming, someone can try...  */

#define HARD_REGNO_RENAME_OK(SRC, TARGET)  \
   ((SRC) < FIRST_STACK_REG || (SRC) > LAST_STACK_REG)


/*
Local variables:
version-control: t
End:
*/
