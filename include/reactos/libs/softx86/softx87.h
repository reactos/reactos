
#ifndef SOFTX87_H
#define SOFTX87_H

/* get the Softx86 configuration defines and Softx86 headers */
#include "softx86.h"
#include <softx86cfg.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// version composed as follows: H.LL.SSSS
// where: H = major version number (any length)
//        L = minor version number (2 digits)
//        S = sub-minor version number (4 digits)
//
// This is v0.00.0033
#define SOFTX87_VERSION_HI		0
#define SOFTX87_VERSION_LO		0
#define SOFTX87_VERSION_SUBLO		33

#ifdef __MINGW32__  // MINGW

typedef unsigned char				sx87_ubyte;
typedef signed char				sx87_sbyte;
typedef unsigned short int			sx87_uword;
typedef signed short int			sx87_sword;
typedef unsigned int				sx87_udword;
typedef signed int				sx87_sdword;
typedef unsigned long long int			sx87_uldword;
typedef signed long long int			sx87_sldword;

#elif WIN32		// MSVC

typedef unsigned char				sx87_ubyte;
typedef signed char				sx87_sbyte;
typedef unsigned short int			sx87_uword;
typedef signed short int			sx87_sword;
typedef unsigned int				sx87_udword;
typedef signed int				sx87_sdword;
typedef unsigned __int64			sx87_uldword;
typedef signed __int64				sx87_sldword;

#else				// GCC 3.xx.xx

typedef unsigned char				sx87_ubyte;
typedef signed char				sx87_sbyte;
typedef unsigned short int			sx87_uword;
typedef signed short int			sx87_sword;
typedef unsigned int				sx87_udword;
typedef signed int				sx87_sdword;
typedef unsigned long long int			sx87_uldword;
typedef signed long long int			sx87_sldword;

#endif

/* NOTE: Each FPU register is maintained in a format similar but not
         quite the same as the 80-bit format documented by Intel to
	 be the "internal" format used by the FPU.

         We store the sign, mantissa, and exponent differently.

Intel documents the 80-bit register to be stored like this:

bit     79: sign
bits 64-78: exponent
bit   0-63: mantissa.

This is referred to in their documentation as "double extended precision F.P"
 */
typedef struct {
	sx86_uldword		mantissa;
	sx86_uword		exponent;
	sx86_ubyte		sign_bit;
} softx87_reg80;

/*============================================================================
   FPU state variables.

   WARNING: This structure is likely to change constantly over time, do not
            rely on this structure staying the same from version to version.
*/

typedef struct {
	sx86_udword		offset;
	sx86_uword		segment;
} softx87_ptr;

typedef struct {
	sx86_uword		status_word;			/* status word */
	sx86_uword		control_word;			/* control word */
	sx86_uword		tag_word;			/* "TAG" word */
	softx87_ptr		data_pointer;			/* ? */
	softx87_ptr		last_instruction;		/* last instruction address */
	softx87_ptr		last_instruction_memptr;	/* last instruction memory reference */
	sx87_uword		last_opcode;			/* last FPU opcode */
	softx87_reg80		st[8];				/* registers ST(0) thru ST(7) */
} softx87_fpustate;

typedef struct {
/* I/O */
	void			(*on_read_memory)			(/* softx87_ctx */ void* _ctx,sx87_udword address,sx87_ubyte *buf,int size);		/* called to fetch from memory <size> bytes from address <address> */
	void			(*on_write_memory)			(/* softx87_ctx */ void* _ctx,sx87_udword address,sx87_ubyte *buf,int size);		/* called to write to memory <size> bytes from address <address> */
/* callbacks to softx86 for fetching more opcodes */
	sx86_ubyte		(*on_softx86_fetch_exec_byte)		(softx86_ctx *ctx);
	sx86_ubyte		(*on_softx86_fetch_dec_byte)		(softx86_ctx *ctx);
	void			(*on_sx86_exec_full_modrmonly_memx)	(softx86_ctx *ctx,sx86_ubyte mod,sx86_ubyte rm,int sz,void (*op64)(softx86_ctx* ctx,char *datz,int sz));
	void			(*on_sx86_exec_full_modrw_memx)		(softx86_ctx* ctx,sx86_ubyte mod,sx86_ubyte rm,int sz,void (*op64)(softx86_ctx* ctx,char *datz,int sz));
	void			(*on_sx86_dec_full_modrmonly)		(softx86_ctx *ctx,sx86_ubyte is_word,sx86_ubyte dat32,sx86_ubyte mod,sx86_ubyte rm,char* op1);
} softx87_callbacks;

/*=====================================================================================
   Softx87 bug structure

   This structure is used to contain variables related to the emulation of various
   flaws and bugs in the FPU.

   Please do not modify these directly.
*/
typedef struct {
	sx87_ubyte			ip_ignores_prefix;		// 1=8087 FPU behavior where FPU last instruction pointer ignores the 8086 prefixes
} softx87_bugs;

/*=====================================================================================
   Softx87 context structure

   This structure is used to interact with the host application.
*/
typedef struct {
// version
	sx87_ubyte			version_hi;
	sx87_ubyte			version_lo;
	sx87_uword			version_sublo;
// other
	softx87_fpustate		state;
	softx87_callbacks		callbacks;
	softx87_bugs			bugs;
	void*				opcode_table;
	int				level;
	softx86_ctx*			ref_softx86;
} softx87_ctx;

/* FPU emulation levels */
#define SX87_FPULEVEL_8087		0			// 8088
#define SX87_FPULEVEL_80287		1			// 80287
#define SX87_FPULEVEL_80387		2			// 80387
#define SX87_FPULEVEL_80487		3			// 80487

/* Types as returned by softx87_get_fpu_register_double() */
#define SX87_FPU_NUMTYPE_NUMBER		0			// a number
#define SX87_FPU_NUMTYPE_NEGINF		1			// -infinity
#define SX87_FPU_NUMTYPE_POSINF		2			// +infinity
#define SX87_FPU_NUMTYPE_NAN		3			// NaN

/* API */
int softx87_getversion(int *major,int *minor,int *subminor);
int softx87_init(softx87_ctx* ctx,int level);
int softx87_reset(softx87_ctx* ctx);
int softx87_free(softx87_ctx* ctx);
double softx87_get_fpu_double(softx87_ctx* ctx,softx87_reg80 *reg,int *numtype);
void softx87_set_fpu_double(softx87_ctx* ctx,softx87_reg80 *reg,double val);
double softx87_get_fpu_register_double(softx87_ctx* ctx,int i,int *numtype);
void softx87_set_fpu_register_double(softx87_ctx* ctx,int i,double val);
void softx87_finit_setup(softx87_ctx* ctx);
/* callbacks intended for reference to softx86 */
int softx87_on_fpu_opcode_exec(/* softx86_ctx */ void* _ctx86,/* softx87_ctx */ void* _ctx87,sx86_ubyte opcode);
int softx87_on_fpu_opcode_dec(/* softx86_ctx */ void* _ctx86,/* softx87_ctx */ void* _ctx87,sx86_ubyte opcode,char buf[128]);

/* mask constants for FPU status */
/*-----------exception flags----------*/
#define SX87_FPUSTAT_INVALID_OP		0x0001
#define SX87_FPUSTAT_DENORMAL		0x0002
#define SX87_FPUSTAT_ZERO_DIVIDE	0x0004
#define SX87_FPUSTAT_OVERFLOW		0x0008
#define SX87_FPUSTAT_UNDERFLOW		0x0010
#define SX87_FPUSTAT_PRECISION		0x0020
/*-----------more flags---------------*/
#define SX87_FPUSTAT_STACK_FAULT	0x0040
#define SX87_FPUSTAT_ERROR_SUMMARY	0x0080
/*-----------condition codes----------*/
#define SX87_FPUSTAT_C0			0x0100
#define SX87_FPUSTAT_C1			0x0200
#define SX87_FPUSTAT_C2			0x0400
#define SX87_FPUSTAT_TOP_MASK		0x3800
#define SX87_FPUSTAT_TOP(x)		(((x)>>11)&7)
#define SX87_FPUSTAT_TOP_SET(x,y)	(x = (((x)&(~SX87_FPUSTAT_TOP_MASK)) | (((y)&7)<<11)))
#define SX87_FPUSTAT_C3			0x4000
#define SX87_FPUSTAT_BUSY		0x8000

/* mask constants for FPU control word */
/*-----------exception flags----------*/
#define SX87_FPUCTRLW_INVALID_OP	0x0001
#define SX87_FPUCTRLW_DENORMAL		0x0002
#define SX87_FPUCTRLW_ZERO_DIVIDE	0x0004
#define SX87_FPUCTRLW_OVERFLOW		0x0008
#define SX87_FPUCTRLW_UNDERFLOW		0x0010
#define SX87_FPUCTRLW_PRECISION		0x0020
/*-----------precision control flag---*/
#define SX87_FPUCTRLW_PCTL_MASK		0x0300
#define SX87_FPUCTRLW_PCTL(x)		(((x)>>8)&3)
#define SX87_FPUCTRLW_PCTL_SET(x,y)	(x = (((x)&(~SX87_FPUCTRLW_PCTL_MASK)) | (((y)&3)<<8)))
/*-----------rounding control flag----*/
#define SX87_FPUCTRLW_RNDCTL_MASK	0x0C00
#define SX87_FPUCTRLW_RNDCTL(x)		(((x)>>10)&3)
#define SX87_FPUCTRLW_RNDCTL_SET(x,y)	(x = (((x)&(~SX87_FPUCTRLW_RNDCTL_MASK)) | (((y)&3)<<10)))
/*-----------infinity control flag----*/
#define SX87_FPUCTRLW_INFCTL		0x1000

/* precision control flag values */
#define SX87_FPUCTRLW_PCTL_24_BIT	0
#define SX87_FPUCTRLW_PCTL_RESERVED_1	1
#define SX87_FPUCTRLW_PCTL_53_BIT	2
#define SX87_FPUCTRLW_PCTL_64_BIT	3

/* rounding control flag values */
#define SX87_FPUCTRLW_RNDCTL_NEAREST	0	/* nearest */
#define SX87_FPUCTRLW_RNDCTL_DOWNINF	1	/* down towards -inf */
#define SX87_FPUCTRLW_RNDCTL_UPINF	2	/* down towards +inf */
#define SX87_FPUCTRLW_RNDCTL_ZERO	3	/* towards zero (truncate) */

/* mask constants for FPU tag word */
#define SX87_FPUTAGW_TAG(x,i)		(((x)>>(i*2))&3)
#define SX87_FPUTAGW_TAG_SET(x,i,y)	(x = (((x) & (~(3<<(i*2)))) | (((y)&3)<<(i*2))))
#define SX87_FPUTAGVAL_VALID		0
#define SX87_FPUTAGVAL_ZERO		1
#define SX87_FPUTAGVAL_SPECIAL		2
#define SX87_FPUTAGVAL_EMPTY		3

/* takes a ST(i) index and converts it to the true register index */
#define SX87_FPU_ST(TOP,i)		((TOP+i)&7)

/* default callback functions within the library */
void softx87_step_def_on_read_memory(void* _ctx,sx87_udword address,sx87_ubyte *buf,int size);
void softx87_step_def_on_write_memory(void* _ctx,sx87_udword address,sx87_ubyte *buf,int size);
sx86_ubyte softx87_def_on_softx86_fetch_exec_byte(softx86_ctx* ctx);
sx86_ubyte softx87_def_on_softx86_fetch_dec_byte(softx86_ctx* ctx);
void softx87_on_sx86_exec_full_modrmonly_memx(softx86_ctx *ctx,sx86_ubyte mod,sx86_ubyte rm,int sz,void (*op64)(softx86_ctx* ctx,char *datz,int sz));
void softx87_on_sx86_dec_full_modrmonly(softx86_ctx* ctx,sx86_ubyte is_word,sx86_ubyte dat32,sx86_ubyte mod,sx86_ubyte rm,char* op1);

/* bug emulation constants. each is meant to be unique. */
#define SX87_BUG_IP_IGNORES_PREFIX		0x56780100

/* use this to change values in the bug emulation structure */
int softx87_setbug(softx87_ctx* ctx,sx87_udword bug_id,sx87_ubyte on_off);

/* utility */
void softx87_normalize(softx87_ctx* ctx,softx87_reg80 *val);

/* loading/saving */
void softx87_unpack_raw_int16(softx87_ctx* ctx,sx87_ubyte *data,softx87_reg80 *v);
void softx87_unpack_raw_int32(softx87_ctx* ctx,sx87_ubyte *data,softx87_reg80 *v);
void softx87_unpack_raw_fp32(softx87_ctx* ctx,sx87_ubyte *data,softx87_reg80 *v);
void softx87_unpack_raw_fp64(softx87_ctx* ctx,sx87_ubyte *data,softx87_reg80 *v);

#ifdef __cplusplus
}
#endif //__cplusplus

/* softx87_connect_to_CPU(context)
   Connects Softx86 CPU to a Softx87 FPU.
   This function assumes that you want the CPU and FPU to
   share the same memory. Provided as a service for those
   who don't want to manually link pointers together and such. */
static int softx87_connect_to_CPU(softx86_ctx* cpu,softx87_ctx* fpu)
{
	if (!cpu || !fpu) return 0;

	cpu->ref_softx87_ctx =					fpu;
	fpu->ref_softx86 =					cpu;
	cpu->callbacks->on_fpu_opcode_exec =			softx87_on_fpu_opcode_exec;
	cpu->callbacks->on_fpu_opcode_dec =			softx87_on_fpu_opcode_dec;
	fpu->callbacks.on_read_memory =				cpu->callbacks->on_read_memory;
	fpu->callbacks.on_write_memory =			cpu->callbacks->on_write_memory;
	fpu->callbacks.on_softx86_fetch_dec_byte =		softx86_fetch_dec_byte;
	fpu->callbacks.on_softx86_fetch_exec_byte =		softx86_fetch_exec_byte;
	fpu->callbacks.on_sx86_dec_full_modrmonly =		sx86_dec_full_modrmonly;
	fpu->callbacks.on_sx86_exec_full_modrmonly_memx =	sx86_exec_full_modrmonly_memx;
	fpu->callbacks.on_sx86_exec_full_modrw_memx =		sx86_exec_full_modrw_memx;

	return 1;
}

#endif //SOFTX87_H

