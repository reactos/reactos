
#ifndef SOFTX86_H
#define SOFTX86_H

/* get the Softx86 configuration defines */
#include <softx86cfg.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// version composed as follows: H.LL.SSSS
// where: H = major version number (any length)
//        L = minor version number (2 digits)
//        S = sub-minor version number (4 digits)
//
// The context structure is only allowed to change if the major (high)
// version number changes.
//
// This is v0.00.0033
#define SOFTX86_VERSION_HI		0
#define SOFTX86_VERSION_LO		0
#define SOFTX86_VERSION_SUBLO		33

#ifdef __MINGW32__  // MINGW

typedef unsigned char				sx86_ubyte;
typedef signed char				sx86_sbyte;
typedef unsigned short int			sx86_uword;
typedef signed short int			sx86_sword;
typedef unsigned int				sx86_udword;
typedef signed int				sx86_sdword;
typedef unsigned long long int			sx86_uldword;
typedef signed long long int			sx86_sldword;

#elif WIN32		// MSVC

typedef unsigned char				sx86_ubyte;
typedef signed char				sx86_sbyte;
typedef unsigned short int			sx86_uword;
typedef signed short int			sx86_sword;
typedef unsigned int				sx86_udword;
typedef signed int				sx86_sdword;
typedef unsigned __int64			sx86_uldword;
typedef signed __int64				sx86_sldword;

#else				// GCC 3.xx.xx

typedef unsigned char				sx86_ubyte;
typedef signed char				sx86_sbyte;
typedef unsigned short int			sx86_uword;
typedef signed short int			sx86_sword;
typedef unsigned int				sx86_udword;
typedef signed int				sx86_sdword;
typedef unsigned long long int			sx86_uldword;
typedef signed long long int			sx86_sldword;

#endif

/* NOTE: All registers contain the CPU values in native byte order NOT little
         Endian byte order! When loading values from RAM always use
	 the SWAP_WORD_FROM_LE() macro! */
typedef union {
	sx86_udword		val;			/* the entire 32-bit register as a whole (i.e. EAX) */

#if SX86_BYTE_ORDER == LE

	struct __dummy_w {
		sx86_uword	lo;			/* lower 16 bits of register (i.e. AX) */
		sx86_uword	hi;			/* upper 16 bits of register */
	} w;

	struct __dummy_b {
		sx86_ubyte	lo;			/* lower 8 bits of register (i.e. AL) */
		sx86_ubyte	hi;			/* bits 8-15 of register (i.e. AH) */
		sx86_ubyte	extra[2];
	} b;

#elif SX86_BYTE_ORDER == BE	/* member order reversed from LE because byte order reversed */

	struct __dummy_w {
		sx86_uword	hi;			/* upper 16 bits of register */
		sx86_uword	lo;			/* lower 16 bits of register (i.e. AX) */
	} w;

	struct __dummy_b {
		sx86_ubyte	extra[2];
		sx86_ubyte	hi;			/* bits 8-15 of register (i.e. AH) */
		sx86_ubyte	lo;			/* lower 8 bits of register (i.e. AL) */
	} b;

#endif

} softx86_regval;

typedef struct {
	sx86_uword		val;			/* visible to program---the value */
	sx86_udword		cached_linear;		/* hidden---linear address of segment */
	sx86_udword		cached_limit;		/* hidden---limit of segment */
} softx86_segregval;

/*============================================================================
   CPU state variables.

   All segment register values are handled in little endian order.

   WARNING: Also this structure will change over time to accomodate the
            "hidden portions" of registers that, like the real thing,
	    are used to avoid recalculating values that hardly change
	    anyway (such as the segment register to linear address conversions).

   WARNING: This structure is likely to change constantly over time, do not
            rely on this structure staying the same from version to version.
	    THE VERSION NUMBERS ARE THERE TO HELP YOU WITH THIS.

04-21-2003: Jonathan Campbell <jcampbell@mdjk.com>
            Registers are now represented by a C union rather than a word
	    value. This makes referring to upper and lower parts (i.e.
	    AL or AH in AX or AX in EAX) easier to code because the members
	    can be reordered based on the byte order of the platform we are
	    running on. Thanks to Paul Muller for this idea!

            All general-purpose registers are now stowed in one big array.
	    Use the constants SX86_REG_AX, SX86_REG_BX... to refer to it.
*/

/* This enumeration refers to indices within softx86_ctx.state.general_reg[] */
enum softx86_genregidx16 {
	SX86_REG_AX=0,
	SX86_REG_CX,
	SX86_REG_DX,
	SX86_REG_BX,
	SX86_REG_SP,
	SX86_REG_BP,
	SX86_REG_SI,
	SX86_REG_DI,
};

/* This enumeration refers to indices within softx86_ctx.ptr_regs_8reg[],
   which are set at run time to point to the representative portion of
   softx86_ctx.state.general_reg[] */
enum softx86_genregidx8 {
	SX86_REG_AL=0,
	SX86_REG_CL,
	SX86_REG_DL,
	SX86_REG_BL,
	SX86_REG_AH,
	SX86_REG_CH,
	SX86_REG_DH,
	SX86_REG_BH,
};

/* seg-reg IDs used in this library to refer to softx86_ctx.state.segment_reg[] */
enum softx86_segregidx {
	SX86_SREG_ES=0,
	SX86_SREG_CS,
	SX86_SREG_SS,
	SX86_SREG_DS,
};

typedef struct {
/************************ VISIBLE REGISTERS ****************************/
/*--------------------general purpose registers------------------------*/
	softx86_regval		general_reg[8];
/*-------------------------segment registers---------------------------*/
	softx86_segregval	segment_reg[8];
/*-----------------instruction and FLAGS registers---------------------*/
	softx86_regval		reg_flags;
	sx86_udword		reg_ip;
/*-----------------control/machine state registers---------------------*/
	sx86_udword		reg_cr0;
/************* INVISIBLE REGISTERS (FOR CACHING PURPOSES) **************/
/*------------instruction pointer saved before execution---------------*/
	sx86_uword		reg_cs_exec_initial;	/* these are copied from CS:IP before executing an instruction */
	sx86_udword		reg_ip_exec_initial;	/* <------'                                                    */
/*----------------instruction pointer for decompiler-------------------*/
	sx86_uword		reg_cs_decompiler;
	sx86_udword		reg_ip_decompiler;
/*-------------------interrupt flags-----------------------------------*/
	sx86_ubyte		int_hw_flag;		/* flag to indicate hardware (external) interrupt pending */
	sx86_ubyte		int_hw;			/* if int_hw_flag==1, which interrupt is being signalled */
	sx86_ubyte		int_nmi;		/* flag to indicate NMI hardware (external) interrupt pending */
/*-------------------other internal flags------------------------------*/
	sx86_ubyte		rep_flag;		/* 0=no REP loop, 1=REPNZ, 2=REPZ */
	sx86_ubyte		is_segment_override;	/* 1=segment override was encountered */
	sx86_uword		segment_override;	/* value of segment that overrides the default */
} softx86_cpustate;

typedef struct {
/* I/O */
	void			(*on_read_memory)	(/* softx86_ctx */ void* _ctx,sx86_udword address,sx86_ubyte *buf,int size);		/* called to fetch from memory <size> bytes from address <address> */
	void			(*on_read_io)		(/* softx86_ctx */ void* _ctx,sx86_udword address,sx86_ubyte *buf,int size);		/* called to fetch from I/O <size> bytes from address <address> */
	void			(*on_write_memory)	(/* softx86_ctx */ void* _ctx,sx86_udword address,sx86_ubyte *buf,int size);		/* called to write to memory <size> bytes from address <address> */
	void			(*on_write_io)		(/* softx86_ctx */ void* _ctx,sx86_udword address,sx86_ubyte *buf,int size);		/* called to write to I/O <size> bytes from address <address> */
/* interrupts */
	void			(*on_hw_int)		(/* softx86_ctx */ void* _ctx,sx86_ubyte i);						/* called when an external (to this library) interrupt occurs */
	void			(*on_sw_int)		(/* softx86_ctx */ void* _ctx,sx86_ubyte i);						/* called when an internal (emulated software) interrupt occurs */
	void			(*on_hw_int_ack)	(/* softx86_ctx */ void* _ctx,sx86_ubyte i);						/* called when the CPU (this library) acknowledges the interrupt */
	void			(*on_nmi_int)		(/* softx86_ctx */ void* _ctx);								/* called when an external (to this library) NMI interrupt occurs */
	void			(*on_nmi_int_ack)	(/* softx86_ctx */ void* _ctx);								/* called when the CPU (this library) acknowledges the NMI interrupt */
/* host-to-CPU interaction */
	void			(*on_idle_cycle)	(/* softx86_ctx */ void* _ctx);								/* called so host application can do finer-grained things during the execution of one instruction */
	void			(*on_reset)		(/* softx86_ctx */ void* _ctx);								/* informs host application if CPU reset itself or has been reset */
/* CPU-to-FPU interaction */
	int			(*on_fpu_opcode_exec)	(/* softx86_ctx */ void* _ctx86,/* softx87_ctx */ void* _ctx87,sx86_ubyte opcode);					/* called when an FPU opcode is encountered during execution */
	int			(*on_fpu_opcode_dec)	(/* softx86_ctx */ void* _ctx86,/* softx87_ctx */ void* _ctx87,sx86_ubyte opcode,char buf[128]);			/* called when an FPU opcode is encountered during decompile */
} softx86_callbacks;

#ifndef bswap16
#define bswap16(x)			(((x&0xFF)<<8) | ((x>>8)&0xFF))
#endif //bswap16

/* segment to linear conversion routines */
#define SEGMENT_TO_LINEAR(x)		(x<<4)
#define SEGMENT_OFFSET_TO_LINEAR(s,o)	(SEGMENT_TO_LINEAR(x)+o)

/* force a given value to fit within the specified size */
#define MASK_BYTE_SIZE(x)		(x &= 0xFF)
#define MASK_WORD_SIZE(x)		(x &= 0xFFFF)
#define MASK_DWORD_SIZE(x)		(x &= 0xFFFFFFFFL)
#define FORCE_BYTE_SIZE(x)		(x &  0xFF)
#define FORCE_WORD_SIZE(x)		(x &  0xFFFF)
#define FORCE_DWORD_SIZE(x)		(x &  0xFFFFFFFFL)

/* sign-extending macros */
#define SGNEXT8(x)			(x | ((x & 0x80) ? 0xFFFFFF00 : 0))
#define SGNEXT16(x)			(x | ((x & 0x8000) ? 0xFFFF0000 : 0))

/*=====================================================================================
   Softx86 bug structure

   This structure is used to contain variables related to the emulation of various
   flaws and bugs in the CPU.

   Please do not modify these directly.
*/
typedef struct {
	sx86_ubyte			preemptible_after_prefix;	// 1=interrupts can occur between the prefix and the opcode (8088/8086 behavior)
	sx86_ubyte			decrement_sp_before_store;	// 1=(E)SP is decremented before storing
	sx86_ubyte			mask_5bit_shiftcount;		// 1=mask shift count to 5 bits (286+ behavior)
} softx86_bugs;

/*=====================================================================================
   Softx86 internal context structure

   This structure is used internally by softx86
*/
typedef struct {
#ifdef SX86_INSIDER
/* ------------------ if compiling softx86 library source code ---------------- */
	void*				opcode_table;		// main opcode table
	void*				opcode_table_sub_0Fh;	// alternate opcode table for instructions that start with 0Fh
	sx86_udword			addr_mask;		// used to limit addressing based on CPU level
	int				level;			// which CPU are we emulating?
	sx86_ubyte*			ptr_regs_8reg[8];	// pointer table to 8-bit regs given reg value in opcode
#else
/* ----------------------- if compiling outsider source code ------------------- */
	void*				nothing;
#endif
} softx86_internal;

/*=====================================================================================
   Softx86 context structure

   This structure is used to interact with the host application.
*/
typedef struct {
// version
	sx86_ubyte			version_hi;
	sx86_ubyte			version_lo;
	sx86_uword			version_sublo;
// publicly viewable/modifiable standard vars
	softx86_cpustate*		state;
	softx86_callbacks*		callbacks;
	softx86_bugs*			bugs;
// private vars
	softx86_internal*		__private;
// linkage vars
	void*				ref_softx87_ctx;	// link to context structure of softx87
// "user-defined" vars (whatever the host application wants to carry with this context structure)
	void*				user;
} softx86_ctx;

/* CPU emulation levels */
#define SX86_CPULEVEL_8088		0			// 8088
#define SX86_CPULEVEL_8086		1			// 8086
#define SX86_CPULEVEL_80186		2			// 80186
#define SX86_CPULEVEL_80286		3			// 80286

#define sx86_far_to_linear(ctx,seg,ofs)		((seg<<4)+ofs)

void sx86_exec_full_modregrm_rw(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,sx86_ubyte opswap,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val));
void sx86_exec_full_modregrm_ro(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,sx86_ubyte opswap,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val));
void sx86_exec_full_modsregrm_rw(softx86_ctx* ctx,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,sx86_ubyte opswap,sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val));
void sx86_exec_full_modrmonly_rw(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src));
void sx86_exec_full_modrmonly_rw_imm(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val),int sx);
void sx86_exec_full_modrmonly_rw_imm8(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val));
void sx86_exec_full_modrmonly_ro_imm(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src,sx86_uword val),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src,sx86_udword val),int sx);
void sx86_exec_full_modregrm_xchg(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm);
void sx86_exec_full_modregrm_lea(softx86_ctx* ctx,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm);
void sx86_exec_full_modrw_memx(softx86_ctx* ctx,sx86_ubyte mod,sx86_ubyte rm,int sz,void (*op64)(softx86_ctx* ctx,char *datz,int sz));
void sx86_exec_full_modrmonly_memx(softx86_ctx* ctx,sx86_ubyte mod,sx86_ubyte rm,int sz,void (*op64)(softx86_ctx* ctx,char *datz,int sz));
void sx86_exec_full_modrmonly_callfar(softx86_ctx* ctx,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,void (*op16)(softx86_ctx* ctx,sx86_uword seg,sx86_uword ofs),void (*op32)(softx86_ctx* ctx,sx86_udword seg,sx86_udword ofs));
void sx86_exec_full_modrmonly_ro(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,sx86_ubyte (*op8)(softx86_ctx* ctx,sx86_ubyte src),sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword src),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword src));
void sx86_exec_full_modrmonly_wo(softx86_ctx* ctx,sx86_ubyte w16,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte rm,sx86_ubyte (*op8)(softx86_ctx* ctx),sx86_uword (*op16)(softx86_ctx* ctx),sx86_udword (*op32)(softx86_ctx* ctx));
void sx86_exec_full_modregrm_far(softx86_ctx* ctx,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,sx86_uword (*op16)(softx86_ctx* ctx,sx86_uword seg,sx86_uword ofs),sx86_udword (*op32)(softx86_ctx* ctx,sx86_udword seg,sx86_udword ofs));
void sx86_exec_full_modregrm_far_ro3(softx86_ctx* ctx,sx86_ubyte d32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,sx86_sword (*op16)(softx86_ctx* ctx,sx86_sword idx,sx86_sword upper,sx86_sword lower),sx86_sdword (*op32)(softx86_ctx* ctx,sx86_sdword idx,sx86_sdword upper,sx86_sdword lower));

void sx86_dec_full_modrmonly(softx86_ctx* ctx,sx86_ubyte is_word,sx86_ubyte dat32,sx86_ubyte mod,sx86_ubyte rm,char* op1);
void sx86_dec_full_modregrm(softx86_ctx* cpu,sx86_ubyte is_word,sx86_ubyte dat32,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,char* op1,char* op2);
void sx86_dec_full_modsregrm(softx86_ctx* cpu,sx86_ubyte mod,sx86_ubyte reg,sx86_ubyte rm,char* op1,char* op2);

void softx86_bswap2(sx86_ubyte *x);
void softx86_bswap4(sx86_ubyte *x);

#if SX86_BYTE_ORDER == LE
#define SWAP_WORD_FROM_LE(x)
#define SWAP_DWORD_FROM_LE(x)
#else
#define SWAP_WORD_FROM_LE(x)		softx86_bswap2((sx86_ubyte*)(&x));
#define SWAP_DWORD_FROM_LE(x)		softx86_bswap4((sx86_ubyte*)(&x));
#endif

#define SWAP_WORD_TO_LE(x)		SWAP_WORD_FROM_LE(x)
#define SWAP_DWORD_TO_LE(x)		SWAP_DWORD_FROM_LE(x)

/* API */
int softx86_getversion(int *major,int *minor,int *subminor);
int softx86_init(softx86_ctx* ctx,int level);
int softx86_reset(softx86_ctx* ctx);
int softx86_free(softx86_ctx* ctx);
int softx86_step(softx86_ctx* ctx);
int softx86_decompile(softx86_ctx* ctx,char asmbuf[256]);
int softx86_decompile_exec_cs_ip(softx86_ctx* ctx);
/* API for stack */
sx86_uword softx86_stack_popw(softx86_ctx* ctx);
void softx86_stack_discard_n(softx86_ctx* ctx,int bytez);
void softx86_stack_add_n(softx86_ctx* ctx,int bytez);
void softx86_stack_pushw(softx86_ctx* ctx,sx86_uword data);
/* API for CPU itself in terms of fetching data properly */
int softx86_fetch(softx86_ctx* ctx,void *reserved,sx86_udword addr,void *buf,int sz);
int softx86_write(softx86_ctx* ctx,void *reserved,sx86_udword addr,void *buf,int sz);
sx86_ubyte softx86_fetch_exec_byte(softx86_ctx* ctx);
sx86_ubyte softx86_fetch_dec_byte(softx86_ctx* ctx);
/* API for setting segment register values */
int softx86_setsegval(softx86_ctx* ctx,int sreg_id,sx86_udword x);
/* API for setting important pointers (NOTE: use these instead of directly
 * modifying the register values please, as it will make future development
 * of instruction queues, caches, and pipelines much easier to manage in
 * terms of CS:IP and SS:SP) */
int softx86_set_instruction_ptr(softx86_ctx* ctx,sx86_udword cs,sx86_udword ip);
int softx86_set_instruction_dec_ptr(softx86_ctx* ctx,sx86_udword cs,sx86_udword ip);
int softx86_set_stack_ptr(softx86_ctx* ctx,sx86_udword ss,sx86_udword sp);
int softx86_set_near_instruction_ptr(softx86_ctx* ctx,sx86_udword ip);
/* get an interrupt vector */
int softx86_get_intvect(softx86_ctx* ctx,sx86_ubyte i,sx86_uword *seg,sx86_uword *ofs);
int softx86_set_intvect(softx86_ctx* ctx,sx86_ubyte i,sx86_uword  seg,sx86_uword  ofs);
/* save instruction pointer on stack, get int vector, set instruction pointer there */
void softx86_go_int_frame(softx86_ctx* ctx,sx86_ubyte i);
/* generate stack frame and (optionally) set the instruction pointer */
int softx86_make_simple_near_call(softx86_ctx* ctx,sx86_udword* ip);
int softx86_make_simple_far_call(softx86_ctx* ctx,sx86_udword* cs,sx86_udword* ip);
int softx86_make_simple_interrupt_call(softx86_ctx* ctx,sx86_udword* cs,sx86_udword* ip);
/* macros for more advanced versions of the above functions (param1, param2, etc...
   refers to the number of parameters passed to the function). */
/*---------------------- "typical" refers to the fact that most
  ---------------------- C/C++ compilers for DOS and Win16 (ignoring
  ---------------------- those that use the CPU registers) use
  ---------------------- a simple calling convention where the
  ---------------------- parameters are pushed onto the stack
  ---------------------- in the opposite of the order they
  ---------------------- appear in the parameter list. __pascal, __stdcall,
  ---------------------- __cdecl, and the like all share this
  ---------------------- apparently (the difference is how the
  ---------------------- stack is cleaned up) */
#define softx86_make_typical_near_call_param1(ctx,ip,p1)\
	softx86_stack_pushw(ctx,p1);\
	softx86_make_simple_near_call(ctx,ip);

#define softx86_make_typical_near_call_param2(ctx,ip,p1,p2)\
	softx86_stack_pushw(ctx,p2);\
	softx86_stack_pushw(ctx,p1);\
	softx86_make_simple_near_call(ctx,ip);

#define softx86_make_typical_near_call_param3(ctx,ip,p1,p2,p3)\
	softx86_stack_pushw(ctx,p3);\
	softx86_stack_pushw(ctx,p2);\
	softx86_stack_pushw(ctx,p1);\
	softx86_make_simple_near_call(ctx,ip);

#define softx86_make_typical_near_call_param4(ctx,ip,p1,p2,p3,p4)\
	softx86_stack_pushw(ctx,p4);\
	softx86_stack_pushw(ctx,p3);\
	softx86_stack_pushw(ctx,p2);\
	softx86_stack_pushw(ctx,p1);\
	softx86_make_simple_near_call(ctx,ip);


#define softx86_make_typical_far_call_param1(ctx,cs,ip,p1)\
	softx86_stack_pushw(ctx,p1);\
	softx86_make_simple_far_call(ctx,cs,ip);

#define softx86_make_typical_far_call_param2(ctx,cs,ip,p1,p2)\
	softx86_stack_pushw(ctx,p2);\
	softx86_stack_pushw(ctx,p1);\
	softx86_make_simple_far_call(ctx,cs,ip);

#define softx86_make_typical_far_call_param3(ctx,cs,ip,p1,p2,p3)\
	softx86_stack_pushw(ctx,p3);\
	softx86_stack_pushw(ctx,p2);\
	softx86_stack_pushw(ctx,p1);\
	softx86_make_simple_far_call(ctx,cs,ip);

#define softx86_make_typical_far_call_param4(ctx,cs,ip,p1,p2,p3,p4)\
	softx86_stack_pushw(ctx,p4);\
	softx86_stack_pushw(ctx,p3);\
	softx86_stack_pushw(ctx,p2);\
	softx86_stack_pushw(ctx,p1);\
	softx86_make_simple_far_call(ctx,cs,ip);


/* mask constants for CR0 (286+ compatible) */
#define SX86_CR0_LMSW_MASK		0x0000000E
#define SX86_CR0FLAG_PE			0x00000001
#define SX86_CR0FLAG_MP			0x00000002
#define SX86_CR0FLAG_EM			0x00000004
#define SX86_CR0FLAG_TS			0x00000008
/* mask constants for CR0 (386+ compatible) */
#define SX86_CR0FLAG_ET			0x00000010
#define SX86_CR0FLAG_NE			0x00000020
#define SX86_CR0FLAG_WP			0x00010000
#define SX86_CR0FLAG_AM			0x00040000
#define SX86_CR0FLAG_NW			0x20000000
#define SX86_CR0FLAG_CD			0x40000000
#define SX86_CR0FLAG_PG			0x80000000

/* mask constants for CPU flags */
#define SX86_CPUFLAG_CARRY		0x00000001
#define SX86_CPUFLAG_RESERVED_01	0x00000002
#define SX86_CPUFLAG_PARITY		0x00000004
#define SX86_CPUFLAG_RESERVED_03	0x00000008
#define SX86_CPUFLAG_AUX		0x00000010
#define SX86_CPUFLAG_RESERVED_05	0x00000020
#define SX86_CPUFLAG_ZERO		0x00000040
#define SX86_CPUFLAG_SIGN		0x00000080
#define SX86_CPUFLAG_TRAP		0x00000100
#define SX86_CPUFLAG_INTENABLE		0x00000200
#define SX86_CPUFLAG_DIRECTIONREV	0x00000400
#define SX86_CPUFLAG_OVERFLOW		0x00000800
#define SX86_CPUFLAG_IOPL		0x00003000
#define SX86_CPUFLAG_IOPLSET(x)		(((x&3))<<12)
#define SX86_CPUFLAG_IOPLGET(x)		((x>>12)&3)
#define SX86_CPUFLAG_NESTEDTASK		0x00004000
#define SX86_CPUFLAG_RESERVED_15	0x00008000
#define SX86_CPUFLAG_RESUME		0x00010000
#define SX86_CPUFLAG_V86		0x00020000
#define SX86_CPUFLAG_ALIGNMENTCHK	0x00040000
#define SX86_CPUFLAG_RESERVED_19	0x00080000
#define SX86_CPUFLAG_RESERVED_20	0x00100000
#define SX86_CPUFLAG_RESERVED_21	0x00200000
#define SX86_CPUFLAG_RESERVED_22	0x00400000
#define SX86_CPUFLAG_RESERVED_23	0x00800000
#define SX86_CPUFLAG_RESERVED_24	0x01000000
#define SX86_CPUFLAG_RESERVED_25	0x02000000
#define SX86_CPUFLAG_RESERVED_26	0x04000000
#define SX86_CPUFLAG_RESERVED_27	0x08000000
#define SX86_CPUFLAG_RESERVED_28	0x10000000
#define SX86_CPUFLAG_RESERVED_29	0x20000000
#define SX86_CPUFLAG_RESERVED_30	0x40000000
#define SX86_CPUFLAG_RESERVED_31	0x80000000

#define SX86_CPUFLAGBO_CARRY		0
#define SX86_CPUFLAGBO_RESERVED_01	1
#define SX86_CPUFLAGBO_PARITY		2
#define SX86_CPUFLAGBO_RESERVED_03	3
#define SX86_CPUFLAGBO_AUX		4
#define SX86_CPUFLAGBO_RESERVED_05	5
#define SX86_CPUFLAGBO_ZERO		6
#define SX86_CPUFLAGBO_SIGN		7
#define SX86_CPUFLAGBO_TRAP		8
#define SX86_CPUFLAGBO_INTENABLE	9
#define SX86_CPUFLAGBO_DIRECTIONREV	10
#define SX86_CPUFLAGBO_OVERFLOW		11
#define SX86_CPUFLAGBO_IOPL		12
#define SX86_CPUFLAGBO_NESTEDTASK	14
#define SX86_CPUFLAGBO_RESERVED_15	15
#define SX86_CPUFLAGBO_RESUME		16
#define SX86_CPUFLAGBO_V86		17
#define SX86_CPUFLAGBO_ALIGNMENTCHK	18
#define SX86_CPUFLAGBO_RESERVED_19	19
#define SX86_CPUFLAGBO_RESERVED_20	20
#define SX86_CPUFLAGBO_RESERVED_21	21
#define SX86_CPUFLAGBO_RESERVED_22	22
#define SX86_CPUFLAGBO_RESERVED_23	23
#define SX86_CPUFLAGBO_RESERVED_24	24
#define SX86_CPUFLAGBO_RESERVED_25	25
#define SX86_CPUFLAGBO_RESERVED_26	26
#define SX86_CPUFLAGBO_RESERVED_27	27
#define SX86_CPUFLAGBO_RESERVED_28	28
#define SX86_CPUFLAGBO_RESERVED_29	29
#define SX86_CPUFLAGBO_RESERVED_30	30
#define SX86_CPUFLAGBO_RESERVED_31	31

/* default callback functions within the library */
void softx86_step_def_on_read_memory(void* _ctx,sx86_udword address,sx86_ubyte *buf,int size);
void softx86_step_def_on_read_io(void* _ctx,sx86_udword address,sx86_ubyte *buf,int size);
void softx86_step_def_on_write_memory(void* _ctx,sx86_udword address,sx86_ubyte *buf,int size);
void softx86_step_def_on_write_io(void* _ctx,sx86_udword address,sx86_ubyte *buf,int size);
void softx86_step_def_on_hw_int(void* _ctx,sx86_ubyte i);
void softx86_step_def_on_sw_int(void* _ctx,sx86_ubyte i);
void softx86_step_def_on_hw_int_ack(void* _ctx,sx86_ubyte i);
void softx86_step_def_on_idle_cycle(void* _ctx);
void softx86_step_def_on_nmi_int(void* _ctx);
void softx86_step_def_on_nmi_int_ack(void* _ctx);
int  softx86_step_def_on_fpu_opcode_exec(void* _ctx86,void* _ctx87,sx86_ubyte opcode);
int  softx86_step_def_on_fpu_opcode_dec(void* _ctx86,void* _ctx87,sx86_ubyte opcode,char buf[128]);
void softx86_step_def_on_reset(void* _ctx);

/* external/internal/NMI interrupt generation and acknowledgement */
int softx86_ext_hw_signal(softx86_ctx* ctx,sx86_ubyte i);
int softx86_ext_hw_ack(softx86_ctx* ctx);
int softx86_int_sw_signal(softx86_ctx* ctx,sx86_ubyte i);
int softx86_ext_hw_nmi_signal(softx86_ctx* ctx);
int softx86_ext_hw_nmi_ack(softx86_ctx* ctx);

/* parity flag computation routines */
int softx86_parity8(sx86_ubyte ret);
int softx86_parity16(sx86_uword ret);
int softx86_parity32(sx86_udword ret);
int softx86_parity64(sx86_uldword ret);

/* bug emulation constants. each is meant to be unique. */
#define SX86_BUG_PREEMPTIBLE_AFTER_PREFIX	0x12340500
#define SX86_BUG_SP_DECREMENT_BEFORE_STORE	0x12340600	// specific to all prior to 286
#define SX86_BUG_5BIT_SHIFTMASK			0x12340700	// 286+ behavior of masking shift count to 5 bits

/* use this to change values in the bug emulation structure */
int softx86_setbug(softx86_ctx* ctx,sx86_udword bug_id,sx86_ubyte on_off);

/* mod/reg/rm unpacking */
#define sx86_modregrm_unpack(mrr,mod,reg,rm)	mod = (mrr>>6);\
						reg = (mrr>>3)&7;\
						rm  =  mrr&7;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //SOFTX86_H

